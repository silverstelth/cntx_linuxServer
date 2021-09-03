/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SIPSTRINGFUNC_H__
#define __SIPSTRINGFUNC_H__

#ifdef SIP_OS_WINDOWS

#include <windows.h>
#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>			// for String... functions
#include <crtdbg.h>				// for _ASSERTE 

/*!
 * \brief	The namespace of the Sip's library
 *
 * This is the namespace for source codes written by Sip.
 */
namespace SIPBASE {

	/*! \brief	Returns the number of bytes which is required to convert an UNICODE string into an ANSI string */
	HRESULT NeededCharsForUnicode2Ansi (LPCWSTR wszSrc, int * pcbNeeded) ;
	/*! \brief	Converts an UNICODE string into an ANSI string */
	HRESULT Unicode2Ansi (LPSTR szDest, int cbDest, LPCWSTR wszSrc) ;
	/*! \brief	Converts an UNICODE string into an ANSI string */
	HRESULT Unicode2Ansi (LPSTR * pszDest, LPCWSTR wszSrc, HANDLE hHeap) ;

	/*! \brief	Returns the number of bytes which is required to convert an UNICODE string into an UTF-8 string */
	HRESULT NeededCharsForUnicode2UTF8 (LPCWSTR wszSrc, int * pcbNeeded) ;
	/*! \brief	Converts an UNICODE string into an UTF-8 string */
	HRESULT Unicode2UTF8 (LPSTR szDest, int cbDest, LPCWSTR wszSrc) ;
	/*! \brief	Converts an UNICODE string into an UTF-8 string */
    HRESULT Unicode2UTF8 (LPSTR * pszDest, LPCWSTR wszSrc, HANDLE hHeap) ;

	/*! \brief	Returns the number of characters which is required to convert an ANSI string into an UNICODE string */
	HRESULT NeededCharsForAnsi2Unicode (LPCSTR szSrc, int * pcchNeeded) ;
	/*! \brief	Converts an ANSI string into an UNICODE string */
	HRESULT Ansi2Unicode (LPWSTR wszDest, int cchDest, LPCSTR szSrc) ;
	/*! \brief	Converts an ANSI string into an UNICODE string */
	HRESULT Ansi2Unicode (LPWSTR * pwszDest, LPCSTR szSrc, HANDLE hHeap) ;

}

#endif // SIP_OS_WINODWS

#endif // end of guard __SIPSTRINGFUNC_H__
