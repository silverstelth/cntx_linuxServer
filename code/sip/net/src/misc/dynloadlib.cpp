/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/dynloadlib.h"
#include "misc/path.h"

using namespace std;

namespace SIPBASE
{

SIP_LIB_HANDLE sipLoadLibrary(const std::string &libName)
{
#if defined SIP_OS_WINDOWS
	return LoadLibraryA(libName.c_str());
#elif defined (SIP_OS_UNIX)
	return dlopen(libName.c_str(), RTLD_NOW);
#else
#error "You must define sipLoadLibrary for your platform"
#endif
}

SIP_LIB_HANDLE sipLoadLibraryW(const std::basic_string<ucchar> &libName)
{
	GET_WFILENAME(wLibName, libName.c_str());
#if defined SIP_OS_WINDOWS
	return LoadLibraryW(wLibName);
#elif defined (SIP_OS_UNIX)
	return dlopen(wLibName, RTLD_NOW);
#else
#error "You must define sipLoadLibrary for your platform"
#endif
}

bool sipFreeLibrary(SIP_LIB_HANDLE libHandle)
{
#if defined SIP_OS_WINDOWS
	return FreeLibrary(libHandle) > 0;
#elif defined (SIP_OS_UNIX)
	return dlclose(libHandle) == 0;
#else
#error "You must define sipFreeLibrary for your platform"
#endif
}

void *sipGetSymbolAddress(SIP_LIB_HANDLE libHandle, const std::string &procName)
{
#if defined (SIP_OS_WINDOWS)
	return GetProcAddress(libHandle, procName.c_str());
#elif defined (SIP_OS_UNIX)
	return dlsym(libHandle, procName.c_str());
#else
#error "You must define nlGetProcAddress for your platform"
#endif
}

// Again some OS specifics stuff
#if defined (SIP_OS_WINDOWS)
  const string	SIP_LIB_PREFIX;	// empty
  const string	SIP_LIB_EXT(".dll");
#elif defined (SIP_OS_UNIX)
  const string	SIP_LIB_PREFIX("lib");
  const string	SIP_LIB_EXT(".so");
#else
#error "You must define the default dynamic lib extention"
#endif

// Compilation mode specific suffixes
#if defined (SIP_OS_WINDOWS)
 #ifdef SIP_DEBUG_INSTRUMENT
  const string	SIP_LIB_SUFFIX("_di");
 #elif defined (SIP_DEBUG_FAST)
   const string	SIP_LIB_SUFFIX("_df");
 #elif defined (SIP_DEBUG)
   const string	SIP_LIB_SUFFIX("_d");
 #elif defined (SIP_RELEASE_DEBUG)
   const string	SIP_LIB_SUFFIX("_rd");
 #elif defined (SIP_RELEASE)
   const string	SIP_LIB_SUFFIX("_r");
 #else
   #error "Unknown compilation mode, can't build suffix"
 #endif
#elif defined (SIP_OS_UNIX)
   const string	SIP_LIB_SUFFIX;	// empty
#else
 #error "Lib suffix not defined for your platform"
#endif

std::vector<std::string>	CLibrary::_LibPaths;


CLibrary::CLibrary (const CLibrary &other)
{
	// Nothing to do has it is forbidden.
	// Allowing copy require to manage reference count from CLibrary to the module resource.
	sipassert(false);
}

CLibrary &CLibrary::operator =(const CLibrary &other)
{
	// Nothing to do has it is forbidden.
	// Allowing assignment require to manage reference count from CLibrary to the module resource.
	sipassert(false);
	return *this;
}



std::string CLibrary::makeLibName(const std::string &baseName)
{
	return SIP_LIB_PREFIX+baseName+SIP_LIB_SUFFIX+SIP_LIB_EXT;
}

std::string CLibrary::cleanLibName(const std::string &decoratedName)
{
	// remove path and extension
	string ret = CFileA::getFilenameWithoutExtension(decoratedName);

	if (!SIP_LIB_PREFIX.empty())
	{
		// remove prefix
		if (ret.find(SIP_LIB_PREFIX) == 0)
			ret = ret.substr(SIP_LIB_PREFIX.size());
	}
	if (!SIP_LIB_SUFFIX.empty())
	{
		// remove suffix
		if (ret.substr(ret.size()-SIP_LIB_SUFFIX.size()) == SIP_LIB_SUFFIX)
			ret = ret.substr(0, ret.size() - SIP_LIB_SUFFIX.size());
	}

	return ret;
}

void CLibrary::addLibPaths(const std::vector<std::string> &paths)
{
	for (uint i=0; i<paths.size(); ++i)
	{
		string newPath = CPath::standardizePath(paths[i]);

		// only add new path
		if (std::find(_LibPaths.begin(), _LibPaths.end(), newPath) == _LibPaths.end())
		{
			_LibPaths.push_back(newPath);
		}
	}
}

void CLibrary::addLibPath(const std::string &path)
{
	string newPath = CPath::standardizePath(path);

	// only add new path
	if (std::find(_LibPaths.begin(), _LibPaths.end(), newPath) == _LibPaths.end())
	{
		_LibPaths.push_back(newPath);
	}
}
	
CLibrary::CLibrary()
:	_LibHandle(NULL),
	_Ownership(true),
	_PureSipLibrary(NULL)
{
}

CLibrary::CLibrary(SIP_LIB_HANDLE libHandle, bool ownership)
: _PureSipLibrary(NULL)
{
	_LibHandle = libHandle;
	_Ownership = ownership;
	_LibFileName = "unknown";
}

CLibrary::CLibrary(const std::string &libName, bool addSipDecoration, bool tryLibPath, bool ownership)
: _PureSipLibrary(NULL)
{
	loadLibrary(libName, addSipDecoration, tryLibPath, ownership);
	// Assert here !
	sipassert(_LibHandle != NULL);
}


CLibrary::~CLibrary()
{
	if (_LibHandle != NULL && _Ownership)
	{
		sipFreeLibrary(_LibHandle);
	}
}

bool CLibrary::loadLibrary(const std::string &libName, bool addSipDecoration, bool tryLibPath, bool ownership)
{
	_Ownership = ownership;
	string libPath = libName;

	if (addSipDecoration)
		libPath = makeLibName(libPath);

	if (tryLibPath)
	{
		// remove any directory spec
		string filename = CFileA::getFilename(libPath);

		for (uint i=0; i<_LibPaths.size(); ++i)
		{
			string pathname = _LibPaths[i]+filename;
			if (CFileA::isExists(pathname))
			{
				// we found it, replace libPath
				libPath = pathname;
				break;
			}
		}
	}

	sipdebug("Loading dynamic library '%s'", libPath.c_str());
	// load the lib now
	_LibHandle = sipLoadLibrary(libPath);
	_LibFileName = libPath;
	// MTR: some new error handling. Just logs if it couldn't load the handle.
	if(_LibHandle == NULL) {
		const char *errormsg="Verify DLL existence.";
#ifdef SIP_OS_UNIX
		errormsg=dlerror();
#endif
		sipwarning("Loading library %s failed: %s", libPath.c_str(), errormsg);
	}
	else
	{
		// check for 'pure' sip library
		void *entryPoint = getSymbolAddress(SIP_MACRO_TO_STR(SIPBASE_PURE_LIB_ENTRY_POINT));
		if (entryPoint != NULL)
		{
			// rebuild the interface pointer
			_PureSipLibrary = *(reinterpret_cast<ISipLibrary**>(entryPoint));
			// call the private initialisation method.
			_PureSipLibrary->_onLibraryLoaded(ISipContext::getInstance());
		}
	}

	return _LibHandle != NULL;
}

void CLibrary::freeLibrary()
{
	if (_LibHandle)
	{
		sipassert(_Ownership);

		if (_PureSipLibrary)
		{
			// call the private finalisation method.
			_PureSipLibrary->_onLibraryUnloaded();
		}

		sipdebug("Freeing dynamic library '%s'", _LibFileName.c_str());
		sipFreeLibrary(_LibHandle);

		_PureSipLibrary = NULL;
		_LibHandle = NULL;
		_Ownership = false;
		_LibFileName = "";
	}
}

void *CLibrary::getSymbolAddress(const std::string &symbolName)
{
	sipassert(_LibHandle != NULL);

	return sipGetSymbolAddress(_LibHandle, symbolName);
}

bool CLibrary::isLibraryLoaded()
{
	return _LibHandle != NULL;
}

bool CLibrary::isLibraryPure()
{
	return _LibHandle != NULL && _PureSipLibrary != NULL;
}

ISipLibrary *CLibrary::getSipLibraryInterface()
{
	if (!isLibraryPure())
		return NULL;

	return _PureSipLibrary;
}


ISipLibrary::~ISipLibrary()
{
	// cleanup ram
	if (_LibContext != NULL)
		delete _LibContext;
}


void ISipLibrary::_onLibraryLoaded(ISipContext &sipContext)
{
	++_LoadingCounter;

	if (_LoadingCounter == 1)
	{
		// initialise Sip context
		sipassert(!SIPBASE::ISipContext::isContextInitialised());

		_LibContext = new CLibraryContext(sipContext);
	}
	else
	{
		sipassert(SIPBASE::ISipContext::isContextInitialised());
	}

	onLibraryLoaded(_LoadingCounter==1);
}

void ISipLibrary::_onLibraryUnloaded()
{
	sipassert(_LoadingCounter > 0);
	
	onLibraryUnloaded(_LoadingCounter == 1);

	--_LoadingCounter;
}

uint32	ISipLibrary::getLoadingCounter()
{
	return _LoadingCounter;
}



}	// namespace SIPBASE
