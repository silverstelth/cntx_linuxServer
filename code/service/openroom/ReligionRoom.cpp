#include <misc/ucstring.h>
#include <net/service.h>

#include "ReligionRoom.h"
#include "PublicRoom.h"
#include "openroom.h"
#include "Character.h"
#include "Family.h"
#include "mst_room.h"
#include "outRoomKind.h"
#include "room.h"
#include "contact.h"
#include "Lobby.h"
#include "Inven.h"
#include "ChannelWorld.h"

#include "../Common/DBLog.h"
#include "../Common/netCommon.h"
#include "misc/DBCaller.h"
#include "ParseArgs.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller	*DBCaller;

static void	ResetPBYHisBuffer();
static void	ResetPBYMyHisBuffer(T_FAMILYID FID);

#define DELEGATEPROC_TO_CHANNEL(packetname, _msgin, _tsid)	\
T_FAMILYID	FID;	\
_msgin.serial(FID);	\
CReligionWorld *inManager = GetReligionWorldFromFID(FID);	\
if (inManager != NULL)	\
{	\
	ReligionChannelData	*pChannelData = inManager->GetFamilyChannel(FID);	\
	if (pChannelData)	\
	{	\
	pChannelData->on_##packetname(FID, _msgin, _tsid);	\
	}	\
	else	\
	{	\
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);	\
		return;	\
	}	\
}	\
else	\
{	\
	sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);	\
	return;		\
}	\


static CReligionWorld* GetReligionWorld(T_ROOMID _rid)
{
	CChannelWorld* pChannelWorld = GetChannelWorld(_rid);
	if (pChannelWorld == NULL)
		return NULL;
	
	if (pChannelWorld->m_RoomID == ROOM_ID_BUDDHISM)
		return dynamic_cast<CReligionWorld*>(pChannelWorld);

	return NULL;
}

static CReligionWorld* GetReligionWorldFromFID(T_FAMILYID _fid)
{
	CChannelWorld* pChannelWorld = GetChannelWorldFromFID(_fid);
	if (pChannelWorld == NULL)
		return NULL;

	if (pChannelWorld->m_RoomID == ROOM_ID_BUDDHISM)
		return dynamic_cast<CReligionWorld*>(pChannelWorld);

	return NULL;
}

static void	DBCB_DBGetVirtueList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			RRoomID	= *(uint32 *)argv[0];

	if (!nQueryResult)
		return;

	CReligionWorld *roomManager = GetReligionWorld(RRoomID);
	if(roomManager == NULL)
	{
		sipwarning("GetReligionWorld = NULL. RoomID=%D", RRoomID);
		return;
	}

	uint32	nRows;	resSet->serial(nRows);
	roomManager->m_nFamilyVirtueCount = (int)nRows;

	for(uint32 i = 0; i < nRows; i ++)
	{
		resSet->serial(roomManager->m_FamilyVirtueList[i].FID);
		resSet->serial(roomManager->m_FamilyVirtueList[i].FamilyName);
		resSet->serial(roomManager->m_FamilyVirtueList[i].Virtue);
	}
}

static void	DBCB_DBGetFishFlowerData(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			RRoomID	= *(uint32 *)argv[0];

	if (!nQueryResult)
		return;

	CReligionWorld *roomManager = GetReligionWorld(RRoomID);
	if(roomManager == NULL)
	{
		sipwarning("GetReligionWorld = NULL. RoomID=%D", RRoomID);
		return;
	}

	uint32	nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		uint8	FishLevel;	resSet->serial(FishLevel);
		resSet->serial(roomManager->m_FishExp);
		resSet->serial(roomManager->m_FlowerID);
		resSet->serial(roomManager->m_FlowerLevel);
		resSet->serial(roomManager->m_FlowerDelTime);
		resSet->serial(roomManager->m_FlowerFID);
		resSet->serial(roomManager->m_FlowerFamilyName);
	}
	else
	{
		sipwarning("nRows = 0. RRoomID=%d", RRoomID);
	}
}

bool CreateReligionRoom(T_ROOMID RRoomID)
{
	uint16	SceneID = GetReligionRoomSceneID(RRoomID);
	if (SceneID == 0)
	{
		sipwarning("Invalid ReligionRoomID. RRoomID=%d", RRoomID);
		return false;
	}

	CReligionWorld	*pRRoom = new CReligionWorld(RRoomID); // DeskCountForLetter_R[0]);
	if (pRRoom == NULL)
	{
		sipwarning("new CReligionWorld = NULL.");
		return false;
	}

	pRRoom->m_SceneID = SceneID;

	pRRoom->InitMasterDataValues();

	{
		for (uint32 i = 1; i <= pRRoom->m_nDefaultChannelCount; i++)
		{
			pRRoom->CreateNewChannel(i);
			//C판본을 위한 채널창조 by krs
			pRRoom->CreateNewChannel(i + ROOM_UNITY_CHANNEL_START);
		}
	}
	{
		TChannelWorld	rw(pRRoom);
		map_RoomWorlds.insert(make_pair(RRoomID, rw));
	}

	// 꽃/물고기정보를 얻는다.
	{
		MAKEQUERY_GetFishFlowerData(strQuery, RRoomID);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetFishFlowerData,
			"D",
			CB_,		RRoomID);
	}

	// 공덕비자료를 얻는다.
	{
		MAKEQUERY_GetVirtueList(strQuery, 0);	// 0이면 불교구역을 의미한다.
		DBCaller->executeWithParam(strQuery, DBCB_DBGetVirtueList,
			"D",
			CB_,		RRoomID);
	}
	
	pRRoom->ReloadYiSiResultForToday();
	pRRoom->ReloadEditedSBY();
	pRRoom->SetRoomEventStatus(ROOMEVENT_STATUS_NONE);
	pRRoom->SetRoomEvent();

	pRRoom->InitSharedActWorld();
/*	// FS에 통지한다.
	{
		uint8	flag = 1;	// Room Created
		CMessage	msgOut(M_NT_ROOM);
		msgOut.serial(RRoomID, flag);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut, false);
	}

	// ManagerService에 통지한다.
	{
		ucstring	RoomName = pRRoom->m_RoomName;
		uint32		MasterFID = 0, Result = 0;
		CMessage	msgOut(M_NT_ROOM_CREATED);
		msgOut.serial(RRoomID, Result, SceneID, RoomName, MasterFID);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	}

	TTime	curTime = GetDBTimeSecond();
	NextReligionCheckEveryWorkTimeSecond = GetUpdateNightTime(curTime);*/

	//sipinfo("Religion Room Created. RRoomID=%d", RRoomID); byy krs

	return true;
}

void RefreshReligionRoom()
{
	static TTime	LastUpdateActivityTime = 0;
	static TTime	LastUpdateAnimal = 0;
	static TTime	LastOneMinuteCycleWork = 0;

	bool	bAnimal = true, bActivity = true, bLargeAct = true, bOneMinuteCycleWork = true;

	TTime curTime = CTime::getLocalTime();
	if (curTime - LastUpdateActivityTime < 10000)
		bActivity = false;
	if (curTime - LastUpdateAnimal < 1000)
		bAnimal = false;
	if (curTime - LastOneMinuteCycleWork < 60000)
		bOneMinuteCycleWork = false;
	

	TChannelWorlds::iterator It;
	for (It = map_RoomWorlds.begin(); It != map_RoomWorlds.end(); It++)
	{
		CChannelWorld*		pChannel = It->second.getPtr();
		if (pChannel->m_RoomID != ROOM_ID_BUDDHISM)
			continue;
		CReligionWorld	*rw = dynamic_cast<CReligionWorld*>(pChannel);
//		if(bActivity)
//			rw->RefreshActivity();
		if (bAnimal)
		{
			rw->RefreshAnimal();
			// 폭죽관련
			rw->RefreshCracker();
		}
		rw->RefreshCharacterState();
		rw->UpdateBY();
		if (bOneMinuteCycleWork)
		{
			rw->CheckDoEverydayWork();
			rw->RefreshTouXiang();
			rw->RefreshFreezingPlayers();
		}

		if (rw->GetRoomEventStatus() == ROOMEVENT_STATUS_DOING)
			rw->RefreshRoomEvent();
		else
		{
			if (bOneMinuteCycleWork)
				rw->RefreshRoomEvent();
		}
	}

	curTime = CTime::getLocalTime();
	if(bActivity)
		LastUpdateActivityTime = curTime;
	if(bAnimal)
		LastUpdateAnimal = curTime;
	if (bOneMinuteCycleWork)
		LastOneMinuteCycleWork = curTime;
}

void CReligionWorld::CheckDoEverydayWork()
{
	TTime	curTime = GetDBTimeSecond();
	if (m_NextReligionCheckEveryWorkTimeSecond > curTime)
	{
		return;
	}
	ResetSharedYiSiResult();
	SendSharedYiSiResultToAll();
	SetRoomEvent();
	{
		MAKEQUERY_GetVirtueList(strQuery, 0);	// 0이면 불교구역을 의미한다.
		DBCaller->executeWithParam(strQuery, DBCB_DBGetVirtueList,
			"D",
			CB_,		m_RoomID);
	}

	m_NextReligionCheckEveryWorkTimeSecond = GetUpdateNightTime(curTime);
}

static uint32 nCandiSBYID = 1;
ReligionChannelData* CReligionWorld::CreateNewChannel(uint32 nNewID)
{
	if (nNewID == 0)
	{
		uint32 nCandiID = 1;
		while (GetChannel(nCandiID) != NULL)
		{
			nCandiID++;
		}
		nNewID = nCandiID;
	}

	ReligionChannelData	data(this, COUNT_R0_ACTPOS, m_RoomID, nNewID, m_SceneID);
	m_mapChannels.insert(make_pair(nNewID, data));

	TmapReligionChannelData::iterator it = m_mapChannels.find(nNewID);
	ReligionChannelData* _data = &(it->second);

	if ((nNewID != 1) && (nNewID < ROOM_SPECIAL_CHANNEL_START))
	{
		ReligionChannelData* pDefaultChannel = GetChannel(1);
		if (pDefaultChannel && pDefaultChannel->m_CurrentSBY)
		{
			_data->m_CurrentSBY = new CReligionSBY(nCandiSBYID++, &(pDefaultChannel->m_CurrentSBY->m_BaseData), _data);
			_data->m_CurrentSBY->m_CurrentStep = pDefaultChannel->m_CurrentSBY->m_CurrentStep;
		}
	}
	// sipdebug("Channel Created. RoomID=%d, ChannelID=%d", m_RoomID, nNewID);
	return _data;
}

ReligionChannelData* CReligionWorld::GetChannel(uint32 ChannelID)
{
	TmapReligionChannelData::iterator	itChannel = m_mapChannels.find(ChannelID);
	if(itChannel == m_mapChannels.end())
		return NULL;
	return &itChannel->second;
}

ReligionChannelData* CReligionWorld::GetFamilyChannel(T_FAMILYID FID)
{
	uint32	ChannelID = GetFamilyChannelID(FID);
	if(ChannelID == 0)
		return NULL;
	TmapReligionChannelData::iterator	itChannel = m_mapChannels.find(ChannelID);
	if(itChannel == m_mapChannels.end())
		return NULL;
	return &itChannel->second;
}

// 방선로목록을 요청한다. - 종교구역에서는 선로의 자동추가/삭제가 없다.
//void CReligionWorld::RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
//{
///*
//	CMessage	msgOut(M_SC_CHANNELLIST);
//	msgOut.serial(_FID, m_RoomID);
//
//	// Channel목록 만들기
//	uint32	numLimit = (uint32)m_nMaxCharacterOfRoomKind;
//
//	TmapReligionChannelData::iterator	itChannel;
//	TmapDwordToDword	user_count;
//	uint32				usercount;
//	for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); itChannel ++)
//	{
//		usercount = 0;
//		for(TFesFamilyIds::iterator itFes = itChannel->second.m_mapFesChannelFamilys.begin(); 
//			itFes != itChannel->second.m_mapFesChannelFamilys.end(); itFes ++)
//		{
//			usercount += itFes->second.size();
//		}
//		user_count[itChannel->first] = usercount;
//	}
//
//	for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); )
//	{
//		uint8	channel_status;
//		uint32	channelid = itChannel->first;
//		usercount = user_count[channelid];
//
//		if(usercount >= numLimit)
//			channel_status = ROOMCHANNEL_FULL;
//		else if(usercount * 100 >= numLimit * g_nRoomSmoothFamPercent)
//			channel_status = ROOMCHANNEL_COMPLEX;
//		else
//			channel_status = ROOMCHANNEL_SMOOTH;
//
//		msgOut.serial(channelid, channel_status);
//
//		itChannel ++;
//	}
//
//	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
//	*/
//
//	CMessage	msgOut(M_SC_CHANNELLIST);
//	msgOut.serial(_FID, m_RoomID);
//
//	// Channel목록 만들기
//	uint32	numLimit = (uint32)m_nMaxCharacterOfRoomKind;
//
//	uint32	new_channelid = 0;
//	uint8	channel_status;
//	bool	bSmoothChannelExist = false;
//	int		empty_channel_count = 0;
//
//	// 선로중 비지 않은 원활한 선로가 있는가를 검사한다. 있으면 빈 선로를 삭제할수 있다. 1번선로는 삭제하지 않는다.
//	TmapReligionChannelData::iterator	itChannel;
//	TmapDwordToDword	user_count;
//	uint32				usercount;
//	for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); itChannel ++)
//	{
//		uint32 channelid = itChannel->first;
//		ReligionChannelData* pChannel = &(itChannel->second);
//		usercount = pChannel->GetChannelUserCount();
//		if (pChannel->m_CurrentSBY)
//		{
//			usercount += pChannel->m_CurrentSBY->m_JoinBookingUser.size();
//		}
//		
//		user_count[itChannel->first] = usercount;
//		if(usercount == 0)
//			empty_channel_count ++;
//		else if(usercount * 100 < numLimit * g_nRoomSmoothFamPercent)
//			bSmoothChannelExist = true;
//	}
//
//	// 대기렬에 사람이 없는 경우, 원활한선로가 존재하는 경우에 빈선로를 삭제할수 있다.
//	bool	bDeleteAvailable = false;
//	if((map_EnterWait.size() <= 1) && (bSmoothChannelExist || (empty_channel_count > 1)))	// (본인은 물론 현재의 대기렬에 있으므로) 대기렬에 2명이상이면 선로삭제를 하지 않는다.
//		bDeleteAvailable = true;
//
//	int		cur_empty_channel = 0;
//	for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); )
//	{
//		uint32	channelid = itChannel->first;
//		uint32	usercount = user_count[channelid];
//
//		if(bDeleteAvailable && (usercount == 0)) // 빈선로는 삭제가능성 검사한다.
//		{
//			cur_empty_channel ++;
//			if(channelid > m_nDefaultChannelCount) // default선로는 삭제하지 않는다.
//			{
//				if(bSmoothChannelExist || (cur_empty_channel > 1))
//				{
//					// Channel이 삭제됨을 현재 Channel의 사용자들에게 통지한다.
//					NoticeChannelChanged(channelid, 0);
//
//					sipdebug("Channel Deleted. RoomID=%d, ChannelID=%d", m_RoomID, channelid);
//
//					// 빈 선로를 삭제한다. 1번선로는 삭제하지 않는다.
//					itChannel = m_mapChannels.erase(itChannel);
//					continue;
//				}
//			}
//		}
//
//		if(channelid > new_channelid)
//			new_channelid = channelid;
//
//		if(usercount >= numLimit)
//			channel_status = ROOMCHANNEL_FULL;
//		else if(usercount * 100 >= numLimit * g_nRoomSmoothFamPercent)
//			channel_status = ROOMCHANNEL_COMPLEX;
//		else
//		{
//			bSmoothChannelExist = true;
//			channel_status = ROOMCHANNEL_SMOOTH;
//		}
//
//		msgOut.serial(channelid, channel_status);
//
//		itChannel ++;
//	}
//
//	if(!bSmoothChannelExist && (empty_channel_count == 0))
//	{
//		// 원활한 선로가 없다면 하나 창조한다.
//		new_channelid ++;
//
//		CreateNewChannel(new_channelid);
//
//		channel_status = ROOMCHANNEL_SMOOTH;
//
//		msgOut.serial(new_channelid, channel_status);
//
//		// Channel이 추가됨을 현재 Channel의 사용자들에게 통지한다.
//		NoticeChannelChanged(new_channelid, 1);
//	}
//
//	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
//}

// 방특수선로를 요청한다
void	CReligionWorld::RequestSpecialChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	ProcRemovedSpecialChannel();
	
	uint32	channelid = m_nCandiSpecialCannel;
	uint8 channel_status = ROOMCHANNEL_SMOOTH;
	CMessage	msgOut(M_SC_CHANNELLIST);
	msgOut.serial(_FID, m_RoomID, channelid, channel_status);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);

	m_nCandiSpecialCannel++;
	ReligionChannelData	data(this, COUNT_R0_ACTPOS, m_RoomID, channelid, m_SceneID);
	m_mapChannels.insert(make_pair(channelid, data));
	//sipdebug("Special Channel Created. RoomID=%d, ChannelID=%d, FID=%d", m_RoomID, channelid, _FID); byy krs
}

// 방선로목록을 요청한다.
void	CReligionWorld::RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	ProcRemovedSpecialChannel();

	CChannelWorld::RequestChannelList(_FID, _sid);
}

