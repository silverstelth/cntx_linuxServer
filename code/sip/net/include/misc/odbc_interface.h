/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __ODBC_INTERFACE_H__
#define __ODBC_INTERFACE_H__

#include <sql.h> 
#include <sqlext.h>
#include <odbcinst.h>
#ifdef SIP_OS_WINDOWS
#	include <tchar.h>
#else
#	include <string.h>
#endif
#include "string_common.h"
#include "ucstring.h"
#include "time_sip.h"
#include "debug.h"
//--
#pragma comment(lib,"odbc32.lib")
#pragma comment(lib,"odbcbcp.lib")
#pragma comment(lib,"OdbcCP32.Lib")
//--
#define IS_SQL_ERR !IS_SQL_OK
#define IS_SQL_OK(res) (res==SQL_SUCCESS_WITH_INFO || res==SQL_SUCCESS)
#define IS_SQL_EXCUTE_OK(res) (res==SQL_SUCCESS_WITH_INFO || res==SQL_SUCCESS || res==SQL_NO_DATA)
#define SAFE_STR(str) ((str==NULL) ? _t("") : str)
//--
enum sqlDataTypes
{
	sqlDataTypeUnknown=SQL_UNKNOWN_TYPE,
	sqlDataTypeChar=SQL_CHAR,
	sqlDataTypeNumeric=SQL_NUMERIC,
	sqlDataTypeDecimal=SQL_DECIMAL,
	sqlDataTypeInteger=SQL_INTEGER,
	sqlDataTypeSmallInt=SQL_SMALLINT,
	sqlDataTypeFloat=SQL_FLOAT,
	sqlDataTypeReal=SQL_REAL,
	sqlDataTypeDouble=SQL_DOUBLE,
#if (ODBCVER >= 0x0300)
	sqlDataTypeDateTime=SQL_DATETIME,
#endif
	sqlDataTypeVarChar=SQL_VARCHAR
};

static SQLTCHAR *toSQLString(const tchar* strSQL) {
	int strLen;
	SQLTCHAR *result;

	bool needSemicolon = false;
#ifdef SIP_USE_UNICODE
	ucstring strQuery(strSQL);
	wchar_t last = strQuery.at(strQuery.length() - 1);
	if (last != L';' && last != L'}')
		needSemicolon = true;
#else
	std::string strQuery(strSQL);
	char last = strQuery.at(strQuery.length() - 1);
	if (last != ';' && last != '}')
		needSemicolon = true;
#endif
	strLen = strQuery.length();
	result = (SQLTCHAR*)malloc((needSemicolon ? strLen + 2 : strLen + 1) * sizeof(SQLTCHAR));
	int i = 0;
	for (; i < strLen; i++) {
		result[i] = strQuery.at(i);
	}
	if (needSemicolon) {
		result[i++] = ';';
	}
	result[i] = '\0';
	return result;
};

class ODBCConnection
{
public:

	void GetDiagRec(SQLHANDLE *_handle)
	{
		SQLTCHAR 	SQLState[0x100], MessageText[0x100];
		SQLINTEGER 	NativeErrorPtr;
		SQLSMALLINT	TextLengthPtr = 0;
		SQLGetDiagRec(
			SQL_HANDLE_DBC,
			*_handle,
			1,
			SQLState,
			&NativeErrorPtr,
			MessageText,
			0x100,
			&TextLengthPtr);

#ifdef SIP_USE_UNICODE
		ucchar ucStrState[0x100], ucMessageText[0x100];
		for (int i = 0; i < 0x100; i++)
			ucStrState[i] = SQLState[i];
		
		for (int i = 0; i < 0x100; i++)
			ucMessageText[i] = MessageText[i];			
		
		sipinfo("Error : SQLState = %S, MessageText = %S, NativeErrorPtr = 0x%x", ucStrState, ucMessageText, NativeErrorPtr);
#else		
		sipinfo("Error : SQLState = %s, MessageText = %s, NativeErrorPtr = 0x%x", SQLState, MessageText, NativeErrorPtr);
#endif
	}

