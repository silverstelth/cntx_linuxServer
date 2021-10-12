#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <wchar.h>
#include <sql.h>
#include <sqlext.h>
#include <sqlucode.h>
#include <odbcinst.h>
#include <math.h>

#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "type.h"

using namespace std;

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

typedef struct STR_BINDING {
	SQLSMALLINT cDisplaySize;		// size to display
	WCHAR *wszBuffer;				// display buffer
	SQLLEN indPtr;					// size or null
	BOOL fChar;						// character col?
	struct STR_BINDING *sNext;		// linked list
} BINDING;

#define DISPLAY_MAX				50
#define DISPLAY_FORMAT_EXTRA	3
#define DISPLAY_FORMAT			L"%c %*.*s "
#define DISPLAY_FORMAT_C		L"%c %-*.*s "
#define NULL_SIZE				6
#define SQL_QUERY_SIZE			1000

#define PIPE					L'|'

short gHeight = 80;

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

void AllocateBindings(HSTMT hStmt,
						SQLSMALLINT cCols,
						BINDING **ppBinding,
						SQLSMALLINT *pDisplay);

void DisplayTitles(HSTMT    hStmt,
                   DWORD    cDisplaySize,
                   BINDING* pBinding);



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

void DisplayTitles(HSTMT    hStmt,
                   DWORD    cDisplaySize,
                   BINDING* pBinding)
{
	bool noError = true;
	WCHAR	wszTitle[DISPLAY_MAX];
	SQLSMALLINT iCol = 1;
	
	for (; pBinding; pBinding = pBinding->sNext)
	{
		TRYODBC(hStmt,
                SQL_HANDLE_STMT,
                SQLColAttribute(hStmt,
                    iCol++,
                    SQL_DESC_NAME,
                    wszTitle,
                    sizeof(wszTitle), // Note count of bytes!
                    NULL,
                    NULL));

		if (noError)
		{
			wprintf(DISPLAY_FORMAT_C,
		             PIPE,
		             pBinding->cDisplaySize,
		             pBinding->cDisplaySize,
		             wszTitle);
		}
	}

	if (noError != false)
	{
		wprintf(L" %c\n", PIPE);
	}
}

void AllocateBindings(HSTMT hStmt,
						SQLSMALLINT cCols,
						BINDING **ppBinding,
						SQLSMALLINT *pDisplay)
{
	bool noError = true;
	SQLSMALLINT iCol;
	BINDING *pThisBinding, *pLastBinding = NULL;
	SQLLEN cchDisplay, ssType;
	SQLSMALLINT cchColumnNameLength;
	
	*pDisplay = 0;
	for (iCol = 1; iCol <= cCols; iCol++)
	{
		pThisBinding = (BINDING*)(malloc(sizeof(BINDING)));
		if (!(pThisBinding))
		{
			fwprintf(stderr, L"Out of memory!\n");
			exit(-100);
		}

		if (iCol == 1)
		{
			*ppBinding = pThisBinding;
		}
		else
		{
			pLastBinding->sNext = pThisBinding;
		}
		pLastBinding = pThisBinding;
		
		// Figure out the display length of the column (we will
		// bind to char since we are only displaying data, in general
		// you should bind to the appropriate C type if you are going
		// to manipulate data since it is much faster...)
		
		TRYODBC(hStmt,
			SQL_HANDLE_STMT,
			SQLColAttribute(hStmt,
				iCol,
				SQL_DESC_DISPLAY_SIZE,
				NULL,
				0,
				NULL,
				&cchDisplay));

		// Figure out if this is a character or numeric column; this is
		// used to determine if we want to display the data left- or right-
		// aligned.
		
		// SQL_DESC_CONCISE_TYPE maps to the 1.x SQL_COLUMN_TYPE
		// This is what you must use if you want to work
		// against a 2.x driver.
		
		TRYODBC(hStmt,
			SQL_HANDLE_STMT,
			SQLColAttribute(hStmt,
				iCol,
				SQL_DESC_CONCISE_TYPE,
				NULL,
				0,
				NULL,
				&ssType));
		
		pThisBinding->fChar = (ssType == SQL_CHAR ||
                                ssType == SQL_VARCHAR ||
                                ssType == SQL_LONGVARCHAR);

		pThisBinding->sNext = NULL;

		// Arbitrary limit on display size
		if (cchDisplay > DISPLAY_MAX)
			cchDisplay = DISPLAY_MAX;

		// Allocate a buffer big enough to hold the text representation
		// of the data. Add one character for the null terminator
		pThisBinding->wszBuffer = (WCHAR*)malloc((cchDisplay+1)*sizeof(WCHAR));

		if (!(pThisBinding->wszBuffer))
        {
            fwprintf(stderr, L"Out of memory!\n");
            exit(-100);
        }

        // Map this buffer to the driver's buffer.   At Fetch time,
        // the driver will fill in this data.  Note that the size is
        // count of bytes (for Unicode).  All ODBC functions that take
        // SQLPOINTER use count of bytes; all functions that take only
        // strings use count of characters.

		TRYODBC(hStmt,
                SQL_HANDLE_STMT,
                SQLBindCol(hStmt,
                    iCol,
                    SQL_C_TCHAR,
                    (SQLPOINTER) pThisBinding->wszBuffer,
                    (cchDisplay + 1) * sizeof(WCHAR),
                    &pThisBinding->indPtr));
		
		// Now set the display size that we will use to display
        // the data.   Figure out the length of the column name

        TRYODBC(hStmt,
                SQL_HANDLE_STMT,
                SQLColAttribute(hStmt,
                    iCol,
                    SQL_DESC_NAME,
                    NULL,
                    0,
                    &cchColumnNameLength,
                    NULL));

		pThisBinding->cDisplaySize = fmax((SQLSMALLINT)cchDisplay, cchColumnNameLength);
		if (pThisBinding->cDisplaySize < NULL_SIZE)
			pThisBinding->cDisplaySize = NULL_SIZE;

		*pDisplay += pThisBinding->cDisplaySize + DISPLAY_FORMAT_EXTRA;
	}
}

