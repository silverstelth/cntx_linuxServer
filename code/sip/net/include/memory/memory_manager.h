/** \file memory_manager.h
 * A new memory manager
 *
 * $Id: memory_manager.h,v 1.4 2010/02/18 08:38:44 KimSuRyon Exp $
 */

#ifndef SIP_MEMORY_MANAGER_H
#define SIP_MEMORY_MANAGER_H

#include "misc/types_sip.h"

// CONFIGURATION
#include "memory/memory_config.h"

#ifdef _STLPORT_VERSION
#	if _STLPORT_VERSION >= 0x510
		#include <stl/config/user_config.h>
#	else
		#include <stl/_site_config.h>
#	endif // _STLPORT_VERSION
#endif

/*	Doc:
	----

	// Allocate memory
	void*		MemoryAllocate (unsigned int size);

	// Allocate debug memory : specify a filename, the line and the category of allocation (7 letters max)
	void*		MemoryAllocateDebug (unsigned int size, const char *filename, unsigned int line, const char *category);

	// Deallocation
	void		MemoryDeallocate (void *p);

	// Returns the amount of allocated memory
	// Works only with sip memory allocator
	unsigned int GetAllocatedMemory ();

	// Returns the amount of free memory inside the allocator. This is not the amount of free system memory.
	// Works only with sip memory allocator
	unsigned int GetFreeMemory ();

	// Returns the total of memory used by the allocator
	// Works only with sip memory allocator
	unsigned int GetTotalMemoryUsed ();

	// Returns the amount of memory used by debug purpose
	// Works only with sip memory allocator
	unsigned int GetDebugInfoSize ();

	// Returns the amount of memory used by a category
	// Works only with sip memory allocator with debug features activated
	unsigned int GetAllocatedMemoryByCategory (const char *category);

	// Returns the size of a memory block
	// Works only with sip memory allocator
	unsigned int GetBlockSize (void *pointer);

	// Returns the fragentation ratio. 0 : no fragmentation, 1 : heavy fragmentation
	// Works only with sip memory allocator
	float		GetFragmentationRatio ();

	// Returns the amount of system memory allocated by the allocator tracing the process heap
	// Works only with sip memory allocator
	unsigned int GetAllocatedSystemMemoryByAllocator ();

	// Returns the amount of system memory allocated tracing the process heap
	// Works only with sip memory allocator
	unsigned int GetAllocatedSystemMemory ();
	
	// Get the amount of system memory allocated since the dll as been loaded. Use memory hook, available only in _DEBUG mode.
	// Works only with sip memory allocator
	unsigned int GetAllocatedSystemMemoryHook ();
	  
	// Check the heap
	bool		CheckHeap (bool stopOnError);
	  
	// Check block by size
	// Works only with sip memory allocator with debug features activated
	bool		CheckHeapBySize (bool stopOnError, unsigned int blockSize);

	// Make a statistic report
	// Works only with sip memory allocator with debug features activated
	bool		StatisticsReport (const char *filename, bool memoryDump);

	// Report memory leaks in debug window
	// Works with standard memory allocator in _DEBUG and with sip memory allocator with debug features activated
	bool		ReportMemoryLeak ();

	// Set the flag to check memory at each allocation / deallocation (warning, very slow)
	// Works with standard memory allocator in _DEBUG and with sip memory allocator with debug features activated
	void		AlwaysCheckMemory(bool alwaysCheck);

	// Get the flag
	// Works with standard memory allocator in _DEBUG and with sip memory allocator with debug features activated
	bool		IsAlwaysCheckMemory();

	// Set a out of memory callback
	// Works only with sip memory allocator
	void		SetOutOfMemoryHook(void (*outOfMemoryCallback)());

	 * Start/End the memory allocation loging
	 * Works only with sip memory allocator with debug features activated
	 * blockSize is the size of the block to log
	 *
	 * Log format : 
	 * 4 bytes : uint32 size of the logged blocks.
	 * n bytes : log entries
	 *
	 * Log entry format :
	 * 4 bytes : start address of the bloc data
	 * \0 terminated string : context where the bloc has been allocated
	 *
	bool		StartAllocationLog (const char *filename, uint blockSize);
	bool		EndAllocationLog ();
*/

// Memory debug for windows
#ifdef WIN32
#include <crtdbg.h>
#endif // WIN32