	bool Connect(lpctstr svSource)
	{
		int nConnect = SQLAllocHandle( SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv );
		if( nConnect == SQL_SUCCESS || nConnect == SQL_SUCCESS_WITH_INFO ) 
		{
			nConnect = SQLSetEnvAttr( m_hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, 0 );
			if( nConnect == SQL_SUCCESS || nConnect == SQL_SUCCESS_WITH_INFO ) 
			{
				nConnect = SQLAllocHandle( SQL_HANDLE_DBC, m_hEnv, &m_hDBC );
				if( nConnect == SQL_SUCCESS || nConnect == SQL_SUCCESS_WITH_INFO ) 
				{
					short shortResult = 0;

					shortResult = SQLSetConnectOption(m_hDBC, SQL_LOGIN_TIMEOUT, 5);
					// KSR_ADD
					// shortResult = SQLSetConnectOption(m_hDBC, SQL_ODBC_CURSORS, SQL_CUR_USE_DRIVER);

					shortResult = SQLSetConnectAttr(m_hDBC, SQL_ATTR_ODBC_CURSORS, (void*)SQL_CUR_USE_DRIVER,0);
					// shortResult = SQLSetConnectAttr(m_hDBC, SQL_ATTR_ASYNC_ENABLE, (void*)SQL_ASYNC_ENABLE_ON,0);

					// SQLTCHAR szOutConnectString[ 1024 ];
					SQLTCHAR *sqlSource = toSQLString(svSource);
					nConnect = SQLDriverConnect( m_hDBC,                // Connection Handle
						NULL,											// Window Handle
						sqlSource,	// svSource,						// InConnectionString
						SQL_NTS, 	//_tcslen(svSource),				// StringLength1
						NULL, 		//szOutConnectString,				// OutConnectionString
						0, 			//sizeof( szOutConnectString ),		// Buffer length
						NULL, 		//&shortResult,						// StringLength2Ptr
						SQL_DRIVER_NOPROMPT );							// no User prompt
				}
			}
		}
		if (!IS_SQL_OK(nConnect))
		{
			if( m_hDBC != NULL ) 
			{
				m_nReturn = SQLDisconnect( m_hDBC );
				m_nReturn = SQLFreeHandle( SQL_HANDLE_DBC,  m_hDBC );
			}
			if( m_hEnv!=NULL )
				m_nReturn = SQLFreeHandle( SQL_HANDLE_ENV, m_hEnv );

			m_hDBC              = NULL;
			m_hEnv              = NULL;
			m_nReturn           = SQL_ERROR;
			m_bReconnectFlag = true;
		}
		else
		{
			CreateNewHSTMT();
			CreateNewHSTMT();
			CreateNewHSTMT();
			CreateNewHSTMT();
			CreateNewHSTMT();
			m_bReconnectFlag = false;
		}
		return( IS_SQL_OK(nConnect) );
	}
	ODBCConnection()
	{
		m_hDBC              = NULL;
		m_hEnv              = NULL;
		m_nReturn           = SQL_ERROR;
		m_bReconnectFlag	= false;
	}

	virtual ~ODBCConnection()
	{
		Release();
	}

	void Release()
	{
//		if( m_hEnv!=NULL )
//			m_nReturn = SQLFreeHandle( SQL_HANDLE_ENV, m_hEnv );
//		m_hEnv = NULL;

		Disconnect();
	}

	void Disconnect()
	{
		if( m_hDBC != NULL )
		{
			std::list<SQLHSTMT>::iterator it;
			for (it = m_hStmtPool.begin(); it != m_hStmtPool.end(); it++)
			{
				SQLHSTMT	hb = (*it);
				m_nReturn = SQLFreeHandle(SQL_HANDLE_STMT, hb);
			}

			m_nReturn = SQLDisconnect( m_hDBC );
			m_nReturn = SQLFreeHandle( SQL_HANDLE_DBC,  m_hDBC );
		}
		m_hStmtPool.clear();
		m_hDBC = NULL;

		if( m_hEnv!=NULL )
			m_nReturn = SQLFreeHandle( SQL_HANDLE_ENV, m_hEnv );
		m_hEnv = NULL;
	}
	bool	IsConnected()
	{
		if (m_hDBC != NULL)
			return true;
		return false;
	}
	SQLHSTMT	GetRestHSTMT()
	{
		std::list<SQLHSTMT>::iterator it = m_hStmtPool.begin();
		if (it == m_hStmtPool.end())
			return INVALID_HANDLE_VALUE;

		SQLHSTMT	hb = (*it);
		m_hStmtPool.pop_front();
		CreateNewHSTMT();
		return hb;
	}
	operator HDBC()
	{
		return m_hDBC;
	}
	void	NotifyDBErrorOccur(SQLHSTMT hStmt, SQLINTEGER nErrorCode)
	{
		m_bReconnectFlag = true;
	}
	void	SetReconectFlag()
	{
		m_bReconnectFlag = true;
	}
	bool	GetReconnectFlag()
	{
		return m_bReconnectFlag;
	}

protected:
	void	CreateNewHSTMT()
	{
		SQLRETURN	nReturn;
		SQLHSTMT		hStmt;
		nReturn = SQLAllocHandle( SQL_HANDLE_STMT, m_hDBC, &hStmt );
		if(!IS_SQL_OK(nReturn))
		{
			sipwarning("SQLAllocHandle failed!!!!! DBCOnnection=0x%x", m_hDBC);
			hStmt=INVALID_HANDLE_VALUE;
		}
		else
		{
			SQLSetStmtAttr(hStmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_ROWVER, 0);
			SQLSetStmtAttr(hStmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_KEYSET_DRIVEN, 0);

			SQLSetStmtAttr( hStmt, SQL_CONCURRENCY, (void *)SQL_CONCUR_READ_ONLY, SQL_NTS);
			SQLSetStmtAttr( hStmt, SQL_CURSOR_TYPE, (void *)SQL_CURSOR_FORWARD_ONLY, SQL_NTS );
			SQLSetStmtOption( hStmt, SQL_ROWSET_SIZE, 1);
			SQLUINTEGER    fFuncs;
			SQLGetInfo(m_hDBC, SQL_MAX_ASYNC_CONCURRENT_STATEMENTS,(SQLPOINTER)&fFuncs,sizeof(fFuncs), NULL);
			SQLGetInfo(m_hDBC, SQL_ASYNC_MODE, (SQLPOINTER)&fFuncs,sizeof(fFuncs), NULL);
			if (fFuncs & SQL_AM_STATEMENT)
				SQLSetStmtAttr(hStmt, SQL_ATTR_ASYNC_ENABLE, SQLPOINTER(SQL_ASYNC_ENABLE_ON), 0);
			else
				sipwarning("DBINTERFACE: SQL_ATTR_ASYNC_ENABLE is not able");

			sipdebug("DBINTERFACE: SQLHSTMT created. Connection=0x%x, SQLHSTMT=0x%x", m_hDBC, hStmt);
			m_hStmtPool.push_back(hStmt);
		}
	}
private:
	SQLRETURN       m_nReturn;      // Internal SQL Error code
	HENV            m_hEnv;         // Handle to environment
	bool			m_bReconnectFlag;
public:
	HDBC				m_hDBC;         // Handle to database connection
	std::list<SQLHSTMT>	m_hStmtPool;
//	SQLHSTMT				m_hStmt;		// Handle to statement
};


