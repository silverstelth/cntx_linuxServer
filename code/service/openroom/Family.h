#ifndef OPENROOM_FAMILY_H
#define OPENROOM_FAMILY_H

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../../common/common.h"
#include "../Common/Common.h"
#include "Character.h"
#include "mst_room.h"

enum TWorldType {
	WORLD_NONE,
	WORLD_LOBBY,
	WORLD_ROOM,
	WORLD_COMMON,
};

struct MAINDBMONEYLOG
{
	MAINDBMONEYLOG() : MainType(0), SubType(0), UID(0), ItemID(0), ItemTypeID(0), VI1(0), VI2(0), VI3(0) {}
	uint16		MainType;
	uint8		SubType;
	uint32		UID;
	uint32		ItemID;
	uint32		ItemTypeID;
	uint32		VI1;
	uint32		VI2;
	uint32		VI3;
};

struct FAMILY
{
	FAMILY(T_FAMILYID _ID, T_FAMILYNAME _Name, T_ACCOUNTID _UserID, ucstring _UserName, uint8 _bIsMobile, uint8 _UserType, bool _uGM, std::string _UserIP, SIPBASE::TTime _logintime) :
		m_ID(_ID), m_UserID(_UserID), m_UserName(_UserName), m_bIsMobile(_bIsMobile), m_UserType(_UserType), m_bGM(_uGM), m_UserIP(_UserIP), 
		m_WorldType(WORLD_NONE), m_WorldId(0), m_FES(NOFES), m_loginTime(_logintime), m_uShowType(CHSHOW_GENERAL), //m_SessionState(_SessionState),
		m_Name(_Name), m_LastAnimalPrizeTime(0)
		{}

		T_FAMILYID							m_ID;						// id
		T_ACCOUNTID							m_UserID;					// 拌沥id
		ucstring							m_UserName;					// 拌沥捞抚
		uint8								m_bIsMobile;				// 
		uint8								m_UserType;					// 拌沥辆幅
		bool								m_bGM;						// GM(0:No GM, 1:Yes)
		uint8								m_uShowType;				// 焊烙漂喊加己(0:老馆, 1:GM)
		TWorldType							m_WorldType;
		T_ROOMID							m_WorldId;					// 泅犁 甸绢啊 乐绰 规id
		SIPBASE::TTime						m_loginTime;				// 肺弊牢茄 矫埃
		SIPNET::TServiceId					m_FES;						// 立加结厚胶ID
		//T_SESSIONSTATE						m_SessionState;				// session state
		std::string							m_UserIP;					// 

		T_FAMILYNAME						m_Name;						// FamilyName

		uint32								m_LastAnimalPrizeTime;		// 마지막으로 AnimalPrize를 받은 시간 getSecondsSince1970()

		FAMILY() {}
};

typedef		std::map<T_FAMILYID,	FAMILY>					MAP_FAMILY;
extern		MAP_FAMILY				map_family;

typedef ITEM_LIST<MAX_INVEN_NUM>	_InvenItems;

#pragma pack(push, 1)
struct _Family_Exp_His
{
	uint32	Day;		// 날자(년월일)
	uint8	Count;		// 인원수
};
#pragma pack(pop)

struct AutoPlayRequestInfo
{
	T_ROOMID	RoomID;
	uint8		ActPos;
	uint32		IsXDG;
	ucstring	Pray;
	uint8		SecretFlag;

	void init(T_ROOMID _RoomID, uint8 _ActPos, uint32 _IsXDG, ucstring &_Pray, uint8 _SecretFlag)
	{
		RoomID = _RoomID;
		ActPos = _ActPos;
		IsXDG = _IsXDG;
		Pray = _Pray;
		SecretFlag = _SecretFlag;
	}
};

// 방안에 있는 가족정보
struct INFO_FAMILYINROOM 
{
	INFO_FAMILYINROOM(T_FAMILYID _FID, T_CHARACTERID _CHID, T_MODELID _ChModelID, T_FAMILYNAME _Name, T_F_LEVEL _Level, T_F_EXP _Exp, uint32 _GMoney, ucstring _ucComment, uint32 _uRole, uint16 _RoomCount, SIPNET::TServiceId _sid)
		:m_FID(_FID), m_ServiceID(_sid), m_ucComment(_ucComment), m_RoomRole(_uRole), 
		m_Name(_Name), m_Level(_Level), m_Exp(_Exp), m_bStatusChanged(false),
		m_RefreshMoneyTime(0), m_nUMoney(0), m_nGMoney(_GMoney), m_nCurActionGroup(0), m_RoomCount(_RoomCount), m_LuckTime(0), m_AutoPlayCount(0),
		m_DoneActInRoom(0)
	{
		m_mapCH.insert( std::make_pair(_CHID, INFO_CHARACTERINROOM(_CHID, _ChModelID)));
		m_EnterTime = SIPBASE::CTime::getLocalTime ();

		memset(m_FamilyExpHis, 0, sizeof(m_FamilyExpHis));

		memset(m_FamilyVirtueHis, 0, sizeof(m_FamilyVirtueHis));

		m_nChannelID = 1;

		m_nVirtue = 0;

		//m_bFamilyExpChanged = false;
		//m_dwFamilyExpSavedTime = 0;

		//m_bFamilyExpHisChanged = false;
		//m_dwFamilyExpHisSavedTime = 0;

		m_bFamilyExpDataChanged = false;
		m_nFamilyLevelChanged = 0;
		m_dwFamilyExpDataSavedTime = 0;
	}

