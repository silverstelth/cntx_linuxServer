/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UNITIME_H__
#define __UNITIME_H__

#include "misc/types_sip.h"
#include "misc/time_sip.h"
#include "misc/debug.h"
#include "callback_net_base.h"

namespace SIPNET
{

class CInetAddress;
class CCallbackServer;
class CCallbackClient;

/**
 * This class provide a independant universal time system.
 * \date 2000
 *
 * THIS CLASS IS DEPRECATED, DON'T USE IT
 *
 */
class _CUniTime : public SIPBASE::CTime
{
public:

	/// Return the time in millisecond. This time is the same on all computers at the \b same moment.
	static SIPBASE::TTime	getUniTime ();

	/// Return the time in a string format to be display
	static const char		*getStringUniTime ();

	/// Return the time in a string format to be display
	static const char		*getStringUniTime (SIPBASE::TTime ut);


	/** You need to call this function before calling getUniTime or an assert will occured.
	 * This function will connect to the time service and synchronize your computer.
	 * This function assumes that all services run on server that are time synchronized with NTP for example.
	 * If addr is NULL, the function will connect to the Time Service via the Naming Service. In this case,
	 * the CNamingClient must be connected to a Naming Service.
	 * This function can be called *ONLY* by services that are inside of the shard.
	 * Don't use it for a client or a service outside of the shard.
	 */
	static void				syncUniTimeFromService (CCallbackNetBase::TRecordingState rec=CCallbackNetBase::Off, const CInetAddress *addr = NULL);

	/** Call this function in the init part of the front end service to enable time syncro between
	 * shard and clients.
	 */
	static void				installServer (CCallbackServer *server);

	/** Call this functions in the init part of the client side to synchronize between client and shard.
	 * client is the connection between the client and the front end. The connection must be established before
	 * calling this function.
	 */
	static void				syncUniTimeFromServer (CCallbackClient *client);

	/** \internal used by the time service to set the universal time the first time
	  */
	static void				setUniTime (SIPBASE::TTime uTime, SIPBASE::TTime lTime);
	/** \internal
	  */
	static void				setUniTime (SIPBASE::TTime uTime);

	/**
	 * Call this method before to prevent syncUniTimeFromService() from real synchronization:
	 * syncUniTimeFromService() will still communicate with the time service, as usual,
	 * but the local time will not be synchronized.
	 */
	static void				simulate() { sipstop; _Simulate = true; }

	static bool				Sync;				// true if the synchronization occured
private:

	static SIPBASE::TTime	_SyncUniTime;		// time in millisecond when the universal time received
	static SIPBASE::TTime	_SyncLocalTime;		// time in millisecond when the syncro with universal time occured

	// If true, do not synchronize
	static bool				_Simulate;
};

} // SIPNET

#endif // __UNITIME_H__

/* End of unitime.h */
