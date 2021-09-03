/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/co_task.h"
#include "misc/tds.h"

// Flag to use thread instead of coroutine primitives (i.e windows fibers or gcc context)
#ifndef SIP_OS_WINDOWS
#define SIP_USE_THREAD_COTASK
#endif
// flag to activate debug message
//#define SIP_GEN_DEBUG_MSG

#ifdef SIP_GEN_DEBUG_MSG
#define SIP_CT_DEBUG sipdebug
#else
#define SIP_CT_DEBUG if(0)sipdebug
#endif

#if defined(SIP_USE_THREAD_COTASK)
//	#pragma message(SIP_LOC_MSG "Using threaded coroutine")
	# include "misc/thread.h"
#else //SIP_USE_THREAD_COTASK
// some platform specifics
#if defined (SIP_OS_WINDOWS)
# if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
#  ifdef _WIN32_WINNT
#   undef _WIN32_WINNT
#  endif
#  define _WIN32_WINNT 0x0500
# endif
# define SIP_WIN_CALLBACK CALLBACK
// Visual .NET won't allow Fibers for a Windows version older than 2000. However the basic features are sufficient for us, we want to compile them for all Windows >= 95
# if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0400)
#  ifdef _WIN32_WINNT
#   undef _WIN32_WINNT
#  endif
#  define _WIN32_WINNT 0x0400
# endif

# include <windows.h>
#elif defined (SIP_OS_UNIX)
# define SIP_WIN_CALLBACK 
# include <ucontext.h>
#else
# error "Coroutine task are not supported yet by your platform, do it ?"
#endif
#endif //SIP_USE_THREAD_COTASK

namespace SIPBASE
{

	// platform specific data
#if  defined(SIP_USE_THREAD_COTASK)
	struct TCoTaskData : public IRunnable
#else //SIP_USE_THREAD_COTASK
	struct TCoTaskData
#endif //SIP_USE_THREAD_COTASK
	{
#if  defined(SIP_USE_THREAD_COTASK)
		/// The thread id for the co task
//		TThreadId	*_TaskThreadId;
		/// The parent thread id
//		TThreadId	*_ParentThreadId;

		// the thread of the task
		IThread				*_TaskThread;
		/// The mutex of the task task
		CFastMutex	_TaskMutex;

		CCoTask				*_CoTask;

		// set by master, cleared by task
		volatile bool		_ResumeTask;
		// set by task, cleared by master
		volatile bool		_TaskHasYield;


		TCoTaskData(CCoTask *task)
			:	_TaskThread(NULL),
				_CoTask(task),
				_ResumeTask(false),
				_TaskHasYield(false)
		{
		}

		virtual ~TCoTaskData()
		{
			SIP_CT_DEBUG("CoTaskData : ~TCoTaskData %p : deleting cotask data", this);
			if (_TaskThread != NULL)
			{
				SIP_CT_DEBUG("CoTask : ~TCoTaskData (%p) waiting for thread termination", this);

				// waiting for thread to terminate
				_TaskThread->wait();

				delete _TaskThread;
				_TaskThread = NULL;
			}
		}

		void run();

#else //SIP_USE_THREAD_COTASK
#if defined (SIP_OS_WINDOWS)
		/// The fiber pointer for the task fiber
		LPVOID	_Fiber;
		/// The fiber pointer of the main (or master, or parent, as you want)
		LPVOID	_ParentFiber;
#elif defined (SIP_OS_UNIX)
		/// The coroutine stack pointer (allocated memory)
		uint8		*_Stack;
		/// The task context
		ucontext_t	_Ctx;
		/// The main (or master or parent, as you want) task context
		ucontext_t	_ParentCtx;
#endif
#endif //SIP_USE_THREAD_COTASK
		
#if !defined(SIP_USE_THREAD_COTASK)
		/** task bootstrap function
		 *	NB : this function is in this structure because of the
		 *	SIP_WIN_CALLBACK symbol that need <windows.h> to be defined, so
		 *	to remove it from the header, I moved the function here
		 *	(otherwise, it should be declared in the CCoTask class as
		 *	a private member)
		 */
		static void SIP_WIN_CALLBACK startFunc(void* param)
		{
			CCoTask *task = reinterpret_cast<CCoTask*>(param);

			SIP_CT_DEBUG("CoTask : task %p start func called", task);

			try
			{
				// run the task
				task->run();
			}
			catch(...)
			{
				sipwarning("CCoTask::startFunc : the task has generated an unhandled exeption and will terminate");
			}

			task->_Finished = true;

			SIP_CT_DEBUG("CoTask : task %p finished, entering infinite yield loop (waiting destruction)", task);

			// nothing more to do
			for (;;)
				// return to parent task
				task->yield();

			sipassert(false);
		}
#endif //SIP_USE_THREAD_COTASK
	};

