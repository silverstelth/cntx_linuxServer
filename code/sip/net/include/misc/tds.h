/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TDS_H__
#define __TDS_H__

#include "types_sip.h"

namespace SIPBASE 
{

/**
 * Thread dependant storage class
 *
 * This class provides a thread specific (void*).
 * It is initialized at NULL.
 *
 * \date 2002
 */
class CTDS
{
public:

	/// Constructor. The pointer is initialized with NULL.
	CTDS ();

	/// Destructor
	~CTDS ();

	/// Get the thread specific pointer
	void	*getPointer () const;

	/// Set the thread specific pointer
	void	setPointer (void* pointer);

private:
#ifdef SIP_OS_WINDOWS
	uint32			_Handle;
#else // SIP_OS_WINDOWS
	pthread_key_t	_Key;
#endif // SIP_OS_WINDOWS

};


} // SIPBASE


#endif // __TDS_H__

/* End of tds.h */
