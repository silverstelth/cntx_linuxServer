
#pragma once

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "misc/db_interface.h"

#include "World.h"
#include "../Common/QuerySetForShard.h"
#include "Character.h"
#include "Family.h"
#include "outRoomKind.h"
#include "mst_room.h"

typedef std::map<uint32, uint32>	TmapDwordToDword;

void	init_ChannelWorldService();
void	update_ChannelWorldService();
void	release_ChannelWorldService();


int CalcDistance2(float x1, float y1, float z1, float x2, float y2, float z2);

//// 2014/06/11:start:스텝방식행사위해 받는 파라미터 // 2014/06/27
//struct _JoinParam
//{
//	uint32	FID;
//	uint32	ItemID; // flowerID
//
//	_JoinParam()
//	{
//		init();
//	}
//
//	void init(uint32 _FID = 0, uint32 _ItemID = 0)
//	{
//		FID		= _FID;
//		ItemID	= _ItemID;
//	}
//
//	_JoinParam& operator=(_JoinParam& other)
//	{
//		FID		= other.FID;
//		ItemID	= other.ItemID;
//
//		return *this;
//	}
//};

struct _ActivityStepParam
{
	// 아이템관련 개인/단체행사 공통사용
	uint8				GongpinCount; // for only checksum : must be the same value of the size of GongpinsList.
	std::list<uint32>	GongpinsList; // 아이템 목록
	uint8				BowType;

	// 참가자관련 단체행사만 사용
//	uint8					JoinCount;
//	std::list<_JoinParam>	JoinList;
	uint32					IsXDG;	// 2014/07/19:개별행사에서도 사용 32비트로 확장

	uint32				StepID;

	_ActivityStepParam()
	{
		init();
	}

	void init()
	{
		GongpinCount = 0;
		StepID = 0;
		BowType = 0;
		GongpinsList.clear();

//		JoinCount = 0;
//		JoinList.clear();

		IsXDG = 0;
	}

	_ActivityStepParam& operator=(_ActivityStepParam& other)
	{
		GongpinCount	= other.GongpinCount;
		GongpinsList	= other.GongpinsList;
		BowType			= other.BowType;
		StepID			= other.StepID;

//		JoinCount		= other.JoinCount;
//		JoinList		= other.JoinList;

		IsXDG			= other.IsXDG;

		return *this;
	}

};
// end


// 초청대기자목록
struct _Invite_Data
{
	uint32			srcFID;
	uint32			targetFID;
	SIPBASE::TTime	RequestTime;						// 요청시간 - CTime::getLocalTime()

	_Invite_Data(uint32 _srcFID, uint32 _targetFID) : srcFID(_srcFID), targetFID(_targetFID)
	{
	}
};
typedef std::vector<_Invite_Data> VEC_INVITELIST;

// 방진입대기목록
struct ENTERROOMWAIT
{
	T_FAMILYID			FID;								// 대기ID
	SIPBASE::TTime		RequestTime;						// 요청시간 - CTime::getLocalTime()
};
typedef	std::map<T_FAMILYID, ENTERROOMWAIT>	MAP_ENTERROOMWAIT;

//********** 등불관련 *******************
struct _Lantern_Data
{
	uint32	X, Y, Z, Dir;
	uint32	ItemID;
	SIPBASE::TTime	CreateTime;		// CTime::getLocalTime()
	_Lantern_Data(uint32 x, uint32 y, uint32 z, uint32 dir, uint32 itemid)
		: X(x), Y(y), Z(z), Dir(dir), ItemID(itemid)
	{
	}
};
typedef std::vector<_Lantern_Data> VEC_LANTERNLIST;
//******************************************
struct _Drum_Data
{
	uint32			FID;
	uint32			DrumID;
//	T_COMMON_NAME	NPCName;
	SIPBASE::TTime	LastTime;
	_Drum_Data() : LastTime(0) {}
	void			Reset()
	{
		FID = 0;
		DrumID = 0;
//		NPCName = L"";
	}
};

extern uint32	CandiMultiLanternID;
class _MultiLanternData
{
public:
	_MultiLanternData(T_FAMILYID FID, float x, float y, float z, T_ITEMSUBTYPEID ItemID)
	{
		MultiLanterID = CandiMultiLanternID++;
		MasterID = FID;
		memset(JoinID, 0, sizeof(JoinID));
		RequestTime = SIPBASE::CTime::getLocalTime();
		dX = x;	dY = y; dZ = z;
		ReqItemID = ItemID;
		bStart = false;
	}
	uint32								MultiLanterID;
	T_FAMILYID							MasterID;
	T_FAMILYID							JoinID[MAX_MULTILANTERNJOINUSERS];
	SIPBASE::TTime						RequestTime;
	float								dX;
	float								dY;
	float								dZ;
	T_ITEMSUBTYPEID						ReqItemID;
	bool								bStart;
};
typedef	std::map<uint32, _MultiLanternData>	MAP_MULTILANTERNDATA;

//********** 폭죽관련 *********************
struct _Cracker_Data
{
	uint32			CrackerID;
	SIPBASE::TTime	DeletedTime;						// 폭죽이 터진 시간 - CTime::getLocalTime()
};
typedef std::vector<_Cracker_Data> VEC_CRACKERLIST;
//******************************************

