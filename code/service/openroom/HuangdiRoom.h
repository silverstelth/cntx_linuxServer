#ifndef __KONGZIHUANGDIROOM_H
#define __KONGZIHUANGDIROOM_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "misc/db_interface.h"
#include "misc/time_sip.h"
#include "misc/vector.h"
#include "misc/common.h"
#include "misc/time_sip.h"

#include "net/syspacket.h"

#include "../Common/QuerySetForShard.h"
#include "../../Common/Packet.h"
#include "../../Common/Macro.h"
#include "../Common/Common.h"
#include "../../Common/Common.h"
#include "../Common/DBData.h"
#include "SharedActWorld.h"
#include "openroom.h"
#include "mst_room.h"

class LargeActUser
{
public:	
	LargeActUser(T_FAMILYID fid, uint16 pos):FID(fid), ActPos(pos) {}
	LargeActUser() : FID(NOFAMILYID), ActPos(0xFFFF) {}
	T_FAMILYID	FID;
	uint16		ActPos;
};

class CKongziHuangdiRoomWorld;
class KongziHuangdiRoomChannelData : public CSharedActChannelData
{
public:
	// uint32			m_LargeActUserCountNew;
	std::list<LargeActUser>	m_LargeActUserList;
	uint32			m_nCurrentLargeActStep;

	KongziHuangdiRoomChannelData(CKongziHuangdiRoomWorld* pWorld, int actcount, T_ROOMID rid, T_COMMON_ID cid, T_MSTROOM_ID mid) : 
		CSharedActChannelData((CChannelWorld*)pWorld, actcount, rid, cid, mid)
	{
		m_nCurrentLargeActStep = 0;
		for (int j = 0; j < m_ActivityCount; j ++)
		{
			m_Activitys[j].init(DeskCountPerActivity_R[2][j]);
		}
	};
	virtual ~KongziHuangdiRoomChannelData()
	{
	};
	virtual bool IsValidMultiActPos(uint8 ActPos)
	{
		if (ActPos != ACTPOS_R2_YISHI_1)
			return false;
		return true;
	}
	uint32 EstimateChannelUserCount(uint32 maxCount);

	virtual void	on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
};
typedef std::map<uint32, KongziHuangdiRoomChannelData>	TmapKongziHuangdiRoomChannelData;

struct TLargeActPrizeItemInfo 
{
	T_ITEMSUBTYPEID	ItemID;
	uint16			ItemNum;
};

struct TLargeActVIP 
{
	TLargeActVIP() : FID(NOFAMILYID), bRequest(false) {}
	TLargeActVIP(T_FAMILYID fid) : FID(fid), bRequest(false) {}
	TLargeActVIP& operator =(const TLargeActVIP& other)
	{
		FID		= other.FID;
		FName	= other.FName;
		bRequest= other.bRequest;
		return *this;
	}
	T_FAMILYID		FID;
	T_FAMILYNAME	FName;
	bool			bRequest;
};
class LargeAct
{
public:
	uint32		ActID;
	ucstring	Title;
	ucstring	Comment;
	ucstring	FlagText;
	ucstring	ActMean;
	uint32		Option;
	uint32		AcceptTime;	// GetDBTimeMinute()
	uint32		StartTime;	// GetDBTimeMinute()
	uint32		ItemIDs[MAX_LARGEACT_ITEMCOUNT];
	uint32		PhotoIDs[MAX_LARGEACT_PHOTOCOUNT];
	uint8		VIPNum;
	TLargeActVIP	VIPS[MAX_LARGEACT_VIPCOUNT];
	uint32		UserCount;

	LARGEACTITEMPRIZE_MODE	PrizeItemMode;
	uint8			PrizeItemNum;
	TLargeActPrizeItemInfo	PrizeInfo[MAX_LARGEACTPRIZENUM];

	LARGEACTBLESSPRIZE_MODE	PrizeBlessMode;
	uint8			PrizeBless[MAX_BLESSCARD_ID];
	bool			bNotifyToVIP;
	T_COMMON_CONTENT	OpeningNotice;
	T_COMMON_CONTENT	EndingNotice;
	std::map<T_M1970, T_M1970>	OpeningNoticeHis;

	void Init()
	{
		ActID = 0;
		Title = L"";
		Comment = L"";
		Option = 0;
		AcceptTime = 0;
		StartTime = 0;
		for (int i = 0; i < MAX_LARGEACT_ITEMCOUNT; i ++)
			ItemIDs[i] = 0;
		for (int i = 0; i < MAX_LARGEACT_PHOTOCOUNT; i ++)
			PhotoIDs[i] = 0;
		UserCount = 0;
		PrizeItemMode = LARGEACTITEMPRIZE_WHOLE;
		PrizeBlessMode = LARGEACTBLESSPRIZE_WHOLE;
		PrizeItemNum = 0;
		memset(PrizeBless, 0, sizeof(PrizeBless));
		memset(VIPS, 0, sizeof(VIPS));
		VIPNum = 0;
		bNotifyToVIP = false;
		OpeningNoticeHis.clear();
	}
	bool	IsVIP(T_FAMILYID fid)
	{
		for (uint8 i = 0; i < VIPNum; i++)
		{
			if (VIPS[i].FID == fid)
				return true;
		}
		return false;
	}
	void SetVIPRequest(T_FAMILYID fid, bool bRequest)
	{
		for (uint8 i = 0; i < VIPNum; i++)
		{
			if (VIPS[i].FID == fid)
			{
				VIPS[i].bRequest = bRequest;
				return;
			}
		}
	}
	bool IsVIPRequest(T_FAMILYID fid)
	{
		for (uint8 i = 0; i < VIPNum; i++)
		{
			if (VIPS[i].FID == fid)
				return VIPS[i].bRequest;
		}
		return false;
	}
	void ResetHis()
	{
		OpeningNoticeHis.clear();
	}
};

