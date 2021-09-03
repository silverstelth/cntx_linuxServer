/** \file memory_tds.cpp
 * Thread dependant storage class
 *
 * $Id: memory_tds.cpp,v 1.1 2008/06/26 13:41:17 RoMyongGuk Exp $
 */

#include "memory/memory_manager.h"
#include "memory_tds.h"

#ifdef SIP_OS_WINDOWS

#include <windows.h>

#else // SIP_OS_WINDOWS

#include <pthread.h>

#endif // SIP_OS_WINDOWS

namespace SIPMEMORY 
{

// *********************************************************

CMemoryTDS::CMemoryTDS ()
{
	/* Please no assert in the constructor because it is called by the SIP memory allocator constructor */
#ifdef SIP_OS_WINDOWS
	_Handle = TlsAlloc ();
	TlsSetValue (_Handle, NULL);
#else // SIP_OS_WINDOWS
	_Key = pthread_key_create (&_Key, 0);
	pthread_setspecific(_Key, 0);
#endif // SIP_OS_WINDOWS
}

// *********************************************************

CMemoryTDS::~CMemoryTDS ()
{
#ifdef SIP_OS_WINDOWS
	TlsFree (_Handle);
#else // SIP_OS_WINDOWS
	pthread_key_delete (_Key);
#endif // SIP_OS_WINDOWS
}

// *********************************************************

void *CMemoryTDS::getPointer () const
{
#ifdef SIP_OS_WINDOWS
	return TlsGetValue (_Handle);
#else // SIP_OS_WINDOWS
	return pthread_getspecific (_Key);
#endif // SIP_OS_WINDOWS
}

// *********************************************************

void CMemoryTDS::setPointer (void* pointer)
{
#ifdef SIP_OS_WINDOWS
	TlsSetValue (_Handle, pointer);
#else // SIP_OS_WINDOWS
	pthread_setspecific (_Key, pointer);
#endif // SIP_OS_WINDOWS
}

// *********************************************************

} // SIPMEMORY
