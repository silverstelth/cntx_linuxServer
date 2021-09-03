#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>

#include "Family.h"

#include "misc/db_interface.h"
#include "misc/DBCaller.h"
#include "../Common/QuerySetForShard.h"

#include "contact.h"
#include "Inven.h"
#include "room.h"
#include "mst_room.h"
#include "mst_item.h"
#include "openroom.h"
#include "Character.h"
#include "Lobby.h"
#include "outRoomKind.h"
#include "../Common/DBLog.h"
#include "../Common/netCommon.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

#include "Mail.h"

//extern	DBCONNECTION	DB;
extern CDBCaller	*DBCaller;

MAP_FAMILY				map_family;		// All Connected User List
std::map<T_FAMILYID, T_FAMILYNAME>		map_familyName;
void AddFamilyNameData(T_FAMILYID _FID, T_FAMILYNAME _FName)
{
	map_familyName[_FID] = _FName;
}
T_FAMILYNAME GetFamilyNameFromBuffer(T_FAMILYID _FID)
{
	std::map<T_FAMILYID, T_FAMILYNAME>::iterator it = map_familyName.find(_FID);
	if (it == map_familyName.end())
	{
		return L"";
	}
	return it->second;
}

// [10/29/2009 NamJuSong]
/************************************************************************/
/*		Process money, check sufficient and refresh to main site        */
/************************************************************************/

void	RefreshFamilyMoney(T_FAMILYID _FID)
{
	uint16	sid = SIPNET::IService::getInstance()->getServiceId().get();

	uint32	UserID;
	uint16	FesSID;

	FAMILY *	pFamily = GetFamilyData(_FID);
	if (pFamily != NULL)
	{
		UserID = pFamily->m_UserID;
		FesSID = pFamily->m_FES.get();
	}
	else
	{
		sipwarning("GetFamilyData = NULL. FID=%d", _FID);
		return;
	}

	CMessage msgout(M_LS_REFRESHMONEY);
	msgout.serial(UserID);
	msgout.serial(_FID);
	msgout.serial(sid);
	msgout.serial(FesSID);
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgout);
}

// Packet: M_LS_USERMONEY
void    cb_UserMoney(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint16		FesSID;		_msgin.serial(FesSID);
	T_PRICE		UMoney;		_msgin.serial(UMoney);

	bool	bChanged = false;
	uint32	umoney = UMoney;
	uint32	gmoney = 0;

	INFO_FAMILYINROOM*	pFamily = GetFamilyInfo(FID);
	if (pFamily != NULL)
	{
		bChanged = (UMoney != pFamily->m_nUMoney);

		// update info of family
		pFamily->m_nUMoney = UMoney;
		gmoney = pFamily->m_nGMoney;
		pFamily->m_RefreshMoneyTime = SIPBASE::CTime::getSecondsSince1970();

		// notify to client
		// if ( bChanged )
		{
			TServiceId	Fes(FesSID);
			SendFamilyProperty_Money(Fes, FID, gmoney, umoney);
		}
	}
}

// Packet: M_LS_CHECKPWD2_R
void    cb_CheckPwd2(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32	result;		_msgin.serial(result);
	uint32	FID;		_msgin.serial(FID);
	uint8	reason;		_msgin.serial(reason);
	uint32	RoomID;		_msgin.serial(RoomID);
	uint32	nextFID;	_msgin.serial(nextFID);

	switch(reason)
	{
	case 1:
		{
			CRoomWorld* roomManager = GetOpenRoomWorld(RoomID);
			if (roomManager == NULL)
			{
				sipwarning("Failed Get room manager(roomid=%d)", RoomID);
				return;
			}
			roomManager->ChangeRoomMaster_Step2(FID, nextFID, result);
		}
		break;
	};
}

static bool LoadUserNengLiang(uint32 FID, uint32 &OrgNengLiang)
{
	//uint32 CurNengLiang;
	MAKEQUERY_GetFamilyNengLiang(strQuery, FID);

	CDBConnectionRest	DB(DBCaller);
	DB_EXE_QUERY(DB, Stmt, strQuery);
	if (!nQueryResult)
	{
		return false;
	}

	DB_QUERY_RESULT_FETCH(Stmt, row);
	if (!IS_DB_ROW_VALID(row))
	{
		sipwarning("Cannot find Family Money. FID=%d", FID);
		return false;
	}

	COLUMN_DIGIT(row, 0, uint32, CurNengLiang);
	OrgNengLiang = CurNengLiang;

	return true;
}

bool	CheckMoneySufficient(T_FAMILYID _FID, T_PRICE UPrice, T_PRICE GPrice, uint8 MoneyType)
{
	INFO_FAMILYINROOM*	pFamily = GetFamilyInfo(_FID);
	if ( pFamily == NULL )
		return false;

	// Get NengLiang from DB      by krs
	uint32 OrgNengLiang;	
	if (!LoadUserNengLiang(_FID, OrgNengLiang))
	{
		sipwarning("LoadUserNengLiang failed. FID=%d", _FID);
		return false;
	}
	pFamily->m_nGMoney = OrgNengLiang;

	// if sufficient
	//sipinfo("total_umoney=%d, total_gmoney=%d, (%d, %d)", pFamily->m_nUMoney, pFamily->m_nGMoney, UPrice, GPrice);  byy krs

	if(MoneyType == MONEYTYPE_UMONEY)
	{
		if ( pFamily->m_nUMoney >= UPrice )
			return true;

		// get money info again from main site
		CheckRefreshMoney(_FID);
	}
	else
	{
		if ( pFamily->m_nGMoney >= GPrice )
			return true;
	}

	// notify to client
	uint32	umoney = pFamily->m_nUMoney;
	uint32	gmoney = pFamily->m_nGMoney;
	CMessage msgout(M_SC_MONEY_DEFICIT);
	msgout.serial(_FID, umoney, gmoney);
	CUnifiedNetwork::getInstance()->send(pFamily->m_ServiceID, msgout);

	return false;
}

//static void	DBCB_DBGetAllFestival(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID			FID	= *(T_FAMILYID *)argv[0];
//	sint32				ret	= *(sint32 *)argv[1];
//	TServiceId			tsid(*(uint16 *)argv[2]);
//
//	if (!nQueryResult || ret != 0)
//		return;
//
//	{
//		uint8	bStart, bEnd;
//		bStart = 1;
//		CMessage msgOut(M_SC_ALLFESTIVAL);
//		msgOut.serial(FID, bStart);
//
//		uint32 nRows;	resSet->serial(nRows);
//		for (uint32 i = 0; i < nRows; i ++)
//		{
//			// Packet의 길이가 너무 길어서 잘라서 보낸다.
//			if(msgOut.length() > 2500)
//			{
//				bEnd = 0;
//				msgOut.serial(bEnd);
//				CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//
//				CMessage	msgtmp(M_SC_ALLFESTIVAL);
//				bStart = 0;
//				msgtmp.serial(FID, bStart);
//
//				msgOut = msgtmp;
//			}
//
//			uint32	idFestival;			resSet->serial(idFestival);
//			T_NAME	ucName;				resSet->serial(ucName);
//			uint32	date;				resSet->serial(date);
//
//			msgOut.serial(idFestival);
//			msgOut.serial(date);
//			msgOut.serial(ucName);
//		}
//
//		bEnd = 1;
//		msgOut.serial(bEnd);
//		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//	}
//
//	CMessage msgAlarm(M_SC_ALL_FESTIVALALARM);	
//	msgAlarm.serial(FID);
//	uint32 lenth = msgAlarm.length();
//
//	uint32 nRows2;	resSet->serial(nRows2);
//	for (uint32 i = 0; i < nRows2; i ++)
//	{
//		T_FAMILYID	fid;				resSet->serial(fid);
//		uint8		festivalType;		resSet->serial(festivalType);
//		uint32		festiveId;			resSet->serial(festiveId);
//		uint8		remindDays;			resSet->serial(remindDays);
//
//		msgAlarm.serial(festivalType);
//		msgAlarm.serial(festiveId);
//		msgAlarm.serial(remindDays);
//	}
//
//	if (msgAlarm.length() > lenth)
//	{
//		CUnifiedNetwork::getInstance ()->send(tsid, msgAlarm);
//		sipinfo(L"-> [M_SC_ALL_FESTIVALALARM] Send User %d Festival remind days", FID);
//	}
//}

//static void	SendOwnFestivalInfo(T_FAMILYID familyId, SIPNET::TServiceId _tsid)
//{
//	MAKEQUERY_GETALLFESTIVAL(strQ);
//	DBCaller->executeWithParam(strQ, DBCB_DBGetAllFestival,
//		"DDW",
//		IN_ | CB_,			familyId,
//		OUT_,				NULL,
//		CB_,				_tsid.get());
//}


static void	DBCB_DBSaveFamilyGMoney(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if ( !nQueryResult )
		return;

	int				ret			= *(int *)argv[0];
	T_FAMILYID		FID			= *(T_FAMILYID *)argv[1];
	T_FAMILYNAME	Name		= (ucchar *)argv[2];
	T_PRICE			oldGMoney	= *(T_PRICE *)argv[3];
	T_PRICE			GMoney		= *(T_PRICE *)argv[4];

	if (ret == -1)
		return;

	DBLOG_CHFamilyGMoney(FID, Name, oldGMoney, GMoney);
}

// Notify about expending money to main site
// uFlag = 1 : Used, 2 : Added, 3 : RoomCard
void	ExpendMoney(T_FAMILYID FID, T_PRICE UPrice, T_PRICE GPrice,
					uint8 uFlag, uint16 MainType, uint8 SubType,
					uint32 itemId, uint32 ItemTypeId,
					uint32 VI1, uint32 VI2, uint32 VI3)
{
	// get family info
	FAMILY *	pFamily = GetFamilyData(FID);
	if ( pFamily == NULL )
	{
		sipdebug("GetFamilyData(%d) = NULL", FID);
		return;
	}

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if ( pFamilyInfo == NULL )
	{
		sipdebug("GetFamilyInfo(%d) = NULL", FID);
		return;
	}
	
	uint32 old_GMoney = pFamilyInfo->m_nGMoney;
	//sipwarning("LoadUserNengLiang : FID=%d, pFamilyInfo->m_nGMoney=%d", FID, pFamilyInfo->m_nGMoney);
	// subtract from family's money
	if (uFlag == 1)
	{
		pFamilyInfo->m_nUMoney -= UPrice;
		pFamilyInfo->m_nGMoney -= GPrice;
	}
	else if (uFlag == 2)
	{
		pFamilyInfo->m_nUMoney += UPrice;
		pFamilyInfo->m_nGMoney += GPrice;
	}
	else if (uFlag == 3)
	{
		// 천원카드사용시에 자은비를 추가해준다.
		pFamilyInfo->m_nUMoney += VI1;
	}

	T_ACCOUNTID UserID = pFamily->m_UserID;
	uint8 UserType = pFamily->m_UserType;

	if(GPrice != 0)
	{
		//sipwarning("GPrice != 0 : FID=%d, pFamilyInfo->m_nGMoney=%d", FID, pFamilyInfo->m_nGMoney);
		MAKEQUERY_SaveFamilyGMoney(strQ, FID, pFamilyInfo->m_nGMoney);
		DBCaller->executeWithParam(strQ, DBCB_DBSaveFamilyGMoney,
			"DDSDD",
			OUT_,			NULL,
			CB_,			FID,
			CB_,			pFamily->m_Name.c_str(),
			CB_,			old_GMoney,
			CB_,			pFamilyInfo->m_nGMoney);
	}

	// notify to main site
	if(UPrice != 0)
	{
		CMessage	msgout(M_LS_EXPENDMONEY);
		msgout.serial(uFlag, UPrice);
		msgout.serial(MainType, SubType, UserID, UserType, FID);
		msgout.serial(itemId, ItemTypeId, VI1, VI2, VI3);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgout);
	}
}

