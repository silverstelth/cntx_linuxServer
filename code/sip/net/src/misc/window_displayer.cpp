/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <signal.h> // <csignal>
#include "misc/path.h"
#include "misc/command.h"
#include "misc/thread.h"

#include "misc/window_displayer.h"

using namespace std;

namespace SIPBASE {

class CUpdateThreadA : public IRunnable
{
	CWindowDisplayerA *Disp;
	string WindowNameEx;
	sint X, Y, W, H, HS;
	bool Iconified;
	uint32 FS;
	string FN;
	bool WW;
	CLogA *Log;

public:
	CUpdateThreadA (CWindowDisplayerA *disp, string windowNameEx, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::string &fn, bool ww, CLogA *log) :
	  Disp(disp), WindowNameEx(windowNameEx), X(x), Y(y), W(w), H(h), HS(hs), Iconified(iconified), FS(fs), FN(fn), WW(ww), Log(log)
	{
	}

	void run()
	{
		Disp->open (WindowNameEx, Iconified, X, Y, W, H, HS, FS, FN, WW, Log);
		Disp->display_main ();
	}
};

class CUpdateThreadW : public IRunnable
{
	CWindowDisplayerW *Disp;
	basic_string<ucchar> WindowNameEx;
	sint X, Y, W, H, HS;
	bool Iconified;
	uint32 FS;
	basic_string<ucchar> FN;
	bool WW;
	CLogW *Log;

public:
	CUpdateThreadW (CWindowDisplayerW *disp, std::basic_string<ucchar> windowNameEx, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::basic_string<ucchar> &fn, bool ww, CLogW *log) :
	  Disp(disp), WindowNameEx(windowNameEx), X(x), Y(y), W(w), H(h), HS(hs), Iconified(iconified), FS(fs), FN(fn), WW(ww), Log(log)
	{
	}

	void run()
	{
		Disp->open (WindowNameEx, Iconified, X, Y, W, H, HS, FS, FN, WW, Log);
		Disp->display_main ();
	}
};

// ANSI version
CWindowDisplayerA::~CWindowDisplayerA ()
{
	// we have to wait the exit of the thread
	_Continue = false;
	sipassert (_Thread != NULL);
	_Thread->wait();
	delete _Thread;
	delete _Runable;
}

bool CWindowDisplayerA::update ()
{
	vector<string> copy;
	{
		CSynchronized<std::vector<std::string> >::CAccessor access (&_CommandsToExecute);
		copy = access.value();
		access.value().clear ();
	}

	// execute all commands in the main thread
	for (uint i = 0; i < copy.size(); i++)
	{
		sipassert (Log != NULL);
		ICommand::execute (copy[i], *(CLog *)Log);
	}

	return _Continue;
}

uint CWindowDisplayerA::createLabel (const char *value)
{
	int pos;
	{
		CSynchronized<std::vector<CLabelEntry> >::CAccessor access (&_Labels);
		access.value().push_back (CLabelEntry(value));
		pos = access.value().size()-1;
	}
	return pos;
}

void CWindowDisplayerA::setLabel (uint label, const string &value)
{
	{
		CSynchronized<std::vector<CLabelEntry> >::CAccessor access (&_Labels);
		sipassert (label < access.value().size());
		if (access.value()[label].Value != value)
		{
			access.value()[label].Value = value;
			access.value()[label].NeedUpdate = true;
		}
	}
}

void CWindowDisplayerA::create (string windowNameEx, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::string &fn, bool ww, CLogA *log)
{
	sipassert (_Thread == NULL);
	_Runable = new CUpdateThreadA(this, windowNameEx, iconified, x, y, w, h, hs, fs, fn, ww, log);
	_Thread = IThread::create (_Runable);

	Log = log;

	_Thread->start ();
}

void CWindowDisplayerA::doDisplay (const SIPBASE::CLogA::TDisplayInfo &args, const char *message)
{
	bool needSpace = false;
	//stringstream ss;
	string str;

	uint32 color = 0xFF000000;

	if (args.LogType != ILog::LOG_NO)
	{
		str += logTypeToString(args.LogType);
		if (args.LogType == ILog::LOG_ERROR || args.LogType == ILog::LOG_ASSERT) color = 0x00FF0000;
		else if (args.LogType == ILog::LOG_WARNING) color = 0x00800000;
		else if (args.LogType == ILog::LOG_DEBUG) color = 0x00808080;
		else color = 0;
		needSpace = true;
	}
	str += " ";
	str += dateToHumanString(args.Date);
	// Write thread identifier
	if ( args.ThreadId != 0 )
	{
		if (needSpace) { str += " "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString("%4x", args.ThreadId);
#else
		str += SIPBASE::toString("%4u", args.ThreadId);
#endif
		needSpace = true;
	}

	if (args.FileName != NULL)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += SIPBASE::toString("%20s", CFileA::getFilename(args.FileName).c_str());
		needSpace = true;
	}

	if (args.Line != -1)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += SIPBASE::toString("%4u", args.Line);
		//ss << setw(4) << args.Line;
		needSpace = true;
	}

