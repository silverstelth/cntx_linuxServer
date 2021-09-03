/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#ifdef SIP_OS_WINDOWS
#include "misc/win_thread.h"
#include "misc/ucstring.h"
#include <windows.h>
#include <tchar.h>
#	pragma comment(lib, "Psapi.lib")

// Debug : Sept 01 2006
#if _STLPORT_VERSION >= 0x510
	#include <typeinfo>
#else
	#include <typeinfo.h>
#endif

namespace SIPBASE {

CWinThread MainThread ((void*)GetCurrentThread (), GetCurrentThreadId());
DWORD TLSThreadPointer = 0xFFFFFFFF;

// the IThread static creator
IThread *IThread::create (IRunnable *runnable, uint32 stackSize)
{
	return new CWinThread (runnable, stackSize);
}

IThread *IThread::getCurrentThread ()
{
	// TLS alloc must have been done	
	sipassert (TLSThreadPointer != 0xffffffff);

	// Get the thread pointer
	IThread *thread = (IThread*)TlsGetValue (TLSThreadPointer);

	// Return current thread
	return thread;
}
 
static unsigned long __stdcall ProxyFunc (void *arg)
{
	CWinThread *parent = (CWinThread *) arg;

	// TLS alloc must have been done	
	sipassert (TLSThreadPointer != 0xffffffff);

	// Set the thread pointer in TLS memory
	bool b = (TlsSetValue (TLSThreadPointer, (void*)parent) != 0);
	sipverify (b);

	// Run the thread
	parent->Runnable->run();
	
	return 0;
}

CWinThread::CWinThread (IRunnable *runnable, uint32 stackSize)
{
	_StackSize = stackSize;
	this->Runnable = runnable;
	ThreadHandle = NULL;
	_MainThread = false;
}

CWinThread::CWinThread (void* threadHandle, uint32 threadId)
{
	// Main thread
	_MainThread = true;
	this->Runnable = NULL;
	ThreadHandle = threadHandle;
	ThreadId = threadId;

	// TLS alloc must have been done	
	TLSThreadPointer = TlsAlloc ();
	sipassert (TLSThreadPointer!=0xffffffff);
 
	// Set the thread pointer in TLS memory
	bool b = (TlsSetValue (TLSThreadPointer, (void*)this) != 0); 
	sipverify (b);
}

CWinThread::~CWinThread ()
{
	// If not the main thread
	if (_MainThread)
	{
		// Free TLS memory
		sipassert (TLSThreadPointer!=0xffffffff);
		TlsFree (TLSThreadPointer);
	}
	else
	{
		if (ThreadHandle != NULL) terminate();
	}
}

void CWinThread::start ()
{
//	ThreadHandle = (void *) ::CreateThread (NULL, _StackSize, ProxyFunc, this, 0, (DWORD *)&ThreadId);
	ThreadHandle = (void *) ::CreateThread (NULL, 0, ProxyFunc, this, 0, (DWORD *)&ThreadId);
//	sipdebug("SIPBASE: thread %x started for runnable '%x'", typeid( Runnable ).name());
	OutputDebugStringA(toString(SIP_LOC_MSG " SIPBASE: thread %x started for runnable '%s'\n", ThreadId, typeid( *Runnable ).name()).c_str());
	SetThreadPriorityBoost (ThreadHandle, TRUE); // FALSE == Enable Priority Boost
	if (ThreadHandle == NULL)
	{
		throw EThread ( "Cannot create new thread" );
	}
}

bool CWinThread::isRunning()
{
	if (ThreadHandle == NULL)
		return false;

	DWORD exitCode;
	if (!GetExitCodeThread(ThreadHandle, &exitCode))
		return false;
	
	return exitCode == STILL_ACTIVE;
}


void CWinThread::terminate ()
{
	BOOL i = TerminateThread((HANDLE)ThreadHandle, 0);
	if(!i) 
	{
		DWORD e = GetLastError();		
	}
	i = CloseHandle((HANDLE)ThreadHandle);
	ThreadHandle = NULL;
}

void CWinThread::wait ()
{
	if (ThreadHandle == NULL) return;

	WaitForSingleObject(ThreadHandle, INFINITE);
	CloseHandle(ThreadHandle);
	ThreadHandle = NULL;
}

bool CWinThread::setCPUMask(uint64 cpuMask)
{
	// Thread must exist
	if (ThreadHandle == NULL)
		return false;

	// Ask the system for number of processor available for this process
	return SetThreadAffinityMask ((HANDLE)ThreadHandle, (DWORD)cpuMask) != 0;
}

uint64 CWinThread::getCPUMask()
{
	// Thread must exist
	if (ThreadHandle == NULL)
		return 1;

	// Get the current process mask
	uint64 mask=IProcess::getCurrentProcess ()->getCPUMask ();

	// Get thread affinity mask
	DWORD old = SetThreadAffinityMask ((HANDLE)ThreadHandle, (DWORD)mask);
	sipassert (old != 0);
	if (old == 0)
		return 1;

	// Reset it
	SetThreadAffinityMask ((HANDLE)ThreadHandle, (DWORD)old);

	// Return the mask
	return old;
}

std::string CWinThread::getUserName()
{
	char userName[512];
	DWORD size = 512;
	GetUserNameA (userName, &size);
	return (const char*)userName;
}

// **** Process

// The current process
CWinProcess CurrentProcess ((void*)GetCurrentProcess());

// Get the current process
IProcess *IProcess::getCurrentProcess ()
{
	return &CurrentProcess;
}

CWinProcess::CWinProcess (void *handle)
{
	// Get the current process handle
	_ProcessHandle = handle;
}

uint64 CWinProcess::getCPUMask()
{
	// Ask the system for number of processor available for this process
	DWORD processAffinityMask;
	DWORD systemAffinityMask;
	if (GetProcessAffinityMask((HANDLE)_ProcessHandle, &processAffinityMask, &systemAffinityMask))
	{
		// Return the CPU mask
		return (uint64)processAffinityMask;
	}
	else
		return 1;
}

bool CWinProcess::setCPUMask(uint64 mask)
{
	// Ask the system for number of processor available for this process
	DWORD processAffinityMask= (DWORD)mask;
	return SetProcessAffinityMask((HANDLE)_ProcessHandle, processAffinityMask)!=0;
}


BOOL adjust_dacl(HANDLE h, DWORD dwDesiredAccess)
{
	SID world = { SID_REVISION, 1, SECURITY_WORLD_SID_AUTHORITY, 0 };

	EXPLICIT_ACCESS ea =
	{
		0,
			SET_ACCESS,
			NO_INHERITANCE,
		{
			0, NO_MULTIPLE_TRUSTEE,
				TRUSTEE_IS_SID,
				TRUSTEE_IS_USER,
				0
		}
	};

	ACL* pdacl = 0;
	DWORD err = SetEntriesInAcl(1, &ea, 0, &pdacl);

	ea.grfAccessPermissions = dwDesiredAccess;
	ea.Trustee.ptstrName = (LPTSTR)(&world);

	if (err == ERROR_SUCCESS)
	{
		err = SetSecurityInfo(h, SE_KERNEL_OBJECT, DACL_SECURITY_INFORMATION, 0, 0, pdacl, 0);
		LocalFree(pdacl);

		return (err == ERROR_SUCCESS);
	}
	else
	{
		return(FALSE);
	}
}

BOOL enable_token_privilege(HANDLE htok, LPCTSTR szPrivilege, TOKEN_PRIVILEGES *tpOld)
{
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	if (LookupPrivilegeValue(0, szPrivilege, &tp.Privileges[0].Luid))
	{
		DWORD cbOld = sizeof (*tpOld);

		if (AdjustTokenPrivileges(htok, FALSE, &tp, cbOld, tpOld, &cbOld))
		{
			return (ERROR_NOT_ALL_ASSIGNED != GetLastError());
		}
		else
		{
			return (FALSE);
		}
	}
	else
	{
		return (FALSE);
	}
}

BOOL	Set_SE_TAKE_OWNERSHIP_NAME_Privilege(HANDLE hProcess)
{
	HANDLE htok;
	TOKEN_PRIVILEGES tpOld;

	if (!OpenProcessToken(hProcess, TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &htok))
	{
		return (FALSE);
	}

	BOOL bEnable = enable_token_privilege(htok, SE_TAKE_OWNERSHIP_NAME, &tpOld);

	CloseHandle(htok);

	return bEnable;
}

HANDLE adv_open_process(DWORD pid, DWORD dwAccessRights)
{
	HANDLE hProcess = OpenProcess(dwAccessRights, FALSE, pid);

	if (hProcess == NULL)
	{
		HANDLE hpWriteDAC = OpenProcess(WRITE_DAC, FALSE, pid);

		if (hpWriteDAC == NULL)
		{
			HANDLE htok;
			TOKEN_PRIVILEGES tpOld;

			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &htok))
			{
				return(FALSE);
			}

			if (enable_token_privilege(htok, SE_TAKE_OWNERSHIP_NAME, &tpOld))
			{
				HANDLE hpWriteOwner = OpenProcess(WRITE_OWNER, FALSE, pid);

				if (hpWriteOwner != NULL)
				{
					BYTE buf[512];
					DWORD cb = sizeof buf;

					if (GetTokenInformation(htok, TokenUser, buf, cb, &cb))
					{
						DWORD err = SetSecurityInfo(hpWriteOwner, SE_KERNEL_OBJECT, OWNER_SECURITY_INFORMATION, ((TOKEN_USER *)(buf))->User.Sid, 0, 0, 0);

						if (err == ERROR_SUCCESS)
						{
							if (!DuplicateHandle(GetCurrentProcess(), hpWriteOwner, GetCurrentProcess(), &hpWriteDAC, WRITE_DAC, FALSE, 0))
							{
								hpWriteDAC = NULL;
							}
						}
					}

					CloseHandle(hpWriteOwner);
				}

				AdjustTokenPrivileges(htok, FALSE, &tpOld, 0, 0, 0);
			}

			CloseHandle(htok);
		}

