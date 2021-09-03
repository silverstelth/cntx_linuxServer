/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include <ctime>

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#elif defined (SIP_OS_UNIX)
#	include <sys/time.h>
#	include <unistd.h>
#endif

#ifdef SIP_OS_MAC
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#include "misc/time_sip.h"
#include "misc/sstring.h"
#include "misc/mutex.h"


namespace SIPBASE
{
	/* Return the number of second since midnight (00:00:00), January 1, 1970,
	 * coordinated universal time, according to the system clock.
	 * This values is the same on all computer if computers are synchronized (with NTP for example).
	 */
	uint32 CTime::getSecondsSince1970 ()
	{
		return sip_time (NULL);
	}

	/** Return the number of second since midnight (00:00:00), January 1, 1970,
	 * coordinated universal time, according to the system clock.
	 * The time returned is UTC (aka GMT+0), ie it does not have the local time ajustement
	 * nor it have the daylight saving ajustement.
	 * This values is the same on all computer if computers are synchronized (with NTP for example).
	 */
//	uint32	CTime::getSecondsSince1970UTC ()
//	{
		// get the local time
//		time_t nowLocal = time ( NULL );
		// convert it to GMT time (UTC)
//		struct tm * timeinfo;
//		timeinfo = gmtime ( &nowLocal );
//		return sip_mktime ( timeinfo );
//	}

	/* Return the local time in milliseconds.
	 * Use it only to measure time difference, the absolute value does not mean anything.
	 * On Unix, getLocalTime() will try to use the Monotonic Clock if available, otherwise
	 * the value can jump backwards if the system time is changed by a user or a NTP time sync process.
	 * The value is different on 2 different computers; use the CUniTime class to get a universal
	 * time that is the same on all computers.
	 * \warning On Win32, the value is on 32 bits only. It wraps around to 0 every about 49.71 days.
	 */
	CFairMutex		s;
	static bool		initlocaltime = false;
	static TTime	lastgettime;
	static uint32	cycletime = 0;
	static bool		bGettingLocalTime = false;
	static TTime	lastgetlocaltime;

	TTime CTime::getLocalTime ()
	{

#ifdef SIP_OS_WINDOWS

		//static bool initdone = false;
		//static bool byperfcounter;
		// Initialization
		//if ( ! initdone )
		//{
		//byperfcounter = (getPerformanceTime() != 0);
		//initdone = true;
		//}

		/* Retrieve time is ms
		 * Why do we prefer getPerformanceTime() to timeGetTime() ? Because on one dual-processor Win2k
		 * PC, we have noticed that timeGetTime() slows down when the client is running !!!
		 */
		/* Now we have noticed that on all WinNT4 PC the getPerformanceTime can give us value that
		 * are less than previous
		 */

		//if ( byperfcounter )
		//{
		//	return (TTime)(ticksToSecond(getPerformanceTime()) * 1000.0f);
		//}
		//else
		//{
		// This is not affected by system time changes. But it cycles every 49 days.

		// timeGetTime() 은 49일에 한번씩 순환한다. (왜? 32비트값이므로)
		// 그런데 써버 OS가 기동한 후 49일동안 켜있을 가능성은 충분히 있다.
		// 이로 인하여 많은 부분들이 영향을 받는다.
		// 전반적인 검토가 필요하다.
		uint32	maxuint32 = 0xFFFFFFFF;
		if (!initlocaltime)
		{
			TTime	mycurlocaltime = timeGetTime();
			lastgetlocaltime = lastgettime = mycurlocaltime;
			cycletime = 0;
			initlocaltime = true;
			return mycurlocaltime;
		}
		if (bGettingLocalTime)
			return lastgetlocaltime;
		else
			bGettingLocalTime = true;

		TTime	mycurlocaltime = timeGetTime();
		if (mycurlocaltime < lastgettime)
		{
			cycletime ++;
		}
		lastgettime = mycurlocaltime;

		TTime resulttime;
		if (cycletime > 0)
			resulttime =  mycurlocaltime +  cycletime*maxuint32;
		else
			resulttime = mycurlocaltime;

		lastgetlocaltime = resulttime;
		bGettingLocalTime = false;
		
		return resulttime;
		
		//}

#elif defined (SIP_OS_UNIX)

		static bool initdone = false;
		static bool isMonotonicClockSupported = false;
		if ( ! initdone )
		{

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
#if defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK >= 0)

			/* Initialize the local time engine.
			* On Unix, this method will find out if the Monotonic Clock is supported
			* (seems supported by kernel 2.6, not by kernel 2.4). See getLocalTime().
			*/
			struct timespec tv;
			if ( (clock_gettime( CLOCK_MONOTONIC, &tv ) == 0) &&
			     (clock_getres( CLOCK_MONOTONIC, &tv ) == 0) )
			{
				sipdebug( "Monotonic local time supported (resolution %.6f ms)", ((float)tv.tv_sec)*1000.0f + ((float)tv.tv_nsec)/1000000.0f );
				isMonotonicClockSupported = true;
			}
			else

#endif
#endif
			{
				sipwarning( "Monotonic local time not supported, caution with time sync" );
			}

			initdone = true;
		}

#if defined(_POSIX_TIMERS) && (_POSIX_TIMERS > 0)
#if defined(_POSIX_MONOTONIC_CLOCK) && (_POSIX_MONOTONIC_CLOCK >= 0)

		if ( isMonotonicClockSupported )
		{
			struct timespec tv;
			// This is not affected by system time changes.
			if ( clock_gettime ( CLOCK_MONOTONIC, &tv ) != 0 )
				siperror ( "Can't get clock time again" );
			return (TTime) tv.tv_sec * (TTime) 1000 + (TTime) ( ( tv.tv_nsec/*+500*/ ) / 1000000 );
		}

#endif
#endif

		// This is affected by system time changes.
		struct timeval tv;
		if ( gettimeofday ( &tv, NULL ) != 0 )
			siperror ( "Can't get time of day" );
		return ( TTime ) tv.tv_sec * ( TTime ) 1000 + ( TTime ) tv.tv_usec / ( TTime ) 1000;

#endif
	}


