#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <misc/stream.h>
#include <net/service.h>
#include <net/SecurityServer.h>
#include "net/UdpPeerBroker.h"

#include <map>
#include <utility>

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"
#include "../Common/DBLog.h"
#include "../Common/netCommon.h"

#include "MultiService.h"
#include "main.h"
#include "shard_client.h"
#include "WorldChatManaget.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

CVariable<uint32>	MAX_USERLIST_COUNT("FES", "MAX_USERLIST_COUNT", "Max Userlist Count", 50, 0, true);

#define		NONE_NUMBER				0
#define		NONE_NVARCHAR			L""

extern	void	InitFilter();
extern	bool	ProcPacketFilter(MsgNameType _sPacket, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern	void	ProcResponse(MsgNameType _sPacket, uint32 _uAccountID);
extern	bool	CheckIfSilencePlayer(T_FAMILYID FID, bool bSendInfo);
void	CommonCheckForChat(CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb);


void	cbC_toLobby( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );

// Lobby
static	TServiceId	s_LobbyManagerId(0);
static  uint32 IndexForLobbySId = 0;
TLobbys	s_Lobbys;
TRooms	RoomsInfo;
TLobbySIDs s_lobbySID;

// Log
void	DBLOG_ShardLogin(T_FAMILYID _FID, T_FAMILYNAME _FName, T_ACCOUNTID _UserId, ucstring _UserName)
{
	uint32				MainType = NONE_NUMBER, SubType = NONE_NUMBER;
	uint32				PID = NONE_NUMBER;
	uint32				UID = NONE_NUMBER;
	uint32				ItemID = NONE_NUMBER;
	uint32				ItemTypeID = NONE_NUMBER;
	sint32				VI1 = NONE_NUMBER, VI2 = NONE_NUMBER, VI3 = NONE_NUMBER, VI4  =NONE_NUMBER;
	ucstring			PName = NONE_NVARCHAR;
	ucstring			ItemName = NONE_NVARCHAR;

	MainType = DBLOG_MAINTYPE_SHARDLOGIN;
	PID = _FID;
	UID = _UserId;
	PName = _FName;
	ItemName = _UserName;

	CMessage			mesLog(M_CS_SHARDLOG);
	mesLog.serial(MainType, SubType, PID, UID, ItemID, ItemTypeID);
	mesLog.serial(VI1, VI2, VI3, VI4, PName, ItemName);
	CUnifiedNetwork::getInstance ()->send( LOG_SHORT_NAME, mesLog );
}

void	DBLOG_ShardLogout(T_FAMILYID _FID, T_FAMILYNAME _FName, T_ACCOUNTID _UserId, ucstring _UserName)
{
	uint32				MainType = NONE_NUMBER, SubType = NONE_NUMBER;
	uint32				PID = NONE_NUMBER;
	uint32				UID = NONE_NUMBER;
	uint32				ItemID = NONE_NUMBER;
	uint32				ItemTypeID = NONE_NUMBER;
	sint32				VI1 = NONE_NUMBER, VI2 = NONE_NUMBER, VI3 = NONE_NUMBER, VI4  =NONE_NUMBER;
	ucstring			PName = NONE_NVARCHAR;
	ucstring			ItemName = NONE_NVARCHAR;

	MainType = DBLOG_MAINTYPE_SHARDLOGOUT;
	PID = _FID;
	UID = _UserId;
	PName = _FName;
	ItemName = _UserName;

	CMessage			mesLog(M_CS_SHARDLOG);
	mesLog.serial(MainType, SubType, PID, UID, ItemID, ItemTypeID);
	mesLog.serial(VI1, VI2, VI3, VI4, PName, ItemName);
	CUnifiedNetwork::getInstance ()->send( LOG_SHORT_NAME, mesLog );
}

CTwinMap<T_FAMILYID, TTcpSockId>	twinmapFIDandSID;		// Current FS Users
map<T_FAMILYID, T_FAMILYNAME>		mapFamilyName;	// Current FS Users
map<T_FAMILYID, TTcpSockId>			mapGMFamily;
//---map<T_FAMILYNAME, T_FAMILYID>		mapFamilyFromName;

// Get family id from socket id
T_FAMILYID	GetFamilyID(TTcpSockId _sid)
{
	T_FAMILYID	*uR = (T_FAMILYID *)(twinmapFIDandSID.getA(_sid));
	if (uR == NULL)
		return 0;
	return *uR;
}

// Get socket id from family id
TTcpSockId	GetSocketIDForFamily(T_FAMILYID _uid)
{
	TTcpSockId *uR = (TTcpSockId *)(twinmapFIDandSID.getB(_uid));
	if (uR == NULL)
		return TTcpSockId(0);
	return *uR;
}

map<T_FAMILYID,		FAMILYIINFO>		mapAllOnlineFamilyForFID;		// Onlined All family info
map<T_ACCOUNTID,	T_FAMILYID>		mapAllOnlineFIDForUserID;		// Onlined All family info
//uint32		g_LastFIDForUserList = 0;	// for M_SC_USERLIST
//---map<T_FAMILYNAME,	FAMILYIINFO>		mapAllOnlineFamilyForFName;		// Onlined All family info
list<T_FAMILYID>		g_UserList;

void	SendToAllFamily(CMessage &msg)
{
	map<uint32, TTcpSockId> chMap = twinmapFIDandSID.getAToBMap();

	for (map<uint32, TTcpSockId>::iterator it = chMap.begin(); it != chMap.end (); it++)
	{
		TTcpSockId	sock = it->second;
		Clients->send(msg, sock);
	}
}

static	CWorldChatManager	WorldChatManager;
void	SendWorldChat()
{
	WORLDCHATITEM	curchat;
	if (!WorldChatManager.PopChat(&curchat))
		return;

	T_FAMILYID	fid = curchat.fid;
	ucstring	ucname = curchat.fname;
	ucstring	ucchat = curchat.chat;

	CMessage	msgOut(M_SC_CHATALL);
	msgOut.serial(fid, ucname, ucchat);
	SendToAllFamily(msgOut);
}

CMultiService<uint32>	OpenRoomService(OROOM_SHORT_NAME);
CMultiService<uint32>	LobbyService(LOBBY_SHORT_NAME);


// To his manager
void	cbC_toHisManager( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;

//	g_LastFIDForUserList = FID;

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);
	CUnifiedNetwork::getInstance ()->send( HISMANAGER_SHORT_NAME, msgOut );
}

// To manager service
void	cbC_toManagerService( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;

//	g_LastFIDForUserList = FID;

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);
	CUnifiedNetwork::getInstance ()->send(MANAGER_SHORT_NAME, msgOut);
}

// To WS
void	cbC_toWelcomeService( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;

//	g_LastFIDForUserList = FID;

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);
	CUnifiedNetwork::getInstance ()->send(WELCOME_SHORT_NAME, msgOut);
}

//void cbC_toHSFORURL( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
//{
//	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
//		return;
//
//	T_FAMILYID FID = GetFamilyID(_from);
//	if(FID == 0)
//		return;
//
//	// flag : 1 for request album or photo frame URL, 2 for request video URL, 
//	//        3 for request select Video playing in the video area, 4 for history management of memorial object
//	uint8 flag = 0;
//	_msgin.serial(flag);
//
//	CMessage	msgHis(M_CS_REQURL);
//	msgHis.serial(FID);
//	msgHis.serial(flag);
//
//	// if flag equals to 1, receive PhotoFrameId from Client
//	if (flag == 1)
//	{
//		uint32 PhotoFrameId = 0;
//		_msgin.serial(PhotoFrameId);
//		msgHis.serial(PhotoFrameId);
//	}	
//
//	CUnifiedNetwork::getInstance ()->send( HISMANAGER_SHORT_NAME, msgHis );
//}

//void cbC_toHSFORPHOTOURL( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
//{
//	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
//		return;
//
//	T_FAMILYID FID = GetFamilyID(_from);
//	if(FID == 0)
//		return;
//
//	CMessage msgHis(M_CS_REQPHOTOURL); 
//	msgHis.serial(FID);
//	CUnifiedNetwork::getInstance ()->send( HISMANAGER_SHORT_NAME, msgHis );
//}


// To All openroom service
void	cbC_toAllOpenRoom( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;

//	g_LastFIDForUserList = FID;

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);

	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgOut);
}

// To openroom service
void	cbC_toOpenRoom( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;
	TServiceId	sid = OpenRoomService.GetSIDFromFID(FID);
	if (sid.get() == 0)
		sid = OpenRoomService.GetBestServiceID();

	if (sid.get() == 0)
		return;

//	g_LastFIDForUserList = FID;

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);
	CUnifiedNetwork::getInstance ()->send( sid, msgOut );
}

void cbC_toSpecService(CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb)
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;

//	g_LastFIDForUserList = FID;

	uint16	r_sid;
	_msgin.serial(r_sid);

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);

	TServiceId sid(r_sid);
	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

// To openroom service on room id insdie packet
void cbC_toOpenRoomForRoomID( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
	{
		return;
	}

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
	{
		return;
	}

	T_ROOMID	RoomID;
	_msgin.serial(RoomID);

	TServiceId sid;
	TRooms::iterator	it = RoomsInfo.find(RoomID);
	if(it != RoomsInfo.end())
	{
		sid = it->second;
	}
	else
	{
		sid = OpenRoomService.GetNewServiceIDForRoomID(RoomID);
		if (sid.get() == 0)
		{
			return;
		}
	}

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);
	CUnifiedNetwork::getInstance()->send( sid, msgOut );
}

// To any openroom service before valid family
void	cbC_toServerBeforeFID( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;
	
	uint32		uIdenID = GetIdentifiedID(_from);
	if (uIdenID == 0)
		return;

	TServiceId	sid = OpenRoomService.GetBestServiceID();
	if (sid.get() == 0)
		return;

	CMessage	msgOut;
	_msgin.copyWithPrefix(uIdenID, msgOut);
	CUnifiedNetwork::getInstance ()->send( sid, msgOut );
}

// To any openroom service after valid family
void	cbC_toServerAfterFID( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;
	
	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;

//	g_LastFIDForUserList = FID;

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);

	TServiceId	sid = OpenRoomService.GetBestServiceID();
	if (sid.get() == 0)
		CUnifiedNetwork::getInstance ()->send( RROOM_SHORT_NAME, msgOut );
	else
		CUnifiedNetwork::getInstance ()->send( sid, msgOut );
}

// To master opneroom
void	cbC_toMasterServer( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;

	TServiceId	sid = OpenRoomService.GetMasterServiceID();
	if (sid.get() == 0)
		return;

//	g_LastFIDForUserList = FID;

	CMessage	msgOut;
	_msgin.copyWithPrefix(FID, msgOut);
	CUnifiedNetwork::getInstance ()->send( sid, msgOut );
}

/************************************************************************/
/* Lobby                                                                */
/************************************************************************/
void	cbC_toLobbyManager( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
#ifdef SIP_MSG_NAME_DWORD
	sipdebug(L"Received packet=%d, time=%d", _msgin.getName(), CTime::getLocalTime());
#else
	sipdebug(L"Received packet=%s, time=%d", ucstring(_msgin.getName()).c_str(), CTime::getLocalTime());
#endif // SIP_MSG_NAME_DWORD

	if ( s_LobbyManagerId.get() == 0 )
	{
		sipwarning("Invalid Lobby manager service Id");
		return;
	}

	T_FAMILYID FID = GetFamilyID(_from);
	if ( FID == 0 )
		return;

//	g_LastFIDForUserList = FID;

	CMessage	msgout;
	_msgin.copyWithPrefix(FID, msgout);

	CUnifiedNetwork::getInstance()->send(s_LobbyManagerId, msgout);
}

//void	cbC_toLobbyWithLobbyId( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
//{
//	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
//		return;
//
//	T_FAMILYID FID = GetFamilyID(_from);
//	if ( FID == 0 )
//		return;
//
//	T_ROOMID	lobby_id;	_msgin.serial(lobby_id);
//	sipinfo("_msgin=%s, FID=%d, lobby_id=%d", _msgin.toString().c_str(), FID, lobby_id);
//
//	CMessage	msgout;
//	_msgin.copyWithPrefix(FID, msgout);
//
//	TServiceId sid(0);
//	if ( s_Lobbys.find(lobby_id) != s_Lobbys.end() )
//	{
//		sid = s_Lobbys[lobby_id];
//		CUnifiedNetwork::getInstance()->send(sid, msgout);
//	}
//	else
//	{
//		sipwarning("Can't process packet because TServiceId is NULL: lobby_id=%d", lobby_id);
//	}
//}

void	cbC_toLobby( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if ( FID == 0 )
		return;

//	g_LastFIDForUserList = FID;

	//map<T_FAMILYID,	FAMILYIINFO>::iterator itForRoom = mapAllOnlineFamilyForFID.find(FID);
	//if (itForRoom == mapAllOnlineFamilyForFID.end())
	//{
	//	sipwarning("No exist in online family list");
	//	return;
	//}

	/*if ( ! itForRoom->second.bInLobby )
	{
		sipwarning("bInLobby is false");
		return;
	}*/

	/*T_ROOMID	lobby_id = itForRoom->second.RoomID;
	sipinfo("_msgin=%s, FID=%d, lobby_id=%d", _msgin.toString().c_str(), FID, lobby_id);*/

	CMessage	msgout;
	_msgin.copyWithPrefix(FID, msgout);

	TLobbySIDs::iterator it = s_lobbySID.find(_from);
	if ( it != s_lobbySID.end() )
	{
		CUnifiedNetwork::getInstance()->send(it->second, msgout);
	}
	else
	{
		sipwarning("Can't process packet because TServiceId is NULL. FID=%d, PacketID=%d", FID, _msgin.getName());
	}
}

void	cbC_toOwnRoom( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if ( FID == 0 )
		return;

//	g_LastFIDForUserList = FID;

	map<T_FAMILYID,	FAMILYIINFO>::iterator itForRoom = mapAllOnlineFamilyForFID.find(FID);
	if (itForRoom == mapAllOnlineFamilyForFID.end())
	{
		sipwarning("No exist in online family list. FID=%d", FID);
		return;
	}

	if ( itForRoom->second.bInLobby )
	{
		CMessage	msgout;
		_msgin.copyWithPrefix(FID, msgout);

		TServiceId sid(0);
		if ( s_lobbySID.find(_from) != s_lobbySID.end() )
		{
			sid = s_lobbySID[_from];
			CUnifiedNetwork::getInstance()->send(sid, msgout);
		}
		else
		{
			sipwarning("Can't process packet because the lobby TServiceId is NULL: FID=%d, Packet=%d", FID, _msgin.getName());
		}
	}
	else
	{
		TServiceId	sid = OpenRoomService.GetSIDFromFID(FID);
		if (sid.get() != 0)
		{
			CMessage	msgOut;
			_msgin.copyWithPrefix(FID, msgOut);
			CUnifiedNetwork::getInstance ()->send( sid, msgOut );
		}
		else
		{
			sipwarning("Can't process packet because TServiceId is NULL: FID=%d, Packet=%d", FID, _msgin.getName());
		}
	}
}

void cbC_M_CS_ENTERROOM( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	T_FAMILYID FID = GetFamilyID(_from);
	if( FID == 0 )
		return;

	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(FID);
	if ( it == mapAllOnlineFamilyForFID.end() )
		return;

	if ( it->second.bInLobby )
	{
		sipwarning("FID is in Lobby! FID=%d", FID);	// SP\C0\C7 \C3\D6\C0\FBȭ\BF\CD \B0\FC\B7\C3\C7Ͽ\A9 M_CS_LEAVE_LOBBY\B8\A6 \BF÷\C1\BA\B8\B3\BD \B4\D9\C0\BD\BF\A1 M_CS_ENTERROOM\B8\A6 \BF÷\C1\BA\B8\B3\BB\B4°\CD\C0\B8\B7\CE \BC\F6\C1\A4
		CMessage msgout(M_NT_LEAVE_LOBBY);
		msgout.serial(FID);
		if ( s_lobbySID.find(_from) == s_lobbySID.end() )
		{
			"Can't process the message, because lobby service not found";
			return;
		}
		TServiceId	sid = s_lobbySID[_from];
		CUnifiedNetwork::getInstance()->send(sid, msgout);
		it->second.bInLobby = false;
	}
	cbC_toOpenRoomForRoomID(_msgin, _from, _clientcb);
}

