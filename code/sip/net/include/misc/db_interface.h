/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __DB_INTERFACE_H__
#define __DB_INTERFACE_H__

// #define MYSQL_USE

#ifdef	MYSQL_USE

#ifdef SIP_OS_WINDOWS
#include "winsock.h"
#endif //SIP_OS_WINDOWS

#include "mysql_interface.h"
#else //MSSQL_USE
#include "odbc_interface.h"
#endif //MYSQL_USE

// DB접속담당변수형 마크로
#ifdef	MYSQL_USE
	#define DBCONNECTION			MYSQLConnection
	#define DBSTMT					CMysqlStmt
	typedef	MYSQL_ROW				TDBResult;
#else
	#define DBCONNECTION			MSSQLConnection
	#define DBSTMT					ODBCStmt
	typedef	ODBCRecord				TDBResult;
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*	
	DB초기화및 접속함수
	파라메터:
	DBCon		DB접속담당변수
	Server		DB가 설치된 기대명 혹은 주소
	Uid			DB사용자ID
	Pwd			DB사용자암호
	DB			Default자료기지
*/ 
//BOOL InitDatabase(DBCONNECTION *DBCon, LPCSTR Server, LPCSTR Uid, const LPCSTR Pwd, LPCSTR DB, UINT Port=0)
//{
//	return DBCon->Connect(Uid, Pwd, Server, DB, Port);
//}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
/*	
	DB접속해제함수
	파라메터:
	DBCon		DB접속담당변수
*/
//void CloseDatabase(DBCONNECTION *DBCon)
//{
//	DBCon->Disconnect();
//}
//////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*	
	Query실행마크로
	여러 명령문의 복합으로 이루어진다.
	이마크로를 사용하는 경우에는 Query의 기능을 다 리용할때까지 하나의 Scope안에 넣어 사용하는것을 권고한다.
	빈Scope도 가능하다.
	실례:
	{
		TCHAR strQuery[256]=_T("SELECT * FROM [publishers]");
		DB_EXE_QUERY(DBCon, Stmt, strQuery);
		...
		...
	}
	파라메터:
	DBCon		DB접속담당변수
	Stmt		Query결과를 조종하게 될 변수
	StrQuery	Query문자렬
*/
// DB련결이 유효한가?
#define ISCONNECTEDDB(DBCon)	DBCon.IsConnected()

#ifdef	MYSQL_USE

	#define DB_EXE_QUERY_RELEASE(Stmt)          Stmt.Release(); 

#else
	#define DB_RECONNECT(DBCon, __DBName) 
	#define DB_EXE_QUERY_RELEASE(Stmt)
#endif

#ifdef MYSQL_USE
	#define DB_EXE_QUERY(DBCon, Stmt, StrQuery)			\
		int nQueryResult = true;                        \
	    ucstring query = StrQuery;                      \
		std::string str = query.toUtf8();               \
		char StrQuery_Multi[500];                       \
		sprintf(StrQuery_Multi, "%s", str.c_str());             \
		if(DBCon.Query((const char*)StrQuery_Multi) != 0)  \
			nQueryResult = false;                       \
		CMySQLResult Stmt(DBCon);
#else
	#define DB_EXE_QUERY(DBCon, Stmt, StrQuery)			\
		ODBCStmt Stmt(DBCon);							\
		sipdebug(L"DBINTERFACE: Execute DB query : (%s), DBConnection=0x%x, HSTMT=0x%x", StrQuery, (HSTMT)(DBCon), (HSTMT)(Stmt));	\
		int nQueryResult = Stmt.Query(StrQuery);		\
		if (nQueryResult == false)						\
			sipwarning(L"DBINTERFACE: FAILED DB query : (%s)", StrQuery);
#endif