	/* Return the time in processor ticks. Use it for profile purpose.
	 * If the performance time is not supported on this hardware, it returns 0.
	 * \warning On a multiprocessor system, the value returned by each processor may
	 * be different. The only way to workaround this is to set a processor affinity
	 * to the measured thread.
	 * \warning The speed of tick increase can vary (especially on laptops or CPUs with
	 * power management), so profiling several times and computing the average could be
	 * a wise choice.
	 */
	TTicks CTime::getPerformanceTime ()
	{
#ifdef SIP_OS_WINDOWS
		LARGE_INTEGER ret;
		if ( QueryPerformanceCounter ( &ret ) )
			return ret.QuadPart;
		else
			return 0;
#elif defined(SIP_OS_MAC)	 // SIP_OS_WINDOWS
		return mach_absolute_time();
#else // SIP_OS_WINDOWS

#if defined(HAVE_X86_64)
		unsigned long long int hi, lo;
		__asm__ volatile (".byte 0x0f, 0x31" : "=a" (lo), "=d" (hi));
		return (hi << 32) | (lo & 0xffffffff);
#elif defined(HAVE_X86) and !defined(SIP_OS_MAC)
		unsigned long long int x;
		__asm__ volatile (".byte 0x0f, 0x31" : "=A" (x));
		return x;
#else // HAVE_X86
		static bool firstWarn = true;
		if (firstWarn)
		{
			sipwarning ("TTicks CTime::getPerformanceTime () is not implemented for your processor, returning 0");
			firstWarn = false;
		}
		return 0;
#endif // HAVE_X86
		
#endif // SIP_OS_WINDOWS
	}

	uint32 CTime::getTick()
	{
		// changed by ysg 2009-05-16
#ifdef SIP_OS_WINDOWS
		return (uint32)GetTickCount();
#else
		struct timespec now;
		if (clock_gettime(CLOCK_MONOTONIC, &now))
			return 0;
		
		return (uint32)(now.tv_sec * 1000 + now.tv_nsec / 1000000);
//		sipwarning ( "not tested : CTime::getTick" );
//		return ( uint32 ) ticksToSecond ( getPerformanceTime() ) * 1000;
#endif

//	return (uint32)ticksToSecond(getPerformanceTime()) * 1000;
	}

