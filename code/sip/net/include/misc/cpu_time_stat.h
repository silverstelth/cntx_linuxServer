/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CPU_TIME_STAT_H__
#define __CPU_TIME_STAT_H__

#include "misc/types_sip.h"
#include "misc/time_sip.h"
#include <deque>

namespace SIPBASE
{

/**
 * Utility class to read cpu time information from /proc/stat and /cpu/pid/stat
 * Allows accurate timing measures for both cpu and process (at least at OS timing accuracy)
 * Call peekMeasures() once in a while (once a second, for instance, to avoid to much load)
 * then call each getCPU... and getProcess... methods to get instant CPU or Process load.
 *
 * \date 2004
 */
class CCPUTimeStat
{
public:

	CCPUTimeStat();

	/**
	 * Get absolute ticks value for the whole cpu set
	 * e.g. a foursome of cpu will all tell their ticks through these numbers
	 */
	static bool		getCPUTicks(uint64& user, uint64& nice, uint64& system, uint64& idle, uint64& iowait);

	/// Get absolute ticks values for a specific pid
	static bool		getPIDTicks(uint64& utime, uint64& stime, uint64& cutime, uint64& cstime, uint pid);



	/// \name Measure peeking and reading
	// @{

	/// Peek measure
	void			peekMeasures();

	enum TMeasureType
	{
		Instant,
		Mean,
		Peak
	};

	/**
	 * Get Overall CPU Load
	 * Call this method to know the overall server CPU usage (0=min, 1=max)
	 * All processes running on the same machine will return approximately
	 * the same value
	 */
	float			getCPULoad(TMeasureType type = Instant) const				{ return _CPUUser.get(type)+_CPUSystem.get(type)+_CPUNice.get(type)+_CPUIOWait.get(type); }

	/**
	 * Get overall load for this process (children excluded)
	 * Call this method to know the CPU usage for this process (0=min, 1=max)
	 * Please note that quadriprocessor machines will show maximum of 0.25 per process
	 * (0.5 for a biprocessor).
	 * Children of a process are processes that have been launched from this process
	 * (that is, this process is marked as parent of the child process, see ppid, fork() and/or exec())
	 * Unused but by the AES (all launched services are children of the AES)
	 */
	float			getProcessLoad(TMeasureType type = Instant) const			{ return _PIDUTime.get(type)+_PIDSTime.get(type); }

	// @}



	/** \name Detailed measures
	 * These methods allow you to check precisely what consumes cpu power,
	 * (user processes/kernel/nice processes...)
	 */
	// @{

	/// Get User Load
	float			getCPUUserLoad(TMeasureType type = Instant) const			{ return _CPUUser.get(type); }
	/// Get System Load
	float			getCPUSystemLoad(TMeasureType type = Instant) const			{ return _CPUSystem.get(type); }
	/// Get Nice Load
	float			getCPUNiceLoad(TMeasureType type = Instant) const			{ return _CPUNice.get(type); }
	/// Get Nice Load
	float			getCPUIOWaitLoad(TMeasureType type = Instant) const			{ return _CPUIOWait.get(type); }

	/// Get User load for this process (children excluded)
	float			getProcessUserLoad(TMeasureType type = Instant) const		{ return _PIDUTime.get(type); }
	/// Get Sytem load for this process (children excluded)
	float			getProcessSystemLoad(TMeasureType type = Instant) const		{ return _PIDSTime.get(type); }
	/// Get User load for this process (children included)
	float			getProcessCUserLoad(TMeasureType type = Instant) const		{ return _PIDCUTime.get(type); }
	/// Get Sytem load for this process (children included)
	float			getProcessCSystemLoad(TMeasureType type = Instant) const	{ return _PIDCSTime.get(type); }
	/// Get overall load for this process (children included)
	float			getProcessCLoad(TMeasureType type = Instant) const			{ return _PIDCUTime.get(type)+_PIDCSTime.get(type); }



	// @}


private:

	struct CTickStat
	{
		CTickStat() : Tick(0), Diff(0), Load(0.0f)	{ }
		uint64		Tick;
		uint32		Diff;
		float		Load;

		typedef std::deque<std::pair<SIPBASE::TTime, float> >	TLoadQueue;
		TLoadQueue	LoadQueue;

		void	computeDiff(uint64 newTick)
		{
			Diff = (uint32)(newTick-Tick);
			Tick = newTick;
		}

		void	computeLoad(uint32 total, const SIPBASE::TTime& ctime)
		{
			Load = (float)Diff / (float)total;

			while (!LoadQueue.empty() && ctime-LoadQueue.front().first > 60000)
				LoadQueue.pop_front();
			LoadQueue.push_back(std::make_pair(ctime, Load));
		}

		float	get(TMeasureType type) const
		{
			if (type == Peak)
				return getPeakLoad();
			else if (type == Mean)
				return getMeanLoad();
			else
				return Load;
		}

		float	getPeakLoad() const
		{
			float	peak = 0.0f;
			uint	i;
			for (i=0; i<LoadQueue.size(); ++i)
				if (LoadQueue[i].second > peak)
					peak = LoadQueue[i].second;
			return peak;
		}

		float	getMeanLoad() const
		{
			if (LoadQueue.empty())
				return 0.0f;
			float	t = 0.0f;
			uint	i;
			for (i=0; i<LoadQueue.size(); ++i)
				t += LoadQueue[i].second;
			return t/i;
		}
	};

	uint32			_PID;
	bool			_FirstTime;

	uint64			_LastCPUTicks;
	CTickStat		_CPUUser;
	CTickStat		_CPUNice;
	CTickStat		_CPUSystem;
	CTickStat		_CPUIdle;
	CTickStat		_CPUIOWait;

	uint64			_LastPIDTicks;
	CTickStat		_PIDUTime;
	CTickStat		_PIDSTime;
	CTickStat		_PIDCUTime;
	CTickStat		_PIDCSTime;
};

}

#endif
