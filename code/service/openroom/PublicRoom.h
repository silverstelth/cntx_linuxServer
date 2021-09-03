#ifndef __PUBLICROOM_H
#define __PUBLICROOM_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "misc/db_interface.h"
#include "misc/time_sip.h"
#include "misc/vector.h"
#include "misc/common.h"
#include "misc/time_sip.h"

#include "net/syspacket.h"

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../../common/common.h"
#include "../Common/DBData.h"
#include "../Common/QuerySetForShard.h"
#include "../Common/Common.h"
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

class CPublicRoomWorld;
class PublicRoomChannelData : public CSharedActChannelData
{
public:
	// uint32			m_LargeActUserCountNew;
	std::list<LargeActUser>	m_LargeActUserList;
	uint32			m_nCurrentLargeActStep;

	PublicRoomChannelData(CPublicRoomWorld* pWorld, int actcount, T_ROOMID rid, T_COMMON_ID cid, T_MSTROOM_ID mid) : 
		CSharedActChannelData((CChannelWorld*)pWorld, actcount, rid, cid, mid)
	{
		m_nCurrentLargeActStep = 0;
		for (int j = 0; j < m_ActivityCount; j ++)
		{
			switch (rid)
			{
			case ROOM_ID_PUBLICROOM:
				m_Activitys[j].init(DeskCountPerActivity_R[1][j]);
				break;
			case ROOM_ID_KONGZI:
				m_Activitys[j].init(DeskCountPerActivity_R[2][j]);
				break;
			case ROOM_ID_HUANGDI:
				m_Activitys[j].init(DeskCountPerActivity_R[3][j]);
				break;
			default:
				sipwarning("Invalid RoomID. RoomID=%d", rid);
				break;
			}
		}
	};
	virtual ~PublicRoomChannelData()
	{
	};
	virtual bool IsValidMultiActPos(uint8 ActPos)
	{
		if ((ActPos != ACTPOS_PUB_YISHI_1) && 
			(ActPos != ACTPOS_PUB_YISHI_2) &&
			(ActPos != ACTPOS_PUB_LUCK))
			return false;
		return true;
	}
	virtual uint32	IsPossibleActReq(uint8 ActPos);
	uint32 EstimateChannelUserCount(uint32 maxCount);

	virtual void	on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
};
typedef std::map<uint32, PublicRoomChannelData>	TmapPublicRoomChannelData;

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

struct PLuckHisInfo
{
	PLuckHisInfo(uint32 hisID, uint32 luckID, T_FAMILYID fID, T_FAMILYNAME fName, uint32 modelID, T_S1970 luckTime) :
		HisID(HisID), LuckID(luckID), FamilyID(fID), FamilyName(fName), ModelID(modelID), LuckTime(luckTime) { }
	uint32			HisID;
	uint32			LuckID;
	T_FAMILYID		FamilyID;
	T_FAMILYNAME	FamilyName;
	uint32			ModelID;
	T_S1970			LuckTime;
};

struct PLuckHisPageInfo
{
	PLuckHisPageInfo() { bValid = false; HisAllCount = 0; }
	bool						bValid;
	uint32						HisAllCount;
	std::list<PLuckHisInfo>	HisList;
};
typedef std::map<uint32, PLuckHisPageInfo>	TPLuckHisPageInfo;

// 방�?리자
class CPublicRoomWorld : public CSharedActWorld
{
public:
	CPublicRoomWorld(T_ROOMID RRoomID) 
		: CSharedActWorld(RRoomID, WORLD_COMMON)
	{
		m_RoomID = RRoomID;
		m_RoomName = L"PRoom";
		m_nDefaultChannelCount = RROOM_DEFAULT_CHANNEL_COUNT;
		m_nLargeActCount = 0;
		m_nCurrentLargeActIndex = -1;
		m_nLuckID = 0;
	}
	virtual ~CPublicRoomWorld() {}

	TmapPublicRoomChannelData	m_mapChannels;				// ?�로ID목록

	PublicRoomChannelData*		GetChannel(uint32 ChannelID)
	{
		TmapPublicRoomChannelData::iterator	itChannel = m_mapChannels.find(ChannelID);
		if(itChannel == m_mapChannels.end())
			return NULL;
		return &itChannel->second;
	}

