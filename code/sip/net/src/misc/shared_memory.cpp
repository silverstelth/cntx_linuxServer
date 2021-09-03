/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/shared_memory.h"

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#else
# include <sys/types.h>
# include <sys/ipc.h>
# include <sys/shm.h>
#endif

using namespace std;


namespace SIPBASE {


#ifdef SIP_OS_WINDOWS
	// Storage for file handles, necessary to close the handles
	map<void*,HANDLE>	AccessAddressesToHandles;
#else
  // Storage for shmid, necessary to destroy the segments
  map<TSharedMemId, int>   SharedMemIdsToShmids;
#endif


/*
 * Create a shared memory segment
 */
void			*CSharedMemory::createSharedMemory( TSharedMemId sharedMemId, uint32 size )
{
#ifdef SIP_OS_WINDOWS

	// Create a file mapping backed by the virtual memory swap file (not a data file)
	HANDLE hMapFile = CreateFileMappingA( INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, sharedMemId );
	if ( (hMapFile == NULL) || (GetLastError() == ERROR_ALREADY_EXISTS) )
	{
		sipwarning( "SHDMEM: Cannot create file mapping: error %u, mapFile %p", GetLastError(), hMapFile );
		return NULL;
	}
	//else
	//	sipdebug( "SHDMEM: Creating smid %s --> mapFile %p", sharedMemId, hMapFile );
	

	// Map the file into memory address space
	void *accessAddress = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
	AccessAddressesToHandles.insert( make_pair( accessAddress, hMapFile ) );
	/*if ( accessAddress == NULL )
	{
		sipwarning( "SHDMEM: Cannot map view of file: error %u", GetLastError() );
	}*/
	return accessAddress;

#else

	// Create a shared memory segment
	int shmid = shmget( sharedMemId, size, IPC_CREAT | IPC_EXCL | 0666 );
	if ( shmid == -1 )
		return NULL;
	SharedMemIdsToShmids.insert( make_pair( sharedMemId, shmid ) );

	// Map the segment into memory address space
	void *accessAddress = (void*)shmat( shmid, 0, 0 );
	if ( accessAddress == (void*)-1 )
		return NULL;
	else
		return accessAddress;
#endif
}


/*
 * Get access to an existing shared memory segment
 */
void			*CSharedMemory::accessSharedMemory( TSharedMemId sharedMemId )
{
#ifdef SIP_OS_WINDOWS

	// Open the existing file mapping by name
	HANDLE hMapFile = OpenFileMappingA( FILE_MAP_ALL_ACCESS, false, sharedMemId );
	if ( hMapFile == NULL )
		return NULL;
	//sipdebug( "SHDMEM: Opening smid %s --> mapFile %p", sharedMemId, hMapFile );

	// Map the file into memory address space
	void *accessAddress = MapViewOfFile( hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0 );
	AccessAddressesToHandles.insert( make_pair( accessAddress, hMapFile ) );
	return accessAddress;
	
#else

	// Open an existing shared memory segment
	int shmid = shmget( sharedMemId, 0, 0666 );
	if ( shmid == -1 )
		return NULL;

	// Map the segment into memory address space
	void *accessAddress = (void*)shmat( shmid, 0, 0 );
	if ( accessAddress == (void*)-1 )
		return NULL;
	else
		return accessAddress;

#endif
}


/*
 * Close (detach) a shared memory segment
 */
bool			CSharedMemory::closeSharedMemory( void *accessAddress )
{
#ifdef SIP_OS_WINDOWS

	bool result = true;

	// Unmap the file from memory address space
	if ( UnmapViewOfFile( accessAddress ) == 0 )
	{
		sipwarning( "SHDMEM: UnmapViewOfFile failed: error %u", GetLastError() );
		result = false;
	}

	// Close the corresponding handle
	map<void*,HANDLE>::iterator im = AccessAddressesToHandles.find( accessAddress );
	if ( im != AccessAddressesToHandles.end() )
	{
		//sipdebug( "SHDMEM: CloseHandle mapFile %u", (*im).second );
		if ( ! CloseHandle( (*im).second ) )
			sipwarning( "SHDMEM: CloseHandle failed: error %u, mapFile %u", GetLastError(), (*im).second );
		AccessAddressesToHandles.erase( im );
		return result;
	}
	else
	{
		return false;
	}

#else

	// Detach the shared memory segment
	return ( shmdt( accessAddress ) != -1 );
	
#endif
}


/*
 * Destroy a shared memory segment (to call only by the process that created the segment)
 * Note: does nothing under Windows, it is automatic.
 * "Rescue feature": set "force" to true if a segment was created and left out of
 * control (meaning a new createSharedMemory() with the same sharedMemId fails), but
 * before, make sure the segment really belongs to you!
 * 
 * Note: this method does nothing under Windows, destroying is automatic.
 * Under Unix, the segment will actually be destroyed after the last detach
 * (quoting shmctl man page). It means after calling destroySharedMemory(), the
 * segment is still accessible by another process until it calls closeSharedMemory().
 */
void        CSharedMemory::destroySharedMemory( TSharedMemId sharedMemId, bool force )
{
#ifndef SIP_OS_WINDOWS
  // Set the segment to auto-destroying (when the last process detaches)
  map<TSharedMemId,int>::iterator im = SharedMemIdsToShmids.find( sharedMemId );
  if ( im != SharedMemIdsToShmids.end() )
	{
	  // Destroy the segment created before
	  shmctl( (*im).second, IPC_RMID, 0 );
	  SharedMemIdsToShmids.erase( im );
	}
  else if ( force )
	{
	  // Open and destroy the segment
	  int shmid = shmget( sharedMemId, 0, 0666 );
	  if ( shmid != -1 )
		{
		  // Destroy the segment
		  shmctl( shmid, IPC_RMID, 0 );
		}
	}
#endif
}


} // SIPBASE