struct ReligionWishInfo
{
	ReligionWishInfo(uint32 wishID, T_FAMILYID fid, T_FAMILYNAME fname, T_M1970 beginTime) :
		WishID(wishID), FID(fid), FName(fname), BeginTime(beginTime) { }
	uint32			WishID;
	T_FAMILYID		FID;
	T_FAMILYNAME	FName;
	T_M1970			BeginTime;
};
struct ReligionWishPageInfo
{
	ReligionWishPageInfo() : bValid(false) {}
	bool						bValid;
	std::list<ReligionWishInfo>	wishList;
};
struct ReligionWishListHistory
{
	uint32	AllCount;
	std::map<uint32, ReligionWishPageInfo>	PageInfo;

	void Reset()
	{
		AllCount = 0;
		PageInfo.clear();
	}
};
static ReligionWishListHistory	s_ReligionWishListHistory;

// Add wish ([w:InvenPos][u:TargetName][u:Pray][d:SyncCode])
void cb_M_CS_ADDWISH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint16			InvenPos;
	ucstring		targetName = "";
	ucstring		pray = "";
	uint32			SyncCode;

	_msgin.serial(FID, InvenPos);
	NULLSAFE_SERIAL_WSTR(_msgin, targetName, MAX_LEN_WISH_TARGET, FID);
	NULLSAFE_SERIAL_WSTR(_msgin, pray, MAX_LEN_WISH, FID);
	_msgin.serial(SyncCode);

	CReligionWorld *roomManager = GetReligionWorldFromFID(FID);
	if (roomManager == NULL)
	{
		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	roomManager->on_M_CS_ADDWISH(FID, InvenPos, targetName, pray, SyncCode, _tsid);
}

void CReligionWorld::on_M_CS_ADDWISH(uint32 FID, uint16 InvenPos, ucstring targetName, ucstring pray, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	ReligionChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	if(!IsValidInvenPos(InvenPos))
	{
		// 비법파케트발견
		ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos! UserFID=%d, InvenPos=%d", FID, InvenPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	_InvenItems*	pInven = GetFamilyInven(FID);
	if(pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL");
		return;
	}
	int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
	ITEM_INFO	&item = pInven->Items[InvenIndex];
	uint32	ItemID = item.ItemID;

	const	MST_ITEM	*mstItem = GetMstItemInfo(ItemID);
	if((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_XUYUNQIAN))
	{
		sipwarning(L"Invalid Item. FID=%d, ItemID=%d", FID, ItemID);
		return;
	}

	// Check OK

	// Delete from Inven
	if(1 < item.ItemCount)
	{
		item.ItemCount --;
	}
	else
	{
		item.ItemID = 0;
	}

	if(!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
		return;
	}

	// 경험값 변경
	ChangeFamilyExp(Family_Exp_Type_Wish, FID);

	// 공덕값 변경
	if(item.MoneyType == MONEYTYPE_UMONEY)
		ChangeFamilyVirtue(Family_Virtue0_Type_UseMoney, FID, mstItem->UMoney);
	else
	{
		if(mstItem->GMoney > 0)
			ChangeFamilyVirtue(Family_Virtue0_Type_UsePoint, FID, mstItem->GMoney);
	}
	ChangeFamilyVirtue(Family_Virtue0_Type_Wish, FID);

	// Log
	DBLOG_ItemUsed(DBLSUB_USEITEM_R0_XUYUAN, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);

	FAMILY	*pFamily = GetFamilyData(FID);
	if (pFamily && !IsSystemAccount(pFamily->m_UserID))
	{
		MAKEQUERY_AddWish(strQuery, m_RoomID, FID, ItemID, targetName, pray, mstItem->UseTime);
		DBCaller->execute(strQuery);
		s_ReligionWishListHistory.Reset();
	}

	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
	}

	{
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_WISH)
			msgOut.serial(FID);
		FOR_END_FOR_FES_ALL()
	}
}