#undef MEMORY_API
#ifdef MEMORY_EXPORTS
 #ifdef WIN32
  #define MEMORY_API __declspec(dllexport)
 #else 
  #define MEMORY_API
 #endif // WIN32
#else
 #ifdef WIN32
  #define MEMORY_API __declspec(dllimport)
 #else 
  #define MEMORY_API
 #endif // WIN32
#endif

#ifdef __cplusplus

/* Define this to use the default memory allocator for new, delete and STL allocator. It is defined only if _STLP_USE_SIP
 * is not defined in the STlport site config file "stl/config/user_config.h". To use the sip memory manager, you need a modified 
 * version of STLport */

/* To active sip allocator in STLport, add the following code at the end of "stl/config/user_config.h" and rebuild STLPort */

#if 0
// ********* Cut here
/*
 * Set _STLP_SIP_ALLOC to use sip allocator that perform memory debugging,
 * such as padding/checking for memory consistency, a lot of debug features
 * and new/stl allocation statistics. 
 * Changing default will require you to rebuild STLPort library !
 *
 * To build STLPort library with sip allocator, you should have build
 * all the sip memory libraries.
 *
 * When you rebuild STLPort library, you must add to the environnement
 * variable the path of the sip library in the command promp with the
 * commands:
 * SET LIB=%LIB%;YOUR_SIP_LIB_DIRECTORY
 * SET INCLUDE=%INCLUDE%;YOUR_SIP_INCLUDE_DIRECTORY
 *
 * Most of the time YOUR_SIP_LIB_DIRECTORY is r:\code\sip\lib
 * Most of the time YOUR_SIP_INCLUDE_DIRECTORY is r:\code\sip\include
 *
 * Only tested under Visual 6
 */
#define   _STLP_USE_SIP 1

#ifdef _STLP_USE_SIP
# if !defined (MEMORY_EXPORTS)
#  include "memory/memory_manager.h"
# endif
# undef new
# define _STLP_USE_NEWALLOC 1
# undef  _STLP_USE_MALLOC
# undef  _STLP_DEBUG_ALLOC
# if defined (WIN32) && ! defined (MEMORY_EXPORTS)
#  if defined (NDEBUG)
#   pragma message("Using SIP memory manager in Release")
#   pragma comment( lib, "sip_memory_rd" )
#  elif defined (_STLP_DEBUG)
#   pragma message("Using SIP memory manager in STLDebug")
#   pragma comment( lib, "sip_memory_d" )
#  else
#   pragma message("Using SIP memory manager in Debug")
#   pragma comment( lib, "sip_memory_df" )
#  endif
# endif // WIN32
#endif // _STLP_USE_SIP

// ********* Cut here
#endif // 0


#if 0
/*
 * To use sip Memory with STLport 4.5.3/4.6 for GCC, you will have to patch STLport a bit more than for VC6.
 *
 * To build STLPort library with sip allocator, you should have build
 * all the sip memory libraries.
 *
 * Only tested under c++ (GCC) 3.2.3 (Debian)
 */

/* This tells STLport to perform allocations only through new operator, without any inner debug allocation (sip memory does it) */
// ********* Patch in "stl/_site_config.h, l. 104 - insert following block
#define _STLP_USE_SIP 1
#define _STLP_USE_NEWALLOC 1
#undef  _STLP_USE_MALLOC
#undef  _STLP_DEBUG_ALLOC
// ********* Cut here

/* If using sip, read native new header, then overload them with sip memory */
// ********* Patch in STLport 4.5.3 & 4.6 new, l. 35 - replace #if .. #endif by following block
# if defined (_STLP_USE_SIP)
#  include _STLP_NATIVE_CPP_RUNTIME_HEADER(new)
#  include "memory/memory_manager.h"
#else
# if !defined (_STLP_NO_NEW_NEW_HEADER)
#   include _STLP_NATIVE_CPP_RUNTIME_HEADER(new)
#  else
#   include  <new.h>
# endif
#endif
// ********* Cut here

