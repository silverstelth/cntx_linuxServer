/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MISC_SIPCOMMON_H__
#define __MISC_SIPCOMMON_H__

#ifndef _MAX_PATH
#define _MAX_PATH 1024
#endif


#ifdef SIP_OS_WINDOWS
#define _VSNPRINTFL(_cstring, _size, _format, _args) 	_vsnprintf (_cstring, _size, _format, _args)
#define _VSNWPRINTFL(_cstring, _size, _format, _args) 	_vsnwprintf (_cstring, _size, _format, _args)
#define _WTOF_L(floatStr) 								_wtof(floatStr)
#define _VSCPRINTF_L(formatStr, varList)				_vscprintf(formatStr, varList)
#define _VSCWPRINTF_L(formatStr, varList)				_vscwprintf(formatStr, varList)
#define _VSWPRINTF_L(buffer, size, format, varList)		vswprintf(buffer, size, format, varList)

#define GET_WFILENAME(pChar, pWChar)					const ucchar*	pChar = pWChar

//#define TOSTRING()

#else
#define _VSNPRINTFL(_cstring, _size, _format, _args) 	vsnprintf (_cstring, _size, _format, _args)
#define _VSNWPRINTFL(_cstring, _size, _format, _args) 	vswprintf (_cstring, _size, _format, _args)
#define _WTOF_L(floatStr) 								wcstod(floatStr, NULL)
#define _VSCPRINTF_L(formatStr, varList)				vprintf(formatStr, varList)
#define _VSCWPRINTF_L(formatStr, varList)				vwprintf(formatStr, varList)
#define _VSWPRINTF_L(buffer, size, format, varList)		vswprintf(buffer, size, format, varList)

#define GET_WFILENAME(pChar, pWChar)	\
		char	pChar[_MAX_PATH]; \
		sprintf(pChar, "%S", pWChar)
#endif

#endif //__MISC_SIPCOMMON_H__