static void	DBCB_DBGetWishList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			AllCount	= *(uint32 *)argv[0];
	uint32			ret			= *(uint32 *)argv[1];
	uint32			FID			= *(uint32 *)argv[2];
	uint32			Page		= *(uint32 *)argv[3];
	TServiceId		tsid(*(uint16 *)argv[4]);

	if (!nQueryResult)
		return;

	if(ret != 0 && ret != 1010)
	{
		sipwarning("Invalid return code. ret=%d", ret);
		return;
	}

	CMessage	msgOut(M_SC_WISH_LIST);
	msgOut.serial(FID, AllCount, Page);
	if(ret == 0)
	{
		ReligionWishPageInfo newPageInfo;
		newPageInfo.bValid = true;

		uint32	nRows;	resSet->serial(nRows);
		for(uint32 i = 0; i < nRows; i ++)
		{
			uint32		fid;			resSet->serial(fid);
			ucstring	FName;			resSet->serial(FName);
			T_DATETIME	BeginTime;		resSet->serial(BeginTime);
			uint32		WishID;			resSet->serial(WishID);
			uint32	nBeginTime = getMinutesSince1970(BeginTime);

			msgOut.serial(WishID, fid, FName, nBeginTime);
			AddFamilyNameData(fid, FName);
			newPageInfo.wishList.push_back(ReligionWishInfo(WishID, fid, FName, nBeginTime));
		}

		uint32 mapIndex = Page+1;	// For page is started from 0
		std::map<uint32, ReligionWishPageInfo>::iterator itPage = s_ReligionWishListHistory.PageInfo.find(mapIndex);
		if (itPage != s_ReligionWishListHistory.PageInfo.end())
		{
			s_ReligionWishListHistory.AllCount = AllCount;
			s_ReligionWishListHistory.PageInfo[mapIndex] = newPageInfo;
		}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

// request wish list ([d:Page][d:PageSize])
void cb_M_CS_WISH_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			Page, PageSize;

	_msgin.serial(FID, Page, PageSize);

	T_ROOMID RoomID = GetFamilyWorld(FID);
	if(!ISVALIDROOM(RoomID))
	{
		sipwarning("GetFamilyWorld = NULL. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	/* PageSize를 넣으면서 이 부분의 의미가 없어짐. 더 검토 필요함
	uint32 mapIndex = Page+1;	// For page is started from 0
	std::map<uint32, ReligionWishPageInfo>::iterator itPage = s_ReligionWishListHistory.PageInfo.find(mapIndex);
	if (itPage != s_ReligionWishListHistory.PageInfo.end())
	{
		ReligionWishPageInfo& pageInfo = itPage->second;
		if (pageInfo.bValid)
		{
			CMessage	msgOut(M_SC_WISH_LIST);
			msgOut.serial(FID, s_ReligionWishListHistory.AllCount, Page);
			std::list<ReligionWishInfo>::iterator lstit;
			for (lstit = pageInfo.wishList.begin(); lstit != pageInfo.wishList.end(); lstit++)
			{
				ReligionWishInfo& WishInfo = *lstit;
				uint32			WishID = WishInfo.WishID;
				T_FAMILYID		actFID = WishInfo.FID;
				T_FAMILYNAME	actFName = GetFamilyNameFromBuffer(actFID);
				uint32			BeginTime = WishInfo.BeginTime;

				msgOut.serial(WishID, actFID, actFName, BeginTime);
			}
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
			return;
		}
	}
	else
	{
		ReligionWishPageInfo newPageInfo;
		s_ReligionWishListHistory.PageInfo[mapIndex] = newPageInfo;
	}
	*/

	MAKEQUERY_GetWishList(strQuery, RoomID, Page, PageSize);
	DBCaller->executeWithParam(strQuery, DBCB_DBGetWishList,
		"DDDDW",
		OUT_,		NULL,
		OUT_,		NULL,
		CB_,		FID,
		CB_,		Page,
		CB_,		_tsid.get());
}

static void	DBCB_DBGetMyWishList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			AllCount	= *(uint32 *)argv[0];
	uint32			ret			= *(uint32 *)argv[1];
	uint32			FID			= *(uint32 *)argv[2];
	uint32			Page		= *(uint32 *)argv[3];
	TServiceId		tsid(*(uint16 *)argv[4]);

	if (!nQueryResult)
		return;

	if(ret != 0 && ret != 1010)
	{
		sipwarning("Invalid return code. ret=%d", ret);
		return;
	}

	CMessage	msgOut(M_SC_MY_WISH_LIST);
	msgOut.serial(FID, AllCount, Page);
	if(ret == 0)
	{
		uint32	nRows;	resSet->serial(nRows);
		for(uint32 i = 0; i < nRows; i ++)
		{
			uint32		WishID;			resSet->serial(WishID);
			uint32		ItemID;			resSet->serial(ItemID);
			ucstring	TargetName;		resSet->serial(TargetName);
			T_DATETIME	BeginTime;		resSet->serial(BeginTime);
			T_DATETIME	EndTime;		resSet->serial(EndTime);

			uint32	nBeginTime = getMinutesSince1970(BeginTime);
			uint32	nEndTime = getMinutesSince1970(EndTime);

			msgOut.serial(WishID, ItemID, TargetName, nBeginTime, nEndTime);
		}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

// request my wish list ([d:Page][d:PageSize])
void cb_M_CS_MY_WISH_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			page, PageSize;

	_msgin.serial(FID, page, PageSize);

	T_ROOMID RoomID = GetFamilyWorld(FID);
	if(!ISVALIDROOM(RoomID))
	{
		sipwarning("GetFamilyWorld = NULL. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	MAKEQUERY_GetMyWishList(strQuery, RoomID, FID, page, PageSize);
	DBCaller->executeWithParam(strQuery, DBCB_DBGetMyWishList,
		"DDDDW",
		OUT_,		NULL,
		OUT_,		NULL,
		CB_,		FID,
		CB_,		page,
		CB_,		_tsid.get());
}

static void	DBCB_DBGetWishPray(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			FID			= *(uint32 *)argv[0];
	uint32			WishID		= *(uint32 *)argv[1];
	TServiceId		tsid(*(uint16 *)argv[2]);

	if (!nQueryResult)
		return;

	uint32	nRows;	resSet->serial(nRows);
	if(nRows != 1)
	{
		sipwarning("Invalid RecordCount. nRows=%d", nRows);
		return;
	}

	uint32		_RoomID;	resSet->serial(_RoomID);
	uint32		_FID;		resSet->serial(_FID);
	ucstring	Pray;		resSet->serial(Pray);

	if(FID != _FID)
	{
		sipwarning("FID != _FID !!!, FID=%d, _FID=%d", FID, _FID);
		return;
	}

	T_ROOMID RoomID = GetFamilyWorld(FID);
	if(!ISVALIDROOM(RoomID))
	{
		sipwarning("GetFamilyWorld = NULL. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}
	if(RoomID != _RoomID)
	{
		sipwarning("RoomID != _RoomID !!! RoomID=%d, _RoomID=%d", RoomID, _RoomID);
		return;
	}

	CMessage	msgOut(M_SC_MY_WISH_DATA);
	msgOut.serial(FID, WishID, Pray);

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

// request my wish data ([d:WishID])
void cb_M_CS_MY_WISH_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			WishID;

	_msgin.serial(FID, WishID);

	MAKEQUERY_GetWishPray(strQuery, WishID);
	DBCaller->executeWithParam(strQuery, DBCB_DBGetWishPray,
		"DDW",
		CB_,		FID,
		CB_,		WishID,
		CB_,		_tsid.get());
}
/*
// Activity Time처리
//void CReligionWorld::RefreshActivity()
//{
//	TTime curTime = CTime::getLocalTime();
//	TServiceId	sidTemp(0);
//	TmapReligionChannelData::iterator	itChannel;
//	for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); itChannel ++)
//	{
//		ReligionChannelData	*pChannelData = &itChannel->second;
//		for(int ActPos = 0; ActPos < m_ActivityCount; ActPos ++)
//		{
//			_ReligionActivity	&rActivity = pChannelData->m_Activitys[ActPos];
//			for(int DeskNo = 0; DeskNo < rActivity.ActivityDeskCount; DeskNo ++)
//			{
//				_ReligionActivityDesk	&rActivity_Desk = rActivity.ActivityDesk[DeskNo];
//
//				if(rActivity_Desk.FID == 0)
//					continue;
//				if(rActivity_Desk.StartTime == 0)	// 행사진행중이면
//					continue;
//				// 행사차례가 되였는데 계속 하지 않고있는 사용자 절단
//				if(curTime - rActivity_Desk.StartTime < MAX_ACTCAPTURE_TIME)
//					continue;
//				ActWaitCancel(rActivity_Desk.FID, itChannel->first, ActPos, false, true);
//			}
//		}
//	}
//}
*/
// Request Paper ([d:DeskNo])
//void cb_M_CS_R_PAPER_START(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;
//	uint32			DeskNo;
//	_msgin.serial(FID, DeskNo);
//
//	// 방관리자를 얻는다
//	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
//	if (inManager == NULL)
//	{
//		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
//		return;
//	}
//	if(DeskNo >= inManager->m_DeskCountForLetter)
//	{
//		sipwarning("DeskNo >= inManager->m_DeskCountForLetter. DeskNo=%d, FID=%d", DeskNo, FID);
//		return;
//	}
//
//	ReligionChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
//	if(pChannelData == NULL)
//	{
//		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
//		return;
//	}
//
//	// 이미 앉아있는 경우 처리
//	for(uint32 i = 0; i < inManager->m_DeskCountForLetter; i ++)
//	{
//		if(pChannelData->m_DeskForLetters[i] == FID)
//		{
//			sipwarning("Already writting paper in %d. FID=%d", i, FID);
//			return;
//		}
//	}
//
//	uint32	err = 0;
//	if(pChannelData->m_DeskForLetters[DeskNo] != 0)
//	{
//		err = E_PAPER_R_ALREADY_SIT;
//		sipwarning("already exist in desk %d.", DeskNo);
//
//		CMessage	msgOut(M_SC_R_PAPER_START);
//		uint32	zero = 0;
//		msgOut.serial(FID, zero);
//		msgOut.serial(err, FID, DeskNo);
//		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
//	}
//	else
//	{
//		pChannelData->m_DeskForLetters[DeskNo] = FID;
//
//		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_R_PAPER_START)
//			msgOut.serial(err, FID, DeskNo);
//		FOR_END_FOR_FES_ALL()
//	}
//}
//
//// Notice that writting Paper finished and start autoplay ([d:DeskNo])
//void cb_M_CS_R_PAPER_PLAY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;
//	uint32			DeskNo;
//	_msgin.serial(FID, DeskNo);
//
//	// 방관리자를 얻는다
//	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
//	if (inManager == NULL)
//	{
//		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
//		return;
//	}
//	if(DeskNo >= inManager->m_DeskCountForLetter)
//	{
//		sipwarning("DeskNo >= inManager->m_DeskCountForLetter. DeskNo=%d, FID=%d", DeskNo, FID);
//		return;
//	}
//
//	ReligionChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
//	if(pChannelData == NULL)
//	{
//		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
//		return;
//	}
//
//	if(pChannelData->m_DeskForLetters[DeskNo] != FID)
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("cb_M_CS_R_PAPER_PLAY::pChannelData->m_DeskForLetters[DeskNo] != FID. DeskNo=%d, FID=%d, PrevFID=%d", DeskNo, FID, pChannelData->m_DeskForLetters[DeskNo]);
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_R_PAPER_PLAY)
//		msgOut.serial(FID, DeskNo);
//	FOR_END_FOR_FES_ALL()
//}
//
//// Paper Write End ([d:DeskNo])
//void cb_M_CS_R_PAPER_END(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;
//	uint32			DeskNo;
//	uint8			bIsOK;
//	_msgin.serial(FID, DeskNo, bIsOK);
//
//	// 방관리자를 얻는다
//	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
//	if (inManager == NULL)
//	{
//		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
//		return;
//	}
//	if(DeskNo >= inManager->m_DeskCountForLetter)
//	{
//		sipwarning("DeskNo >= inManager->m_DeskCountForLetter. DeskNo=%d, FID=%d", DeskNo, FID);
//		return;
//	}
//
//	ReligionChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
//	if(pChannelData == NULL)
//	{
//		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
//		return;
//	}
//
//	if(pChannelData->m_DeskForLetters[DeskNo] != FID)
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("cb_M_CS_R_PAPER_END::pChannelData->m_DeskForLetters[DeskNo] != FID. DeskNo=%d, FID=%d, PrevFID=%d", DeskNo, FID, pChannelData->m_DeskForLetters[DeskNo]);
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//	pChannelData->m_DeskForLetters[DeskNo] = 0;
//
//	// 공덕값 변경
//	inManager->ChangeFamilyVirtue(Family_Virtue0_Type_Letter, FID);
//
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_R_PAPER_END)
//		msgOut.serial(FID, DeskNo, bIsOK);
//	FOR_END_FOR_FES_ALL()
//}

void CReligionWorld::EnterFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	SendReligionChannelData(FID, sid);
	LoadFamilyVirtueHistory(FID);
	SendRoomEventData(FID, sid);
}

//void CReligionWorld::ChangeChannelFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid)
//{
//	EnterFamily_Spec(FID, sid);
//}

void CReligionWorld::SendReligionChannelData(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	ReligionChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	// 편지올리기상태를 내려보낸다.
	/*
	{
		CMessage	msgOut(M_SC_R_PAPER_LIST);
		msgOut.serial(FID);
		for(uint32 i = 0; i < m_DeskCountForLetter; i ++)
		{
			if(pChannelData->m_DeskForLetters[i] != 0)
			{
				msgOut.serial(i, pChannelData->m_DeskForLetters[i]);
			}
		}
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}
	*/

	// 념불읽기정보를 내려보낸다.
	pChannelData->SendNianFoState(FID, sid);
	pChannelData->SendNeiNianFoState(FID, sid);

	// 행사효과모델목록을 내려보낸다.
	pChannelData->SendActivityList(FID, sid);
	// 탁우에 놓일 행사결과정보를 내려보낸다.
	SendYiSiResult(FID, sid);
	// 설정된 대형행사정보를 내려보낸다
	pChannelData->SendSettingBYInfo(FID, sid);
	// 진행중인 대형행사상태정보를 내려보낸다
	pChannelData->SendDoingBYState(FID, sid);

	// 체계대형행사예약자이면 추가통보를 내려보낸다(이함수의 제일 마지막에 처리하여야 한다)
	pChannelData->ProcSBYBooking(FID);
}

void CReligionWorld::ProcRemoveFamily(T_FAMILYID FID)
{ 
	RemoveSBYBooking(FID);

	for (ReligionChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		pChannelData->AllActCancel(FID, false, true);
	}
}

void CReligionWorld::OutRoom_Spec(T_FAMILYID _FID)
{
	uint32	ChannelID = GetFamilyChannelID(_FID);
	ReligionChannelData*	pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("GetChannel() = NULL. FID=%d, ChannelID=%d", _FID, ChannelID);
		return;
	}

	// 사용자가 창조하였던 혹은 대기하였던 활동을 삭제한다.
	pChannelData->AllActCancel(_FID, false, false);

	//// 편지올리기를 하고있는 경우 처리
	//for(uint32 i = 0; i < m_DeskCountForLetter; i ++)
	//{
	//	if(pChannelData->m_DeskForLetters[i] == _FID)
	//	{
	//		pChannelData->m_DeskForLetters[i] = 0;

	//		uint8		bIsOK = 0;
	//		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_R_PAPER_END)
	//			msgOut.serial(_FID, i, bIsOK);
	//		FOR_END_FOR_FES_ALL()

	//		break;
	//	}
	//}
	// 체계대형행사에서 탈퇴
	if (pChannelData->m_CurrentPBY)
	{
		pChannelData->m_CurrentPBY->ProcCancel(_FID);
		pChannelData->m_CurrentPBY->ExitJoin(_FID);
	}
	else if (pChannelData->m_CurrentSBY)
	{
		pChannelData->m_CurrentSBY->ExitJoin(_FID);
		pChannelData->m_CurrentSBY->RemoveBooking(_FID);
	}

	// 념불에서 탈퇴
	pChannelData->ExitNianFo(_FID);
	pChannelData->ExitNeiNianFo(_FID);
}

// 사용자의 공덕값을 추가한다.
void CReligionWorld::ChangeFamilyVirtue(uint8 FamilyVirtueType, T_FAMILYID FID, uint32 MoneyOrPoint)
{
	TRoomFamilys::iterator	it = m_mapFamilys.find(FID);
	if(it == m_mapFamilys.end())
	{
		sipwarning("FamilyID can't find in m_mapFamilys !!! FID=%d", FID);
		return;
	}
	INFO_FAMILYINROOM	&FamilyInfo = it->second;

	uint32	virtue = 0;

	switch(m_RoomID)
	{
	case ROOM_ID_BUDDHISM:
		switch(FamilyVirtueType)
		{
		case Family_Virtue0_Type_UseMoney:
			virtue = MoneyOrPoint;
			break;
		case Family_Virtue0_Type_UsePoint:
			virtue = MoneyOrPoint >> 2;
			break;
		case Family_Virtue0_Type_Visit:
		case Family_Virtue0_Type_Jisi:
		case Family_Virtue0_Type_Wish:
		case Family_Virtue0_Type_Letter:
			{
				uint32	today = GetDBTimeDays();

				if(today != FamilyInfo.m_FamilyVirtueHis[FamilyVirtueType].Day)
				{
					FamilyInfo.m_FamilyVirtueHis[FamilyVirtueType].Day = today;
					FamilyInfo.m_FamilyVirtueHis[FamilyVirtueType].Count = 0;
				}
				if(mstFamilyVirtue0[FamilyVirtueType].MaxCount > FamilyInfo.m_FamilyVirtueHis[FamilyVirtueType].Count)
				{
					FamilyInfo.m_FamilyVirtueHis[FamilyVirtueType].Count ++;
					virtue = mstFamilyVirtue0[FamilyVirtueType].Virtue;
				}
			}
			break;
		default:
			sipwarning("Unknown FamilyVirtueType. RoomID=%d, TypeID=%d", m_RoomID, FamilyVirtueType); 
			return;
		}
		break;
	default:
		siperror("Unknown RoomType. RoomID=%d", m_RoomID);
		return;
	}

	if(virtue == 0)
		return;

	if(!SaveFamilyVirtue(FamilyInfo, virtue))
	{
		sipwarning("SaveFamilyVirtue failed. FID=%d", FID);
	}
}

static void	DBCB_DBReadFamilyVirtueHisData(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32	FID	= *(uint32 *)argv[0];
	uint32	RoomID	= *(uint32 *)argv[1];

	if (!nQueryResult)
		return;

	//TRoomFamilys::iterator	it = m_mapFamilys.find(FID);
	//if (it == m_mapFamilys.end())
	//{
	//	sipwarning("FamilyID can't find in m_mapFamilys !!! FID=%d", FID);
	//	return false;
	//}
	INFO_FAMILYINROOM	*pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	uint32	nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		uint32	bufsize;		resSet->serial(bufsize);
		resSet->serialBuffer((uint8*)(&pFamilyInfo->m_FamilyVirtueHis), bufsize);
		resSet->serial(pFamilyInfo->m_nVirtue);
	}

	// 종교구역에 들어갈때 공덕값 변화
	CReligionWorld	*pRRoomWorld = GetReligionWorld(RoomID);
	if (pRRoomWorld == NULL)
	{
		sipwarning("GetReligionWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	switch (RoomID)
	{
	case ROOM_ID_BUDDHISM:
		pRRoomWorld->ChangeFamilyVirtue(Family_Virtue0_Type_Visit, FID);
		break;
	}
}

// 사용자의 공덕값리력정보를 DB에서 읽는다.
bool CReligionWorld::LoadFamilyVirtueHistory(uint32 FID)
{
	TRoomFamilys::iterator	it = m_mapFamilys.find(FID);
	if (it == m_mapFamilys.end())
	{
		sipwarning("FamilyID can't find in m_mapFamilys !!! FID=%d", FID);
		return false;
	}
	INFO_FAMILYINROOM	&FamilyInfo = it->second;

	memset(&FamilyInfo.m_FamilyVirtueHis, 0, sizeof(FamilyInfo.m_FamilyVirtueHis));
	FamilyInfo.m_nVirtue = 0;

	MAKEQUERY_ReadFamilyVirtueHisData(strQ, FID, 0);	// 0이면 불교구역을 의미한다.
	DBCaller->executeWithParam(strQ, DBCB_DBReadFamilyVirtueHisData,
		"DD",
		CB_,		FID,
		CB_,		m_RoomID);

	return true;
}

// 사용자의 공덕값의 변화를 DB에 보관한다.
bool CReligionWorld::SaveFamilyVirtue(INFO_FAMILYINROOM &FamilyInfo, uint32 virtue)
{
	FamilyInfo.m_nVirtue += virtue;

	uint32 FID = FamilyInfo.m_FID;

	int	num1;
	switch (m_RoomID)
	{
	case ROOM_ID_BUDDHISM:
		num1 = FAMILY_VIRTUE0_TYPE_MAX * sizeof(_Family_Exp_His);	// 5 = sizeof(_Family_Exp_His)
		break;
	default:
		sipwarning("Unknown RoomID. RoomID=%d", m_RoomID);
		return false;
	}

	uint8 *buf = (uint8*) (&FamilyInfo.m_FamilyVirtueHis);
	tchar str_buf[FAMILY_VIRTUE_TYPE_MAX * sizeof(_Family_Exp_His) * 2 + 10];
	for (int i = 0; i < num1; i ++)
	{
		smprintf((tchar*)(&str_buf[i * 2]), 2, _t("%02x"), buf[i]);
	}
	str_buf[num1 * 2] = 0;

	MAKEQUERY_SaveFamilyVirtueData(strQ, FID, 0, virtue, str_buf);	// 0이면 불교구역을 의미한다.
	DBCaller->execute(strQ);

	return true;

	//MAKEQUERY_SaveFamilyVirtueData(strQ, FID, m_RRoomType, virtue);

	//CDBConnectionRest	DB(DBCaller);
	//DB_EXE_PREPARESTMT(DB, Stmt, strQ);
	//if (!nPrepareRet)
	//{
	//	sipwarning("DB_EXE_PREPARESTMT failed in Shrd_Family_UpdateVirtueData!");
	//	return false;
	//}

	//DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_INVENBUF_SIZE, 0, &FamilyInfo.m_FamilyVirtueHis, 0, &num1);
	//if (!nBindResult)
	//{
	//	sipwarning("bind parameter failed in Shrd_Family_UpdateVirtueData!!");
	//	return false;
	//}

	//DB_EXE_PARAMQUERY(Stmt, strQ);
	//if (!nQueryResult)
	//{
	//	sipwarning("execute failed in Shrd_Family_UpdateVirtueData!");
	//	return false;
	//}

	//return true;
}

// Request virtue list
void cb_M_CS_VIRTUE_LIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	_msgin.serial(FID);

	// 방관리자를 얻는다
	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
		return;
	}

//	if (inManager->m_RRoomType != RROOM_BUDDHISM)
//	{
//		sipwarning("inManager->m_RRoomType != RROOM_BUDDHISM. inManager->m_RRoomType=%d", inManager->m_RRoomType);
//		return;
//	}

	CMessage	msgOut(M_SC_VIRTUE_LIST);
	msgOut.serial(FID, inManager->m_nFamilyVirtueCount);

	for(uint32 i = 0; i < inManager->m_nFamilyVirtueCount; i ++)
	{
		msgOut.serial(inManager->m_FamilyVirtueList[i].FID, inManager->m_FamilyVirtueList[i].FamilyName, inManager->m_FamilyVirtueList[i].Virtue);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

// Request my current virtue value
void cb_M_CS_VIRTUE_MY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;

	_msgin.serial(FID);

	INFO_FAMILYINROOM	*pFamily = GetFamilyInfo(FID);
	if(pFamily == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	CMessage	msgOut(M_SC_VIRTUE_MY);
	msgOut.serial(FID, pFamily->m_nVirtue);

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}
//////////////////////////////////////////////////////////////////////////

uint32	CReligionWorld::SaveYisiAct(T_FAMILYID FID, TServiceId _tsid, uint8 NpcID, uint8 ActionType, uint8 Secret, ucstring Pray, uint8 ItemNum, uint32 SyncCode, uint16* invenpos, uint32 ActID)
{
	uint8	itembuf[MAX_ITEM_PER_ACTIVITY * sizeof(uint32) + 1];
	itembuf[0] = 0;
	uint32	*itemlist = (uint32*)(&itembuf[1]);
	_InvenItems*	pInven = GetFamilyInven(FID);
	if(pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL:FID=%d", FID);
		return ActID;
	}
	bool	bYiSiMaster = (ActID == 0);
	uint32	UsedUMoney = 0, UsedGMoney = 0;
	bool bChangeInven = false;
	bool bTaoCanItem = false;
	for (int i = 0; i < ItemNum; i ++)
	{
		uint16 InvenPos = invenpos[i];
		if (InvenPos == 0)
			continue;

		uint32	ItemID = 0;
		// InvenPos 검사
		if(!IsValidInvenPos(InvenPos))
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos! FID=%d, InvenPos=%d", FID, InvenPos);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return ActID;
		}
		int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
		ITEM_INFO	&item = pInven->Items[InvenIndex];
		ItemID = item.ItemID;
		// ItemID의 정확성은 검사하지 않는다.
		const	MST_ITEM	*mstItem = GetMstItemInfo(ItemID);
		if(mstItem == NULL)
		{
			sipwarning(L"Invalid Item. FID=%d, ItemID=%d", FID, ItemID);
			return ActID;
		}
		if (GetTaocanItemInfo(ItemID) != NULL)
			bTaoCanItem = true;

		// Delete from Inven
		if(1 < item.ItemCount)
		{
			item.ItemCount --;
		}
		else
		{
			item.ItemID = 0;
		}
		// 공덕값 변경
		if(item.MoneyType == MONEYTYPE_UMONEY)
			UsedUMoney += mstItem->UMoney;
		else
			UsedGMoney += mstItem->GMoney;

		// Log
		switch(ActionType)
		{
		case AT_PRAYRITE:
		case AT_PRAYMULTIRITE:
			DBLOG_ItemUsed(DBLSUB_USEITEM_R0_TAOCAN, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			break;
		case AT_PRAYXIANG:
			DBLOG_ItemUsed(DBLSUB_USEITEM_R0_XIANG, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			break;
		}
		*itemlist = ItemID;
		itembuf[0] ++;
		itemlist ++;

		bChangeInven = true;
	}
	if (bChangeInven)
	{
		if(!SaveFamilyInven(FID, pInven))
		{
			sipwarning("SaveFamilyInven failed. FID=%d", FID);
			return ActID;
		}
	}
	uint32 ActPos = NpcID;
	//if (ActionType == AT_PRAYRITE && ActPos >= ACTPOS_R0_YISHI_1 && ActPos <= ACTPOS_R0_YISHI_6 )  - M_SC_INVENSYNC 다음으로!!
	//{
	//	CheckPrize(FID, PRIZE_TYPE_BUDDHISM_YISHI, UsedUMoney);
	//}
	// 공덕값 변경
	if(UsedUMoney > 0)
		ChangeFamilyVirtue(Family_Virtue0_Type_UseMoney, FID, UsedUMoney);
	if(UsedGMoney > 0)
		ChangeFamilyVirtue(Family_Virtue0_Type_UsePoint, FID, UsedGMoney);
	// 공덕값 변경
	ChangeFamilyVirtue(Family_Virtue0_Type_Jisi, FID);
	// 경험값증가
	switch (ActionType)
	{
	case AT_PRAYRITE:
		if (bYiSiMaster)
			ChangeFamilyExp(Family_Exp_Type_ReligionYiSiMaster, FID);
		break;
	case AT_PRAYMULTIRITE:
		if (bYiSiMaster)
			ChangeFamilyExp(Family_Exp_Type_ReligionYiSiMaster, FID);
		else
			ChangeFamilyExp(Family_Exp_Type_ReligionYiSiSlaver, FID);
		break;
	case AT_PRAYXIANG:
		if (ActPos >= ACTPOS_R0_XIANG_INDOOR_1 && ActPos <= ACTPOS_R0_XIANG_INDOOR_6)
			ChangeFamilyExp(Family_Exp_Type_ReligionTouXiang, FID);
		if (ActPos >= ACTPOS_R0_XIANG_OUTDOOR_1 && ActPos <= ACTPOS_R0_XIANG_OUTDOOR_7)
			ChangeFamilyExp(Family_Exp_Type_ReligionPuTongXiang, FID);
		break;
	}

	// Inven처리정형 통보
	if (SyncCode > 0)
	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	if (ActionType == AT_PRAYRITE && ActPos >= ACTPOS_R0_YISHI_1 && ActPos <= ACTPOS_R0_YISHI_6 )
	{
		CheckPrize(FID, PRIZE_TYPE_BUDDHISM_YISHI, UsedUMoney);
	}

	// 행사를 DB에 보관
	if (ActID == 0)
	{
		{
			MAKEQUERY_Religion_InsertAct(strQ, m_SharedPlaceType, FID, NpcID, ActionType, Secret); // 0이면 불교구역을 의미한다.
			CDBConnectionRest	DB(DBCaller);
			DB_EXE_QUERY(DB, Stmt, strQ);
			if (nQueryResult == true)
			{
				DB_QUERY_RESULT_FETCH(Stmt, row);
				if ( IS_DB_ROW_VALID(row) )
				{
					COLUMN_DIGIT(row, 0, uint32, ai);
					ActID = ai;
				}
			}
		}
		m_ReligionActListHistory[NpcID].Reset();
	}
	if (ActID > 0)
	{
		MAKEQUERY_Religion_InsertPray(strQ, ActID, FID, Secret, Pray);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning(L"DB_EXE_PREPARESTMT failed");
			return ActID;
		}
		SQLLEN	len1 = itembuf[0] * sizeof(uint32) + 1;
		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, sizeof(itembuf), 0, itembuf, 0, &len1);
		if (!nBindResult)
		{
			sipwarning(L"DB_EXE_BINDPARAM_IN failed");
			return ActID;
		}
		DB_EXE_PARAMQUERY(Stmt, strQ);
	}

	if ((ActionType == AT_PRAYRITE || ActionType == AT_PRAYMULTIRITE) &&
		bYiSiMaster && ItemNum > 0)
	{
		// 행사결과를 위한 행사종류결정
		_YishiResultType	yisiType = YishiResultType_Large;
		if (ActionType == AT_PRAYMULTIRITE)
			yisiType = YishiResultType_Multi;
		else
		{
			if (!bTaoCanItem) // 세트아이템인 경우에는 대형으로 처리한다.
			{
				if (ItemNum <= 3)
					yisiType = YishiResultType_Small;
				else if (ItemNum <= 6)
					yisiType = YishiResultType_Medium;
			}
		}

		FAMILY	*pFamily = GetFamilyData(FID);
		if (pFamily)
			AddSharedYiSiResult(NpcID, yisiType, ActID, FID, pFamily->m_Name);
	}

	return ActID;
}

//////////////////////////////////////////////////////////////////////////
// 불교구역대형행사관련
//////////////////////////////////////////////////////////////////////////
void	cb_M_GMCS_SBY_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID FID;
	_msgin.serial(FID);
	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_GMCS_SBY_LIST(FID, _tsid);
	}
}

