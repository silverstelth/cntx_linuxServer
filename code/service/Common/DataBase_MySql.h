#ifndef DATABASE_MYSQL_H
#define DATABASE_MYSQL_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include <mysql.h>
#include <misc/types_sip.h>
#include "net/service.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

// 자료기지 접속자
extern MYSQL *DatabaseConnection;

/**
 * Encapsulation of MySQL result
 */
class CMySQLResult
{
public:

	/// Constructor
	CMySQLResult(MYSQL* _database)
	{ 
		m_DatabaseConnection = _database;
		m_Result = mysql_store_result(m_DatabaseConnection);
		m_bSuccess = true;
		m_bIsFieldCount = true;
		if (m_Result == NULL)
		{
			if (mysql_field_count(m_DatabaseConnection) != 0)
				m_bSuccess = false;
			else
				m_bIsFieldCount = false;
		}
	}
	/// Destructor
	~CMySQLResult()
	{
		if (m_Result != NULL)
		{
			mysql_free_result(m_Result);
			mysql_next_result(m_DatabaseConnection);
		}
	}

	/// Cast operator
	operator MYSQL_RES*()
	{
		return m_Result;
	}
	/// Affectation
	CMySQLResult&	operator = (MYSQL_RES* res)
	{
		if (res == m_Result)
			return *this;
		if (m_Result != NULL)
			mysql_free_result(m_Result);
		m_Result = res;
		return *this;
	}

	/// Test success
	bool			IsSuccess() const
	{
		return m_bSuccess;
	}
	/// Test failure
	bool			IsFailed() const
	{
		return !IsSuccess();
	}

	/// Number of rows of result
	uint			NumberRows()
	{
		if (m_bIsFieldCount)
			return (uint)mysql_num_rows(m_Result);
		return (uint)mysql_affected_rows(m_DatabaseConnection);
	}

	/// Fetch row
	MYSQL_ROW		FetchRow()
	{
		return mysql_fetch_row(m_Result);
	}


private:

	MYSQL_RES*	m_Result;
	bool		m_bSuccess;
	bool		m_bIsFieldCount;
	MYSQL		*m_DatabaseConnection;

};


// SQL Helpers, : By KSR add
#define		COLUMN_DIGIT(nCol, Type) 	(Type)atoui(string(row[nCol]).c_str())
#define		COLUMN_STR(nCol) 	string(row[nCol])
#define		COLUMN_UCSTR(nCol) 	ucstring::makeFromUtf8(string(row[nCol]))
#define		UCFROMUTF8(str)		ucstring::makeFromUtf8(str)

// By KSR add
#define DB_EXE_QUERY_WITHONEROW(__strQuery) \
	DB_EXE_QUERY_WITHRESULT(__strQuery)	\
	MYSQL_ROW row;		\
	if (bSuccess && uRowNum >= 1)	\
	{	\
		row = res.FetchRow();	\
		if (row == NULL)	\
			bSuccess = false;	\
	}	\
	else	\
	{	\
		bSuccess = false;	\
	}	\

// 한개 col을 가진 한개의 row를 결과로 하는 query를 실행하는 마크로
#define DB_EXE_QUERY_WITHONECOLROW(__strQuery) \
	DB_EXE_QUERY_WITHRESULT(__strQuery)	\
	string	strResult;		\
	if (bSuccess && uRowNum == 1)	\
	{	\
		MYSQL_ROW row = res.FetchRow();	\
		if (row != NULL)	\
			strResult = string(row[0]);	\
		else	\
			bSuccess = false;	\
	}

// query를 실행하고 결과를 받는 마크로
/************************************************************************/
#define DB_EXE_QUERY_WITHRESULT(__strQuery) \
	DB_EXE_QUERY(__strQuery)	\
	uint32 uRowNum = 0;			\
	CMySQLResult res(DatabaseConnection);	\
	if (bSuccess)				\
	{							\
		if (res.IsFailed())	\
		{	\
			WARNING_TO_LOG("mysql_store_result () failed from query '%s': %s", __strQuery.c_str (),  mysql_error(DatabaseConnection));	\
			bSuccess = false;	\
		}	\
		else	\
			uRowNum = res.NumberRows();	\
	}
/************************************************************************/

// 일반query를 실행하는 마크로
/************************************************************************/
#define DB_EXE_QUERY(__strQuery) \
	bool	bSuccess = true;	\
	int ret = mysql_query (DatabaseConnection, __strQuery.c_str());	\
	if (ret != 0)	\
{	\
	WARNING_TO_LOG("mysql_query (%s) failed: %s", __strQuery.c_str(),  mysql_error(DatabaseConnection));	\
	bSuccess = false;	\
}	
/************************************************************************/


// stored procedure를 실행하는 마크로
#define	SP_EXCUTE(_strQuery) \
	DB_EXE_QUERY(_strQuery);	\
	if (bSuccess)	\
	{	\
		do \
		{	\
			MYSQL_RES*	sqlResult = mysql_store_result(DatabaseConnection);	\
			if (sqlResult == NULL)	\
			{	\
				if (mysql_field_count(DatabaseConnection) == 0)	\
					break;	\
				else	\
				{	\
					WARNING_TO_LOG("mysql_store_result () failed from query '%s': %s", _strQuery.c_str (),  mysql_error(DatabaseConnection));	\
					bSuccess = false;	\
					break;	\
				}	\
			}	\
			else	

#define	IF_SP_SUCCESS() \
			{	\
					uint32	uRowNum = (uint32)mysql_num_rows(sqlResult);

#define	ENDIF_SP_SUCCESS() \
			}	\
			mysql_free_result(sqlResult);	\
		} while(!mysql_next_result(DatabaseConnection));	\
	}

#define	IF_SP_FAILED() \
	if (!bSuccess)	\
	{
#define	ENDIF_SP_FAILED() \
	}

// 자료기지사용을 위한 초기화및 접속를 진행하는 마크로
/****************************************************************************/
#define INIT_DATABASE() \
{	\
	/* 설정파일에서 자료기지접속과 관련한 값들을 얻는다	*/	\
	string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("DatabaseName").asString ();	\
	string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("DatabaseHost").asString ();	\
	string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("DatabaseLogin").asString ();	\
	string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("DatabasePassword").asString ();	\
	\
	/* 자료기지접속을 위한 초기화 */	\
	MYSQL *db = mysql_init(NULL);	\
	if(db == NULL)	\
	{	\
		siperrornoex("mysql_init() failed");	\
		return;	\
	}	\
	InfoLog->_displayNL("Database initialization is successed");	\
	\
	/* 자료기지접속 */	\
	DatabaseConnection = mysql_real_connect(db, strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabasePassword.c_str(), strDatabaseName.c_str(),0,NULL,CLIENT_MULTI_STATEMENTS); \
	if (DatabaseConnection == NULL || DatabaseConnection != db) \
	{	\
		siperrornoex("mysql_real_connect() failed to '%s' with login '%s' and database name '%s'", \
			strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());	\
		return;	\
	}	\
	InfoLog->_displayNL("Database connection is successed");	\
}
/****************************************************************************/
// 자료기지와의 접속을 닫는 마크로
/****************************************************************************/
#define CLOSE_DATABASE() \
{	\
	if (DatabaseConnection != NULL)	\
	{	\
		mysql_close(DatabaseConnection);	\
		DatabaseConnection = NULL;			\
	}	\
}
/****************************************************************************/


#endif // DATABASE_MYSQL_H

