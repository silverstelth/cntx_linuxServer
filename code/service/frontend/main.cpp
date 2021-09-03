
/*
 * This file contain the Snowballs Frontend Service.
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <map>
#include <utility>

// This include is mandatory to use SIP. It include SIP types.
#include <misc/types_sip.h>

// We're using the SIP Service framework and layer 5
#include "net/service.h"
#include "net/login_server.h"
#include "net/Key.h"
#include "net/CryptEngine.h"
#include "net/callback_server.h"
#include "net/UdpHolePunchingServer.h"

#include "../Common/DBData.h"
#include "../Common/Common.h"
#include "../../common/Packet.h"

#include "main.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

CCallbackServer *Clients = 0;
static CUdpHolePunchingServer	*UDPHolePunchingServer = NULL;

CVariable<uint16>	FSPort("FES", "FSPort", "TCP Port of FES Server for Clients", 37000, 0, true);
CVariable<uint16>	UdpPeerPort("FES", "UdpPeerPort", "UDP Port of FES Server for Clients(Voice chatting)", 0, 0, true);
CVariable<uint32>	FSLockFlag("FES", "FSLockFlag", "Show FSLockFlag, 0:Unlocked, 1:Locked", 0, 0, true);

//TSockId clientfrom;

/*
 * Keep a list of the players connected to that Frontend. Only map Id
 * to a Connection
 */

_pmap localPlayers;

// Get User id from socket id
uint32	GetIdentifiedID(TSockId _sid)
{
	uint32 id = (uint32)_sid->appId();
	if(id == 0)
		return 0;
	return id;
}
// Get socket id from User id
TSockId	GetSocketForIden(uint32 _uid)
{
	_pmap::iterator ItPlayer;
	ItPlayer = localPlayers.find(_uid);
	if ( ItPlayer == localPlayers.end() )
		return TSockId(0);
	else
		return ((*ItPlayer).second).con;
}

bool	IsGM(uint32 _uid)
{
	_pmap::iterator ItPlayer;
	ItPlayer = localPlayers.find(_uid);
	if ( ItPlayer == localPlayers.end() )
		return false;
//return true;

	uint8	userType = ItPlayer->second.UserType;
	if ( ItPlayer->second.bIsGM &&
		( userType == USERTYPE_GAMEMANAGER ||
		userType == USERTYPE_GAMEMANAGER_H ||
		userType == USERTYPE_EXPROOMMANAGER || 
		userType == USERTYPE_WEBMANAGER) )
		return true;

	return false;
}

bool	IsGMActive(uint32 _uid)
{
	_pmap::iterator ItPlayer;
	ItPlayer = localPlayers.find(_uid);
	if ( ItPlayer == localPlayers.end() )
		return false;
	//return true;

	uint8	userType = ItPlayer->second.UserType;
	if ( ItPlayer->second.bIsGM &&
		( userType == USERTYPE_GAMEMANAGER ||
		userType == USERTYPE_GAMEMANAGER_H ||
		userType == USERTYPE_EXPROOMMANAGER ||
		userType == USERTYPE_WEBMANAGER ) )

		return (ItPlayer->second.GMActive > 0);

	return false;
}

uint8	GetUserType(uint32 _uid)
{
	_pmap::iterator ItPlayer;
	ItPlayer = localPlayers.find(_uid);
	if ( ItPlayer == localPlayers.end() )
		return 0;

	return ItPlayer->second.UserType;
}

void	SendGMFail(T_ERR err, SIPNET::TTcpSockId from)
{
	CMessage	msgout(M_GMSC_FAIL);
	sipwarning("GM Command Error %d", err);
	msgout.serial(err);
	Clients->send( msgout, from );
}

