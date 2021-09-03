/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"


#ifdef SIP_OS_WINDOWS
#	include <io.h>
#	include <fcntl.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#else
#	include <cerrno>
#endif // SIP_OS_WINDOWS

#include <stdio.h> // <cstdio>
#include <stdlib.h> // <cstdlib>
#include <ctime>

#include <iostream>
#include <fstream>
//#include <sstream>
#include <iomanip>

#include "misc/path.h"
#include "misc/mutex.h"
#include "misc/report.h"

#include "misc/debug.h"


#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#else
#	define IsDebuggerPresent() false
#endif

#include "misc/displayer.h"

using namespace std;

namespace SIPBASE
{

IDisplayer_base::IDisplayer_base(const char *displayerName)
{
	_Mutex = new CMutex (string(displayerName) + "DISP");
	DisplayerName = displayerName;
}

IDisplayer_base::~IDisplayer_base()
{
	delete _Mutex;
}

static const char *LogTypeToString[][8] = {
	{ "", "ERR", "WRN", "INF", "DBG", "STT", "AST", "UKN" },
	{ "", "Error", "Warning", "Information", "Debug", "Statistic", "Assert", "Unknown" },
	{ "", "A fatal error occurs. The program must quit", "", "", "", "", "A failed assertion occurs", "" },
};

static const ucchar *LogTypeToStringW[][8] = {
	{ L"", L"ERR", L"WRN", L"INF", L"DBG", L"STT", L"AST", L"UKN" },
	{ L"", L"Error", L"Warning", L"Information", L"Debug", L"Statistic", L"Assert", L"Unknown" },
	{ L"", L"A fatal error occurs. The program must quit", L"", L"", L"", L"", L"A failed assertion occurs", L"" },
};

const char *IDisplayerA::logTypeToString (ILog::TLogType logType, bool longFormat)
{
	if (logType < ILog::LOG_NO || logType > ILog::LOG_UNKNOWN)
		return "<NotDefined>";

	return LogTypeToString[longFormat ? 1 : 0][logType];
}

const char *IDisplayerA::dateToHumanString ()
{
	time_t date;
	time (&date);
	return dateToHumanString (date);
}

const char *IDisplayerA::dateToHumanString (time_t date)
{
	static char cstime[25];
	struct tm *tms = localtime(&date);

	if (tms)
		strftime (cstime, 25, "%Y/%m/%d %H:%M:%S", tms);
	else
		sprintf(cstime, "bad date %d", (uint32)date);
	
	return cstime;
}

const char *IDisplayerA::dateToComputerString (time_t date)
{
	static char cstime[25];

	smprintf (cstime, 25, "%ld", &date);
	return cstime;
}

const char *IDisplayerA::HeaderString ()
{
	static char header[1024];

	smprintf(header, 1024, "\nLog Starting [%s]\n", dateToHumanString());
	return header;
}

/*
 * Display the string where it does.
 */
void IDisplayerA::display (const CLogA::TDisplayInfo& args, const char *message)
{
	_Mutex->enter();
	try
	{
		doDisplay( args, message );
	}
	catch (Exception &)
	{
		// silence
	}
	_Mutex->leave();
}

// unicode version
const ucchar *IDisplayerW::logTypeToString (ILog::TLogType logType, bool longFormat)
{
	if (logType < ILog::LOG_NO || logType > ILog::LOG_UNKNOWN)
		return L"<NotDefined>";

	return LogTypeToStringW[longFormat ? 1 : 0][logType];
}

const ucchar *IDisplayerW::dateToHumanString ()
{
	time_t date;
	time (&date);
	return dateToHumanString (date);
}

const ucchar *IDisplayerW::dateToHumanString (time_t date)
{
	static ucchar cstime[25];
	struct tm *tms = localtime(&date);

	if (tms)
		wcsftime (cstime, 25, L"%Y/%m/%d %H:%M:%S", tms);
	else
		smprintf(cstime, 25, L"bad date %d", (uint32)date);
	
	return cstime;
}

const ucchar *IDisplayerW::dateToComputerString (time_t date)
{
	static ucchar ucstime[25];
	smprintf (ucstime, 25, L"%ld", &date);
	return ucstime;
}

const ucchar *IDisplayerW::HeaderString ()
{
	static ucchar header[1024];
	smprintf (header, 1024, L"\nLog Starting [%s]\n", dateToHumanString());
	return header;
}

/*
 * Display the string where it does.
 */
void IDisplayerW::display (const CLogW::TDisplayInfo& args, const ucchar *message)
{
	_Mutex->enter();
	try
	{
		doDisplay( args, message );
	}
	catch (Exception &)
	{
		// silence
	}
	_Mutex->leave();
}

// Log format : "<LogType> <ThreadNo> <FileName> <Line> <ProcessName> : <Msg>"
void CStdDisplayerA::doDisplay ( const CLogA::TDisplayInfo& args, const char *message )
{
	bool needSpace = false;
	string str;

	if (args.LogType != ILog::LOG_NO)
	{
		str += logTypeToString(args.LogType);
		needSpace = true;
	}

	// Write thread identifier
	if ( args.ThreadId != 0 )
	{
		if (needSpace) { str += " "; needSpace = false; }
		#ifdef SIP_OS_WINDOWS
			str += SIPBASE::toString("%5x", args.ThreadId);
		#else
			str += SIPBASE::toString("%d", args.ThreadId);
		#endif
		needSpace = true;
	}

	if (args.FileName != NULL)
	{
		str += " "; 
		needSpace = false; 
		str += CFileA::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1)
	{
		str += " "; 
		needSpace = false; 
		str += SIPBASE::toStringA(args.Line);
		needSpace = true;
	}
	
	if (args.FuncName != NULL)
	{
		if (needSpace)
		{
			str += " "; 
			needSpace = false; 
		}

		str += args.FuncName;
		needSpace = true;
	}

	if (!args.ProcessName.empty())
	{
		if (needSpace)
		{
			str += " "; 
			needSpace = false; 
		}

		str += args.ProcessName;
		needSpace = true;
	}

	if (needSpace)
	{
		str += " : ";
		needSpace = false;
	}
	
	str += message;
	
	static bool consoleMode = true;

#if defined(SIP_OS_WINDOWS)
	static bool consoleModeTest = false;
	if (!consoleModeTest)
	{
		HANDLE handle;
		handle = CreateFileA ("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
		consoleMode = handle != INVALID_HANDLE_VALUE;
		if (consoleMode)
			CloseHandle (handle);
		consoleModeTest = true;
	}
#endif // SIP_OS_WINDOWS

	// Printf ?
	if (consoleMode)
	{
		// we don't use cout because sometimes, it crashs because cout isn't already init, printf doesn t crash.
		if (!str.empty())
			printf ("%s", str.c_str());
		
		if (!args.CallstackAndLog.empty())
			printf (args.CallstackAndLog.c_str());
		
		fflush(stdout);
	}

#ifdef SIP_OS_WINDOWS
	// display the string in the debugger if the application is started with the debugger
	if (!IsDebuggerPresent ())
		return;
	
	string str2;
	needSpace = false;

	if (args.FileName != NULL) str2 += args.FileName;

	if (args.Line != -1)
	{
		str2 += "(" + SIPBASE::toStringA(args.Line) + ")";
		needSpace = true;
	}

	if (needSpace)
	{
		str2 += " : ";
		needSpace = false;
	}

	if (args.FuncName != NULL)
	{
		str2 += string(args.FuncName);
		str2 += " ";
	}

	if (args.LogType != ILog::LOG_NO)
	{
		str2 += logTypeToString(args.LogType);
		needSpace = true;
	}

	// Write thread identifier
	if ( args.ThreadId != 0 )
	{
		str2 += SIPBASE::toString("%5x: ", args.ThreadId);
	}

	str2 += message;

	const sint maxOutString = 2*1024;

	if(str2.size() < maxOutString)
	{
		//////////////////////////////////////////////////////////////////
		// WARNING: READ THIS !!!!!!!!!!!!!!!! ///////////////////////////
		// If at the release time, it freezes here, it's a microsoft bug:
		// http://support.microsoft.com/support/kb/articles/q173/2/60.asp
		OutputDebugStringA(str2.c_str());
	}
	else
	{
		sint count = 0;	
		uint n = strlen(message);
		std::string s(&str2.c_str()[0], (str2.size() - n));
		OutputDebugStringA(s.c_str());
		
		while(true)
		{
			if((n - count) < maxOutString )
			{
				s = std::string(&message[count], (n - count));
				OutputDebugStringA(s.c_str());
				OutputDebugStringA("\n");
				break;
			}	
			else
			{
				s = std::string(&message[count] , count + maxOutString);
				OutputDebugStringA(s.c_str());
				OutputDebugStringA("\n\t\t\t");
				count += maxOutString;
			}
		}
	}

	// OutputDebugString is a big shit, we can't display big string in one time, we need to split
	uint32 pos = 0;
	string splited;
	while(true)
	{
		if (pos+1000 < args.CallstackAndLog.size ())
		{
			splited = args.CallstackAndLog.substr (pos, 1000);
			OutputDebugStringA(splited.c_str());
			pos += 1000;
		}
		else
		{
			splited = args.CallstackAndLog.substr (pos);
			OutputDebugStringA(splited.c_str());
			break;
		}
	}
#endif
}

void CStdDisplayerW::doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message )
{
	bool needSpace = false;
	std::basic_string<ucchar> str;

	if (args.LogType != CLogW::LOG_NO)
	{
		str += logTypeToString(args.LogType);
		needSpace = true;
	}

	// Write thread identifier
	if ( args.ThreadId != 0 )
	{
		if (needSpace) { str += L" "; needSpace = false; }
		#ifdef SIP_OS_WINDOWS
			str += SIPBASE::toString(L"%5x", args.ThreadId);
		#else
			str += SIPBASE::toStringLW("%d", args.ThreadId);
		#endif
		needSpace = true;
	}

	if (!args.FileName.empty())
	{
		if (needSpace)
		{
			str += L" ";
			needSpace = false; 
		}
		str += CFileW::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1)
	{
		if (needSpace)
		{
			str += L" ";
			needSpace = false; 
		}
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString(L"%hd", args.Line);
#else
		str += SIPBASE::toStringLW("%hd", args.Line);
#endif
		needSpace = true;
	}
	
	if (!args.FuncName.empty())
	{
		if (needSpace)
		{
			str += L" ";
			needSpace = false; 
		}

		str += args.FuncName;
		needSpace = true;
	}

	if (!args.ProcessName.empty())
	{
		if (needSpace)
		{
			str += L" ";
			needSpace = false; 
		}

		str += args.ProcessName.c_str(); // KSR_ADD 2010_1_30
		needSpace = true;
	}

	if (needSpace)
	{
		str += L" : ";
		needSpace = false;
	}
	
	str += message;
	
	static bool consoleMode = true;

#if defined(SIP_OS_WINDOWS)
	static bool consoleModeTest = false;
	if (!consoleModeTest)
	{
		HANDLE handle;
		handle = CreateFileW (L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);

		consoleMode = handle != INVALID_HANDLE_VALUE;
		if (consoleMode)
			CloseHandle (handle);
		consoleModeTest = true;
	}
#endif // SIP_OS_WINDOWS

	// Printf ?
	if (consoleMode)
	{
		// we don't use cout because sometimes, it crashs because cout isn't already init, printf doesn t crash.

#ifdef SIP_OS_WINDOWS
		if (!str.empty())
			wprintf (L"%s", str.c_str());
#else
		if (!str.empty())
			wprintf (L"%S", str.c_str());
#endif	
		if (!args.CallstackAndLog.empty())
		{
#ifdef SIP_OS_WINDOWS
			wprintf (L"%s", args.CallstackAndLog.c_str());
#else
			wprintf (L"%S", args.CallstackAndLog.c_str());
#endif
		}
		
		fflush(stdout);
	}

#ifdef SIP_OS_WINDOWS
	// display the string in the debugger if the application is started with the debugger
	if (!IsDebuggerPresent ())
		return;
	
	basic_string<ucchar> str2;
	needSpace = false;

	if (!args.FileName.empty())
		str2 += args.FileName;

	if (args.Line != -1)
	{
#ifdef SIP_OS_WINDOWS
		str2 += L"(" + SIPBASE::toString(L"%hd", args.Line) + L")";
#else
		str2 += L"(" + SIPBASE::toStringLW("%hd", args.Line) + L")";
#endif
		needSpace = true;
	}

	if (needSpace)
	{
		str2 += L" : ";
		needSpace = false;
	}

	if (!args.FuncName.empty())
	{
		str2 += args.FuncName;
		str2 += L" ";
	}

	if (args.LogType != CLogW::LOG_NO)
	{
		str2 += logTypeToString(args.LogType);
		needSpace = true;
	}

	// Write thread identifier
	if ( args.ThreadId != 0 )
	{
#ifdef SIP_OS_WINDOWS
		str2 += SIPBASE::toString(L"%5x: ", args.ThreadId);
#else
		str2 += SIPBASE::toStringLW("%5x: ", args.ThreadId);
#endif
	}

	str2 += message;

	const sint maxOutString = 2*1024;

	if (str2.size() < maxOutString)
	{
		OutputDebugStringW(str2.c_str());
	}
	else
	{
		sint count = 0;	
		uint n = wcslen(message);
		std::basic_string<ucchar> s(&str2.c_str()[0], (str2.size() - n));
		OutputDebugStringW(s.c_str());
		
		while(true)
		{			
			if((n - count) < maxOutString )
			{
				s = std::basic_string<ucchar>(&message[count], (n - count));
				OutputDebugStringW(s.c_str());
				OutputDebugStringW(L"\n");
				break;
			}	
			else
			{
				s = std::basic_string<ucchar>(&message[count] , count + maxOutString);
				OutputDebugStringW(s.c_str());
				OutputDebugStringW(L"\n\t\t\t");
				count += maxOutString;
			}
		}
	}

	// OutputDebugString is a big shit, we can't display big string in one time, we need to split
	uint32 pos = 0;
	std::basic_string<ucchar> splited;
	while(true)
	{
		if (pos+1000 < args.CallstackAndLog.size ())
		{
			splited = args.CallstackAndLog.substr (pos, 1000);
			OutputDebugStringW(splited.c_str());
			pos += 1000;
		}
		else
		{
			splited = args.CallstackAndLog.substr (pos);
			OutputDebugStringW(splited.c_str());
			break;
		}
	}
#endif
}

CFileDisplayerA::CFileDisplayerA (const std::string &filename, bool eraseLastLog, const char *displayerName, bool raw) :
	IDisplayerA (displayerName), _NeedHeader(true), _LastLogSizeChecked(0), _Raw(raw)
{
	_FilePointer = (FILE*)1;
	setParam (filename, eraseLastLog);
}

CFileDisplayerA::CFileDisplayerA (bool raw) :
	IDisplayerA (""), _NeedHeader(true), _LastLogSizeChecked(0), _Raw(raw)
{
	_FilePointer = (FILE*)1;
}

CFileDisplayerA::~CFileDisplayerA ()
{
	if (_FilePointer > (FILE*)1)
	{
		fclose(_FilePointer);
		_FilePointer = NULL;
	}
}

void CFileDisplayerA::setParam (const std::string &filename, bool eraseLastLog)
{
	_FileName = filename;

	if (filename.empty())
	{
		// can't do sipwarning or infinite recurs
		printf ("CFileDisplayer::setParam(): Can't create file with empty filename\n");
		return;
	}

	if (eraseLastLog)
	{
		ofstream ofs (filename.c_str(), ios_base::out | ios_base::trunc);
		if (!ofs.is_open())
		{
			// can't do sipwarning or infinite recurs
			printf ("CFileDisplayer::setParam(): Can't open and clear the log file '%s'\n", filename.c_str());
		}

		// Erase all the derived log files
		int i = 0;
		bool fileExist = true;
		while (fileExist)
		{
			string fileToDelete = CFileA::getPath (filename) + CFileA::getFilenameWithoutExtension (filename) + 
				toString ("%05d.", i) + CFileA::getExtension (filename);
			fileExist = CFileA::isExists (fileToDelete);
			if (fileExist)
				CFileA::deleteFile (fileToDelete);
			i++;
		}		
	}

	if (_FilePointer > (FILE*)1)
	{
		fclose (_FilePointer);
		_FilePointer = (FILE*)1;
	}
}

// Log format: "2000/01/15 12:05:30 <ProcessName> <LogType> <ThreadId> <FileName> <Line> : <Msg>"
void CFileDisplayerA::doDisplay ( const CLogA::TDisplayInfo& args, const char *message )
{
	bool needSpace = false;
	//stringstream ss;
	std::string str;

	// if the filename is not set, don't log
	if (_FileName.empty())
		return;

	if (args.Date != 0 && !_Raw)
	{
		str += dateToHumanString(args.Date);
		needSpace = true;
	}

	if (args.LogType != ILog::LOG_NO && !_Raw)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += logTypeToString(args.LogType);
		needSpace = true;
	}

