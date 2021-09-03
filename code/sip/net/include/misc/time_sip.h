/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TIME_SIP_H__
#define __TIME_SIP_H__

#include "types_sip.h"

#ifdef SIP_OS_WINDOWS
// automatically add the win multimedia library if you use CTime class
#	pragma comment(lib, "winmm.lib")
#endif

namespace SIPBASE
{
/// New time types
typedef double TGameTime;		// Time according to the game (used for determining day, night...) (double in seconds)
typedef uint32 TGameCycle;		// Integer game cycle count from the game (in game ticks)
typedef double TLocalTime;		// Time according to the machine's local clock (double in seconds)
typedef sint64 TCPUCycle;		// Integer cycle count from the CPU (for profiling in cpu ticks)

/// Old time type
typedef sint64 TTime;
typedef sint64 TTicks;


/**
 * This class provide a independant local time system.
 * \date 2000-2005
 */
class CTime
{
public:
	/** Return the number of second since midnight (00:00:00), January 1, 1970,
	 * coordinated universal time, according to the system clock.
	 * This values is the same on all computer if computers are synchronized (with NTP for example).
	 */
	static uint32	getSecondsSince1970 ();

	/** Return the local time in milliseconds.
	 * Use it only to measure time difference, the absolute value does not mean anything.
	 * On Unix, getLocalTime() will try to use the Monotonic Clock if available, otherwise
	 * the value can jump backwards if the system time is changed by a user or a NTP time sync process.
	 * The value is different on 2 different computers; use the CUniTime class to get a universal
	 * time that is the same on all computers.
	 * \warning On Win32, the value is on 32 bits only. It wraps around to 0 every about 49.71 days.
	 */
	static TTime	getLocalTime ();

	/** Return the time in processor ticks. Use it for profile purpose.
	 * If the performance time is not supported on this hardware, it returns 0.
	 * \warning On a multiprocessor system, the value returned by each processor may
	 * be different. The only way to workaround this is to set a processor affinity
	 * to the measured thread.
	 * \warning The speed of tick increase can vary (especially on laptops or CPUs with
	 * power management), so profiling several times and computing the average could be
	 * a wise choice.
	 */
	static TTicks	getPerformanceTime ();

	/** Convert a ticks count into second. If the performance time is not supported on this
	 * hardware, it returns 0.0.
	 */
	static double	ticksToSecond (TTicks ticks);

	/** Build a human readable string of a time difference in second.
	 *	The result will be of the form '1 years 2 months 2 days 10 seconds'
	 */
	static std::string	getHumanRelativeTimeA(sint32 nbSeconds);
	static std::basic_string<ucchar> getHumanRelativeTimeW(sint32 nbSeconds);

	/** Build a human readable string of a time difference in second.
	*	The result will be of the form '33:50:15'
	*/
	static std::string	getHumanRelativeDigitalTimeA(sint32 nbSeconds);
	static std::basic_string<ucchar> getHumanRelativeDigitalTimeW(sint32 nbSeconds);

	/** Return current system time
	 *	The return value's type is unsigned long int(64bit)
	 *	The return value means current time as number 20071009132314(year(4),month(2),day(2),hour(2),minute(2),second(2))
	 */
	static uint64	getCurrentTime();
	/** Return unsigned long int time
	*	The return value means time as number 20071009132314(year(4),month(2),day(2),hour(2),minute(2),second(2))
	*/
	static uint64	MakeLongTime(int nYear, int nMonth, int nDay, int nHour, int nMinute, int nSecond);
	
	/** 체계가 기동해서부터 흘러간 시간을 ms단위로 얻는다.
	 */
	static uint32	getTick();

	//By NamJuSong 2008/01/15
	//Return time string be of the form "yyyy'/'MM'/'dd'-'" "HH':'mm"
	static std::string	getMixedTime();
};

class	MSSQLTIME
{
public:
	MSSQLTIME();
	bool SetTime(std::string _time);

	TTime	timevalue;		// seconds elapsed from 1970.1.1 (00:00:00)
	int		GetDeltaSecondFrom(const MSSQLTIME &_diff);

	static int getTotalDays( int year, int month, int day );
	uint32	GetDaysFrom1970();
};

} // SIPBASE

#endif // __TIME_SIP_H__

/* End of time_sip.h */
