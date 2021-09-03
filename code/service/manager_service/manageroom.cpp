#include <misc/ucstring.h>
#include <net/service.h>

#include "misc/DBCaller.h"
#include "../../common/Macro.h"
#include "../../common/Packet.h"
#include "../../common/err.h"
#include "../Common/Lang.h"
#include "../Common/QuerySetForShard.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

#include "manageroom.h"
#include "main.h"
#include "AutoPlayer.h"
#include "player.h"
#include "toplist.h"

extern CDBCaller	*DBCaller;


//================================================
// Global Variables
//================================================
std::map<T_ROOMID, Room>	g_Rooms;	// 완전히 창조된 방목록
std::map<T_ROOMID, T_ROOMID>	g_RoomCreatingIDs;	// 현재 창조중인 방목록
std::list<RoomCreateWait>	g_RoomCreateRequests;		// 방창조대기렬

std::map<T_ROOMID, uint32>	g_RoomPromotionRequests;	// 승급요청된 방목록, 승급요청된 방목록은 일정한 시간동안 들어갈수 없다. - CTime::getSecondsSince1970()

std::map<uint32, RoomEnterWait>	g_RoomEnterWaits;
std::map<uint32, RoomChangemasterWait>	g_RoomChangemasterWaits;
std::map<uint32, RoomChangemasterAnsWait>	g_RoomChangemasterAnsWaits;
std::map<uint32, RoomChangemasterCancelWait>	g_RoomChangemasterCancelWaits;
std::map<uint32, RoomChangemasterForceWait>	g_RoomChangemasterForceWaits;

//std::map<uint8, uint8>	g_mapOwnShardCodes;
uint8	ShardMainCode[25] = {0};



void SendEnterroom(uint32 UnitID);
void SendCheckEnterroom(uint32 UnitID);
void SendChangemasterReq(uint32 UnitID);
void SendChangemasterAns(uint32 UnitID);
void SendChangemasterCancel(uint32 UnitID);
void SendChangemasterForce(uint32 UnitID);

//================================================
// Global Functions
//================================================

bool IsOpenroom(uint32 RoomID)
{
	if (RoomID >= 0x01000000)
		return true;
	return false;
}

bool IsThisShardRoom(uint32 RoomID)
{
	if (!IsOpenroom(RoomID))
		return true;

	uint8	ShardCode = (uint8)(RoomID >> 24);
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		if (ShardMainCode[i] == ShardCode)
			return true;
	}
	return false;
}

bool IsThisShardFID(uint32 FID)
{
	uint8	ShardCode = (uint8)((FID >> 24) + 1);
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		if (ShardMainCode[i] == ShardCode)
			return true;
	}
	return false;
}

//void cb_M_WM_SHARDCODELIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	ucstring	ucShardCodeList;
//	_msgin.serial(ucShardCodeList);
//
//	sipinfo(L"Receive ShardCodeList : %s", ucShardCodeList.c_str());
//
//	uint8	ShardCode;
//	ucstring	strShardCode;
//	int		pos;
//	do
//	{
//		pos = ucShardCodeList.find(L",");
//		if(pos >= 0)
//		{
//			strShardCode = ucShardCodeList.substr(0, pos);
//			ucShardCodeList = ucShardCodeList.substr(pos + 1);
//		}
//		else
//		{
//			strShardCode = ucShardCodeList;
//		}
//		ShardCode = (uint8)_wtoi(strShardCode.c_str());
//
//		g_mapOwnShardCodes[ShardCode] = ShardCode;
//	} while(pos >= 0);
//}

// Send Shard Code from WS to Lobby [[b:ShardMainCode]]
void	cb_M_W_SHARDCODE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	int count = 0;
	while (true)
	{
		_msgin.serial(ShardMainCode[count]);
		if (ShardMainCode[count] == 0)
			break;
		count ++;
	}

	LoadTopList();

	sipdebug("Get Main count=%d, ShardCode[0] = %d", count, ShardMainCode[0]);
}

void CreateReligionRoom(uint32 rroomid)
{
	CMessage	msgOut(M_NT_ROOM_CREATE_REQ);
	msgOut.serial(rroomid);
	CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, msgOut);
}