class MSSQLConnection : public ODBCConnection
{
public:
	enum enumProtocols
	{
		protoNamedPipes,
		protoWinSock,
		protoIPX,
		protoBanyan,
		protoRPC
	};
public:
	MSSQLConnection (){};
	virtual ~MSSQLConnection (){};
	bool Connect(lpctstr User= _t("sa"), lpctstr Pass= _t(""), lpctstr Host= _t("localhost"), lpctstr Database= _t("pubs"), UINT Port = 0)
	{
		tchar str[512] = _t("");
#ifdef SIP_OS_WINDOWS
		SIPBASE::smprintf(str, sizeof(str), _t("DRIVER={SQL Server};SERVER=%s;UID=%s;PWD=%s;DATABASE=%s;"),
			Host,	User,	Pass,	Database);
#else // SIP_OS_WINDOWS
#ifdef SIP_USE_UNICODE
		SIPBASE::smprintf(str, sizeof(str), _t("Driver=FreeTDS;Server=%S;Port=%d;UID=%S;PWD=%S;Database=%S;"),
			Host,	Port ? Port : 1433, 	User,	Pass,	Database);
#else // SIP_USE_UNICODE
		SIPBASE::smprintf(str, sizeof(str), _t("Driver=FreeTDS;Server=%s;Port=%d;UID=%s;PWD=%s;Database=%s;"),
			Host,	Port ? Port : 1433, 	User,	Pass,	Database);
#endif
#endif

/*		_stprintf(str,_t("DRIVER={SQL Server};SERVER=%s;UID=%s;PWD=%s;Trusted_Connection=%s;Network="),
			SAFE_STR(Host),SAFE_STR(User),SAFE_STR(Pass),
			(Trusted ? _t("Yes") : _t("No");
		switch(Proto)
		{
		case protoNamedPipes:
			_tcscat(str,_t("dbnmpntw");
			break;
		case protoWinSock:
			_tcscat(str,_t("dbmssocn");
			break;
		case protoIPX:
			_tcscat(str,_t("dbmsspxn");
			break;
		case protoBanyan:
			_tcscat(str,_t("dbmsvinn");
			break;
		case protoRPC:
			_tcscat(str,_t("dbmsrpcn");
			break;
		default:
			_tcscat(str,_t("dbmssocn");
			break;
		}
		_tcscat(str,_t(";"));*/
		return ODBCConnection::Connect(str);
	};
};

