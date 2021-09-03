#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>
#include <net/SecurityServer.h>
#include <misc/stream.h>

#include <map>
#include <utility>

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"
#include "../Common/DBLog.h"

#include "shard_client.h"
#include "main.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

void SendSilentToGM(TTcpSockId _from);

#define		NONE_NUMBER				0
#define		NONE_NVARCHAR			L""

#define		DECLARE_GMLOG_VARIABLES()											\
	map<T_FAMILYID,	FAMILYIINFO>::iterator itGM = mapAllOnlineFamilyForFID.find(_GMID);	\
	if (itGM == mapAllOnlineFamilyForFID.end())	\
	return;	\
	\
	uint32				MainType = NONE_NUMBER;								\
	uint32				PID = NONE_NUMBER;									\
	uint32				UID = NONE_NUMBER;									\
	uint32				PID2 = NONE_NUMBER;									\
	uint32				UID2 = NONE_NUMBER;									\
	sint32				VI1 = NONE_NUMBER, VI2 = NONE_NUMBER, VI3 = NONE_NUMBER;	\
	ucstring			VU1 = NONE_NVARCHAR;								\
	ucstring			UName = NONE_NVARCHAR;								\
	ucstring			ObjectName = NONE_NVARCHAR;							\
	\
	uint32				UserID = NONE_NUMBER;								\
	ucstring			FamilyName = NONE_NVARCHAR;							\
	ucstring			UserName = NONE_NVARCHAR;							\
	\
	PID = _GMID;															\
	UID = itGM->second.UserID;												\
	UName	= L"[" + itGM->second.UserName + L"]" + itGM->second.FName;		\

#define		DOING_GMLOG()												\
	CMessage			mesLog(M_CS_GMLOG);								\
	mesLog.serial(MainType, PID, UID, PID2, UID2);						\
	mesLog.serial(VI1, VI2, VI3, VU1, UName, ObjectName);				\
	CUnifiedNetwork::getInstance ()->send( LOG_SHORT_NAME, mesLog );	\

#define		DOING_GMLOG_TOOL()												\
	UName	= L"[" + itGM->second.UserName + L"]" + "0";				\
	CMessage			mesLog(M_CS_GMLOG);								\
	mesLog.serial(MainType, PID, UID, PID2, UID2);						\
	mesLog.serial(VI1, VI2, VI3, VU1, UName, ObjectName);				\
	CUnifiedNetwork::getInstance ()->send( LOG_SHORT_NAME, mesLog );	\

void	GMLog_Notice(uint32 _GMID, ucstring _Notice)
{
	DECLARE_GMLOG_VARIABLES()

	MainType = GMLOG_MAINTYPE_NOTIC;
	VU1 = _Notice;

	UserName = GetUserName(_GMID);
	UName	= L"[" + UserName + L"]";

	DOING_GMLOG();
}

// Only GMTool
void	GMLog_LogoffFamily(T_FAMILYID _GMID, T_FAMILYID FID)
{
	DECLARE_GMLOG_VARIABLES();

	map<T_FAMILYID,	FAMILYIINFO>::iterator itFamily = mapAllOnlineFamilyForFID.find(FID);
	if (itFamily == mapAllOnlineFamilyForFID.end())
	{
		TTcpSockId	_from = GetSocketIDForFamily(_GMID);
		SendGMFail(ERR_ALREADYOFFLINE,_from);
		return;
	}

	MainType = GMLOG_MAINTYPE_LOGOFFFAMILY;
	PID2 = itFamily->second.FID;
	UID2 = itFamily->second.UserID;
	ObjectName	= L"[" + itFamily->second.UserName + L"]" + itFamily->second.FName;

	DOING_GMLOG_TOOL();
}