// Get family info
void cbC_M_CS_FAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;
	
	uint32		uIdenID = GetIdentifiedID(_from);
	if (uIdenID == 0)
		return;

	TServiceId	sid = LobbyService.GetNextLobbySIDForFID(IndexForLobbySId);
	if (sid.get() == 0)
		return;

	IndexForLobbySId ++;
	s_lobbySID[_from] = sid;

	CInetAddress	addr = _clientcb.hostAddress(_from);
	std::string		strAddr = addr.ipAddress();

	CMessage	msgOut;
	_msgin.copyWithPrefix(uIdenID, msgOut);
	msgOut.serial(strAddr);

	bool		bGM	= IsGM(uIdenID);
	uint8		uUserType = GetUserType(uIdenID);
	ucstring	UserName = GetUserName(uIdenID);

	msgOut.serial(bGM, uUserType, UserName);

	CUnifiedNetwork::getInstance()->send( sid, msgOut );
}

// Create family
void cbC_M_CS_NEWFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;
	
	uint32		uIdenID = GetIdentifiedID(_from);
	if (uIdenID == 0)
		return;

	if ( s_lobbySID.find(_from) == s_lobbySID.end() )
		return;
	
	TServiceId	sid = s_lobbySID[_from];
	if (sid.get() == 0)
		return;

	CInetAddress	addr = _clientcb.hostAddress(_from);
	std::string		strAddr = addr.ipAddress();

	ucstring	UserName = GetUserName(uIdenID);
	CMessage	msgTemp;
	_msgin.copyWithPrefix(UserName, msgTemp);

	msgTemp.invert();
	CMessage	msgOut;
	msgTemp.copyWithPrefix(uIdenID, msgOut);
	msgOut.serial(strAddr);

	bool		bGM	= IsGM(uIdenID);
	uint8		uUserType = GetUserType(uIdenID);
	msgOut.serial(bGM, uUserType);

	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

// Get User List
void cbC_M_CS_USERLIST(CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb)
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	CMessage	msgOut(M_SC_USERLIST);
	uint32 count = 0;
	for (list<T_FAMILYID>::iterator it = g_UserList.begin(); it != g_UserList.end(); )
	{
		T_FAMILYID	FID = *it;
		map<T_FAMILYID,	FAMILYIINFO>::iterator itFamily = mapAllOnlineFamilyForFID.find(FID);
		if (itFamily == mapAllOnlineFamilyForFID.end())
		{
			it = g_UserList.erase(it);
		}
		else
		{
			if (!IsSystemAccount(itFamily->second.UserID))
			{
				msgOut.serial(FID, itFamily->second.FName, itFamily->second.bIsMobile, itFamily->second.ModelID);
				count ++;
				if (count >= MAX_USERLIST_COUNT)
					break;
			}
			it ++;
		}
	}

	Clients->send(msgOut, _from);
}

// Logout
void	cbC_M_CS_LOGOUT( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;
	_clientcb.disconnect(_from);
}

// Entire chatting
void cbC_M_CS_CHATALL( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	// System account can't do world chatting.
	T_ACCOUNTID			UserId = GetIdentifiedID(_from);
	if (IsSystemAccount(UserId))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;
	map<T_FAMILYID, T_FAMILYNAME>::iterator	it = mapFamilyName.find(FID);
	if (it == mapFamilyName.end())
		return;

//	g_LastFIDForUserList = FID;

	T_FAMILYNAME	ucName = it->second;
	T_CHATTEXT		ucChat;
	_msgin.serial(ucChat);
	if ( ucChat.size() > MAX_LEN_CHAT || ucChat.size() < 1 )
		return;

	WorldChatManager.AddChat(FID, ucName.c_str(), ucChat.c_str());

	CMessage	msgOut(M_SC_CHATALL);
	msgOut.serial(FID, ucName, ucChat);
//	Clients->send (msgOut, InvalidSockId);
	
	CUnifiedNetwork::getInstance()->send( FRONTEND_SHORT_NAME, msgOut, false );
}

//	{ M_CS_CHATROOM,			CommonCheckForChat}, // cbC_toOwnRoom },
//	{ M_CS_CHATCOMMON,			CommonCheckForChat}, // cbC_toOwnRoom },
//	{ M_CS_CHATEAR,				CommonCheckForChat}, // cbC_toServerAfterFID },	
//	{ M_CS_CHATALL,				CommonCheckForChat}, // cbC_M_CS_CHATALL },
void CommonCheckForChat( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	T_FAMILYID FID = GetFamilyID(_from);
	if(FID == 0)
		return;

	if (CheckIfSilencePlayer(FID, false))
	{
		sipinfo("Chat Blocked. MsgID=%d, FID=%d", _msgin.getName(), FID);
		return;
	}

	switch (_msgin.getName())
	{
	case M_CS_CHATROOM:		cbC_toOwnRoom(_msgin, _from, _clientcb);		break;
	case M_CS_CHATCOMMON:	cbC_toOwnRoom(_msgin, _from, _clientcb);		break;
	case M_CS_CHATEAR:		cbC_toServerAfterFID(_msgin, _from, _clientcb);	break;
	case M_CS_CHATALL:		cbC_M_CS_CHATALL(_msgin, _from, _clientcb);		break;
	}
}

// callback function of M_CS_LOBBY_ADDWISH, for wish in lobby from the Client
//void cbC_M_CS_LOBBY_ADDWISH( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
//{
//	T_FAMILYID FID = GetFamilyID(_from);
//
//	if (FID == 0)
//		return;
//
//	map<T_FAMILYID,	FAMILYIINFO>::iterator itForRoom = mapAllOnlineFamilyForFID.find(FID);
//	if (itForRoom == mapAllOnlineFamilyForFID.end())
//		return;
//	T_ROOMID	RoomID = itForRoom->second.RoomID;
//	if (RoomID == 0)
//		return;
//
//	if ( itForRoom->second.bInLobby )
//	{
//		T_ROOMID lobby_id = itForRoom->second.RoomID;
//		if ( s_Lobbys.find(lobby_id) == s_Lobbys.end() )
//		{
//			sipwarning("Can't process M_CS_STATECH because lobby service not found");
//			return;
//		}
//
//		TServiceId lobby_sid = s_Lobbys[lobby_id];
//		CMessage	msgout;
//		_msgin.copyWithPrefix(FID, msgout);
//		CUnifiedNetwork::getInstance()->send(lobby_sid, msgout);
//		return;
//	}
//}

//void cbC_M_CS_STATECH( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
//{
////	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
////		return;
//
//	T_FAMILYID FID = GetFamilyID(_from);
//
//	if (FID == 0)
//		return;
//
//	uint8	CHNum;			_msgin.serial(CHNum);
//	T_CHARACTERID CHID;		_msgin.serial(CHID);
//	T_CH_STATE State;		_msgin.serial(State);
//	T_CH_DIRECTION Dir;		_msgin.serial(Dir);
//	T_CH_X X;				_msgin.serial(X);
//	T_CH_Y Y;				_msgin.serial(Y);
//	T_CH_Z Z;				_msgin.serial(Z);
//
//	map<T_FAMILYID,	FAMILYIINFO>::iterator itForRoom = mapAllOnlineFamilyForFID.find(FID);
//	if (itForRoom == mapAllOnlineFamilyForFID.end())
//		return;
//
//	T_ROOMID	RoomID = itForRoom->second.RoomID;
//	if (RoomID == 0)
//		return;
//
//	/********************* old ***********************/
//
//	/*if ( itForRoom->second.bInLobby )
//	{
//		T_ROOMID lobby_id = itForRoom->second.RoomID;
//		if ( s_Lobbys.find(lobby_id) == s_Lobbys.end() )
//		{
//			sipwarning("Can't process M_CS_STATECH because lobby service not found");
//			return;
//		}
//
//		TServiceId lobby_sid = s_Lobbys[lobby_id];
//		CMessage	msgout;
//		_msgin.copyWithPrefix(FID, msgout);
//		CUnifiedNetwork::getInstance()->send(lobby_sid, msgout);
//		return;
//	}*/
//
//	CMessage	mesOut(M_SC_STATECH);
//	CHNum = 1;
//	mesOut.serial(RoomID, FID, CHNum, CHID, State, Dir, X, Y, Z);
//
////	uint32		uIdenID = GetIdentifiedID(_from);
////	if (uIdenID >=1302 && uIdenID <= 1801 )
////		return;
//
//	map<uint32, TTcpSockId> FamilyMap = mapFamily.getAToBMap();
//	for (map<uint32, TTcpSockId>::iterator it = FamilyMap.begin(); it != FamilyMap.end (); it++)
//	{
//		T_FAMILYID	CurFID = (*it).first;
//		TTcpSockId	sock = it->second;
//		map<T_FAMILYID,	FAMILYIINFO>::iterator it2 = mapAllOnlineFamilyForFID.find(CurFID);
//		if (it2 == mapAllOnlineFamilyForFID.end())
//			continue;
//
//		if (RoomID == it2->second.RoomID)
//			Clients->send (mesOut, sock);
//	}
//	
//	CMessage	mesNT(M_NT_STATECH);
//	mesNT.serial(RoomID, FID, CHNum, CHID, State, Dir, X, Y, Z);
//	CUnifiedNetwork::getInstance ()->send( FRONTEND_SHORT_NAME, mesNT, false );
//
//	TServiceId	sid = OpenRoomService.GetSIDFromFID(FID);
//	if (sid.get() == 0)
//		sid = OpenRoomService.GetBestServiceID();
//
//	if (sid.get() == 0)
//		return;
//	CMessage	msgOut;
//	_msgin.copyWithPrefix(FID, msgOut);
//	CUnifiedNetwork::getInstance ()->send( sid, msgOut );
//}

//void cbS_M_NT_STATECH(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_ROOMID RoomID;		_msgin.serial(RoomID);
//	T_FAMILYID FID;			_msgin.serial(FID);
//	uint8	FNum;			_msgin.serial(FNum);
//	T_CHARACTERID CHID;		_msgin.serial(CHID);
//	T_CH_STATE State;		_msgin.serial(State);
//	T_CH_DIRECTION Dir;		_msgin.serial(Dir);
//	T_CH_X X;				_msgin.serial(X);
//	T_CH_Y Y;				_msgin.serial(Y);
//	T_CH_Z Z;				_msgin.serial(Z);
//
//	CMessage	mesOut(M_SC_STATECH);
//	mesOut.serial(RoomID, FID, FNum, CHID, State, Dir, X, Y, Z);
//
//	map<uint32, TTcpSockId> FamilyMap = mapFamily.getAToBMap();
//	for (map<uint32, TTcpSockId>::iterator it = FamilyMap.begin(); it != FamilyMap.end (); it++)
//	{
//		T_FAMILYID	CurFID = (*it).first;
//		TTcpSockId	sock = it->second;
//		map<T_FAMILYID,	FAMILYIINFO>::iterator it2 = mapAllOnlineFamilyForFID.find(CurFID);
//		if (it2 == mapAllOnlineFamilyForFID.end())
//			continue;
//
//		if ( RoomID == it2->second.RoomID )
//			Clients->send (mesOut, sock);
//	}
//}

void cbC_M_CS_PROMOTION_ROOM( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	T_ROOMID RoomID;		_msgin.serial(RoomID);

	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);
	if ( FID == 0 )
		return;

	uint16	sid;
	TLobbySIDs::iterator	itLobby = s_lobbySID.find(_from);
	if ( itLobby == s_lobbySID.end() )
	{
		sipwarning("Can't process packet because TServiceId is NULL");
		return;
	}
	sid = itLobby->second.get();

	CMessage	msgout;
	_msgin.copyWithPrefix(FID, msgout);
	msgout.serial(sid);
	CUnifiedNetwork::getInstance()->send( MANAGER_SHORT_NAME, msgout );
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void cbC_toSelServiceByState( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	if (!ProcPacketFilter(_msgin.getName(), _from, _clientcb))
		return;

	T_FAMILYID FID = GetFamilyID(_from);

	if ( FID == 0 )
		return;

//	g_LastFIDForUserList = FID;

	map<T_FAMILYID,	FAMILYIINFO>::iterator itForRoom = mapAllOnlineFamilyForFID.find(FID);
	if ( itForRoom == mapAllOnlineFamilyForFID.end() )
		return;

	/*T_ROOMID	RoomID = itForRoom->second.RoomID;
	if ( RoomID == 0 )
		return;*/

	CMessage	msgout;
	_msgin.copyWithPrefix(FID, msgout);

	if ( itForRoom->second.bInLobby )
	{
		/*T_ROOMID lobby_id = itForRoom->second.RoomID;
		if ( s_Lobbys.find(lobby_id) == s_Lobbys.end() )
		{
			sipwarning("Can't process M_CS_STATECH because lobby service not found");
			return;
		}

		TServiceId lobby_sid = s_Lobbys[lobby_id];*/
		if ( s_lobbySID.find(_from) == s_lobbySID.end() )
		{
			sipwarning("Can't process the message, because lobby service not found");
			return;
		}
		TServiceId lobby_sid = s_lobbySID[_from];
		CUnifiedNetwork::getInstance()->send(lobby_sid, msgout);
		return;
	}
	else
	{
		TServiceId	sid = OpenRoomService.GetSIDFromFID(FID);
		if ( sid.get() == 0 )
			sid = OpenRoomService.GetBestServiceID();

		if ( sid.get() == 0 )
			return;
		CUnifiedNetwork::getInstance ()->send(sid, msgout);
	}
}

void cbS_toClientBeforeFID(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32		uIdenID;
	CMessage	msgOut;
	_msgin.copyWithoutPrefix(uIdenID, msgOut);

	TTcpSockId sid = GetSocketForIden(uIdenID);
	if (sid != 0)
	{
		ProcResponse(_msgin.getName(), uIdenID);
		Clients->send (msgOut, sid);
	}
}

void cbS_toClientAfterFID(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	CMessage	msgOut;
	_msgin.copyWithoutPrefix(FID, msgOut);

	TTcpSockId sid = GetSocketIDForFamily(FID);

	if (sid != 0)
	{
		uint32		uIdenID = GetIdentifiedID(sid);
		ProcResponse(_msgin.getName(), uIdenID);
		Clients->send (msgOut, sid);
	}
	else
	{
#ifdef SIP_MSG_NAME_DWORD
		sipdebug("Can't send packet=%d, because client is disconnected:FID=%d", _msgin.getName(), FID);
#else
		sipdebug("Can't send packet=%s, because client is disconnected:FID=%d", _msgin.getName().c_str(), FID);
#endif // SIP_MSG_NAME_DWORD
	}
}

// send URL to Client
//void cbS_toClientForURL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID FID = 0;
//	_msgin.serial(FID);
//    TTcpSockId sid = GetSocketIDForFamily(FID);
//
//	uint8 IsUrlSuccess = 0;
//	_msgin.serial(IsUrlSuccess);
//
//	CMessage msgout(M_SC_URL);
//	msgout.serial(IsUrlSuccess);
//
//	// if only IsUrlSuccess equals to 1, send URL to Client 
//	if (IsUrlSuccess == 1)
//	{
//		string strURL;
//		_msgin.serial(strURL);
//		msgout.serial(strURL);
//	}
//	
//	if (sid!=0)
//	{
//		uint32		uIdenID = GetIdentifiedID(sid);
//		ProcResponse(_msgin.getName(), uIdenID);
//		Clients->send(msgout, sid);                                                 
//	}
//
//}

