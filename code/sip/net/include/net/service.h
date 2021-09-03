/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SERVICE_H__
#define __SERVICE_H__

//
// Includes
//

#include "misc/types_sip.h"

#if defined(SIP_OS_WINDOWS) && defined(_WINDOWS)
#	include <windows.h>
#	undef min
#	undef max
#endif

#include "misc/config_file.h"
#include "misc/entity_id.h"
#include "misc/variable.h"
#include "misc/command.h"
#include "misc/entity_id.h"
#include "misc/cpu_time_stat.h"
#include "misc/sstring.h"

#include "unified_network.h"

#include <string>
#include <vector>

namespace SIPBASE
{
	class CWindowDisplayer;
}

namespace SIPNET
{

class CCallbackServer;
class IServiceUpdatable;


//
// Macros
//


/**
 * The goal of this macro is to simplify the service creation, it creates the main body function.
 *
 * If you don't want to give a callback array, just put EmptyCallbackArray in the last argument
 *
 * Example:
 *\code
	// Create the Test Service class
	class CTestService : public IService
	{
	public:
		void init () { sipinfo("init()"); }
		bool update () { sipinfo ("update();"); return true; }
		void release () { sipinfo("release()"); }
	};
	// Create the main() function that create a test service instance and execute it.
	// "TS" is the short service name and "test_service" is the long one.
	// The name of the config file is based on the long name!
	// EmptyCallbackArray means that you don't provide right now the callback
	// the last 2 path are where is the config file is (directory) and where to log info (directory)
	SIPNET_SERVICE_MAIN(CTestService, "TS", "test_service", "", 0, false, EmptyCallbackArray, "", "");
 *\endcode
 *
 * If you want the port to not be auto-assigned by the naming service, set the port to a number different than 0.
 *
 * Args used by service are always in lower case:
 *
 * -A followed by the path where to execute the service (it uses chdir())
 * -B followed by the IP address where the naming service is
 * -C followed by the path where we can find the config file
 * -D followed by the client listening address of the frontend for the login system (FS only)
 * -I to start the service iconified
 * -L followed by the directory where we have to log
 * -N followed by the alias name (used by the admin system)
 * -P followed by the listen port (ListenAddress)
 * -Q to make the service quit immediately after the first update
 * -S followed by the shard Id (sint32) (WS only)
 * -T followed by the IP address where the login service is (WS only)
 * -W followed by the path where to save all shard data (SaveFilesDirectory)
 * -Z[u] to just init the config file then return (used for test), use Zu to not release the service
 * 
 *
 */


#if defined(SIP_OS_WINDOWS) && defined(_WINDOWS)
#define SIPNET_SERVICE_MAIN(__ServiceClassName, __ServiceShortName, __ServiceLongName, __ServicePortConfig, __ServiceDefaultPort, __LongNameMutex, __ServiceCallbackArray, __ConfigDir, __LogDir) \
 \
int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) \
{ \
	HANDLE hMutex = 0; \
	if ( __LongNameMutex ) \
		hMutex = ::CreateMutexA(NULL, TRUE, __ServiceLongName); \
	else \
	{ \
		std::string mutexName = ::GetCommandLineA(); \
		std::replace( mutexName.begin(), mutexName.end(), '\\', '_'); \
		std::string::iterator newend = std::remove( mutexName.begin(), mutexName.end(), '\"' ); \
		mutexName.erase(newend, mutexName.end()); \
		mutexName = SIPBASE::toLower(mutexName); \
		mutexName = SIPBASE::trim(mutexName); \
		hMutex = ::CreateMutexA(NULL, TRUE, mutexName.c_str()); \
	} \
	if ( hMutex == 0 ) \
		return 0; \
	if ( GetLastError() == ERROR_ALREADY_EXISTS ) \
	{ \
		::CloseHandle(hMutex); \
		return 0; \
	} \
	\
	SIPBASE::CApplicationContext	serviceContext; \
	__ServiceClassName *scn = new __ServiceClassName; \
	scn->setArgs (lpCmdLine); \
	createDebug (NULL, false); \
	scn->setCallbackArray (__ServiceCallbackArray, sizeof(__ServiceCallbackArray)/sizeof(__ServiceCallbackArray[0])); \
    sint retval = scn->main (__ServiceShortName, __ServiceLongName, __ServicePortConfig, __ServiceDefaultPort, __ConfigDir, __LogDir, __DATE__" "__TIME__); \
	delete scn; \
	\
	::CloseHandle(hMutex); \
	\
	return retval; \
}