// Packet: M_GMCS_NOTICE
// GM
void cbC_M_GM_NOTICE( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	uint8		uType;
	ucstring	ucNotice;
	_msgin.serial(uType, ucNotice);

	CMessage mes(M_SC_CHATSYSTEM);
	mes.serial(uType, ucNotice);
	Clients->send (mes, InvalidSockId);

	CUnifiedNetwork::getInstance ()->send( FRONTEND_SHORT_NAME, mes, false );
	
	// GM Log
	GMLog_Notice(uIdenID, ucNotice);
}

static bool	IsOnline(T_FAMILYID _FID)
{
	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(_FID);
	if (it == mapAllOnlineFamilyForFID.end())
		return false;

	return	true;
}

static bool	IsInLobby(T_FAMILYID _FID)
{
	bool	bLobby	= false;
	bool	bOnline = false;
	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(_FID);
	if (it == mapAllOnlineFamilyForFID.end())
		return false;

	bOnline = true;
	bLobby	= it->second.bInLobby;

	return bLobby;
}

void cbC_M_GMCS_WHEREFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	T_FAMILYID	FID;
	_msgin.serial(FID);

	if ( IsGM(FID) )
	{
		SendGMFail(ERR_GM_NOTOUCHGM, _from);
		return;
	}

	bool			bOnline = false, bLobby = false;
	T_ROOMID		uRoomID = 0;
	T_ROOMCHANNELID	uRoomChannelID = 0;
	ucstring ucRoomName = L"", ucFamilyName = L"";
	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.find(FID);
	if (it == mapAllOnlineFamilyForFID.end())
	{
		ucFamilyName = L"";
		bOnline = false;
		bLobby	= false;
		uRoomID = 0;
		ucRoomName = L"";
		uRoomChannelID = 0;
	}
	else
	{
		ucFamilyName = it->second.FName;
		bOnline = true;
		bLobby	= it->second.bInLobby;
		uRoomID = it->second.RoomID;
		ucRoomName = it->second.RoomName;
		uRoomChannelID = it->second.RoomChannelID;
	}

	CMessage mes(M_GMSC_WHEREFAMILY);
	mes.serial(FID, ucFamilyName, bOnline, bLobby, uRoomID, ucRoomName, uRoomChannelID);
	Clients->send (mes, _from);
}

void cbC_M_GMCS_PULLFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	T_FAMILYID uObjectID;	_msgin.serial(uObjectID);
	T_CH_STATE State;		_msgin.serial(State);
	T_CH_DIRECTION Dir;		_msgin.serial(Dir);
	T_CH_X X;				_msgin.serial(X);
	T_CH_Y Y;				_msgin.serial(Y);
	T_CH_Z Z;				_msgin.serial(Z);

	if ( !IsOnline(uObjectID) )
	{
		sipwarning("family : %d is already offline", uObjectID);
		SendGMFail(ERR_ALREADYOFFLINE, _from);
		return;
	}

	if ( IsInLobby(uObjectID) )
	{
		SendGMFail(ERR_GM_LOBBYOP, _from);
		return;
	}

	if ( uGMFID != uObjectID && IsGM(uObjectID) )
	{
		SendGMFail(ERR_GM_NOTOUCHGM, _from);
		return;
	}

	TServiceId	sid = OpenRoomService.GetSIDFromFID(uGMFID);
	if (sid.get() == 0)
		return;

	CMessage	msgOut(M_GMCS_PULLFAMILY);
	msgOut.serial(uGMFID, uObjectID, Dir, X, Y, Z);
	CUnifiedNetwork::getInstance ()->send( sid, msgOut );

}