void CreateReligionRooms()
{
	// 방들을 창조한다.
	std::string s_rroomids, s_rroomid;
	s_rroomids = IService::getInstance()->ConfigFile.getVar("RRoomIDList").asString();

	uint32 rroomid;
	size_t n = s_rroomids.find(',');
	while (n != std::string::npos)
	{
		s_rroomid = s_rroomids.substr(0, n);

		rroomid = atoi(s_rroomid.c_str());
		CreateReligionRoom(rroomid);

		s_rroomids = s_rroomids.substr(n + 1);
		n = s_rroomids.find(',');
	}
	rroomid = atoi(s_rroomids.c_str());
	CreateReligionRoom(rroomid);
}

bool CreateOpenRoom(T_ROOMID RoomID, uint8 Reason, uint32 Param, T_FAMILYID FID)
{
	sipinfo("Create Room need. RoomID=%d, Reason=%d", RoomID, Reason); // byy krs

	if (g_Rooms.find(RoomID) != g_Rooms.end())
		return true;

	// 이 Shard의 방이 아닌지 검사한다.
	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("This is not this shard's room! RoomID=%d", RoomID);
		return false;
	}

	// 방이 승급중인지 검사한다.
	std::map<T_ROOMID, uint32>::iterator	it = g_RoomPromotionRequests.find(RoomID);
	if (it != g_RoomPromotionRequests.end())
	{
		if (CTime::getSecondsSince1970() - it->second <= 5)	// 승급중인 방에는 5초동안 들어갈수 없다.
		{
			// 이 순간을 맞춘 사용자는 운수가 나쁘게 아무 응답도 받지 못할것이다!!!
			sipwarning("Room is now promotioning so can't create room. RoomID=%d, Reason=%d", RoomID, Reason);
			return false;
		}
		g_RoomPromotionRequests.erase(it);
	}

	RoomCreateWait	data(RoomID, Reason, Param);
	g_RoomCreateRequests.push_back(data);

	if (g_RoomCreatingIDs.find(RoomID) == g_RoomCreatingIDs.end())
	{
		g_RoomCreatingIDs[RoomID] = RoomID;

		CMessage	msgOut(M_NT_ROOM_CREATE_REQ);
		msgOut.serial(RoomID, FID);
		SendMsgToOpenroomForCreate(RoomID, msgOut);
	}

	return false;
}

// Notice room created. OpenRoomService->ManagerService ([d:RoomID][w:SceneID][u:RoomName][d:MasterFID])
void cb_M_NT_ROOM_CREATED(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID		RoomID;
	uint32			Result;
	T_SCENEID		SceneID = 0;
	ucstring		RoomName = L"";
	T_FAMILYID		MasterFID = 0;

	_msgin.serial(RoomID, Result);
	if (Result == 0)
		_msgin.serial(SceneID, RoomName, MasterFID);

	if (g_Rooms.find(RoomID) != g_Rooms.end())
	{
		sipwarning("g_Rooms.find(RoomID) != g_Rooms.end(). RoomID=%d", RoomID);
		return;
	}

	if (Result == 0)
	{
		Room	data(RoomID, SceneID, RoomName, MasterFID, _tsid);
		g_Rooms[RoomID] = data;

		// 방주인의 Contact목록을 읽는다.
		if (IsOpenroom(RoomID)) {
			sipinfo("MasterFID = %d, RoomID = %d", MasterFID, RoomID);
			RequestFriendListInfo(MasterFID, RoomID);
		}
		else
			DoAfterRoomCreated(RoomID);

		sipdebug(L"Room created: RoomID=%d", RoomID); // byy krs
	}
	else
	{
		// 방창조 실패 후처리 진행 - 이미전의 요청들을 다 삭제한다.
		std::map<T_ROOMID, T_ROOMID>::iterator it1 = g_RoomCreatingIDs.find(RoomID);
		if (it1 != g_RoomCreatingIDs.end())
		{
			g_RoomCreatingIDs.erase(it1);
		}

		for (std::list<RoomCreateWait>::iterator it = g_RoomCreateRequests.begin(); it != g_RoomCreateRequests.end(); )
		{
			if (it->m_RoomID != RoomID)
			{
				it ++;
				continue;
			}

			if (it->m_Reason == RoomCreateWait::CREATE_ROOM_REASON_AUTOPLAY)
				RemoveAutoplayDatas(RoomID, Result);

			it = g_RoomCreateRequests.erase(it);
		}

		sipwarning(L"Room create failed. RoomID=%d, Result=%d", RoomID, Result);
	}
}

