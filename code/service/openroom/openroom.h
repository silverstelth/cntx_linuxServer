#ifndef OPENROOM_BASE_H
#define OPENROOM_BASE_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "misc/db_interface.h"
#include "misc/time_sip.h"
#include "misc/vector.h"
#include "misc/common.h"
#include "misc/time_sip.h"

#include "net/syspacket.h"

#include "../Common/DBData.h"
#include "../Common/Common.h"
#include "../Common/QuerySetForShard.h"
#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../../common/common.h"
#include "ChannelWorld.h"
#include "mst_room.h"

extern	void	DestroyRoomWorlds();

//#define ROOM_DELETED_DAY						7		// 삭제된 방의 유효기일
#define ANIMAL_MOVE_DELTATIME					0.5f	// delta time of animal move interval
#define ANIMAL_UPDATE_INTERVAL					1000	// update interval time of animal move (millisecond)

//// wish info in lobby
//struct INFO_WISHINLOBBY
//{
//	uint32 WishID;
//	uint32 lobby_id;
//	T_FAMILYID			m_FID;							
//	T_FAMILYNAME		m_FName;
//	sint32                 posX;
//	sint32                 posY;
//	sint32                 posZ;
//	uint8               paraY;
//	uint16              direction;
//	uint8               wishType;
//	uint8                bOpen;
//	T_COMMON_CONTENT    wishContent;
//	uint32              ItemSubTypeId;
//    T_DATETIME      	m_CreateTime;
//	uint8               ScopeIndex;
//	uint8               SpaceIndex;
//	
//	
//	INFO_WISHINLOBBY(uint32 _WishID, uint32 _lobbyid, T_FAMILYID _FID, T_FAMILYNAME _FName, sint32 _X, sint32 _Y, sint32 _Z,
//		uint8 _pY, uint16 _dirt, uint8 _Type, uint8 _bOpen, T_COMMON_CONTENT _content, uint32 _ItemSubTypeId, T_DATETIME creationTime, uint8 _ScopeIndex, uint8 _SpaceIndex)
//	{
//		WishID = _WishID;
//		lobby_id = _lobbyid;
//		m_FID = _FID;
//		m_FName = _FName;
//		posX = _X;
//		posY = _Y;
//		posZ = _Z;
//		paraY = _pY;
//		direction = _dirt;
//		wishType = _Type;
//		bOpen  = _bOpen;
//		wishContent = _content;
//		ItemSubTypeId = _ItemSubTypeId;
//		m_CreateTime = creationTime;
//		ScopeIndex = _ScopeIndex;
//		SpaceIndex = _SpaceIndex;
//	}
//	INFO_WISHINLOBBY() {}
//};
// 고인자료
struct DECEASED 
{
	DECEASED()
		:m_bValid(false), m_PhotoId(0), m_Surnamelen(0)	{}
	T_DECEASED_POS		m_Pos;								// 위치
	T_DECEASED_NAME		m_Name;								// 이름
	uint8				m_Surnamelen;						// 성길이
	T_SEX				m_Sex;								// 성별
	T_DATE4				m_Birthday;							// 생일
	T_DATE4				m_Deadday;							// 사망일
    T_DECEASED_HIS		m_His;								// 략력
	ucstring			m_Domicile;							// 
	ucstring			m_Nation;							// 
	uint32				m_PhotoId;							// 초상사진ID
	bool				m_bValid;							// 정보가 있는가
	bool			IsValid() {  return m_bValid; };
};

//// 방의 부분품
//struct ROOMPART 
//{
//	T_ROOMPARTID		m_PartID;							// 부분품ID
//};
//typedef	std::map<T_ROOMPARTID, ROOMPART>	MAP_ROOMPART;

//// 제상의 제물목록
//struct ROOMSACRIFICE
//{
//	T_COMMON_ID			m_SacID;							// DB에서 ID
//	T_ITEMSUBTYPEID		m_SubTypeId;						// 아이템ID
//	T_SACRIFICEITEMNUM	m_Num;								// 아이템개수
//	T_SACRIFICEX		m_X;								// 위치X
//	T_SACRIFICEY		m_Y;								// 위치Y
//	SIPBASE::MSSQLTIME			m_putTime;							// 놓은 시간
//};
//typedef	std::map<T_COMMON_ID, ROOMSACRIFICE>	MAP_ROOMSACRIFICE;

// 개별행사목록 -- 개별활동 구조체로 바뀜
//struct ROOMEVENT
//{
//	T_FAMILYID			m_FID;								// 행사를 예약한 가족ID
//	T_FAMILYNAME		m_FName;							// 가족이름
//	T_ROOMEVENTTYPE		m_EventType;						// 개별행사 종류
//	SIPBASE::TTime		m_OrderTime;						// 예약시간
//	SIPBASE::TTime		m_StartTime;						// 시작시간
//};
//typedef	std::list<ROOMEVENT>					LST_ROOMEVENT;	

// 비석목록
typedef	std::map<T_TOMBID, T_COMMON_CONTENT>	MAP_ROOMTOMB;

// 방의 페트목록
//struct ROOMPET
//{
//	T_COMMON_ID			m_ID;							// 구분ID(DB)
//	T_PETID				m_PetID;						// 페트ID
//	T_PETTYPEID			m_PetTypeID;					// 페트종류ID
//	T_PETCOUNT			m_PetNum;						// 수량
//	T_PETGROWSTEP		m_PetGrow;						// 페트단계
//	T_PETLIFE			m_PetLife;						// 페트생명력
//	SIPBASE::MSSQLTIME	m_CreateTime;					// 페트만든시간
//	T_PETREGIONID		m_PetRegion;					// 페트령역ID
//	ucstring			m_ucPos;						// 초기위치
//	SIPBASE::MSSQLTIME	m_LastLifeTime;					// 마지막으로 생명력 늘은 시간
//	bool				m_DefaultPet;					// Default페트인가?
//};
//typedef	std::map<T_COMMON_ID, ROOMPET>					MAP_ROOMPET;

