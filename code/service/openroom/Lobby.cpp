#pragma warning (disable: 4267)
#pragma warning (disable: 4244)

#include "misc/types_sip.h"

#include <fcntl.h>
#include <sys/stat.h>

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#	include <direct.h>
#	undef max
#	undef min
#else
#	include <unistd.h>
#	include <errno.h>
#endif

#include <string>
#include <list>

#include "net/service.h"
#include "net/unified_network.h"
#include "net/naming_client.h"

#include "../../common/Packet.h"
#include "../../common/common.h"
#include "../Common/Common.h"
#include "../Common/DBLog.h"

#include "mst_room.h"
#include "outRoomKind.h"
#include "Lobby.h"
#include "Inven.h"

#include "misc/DBCaller.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

extern CDBCaller		*DBCaller;

uint8	ShardMainCode[25] = {0};

static  CLobbyWorld *       pLobby  = NULL;

bool IsThisShardRoom(uint32 RoomID)
{
	if (RoomID < 0x01000000)
		return true;

	uint8	ShardCode = (uint8)(RoomID >> 24);
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		if (ShardMainCode[i] == ShardCode)
			return true;
	}
	return false;
}

bool IsThisShardFID(uint32 FID)
{
	uint8	ShardCode = (uint8)((FID >> 24) + 1);
	//sipwarning("***** IsThisShardFID: FID=%d, ShardCode=%d", FID, ShardCode); //by krs
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		//sipwarning("+++++ IsThisShardFID: FID=%d, ShardMainCode=%d", FID, ShardMainCode[i]); //by krs
		if (ShardMainCode[i] == ShardCode)
			return true;
	}
	return false;
}

void EnterFamilyInLobby_Callback(T_FAMILYID FID, CWorld* pWorld, uint32 bReqItemRefresh)
{
	CLobbyWorld	*pLobby = dynamic_cast<CLobbyWorld*>(pWorld);
	if (pLobby == NULL)
	{
		sipwarning("dynamic_cast<CLobbyWorld*>(pWorld) == NULL");
		return;
	}
	pLobby->EnterFamilyInLobby_After(FID, bReqItemRefresh);
}

map<T_ACCOUNTID, uint32>		g_AddJifenInfo;
void cb_M_ADDEDJIFEN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_ACCOUNTID		uid;		_msgin.serial(uid);
	uint32			addJifen;	_msgin.serial(addJifen);

	map<T_ACCOUNTID, uint32>::iterator it = g_AddJifenInfo.find(uid);
	if (it == g_AddJifenInfo.end())
	{
		g_AddJifenInfo.insert(make_pair(uid, addJifen));
	}
	else
	{
		it->second += addJifen;
	}
}

void cb_M_NT_REMOVEJIFEN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_ACCOUNTID		uid;		_msgin.serial(uid);
	g_AddJifenInfo.erase(uid);
}

void CLobbyWorld::SendAddedJifen(T_FAMILYID _FID)
{
	FAMILY *	pFamily = GetFamilyData(_FID);
	if (pFamily == NULL)
		return;
	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(_FID);
	if (pFamilyInfo == NULL)
		return;

	map<T_ACCOUNTID, uint32>::iterator it = g_AddJifenInfo.find(pFamily->m_UserID);
	if (it == g_AddJifenInfo.end())
		return;

	uint32 addJifen = it->second;
	g_AddJifenInfo.erase(pFamily->m_UserID);

	CMessage msg(M_SC_ADDEDJIFEN);
	msg.serial(_FID, addJifen);
	CUnifiedNetwork::getInstance()->send(pFamilyInfo->m_ServiceID, msg);

	pFamilyInfo->m_nGMoney += addJifen;
	MAKEQUERY_SaveFamilyGMoney(strQ, _FID, pFamilyInfo->m_nGMoney);
	DBCaller->executeWithParam(strQ, NULL,
		"D",
		OUT_,			NULL);

	SendFamilyProperty_Money(pFamilyInfo->m_ServiceID, _FID, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);
	//sipinfo("Added Jifen: FID=%d, AddJifen=%d, TotalJifen=%d", _FID, addJifen, pFamilyInfo->m_nGMoney); byy krs

	{
		CMessage msg(M_NT_REMOVEJIFEN);
		msg.serial(pFamily->m_UserID);
		CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, msg);
	}
}

void CLobbyWorld::EnterFamilyInLobby_After(T_FAMILYID _FID, uint32 bReqItemRefresh)
{
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(_FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", _FID);
		return;
	}

	SIPNET::TServiceId _sid = pInfo->m_ServiceID;

	SetFamilyWorld(_FID, 0, WORLD_LOBBY);
	//sipdebug("m_mapFamilys Added. size=%d",m_mapFamilys.size()); byy krs

	// Change Exp
	ChangeFamilyExp(Family_Exp_Type_Visitday, _FID);

	// send permission to client that enter lobby
	CMessage msgout(M_SC_ENTER_LOBBY);
	uint32		code = ERR_NOERR;
	msgout.serial(_FID, code);
	CUnifiedNetwork::getInstance()->send(_sid, msgout);

	if (bReqItemRefresh)
	{
		ItemRefresh(_FID, _sid);

		//// Check if new mail exist - DB호출 처리 필요 - Main으로 이동
		//SendCheckMailboxResult(_FID);
	}
	else
	{
		// ManagerService에 통지
		{
			uint32	zero = 0;
			CMessage	msgOut(M_NT_PLAYER_ENTERROOM);
			msgOut.serial(_FID, zero, zero);
			CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
		}
	}

	// AddJifen
	SendAddedJifen(_FID);
}

void CLobbyWorld::EnterFamilyInLobby(T_FAMILYID _FID, SIPNET::TServiceId _sid, uint32 bReqItemRefresh)
{
	EnterFamilyInWorld(_FID, _sid, EnterFamilyInLobby_Callback, bReqItemRefresh);
}

void	CLobbyWorld::ch_leave(T_FAMILYID _FID)
{
	TRoomFamilys::iterator it2 = m_mapFamilys.find(_FID);
	if (it2 == m_mapFamilys.end())
		return;

	// Notice to CWorld
	LeaveFamilyInWorld(_FID);

	TServiceId sid = it2->second.m_ServiceID;

	// send permission to client that leave lobby
	CMessage msgout(M_SC_LEAVE_LOBBY);
	uint32		code = ERR_NOERR;
	msgout.serial(_FID, code);
	CUnifiedNetwork::getInstance()->send(sid, msgout);

	map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator itch;
	for (itch = it2->second.m_mapCH.begin(); itch != it2->second.m_mapCH.end(); itch++)
		::DelCharacterData(itch->first);

	TFesFamilyIds::iterator itFes = m_mapFesFamilys.find(sid);
	if (itFes != m_mapFesFamilys.end())
		itFes->second.erase(_FID);

	m_mapFamilys.erase(_FID);
	SetFamilyWorld(_FID, NOROOMID);

	// ManagerService에 통지
	{
		uint32	zero = 0;
		CMessage	msgOut(M_NT_PLAYER_LEAVEROOM);
		msgOut.serial(_FID, zero, zero);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	}

	//sipdebug("m_mapFamilys Deleted. size=%d", m_mapFamilys.size());
}

//static	void	cb_M_NT_LOGIN_Lobby(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;
//	T_ACCOUNTID		UserID;
//	T_FAMILYNAME	FName;
//	ucstring		UserName;
//	uint8			uUserType;
//	bool			bGM;	
//	string			strIP;
//
//	_msgin.serial(FID, UserID, FName, UserName, uUserType, bGM, strIP);
//
//	if (map_family.find(FID) == map_family.end())
//	{
//		AddFamilyData(FID, UserID, FName, UserName, uUserType, bGM, strIP, CTime::getLocalTime());
//	}
//
//	SetFesForFamilyID(FID, _tsid);	
//}