void cbC_M_GMCS_GETFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	int	size = mapAllOnlineFamilyForFID.size();
	map<T_FAMILYID,	FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.begin();
	while( size > 0 )
	{
		CMessage	msgout(M_GMSC_GETFAMILY);
		uint32	nSendOnce = (size >= 30) ? 30 : size;
		msgout.serial(uGMFID);
		msgout.serial(nSendOnce);
		for ( uint32 i = 0; i < nSendOnce; i ++, it++ )
		{
			if (it == mapAllOnlineFamilyForFID.end())
				break;
			FAMILYIINFO	info = it->second;
			T_FAMILYID		familyid = info.FID;			msgout.serial(familyid);
			T_FAMILYNAME	familyName = info.FName;		msgout.serial(familyName);
			T_ACCOUNTID		userID = info.UserID;			msgout.serial(userID);
			uint8		uUserType = GetUserType(userID);	msgout.serial(uUserType);
			T_COMMON_NAME	userName = info.UserName;		msgout.serial(userName);
			T_ROOMID		roomID = info.RoomID;			msgout.serial(roomID);
			T_ROOMNAME		roomName = info.RoomName;		msgout.serial(roomName);
			bool			bInLobby = info.bInLobby;		msgout.serial(bInLobby);										
		}
		size -= nSendOnce;
		bool	bEndSend = (size > 0) ? false : true;			msgout.serial(bEndSend);
		Clients->send(msgout, _from);
	}

	SendSilentToGM(_from);
}

// OPENROOM_S -> FS
// packet : M_GMSC_GETFAMILY
void cbS_M_GMSC_GETFAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
/*	
	uint32	uGMFID;					_msgin.serial(uGMFID);
	uint32	sizeCH;					_msgin.serial(sizeCH);
	if ( sizeCH == 0 )
		return;

	for ( uint i = 0; i < sizeCH; i ++ )
	{
		T_CHARACTERID	chid;		_msgin.serial(chid);
		T_CHARACTERNAME	chname;		_msgin.serial(chname);
		T_FAMILYID		familyid;	_msgin.serial(familyid);
		if ( uGMFID == familyid )
			break;

		map<T_FAMILYID,		FAMILYIINFO>::iterator	it = mapAllOnlineFamilyForFID.find(familyid);
		if ( it == mapAllOnlineFamilyForFID.end() )
		{
			sipwarning("There is no Family(id:%d) information For character(id:%d) in room", familyid, chid);
			continue;
		}
	}
	bool			bEnd;		_msgin.serial(bEnd);
	if ( !bEnd )
		return;

	uint nSize = mapAllOnlineFamilyForFID.size();

	map<T_FAMILYID,		FAMILYIINFO>::iterator	it;
	it = mapAllOnlineFamilyForFID.begin();
	while( nSize > 0 )
	{
		CMessage	msgout(M_GMSC_GETFAMILY);
		msgout.serial(uGMFID);
		uint32	nSendOnce = (nSize >= 15)? 15:nSize;	msgout.serial(nSendOnce);
		for ( uint32 i = 0; i < nSendOnce; i ++, it++ )
		{
			if (it == mapAllOnlineFamilyForFID.end())
				break;

			FAMILYIINFO	info = it->second;
			T_FAMILYID		familyid = info.FID;			msgout.serial(familyid);
			T_FAMILYNAME	familyName = info.FName;		msgout.serial(familyName);
			T_ACCOUNTID		userID = info.UserID;			msgout.serial(userID);
			uint8		uUserType = GetUserType(userID);	msgout.serial(uUserType);
			T_COMMON_NAME	userName = info.UserName;		msgout.serial(userName);
			T_ROOMID		roomID = info.RoomID;			msgout.serial(roomID);
			T_ROOMNAME		roomName = info.RoomName;		msgout.serial(roomName);
			bool			bInLobby = info.bInLobby;		msgout.serial(bInLobby);										
		}								
		TTcpSockId	sockGM = GetSocketIDForFamily(uGMFID);
		Clients->send(msgout, sockGM);
		sipinfo("Send to GM(id:%d) UserList(size:%d)", uGMFID, nSendOnce);

		nSize -= nSendOnce;
	}
	*/
}