	/*
	#define GETTICKS(t) asm volatile ("push %%esi\n\t" "mov %0, %%esi" : : "r" (t)); \
	                      asm volatile ("push %eax\n\t" "push %edx"); \
	                      asm volatile ("rdtsc"); \
	                      asm volatile ("movl %eax, (%esi)\n\t" "movl %edx, 4(%esi)"); \
	                      asm volatile ("pop %edx\n\t" "pop %eax\n\t" "pop %esi");
	*/


	/* Convert a ticks count into second. If the performance time is not supported on this
	 * hardware, it returns 0.0.
	 */
	double CTime::ticksToSecond (TTicks ticks)
	{
#ifdef SIP_OS_WINDOWS
		LARGE_INTEGER ret;
		if (QueryPerformanceFrequency(&ret))
		{
			return (double)(sint64)ticks/(double)ret.QuadPart;
		}
		else
#elif defined(SIP_OS_MAC)
		{
			static mach_timebase_info_data_t tbInfo;
			if(tbInfo.denom == 0) mach_timebase_info(&tbInfo);
			return double(ticks * tbInfo.numer / tbInfo.denom)/1000000.0;
		}
#endif // SIP_OS_WINDOWS
		{
			static bool benchFrequency = true;
			static sint64 freq = 0;
			if (benchFrequency)
			{
				// try to have an estimation of the cpu frequency

				TTicks tickBefore = getPerformanceTime ();
				TTicks tickAfter = tickBefore;
				TTime timeBefore = getLocalTime ();
				TTime timeAfter = timeBefore;
				while (true)
				{
					if (timeAfter - timeBefore > 1000)
						break;
					timeAfter = getLocalTime ();
					tickAfter = getPerformanceTime ();
				}

				TTime timeDelta = timeAfter - timeBefore;
				TTicks tickDelta = tickAfter - tickBefore;

				freq = 1000 * tickDelta / timeDelta;
				benchFrequency = false;
			}

			return (double)(sint64)ticks/(double)freq;
		}
	}

	std::string	CTime::getHumanRelativeDigitalTimeA(sint32 nbSeconds)
	{
		sint32 delta = nbSeconds;
		if (delta < 0)
			delta = -delta;

		// some constants of time duration in seconds
		const uint32 oneMinute = 60;
		const uint32 oneHour = oneMinute * 60;
		uint32 hour, minute;
		hour = minute = 0;

		/// compute the different parts
		while (delta > oneHour)
		{
			++hour;
			delta -= oneHour;
		}

		while (delta > oneMinute)
		{
			++minute;
			delta -= oneMinute;
		}

		std::string	s = SIPBASE::toString("%02d:%02d:%02d", hour, minute, delta);
		return s;
		// compute the string
		CSStringA ret;

		ret << hour << " : ";
		ret << minute << " : ";
		ret << delta;

		return ret;
	}

	std::basic_string<ucchar> CTime::getHumanRelativeDigitalTimeW(sint32 nbSeconds)
	{
		sint32 delta = nbSeconds;
		if (delta < 0)
			delta = -delta;

		// some constants of time duration in seconds
		const uint32 oneMinute = 60;
		const uint32 oneHour = oneMinute * 60;

		uint32 hour, minute;
		hour = minute = 0;

		/// compute the different parts
		while (delta > oneHour)
		{
			++hour;
			delta -= oneHour;
		}

		while (delta > oneMinute)
		{
			++minute;
			delta -= oneMinute;
		}

		std::basic_string<ucchar>	s = SIPBASE::toString(L"%02d:%02d:%02d", hour, minute, delta).c_str();
		return s;

		// compute the string
		CSStringW ret;

		if (hour)
			ret << hour << L" :";
		if (minute)
			ret << minute << L" : ";
		if (delta || ret.empty())
			ret << delta;

		return ret;
	}

