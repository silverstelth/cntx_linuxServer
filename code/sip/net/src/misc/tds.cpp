/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/tds.h"

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif // SIP_OS_WINDOWS

namespace SIPBASE 
{

// *********************************************************

CTDS::CTDS ()
{
	/* Please no assert in the constructor because it is called by the SIP memory allocator constructor */
#ifdef SIP_OS_WINDOWS
	_Handle = TlsAlloc ();
	TlsSetValue (_Handle, NULL);
#else // SIP_OS_WINDOWS
//	sipdebug("CTDS::CTDS...");
	sipverify(pthread_key_create (&_Key, NULL) == 0);
//	sipdebug("CTDS::CTDS : create a new key %u", _Key);
	pthread_setspecific(_Key, NULL);
#endif // SIP_OS_WINDOWS
}

// *********************************************************

CTDS::~CTDS ()
{
#ifdef SIP_OS_WINDOWS
	bool b = (TlsFree (_Handle) != 0);
	sipverify (b);
#else // SIP_OS_WINDOWS
//	sipdebug("CTDS::~CTDS : deleting key %u", _Key);
	sipverify (pthread_key_delete (_Key) == 0);
#endif // SIP_OS_WINDOWS
}

// *********************************************************

void *CTDS::getPointer () const
{
#ifdef SIP_OS_WINDOWS
	return TlsGetValue (_Handle);
#else // SIP_OS_WINDOWS
//	sipdebug("CTDS::getPointer for key %u...", _Key);
	void *ret = pthread_getspecific (_Key);
//	sipdebug("CTDS::getPointer returing value %p", ret);
	return ret; 
#endif // SIP_OS_WINDOWS
}

// *********************************************************

void CTDS::setPointer (void* pointer)
{
#ifdef SIP_OS_WINDOWS
	bool b = (TlsSetValue (_Handle, pointer) != 0);
	sipverify (b);
#else // SIP_OS_WINDOWS
//	sipdebug("CTDS::setPointer for key %u to value %p", _Key, pointer);
	sipverify (pthread_setspecific (_Key, pointer) == 0);
#endif // SIP_OS_WINDOWS
}

// *********************************************************

} // SIPBASE