//// 소원목록
//struct ROOMWISH
//{
//	T_COMMON_ID			m_ID;							// 구분ID(DB)
//	T_WISHPOS			m_Pos;							// 소원번호
//	T_COMMON_CONTENT	m_Wish;							// 소원내용
//	bool				m_bOpen;						// 이름공개속성
//	T_FAMILYID			m_FID;							// 소원을 넣은 가족
//	T_FAMILYNAME		m_FName;						// 가족이름
//	SIPBASE::MSSQLTIME	m_CreateTime;					// 소원넣은시간
//};
//typedef	std::map<T_WISHPOS, ROOMWISH>					MAP_ROOMWISH;

//// 신비문출현목록
//struct ROOMTGATE
//{
//	T_ROOMVIRTUE		RoomVirtue;			// 방의 공덕값
//	float				GateFre;			// 출현빈도수
//	uint32				GateTime;			// 출현시간(초)
//	REGIONSCOPE			GateScope;			// 출현Scope
//};
//typedef	std::list<ROOMTGATE>		LST_ROOMTGATE;

//// 생성된 신비한문
//struct	ROOMNOWGATE
//{
//	ROOMNOWGATE()		
//	{
//		bValidParam = false;
//		fMakeNum = 0.0f;
//		LastTime = SIPBASE::CTime::getLocalTime();
//		Reset();
//	}
//	bool				bIs;				// 유효한가?
//	SIPBASE::TTime		DeadTime;			// 문이 없어져야 하는 시간
//	SIPBASE::TTime		LastTime;			// 마감생성시간
//	T_POSX				X;					// 생성된 위치X
//	T_POSY				Y;					// 생성된 위치Y
//	T_POSZ				Z;					// 생성된 위치Z
//	bool				bValidParam;
//	ROOMTGATE			Param;				// 생성파라메터
//	uint32				uRepeatNum;
//	float				fMakeNum;			// 문출현회수
//	void	Reset()
//	{
//		bIs = false;
//		uRepeatNum = 0;
//	}
//};

// 방의 경험값을 관리하기 위한 루적자료
struct _Room_Exp_His
{
	uint32	Day;		// 날자(년월일)
	uint8	Count;		// 인원수
};


struct _Letter_Modify_Data
{
	uint32		FID;
	uint32		LetterID;
};
typedef std::vector<_Letter_Modify_Data> VEC_LETTER_MODIFY_LIST;

struct BlessLampData
{
	uint32		MemoID;
	uint32		FID;
	ucstring	FName;
	uint32		Time;	// GetDBTimeSecond - 길상등 올린 시간
};
class CRoomWorld;
class RoomChannelData : public ChannelData
{
public:
	_RoomActivity		m_Activitys[COUNT_ACTPOS];			// 방에서 진행되는 활동자료들
	BlessLampData		m_BlessLamp[MAX_BLESS_LAMP_COUNT];	// 길상등(BlessLamp)

	RoomChannelData(CRoomWorld *pWorld, T_ROOMID rid, T_COMMON_ID cid, T_MSTROOM_ID mid) : ChannelData((CChannelWorld*)(pWorld), rid, cid, mid)
	{
		for (int i = 0; i < MAX_BLESS_LAMP_COUNT; i ++)
		{
			m_BlessLamp[i].MemoID = 0;
		}
	}
	virtual ~RoomChannelData() {};

	void SetFamilyName(T_FAMILYID FID, T_FAMILYNAME FamilyName)
	{
		for (int i = 0; i < MAX_BLESS_LAMP_COUNT; i++)
		{
			if (m_BlessLamp[i].MemoID == 0)
				continue;
			if (m_BlessLamp[i].FID != FID)
				continue;
			m_BlessLamp[i].FName = FamilyName;
		}
	}
	uint32 EstimateChannelUserCount(uint32 maxCount)
	{
		return GetChannelUserCount();
	}
};
typedef std::map<uint32, RoomChannelData>	TmapRoomChannelData;

#define MAX_ACTIONRESULT_BUFSIZE	300	// > ACTIONRESULT_YISHI_ITEMCOUNT(10) * (1 + 4 + 4 + MAX_LEN_CHARACTER_NAME(8) * 2)
class ActionResult
{
public:
	uint32		m_nRoomID;
	uint32		m_AncestorDeceasedCount;
	uint32		m_nTime[ActionResultType_MAX];
	uint32		m_nCurIndex[ActionResultType_MAX];

	uint8		m_byIDs[ActionResultType_MAX][MAX_ACTIONRESULT_ITEM_COUNT];
	uint32		m_nIDs[ActionResultType_MAX][MAX_ACTIONRESULT_ITEM_COUNT];
	uint32		m_FIDs[ActionResultType_MAX][MAX_ACTIONRESULT_ITEM_COUNT];
	ucstring	m_strNames[ActionResultType_MAX][MAX_ACTIONRESULT_ITEM_COUNT];

	ActionResult();
	void setRoomInfo(int nRoomID, int nAncestorDeceasedCount);
	void init(uint8 type, uint32 time, uint8* buf);
	void clearPrevData();
	int getCurIndex(int type) { return m_nCurIndex[type]; }

	void registerXianbaoResult(int xianbao_type, uint32 action_id, T_FAMILYID FID, ucstring name);
	void registerYishiResult(int yishi_type, uint32 action_id, T_FAMILYID FID, ucstring name, BOOL bIsAuto = false);
	void deleteYishiResult(uint32 action_id);
	void registerAuto2Result(uint32 action_id, T_FAMILYID FID, ucstring name);
	void registerAncestorJisiResult(int yishi_type, uint32 action_id, T_FAMILYID FID, ucstring name);
	void registerAncestorDeceasedXiangResult(int deceased_index, int itemID, ucstring name);
	void registerAncestorDeceasedHuaResult(int deceased_index, int itemID, ucstring name);

	int removeAncestorDeceasedCurrentHua(int deceased_index);

private:
	void clearPrevData(int type, uint32 curTime);

	void saveActionResultData(int type, uint8 *buf, int bufsize);

	void saveXianbaoResultData();
	void saveYishiResultData();
	void saveAuto2ResultData();
	void saveAncestorJisiResultData();
	void saveAncestorDeceasedXiangResultData(int type);
	void saveAncestorDeceasedHuaResultData(int type);

	void sendXianbaoResultData();
	void sendYishiResultData();
	void sendAuto2ResultData();
	void sendAncestorJisiResultData();
	void sendAncestorDeceasedXiangResultData(int type);
	void sendAncestorDeceasedHuaResultData(int type);
};