// Client -> FS
// packet : M_GMCS_LOGOFFFAMILY
void cbC_M_GMCS_LOGOFFFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	FID;
	_msgin.serial(FID);
	if ( IsGM(FID) )
	{
		SendGMFail(ERR_GM_NOTOUCHGM, _from);
		return;
	}
	
	// GM Log
	T_FAMILYID GMID = GetFamilyID(_from);
	GMLog_LogoffFamily(GMID, FID);

	map<T_FAMILYID, T_FAMILYNAME>::iterator it = mapFamilyName.find(FID);
	if (it != mapFamilyName.end())
	{
		sipwarning(L"GM(accountid=%d) make logoffing for user(familyname=%s) forcely.", uIdenID, it->second.c_str());
		DisconnectClientForFamilyID(FID, L"By GM");
	}
	else
	{
		CMessage mes(M_GMCS_LOGOFFFAMILY);
		mes.serial(uIdenID, FID);
		CUnifiedNetwork::getInstance ()->send( FRONTEND_SHORT_NAME, mes, false );
	}
	CMessage mes(M_GMSC_LOGOFFFAMILY);
	mes.serial(uIdenID, FID);
	Clients->send(mes, _from);
}

// FS -> FS
// packet : M_GMCS_LOGOFFFAMILY
void cbS_M_GMCS_LOGOFFFAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32		uGMID;
	T_FAMILYID	FID;
	_msgin.serial(uGMID, FID);
	TTcpSockId	_from = GetSocketForIden(uGMID);
	if ( IsGM(FID) )
	{
		SendGMFail(ERR_GM_NOTOUCHGM, _from);
		return;
	}

	map<T_FAMILYID, T_FAMILYNAME>::iterator it = mapFamilyName.find(FID);
	if (it != mapFamilyName.end())
	{
		sipwarning(L"GM(accountid=%d) make logoffing for user(familyname=%s) forcely.", uGMID, it->second.c_str());
		DisconnectClientForFamilyID(FID, L"By GM");

//		CMessage mes(M_GMSC_LOGOFFFAMILY);
//		mes.serial(uGMID, FID);
//		Clients->send(mes, _from);
		return;
	}
}

void cbC_M_GMCS_OUTFAMILY( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	T_FAMILYID	FID;
	_msgin.serial(FID);
	if ( IsGM(FID) )
	{
		SendGMFail(ERR_GM_NOTOUCHGM, _from);
		return;
	}

	T_ROOMID	roomid = NOROOMID;
	map<T_FAMILYID,	FAMILYIINFO>::iterator itName = mapAllOnlineFamilyForFID.find(FID);
	if (itName != mapAllOnlineFamilyForFID.end())
	{
		roomid = itName->second.RoomID;
	}

	CMessage	mesOut(M_GMCS_OUTFAMILY);
	mesOut.serial(uGMFID, FID);
	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, mesOut, false);
	CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, mesOut, false);
	CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, mesOut, false);
}

void cbC_M_GMCS_ACTIVE( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!IsGM(uIdenID))
	{
		sipwarning("User(id:%d) is'nt GMType", uIdenID);
		return;
	}

	uint8	bOn, bGMTool;
	_msgin.serial(bOn, bGMTool);
	SetGMActive(uIdenID, bOn);

	uint32 uFID = GetFamilyID(_from);
	if( !bGMTool && uFID == 0 )
	{
		sipwarning("uFID is NULL. bGMTool=%d, uFID=%d", bGMTool, uFID);// 2014/04/09 
		return;
	}

	uint8	utype = GetUserType(uIdenID);
	CMessage	mes1(M_GMSC_ACTIVE);
	mes1.serial(uFID, bOn, utype);
	Clients->send (mes1, _from);
	if ( bOn )
		sipinfo("GM Activate id : %d", uIdenID);
	else
		sipinfo("GM Deactivate id : %d", uIdenID);

	if (utype == USERTYPE_WEBMANAGER) 
	{
		// 2014/04/22:방에 들어가지 않기때문에 구태여 openroom에 메씨지 내려보낼 필요 없다.
		sipinfo("GM is Web Manager!");
	}
	else
	{
		CMessage	mes2(M_GMCS_ACTIVE);
		mes2.serial(uFID, bOn, utype);
		CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, mes2, false);
		CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, mes2, false);
		CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, mes2, false);
	}

	if (bGMTool)
	{
		map<T_FAMILYID, T_FAMILYNAME>::iterator it = mapFamilyName.find(uFID);
		if (it != mapFamilyName.end())
		{
			T_FAMILYNAME GMName = it->second;
			CMessage mesNT(M_NT_ZAIXIANGM_LOGON);
			mesNT.serial(uFID, GMName);
			CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, mesNT, false);
		}
		else
		{
			sipwarning("There is no GM name : GMID=%d", uIdenID);
		}
	}
}

