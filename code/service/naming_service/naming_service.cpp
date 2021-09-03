/** \file naming_service.cpp
 * Naming Service (NS)
 *
 * $Id: naming_service.cpp,v 1.21 2016/01/08 00:13:44 RyuSangChol Exp $
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef SIPNS_CONFIG
#define SIPNS_CONFIG ""
#endif // SIPNS_CONFIG

#ifndef SIPNS_LOGS
#define SIPNS_LOGS ""
#endif // SIPNS_LOGS

//
/// \todo ace: check that if there are 2 subnet with different service that services are well connected with well services for his subnet and not the other one
//

//
// Includes
//

#include "misc/types_sip.h"

#include <list>
#include <string>

#include "misc/debug.h"
#include "misc/command.h"
#include "misc/variable.h"
#include "misc/displayer.h"
#include "net/email.h"
#include "net/callback_server.h"
#include "net/service.h"
#include "net/module_manager.h"

#include "../Common/Common.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

struct CServiceEntry
{
	CServiceEntry (TSockId sock, const vector<CInetAddress> &a, const string &n, TServiceId s) : SockId(sock), Addr(a), Name(n), SId (s), WaitingUnregistration(false) { }

	TSockId						SockId;			// the connection between the service and the naming service
	vector<CInetAddress>		Addr;			// address to send to the service who wants to lookup this service
												// it s possible to have more than one addr, anyway, the naming service
												// will send good address depending of the sub net address of the service
	string						Name;			// name of the service
	TServiceId					SId;			// id of the service

	bool				WaitingUnregistration;			// true if this service is in unregistration process (wait other service ACK)
	TTime				WaitingUnregistrationTime;		// time of the beginning of the inregistration process
	list<TServiceId>	WaitingUnregistrationServices;	// list of service that we wait the answer
};



// Helper that emulates layer5's send()
//void sendToService( uint16 sid, CMessage& msgout );

// Helper that emulate layer5's getServiceName()
string getServiceName( TServiceId  sid );

// Helper that returns the first address of a service
CInetAddress getHostAddress( TServiceId  sid );

// Asks a service to stop and tell every one
void doUnregisterService (TServiceId sid);

/**
 * Manager for services instances
 * (Moved from the TICKS to the NS)
 * Implementable with layer 5, here implemented in NS (layer 3)
 * \date 2003
 */
class CServiceInstanceManager
{
public:
	
	/// Constructor
	CServiceInstanceManager();

	/** Add the name of a service which must not be duplicated
	 * If uniqueOnShard is true, only one service is allowed.
	 * If uniqueOnShard is false, one service is allowed by physical machine.
	 */
	void		addUniqueService( const std::string& serviceName, bool uniqueOnShard )
	{
		_UniqueServices.insert( std::make_pair( serviceName, uniqueOnShard ) );
	}

	/// Check if a service is allowed to start (if so, add it)
	bool		queryStartService( const std::string& serviceName, TServiceId  serviceId, const std::vector<SIPNET::CInetAddress> &addr, string& reason );

	/// Release a service instance
	void		releaseService( SIPNET::TServiceId serviceId );

	/// Display information
	void		displayInfo( SIPBASE::CLog *log = SIPBASE::InfoLog ) const;

	/// Make all controlled services quit
	void		killAllServices();

private:

	/// List of restricted services
	std::map< std::string, bool >	_UniqueServices;

	/// List of granted (online) services
	std::set< TServiceId >				_OnlineServices;
};


CServiceInstanceManager *SIMInstance = NULL;


/*
 * Constructor
 */
CServiceInstanceManager::CServiceInstanceManager()
{
	sipassert( ! SIMInstance );
	SIMInstance = this;

	// Note: addCallbackArray() done in CRangeMirrorManager::init()
}


/*
 * Check if a service is allowed to start. Answer with a GSTS (Grant Start Service) message
 */
