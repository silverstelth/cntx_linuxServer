/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/types_sip.h"

#include <stdio.h> // <cstdio>

#include <iostream>
#include <fstream>
//#include <sstream>
#include <iomanip>
#include <string>

#include "misc/mem_displayer.h"
#include "misc/path.h"
#include "misc/command.h"
#include "misc/debug.h"

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#	include <imagehlp.h>
#	pragma comment(lib, "imagehlp.lib")
#endif // SIP_OS_WINDOWS

using namespace std;

namespace SIPBASE {


//  UNTIL we found who to walk in the call stack wihtout error, we disactive this feature


#ifdef SIP_OS_WINDOWS

static const uint32 stringSize = 1024;

static string getFuncInfo (DWORD funcAddr, DWORD stackAddr)
{
	string str ("NoSymbol");

	DWORD symSize = 10000;
	PIMAGEHLP_SYMBOL  sym = (PIMAGEHLP_SYMBOL) GlobalAlloc (GMEM_FIXED, symSize);
	::ZeroMemory (sym, symSize);
	sym->SizeOfStruct = symSize;
	sym->MaxNameLength = symSize - sizeof(IMAGEHLP_SYMBOL);

	DWORD disp = 0;
	if (SymGetSymFromAddr (GetCurrentProcess(), funcAddr, &disp, sym) == FALSE)
	{
		return str;
	}

	CHAR undecSymbol[stringSize];
	if (UnDecorateSymbolName (sym->Name, undecSymbol, stringSize, UNDNAME_COMPLETE | UNDNAME_NO_THISTYPE | UNDNAME_NO_SPECIAL_SYMS | UNDNAME_NO_MEMBER_TYPE | UNDNAME_NO_MS_KEYWORDS | UNDNAME_NO_ACCESS_SPECIFIERS ) > 0)
	{
		str = undecSymbol;
	}
	else if (SymUnDName (sym, undecSymbol, stringSize) == TRUE)
	{
		str = undecSymbol;
	}

	if (disp != 0)
	{
		str += " + ";
		str += toStringA ((uint32)disp);
		str += " bytes";
	}

	// replace param with the value of the stack for this param

	string parse = str;
	str = "";
	uint pos = 0;
	sint stop = 0;

	for (uint i = 0; i < parse.size (); i++)
	{
		if (parse[i] == '<')
			 stop++;
		if (parse[i] == '>')
			 stop--;

		if (stop==0 && (parse[i] == ',' || parse[i] == ')'))
		{
			char tmp[32];
			sprintf (tmp, "=0x%X", *((ULONG*)(stackAddr) + 2 + pos++));
			str += tmp;
		}
		str += parse[i];
	}
	GlobalFree (sym);

	return str;
}

static string getSourceInfo (DWORD addr)
{
	string str;

	IMAGEHLP_LINE  line;
	::ZeroMemory (&line, sizeof (line));
	line.SizeOfStruct = sizeof(line);

// It doesn't work in windows 98
/*	DWORD disp;
	if (SymGetLineFromAddr (GetCurrentProcess(), addr, &disp, &line))
	{
		str = line.FileName;
		str += "(" + toString (line.LineNumber) + ")";
	}
	else
*/	{
		IMAGEHLP_MODULE module;
		::ZeroMemory (&module, sizeof(module));
		module.SizeOfStruct = sizeof(module);

		if (SymGetModuleInfo (GetCurrentProcess(), addr, &module))
		{
			str = module.ModuleName;
		}
		else
		{
			str = "<NoModule>";
		}
		char tmp[32];
		sprintf (tmp, "!0x%X", addr);
		str += tmp;
	}
	
	return str;
}

static DWORD __stdcall GetModuleBase(HANDLE hProcess, DWORD dwReturnAddress)
{
	IMAGEHLP_MODULE moduleInfo;

	if (SymGetModuleInfo(hProcess, dwReturnAddress, &moduleInfo))
		return moduleInfo.BaseOfImage;
	else
	{
		MEMORY_BASIC_INFORMATION memoryBasicInfo;

		if (::VirtualQueryEx(hProcess, (LPVOID) dwReturnAddress,
			&memoryBasicInfo, sizeof(memoryBasicInfo)))
		{
			DWORD cch = 0;
			char szFile[MAX_PATH] = { 0 };

		 cch = GetModuleFileNameA((HINSTANCE)memoryBasicInfo.AllocationBase,
								 szFile, MAX_PATH);

		if (cch && (lstrcmpA(szFile, "DBFN")== 0))
		{
			 if (!SymLoadModule(hProcess,
				   NULL, "MN",
				   NULL, (DWORD) memoryBasicInfo.AllocationBase, 0))
				{
					DWORD dwError = GetLastError();
//					sipinfo("Error: %d", dwError);
				}
		}
		else
		{
		 if (!SymLoadModule(hProcess,
			   NULL, ((cch) ? szFile : NULL),
			   NULL, (DWORD) memoryBasicInfo.AllocationBase, 0))
			{
				DWORD dwError = GetLastError();
//				sipinfo("Error: %d", dwError);
			 }

		}

		 return (DWORD) memoryBasicInfo.AllocationBase;
	  }
//		else
//			sipinfo("Error is %d", GetLastError());
	}

	return 0;
}

static void displayCallStack (CLogA *log)
{
	static string symbolPath;

	DWORD symOptions = SymGetOptions();
	symOptions |= SYMOPT_LOAD_LINES; 
	symOptions &= ~SYMOPT_UNDNAME;
	SymSetOptions (symOptions);

	//
	// Create the path where to find the symbol
	//

	if (symbolPath.empty())
	{
		char tmpPath[stringSize];

		symbolPath = ".";

		if (GetEnvironmentVariableA ("_NT_SYMBOL_PATH", tmpPath, stringSize))
		{
			symbolPath += ";";
			symbolPath += tmpPath;
		}

		if (GetEnvironmentVariableA ("_NT_ALTERNATE_SYMBOL_PATH", tmpPath, stringSize))
		{
			symbolPath += ";";
			symbolPath += tmpPath;
		}

		if (GetEnvironmentVariableA ("SYSTEMROOT", tmpPath, stringSize))
		{
			symbolPath += ";";
			symbolPath += tmpPath;
			symbolPath += ";";
			symbolPath += tmpPath;
			symbolPath += "\\system32";
		}
	}

	//
	// Initialize
	//

	if (SymInitialize (GetCurrentProcess(), NULL, FALSE) == FALSE)
	{
		sipwarning ("DISP: SymInitialize(%p, '%s') failed", GetCurrentProcess(), symbolPath.c_str());
		return;
	}

	CONTEXT context;
	::ZeroMemory (&context, sizeof(context));
	context.ContextFlags = CONTEXT_FULL;

	if (GetThreadContext (GetCurrentThread(), &context) == FALSE)
	{
		sipwarning ("DISP: GetThreadContext(%p) failed", GetCurrentThread());
		return;
	}
	
	STACKFRAME callStack;
	::ZeroMemory (&callStack, sizeof(callStack));
	callStack.AddrPC.Mode      = AddrModeFlat;
	callStack.AddrPC.Offset    = context.Eip;
	callStack.AddrStack.Mode   = AddrModeFlat;
	callStack.AddrStack.Offset = context.Esp;
	callStack.AddrFrame.Mode   = AddrModeFlat;
	callStack.AddrFrame.Offset = context.Ebp;

	for (uint32 i = 0; ; i++)
	{
		BOOL res = StackWalk (IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &callStack,
			NULL, NULL, SymFunctionTableAccess, GetModuleBase, NULL);

/*		if (res == FALSE)
		{
			DWORD r = GetLastError ();
			sipinfo ("%d",r);
		}
*/
		if (i == 0)
		   continue;

		if (res == FALSE || callStack.AddrFrame.Offset == 0)
			break;
	
		string symInfo, srcInfo;

		symInfo = getFuncInfo (callStack.AddrPC.Offset, callStack.AddrFrame.Offset);
		srcInfo = getSourceInfo (callStack.AddrPC.Offset);

		log->displayNL ("   %s : %s", srcInfo.c_str(), symInfo.c_str());
	}
}

static void displayCallStackW (CLogW *log)
{
	static basic_string<ucchar> symbolPath;

	DWORD symOptions = SymGetOptions();
	symOptions |= SYMOPT_LOAD_LINES; 
	symOptions &= ~SYMOPT_UNDNAME;
	SymSetOptions (symOptions);

	//
	// Create the path where to find the symbol
	//

	if (symbolPath.empty())
	{
		ucchar tmpPath[stringSize];

		symbolPath = L".";

		if (GetEnvironmentVariableW (L"_NT_SYMBOL_PATH", tmpPath, stringSize))
		{
			symbolPath += L";";
			symbolPath += tmpPath;
		}

		if (GetEnvironmentVariableW (L"_NT_ALTERNATE_SYMBOL_PATH", tmpPath, stringSize))
		{
			symbolPath += L";";
			symbolPath += tmpPath;
		}

		if (GetEnvironmentVariableW (L"SYSTEMROOT", tmpPath, stringSize))
		{
			symbolPath += L";";
			symbolPath += tmpPath;
			symbolPath += L";";
			symbolPath += tmpPath;
			symbolPath += L"\\system32";
		}
	}

	//
	// Initialize
	//

	if (SymInitialize (GetCurrentProcess(), NULL, FALSE) == FALSE)
	{
		sipwarning ("DISP: SymInitialize(%p, '%s') failed", GetCurrentProcess(), symbolPath.c_str());
		return;
	}

	CONTEXT context;
	::ZeroMemory (&context, sizeof(context));
	context.ContextFlags = CONTEXT_FULL;

	if (GetThreadContext (GetCurrentThread(), &context) == FALSE)
	{
		sipwarning ("DISP: GetThreadContext(%p) failed", GetCurrentThread());
		return;
	}
	
	STACKFRAME callStack;
	::ZeroMemory (&callStack, sizeof(callStack));
	callStack.AddrPC.Mode      = AddrModeFlat;
	callStack.AddrPC.Offset    = context.Eip;
	callStack.AddrStack.Mode   = AddrModeFlat;
	callStack.AddrStack.Offset = context.Esp;
	callStack.AddrFrame.Mode   = AddrModeFlat;
	callStack.AddrFrame.Offset = context.Ebp;

	for (uint32 i = 0; ; i++)
	{
		BOOL res = StackWalk (IMAGE_FILE_MACHINE_I386, GetCurrentProcess(), GetCurrentThread(), &callStack,
			NULL, NULL, SymFunctionTableAccess, GetModuleBase, NULL);

/*		if (res == FALSE)
		{
			DWORD r = GetLastError ();
			sipinfo ("%d",r);
		}
*/
		if (i == 0)
		   continue;

		if (res == FALSE || callStack.AddrFrame.Offset == 0)
			break;
	
		string symInfo, srcInfo;

		symInfo = getFuncInfo (callStack.AddrPC.Offset, callStack.AddrFrame.Offset);
		srcInfo = getSourceInfo (callStack.AddrPC.Offset);

		log->displayNL (L"   %s : %s", srcInfo.c_str(), symInfo.c_str());
	}
}

#else // SIP_OS_WINDOWS

static void displayCallStack (CLogA *log)
{
	log->displayNL ("no call stack info available");
}

static void displayCallStackW (CLogW *log)
{
	log->displayNL (L"no call stack info available");
}

#endif // SIP_OS_WINDOWS


/*
 * Constructor
 */
CMemDisplayerA::CMemDisplayerA (const char *displayerName) : IDisplayerA (displayerName), _NeedHeader(true), _MaxStrings(50), _CanUseStrings(true)
{
	setParam (50);
}

void CMemDisplayerA::setParam (uint32 maxStrings)
{
	_MaxStrings = maxStrings;
}


// Log format: "2000/01/15 12:05:30 <ProcessName> <LogType> <ThreadId> <FileName> <Line> : <Msg>"
void CMemDisplayerA::doDisplay ( const CLogA::TDisplayInfo& args, const char *message )
{
//	stringstream	ss;
	string str;
	bool			needSpace = false;

	if (!_CanUseStrings) return;

	if (_NeedHeader)
	{
		str += HeaderString();
		_NeedHeader = false;
	}

	if (args.Date != 0)
	{
		str += dateToHumanString(args.Date);
		needSpace = true;
	}

	if (!args.ProcessName.empty())
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += args.ProcessName;
		needSpace = true;
	}

