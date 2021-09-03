#ifndef __RELIGION_ROOM_H
#define __RELIGION_ROOM_H

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
#include "../Common/netCommon.h"
#include "../Common/DBData.h"
#include "../Common/Common.h"
#include "../Common/QuerySetForShard.h"
#include "SharedActWorld.h"
#include "openroom.h"
#include "mst_room.h"

//// 방에 편지드리기 탁개수
//const int DeskCountForLetter_R[] =
//{
//	36,
//};
//#define MAX_PAPER_R_DESK_COUNT	36
//typedef std::vector<uint32>	TWaitFIDList_R;

// 불교대형행사기초자료
class CReligionBYBaseData
{
public:	
	CReligionBYBaseData() { m_MasterID = NOFAMILYID; m_MasterName = L""; m_MaxNumUser = 0;};
	virtual ~CReligionBYBaseData() {};

	T_FAMILYID			m_MasterID;
	T_FAMILYNAME		m_MasterName;
	T_COMMON_CONTENT	m_ActTarget;
	T_COMMON_CONTENT	m_ActProperty;
	T_S1970				m_BeginTimeS;
	T_COMMON_CONTENT	m_ActText;
	uint8				m_ActItemNum;
	T_ITEMSUBTYPEID		m_ActItem[MAX_NUM_BYITEM];
	uint16				m_MaxNumUser;
	virtual ucstring GetRealBeginTime()
	{
		T_S1970	begintime = m_BeginTimeS;
		ucstring btime = GetTimeStrFromS1970(begintime);
		return btime;
	}

};
// 체계대형행사기초자료
class CReligionSBYBaseData : public CReligionBYBaseData
{
public:	
	CReligionSBYBaseData(uint16 MaxNum) { m_MaxNumUser = MaxNum; };
	//CReligionSBYBaseData() { m_MaxNumUser = MAX_NUM_SBYUSER; };
	CReligionSBYBaseData(const CReligionSBYBaseData &Other)   
	{
		*this = Other;
	}
	CReligionSBYBaseData &operator =(const CReligionSBYBaseData &Other) 
	{
		if (this != &Other) 
		{
			m_MasterID = Other.m_MasterID;
			m_MasterName = Other.m_MasterName;
			m_ActTarget = Other.m_ActTarget;
			m_ActProperty = Other.m_ActProperty;
			m_BeginTimeS = Other.m_BeginTimeS;
			m_BackMusicID = Other.m_BackMusicID;
			m_ActText = Other.m_ActText;
			m_ActItemNum = Other.m_ActItemNum;
			memcpy(m_ActItem, Other.m_ActItem, sizeof(m_ActItem));
			m_MaxNumUser = Other.m_MaxNumUser;
			m_ActCycle = Other.m_ActCycle;
		}
		return *this;
	}
	virtual ~CReligionSBYBaseData() {}

	void RecalcRealBeginTime()
	{
		if (m_ActCycle == SBY_ONLYONE)
			return;
		if (m_BeginTimeS > SECOND_PERONEDAY)	// 이미 recalc된 상태이면
			return;
		m_BeginTimeS = GetDBTimeDays() * SECOND_PERONEDAY + m_BeginTimeS;
		if (m_BeginTimeS <= GetDBTimeSecond())
		{
			m_BeginTimeS += SECOND_PERONEDAY;
		}
	}
	virtual ucstring GetRealBeginTime()
	{
		T_S1970	begintime = m_BeginTimeS;
		if (m_ActCycle == SBY_EVERYDAY && m_BeginTimeS <= SECOND_PERONEDAY)
		{
			begintime = GetDBTimeDays() * SECOND_PERONEDAY + m_BeginTimeS;
			if (begintime <= GetDBTimeSecond())
			{
				begintime += SECOND_PERONEDAY;
			}
		}
		ucstring btime = GetTimeStrFromS1970(begintime);
		return btime;
	}
	T_S1970 GetReadyDBTime()
	{
		return m_BeginTimeS - BUDDISMYISHI_READYTIMESECOND;
	}
	uint8				m_BackMusicID;
	uint8				m_ActCycle;
};