void DoAfterRoomCreated(T_ROOMID RoomID)
{
	// 방이 창조된 다음의 후처리 진행
	std::map<T_ROOMID, T_ROOMID>::iterator it1 = g_RoomCreatingIDs.find(RoomID);
	if (it1 == g_RoomCreatingIDs.end())
		return;

	g_RoomCreatingIDs.erase(it1);

	for (std::list<RoomCreateWait>::iterator it = g_RoomCreateRequests.begin(); it != g_RoomCreateRequests.end(); )
	{
		if (it->m_RoomID != RoomID)
		{
			it ++;
			continue;
		}

		switch (it->m_Reason)
		{
		case RoomCreateWait::CREATE_ROOM_REASON_ENTER:
			SendEnterroom(it->m_Param);
			break;
		case RoomCreateWait::CREATE_ROOM_REASON_CHECK_ENTER:
			SendCheckEnterroom(it->m_Param);
			break;
		case RoomCreateWait::CREATE_ROOM_REASON_CHANGEMASTER_REQUEST:
			SendChangemasterReq(it->m_Param);
			break;
		case RoomCreateWait::CREATE_ROOM_REASON_CHANGEMASTER_CANCEL:
			SendChangemasterCancel(it->m_Param);
			break;
		case RoomCreateWait::CREATE_ROOM_REASON_CHANGEMASTER_ANSWER:
			SendChangemasterAns(it->m_Param);
			break;
		case RoomCreateWait::CREATE_ROOM_REASON_CHANGEMASTER_FORCE:
			SendChangemasterForce(it->m_Param);
			break;
		case RoomCreateWait::CREATE_ROOM_REASON_AUTOPLAY:
			RegisterActionMemo(it->m_Param);
			break;
		default:
			sipwarning("Invalid Reason. Reason=%d", it->m_Reason);
			break;
		}
		it = g_RoomCreateRequests.erase(it);
	}
}

// Notice room deleted. OpenRoomService->ManagerService ([d:RoomID])
void cb_M_NT_ROOM_DELETED(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID		RoomID;

	_msgin.serial(RoomID);

	std::map<T_ROOMID, Room>::iterator it = g_Rooms.find(RoomID);
	if (it == g_Rooms.end())
	{
		sipwarning("it == g_Rooms.end(). RoomID=%d", RoomID);
		return;
	}

	g_Rooms.erase(it);
	//sipdebug(L"Room deleted: RoomID=%d", RoomID); byy krs
}

void SendMsgToRoomID(T_ROOMID RoomID, CMessage &msgOut)
{
	if (RoomID == 0)
		CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, msgOut);
	else
	{
		Room*	pRoom = GetRoom(RoomID);
		if (pRoom == NULL)
		{
			if (msgOut.getName() == M_CS_CANCELREQEROOM)
				sipinfo("GetRoom = NULL. RoomID=%d, MsgID=%d", RoomID, msgOut.getName());
			else
				sipwarning("GetRoom = NULL. RoomID=%d, MsgID=%d", RoomID, msgOut.getName());
			return;
		}

		CUnifiedNetwork::getInstance()->send(pRoom->m_RoomSID, msgOut);
	}
}

void SendEnterroom(uint32 UnitID)
{
	std::map<uint32, RoomEnterWait>::iterator it = g_RoomEnterWaits.find(UnitID);
	if (it == g_RoomEnterWaits.end())
	{
		sipwarning("it == g_RoomEnterWaits.end(). UnitID=%d", UnitID);
		return;
	}

	Player *pPlayer = GetPlayer(it->second.m_FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", it->second.m_FID);
		return;
	}

	Room *pRoom = GetRoom(it->second.m_RoomID);
	if (pRoom == NULL)
	{
		sipwarning("GetRoom = NULL. it->second.m_RoomID=%d", it->second.m_RoomID);
		return;
	}

	uint16		client_FsSid = pPlayer->m_FSSid.get();
	uint8		ContactStatus = CheckContactStatus(pRoom->m_MasterFID, it->second.m_FID);

	CMessage	msgOut(M_CS_REQEROOM);
	msgOut.serial(it->second.m_FID, it->second.m_RoomID, it->second.m_AuthKey, it->second.m_ChannelID, client_FsSid, ContactStatus);
	SendMsgToRoomID(it->second.m_RoomID, msgOut);

	g_RoomEnterWaits.erase(it);
}

