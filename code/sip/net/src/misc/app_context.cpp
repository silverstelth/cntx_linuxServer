/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/app_context.h"
#include "misc/dynloadlib.h"
#include "misc/command.h"

namespace SIPBASE
{

static CAutoDestructorSingletone *AutoDestructorSingletone = NULL;
CAutoDestructorSingletone::~CAutoDestructorSingletone() 
{
	if (!_SingletonRegistry.empty())
	{
		void				*pSingleton;
		FUNC_FREESINGLETONE pFreeFunc;
		TSingletonRegistry::iterator it = _SingletonRegistry.begin(), last = _SingletonRegistry.end();
		for (; it != last; it ++)
		{
			pSingleton	= it->second.pInstance;
			pFreeFunc	= it->second.pFreeFunc;
			if (pSingleton && pFreeFunc)
			{
				pFreeFunc( pSingleton );
			}
			else if (pFreeFunc == NULL)
			{
				free(pSingleton);
				// delete pSingleton;
			}
		}
		_SingletonRegistry.clear();
	}
}
void	CAutoDestructorSingletone::setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func)
{
	sipassert(_SingletonRegistry.find(singletonName) == _SingletonRegistry.end());
	_SingletonRegistry[singletonName] = TSingletonRegistryProperty(ptr, free_func);
}

void	AutoDestructorSingletone_setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func)
{
	if (AutoDestructorSingletone == NULL)
	{
		AutoDestructorSingletone = new CAutoDestructorSingletone;
	}
	AutoDestructorSingletone->setSingletonPointer(singletonName, ptr, free_func);
}

class CDestructor_AutoDestructorSingletone
{
public:
	~CDestructor_AutoDestructorSingletone();
};

CDestructor_AutoDestructorSingletone::~CDestructor_AutoDestructorSingletone()
{
	delete AutoDestructorSingletone;
}

static CDestructor_AutoDestructorSingletone	Destructor_AutoDestructorSingletone;

//ISipContext *SipContext = NULL;
ISipContext *ISipContext::_SipContext = NULL;

ISipContext &ISipContext::getInstance()
{
	if (_SipContext == NULL)
	{
		_SipContext = new CApplicationContext;
	}

	return *_SipContext;
}

bool ISipContext::isContextInitialised()
{
	return _SipContext != NULL;
}

ISipContext::~ISipContext()
{
	_SipContext = NULL;
}

void	ISipContext::contextReady()
{
	// Register the SIP Context
	sipassert(_SipContext == NULL);
	_SipContext = this;

	// register any pending thinks

	// register local instance counter in the global instance counter manager
	CInstanceCounterLocalManager::getInstance().registerLocalManager();

	// register local commands into the global command registry (except it there is no command at all)
	if (ICommand::LocalCommands != NULL)
	{
		ICommand::TCommand::iterator first(ICommand::LocalCommands->begin()), last(ICommand::LocalCommands->end());
		for (; first != last; ++first)
		{
			CCommandRegistry::getInstance().registerCommand(first->second);
		}
	}
}

CApplicationContext::CApplicationContext()
{
	// init
	ErrorLog = NULL;
	WarningLog = NULL;
	InfoLog = NULL;
	DebugLog = NULL;
	AssertLog = NULL;

	DefaultStdDisplayer = NULL;
	DefaultFileDisplayer = NULL;
	DefaultMemDisplayer = NULL;
	DefaultMsgBoxDisplayer = NULL;

	DebugNeedAssert = false;
	NoAssert = false;
	AlreadyCreateSharedAmongThreads = false;

	contextReady();
}

CApplicationContext::~CApplicationContext()
{
	releaseContext();
}

void *CApplicationContext::getSingletonPointer(const std::string &singletonName)
{
	TSingletonRegistry::iterator it(_SingletonRegistry.find(singletonName));
	if (it != _SingletonRegistry.end())
		return it->second.pInstance;

//	sipwarning("Can't find singleton '%s'", singletonName.c_str());
	return NULL;
}

void CApplicationContext::setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func)
{
	sipassert(_SingletonRegistry.find(singletonName) == _SingletonRegistry.end());
	_SingletonRegistry[singletonName] = TSingletonRegistryProperty(ptr, free_func);
}

void CApplicationContext::releaseSingletonPointer(const std::string &singletonName, void *ptr)
{
	sipassert(_SingletonRegistry.find(singletonName) != _SingletonRegistry.end());
	sipassert(_SingletonRegistry.find(singletonName)->second.pInstance == ptr);
	_SingletonRegistry.erase(singletonName);
}


CLog *CApplicationContext::getErrorLog()
{
	return ErrorLog;
}

void CApplicationContext::setErrorLog(CLog *errorLog)
{
	ErrorLog = errorLog;
}

CLog *CApplicationContext::getWarningLog()
{
	return WarningLog;
}

void CApplicationContext::setWarningLog(CLog *warningLog)
{
	WarningLog = warningLog;
}