void cbS_toSpecClient(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	CMessage	msgTemp, msgOut;
	T_FAMILYID	FID;

	msgTemp = _msgin;
	while(1)
	{
		msgTemp.copyWithoutPrefix(FID, msgOut);
		if(FID == 0)
			break;
		msgTemp = msgOut;
		msgTemp.invert();
	}

	while(1)
	{
		_msgin.serial(FID);
		if(FID == 0)
			break;
		TTcpSockId sid = GetSocketIDForFamily(FID);

		if (sid != 0)
		{
			uint32		uIdenID = GetIdentifiedID(sid);
			ProcResponse(_msgin.getName(), uIdenID);
			Clients->send (msgOut, sid);
		}
		else
		{
/*
#ifdef SIP_MSG_NAME_DWORD
			sipwarning("GetSocketIDForFamily = 0, Packet=%d, FID=%d", msgTemp.getName(), FID);
#else
			sipwarning("GetSocketIDForFamily = 0, Packet=%s, FID=%d", msgTemp.getName().c_str(), FID);
#endif // SIP_MSG_NAME_DWORD
*/
		}
	}
}

void cbS_M_SC_CHATALL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_FAMILYNAME	ucName;
	T_CHATTEXT		ucChat;
	_msgin.serial(FID, ucName, ucChat);

	WorldChatManager.AddChat(FID, ucName.c_str(), ucChat.c_str());
}


void cbS_toAll(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	CMessage	msgOut(_msgin);
	msgOut.setCryptMode(NON_CRYPT);
	msgOut.invert();
	SendToAllFamily(msgOut);
}

void cbS_toGMFamily(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	for (map<uint32, TTcpSockId>::iterator it = mapGMFamily.begin(); it != mapGMFamily.end (); it++)
	{
		TTcpSockId	sock = it->second;
		Clients->send(_msgin, sock);
	}
}

void onReconnectOpenRoom (const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	OpenRoomService.AddService(_tsid);
}

// \BE\EE\B4\C0 \C7\D1 OpenRoom\BD\E1\BA񽺰\A1 \B6\B3\BE\EE\C1\F8 \B0\E6\BF\EC
void onDisconnectOpenRoom(const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	// RoomsInfo\BF\A1 \B5\EE\B7ϵǿ\A9\C0ִ\C2 \B8\F1\B7\CF\C0\BB \BB\E8\C1\A6\C7Ѵ\D9.
	for(TRooms::iterator it = RoomsInfo.begin(); it != RoomsInfo.end(); )
	{
		if(it->second == _tsid)
			it = RoomsInfo.erase(it);
		else
			it ++;
	}

	// \C7\F6\C0\E7\C0\C7 FrontEnd\BF\A1 \BAپ\EE\C0ִ\C2 \BB\E7\BF\EB\C0ڸ\F1\B7Ͽ\A1\BC\AD \BB\E8\C1\A6\C7ϸ\E7, Ŭ\B6\F3\C0̾\F0Ʈ\B8\A6 \C0\FD\B4\DC\C7Ѵ\D9.
	std::vector<T_FAMILYID> FIDs;
	T_FAMILYID FID;
	for(map<T_FAMILYID, T_FAMILYNAME>::iterator it1 = mapFamilyName.begin(); it1 != mapFamilyName.end(); it1 ++)
	{
		FID = it1->first;
		if(OpenRoomService.GetSIDFromFID(FID) == _tsid)
			FIDs.push_back(FID);
	}
	for(std::vector<T_FAMILYID>::iterator it2 = FIDs.begin(); it2 != FIDs.end(); it2 ++)
	{
		DisconnectClientForFamilyID(*it2, L"Disconnected openroom");
	}

	// OpenRoomService \BF\A1\BC\AD \BB\E8\C1\A6\C7Ѵ\D9.
	OpenRoomService.RemoveService(_tsid);
}

void onDisconnectLobby (const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	// s_Lobbys\BF\A1 \B5\EE\B7ϵǿ\A9\C0ִ\C2 \B8\F1\B7\CF\C0\BB \BB\E8\C1\A6\C7Ѵ\D9.
	for(TLobbys::iterator it = s_Lobbys.begin(); it != s_Lobbys.end(); )
	{
		if(it->second == _tsid)
			it = s_Lobbys.erase(it);
		else
			it ++;
	}

	// \C7\F6\C0\E7\C0\C7 FrontEnd\BF\A1 \BAپ\EE\C0ִ\C2 \BB\E7\BF\EB\C0ڸ\F1\B7Ͽ\A1\BC\AD \BB\E8\C1\A6\C7Ѵ\D9.
	for(TLobbySIDs::iterator it1 = s_lobbySID.begin(); it1 != s_lobbySID.end(); )
	{
		if(it1->second == _tsid)
			it1 = s_lobbySID.erase(it1);
		else
			it1 ++;
	}

	LobbyService.RemoveLobbyService(_tsid);
}

void onReconnectLobby(const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	// old
	/*CMessage msgout(M_NT_QUERY_LOBBY);
	CUnifiedNetwork::getInstance ()->send(_tsid, msgout);*/

	LobbyService.AddLobbyService(_tsid);
}

void cbS_M_NTENTERROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		uFID;				_msgin.serial(uFID);
	T_ROOMID		uRoomID;			_msgin.serial(uRoomID);
	T_ROOMNAME		ucRoomName;			_msgin.serial(ucRoomName);
	T_ROOMCHANNELID	uRoomChannelID;		_msgin.serial(uRoomChannelID);

	OpenRoomService.AddFamilyWithLoad(_tsid, uFID, uRoomID);
	
	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(uFID);
	if (it == mapAllOnlineFamilyForFID.end())
		return;

	it->second.RoomID = uRoomID;
	it->second.RoomName = ucRoomName;
	it->second.RoomChannelID = uRoomChannelID;
	
//---	map<T_FAMILYNAME,	FAMILYIINFO>::iterator itName = mapAllOnlineFamilyForFName.find(FName);
//---	if (itName == mapAllOnlineFamilyForFName.end())
//---		return;

//---	itName->second.RoomID = uRoomID;
//---	itName->second.RoomName = ucRoomName;
}

//void cbS_M_NTCHANGEROOMCHANNEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		uFID;				_msgin.serial(uFID);
//	T_ROOMID		uRoomID;			_msgin.serial(uRoomID);
//	T_ROOMCHANNELID	uRoomChannelID;		_msgin.serial(uRoomChannelID);
//
//	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(uFID);
//	if (it == mapAllOnlineFamilyForFID.end())
//		return;
//
//	it->second.RoomChannelID = uRoomChannelID;
//}

void cbS_M_NTOUTROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	uFID;				_msgin.serial(uFID);
	uint32	uRoomID;			_msgin.serial(uRoomID);
	OpenRoomService.RemoveFamily(uFID);

	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(uFID);
	if (it == mapAllOnlineFamilyForFID.end())
		return;

	T_FAMILYNAME FName = it->second.FName;
	it->second.RoomID = 0;
	it->second.RoomName = L"";
	it->second.RoomChannelID = 0;

//---	map<T_FAMILYNAME,	FAMILYIINFO>::iterator itName = mapAllOnlineFamilyForFName.find(FName);
//---	if (itName == mapAllOnlineFamilyForFName.end())
//---		return;

//---	itName->second.RoomID = 0;
//---	itName->second.RoomName = L"";
}

void cbS_M_NT_CHANGE_NAME(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;				_msgin.serial(FID);
	ucstring		FamilyName;			_msgin.serial(FamilyName);

	map<T_FAMILYID, T_FAMILYNAME>::iterator it1 = mapFamilyName.find(FID);
	if(it1 != mapFamilyName.end())
		it1->second = FamilyName;

	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(FID);
	if (it != mapAllOnlineFamilyForFID.end())
	{
		it->second.FName = FamilyName;
	}
}

/************************************************************************/
/*		Lobby                                                           */
/************************************************************************/

void cbS_M_NT_LOBBY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	bool	bManager;	_msgin.serial(bManager);
	if ( bManager )
	{
		s_LobbyManagerId = _tsid;
		sipinfo("LobbyManager registered: service=%d", _tsid.get());
	}
	while ( (uint32)_msgin.getPos() < _msgin.length() )
	{
		uint32	lobby_id;	_msgin.serial(lobby_id);
		s_Lobbys[lobby_id] = _tsid;
		sipinfo("Lobby registered: id=%d, service=%d", lobby_id, _tsid.get());
	}
}

//void cbS_M_NT_NEW_LOBBY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_ROOMID	lobby_id;	_msgin.serial(lobby_id);
//	s_Lobbys[lobby_id] = _tsid;
//	sipinfo("new lobby: lobby_id=%d, _tsid=%d", lobby_id, _tsid.get());
//}

//void cbS_M_NT_DEL_LOBBY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_ROOMID	lobby_id;	_msgin.serial(lobby_id);
//	s_Lobbys.erase(lobby_id);
//	sipinfo("lobby deleted: lobby_id=%d", lobby_id);
//}

void cbS_M_SC_ENTER_LOBBY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	
	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(FID);
	if ( it == mapAllOnlineFamilyForFID.end() )
		return;
	it->second.bInLobby = true;

	CMessage	msgOut;
	_msgin.copyWithoutPrefix(FID, msgOut);

	TTcpSockId sid = GetSocketIDForFamily(FID);

	if (sid != 0)
	{
		uint32		uIdenID = GetIdentifiedID(sid);
		ProcResponse(_msgin.getName(), uIdenID);
		Clients->send (msgOut, sid);
	}
}

void cbS_M_SC_LEAVE_LOBBY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(FID);
	if ( it == mapAllOnlineFamilyForFID.end() )
		return;
	it->second.bInLobby = false;

	CMessage	msgOut;
	_msgin.copyWithoutPrefix(FID, msgOut);

	TTcpSockId sid = GetSocketIDForFamily(FID);

	if (sid != 0)
	{
		uint32		uIdenID = GetIdentifiedID(sid);
		ProcResponse(_msgin.getName(), uIdenID);
		Clients->send (msgOut, sid);
	}
}

//////////////////////////////////////////////////////////////////////////

void	LogoutCharacter(TTcpSockId _sock)
{
	uint32 uFID = GetFamilyID(_sock);
	if(uFID == 0)
		return;

	T_FAMILYNAME		FName;
	map<T_FAMILYID, T_FAMILYNAME>::iterator	it = mapFamilyName.find(uFID);
	if (it != mapFamilyName.end())
		FName = it->second;
	T_ACCOUNTID			UserId = GetIdentifiedID(_sock);
	ucstring			UserName = GetUserName(UserId);

	DBLOG_ShardLogout(uFID, FName, UserId, UserName);

	twinmapFIDandSID.removeWithA(uFID);
	mapFamilyName.erase(uFID);
	mapGMFamily.erase(uFID);

//---	mapFamilyFromName.erase(FName);	
	mapAllOnlineFamilyForFID.erase( uFID );
	mapAllOnlineFIDForUserID.erase( UserId );
//---	mapAllOnlineFamilyForFName.erase( FName );

	//TServiceId	sid = OpenRoomService.GetBestServiceID();
	//if (sid.get() == 0)
	//	return;

	//CMessage	msgOut(M_CS_LOGOUT);
	//msgOut.serial(uFID);
	//CUnifiedNetwork::getInstance ()->send( sid, msgOut );

	CMessage	msgOutNT(M_NT_LOGOUT);
	msgOutNT.serial(uFID);

	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME,	msgOutNT, false);
	CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME,	msgOutNT, false);
	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME,	msgOutNT, false);
	CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME,	msgOutNT, false);
	CUnifiedNetwork::getInstance()->send(RELAY_SHORT_NAME, msgOutNT, false);

	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOutNT, false);

	// sipdebug("mapAllOnlineFamilyForFID Deleted. size=%d", mapAllOnlineFamilyForFID.size());
}

void	DisconnectClientForUserID(uint32 _uid)
{
	TTcpSockId sid = GetSocketForIden(_uid);
	if (sid != 0)
	{
		Clients->disconnect(sid);
		ucstring	UserName = GetUserName(_uid);
		sipwarning(L"Client(%s) is closed by force of DisconnectClientForUserID.", UserName.c_str());
	}
}

void	DisconnectClientForFamilyID(T_FAMILYID _fid, ucstring _ucReason)
{
	TTcpSockId sid = GetSocketIDForFamily(_fid);
	if (sid != 0)
	{
		Clients->disconnect(sid);
		uint32		UserID = GetIdentifiedID(sid);
		ucstring	UserName = GetUserName(UserID);
		sipwarning(L"Client(%s) is closed by force of DisconnectClientForFamilyID. Reason=%s", UserName.c_str(), _ucReason.c_str());
	}
}

void DisconnectAllClients()
{
	const std::map<TTcpSockId, T_FAMILYID>	&data = twinmapFIDandSID.getBToAMap();
	for (std::map<TTcpSockId, T_FAMILYID>::const_iterator it = data.begin(); it != data.end(); it ++)
	{
		Clients->disconnect(it->first);

		sipinfo(L"Client(FID=%d) is closed by force of DisconnectAllClients.", it->second);
	}
}

void cbS_M_SC_FAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ACCOUNTID	uIdenID;
	CMessage	msgOut;
	_msgin.copyWithoutPrefix(uIdenID, msgOut);
	TTcpSockId sid = GetSocketForIden(uIdenID);

	if (sid == 0)
	{
		bool	bSystemUser;	_msgin.serial(bSystemUser);
		T_FAMILYID		fid;	_msgin.serial(fid);
		CMessage	msgOut(M_NT_LOGOUT);
		msgOut.serial(fid);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		return;
	}

	ProcResponse(_msgin.getName(), uIdenID);
	Clients->send(msgOut, sid);

	CInetAddress	addr = Clients->hostAddress(sid);
	std::string		strAddr = addr.ipAddress();

	bool	bSystemUser;	_msgin.serial(bSystemUser);
	T_FAMILYID		fid;	_msgin.serial(fid);
	if (fid != NOFAMILYID)
	{
		T_FAMILYNAME	fname;	_msgin.serial(fname);
		uint8			bIsMobile;	_msgin.serial(bIsMobile);
		uint32			ModelID;	_msgin.serial(ModelID);

		TTcpSockId sid2 = GetSocketIDForFamily(fid);
		if (sid2 == 0)
		{
			twinmapFIDandSID.add(fid, sid);
			mapFamilyName.insert( make_pair(fid, fname) );
//---			mapFamilyFromName.insert( make_pair(fname, fid) );
			
			ucstring	ucName = GetUserName(uIdenID);
			bool		bGM	= IsGM(uIdenID);
			uint8		uUserType = GetUserType(uIdenID);
			if (bGM)
			{
				mapGMFamily.insert(make_pair(fid, sid));
			}

			uint16	RoomSID = _tsid.get();
			CMessage	msgOut(M_NT_LOGIN);
			msgOut.serial(fid, uIdenID, fname, ucName, bIsMobile, ModelID, uUserType, bGM);
			msgOut.serial(strAddr, RoomSID);
			CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
			CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgOut, false);
			CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, msgOut, false);
			CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, msgOut, false);
			CUnifiedNetwork::getInstance()->send(RELAY_SHORT_NAME, msgOut, false);

			mapAllOnlineFamilyForFID.insert( make_pair(fid, FAMILYIINFO(fid, fname, uIdenID, ucName, bIsMobile, ModelID)) );
			mapAllOnlineFIDForUserID.insert( make_pair(uIdenID, fid) );

			// g_UserList\B8\F1\B7\CF\C1߿\A1 \B0\E3ġ\B4°\CD\C0\BB \BB\E8\C1\A6\C7Ѵ\D9.
			for (list<T_FAMILYID>::iterator itUserList = g_UserList.begin(); itUserList != g_UserList.end(); itUserList ++)
			{
				if ((*itUserList) == fid)
				{
					g_UserList.erase(itUserList);
					break;
				}
			}
			g_UserList.push_front(fid);