	// Write thread identifier
	if ( args.ThreadId != 0 && !_Raw)
	{
		if (needSpace) { str += " "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString("%4x", args.ThreadId);
#else
		str += SIPBASE::toString("%4u", args.ThreadId);
#endif
		needSpace = true;
	}

	if (!args.ProcessName.empty() && !_Raw)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += args.ProcessName;
		needSpace = true;
	}

	if (args.FileName != NULL && !_Raw)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += CFileA::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1 && !_Raw)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += SIPBASE::toStringA(args.Line);
		needSpace = true;
	}
	
	if (args.FuncName != NULL && !_Raw)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += args.FuncName;
		needSpace = true;
	}

	if (needSpace) { str += " : "; needSpace = false; }

	str += message;

	if (_FilePointer > (FILE*)1)
	{
		// if the file is too big (>5mb), rename it and create another one (check only after 20 lines to speed up)
		if (_LastLogSizeChecked++ > 20)
		{
			int res = ftell (_FilePointer);
			if (res > 5*1024*1024)
			{
				fclose (_FilePointer);
				rename (_FileName.c_str(), CFileA::findNewFile (_FileName).c_str());
				_FilePointer = (FILE*) 1;
				_LastLogSizeChecked = 0;
			}
		}
	}

	if (_FilePointer == (FILE*)1)
	{
		_FilePointer = fopen (_FileName.c_str(), "at");
		if (_FilePointer == NULL)
			printf ("Can't open log file '%s': %s\n", _FileName.c_str(), strerror (errno));
	}
	
	if (_FilePointer != 0)
	{
		if (_NeedHeader)
		{
			const char *hs = HeaderString();
			fwrite (hs, strlen (hs), 1, _FilePointer);
			_NeedHeader = false;
		}
		
		if(!str.empty())
			fwrite (str.c_str(), str.size(), 1, _FilePointer);

		if(!args.CallstackAndLog.empty())
			fwrite (args.CallstackAndLog.c_str(), args.CallstackAndLog.size (), 1, _FilePointer);

		fflush (_FilePointer);
	}	
}

