/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#ifdef SIP_OS_WINDOWS
#include <process.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <ctime>
#include <cstdarg>

#include "misc/displayer.h"
#include "misc/log.h"
#include "misc/debug.h"
#include "misc/path.h"

using namespace std;

namespace SIPBASE
{

ILog::ILog( TLogType logType) :
	_LogType (logType), _FileName(NULL), _Line(-1), _FuncName(NULL), _Mutex("LOG" + toStringA((uint)logType)), _PosSet(0)
{
}

void ILog::addDisplayer (IDisplayer_base *displayer, bool bypassFilter)
{
	if (displayer == NULL)
	{
		return;
	}

	if (bypassFilter)
	{
		CDisplayers::iterator idi = std::find (_BypassFilterDisplayers.begin (), _BypassFilterDisplayers.end (), displayer);
		if (idi == _BypassFilterDisplayers.end ())
		{
			_BypassFilterDisplayers.push_back (displayer);
		}
	}
	else
	{
		CDisplayers::iterator idi = std::find (_Displayers.begin (), _Displayers.end (), displayer);
		if (idi == _Displayers.end ())
		{
			_Displayers.push_back (displayer);
		}
	}
}

IDisplayer_base *ILog::getDisplayer (const char *displayerName)
{
	if (displayerName == NULL || displayerName[0] == '\0')
	{
		return NULL;
	}
		
	CDisplayers::iterator idi;
	for (idi = _Displayers.begin (); idi != _Displayers.end (); idi++)
	{
		if ((*idi)->DisplayerName == displayerName)
		{
			return *idi;
		}
	}
	for (idi = _BypassFilterDisplayers.begin (); idi != _BypassFilterDisplayers.end (); idi++)
	{
		if ((*idi)->DisplayerName == displayerName)
		{
			return *idi;
		}
	}
	return NULL;
}

void ILog::removeDisplayer (IDisplayer_base *displayer)
{
	if (displayer == NULL)
	{
		return;
	}

	CDisplayers::iterator idi = std::find (_Displayers.begin (), _Displayers.end (), displayer);
	if (idi != _Displayers.end ())
	{
		_Displayers.erase (idi);
	}

	idi = std::find (_BypassFilterDisplayers.begin (), _BypassFilterDisplayers.end (), displayer);
	if (idi != _BypassFilterDisplayers.end ())
	{
		_BypassFilterDisplayers.erase (idi);
	}
}

void ILog::removeDisplayer (const char *displayerName)
{
	if (displayerName == NULL || displayerName[0] == '\0')
	{
		return;
	}
	
	CDisplayers::iterator idi;
	for (idi = _Displayers.begin (); idi != _Displayers.end ();)
	{
		if ((*idi)->DisplayerName == displayerName)
		{
			idi = _Displayers.erase (idi);
		}
		else
		{
			idi++;
		}
	}

	for (idi = _BypassFilterDisplayers.begin (); idi != _BypassFilterDisplayers.end ();)
	{
		if ((*idi)->DisplayerName == displayerName)
		{
			idi = _BypassFilterDisplayers.erase (idi);
		}
		else
		{
			idi++;
		}
	}
}

/*
 * Returns true if the specified displayer is attached to the log object
 */
bool ILog::attached(IDisplayer_base *displayer) const 
{
	return (find( _Displayers.begin(), _Displayers.end(), displayer ) != _Displayers.end()) ||
			(find( _BypassFilterDisplayers.begin(), _BypassFilterDisplayers.end(), displayer ) != _BypassFilterDisplayers.end());
}

void ILog::setPosition (sint line, const char *fileName, const char *funcName)
{
	if ( !noDisplayer() )
	{
		_Mutex.enter();
		_PosSet ++;
		_FileName = fileName;
	    _Line = line;
		_FuncName = funcName;
	}
}

/// Symetric to setPosition(). Automatically called by display...(). Do not call if noDisplayer().
void ILog::unsetPosition()
{
	if (_PosSet > 0)
	{
		_FileName = NULL;
		_Line = -1;
		_FuncName = NULL;
		_PosSet --;
		_Mutex.leave(); // needs setPosition() to have been called
	}
}

std::basic_string<char> CLogA::_ProcessName;

void CLogA::setDefaultProcessName ()
{
#ifdef SIP_OS_WINDOWS
	if (_ProcessName.empty())
	{
		char name[1024];
		GetModuleFileNameA (NULL, name, 1023);
		_ProcessName = CFileA::getFilename(name);
	}
#else
	if (_ProcessName.empty())
	{
		_ProcessName = "<Unknown>";
	}
#endif
}

void CLogA::setProcessName (const std::basic_string<char> &processName)
{
	_ProcessName = processName;
}

void CLogA::displayString (const char *str)
{
	const char *disp = NULL;
	TDisplayInfo localargs, *args = NULL;

	setDefaultProcessName ();

	if (strchr(str, '\n') == NULL)
	{
		if (TempString.empty())
		{
			time (&TempArgs.Date);
			TempArgs.LogType = _LogType;
			TempArgs.ProcessName = _ProcessName;
			TempArgs.ThreadId = getThreadId();
			TempArgs.FileName = _FileName;
			TempArgs.Line = _Line;
			TempArgs.FuncName = _FuncName;
			TempArgs.CallstackAndLog = "";

			TempString = str;
		}
		else
		{
			TempString += str;
		}
		unsetPosition(); //add by LCI 091017
		return;
	}
	else
	{
		if (TempString.empty())
		{
			time (&localargs.Date);
			localargs.LogType = _LogType;
			localargs.ProcessName = _ProcessName;
			localargs.ThreadId = getThreadId();
			localargs.FileName = _FileName;
			localargs.Line = _Line;
			localargs.FuncName = _FuncName;
			localargs.CallstackAndLog = "";

			disp = str;
			args = &localargs;
		}
		else
		{
			TempString += str;
			disp = TempString.c_str();
			args = &TempArgs;
		}
	}

	// send to all bypass filter displayers
	for (CDisplayers::iterator idi=_BypassFilterDisplayers.begin(); idi!=_BypassFilterDisplayers.end(); idi++ )
	{
		(*idi)->display( *args, disp );
	}

	// get the log at the last minute to be sure to have everything
	if(args->LogType == LOG_ERROR || args->LogType == LOG_ASSERT)
	{
		getCallStackAndLogA (args->CallstackAndLog, 4);
	}

	if (passFilter (disp))
	{
		// Send to the attached displayers
		for (CDisplayers::iterator idi=_Displayers.begin(); idi!=_Displayers.end(); idi++ )
		{
			(*idi)->display( *args, disp );
		}
	}
	
	TempString = "";
	unsetPosition();
}

/*
 * Display the string with decoration and final new line to all attached displayers
 */
#ifdef SIP_OS_WINDOWS
void CLogA::_displayNL (const char *format, ...)
#else
void CLogA::displayNL (const char *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;	
	SIPBASE_CONVERT_VARGS (str, format, 1256);

	if (strlen(str) < 1256 - 1)
		strcat (str, "\n");
	else
		str[1256 - 2] = '\n';

	displayString (str);
}

/*
 * Display the string with decoration to all attached displayers
 */
#ifdef SIP_OS_WINDOWS
void CLogA::_display (const char *format, ...)
#else
void CLogA::display (const char *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;
	SIPBASE_CONVERT_VARGS (str, format, 1256);
	
	displayString (str);
}

void CLogA::displayRawString (const char *str)
{
	const char *disp = NULL;
	TDisplayInfo localargs, *args = NULL;

	setDefaultProcessName ();

	if (strchr(str, '\n') == NULL)
	{
		if (TempString.empty())
		{
			localargs.Date = 0;
			localargs.LogType = LOG_NO;
			localargs.ProcessName = "";
			localargs.CallstackAndLog = "";
			localargs.ThreadId = 0;
			localargs.FileName = NULL;
			localargs.Line = -1;

			TempString = str;
		}
		else
		{
			TempString += str;
		}
		unsetPosition(); //add by LCI 091017
		return;
	}
	else
	{
		if (TempString.empty())
		{
			localargs.Date = 0;
			localargs.LogType = LOG_NO;
			localargs.ProcessName = "";
			localargs.CallstackAndLog = "";
			localargs.ThreadId = 0;
			localargs.FileName = NULL;
			localargs.Line = -1;
			
			disp = str;
			args = &localargs;
		}
		else
		{
			TempString += str;
			disp = TempString.c_str();
			args = &TempArgs;
		}
	}

	// send to all bypass filter displayers
	for (CDisplayers::iterator idi=_BypassFilterDisplayers.begin(); idi!=_BypassFilterDisplayers.end(); idi++ )
	{
		(*idi)->display( *args, disp );
	}

	// get the log at the last minute to be sure to have everything
	if(args->LogType == LOG_ERROR || args->LogType == LOG_ASSERT)
	{
		getCallStackAndLogA (args->CallstackAndLog, 4);
	}

	if ( passFilter( disp ) )
	{
		// Send to the attached displayers
		for ( CDisplayers::iterator idi=_Displayers.begin(); idi!=_Displayers.end(); idi++ )
		{
			(*idi)->display( *args, disp );
		}
	}

	TempString = "";
	unsetPosition();
}

/*
 * Display a string (and nothing more) to all attached displayers
 */
#ifdef SIP_OS_WINDOWS
void CLogA::_displayRawNL( const char *format, ... )
#else
void CLogA::displayRawNL( const char *format, ... )
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;