// 방관리자
class CKongziHuangdiRoomWorld : public CSharedActWorld
{
public:
	CKongziHuangdiRoomWorld(T_ROOMID RRoomID) 
		: CSharedActWorld(RRoomID, WORLD_COMMON)
	{
		m_RoomID = RRoomID;
		m_RoomName = (RRoomID == ROOM_ID_KONGZI) ? L"KongziRoom" : L"HuangdiRoom";
		m_nDefaultChannelCount = RROOM_DEFAULT_CHANNEL_COUNT;
		m_nLargeActCount = 0;
		m_nCurrentLargeActIndex = -1;
	}
	virtual ~CKongziHuangdiRoomWorld() {}

	TmapKongziHuangdiRoomChannelData	m_mapChannels;				// 선로ID목록

	KongziHuangdiRoomChannelData*		GetChannel(uint32 ChannelID)
	{
		TmapKongziHuangdiRoomChannelData::iterator	itChannel = m_mapChannels.find(ChannelID);
		if(itChannel == m_mapChannels.end())
			return NULL;
		return &itChannel->second;
	}

	KongziHuangdiRoomChannelData*		GetFamilyChannel(T_FAMILYID FID)
	{
		uint32	ChannelID = GetFamilyChannelID(FID);
		if(ChannelID == 0)
			return NULL;
		TmapKongziHuangdiRoomChannelData::iterator	itChannel = m_mapChannels.find(ChannelID);
		if(itChannel == m_mapChannels.end())
			return NULL;
		return &itChannel->second;
	}

private:
	TmapKongziHuangdiRoomChannelData::iterator m_mapChannels_it;
public:
	KongziHuangdiRoomChannelData* GetFirstChannel()
	{
		m_mapChannels_it = m_mapChannels.begin();
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	};
	KongziHuangdiRoomChannelData* GetNextChannel()
	{
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		m_mapChannels_it ++;
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	}
	KongziHuangdiRoomChannelData* RemoveChannel()
	{
		m_mapChannels_it = m_mapChannels.erase(m_mapChannels_it);
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	}
	void CreateChannel(uint32 channelid);

	void	InitKongziHuangdiRoom();
	void	CheckDoEverydayWork();
	void	on_M_CS_LARGEACT_LIST(T_FAMILYID FID, SIPNET::TServiceId _sid);
	void	ClearLargeActUserList();
	void	SendCurrentLargeActStatus(T_FAMILYID FID, SIPNET::TServiceId _sid);
	void	RefreshLargeActStatus();
	void	SendReadyToVIP();
	void	SendLargeActPreparePacket(T_FAMILYID fid=NOFAMILYID);
	bool	StartLargeAct();
	void	on_M_CS_LARGEACT_REQUEST(T_FAMILYID FID, uint32	SyncCode, SIPNET::TServiceId _sid);
	void	RemoveFIDFromLargeActWaitList(T_FAMILYID FID);
	void	on_M_CS_LARGEACT_STEP(T_FAMILYID FID, uint32 Step);
	void	SetLargeActStep(uint32 ChannelID, uint32 Step);
	int		GetIndexOfActID(uint32 actid);
	int		GetCurrentCandiActIndex();
	void	ChangeCurrentLargeAct(int nIndex);
	void	InitLargeAct();
	void	SendReadyToVIPForLogin(T_FAMILYID fid);

	uint32	IsPossibleActReq(uint8 ActPos);
public:
//	virtual void		RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	LargeAct		m_LargeActs[MAX_LARGEACT_COUNT];
	int				m_nLargeActCount;
	int				m_nCurrentLargeActIndex;
	uint32			m_nCurrentLargeActStatus;
	std::map<uint32, uint32>	m_CurrentLargeActUserList;	// <FID, ChannelID>

protected:
	virtual void	OutRoom_Spec(T_FAMILYID _FID);
	virtual void	EnterFamily_Spec(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	//virtual void	ChangeChannelFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid);
	virtual void	ProcRemoveFamily(T_FAMILYID FID);

	virtual uint32	SaveYisiAct(T_FAMILYID FID, SIPNET::TServiceId _tsid, uint8 NpcID, uint8 ActionType, uint8 Secret, ucstring Pray, uint8 ItemNum, uint32 SyncCode, uint16* invenpos, uint32 ActID);

	uint32			GetCurrentTotalLargeActUser();
	void			CheckCurrentLargeActStatus();
};

extern bool CreateKongziHuangdiRoom();
extern void RefreshKongziHuangdiRoom();
extern void SendReadyToVIPForLogin(T_FAMILYID fid);

#endif //__KONGZIHUANGDIROOM_H