	std::string	CTime::getHumanRelativeTimeA(sint32 nbSeconds)
	{
		sint32 delta = nbSeconds;
		if (delta < 0)
			delta = -delta;

		// some constants of time duration in seconds
		const uint32 oneMinute = 60;
		const uint32 oneHour = oneMinute * 60;
		const uint32 oneDay = oneHour * 24;
		const uint32 oneWeek = oneDay * 7;
		const uint32 oneMonth = oneDay * 30; // aprox, a more precise value is 30.416666... but no matter
		const uint32 oneYear = oneDay * 365; // aprox, a more precise value is 365.26.. who care?

		uint32 year, month, week, day, hour, minute;
		year = month = week = day = hour = minute = 0;

		/// compute the different parts
		while (delta > oneYear)
		{
			++year;
			delta -= oneYear;
		}

		while (delta > oneMonth)
		{
			++month;
			delta -= oneMonth;
		}

		while (delta > oneWeek)
		{
			++week;
			delta -= oneWeek;
		}

		while (delta > oneDay)
		{
			++day;
			delta -= oneDay;
		}

		while (delta > oneHour)
		{
			++hour;
			delta -= oneHour;
		}

		while (delta > oneMinute)
		{
			++minute;
			delta -= oneMinute;
		}

		// compute the string
		CSStringA ret;

		if (year)
			ret << year << " years ";
		if (month)
			ret << month << " months ";
		if (week)
			ret << week << " weeks ";
		if (day)
			ret << day << " days ";
		if (hour)
			ret << hour << " hours ";
		if (minute)
			ret << minute << " minutes ";
		if (delta || ret.empty())
			ret << delta << " seconds ";

		return ret;
	}

	std::basic_string<ucchar> CTime::getHumanRelativeTimeW(sint32 nbSeconds)
	{
		sint32 delta = nbSeconds;
		if (delta < 0)
			delta = -delta;

		// some constants of time duration in seconds
		const uint32 oneMinute = 60;
		const uint32 oneHour = oneMinute * 60;
		const uint32 oneDay = oneHour * 24;
		const uint32 oneWeek = oneDay * 7;
		const uint32 oneMonth = oneDay * 30; // aprox, a more precise value is 30.416666... but no matter
		const uint32 oneYear = oneDay * 365; // aprox, a more precise value is 365.26.. who care?

		uint32 year, month, week, day, hour, minute;
		year = month = week = day = hour = minute = 0;

		/// compute the different parts
		while (delta > oneYear)
		{
			++year;
			delta -= oneYear;
		}

		while (delta > oneMonth)
		{
			++month;
			delta -= oneMonth;
		}

		while (delta > oneWeek)
		{
			++week;
			delta -= oneWeek;
		}

		while (delta > oneDay)
		{
			++day;
			delta -= oneDay;
		}

		while (delta > oneHour)
		{
			++hour;
			delta -= oneHour;
		}

		while (delta > oneMinute)
		{
			++minute;
			delta -= oneMinute;
		}

		// compute the string
		CSStringW ret;

		if (year)
			ret << year << L" years ";
		if (month)
			ret << month << L" months ";
		if (week)
			ret << week << L" weeks ";
		if (day)
			ret << day << L" days ";
		if (hour)
			ret << hour << L" hours ";
		if (minute)
			ret << minute << L" minutes ";
		if (delta || ret.empty())
			ret << delta << L" seconds ";
	
		return ret;
	}

	/** Return unsigned long int time
	*	The return value means time as number 20071009132314(year(4),month(2),day(2),hour(2),minute(2),second(2))
	*/
	uint64	CTime::MakeLongTime(int nYear, int nMonth, int nDay, int nHour, int nMinute, int nSecond)
	{
		// changed by ysg 2009-05-016
		uint64	r = nYear * 100000 * 100000 + nMonth * 10000 * 10000 + nDay * 1000000 + nHour * 10000 + nMinute * 100 + nSecond;
		return r;
	}