void	cb_M_GMCS_SBY_DEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		ListID;
	_msgin.serial(FID, ListID);

	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_GMCS_SBY_DEL(FID, ListID, _tsid);
	}
}

CVariable<uint16>	SBYMaxNum("rroom","SBYMaxNum", "SBYMaxNum", 0, 0, true );
static uint16 GetSBYMaxUserNum()
{
	uint16 un = SBYMaxNum.get();
	if (un == 0)
		return MAX_NUM_SBYUSER;
	return un;
}
void	DBCB_DBSBYLoad(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ROOMID		RoomID		= *(T_ROOMID*)(argv[0]);
	CReligionWorld* pWorld	= GetReligionWorld(RoomID);
	if (pWorld == NULL)
	{
		sipwarning("GetReligionWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	if (!nQueryResult)
		return;

	uint32	nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32 ListID;
		CReligionSBYBaseData newData(GetSBYMaxUserNum());
		resSet->serial(ListID, newData.m_ActTarget, newData.m_ActProperty, newData.m_ActCycle, newData.m_BeginTimeS, newData.m_BackMusicID, newData.m_ActText);

		uint8	byItemBuf[MAX_NUM_BYITEM * sizeof(uint32) + 1];
		memset(byItemBuf, 0, sizeof(byItemBuf));
		uint32		len=0;			
		resSet->serialBufferWithSize(byItemBuf, len);
		newData.m_ActItemNum = byItemBuf[0];
		if (newData.m_ActItemNum > 0)
		{
			uint32* pItemID = (uint32*)(&byItemBuf[1]);
			for (int k = 0; k < newData.m_ActItemNum; k++)
			{
				newData.m_ActItem[k] = *pItemID;
				pItemID++;
			}
		}
		pWorld->m_SBYList.insert(std::make_pair(ListID, newData));
	}
	pWorld->ResetCurrentSBY();
}

void CReligionWorld::ReloadEditedSBY()
{
	MAKEQUERY_Religion_SBY_Load(strQuery);
	DBCaller->execute(strQuery, DBCB_DBSBYLoad,
		"D",
		CB_,		m_RoomID);
}

void	cb_M_GMCS_SBY_EDIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;	_msgin.serial(FID);

	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
	if (inManager == NULL)
		return;

	uint32 ListID;
	CReligionSBYBaseData newData(GetSBYMaxUserNum());
	_msgin.serial(ListID);
	NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActTarget, MAX_LEN_BUDDISMYISHITARGET, FID);
	NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActProperty, MAX_LEN_BUDDISMYISHIPROPERTY, FID);
	_msgin.serial(newData.m_ActCycle, newData.m_BeginTimeS, newData.m_BackMusicID);
	NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActText, MAX_LEN_BUDDISMYISHICONTENT, FID);
	_msgin.serial(newData.m_ActItemNum);
	if (newData.m_ActItemNum > MAX_NUM_BYITEM)
	{
		sipwarning("Over MAX_NUM_BYITEM:num=%d", newData.m_ActItemNum);
		return;
	}
	for (uint8 i = 0; i < newData.m_ActItemNum; i++)
	{
		_msgin.serial(newData.m_ActItem[i]);
	}

	uint32	uListID = inManager->EditSBY(ListID, newData);
	uint32	errCode = ERR_NOERR;
	if (uListID == 0)
		errCode = E_CANT_EDIT_SBY;

	CMessage mes(M_GMSC_SBY_EDITRES);
	mes.serial(FID, errCode);
	if (errCode == ERR_NOERR)
	{
		mes.serial(uListID, newData.m_ActTarget, newData.m_ActProperty, newData.m_ActCycle, newData.m_BeginTimeS);
		mes.serial(newData.m_BackMusicID, newData.m_ActText, newData.m_ActItemNum);
		for (uint8 i = 0; i < newData.m_ActItemNum; i++)
		{
			mes.serial(newData.m_ActItem[i]);
		}
	}
	CUnifiedNetwork::getInstance()->send(_tsid, mes);
}

void	cb_M_GMCS_SBY_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;	_msgin.serial(FID);

	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
	if (inManager == NULL)
		return;

	CReligionSBYBaseData newData(GetSBYMaxUserNum());
	NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActTarget, MAX_LEN_BUDDISMYISHITARGET, FID);
	NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActProperty, MAX_LEN_BUDDISMYISHIPROPERTY, FID);
	_msgin.serial(newData.m_ActCycle, newData.m_BeginTimeS, newData.m_BackMusicID);
	NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActText, MAX_LEN_BUDDISMYISHICONTENT, FID);
	_msgin.serial(newData.m_ActItemNum);
	if (newData.m_ActItemNum > MAX_NUM_BYITEM)
	{
		sipwarning("Over MAX_NUM_BYITEM:num=%d", newData.m_ActItemNum);
		return;
	}
	for (uint8 i = 0; i < newData.m_ActItemNum; i++)
	{
		_msgin.serial(newData.m_ActItem[i]);
	}
	
	if (newData.m_ActCycle == SBY_EVERYDAY && newData.m_BeginTimeS >= SECOND_PERONEDAY)
	{
		sipwarning("Everyday BeginTime invalid");
		return;
	}

	uint32	uListID = inManager->AddSBY(newData);
	uint32	errCode = ERR_NOERR;
	if (uListID == 0)
		errCode = E_CANT_ADD_SBY;

	CMessage mes(M_GMSC_SBY_ADDRES);
	mes.serial(FID, errCode);
	if (errCode == ERR_NOERR)
	{
		mes.serial(uListID, newData.m_ActTarget, newData.m_ActProperty, newData.m_ActCycle, newData.m_BeginTimeS);
		mes.serial(newData.m_BackMusicID, newData.m_ActText, newData.m_ActItemNum);
		for (uint8 i = 0; i < newData.m_ActItemNum; i++)
		{
			mes.serial(newData.m_ActItem[i]);
		}
	}
	CUnifiedNetwork::getInstance()->send(_tsid, mes);
}

void	cb_M_CS_SBY_ADDREQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		ActID;
	_msgin.serial(FID, ActID);
	CReligionWorld *inManager = GetReligionWorldFromFID(FID);
	if (inManager != NULL)
		inManager->on_M_CS_SBY_ADDREQ(FID, ActID, _tsid);
}

void	cb_M_CS_SBY_EXIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_SBY_EXIT, _msgin, _tsid);
}

void	cb_M_CS_SBY_OVER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_SBY_OVER, _msgin, _tsid);
}

void	cb_M_CS_PBY_MAKE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_PBY_MAKE, _msgin, _tsid);
}

void	cb_M_CS_PBY_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_PBY_SET, _msgin, _tsid);
}

void	cb_M_CS_PBY_JOINSET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_PBY_JOINSET, _msgin, _tsid);
}


void	cb_M_CS_PBY_ADDREQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_PBY_ADDREQ, _msgin, _tsid);
}

void	cb_M_CS_PBY_EXIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_PBY_EXIT, _msgin, _tsid);
}

void	cb_M_CS_PBY_OVERUSER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_PBY_OVERUSER, _msgin, _tsid);
}

void	cb_M_CS_PBY_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_PBY_CANCEL, _msgin, _tsid);
}

void	cb_M_CS_PBY_UPDATESTATE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_PBY_UPDATESTATE, _msgin, _tsid);
}

void CReligionWorld::on_M_GMCS_SBY_LIST(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	uint8 actNum = (uint8)m_SBYList.size();
	CMessage mes(M_GMSC_SBY_LIST);
	mes.serial(FID, actNum);

	std::map<uint32, CReligionSBYBaseData>::iterator it;
	for (it = m_SBYList.begin(); it != m_SBYList.end(); it++)
	{
		uint32	listid = it->first;
		CReligionSBYBaseData& basedata = it->second;
		mes.serial(listid, basedata.m_ActTarget, basedata.m_ActProperty, basedata.m_ActCycle, basedata.m_BeginTimeS);
		mes.serial(basedata.m_BackMusicID, basedata.m_ActText, basedata.m_ActItemNum);
		for (uint8 i = 0; i < basedata.m_ActItemNum; i++)
		{
			mes.serial(basedata.m_ActItem[i]);
		}
	}
	CUnifiedNetwork::getInstance()->send(_tsid, mes);
}

void CReligionWorld::on_M_GMCS_SBY_DEL(T_FAMILYID FID, uint32 ListID, SIPNET::TServiceId _tsid)
{
	MAKEQUERY_Religion_SBY_Delete(strQuery, ListID);
	DBCaller->execute(strQuery);

	m_SBYList.erase(ListID);
	ResetCurrentSBY();

	uint32 errCode = ERR_NOERR;
	CMessage mes(M_GMSC_SBY_DELRES);
	mes.serial(FID, errCode, ListID);
	CUnifiedNetwork::getInstance()->send(_tsid, mes);

	//sipdebug("on_M_GMCS_SBY_DEL:ListID=%d", ListID); byy krs
}

void CReligionWorld::on_M_CS_SBY_ADDREQ(T_FAMILYID FID, uint32 ActID, SIPNET::TServiceId _tsid)
{
	ReligionChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	uint32 nCandiChannelID = pChannelData->m_ChannelID;
	uint16 joinPos = 0;
	uint32 errCode = pChannelData->on_M_CS_SBY_ADDREQ(FID, ActID, &joinPos, _tsid);

	if (errCode != ERR_NOERR && errCode != E_SBY_USERFULL)
	{
		sipdebug("Request join result=%d, ChannelID=%d, FID=%d, ActID=%d", errCode, pChannelData->m_ChannelID, FID, ActID);
	}
	
	if (errCode == E_SBY_USERFULL)	// 현재선로에 참가할 자리가 없으면 다른 선로를 찾는다
	{
		ReligionChannelData *pCandiChannel = NULL;
		int nCandiPos_base0 = -1;

		FAMILY	*pFamily = GetFamilyData(FID); // by krs

		// 현재 있는 선로에서 참가 가능한 선로를 찾는다
		for (ReligionChannelData *pChannel = GetFirstChannel(); pChannel != NULL; pChannel = GetNextChannel())
		{
			if (pChannelData->m_ChannelID == pChannel->m_ChannelID)
				continue;

			if ((pFamily->m_bIsMobile == 2 && pChannel->m_ChannelID < ROOM_UNITY_CHANNEL_START) || (pFamily->m_bIsMobile == 0 && pChannel->m_ChannelID > ROOM_UNITY_CHANNEL_START)) // by krs
				continue;

			if (pChannel->m_CurrentSBY)
			{
				nCandiPos_base0 = pChannel->m_CurrentSBY->GetCandiJoinPos_base0();
				if (nCandiPos_base0 >= 0)
				{
					nCandiChannelID = pChannel->m_ChannelID;
					pCandiChannel = pChannel;
					break;
				}
			}
		}
		// 가능한 선로가 없으면 새로운 선로를 창조한다
		if (pCandiChannel == NULL)
		{
			ReligionChannelData *pNewChannel = CreateNewChannel();
			nCandiPos_base0 = 0;
			nCandiChannelID = pNewChannel->m_ChannelID;
			pCandiChannel = pNewChannel;
		}
		if (pCandiChannel == NULL)
		{
			sipwarning("Can't new channel");
			return;
		}
		else
		{
			CReligionSBY::CJoinBooking newBooking;
			newBooking.m_FID = FID;
			newBooking.m_BookingTime = (uint32)GetDBTimeSecond();
			newBooking.m_BookingPos_base1 = nCandiPos_base0 + 1;
			pCandiChannel->m_CurrentSBY->m_JoinBookingUser[FID] = newBooking;
			errCode = ERR_NOERR;
			joinPos = nCandiPos_base0;
		}
	}

	CMessage mes(M_SC_SBY_ADDRES);
	mes.serial(FID, errCode, ActID, joinPos, nCandiChannelID);
	CUnifiedNetwork::getInstance()->send(_tsid, mes);

}

static uint32 nCandiSBYListID = 1;
uint32 CReligionWorld::AddSBY(CReligionSBYBaseData& newData)
{
	if (m_SBYList.size() >= MAX_NUM_SBYLIST)
		return 0;
	uint32 newID = 0;

	uint8	itembuf[MAX_NUM_BYITEM * sizeof(uint32) + 1];
	itembuf[0] = 0;
	uint32	*itemlist = (uint32*)(&itembuf[1]);
	for (uint8 i = 0; i < newData.m_ActItemNum; i++)
	{
		*itemlist = newData.m_ActItem[i];
		itembuf[0] ++;
		itemlist ++;
	}

	MAKEQUERY_Religion_SBY_Insert(strQ, newData.m_ActTarget.c_str(), newData.m_ActProperty.c_str(), newData.m_ActCycle, newData.m_BeginTimeS, newData.m_BackMusicID, newData.m_ActText.c_str());
	CDBConnectionRest	DB(DBCaller);
	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
	if (!nPrepareRet)
	{
		sipwarning(L"DB_EXE_PREPARESTMT failed");
		return 0 ;
	}
	SQLLEN	len1 = itembuf[0] * sizeof(uint32) + 1;
	DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, sizeof(itembuf), 0, itembuf, 0, &len1);
	if (!nBindResult)
	{
		sipwarning(L"DB_EXE_BINDPARAM_IN failed");
		return 0 ;
	}
	DB_EXE_PARAMQUERY(Stmt, strQ);
	if (nQueryResult == true)
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( IS_DB_ROW_VALID(row) )
		{
			COLUMN_DIGIT(row, 0, uint32, ai);
			newID = ai;
		}
	}
	else
		return 0;

	m_SBYList.insert(std::make_pair(newID, newData));
	
	ResetCurrentSBY();
	
	//sipdebug("AddSBY:ListID=%d", newID); byy krs
	return newID;
}

uint32 CReligionWorld::EditSBY(uint32 ListID, CReligionSBYBaseData& newData)
{
	uint8	itembuf[MAX_NUM_BYITEM * sizeof(uint32) + 1];
	itembuf[0] = 0;
	uint32	*itemlist = (uint32*)(&itembuf[1]);
	for (uint8 i = 0; i < newData.m_ActItemNum; i++)
	{
		*itemlist = newData.m_ActItem[i];
		itembuf[0] ++;
		itemlist ++;
	}

	MAKEQUERY_Religion_SBY_Modify(strQ, ListID, newData.m_ActTarget.c_str(), newData.m_ActProperty.c_str(), newData.m_ActCycle, newData.m_BeginTimeS, newData.m_BackMusicID, newData.m_ActText.c_str());
	CDBConnectionRest	DB(DBCaller);
	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
	if (!nPrepareRet)
	{
		sipwarning(L"DB_EXE_PREPARESTMT failed");
		return 0 ;
	}
	SQLLEN	len1 = itembuf[0] * sizeof(uint32) + 1;
	DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, sizeof(itembuf), 0, itembuf, 0, &len1);
	if (!nBindResult)
	{
		sipwarning(L"DB_EXE_BINDPARAM_IN failed");
		return 0 ;
	}
	DB_EXE_PARAMQUERY(Stmt, strQ);

	std::map<uint32, CReligionSBYBaseData>::iterator it = m_SBYList.find(ListID);
	if (it == m_SBYList.end())
		return 0;
	it->second = newData;
	ResetCurrentSBY();
	
	//sipdebug("EditSBY:ListID=%d", ListID); byy krs
	return ListID;
}

void CReligionWorld::ResetCurrentSBY()
{
	for (ReligionChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		pChannelData->m_CurrentSBYUpdateFlag = true;

		if (pChannelData->m_CurrentSBY == NULL)
			continue;

		// 행사도중에는 reset할수 없다
		if (pChannelData->m_CurrentSBY->IsDoing())
			continue;

		ucstring btime = pChannelData->m_CurrentSBY->m_BaseData.GetRealBeginTime();
		//sipdebug(L"Reset SBY:ChannelID=%d, BeginTime=%s", pChannelData->m_ChannelID, btime.c_str()); byy krs

		delete pChannelData->m_CurrentSBY;
		pChannelData->m_CurrentSBY = NULL;
	}
}

void	CReligionWorld::RemoveSBYBooking(T_FAMILYID FID)
{
	for (ReligionChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		if (pChannelData->m_CurrentSBY)
			pChannelData->m_CurrentSBY->RemoveBooking(FID);
	}
}