	SIPBASE_CONVERT_VARGS (str, format, 1256);

	if (strlen(str) < 1256 - 1)
		strcat (str, "\n");
	else
		str[1256 - 2] = '\n';
	
	displayRawString(str);
}

/*
 * Display a string (and nothing more) to all attached displayers
 */
#ifdef SIP_OS_WINDOWS
void CLogA::_displayRaw( const char *format, ... )
#else
void CLogA::displayRaw( const char *format, ... )
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;
	SIPBASE_CONVERT_VARGS (str, format, 1256);
	
	displayRawString(str);
}

#ifdef SIP_OS_WINDOWS
void CLogA::_forceDisplayRaw (const char *format, ...)
#else
void CLogA::forceDisplayRaw (const char *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;
	SIPBASE_CONVERT_VARGS (str, format, 1256);
	
	TDisplayInfo args;
	CDisplayers::iterator idi;

	// send to all bypass filter displayers
	for (idi=_BypassFilterDisplayers.begin(); idi!=_BypassFilterDisplayers.end(); idi++ )
	{
		(*idi)->display( args, str );
	}

	// Send to the attached displayers
	for ( idi=_Displayers.begin(); idi!=_Displayers.end(); idi++ )
	{
		(*idi)->display( args, str );
	}
}

/*
 * Returns true if the string must be logged, according to the current filter
 */