enum ROOMEVENT_STATUS{
	ROOMEVENT_STATUS_NONE,
	ROOMEVENT_STATUS_READY,
	ROOMEVENT_STATUS_DOING
};

// 깜짝이벤트자료
struct CRoomEvent
{
	uint8	byRoomEventType;		// 
	uint8	byRoomEventPos;		// Random Value
	uint32	nRoomEventStartTime;	// GetDBTimeSecond()
	uint32	nRoomEventTimeDuration;
	void SetInfo(uint8 ty, uint8 po, uint32 st, bool bIsMusicEvent)
	{
		byRoomEventType = ty;
		byRoomEventPos = po;
		nRoomEventStartTime = st;
		nRoomEventTimeDuration = ROOM_EVENT_MAX_TIME;
		if (bIsMusicEvent) // (byRoomEventType == ROOM_EVENT_MUSIC)
			nRoomEventTimeDuration = 95;
	}
};

extern SIPBASE::TTime	GetDBTimeSecond();
extern uint32			GetDBTimeMinute();

class	CTouXiangInfo
{
public:
	CTouXiangInfo() {}
	CTouXiangInfo(uint8 ap, T_FAMILYID fid, T_FAMILYNAME fname, T_ITEMSUBTYPEID iid) :
	ActPos(ap), FID(fid), FName(fname), ItemID(iid)
	{
		StartTimeS = (uint32)GetDBTimeSecond();
	}
	virtual ~CTouXiangInfo() {}

	uint8			ActPos;
	T_FAMILYID		FID;
	T_FAMILYNAME	FName;
	T_ITEMSUBTYPEID	ItemID;
	T_S1970			StartTimeS;
};

class CActionPlayerInfo
{
public:
	CActionPlayerInfo() {}
	CActionPlayerInfo(uint32 _time, ucstring &_actStr) : FinishTime(_time), ActionString(_actStr) {}
	~CActionPlayerInfo() {}

	uint32		FinishTime;
	ucstring	ActionString;
};

class CChannelWorld;
class ChannelData
{
public:
	T_COMMON_ID			m_ChannelID;
	T_ROOMID			m_RoomID;
	TFesFamilyIds		m_mapFesChannelFamilys;
	MAP_ROOMANIMAL		m_RoomAnimal;				// 방의 동물목록
	VEC_INVITELIST		m_InviteList;				// 초청대기목록

	uint32				m_ChairFIDs[MAX_CHAIR_COUNT + 1];	// 현재 의자에 앉아있는 사용자 목록, ChairID는 1부터 시작한다.

	std::map<T_FAMILYID, uint32>	m_CurMusicFIDs;		// 현재 음악을 치고있는 사용자 & 위치
	VEC_LANTERNLIST		m_LanternList;				// 등불목록
	_Drum_Data			m_CurDrumData[MAX_DRUM_COUNT];				// 현재 북을 치고있는 사용자
	uint32				m_CurBellFID;				// 현재 종을 치고있는 사용자
	SIPBASE::TTime		m_BellLastTime;				// 종을 마지막으로 친 시간

	MAP_MULTILANTERNDATA	m_MultiLanternList;		// 집체공명등자료

	bool									m_bValidEditAnimal;
	MAP_ROOMEDITANIMAL						m_RoomEditAnimal;
	MAP_ROOMEDITANIMALACTIONINFO			m_RoomEditAnimalActionInfo;
	
	T_FAMILYID								m_MainFamilyID;

	uint32				m_FlowerUsingFID;				// 현재 방의 꽃을 심거나 물을 주는 사용자

	T_FAMILYID			m_MountLuck3FID;				// 상서동물을 타고 있는 사용자
	T_ANIMALID			m_MountLuck3AID;				// 타고 있는 상서동물 ID

	VEC_CRACKERLIST		m_CracketList;				// 삭제된 폭죽목록

	std::map<uint8, CTouXiangInfo>	m_TouXiangInfo;			

	std::map<uint32, CActionPlayerInfo>	m_ActionPlayers;					// Freezing된 사용자를 검사하기 위한 목록(FID, GetDBTimeMinute)

