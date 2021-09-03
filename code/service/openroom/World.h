#pragma once

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "misc/db_interface.h"

#include "../Common/QuerySetForShard.h"
#include "Character.h"
#include "Family.h"
#include "mst_room.h"
#include "misc/vector.h"

void	init_WorldService();
void	update_WorldService();
void	release_WorldService();

typedef	std::map<T_FAMILYID, T_FAMILYID>			MAP_FAMILYID;
typedef std::list<T_FAMILYID>						TFamilyIds;
typedef	std::map<T_FAMILYID, INFO_FAMILYINROOM>		TRoomFamilys;
typedef	std::map<SIPNET::TServiceId, MAP_FAMILYID>	TFesFamilyIds;

#define FOR_START_FOR_FES_ALL(FesFamilys, msgOut, mesid)							\
	TFesFamilyIds::iterator it_FOR_FES_ALL;														\
	for (it_FOR_FES_ALL = FesFamilys.begin(); it_FOR_FES_ALL != FesFamilys.end(); it_FOR_FES_ALL++)						\
	{																				\
		if (it_FOR_FES_ALL->second.size() == 0)	continue;										\
		SIPNET::CMessage	msgOut(mesid);													\
		MAP_FAMILYID::iterator f;													\
		for (f = it_FOR_FES_ALL->second.begin(); f != it_FOR_FES_ALL->second.end(); f++)					\
		{																			\
			T_FAMILYID fid = f->first;												\
			msgOut.serial(fid);														\
		}																			\
		uint32 uZero = 0;															\
		msgOut.serial(uZero);

#define FOR_END_FOR_FES_ALL()														\
		SIPNET::CUnifiedNetwork::getInstance ()->send(it_FOR_FES_ALL->first, msgOut);					\
	}																				\

extern	uint32	GetDBTimeDays();

class CWorld;
typedef		void (*EnterFamilyInWorldCallback)(T_FAMILYID FID, CWorld* pWorld, uint32 param);
class CWorld : public SIPBASE::CRefCount
{
public:
	TWorldType			m_WorldType;
	T_ROOMID			m_RoomID;
	T_ROOMNAME			m_RoomName;						// πÊ¿Ã∏ß

	TRoomFamilys		m_mapFamilys;

	// Family list connected to frontend service
	TFesFamilyIds		m_mapFesFamilys;

protected:
	// For Visit List of a room, this is used for OpenRoom & Family's Exp
	uint32		VisHis_Day;
//	uint32		VisHis_Count;
//	uint32		VisHis_IDList[MAX_ROOM_VISIT_HIS_COUNT];
	std::map<uint32, uint32>	VisHis_IDList;

public:
	bool	IsEmptyWorld()	{ return m_mapFamilys.empty(); }

	//void	EnterFamilyInWorld(T_FAMILYID FID);
	void	EnterFamilyInWorld(T_FAMILYID FID, SIPNET::TServiceId tsid, EnterFamilyInWorldCallback callback, uint32 param);
	void	EnterFamilyInWorld(T_FAMILYID FID, SIPNET::TServiceId tsid, void *resSet, EnterFamilyInWorldCallback callback, uint32 param);
	void	LeaveFamilyInWorld(T_FAMILYID FID);

	// For Family Inven
	_InvenItems* GetFamilyInven(T_FAMILYID FID);

	// For EXP & LEVEL
	bool	CheckIfRoomExpIncWhenFamilyEnter(uint32 FID);
	bool	CheckIfFamilyExpIncWhenFamilyEnter(uint32 FID);
	void	FamilyEnteredForExp(uint32 FID);
//	int		ChangeFamilyExp(uint8 FamilyExpType, T_FAMILYID FID, ucstring ItemInfo = "", uint32 ExpParam = 0, bool bSaveDB = true);
	void	ChangeFamilyExp(uint8 FamilyExpType, T_FAMILYID FID, uint32 ExpParam = 0);

	// For Chat
	virtual void	SendChatCommon(T_FAMILYID _FID, T_CHATTEXT _Chat);	// Normal Chatting
	virtual void	SendChatRoom(T_FAMILYID _FID, T_CHATTEXT _Chat);	// Room Chatting

	// 
	INFO_FAMILYINROOM* GetFamilyInfo(T_FAMILYID FID);

	void			SetCharacterState(T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_DIRECTION _Dir, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z, ucstring _AniName, T_CH_STATE _State, T_FITEMID _HoldItem);
	void			SetCharacterPos(T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z);

	uint32			FamilyItemUsed(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, uint8 ItemType, SIPNET::TServiceId _sid);

	void			on_M_CS_LARGEACT_USEITEM(T_FAMILYID FID, uint16 invenpos, uint32 synccode, SIPNET::TServiceId _sid);
	
public:
	CWorld(TWorldType WorldType);
	virtual ~CWorld();

protected:
	// For Family Inven
	//bool	ReadFamilyInfo(T_FAMILYID FID);
	bool	SaveFamilyInfo(T_FAMILYID FID);
};

//bool SaveFamilyLevelAndExp(INFO_FAMILYINROOM &FamilyInfo);
//bool LoadFamilyExpHistory(INFO_FAMILYINROOM &FamilyInfo);
//bool SaveFamilyExpHistory(INFO_FAMILYINROOM &FamilyInfo);
//int ChangeFamilyExp(uint8 FamilyExpType, T_FAMILYID FID, ucstring ItemInfo = "", uint32 ExpParam = 0, bool bSaveDB = true);
void ChangeFamilyExp(uint8 FamilyExpType, T_FAMILYID FID, uint32 ExpParam = 0);

//extern	DBCONNECTION	DB;

//bool ReadFamilyInven(T_FAMILYID FID, _InvenItems *pInven);
bool SaveFamilyInven(T_FAMILYID FID, _InvenItems *pInven, bool bDBNormal = true);

extern CWorld*	GetWorldFromFID(T_FAMILYID _FID);

extern void		SendFamilyProperty_Level(SIPNET::TServiceId _sid, T_FAMILYID _FID, T_F_EXP _exp, T_F_LEVEL _level);
extern void		SendFamilyProperty_Money(SIPNET::TServiceId _sid, T_FAMILYID _FID, T_PRICE _GMoney, T_PRICE _UMoney);

extern	void	RemoveFamily(T_FAMILYID FID);
extern	void	InitBroker();
extern	void	UpdateBroker();
extern	void	ReleaseBroker();

extern	SIPBASE::MSSQLTIME		ServerStartTime_DB;
extern	SIPBASE::TTime			ServerStartTime_Local;
extern	ucstring				OutDataRootPath;

void	cb_M_NT_LOGIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_NT_CHANGE_NAME(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_W_SHARDCODE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_CHATCOMMON(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_CHATROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_MANAGEROOMS_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void	cb_M_CS_RECOMMEND_ROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	cb_M_CS_LARGEACT_USEITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_CS_CHECKIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_NT_RESCHECKIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	cb_M_CS_REQ_MST_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void	LoadHolidayList();

extern	bool	LoadOutData();
extern	void	CheckTodayHoliday();
extern	bool	g_bIsHoliday;

void	cb_M_NT_BLESSCARD_USED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void ProcRemoveFamilyForChannelWorld(T_FAMILYID FID);

extern bool IsThisShardRoom(uint32 RoomID);
extern bool IsThisShardFID(uint32 FID);

void	cb_M_NT_AUTOPLAY3_EXP_ADD_ONLINE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	cb_M_NT_AUTOPLAY3_EXP_ADD_OFFLINE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