bool CLogA::passFilter( const char *filter )
{
	bool yes = _PositiveFilter.empty();

	bool found;
	list<std::basic_string<char> >::iterator ilf;

	// 1. Positive filter
	for ( ilf=_PositiveFilter.begin(); ilf!=_PositiveFilter.end(); ++ilf )
	{
		found = ( strstr( filter, (*ilf).c_str() ) != NULL );
		
		if ( found )
		{
			yes = true; // positive filter passed (no need to check another one)
			break;
		}
		// else try the next one
	}
	if ( ! yes )
	{
		return false; // positive filter not passed
	}

	// 2. Negative filter
	for ( ilf=_NegativeFilter.begin(); ilf!=_NegativeFilter.end(); ++ilf )
	{
		found = ( strstr( filter, (*ilf).c_str() ) != NULL );
		
		if ( found )
		{
			return false; // negative filter not passed (no need to check another one)
		}
	}
	return true; // negative filter passed
}


/*
 * Removes a filter by name. Returns true if it was found.
 */
void CLogA::removeFilter( const char *filterstr )
{
	if (filterstr == NULL)
	{
		_PositiveFilter.clear();
		_NegativeFilter.clear();
	}
	else
	{
		_PositiveFilter.remove( filterstr );
		_NegativeFilter.remove( filterstr );
	}
}

void CLogA::displayFilter( CLogA &log )
{
	std::list<std::basic_string<char> >::iterator it;

	log.displayNL ("Positive Filter(s):");
	for (it = _PositiveFilter.begin (); it != _PositiveFilter.end (); it++)
	{
		log.displayNL ("'%s'", (*it).c_str());
	}
	log.displayNL ("Negative Filter(s):");
	for (it = _NegativeFilter.begin (); it != _NegativeFilter.end (); it++)
	{
		log.displayNL ("'%s'", (*it).c_str());
	}
}

void CLogA::addPositiveFilter( const char *filterstr )
{
	_PositiveFilter.push_back( filterstr );
}

void CLogA::addNegativeFilter( const char *filterstr )
{
	_NegativeFilter.push_back( filterstr );
}

void CLogA::resetFilters()
{
	_PositiveFilter.clear();
	_NegativeFilter.clear();
}

// unicode version
std::basic_string<ucchar> CLogW::_ProcessName;

