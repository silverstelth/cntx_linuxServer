/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#ifdef SIP_OS_WINDOWS
	#include <windows.h>
//	#include <tchar.h>
#else
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/sysctl.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <cerrno>
#endif // SIP_OS_WINDOWS

#include "misc/system_info.h"
#include "misc/command.h"
#include "misc/variable.h"
#include "misc/CPU.h"

using namespace std;

namespace SIPBASE {
	
#ifdef SIP_OS_UNIX
	static string getCpuInfo(const string &colname)
	{
		if (colname.empty())
			return "";
		
		int fd = open("/proc/cpuinfo", O_RDONLY);
		if (fd == -1)
		{
			sipwarning ("SI: Can't open /proc/cpuinfo: %s", strerror (errno));
			return "";
		}
		else
		{
			char buffer[4096+1];
			uint32 len = read(fd, buffer, sizeof(buffer)-1);
			close(fd);
			buffer[len] = '\0';
			
			vector<string> splitted;
			explode(string(buffer), string("\n"), splitted, true);
			
			for(uint32 i = 0; i < splitted.size(); i++)
			{
				vector<string> sline;
				explode(splitted[i], string(":"), sline, true);
				if(sline.size() == 2 && trim(sline[0]) == colname)
				{
					return trim(sline[1]);
				}
			}
		}
		sipwarning ("SI: Can't find the colname '%s' in /proc/cpuinfo", colname.c_str());
		return "";
	}
	
	// return the value of the colname in bytes from /proc/meminfo
	static uint64 getSystemMemory (const string &colname)
	{
		if (colname.empty())
			return 0;
		
		int fd = open("/proc/meminfo", O_RDONLY);
		if (fd == -1)
		{
			sipwarning ("SI: Can't open /proc/meminfo: %s", strerror (errno));
			return 0;
		}
		else
		{
			char buffer[4096+1];
			uint32 len = read(fd, buffer, sizeof(buffer)-1);
			close(fd);
			buffer[len] = '\0';
			
			vector<string> splitted;
			explode(string(buffer), string("\n"), splitted, true);
			
			for(uint32 i = 0; i < splitted.size(); i++)
			{
				vector<string> sline;
				explode(splitted[i], string(" "), sline, true);
				if(sline.size() == 3 && sline[0] == colname)
				{
					uint64 val = atoiInt64(sline[1].c_str());
					if(sline[2] == "kB") val *= 1024;
					return val;
				}
			}
		}
		sipwarning ("SI: Can't find the colname '%s' in /proc/meminfo", colname.c_str());
		return 0;
	}
#endif // SIP_OS_UNIX
	
#ifdef SIP_OS_MAC
	static sint32 getsysctlnum(const string &name)
	{
		sint32 value = 0;
		size_t len = sizeof(value);
		if(sysctlbyname(name.c_str(), &value, &len, NULL, 0) != 0)
		{
			sipwarning("SI: Can't get '%s' from sysctl: %s", name.c_str(), strerror (errno));
		}
		return value;
	}
	
	static sint64 getsysctlnum64(const string &name)
	{
		sint64 value = 0;
		size_t len = sizeof(value);
		if(sysctlbyname(name.c_str(), &value, &len, NULL, 0) != 0)
		{
			sipwarning("SI: Can't get '%s' from sysctl: %s", name.c_str(), strerror (errno));
		}
		return value;
	}
	
	static string getsysctlstr(const string &name)
	{
		string value("Unknown");
		size_t len;
		char *p;
		if(sysctlbyname(name.c_str(), NULL, &len, NULL, 0) == 0)
		{
			p = (char*)malloc(len);
			if(sysctlbyname(name.c_str(), p, &len, NULL, 0) == 0)
			{
				value = p;
			}
			else
			{
				sipwarning("SI: Can't get '%s' from sysctl: %s", name.c_str(), strerror (errno));
			}
			free(p);
		}
		else
		{
			sipwarning("SI: Can't get '%s' from sysctl: %s", name.c_str(), strerror (errno));
		}
		return value;
	}
#endif // SIP_OS_MAC
	


string CSystemInfo::getOS(bool bNotFull, bool bNeedBuildNum)
{
	string OSString = "Unknown";

#ifdef SIP_OS_WINDOWS

	OSVERSIONINFOEXA osvi;
	BOOL bOsVersionInfoEx;
	const int BUFSIZE = 80;

	// Try calling GetVersionEx using the OSVERSIONINFOEX structure.
	// If that fails, try using the OSVERSIONINFO structure.

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

	if( !(bOsVersionInfoEx = GetVersionExA ((OSVERSIONINFOA *) &osvi)) )
	{
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA);
		if (! GetVersionExA ( (OSVERSIONINFOA *) &osvi) ) 
		return OSString+" Can't GetVersionEx()";
	}