static void cb_M_CS_ENTER_LOBBY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);		
		
	sipinfo("FID=%d", FID);

	if (pLobby == NULL)
	{
		sipwarning("the lobby service isn't ready!");
		return;
	}	

	TWorldType	type;
	T_ROOMID rid = GetFamilyWorld(FID, &type);
	if(type != WORLD_NONE)
	{
		sipwarning("FID is in Room! FID=%d, type=%d", FID, type);	// SP의 최적화와 관련하여 M_CS_OUTROOM를 올려보낸 다음에 M_CS_ENTER_LOBBY를 올려보내는것으로 수정
	}

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->OutRoom(FID);
	}
	else
	{
		CMessage	msgOut(M_CS_OUTROOM);
		msgOut.serial(FID);
		CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgOut, false);
		CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, msgOut, false);
	}	
	
	// set the family in lobby and save the familyInfo
	pLobby->EnterFamilyInLobby(FID, _tsid, 0); // 0 : bReqItemRefresh
}

static	void	cb_M_CS_LEAVE_LOBBY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	if ( pLobby == NULL )
	{
		sipwarning("Lobby object is NULL");
		return;
	}
	pLobby->ch_leave(FID);
	//RemoveFamily(FID);
}

static	void	cb_M_NT_LOGOUT_Lobby(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	if ( pLobby == NULL )
	{
		sipwarning("Lobby object is NULL");
		return;
	}
	pLobby->ch_leave(FID);

	RemoveFamily(FID);
}

static void	DBCB_DBCreateRoomExp(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		_FID		= *(T_FAMILYID *)argv[0];
	T_MSTROOM_ID	_PriceID	= *(T_MSTROOM_ID *)argv[1];

	FAMILY *family = GetFamilyData(_FID);
	if (family == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", _FID);
		return;
	}

	uint32	uResult = E_COMMON_FAILED;
	T_ROOMID RoomID = INVALID_ROOMID;
	if ( nQueryResult )
	{
		uint32	nRows;		resSet->serial(nRows);
		if (nRows > 0)
		{
			uint32 ret;			resSet->serial(ret);
			if (ret == ERR_NOERR)
			{
				uResult = ERR_NOERR;
				resSet->serial(RoomID);
			}
		}
	}

	uint32		AddUMoney = 0;
	CMessage	msgout(M_SC_NEWROOM);
	msgout.serial(_FID, uResult, RoomID, _PriceID, AddUMoney);
	CUnifiedNetwork::getInstance ()->send(family->m_FES, msgout);
}

static void	CreateExpRoom(T_FAMILYID _FID, T_MSTROOM_ID _MstRoomID)
{
	FAMILY *family = GetFamilyData(_FID);
	if (family == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", _FID);
		return;
	}

	MST_ROOM	*room	= GetMstRoomData(_MstRoomID);
	if ( room == NULL )
	{
		// 鍮꾨쾿?뚯??몃컻寃?
		ucstring	ucUnlawContent = SIPBASE::_toString("Improper Room IDRoomTypeByPrice(ID:%u)", _MstRoomID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
		return;
	}
	MAKEQUERY_CREATEROOMEXP(strQuery, room->m_SceneID, _FID);
	DBCaller->execute(strQuery, DBCB_DBCreateRoomExp,
		"DW",
		CB_,	_FID,
		CB_,	_MstRoomID);
}

//void	MakeRoomDefaultPet(T_MSTROOM_ID _typeID, T_ROOMID _uRoomid)
//{
//	T_PETID PetID;
//	T_PETCOUNT PetNum;
//	T_PETREGIONID PetRegion;
//	T_PETPOS PetPos = L"";
//	T_PETTYPEID		PetTyepID;
//	T_PETGROWSTEP	PetStep;
//	T_PETLIFE		PetLife;
//
//	for (LST_OUT_ROOMDEFAULTPET::iterator it = lst_OUT_ROOMDEFAULTPET.begin(); it != lst_OUT_ROOMDEFAULTPET.end (); it++)
//	{
//		if ((*it).RoomID == _typeID)
//		{
//			PetID = (*it).PetID;
//			PetNum = (*it).PetNum;
//			PetRegion = (*it).RegionID;
//
//			OUT_PETSET	*pPetSet = GetOutPetSet(PetID);
//			if (pPetSet == NULL)
//			{
//				sipwarning(L"There is no OUT_PETSET : PetID=%d", PetID);
//				continue;
//			}
//			PetTyepID = pPetSet->TyepID;
//			PetStep = pPetSet->OriginStep;
//			PetLife = pPetSet->OriginLife;
//
//			MAKEQUERY_ADDDEFAULTPET(strQ, _uRoomid, PetTyepID, PetID, PetNum, PetStep, PetLife, PetRegion, PetPos.c_str());
//			DBCaller->execute(strQ, NULL, "");
//			// DB_EXE_QUERY(DB, Stmt, strQ);
//		}
//	}
//}

static void	DBCB_DBCreateRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		idMaster			= *(T_FAMILYID *)argv[0];
	uint16			idRoomTypeByPrice	= *(uint16 *)argv[1];
	uint32			TryCount			= *(uint32 *)argv[2];
	TServiceId		tsid(*(uint16 *)argv[3]);

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(idMaster);
	if(pFamilyInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", idMaster);
		return;
	}

	MST_ROOM	*room	= GetMstRoomData(idRoomTypeByPrice);
	if ( room == NULL )
	{
		sipwarning("GetMstRoomData = NULL. RoomTypeID=%d", idRoomTypeByPrice);
		return;
	}

	uint32	uDays	= room->m_Years * 365 + room->m_Months * 30 + room->m_Days;
	uint16	idScene	= room->m_SceneID;
	T_PRICE	UPrice = room->GetRealPrice();

	uint32	uResult = E_COMMON_FAILED, uRoomid;//, UMoney, GMoney;
	{	// Don't remove this empty scope!!!
		if (nQueryResult != true)
		{
			uResult = E_DBERR;
			goto	SEND;
		}
		uint32		nRows;		resSet->serial(nRows);
		if (nRows < 1)
		{
			uResult = E_DBERR;
			goto	SEND;
		}
		uint32 uret;				resSet->serial(uret);
		uint32 idRoom;			resSet->serial(idRoom);
		uRoomid = idRoom;
		int ret = (int)uret;
		switch ( ret )
		{
		case 0:
			uResult = ERR_NOERR;
			break;
		case 1:
		case 1001:
			{
				// FID에 해당한 정보를 찾을수 없다. - 나타날 경우가 없다.
				ucstring ucUnlawContent = SIPBASE::_toString("Cannot Create Room, FID=%d", idMaster).c_str();
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
				return;
			}
		case 1019:
			{
				// 이미 체험방을 창조하였기때문에 더 창조할수 없다.
				ucstring ucUnlawContent = SIPBASE::_toString("Cannot Create Room because FreeUsed=1. FID=%d", idMaster).c_str();
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
				return;
			}
		case 1060: // 방창조할때 ID창조가 실패함. 다시 시도해야 한다.
			if(TryCount >= 5)	// 5번 계속 실패하였으면 E_CREATEROOMID_FAIL로 처리한다.
			{
				uResult = E_CREATEROOMID_FAIL;
				goto	SEND;
			}
			{
				TryCount ++;
				T_PRICE	TotalUMoney = pFamilyInfo->m_nUMoney;
				MAKEQUERY_CREATEROOM(strQuery, idScene, idMaster/*, TotalUMoney*/, room->m_Price, room->m_Years, room->m_Months, room->m_Days, room->m_HisSpace, ShardMainCode[0]);
				DBCaller->execute(strQuery, DBCB_DBCreateRoom,
					"DWDW",
					CB_,		idMaster,
					CB_,		idRoomTypeByPrice,
					CB_,		TryCount,
					CB_,		tsid.get());
			}
			return;
		case -1:
			sipinfo("DBError : Can't Insert");
			uResult = E_DBERR;
			goto	SEND;
		default:
			break;
		}
//		T_FAMILYNAME	fn;		resSet->serial(fn);
//		uint32 um;				resSet->serial(um);
//		uint32 gm;				resSet->serial(gm);
//		UMoney = um;
//		GMoney = gm;
	}

SEND:
	uint32		AddUMoney = 0;
	CMessage	msgout(M_SC_NEWROOM);
	msgout.serial(idMaster, uResult, uRoomid, idRoomTypeByPrice, AddUMoney);
	CUnifiedNetwork::getInstance ()->send(tsid, msgout);
	//sipinfo(L"->[M_SC_NEWROOM] Create Room master :%u, Result: %u, room:%u", idMaster, uResult, uRoomid); byy krs

	if ( uResult == ERR_NOERR )
	{
		// 諛⑹쓽 Default Pet?앹꽦
		//MakeRoomDefaultPet(idScene, uRoomid);
//		sipinfo(L"Family (id:%d) buy new (room %d) UMoney:%u, GMoney:%u", idMaster, uRoomid, UMoney, GMoney);
		sipinfo(L"Family (id:%d) buy new (room %d)", idMaster, uRoomid);

		// Log
		DBLOG_CreateRoom(idMaster, uRoomid, idScene, room->m_Name, uDays, UPrice, 0);

		// Notify LS to subtract the user's money by sending message
		ExpendMoney(idMaster, UPrice, 0, 1, DBLOG_MAINTYPE_BUYROOM, DBLSUB_BUYROOM_NEW,
			0, idScene, uDays, UPrice, uRoomid);
		// 2020/04/15 Add point(=UPrice) when buy room
		ExpendMoney(idMaster, 0, UPrice, 2, DBLOG_MAINTYPE_BUYROOM, DBLSUB_BUYROOM_NEW, 0, idScene);

		// Send Money
		SendFamilyProperty_Money(tsid, idMaster, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);
	}
}

