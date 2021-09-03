/** \file unified_network_mt.h
 * Network engine, layer 5
 *
 * $Id: unified_network_mt.h,v 1.2 2010/02/18 08:38:47 KimSuRyon Exp $
 */


/***************************************************************************
**** This is the same class as unified network but thread safe but contains
**** lot of bugs so it was #if 0 and only here "in case of"
***************************************************************************/

#if 0


#ifndef SIP_UNIFIED_NETWORD_H
#define SIP_UNIFIED_NETWORD_H

#include "misc/types_sip.h"

#include <hash_map>

#include "misc/time_sip.h"
#include "callback_client.h"
#include "callback_server.h"
#include "naming_client.h"
#include "misc/mutex.h"
#include "misc/reader_writer.h"

namespace SIPNET {

/** Callback function type for service up/down processing
 * \param serviceName name of the service that is un/registered to the naming service
 * \param arg a pointer initialized by the user
 */
typedef void (*TUnifiedNetCallback) (const std::string &serviceName, uint16 sid, void *arg);

/** Callback function type for message processing
 * \param msgin message received
 * \param serviceName name of the service that sent the message
 * \param sid id of the service that sent the message
 */
typedef void (*TUnifiedMsgCallback) (CMessage &msgin, const std::string &serviceName, uint16 sid);

/// Callback items. See CMsgSocket::update() for an explanation on how the callbacks are called.
struct TUnifiedCallbackItem
{
	/// Key C string. It is a message type name, or "C" for connection or "D" for disconnection
	char				*Key;
	/// The callback function
	TUnifiedMsgCallback	Callback;

};

/**
 * Layer 5
 *
 * \date 2001
 */
class CUnifiedNetwork
{
	friend void	uncbConnection(TTcpSockId from, void *arg);
	friend void	uncbDisconnection(TTcpSockId from, void *arg);
	friend void	uncbServiceIdentification(CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase);
	friend void	uncbMsgProcessing(CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase);
	friend void	uNetUnregistrationBroadcast(const std::string &name, TServiceId sid, const CInetAddress &addr);

	SIPBASE_SAFE_SINGLETON_DECL_PTR(CUnifiedNetwork);
public:

	/** Returns the singleton instance of the CUnifiedNetwork class.
	 */
//	static CUnifiedNetwork *getInstance ();

	/** Returns true if the application called getInstance(). This function is used to know if the user is using layer 4 or layer 5
	 */
	static bool isUsed ();

	/** Creates the connection to the Naming Service.
	 * If the connection failed, ESocketConnectionFailed exception is generated.
	 * This function is called automatically called by the service class at the beginning.
	 *
	 * \param addr address of the naming service (NULL is you don't want to use the naming service)
	 * \param rec recorging state to know if we have to record or replay messages
	 */
	void	init (const CInetAddress *addr, CCallbackNetBase::TRecordingState rec, const std::string &shortName, uint16 port, TServiceId &sid );

	/** Registers to the Naming Service, and connects to the present services
	 */
	void	connect();

	/** Closes the connection to the naming service, every other connection and free.
	 */
	void	release ();

	/** Adds a specific service to the list of connected services.
	 */
	void	addService(const std::string &name, const CInetAddress &addr, bool sendId = true, bool external = true, uint16 sid=0, bool autoRetry = true);
	void	addService(const std::string &name, const std::vector<CInetAddress> &addr, bool sendId = true, bool external = true, uint16 sid=0, bool autoRetry = true);

	/** Adds a callback array in the system. You can add callback only *after* adding the server, the client or the group.
	 */
	void	addCallbackArray (const TUnifiedCallbackItem *callbackarray, SIPBASE::CStringIdArray::TStringId arraysize);

	/** Call it evenly. the parameter select the timeout value in seconds for each update. You are absolutely certain that this
	 * function will not be returns before this amount of time you set.
	 * If you set the update timeout value higher than 0, all messages in queues will be process until the time is greater than the timeout user update().
	 * If you set the update timeout value to 0, all messages in queues will be process one time before calling the user update(). In this case, we don't sipSleep(1).
	 */
	void	update (SIPBASE::TTime timeout = 0);

