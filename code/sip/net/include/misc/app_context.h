/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __APP_CONTEXT_H__
#define __APP_CONTEXT_H__

#include "misc/types_sip.h"
#include <map>

namespace SIPBASE
{
	class CLog;
    class CStdDisplayer;
	class CFileDisplayer;
	class CMemDisplayer;
	class CMsgBoxDisplayer;

	typedef	void (*FUNC_FREESINGLETONE)(void *_cl);
	/// Singleton registry
	struct TSingletonRegistryProperty 
	{
		TSingletonRegistryProperty(void *pI, FUNC_FREESINGLETONE pF) : pInstance(pI), pFreeFunc(pF) {}
		TSingletonRegistryProperty() : pInstance(NULL), pFreeFunc(NULL) {}
		void					*pInstance;
		FUNC_FREESINGLETONE		pFreeFunc;
	};
	typedef std::map<std::string, TSingletonRegistryProperty> TSingletonRegistry;

	class CAutoDestructorSingletone
	{
	public:
		~CAutoDestructorSingletone();
		void setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func);
	private:
		TSingletonRegistry		_SingletonRegistry;
	};
	extern void AutoDestructorSingletone_setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func);
	/** Interface definition for sip context.
	 *	Any application wide data can be accessed thru this interface.
	 *
	 *	The SIP context is a mean to allow dynamic library loading in SIP.
	 *	In order to make all SIP application safe, it is mandatory to declare
	 *	a SIP context at startup of any application (first instruction of the
	 *	main() or WinMain() is good practice).
	 *	Note that for SIPNET::IService oriented application, service framwork
	 *	already provide the application context.
	 *
	 *  \date 2005
	 */
	class ISipContext
	{
	public:

		/// Access to the context singleton
		static ISipContext &getInstance();

		static bool isContextInitialised();

		virtual ~ISipContext();

		//@name Singleton registry
		//@{
		/** Return the pointer associated to a given singleton name
		 *	If the name is not present, return NULL.
		 */
		virtual void *getSingletonPointer(const std::string &singletonName) =0;
		/** Register a singleton pointer.
		*/
		virtual void setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func) =0;
		/** Release a singleton pointer */
		virtual void releaseSingletonPointer(const std::string &singletonName, void *ptr) =0;
		//@}

		//@name Global debugging object
		//@{
		virtual CLog *getErrorLog() =0;
		virtual void setErrorLog(CLog *errorLog) =0;
		
		virtual CLog *getWarningLog() =0;
		virtual void setWarningLog(CLog *warningLog) =0;

		virtual CLog *getInfoLog() =0;
		virtual void setInfoLog(CLog *infoLog) =0;

		virtual CLog *getDebugLog() =0;		
		virtual void setDebugLog(CLog *debugLog) =0;

		virtual CLog *getAssertLog() =0;		
		virtual void setAssertLog(CLog *assertLog) =0;
		
		virtual CStdDisplayer *getDefaultStdDisplayer() =0;
		virtual void setDefaultStdDisplayer(CStdDisplayer *stdDisplayer) =0;

		virtual CFileDisplayer *getDefaultFileDisplayer() =0;
		virtual void setDefaultFileDisplayer(CFileDisplayer *fileDisplayer) =0;

		virtual CMemDisplayer *getDefaultMemDisplayer() =0;
		virtual void setDefaultMemDisplayer(CMemDisplayer *memDisplayer) =0;
		
		virtual CMsgBoxDisplayer *getDefaultMsgBoxDisplayer() =0;
		virtual void setDefaultMsgBoxDisplayer(CMsgBoxDisplayer *msgBoxDisplayer) =0;
		
		virtual bool getDebugNeedAssert() =0;
		virtual void setDebugNeedAssert(bool needAssert) =0;
		virtual bool getNoAssert() =0;
		virtual void setNoAssert(bool noAssert) =0;
		virtual bool getAlreadyCreateSharedAmongThreads() =0;
		virtual void setAlreadyCreateSharedAmongThreads(bool b) =0;
		//@}
	protected:
		/// Called by derived class to finalize initialisation of context
		void	contextReady();

		static ISipContext *_SipContext;		

	public:
		
		virtual void releaseContext(void) = 0;
	};

	/** This class implement the context interface for the application module
	 *	That mean that this class will really hold the data.
	 *  \date 2005
	 */
	class CApplicationContext : public ISipContext
	{
	public:
		
		CApplicationContext();
		virtual ~CApplicationContext();

		virtual void *getSingletonPointer(const std::string &singletonName);
		virtual void setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func);
		virtual void releaseSingletonPointer(const std::string &singletonName, void *ptr);

		virtual CLog  *getErrorLog();
		virtual void setErrorLog (CLog  *errorLog);
		
		virtual CLog  *getWarningLog();
		virtual void setWarningLog (CLog  *warningLog);

		virtual CLog  *getInfoLog();
		virtual void setInfoLog (CLog  *infoLog);

		virtual CLog  *getDebugLog();		
		virtual void setDebugLog (CLog  *debugLog);

		virtual CLog  *getAssertLog();		
		virtual void setAssertLog (CLog  *assertLog);
		
		virtual CStdDisplayer  *getDefaultStdDisplayer();
		virtual void setDefaultStdDisplayer (CStdDisplayer  *stdDisplayer);

		virtual CFileDisplayer  *getDefaultFileDisplayer();
		virtual void setDefaultFileDisplayer (CFileDisplayer  *fileDisplayer);

		virtual CMemDisplayer  *getDefaultMemDisplayer();
		virtual void setDefaultMemDisplayer (CMemDisplayer  *memDisplayer);
		
		virtual CMsgBoxDisplayer  *getDefaultMsgBoxDisplayer();
		virtual void setDefaultMsgBoxDisplayer (CMsgBoxDisplayer  *msgBoxDisplayer);

		virtual bool getDebugNeedAssert();
		virtual void setDebugNeedAssert(bool needAssert);
		virtual bool getNoAssert();
		virtual void setNoAssert(bool noAssert);
		virtual bool getAlreadyCreateSharedAmongThreads();
		virtual void setAlreadyCreateSharedAmongThreads(bool b);
	
	private:
		TSingletonRegistry		_SingletonRegistry;

		CLog  *ErrorLog;
		CLog  *WarningLog;		
		CLog  *InfoLog;
		CLog  *DebugLog;
		CLog  *AssertLog;

		CStdDisplayer		*DefaultStdDisplayer;
		CFileDisplayer		*DefaultFileDisplayer;
		CMemDisplayer		*DefaultMemDisplayer;
		CMsgBoxDisplayer	*DefaultMsgBoxDisplayer;

		bool DebugNeedAssert;
		bool NoAssert;
		bool AlreadyCreateSharedAmongThreads;
	
	public:
		virtual void releaseContext(void);
	};

	/** This class implement the context interface for the a library module.
	 *	All it contains is forward call to the application context instance.
	 *  \date 2005
	 */
	class CLibraryContext : public ISipContext
	{
	public:
		CLibraryContext (ISipContext &applicationContext);
		virtual ~CLibraryContext();

		virtual void *getSingletonPointer(const std::string &singletonName);
		virtual void setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func);
		virtual void releaseSingletonPointer(const std::string &singletonName, void *ptr);

		virtual CLog *getErrorLog();
		virtual void setErrorLog(CLog *errorLog);

		virtual CLog *getWarningLog();
		virtual void setWarningLog(CLog *warningLog);

		virtual CLog *getInfoLog();
		virtual void setInfoLog(CLog *infoLog);

		virtual CLog *getDebugLog();		
		virtual void setDebugLog(CLog *debugLog);

		virtual CLog *getAssertLog();		
		virtual void setAssertLog(CLog *assertLog);
		
		virtual CStdDisplayer *getDefaultStdDisplayer();
		virtual void setDefaultStdDisplayer(CStdDisplayer *stdDisplayer);

		virtual CFileDisplayer *getDefaultFileDisplayer();
		virtual void setDefaultFileDisplayer(CFileDisplayer *fileDisplayer);

		virtual CMemDisplayer *getDefaultMemDisplayer();
		virtual void setDefaultMemDisplayer(CMemDisplayer *memDisplayer);
		
		virtual CMsgBoxDisplayer *getDefaultMsgBoxDisplayer();
		virtual void setDefaultMsgBoxDisplayer(CMsgBoxDisplayer *msgBoxDisplayer);

		virtual bool getDebugNeedAssert();
		virtual void setDebugNeedAssert(bool needAssert);
		virtual bool getNoAssert();
		virtual void setNoAssert(bool noAssert);
		virtual bool getAlreadyCreateSharedAmongThreads();
		virtual void setAlreadyCreateSharedAmongThreads(bool b);

	private:
		/// Pointer to the application context.
		ISipContext		&_ApplicationContext;

	public:
		virtual void releaseContext(void);
	};


	//@name Singleton utility
	//@{
	/** Some utility macro to build singleton compatible with
	 *	the dynamic loading of library
	 *	This macro must be put inside the singleton class
	 *	definition.
	 *	Warning : this macro change the current access right, it end up with
	 *	private access right.
	 */