void cbC_M_GMCS_SHOW( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	bool	bShow;
	_msgin.serial(bShow);

	CMessage	mes1(M_GMSC_SHOW);
	mes1.serial(uGMFID, bShow);
	Clients->send (mes1, _from);

	CMessage	mes2(M_GMCS_SHOW);
	mes2.serial(uGMFID, bShow);
	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, mes2, false);
	CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, mes2, false);
	CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, mes2, false);
}

void cbC_M_GMCS_SETLEVEL( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	T_FAMILYID		FamilyID;
	T_F_LEVEL		uLevel;
	_msgin.serial(FamilyID, uLevel);
	if ( IsGM(FamilyID) )
	{
		SendGMFail(ERR_GM_NOTOUCHGM, _from);
		return;
	}


	T_ROOMID	roomid = NOROOMID;
	map<T_FAMILYID,	FAMILYIINFO>::iterator itName = mapAllOnlineFamilyForFID.find(FamilyID);
	if (itName != mapAllOnlineFamilyForFID.end())
		roomid = itName->second.RoomID;

	CMessage	mesOut(M_GMCS_SETLEVEL);
	mesOut.serial(uGMFID, FamilyID, uLevel);
	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, mesOut, false);
	CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, mesOut, false);
	CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, mesOut, false);
}

void cbC_M_GMCS_LISTCHANNEL( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	T_ROOMID	uRoomID;
	_msgin.serial(uRoomID);

	TServiceId sid;
	TRooms::iterator	it = RoomsInfo.find(uRoomID);
	if(it != RoomsInfo.end())
	{
		sid = it->second;
		CMessage	msgOut(M_GMCS_LISTCHANNEL);
		msgOut.serial(uGMFID, uRoomID);
		CUnifiedNetwork::getInstance ()->send( sid, msgOut );
	}
	else
	{
		uint32 uNum = 0;
		CMessage mes(M_GMSC_LISTCHANNEL);
		mes.serial(uRoomID, uNum);
		Clients->send (mes, _from);
	}
}

