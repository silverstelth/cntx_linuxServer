/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __DYNLOADLIB_H__
#define __DYNLOADLIB_H__

#include "types_sip.h"
#include <string>
#include <vector>

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#undef max
#undef min
#else
#include <dlfcn.h>
#endif

namespace SIPBASE 
{

/// Define the os specific type for dynamic library module handler
#if defined (SIP_OS_WINDOWS)
typedef HMODULE		SIP_LIB_HANDLE;
#elif defined (SIP_OS_UNIX)
typedef void*		SIP_LIB_HANDLE;
#else
# error "You must define the module type on this platform"
#endif

#ifdef SIP_OS_WINDOWS
// Windows need explicit tag to export or import symbol form a code module
#define SIP_LIB_EXPORT	__declspec(dllexport)
#define SIP_LIB_IMPORT	__declspec(dllimport)
#else
// other systems don't bother with this kind of detail, they export any 'extern' sumbol (almost all functions)
#define SIP_LIB_EXPORT
#define SIP_LIB_IMPORT
#endif

/// Generic dynamic library loading function.
SIP_LIB_HANDLE	sipLoadLibrary (const std::string &libName);
SIP_LIB_HANDLE	sipLoadLibraryW(const std::basic_string<ucchar> &libName);
/// Generic dynamic library unloading function.
bool			sipFreeLibrary(SIP_LIB_HANDLE libHandle);
/// Generic dynamic library symbol address lookup function.
void			*sipGetSymbolAddress(SIP_LIB_HANDLE libHandle, const std::string &symbolName);


// Utility macro to export a module entry point as a C pointer to a C++ class or function
#define SIP_LIB_EXPORT_SYMBOL(symbolName, classOrFunctionName, instancePointer) \
extern "C"\
{\
	SIP_LIB_EXPORT classOrFunctionName	*symbolName = instancePointer;\
};

/*
 *
 * \date 2004
 */
class CLibrary
{
	/// Dynamic library handle
	SIP_LIB_HANDLE	_LibHandle;
	/// Loaded library name
	std::string		_LibFileName;

	/** When a module handle is assigned to the instance, this
	 *	flag state whether the CLibrary will free the library or not
	 *	when it is deleted.
	 */
	bool			_Ownership;
	/** This point to the 'pure' Sip library interface.
	 *	This mean that the library export an entry point called
	 *	'SIPLibraryEntry' that point to a ISipLibrary interface.
	 *	This interface is used by the CLibrary class to 
	 *	management efficiently the library and to give
	 *	library implementors some hooks on library management.
	 */
	class ISipLibrary		*_PureSipLibrary;

	/// Lib paths
	static std::vector<std::string>	_LibPaths;

	/// Private copy constructor, not authorized
	CLibrary (const CLibrary &other);

	// Private assignment operator
	CLibrary &operator =(const CLibrary &other);
	
public:
	CLibrary();
	/** Assign a existing module handler to a new dynamic library instance
	 *	Note that you cannot use 'pure Sip library' this way.
	 */
	CLibrary(SIP_LIB_HANDLE libHandle, bool ownership);
	/// Load the specified library and take ownership by default
	CLibrary(const std::string &libName, bool addSipDecoration, bool tryLibPath, bool ownership = true);
	/// Destructor, free the library if the object have ownership
	virtual ~CLibrary();

	/** Load the specified library.
	*	The method assert if a module is already assigned or loaded
	*	If addSipDecoration is true, the standard Sip suffix and library extention or prefix are 
	*	added to the lib name (with is just a base name).
	*	If tryLibPath is true, then the method will try to find the required 
	*	library in the added library files (in order of addition).
	*	Return true if the library load ok.
	*/
	bool loadLibrary(const std::string &libName, bool addSipDecoration, bool tryLibPath, bool ownership = true);

	/** Unload (free) the assigned/loaded library.
	*	The object must have ownership over the library or the call will assert.
	*	After this call, you can recall loadLibrary.
	*/
	void freeLibrary();