	/** Sends a message to a specific serviceName. If there's more than one service with this name, all services of this name will receive the message.
	 * \param serviceName name of the service you want to send the message (may not be unique.)
	 * \param the message you want to send.
	 */
	void	send (const std::string &serviceName, const CMessage &msg);

	/** Sends a message to a specific serviceId.
	 * \param serviceId Id of the service you want to send the message.
	 * \param the message you want to send.
	 */
	void	send (uint16 serviceId, const CMessage &msg);

	/** Broadcasts a message to all connected services.
	 * \param the message you want to send.
	 */
	void	send (const CMessage &msg);


	/** Sets callback for incoming connections.
	 * On a client, the callback will be call when the connection to the server is established (the first connection or after the server shutdown and started)
	 * On a server, the callback is called each time a new client is connected to him
	 * 
	 * If the serviceName is "*", you can set more than one callback, each one will be called one after one.
	 * Otherwise only the last setCallback will be called (and you can set cb to NULL to remove the callback).
	 * If the serviceName is "*", the callback will be call for any services
	 * If you set the same callback for a specific service S and for "*", the callback might be
	 * call twice (in case the service S is up)
	 */
	void	setServiceUpCallback (const std::string &serviceName, TUnifiedNetCallback cb, void *arg = NULL, bool back=true);

	/** Sets callback for disconnections.
	 * On a client, the callback will be call each time the connection to the server is lost.
	 * On a server, the callback is called each time a client is disconnected.
	 * 
	 * If the serviceName is "*", you can set more than one callback, each one will be called one after one.
	 * Otherwise only the last setCallback will be called (and you can set cb to NULL to remove the callback).
	 * If the serviceName is "*", the callback will be call for any services
	 * If you set the same callback for a specific service S and for "*", the callback might be
	 * call twice (in case the service S is down)
	 */
	void	setServiceDownCallback (const std::string &serviceName, TUnifiedNetCallback cb, void *arg = NULL, bool back=true);


	/// Gets the CCallbackNetBase of the service
	CCallbackNetBase	*getNetBase(const std::string &name, TTcpSockId &host);

	/// Gets the CCallbackNetBase of the service
	CCallbackNetBase	*getNetBase(TServiceId sid, TTcpSockId &host);

	/// Gets the total number of bytes sent
	uint64				getBytesSent ();

	/// Gets the total number of bytes received
	uint64				getBytesReceived ();

	/// Gets the total number of bytes queued for sending
	uint64				getSendQueueSize ();

	/// Gets the total number of bytes queued after receiving
	uint64				getReceiveQueueSize ();

	/// Find a callback in the array
	TUnifiedMsgCallback findCallback (const std::string &callbackName);

private:

	/// A map of service ids, referred by a service name
	struct TNameMappedConnection : public std::hash_multimap<std::string, uint16> {};

	/// A map of callbacks, reffered by message name
	typedef std::map<std::string, TUnifiedMsgCallback>			TMsgMappedCallback;


	/// A callback and its user data
	typedef std::pair<TUnifiedNetCallback, void *>				TCallbackArgItem;
	/// A Map of service up/down callbacks with their user data.
	typedef std::hash_multimap<std::string, TCallbackArgItem>	TNameMappedCallback;



	/// This may contains a CCallbackClient or a TTcpSockId, depending on which type of connection it is.
	class CUnifiedConnection
	{
	public:
		/// The name of the service (may not be unique)
		std::string				ServiceName;
		/// The id of the service (is unique)
		uint16					ServiceId;
		/// If the current service is connect to the other service as a server or a client
		bool					IsServerConnection;
		/// If the service entry is used
		bool					EntryUsed;
		/// The connection state
		bool					IsConnected;
		/// If the connection is extern to the naming service
		bool					IsExternal;
		/// Auto-retry mode
		bool					AutoRetry;
		/// Auto identify at connection
		bool					SendId;
		/// The external connection address
		std::vector<CInetAddress>			ExtAddress;
		/// The connection
		struct TConnection
		{
			TConnection(CCallbackClient*cbc) : CbClient(cbc) { HostId = 0; }
			TConnection(TTcpSockId hi) : HostId(hi) { CbClient = 0; }
			CCallbackClient		*CbClient;
			TTcpSockId				HostId;
		};
		std::vector<TConnection>				Connection;
		///
		bool					AutoCheck;

