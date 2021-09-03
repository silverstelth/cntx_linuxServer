/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TGEXCEPTION_H__
#define __TGEXCEPTION_H__

class MSJExceptionHandler
{
public:

	MSJExceptionHandler( );
	~MSJExceptionHandler( );

	//void SetLogFileName( TCHAR* pszLogFileName );
	// entry point where control comes on an unhandled exception
	static LONG WINAPI MSJUnhandledExceptionFilter(
		PEXCEPTION_POINTERS pExceptionInfo );
	
	static void Exit(bool del_olds = true);

	static LPTOP_LEVEL_EXCEPTION_FILTER m_previousFilter;

	static void SetDumpLevel(unsigned int nLevel) { m_nDumpLevel = nLevel; }
	static unsigned int GetDumpLevel() { return m_nDumpLevel; }
private:

	// Create mini dump file. (HCJ 2012/1/17)
	static void CreateMiniDumpFile( PEXCEPTION_POINTERS pExceptionInfo );

	static void DeleteOldLogs();

	static unsigned int m_nDumpLevel;
};

extern MSJExceptionHandler g_MSJExceptionHandler;   // global instance of class

#endif // end of guard __TGEXCEPTION_H__