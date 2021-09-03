#ifndef TIAN_FRONTEND_SHARDCLIENT_H
#define TIAN_FRONTEND_SHARDCLIENT_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "MultiService.h"

extern	SIPBASE::CTwinMap<T_FAMILYID, SIPNET::TTcpSockId>	twinmapFIDandSID;
extern	std::map<T_FAMILYID, T_FAMILYNAME>				mapFamilyName;
//---extern	std::map<T_FAMILYNAME, T_FAMILYID>				mapFamilyFromName;

extern	T_FAMILYID			GetFamilyID(SIPNET::TTcpSockId _sid);
extern	SIPNET::TTcpSockId	GetSocketIDForFamily(T_FAMILYID _uid);

struct	FAMILYIINFO
{
	FAMILYIINFO(T_FAMILYID _FID, T_FAMILYNAME _FName, T_ACCOUNTID _UserID, T_COMMON_NAME _UserName, uint8 _bIsMobile, uint32 _ModelID) :
		FID(_FID), FName(_FName), UserID(_UserID), UserName(_UserName), RoomID(0), RoomChannelID(0), bInLobby(false), bIsMobile(_bIsMobile), ModelID(_ModelID) {}//, CHID(0), CHName("") {}
	T_FAMILYID		FID;
	T_FAMILYNAME	FName;
	T_ACCOUNTID		UserID;
	T_COMMON_NAME	UserName;
	T_ROOMID		RoomID;
	T_ROOMCHANNELID	RoomChannelID;
	T_ROOMNAME		RoomName;
	bool			bInLobby;
	uint8			bIsMobile;
	uint32			ModelID;
	// Only In Room/Lobby
//	T_CHARACTERID	CHID;
//	T_CHARACTERNAME	CHName;
};

extern	std::map<T_FAMILYID,	FAMILYIINFO>		mapAllOnlineFamilyForFID;
extern	std::map<T_ACCOUNTID,	T_FAMILYID>		mapAllOnlineFIDForUserID;
//---extern	std::map<T_FAMILYNAME,	FAMILYIINFO>		mapAllOnlineFamilyForFName;

extern	CMultiService<uint32>	OpenRoomService;
extern  CMultiService<uint32>	LobbyService;

extern void	DisconnectClientForUserID(uint32 _uid);
extern void	DisconnectClientForFamilyID(T_FAMILYID _fid, ucstring _ucReason);

typedef std::map<T_ROOMID, TServiceId>		TLobbys;		//lobby id vs TServiceId
extern	TLobbys	s_Lobbys;
typedef std::map<T_ROOMID, TServiceId>		TRooms;			//RoomID vs TServiceId
extern	TRooms	RoomsInfo;

typedef std::map<SIPNET::TTcpSockId, TServiceId> TLobbySIDs; // the client from VS lobby ServiceId
extern TLobbySIDs s_lobbySID;
#endif	// TIAN_FRONTEND_SHARDCLIENT_H