void CLogW::setDefaultProcessName ()
{
#ifdef SIP_OS_WINDOWS
	if (_ProcessName.empty())
	{
		ucchar name[1024];
		GetModuleFileNameW (NULL, name, 1023);
		_ProcessName = SIPBASE::toString(L"%s", CFileW::getFilename(name).c_str());
	}
#else
	if (_ProcessName.empty())
	{
		_ProcessName = L"<Unknown>";
	}
#endif
}

void CLogW::setProcessName (const std::basic_string<char> &processName)
{
	setProcessNameW (SIPBASE::getUcsFromMbs(processName.c_str()));
}

void CLogW::setProcessNameW (const std::basic_string<ucchar> &processName)
{
	_ProcessName = processName;
}

void CLogW::displayString (const ucchar *str)
{
	const ucchar *disp = NULL;
	TDisplayInfo localargs, *args = NULL;

	setDefaultProcessName ();

	if (wcschr(str, L'\n') == NULL)
	{
		if (TempString.empty())
		{
			time (&TempArgs.Date);
			TempArgs.LogType = _LogType;
			TempArgs.ProcessName = _ProcessName;
			TempArgs.ThreadId = getThreadId();
			if (_FileName != NULL)
			{
				TempArgs.FileName = SIPBASE::getUcsFromMbs(_FileName);
			}
			else
				TempArgs.FileName = L"";
			TempArgs.Line = _Line;
			if (_FuncName != NULL)
			{
				TempArgs.FuncName = SIPBASE::getUcsFromMbs(_FuncName);
			}
			else
				TempArgs.FuncName = L"";
			TempArgs.CallstackAndLog = L"";

			TempString = str;
		}
		else
		{
			TempString += str;
		}
		unsetPosition(); //add by LCI 091017
		return;
	}
	else
	{
		if (TempString.empty())
		{
			time (&localargs.Date);
			localargs.LogType = _LogType;
			localargs.ProcessName = _ProcessName;
			localargs.ThreadId = getThreadId();
			if (_FileName != NULL)
				localargs.FileName = SIPBASE::getUcsFromMbs(_FileName);
			else
				localargs.FileName = L"";
			localargs.Line = _Line;
			if (_FuncName != NULL)
			{
				localargs.FuncName = SIPBASE::getUcsFromMbs(_FuncName);
			}
			else
				localargs.FuncName = L"";
			localargs.CallstackAndLog = L"";

			disp = str;
			args = &localargs;
		}
		else
		{
			TempString += str;
			disp = TempString.c_str();
			args = &TempArgs;
		}
	}

	// send to all bypass filter displayers
	for (CDisplayers::iterator idi=_BypassFilterDisplayers.begin(); idi!=_BypassFilterDisplayers.end(); idi++ )
	{
		(*idi)->display( *args, disp );
	}

	// get the log at the last minute to be sure to have everything
	if(args->LogType == LOG_ERROR || args->LogType == LOG_ASSERT)
	{
		getCallStackAndLogW (args->CallstackAndLog, 4);
	}

	if (passFilter (disp))
	{
		// Send to the attached displayers
		for (CDisplayers::iterator idi=_Displayers.begin(); idi!=_Displayers.end(); idi++ )
		{
			(*idi)->display( *args, disp );
		}
	}
	
	TempString = L"";
	unsetPosition();
}

/*
 * Display the string with decoration and final new line to all attached displayers
 */
#ifdef SIP_OS_WINDOWS
void CLogW::_displayNL (const ucchar *format, ...)
#else
void CLogW::displayNL (const ucchar *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	ucchar *str;	
	SIPBASE_CONVERT_VARGS_WIDE (str, format, 1256);

	if (wcslen(str) <= 1254)
		wcscat (str, L"\n");
	else
		str[1254] = L'\n';

	displayString (str);
}

#ifdef SIP_OS_WINDOWS
void CLogW::_displayNL (const char *format, ...)
#else
void CLogW::displayNL (const char *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;
	SIPBASE_CONVERT_VARGS (str, format, 1256);

	if (strlen(str) <= 1254)
		strcat (str, "\n");
	else
		str[1254] = '\n';

	basic_string<ucchar> ucstr = getUcsFromMbs(str);
	displayString (ucstr.c_str());
}

/*
 * Display the string with decoration to all attached displayers
 */
