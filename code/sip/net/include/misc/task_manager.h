/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TASK_MANAGER_H__
#define __TASK_MANAGER_H__

#include "types_sip.h"

#include <list>

#include "mutex.h"
#include "thread.h"

namespace SIPBASE {

/**
 * CTaskManager is a class that manage a list of Task with one Thread
 * \date 2000
 */
class CTaskManager : public IRunnable
{
public:

	/// Constructor
	CTaskManager();

	/// Destructeur
	~CTaskManager();

	/// Manage TaskQueue
	void run(void);

	/// Add a task to TaskManager and its priority
	void addTask(IRunnable *, float priority=0);

	/// Delete a task, only if task is not running, return true if found and deleted
	bool deleteTask(IRunnable *r);

	/// Sleep a Task
	void sleepTask(void) { sipSleep(10); }

	/// Task list size
	uint taskListSize(void); 

	/// return false if exit() is required. task added with addTask() should test this flag.
	bool	isThreadRunning() const {return _ThreadRunning;}

	/// Dump task list
	void	dump (std::vector<std::string> &result);
	
	/// Clear the dump of Done files
	void	clearDump();

	/// Get number of waiting task
	uint	getNumWaitingTasks();
	
	/// Is there a current task ?
	bool	isTaskRunning() const {return _IsTaskRunning;}
	
	/// A callback to modify the task priority
	class IChangeTaskPriority
	{
	public:
		virtual float getTaskPriority(const IRunnable &runable) = 0;
		virtual ~IChangeTaskPriority(){};
	};

	/// Register task priority callback
	void	registerTaskPriorityCallback (IChangeTaskPriority *callback);

private:

	/// Register task priority callback
	void	changeTaskPriority ();
	
	/// The callback
	IChangeTaskPriority		*_ChangePriorityCallback;

protected:
	
	/** If any, wait the current running task to complete
	 *	this function MUST be called in a 'accessor to the _TaskQueue' statement because a mutex is required
	 *	eg:
	 *	\code
	 *	{
	 *		CSynchronized<list<IRunnable *> >::CAccessor acces(&_TaskQueue);
	 *		waitCurrentTaskToComplete();
	 *	}
	 *	\endcode
	 */
	void	waitCurrentTaskToComplete ();

protected:

	// A task in the waiting queue with its parameters
	class CWaitingTask
	{
	public:
		CWaitingTask (IRunnable *task, float priority)
		{
			Task = task;
			Priority = priority;
		}
		IRunnable		*Task;
		float			Priority;

		// For the sort
		bool			operator< (const CWaitingTask &other) const
		{
			return Priority < other.Priority;
		}
	};

	/// queue of tasks, using list container instead of queue for DeleteTask methode
	CSynchronized<std::string>				_RunningTask;
	CSynchronized<std::list<CWaitingTask> >	_TaskQueue;
	CSynchronized<std::deque<std::string> >	_DoneTaskQueue;
	
	/// thread pointer
	IThread *_Thread;

	/// flag indicate thread loop, if false cause thread exit
	volatile	bool _ThreadRunning;

private:

	volatile	bool _IsTaskRunning;

};


} // SIPBASE


#endif // __TASK_MANAGER_H__

/* End of task_manager.h */
