/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/mutex.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE

#include "misc/time_sip.h"

#ifdef MUTEX_DEBUG
#include <iostream>
#endif

using namespace std;

#ifndef MUTEX_DEBUG
#define debugCreateMutex() ;
#define debugBeginEnter() ;
#define debugEndEnter() ;
#define debugLeave() ;
#define debugDeleteMutex() ;
#endif


/****************
 * Windows code *
 ****************/

#ifdef SIP_OS_WINDOWS

#include <windows.h>

namespace SIPBASE {

inline void EnterMutex( void *handle )
{
#ifdef SIP_DEBUG
	DWORD timeout;
	if ( IsDebuggerPresent() )
		timeout = INFINITE;
	else
		timeout = 10000;

    // Request ownership of mutex
	DWORD dwWaitResult = WaitForSingleObject (handle, timeout);
#else
    // Request ownership of mutex during 10s
	DWORD dwWaitResult = WaitForSingleObject (handle, 10000);
#endif // SIP_DEBUG
	switch (dwWaitResult)
	{
	// The thread got mutex ownership.
	case WAIT_OBJECT_0:		break;
	// Cannot get mutex ownership due to time-out.
	case WAIT_TIMEOUT:		siperror ("Dead lock in a mutex (or more that 10s for the critical section");
	// Got ownership of the abandoned mutex object.
	case WAIT_ABANDONED:	siperror ("A thread forgot to release the mutex");
	default:				siperror ("EnterMutex failed");
    }
}


inline void LeaveMutex( void *handle )
{
	if (ReleaseMutex(handle) == 0)
	{
		//LPVOID lpMsgBuf;
		//FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		//				 NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &lpMsgBuf, 0, NULL );*/
		siperror ("error while releasing the mutex (0x%x %d), %p", GetLastError(), GetLastError(), handle);
		//LocalFree( lpMsgBuf );
	}
}


/////////////////////////// CUnfairMutex


/*
 * Windows version
 */

CUnfairMutex::CUnfairMutex()
{
	// Create a mutex with no initial owner.
	_Mutex = (void *) CreateMutex( NULL, FALSE, NULL );
	sipassert( _Mutex != NULL );
}


CUnfairMutex::CUnfairMutex( const std::string &name )
{
	// Create a mutex with no initial owner.
	_Mutex = (void *) CreateMutex( NULL, FALSE, NULL );
	sipassert( _Mutex != NULL );
	
	// (Does not use name, only provided for debug compatibility with CFairMutex)
}


/*
 * Windows version
 */
CUnfairMutex::~CUnfairMutex()
{
	CloseHandle( _Mutex );
}


/*
 * Windows version
 */
void CUnfairMutex::enter()
{
	EnterMutex( _Mutex );
}


/*
 * Windows version
 */
void CUnfairMutex::leave()
{
	LeaveMutex( _Mutex );
}


/////////////////////////// CSharedMutexW


/*
 *
 */
CSharedMutex::CSharedMutex()
{
	_Mutex = NULL;
}


/*
 * Create or use an existing mutex (created by another process) with a specific object name (createNow must be false in the constructor)
 * Returns false if it failed.
 */
bool	CSharedMutex::createByName( const char *objectName )
{
#ifdef SIP_DEBUG
	sipassert( _Mutex == NULL );
#endif
	_Mutex = (void *) CreateMutexA( NULL, FALSE, objectName );
	//sipdebug( "Creating mutex %s: handle %p", objectName, _Mutex );
	return ( _Mutex != NULL );
}


/*
 *
 */
void CSharedMutex::destroy()
{
	CloseHandle( _Mutex );
	_Mutex = NULL;
}

/*
 *
 */
void CSharedMutex::enter()
{
	EnterMutex( _Mutex );
}


/*
 *
 */
void CSharedMutex::leave()
{
	LeaveMutex( _Mutex );
}


/////////////////////////// CFairMutex


/*
 * Windows version
 */
CFairMutex::CFairMutex()
{
#ifdef STORE_MUTEX_NAME
	Name = "unset mutex name";
#endif

	debugCreateMutex();

	// Check that the CRITICAL_SECTION structure has not changed
	sipassert( sizeof(TSipRtlCriticalSection)==sizeof(CRITICAL_SECTION) );

#if (_WIN32_WINNT >= 0x0500)
	DWORD dwSpinCount = 0x80000000; // set high-order bit to use preallocation
	if ( ! InitializeCriticalSectionAndSpinCount( (CRITICAL_SECTION*)&_Cs, dwSpinCount ) )
	{
		siperror( "Error entering critical section" );
	}
#else
	InitializeCriticalSection( (CRITICAL_SECTION*)&_Cs );
#endif
}


CFairMutex::CFairMutex(const string &name)
{
#ifdef STORE_MUTEX_NAME
	Name = name;
#endif

#ifdef MUTEX_DEBUG
	debugCreateMutex();
#endif

	// Check that the CRITICAL_SECTION structure has not changed
	sipassert( sizeof(TSipRtlCriticalSection)==sizeof(CRITICAL_SECTION) );

#if (_WIN32_WINNT >= 0x0500)
	DWORD dwSpinCount = 0x80000000; // set high-order bit to use preallocation
	if ( ! InitializeCriticalSectionAndSpinCount( (CRITICAL_SECTION*)&_Cs, dwSpinCount ) )
	{
		siperror( "Error entering critical section" );
	}
#else
	InitializeCriticalSection( (CRITICAL_SECTION*)&_Cs );
#endif
}



/*
 * Windows version
 */
CFairMutex::~CFairMutex()
{
	DeleteCriticalSection( (CRITICAL_SECTION*)&_Cs );

	debugDeleteMutex();
}


/*
 * Windows version
 */
void CFairMutex::enter()
{
	debugBeginEnter();

	EnterCriticalSection( (CRITICAL_SECTION*)&_Cs );

	debugEndEnter();
}


/*
 * Windows version
 */
void CFairMutex::leave()
{
	LeaveCriticalSection( (CRITICAL_SECTION*)&_Cs );

	debugLeave();
}

/*************
 * Unix code *
 *************/

#elif defined SIP_OS_UNIX

#include <pthread.h>
#include <cerrno>
#include <unistd.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h> // System V semaphores



/*
 * Clanlib authors say: "We need to do this because the posix threads library
 * under linux obviously suck:"
 */
extern "C"
{
	int pthread_mutexattr_setkind_np( pthread_mutexattr_t *attr, int kind );
}


namespace SIPBASE {


CUnfairMutex::CUnfairMutex()
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init( &attr );
	// Fast mutex. Note: on Windows all mutexes are recursive
	//pthread_mutexattr_setkind_np( &attr, PTHREAD_MUTEX_RECURSIVE_NP ); //PTHREAD_MUTEX_ERRORCHECK_NP );//PTHREAD_MUTEX_ADAPTIVE_NP );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE ); //lci 09_11-30
	pthread_mutex_init( &mutex, &attr );
	pthread_mutexattr_destroy( &attr );
}


/*
 * Unix version
 */
CUnfairMutex::CUnfairMutex(const std::string &name)
{
	pthread_mutexattr_t attr;
	pthread_mutexattr_init( &attr );
	// Fast mutex. Note: on Windows all mutexes are recursive
	//pthread_mutexattr_setkind_np( &attr, PTHREAD_MUTEX_RECURSIVE_NP ); //PTHREAD_MUTEX_ERRORCHECK_NP );//PTHREAD_MUTEX_ADAPTIVE_NP );
	pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE ); //lci 09-11-30
	pthread_mutex_init( &mutex, &attr );
	pthread_mutexattr_destroy( &attr );
}