// Request entering room([d:Room id][s:AuthKey][d:ChannelID]) ChannelID는 특별한 용도에 사용됨 (집체행사초청인경우)
static uint32	UnitID = 1;
void cb_M_CS_REQEROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	string			AuthKey;
	uint32			ChannelID;

	_msgin.serial(FID, RoomID, AuthKey, ChannelID);

	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. RoomID=%d, FID=%d", RoomID, FID);
		return;
	}

	uint32	curUnitID = UnitID; UnitID ++;
	RoomEnterWait	data(FID, RoomID, AuthKey, ChannelID);
	g_RoomEnterWaits[curUnitID] = data;

	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_ENTER, curUnitID, FID))
		SendEnterroom(curUnitID);
}

void cb_M_CS_CANCELREQEROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	_msgin.serial(FID, RoomID);

	CMessage msgOut(M_CS_CANCELREQEROOM);
	msgOut.serial(FID, RoomID);

	SendMsgToRoomID(RoomID, msgOut);
}

void SendCheckEnterroom(uint32 UnitID)
{
	std::map<uint32, RoomEnterWait>::iterator it = g_RoomEnterWaits.find(UnitID);
	if (it == g_RoomEnterWaits.end())
	{
		sipwarning("it == g_RoomEnterWaits.end(). UnitID=%d", UnitID);
		return;
	}

	Player *pPlayer = GetPlayer(it->second.m_FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", it->second.m_FID);
		return;
	}

	Room *pRoom = GetRoom(it->second.m_RoomID);
	if (pRoom == NULL)
	{
		sipwarning("GetRoom = NULL. it->second.m_RoomID=%d", it->second.m_RoomID);
		return;
	}

	uint16	client_FsSid = pPlayer->m_FSSid.get();
	uint8	ContactStatus = CheckContactStatus(pRoom->m_MasterFID, it->second.m_FID);

	CMessage	msgOut(M_CS_CHECK_ENTERROOM);
	msgOut.serial(it->second.m_FID, it->second.m_RoomID, it->second.m_AuthKey, client_FsSid, ContactStatus);
	SendMsgToRoomID(it->second.m_RoomID, msgOut);

	g_RoomEnterWaits.erase(it);
}

// Check enter room ([d:Room id][s:AuthKey])
void cb_M_CS_CHECK_ENTERROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	string			AuthKey;

	_msgin.serial(FID, RoomID, AuthKey);

	uint32	curUnitID = UnitID; UnitID ++;
	RoomEnterWait	data(FID, RoomID, AuthKey, 0);
	g_RoomEnterWaits[curUnitID] = data;

	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_CHECK_ENTER, curUnitID, FID))
		SendCheckEnterroom(curUnitID);
}

void SendChangemasterReq(uint32 UnitID)
{
	std::map<uint32, RoomChangemasterWait>::iterator it = g_RoomChangemasterWaits.find(UnitID);
	if (it == g_RoomChangemasterWaits.end())
	{
		sipwarning("it == g_RoomChangemasterWaits.end(). UnitID=%d", UnitID);
		return;
	}

	Player *pPlayer = GetPlayer(it->second.m_FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", it->second.m_FID);
		return;
	}

	uint16		client_FsSid = pPlayer->m_FSSid.get();
	CMessage	msgOut(M_CS_CHANGEMASTER);
	msgOut.serial(it->second.m_FID, it->second.m_RoomID, it->second.m_toFID, it->second.m_TwoPassword, client_FsSid);
	SendMsgToRoomID(it->second.m_RoomID, msgOut);

	g_RoomChangemasterWaits.erase(it);
}

// Give room([d:Room id][d:toFID][s:TwoPassword])
void cb_M_CS_CHANGEMASTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	static uint32	UnitID = 1;

	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	T_ROOMID			RoomID;				_msgin.serial(RoomID);				
	T_FAMILYID			NextID;				_msgin.serial(NextID);			// 넘겨주려는 가족id
	string				password2;			_msgin.serial(password2);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}
	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}
	if (!IsThisShardFID(NextID))
	{
		sipwarning("IsThisShardFID(NextID) = false. FID=%d, NextFID=%d", FID, NextID);
		return;
	}

	uint32	curUnitID = UnitID; UnitID ++;
	RoomChangemasterWait	data(FID, RoomID, NextID, password2);
	g_RoomChangemasterWaits[curUnitID] = data;

	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_CHANGEMASTER_REQUEST, curUnitID, FID))
		SendChangemasterReq(curUnitID);
}