#define SIPBASE_SAFE_SINGLETON_DECL(className) \
	private:\
		/* declare private constructors*/ \
		/*className () {}*/\
		/*className (const className &) {}*/\
		/* the local static pointer to the singleton instance */ \
		static className *_Instance; \
	public:\
		static	void	free_##className(void * _cl) { className *Ins = (className *)_cl;	delete Ins; } \
		static className &getInstance() \
		{ \
			if (_Instance == NULL) \
			{ \
				/* the SIP context MUST be initialised */ \
				sipassertex(SIPBASE::ISipContext::isContextInitialised(), ("You are trying to access a safe singleton without having initialized a SIP context. The simplest correction is to add 'SIPBASE::CApplicationContext myApplicationContext;' at the very begining of your application.")); \
				void *ptr = SIPBASE::ISipContext::getInstance().getSingletonPointer(#className); \
				if (ptr == NULL) \
				{ \
					/* allocate the singleton and register it */ \
					_Instance = new className; \
					SIPBASE::ISipContext::getInstance().setSingletonPointer(#className, _Instance, free_##className); \
				} \
				else \
				{ \
					_Instance = reinterpret_cast<className*>(ptr); \
				} \
			} \
			return *_Instance; \
		} \
	private:

	/** The same as above, but generate a getInstance method that
	 *	return a pointer instead of a reference
	 */
#define SIPBASE_SAFE_SINGLETON_DECL_PTR(className) \
	private:\
		/* declare private constructors*/ \
		/*className () {}*/\
		/*className (const className &) {}*/\
		/* the local static pointer to the singleton instance */ \
		static className *_Instance; \
	public:\
	static	void	free_##className(void * _cl) { className *Ins = (className *)_cl;	delete Ins; } \
		static className *getInstance() \
		{ \
			if (_Instance == NULL) \
			{ \
				/* the Sip context MUST be initialised */ \
				sipassertex(SIPBASE::ISipContext::isContextInitialised(), ("You are trying to access a safe singleton without having initialized a SIP context. The simplest correction is to add 'SIPBASE::CApplicationContext myApplicationContext;' at the very begining of your application.")); \
				void *ptr = SIPBASE::ISipContext::getInstance().getSingletonPointer(#className); \
				if (ptr == NULL) \
				{ \
					/* allocate the singleton and register it */ \
					_Instance = new className; \
					SIPBASE::ISipContext::getInstance().setSingletonPointer(#className, _Instance, free_##className); \
				} \
				else \
				{ \
					_Instance = reinterpret_cast<className*>(ptr); \
				} \
			} \
			return _Instance; \
		} \
	private:

	 /** This macro is the complement of the previous one.
	 *	It must be put in a cpp file to implement the static 
	 *	property of the singleton.
	 */
#define SIPBASE_SAFE_SINGLETON_IMPL(className) className *className::_Instance = NULL;

/// Function type for library entry point
typedef void (*TInitLibraryFunc)(void *appContextPtr, bool log);

/** An helper macro to build the dll entry point easily.
 */
#define SIPBASE_LIBRARY_ENTRY															\
	void libraryEntryImp(void *appContextPtr, bool log)									\
	{																					\
		if (!SIPBASE::ISipContext::isContextInitialised())								\
		{																				\
			if ( appContextPtr )														\
				new SIPBASE::CLibraryContext(*(SIPBASE::ISipContext*)appContextPtr);	\
			else																		\
				new SIPBASE::CApplicationContext();										\
		}																				\
	}																					\
	extern "C"																			\
	{																					\
		SIP_LIB_EXPORT SIPBASE::TInitLibraryFunc libraryEntry = libraryEntryImp;		\
	}																					\


class CLibrary;
/// helper function to init newly loaded sip library
void initSipLibrary(CLibrary &lib, bool log);


} // namespace SIPBASE


#endif //__APP_CONTEXT_H__