	/** Return current system time
	*	The return value's type is unsigned long int(64bit)
	*	The return value means current time as 20071009132314(year(4),month(2),day(2),hour(2),minute(2),second(2))
	*/
	uint64	CTime::getCurrentTime()
	{
		time_t date;
		time (&date);
		struct tm *tms = localtime(&date);

		uint64	uCurTime = 0;
		if (tms)
		{
			// changed by ysg 2009-05-016
//		uCurTime =	(tms->tm_year+1900) * 10000000000 +
//			(tms->tm_mon+1) * 100000000 +
			uCurTime =	( tms->tm_year+1900 ) * 100000 * 100000 +
			           ( tms->tm_mon+1 ) * 10000 * 10000 +
			           tms->tm_mday * 1000000 +
			           tms->tm_hour *10000 +
			           tms->tm_min * 100 +
			           tms->tm_sec;
		}
	
		return uCurTime;
	}

//By NamJuSong 2008/01/15
#define	FILENAME_DATE	"yyyy'-'MM'-'dd' '"
#define	FILENAME_TIME	"HH':'mm':'ss"

	std::string	CTime::getMixedTime()
	{
		std::string	strTime;
		// changed by ysg 2009-05-16
#ifdef SIP_OS_WINDOWS
		char str[16];
		GetDateFormatA ( LOCALE_USER_DEFAULT, NULL, NULL, FILENAME_DATE, str, 16 );
		strTime += str;
		GetTimeFormatA ( LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, NULL, FILENAME_TIME, str, 16 );
		strTime += str;
#else
		sipwarning ( "not tested : CTime::getMixedTime" );
		char str[128];
		time_t curtime;
		time(&curtime);
		tm *curtm = localtime(&curtime);
		strftime(str, 128, "%Y-%m-%d %H:%M:%S", curtm);
		strTime = str;
#endif
		return strTime;
	}

	MSSQLTIME::MSSQLTIME() : 
	timevalue(0)	{}
	// _time must be formatted on "YYYY-MM-DD HH:MM::SS"
	bool	MSSQLTIME::SetTime(std::string _time)
{
		uint16	year;
		uint16	month;
		uint16	day;
		uint16	hour;
		uint16	minute;
		uint16	second;

		timevalue = 0;
		if (_time.length() < 10)
			return false;
	
		const char *pBuffer = _time.c_str();
	
		char chYear[10], chMonth[10], chDay[10];
		memset(chYear, 0, sizeof(chYear));
		memset(chMonth, 0, sizeof(chMonth));
		memset(chDay, 0, sizeof(chDay));
	
		strncpy(chYear, pBuffer, 4);
		strncpy(chMonth, pBuffer+5, 2);
		strncpy(chDay, pBuffer+8, 2);

		year = (uint16)atoi(chYear);
		month = (uint16)atoi(chMonth);
		day = (uint16)atoi(chDay);
	
		if (_time.length() < 19)
		{
			hour = minute = second = 0;
		}
		else
		{
			char chH[10], chM[10], chS[10];
			memset(chH, 0, sizeof(chH));
			memset(chM, 0, sizeof(chM));
			memset(chS, 0, sizeof(chS));
	
			strncpy(chH, pBuffer + 11, 2);
			strncpy(chM, pBuffer + 14, 2);
			strncpy(chS, pBuffer + 17, 2);

			hour = (uint16)atoi(chH);
			minute = (uint16)atoi(chM);
			second = (uint16)atoi(chS);
		}

	if ( year >= 1970 )
	{
		int total = getTotalDays( year, month, day );
		timevalue = (TTime)(((TTime)(total) - 719529) * 86400 + (TTime)(hour) * 3600 + (TTime)(minute) * 60 + (TTime)(second));
	}
		
	else
		timevalue = 0;

		return true;
	}

	// Get time interval for second
	int	MSSQLTIME::GetDeltaSecondFrom(const MSSQLTIME &_diff)
	{
		return (int)(timevalue - _diff.timevalue);
	}

	int MSSQLTIME::getTotalDays( int year, int month, int day )
	{
		static int monthDay[12] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	
		// base days
		int total = year * 365;
	
		// add this year days elapsed
		int i, iDays = day + 1;
		for ( i = 1;i < month; i ++ )
		{
			iDays += monthDay[ i - 1 ];
			if ( (i == 2) && (year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) )
				iDays += 1;
		}
		total += iDays;
	
		// add over days
		if ( year > 0 )
		{
			year --;
			total += (year/4 - year/100 + year/400);
		}
	
		return total;
	}
	uint32 MSSQLTIME::GetDaysFrom1970()
	{
		return (uint32)(timevalue / 86400); // 60*60*24(seconds per one day)
	}

} // SIPBASE