void DisplayResults(HSTMT hStmt,
					SQLSMALLINT cCols)
{
	wprintf(L"cols = %d\n", cCols);

	bool noError = true;
	BINDING *pFirstBinding, *pThisBinding;
	SQLSMALLINT cDisplaySize;
	RETCODE RetCode = SQL_SUCCESS;
	int iCount = 0;

	// Allocate memory for each column
	AllocateBindings(hStmt, cCols, &pFirstBinding, &cDisplaySize);
	
	// Set the display mode and write the titles
	DisplayTitles(hStmt, cDisplaySize+1, pFirstBinding);

	// Fetch and display the data
	bool fNoData = false;
	do {
		// Fetch a row
		if (iCount++ >= gHeight - 2)
		{
			int nInputChar;
			bool fEnterReceived = false;
			
			while(!fEnterReceived)
			{
				wprintf(L"              ");
                wprintf(L"   Press ENTER to continue, Q to quit (height:%hd)", gHeight);

				nInputChar = scanf("%c");
                wprintf(L"\n");
                if ((nInputChar == 'Q') || (nInputChar == 'q'))
                {
                    goto Exit;
                }
                else if ('\r' == nInputChar)
                {
                    fEnterReceived = true;
                }
                // else loop back to display prompt again
			}

			iCount = 1;
			DisplayTitles(hStmt, cDisplaySize+1, pFirstBinding);
		}

		TRYODBC(hStmt, SQL_HANDLE_STMT, RetCode = SQLFetch(hStmt));

		if (!noError)
			goto Exit;

		if (RetCode == SQL_NO_DATA_FOUND)
        {
            fNoData = true;
        }
        else
		{
			// Display the data, Ignore truncations
			for (pThisBinding = pFirstBinding;
				pThisBinding;
				pThisBinding = pThisBinding->sNext)
			{
				if (pThisBinding->indPtr != SQL_NULL_DATA)
                {
                    wprintf(pThisBinding->fChar ? DISPLAY_FORMAT_C : DISPLAY_FORMAT,
                        PIPE,
                        pThisBinding->cDisplaySize,
                        pThisBinding->cDisplaySize,
                        pThisBinding->wszBuffer);
                }
                else
                {
                    wprintf(DISPLAY_FORMAT_C,
                        PIPE,
                        pThisBinding->cDisplaySize,
                        pThisBinding->cDisplaySize,
                        L"<NULL>");
                }
				wprintf(L" %c\n",PIPE);
			}
		}
	} while(!fNoData);

	wprintf(L"%*.*s", cDisplaySize+2, cDisplaySize+2, L" ");
    wprintf(L"\n");

Exit:
	// Clean up the allocated buffers
	while(pFirstBinding)
	{
		pThisBinding = pFirstBinding->sNext;
		free(pFirstBinding->wszBuffer);
		free(pFirstBinding);
		pFirstBinding = pThisBinding;
	}
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


void GetIPAddress()
{
	struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }
        if (ifa->ifa_addr->sa_family == AF_INET) { // check it is IP4
            // is a valid IP4 Address
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            wprintf(L"%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        } else if (ifa->ifa_addr->sa_family == AF_INET6) { // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            wprintf(L"%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
        } 
    }
    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
}

static wchar_t str_buf[200 * 2+20];
static uint8 buf[200] = {0,1,0,0,5,6,'b',0,0,'a',0,0,0,1,0,0};
//303130303563
int main(int argc, const char** args)
{
	//wchar_t test[100];
	//char test_str = 'a';
	//short  dd=20;
	//swprintf(test, 100, L"%2x",test_str);
	//test[3] = L'\0';
	//wprintf(L"%ls %x %x %x\n", test, test[0],test[1],test[2]);

	for (int i = 0; i < 10; i++)
	{
		swprintf(&str_buf[i*2], 4, L"%02x", buf[i]);
//str_buf[i] = (buf[i]>='a' && buf[i]<='z')?buf[i]:buf[i]+'0';
		wprintf(L"%ls,%4x\t", &str_buf[i], str_buf[i]);
	}
//wprintf(L"%c,%x\t", str_buf[0], str_buf[0]);
	wprintf(L"\nhere: %ls\n", str_buf);
 	wprintf(L"hi test\n");

	// wprintf(L"wchar = %d, short = %d\n", sizeof(wchar_t), sizeof(unsigned short));

	/*
	wchar_t buf[100];
	swprintf(buf, sizeof(buf), L"Driver=FreeTDS;Server=%S;Port=%d;UID=%S;PWD=%S;Database=%S;", L"192.168.8.198", 1433, L"sa", L"Dd123", L"Main");
	
	if (!Connect(buf))
		return -1;
	*/

	GetIPAddress();

	return 0;
}