void cbC_GeneralGMPacket_ForMasterService( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	TServiceId	sid = OpenRoomService.GetMasterServiceID();

	if (sid.get() == 0)
		return;

	CMessage	msgOut;
	_msgin.copyWithPrefix(uGMFID, msgOut);
	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

void cbC_GeneralGMPacket_ForManagerService( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	CMessage	msgOut;
	_msgin.copyWithPrefix(uGMFID, msgOut);
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
}

void cbC_GeneralGMPacket_ForBestService( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	TServiceId	sid = OpenRoomService.GetBestServiceID();

	if (sid.get() == 0)
		return;

	CMessage	msgOut;
	_msgin.copyWithPrefix(uGMFID, msgOut);
	CUnifiedNetwork::getInstance ()->send( sid, msgOut );
}

void cbC_GeneralGMPacket_ForFamily( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	T_FAMILYID	FID;
	CMessage	msgTemp = _msgin;
	msgTemp.serial(FID);
	if ( IsGM(FID) )
	{
		SendGMFail(ERR_GM_NOTOUCHGM, _from);
		return;
	}

/*	if ( IsInLobby(FID) )
	{
		SendGMFail(ERR_GM_LOBBYOP, _from);
		return;
	}
*/
	map<T_FAMILYID,	FAMILYIINFO>::iterator itForRoom = mapAllOnlineFamilyForFID.find(FID);
	if (itForRoom != mapAllOnlineFamilyForFID.end())
	{
		if ( itForRoom->second.bInLobby )
		{
			T_ROOMID	lobby_id = itForRoom->second.RoomID;
			sipinfo("_msgin=%s, FID=%d, lobby_id=%d", _msgin.toString().c_str(), FID, lobby_id);

			CMessage	msgout;
			_msgin.copyWithPrefix(uGMFID, msgout);

			TServiceId sid(0);
			if ( s_Lobbys.find(lobby_id) != s_Lobbys.end() )
			{
				sid = s_Lobbys[lobby_id];
				CUnifiedNetwork::getInstance()->send(sid, msgout);
			}
			else
				sipwarning("Can't process packet because TServiceId is NULL: lobby_id=%d", lobby_id);
		}
		else
		{
			TServiceId	sid = OpenRoomService.GetSIDFromFID(FID);
			if (sid.get() != 0)
			{
				CMessage	msgOut;
				_msgin.copyWithPrefix(uGMFID, msgOut);
				CUnifiedNetwork::getInstance ()->send( sid, msgOut );
			}
			else
				sipwarning("Can't process packet because TServiceId is NULL: room_id=%d", itForRoom->second.RoomID);
		}
	}
	else
	{
		TServiceId	sid = OpenRoomService.GetBestServiceID();
		if (sid.get() == 0)
			return;
		CMessage	msgOut;
		_msgin.copyWithPrefix(uGMFID, msgOut);
		CUnifiedNetwork::getInstance ()->send( sid, msgOut );
	}
}

void cbC_WebGMPacket_ForUser( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

	T_FAMILYID	userID;
	CMessage	msgTemp = _msgin;
	msgTemp.serial(userID);
	if ( IsGM(userID) )
	{
		SendGMFail(ERR_GM_NOTOUCHGM, _from);
		return;
	}

/*	if ( IsInLobby(FID) )
	{
		SendGMFail(ERR_GM_LOBBYOP, _from);
		return;
	}
*/
	BOOL bOnLine = true;
	T_FAMILYID FID;
	map<T_ACCOUNTID,	T_FAMILYID>::iterator ifForFID = mapAllOnlineFIDForUserID.find(userID);
	if ( ifForFID != mapAllOnlineFIDForUserID.end() )
	{
		FID = ifForFID->second;
		map<T_ACCOUNTID,	FAMILYIINFO>::iterator itForRoom = mapAllOnlineFamilyForFID.find(FID);
		if (itForRoom != mapAllOnlineFamilyForFID.end())
		{
			bOnLine = true;

			if ( itForRoom->second.bInLobby )
			{
				T_ROOMID	lobby_id = itForRoom->second.RoomID;
				sipinfo("_msgin=%s, FID=%d, lobby_id=%d", _msgin.toString().c_str(), FID, lobby_id);

				CMessage	msgout;
				_msgin.copyWithPrefix(uGMFID, msgout);

				TServiceId sid(0);
				if ( s_Lobbys.find(lobby_id) != s_Lobbys.end() )
				{
					sid = s_Lobbys[lobby_id];
					CUnifiedNetwork::getInstance()->send(sid, msgout);
				}
				else
					sipwarning("Can't process packet because TServiceId is NULL: lobby_id=%d", lobby_id);
			}
			else
			{
				TServiceId	sid = OpenRoomService.GetSIDFromFID(FID);
				if (sid.get() != 0)
				{
					CMessage	msgOut;
					_msgin.copyWithPrefix(uGMFID, msgOut);
					CUnifiedNetwork::getInstance ()->send( sid, msgOut );
				}
				else
					sipwarning("Can't process packet because TServiceId is NULL: room_id=%d", itForRoom->second.RoomID);
			}
		}
		else
		{
			bOnLine = false;
		}
	}
	else
	{
		bOnLine = false;
	}

	if ( ! bOnLine )
	{
		TServiceId	sid = OpenRoomService.GetBestServiceID();
		if (sid.get() == 0)
			return;
		CMessage	msgOut;
		_msgin.copyWithPrefix(uGMFID, msgOut);
		CUnifiedNetwork::getInstance ()->send( sid, msgOut );
	}
}

void cbC_GeneralGMPacket_ForOpenRoomForRoomID( CMessage& _msgin, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uIdenID = GetIdentifiedID(_from);
	if (!AbleGMPacketAccept(uIdenID, _msgin.getName()))
		return;

	T_FAMILYID	uGMFID = GetFamilyID(_from);
	if (uGMFID == 0)
		return;

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
	_msgin.copyWithPrefix(uGMFID, msgOut);
	CUnifiedNetwork::getInstance()->send( sid, msgOut );
}

/************************************************************************/
/*		Silence                                                           */
/************************************************************************/
static int g_nTimeDiffFromDBTime;
static map<T_FAMILYID, uint32>	g_mapSilencePlayers;
void cbS_M_NT_SET_DBTIME(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32		DBTime;
	_msgin.serial(DBTime);

	g_nTimeDiffFromDBTime = (int)(DBTime - CTime::getSecondsSince1970());
}

uint32 GetDBTimeSecond()
{
	return (uint32)(CTime::getSecondsSince1970() + g_nTimeDiffFromDBTime);
}

void cbS_M_NT_SILENCE_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		RemainTime;

	_msgin.serial(FID, RemainTime);

	if (RemainTime == 0)
	{
		g_mapSilencePlayers.erase(FID);
	}
	else
	{
		g_mapSilencePlayers[FID] = RemainTime;
	}

	TTcpSockId sid = GetSocketIDForFamily(FID);

	if (sid != 0)
	{
		CMessage	msgOut(M_SC_SILENCE_TIME);
		msgOut.serial(RemainTime);

		Clients->send (msgOut, sid);
	}
}