// 개인대형행사기초자료
class CReligionPBYBaseData : public CReligionBYBaseData
{
public:	
	CReligionPBYBaseData() { m_MaxNumUser = MAX_NUM_PBYUSER; };
	CReligionPBYBaseData(const CReligionPBYBaseData &Other)   
	{
		*this = Other;
	}
	CReligionPBYBaseData &operator =(const CReligionPBYBaseData &Other) 
	{
		if (this != &Other) 
		{
			m_MasterID = Other.m_MasterID;
			m_MasterName = Other.m_MasterName;
			m_ActTarget = Other.m_ActTarget;
			m_ActProperty = Other.m_ActProperty;
			m_BeginTimeS = Other.m_BeginTimeS;
			m_ActText = Other.m_ActText;
			m_bActTextOpenFlag = Other.m_bActTextOpenFlag;
			m_ActItemNum = Other.m_ActItemNum;
			memcpy(m_ActItem, Other.m_ActItem, sizeof(m_ActItem));
			m_MaxNumUser = Other.m_MaxNumUser;
		}
		return *this;
	}
	virtual ~CReligionPBYBaseData() {}

	bool				m_bActTextOpenFlag;
};

// 설정된 체계대형행사
class CBYListener;
class CReligionSBY
{
public:
	CReligionSBY( const uint32&	_SBYID, CReligionSBYBaseData* pBaseData, CBYListener* pListener ) :
		m_SBYID(_SBYID), m_BaseData(*pBaseData), m_BYListener(pListener)
		{
			InitForConstructor();
			m_BaseData.RecalcRealBeginTime();
		}
	virtual ~CReligionSBY() {}

	class	CJoinBooking
	{
	public:
		CJoinBooking() {}
		virtual ~CJoinBooking() {}
		T_FAMILYID	m_FID;
		uint16		m_BookingPos_base1;	// 1~
		T_S1970		m_BookingTime;
	};

protected:
	void	InitForConstructor()
	{
		m_CurrentStep = -1;
		m_bSendPrize = false;
		m_FirstFinishLocalTime = 0;
	}
public:
	void	SendCurrentState(T_FAMILYID _fid, SIPNET::TServiceId _tsid);
	bool	Update();
	bool	IsAlreadyJoin(T_FAMILYID _fid, uint16* pos);
	ENUMERR	RequestJoin(T_FAMILYID _fid, uint16* _pPosBuffer);
	void	ExitJoin(T_FAMILYID _fid);
	bool	IsAllFinished();
	bool	IsDoing();
	void	SetAllFinish();
	bool	IsBookingPos_base1(uint16 pos);
	bool	ProcSBYBooking(T_FAMILYID FID, uint16 *_pPosBuffer=NULL);
	void	RemoveBooking(T_FAMILYID FID);
	int		GetCandiJoinPos_base0();
	void	SendPrize();
	void	OverOneUser(T_FAMILYID FID);
	CBYListener*					m_BYListener;
	CReligionSBYBaseData			m_BaseData;
	uint32							m_SBYID;
	std::map<uint16, T_FAMILYID>	m_JoinUser;	// 참가위치값(1시작)을 key로 하는 map
	std::map<T_FAMILYID, CJoinBooking>	m_JoinBookingUser;	// 참가를 예약한 캐릭터정보
	std::map<T_FAMILYID, T_FAMILYID>	m_OverUser;	// 완료를 통보한 캐릭터
	std::map<T_FAMILYID, T_FAMILYID>	m_JoinHistory;	// 한번이라도 참가를 했던 캐릭터목록
	int								m_CurrentStep;
	bool							m_bSendPrize;
	SIPBASE::TTime					m_FirstFinishLocalTime;	// 처음으로 완료통보를 받은 시간
};