bool	AbleGMPacketAccept(uint32 _uid, MsgNameType mesName)
{
	_pmap::iterator ItPlayer;
	ItPlayer = localPlayers.find(_uid);
	if (ItPlayer == localPlayers.end())
		return false;
//return true;

	TTcpSockId	from	= ItPlayer->second.con;
	uint8		uType	= ItPlayer->second.UserType;
	if (!IsGM(_uid))
	{
		SendGMFail(ERR_GM_NOTYPE, from);
		sipwarning("He(%d) 's not GM", _uid);
		return false;
	}

	if (ItPlayer->second.GMActive == 0)
	{
		SendGMFail(ERR_GM_NOACTIVATE, from);
		sipwarning("GM(%d) 's not active", _uid);
		return false;
	}

	if (mesName == M_GMCS_SHOW)
		return true;

	bool	bRet = true;
	switch (uType)
	{
	case USERTYPE_GAMEMANAGER:
		{
			if (mesName == M_GMCS_GETITEM ||
				//mesName == M_GMCS_GIVEITEM ||
				mesName == M_GMCS_GETINVENEMPTY) // USERTYPE_GAMEMANAGER_H -> OK about ALL
			{
				SendGMFail(ERR_GM_PRIV, from);
				sipwarning("GM(%d) 's not High_GM, but Low_GM", _uid);
				bRet = false;
			}
		}
	case USERTYPE_GAMEMANAGER_H:
		{
			if (mesName == M_GMCS_EXPROOM_GROUP_ADD 
				|| mesName == M_GMCS_EXPROOM_GROUP_MODIFY 
				|| mesName == M_GMCS_EXPROOM_GROUP_DELETE 
				|| mesName == M_GMCS_EXPROOM_ADD 
				|| mesName == M_GMCS_EXPROOM_MODIFY 
				|| mesName == M_GMCS_EXPROOM_DELETE 
				//|| mesName == M_GMCS_RECOMMEND_SET 
				//|| mesName == M_GMCS_REVIEW_ADD 
				//|| mesName == M_GMCS_REVIEW_MODIFY 
				//|| mesName == M_GMCS_REVIEW_DELETE
				|| mesName == M_GMCS_PUBLICROOM_FRAME_SET
				|| mesName == M_GMCS_LARGEACT_SET
				|| mesName == M_GMCS_LARGEACT_DETAIL
				)
			{
				SendGMFail(ERR_GM_PRIV_EXPMAN, from);
				sipwarning("GM(%d) 's not ExpRoom Manager, but High_GM", _uid);
				bRet = false;
			}
		}
		break;
	case USERTYPE_EXPROOMMANAGER:
		{
			if (mesName != M_GMCS_EXPROOM_GROUP_ADD  
				&& mesName != M_GMCS_EXPROOM_GROUP_MODIFY 
				&& mesName != M_GMCS_EXPROOM_GROUP_DELETE 
				&& mesName != M_GMCS_EXPROOM_ADD 
				&& mesName != M_GMCS_EXPROOM_MODIFY 
				&& mesName != M_GMCS_EXPROOM_DELETE 
				//&& mesName != M_GMCS_RECOMMEND_SET 
				//&& mesName != M_GMCS_REVIEW_ADD 
				//&& mesName != M_GMCS_REVIEW_MODIFY 
				//&& mesName != M_GMCS_REVIEW_DELETE
				&& mesName != M_GMCS_PUBLICROOM_FRAME_SET
				&& mesName != M_GMCS_LARGEACT_SET
				&& mesName != M_GMCS_LARGEACT_DETAIL
				)
			{
				SendGMFail(ERR_GM_PRIV, from);
#ifdef SIP_MSG_NAME_DWORD
				sipwarning("GM(%d) 's a ExpRoom Manager, so can run only EXPRoom Command. Message=%d", _uid, mesName);
#else
				sipwarning("GM(%d) 's a ExpRoom Manager, so can run only EXPRoom Command.", _uid);
#endif // SIP_MSG_NAME_DWORD
				bRet = false;
			}
		}
		break;

	case USERTYPE_WEBMANAGER:
		{
			if (mesName != M_GMCS_GET_GMONEY)
			{
				SendGMFail(ERR_GM_PRIV, from);
				sipwarning("GM(%d) 's Web Manager.", _uid);
				bRet = false;
			}
		}
		break;
	}

	return	bRet;
}