// 조상탑의 고인정보
struct ANCESTOR_DECEASED 
{
	uint8				m_AncestorIndex;					// 
	uint8				m_DeceasedID;						// 위치
	uint8				m_Surnamelen;						// 성길이
	ucstring			m_Name;								// 이름
	T_SEX				m_Sex;								// 성별
	T_DATE4				m_Birthday;							// 생일
	T_DATE4				m_Deadday;							// 사망일
	ucstring			m_His;								// 략력
	ucstring			m_Domicile;							// 
	ucstring			m_Nation;							// 
	uint32				m_PhotoId;							// 초상사진ID
	uint8				m_PhotoType;						// 초상사진표시형태

	void init(uint8 _AncestorIndex, uint8 _DeceasedID, uint8 _Surnamelen, ucstring _Name, T_SEX _Sex, T_DATE4 _Birthday, T_DATE4 _Deadday, 
		ucstring _His, ucstring _Domicile, ucstring _Nation, uint32 _PhotoId, uint8 _PhotoType)
	{
		m_AncestorIndex = _AncestorIndex;
		m_DeceasedID = _DeceasedID;
		m_Surnamelen = _Surnamelen;
		m_Name = _Name;
		m_Sex = _Sex;
		m_Birthday = _Birthday;
		m_Deadday = _Deadday;
		m_His = _His;
		m_Domicile = _Domicile;
		m_Nation = _Nation;
		m_PhotoId = _PhotoId;
		m_PhotoType = _PhotoType;
	}
};

// AutoPlay
struct AutoPlayUnitInfo
{
	uint32		UnitID;
	T_ROOMID	RoomID;
	ucstring	FamilyName;
	uint8		ActPos;
	uint32		IsXDG;
	ucstring	Pray;
	uint8		SecretFlag;
	uint32		ItemID[9];
	uint32		UsedItemID;
	uint8		UsedItemMoneyType;

	AutoPlayUnitInfo& operator=(AutoPlayUnitInfo& other)
	{
		UnitID = other.UnitID;
		RoomID = other.RoomID;
		FamilyName = other.FamilyName;
		ActPos = other.ActPos;
		IsXDG = other.IsXDG;
		Pray = other.Pray;
		SecretFlag = other.SecretFlag;
		memcpy(ItemID, other.ItemID, sizeof(ItemID));
		UsedItemID = other.UsedItemID;
		UsedItemMoneyType = other.UsedItemMoneyType;

		return *this;
	}
};

// Current AutoPlay User Info
struct AutoPlayUserInfo
{
public:
	T_FAMILYID		AutoFID;
	uint8			ActPos;
	T_FAMILYID		FID;
	T_FAMILYNAME	FamilyName;
	T_MODELID		ModelID;
	T_DRESS			DressID;
	T_FACEID		FaceID;
	T_HAIRID		HairID;

	AutoPlayUserInfo(T_FAMILYID _AutoFID, uint8 _ActPos, T_FAMILYID _FID, T_FAMILYNAME _FamilyName, 
		T_MODELID _ModelID, T_DRESS _DressID, T_FACEID _FaceID, T_HAIRID _HairID)
		:	AutoFID(_AutoFID), ActPos(_ActPos), FID(_FID), FamilyName(_FamilyName), 
			ModelID(_ModelID), DressID(_DressID), FaceID(_FaceID), HairID(_HairID)
	{
	}
};

// 금붕어정보
struct GoldFishOne
{
public:
	uint32		GoldFishID;
	uint32		GoldFishDelTime;
	T_FAMILYID	FID;
	T_FAMILYNAME	FName;
	GoldFishOne()
	{
		GoldFishID = 0;
		GoldFishDelTime = 0;
		FID = 0;
		FName = L"";
	}
};
class GoldFishData
{
public:
	GoldFishOne	m_GoldFishs[MAX_GOLDFISH_COUNT];
};

// 기념관보물창고 아이템 Lock자료
class RoomStoreLockData
{
public:
	uint32			LockID;
	T_FAMILYID		FID;
	uint8			ItemCount;
	uint32			ItemIDs[MAX_ITEM_PER_ACTIVITY];
	uint32			LockTime;	// GetDBTimeSecond()
	RoomStoreLockData() {};
	RoomStoreLockData(uint32 _LockID, T_FAMILYID _FID, uint8 _ItemCount, uint32* pItemIDs);
};

// 기념관보물창고자료
class RoomStore
{
public:
	uint32	StoreSyncCode;
	std::map<uint32, uint16>	mapStoreItems;		// <ItemID, ItemCount>
	std::map<uint32, RoomStoreLockData>	mapLockData;	// <LockID, RoomStoreLockData>
	uint32	AllItemCount;	// mapStoreItems 의 전체 아이템 개수
	uint8	LastItemCount;
	uint32	LastItemID[MAX_ROOMSTORE_SAVE_COUNT];		// 마지막에 증정한 아이템 (책상우에 놓여있는 아이템)

	RoomStore() : StoreSyncCode(0), LastItemCount(0), AllItemCount(0) {};
	void init(uint8 *buff, uint32 len);
	void init(RoomStore &data);
	int GetStoreBoxCount();
};

class RoomManager
{
public:
	uint32	FID;
	uint8	Permission;
};

class RoomRuyibei
{
public:
	T_COMMON_ID			ItemID;
	T_COMMON_CONTENT	Ruyibei;
};

// 방관리자
class CRoomWorld : public CChannelWorld
{
public:
	T_FAMILYID								m_CreatorID;				// 방창조자ID
	T_FAMILYID								m_MasterID;					// 방주인ID
	T_FAMILYNAME							m_MasterName;				// 방주인이름
	T_ACCOUNTID								m_MasterUserID;				// 방주인계정ID
	T_ROOMPOWERID							m_PowerID;					// 방리용권한
	T_DATETIME								m_CreationDay;				// 방창조날자
	T_DATETIME								m_BackDay;					// 방반환날자
	SIPBASE::MSSQLTIME						m_BackTime;					// 반환날자
	T_ROOMVISITS							m_Visits;					// 방문회수
	T_ROOMVDAYS								m_VDays;					// 유효입방날자
	T_ROOMLEVEL								m_Level;					// 방급수
	T_ROOMEXP								m_Exp;						// 방경험값
	T_ROOMVIRTUE							m_Virtue;					// 방공덕값
	T_ROOMFAME								m_Fame;						// 방명망값
	T_ROOMAPPRECIATION						m_Appreciation;				// 방감은값
	SIPBASE::MSSQLTIME						m_LastIncTime;				// 마지막 유효방문계산
	uint32									m_URLPassword;				// 사적자료인증값(관리자용)
	uint32									m_URLPasswordForContact;	// 사적자료인증값(련계인용)
	DECEASED								m_Deceased1;				// 고인1
	DECEASED								m_Deceased2;				// 고인2
	T_COMMON_CONTENT						m_RoomComment;				// 방설명문
	uint32									m_HisSpaceCapacity;			// 총 사적자료공간
	uint32									m_HisSpaceUsed;				// 사용한 사적자료공간
	uint8									m_FreeFlag;
	uint8									m_PhotoType;						// 초상사진표시형태

