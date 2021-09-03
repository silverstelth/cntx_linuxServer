
#pragma once

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "misc/db_interface.h"

#include "ChannelWorld.h"
#include "../Common/QuerySetForShard.h"
#include "Character.h"
#include "Family.h"
#include "outRoomKind.h"
#include "mst_room.h"


#define MAX_ACT_R_DESK_COUNT	10
// COUNT_R0_ACTPOS는 한 방에 있을수 있는 ActPos의 최대값을 대신한다. - 현재 ReligionRoom(R0)에 ActPos가 가장 많으므로 이것을 사용함.
const int DeskCountPerActivity_R[][COUNT_R0_ACTPOS] =
{
	// ReligionRoom
	{0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,5,5,5,5,5,5,0,0,0,0},		// 0은 사용되지 않는다.

	// PublicRoom
	{0,1,1,1},				// 0은 사용되지 않는다.

	// Kongzi room
	{0,0,0,9},				// 0은 사용되지 않는다.

	// Huangdi room
	{0,0,0,9}				// 0은 사용되지 않는다.
};

/***************************************************
*     for CommonRoom
***************************************************/
void cb_M_CS_REQEROOM_Rroom(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

static void	cb_M_CS_ACT_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void	cb_M_CS_ACT_WAIT_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void	cb_M_CS_ACT_RELEASE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void	cb_M_CS_ACT_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_R_ACT_INSERT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_R_ACT_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_R_ACT_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_R_ACT_MY_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_R_ACT_PRAY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_NT_ROOM_CREATE_REQ_Rroom(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);


/***************************************************
*     for ShardActWorld
***************************************************/
void	init_ShardActWorldService();
void	update_ShardActWorldService();
void	release_ShardActWorldService();

struct CSharedActivityDesk
{
public:
	uint32						FID;		// 현재 진행중인 혹은 현재 차례인 사용자
	uint8						bStartFlag;	// 0 - not started, 1 - go, 2 - started
	T_M1970						ReqTimeM;	// 신청시간의 분값(1970년이후 경과분)
	uint8						ReqTimeS;	// 신청시간의 초값(0~59)
//	uint32						User_Item;	// 사용자가 들고있는 아이템

//	std::string					ScopeModel[MAX_SCOPEMODEL_PER_ACTIVITY];
//	std::vector<_RoomActivity_NPC>	NPCs;//	_RoomActivity_NPC			NPCs[MAX_NPCCOUNT_PER_ACTIVITY];
//	uint8						HoldItem_Count;
//	_RoomActivity_HoldedItem	HoldItems[MAX_ITEM_PER_ACTIVITY];	// 제상에 놓여있는 아이템 목록

	uint8						DeskNo;
	uint32						ActivityType;	// ACTIVITY_TYPE_SINGLE, ACTIVITY_TYPE_MULTI
	_RoomActivity_Family		ActivityUsers[MAX_MULTIACT_FAMILY_COUNT];	// 집체행사인 경우에만 유효하다.

	_ActivityStepParam			ActivityStepParam; // 2014/06/11

public:
	CSharedActivityDesk()
	{
		init(NOFAMILYID, AT_VISIT);
	}

	void init(uint32 _FID, uint32 _ActivityType, uint32 *ActFIDs = NULL)
	{
		int	i;
		FID = _FID;

		bStartFlag = 0;
		ReqTimeM = GetDBTimeMinute();
		ReqTimeS = static_cast<uint8>(GetDBTimeSecond() - ReqTimeM*60);

//		for(i = 0; i < MAX_SCOPEMODEL_PER_ACTIVITY; i ++)
//			ScopeModel[i] = "";
//		NPCs.resize(MAX_NPCCOUNT_PER_ACTIVITY);
//		for(i = 0; i < MAX_NPCCOUNT_PER_ACTIVITY; i ++)
//			NPCs[i].init();
//		User_Item = 0;
//		HoldItem_Count = 0;

		ActivityType = _ActivityType;

		if(ActFIDs != NULL)
		{
			for(int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
				ActivityUsers[j].init(ActFIDs[j]);
			ActivityUsers[0].Status = MULTIACTIVITY_FAMILY_STATUS_READY;
		}
		else
		{
			for(i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
				ActivityUsers[i].init();
		}
		ActivityStepParam.init(); // 2014/06/11
	}
	bool AddFamily(T_FAMILYID _FID)
	{
		for(int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if (ActivityUsers[i].FID == 0)
			{
				ActivityUsers[i].init(_FID);
				return true;
			}
		}
		return false;
	}
	//int GetNpcidOfAct(std::string &NPCName)
	//{
	//	for(int npcid = 0; npcid < MAX_NPCCOUNT_PER_ACTIVITY; npcid ++)
	//	{
	//		if(NPCs[npcid].Name == NPCName)
	//			return npcid;
	//	}
	//	return -1;
	//}
	uint8	GetJoinFamilyCount()
	{
		uint8 count = 0;
		for(int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if (ActivityUsers[i].FID != NOFAMILYID)
				count ++;
		}
		return count;
	}
};

class CSharedActivity
{
public:
	int						ActivityDeskCount;
	CSharedActivityDesk		ActivityDesk[MAX_ACT_R_DESK_COUNT];
	TWaitFIDList			WaitFIDs;

public:
	CSharedActivity()
	{
		ActivityDeskCount = 0;
		for (uint8 i = 0; i < MAX_ACT_R_DESK_COUNT; i ++)
			ActivityDesk[i].DeskNo = i;
	}

	void init(int _DeskCount)
	{
		if(_DeskCount == 0)
		{
			_DeskCount = 1;
		}
		if(_DeskCount > MAX_ACT_R_DESK_COUNT)
		{
			sipwarning("_DeskCount > MAX_ACT_R_DESK_COUNT !!!");
		}
		ActivityDeskCount = _DeskCount;
	}

	int GetEmptyDeskPos()
	{
		for(int j = 0; j < ActivityDeskCount; j ++)
		{
			if(ActivityDesk[j].FID == NOFAMILYID)
				return j;
		}
		return -1;
	}
};

class  CNianFoInfo
{
public:
	CNianFoInfo() { FID = NOFAMILYID; }
	CNianFoInfo(T_FAMILYID fid) { FID = fid; }
	virtual ~CNianFoInfo() {}
	T_FAMILYID	FID;
	T_S1970		StartTimeS;
};

#define MAX_SHAREDACTPOS 50
class CSharedActChannelData : public ChannelData
{
public:
	std::map<uint32, CNianFoInfo>	m_NianFoInfo;			// 념불중인 사용자정보(념불npcid가 key값이다)
	std::map<uint32, CNianFoInfo>	m_NeiNianFoInfo;		

public:
	CSharedActChannelData(CChannelWorld *pWorld, int actcount, T_ROOMID rid, T_COMMON_ID cid, T_MSTROOM_ID mid) : ChannelData(pWorld, rid, cid, mid)
	{
		if (actcount > MAX_SHAREDACTPOS)
			sipwarning("actcount(%d) > MAX_SHAREDACTPOS", actcount);
		m_ActivityCount = actcount;
		m_Activitys = new CSharedActivity[m_ActivityCount];
	}
	virtual ~CSharedActChannelData()
	{
		if (m_Activitys != NULL)
		{
			delete[] m_Activitys;
			m_Activitys = NULL;
		}
	};
	CSharedActChannelData(const CSharedActChannelData &Other) : ChannelData(Other)
	{
		m_ActivityCount = 0;
		m_Activitys = NULL;
		*this = Other;
	}
	CSharedActChannelData &operator =(const CSharedActChannelData &Other)
	{
		ChannelData::operator=(Other);
		m_ActivityCount = Other.m_ActivityCount;
		if (m_Activitys != NULL)
		{
			delete[] m_Activitys;
			m_Activitys = NULL;
		}
		if (m_ActivityCount == 0)
			return *this;

		m_Activitys = new CSharedActivity[m_ActivityCount];
		for (int i = 0; i < m_ActivityCount; i++)
		{
			m_Activitys[i] = Other.m_Activitys[i];
		}
		return *this;
	}

protected:
	CSharedActivity*	m_Activitys;
	int					m_ActivityCount;
public:
	virtual void	on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid) {};
	void	on_M_CS_ACT_WAIT_CANCEL(T_FAMILYID FID, uint8 ActPos);
	void	on_M_CS_ACT_RELEASE(T_FAMILYID FID, SIPNET::TServiceId _tsid);
	void	on_M_CS_ACT_START(T_FAMILYID FID, SIPNET::TServiceId _tsid, _ActivityStepParam* pYisiParam); // 2014/06/11
public:
	void	on_M_CS_MULTIACT2_ITEMS(T_FAMILYID FID, uint8 ActPos, _ActivityStepParam* pActParam, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT2_ASK(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT2_JOIN(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT_REQ(T_FAMILYID* ActFIDs, uint8 ActPos, uint8 ActType, uint16 CountDownSec, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT_ANS(T_FAMILYID JoinFID, uint8 ActPos, uint8 Answer);
	void	on_M_CS_MULTIACT_READY(T_FAMILYID JoinFID, uint8 ActPos, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT_GO(T_FAMILYID FID, uint8 ActPos, uint32 bXianDaGong, _ActivityStepParam* pActParam);
	void	on_M_CS_MULTIACT_START(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint16 InvenPos, uint32 SyncCode, uint8 Secret, ucstring Pray, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT_CANCEL(T_FAMILYID FID, uint8 ActPos);
	void	on_M_CS_MULTIACT_COMMAND(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID JoinFID, uint8 Command, uint32 Param);
	void	on_M_CS_MULTIACT_REPLY(T_FAMILYID JoinFID, uint8 ActPos, uint8 Command, uint32 Param);
	void	on_M_CS_MULTIACT_REQADD(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID* ActFIDs, uint16 CountDownSec, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT_RELEASE(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint32 SyncCode, uint8 Number, uint16 *pInvenPos, SIPNET::TServiceId _tsid);
	void	on_M_CS_ACT_STEP(T_FAMILYID FID, uint8 ActPos, uint32 StepID);// 14/07/23
//	void	SendMultiActAskToAll(uint8 ActPos);
	void	CheckMultiactAllStarted(uint8 ActPos, int DeskNo);

	void	on_M_CS_NIANFO_BEGIN(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void	on_M_CS_NIANFO_END(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void	SendNianFoState(T_FAMILYID FID, SIPNET::TServiceId sid);
	void	ExitNianFo(T_FAMILYID FID);

	void	on_M_CS_NEINIANFO_BEGIN(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void	on_M_CS_NEINIANFO_END(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void	SendNeiNianFoState(T_FAMILYID FID, SIPNET::TServiceId sid);
	void	ExitNeiNianFo(T_FAMILYID FID);
protected:
	void	ActWaitCancel(T_FAMILYID FID, uint8 ActPos, uint8 SuccessFlag, bool bUnconditional=false);
	void	CompleteActivity(uint8 ActPos, uint8 DeskNo, uint8 SuccessFlag);
	int		GetActivityIndexOfUser(T_FAMILYID FID, uint8 ActPos);
	int		GetActivityIndexOfUser(T_FAMILYID FID);
	CSharedActivityDesk* GetActivityDesk(T_FAMILYID FID, uint8 *ActPos);
	void	SetMultiActStarted(uint8 ActPos, CSharedActivityDesk &ActivityDesk);

	void	SendMULTIACT_CANCEL(uint8 ActPos, _RoomActivity_Family* pActivityUsers, T_FAMILYID FID, bool bUnconditional);
	void	SendMULTIACT_RELEASE(uint8 ActPos, uint8 DeskNo, _RoomActivity_Family* pActivityUsers, uint8 SuccessFlag);
	void	Send_M_SC_MULTIACT2_ITEMS(uint8 ActPos, T_FAMILYID FID, SIPNET::TServiceId _tsid);

	int		FindMultiActHostDeskNo(T_FAMILYID HostFID, uint8 ActPos);
	int		FindMultiActJoinDeskNo(T_FAMILYID JoinFID, uint8 ActPos);
	int		FindMultiActDeskNo(T_FAMILYID FID, uint8 ActPos);

	//////////////////////////////////////////////////////////////////////////
public:
	// void	RefreshActivity();
	void	SendActivityList(T_FAMILYID FID, SIPNET::TServiceId _tsid);
	void	AllActCancel(T_FAMILYID FID, uint8 SuccessFlag, bool bUnconditional);
	bool	IsDoingActivity(uint8 ActPos);
	void	CancelActivityWaiting(uint8 ActPos);
protected:
	virtual bool	IsValidMultiActPos(uint8 ActPos)=0;
	virtual uint32	IsPossibleActReq(uint8 ActPos) { return ERR_NOERR; };
	_RoomActivity_Family*	FindJoinFID(uint8 ActPos, T_FAMILYID JoinFID, uint32* pRemainCount = NULL, int* pDeskNo = NULL);
};

struct SharedActInfo
{
	SharedActInfo() { ActID=0; }
	SharedActInfo(uint32 aid, T_FAMILYID fid, T_FAMILYNAME fname, uint32 modelID, uint8 at, uint8 bs, uint32 dt) :
	ActID(aid), actFID(fid), actFName(fname), ModelID(modelID), ActionType(at), bSecret(bs), DateSecond(dt) {}
	uint32			ActID;
	T_FAMILYID		actFID;
	T_FAMILYNAME	actFName;
	uint32			ModelID;
	uint8			ActionType, bSecret;
	uint32			DateSecond;
};
struct SharedActPageInfo
{
	SharedActPageInfo() : bValid(false) {}
	bool						bValid;
	std::list<uint32>		actList;
};
struct SharedActListHistory
{
	uint32	AllCount;
	std::map<uint32, SharedActPageInfo>	PageInfo;

	void Reset()
	{
		AllCount = 0;
		PageInfo.clear();
	}
};
struct SharedPrayInfo
{
	SharedPrayInfo(): PrayID(0), ItemNum(0) 
	{
		InfoTime = SIPBASE::CTime::getLocalTime();
	}
	SharedPrayInfo(uint32 pid, uint32 aid, T_FAMILYID fid, T_FAMILYNAME fname, uint8 bs, ucstring py) :
	PrayID(pid), ActID(aid), actFID(fid), actFName(fname), bSecret(bs), Pray(py), ItemNum(0) 
	{
		InfoTime = SIPBASE::CTime::getLocalTime();
	}

	uint32			PrayID;
	uint32			ActID;
	T_FAMILYID		actFID;
	T_FAMILYNAME	actFName;
	uint8			bSecret;
	ucstring		Pray;
	uint8			ItemNum;
	uint32			ItemID[MAX_ITEM_PER_ACTIVITY];
	SIPBASE::TTime			InfoTime;
};

class CSharedYiSiResult
{
public:
	CSharedYiSiResult() { m_YisiType = YishiResultType_None; }
	CSharedYiSiResult(uint8 YisiType, uint32 ActID, T_FAMILYID FID, T_FAMILYNAME FName) :
	m_YisiType(YisiType), m_ActID(ActID), m_FID(FID), m_FName(FName)	{}
	virtual ~CSharedYiSiResult() {}
	void Reset() { m_YisiType = YishiResultType_None; }

	uint8			m_YisiType;
	uint32			m_ActID;
	T_FAMILYID		m_FID;
	T_FAMILYNAME	m_FName;
};
class CSharedYiSiResultSet
{
public:
	CSharedYiSiResultSet() { m_nCandiIndex = 0; }
	virtual ~CSharedYiSiResultSet() {}
	void Reset()
	{
		m_nCandiIndex = 0;
		for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT_RELIGION; i++)
		{
			m_Result[i].Reset();
		}
	}
	void AddResult(CSharedYiSiResult& newR)
	{
		m_Result[m_nCandiIndex] = newR;
		m_nCandiIndex ++;
		if (m_nCandiIndex >= ACTIONRESULT_YISHI_ITEMCOUNT_RELIGION)
			m_nCandiIndex = 0;
	}
	uint32					m_nCandiIndex;
	CSharedYiSiResult		m_Result[ACTIONRESULT_YISHI_ITEMCOUNT_RELIGION];
};
typedef std::map<uint32, CSharedYiSiResultSet>	TSharedYiSiResultSet;


struct PublicroomFrameInfo
{
	uint8	Index;
	uint32	PhotoID;
	uint32	RoomID;
};

class CSharedActWorld : public CChannelWorld
{
protected:
	//enum
	//{
	//	SHAREDPLACETYPE_NONE = 0xFF,
	//	SHAREDPLACETYPE_RELIGION = 0,
	//	SHAREDPLACETYPE_PUBLIC
	//};

	CSharedActWorld(T_ROOMID RRoomID, TWorldType WorldType) :  CChannelWorld(WorldType), m_NextReligionCheckEveryWorkTimeSecond(0)
	{
		if (RRoomID >= MIN_RELIGIONROOMID && m_RoomID < MAX_RELIGIONROOMID)
			m_SharedPlaceType = RRoomID - MIN_RELIGIONROOMID;
		else
		{
			sipwarning("Invalid RRoomID. RRoomID=%d", RRoomID);
			m_SharedPlaceType = 0xFF;
		}
		m_nPublicroomFrameCount = 0;
	}
	virtual ~CSharedActWorld() {}
	int m_SharedPlaceType;	// 대형구역을 가르기 위한 값(불교=0, 공동=1)
public:
	virtual void	on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
	virtual void	on_M_CS_ACT_WAIT_CANCEL(T_FAMILYID FID, uint8 ActPos);
	virtual void	on_M_CS_ACT_RELEASE(T_FAMILYID FID, SIPNET::TServiceId _tsid);
	virtual void	on_M_CS_ACT_START(T_FAMILYID FID, SIPNET::TServiceId _tsid, _ActivityStepParam* pYisiParam);// 2014/06/11

	void	on_M_CS_MULTIACT2_ITEMS(T_FAMILYID FID, uint8 ActPos, _ActivityStepParam* pActParam, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT2_ASK(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTIACT2_JOIN(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
	virtual void	on_M_CS_MULTIACT_REQ(T_FAMILYID* ActFIDs, uint8 ActPos, uint8 ActType, uint16 CountDownSec, SIPNET::TServiceId _tsid);
	virtual void	on_M_CS_MULTIACT_ANS(T_FAMILYID JoinFID, uint32 ChannelID, uint8 ActPos, uint8 Answer);
	virtual void	on_M_CS_MULTIACT_READY(T_FAMILYID JoinFID, uint8 ActPos, SIPNET::TServiceId _tsid);
	virtual void	on_M_CS_MULTIACT_GO(T_FAMILYID FID, uint8 ActPos, uint32 bXianDaGong, _ActivityStepParam* pActParam);
	virtual void	on_M_CS_MULTIACT_START(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint16 InvenPos, uint32 SyncCode, uint8 Secret, ucstring Pray, SIPNET::TServiceId _tsid);
	virtual void	on_M_CS_MULTIACT_RELEASE(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint32 SyncCode, uint8 Number, uint16 *pInvenPos, SIPNET::TServiceId _tsid);
	virtual void	on_M_CS_MULTIACT_CANCEL(T_FAMILYID FID, uint32 ChannelID, uint8 ActPos);
	virtual void	on_M_CS_MULTIACT_COMMAND(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID JoinFID, uint8 Command, uint32 Param);
	virtual void	on_M_CS_MULTIACT_REPLY(T_FAMILYID JoinFID, uint8 ActPos, uint8 Command, uint32 Param);
	virtual void	on_M_CS_MULTIACT_REQADD(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID* ActFIDs, uint16 CountDownSec, SIPNET::TServiceId _tsid);

	virtual void	on_M_CS_ACT_STEP(T_FAMILYID FID, uint8 ActPos, uint32 StepID);// 14/07/23

	virtual uint32	SaveYisiAct(T_FAMILYID FID, SIPNET::TServiceId _tsid, uint8 NpcID, uint8 ActionType, uint8 Secret, ucstring Pray, uint8 ItemNum, uint32 SyncCode, uint16* invenpos, uint32 ActID) = 0;

	void			AddActInfo(SharedActInfo& actInfo);
	void			on_M_CS_R_ACT_LIST(T_FAMILYID FID, uint8 NpcID, uint32 Page, uint8 PageSize, SIPNET::TServiceId _tsid);
	void			on_M_CS_R_ACT_MY_LIST(T_FAMILYID FID, uint8 NpcID, uint32 Page, uint8 PageSize, SIPNET::TServiceId _tsid);
	void			on_M_CS_R_ACT_MODIFY(T_FAMILYID FID, uint32 PrayID, uint8 bSecret, ucstring Pray);
	void			on_M_CS_R_ACT_PRAY(T_FAMILYID FID, uint32 ActID, SIPNET::TServiceId _tsid);
	std::map<uint32, SharedPrayInfo>		m_PrayInfo;
	std::map<uint32, std::list<uint32>>		m_PraySetPerAct;
	std::map<uint32, SharedActInfo>			m_ActInfo;
	std::map<uint32, SharedActListHistory>	m_ReligionActListHistory;

	void			ReloadYiSiResultForToday();
	void			ResetSharedYiSiResult();
	void			AddSharedYiSiResult(uint32 NPCID, uint8 YiSiType, uint32 ActID, T_FAMILYID FID, T_FAMILYNAME FName, bool bNotify = true);
	void			SendYiSiResult(T_FAMILYID FID, SIPNET::TServiceId _tsid);
	void			SendSharedYiSiResultToAll(uint32 NPCID=0);

	TSharedYiSiResultSet		m_SharedYiSiResultPerNPC;

	SIPBASE::TTime	m_NextReligionCheckEveryWorkTimeSecond;
	void	InitSharedActWorld();

	void	on_M_GMCS_PUBLICROOM_FRAME_SET(T_FAMILYID FID, uint8 Index, uint32 PhotoID, uint32 ExpRoomID);

	uint32						m_nPublicroomFrameCount;
	PublicroomFrameInfo			m_PublicroomFrameList[MAX_PUBLICROOM_FRAME_COUNT];	// 벽에 붙이는 체험방자료

	virtual	bool IsMusicEvent(uint8 EventType)
	{
		return false;
	}

protected:
	virtual uint8	GetFamilyRoleInRoom(T_FAMILYID _FID);
};

extern	void	cb_M_CS_MULTIACT2_ITEMS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT2_ASK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT2_JOIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void		cb_M_CS_NIANFO_BEGIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void		cb_M_CS_NIANFO_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void		cb_M_CS_NEINIANFO_BEGIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void		cb_M_CS_NEINIANFO_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	cb_M_GMCS_PUBLICROOM_FRAME_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_PUBLICROOM_FRAME_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