/*
 * Unix version
 */
CUnfairMutex::~CUnfairMutex()
{
	pthread_mutex_destroy( &mutex );
}


/*
 * Unix version
 */
void CUnfairMutex::enter()
{
	//cout << getpid() << ": Locking " << &mutex << endl;
	if ( pthread_mutex_lock( &mutex ) != 0 )
	{
	  //cout << "Error locking a mutex " << endl;
		siperror( "Error locking a mutex" );
	}
	/*else
	{
	  cout << getpid() << ": Owning " << &mutex << endl;
	}*/
}


/*
 * Unix version
 */
void CUnfairMutex::leave()
{
	//int errcode;
	//cout << getpid() << ": Unlocking " << &mutex << endl;
	if ( (/*errcode=*/pthread_mutex_unlock( &mutex )) != 0 )
	{
	 /* switch ( errcode )
	    {
	    case EINVAL: cout << "INVAL" << endl; break;
	    case EPERM: cout << "PERM" << endl; break;
	    default: cout << "OTHER" << endl;
	    }
	  */
	  //cout << "Error unlocking a mutex " /*<< &mutex*/ << endl;
		siperror( "Error unlocking a mutex" );
	}
	/*else
	{
	  cout << getpid() << ": Released " << &mutex << endl;
	}*/
}