	/** Management of current task in a thread.
	 *	This class is used to store and retrieve the current
	 *	CCoTask pointer in the current thread.
	 *	It is build upon the SAFE_SINGLETON paradigm, making it
	 *	safe to use with SIP DLL.
	 *	For windows platform, this singleton also hold the
	 *	fiber pointer of the current thread. This is needed because
	 *	of the bad design the the fiber API before Windows XP.
	 */
	class CCurrentCoTask
	{
		SIPBASE_SAFE_SINGLETON_DECL(CCurrentCoTask);

		/// A thread dependent storage to hold by thread coroutine info
		CTDS	_CurrentTaskTDS;

#if defined (SIP_OS_WINDOWS)
		/// A Thread dependent storage to hold fiber pointer.
		CTDS	_ThreadMainFiber;
#endif

		CCurrentCoTask()
		{}

	public:
		/// Set the current task for the calling thread
		void setCurrentTask(CCoTask *task)
		{
			SIP_CT_DEBUG("CoTask : setting current co task to %p", task);
			_CurrentTaskTDS.setPointer(task);
		}

		/// retrieve the current task for the calling thread
		CCoTask *getCurrentTask()
		{
			return reinterpret_cast<CCoTask*>(_CurrentTaskTDS.getPointer());
		}
#if defined (SIP_OS_WINDOWS) && !defined(SIP_USE_THREAD_COTASK)
		void setMainFiber(LPVOID fiber)
		{
			_ThreadMainFiber.setPointer(fiber);
		}

		/** Return the main fiber for the calling thread. Return NULL if 
		 *	the thread has not been converted to fiber.
		 */
		LPVOID getMainFiber()
		{
			return _ThreadMainFiber.getPointer();
		}
#endif
	};

	SIPBASE_SAFE_SINGLETON_IMPL(CCurrentCoTask);

	CCoTask *CCoTask::getCurrentTask()
	{
		return CCurrentCoTask::getInstance().getCurrentTask();
	}


	CCoTask::CCoTask(uint stackSize)
		: _Started(false),
		_TerminationRequested(false),
		_Finished(false)
	{
		SIP_CT_DEBUG("CoTask : creating task %p", this);
#if defined(SIP_USE_THREAD_COTASK)
		// allocate platform specific data storage
		_PImpl = new TCoTaskData(this);
//		_PImpl->_TaskThreadId = 0;
//		_PImpl->_ParentThreadId = 0;
#else //SIP_USE_THREAD_COTASK
		// allocate platform specific data storage
		_PImpl = new TCoTaskData;
#if defined (SIP_OS_WINDOWS)
		_PImpl->_Fiber = NULL;
		_PImpl->_ParentFiber = NULL;
#elif defined(SIP_OS_UNIX)
		// allocate the stack
		_PImpl->_Stack = new uint8[stackSize];
#endif
#endif //SIP_USE_THREAD_COTASK
	}

	CCoTask::~CCoTask()
	{
		SIP_CT_DEBUG("CoTask : deleting task %p", this);
		_TerminationRequested = true;

		if (_Started)
		{
			while (!_Finished)
				resume();
		}

#if defined(SIP_USE_THREAD_COTASK)

#else //SIP_USE_THREAD_COTASK 
#if defined (SIP_OS_WINDOWS)
		DeleteFiber(_PImpl->_Fiber);
#elif defined(SIP_OS_UNIX)
		// free the stack
		delete [] _PImpl->_Stack;
#endif
#endif //SIP_USE_THREAD_COTASK 

		// free platform specific storage
		delete _PImpl;
	}