// Create room([w:Room price id])
void	cb_M_CS_NEWROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32		idMaster;	_msgin.serial(idMaster);
	uint16		idRoomTypeByPrice;		_msgin.serial(idRoomTypeByPrice);

	if (!IsThisShardFID(idMaster))
	{
		sipwarning("IsThisShardFID = false. FID=%d", idMaster);
		return;
	}

	FAMILY *family = GetFamilyData(idMaster);
	if (family != NULL)
	{
		if (family->m_UserType == USERTYPE_EXPROOMMANAGER)
		{
			CreateExpRoom(idMaster, idRoomTypeByPrice);
			return;
		}
	}

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(idMaster);
	if(pFamilyInfo == NULL)
	{
		sipwarning("GetFamilyInfo=NULL FID=%d", idMaster);
		return;
	}

	MST_ROOM	*room	= GetMstRoomData(idRoomTypeByPrice);
	if ( room == NULL )
	{
		// 鍮꾨쾿?뚯??몃컻寃?
		ucstring	ucUnlawContent = SIPBASE::_toString("Improper Room IDRoomTypeByPrice(ID:%u)", idRoomTypeByPrice).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
		return;
	}

	uint32	uDays	= room->m_Years * 365 + room->m_Months * 30 + room->m_Days;
	uint32	price	= room->m_Price;
	uint32	discPercent	= room->m_DiscPercent;
	uint16	idScene	= room->m_SceneID;

	T_PRICE		UPrice = room->GetRealPrice();
	if(UPrice > 0)
	{
		if ( ! CheckMoneySufficient(idMaster, UPrice, 0, MONEYTYPE_UMONEY) )
		{
			uint32	uResult = E_NOMONEY;
			CMessage	msgout(M_SC_NEWROOM);
			msgout.serial(idMaster, uResult);
			CUnifiedNetwork::getInstance ()->send(_tsid, msgout);
			sipdebug("No Money");
			return;
		}
	}
	T_PRICE	TotalUMoney = pFamilyInfo->m_nUMoney;

	uint32	TryCount = 1;
	MAKEQUERY_CREATEROOM(strQuery, idScene, idMaster/*, TotalUMoney*/, room->m_Price, room->m_Years, room->m_Months, room->m_Days, room->m_HisSpace, ShardMainCode[0]);
	DBCaller->execute(strQuery, DBCB_DBCreateRoom,
		"DWDW",
		CB_,		idMaster,
		CB_,		idRoomTypeByPrice,
		CB_,		TryCount,
		CB_,		_tsid.get());
}