/*
class mySQLConnection : public ODBCConnection
{
public:
	mySQLConnection(){};
	virtual ~mySQLConnection(){};
	int Connect(lpctstr User=_t(""),lpctstr Pass=_t(""),lpctstr Host=_t("localhost"),UINT Port=3306,UINT Option=0)
	{
		TCHAR str[512]=_t("");
		_stprintf(str,_t("Driver={MySQL ODBC 3.51 Driver};Uid=%s;Pwd=%s;Server=%s;Port=%d;"),
			SAFE_STR(User),SAFE_STR(Pass),
			SAFE_STR(Host),Port);
		return ODBCConnection::Connect(str);
	};
	int ConnectDB(lpctstr DB,lpctstr User=_t(""),
		lpctstr Pass=_t(""),lpctstr Host=_t("localhost"),
		UINT Port=3306,UINT Option=0)
	{
		TCHAR str[512]=_t("");
		_stprintf(str,_t("Driver={MySQL ODBC 3.51 Driver};Database=%s;Uid=%s;Pwd=%s;Server=%s;Port=%d;"),
			SAFE_STR(DB),SAFE_STR(User),SAFE_STR(Pass),
			SAFE_STR(Host),Port);
		return ODBCConnection::Connect(str);
	};
};
*/
namespace SIPBASE
{
	extern void NotifyDBErrorOccur(HDBC	hDBCLink, SQLHSTMT hStmt, SQLRETURN nRetError);
}
class ODBCStmt
{
	ODBCConnection*	m_DBCConnection;
	SQLHSTMT			m_hStmt;
public:
	operator SQLHSTMT()
	{
		return m_hStmt;
	}
	ODBCStmt()
	{
	}

	
//	ODBCStmt(HDBC hDBCLink)
	ODBCStmt(ODBCConnection &OdbcCon)
	{
		m_DBCConnection		= &OdbcCon;
		m_hStmt				= OdbcCon.GetRestHSTMT();
	}
	ODBCStmt(ODBCConnection *OdbcCon)
	{
		m_DBCConnection		= OdbcCon;
		m_hStmt				= OdbcCon->GetRestHSTMT();
/*
		SQLRETURN m_nReturn;
		m_nReturn = SQLAllocHandle( SQL_HANDLE_STMT, hDBCLink, &m_hStmt );
		if(!IS_SQL_OK(m_nReturn))
		{
			sipwarning("SQLAllocHandle failed!!!!! DBCOnnection=0x%x", hDBCLink);
			m_hStmt=INVALID_HANDLE_VALUE;
			return;
		}

		SQLSetStmtAttr(m_hStmt, SQL_ATTR_CONCURRENCY, (SQLPOINTER) SQL_CONCUR_ROWVER, 0);
		SQLSetStmtAttr(m_hStmt, SQL_ATTR_CURSOR_TYPE, (SQLPOINTER) SQL_CURSOR_KEYSET_DRIVEN, 0);

		SQLSetStmtAttr( m_hStmt, SQL_CONCURRENCY, (void *)SQL_CONCUR_READ_ONLY, SQL_NTS);
		SQLSetStmtAttr( m_hStmt, SQL_CURSOR_TYPE, (void *)SQL_CURSOR_FORWARD_ONLY, SQL_NTS );
		SQLSetStmtOption( m_hStmt, SQL_ROWSET_SIZE, 1);
		SQLUINTEGER    fFuncs;
		SQLGetInfo(hDBCLink, SQL_ASYNC_MODE, (SQLPOINTER)&fFuncs,sizeof(fFuncs), NULL);
		if (fFuncs & SQL_AM_STATEMENT)
			SQLSetStmtAttr(m_hStmt, SQL_ATTR_ASYNC_ENABLE, SQLPOINTER(SQL_ASYNC_ENABLE_ON), 0);
		else
			sipwarning(_t("DBINTERFACE: SQL_ATTR_ASYNC_ENABLE is not able");

		sipdebug("DBINTERFACE: SQLHSTMT created. Connection=0x%x, SQLHSTMT=0x%x", hDBCLink, m_hStmt);
		*/
	}
	virtual ~ODBCStmt()
	{
		FreeHandle();
	}
	BOOL FreeHandle()
	{
		BOOL bSuccess = true;
		if (m_hStmt != INVALID_HANDLE_VALUE)
		{
			SQLRETURN	nReturn = SQLFreeHandle(SQL_HANDLE_STMT, m_hStmt);
			if (nReturn == SQL_ERROR)
			{
				ProcError(_t("SQLFreeHandle"), nReturn);
				bSuccess = false;
			}
			else
				sipdebug("DBINTERFACE: SQLHSTMT destroyed. Connection=0x%x, SQLHSTMT=0x%x, Code=%d", (SQLHSTMT)(*m_DBCConnection), m_hStmt, nReturn);
		}
		m_hStmt = INVALID_HANDLE_VALUE; // NULL;	// Modified by RSC 2014/04/18
		return bSuccess;
	}
	BOOL IsValid()
	{
		return m_hStmt != INVALID_HANDLE_VALUE;
	}
	DWORD GetChangedRowCount(void)
	{
		long nRows=0;
		int	nRet = SQLRowCount(m_hStmt,&nRows);
		if(!IS_SQL_OK(nRet))
			return 0;
		return nRows;
	}
	BOOL Query( lpctstr strSQL)
	{
		SQLTCHAR *sqlString = toSQLString(strSQL);

		SIPBASE::TTime	tmStart = SIPBASE::CTime::getLocalTime();
		SQLRETURN nRet = SQLExecDirect( m_hStmt, sqlString, SQL_NTS );
		while( nRet == SQL_STILL_EXECUTING)
		{
			SIPBASE::TTime	tmCur = SIPBASE::CTime::getLocalTime();
			if (tmCur - tmStart > 60000)
			{
				sipwarning(_t("DBINTERFACE: Execute DB timeout : (%s)"), strSQL);
				break;
			}
			SIPBASE::sipSleep(1);
			nRet = SQLExecDirect( m_hStmt, sqlString, SQL_NTS );
		}
		if ( IS_SQL_EXCUTE_OK( nRet ) )
			return true;

		ProcError(strSQL, nRet);
		return false;
	}
	BOOL Prepare(lpctstr strSQL)
	{
		SQLTCHAR *sqlString = toSQLString(strSQL);

		SQLRETURN nRet=SQLPrepare( m_hStmt, sqlString, SQL_NTS );
		return IS_SQL_OK( nRet );
	}

