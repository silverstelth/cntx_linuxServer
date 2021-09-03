/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/cpu_time_stat.h"
#include "misc/common.h"

#include <string>
#include <vector>

#include <stdio.h> // <cstdio>

#ifdef SIP_OS_UNIX
#include <unistd.h>
#else
#include <process.h>
#endif

using namespace std;

namespace SIPBASE
{


// Get absolute ticks value for the whole cpu set
bool	CCPUTimeStat::getCPUTicks(uint64& user, uint64& nice, uint64& system, uint64& idle, uint64& iowait)
{
#ifdef SIP_OS_UNIX

	const char*	statfile = "/proc/stat";
	FILE*		f = fopen(statfile, "r");

	if (f == NULL)
		return false;

	// /proc/stat
			// cpu  [user]     [nice]    [system]    [idle]     [iowait]   [irq] [softirq]
	fscanf(f, "cpu  %" SIP_I64 "u %" SIP_I64 "u %" SIP_I64 "u %" SIP_I64 "u %" SIP_I64 "u", &user, &nice, &system, &idle, &iowait);

	fclose(f);

	return true;
#else
	return false;
#endif
}



// Get absolute ticks values for a specified pid
bool	CCPUTimeStat::getPIDTicks(uint64& utime, uint64& stime, uint64& cutime, uint64& cstime, uint pid)
{
#ifdef SIP_OS_UNIX

	std::string	statfile = SIPBASE::toString("/proc/%u/stat", pid);
	FILE*	f = fopen(statfile.c_str(), "r");

	if (f == NULL)
		return false;

	// /proc/<pid>/stat
			// pid com sta ppi pgi ses tty tpg fla mif maf cmi cma [utime]    [stime]    [cutime]   [cstime] ...
	fscanf(f, "%*d %*s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %" SIP_I64 "u %" SIP_I64 "u %" SIP_I64 "u %" SIP_I64 "u", &utime, &stime, &cutime, &cstime);

	fclose(f);

	return true;
#else
	return false;
#endif
}

CCPUTimeStat::CCPUTimeStat() :  _PID(0), _FirstTime(true), _LastCPUTicks(0), _LastPIDTicks(0)
{
#ifdef SIP_OS_UNIX

	_PID = getpid();

#else

	_PID = _getpid();

#endif

}


// Peek measure
void	CCPUTimeStat::peekMeasures()
{
	SIPBASE::TTime	ctime = SIPBASE::CTime::getLocalTime();

	uint64	u, n, s, i, io;
	uint64	ut, st, cut, cst;
	if (getCPUTicks(u, n, s, i, io) && getPIDTicks(ut, st, cut, cst, _PID))
	{
		uint64	cpuTicks = u+n+s+i+io;
		uint64	pidTicks = ut+st+cut+cst;

		// only compute diff
		if (_LastCPUTicks == cpuTicks || _LastPIDTicks == pidTicks)
			return;

		_LastCPUTicks = cpuTicks;
		_CPUUser.computeDiff(u);
		_CPUNice.computeDiff(n);
		_CPUSystem.computeDiff(s);
		_CPUIdle.computeDiff(i);
		_CPUIOWait.computeDiff(io);

		_LastPIDTicks = pidTicks;
		_PIDUTime.computeDiff(ut);
		_PIDSTime.computeDiff(st);
		_PIDCUTime.computeDiff(cut);
		_PIDCSTime.computeDiff(cst);

		if (!_FirstTime)
		{
			uint32	cpuTotal = _CPUUser.Diff+_CPUNice.Diff+_CPUSystem.Diff+_CPUIdle.Diff+_CPUIOWait.Diff;

			_CPUUser.computeLoad(cpuTotal, ctime);
			_CPUNice.computeLoad(cpuTotal, ctime);
			_CPUSystem.computeLoad(cpuTotal, ctime);
			_CPUIdle.computeLoad(cpuTotal, ctime);
			_CPUIOWait.computeLoad(cpuTotal, ctime);

			uint32	pidTotal = _PIDUTime.Diff+_PIDSTime.Diff+_PIDCUTime.Diff+_PIDCSTime.Diff;
			if (pidTotal > cpuTotal)
			{
				// EEK! should not happen!!
				cpuTotal = pidTotal;
			}

			_PIDUTime.computeLoad(cpuTotal, ctime);
			_PIDSTime.computeLoad(cpuTotal, ctime);
			_PIDCUTime.computeLoad(cpuTotal, ctime);
			_PIDCSTime.computeLoad(cpuTotal, ctime);
		}

		_FirstTime = false;
	}
}


}