CLog *CApplicationContext::getInfoLog()
{
	return InfoLog;
}

void CApplicationContext::setInfoLog(CLog *infoLog)
{
	InfoLog = infoLog;
}

CLog *CApplicationContext::getDebugLog()
{
	return DebugLog;
}

void CApplicationContext::setDebugLog(CLog *debugLog)
{
	DebugLog = debugLog;
}

CLog *CApplicationContext::getAssertLog()
{
	return AssertLog;
}

void CApplicationContext::setAssertLog(CLog *assertLog)
{
	AssertLog = assertLog;
}

CStdDisplayer *CApplicationContext::getDefaultStdDisplayer()
{
	return DefaultStdDisplayer;
}

void CApplicationContext::setDefaultStdDisplayer(CStdDisplayer *stdDisplayer)
{
	DefaultStdDisplayer = stdDisplayer;
}

CFileDisplayer *CApplicationContext::getDefaultFileDisplayer()
{
	return DefaultFileDisplayer;
}

void CApplicationContext::setDefaultFileDisplayer(CFileDisplayer *fileDisplayer)
{
	DefaultFileDisplayer = fileDisplayer;
}

CMemDisplayer *CApplicationContext::getDefaultMemDisplayer()
{
	return DefaultMemDisplayer;
}

void CApplicationContext::setDefaultMemDisplayer(CMemDisplayer *memDisplayer)
{
	DefaultMemDisplayer = memDisplayer;
}

CMsgBoxDisplayer *CApplicationContext::getDefaultMsgBoxDisplayer()
{
	return DefaultMsgBoxDisplayer;
}

void CApplicationContext::setDefaultMsgBoxDisplayer(CMsgBoxDisplayer *msgBoxDisplayer)
{
	DefaultMsgBoxDisplayer = msgBoxDisplayer;
}

bool CApplicationContext::getDebugNeedAssert()
{
	return DebugNeedAssert;
}

void CApplicationContext::setDebugNeedAssert(bool needAssert)
{
	DebugNeedAssert = needAssert;
}

bool CApplicationContext::getNoAssert()
{
	return NoAssert;
}

void CApplicationContext::setNoAssert(bool noAssert)
{
	NoAssert = noAssert;
}

bool CApplicationContext::getAlreadyCreateSharedAmongThreads()
{
	return AlreadyCreateSharedAmongThreads;
}

void CApplicationContext::setAlreadyCreateSharedAmongThreads(bool b)
{
	AlreadyCreateSharedAmongThreads = b;
}

void CApplicationContext::releaseContext(void)
{
	if (ErrorLog != NULL)
	{
		delete ErrorLog;
		ErrorLog = NULL;
	}
	if (WarningLog != NULL)
	{
		delete WarningLog;
		WarningLog = NULL;
	}
	if (InfoLog != NULL)
	{
		delete InfoLog;
		InfoLog = NULL;
	}
	if (DebugLog != NULL)
	{
		delete DebugLog;
		DebugLog = NULL;
	}
	if (AssertLog != NULL)
	{
		delete AssertLog;
		AssertLog = NULL;
	}

	if (DefaultStdDisplayer != NULL)
	{
		delete DefaultStdDisplayer;
		DefaultStdDisplayer = NULL;
	}
	if (DefaultFileDisplayer != NULL)
	{
		delete DefaultFileDisplayer;
		DefaultFileDisplayer = NULL;
	}
	if (DefaultMemDisplayer != NULL)
	{
		delete DefaultMemDisplayer;
		DefaultMemDisplayer = NULL;
	}
	if (DefaultMsgBoxDisplayer != NULL)
	{
		delete DefaultMsgBoxDisplayer;
		DefaultMsgBoxDisplayer = NULL;
	}

	// unregister still undeleted local command into the global command registry
	if (ICommand::LocalCommands != NULL)
	{
		ICommand::TCommand::iterator first(ICommand::LocalCommands->begin()), last(ICommand::LocalCommands->end());
		for (; first != last; ++first)
		{
			ICommand *command = first->second;
			CCommandRegistry::getInstance().unregisterCommand(command);
		}	
	}

	CInstanceCounterLocalManager::releaseInstance();

	if (!_SingletonRegistry.empty())
	{
		void				*pSingleton;
		FUNC_FREESINGLETONE pFreeFunc;
		TSingletonRegistry::iterator it = _SingletonRegistry.begin(), last = _SingletonRegistry.end();
		for (; it != last; it ++)
		{
			pSingleton	= it->second.pInstance;
			pFreeFunc	= it->second.pFreeFunc;
			if (pSingleton && pFreeFunc)
			{
				pFreeFunc( pSingleton );
			}
			else if (pFreeFunc == NULL)
			{
				free(pSingleton);
				// delete pSingleton;
			}
		}
		_SingletonRegistry.clear();
	}
}

CLibraryContext::CLibraryContext(ISipContext &applicationContext)
: _ApplicationContext(applicationContext)
{
	contextReady();
}

