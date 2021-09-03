/** \file memory_manager.cpp
 * A new memory manager
 *
 * $Id: memory_manager.cpp,v 1.1 2008/06/26 13:41:17 RoMyongGuk Exp $
 */

// Include STLPort first
//#include <stl/_site_config.h>
#include "memory_common.h"
#include "memory/memory_manager.h"
#include "heap_allocator.h"
#include <memory.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif // SIP_OS_WINDOWS

#undef new

// *********************************************************

// Exported functions

namespace SIPMEMORY
{

// Global allocator
CHeapAllocator *GlobalHeapAllocator = NULL;
sint32			GlobalHeapAllocatorSystemAllocated = 0;

// *********************************************************

inline CHeapAllocator*	getGlobalHeapAllocator()
{
#ifndef SIP_OS_WINDOWS
	// check GlobalHeapAllocator is allocated...
	if (GlobalHeapAllocator == NULL)
	{
		GlobalHeapAllocator = (CHeapAllocator*)malloc (sizeof (CHeapAllocator));
		new (GlobalHeapAllocator) CHeapAllocator (1024*1024*10, 1);
	}
#endif
	// under windows, SIP memory is used as a DLL, and global heap allocator is allocated in dll_main
	return GlobalHeapAllocator;
}

#ifndef SIP_USE_DEFAULT_MEMORY_MANAGER

#ifdef SIP_HEAP_ALLOCATION_NDEBUG

MEMORY_API void* MemoryAllocate (unsigned int size)
{
	return getGlobalHeapAllocator()->allocate (size);
}

MEMORY_API void* MemoryAllocateDebug (uint size, const char *filename, uint line, const char *category)
{
	return getGlobalHeapAllocator()->allocate (size);
}

#else // SIP_HEAP_ALLOCATION_NDEBUG

MEMORY_API void* MemoryAllocate (unsigned int size)
{
	return getGlobalHeapAllocator()->allocate (size, "", 0, 0);
}

MEMORY_API void* MemoryAllocateDebug (uint size, const char *filename, uint line, const char *category)
{
	return getGlobalHeapAllocator()->allocate (size, filename, line, category);
}

#endif // SIP_HEAP_ALLOCATION_NDEBUG

MEMORY_API unsigned int GetAllocatedMemory ()
{
	return getGlobalHeapAllocator()->getAllocatedMemory ();
}

// *********************************************************

MEMORY_API unsigned int GetFreeMemory ()
{
	return getGlobalHeapAllocator()->getFreeMemory ();
}

// *********************************************************

MEMORY_API unsigned int GetTotalMemoryUsed ()
{
	return getGlobalHeapAllocator()->getTotalMemoryUsed ();
}

// *********************************************************

MEMORY_API unsigned int GetDebugInfoSize ()
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return getGlobalHeapAllocator()->debugGetDebugInfoSize ();
#else // SIP_HEAP_ALLOCATION_NDEBUG
	return 0;
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

// *********************************************************

MEMORY_API unsigned int GetAllocatedMemoryByCategory (const char *category)
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return getGlobalHeapAllocator()->debugGetAllocatedMemoryByCategory (category);
#else // SIP_HEAP_ALLOCATION_NDEBUG
	return 0;
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

// *********************************************************

MEMORY_API bool StartAllocationLog (const char *filename, uint blockSize)
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return getGlobalHeapAllocator()->debugStartAllocationLog (filename, blockSize);
#else // SIP_HEAP_ALLOCATION_NDEBUG
	return false;
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

// *********************************************************

MEMORY_API bool EndAllocationLog ()
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return getGlobalHeapAllocator()->debugEndAllocationLog ();
#else // SIP_HEAP_ALLOCATION_NDEBUG
	return false;
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

// *********************************************************

MEMORY_API float GetFragmentationRatio ()
{
	return getGlobalHeapAllocator()->getFragmentationRatio ();
}

// *********************************************************

MEMORY_API unsigned int GetAllocatedSystemMemoryByAllocator ()
{
	return getGlobalHeapAllocator()->getAllocatedSystemMemoryByAllocator ();
}

// *********************************************************

MEMORY_API unsigned int GetAllocatedSystemMemory ()
{
	return getGlobalHeapAllocator()->getAllocatedSystemMemory ();
}

// *********************************************************

MEMORY_API unsigned int GetAllocatedSystemMemoryHook ()
{
	return GlobalHeapAllocatorSystemAllocated;
}

// *********************************************************

MEMORY_API bool CheckHeap (bool stopOnError)
{
	return getGlobalHeapAllocator()->checkHeap (stopOnError);
}

// *********************************************************

MEMORY_API bool			CheckHeapBySize (bool stopOnError, uint blockSize)
{
	return getGlobalHeapAllocator()->checkHeapBySize (stopOnError, blockSize);
}

// *********************************************************

MEMORY_API bool StatisticsReport (const char *filename, bool memoryDump)
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return getGlobalHeapAllocator()->debugStatisticsReport (filename, memoryDump);
#else // SIP_HEAP_ALLOCATION_NDEBUG
	return false;
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

// *********************************************************

MEMORY_API void	ReportMemoryLeak ()
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	getGlobalHeapAllocator()->debugReportMemoryLeak ();
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

// *********************************************************

MEMORY_API void			AlwaysCheckMemory(bool alwaysCheck)
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	getGlobalHeapAllocator()->debugAlwaysCheckMemory (alwaysCheck);
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

// *********************************************************

MEMORY_API bool			IsAlwaysCheckMemory()
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return getGlobalHeapAllocator()->debugIsAlwaysCheckMemory ();
#endif // SIP_HEAP_ALLOCATION_NDEBUG
	return false;
}