	//MAP_ROOMPART							m_RoomPart;					// 방의 부분품정보
	//MAP_ROOMSACRIFICE						m_RoomSacrifice;			// 제물목록
	MAP_ROOMTOMB							m_RoomTomb;					// 비석목록
	RoomRuyibei								m_RoomRuyibei;
	//LST_ROOMEVENT							m_RoomEvent;				// 방의 개별행사예약목록
	//MAP_ROOMPET								m_RoomPet;					// 기념관페트목록
	//MAP_ROOMWISH							m_RoomWish;					// 소원목록

	std::map<uint32, GoldFishData>			m_GoldFishData;				// 금붕어정보

	uint32									m_nPeacockType;				// 공작새 종류 : 초기화하지 않아도 되겠지?

	//LST_ROOMTGATE							m_RoomTGate;				// 신비문출현목록
	//ROOMNOWGATE								m_CurGate;					// 현재의 신비문상태
	bool									m_bClosed;					// 동결기발
	T_FAMILYID								m_CurGiveID;				// 방주인으로 양도하려는 가족ID
	SIPBASE::TTime							m_CurGiveTime;				// 양도시작한 시간 - CTime::getLocalTime()
	SIPBASE::MSSQLTIME						m_LastGiveTime;				// 마지막으로 양도한 시간
	T_DATETIME								m_RenewTime;				// time of last renew time
	std::string								m_AuthKey;					// Password

	_Room_Exp_His							m_RoomExpHis[ROOM_EXP_TYPE_MAX];	// 방의 경험값을 관리하기 위한 자료
	bool									m_bRoomExpChanged;			// 방의 경험값이 변했는데 Save되지 않았음을 나타낸다.
	uint32									m_dwRoomExpSavedTime;		// 마지막으로 방의 경헙값을 보관한 시간, CTime::getSecondsSince1970()

	VEC_LETTER_MODIFY_LIST					m_LetterModifyList;	// 현재 방에서 수정중에 있는 편지목록
	//VEC_LETTER_MODIFY_LIST					m_MemoModifyList;	// 현재 방에서 수정중에 있는 방문록목록

	//// 한사람은 한방에서 방문록에 10개까지의 글을 남길수 있다.
	//uint32									m_LiuyanDay;				// 날자(년월일)
	//TmapDwordToDword						m_mapFamilyLiuyanTime;		// 방문록에 글남긴 수

	// 길상등
	uint32									m_BlesslampDay;				// 날자(년월일)
	TmapDwordToDword						m_mapFamilyBlesslampTime;	// 길상등을 남긴 수

	TmapRoomChannelData						m_mapChannels;				// 기념관 선로ID목록

	int										m_RoomManagerCount;	// 방관리자 수
	RoomManager								m_RoomManagers[MAX_MANAGER_COUNT];	// 방관리자목록

	// 행사결과물
	ActionResult							m_ActionResults;

	// 조상탑정보
	ucstring						m_AncestorText;
	int								m_nAncestorDeceasedCount;
	ANCESTOR_DECEASED				m_AncestorDeceased[MAX_ANCESTOR_DECEASED_COUNT * 2];

	// 행운출현자료
	uint32							m_LuckID_Lv3;
	uint32							m_LuckID_Lv4;
	SIPBASE::TTime					m_LuckID_Lv4_ShowTime;

	// 현재 방의 AutoPlay목록
	std::vector<AutoPlayUserInfo>	m_AutoPlayUserList;

	// 기념관보물창고
	RoomStore						m_RoomStore;

	// 2배경험값 - 방창조날에 2배경험값을 준다.
	bool							m_bIsRoomcreateDay;

//	uint8			m_byRoomEventType[MAX_ROOMEVENT_COUNT];		// 
//	uint8			m_byRoomEventPos[MAX_ROOMEVENT_COUNT];		// Random Value
//	uint32			m_nRoomEventStartTime[MAX_ROOMEVENT_COUNT];	// GetDBTimeSecond()

	void			init(uint32 RoomID);

	RoomChannelData* GetChannel(uint32 ChannelID);
	RoomChannelData* GetFamilyChannel(T_FAMILYID FID);

	void			DoEverydayWorkInRoom();
	void			CheckRoomcreateHoliday();

	bool			IsValid() { if (m_RoomID != 0) return true; return false; };
	virtual uint32	GetEnterRoomCode(T_FAMILYID _FID, std::string password, uint32 ChannelID, uint8 ContactStatus);
	virtual void	RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	virtual void	RequestSpecialChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	virtual uint8	GetFamilyRoleInRoom(T_FAMILYID _FID);
	virtual void	EnterFamily_Spec(T_FAMILYID _FID, SIPNET::TServiceId _sid);

	//void			ProlongRequestFreeRoom(T_FAMILYID _FID, SIPNET::TServiceId _sid);

	T_MSTROOM_CHNUM	GetCurCharacterNum();							// 방의 현재 캐릭터수 얻기
	T_MSTROOM_CHNUM	GetCurCharacterNumForOther();					// 방의 현재 캐릭터수 얻기(주인, 관리자제외)