#else
#define SIPNET_SERVICE_MAIN(__ServiceClassName, __ServiceShortName, __ServiceLongName, __ServicePortConfig, __ServiceDefaultPort, __Mutex, __ServiceCallbackArray, __ConfigDir, __LogDir) \
 \
int main(int argc, const char **argv) \
{ \
	SIPBASE::CApplicationContext serviceContext; \
	__ServiceClassName *scn = new __ServiceClassName; \
	scn->setArgs (argc, argv); \
	createDebug (NULL, !scn->haveLongArg("nolog")); \
	scn->setCallbackArray (__ServiceCallbackArray, sizeof(__ServiceCallbackArray)/sizeof(__ServiceCallbackArray[0])); \
	std::string date(__DATE__), time(__TIME__);	\
	sint retval = scn->main (__ServiceShortName, __ServiceLongName, __ServicePortConfig, __ServiceDefaultPort, __ConfigDir, __LogDir, (date + " " + time).c_str()); \
	delete scn; \
	return retval; \
}

#define SIPNET_SERVICE_MAIN_WIDE(__ServiceClassName, __ServiceShortName, __ServiceLongName, __ServicePort, __ServiceCallbackArray, __ConfigDir, __LogDir) \
 \
int main(int argc, const char **argv) \
{ \
	SIPBASE::CApplicationContext serviceContext; \
	__ServiceClassName *scn = new __ServiceClassName; \
	scn->setArgs (argc, argv); \
	createDebugW(NULL, !scn->haveLongArg("nolog"));\
	scn->setCallbackArray (__ServiceCallbackArray, sizeof(__ServiceCallbackArray)/sizeof(__ServiceCallbackArray[0])); \
	sint retval = scn->main (__ServiceShortName, __ServiceLongName, __ServicePort, __ConfigDir, __LogDir, __DATE__" "__TIME__); \
	delete scn; \
	return retval; \
}

#endif

#define DEFAULT_SHARD_ID 0xFFFFFFFF


//
// Typedefs
//

//typedef uint16 TServiceId;

/// Callback where you can return true for direct clearance, or false for later clearance.
typedef bool (*TRequestClosureClearanceCallback) ();

//
// Variables provided to application and unused in the SIP library itself.
//

extern TUnifiedCallbackItem EmptyCallbackArray[1];

//
// Classes
//

/**
 * Base class for all network services.
 * You must inherite from this class to create your own service. You must not
 * create ctor and dtor but implement init() and release() methods.
 * You have to create a global callback array called CallbackArray.
 *
 * \ref service_howto
 *
 * Temporary command line arguments :
 * \li -n<AliasName>
 *
 * \date 2000
 */
class IService
{
public:

	/// \name User overload methods. These methods can be overload by the user do handle init, update and release operation.
	// @{

	/** Called before the displayer is created, no displayer or network connection are built.
	    Use this callback to check some args and perform some command line based stuff */
	virtual void			commandStart () {}

	virtual void			preInit() {}
	/// Initializes the service (must be called before the first call to update())
	virtual void			init () {}

	/// This function is called every "frame" (you must call init() before). It returns false if the service is stopped.
	virtual bool			update () { return true; }

	/// Finalization. Release the service. For example, this function frees all allocations made in the init() function.
	virtual void			release () {}

