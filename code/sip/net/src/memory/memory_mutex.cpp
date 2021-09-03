/** \file memory_mutex.cpp
 * Mutex used by the memory manager
 *
 * $Id: memory_mutex.cpp,v 1.1 2008/06/26 13:41:17 RoMyongGuk Exp $
 */

#include "memory/memory_manager.h"
#include "memory_mutex.h"


namespace SIPMEMORY 
{

#ifdef SIP_OS_WINDOWS

// *********************************************************

CMemoryMutex::CMemoryMutex ()
{
	_Lock = 0;
}

#else // SIP_OS_WINDOWS

/*
 * Unix version
 */

// *********************************************************

CMemoryMutex::CMemoryMutex()
{
	sem_init( const_cast<sem_t*>(&_Sem), 0, 1 );
}

// *********************************************************

CMemoryMutex::~CMemoryMutex()
{
	sem_destroy( const_cast<sem_t*>(&_Sem) ); // needs that no thread is waiting on the semaphore
}

// *********************************************************

void CMemoryMutex::enter()
{
	sem_wait( const_cast<sem_t*>(&_Sem) );
}

// *********************************************************

void CMemoryMutex::leave()
{
	sem_post( const_cast<sem_t*>(&_Sem) );
}

// *********************************************************

#endif // SIP_OS_WINDOWS

} // SIPMEMORY
