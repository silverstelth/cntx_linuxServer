/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __FAST_MEM_H__
#define __FAST_MEM_H__

#include "types_sip.h"


namespace SIPBASE
{

//typedef void  *memcpyptr(void *dts, const void *src, size_t nbytes);

/**
 * Functions for Fast Memory manipulation with Pentium-class processors.
 * From http://www.sgi.com/developers/technology/irix/resources/asc_cpu.html
 * \date 2002
 */
class CFastMem
{
public:

	/**
	 *	This is a function pointer that points on the best memcpy function available depending of the OS and proc.
	 *  In the best case, it will use memcpySSE(), and in worst case, it'll use the libc memcpy()
	 *  Simply use it this way: CFastMem::memcpy(dst, src, size);
	 */
	static void  *(*memcpy)(void *dts, const void *src, size_t nbytes);

	/**
	 *	Fast precaching of memory in L1 cache using SSE or MMX where available 
	 *	(NB: others methods don't do the test)
	 *	nbytes should not override 4K
	 */
	static void		precache(const void *src, uint nbytes);

	/////////////////////////////////////////////

	/**
	 *	Fast memcpy using SSE instructions: prefetchnta and movntq. Can be called only if SSE and MMX is supported
	 *	NB: Copy per block of 4K through L1 cache
	 *	Result is typically 420 Mo/s instead of 150 Mo/s.
	 */
	static void		*memcpySSE(void *dst, const void *src, size_t nbytes);

	/**
	 *	Fast precaching of memory in L1 cache using MMX/SSE instructions: movq and prefetchnta
	 *	Result is typically 880 Mo/s (surely slower because of overhead).
	 *	nbytes should not override 4K
	 */
	static void		precacheSSE(const void *src, uint nbytes);

	/**
	 *	Fast precaching of memory in L1 cache using MMX instructions only: movq 
	 *	Result is typically 720 Mo/s (surely slower because of overhead).
	 *	Hence prefer precacheSSE() when available.
	 *	nbytes should not override 4K
	 */
	static void		precacheMMX(const void *src, uint nbytes);
};


} // SIPBASE


#endif // __FAST_MEM_H__

/* End of fast_mem.h */