// 설정된 개인대형행사
class CPBYJoinData
{
public:
	CPBYJoinData() { m_Item = 0; m_HisSubID = 0;}
	virtual ~CPBYJoinData() {}
	uint32				m_HisSubID;
	T_FAMILYID			m_FID;
	T_FAMILYNAME		m_FName;
	bool				m_bOpenFlag;
	T_COMMON_CONTENT	m_ActText;
	T_ITEMSUBTYPEID		m_Item;
	T_S1970				m_JoinTime;
};
class CReligionPBY
{
public:
	CReligionPBY(const uint32&	_PBYID, CReligionPBYBaseData* pBaseData, CBYListener* pListener) :
	  m_PBYID(_PBYID), m_BaseData(*pBaseData), m_BYListener(pListener)
	  {
		  InitForConstructor();
	  }
	  virtual ~CReligionPBY() {}
protected:
	void	InitForConstructor()
	{
		m_CurrentStep = -1;
		m_CurrentStepStartTime = (uint32)GetDBTimeSecond();
		m_bSendPrize = false;
		m_FirstFinishLocalTime = 0; // by krs
	}
public:
	void	SendCurrentState(T_FAMILYID _fid, SIPNET::TServiceId _tsid);
	bool	Update();
	bool	IsAlreadyJoin(T_FAMILYID _fid, uint16* pos);
	ENUMERR	RequestJoin(T_FAMILYID _fid, uint16* _pPosBuffer, bool bOpenFlag, ucstring& ActText, uint8 ItemNum, T_ITEMSUBTYPEID* Items);
	void	ExitJoin(T_FAMILYID _fid);
	void	SuccessOverJoin(T_FAMILYID _fid);
	bool	ProcCancel(T_FAMILYID _fid);
	void	ChangeStep(T_FAMILYID _fid, uint16 ActStep);
	bool	IsAllFinished();
	void	SetAllFinish();
	void	SavePBYHis();
	void	SendPrize();

	CBYListener*					m_BYListener;
	CReligionPBYBaseData			m_BaseData;
	uint32							m_PBYID;
	std::map<uint16, CPBYJoinData>	m_JoinUser;			// 참가위치값(1시작)을 key로 하는 map
	std::map<T_FAMILYID, CPBYJoinData>	m_OverUser;		// 완료를 통보한 캐릭터
	std::map<T_FAMILYID, T_FAMILYID>	m_JoinHistory;	// 한번이라도 참가를 했던 캐릭터목록
	int								m_CurrentStep;
	uint32							m_CurrentStepStartTime;
	bool							m_bSendPrize;
	SIPBASE::TTime					m_FirstFinishLocalTime;	// 처음으로 완료통보를 받은 시간
};

class CBYListener
{
public:
	virtual void	SBYStart(CReligionSBY* pSBY) = 0;
	virtual void	SBYOver(CReligionSBY* pSBY) = 0;
	virtual void	SBYChangeStep(CReligionSBY* pSBY, uint16 curStep) = 0;
	virtual void	SBYNewJoin(CReligionSBY* pSBY, T_FAMILYID fid, uint16 joinpos) = 0;
	virtual void	SBYExitJoin(CReligionSBY* pSBY, T_FAMILYID fid, uint16 JoinPos) = 0;
	virtual void	PBYNewJoin(CReligionPBY* pPBY, T_FAMILYID fid, uint16 joinpos) = 0;
	virtual void	PBYExitJoin(CReligionPBY* pPBY, T_FAMILYID fid, uint16 joinpos) = 0;
	virtual void	PBYCancel(CReligionPBY* pPBY) = 0;
	virtual void	PBYChangeStep(CReligionPBY* pPBY, uint16 ActStep) = 0;
	virtual void	PBYOver(CReligionPBY* pPBY) = 0;
	
};

