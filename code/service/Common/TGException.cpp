/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center

Matt Pietrek
Microsoft Systems Journal, April 1997
FILE: MSJEXHND.CPP
<yjp modified @ 2010-5-14 to use in our product. /yjp>
-----------------------------------------------------------------------------
*/

#include "misc/types_sip.h"

#include <windows.h>

#include "TGException.h"
#include <tchar.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

LPTOP_LEVEL_EXCEPTION_FILTER MSJExceptionHandler::m_previousFilter;
unsigned int MSJExceptionHandler::m_nDumpLevel = 0;

MSJExceptionHandler g_MSJExceptionHandler; // Declare global instance of class

//============================== Class Methods =============================

//=============
// Constructor 
//=============
MSJExceptionHandler::MSJExceptionHandler( )
{
	// Install the unhandled exception filter function
	m_previousFilter = SetUnhandledExceptionFilter(MSJUnhandledExceptionFilter);
}

//============
// Destructor 
//============
MSJExceptionHandler::~MSJExceptionHandler( )
{
	SetUnhandledExceptionFilter( m_previousFilter );
}

//===========================================================
// Entry point where control comes on an unhandled exception 
//===========================================================
LONG WINAPI MSJExceptionHandler::MSJUnhandledExceptionFilter(
	PEXCEPTION_POINTERS pExceptionInfo )
{
	CreateMiniDumpFile(pExceptionInfo); // by HCJ

	Exit(false);

	if (m_previousFilter)
		return m_previousFilter( pExceptionInfo );
	else
		return EXCEPTION_CONTINUE_SEARCH;
}

//======================================================================
// Create mini dump file. by HCJ
//======================================================================
void MSJExceptionHandler::CreateMiniDumpFile(PEXCEPTION_POINTERS pExceptionInfo)
{
	TCHAR szFileName[MAX_PATH];

	TCHAR path[MAX_PATH] = _T("");
	TCHAR date[7] = _T(""), time[7] = _T("");

	GetModuleFileName(NULL, path, MAX_PATH);
	GetDateFormatW(LOCALE_USER_DEFAULT, 0, NULL, L"yyMMdd", date, 7);
	GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, NULL, L"hhmmss", time, 7);

	TCHAR drive[_MAX_DRIVE], dir[_MAX_PATH], fname[_MAX_FNAME], ext[_MAX_EXT];
	_tsplitpath_s(path, drive, dir, fname, ext);

	_stprintf(szFileName, _T("%s%s%s_%s-%s.dmp"), drive, dir, fname, date, time);

	DeleteOldLogs();

	HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return;

	MINIDUMP_EXCEPTION_INFORMATION ExInfo;

	ExInfo.ThreadId = ::GetCurrentThreadId();
	ExInfo.ExceptionPointers = pExceptionInfo;
	ExInfo.ClientPointers = false;

	// mini-dump
	MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory |
		MiniDumpScanMemory);
	
	switch (m_nDumpLevel)
	{
	case 1:
		// normal-dump
		mdt = MiniDumpNormal;
		break;
	case 2:
		// midi-dump
		mdt = (MINIDUMP_TYPE)(MiniDumpWithPrivateReadWriteMemory | 
								MiniDumpWithDataSegs |
								MiniDumpWithHandleData |
								MiniDumpWithFullMemoryInfo |
								MiniDumpWithThreadInfo |
								MiniDumpWithUnloadedModules); 
		break;
	case 3:
		// maxi-dump
		mdt = (MINIDUMP_TYPE)(MiniDumpWithFullMemory | 
								MiniDumpWithFullMemoryInfo | 
								MiniDumpWithHandleData | 
								MiniDumpWithThreadInfo | 
								MiniDumpWithUnloadedModules ); 
		break;
	}

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, &ExInfo, NULL, NULL);

	::CloseHandle(hFile);
}