	//bool			IsBreakObject(T_FAMILYID _FID);					// 차단객으로 등록되였는가
	//bool			IsNeighborObject(T_FAMILYID _FID);				// 련계인으로 등록되였는가
	int				IsRoomManager(T_FAMILYID _FID);					// 방의 관리자중 한명인가
	SIPNET::TServiceId		GetServiceId(T_FAMILYID _FID);			// 가족이 접속한 접속써비스 Id를 얻는다
	void			UpdateHisSpaceInfoFroEnter(T_FAMILYID _FID);
	void			SendAllInRoomToOne(T_FAMILYID _FID, SIPNET::TServiceId sid);			// 방안의 모든 정보를 보낸다
	//void			SendItemPartInRoomToOne(T_FAMILYID _FID);		
	void			SendDeceasedDataTo(T_FAMILYID _FID);			// 방의 고인정보를 내려보낸다
	//void            SendDeceasedHisDataTo(T_FAMILYID _FID);         // send the open room's dead history data to client
	//void			SendSacrifice(T_FAMILYID _FID);					// 제상이 제물목록을 내려보낸다
	void			SendTomb(T_FAMILYID _FID, SIPNET::TServiceId sid);						// 비석목록을 내려보낸다
	void			SendRuyibei(T_FAMILYID _FID, SIPNET::TServiceId sid);					// Ruyibei자료를 내려보낸다
	void			SetRuyibei(uint32 _itemid, T_COMMON_CONTENT _ruyibei);
	void			SendRuyibeiText(T_FAMILYID _FID, SIPNET::TServiceId sid);				// Ruyibei자료를 내려보낸다
	void			SendGoldFishInfo(T_FAMILYID _FID, SIPNET::TServiceId sid);
	void			SendLuckList(T_FAMILYID _FID, SIPNET::TServiceId sid);
	//void			SendPet(T_FAMILYID _FID, SIPNET::TServiceId sid);						// 페트목록을 내려보낸다
	//void			SendWish(T_FAMILYID _FID);						// 소원록을 내려보낸다
	//void			SendTGate(T_FAMILYID _FID);						// 신비한 문정보를 내려보낸다
	//void			RoomPartChange(T_FAMILYID _FID, T_ROOMPARTID _PartID, SIPNET::TServiceId _tsid);	// 방의 부분품을 교체한다
	//T_ROOMPARTID	GetRoomPartIDForPlace(T_ROOMPARTPOS _place);		// 방의 특정위치에 있는 부분품ID를 얻는다
	void			SetRoomInfo( T_FAMILYID	_FID, T_ROOMID	_RoomID, T_ROOMNAME	_RoomName, T_ROOMPOWERID _OpenProp, T_COMMON_CONTENT _RCom, std::string AuthKey);
	void			SetDeadInfo( const _entryDead *_info);
	//bool			ReloadDeadInfo();
	void			DeleteDeadInfo( uint8 _index );
	//void			DelRoomMemorial(T_FAMILYID _FID, T_COMMON_ID _memoid, SIPNET::TServiceId _sid);
	//void			ReqRoomMemorial(T_FAMILYID _FID, uint32 _PagePos, SIPNET::TServiceId _sid, uint32 year_month, uint8 SearchType = 0);
	//void			AddRoomSacrifice(T_FAMILYID _FID, uint8 _num, ROOMSACRIFICE *_info, SIPNET::TServiceId _sid);
	void			SetTomb(T_FAMILYID _FID, T_TOMBID _tombid, T_COMMON_CONTENT _tomb, SIPNET::TServiceId _sid);
	//void			GetOrderEvents(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	//void			AddEvent(T_FAMILYID _FID, T_ROOMEVENTTYPE _EventType, SIPNET::TServiceId _sid);
	//void			EndEvent(T_FAMILYID _FID, SIPNET::TServiceId _sid);
	void			ChangeRoomMaster_Step1(T_FAMILYID _FID, T_FAMILYID _NextId, std::string &password2, SIPNET::TServiceId _tsid);
	void			ChangeRoomMaster_Step2(T_FAMILYID _FID, T_FAMILYID _NextId, uint32 result);
	void			ChangeRoomMaster_Step3(T_FAMILYID _NextId, uint32 Answer, SIPNET::TServiceId _tsid);
	void			ChangeRoomMaster_Cancel(T_FAMILYID _FID, T_FAMILYID _NextId, SIPNET::TServiceId _tsid);
	//void			PutHoldedItem(T_FAMILYID _FID,  T_CHARACTERID _CHID, T_SACRIFICEX _X, T_SACRIFICEY _Y, SIPNET::TServiceId _sid);
	//void			PutItem(T_FAMILYID _FID,  T_CHARACTERID _CHID, T_FITEMID _ItemID, T_FITEMNUM _ItemNum, T_SACRIFICEX _X, T_SACRIFICEY _Y, SIPNET::TServiceId _sid);

	virtual void	on_M_CS_FISH_FOOD(T_FAMILYID _FID, uint32 StoreLockID, T_FITEMPOS InvenPos, T_ITEMSUBTYPEID _ItemID, uint32 FishScopeID, uint32 SyncCode, SIPNET::TServiceId _sid);
	virtual BOOL	IsChangeableFlower(T_FAMILYID FID);
	virtual void	on_M_CS_FLOWER_NEW(T_FAMILYID _FID, uint32 StoreLockID, T_FITEMPOS InvenPos, T_ITEMSUBTYPEID _ItemID, uint32 SyncCode, SIPNET::TServiceId _sid);

	void			GoldFishChange(T_FAMILYID _FID, uint32 StoreLockID, T_FITEMPOS InvenPos, uint32 GoldFishScopeID, uint8 GoldFishIndex, uint32 SyncCode, SIPNET::TServiceId _sid);
	void			ChangeGoldFish(uint32 GoldFishScopeID);
	//void			SaveAndSendToAllFishData(uint32 FID, uint32 FishScopeID, uint32 FishFoodID);

	void			SendPeacockType(T_FAMILYID FID, SIPNET::TServiceId sid);

	void			on_M_CS_LUCK_REQ(T_FAMILYID FID, SIPNET::TServiceId _tsid);
//	void			cb_M_NT_LUCK4_RES(bool bPlay, T_FAMILYID FID, T_FAMILYNAME	FName);
	void			cb_M_NT_LUCK4_CONFIRM(bool bPlay, T_ROOMID RoomID, T_FAMILYID FID, T_FAMILYNAME FName, uint32 LuckID);