// *********************************************************

MEMORY_API void			SetOutOfMemoryHook (void (*outOfMemoryCallback)())
{
	getGlobalHeapAllocator()->setOutOfMemoryHook (outOfMemoryCallback);
}

// *********************************************************

MEMORY_API unsigned int GetBlockSize (void *pointer)
{
	return getGlobalHeapAllocator()->getBlockSize (pointer);
}

// *********************************************************

MEMORY_API const char * GetCategory (void *pointer)
{
	return getGlobalHeapAllocator()->getCategory (pointer);
}

// *********************************************************

MEMORY_API void MemoryDeallocate (void *p)
{
	getGlobalHeapAllocator()->free (p);
}

// *********************************************************

MEMORY_API SIPMEMORY::CHeapAllocator* GetGlobalHeapAllocator ()
{
	return getGlobalHeapAllocator();
}

// *********************************************************

MEMORY_API void* SIPMEMORY::MemoryReallocate (void *p, unsigned int size)
{
	// Get the block size
/*	uint oldSize = SIPMEMORY::GetBlockSize (p);
	if (size > oldSize)
	{
		void *newPtr = MemoryAllocate (size);
		memcpy (newPtr, p, oldSize);
		MemoryDeallocate (p);
		p = newPtr;
	}
	return p;*/

	uint oldSize = SIPMEMORY::GetBlockSize (p);
	
	void *newPtr = MemoryAllocate (size);
	memcpy (newPtr, p, (size<oldSize) ? size : oldSize);
	
	MemoryDeallocate (p);
	return newPtr;
	
}

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

// Class to report memory leak at exit
class CReportMemoryLeak 
{
public:
	~CReportMemoryLeak ()
	{
		// Report memory leak
		getGlobalHeapAllocator()->debugReportMemoryLeak ();
	}
};

// *********************************************************

// Singleton
CReportMemoryLeak	ReportMemoryLeakSingleton;

// *********************************************************

#endif // SIP_HEAP_ALLOCATION_NDEBUG

#endif // SIP_USE_DEFAULT_MEMORY_MANAGER

} // SIPMEMORY

extern "C"
{

MEMORY_API void* SipMemoryAllocate (unsigned int size)
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return SIPMEMORY::getGlobalHeapAllocator()->allocate (size, "", 0, 0);
#else // SIP_HEAP_ALLOCATION_NDEBUG
	return SIPMEMORY::getGlobalHeapAllocator()->allocate (size);
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

MEMORY_API void* SipMemoryAllocateDebug (uint size, const char *filename, uint line, const char *category)
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return SIPMEMORY::getGlobalHeapAllocator()->allocate (size, filename, line, category);
#else // SIP_HEAP_ALLOCATION_NDEBUG
	return SIPMEMORY::getGlobalHeapAllocator()->allocate (size);
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

MEMORY_API void SipMemoryDeallocate (void *pointer)
{
	SIPMEMORY::getGlobalHeapAllocator()->free (pointer);
}

}

// *********************************************************

#ifndef SIP_USE_DEFAULT_MEMORY_MANAGER

// Need a debug new ?
#ifdef SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

#undef new

#define new SIP_NEW

#else // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

SIPMEMORY::CAllocContext::CAllocContext (const char* str)
{
	getGlobalHeapAllocator()->debugPushCategoryString (str);
}

// *********************************************************

SIPMEMORY::CAllocContext::~CAllocContext ()
{
	getGlobalHeapAllocator()->debugPopCategoryString ();
}

// *********************************************************

#undef new

// *********************************************************

#define new SIP_NEW

#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************
#endif // SIP_USE_DEFAULT_MEMORY_MANAGER