void	SetGMActive(uint32 _uid, uint8 _bOn)
{
	if ( !IsGM(_uid) )
		return;
	_pmap::iterator ItPlayer = localPlayers.find(_uid);
	ItPlayer->second.GMActive = (uint8)_bOn;
}

ucstring	GetUserName(uint32 _uid)
{
	_pmap::iterator ItPlayer;
	ItPlayer = localPlayers.find(_uid);
	if ( ItPlayer == localPlayers.end() )
		return "";

	return ItPlayer->second.UserName;
}
extern	void	AddUserToPacketFilter(uint32 _uid);
extern	void	DelUserToPacketFilter(uint32 _uid);

/****************************************************************************
 * Connection callback for a client
 ****************************************************************************/
void onConnectionClient (TSockId from, const CLoginCookie &cookie)
{
	if (FSLockFlag)
	{
		sipwarning("FSLockFlag = 1");
		Clients->disconnect(from);
		return;
	}

	uint32 id;

	id = cookie.getUserId();
	uint8		uUserType = cookie.getUserType();
	ucstring	ucUserName = cookie.getUserName();
	uint8		bIsGM = cookie.getIsGM();

	// Add new client to the list of player managed by this FrontEnd
	pair<_pmap::iterator, bool> player = localPlayers.insert( make_pair( id, CPlayer( id, from, uUserType, ucUserName, bIsGM )));
	//sipinfo( L"The client(%s)  is connected, UserType=%d, GM=%d, SOCKET=%p %S, IPAdress=%S, CurUserNum=%d", ucUserName.c_str(), uUserType, bIsGM, from, from->asString().c_str(), from->getTcpSock()->remoteAddr().ipAddress().c_str(), localPlayers.size() ); byy krs
	
	AddUserToPacketFilter(id);
	// store the player info in appId

	_pmap::iterator it = player.first;
	CPlayer *p = &((*it).second);
	from->setAppId((uint64)(uint)p->id);

	// Output: send the IDENTIFICATION number to the new connected client
	CMessage msgout( M_SC_IDENTIFICATION );
	msgout.serial( id );

	// Send the message to connected client "from"
	Clients->send( msgout, from );

	// sipdebug( "SB: Sent IDENTIFICATION message to the new client");
}

/****************************************************************************
 * ForceConnection callback for a client
 ****************************************************************************/
void onForceConnectionClient (TSockId from, uint32 userid)
{
	//sipinfo( "The client with uniq Id %u is connected(FORCESV)", userid ); byy krs

	// Add new client to the list of player managed by this FrontEnd
	pair<_pmap::iterator, bool> player = localPlayers.insert( make_pair( userid, CPlayer( userid, from, USERTYPE_COMMON, "Unknown", false )));

	// store the player info in appId
	_pmap::iterator it = player.first;
	CPlayer *p = &((*it).second);
	from->setAppId((uint64)(uint)p->id);

	// Send the message to connected client "from"
	// Clients->send( msgout, from );
	beep(2000, 500);

	//sipdebug( "SB: Sent FORCESV message to the new client"); byy krs
}

/****************************************************************************
 * Disonnection callback for a client
 ****************************************************************************/
void onDisconnectClient ( TSockId from, void *arg )
{
	uint32 id = (uint32)from->appId();

	if(id == 0)
		return;

	CKeyManager::getInstance().unregisterKey(from);

	ucstring	UserName = GetUserName(id);
	//sipinfo( L"A client(%s) has disconnected, SOCKET=%p", UserName.c_str(), from ); byy krs

	// tell the login system that this client is disconnected
	uint8	bIsGM = IsGM(id);
	CLoginServer::onClientDisconnected ( id, bIsGM );

	extern void	LogoutCharacter(TSockId _sock);
	LogoutCharacter(from);

	// remove the player from the local player list
	localPlayers.erase( id );

	DelUserToPacketFilter(id);
}