CLibraryContext::~CLibraryContext()
{
	releaseContext();
}

void *CLibraryContext::getSingletonPointer(const std::string &singletonName)
{
//	sipassert(_ApplicationContext != NULL); 

	// just forward the call
	return _ApplicationContext.getSingletonPointer(singletonName);
}

void CLibraryContext::setSingletonPointer(const std::string &singletonName, void *ptr, FUNC_FREESINGLETONE free_func)
{
//	sipassert(_ApplicationContext != NULL); 

	// just forward the call
	_ApplicationContext.setSingletonPointer(singletonName, ptr, free_func);
}

void CLibraryContext::releaseSingletonPointer(const std::string &singletonName, void *ptr)
{
//	sipassert(_ApplicationContext != NULL); 

	// just forward the call
	_ApplicationContext.releaseSingletonPointer(singletonName, ptr);
}


CLog *CLibraryContext::getErrorLog()
{
	return _ApplicationContext.getErrorLog();
}

void CLibraryContext::setErrorLog(CLog *errorLog)
{
	_ApplicationContext.setErrorLog(errorLog);
}

CLog *CLibraryContext::getWarningLog()
{
	return _ApplicationContext.getWarningLog();
}

void CLibraryContext::setWarningLog(CLog *warningLog)
{
	_ApplicationContext.setWarningLog(warningLog);
}

CLog *CLibraryContext::getInfoLog()
{
	return _ApplicationContext.getInfoLog();
}

void CLibraryContext::setInfoLog(CLog *infoLog)
{
	_ApplicationContext.setInfoLog(infoLog);
}

CLog *CLibraryContext::getDebugLog()
{
	return _ApplicationContext.getDebugLog();
}

void CLibraryContext::setDebugLog(CLog *debugLog)
{
	_ApplicationContext.setDebugLog(debugLog);
}

CLog *CLibraryContext::getAssertLog()
{
	return _ApplicationContext.getAssertLog();
}

void CLibraryContext::setAssertLog(CLog *assertLog)
{
	_ApplicationContext.setAssertLog(assertLog);
}

CStdDisplayer *CLibraryContext::getDefaultStdDisplayer()
{
	return _ApplicationContext.getDefaultStdDisplayer();
}

void CLibraryContext::setDefaultStdDisplayer(CStdDisplayer *stdDisplayer)
{
	_ApplicationContext.setDefaultStdDisplayer(stdDisplayer);
}

CFileDisplayer *CLibraryContext::getDefaultFileDisplayer()
{
	return _ApplicationContext.getDefaultFileDisplayer();
}

void CLibraryContext::setDefaultFileDisplayer(CFileDisplayer *fileDisplayer)
{
	_ApplicationContext.setDefaultFileDisplayer(fileDisplayer);
}

CMemDisplayer *CLibraryContext::getDefaultMemDisplayer()
{
	return _ApplicationContext.getDefaultMemDisplayer();
}

void CLibraryContext::setDefaultMemDisplayer(CMemDisplayer *memDisplayer)
{
	_ApplicationContext.setDefaultMemDisplayer(memDisplayer);
}

CMsgBoxDisplayer *CLibraryContext::getDefaultMsgBoxDisplayer()
{
	return _ApplicationContext.getDefaultMsgBoxDisplayer();
}

void CLibraryContext::setDefaultMsgBoxDisplayer(CMsgBoxDisplayer *msgBoxDisplayer)
{
	_ApplicationContext.setDefaultMsgBoxDisplayer(msgBoxDisplayer);
}

bool CLibraryContext::getDebugNeedAssert()
{
	return _ApplicationContext.getDebugNeedAssert();
}

void CLibraryContext::setDebugNeedAssert(bool needAssert)
{
	_ApplicationContext.setDebugNeedAssert(needAssert);
}

bool CLibraryContext::getNoAssert()
{
	return _ApplicationContext.getNoAssert();
}

void CLibraryContext::setNoAssert(bool noAssert)
{
	_ApplicationContext.setNoAssert(noAssert);
}

bool CLibraryContext::getAlreadyCreateSharedAmongThreads()
{
	return _ApplicationContext.getAlreadyCreateSharedAmongThreads();
}

void CLibraryContext::setAlreadyCreateSharedAmongThreads(bool b)
{
	_ApplicationContext.setAlreadyCreateSharedAmongThreads(b);
}

void CLibraryContext::releaseContext(void)
{
	// KSR_ADD (COMMENT)
	//_ApplicationContext.releaseContext();
}

void initSipLibrary(SIPBASE::CLibrary &lib, bool log)
{
	sipassert(lib.isLibraryLoaded());

	TInitLibraryFunc *funptrptr = reinterpret_cast<TInitLibraryFunc*>(lib.getSymbolAddress("libraryEntry"));
	sipassert(funptrptr != NULL);

	TInitLibraryFunc funptr = *funptrptr;

	// call the initialisation function
	funptr(&SIPBASE::ISipContext::getInstance(), log);
}

} // namespace SIPBASE