//struct	USEMONEYRESULT
//{
//	USEMONEYRESULT(uint32 _uMesID) : bResult(false), uMesID(_uMesID) {}
//	bool		bResult;
//	uint32		uMesID;
//	uint32		uResult;
//	T_PRICE		uMoney;
//};
//map<uint32, USEMONEYRESULT>	map_USEMONEYRESULT;
//
//bool	NotifyToMainSiteForUseMoney(T_FAMILYID _FID, T_PRICE _UMoney, T_PRICE &_TotalUMoney, uint8 _uFlag, bool _bUserID)
//{
//	static	uint32	uUseMoneyMessageID = 1;
//
//	T_ACCOUNTID	UserID = _FID;
//	uint8		UserType = 0;
//	if (!_bUserID)
//	{
//		FAMILY	*pFamily = GetFamilyData(_FID);
//		if (pFamily == NULL)
//			return false;
//		UserID = pFamily->m_UserID;
//		UserType = pFamily->m_UserType;
//	}
//
//	uint8		uFlag = _uFlag;
//	uint32		uMesID = uUseMoneyMessageID ++;
//
//	CMessage mesWS(M_LS_CANUSEMONEY);
//	mesWS.serial(uMesID, uFlag, _UMoney, UserID);
//	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, mesWS);
//
//	map_USEMONEYRESULT.insert(make_pair(uMesID, USEMONEYRESULT(uMesID)));
//
//	TTime	tmStart = CTime::getLocalTime();
//	while (true)
//	{
//		CUnifiedNetwork::getInstance()->update(50);
//
//		map<uint32, USEMONEYRESULT>::iterator it = map_USEMONEYRESULT.find(uMesID);
//		if (it == map_USEMONEYRESULT.end())
//			return false;
//
//		if (it->second.bResult)
//		{			
//			if (it->second.uResult == ERR_NOERR)
//			{
//				_TotalUMoney = it->second.uMoney;
//				map_USEMONEYRESULT.erase(uMesID);
//				return true;
//			}
//			else
//			{
//				sipwarning("Can use money error : MesID=%d, UserID=%d, MoneyExpend=%d, Error=%d", uMesID, UserID, _UMoney, it->second.uResult);
//				map_USEMONEYRESULT.erase(uMesID);
//				return false;
//			}
//		}
//		if (CTime::getLocalTime() - tmStart > 5000)
//		{
//			sipwarning("There is no usemoney result : MesID=%d, UserID=%d, MoneyExpend=%d", uMesID, UserID, _UMoney);
//			map_USEMONEYRESULT.erase(uMesID);
//			return false;
//		}
//		sipSleep(10);
//	}
//	return false;
//}

//////////////////////////////////////////////////////////////////////////

//T_F_LEVEL MakeDegreeFromVDays(T_F_VISITDAYS _vdays)
//{
//	double dv = (double)_vdays / 0.365;
//	double dvsqrt = sqrt(dv);
//	T_F_LEVEL uLevel = (T_F_LEVEL)dvsqrt;
//	return uLevel;
//}

//T_F_VISITDAYS MakeVDaysFromDegree(T_F_LEVEL _level)
//{
//	return (T_F_VISITDAYS)(_level * _level * 0.365);
//}

// 가족정보를 추가한다
void	AddFamilyData(T_FAMILYID _FID, T_ACCOUNTID _UserID, T_FAMILYNAME _FName, ucstring _UserName, uint8 _bIsMobile,
					uint8 _UserType, bool _bGM, string strIP, SIPBASE::TTime _logintime)
{
	std::map<T_FAMILYID, FAMILY>::iterator it = map_family.find(_FID);
	if (it != map_family.end())
	{
		sipwarning("map_family.find != end. FID=%d", _FID);
		map_family.erase(it);
	}
//	if (map_family.find(_FID) == map_family.end())
	{
		FAMILY	f(_FID, _FName, _UserID, _UserName, _bIsMobile, _UserType, _bGM, strIP, _logintime);

		map_family.insert( make_pair(_FID, f));

		//sipdebug("map_family Added. FID=%d, size=%d", _FID, map_family.size()); byy krs
	}
	AddFamilyNameData(_FID, _FName);

	/*map_family[_FID] = f;*/
	
}

//void	SetFamilyState(T_FAMILYID _FID, T_SESSIONSTATE	state)
//{
//	MAP_FAMILY::iterator	it = map_family.find(_FID);
//	if ( it != map_family.end() )
//	{
//		FAMILY	&family = it->second;
//		family.m_SessionState = state;
//		sipdebug("SUCCESSED Setting Family State : family id:%d, state:%d", _FID, state);
//	}
//	else
//		sipwarning("FAILED Setting Family State : There is no family id : %d", _FID);
//}

// 가족정보를 삭제한다
void	DeleteFamilyData(T_FAMILYID _FID)
{
//===	DelCharacterData(_FID); // 캐릭터정보는 여기에서 삭제할 필요가 없다. ch_leave. OutRoom에서 삭제된다. 또한 FID가 아니라 CHID로 호출하여야 한다.

	MAP_FAMILY::iterator it = map_family.find(_FID);
	if (it != map_family.end())
	{
		map_family.erase(_FID);

		//sipdebug("map_family Deleted. FID=%d, remain_size=%d", _FID, map_family.size());  byy krs
	}
}

// 가족의 online정보를 얻는다
sint8	GetFamilyOnlineState(T_FAMILYID _FID)
{
	MAP_FAMILY::iterator it = map_family.find(_FID);
	if (it == map_family.end())
		return CH_OFFLINE;
	return CH_ONLINE;
}

//T_SESSIONSTATE	GetFamilySessionState(T_FAMILYID _FID)
//{
//	MAP_FAMILY::iterator it = map_family.find(_FID);
//	if (it == map_family.end())
//	{
//		sipdebug("FAILED Getting Family Session State : There is no family id : %d", _FID);
//		return 0;
//	}
//	
//	sipinfo("SUCCESSED Getting Family Session State : %u, family id : %d", it->second.m_SessionState, _FID);
//	return	it->second.m_SessionState;
//}

// 가족정보를 얻는다
FAMILY	*GetFamilyData(T_FAMILYID _FID)
{
	MAP_FAMILY::iterator it = map_family.find(_FID);
	if (it == map_family.end())
		return NULL;
	return &(it->second);
}

INFO_FAMILYINROOM* GetFamilyInfo(T_FAMILYID _FID)
{
	CWorld	*pWorld = GetWorldFromFID(_FID);
	if(pWorld == NULL)
		return NULL;

	return pWorld->GetFamilyInfo(_FID);
}

T_ROOMID	GetFamilyWorld(T_FAMILYID _FID, TWorldType * pType)
{
	MAP_FAMILY::iterator it = map_family.find(_FID);
	if (it == map_family.end())
		return NOROOMID;
	if ( pType != NULL )
		*pType = it->second.m_WorldType;
	return it->second.m_WorldId;
}

void	SetFamilyWorld(T_FAMILYID _FID, T_ROOMID _RoomID, TWorldType _Type)
{
	MAP_FAMILY::iterator it = map_family.find(_FID);
	if (it == map_family.end())
		return;
	FAMILY *family = &(it->second);
	family->m_WorldId = _RoomID;
	family->m_WorldType = _Type;
}

TServiceId		GetFesForFamilyID(T_FAMILYID _FID)
{
	MAP_FAMILY::iterator it = map_family.find(_FID);
	if (it == map_family.end())
		return TServiceId(0);
	return it->second.m_FES;

}
// 가족이 접속한 접속써비스를 설정한다
void	SetFesForFamilyID(T_FAMILYID _FID, TServiceId _FES)
{
	MAP_FAMILY::iterator it = map_family.find(_FID);
	if (it == map_family.end())
		return;
	FAMILY *family = &(it->second);
	family->m_FES = _FES;
}


//void	SetFamilyOnlineFlag(T_FAMILYID _FID, bool _bOnline, ucstring _IP)
//{
//	MAKEQUERY_LOGINFAMILY(strQ, _FID, _bOnline); // , _IP.c_str());
//	DBCaller->execute(strQ, NULL);
//}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//// 가족과 관련한 초기정보를 내려보낸다
//static void	SendFamilyAllInfo(T_FAMILYID _FID, TServiceId _tsid)
//{
//	// ==========Master Datas==========
//	// List of room price
//	SendMstRoomData(_FID, _tsid);
//	// List of item price
//	SendMstItemData(_FID, _tsid);
//	// List of history space price
//	SendMstHisSpaceData(_FID, _tsid);
//	// Send Room Level's Exp
//	SendMstRoomLevelExpData(_FID, _tsid);
//	// Send Family Level's Exp
//	SendMstFamilyLevelExpData(_FID, _tsid);
//	// Send Room Fish's data
//	SendMstFishData(_FID, _tsid);
//
//	// =========Family Datas==========
//	// Characters
//	SendCharacterInfo(_FID, _tsid);
//
//	// List of my rooms
//	SendOwnRoomInfo(_FID, _tsid);
//	// Send his all festival infos
//	SendOwnFestivalInfo(_FID, _tsid);
//
//	// Send his all festival
////	SendOwnDeadMemorialDaysInfo(_FID, _tsid);
//
//	// send contact list
////Move to Manager Service	SendContactlist(_FID, _tsid);
//	// Send Chit
//	SendAllChit(_FID, _tsid);
//
//	// Notify Contact Online State
//	//NotifyOnlineToHisRegContacts(_FID, CONTACTSTATE_ONLINE);
//}

// 새로 로그인하는 사용자에게 master 정보를 내려보낸다.
static void	SendAllMasterDatas(T_FAMILYID _FID, TServiceId _tsid)
{
	// List of room price
	SendMstRoomData(_FID, _tsid);
	// List of item price
	SendMstItemData(_FID, _tsid);
	// List of history space price
	SendMstHisSpaceData(_FID, _tsid);
	// Send Room Level's Exp
	SendMstRoomLevelExpData(_FID, _tsid);
	// Send Family Level's Exp
	SendMstFamilyLevelExpData(_FID, _tsid);
	// Send Config Data
	SendMstConfigData(_FID, _tsid);
	// Send Room Fish's data
	SendMstFishData(_FID, _tsid);
}

//void	Family_M_CS_LOGOUT(T_FAMILYID _FID, TServiceId _tsid)
//{
//	uint32	ontime = 0;
//	FAMILY	*family = GetFamilyData(_FID);
//	if (family == NULL)
//		return;
//	if (family->m_loginTime != 0)
//		ontime = (uint32)((CTime::getLocalTime() - family->m_loginTime) / 1000 / 60);
//}

void	SendFamilyProperty_Level(SIPNET::TServiceId _sid, T_FAMILYID _FID, T_F_EXP _exp, T_F_LEVEL _level)
{
	// To Frontend service
	CMessage	msgFS(M_SC_FPROPERTY_LEVEL);
	//uint32	MaxExp;
	//uint16	ExpPercent = CalcFamilyExpPercent(_level, _exp, &MaxExp);
	msgFS.serial(_FID, _exp); // , MaxExp, _level, ExpPercent);
	CUnifiedNetwork::getInstance()->send(_sid, msgFS);
}

void	SendFamilyProperty_Money(SIPNET::TServiceId _sid, T_FAMILYID _FID, T_PRICE _GMoney, T_PRICE _UMoney)
{
	CMessage	msgFS(M_SC_FPROPERTY_MONEY);
	msgFS.serial(_FID, _GMoney, _UMoney);
	CUnifiedNetwork::getInstance()->send(_sid, msgFS);
}


