/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/callback_client.h"
#include "net/service.h"

#include "net/login_cookie.h"
#include "net/login_server.h"

#include "net/udp_sock.h"

#include "net/CryptEngine.h"
#include "./../../../common/common.h"
#include "./../../../common/err.h"

using namespace std;
using namespace SIPBASE;

namespace SIPNET {

struct CPendingUser
{
	CPendingUser (const CLoginCookie &cookie, uint8 &up, uint32 instanceId, uint32 charSlot)
		: Cookie (cookie), UserPriv(up), InstanceId(instanceId), CharSlot(charSlot)
	{
		Time = CTime::getSecondsSince1970();
	}

	CLoginCookie Cookie;
	uint8		UserPriv;		// privilege for executing commands from the clients
	uint32		InstanceId;		// the world instance in witch the user is awaited
	uint32		CharSlot;		// the expected character slot, any other will be denied
	uint32		Time;			// when the cookie is inserted in pending list
};

static list<CPendingUser> PendingUsers;

static CCallbackServer *Server = NULL;
static string ListenAddr;

static bool AcceptInvalidCookie = false;

static uint8 DefaultUserPriv = USERTYPE_COMMON;

static TDisconnectClientCallback DisconnectClientCallback = NULL;

// true=tcp   false=udp
static bool ModeTcp = 0;

// default value is 15 minutes
static uint TimeBeforeEraseCookie = 15*60;

/// contains the correspondance between userid and the sockid
map<uint32, TTcpSockId> UserIdSockAssociations;

TNewClientCallback			NewClientCallback = NULL;
TForceConnectionCallback	ForceCallback = NULL;

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
///////////// CONNECTION TO THE WELCOME SERVICE //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void	notifyWSRemovedPendingCookie(CLoginCookie& cookie)
{
	CMessage msgout (M_LOGIN_SERVER_RPC);	// remove pending cookie
	msgout.serial(cookie);
	CUnifiedNetwork::getInstance()->send ("WS", msgout);
}

void CLoginServer::refreshPendingList ()
{
	// delete too old cookie

	list<CPendingUser>::iterator it = PendingUsers.begin();
	uint32 Time = CTime::getSecondsSince1970();
	while (it != PendingUsers.end ())
	{
		if ((*it).Time < Time - TimeBeforeEraseCookie)
		{
			sipinfo("LS: Removing cookie '%s' because too old", (*it).Cookie.toString().c_str());
			notifyWSRemovedPendingCookie((*it).Cookie);
			it = PendingUsers.erase (it);
		}
		else
		{
			it++;
		}
	}
}

void	DisconnectAccount(uint32 userid)
{
	TTcpSockId	UserAddr;
	if (ModeTcp)
	{
		map<uint32, TTcpSockId>::iterator it = UserIdSockAssociations.find (userid);
		if (it != UserIdSockAssociations.end ())
		{
			CMessage	msgout(M_CLIENT_DISCONNECT);
			UserAddr = it->second;
			Server->send(msgout, UserAddr);
			Server->flush(UserAddr);
			sipinfo("FS: Disconnect the user %d", userid);
			Server->disconnect ((*it).second);
			UserIdSockAssociations.erase (userid);
		}
	}

	if (DisconnectClientCallback != NULL)
	{
		//DisconnectClientCallback (userid, serviceName);
	}
}

void	ForceDisconnectAccount(uint32 userid)
{
	TTcpSockId	UserAddr;
	if (ModeTcp)
	{
		map<uint32, TTcpSockId>::iterator it = UserIdSockAssociations.find (userid);
		if (it != UserIdSockAssociations.end ())
		{
			CMessage	msgout(M_CLIENT_DISCONNECT);
			UserAddr = it->second;
			Server->send(msgout, UserAddr);
			Server->flush(UserAddr);
			sipinfo ("FS: Disconnect the user %d", userid);
			Server->forceClose((*it).second);
			UserIdSockAssociations.erase (userid);
		}
	}

	if (DisconnectClientCallback != NULL)
	{
		//DisconnectClientCallback (userid, serviceName);
	}
}

// Packet:M_ENTER_SHARD_REQUEST
// WS -> FES
void cbWSChooseShard (CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	T_ERR		wErrNo;

	//	refreshPendingList ();
	CLoginCookie cookie;	msgin.serial (cookie);
	uint8		userPriv;	msgin.serial (userPriv);
	uint32		instanceId;	msgin.serial (instanceId);
	uint32		charSlot;	msgin.serial (charSlot);

	// add the user association
	uint32 userid = cookie.getUserId();
	map<uint32, TTcpSockId>::iterator itsock = UserIdSockAssociations.find (userid);
	if (itsock != UserIdSockAssociations.end ())
        DisconnectAccount(userid);

	vector<list<CPendingUser>::iterator> pendingToRemove;
	list<CPendingUser>::iterator it;
	for (it = PendingUsers.begin(); it != PendingUsers.end (); it++)
	{
		const CPendingUser &pu = *it;
		if (pu.Cookie.getUserId() == cookie.getUserId())
		{
			// the cookie already exists, erase it and return false
			sipwarning ("LS: Cookie %s is already in the pending user list [WRN_5]", cookie.toString().c_str());
//			notifyWSRemovedPendingCookie((*it).Cookie);
			PendingUsers.erase (it);
			// iterator is invalid, set it to end
			it = PendingUsers.end();
			break;
		}
	}
	// remove any useless cooky
	while (!pendingToRemove.empty())
	{
		PendingUsers.erase(pendingToRemove.back());
		pendingToRemove.pop_back();
	}

	if (it == PendingUsers.end ())
	{
		// add it to the awaiting client
		sipdebug("LS: New cookie %s (priv '%u' instance %u slot %u) inserted in the pending user list (awaiting new client)", cookie.toString().c_str(), userPriv, instanceId, charSlot);
		PendingUsers.push_back (CPendingUser (cookie, userPriv, instanceId, charSlot));
		wErrNo = ERR_NOERR;
		{
			uint32 uid = cookie.getUserId();
			uint32 addJifen = cookie.getAddJifen();
			if (addJifen > 0)
			{
				CMessage msgout (M_ADDEDJIFEN);
				msgout.serial (uid);
				msgout.serial (addJifen);
				CUnifiedNetwork::getInstance()->send ("LOBBY", msgout);	// LOBBY_SHORT_NAME
			}
		}
	}

	CMessage msgout (M_ENTER_SHARD_ANSWER);
	msgout.serial (wErrNo);
	msgout.serial (cookie);
	msgout.serial (ListenAddr);
	CUnifiedNetwork::getInstance()->send ("WS", msgout);
}

// Packet:M_CLIENT_DISCONNECT
// WS -> FES
void cbWSDisconnectClient (CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	// the WS tells me that i have to disconnect a client

	uint32 userid;
	msgin.serial (userid);
	sipinfo("<DC>: LS says disconnect client userid:%u", userid);

	DisconnectAccount(userid);
}

// Packet:M_M_ENTER__SHARD
// WS -> FES
void cbWSEnterShardOk (CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint32 userid;
	msgin.serial(userid);

	map<uint32, TTcpSockId>::iterator it = UserIdSockAssociations.find (userid);
	if (it == UserIdSockAssociations.end ())
	{
		sipwarning("UserIdSockAssociations.find (userid) = null. UID=%d", userid);
		return;
	}

	TTcpSockId	UserAddr = it->second;
	T_ERR	errNo = ERR_NOERR;

	CMessage msgOut(M_ENTER__SHARD);
	msgOut.serial (errNo);
	Server->send(msgOut, UserAddr);
	Server->flush(UserAddr);
}

static TUnifiedCallbackItem WSCallbackArray[] =
{
	{ M_ENTER_SHARD_REQUEST,	cbWSChooseShard },
	{ M_CLIENT_DISCONNECT,		cbWSDisconnectClient },
	{ M_ENTER__SHARD,			cbWSEnterShardOk },
};

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
///////////// CONNECTION TO THE CLIENT ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

/***************************************************************************************/
//									검사클라이언트
/***************************************************************************************/
// Packet : FORCESV
void	OnForceConnect( uint32	userid, TTcpSockId from, CCallbackNetBase& clientcb )
{
	map<uint32, TTcpSockId>::iterator it = UserIdSockAssociations.find (userid);
	if (it != UserIdSockAssociations.end ())
        DisconnectAccount(userid);

	CMessage msgout (M_LOGIN_SERVER_FORCESV);
	msgout.serial(userid);
	clientcb.send (msgout, from);

	if (ModeTcp)
		UserIdSockAssociations.insert (make_pair(userid, from));

	// identification OK, let's call the user callback
	if ( ForceCallback != NULL )
		ForceCallback (from, userid);

	// ok, now, he can call all callback
	//Server->authorizeOnly (NULL, from);
}

// static	uint32	uOrder = 1;
// Packet M_ENTER__SHARD
// Client -> FES
void cbShardValidation (CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase)
{
	//
	// S13: receive "SV" message from client
	//
	// the client send me a cookie
	CLoginCookie	cookie;
	T_ERR			errNo;
	msgin.serial (cookie);

	// add the user association
	uint32 userid = cookie.getUserId();
	map<uint32, TTcpSockId>::iterator it = UserIdSockAssociations.find (userid);
	if (it != UserIdSockAssociations.end ())
        DisconnectAccount(userid);

	uint8		userPriv;
	uint32		instanceId, charSlot;
	// verify that the user was pending
	errNo = CLoginServer::isValidCookie (cookie, userPriv, instanceId, charSlot, from, netbase);

	// if the cookie is not valid and we accept them, clear the error
	if ( errNo != ERR_NOERR )
	{
		if ( AcceptInvalidCookie )
		{
			errNo = ERR_NOERR;
			userPriv = DefaultUserPriv;
			instanceId = 0xffffffff;

			while (cookie.getUserId() == 0)
			{
				srand ((uint)CTime::getCurrentTime());			uint32 uID = rand();	uID += 1000;
				cookie.set (rand(), rand(), uID, userPriv, "", rand(), rand(), rand(), rand(), 0, 0, 0);
			}
			OnForceConnect(userid, from, netbase);
			return;
		}
	}

	/*
	if(AcceptInvalidCookie && errNo != ERR_NOERR)
	{
		errNo = ERR_NOERR;
		srand ((uint)CTime::getCurrentTime());			uint32 uID = rand();	uID += 1000;
		cookie.set (rand(), rand(), uID, rand(), rand(), rand(), rand());
	}
	if(AcceptInvalidCookie)
	{
		while (cookie.getUserId() == 0)
		{
			srand ((uint)CTime::getCurrentTime());			uint32 uID = rand();	uID += 1000;
			cookie.set (rand(), rand(), uID, rand(), rand(), rand(), rand());
		}
		OnForceConnect(userid, from, netbase);
		return;
	}
	*/

	if ( errNo != ERR_NOERR )
	{
		CMessage msgout2 (M_ENTER__SHARD);
		msgout2.serial (errNo);
		netbase.send (msgout2, from);

		sipwarning ("LS: User (%s) is not in the pending user list (cookie:%s) [WRN_6]", netbase.hostAddress(from).asString().c_str(), cookie.toString().c_str());
		// deconnect him
		netbase.disconnect (from);
		return;
	}
	else
	{
		///=====================
//		CMessage msgout2 (M_ENTER__SHARD);
//		msgout2.serial (errNo);
//		netbase.send (msgout2, from);

		if (ModeTcp)
			UserIdSockAssociations.insert (make_pair(userid, from));

		// identification OK, let's call the user callback
		if (NewClientCallback != NULL)
			NewClientCallback (from, cookie);

		// ok, now, he can call all callback
		//Server->authorizeOnly (NULL, from);
	}

}

// Packet : DC
// Client -> FES
void cbDisconnectionClient(CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase)
{
	// the WS tells me that i have to disconnect a client

	uint32	userid;
	msgin.serial (userid);

	DisconnectAccount(userid);
}

void SecurityClientConnection (TTcpSockId from, void *arg)
{
	// sipdebug("LS: new client connection: %s", from->asString ().c_str ());

	// the client could only call "SV" message
	Server->authorizeOnly (M_ENTER__SHARD, from, AuthorizeTime.get());
}

void ClientConnection (TTcpSockId from, void *arg)
{
	// the client could only call "SV" message
	Server->authorizeOnly (M_ENTER__SHARD, from, AuthorizeTime.get());
}

static const TCallbackItem ClientCallbackArray[] =
{
	{ M_ENTER__SHARD,			cbShardValidation },
	{ M_CLIENT_DISCONNECT,			cbDisconnectionClient },
};

void CLoginServer::setListenAddress(const string &la)
{
	// if the var is empty or not found, take it from the listenAddress()
	if (la.empty() && ModeTcp && Server != NULL)
	{
		ListenAddr = Server->listenAddress ().asIPString();
	}
	else
	{
		ListenAddr = la;
	}

	// check that listen address is valid
	if (ListenAddr.empty())
	{
		siperror("FATAL : listen address in invalid, it should be either set via ListenAddress variable or with -D argument");
		sipstop;
	}

	sipinfo("LS: Listen Address that will be sent to the client is now '%s'", ListenAddr.c_str());
}

uint32 CLoginServer::getNbPendingUsers()
{
	return PendingUsers.size();
}

void cfcbListenAddress (CConfigFile::CVar &var)
{
	CLoginServer::setListenAddress (var.asString());
}

void cfcbDefaultUserPriv(CConfigFile::CVar &var)
{
	// set the new ListenAddr
	DefaultUserPriv = var.asInt();
	
	sipinfo("LS: The default user priv is '%u'", DefaultUserPriv);
}

void cfcbAcceptInvalidCookie(CConfigFile::CVar &var)
{
	// set the new ListenAddr
	AcceptInvalidCookie = var.asInt() == 1;
	
	sipinfo("LS: This service %saccept invalid cookie", AcceptInvalidCookie?"":"doesn't ");
}

void cfcbTimeBeforeEraseCookie(CConfigFile::CVar &var)
{
	// set the new ListenAddr
	TimeBeforeEraseCookie = var.asInt();
	
	sipinfo("LS: This service will remove cookie after %d seconds", TimeBeforeEraseCookie);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
///////////// CONNECTION TO THE WELCOME SERVICE //////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

// temparary
void CLoginServer::init (CCallbackServer *server)
{
	Server = server;
}

// common init
void CLoginServer::init (const string &listenAddress)
{
	// connect to the welcome service
	connectToWS ();
	
	try {
		cfcbDefaultUserPriv(IService::getInstance()->ConfigFile.getVar("DefaultUserPriv"));
		IService::getInstance()->ConfigFile.setCallback("DefaultUserPriv", cfcbDefaultUserPriv);
	} catch(Exception &) { }
	
	try {
		cfcbAcceptInvalidCookie (IService::getInstance()->ConfigFile.getVar("AcceptInvalidCookie"));
		IService::getInstance()->ConfigFile.setCallback("AcceptInvalidCookie", cfcbAcceptInvalidCookie);
	} catch(Exception &) { }

	try {
		cfcbTimeBeforeEraseCookie (IService::getInstance()->ConfigFile.getVar("TimeBeforeEraseCookie"));
		IService::getInstance()->ConfigFile.setCallback("TimeBeforeEraseCookie", cfcbTimeBeforeEraseCookie);
	} catch(Exception &) { }

	// setup the listen address

	string la;
	
	if (IService::getInstance()->haveArg('D'))
	{
		// use the command line param if set
		la = IService::getInstance()->getArg('D');
	}
	else if (IService::getInstance()->ConfigFile.exists ("ListenAddress"))
	{
		// use the config file param if set
		la = IService::getInstance()->ConfigFile.getVar ("ListenAddress").asString();
	}
	else
	{
		la = listenAddress;
	}
	setListenAddress (la);
	IService::getInstance()->ConfigFile.setCallback("ListenAddress", cfcbListenAddress);
}

// listen socket is TCP
void CLoginServer::init (CCallbackServer* server, CInetAddress & listenAddr, TNewClientCallback ncl)
{
	init (listenAddr.asIPString());

	Server = server;
	Server->setConnectionCallback(ClientConnection, server);

	Server->CCallbackNetBase::addCallbackArray(ClientCallbackArray, sizeof(ClientCallbackArray)/sizeof(ClientCallbackArray[0]));

	if ( Server == NULL )
	{
		sipinfo("SS Listening Port for Client opening Failed!");
		throw	ESocket("Server init Error, reason");
	}

	NewClientCallback = ncl;
	ModeTcp = true;
}

void	CLoginServer::setForceConnectionCallback (CCallbackServer *server, TForceConnectionCallback cb)
{
	ForceCallback = cb;
}

// listen socket is UDP
void CLoginServer::init (CUdpSock &server, TDisconnectClientCallback dc)
{
	init (server.localAddr ().asIPString());

	DisconnectClientCallback = dc;

	ModeTcp = false;
}

void CLoginServer::init (const std::string &listenAddr, TDisconnectClientCallback dc)
{
	init (listenAddr);

	DisconnectClientCallback = dc;

	ModeTcp = false;
}

T_ERR CLoginServer::isValidCookie (const CLoginCookie &lc, uint8 &userPriv, uint32 &instanceId, uint32 &charSlot, TTcpSockId from, CCallbackNetBase &netbase)
{
	userPriv = 0;

	if (!AcceptInvalidCookie && !lc.isValid())
		return ERR_INVALIDLOGINCOOKIE;

	// verify that the user was pending
	list<CPendingUser>::iterator it;
	for (it = PendingUsers.begin(); it != PendingUsers.end (); it++)
	{
		CPendingUser &pu = *it;
		if (pu.Cookie == lc)
		{
			// sipdebug ("LS: Cookie '%s' is valid and pending (user %u), send the client connection to the WS", lc.toString ().c_str (), lc.getUserId ());

			// warn the WS that the client effectively connected
			uint8 con = 1;
			CMessage msgout (M_CLIENT_CONNECT_STATUS);
			uint32 userid = lc.getUserId();
			msgout.serial (userid);
			msgout.serial (con);
			const CInetAddress &ia = netbase.hostAddress(from);
			string str = ia.asIPString();
			msgout.serial(str);
			msgout.serial(pu.Cookie);

			CUnifiedNetwork::getInstance()->send("WS", msgout);

			userPriv = pu.UserPriv;
			instanceId = pu.InstanceId;
			charSlot = pu.CharSlot;

			// ok, it was validate, remove it
			PendingUsers.erase (it);

			return ERR_NOERR;
		}
	}

	// problem
	return ERR_NOCOOKIE;
}

void CLoginServer::connectToWS ()
{
	CUnifiedNetwork::getInstance()->addCallbackArray(WSCallbackArray, sizeof(WSCallbackArray)/sizeof(WSCallbackArray[0]));
}

void CLoginServer::onClientDisconnected (uint32 userId, uint8 bIsGM)
{
	uint8 con = 0;
	CMessage msgout (M_CLIENT_CONNECT_STATUS);
	msgout.serial (userId);
	msgout.serial (con);
	msgout.serial (bIsGM);

	CUnifiedNetwork::getInstance()->send("WS", msgout);

	// remove the user association
	if (ModeTcp)
		UserIdSockAssociations.erase (userId);
}

void CLoginServer::disconnectUser(TSockId from)
{
	//TSockId	from;
	//CCallbackNetBase *cnb = CUnifiedNetwork::getInstance()->getNetBase(sid, from);
	Server->flush(from);
	Server->disconnect(from);
}

/// Call this method to retrieve the listen address
const std::string &CLoginServer::getListenAddress()
{
	return ListenAddr;
}

bool CLoginServer::acceptsInvalidCookie()
{
	return AcceptInvalidCookie;
}


//
// Commands
//

SIPBASE_CATEGORISED_COMMAND(sip, lsUsers, "displays the list of all connected users", "")
{
	if(args.size() != 0) return false;

	if (ModeTcp)
	{
		log.displayNL ("Display the %d connected users :", UserIdSockAssociations.size());
		for (map<uint32, TTcpSockId>::iterator it = UserIdSockAssociations.begin(); it != UserIdSockAssociations.end (); it++)
		{
			log.displayNL ("> %u %s", (*it).first, (*it).second->asString().c_str());
		}
		log.displayNL ("End of the list");
	}
	else
	{
		log.displayNL ("No user list in udp mode");
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(sip, lsPending, "displays the list of all pending users", "")
{
	if(args.size() != 0) return false;

	log.displayNL ("Display the %d pending users :", PendingUsers.size());
	for (list<CPendingUser>::iterator it = PendingUsers.begin(); it != PendingUsers.end (); it++)
	{
		log.displayNL ("> %s", (*it).Cookie.toString().c_str());
	}
	log.displayNL ("End of the list");

	return true;
}


SIPBASE_CATEGORISED_DYNVARIABLE(sip, string, LSListenAddress, "the listen address sended to the client to connect on this front_end")
{
	if (get)
	{
		*pointer = ListenAddr;
	}
	else
	{
		if ((*pointer).find (":") == string::npos)
		{
			sipwarning ("LS: You must set the address + port (ie: \"www.samilpo.org:38000\")");
			return;
		}
		else if ((*pointer).empty())
		{
			ListenAddr = Server->listenAddress ().asIPString();
		}
		else
		{
			ListenAddr = *pointer;
		}
		sipinfo ("LS: Listen Address that will be send to client is '%s'", ListenAddr.c_str());
	}
}

SIPBASE_CATEGORISED_VARIABLE(sip, uint8, DefaultUserPriv, "Default User priv for people who don't use the login system");

} // SIPNET