	//@}


	/// \name get methods. These methods provide a way to read internal service variables.
	// @{

	/// Returns the instance of the service to access to methods/variables class
	static IService			*getInstance ();

	/// Returns true if the service singleton has been initialized
	static bool				isServiceInitialized() { return _Instance != NULL; }

	/// Returns the current service short name (ie: TS)
	const std::string		&getServiceShortName () const { return _ShortName; };

	/// Returns the current service long name (ie: test_serivce)
	const std::string		&getServiceLongName () const { return _LongName; };

	/// Returns the current service alias name setted by AES
	const std::string		&getServiceAliasName () const { return _AliasName; };

	/// Returns the current service unified name that is   alias/short-id or short-id if alias is empty
	std::string				getServiceUnifiedName () const;

	/// Returns the service identifier
	TServiceId				getServiceId () const { return _SId; }

	/// Return the host name of the host machine
	const std::string		&getHostName() const		{ return _HostName; }

	/// Returns the status
	sint					getExitStatus () const { return _ExitStatus; }

	/// Returns the date of launch of the service. Unit: see CTime::getSecondsSince1970()
	uint32					getLaunchingDate () const;

	/// Return true if this service don't use the NS (naming service)
	bool					getDontUseNS() const { return _DontUseNS; };

	/// Return true if this service don't use the AES (admin executor service)
	bool					getDontUseAES() const { return _DontUseAES; };

	/// Returns arguments of the program pass from the user to the program using parameters (ie: "myprog param1 param2")
	const SIPBASE::CVectorSStringA	&getArgs () const { return _Args; }

	/// Returns true if the argument if present in the command line (ie: haveArg('p') will return true if -p is in the command line)
	bool					haveArg (char argName) const;

	/// Get ConfigDirectory
	std::string				getConfigDirectory() const { return ConfigDirectory.c_str(); };

	/** Returns the parameter linked to an option
	 * getArg('p') will return toto if -ptoto is in the command line
	 * getArg('p') will return C:\Documents and Settings\toto.tmp if -p"C:\Documents and Settings\toto.tmp" is in the command line
	 * It'll thrown an Exception if the argName is not found
	 */
	std::string				getArg (char argName) const;

	/// return true if named long arg is present on the commandline
	/// eg haveLongArg("toto") returns true if "--toto" or "--toto=xxx" can be found on commandline
	bool					haveLongArg (const char* argName) const;

	/// returns the value associated with the given named argument
	/// both "--toto=xxx" and "--toto xxx" are acceptable
	/// quotes round arguments are stripped
	std::string				getLongArg (const char* argName) const;

	/// Returns an uniq id for an entities on this service.
	/*uint64				getEntityId (uint8 type)
	{
		return SIPBASE::CEntityId::getNewEntityId( type ).getRawId();
	}*/
	
	/// Returns the recording state (don't needed if you use layer5)
	CCallbackNetBase::TRecordingState	getRecordingState() const { return _RecordingState; }


	//@}


	/// \name set methods. These methods provide a way to modify internal service variables.
	// @{

	/** Sets the status of the service, this status is return to the application. EXIT_SUCCESS is the default status
	 * You can set it to EXIT_FAILURE or any value you want. It's useful when you use the service in a script and you
	 * want to know the return value of the application to do the appropriate things.
	 */
	void					setExitStatus (sint exitStatus) { _ExitStatus = exitStatus; }

	/** Call this function if you want the service quits next loop. The code will be returned outside of the application.
	 * \warning If you set the code to 0, it ll NOT exit the service */
	void					exit (sint code = 0x10);

	/** Selects timeout value in seconds for each update. You are absolutely certain that your update()
	 * function will not be called before this amount of time you set.
	 * If you set the update timeout value higher than 0, all messages in queues will be process until the time greater than the timeout user update().
	 * If you set the update timeout value to 0, all messages in queues will be process one time before calling the user update().
	 *
	 * The default value is 100 (100ms) or the value found in the config file (UpdateTimeout)
	 */
	void					setUpdateTimeout (SIPBASE::TTime timeout) { /*if (timeout>1.0) siperror ("IServer::setUpdateTimeout is now a double in SECOND and not ms");*/ _UniNetUpdateTime = timeout; } 

