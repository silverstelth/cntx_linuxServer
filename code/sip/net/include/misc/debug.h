/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdio.h> // <cstdio>

#include "common.h"
#include "log.h"
#include "mutex.h"
#include "mem_displayer.h"
#include "displayer.h"
#include "app_context.h"
#include <set>

namespace SIPBASE
{	

#ifdef ASSERT_THROW_EXCEPTION
#define ASSERT_THROW_EXCEPTION_CODE(exp) ASSERT_THROW_EXCEPTION_CODE_EX(exp, #exp)
#define ASSERT_THROW_EXCEPTION_CODE_EX(exp, str) if(!(exp)) throw SIPBASE::Exception(str" returns false");
#else
#define ASSERT_THROW_EXCEPTION_CODE(exp)
#define ASSERT_THROW_EXCEPTION_CODE_EX(exp, str)
#endif

/** Imposter class to wrap all global access to the sip context for backward compatibility
 *	Yoyo note: This was a template before, hence with inline. 
 *	We removed the inline because there was a hard compilation bug
 *	in plugin max under some compiler wich caused operator-> to crash.... (don't understand why grrrr)
 *	Btw the method is optimized like this (1 call instead of 3 (and one with virual)) because we added a local cache (_Log)
 *	Thus it is much better like this.
 */
class CImposterLog
{
private:
	typedef CLog *(ISipContext::*TAccessor)();

	// Method to access the Log
	TAccessor	_Accessor;
public:
	CImposterLog(TAccessor accessor);
	
	CLog *operator -> ();
	
	operator CLog*();
	