	void ProcError(lpctstr strFunc, SQLRETURN nRet)
	{
		// print error
		SQLTCHAR	SQLState[0x100], MessageText[0x200];
		SQLINTEGER	NativeErrorPtr;
		SQLSMALLINT	TextLengthPtr = 0;
		SQLGetDiagRec(
			SQL_HANDLE_STMT,
			m_hStmt,
			1,
			SQLState,
			&NativeErrorPtr,
			MessageText,
			0x100,
			&TextLengthPtr);
		ucchar ucStrState[0x100], ucMessageText[0x100];
		for (int i = 0; i < 0x100; i++)
			ucStrState[i] = SQLState[i];
		
		for (int i = 0; i < 0x200; i++)
			ucMessageText[i] = MessageText[i];			
		
#ifdef SIP_USE_UNICODE
		sipwarning("DBINTERFACE: %S failed. DBConnection=0x%x, SQLHSTMT=0x%x, ReturnCode=0x%x, ErrorCode=0x%x, SQLState=%S, MessageText=%S", strFunc, (SQLHSTMT)(*m_DBCConnection), m_hStmt, nRet, NativeErrorPtr, ucStrState, ucMessageText);
#else
		sipwarning("DBINTERFACE: %s failed. DBConnection=0x%x, SQLHSTMT=0x%x, ReturnCode=0x%x, ErrorCode=0x%x, SQLState=%s, MessageText=%s", strFunc, (SQLHSTMT)(*m_DBCConnection), m_hStmt, nRet, NativeErrorPtr, ucStrState, ucMessageText);
#endif
		TextLengthPtr = 0;

		// Notify
		m_DBCConnection->NotifyDBErrorOccur(m_hStmt, NativeErrorPtr);
	}

