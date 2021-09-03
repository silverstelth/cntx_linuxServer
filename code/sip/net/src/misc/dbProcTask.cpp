/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/queryRequest.h"
#include "misc/DBCaller.h"
#include "misc/dbProcTask.h"
#include "misc/connectionPool.h"

namespace SIPBASE
{
typedef SIPBASE::CSmartPtr<CQueryResult> TResultPtr;

//////////////////////////////////////////////////////////////////////////
// CDBProcTask
CDBProcTask::CDBProcTask()
:_ExitRequired(false)
,_DBExecutor(NULL)
,_LastDBPingTime(0)
{
}

bool	CDBProcTask::init(CDBCaller *caller)
{
	_Caller		= caller;

	_DBExecutor = new CDBConnector;
	if ( !_DBExecutor )
		return false;
	return	_DBExecutor->connect(caller->DBLoginName(), caller->DBPassword(), caller->DBHost(), caller->DBName());
}

uint32	CDBProcTask::process()
{
	uint32	nProcNum = 0;
//	do 
//	{
		CQueryRequest	request;
		bool bIs = _Caller->GetDBRequestForProcessing(&request);
		if (bIs)
		{
			request.init();
			// execute
			CQueryResult	*result = new CQueryResult(request, _DBExecutor->DBConnection());
			//CQueryResult	result(request, _DBExecutor->DBConnection());
			TTime	t0 = CTime::getLocalTime();
			int		nRet = _DBExecutor->executeSP(request, result);
			TTime	t1 = CTime::getLocalTime();
			_Caller->AddProcedDBRequest((uint32)(t1-t0));

			nProcNum ++;

			if ( request.Mode() == EXECUTE_BLOCK )
			{
				request.setCompletionEvent();
				return nProcNum;
			}

			_Caller->PushQueryResult(result);
		}
//	} while (TRUE);
	return nProcNum;
}

void	CDBProcTask::ReConnectDB() 
{ 
	if (_DBExecutor)
		_DBExecutor->SetReconnectFlag(); 
}

HDBC	CDBProcTask::GetDBHandle()
{ 
	if (_DBExecutor)
	{
		DBCONNECTION &DB = _DBExecutor->DBConnection();
		return (HDBC)(DB);
	}
	return NULL;
}

void	CDBProcTask::run()
{
	while(!exitRequired())
	{
		if (_DBExecutor->IsReconnectFlag())
		{
			bool bRe = _DBExecutor->connect(_Caller->DBLoginName(), _Caller->DBPassword(), _Caller->DBHost(), _Caller->DBName());
			if (bRe == false)
			{
				sipSleep(1);
				continue;
			}
		}
		
		uint32 nProcNum = process();

		if (nProcNum == 0)
		{
			SIPBASE::TTime	curTime = SIPBASE::CTime::getLocalTime();
			if (curTime - _LastDBPingTime > 180000)	// 3 minutes
			{
				_DBExecutor->pingDB();
				_LastDBPingTime = curTime;
			}
			sipSleep(1);
		}
		else
		{
			_LastDBPingTime = SIPBASE::CTime::getLocalTime();
		}
	}
}

void	CDBProcTask::close()
{
	if ( _DBExecutor != NULL )
	{
		_DBExecutor->disconnect();
		delete _DBExecutor;
		_DBExecutor = NULL;
	}
}
} // SIPBASE