	switch (osvi.dwPlatformId)
	{
	// Test for the Windows NT product family.
	case VER_PLATFORM_WIN32_NT:

		// Test for the specific product family.
		if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 0)
			OSString = "Microsoft Windows Vista ";
		if ( osvi.dwMajorVersion == 6 && osvi.dwMinorVersion == 1)
			OSString = "Microsoft Windows 7";
		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
			OSString = "Microsoft Windows Server 2003 family ";

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 )
			OSString = "Microsoft Windows XP ";

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
			OSString = "Microsoft Windows 2000 ";

		if ( osvi.dwMajorVersion <= 4 )
			OSString = "Microsoft Windows NT ";

         // Test for specific product on Windows NT 4.0 SP6 and later.
         if( bOsVersionInfoEx )
         {
			 // not available on visual 6 SP4, then comment it

/*            // Test for the workstation type.
            if ( osvi.wProductType == VER_NT_WORKSTATION )
            {
               if( osvi.dwMajorVersion == 4 )
                  printf ( "Workstation 4.0 " );
               else if( osvi.wSuiteMask & VER_SUITE_PERSONAL )
                  printf ( "Home Edition " );
               else
                  printf ( "Professional " );
            }
            
            // Test for the server type.
            else if ( osvi.wProductType == VER_NT_SERVER )
            {
               if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 )
               {
                  if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                     printf ( "Datacenter Edition " );
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     printf ( "Enterprise Edition " );
                  else if ( osvi.wSuiteMask == VER_SUITE_BLADE )
                     printf ( "Web Edition " );
                  else
                     printf ( "Standard Edition " );
               }

               else if( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 )
               {
                  if( osvi.wSuiteMask & VER_SUITE_DATACENTER )
                     printf ( "Datacenter Server " );
                  else if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     printf ( "Advanced Server " );
                  else
                     printf ( "Server " );
               }

               else  // Windows NT 4.0 
               {
                  if( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
                     printf ("Server 4.0, Enterprise Edition " );
                  else
                     printf ( "Server 4.0 " );
               }
            }*/
		}
		else  // Test for specific product on Windows NT 4.0 SP5 and earlier
		{
			HKEY hKey;
			char szProductType[BUFSIZE];
			DWORD dwBufLen=BUFSIZE;
			LONG lRet;

			lRet = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\ProductOptions", 0, KEY_QUERY_VALUE, &hKey );
			if( lRet != ERROR_SUCCESS )
				return OSString + " Can't RegOpenKeyEx";

			lRet = RegQueryValueExA( hKey, "ProductType", NULL, NULL, (LPBYTE) szProductType, &dwBufLen);
			if( (lRet != ERROR_SUCCESS) || (dwBufLen > BUFSIZE) )
				return OSString + " Can't ReQueryValueEx";

			RegCloseKey( hKey );

			if ( lstrcmpiA( "WINNT", szProductType) == 0 )
				OSString += "Workstation ";
			if ( lstrcmpiA( "LANMANNT", szProductType) == 0 )
				OSString += "Server ";
			if ( lstrcmpiA( "SERVERNT", szProductType) == 0 )
				OSString += "Advanced Server ";
         }

		// Display service pack (if any) and build number.

		if( osvi.dwMajorVersion == 4 && lstrcmpiA( osvi.szCSDVersion, "Service Pack 6" ) == 0 )
		{
			HKEY hKey;
			LONG lRet;

			// Test for SP6 versus SP6a.
			lRet = RegOpenKeyExA( HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Hotfix\\Q246009", 0, KEY_QUERY_VALUE, &hKey );
			if( lRet == ERROR_SUCCESS )
			{
				if ( bNeedBuildNum )
					OSString += toString("Service Pack 6a (Build %d) ", osvi.dwBuildNumber & 0xFFFF );
				else
					OSString += toString("Service Pack 6a ", osvi.dwBuildNumber & 0xFFFF );
			}
			else // Windows NT 4.0 prior to SP6a
			{
				if ( bNeedBuildNum )
					OSString += toString("%s (Build %d) ", osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
				else
					OSString += toString("%s ", osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
			}

			RegCloseKey( hKey );
		}
		else // Windows NT 3.51 and earlier or Windows 2000 and later
		{
			if ( bNeedBuildNum )
				OSString += toString("%s (Build %d) ", osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
			else
				OSString += toString("%s ", osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
		}

		break;

		// Test for the Windows 95 product family.
		case VER_PLATFORM_WIN32_WINDOWS:

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
			{
				OSString = "Microsoft Windows 95 ";
				if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' )
					OSString += "OSR2 ";
			} 

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
			{
				OSString = "Microsoft Windows 98 ";
				if ( osvi.szCSDVersion[1] == 'A' )
					OSString += "SE ";
			} 

			if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
			{
				OSString = "Microsoft Windows Millennium Edition ";
			} 
		break;

		case VER_PLATFORM_WIN32s:

			OSString = "Microsoft Win32s ";
		break;
	}

	if ( bNotFull )
		return normalizeString(OSString);

	OSString += toString( "(%d.%d %d)", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber & 0xFFFF);

#elif defined SIP_OS_MAC
	
	OSString = getsysctlstr("kern.version");	
	
#elif defined SIP_OS_UNIX
	
	int fd = open("/proc/version", O_RDONLY);
	if (fd == -1)
	{
		sipwarning ("SI: Can't get OS from /proc/version: %s", strerror (errno));
	}
	else
	{
		char buffer[4096+1];
		int len = read(fd, buffer, sizeof(buffer)-1);
		close(fd);
		
		// remove the \n and set \0
		buffer[len-1] = '\0';
		
		OSString = buffer;
	}
	
#endif	// SIP_OS_UNIX
	
	return normalizeString(OSString);
}


#if 0 // old getOS() function

string /*CSystemInfo::*/_oldgetOS()
{
	string OSString = "Unknown";
#ifdef SIP_OS_WINDOWS
	char ver[1024];

	OSVERSIONINFOEXA osvi;
	BOOL bOsVersionInfoEx;

	// Try calling GetVersionEx using the OSVERSIONINFOEX structure,
	// which is supported on Windows 2000.
	//
	// If that fails, try using the OSVERSIONINFO structure.

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXA));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);

	if( !(bOsVersionInfoEx = GetVersionExA ((OSVERSIONINFOA *) &osvi)) )
	{
		// If OSVERSIONINFOEX doesn't work, try OSVERSIONINFO.
		osvi.dwOSVersionInfoSize = sizeof (OSVERSIONINFOA);
		if (! GetVersionExA ( (OSVERSIONINFOA *) &osvi) ) 
			return "Windows Unknown";
	}

	switch (osvi.dwPlatformId)
	{
	case VER_PLATFORM_WIN32_NT:
		// Test for the product.

		if (osvi.dwMajorVersion <= 4)
			OSString = "Microsoft Windows NT ";
		else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0)
			OSString = "Microsoft Windows 2000 ";
		else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1)
		{
			OSString = "Microsoft Windows XP ";
		}
		else if (osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2)
			OSString = "Microsoft Windows Server 2003 family ";
		
		// Test for workstation versus server.
