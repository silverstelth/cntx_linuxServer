/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/DBCaller.h"
#include "misc/connectionPool.h"

#define		DECL_VAR_N(a, n, val)		int	a##i = val;
#define		VAR_N(a, i)					(a##i)

namespace	SIPBASE
{
//////////////////////////////////////////////////////////////////////////
// TPRAM
CVariable<unsigned int>	DBTimeout("DBCall",	"DBTimeout", "DB Blocking Timeout", 600000, 0, true);

static	std::map<CDBCaller*, CDBCaller*>	mapAllDBCaller;
//////////////////////////////////////////////////////////////////////////
// CDBCaller

CDBCaller::CDBCaller(ucstring ucLogin, ucstring ucPassword, ucstring ucDBHost, ucstring ucDBName)
:_DBReqFifo("CDBCaller::_DBReqFifo")
,_DBReqFifoRest("CDBCaller::_DBReqFifoRest")
,_DBResultFifo("CDBCaller::_DBResultFifo")
,_DBThreadPool("CDBCaller::_DBThreadPool")
,_lstQueryResult("CDBCaller::_lstQueryResult")
,_ucLogin(ucLogin)
,_ucPassword(ucPassword)
,_ucDBHost(ucDBHost)
,_ucDBName(ucDBName)
,_uCurrentDBRequestNumber(0)
,_uMisProcedDBRequestNumber(0)
,_uProcedDBRequestNumber(0)
,_uProcedDBRequestTime(0)

{
	mapAllDBCaller.insert(std::make_pair(this, this));
	_lastNotifyErrorhDBCLink = NULL;
	_lastNotifyErrorhStmt = NULL;
	_lastNotifyErrornRetError = SQL_SUCCESS;
	_bDBReconnectFlag = false;
}

CDBCaller::~CDBCaller(void)
{
	LSTQueryResult::iterator it;
	CSynchronized<LSTQueryResult>::CAccessor poolsync( &_lstQueryResult );
	for ( it=poolsync.value().begin(); it!=poolsync.value().end(); ++it )
	{
		CQueryResult *p = (*it);
		delete p;
	}
	poolsync.value().clear();

	mapAllDBCaller.erase(this);
}

bool	CDBCaller::init(uint32 uPoolSize)
{
	CDBProcTask		* DBTask;
	IThread			* DBThread;

	CSynchronized<CThreadPool>::CAccessor	poolsync( &_DBThreadPool );
	for ( uint32 i = 0; i < uPoolSize; i ++ )
	{
		DBTask			= new CDBProcTask;
		if ( !DBTask )
			return	false;
		
		sipinfo("dbCaller");
		DBThread		= IThread::create( DBTask, 1024*4*4 );
		if ( !DBTask->init(this) )
		{
			free(DBTask);
			free(DBThread);

			return	false;
		}

		DBThread->start();

		poolsync.value().push_back(DBThread);
	}

//////////////////////////////////////////////////////////////////////////
	for ( uint32 i = 0; i < 2; i ++ )
	{
		DBCONNECTION *pNew = new DBCONNECTION;
		BOOL bSuccess = pNew->Connect(	_ucLogin.c_str(), _ucPassword.c_str(), _ucDBHost.c_str(), _ucDBName.c_str());
		if (!bSuccess)
		{
			free(pNew);
			continue;
		}

		sipdebug("MainThread DBConnection i=%d, Handle=0x%x", i, pNew->m_hDBC);
		_DBConnectionsForReserved.push_back(pNew);
		_DBConnectionsRest.push_back(pNew);
	}
	if (_DBConnectionsForReserved.size() < 1)
	{
		siperrornoex(L"DBCaller's database connection for reserved failed to '%s' with login '%s' and database name '%s'",
			_ucDBHost.c_str(), _ucLogin.c_str(), _ucDBName.c_str());
		return false;
	}
	_lastDBPingtime = CTime::getLocalTime();

	return true;
}

void	CDBCaller::CheckRestDBReconectAndDo()
{
	_mutexDBConnections.enter();
	std::list<DBCONNECTION *>::iterator it;
	for (it = _DBConnectionsForReserved.begin(); it != _DBConnectionsForReserved.end(); it++)
	{
		DBCONNECTION *pDB = (*it);
		if (pDB->GetReconnectFlag())
		{
			pDB->Release();
			pDB->Connect(_ucLogin.c_str(), _ucPassword.c_str(), _ucDBHost.c_str(), _ucDBName.c_str());
		}
	}
	_mutexDBConnections.leave();
}

void	CDBCaller::DBReconnect()
{
	CSynchronized<CThreadPool>::CAccessor	poolsync( &_DBThreadPool );
	uint32	nSize = poolsync.value().size();

	CDBProcTask	* DBTask;
	IThread		* DBThread;
	for ( uint32 i = 0; i < nSize; i ++ )
	{
		DBThread= poolsync.value()[i];
		DBTask	= (CDBProcTask *)DBThread->getRunnable();
		DBTask->ReConnectDB();
	}

	_mutexDBConnections.enter();
	std::list<DBCONNECTION *>::iterator it;
	for (it = _DBConnectionsForReserved.begin(); it != _DBConnectionsForReserved.end(); it++)
	{
		DBCONNECTION *pDB = (*it);
		pDB->Release();
		pDB->Connect(_ucLogin.c_str(), _ucPassword.c_str(), _ucDBHost.c_str(), _ucDBName.c_str());
	}
	_mutexDBConnections.leave();
}

void	CDBCaller::NotifyReconnect()
{
	_bDBReconnectFlag = true;
}

void	CDBCaller::DBReconnectForError(HDBC hDBCLink, HSTMT hStmt, SQLRETURN nRetError)
{
	_mutexDBConnections.enter();
	std::list<DBCONNECTION *>::iterator it;
	for (it = _DBConnectionsForReserved.begin(); it != _DBConnectionsForReserved.end(); it++)
	{
		DBCONNECTION *pDB = (*it);
		if (hDBCLink == (HDBC)(*pDB))
		{
			pDB->Release();
			pDB->Connect(_ucLogin.c_str(), _ucPassword.c_str(), _ucDBHost.c_str(), _ucDBName.c_str());
		}
	}
	_mutexDBConnections.leave();
}

void	CDBCaller::NotifyDBError(HDBC hDBCLink, HSTMT hStmt, SQLRETURN nRetError)
{
	{
		CSynchronized<CThreadPool>::CAccessor	poolsync( &_DBThreadPool );
		uint32	nSize = poolsync.value().size();

		CDBProcTask	* DBTask;
		IThread		* DBThread;
		for ( uint32 i = 0; i < nSize; i ++ )
		{
			DBThread= poolsync.value()[i];
			DBTask	= (CDBProcTask *)DBThread->getRunnable();
			HDBC hDB = DBTask->GetDBHandle();
			if (hDB == hDBCLink)
				DBTask->ReConnectDB();
		}
	}

	_lastNotifyErrorhDBCLink = hDBCLink;
	_lastNotifyErrorhStmt = hStmt;
	_lastNotifyErrornRetError = nRetError;
}

void	CDBCaller::release()
{
	CSynchronized<CThreadPool>::CAccessor	poolsync( &_DBThreadPool );
	uint32	nSize = poolsync.value().size();

	CDBProcTask	* DBTask;
	IThread		* DBThread;
	for ( uint32 i = 0; i < nSize; i ++ )
	{
		DBThread= poolsync.value()[i];
		DBTask	= (CDBProcTask *)DBThread->getRunnable();
		DBTask->requireExit();
	}

	for ( uint32 i = 0; i < nSize; i ++ )
	{
		DBThread= poolsync.value()[i];
		DBTask	= (CDBProcTask *)DBThread->getRunnable();

#ifdef SIP_OS_UNIX
// 		DBTask->wakeUp();
#endif

		DBThread->wait();
		DBTask->close();
		delete DBThread;
		delete DBTask;
	}
	
	_mutexDBConnections.enter();
	std::list<DBCONNECTION *>::iterator it;
	for (it = _DBConnectionsForReserved.begin(); it != _DBConnectionsForReserved.end(); it++)
	{
		DBCONNECTION *pDB = (*it);
		pDB->Disconnect();
		delete pDB;
	}
	_DBConnectionsForReserved.clear();
	_DBConnectionsRest.clear();
	_mutexDBConnections.leave();
}

void	CDBCaller::AddNewDBRequest(tchar * sQuery, uint8 mode, uint8 prio, uint32 expiretime, TDBCompleteCallback completionCallback, const char * strInFormat, va_list param_list)
{
	tstring		strQuery(sQuery);
	CQueryRequest	queryReq(strQuery, completionCallback, mode, prio, expiretime);
	queryReq.init(strInFormat, param_list);

	CMemStream	s;		s.serial(queryReq);
	if (prio == ESP_NORMAL)
	{
		CFifoAccessor reqFifo( &_DBReqFifo );
		reqFifo.value().push(s);
	}
	else
	{
		CFifoAccessor reqFifoRest( &_DBReqFifoRest );
		reqFifoRest.value().push(s);
	}
	_uCurrentDBRequestNumber ++;
}

void	CDBCaller::executeWithParam(tchar * sQuery, TDBCompleteCallback completionCallback, const char * strInFormat, ...)
{
	va_list		param_list;
	va_start(param_list, strInFormat);

	AddNewDBRequest(sQuery, EXECUTE_PARAM, ESP_NORMAL, 0, completionCallback, strInFormat, param_list);

	va_end(param_list);

/*
	tstring		strQuery(sQuery);

	va_list		param_list;
	va_start(param_list, strInFormat);

	CQueryRequest	queryReq(strQuery, completionCallback, EXECUTE_PARAM, ESP_NORMAL);
	queryReq.init(strInFormat, param_list);

	CMemStream	s;		s.serial(queryReq);
	{
		CFifoAccessor reqFifo( &_DBReqFifo );
		reqFifo.value().push(s);		
	}

	va_end(param_list);
	*/
}

void	CDBCaller::execute(tchar * sQuery, TDBCompleteCallback completionCallback, const char * strInFormat, ...)
{
	va_list		param_list;
	va_start(param_list, strInFormat);

	AddNewDBRequest(sQuery, EXECUTE_NORMAL, ESP_NORMAL, 0, completionCallback, strInFormat, param_list);

	va_end(param_list);
/*
	tstring		strQuery(sQuery);

	va_list		param_list;
	va_start(param_list, strInFormat);

	CQueryRequest	queryReq(strQuery, completionCallback, EXECUTE_NORMAL, ESP_NORMAL);
	queryReq.init(strInFormat, param_list);

	CMemStream	s;		s.serial(queryReq);
	{
		CFifoAccessor reqFifo( &_DBReqFifo );
		reqFifo.value().push(s);		
	}

	va_end(param_list);
	*/
}

void	CDBCaller::executeWithParam2(tchar * sQuery, uint32 expiretime, TDBCompleteCallback completionCallback, const char * strInFormat, ...)
{
	va_list		param_list;
	va_start(param_list, strInFormat);

	AddNewDBRequest(sQuery, EXECUTE_PARAM, ESP_REST, expiretime, completionCallback, strInFormat, param_list);

	va_end(param_list);

}

void	CDBCaller::execute2(tchar * sQuery, uint32 expiretime, TDBCompleteCallback completionCallback, const char * strInFormat, ...)
{
	va_list		param_list;
	va_start(param_list, strInFormat);

	AddNewDBRequest(sQuery, EXECUTE_NORMAL, ESP_REST, expiretime, completionCallback, strInFormat, param_list);

	va_end(param_list);
}

int	CDBCaller::executeDirect(tchar * sQuery)
{
//	#error("Sorry, Not preared yet. But recommend you to prevent execute SP synchronously")

// 	tstring strQuery(sQuery);
// 	CQueryRequest	queryReq(strQuery, NULL, EXECUTE_BLOCK);
// 	queryReq.init();

	//CDBConnector	executor;

//	int nRet	= executeSP(queryReq);
/*

	CMemStream	s;		s.serial(queryReq);
	{
		CFifoAccessor reqFifo( &_DBReqFifo );
		reqFifo.value().push(s);
	}
	SetEvent(_hPushDataEvent);
	ResetEvent(_hEmptyEvent);

	int	nRet = 0;
	HANDLE	hPopEvt	= queryReq.Event();

	do 
	{
		if ( hPopEvt == NULL )
			break;

		DWORD	state	= WaitForSingleObject(hPopEvt, DBTimeout.get());
		if ( state == WAIT_OBJECT_0)
		{
			nRet	= executeSP(queryReq);
			_RunMode= EXECUTE_BLOCK;
			break;
		}

		sipwarning("FAIL Synchronize SP Call <%s>", strQuery.c_str());

		if ( state == WAIT_TIMEOUT)
			sipwarning("TIMEOUT %d(ms)", DBTimeout.get());

		break;
	} while (true);
*/
	//return	nRet;

	return	1;
}

// bool	CDBCaller::fetchResult(TDBResult &dbRes)
// {
// #ifdef	MYSQL_USE
// 	MYSQL_ROW row = DBStmt->FetchRow();
// #else
// 	HSTMT hStmt = INVALID_HANDLE_VALUE;
// 	DBSTMT	*DBStmt = _curResult.DBStmt();
// 	if (DBStmt->Fetch())
// 	{
// 		hStmt = (HSTMT)(*DBStmt);
// 	}
// 	ODBCRecord row(hStmt);
// #endif	
// 
// 	if (!IS_DB_ROW_VALID(row))
// 	{
// // 		if ( _RunMode == EXECUTE_BLOCK )
// // 			SetEvent(_hRunBlockingEvent);
// 		
// 		return false;
// 	}
// 	dbRes = row;
// 
// 	return true;
// }

// bool	CDBCaller::moreResult()
// {
// 	DBSTMT	*DBStmt = _curResult.DBStmt();
// 	return	( DBStmt->MoreResults() == TRUE );
// }

bool	CDBCaller::GetDBRequestForProcessing(CQueryRequest *pRequest)
{
	CFifoAccessor dbfifo(&_DBReqFifo);

	//sipassert( ! dbfifo.value().empty() );
	if ( dbfifo.value().empty() )
	{
		while (true)
		{
			CFifoAccessor dbfifoRest(&_DBReqFifoRest);
			if (dbfifoRest.value().empty())
				return false;
			else
			{
				CQueryRequest	tempRequest;
				CMemStream buffer(true);
				dbfifoRest.value().front( buffer );

				// pop
				buffer.serial(tempRequest);
				dbfifoRest.value().pop();
				_uCurrentDBRequestNumber --;
				uint32	exptime	= tempRequest.Expiretime();
				TTime	requesttm = tempRequest.RequestTime();
				TTime	currenttm = CTime::getLocalTime();
				if (exptime > 0 && currenttm - requesttm > exptime)
				{	
					_uMisProcedDBRequestNumber ++;
					tstring strQuery = tempRequest.Query();
					sipwarning(L"DBCall: DBRequest is time out: Query = %s", strQuery.c_str());
				}
				else
				{
					*pRequest = tempRequest;
					break;
				}
			}
		}
	}
	else
	{
		CMemStream buffer(true);
		dbfifo.value().front( buffer );

		// pop
		buffer.serial(*pRequest);
		dbfifo.value().pop();
		_uCurrentDBRequestNumber --;
	}

	return true;
}

void	CDBCaller::AddProcedDBRequest(uint32 tmProc)
{
	_mutexDBRequestStatus.enter();
	_uProcedDBRequestNumber ++;
	_uProcedDBRequestTime += tmProc;
	_mutexDBRequestStatus.leave();
}

void	CDBCaller::PushQueryResult(CQueryResult *result)
{
	CSynchronized<LSTQueryResult>::CAccessor poolsync( &_lstQueryResult );
	poolsync.value().push_back(result);
}

CQueryResult *	CDBCaller::PopQueryResult()
{
	CSynchronized<LSTQueryResult>::CAccessor poolsync( &_lstQueryResult );
	if (poolsync.value().size() < 1)
		return NULL;

	LSTQueryResult::iterator it = poolsync.value().begin();
	CQueryResult	*pTemp = (*it);
	poolsync.value().pop_front();
	return pTemp;
}

void	CDBCaller::update()
{
	do 
	{
		autoPtr<CQueryResult>	pResult ( PopQueryResult() );
		if (pResult.get() == NULL)
			break;

		TDBCompleteCallback	callback = pResult->Callback();
		if ( callback == NULL )
			continue;

		int		nSize	= pResult->ParamAry().size();
		void ** param	= NULL;
		if ( nSize )
		{
			param = new void *[nSize];
			//CSmartPtr<void *>	ptr;
			for ( int i = 0; i < nSize; i ++ )
			{
				param[i] = pResult->ParamAry()[i]->param();
			}
		}

		CMemStream	*resSet = pResult->ResultSet();
		resSet->invert();
		sipdebug(L"DBCALL: @@@@@@@@@@@@@@############### %s ###(Calling callback)", pResult->Query().c_str());
		callback(pResult->nResult(), nSize, param, resSet);

		if ( param != NULL )
			delete[] param;

	} while (true);

	// update db connection
	if (_bDBReconnectFlag)
	{
		DBReconnect();
		_bDBReconnectFlag = false;
	}
	else
	{
		if (_lastNotifyErrorhDBCLink != NULL)
		{
			DBReconnectForError(_lastNotifyErrorhDBCLink, _lastNotifyErrorhStmt, _lastNotifyErrornRetError);
			_lastNotifyErrorhDBCLink = NULL;
			_lastNotifyErrorhStmt = NULL;
			_lastNotifyErrornRetError = SQL_SUCCESS;
		}
	}
	CheckRestDBReconectAndDo();

	// ping db
	if (CTime::getLocalTime() - _lastDBPingtime > 180000) // 3 minutes
	{
		_mutexDBConnections.enter();
		std::list<DBCONNECTION *>::iterator it;
		for (it = _DBConnectionsForReserved.begin(); it != _DBConnectionsForReserved.end(); it++)
		{
			DBCONNECTION *pDB = (*it);
			if (pDB->IsConnected())
			{
				DB_EXE_QUERY((*pDB), DBStmt, L"SELECT GETDATE()");
			}
		}
		_mutexDBConnections.leave();

		_lastDBPingtime = CTime::getLocalTime();
	}
}

DBCONNECTION* CDBCaller::PopDBConnectionRest()
{
	DBCONNECTION* popDB = NULL;
	
	_mutexDBConnections.enter();
	if (_DBConnectionsRest.size() > 0)
	{
		std::list<DBCONNECTION *>::iterator itBegin = _DBConnectionsRest.begin();
		popDB = (*itBegin);
		if (popDB->GetReconnectFlag())
		{
			popDB->Release();
			popDB->Connect(_ucLogin.c_str(), _ucPassword.c_str(), _ucDBHost.c_str(), _ucDBName.c_str());
		}
		_DBConnectionsRest.pop_front();
	}
	_mutexDBConnections.leave();

	return popDB;
}

DBCONNECTION* CDBCaller::GetRestDBConnection()
{
	DBCONNECTION* restDBConnection = NULL;
	do 
	{
		restDBConnection = PopDBConnectionRest();
	} while (!restDBConnection || !(restDBConnection->IsConnected()));
		
	return restDBConnection;
}

void	CDBCaller::ReleaseRestDBConnection(DBCONNECTION *restDBConnection)
{
	_mutexDBConnections.enter();
	_DBConnectionsRest.push_back(restDBConnection);
	_mutexDBConnections.leave();
}

bool CDBCaller::IsIdle()
{
	CFifoAccessor dbfifo(&_DBReqFifo);
	if (!dbfifo.value().empty())
		return false;

	CFifoAccessor dbfifoRest(&_DBReqFifoRest);
	if (!dbfifoRest.value().empty())
		return false;

	CSynchronized<LSTQueryResult>::CAccessor poolsync( &_lstQueryResult );
	if (poolsync.value().size() > 0)
		return false;

	return true;
}

void CDBCaller::WaitForIdle()
{
	while (!IsIdle())
	{
		update();
		sipSleep(1);
	}

	sipSleep(100);
}

//////////////////////////////////////////////////////////////////////////
CDBConnectionRest::CDBConnectionRest(CDBCaller *pDBCaller)
{
	sipassert(pDBCaller);
	_DBCaller = pDBCaller;

	_DBConnection = _DBCaller->GetRestDBConnection();
}

CDBConnectionRest::~CDBConnectionRest()
{
	sipassert(_DBConnection);
	_DBCaller->ReleaseRestDBConnection(_DBConnection);
}

void NotifyDBErrorOccur(HDBC hDBCLink, HSTMT hStmt, SQLRETURN nRetError)
{
	std::map<CDBCaller*, CDBCaller*>::iterator it;
	for (it = mapAllDBCaller.begin(); it != mapAllDBCaller.end(); it++)
	{
		CDBCaller* pDB = it->first;
		pDB->NotifyDBError(hDBCLink, hStmt, nRetError);
	}
}

SIPBASE_CATEGORISED_COMMAND(sip, reconnectAllDB, "reconnect All DB", "")
{
	std::map<CDBCaller*, CDBCaller*>::iterator it;
	for (it = mapAllDBCaller.begin(); it != mapAllDBCaller.end(); it++)
	{
		CDBCaller* pDB = it->first;
		pDB->NotifyReconnect();
	}
	return true;
}

}