void MSJExceptionHandler::DeleteOldLogs()
{
	TCHAR path[MAX_PATH] = _T("");
	GetModuleFileName(NULL, path, MAX_PATH);

	TCHAR drive[_MAX_DRIVE], dir[_MAX_PATH], fname[_MAX_FNAME], ext[_MAX_EXT];
	_tsplitpath_s(path, drive, dir, fname, ext);

	TCHAR szFilter[MAX_PATH];
	_stprintf(szFilter, _T("%s%s%s*.dmp"), drive, dir, fname);

	WIN32_FIND_DATA ff_data;
	HANDLE h_find = FindFirstFile(szFilter, &ff_data);
	while (h_find != INVALID_HANDLE_VALUE)
	{
		WCHAR cur_file[_MAX_PATH];
		_stprintf(cur_file, _T("%s%s%s"), drive, dir, ff_data.cFileName);

		DeleteFile(cur_file);

		if (!FindNextFile(h_find, &ff_data))
			break;
	}
}

//============================================================================
// Execute file for mail sending and down your project.
//============================================================================
void MSJExceptionHandler::Exit(bool del_olds)
{
//#ifdef _PRODUCT
//	// if dump level is greater than 0, don't send
//	if (m_nDumpLevel > 0)
//	{
//#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
//		MessageBox(NULL, __t("Don't send dump file because a file is big. Please connect to development team"), __t("Error"), MB_OK);
//#endif
//		return;
//	}
//
//	static bool called = false;
//
//	if (!called)
//	{
//		called = true;
//
//		if (del_olds)
//			DeleteOldLogs();
//
//		S3D::String root_path = S3D::Util::getModulePath();
//		S3D::String exe_path = root_path + SEND_MAIL_FILE + SEND_MAIL_EXT;
//
//		if (!S3D::Util::isExist(exe_path))
//			return;
//
//		TCHAR temp_path[_MAX_PATH];
//		GetTempPath(_MAX_PATH, temp_path);
//
//		// Copy send mail execution file to other name one for prevent firewall.
//		TCHAR szDate[7] = _T(""), szTime[7] = _T("");
//		GetDateFormatW(LOCALE_USER_DEFAULT, 0, NULL, _T("yyMMdd"), szDate, 7);
//		GetTimeFormatW(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, NULL, _T("hhmmss"), szTime, 7);
//
//		S3D::String dest_exe_path = S3D::StringUtil::toFormattedString(
//			_T("%s%s@%s-%s%s"), temp_path, SEND_MAIL_FILE, szDate, szTime, SEND_MAIL_EXT);
//
//		if (S3D::Util::CopyMoveFile(dest_exe_path, exe_path))
//			exe_path = dest_exe_path;
//
//		// Get identifier.
//		int id = 0;
//		TCHAR enter_file[_MAX_PATH];
//		wcscpy(enter_file, temp_path);
//		wcscat(enter_file, __t("tgenter.tmp"));
//		FILE* file = _wfopen(enter_file, __t("rt"));
//		if (file)
//		{
//			fwscanf_s(file, __t("%d"), &id);
//			fclose(file);
//		}
//
//		S3D::String version;
//		TianguoApplication* app = TianguoApplication::getSingletonPtr();
//		if (app)
//		{
//			version = app->m_version;
//			if (version.empty())
//				version = Version::getVersionDirect(app->m_sVersionPath, true);
//		}
//
//		if (version.empty())
//			version = __t("unknown");
//
//		S3D::String exe_arg;
////		exe_arg = S3D::StringUtil::toFormattedString(__t("-cientx(ver:%s id:%d)"), app->m_version.c_str(), id);
//		exe_arg = S3D::StringUtil::toFormattedString(__t("\"%s\" -cientx(ver:%s id:%d) /path\"%s\""), 
//			exe_path.c_str(), app->m_version.c_str(), id, root_path.c_str());
//
////		ShellExecute(NULL, __t("open"), exe_path.c_str(), exe_arg.c_str(), __t(""), SW_SHOW);
//		_wexeclp(exe_path.c_str(), exe_arg.c_str(), 0);
//	}
//	else
//	{
//		exit(-1);
//	}
//#endif
}