//---			mapAllOnlineFamilyForFName.insert( make_pair(fname, FAMILYIINFO(fid, fname, uIdenID, ucName)) );
			CUnifiedNetwork::getInstance ()->send(FRONTEND_SHORT_NAME, msgOut, false);

			CheckIfSilencePlayer(fid, true);

			DBLOG_ShardLogin(fid, fname, uIdenID, ucName);

			//sipdebug(L"Add Family:FID=%d, UserID=%d, FName=%s, UserName=%s, IPAddress=%S, bIsMobile=%d", fid, uIdenID, fname.c_str(), ucName.c_str(), strAddr.c_str(), bIsMobile); byy krs
		}
	}
}

void cbS_M_NT_LOGIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ACCOUNTID		UserID;
	T_FAMILYNAME	FName;
	T_COMMON_NAME	UserName;
	uint8			bIsMobile;
	uint32			ModelID;

	_msgin.serial(FID, UserID, FName, UserName, bIsMobile, ModelID);

	mapAllOnlineFamilyForFID.insert( make_pair(FID, FAMILYIINFO(FID, FName, UserID, UserName, bIsMobile, ModelID)) );
	mapAllOnlineFIDForUserID.insert( make_pair(UserID, FID) );
//---	mapAllOnlineFamilyForFName.insert( make_pair(FName, FAMILYIINFO(FID, FName, UserID, UserName)) );

	// sipdebug("mapAllOnlineFamilyForFID Added. size=%d", mapAllOnlineFamilyForFID.size());
}

void cbS_M_NT_LOGOUT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	_msgin.serial(FID);

	map<T_FAMILYID,		FAMILYIINFO>::iterator	it = mapAllOnlineFamilyForFID.find(FID);
	if (it == mapAllOnlineFamilyForFID.end())
		return;

//	T_FAMILYNAME		FName = it->second.FName;
	T_ACCOUNTID		userId = it->second.UserID;

	mapAllOnlineFamilyForFID.erase( FID );
	mapAllOnlineFIDForUserID.erase( userId );
//---	mapAllOnlineFamilyForFName.erase( FName );

	// sipdebug("mapAllOnlineFamilyForFID Deleted. size=%d", mapAllOnlineFamilyForFID.size());
}

void cbS_M_NT_ROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID		RoomID;
	uint8			flag;
	_msgin.serial(RoomID, flag);

	if(flag == 1)
	{
		// Room Created
		RoomsInfo[RoomID] = _tsid;

		// sipdebug("Room Created. RoomID=%d, OpenRoomSID=%d", RoomID, _tsid.get()); byy krs
	}
	else
	{
		// Room Deleted
		RoomsInfo.erase(RoomID);

		// sipdebug("Room Deleted. RoomID=%d, OpenRoomSID=%d", RoomID, _tsid.get()); byy krs
	}
}

void cb_M_SC_DISCONNECT_OTHERLOGIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	UserID;		_msgin.serial(UserID);

	_pmap::iterator ItPlayer;
	ItPlayer = localPlayers.find(UserID);
	if ( ItPlayer == localPlayers.end() )
		return;

	CMessage	msgOut(M_SC_DISCONNECT_OTHERLOGIN);
	Clients->send(msgOut, ItPlayer->second.con);
}

void cbS_PrcoPacket(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	MsgNameType		Packet;
	_msgin.serial(FID, Packet);

	TTcpSockId sid = GetSocketIDForFamily(FID);
	if (sid != 0)
	{
		uint32		uIdenID = GetIdentifiedID(sid);
		ProcResponse(Packet, uIdenID);
	}
}

void cbS_M_FORCEDISCONNECT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	ucstring		ucReason;
	_msgin.serial(FID, ucReason);
	DisconnectClientForFamilyID(FID, ucReason);
}

// Send room's luck to all (type3 & type4) ([d:FID][u:FName][d:LuckID][d:RoomID][u:RoomName][w:SceneID])
void cbS_M_SC_LUCK_TOALL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	ucstring	FamilyName;	_msgin.serial(FamilyName);
	uint32		LuckID;		_msgin.serial(LuckID);
	T_ROOMID	RoomID;		_msgin.serial(RoomID);
	ucstring	RoomName;	_msgin.serial(RoomName);
	uint16		SceneID;	_msgin.serial(SceneID);
	T_FAMILYID	MasterFID;	_msgin.serial(MasterFID);
	ucstring	MasterFName;_msgin.serial(MasterFName);

	CMessage	msgOut(M_SC_LUCK);
	msgOut.serial(FID, FamilyName, LuckID, RoomID, RoomName, SceneID, MasterFID, MasterFName);
	SendToAllFamily(msgOut);
}

// Notify that Luck4 list reseted in 24:00:00
void cbS_M_NT_LUCK4_RESET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	CMessage	msgOut(M_SC_LUCK4_LIST);
	SendToAllFamily(msgOut);
}

void cbS_M_NT_CURLUCKINPUBROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32			LuckID;			_msgin.serial(LuckID);
	T_FAMILYID		FamilyID;		_msgin.serial(FamilyID);
	T_FAMILYNAME	FamilyName;		_msgin.serial(FamilyName);
	uint32			TimeM;			_msgin.serial(TimeM);
	uint8			TimeDeltaS;		_msgin.serial(TimeDeltaS);

	CMessage	msgOut(M_SC_CURLUCKINPUBROOM);
	msgOut.serial(LuckID, FamilyID, FamilyName, TimeM, TimeDeltaS);
	SendToAllFamily(msgOut);
}