/* can't access to product type
		if( bOsVersionInfoEx )
		{
			if ( osvi.wProductType == VER_NT_WORKSTATION )
			OSString += "Professional ";

			if ( osvi.wProductType == VER_NT_SERVER )
			OSString += "Server ";
		}
		else
*/		{
			HKEY hKey;
			char szProductType[80];
			DWORD dwBufLen;

			RegOpenKeyEx( HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\ProductOptions", 0, KEY_QUERY_VALUE, &hKey );
			RegQueryValueEx( hKey, "ProductType", NULL, NULL, (LPBYTE) szProductType, &dwBufLen);
			RegCloseKey( hKey );
			if ( lstrcmpi( "WINNT", szProductType) == 0 )
				OSString += "Workstation ";
			if ( lstrcmpi( "SERVERNT", szProductType) == 0 )
				OSString += "Server ";
		}

		// Display version, service pack (if any), and build number.
		smprintf(ver, 1024, "version %d.%d '%s' (Build %d)", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
		OSString += ver;
		break;

	case VER_PLATFORM_WIN32_WINDOWS:

		if(osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0)
		{
			OSString = "Microsoft Windows 95 ";
			if(osvi.szCSDVersion[0] == 'B' || osvi.szCSDVersion[0] == 'C')
				OSString += "OSR2 ";
		}
		else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10)
		{
			OSString = "Microsoft Windows 98 ";
			if(osvi.szCSDVersion[0] == 'A')
				OSString += "SE ";
		}
		else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90)
			OSString = "Microsoft Windows Me ";
		else
			OSString = "Microsoft Unknown dwMajorVersion="+toString((int)osvi.dwMajorVersion)+" dwMinorVersion="+toString((int)osvi.dwMinorVersion);


		// Display version, service pack (if any), and build number.
		smprintf(ver, 1024, "version %d.%d %s (Build %d)", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.szCSDVersion, osvi.dwBuildNumber & 0xFFFF);
		OSString += ver;
		break;

	case VER_PLATFORM_WIN32s:
		OSString = "Microsoft Win32s";
		break;
	}

#elif defined SIP_OS_UNIX

	int fd = open("/proc/version", O_RDONLY);
	if (fd == -1)
	{
		sipwarning ("SI: Can't get OS from /proc/version: %s", strerror (errno));
	}
	else
	{
		char buffer[4096+1];
		int len = read(fd, buffer, sizeof(buffer)-1);
		close(fd);
		
		// remove the \n and set \0
		buffer[len-1] = '\0';
		
		OSString = buffer;
	}
	
#endif	// SIP_OS_UNIX

	return normalizeString(OSString);
}
#endif // old getOS() function

string CSystemInfo::getProc (bool bNotFull)
{
	string ProcString = "Unknown";

#ifdef SIP_OS_WINDOWS

	LONG result;
	char value[1024];
	DWORD valueSize;
	HKEY hKey;

	result = ::RegOpenKeyExA (HKEY_LOCAL_MACHINE, "Hardware\\Description\\System\\CentralProcessor\\0", 0, KEY_QUERY_VALUE, &hKey);
	if (result == ERROR_SUCCESS)
	{
		// get processor name
		valueSize = 1024;
		result = ::RegQueryValueExA (hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)value, &valueSize);
		if (result == ERROR_SUCCESS)
			ProcString = value;
		else
			ProcString = "UnknownProc";

		if ( bNotFull )
		{
			// Remove begining spaces
			ProcString = ProcString.substr (ProcString.find_first_not_of (" "));
			return normalizeString(ProcString);
		}

		ProcString += " / ";

		// get processor identifier
		valueSize = 1024;
		result = ::RegQueryValueExA (hKey, "Identifier", NULL, NULL, (LPBYTE)value, &valueSize);
		if (result == ERROR_SUCCESS)
			ProcString += value;
		else
			ProcString += "UnknownIdentifier";

		ProcString += " / ";

		// get processor vendor
		valueSize = 1024;
		result = ::RegQueryValueExA (hKey, "VendorIdentifier", NULL, NULL, (LPBYTE)value, &valueSize);
		if (result == ERROR_SUCCESS)
			ProcString += value;
		else
			ProcString += "UnknownVendor";

		ProcString += " / ";
		
		// get processor frequence
		result = ::RegQueryValueExA (hKey, "~MHz", NULL, NULL, (LPBYTE)value, &valueSize);
		if (result == ERROR_SUCCESS)
		{
			ProcString += itoa (*(int *)value, value, 10);
			ProcString += "MHz";
		}
		else
			ProcString += "UnknownFreq";
	}

	// Make sure to close the reg key
	RegCloseKey (hKey);
	
	// count the number of processor (max 8, in case this code don't work well)
	uint	numProc= 1;
	for(uint i=1;i<8;i++)
	{
		string	tmp= string("Hardware\\Description\\System\\CentralProcessor\\") + toStringA(i);

		// try to open the key
		result = ::RegOpenKeyExA (HKEY_LOCAL_MACHINE, tmp.c_str(), 0, KEY_QUERY_VALUE, &hKey);
		// Make sure to close the reg key
		RegCloseKey (hKey);

		if(result == ERROR_SUCCESS)
			numProc++;
		else
			break;
	}
	ProcString += " / ";
	ProcString += toStringA(numProc) + " Processors found";

	