struct PBYHisMainInfo
{
	PBYHisMainInfo(uint32 HisID, T_FAMILYID fid, T_FAMILYNAME fname, T_S1970 beginTime) :
			HisMainID(HisID), FamilyID(fid), FName(fname), PBYTime(beginTime) { }
	uint32			HisMainID;
	T_FAMILYID		FamilyID;
	T_FAMILYNAME	FName;
	T_S1970			PBYTime;
};
struct PBYHisMainPageInfo
{
	PBYHisMainPageInfo() { bValid = false; HisAllCount = 0; }
	bool						bValid;
	uint32						HisAllCount;
	std::list<PBYHisMainInfo>	HisList;
};
typedef std::map<uint32, PBYHisMainPageInfo>	TPBYHisMainPageInfo;

//////////////////////////////////////////////////////////////////////////
class ReligionChannelData : public CSharedActChannelData, public CBYListener
{
public:
	//uint32				m_DeskForLetters[MAX_PAPER_R_DESK_COUNT];

	CReligionSBY		*m_CurrentSBY;						// 현재 설정된 체계대형행사
	bool				m_CurrentSBYUpdateFlag;
	CReligionPBY		*m_CurrentPBY;						// 현재 설정된 개인대형행사

	ReligionChannelData(CChannelWorld *pWorld, int actcount, T_ROOMID rid, T_COMMON_ID cid, T_MSTROOM_ID mid) : CSharedActChannelData(pWorld, actcount, rid, cid, mid)
	{
//		memset(m_DeskForLetters, 0, sizeof(m_DeskForLetters));

		m_CurrentSBY = NULL;
		m_CurrentPBY = NULL;
		m_CurrentSBYUpdateFlag = true;

		for (int j = 0; j < m_ActivityCount; j ++)
		{
			m_Activitys[j].init(DeskCountPerActivity_R[0][j]);
		}
	};
	virtual ~ReligionChannelData()
	{
		if (m_CurrentSBY != NULL)
			delete m_CurrentSBY;
		if (m_CurrentPBY != NULL)
			delete m_CurrentPBY;
	};
	virtual bool IsValidMultiActPos(uint8 ActPos)
	{
		if ((ActPos != ACTPOS_R0_YISHI_1) && 
			(ActPos != ACTPOS_R0_YISHI_2) && 
			(ActPos != ACTPOS_R0_YISHI_3) && 
			(ActPos != ACTPOS_R0_YISHI_4) && 
			(ActPos != ACTPOS_R0_YISHI_5) &&
			(ActPos != ACTPOS_R0_YISHI_6) )
			return false;
		return true;
	}
	uint32 EstimateChannelUserCount(uint32 maxCount)
	{
		uint32	usercount = GetChannelUserCount();
		if (m_CurrentSBY)
		{
			usercount += m_CurrentSBY->m_JoinBookingUser.size();
		}
		return usercount;
	}

	
public:
	virtual void	SBYStart(CReligionSBY* pSBY);
	virtual void	SBYOver(CReligionSBY* pSBY);
	virtual void	SBYChangeStep(CReligionSBY* pSBY, uint16 curStep);
	virtual void	SBYNewJoin(CReligionSBY* pSBY, T_FAMILYID fid, uint16 joinpos);
	virtual void	SBYExitJoin(CReligionSBY* pSBY, T_FAMILYID fid, uint16 JoinPos);
	virtual void	PBYNewJoin(CReligionPBY* pPBY, T_FAMILYID fid, uint16 joinpos);
	virtual void	PBYExitJoin(CReligionPBY* pPBY, T_FAMILYID fid, uint16 joinpos);
	virtual void	PBYCancel(CReligionPBY* pPBY);
	virtual void	PBYChangeStep(CReligionPBY* pPBY, uint16 ActStep);
	virtual void	PBYOver(CReligionPBY* pPBY);

	void SendSettingBYInfo(T_FAMILYID FID=NOFAMILYID, SIPNET::TServiceId sid=SIPNET::TServiceId(0));
	void SendDoingBYState(T_FAMILYID FID, SIPNET::TServiceId sid);
	bool ProcSBYBooking(T_FAMILYID FID);