void SendChangemasterAns(uint32 UnitID)
{
	std::map<uint32, RoomChangemasterAnsWait>::iterator it = g_RoomChangemasterAnsWaits.find(UnitID);
	if (it == g_RoomChangemasterAnsWaits.end())
	{
		sipwarning("it == g_RoomChangemasterAnsWaits.end(). UnitID=%d", UnitID);
		return;
	}

	Player *pPlayer = GetPlayer(it->second.m_FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", it->second.m_FID);
		return;
	}

	uint16		client_FsSid = pPlayer->m_FSSid.get();
	CMessage	msgOut(M_CS_CHANGEMASTER_ANS);
	msgOut.serial(it->second.m_FID, it->second.m_RoomID, it->second.m_Answer, client_FsSid);
	SendMsgToRoomID(it->second.m_RoomID, msgOut);

	g_RoomChangemasterAnsWaits.erase(it);
}

// Answer of M_SC_CHANGEMASTER_REQ ([d:Room id][b:Answer, 1:Yes,0:No])
void cb_M_CS_CHANGEMASTER_ANS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	static uint32	UnitID = 1;

	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ROOMID			RoomID;				_msgin.serial(RoomID);				
	uint8				Answer;				_msgin.serial(Answer);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}
	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	uint32	curUnitID = UnitID; UnitID ++;
	RoomChangemasterAnsWait	data(FID, RoomID, Answer);
	g_RoomChangemasterAnsWaits[curUnitID] = data;

	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_CHANGEMASTER_ANSWER, curUnitID, FID))
		SendChangemasterAns(curUnitID);
}

void SendChangemasterCancel(uint32 UnitID)
{
	std::map<uint32, RoomChangemasterCancelWait>::iterator it = g_RoomChangemasterCancelWaits.find(UnitID);
	if (it == g_RoomChangemasterCancelWaits.end())
	{
		sipwarning("it == g_RoomChangemasterCancelWaits.end(). UnitID=%d", UnitID);
		return;
	}

	Player *pPlayer = GetPlayer(it->second.m_FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", it->second.m_FID);
		return;
	}

	uint16		client_FsSid = pPlayer->m_FSSid.get();
	CMessage	msgOut(M_CS_CHANGEMASTER_CANCEL);
	msgOut.serial(it->second.m_FID, it->second.m_RoomID, it->second.m_toFID, client_FsSid);
	SendMsgToRoomID(it->second.m_RoomID, msgOut);

	g_RoomChangemasterCancelWaits.erase(it);
}

// Cancel giveroom (by timeout, etc...) ([d:Room id][d:toFID])
void cb_M_CS_CHANGEMASTER_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	static uint32	UnitID = 1;

	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ROOMID			RoomID;				_msgin.serial(RoomID);				
	T_FAMILYID			NextID;				_msgin.serial(NextID);

	uint32	curUnitID = UnitID; UnitID ++;
	RoomChangemasterCancelWait	data(FID, RoomID, NextID);
	g_RoomChangemasterCancelWaits[curUnitID] = data;

	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_CHANGEMASTER_CANCEL, curUnitID, FID))
		SendChangemasterCancel(curUnitID);
}

void SendChangemasterForce(uint32 UnitID)
{
	std::map<uint32, RoomChangemasterForceWait>::iterator it = g_RoomChangemasterForceWaits.find(UnitID);
	if (it == g_RoomChangemasterForceWaits.end())
	{
		sipwarning("it == RoomChangemasterForceWait.end(). UnitID=%d", UnitID);
		return;
	}

	uint16		client_FsSid = 0;
	uint8		answer = 1;
	CMessage	msgOut(M_NT_CHANGEMASTER_FORCE);
	msgOut.serial(it->second.m_FID, it->second.m_RoomID);
	SendMsgToRoomID(it->second.m_RoomID, msgOut);

	g_RoomChangemasterForceWaits.erase(it);
}

void ChangeRoomMasterForce(T_ROOMID RoomID, T_FAMILYID NewFID)
{
	static uint32	UnitID = 1;

	uint32	curUnitID = UnitID; UnitID ++;
	RoomChangemasterForceWait	data(NewFID, RoomID);
	g_RoomChangemasterForceWaits[curUnitID] = data;

	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_CHANGEMASTER_FORCE, curUnitID, NewFID))
		SendChangemasterForce(curUnitID);
}

Room* GetRoom(T_ROOMID RoomID)
{
	std::map<T_ROOMID, Room>::iterator it = g_Rooms.find(RoomID);
	if (it == g_Rooms.end())
		return NULL;
	return &it->second;
}