	CChannelWorld		*m_ChannelWorld;

public:
	ChannelData(const ChannelData &Other) 
	{
		*this = Other;
	}
	ChannelData(CChannelWorld *pWorld, T_ROOMID rid, T_COMMON_ID cid, T_MSTROOM_ID mid) : m_ChannelID(cid), m_RoomID(rid), m_bValidEditAnimal(false), m_FlowerUsingFID(0), m_CurBellFID(0), m_BellLastTime(0), m_ChannelWorld(pWorld)
	{
		for (int i = 0; i < MAX_DRUM_COUNT; i ++)
		{
			m_CurDrumData[i].Reset();
		}

		MAP_OUT_ROOMKIND::iterator itroomkind = map_OUT_ROOMKIND.find(mid);
		if (itroomkind != map_OUT_ROOMKIND.end())
		{
			OUT_ROOMKIND& outinfo = itroomkind->second;
			int channel_type = getChannelType();
			if (channel_type == 0) // C version for PC
			{
				m_RoomAnimal = outinfo.RoomAnimalDefault_PC;
				if (outinfo.EditAnimalIniFlag_PC)
				{
					m_RoomEditAnimal = outinfo.RoomEditAnimalDefault_PC;
					m_RoomEditAnimalActionInfo = outinfo.RoomEditAnimalActionInfo_PC;
					m_bValidEditAnimal = true;
				}
			}
			else if (channel_type == 2) // Unity version for PC
			{
				m_RoomAnimal = outinfo.RoomAnimalDefault_PC_Unity;
				if (outinfo.EditAnimalIniFlag_PC)
				{
					m_RoomEditAnimal = outinfo.RoomEditAnimalDefault_PC;
					m_RoomEditAnimalActionInfo = outinfo.RoomEditAnimalActionInfo_PC;
					m_bValidEditAnimal = true;
				}
			}
			else // Mobile인 경우
			{
				m_RoomAnimal = outinfo.RoomAnimalDefault_Mobile;
				if (outinfo.EditAnimalIniFlag_Mobile)
				{
					m_RoomEditAnimal = outinfo.RoomEditAnimalDefault_Mobile;
					m_RoomEditAnimalActionInfo = outinfo.RoomEditAnimalActionInfo_Mobile;
					m_bValidEditAnimal = true;
				}
			}
			
			memset(m_ChairFIDs, 0, sizeof(m_ChairFIDs));
			m_MainFamilyID = NOFAMILYID;
		}
		else
		{
			siperrornoex("Room kind info = NULL. SceneID=%d", mid);
		}
		m_MountLuck3FID = NOFAMILYID;
		m_MountLuck3AID = 0;
	}
	virtual ~ChannelData() {};
	
	void ResetMainFamily();
	void SendEditAnimalInfo(T_FAMILYID _FID, SIPNET::TServiceId sid);
	uint32	GetChannelUserCount()
	{
		uint32		user_count = 0;
		for(TFesFamilyIds::iterator itFes = m_mapFesChannelFamilys.begin();
			itFes != m_mapFesChannelFamilys.end(); itFes ++)
		{
			user_count += static_cast<uint32>(itFes->second.size());
		}
		return user_count;
	}
	bool	IsChannelFamily(T_FAMILYID FID)
	{
		for(TFesFamilyIds::iterator itFes = m_mapFesChannelFamilys.begin(); 
			itFes != m_mapFesChannelFamilys.end(); itFes ++)
		{
			if (itFes->second.find(FID) != itFes->second.end())
			{
				return true;
			}
		}
		return false;
	}
	void	ResetMountAnimalInfo()
	{
		if (m_MountLuck3FID != 0)
		{
			{
				FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_UNMOUNT_LUCKANIMAL)
					msgOut.serial(m_MountLuck3FID);
				FOR_END_FOR_FES_ALL()
			}
/*
			MAP_ROOMANIMAL::iterator itAnimal = m_RoomAnimal.find(m_MountLuck3AID);
			if (itAnimal != m_RoomAnimal.end())
			{
				ROOMANIMAL &	animal = itAnimal->second;
				static uint8	AnimalMoveType = 0;
				FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_ANIMALMOVE)
					uint8	bIsEscape = 0;
					msgOut.serial(animal.m_ID, animal.m_PosX, animal.m_PosY, animal.m_PosZ, AnimalMoveType, bIsEscape);
					AnimalMoveType ++;
				FOR_END_FOR_FES_ALL();
			}
*/
			m_MountLuck3FID = 0;
			SetAnimalControlType(m_MountLuck3AID, SCT_YES);
			m_MountLuck3AID = 0;
		}
	}
	void	SetAnimalControlType(T_ANIMALID AID, uint8 ctype)
	{
		MAP_ROOMANIMAL::iterator itAnimal = m_RoomAnimal.find(AID);
		if (itAnimal != m_RoomAnimal.end())
		{
			ROOMANIMAL &animal = itAnimal->second;
			animal.m_ServerControlType = ctype;
		}
	}
	int getChannelType() 
	{
		if ((m_ChannelID > ROOM_MOBILE_CHANNEL_START) && (m_ChannelID < ROOM_SPECIAL_CHANNEL_START)) {
			return 1; // Mobile
		}
		else if (m_ChannelID >= ROOM_SPECIAL_CHANNEL_START) { // 이 부분은 Unity PC판이 출하되면 삭제되여야 한다.
			return 0; // C PC 
		}
		else //by krs
		{
			if (m_ChannelID > ROOM_UNITY_CHANNEL_START)
				return 2; // Unity PC
			else
				return 0; // C PC
		}
	}
	virtual uint32 EstimateChannelUserCount(uint32 maxCount) = 0;

	void on_M_CS_ADD_TOUXIANG(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void SendTouXiangState(T_FAMILYID FID, SIPNET::TServiceId sid);
	void RefreshTouXiang();

	void RegisterActionPlayer(T_FAMILYID FID, uint32 LimitMinutes, ucstring &ActionStr)
	{
		CActionPlayerInfo	info(GetDBTimeMinute() + LimitMinutes, ActionStr);
		m_ActionPlayers[FID] = info;
//		sipwarning(L"Register : FID=%d, ActionString=%s", FID, ActionStr.c_str());
	}

	void UnregisterActionPlayer(T_FAMILYID FID)
	{
		std::map<uint32, CActionPlayerInfo>::iterator it = m_ActionPlayers.find(FID);
		if (it != m_ActionPlayers.end())
		{
//			sipwarning(L"Unregister : FID=%d, ActionString=%s", FID, it->second.ActionString.c_str());
			m_ActionPlayers.erase(it);
		}
	}

	T_FAMILYID FindFreezingPlayer()
	{
		uint32	now = GetDBTimeMinute();
		for (std::map<uint32, CActionPlayerInfo>::iterator it = m_ActionPlayers.begin(); it != m_ActionPlayers.end(); it ++)
		{
			if (it->second.FinishTime < now)
			{
				T_FAMILYID	FID = it->first;
				sipwarning(L"FreezenPlayer : FID=%d, ActionString=%s", FID, it->second.ActionString.c_str());
				m_ActionPlayers.erase(it);
				return FID;
			}
		}
		return 0;
	}
};