	//@}

	//@{
	//@name Service status management methods
	/// Push a new status on the status stack.
	void					setCurrentStatus(const std::string &status);
	/// Remove a status from the status stack. If this status is at top of stack, the next status become the current status
	void					clearCurrentStatus(const std::string &status);
	/// Add a tag in the status string
	void					addStatusTag(const std::string &statusTag);
	/// Remove a tag from the status string
	void					removeStatusTag(const std::string &statusTag);
	/// Get the current status with attached tags
	std::string				getFullStatus() const;
	//@}

	/// \name variables. These variables can be read/modified by the user.
	// @{

	SIPBASE::CConfigFile	ConfigFile;

	// use to display result of command (on file and windows displayer) **without** filter
	SIPBASE::CLog CommandLog;

	//@}


	/// \name private methods. These methods are used by internal system but can't be put in private, don't use them.
	// @{

	/// This main is called by the macro (service5 says if we have to use layer5 or not)
	sint main (const char *serviceShortName, const char *serviceLongName, const char *servicePortConfig, uint16 servicePort, const char *configDir, const char *logDir, const char *compilationDate);

	/// Sets the command line and init _Args variable. You must call this before calling main()
	void setArgs (int argc, const char **argv);

	/// Sets the command line and init _Args variable. You must call this before calling main()
	void setArgs (const char *args);

	/// Sets the default callback array given from the macro
	void setCallbackArray (TUnifiedCallbackItem *array, uint nbelem) { _CallbackArray = array; _CallbackArraySize = nbelem; }

	TREGPORTARY		getRegPortAry() { return _RegPortAry; };

	void setRegPortAry(TRegPortItem * arry, uint8 size);


	/// Require to reset the hierarchical timer
	void requireResetMeasures();

	/// Ctor. You must not inherit ctor but overload init() function
	IService ();

	/// Dtor. You must not inherit dtor but overload release() function
	virtual ~IService ();

	//@}

	//@{
	/// \name Updatable are object that require an update at each service loop
	/// Register an updatable interface
	void registerUpdatable(IServiceUpdatable *updatable);
	/// Unregister an updatable interface
	void unregisterUpdatable(IServiceUpdatable *updatable);
	//@}

	/// The window displayer instance
	SIPBASE::CWindowDisplayer			*WindowDisplayer;

	/// Directory where to store files that the services will write but are the same for all shard instance (for example: packet_sheets)
	/// Use .toString() to access to the value
	SIPBASE::CVariable<std::string>		WriteFilesDirectory;
	
	/// Directory where to store files that the services will write during the exploitation of the game (for example: player backup, string cache)
	/// Use .toString() to access to the value
	SIPBASE::CVariable<std::string>		SaveFilesDirectory;
	
	/// If true (default), the provided SaveFilesDirectory will be converted to a full path (ex: "saves" -> "/home/dir/saves")
	SIPBASE::CVariable<bool>				ConvertSavesFilesDirectoryToFullPath;

	/** You can provide a callback interface (only one) that will be called if any of the directory variables
	 * (WriteFilesDirectory, SaveFilesDirectory, ConfigDirectory, LogDirectory, RunningDirectory) is changed
	 * (also called for the first setting read from the .cfg file). Default is NULL.
	 */
	void								setDirectoryChangeCallback( SIPBASE::IVariableChangedCallback *cbi ) { _DirectoryChangedCBI = cbi; }

	void								setVersion (const std::string &version) { Version = version; }

	uint16								getPort() { return ListeningPort; }

	// Warning: can take a moment to be received from the WS. The default value (when not received yet) is DEFAULT_SHARD_ID.
	uint32								getShardId() const { return _ShardId; }
	