//////////////////////////////////////////////////////////////////////////
/*
	Query결과Fetch마크로
	이 마크로는 Query실행마크로와 같은 계렬의 Scope에서 사용하여야 한다.
	파라메터:
	Stmt		Query결과를 조종하는 변수(이미 Query실행마크로에 의하여 할당된 변수를 사용하여야 한다)
	Row			결과행을 조종하게 될 변수
*/
#ifdef	MYSQL_USE
	#define DB_QUERY_RESULT_FETCH(Stmt, row)	\
		MYSQL_ROW row = Stmt.FetchRow();	
#else
	#define DB_QUERY_RESULT_FETCH(Stmt, row)	\
		HSTMT hStmt = INVALID_HANDLE_VALUE;		\
		if (Stmt.Fetch())						\
		{										\
			hStmt = (HSTMT)Stmt;				\
		}										\
		ODBCRecord row(hStmt);
#endif	

/*	
	결과행을 조종하는 변수의 유효성을 검증하는 마크로
	이 마크로는 함수처럼 사용할수 있다.
	파라메터:
	Row			결과행을 조종하는 변수
*/
#ifdef MYSQL_USE
	#define IS_DB_ROW_VALID(Row)	(Row != NULL)
#else
	#define IS_DB_ROW_VALID(Row)	(Row.IsValid())
#endif

// Case query return one row, mostly case return  special check value
#ifdef	MYSQL_USE
	#define DB_EXE_QUERY_WITHONEROW(DBCon, Stmt, StrQuery)	\
		DB_EXE_QUERY(DBCon, Stmt, StrQuery)		\
		uint32 uRowNum = 0;			\
		if (nQueryResult)				\
		{							\
			if (Stmt.IsFailed())	\
				nQueryResult = 0;	\
			else	\
				uRowNum = Stmt.NumberRows();	\
		}	\
		MYSQL_ROW row;		\
		if (nQueryResult && uRowNum >= 1)	\
		{	\
			row = Stmt.FetchRow();	\
			if (row == NULL)	\
				nQueryResult = 0;	\
		}	\
		else	\
		{	\
			nQueryResult = 0;	\
		}
#else
	#define DB_EXE_QUERY_WITHONEROW(DBCon, Stmt, StrQuery)	\
		DB_EXE_QUERY(DBCon, Stmt, StrQuery)		\
		\
		HSTMT hStmt = INVALID_HANDLE_VALUE;		\
		ODBCRecord row;	\
		if (nQueryResult == 1)	\
		{	\
			if (Stmt.Fetch())	\
			{	\
				hStmt = (HSTMT)Stmt;	\
			}	\
		}	\
		\
		row.SetStmt(hStmt);	\
		if (!IS_DB_ROW_VALID(row))	\
			nQueryResult = 0;
#endif


// Case want know the number of return rows, if query return few row
#ifdef	MYSQL_USE
	#define DB_EXE_QUERY_WITHRESULT(DBCon, Stmt, StrQuery)	\
		DB_EXE_QUERY(DBCon, Stmt, StrQuery)	\
		uint32 uRowNum = 0;			\
		if (nQueryResult)			\
		{							\
			if (Stmt.IsFailed())	\
			{	\
				WARNING_TO_LOG("mysql_store_result () failed from query '%s': %s", __strQuery.c_str (),  mysql_error(DBCon));	\
				nQueryResult = 0;	\
			}	\
			else	\
				uRowNum = Stmt.NumberRows();	\
		}
#else
	#define	DB_EXE_QUERY_WITHRESULT(DBCon, Stmt, StrQuery)	\
		DB_EXE_QUERY(DBCon, Stmt, StrQuery)	\
		uint32 uRowNum = 0;			\
		if (nQueryResult)			\
		{							\
			if (!Stmt.IsValid())	\
				nQueryResult = 0;	\
			else	\
				uRowNum = Stmt.GetChangedRowCount();	\
		}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OutPurParameter