// Use Tianyuan Card New Room ([s:CardID])
void	cb_M_CS_ROOMCARD_NEW(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32		FID;
	string		CardID;
	_msgin.serial(FID, CardID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	if(CardID.empty() || CardID.length() > MAX_CARD_ID_LEN)
	{
		sipwarning("Invalid CardID. FID=%d, CardID=%s", FID, CardID.c_str());
		return;
	}

	FAMILY	*pFamily = GetFamilyData(FID);
	if(pFamily == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	CMessage	msgOut(M_SM_ROOMCARD_USE);
	T_ROOMID	RoomID = 0;
	uint16	sid = SIPNET::IService::getInstance()->getServiceId().get();
	msgOut.serial(FID, pFamily->m_UserID, CardID, pFamily->m_UserIP, RoomID, sid);
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
}

uint16 GetRoomPriceIDFromCardType(uint16 CardType)
{
	MAP_MST_ROOM::iterator it;
	for(it = map_mstRoom.begin(); it != map_mstRoom.end(); it ++)
	{
		if(it->second.m_CardType == CardType)
			break;
	}
	if(it == map_mstRoom.end())
	{
		sipwarning("Can't find mst_room of CardType=%d", CardType);
		return 0;
	}
	return it->first;
}

static void	DBCB_DBCreateRoomByCard(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID			= *(T_FAMILYID *)argv[0];
	string			CardID		= (char*) argv[1];
	uint16			CardType	= *(uint16 *)argv[2];
	uint32			TryCount	= *(uint32 *)argv[3];

	if(nQueryResult != true)
	{
		sipwarning("nQueryResult != true.");
		return;
	}

	uint32		nRows;		resSet->serial(nRows);
	if (nRows < 1)
	{
		sipwarning("nRows < 1");
		return;
	}

	uint32	ret;			resSet->serial(ret);
	uint32	RoomID;			resSet->serial(RoomID);

	uint16	RoomPriceID = GetRoomPriceIDFromCardType(CardType);
	MST_ROOM	*mst_room	= GetMstRoomData(RoomPriceID);

	uint32	err = 0;
	switch(ret)
	{
	case 0:
		{
			// Log
			DBLOG_CreateRoomByCard(FID, RoomID, RoomPriceID, mst_room->m_Name, 0);

			uint32	UPrice = mst_room->GetRealPrice(); // mst_room->m_Price;
			uint32	AddUMoney = 0; // (uint32)(UPrice * g_nRoomcardBackRate / 100);
			//ExpendMoney(FID, UPrice, 0, 3, DBLOG_MAINTYPE_BUYROOM, DBLSUB_BUYROOM_NEW_CARD,
			//	0, mst_room->m_SceneID, AddUMoney, UPrice, RoomID); 2020/04/21 삭제됨

			// 2020/04/15 Add point(=UPrice-AddUMoney) when buy room
			ExpendMoney(FID, 0, UPrice - AddUMoney, 2, DBLOG_MAINTYPE_BUYROOM, DBLSUB_BUYROOM_NEW_CARD, 0, mst_room->m_SceneID);

			// 방구입을 통지
			{
				INFO_FAMILYINROOM	*pFamilyInfo = GetFamilyInfo(FID);
				if(pFamilyInfo == NULL)
				{
					sipwarning("GetFamilyData = NULL. FID=%d", FID);
				}
				else
				{
					// 방에 들어갈 때 다시 내려보내므로 여기서는 보내지 않는다.
					// SendFamilyProperty_Money(pFamilyInfo->m_ServiceID, FID, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);

					CMessage	msgout(M_SC_NEWROOM);
					msgout.serial(FID, err, RoomID, RoomPriceID, AddUMoney);
					CUnifiedNetwork::getInstance()->send(pFamilyInfo->m_ServiceID, msgout);
				}
			}

			sipinfo("Family buy new room by RoomCard. FID=%d, RoomID=%d, CardID=%s, PriceID=%d", FID, RoomID, CardID.c_str(), RoomPriceID);
		}
		break;

	case 1060: // 방창조할때 ID창조가 실패함. 다시 시도해야 한다.
		{
			if(TryCount >= 5)	// 5번 계속 실패하였으면 E_CREATEROOMID_FAIL로 처리한다.
			{
				sipwarning("TryCount >= 5!!! FID=%d, CardID=%s", FID, CardID.c_str());
				INFO_FAMILYINROOM	*pFamilyInfo = GetFamilyInfo(FID);
				if(pFamilyInfo == NULL)
				{
					sipwarning("GetFamilyData = NULL. FID=%d", FID);
				}
				else
				{
					err = E_CREATEROOMID_FAIL;
					CMessage	msgout(M_SC_NEWROOM);
					msgout.serial(FID, err);
					CUnifiedNetwork::getInstance()->send(pFamilyInfo->m_ServiceID, msgout);
				}
				break;
			}

			TryCount ++;
			MAKEQUERY_CREATEROOM(strQuery, mst_room->m_SceneID, FID, mst_room->m_Price, mst_room->m_Years, mst_room->m_Months, mst_room->m_Days, mst_room->m_HisSpace, ShardMainCode[0]);
			DBCaller->execute(strQuery, DBCB_DBCreateRoomByCard,
				"DsWD",
				CB_,		FID,
				CB_,		CardID.c_str(),
				CB_,		CardType,
				CB_,		TryCount);

			return;
		}
		break;

	default:
		sipwarning("DBError : Error=%d", ret);
		return;
	}
}

// Use Tianyuan Card  Result([d:FID][s:CardID][d:ErrorCode][w:CardType])
void    cb_M_MS_ROOMCARD_USE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint16	CardType;
	uint32	err, FID, RoomID;
	string	CardID;
	_msgin.serial(FID, CardID, RoomID, err, CardType);

	uint16	RoomPriceID = 0;
	MST_ROOM	*mst_room = NULL;
	if(err == 0)
	{
		RoomPriceID = GetRoomPriceIDFromCardType(CardType);

		mst_room = GetMstRoomData(RoomPriceID);
		if(mst_room == NULL)
		{
			sipwarning("GetMstRoomData = NULL. CardType=%d, RoomPriceID=%d", CardType, RoomPriceID);
			return;
		}
	}

	if (RoomID == 0)
	{
		// Buy New Room
		if(err != 0)
		{
			FAMILY	*pFamily = GetFamilyData(FID);
			if(pFamily == NULL)
			{
				sipwarning("GetFamilyData = NULL. FID=%d", FID);
			}
			else
			{
				CMessage	msgOut(M_SC_NEWROOM);
				msgOut.serial(FID, err);
				CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
			}

			sipinfo("NewRoom CheckRoomCard Failed. FID=%d, ErrorCode=%d, CardID=%s", FID, err, CardID.c_str());
			return;
		}

		uint32	TryCount = 1;
		MAKEQUERY_CREATEROOM(strQuery, mst_room->m_SceneID, FID, mst_room->m_Price, mst_room->m_Years, mst_room->m_Months, mst_room->m_Days, mst_room->m_HisSpace, ShardMainCode[0]);
		DBCaller->execute(strQuery, DBCB_DBCreateRoomByCard,
			"DsWD",
			CB_,		FID,
			CB_,		CardID.c_str(),
			CB_,		CardType,
			CB_,		TryCount);
	}
	else
	{
		// Promotion Room
		if(err != 0)
		{
			FAMILY	*pFamily = GetFamilyData(FID);
			if(pFamily == NULL)
			{
				sipwarning("GetFamilyData = NULL. FID=%d", FID);
			}
			else
			{
				CMessage	msgOut(M_SC_PROMOTION_ROOM);
				msgOut.serial(FID, err, RoomID);
				CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
			}

			sipinfo("PromotionRoom CheckRoomCard Failed. FID=%d, ErrorCode=%d, CardID=%s, RoomID=%d", FID, err, CardID.c_str(), RoomID);
			return;
		}

		MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID(SCENE_GANEN_FREE, 0, 0, 0);
		if (mst_room_old == NULL)
		{
			sipwarning("GetMstRoomData_FromSceneID = NULL. SceneID=%d", SCENE_GANEN_FREE);
			return;
		}

		int addhisspace = mst_room->m_HisSpace - mst_room_old->m_HisSpace;

		// Update
		{
			// 원래는 MAKEQUERY_PromotionRoom만 호출하려고 했는데 여기서는 FreeUse를 0으로 설정해주지 않기때문에 MAKEQUERY_RenewRoom도 호출한다!! - 정리필요
			{
				MAKEQUERY_PromotionRoom(strQuery, RoomID, addhisspace, mst_room->m_SceneID, 0);
				DBCaller->executeWithParam(strQuery, NULL,
					"D",
					OUT_,		NULL);
			}
			{
				MAKEQUERY_RenewRoom(strQuery, FID, RoomID, mst_room->m_SceneID, mst_room->m_Years, mst_room->m_Months, mst_room->m_Days, mst_room->m_Price, 0);
				DBCaller->execute(strQuery);
			}
		}

		// Log
		DBLOG_PromotionRoomByCard(FID, RoomID, mst_room->m_SceneID);

		uint32	UPrice = mst_room->GetRealPrice(); // mst_room->m_Price;
		uint32	AddUMoney = 0; // (uint32)(UPrice * g_nRoomcardBackRate / 100);
		//ExpendMoney(FID, UPrice, 0, 3, DBLOG_MAINTYPE_BUYROOM, DBLSUB_BUYROOM_RENEW_CARD,
		//	0, mst_room->m_SceneID, AddUMoney, UPrice, RoomID); 2020/04/21 삭제됨

		// 2020/04/15 Add point(=UPrice-AddUMoney) when renew room
		ExpendMoney(FID, 0, UPrice - AddUMoney, 2, DBLOG_MAINTYPE_BUYROOM, DBLSUB_BUYROOM_RENEW_CARD, 0, mst_room->m_SceneID);


		// 방구입을 통지
		{
			INFO_FAMILYINROOM	*pFamilyInfo = GetFamilyInfo(FID);
			if(pFamilyInfo == NULL)
			{
				sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
			}
			else
			{
				// 방에 들어갈 때 다시 내려보내므로 여기서는 보내지 않는다.
				// SendFamilyProperty_Money(pFamilyInfo->m_ServiceID, FID, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);

				CMessage	msgout(M_SC_PROMOTION_ROOM);
				msgout.serial(FID, err, RoomID, mst_room->m_ID, AddUMoney);
				CUnifiedNetwork::getInstance()->send(pFamilyInfo->m_ServiceID, msgout);
			}
		}

		sipinfo("Family promotion room by RoomCard. FID=%d, RoomID=%d, CardID=%s, PriceID=%d", FID, RoomID, CardID.c_str(), mst_room->m_Price);
	}

	//case 2: // RenewRoom
	//	{
	//		if(err != 0)
	//		{
	//			sipwarning("NewRoom CheckRoomCard Failed. ErrorCode=%d", err);
	//			return;
	//		}

	//		T_MSTROOM_ID	SceneID;
	//		T_ROOMNAME		roomName;
	//		ucstring		OldTermTime;
	//		uint8			FreeUse;
	//		{
	//			MAKEQUERY_GetRoomInfo(strQuery, FID, RoomID);
	//			CDBConnectionRest	DB(DBCaller);
	//			DB_EXE_QUERY(DB, Stmt, strQuery);
	//			if (!nQueryResult)
	//			{
	//				err = E_DBERR;
	//				CMessage	msgOut(M_SC_RENEWROOM);
	//				msgOut.serial(FID, err, RoomID);
	//				if(FSid.get() != 0)
	//					CUnifiedNetwork::getInstance()->send(FSid, msgOut);

	//				sipinfo("FAIL user(Fid:%d) Renew room(Rid:%d) by Card", FID, RoomID);
	//				return;
	//			}

	//			DB_QUERY_RESULT_FETCH(Stmt, row);
	//			if (IS_DB_ROW_VALID(row))
	//			{
	//				COLUMN_DIGIT(row, 0, uint16, sceneId);
	//				COLUMN_WSTR(row, 1, rname, MAX_LEN_ROOM_NAME);
	//				COLUMN_DATETIME(row, 4, ttime);
	//				COLUMN_DIGIT(row, 7, uint8, freeuse);
	//				SceneID = sceneId;
	//				roomName = rname;
	//				OldTermTime = ttime;
	//				FreeUse = freeuse;
	//			}
	//			else
	//			{
	//				// 비법파케트발견
	//				ucstring ucUnlawContent = SIPBASE::_toString("Invalid Room ID:%d by Card", RoomID).c_str();
	//				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
	//				return;
	//			}
	//		}

	//		// 무한방은 연장할수 없다.
	//		if(OldTermTime == L"")
	//		{
	//			// 비법파케트발견
	//			ucstring ucUnlawContent = SIPBASE::_toString("Cannot renew room by card because unlimited. FID=%d, RoomID=%d", FID, RoomID).c_str();
	//			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
	//			return;
	//		}

	//		MST_ROOM	*room	= GetMstRoomData(RoomPriceID);
	//		if(room == NULL)
	//		{
	//			sipwarning("GetMstRoomData = NULL. CardType=%d, RoomPriceID=%d", CardType, RoomPriceID);
	//			return;
	//		}

	//		uint32	addhisspace = 0;
	//		if(FreeUse != 0)
	//		{
	//			MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID_FreeRoom(SceneID);
	//			addhisspace = room->m_HisSpace - mst_room_old->m_HisSpace;
	//		}

	//		// 방기한연장
	//		{
	//			MAKEQUERY_RenewRoom(strQuery, FID, RoomID, SceneID, room->m_Years, room->m_Months, room->m_Days, 0, addhisspace);
	//			CDBConnectionRest	DB(DBCaller);
	//			DB_EXE_QUERY_WITHONEROW(DB, Stmt, strQuery);
	//			if(nQueryResult != true)
	//			{
	//				sipwarning("MAKEQUERY_RenewRoom by Card failed.");
	//				return;
	//			}

	//			COLUMN_DIGIT(row, 0, sint32, ret);

	//			if(ret != 0)
	//			{
	//				sipwarning("MAKEQUERY_RenewRoom by Card return err. ErrorCode=%d", ret);
	//				return;
	//			}

	//			COLUMN_DATETIME(row, 1, ttime);
	//			T_DATETIME	termtime;
	//			termtime = ttime;
	//		}

	//		// Main Server에 카드가 리용되였음을 통지한다.
	//		{
	//			uint8	flag = 2;	// RenewRoom
	//			CMessage	msgOut(M_SM_ROOMCARD_USE);
	//			msgOut.serial(CardID, CardPwd, flag, pFamily->m_UserID, FID, RoomID, pFamily->m_UserName, pFamily->m_UserIP);
	//			CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	//		}

	//		// 방관리자에게 통보
	//		{
	//			CMessage	msgOut(M_SC_RENEWROOM);
	//			msgOut.serial(FID, err, RoomID);
	//			if(FSid.get() != 0)
	//				CUnifiedNetwork::getInstance()->send(FSid, msgOut);

	//			sipinfo("SUCCESS user(id:%d) Renew room(id:%d) by Card", FID, RoomID);
	//		}

	//		// Log
	//		if (roomName == L"")
	//			roomName = room->m_Name;
	//		uint32		uDays = room->m_Years * 365 + room->m_Months * 30 + room->m_Days;
	//		DBLOG_RenewRoomByCard(FID, RoomID, SceneID, roomName, uDays);
	//	}
	//	break;

	//case 3: // PromotionRoom
	//	{
	//		if(err != 0)
	//		{
	//			sipwarning("PromotionRoom CheckRoomCard Failed. ErrorCode=%d", err);
	//			return;
	//		}

	//		MAKEQUERY_PromotionRoomByCard(strQ, RoomID, CardType);
	//		DBCaller->executeWithParam(strQ, DBCB_DBPromotionRoomByCard, 
	//			"DDDWss",
	//			OUT_,	NULL,
	//			CB_,	FID,
	//			CB_,	RoomID,
	//			CB_,	CardType,
	//			CB_,	CardID.c_str(),
	//			CB_,	CardPwd.c_str());
	//	}
	//	break;

	//default:
	//	sipwarning("Invalid flag. flag=%d", flag);
	//	break;
	//}
}

static bool	IsGanEnYuan(uint32 SceneID)
{
	if (SceneID == SCENE_GANEN_FREE ||
		SceneID == SCENE_GANEN_NOFREE )
		return true;
	return false;
}

static void	DBCB_DBGetRoomRenewHistory(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint8			Flag				= *(uint8 *)argv[0];
	uint32			OldSceneID			= *(uint32 *)argv[1];
	sint32			ret				= *(sint32 *)argv[2];
	T_FAMILYID		idMaster			= *(T_FAMILYID *)argv[3];
	T_ROOMID		idRoom				= *(T_ROOMID *)argv[4];
	uint16			idScene				= *(uint16 *)argv[5];
	uint16			idPrice				= *(uint16 *)argv[6];
	string			CardID				= (char*) argv[7];

	if (!nQueryResult || ret == -1)
	{
		sipwarning("Shrd_Room_Renew_History_List: execute failed!");
		return;	// E_DBERR
	}

	// 천원카드승급: 면비방만 할수 있다.
	if (idScene == 0)
	{
		// 천원카드승급는 면비방만 할수 있다.
		if (Flag != 1)
		{
			sipwarning("Can'r Promotion by card if room is not free room. FID=%d, RoomID=%d", idMaster, idRoom);
			return;
		}

		// 검사
		if(CardID.empty() || CardID.length() > MAX_CARD_ID_LEN)
		{
			sipwarning("Invalid CardID. FID=%d, CardID=%s", idMaster, CardID.c_str());
			return;
		}

		FAMILY	*pFamily = GetFamilyData(idMaster);
		if(pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", idMaster);
			return;
		}

		CMessage	msgOut(M_SM_ROOMCARD_USE);
		uint16	sid = SIPNET::IService::getInstance()->getServiceId().get();
		msgOut.serial(idMaster, pFamily->m_UserID, CardID, pFamily->m_UserIP, idRoom, sid);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);

		return;
	}

	// 동급천원승급인지 검사
	bool bIsSamePrice = false;
	if (Flag != 1 && OldSceneID != idScene)
	{
		MST_ROOM	*mst_room_old	= NULL;
		if (IsGanEnYuan(OldSceneID))
			mst_room_old = GetMstRoomData_FromSceneID((uint16)OldSceneID, 0, 0, 0);
		else
			mst_room_old = GetMstRoomData_FromSceneID((uint16)OldSceneID, 1, 0, 0);

		MST_ROOM	*mst_room_new	= GetMstRoomData_FromSceneID((uint16)idScene, 1, 0, 0);
		if ((mst_room_old == NULL) || (mst_room_new == NULL))
		{
			sipwarning("GetMstRoomData_FromSceneID failed! - Master Data Error, FID=%d, RoomID=%d, OldSceneID=%d, NewSceneID=%d", idMaster, idRoom, OldSceneID, idScene);
			return; // E_MASTERDATA
		}
		if (mst_room_old->m_Price == mst_room_new->m_Price)
			bIsSamePrice = true;
	}

	// Calc Price
	int	addhisspace = -1;
	uint32	need_price = 0;
	uint32	curtime_min = (uint32)(GetDBTimeSecond() / 60);
	bool bIsSamePromotion = false;
	if (Flag == 1) // old is free room
	{
		// 원래 면비방인 경우
		MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID_FreeRoom((uint16)OldSceneID);
		MST_ROOM	*mst_room_new	= GetMstRoomData(idPrice);
		if ((mst_room_old == NULL) || (mst_room_new == NULL))
		{
			sipwarning("GetMstRoomData_FromSceneID failed! - Master Data Error, FID=%d, RoomID=%d, OldSceneID=%d, NewSceneID=%d", idMaster, idRoom, OldSceneID, idScene);
			return; // E_MASTERDATA
		}

		// 가격은 새 방가격이다.
		need_price = mst_room_new->GetRealPrice();

		addhisspace = (mst_room_new->m_HisSpace - mst_room_old->m_HisSpace);
	}
	else if ( (bIsSamePrice) && (idPrice == 0) )
	{
		// 동급천원승급인 경우
		need_price = g_nSamePromotionPrice;
		addhisspace = 0;
		bIsSamePromotion = true;
	}
	else if (Flag == 2) // old is unlimited room
	{
		// 원래 무한방인 경우
		MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID((uint16)OldSceneID, 0, 0, 0);
		MST_ROOM	*mst_room_new	= GetMstRoomData_FromSceneID((uint16)idScene, 0, 0, 0);
		if ((mst_room_old == NULL) || (mst_room_new == NULL))
		{
			sipwarning("GetMstRoomData_FromSceneID failed! - Master Data Error, FID=%d, RoomID=%d, OldSceneID=%d, NewSceneID=%d", idMaster, idRoom, OldSceneID, idScene);
			return; // E_MASTERDATA
		}
		if (mst_room_new->m_Price < mst_room_old->m_Price)
		{
			// 낮은가격방으로의 승급은 불가능하다.
			ucstring	ucUnlawContent = SIPBASE::_toString("Cannot promotion to lower level room(Unlimited) FID=%d, RoomID=%d, OldSceneID=%d, NewSceneID=%d", idMaster, idRoom, OldSceneID, idScene).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
			return;
		}
		if (mst_room_new->m_Price == mst_room_old->m_Price)
		{
			need_price = g_nSamePromotionPrice;
			addhisspace = 0;
			bIsSamePromotion = true;
		}
		else
		{
			// 가격은 두 무한방가격의 차이이다.
			need_price = (mst_room_new->m_Price - mst_room_old->m_Price) * mst_room_new->m_DiscPercent / 100;

			addhisspace = (mst_room_new->m_HisSpace - mst_room_old->m_HisSpace);

			// idPrice = 0;
		}
	}
	else
	{
		uint32		nRows;			resSet->serial(nRows);
		if (idPrice == 0 && nRows == 0)
		{
			sipwarning("idPrice == 0 && nRows == 0");
			return;
		}
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint32			rhis_id;		resSet->serial(rhis_id);
			T_DATETIME		begintime;		resSet->serial(begintime);
			T_DATETIME		endtime;		resSet->serial(endtime);
			uint32			price;			resSet->serial(price);
			uint8			years;			resSet->serial(years);
			uint8			months;			resSet->serial(months);
			uint8			days;			resSet->serial(days);
			uint16			sceneid;		resSet->serial(sceneid);

			MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID((uint16)sceneid, years, months, days);
			if (mst_room_old == NULL)
			{
				sipwarning("GetMstRoomData_FromSceneID failed! - Master Data Error (%d,%d,%d,%d)", sceneid, years, months, days);
				return; // E_MASTERDATA
			}

			uint32	begintime_min = getMinutesSince1970(begintime);
			uint32	endtime_min = getMinutesSince1970(endtime);
			uint32 uAllMinutes = endtime_min - begintime_min;
			if (endtime_min <= curtime_min)
			{
				// sipassert(false);
				continue;
			}
			if (begintime_min < curtime_min)
				begintime_min = curtime_min;

			if (idPrice == 0)
			{
				// 일반승급인 경우
				MST_ROOM	*mst_room	= GetMstRoomData_FromSceneID((uint16)idScene, years, months, days);
				if (mst_room == NULL)
				{
					sipwarning("GetMstRoomData_FromSceneID failed! - Master Data Error (%d,%d,%d,%d)", idScene, years, months, days);
					return; // E_MASTERDATA
				}
				if (mst_room->m_Price <= mst_room_old->m_Price)
				{
					// 낮은가격방으로의 승급은 불가능하다.
					ucstring	ucUnlawContent = SIPBASE::_toString("Cannot promotion to lower level room FID=%d, RoomID=%d", idMaster, idRoom).c_str();
					RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
					return;
				}

				if (addhisspace < 0)
				{
					addhisspace = mst_room->m_HisSpace - mst_room_old->m_HisSpace;
					if (addhisspace < 0)
					{
						sipwarning("HisSpace Error!!! - Master Data Error");
						return; // E_MASTERDATA
					}
				}

				need_price += (uint32)(1.0 * (endtime_min - begintime_min) * (mst_room->m_Price - mst_room_old->m_Price) * mst_room->m_DiscPercent / (uAllMinutes * 100));
			}
			else
			{
				// 승급연장인 경우
				need_price += (uint32)(1.0 * (endtime_min - begintime_min) * mst_room_old->m_Price / uAllMinutes);
			}
		}

		if (idPrice != 0)
		{
			// 승급연장인 경우
			MST_ROOM	*mst_room_new	= GetMstRoomData(idPrice);
			if (mst_room_new == NULL)
			{
				sipwarning("GetMstRoomData = NULL. PriceID=%d", idPrice);
				return;
			}

			need_price = (mst_room_new->m_Price - need_price) * mst_room_new->m_DiscPercent / 100;
			if (need_price < 0)
			{
				sipwarning("need_price < 0. need_price=%d", need_price);
				need_price = 0;
			}

			/*
			if (bIsSamePrice)
				need_price += g_nSamePromotionPrice;// 2013-01-16 */

			MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID(OldSceneID, mst_room_new->m_Years, mst_room_new->m_Months, mst_room_new->m_Days);
			if (mst_room_old == NULL)
			{
				sipwarning("GetMstRoomData_FromSceneID = NULL. SceneID=%d", OldSceneID);
				return;
			}
			addhisspace = mst_room_new->m_HisSpace - mst_room_old->m_HisSpace;
		}
	}

	// Check Money
	if (!CheckMoneySufficient(idMaster, need_price, 0, MONEYTYPE_UMONEY))
	{
		return;
	}

	// OK
	if (!(OldSceneID == SCENE_GANEN_FREE && idScene == SCENE_GANEN_NOFREE))
	{
		MAKEQUERY_PromotionRoom(strQ, idRoom, addhisspace, idScene, (bIsSamePromotion) ? 1 : 0);

		DBCaller->executeWithParam(strQ, NULL,
			"D",
			OUT_,		NULL);
	}

	if (idPrice != 0)
	{
		// 승급연장인 경우 새로 하나 추가하여야 한다.
		MST_ROOM	*mst_room	= GetMstRoomData(idPrice);

		// 앞에서 addhisspace를 추가하였으므로 여기서는 더 추가하지 않느다.
		if (!(OldSceneID == SCENE_GANEN_FREE && idScene == SCENE_GANEN_NOFREE))
			addhisspace = 0;

		MAKEQUERY_RenewRoom(strQuery, idMaster, idRoom, idScene, mst_room->m_Years, mst_room->m_Months, mst_room->m_Days, mst_room->m_Price, addhisspace);
		DBCaller->execute(strQuery);
	}
	else if (Flag == 0)
	{
		// 일반승급인 경우
		MAKEQUERY_UpdateRoomRenewHistory(strQ, idRoom, idScene);

		DBCaller->execute(strQ);
	}

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(idMaster);
	if (pFamilyInfo == NULL)
	{
		sipwarning("System Error!");
		return;
	}

	TServiceId	tsid = pFamilyInfo->m_ServiceID;

	// Notify LS to subtract the user's money by sending message
	ExpendMoney(idMaster, need_price, 0, 1, DBLOG_MAINTYPE_PROMOTIONROOM, DBLSUB_PROMOTION, 0, 0, idScene, need_price, idRoom);
	// 2020/04/29 Add point(=need_price) when buy room
	ExpendMoney(idMaster, 0, need_price, 2, DBLOG_MAINTYPE_PROMOTIONROOM, DBLSUB_PROMOTION, 0, 0);

	// Money
	SendFamilyProperty_Money(tsid, idMaster, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);

	// Log
	DBLOG_PromotionRoom(idMaster, idRoom, idScene, need_price, 0);

	// Finish
	uint32	result = ERR_NOERR;
	uint32	AddUMoney = 0;
	CMessage	msgout(M_SC_PROMOTION_ROOM);
	msgout.serial(idMaster, result, idRoom, idPrice, AddUMoney);

	CUnifiedNetwork::getInstance()->send(tsid, msgout);

	sipinfo("SUCCESS user(id:%u) Promotion room(id:%u)", idMaster, idRoom);
}