	const SIPBASE::CCPUTimeStat&			getCPUUsageStats() const	{ return _CPUUsageStats; }

	/**
	 * If your service needs a delay when it is asked to quit, provide a callback here (optional).
	 * Then, when the service will be asked to quit, this callback will be called. Then you can
	 * either return true to allow immediate closure, or false to delay the closure. The closure
	 * will then happen after you call clearForClosure().
	 * 
	 * If you don't provide a callback here, or if you call with NULL, the service will exit
	 * immediately when asked to quit.
	 */
	void								setClosureClearanceCallback( TRequestClosureClearanceCallback cb ) { _RequestClosureClearanceCallback = cb; }

	/**
	 * If using clearance for closure (see setClosureClearanceCallback()), you can call this method
	 * to allow the service to quit. If calling it while your callback has not be called yet, the
	 * callback will be bypassed and the service will quit immediately when asked to quit. If calling it
	 * after you callback was called (and you returned false), the service will quit shortly.
	 */
	void								clearForClosure() { _ClosureClearanceStatus = CCClearedForClosure; }

	/** Set the shard id (by the user code), when known before IService receives it by the WS).
	 * If a non-default value is already set and different than shardId => siperror.
	 * If later IService receives a different value from the WS => siperror.
	 */
	void								anticipateShardId( uint32 shardId );

private:

	/// \name methods. These methods are used by internal system.
	// @{

	/// Changes the recording state (use if you know what you are doing)
	void								setRecordingState( CCallbackNetBase::TRecordingState rec ) { _RecordingState = rec; }

	/** Set the shard id (received from the WS). See also anticipateShardId().
	 */
	void								setShardId( uint32 shardId );

	//@}

	/// \name variables. These variables are used by the internal system.
	// @{

	/// Array of arguments pass from the command line
	SIPBASE::CVectorSStringA				_Args;

	/// Host name of the host machine that run the service
	std::string							_HostName;

	/// Recording state
	CCallbackNetBase::TRecordingState	_RecordingState;

	/// Current service name sets by the actual service when declaring SIPNET_SERVICE_MAIN
	std::string							_ShortName;		// ie: "NS"
	std::string							_LongName;		// ie: "naming_service"
	//std::string							_ServiceName;		// ie: "naming_service"
	std::string							_AliasName;		// this name is initialized by the admin executor service via the args

	/// Instance of this service (singleton)
	static IService						*_Instance;

	/// Select timeout value in milliseconds between to call of user update()
	SIPBASE::TTime						_UniNetUpdateTime;

	/// the service id of this sevice
	TServiceId							_SId;

	/// the exit status of this service (the status is returned by the service at the release time)
	sint								_ExitStatus;

	/// true if the service initialisation is passed
	bool								_Initialized;

	/// The directory where the configfile is
	SIPBASE::CVariable<std::string>		ConfigDirectory;
	
	/// The directory where the logfiles are
	SIPBASE::CVariable<std::string>		LogDirectory;
	
	/// The directory where the service is running
	SIPBASE::CVariable<std::string>		RunningDirectory;

	SIPBASE::CVariable<std::string>		Version;

	TUnifiedCallbackItem				*_CallbackArray;
	uint								_CallbackArraySize;

	TREGPORTARY							_RegPortAry;

	/// Require to reset the hierarchical timer
	bool								_ResetMeasures;

	/// Shard Id
	uint32								_ShardId;

	/// CPU usage stats
	SIPBASE::CCPUTimeStat				_CPUUsageStats;

	/// Registered updatable interface
	std::set<IServiceUpdatable*>		_Updatables;

	//@{
	//@name Service running status management
	/// The status stack is used to display the most recent set status.
	std::vector<std::string>			_ServiceStatusStack;
	/// The status tags. All added tags are displayed.
	std::set<std::string>				_ServiveStatusTags;
	//@}