#ifdef	MYSQL_USE
	//#error (Sorry, Not Prepared function of getting outparam yet (DEV))
	#define DB_EXE_PREPARESTMT(DBCon, Stmt, StrQuery)   \
		CMysqlStmt  Stmt(DBCon);                        \
		ucstring query = StrQuery;			\
		std::string strUtf8 =query.toUtf8();       \
		bool nPrepareRet = Stmt.Prepare(strUtf8.c_str());      \
		if( nPrepareRet == false )                      \
			sipwarning( "DBINTERFACE: FAILED Prepare query : (%s)", strUtf8.c_str());  \
		int nBindResult = true;    \
		int nRet = true;
#else
	#define	DB_EXE_PREPARESTMT(DBCon, Stmt, StrQuery)	\
		ODBCStmt Stmt(DBCon);							\
		int nPrepareRet = Stmt.Prepare(StrQuery);		\
		if ( nPrepareRet == false )						\
			sipwarning(L"DBINTERFACE: FAILED Prepare query : (%s)", StrQuery);	\
		int nBindResult = true;	\
		int nRet = true;
	#endif

#ifdef MYSQL_USE
	#define DB_EXE_BINDPARAM_IN(Stmt, No, SQLType, ColumnSize, ptrParam, StrLen_or_IndPtr)	\
		bind[No-1].buffer_type = SQLType;\
		bind[No-1].buffer = (char *)ptrParam;\
		bind[No-1].buffer_length =(unsigned)(*StrLen_or_IndPtr);\
		/*bind.is_null = 0;*/\
		bind[No-1].length =  ColumnSize;
#else
	#define DB_EXE_BINDPARAM_IN(Stmt, No, CType, SQLType, ColumnSize, DecimalDigits, ptrParam, BufferLength , StrLen_or_IndPtr)	\
		nBindResult = Stmt.BindParam(No, SQL_PARAM_INPUT, CType, SQLType, ColumnSize, DecimalDigits, ptrParam, BufferLength , StrLen_or_IndPtr);	\
		if ( nBindResult == false )				\
		{	\
			sipwarning(L"DBINTERFACE: FAILED Bind Parameter IN, No : (%u)", No);	\
			if ( nRet )	\
				nRet = false;	\
		}
#endif

#ifdef MYSQL_USE
	#define DB_EXE_BINDPARAM_OUT(Stmt, No, SQLType, ColumnSize, ptrParam, Buffer_length)	\
		bindout[No-1].buffer_type = SQLType;\
		bindout[No-1].buffer = (char *)ptrParam;\
		bindout[No-1].buffer_length =Buffer_length;\
		/*bind.is_null = 0;*/\
		bindout[No-1].length = ColumnSize;
#else
	#define DB_EXE_BINDPARAM_OUT(Stmt, No, CType, SQLType, ColumnSize, DecimalDigits, ptrParam, BufferLength , StrLen_or_IndPtr)	\
		nBindResult = Stmt.BindParam(No, SQL_PARAM_OUTPUT, CType, SQLType, ColumnSize, DecimalDigits, ptrParam, BufferLength , StrLen_or_IndPtr);	\
		if ( nBindResult == false )				\
		{	\
			sipwarning(L"DBINTERFACE: FAILED Bind Parameter OUT, No : (%u)", No);	\
			if ( nRet )	\
				nRet = false;	\
		}
#endif

#ifdef MYSQL_USE
	#define DB_EXE_PARAMQUERY(Stmt, StrQuery)		\
		int nQueryResult = Stmt.Query();			\
		if (nQueryResult == false)					\
			  sipwarning(L"DBINTERFACE: FAILED DB query param : (%s)", StrQuery);
#else
	#define DB_EXE_PARAMQUERY(Stmt, StrQuery)		\
		sipdebug(L"DBINTERFACE: Execute DB query param : (%s)", StrQuery);	\
		int nQueryResult = Stmt.Query();			\
		if (nQueryResult == false)					\
			sipwarning(L"DBINTERFACE: FAILED DB query param : (%s)", StrQuery);
#endif