//static void	DBCB_DBGetFamilyOnUserIDForLogin(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	if (!nQueryResult)
//		return;
//
//	T_FAMILYID		FID			= *(T_FAMILYID *)argv[0];
//	uint32			Code		= *(uint32 *)argv[1];
//	T_ACCOUNTID		uID			= *(T_ACCOUNTID *)argv[2];
//	uint8			newuser		= *(uint8 *)argv[3];
//	uint8			bIsMobile	= *(uint8 *)argv[4];
//	string			sAddress	= (char *)argv[5];
//	bool			bGM			= *(bool *)argv[6];
//	uint8		    uUserType	= *(uint8 *)argv[7];
//	T_COMMON_NAME	UserName	= (ucchar *)argv[8];
//	TServiceId		tsid(*(uint16 *)argv[9]);
//
//	bool	bSystemUser = IsSystemAccount(uID);
//	uint8	PropertyCode = (uint8)bSystemUser;
//	if (Code == 1002)
//	{
//		CMessage	msgOut(M_SC_FAMILY);
//		msgOut.serial(uID, PropertyCode, FID);
//		sipinfo(L"Send family no info to account = %d", uID);
//		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//		return;
//	}
//	if (Code == 1066)
//	{
//		CMessage	msgOut(M_SC_FAMILY);
//		PropertyCode = 2;
//		msgOut.serial(uID, PropertyCode, FID);
//		sipinfo(L"Send family fengting info to account = %d", uID);
//		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//		return;
//	}
//
//	if (FID == 0)
//	{
//		sipwarning("Invalid FID! FID=0: UID=%d, newuser=%d, bIsMobile=%d", uID, newuser, bIsMobile);
//		return;
//	}
//
//	uint32 nRows;
//
//	// tbl_Family
//	resSet->serial(nRows);
//	if (nRows != 1)
//	{
//		sipwarning("nRows != 1 in tbl_Family. FID=%d", FID);
//		return;
//	}
//	T_FAMILYID			fid;				resSet->serial(fid);
//	T_FAMILYNAME		FamilyName;			// resSet->serial(FamilyName);
//	T_PRICE				GMoney;				resSet->serial(GMoney);
//	T_F_EXP				Exp;				resSet->serial(Exp);
//	T_F_FAME			Fame;				resSet->serial(Fame);
//	T_F_VISITDAYS		vdays;				resSet->serial(vdays);
//	T_F_LEVEL			uLevel;				resSet->serial(uLevel);
//	uint8				FreeUsed;			resSet->serial(FreeUsed);
//
//	// tbl_Character
//	resSet->serial(nRows);
//	if (nRows != 1)
//	{
//		sipwarning("nRows != 1 in tbl_Character. FID=%d", FID);
//		return;
//	}
//	T_CHARACTERID	uid;			resSet->serial(uid);
//	T_COMMON_NAME	name;			resSet->serial(name);		FamilyName = name;
//	T_MODELID		modelid;		resSet->serial(modelid);
//	T_DRESS			ddressid;		resSet->serial(ddressid);
//	T_FACEID		faceid;			resSet->serial(faceid);
//	T_HAIRID		hairid;			resSet->serial(hairid);
//	T_DATETIME		createdate;		resSet->serial(createdate);
//	T_ISMASTER		master;			resSet->serial(master);
//	T_DRESS			dressid;		resSet->serial(dressid);
//	T_DATETIME		changedate;		resSet->serial(changedate);
//	T_CHBIRTHDATE	birthDate;		resSet->serial(birthDate);
//	T_CHCITY		city;			resSet->serial(city);
//	T_COMMON_CONTENT resume;		resSet->serial(resume);
//	T_F_LEVEL		fLevel;			fLevel = uLevel; // resSet->serial(fLevel);
//	T_F_EXP			fExp;			fExp = Exp; // resSet->serial(fExp);
//
//	{
//		CMessage	msgOut(M_SC_FAMILY);
//		msgOut.serial(uID, PropertyCode, FID);
//
//		msgOut.serial(FamilyName, bIsMobile, modelid, uLevel, Exp, FreeUsed, resume);
//
//		uint32	cur_time = (uint32)GetDBTimeSecond();
//		msgOut.serial(cur_time);
//
//		sipinfo(L"Send family info to account = %d", uID);
//		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//	}
//
//	{
//		CMessage	msgOut(M_SC_ALLCHARACTER);
//		msgOut.serial(FID, FID);
//		msgOut.serial(fLevel, fExp);
//		msgOut.serial(uid, name, modelid, ddressid, faceid, hairid, createdate, master);
//		msgOut.serial(dressid, changedate);
//		msgOut.serial(birthDate, city, resume);
//		sipinfo(L"Send character info %u to %u", FID, FID);
//		CUnifiedNetwork::getInstance ()->send(tsid, msgOut);
//	}
//
//	// Own Room Info
//	{
//		uint8	bStart, bEnd;
//		CMessage	msgOut(M_SC_OWNROOM);
//		bStart = 1;
//		msgOut.serial(FID, bStart);
//
//		uint32	nCurtime = (uint32)(GetDBTimeSecond() / 60);
//
//		uint16	nCount = 0;
//		uint32 nRows;	resSet->serial(nRows);
//		for (uint32 i = 0; i < nRows; i ++)
//		{
//			// Packet의 길이가 너무 길어서 잘라서 보낸다.
//			if (msgOut.length() > 2500)
//			{
//				bEnd = 0;
//				msgOut.serial(bEnd);
//				CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//
//				CMessage	msgtmp(M_SC_OWNROOM);
//				bStart = 0;
//				msgtmp.serial(FID, bStart);
//
//				msgOut = msgtmp;
//			}
//
//			uint32			curGiveId;				resSet->serial(curGiveId);
//			T_DATETIME		curGiveTime;			resSet->serial(curGiveTime);
//			T_MSTROOM_ID	SceneID;				resSet->serial(SceneID);
//			T_DATETIME		creattime;				resSet->serial(creattime);
//			T_DATETIME		termtime;				resSet->serial(termtime);
//			T_ROOMID		id;						resSet->serial(id);
//			T_ROOMNAME		roomName;				resSet->serial(roomName);
//			T_ROOMPOWERID	opentype;				resSet->serial(opentype);
//			string			openPwd;				resSet->serial(openPwd);
//			bool			freeuse;				resSet->serial(freeuse);
//			bool			bclosed;				resSet->serial(bclosed);
//			T_DATETIME			renewtime;			resSet->serial(renewtime);
//			T_COMMON_CONTENT	comment;			resSet->serial(comment);
//			uint32				visits;				resSet->serial(visits);
//			uint8				deleteflag;			resSet->serial(deleteflag);
//			T_DATETIME			deletetime;			resSet->serial(deletetime);
//			uint32		spacecapacity;				resSet->serial(spacecapacity);
//			uint32		spaceuse;					resSet->serial(spaceuse);
//			T_ROOMLEVEL	level;						resSet->serial(level);
//			T_ROOMEXP	exp;						resSet->serial(exp);
//			uint8				DeadSurnameLen1;	resSet->serial(DeadSurnameLen1);
//			T_DECEASED_NAME		Deadname1;			resSet->serial(Deadname1);
//			uint32		DeadBirthday1;				resSet->serial(DeadBirthday1);
//			uint32		DeadDeadday1;				resSet->serial(DeadDeadday1);
//			uint8				DeadSurnameLen2;	resSet->serial(DeadSurnameLen2);
//			T_DECEASED_NAME		Deadname2;			resSet->serial(Deadname2);
//			uint32		DeadBirthday2;				resSet->serial(DeadBirthday2);
//			uint32		DeadDeadday2;				resSet->serial(DeadDeadday2);
//			uint8		_SamePromotionCount;		resSet->serial(_SamePromotionCount);
//			uint8		PhotoType;					resSet->serial(PhotoType);
//
//			uint32	nCreateTime = getMinutesSince1970(creattime);
//			uint32	nRenewTime = getMinutesSince1970(renewtime);
//			uint32	nTermTime = getMinutesSince1970(termtime);
//			uint32	nDeleteRemainMinutes = 0;
//			if (deleteflag)
//			{
//				// deleteflag: 1 - 삭제중 회복가능, 2 - 완전삭제됨
//				if (deleteflag != 1)
//					continue;
//
//				uint32 nDeleteTime = getMinutesSince1970(deletetime);
//				if (nCurtime >= nDeleteTime)
//					continue;
//				nDeleteRemainMinutes = nDeleteTime - nCurtime;
//			}
//			uint32 nCurGiveTime = 0;
//			if (curGiveId != 0)
//				nCurGiveTime = getMinutesSince1970(curGiveTime);
//
//			if (Deadname1 == L"")
//				DeadSurnameLen1 = 0;
//			if (Deadname2 == L"")
//				DeadSurnameLen2 = 0;
//
//			//uint16	exp_percent = CalcRoomExpPercent(level, exp);
//
//			//		T_FLAG	uState = bclosed ? ROOMSTATE_CLOSED : ROOMSTATE_NORMAL;
//
//			msgOut.serial(id, SceneID, nCreateTime, nTermTime, nRenewTime);
//			msgOut.serial(roomName, opentype, openPwd, freeuse, visits, comment, nDeleteRemainMinutes);
//			msgOut.serial(spacecapacity, spaceuse, level, exp/*, exp_percent*/);
//			msgOut.serial(curGiveId, nCurGiveTime, PhotoType);
//			msgOut.serial(Deadname1, DeadSurnameLen1, DeadBirthday1, DeadDeadday1);
//			msgOut.serial(Deadname2, DeadSurnameLen2, DeadBirthday2, DeadDeadday2);
//			uint32		SamePromotionCount = (uint32)_SamePromotionCount;
//			msgOut.serial(SamePromotionCount);
//
//			nCount++;
//		}
//
//		bEnd = 1;
//		msgOut.serial(bEnd);
//		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//		sipinfo(L"-> [M_SC_OWNROOM] Request My Own Creating Room num : %u, size=%d", nCount, msgOut.length());
//	}
//
//	// Festival Info
//	{
//		uint8	bStart, bEnd;
//		bStart = 1;
//		CMessage msgOut(M_SC_ALLFESTIVAL);
//		msgOut.serial(FID, bStart);
//
//		uint32 nRows;	resSet->serial(nRows);
//		for (uint32 i = 0; i < nRows; i ++)
//		{
//			// Packet의 길이가 너무 길어서 잘라서 보낸다.
//			if(msgOut.length() > 2500)
//			{
//				bEnd = 0;
//				msgOut.serial(bEnd);
//				CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//
//				CMessage	msgtmp(M_SC_ALLFESTIVAL);
//				bStart = 0;
//				msgtmp.serial(FID, bStart);
//
//				msgOut = msgtmp;
//			}
//
//			uint32	idFestival;			resSet->serial(idFestival);
//			T_NAME	ucName;				resSet->serial(ucName);
//			uint32	date;				resSet->serial(date);
//
//			msgOut.serial(idFestival);
//			msgOut.serial(date);
//			msgOut.serial(ucName);
//		}
//
//		bEnd = 1;
//		msgOut.serial(bEnd);
//		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//	}
//
//	{
//		CMessage msgOut(M_SC_ALL_FESTIVALALARM);
//		msgOut.serial(FID);
//		uint32 lenth = msgOut.length();
//
//		uint32 nRows;	resSet->serial(nRows);
//		for (uint32 i = 0; i < nRows; i ++)
//		{
//			uint8		festivalType;		resSet->serial(festivalType);
//			uint32		festiveId;			resSet->serial(festiveId);
//			uint8		remindDays;			resSet->serial(remindDays);
//
//			msgOut.serial(festivalType);
//			msgOut.serial(festiveId);
//			msgOut.serial(remindDays);
//		}
//
//		if (msgOut.length() > lenth)
//		{
//			CUnifiedNetwork::getInstance ()->send(tsid, msgOut);
//			sipinfo(L"-> [M_SC_ALL_FESTIVALALARM] Send User %d Festival remind days", FID);
//		}
//	}
//
//	// Chit List
//	{
//		struct	_CHIT
//		{
//			_CHIT(){};
//			_CHIT(uint32 _id, T_FAMILYID _idSender, T_FAMILYID _idReciever, uint8	_idType, ucstring _nameSender, uint32 _p1, uint32 _p2, ucstring _p3, ucstring _p4)
//			{
//				id = _id;
//				idSender	= _idSender;
//				idReciever	= _idReciever;
//				idChatType	= _idType;
//				nameSender	= _nameSender;
//				p1 = _p1;
//				p2 = _p2;
//				p3 = _p3;
//				p4 = _p4;
//			};
//			uint32			id;
//			T_FAMILYID		idSender;
//			ucstring		nameSender;
//			T_FAMILYID		idReciever;
//			uint8			idChatType;
//			uint32			p1;
//			uint32			p2;
//			ucstring		p3;
//			ucstring		p4;
//		};
//
//		vector<_CHIT>	aryChit;
//		{
//			uint32		nRows;			resSet->serial(nRows);
//			for (uint32 i = 0; i < nRows; i++)
//			{
//				T_FAMILYID		idSender;		resSet->serial(idSender);
//				uint8			idType;			resSet->serial(idType);
//				uint32			p1;				resSet->serial(p1);
//				uint32			p2;				resSet->serial(p2);
//				ucstring		p3;				resSet->serial(p3);
//				ucstring		p4;				resSet->serial(p4);
//				uint32			id;				resSet->serial(id);
//				T_FAMILYNAME	nameSender;		resSet->serial(nameSender);
//
//				if (idType != CHITTYPE_CHROOMMASTER)
//				{
//					_CHIT	chit(id, idSender, FID, idType, nameSender, p1, p2, p3, p4);
//					aryChit.push_back(chit);
//				}
//			}
//		}
//
//		// 련계인 신청Chit는 련계인목록이 내려간 다음에 내려가야 하므로 여기에서 처리하지 않고 ManagerService로 옮긴다.
//		// 그러나 현재의 SP는 수정하지 않았으므로 resSet에서 자료를 뽑아내는 부분은 그대로 둔다.
//		//uint32 nAllChitSize = aryChit.size();
//		//uint32 nChitCount = 0;
//		//uint32 nGroup = 0;
//		//while (nChitCount < nAllChitSize)
//		//{
//		//	uint16		r_sid = SIPNET::IService::getInstance()->getServiceId().get();
//		//	CMessage	msgOut(M_SC_CHITLIST);
//		//	msgOut.serial(FID, r_sid);
//		//	uint32 num = (nAllChitSize - nChitCount > 20) ? 20 : (nAllChitSize - nChitCount);
//		//	msgOut.serial(num);
//		//	nGroup = 0;
//		//	while (true)
//		//	{
//		//		msgOut.serial(aryChit[nChitCount].id, aryChit[nChitCount].idSender, aryChit[nChitCount].nameSender, aryChit[nChitCount].idChatType);
//		//		msgOut.serial(aryChit[nChitCount].p1, aryChit[nChitCount].p2, aryChit[nChitCount].p3, aryChit[nChitCount].p4);
//		//		nGroup ++;
//		//		nChitCount ++;
//
//		//		if (nGroup >= num)
//		//			break;
//		//	}
//		//	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//		//}
//		//sipdebug("Send ChitList, num : %u", nAllChitSize);
//
//		//// 1회용 Chit들은 삭제한다.
//		//uint8	bDelete = 1;
//		//for (nChitCount = 0; nChitCount < nAllChitSize; nChitCount ++)
//		//{
//		//	switch(aryChit[nChitCount].idChatType)
//		//	{
//		//	case CHITTYPE_MANAGER_ADD:
//		//	case CHITTYPE_MANAGER_ANS:
//		//	case CHITTYPE_MANAGER_DEL:
//		//	case CHITTYPE_GIVEROOM_RESULT:
//		//	case CHITTYPE_ROOM_DELETED:
//		//		{
//		//			MAKEQUERY_GETCHIT(strQ, aryChit[nChitCount].id, FID, bDelete);
//		//			DBCaller->execute(strQ, NULL);
//		//		}
//		//		break;
//		//	}
//		//}
//	}
//
//	// 양도받는 방목록
//	{
//		uint32 nRows;	resSet->serial(nRows);
//		for (uint32 i = 0; i < nRows; i ++)
//		{
//			T_ROOMID		RoomID;		resSet->serial(RoomID);
//			T_SCENEID		SceneID;	resSet->serial(SceneID);
//			T_COMMON_NAME	RoomName;	resSet->serial(RoomName);
//			T_FAMILYID		OwnerID;	resSet->serial(OwnerID);
//			T_FAMILYNAME	OwnerName;	resSet->serial(OwnerName);
//			T_DATETIME		curGiveTime;resSet->serial(curGiveTime);
//			uint32 nCurGiveTime = getMinutesSince1970(curGiveTime);
//
//			CMessage	msgOut(M_SC_CHANGEMASTER_REQ);
//			msgOut.serial(FID, RoomID, SceneID, RoomName, OwnerID, OwnerName, nCurGiveTime);
//			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//		}
//	}
//
//	SendAllMasterDatas(FID, tsid);
//
//	AddFamilyData(FID, uID, FamilyName, UserName, bIsMobile, uUserType, bGM, sAddress, CTime::getLocalTime());
//
//	// Manager Service에 통지한다.
//	//{
//	//	uint16	clientFsSid = tsid.get();
//	//	CMessage	msgOut(M_NT_PLAYER_LOGIN);
//	//	msgOut.serial(FID, uID, FamilyName, clientFsSid);
//	//	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
//	//}
//
//	if (newuser == 1)
//	{
//		// Check if exist NewUser's BankItem
//		// CMessage	msgOut1(M_SM_BANKITEM_GET_NUSER);
//		// msgOut1.serial(FID, uID);
//		// CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut1);
//
//		DBLOG_CreateFamily(FID, FamilyName, uID, UserName);
//	}
//
//	// Lobby진입처리
//	CLobbyWorld	*pLobby = getLobbyWorld();
//	if (pLobby == NULL)
//	{
//		sipwarning("getLobbyWorld = NULL. FID=%d", FID);
//		return;
//	}
//
//	pLobby->EnterFamilyInLobby(FID, tsid, 1); // 1 = bReqItemRefresh
////	ItemRefresh(FID, tsid); // 동기화처리 필요!!!!!! ============ 처리 필요
//}