bool		CServiceInstanceManager::queryStartService( const std::string& serviceName, TServiceId  serviceId, const vector<CInetAddress> &addr, string& reason )
{
	bool grantStarting = true;
	std::map< std::string, bool >::iterator ius = _UniqueServices.find( serviceName );
	if ( ius != _UniqueServices.end() )
	{
		// Service is restricted
		set< TServiceId >::iterator ios;
		bool uniqueOnShard = (*ius).second;
		for ( ios=_OnlineServices.begin(); ios!=_OnlineServices.end(); ++ios )
		{
			string name = getServiceName( *ios );
			if ( name == serviceName || strstr(serviceName.c_str(), name.c_str()) != NULL )
			{
				if ( uniqueOnShard )
				{
					// Only one service by shard is allowed => deny
					grantStarting = false;
					reason = toString( "Service %s already found as %hu, must be unique on shard", serviceName.c_str(), ios->get() );
					sipinfo( reason.c_str() );
					break;
				}
				else
				{
					// Only one service by physical machine is allowed

					// Implementation for layer5
					//TSockId hostid1, hostid2;
					/*CCallbackNetBase *cnb1 = CUnifiedNetwork::getInstance()->getNetBase( serviceId, hostid1 );
					CCallbackNetBase *cnb2 = CUnifiedNetwork::getInstance()->getNetBase( *ios, hostid2 );
					if ( cnb1->hostAddress( hostid1 ).internalIPAddress() == cnb2->hostAddress( hostid2 ).internalIPAddress() )*/

					// Implementation for NS
					if ( addr[0].internalIPAddress() == getHostAddress( *ios ).internalIPAddress() )
					{
						grantStarting = false;
						reason = toString( "Service %s already found as %hu on same machine", serviceName.c_str(), ios->get() );
						sipinfo( reason.c_str() );
						break;
					}
				}
			}
		}
	}

	if ( grantStarting )
	{
		_OnlineServices.insert( serviceId );
	}
	return grantStarting;
}


/*
 * Release a service instance
 */
void		CServiceInstanceManager::releaseService( SIPNET::TServiceId serviceId )
{
	_OnlineServices.erase( serviceId ); // not a problem if not found
}


/*
 * Display information
 */
void		CServiceInstanceManager::displayInfo( SIPBASE::CLog *log ) const
{
	log->displayNL( "Restricted services:" );
	std::map< std::string, bool >::const_iterator ius;
	for ( ius=_UniqueServices.begin(); ius!=_UniqueServices.end(); ++ius )
	{
		log->displayNL( "%s -> only one per %s", (*ius).first.c_str(), (*ius).second?"shard":"machine" );
	}
	log->displayNL( "Online registered services:" );
	std::set< TServiceId >::const_iterator ios;
	for ( ios=_OnlineServices.begin(); ios!=_OnlineServices.end(); ++ios )
	{
		log->displayNL( "%s", CUnifiedNetwork::getInstance()->getServiceUnifiedName( *ios ).c_str() );
	}
}


/*
 * Make all controlled services quit
 */
void		CServiceInstanceManager::killAllServices()
{
	// Send to all known online services
	std::set< TServiceId >::const_iterator ios;
	for ( ios=_OnlineServices.begin(); ios!=_OnlineServices.end(); ++ios )
	{
		doUnregisterService( (TServiceId)(*ios) );
	}
}



//
// Variables
//

list<CServiceEntry>	RegisteredServices;		/// List of all registred services

uint16				MinBasePort = 51000;	/// Ports begin at 51000
uint16				MaxBasePort = 52000;	/// (note: in this implementation there can be no more than 1000 services)

const TServiceId	BaseSId(128);			/// Allocated SIds begin at 128 (except for Agent Service)

const TTime			UnregisterTimeout = 10000;	/// After 10s we remove an unregister service if every server didn't ACK the message

CCallbackServer		*CallbackServer = NULL;

//
// Functions
//

bool canAccess (const vector<CInetAddress> &addr, const CServiceEntry &entry, vector<CInetAddress> &accessibleAddr)
{
	accessibleAddr.clear ();
	
	if (entry.WaitingUnregistration)
		return false;

	for (uint i = 0; i < addr.size(); i++)
	{
		uint32 net = addr[i].internalNetAddress();
		for (uint j = 0; j < entry.Addr.size(); j++)
		{
			if (net == entry.Addr[j].internalNetAddress())
			{
				accessibleAddr.push_back (entry.Addr[j]);
			}
		}
	}

	if (accessibleAddr.empty())
	{
		sipdebug ("service %s-%hu is not accessible by '%s'", entry.Name.c_str(), entry.SId.get(), vectorCInetAddressToString (addr).c_str ());
	}
	else
	{
		sipdebug ("service %s-%hu is accessible by '%s'", entry.Name.c_str(), entry.SId.get(), vectorCInetAddressToString (accessibleAddr).c_str ());
	}

	return !accessibleAddr.empty ();
}