#ifdef MYSQL_USE
	/* --comment by LCI 09-04-17
	#define DB_EXE_BINDPARAM_IO(Stmt, No, CType, SQLType, ColumnSize, DecimalDigits, ptrParam, BufferLength , StrLen_or_IndPtr)	\
		nBindResult = Stmt.BindParam(No, SQL_PARAM_INPUT_OUTPUT, CType, SQLType, ColumnSize, DecimalDigits, ptrParam, BufferLength , StrLen_or_IndPtr);	\
		if ( nBindResult == false )				\
		{	\
			sipwarning(L"DBINTERFACE: FAILED Bind Parameter, No : (%u)", No);	\
			if ( nRet )	\
				nRet = false;	\
		}
	*/
#else
	#define DB_EXE_BINDPARAM_IO(Stmt, No, CType, SQLType, ColumnSize, DecimalDigits, ptrParam, BufferLength , StrLen_or_IndPtr)	\
		nBindResult = Stmt.BindParam(No, SQL_PARAM_INPUT_OUTPUT, CType, SQLType, ColumnSize, DecimalDigits, ptrParam, BufferLength , StrLen_or_IndPtr);	\
		if ( nBindResult == false )				\
		{	\
			sipwarning(L"DBINTERFACE: FAILED Bind Parameter IO, No : (%u)", No);	\
			if ( nRet )	\
				nRet = false;	\
		}
#endif

#ifdef MYSQL_USE
	/* --comment by LCI 09-04-17
	#define DB_EXE_PARAMQUERY_WITHONEROW(Stmt, StrQuery)	\
		DB_EXE_PARAMQUERY(Stmt, StrQuery)		\
		\
		HSTMT hStmt = INVALID_HANDLE_VALUE;		\
		ODBCRecord row;	\
		if (nQueryResult == 1)	\
		{	\
			if (Stmt.Fetch())	\
			{	\
				hStmt = (HSTMT)Stmt;	\
			}	\
		}	\
		\
		row.SetStmt(hStmt);	\
		if (!IS_DB_ROW_VALID(row))	\
			nQueryResult = 0; */
#else
	#define DB_EXE_PARAMQUERY_WITHONEROW(Stmt, StrQuery)	\
		DB_EXE_PARAMQUERY(Stmt, StrQuery)		\
		\
		HSTMT hStmt = INVALID_HANDLE_VALUE;		\
		ODBCRecord row;	\
		if (nQueryResult == 1)	\
		{	\
			if (Stmt.Fetch())	\
			{	\
				hStmt = (HSTMT)Stmt;	\
			}	\
		}	\
		\
		row.SetStmt(hStmt);	\
		if (!IS_DB_ROW_VALID(row))	\
			nQueryResult = 0;
#endif

#ifdef MYSQL_USE
	/* --comment by LCI 09-04-17
	#define	DB_EXE_PARAMQUERY_WITHRESULT(Stmt, StrQuery)	\
		DB_EXE_PARAMQUERY(Stmt, StrQuery)	\
		uint32 uRowNum = 0;			\
		if (nQueryResult)			\
		{							\
			if (!Stmt.IsValid())	\
				nQueryResult = 0;	\
			else	\
				uRowNum = Stmt.GetChangedRowCount();	\
		}
	*/
#else
	#define	DB_EXE_PARAMQUERY_WITHRESULT(Stmt, StrQuery)	\
		DB_EXE_PARAMQUERY(Stmt, StrQuery)	\
		uint32 uRowNum = 0;			\
		if (nQueryResult)			\
		{							\
			if (!Stmt.IsValid())	\
				nQueryResult = 0;	\
			else	\
				uRowNum = Stmt.GetChangedRowCount();	\
		}
#endif