#ifdef SIP_OS_WINDOWS
void CLogW::_display (const ucchar *format, ...)
#else
void CLogW::display (const ucchar *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	ucchar *str;
	SIPBASE_CONVERT_VARGS_WIDE (str, format, 1256);
	
	displayString (str);
}

#ifdef SIP_OS_WINDOWS
void CLogW::_display (const char *format, ...)
#else
void CLogW::display (const char *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;
	SIPBASE_CONVERT_VARGS (str, format, 1256);

	basic_string<ucchar> ucstr = getUcsFromMbs(str);
	displayString (ucstr.c_str());
}

void CLogW::displayRawString (const ucchar *str)
{
	const ucchar *disp = NULL;
	TDisplayInfo localargs, *args = NULL;

	setDefaultProcessName ();

	if (wcschr(str, L'\n') == NULL)
	{
		if (TempString.empty())
		{
			localargs.Date = 0;
			localargs.LogType = LOG_NO;
			localargs.ProcessName = L"";
			localargs.CallstackAndLog = L"";
			localargs.ThreadId = 0;
			localargs.FileName = L"";
			localargs.Line = -1;

			TempString = str;
		}
		else
		{
			TempString += str;
		}
		unsetPosition(); //add by LCI 091017
		return;
	}
	else
	{
		if (TempString.empty())
		{
			localargs.Date = 0;
			localargs.LogType = LOG_NO;
			localargs.ProcessName = L"";
			localargs.CallstackAndLog = L"";
			localargs.ThreadId = 0;
			localargs.FileName = L"";
			localargs.Line = -1;
			
			disp = str;
			args = &localargs;
		}
		else
		{
			TempString += str;
			disp = TempString.c_str();
			args = &TempArgs;
		}
	}

	// send to all bypass filter displayers
	for (CDisplayers::iterator idi=_BypassFilterDisplayers.begin(); idi!=_BypassFilterDisplayers.end(); idi++ )
	{
		(*idi)->display( *args, disp );
	}

	// get the log at the last minute to be sure to have everything
	if(args->LogType == LOG_ERROR || args->LogType == LOG_ASSERT)
	{
		getCallStackAndLogW (args->CallstackAndLog, 4);
	}

	if ( passFilter( disp ) )
	{
		// Send to the attached displayers
		for ( CDisplayers::iterator idi=_Displayers.begin(); idi!=_Displayers.end(); idi++ )
		{
			(*idi)->display( *args, disp );
		}
	}

	TempString = L"";
	unsetPosition();
}

/*
 * Display a string (and nothing more) to all attached displayers
 */
#ifdef SIP_OS_WINDOWS
void CLogW::_displayRawNL( const ucchar *format, ... )
#else
void CLogW::displayRawNL( const ucchar *format, ... )
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	ucchar *str;
	SIPBASE_CONVERT_VARGS_WIDE (str, format, 1256);

	if (wcslen(str) <= 1254)
		wcscat (str, L"\n");
	else
		str[1254] = L'\n';

	displayRawString(str);
}

#ifdef SIP_OS_WINDOWS
void CLogW::_displayRawNL( const char *format, ... )
#else
void CLogW::displayRawNL( const char *format, ... )
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;	
	SIPBASE_CONVERT_VARGS (str, format, 1256);

	if (strlen(str) <= 1254)
		strcat (str, "\n");
	else
		str[1254] = '\n';

	basic_string<ucchar> ucstr = getUcsFromMbs(str);
	displayRawString (ucstr.c_str());
}

/*
 * Display a string (and nothing more) to all attached displayers
 */
#ifdef SIP_OS_WINDOWS
void CLogW::_displayRaw( const ucchar *format, ... )
#else
void CLogW::displayRaw( const ucchar *format, ... )
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	ucchar *str;
	SIPBASE_CONVERT_VARGS_WIDE (str, format, 1256);
	
	displayRawString(str);
}

#ifdef SIP_OS_WINDOWS
void CLogW::_displayRaw( const char *format, ... )
#else
void CLogW::displayRaw( const char *format, ... )
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;	
	SIPBASE_CONVERT_VARGS (str, format, 1256);

	basic_string<ucchar> ucstr = getUcsFromMbs(str);
	displayRawString (ucstr.c_str());
}