static void	DBCB_DBGetFamilyAllInfoForLogin_Lobby(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	T_FAMILYID		FID			= *(T_FAMILYID *)argv[0];
	uint32			Code		= *(uint32 *)argv[1];
	T_ACCOUNTID		uID			= *(T_ACCOUNTID *)argv[2];
	uint8			newuser		= *(uint8 *)argv[3];
	uint8			bIsMobile	= *(uint8 *)argv[4];
	string			sAddress	= (char *)argv[5];
	bool			bGM			= *(bool *)argv[6];
	uint8		    uUserType	= *(uint8 *)argv[7];
	T_COMMON_NAME	UserName	= (ucchar *)argv[8];
	TServiceId		tsid(*(uint16 *)argv[9]);

	bool	bSystemUser = IsSystemAccount(uID);
	uint8	PropertyCode = (uint8)bSystemUser;
	if (Code == 1002)
	{
		CMessage	msgOut(M_SC_FAMILY);
		msgOut.serial(uID, PropertyCode, FID);
		sipinfo(L"Send family no info to account = %d", uID);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		return;
	}
	if (Code == 1066)
	{
		CMessage	msgOut(M_SC_FAMILY);
		PropertyCode = 2;
		msgOut.serial(uID, PropertyCode, FID);
		sipinfo(L"Send family fengting info to account = %d", uID);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		return;
	}

	if (FID == 0)
	{
		sipwarning("Invalid FID! FID=0: UID=%d, newuser=%d, bIsMobile=%d", uID, newuser, bIsMobile);
		return;
	}

	uint32 nRows;

	// tbl_Family
	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("nRows != 1 in tbl_Family. FID=%d", FID);
		return;
	}
	T_FAMILYID			fid;				resSet->serial(fid);
	T_FAMILYNAME		FamilyName;			// resSet->serial(FamilyName);
	T_PRICE				GMoney;				resSet->serial(GMoney);
	T_F_EXP				Exp;				resSet->serial(Exp);
	T_F_FAME			Fame;				resSet->serial(Fame);
	T_F_VISITDAYS		vdays;				resSet->serial(vdays);
	T_F_LEVEL			uLevel;				resSet->serial(uLevel);
	uint8				FreeUsed;			resSet->serial(FreeUsed);

	// tbl_Character
	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("nRows != 1 in tbl_Character. FID=%d", FID);
		return;
	}
	T_CHARACTERID	uid;			resSet->serial(uid);
	T_COMMON_NAME	name;			resSet->serial(name);		FamilyName = name;
	T_MODELID		modelid;		resSet->serial(modelid);
	T_DRESS			ddressid;		resSet->serial(ddressid);
	T_FACEID		faceid;			resSet->serial(faceid);
	T_HAIRID		hairid;			resSet->serial(hairid);
	T_DATETIME		createdate;		resSet->serial(createdate);
	T_ISMASTER		master;			resSet->serial(master);
	T_DRESS			dressid;		resSet->serial(dressid);
	T_DATETIME		changedate;		resSet->serial(changedate);
	T_CHBIRTHDATE	birthDate;		resSet->serial(birthDate);
	T_CHCITY		city;			resSet->serial(city);
	T_COMMON_CONTENT resume;		resSet->serial(resume);
	T_F_LEVEL		fLevel;			fLevel = uLevel; // resSet->serial(fLevel);
	T_F_EXP			fExp;			fExp = Exp; // resSet->serial(fExp);

	{
		CMessage	msgOut(M_SC_FAMILY);
		msgOut.serial(uID, PropertyCode, FID);

		msgOut.serial(FamilyName, bIsMobile, modelid, uLevel, Exp, FreeUsed, resume);

		uint32	cur_time = (uint32)GetDBTimeSecond();
		msgOut.serial(cur_time);

		sipinfo(L"Send family info to account = %d", uID);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}

	{
		CMessage	msgOut(M_SC_ALLCHARACTER);
		msgOut.serial(FID, FID);
		msgOut.serial(fLevel, fExp);
		msgOut.serial(uid, name, modelid, ddressid, faceid, hairid, createdate, master);
		msgOut.serial(dressid, changedate);
		msgOut.serial(birthDate, city, resume);
		sipinfo(L"Send character info %u to %u", FID, FID);
		CUnifiedNetwork::getInstance ()->send(tsid, msgOut);
	}

	AddFamilyData(FID, uID, FamilyName, UserName, bIsMobile, uUserType, bGM, sAddress, CTime::getLocalTime());

	if (newuser == 1)
	{
		DBLOG_CreateFamily(FID, FamilyName, uID, UserName);
	}

	// Lobby진입처리
	CLobbyWorld	*pLobby = getLobbyWorld();
	if (pLobby == NULL)
	{
		sipwarning("getLobbyWorld = NULL. FID=%d", FID);
		return;
	}

	pLobby->EnterFamilyInLobby(FID, tsid, 1); // 1 = bReqItemRefresh
}

void cb_M_CS_FAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ACCOUNTID		uID;		_msgin.serial(uID);
	uint8			bIsMobile;	_msgin.serial(bIsMobile);
	string			sAddress;	_msgin.serial(sAddress);
	bool		    bGM;		_msgin.serial(bGM);
	uint8		    uUserType;	_msgin.serial(uUserType);
	T_COMMON_NAME	UserName;	_msgin.serial(UserName);

	uint8	newuser = 0;
	//MAKEQUERY_GetFamilyOnUserIDForLogin(strQuery, uID);
	//DBCaller->executeWithParam(strQuery, DBCB_DBGetFamilyOnUserIDForLogin,
	//	"DDDBBsBBSW",
	//	OUT_,		NULL,
	//	OUT_,		NULL,
	//	CB_,		uID,
	//	CB_,		newuser,
	//	CB_,		bIsMobile,
	//	CB_,		sAddress.c_str(),
	//	CB_,		bGM,
	//	CB_,		uUserType,
	//	CB_,		UserName.c_str(),
	//	CB_,		_tsid.get());
	MAKEQUERY_GetFamilyAllInfoForLogin_Lobby(strQuery, uID);
	DBCaller->executeWithParam(strQuery, DBCB_DBGetFamilyAllInfoForLogin_Lobby,
		"DDDBBsBBSW",
		OUT_,		NULL,
		OUT_,		NULL,
		CB_,		uID,
		CB_,		newuser,
		CB_,		bIsMobile,
		CB_,		sAddress.c_str(),
		CB_,		bGM,
		CB_,		uUserType,
		CB_,		UserName.c_str(),
		CB_,		_tsid.get());
}

