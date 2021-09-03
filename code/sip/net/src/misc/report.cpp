/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include <stdio.h> // <cstdio>
#include <stdlib.h> // <cstdlib>
#ifdef SIP_OS_MAC
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include "misc/common.h"

#include "misc/report.h"

#ifdef SIP_OS_WINDOWS

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

#ifdef min
#	undef min
#endif // min

#ifdef max
#	undef max
#endif // max

#endif // SIP_OS_WINDOWS

using namespace std;

namespace SIPBASE 
{

#ifdef SIP_OS_WINDOWS
static HWND sendReport=NULL;
#endif

//old doesn't work on visual c++ 7.1 due to default parameter typedef bool (*TEmailFunction) (const std::string &smtpServer, const std::string &from, const std::string &to, const std::string &subject, const std::string &body, const std::string &attachedFile = "", bool onlyCheck = false);
typedef bool (*TEmailFunction)  (const std::string &smtpServer, const std::string &from, const std::string &to, const std::string &subject, const std::string &body, const std::string &attachedFile, bool onlyCheck);
typedef bool (*TEmailFunctionW) (const std::string &smtpServer, const std::string &from, const std::string &to, 
								 const std::basic_string<ucchar> &subject, const std::basic_string<ucchar> &body, const std::basic_string<ucchar> &attachedFile, bool onlyCheck);

#define DELETE_OBJECT(a) if((a)!=NULL) { DeleteObject (a); a = NULL; }

static void *EmailFunction = NULL;

void setReportEmailFunction (void *emailFunction)
{
	EmailFunction = emailFunction;

#ifdef SIP_OS_WINDOWS
	if (sendReport)
		EnableWindow(sendReport, FALSE);
#endif
}

#ifndef SIP_OS_WINDOWS

// GNU/Linux, do nothing

void report ()
{
}

#else

// Windows specific version

static string Body;
static std::basic_string<ucchar> BodyW;

static string Subject;
static std::basic_string<ucchar> SubjectW;

static string AttachedFile;
static std::basic_string<ucchar> AttachedFileW;

static HWND checkIgnore=NULL;
static HWND debug=NULL;
static HWND ignore=NULL;
static HWND quit=NULL;
static HWND dialog=NULL;

static bool NeedExit;
static TReportResult Result;
static bool IgnoreNextTime;
static bool CanSendMailReport= false;

static bool DebugDefaultBehavior, QuitDefaultBehavior;

static void sendEmail()
{
	if (CanSendMailReport && SendMessageA(sendReport, BM_GETCHECK, 0, 0) != BST_CHECKED)
	{
		bool res = ((TEmailFunction)EmailFunction) ("", "", "", Subject, Body, AttachedFile, false);
		if (res)
		{
			// EnableWindow(sendReport, FALSE);
			// MessageBox (dialog, "The email was successfully sent", "email", MB_OK);
			FILE *file = fopen ("report_sent", "wb");
			fclose (file);
		}
		else
		{
			FILE *file = fopen ("report_failed", "wb");
			fclose (file);
			// MessageBox (dialog, "Failed to send the email", "email", MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		FILE *file = fopen ("report_refused", "wb");
		fclose (file);
	}
}

static void sendEmailW()
{
	if (CanSendMailReport && SendMessageW(sendReport, BM_GETCHECK, 0, 0) != BST_CHECKED)
	{
		bool res = ((TEmailFunctionW) EmailFunction) ("", "", "", SubjectW, BodyW, AttachedFileW, false);
		if (res)
		{
			// EnableWindow(sendReport, FALSE);
			// MessageBox (dialog, "The email was successfully sent", "email", MB_OK);
			FILE *file = sfWfopen (L"report_sent", L"wb");
			fclose (file);
		}
		else
		{
			FILE *file = sfWfopen (L"report_failed", L"wb");
			fclose (file);
			// MessageBox (dialog, "Failed to send the email", "email", MB_OK | MB_ICONERROR);
		}
	}
	else
	{
		FILE *file = sfWfopen (L"report_refused", L"wb");
		fclose (file);
	}
}

static LRESULT CALLBACK WndProc (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//MSGFILTER *pmf;

	if (message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED)
	{
		if ((HWND) lParam == checkIgnore)
		{
			IgnoreNextTime = !IgnoreNextTime;
		}
		else if ((HWND) lParam == debug)
		{
			sendEmail();
			NeedExit = true;
			Result = ReportDebug;
			if (DebugDefaultBehavior)
			{
				SIPBASE_BREAKPOINT;
			}
		}
		else if ((HWND) lParam == ignore)
		{
			sendEmail();
			NeedExit = true;
			Result = ReportIgnore;
		}
		else if ((HWND) lParam == quit)
		{
			sendEmail();
			NeedExit = true;
			Result = ReportQuit;

			if (QuitDefaultBehavior)
			{
				exit(EXIT_SUCCESS);
			}
		}
		/*else if ((HWND) lParam == sendReport)
		{
			if (EmailFunction != NULL)
			{
				bool res = EmailFunction ("", "", "", Subject, Body, AttachedFile, false);
				if (res)
				{
					EnableWindow(sendReport, FALSE);
					MessageBox (dialog, "The email was successfully sent", "email", MB_OK);
					FILE *file = fopen ("report_sent", "wb");
					fclose (file);
				}
				else
				{
					MessageBox (dialog, "Failed to send the email", "email", MB_OK | MB_ICONERROR);
				}
			}
		}*/
	}
	else if (message == WM_CHAR)
	{
		if (wParam == 27)
		{
			// ESC -> ignore
			sendEmail();
			NeedExit = true;
			Result = ReportIgnore;
		}
	}

	return DefWindowProc (hWnd, message, wParam, lParam);
}

static LRESULT CALLBACK WndProcW (HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//MSGFILTER *pmf;

	if (message == WM_COMMAND && HIWORD(wParam) == BN_CLICKED)
	{
		if ((HWND) lParam == checkIgnore)
		{
			IgnoreNextTime = !IgnoreNextTime;
		}
		else if ((HWND) lParam == debug)
		{
			sendEmailW();
			NeedExit = true;
			Result = ReportDebug;
			if (DebugDefaultBehavior)
			{
				SIPBASE_BREAKPOINT;
			}
		}
		else if ((HWND) lParam == ignore)
		{
			sendEmailW();
			NeedExit = true;
			Result = ReportIgnore;
		}
		else if ((HWND) lParam == quit)
		{
			sendEmailW();
			NeedExit = true;
			Result = ReportQuit;

			if (QuitDefaultBehavior)
			{
				exit(EXIT_SUCCESS);
			}
		}
		/*else if ((HWND) lParam == sendReport)
		{
			if (EmailFunction != NULL)
			{
				bool res = EmailFunction ("", "", "", Subject, Body, AttachedFile, false);
				if (res)
				{
					EnableWindow(sendReport, FALSE);
					MessageBox (dialog, "The email was successfully sent", "email", MB_OK);
					FILE *file = fopen ("report_sent", "wb");
					fclose (file);
				}
				else
				{
					MessageBox (dialog, "Failed to send the email", "email", MB_OK | MB_ICONERROR);
				}
			}
		}*/
	}
	else if (message == WM_CHAR)
	{
		if (wParam == 27)
		{
			// ESC -> ignore
			sendEmailW();
			NeedExit = true;
			Result = ReportIgnore;
		}
	}

	return DefWindowProc (hWnd, message, wParam, lParam);
}

TReportResult report (const std::string &title, const std::string &header, const std::string &subject, const std::string &body, bool enableCheckIgnore, uint debugButton, bool ignoreButton, sint quitButton, bool sendReportButton, bool &ignoreNextTime, const string &attachedFile)
{
	// register the window
	static bool AlreadyRegister = false;
	if(!AlreadyRegister)
	{
		WNDCLASSA wc;
		memset (&wc,0,sizeof(wc));
		wc.style			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc		= (WNDPROC)WndProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= GetModuleHandle(NULL);
		wc.hIcon			= NULL;
		wc.hCursor			= LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)COLOR_WINDOW;
		wc.lpszClassName	= "SIPReportWindow";
		wc.lpszMenuName		= NULL;
		if (!RegisterClassA(&wc)) return ReportError;
		AlreadyRegister = true;
	}

	string formatedTitle = title.empty() ? "SIP report" : title;


	// create the window
	dialog = CreateWindowA ("SIPReportWindow", formatedTitle.c_str(), WS_DLGFRAME | WS_CAPTION /*| WS_THICKFRAME*/, CW_USEDEFAULT, CW_USEDEFAULT, 456, 400,  NULL, NULL, GetModuleHandle(NULL), NULL);

	// create the font
	HFONT font = CreateFontA (-12, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial");

	Subject = subject;
	AttachedFile = attachedFile;

	// create the edit control
	HWND edit = CreateWindowA ("EDIT", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_READONLY | ES_LEFT | ES_MULTILINE, 7, 70, 429, 212, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageA (edit, WM_SETFONT, (LONG) font, TRUE);

	// set the edit text limit to lot of :)
	SendMessageA (edit, EM_LIMITTEXT, ~0U, 0);

	Body = addSlashR (body);

	// set the message in the edit text
	SendMessageA (edit, WM_SETTEXT, (WPARAM)0, (LPARAM)Body.c_str());

	if (enableCheckIgnore)
	{
		// create the combo box control
		checkIgnore = CreateWindowA ("BUTTON", "Don't display this report again", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 7, 290, 429, 18, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
		SendMessageA (checkIgnore, WM_SETFONT, (LONG) font, TRUE);

		if(ignoreNextTime)
		{
			SendMessageA (checkIgnore, BM_SETCHECK, BST_CHECKED, 0);
		}
	}

	// create the debug button control
	debug = CreateWindowA ("BUTTON", "Debug", WS_CHILD | WS_VISIBLE, 7, 315, 75, 25, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageA (debug, WM_SETFONT, (LONG) font, TRUE);

	if (debugButton == 0)
		EnableWindow(debug, FALSE);

	// create the ignore button control
	ignore = CreateWindowA ("BUTTON", "Ignore", WS_CHILD | WS_VISIBLE, 75+7+7, 315, 75, 25, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageA (ignore, WM_SETFONT, (LONG) font, TRUE);

	if (ignoreButton == 0)
		EnableWindow(ignore, FALSE);

	// create the quit button control
	quit = CreateWindowA ("BUTTON", "Quit", WS_CHILD | WS_VISIBLE, 75+75+7+7+7, 315, 75, 25, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageA (quit, WM_SETFONT, (LONG) font, TRUE);

	if (quitButton == 0)
		EnableWindow(quit, FALSE);

	// create the debug button control
	sendReport = CreateWindowA ("BUTTON", "Don't send the report", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 7, 315+32, 429, 18, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageA (sendReport, WM_SETFONT, (LONG) font, TRUE);

	string formatedHeader;
	if (header.empty())
	{
		formatedHeader = "This application stopped to display this report.";
	}
	else
	{
		formatedHeader = header;
	}

	// ace don't do that because it s slow to try to send a mail
	//CanSendMailReport  = sendReportButton && EmailFunction != NULL && EmailFunction("", "", "", "", "", true);
	CanSendMailReport  = sendReportButton && EmailFunction != NULL;

	if (CanSendMailReport)
		formatedHeader += " Send report will only email the contents of the box below. Please, send it to help us (it could take few minutes to send the email, be patient).";
	else
		EnableWindow(sendReport, FALSE);


	// create the label control
	HWND label = CreateWindowA ("STATIC", formatedHeader.c_str(), WS_CHILD | WS_VISIBLE /*| SS_WHITERECT*/, 7, 7, 429, 51, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageA (label, WM_SETFONT, (LONG) font, TRUE);


	DebugDefaultBehavior = debugButton==1;
	QuitDefaultBehavior = quitButton==1;

	IgnoreNextTime = ignoreNextTime;

	// show until the cursor really show :)
	while (ShowCursor(TRUE) < 0) {};

	SetWindowPos (dialog, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	
	SetFocus(dialog);
	SetForegroundWindow(dialog);

	NeedExit = false;

	while(!NeedExit)
	{
		MSG	msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		sipSleep (1);
	}

	// set the user result
	ignoreNextTime = IgnoreNextTime;

	ShowWindow(dialog, SW_HIDE);

	DELETE_OBJECT(sendReport)
	DELETE_OBJECT(quit)
	DELETE_OBJECT(ignore)
	DELETE_OBJECT(debug)
	DELETE_OBJECT(checkIgnore)
	DELETE_OBJECT(edit)
	DELETE_OBJECT(label)
	DELETE_OBJECT(dialog)

	return Result;
}

TReportResult reportW(const std::basic_string<ucchar> &title, const std::basic_string<ucchar> &header, const std::basic_string<ucchar> &subject, const std::basic_string<ucchar> &body, bool enableCheckIgnore, uint debugButton, bool ignoreButton, sint quitButton, bool sendReportButton, bool &ignoreNextTime, const std::basic_string<ucchar> &attachedFile)
{
	// register the window
	static bool AlreadyRegister = false;
	if(!AlreadyRegister)
	{
		WNDCLASSW wc;
		memset (&wc,0,sizeof(wc));
		wc.style			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc		= (WNDPROC)WndProcW;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= GetModuleHandle(NULL);
		wc.hIcon			= NULL;
		wc.hCursor			= LoadCursor(NULL,IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)COLOR_WINDOW;
		wc.lpszClassName	= L"SIPReportWindow";
		wc.lpszMenuName		= NULL;
		if (!RegisterClassW(&wc)) return ReportError;
		AlreadyRegister = true;
	}

	std::basic_string<ucchar> formatedTitle = title.empty() ? L"SIP report" : title;


	// create the window
	dialog = CreateWindowW (L"SIPReportWindow", formatedTitle.c_str(), WS_DLGFRAME | WS_CAPTION /*| WS_THICKFRAME*/, CW_USEDEFAULT, CW_USEDEFAULT, 456, 400,  NULL, NULL, GetModuleHandle(NULL), NULL);

	// create the font
	HFONT font = CreateFontW (-12, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Arial");

	SubjectW = subject;
	AttachedFileW = attachedFile;

	// create the edit control
	HWND edit = CreateWindowW (L"EDIT", NULL, WS_BORDER | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | ES_READONLY | ES_LEFT | ES_MULTILINE, 7, 70, 429, 212, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageW (edit, WM_SETFONT, (LONG) font, TRUE);

	// set the edit text limit to lot of :)
	SendMessageW (edit, EM_LIMITTEXT, ~0U, 0);

	BodyW = addSlashR_W (body);

	// set the message in the edit text
	SendMessageW (edit, WM_SETTEXT, (WPARAM)0, (LPARAM)BodyW.c_str());

	if (enableCheckIgnore)
	{
		// create the combo box control
		checkIgnore = CreateWindowW (L"BUTTON", L"Don't display this report again", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | BS_CHECKBOX, 7, 290, 429, 18, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
		SendMessageW (checkIgnore, WM_SETFONT, (LONG) font, TRUE);

		if (ignoreNextTime)
		{
			SendMessageW (checkIgnore, BM_SETCHECK, BST_CHECKED, 0);
		}
	}

	// create the debug button control
	debug = CreateWindowW (L"BUTTON", L"Debug", WS_CHILD | WS_VISIBLE, 7, 315, 75, 25, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageW (debug, WM_SETFONT, (LONG) font, TRUE);

	if (debugButton == 0)
		EnableWindow(debug, FALSE);

	// create the ignore button control
	ignore = CreateWindowW (L"BUTTON", L"Ignore", WS_CHILD | WS_VISIBLE, 75+7+7, 315, 75, 25, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageW (ignore, WM_SETFONT, (LONG) font, TRUE);

	if (ignoreButton == 0)
		EnableWindow(ignore, FALSE);

	// create the quit button control
	quit = CreateWindowW (L"BUTTON", L"Quit", WS_CHILD | WS_VISIBLE, 75+75+7+7+7, 315, 75, 25, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageW (quit, WM_SETFONT, (LONG) font, TRUE);

	if (quitButton == 0)
		EnableWindow(quit, FALSE);

	// create the debug button control
	sendReport = CreateWindowW (L"BUTTON", L"Don't send the report", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 7, 315+32, 429, 18, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageW (sendReport, WM_SETFONT, (LONG) font, TRUE);

	std::basic_string<ucchar> formatedHeader;
	if (header.empty())
	{
		formatedHeader = L"This application stopped to display this report.";
	}
	else
	{
		formatedHeader = header;
	}

	// ace don't do that because it s slow to try to send a mail
	//CanSendMailReport  = sendReportButton && EmailFunction != NULL && EmailFunction("", "", "", "", "", true);
	CanSendMailReport  = sendReportButton && EmailFunction != NULL;

	if (CanSendMailReport)
		formatedHeader += L" Send report will only email the contents of the box below. Please, send it to help us (it could take few minutes to send the email, be patient).";
	else
		EnableWindow(sendReport, FALSE);


	// create the label control
	HWND label = CreateWindowW (L"STATIC", formatedHeader.c_str(), WS_CHILD | WS_VISIBLE /*| SS_WHITERECT*/, 7, 7, 429, 51, dialog, (HMENU) NULL, (HINSTANCE) GetWindowLong(dialog, GWL_HINSTANCE), NULL);
	SendMessageW (label, WM_SETFONT, (LONG) font, TRUE);


	DebugDefaultBehavior = debugButton==1;
	QuitDefaultBehavior = quitButton==1;

	IgnoreNextTime = ignoreNextTime;

	// show until the cursor really show :)
	while (ShowCursor(TRUE) < 0) {};

	SetWindowPos (dialog, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	
	SetFocus(dialog);
	SetForegroundWindow(dialog);

	NeedExit = false;

	while(!NeedExit)
	{
		MSG	msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		sipSleep (1);
	}

	// set the user result
	ignoreNextTime = IgnoreNextTime;

	ShowWindow(dialog, SW_HIDE);

	DELETE_OBJECT(sendReport)
	DELETE_OBJECT(quit)
	DELETE_OBJECT(ignore)
	DELETE_OBJECT(debug)
	DELETE_OBJECT(checkIgnore)
	DELETE_OBJECT(edit)
	DELETE_OBJECT(label)
	DELETE_OBJECT(dialog)

	return Result;
}

#endif


} // SIPBASE