class CChannelWorld : public CWorld
{
public:
	T_MSTROOM_ID			m_SceneID;					// SceneID

	SIPBASE::TTime			m_tmLastUpdateCharacterState;	// 캐릭터들의 상태를 마지막으로 send한 시간
	SIPBASE::TTime			m_tmLastRefreshCharacterState;	// 전체 캐릭터들의 상태를 마지막으로 send한 시간

	MAP_ENTERROOMWAIT		map_EnterWait;				// 진입대기목록

	T_MSTROOM_CHNUM			m_nMaxCharacterOfRoomKind;
	uint32					m_nDefaultChannelCount;

	// 기념관 깜짝이벤트자료
	int				m_nRoomEventStatus;		// 0 - No Exist, 1 - Ready, 2+eventno - Started
	CRoomEvent		m_RoomEvent[MAX_ROOMEVENT_COUNT];

	void	LoadAnimalScope();
	void	InitMasterDataValues();
	void	RefreshEnterWait();
	void	CancelRequestEnterRoom(T_FAMILYID _FID);
	void	OutRoom(T_FAMILYID _FID, BOOL bForce = false);						// 방에서 내보낸다
	void	OutRoomAll();						// 방에서 내보낸다
	void	ClearAllFamilyActions(T_FAMILYID _FID);
	uint32	GetFamilyChannelID(T_FAMILYID FID);
	//void	ForceOutRoom(T_FAMILYID _FID, T_FAMILYID _ObjectID, SIPNET::TServiceId _sid );
	void	SendGMActive(T_FAMILYID _FID, uint8 _bOn, uint8 _utype);
	void	SendGMShow(T_FAMILYID _FID, bool _bShow);
	void	RequestEnterRoom(T_FAMILYID _FID, std::string password, SIPNET::TServiceId _sid, uint32 ChannelID = 0, uint8 ContactStatus = 0);
	void	NoticeChannelChanged(uint32 ChannelID, uint8 bAdd);
	int		CheckEnterWaitFamily(T_FAMILYID _FID, uint32 ChannelID);
	void	EnterFamily(uint32 ChannelID, T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_DIRECTION _CHDirection, T_CH_X _CHX, T_CH_Y _CHY, T_CH_Z _CHZ, SIPNET::TServiceId _sid);				// 방에 들여보낸다
	void	EnterFamilyInChannel_After(T_FAMILYID _FID, uint32 serial_key);
	void	SendAllFIDInRoomToOne(T_FAMILYID _FID, SIPNET::TServiceId sid);			// 방안의 캐릭터들의 모든 정보를 보낸다
	void	SendMountAnimal(T_FAMILYID _FID, SIPNET::TServiceId sid);
	void	SendAnimal(T_FAMILYID _FID, SIPNET::TServiceId sid);					// 움직임 동물록을 내려보낸다
	void	NotifyFamilyState(T_FAMILYID _FID);				// 방안의 모든 가족들에게 _FID가족의 상태를 알린다
	//void	ExeMethod(T_FAMILYID _FID, T_CHARACTERID _CHID, ucstring _ucMethod, uint32 _uParam);
	//void	OutChannel(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	//int		CheckChangeChannelFamily(T_FAMILYID _FID, uint32 ChannelID);
	//void	ChangeChannelFamily(uint32 ChannelID, T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_DIRECTION _CHDirection, T_CH_X _CHX, T_CH_Y _CHY, T_CH_Z _CHZ, SIPNET::TServiceId _sid);				// 방에 들여보낸다
	bool	IsInThisChannel(T_FAMILYID FID, uint32 ChannelID);

	bool	GetMoveablePos(std::string _strRegion, T_POSX _curX, T_POSY _curY, T_POSZ _curZ, T_DISTANCE _maxDis, T_POSX *_nextX, T_POSY *_nextY, T_POSZ *_nextZ);
	void	RefreshAnimal();
	void	RefreshCracker();
	void	RefreshTouXiang();

	void	RefreshCharacterState();
	void	HoldItem(T_FAMILYID _FID,  T_CHARACTERID _CHID, T_FITEMID _ItemID, SIPNET::TServiceId _sid);