static void	DBCB_DBFindFamilyWithID(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32			ret			= *(int *)argv[0];
	T_FAMILYID		FID			= *(T_FAMILYID *)argv[1];
	uint8			flag		= *(uint8 *)argv[2];
	ucstring		ucFidOrFname= (ucchar *)argv[3];
	uint16          page		= *(uint16 *)argv[4];
	uint16          CountPerPage= *(uint16 *)argv[5];
	TServiceId		tsid(*(uint16 *)argv[6]);

	if ( !nQueryResult )
	{
		sendErrorResult(M_SC_FINDFAMILY, FID, E_DBERR, tsid);
		return;
	}

	std::string sFid = ucFidOrFname.toString();
	T_FAMILYID      FidForFind = atoui(sFid.c_str());

	if(ret == 1010) // No Data
	{
		ret = ERR_NOERR;
		uint32	totalCount = 0;
		CMessage msgout(M_SC_FINDFAMILY);
		msgout.serial(FID);
		msgout.serial(ret);
		msgout.serial(flag, ucFidOrFname);
		msgout.serial(totalCount, page);
		CUnifiedNetwork::getInstance()->send(tsid, msgout);
		return;
	}

	uint32	totalCount = 1;
	CMessage msgout(M_SC_FINDFAMILY);
	msgout.serial(FID);
	msgout.serial(ret);
	msgout.serial(flag, ucFidOrFname);

	if (ret == ERR_NOERR)
	{
		uint32 nRows;	resSet->serial(nRows);
		if (nRows != 1)
		{
			sipwarning("[FID=%d] %s", FID, "Shrd_family_findWithID: fetch failed!");
			sendErrorResult(M_SC_FINDFAMILY, FID, E_DBERR, tsid);
			return;
		}

		T_FAMILYNAME		FamilyName;			resSet->serial(FamilyName);
		T_MODELID			ModelID;			resSet->serial(ModelID);
		T_CHBIRTHDATE		birthDate;			resSet->serial(birthDate);
		T_CHCITY			city;				resSet->serial(city);
		T_F_VISITDAYS		vDays;				resSet->serial(vDays);
		T_FLAG				bOnline;			resSet->serial(bOnline);
		T_F_LEVEL			fLevel;				resSet->serial(fLevel);
		uint32				CharacterID;		resSet->serial(CharacterID);

		if (bOnline)
		{
			FAMILY	*pInfo = GetFamilyData(FidForFind);
			if (pInfo)
			{
				bOnline |= (pInfo->m_bIsMobile << 4);
			}
		}

		msgout.serial(totalCount, page, FamilyName, FidForFind, CharacterID, ModelID, fLevel);
		msgout.serial(birthDate, city, bOnline);
		sipinfo(L"Find result: total=%d, fid=%u, %s, online=%d", totalCount, FidForFind, FamilyName.c_str(), bOnline);
	}		
	else
	{
		totalCount = 0;
		msgout.serial(totalCount, page);
	}
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

static void	DBCB_DBFindFamily(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	struct sendFamilyInfo
	{
		T_FAMILYNAME	FamilyName;
		T_FAMILYID		fid;
		uint32			CHID;
		T_MODELID		ModelID;
		T_CHBIRTHDATE	birthDate;
		T_CHCITY		city;
		T_F_LEVEL		fLevel;
		T_FLAG			bOnline;

		sendFamilyInfo(T_FAMILYNAME _FamilyName, T_FAMILYID _fid, uint32 _chid, T_MODELID _ModelID, T_CHBIRTHDATE _birthDate, T_CHCITY _city, T_F_LEVEL _fLevel, T_FLAG _bOnline)
			:FamilyName(_FamilyName), fid(_fid), CHID(_chid), ModelID(_ModelID), birthDate(_birthDate), city(_city), fLevel(_fLevel), bOnline(_bOnline)
			{}

	};
	typedef std::vector<sendFamilyInfo> FamilyInfoList;

	uint32			ret				= *(uint32 *)argv[0];
	uint32			RecordCount		= *(uint32 *)argv[1];
	uint32			PageCount		= *(uint32 *)argv[2];
	uint32			uPage			= *(uint32 *)argv[3];
	T_FAMILYID		FID				= *(T_FAMILYID *)argv[4];
	uint8			flag		= *(uint8 *)argv[5];
	ucstring		ucFidOrFname= (ucchar *)argv[6];
	TServiceId		tsid(*(uint16 *)argv[7]);

	uint16			page = static_cast<uint16>(uPage);
	FamilyInfoList  vSendFamilyInfo;

	if ( !nQueryResult )
	{
		sendErrorResult(M_SC_FINDFAMILY, FID, E_DBERR, tsid);
		return;
	}

	if(ret == 1010) // No Data
	{
		uint32	totalCount = 0;
		ret = ERR_NOERR;
		CMessage msgout(M_SC_FINDFAMILY);
		msgout.serial(FID);
		msgout.serial(ret);
		msgout.serial(flag, ucFidOrFname);
		msgout.serial(totalCount, page);
		CUnifiedNetwork::getInstance()->send(tsid, msgout);
		return;
	}

	CMessage msgout(M_SC_FINDFAMILY);
	msgout.serial(FID);
	msgout.serial(ret);
	msgout.serial(flag, ucFidOrFname);

	if (ret == ERR_NOERR)
	{
		uint32	nRows;				resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i++)
		{
			T_FAMILYID			fid;					resSet->serial(fid);
			T_FAMILYNAME		FamilyName;				resSet->serial(FamilyName);
			T_MODELID			ModelID;				resSet->serial(ModelID);
			T_CHBIRTHDATE		birthDate;				resSet->serial(birthDate);
			T_CHCITY			city;					resSet->serial(city);
			T_F_VISITDAYS		vDays;					resSet->serial(vDays);
			T_FLAG				bOnline;				resSet->serial(bOnline);
			T_F_LEVEL			fLevel;					resSet->serial(fLevel);
			uint32				CharacterID;			resSet->serial(CharacterID);

			if (bOnline )
			{
				FAMILY	*pInfo = GetFamilyData(fid);
				if (pInfo)
				{
					bOnline |= (pInfo->m_bIsMobile << 4);
				}
			}
			sendFamilyInfo FamilyInfo(FamilyName, fid, CharacterID, ModelID, birthDate, city, fLevel, bOnline);
			vSendFamilyInfo.push_back(FamilyInfo);
//			sipinfo(L"Find result: fid=%u, %s, online=%d", fid, FamilyName.c_str(), bOnline);
		}

		sipinfo("Find result: total=%d", RecordCount);
		msgout.serial(RecordCount);
		msgout.serial(page);
		for (unsigned int i = 0; i < vSendFamilyInfo.size(); i++)
		{
			msgout.serial( vSendFamilyInfo[i].FamilyName );
			msgout.serial( vSendFamilyInfo[i].fid );
			msgout.serial( vSendFamilyInfo[i].CHID );
			msgout.serial( vSendFamilyInfo[i].ModelID );
			msgout.serial( vSendFamilyInfo[i].fLevel );
			msgout.serial( vSendFamilyInfo[i].birthDate );
			msgout.serial( vSendFamilyInfo[i].city );
			msgout.serial( vSendFamilyInfo[i].bOnline );
		}
	}		
	else
	{
		uint32	totalCount = 0;
		msgout.serial(totalCount, page);
	}
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

// packet : M_CS_FINDFAMILY
// Find Family with Family Name or FamilyId
void	cb_M_CS_FINDFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID = 0;				_msgin.serial(FID);
	T_FLAG          flag = 0;               _msgin.serial(flag);
	uint16          page = 0;               _msgin.serial(page);
	uint16          CountPerPage = 0;       _msgin.serial(CountPerPage);
	ucstring        ucFidOrFname;           _msgin.serial(ucFidOrFname);

	sipinfo(L"FID=%u, flag=%u, page=%d, per=%d, key=%s", FID, flag, page, CountPerPage, ucFidOrFname.c_str());

	T_FAMILYID      FidForFind = 0;
	T_FAMILYNAME	uFamilyName = "";

	try
	{
		if( flag == 0 )
		{
			std::string sFid = ucFidOrFname.toString();
			FidForFind = atoui( sFid.c_str() );	
			MAKEQUERY_FINDFAMILYWITHID(strQ, FidForFind);
			DBCaller->executeWithParam2(strQ, 10000, DBCB_DBFindFamilyWithID, 
				"DDBSWWW",
				OUT_,		NULL,
				CB_,		FID,
				CB_,		flag,
				CB_,		ucFidOrFname.c_str(),
				CB_,		page,
				CB_,		CountPerPage,
				CB_,		_tsid.get());
		}
		else
		{
			if (CountPerPage <= 0)
			{
				return;
			}

			uFamilyName = ucFidOrFname;
			uint32  uPage = (uint32)page;
			uint32  uCountPerPage = (uint32)CountPerPage;
			MAKEQUERY_FINDFAMILY(strQ, uPage, uFamilyName, uCountPerPage);
			DBCaller->executeWithParam2(strQ, 10000, DBCB_DBFindFamily, 
				"DDDDDBSW",
				OUT_,			NULL,
				OUT_,			NULL,
				OUT_,			NULL,
				CB_,			uPage,
				CB_,			FID,
				CB_,		flag,
				CB_,		ucFidOrFname.c_str(),
				CB_,			_tsid.get());
		}
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", FID, str);
		sendErrorResult(M_SC_FINDFAMILY, FID, E_DBERR, _tsid);
	}
}	


static void	DBCB_DBNewFamily_GetFID(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet);
static void	DBCB_DBNewFamily(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
	{
		sipwarning("QueryResult failed.");
		return;
	}

	uint8				bIsMobile		= *(uint8*)argv[0];
	uint32				FID				= *(uint32*)argv[1];
	T_FAMILYNAME		ucFName			= (ucchar *)argv[2];
	T_ACCOUNTID			uAccountID		= *(T_ACCOUNTID *)argv[3];
	uint32				modelid			= *(uint32*)argv[4];
	uint32				dressid			= *(uint32*)argv[5];
	uint32				faceid			= *(uint32*)argv[6];
	uint32				hairid			= *(uint32*)argv[7];
	uint32				city			= *(uint32*)argv[8];
	ucstring			UserName		= (ucchar *)argv[9];
	string				sAddress		= (char *)argv[10];
	bool		        bGM				= *(bool *)argv[11];
	uint8		        uUserType		= *(uint8 *)argv[12];
	TServiceId			tsid(*(uint16 *)argv[13]);

	uint32	nSuccess = 0;
	uint32	nRows;			resSet->serial(nRows);
	if (nRows < 1)
	{
		sipwarning("RecordCount = 0.");
		return;
	}

	uint32	nResult;		resSet->serial(nResult);
	uint32	nVal;			resSet->serial(nVal);

	// 이미 가족창조가 되여 있다면
	if (nResult == false && nVal == 1015)
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = ucstring("Double creating family : ") + ucstring("AccountID = ") + toStringW(uAccountID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, uAccountID);
		return;
	}

	// 이미 같은 FID가 있다면
	if (nResult == false && nVal == 100)
	{
		MAKEQUERY_GetNewFamilyID(strQuery, 0);
		DBCaller->executeWithParam(strQuery, DBCB_DBNewFamily_GetFID,
			"DBDDDDDDSSsBBW",
			OUT_,			NULL,
			CB_,			bIsMobile,
			CB_,			modelid,
			CB_,			dressid,
			CB_,			faceid,
			CB_,			hairid,
			CB_,			city,
			CB_,			uAccountID,
			CB_,			UserName.c_str(),
			CB_,			ucFName.c_str(),
			CB_,			sAddress.c_str(),
			CB_,			bGM,
			CB_,			uUserType,
			CB_,			tsid.get());
		return;
	}

	nSuccess = 	nResult;
	uint32 nCode = nVal;
	if (nResult == false)
	{
		switch(nVal)
		{
		case 1015:		nCode = E_ALREADY_FAMILY;		break;
		case 1023:		nCode = E_FAMILY_DOUBLE;		break;
		case 100:		nCode = E_DBERR;				break;
		}
	}
	// 클라이언트에 결과를 통지한다
	CMessage	msgOut(M_SC_NEWFAMILY);
	msgOut.serial(uAccountID, nResult, nCode);
	CUnifiedNetwork::getInstance()->send(tsid, msgOut);

	sipinfo(L"Create new family : result = %d, value = %d, accountid = %d, name = %s ", nResult, nVal, uAccountID, ucFName.c_str());

	if (!nSuccess)
		return;

	//CLobbyWorld	*pLobby = getLobbyWorld();
	//if(pLobby == NULL)
	//{
	//	sipwarning("getLobbyWorld = NULL");
	//	return;
	//}

	//MAKEQUERY_GetFamilyOnUserID(strQuery, uAccountID);
	//DBCaller->execute(strQuery, DBCB_DBGetFamilyOnUserIDForNew,
	//	"DsBBSSW",
	//	CB_,			uAccountID,
	//	CB_,			sAddress.c_str(),
	//	CB_,			bGM,
	//	CB_,			uUserType,
	//	CB_,			UserName.c_str(),
	//	CB_,			ucFName.c_str(),
	//	CB_,			tsid.get());

	{
		uint8	newuser = 1;
		//MAKEQUERY_GetFamilyOnUserIDForLogin(strQuery, uAccountID);
		//DBCaller->executeWithParam(strQuery, DBCB_DBGetFamilyOnUserIDForLogin,
		//	"DDDBBsBBSW",
		//	OUT_,		NULL,
		//	OUT_,		NULL,
		//	CB_,		uAccountID,
		//	CB_,		newuser,
		//	CB_,		bIsMobile,
		//	CB_,		sAddress.c_str(),
		//	CB_,		bGM,
		//	CB_,		uUserType,
		//	CB_,		UserName.c_str(),
		//	CB_,		tsid.get());
		MAKEQUERY_GetFamilyAllInfoForLogin_Lobby(strQuery, uAccountID);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetFamilyAllInfoForLogin_Lobby,
			"DDDBBsBBSW",
			OUT_,		NULL,
			OUT_,		NULL,
			CB_,		uAccountID,
			CB_,		newuser,
			CB_,		bIsMobile,
			CB_,		sAddress.c_str(),
			CB_,		bGM,
			CB_,		uUserType,
			CB_,		UserName.c_str(),
			CB_,		tsid.get());
	}

	{
		CMessage	msgOut1(M_NT_NEWFAMILY);
		msgOut1.serial(uAccountID, FID, ucFName);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut1);
	}
}