	uint32 on_M_CS_SBY_ADDREQ(T_FAMILYID FID, uint32 ActID, uint16 *joinPos, SIPNET::TServiceId _tsid);
	void on_M_CS_SBY_EXIT(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_SBY_OVER(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_PBY_MAKE(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_PBY_SET(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_PBY_JOINSET(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_PBY_ADDREQ(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_PBY_EXIT(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_PBY_OVERUSER(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_PBY_CANCEL(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	void on_M_CS_PBY_UPDATESTATE(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid);
	
	virtual void	on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
};
typedef std::map<uint32, ReligionChannelData>	TmapReligionChannelData;

typedef struct _FamilyVirtueInfo
{
	T_FAMILYID		FID;
	T_FAMILYNAME	FamilyName;
	uint32			Virtue;
} FamilyVirtueInfo;

// 방관리자
class CReligionWorld : public CSharedActWorld
{
public:
	CReligionWorld(T_ROOMID RRoomID) // int DeskCountForLetter) 
		: CSharedActWorld(RRoomID, WORLD_COMMON)//, m_DeskCountForLetter(DeskCountForLetter)
	{
		m_RoomID = RRoomID;
		m_RoomName = L"RRoom";
		m_nDefaultChannelCount = RROOM_DEFAULT_CHANNEL_COUNT;

		//if(DeskCountForLetter > MAX_PAPER_R_DESK_COUNT)
		//{
		//	siperror("DeskCountForLetter > MAX_PAPER_R_DESK_COUNT!!! DeskCountForLetter=%d", DeskCountForLetter);
		//}
	}
	virtual ~CReligionWorld() {}

	TmapReligionChannelData		m_mapChannels;				// 선로ID목록

	//uint32						m_DeskCountForLetter;		// 종교구역에 있는 편지쓰기탁 개수

	uint32						m_nFamilyVirtueCount;							// 전번주의 공덕값이 높은 사용자 수
	FamilyVirtueInfo			m_FamilyVirtueList[MAX_VIRTUE_FAMILY_COUNT];	// 전번주의 공덕값이 높은 사용자들
	
	ReligionChannelData* GetChannel(uint32 ChannelID);
	ReligionChannelData* GetFamilyChannel(T_FAMILYID FID);

private:
	TmapReligionChannelData::iterator m_mapChannels_it;
public:
	ReligionChannelData* CreateNewChannel(uint32 nNewID=0);

	ReligionChannelData* GetFirstChannel()
	{
		m_mapChannels_it = m_mapChannels.begin();
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	};
	ReligionChannelData* GetNextChannel()
	{
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		m_mapChannels_it ++;
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	}
	ReligionChannelData* RemoveChannel()
	{
		m_mapChannels_it = m_mapChannels.erase(m_mapChannels_it);
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	}
	void CreateChannel(uint32 channelid)
	{
		CreateNewChannel(channelid);
	}

	void CheckDoEverydayWork();
	void	on_M_CS_ADDWISH(uint32 FID, uint16 InvenPos, ucstring targetName, ucstring pray, uint32 SyncCode, SIPNET::TServiceId _sid);

	// void	RefreshActivity();

	virtual uint32	SaveYisiAct(T_FAMILYID FID, SIPNET::TServiceId _tsid, uint8 NpcID, uint8 ActionType, uint8 Secret, ucstring Pray, uint8 ItemNum, uint32 SyncCode, uint16* invenpos, uint32 ActID);

	void	ChangeFamilyVirtue(uint8 FamilyVirtueType, T_FAMILYID FID, uint32 MoneyOrPoint = 0);
	bool	LoadFamilyVirtueHistory(T_FAMILYID FID);
	bool	SaveFamilyVirtue(INFO_FAMILYINROOM &FamilyInfo, uint32 virtue);

public:
	virtual void		RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	virtual void	RequestSpecialChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid);

protected:
	virtual void	OutRoom_Spec(T_FAMILYID _FID);
	virtual void	EnterFamily_Spec(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	//virtual void	ChangeChannelFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid);
	virtual void	ProcRemoveFamily(T_FAMILYID FID);

	void ProcRemovedSpecialChannel()
	{
		std::map<uint32, uint32>::iterator it;
		for (it = m_RemoveSpecialChannelList.begin(); it != m_RemoveSpecialChannelList.end(); it++)
		{
			uint32 removechid = it->first;
			m_mapChannels.erase(removechid);
			sipdebug("Special Channel deleted. RoomID=%d, ChannelID=%d", m_RoomID, removechid);
		}
		m_RemoveSpecialChannelList.clear();
	}

protected:
	void	SendReligionChannelData(T_FAMILYID FID, SIPNET::TServiceId sid);

public:
	void	UpdateBY();

	void ReloadEditedSBY();
	void on_M_GMCS_SBY_LIST(T_FAMILYID FID, SIPNET::TServiceId _tsid);
	void on_M_GMCS_SBY_DEL(T_FAMILYID FID, uint32 ListID, SIPNET::TServiceId _tsid);
	void on_M_CS_SBY_ADDREQ(T_FAMILYID FID, uint32 ActID, SIPNET::TServiceId _tsid);
	uint32 AddSBY(CReligionSBYBaseData& newData);
	uint32 EditSBY(uint32 ListID, CReligionSBYBaseData& newData);

	std::map<uint32, CReligionSBYBaseData>	m_SBYList;	// 편집된 체계대형행사목록(목록id가 key값이다)
	void	RemoveSBYBooking(T_FAMILYID FID);
	void	LogReligionBY(SIPBASE::CLog &log);
	
	void	RequestHisPBYPageInfo(T_FAMILYID FID, uint32 Page, uint32 PageSize, SIPNET::TServiceId _tsid);
	void	RequestHisMyPBYPageInfo(T_FAMILYID FID, uint32 Page, uint32 PageSize, SIPNET::TServiceId _tsid);
	void	RequestHisPBYInfo(T_FAMILYID FID, uint32 HisMainID, SIPNET::TServiceId _tsid);
	void	RequestHisPBYSubInfo(T_FAMILYID FID, uint32 HisSubID, SIPNET::TServiceId _tsid);
	void	PBYHisModify(T_FAMILYID FID, uint32 HisMainID, bool bOpenProperty, T_COMMON_CONTENT ActText);
	void	PBYSubHisModify(T_FAMILYID FID, uint32 HisMainID, bool bOpenProperty, T_COMMON_CONTENT ActText);
	
	TPBYHisMainPageInfo								m_HisPBYPageInfo;
	std::map<uint32, TPBYHisMainPageInfo>			m_HisMyPBYPageInfo;
	std::map<uint32, CReligionPBY>					m_HisPBY;
	std::map<uint32, CPBYJoinData>					m_HisPBYJoinInfo;

	void ResetCurrentSBY();

	//virtual	void SetRoomEvent();

protected:
	CReligionSBYBaseData* GetCandiSBY();

};

bool CreateReligionRoom(T_ROOMID RRoomID);
void RefreshReligionRoom();


extern	void	cb_M_CS_ADDWISH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_WISH_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MY_WISH_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MY_WISH_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_CS_R_PAPER_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_R_PAPER_PLAY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_R_PAPER_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_VIRTUE_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_VIRTUE_MY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_GMCS_SBY_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_GMCS_SBY_DEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_GMCS_SBY_EDIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_GMCS_SBY_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_SBY_ADDREQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_SBY_EXIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_SBY_OVER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_MAKE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_JOINSET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_ADDREQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_EXIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_OVERUSER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_UPDATESTATE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_HISLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_MYHISLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_HISINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_SUBHISINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_HISMODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_PBY_SUBHISMODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);


#endif //__RELIGION_ROOM_H