CReligionSBYBaseData* CReligionWorld::GetCandiSBY()
{
	CReligionSBYBaseData* pCandi = NULL;
	uint32	minRemainTime = 0xFFFFFFFF;
	uint32 curDaysSeconds = GetDBTimeDays() * SECOND_PERONEDAY;
	uint32 curSeconds = (uint32)GetDBTimeSecond();

	std::map<uint32, CReligionSBYBaseData>::iterator it;
	for (it = m_SBYList.begin(); it != m_SBYList.end(); it++)
	{
		CReligionSBYBaseData& data = it->second;
		uint32 SBYBeginTime = data.m_BeginTimeS;
		if (data.m_ActCycle == SBY_EVERYDAY)
		{
			SBYBeginTime = curDaysSeconds + data.m_BeginTimeS;
			if (SBYBeginTime <= curSeconds)
				SBYBeginTime += SECOND_PERONEDAY;
		}
		if (SBYBeginTime <= curSeconds)
			continue;
		uint32 remainTime = SBYBeginTime - curSeconds;
		if (remainTime < minRemainTime)
		{
			pCandi = &(it->second);
			minRemainTime = remainTime;
		}
	}
	return pCandi;
}

void CReligionWorld::UpdateBY()
{
	CReligionSBYBaseData*	pCandiSBY = NULL;
	for (ReligionChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		bool bChange = false;
		if (pChannelData->m_CurrentPBY)
		{
			if (!pChannelData->m_CurrentPBY->Update())
			{
				delete pChannelData->m_CurrentPBY;
				pChannelData->m_CurrentPBY = NULL;
				bChange = true;
			}
		}
		else
		{
			if (pChannelData->m_CurrentSBY)
			{
				if (pChannelData->m_CurrentSBY->Update())
					continue;
				else
				{
					delete pChannelData->m_CurrentSBY;
					pChannelData->m_CurrentSBY = NULL;
					pChannelData->m_CurrentSBYUpdateFlag = true;
				}
			}

			if (pChannelData->m_CurrentSBYUpdateFlag && (pChannelData->m_ChannelID < ROOM_SPECIAL_CHANNEL_START))
			{
				if (pCandiSBY == NULL)
					pCandiSBY = GetCandiSBY();
				if (pCandiSBY)
				{
					uint32 ActID = nCandiSBYID++;
					pChannelData->m_CurrentSBY = new CReligionSBY(ActID, pCandiSBY, pChannelData);
					ucstring btime = pChannelData->m_CurrentSBY->m_BaseData.GetRealBeginTime();
					if (pCandiSBY->m_ActCycle == SBY_ONLYONE)
					{
						sipdebug(L"Set SBY(OnlyOne):ChannelID=%d, ActID=%d, BeginTime=%s", pChannelData->m_ChannelID, ActID, btime.c_str());
					}
					else
					{
						sipdebug(L"Set SBY(EveryDay):ChannelID=%d, ActID=%d, BeginTime=%s", pChannelData->m_ChannelID, ActID, btime.c_str());
					}
				}
				bChange = true;
				pChannelData->m_CurrentSBYUpdateFlag = false;
			}
		}
		if (bChange)
			pChannelData->SendSettingBYInfo();

	}
}

void	CReligionWorld::LogReligionBY(SIPBASE::CLog &log)
{

	log.displayNL("----- Current edited SBY number = %d -----", m_SBYList.size());
	std::map<uint32, CReligionSBYBaseData>::iterator it;
	for (it = m_SBYList.begin(); it != m_SBYList.end(); it++)
	{
		uint32	listid = it->first;
		CReligionSBYBaseData& basedata = it->second;
		log.displayNL(L"m_ActCycle=%d, BeginTime=%s, m_ActTarget=%s, m_ActProperty=%s, m_BackMusicID =%d, m_ActItemNum=%d", 
			basedata.m_ActCycle, basedata.GetRealBeginTime().c_str(), basedata.m_ActTarget.c_str(), basedata.m_ActProperty.c_str(), basedata.m_BackMusicID, basedata.m_ActItemNum);
	}

	for (ReligionChannelData *pChannel = GetFirstChannel(); pChannel != NULL; pChannel = GetNextChannel())
	{
		log.displayNL("-------------------------------------------", m_SBYList.size());
		if (pChannel->m_CurrentPBY)
		{
			log.displayNL(L"Channel=%d, PBYID=%d, BeginTime=%s, MasterID=%d, MasterName=%s, CurStep=%d, CurUserNum=%d",
				pChannel->m_ChannelID, pChannel->m_CurrentPBY->m_PBYID, pChannel->m_CurrentPBY->m_BaseData.GetRealBeginTime().c_str(), pChannel->m_CurrentPBY->m_BaseData.m_MasterID, pChannel->m_CurrentPBY->m_BaseData.m_MasterName.c_str(), pChannel->m_CurrentPBY->m_CurrentStep, pChannel->m_CurrentPBY->m_JoinUser.size());
		}
		if (pChannel->m_CurrentSBY)
		{
			log.displayNL(L"Channel=%d, SBYID=%d, BeginTime=%s, CurStep=%d, CurUserNum=%d",
				pChannel->m_ChannelID, pChannel->m_CurrentSBY->m_SBYID, pChannel->m_CurrentSBY->m_BaseData.GetRealBeginTime().c_str(), pChannel->m_CurrentSBY->m_CurrentStep, pChannel->m_CurrentSBY->m_JoinUser.size());
		}
	}		
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void	ReligionChannelData::SBYStart(CReligionSBY* pSBY)
{
	{
		FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_SBY_START)
		{
			msgOut.serial(pSBY->m_SBYID);
			msgOut.serial(pSBY->m_BaseData.m_BackMusicID, pSBY->m_BaseData.m_ActItemNum);
			for (uint8 i = 0; i < pSBY->m_BaseData.m_ActItemNum; i++)
			{
				msgOut.serial(pSBY->m_BaseData.m_ActItem[i]);
			}
		}
		FOR_END_FOR_FES_ALL()
	}
	//sipdebug("SBYStart:ChannelID=%d, ActID=%d", m_ChannelID, pSBY->m_SBYID); byy krs
}

void	ReligionChannelData::SBYOver(CReligionSBY* pSBY)
{
	{
		FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_SBY_OVER)
		{
			msgOut.serial(pSBY->m_SBYID);
		}
		FOR_END_FOR_FES_ALL()
	}
	//sipdebug("SBYOver:ChannelID=%d, ActID=%d", m_ChannelID, pSBY->m_SBYID); byy krs
}

void	ReligionChannelData::SBYChangeStep(CReligionSBY* pSBY, uint16 curStep)
{
/*
	{
		FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_SBY_CHANGESTEP)
		{
			msgOut.serial(pSBY->m_SBYID, curStep);
			if (curStep == BUDDISMYISHI_READYSTEP)
			{
				msgOut.serial(pSBY->m_BaseData.m_BackMusicID, pSBY->m_BaseData.m_ActItemNum);
				for (uint8 i = 0; i < pSBY->m_BaseData.m_ActItemNum; i++)
				{
					msgOut.serial(pSBY->m_BaseData.m_ActItem[i]);
				}
			}
		}
		FOR_END_FOR_FES_ALL()
	}
	sipdebug("SBYChangeStep:ChannelID=%d, ActID=%d, curStep=%d", m_ChannelID, pSBY->m_SBYID, curStep);
	*/
}

void	ReligionChannelData::SBYNewJoin(CReligionSBY* pSBY, T_FAMILYID JoinUserID, uint16 JoinPos)
{
	uint16	curJoinNum = pSBY->m_JoinUser.size();

	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_SBY_ADDUSER)
	{
		msgOut.serial(pSBY->m_SBYID, JoinUserID, JoinPos, curJoinNum);
	}
	FOR_END_FOR_FES_ALL()

	//sipdebug("SBYNewJoin:ChannelID=%d, ActID=%d, JoinID=%d, JoinPos=%d, curNum=%d", m_ChannelID, pSBY->m_SBYID, JoinUserID, JoinPos, curJoinNum); byy krs
}

void	ReligionChannelData::SBYExitJoin(CReligionSBY* pSBY, T_FAMILYID ExitUserID, uint16 JoinPos)
{
	uint16	curJoinNum = pSBY->m_JoinUser.size();

	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_SBY_EXITUSER)
	{
		msgOut.serial(pSBY->m_SBYID, ExitUserID, JoinPos, curJoinNum);
	}
	FOR_END_FOR_FES_ALL()

	//sipdebug("SBYExitJoin:ChannelID=%d, ActID=%d, ExitID=%d, ExitPos=%d, curNum=%d", m_ChannelID, pSBY->m_SBYID, ExitUserID, JoinPos, curJoinNum); byy krs
}

void	ReligionChannelData::PBYNewJoin(CReligionPBY* pPBY, T_FAMILYID JoinUserID, uint16 JoinPos)
{
	uint16	curJoinNum = pPBY->m_JoinUser.size();

	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_PBY_ADDUSER)
	{
		msgOut.serial(pPBY->m_PBYID, JoinUserID, JoinPos, curJoinNum);
	}
	FOR_END_FOR_FES_ALL()

	//	sipdebug("PBYNewJoin:ChannelID=%d, ActID=%d, JoinID=%d, JoinPos=%d, curNum=%d", m_ChannelID, pPBY->m_PBYID, JoinUserID, JoinPos, curJoinNum); byy krs
}

void	ReligionChannelData::PBYExitJoin(CReligionPBY* pPBY, T_FAMILYID ExitUserID, uint16 JoinPos)
{
	uint16	curJoinNum = pPBY->m_JoinUser.size();

	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_PBY_EXITUSER)
	{
		msgOut.serial(pPBY->m_PBYID, ExitUserID, JoinPos, curJoinNum);
	}
	FOR_END_FOR_FES_ALL()

	//sipdebug("PBYExitJoin:ChannelID=%d, ActID=%d, ExitID=%d, ExitPos=%d, curNum=%d", m_ChannelID, pPBY->m_PBYID, ExitUserID, JoinPos, curJoinNum); byy krs
}

void	ReligionChannelData::PBYCancel(CReligionPBY* pPBY)
{
	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_PBY_CANCEL)
	{
		msgOut.serial(pPBY->m_PBYID);
	}
	FOR_END_FOR_FES_ALL()

	//sipdebug("PBYCancel:ChannelID=%d, ActID=%d", m_ChannelID, pPBY->m_PBYID); byy krs
}

void	ReligionChannelData::PBYChangeStep(CReligionPBY* pPBY, uint16 ActStep)
{
	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_PBY_UPDATESTATE)
	{
		msgOut.serial(pPBY->m_PBYID, ActStep);
		if (ActStep == BUDDISMYISHI_REALSTARTSTEP)
		{
			msgOut.serial(pPBY->m_BaseData.m_ActItemNum);
			for (uint8 i = 0; i < pPBY->m_BaseData.m_ActItemNum; i++)
			{
				msgOut.serial(pPBY->m_BaseData.m_ActItem[i]);
			}
		}
	}
	FOR_END_FOR_FES_ALL()

	//sipdebug("PBYChangeStep:ChannelID=%d, ActID=%d, ActStpe=%d", m_ChannelID, pPBY->m_PBYID, ActStep); byy krs
}

void	ReligionChannelData::PBYOver(CReligionPBY* pPBY)
{
	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_PBY_OVERALL)
	{
		msgOut.serial(pPBY->m_PBYID);
	}
	FOR_END_FOR_FES_ALL()

	//sipdebug("PBYOver:ChannelID=%d, ActID=%d", m_ChannelID, pPBY->m_PBYID); byy krs
}

void ReligionChannelData::SendSettingBYInfo(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	uint32					actID = 0;
	uint8					actMode = 0;
	CReligionBYBaseData*	baseData = NULL;
	uint16					curJoinNum;
	if (m_CurrentPBY)
	{
		actID = m_CurrentPBY->m_PBYID;
		actMode = BUDDISMYISHI_PERSONAL;
		baseData = dynamic_cast<CReligionBYBaseData*>(&m_CurrentPBY->m_BaseData);
		curJoinNum = m_CurrentPBY->m_JoinUser.size();
	}
	else if (m_CurrentSBY)
	{
		actID = m_CurrentSBY->m_SBYID;
		actMode = BUDDISMYISHI_SYSTEM | (m_CurrentSBY->m_BaseData.m_ActCycle << 4);
		baseData = dynamic_cast<CReligionBYBaseData*>(&m_CurrentSBY->m_BaseData);
		curJoinNum = m_CurrentSBY->m_JoinUser.size();
	}

	if (FID == NOFAMILYID)
	{
		FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_BY_CURRENT)
		{
			msgOut.serial(actID);
			if (baseData != NULL)
			{
				msgOut.serial(actMode);
				msgOut.serial(baseData->m_MasterID, baseData->m_MasterName);
				msgOut.serial(baseData->m_ActTarget, baseData->m_ActProperty, baseData->m_ActText, baseData->m_BeginTimeS, baseData->m_MaxNumUser);
				msgOut.serial(curJoinNum);
			}
		}
		FOR_END_FOR_FES_ALL()
	}
	else
	{
		uint32 zero = 0;
		CMessage mes(M_SC_BY_CURRENT);
		mes.serial(FID, zero);
		mes.serial(actID);
		if (baseData != NULL)
		{
			mes.serial(actMode);
			mes.serial(baseData->m_MasterID, baseData->m_MasterName);
			mes.serial(baseData->m_ActTarget, baseData->m_ActProperty, baseData->m_ActText, baseData->m_BeginTimeS, baseData->m_MaxNumUser);
			mes.serial(curJoinNum);
		}
		CUnifiedNetwork::getInstance()->send(sid, mes);
	}
}

void ReligionChannelData::SendDoingBYState(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	if (m_CurrentPBY)
	{
		// M_SC_PSY_CURSTATE	방에 새로 들어온 캐릭터에게 진행중인 개인대형행사상태 통지	[d:행사id][b:행사아이템개수][[d:행사아이템id]][w:행사진척단계][d:단계경과시간S][w:현재참가수][[d:참가자id][w:참가위치]]
		m_CurrentPBY->SendCurrentState(FID, sid);
	}
	else if (m_CurrentSBY)
	{
		m_CurrentSBY->SendCurrentState(FID, sid);
	}
}

bool	ReligionChannelData::ProcSBYBooking(T_FAMILYID FID)
{
	if (m_CurrentSBY)
	{
		uint16 pos;
		bool bBooking = m_CurrentSBY->ProcSBYBooking(FID, &pos);
		if (bBooking)
		{
			sipdebug("Proc SBY booking : ChannelID=%d, FID=%d, pos=%d", m_ChannelID, FID, pos);
		}
	}
	return false;
}