void	cb_M_CS_PROMOTION_ROOM_Lobby(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		idMaster;		_msgin.serial(idMaster);
	T_ROOMID		idRoom;			_msgin.serial(idRoom);
	uint16			idScene;		_msgin.serial(idScene);
	uint16			idPrice;		_msgin.serial(idPrice);
	string			CardID;			_msgin.serial(CardID);

	// M_CS_PROMOTION_ROOM
	MAKEQUERY_GetRoomRenewHistory(strQ, idRoom);
	DBCaller->executeWithParam(strQ, DBCB_DBGetRoomRenewHistory, 
		"BDDDDWWs",
		OUT_,	NULL,
		OUT_,	NULL,
		OUT_,	NULL,
		CB_,	idMaster,
		CB_,	idRoom,
		CB_,	idScene,
		CB_,	idPrice,
		CB_,	CardID.c_str());
}

//void	cb_M_CS_ROOMCARD_PROMOTION_Lobby(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	//uint32		FID, RoomID;
//	//string		CardID, CardPwd;
//	//_msgin.serial(FID, RoomID, CardID, CardPwd);
//
//	//if(CardID.empty() || CardID.length() > MAX_CARD_ID_LEN)
//	//{
//	//	sipwarning("Invalid CardID. CardID=%s", CardID.c_str());
//	//	return;
//	//}
//	//if(CardPwd.empty() || CardPwd.length() > MAX_CARD_PWD_LEN)
//	//{
//	//	sipwarning("Invalid CardID. CardPwd=%s", CardPwd.c_str());
//	//	return;
//	//}
//
//	//uint8	flag = 3;	// PromotionRoom
//	//CMessage	msgOut(M_SM_ROOMCARD_CHECK);
//	//uint16	sid = SIPNET::IService::getInstance()->getServiceId().get();
//	//msgOut.serial(FID, flag, RoomID, CardID, CardPwd, sid);
//	//CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
//}

