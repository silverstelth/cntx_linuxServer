/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CPU_INFO_H__
#define __CPU_INFO_H__

#include "types_sip.h"

namespace SIPBASE {


/**
 * This helps to know wether cpu has some features such as mmx, sse ... 
 * \date 2001
 */
struct CCpuInfo___
{
	/** test wether the cpuid instruction is supported
	  * (always false on non intel architectures)
	  */
	static bool hasCPUID(void);

	/** helps to know wether the processor features mmx instruction set 
	  * This is initialized at started, so its fast
	  * (always false on not 0x86 architecture ...)
	  */	  
	static bool hasMMX(void);

	/** helps to know wether the processor has streaming SIMD instructions (the OS must supports it)
	  * This is initialized at started, so its fast
	  * (always false on not 0x86 architecture ...)
	  */
	static bool hasSSE(void);
};


} // SIPBASE


#endif // __CPU_INFO_H__

/* End of cpu_info.h */