	if (args.LogType != ILog::LOG_NO)
	{
		if (needSpace) { str += " "; needSpace = false; }
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
		if (needSpace) { str += " "; needSpace = false; }
		str += CFileA::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += SIPBASE::toStringA(args.Line);
		needSpace = true;
	}
	
	if (needSpace) { str += " : "; needSpace = false; }

	str += message;

	// clear old line
	while (_Strings.size () > _MaxStrings)
	{
		_Strings.pop_front ();
	}

	_Strings.push_back (str);
}

void CMemDisplayerA::write (CLogA *log, bool quiet)
{
	if (log == NULL)
		log = (CLogA *)((CLog *)InfoLog);

	if ( ! quiet )
	{
		log->forceDisplayRaw ("------------------------------------------------------------------------------\n");
		log->forceDisplayRaw ("----------------------------------------- display MemDisplayer history -------\n");
		log->forceDisplayRaw ("------------------------------------------------------------------------------\n");
	}
	for (deque<string>::iterator it = _Strings.begin(); it != _Strings.end(); it++)
	{
		log->forceDisplayRaw ((*it).c_str());
	}
	if ( ! quiet )
	{
		log->forceDisplayRaw ("------------------------------------------------------------------------------\n");
		log->forceDisplayRaw ("----------------------------------------- display MemDisplayer callstack -----\n");
		log->forceDisplayRaw ("------------------------------------------------------------------------------\n");
		displayCallStack(log);
		log->forceDisplayRaw ("------------------------------------------------------------------------------\n");
		log->forceDisplayRaw ("----------------------------------------- end of MemDisplayer display --------\n");
		log->forceDisplayRaw ("------------------------------------------------------------------------------\n");
	}
}