	enum TClosureClearanceStatus { CCMustRequestClearance, CCWaitingForClearance, CCClearedForClosure, CCCallbackThenClose=256 };

	/// Closure clearance state (either CCMustRequestClearance, CCWaitingForClearance, CCClearedForClosure or CCCallbackThenClose + any other as a backup value)
	uint								_ClosureClearanceStatus;

	/// Closure clearance callback (NULL if no closure clearance required)
	TRequestClosureClearanceCallback	_RequestClosureClearanceCallback;

	/// Directory changed callback
	SIPBASE::IVariableChangedCallback*	_DirectoryChangedCBI;

	friend void serviceGetView (uint32 rid, const std::string &rawvarpath, std::vector<std::string> &vara, std::vector<std::string> &vala);
	friend void cbAESConnection (const std::string &serviceName, TServiceId sid, void *arg);
	friend struct sip_serviceInfoClass;
	friend struct sip_getWinDisplayerInfoClass;
	SIPBASE_CATEGORISED_COMMAND_FRIEND(sip,downloadFileFromFTP);

	friend void cbDirectoryChanged (SIPBASE::IVariable &var);
	friend void cbReceiveShardId (SIPNET::CMessage& msgin, const std::string &serviceName, TServiceId serviceId);

	SIPBASE_CATEGORISED_DYNVARIABLE_FRIEND(sip, State);
protected:
	/// true if the service don't use the naming service
	bool								_DontUseNS;
	/// true if the service don't use the admin executor service
	bool								_DontUseAES;
	/// Listening port of this service
	SIPBASE::CVariable<uint16>			ListeningPort;
};

/** Get value from config file
	In this macro, third parameter "ValueType" must be function's name defined in "config_file.h".
	For example, It may be "asInt", "asString", and so on. 
*/ 
#define	GET_FROM_SERVICECONFIG(ConfigName, DefaultValue, ValueType)						\
	IService::getInstance()->ConfigFile.exists(ConfigName) ?							\
	IService::getInstance()->ConfigFile.getVar(ConfigName).ValueType() : DefaultValue	\

// services stat
extern SIPBASE::CVariable<sint32> UserUpdateSpeedLoop;
extern SIPBASE::CVariable<sint32> UniNetSpeedLoop;

// Timeout for UserUpdate
extern SIPBASE::CVariable<sint32> UserUpdateTime;
extern SIPBASE::CVariable<sint32> MinUserUpdateTime;

// Timeout for UnifiedNetwork
extern SIPBASE::CVariable<sint32> UniNetUpdateTime;

/** Interface class for object that need an update call during each service loop.
*/
class IServiceUpdatable
{
public:
	IServiceUpdatable()
	{
		if (IService::isServiceInitialized())
		{
			IService::getInstance()->registerUpdatable(this);
		}
		else
		{
			sipwarning("IServiceUpdatable : IService is not initialized, IUpdatable will not be called");
		}
	}
	virtual ~IServiceUpdatable()
	{
		if (IService::isServiceInitialized())
		{
			IService *service = IService::getInstance();

			service->unregisterUpdatable(this);
		}
	}

	/// implemente this virtual in you derived class
	virtual void serviceLoopUpdate() =0;
};


inline IService *IService::getInstance()
{
	if (_Instance == NULL) 
	{ 
		/* the sip context MUST be initialised */ 
		sipassertex(SIPBASE::ISipContext::isContextInitialised(), ("You are trying to access a safe singleton without having initialized a SIP context. The simplest correction is to add 'SIPBASE::CApplicationContext myApplicationContext;' at the very begining of your application.")); 
		// try to retrieve the safe singleton pointer
		_Instance = reinterpret_cast<IService*>(SIPBASE::ISipContext::getInstance().getSingletonPointer("IService"));
	} 
	return _Instance; 
}



}; // SIPNET

#endif // __SERVICE_H__

/* End of service.h */