CVariable<bool> UseUDP("net", "UseUDP", "this flag is set wheather if we use UDP communication", false, 0, true);
CVariable<bool>	UseSecurity("security", "UseSecurity", "whether if we use secret algorithm, false : no use, true : use", false, 0, true);
CVariable<uint32>	PlayerLimit("ws","PlayerLimit", "Rough max number of players accepted on this shard (-1 for Unlimited)", 2000, 0, true );
bool	initClientServer(CInetAddress listenAddr)
{
	uint16 nPort = listenAddr.port();
	//sipinfo("FES Public Address	: %s", listenAddr.asIPString().c_str()); byy krs
	try
	{
        Clients = new CCallbackServer(UseUDP.get(), UseSecurity.get(), true, PlayerLimit.get());
		Clients->init(nPort);
		Clients->setDisconnectionCallback(onDisconnectClient, NULL);

		CLoginServer::init(Clients, listenAddr, onConnectionClient);
		CLoginServer::setForceConnectionCallback(Clients, onForceConnectionClient);

		if ( Clients == NULL )
		{
			sipinfo("SS Listening Port for Client opening Failed!");
			return false;
		}
	}
	catch (ESocket* e)
	{
		siperror("Server init Error, reason : %s", e->what());
		return false;
	}
	catch (ESecurity* e)
	{
		siperror("Security Server init Error, reason : %s", e->what());
		return false;
	}

	return true;
}

uint32	ClientRecvTime = 0, ClientSendTime = 0, ClientProcNum = 0;
uint32	AllRecvTime = 0, AllProcNum = 0;

uint16	GetUDPPeerPort()
{
	return UdpPeerPort.get();
}
/****************************************************************************
 * CFrontEndService
 ****************************************************************************/
class CFrontEndService : public IService
{
public:
	void preInit()
	{
		if (UdpPeerPort.get() == 0)
			UdpPeerPort.set(FSPort.get());

		TRegPortItem	PortAry[] =
		{
			{"TIANGUO_FSPort",		PORT_TCP, FSPort.get(),			FILTER_NONE},
			{"TIANGUO_UDPPeerPort",	PORT_UDP, UdpPeerPort.get(),	FILTER_NONE},
		};

		setRegPortAry(PortAry, sizeof(PortAry)/sizeof(PortAry[0]));
	}

	// Initialisation
	void init()
	{
		// Create the server where the client must connect into
		// In a real game, it should be an UDP server with specific protocol to manage packet lost and so on.
		CInetAddress	la = CInetAddress::localHost();
		if (IService::getInstance()->haveArg('D'))
			la = IService::getInstance()->getArg('D');
		else if (IService::getInstance()->ConfigFile.exists ("FSHost"))
			la = IService::getInstance()->ConfigFile.getVar ("FSHost").asString();
		la.setPort(FSPort.get());

		if (!CInetAddress::isLocalHostIP(la.ipAddress()))
		{
			sipwarning("This computer have not such ip(%s) for client.", la.ipAddress().c_str());
//			siperrornoex("This computer have not such ip(%s) for client.", la.ipAddress().c_str());
//			return;
		}

		initClientServer(la);

		extern void	InitShardClient();
		InitShardClient();

		string strRelayHost = GET_FROM_SERVICECONFIG( "RelayHost", "localhost:9600", asString);
		UDPHolePunchingServer = new CUdpHolePunchingServer();
		extern	T_FAMILYID	GetFamilyID(SIPNET::TTcpSockId _sid);
		UDPHolePunchingServer->Init(Clients, UdpPeerPort.get(), strRelayHost, GetFamilyID);
	}