static void	DBCB_DBGetRoomRenewHistoryForGet(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint8			Flag				= *(uint8 *)argv[0];
	uint32			OldSceneID			= *(uint32 *)argv[1];
	sint32			ret					= *(sint32 *)argv[2];
	T_FAMILYID		idMaster			= *(T_FAMILYID *)argv[3];
	T_ROOMID		idRoom				= *(T_ROOMID *)argv[4];
	uint16			idScene				= *(uint16 *)argv[5];
	uint8			bToUnlimited		= *(uint8 *)argv[6];
	TServiceId		tsid(*(uint16 *)argv[7]);

	if (!nQueryResult || ret == -1)
	{
		sipwarning("Shrd_Room_Renew_History_List: execute failed!");
		return;	// E_DBERR
	}

	// 동급천원승급인지 검사
	bool bIsSamePromotion = false;
	if (OldSceneID != idScene)
	{
		MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID((uint16)OldSceneID, 1, 0, 0);
		MST_ROOM	*mst_room_new	= GetMstRoomData_FromSceneID((uint16)idScene, 1, 0, 0);
		if (mst_room_old->m_Price == mst_room_new->m_Price)
			bIsSamePromotion = true;
	}

	uint32	result = ERR_NOERR;
	uint32	need_price = 0;

	// 원래 공짜방인 경우
	if (Flag == 1)
	{
		sipwarning("This room is FreeRoom!!!");
		return;

		//MST_ROOM	*mst_room	= GetMstRoomData(idPrice);
		//if(mst_room == NULL)
		//{
		//	sipwarning("cb_M_CS_GET_PROMOTION_PRICE: GetMstRoomData failed! - Master Data Error, PriceID=%d", idPrice);
		//	return;
		//}
		//if(mst_room->GetRealPrice() == 0)
		//{
		//	// 공짜방->공짜방 승급은 불가능하다.
		//	ucstring	ucUnlawContent = SIPBASE::_toString("Cannot promotion from FreeUse to FreeUse. FID=%d RoomID=%d, NewPriceID=%d", idMaster, idRoom, idPrice).c_str();
		//	RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
		//	return;
		//}

		//// 가격은 새 방의 가격이다.
		//need_price += mst_room->GetRealPrice();

		//result = ERR_NOERR;
		//uint16	LastOldRoomKind = 0; // 면비방이 승급할때에는 0으로 보낸다.
		//CMessage	msgout(M_SC_PROMOTION_PRICE);
		//msgout.serial(idMaster, result, idRoom, idScene, need_price, LastOldRoomKind);

		//CUnifiedNetwork::getInstance()->send(tsid, msgout);

		//sipinfo("SUCCESS(FreeUse) user(id:%d) cb_M_CS_GET_PROMOTION_PRICE(id:%d), PriceID=%d, Price=%d", idMaster, idRoom, idPrice, need_price);

		//return;
	}

	// 원래 무한방인 경우
	if (Flag == 2)
	{
		sipwarning("This room is UnlimitedRoom!!!");
		return;

		//MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID((uint16)OldSceneID, 0, 0, 0);
		//MST_ROOM	*mst_room_new	= GetMstRoomData_FromSceneID((uint16)idScene, 0, 0, 0);
		//if((mst_room_old == NULL) || (mst_room_new == NULL))
		//{
		//	sipwarning("cb_M_CS_GET_PROMOTION_PRICE: GetMstRoomData_FromSceneID failed! - Master Data Error, FID=%d, RoomID=%d, OldSceneID=%d, NewSceneID=%d", idMaster, idRoom, OldSceneID, idScene);
		//	return; // E_MASTERDATA
		//}
		//if(mst_room_new->m_Price <= mst_room_old->m_Price)
		//{
		//	// 낮은가격방으로의 승급은 불가능하다.
		//	ucstring	ucUnlawContent = SIPBASE::_toString("Cannot promotion to lower level room(Unlimited) FID=%d, RoomID=%d, OldSceneID=%d, NewSceneID=%d", idMaster, idRoom, OldSceneID, idScene).c_str();
		//	RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
		//	return;
		//}

		//// 가격은 두 무한방가격의 차이이다.
		//need_price += (mst_room_new->GetRealPrice() - mst_room_old->GetRealPrice());

		//result = ERR_NOERR;
		//uint16	LastOldRoomKind = 0; // 무한방이 승급할때에는 0으로 보낸다.
		//CMessage	msgout(M_SC_PROMOTION_PRICE);
		//msgout.serial(idMaster, result, idRoom, idScene, need_price, LastOldRoomKind);

		//CUnifiedNetwork::getInstance()->send(tsid, msgout);

		//sipinfo("SUCCESS(Unlimited->Unlimited) user(id:%d) cb_M_CS_GET_PROMOTION_PRICE(id:%d), Price=%d", idMaster, idRoom, need_price);

		//return;
	}

	uint32	curtime_min = (uint32)(GetDBTimeSecond() / 60);
	uint16	LastOldSceneId = 0;
	uint8	LastOldYears = 0, LastOldMonths = 0, LastOldDays = 0;

	uint32	nRows;			resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32			rhis_id;		resSet->serial(rhis_id);
		T_DATETIME		begintime;		resSet->serial(begintime);
		T_DATETIME		endtime;		resSet->serial(endtime);
		uint32			price;			resSet->serial(price);
		uint8			years;			resSet->serial(years);
		uint8			months;			resSet->serial(months);
		uint8			days;			resSet->serial(days);
		uint16			oldsceneid;		resSet->serial(oldsceneid);

		LastOldSceneId = oldsceneid;
		LastOldYears = years;
		LastOldMonths = months;
		LastOldDays = days;

		MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID((uint16)oldsceneid, years, months, days);
		if (mst_room_old == NULL)
		{
			sipwarning("cb_M_CS_GET_PROMOTION_PRICE: GetMstRoomData_FromSceneID failed! - Master Data Error (%d,%d,%d,%d)", oldsceneid, years, months, days);
			return; // E_MASTERDATA
		}
		MST_ROOM	*mst_room	= GetMstRoomData_FromSceneID((uint16)idScene, years, months, days);
		if (mst_room == NULL)
		{
			sipwarning("cb_M_CS_GET_PROMOTION_PRICE: GetMstRoomData_FromSceneID failed! - Master Data Error (%d,%d,%d,%d)", idScene, years, months, days);
			return; // E_MASTERDATA
		}
		if (bToUnlimited == 0)	// 무한방으로의 연장이 아닌 경우
		{
			if (mst_room->m_Price <= mst_room_old->m_Price)
			{
				// 낮은가격방으로의 승급은 불가능하다.
				ucstring	ucUnlawContent = SIPBASE::_toString("Cannot promotion to lower level room FID=%d, RoomID=%d", idMaster, idRoom).c_str();
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
				return;
			}
		}
		else
		{
			if (mst_room->m_Price < mst_room_old->m_Price)
			{
				// 낮은가격방으로의 승급은 불가능하다.
				ucstring	ucUnlawContent = SIPBASE::_toString("Cannot promotion to lower level room FID=%d, RoomID=%d", idMaster, idRoom).c_str();
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idMaster);
				return;
			}
		}

		uint32	begintime_min = getMinutesSince1970(begintime);
		uint32	endtime_min = getMinutesSince1970(endtime);
		uint32 uAllMinutes = endtime_min - begintime_min;
		if (endtime_min <= curtime_min)
		{
//			sipassert(false);
			continue;
		}
		if (begintime_min < curtime_min)
			begintime_min = curtime_min;

		// 자리초과가 일어날수 있으므로 float연산으로 한다.
		if (bToUnlimited == 0)
			need_price += (uint32)(1.0 * (endtime_min - begintime_min) * (mst_room->m_Price - mst_room_old->m_Price) * mst_room->m_DiscPercent / (uAllMinutes * 100));
		else	// 무한방으로의 연장인 경우에는 현재 남아있는 돈의 합을 계산한다.
			need_price += (uint32)(1.0 * (endtime_min - begintime_min) * mst_room_old->m_Price / uAllMinutes);
	}

	if (nRows == 0)
	{
		sipwarning("Shrd_Room_Renew_History_List: return empty!");
		return;	// E_ROOMDATA
	}

	if (bToUnlimited != 0)
	{
		// 무한방으로의 연장인 경우
		MST_ROOM	*mst_room_unlimited = GetMstRoomData_FromSceneID(idScene, 0, 0, 0);
		if (mst_room_unlimited == NULL)
		{
			sipwarning("cb_M_CS_GET_PROMOTION_PRICE: GetMstRoomData_FromSceneID failed! - Master Data Error");
			return; // E_MASTERDATA
		}
		need_price = (mst_room_unlimited->m_Price - need_price) * mst_room_unlimited->m_DiscPercent / 100;
		if (need_price < 0)
		{
			sipwarning("need_price < 0. need_price=%d", need_price);
			need_price = 0;
		}

		/*if (bIsSamePromotion)
			need_price += g_nSamePromotionPrice;//2013-01-16*/
	}

	uint32	last_price_id = 0;
	if (nRows == 1)
	{
		MST_ROOM	*mst_room_last_old = GetMstRoomData_FromSceneID(LastOldSceneId, LastOldYears, LastOldMonths, LastOldDays);
		if (mst_room_last_old == NULL)
		{
			sipwarning("cb_M_CS_GET_PROMOTION_PRICE: GetMstRoomData_FromSceneID failed! - Master Data Error");
			return; // E_MASTERDATA
		}
		last_price_id = mst_room_last_old->m_ID;
	}

	result = ERR_NOERR;
	CMessage	msgout(M_SC_PROMOTION_PRICE);
	msgout.serial(idMaster, result, idRoom, idScene, bToUnlimited, need_price, last_price_id);

	CUnifiedNetwork::getInstance()->send(tsid, msgout);

	sipinfo("SUCCESS user(id:%d) cb_M_CS_GET_PROMOTION_PRICE(id:%d), Price=%d", idMaster, idRoom, need_price);
}