static void	DBCB_DBNewFamily_GetFID(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
	{
		sipwarning("QueryResult failed.");
		return;
	}

	uint32				FID				= *(uint32*)argv[0];
	uint8		        bIsMobile		= *(uint8 *)argv[1];
	uint32				modelid			= *(uint32*)argv[2];
	uint32				dressid			= *(uint32*)argv[3];
	uint32				faceid			= *(uint32*)argv[4];
	uint32				hairid			= *(uint32*)argv[5];
	uint32				city			= *(uint32*)argv[6];
	T_ACCOUNTID			uAccountID		= *(T_ACCOUNTID *)argv[7];
	ucstring			UserName		= (ucchar *)argv[8];
	T_FAMILYNAME		ucFName			= (ucchar *)argv[9];
	string				sAddress		= (char *)argv[10];
	bool		        bGM				= *(bool *)argv[11];
	uint8		        uUserType		= *(uint8 *)argv[12];
	TServiceId			_tsid(*(uint16 *)argv[13]);

	if(FID == 0)
	{
		// SP실행이 실패한 경우이다. 다시 처음부터 시작한다. - 무한순환의 여지가 있다!!! 주의!!!
		MAKEQUERY_GetNewFamilyID(strQuery, 0);
		DBCaller->executeWithParam(strQuery, DBCB_DBNewFamily_GetFID,
			"DBDDDDDDSSsBBW",
			OUT_,			NULL,
			CB_,			bIsMobile,
			CB_,			modelid,
			CB_,			dressid,
			CB_,			faceid,
			CB_,			hairid,
			CB_,			city,
			CB_,			uAccountID,
			CB_,			UserName.c_str(),
			CB_,			ucFName.c_str(),
			CB_,			sAddress.c_str(),
			CB_,			bGM,
			CB_,			uUserType,
			CB_,			_tsid.get());
		return;
	}

	// FID를 검사한다.
	uint32	CharShardCode = (FID & 0xFF000000);		// 지역코드 (0-A, 1-B, ...)

	int		i;
	int		n[MAX_LEN_CHARACTER_ID];
	n[0] = FID & 0x00FFFFFF;
	for(i = 1; i < MAX_LEN_CHARACTER_ID; i ++)
	{
		n[i] = n[i - 1] / 36;
		n[i - 1] = n[i - 1] % 36;
	}

	string	strClientFID, strSubFID;
	for(i = 0; i < MAX_LEN_CHARACTER_ID; i ++)
	{
		int c;
		if(n[i] < 10) c = n[i] + '0'; else c = n[i] + ('a' - 10);
		strSubFID = toString("%c", c);
		strClientFID = strSubFID + strClientFID;
	}

	for(i = 1; i <= MAX_LEN_CHARACTER_ID; i ++)
	{
		for(int j = 0; j <= MAX_LEN_CHARACTER_ID - i; j ++)
		{
			strSubFID = "-" + strClientFID.substr(j, i) + "-";
			if(strstr(mstBadFIDList.c_str(), strSubFID.c_str()) != NULL)
			{
				// 다음 FID를 추정한다.
				uint32	NewFID = 0;
				if(i + j < MAX_LEN_CHARACTER_ID)
				{
					int z = MAX_LEN_CHARACTER_ID - (i + j);
					for(int k = MAX_LEN_CHARACTER_ID - 1; k >= 0; k --)
					{
						NewFID *= 36;
						if(k > z)
							NewFID += n[k];
						else if(k == z)
							NewFID += (n[k] + 1);
					}
					NewFID --;
				}

				if (NewFID != 0)
					NewFID += CharShardCode;

				// 무한순환의 여지가 있다!!! 주의!!!
				MAKEQUERY_GetNewFamilyID(strQuery, NewFID);
				DBCaller->executeWithParam(strQuery, DBCB_DBNewFamily_GetFID,
					"DBDDDDDDSSsBBW",
					OUT_,			NULL,
					CB_,			bIsMobile,
					CB_,			modelid,
					CB_,			dressid,
					CB_,			faceid,
					CB_,			hairid,
					CB_,			city,
					CB_,			uAccountID,
					CB_,			UserName.c_str(),
					CB_,			ucFName.c_str(),
					CB_,			sAddress.c_str(),
					CB_,			bGM,
					CB_,			uUserType,
					CB_,			_tsid.get());

				return;
			}
		}
	}

	// FID OK
	uint32		sexid = GetModelSex(modelid);
	MAKEQUERY_InsertFamily(strQuery, FID, ucFName, uAccountID, modelid, sexid, dressid, faceid, hairid, city);
	DBCaller->execute(strQuery, DBCB_DBNewFamily,
		"BDSDDDDDDSsBBW",
		CB_,			bIsMobile,
		CB_,			FID,
		CB_,			ucFName.c_str(),
		CB_,			uAccountID,
		CB_,			modelid,
		CB_,			dressid,
		CB_,			faceid,
		CB_,			hairid,
		CB_,			city,
		CB_,			UserName.c_str(),
		CB_,			sAddress.c_str(),
		CB_,			bGM,
		CB_,			uUserType,
		CB_,			_tsid.get());
}

// 새가족을 창조한다
void	cb_M_CS_NEWFAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ACCOUNTID			uAccountID;		_msgin.serial(uAccountID);
	ucstring			UserName;		_msgin.serial(UserName);
	if (UserName == ucstring("") || UserName.size() > MAX_LEN_USER_ID)
	{
		sipwarning("UserName Error: UserName=%s", UserName.c_str());
		return;
	}

	T_FAMILYNAME		ucFName;		_msgin.serial(ucFName);
	uint8				bIsMobile;		_msgin.serial(bIsMobile);
	if (ucFName == ucstring("") || ucFName.size() > MAX_LEN_FAMILY_NAME)
	{
		sipwarning("FamilyName Error: FamilyName=%s", ucFName.c_str());
		return;
	}

	T_MODELID			modelid;
	T_DRESS				dressid;
	T_FACEID			faceid;
	uint32				city;
	T_HAIRID			hairid;			_msgin.serial(modelid, dressid, faceid, hairid, city);
	string				sAddress;		_msgin.serial(sAddress);
	bool		        bGM;            _msgin.serial(bGM);
	uint8		        uUserType;      _msgin.serial(uUserType);

	uint32		sexid = GetModelSex(modelid);
	if (sexid == 0)
	{
		sipwarning("Invalid ModelID! (SexID=0) ModelID=%d", modelid);
		return;
	}

	MAKEQUERY_GetNewFamilyID(strQuery, 0);
	DBCaller->executeWithParam(strQuery, DBCB_DBNewFamily_GetFID,
		"DBDDDDDDSSsBBW",
		OUT_,			NULL,
		CB_,			bIsMobile,
		CB_,			modelid,
		CB_,			dressid,
		CB_,			faceid,
		CB_,			hairid,
		CB_,			city,
		CB_,			uAccountID,
		CB_,			UserName.c_str(),
		CB_,			ucFName.c_str(),
		CB_,			sAddress.c_str(),
		CB_,			bGM,
		CB_,			uUserType,
		CB_,			_tsid.get());
}

// 캐랙터가 없으면 강제로 생성한다
/*T_CHARACTERID	ForceCharacter(T_ACCOUNTID uFamilyID, T_CHARACTERNAME ucName, TServiceId _tsid)
{
	T_CHARACTERID	chid=0;
	if (ucName.size() < 1)
	{
		sipwarning(L"Character name is empty!(ID=%d)", uFamilyID);
		return -1;
	}

	// 해당 캐랙터가 있는가를 검사한다
	CMessage	msgOut(M_SC_ALLCHARACTER);
	msgOut.serial( uFamilyID );				// 응답을 받을 대상
    
	uint32	nCharacter = 0;
	MAKEQUERY_GETCHARACTERS(strQ, uFamilyID);
	CDBConnectionRest	DB(DBCaller);
	DB_EXE_QUERY(DB, Stmt, strQ);
	if (nQueryResult == TRUE)
	{
		do 
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (IS_DB_ROW_VALID(row))
			{
				COLUMN_DIGIT(row, 0, T_CHARACTERID, uid);			chid = uid;
				COLUMN_WSTR(row, 1, name, MAX_LEN_CHARACTER_NAME);
				COLUMN_DIGIT(row, 2, T_MODELID, modelid);
				COLUMN_DIGIT(row, 3, T_DRESS, ddressid);
				COLUMN_DIGIT(row, 4, T_FACEID, faceid);
				COLUMN_DIGIT(row, 5, T_HAIRID, hairid);
				COLUMN_DATETIME(row, 6, createdate);
				COLUMN_DIGIT(row, 7, T_ISMASTER, master);
				COLUMN_DIGIT(row, 8, T_DRESS, dressid);
				COLUMN_DATETIME(row, 9, changedate);
				COLUMN_DIGIT(row, 10, T_CHBIRTHDATE, birthDate);
				COLUMN_DIGIT(row, 11, T_CHCITY, city);
				COLUMN_WSTR(row, 12, resume, MAX_RESUME_NUM);
				msgOut.serial(uid, name, modelid, ddressid, faceid, hairid, createdate, master);
				msgOut.serial(dressid, changedate);
				msgOut.serial(birthDate);
				msgOut.serial(city);
				msgOut.serial(resume);

				nCharacter ++;
			}
			else
				break;
		} while(TRUE);
	}
	sipinfo(L"Send character (id:%d) info to account = %d", chid, uFamilyID);
	CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
	if ( nCharacter > 0 )
		return chid;

	return	-1;
}*/