	BOOL BindParam(int No, int IOType, int CType, int SQLType, SQLULEN ColumnSize, int DecimalDigits, SQLPOINTER ptrParam, SQLLEN BufferLength , SQLLEN *StrLen_or_IndPtr)
	{
		SQLRETURN nRet=SQLBindParameter( m_hStmt, (SQLSMALLINT)No, (SQLSMALLINT)IOType, (SQLSMALLINT)CType, (SQLSMALLINT)SQLType,
			ColumnSize, (SQLSMALLINT)DecimalDigits, (SQLPOINTER)ptrParam, BufferLength , StrLen_or_IndPtr );
        if ( IS_SQL_OK( nRet ) )
			return true;

		ProcError(_t("BindParam"), nRet);
		return false;
	}
	BOOL ParamData(void **prgbValue)
	{
		SQLRETURN nRet=SQLParamData( m_hStmt, (SQLPOINTER *)prgbValue );
        return IS_SQL_OK( nRet );
	}
	BOOL BindColumn(uint16 Column, void *pBuffer,
		int BufferSize,int * pReturnedBufferSize=NULL,
		sint16 nType=SQL_C_TCHAR)
	{
		int		pReturnedSize=0;
		if ( nType == SQL_VARCHAR )
			nType = SQL_C_CHAR;
		else if ( nType == SQL_WVARCHAR || nType == SQL_WCHAR )
			nType = SQL_C_WCHAR;
		else if ( (nType == SQL_DATETIME) || (nType == SQL_TYPE_DATE) || (nType == SQL_TYPE_TIME) || (nType == SQL_TYPE_TIMESTAMP) )
			nType = SQL_C_CHAR;
		else
			BufferSize = 0;

		SQLRETURN nRet=SQLBindCol(m_hStmt, Column, nType, pBuffer, BufferSize, (SQLLEN *)&pReturnedSize);
		if(pReturnedBufferSize)
			*pReturnedBufferSize=pReturnedSize;

		if ( IS_SQL_OK( nRet ) )
			return true;

		ProcError(_t("BindColumn"), nRet);
		return false;
	}
	BOOL Query()
	{
		SIPBASE::TTime	tmStart = SIPBASE::CTime::getLocalTime();
		SQLRETURN nRet=SQLExecute( m_hStmt );
		while( nRet == SQL_STILL_EXECUTING)
		{
			SIPBASE::TTime	tmCur = SIPBASE::CTime::getLocalTime();
			if (tmCur - tmStart > 60000)
			{
				sipwarning("DBINTERFACE: Execute DB timeout : ");
				break;
			}
			SIPBASE::sipSleep(1);
			nRet = SQLExecute( m_hStmt );
		}

		if(IS_SQL_EXCUTE_OK( nRet ))
			return true;

		ProcError(_t("Execute Query"), nRet);
		return false;
	}
	BOOL MoreResults()
	{
		SQLUINTEGER    syncmode;
		SQLGetStmtAttr(m_hStmt, SQL_ATTR_ASYNC_ENABLE, (SQLPOINTER)&syncmode,sizeof(syncmode), NULL);
		if (syncmode == SQL_ASYNC_ENABLE_ON)
			SQLSetStmtAttr(m_hStmt, SQL_ATTR_ASYNC_ENABLE, SQLPOINTER(SQL_ASYNC_ENABLE_OFF), 0);
		SIPBASE::TTime	tmStart = SIPBASE::CTime::getLocalTime();
		SQLRETURN nRet = SQLMoreResults(m_hStmt);
		while (nRet == SQL_STILL_EXECUTING)
		{
			SIPBASE::TTime	tmCur = SIPBASE::CTime::getLocalTime();
			if (tmCur - tmStart > 60000)
			{
				sipwarning("DBINTERFACE: more result timeout");
				if (syncmode == SQL_ASYNC_ENABLE_ON)
					SQLSetStmtAttr(m_hStmt, SQL_ATTR_ASYNC_ENABLE, SQLPOINTER(SQL_ASYNC_ENABLE_ON), 0);
				return false;
			}
			SIPBASE::sipSleep(1);
			nRet = SQLMoreResults(m_hStmt);
		}
		if (syncmode == SQL_ASYNC_ENABLE_ON)
			SQLSetStmtAttr(m_hStmt, SQL_ATTR_ASYNC_ENABLE, SQLPOINTER(SQL_ASYNC_ENABLE_ON), 0);
		if ( IS_SQL_OK( nRet ) )
		{
			return true;
		}
		return false;
	}
	BOOL Fetch()
	{
		SIPBASE::TTime	tmStart = SIPBASE::CTime::getLocalTime();
		SQLRETURN nRet = SQLFetch(m_hStmt);
		while (nRet == SQL_STILL_EXECUTING)
		{
			SIPBASE::TTime	tmCur = SIPBASE::CTime::getLocalTime();
			if (tmCur - tmStart > 60000)
			{
				sipwarning(_t("DBINTERFACE: fetch result timeout"));
				return false;
			}
			SIPBASE::sipSleep(1);
			nRet = SQLFetch(m_hStmt);
		}
//sipdebug("DBINTERFACE: Fetch = %d", nRet);
		if ( IS_SQL_OK( nRet ) )
		{
			return true;
		}
		return false;
	}
	BOOL FecthRow(UINT nRow)
	{
		int	nRet = SQLSetPos(m_hStmt, nRow, SQL_POSITION, SQL_LOCK_NO_CHANGE);
		return IS_SQL_OK(nRet);
	}
	BOOL FetchPrevious()
	{
		SQLRETURN nRet=SQLFetchScroll(m_hStmt,SQL_FETCH_PRIOR,0);
		return IS_SQL_OK(nRet);
	}
	BOOL FecthNext()
	{
		SQLRETURN nRet=SQLFetchScroll(m_hStmt,SQL_FETCH_NEXT,0);
		return IS_SQL_OK(nRet);
	}
	BOOL FetchRow(ULONG nRow,BOOL Absolute=1)
	{
		SQLRETURN nRet=SQLFetchScroll(m_hStmt,
			(Absolute ? SQL_FETCH_ABSOLUTE : SQL_FETCH_RELATIVE),nRow);
		return IS_SQL_OK(nRet);
	}
	BOOL FetchFirst()
	{
		SQLRETURN nRet=SQLFetchScroll(m_hStmt,SQL_FETCH_FIRST,0);
		return IS_SQL_OK(nRet);
	}
	BOOL FetchLast()
	{
		SQLRETURN nRet=SQLFetchScroll(m_hStmt,SQL_FETCH_LAST,0);
		return IS_SQL_OK(nRet);
	}
	BOOL Cancel()
	{
		SQLRETURN nRet=SQLCancel(m_hStmt);
		return IS_SQL_OK(nRet);
	}
	USHORT GetRowCount()
	{
		int	nRows = 0;
		int	nRet = SQLRowCount(m_hStmt, (SQLLEN *) &nRows);
		if(!IS_SQL_OK(nRet))
			return 0;
		return nRows;
	}
	USHORT GetColumnCount()
	{
		short nCols=0;
		int	nRet = SQLNumResultCols(m_hStmt,&nCols);
		if(!IS_SQL_OK(nRet))
			return 0;
		return nCols;
	}
	USHORT GetColumnByName(lpctstr Column)
	{
		short nCols=GetColumnCount();
		for(USHORT i=1;i<(nCols+1);i++)
		{
			tchar Name[256]=_t("");
			GetColumnName(i,Name,(short)sizeof(Name));
			if(!_tcsicmp(Name,Column))
				return i;
		}
		return 0;
	}
	BOOL GetData(USHORT Column, lpvoid pBuffer, 
		ULONG pBufLen, SQLLEN * dataLen=NULL, int Type=SQL_C_DEFAULT)
	{
		SQLLEN od=0;
		SIPBASE::TTime	tmStart = SIPBASE::CTime::getLocalTime();
		int Err=SQLGetData(m_hStmt,Column,Type,pBuffer,pBufLen,&od);
		while( Err == SQL_STILL_EXECUTING)
		{
			SIPBASE::TTime	tmCur = SIPBASE::CTime::getLocalTime();
			if (tmCur - tmStart > 60000)
			{
				sipwarning("DBINTERFACE: GetData timeout");
				break;
			}
			SIPBASE::sipSleep(1);
			Err=SQLGetData(m_hStmt,Column,Type,pBuffer,pBufLen,&od);
		}
		if(IS_SQL_ERR(Err))
		{ 
			return 0;
		} 
		if(dataLen)
			*dataLen=od;
		return 1;
	}
	int GetColumnType( USHORT Column )
	{
		int nType=SQL_C_DEFAULT;
		SQLTCHAR svColName[ 256 ];
		memset(svColName, 0, sizeof(svColName));
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef;
		SQLDescribeCol( m_hStmt,            // Statement handle
			Column,             // ColumnNumber
			svColName,          // ColumnName
			sizeof( svColName), // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		nType=(int)swType;
		return( nType );
	}
	DWORD GetColumnSize( USHORT Column )
	{
		int nType=SQL_C_DEFAULT;
		SQLTCHAR svColName[ 256 ];
		memset(svColName, 0, sizeof(svColName));
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef=0;
		SQLDescribeCol( m_hStmt,            // Statement handle
			Column,             // ColumnNumber
			svColName,          // ColumnName
			sizeof( svColName), // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		return pcbColDef;
	}
	DWORD GetColumnScale( USHORT Column )
	{
		int nType=SQL_C_DEFAULT;
		SQLTCHAR svColName[ 256 ];
		memset(svColName, 0, sizeof(svColName));
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef=0;
		SQLDescribeCol( m_hStmt,            // Statement handle
			Column,             // ColumnNumber
			svColName,          // ColumnName
			sizeof( svColName), // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		return swScale;
	}
	BOOL GetColumnName( USHORT Column, lptstr Name, short NameLen )
	{
		int nType=SQL_C_DEFAULT;
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef=0;
		SQLRETURN Ret=
			SQLDescribeCol( m_hStmt,            // Statement handle
			Column,               // ColumnNumber
			(SQLTCHAR*)Name,     // ColumnName
			NameLen,    // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		if(IS_SQL_ERR(Ret))
			return 0;
		return 1;
	}
	BOOL IsColumnNullable( USHORT Column )
	{
		int nType=SQL_C_DEFAULT;
		SQLTCHAR svColName[ 256 ];
		memset(svColName, 0, sizeof(svColName));
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef;
		SQLDescribeCol( m_hStmt,            // Statement handle
			Column,             // ColumnNumber
			svColName,          // ColumnName
			sizeof( svColName), // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		return (swNull==SQL_NULLABLE);
	}
};
//--
class ODBCRecord
{
	SQLHSTMT m_hStmt;
public:
	ODBCRecord(){};
	ODBCRecord(SQLHSTMT hStmt){m_hStmt=hStmt;};
	~ODBCRecord(){};
	BOOL IsValid()
	{
		return m_hStmt!=INVALID_HANDLE_VALUE;
	}

	void SetStmt(SQLHSTMT hStmt)
	{
		m_hStmt = hStmt;
	}

	USHORT GetColumnCount()
	{
		short nCols=0;
		int	nRet = SQLNumResultCols(m_hStmt,&nCols);
		if(!IS_SQL_OK(nRet))
			return 0;
		return nCols;
	}
	BOOL BindColumn(uint16 Column, void *pBuffer,
		int pBufferSize,int * pReturnedBufferSize=NULL,
		sint16 nType=SQL_C_TCHAR)
	{
		int		pReturnedSize=0;
		if ( nType == SQL_WCHAR )
			nType = SQL_CHAR;
		if ( nType == SQL_WVARCHAR )
			nType = SQL_VARCHAR;

		SQLRETURN nRet=SQLBindCol(m_hStmt, Column, nType, pBuffer, pBufferSize, (SQLLEN *)&pReturnedSize);
		if(pReturnedBufferSize)
			*pReturnedBufferSize=pReturnedSize;

		if ( IS_SQL_OK( nRet ) )
			return true;

		return false;
	}
	USHORT GetColumnByName(lpctstr Column)
	{
		short nCols=GetColumnCount();
		for(USHORT i=1;i<(nCols+1);i++)
		{
			tchar Name[256]=_t("");
			GetColumnName(i,Name,sizeof(Name));
			if(!_tcsicmp(Name,Column))
				return i;
		}
		return 0;
	}
	BOOL GetData(USHORT Column, lpvoid pBuffer, 
		ULONG pBufLen, SQLLEN * dataLen=NULL, int Type=SQL_C_DEFAULT)
	{
		SQLLEN od=0;
		SIPBASE::TTime	tmStart = SIPBASE::CTime::getLocalTime();
		int Err=SQLGetData(m_hStmt,Column,Type,pBuffer,pBufLen,&od);
		while( Err == SQL_STILL_EXECUTING)
		{
			SIPBASE::TTime	tmCur = SIPBASE::CTime::getLocalTime();
			if (tmCur - tmStart > 60000)
			{
				sipwarning("DBINTERFACE: GetData timeout");
				break;
			}
			SIPBASE::sipSleep(1);
			Err=SQLGetData(m_hStmt,Column,Type,pBuffer,pBufLen,&od);
		}

		if(IS_SQL_ERR(Err))
		{ 
			return 0;
		} 
		if(dataLen)
			*dataLen=od;
		return 1;
	}
	int GetColumnType( USHORT Column )
	{
		int nType=SQL_C_DEFAULT;
		SQLTCHAR svColName[ 256 ];
		memset(svColName, 0, sizeof(svColName));
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef;
		SQLDescribeCol( m_hStmt,            // Statement handle
			Column,             // ColumnNumber
			svColName,          // ColumnName
			sizeof( svColName), // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		nType=(int)swType;
		return( nType );
	}
	DWORD GetColumnSize( USHORT Column )
	{
		int nType=SQL_C_DEFAULT;
		SQLTCHAR svColName[ 256 ];
		memset(svColName, 0, sizeof(svColName));
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef=0;
		SQLDescribeCol( m_hStmt,            // Statement handle
			Column,             // ColumnNumber
			svColName,          // ColumnName
			sizeof( svColName), // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		return pcbColDef;
	}
	DWORD GetColumnScale( USHORT Column )
	{
		int nType=SQL_C_DEFAULT;
		SQLTCHAR svColName[ 256 ];
		memset(svColName, 0, sizeof(svColName));
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef=0;
		SQLDescribeCol( m_hStmt,            // Statement handle
			Column,             // ColumnNumber
			svColName,          // ColumnName
			sizeof( svColName), // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		return swScale;
	}
	BOOL GetColumnName( USHORT Column, lptstr Name, short NameLen )
	{
		int nType=SQL_C_DEFAULT;
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef=0;
		SQLRETURN Ret=
			SQLDescribeCol( m_hStmt,            // Statement handle
			Column,               // ColumnNumber
			(SQLTCHAR*)Name,     // ColumnName
			NameLen,    // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		if(IS_SQL_ERR(Ret))
			return 0;
		return 1;
	}
	BOOL IsColumnNullable( USHORT Column )
	{
		int nType=SQL_C_DEFAULT;
		SQLTCHAR svColName[ 256 ];
		memset(svColName, 0, sizeof(svColName));
		SWORD swCol=0,swType=0,swScale=0,swNull=0;
		SQLULEN pcbColDef;
		SQLDescribeCol( m_hStmt,            // Statement handle
			Column,             // ColumnNumber
			svColName,          // ColumnName
			sizeof( svColName), // BufferLength
			&swCol,             // NameLengthPtr
			&swType,            // DataTypePtr
			&pcbColDef,         // ColumnSizePtr
			&swScale,           // DecimalDigitsPtr
			&swNull );          // NullablePtr
		return (swNull==SQL_NULLABLE);
	}
};

#endif // end of guard __ODBC_INTERFACE_H__
