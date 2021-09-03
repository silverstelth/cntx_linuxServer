/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#ifdef SIP_USE_GTK

#define GTK_ENABLE_BROKEN
#include <gdk/gdkkeysyms.h>

#ifdef SIP_OS_WINDOWS
// automatically add gtk library
#pragma comment(lib, "gtk-1.3.lib")
#pragma comment(lib, "gdk-1.3.lib")
#pragma comment(lib, "glib-1.3.lib")
#pragma comment(lib, "gthread-1.3.lib")
#endif

#include <iostream>
#include <fstream>
#include <iomanip>
#include <signal.h> // <csignal>

#include "misc/path.h"
#include "misc/command.h"
#include "misc/thread.h"

#include "misc/gtk_displayer.h"

using namespace std;

namespace SIPBASE {

//
// Variables
//

static vector<string> CommandHistory;
static uint32 CommandHistoryPos = 0;
static CLog *Log = 0;
	
static GtkWidget *RootWindow = NULL, *OutputText = NULL, *InputText = NULL, *hrootbox = NULL;

//
// Functions
//

CGtkDisplayerA::~CGtkDisplayerA ()
{
	if (_Init)
	{
	}
}

/*gint ButtonClicked(GtkWidget *Widget, gpointer *Data)
{
	CGtkDisplayerA *disp = (CGtkDisplayerA *) Data;
	
	// find the button and execute the command
	CSynchronized<std::vector<CGtkDisplayerA::CLabelEntry> >::CAccessor access (&(disp->_Labels));
	for (uint i = 0; i < access.value().size(); i++)
	{
		if (access.value()[i].Hwnd == Widget)
		{
			if(access.value()[i].Value == "@Clear|CLEAR")
			{
				// special commands because the clear must be called by the display thread and not main thread
				disp->clear ();
			}
			else
			{
				// the button was found, add the command in the command stack
				CSynchronized<std::vector<std::string> >::CAccessor accessCommands (&disp->_CommandsToExecute);
				string str;
				sipassert (!access.value()[i].Value.empty());
				sipassert (access.value()[i].Value[0] == '@');
				
				int pos = access.value()[i].Value.find ("|");
				if (pos != string::npos)
				{
					str = access.value()[i].Value.substr(pos+1);
				}
				else
				{
					str = access.value()[i].Value.substr(1);
				}
				if (!str.empty())
					accessCommands.value().push_back(str);
			}
			break;
		}
	}
	return TRUE;
}*/

gint ButtonClicked(GtkWidget *Widget, gpointer *Data)
{
	CGtkDisplayerW *disp = (CGtkDisplayerW *) Data;
	
	// find the button and execute the command
	CSynchronized<std::vector<CGtkDisplayerW::CLabelEntry> >::CAccessor access (&(disp->_Labels));
	for (uint i = 0; i < access.value().size(); i++)
	{
		if (access.value()[i].Hwnd == Widget)
		{
			if(access.value()[i].Value == L"@Clear|CLEAR")
			{
				// special commands because the clear must be called by the display thread and not main thread
				disp->clear ();
			}
			else
			{
				// the button was found, add the command in the command stack
				CSynchronized<std::vector<std::string> >::CAccessor accessCommands (&disp->_CommandsToExecute);
				ucstring str;
				sipassert (!access.value()[i].Value.empty());
				sipassert (access.value()[i].Value[0] == '@');
				
				int pos = access.value()[i].Value.find (L"|");
				if (pos != ucstring::npos)
				{
					str = access.value()[i].Value.substr(pos+1);
				}
				else
				{
					str = access.value()[i].Value.substr(1);
				}
				
				if (!str.empty())
				{
					gchar str1[256];
					sprintf(str1, "%S", str.c_str());
					accessCommands.value().push_back(str1);
				}
			}
			break;
		}
	}
	return TRUE;
}

void CGtkDisplayerA::updateLabels ()
{
	{
		CSynchronized<std::vector<CLabelEntry> >::CAccessor access (&_Labels);
		for (uint i = 0; i < access.value().size(); i++)
		{
			if (access.value()[i].NeedUpdate && !access.value()[i].Value.empty())
			{
				string n;
				
				if (access.value()[i].Value[0] != '@')
					n = access.value()[i].Value;
				else
				{
					int pos = access.value()[i].Value.find ('|');
					if (pos != string::npos)
					{
						n = access.value()[i].Value.substr (1, pos - 1);
					}
					else
					{
						n = access.value()[i].Value.substr (1);
					}
				}
				
				if (access.value()[i].Hwnd == NULL)
				{
					// create a button for command and label for variables
					if (access.value()[i].Value[0] == '@')
					{
						access.value()[i].Hwnd = gtk_button_new_with_label (n.c_str());
						sipassert (access.value()[i].Hwnd != NULL);
						gtk_signal_connect (GTK_OBJECT (access.value()[i].Hwnd), "clicked", GTK_SIGNAL_FUNC (ButtonClicked), (gpointer) this);
						gtk_label_set_justify (GTK_LABEL (access.value()[i].Hwnd), GTK_JUSTIFY_LEFT);
						gtk_label_set_line_wrap (GTK_LABEL (access.value()[i].Hwnd), FALSE);
						gtk_widget_show (GTK_WIDGET (access.value()[i].Hwnd));
						gtk_box_pack_start (GTK_BOX (hrootbox), GTK_WIDGET (access.value()[i].Hwnd), TRUE, TRUE, 0);
					}
					else
					{
						access.value()[i].Hwnd = gtk_label_new ("");
						gtk_label_set_justify (GTK_LABEL (access.value()[i].Hwnd), GTK_JUSTIFY_LEFT);
						gtk_label_set_line_wrap (GTK_LABEL (access.value()[i].Hwnd), FALSE);
						gtk_widget_show (GTK_WIDGET (access.value()[i].Hwnd));
						gtk_box_pack_start (GTK_BOX (hrootbox), GTK_WIDGET (access.value()[i].Hwnd), TRUE, TRUE, 0);
					}
				}

				if (access.value()[i].Value[0] != '@')
					gtk_label_set_text (GTK_LABEL (access.value()[i].Hwnd), n.c_str());

				access.value()[i].NeedUpdate = false;
			}
		}
	}
}

// windows delete event => quit
gint delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gtk_main_quit();