// unicode version of CFileDisplayer
CFileDisplayerW::CFileDisplayerW (const std::string &filename, bool eraseLastLog, const char *displayerName, bool raw) :
	IDisplayerW (displayerName), _NeedHeader(true), _LastLogSizeChecked(0), _Raw(raw)
{
	_FilePointer = (FILE*)1;
	setParam (filename, eraseLastLog);
}

CFileDisplayerW::CFileDisplayerW (const std::basic_string<ucchar> &filename, bool eraseLastLog, const char *displayerName, bool raw) :
	IDisplayerW (displayerName), _NeedHeader(true), _LastLogSizeChecked(0), _Raw(raw)
{
	_FilePointer = (FILE*)1;
	setParam (filename, eraseLastLog);
}

CFileDisplayerW::CFileDisplayerW (bool raw) :
	IDisplayerW (""), _NeedHeader(true), _LastLogSizeChecked(0), _Raw(raw)
{
	_FilePointer = (FILE*)1;
}

CFileDisplayerW::~CFileDisplayerW ()
{
	if (_FilePointer > (FILE*)1)
	{
		fclose(_FilePointer);
		_FilePointer = NULL;
	}
}

void CFileDisplayerW::setParam (const std::string &filename, bool eraseLastLog)
{
	setParam (SIPBASE::getUcsFromMbs(filename.c_str()), eraseLastLog);
}