		if (hpWriteDAC)
		{
			adjust_dacl(hpWriteDAC, dwAccessRights);

			if (!DuplicateHandle(GetCurrentProcess(), hpWriteDAC, GetCurrentProcess(), &hProcess, dwAccessRights, FALSE, 0))
			{
				hProcess = NULL;
			}

			CloseHandle(hpWriteDAC);
		}
	}

	return (hProcess);
}

BOOL kill_process(DWORD pid)
{
	HANDLE hp = adv_open_process(pid, PROCESS_TERMINATE);

	if (hp != NULL)
	{
		BOOL bRet = TerminateProcess(hp, 1);
		CloseHandle(hp);

		return (bRet);
	}

	return (FALSE);
}
bool ExtractProcessOwner( HANDLE hProcess_i,
						 TCHAR* csOwner_o )
{
	// Get process token
	HANDLE hProcessToken = NULL;
	if ( !::OpenProcessToken( hProcess_i, TOKEN_READ, &hProcessToken ) || !hProcessToken )
	{
		return false;
	}

	// First get size needed, TokenUser indicates we want user information from given token
	DWORD dwProcessTokenInfoAllocSize = 0;
	::GetTokenInformation(hProcessToken, TokenUser, NULL, 0, &dwProcessTokenInfoAllocSize);

	// Call should have failed due to zero-length buffer.
	if( ::GetLastError() == ERROR_INSUFFICIENT_BUFFER )
	{
		// Allocate buffer for user information in the token.
		PTOKEN_USER pUserToken = reinterpret_cast<PTOKEN_USER>( new BYTE[dwProcessTokenInfoAllocSize] );
		if (pUserToken != NULL)
		{
			// Now get user information in the allocated buffer
			if (::GetTokenInformation( hProcessToken, TokenUser, pUserToken, dwProcessTokenInfoAllocSize, &dwProcessTokenInfoAllocSize ))
			{
				// Some vars that we may need
				SID_NAME_USE   snuSIDNameUse;
				TCHAR          szUser[MAX_PATH] = { 0 };
				DWORD          dwUserNameLength = MAX_PATH;
				TCHAR          szDomain[MAX_PATH] = { 0 };
				DWORD          dwDomainNameLength = MAX_PATH;

				// Retrieve user name and domain name based on user's SID.
				if ( ::LookupAccountSid( NULL,
					pUserToken->User.Sid,
					szUser,
					&dwUserNameLength,
					szDomain,
					&dwDomainNameLength,
					&snuSIDNameUse ))
				{
					// Prepare user name string
					//csOwner_o = _T("\\\\");
					//csOwner_o += szDomain;
					//csOwner_o += _T("\\");
					//csOwner_o += szUser;
					wcscpy(csOwner_o, szUser);

					// We are done!
					CloseHandle( hProcessToken );
					delete [] pUserToken;

					// We succeeded
					return true;
				}//End if
			}// End if

			delete [] pUserToken;
		}// End if
	}// End if

	CloseHandle( hProcessToken );

	// Oops trouble
	return false;
}// End GetProcessOwner

