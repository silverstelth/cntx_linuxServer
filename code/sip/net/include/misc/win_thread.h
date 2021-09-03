/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __WIN_THREAD_H__
#define __WIN_THREAD_H__

#include "types_sip.h"
#include "thread.h"

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#include <Psapi.h>
#include <aclapi.h>
#include <shlwapi.h>

namespace SIPBASE {

/**
 * Windows implementation of CThread class (look thread.h)
 * \date 2000
 */
class CWinThread : public IThread
{
public:

	/// Constructor
	CWinThread(IRunnable *runnable, uint32 stackSize);

	/// Don't use this constructor, only used to initialise the main thread class
	CWinThread (void* threadHandle, uint32 threadId);

	virtual ~CWinThread();
	
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

	/// private use
	IRunnable	*Runnable;

private:

	uint32		_StackSize;
	void		*ThreadHandle;	// HANDLE	don't put it to avoid including windows.h
	uint32		ThreadId;		// DWORD	don't put it to avoid including windows.h
	bool		_MainThread;	// true if ths thread is the main thread, else false
};

/**
 * Windows Process
 * \date 2001
 */
class CWinProcess : public IProcess
{
public:

	CWinProcess (void *handle);

	virtual uint64 getCPUMask();
	virtual bool setCPUMask(uint64 mask);

private:
	void	*_ProcessHandle;
};

extern	BOOL	TerminateRunningProcess(TCHAR *_ucName);
extern	BOOL	kill_process(DWORD pid);
extern	bool	ExtractProcessOwner( HANDLE hProcess_i, TCHAR* csOwner_o );
extern	HANDLE adv_open_process(DWORD pid, DWORD dwAccessRights);
extern	BOOL	IsRunningProcess(TCHAR *_ucName);
extern	BOOL	Set_SE_TAKE_OWNERSHIP_NAME_Privilege(HANDLE hProcess);

} // SIPBASE

#endif // SIP_OS_WINDOWS

#endif // __WIN_THREAD_H__

/* End of win_thread.h */
