//////////////////////////////////////////////////////////////////////////
/* MySQL Wrapper file for Project(FtpDown.cpp) Created by KSR, Date: 2007.10.12 */

#ifndef _SQLWRAPPER_H_
#define _SQLWRAPPER_H_

#include "misc/types_sip.h"

#if defined (SIP_OS_WINDOWS)
#include <windows.h>
#endif

#include <mysql.h>
#include "net/service.h"

#include "../common/LoginCommon.h"
#include "../../common/err.h"

// SQL Helpers

#define		COLUMN_DIGIT(nCol, Type) 	(Type)atoui(string(row[nCol]).c_str())
#define		COLUMN_STR(nCol) 	string(row[nCol])
#define		COLUMN_UCSTR(nCol) 	ucstring::makeFromUtf8(string(row[nCol]))



/**
* Encapsulation of MySQL result
*/
class CMySQLResult
{
public:

	/// Constructor
	void init(MYSQL_RES* res = NULL);
	/// Constructor
	void init(MYSQL* database);
	
	void	release();

	/// Cast operator
	operator MYSQL_RES*();

	/// Affectation
	CMySQLResult&	operator = (MYSQL_RES* res);

	CMySQLResult&	operator = (CMySQLResult& sqlResult);

	/// Test success
	bool			success() const;
	/// Test failure
	bool			failed() const;



	/// Number of rows of result
	uint			numRows();
	/// Fetch row
	MYSQL_ROW		fetchRow();


private:

	MYSQL_RES*	_Result;

};

BOOL		sqlInit (std::string sHost, std::string sLogin, std::string sPassword, std::string sDBName);
MYSQL_ROW	sqlQueryOneRow (const char *format, ...);
uint		sqlQueryWithoutRes(const char *format, ...);
MYSQL_ROW	sqlNextRow ();
void		sqlFlushResult();

CMySQLResult	*sqlQuery (const char *format, ...);

#endif // _SQLWRAPPER_H_