/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __THREAD_H__
#define __THREAD_H__

#include "types_sip.h"
#include "common.h"

namespace SIPBASE {

/**
 * Thread callback interface.
 * When a thread is created, it will call run() in its attached IRunnable interface.
 *
 *\code

	#include "thread.h"

	class HelloLoopThread : public IRunnable
	{
		void run ()
		{
			while(true)	printf("Hello World\n");
		}

	};

	IThread *thread = IThread::create (new HelloLoopThread);
	thread->start ();

 *\endcode
 *
 *
 *
 *
 * \date 2000
 */
class IRunnable
{
public:
	// Called when a thread is run.
	virtual void run()=0;
	virtual ~IRunnable()
	{
	}
	// Return the runnable name
	virtual void getName (std::string &result) const
	{
		result = "NoName";
	}
};

/**
 * Thread base interface, must be implemented for all OS
 * \date 2000
 */
class IThread
{
public:

	/** 
	  * Create a new thread.
	  * Implemented in the derived class.
	  */
	static IThread *create(IRunnable *runnable, uint32 stackSize = 0);

	/** 
	  * Return a pointer on the current thread.
	  * Implemented in the derived class.
	  * Not implemented under Linux.
	  */
	static IThread *getCurrentThread ();

	virtual ~IThread () { }
	
	// Starts the thread.
	virtual void start()=0;

	// Check if the thread is still running
	virtual bool isRunning() =0;

	// Terminate the thread (risky method, use only in extreme cases)
	virtual void terminate()=0;

	// In the calling program, wait until the specified thread has exited. After wait() has returned, you can delete the thread object.
	virtual void wait()=0;

	/// Return a pointer to the runnable object
	virtual IRunnable *getRunnable()=0;

	/**
	  * Set the CPU mask of this thread. Thread must have been started before.
	  * The mask must be a subset of the CPU mask returned by IProcess::getCPUMask() thread process.
	  * Not implemented under Linux.
	  */
	virtual bool setCPUMask(uint64 cpuMask)=0;

	/**
	  * Get the CPU mask of this thread. Thread must have been started before.
	  * The mask should be a subset of the CPU mask returned by IProcess::getCPUMask() thread process.
	  * Not implemented under Linux.
	  */
	virtual uint64 getCPUMask()=0;

	/**
	  * Get the thread user name.
	  * Notimplemented under linux, under windows return the name of the logon user.
	  */
	virtual std::string getUserName()=0;
};


/*
 * Thread exception
 */
struct EThread : public Exception
{
	EThread (const char* message) : Exception (message) {};
	EThread (const ucchar* message) : Exception (message) {};
};


/**
 * Process base interface, must be implemented for all OS
 * \date 2000
 */
class IProcess
{
public:

	/** 
	  * Return a pointer on the current process.
	  * Implemented in the derived class.
	  */
	static IProcess *getCurrentProcess ();

	/**
	  * Return process CPU mask. Each bit stand for a CPU usable by the process threads.
	  * Not implemented under Linux.
	  */
	virtual uint64 getCPUMask()=0;

	/**
	  * Set the process CPU mask. Each bit stand for a CPU usable by the process threads.
	  * Not implemented under Linux.
	  */
	virtual bool setCPUMask(uint64 mask)=0;

	virtual ~IProcess() {}
};


} // SIPBASE


#endif // __THREAD_H__

/* End of thread.h */