#elif defined SIP_OS_MAC
	
	ProcString = getsysctlstr("machdep.cpu.brand_string");
	ProcString += " / ";
	ProcString += getsysctlstr("hw.machine");
	ProcString += " Family " + toStringA(getsysctlnum("machdep.cpu.family"));
	ProcString += " Model " + toStringA(getsysctlnum("machdep.cpu.model"));
	ProcString += " Stepping " + toStringA(getsysctlnum("machdep.cpu.stepping"));
	ProcString += " / ";
	ProcString += getsysctlstr("machdep.cpu.vendor");
	ProcString += " / ";
	ProcString += toStringA(getsysctlnum64("hw.cpufrequency")/1000000)+"MHz";
	ProcString += " / ";
	ProcString += toStringA(getsysctlnum("hw.ncpu")) + " Processors found";
	
#elif defined SIP_OS_UNIX
	
	ProcString = getCpuInfo("model name");
	ProcString += " / ?";
	ProcString += " Family " + getCpuInfo("cpu family");
	ProcString += " Model " + getCpuInfo("model");
	ProcString += " Stepping " + getCpuInfo("stepping");
	ProcString += " / ";
	ProcString += getCpuInfo("vendor_id");
	ProcString += " / ";
	ProcString += getCpuInfo("cpu MHz")+"MHz";
	ProcString += " / ";
	ProcString += "? Processors found";
	
#endif
	
	// Remove begining spaces
	ProcString = ProcString.substr (ProcString.find_first_not_of (" "));

	return normalizeString(ProcString);
}

uint64 CSystemInfo::getProcessorFrequency(bool quick)
{
	static uint64 freq = 0;
#ifdef	SIP_CPU_INTEL
	static bool freqComputed = false;	
	if (freqComputed) return freq;
	
	if (!quick)
	{
		TTicks bestNumTicks   = 0;
		uint64 bestNumCycles;
		uint64 numCycles;
		const uint numSamples = 5;
		const uint numLoops   = 50000000;
		
		volatile uint k; // prevent optimisation for the loop
		for(uint l = 0; l < numSamples; ++l)
		{	
			TTicks startTick = SIPBASE::CTime::getPerformanceTime();
			uint64 startCycle = rdtsc();
			volatile uint dummy = 0;
			for(k = 0; k < numLoops; ++k)
			{		
				++ dummy;
			}		
			numCycles = rdtsc() - startCycle;
			TTicks numTicks = SIPBASE::CTime::getPerformanceTime() - startTick;
			if (numTicks > bestNumTicks)
			{		
				bestNumTicks  = numTicks;
				bestNumCycles = numCycles;
			}
		}
		freq = (uint64) ((double) bestNumCycles * 1 / CTime::ticksToSecond(bestNumTicks));
	}
	else
	{
		TTicks timeBefore = SIPBASE::CTime::getPerformanceTime();
		uint64 tickBefore = rdtsc();
		sipSleep (100);
		TTicks timeAfter = SIPBASE::CTime::getPerformanceTime();
		TTicks tickAfter = rdtsc();
		
		double timeDelta = CTime::ticksToSecond(timeAfter - timeBefore);
		TTicks tickDelta = tickAfter - tickBefore;
		
		freq = (uint64) ((double)tickDelta / timeDelta);
	}
	
	sipinfo ("SI: CSystemInfo: Processor frequency is %.0f MHz", (float)freq/1000000.0);
	freqComputed = true;
#endif // SIP_CPU_INTEL
	return freq;
}

static bool DetectMMX()
{		
#ifdef SIP_CPU_INTEL	
		if (!CSystemInfo::hasCPUID()) return false; // cpuid not supported ...

		uint32 result = 0;
	#ifdef SIP_OS_WINDOWS
		__asm
		{
			 mov  eax,1
			 cpuid
			 test edx,0x800000  // bit 23 = MMX instruction set
			 je   noMMX
			 mov result, 1	
			noMMX:
		}

	#elif SIP_OS_UNIX
		__asm__ __volatile__ (
			  "movl   $1, %%eax;"
			  "cpuid;"
			  "movl   $0, %0;"
			  "testl  $0x800000, %%edx;"
			  "je     NoMMX;"
			  "movl   $1, %0;"
			  "NoMMX:;"
			  :"=b"(result)
			  );
	#endif // SIP_OS_UNIX
	
		return result == 1;
 
		// printf("mmx detected\n");

#else //SIP_CPU_INTEL
		return false;
#endif //SIP_CPU_INTEL
}