	//void			BurnItem(T_FAMILYID _FID,  T_FITEMPOS _Pos, T_FITEMNUM _Num, SIPNET::TServiceId _sid);
	//void			BurnItemInstant(T_FAMILYID _FID,  T_ITEMSUBTYPEID _ItemID, T_FITEMNUM _Num, SIPNET::TServiceId _sid);
	
	//void			AddWish(T_FAMILYID _FID, T_WISHPOS _Pos, T_COMMON_CONTENT _Wish, bool _bOpen, SIPNET::TServiceId _sid);
	//void			DelWish(T_FAMILYID _FID,  T_WISHPOS _Pos, SIPNET::TServiceId _sid);
	//void			SetMusic(T_FAMILYID _FID,  T_ROOMMUSICID _MID, SIPNET::TServiceId _sid);
	void			EnterTGate(T_FAMILYID _FID,  SIPNET::TServiceId _sid);
	//void			ForceOut(T_FAMILYID _FID, T_FAMILYID _ObjectID, SIPNET::TServiceId _sid);
	//void			GetFamilyComment(T_FAMILYID _FID, T_FAMILYID _ObjectID, SIPNET::TServiceId _sid);
	//void			GiveRoomResult(T_FAMILYID _RecvID, bool _bRecv);
	void			SendRoomTermTimeToAll(T_DATETIME _termTime);
	//void			SendAllFamilyInRoom(T_FAMILYID _FID,  SIPNET::TServiceId _sid);

	//void			StartFirstEvent();
	//void			RefreshSacrifice();
	//void			RefreshOrderEvent();
	//void			RefreshPet();
	//void			RefreshWish();
	//void			RefreshTGate();
	//void			RefreshGiveProperty(); // Offline양도기능

	//void			SetRoomVirtue(T_ROOMVIRTUE _V);
	//T_F_LEVEL		SetDegreeFromVDays(T_FAMILYID _FID, T_F_VISITDAYS _vdays);
	void			SetFamilyComment(T_FAMILYID _FID, ucstring _ucComment);
	T_ROOMNAME		GetRoomNameForLog();

	void			PullFamily(T_FAMILYID _GMID, T_FAMILYID _FID, T_CH_DIRECTION _Dir, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z);

	//void			ReqVisitList(T_FAMILYID FID, uint32 year, uint32 month, uint32 page, SIPNET::TServiceId _tsid, uint8 bOnlyOwnData);
	//void			ReqActionMemo(T_FAMILYID FID, uint32 group_id, SIPNET::TServiceId _tsid);
	uint32			AddActionMemo(T_FAMILYID FID, uint8 action_type, uint32 ParentID, ucstring pray, uint8 secret, uint32 StoreLockID, uint16* invenpos, T_ITEMSUBTYPEID* _itemid, uint32 SyncCode, SIPNET::TServiceId _tsid);

	//void			GetLetterList(T_FAMILYID FID, uint32 year, uint32 month, SIPNET::TServiceId _tsid);
	//void			GetLetterContent(T_FAMILYID FID, uint32 letter_id, SIPNET::TServiceId _tsid);
	//void			AddNewLetter(T_FAMILYID FID, ucstring title, ucstring content, uint8 opened, SIPNET::TServiceId _tsid);
	bool			DelLetter(T_FAMILYID FID, uint32 letter_id);
	void			ModifyLetter(T_FAMILYID FID, uint32 letter_id, ucstring title, ucstring content, uint8 opened, SIPNET::TServiceId _tsid);
	void			ModifyLetterStart(T_FAMILYID FID, uint32 letter_id);
	void			ModifyLetterEnd(T_FAMILYID FID, uint32 letter_id);