void CFileDisplayerW::setParam (const std::basic_string<ucchar> &filename, bool eraseLastLog)
{
	_FileName = filename;

	if (filename.empty())
	{
		// can't do sipwarning or infinite recurs
		wprintf (L"CFileDisplayer::setParam(): Can't create file with empty filename\n");
		return;
	}

	if (eraseLastLog)
	{
		if (CFileW::isExists (filename))
			CFileW::deleteFile(filename);
		else
		{
			// can't do sipwarning or infinite recurs
#ifdef SIP_OS_WINDOWS
			wprintf (L"CFileDisplayer::setParam(): Can't open and clear the log file '%s'\n", filename.c_str());
#else
			wprintf (L"CFileDisplayer::setParam(): Can't open and clear the log file '%S'\n", filename.c_str());
#endif
		}

		// Erase all the derived log files
		sint i = 0;
		bool fileExist = true;
		while (fileExist)
		{
			std::basic_string<ucchar> fileToDelete = CFileW::getPath (filename) + CFileW::getFilenameWithoutExtension (filename) + 
				SIPBASE::toString (L"%03d.", i) + CFileW::getExtension (filename);
			fileExist = CFileW::isExists (fileToDelete);
			if (fileExist)
				CFileW::deleteFile (fileToDelete);
			i ++;
		}
	}

	if (_FilePointer > (FILE*)1)
	{
		fclose (_FilePointer);
		_FilePointer = (FILE*)1;
	}
}

