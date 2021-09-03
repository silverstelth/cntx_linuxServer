/** \file memory_mutex.h
 * Mutex used by the memory manager
 *
 * $Id: memory_mutex.h,v 1.1 2008/06/26 13:41:17 RoMyongGuk Exp $
 */

#ifndef SIP_MEMORY_MUTEX_H
#define SIP_MEMORY_MUTEX_H

#include "memory_common.h"

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#include <winbase.h>
#endif

#ifdef SIP_OS_UNIX
#include <pthread.h> // PThread
#include <semaphore.h> // PThread POSIX semaphores
#endif

namespace SIPMEMORY 
{

/**
 * Mutex used by the memory manager
 *
 * \date 2002
 */
class CMemoryMutex
{
public:

#ifdef SIP_OS_WINDOWS

	/// Constructor
	CMemoryMutex();

	__forceinline static bool atomic_swap (volatile uint32 *lockPtr)
	{
		uint32 result;
#ifdef SIP_DEBUG_FAST
		// Workaround for dumb inlining bug (returning of function goes into the choux): push/pop registers
		__asm
		{
				push eax
				push ecx
			mov ecx,lockPtr
			mov eax,1
			xchg [ecx],eax
			mov [result],eax
				pop ecx
				pop eax
		}
#else
		__asm
		{
			mov ecx,lockPtr
			mov eax,1
			xchg [ecx],eax
			mov [result],eax
		}
#endif
		return result != 0;
	}

	__forceinline void enter ()
	{
		if (atomic_swap (&_Lock))
		{
			// First test
			uint i;
			for (i = 0 ;; ++i)
			{
				uint wait_time = i + 6;

				// Increment wait time with a log function
				if (wait_time > 27) 
					wait_time = 27;

				// Sleep
				if (wait_time <= 20) 
					wait_time = 0;
				else
					wait_time = 1 << (wait_time - 20);

				if (!atomic_swap (&_Lock))
					break;

				Sleep (wait_time);
			}
		}
	}

	__forceinline void leave ()
	{
		_Lock = 0;
	}

	volatile uint32 _Lock;

#else // SIP_OS_WINDOWS

public:

	/// Constructor
	CMemoryMutex();

	/// Destructor
	~CMemoryMutex();

	void enter ();
	void leave ();

private:

	sem_t			_Sem;

#endif // SIP_OS_WINDOWS

};


} // SIPMEMORY


#endif // SIP_MEMORY_MUTEX_H

/* End of memory_mutex.h */