	exit(1);
	return FALSE;
}

gint KeyIn(GtkWidget *Widget, GdkEventKey *Event, gpointer *Data)
{
	switch (Event->keyval)
	{
	case GDK_Escape : gtk_entry_set_text (GTK_ENTRY(Widget), ""); break;
	case GDK_Up : if (CommandHistoryPos > 0) { CommandHistoryPos--; gtk_entry_set_text (GTK_ENTRY(Widget), CommandHistory[CommandHistoryPos].c_str()); } break;
	case GDK_Down : if (CommandHistoryPos + 1 < CommandHistory.size()) { CommandHistoryPos++; gtk_entry_set_text (GTK_ENTRY(Widget), CommandHistory[CommandHistoryPos].c_str()); } break;
	case GDK_KP_Enter : gtk_signal_emit_by_name(GTK_OBJECT(Widget),"activate"); return FALSE; break;
	default : return FALSE;
	}
	gtk_signal_emit_stop_by_name(GTK_OBJECT(Widget),"key_press_event");
	return TRUE;
}

void updateInput ()
{
	gtk_widget_grab_focus (InputText);
}

gint KeyOut(GtkWidget *Widget, GdkEventKey *Event, gpointer *Data)
{
	updateInput();
	gtk_signal_emit_stop_by_name(GTK_OBJECT(Widget),"key_press_event");
	return TRUE;
}


/*gint ButtonClear(GtkWidget *Widget, GdkEventKey *Event, gpointer *Data)
{
	CGtkDisplayerA *disp = (CGtkDisplayerA *) Data;

	disp->clear ();
	return TRUE;
}
*/


// the user typed  command, execute it
gint cbValidateCommand (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	string cmd = gtk_entry_get_text (GTK_ENTRY(widget));
	CommandHistory.push_back (cmd);
	// execute the command
	if(Log == NULL)
		Log = InfoLog;
	ICommand::execute (cmd, *Log);
	// clear the input text
	gtk_entry_set_text (GTK_ENTRY(widget), "");
	CommandHistoryPos = CommandHistory.size();
	return TRUE;
}


void CGtkDisplayerA::setTitleBar (const string &titleBar)
{
	string wn;
	if (!titleBar.empty())
	{
		wn += titleBar;
		wn += ": ";
	}
#ifdef SIP_RELEASE_DEBUG
	string mode = "SIP_RELEASE_DEBUG";
#elif defined(SIP_DEBUG_FAST)
	string mode = "SIP_DEBUG_FAST";
#elif defined(SIP_DEBUG)
	string mode = "SIP_DEBUG";
#elif defined(SIP_RELEASE)
	string mode = "SIP_RELEASE";
#else
	string mode = "???";
#endif
	wn += "Sip Service Console (compiled " __DATE__ " " __TIME__ " in " + mode + " mode)";

	gtk_window_set_title (GTK_WINDOW (RootWindow), wn.c_str());
}

