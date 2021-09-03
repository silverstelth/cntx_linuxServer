/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __NAMING_CLIENT_H__
#define __NAMING_CLIENT_H__

#include "misc/types_sip.h"

#include <string>

#include "misc/mutex.h"

#include "inet_address.h"
#include "callback_client.h"
#include "service.h"


namespace SIPNET {


//typedef uint16 TServiceId;

typedef void (*TBroadcastCallback)(const std::string &name, TServiceId sid, const std::vector<CInetAddress> &addr);

/**
 * Client side of Naming Service. Allows to register/unregister services, and to lookup for
 * a registered service.
 *
 * \warning the Naming Service can be down, it will reconnect when up but if other services try to register during the
 * NS is offline, it will cause bug
 *
 * \date 2001
 */
class CNamingClient
{
public:
	struct CServiceEntry
	{
		CServiceEntry (std::string n, TServiceId s, std::vector<CInetAddress> a) : Name(n), SId (s), Addr(a) { }

		// name of the service
		std::string					Name;
		// id of the service
		TServiceId					SId;
		// address to send to the service who wants to lookup this service (could have more than one)
		std::vector<CInetAddress>	Addr;
	};

public:

	/// Connect to the naming service.
	static void			connect( const CInetAddress& addr, CCallbackNetBase::TRecordingState rec, const std::vector<CInetAddress> &addresses );

	/// Return true if the connection to the Naming Service was done.
	static bool			connected () { return _Connection != NULL && _Connection->connected (); }

	/// Close the connection to the naming service.
	static void			disconnect ();

	/// Returns information about the naming connection (connected or not, which naming service and so on)
	static std::string	info ();

	/** Register a service within the naming service.
	 * Returns false if the registration failed.
	 */
	static bool			registerService (const std::string &name, const std::vector<CInetAddress> &addr, TServiceId &sid);

	/** Register a service within the naming service, using a specified service identifier.
	 * Returns false if the service identifier is unavailable i.e. the registration failed.
	 */
	static bool			registerServiceWithSId (const std::string &name, const std::vector<CInetAddress> &addr, TServiceId sid);

	/** If the NS is down and goes up, we have to send it again the registration. But in this case, the NS
	 * must not send a registration broadcast, so we have a special message
	 */
	static void			resendRegisteration (const std::string &name, const std::vector<CInetAddress> &addr, TServiceId sid);

	/// Unregister a service from the naming service, service identifier.
	static void			unregisterService (TServiceId sid);

	/// Unregister all services registered by this client. You don't have to
	static void			unregisterAllServices ();


	/** Requests the naming service to choose a port for the service
	 * \return An empty port number
	 */
	static uint16		queryServicePort ();


	/** Returns true and the address of the specified service if it is found, otherwise returns false
	 * \param name [in] Short name of the service to find
	 * \param addr [out] Address of the service
	 * \param validitytime [out] After this number of seconds are elapsed, another lookup will be necessary
	 * before sending a message to the service
	 * \return true if all worked fine
	 */
	static bool			lookup (const std::string &name, CInetAddress &addr);

	/// Same as lookup(const string&, CInetAddress&, uint16&)
	static bool			lookup (TServiceId sid, CInetAddress &addr);

	/** Tells the Naming Service the specified address does not respond for the specified service,
	 * and returns true and another address for the service if available, otherwise returns false
	 * \param name [in] Short name of the service to find
	 * \param addr [in/out] In: Address of the service that does not respond. Out: Alternative address
	 * \param validitytime [out] After this number of seconds are elapsed, another lookup will be necessary
	 * before sending a message to the service
	 * \return true if all worked fine.
	 */
	static bool			lookupAlternate (const std::string& name, CInetAddress& addr);


	/**
	 * Returns all services corresponding to the specified short name.
	 * Ex: lookupAll ("AS", addresses);
	 */
	static void			lookupAll (const std::string &name, std::vector<CInetAddress> &addresses);

	/**
	 * Returns all registered services.
	 */
	static const std::list<CServiceEntry>	&getRegisteredServices() { return RegisteredServices; }


	/** Obtains a socket connected to a service providing the service \e name.
	 * In case of failure to connect, the method informs the Naming Service and tries to get another service.
	 * \param name [in] Short name of the service to find and connected
	 * \param sock [out] The connected socket.
	 * \param validitytime [out] After this number of seconds are elapsed, another lookup will be necessary
	 * before sending a message to the service.
	 * \return false if the service was not found.
	 */
	static bool			lookupAndConnect (const std::string &name, CCallbackClient &sock);


	/// Call it evenly
	static void			update ();


	/// You can link a callback if you want to know when a new service is registered (NULL to disable callback)
	static void			setRegistrationBroadcastCallback (TBroadcastCallback cb);


	/// You can link a callback if you want to know when a new service is unregistered (NULL to disable callback)
	static void			setUnregistrationBroadcastCallback (TBroadcastCallback cb);

	static void displayRegisteredServices (SIPBASE::CLog *log = SIPBASE::DebugLog)
	{
		RegisteredServicesMutex.enter ();
		log->displayNL ("Display the %d registered services :", RegisteredServices.size());
		for (std::list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
		{
			log->displayNL (" > %s-%hu %d addr", (*it).Name.c_str(), (*it).SId.get(), (*it).Addr.size());
			for(uint i = 0; i < (*it).Addr.size(); i++)
				log->displayNL ("            '%s'", (*it).Addr[i].asString().c_str());
		}
		log->displayNL ("End of the list");
		RegisteredServicesMutex.leave ();
	}

	static	bool	isMasterService();

private:

	static CCallbackClient *_Connection;

	// The service Id of this service
	static TServiceId _MySId;
	
	/// Type of map of registered services
	typedef std::map<TServiceId, std::string> TRegServices;

	// this container contains the server that *this* service have registered (often, there's only one)
	static TRegServices _RegisteredServices;

	/// Constructor
	CNamingClient() {}

	/// Destructor
	~CNamingClient() {}

	static void doReceiveLookupAnswer (const std::string &name, std::vector<CInetAddress> &addrs);

	// This container contains the list of other registered services on the shard
	static std::list<CServiceEntry>	RegisteredServices;
	static SIPBASE::CMutex RegisteredServicesMutex;

	static void find (const std::string &name, std::vector<CInetAddress> &addrs)
	{
		RegisteredServicesMutex.enter ();
		for (std::list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
			if (name == (*it).Name)
				addrs.push_back ((*it).Addr[0]);
		RegisteredServicesMutex.leave ();
	}

	static void find (TServiceId sid, std::vector<CInetAddress> &addrs)
	{
		RegisteredServicesMutex.enter ();
		for (std::list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
			if (sid == (*it).SId)
				addrs.push_back ((*it).Addr[0]);
		RegisteredServicesMutex.leave ();
	}


	friend void cbRegisterBroadcast (CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase);
	friend void cbUnregisterBroadcast (CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase);


protected:
	/// \todo ace: debug feature that we should remove one day before releasing the game
	static uint	_ThreadId;
	static void	checkThreadId ();
};


} // SIPNET


#endif // __NAMING_CLIENT_H__

/* End of naming_client.h */