/* Force STLport to use sip memory functions, as GCC seems not to handle complex new operator use */
// ********* Patch in STLport 4.6 stl/_new.h, l. 86 or STLport 4.5.3 new, l. 97 - insert following block
//#elif defined(_STLP_USE_SIP) <-- remove this comment and comment at beginning of line !!!!
inline void*  _STLP_CALL __stl_new(size_t __n)   { _STLP_CHECK_NULL_ALLOC(SIPMEMORY::MemoryAllocateDebug(__n, __FILE__, __LINE__, "STL")); }
inline void   _STLP_CALL __stl_delete(void* __p) { SIPMEMORY::MemoryDeallocate(__p); }
// ********* Cut here


/* Placement new is not handled right with sip memory, work around so STLport uses vendor placement new.
 * Actually, this undefs new (as being SIP_NEW), then uses placement new, then defines new with appropriate value (SIP_NEW or DEBUG_NEW). */
// ********* Patch in STLport 4.6 stl/_locale.h, l. 127 - replace line with following block
#   if defined (new)
#     define _STLP_NEW_REDEFINE new
#     undef new
#   endif 
      _STLP_PLACEMENT_NEW (this) locale(__loc._M_impl, __f != 0);
#   if defined(_STLP_NEW_REDEFINE)
#     ifdef _STLP_USE_SIP
#     define new SIP_NEW
#   elif DEBUG_NEW
#     define new DEBUG_NEW
#   endif
#     undef _STLP_NEW_REDEFINE
#   endif 
// ********* Cut here


/* Placement new is not handled right with sip memory, work around so STLport uses vendor placement new.
 * Actually, this undefs new (as being SIP_NEW), then uses placement new, then defines new with appropriate value (SIP_NEW or DEBUG_NEW). */
// ********* Patch in STLport 4.6 stl/_construct.h, l. 113 - replace #if ... #endif with following block
# if defined(_STLP_NEW_REDEFINE)
# ifdef _STLP_USE_SIP
#  define new SIP_NEW
# elif DEBUG_NEW
#  define new DEBUG_NEW
# endif
#  undef _STLP_NEW_REDEFINE
# endif 
// ********* Cut here

// ********* Patch in STLport 4.6 stl/_pthread_alloc.h, l. 52 - insert following block
#   if defined (new)
#     define _STLP_NEW_REDEFINE new
#     undef new
#   endif 
// ********* Cut here

// ********* Patch in STLport 4.6 stl/_pthread_alloc.h, l. 486 - insert following block
#   if defined(_STLP_NEW_REDEFINE)
#     ifdef _STLP_USE_SIP
#     define new SIP_NEW
#   elif DEBUG_NEW
#     define new DEBUG_NEW
#   endif
#     undef _STLP_NEW_REDEFINE
#   endif 
// ********* Cut here

// ********* Patch in STLport 4.6 src/iostream.cpp, l. 37 - insert following block
#   if defined (new)
#     define _STLP_NEW_REDEFINE new
#     undef new
#   endif 
// ********* Cut here

// ********* Patch in STLport 4.6 src/iostream.cpp, l. 397 - insert following block
#   if defined(_STLP_NEW_REDEFINE)
#     ifdef _STLP_USE_SIP
#     define new SIP_NEW
#   elif DEBUG_NEW
#     define new DEBUG_NEW
#   endif
#     undef _STLP_NEW_REDEFINE
#   endif 
// ********* Cut here

// ********* Patch in STLport 4.6 src/locale_impl.cpp, l. 35 - insert following block
#   if defined (new)
#     define _STLP_NEW_REDEFINE new
#     undef new
#   endif 
// ********* Cut here

// ********* Patch in STLport 4.6 src/locale_impl.cpp, l. 485 - insert following block
#   if defined(_STLP_NEW_REDEFINE)
#     ifdef _STLP_USE_SIP
#     define new SIP_NEW
#   elif DEBUG_NEW
#     define new DEBUG_NEW
#   endif
#     undef _STLP_NEW_REDEFINE
#   endif 
// ********* Cut here


#endif // 0


#ifndef _STLP_USE_SIP
#define SIP_USE_DEFAULT_MEMORY_MANAGER
#endif // _STLP_USE_SIP


// *********************************************************
#ifdef SIP_USE_DEFAULT_MEMORY_MANAGER
// *********************************************************

// Malloc
#ifdef SIP_OS_MAC
#include <sys/malloc.h>
#else	//SIP_OS_MAC
#include <malloc.h>
#endif

#ifdef SIP_OS_UNIX
#include <new>
#endif // SIP_OS_UNIX