// Calc Room Promotion Price
// [d:Room id][w:Scene id]
void	cb_M_CS_GET_PROMOTION_PRICE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		idMaster;
	T_ROOMID		idRoom;
	uint16			idScene;
	uint8			bToUnlimited;
	_msgin.serial(idMaster, idRoom, idScene, bToUnlimited);

	if (!IsThisShardFID(idMaster))
	{
		sipwarning("IsThisShardFID = false. FID=%d", idMaster);
		return;
	}

	MAKEQUERY_GetRoomRenewHistory(strQ, idRoom);
	DBCaller->executeWithParam(strQ, DBCB_DBGetRoomRenewHistoryForGet, 
		"BDDDDWBW",
		OUT_,	NULL,
		OUT_,	NULL,
		OUT_,	NULL,
		CB_,	idMaster,
		CB_,	idRoom,
		CB_,	idScene,
		CB_,	bToUnlimited,
		CB_,	_tsid.get());
}

// Send Shard Code from WS to Lobby [[b:ShardMainCode]]
void	cb_M_W_SHARDCODE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	int count = 0;
	while (true)
	{
		_msgin.serial(ShardMainCode[count]);
		if (ShardMainCode[count] == 0)
			break;
		count ++;
	}

	sipdebug("Get Main count=%d, ShardCode[0] = %d", count, ShardMainCode[0]);
}