void displayRegisteredServices (CLog *log = InfoLog)
{
	log->displayNL ("Display the %d registered services :", RegisteredServices.size());
	for (list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		TSockId id = (*it).SockId;
		if (id == NULL)
		{
			log->displayNL ("> %s-%hu %s '%s' %s %d addr", (*it).Name.c_str(), it->SId.get(), "<NULL>", "<NULL>", (*it).WaitingUnregistration?"WaitUnreg":"", (*it).Addr.size());
			for(uint i = 0; i < (*it).Addr.size(); i++)
				log->displayNL ("              '%s'", (*it).Addr[i].asString().c_str());
		}
		else
		{
			log->displayNL ("> %s-%hu %s '%s' %s %d addr", (*it).Name.c_str(), it->SId.get(), (*it).SockId->asString().c_str(), CallbackServer->hostAddress((*it).SockId).asString().c_str(), (*it).WaitingUnregistration?"WaitUnreg":"", (*it).Addr.size());
			for(uint i = 0; i < (*it).Addr.size(); i++)
				log->displayNL ("              '%s'", (*it).Addr[i].asString().c_str());
		}
	}
	log->displayNL ("End of the list");
}


list<CServiceEntry>::iterator effectivelyRemove (list<CServiceEntry>::iterator &it)
{
	// remove the service from the registered service list
	sipinfo ("Effectively remove the service %s-%hu", (*it).Name.c_str(), it->SId.get());
	return RegisteredServices.erase (it);
}

/*
 * Helper procedure for cbLookupAlternate and cbUnregister.
 * Note: name is used for a LOGS.
 */
list<CServiceEntry>::iterator doRemove (list<CServiceEntry>::iterator it)
{
	sipdebug ("Unregister the service %s-%hu '%s'", (*it).Name.c_str(), it->SId.get(), (*it).Addr[0].asString().c_str());
	
	// tell to everybody that this service is unregistered

	CMessage msgout (M_NAMING_CLIENT_UNB);
	msgout.serial ((*it).Name);
	msgout.serial ((*it).SId);

	vector<CInetAddress> accessibleAddress;
	sipinfo ("Broadcast the Unregistration of %s-%hu to all registered services", (*it).Name.c_str(), it->SId.get());
	for (list<CServiceEntry>::iterator it3 = RegisteredServices.begin(); it3 != RegisteredServices.end (); it3++)
	{
		if (canAccess((*it).Addr, (*it3), accessibleAddress))
		{
				CallbackServer->send (msgout, (*it3).SockId);
			//CNetManager::send (NS_S_NAME, msgout, (*it3).SockId);
			sipdebug ("Broadcast to %s-%hu", (*it3).Name.c_str(), it3->SId.get());
		}
	}

	// new system, after the unregistation broadcast, we wait ACK from all services before really remove
	// the service, before, we tag the service as 'wait before unregister'
	// if everybody didn't answer before the time out, we remove it

	(*it).SockId = NULL;

	(*it).WaitingUnregistration = true;
	(*it).WaitingUnregistrationTime = CTime::getLocalTime();

	// we remove all services awaiting his ACK because this service is down so it'll never ACK
	for (list<CServiceEntry>::iterator itr = RegisteredServices.begin(); itr != RegisteredServices.end (); itr++)
	{
		for (list<TServiceId>::iterator itw = (*itr).WaitingUnregistrationServices.begin(); itw != (*itr).WaitingUnregistrationServices.end ();)
		{
			if ((*itw) == (*it).SId)
			{
				itw = (*itr).WaitingUnregistrationServices.erase (itw);
			}
			else
			{
				itw++;
			}
		}
	}

	string res;
	for (list<CServiceEntry>::iterator it2 = RegisteredServices.begin(); it2 != RegisteredServices.end (); it2++)
	{
		if (!(*it2).WaitingUnregistration)
		{
			(*it).WaitingUnregistrationServices.push_back ((*it2).SId);
			res += toStringA((*it2).SId.get()) + " ";
		}
	}

	sipinfo ("Before removing the service, we wait the ACK of '%s'", res.c_str());
	
	// Release from the service instance manager
	SIMInstance->releaseService( (*it).SId );

	if ((*it).WaitingUnregistrationServices.empty())
	{
		return effectivelyRemove (it);
	}
	else
	{
		return ++it;
	}

}

