//////////////////////////////////////////////////////////////////////////////////
/* MySQL Wrapper file for Project(FtpDown.cpp) Created by KSR, Date: 2007.10.12 */

#include <stdio.h>
#include "sqlWrapper.h"

using namespace	std;


MYSQL			*dbConnection = NULL;
static			CMySQLResult	g_sqlResult;

void	CMySQLResult::init(MYSQL* database)
{
	_Result = mysql_store_result(database);
}

void	CMySQLResult::init(MYSQL_RES* res)
{
	_Result = res;
}

void	CMySQLResult::release()
{
	if (_Result != NULL)
		mysql_free_result(_Result);

	_Result = NULL;
}

/// Cast operator
CMySQLResult::operator MYSQL_RES*()
{
	return _Result;
}

/// Affectation
CMySQLResult&	CMySQLResult::operator = (MYSQL_RES* res)
{
	if (res == _Result)
		return *this;
	if (_Result != NULL)
		mysql_free_result(_Result);
	_Result = res;
	return *this;
}

CMySQLResult&	CMySQLResult::operator = (CMySQLResult& sqlResult)
{
	if (sqlResult._Result == _Result)
		return *this;
	if (_Result != NULL)
		mysql_free_result(_Result);
	_Result = sqlResult._Result;

	return *this;
}


/// Test success
bool			CMySQLResult::success() const
{
	return _Result != NULL;
}
/// Test failure
bool			CMySQLResult::failed() const
{
	if (!success())
	{
		sipwarning ("mysql_fetch_row () failed : %s", mysql_error(dbConnection));
	}

	return !success();
}



/// Number of rows of result
uint			CMySQLResult::numRows()
{
	return (uint)mysql_num_rows(_Result);
}
/// Fetch row
MYSQL_ROW		CMySQLResult::fetchRow()
{
	return mysql_fetch_row(_Result);
}

//////////////////////////////////////////////////////////////////////////
// SQL Helpers
BOOL sqlInit (string sHost, string sLogin, string sPassword, string sDBName)
{
	//	if (DontUseDataBase)
	//		return;

	MYSQL *db = mysql_init(NULL);
	if(db == NULL)
	{
		siperror ("mysql_init() failed");
		return FALSE;
	}

	dbConnection = mysql_real_connect(db, sHost.c_str(), sLogin.c_str(), sPassword.c_str(), sDBName.c_str(), 0,NULL,0);
	if (dbConnection == NULL || dbConnection != db)
	{
		siperror ("mysql_real_connect() failed to '%s' with login '%s' and database name '%s' with %s",
			sHost.c_str(), sLogin.c_str(), sDBName.c_str(),
			sPassword.empty()?"empty password":"password");
		return FALSE;
    }
	return TRUE;
}

// sqlQueryOneRow() : 
// sqlFlushResult()
MYSQL_ROW sqlQueryOneRow (const char *format, ...)
{
	char	*query;
	SIPBASE_CONVERT_VARGS(query, format, MAX_SQL_LEN);
	
	if (dbConnection == 0)
	{
		sipwarning ("MYSQL: mysql_query (%s) failed: DatabaseConnection is 0", query);
		return NULL;
	}

	int ret = mysql_query (dbConnection, query);
	if (ret != 0)
	{
		sipwarning ("MYSQL: mysql_query () failed for query '%s': %s", query,  mysql_error(dbConnection));
		return NULL;
	}

	g_sqlResult.init(dbConnection);
	
	if ( g_sqlResult.failed() )
	{
		sipwarning ("MYSQL: mysql_store_result () failed for query '%s': %s", query,  mysql_error(dbConnection));
		return NULL;
	}

	uint32 nRows = (uint32) g_sqlResult.numRows();
	if ( nRows == 0 )
	{
		sipinfo("MYSQL: mysql_num_rows()  : has 0 row '%s': %s", query,  mysql_error(dbConnection));
        return	NULL;
	}

	MYSQL_ROW row = g_sqlResult.fetchRow();
	if (row == 0)
	{
		sipwarning ("MYSQL: mysql_fetch_row () failed for query '%s': %s", query,  mysql_error(dbConnection));
	}
	sipdebug ("MYSQL: sqlQueryOneRow(%s) returns %d rows", query, nRows);

	return row;
}

MYSQL_ROW sqlNextRow ()
{
	if ( g_sqlResult.failed() )
		return 0;

	return	g_sqlResult.fetchRow();
}

void	sqlFlushResult()
{
	g_sqlResult.release();
}

// When success return 0
uint sqlQueryWithoutRes(const char *format, ...)
{
	char	*query;
	SIPBASE_CONVERT_VARGS(query, format, MAX_SQL_LEN);

	if (dbConnection == 0)
	{
		sipwarning ("MYSQL: mysql_query (%s) failed: DatabaseConnection is 0", query);
		return ERR_DB;
	}

	int ret = mysql_query (dbConnection, query);
	if (ret != 0)
	{
		sipwarning ("MYSQL: mysql_query () failed for query '%s': %s", query,  mysql_error(dbConnection));
		return ERR_DB;
	}

	return 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////////

CMySQLResult	*sqlQuery (const char *format, ...)
{
	char	*query;
	SIPBASE_CONVERT_VARGS(query, format, MAX_SQL_LEN);

	if (dbConnection == 0)
	{
		sipwarning ("MYSQL: mysql_query (%s) failed: DatabaseConnection is 0", query);
		return NULL;
	}

	int ret = mysql_query (dbConnection, query);
	if (ret != 0)
	{
		sipwarning ("MYSQL: mysql_query () failed for query '%s': %s", query,  mysql_error(dbConnection));
		return NULL;
	}

	g_sqlResult.init(dbConnection);

	return	&g_sqlResult;
}