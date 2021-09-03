
#ifndef MST_ROOM_H
#define MST_ROOM_H

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../../common/common.h"
#include "../Common/Common.h"
#include "mst_item.h"
/*
// ?????
struct  MST_ROOMPARTS
{
	T_ITEMSUBTYPEID							m_ItemSubtypeId;			// ??Id
	T_COMMON_NAME							m_Name;						// ?????
	T_PRICE									m_GMoney;					// ???
	T_PRICE									m_Money;					// ??
	T_F_EXP									m_ExpInc;					// ???????
	T_F_VIRTUE								m_VirtueInc;				// ???????
	T_F_FAME								m_FameInc;					// ???????
	T_ROOMEXP								m_RoomExpInc;				// ??????
	T_ROOMVIRTUE							m_RoomVirtueInc;			// ??????
};
typedef		std::map<T_ITEMSUBTYPEID,	MST_ROOMPARTS>					MAP_MST_ROOMPARTS;
extern		MAP_MST_ROOMPARTS				map_mstRoomParts;
*/
// ??????????
struct ROOMPARTINFOALL 
{
	T_ROOMPARTID							m_PartId;					// ??Id(????? ????)
	MST_ITEM								m_ItemSubtypeInfo;			// ???????
	T_ROOMPARTPOS							m_Pos;						// ??
	T_ROOMLEVEL								m_NeedDegree;				// ?????
};

// 
struct MST_CONFIG
{
	std::string				Name;
	uint32					ValueINT;
	MST_CONFIG(std::string _Name, uint32 _ValueINT) : Name(_Name), ValueINT(_ValueINT)	{}
};

typedef		std::map<std::string, MST_CONFIG>					MAP_MST_CONFIG;
extern		MAP_MST_CONFIG				map_mstConfig;

extern	MST_CONFIG* GetMstConfigData(std::string Name);

// ??????
struct MST_ROOM 
{
	T_MSTROOM_ID							m_ID;						// dbid
	T_MSTROOM_ID							m_SceneID;					// ??id
	T_MSTROOM_NAME							m_Name;
	T_PRICE									m_Price;
	T_MSTROOM_YEARS							m_Years;
	T_MSTROOM_MONTHS						m_Months;
	T_MSTROOM_DAYS							m_Days;
	T_PRICEDISC								m_DiscPercent;
	uint32									m_HisSpace;
	uint16									m_CardType;
	T_PRICE		GetRealPrice()				const
	{
		return (m_Price * m_DiscPercent) / 100;
	}
};

typedef		std::map<T_MSTROOM_ID,	MST_ROOM>					MAP_MST_ROOM;
extern		MAP_MST_ROOM				map_mstRoom;

/////////////////////////////////////////////////////////////
//          For Family & Room Exp Data

// Type list that increase a room's Exp
enum _Room_Exp_Type
{
	Room_Exp_Type_UseMoney = 0,
	Room_Exp_Type_Visitroom = 1,
	Room_Exp_Type_Jisi_One = 2,
	Room_Exp_Type_Jisi_Many = 3,
	Room_Exp_Type_Letter = 4,
	Room_Exp_Type_Xianbao = 5,
	Room_Exp_Type_Memo = 6,
	Room_Exp_Type_Flower_Set = 7,
	Room_Exp_Type_Flower_Water = 8,
	Room_Exp_Type_Fish = 9,
	Room_Exp_Type_ChangeFish = 10,
	Room_Exp_Type_Auto2 = 11,
	ROOM_EXP_TYPE_MAX = 12
};

struct _Mst_Room_Exp_Type
{
	uint8		TypeID;
	T_ROOMEXP	Exp;
	uint8		MaxCount;
};
extern	_Mst_Room_Exp_Type	mstRoomExp[ROOM_EXP_TYPE_MAX];

// Room Level List
struct _Mst_Room_Level
{
	T_ROOMLEVEL	Level;
	T_ROOMEXP	Exp;
	uint8		ShowLevel;
};
extern	_Mst_Room_Level	mstRoomLevel[ROOM_LEVEL_MAX + 1];