void doUnregisterService (TServiceId sid)
{
	list<CServiceEntry>::iterator it;
	for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		if ((*it).SId == sid)
		{
			// found it, remove it
			doRemove (it);
			return;
		}
	}
	sipwarning ("Service %hu not found", sid.get());
}

void doUnregisterService (TSockId from)
{
	list<CServiceEntry>::iterator it;
	for (it = RegisteredServices.begin(); it != RegisteredServices.end ();)
	{
		if ((*it).SockId == from)
		{
			// it's possible that one "from" have more than one registred service, so we have to find in all the list
			// found it, remove it
			it = doRemove (it);
		}
		else
		{
			it++;
		}
	}
}

/*void doUnregisterService (const CInetAddress &addr)
{
	list<CServiceEntry>::iterator it;
	for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		if ((*it).Addr == addr)
		{
			// found it, remove it
			doRemove (it);
			return;
		}
	}
	sipwarning ("Service %s not found", addr.asString().c_str());
}*/

static	bool	checkMasterSlave(const string &name, TServiceId sid)
{
	list<CServiceEntry>::iterator it;
	for ( it = RegisteredServices.begin(); it != RegisteredServices.end(); it++ )
	{
		if ( (*it).SId != sid )
			if 	( strstr(name.c_str(), (*it).Name.c_str())!= NULL ||
				((*it).Name == name && (*it).SId != sid) )
				return false;
	}
	return true;
}

/*
 * Helper function for cbRegister.
 * If alloc_sid is true, sid is ignored
 * Returns false in case of failure of sid allocation or bad sid provided
 * Note: the reply is included in this function, because it must be done before things such as syncUniTime()
 */