	// For Activity
	void			RefreshActivity();
	void			SendActivityList(T_FAMILYID FID, SIPNET::TServiceId _tsid);
	void			CompleteActivity(uint32 ChannelID, int index, uint8 SuccessFlag);
	int				GetActivitingIndexOfUser(uint32 ChannelID, T_FAMILYID FID, bool bHostOnly = true);
	int				GetWaitingActivity(uint32 ChannelID, T_FAMILYID FID);
//	int				GetNpcidOfAct(uint32 ChannelID, int index, std::string &NPCName);
	void			on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid);
	void			ChackActPosForActWaitCancel(T_FAMILYID FID, uint8 &ActPos);
	void			ActWaitCancel(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid, uint8 SuccessFlag, uint32	ChannelID = 0, bool bUnconditional = false);
	void			ActRelease(T_FAMILYID FID, SIPNET::TServiceId _tsid);
	void			ActStart(T_FAMILYID FID, uint8 ActPos, uint32 Param, SIPNET::TServiceId _tsid, _ActivityStepParam* pYisiParam);// 2014/06/11

	_RoomActivity_Family*	FindJoinFIDStatus(uint32 ChannelID, T_FAMILYID JoinFID);
	_RoomActivity_Family*	FindJoinFID(uint32 ChannelID, uint8 ActPos, T_FAMILYID JoinFID, uint32* pRemainCount = NULL);
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
	virtual void	on_M_CS_MOUNT_LUCKANIMAL(T_FAMILYID HostFID, T_ANIMALID AID, SIPNET::TServiceId _tsid);
	virtual void	on_M_CS_UNMOUNT_LUCKANIMAL(T_FAMILYID HostFID, SIPNET::TServiceId _tsid);
	void	CheckMultiactAllStarted(uint32 ChannelID, uint8 ActPos);
	// For Exp
	virtual int		ChangeRoomExp(uint8 RoomExpType, T_FAMILYID FID, uint32 UMoney = 0, uint32 GMoney = 0, ucstring itemInfo = "");
	virtual bool	CheckIfActSave(T_FAMILYID FID);
	bool			SaveRoomLevelAndExp(bool bMustSave);

	virtual	bool	ProcCheckingRoomRequest(T_FAMILYID srcFID, T_FAMILYID targetFID, uint8 ContactStatus1, uint8 ContactStatus2);
	int				CanBanish(uint32 FID, uint32 targetFID);

	bool			IsAnyFamilyOnRoomlife();

	void			SendRoomChannelData(T_FAMILYID FID, SIPNET::TServiceId sid);

	void			SendActionResults(T_FAMILYID FID, SIPNET::TServiceId sid);
	void			SendXianbaoResultDataToAll();
	void			SendYishiResultDataToAll();
	void			SendAuto2ResultDataTo(T_FAMILYID FID, SIPNET::TServiceId sid);
	void			SendAncestorJisiResultDataToAll();
	void			SendAncestorDeceasedXiangResultDataToAll(int type);
	void			SendAncestorDeceasedHuaResultDataToAll(int type);

	void			SendAncestorData(T_FAMILYID FID, SIPNET::TServiceId sid);
	void			SendAncestorTextToAll();
	void			SendAncestorDeceasedToAll(int i);
	void			on_M_CS_ANCESTOR_TEXT_SET(T_FAMILYID FID, SIPNET::CMessage &_msgin);
	void			on_M_CS_ANCESTOR_DECEASED_SET(T_FAMILYID FID, SIPNET::CMessage &_msgin);

	void			AddAutoplayActionMemo(T_FAMILYID FID, uint32 autoplay_key);
	void			AutoPlayRequest(T_FAMILYID AutoFID, uint8 ActPos, T_FAMILYID FID, SIPNET::TServiceId _tsid);
	void			AutoPlayStart(T_FAMILYID AutoFID, uint8 ActPos, T_FAMILYID FID, T_FAMILYNAME FamilyName, 
						T_MODELID ModelID, T_DRESS DressID, T_FACEID FaceID, T_HAIRID HairID, SIPNET::TServiceId _tsid);
	void			AutoPlayEnd(T_FAMILYID AutoFID, uint8 ActPos);
	void			ChangeAutoCharSTATECH(T_FAMILYID AutoFID, uint32 X, uint32 Y, uint32 Z, uint32 Dir, ucstring AniName, uint32 AniState, uint32 HoldItem);
	//void			ChangeAutoCharCHMETHOD(T_FAMILYID AutoFID, ucstring MethodName, uint32 Param);
	//void			ChangeAutoCharHOLDITEM(T_FAMILYID AutoFID, uint32 ItemID);
	void			ChangeAutoCharMOVECH(T_FAMILYID AutoFID, uint32 X, uint32 Y, uint32 Z);

	void			SaveRoomStore();
	void			SaveRoomStoreHistory(uint8 flag, T_FAMILYID FID, uint8 Protected, ucstring Comment, uint8 Count, uint32 *pItemIDs, uint8 *pItemCount);
	//BOOL			CheckRoomStoreUseable(uint8 Count, uint32 *pItemIDs, uint8 *pItemCount);
	void			GetRoomStore(T_FAMILYID _FID, uint32 StoreSyncCode, SIPNET::TServiceId _sid);
	void			AddRoomStore(T_FAMILYID FID, uint32 InvenSyncCode, uint8 Protected, ucstring Comment, uint8 Count, uint16 *pInvenPos, uint8 *pItemCount, SIPNET::TServiceId _sid);
	void			GetRoomStoreHistory(T_FAMILYID FID, uint16 Year, uint8 Month, uint8 Flag, uint8 bOwnDataOnly, uint32 Page, SIPNET::TServiceId _sid);
	void			GetRoomStoreHistoryDetail(T_FAMILYID FID, uint32 ListID, SIPNET::TServiceId _sid);
	void			LockRoomStoreItems(T_FAMILYID FID, uint8 Count, uint32* pItemIDs, SIPNET::TServiceId _sid);
	void			UnlockRoomStoreItems(T_FAMILYID FID, uint32 StoreLockID);
	void			RoomStoreItemUsed(T_FAMILYID FID, uint32 StoreLockID);
	RoomStoreLockData*	GetStoreLockData(T_FAMILYID FID, uint32 StoreLockID);
	void			SetRoomStoreLastItem(T_FAMILYID FID, uint8 Count, uint32* LastItemID);
	void			SendRoomstoreStatus();
	void			SendRoomstoreStatus(T_FAMILYID FID, SIPNET::TServiceId _sid);
	void			GetRoomStoreItems(T_FAMILYID FID, uint8 Count, uint16 *pInvenPoses, uint32* pItemIDs, uint16 *pItemCounts, SIPNET::TServiceId _sid);

	void			SetRoomManagerPermission(T_FAMILYID FID, T_FAMILYID targetFID, uint8 Permission);			// 방관리자의 권한을 변경한다.
	void			SetFamilyNameForOpenRoom(T_FAMILYID FID, T_FAMILYNAME FamilyName);

	// 깜짝이벤트를 설정
	virtual void RefreshRoomEvent();
	virtual void StartRoomEvent(int i);

	void			NotifyChangeRoomInfoForManager();
	// Master Data Values

	void			on_M_CS_ACT_STEP(T_FAMILYID FID, uint8 ActPos, uint32 StepID);// 14/06/27

	// 예약의식
	uint32	YuyueYishiRegActionMemo(T_FAMILYID FID, T_FAMILYNAME FamilyName, uint32 YangyuItemID, uint32 CountPerDay, uint32 nCurActionGroup);
	int		FindNextBlesslampPos(uint32 ChannelID);

protected:
	virtual void	OutRoom_Spec(T_FAMILYID _FID);
	virtual int		CheckEnterWaitFamily_Spec();
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

	bool IsValidMultiActPos(uint8 ActPos)
	{
		if ((ActPos != ACTPOS_YISHI) && (ActPos != ACTPOS_ANCESTOR_JISI) && (ActPos != ACTPOS_MULTILANTERN))
			return false;
		return true;
	}

public:
	CRoomWorld() : CChannelWorld(WORLD_ROOM), m_CurGiveID(0)//, m_LiuyanDay(0)
	{ 
		m_nDefaultChannelCount = 1;
		m_HisSpaceCapacity = 0;
		m_HisSpaceUsed = 0;
		m_FreeFlag = 0;
		m_PhotoType = 0;

		m_bRoomExpChanged = false;
		m_dwRoomExpSavedTime = 0;
	}
	virtual ~CRoomWorld();

private:
	TmapRoomChannelData::iterator m_mapChannels_it;
public:
	RoomChannelData* GetFirstChannel()
	{
		m_mapChannels_it = m_mapChannels.begin();
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	};
	RoomChannelData* GetNextChannel()
	{
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		m_mapChannels_it ++;
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	}
	RoomChannelData* RemoveChannel()
	{
		m_mapChannels_it = m_mapChannels.erase(m_mapChannels_it);
		if(m_mapChannels_it == m_mapChannels.end())
			return NULL;
		return &m_mapChannels_it->second;
	}
	virtual void CreateChannel(uint32 channelid);
};