	void CCoTask::start()
	{
		SIP_CT_DEBUG("CoTask : Starting task %p", this);
		sipassert(!_Started);

		_Started = true;

#if defined(SIP_USE_THREAD_COTASK)

		// create the thread
		_PImpl->_TaskThread = IThread::create(_PImpl);

		SIP_CT_DEBUG("CoTask : start() task %p entering mutex", this);  
		// get the mutex
		_PImpl->_TaskMutex.enter();
		SIP_CT_DEBUG("CoTask : start() task %p mutex entered", this);

		// set the resume flag to true
		_PImpl->_ResumeTask = true;

		// start the thread
		_PImpl->_TaskThread->start();

		SIP_CT_DEBUG("CoTask : start() task %p leaving mutex", this);
		// leave the mutex
		_PImpl->_TaskMutex.leave();

		// wait until the task has yield
		for (;;)
		{
			// give up the time slice to the co task
			sipSleep(0);
			SIP_CT_DEBUG("CoTask : start() task %p entering mutex", this);  
			// get the mutex
			_PImpl->_TaskMutex.enter();
			SIP_CT_DEBUG("CoTask : start() task %p mutex entered", this);

			if (!_PImpl->_TaskHasYield)
			{
				// not finished
				SIP_CT_DEBUG("CoTask : start() task %p has not yield, leaving mutex", this);
				// leave the mutex
				_PImpl->_TaskMutex.leave();
			}
			else
			{
				break;
			}
		}

		// clear the yield flag
		_PImpl->_TaskHasYield = false;

		SIP_CT_DEBUG("CoTask : start() task %p has yield", this);

		// in the treaded mode, there is no need to call resume() inside start()

#else //SIP_USE_THREAD_COTASK
  #if defined (SIP_OS_WINDOWS)

		LPVOID mainFiber = CCurrentCoTask::getInstance().getMainFiber();

		if (mainFiber == NULL)
		{
			// we need to convert this thread to a fiber
			mainFiber = ConvertThreadToFiber(NULL);

			CCurrentCoTask::getInstance().setMainFiber(mainFiber);
		}

		_PImpl->_ParentFiber = mainFiber;
		_PImpl->_Fiber = CreateFiber(SIP_TASK_STACK_SIZE, TCoTaskData::startFunc, this);
		sipassert(_PImpl->_Fiber != NULL);
  #elif defined (SIP_OS_UNIX)
		// store the parent ctx
		sipverify(getcontext(&_PImpl->_ParentCtx) == 0);
		// build the task context
		sipverify(getcontext(&_PImpl->_Ctx) == 0);

		// change the task context
 		_PImpl->_Ctx.uc_stack.ss_sp = _PImpl->_Stack;
		_PImpl->_Ctx.uc_stack.ss_size = SIP_TASK_STACK_SIZE;

		_PImpl->_Ctx.uc_link = NULL;
		_PImpl->_Ctx.uc_stack.ss_flags = 0;

		makecontext(&_PImpl->_Ctx, reinterpret_cast<void (*)()>(TCoTaskData::startFunc), 1, this);
  #endif
		resume();
#endif //SIP_USE_THREAD_COTASK
	}

	void CCoTask::yield()
	{
		SIP_CT_DEBUG("CoTask : task %p yield", this);
		sipassert(_Started);
		sipassert(CCurrentCoTask::getInstance().getCurrentTask() == this);
#if defined(SIP_USE_THREAD_COTASK)

		// set the yield flag
		_PImpl->_TaskHasYield = true;

		// release the mutex
		SIP_CT_DEBUG("CoTask : yield() task %p leaving mutex", this);
		_PImpl->_TaskMutex.leave();

		// now, wait until the resume flag is set
		for (;;)
		{
			// give up the time slice to the master thread
			sipSleep(0);
			// And get back the mutex for waiting for next resume (this should lock)
			SIP_CT_DEBUG("CoTask : yield() task %p entering mutex", this);
			_PImpl->_TaskMutex.enter();
			SIP_CT_DEBUG("CoTask : yield() task %p mutex entered", this);

			if (!_PImpl->_ResumeTask)
			{
				// not time to resume, release the mutex and sleep
				SIP_CT_DEBUG("CoTask : yield() task %p not time to resume, leaving mutex", this);
				_PImpl->_TaskMutex.leave();
//				sipSleep(0);
			}
			else
				break;
		}

		// clear the resume flag
		_PImpl->_ResumeTask = false;

#else //SIP_USE_THREAD_COTASK 
		CCurrentCoTask::getInstance().setCurrentTask(NULL);
#if defined (SIP_OS_WINDOWS)
		SwitchToFiber(_PImpl->_ParentFiber);
#elif defined (SIP_OS_UNIX)
		// swap to the parent context
		sipverify(swapcontext(&_PImpl->_Ctx, &_PImpl->_ParentCtx) == 0);
#endif
#endif //SIP_USE_THREAD_COTASK 

		SIP_CT_DEBUG("CoTask : task %p have been resumed", this);
	}