bool doRegister (const string &name, const vector<CInetAddress> &addr, TServiceId sid, TSockId from, CCallbackNetBase &netbase, bool reconnection = false)
{
	// Find if the service is not already registered
	string reason;
	uint8 ok = true;
	bool needRegister = true;
	/*for (list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		if ((*it).Addr.asIPString() == addr.asIPString() )
		{
			// we already have a service on this address, remplace it if it's the same name
			if ((*it).Name == name)
			{
				// it's the same service, replace it
				(*it).SockId = from;
				sid = (*it).SId;
				sipinfo ("Replace the service %s", name.c_str());
			}
			else
			{
				sipwarning ("Try to register %s to %s but the service %s already on this address. ignore it!", name.c_str(), addr.asIPString().c_str(), (*it).Name.c_str());
				ok = false;
			}
			needRegister = false;
			break;
		}
	}*/

	if (needRegister)
	{
		if (sid.get() == 0)
		{
			// we have to find a sid
			sid = BaseSId;
			bool found = false;
			while (!found)
			{
				list<CServiceEntry>::iterator it;
				for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
				{
					if ((*it).SId == sid)
					{
						break;
					}
				}
				if (it == RegisteredServices.end ())
				{
					// ok, we have an empty sid
					found = true;
				}
				else
				{
					sid.set(sid.get()+1);
					if (sid.get() == 0) // round the clock
					{
						sipwarning ("Service identifier allocation overflow");
						ok = false;
						break;
					}
				}
			}

		}
		else
		{
			// we have to check that the user provided sid is available
			list<CServiceEntry>::iterator it;
			for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
			{
				if ((*it).SId == sid)
				{
					sipwarning ("Sid %d already used by another service", sid.get());
					ok = false;
					break;
				}
			}
			if (it != RegisteredServices.end ())
			{
				ok = true;
			}
		}

		// if ok, register the service and send a broadcast to other people
		if (ok)
		{
			// Check if the instance is allowed to start, according to the restriction in the config file			
			if ( SIMInstance->queryStartService( name, sid, addr, reason ) )
			{
				// add him in the registered list
				RegisteredServices.push_back (CServiceEntry(from, addr, name, sid));

				// tell to everybody but not him that this service is registered
				if (!reconnection)
				{
					CMessage msgout (M_NAMING_CLIENT_RGB);
					TServiceId::size_type s = 1;
					msgout.serial (s);
					msgout.serial (const_cast<string &>(name));
					msgout.serial (sid);
					// we need to send all addr to all services even if the service can't access because we use the address index
					// to know which connection comes.
					msgout.serialCont (const_cast<vector<CInetAddress> &>(addr));
					sipinfo ("The service is %s-%d, broadcast the Registration to everybody", name.c_str(), sid.get());

					vector<CInetAddress> accessibleAddress;
					for (list<CServiceEntry>::iterator it3 = RegisteredServices.begin(); it3 != RegisteredServices.end (); it3++)
					{
						// send only services that can be accessed and not itself
						if ((*it3).SId != sid && canAccess(addr, (*it3), accessibleAddress))
						{
							CallbackServer->send (msgout, (*it3).SockId);
							//CNetManager::send (NS_S_NAME, msgout, (*it3).SockId);
							sipdebug ("Broadcast to %s-%hu", (*it3).Name.c_str(), it3->SId.get());
						}
					}
				}

				// set the sid only if it s ok
				from->setAppId (sid.get());
			}
			else
			{
				// Reply "startup denied", and do not send registration to other services
				ok = false;
			}
		}

		// send the message to the service to say if it s ok or not
		if (!reconnection)
		{
			// send the answer to the client
			CMessage msgout (M_NAMING_CLIENT_RG);
			msgout.serial (ok);
			if (ok)
			{
				msgout.serial (sid);

				// send him all services available (also itself)
				TServiceId::size_type nb = 0;

				vector<CInetAddress> accessibleAddress;

				for (list<CServiceEntry>::iterator it2 = RegisteredServices.begin(); it2 != RegisteredServices.end (); it2++)
				{
					// send only services that are available
					if (canAccess(addr, (*it2), accessibleAddress))
						nb++;
				}
				msgout.serial (nb);

				for (list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
				{
					// send only services that are available
					if (canAccess(addr, (*it), accessibleAddress))
					{
						msgout.serial ((*it).Name);
						msgout.serial ((*it).SId);
						msgout.serialCont ((*it).Addr);
					}
				}

				//NamJuSong 2009/10/6
				bool bMasterSlave = checkMasterSlave(name, sid);
				msgout.serial (bMasterSlave);
			}
			else
			{
				msgout.serial( reason );
			}
			
			netbase.send (msgout, from);
			netbase.flush (from);
		}
	}

	//displayRegisteredServices ();

	return ok!=0;
}

void checkWaitingUnregistrationServices ()
{
	for (list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end ();)
	{
		if ((*it).WaitingUnregistration && ((*it).WaitingUnregistrationServices.empty() || CTime::getLocalTime() > (*it).WaitingUnregistrationTime + UnregisterTimeout))
		{
			if ((*it).WaitingUnregistrationServices.empty())
			{
				sipinfo ("Remove the service because all services ACK the remove");
			}
			else
			{
				string res;
				for (list<TServiceId>::iterator it2 = (*it).WaitingUnregistrationServices.begin(); it2 != (*it).WaitingUnregistrationServices.end (); it2++)
				{
					res += toStringA(it2->get()) + " ";
				}
				sipwarning ("Remove the service because time out occurs (service numbers %s didn't ACK)", res.c_str());
			}
			it = effectivelyRemove (it);
		}
		else
		{
			it++;
		}
	}
}


/**
 * Callback for service unregistration ACK. Mean that a service was ACK the unregistration broadcast
 */
// Packet : M_NAMING_CLIENT_ACK_UNI
static void cbACKUnregistration (CMessage& msgin, TSockId from, CCallbackNetBase &netbase)
{
	TServiceId sid;
	msgin.serial (sid);

	for (list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		if ((*it).SId == sid && (*it).WaitingUnregistration)
		{
			for (list<TServiceId>::iterator it2 = (*it).WaitingUnregistrationServices.begin(); it2 != (*it).WaitingUnregistrationServices.end (); it2++)
			{
				if (*it2 == TServiceId(uint16(from->appId())))
				{
					// remove the acked service
					(*it).WaitingUnregistrationServices.erase (it2);
					checkWaitingUnregistrationServices ();
					return;
				}
			}
		}
	}
}


/**
 * Callback for service registration when the naming service goes down and up (don't need to broadcast)
 */
// Packet : M_NAMING_CLIENT_RRG
static void cbResendRegisteration (CMessage& msgin, TSockId from, CCallbackNetBase &netbase)
{
	string name;
	vector<CInetAddress> addr;
	TServiceId sid;
	msgin.serial (name);
	msgin.serialCont (addr);
	msgin.serial (sid);

	doRegister (name, addr, sid, from, netbase, true);
}



/**
 * Callback for service registration.
 *
 * Message expected : RG
 * - Name of service to register (string)
 * - Address of service (CInetAddress)
 *
 * Message emitted : RG
 * - Allocated service identifier (TServiceId) or 0 if failed
 */
// Packet : M_NAMING_CLIENT_RG
static void cbRegister (CMessage& msgin, TSockId from, CCallbackNetBase &netbase)
{
	string name;
	vector<CInetAddress> addr;
	TServiceId sid;
	msgin.serial (name);
	msgin.serialCont (addr);
	msgin.serial (sid);

	doRegister (name, addr, sid, from, netbase);
}


/**
 * Callback for service unregistration.
 *
 * Message expected : UNI
 * - Service identifier (TServiceId)
 */
// Packet : M_NAMING_CLIENT_UNI
static void cbUnregisterSId (CMessage& msgin, TSockId from, CCallbackNetBase &netbase)
{
	TServiceId sid;
	msgin.serial( sid );

	doUnregisterService (sid);
	//displayRegisteredServices ();
}


/*
 * Helper function for cbQueryPort
 *
 * \warning QueryPort + Registration is not atomic so more than one service could ask a port before register
 */
uint16 doAllocatePort (const CInetAddress &addr)
{
	static uint16 nextAvailablePort = MinBasePort;

	// check if nextavailableport is free

	if (nextAvailablePort >= MaxBasePort) nextAvailablePort = MinBasePort;

	bool ok;
	do
	{
		ok = true;
		list<CServiceEntry>::iterator it;
		for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
		{
			if ((*it).Addr[0].port () == nextAvailablePort)
			{
				nextAvailablePort++;
				ok = false;
				break;
			}
		}
	}
	while (!ok);

	return nextAvailablePort++;
}


/**
 * Callback for port allocation
 * Note: if a service queries a port but does not register itself to the naming service, the
 * port will remain allocated and unused.
 *
 * Message expected : QP
 * - Name of service to register (string)
 * - Address of service (CInetAddress) (its port can be 0)
 *
 * Message emitted : QP
 * - Allocated port number (uint16)
 */
// Packet : M_NAMING_CLIENT_QP
static void cbQueryPort (CMessage& msgin, TSockId from, CCallbackNetBase &netbase)
{
	// Allocate port
	uint16 port = doAllocatePort (netbase.hostAddress (from));

	// Send port back
	CMessage msgout (M_NAMING_CLIENT_QP);
	msgout.serial (port);
	netbase.send (msgout, from);

	sipinfo ("The service got port %hu", port);
}


/*
 * Unregisters a service if it has not been done before.
 * Note: this callback is called whenever someone disconnects from the NS.
 * May be there are too many calls if many clients perform many transactional lookups.
 */
static void cbDisconnect /*(const string &serviceName, TSockId from, void *arg)*/ ( TSockId from, void *arg )
{
	doUnregisterService (from);
	//displayRegisteredServices ();
}

/*
 * a service is connected, send him all services infos
 */
static void cbConnect /*(const string &serviceName, TSockId from, void *arg)*/ ( TSockId from, void *arg )
{
	// we have to wait the registred services message to send all services because it this points, we can't know which sub net
	// the service can use

	//displayRegisteredServices ();

	// set the appid with a bad id (-1)
	from->setAppId (~0);
}

/*// returns the list of accessible services with a list of address
static void cbRegisteredServices(CMessage& msgin, TSockId from, CCallbackNetBase &netbase)
{
	vector<CInetAddress> addr;
	msgin.serialCont (addr);

	sipinfo ("New service ask me the available services, sending him all services available");
	// send to the new service the list of all services that this service can access (depending of his sub net)

	CMessage msgout ("RGB");

	uint8 nb = 0;

	vector<CInetAddress> accessibleAddress;

	for (list<CServiceEntry>::iterator it2 = RegisteredServices.begin(); it2 != RegisteredServices.end (); it2++)
	{
		// send only services that are available
		if (canAccess(addr, (*it2), accessibleAddress))
			nb++;
	}

	msgout.serial (nb);

	for (list<CServiceEntry>::iterator it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		// send only services that are available
		if (canAccess(addr, (*it), accessibleAddress))
		{
			msgout.serial ((*it).Name);
			msgout.serial ((*it).SId);
			msgout.serialCont (accessibleAddress);
		}
	}

	CNetManager::send (NS_S_NAME, msgout, from);
}*/


/*
 * Helper that emulates layer5 send()
 */
/*void sendToService( uint16 sid, CMessage& msgout )
{
	list<CServiceEntry>::iterator it;
	for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		if ((*it).SId == sid)
		{
			CallbackServer->send (msgout, (*it).SockId);
		}
	}
}*/


/*
 * Helper that emulate layer5's getServiceName()
 */
string getServiceName( TServiceId  sid )
{
	list<CServiceEntry>::iterator it;
	for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		if ((*it).SId == sid)
		{
			return (*it).Name;
		}
	}
	return ""; // not found
}


