/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STOP_WATCH_H__
#define __STOP_WATCH_H__

#include "types_sip.h"
#include "time_sip.h"
#include <deque>


namespace SIPBASE {


typedef uint32 TTickDuration;
typedef uint32 TMsDuration;


/**
 * Stopwatch class used for performance measurements and statistics.
 * To measure the duration of a cycle, call stop(), get the results, then call start().
 * \date 2001
 */
class CStopWatch
{
public:

	/// Constructor. Set a non-zero queueLength for partial average calculation
	CStopWatch( uint queueLength=0 );

	// ADD by KSR
	CStopWatch( const char *name, uint queueLength=0 );

	const char	   *getName() const { return _Name; }
	void			setName(const char *name) { _Name = name; }

	
	/// Begin measurement
	void						start();

	/// Pause
	void						pause();

	/// Resume
	void						resume();

	/// Add time (in ticks unit) to the current measurement
	void						addTime( TTickDuration t );

	/// End measurement
	void						stop();

	/// Add an external duration (in ticks unit) to the average queue
	void						addMeasurement( TTickDuration t );


	/// Elapsed time in millisecond (call it after stop())
	//TMsDuration					getDuration() const;
	TTicks						getDuration() const;

	/// Average duration of the queueLength last durations (using the queueLength argument specified in the constructor)
	//TMsDuration					getPartialAverage() const;
	TTicks						getPartialAverage() const;

	/// Average of the duration
	//TMsDuration					getAverageDuration() const;
	TTicks						getAverageDuration() const;


	
	/// Sum of the measured durations (in ticks)
	//TTickDuration				sumTicks() const { return _SumTicks; }
	TTicks						sumTicks() const { return _SumTicks; }

	/// Number of measurements
	uint32						measurementNumber() const { return _MeasurementNumber; }

private:

	TTicks						_BeginTime;
	TTicks						_ElapsedTime;
	//TTickDuration				_ElapsedTicks;
	TTicks						_ElapsedTicks;

	//TTickDuration				_SumTicks;
	TTicks						_SumTicks;
	uint32						_MeasurementNumber;

	std::deque<TTicks>			_Queue;
	uint						_QLength;

	const  char					*_Name; // KSR ADD
};

// ADD by KSR
class	CAutoStopWatch
{
private:
	CStopWatch		*_Watch;
public:
	CAutoStopWatch(CStopWatch *watch) : _Watch(watch) { _Watch->start(); };
	~CAutoStopWatch() { _Watch->stop(); sipinfo("StopWatch <%s> Elapsed Time : %u (ms)", _Watch->getName(), _Watch->getDuration()); };

	CAutoStopWatch(){};
};

#define	G_WATCH_DECL(__name)	SIPBASE::CStopWatch __watch##__name(#__name);
#define	G_WATCH_BEGIN(__name)	__watch##__name.start();
#define	G_WATCH_END(__name)	__watch##__name.stop(); sipinfo("StopWatch <%s> Elapsed Time : %u (ms)", __watch##__name.getName(), __watch##__name.getDuration());


#define	WATCH_BEGIN(__name)	SIPBASE::CStopWatch __watch##__name(#__name); __watch##__name.start();
#define	WATCH_END(__name)	__watch##__name.stop(); sipinfo("StopWatch <%s> Elapsed Time : %u (ms)", __watch##__name.getName(), __watch##__name.getDuration());

#define	WATCH_END_RET(__name, ret)	__watch##__name.stop(); sipinfo("StopWatch <%s> Elapsed Time : %u (ms)", __watch##__name.getName(), __watch##__name.getDuration()); ret = __watch##__name.getDuration();

#define AUTO_WATCH(__name)	SIPBASE::CStopWatch __watch##__name(#__name); CAutoStopWatch autowatch##__name(&__watch##__name);


} // SIPBASE


#endif // __STOP_WATCH_H__

/* End of stop_watch.h */