enum	ENUM_CELL_OBJECT {
	CELLOBJ_NONE,
	CELLOBJ_ANIMAL,
	//CELLOBJ_WISH,
};

//enum CREATE_ROOM_REASON
//{
//	CREATE_ROOM_REASON_ENTER = 1,
//	CREATE_ROOM_REASON_CHANGEMASTER_REQUEST,
//	CREATE_ROOM_REASON_CHANGEMASTER_CANCEL,
//	CREATE_ROOM_REASON_CHANGEMASTER_ANSWER,
//	CREATE_ROOM_REASON_AUTOPLAY,
//};

//extern CRoomWorld *	CreateRoomWorld(T_ROOMID _rid);
extern void			CreateRoomWorld(T_ROOMID RoomID, uint32 NextType, uint32 nParam1, uint32 nParam2, uint16 client_sid, std::string sParam3 = "");
extern void			DeleteRoomWorld(TID _roomid);
extern CWorld *		GetWorldFromFID(T_FAMILYID _FID);
extern void			RefreshRoom();
extern void			DoEverydayWork();

extern	void	cb_M_CS_REQEROOM_Oroom(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_CHECK_ENTERROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_CS_PROLONG_PASS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_BANISH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_MANAGER_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MANAGER_ANS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MANAGER_DEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MANAGER_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_MANAGER_DEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MANAGER_SET_PERMISSION(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_CS_ALLINROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_CS_ROOMPARTCHANGE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_SETROOMINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_SETDEAD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_SET_DEAD_FIGURE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_DECEASED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_NEWRMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_MODIFYRMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_DELRMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_REQRMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_MODIFY_MEMO_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_MODIFY_MEMO_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ROOMMEMO_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_REQRVISITLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ADDACTIONMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_VISIT_DETAIL_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_VISIT_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_VISIT_DATA_SHIELD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_VISIT_DATA_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_VISIT_DATA_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_GET_ROOM_VISIT_FID(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_REQ_PAPER_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_REQ_PAPER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_NEW_PAPER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_DEL_PAPER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MODIFY_PAPER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MODIFY_PAPER_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MODIFY_PAPER_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void	cb_M_GMCS_GET_GMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_CS_NEWSACRIFICE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_TOMBSTONE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_RUYIBEI_TEXT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_EVENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ADDEVENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ENDEVENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_PUTHOLEDITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_PUTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_GOLDFISH_CHANGE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_NEWPET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_CAREPET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_LUCK_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_LUCK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_LUCKHIS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_NT_LUCK4_RES(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_LUCK4_CONFIRM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_ANIMAL_APPROACH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ANIMAL_STOP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ANIMAL_SELECT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ANIMAL_SELECT_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_CS_BURNITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_BURNITEMINS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ROOMMUSIC(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ENTERTGATE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_RENEWROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ROOMCARD_RENEW(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_FORCEOUT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMFORFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ROOMFAMILYCOMMENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_GETALLINROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_PROMOTION_ROOM_Oroom(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_GET_PROMOTION_PRICE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_CS_ROOMCARD_CHECK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void    cb_M_MS_ROOMCARD_CHECK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);


//extern	void	cb_M_GIVEROOMRESULT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_GMCS_ACTIVE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_GMCS_SHOW(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_GMCS_PULLFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_GMCS_OUTFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void		cb_M_GMCS_GIVEITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void		cb_M_GMCS_GETITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void		cb_M_GMCS_GETINVENEMPTY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void		cb_M_GMCS_LISTCHANNEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void		cb_M_NT_ROOM_RENEW(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void		cb_M_NT_RELOAD_ROOMINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_ANCESTOR_TEXT_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ANCESTOR_DECEASED_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);


// AutoPlay기능
extern	void	cb_M_CS_AUTOPLAY_REQ_BEGIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_AUTOPLAY_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_ADDACTIONMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_AUTOPLAY_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_AUTOPLAY_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_AUTOPLAY_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_NT_AUTOPLAY_STATECH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_NT_AUTOPLAY_CHMETHOD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_NT_AUTOPLAY_HOLDITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_NT_AUTOPLAY_MOVECH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_AUTOPLAY2_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_AUTOPLAY3_REGISTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_NT_ROOM_CREATE_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	cb_M_CS_GET_ROOMSTORE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ADD_ROOMSTORE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_GET_ROOMSTORE_HISTORY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_GET_ROOMSTORE_HISTORY_DETAIL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMSTORE_LOCK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMSTORE_UNLOCK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMSTORE_LASTITEM_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMSTORE_HISTORY_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMSTORE_HISTORY_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMSTORE_HISTORY_SHIELD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_ROOMSTORE_GETITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern SIPBASE::TTime	GetDBTimeSecond();
extern uint32			GetDBTimeMinute();
extern uint32	getDaysSince1970(std::string& date);
extern uint32	getMinutesSince1970(std::string& date);
extern uint32	getSecondsSince1970(std::string& date);
extern void		sendErrorResult(const MsgNameType &_name, T_FAMILYID _fid, uint32 _flag, SIPNET::TServiceId _tsid);

//extern void		ForceEnterRoom(uint32 ChannelID, T_FAMILYID FID, SIPNET::TServiceId _tsid, T_ROOMID	RoomID = 1, T_CHARACTERID	CHID = 0);
//extern void		SendOwnDeadMemorialDaysInfo(T_FAMILYID familyId, SIPNET::TServiceId _tsid);

extern uint16 GetRoomPriceIDFromCardType(uint16 CardType);

extern void SendChit(uint32 srcFID, uint32 targetFID, uint8 ChitType, uint32 p1, uint32 p2, ucstring &p3, ucstring &p4);

void	DoEverydayWorkInRoom();

// 방양도
//void SendReceiveRooms(T_FAMILYID _FID, SIPNET::TServiceId _tsid);
void cb_M_CS_CHANGEMASTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_CHANGEMASTER_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_CHANGEMASTER_ANS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_CHANGEMASTER_FORCE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

CRoomWorld* GetOpenRoomWorld(T_ROOMID _rid);

#endif //OPENROOM_BASE_H
