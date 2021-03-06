/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TYPES_SIP_H__
#define __TYPES_SIP_H__

// Wrapper for the clib time function
#define sip_time time
#define sip_mktime mktime
#define sip_gmtime gmtime
#define sip_localtime localtime
#define sip_difftime difftime

// sipconfig.h inclusion, file generated by autoconf
#ifdef HAVE_SIPCONFIG_H
	#include "sipconfig.h"
#endif // HAVE_SIPCONFIG_H

#ifdef FINAL_VERSION
	// If the FINAL_VERSION is defined externaly, check that the value is 0 or 1
	#if FINAL_VERSION != 1 && FINAL_VERSION != 0
		#error "Bad value for FINAL_VERSION, it must be 0 or 1"
	#endif
#else
	// If you want to compile in final version just put 1 instead of 0
	// WARNING: never comment this #define
	#define FINAL_VERSION 0
#endif // FINAL_VERSION

// Operating systems definition

#ifdef WIN32
#	define SIP_OS_WINDOWS
#	define SIP_LITTLE_ENDIAN
#	define SIP_CPU_INTEL
#   ifndef _WIN32_WINNT
#	define _WIN32_WINNT 0x0400
#   endif
#	if _MSC_VER >= 1400
#		if _MSC_VER >= 1500
#			define SIP_COMP_VC9
#		else
#			define SIP_COMP_VC8
#		endif
#		undef sip_time
#		define sip_time _time32		// use the old 32 bit time function
#		undef sip_mktime
#		define sip_mktime _mktime32	// use the old 32 bit time function
#		undef sip_gmtime
#		define sip_gmtime _gmtime32	// use the old 32 bit time function
#		undef sip_localtime
#		define sip_localtime _localtime32	// use the old 32 bit time function
#		undef sip_difftime
#		define sip_difftime _difftime32	// use the old 32 bit time function
#	elif _MSC_VER >= 1310
#		define SIP_COMP_VC71
#	elif _MSC_VER >= 1300
#		define SIP_COMP_VC7
#	elif _MSC_VER >= 1200
#		define SIP_COMP_VC6
#		define SIP_COMP_NEED_PARAM_ON_METHOD
#	endif
#	ifdef _DEBUG
#		define SIP_DEBUG
#	else
#		ifndef SIP_RELEASE_DEBUG
#			define SIP_RELEASE
#		endif
#	endif
#else //WIN32
#	ifdef __APPLE__
#		define SIP_OS_MAC
#		ifdef __BIG_ENDIAN__
#			define SIP_BIG_ENDIAN
#		elif defined(__LITTLE_ENDIAN__)
#			define SIP_LITTLE_ENDIAN
#		else
#			error "Cannot detect the endianness of this Mac"
#		endif
#	else
#		ifdef WORDS_BIGENDIAN
#			define SIP_BIG_ENDIAN
#		else
#			define SIP_LITTLE_ENDIAN
#		endif
#	endif
// these define are set the linux and mac os
#	define SIP_OS_UNIX
#	define SIP_COMP_GCC
#endif //WIN32

#	if !defined(SIP_DEBUG) && !defined(SIP_RELEASE_DEBUG) && !defined(SIP_RELEASE)
#		ifdef _DEBUG
#			define SIP_DEBUG
#		else
#			ifndef SIP_RELEASE_DEBUG
#				define SIP_RELEASE
#			endif
#		endif
#	endif

// Mode checks: SIP_DEBUG and SIP_DEBUG_FAST are allowed at the same time, but not with any release mode
// (by the way, SIP_RELEASE and SIP_RELEASE_DEBUG are not allowed at the same time, see above)
#if defined (SIP_DEBUG) || defined (SIP_DEBUG_FAST)
#	if defined (SIP_RELEASE) || defined (SIP_RELEASE_DEBUG)
#		error "Error in preprocessor directives for SIP debug mode!"
#	endif
#endif

// gcc 3.4 introduced ISO C++ with tough template rules
//
// SIP_ISO_SYNTAX can be used using #if SIP_ISO_SYNTAX or #if !SIP_ISO_SYNTAX
//
// SIP_ISO_TEMPLATE_SPEC can be used in front of an instanciated class-template member data definition,
// because sometimes MSVC++ 6 produces an error C2908 with a definition with template <>.
#if defined(SIP_OS_WINDOWS) || (defined(__GNUC__) && ((__GNUC__ < 3) || (__GNUC__ == 3 && __GNUC_MINOR__ <= 3)))
#define SIP_ISO_SYNTAX 0
#define SIP_ISO_TEMPLATE_SPEC
#else
#define SIP_ISO_SYNTAX 1
#define SIP_ISO_TEMPLATE_SPEC template <>
#endif

// Remove stupid Visual C++ warnings