	T_FAMILYID											m_FID;			// 가족id
	uint32												m_RoomRole;		// 방에서의 권한
	ucstring											m_ucComment;	// 설명
	std::map<T_CHARACTERID, INFO_CHARACTERINROOM>		m_mapCH;		// 데리고 들어온 캐릭터
	bool												m_bStatusChanged;	// 캐릭터의 상태가 변하였는가 아닌가를 표시한다.
	SIPNET::TServiceId									m_ServiceID;	// 접속써비스ID
	SIPBASE::TTime										m_EnterTime;	// 방에 들어온 시간
	uint16												m_RoomCount;	// 가지고있는 방의 개수 (tbl_Family.RoomCount)

	_InvenItems											m_InvenItems;	// 인벤 아이템 목록
	//bool												m_bInvenModified;	// 인벤이 변했는지를 나타내는 기발

	// 현재의 ActionGroup번호
	// 기념관에 들어갔을 때 Tbl_Visit에서의 GroupID이다.
	uint32												m_nCurActionGroup;

	_Family_Exp_His						m_FamilyExpHis[FAMILY_EXP_TYPE_MAX];	// Family의 경험값을 관리하기 위한 자료

	uint32								m_nVirtue;
	_Family_Exp_His						m_FamilyVirtueHis[FAMILY_VIRTUE_TYPE_MAX];	// Family의 종교구역에서의 공덕값을 관리하기 위한 자료

	T_FAMILYNAME						m_Name;						// FamilyName
	T_F_LEVEL							m_Level;					// 급수
	T_F_EXP								m_Exp;						// 경험치

	uint32								m_nUMoney, m_nGMoney;
	uint32								m_RefreshMoneyTime;			// 초

	uint32								m_nChannelID;				// 방에서 선로ID

	uint32								m_LuckTime;		// 행운을 요청한 시간 GetDBTimeSecond()
	uint32								m_DoneActInRoom;

	// For AutoPlay - 앞으로 동접자수가 많아진다면 메모리사용량이 클수 있기때문에 INFO_FAMILYINROOM에서 꺼내서 독자적인 변수로 놓아야 할수도 있다.
	uint32		m_AutoPlaySyncCode;
	uint8		m_AutoPlayCount, m_AutoPlayCurCount, m_AutoPlay2Count;
	uint16		m_AutoPlayInvenPos[MAX_AUTOPLAY_ROOM_COUNT * 2 + 1];
	AutoPlayRequestInfo	m_AutoPlayRequestList[MAX_AUTOPLAY_ROOM_COUNT * 3];

	// 데리고 들어온 캐릭터수 얻기
	T_MSTROOM_CHNUM		GetCurCharacterNum()
	{
		return (T_MSTROOM_CHNUM)(m_mapCH.size());
	}
	// 캐릭터의 상태를 설정한다
	void	SetCharacterState(T_CHARACTERID _CHID, T_CH_DIRECTION _Dir, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z, ucstring _AniName, T_CH_STATE _State, T_FITEMID _HoldItem)
	{
		std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = m_mapCH.find(_CHID);
		if (it != m_mapCH.end())
		{
			it->second.m_Direction = _Dir;
			it->second.m_X = _X;	it->second.m_Y = _Y;	it->second.m_Z = _Z;
			it->second.m_ucAniName = _AniName;
			it->second.m_AniState = _State;
			it->second.m_HoldItemID = _HoldItem;
		}
	}

	void	SetCharacterPos(T_CHARACTERID _CHID, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z)
	{
		std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = m_mapCH.find(_CHID);
		if (it != m_mapCH.end())
		{
			it->second.m_X = _X;	it->second.m_Y = _Y;	it->second.m_Z = _Z;
			m_bStatusChanged = true;
		}
	}
	
	// 손에 든 아이템을 설정한다
	void	SetCharacterHoldItem(T_CHARACTERID _CHID, T_FITEMID _Itemid)
	{
		std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = m_mapCH.find(_CHID);
		if (it != m_mapCH.end())
		{
			it->second.m_HoldItemID = _Itemid;
		}
	}
	bool	GetFirstCHState(T_CH_DIRECTION &_Dir, T_CH_X &_X, T_CH_Y &_Y, T_CH_Z &_Z)
	{
		std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = m_mapCH.begin();
		if (it == m_mapCH.end())
			return false;

		_Dir = it->second.m_Direction;
		_X = it->second.m_X;	_Y = it->second.m_Y;	_Z = it->second.m_Z;
		return true;
	}
	bool	SetFirstCHState(T_CH_DIRECTION _Dir, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z)
	{
		std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = m_mapCH.begin();
		if (it == m_mapCH.end())
			return false;
		
		it->second.m_Direction = _Dir;
		it->second.m_X = _X;	it->second.m_Y = _Y;	it->second.m_Z = _Z;
		return true;
	}
	void	DonActInRoom(uint32	act)
	{
		m_DoneActInRoom |= act;
	}
	bool	IsDoneAnyAct()
	{
		if (m_DoneActInRoom == 0)
			return false;
		return true;
	}

