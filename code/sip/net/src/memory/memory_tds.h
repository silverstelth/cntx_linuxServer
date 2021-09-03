/** \file memory_tds.h
 * Thread dependant storage class
 *
 * $Id: memory_tds.h,v 1.1 2008/06/26 13:41:17 RoMyongGuk Exp $
 */

#ifndef SIP_MEMORY_TDS_H
#define SIP_MEMORY_TDS_H

#include "memory_common.h"

namespace SIPMEMORY {


/**
 * Thread dependant storage class
 *
 * This class provides a thread specific (void*).
 * It is initialized at NULL.
 *
 * \date 2002
 */
class CMemoryTDS
{
public:

	/// Constructor. The pointer is initialized with NULL.
	CMemoryTDS ();

	/// Destructor
	~CMemoryTDS ();

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


} // SIPMEMORY


#endif // SIP_MEMORY_TDS_H

/* End of memory_tds.h */
