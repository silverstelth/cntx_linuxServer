/** \file memory_common.h
 * Common definition for memory project
 *
 * $Id: memory_common.h,v 1.1 2008/06/26 13:41:17 RoMyongGuk Exp $
 */

#ifndef SIP_MEMORY_COMMON_H
#define SIP_MEMORY_COMMON_H

#include "memory/memory_config.h"

#ifndef MEMORY_API
#ifdef MEMORY_EXPORTS
#define MEMORY_API __declspec(dllexport)
#else
#define MEMORY_API __declspec(dllimport)
#endif
#endif

// Do not force MSVC library name
#define _STLP_DONT_FORCE_MSVC_LIB_NAME

#include <cassert>

// Operating systems definition

#ifdef WIN32
#  define SIP_OS_WINDOWS
#  define SIP_LITTLE_ENDIAN
#  define SIP_CPU_INTEL
#  ifdef _DEBUG
#    define SIP_DEBUG
#  else
#    define SIP_RELEASE
#  endif
#else
#  define SIP_OS_UNIX
#  ifdef WORDS_BIGENDIAN
#    define SIP_BIG_ENDIAN
#  else
#    define SIP_LITTLE_ENDIAN
#  endif
#endif

// Remove stupid Visual C++ warning

#ifdef SIP_OS_WINDOWS
#  pragma warning (disable : 4503)			// STL: Decorated name length exceeded, name was truncated
#  pragma warning (disable : 4786)			// STL: too long indentifier
#  pragma warning (disable : 4290)			// throw() not implemented warning
#  pragma warning (disable : 4250)			// inherits via dominance (informational warning).
#endif // SIP_OS_UNIX

#ifdef SIP_OS_WINDOWS

typedef	signed		__int8		sint8;
typedef	unsigned	__int8		uint8;
typedef	signed		__int16		sint16;
typedef	unsigned	__int16		uint16;
typedef	signed		long		sint32;
typedef	unsigned	long		uint32;
typedef	signed		__int64		sint64;
typedef	unsigned	__int64		uint64;

typedef	signed		int			sint;			// at least 32bits (depend of processor)
typedef	unsigned	int			uint;			// at least 32bits (depend of processor)

#define	SIP_I64	\
		"I64"

#elif defined (SIP_OS_UNIX)

#include <sys/types.h>

typedef	int8_t		sint8;
typedef	u_int8_t	uint8;
typedef	int16_t		sint16;
typedef	u_int16_t	uint16;
typedef	int32_t		sint32;
typedef	u_int32_t	uint32;
typedef	int64_t		sint64;
typedef	u_int64_t	uint64;

typedef	signed		int			sint;			// at least 32bits (depend of processor)
typedef	unsigned	int			uint;			// at least 32bits (depend of processor)

#define	SIP_I64	\
		"ll"

#endif // SIP_OS_UNIX

#define memory_assert assert

#endif // SIP_MEMORY_COMMON_H

/* End of memory_mutex.h */