	bool update ()
	{
		TTime t0 = CTime::getLocalTime();

		// Manage messages from clients
		Clients->CCallbackServer::update2 (UserUpdateTime, MinUserUpdateTime, &ClientRecvTime, &ClientSendTime, &ClientProcNum);

		CLoginServer::refreshPendingList(); // KSR

		extern	void	LogUnlawAction();
		LogUnlawAction();

		extern	void	SendWorldChat();
		SendWorldChat();
		return true;
	}

	void release()
	{
		if (Clients != NULL)
			delete Clients;
		Clients = NULL;
		
		delete UDPHolePunchingServer;
		UDPHolePunchingServer = NULL;
	}
};

/****************************************************************************   
 * SNOWBALLS FRONTEND SERVICE MAIN Function
 *
 * This call create a main function for a service:
 *
 *    - based on the "CFrontEndService" class
 *    - having the short name "FS"
 *    - having the long name "frontend_service"
 *    - listening on the port "0" (dynamically determined)
 *    - and shard callback set to "CallbackArray"
 *
 ****************************************************************************/
SIPNET_SERVICE_MAIN (CFrontEndService, FRONTEND_SHORT_NAME, FRONTEND_LONG_NAME, "", 0, false, EmptyCallbackArray, "", "")

SIPBASE_CATEGORISED_DYNVARIABLE(FES, uint64, ClientReceivedBytes, "total of bytes received by this FES service")
{
	if (get)
	{
		*pointer = Clients->getBytesReceived();
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(FES, uint64, ClientSentBytes, "total of bytes sent by this FES service")
{
	if (get)
	{
		*pointer = Clients->getBytesSent();
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(FES, uint32, OnlineUsersNumber, "number of connected users on this FES")
{
	// we can only read the value
	if (get)
	{
		uint32 nbusers = localPlayers.size();
		*pointer = nbusers;
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(FES, uint64, ProcedMessageNumSum, "number of processed message")
{
	if (get)
	{
		*pointer = Clients->getProcMessageTotalNum();
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(FES, uint32, ProcClientMessagePerMS, "number of processing message per millisecond")
{
	// we can only read the value
	if (get)
	{
		*pointer = (uint32)Clients->getProcMessageNumPerMS();
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(FES, uint64, ClientsRecvBufferSumKB, "sum of all clients recv buffer")
{
	// we can only read the value
	if (get)
	{
		uint64	v = Clients->getReceiveQueueSize();
		*pointer = (v >> 10);
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(FES, uint64, ClientsSendBufferSumKB, "sum of all clients send buffer")
{
	// we can only read the value
	if (get)
	{
		uint64 v = Clients->getSendQueueSize();
		*pointer = (v >> 10);
	}
}

SIPBASE_CATEGORISED_COMMAND(FES, displayClientSendQueueStat, "display client's send queue state", "")
{
	Clients->displaySendQueueStat(&log);
	return true;
}

SIPBASE_CATEGORISED_COMMAND(FES, displayClientRecvQueueStat, "display client's recv queue state", "")
{
	Clients->displayReceiveQueueStat(&log);
	return true;
}

SIPBASE_CATEGORISED_COMMAND(FES, clientUpdateTime, "", "")
{
	log.displayNL ("RecvTime=%d, SendTime=%d, ProcNum=%d", ClientRecvTime, ClientSendTime, ClientProcNum);
	return true;
}

SIPBASE_CATEGORISED_COMMAND(FES, LockFS, "Lock FS services. (Disconnect all users and users can't connect to FS.)", "")
{
	FSLockFlag = 1;

	void DisconnectAllClients();
	DisconnectAllClients();

	return true;
}

SIPBASE_CATEGORISED_COMMAND(FES, UnlockFS, "Unlock FS services.", "")
{
	FSLockFlag = 0;
	return true;
}

/* end of file */