	CLog &operator ()();
};

//
// Externals
//

extern bool	g_bLog;

// NOTE: The following are all NULL until createDebug() has been called at least once
extern CImposterLog		ErrorLog;
extern CImposterLog		WarningLog;
extern CImposterLog		InfoLog;
extern CImposterLog		DebugLog;
extern CImposterLog		AssertLog;

extern CMemDisplayer	*DefaultMemDisplayer;
extern CMsgBoxDisplayer	*DefaultMsgBoxDisplayer;

//
// Functions
//

// internal use only
#ifdef SIP_USE_UNICODE
void createDebug (const ucchar *logPath = NULL, bool logInFile = false);
#else
void createDebug (const char *logPath = NULL, bool logInFile = false);
#endif

// call this if you want to change the dir of the sip.log file
void changeLogDirectory (const std::string &dir);
void changeLogDirectoryW(const std::basic_string<ucchar> &dir);

// internal breakpoint window
void enterBreakpoint (const char *message);

// if true, the assert generates an assert
// if false, the assert just displays a warning and continue
void setAssert (bool assert);

// Beep (Windows only, no effect elsewhere)
void beep( uint freq, uint duration );


typedef std::string (*TCrashCallback)();

// this function enables user application to add information in the log when a crash occurs
void setCrashCallback(TCrashCallback crashCallback);

// For Crash report window. allow to know if a crash has alredy raised in the application
bool	isCrashAlreadyReported();
void	setCrashAlreadyReported(bool state);


// This very amazing macro __FUNCTION__ doesn't exist on VC6, map it to NULL
#ifdef SIP_COMP_VC6
#	define __FUNCTION__ NULL
#endif


// Macros

/// Utility macro used by SIP_MACRO_TO_STR to concatenate macro in text message.
#define SIP_MACRO_TO_STR_SUBPART(x) #x

/** Use this macro to concatenate macro such
 *	You can use this macro to build '#pragma message' friendly macro
 *	or to make macro definition into string
 *	eg : #define M1 foo
 *		 #define MESSAGE "the message is "SIP_MACRO_TO_STR(M1)
 *		 #pragma message(MESSAGE)
 *		 printf(SIP_MACRO_TO_STR(M1));
 */
#define SIP_MACRO_TO_STR(x) SIP_MACRO_TO_STR_SUBPART(x)

/** the two following macros help to build compiler message using #pragma message
 *	on visual C++.
 *	The macro generate a message formated like the visual C++ compiler message.
 *	SIP_LOC_MSG generate informative message and
 *	SIP_LOC_WRN generate warning message not differentiable to genuine Visual C++ warning.
 *	The two message allow automatic source access with F4 or double click in 
 *	output window.
 *
 *  usage : #pragma message( SIP_LOC_MGS "your message" )
 *
 *  Note : If you want to concatenate another macro to your message, you
 *			can append using the SIP_MACRO_TO_STR macro like in
 *			#define CLASS_NAME TheClassName
 *			#pragma message( SIP_LOC_MGS "The class name is " SIP_MACRO_TO_STR(CLASS_NAME))
 */
#define SIP_LOC_MSG __FILE__ "(" SIP_MACRO_TO_STR(__LINE__) ") : Message: "
#define SIP_LOC_WRN __FILE__ "(" SIP_MACRO_TO_STR(__LINE__) ") : Warning Msg: "


/**
 * \def sipdebug(exp)
 * Log a debug string. You don't have to put the final new line. It will be automatically append at the end of the string.
 *
 * Example:
 *\code
	void function(sint type)
	{
		// display the type value.
		sipdebug("type is %d", type);
	}
 *\endcode
 */
#ifdef SIP_RELEASE
#	ifdef SIP_COMP_VC71
#		define sipdebug  DEBUG_TO_LOG // __noop
#	else
#		define sipdebug  DEBUG_TO_LOG // 0&&
#	endif
#else // SIP_RELEASE
#	define sipdebug DEBUG_TO_LOG
#endif // SIP_RELEASE

/**
 * \def sipinfo(exp)
 * Same as sipdebug but it will be display in debug and in release mode.
 */
#ifdef SIP_RELEASE
#	ifdef SIP_COMP_VC71
#		define sipinfo  INFO_TO_LOG // __noop
#	else
#		define sipinfo  INFO_TO_LOG // 0&&
#	endif
#else // SIP_RELEASE
#	define sipinfo	INFO_TO_LOG
#endif // SIP_RELEASE

/**
 * \def sipwarning(exp)
 * Same as sipinfo but you have to call this macro when something goes wrong but it's not a fatal error, the program could continue.
 *
 * Example:
 *\code
	void function(char *str)
	{
		// display the type value.
		if (str==NULL)
		{
			sipwarning("in function(), str should not be NULL, assume it's an empty string");
			str="";
		}
	}
 *\endcode
 */

#ifdef SIP_RELEASE
#	ifdef SIP_COMP_VC71
#		define sipwarning  WARNING_TO_LOG // __noop
#	else
#		define sipwarning  WARNING_TO_LOG // 0&&
#	endif
#else // SIP_RELEASE
#	define sipwarning WARNING_TO_LOG
#endif // SIP_RELEASE

/**
 * \def siperror(exp)
 * Same as sipinfo but you have to call it when you have a fatal error, this macro display the text and \b exit the application
 * automatically. siperror must be in a try/catch because it generates an EFatalError exception to exit the application.
 *
 *\code
	void function(char *filename)
	{
		FILE *fp = fopen (filename, "r");
		if (fp==NULL)
		{
			siperror("file not found");
		}
	}
 *\endcode
 */
#define siperror SIPBASE::createDebug(), SIPBASE::ErrorLog->setPosition( __LINE__, __FILE__, __FUNCTION__ ), SIPBASE::sipFatalError

/**
 * \def siperrornoex(exp)
 * Same as siperror but it doesn't generate any exceptions. It's used only in very specific case, for example, when you
 * call a siperror in a catch block (look the service.cpp)
 */
#define siperrornoex SIPBASE::createDebug(), SIPBASE::ErrorLog->setPosition( __LINE__, __FILE__, __FUNCTION__ ), SIPBASE::sipError

/**
 * \def sipassert(exp)
 * Try the assertion of \c exp. In release mode, sipassert do \b *nothing*. In debug mode, If \c exp is true, nothing happen,
 * otherwise, the program stop on the assertion line.
 *
 * Example:
 *\code
	void function(char *string)
	{
		// the string must not be NULL.
		sipassert(string!=NULL);
	}
 *\endcode
 */

/**
 * \def sipassertonce(exp)
 * Same behaviour as sipassert but the assertion will be test on time only. It's useful when you are under an inner loop and
 * you don't want to flood log file for example.
 *
 * Example:
 *\code
	for (int i = 0; i < Vect.size(); i++)
	{
		// all string must not be NULL, but if it's happen, log only the first time.
		sipassertonce(Vect[i].string!=NULL);
	}
 *\endcode
 */

/**
 * \def sipassertex(exp,str)
 * Same behaviour as sipassert but add a user defined \c str variables args string that will be display with the assert message.
 * Very useful when you can't debug directly on your computer.
 *
 * Example:
 *\code
	void function(sint type)
	{
		// the \c type must be between 0 and 15.
		sipassertex(type>=0&&type<=16, ("type was %d", type));
		// it'll display something like "assertion failed line 10 of test.cpp: type>=&&type<=16, type was 423555463"
	}
 *\endcode
 */

/**
 * \def sipverify(exp)
 * Same behaviour as sipassert but the \c exp will be executed in release mode too (but not tested).
 *
 * Example:
 *\code
	// Load a file and assert if the load failed. This example will work \b only in debug mode because in release mode,
	// sipassert do nothing, the the load function will not be called...
	sipassert(load("test.tga"));

	// If you want to do that anyway, you could call sipverify. In release mode, the assertion will not be tested but
	// the \c load function will be called.
	sipverify(load("test.tga"));

	// You can also do this:
	bool res = load ("test.tga"));
	assert(res);
 
 *\endcode
 */

/**
 * \def sipverifyonce(exp)
 * Same behaviour as sipassertonce but it will execute \c exp in debug and release mode.
 */

/**
 * \def sipverifyex(exp,str)
 * Same behaviour as sipassertex but it will execute \c exp in debug and release mode.
 */

/**
 * \def sipstop
 * It stop the application at this point. It's exactly the same thing as "sipassert(false)".
 * Example:
 *\code
	switch(type)
	{
	case 1: ... break;
	case 2: ... break;
	default: sipstop;	// it should never happen...
	}
 *\endcode
 */

/**
 * \def sipstoponce
 * Same as sipassertonce(false);
 */

/**
 * \def sipstopex(exp)
 * Same as sipassertex(false,exp);
 */

// removed because we always check assert (even in release mode) #if defined (SIP_OS_WINDOWS) && defined (SIP_DEBUG)
#if defined (SIP_OS_WINDOWS)
#define SIPBASE_BREAKPOINT _asm { int 3 }
#else
#define SIPBASE_BREAKPOINT abort()
#endif

// Internal, don't use it (make smaller assert code)
extern bool _assert_stop(bool &ignoreNextTime, sint line, const char *file, const char *funcName, const char *exp);
extern void _assertex_stop_0(bool &ignoreNextTime, sint line, const char *file, const char *funcName, const char *exp);
extern bool _assertex_stop_1(bool &ignoreNextTime);

// removed because we always check assert (even in release mode) #if defined(SIP_DEBUG)

#ifdef SIP_RELEASE
#define sipassert(exp) \
if(false)
#define sipassertonce(exp) \
if(false)
#define sipassertex(exp, str) \
if(false)
#define sipverify(exp) \
{ exp; }
#define sipverifyonce(exp) \
{ exp; }
#define sipverifyex(exp, str) \
{ exp; }
#else // SIP_RELEASE

#ifdef SIP_OS_UNIX

// Linux set of asserts is reduced due tothat there is no message box displayer
#define sipassert(exp) \
{ \
	if (!(exp)) { \
		SIPBASE::createDebug (); \
		SIPBASE::AssertLog->setPosition (__LINE__, __FILE__, __FUNCTION__); \
		SIPBASE::AssertLog->displayNL ("\"%s\" ", #exp); \
		SIPBASE_BREAKPOINT; \
	} \
}

#define sipassertonce(exp) sipassert(exp)

#define sipassertex(exp, str) \
{ \
	if (!(exp)) { \
		SIPBASE::createDebug (); \
		SIPBASE::AssertLog->setPosition (__LINE__, __FILE__, __FUNCTION__); \
		SIPBASE::AssertLog->displayNL ("\"%s\" ", #exp); \
		SIPBASE::AssertLog->displayRawNL (#str); \
		SIPBASE_BREAKPOINT; \
	} \
}

#define sipverify(exp) sipassert(exp)
#define sipverifyonce(exp) sipassert(exp)
#define sipverifyex(exp, str) sipassertex(exp, str)

#else // SIP_OS_UNIX

#define sipassert(exp) \
{ \
	static bool ignoreNextTime = false; \
	bool _expResult_ = (exp) ? true : false; \
	if (!ignoreNextTime && !_expResult_) { \
		if(SIPBASE::_assert_stop(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, #exp)) \
			SIPBASE_BREAKPOINT; \
			SIPBASE::sipFatalError(#exp);\
	} \
	ASSERT_THROW_EXCEPTION_CODE_EX(_expResult_, #exp) \
}

#define sipassertonce(exp) \
{ \
	static bool ignoreNextTime = false; \
	if (!ignoreNextTime && !(exp)) { \
		ignoreNextTime = true; \
		if(SIPBASE::_assert_stop(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, #exp)) \
			SIPBASE_BREAKPOINT; \
	} \
}

#define sipassertex(exp, str) \
{ \
	static bool ignoreNextTime = false; \
	bool _expResult_ = (exp) ? true : false; \
	if (!ignoreNextTime && !_expResult_) { \
		SIPBASE::_assertex_stop_0(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, #exp); \
		SIPBASE::AssertLog->displayRawNL str; \
		if(SIPBASE::_assertex_stop_1(ignoreNextTime)) \
			SIPBASE_BREAKPOINT; \
	} \
	ASSERT_THROW_EXCEPTION_CODE_EX(_expResult_, #exp) \
}

#define sipverify(exp) \
{ \
	static bool ignoreNextTime = false; \
	bool _expResult_ = (exp) ? true : false; \
	if (!_expResult_ && !ignoreNextTime) { \
		if(SIPBASE::_assert_stop(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, #exp)) \
			SIPBASE_BREAKPOINT; \
	} \
	ASSERT_THROW_EXCEPTION_CODE_EX(_expResult_, #exp) \
}

#define sipverifyonce(exp) \
{ \
	static bool ignoreNextTime = false; \
	bool _expResult_ = (exp) ? true : false; \
	if (!_expResult_ && !ignoreNextTime) { \
		ignoreNextTime = true; \
		if(SIPBASE::_assert_stop(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, #exp)) \
			SIPBASE_BREAKPOINT; \
	} \
}

#define sipverifyex(exp, str) \
{ \
	static bool ignoreNextTime = false; \
	bool _expResult_ = (exp) ? true : false; \
	if (!_expResult_ && !ignoreNextTime) { \
		SIPBASE::_assertex_stop_0(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, #exp); \
		SIPBASE::AssertLog->displayRawNL str; \
		if(SIPBASE::_assertex_stop_1(ignoreNextTime)) \
			SIPBASE_BREAKPOINT; \
	} \
	ASSERT_THROW_EXCEPTION_CODE_EX(_expResult_, #exp) \
}

#endif // SIP_OS_UNIX

#endif // SIP_RELEASE


#define sipstop \
{ \
	static bool ignoreNextTime = false; \
	if (!ignoreNextTime) { \
		if(SIPBASE::_assert_stop(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, NULL)) \
			SIPBASE_BREAKPOINT; \
	} \
	ASSERT_THROW_EXCEPTION_CODE(false) \
}

#define sipstoponce \
{ \
	static bool ignoreNextTime = false; \
	if (!ignoreNextTime) { \
		ignoreNextTime = true; \
		if(SIPBASE::_assert_stop(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, NULL)) \
			SIPBASE_BREAKPOINT; \
	} \
}

#define sipstopex(str) \
{ \
	static bool ignoreNextTime = false; \
	if (!ignoreNextTime) { \
		SIPBASE::_assertex_stop_0(ignoreNextTime, __LINE__, __FILE__, __FUNCTION__, NULL); \
		SIPBASE::AssertLog->displayRawNL str; \
		if(SIPBASE::_assertex_stop_1(ignoreNextTime)) \
			SIPBASE_BREAKPOINT; \
	} \
}

struct EFatalError : public Exception
{
	EFatalError() : Exception( "siperror() called" ) {}
};

class ETrapDebug : public Exception
{
};

// undef default assert to force people to use sipassert() instead of assert()
// if SIP_MAP_ASSERT is set we map assert to sipassert instead of removing it
// this makes it compatible with zeroc's ICE network library

#ifdef SIP_MAP_ASSERT
#	ifdef assert
#		undef assert
#		define assert sipassert
#	endif
#else
#	ifdef assert
#		undef assert
#		define assert(a) you_must_not_use_assert___use_sipassert___read_debug_h_file
#	endif
#endif


/// Get the call stack and set it with result
void getCallStackAndLogA(std::string &result, sint skipNFirst = 0);
void getCallStackAndLogW(std::basic_string<ucchar> &result, sint skipNFirst = 0);

/**
 * safe_cast<>: this is a function which sipassert() a dynamic_cast in Debug, and just do a static_cast in release.
 * So slow check is made in debug, but only fast cast is made in release.
 */
template<class T, class U>	inline T	safe_cast(U o)
{
	// NB: must check debug because assert may still be here in release
#ifdef SIP_DEBUG
	sipassert(dynamic_cast<T>(o));
#endif
	return static_cast<T>(o);
}

/**
 * type_cast<>: this is a function which sipassert() a dynamic_cast in Debug, and just do a static_cast in release.
 * So slow check is made in debug, but only fast cast is made in release.
 * Differs from safe_cast by allowinf NULL objets. (ask Stephane LE DORZE for more explanations).
 */
template<class T, class U>	inline T	type_cast(U o)
{
	// NB: must check debug because assert may still be here in release
#ifdef SIP_DEBUG
	if (o)
		sipassert(dynamic_cast<T>(o));
#endif
	//	optimization made to check pointer validity before address translation. (hope it works on linux).
	if ((size_t)(static_cast<T>((U)0x0400)) == (size_t)((U)0x0400))
	{
		return static_cast<T>(o);
	}
	else
	{
		return (o==0)?0:static_cast<T>(o);
	}
}

/** Compile time assertion
  */
#define sipctassert(cond) sizeof(uint[(cond) ? 1 : 0])

/**
*	Allow to verify an object was accessed before its destructor call.
*	For instance, it could be used to check if the user take care of method call return.
*	ex:
*		CMustConsume<TErrorCode>	foo()
*		{
*			...
*			return	ErrorInvalidateType;		//	part of TErrorCode enum.
*		}
*	Exclusive implementation samples:
*		TerrorCode code=foo().consumeValue();	//	Good!
*		foo().consume();						//	Good!
*		TerrorCode code=foo();					//	Mistake!
*		foo();									//	Will cause an assert at next ending brace during execution time.
*		TerrorCode code=foo().Value();			//	Will cause an assert at next ending brace during execution time.
*	(ask Stephane LE DORZE for more explanations).
*/

// Need a breakpoint in the assert / verify macro
extern bool DebugNeedAssert;

// Internal process, don't use it
extern bool NoAssert;


template<class T>
class CMustConsume
{
public:
	CMustConsume(const T &val) : Value(val)
#if !FINAL_VERSION
	, Consumed(false)
#endif
	{
	}

	~CMustConsume()
	{
#if !FINAL_VERSION
		sipassert(Consumed == true);
#endif
	}

	//	Get the value without validating the access.
	const T &value() const
	{
		return Value;
	}

	operator const T &() const
	{
#if !FINAL_VERSION
		Consumed = true;
#endif
		return Value;
	}
	
	//	Get the value and validate the access.
	const T &consumeValue() const
	{
#if !FINAL_VERSION
		Consumed = true;
#endif
		return Value;
	}
	//	Only consume the access.
	void consume() const
	{
#if !FINAL_VERSION
		Consumed = true;
#endif
	}
	
private:
	T				Value;
#if !FINAL_VERSION
	mutable	bool	Consumed;
#endif
};

/// Data for instance counting
struct TInstanceCounterData
{
	sint32	_InstanceCounter;
	sint32	_DeltaCounter;
	const char	*_ClassName;

	TInstanceCounterData(const char *className);

	~TInstanceCounterData();
};

// forward declaration for members of CInstanceCounterManager
class CInstanceCounterLocalManager;

// The singleton used to display the instance counter
class CInstanceCounterManager
{
//	SIPBASE_SAFE_SINGLETON_DECL(CInstanceCounterManager);
	private:
		/* declare private constructors*/ 
		CInstanceCounterManager () {}
		CInstanceCounterManager (const CInstanceCounterManager &) {}
		/* the local static pointer to the singleton instance */ 
		static CInstanceCounterManager	*_Instance;

	public:
		static	void	free_CInstanceCounterManager(void * _cl) { CInstanceCounterManager *Ins = (CInstanceCounterManager *)_cl;	delete Ins; } \
		static CInstanceCounterManager &getInstance() 
		{ 
			if (_Instance == NULL) 
			{ 
				/* the sip context MUST be initialised */ 
//				sipassert(SIPBASE::SipContext != NULL); 
				void *ptr = SIPBASE::ISipContext::getInstance().getSingletonPointer("CInstanceCounterManager"); 
				if (ptr == NULL) 
				{ 
					/* allocate the singleton and register it */ 
					_Instance = new CInstanceCounterManager; 
					SIPBASE::ISipContext::getInstance().setSingletonPointer("CInstanceCounterManager", _Instance, free_CInstanceCounterManager); 
				} 
				else 
				{ 
					_Instance = reinterpret_cast<CInstanceCounterManager*>(ptr); 
				} 
			} 
			return *_Instance; 
		} 
	private:
public:

	std::string displayCounters() const;

	void resetDeltaCounter();

	uint32 getInstanceCounter(const std::string &className) const;
	sint32 getInstanceCounterDelta(const std::string &className) const;

private:

	friend class CInstanceCounterLocalManager;

	void registerInstaceCounterLocalManager(CInstanceCounterLocalManager *localMgr);
	void unregisterInstaceCounterLocalManager(CInstanceCounterLocalManager *localMgr);

//	static CInstanceCounterManager		*_Instance;
	std::set<CInstanceCounterLocalManager*>	_InstanceCounterMgrs;
};

//
// Local instance counter
//
class CInstanceCounterLocalManager
{
public:
	static CInstanceCounterLocalManager &getInstance()
	{
		if (_Instance == NULL)
		{
			_Instance = new CInstanceCounterLocalManager;
		}
		return *_Instance;
	}

	static void releaseInstance()
	{
		if (_Instance != NULL)
		{	
			delete _Instance;
			_Instance = NULL;
		}
	}

	void registerInstanceCounter(TInstanceCounterData *counter)
	{
		_InstanceCounters.insert(counter);
	}

	void unregisterInstanceCounter(TInstanceCounterData *counter);

	~CInstanceCounterLocalManager()
	{
		CInstanceCounterManager::getInstance().unregisterInstaceCounterLocalManager(this);
	}

private:
	friend class CInstanceCounterManager;
	friend class ISipContext;

	CInstanceCounterLocalManager()
	{
	}

	void registerLocalManager()
	{
		CInstanceCounterManager::getInstance().registerInstaceCounterLocalManager(this);
	}

	static CInstanceCounterLocalManager		*_Instance;
	std::set<TInstanceCounterData*>			_InstanceCounters;
};


/** Utility to count instance of class. 
 *	This class is designed to be lightweight and to trace 
 *	the number of instance of 'tagged' class.
 *	Commands are provided to display the actual number
 *	of object of each class and to compute delta
 *	between two call of the displayer.
 *	Usage is simple, you just have to put a macro
 *	inside the class definition to trace it's allocation
 *	and a macro in the implementation file.
 *	The macro only add a compiler minimum size for member
 *	struct of 0 octets (witch can be 0 or 1 octet, compiler
 *	dependend).
 *	usage :
 *	
 *	In the header :
 *	class foo	// This is the class we want to count instance
 *	{
 *		SIP_INSTANCE_COUNTER_DECL(foo);
 *	}
 *	In the cpp :
 *	SIP_INSTANCE_COUNTER_IMPL(foo);
 */
#define SIP_INSTANCE_COUNTER_DECL(className) \
public: \
	struct className##InstanceCounter \
	{ \
		className##InstanceCounter() \
		{ \
			_InstanceCounterData._InstanceCounter++; \
		} \
		className##InstanceCounter(const className##InstanceCounter &other) \
		{ \
			_InstanceCounterData._InstanceCounter++; \
		} \
		\
		~className##InstanceCounter()\
		{ \
			_InstanceCounterData._InstanceCounter--; \
		} \
		static sint32 getInstanceCounter() \
		{ \
			return _InstanceCounterData._InstanceCounter; \
		} \
		static sint32 getInstanceCounterDelta() \
		{ \
			return _InstanceCounterData._InstanceCounter - _InstanceCounterData._DeltaCounter; \
		} \
		static SIPBASE::TInstanceCounterData	_InstanceCounterData; \
	}; \
	 \
	className##InstanceCounter	_##className##InstanceCounter; \
private:

/// The macro to make the implementation of the counter
#define SIP_INSTANCE_COUNTER_IMPL(className) SIPBASE::TInstanceCounterData className::className##InstanceCounter::_InstanceCounterData(#className);

/// An utility macro to get the instance counter for a class
#define SIP_GET_LOCAL_INSTANCE_COUNTER(className) className::className##InstanceCounter::getInstanceCounter()
#define SIP_GET_INSTANCE_COUNTER(className) SIPBASE::CInstanceCounterManager::getInstance().getInstanceCounter(#className)

/// An utility macro to get the delta since the last counter reset.
#define SIP_GET_LOCAL_INSTANCE_COUNTER_DELTA(className) className::className##InstanceCounter::getInstanceCounterDelta()
#define SIP_GET_INSTANCE_COUNTER_DELTA(className) SIPBASE::CInstanceCounterManager::getInstance().getInstanceCounterDelta(#className)

//
// Following are internal functions, you should never use them
//

/// Never use this function (internal use only)
void sipFatalErrorA (const char *format, ...);
void sipFatalErrorW (const ucchar *format, ...);
void sipFatalErrorW (const char *format, ...);

/// Never use this function but call the siperror macro (internal use only)
void sipErrorA(const char *format, ...);
void sipErrorW(const ucchar *format, ...);
void sipErrorW(const char *format, ...);

#define SIP_CRASH_DUMP_FILE		"Sip_debug.dmp"
#define SIP_CRASH_DUMP_FILE_W	L"Sip_debug.dmp"

} // SIPBASE

#endif // SIP_DEBUG_H

/* End of debug.h */
