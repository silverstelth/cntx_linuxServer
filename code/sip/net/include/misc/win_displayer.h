/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __WIN_DISPLAYER_H__
#define __WIN_DISPLAYER_H__

#include "types_sip.h"

#ifdef SIP_OS_WINDOWS

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#undef min
#undef max

#include "displayer.h"
#include "reader_writer.h"
#include "window_displayer.h"

namespace SIPBASE {


/**
 * this displayer displays on a win32 windows.
 * MT = Main Thread, DT = Display Thread
 * \date 2001
 */
class CWinDisplayerA : public SIPBASE::CWindowDisplayerA
{
public:

	CWinDisplayerA (const char *displayerName = "") : CWindowDisplayerA(displayerName), Exit(false)
	{
		needSlashR = true;
		createLabel ("@Clear|CLEAR");
	}

	virtual ~CWinDisplayerA ();

#ifdef SIP_OS_WINDOWS
	HWND getHWnd () const { return _HWnd; }
#endif // SIP_OS_WINDOWS

private:

	// called by DT only
	void	resizeLabels ();
	// called by DT only
	void	updateLabels ();

	// called by DT only
	void	open (std::string titleBar, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::string &fn, bool ww, CLogA *log);
	// called by DT only
	void	clear ();
	// called by DT only
	void	display_main ();

	virtual void	setTitleBar (const std::string &titleBar);

	virtual void	getWindowPos (uint32 &x, uint32 &y, uint32 &w, uint32 &h);

	// all these variables above is used only by the DT

	HWND _HEdit, _HWnd, _HInputEdit;
	HFONT _HFont;
	HMODULE _HLibModule;

	CLogA *Log;

	// the MT must set the value to true to exit the thread
	bool Exit;

	friend LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

class CWinDisplayerW : public SIPBASE::CWindowDisplayerW
{
public:

	CWinDisplayerW (const char *displayerName = "");

	virtual ~CWinDisplayerW ();

#ifdef SIP_OS_WINDOWS
	HWND getHWnd () const { return _HWnd; }
#endif // SIP_OS_WINDOWS

private:

	// called by DT only
	void	resizeLabels ();
	// called by DT only
	void	updateLabels ();

	// called by DT only
	void	open (std::basic_string<ucchar> titleBar, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::basic_string<ucchar> &fn, bool ww, CLogW *log);
	// called by DT only
	void	clear ();
	// called by DT only
	void	display_main ();

	virtual void	setTitleBar (const std::string &titleBar);
	virtual void	setTitleBar (const std::basic_string<ucchar> &titleBar);

	virtual void	getWindowPos (uint32 &x, uint32 &y, uint32 &w, uint32 &h);

	// all these variables above is used only by the DT

	HWND _HEdit, _HWnd, _HInputEdit;
	HFONT _HFont;
	HMODULE _HLibModule;

	CLogW *Log;

	// the MT must set the value to true to exit the thread
	bool Exit;

	friend LRESULT CALLBACK WndProcW (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
};

} // SIPBASE

#endif // SIP_OS_WINDOWS

#endif // __WIN_DISPLAYER_H__

/* End of win_displayer.h */