static void	DBCB_DBGetAuditingErrList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows, i;
	resSet->serial(nRows);
	for (i = 0; i < nRows; i ++)
	{
		uint32		RoomID;		resSet->serial(RoomID);
		Room*	pRoom = GetRoom(RoomID);
		if (pRoom != NULL)
		{
			CMessage	msgOut(M_NT_RELOAD_ROOMINFO);
			msgOut.serial(RoomID);
			CUnifiedNetwork::getInstance()->send(pRoom->m_RoomSID, msgOut);
		}
	}
}

void LoadAuditErrRooms()
{
	MAKEQUERY_GetAuditingErrList(strQ);
	DBCaller->executeWithParam(strQ, DBCB_DBGetAuditingErrList,
		"");
}

void	cb_M_CS_PROMOTION_ROOM_Manager(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	uint16			LobbySID;
	uint16			SceneID = 0;
	uint16			PriceID = 0;
	string			CardID;

	_msgin.serial(FID, RoomID);
	if (_msgin.getName() == M_CS_PROMOTION_ROOM)
		_msgin.serial(SceneID, PriceID);
	else	// M_CS_ROOMCARD_PROMOTION
		_msgin.serial(CardID);
	_msgin.serial(LobbySID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	if (g_RoomCreatingIDs.find(RoomID) != g_RoomCreatingIDs.end())
	{
		uint32	result = E_NOEMPTY;
		CMessage	msgout(M_SC_PROMOTION_ROOM);
		msgout.serial(FID, result, RoomID);

		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
		return;
	}

	g_RoomPromotionRequests[RoomID] = CTime::getSecondsSince1970();

	Room*	pRoom = GetRoom(RoomID);
	if (pRoom != NULL)
	{
		uint16		client_FsSid = _tsid.get();
		CMessage	msgout(M_CS_PROMOTION_ROOM);
		msgout.serial(FID, RoomID, SceneID, PriceID, CardID, LobbySID, client_FsSid);

		CUnifiedNetwork::getInstance()->send(pRoom->m_RoomSID, msgout);
		return;
	}

	{
		CMessage	msgout(M_CS_PROMOTION_ROOM);
		msgout.serial(FID, RoomID, SceneID, PriceID, CardID);

		TServiceId	Lobby(LobbySID);
		CUnifiedNetwork::getInstance()->send(Lobby, msgout);
	}
}

//void	cb_M_CS_ROOMCARD_PROMOTION_Manager(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	sipwarning("Not supported now");
//}


//static void	DBCB_DBDeleteNonusedFreeRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	if (!nQueryResult)
//		return;
//
//	ucstring	empty = L"";
//	uint32 nRows, i;
//	resSet->serial(nRows);
//	for (i = 0; i < nRows; i ++)
//	{
//		uint32		RoomID;		resSet->serial(RoomID);
//		ucstring	RoomName;	resSet->serial(RoomName);
//		uint32		FID;		resSet->serial(FID);
//
//		SendChit(CHITADDTYPE_ADD_ONLY, FID, FID, CHITTYPE_ROOM_DELETED, RoomID, 0, RoomName, empty);
//
//		sipinfo(L"Room Deleted. RoomID=%d, MasterFID=%d", RoomID, FID);
//	}
//}

//void DeleteNonusedFreeRooms()
//{
//	MAKEQUERY_DeleteNonusedFreeRoom(strQ);
//	DBCaller->executeWithParam(strQ, DBCB_DBDeleteNonusedFreeRoom,
//		"");
//}

//================================================
// RoomCreateWait Functions
//================================================
RoomCreateWait::RoomCreateWait()
{
}

RoomCreateWait::~RoomCreateWait()
{
}

RoomCreateWait::RoomCreateWait(T_ROOMID _RoomID, uint8 _Reason, uint32 _Param)
	: m_RoomID(_RoomID), m_Reason(_Reason), m_Param(_Param)
{
}

//================================================
// Room Functions
//================================================
Room::Room() : m_nPlayerCount(0)
{
}

Room::Room(T_ROOMID _RoomID, T_SCENEID _SceneID, ucstring	_RoomName, T_FAMILYID _MasterFID, TServiceId _tsid)
	: m_nPlayerCount(0), m_RoomID(_RoomID), m_SceneID(_SceneID), m_RoomName(_RoomName), m_MasterFID(_MasterFID), m_RoomSID(_tsid)
{
}

Room::~Room()
{

}