// Log format: "2000/01/15 12:05:30 <ProcessName> <LogType> <ThreadId> <FileName> <Line> : <Msg>"
void CFileDisplayerW::doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message )
{
	bool needSpace = false;
	//stringstream ss;
	std::basic_string<ucchar> str;

	// if the filename is not set, don't log
	if (_FileName.empty())
		return;

	if (args.Date != 0 && !_Raw)
	{
		str += dateToHumanString(args.Date);
		needSpace = true;
	}

	if (args.LogType != ILog::LOG_NO && !_Raw)
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += logTypeToString(args.LogType);
		needSpace = true;
	}

	// Write thread identifier
	if ( args.ThreadId != 0 && !_Raw)
	{
		if (needSpace) { str += L" "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString(L"%4x", args.ThreadId);
#else
		str += SIPBASE::toStringLW("%4u", args.ThreadId);
#endif
		needSpace = true;
	}

	if (!args.ProcessName.empty() && !_Raw)
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += args.ProcessName;
		needSpace = true;
	}

	if (!args.FileName.empty() && !_Raw)
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += CFileW::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1 && !_Raw)
	{
		if (needSpace) { str += L" "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString(L"%hd", args.Line);
#else
		str += SIPBASE::toStringLW("%hd", args.Line);
#endif
		needSpace = true;
	}
	
	if (!args.FuncName.empty() && !_Raw)
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += args.FuncName;
		needSpace = true;
	}

	if (needSpace) { str += L" : "; needSpace = false; }

	str += message;

	if (_FilePointer > (FILE*)1)
	{
		// if the file is too big (>5mb), rename it and create another one (check only after 20 lines to speed up)
		if (_LastLogSizeChecked++ > 20)
		{
			int res = ftell (_FilePointer);
			if (res > 5*1024*1024)
			{
				fclose (_FilePointer);
				sfWrename (_FileName.c_str(), CFileW::findNewFile (_FileName).c_str());
				_FilePointer = (FILE*) 1;
				_LastLogSizeChecked = 0;
			}
		}
	}

	if (_FilePointer == (FILE*)1)
	{
		bool fileExist = CFileW::isExists(_FileName);
		_FilePointer = sfWfopen (_FileName.c_str(), L"ab");
		if (_FilePointer == NULL)
#ifdef SIP_OS_WINDOWS
			wprintf (L"Can't open log file '%s': %s\n", _FileName.c_str(), _wcserror (errno));
#else
			wprintf (L"Can't open log file '%S': %d\n", _FileName.c_str(), errno);
#endif
		else if (fileExist == false)
		{
			ucchar sign = 0xfeff;
			fputwc(sign, _FilePointer);
		}
	}
	
	if (_FilePointer != 0)
	{
		if (_NeedHeader)
		{
			const ucchar *hs = HeaderString();

			std::basic_string<ucchar> str1;
			ucchar *npos, *pos = const_cast<ucchar *>(hs);
			while ((npos = wcschr (pos, L'\n')))
			{
				*npos = L'\0';
				str1 += pos;
				str1 += L"\r\n";
				*npos = L'\n';
				pos = npos + 1;
			}
			str1 += pos;
			fputws (str1.c_str(), _FilePointer);
			
			_NeedHeader = false;
		}
		
		if (!str.empty())
		{
			std::basic_string<ucchar> str1;
			ucchar *npos, *pos = const_cast<ucchar *>(str.c_str());
			while ((npos = wcschr (pos, L'\n')))
			{
				*npos = L'\0';
				str1 += pos;
				str1 += L"\r\n";
				*npos = L'\n';
				pos = npos + 1;
			}
			str1 += pos;
			fputws (str1.c_str(), _FilePointer);
		}

		if (!args.CallstackAndLog.empty())
		{
			std::basic_string<ucchar> str1;
			ucchar *npos, *pos = const_cast<ucchar *>(args.CallstackAndLog.c_str());
			while ((npos = wcschr (pos, L'\n')))
			{
				*npos = L'\0';
				str1 += pos;
				str1 += L"\r\n";
				*npos = L'\n';
				pos = npos + 1;
			}
			str1 += pos;

			fputws (str1.c_str(), _FilePointer);
		}

		fflush (_FilePointer);
	}
}