uint32	ReligionChannelData::on_M_CS_SBY_ADDREQ(T_FAMILYID FID, uint32 ActID, uint16 *joinPos, SIPNET::TServiceId _tsid)
{
	uint32	errCode = ERR_NOERR;
	if (m_CurrentSBY == NULL)
	{
		errCode = E_SBY_CANTJOIN;
		sipdebug("There is no SBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
	}
	else
	{
		if (m_CurrentSBY->m_SBYID != ActID)
		{
			errCode = E_SBY_CANTJOIN;
			sipwarning("Mismatched SBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentSBY->m_SBYID, ActID);
		}
		else
		{
			errCode	= m_CurrentSBY->RequestJoin(FID, joinPos);
			//sipdebug(" on_M_CS_SBY_ADDREQ :: FID=%d, errCode=%d ", FID, errCode); // by krs
		}
	}
	return errCode;
}

void	ReligionChannelData::on_M_CS_SBY_EXIT(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	uint32	ActID;	_msgin.serial(ActID);
	if (m_CurrentSBY == NULL)
	{
		sipdebug("There is no SBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
	}
	else
	{
		if (m_CurrentSBY->m_SBYID != ActID)
		{
			sipwarning("Mismatched SBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentSBY->m_SBYID, ActID);
		}
		else
		{
			m_CurrentSBY->ExitJoin(FID);
		}
	}
}

void	ReligionChannelData::on_M_CS_SBY_OVER(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	uint32 ActID;	_msgin.serial(ActID);
	if (m_CurrentSBY == NULL)
	{
		// 이 로그는 필요없다. 행사에 참가하지 않고 방에 있는 사용자들도 이 파케트를 올려보내기때문이다.
		// sipdebug("There is no SBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	if (m_CurrentSBY->m_SBYID != ActID)
	{
		// 이 로그는 필요없다. 행사에 참가하지 않고 방에 있는 사용자들도 이 파케트를 올려보내기때문이다.
		// sipwarning("Mismatched SBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d, Reason: SBY already finished.", m_ChannelID, FID, m_CurrentSBY->m_SBYID, ActID);
		return;
	}
	if (m_CurrentSBY->m_CurrentStep != BUDDISMYISHI_ALLFINISH)
	{
		m_CurrentSBY->OverOneUser(FID);

		sipdebug("ChannelID=%d, ActID=%d, OverFID=%d", m_ChannelID, m_CurrentSBY->m_SBYID, FID);
	}
}
static uint32 nCandPBYID = 1;
void	ReligionChannelData::on_M_CS_PBY_MAKE(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	uint32	errCode = ERR_NOERR;
	uint32	actid = 0;
	uint32 curTime = (uint32)GetDBTimeSecond();
	if (m_CurrentPBY)
	{
		errCode = E_PBY_ALIVE;
		actid = 0;
	}
	else if (m_CurrentSBY)
	{
		if (curTime >= m_CurrentSBY->m_BaseData.m_BeginTimeS - MAX_TIMES_BUDDISMPBYYISHI)
		{
			errCode = E_PBY_CANTMAKE;
			actid = 0;
		}
	}

	if (errCode != ERR_NOERR)
	{
		CMessage	mes(M_SC_PBY_MAKERES);
		mes.serial(FID);
		mes.serial(errCode, actid);
		CUnifiedNetwork::getInstance()->send(_tsid, mes);
	}
	else
	{
		uint32 nPBYID = nCandPBYID++;
		{
			CReligionPBYBaseData newData;
			NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActTarget, MAX_LEN_BUDDISMYISHITARGET, FID);
			NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActProperty, MAX_LEN_BUDDISMYISHIPROPERTY, FID);
			_msgin.serial(newData.m_bActTextOpenFlag);
			NULLSAFE_SERIAL_WSTR(_msgin, newData.m_ActText, MAX_LEN_BUDDISMYISHICONTENT, FID);
			_msgin.serial(newData.m_ActItemNum);
			if (newData.m_ActItemNum > MAX_NUM_BYITEM)
			{
				sipwarning("Over MAX_NUM_BYITEM:num=%d", newData.m_ActItemNum);
				return;
			}
			for (uint8 i = 0; i < newData.m_ActItemNum; i++)
			{
				_msgin.serial(newData.m_ActItem[i]);
			}
			newData.m_BeginTimeS = curTime;
			newData.m_MasterID = FID;
			FAMILY*	pFamily = GetFamilyData(FID);
			if (pFamily != NULL)
			{
				newData.m_MasterName = pFamily->m_Name;
			}

			delete m_CurrentPBY;
			m_CurrentPBY = new CReligionPBY(nPBYID, &newData, this);
			errCode = ERR_NOERR;
			actid = nPBYID;

			ChangeFamilyExp(Family_Exp_Type_ReligionYiSiMaster, FID);
			sipdebug("MAKE new PBY:ChannelID=%d, ActID=%d, FID=%d", m_ChannelID, nPBYID, FID);
		}

		CMessage	mes(M_SC_PBY_MAKERES);
		mes.serial(FID);
		mes.serial(errCode, actid);
		CUnifiedNetwork::getInstance()->send(_tsid, mes);

		SendSettingBYInfo();
		
		m_CurrentPBY->ChangeStep(FID, BUDDISMYISHI_READYSTEP);
	}
}

void	ReligionChannelData::on_M_CS_PBY_SET(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	uint32 ActID;
	_msgin.serial(ActID);
	if (m_CurrentPBY == NULL)
	{
		sipwarning("There is no PBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	if (m_CurrentPBY->m_CurrentStep > BUDDISMYISHI_READYSTEP)
	{
		uint32		errCode = E_PBY_CANTSET;
		CMessage	mes(M_SC_PBY_SET);
		mes.serial(FID, errCode, ActID);
		CUnifiedNetwork::getInstance()->send(_tsid, mes);
		sipdebug("PBY started already:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	if (m_CurrentPBY->m_PBYID != ActID)
	{
		sipwarning("Mismatched PBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentPBY->m_PBYID, ActID);
		return;
	}
	if (m_CurrentPBY->m_BaseData.m_MasterID != FID)
	{
		sipwarning("No PBY master:ChannelID=%d, FID=%d, ServerID=%d", m_ChannelID, FID, m_CurrentPBY->m_BaseData.m_MasterID);
		return;
	}
	NULLSAFE_SERIAL_WSTR(_msgin, m_CurrentPBY->m_BaseData.m_ActTarget, MAX_LEN_BUDDISMYISHITARGET, FID);
	NULLSAFE_SERIAL_WSTR(_msgin, m_CurrentPBY->m_BaseData.m_ActProperty, MAX_LEN_BUDDISMYISHIPROPERTY, FID);
	_msgin.serial(m_CurrentPBY->m_BaseData.m_bActTextOpenFlag);
	NULLSAFE_SERIAL_WSTR(_msgin, m_CurrentPBY->m_BaseData.m_ActText, MAX_LEN_BUDDISMYISHICONTENT, FID);
	uint8 ItemNum;
	_msgin.serial(ItemNum);
	if (ItemNum > MAX_NUM_BYITEM)
	{
		sipwarning("Over MAX_NUM_BYITEM:num=%d", ItemNum);
		return;
	}
	m_CurrentPBY->m_BaseData.m_ActItemNum = ItemNum;
	for (uint8 i = 0; i < m_CurrentPBY->m_BaseData.m_ActItemNum; i++)
	{
		_msgin.serial(m_CurrentPBY->m_BaseData.m_ActItem[i]);
	}

	uint32		errCode = ERR_NOERR;
	CMessage	mes(M_SC_PBY_SET);
	mes.serial(FID, errCode, ActID);
	CUnifiedNetwork::getInstance()->send(_tsid, mes);

	//sipdebug("PBY edited:ChannelID=%d, FID=%d", m_ChannelID, FID); byy krs
}

void	ReligionChannelData::on_M_CS_PBY_JOINSET(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	if (m_CurrentPBY == NULL)
	{
		sipwarning("There is no PBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	if (m_CurrentPBY->m_CurrentStep > BUDDISMYISHI_READYSTEP)
	{
		sipwarning("PBY started already:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	uint32 ActID;
	_msgin.serial(ActID);
	if (m_CurrentPBY->m_PBYID != ActID)
	{
		sipwarning("Mismatched PBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentPBY->m_PBYID, ActID);
		return;
	}

	map<uint16, CPBYJoinData>::iterator it;
	for (it = m_CurrentPBY->m_JoinUser.begin(); it != m_CurrentPBY->m_JoinUser.end(); it++)
	{
		CPBYJoinData& joinData = it->second;
		if (joinData.m_FID == FID)
		{
			_msgin.serial(joinData.m_bOpenFlag);
			NULLSAFE_SERIAL_WSTR(_msgin, joinData.m_ActText, MAX_LEN_BUDDISMYISHICONTENT, FID);
			uint8 ItemNum;
			_msgin.serial(ItemNum);
			if (ItemNum > MAX_NUM_BYITEM)
			{
				sipwarning("Over MAX_NUM_BYITEM:num=%d", ItemNum);
				return;
			}
			T_ITEMSUBTYPEID		Items[MAX_NUM_BYITEM];
			for (uint8 i = 0; i < ItemNum; i++)
			{
				_msgin.serial(Items[i]);
			}
			if (ItemNum == 0)
			{
				joinData.m_Item = 0;
			}
			else
				joinData.m_Item = Items[0];
			return;
		}
	}
}

void	ReligionChannelData::on_M_CS_PBY_ADDREQ(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	if (m_CurrentPBY == NULL)
	{
		sipwarning("There is no PBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	uint32 ActID;
	_msgin.serial(ActID);
	if (m_CurrentPBY->m_PBYID != ActID)
	{
		sipwarning("Mismatched PBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentPBY->m_PBYID, ActID);
		return;
	}
	bool				bOpenFlag;
	T_COMMON_CONTENT	ActText;
	uint8				ItemNum;
	T_ITEMSUBTYPEID		Items[MAX_NUM_BYITEM];
	_msgin.serial(bOpenFlag);
	NULLSAFE_SERIAL_WSTR(_msgin, ActText, MAX_LEN_BUDDISMYISHICONTENT, FID);
	_msgin.serial(ItemNum);
	if (ItemNum > MAX_NUM_BYITEM)
	{
		sipwarning("Over MAX_NUM_BYITEM:num=%d", ItemNum);
		return;
	}
	for (uint8 i = 0; i < ItemNum; i++)
	{
		_msgin.serial(Items[i]);
	}
	uint16 Pos = 0;
	uint32 errCode = m_CurrentPBY->RequestJoin(FID, &Pos, bOpenFlag, ActText, ItemNum, Items);
	if (errCode != ERR_NOERR)
	{
		sipdebug("Request join result=%d, ChannelID=%d, FID=%d, ActID=%d", errCode, m_ChannelID, FID, ActID);
	}
	//sipdebug(" on_M_CS_PBY_ADDREQ :: FID=%d, errCode=%d ChannelID=%d, ActID=%d", FID, errCode, m_ChannelID, ActID); // by krs
	CMessage mes(M_SC_PBY_ADDRES);
	mes.serial(FID, errCode, ActID, Pos);
	CUnifiedNetwork::getInstance()->send(_tsid, mes);
}

void	ReligionChannelData::on_M_CS_PBY_EXIT(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	if (m_CurrentPBY == NULL)
	{
		sipwarning("There is no PBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	uint32 ActID;
	_msgin.serial(ActID);
	if (m_CurrentPBY->m_PBYID != ActID)
	{
		sipwarning("Mismatched PBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentPBY->m_PBYID, ActID);
		return;
	}
	m_CurrentPBY->ExitJoin(FID);
}

void	ReligionChannelData::on_M_CS_PBY_OVERUSER(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	if (m_CurrentPBY == NULL)
	{
		sipwarning("There is no PBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	uint32 ActID;
	_msgin.serial(ActID);
	if (m_CurrentPBY->m_PBYID != ActID)
	{
		sipwarning("Mismatched PBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentPBY->m_PBYID, ActID);
		return;
	}
	m_CurrentPBY->SuccessOverJoin(FID);
}

void	ReligionChannelData::on_M_CS_PBY_CANCEL(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	if (m_CurrentPBY == NULL)
	{
		sipwarning("There is no PBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	uint32 ActID;
	_msgin.serial(ActID);
	if (m_CurrentPBY->m_PBYID != ActID)
	{
		sipwarning("Mismatched PBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentPBY->m_PBYID, ActID);
		return;
	}
	m_CurrentPBY->ProcCancel(FID);
}

void	ReligionChannelData::on_M_CS_PBY_UPDATESTATE(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	if (m_CurrentPBY == NULL)
	{
		sipwarning("There is no PBY:ChannelID=%d, FID=%d", m_ChannelID, FID);
		return;
	}
	uint32 ActID;
	uint16 ActStep;
	_msgin.serial(ActID, ActStep);
	if (m_CurrentPBY->m_PBYID != ActID)
	{
		sipwarning("Mismatched PBYID:ChannelID=%d, FID=%d, ServerID=%d, ClientID=%d", m_ChannelID, FID, m_CurrentPBY->m_PBYID, ActID);
		return;
	}
	m_CurrentPBY->ChangeStep(FID, ActStep);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool	CReligionSBY::Update()
{
	if (m_CurrentStep == BUDDISMYISHI_ALLFINISH)
		return false;

	if (m_CurrentStep == -1)
	{
		uint32 curTime = (uint32)GetDBTimeSecond();
		if (curTime >= m_BaseData.GetReadyDBTime())
		{
			if (m_BYListener)
			{
				m_BYListener->SBYStart(this);
			}
			m_CurrentStep = BUDDISMYISHI_READYSTEP;
			m_FirstFinishLocalTime = 0;
		}
		return true;
	}

	if (m_CurrentStep >= BUDDISMYISHI_READYSTEP)
	{
		uint32 delatTime = (uint32)GetDBTimeSecond() - m_BaseData.GetReadyDBTime();
		if (delatTime >= MAX_TIMES_BUDDISMYISHI)
		{
			SetAllFinish();
			return false;
		}
	}

	if (m_CurrentStep == BUDDISMYISHI_FINISH)
	{
		if (IsAllFinished())
		{
			SetAllFinish();
			return false;
		}
		if (m_FirstFinishLocalTime > 0)
		{
			TTime deltatime = CTime::getLocalTime() - m_FirstFinishLocalTime;
			if (deltatime >= 120000) // 2분
			{
				if (m_OverUser.size() * 2 > m_JoinUser.size())
				{
					SetAllFinish();
					return false;
				}
			}
		}
	}
	return true;
}

bool	CReligionSBY::IsBookingPos_base1(uint16 pos)
{
	std::map<T_FAMILYID, CJoinBooking>::iterator it;
	for (it = m_JoinBookingUser.begin(); it != m_JoinBookingUser.end(); it++)
	{
		if (it->second.m_BookingPos_base1 == pos)
			return true;
	}
	return false;
}

bool	CReligionSBY::ProcSBYBooking(T_FAMILYID FID, uint16 *_pPosBuffer)
{
	std::map<T_FAMILYID, CJoinBooking>::iterator it = m_JoinBookingUser.find(FID);
	if (it != m_JoinBookingUser.end())
	{
		uint16 pos = it->second.m_BookingPos_base1;
		m_JoinUser.insert(std::make_pair(pos, FID));
		m_JoinBookingUser.erase(FID);

		uint16 realPos = pos - 1;
		if (_pPosBuffer)
			*_pPosBuffer = realPos;
		if (m_BYListener)
		{
			m_BYListener->SBYNewJoin(this, FID, realPos);
		}
		// 경험값증가(하나의 행사에 대하여 한번만 증가한다)
		if (m_JoinHistory.find(FID) == m_JoinHistory.end())
		{
			ChangeFamilyExp(Family_Exp_Type_SBY, FID);
			m_JoinHistory[FID] = FID;
		}
		return true;
	}
	return false;
}

void	CReligionSBY::RemoveBooking(T_FAMILYID FID)
{
	m_JoinBookingUser.erase(FID);
}

int		CReligionSBY::GetCandiJoinPos_base0()
{
	//sipdebug("GetCandiJoinPos_base0: MaxNumUser=%d", m_BaseData.m_MaxNumUser); // by krs
	if (m_JoinUser.size() >= m_BaseData.m_MaxNumUser)
	{
		//sipdebug("GetCandiJoinPos_base0(m_JoinUser.size() >= m_BaseData.m_MaxNumUser): JoinUserSize=%d, MaxNumUser=%d", m_JoinUser.size(), m_BaseData.m_MaxNumUser); // by krs
		return -1;
	}
	for (uint16 pos = 1; pos <= m_BaseData.m_MaxNumUser; pos++)
	{
		if (IsBookingPos_base1(pos))
			continue;

		map<uint16, T_FAMILYID>::iterator it = m_JoinUser.find(pos);
		if (it == m_JoinUser.end())
			return (pos-1);
	}
	return -1;
}

bool	CReligionSBY::IsAlreadyJoin(T_FAMILYID _fid, uint16* pos)
{
	map<uint16, T_FAMILYID>::iterator it;
	for (it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
	{
		if (_fid == it->second)
		{
			*pos = it->first;
			return true;
		}
	}
	return false;
}

ENUMERR	CReligionSBY::RequestJoin(T_FAMILYID _fid, uint16* _pPosBuffer)
{
	if (!IsDoing())
	{
		return E_SBY_CANTJOIN;
	}
	// 이미 예약되였는가
	uint16 bookingPos = 0;
	if (ProcSBYBooking(_fid, &bookingPos))
	{
		*_pPosBuffer = bookingPos;
		return ERR_NOERR;
	}
	uint16 tempPos = 0;
	if (IsAlreadyJoin(_fid, &tempPos))
	{
		*_pPosBuffer = tempPos;
		return ERR_NOERR;
	}

	int nCandiPos_base0 = GetCandiJoinPos_base0();
	if (nCandiPos_base0 == -1)
		return E_SBY_USERFULL;

	// 요청처리
	uint16 realPos = (uint16)nCandiPos_base0;
	uint16 pos = realPos+1;
	*_pPosBuffer = realPos;

	m_JoinUser.insert(std::make_pair(pos, _fid));
	if (m_BYListener)
	{
		m_BYListener->SBYNewJoin(this, _fid, realPos);
	}
	// 경험값증가(하나의 행사에 대하여 한번만 증가한다)
	if (m_JoinHistory.find(_fid) == m_JoinHistory.end())
	{
		ChangeFamilyExp(Family_Exp_Type_SBY, _fid);
		m_JoinHistory[_fid] = _fid;
	}

	return ERR_NOERR;
}

void	CReligionSBY::ExitJoin(T_FAMILYID _fid)
{
	for (uint16 pos = 1; pos <= m_BaseData.m_MaxNumUser; pos++)
	{
		map<uint16, T_FAMILYID>::iterator it = m_JoinUser.find(pos);
		if (it != m_JoinUser.end())
		{
			T_FAMILYID jid = it->second;
			if (jid == _fid)
			{
				m_JoinUser.erase(pos);
				m_OverUser.erase(_fid);
				uint16 realPos = pos-1;
				if (m_BYListener)
				{
					m_BYListener->SBYExitJoin(this, _fid, realPos);
				}
				return;
			}
		}
	}
}

//bool	CReligionSBY::IsAllFinished()
//{
//	if (m_CurrentStep >= BUDDISMYISHI_FINISH)
//	{
//		bool bAllFinish = true;
//		map<uint16, T_FAMILYID>::iterator it;
//		for (it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
//		{
//			T_FAMILYID	fid = it->second;
//			if (m_OverUser.find(fid) == m_OverUser.end())
//			{
//				bAllFinish = false;
//				break;
//			}
//		}
//		return bAllFinish;
//	}
//	return false;
//}
bool	CReligionSBY::IsAllFinished()
{
	if (m_CurrentStep >= BUDDISMYISHI_FINISH)
	{
		if (m_JoinUser.size() <= m_OverUser.size())
			return true;
	}
	return false;
}

bool	CReligionSBY::IsDoing()
{
	if (m_CurrentStep < BUDDISMYISHI_READYSTEP)
		return false;
	return !IsAllFinished();
}

void	CReligionSBY::SetAllFinish()
{
	if (m_BYListener)
	{
		m_BYListener->SBYOver(this);
	}
	m_CurrentStep = BUDDISMYISHI_ALLFINISH;
}

void	CReligionSBY::SendCurrentState(T_FAMILYID _fid, SIPNET::TServiceId _tsid)
{
	if (!IsDoing())
		return;

	CReligionSBYBaseData& baseData = m_BaseData;
	uint16 curJoinNum = m_JoinUser.size();
	CMessage mes(M_SC_SBY_CURSTATE);
	mes.serial(_fid);
	mes.serial(m_SBYID, baseData.m_BackMusicID, baseData.m_ActItemNum);
	for (uint8 i = 0; i < baseData.m_ActItemNum; i++)
	{
		mes.serial(baseData.m_ActItem[i]);
	}
	uint16 deltaTime = uint16((uint32)GetDBTimeSecond() - baseData.GetReadyDBTime());
	mes.serial(deltaTime);
	mes.serial(curJoinNum);
	map<uint16, T_FAMILYID>::iterator it;
	for (it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
	{
		uint16		pos = it->first-1;
		T_FAMILYID	fid = it->second;
		mes.serial(fid, pos);
	}
	CUnifiedNetwork::getInstance()->send(_tsid, mes);
}

void	CReligionSBY::SendPrize()
{
	if (m_bSendPrize)
		return;
	map<uint16, T_FAMILYID>::iterator it;
	for (it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
	{
		T_FAMILYID	fid = it->second;
		CheckPrize(fid, PRIZE_TYPE_BUDDHISM_BIGYISHI, 0, true);
	}
	m_bSendPrize = true;
}

void CReligionSBY::OverOneUser(T_FAMILYID FID)
{
	m_CurrentStep = BUDDISMYISHI_FINISH;
	SendPrize();

	// 현재 행사진행중인 사용자라면 m_OverUser에 추가한다.
	for (map<uint16, T_FAMILYID>::iterator it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
	{
		if (FID == it->second)
		{
			if (m_FirstFinishLocalTime == 0)
				m_FirstFinishLocalTime = CTime::getLocalTime();
			else
				m_FirstFinishLocalTime = (m_FirstFinishLocalTime * m_OverUser.size() + CTime::getLocalTime()) / (m_OverUser.size() + 1);

			m_OverUser.insert(std::make_pair(FID, FID));
			break;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
bool	CReligionPBY::Update()
{
	if (m_CurrentStep == BUDDISMYISHI_ALLFINISH)
		return false;

	if (m_CurrentStep == BUDDISMYISHI_FINISH)
	{
		if (IsAllFinished())
		{
			SetAllFinish();
			return false;
		}
		if (m_FirstFinishLocalTime > 0) // by krs
		{
			TTime deltatime = CTime::getLocalTime() - m_FirstFinishLocalTime;
			if (deltatime >= 180000) // 3분
			{
				SetAllFinish();
				return false;
			}
		}
	}
	return true;
}

void	CReligionPBY::SendCurrentState(T_FAMILYID _fid, SIPNET::TServiceId _tsid)
{
	CReligionPBYBaseData& baseData = m_BaseData;
	uint16 curJoinNum = m_JoinUser.size();
	CMessage mes(M_SC_PBY_CURSTATE);
	mes.serial(_fid);
	mes.serial(m_PBYID, baseData.m_ActItemNum);
	for (uint8 i = 0; i < baseData.m_ActItemNum; i++)
	{
		mes.serial(baseData.m_ActItem[i]);
	}
	uint16	curStep = (uint16)m_CurrentStep;
	uint16 deltaTime = uint16((uint32)GetDBTimeSecond() - m_CurrentStepStartTime);
	mes.serial(curStep, deltaTime);
	mes.serial(curJoinNum);
	map<uint16, CPBYJoinData>::iterator it;
	for (it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
	{
		uint16		pos = it->first-1;
		T_FAMILYID	fid = it->second.m_FID;
		mes.serial(fid, pos);
	}
	CUnifiedNetwork::getInstance()->send(_tsid, mes);
}

bool	CReligionPBY::IsAlreadyJoin(T_FAMILYID _fid, uint16* pos)
{
	map<uint16, CPBYJoinData>::iterator it;
	for (it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
	{
		if (_fid == it->second.m_FID)
		{
			*pos = it->first;
			return true;
		}
	}
	return false;
}

ENUMERR	CReligionPBY::RequestJoin(T_FAMILYID _fid, uint16* _pPosBuffer, bool bOpenFlag, ucstring& ActText, uint8 ItemNum, T_ITEMSUBTYPEID* Items)
{
	if (m_JoinUser.size() >= m_BaseData.m_MaxNumUser)
	{
		return E_PBY_CANTJOIN;
	}
	uint16 tempPos = 0;
	if (IsAlreadyJoin(_fid, &tempPos))
	{
		*_pPosBuffer = tempPos;
		return ERR_NOERR;
	}

	for (uint16 pos = 1; pos <= m_BaseData.m_MaxNumUser; pos++)
	{
		map<uint16, CPBYJoinData>::iterator it = m_JoinUser.find(pos);
		if (it == m_JoinUser.end())
		{
			CPBYJoinData newJoin;
			newJoin.m_FID = _fid;
			newJoin.m_bOpenFlag = bOpenFlag;
			newJoin.m_ActText = ActText;
			if (ItemNum > 0)
				newJoin.m_Item = Items[0];
			newJoin.m_JoinTime = (uint32)GetDBTimeSecond();

			m_JoinUser.insert(std::make_pair(pos, newJoin));
			uint16 realPos = pos-1;
			*_pPosBuffer = realPos;
			if (m_BYListener)
			{
				m_BYListener->PBYNewJoin(this, _fid, realPos);
			}
			// 경험값증가(하나의 행사에 대하여 한번만 증가한다)
			if (m_JoinHistory.find(_fid) == m_JoinHistory.end())
			{
				ChangeFamilyExp(Family_Exp_Type_ReligionYiSiSlaver, _fid);
				m_JoinHistory[_fid] = _fid;
			}

			return ERR_NOERR;
		}
	}

	return E_PBY_CANTJOIN;
}

void	CReligionPBY::ExitJoin(T_FAMILYID _fid)
{
	for (uint16 pos = 1; pos <= m_BaseData.m_MaxNumUser; pos++)
	{
		map<uint16, CPBYJoinData>::iterator it = m_JoinUser.find(pos);
		if (it != m_JoinUser.end())
		{
			T_FAMILYID jid = it->second.m_FID;
			if (jid == _fid)
			{
				m_JoinUser.erase(pos);
				uint16 realPos = pos-1;
				if (m_BYListener)
				{
					m_BYListener->PBYExitJoin(this, _fid, realPos);
				}
				return;
			}
		}
	}
}

void	CReligionPBY::SuccessOverJoin(T_FAMILYID FID)
{
	map<uint16, CPBYJoinData>::iterator it;
	for (it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
	{
		uint16		pos = it->first-1;
		T_FAMILYID	fid = it->second.m_FID;
		if (fid == FID)
		{
			m_OverUser.insert(std::make_pair(FID, it->second));
			m_JoinUser.erase(it);
			if (m_BYListener)
			{
				m_BYListener->PBYExitJoin(this, FID, pos);
			}
			CheckPrize(FID, PRIZE_TYPE_BUDDHISM_BIGYISHI, 0, true);
			return;
		}
	}
}

bool	CReligionPBY::ProcCancel(T_FAMILYID _fid)
{
	if (m_BaseData.m_MasterID != _fid)
		return false;
	if (m_CurrentStep >= BUDDISMYISHI_FINISH)
		return false;

	if (m_BYListener)
	{
		m_BYListener->PBYCancel(this);
	}
	m_CurrentStep = BUDDISMYISHI_ALLFINISH;
	return true;
}

void	CReligionPBY::ChangeStep(T_FAMILYID _fid, uint16 ActStep)
{
	if (m_BaseData.m_MasterID != _fid)
		return;
	if (m_CurrentStep >= ActStep)
		return;

	m_CurrentStep = ActStep;
	m_CurrentStepStartTime = (uint32)GetDBTimeSecond();

	if (m_BYListener)
	{
		m_BYListener->PBYChangeStep(this, ActStep);
	}
	if (ActStep == BUDDISMYISHI_FINISH)
	{
		CheckPrize(_fid, PRIZE_TYPE_BUDDHISM_BIGYISHI, 0, true);
		if (m_FirstFinishLocalTime == 0) // by krs
			m_FirstFinishLocalTime = CTime::getLocalTime();
	}
}

bool	CReligionPBY::IsAllFinished()
{
	if (m_CurrentStep >= BUDDISMYISHI_FINISH)
	{
		bool bAllFinish = true;
		map<uint16, CPBYJoinData>::iterator it;
		for (it = m_JoinUser.begin(); it != m_JoinUser.end(); it++)
		{
			T_FAMILYID	fid = it->second.m_FID;
			if (m_OverUser.find(fid) == m_OverUser.end())
			{
				bAllFinish = false;
				break;
			}
		}
		return bAllFinish;
	}
	return false;
}

void	CReligionPBY::SetAllFinish()
{
	if (m_BYListener)
	{
		m_BYListener->PBYOver(this);
	}
	m_CurrentStep = BUDDISMYISHI_ALLFINISH;
	SavePBYHis();
}

void	CReligionPBY::SavePBYHis()
{
	uint8	itembuf[MAX_NUM_BYITEM * sizeof(uint32) + 1];
	itembuf[0] = 0;
	uint32	*itemlist = (uint32*)(&itembuf[1]);
	for (uint8 i = 0; i < m_BaseData.m_ActItemNum; i++)
	{
		*itemlist = m_BaseData.m_ActItem[i];
		itembuf[0] ++;
		itemlist ++;
	}
	uint32 MainID = 0;
	MAKEQUERY_Religion_PBYMain_Insert(strQ, m_BaseData.m_MasterID, m_BaseData.m_ActTarget.c_str(), m_BaseData.m_ActProperty.c_str(), m_BaseData.m_bActTextOpenFlag, m_BaseData.m_ActText.c_str(), m_BaseData.m_BeginTimeS);
	CDBConnectionRest	DB(DBCaller);
	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
	if (!nPrepareRet)
	{
		sipwarning(L"DB_EXE_PREPARESTMT failed");
		return;
	}
	SQLLEN	len1 = itembuf[0] * sizeof(uint32) + 1;
	DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, sizeof(itembuf), 0, itembuf, 0, &len1);
	if (!nBindResult)
	{
		sipwarning(L"DB_EXE_BINDPARAM_IN failed");
		return;
	}
	DB_EXE_PARAMQUERY(Stmt, strQ);
	if (nQueryResult == true)
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( IS_DB_ROW_VALID(row) )
		{
			COLUMN_DIGIT(row, 0, uint32, ai);
			MainID = ai;
		}
	}
	if (MainID > 0)
	{
		std::map<T_FAMILYID, CPBYJoinData>::iterator it;
		for (it = m_OverUser.begin(); it != m_OverUser.end(); it++)
		{
			CPBYJoinData& joinData = it->second;

			MAKEQUERY_Religion_PBYJoin_Insert(strQ, MainID, joinData.m_FID, joinData.m_bOpenFlag, joinData.m_ActText.c_str(), joinData.m_JoinTime, joinData.m_Item);
			CDBConnectionRest	DB(DBCaller);
			DB_EXE_QUERY(DB, Stmt, strQ);
		}
	}
	ResetPBYHisBuffer();
	ResetPBYMyHisBuffer(m_BaseData.m_MasterID);

}

void	cb_M_CS_PBY_HISLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			Page, PageSize;

	_msgin.serial(FID, Page, PageSize);

	CReligionWorld* pWorld = GetReligionWorldFromFID(FID);
	if(pWorld == NULL)
	{
		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	pWorld->RequestHisPBYPageInfo(FID, Page, PageSize, _tsid);
}

void	cb_M_CS_PBY_MYHISLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			Page, PageSize;

	_msgin.serial(FID, Page, PageSize);

	CReligionWorld* pWorld = GetReligionWorldFromFID(FID);
	if(pWorld == NULL)
	{
		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	pWorld->RequestHisMyPBYPageInfo(FID, Page, PageSize, _tsid);
}

void	cb_M_CS_PBY_HISINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			HisMainID;

	_msgin.serial(FID, HisMainID);

	CReligionWorld* pWorld = GetReligionWorldFromFID(FID);
	if(pWorld == NULL)
	{
		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	pWorld->RequestHisPBYInfo(FID, HisMainID, _tsid);
}

void	cb_M_CS_PBY_SUBHISINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			HisSubID;

	_msgin.serial(FID, HisSubID);

	CReligionWorld* pWorld = GetReligionWorldFromFID(FID);
	if(pWorld == NULL)
	{
		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	pWorld->RequestHisPBYSubInfo(FID, HisSubID, _tsid);
}

void	cb_M_CS_PBY_HISMODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;
	uint32				HisMainID;
	bool				bOpenProperty;
	T_COMMON_CONTENT	ActText;
	_msgin.serial(FID, HisMainID, bOpenProperty);
	NULLSAFE_SERIAL_WSTR(_msgin, ActText, MAX_LEN_BUDDISMYISHICONTENT, FID);

	CReligionWorld* pWorld = GetReligionWorldFromFID(FID);
	if(pWorld == NULL)
	{
		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	pWorld->PBYHisModify(FID, HisMainID, bOpenProperty, ActText);
}

void	cb_M_CS_PBY_SUBHISMODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;
	uint32				HisSubID;
	bool				bOpenProperty;
	T_COMMON_CONTENT	ActText;
	_msgin.serial(FID, HisSubID, bOpenProperty);
	NULLSAFE_SERIAL_WSTR(_msgin, ActText, MAX_LEN_BUDDISMYISHICONTENT, FID);

	CReligionWorld* pWorld = GetReligionWorldFromFID(FID);
	if(pWorld == NULL)
	{
		sipwarning("GetReligionWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	pWorld->PBYSubHisModify(FID, HisSubID, bOpenProperty, ActText);
}

static void	DBCB_DBGetReligionPBYList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID			= *(uint32 *)argv[0];
	uint32			Page		= *(uint32 *)argv[1];
	TServiceId		tsid(*(uint16 *)argv[2]);
	T_ROOMID		RoomID		= *(T_ROOMID*)(argv[3]);
	if (!nQueryResult)
		return;

	CReligionWorld* pWorld		= GetReligionWorld(RoomID);
	if (pWorld == NULL)
	{
		sipwarning("GetReligionWorld = NULL. RoomID=%d", RoomID);
		return;
	}
	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipdebug(L"DBCB_DBGetReligionPBYList failed(Can't get allcount).");
		return;
	}
	uint32 allcount;			resSet->serial(allcount);

	CMessage	msgOut(M_SC_PBY_HISLIST);
	msgOut.serial(FID, allcount, Page);
	{
		PBYHisMainPageInfo newPageInfo;
		newPageInfo.bValid = true;
		newPageInfo.HisAllCount = allcount;

		uint32	nRows2;	resSet->serial(nRows2);
		
		for(uint32 i = 0; i < nRows2; i ++)
		{
			uint32			HisID;			resSet->serial(HisID);
			T_FAMILYID		fid;			resSet->serial(fid);
			T_FAMILYNAME	FName;			resSet->serial(FName);
			T_S1970			BeginTime;		resSet->serial(BeginTime);
			//byte			OpenProperty;	resSet->serial(OpenProperty);
			
			msgOut.serial(HisID, fid, FName, BeginTime);			

			AddFamilyNameData(fid, FName);
			newPageInfo.HisList.push_back(PBYHisMainInfo(HisID, fid, FName, BeginTime));
		}

		uint32 mapIndex = Page+1;	// For page is started from 0
		TPBYHisMainPageInfo::iterator itPage = pWorld->m_HisPBYPageInfo.find(mapIndex);
		if (itPage != pWorld->m_HisPBYPageInfo.end())
		{
			pWorld->m_HisPBYPageInfo[mapIndex] = newPageInfo;
		}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

void	CReligionWorld::RequestHisPBYPageInfo(T_FAMILYID FID, uint32 Page, uint32 PageSize, SIPNET::TServiceId _tsid)
{
	uint32 mapIndex = Page+1;	// For page is started from 0
	TPBYHisMainPageInfo::iterator itPage = m_HisPBYPageInfo.find(mapIndex);
	if (itPage != m_HisPBYPageInfo.end())
	{
		PBYHisMainPageInfo& pageInfo = itPage->second;
		if (pageInfo.bValid)
		{
			/* PageSize때문에 이 방법을 쓸수 없다 연구 필요
			CMessage	msgOut(M_SC_PBY_HISLIST);
			msgOut.serial(FID, pageInfo.HisAllCount, Page);
			std::list<PBYHisMainInfo>::iterator lstit;
			for (lstit = pageInfo.HisList.begin(); lstit != pageInfo.HisList.end(); lstit++)
			{
				PBYHisMainInfo& HisInfo = *lstit;
				uint32			HisMainID = HisInfo.HisMainID;
				T_FAMILYID		HisFamilyID = HisInfo.FamilyID;
				T_FAMILYNAME	HisFName = GetFamilyNameFromBuffer(HisFamilyID);
				T_S1970			HisPBYTime = HisInfo.PBYTime;

				msgOut.serial(HisMainID, HisFamilyID, HisFName, HisPBYTime);
			}
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
			return;*/
		}
	}
	else
	{
		PBYHisMainPageInfo newPageInfo;
		m_HisPBYPageInfo[mapIndex] = newPageInfo;
	}

	MAKEQUERY_GetReligionPBYHisList(strQuery, Page, PageSize);	// PageSize = 10 : 수정필요
	DBCaller->executeWithParam(strQuery, DBCB_DBGetReligionPBYList,
		"DDWD",
		CB_,		FID,
		CB_,		Page,
		CB_,		_tsid.get(),
		CB_,		m_RoomID);
}

static void	DBCB_DBGetReligionMyPBYList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID			= *(uint32 *)argv[0];
	uint32			Page		= *(uint32 *)argv[1];
	TServiceId		tsid(*(uint16 *)argv[2]);
	T_ROOMID		RoomID		= *(T_ROOMID*)(argv[3]);
	if (!nQueryResult)
		return;

	CReligionWorld* pWorld		= GetReligionWorld(RoomID);
	if (pWorld == NULL)
	{
		sipwarning("GetReligionWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipdebug(L"DBCB_DBGetReligionMyPBYList failed(Can't get allcount).");
		return;
	}
	uint32 allcount;			resSet->serial(allcount);

	CMessage	msgOut(M_SC_PBY_MYHISLIST);
	msgOut.serial(FID, allcount, Page);
	{
		PBYHisMainPageInfo newPageInfo;
		newPageInfo.bValid = true;
		newPageInfo.HisAllCount = allcount;

		uint32	nRows2;	resSet->serial(nRows2);
		for(uint32 i = 0; i < nRows2; i ++)
		{
			uint32			HisID;			resSet->serial(HisID);
			T_S1970			BeginTime;		resSet->serial(BeginTime);
			//byte			OpenProperty;	resSet->serial(OpenProperty); by krs

			msgOut.serial(HisID, BeginTime);
			newPageInfo.HisList.push_back(PBYHisMainInfo(HisID, FID, L"", BeginTime));
		}

		std::map<uint32, TPBYHisMainPageInfo>::iterator it = pWorld->m_HisMyPBYPageInfo.find(FID);
		if (it != pWorld->m_HisMyPBYPageInfo.end())
		{
			TPBYHisMainPageInfo& HisPBYPageInfo = it->second;
			uint32 mapIndex = Page+1;	// For page is started from 0
			TPBYHisMainPageInfo::iterator itPage = HisPBYPageInfo.find(mapIndex);
			if (itPage != HisPBYPageInfo.end())
			{
				HisPBYPageInfo[mapIndex] = newPageInfo;
			}
		}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

void	CReligionWorld::RequestHisMyPBYPageInfo(T_FAMILYID FID, uint32 Page, uint32 PageSize, SIPNET::TServiceId _tsid)
{
	TPBYHisMainPageInfo* HisPBYPageInfo = NULL;
	std::map<uint32, TPBYHisMainPageInfo>::iterator it = m_HisMyPBYPageInfo.find(FID);
	if (it != m_HisMyPBYPageInfo.end())
	{
		HisPBYPageInfo = &(it->second);
	}
	else
	{
		TPBYHisMainPageInfo newData;
		m_HisMyPBYPageInfo.insert(std::make_pair(FID, newData));
		it = m_HisMyPBYPageInfo.find(FID);
		HisPBYPageInfo = &(it->second);
	}

	uint32 mapIndex = Page+1;	// For page is started from 0
	TPBYHisMainPageInfo::iterator itPage = HisPBYPageInfo->find(mapIndex);
	if (itPage != HisPBYPageInfo->end())
	{
		PBYHisMainPageInfo& pageInfo = itPage->second;
		if (pageInfo.bValid)
		{
			/* PageSize가 가변이므로 아래의 부분은 쓸수 없다. 갱신 필요
			CMessage	msgOut(M_SC_PBY_MYHISLIST);
			msgOut.serial(FID, pageInfo.HisAllCount, Page);
			std::list<PBYHisMainInfo>::iterator lstit;
			for (lstit = pageInfo.HisList.begin(); lstit != pageInfo.HisList.end(); lstit++)
			{
				PBYHisMainInfo& HisInfo = *lstit;
				uint32			HisMainID = HisInfo.HisMainID;
				T_S1970			HisPBYTime = HisInfo.PBYTime;

				msgOut.serial(HisMainID, HisPBYTime);
			}
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
			return;*/
		}
	}
	else
	{
		PBYHisMainPageInfo newPageInfo;
		HisPBYPageInfo->insert(std::make_pair(mapIndex, newPageInfo));
	}

	MAKEQUERY_GetReligionMyPBYHisList(strQuery, FID, Page, PageSize);
	DBCaller->executeWithParam(strQuery, DBCB_DBGetReligionMyPBYList,
		"DDWD",
		CB_,		FID,
		CB_,		Page,
		CB_,		_tsid.get(),
		CB_,		m_RoomID);
}

static void	DBCB_DBGetReligionPBYInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID			= *(uint32 *)argv[0];
	uint32			HisMainID	= *(uint32 *)argv[1];
	TServiceId		tsid(*(uint16 *)argv[2]);
	T_ROOMID		RoomID		= *(T_ROOMID*)(argv[3]);
	if (!nQueryResult)
		return;
	CReligionWorld* pWorld		= GetReligionWorld(RoomID);
	if (pWorld == NULL)
	{
		sipwarning("GetReligionWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	uint32 nRow;	resSet->serial(nRow);
	CReligionPBYBaseData baseData;
	resSet->serial(baseData.m_MasterID, baseData.m_MasterName, baseData.m_ActTarget, baseData.m_ActProperty, baseData.m_bActTextOpenFlag, baseData.m_ActText, baseData.m_BeginTimeS);
	AddFamilyNameData(baseData.m_MasterID, baseData.m_MasterName);
	uint8	byItemBuf[MAX_NUM_BYITEM * sizeof(uint32) + 1];
	memset(byItemBuf, 0, sizeof(byItemBuf));
	uint32		len=0;			
	resSet->serialBufferWithSize(byItemBuf, len);
	baseData.m_ActItemNum = byItemBuf[0];
	if (baseData.m_ActItemNum > 0)
	{
		uint32* pItemID = (uint32*)(&byItemBuf[1]);
		for (int i = 0; i < baseData.m_ActItemNum; i++)
		{
			baseData.m_ActItem[i] = *pItemID;
			pItemID++;
		}
	}

	CReligionPBY newPBY(HisMainID, &baseData, NULL);
	uint32	nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		CPBYJoinData		joinData;
		resSet->serial(joinData.m_HisSubID);
		resSet->serial(joinData.m_FID);
		resSet->serial(joinData.m_FName);
		resSet->serial(joinData.m_bOpenFlag);
		resSet->serial(joinData.m_ActText);
		resSet->serial(joinData.m_Item);
		resSet->serial(joinData.m_JoinTime);
		newPBY.m_JoinUser.insert(std::make_pair((uint16)(i+1), joinData));
		pWorld->m_HisPBYJoinInfo.insert(std::make_pair(joinData.m_HisSubID, joinData));
	}
	pWorld->m_HisPBY.insert(std::make_pair(HisMainID, newPBY));
	pWorld->RequestHisPBYInfo(FID, HisMainID, tsid);
}

void	CReligionWorld::RequestHisPBYInfo(T_FAMILYID FID, uint32 HisMainID, SIPNET::TServiceId _tsid)
{
	std::map<uint32, CReligionPBY>::iterator it = m_HisPBY.find(HisMainID);
	if (it == m_HisPBY.end())
	{
		MAKEQUERY_GetReligionPBYInfo(strQuery, HisMainID);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetReligionPBYInfo,
			"DDWD",
			CB_,		FID,
			CB_,		HisMainID,
			CB_,		_tsid.get(),
			CB_,		m_RoomID);
		return;
	}
	CReligionPBY& PbyInfo = it->second;
	CMessage msgOut(M_SC_PBY_HISINFO);
	msgOut.serial(FID, HisMainID);
	msgOut.serial(PbyInfo.m_BaseData.m_ActTarget, PbyInfo.m_BaseData.m_ActProperty, PbyInfo.m_BaseData.m_bActTextOpenFlag, PbyInfo.m_BaseData.m_ActText);
	msgOut.serial(PbyInfo.m_BaseData.m_ActItemNum);
	for (uint8 i = 0; i < PbyInfo.m_BaseData.m_ActItemNum; i++)
	{
		msgOut.serial(PbyInfo.m_BaseData.m_ActItem[i]);
	}
	uint32	UserNum = PbyInfo.m_JoinUser.size();
	msgOut.serial(UserNum);
	for (std::map<uint16, CPBYJoinData>::iterator itJoin = PbyInfo.m_JoinUser.begin(); itJoin !=  PbyInfo.m_JoinUser.end(); itJoin++)
	{
		CPBYJoinData& join = itJoin->second;
		uint32	HisSubID = join.m_HisSubID;
		std::map<uint32, CPBYJoinData>::iterator itHisJoin = m_HisPBYJoinInfo.find(HisSubID);
		if (itHisJoin != m_HisPBYJoinInfo.end())
		{
			CPBYJoinData& Hisjoin = itHisJoin->second;
			msgOut.serial(HisSubID, Hisjoin.m_FID, Hisjoin.m_FName, Hisjoin.m_JoinTime);
		}
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

void	CReligionWorld::RequestHisPBYSubInfo(T_FAMILYID FID, uint32 HisSubID, SIPNET::TServiceId _tsid)
{
	std::map<uint32, CPBYJoinData>::iterator it = m_HisPBYJoinInfo.find(HisSubID);
	if (it == m_HisPBYJoinInfo.end())
	{
		sipwarning("There is no PBYJoinData:HisSubID=%d", HisSubID);
		return;
	}
	CPBYJoinData& joinData = it->second;
	uint8 ItemNum = 1;
	CMessage msgOut(M_SC_PBY_SUBHISINFO);
	msgOut.serial(FID, HisSubID);
	msgOut.serial(joinData.m_bOpenFlag, joinData.m_ActText, ItemNum, joinData.m_Item);
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

void	CReligionWorld::PBYHisModify(T_FAMILYID FID, uint32 HisMainID, bool bOpenProperty, T_COMMON_CONTENT ActText)
{
	std::map<uint32, CReligionPBY>::iterator it = m_HisPBY.find(HisMainID);
	if (it != m_HisPBY.end())
	{
		CReligionPBY& PBYInfo = it->second;
		if (PBYInfo.m_BaseData.m_MasterID == FID)
		{
			PBYInfo.m_BaseData.m_bActTextOpenFlag = bOpenProperty;
			PBYInfo.m_BaseData.m_ActText = ActText;
		}
		else
		{
			sipwarning("No PBY master:ID=%d, FID=%d", HisMainID, FID);
			return;
		}
	}
	MAKEQUERY_Religion_PBYMain_Modify(strQuery, HisMainID, bOpenProperty, ActText.c_str());
	DBCaller->execute(strQuery);
}

void	CReligionWorld::PBYSubHisModify(T_FAMILYID FID, uint32 HisSubID, bool bOpenProperty, T_COMMON_CONTENT ActText)
{
	std::map<uint32, CPBYJoinData>::iterator it = m_HisPBYJoinInfo.find(HisSubID);
	if (it != m_HisPBYJoinInfo.end())
	{
		CPBYJoinData& JoinInfo = it->second;
		if (JoinInfo.m_FID == FID)
		{
			JoinInfo.m_bOpenFlag = bOpenProperty;
			JoinInfo.m_ActText = ActText;
		}
		else
		{
			sipwarning("No PBYJoin master:ID=%d, FID=%d", HisSubID, FID);
			return;
		}
	}
	MAKEQUERY_Religion_PBYJoin_Modify(strQuery, HisSubID, bOpenProperty, ActText.c_str());
	DBCaller->execute(strQuery);
}

static void	ResetPBYHisBuffer()
{
	CReligionWorld *roomManager = GetReligionWorld(ROOM_ID_BUDDHISM);
	if (roomManager)
	{
		roomManager->m_HisPBYPageInfo.clear();
	}
}

static void	ResetPBYMyHisBuffer(T_FAMILYID FID)
{
	CReligionWorld *roomManager = GetReligionWorld(ROOM_ID_BUDDHISM);
	if (roomManager)
	{
		std::map<uint32, TPBYHisMainPageInfo>::iterator it = roomManager->m_HisMyPBYPageInfo.find(FID);
		if (it != roomManager->m_HisMyPBYPageInfo.end())
		{
			roomManager->m_HisMyPBYPageInfo.erase(it);
		}
	}	
}

//void CReligionWorld::SetRoomEvent()
//{
//	SetRoomEventStatus(ROOMEVENT_STATUS_READY);
//
//	uint32	curDay = (uint32)(GetDBTimeSecond() / SECOND_PERONEDAY);
//	uint32	tm = (SECOND_PERONEDAY - 1200) / MAX_ROOMEVENT_COUNT;	// 0:10~23:50사이에 나오게 한다.
//
//	for (int i = 0; i < MAX_ROOMEVENT_COUNT; i ++)
//	{
//		uint8 ty = (uint8)(Irand(0, 2));
//		switch (ty)
//		{
//		case 0:		ty = ROOM_EVENT_FENGHUANG; break;
//		case 1:		ty = ROOM_EVENT_RAIN; break;
//		case 2:		ty = ROOM_EVENT_BLESSING; break;
//		}
//		uint8 po = (int)(rand() * 256 / RAND_MAX);
//		uint32 st = (int)(rand() * (tm - ROOM_EVENT_MAX_TIME) / RAND_MAX + tm * i) + 600;
//		st += (curDay * SECOND_PERONEDAY);
//		m_RoomEvent[i].SetInfo(ty, po, st);
//
//		ucstring timestr = GetTimeStrFromS1970(st);
//		sipdebug(L"Set room event : RoomID=%d, EventType=%d, EventPos=%d, time=%s", m_RoomID, ty, po, timestr.c_str());
//	}
//}

#define CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(act1, act2, errco) \
	if (ActPos == act1 && m_Activitys[act2].ActivityDesk[0].FID != NOFAMILYID)	\
{	\
	uint32		ErrorCode = errco;	\
	uint32		RemainCount = 0;	\
	uint8		DeskNo = 0;	\
	CMessage	msgOut(M_SC_ACT_WAIT);	\
	msgOut.serial(FID, ActPos, ErrorCode, RemainCount, DeskNo);	\
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);	\
	return;	\
}	


void	ReligionChannelData::on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	if(ActPos >= m_ActivityCount)
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"ActPos >= m_ActivityCount: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	uint32	ChannelID = m_ChannelID;

	// 1명의 User는 하나의 활동 혹은 대기만 할수 있다.
	int		ret = GetActivityIndexOfUser(FID);
	if(ret >= 0)
	{
		sipwarning("GetActivityIndexOfUser(ChannelID, FID) >= 0. FID=%d, ret=%d", FID, ret);
		return;
	}

	CSharedActivity &rActivity = m_Activitys[ActPos];
	
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_1, ACTPOS_R0_XIANG_INDOOR_1, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_2, ACTPOS_R0_XIANG_INDOOR_2, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_3, ACTPOS_R0_XIANG_INDOOR_3, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_4, ACTPOS_R0_XIANG_INDOOR_4, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_5, ACTPOS_R0_XIANG_INDOOR_5, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_6, ACTPOS_R0_XIANG_INDOOR_6, E_ALREADY_FOJIAOTOUXIANG);

	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_YISHI_1, ACTPOS_R0_XIANG_INDOOR_1, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_YISHI_2, ACTPOS_R0_XIANG_INDOOR_2, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_YISHI_3, ACTPOS_R0_XIANG_INDOOR_3, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_YISHI_4, ACTPOS_R0_XIANG_INDOOR_4, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_YISHI_5, ACTPOS_R0_XIANG_INDOOR_5, E_ALREADY_FOJIAOTOUXIANG);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_YISHI_6, ACTPOS_R0_XIANG_INDOOR_6, E_ALREADY_FOJIAOTOUXIANG);

	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_1, ACTPOS_R0_YISHI_1, E_ALREADY_FOJIAOYISHI);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_2, ACTPOS_R0_YISHI_2, E_ALREADY_FOJIAOYISHI);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_3, ACTPOS_R0_YISHI_3, E_ALREADY_FOJIAOYISHI);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_4, ACTPOS_R0_YISHI_4, E_ALREADY_FOJIAOYISHI);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_5, ACTPOS_R0_YISHI_5, E_ALREADY_FOJIAOYISHI);
	CHECK_FOJIAO_YISHIANDTOUXIANG_DOUBLE(ACTPOS_R0_XIANG_INDOOR_6, ACTPOS_R0_YISHI_6, E_ALREADY_FOJIAOYISHI);

	uint32		ErrorCode = 0, RemainCount = -1;
	uint8		DeskNo;
	for(int j = 0; j < rActivity.ActivityDeskCount; j ++)
	{
		if(rActivity.ActivityDesk[j].FID == 0)
		{
			// 행사시작
			rActivity.ActivityDesk[j].init(FID, AT_SIMPLERITE);

			RemainCount = 0;
			DeskNo = (uint8)j;
			break;
		}
	}

	if(RemainCount == -1)
	{
		// 행사대기
		RemainCount = 1 + rActivity.WaitFIDs.size();
		_RoomActivity_Wait	data(FID, AT_SIMPLERITE,0,0);
		rActivity.WaitFIDs.push_back(data);
	}

	CMessage	msgOut(M_SC_ACT_WAIT);
	msgOut.serial(FID, ActPos, ErrorCode, RemainCount, DeskNo);
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, viewReligionBY, "View Religion BY", "")
{
	CReligionWorld *roomManager = GetReligionWorld(ROOM_ID_BUDDHISM);
	
	roomManager->LogReligionBY(log);
	return true;
}