namespace SIPMEMORY
{

inline void*		MemoryAllocate (unsigned int size) {return malloc(size);}
inline void*		MemoryAllocateDebug (unsigned int size, const char *filename, unsigned int line, const char *category)
{
#ifdef WIN32
	return _malloc_dbg(size, _NORMAL_BLOCK, filename, line);
#else // WIN32
	return malloc(size);
#endif // WIN32
}
inline void			MemoryDeallocate (void *p)
{
	free(p);
}
inline void*		MemoryReallocate (void *p, unsigned int size) {return realloc(p, size);}
inline unsigned int GetAllocatedMemory () { return 0;}
inline unsigned int GetFreeMemory () { return 0;}
inline unsigned int GetTotalMemoryUsed () { return 0;}
inline unsigned int GetDebugInfoSize () { return 0;}
inline unsigned int GetAllocatedMemoryByCategory (const char *category) { return 0;}
inline bool			StartAllocationLog (const char *category, unsigned int blockSize) { return false;}
inline bool			EndAllocationLog () { return false;}
inline unsigned int GetBlockSize (void *pointer) { return 0;}
inline bool			CheckHeapBySize (bool stopOnError, unsigned int blockSize) { return true; }
inline const char * GetCategory (void *pointer) { return 0; }
inline float		GetFragmentationRatio () { return 0.0f;}
inline unsigned int GetAllocatedSystemMemoryByAllocator () { return 0;}
inline unsigned int GetAllocatedSystemMemory () { return 0;}
inline unsigned int GetAllocatedSystemMemoryHook () { return 0; }
inline void			ReportMemoryLeak() 
{
#ifdef WIN32
	_CrtDumpMemoryLeaks();
#endif // WIN32
}
inline bool			CheckHeap (bool stopOnError) 
{ 
#ifdef WIN32
	if (!_CrtCheckMemory())
	{
		if (stopOnError)
			// ***************
			// Invalide heap !
			// ***************
			_ASSERT(0);
		return false;
	}
#endif // WIN32
	return true;
}
inline void			AlwaysCheckMemory(bool alwaysCheck)
{
#ifdef WIN32
	int previous = _CrtSetDbgFlag (0);
	_CrtSetDbgFlag (alwaysCheck?(previous|_CRTDBG_CHECK_ALWAYS_DF):(previous&(~_CRTDBG_CHECK_ALWAYS_DF)));
#endif // WIN32
}
inline bool			IsAlwaysCheckMemory()
{
#ifdef WIN32
	int previous = _CrtSetDbgFlag (0);
	_CrtSetDbgFlag (previous);
	return (previous&_CRTDBG_CHECK_ALWAYS_DF) != 0;
#else // WIN32
	return false;
#endif // WIN32
}
inline bool			StatisticsReport (const char *filename, bool memoryDump) { return true; }
inline void			SetOutOfMemoryHook (void (*outOfMemoryCallback)()) { }

}

// *********************************************************
#else // SIP_USE_DEFAULT_MEMORY_MANAGER
// *********************************************************

namespace SIPMEMORY
{

MEMORY_API void*		MemoryAllocate (unsigned int size);
MEMORY_API void*		MemoryAllocateDebug (unsigned int size, const char *filename, unsigned int line, const char *category);
MEMORY_API void*		MemoryReallocate (void *p, unsigned int size);
MEMORY_API void			MemoryDeallocate (void *p);
MEMORY_API unsigned int GetAllocatedMemory ();
MEMORY_API unsigned int GetFreeMemory ();
MEMORY_API unsigned int GetTotalMemoryUsed ();
MEMORY_API unsigned int GetDebugInfoSize ();
MEMORY_API unsigned int GetAllocatedMemoryByCategory (const char *category);
MEMORY_API unsigned int GetBlockSize (void *pointer);
MEMORY_API const char * GetCategory (void *pointer);
MEMORY_API float		GetFragmentationRatio ();
MEMORY_API unsigned int GetAllocatedSystemMemoryByAllocator ();
MEMORY_API unsigned int GetAllocatedSystemMemory ();
MEMORY_API unsigned int GetAllocatedSystemMemoryHook ();
MEMORY_API bool			CheckHeap (bool stopOnError);
MEMORY_API bool			CheckHeapBySize (bool stopOnError, unsigned int blockSize);
MEMORY_API bool			StatisticsReport (const char *filename, bool memoryDump);
MEMORY_API void			ReportMemoryLeak ();
MEMORY_API void			AlwaysCheckMemory(bool alwaysCheck);
MEMORY_API bool			IsAlwaysCheckMemory();
MEMORY_API void			SetOutOfMemoryHook (void (*outOfMemoryCallback)());
MEMORY_API bool			StartAllocationLog (const char *category, unsigned int blockSize);
MEMORY_API bool			EndAllocationLog ();