// GM
extern void cbC_M_GM_NOTICE( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_M_GMCS_WHEREFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_M_GMCS_PULLFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_M_GMCS_GETFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbS_M_GMSC_GETFAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
extern void cbC_M_GMCS_LOGOFFFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbS_M_GMCS_LOGOFFFAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
extern void cbC_M_GMCS_OUTFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_M_GMCS_ACTIVE( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_M_GMCS_SHOW( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_M_GMCS_SETLEVEL( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_M_GMCS_LISTCHANNEL( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_GeneralGMPacket_ForBestService( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_GeneralGMPacket_ForFamily( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_GeneralGMPacket_ForMasterService( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_GeneralGMPacket_ForManagerService( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );
extern void cbC_GeneralGMPacket_ForOpenRoomForRoomID( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );

extern void cbC_WebGMPacket_ForUser( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb );

extern void cbS_M_NT_SET_DBTIME(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
extern void cbS_M_NT_SILENCE_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);


//////////////////////////////////////////////////////////////////////////

// Callback from client
TCallbackItem ClientCallbackArrayForShard[] = 
{
	//{ M_CS_TOPLIST,				cbC_toManagerService },
	//{ M_CS_EXPROOMS,			cbC_toManagerService },
	{ M_GMCS_EXPROOM_GROUP_ADD,	cbC_GeneralGMPacket_ForManagerService },
	{ M_GMCS_EXPROOM_GROUP_MODIFY,	cbC_GeneralGMPacket_ForManagerService },
	{ M_GMCS_EXPROOM_GROUP_DELETE,	cbC_GeneralGMPacket_ForManagerService },
	{ M_GMCS_EXPROOM_ADD,		cbC_GeneralGMPacket_ForManagerService },
	{ M_GMCS_EXPROOM_MODIFY,	cbC_GeneralGMPacket_ForManagerService },
	{ M_GMCS_EXPROOM_DELETE,	cbC_GeneralGMPacket_ForManagerService },

	//{ M_CS_BESTSELLITEM,		cbC_toManagerService },

	{ M_CS_BUYHISSPACE,			cbC_toOwnRoom },
	//{ M_CS_EXPERIENCE,			cbC_toServerAfterFID }, 
	{ M_CS_RESCHIT,				cbC_toWelcomeService }, // cbC_toSpecService },

	{ M_CS_NEWROOM,				cbC_toLobby },
	{ M_CS_ROOMCARD_NEW,		cbC_toLobby },

	{ M_CS_RENEWROOM,			cbC_toOwnRoom },
	//{ M_CS_ROOMCARD_RENEW,		cbC_toOwnRoom },

	//{ M_CS_PROLONG_PASS,		cbC_toOpenRoomForRoomID },

	{ M_CS_PROMOTION_ROOM,		cbC_M_CS_PROMOTION_ROOM },
	{ M_CS_ROOMCARD_PROMOTION,	cbC_M_CS_PROMOTION_ROOM },
	{ M_CS_GET_PROMOTION_PRICE,	cbC_toSelServiceByState },

	//{ M_CS_ROOMCARD_CHECK,		cbC_toOpenRoom },

	//{ M_CS_GETRMAN,				cbC_toServerAfterFID },

	//{ M_CS_CONTFAMILY,			cbC_toServerAfterFID },	
	//{ M_CS_ROOMFORCONTACT,		cbC_toServerAfterFID },
	
	{ M_CS_FPROPERTY_MONEY,		cbC_toOwnRoom },
	{ M_CS_INVENSYNC,			cbC_toOwnRoom },
	{ M_CS_BUYITEM,				cbC_toOwnRoom },
	//{ M_CS_USEITEM,				cbC_toOwnRoom },
	//{ M_CS_THROWITEM,			cbC_toOwnRoom },
	//{ M_CS_TIMEOUT_ITEM,		cbC_toOwnRoom },
	//{ M_CS_TRIMITEM,			cbC_toOwnRoom },
	//{ M_CS_FITEMPOS,			cbC_toOwnRoom },

	//{ M_CS_BANKITEM_LIST,		cbC_toSelServiceByState },
	//{ M_CS_BANKITEM_GET,		cbC_toOwnRoom },

	{ M_CS_FINDFAMILY,			cbC_toSelServiceByState },
	{ M_CS_FINDROOM,			cbC_toSelServiceByState },
	//{ M_CS_SETROOMPWD,          cbC_toSelServiceByState },
	{ M_CS_ROOMINFO,			cbC_toWelcomeService}, // cbC_toSelServiceByState },

	// Favorite room
	{ M_CS_INSFAVORROOM,		cbC_toWelcomeService}, // cbC_toManagerService },
	{ M_CS_DELFAVORROOM,		cbC_toWelcomeService}, // cbC_toManagerService },
	{ M_CS_MOVFAVORROOM,		cbC_toWelcomeService}, // cbC_toManagerService },
	{ M_CS_NEW_FAVORROOMGROUP,	cbC_toWelcomeService}, // cbC_toManagerService },
	{ M_CS_DEL_FAVORROOMGROUP,	cbC_toWelcomeService}, // cbC_toManagerService },
	{ M_CS_REN_FAVORROOMGROUP,	cbC_toWelcomeService}, // cbC_toManagerService },

	{ M_CS_REQ_FAVORROOM_INFO,	cbC_toWelcomeService },
	//{ M_CS_REQ_ROOMORDERINFO,	cbC_toManagerService },
	{ M_CS_ROOMORDERINFO,		cbC_toManagerService },

	// [CH desc]
	{ M_CS_CHANGE_CHDESC,		cbC_toSelServiceByState },	
	//{ M_CS_CHDESC,			cbC_toSelServiceByState },

	// festival remind
	{ M_CS_SET_FESTIVALALARM,   cbC_toSelServiceByState	},
	
	// festival
	{ M_CS_REGFESTIVAL,			cbC_toSelServiceByState },	{ M_CS_MODIFYFESTIVAL,		cbC_toSelServiceByState },
	{ M_CS_DELFESTIVAL,			cbC_toSelServiceByState },	

	// \BB\E7\BF\EB\C0\DA\C0\C7 \C7\FC\BB\F3\BB\E7\C1\F8
	{ M_CS_SET_FAMILY_FIGURE,		cbC_toManagerService },
	{ M_CS_GET_FAMILY_FIGURE,		cbC_toManagerService },
	//{ M_CS_SET_FAMILY_FACEMODEL,	cbC_toManagerService },
	//{ M_CS_GET_FAMILY_FACEMODEL,	cbC_toManagerService },

	// contact group and contact
	{ M_CS_ADDCONTGROUP,		cbC_toManagerService }, // cbC_toSelServiceByState },
	{ M_CS_DELCONTGROUP,		cbC_toManagerService }, // cbC_toSelServiceByState },
	{ M_CS_RENCONTGROUP,		cbC_toManagerService }, // cbC_toSelServiceByState },
	{ M_CS_ADDCONT,				cbC_toManagerService }, // cbC_toSelServiceByState },
	{ M_CS_DELCONT,				cbC_toManagerService }, // cbC_toSelServiceByState },
	{ M_CS_CHANGECONTGRP,		cbC_toManagerService }, // cbC_toSelServiceByState },	
	{ M_CS_SETCONTGROUP_INDEX,  cbC_toManagerService }, // cbC_toSelServiceByState },
	{ M_CS_SELFSTATE,			cbC_toManagerService }, // cbC_toServerAfterFID },	


	// Broker
//	{ M_CS_JOIN_SESSION,		cbC_toMasterServer },
//	{ M_CS_EXIT_SESSION,		cbC_toMasterServer },
//	{ M_CS_SPEAK_TO_SESSION,	cbC_toMasterServer },
	{ M_CS_REQUEST_TALK,		cbC_toMasterServer },
	{ M_CS_RESPONSE_TALK,		cbC_toMasterServer },
	{ M_CS_HANGUP_TALK,			cbC_toMasterServer },
	
	{ M_CS_OUTROOM,				cbC_toOpenRoom },

	{ M_CS_INVITE_REQ,			cbC_toManagerService }, // cbC_toOpenRoom },
	{ M_CS_INVITE_ANS,			cbC_toOpenRoomForRoomID },
	{ M_CS_BANISH,				cbC_toOpenRoom },

	{ M_CS_MANAGER_ADD,			cbC_toOpenRoomForRoomID },
	{ M_CS_MANAGER_ANS,			cbC_toOpenRoomForRoomID },
	{ M_CS_MANAGER_DEL,			cbC_toOpenRoomForRoomID },
	{ M_CS_MANAGER_GET,			cbC_toOpenRoomForRoomID },
	{ M_CS_MANAGEROOMS_GET,		cbC_toSelServiceByState },
	{ M_CS_MANAGER_SET_PERMISSION,	cbC_toOpenRoom },

	{ M_CS_CHATROOM,			CommonCheckForChat}, // cbC_toOwnRoom },
	{ M_CS_CHATCOMMON,			CommonCheckForChat}, // cbC_toOwnRoom },
	{ M_CS_CHATEAR,				CommonCheckForChat}, // cbC_toServerAfterFID },	
	{ M_CS_CHATALL,				CommonCheckForChat}, // cbC_M_CS_CHATALL },

	//{ M_CS_ROOMPARTCHANGE,		cbC_toOpenRoom },
	//{ M_CS_DELDEAD,				cbC_toOpenRoom },

	//{ M_CS_ADDRMAN,				cbC_toOpenRoom },
	//{ M_CS_DELRMAN,				cbC_toOpenRoom },		
	{ M_CS_TOMBSTONE,			cbC_toOpenRoom },
	{ M_CS_RUYIBEI_TEXT,		cbC_toOpenRoom },
	//{ M_CS_EVENT,				cbC_toOpenRoom },
	//{ M_CS_ADDEVENT,			cbC_toOpenRoom },
	//{ M_CS_ENDEVENT,			cbC_toOpenRoom },		
	//{ M_CS_HOLDITEM,			cbC_toOpenRoom },
	//{ M_CS_PUTHOLEDITEM,		cbC_toOpenRoom },		

	{ M_CS_FISH_FOOD,			cbC_toOpenRoom },
	{ M_CS_FLOWER_NEW,			cbC_toOpenRoom },
	{ M_CS_FLOWER_WATER,		cbC_toOpenRoom },
	{ M_CS_FLOWER_END,			cbC_toOpenRoom },
	{ M_CS_FLOWER_NEW_REQ,		cbC_toOpenRoom },
	{ M_CS_FLOWER_WATER_REQ,	cbC_toOpenRoom },
	{ M_CS_GOLDFISH_CHANGE,		cbC_toOpenRoom },
	{ M_CS_FLOWER_WATERTIME_REQ,cbC_toOpenRoom },
	//{ M_CS_NEWPET,				cbC_toOpenRoom },
	//{ M_CS_CAREPET,				cbC_toOpenRoom },

	{ M_CS_LUCK_REQ,			cbC_toOpenRoom },
	//{ M_CS_LUCK4_LIST,			cbC_toManagerService },

	{ M_CS_DRUM_START,			cbC_toOwnRoom },
	{ M_CS_DRUM_END,			cbC_toOwnRoom },
	{ M_CS_BELL_START,			cbC_toOwnRoom },
	{ M_CS_BELL_END,			cbC_toOwnRoom },
	{ M_CS_MUSIC_START,			cbC_toOpenRoom },
	{ M_CS_MUSIC_END,			cbC_toOpenRoom },
	{ M_CS_SHENSHU_START,		cbC_toOwnRoom }, // by krs

	{ M_CS_ANIMAL_APPROACH,		cbC_toOpenRoom },
	{ M_CS_ANIMAL_STOP,			cbC_toOpenRoom },
	{ M_CS_ANIMAL_SELECT,		cbC_toOpenRoom },
	{ M_CS_ANIMAL_SELECT_END,	cbC_toOpenRoom },

	{ M_CS_EAINITINFO,			cbC_toOpenRoom },
	{ M_CS_EAMOVE,				cbC_toOpenRoom },
//	{ M_CS_EAANIMATION,			cbC_toOpenRoom },
	{ M_CS_EABASEINFO,			cbC_toOpenRoom },
	{ M_CS_EAANISELECT,			cbC_toOpenRoom },
	
	//{ M_CS_BURNITEM,			cbC_toOpenRoom },
	//{ M_CS_ROOMMUSIC,			cbC_toOpenRoom },		
	//{ M_CS_ENTERTGATE,			cbC_toOpenRoom },			
	//{ M_CS_PUTITEM,				cbC_toOpenRoom },
	//{ M_CS_ROOMFAMILYCOMMENT,	cbC_toOpenRoom },
	//{ M_CS_BURNITEMINS,			cbC_toOpenRoom },
	{ M_CS_NEWRMEMO,			cbC_toOpenRoom },
	//{ M_CS_REQRMEMO,			cbC_toOpenRoom },	
	//{ M_CS_NEWSACRIFICE,		cbC_toOpenRoom },
	//{ M_CS_MODIFYRMEMO,			cbC_toOpenRoom },
	//{ M_CS_DELRMEMO,			cbC_toOpenRoom },
	//{ M_CS_MODIFY_MEMO_START,	cbC_toOpenRoom },
	//{ M_CS_MODIFY_MEMO_END,		cbC_toOpenRoom },
	//{ M_CS_ROOMMEMO_GET,		cbC_toOpenRoom },	

	{ M_CS_ACT_REQ,				cbC_toOwnRoom },
	{ M_CS_ACT_WAIT_CANCEL,		cbC_toOwnRoom },
	{ M_CS_ACT_RELEASE,			cbC_toOwnRoom },
	{ M_CS_ACT_START,			cbC_toOwnRoom },
	{ M_CS_ACT_STEP,			cbC_toOwnRoom },

	{ M_CS_MULTIACT2_ITEMS,		cbC_toOwnRoom },
	{ M_CS_MULTIACT2_ASK,		cbC_toOwnRoom },
	{ M_CS_MULTIACT2_JOIN,		cbC_toOwnRoom },
	{ M_CS_MULTIACT_REQ,		cbC_toOwnRoom },
	{ M_CS_MULTIACT_ANS,		cbC_toOpenRoomForRoomID },
	{ M_CS_MULTIACT_READY,		cbC_toOwnRoom },
	{ M_CS_MULTIACT_GO,			cbC_toOwnRoom },
	{ M_CS_MULTIACT_START,		cbC_toOwnRoom },
	{ M_CS_MULTIACT_RELEASE,	cbC_toOwnRoom },
	{ M_CS_MULTIACT_CANCEL,		cbC_toOpenRoomForRoomID },
	{ M_CS_MULTIACT_COMMAND,	cbC_toOwnRoom },
	{ M_CS_MULTIACT_REPLY,		cbC_toOwnRoom },
	{ M_CS_MULTIACT_REQADD,		cbC_toOwnRoom },
	
	{ M_CS_CRACKER_BOMB,		cbC_toOwnRoom },
	{ M_CS_LANTERN,				cbC_toOwnRoom },
	{ M_CS_ITEM_MUSICITEM,		cbC_toOwnRoom },
	//{ M_CS_ITEM_FLUTES,			cbC_toOwnRoom },
	//{ M_CS_ITEM_VIOLIN,			cbC_toOwnRoom },
	//{ M_CS_ITEM_GUQIN,			cbC_toOwnRoom },
	//{ M_CS_ITEM_DRUM,			cbC_toOwnRoom },
	//{ M_CS_ITEM_FIRECRACK,		cbC_toOwnRoom },
	{ M_CS_ITEM_RAIN,			cbC_toOwnRoom },
	{ M_CS_ITEM_POINTCARD,		cbC_toOwnRoom },

	{ M_CS_MOUNT_LUCKANIMAL,	cbC_toOpenRoom },
	{ M_CS_UNMOUNT_LUCKANIMAL,	cbC_toOpenRoom },

	{ M_CS_REQRVISITLIST,		cbC_toOwnRoom },
	{ M_CS_ADDACTIONMEMO,		cbC_toOwnRoom },

	{ M_CS_VISIT_DETAIL_LIST,	cbC_toOwnRoom },
	{ M_CS_VISIT_DATA,			cbC_toOwnRoom },
	{ M_CS_VISIT_DATA_SHIELD,	cbC_toOwnRoom },
	{ M_CS_VISIT_DATA_DELETE,	cbC_toOwnRoom },
	{ M_CS_VISIT_DATA_MODIFY,	cbC_toOwnRoom },
	{ M_CS_GET_ROOM_VISIT_FID,	cbC_toOwnRoom },

	{ M_CS_REQ_PAPER_LIST,		cbC_toOwnRoom },
	{ M_CS_REQ_PAPER,			cbC_toOwnRoom },
	{ M_CS_NEW_PAPER,			cbC_toOwnRoom },
	{ M_CS_DEL_PAPER,			cbC_toOwnRoom },
	{ M_CS_MODIFY_PAPER,		cbC_toOwnRoom },
	{ M_CS_MODIFY_PAPER_START,	cbC_toOwnRoom },
	{ M_CS_MODIFY_PAPER_END,	cbC_toOwnRoom },

	// Mail
	{ M_CS_MAIL_GET_LIST,		cbC_toWelcomeService}, // cbC_toSelServiceByState },
	{ M_CS_MAIL_GET,			cbC_toWelcomeService}, // cbC_toSelServiceByState },
	{ M_CS_MAIL_SEND,			cbC_toOwnRoom },
	{ M_CS_MAIL_DELETE,			cbC_toWelcomeService}, // cbC_toSelServiceByState },
	{ M_CS_MAIL_REJECT,			cbC_toWelcomeService}, // cbC_toSelServiceByState },
	{ M_CS_MAIL_ACCEPT_ITEM,	cbC_toOwnRoom },

	{ M_CS_REQ_FAMILYCHS,		cbC_toSelServiceByState }, 
	//{ M_CS_FAMILYCOMMENT,		cbC_toSelServiceByState }, 
	{ M_CS_ROOMFORFAMILY,		cbC_toSelServiceByState },
	//{ M_CS_CHMETHOD,			cbC_toOwnRoom },
	{ M_CS_DECEASED,			cbC_toOpenRoomForRoomID },
	{ M_CS_SETDEAD,				cbC_toOpenRoomForRoomID },
	{ M_CS_SET_DEAD_FIGURE,		cbC_toOpenRoomForRoomID },

	//{ M_CS_GETALLINROOM,		cbC_toOpenRoomForRoomID },

	{ M_CS_SETROOMINFO,			cbC_toOpenRoomForRoomID },
	{ M_CS_CHANGEMASTER,		cbC_toManagerService }, // cbC_toOpenRoomForRoomID },
	{ M_CS_CHANGEMASTER_CANCEL,	cbC_toManagerService }, // cbC_toOpenRoomForRoomID },
	{ M_CS_CHANGEMASTER_ANS,	cbC_toManagerService }, // cbC_toOpenRoomForRoomID },

	{ M_CS_CHECK_ENTERROOM,		cbC_toManagerService},
	{ M_CS_REQEROOM,			cbC_toManagerService}, // cbC_toOpenRoomForRoomID },
	{ M_CS_CANCELREQEROOM,		cbC_toManagerService}, // cbC_toOpenRoomForRoomID },
	{ M_CS_CHANNELLIST,			cbC_toOpenRoomForRoomID },
	{ M_CS_ENTERROOM,			cbC_M_CS_ENTERROOM },
//	{ M_CS_CHANGECHANNELIN,		cbC_toOwnRoom },
//	{ M_CS_CHANGECHANNELOUT,	cbC_toOwnRoom },
	{ M_CS_DELROOM,				cbC_toOpenRoomForRoomID },
	{ M_CS_UNDELROOM,			cbC_toSelServiceByState },

	//{ M_CS_REQPORTRAIT,			cbC_toHisManager },
	//{ M_CS_REQFORURL,			cbC_toHSFORURL},
	
	{ M_CS_FAMILY,				cbC_M_CS_FAMILY },	
	{ M_CS_LOGOUT,				cbC_M_CS_LOGOUT },

	{ M_CS_STATECH,				cbC_toOwnRoom },
	{ M_CS_MOVECH,				cbC_toOwnRoom },
	{ M_CS_ACTION,				cbC_toOwnRoom },

	{ M_CS_CHAIR_SITDOWN,		cbC_toOwnRoom },
	{ M_CS_CHAIR_STANDUP,		cbC_toOwnRoom },

	//{ M_CS_LOBBY_ADDWISH,       cbC_M_CS_LOBBY_ADDWISH },

	{ M_CS_USERLIST,			cbC_M_CS_USERLIST },	

	//For GMTool
	{ M_GMCS_NOTICE,			cbC_M_GM_NOTICE },
	{ M_GMCS_WHEREFAMILY,		cbC_M_GMCS_WHEREFAMILY },
	{ M_GMCS_PULLFAMILY,		cbC_M_GMCS_PULLFAMILY },
	{ M_GMCS_GETFAMILY,			cbC_M_GMCS_GETFAMILY },
	{ M_GMCS_LOGOFFFAMILY,		cbC_M_GMCS_LOGOFFFAMILY },
	{ M_GMCS_OUTFAMILY,			cbC_M_GMCS_OUTFAMILY },
	{ M_GMCS_ACTIVE,			cbC_M_GMCS_ACTIVE },
	{ M_GMCS_SHOW,				cbC_M_GMCS_SHOW },
	{ M_GMCS_SETLEVEL,			cbC_M_GMCS_SETLEVEL },
	//{ M_GMCS_GIVEITEM,			cbC_GeneralGMPacket_ForFamily },
	//{ M_GMCS_GIVEMONEY,			cbC_GeneralGMPacket_ForFamily },
	{ M_GMCS_GETMONEY,			cbC_GeneralGMPacket_ForFamily	},
	{ M_GMCS_GETITEM,			cbC_GeneralGMPacket_ForFamily },
	{ M_GMCS_GETLEVEL,			cbC_GeneralGMPacket_ForFamily },
	{ M_GMCS_GETINVENEMPTY,		cbC_GeneralGMPacket_ForFamily },
	{ M_GMCS_LISTCHANNEL,		cbC_M_GMCS_LISTCHANNEL },
 	{ M_GMCS_GET_GMONEY,		cbC_WebGMPacket_ForUser	},

	//{ M_GMCS_MAIL_SEND,			cbC_GeneralGMPacket_ForFamily },
	{ M_GMCS_ZAIXIAN_ANSWER,	cbC_toManagerService },
	{ M_GMCS_ZAIXIAN_CONTROL,	cbC_toManagerService },

	{ M_GMCS_SILENCE_SET,		cbC_GeneralGMPacket_ForManagerService },

	// For Stress Test Tool
	//{ M_CS_FORCEFAMILY,			cbC_toServerBeforeFID },
	//{ M_CS_FORCESTATECH,		cbC_toServerAfterFID },	
	//{ M_CS_FORCEOUT,			cbC_toOpenRoomForRoomID },		
	
	// Lobby
	//{ M_CS_REQUEST_LOBBYS,		cbC_toLobbyManager },	
	//{ M_CS_CANCEL_WAITLOBBY,	cbC_toLobbyManager },
	//{ M_CS_REQ_ENTERLOBBY,		cbC_toLobby },
	{ M_CS_ENTER_LOBBY,			cbC_toLobby },
	{ M_CS_LEAVE_LOBBY,			cbC_toLobby },
	{ M_CS_NEWFAMILY,			cbC_M_CS_NEWFAMILY },	
	//{ M_CS_REQ_CHANGECH,		cbC_toLobby },	
	{ M_CS_CHCHARACTER,			cbC_toLobby },


	{ M_CS_HISMANAGE_START,		cbC_toHisManager },
	{ M_CS_HISMANAGE_END,		cbC_toHisManager },

	{ M_CS_FIGUREFRAME_SET,		cbC_toHisManager },

	// Upload/download/Delete
	{ M_CS_REQ_UPLOAD,			cbC_toHisManager },
	{ M_CS_UPLOAD_DONE,			cbC_toHisManager },
	{ M_CS_REQ_DOWNLOAD,		cbC_toHisManager },
	{ M_CS_REQ_DELETE,			cbC_toHisManager },
	{ M_CS_PLAY_START,			cbC_toHisManager },
	{ M_CS_PLAY_STOP,			cbC_toHisManager },

	// data
	{ M_CS_RENDATA,				cbC_toHisManager },
	{ M_CS_PHOTOALBUMLIST,		cbC_toHisManager },
	{ M_CS_VIDEOALBUMLIST,		cbC_toHisManager },
	{ M_CS_ADDALBUM,			cbC_toHisManager },
	{ M_CS_DELALBUM,			cbC_toHisManager },
	{ M_CS_RENALBUM,			cbC_toHisManager },
	{ M_CS_CHANGEALBUMINDEX,	cbC_toHisManager },
	{ M_CS_MOVDATA,				cbC_toHisManager },
	{ M_CS_CHANGEVIDEOINDEX,	cbC_toHisManager },
//---	{ M_CS_DELDATA,             cbC_toHisManager },
	{ M_CS_FRAMEDATA_SET,		cbC_toHisManager },
	{ M_CS_FRAMEDATA_GET,		cbC_toHisManager },
	{ M_CS_ALBUMINFO,			cbC_toHisManager },
	{ M_CS_REQDATA_PHOTO,		cbC_toHisManager },
	{ M_CS_REQDATA_VIDEO,		cbC_toHisManager },

	{ M_CS_ADDWISH,				cbC_toOwnRoom },
	{ M_CS_WISH_LIST,			cbC_toOwnRoom },
	{ M_CS_MY_WISH_LIST,		cbC_toOwnRoom },
	{ M_CS_MY_WISH_DATA,		cbC_toOwnRoom },

	//{ M_CS_R_PAPER_START,		cbC_toOwnRoom },
	//{ M_CS_R_PAPER_PLAY,		cbC_toOwnRoom },
	//{ M_CS_R_PAPER_END,			cbC_toOwnRoom },

	//{ M_GMCS_RECOMMEND_SET,		cbC_GeneralGMPacket_ForBestService },
	//{ M_CS_RECOMMEND_ROOM,		cbC_toOwnRoom },

	{ M_CS_VIRTUE_LIST,			cbC_toOwnRoom },
	{ M_CS_VIRTUE_MY,			cbC_toOwnRoom },

	{ M_CS_R_ACT_INSERT,		cbC_toOwnRoom },
	{ M_CS_R_ACT_MODIFY,		cbC_toOwnRoom },
	{ M_CS_R_ACT_LIST,			cbC_toOwnRoom },
	{ M_CS_R_ACT_MY_LIST,		cbC_toOwnRoom },
	{ M_CS_R_ACT_PRAY,			cbC_toOwnRoom },

	{ M_CS_ANCESTOR_TEXT_SET,	cbC_toOwnRoom },
	{ M_CS_ANCESTOR_DECEASED_SET,	cbC_toOwnRoom },

	{ M_CS_AUTOPLAY_REQ_BEGIN,	cbC_toOwnRoom },
	{ M_CS_AUTOPLAY_REQ,		cbC_toOwnRoom },

	{ M_CS_AUTOPLAY2_REQ,		cbC_toOwnRoom },

	{ M_CS_AUTOPLAY3_REGISTER,	cbC_toOwnRoom },

	{ M_CS_REQ_ACTIVITY_ITEM,	cbC_toOwnRoom}, // cbC_toManagerService },
	{ M_CS_RECV_GIFTITEM,		cbC_toOwnRoom}, // cbC_toManagerService },

	{ M_CS_GET_ROOMSTORE,			cbC_toOpenRoom },
	{ M_CS_ADD_ROOMSTORE,			cbC_toOpenRoom },
	{ M_CS_GET_ROOMSTORE_HISTORY,	cbC_toOpenRoom },
	{ M_CS_GET_ROOMSTORE_HISTORY_DETAIL,	cbC_toOpenRoom },
	{ M_CS_ROOMSTORE_LOCK,			cbC_toOpenRoom },
	{ M_CS_ROOMSTORE_UNLOCK,		cbC_toOpenRoom },
	{ M_CS_ROOMSTORE_LASTITEM_SET,	cbC_toOpenRoom },
	{ M_CS_ROOMSTORE_HISTORY_MODIFY,	cbC_toOpenRoom },
	{ M_CS_ROOMSTORE_HISTORY_DELETE,	cbC_toOpenRoom },
	{ M_CS_ROOMSTORE_HISTORY_SHIELD,	cbC_toOpenRoom },
	{ M_CS_ROOMSTORE_GETITEM,	cbC_toOpenRoom },

	// \C3ູī\B5\E5
	{ M_CS_BLESSCARD_GET,		cbC_toManagerService },
	{ M_CS_BLESSCARD_SEND,		cbC_toManagerService },

	{ M_CS_ZAIXIAN_REQUEST,		cbC_toManagerService },
	{ M_CS_ZAIXIAN_END,			cbC_toManagerService },
	{ M_CS_ZAIXIAN_ASK,			cbC_toManagerService },

	// \BB\E7\BF\EB\C8ı\E2
	//{ M_CS_REVIEW_LIST,			cbC_toManagerService },
	//{ M_GMCS_REVIEW_ADD,		cbC_GeneralGMPacket_ForManagerService },
	//{ M_GMCS_REVIEW_MODIFY,		cbC_GeneralGMPacket_ForManagerService },
	//{ M_GMCS_REVIEW_DELETE,		cbC_GeneralGMPacket_ForManagerService },

	// \B1\E2\B3\E4\B0\FC \B1\F4¦\C0̺\A5Ʈ \B0˻\E7
	{ M_GMCS_ROOMEVENT_ADD,		cbC_GeneralGMPacket_ForOpenRoomForRoomID },

	// \B0\F8\B0\F8\B1\B8\BF\AA\BF\A1\BC\AD Frame\BF\A1 \B0ɷ\C1\C0ִ\C2 ü\C7\E8\B9\E6\C0ڷ\E1
	{ M_GMCS_PUBLICROOM_FRAME_SET,	cbC_GeneralGMPacket_ForOpenRoomForRoomID },
	{ M_CS_PUBLICROOM_FRAME_LIST,	cbC_toOwnRoom },

	// \B0\F8\B0\F8\B1\B8\BF\AA\BF\A1\BC\AD \B4\EB\C7\FC\C7\E0\BB\E7\C1\B6\C1\F7
	//{ M_CS_LARGEACT_LIST,		cbC_toOpenRoomForRoomID }, //by krs
	{ M_GMCS_LARGEACT_SET,		cbC_GeneralGMPacket_ForOpenRoomForRoomID },
	{ M_GMCS_LARGEACT_DETAIL,	cbC_GeneralGMPacket_ForOpenRoomForRoomID },

	// \B0\F8\B0\F8\B1\B8\BF\AA\BF\A1\BC\AD \B4\EB\C7\FC\C7\E0\BB\E7\C2\FC\B0\A1/\C1\F8\C7\E0
	{ M_CS_LARGEACT_REQUEST,	cbC_toOpenRoomForRoomID },
	{ M_CS_LARGEACT_CANCEL,		cbC_toOpenRoomForRoomID },
	{ M_CS_LARGEACT_STEP,		cbC_toOwnRoom },
	{ M_CS_LARGEACT_USEITEM,	cbC_toOwnRoom },

	{ M_CS_MULTILANTERN_REQ,	cbC_toOwnRoom },
	{ M_CS_MULTILANTERN_JOIN,	cbC_toOwnRoom },
	{ M_CS_MULTILANTERN_START,	cbC_toOwnRoom },
	{ M_CS_MULTILANTERN_END,	cbC_toOwnRoom },
	{ M_CS_MULTILANTERN_CANCEL,	cbC_toOwnRoom },
	{ M_CS_MULTILANTERN_OUTJOIN,cbC_toOwnRoom },

	{ M_CS_NIANFO_BEGIN,		cbC_toOwnRoom },
	{ M_CS_NIANFO_END,			cbC_toOwnRoom },
	{ M_CS_NEINIANFO_BEGIN,		cbC_toOwnRoom },
	{ M_CS_NEINIANFO_END,		cbC_toOwnRoom },
	{ M_CS_ADD_TOUXIANG,		cbC_toOwnRoom },
	{ M_GMCS_SBY_LIST,			cbC_toOwnRoom },
	{ M_GMCS_SBY_DEL,			cbC_toOwnRoom },
	{ M_GMCS_SBY_EDIT,			cbC_toOwnRoom },
	{ M_GMCS_SBY_ADD,			cbC_toOwnRoom },
	{ M_CS_SBY_ADDREQ,			cbC_toOwnRoom },
	{ M_CS_SBY_EXIT,			cbC_toOwnRoom },
	{ M_CS_SBY_OVER,			cbC_toOwnRoom },
	{ M_CS_PBY_MAKE,			cbC_toOwnRoom },
	{ M_CS_PBY_SET,				cbC_toOwnRoom },
	{ M_CS_PBY_JOINSET,			cbC_toOwnRoom },
	{ M_CS_PBY_ADDREQ,			cbC_toOwnRoom },
	{ M_CS_PBY_EXIT,			cbC_toOwnRoom },
	{ M_CS_PBY_UPDATESTATE,		cbC_toOwnRoom },
	{ M_CS_PBY_CANCEL,			cbC_toOwnRoom },
	{ M_CS_PBY_OVERUSER,		cbC_toOwnRoom },
	{ M_CS_PBY_HISLIST,			cbC_toOwnRoom },
	{ M_CS_PBY_MYHISLIST,		cbC_toOwnRoom },
	{ M_CS_PBY_HISINFO,			cbC_toOwnRoom },
	{ M_CS_PBY_SUBHISINFO,		cbC_toOwnRoom },
	{ M_CS_PBY_HISMODIFY,		cbC_toOwnRoom },
	{ M_CS_PBY_SUBHISMODIFY,	cbC_toOwnRoom },

	// checkin everyday
	{ M_CS_CHECKIN,				cbC_toOwnRoom },
	{ M_CS_REQLUCKINPUBROOM,	cbC_toOwnRoom },
	{ M_CS_GETBLESSCARDINPUBROOM,cbC_toOwnRoom },
	{ M_CS_HISLUCKINPUBROOM,	cbC_toOwnRoom },

	// \C1\F6\BF\AA\BD\E1\B9\F6\C0\FDȯ\BF\EB
	{ M_CS_CHANGE_SHARD_REQ,	cbC_toWelcomeService },
	{ M_CS_REQ_MST_DATA,		cbC_toServerAfterFID },
	{ M_CS_REQ_PRIMARY_DATA,	cbC_toManagerService },
};

// Callback from services
TUnifiedCallbackItem CallbackArrayForShard[] =
{
	{ M_NT_LOGIN,				cbS_M_NT_LOGIN },	// From FS
	{ M_NT_LOGOUT,				cbS_M_NT_LOGOUT },	// From FS
	{ M_NT_ROOM,				cbS_M_NT_ROOM },	// From OpenRoomService

	{ M_SC_DISCONNECT_OTHERLOGIN,	cb_M_SC_DISCONNECT_OTHERLOGIN },	// From WS

	// character \BD\E1\BA\F1\BD\BA
	{ M_SC_FAMILY,				cbS_M_SC_FAMILY },
	//{ M_SC_DELFAMILY,			cbS_M_SC_DELFAMILY },
	{ M_NTENTERROOM,			cbS_M_NTENTERROOM },	
	{ M_NTOUTROOM,				cbS_M_NTOUTROOM },
	//{ M_NTCHANGEROOMCHANNEL,	cbS_M_NTCHANGEROOMCHANNEL },

	{ M_NT_CHANGE_NAME,			cbS_M_NT_CHANGE_NAME },	// From OpenRoomService

	{ M_NT_SET_DBTIME,			cbS_M_NT_SET_DBTIME },	// From ManagerService
	{ M_NT_SILENCE_SET,			cbS_M_NT_SILENCE_SET },	// From ManagerService

	//{ M_CS_FORURL,              cbS_toClientForURL }, 
	//{ M_NT_STATECH,             cbS_M_NT_STATECH }, 
	

	{ M_SC_NEWFAMILY,			cbS_toClientBeforeFID },
	//{ M_SC_FNAMECHECK,			cbS_toClientBeforeFID },

	{ M_SC_ALLCHARACTER,		cbS_toClientAfterFID },
	{ M_SC_FAMILY_FIGURE,		cbS_toClientAfterFID },
	//{ M_SC_FAMILY_FACEMODEL,	cbS_toClientAfterFID },
	//{ M_SC_CHCHARACTER,			cbS_toClientAfterFID },		
	{ M_SC_MSTITEM,				cbS_toClientAfterFID },
	{ M_SC_FAMILYITEM,			cbS_toClientAfterFID },
	//{ M_SC_FITEMPOS,			cbS_toClientAfterFID },
	{ M_SC_SETINVEN,			cbS_toClientAfterFID },
	{ M_SC_SETINVENNUM,			cbS_toClientAfterFID },
	{ M_SC_FPROPERTY_MONEY,		cbS_toClientAfterFID },
	{ M_SC_FPROPERTY_LEVEL,		cbS_toClientAfterFID },
	{ M_SC_INVENSYNC,			cbS_toClientAfterFID },
	//{ M_SC_CHDRESS,				cbS_toClientAfterFID },
	{ M_SC_BUYITEM,				cbS_toClientAfterFID },
	//{ M_SC_TIMEOUT_ITEM,		cbS_toClientAfterFID },
	//{ M_SC_TRIMITEM,			cbS_toClientAfterFID },

	//{ M_SC_BANKITEM_LIST,		cbS_toClientAfterFID },

	{ M_SC_MSTROOM,				cbS_toClientAfterFID },
	//{ M_SC_FAMILYCOMMENT,		cbS_toClientAfterFID },

	{ M_SC_EXPROOM_GROUP,		cbS_toClientAfterFID },
	{ M_SC_EXPROOMS,			cbS_toClientAfterFID },
	//{ M_GMSC_EXPROOM_GROUP_ADD,	cbS_toClientAfterFID },
	{ M_SC_TOPLIST_ROOMEXP,		cbS_toClientAfterFID },
	{ M_SC_TOPLIST_ROOMVISIT,	cbS_toClientAfterFID },
	{ M_SC_TOPLIST_FAMILYEXP,	cbS_toClientAfterFID },

	{ M_SC_BESTSELLITEM,		cbS_toClientAfterFID },
	{ M_SC_FINDFAMILY,			cbS_toClientAfterFID },
	//{ M_SC_REQ_CHANGECH,		cbS_toClientAfterFID },
	//{ M_SC_CHANGE_CHDESC,		cbS_toClientAfterFID },
	//{ M_SC_CHDESC,				cbS_toClientAfterFID },
	
	// Money
//	{ M_SC_MONEY,				cbS_toClientAfterFID },
	{ M_SC_MONEY_DEFICIT,		cbS_toClientAfterFID },

	// RoomService
	{ M_SC_OWNROOM,				cbS_toClientAfterFID },
	{ M_SC_NEWROOM,				cbS_toClientAfterFID },
	{ M_SC_DELROOM,				cbS_toClientAfterFID },
	{ M_SC_UNDELROOM,			cbS_toClientAfterFID },
	{ M_SC_FINDROOMRESULT,		cbS_toClientAfterFID },	
	//{ M_SC_SETROOMINFO, 		cbS_toClientAfterFID },	
	{ M_SC_ROOMINFO,			cbS_toClientAfterFID },
	//{ M_SC_SETROOMPWD,          cbS_toClientAfterFID },

	// Favorite
	//{ M_SC_FAVORROOMGROUP,		cbS_toClientAfterFID },
	//{ M_SC_FAVORROOM,			cbS_toClientAfterFID },
	{ M_SC_FAVORROOM_AUTODEL,	cbS_toClientAfterFID },
	{ M_SC_FAVORROOM_INFO,		cbS_toClientAfterFID },
	{ M_SC_ROOMORDERINFO,		cbS_toClientAfterFID },
	
	// Festival
	{ M_SC_ALLFESTIVAL,			cbS_toClientAfterFID },
	//{ M_SC_DEADSMEMORIALDAY,	cbS_toClientAfterFID },
	{ M_SC_REGFESTIVAL,			cbS_toClientAfterFID },
	{ M_SC_RENEWROOM,			cbS_toClientAfterFID },
	//{ M_SC_PROLONG_PASS,		cbS_toClientAfterFID },

	{ M_SC_MODIFYFESTIVAL,		cbS_toClientAfterFID },
	{ M_SC_DELFESTIVAL,			cbS_toClientAfterFID },
	{ M_SC_ALL_FESTIVALALARM,	cbS_toClientAfterFID },
	{ M_SC_SET_FESTIVALALARM,	cbS_toClientAfterFID },

	{ M_SC_PROMOTION_ROOM,		cbS_toClientAfterFID },
	{ M_SC_PROMOTION_PRICE,		cbS_toClientAfterFID },

	//{ M_SC_ROOMCARD_CHECK,		cbS_toClientAfterFID },

	// ContactService
	//{ M_SC_MYCONTGROUP,			cbS_toClientAfterFID },
	//{ M_SC_MYCONTLIST,			cbS_toClientAfterFID },
	//{ M_SC_ADDCONTGROUP,		cbS_toClientAfterFID },
	//{ M_SC_DELCONTGROUP,		cbS_toClientAfterFID },
	//{ M_SC_RENCONTGROUP,		cbS_toClientAfterFID },
	//{ M_SC_ADDCONTRESULT,		cbS_toClientAfterFID },
	//{ M_SC_NOTEREQCONT,			cbS_toClientAfterFID },
	//{ M_SC_ADDCONT,				cbS_toClientAfterFID },
	//{ M_SC_DELCONT,				cbS_toClientAfterFID },
	//{ M_SC_CHANGECONTGRP,		cbS_toClientAfterFID },
	//{ M_SC_SETCONTGROUP_INDEX,  cbS_toClientAfterFID },
	{ M_SC_FAMILY_MODEL,		cbS_toClientAfterFID },
	{ M_SC_CHITLIST,			cbS_toClientAfterFID },
	//{ M_SC_CONTFAMILY,			cbS_toClientAfterFID },
	//{ M_SC_REQPORTRAIT,			cbS_toClientAfterFID },

	// oenproom \BD\E1\BA\F1\BD\BA
	{ M_SC_CHECK_ENTERROOM,		cbS_toClientAfterFID },
	{ M_SC_REQEROOM,			cbS_toClientAfterFID },
	{ M_SC_CHANNELLIST,			cbS_toClientAfterFID },
	{ M_SC_ENTERROOM,			cbS_toClientAfterFID },	
	{ M_SC_CHANGECHANNELIN,		cbS_toClientAfterFID },	
	{ M_SC_CHANNELCHANGED,		cbS_toSpecClient },	

	{ M_SC_INVITE_REQ,			cbS_toClientAfterFID },
	{ M_SC_INVITE_ANS,			cbS_toClientAfterFID },
	{ M_SC_BANISH,				cbS_toClientAfterFID },

	{ M_SC_MANAGEROOMS_LIST,	cbS_toClientAfterFID },
	{ M_SC_MANAGER_LIST,		cbS_toClientAfterFID },

	{ M_SC_ALLINROOM,			cbS_toClientAfterFID },
	{ M_SC_DECEASED,			cbS_toClientAfterFID },
	{ M_SC_SET_DEAD_FIGURE,		cbS_toClientAfterFID },
	//{ M_SC_DECEASED_HIS_LIST,	cbS_toClientAfterFID },
	//{ M_SC_DECEASED_HIS_ADD,	cbS_toClientAfterFID },
	//{ M_SC_DECEASED_HIS_UPDATE,	cbS_toClientAfterFID },
	//{ M_SC_DECEASED_HIS_DEL,	cbS_toClientAfterFID },
	//{ M_SC_DECEASED_HIS_GET,	cbS_toClientAfterFID },
	{ M_SC_CHATEAR,				cbS_toClientAfterFID },
	{ M_SC_CHATEAR_FAIL,		cbS_toClientAfterFID },
	//{ M_SC_ROOMPART,			cbS_toClientAfterFID },		
	//{ M_SC_DELDEAD,				cbS_toClientAfterFID },
	{ M_SC_NEWRMEMO,			cbS_toClientAfterFID },
	{ M_SC_ROOMMEMO,			cbS_toClientAfterFID },		
	//{ M_SC_DELRMEMO,			cbS_toClientAfterFID },
	//{ M_SC_MODIFYRMEMO,         cbS_toClientAfterFID },
	{ M_SC_ROOMMEMO_LIST,		cbS_toClientAfterFID },
	{ M_SC_ROOMMEMO_UPDATE,		cbS_toSpecClient },
	//{ M_SC_ROOMMEMO_DATA,		cbS_toClientAfterFID },

	//{ M_SC_GETRMAN,				cbS_toClientAfterFID },
	//{ M_SC_ADDRMAN,				cbS_toClientAfterFID },
	//{ M_SC_DELRMAN,				cbS_toClientAfterFID },		
	//{ M_SC_EVENT,				cbS_toClientAfterFID },
	//{ M_SC_ADDEVENT,			cbS_toClientAfterFID },		
	//{ M_SC_PLAYEVENT,			cbS_toClientAfterFID },
	//{ M_SC_NTDELMAN,			cbS_toClientAfterFID },
	//{ M_SC_NTADDMAN,			cbS_toClientAfterFID },		
	//{ M_SC_ROOMMUSIC,			cbS_toClientAfterFID },			
	//{ M_SC_HISDOWNREQ,			cbS_toClientAfterFID },
	{ M_SC_MSTHISSPACE,			cbS_toClientAfterFID },
	{ M_SC_MSTROOMLEVELEXP,		cbS_toClientAfterFID },
	{ M_SC_MSTFAMILYLEVELEXP,	cbS_toClientAfterFID },
	{ M_SC_MSTCONFIG,			cbS_toClientAfterFID },

	{ M_SC_CURHISSPACE,			cbS_toClientAfterFID },			{ M_SC_ROOMFORFAMILY,		cbS_toClientAfterFID },		
	//{ M_SC_ROOMFAMILYCOMMENT,	cbS_toClientAfterFID },
	//{ M_SC_GETALLINROOM,		cbS_toClientAfterFID },
	{ M_SC_CHANGEMASTER,		cbS_toClientAfterFID },
	{ M_SC_CHANGEMASTER_REQ,	cbS_toClientAfterFID },
	{ M_SC_SETROOMMASTER,		cbS_toClientAfterFID },
	{ M_SC_CONNECT_PEER,		cbS_toClientAfterFID },
	{ M_REQ_CONNECT_PEER,		cbS_toClientAfterFID },			{ M_SC_REQUEST_TALK,		cbS_toClientAfterFID },
	{ M_SC_RESPONSE_TALK,		cbS_toClientAfterFID },			{ M_SC_SESSION_ADDMEMBER,	cbS_toClientAfterFID },
	{ M_SC_JOIN_SESSION,		cbS_toClientAfterFID },			{ M_SC_SPEAK_TO_SESSION,	cbS_toClientAfterFID },
	{ M_SC_DISCONNECT_PEER,		cbS_toClientAfterFID },
	{ M_SC_HANGUP_TALK,			cbS_toClientAfterFID },

	{ M_SC_ACT_WAIT,			cbS_toClientAfterFID },
	{ M_SC_ACT_RELEASE,			cbS_toSpecClient },
	{ M_SC_ACT_START,			cbS_toSpecClient },
//	{ M_SC_ACT_ADD_MODEL,		cbS_toSpecClient },
//	{ M_SC_ACT_DEL_MODEL,		cbS_toSpecClient },
	//{ M_SC_ACT_CAPTURE,			cbS_toSpecClient },
//	{ M_SC_ACT_NPC_SHOW,		cbS_toSpecClient },
//	{ M_SC_ACT_NPC_HIDE,		cbS_toSpecClient },
//	{ M_SC_ACT_NPC_MOVE,		cbS_toSpecClient },
//	{ M_SC_ACT_NPC_ITEM_HOLD,	cbS_toSpecClient },
	//{ M_SC_ACT_NPC_ITEM_GIVE,	cbS_toSpecClient },
	//{ M_SC_ACT_NPC_ITEM_PUT,	cbS_toSpecClient },
//	{ M_SC_ACT_NPC_ANI,			cbS_toSpecClient },
//	{ M_SC_ACT_NPC_ROTATE,		cbS_toSpecClient },
//	{ M_SC_ACT_ITEM_PUT,		cbS_toSpecClient },
	{ M_SC_ACT_STEP,			cbS_toSpecClient },
	{ M_SC_ACT_NOW_START,		cbS_toSpecClient },

	{ M_SC_MULTIACT2_ITEMS,		cbS_toSpecClient },
	{ M_SC_MULTIACT2_ASK,		cbS_toSpecClient },
	{ M_SC_MULTIACT2_JOIN,		cbS_toClientAfterFID },
	{ M_SC_MULTIACT2_JOINADD,	cbS_toClientAfterFID },
	{ M_SC_MULTIACT_REQ,		cbS_toClientAfterFID },
	{ M_SC_MULTIACT_WAIT,		cbS_toClientAfterFID },
	{ M_SC_MULTIACT_ANS,		cbS_toClientAfterFID },
	{ M_SC_MULTIACT_READY,		cbS_toClientAfterFID },
	{ M_SC_MULTIACT_GO,			cbS_toClientAfterFID },
	{ M_SC_MULTIACT_STARTED,	cbS_toSpecClient },
	{ M_SC_MULTIACT_START,		cbS_toSpecClient },
	{ M_SC_MULTIACT_CANCEL,		cbS_toSpecClient },
	{ M_SC_MULTIACT_RELEASE,	cbS_toSpecClient },
	{ M_SC_MULTIACT_COMMAND,	cbS_toClientAfterFID },
	{ M_SC_MULTIACT_REPLY,		cbS_toClientAfterFID },
	{ M_SC_MULTIACT_REQADD,		cbS_toClientAfterFID },

	{ M_SC_CRACKER_LIST,		cbS_toClientAfterFID },
	{ M_SC_CRACKER_BOMB,		cbS_toSpecClient },
	{ M_SC_CRACKER_CREATE,		cbS_toSpecClient },
	{ M_SC_LANTERN,				cbS_toSpecClient },
	{ M_SC_ITEM_RAIN,			cbS_toSpecClient },
	{ M_SC_ITEM_POINTCARD,		cbS_toSpecClient },

	{ M_SC_ROOMVISITLIST,		cbS_toClientAfterFID },

	{ M_SC_VISIT_DETAIL_LIST,	cbS_toClientAfterFID },
	{ M_SC_VISIT_DATA,			cbS_toClientAfterFID },
	{ M_SC_VISIT_FISHFLOWER_LIST,	cbS_toClientAfterFID },
	{ M_SC_ROOM_VISIT_FID,		cbS_toClientAfterFID },

	{ M_SC_PAPER_LIST,			cbS_toClientAfterFID },
	{ M_SC_PAPER,				cbS_toClientAfterFID },
	{ M_SC_NEW_PAPER,			cbS_toClientAfterFID },
	{ M_SC_DEL_PAPER,			cbS_toClientAfterFID },
	//{ M_SC_MODIFY_PAPER,		cbS_toClientAfterFID },

	// Mail
	{ M_SC_MAILBOX_STATUS,		cbS_toClientAfterFID },
	{ M_SC_MAIL_LIST,			cbS_toClientAfterFID },
	{ M_SC_MAIL,				cbS_toClientAfterFID },
	{ M_SC_MAIL_SEND,			cbS_toClientAfterFID },
	//{ M_SC_MAIL_DELETE,			cbS_toClientAfterFID },
	{ M_SC_MAIL_REJECT,			cbS_toClientAfterFID },
	//{ M_SC_MAIL_ACCEPT_ITEM,	cbS_toClientAfterFID },

	{ M_SC_NEWENTER,			cbS_toSpecClient },
	{ M_SC_OUTROOM,				cbS_toSpecClient },
	{ M_SC_STATECH,				cbS_toSpecClient },
	{ M_SC_MOVECH,				cbS_toSpecClient },
	{ M_SC_ACTION,				cbS_toSpecClient },

	{ M_SC_CHAIR_LIST,			cbS_toClientAfterFID },
	{ M_SC_CHAIR_SITDOWN,		cbS_toSpecClient },
	{ M_SC_CHAIR_STANDUP,		cbS_toSpecClient },

	{ M_SC_CHATROOM,			cbS_toSpecClient },
	{ M_SC_CHATCOMMON,			cbS_toSpecClient },
	//{ M_SC_ROOMPARTCHANGE,		cbS_toSpecClient },
	//{ M_SC_SACRIFICE,			cbS_toSpecClient },
	//{ M_SC_DELSACRIFICE,		cbS_toSpecClient },	
	//{ M_SC_CHMETHOD,			cbS_toSpecClient },
	{ M_SC_CHANGECHDRESS,		cbS_toSpecClient },
	{ M_SC_ROOMPRO,				cbS_toSpecClient },
	{ M_SC_TOMBSTONE,			cbS_toSpecClient },
	{ M_SC_RUYIBEI,				cbS_toSpecClient },
	{ M_SC_RUYIBEI_TEXT,		cbS_toClientAfterFID },
	//{ M_SC_HOLDITEM,			cbS_toSpecClient },

	{ M_SC_UPDATE_FISH,			cbS_toSpecClient },
	{ M_SC_UPDATE_FLOWER,		cbS_toSpecClient },
	{ M_SC_FLOWER_NEW_ANS,		cbS_toClientAfterFID },
	{ M_SC_FLOWER_WATER_ANS,	cbS_toClientAfterFID },
	{ M_SC_UPDATE_GOLDFISH,		cbS_toSpecClient },
	{ M_SC_FLOWER_WATERTIME_ANS,cbS_toClientAfterFID },
	//{ M_SC_NEWPET,				cbS_toSpecClient },
	//{ M_SC_UPDATEPET,			cbS_toSpecClient },
	//{ M_SC_DELPET,				cbS_toSpecClient },
	{ M_SC_MSTFISH,				cbS_toClientAfterFID },
	//{ M_SC_UPDATE_ROOMFISH,		cbS_toSpecClient },

	{ M_SC_LUCK,				cbS_toSpecClient },
	{ M_SC_LUCK_LIST,			cbS_toClientAfterFID },
	{ M_SC_LUCK_TOALL,			cbS_M_SC_LUCK_TOALL },
	{ M_SC_LUCK4_LIST,			cbS_toClientAfterFID },
	{ M_NT_LUCK4_RESET,			cbS_M_NT_LUCK4_RESET },

	{ M_SC_DRUM_START,			cbS_toSpecClient },
	{ M_SC_DRUM_END,			cbS_toSpecClient },
	{ M_SC_BELL_START,			cbS_toSpecClient },
	{ M_SC_BELL_END,			cbS_toSpecClient },
	{ M_SC_MUSIC_START,			cbS_toSpecClient },
	{ M_SC_MUSIC_END,			cbS_toSpecClient },
	{ M_SC_SHENSHU_START,		cbS_toSpecClient }, // by krs

	//{ M_SC_NEWWISH,				cbS_toSpecClient },
	//{ M_SC_DELWISH,				cbS_toSpecClient },
	//{ M_SC_DELTGATE,			cbS_toSpecClient },
	//{ M_SC_TGATE,				cbS_toSpecClient },
	//{ M_SC_ENTERTGATE,			cbS_toSpecClient },
	//{ M_SC_ROOMTERMTIME,		cbS_toSpecClient },
	//{ M_SC_CHFAMILYINFO,		cbS_toSpecClient },

	// Animal
	//{ M_SC_ANIMAL,				cbS_toClientAfterFID },
	{ M_SC_ANIMALMOVE,			cbS_toSpecClient },
	{ M_SC_ANIMAL_ADD,			cbS_toClientAfterFID },
	//{ M_SC_ANIMAL_DEL,          cbS_toSpecClient },
	{ M_SC_ANIMAL_SELECT,		cbS_toSpecClient },
	{ M_SC_PEACOCK_TYPE,		cbS_toClientAfterFID },

	{ M_SC_EANOTIFYMAINCLIENT,	cbS_toClientAfterFID },
	{ M_SC_EASETPOS,			cbS_toSpecClient },
	{ M_SC_EAMOVE,				cbS_toSpecClient },
	{ M_SC_EABASEINFO,			cbS_toSpecClient },
	{ M_SC_EAANISELECT,			cbS_toSpecClient },
	
	{ M_SC_CHATALL,				cbS_M_SC_CHATALL },					{ M_SC_CHATSYSTEM,			cbS_toAll },				
	{ M_SC_CHATMAIN,			cbS_toAll },

	{ M_PROCPACKET,				cbS_PrcoPacket },

	// For GM Tool
	{ M_GMCS_LOGOFFFAMILY,		cbS_M_GMCS_LOGOFFFAMILY },		// From FS
	{ M_GMSC_GETFAMILY,			cbS_M_GMSC_GETFAMILY },			// From OPENROOM/LOBBY_S
	{ M_GMSC_ACTIVE,			cbS_toSpecClient },				// From OPENROOM
	{ M_GMSC_SHOW,				cbS_toSpecClient },				// From OPENROOM
	{ M_GMSC_MOVETO,			cbS_toSpecClient },				// From OPENROOM
	{ M_GMSC_GETMONEY,			cbS_toClientAfterFID },			// From OPENROOM
	{ M_GMSC_GETITEM,			cbS_toClientAfterFID },			// From OPENROOM
	{ M_GMSC_GETINVENEMPTY,		cbS_toClientAfterFID },			// From OPENROOM
	{ M_GMSC_GETLEVEL,			cbS_toClientAfterFID },			// From OPENROOM
	//{ M_GMSC_MAIL_SEND,			cbS_toClientAfterFID },
	{ M_GMSC_LISTCHANNEL,		cbS_toClientAfterFID },
	{ M_GMSC_ROOMCHAT,			cbS_toGMFamily },
	{ M_GMSC_ZAIXIAN_END,		cbS_toClientAfterFID },
	{ M_GMSC_ZAIXIAN_ASK,		cbS_toClientAfterFID },
	{ M_GMSC_GET_GMONEY,		cbS_toClientAfterFID },			// From OPENROOM

	// For Stress Tool
	{ M_FORCEDISCONNECT,		cbS_M_FORCEDISCONNECT },
	//{ M_SC_FORCEFAMILY,			cbS_toClientAfterFID },
	//{ M_SC_FORCEOUT,			cbS_toClientAfterFID },
	{ M_SC_FORCEOUTROOM,		cbS_toSpecClient },

	// Lobby
	{ M_NT_LOBBY,				cbS_M_NT_LOBBY },	
	//{ M_NT_NEW_LOBBY,			cbS_M_NT_NEW_LOBBY },
	//{ M_NT_DEL_LOBBY,			cbS_M_NT_DEL_LOBBY },
	{ M_SC_ENTER_LOBBY,			cbS_M_SC_ENTER_LOBBY },
	//{ M_SC_LOBBYS,				cbS_toClientAfterFID },
	//{ M_SC_REQ_ENTERLOBBY,		cbS_toClientAfterFID },	
	//{ M_SC_WAIT_ENTERLOBBY,		cbS_toClientAfterFID },
	{ M_SC_LEAVE_LOBBY,			cbS_M_SC_LEAVE_LOBBY },
	//{ M_SC_LOBBY_ADDWISH,       cbS_toClientAfterFID },
	//{ M_SC_LOBBY_WISH,          cbS_toClientAfterFID },
	//{ M_SC_LOBBY_NEWWISH,       cbS_toSpecClient },
	//{ M_SC_LOBBY_DELWISH,       cbS_toSpecClient },
	{ M_SC_CONTSTATE,			cbS_toSpecClient },

	// Cell
	{ M_SC_CELL_IN,				cbS_toSpecClient },
	{ M_SC_CELL_OUT,			cbS_toSpecClient },

	// upload/download
	{ M_SC_HISMANAGE_START,		cbS_toClientAfterFID },

	{ M_SC_FIGUREFRAME_LIST,	cbS_toClientAfterFID },

	//{ M_SC_PHOTOLIST,			cbS_toClientAfterFID },
	//{ M_SC_VIDEOLIST,			cbS_toClientAfterFID },
	{ M_SC_REQ_UPLOAD,			cbS_toClientAfterFID },
	{ M_SC_UPLOAD_DONE,			cbS_toClientAfterFID },
	{ M_SC_REQ_DOWNLOAD,		cbS_toClientAfterFID },
	{ M_SC_REQ_DELETE,			cbS_toClientAfterFID },
	{ M_SC_DELETE_DONE,			cbS_toClientAfterFID },

	// data
	//{ M_SC_RENDATA,				cbS_toClientAfterFID },
	//{ M_SC_ADDALBUM,			cbS_toClientAfterFID },
	//{ M_SC_DELALBUM,			cbS_toClientAfterFID },
	{ M_SC_MOVDATA,				cbS_toClientAfterFID },
	//{ M_SC_CHANGEVIDEOINDEX,	cbS_toClientAfterFID },
	//{ M_SC_RENALBUM,            cbS_toClientAfterFID },
//---	{ M_SC_DELDATA,             cbS_toClientAfterFID },
	//{ M_SC_FRAMEDATA,			cbS_toClientAfterFID },
	{ M_SC_ALBUMINFO,			cbS_toClientAfterFID },
	{ M_SC_FRAMELIST,			cbS_toClientAfterFID },
	{ M_SC_FRAMEDATA_GET,		cbS_toClientAfterFID },
	{ M_SC_PHOTOALBUMLIST,		cbS_toClientAfterFID },
	{ M_SC_VIDEOALBUMLIST,		cbS_toClientAfterFID },
	{ M_SC_REQDATA_PHOTO,		cbS_toClientAfterFID },
	{ M_SC_REQDATA_VIDEO,		cbS_toClientAfterFID },
	//{ M_SC_CHANGEALBUMINDEX,	cbS_toClientAfterFID },

	{ M_SC_WISH,				cbS_toSpecClient },
	{ M_SC_WISH_LIST,			cbS_toClientAfterFID },
	{ M_SC_MY_WISH_LIST,		cbS_toClientAfterFID },
	{ M_SC_MY_WISH_DATA,		cbS_toClientAfterFID },

	//{ M_SC_R_PAPER_LIST,		cbS_toClientAfterFID },
	//{ M_SC_R_PAPER_START,		cbS_toSpecClient },
	//{ M_SC_R_PAPER_PLAY,		cbS_toSpecClient },
	//{ M_SC_R_PAPER_END,			cbS_toSpecClient },

	//{ M_SC_RECOMMEND_ROOM,		cbS_toClientAfterFID },

	{ M_SC_VIRTUE_LIST,			cbS_toClientAfterFID },
	{ M_SC_VIRTUE_MY,			cbS_toClientAfterFID },

	{ M_SC_R_ACT_LIST,			cbS_toClientAfterFID },
	{ M_SC_R_ACT_MY_LIST,		cbS_toClientAfterFID },
	{ M_SC_R_ACT_PRAY,			cbS_toClientAfterFID },

	{ M_SC_ACT_RESULT_XIANBAO,	cbS_toSpecClient },
	{ M_SC_ACT_RESULT_YISHI,	cbS_toSpecClient },
	{ M_SC_ACT_RESULT_AUTO2,	cbS_toSpecClient },
	{ M_SC_ACT_RESULT_ANCESTOR_JISI,	cbS_toSpecClient },
	{ M_SC_ACT_RESULT_ANCESTOR_DECEASED_XIANG,	cbS_toSpecClient },
	{ M_SC_ACT_RESULT_ANCESTOR_DECEASED_HUA,	cbS_toSpecClient },

	{ M_SC_ANCESTOR_TEXT,		cbS_toSpecClient },
	{ M_SC_ANCESTOR_DECEASED,	cbS_toSpecClient },

	{ M_SC_AUTO_ENTER,			cbS_toSpecClient },
	{ M_SC_AUTO_LEAVE,			cbS_toSpecClient },
	//{ M_SC_AUTO_STATECH,		cbS_toSpecClient },
	//{ M_SC_AUTO_CHMETHOD,		cbS_toSpecClient },
	//{ M_SC_AUTO_HOLDITEM,		cbS_toSpecClient },
	//{ M_SC_AUTO_MOVECH,			cbS_toSpecClient },
	{ M_SC_AUTOPLAY_STATUS,		cbS_toClientAfterFID },

	{ M_SC_CURRENT_ACTIVITY,	cbS_toClientAfterFID },
	{ M_SC_GIFTITEM,			cbS_toClientAfterFID },

	{ M_SC_ROOMSTORE,			cbS_toClientAfterFID },
	{ M_SC_ADD_ROOMSTORE_RESULT,cbS_toClientAfterFID },
	{ M_SC_ROOMSTORE_HISTORY,	cbS_toClientAfterFID },
	{ M_SC_ROOMSTORE_HISTORY_DETAIL,cbS_toClientAfterFID },
	{ M_SC_ROOMSTORE_LOCK,		cbS_toClientAfterFID },
	{ M_SC_ROOMSTORE_STATUS,	cbS_toSpecClient },
	{ M_SC_ROOMSTORE_GETITEM,	cbS_toClientAfterFID },

	{ M_SC_PRIZE,				cbS_toSpecClient },
	{ M_SC_YUWANGPRIZE,			cbS_toSpecClient },
	{ M_SC_ADDEDJIFEN,			cbS_toClientAfterFID },
	
	{ M_SC_LARGEACTPRIZE,		cbS_toClientAfterFID },
	{ M_SC_BLESSCARD_LIST,		cbS_toClientAfterFID },
	{ M_SC_BLESSCARD_GET,		cbS_toClientAfterFID },
	{ M_SC_BLESSCARD_SEND,		cbS_toClientAfterFID },
	{ M_SC_BLESSCARD_RECEIVE,	cbS_toClientAfterFID },

	//{ M_SC_REVIEW,				cbS_toClientAfterFID },

	{ M_SC_ROOMEVENT,			cbS_toSpecClient },
	{ M_SC_ROOMEVENT_END,		cbS_toSpecClient },

	{ M_SC_MOUNT_LUCKANIMAL,	cbS_toSpecClient },
	{ M_SC_UNMOUNT_LUCKANIMAL,	cbS_toSpecClient },
	

	{ M_SC_ZAIXIAN_RESPONSE,	cbS_toClientAfterFID },
	{ M_SC_ZAIXIAN_ANSWER,		cbS_toClientAfterFID },
	{ M_SC_ZAIXIAN_CONTROL,		cbS_toClientAfterFID },

	{ M_SC_PUBLICROOM_FRAME_LIST,	cbS_toClientAfterFID },

	{ M_SC_LARGEACT_LIST,			cbS_toClientAfterFID },
	{ M_GMSC_LARGEACT_NEWID,		cbS_toClientAfterFID },
	{ M_GMSC_LARGEACT_DETAIL,		cbS_toClientAfterFID },
	{ M_GMSC_LARGEACT_SET,			cbS_toClientAfterFID },

	{ M_SC_LARGEACT_CURRENT,		cbS_toSpecClient },

	{ M_SC_LARGEACT_REQUEST,		cbS_toClientAfterFID },
	{ M_SC_LARGEACT_PREPARE,		cbS_toClientAfterFID },
	{ M_SC_LARGEACT_START,			cbS_toSpecClient },
	{ M_SC_LARGEACT_OUT,			cbS_toSpecClient },
	{ M_SC_LARGEACT_STEP,			cbS_toSpecClient },
	{ M_SC_LARGEACT_USERNUM,		cbS_toSpecClient },
	{ M_SC_LARGEACT_INFOCHANGE,		cbS_toAll },
	{ M_SC_LARGEACT_READYVIP,		cbS_toClientAfterFID },

	{ M_SC_MULTILANTERN_RES,		cbS_toClientAfterFID },
	{ M_SC_MULTILANTERN_NEW,		cbS_toSpecClient },
	{ M_SC_MULTILANTERN_JOIN,		cbS_toClientAfterFID },
	{ M_SC_MULTILANTERN_START,		cbS_toSpecClient },
	{ M_SC_MULTILANTERN_END,		cbS_toSpecClient },
	{ M_SC_MULTILANTERN_CANCEL,		cbS_toSpecClient },
	{ M_SC_MULTILANTERN_OUTJOIN,	cbS_toClientAfterFID },
	{ M_SC_MULTILANTERN_READYLIST,	cbS_toClientAfterFID },
	
	{ M_SC_NIANFO_BEGIN,			cbS_toSpecClient },
	{ M_SC_NIANFO_END,				cbS_toSpecClient },
	{ M_SC_NEINIANFO_BEGIN,			cbS_toSpecClient },
	{ M_SC_NEINIANFO_END,			cbS_toSpecClient },
	{ M_SC_ADD_TOUXIANG,			cbS_toSpecClient },
	{ M_SC_DEL_TOUXIANG,			cbS_toSpecClient },

	{ M_SC_BY_CURRENT,				cbS_toSpecClient },
	{ M_GMSC_SBY_LIST,				cbS_toClientAfterFID },
	{ M_GMSC_SBY_DELRES,			cbS_toClientAfterFID },
	{ M_GMSC_SBY_EDITRES,			cbS_toClientAfterFID },
	{ M_GMSC_SBY_ADDRES,			cbS_toClientAfterFID },
	{ M_SC_SBY_ADDRES,				cbS_toClientAfterFID },
	{ M_SC_SBY_ADDUSER,				cbS_toSpecClient },
	{ M_SC_SBY_EXITUSER,			cbS_toSpecClient },
	{ M_SC_SBY_START,				cbS_toSpecClient },
	{ M_SC_SBY_OVER,				cbS_toSpecClient },
	{ M_SC_SBY_CURSTATE,			cbS_toClientAfterFID },
	{ M_SC_PBY_MAKERES,				cbS_toClientAfterFID },
	{ M_SC_PBY_SET,					cbS_toClientAfterFID },
	{ M_SC_PBY_ADDRES,				cbS_toClientAfterFID },
	{ M_SC_PBY_ADDUSER,				cbS_toSpecClient },
	{ M_SC_PBY_EXITUSER,			cbS_toSpecClient },
	{ M_SC_PBY_UPDATESTATE,			cbS_toSpecClient },
	{ M_SC_PBY_CURSTATE,			cbS_toClientAfterFID },
	{ M_SC_PBY_CANCEL,				cbS_toSpecClient },
	{ M_SC_PBY_OVERALL,				cbS_toSpecClient },
	{ M_SC_PBY_HISLIST,				cbS_toClientAfterFID },
	{ M_SC_PBY_MYHISLIST,			cbS_toClientAfterFID },
	{ M_SC_PBY_HISINFO,				cbS_toClientAfterFID },
	{ M_SC_PBY_SUBHISINFO,			cbS_toClientAfterFID },

	{ M_SC_CHECKIN,					cbS_toClientAfterFID },
	{ M_SC_CHECKINSTATE,			cbS_toClientAfterFID },
	{ M_SC_HISLUCKINPUBROOM,		cbS_toClientAfterFID },
	{ M_SC_CURLUCKINPUBROOM,		cbS_toClientAfterFID },
	{ M_NT_CURLUCKINPUBROOM,		cbS_M_NT_CURLUCKINPUBROOM	},

	// \C1\F6\BF\AA\BD\E1\B9\F6\C0\FDȯ\BF\EB
	{ M_SC_CHANGE_SHARD_ANS,		cbS_toClientAfterFID },
};

void	InitShardClient()
{
	Clients->CCallbackNetBase::addCallbackArray (ClientCallbackArrayForShard, sizeof(ClientCallbackArrayForShard)/sizeof(ClientCallbackArrayForShard[0]));
	Clients->SetWarningMessageLengthForSend(MAX_PACKET_BUF_LEN);
	InitFilter();

	CUnifiedNetwork::getInstance()->addCallbackArray(CallbackArrayForShard, sizeof(CallbackArrayForShard)/sizeof(CallbackArrayForShard[0]));
	CUnifiedNetwork::getInstance()->setServiceUpCallback (OROOM_SHORT_NAME, onReconnectOpenRoom, 0);
	CUnifiedNetwork::getInstance()->setServiceDownCallback (OROOM_SHORT_NAME, onDisconnectOpenRoom, 0);

	CUnifiedNetwork::getInstance()->setServiceUpCallback (LOBBY_SHORT_NAME, onReconnectLobby, 0);
	CUnifiedNetwork::getInstance ()->setServiceDownCallback (LOBBY_SHORT_NAME, onDisconnectLobby, 0);
}

SIPBASE_CATEGORISED_COMMAND(FES, lsFamilys, "displays the list of all login familys", "")
{
	map<uint32, TTcpSockId> chMap = twinmapFIDandSID.getAToBMap();

	log.displayNL ("Display the %d connected familys :", chMap.size());
	for (map<uint32, TTcpSockId>::iterator it = chMap.begin(); it != chMap.end (); it++)
	{
		T_FAMILYID FID = (*it).first;
		map<T_FAMILYID, T_FAMILYNAME>::iterator	itName = mapFamilyName.find(FID);
		T_FAMILYNAME FName;
		if (itName != mapFamilyName.end())
			FName = itName->second;

		T_ACCOUNTID	UserId = GetIdentifiedID(it->second);
		ucstring	UserName = GetUserName(UserId);

		const CTcpSock*		tcpSock = it->second->getTcpSock();
		CInetAddress	radd = tcpSock->remoteAddr();
		log.displayNL (L" FamilyId=%u\ttFamilyName=%s\ttUserName=%s\ttSocket=%p, Address=%s", FID, FName.c_str(), UserName.c_str(), it->second, ucstring(radd.asString()).c_str());
	}
	log.displayNL ("End of the list");
	
	return true;
}

SIPBASE_CATEGORISED_COMMAND(FES, disconnectFamily, "Disconnect family", "")
{
	if(args.size() < 1)	return false;
	
	string familyId	= args[0];
	uint32 uF = atoui(familyId.c_str());
	DisconnectClientForFamilyID(uF, L"From command line");
	log.displayNL ("Disconnect family(%d)", uF);

	return true;
}

SIPBASE_CATEGORISED_COMMAND(FES, disconnectUser, "Disconnect user", "")
{
	if(args.size() < 1)	return false;

	string userId = args[0];
	uint32 uId = atoui(userId.c_str());
	DisconnectClientForUserID(uId);
	log.displayNL ("Disconnect user(%d)", uId);

	return true;
}

