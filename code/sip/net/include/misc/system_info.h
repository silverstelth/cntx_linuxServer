/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SYSTEM_INFO_H__
#define __SYSTEM_INFO_H__

#include "types_sip.h"
#include <string>

namespace SIPBASE {

/**
 * TODO Class description
 * \date 2000
 */
class CSystemInfo
{
public:

	static std::string getOS (bool bNotFull = false, bool bNeedBuildNum = true);
	static std::string getProc (bool bNotFull = false);

	/** Gives an evalutation of the processor frequency, in hertz
	  * \param quick true to do quick frequency evaluation
	  * \warning Supports only intel architectures for now. Returns 0 if not implemented.
	  */
	static uint64 getProcessorFrequency (bool quick = false);

	/** test wether the cpuid instruction is supported
	  * (always false on non intel architectures)
	  */
	static bool hasCPUID ();

	/** helps to know wether the processor features mmx instruction set 
	  * This is initialized at started, so its fast
	  * (always false on non 0x86 architecture ...)
	  */	  
	static bool hasMMX () {return _HaveMMX;}

	/** helps to know wether the processor has streaming SIMD instructions (the OS must supports it)
	  * This is initialized at started, so its fast
	  * (always false on non 0x86 architecture ...)
	  */
	static bool hasSSE () {return _HaveSSE;}

	/** get the CPUID (if available). Usefull for debug info
	 */
	static uint32 getCPUID();

	/** true if the Processor has HyperThreading
	 */
	static bool hasHyperThreading();
	
	/** true if running under NT
	 */
	static bool isNT();

	/* returns the space left on the hard drive that contains the filename
	 */
	static std::string availableHDSpace (const std::string &filename);

	/** returns all the physical memory available on this computer
	  */
	static uint64 availablePhysicalMemory ();

	/** returns all the physical memory on this computer
	  */
	static uint64 totalPhysicalMemory ();

	/* Return the amount of allocated system memory */
	static uint32 getAllocatedSystemMemory ();

	/** returns all the virtual memory used by this process
	  */
	static uint64 virtualMemory ();

	/** Return the main video card name and the video driver version
	  */
	static bool getVideoInfo (std::string &deviceName, uint64 &driverVersion);

	static bool	IsSystemService(std::string serviceName);
private:
	static bool _HaveMMX;
	static bool _HaveSSE;
	
};


} // SIPBASE


#endif // __SYSTEM_INFO_H__

/* End of system_info.h */