#ifdef SIP_OS_WINDOWS
	#pragma warning (disable : 4503)			// STL: Decorated name length exceeded, name was truncated
	#pragma warning (disable : 4786)			// STL: too long identifier
	#pragma warning (disable : 4290)			// throw() not implemented warning
	#pragma warning (disable : 4250)			// inherits via dominance (informational warning).
	#pragma warning (disable : 4390)			// don't warn in empty block "if(exp) ;"
	// Debug : Sept 01 2006
	#if defined(SIP_COMP_VC8) || defined(SIP_COMP_VC9)
		#pragma warning (disable : 4005)			// don't warn on redefenitions caused by xp platform sdk
		#pragma warning (disable : 4996)			// don't warn for deprecated function (sprintf, sscanf in VS8)
		#pragma warning (disable : 4819)			// don't warn when the file contains a character that cannot be represented in the current code page (number).
	#endif // SIP_COMP_VC8 
#endif // SIP_OS_WINDOWS


// Standard include

#include <string>
#include <exception>

// Setup extern asm functions.

#ifndef SIP_NO_ASM							// If SIP_NO_ASM is externely defined, don't override it.
#	ifndef SIP_CPU_INTEL						// If not on an Intel compatible plateforme (BeOS, 0x86 Linux, Windows)
#		define SIP_NO_ASM						// Don't use extern ASM. Full C++ code.
#	endif // SIP_CPU_INTEL
#endif // SIP_NO_ASM


// Define this if you want to use GTK for gtk_displayer

#ifdef SIP_OS_UNIX
#define SIP_USE_GTK
#endif //SIP_OS_UNIX
 // KSR_UNIFY_ADD Must define in config file!
//#undef SIP_USE_GTK


// Standard types

/*
 * correct numeric types:	sint8, uint8, sint16, uint16, sint32, uint32, sint64, uint64, sint, uint
 * correct char types:		char, string, ucchar, ucstring, basic_string<ucchar>, CSString
 * correct misc types:		void, bool, float, double
 *
 */

/**
 * \typedef uint8
 * An unsigned 8 bits integer (use char only as \b character and not as integer)
 **/

/**
 * \typedef sint8
 * An signed 8 bits integer (use char only as \b character and not as integer)
 */

/**
 * \typedef uint16
 * An unsigned 16 bits integer (don't use short)
 **/

/**
 * \typedef sint16
 * An signed 16 bits integer (don't use short)
 */

/**
 * \typedef uint32
 * An unsigned 32 bits integer (don't use int or long)
 **/

/**
 * \typedef sint32
 * An signed 32 bits integer (don't use int or long)
 */

/**
 * \typedef uint64
 * An unsigned 64 bits integer (don't use long long or __int64)
 **/

/**
 * \typedef sint64
 * An signed 64 bits integer (don't use long long or __int64)
 */

/**
 * \typedef uint
 * An unsigned integer, at least 32 bits (used only for interal loops or speedy purpose, processor dependant)
 **/

/**
 * \typedef sint
 * An signed integer at least 32 bits (used only for interal loops or speedy purpose, processor dependant)
 */

/**
 * \def SIP_I64
 * Used to display a int64 in a platform independant way with printf like functions.
 \code
 sint64 myint64 = SINT64_CONSTANT(0x123456781234);
 printf("This is a 64 bits int: %"SIP_I64"u", myint64);
 \endcode
 */

#ifdef SIP_OS_WINDOWS

typedef	signed		__int8		sint8;
typedef	unsigned	__int8		uint8;
typedef	signed		__int16		sint16;
typedef	unsigned	__int16		uint16;
//typedef	signed		__int32		sint32;
//typedef	unsigned	__int32		uint32;
typedef	signed		long		sint32;
typedef	unsigned	long		uint32;
typedef	signed		__int64		sint64;
typedef	unsigned	__int64		uint64;

typedef				int			sint;			// at least 32bits (depend of processor)
typedef	unsigned	int			uint;			// at least 32bits (depend of processor)

#define	SIP_I64 "I64"

#include "SipConfig.h"

#elif defined (SIP_OS_UNIX)

#include <sys/types.h>
#include <memory>
#include <unordered_map>
#include <unordered_set>

typedef	int8_t		sint8;
typedef	u_int8_t	uint8;
typedef	int16_t		sint16;
typedef	u_int16_t	uint16;
typedef	int32_t		sint32;
typedef	u_int32_t	uint32;
typedef	long long int		sint64;
typedef	unsigned long long int	uint64;
typedef unsigned long long ULONG_PTR;

typedef			int			sint;			// at least 32bits (depend of processor)
typedef	unsigned	int			uint;			// at least 32bits (depend of processor)

#define INVALID_HANDLE_VALUE	((void*)(sint64)-1)

#define	SIP_I64	\
		"ll"

#endif // SIP_OS_UNIX

#if (defined(SIP_COMP_VC71) || defined(SIP_COMP_VC8) || defined(SIP_COMP_VC9)) && !defined(_STLPORT_VERSION)
#	define HashMap		stdext::hash_map
#	define HashSet		stdext::hash_set
#	define HashMultiMap	stdext::hash_multimap
#else
#	define HashMap		std::unordered_map
#	define HashSet		std::unordered_set
#	define HashMultiMap	std::unordered_multimap
#endif