	void CCoTask::resume()
	{
		SIP_CT_DEBUG("CoTask : resuming task %p", this);
		sipassert(CCurrentCoTask::getInstance().getCurrentTask() != this);
		if (!_Started)
			start();
		else if (!_Finished)
		{
			sipassert(_Started);

#if defined(SIP_USE_THREAD_COTASK)

			// set the resume flag to true
			_PImpl->_ResumeTask = true;
			_PImpl->_TaskHasYield = false;
			// Release the mutex
			SIP_CT_DEBUG("CoTask : resume() task %p leaving mutex", this);
			_PImpl->_TaskMutex.leave();
			// wait that the task has started
			while (_PImpl->_ResumeTask)
				sipSleep(0);

			SIP_CT_DEBUG("CoTask : resume() task %p is started, waiting yield", this);
			// ok the task has started
			// now wait for task to yield
			for (;;)
			{
				// give up the time slice to the co task
				sipSleep(0);

				// acquire the mutex
				SIP_CT_DEBUG("CoTask : resume() task %p entering mutex", this);
				_PImpl->_TaskMutex.enter();
				SIP_CT_DEBUG("CoTask : resume() task %p mutex entered", this);

				if (!_PImpl->_TaskHasYield)
				{
					SIP_CT_DEBUG("CoTask : resume() task %p still not yielding, leaving mutex", this);
					_PImpl->_TaskMutex.leave();
					// give the focus to another thread before acquiring the mutex
//					sipSleep(0);
				}
				else
				{
					// the task has yield
					break;
				}
			}

			// clear the yield flag
			_PImpl->_TaskHasYield = false;

#else // SIP_USE_THREAD_COTASK
			CCurrentCoTask::getInstance().setCurrentTask(this);
#if defined (SIP_OS_WINDOWS)
			SwitchToFiber(_PImpl->_Fiber);
#elif defined (SIP_OS_UNIX)
			// swap to the parent context
			sipverify(swapcontext(&_PImpl->_ParentCtx, &_PImpl->_Ctx) == 0);
#endif
#endif //SIP_USE_THREAD_COTASK
		}

		SIP_CT_DEBUG("CoTask : task %p has yield", this);
	}

	/// wait until the task terminate
	void CCoTask::wait()
	{
		SIP_CT_DEBUG("CoTask : waiting for task %p to terminate", this);
		// resume the task until termination
		while (!_Finished)
			resume();
	}

#if  defined(SIP_USE_THREAD_COTASK)
	void TCoTaskData::run()
	{
		SIP_CT_DEBUG("CoTask : entering TCoTaskData::run for task %p", _CoTask);
		// set the current task
		CCurrentCoTask::getInstance().setCurrentTask(_CoTask);
		// Set the task as running
//		_Running = true;
		SIP_CT_DEBUG("CoTask : TCoTaskData::run() task %p entering mutex", this);
		// Acquire the task mutex
		_TaskMutex.enter();
		SIP_CT_DEBUG("CoTask : TCoTaskData::run mutex aquired, calling '_CoTask->run()' for task %p", _CoTask);

		// clear the resume flag
		_CoTask->_PImpl->_ResumeTask = false;

		// run the task
		_CoTask->run();

		// mark the task has yielding
		_CoTask->_PImpl->_TaskHasYield = true;
		// mark the task has finished
		_CoTask->_Finished = true;

		// nothing more to do, just return to terminate the thread
		SIP_CT_DEBUG("CoTask : leaving TCoTaskData::run for task %p", _CoTask);

		SIP_CT_DEBUG("CoTask : TCoTaskData::run() task %p leaving mutex", this);
		// Release the parent mutex
		_TaskMutex.leave();

	}
#endif //SIP_USE_THREAD_COTASK

} // namespace SIPBASE

