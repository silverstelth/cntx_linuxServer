/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/stop_watch.h"
#include <numeric>

using namespace std;

namespace SIPBASE {


/*
 * Constructor
 */
CStopWatch::CStopWatch( uint queueLength ) :
	_BeginTime( 0 ),
	_ElapsedTicks( 0 ),
	_SumTicks( 0 ),
	_MeasurementNumber( 0 ),
	_Queue(),
	_QLength( queueLength ),
    _Name(NULL)
{}


CStopWatch::CStopWatch( const char *name, uint queueLength ) :
	_BeginTime( 0 ),
	_ElapsedTicks( 0 ),
	_SumTicks( 0 ),
	_MeasurementNumber( 0 ),
	_Queue(),
	_QLength( queueLength ),
    _Name(name)
{
}


/*
 * Begin measurement
 */
void	CStopWatch::start()
{
	_BeginTime = CTime::getPerformanceTime();
	_ElapsedTicks = 0;
}


/*
 * Pause
 */
void	CStopWatch::pause()
{
	_ElapsedTicks += (TTickDuration)(CTime::getPerformanceTime() - _BeginTime);

}


/*
 * Resume
 */
void	CStopWatch::resume()
{
	_BeginTime = CTime::getPerformanceTime();
}


/*
 * Add time (in TTicks unit) to the current measurement
 */
void	CStopWatch::addTime( TTickDuration t )
{
	_ElapsedTicks += t;
}


/*
 * End measurement
 */
void	CStopWatch::stop()
{
	//_ElapsedTicks += (TTickDuration)(CTime::getPerformanceTime() - _BeginTime);
	_ElapsedTicks += (CTime::getPerformanceTime() - _BeginTime);

	// Setup average
	_SumTicks += _ElapsedTicks;
	++_MeasurementNumber;

	// Setup partial average
	if ( _QLength != 0 )
	{
		_Queue.push_back( _ElapsedTicks );
		if ( _Queue.size() > _QLength )
		{
			_Queue.pop_front();
		}
	}
}


/*
 * Add an external duration (in TTicks unit) to the average queue
 */
void	CStopWatch::addMeasurement( TTickDuration t )
{
	// Setup average
	_SumTicks += t;
	++_MeasurementNumber;

	// Setup partial average
	if ( _QLength != 0 )
	{
		_Queue.push_back( t );
		if ( _Queue.size() > _QLength )
		{
			_Queue.pop_front();
		}
	}

}


/*
 * Elapsed time in millisecond (call it after stop())
 */
TTicks	CStopWatch::getDuration() const
{
	return (TTicks)(CTime::ticksToSecond( _ElapsedTicks ) * 1000.0);
}


/*
 * Average of the queueLength last durations (using the queueLength argument specified in the constructor)
 */
TTicks	CStopWatch::getPartialAverage() const
{
	if (_Queue.size() == 0)
		return (TTicks)0;
	else
		return (TTicks)(CTime::ticksToSecond( accumulate( _Queue.begin(), _Queue.end(), (TTicks)0 ) / _Queue.size() ) * 1000.0);
}


/*
 * Average of the duration
 */
TTicks	CStopWatch::getAverageDuration() const
{
	return (TTicks)(CTime::ticksToSecond( _SumTicks / _MeasurementNumber ) * 1000.0);
}



} // SIPBASE
