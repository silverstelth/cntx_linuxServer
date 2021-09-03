/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/cpu_info.h"

namespace SIPBASE 
{

static bool DetectMMX(void)
{		
	#ifdef SIP_OS_WINDOWS		
		if (!CCpuInfo___::hasCPUID()) return false; // cpuid not supported ...

		uint32 result = 0;
		__asm
		{
			 mov  eax,1
			 cpuid
			 test edx,0x800000  // bit 23 = MMX instruction set
			 je   noMMX
			 mov result, 1	
			noMMX:
		}

		return result == 1;
 
		// printf("mmx detected\n");

	#else
		return false;
	#endif
}


static bool DetectSSE(void)
{	
	#ifdef SIP_OS_WINDOWS
		if (!CCpuInfo___::hasCPUID()) return false; // cpuid not supported ...

		uint32 result = 0;
		__asm
		{			
			mov eax, 1   // request for feature flags
			cpuid 							
			test EDX, 002000000h   // bit 25 in feature flags equal to 1
			je noSSE
			mov result, 1  // sse detected
		noSSE:
		}


		if (result)
		{
			// check OS support for SSE
			try 
			{
				__asm
				{
					xorps xmm0, xmm0  // Streaming SIMD Extension
				}
			}
			catch(...)
			{
				return false;
			}
		
			// printf("sse detected\n");

			return true;
		}
		else
		{
			return false;
		}
	#else
		return false;
	#endif
}

bool HasMMX = DetectMMX();
bool HasSSE = DetectSSE();

bool CCpuInfo___::hasCPUID(void)
{
	#ifdef SIP_OS_WINDOWS
		 uint32 result;
		 __asm
		 {
			 pushad
			 pushfd						
			 //	 If ID bit of EFLAGS can change, then cpuid is available
			 pushfd
			 pop  eax					// Get EFLAG
			 mov  ecx,eax
			 xor  eax,0x200000			// Flip ID bit
			 push eax
			 popfd						// Write EFLAGS
			 pushfd      
			 pop  eax					// read back EFLAG
			 xor  eax,ecx				
			 je   noCpuid				// no flip -> no CPUID instr.
			 
			 popfd						// restore state
			 popad
			 mov  result, 1
			 jmp  CPUIDPresent
		
			noCpuid:
			 popfd					    // restore state
			 popad
			 mov result, 0
			CPUIDPresent:	 
		 }
		 return result == 1;
	#else
		 return false;
	#endif
}
bool CCpuInfo___::hasMMX(void) { return HasMMX; }
bool CCpuInfo___::hasSSE(void) { return HasSSE; }

} // SIPBASE