void CMemDisplayerA::write (string &str, bool crLf)
{
	for (deque<string>::iterator it = _Strings.begin(); it != _Strings.end(); it++)
	{
		str += (*it);
		if ( crLf )
		{
			if ( (!str.empty()) && (str[str.size()-1] == '\n') )
			{
				str[str.size()-1] = '\r';
				str += '\n';
			}
		}
	}
}

/*
 * Constructor
 */
CMemDisplayerW::CMemDisplayerW (const char *displayerName) : IDisplayerW (displayerName), _NeedHeader(true), _MaxStrings(50), _CanUseStrings(true)
{
	setParam (50);
}

CMemDisplayerW::~CMemDisplayerW()
{
	_Strings.clear();
}

void CMemDisplayerW::setParam (uint32 maxStrings)
{
	_MaxStrings = maxStrings;
}


// Log format: "2000/01/15 12:05:30 <ProcessName> <LogType> <ThreadId> <FileName> <Line> : <Msg>"
void CMemDisplayerW::doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message )
{
	std::basic_string<ucchar> str;
	bool			needSpace = false;

	if (!_CanUseStrings) return;

	if (_NeedHeader)
	{
		str += HeaderString();
		_NeedHeader = false;
	}

	if (args.Date != 0)
	{
		str += dateToHumanString(args.Date);
		needSpace = true;
	}

	if (!args.ProcessName.empty())
	{
		if (needSpace) { str += L" "; needSpace = false; }
		str += args.ProcessName.c_str(); // KSR_ADD 2010_1_30
		needSpace = true;
	}

	if (args.LogType != ILog::LOG_NO)
	{
		if (needSpace) { str += L" "; needSpace = false; }
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
		if (needSpace) { str += L" "; needSpace = false; }
		str += CFileW::getFilename(args.FileName);
		needSpace = true;
	}

	if (args.Line != -1)
	{
		if (needSpace) { str += L" "; needSpace = false; }
		#ifdef SIP_OS_WINDOWS
			str += SIPBASE::toString(L"%hu", args.Line);
		#else
			str += SIPBASE::toStringLW("%hu", args.Line);
		#endif
		
		needSpace = true;
	}
	
	if (needSpace) { str += L" : "; needSpace = false; }

	str += message;

	// clear old line
	while (_Strings.size () > _MaxStrings)
	{
		_Strings.pop_front ();
	}

	_Strings.push_back (str);
}