//////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef	MYSQL_USE	/************************************************************************/
	//	old code
	//	#define	COLUMN_DIGIT(row, nCol, type, val)
	//	type val = 0;
	//	val = (type)atoi(row[nCol]);
	
	//modified by LCI --091218
	//On Linux, atoi occur error for unsigned integer value.
	#define	COLUMN_DIGIT(row, nCol, type, val)				\
		type val = (type) 0xFFFFFFFF;					\
		if (val < 0){							\
			val = (type) strtol(row[nCol], NULL, 10);				\
		} else	{							\
			val = (type) strtoul(row[nCol], NULL, 10);			\
		} 
#else
	#define	COLUMN_DIGIT(row, nCol, type, val)				\
		type val = 0;										\
		row.GetData(nCol+1, (&val), sizeof(val), NULL);
#endif

#ifdef MYSQL_USE
	#define	COLUMN_STR(row, nCol, val, nLen)				\
		char temp##val[nLen+1] = "";						\
		strcpy(temp##val, row[nCol]);						\
		string	val(temp##val);
#else
	#define		COLUMN_STR(row, nCol, val, nLen)			\
		char temp##val[nLen+1] = "";								\
		row.GetData(nCol+1, temp##val, sizeof(char)*(nLen+1), NULL, SQL_C_CHAR);	\
		string	val(temp##val);
#endif

#ifdef MYSQL_USE
	/*comment by cch 20100130
	#define	COLUMN_WSTR(row, nCol, val, nLen)				\
		wchar_t temp##val[nLen+1] = L"";					\
		MultiByteToWideChar(CP_UTF8, 0, row[nCol], (int)strlen(row[nCol]), temp##val, nLen+1);	\
		ucstring	val(temp##val);
	*/
	#define	COLUMN_WSTR(row, nCol, val, nLen)				\
		ucstring val;							\
		if(row[nCol] == NULL )						\
			val = L" ";						\
		else								\
			val = ucstring::makeFromUtf8(row[nCol]); //by lci 09-06-09
#else
	#define		COLUMN_WSTR(row, nCol, val, nLen)						\
		SQLWCHAR temp##val[nLen+1];			\
		memset(temp##val, 0, sizeof(SQLWCHAR) * (nLen + 1));			\
		row.GetData(nCol+1, temp##val, sizeof(SQLWCHAR) * (nLen + 1), NULL, SQL_C_WCHAR);	\
											\
		wchar_t str##val[nLen + 1] = L"";	\
		for (int i = 0; i < nLen + 1; i++)	\
		{									\
			str##val[i] = temp##val[i];		\
		}									\
		ucstring val(str##val);
		/*
		wchar_t temp##val[nLen+1] = L"";					\
		row.GetData(nCol+1, temp##val, sizeof(wchar_t)*(nLen+1), NULL, SQL_C_WCHAR);	\
		ucstring	val(temp##val);
		*/
#endif

#ifdef MYSQL_USE
	#define		COLUMN_DATETIME(row, nCol, val)				COLUMN_STR(row, nCol, val, 30)
#else
	#define		COLUMN_DATETIME(row, nCol, val)				COLUMN_STR(row, nCol, val, 30)
#endif

//#define MAKE_SP_GETCHARACTER(strQuery, strID)			\
//	char strQuery[256] = "";							\
//	sprintf(strQuery, "CALL SP_GETCHINFO('%s')", strID);

#ifdef MYSQL_USE
	#define	COLUMN_DOUBLE(row, nCol, val)
#else
	#define	COLUMN_DOUBLE(row, nCol, val)					\
		char Desc[20] = "";									\
		row.GetData(nCol+1, Desc, sizeof(Desc), NULL);		\
		double val = atof(Desc);
#endif

#ifdef MYSQL_USE
	#define		COLUMN_WSTR_BUF(row, nCol, val, buf, buflen)
#else
	#define		COLUMN_WSTR_BUF(row, nCol, val, buf, buflen)			\
		row.GetData(nCol+1, buf, sizeof(wchar_t) * buflen, NULL, SQL_C_WCHAR);	\
		ucstring	val(buf);
#endif
#endif //__DB_INTERFACE_H__