void CGtkDisplayerA::open (std::string titleBar, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::string &fn, bool ww, CLogA *log)
{
	_HistorySize = hs;

	if (w == -1)
		w = 700;
	if (h == -1)
		h = 300;
	if (hs = -1)
		hs = 10000;

	gtk_init (NULL, NULL);

	Log = log;

	// Root window
	RootWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (RootWindow), w, h);
	gtk_signal_connect (GTK_OBJECT (RootWindow), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);

	// Vertical box
	GtkWidget *vrootbox = gtk_vbox_new (FALSE, 0);
	sipassert (vrootbox != NULL);
	gtk_container_add (GTK_CONTAINER (RootWindow), vrootbox);

	// Horizontal box (for labels)
	hrootbox = gtk_hbox_new (FALSE, 0);
	sipassert (hrootbox != NULL);
	gtk_box_pack_start (GTK_BOX (vrootbox), hrootbox, FALSE, FALSE, 0);

/*	// Clear button
    GtkWidget *button = gtk_button_new_with_label ("Clear");
	sipassert (button != NULL);
    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (ButtonClear), (gpointer) this);
	gtk_box_pack_start (GTK_BOX (hrootbox), button, FALSE, FALSE, 0);
*/
	// Output text
	GtkWidget *scrolled_win2 = gtk_scrolled_window_new (NULL, NULL);
	sipassert (scrolled_win2 != NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win2), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_widget_show (scrolled_win2);
	gtk_container_add (GTK_CONTAINER (vrootbox), scrolled_win2);

	OutputText = gtk_text_new (NULL, NULL);
	sipassert (OutputText != NULL);
	gtk_signal_connect(GTK_OBJECT(OutputText),"key_press_event",GTK_SIGNAL_FUNC(KeyOut),NULL);
	gtk_text_set_editable (GTK_TEXT (OutputText), FALSE);
	gtk_container_add (GTK_CONTAINER (scrolled_win2), OutputText);

	// Input text
	InputText = gtk_entry_new ();
	sipassert (InputText != NULL);
	gtk_signal_connect (GTK_OBJECT(InputText), "activate", GTK_SIGNAL_FUNC(cbValidateCommand), NULL);
	gtk_signal_connect(GTK_OBJECT(InputText),"key_press_event",GTK_SIGNAL_FUNC(KeyIn),NULL);
	gtk_box_pack_start (GTK_BOX (vrootbox), InputText, FALSE, FALSE, 0);

//	gtk_widget_show (button);
	gtk_widget_show (OutputText);
	gtk_widget_show (InputText);
	
	gtk_widget_show (hrootbox);
	gtk_widget_show (vrootbox);
	gtk_widget_show (RootWindow);

	setTitleBar (titleBar);

	_Init = true;
}

void CGtkDisplayerA::clear ()
{
	int n;

	gtk_text_set_point(GTK_TEXT(OutputText),0);
	n = gtk_text_get_length(GTK_TEXT(OutputText));
	gtk_text_forward_delete(GTK_TEXT(OutputText),n);
}

gint updateInterf (gpointer data)
{
	CGtkDisplayer *disp = (CGtkDisplayer *)data;

	//
	// Update labels
	//

	disp->updateLabels ();

	//
	// Display the bufferized string
	//

	GtkAdjustment *Adj = (GTK_TEXT(OutputText))->vadj;
	bool Bottom = (Adj->value >= Adj->upper - Adj->page_size);

	std::list<std::pair<uint32, std::basic_string<ucchar> > >::iterator it;
	{
		CSynchronized<std::list<std::pair<uint32, std::basic_string<ucchar> > > >::CAccessor access (&disp->_Buffer);

		for (it = access.value().begin(); it != access.value().end(); it++)
		{
			ucstring str((*it).second.c_str());

			gtk_text_freeze (GTK_TEXT (OutputText));
			gtk_text_insert (GTK_TEXT (OutputText), NULL, NULL, NULL, str.toUtf8().c_str(), -1);
			gtk_text_thaw (GTK_TEXT (OutputText));
		}
		access.value().clear ();
	}

	if (Bottom)
	{
		gtk_adjustment_set_value(Adj,Adj->upper-Adj->page_size);
	}

	return TRUE;
}


void CGtkDisplayerA::display_main ()
{
	//
	// Manage windows message
	//

	gtk_timeout_add (10, updateInterf, this);
	gtk_main ();
}


