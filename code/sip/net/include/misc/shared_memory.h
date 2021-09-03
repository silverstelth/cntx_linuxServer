/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SHARED_MEMORY_H__
#define __SHARED_MEMORY_H__

#include "types_sip.h"

#ifdef SIP_OS_WINDOWS
#include <map>
#endif

namespace SIPBASE {

#ifdef SIP_OS_WINDOWS
typedef const char *TSharedMemId;
#else
typedef key_t TSharedMemId;
#endif


/* Helpers:
 * toSharedMemId: converts an integer to TSharedMemId
 * SIP_SMID: format type used for printf syntax. Ex: sipdebug( "Segment %"SIP_SMID" was created", sharedMemId );
 */
#ifdef SIP_OS_WINDOWS
#define toSharedMemId( id ) toString( "SIPSM_%d", id ).c_str()
#define SIP_SMID "s"
#else
#define toSharedMemId( id ) (id)
#define SIP_SMID "d"
#endif


/**
 * Encapsulation of shared memory APIs.
 * Using file mapping under Windows, System V shared memory (shm) under Linux.
 *
 * Note: under Linux, an option could be added to prevent a segment to be swapped out.
 *
 * \date 2002
 */
class CSharedMemory
{
public:

	// Constructor
	//CSharedMemory();

	/**
	 * Create a shared memory segment and get access to it. The id must not be used.
	 * The id 0x3a732235 is reserved by the SIP memory manager.
	 * \return Access address of the segment of the choosen size
	 */
	static void *		createSharedMemory( TSharedMemId sharedMemId, uint32 size );

	/**
	 * Get access to an existing shared memory segment. The id must be used.
	 * \return Access address of the segment. Its size was determined at the creation.
	 */
	static void *		accessSharedMemory( TSharedMemId sharedMemId );

	/**
	 * Close (detach) a shared memory segment, given the address returned by createSharedMemory()
	 * or accessSharedMemory(). Must be called by each process that called createSharedMemory()
	 * or accessSharedMemory().
	 */
	static bool			closeSharedMemory( void * accessAddress );

  /**
   * Destroy a shared memory segment (must be called by the process that created the segment,
   * not by the accessors).
   *
   * "Rescue feature": set "force" to true if a segment was created and left out of
   * control (meaning a new createSharedMemory() with the same sharedMemId fails), but
   * before, make sure the segment really belongs to you!
   * 
   * Note: this method does nothing under Windows, destroying is automatic.
   * Under Unix, the segment will actually be destroyed after the last detach
   * (quoting shmctl man page). It means after calling destroySharedMemory(), the
   * segment is still accessible by another process until it calls closeSharedMemory().
   */
  static void destroySharedMemory( TSharedMemId sharedMemId, bool force=false );
};


} // SIPBASE


#endif // __SHARED_MEMORY_H__

/* End of shared_memory.h */