/*
 * Helper that returns the first address of a service
 */
CInetAddress getHostAddress( TServiceId  sid )
{
	list<CServiceEntry>::iterator it;
	for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
	{
		if ((*it).SId == sid)
		{
			return (*it).Addr[0];
		}
	}
	return CInetAddress();
}


//
// Callback array
//

TCallbackItem CallbackArray[] =
{
	{ M_NAMING_CLIENT_RG, cbRegister },
	{ M_NAMING_CLIENT_RRG, cbResendRegisteration },
	{ M_NAMING_CLIENT_QP, cbQueryPort },
	{ M_NAMING_CLIENT_UNI, cbUnregisterSId },
	{ M_NAMING_CLIENT_ACK_UNI, cbACKUnregistration },
//	{ "RS", cbRegisteredServices },
};

//
// Service
//
class CNamingService : public SIPNET::IService
{
public:

	void preInit()
	{
		_DontUseNS = true;
	}
	/**
	 * Init
	 */
	void init()
	{
		CConfigFile::CVar *var;

		// if a baseport is available in the config file, get it
		if ((var = ConfigFile.getVarPtr ("BasePort")) != NULL)
		{
			uint16 newBasePort = var->asInt ();
			sipinfo ("Changing the MinBasePort number from %hu to %hu", MinBasePort, newBasePort);
			sint32 delta = MaxBasePort - MinBasePort;
			sipassert (delta > 0);
			MinBasePort = newBasePort;
			MaxBasePort = MinBasePort + uint16 (delta);
		}

		// Parameters for the service instance manager
		try
		{
			// UniqueOnShardServices: unique services within a shard
			CConfigFile::CVar& uniqueServices = ConfigFile.getVar("UniqueOnShardServices");
			for ( uint i=0; i!=uniqueServices.size(); ++i )
			{
				_ServiceInstances.addUniqueService( uniqueServices.asString(i), true );
			}
		}
		catch(Exception &)
		{}
		try
		{
			// UniqueByMachineServices: unique services on a machine
			CConfigFile::CVar& uniqueServicesM = ConfigFile.getVar("UniqueByMachineServices");
			for ( uint i=0; i!=uniqueServicesM.size(); ++i )
			{
				_ServiceInstances.addUniqueService( uniqueServicesM.asString(i), false );
			}
		}
		catch(Exception &)
		{}

/*
		// we don't try to associate message from client
		CNetManager::getNetBase (NS_S_NAME)->ignoreAllUnknownId (true);

		// add the callback in case of disconnection
		CNetManager::setConnectionCallback (NS_S_NAME, cbConnect, NULL);
		
		// add the callback in case of disconnection
		CNetManager::setDisconnectionCallback (NS_S_NAME, cbDisconnect, NULL);
*/
		// DEBUG
		// DebugLog->addDisplayer( new CStdDisplayer() );

		vector<CInetAddress> v = CInetAddress::localAddresses();
		sipinfo ("%d detected local addresses:", v.size());
		for (uint i = 0; i < v.size(); i++)
		{
			sipinfo (" %d - '%s'",i, v[i].asString().c_str());
		}

		uint16 nsport = 50000;
		if ((var = ConfigFile.getVarPtr ("NSPort")) != NULL)
		{
			nsport = var->asInt ();
		}

		CallbackServer = new CCallbackServer;
		CallbackServer->init(nsport);
		CallbackServer->addCallbackArray(CallbackArray, sizeof(CallbackArray)/sizeof(CallbackArray[0]));
		CallbackServer->setConnectionCallback(cbConnect, NULL);
		CallbackServer->setDisconnectionCallback(cbDisconnect, NULL);
	}