	void	RefreshInviteWait();
	void	InviteUser_Req(T_FAMILYID srcFID, T_FAMILYID targetFID, SIPNET::TServiceId _tsid, uint8 ContactStatus1, uint8 ContactStatus2);
	void	InviteUser_Ans(T_FAMILYID targetFID, T_FAMILYID srcFID, uint32 ChannelID, uint8 answer);
	virtual	bool ProcCheckingRoomRequest(T_FAMILYID srcFID, T_FAMILYID targetFID, uint8 ContactStatus1, uint8 ContactStatus2) { return false; }

	void	AnimalApproached(T_FAMILYID FID, uint32 AnimalID);
	void	AnimalStop(T_FAMILYID FID, uint32 AnimalID, float X, float Y, float Z);
	void	AnimalSelect(T_FAMILYID FID, uint32 AnimalID, float X, float Y, float Z, SIPNET::TServiceId _sid);
	void	AnimalSelectStop(T_FAMILYID FID, uint32 AnimalID = 0);

	void	LanternCreate(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, uint32 X, uint32 Y, uint32 Z, uint32 Dir, SIPNET::TServiceId _sid);
	void	AddNewLantern(T_FAMILYID FID, uint32 X, uint32 Y, uint32 Z, uint32 Dir, uint32 ItemID, bool bPacketSend=true);
	void	UseRainItem(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, SIPNET::TServiceId _sid);
	void	UsePointCardItem(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, SIPNET::TServiceId _sid);
	void	UseMusicTool(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, SIPNET::TServiceId _sid);

	void	on_M_CS_MUSIC_START(T_FAMILYID FID, uint32 MusicPosID);
	void	on_M_CS_MUSIC_END(T_FAMILYID FID);
	void	on_M_CS_DRUM_START(T_FAMILYID FID, uint32 DrumID, T_COMMON_NAME	NPCName);
	void	on_M_CS_DRUM_END(T_FAMILYID FID, uint32 DrumID);
	void	on_M_CS_BELL_START(T_FAMILYID FID);
	void	on_M_CS_BELL_END(T_FAMILYID FID);
	void	on_M_CS_SHENSHU_START(T_FAMILYID FID, T_COMMON_NAME	NPCName); // by krs

