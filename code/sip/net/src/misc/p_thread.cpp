/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#ifdef SIP_OS_UNIX
#include <sys/wait.h>
#include <fstream>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include "misc/ucstring.h"
#include "misc/p_thread.h"

using namespace std;

namespace SIPBASE {


/*
 * The IThread static creator
 */
IThread *IThread::create( IRunnable *runnable, uint32 stackSize)
{
	return new CPThread( runnable, stackSize );
}


CPThread CurrentThread(NULL, 0);

/*
 * Get the current thread
 */
IThread *IThread::getCurrentThread ()
{
	/// \todo: implement this functionality for POSIX thread
	return &CurrentThread;
} 

/*
 * Thread beginning
 */
void ProxyFunc( void *arg )
{
	CPThread *parent = (CPThread*)arg;

	// Allow to terminate the thread without cancellation point
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);

	// Run the code of the thread
	parent->Runnable->run();

	// Allow some clean
// 	pthread_exit(0);
}



/*
 * Constructor
 */
CPThread::CPThread(IRunnable *runnable, uint32 stackSize) 
	:	_State(0), 
		_StackSize(stackSize),
		Runnable(runnable)
{
}


/*
 * Destructor
 */
CPThread::~CPThread()
{
	if(_State == 1)
		terminate(); // force the end of the thread if not already ended
	
	if(_State > 0)
		pthread_detach(_ThreadHandle); // free allocated resources only if it was created
}

/*
 * start
 */
void CPThread::start()
{
	pthread_attr_t tattr;
	int ret = 0;

	/* initialized with default attributes */
	ret = pthread_attr_init(&tattr);
	if (ret != 0)
	{
		throw EThread("Cannot start new thread at pthread init");
	}

	/* setting the size of the stack also */
	if (_StackSize > PTHREAD_STACK_MIN) {
		ret = pthread_attr_setstacksize(&tattr, _StackSize);
		if (ret != 0)
		{
			sipwarning("At pthread setstacksize ret = %d, stacksize = %d", ret, _StackSize);
			throw EThread("Cannot start new thread at setstacksize");
		}
	}

	if(pthread_create(&_ThreadHandle, _StackSize != 0 ? &tattr : 0, (void *(*)(void*))ProxyFunc, this) != 0)
	{
		throw EThread("Cannot start new thread");
	}
	_State = 1;
}

bool CPThread::isRunning()
{
	// TODO : need a real implementation here that check thread status
	return _State == 1;
}

/*
 * terminate
 */
void CPThread::terminate()
{
	if(_State == 1)
	{
		// cancel only if started
		pthread_cancel(_ThreadHandle);
		_State = 2;	// set to finished
	}
}

/*
 * wait
 */
void CPThread::wait ()
{
	if(_State == 1)
	{
		if(pthread_join(_ThreadHandle, 0) != 0)
		{
			throw EThread( "Cannot join with thread" );
		}
		_State = 2;	// set to finished
	}
}

/*
 * setCPUMask
 */
bool CPThread::setCPUMask(uint64 cpuMask)
{
	/// \todo: handle processor selection under POSIX thread
	return true;
}

/*
 * getCPUMask
 */
uint64 CPThread::getCPUMask()
{
	/// \todo: handle processor selection under POSIX thread
	return 1;
}

/*
 * getUserName
 */
std::string CPThread::getUserName()
{
	/// \todo: return the thread user name
	return "Not implemented";
}


// **** Process

// The current process
CPProcess CurrentProcess;

// Get the current process
IProcess *IProcess::getCurrentProcess ()
{
	return &CurrentProcess;
}

/*
 * getCPUMask
 */
uint64 CPProcess::getCPUMask()
{
	/// \todo: handle processor selection under POSIX thread
	return 1;
}

/// set the CPU mask
bool CPProcess::setCPUMask(uint64 mask)
{
	/// \todo: handle processor selection under POSIX thread
	return 1;
}


} // SIPBASE

int getProcIdByName(const ucchar *_procName) 
{
	int pid = -1;
	
	static int index = 0;
	// sipinfo("procName = %S, index = %d", _procName, index++);
	ucstring procName(_procName);
	// Open the /proc directory
	DIR *dp = opendir("/proc");
	if (dp != NULL);
	{
		// Enumerate all entries in directory until process found
		struct dirent *dirp;
		while (pid < 0 && (dirp = readdir(dp)))
		{
			// Skip non-numeric entries
			int id = atoi(dirp->d_name);
			if (id > 0)
			{
				// Read contents of virtual /proc/{pid}/cmdline file
				string cmdPath = string("/proc/") + dirp->d_name + "/cmdline";
				ifstream cmdFile(cmdPath.c_str());

				string cmdLine;
				getline(cmdFile, cmdLine);
				if (!cmdLine.empty())
				{
					ucstring ucCmdLine = ucstring::makeFromUtf8(cmdLine);
					// Keep first cmdline item which contains the program path

					size_t pos = ucCmdLine.find(L'\0');
					if (pos != string::npos)
						ucCmdLine = ucCmdLine.substr(0, pos);
					// Keep program name only, removing the path
					pos = ucCmdLine.rfind(L'/');
					if (pos != string::npos)
						ucCmdLine = ucCmdLine.substr(pos + 1);
					// Compare against requested process name
					if (procName == ucCmdLine)
						pid = id;
				}
			}
		}
	}

	closedir(dp);
	
	return pid;
}

bool TerminateRunningProcess(const ucchar *_ucName)
{
	// to do for shutdown privilege
	int pid = getProcIdByName(_ucName);
	if (pid == -1)
		return true;

	int status;
	kill(pid, SIGTERM);
	pid_t nRet = waitpid(pid, &status, WNOHANG);	// Check specified
	printf("status=%d, nRet=%d", status, nRet);

	// decide the return value after look up status
	return true;
}

bool IsRunningProcess(const ucchar *_ucName)
{
	// to do for shudown privilege
	return getProcIdByName(_ucName) != -1;
}

#else // SIP_OS_UNIX

// remove stupid VC6 warnings
void foo_p_thread_cpp() {}

#endif // SIP_OS_UNIX
