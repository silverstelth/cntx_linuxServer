#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <wchar.h>
#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <odbcinst.h>
#include "type.h"

/***************************************/
/* Macro to call ODBC functions and    */
/* report an error on failure.         */
/* Takes handle, handle type, and stmt */
/***************************************/
#define TRYODBC(h, ht, x) {  \
	if (noError) \
	{ \
		RETCODE rc = x; \
		if (rc != SQL_SUCCESS) \
		{ \
			HandleDiagnosticRecord (h, ht, rc); \
		} \
		if (rc == SQL_ERROR) \
		{ \
			fwprintf(stderr, L"Error in %d \n", #x); \
			noError = false; \
		} \
	} \
}

#define SQL_QUERY_SIZE			1000

/******************************************/
/* Forward references                     */
/******************************************/
void HandleDiagnosticRecord (SQLHANDLE      hHandle,
                 SQLSMALLINT    hType,
                 RETCODE        RetCode);


bool Connect(lpcwstr connStr);

SQLWCHAR* ConvertString(lpcwstr str);

void DisplayResults(HSTMT hStmt,
					SQLSMALLINT cCols);

SQLWCHAR* ConvertString(lpcwstr cStr, int& len)
{
	std::wstring str(cStr);
	int strLen = str.length();
	SQLWCHAR* sqlConnStr = (SQLWCHAR*)malloc(sizeof(SQLWCHAR) * strLen);
	for (int i = 0; i < strLen; i++)
	{
		sqlConnStr[i] = cStr[i];
	}
	return sqlConnStr;
}

void DisplayResults(HSTMT hStmt,
					SQLSMALLINT cCols)
{
	wprintf(L"cols = %d", cCols);
	BINDING *pFirstBinding, *pThisBinding;
	SQLSMALLINT cDisplaySize;
	RETCODE RetCode = SQL_SUCCESS;
	int iCount = 0;

	// Allocate memory for each column
	AllocateBindings(hStmt, cCols, &pFirstBinding, &cDisplaySize);
	
}

bool Connect(lpcwstr connStr)
{
	bool noError = true;
	SQLHENV hEnv = NULL;
	SQLHDBC	hDbc = NULL;
	SQLHSTMT hStmt = NULL;
	wchar_t wszInput[SQL_QUERY_SIZE];

	if (SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv) == SQL_ERROR)
	{
		fwprintf(stderr, L"Unable to alocate and environment handle\n");
		return true;
	}
	
	// Register this as an application that expects 3.x behavior,
	// you must register something if you use AllocHandle
	
	TRYODBC(hEnv,
		SQL_HANDLE_ENV,
		SQLSetEnvAttr(hEnv,
			SQL_ATTR_ODBC_VERSION,
			(SQLPOINTER)SQL_OV_ODBC3,
			0));

	// Allocate a connection
	TRYODBC(hEnv,
		SQL_HANDLE_ENV,
		SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc));

	// Connect to the driver.
	if (noError)
	{
		int strLen;
		SQLWCHAR* sqlConnStr = ConvertString(connStr, strLen);
		wprintf(L"%S : %d\n", connStr, strLen);

		TRYODBC(hDbc,
			SQL_HANDLE_DBC,
			SQLDriverConnectW(hDbc,
							NULL,
							sqlConnStr,
							SQL_NTS,
							NULL,
							0,
							NULL,
							SQL_DRIVER_COMPLETE));

		fwprintf(stdout, noError ? L"Connected!\n" : L"Failed to connect!\n");

		TRYODBC(hDbc,
			SQL_HANDLE_DBC,
			SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt));

		if (noError)
		{
			wprintf(L"Enter SQL Commands, type (control)Z to exit\nSQL COMMAND>");

			while(fwscanf(stdin, L"%S", wszInput))
			{
				RETCODE RetCode;
				SQLSMALLINT sNumResults;
				
				// Execute the query
				if (!(*wszInput))
				{
					wprintf(L"SQL COMMAND>");
					continue;
				}

				int strLen;
				SQLWCHAR* sqlConnStr = ConvertString(wszInput, strLen);
				RetCode = SQLExecDirectW(hStmt, sqlConnStr, SQL_NTS);
				switch(RetCode)
				{
				case SQL_SUCCESS_WITH_INFO:
				{
					HandleDiagnosticRecord(hStmt, SQL_HANDLE_STMT, RetCode);
				}
				case SQL_SUCCESS:
				{
					// If this is a row-returning, display
					// results
					TRYODBC(hStmt,
						SQL_HANDLE_STMT,
						SQLNumResultCols(hStmt, &sNumResults));

					if (sNumResults > 0)
					{
						DisplayResults(hStmt, sNumResults);
					}
					else
					{
						SQLLEN cRowCount;
						
						TRYODBC(hStmt,
							SQL_HANDLE_STMT,
							SQLRowCount(hStmt, &cRowCount));

						if (cRowCount >= 0)
						{
							wprintf(L"%Id %s affected\n",
								cRowCount,
								cRowCount == 1 ? L"row" : L"rows");
						}
					}
					break;
				}
				case SQL_ERROR:
				{
					HandleDiagnosticRecord(hStmt, SQL_HANDLE_STMT, RetCode);
					break;
				}
				default:
					fwprintf(stderr, L"Unexpected return code %hd!\n", RetCode);
				}
			}
		}
	}

	if (!noError)
	{
		if (hStmt)
		{
			SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
		}

		if (hDbc)
		{
			SQLDisconnect(hDbc);
			SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
		}

		if (hEnv)
		{
			SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
		}
	}

	return noError;
}

/************************************************************************
/* HandleDiagnosticRecord : display error/warning information
/*
/* Parameters:
/*      hHandle     ODBC handle
/*      hType       Type of handle (HANDLE_STMT, HANDLE_ENV, HANDLE_DBC)
/*      RetCode     Return code of failing command
/************************************************************************/
void HandleDiagnosticRecord (SQLHANDLE      hHandle,
                             SQLSMALLINT    hType,
                             RETCODE        RetCode)
{
	SQLSMALLINT iRec = 0;
	SQLINTEGER	iError;
	WCHAR		wszMessage[1000];
	WCHAR		wszState[SQL_SQLSTATE_SIZE+1];
}


int main(int argc, const char** args)
{
	wchar_t buf[100];
	swprintf(buf, sizeof(buf), L"Driver=FreeTDS;Server=%S;Port=%d;UID=%S;PWD=%S;Database=%S;", L"192.168.8.198", 1433, L"sa", L"Dd123", L"Main");
	if (!Connect(buf))
		return -1;

	return 0;
}