	void	on_M_CS_MULTILANTERN_REQ(T_FAMILYID FID, float dX, float dY, float dZ, T_ITEMSUBTYPEID ItemID, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTILANTERN_JOIN(T_FAMILYID FID, uint32 MID, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTILANTERN_START(T_FAMILYID FID, uint32 MID, uint32 syncCode, T_FITEMPOS invenPos, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTILANTERN_END(T_FAMILYID FID, uint32 MID, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTILANTERN_CANCEL(T_FAMILYID FID, uint32 MID, SIPNET::TServiceId _tsid);
	void	on_M_CS_MULTILANTERN_OUTJOIN(T_FAMILYID FID, uint32 MID, SIPNET::TServiceId _tsid);

	virtual void	on_M_CS_MULTIACT_REQ(T_FAMILYID* ActFIDs, uint8 ActPos, uint8 ActType, uint16 CountDownSec, SIPNET::TServiceId _tsid) {}
	virtual void	on_M_CS_MULTIACT_ANS(T_FAMILYID JoinFID, uint32 ChannelID, uint8 ActPos, uint8 Answer) {}
	virtual void	on_M_CS_MULTIACT_READY(T_FAMILYID JoinFID, uint8 ActPos, SIPNET::TServiceId _tsid) {}
	virtual void	on_M_CS_MULTIACT_GO(T_FAMILYID FID, uint8 ActPos, uint32 bXianDaGong, _ActivityStepParam* pActParam) {}
	virtual void	on_M_CS_MULTIACT_START(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint16 InvenPos, uint32 SyncCode, uint8 Secret, ucstring Pray, SIPNET::TServiceId _tsid) {}
	virtual void	on_M_CS_MULTIACT_RELEASE(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint32 SyncCode, uint8 Number, uint16 *pInvenPos, SIPNET::TServiceId _tsid) {}
	virtual void	on_M_CS_MULTIACT_CANCEL(T_FAMILYID FID, uint32 ChannelID, uint8 ActPos) {}
	virtual void	on_M_CS_MULTIACT_COMMAND(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID JoinFID, uint8 Command, uint32 Param) {}
	virtual void	on_M_CS_MULTIACT_REPLY(T_FAMILYID JoinFID, uint8 ActPos, uint8 Command, uint32 Param) {}
	virtual void	on_M_CS_MULTIACT_REQADD(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID* ActFIDs, uint16 CountDownSec, SIPNET::TServiceId _tsid) {}

	virtual void	on_M_CS_ACT_STEP(T_FAMILYID FID, uint8 ActPos, uint32 StepID) {}// 14/07/23

	uint32		m_FishExp;					// 방의 물고기 경험값
	uint32		m_FishLevel;				// 방의 물고기 레벨
	uint32		m_nFishFoodCount;			// 오늘 방에 물고기먹이를 준 회수 (MAX_FISHFOOD_COUNT로 제한됨)
	uint32		m_FlowerID;					// 방의 꽃ID
	uint32		m_FlowerLevel;				// 방의 꽃 물준회수(경험값)
	uint32		m_FlowerDelTime;			// 방의 꽃이 삭제되는 시간 (초단위)
	uint32		m_FlowerFID;				// 방의 꽃을 심은 사용자FID
	ucstring	m_FlowerFamilyName;			// 방의 꽃을 심은 사용자이름
	std::map<uint32, uint32>	m_mapFamilyFlowerWaterTime;		// 꽃에 물을 준 시간

	// 中奖 - 물고기에서 마지막으로 中奖받은 시간 : 한방에서 물고기中奖은 60초내에 다시 나오지 않게 한다.
	uint32							m_LastPrizeForFishTime;	// CTime::getSecondsSince1970()

	virtual void	on_M_CS_FISH_FOOD(T_FAMILYID _FID, uint32 StoreLockID, T_FITEMPOS InvenPos, T_ITEMSUBTYPEID _ItemID, uint32 FishScopeID, uint32 SyncCode, SIPNET::TServiceId _sid);
	virtual BOOL	IsChangeableFlower(T_FAMILYID FID);
	virtual void	on_M_CS_FLOWER_NEW(T_FAMILYID _FID, uint32 StoreLockID, T_FITEMPOS InvenPos, T_ITEMSUBTYPEID _ItemID, uint32 SyncCode, SIPNET::TServiceId _sid);
	virtual int		ChangeRoomExp(uint8 RoomExpType, T_FAMILYID FID, uint32 UMoney = 0, uint32 GMoney = 0, ucstring itemInfo = "") { return 0; };
	virtual bool	CheckIfActSave(T_FAMILYID FID);
	virtual bool	IsPossiblePrize(uint8 PrizeType);
	virtual void	ProcRemoveFamily(T_FAMILYID FID) {}

	BOOL	IfFlowerExist();
	void	on_M_CS_FLOWER_NEW_REQ(T_FAMILYID _FID, uint32 CallKey, SIPNET::TServiceId _sid);
	void	on_M_CS_FLOWER_WATER_REQ(T_FAMILYID _FID, uint32 CallKey, SIPNET::TServiceId _sid);
	void	on_M_CS_FLOWER_WATERTIME_REQ(T_FAMILYID FID, SIPNET::TServiceId _sid);
	void	on_M_CS_FLOWER_WATER(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	void	on_M_CS_FLOWER_END(T_FAMILYID _FID,  SIPNET::TServiceId _sid);
	void	ChangeFishExp(int exp, uint32 FID = 0, uint32 FishScopeID = 0, uint32 FishItemID = 0);
	void	ChangeFlowerLevel(int count, uint32 FID, uint8 flag = 0);	// flag=0: previous, 1:YuYue
	void	SendFishFlowerInfo(T_FAMILYID _FID, SIPNET::TServiceId sid);
	void	SetFishExp(uint32 exp);
	void	SendMultiLanterList(T_FAMILYID _FID, SIPNET::TServiceId sid);
	// Prize
	bool			CheckPrize(T_FAMILYID FID, uint8 PrizeType, uint32 Param, bool bOnlyOne = false);
	void			CheckYuWangPrize(T_FAMILYID FID);
	void			SetCharacterDress(T_FAMILYID _FID, T_CHARACTERID _CHID, T_DRESS _DressID);

	void			RefreshFreezingPlayers();

	virtual ChannelData*	GetFirstChannel() = 0;// { return NULL; };
	virtual ChannelData*	GetNextChannel() = 0;// { return NULL; };
	virtual ChannelData*	RemoveChannel() = 0;// { return NULL; };
	virtual void			CreateChannel(uint32 channelid) = 0;// { return NULL; };
	virtual ChannelData*	GetChannel(uint32 ChannelID) = 0; // { return NULL; };
	virtual ChannelData*	GetFamilyChannel(T_FAMILYID FID) = 0; // { return NULL; };
	
	virtual uint32			GetEnterRoomCode(T_FAMILYID _FID, std::string password, uint32 ChannelID, uint8 ContactStatus) { return ERR_NOERR; };
	virtual void			RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	virtual uint8			GetFamilyRoleInRoom(T_FAMILYID _FID) { return ROOM_NONE; };
	virtual void			RequestSpecialChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid) {}
	
protected:
	virtual void	OutRoom_Spec(T_FAMILYID _FID) {};
	virtual int		CheckEnterWaitFamily_Spec() { return 0; };
	virtual void	EnterFamily_Spec(T_FAMILYID _FID, SIPNET::TServiceId _sid) {};
	//virtual void	ChangeChannelFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid) {};
	virtual void	SendChatCommon(T_FAMILYID _FID, T_CHATTEXT _Chat);	// Normal Chatting
	virtual void	SendChatRoom(T_FAMILYID _FID, T_CHATTEXT _Chat);	// Room Chatting
//	virtual void	AddRemoveSpecialChannel(uint32 channelid) {};	// only for OpenRoom

	uint32			m_nCandiSpecialCannel;
	std::map<uint32, uint32>	m_RemoveSpecialChannelList;
	virtual void AddRemoveSpecialChannel(uint32 channelid)
	{
		m_RemoveSpecialChannelList.insert(std::make_pair(channelid, channelid));
	}

public:
	CChannelWorld(TWorldType WorldType) : CWorld(WorldType), m_tmLastUpdateCharacterState(0), m_tmLastRefreshCharacterState(0), m_nFishFoodCount(0)
			, m_LastPrizeForFishTime(0)
	{
		SetFishExp(0);
		m_nFishFoodCount	= 0;
		m_FlowerID			= 0;
		m_FlowerLevel		= 0;
		m_FlowerDelTime		= 0;
		m_FlowerFID			= 0;

		m_nCandiSpecialCannel = ROOM_SPECIAL_CHANNEL_START;
	};
	virtual ~CChannelWorld();

	virtual	void SetRoomEvent();
	virtual	bool IsMusicEvent(uint8 EventType)
	{
		return IsMusicRoomEvent(EventType);
	}
	virtual void RefreshRoomEvent();
	virtual void StartRoomEvent(int i);
	void	SendRoomEventData(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	void	StartRoomEventByGM(uint8 RoomEventType);

	void SetRoomEventStatus(ROOMEVENT_STATUS st, int nIndex=-1)
	{
		switch (st)
		{
		case ROOMEVENT_STATUS_NONE:		m_nRoomEventStatus = 0;
			break;
		case ROOMEVENT_STATUS_READY:	m_nRoomEventStatus = 1;
			break;
		case ROOMEVENT_STATUS_DOING:	m_nRoomEventStatus = 2+nIndex;
			break;
		}
	}
	ROOMEVENT_STATUS GetRoomEventStatus()
	{
		if (m_nRoomEventStatus == 0)
			return ROOMEVENT_STATUS_NONE;

		if (m_nRoomEventStatus == 1)
			return ROOMEVENT_STATUS_READY;

		if (m_nRoomEventStatus >= 2)
			return ROOMEVENT_STATUS_DOING;

		return ROOMEVENT_STATUS_NONE;
	}
	int GetCurrentDoingRoomEventIndex()
	{
		return m_nRoomEventStatus - 2;
	}
	bool IsRoomMusicEventDoing()
	{
		if (m_nRoomEventStatus < 2)
			return false;
		int i = m_nRoomEventStatus - 2;
		if (IsMusicEvent(m_RoomEvent[i].byRoomEventType)) // (m_RoomEvent[i].byRoomEventType == ROOM_EVENT_MUSIC)
			return true;
		return false;
	}
};

typedef	SIPBASE::CSmartPtr<CChannelWorld>		TChannelWorld;
typedef	std::map<T_ROOMID, TChannelWorld>		TChannelWorlds;
extern TChannelWorlds				map_RoomWorlds;
extern CChannelWorld*	GetChannelWorld(T_ROOMID _rid);
extern CChannelWorld*	GetChannelWorldFromFID(T_FAMILYID _FID);
extern bool CheckPrize(T_FAMILYID FID, uint8 PrizeType, uint32 Param, bool bOnlyOne = false);

extern	void	cb_M_CS_CANCELREQEROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_OUTROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_CHANNELLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ENTERROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_CHANGECHANNELIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_CHANGECHANNELOUT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_INVITE_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_INVITE_ANS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_CHAIR_SITDOWN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_CHAIR_STANDUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_MOVECH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_STATECH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ACTION(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_CHMETHOD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_HOLDITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_EAINITINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_EAMOVE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_EAANIMATION(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_EABASEINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_EAANISELECT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_GMCS_RECOMMEND_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_GMCS_ROOMEVENT_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	SendLanternList(uint32 FID, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_LANTERN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ITEM_MUSICITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ITEM_RAIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ITEM_POINTCARD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_FISH_FOOD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_FLOWER_NEW(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_FLOWER_WATER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_FLOWER_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_FLOWER_NEW_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_FLOWER_WATER_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_FLOWER_WATERTIME_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MUSIC_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MUSIC_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_DRUM_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_DRUM_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_BELL_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_BELL_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_SHENSHU_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid); // by krs

extern	void	cb_M_CS_MULTILANTERN_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTILANTERN_JOIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTILANTERN_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTILANTERN_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTILANTERN_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTILANTERN_OUTJOIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_MULTIACT_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_ANS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_READY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_GO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_RELEASE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_COMMAND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_REPLY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MULTIACT_REQADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_CRACKER_BOMB(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	SendCrackerList(uint32 FID, SIPNET::TServiceId _tsid);

extern void		cb_M_CS_ADD_TOUXIANG(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void		cb_M_CS_ACT_STEP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

inline	bool IsMutiActionType(uint32 ActionType) 
{
	if (ActionType == AT_MULTIRITE || ActionType == AT_PRAYMULTIRITE ||  ActionType == AT_XDGMULTIRITE || ActionType == AT_ANCESTOR_MULTIJISI ||
		ActionType == AT_PUBLUCK1_MULTIRITE ||
		ActionType == AT_PUBLUCK2_MULTIRITE ||
		ActionType == AT_PUBLUCK3_MULTIRITE ||
		ActionType == AT_PUBLUCK4_MULTIRITE)
		return true;
	return false;
}
inline	bool IsSingleActionType(uint32 ActionType) 
{
	return (!IsMutiActionType(ActionType));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// 개별활동에 참가하는 NPC자료
//struct _RoomActivity_NPC
//{
//	std::string	Name;
//	uint32		X;
//	uint32		Y;
//	uint32		Z;
//	uint32		Dir;
//	uint32		ItemID;
//
//	_RoomActivity_NPC()
//	{
//		init();
//	}
//
//	void init(std::string _Name="", uint32 _X = 0, uint32 _Y = 0, uint32 _Z = 0, uint32 _Dir = 0, uint32 _ItemID = 0)
//	{
//		Name = _Name;
//		X = _X;
//		Y = _Y;
//		Z = _Z;
//		Dir = _Dir;
//		ItemID = _ItemID;
//	}
//};
// 개별활동에서 제상에 놓아진 Item자료
//struct _RoomActivity_HoldedItem
//{
//	uint32		ItemID;
//	uint32		X, Y, Z;
//	uint32		dirX, dirY, dirZ;
//};

// 집체활동에 참가하는 Family자료
struct _RoomActivity_Family
{
	uint32		FID;
	uint32		ItemID;
	uint32		Status;

	uint32		StoreLockID;
	uint16		InvenPos;
	uint32		SyncCode;
	uint8		Secret;
	ucstring	Pray;

	_RoomActivity_Family()
	{
		init();
	}

	void init(uint32 _FID = 0, uint32 _ItemID = 0, uint32 _Status = MULTIACTIVITY_FAMILY_STATUS_NONE)
	{
		FID = _FID;
		ItemID = _ItemID;
		Status = _Status;
	}
};
/*
enum _ACTIVITY_TYPE
{
	ACTIVITY_TYPE_SINGLE = 1,
	ACTIVITY_TYPE_MULTI
};
*/
struct _RoomActivity_Wait
{
	uint32		ActivityType;
	_RoomActivity_Family	ActivityUsers[MAX_MULTIACT_FAMILY_COUNT];
	_RoomActivity_Wait()
	{
		ActivityType = AT_VISIT;
		for(int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
			ActivityUsers[i].init();
	}
	_RoomActivity_Wait(uint32 _FID, uint8 _ActivityType, uint8 t, uint8 k, uint32 *ActFIDs = NULL)
	{
		for(int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
			ActivityUsers[i].init();
		ActivityUsers[0].FID = _FID;
		ActivityType = _ActivityType;

		if(ActFIDs != NULL)
		{
			for(int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
				ActivityUsers[j].init(ActFIDs[j]);
			ActivityUsers[0].Status = MULTIACTIVITY_FAMILY_STATUS_READY;
		}
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
};
typedef std::vector<_RoomActivity_Wait>	TWaitFIDList;



// 활동
struct _RoomActivity
{
	uint32				FID;
	uint32				ActivityType;	// 1 - 개별행사, 2 - 집체행사

	//	ucstring			FName;
	uint64				StartTime;	// FID>0일때 StartTime>0이면 행사준비중, StartTime=0이면 행사진행중이다.
//	std::string			ScopeModel[MAX_SCOPEMODEL_PER_ACTIVITY];
//	std::vector<_RoomActivity_NPC>	NPCs;//[MAX_NPCCOUNT_PER_ACTIVITY];
//	uint32				User_Item;
	// uint8				HoldItem_Count;
	// _RoomActivity_HoldedItem	HoldItems[MAX_ITEM_PER_ACTIVITY];	// 제상에 놓여있는 아이템 목록
//	std::list<_RoomActivity_HoldedItem>	HoldItem;	// 제상에 놓여있는 아이템 목록

	TWaitFIDList		WaitFIDs;

	uint64				LastActTime;	// 마지막으로 행사를 진행한 시간, 꽃드리기일 때 리용된다.
	uint8				LastActPos;		// 마지막으로 꽃을 드린 위치, 꽃드리기일 때 리용된다.

	_RoomActivity_Family	ActivityUsers[MAX_MULTIACT_FAMILY_COUNT];	// 집체행사인 경우에만 유효하다.

	_ActivityStepParam		ActivityStepParam; // 2014/06/11

	_RoomActivity()
	{
		LastActTime = 0;
		LastActPos = 0;
		init(NOFAMILYID, AT_VISIT, 0, 0);
	}
	void init(uint32 _FID, uint32 _ActivityType, uint8 t, uint8 k, uint32 *ActFIDs = NULL)
	{
		int	i;
		FID = _FID;
		//		FName = _FName;
		StartTime = SIPBASE::CTime::getLocalTime();
//		for(i = 0; i < MAX_SCOPEMODEL_PER_ACTIVITY; i ++)
//			ScopeModel[i] = "";
//		NPCs.resize(MAX_NPCCOUNT_PER_ACTIVITY);
//		for(i = 0; i < MAX_NPCCOUNT_PER_ACTIVITY; i ++)
//			NPCs[i].init();
//		User_Item = 0;
		// HoldItem_Count = 0;
//		HoldItem.clear();

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
};

extern bool IsCommonRoom(T_ROOMID RoomID);
