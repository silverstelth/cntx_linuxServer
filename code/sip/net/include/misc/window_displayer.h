/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __WINDOW_DISPLAYER_H__
#define __WINDOW_DISPLAYER_H__

#include "types_sip.h"
#include "common.h"
#include "debug.h"

#include "displayer.h"
#include "mutex.h"
#include "thread.h"

namespace SIPBASE {

/**
 * this displayer displays on a win32 windows.
 * MT = Main Thread, DT = Display Thread
 * \date 2001
 */
class CWindowDisplayerW : public SIPBASE::IDisplayerW
{
public:

	CWindowDisplayerW (const char *displayerName = "") :
	  IDisplayerW(displayerName), 
		_Buffer("CWindowDisplayer::_Buffer"), _Labels("CWindowDisplayer::_Labels"), _CommandsToExecute("CWindowDisplayer::_CommandsToExecute"),
		_Continue(true), _PosInHistory(0), _Init(false), _HistorySize(0), _ToolBarHeight(22), _InputEditHeight(25), _Thread(0), _Runable(0), Log(0)
	  { }

	virtual ~CWindowDisplayerW ();

	// open the window and run the display thread (MT)
	void	create (std::basic_string<ucchar> titleBar = L"", bool iconified = false, sint x = -1, sint y = -1, sint w = -1, sint h = -1, sint hs = -1, sint fs = 0, const std::basic_string<ucchar> &fn = L"", bool ww = false, CLogW *log = (CLogW *)((CLog *)InfoLog));
	void	create (std::string titleBar = "", bool iconified = false, sint x = -1, sint y = -1, sint w = -1, sint h = -1, sint hs = -1, sint fs = 0, const std::string &fn = "", bool ww = false, CLogW *log = (CLogW *)((CLog *)InfoLog));

	// create a new label. empty string mean separator. start with @ means that is a command (MT)
	uint	createLabel (const ucchar *value = L"?");
	uint	createLabel (const char *value = "?");

	// change the value of a label (MT)
	void	setLabel (uint label, const std::basic_string<ucchar> &value);
	void	setLabel (uint label, const std::string &value);

	// execute user commands (MT) return false to quit
	bool	update ();

	// set a special title to the window bar
	virtual void	setTitleBar (const std::string &titleBar) { }
	virtual void	setTitleBar (const std::basic_string<ucchar> &titleBar) { }

	virtual void	getWindowPos (uint32 &x, uint32 &y, uint32 &w, uint32 &h) { x=y=w=h=0; }

protected:

	// display a string (MT)
	virtual void doDisplay (const SIPBASE::CLogW::TDisplayInfo &args, const ucchar *message);

	// true for windows
	bool needSlashR;

	struct CLabelEntry
	{
		CLabelEntry (const std::basic_string<ucchar> &value) : Hwnd(NULL), Value(value), NeedUpdate(true) { }
		CLabelEntry (const std::string &value) : Hwnd(NULL), Value(SIPBASE::getUcsFromMbs(value.c_str())), NeedUpdate(true) { }
		void						*Hwnd;
		std::basic_string<ucchar>	Value;
		bool						NeedUpdate;
	};

	// buffer that contains the text that the DT will have to display
	// uint32 is the color of the string
	CSynchronized<std::list<std::pair<uint32, std::basic_string<ucchar> > > >	_Buffer;
	CSynchronized<std::vector<CLabelEntry> >									_Labels;
	CSynchronized<std::vector<std::string> >									_CommandsToExecute;

	// called by DT only
	virtual void	open (std::basic_string<ucchar> titleBar, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::basic_string<ucchar> &fn, bool ww, CLog *log) = 0;
	// called by DT only
	virtual void	display_main () = 0;

	// all these variables above is used only by the DT

	bool _Continue;

	std::vector<std::string>	_History;
	uint						_PosInHistory;
	bool _Init;
	sint _HistorySize;
	sint _ToolBarHeight;
	sint _InputEditHeight;

	// the thread used to update the display
	SIPBASE::IThread *_Thread;
	SIPBASE::IRunnable *_Runable;
	CLogW *Log;

	friend class CUpdateThreadW;
};

class CWindowDisplayerA : public SIPBASE::IDisplayerA
{
public:

	CWindowDisplayerA (const char *displayerName = "") :
	  IDisplayerA(displayerName), 
		_Buffer("CWindowDisplayer::_Buffer"), _Labels("CWindowDisplayer::_Labels"), _CommandsToExecute("CWindowDisplayer::_CommandsToExecute"),
		_Continue(true), _PosInHistory(0), _Init(false), _HistorySize(0), _ToolBarHeight(22), _InputEditHeight(25), _Thread(0), _Runable(0), Log(0)
	  { }

	virtual ~CWindowDisplayerA ();

	// open the window and run the display thread (MT)
	void	create (std::string titleBar = "", bool iconified = false, sint x = -1, sint y = -1, sint w = -1, sint h = -1, sint hs = -1, sint fs = 0, const std::string &fn = "", bool ww = false, CLogA *log = (CLogA *)((CLog *)InfoLog));

	// create a new label. empty string mean separator. start with @ means that is a command (MT)
	uint	createLabel (const char *value = "?");

	// change the value of a label (MT)
	void	setLabel (uint label, const std::string &value);

	// execute user commands (MT) return false to quit
	bool	update ();

	// set a special title to the window bar
	virtual void	setTitleBar (const std::string &titleBar) { }

	virtual void	getWindowPos (uint32 &x, uint32 &y, uint32 &w, uint32 &h) { x=y=w=h=0; }

protected:

	// display a string (MT)
	virtual void doDisplay (const SIPBASE::CLogA::TDisplayInfo &args, const char *message);

	// true for windows
	bool needSlashR;

	struct CLabelEntry
	{
		CLabelEntry (const std::string &value) : Hwnd(NULL), Value(value), NeedUpdate(true) { }
		void		*Hwnd;
		std::string	 Value;
		bool		 NeedUpdate;
	};

	// buffer that contains the text that the DT will have to display
	// uint32 is the color of the string
	CSynchronized<std::list<std::pair<uint32, std::string> > >	_Buffer;
	CSynchronized<std::vector<CLabelEntry> >					_Labels;
	CSynchronized<std::vector<std::string> >					_CommandsToExecute;

	// called by DT only
	virtual void	open (std::string titleBar, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::string &fn, bool ww, CLogA *log) = 0;
	// called by DT only
	virtual void	display_main () = 0;

	// all these variables above is used only by the DT

	bool _Continue;

	std::vector<std::string>	_History;
	uint						_PosInHistory;
	bool _Init;
	sint _HistorySize;
	sint _ToolBarHeight;
	sint _InputEditHeight;

	// the thread used to update the display
	SIPBASE::IThread *_Thread;
	SIPBASE::IRunnable *_Runable;

	CLogA *Log;

	friend class CUpdateThreadA;
};

} // SIPBASE

#endif // __WINDOW_DISPLAYER_H__

/* End of window_displayer.h */