	/** Get the address a the specified procedure in the library
	*	Return NULL is the proc is not found,
	*	Assert if the library is not load or assigned.
	*/
	void *getSymbolAddress(const std::string &symbolName);

	/// Get the name of the loaded library
	std::string getLibFileName()
	{
		return _LibFileName;
	}

	/// Check weather a library is effectively loaded
	bool isLibraryLoaded();

	/** Check if the loaded library is a pure library
	 *	Return false if the library is not pure
	 *	or if the library is not loaded.
	 */
	bool isLibraryPure();

	/** Return the pointer on the pure Sip library interface.
	 *	Return NULL if isLibraryPure() is false.
	 */
	class ISipLibrary *getSipLibraryInterface();

	/** Build a SIP standard library name according to platform and compilation mode setting.
	 *	aka : adding decoration one base lib name.
	 *	e.g : 'mylib' become	'mylib_rd.dll' on Windows ReleaseDebug mode or
	 *							'libmylib.so' on unix system.
	 */
	static std::string makeLibName(const std::string &baseName);
	/** Remove SIP standard library name decoration according to platform and compilation mode setting.
	 *	e.g : 'mylib_rd.dll' on windows ReleaseDebug mode become 'mylib'
	 */
	static std::string cleanLibName(const std::string &decoratedName);
	/** Add a list of library path
	 *	@see loadLibrary for a discusion about lib path
	 */
	static void addLibPaths(const std::vector<std::string> &paths);
	/** Add a library path
	 *	@see loadLibrary for a discusion about lib path
	 */
	static void addLibPath(const std::string &path);

};

/** Interface class for 'pure Sip' library module.
 *	This interface is used to expose a standard
 *	entry point for dynamicly loaded library
 *	and to add some hooks for library implementor
 *	over library loading/unloading.
 */
class ISipLibrary
{
	friend class CLibrary;

	/// this counter keep tracks of how many time the library have been loaded in the same process.
	uint32						_LoadingCounter;
	/// The library context
	class CLibraryContext		*_LibContext;

	/** Called by CLibrary after a succesful loadLibrary 
	 *	Note : this methods MUST be virtual to ensure
	 *	that the code executed is the one from 
	 *	the loaded library.
	 */
	virtual void _onLibraryLoaded(class ISipContext &SipContext);
	/** Called by the CLibrary class before unloading the library.
	 *	Note : this methods MUST be virtual to ensure
	 *	that the code executed is the one from 
	 *	the loaded library.
	 */
	virtual void _onLibraryUnloaded();
public:

	ISipLibrary()
		: _LoadingCounter(0),
		_LibContext(NULL)
	{}

	virtual ~ISipLibrary();

	/// Return the loading counter value
	uint32	getLoadingCounter();

	/** Called after each loading of the library.
	 *	firstTime is true if this is the first time that
	 *	this library is loaded in the current process.
	 *	If the process load more than once the same
	 *	library, subsequent call to this method will
	 *	have firstTime false.
	 *	Implement this method to initialise think or whatever.
	 */
	virtual void onLibraryLoaded(bool firstTime) =0;
	/** Called after before each unloading of the library.
	 *	lastTime is true if the library will be effectively
	 *	unmapped from the process memory.
	 *	If the library have been loaded more than once, 
	 *	then lastTime is false, indicating that the 
	 *	library will still be in the process memory
	 *	space after the call.
	 */
	virtual void onLibraryUnloaded(bool lastTime) =0;
};

#define SIPBASE_PURE_LIB_ENTRY_POINT	SipLibraryEntry

#define SIPBASE_DECL_PURE_LIB(className)			\
	className	_PureLibraryEntryInstance;	\
	SIP_LIB_EXPORT_SYMBOL( SIPBASE_PURE_LIB_ENTRY_POINT, ISipLibrary, &_PureLibraryEntryInstance)

} // SIPBASE

#endif // __DYNLOADLIB_H__