// Type list that increase a family's Exp
// log에서 리용되기때문에 ID를 변경시킬수 없다!!!!!
enum _Family_Exp_Type
{
	// Family_Exp_Type_UseMoney = 0, -- No use
	Family_Exp_Type_Visitday = 1,
	Family_Exp_Type_Jisi_One = 2,
	Family_Exp_Type_Jisi_One_Guest = 3,
	Family_Exp_Type_Jisi_Large_Master = 4,
	Family_Exp_Type_Jisi_Large_Guest = 5,
	Family_Exp_Type_Libai = 6,
	Family_Exp_Type_Xianbao = 7,
	Family_Exp_Type_Memo = 8,
	Family_Exp_Type_Letter = 9,
	Family_Exp_Type_Flower_Set = 10,
	Family_Exp_Type_Flower_Water = 11,
	Family_Exp_Type_Fish = 12,
	Family_Exp_Type_Wish = 13,
	Family_Exp_Type_Visitroom = 14,
	Family_Exp_Type_ChangeFish = 15,
	Family_Exp_Type_Blesscard = 16,
	Family_Exp_Type_Auto2 = 17,
	Family_Exp_Type_Daxingyishi = 18,
	Family_Exp_Type_SBY = Family_Exp_Type_Daxingyishi,
	Family_Exp_Type_ReligionYiSiMaster = 19,
	Family_Exp_Type_ReligionYiSiSlaver = 20,
	Family_Exp_Type_Shenming = 21,
	Family_Exp_Type_ShenMiDongWu = 22,
	Family_Exp_Type_UseMusicTool = 23,
	Family_Exp_Type_LanternMaster = 24,
	Family_Exp_Type_LanternSlaver = 25,
	Family_Exp_Type_XingLi = 26,
	Family_Exp_Type_MusicStart = 27,
	Family_Exp_Type_SelectAnimal = 28,
	Family_Exp_Type_NianFo = 29,
	Family_Exp_Type_ReligionPuTongXiang = 30,
	Family_Exp_Type_ReligionTouXiang = 31,
	Family_Exp_Type_NeiNianFo = 32,
	FAMILY_EXP_TYPE_MAX = 33,
};

struct _Mst_Family_Exp_Type
{
	uint8		TypeID;
	T_F_EXP		Exp;
	uint8		MaxCount;
};
extern	_Mst_Family_Exp_Type	mstFamilyExp[FAMILY_EXP_TYPE_MAX];

// Family Level List
struct _Mst_Family_Level
{
	T_F_LEVEL	Level;
	T_F_EXP		Exp;
	uint32		AddPoint;
};
extern	_Mst_Family_Level	mstFamilyLevel[FAMILY_LEVEL_MAX + 1];

// Fish Level List
struct _Mst_Fish_Level
{
	uint8		Level;
	uint32		Exp;
};
extern	_Mst_Fish_Level		mstFishLevel[FISH_LEVEL_MAX + 1];
extern uint32 GetFishLevelFromExp(uint32 exp);

// Type list that increase a family's Virtue in ReligionRoom 0
enum _Family_Virtue0_Type
{
	Family_Virtue0_Type_UseMoney = 0,
	Family_Virtue0_Type_UsePoint = 1,
	Family_Virtue0_Type_Visit = 2,
	Family_Virtue0_Type_Jisi = 3,
	Family_Virtue0_Type_Wish = 4,
	Family_Virtue0_Type_Letter = 5,
	FAMILY_VIRTUE0_TYPE_MAX = 6
};

#define FAMILY_VIRTUE_TYPE_MAX	FAMILY_VIRTUE0_TYPE_MAX

struct _Mst_Family_Virtue_Type
{
	uint8		TypeID;
	uint32		Virtue;
	uint8		MaxCount;
};
extern	_Mst_Family_Virtue_Type	mstFamilyVirtue0[FAMILY_VIRTUE0_TYPE_MAX];

//uint16 CalcRoomExpPercent(T_ROOMLEVEL level, T_ROOMEXP exp);
//uint16 CalcFamilyExpPercent(T_F_LEVEL level, T_F_EXP exp, uint32 *pMaxExp = NULL);
//uint16 CalcFishExpPercent(uint8 level, uint32 exp);

/////////////////////////////////////////////////////////////