static bool DetectSSE()
{	
	#ifdef SIP_CPU_INTEL
		if (!CSystemInfo::hasCPUID()) return false; // cpuid not supported ...

		uint32 result = 0;
		#ifdef SIP_OS_WINDOWS
		__asm
		{			
			mov eax, 1   // request for feature flags
			cpuid 							
			test EDX, 002000000h   // bit 25 in feature flags equal to 1
			je noSSE
			mov result, 1  // sse detected
		noSSE:
		}
		#elif SIP_OS_UNIX // SIP_OS_WINDOWS
			__asm__ __volatile__ (
				  "movl   $1, %%eax;"
				  "cpuid;"
				  "movl   $0, %0;"
				  "test   $0x002000000, %%edx;"
				  "je     NoSSE;"
				  "mov    $1, %0;"
				  "NoSSE:;"
				  :"=b"(result)
				  );
		#endif // SIP_OS_UNIX
	

		if (result)
		{
			// check OS support for SSE
			try 
			{
				#ifdef SIP_OS_WINDOWS
				__asm
				{
					xorps xmm0, xmm0  // Streaming SIMD Extension
				}
				#elif SIP_OS_UNIX
				__asm__ __volatile__ ("xorps %xmm0, %xmm0;");
				#endif // SIP_OS_UNIX
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
	#else //SIP_CPU_INTEL
		return false;
	#endif //SIP_CPU_INTEL
}

bool CSystemInfo::_HaveMMX = DetectMMX ();
bool CSystemInfo::_HaveSSE = DetectSSE ();

bool CSystemInfo::hasCPUID ()
{
	#ifdef SIP_CPU_INTEL
		 uint32 result;
		#ifdef SIP_OS_WINDOWS
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
		#elif SIP_OS_UNIX // SIP_OS_WINDOWS
			__asm__ __volatile__ (
				  /* Save Register */
				  "pushl  %%ebp;"
				  "pushl  %%ebx;"
				  "pushl  %%edx;"
				  
				  /* Check if this CPU supports cpuid */
				  "pushf;"
				  "pushf;"
				  "popl   %%eax;"
				  "movl   %%eax, %%ebx;"
				  "xorl   $(1 << 21), %%eax;"	// CPUID bit
				  "pushl  %%eax;"
				  "popf;"
				  "pushf;"
				  "popl   %%eax;"
				  "popf;"                  	// Restore flags
				  "xorl   %%ebx, %%eax;"
				  "jz     NoCPUID;"
				  "movl   $1, %0;"
				  "jmp    CPUID;"
				  
				  "NoCPUID:;"
				  "movl   $0, %0;" 
				  "CPUID:;"
				  "popl   %%edx;"
				  "popl   %%ebx;"
				  "popl   %%ebp;"
				  
				  :"=a"(result)
				  ); 
		#endif // SIP_OS_UNIX
		return result == 1;
	#else	//SIP_CPU_INTEL
		 return false;
	#endif	//SIP_CPU_INTEL
}


uint32 CSystemInfo::getCPUID()
{
#ifdef SIP_CPU_INTEL
	if(hasCPUID())
	{
		uint32 result = 0;
		#ifdef NL_OS_WINDOWS
		__asm
		{
			mov  eax,1
			cpuid
			mov result, edx
		}
		#elif SIP_OS_UNIX // SIP_OS_WINDOWS
			__asm__ __volatile__ (
				  "movl   $0, %0;"
				  "movl   $1, %%eax;"
				  "cpuid;"
				  :"=d"(result)
				  );
		#endif // SIP_OS_UNIX
		return result;
	}
	else
#endif	// SIP_CPU_INTEL
		return 0;
}

bool CSystemInfo::hasHyperThreading()
{
#ifdef SIP_OS_WINDOWS
	if(hasCPUID())
	{
		unsigned int reg_eax = 0;
		unsigned int reg_edx = 0;
		unsigned int vendor_id[3] = {0, 0, 0};
		__asm 
		{
			xor eax, eax
			cpuid
			mov vendor_id, ebx
			mov vendor_id + 4, edx
			mov vendor_id + 8, ecx
			mov eax, 1
			cpuid
			mov reg_eax, eax
			mov reg_edx, edx
		}

		// pentium 4 or later processor?
		if ( (((reg_eax & 0x0f00) == 0x0f00) || (reg_eax & 0x0f00000)) &&
			(vendor_id[0] == 'uneG') && (vendor_id[1] == 'Ieni') && (vendor_id[2] == 'letn') )
			return (reg_edx & 0x10000000)!=0; // Intel Processor Hyper-Threading

		return false;
	}
	else
#endif
		return false;
}

bool CSystemInfo::isNT()
{
#ifdef SIP_OS_WINDOWS
	OSVERSIONINFOA ver;
	ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
	GetVersionExA(&ver);
	return ver.dwPlatformId == VER_PLATFORM_WIN32_NT;
#else
	return false;
#endif
}

string CSystemInfo::availableHDSpace (const string &filename)
{
#ifdef SIP_OS_UNIX
    string cmd = "df ";
    if(filename.empty())
        cmd += ".";
    else
        cmd += filename;
    cmd += " >/tmp/siphdfs";
    system (cmd.c_str());

    int fd = open("/tmp/siphdfs", O_RDONLY);
    if (fd == -1)
    {
        return 0;
    }
    else
    {
        char buffer[4096+1];
        int len = read(fd, buffer, sizeof(buffer)-1);
        close(fd);
        buffer[len] = '\0';

        vector<string> splitted;
        explode(buffer,"\n", splitted, true);

        if(splitted.size() < 2)
            return "NoInfo";

        vector<string> sline;
        explode(splitted[1]," ", sline, true);

        if(sline.size() < 5)
            return splitted[1];

        string space = sline[3] + "000";
        return bytesToHumanReadable(space);
    }
#else
    return "NoInfo";
#endif
}


uint64 CSystemInfo::availablePhysicalMemory ()
{
#ifdef SIP_OS_WINDOWS

	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx (&ms);
	return ms.ullAvailPhys;

#elif defined SIP_OS_MAC
	return uint64(getsysctlnum64("hw.usermem"));

#elif defined SIP_OS_UNIX
	return getSystemMemory("MemFree:")+getSystemMemory("Buffers:")+getSystemMemory("Cached:");
#else
	return 0;

#endif
}

uint64 CSystemInfo::totalPhysicalMemory ()
{
#ifdef SIP_OS_WINDOWS

	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx (&ms);
	return ms.ullTotalPhys;
#elif defined SIP_OS_MAC
	return uint64(getsysctlnum64("hw.physmem"));
#elif defined SIP_OS_UNIX
	return getSystemMemory("MemTotal:");
#endif
	return 0;
}

#ifndef SIP_OS_WINDOWS
static inline char *skipWS(const char *p)
{
    while (isspace(*p)) p++;
    return (char *)p;
}

static inline char *skipToken(const char *p)
{
    while (isspace(*p)) p++;
    while (*p && !isspace(*p)) p++;
    return (char *)p;
}
#endif

uint32 CSystemInfo::getAllocatedSystemMemory ()
{
	uint systemMemory = 0;
#ifdef SIP_OS_WINDOWS
	// Get system memory informations
	HANDLE hHeap[100];
	DWORD heapCount = GetProcessHeaps (100, hHeap);

	uint heap;
	for (heap = 0; heap < heapCount; heap++)
	{
		PROCESS_HEAP_ENTRY entry;
		entry.lpData = NULL;
		while (HeapWalk (hHeap[heap], &entry))
		{
			if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			{
				systemMemory += entry.cbData + entry.cbOverhead;
			}
		}
	}

#elif defined SIP_OS_UNIX
	
	int fd = open("/proc/self/stat", O_RDONLY);
	if (fd == -1)
	{
		sipwarning ("HA: Can't get OS from /proc/self/stat: %s", strerror (errno));
	}
	else
	{
		char buffer[4096], *p;
		int len = read(fd, buffer, sizeof(buffer)-1);
		close(fd);
	
		buffer[len] = '\0';
		
		p = buffer;
		p = strchr(p, ')')+1;			/* skip pid */
		p = skipWS(p);
		p++;
		
		p = skipToken(p);				/* skip ppid */
		p = skipToken(p);				/* skip pgrp */
		p = skipToken(p);				/* skip session */
		p = skipToken(p);				/* skip tty */
		p = skipToken(p);				/* skip tty pgrp */
		p = skipToken(p);				/* skip flags */
		p = skipToken(p);				/* skip min flt */
		p = skipToken(p);				/* skip cmin flt */
		p = skipToken(p);				/* skip maj flt */
		p = skipToken(p);				/* skip cmaj flt */
		p = skipToken(p);				/* utime */
		p = skipToken(p);				/* stime */
		p = skipToken(p);				/* skip cutime */
		p = skipToken(p);				/* skip cstime */
		p = skipToken(p);				/* priority */
		p = skipToken(p);				/* nice */
		p = skipToken(p);				/* skip timeout */
		p = skipToken(p);				/* skip it_real_val */
		p = skipToken(p);				/* skip start_time */
		
		systemMemory = strtoul(p, &p, 10);	/* vsize in bytes */
	}

#endif // SIP_OS_WINDOWS
	return systemMemory;
}


SIPBASE_CATEGORISED_DYNVARIABLE(sip, string, AvailableHDSpace, "Hard drive space left in bytes")
{
	// ace: it's a little bit tricky, if you don't understand how it works, don't touch!
	static string location;
	if (get)
	{
		*pointer = (CSystemInfo::availableHDSpace(location));
		location = "";
	}
	else
	{
		location = *pointer;
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(sip, uint32, AvailablePhysicalMemory, "Physical memory available on this computer in bytes(M)")
{
	if (get) 
	{
		uint64	uBytes = CSystemInfo::availablePhysicalMemory ();

		*pointer = (uint32)(uBytes / 1024 / 1024);
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(sip, uint32, TotalPhysicalMemory, "Total physical memory on this computer in bytes(M)")
{
	if (get) 
	{
		uint64	uBytes = CSystemInfo::totalPhysicalMemory ();
		*pointer = (uint32)(uBytes / 1024 / 1024);
	}
}

SIPBASE_CATEGORISED_COMMAND(sip, memoryState, "display the state of memory", "")
{
	uint64	uABytes = CSystemInfo::availablePhysicalMemory ();
	uint64	uTBytes = CSystemInfo::totalPhysicalMemory ();

	uint32	uAM = (uint32)(uABytes / 1024 / 1024);
	uint32	uTM = (uint32)(uTBytes / 1024 / 1024);

	uint32	uPercent = (uAM * 100) / uTM;
	log.displayNL ("Total memory = %d(M), Available memory = %d(M), Available percent = %d(%c)", uTM, uAM, uPercent, '%');
	return true;
}

SIPBASE_CATEGORISED_DYNVARIABLE(sip, string, ProcessUsedMemory, "Memory used by this process in bytes")
{
	if (get) *pointer = bytesToHumanReadable(CSystemInfo::getAllocatedSystemMemory ());
}

SIPBASE_CATEGORISED_DYNVARIABLE(sip, string, OS, "OS used")
{
	if (get) *pointer = CSystemInfo::getOS();
}
static	bool bFirstUsage = true;
SIPBASE_CATEGORISED_DYNVARIABLE(sip, uint32, CPUUsage, "CPU usage(%) like task manager")
{
#ifdef	SIP_OS_WINDOWS
	if (get) 
	{
		CPU cpu1, cpu2;
		int sys1, sys2;
		int process1, process2;
		TKTime upTime1, upTime2;
		process1 = cpu1.GetUsage( &sys1, &upTime1 );

		if (bFirstUsage)
		{
			sipSleep(200);
			process2 = cpu2.GetUsage( &sys2, &upTime2 );
			*pointer = sys2;
		}
		else
			*pointer = sys1;
		bFirstUsage = false;
	}
#else	//SIP_OS_WINDOWS
	sipinfo("Sorry, CPU Usage for linux is not prepared yet!"); // KSR_ADD 2010_2_2
#endif	//SIP_OS_WINDOWS
}

#ifdef SIP_OS_WINDOWS
struct DISPLAY_DEVICE_EX
{
    DWORD  cb;
    CHAR  DeviceName[32];
    CHAR  DeviceString[128];
    DWORD  StateFlags;
    CHAR  DeviceID[128];
    CHAR  DeviceKey[128];
};
#endif // SIP_OS_WINDOWS

bool CSystemInfo::getVideoInfo (std::string &deviceName, uint64 &driverVersion)
{
#ifdef SIP_OS_WINDOWS
	/* Get the device name with EnumDisplayDevices (doesn't work under win95).
	 * Look for driver information for this device in the registry
	 *
	 * Follow the recommandations in the news group comp.os.ms-windows.programmer.nt.kernel-mode : "Get Video Driver ... Need Version"
	 */

	bool debug = false;
#ifdef _DEBUG 
	debug = true;
#endif _DEBUG 

	HMODULE hm = GetModuleHandle(TEXT("USER32"));
	if (hm)
	{
		BOOL (WINAPI* EnumDisplayDevices)(LPCSTR lpDevice, DWORD iDevNum, PDISPLAY_DEVICE lpDisplayDevice, DWORD dwFlags) = NULL;
		*(FARPROC*)&EnumDisplayDevices = GetProcAddress(hm, "EnumDisplayDevicesA");
		if (EnumDisplayDevices)
		{
			DISPLAY_DEVICE_EX DisplayDevice;
			uint device = 0;
			DisplayDevice.cb = sizeof (DISPLAY_DEVICE_EX);
			bool found = false;
			while (EnumDisplayDevices(NULL, device, (DISPLAY_DEVICE*)&DisplayDevice, 0))
			{
				// Main board ?
				if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE) &&
					(DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) &&
					((DisplayDevice.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) == 0) &&
					(DisplayDevice.DeviceKey[0] != 0))
				{
					found = true;

					// The device name
					deviceName = DisplayDevice.DeviceString;

					string keyPath = DisplayDevice.DeviceKey;
					string keyName;

					// Get the window version
					OSVERSIONINFOA ver;
					ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
					GetVersionExA(&ver);
					bool atleastNT4 = (ver.dwMajorVersion  > 3) && (ver.dwPlatformId == VER_PLATFORM_WIN32_NT);
					bool winXP = ((ver.dwMajorVersion == 5) && (ver.dwMinorVersion == 1)) || (ver.dwMajorVersion > 5);

					// * Get the registry entry for driver

					// OSversion >= osWin2k
					if (atleastNT4)
					{
						if (winXP)
						{
							int pos = keyPath.rfind ('\\');
							if (pos != string::npos)
								keyPath = keyPath.substr (0, pos+1);
							keyPath += "Video";
							keyName = "Service";
						}
						else
						{
							int pos = toLower(keyPath).find ("\\device");
							if (pos != string::npos)
								keyPath = keyPath.substr (0, pos+1);
							keyName = "ImagePath";
						}
					}
					else // Win 9x
					{
						keyPath += "\\default";
						keyName = "drv";
					}

					// Format the key path
					if (toLower(keyPath).find ("\\registry\\machine") == 0)
					{
						keyPath = "HKEY_LOCAL_MACHINE" + keyPath.substr (strlen ("\\registry\\machine"));
					}

					// Get the root key
					static const char *rootKeys[]=
					{
						"HKEY_CLASSES_ROOT\\",
						"HKEY_CURRENT_CONFIG\\",
						"HKEY_CURRENT_USER\\",
						"HKEY_LOCAL_MACHINE\\",
						"HKEY_USERS\\",
						"HKEY_PERFORMANCE_DATA\\",
						"HKEY_DYN_DATA\\"
					};
					static const HKEY rootKeysH[]=
					{
						HKEY_CLASSES_ROOT,
						HKEY_CURRENT_CONFIG,
						HKEY_CURRENT_USER,
						HKEY_LOCAL_MACHINE,
						HKEY_USERS,
						HKEY_PERFORMANCE_DATA,
						HKEY_DYN_DATA,
					};
					uint i;
					HKEY keyRoot = HKEY_LOCAL_MACHINE;
					for (i=0; i<sizeof(rootKeysH)/sizeof(HKEY); i++)
					{
						if (toUpper(keyPath).find (rootKeys[i]) == 0)
						{
							keyPath = keyPath.substr (strlen (rootKeys[i]));
							keyRoot = rootKeysH[i];
							break;
						}
					}

					// * Read the registry 
					HKEY baseKey;
					if (RegOpenKeyExA(keyRoot, keyPath.c_str(), 0, KEY_READ, &baseKey) == ERROR_SUCCESS)
					{
						DWORD valueType;
						char value[512];
						DWORD size = 512;
						if (RegQueryValueExA(baseKey, keyName.c_str(), NULL, &valueType, (unsigned char *)value, &size) == ERROR_SUCCESS)
						{
							// Null ?
							if (value[0] != 0)
							{
								bool ok = !winXP;
								if (winXP)
								{
									// In Windows'XP we got service name -> not real driver name, so
									string xpKey = string ("System\\CurrentControlSet\\Services\\")+value;
									if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, xpKey.c_str(), 0, KEY_READ, &baseKey) == ERROR_SUCCESS)
									{
										size = 512;
										if (RegQueryValueExA(baseKey, "ImagePath", NULL, &valueType, (unsigned char *)value, &size) == ERROR_SUCCESS)
										{
											if (value[0] != 0)
											{
												ok = true;
											}
											else
												sipwarning ("CSystemInfo::getVideoInfo : empty value ImagePath in key %s", xpKey.c_str());
										}
										else
											sipwarning ("CSystemInfo::getVideoInfo : can't query ImagePath in key %s", xpKey.c_str());
									}
									else
										sipwarning ("CSystemInfo::getVideoInfo : can't open key %s", xpKey.c_str());
								}

								// Version dll link
								HMODULE hmVersion = LoadLibraryA ("version");
								if (hmVersion)
								{
									BOOL (WINAPI* _GetFileVersionInfo)(LPSTR, DWORD, DWORD, LPVOID) = NULL;
									DWORD (WINAPI* _GetFileVersionInfoSize)(LPSTR, LPDWORD) = NULL;
									BOOL (WINAPI* _VerQueryValue)(const LPVOID, LPSTR, LPVOID*, PUINT) = NULL;
									*(FARPROC*)&_GetFileVersionInfo = GetProcAddress(hmVersion, "GetFileVersionInfoA");
									*(FARPROC*)&_GetFileVersionInfoSize = GetProcAddress(hmVersion, "GetFileVersionInfoSizeA");
									*(FARPROC*)&_VerQueryValue = GetProcAddress(hmVersion, "VerQueryValueA");
									if (_VerQueryValue && _GetFileVersionInfoSize && _GetFileVersionInfo)
									{
										// value got the path to the driver
										string driverName = value;
										if (atleastNT4)
										{
											bool b = (GetWindowsDirectoryA(value, 512) != 0);
											sipverify (b);
										}
										else
										{
											bool b = (GetSystemDirectoryA(value, 512) != 0);
											sipverify (b);
										}
										driverName = string (value) + "\\" + driverName;

										DWORD dwHandle;
										DWORD size = _GetFileVersionInfoSize ((char*)driverName.c_str(), &dwHandle);
										if (size)
										{
											vector<uint8> buffer;
											buffer.resize (size);
											if (_GetFileVersionInfo((char*)driverName.c_str(), dwHandle, size, &buffer[0]))
											{
												VS_FIXEDFILEINFO *info;
												UINT len;
												if (_VerQueryValue(&buffer[0], "\\", (VOID**)&info, &len))
												{
													driverVersion = (((uint64)info->dwFileVersionMS)<<32)|info->dwFileVersionLS;
													return true;
												}
												else
													sipwarning ("CSystemInfo::getVideoInfo : VerQueryValue fails (%s)", driverName.c_str());
											}
											else
												sipwarning ("CSystemInfo::getVideoInfo : GetFileVersionInfo fails (%s)", driverName.c_str());
										}
										else
											sipwarning ("CSystemInfo::getVideoInfo : GetFileVersionInfoSize == 0 (%s)", driverName.c_str());
									}
									else
										sipwarning ("CSystemInfo::getVideoInfo : No VerQuery, GetFileVersionInfoSize, GetFileVersionInfo functions");
								}
								else
									sipwarning ("CSystemInfo::getVideoInfo : No version dll");
							}
							else
								sipwarning ("CSystemInfo::getVideoInfo : empty value %s in key %s", keyName.c_str(), keyPath.c_str());
						}
						else
							sipwarning ("CSystemInfo::getVideoInfo : can't query value %s in key %s", keyName.c_str(), keyPath.c_str());
					}
					else
						sipwarning ("CSystemInfo::getVideoInfo : can't open key %s", keyPath.c_str());
				}
				device++;
			}
			if (!found)
				sipwarning ("CSystemInfo::getVideoInfo : No primary display device found");
		}
		else
			sipwarning ("CSystemInfo::getVideoInfo : No EnumDisplayDevices function");
	}
	else
		sipwarning ("CSystemInfo::getVideoInfo : No user32 dll");

#endif // SIP_OS_WINDOWS

	// Fails
	return false;
}

uint64 CSystemInfo::virtualMemory ()
{
#ifdef SIP_OS_WINDOWS

	MEMORYSTATUSEX ms;
	ms.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx (&ms);
	return ms.ullTotalVirtual - ms.ullAvailVirtual;

#endif

	return 0;
}

bool	CSystemInfo::IsSystemService(std::string serviceName)
{
#ifdef	SIP_OS_WINDOWS // KSR_ADD 2010_2_2
	SC_HANDLE	hscManager;
	SC_HANDLE	hscService;
	ucstring	ucServiceName(serviceName);
	hscManager = ::OpenSCManager( NULL, SERVICES_ACTIVE_DATABASE,SC_MANAGER_ALL_ACCESS );
	if (hscManager == NULL)
		return false;
	hscService = ::OpenService( hscManager, ucServiceName.c_str(), SERVICE_ALL_ACCESS );
	if (hscService == NULL)
	{
		CloseServiceHandle(hscManager);
		return false;
	}
	CloseServiceHandle(hscService);
	CloseServiceHandle(hscManager);
	return true;
#else	// SIP_OS_WINDOWS
	return false;
#endif	// SIP_OS_WINDOWS
}
} // SIPBASE