// Packet : M_CS_FORCEFAMILY
/*void	cb_M_CS_FORCEFAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
sipwarning("Check Packet!!! added ChannelID");
return;

	uint32	uResult;

	uint32		userid;			_msgin.serial(userid);
	// Useful only for test client
	_msgin.serial(userid);
	ucstring	ucFamilyName;	_msgin.serial(ucFamilyName);
	ucstring	ucUserName;		_msgin.serial(ucUserName);
	uint8		sex;			_msgin.serial(sex);
	ucstring	ucHistory;		_msgin.serial(ucHistory);
	uint32		roomid;			_msgin.serial(roomid);
	uint32		channelid;		_msgin.serial(channelid);
	ucstring	ucChName;		_msgin.serial(ucChName);

// Check exist of family
	bool	bIsFamily = false;	// Flag whether there is a family for a user
	T_FAMILYID		_FamilyID = 0;
	T_PRICE			_GMoney = 10000;
	T_PRICE			_Money = 10000;
	T_F_EXP			_Exp = 0;
	T_F_VIRTUE		_Virtue = 0;
	T_F_FAME		_Fame = 0;
	T_F_VISITDAYS	_VisitDays = 0;
	uint8			_ExpRoom = 0;

// Force Login Family
	{
		MAKEQUERY_FAMILYLOGIN(strQuery, userid); // , ucUserName.c_str(), _Money, _VisitDays, _Exp, _Virtue, _Fame, _ExpRoom);
		CDBConnectionRest	DB(DBCaller);
   		DB_EXE_QUERY_WITHONEROW(DB, Stmt, strQuery);
		if ( !nQueryResult )
		{
			sipwarning("Query Failed! : %S.", strQuery);
			return;
		}
		COLUMN_DIGIT(row, 0, uint32, nResult);

		T_ERR	err;
		switch ( nResult )
		{
			case 0:
				err = ERR_NOERR;
				break;
			case 10:
				err = E_DBERR;
				break;
			default:
				break;
		}

		if ( err != ERR_NOERR )
			return;
	}

// Send Family Information
	{
		MAKEQUERY_GETFAMILYONUSERID(strQuery, userid);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult == TRUE)
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (IS_DB_ROW_VALID(row))
			{
				COLUMN_DIGIT(row, 0, T_FAMILYID, fid);
				COLUMN_WSTR(row, 1, FamilyName, MAX_LEN_FAMILY_NAME);
				COLUMN_DIGIT(row, 2, T_PRICE, GMoney);
				//COLUMN_DIGIT(row, 3, T_PRICE, Money);
				COLUMN_DIGIT(row, 3, T_F_EXP, Exp);
				COLUMN_DIGIT(row, 4, T_F_VIRTUE, Virtue);
				COLUMN_DIGIT(row, 5, T_F_FAME, Fame);
				COLUMN_DIGIT(row, 6, T_F_VISITDAYS, vdays);

T_PRICE	Money = 0;

				_FamilyID = fid;
				ucFamilyName = FamilyName;
				_GMoney = GMoney;
				_Money = Money;
				_Exp = Exp;
				_Virtue = Virtue;
				_Fame = Fame;
				_VisitDays = vdays;

				bIsFamily = true;
			}
		}
	}
	if (bIsFamily)	// If Exists a Family
	{
		sipinfo(L"Send family %u, info to account = %d", _FamilyID, userid);
		SendFamilyAllInfo(_FamilyID, _tsid);
	}
	else
	{
		T_FAMILYID temp = NOFAMILYID;
		sipinfo(L"Send family no info to account = %d", userid);
	}

	// if no exist, send false
	if ( !bIsFamily )
	{
		uResult = E_DBERR;
		goto	SEND;
	}

	// get his create room id
	if ( roomid == 0 )
	{
		MAKEQUERY_GETOWNCREATEROOM(strQuery, _FamilyID);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult != TRUE)
		{
			uResult = E_DBERR;
            goto	SEND;
		}

		// only one room information
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			uResult = E_DBERR;
			goto	SEND;
		}

		COLUMN_DIGIT(row,	0,	uint16,	idRoomType);
		COLUMN_DATETIME(row,	1, 	creationtime);
		COLUMN_DATETIME(row,	2, 	termtime);
		COLUMN_DIGIT(row,	3,	uint32,	idRoom);
		COLUMN_WSTR(row,	4,	roomName,	MAX_LEN_ROOM_NAME);
		COLUMN_DIGIT(row,	5,	uint8,	idOpenability);
		COLUMN_DIGIT(row,	6,	uint8,	freeUse);

		roomid = idRoom;
	}

	if ( bIsFamily )
	{
		sipinfo("There is already family id:%u, So No create again!", _FamilyID);

		CMessage	msgOut2(M_SC_FAMILY);
		msgOut2.serial(userid, _FamilyID, ucFamilyName, ucUserName, sex, ucHistory);
		msgOut2.serial(_GMoney, _Money);
		msgOut2.serial(_Exp, _Virtue, _Fame);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut2);
		// force enter his room
		// Set FES that his family connected
		AddFamilyData(_FamilyID, userid, ucFamilyName, "", 0, 0, CTime::getLocalTime());
		SetFesForFamilyID(_FamilyID, _tsid);
		INFO_FAMILYINROOM*	pFamily = GetFamilyInfo(_FamilyID);
		if ( pFamily )
			pFamily->m_nGMoney = _GMoney;

		T_CHARACTERID	chid = ForceCharacter(_FamilyID, ucChName, _tsid);
		if ( chid == -1 )
		{
			uResult = E_DBERR;
			goto	SEND;
		}
		ForceEnterRoom(channelid, _FamilyID, _tsid, roomid, chid);
		sipdebug("Family(id:%u, characterId:%d) force entered room(id:%u)", _FamilyID, chid, roomid);

		sipinfo("SUCCESS FORCEFAMILY id:%s", ucFamilyName.c_str());
		return;
	}

SEND:
	CMessage	msgoutError(M_SC_FORCEFAMILY);
	msgoutError.serial(userid);
	msgoutError.serial(uResult);
	CUnifiedNetwork::getInstance()->send(_tsid, msgoutError);

	sipwarning("FAIL FORCEFAMILY id:%s", ucFamilyName.c_str());
}*/

// 가족을 삭제한다
//void	cb_M_CS_DELFAMILY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		uFID;					// account id
//	_msgin.serial(uFID);
//
//	// 체험방관리자는 가족을 삭제할수 없다
//	/*
//	FAMILY	*pFamily = GetFamilyData(uFID);
//	if (pFamily != NULL)
//	{
//		if (pFamily->m_UserType == USERTYPE_EXPROOMMANAGER)
//			return;
//	}
//	*/
//	// 체계가족은 삭제할수 없다
//	if (uFID <= 1000)
//		return;
//
//	uint32 nResult = false, nVal = E_COMMON_FAILED;
//	{
//		MAKEQUERY_DELFAMILY(strQ, uFID);
//		DB_EXE_QUERY(DB, Stmt, strQ);
//		if (nQueryResult == TRUE)
//		{
//			DB_QUERY_RESULT_FETCH(Stmt, row);
//			if (IS_DB_ROW_VALID(row))
//			{
//				COLUMN_DIGIT(row, 0, uint32, nR);
//				if (nR == 0)
//				{
//					nResult = TRUE;
//					nVal = uFID;
//				}
//			}
//		}
//	}
//
//	// 클라이언트에게 통보한다
//	CMessage	msgOut(M_SC_DELFAMILY);
//	msgOut.serial(uFID, nResult, nVal, uFID);
//	CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
//	sipinfo(L"Delete family : Familyid = %d, Result = %d", uFID, nResult);
//
//	if (nResult == TRUE)
//	{
//		DBLOG_DelFamily(uFID);
//	}
//}

// 가족명 중복검사
//void	cb_M_CS_FNAMECHECK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_ACCOUNTID		uAccountID;					_msgin.serial(uAccountID);		// account id
//	T_FAMILYNAME	FName;						_msgin.serial(FName);
//	if ( FName == ucstring("") || FName.size() > MAX_LEN_FAMILY_NAME)
//		return;
//
//	uint8	uResult = FAMILYNAMECHECK_OK;
//	MAKEQUERY_FAMILYNAMECHECK(strQuery, FName.c_str());
//	DB_EXE_QUERY(DB, Stmt, strQuery);
//	if (nQueryResult == TRUE)
//	{
//		DB_QUERY_RESULT_FETCH(Stmt, row);
//		if (IS_DB_ROW_VALID(row))
//			uResult = FAMILYNAMECHECK_ISTHERE;
//	}
//	
//	CMessage	msgOut(M_SC_FNAMECHECK);
//	msgOut.serial(uAccountID, uResult, FName);
//	CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
//}

//void	cb_M_CS_REQ_CHANGECH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID	FID;	_msgin.serial(FID);
//
//	T_PRICE		GPrice = 0;
//	T_PRICE		UPrice = g_nCharChangePrice;
//
//	uint32	error = ERR_NOERR;
//	if ( ! CheckMoneySufficient(FID, UPrice, 0, MONEYTYPE_UMONEY) )
//	{
//		error = E_NOMONEY;
//	}
//
//	CMessage mesOut(M_SC_REQ_CHANGECH);
//	mesOut.serial(FID, error);
//	CUnifiedNetwork::getInstance()->send(_tsid, mesOut);
//}

static void	DBCB_DBBuyHisSpace(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if ( !nQueryResult )
		return;

	T_FAMILYID		FID			= *(T_FAMILYID *)argv[0];
	T_COMMON_ID		uHisSpaceID	= *(T_COMMON_ID *)argv[1];
	T_ROOMID		room_id		= *(T_ROOMID *)argv[2];
	T_PRICE			UPrice		= *(T_PRICE *)argv[3];
	T_PRICE			GPrice		= *(T_PRICE *)argv[4];
	TServiceId		tsid(*(uint16 *)argv[5]);
	const MST_HISSPACE	*pHisSpace = GetMstHisSpace(uHisSpaceID);
	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if ( pFamilyInfo == NULL )
		return;

	uint32	nRows;					resSet->serial(nRows);
	if (nRows < 1)
		return;

	uint32	ret;					resSet->serial(ret);
	if (ret == 0)
	{
		uint32	 HisSpace_New;		resSet->serial(HisSpace_New);

		// Notify LS to subtract the user's money by sending message
		ExpendMoney(FID, UPrice, GPrice, 1, DBLOG_MAINTYPE_BUYSPACE, 0, 0, uHisSpaceID,
			pHisSpace->m_HisSpace, UPrice, room_id);

		// Send NewHisSpace to client
		CMessage msgHis(M_SC_CURHISSPACE);
		msgHis.serial( FID, room_id, uHisSpaceID, HisSpace_New );
		CUnifiedNetwork::getInstance()->send(tsid, msgHis);

		// Modify Family & User Exp
		// ChangeFamilyExp(Family_Exp_Type_UseMoney, FID, UPrice, pHisSpace->m_AddPoint);
		if (pHisSpace->m_AddPoint > 0)
			ExpendMoney(FID, 0, pHisSpace->m_AddPoint, 2, DBLOG_MAINTYPE_CHFGMoney, 0, 0, 0);
		//ChangeRoomExp(room_id, Room_Exp_Type_UseMoney, FID, UPrice);

		// Send GMoney and UMoney to client
		SendFamilyProperty_Money(tsid, FID, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);

		sipinfo(L"Buy history space : FID=%d, HisSpaceId=%d", FID, uHisSpaceID);

		// Log
		DBLOG_BuySpace(FID, uHisSpaceID, pHisSpace->m_HisSpace, UPrice, GPrice, room_id);
	}
	else
		sipwarning(L"Failed buy history space in db: FID=%d, HisSpaceId=%d, Code=%d", FID, uHisSpaceID, ret);
}

// 사적공간선택
void	cb_M_CS_BUYHISSPACE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_COMMON_ID		uHisSpaceID;
	T_ROOMID		room_id;

	_msgin.serial(FID, room_id, uHisSpaceID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d, RoomID=%d", FID, room_id);
		return;
	}

	const MST_HISSPACE	*pHisSpace = GetMstHisSpace(uHisSpaceID);
	if (pHisSpace == NULL)
	{
		sipwarning("Invalid HisSpaceID !");
		return;
	}

	T_PRICE		GPrice = 0; // (pHisSpace->m_GMoney * pHisSpace->m_DiscGMoney) / 100; // 사적자료용량은 point로 구입할수 없다.
	T_PRICE		UPrice = (pHisSpace->m_UMoney * pHisSpace->m_DiscUMoney) / 100;

	// get family info
	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if ( pFamilyInfo == NULL )
	{
		sipwarning("GetFamilyInfo = NULL !");
		return;
	}

	if ( ! CheckMoneySufficient(FID, UPrice, GPrice, MONEYTYPE_UMONEY) )
		return;
	MAKEQUERY_BuyHisSpace(strQ, pHisSpace->m_HisSpace, room_id);
	DBCaller->execute(strQ, DBCB_DBBuyHisSpace,
		"DDDDDW",
		CB_,		FID,
		CB_,		uHisSpaceID,
		CB_,		room_id,
		CB_,		UPrice,
		CB_,		GPrice,
		CB_,		_tsid.get());
}

//static void	DBCB_DBGetChit(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID				FID			= *(T_FAMILYID *)argv[0];
//	T_COMMON_ID				uChitID		= *(T_COMMON_ID *)argv[1];
//	uint8					uRes		= *(uint8 *)argv[2];
//	TServiceId				tsid(*(uint16 *)argv[3]);
//
//	bool		bValid = false;
//	T_FAMILYID	SenderID;
//	T_FAMILYID	RecvID;
//	uint8		ChitType;
//	uint32		P1;
//	uint32		P2;
//	ucstring	P3, P4;
//	{
//		bool	bDelete = true;
//		if ( nQueryResult == TRUE )
//		{
//			uint32	nRows;			resSet->serial(nRows);
//			if (nRows > 0)
//			{
//				T_FAMILYID	senderid;		resSet->serial(senderid);
//				T_FAMILYID	recvid;			resSet->serial(recvid);
//				uint8		chittype;		resSet->serial(chittype);
//				uint32		p1;				resSet->serial(p1);
//				uint32		p2;				resSet->serial(p2);
//				ucstring	p3;				resSet->serial(p3);
//				ucstring	p4;				resSet->serial(p4);
//
//				bValid = true;				
//				SenderID = senderid;		RecvID = recvid;		ChitType = chittype;
//				P1 = p1;	P2 = p2;	P3 = p3;  P4 = p4;
//			}
//		}
//	}
//
//	if (!bValid)
//	{
//		sipwarning(L"Invalid response of chit FID=%d, ChitID=%d", FID, uChitID );
//		return;
//	}
//	if (FID != RecvID)
//	{
//		sipwarning("FID != RecvID!! FID=%d, RecvID=%d", FID, RecvID);
//		return;
//	}
//}