/*
 * Unix version
 */
CFairMutex::CFairMutex()
{
	sem_init( const_cast<sem_t*>(&_Sem), 0, 1 );
}


CFairMutex::CFairMutex(	const std::string &name )
{
	sem_init( const_cast<sem_t*>(&_Sem), 0, 1 );
}


/*
 * Unix version
 */
CFairMutex::~CFairMutex()
{
	sem_destroy( const_cast<sem_t*>(&_Sem) ); // needs that no thread is waiting on the semaphore
}


/*
 * Unix version
 */
void CFairMutex::enter()
{
	sem_wait( const_cast<sem_t*>(&_Sem) );
}


/*
 * Unix version
 */
void CFairMutex::leave()
{
	sem_post( const_cast<sem_t*>(&_Sem) );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 *
 */
CSharedMutex::CSharedMutex() : _SemId(-1)
{}



#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
/* union semun is defined by including <sys/sem.h> */
#else
/* according to X/OPEN we have to define it ourselves */
union semun {
        int val;                    /* value for SETVAL */
        struct semid_ds *buf;       /* buffer for IPC_STAT, IPC_SET */
        unsigned short int *array;  /* array for GETALL, SETALL */
        struct seminfo *__buf;      /* buffer for IPC_INFO */
};
#endif


/*
 *
 */
bool	CSharedMutex::createByKey( int key, bool createNew )
{
	// Create a semaphore set containing one semaphore
    /*key_t mykey = ftok(".", 'n');
	_SemId = semget( mykey, 1, IPC_CREAT | IPC_EXCL | 0666 );*/

	if ( createNew )
		_SemId = semget( key, 1, IPC_CREAT | IPC_EXCL | 0666 );
	else
		_SemId = semget( key, 1, 0666 );
	sipdebug( "Got semid %d", _SemId );
	if( _SemId == -1 )
		return false;

	// Initialise the semaphore to 1
	union semun arg;
	arg.val = 1;
	if ( semctl( _SemId, 0, SETVAL, arg ) == -1 )
	{
		sipwarning( "semid=%d, err=%s", _SemId, strerror(errno) );
		return false;
	}
	return true;
}


/*
 *
 */
void	CSharedMutex::destroy()
{
	// Destroy the semaphore
	union semun arg;
	sipverifyex( semctl( _SemId, 0, IPC_RMID, arg ) != -1, ("semid=%d, err=%s", _SemId, strerror(errno) ) );
	_SemId = -1;
}


/*
 *
 */
void	CSharedMutex::enter()
{
	// Decrement the semaphore
	sembuf buf;
	buf.sem_num = 0;
	buf.sem_op = -1;
	sipverify( semop( _SemId, &buf, 1 ) != -1);
}


/*
 *
 */
void	CSharedMutex::leave()
{
	// Increment the semaphore
	sembuf buf;
	buf.sem_num = 0;
	buf.sem_op = 1;
	sipverify( semop( _SemId, &buf, 1 ) != -1);
}


#endif // SIP_OS_WINDOWS/SIP_OS_UNIX







/******************
 * Debugging code *
 ******************/
	
#ifdef MUTEX_DEBUG

map<CFairMutex*,TMutexLocks>	*AcquireTime = NULL;
uint32						NbMutexes = 0;
CFairMutex						*ATMutex = NULL;
bool						InitAT = true;


/// Inits the "mutex debugging info system"
void initAcquireTimeMap()
{
	if ( InitAT )
	{
		ATMutex = new CFairMutex("ATMutex");
		AcquireTime = new map<CFairMutex*,TMutexLocks>;
		InitAT = false;
	}
}


/// Gets the debugging info for all mutexes (call it evenly, e.g. once per second)
map<CFairMutex*,TMutexLocks>	getNewAcquireTimes()
{
	map<CMutex*,TMutexLocks>	m;
	ATMutex->enter();

	// Copy map
	m = *AcquireTime;

	// Reset map
/*	map<CMutex*,TMutexLocks>::iterator im;
	for ( im=AcquireTime->begin(); im!=AcquireTime->end(); ++im )
	{
		(*im).second.Time = 0;
		(*im).second.Nb = 0;
		(*im).second.Locked = false;
	}
*/
	ATMutex->leave();
	return m;
}


////////////////////////
////////////////////////

void CFairMutex::debugCreateMutex()
{
/*	if ( ! InitAT )
	{
		ATMutex->enter();
		AcquireTime->insert( make_pair( this, TMutexLocks(NbMutexes) ) );
		NbMutexes++;
		ATMutex->leave();
		char dbgstr [256];
#ifdef STORE_MUTEX_NAME
		smprintf( dbgstr, 256, "MUTEX: Creating mutex %p %s (number %u)\n", this, Name.c_str(), NbMutexes-1 );
#else
		smprintf( dbgstr, 256, "MUTEX: Creating mutex %p (number %u)\n", this, NbMutexes-1 );
#endif
#ifdef SIP_OS_WINDOWS
		if ( IsDebuggerPresent() )
			OutputDebugString( dbgstr );
#endif
		cout << dbgstr << endl;
	}
*/}

void CFairMutex::debugDeleteMutex()
{
	if ( (this!=ATMutex ) && (ATMutex!=NULL) )
	{
		ATMutex->enter();
		(*AcquireTime)[this].Dead = true;
		ATMutex->leave();
	}
}

void CFairMutex::debugBeginEnter()
{
	if ( (this!=ATMutex ) && (ATMutex!=NULL) )
	{
		TTicks t = CTime::getPerformanceTime();

		ATMutex->enter();
		std::map<CMutex*,TMutexLocks>::iterator it = (*AcquireTime).find (this);
		if (it == (*AcquireTime).end())
		{
			AcquireTime->insert( make_pair( this, TMutexLocks(NbMutexes++) ) );
			char dbgstr [256];
#ifdef STORE_MUTEX_NAME
			smprintf( dbgstr, 256, "MUTEX: Creating mutex %p %s (number %u)\n", this, Name.c_str(), NbMutexes-1 );
#else
			smprintf( dbgstr, 256, "MUTEX: Creating mutex %p (number %u)\n", this, NbMutexes-1 );
#endif

#ifdef SIP_OS_WINDOWS
			if ( IsDebuggerPresent() ) OutputDebugString( dbgstr );
#endif
			cout << dbgstr << endl;

			it = (*AcquireTime).find (this);
#ifdef STORE_MUTEX_NAME
			(*it).second.MutexName = Name;
#endif
		}
		(*it).second.WaitingMutex++;
		(*it).second.BeginEnter = t;
		ATMutex->leave();
	}
}


void CFairMutex::debugEndEnter()
{
//	printf("1");
/*	char str[1024];
	sprintf(str, "enter %8p %8p %8p\n", this, _Mutex, getThreadId ());
	if (_Mutex == (void*)0x88)
	{
		OutputDebugString (str);
		if (entered) __asm int 3;
		entered = true;
	}
*/
	if ( ( this != ATMutex ) && ( ATMutex != NULL ) )
	{
		TTicks t = CTime::getPerformanceTime();
		ATMutex->enter();
		if ((uint32)(t-(*AcquireTime)[this].BeginEnter) > (*AcquireTime)[this].TimeToEnter)
			(*AcquireTime)[this].TimeToEnter = (uint32)(t-(*AcquireTime)[this].BeginEnter);
		(*AcquireTime)[this].Nb += 1;
		(*AcquireTime)[this].WaitingMutex--;
		(*AcquireTime)[this].ThreadHavingTheMutex = getThreadId();
		(*AcquireTime)[this].EndEnter = t;
		ATMutex->leave();
	}
}


void CFairMutex::debugLeave()
{
//	printf( "0" );
/*	char str[1024];
	sprintf(str, "leave %8p %8p %8p\n", this, _Mutex, getThreadId ());
	if (_Mutex == (void*)0x88)
	{
		OutputDebugString (str);
		if (!entered) __asm int 3;
		entered = false;
	}
*/

	if ( ( this != ATMutex ) && ( ATMutex != NULL ) )
	{
		TTicks Leave = CTime::getPerformanceTime();
		ATMutex->enter();
		if ((uint32)(Leave-(*AcquireTime)[this].EndEnter) > (*AcquireTime)[this].TimeInMutex)
			(*AcquireTime)[this].TimeInMutex = (uint32)(Leave-(*AcquireTime)[this].EndEnter);
		(*AcquireTime)[this].Nb += 1;
		(*AcquireTime)[this].WaitingMutex = false;
		(*AcquireTime)[this].ThreadHavingTheMutex = 0xFFFFFFFF;
		ATMutex->leave();
	}

}

#endif // MUTEX_DEBUG

} // SIPBASE