static TUnifiedCallbackItem Lobby_CallbackArray[] =
{
	//{ M_NT_LOGIN,				cb_M_NT_LOGIN_Lobby			},
	{ M_CS_ENTER_LOBBY,			cb_M_CS_ENTER_LOBBY },
	{ M_CS_LEAVE_LOBBY,			cb_M_CS_LEAVE_LOBBY },
	{ M_NT_LOGOUT,				cb_M_NT_LOGOUT_Lobby },
	{ M_NT_LEAVE_LOBBY,			cb_M_CS_LEAVE_LOBBY },	
	{ M_CS_FAMILY,				cb_M_CS_FAMILY	},
	{ M_CS_NEWFAMILY,			cb_M_CS_NEWFAMILY },	

	{ M_CS_NEWROOM,				cb_M_CS_NEWROOM },
	{ M_CS_ROOMCARD_NEW,		cb_M_CS_ROOMCARD_NEW },
	{ M_CS_PROMOTION_ROOM,		cb_M_CS_PROMOTION_ROOM_Lobby },
	//{ M_CS_ROOMCARD_PROMOTION,	cb_M_CS_ROOMCARD_PROMOTION_Lobby },

	{ M_ADDEDJIFEN,				cb_M_ADDEDJIFEN },
	{ M_NT_REMOVEJIFEN,			cb_M_NT_REMOVEJIFEN },
	{ M_MS_ROOMCARD_USE,		cb_M_MS_ROOMCARD_USE },
};

CLobbyWorld *	getLobbyWorld()
{	
	return pLobby;
}

void onWSConnection(const std::string &serviceName, TServiceId tsid, void *arg)
{
	// Get ShardCode
	CMessage	msgout(M_W_REQSHARDCODE);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

class CLobbyService : public IService
{
public:
	void init()
	{
		init_WorldService();
	
		pLobby = new CLobbyWorld;

		CUnifiedNetwork::getInstance()->setServiceUpCallback(WELCOME_SHORT_NAME, onWSConnection, NULL);
	}

	bool update ()
	{
		update_WorldService();
		return true;
	}

	void release()
	{
		release_WorldService();
	}
};

void run_lobby_service(int argc, const char **argv)
{
	CLobbyService *scn = new CLobbyService;
	scn->setArgs (argc, argv);
	createDebug (NULL, false);
	scn->setCallbackArray (Lobby_CallbackArray, sizeof(Lobby_CallbackArray)/sizeof(Lobby_CallbackArray[0]));
#ifdef SIP_OS_WINDOWS
#	if _MSC_VER >= 1900
		sint retval = scn->main(LOBBY_SHORT_NAME, LOBBY_LONG_NAME, "", 0, "", "", __DATE__ " " __TIME__);
#	else
		sint retval = scn->main(LOBBY_SHORT_NAME, LOBBY_LONG_NAME, "", 0, "", "", __DATE__" "__TIME__);
#	endif
#else
	string strDateTime = string(__DATE__) + " " + string(__TIME__);
	sint retval = scn->main(LOBBY_SHORT_NAME, LOBBY_LONG_NAME, "", 0, "", "", strDateTime.c_str());
#endif
	delete scn;
}

void run_lobby_service(LPSTR lpCmdLine)
{
	CLobbyService *scn = new CLobbyService;
	scn->setArgs (lpCmdLine);
	createDebug (NULL, false);
	scn->setCallbackArray (Lobby_CallbackArray, sizeof(Lobby_CallbackArray)/sizeof(Lobby_CallbackArray[0]));
#ifdef SIP_OS_WINDOWS
#	if _MSC_VER >= 1900
		sint retval = scn->main(LOBBY_SHORT_NAME, LOBBY_LONG_NAME, "", 0, "", "", __DATE__ " " __TIME__);
#	else
		sint retval = scn->main(LOBBY_SHORT_NAME, LOBBY_LONG_NAME, "", 0, "", "", __DATE__" "__TIME__);
#	endif
#else // SIP_OS_WINDOWS
	string strDateTime = string(__DATE__) + " " + string(__TIME__);
	sint retval = scn->main(LOBBY_SHORT_NAME, LOBBY_LONG_NAME, "", 0, "", "", strDateTime.c_str());
#endif
	delete scn;
}