#ifdef SIP_OS_UNIX
#	define autoPtr 		std::unique_ptr
#else
#	define autoPtr 		std::auto_ptr
#endif

/**
 * \typedef ucchar
 * An unicode character (16 bits)
 */
//typedef	uint16	ucchar;
// modified by lci 09-05-09
typedef	wchar_t ucchar;

// -----used only in 3D engine----
typedef bool				bool8;
typedef float				float32;
typedef double				float64;

typedef char				char8;
typedef ucchar				char16;

typedef char*				lpstr;
typedef ucchar*				lpwstr;
typedef const char*			lpcstr;
typedef const ucchar*		lpcwstr;
typedef void *				lpvoid;
typedef const void *		lpcvoid;

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }

#define In
#define Out
#define Inout

// -------------------------------

// To define a 64bits constant; ie: UINT64_CONSTANT(0x123456781234)
#ifdef SIP_OS_WINDOWS
#  if defined(SIP_COMP_VC8) || defined(SIP_COMP_VC9)
#    define INT64_CONSTANT(c)	(c##LL)
#    define SINT64_CONSTANT(c)	(c##LL)
#    define UINT64_CONSTANT(c)	(c##LL)
#  else
#    define INT64_CONSTANT(c)	(c)
#    define SINT64_CONSTANT(c)	(c)
#    define UINT64_CONSTANT(c)	(c)
#  endif
#else
#  define INT64_CONSTANT(c)		(c##LL)
#  define SINT64_CONSTANT(c)	(c##LL)
#  define UINT64_CONSTANT(c)	(c##ULL)
#endif

// Fake "for" to be conform with ANSI "for scope" on Windows compiler older than Visual Studio 8
// On Visual Studio 8, the for is conform with ANSI, no need to define this macro in this case
#if defined(SIP_OS_WINDOWS) && !defined(SIP_EXTENDED_FOR_SCOPE) && !defined(SIP_COMP_VC8) && !defined(SIP_COMP_VC9)
#  define for if(false) {} else for
#endif

// Define a macro to write template function according to compiler weaknes
#ifdef SIP_COMP_NEED_PARAM_ON_METHOD
 #define SIP_TMPL_PARAM_ON_METHOD_1(p1)	<p1>
 #define SIP_TMPL_PARAM_ON_METHOD_2(p1, p2)	<p1, p2>
#else
 #define SIP_TMPL_PARAM_ON_METHOD_1(p1)
 #define SIP_TMPL_PARAM_ON_METHOD_2(p1, p2)	
#endif

#define SIP_USE_UNICODE

#ifdef SIP_USE_UNICODE
#	define UNICODE
#	define IDisplayer IDisplayerW
#	define CStdDisplayer CStdDisplayerW
#	define CFileDisplayer CFileDisplayerW
#	define CMsgBoxDisplayer CMsgBoxDisplayerW
#	define CMemDisplayer CMemDisplayerW
#	define CLightMemDisplayer CLightMemDisplayerW
#	define CWinDisplayer CWinDisplayerW
#	define CWindowDisplayer CWindowDisplayerW
#	define CGtkDisplayer CGtkDisplayerW
#	define CNetDisplayer CNetDisplayerW
#	define CLog CLogW
#	define sipFatalError	sipFatalErrorW
#	define sipError			sipErrorW
#else
#	define IDisplayer IDisplayerA
#	define CStdDisplayer CStdDisplayerA
#	define CFileDisplayer CFileDisplayerA
#	define CMsgBoxDisplayer CMsgBoxDisplayerA
#	define CMemDisplayer CMemDisplayerA
#	define CLightMemDisplayer CLightMemDisplayerA
#	define CWinDisplayer CWinDisplayerA
#	define CWindowDisplayer CWindowDisplayerA
#	define CGtkDisplayer CGtkDisplayerA
#	define CNetDisplayer CNetDisplayerA
#	define CLog CLogA
#	define sipFatalError	sipFatalErrorA
#	define sipError			sipErrorA
#endif

// #if defined (_UNICODE)
#ifdef SIP_USE_UNICODE
	typedef	ucchar			tchar;
	typedef	ucchar			*lptstr;
	typedef const ucchar		*lpctstr;
#	define _tcsicmp			wcscmp
#	define CSString			CSStringW
#	define CVectorSString		CVectorSStringW;
#	define tstring			std::basic_string<ucchar>
#	define _t(x)			L ## x
#else
	typedef char			tchar;
	typedef char			*lptstr;
	typedef const char		*lpctstr;
#	define _tcsicmp			strcmp
#	define CSString			CSStringA
#	define CVectorSString		CVectorSStringA;
#	define tstring			std::string
#	define _t(x)			x
#endif

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

/**
 * Force the use of SIP memory manager
 */
#include "../memory/memory_manager.h"
#include "misc_sipcommon.h"
#endif // __TYPES_SIP_H__