BOOL	TerminateRunningProcess(TCHAR *_ucName)
{
	ucstring	ucName(_ucName);
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; 

	// Get a token for this process. 

	if (!OpenProcessToken(GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
		return false; 

	// Get the LUID for the shutdown privilege. 

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, 
		&tkp.Privileges[0].Luid); 

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	// Get the shutdown privilege for this process. 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
		(PTOKEN_PRIVILEGES)NULL, 0); 

	CloseHandle( hToken );

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return false;

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.
	BOOL bTerminate = true;
	for ( i = 0; i < cProcesses; i++ )
	{
		DWORD processID = aProcesses[i];
		TCHAR szProcessName[MAX_PATH] = _T("unknown");

		// Get a handle to the process.

		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, processID );

		// Get the process name.

		if (NULL != hProcess )
		{
			HMODULE hMod;
			DWORD cbNeeded;

			if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
				&cbNeeded) )
			{
				GetModuleBaseName( hProcess, hMod, szProcessName, 
					sizeof(szProcessName) );
			}
			else 
				continue;
		}
		else
			continue;

		// Print the process name and identifier.
		ucstring ucProcess = ucstring(szProcessName);
		if ( ucName == ucProcess)
		{
			UINT uExitcode = 0;
			bTerminate = kill_process(processID);
		}
		CloseHandle( hProcess );
	}
	return 	bTerminate;
}