void CMemDisplayerW::write (CLogW *log, bool quiet)
{
	if (log == NULL)
		log = (CLogW *)((CLog *)InfoLog);

	if ( ! quiet )
	{
		log->forceDisplayRaw (L"------------------------------------------------------------------------------\n");
		log->forceDisplayRaw (L"----------------------------------------- display MemDisplayer history -------\n");
		log->forceDisplayRaw (L"------------------------------------------------------------------------------\n");
	}
	for (deque<basic_string<ucchar> >::iterator it = _Strings.begin(); it != _Strings.end(); it++)
	{
		log->forceDisplayRaw ((*it).c_str());
	}
	if ( ! quiet )
	{
		log->forceDisplayRaw (L"------------------------------------------------------------------------------\n");
		log->forceDisplayRaw (L"----------------------------------------- display MemDisplayer callstack -----\n");
		log->forceDisplayRaw (L"------------------------------------------------------------------------------\n");
		displayCallStackW(log);
		log->forceDisplayRaw (L"------------------------------------------------------------------------------\n");
		log->forceDisplayRaw (L"----------------------------------------- end of MemDisplayer display --------\n");
		log->forceDisplayRaw (L"------------------------------------------------------------------------------\n");
	}
}

void CMemDisplayerW::write (basic_string<ucchar> &str, bool crLf)
{
	for (deque<basic_string<ucchar> >::iterator it = _Strings.begin(); it != _Strings.end(); it++)
	{
		str += (*it);
		if ( crLf )
		{
			if ( (!str.empty()) && (str[str.size()-1] == L'\n') )
			{
				str[str.size()-1] = L'\r';
				str += L'\n';
			}
		}
	}
}

void CMemDisplayerW::write (string &str, bool crLf)
{
	for (deque<basic_string<ucchar> >::iterator it = _Strings.begin(); it != _Strings.end(); it++)
	{
		str += toString("%S", (*it).c_str());;
		if ( crLf )
		{
			if ( (!str.empty()) && (str[str.size()-1] == '\n') )
			{
				str[str.size()-1] = '\r';
				str += '\n';
			}
		}
	}
}

void	CLightMemDisplayerA::doDisplay ( const CLogA::TDisplayInfo& args, const char *message )
{
	//stringstream	ss;
	string str;
	//bool			needSpace = false;
	
	if (!_CanUseStrings) return;
	
	str += message;
	
	// clear old line
	while (_Strings.size () >= _MaxStrings)
	{
		_Strings.pop_front ();
	}
	
	_Strings.push_back (str);
}

void	CLightMemDisplayerW::doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message )
{
	basic_string<ucchar> str;
	
	if (!_CanUseStrings) return;
	
	str += message;
	
	// clear old line
	while (_Strings.size () >= _MaxStrings)
	{
		_Strings.pop_front ();
	}
	
	_Strings.push_back (str);
}

} // SIPBASE
