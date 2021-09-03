// memory.cpp : Defines the entry point for the DLL application.
//

//#include <stl/_site_config.h>
#include "memory/memory_manager.h"
#include <new>
#ifdef WIN32
#include <crtdbg.h>
#endif
#include "heap_allocator.h"

namespace SIPMEMORY
{
	extern CHeapAllocator	*GlobalHeapAllocator;
	extern sint32			GlobalHeapAllocatorSystemAllocated;
}

#ifdef WIN32
#ifdef _DEBUG
#ifndef SIP_USE_DEFAULT_MEMORY_MANAGER
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
int MemoryHook( int allocType, void *userData, size_t size, int blockType, 
			   long requestNumber, const unsigned char *filename, int lineNumber)
{
	if ( allocType == _HOOK_ALLOC)   // Ignore internal C runtime library allocations
	{
		SIPMEMORY::GlobalHeapAllocatorSystemAllocated += size;
	}
	if ( allocType == _HOOK_FREE)   // Ignore internal C runtime library allocations
	{
		SIPMEMORY::GlobalHeapAllocatorSystemAllocated -= _msize_dbg( userData, _NORMAL_BLOCK);
	}
	if ( allocType == _HOOK_REALLOC)   // Ignore internal C runtime library allocations
	{
		SIPMEMORY::GlobalHeapAllocatorSystemAllocated -= _msize_dbg( userData, _NORMAL_BLOCK);
		SIPMEMORY::GlobalHeapAllocatorSystemAllocated += size;
	}
	
	return TRUE;
}
#endif // SIP_HEAP_ALLOCATION_NDEBUG
#endif // SIP_USE_DEFAULT_MEMORY_MANAGER
#endif // _DEBUG

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    switch (ul_reason_for_call)
	{
		case DLL_PROCESS_ATTACH:
#ifdef WIN32
#ifdef _DEBUG
#ifndef SIP_USE_DEFAULT_MEMORY_MANAGER
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
			_CrtSetAllocHook(MemoryHook);
#endif // SIP_HEAP_ALLOCATION_NDEBUG
#endif // SIP_USE_DEFAULT_MEMORY_MANAGER
#endif // _DEBUG
#endif // WIN32
			SIPMEMORY::GlobalHeapAllocator = (SIPMEMORY::CHeapAllocator*)malloc (sizeof (SIPMEMORY::CHeapAllocator));
#undef new
			new (SIPMEMORY::GlobalHeapAllocator) SIPMEMORY::CHeapAllocator (1024*1024*10, 1);
			SIPMEMORY::GlobalHeapAllocator->setName ("SIP memory manager");
			break;
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}
#else

// do nothing

#endif // WIN32