BOOL	IsRunningProcess(TCHAR *_ucName)
{
	ucstring	ucName(_ucName);
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; 

	// Get a token for this process. 

	if (!OpenProcessToken(GetCurrentProcess(), 
		TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
		return false; 

	// Get the LUID for the shutdown privilege. 

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, 
		&tkp.Privileges[0].Luid); 

	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

	// Get the shutdown privilege for this process. 

	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
		(PTOKEN_PRIVILEGES)NULL, 0); 

	CloseHandle( hToken );

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return false;

	// Calculate how many process identifiers were returned.

	cProcesses = cbNeeded / sizeof(DWORD);

	// Print the name and process identifier for each process.
	BOOL bIs = false;
	for ( i = 0; i < cProcesses; i++ )
	{
		DWORD processID = aProcesses[i];
		TCHAR szProcessName[MAX_PATH] = _T("unknown");

		// Get a handle to the process.

		HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
			PROCESS_VM_READ,
			FALSE, processID );

		// Get the process name.

		if (NULL != hProcess )
		{
			HMODULE hMod;
			DWORD cbNeeded;

			if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
				&cbNeeded) )
			{
				GetModuleBaseName( hProcess, hMod, szProcessName, 
					sizeof(szProcessName) );
			}
			else 
				continue;
		}
		else
			continue;

		// Print the process name and identifier.
		ucstring ucProcess = ucstring(szProcessName);
		if ( ucName == ucProcess)
		{
			UINT uExitcode = 0;
			bIs = true;
		}
		CloseHandle( hProcess );
	}
	return 	bIs;
}

} // SIPBASE

#endif // SIP_OS_WINDOWS