// Log format in clipboard: "2000/01/15 12:05:30 <LogType> <ProcessName> <FileName> <Line>: <Msg>"
// Log format on the screen: in debug   "<ProcessName> <FileName> <Line>: <Msg>"
//                           in release "<Msg>"
void CMsgBoxDisplayerA::doDisplay ( const CLogA::TDisplayInfo& args, const char *message)
{
#ifdef SIP_OS_WINDOWS

	bool needSpace = false;
//	stringstream ss;
	string str;

	// create the string for the clipboard

	if (args.Date != 0)
	{
		str += dateToHumanString(args.Date);
		needSpace = true;
	}

	if (args.LogType != ILog::LOG_NO)
	{
		//if (needSpace) { ss << " "; needSpace = false; }
		if (needSpace) { str += " "; needSpace = false; }
		str += logTypeToString(args.LogType);
		needSpace = true;
	}

	if (!args.ProcessName.empty())
	{
		//if (needSpace) { ss << " "; needSpace = false; }
		if (needSpace) { str += " "; needSpace = false; }
		str += args.ProcessName;
		needSpace = true;
	}
	
	if (args.FileName != NULL)
	{
		//if (needSpace) { ss << " "; needSpace = false; }
		if (needSpace) { str += " "; needSpace = false; }
		str += CFileA::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1)
	{
		//if (needSpace) { ss << " "; needSpace = false; }
		if (needSpace) { str += " "; needSpace = false; }
		str += SIPBASE::toStringA(args.Line);
		needSpace = true;
	}

	if (args.FuncName != NULL)
	{
		//if (needSpace) { ss << " "; needSpace = false; }
		if (needSpace) { str += " "; needSpace = false; }
		str += args.FuncName;
		needSpace = true;
	}

	if (needSpace) { str += ": "; needSpace = false; }

	str += message;

	if (OpenClipboard (NULL))
	{
		HGLOBAL mem = GlobalAlloc (GHND|GMEM_DDESHARE, str.size()+1);
		if (mem)
		{
			char *pmem = (char *)GlobalLock (mem);
			strcpy (pmem, str.c_str());
			GlobalUnlock (mem);
			EmptyClipboard ();
			SetClipboardData (CF_TEXT, mem);
		}
		CloseClipboard ();
	}
	
	// create the string on the screen
	needSpace = false;
//	stringstream ss2;
	string str2;

#ifdef SIP_DEBUG
	if (!args.ProcessName.empty())
	{
		if (needSpace) { str2 += " "; needSpace = false; }
		str2 += args.ProcessName;
		needSpace = true;
	}
	
	if (args.FileName != NULL)
	{
		if (needSpace) { str2 += " "; needSpace = false; }
		str2 += CFileA::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1)
	{
		if (needSpace) { str2 += " "; needSpace = false; }
		str2 += SIPBASE::toStringA(args.Line);
		needSpace = true;
	}

	if (args.FuncName != NULL)
	{
		if (needSpace) { str2 += " "; needSpace = false; }
		str2 += args.FuncName;
		needSpace = true;
	}

	if (needSpace) { str2 += ": "; needSpace = false; }

#endif // SIP_DEBUG

	str2 += message;
	str2 += "\n\n(this message was copied in the clipboard)";

/*	if (IsDebuggerPresent ())
	{
		// Must break in assert call
		DebugNeedAssert = true;
	}
	else
*/	{

		// Display the report

		string body;

		body += toString(LogTypeToString[2][args.LogType]) + "\n";
		body += "ProcName: " + args.ProcessName + "\n";
		body += "Date: " + string(dateToHumanString(args.Date)) + "\n";
		if(args.FileName == NULL)
			body += "File: <Unknown>\n";
		else
			body += "File: " + string(args.FileName) + "\n";
		body += "Line: " + toStringA(args.Line) + "\n";
		if (args.FuncName == NULL)
			body += "FuncName: <Unknown>\n";
		else
			body += "FuncName: " + string(args.FuncName) + "\n";
		body += "Reason: " + toString(message);

		body += args.CallstackAndLog;

		string subject;

		// procname is host/service_name-sid we only want the service_name to avoid redondant mail
		string procname;
		sint pos = args.ProcessName.find ("/");
		if (pos == string::npos)
		{
			procname =  args.ProcessName;
		}
		else
		{
			sint pos2 = args.ProcessName.find ("-", pos+1);
			if (pos2 == string::npos)
			{
				procname =  args.ProcessName.substr (pos+1);
			}
			else
			{
				procname =  args.ProcessName.substr (pos+1, pos2-pos-1);
			}
		}

		subject += procname + " SIP " + toString(LogTypeToString[0][args.LogType]) + " " + (args.FileName ? string(args.FileName):"") + " " + toStringA(args.Line) + " " + (args.FuncName?string(args.FuncName):"");

		// Check the envvar SIP_IGNORE_ASSERT
		if (getenv ("SIP_IGNORE_ASSERT") == NULL)
		{
			// yoyo: allow only to send the crash report once. Because users usually click ignore, 
			// which create noise into list of bugs (once a player crash, it will surely continues to do it).
			if  (ReportDebug == report (args.ProcessName + " SIP " + toString(logTypeToString(args.LogType, true)), "", subject, body, true, 2, true, 1, !isCrashAlreadyReported(), IgnoreNextTime, SIP_CRASH_DUMP_FILE))
			{
				ISipContext::getInstance().setDebugNeedAssert(true);
			}

			// no more sent mail for crash
			setCrashAlreadyReported(true);
		}

/*		// Check the envvar SIP_IGNORE_ASSERT
		if (getenv ("SIP_IGNORE_ASSERT") == NULL)
		{
			// Ask the user to continue, debug or ignore
			int result = MessageBox (NULL, ss2.str().c_str (), logTypeToString(args.LogType, true), MB_ABORTRETRYIGNORE | MB_ICONSTOP);
			if (result == IDABORT)
			{
				// Exit the program now
				exit (EXIT_FAILURE);
			}
			else if (result == IDRETRY)
			{
				// Give the debugger a try
				DebugNeedAssert = true;
 			}
			else if (result == IDIGNORE)
			{
				// Continue, do nothing
			}
		}
*/	}

#endif
}