	if (args.FuncName != NULL)
	{
		if (needSpace) { str += " "; needSpace = false; }
		str += SIPBASE::toString("%20s", args.FuncName);
		needSpace = true;
	}

	if (needSpace) { str += ": "; needSpace = false; }

	uint nbl = 1;

	char *npos, *pos = const_cast<char *>(message);
	while ((npos = strchr (pos, '\n')))
	{
		*npos = '\0';
		str += pos;
		if (needSlashR)
			str += "\r";
		str += "\n";
		*npos = '\n';
		pos = npos+1;
		nbl++;
	}
	str += pos;

	pos = const_cast<char *>(args.CallstackAndLog.c_str());
	while ((npos = strchr (pos, '\n')))
	{
		*npos = '\0';
		str += pos;
		if (needSlashR)
			str += "\r";
		str += "\n";
		*npos = '\n';
		pos = npos+1;
		nbl++;
	}
	str += pos;

	{
		CSynchronized<std::list<std::pair<uint32, std::string > > >::CAccessor access (&_Buffer);
		if (_HistorySize > 0 && access.value().size() >= (uint)_HistorySize)
		{
			access.value().erase (access.value().begin());
		}
		access.value().push_back (make_pair (color, str));
	}
}

// unicode version
CWindowDisplayerW::~CWindowDisplayerW ()
{
	// we have to wait the exit of the thread
	_Continue = false;
	sipassert (_Thread != NULL);
	_Thread->wait();
	delete _Thread;
	delete _Runable;
}

bool CWindowDisplayerW::update ()
{
	vector<string> copy;
	{
		CSynchronized<std::vector<std::string> >::CAccessor access (&_CommandsToExecute);
		copy = access.value();
		access.value().clear ();
	}

	// execute all commands in the main thread
	for (uint i = 0; i < copy.size(); i++)
	{
		sipassert (Log != NULL);
		ICommand::execute (copy[i], *(CLog *)Log);
	}

	return _Continue;
}

uint CWindowDisplayerW::createLabel (const ucchar *value)
{
	int pos;
	{
		CSynchronized<std::vector<CLabelEntry> >::CAccessor access (&_Labels);
		access.value().push_back (CLabelEntry(value));
		pos = access.value().size()-1;
	}
	return pos;
}

uint CWindowDisplayerW::createLabel (const char *value)
{
	int pos;
	{
		CSynchronized<std::vector<CLabelEntry> >::CAccessor access (&_Labels);
		access.value().push_back (CLabelEntry(value));
		pos = access.value().size()-1;
	}
	return pos;
}

void CWindowDisplayerW::setLabel (uint label, const basic_string<ucchar> &value)
{
	{
		CSynchronized<std::vector<CLabelEntry> >::CAccessor access (&_Labels);
		sipassert (label < access.value().size());
		if (access.value()[label].Value != value)
		{
			access.value()[label].Value = value;
			access.value()[label].NeedUpdate = true;
		}
	}
}

void CWindowDisplayerW::setLabel (uint label, const string &value)
{
	{
		CSynchronized<std::vector<CLabelEntry> >::CAccessor access (&_Labels);
		sipassert (label < access.value().size());

		basic_string<ucchar> value1 = getUcsFromMbs(value.c_str());
		if (access.value()[label].Value != value1)
		{
			access.value()[label].Value = value1;
			access.value()[label].NeedUpdate = true;
		}
	}
}