		CUnifiedConnection() { reset(); }

		CUnifiedConnection(const std::string &name, uint16 id) : ServiceName(name), ServiceId(id), IsServerConnection(false), EntryUsed(true), IsConnected(false), IsExternal(false), AutoCheck(false) { Connection.clear (); }

		CUnifiedConnection(const std::string &name, uint16 id, TTcpSockId host) : ServiceName(name), ServiceId(id), IsServerConnection(true), EntryUsed(true), IsConnected(false), IsExternal(false), AutoCheck(false) { sipassert (host != InvalidSockId); Connection.clear (); Connection.push_back(host); }

		void					reset();
	};

	class CConnectionId
	{
	public:
		std::string	SName;
		uint16		SId;
		TTcpSockId		SHost;
		bool		NeedInsert;		// patch in case of deconnection->reconnection in the same loop

		CConnectionId() : SName("DEAD"), SId(0xDEAD), SHost(InvalidSockId), NeedInsert(false) {}
		CConnectionId(const std::string &name, uint16 sid, TTcpSockId hid = InvalidSockId, bool needInsert = true) : SName(name), SId(sid), SHost(hid), NeedInsert(needInsert) {}
	};

	//

	/// Vector of connections by service id
	SIPBASE::CRWSynchronized< std::vector<CUnifiedConnection> >	_IdCnx;

	/// Map of connections by service name
	SIPBASE::CRWSynchronized<TNameMappedConnection>				_NamedCnx;

	/// The callback server
	CCallbackServer												*_CbServer;

	/// The server port
	uint16														_ServerPort;

	/// Map of the up/down service callbacks
	TNameMappedCallback											_UpCallbacks;
	std::vector<TCallbackArgItem>								_UpUniCallback;
	TNameMappedCallback											_DownCallbacks;
	std::vector<TCallbackArgItem>								_DownUniCallback;

	/// Recording state
	CCallbackNetBase::TRecordingState							_RecordingState;

	/// Service id of the running service
	TServiceId													_SId;

	/// Service name
	std::string													_Name;

	/// Map of callbacks
	TMsgMappedCallback											_Callbacks;

	/// Used for external service
	uint16														_ExtSId;

	/// Last time of retry
	SIPBASE::TTime												_LastRetry;

	/// Time of the theorical next update
	SIPBASE::TTime												_NextUpdateTime;

	/// internal stacks used for adding/removing connections in multithread
	std::vector<CConnectionId>									_ConnectionStack;
	std::vector<uint16>											_DisconnectionStack;
	std::vector<uint16>											_ConnectionRetriesStack;
	std::vector<bool>											_TempDisconnectionTable;


	/// The main instance
//	static CUnifiedNetwork										*_Instance;

	//
	SIPBASE::CMutex												_Mutex;
	volatile uint												_MThreadId;
	volatile uint												_MutexCount;

	// Naming service
	SIPNET::CInetAddress											_NamingServiceAddr;

	// Initialised
	bool														_Initialised;

	//
	CUnifiedNetwork() : _ExtSId(256), _LastRetry(0), _MThreadId(0xFFFFFFFF), _MutexCount(0), _CbServer(NULL), _NextUpdateTime(0), _Initialised(false)
	{
	}

	~CUnifiedNetwork() { }
	
	//
	void	updateConnectionTable();

	//
	void	enterReentrant();
	void	leaveReentrant();

	//
	void	autoCheck();
};


} // SIPNET


#endif // SIP_UNIFIED_NETWORK_H

#endif // #if 0

/* End of unified_network.h */