void SendSilentToGM(TTcpSockId _from)
{
	uint32	now = (uint32)GetDBTimeSecond();

	for (map<T_FAMILYID, FAMILYIINFO>::iterator it = mapAllOnlineFamilyForFID.begin(); it != mapAllOnlineFamilyForFID.end(); it ++)
	{
		T_FAMILYID		FID = it->first;

		map<T_FAMILYID, uint32>::iterator it1 = g_mapSilencePlayers.find(FID);
		if (it1 != g_mapSilencePlayers.end())
		{
			if (it1->second <= now)
			{
				g_mapSilencePlayers.erase(FID);
			}
			else
			{
				CMessage	msgOut(M_GMSC_SILENCE_FAMILY);
				uint32	RemainTime = it1->second - now;
				msgOut.serial(FID, RemainTime);
				Clients->send(msgOut, _from);
			}
		}
	}
}

bool CheckIfSilencePlayer(T_FAMILYID FID, bool bSendInfo)
{
	map<T_FAMILYID, uint32>::iterator it = g_mapSilencePlayers.find(FID);
	if (it == g_mapSilencePlayers.end())
		return false;

	uint32	now = (uint32)GetDBTimeSecond();
	if (it->second <= now)
	{
		g_mapSilencePlayers.erase(FID);
		return false;
	}

	if (bSendInfo)
	{
		TTcpSockId sid = GetSocketIDForFamily(FID);

		if (sid != 0)
		{
			CMessage	msgOut(M_SC_SILENCE_TIME);
			msgOut.serial(it->second);

			Clients->send (msgOut, sid);
		}
		else
		{
			sipwarning("sid=0. FID=%d", FID);
		}
	}
	return true;
}
