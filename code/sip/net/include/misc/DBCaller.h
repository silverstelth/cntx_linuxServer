/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __DBCALLER_H__
#define __DBCALLER_H__

#include "misc/thread.h"
#include "misc/buf_fifo.h"
#include "misc/variable.h"

#include "misc/queryRequest.h"
#include "misc/dbProcTask.h"

namespace	SIPBASE
{
typedef std::vector<SIPBASE::IThread*> CThreadPool;

typedef std::list<CQueryResult *>		LSTQueryResult;

enum	EPARAMMODE { IN_PARAM, OUT_PARAM, INOUT_PARAM };
class CDBCaller
{
protected:
	//DBSTMT							*_DBStmt;
	CQueryRequest					_curReq;
	CQueryResult					_curResult;
	//int								_RunMode;

	CSynchronizedFIFO				_DBReqFifo;
	CSynchronizedFIFO				_DBReqFifoRest;
	CSynchronizedFIFO				_DBResultFifo;

	SIPBASE::CSynchronized<LSTQueryResult>		_lstQueryResult;

	SIPBASE::CSynchronized<SIPBASE::CThreadPool>		_DBThreadPool;
	
	ucstring						_ucLogin;
	ucstring						_ucPassword;
	ucstring						_ucDBHost;
	ucstring						_ucDBName;
	
	std::list<DBCONNECTION *>		_DBConnectionsForReserved;
	std::list<DBCONNECTION *>		_DBConnectionsRest;
	SIPBASE::CFairMutex				_mutexDBConnections;
	SIPBASE::TTime					_lastDBPingtime;

	uint32							_uCurrentDBRequestNumber;
	uint64							_uMisProcedDBRequestNumber;
	uint64							_uProcedDBRequestNumber;
	uint64							_uProcedDBRequestTime;
	SIPBASE::CFairMutex				_mutexDBRequestStatus;

	HDBC							_lastNotifyErrorhDBCLink;
	HSTMT							_lastNotifyErrorhStmt;
	SQLRETURN						_lastNotifyErrornRetError;
	bool							_bDBReconnectFlag;
protected:
	CQueryRequest *createRequest();
	DBCONNECTION* PopDBConnectionRest();
	void		AddNewDBRequest(tchar * sQuery, uint8 mode, uint8 prio, uint32 expiretime, TDBCompleteCallback completionCallback, const char * strInFormat, va_list param_list);
	void		DBReconnect();
	void		DBReconnectForError(HDBC hDBCLink, HSTMT hStmt, SQLRETURN nRetError);
	void		CheckRestDBReconectAndDo();

public:
	bool	init(uint32 uPoolSize);
	void	update();
	void	release();
	bool	connect(ucstring ucLogin, ucstring ucPassword, ucstring ucDBHost, ucstring ucDBName);
	void	disconnect();

	// if result == NULL, don't push resultinfo into ResultFIFO
	void	executeWithParam(tchar * sQuery, TDBCompleteCallback completionCallback, const char * strInFormat, ...);
	void	execute(tchar * sQuery, TDBCompleteCallback completionCallback = NULL, const char * strInFormat = "", ...);

	void	executeWithParam2(tchar * sQuery, uint32 expiretime, TDBCompleteCallback completionCallback, const char * strInFormat, ...);
	void	execute2(tchar * sQuery, uint32 expiretime, TDBCompleteCallback completionCallback = NULL, const char * strInFormat = "", ...);

	int		executeDirect(tchar * sQuery);

	//void	setCurRunMode(int nMode) { _RunMode	= nMode; };
	void	setCurRequest(CQueryRequest &req) { _curReq = req; };
	void	setCurResult(CQueryResult &result) { _curResult = result; };
	//bool	fetchResult(TDBResult &dbRes);
	//bool	moreResult();

public:
	// get/set function
	bool	GetDBRequestForProcessing(CQueryRequest *pRequest);
	CSynchronizedFIFO*	ReqQueue() { return &_DBReqFifo; };
	CSynchronizedFIFO*	ReqQueueRest() { return &_DBReqFifoRest; };
	CSynchronizedFIFO*	ResultQueue() { return &_DBResultFifo; };

	void				AddProcedDBRequest(uint32 tmProc);

	uint32				GetCurrentDBRequestNumber() { return _uCurrentDBRequestNumber; }
	uint64				GetMisProcedDBRequestNumber() { return _uMisProcedDBRequestNumber; }
	uint64				GetProcedDBRequestNumber() { return _uProcedDBRequestNumber; };
	uint64				GetProcedDBRequestTime() { return _uProcedDBRequestTime; };
	
	void				PushQueryResult(CQueryResult *result);
	CQueryResult *		PopQueryResult();

	//HANDLE				EmptyEvent() { return _hEmptyEvent; };
	//void				SetEmptyEvent() { SetEvent(_hEmptyEvent); };

	tstring				&Query() { return _curReq.Query(); };

	ucstring &			DBLoginName() { return _ucLogin; };
	ucstring &			DBPassword() { return _ucPassword; };
	ucstring &			DBHost() { return _ucDBHost; };
	ucstring &			DBName() { return _ucDBName; };

	DBCONNECTION		*GetRestDBConnection();
	void				ReleaseRestDBConnection(DBCONNECTION *restDBConnection);
	void				NotifyReconnect();
	void				NotifyDBError(HDBC hDBCLink, HSTMT hStmt, SQLRETURN nRetError);

	bool				IsIdle();
	void				WaitForIdle();
public:
	CDBCaller(ucstring ucLogin, ucstring ucPassword, ucstring ucDBHost, ucstring ucDBName);
	virtual	~CDBCaller(void);
};

extern	CVariable<unsigned int>	DBTimeout;

class CDBConnectionRest
{
public:
	CDBConnectionRest(CDBCaller *pDBCaller);
	virtual	~CDBConnectionRest();

	operator HDBC()
	{
		return (HDBC)(*_DBConnection);
	}

	operator ODBCConnection *()
	{
		return _DBConnection;
	}

protected:
	CDBCaller		*_DBCaller;
	DBCONNECTION	*_DBConnection;
private:
};
}
#endif // end of guard __DBCALLER_H__