extern	MST_ROOM	*GetMstRoomData(T_MSTROOM_ID PriceID);
extern	MST_ROOM	*GetMstRoomData_FromSceneID(uint16 SceneID, T_MSTROOM_YEARS _years, T_MSTROOM_MONTHS _months, T_MSTROOM_DAYS _days);
extern	MST_ROOM	*GetMstRoomData_FromSceneID_FreeRoom(uint16 SceneID);
extern	void		LoadDBMstData();
extern	void		SendMstRoomData(T_FAMILYID	_FID, SIPNET::TServiceId _tsid);
//extern	bool		GetRoomPartInfoAll(T_MSTROOM_ID _roomid, T_ROOMPARTID _partid, ROOMPARTINFOALL *_buffer);

// Master data structure of history space
struct MST_HISSPACE
{
	MST_HISSPACE(T_COMMON_ID _ID, T_HISSPACE _HisSpace, T_PRICE _GMoney, T_PRICEDISC _DiscGMoney, T_PRICE _UMoney, T_PRICEDISC _DiscUMoney, uint32 _AddPoint)
		: m_ID(_ID), m_HisSpace(_HisSpace), m_GMoney(_GMoney), m_DiscGMoney(_DiscGMoney), m_UMoney(_UMoney), m_DiscUMoney(_DiscUMoney), m_AddPoint(_AddPoint) {}
	T_COMMON_ID								m_ID;						// dbid
	T_HISSPACE								m_HisSpace;					// usable space(M)
	T_PRICE									m_GMoney;
	T_PRICEDISC								m_DiscGMoney;
	T_PRICE									m_UMoney;
	T_PRICEDISC								m_DiscUMoney;
	uint32									m_AddPoint;
};
typedef		std::map<T_COMMON_ID,	MST_HISSPACE>		MAP_MST_HISSPACE;
extern		MAP_MST_HISSPACE							map_mstHisSpace;

typedef		std::map<T_COMMON_ID,	PRICE_T>			MAP_MST_PRICES;
//extern		MAP_MST_PRICES								map_mstOtherPrices;

void			SendMstHisSpaceData(T_FAMILYID	_FID, SIPNET::TServiceId _tsid);
MST_HISSPACE	*GetMstHisSpace(T_COMMON_ID _ID);

void			SendMstRoomLevelExpData(T_FAMILYID	_FID, SIPNET::TServiceId _tsid);
void			SendMstFamilyLevelExpData(T_FAMILYID	_FID, SIPNET::TServiceId _tsid);

//extern	void	SendMstExpRoom(T_FAMILYID	_FID, SIPNET::TServiceId _tsid);

void			SendMstConfigData(T_FAMILYID _FID, SIPNET::TServiceId _tsid);

// Master data structure of tbl_mstFish
struct MST_FISH
{
	MST_FISH(uint8 FishID, uint8 FishLevel, uint32* FishLevelExps)
		: m_FishID(FishID), m_FishLevel(FishLevel)
	{
		memcpy(m_FishLevelExps, FishLevelExps, sizeof(m_FishLevelExps));
	}

	uint8		m_FishID;
	uint8		m_FishLevel;
	uint32		m_FishLevelExps[FISH_LEVEL_MAX + 1];
};
typedef		std::map<uint8,	MST_FISH>		MAP_MST_FISH;
extern		MAP_MST_FISH					map_mstFish;
//MST_FISH *GetMstFish(uint8 FishID);
void SendMstFishData(T_FAMILYID _FID, SIPNET::TServiceId _tsid);

// Map에서 계속 참조하는것이 속도상 불리하기때문에 변수를 꺼내놓음.
extern	int		g_nFishExpReducePercent;
extern	uint32	g_nCharChangePrice;
extern	uint32	g_nRoomSmoothFamPercent;
extern	uint32	g_nRoomDeleteDay;
extern	uint32	g_nSamePromotionPrice;	// 동급천원승급시에만 적용되는 가격이다.
extern	uint32	g_nSamePromotionCount;	// 동급천원승급가능회수
//extern	uint32	g_nRoomcardBackRate;	// 천원카드사용시에 되돌려주는 자은비의 비률

extern	std::string			mstBadFIDList;

#endif // MST_ROOM_H