void CWindowDisplayerW::create (basic_string<ucchar> windowNameEx, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::basic_string<ucchar> &fn, bool ww, CLogW *log)
{
	sipassert (_Thread == NULL);
	_Runable = new CUpdateThreadW(this, windowNameEx, iconified, x, y, w, h, hs, fs, fn, ww, log);
	_Thread = IThread::create (_Runable);

	Log = log;

	_Thread->start ();
}

void CWindowDisplayerW::create (string windowNameEx, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::string &fn, bool ww, CLogW *log)
{
	sipassert (_Thread == NULL);
	_Runable = new CUpdateThreadW(this, getUcsFromMbs(windowNameEx.c_str()), iconified, x, y, w, h, hs, fs, getUcsFromMbs(fn.c_str()), ww, log);
	_Thread = IThread::create (_Runable);

	Log = log;

	_Thread->start ();
}

void CWindowDisplayerW::doDisplay (const SIPBASE::CLogW::TDisplayInfo &args, const ucchar *message)
{
	bool needSpace = false;
	//stringstream ss;
	basic_string<ucchar> str;

	uint32 color = 0xFF000000;

	if (args.LogType != ILog::LOG_NO)
	{
		str += logTypeToString(args.LogType);
		if (args.LogType == ILog::LOG_ERROR || args.LogType == ILog::LOG_ASSERT) color = 0x00FF0000;
		else if (args.LogType == ILog::LOG_WARNING) color = 0x00800000;
		else if (args.LogType == ILog::LOG_DEBUG) color = 0x00808080;
		else color = 0;
		needSpace = true;
	}
	str += L" ";
	str += dateToHumanString(args.Date);

	// Write thread identifier
	if ( args.ThreadId != 0 )
	{
		if (needSpace) { str += L" "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString(L"%4x", args.ThreadId);
#else
		str += SIPBASE::toStringLW("%4u", args.ThreadId);
#endif
		needSpace = true;
	}

	if (!args.FileName.empty())
	{
		if (needSpace) { str += L" "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString(L"%20s", CFileW::getFilename(args.FileName).c_str());
#else
		str += SIPBASE::toStringLW("%20S", CFileW::getFilename(args.FileName).c_str());
#endif
		needSpace = true;
	}

	if (args.Line != -1)
	{
		if (needSpace) { str += L" "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString(L"%4u", args.Line);
#else
		str += SIPBASE::toStringLW("%4u", args.Line);
#endif
		//ss << setw(4) << args.Line;
		needSpace = true;
	}

	if (!args.FuncName.empty())
	{
		if (needSpace) { str += L" "; needSpace = false; }
#ifdef SIP_OS_WINDOWS
		str += SIPBASE::toString(L"%20s", args.FuncName.c_str());
#else
		str += SIPBASE::toStringLW("%20S", args.FuncName.c_str());
#endif
		needSpace = true;
	}

	if (needSpace) { str += L": "; needSpace = false; }

	uint nbl = 1;

	ucchar *npos, *pos = const_cast<ucchar *>(message);
	while ((npos = wcschr (pos, L'\n')))
	{
		*npos = L'\0';
		str += pos;
		if (needSlashR)
			str += L"\r";
		str += L"\n";
		*npos = L'\n';
		pos = npos+1;
		nbl++;
	}
	str += pos;

	pos = const_cast<ucchar *>(args.CallstackAndLog.c_str());
	while ((npos = wcschr (pos, L'\n')))
	{
		*npos = L'\0';
		str += pos;
		if (needSlashR)
			str += L"\r";
		str += L"\n";
		*npos = L'\n';
		pos = npos+1;
		nbl++;
	}
	str += pos;

	{
		CSynchronized<std::list<std::pair<uint32, std::basic_string<ucchar> > > >::CAccessor access (&_Buffer);
		if (_HistorySize > 0 && access.value().size() >= (uint)_HistorySize)
		{
			access.value().erase (access.value().begin());
		}
		access.value().push_back (make_pair (color, str));
	}
}

} // SIPBASE











