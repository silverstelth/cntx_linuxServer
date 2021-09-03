/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CPU_H__
#define __CPU_H__

#include "TKTime.h"

namespace SIPBASE
{
#ifdef SIP_OS_WINDOWS
typedef bool ( __stdcall * pfnGetSystemTimes)( LPFILETIME lpIdleTime, LPFILETIME lpKernelTime, LPFILETIME lpUserTime );

class CPU
{
public:
    CPU( void );
    ~CPU( void );
    
    // return :
    // % of cpu usage for this process 
    // % cpu systemUsage 
    // uptime
    int GetUsage( int* pSystemUsage, TKTime* pUpTime );
private:
    static TKDelay s_delay;

    static TKLong s_time;

    static TKLong s_idleTime;
    static TKLong s_kernelTime;
    static TKLong s_userTime;
    static int    s_lastCpu;
    static int    s_cpu[5];

    static TKLong s_kernelTimeProcess;
    static TKLong s_userTimeProcess;
    static int    s_lastCpuProcess;
    static int    s_cpuProcess[5];

    static int    s_count;
    static int    s_index;

    static TKLong s_lastUpTime;

    static HINSTANCE s_hKernel;
    static pfnGetSystemTimes s_pfnGetSystemTimes;

    CRITICAL_SECTION m_lock;
};
#elif defined(SIP_OS_UNIX) // KSR_ADD 2010_2_2
//#error	(Sorry for your inconvenience. For Linux, CPU is not prepared.)
#endif // SIP_OS_WINDOWS
}
#endif // end of guard __CPU_H__
