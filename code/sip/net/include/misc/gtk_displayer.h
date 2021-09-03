/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __GTK_DISPLAYER_H__
#define __GTK_DISPLAYER_H__

#ifdef SIP_USE_GTK

#include "types_sip.h"
#include "displayer.h"
#include "reader_writer.h"
#include "window_displayer.h"
#include <gtk/gtk.h>

namespace SIPBASE {

/**
 * this displayer displays on a gtk windows.
 * MT = Main Thread, DT = Display Thread
 * \date 2001
 */
class CGtkDisplayerA : public SIPBASE::CWindowDisplayerA
{
public:

	CGtkDisplayerA (const char *displayerName = "") : CWindowDisplayerA(displayerName)
	{
		needSlashR = false;
		createLabel ("@Clear|CLEAR");
	}

	virtual ~CGtkDisplayerA ();

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
	
	friend gint updateInterf (gpointer data);
	friend gint ButtonClicked(GtkWidget *Widget, gpointer *Data);
		
	// the MT must set the value to true to exit the thread
	bool Exit;
};


//add start by cch 2009.8.6
class CGtkDisplayerW : public SIPBASE::CWindowDisplayerW
{
public:

	CGtkDisplayerW (const char *displayerName = "") : CWindowDisplayerW(displayerName)
	{
		needSlashR = true;
		createLabel ("@Clear|CLEAR");
	}

	virtual ~CGtkDisplayerW ();

private:

	// called by DT only
	void	resizeLabels ();
	// called by DT only
	void	updateLabels ();

	// called by DT only
	void	open (std::basic_string<ucchar> titleBar, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::basic_string<ucchar>  &fn, bool ww, CLogW *log);
	// called by DT only
	void	clear ();
	// called by DT only
	void	display_main ();

	virtual void	setTitleBar (const std::string &titleBar);
	virtual void	setTitleBar (const std::basic_string<ucchar> &titleBar);
	
	virtual void	getWindowPos (uint32 &x, uint32 &y, uint32 &w, uint32 &h);
	
	// all these variables above is used only by the DT
	
	friend gint updateInterf (gpointer data);
	friend gint ButtonClicked(GtkWidget *Widget, gpointer *Data);
		
	// the MT must set the value to true to exit the thread
	bool Exit;
};
//add end by cch 2009.8.6
} // SIPBASE

#endif // SIP_USE_GTK

#endif // __GTK_DISPLAYER_H__

/* End of gtk_displayer.h */
