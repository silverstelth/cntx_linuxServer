/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/fast_mem.h"
#include "misc/system_info.h"


namespace SIPBASE
{

#ifdef SIP_OS_WINDOWS


// ***************************************************************************
void		*CFastMem::memcpySSE(void *dest, const void *src, size_t nbytes)
{
	_asm 
	{
			mov esi, src 
			mov edi, dest 
			mov ebx, nbytes 

			// edx takes number of bytes%64
			mov	edx, ebx
			and edx, 63

			// ebx takes number of bytes/64
			shr	ebx, 6
			jz	byteCopy


	loop4k: // flush 4k into temporary buffer 
			push esi 
			mov ecx, ebx
			// copy per block of 64 bytes. Must not override 64*64= 4096 bytes.
			cmp ecx, 64
			jle	skipMiniMize
			mov	ecx, 64
	skipMiniMize:
			// eax takes the number of 64bytes packet for this block.
			mov eax, ecx

	loopMemToL1: 
			prefetchnta 64[ESI] // Prefetch next loop, non-temporal 
			prefetchnta 96[ESI] 

			movq mm1,  0[ESI] // Read in source data 
			movq mm2,  8[ESI] 
			movq mm3, 16[ESI] 
			movq mm4, 24[ESI] 
			movq mm5, 32[ESI] 
			movq mm6, 40[ESI] 
			movq mm7, 48[ESI] 
			movq mm0, 56[ESI] 

			add esi, 64 
			dec ecx 
			jnz loopMemToL1 

			pop esi // Now copy from L1 to system memory 
			mov ecx, eax

	loopL1ToMem: 
			movq mm1, 0[ESI] // Read in source data from L1 
			movq mm2, 8[ESI] 
			movq mm3, 16[ESI] 
			movq mm4, 24[ESI] 
			movq mm5, 32[ESI] 
			movq mm6, 40[ESI] 
			movq mm7, 48[ESI] 
			movq mm0, 56[ESI] 

			movntq 0[EDI], mm1 // Non-temporal stores 
			movntq 8[EDI], mm2 
			movntq 16[EDI], mm3 
			movntq 24[EDI], mm4 
			movntq 32[EDI], mm5 
			movntq 40[EDI], mm6 
			movntq 48[EDI], mm7 
			movntq 56[EDI], mm0 

			add esi, 64 
			add edi, 64 
			dec ecx 
			jnz loopL1ToMem

			// Do next 4k block 
			sub ebx, eax
			jnz loop4k 

			emms

	byteCopy:
			// Do last bytes with std cpy
			mov	ecx, edx
			rep movsb
	}
	return dest;
}

// ***************************************************************************
void		CFastMem::precacheSSE(const void *src, uint nbytes)
{
	_asm 
	{ 
			mov esi, src 
			mov ecx, nbytes
			// 64 bytes per pass
			shr ecx, 6 
			jz endLabel

	loopMemToL1: 
			prefetchnta 64[ESI] // Prefetch next loop, non-temporal 
			prefetchnta 96[ESI] 

			movq mm1,  0[ESI] // Read in source data 
			movq mm2,  8[ESI] 
			movq mm3, 16[ESI] 
			movq mm4, 24[ESI] 
			movq mm5, 32[ESI] 
			movq mm6, 40[ESI] 
			movq mm7, 48[ESI] 
			movq mm0, 56[ESI]

			add esi, 64 
			dec ecx 
			jnz loopMemToL1 

			emms

	endLabel:
	}
}

// ***************************************************************************
void		CFastMem::precacheMMX(const void *src, uint nbytes)
{
	_asm 
	{ 
			mov esi, src 
			mov ecx, nbytes
			// 64 bytes per pass
			shr ecx, 6 
			jz endLabel

	loopMemToL1: 
			movq mm1,  0[ESI] // Read in source data 
			movq mm2,  8[ESI] 
			movq mm3, 16[ESI] 
			movq mm4, 24[ESI] 
			movq mm5, 32[ESI] 
			movq mm6, 40[ESI] 
			movq mm7, 48[ESI] 
			movq mm0, 56[ESI]

			add esi, 64 
			dec ecx 
			jnz loopMemToL1 

			emms

	endLabel:
	}
}


// ***************************************************************************
void		CFastMem::precache(const void *src, uint nbytes)
{
	if(SIPBASE::CSystemInfo::hasSSE())
		precacheSSE(src, nbytes);
	else if(SIPBASE::CSystemInfo::hasMMX())
		precacheMMX(src, nbytes);
}


#else


// ***************************************************************************
void		*CFastMem::memcpySSE(void *dst, const void *src, size_t nbytes)
{
	// Use std memcpy.
	return memcpy(dst, src, nbytes);
}
void		CFastMem::precacheSSE(const void *src, uint nbytes)
{
	// no-op.
}
void		CFastMem::precacheMMX(const void *src, uint nbytes)
{
	// no-op.
}
void		CFastMem::precache(const void *src, uint nbytes)
{
	// no-op.
}

#endif

typedef void  *(*memcpyPtr)(void *dts, const void *src, size_t nbytes);

static memcpyPtr findBestmemcpy ()
{
#ifdef SIP_OS_WINDOWS
	if (CSystemInfo::hasSSE ())
		return CFastMem::memcpySSE;
	else
		return ::memcpy;
#else // SIP_OS_WINDOWS
	return ::memcpy;
#endif // SIP_OS_WINDOWS
}

void  *(*CFastMem::memcpy)(void *dts, const void *src, size_t nbytes) = findBestmemcpy ();

} // SIPBASE