void CMsgBoxDisplayerW::doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message)
{
#ifdef SIP_OS_WINDOWS

	bool needSpace = false;
	std::basic_string<ucchar> str;

	// create the string for the clipboard
	if (args.Date != 0)
	{
		str += dateToHumanString(args.Date);
		needSpace = true;
	}

	if (args.LogType != ILog::LOG_NO)
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += logTypeToString(args.LogType);
		needSpace = true;
	}

	if (!args.ProcessName.empty())
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += args.ProcessName;
		needSpace = true;
	}
	
	if (!args.FileName.empty())
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += CFileW::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1)
	{
		if (needSpace) { str += L" "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString(L"%hd", args.Line);
#else
		str += SIPBASE::toStringLW("%hd", args.Line);
#endif
		needSpace = true;
	}

	if (!args.FuncName.empty())
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += args.FuncName;
		needSpace = true;
	}

	if (needSpace) { str += L": "; needSpace = false; }

	str += message;

	if (OpenClipboard (NULL))
	{
		HGLOBAL mem = GlobalAlloc (GHND | GMEM_DDESHARE, (str.size() + 1) * sizeof(ucchar));
		if (mem)
		{
			ucchar *pmem = (ucchar *)GlobalLock (mem);
			wcscpy (pmem, str.c_str());
			GlobalUnlock (mem);
			EmptyClipboard ();
			SetClipboardData (CF_TEXT, mem);
		}
		CloseClipboard ();
	}
	
	// Display the report

	std::basic_string<ucchar> body;

	body += SIPBASE::toString(LogTypeToStringW[2][args.LogType]) + L"\n";