	//bool	m_bFamilyExpChanged;		// 사용자의 경험값이 변했는데 Save되지 않았음을 나타낸다.
	//uint32	m_dwFamilyExpSavedTime;		// 마지막으로 사용자의 경헙값을 보관한 시간, CTime::getSecondsSince1970()
	//void	SaveFamilyLevelAndExp(bool bMustSave);

	//bool	m_bFamilyExpHisChanged;		// 사용자의 경험값변화리력이 변했는데 Save되지 않았음을 나타낸다.
	//uint32	m_dwFamilyExpHisSavedTime;	// 마지막으로 사용자의 경헙값변화리력을 보관한 시간, CTime::getSecondsSince1970()
	//void	SaveFamilyExpHistory(bool bMustSave);

	bool	m_bFamilyExpDataChanged;		// 사용자의 경험값변화리력이 변했는데 Save되지 않았음을 나타낸다.
	int		m_nFamilyLevelChanged;			// 사용자의 급수가 변했는데 Save되지 않았음을 나타낸다. m_bFamilyExpDataChanged와 같이 사용된다.
	uint32	m_dwFamilyExpDataSavedTime;	// 마지막으로 사용자의 경헙값변화리력을 보관한 시간, CTime::getSecondsSince1970()
	void	SaveFamilyExpData(bool bMustSave);
};

extern		void					AddFamilyData(T_FAMILYID _FID, T_ACCOUNTID _UserID, T_FAMILYNAME _FName, ucstring _UserName, uint8 _bIsMobile, uint8 _UserType, bool _bGM, std::string strIP, SIPBASE::TTime _logintime);
extern		void					DeleteFamilyData(T_FAMILYID _FID);

extern		void					RefreshFamilyMoney(T_FAMILYID _FID);
extern		bool					CheckMoneySufficient(T_FAMILYID _FID, T_PRICE UPrice, T_PRICE GPrice, uint8 MoneyType);
extern		void					ExpendMoney(T_FAMILYID FID, T_PRICE UPrice, T_PRICE GPrice,
												uint8 uFlag, uint16 MainType, uint8 SubType,
												uint32 itemId, uint32 ItemTypeId,
												uint32 VI1 = 0, uint32 VI2 = 0, uint32 VI3 = 0);

extern		sint8					GetFamilyOnlineState(T_FAMILYID _FID);
//extern		T_SESSIONSTATE			GetFamilySessionState(T_FAMILYID _FID);
extern		FAMILY*					GetFamilyData(T_FAMILYID _FID);
extern		INFO_FAMILYINROOM*		GetFamilyInfo(T_FAMILYID _FID);

//---extern		T_FAMILYID				GetFamilyIDForName(T_FAMILYNAME _FName);
extern		T_ROOMID				GetFamilyWorld(T_FAMILYID _FID, TWorldType * pType = NULL);
extern		void					SetFamilyWorld(T_FAMILYID _FID, T_ROOMID _RoomID, TWorldType _Type = WORLD_NONE);
extern		SIPNET::TServiceId		GetFesForFamilyID(T_FAMILYID _FID);
extern		void					SetFesForFamilyID(T_FAMILYID _FID, SIPNET::TServiceId _FES);
//extern		void					AddChit(T_FAMILYID _SID, T_FAMILYID _RID, uint8 _ChitType, uint32 _P1, uint32 _P2, ucstring _P3, ucstring _P4);

extern void			AddFamilyNameData(T_FAMILYID _FID, T_FAMILYNAME _FName);
extern T_FAMILYNAME GetFamilyNameFromBuffer(T_FAMILYID _FID);

//extern		T_F_LEVEL				MakeDegreeFromVDays(T_F_VISITDAYS _vdays);
//extern      bool                    NotifyToMainSiteForUseMoney(T_FAMILYID _FID, T_PRICE _UMoney, T_PRICE &_TotalUMoney, uint8 _uFlag = 1, bool _bUserID = false);
//extern void	Family_M_CS_LOGOUT(T_FAMILYID _FID, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_FAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_NEWFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_DELFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_FNAMECHECK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_BUYHISSPACE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_EXPERIENCE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_FINDFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_FAMILYCOMMENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern void cb_M_CS_SELFSTATE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_SELFSTATE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void cb_M_CS_RESCHIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_REQ_MST_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void	cb_USERMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void cb_CANUSEMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void cb_UserMoney(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void cb_CheckPwd2(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_GMCS_GIVEMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_GMCS_GETMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_GMCS_GETLEVEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_GMCS_SETLEVEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);


//extern void	cb_M_CS_REQ_CHANGECH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

#endif // OPENROOM_FAMILY_H
