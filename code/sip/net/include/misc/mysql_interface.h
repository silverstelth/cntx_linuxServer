/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MYSQL_INTERFACE_H__
#define __MYSQL_INTERFACE_H__

#include <mysql/mysql.h>

#define IS_SQL_OK(ret) (ret == 0)

class MYSQLConnection
{
public:
	MYSQLConnection()
	{
		m_DatabaseConnection = NULL;
	};
	virtual ~MYSQLConnection(){};
	bool Connect(const char* User, const char* Pass, const char* Host, const char* Database, uint32 Port = 3306)
	{
		MYSQL *db = mysql_init(NULL);  
		if(db == NULL)  
			return false;  
  
	 	m_DatabaseConnection = mysql_real_connect(db, Host, User, Pass, Database, Port, NULL,CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS); 
		if (m_DatabaseConnection == NULL || m_DatabaseConnection != db)  
			return false;  
		return true;  
	};  
	void Disconnect()
	{
		if (m_DatabaseConnection != NULL)
		{
			mysql_close(m_DatabaseConnection);
			m_DatabaseConnection = NULL;
		}
	}
	int	Query(const char* strQuery)
	{
		return mysql_query (m_DatabaseConnection, strQuery);
	}
	bool	IsConnected()
	{
		if (m_DatabaseConnection != NULL)
			return true;
		return false;
	}
	operator MYSQL*()
	{
		return m_DatabaseConnection;
	}
private:
	MYSQL		*m_DatabaseConnection;
};

 
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
		Release();
	}

	void Release() 
	{
		if (m_Result != NULL)
		{
			mysql_free_result(m_Result);
			mysql_next_result(m_DatabaseConnection);
			m_Result = NULL;
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
	int			NumberRows()
	{
		if (m_bIsFieldCount)
			return (int)mysql_num_rows(m_Result);
		return (int)mysql_affected_rows(m_DatabaseConnection);
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

class CMysqlStmt 
{
public:
	MYSQL_STMT* m_hStmt;
public:
	CMysqlStmt(MYSQL* mysql)
	{
		m_hStmt = mysql_stmt_init(mysql);
	}
	virtual ~CMysqlStmt()
	{
		if ( m_hStmt != NULL )
			mysql_stmt_close(m_hStmt);
	}
	
	bool IsValid()
	{
		return m_hStmt != NULL;
	}
	uint32 GetColumnCount()
	{
		return mysql_stmt_field_count(m_hStmt);
	}
	uint32 GetChangedRowCount(void)
	{
		return (uint32)mysql_stmt_affected_rows(m_hStmt);
	}
	uint32 GetRowCount(void)
	{
		return (uint32)mysql_stmt_num_rows(m_hStmt);
	}
	/*bool Query( LPCTSTR strSQL)
	{

		SQLRETURN nRet=SQLExecDirect( m_hStmt, (SQLTCHAR *)strSQL, SQL_NTS );
		return IS_SQL_OK( nRet );
	}*/
	bool Prepare(const char* strSQL)
	{
		int nRet = mysql_stmt_prepare(m_hStmt, strSQL, strlen(strSQL)) ;
        return ( nRet == 0 );
	}
	bool BindParamIn(MYSQL_BIND* bind)
	{
		int nRet = mysql_stmt_bind_param(m_hStmt, bind);
		return ( nRet == 0);

	}
	bool BindParamOut(MYSQL_BIND* bind)
	{
		int nRet = mysql_stmt_bind_result(m_hStmt, bind);
		return( nRet == 0);
	}
	bool ParamData(void **prgbValue)
	{
		sipassert("not prepared");
		return false;
	}
	bool Query()
	{
		int nRet = mysql_stmt_execute( m_hStmt );
		return ( nRet == 0 );
	}
	bool Fetch()
	{
		int nRet=mysql_stmt_fetch(m_hStmt);
		return ( nRet == 0 );
	}
	bool Store()
	{
		int nRet = mysql_stmt_store_result(m_hStmt);
		return (nRet == 0);
	}
	bool FreeResult()
	{
		int nRet = mysql_stmt_free_result(m_hStmt);
		return (nRet == 0);
	}
	bool FecthRow(uint32 nRow)
	{
		sipassert("not prepared");
		return false;
	}
	bool FetchPrevious()
	{
		sipassert("not prepared");
		return false;
	}
	bool FecthNext()
	{
		sipassert("not prepared");
		return false;
	}
	bool FetchRow(uint32 nRow,bool Absolute=1)
	{
		sipassert("not prepared");
		return false;
	}
	bool FetchFirst()
	{
		sipassert("not prepared");
		return false;
	}
	bool FetchLast()
	{
		sipassert("not prepared");
		return false;
	}
	bool Cancel()
	{
		sipassert("not prepared");
		return false;
	}
};
#endif //__MYSQL_INTERFACE_H__