 /* Allocation context
 */
 class CAllocContext
 {
 public:
	 MEMORY_API CAllocContext::CAllocContext (const char* str);
	 MEMORY_API CAllocContext::~CAllocContext ();
 };
}

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

	MEMORY_API void*		SipMemoryAllocate (unsigned int size);
	MEMORY_API void*		SipMemoryAllocateDebug (unsigned int size, const char *filename, unsigned int line, const char *category);
	MEMORY_API void			SipMemoryDeallocate (void *pointer);

#ifdef __cplusplus
}
#endif // __cplusplus

// *********************************************************
#endif // SIP_USE_DEFAULT_MEMORY_MANAGER
// *********************************************************

// *********************************************************
// New operators
// *********************************************************

#ifndef SIP_USE_DEFAULT_MEMORY_MANAGER

#ifdef SIP_OS_UNIX
#include <new>
#define SIP_MEMORY_THROW_BAD_ALLOC throw (std::bad_alloc)
#define SIP_MEMORY_THROW throw ()
#else
#define SIP_MEMORY_THROW_BAD_ALLOC
#define SIP_MEMORY_THROW
#endif

inline void* operator new(unsigned int size, const char *filename, int line)
{
	return SIPMEMORY::MemoryAllocateDebug (size, filename, line, 0);
}

// *********************************************************

inline void* operator new(unsigned int size) SIP_MEMORY_THROW_BAD_ALLOC
{
	return SIPMEMORY::MemoryAllocateDebug (size, "::new (unsigned int size) operator", 0, 0);
}

// *********************************************************

inline void* operator new[](unsigned int size, const char *filename, int line)
{
	return SIPMEMORY::MemoryAllocateDebug (size, filename, line, 0);
}

// *********************************************************

inline void* operator new[](unsigned int size) SIP_MEMORY_THROW_BAD_ALLOC
{
	return SIPMEMORY::MemoryAllocateDebug (size, "::new (unsigned int size) operator", 0, 0);
}

// *********************************************************

inline void operator delete(void* p) SIP_MEMORY_THROW
{
	SIPMEMORY::MemoryDeallocate (p);
}

// *********************************************************

inline void operator delete(void* p, const char *filename, int line)
{
	SIPMEMORY::MemoryDeallocate (p);
}

// *********************************************************

inline void operator delete[](void* p) SIP_MEMORY_THROW
{
	SIPMEMORY::MemoryDeallocate (p);
}

// *********************************************************

inline void operator delete[](void* p, const char *filename, int line)
{
	SIPMEMORY::MemoryDeallocate (p);
}

#endif // SIP_USE_DEFAULT_MEMORY_MANAGER

// *********************************************************
// Macros
// *********************************************************

/* New operator in debug mode
 * Doesn't work with placement new. To do a placement new, undef new, make your placement new, and redefine new with the macro SIP_NEW */


#if !defined (SIP_USE_DEFAULT_MEMORY_MANAGER) && !defined (SIP_NO_DEFINE_NEW)
	#define SIP_NEW new(__FILE__, __LINE__)
	#define new SIP_NEW
#else
	#define SIP_NEW new
#endif


// Use allocation context ?
#define SIP_ALLOC_CONTEXT(str) ((void)0);

#ifndef SIP_USE_DEFAULT_MEMORY_MANAGER
 #ifndef SIP_HEAP_ALLOCATION_NDEBUG
  #undef SIP_ALLOC_CONTEXT
  #define SIP_ALLOC_CONTEXT(__name) SIPMEMORY::CAllocContext _##__name##MemAllocContext##__name (#__name);
 #endif // SIP_HEAP_ALLOCATION_NDEBUG
#endif // SIP_USE_DEFAULT_MEMORY_MANAGER


namespace SIPMEMORY
{
	class CHeapAllocator;
};

#endif // __cplusplus

#endif // SIP_MEMORY_MANAGER_H