void CGtkDisplayerA::getWindowPos (uint32 &x, uint32 &y, uint32 &w, uint32 &h)
{
// todo
	x = y = w = h = 0;
}

//add start by cch 2009.8.6
CGtkDisplayerW::~CGtkDisplayerW()
{
	if (_Init)
	{
	}
}

void CGtkDisplayerW::setTitleBar (const string &titleBar)
{
	setTitleBar(getUcsFromMbs(titleBar.c_str()));
}

void CGtkDisplayerW::setTitleBar (const basic_string<ucchar> &titleBar)
{
	basic_string<ucchar> wn;
	if (!titleBar.empty())
	{
		wn += titleBar;
		wn += L": ";
	}

/*#ifdef SIP_RELEASE_DEBUG
	basic_string<ucchar> mode = L"���������ٱ�";
#elif defined(SIP_DEBUG_FAST)
	basic_string<ucchar> mode = L"���ٱ��Ľ�Ʈ";
#elif defined(SIP_DEBUG)
	basic_string<ucchar> mode = L"���ٱ�";
#elif defined(SIP_RELEASE)
	basic_string<ucchar> mode = L"������";
#else
	basic_string<ucchar> mode = L"��\EF\BF?;
#endif*/

#ifdef SIP_RELEASE_DEBUG
	basic_string<ucchar> mode = L"SIP_RELEASE_DEBUG";
#elif defined(SIP_DEBUG_FAST)
	basic_string<ucchar> mode = L"SIP_DEBUG_FAST";
#elif defined(SIP_DEBUG)
	basic_string<ucchar> mode = L"SIP_DEBUG";
#elif defined(SIP_RELEASE)
	basic_string<ucchar> mode = L"SIP_RELEASE";
#else
	basic_string<ucchar> mode = L"???";
#endif

#ifdef SIP_OS_WINDOWS
	wn += toStringLW("Sip Service Console (compiled in %s mode)",  mode.c_str());
#else
	wn += toStringLW("Sip Service Console (compiled in %S mode)",  mode.c_str());
#endif
//	wn += toString(L"Sip Service Console (compiled %S %S in %S mode)", __DATE__, __TIME__, mode.c_str());
//	wn += toString(L"������������ (�����Ͻð�: %S %S  �����Ϲ���: %s)", __DATE__, __TIME__, mode.c_str());
	
	gchar str[256];
	sprintf( str, "%S", wn.c_str() );

	gtk_window_set_title (GTK_WINDOW (RootWindow), str);
	//gtk_window_set_title (GTK_WINDOW (RootWindow), wn.c_str());

	//wn += "Sip Service Console (compiled " __DATE__ " " __TIME__ " in " + mode + " mode)";

	//gtk_window_set_title (GTK_WINDOW (RootWindow), wn.c_str());
}

void CGtkDisplayerW::updateLabels ()
{
	{
		CSynchronized<std::vector<CLabelEntry> >::CAccessor access (&_Labels);
		for (uint i = 0; i < access.value().size(); i++)
		{
			if (access.value()[i].NeedUpdate && !access.value()[i].Value.empty())
			{
				ucstring n;
				
				if (access.value()[i].Value[0] != '@')
					n = access.value()[i].Value;
				else
				{
					int pos = access.value()[i].Value.find ('|');
					if (pos != string::npos)
					{
						n = access.value()[i].Value.substr (1, pos - 1);
					}
					else
					{
						n = access.value()[i].Value.substr (1);
					}
				}
				
				gchar str[256];
				sprintf(str, "%S", n.c_str());

				if (access.value()[i].Hwnd == NULL)
				{
					// create a button for command and label for variables
					if (access.value()[i].Value[0] == '@')
					{
						access.value()[i].Hwnd = gtk_button_new_with_label (str);

						sipassert (access.value()[i].Hwnd != NULL);
						gtk_signal_connect (GTK_OBJECT (access.value()[i].Hwnd), "clicked", GTK_SIGNAL_FUNC (ButtonClicked), (gpointer) this);
						// gtk_label_set_justify (GTK_LABEL (access.value()[i].Hwnd), GTK_JUSTIFY_LEFT);
						// gtk_label_set_line_wrap (GTK_LABEL (access.value()[i].Hwnd), FALSE);
						gtk_widget_show (GTK_WIDGET (access.value()[i].Hwnd));
						gtk_box_pack_start (GTK_BOX (hrootbox), GTK_WIDGET (access.value()[i].Hwnd), TRUE, TRUE, 0);
					}
					else
					{
						access.value()[i].Hwnd = gtk_label_new ("");
						gtk_label_set_justify (GTK_LABEL (access.value()[i].Hwnd), GTK_JUSTIFY_LEFT);
						gtk_label_set_line_wrap (GTK_LABEL (access.value()[i].Hwnd), FALSE);
						gtk_widget_show (GTK_WIDGET (access.value()[i].Hwnd));
						gtk_box_pack_start (GTK_BOX (hrootbox), GTK_WIDGET (access.value()[i].Hwnd), TRUE, TRUE, 0);
					}
				}

				if (access.value()[i].Value[0] != '@')
					gtk_label_set_text (GTK_LABEL (access.value()[i].Hwnd), str);

				access.value()[i].NeedUpdate = false;
			}
		}
	}
}