	/**
	 * Update
	 */
	bool update ()
	{
		checkWaitingUnregistrationServices ();

		CallbackServer->update ();

		return true;
	}

	void release()
	{
		if (CallbackServer != NULL)
			delete CallbackServer;
		CallbackServer = NULL;
	}

private:

	/// Service instance manager singleton
	CServiceInstanceManager		_ServiceInstances;
};


static const char* getCompleteServiceName(const IService* theService)
{
	static std::string s;
	s= NS_L_NAME;

	if (theService->haveLongArg("nsname"))
	{
		s+= "_"+theService->getLongArg("nsname");
	}

	if (theService->haveLongArg("fullnsname"))
	{
		s= theService->getLongArg("fullnsname");
	}

	return s.c_str();
}

static const char* getShortServiceName(const IService* theService)
{
	static std::string s;
	s= NS_S_NAME;

	if (theService->haveLongArg("shortnsname"))
	{
		s= theService->getLongArg("shortnsname");
	}
	
	return s.c_str();
}
//
/// Naming Service
//

SIPNET_SERVICE_MAIN( CNamingService, NS_S_NAME, NS_L_NAME, "", 0, false, EmptyCallbackArray, SIPNS_CONFIG, SIPNS_LOGS)

//
// Commands
//


SIPBASE_CATEGORISED_COMMAND(NS, nsServices, "displays the list of all registered services", "")
{
	if(args.size() != 0) return false;

	displayRegisteredServices (&log);

	return true;
}