#ifdef SIP_OS_WINDOWS
void CLogW::_forceDisplayRaw (const ucchar *format, ...)
#else
void CLogW::forceDisplayRaw (const ucchar *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	ucchar *str;
	SIPBASE_CONVERT_VARGS_WIDE (str, format, 1256);
	
	TDisplayInfo args;
	CDisplayers::iterator idi;

	// send to all bypass filter displayers
	for (idi=_BypassFilterDisplayers.begin(); idi!=_BypassFilterDisplayers.end(); idi++ )
	{
		(*idi)->display( args, str );
	}

	// Send to the attached displayers
	for ( idi=_Displayers.begin(); idi!=_Displayers.end(); idi++ )
	{
		(*idi)->display( args, str );
	}
}

#ifdef SIP_OS_WINDOWS
void CLogW::_forceDisplayRaw (const char *format, ...)
#else
void CLogW::forceDisplayRaw (const char *format, ...)
#endif
{
	if ( noDisplayer() )
	{
		return;
	}

	char *str;	
	SIPBASE_CONVERT_VARGS (str, format, 1256);

	basic_string<ucchar> ucstr = getUcsFromMbs(str);

	TDisplayInfo args;
	CDisplayers::iterator idi;

	// send to all bypass filter displayers
	for (idi = _BypassFilterDisplayers.begin(); idi != _BypassFilterDisplayers.end(); idi++ )
	{
		(*idi)->display( args, ucstr.c_str() );
	}

	// Send to the attached displayers
	for ( idi=_Displayers.begin(); idi!=_Displayers.end(); idi++ )
	{
		(*idi)->display( args, ucstr.c_str() );
	}
}

/*
 * Returns true if the string must be logged, according to the current filter
 */
bool CLogW::passFilter( const ucchar *filter )
{
	bool yes = _PositiveFilter.empty();

	bool found;
	list<std::basic_string<ucchar> >::iterator ilf;

	// 1. Positive filter
	for ( ilf=_PositiveFilter.begin(); ilf!=_PositiveFilter.end(); ++ilf )
	{
		found = ( wcsstr( filter, (*ilf).c_str() ) != NULL );
		
		if ( found )
		{
			yes = true; // positive filter passed (no need to check another one)
			break;
		}
		// else try the next one
	}
	if ( ! yes )
	{
		return false; // positive filter not passed
	}

	// 2. Negative filter
	for ( ilf=_NegativeFilter.begin(); ilf!=_NegativeFilter.end(); ++ilf )
	{
		found = ( wcsstr( filter, (*ilf).c_str() ) != NULL );
		
		if ( found )
		{
			return false; // negative filter not passed (no need to check another one)
		}
	}
	return true; // negative filter passed
}

/*
 * Removes a filter by name. Returns true if it was found.
 */
void CLogW::removeFilter( const ucchar *filterstr )
{
	if (filterstr == NULL)
	{
		_PositiveFilter.clear();
		_NegativeFilter.clear();
	}
	else
	{
		_PositiveFilter.remove( filterstr );
		_NegativeFilter.remove( filterstr );
	}
}

void CLogW::removeFilter( const char *filterstr )
{
	if (filterstr == NULL)
	{
		_PositiveFilter.clear();
		_NegativeFilter.clear();
	}
	else
	{
		basic_string<ucchar> str = getUcsFromMbs(filterstr);
		_PositiveFilter.remove( str );
		_NegativeFilter.remove( str );
	}
}

void CLogW::displayFilter( CLogW &log )
{
	std::list<std::basic_string<ucchar> >::iterator it;

	log.displayNL (L"Positive Filter(s):");
	for (it = _PositiveFilter.begin (); it != _PositiveFilter.end (); it++)
	{
		log.displayNL (L"'%s'", (*it).c_str());
	}
	log.displayNL (L"Negative Filter(s):");
	for (it = _NegativeFilter.begin (); it != _NegativeFilter.end (); it++)
	{
		log.displayNL (L"'%s'", (*it).c_str());
	}
}

void CLogW::addPositiveFilter( const ucchar *filterstr )
{
	_PositiveFilter.push_back( filterstr );
}

void CLogW::addPositiveFilter( const char *filterstr )
{
	_PositiveFilter.push_back( getUcsFromMbs(filterstr) );
}

void CLogW::addNegativeFilter( const ucchar *filterstr )
{
	_NegativeFilter.push_back( filterstr );
}

void CLogW::addNegativeFilter( const char *filterstr )
{
	_NegativeFilter.push_back( getUcsFromMbs(filterstr) );
}

void CLogW::resetFilters()
{
	_PositiveFilter.clear();
	_NegativeFilter.clear();
}

} // SIPBASE