void CGtkDisplayerW::open (std::basic_string<ucchar> titleBar, bool iconified, sint x, sint y, sint w, sint h, sint hs, sint fs, const std::basic_string<ucchar>  &fn, bool ww, CLogW *log)
{
	_HistorySize = hs;

	if (w == -1)
		w = 700;
	if (h == -1)
		h = 300;
	if (hs = -1)
		hs = 10000;

	gtk_init (NULL, NULL);

	Log = log;

	// Root window
	RootWindow = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (RootWindow), w, h);
	gtk_signal_connect (GTK_OBJECT (RootWindow), "delete_event", GTK_SIGNAL_FUNC (delete_event), NULL);

	// Vertical box
	GtkWidget *vrootbox = gtk_vbox_new (FALSE, 0);
	sipassert (vrootbox != NULL);
	gtk_container_add (GTK_CONTAINER (RootWindow), vrootbox);

	// Horizontal box (for labels)
	hrootbox = gtk_hbox_new (FALSE, 0);
	sipassert (hrootbox != NULL);
	gtk_box_pack_start (GTK_BOX (vrootbox), hrootbox, FALSE, FALSE, 0);

/*	// Clear button
    GtkWidget *button = gtk_button_new_with_label ("Clear");
	sipassert (button != NULL);
    gtk_signal_connect (GTK_OBJECT (button), "clicked", GTK_SIGNAL_FUNC (ButtonClear), (gpointer) this);
	gtk_box_pack_start (GTK_BOX (hrootbox), button, FALSE, FALSE, 0);
*/
	// Output text
	GtkWidget *scrolled_win2 = gtk_scrolled_window_new (NULL, NULL);
	sipassert (scrolled_win2 != NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win2), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_widget_show (scrolled_win2);
	gtk_container_add (GTK_CONTAINER (vrootbox), scrolled_win2);

	OutputText = gtk_text_new (NULL, NULL);
	sipassert (OutputText != NULL);
	gtk_signal_connect(GTK_OBJECT(OutputText),"key_press_event",GTK_SIGNAL_FUNC(KeyOut),NULL);
	gtk_text_set_editable (GTK_TEXT (OutputText), FALSE);
	gtk_container_add (GTK_CONTAINER (scrolled_win2), OutputText);

	// Input text
	InputText = gtk_entry_new ();
	sipassert (InputText != NULL);
	gtk_signal_connect (GTK_OBJECT(InputText), "activate", GTK_SIGNAL_FUNC(cbValidateCommand), NULL);
	gtk_signal_connect(GTK_OBJECT(InputText),"key_press_event",GTK_SIGNAL_FUNC(KeyIn),NULL);
	gtk_box_pack_start (GTK_BOX (vrootbox), InputText, FALSE, FALSE, 0);

//	gtk_widget_show (button);
	gtk_widget_show (OutputText);
	gtk_widget_show (InputText);
	
	gtk_widget_show (hrootbox);
	gtk_widget_show (vrootbox);
	gtk_widget_show (RootWindow);

	setTitleBar (titleBar);

	_Init = true;
}

void CGtkDisplayerW::clear ()
{
	int n;

	gtk_text_set_point(GTK_TEXT(OutputText),0);
	n = gtk_text_get_length(GTK_TEXT(OutputText));
	gtk_text_forward_delete(GTK_TEXT(OutputText),n);
}

void CGtkDisplayerW::display_main ()
{
	//
	// Manage windows message
	//

	gtk_timeout_add (10, updateInterf, this);
	gtk_main ();
}


void CGtkDisplayerW::getWindowPos (uint32 &x, uint32 &y, uint32 &w, uint32 &h)
{
// todo
	x = y = w = h = 0;
}


} // SIPBASE



//add end by cch 2009.8.6
#else // SIP_USE_GTK

// remove stupid VC6 warnings
void foo_gtk_displayer_cpp() {}

#endif // SIP_USE_GTK
