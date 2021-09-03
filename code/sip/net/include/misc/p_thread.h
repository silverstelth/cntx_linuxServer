/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __P_THREAD_H__
#define __P_THREAD_H__

#ifdef SIP_OS_UNIX

#include "types_sip.h"
#include "thread.h"
#include <pthread.h>

namespace SIPBASE {


/**
 * Posix Thread
 * \date 2001
 */
class CPThread : public IThread
{
public:

	/// Constructor
	CPThread( IRunnable *runnable, uint32 stackSize);

	virtual ~CPThread();
	
	virtual void start();
	virtual bool isRunning();
	virtual void terminate();
	virtual void wait();
	virtual bool setCPUMask(uint64 cpuMask);
	virtual uint64 getCPUMask();
	virtual std::string getUserName();

	virtual IRunnable *getRunnable()
	{
		return Runnable;
	}

	/// Internal use
	IRunnable	*Runnable;

private:
	uint8		_State; // 0=not created, 1=started, 2=finished
	uint32		_StackSize;	
	pthread_t	_ThreadHandle;

};

/**
 * Posix Process
 * \date 2001
 */
class CPProcess : public IProcess
{
public:

	virtual uint64 getCPUMask();
	virtual bool setCPUMask(uint64 mask);
	
};

} // SIPBASE

extern bool TerminateRunningProcess(const ucchar* _ucName);
extern bool IsRunningProcess(const ucchar* _ucName);

#endif // SIP_OS_UNIX

#endif // __P_THREAD_H__

/* End of p_thread.h */