SIPBASE_CATEGORISED_COMMAND(NS, kill, "kill a service and send an unregister broadcast to other service", "<ServiceShortName>|<ServiceId>")
{
	if(args.size() != 1) return false;

	// try with number

	TServiceId sid(atoi(args[0].c_str()));
	if(sid.get() == 0)
	{
		// not a number, try a name
		list<CServiceEntry>::iterator it;
		for (it = RegisteredServices.begin(); it != RegisteredServices.end (); it++)
		{
			if ((*it).Name == args[0])
			{
				sid = (*it).SId;
				break;
			}
		}
		if (it == RegisteredServices.end())
		{
			log.displayNL ("Bad service name or id '%s'", args[0].c_str());
			return false;
		}
	}

	doUnregisterService (sid);
	return true;
}

SIPBASE_CATEGORISED_DYNVARIABLE(NS, uint32, NbRegisteredServices, "display the number of service that are registered in naming service")
{
	if (get) *pointer = static_cast<uint32>(RegisteredServices.size());
}

SIPBASE_CATEGORISED_COMMAND(NS, displayServiceInstances, "SIM: Display info on service instances", "" )
{
	SIMInstance->displayInfo( &log );
	return true;
}

SIPBASE_CATEGORISED_COMMAND(NS, killAllServices, "SIM: Make all the controlled services quit", "" )
{
	SIMInstance->killAllServices();
	return true;
}