//// 쪽지에 대한 응답
//void cb_M_CS_RESCHIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	uint16			r_sid;		_msgin.serial(r_sid);
//	T_COMMON_ID		uChitID;	_msgin.serial(uChitID);
//	uint8			uRes;		_msgin.serial(uRes);
//
//	bool	bDelete = true;
//	MAKEQUERY_GETCHIT(strQ, uChitID, FID, bDelete);
//	DBCaller->execute(strQ, DBCB_DBGetChit, 
//		"DDBW",
//		CB_,		FID,
//		CB_,		uChitID,
//		CB_,		uRes,
//		CB_,		_tsid.get());
//}

void	cb_M_CS_REQ_MST_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);

	SendAllMasterDatas(FID, _tsid);

	SendCheckMailboxResult(FID, _tsid);
}


void	cb_M_GMCS_GETMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	sipwarning("this function not ready.");
	return;
	//T_FAMILYID		GMID;		_msgin.serial(GMID);					// 가족ID
	//T_FAMILYID		FID;		_msgin.serial(FID);

	//{
	//	MAKEQUERY_FAMILYINFOFROMNAME(strQuery, FID);
	//	DB_EXE_QUERY(DB, Stmt, strQuery);
	//	if (nQueryResult == TRUE)
	//	{
	//		DB_QUERY_RESULT_FETCH(Stmt, row);
	//		if (IS_DB_ROW_VALID(row))
	//		{
	//			CMessage mes(M_GMSC_GETMONEY);
	//			mes.serial(GMID, FID);

	//			bool bIs;

	//			COLUMN_DIGIT(row, 0, uint32,	uResult);
	//			if (uResult == 1001)
	//			{
	//				bIs = false;
	//				mes.serial(bIs);
	//			}
	//			else
	//			{
	//				COLUMN_DIGIT(row, 1, T_PRICE,	gmoney);
	//				COLUMN_DIGIT(row, 2, T_PRICE,	umoney);
	//				COLUMN_WSTR(row, 4,	FName,	MAX_LEN_FAMILY_NAME);
	//				bIs = true;
	//				mes.serial(bIs, FName, umoney, gmoney);
	//			}
	//			CUnifiedNetwork::getInstance ()->send(_tsid, mes);
	//		}
	//	}
	//}
}

static void	DBCB_DBFamilyInfoForGMGetLevel(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID				GMID		= *(T_FAMILYID *)argv[0];
	T_FAMILYID				FID			= *(T_FAMILYID *)argv[1];
	TServiceId				tsid(*(uint16 *)argv[2]);
	
	if (!nQueryResult)
		return;

	uint32		nRows;		resSet->serial(nRows);
	if (nRows < 1)
		return;

	CMessage mes(M_GMSC_GETLEVEL);
	mes.serial(GMID, FID);
	bool bIs = true;
	uint32	uResult;		resSet->serial(uResult);
	if (uResult == 1001)
	{
		bIs = false;
		mes.serial(bIs);
	}
	else
	{
		T_PRICE			GMoney;			resSet->serial(GMoney);
		T_F_VISITDAYS	vdays;			resSet->serial(vdays);
		T_FAMILYNAME	FName;			resSet->serial(FName);
		T_F_LEVEL		uLevel;			resSet->serial(uLevel);
		bIs = true;
		mes.serial(bIs, FName, uLevel);
	}
	CUnifiedNetwork::getInstance()->send(tsid, mes);
}

void	cb_M_GMCS_GETLEVEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	sipwarning("This is not supported!!!");
	return;
	//T_FAMILYID		GMID;		_msgin.serial(GMID);					// 가족ID
	//T_FAMILYID		FID;		_msgin.serial(FID);

	//MAKEQUERY_FAMILYINFOFROMNAME(strQuery, FID);
	//DBCaller->execute(strQuery, DBCB_DBFamilyInfoForGMGetLevel,
	//	"DDW",
	//	CB_,		GMID,
	//	CB_,		FID,
	//	CB_,		_tsid.get());
}

void	cb_M_GMCS_SETLEVEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	sipwarning("This is not supported!!!");
	return;

	//T_FAMILYID		GMID;		_msgin.serial(GMID);					// 가족ID
	//T_FAMILYID		FID;		_msgin.serial(FID);
	//T_F_LEVEL		FLevel;		_msgin.serial(FLevel);

	//T_F_VISITDAYS vdays = MakeVDaysFromDegree(FLevel);

	//T_FAMILYID		ObjectID = NOFAMILYID;
	//CMessage mes(M_GMSC_GETLEVEL);
	//mes.serial(GMID, FID);
	//{
	//	MAKEQUERY_VDAYSFROMNAME(strQuery, FID, vdays);
	//	DB_EXE_QUERY(DB, Stmt, strQuery);
	//	if (nQueryResult == TRUE)
	//	{
	//		DB_QUERY_RESULT_FETCH(Stmt, row);
	//		if (IS_DB_ROW_VALID(row))
	//		{
	//			bool bIs = true;
	//			COLUMN_DIGIT(row, 0, uint32,	uResult);
	//			if (uResult == 1001)
	//			{
	//				bIs = false;
	//				mes.serial(bIs);
	//			}
	//			else
	//			{
	//				COLUMN_DIGIT(row, 1, T_FAMILYID,	Oid);
	//				COLUMN_DIGIT(row, 2, uint32,		Uid);
	//				COLUMN_WSTR(row, 3,	UName,	MAX_LEN_FAMILY_NAME);
	//				COLUMN_WSTR(row, 4,	FName,	MAX_LEN_FAMILY_NAME);
	//				ObjectID = Oid;
	//				bIs = true;
	//				mes.serial(bIs, FName, FLevel);

	//				// GM Log
	//				GMLOG_SetLevel( GMID, Oid, Uid, FName, UName, FLevel);
	//			}
	//			CUnifiedNetwork::getInstance ()->send(_tsid, mes);
	//		}
	//	}
	//}
	//
	//// 방에 들어온 상태이면 통보
	//CRoomWorld *inManager = GetRoomWorldFromFID(ObjectID);
	//if (inManager != NULL)
	//	inManager->SetDegreeFromVDays(ObjectID, vdays);
}

//static void	DBCB_DBSetExpRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID				GMID		= *(T_FAMILYID *)argv[0];
//	T_ROOMID				RoomID		= *(T_ROOMID *)argv[1];
//	bool					bSet		= *(bool *)argv[2];
//	TServiceId				tsid(*(uint16 *)argv[3]);
//
//	uint32	uResult = E_COMMON_FAILED;
//	if (nQueryResult == TRUE)
//	{
//		uint32		nRows;		resSet->serial(nRows);
//		if (nRows > 0)
//		{
//			uint32	uR;			resSet->serial(uR);
//			MAKEQUERY_SETEXPROOM(strQ, RoomID, GMID, bSet); // For log
//			switch ( uR )
//			{
//			case 0:		uResult = ERR_NOERR;				RESULT_SHOW(strQ, ERR_NOERR);		break;
//			case 1:		uResult = E_DBERR;					ERR_SHOW(strQ, E_DBERR);			break;
//			case 1045:	uResult = E_EXPROOM_ALREADY;		ERR_SHOW(strQ, E_EXPROOM_ALREADY);	break;
//			case 1046:	uResult = E_NOT_MANAGER;			ERR_SHOW(strQ, E_NOT_MANAGER);		break;
//			case 1047:	uResult = E_IMPROPER_ROOMKINDID;	ERR_SHOW(strQ, E_IMPROPER_ROOMKINDID);	break;
//			case 1048:	uResult = E_EXPROOM_NOCREATE;		ERR_SHOW(strQ, E_EXPROOM_NOCREATE);	break;
//			}
//
//		}
//	}
//
//	CMessage mes(M_GMSC_SETEXPROOM);
//	mes.serial(GMID, uResult, RoomID, bSet);
//	CUnifiedNetwork::getInstance ()->send(tsid, mes);
//
//	// 체험방을 갱신한 다음에 써버는 다음 갱신주기(재기동시)에서 이 정보를 갱신한다.
//	// LoadJingPinTianYuan();
//}
//void	cb_M_GMCS_SETEXPROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		GMID;		_msgin.serial(GMID);					// 가족ID
//	T_ROOMID		RoomID;		_msgin.serial(RoomID);
//	bool			bSet;		_msgin.serial(bSet);
//
//	MAKEQUERY_SETEXPROOM(strQ, RoomID, GMID, bSet);
//	DBCaller->execute(strQ, DBCB_DBSetExpRoom,
//		"DDBW",
//		CB_,	GMID,
//		CB_,	RoomID,
//		CB_,	bSet,
//		CB_,	_tsid.get());
//}

//void	cb_M_GMCS_EXPROOMDATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		GMID;		_msgin.serial(GMID);					// 가족ID
//	T_ROOMID		RoomID;		_msgin.serial(RoomID);
//
//	MAKEQUERY_EXPROOMDATA(strQ, RoomID, GMID, 2);
//	DBCaller->execute(strQ, NULL);
//
//	bool	result = true;
//	CMessage	msgout(M_GMSC_EXPROOMDATA);
//	msgout.serial(GMID, result, RoomID);
//	CUnifiedNetwork::getInstance ()->send(_tsid, msgout);
//}

// callback function for the "CANUSEMONEY" from WS, get the result of checking money enough
//void    cb_CANUSEMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	uint32	msgid;		_msgin.serial(msgid);
//	uint32	uResult;	_msgin.serial(uResult);
//	uint32	userid;		_msgin.serial(userid);
//	uint8	flag;		_msgin.serial(flag);
//	T_PRICE	moneyExpend;	_msgin.serial(moneyExpend);
//	T_PRICE	money;			_msgin.serial(money);
//
//	map<uint32, USEMONEYRESULT>::iterator it = map_USEMONEYRESULT.find(msgid);
//	if (it == map_USEMONEYRESULT.end())
//	{
//		sipwarning("Receive CanUseMoney of invalid : MesID=%d, UserID=%d, MoneyExpend=%d, Money=%d, Result=%d", msgid, userid, moneyExpend, money, uResult);
//		return;
//	}
//	it->second.bResult = true;
//	it->second.uResult = uResult;
//	it->second.uMoney = money;
//
//}

//static void	DBCB_DBAddChit(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID _SID		= *(T_FAMILYID *)argv[0];
//	T_FAMILYID _RID		= *(T_FAMILYID *)argv[1];
//	uint8 _ChitType		= *(uint8 *)argv[2];
//	uint32 _P1			= *(uint32 *)argv[3];
//	uint32 _P2			= *(uint32 *)argv[4];
//	ucstring _P3		= (ucchar *)argv[5];
//	ucstring _P4		= (ucchar *)argv[6];
//
//	if (!nQueryResult)
//		return;
//
//	uint32		nRows;			resSet->serial(nRows);
//	if (nRows < 1)
//		return;
//
//	uint32 ret;				resSet->serial(ret);
//	if (ret == 0)
//	{
//		uint32	chitid;			resSet->serial(chitid);
//		FAMILY	*RecvFamily	= GetFamilyData(_RID);
//		FAMILY	*SendFamily	= GetFamilyData(_SID);
//		if (RecvFamily != NULL && SendFamily != NULL)
//		{
//			uint32	uChitnum = 1;
//			uint16		r_sid = SIPNET::IService::getInstance()->getServiceId().get();
//			CMessage	mesOut(M_SC_CHITLIST);
//			mesOut.serial(_RID, r_sid);
//			mesOut.serial(uChitnum, chitid, _SID, SendFamily->m_Name, _ChitType);
//			mesOut.serial(_P1, _P2, _P3, _P4);
//
//			CUnifiedNetwork::getInstance ()->send(RecvFamily->m_FES, mesOut);
//		}
//	}
//}
//void	AddChit(T_FAMILYID _SID, T_FAMILYID _RID, uint8 _ChitType, uint32 _P1, uint32 _P2, ucstring _P3, ucstring _P4)
//{
//	MAKEQUERY_ADDCHIT(strQuery, _SID, _RID, _ChitType, _P1, _P2, _P3, _P4);
//	DBCaller->execute(strQuery, DBCB_DBAddChit,
//		"DDBDDSSW", 
//		CB_,		_SID,
//		CB_,		_RID,
//		CB_,		_ChitType,
//		CB_,		_P1,
//		CB_,		_P2,
//		CB_,		_P3.c_str(),
//		CB_,		_P4.c_str());
//}