	PublicRoomChannelData*		GetFamilyChannel(T_FAMILYID FID)
	{
		uint32	ChannelID = GetFamilyChannelID(FID);
		if(ChannelID == 0)
			return NULL;
		TmapPublicRoomChannelData::iterator	itChannel = m_mapChannels.find(ChannelID);
		if(itChannel == m_mapChannels.end())
			return NULL;
		return &itChannel->second;
	}

private:
	TmapPublicRoomChannelData::iterator m_mapChannels_it;
public:
	PublicRoomChannelData* GetFirstChannel()
	{
		m_mapChannels_it = m_mapChannels.begin();
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	};
	PublicRoomChannelData* GetNextChannel()
	{
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		m_mapChannels_it ++;
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	}
	PublicRoomChannelData* RemoveChannel()
	{
		m_mapChannels_it = m_mapChannels.erase(m_mapChannels_it);
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	}
	void CreateChannel(uint32 channelid);

	void	InitPublicRoom();
	void	CheckDoEverydayWork();
	void	RefreshTodayLuckList();
//	void	on_M_CS_LARGEACT_LIST(T_FAMILYID FID, SIPNET::TServiceId _sid); //by krs
	void	SendLargeActList(T_FAMILYID FID, SIPNET::TServiceId _sid);
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
	void	SendReadyToVIPForLogin(T_FAMILYID fid, SIPNET::TServiceId _sid);

	void	RequestLuck(T_FAMILYID FID);
	void	RefreshLuck();
	void	RequestBlessCard(T_FAMILYID FID);
	void	NotifySendedBlesscard(T_FAMILYID FID, uint32 LuckID);
	void	RequestLuckHisPageInfo(T_FAMILYID FID, uint32 Page, uint8 PageSize, SIPNET::TServiceId _tsid);
	uint32	IsPossibleActReq(uint8 ActPos);
public:
	virtual void		RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	virtual void	RequestSpecialChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	void	cb_M_NT_AUTOPLAY3_TIANDI(T_FAMILYID _FID, ucstring &FamilyName, uint32 TianItemID, ucstring &TianPray, uint32 DiItemID, ucstring &DiPray, uint32 CountPerDay);
	void	SaveAutoplay3Log(T_FAMILYID FID, ucstring &FamilyName, uint8 NpcID, uint32 ItemID, ucstring &Pray);
	void	SaveAutoplay3Log_2(T_FAMILYID FID, ucstring &FamilyName, uint8 NpcID, uint32 ActID, uint32 ItemID, ucstring &Pray);

	LargeAct		m_LargeActs[MAX_LARGEACT_COUNT];
	int				m_nLargeActCount;
	int				m_nCurrentLargeActIndex;
	uint32			m_nCurrentLargeActStatus;
	std::map<uint32, uint32>	m_CurrentLargeActUserList;	// <FID, ChannelID>

	uint32			m_nLuckID;
	T_FAMILYID		m_LuckFID;
	T_FAMILYNAME	m_LuckFName;
	SIPBASE::TTime	m_LuckStartTime;
	uint32			m_LuckTimeMin;
	uint8			m_LuckTimeSec;
	std::map<T_FAMILYID, T_FAMILYID>	m_ReceiveLuckFamily;

	TPLuckHisPageInfo	m_LuckHisInfo;
protected:
	virtual void	OutRoom_Spec(T_FAMILYID _FID);
	virtual void	EnterFamily_Spec(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	//virtual void	ChangeChannelFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid);
	virtual void	ProcRemoveFamily(T_FAMILYID FID);

	virtual uint32	SaveYisiAct(T_FAMILYID FID, SIPNET::TServiceId _tsid, uint8 NpcID, uint8 ActionType, uint8 Secret, ucstring Pray, uint8 ItemNum, uint32 SyncCode, uint16* invenpos, uint32 ActID);

	uint32			GetCurrentTotalLargeActUser();
	void			CheckCurrentLargeActStatus();

	void ProcRemovedSpecialChannel()
	{
		std::map<uint32, uint32>::iterator it;
		for (it = m_RemoveSpecialChannelList.begin(); it != m_RemoveSpecialChannelList.end(); it++)
		{
			uint32 removechid = it->first;
			m_mapChannels.erase(removechid);
			//sipdebug("Special Channel deleted. RoomID=%d, ChannelID=%d", m_RoomID, removechid); byy krs
		}
		m_RemoveSpecialChannelList.clear();
	}
};

void	cb_M_GMCS_LARGEACT_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_GMCS_LARGEACT_DETAIL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//void	cb_M_CS_LARGEACT_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_LARGEACT_REQUEST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_LARGEACT_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_LARGEACT_STEP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	cb_M_CS_REQLUCKINPUBROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_GETBLESSCARDINPUBROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_HISLUCKINPUBROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_NT_RESGIVEBLESSCARD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	cb_M_NT_AUTOPLAY3_TIANDI(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern bool CreatePublicRoom(T_ROOMID RoomID);
extern void RefreshPublicRoom();

#endif //__PUBLICROOM_H