#ifdef SIP_OS_WINDOWS
	body += SIPBASE::toString(L"ProcName: %s\n", args.ProcessName.c_str());
	body += SIPBASE::toString(L"Date: %s\n", dateToHumanString(args.Date));
	if (args.FileName.empty())
		body += L"File: <Unknown>\n";
	else
		body += SIPBASE::toString(L"File: %s\n", args.FileName.c_str());

	body += SIPBASE::toString(L"Line: %d\n", args.Line);

	if (args.FuncName.empty())
		body += L"FuncName: <Unknown>\n";
	else
		body += SIPBASE::toString(L"FuncName: %s\n", args.FuncName.c_str());

	body += SIPBASE::toString(L"Reason: %s", message);

	body += args.CallstackAndLog;

#else
	body += SIPBASE::toStringLW("ProcName: %s\n", args.ProcessName.c_str());
	body += SIPBASE::toStringLW("Date: %s\n", dateToHumanString(args.Date));
	if (args.FileName.empty())
		body += L"File: <Unknown>\n";
	else
		body += SIPBASE::toStringLW("File: %s\n", args.FileName.c_str());

	body += SIPBASE::toStringLW("Line: %d\n", args.Line);

	if (args.FuncName.empty())
		body += L"FuncName: <Unknown>\n";
	else
		body += SIPBASE::toStringLW("FuncName: %s\n", args.FuncName.c_str());

	body += SIPBASE::toStringLW("Reason: %s", message);

	body += args.CallstackAndLog;
#endif

	// procname is host/service_name-sid we only want the service_name to avoid redundant mail
	
	std::basic_string<ucchar> subject, procname, procname1;
	procname1 = args.ProcessName;
	
	sint pos = procname1.find (L"/");

	if (pos == string::npos)
	{
		procname =  procname1;
	}
	else
	{
		sint pos2 = procname1.find (L"-", pos+1);
		if (pos2 == string::npos)
		{
			procname =  procname1.substr (pos+1);
		}
		else
		{
			procname =  procname1.substr (pos+1, pos2-pos-1);
		}
	}

	subject += (procname + L" SIP ");
	subject += LogTypeToStringW[0][args.LogType];

	if (!args.FileName.empty())
		subject += (L" " + args.FileName);

#ifdef SIP_OS_WINDOWS
	subject +=  (L" " + SIPBASE::toString(L"%hd", args.Line));
#else
	subject +=  (L" " + SIPBASE::toStringLW("%hd", args.Line));
#endif
	
	if (!args.FuncName.empty())
		subject += (L" " + args.FuncName);

	// Check the envvar SIP_IGNORE_ASSERT
	if (getenv ("SIP_IGNORE_ASSERT") == NULL)
	{
		// yoyo: allow only to send the crash report once. Because users usually click ignore, 
		// which create noise into list of bugs (once a player crash, it will surely continues to do it).
		if  (ReportDebug == reportW(procname1 + L" SIP " + SIPBASE::toString(logTypeToString(args.LogType, true)), L"", subject, body, true, 2, true, 1, !isCrashAlreadyReported(), IgnoreNextTime, SIP_CRASH_DUMP_FILE_W))
		{
			ISipContext::getInstance().setDebugNeedAssert(true);
		}

		// no more sent mail for crash
		setCrashAlreadyReported(true);
	}
#endif
}

} // SIPBASE
