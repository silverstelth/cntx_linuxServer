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

#include "misc/DBCaller.h"
#include "net/service.h"
#include "net/unified_network.h"
#include "net/naming_client.h"

#include "World.h"
#include "../Common/DBLog.h"
#include "room.h"
#include "contact.h"
#include "Inven.h"
#include "Mail.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller	*DBCaller;
extern CDBCaller	*MainDBCaller;

void	cb_M_CS_CHATEAR(CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

/****************************************************************************
*	Lobby and Room Common CallbackArray
****************************************************************************/
TUnifiedCallbackItem WorldCallbackArray[] =
{
	{ M_NT_LOGIN,				cb_M_NT_LOGIN },

	{ M_NT_CHANGE_NAME,			cb_M_NT_CHANGE_NAME },

	// Money
	{ M_LS_USERMONEY,			cb_UserMoney },

	{ M_W_SHARDCODE,			cb_M_W_SHARDCODE },

	{ M_CS_CHANGE_CHDESC,		cb_M_CS_CHANGE_CHDESC },
	//{ M_CS_CHDESC,				cb_M_CS_CHDESC },

	{ M_CS_FINDROOM,			cb_M_CS_FINDROOM },
	{ M_CS_FINDFAMILY,			cb_M_CS_FINDFAMILY },
	{ M_CS_SET_FESTIVALALARM,   cb_M_CS_SET_FESTIVALALARM },
	//{ M_CS_SETROOMPWD,          cb_M_CS_SETROOMPWD },

	{ M_CS_CHATCOMMON,			cb_M_CS_CHATCOMMON		},
	{ M_CS_CHATROOM,			cb_M_CS_CHATROOM		},

	{ M_CS_MANAGEROOMS_GET,		cb_M_CS_MANAGEROOMS_GET			},

	// user reserve festival
	{ M_CS_REGFESTIVAL,			cb_M_CS_REGFESTIVAL },
	{ M_CS_MODIFYFESTIVAL,		cb_M_CS_MODIFYFESTIVAL },
	{ M_CS_DELFESTIVAL,			cb_M_CS_DELFESTIVAL },

	// HisSpace
	{ M_CS_BUYHISSPACE,			cb_M_CS_BUYHISSPACE },

	// Buy item and consume
	{ M_CS_FPROPERTY_MONEY,		cb_M_CS_FPROPERTY_MONEY },
	{ M_CS_INVENSYNC,			cb_M_CS_INVENSYNC },
	//{ M_CS_USEITEM,				cb_M_CS_USEITEM },
	{ M_CS_BUYITEM,				cb_M_CS_BUYITEM },
	//{ M_CS_THROWITEM,			cb_M_CS_THROWITEM },
	//{ M_CS_TIMEOUT_ITEM,		cb_M_CS_TIMEOUT_ITEM },
	//{ M_CS_TRIMITEM,			cb_M_CS_TRIMITEM },
	//{ M_CS_FITEMPOS,			cb_M_CS_FITEMPOS },

	//{ M_CS_BANKITEM_LIST,		cb_M_CS_BANKITEM_LIST },
	//{ M_CS_BANKITEM_GET,		cb_M_CS_BANKITEM_GET },

	// Mail
	//{ M_CS_MAIL_GET_LIST,		cb_M_CS_MAIL_GET_LIST },
	//{ M_CS_MAIL_GET,			cb_M_CS_MAIL_GET },
	{ M_CS_MAIL_SEND,			cb_M_CS_MAIL_SEND },
	{ M_NT_MAILBOX_STATUS_FOR_SEND,		cb_M_NT_MAILBOX_STATUS_FOR_SEND },
	{ M_SC_MAIL_SEND,			cb_M_SC_MAIL_SEND },
	//{ M_CS_MAIL_DELETE,			cb_M_CS_MAIL_DELETE },
	//{ M_CS_MAIL_REJECT,			cb_M_CS_MAIL_REJECT },
	{ M_CS_MAIL_ACCEPT_ITEM,	cb_M_CS_MAIL_ACCEPT_ITEM },
	{ M_NT_MAIL_ITEM_FOR_ACCEPT,cb_M_NT_MAIL_ITEM_FOR_ACCEPT },

	{ M_CS_ROOMFORFAMILY,		cb_M_CS_ROOMFORFAMILY },
	//{ M_CS_FAMILYCOMMENT,		cb_M_CS_FAMILYCOMMENT },
	{ M_CS_REQ_FAMILYCHS,		cb_M_CS_REQ_FAMILYCHS },
	{ M_CS_UNDELROOM,			cb_M_CS_UNDELROOM },

	// give room
	{ M_CS_CHANGEMASTER	,		cb_M_CS_CHANGEMASTER },
	{ M_CS_CHANGEMASTER_CANCEL	,		cb_M_CS_CHANGEMASTER_CANCEL },
	{ M_CS_CHANGEMASTER_ANS	,		cb_M_CS_CHANGEMASTER_ANS },
	{ M_NT_CHANGEMASTER_FORCE	,		cb_M_NT_CHANGEMASTER_FORCE },

	//{ M_CS_RESCHIT,				cb_M_CS_RESCHIT },
	//{ M_GIVEROOMRESULT,			cb_M_GIVEROOMRESULT },
	{ M_LS_CHECKPWD2_R,			cb_CheckPwd2 },

	//{ M_MS_ROOMCARD_CHECK,		cb_M_MS_ROOMCARD_CHECK },

	// Buy Room
	{ M_CS_GET_PROMOTION_PRICE,	cb_M_CS_GET_PROMOTION_PRICE },
	{ M_CS_RENEWROOM,			cb_M_CS_RENEWROOM },
	//{ M_CS_ROOMCARD_RENEW,		cb_M_CS_ROOMCARD_RENEW },

	// GM
	{ M_GMCS_ACTIVE,			cb_M_GMCS_ACTIVE },
	{ M_GMCS_SHOW,				cb_M_GMCS_SHOW },
	{ M_GMCS_OUTFAMILY,			cb_M_GMCS_OUTFAMILY },
	{ M_GMCS_PULLFAMILY,		cb_M_GMCS_PULLFAMILY },
	//{ M_GMCS_GIVEITEM,			cb_M_GMCS_GIVEITEM },
	//{ M_GMCS_GIVEMONEY,			cb_M_GMCS_GIVEMONEY },
	{ M_GMCS_GETMONEY,			cb_M_GMCS_GETMONEY },
	{ M_GMCS_GETITEM,			cb_M_GMCS_GETITEM },
	{ M_GMCS_GETINVENEMPTY,		cb_M_GMCS_GETINVENEMPTY },
	{ M_GMCS_GETLEVEL,			cb_M_GMCS_GETLEVEL },
	{ M_GMCS_SETLEVEL,			cb_M_GMCS_SETLEVEL },

	//{ M_GMCS_MAIL_SEND,			cb_M_GMCS_MAIL_SEND },
	{ M_GMCS_GET_GMONEY,		cb_M_GMCS_GET_GMONEY },

	// lobby
	//{ M_CS_REQ_CHANGECH,		cb_M_CS_REQ_CHANGECH },	
	{ M_CS_CHCHARACTER,			cb_M_CS_CHCHARACTER },	

	//// Bank Items
	//{ M_SC_BANKITEM_LIST,		cb_M_SC_BANKITEM_LIST },
	//{ M_MS_BANKITEM_GET,		cb_M_MS_BANKITEM_GET },

	// ??��?�玫规扁??
	//{ M_CS_RECOMMEND_ROOM,		cb_M_CS_RECOMMEND_ROOM },

	// ?�惑??
	{ M_CS_ANCESTOR_TEXT_SET,		cb_M_CS_ANCESTOR_TEXT_SET },
	{ M_CS_ANCESTOR_DECEASED_SET,	cb_M_CS_ANCESTOR_DECEASED_SET },

	// �����Ȱ������
	{ M_NT_CURRENT_ACTIVITY,		cb_M_NT_CURRENT_ACTIVITY },
	{ M_CS_REQ_ACTIVITY_ITEM,		cb_M_CS_REQ_ACTIVITY_ITEM },
	{ M_MS_SET_USER_ACTIVITY,		cb_M_MS_SET_USER_ACTIVITY },

	// ���ۻ���ھ�����
	{ M_NT_BEGINNERMSTITEM,			cb_M_NT_BEGINNERMSTITEM },
	{ M_CS_RECV_GIFTITEM,			cb_M_CS_RECV_GIFTITEM },
	{ M_MS_SET_USER_BEGINNERMSTITEM,cb_M_MS_SET_USER_BEGINNERMSTITEM },

	// ?�侩磊啊 绵汗墨靛???�侩窍看�?
	{ M_NT_BLESSCARD_USED,			cb_M_NT_BLESSCARD_USED },

	{ M_CS_AUTOPLAY_REQ_BEGIN,	cb_M_CS_AUTOPLAY_REQ_BEGIN },
	{ M_CS_AUTOPLAY_REQ,		cb_M_CS_AUTOPLAY_REQ },

	{ M_CS_AUTOPLAY2_REQ,		cb_M_CS_AUTOPLAY2_REQ },

	{ M_CS_AUTOPLAY3_REGISTER,	cb_M_CS_AUTOPLAY3_REGISTER },

	{ M_CS_LARGEACT_USEITEM,	cb_M_CS_LARGEACT_USEITEM },

	{ M_CS_CHECKIN,				cb_M_CS_CHECKIN },
	{ M_NT_RESCHECKIN,			cb_M_NT_RESCHECKIN },

	{ M_CS_CHATEAR,				cb_M_CS_CHATEAR			},

	{ M_CS_REQ_MST_DATA,		cb_M_CS_REQ_MST_DATA },

	// ������� ���谪ó���� ���� ����Ʈ
	{ M_NT_AUTOPLAY3_EXP_ADD_ONLINE,		cb_M_NT_AUTOPLAY3_EXP_ADD_ONLINE },
	{ M_NT_AUTOPLAY3_EXP_ADD_OFFLINE,		cb_M_NT_AUTOPLAY3_EXP_ADD_OFFLINE },
};

void init_WorldService()
{
	// Connect to database
	{
		string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Shard_DBName").asString ();
		string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Shard_DBHost").asString ();
		string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Shard_DBLogin").asString ();
		string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Shard_DBPassword").asString ();

		DBCaller = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
			ucstring(strDatabasePassword).c_str(),
			ucstring(strDatabaseHost).c_str(),
			ucstring(strDatabaseName).c_str());

		// DB龋免???�辑???�瘤?�咯??�??�夸??乐扁??���?4??1???�沥?�促.
		BOOL bSuccess = DBCaller->init(1);
		if (!bSuccess)
		{
			siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
				strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
			return;
		}
		//sipinfo("Shard Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str()); byy krs

		// Main DB
		strDatabaseName = IService::getInstance()->ConfigFile.getVar("Main_DBName").asString();
		strDatabaseHost = IService::getInstance()->ConfigFile.getVar("Main_DBHost").asString();
		strDatabaseLogin = IService::getInstance()->ConfigFile.getVar("Main_DBLogin").asString();
		strDatabasePassword = IService::getInstance()->ConfigFile.getVar("Main_DBPassword").asString();

		MainDBCaller = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
			ucstring(strDatabasePassword).c_str(),
			ucstring(strDatabaseHost).c_str(),
			ucstring(strDatabaseName).c_str());

		bSuccess = MainDBCaller->init(4);
		if (!bSuccess)
		{
			siperrornoex("Main Database connection failed to '%s' with login '%s' and database name '%s'",
				strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
			return;
		}
		//sipwarning("#############  Main Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str()); byy krs

	}

	T_DATETIME		StartTime_DB = "";

	// Get current time from server
	{
		MAKEQUERY_GETCURRENTTIME(strQ);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY(DB, Stmt, strQ);
		sipinfo("nQueryResult = %d", nQueryResult);
		if ( nQueryResult == true )
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (IS_DB_ROW_VALID(row))
			{
				COLUMN_DATETIME(row, 0, curtime);
				StartTime_DB = curtime;

				sipinfo("DBTime = %s", StartTime_DB.c_str());
			}
		}
	}

	if (StartTime_DB.length() < 1)
	{
		siperrornoex("Can't get current time of database server");
		return;
	}
	ServerStartTime_DB.SetTime(StartTime_DB);
	ServerStartTime_Local = CTime::getSecondsSince1970();

	// Load outdata
	LoadOutData();
	LoadDBMstData();
	LoadHolidayList();

	// 2硅版氰摹?�牢???�荤
	CheckTodayHoliday();
	CheckValidItemData();
	CUnifiedNetwork::getInstance()->addCallbackArray(WorldCallbackArray, sizeof(WorldCallbackArray)/sizeof(WorldCallbackArray[0]));
}

void update_WorldService()
{
	DBCaller->update();

	void updateDBCheckFunc();
	updateDBCheckFunc();
}

void release_WorldService()
{
	DBCaller->WaitForIdle();
	DBCaller->release();
	delete DBCaller;
	DBCaller = NULL;

	MainDBCaller->WaitForIdle();
	MainDBCaller->release();
	delete MainDBCaller;
	MainDBCaller = NULL;
}

MSSQLTIME		ServerStartTime_DB;
TTime			ServerStartTime_Local;

void	cb_M_NT_LOGIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ACCOUNTID		UserID;
	T_FAMILYNAME	FName;
	ucstring		UserName;
	uint8			bIsMobile;
	uint32			ModelID;
	uint8			uUserType;
	bool			bGM;
	string			strIP;

	_msgin.serial(FID, UserID, FName, UserName, bIsMobile, ModelID, uUserType, bGM, strIP);

	if (map_family.find(FID) == map_family.end())
	{
		AddFamilyData(FID, UserID, FName, UserName, bIsMobile, uUserType, bGM, strIP, CTime::getLocalTime());
	}

	SetFesForFamilyID(FID, _tsid);

	if (bIsMobile != 1)
	{
		extern void SendReadyToVIPForLogin(T_FAMILYID fid, TServiceId _tsid);
		SendReadyToVIPForLogin(FID, _tsid);

		extern void SendLargeActList(T_FAMILYID fid, TServiceId _tsid);
		SendLargeActList(FID, _tsid);
	}
}

extern	void	FamilyLogoutForBroker(T_FAMILYID _FID);

void	RemoveFamily(T_FAMILYID FID)
{
	// If family is in room, go out room
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager != NULL)
		inManager->OutRoom(FID);

	ProcRemoveFamilyForChannelWorld(FID);

	FamilyLogoutForBroker(FID);

	DeleteFamilyData(FID);
}

CWorld::CWorld(TWorldType WorldType) :
	m_WorldType(WorldType), m_RoomID(0), m_RoomName(L"")
{
	VisHis_Day = 0;
	VisHis_IDList.clear();
}

CWorld::~CWorld()
{
}

INFO_FAMILYINROOM* CWorld::GetFamilyInfo(T_FAMILYID FID)
{
	TRoomFamilys::iterator it = m_mapFamilys.find(FID);
	if (it == m_mapFamilys.end())
		return NULL;
	return &(it->second);
}


//void CWorld::EnterFamilyInWorld(T_FAMILYID FID)
//{
//	ReadFamilyInfo(FID);
//}

static void	DBCB_DBLoadFamilyInfoForEnter(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			FID	= *(uint32 *)argv[0];
	CWorld*			pWorld	= (CWorld*)argv[1];
	EnterFamilyInWorldCallback callback = (EnterFamilyInWorldCallback)argv[2];
	uint32			param	= *(uint32 *)argv[3];
	TServiceId		tsid(*(uint16 *)argv[4]);

	if (!nQueryResult)
		return;
	
	pWorld->EnterFamilyInWorld(FID, tsid, resSet, callback, param);
	
	FAMILY *	pFamily = GetFamilyData(FID); // by krs
	if (pFamily == NULL)
	{
		sipinfo("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	MAKEQUERY_SetAppType(strQ, pFamily->m_UserID, pFamily->m_bIsMobile);
	MainDBCaller->execute(strQ);	
}

void CWorld::EnterFamilyInWorld(T_FAMILYID FID, SIPNET::TServiceId tsid, void *resSet1, EnterFamilyInWorldCallback callback, uint32 param)
{
	CMemStream *resSet = (CMemStream *) resSet1;

	uint32	nRows;

	// tbl_Family
	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("nRows != 1 in tbl_Family. FID=%d", FID);
		return;
	}

	T_FAMILYNAME		FamilyName;//		resSet->serial(FamilyName);
	T_F_VISITDAYS		vdays;			resSet->serial(vdays);
	T_F_LEVEL			uLevel;			resSet->serial(uLevel);
	T_ROOMEXP			exp;			resSet->serial(exp);
	T_PRICE				GMoney;			resSet->serial(GMoney);
	uint16				room_count;		resSet->serial(room_count);

	// tbl_Character
	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("nRows != 1 in tbl_Character. FID=%d", FID);
		return;
	}

	uint32			CHID;		resSet->serial(CHID);
	T_FAMILYNAME	name;		resSet->serial(name);
	uint32			modelid;	resSet->serial(modelid);
	uint32			ddressid;	resSet->serial(ddressid);
	uint32			faceid;		resSet->serial(faceid);
	uint32			hairid;		resSet->serial(hairid);
	uint8			master;		resSet->serial(master);
	uint32			dressid;	resSet->serial(dressid);
	uint32			birthDate;	resSet->serial(birthDate);
	uint32			city;		resSet->serial(city);
	ucstring		resume;		resSet->serial(resume);

	FamilyName = name;

	CHARACTER character(CHID, name, FID, master, modelid, ddressid, faceid, hairid, dressid, birthDate, city, resume);
	AddCharacterData( character );

	uint8	uRoomRole = ROOM_NONE;
	INFO_FAMILYINROOM	fi(FID, CHID, modelid, FamilyName, uLevel, exp, GMoney, resume, uRoomRole, room_count, tsid);
	m_mapFamilys.insert( make_pair(FID, fi) );

	TFesFamilyIds::iterator it0 = m_mapFesFamilys.find(tsid);
	if (it0 == m_mapFesFamilys.end())
	{
		MAP_FAMILYID	newMap;
		newMap.insert( make_pair(FID, FID) );
		m_mapFesFamilys.insert(make_pair(tsid, newMap));
	}
	else
	{
		it0->second.insert( make_pair(FID, FID) );
	}


	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	// tbl_Item
	resSet->serial(nRows);
	if (nRows == 1)
	{
		uint32	len;		resSet->serial(len);
		uint8	InvenBuf[MAX_INVENBUF_SIZE];
		resSet->serialBuffer(InvenBuf, len);

		if(!pInfo->m_InvenItems.fromBuffer(InvenBuf, len))
		{
			sipwarning("failed fromBuffer inven items! FID=%d", FID);
		}
	}

	// tbl_family_ExpHis
	uint32				exp_his_data_len = 0;
	uint8				exp_his_data[sizeof(_Family_Exp_His) * FAMILY_EXP_TYPE_MAX];
	resSet->serial(nRows);
	if (nRows == 1)
	{
		resSet->serial(exp_his_data_len);
		resSet->serialBuffer(exp_his_data, exp_his_data_len);
	}

	// ExpHisData
	if (exp_his_data_len == sizeof(pInfo->m_FamilyExpHis))
	{
		memcpy(&pInfo->m_FamilyExpHis, exp_his_data, sizeof(pInfo->m_FamilyExpHis));
	}
	else
	{
		if (exp_his_data_len != 0)
			sipwarning("exp_his_data_len != sizeof(pInfo->m_FamilyExpHis). FID=%d, %d, %d", FID, exp_his_data_len, sizeof(pInfo->m_FamilyExpHis));
		memset(&pInfo->m_FamilyExpHis, 0, sizeof(pInfo->m_FamilyExpHis));
	}

	// Refresh UserMoney
	RefreshFamilyMoney(FID);

	CMessage msgHis(M_NT_ENTERLOBBY);
	msgHis.serial(FID);
	bool isGM = false;
	MAP_FAMILY::iterator fit = map_family.find(FID);
	if (fit != map_family.end()) {
		isGM = fit->second.m_bGM;
	}
	msgHis.serial(isGM);
	
	CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgHis);

	callback(FID, this, param);
}

void CWorld::EnterFamilyInWorld(T_FAMILYID FID, SIPNET::TServiceId tsid, EnterFamilyInWorldCallback callback, uint32 param)
{
	SetFesForFamilyID(FID, tsid);

	MAKEQUERY_LoadFamilyInfoForEnter(strQ, FID);
	DBCaller->executeWithParam(strQ, DBCB_DBLoadFamilyInfoForEnter,
		"DPPDW",
		CB_,		FID,
		CB_,		(uint64) this,
		CB_,		(uint64) callback,
		CB_,		param,
		CB_,		tsid.get());
}

void CWorld::LeaveFamilyInWorld(T_FAMILYID FID)
{
	SaveFamilyInfo(FID);
}

// Room Chatting
void CWorld::SendChatRoom(T_FAMILYID _FID, T_CHATTEXT _Chat)
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_CHATROOM)
		msgOut.serial(_FID, _Chat);
	FOR_END_FOR_FES_ALL()
}

// Normal Chatting
void CWorld::SendChatCommon(T_FAMILYID _FID, T_CHATTEXT _Chat)
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_CHATCOMMON)
		msgOut.serial(_FID, _Chat);
	FOR_END_FOR_FES_ALL()
}


static ucchar st_str_buf[MAX_INVENBUF_SIZE*2 + 10];

bool SaveFamilyInven(T_FAMILYID FID, _InvenItems *pInven, bool bDBNormal)
{
	uint8	InvenBuf[MAX_INVENBUF_SIZE];
	uint32	InvenBufSize = sizeof(InvenBuf);

	if (!pInven->toBuffer(InvenBuf, &InvenBufSize))
	{
		sipwarning("Can't toBuffer inven items! FID=%d", FID);
		return false;
	}

	for (uint32 i = 0; i < InvenBufSize; i ++)
	{
#ifdef SIP_OS_WINDOWS
		smprintf((ucchar*)(&st_str_buf[i * 2]), 2, _t("%02x"), InvenBuf[i]);
#else
		smprintf((ucchar*)(&st_str_buf[i * 2]), 4, _t("%02x"), InvenBuf[i]);
#endif
	}
	st_str_buf[InvenBufSize * 2] = 0;

	
	/* // Never remove this comment!!!
	MAKEQUERY_SaveFamilyInven(strQ, FID, st_str_buf);
	if (bDBNormal)
	{
		DBCaller->executeWithParam(strQ, NULL,
			"D",
			OUT_,		NULL);
	}
	else
	{
		DBCaller->executeWithParam2(strQ, 0,NULL,
			"D",
			OUT_,		NULL);
	}
	*/
	MAKEQUERY_SaveFamilyInven(strQ, FID, st_str_buf);

	CDBConnectionRest	DB(DBCaller);
	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
	if (!nPrepareRet)
	{
		sipwarning("DB_EXE_PREPARESTMT failed in Shrd_Item_InsertAndUpdate!");
		return false;
	}

	SQLLEN	num2 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num2);
	if (!nBindResult)
	{
		sipwarning("bind parameter failed in Shrd_Item_InsertAndUpdate!!");
		return false;
	}

	DB_EXE_PARAMQUERY(Stmt, strQ);
	if (!nQueryResult || ret == -1)
	{
		sipwarning("execute failed in Shrd_Item_InsertAndUpdate!");
		return false;
	}

	return true;
}

//// Save Family Info
//// Level&Exp
//void INFO_FAMILYINROOM::SaveFamilyLevelAndExp(bool bMustSave)
//{
//	if (m_bFamilyExpChanged)
//	{
//		uint32	now = CTime::getSecondsSince1970();
//		if (bMustSave || (now - m_dwFamilyExpSavedTime > 300))	// 5min
//		{
//			MAKEQUERY_SaveFamilyLevelAndExp(strQ, m_FID, m_Level, m_Exp);
//			DBCaller->executeWithParam(strQ, NULL,
//				"D",
//				OUT_,		NULL);
//
//			m_bFamilyExpChanged = false;
//			m_dwFamilyExpSavedTime = now;
//		}
//		else
//		{
//			sipinfo("FamilyExp changed but not saved. FID=%d, Level=%d, Exp=%d", m_FID, m_Level, m_Exp);
//		}
//	}
//}
//
//void INFO_FAMILYINROOM::SaveFamilyExpHistory(bool bMustSave)
//{
//	if (m_bFamilyExpHisChanged)
//	{
//		uint32	now = CTime::getSecondsSince1970();
//		if (bMustSave || (now - m_dwFamilyExpHisSavedTime > 300))	// 5min
//		{
//			uint32 FID = m_FID;
//
//			int	num1 = sizeof(m_FamilyExpHis);
//			uint8 *buf = (uint8*) (&m_FamilyExpHis);
//			ucchar str_buf[sizeof(m_FamilyExpHis) * 2 + 10];
//			for (int i = 0; i < num1; i ++)
//			{
//				smprintf((ucchar*)(&str_buf[i * 2]), 2, _t("%02x"), buf[i]);
//			}
//			str_buf[num1 * 2] = 0;
//
//			MAKEQUERY_SaveFamilyExpHisData(strQ, FID, str_buf);
//
//			DBCaller->executeWithParam(strQ, NULL,
//				"D",
//				OUT_,		NULL);
//
//			m_bFamilyExpHisChanged = false;
//			m_dwFamilyExpHisSavedTime = now;
//		}
//		else
//		{
//			sipinfo("FamilyExpHis changed but not saved. FID=%d", m_FID);
//		}
//	}
//}

// Save Family Info, Level, Exp, ExpHisData
void INFO_FAMILYINROOM::SaveFamilyExpData(bool bMustSave)
{
	if (m_bFamilyExpDataChanged)
	{
		uint32	now = CTime::getSecondsSince1970();
		if (bMustSave || (now - m_dwFamilyExpDataSavedTime > 300))	// 5min
		{
			uint32 FID = m_FID;

			int	num1 = sizeof(m_FamilyExpHis);
			uint8 *buf = (uint8*) (&m_FamilyExpHis);
			ucchar str_buf[sizeof(m_FamilyExpHis) * 2 + 10];
			for (int i = 0; i < num1; i ++)
			{
				smprintf((ucchar*)(&str_buf[i * 2]), 2, _t("%02x"), buf[i]);
			}
			str_buf[num1 * 2] = 0;

			{
				MAKEQUERY_SaveFamilyLevelAndExp(strQ, FID, m_Level, m_Exp, m_nFamilyLevelChanged);

				DBCaller->executeWithParam(strQ, NULL,
					"");
			}

			{
				MAKEQUERY_SaveFamilyExpHisData(strQ, FID, str_buf);

				DBCaller->executeWithParam(strQ, NULL,
					"D",
					OUT_,		NULL);
			}

			m_bFamilyExpDataChanged = false;
			m_nFamilyLevelChanged = 0;
			m_dwFamilyExpDataSavedTime = now;
		}
		else
		{
			//sipinfo("FamilyExpData changed but not saved. FID=%d, Level=%d, Exp=%d", m_FID, m_Level, m_Exp); byy krs
		}
	}
}

// Save Family Info
// Inven, ExpHisData, Level&Exp
bool CWorld::SaveFamilyInfo(T_FAMILYID FID)
{
	TRoomFamilys::iterator	itF = m_mapFamilys.find(FID);
	if(itF == m_mapFamilys.end())
	{
		sipwarning("Can't find user! FID=%d", FID);
		return false;
	}

	INFO_FAMILYINROOM	&FamilyInfo = itF->second;

	//if(!SaveFamilyInven(FID, &FamilyInfo.m_InvenItems))
	//{
	//	sipwarning("SaveFamilyInven failed. FID=%d", FID);
	//}

	//FamilyInfo.SaveFamilyLevelAndExp(true);
	//FamilyInfo.SaveFamilyExpHistory(true);
	FamilyInfo.SaveFamilyExpData(true);

	return true;
}

_InvenItems* CWorld::GetFamilyInven(T_FAMILYID FID)
{
	TRoomFamilys::iterator	itFamily = m_mapFamilys.find(FID);
	if(itFamily == m_mapFamilys.end())
	{
		sipwarning(L"Can't find FID in pWorld->m_mapFamilys. FID=%d", FID);
		return NULL;
	}
	return	&itFamily->second.m_InvenItems;
}

bool CWorld::CheckIfRoomExpIncWhenFamilyEnter(uint32 FID)
{
	uint32	today = GetDBTimeDays();
	if(today != VisHis_Day)
	{
		VisHis_Day = today;
		VisHis_IDList.clear();
	}

	if(VisHis_IDList.size() >= mstRoomExp[Room_Exp_Type_Visitroom].MaxCount)
	{
		return false;
	}

	std::map<uint32, uint32>::iterator	it = VisHis_IDList.find(FID);
	if (it != VisHis_IDList.end())
		return false;

	return true;
}

bool CWorld::CheckIfFamilyExpIncWhenFamilyEnter(uint32 FID)
{
	uint32	today = GetDBTimeDays();
	if(today != VisHis_Day)
	{
		VisHis_Day = today;
		VisHis_IDList.clear();
	}

	std::map<uint32, uint32>::iterator	it = VisHis_IDList.find(FID);
	if (it != VisHis_IDList.end())
		return false;

	return true;
}

void CWorld::FamilyEnteredForExp(uint32 FID)
{
	uint32	today = GetDBTimeDays();
	if(today != VisHis_Day)
	{
		VisHis_Day = today;
		VisHis_IDList.clear();
	}

	// Update VisHis_IDList
	VisHis_IDList[FID] = FID;
}

void ChangeFamilyExp(uint8 FamilyExpType, T_FAMILYID FID, uint32 ExpParam)
{
	CWorld*	pWorld = GetWorldFromFID(FID);
	if (pWorld == NULL)
	{
		sipwarning("Can't find CWorld !!! FID=%d, FamilyExpType=%d", FID, FamilyExpType);
//		return 0;
	}
	else
	{
		pWorld->ChangeFamilyExp(FamilyExpType, FID, ExpParam);
	}
}

// Family
void CWorld::ChangeFamilyExp(uint8 FamilyExpType, T_FAMILYID FID, uint32 ExpParam)
{
	// int	bExpChanged = 0, bExpHisChanged = 0;

	TRoomFamilys::iterator	it = m_mapFamilys.find(FID);
	if (it == m_mapFamilys.end())
	{
		sipwarning("FamilyID can't find in m_mapFamilys !!! FID=%d", FID);
		return;
	}
	INFO_FAMILYINROOM	&FamilyInfo = it->second;

	uint32		addExp = 0;
	T_F_EXP		exp0 = FamilyInfo.m_Exp;
	T_F_LEVEL	level0 = FamilyInfo.m_Level;

//	switch (FamilyExpType)
//	{
	//case Family_Exp_Type_UseMoney:
	//	if ((UMoney > 0) && (AddGMoney > 0))
	//	{
	//		// Add Family Point(GMoney)
	//		ExpendMoney(FID, 0, AddGMoney, 2, DBLOG_MAINTYPE_CHFGMoney, 0, 0, 0);
	//	}
	//	break;

//	default:
		{
			uint32	today = GetDBTimeDays();

			if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
			{
				FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
				FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;

				//FamilyInfo.m_bFamilyExpHisChanged = true;
			}
			if (FamilyExpType == Family_Exp_Type_NianFo)
			{
				uint32	OldTotalDoingTime = FamilyInfo.m_FamilyExpHis[FamilyExpType].Count;
				uint32	CurTotalDoingTime = FamilyInfo.m_FamilyExpHis[FamilyExpType].Count + ExpParam;
				if (CurTotalDoingTime > OldTotalDoingTime )
				{
					if (OldTotalDoingTime == 0)
					{
						addExp = mstFamilyExp[FamilyExpType].Exp;
						// bExpHisChanged = 1;
					}
					else
					{
						uint32	OldDoingHours = OldTotalDoingTime / 3600;
						uint32	CurDoingHours = CurTotalDoingTime / 3600;
						if (CurDoingHours > OldDoingHours)
						{
							addExp = (CurDoingHours - OldDoingHours) * mstFamilyExp[FamilyExpType].Exp;
							// bExpHisChanged = 1;
						}
					}
					FamilyInfo.m_FamilyExpHis[FamilyExpType].Count += ExpParam;
				}
			}
			else
			{
				if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
				{
					FamilyInfo.m_FamilyExpHis[FamilyExpType].Count ++;
					addExp = mstFamilyExp[FamilyExpType].Exp;

					// bExpHisChanged = 1;
				}
				else if(FamilyExpType == Family_Exp_Type_Shenming && m_RoomID == 10000001)
				{
					addExp = mstFamilyExp[FamilyExpType].Exp;
					CheckPrize(FID, PRIZE_TYPE_LUCK, 0);
				}
			}
		}
//		break;
//	}

	// 2배경?�일 처리
	if (g_bIsHoliday)
		addExp *= 2;

	FamilyInfo.m_Exp += addExp;
	if (exp0 != FamilyInfo.m_Exp)
	{
		bool	bLevelChanged = false;

		while (FamilyInfo.m_Level < FAMILY_LEVEL_MAX)
		{
			if (FamilyInfo.m_Exp < mstFamilyLevel[FamilyInfo.m_Level + 1].Exp)
				break;
			FamilyInfo.m_Level ++;

			bLevelChanged = true;

			// Add Family Point(GMoney)
			if (mstFamilyLevel[FamilyInfo.m_Level].AddPoint > 0)
				ExpendMoney(FID, 0, mstFamilyLevel[FamilyInfo.m_Level].AddPoint, 2, DBLOG_MAINTYPE_CHFGMoney, 0, 0, 0);
		}

		FamilyInfo.m_bFamilyExpDataChanged = true;
		if (bLevelChanged)
			FamilyInfo.m_nFamilyLevelChanged = 1;

		// if (bSaveDB || bLevelChanged)
		{
			// Save to DB
			//FamilyInfo.SaveFamilyLevelAndExp(bLevelChanged);
			//FamilyInfo.SaveFamilyExpHistory(bLevelChanged);
			FamilyInfo.SaveFamilyExpData(bLevelChanged);
		}

		// Log
		ucstring ItemInfo = L"";
		DBLOG_CHFamilyExp(FID, FamilyInfo.m_Name, exp0, FamilyInfo.m_Exp, FamilyInfo.m_Level, FamilyExpType, m_RoomID, m_RoomName, ItemInfo);

		// Notice to Client
		SendFamilyProperty_Level(FamilyInfo.m_ServiceID, FID, FamilyInfo.m_Exp, FamilyInfo.m_Level);

		// GMoney???�窍?�促.
		if (bLevelChanged)
			SendFamilyProperty_Money(FamilyInfo.m_ServiceID, FID, FamilyInfo.m_nGMoney, FamilyInfo.m_nUMoney);
	}

//	return bExpChanged | (bExpHisChanged << 4);
}

// ?�腐磐狼 ?�怕甫 汲沥?�促
void	CWorld::SetCharacterState(T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_DIRECTION _Dir, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z, ucstring _AniName, T_CH_STATE _State, T_FITEMID _HoldItem)
{
	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator	it = m_mapFamilys.find(_FID);
	if (it != m_mapFamilys.end())
	{
		INFO_FAMILYINROOM *f = &(it->second);
		f->SetCharacterState(_CHID, _Dir, _X, _Y, _Z, _AniName, _State, _HoldItem);
	}
}

void	CWorld::SetCharacterPos(T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z)
{
	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator	it = m_mapFamilys.find(_FID);
	if (it != m_mapFamilys.end())
	{
		INFO_FAMILYINROOM *f = &(it->second);
		f->SetCharacterPos(_CHID, _X, _Y, _Z);
	}
}

static void	DBCB_DBGetOwnManageRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			FID	= *(uint32 *)argv[0];
	uint32			targetFID	= *(uint32 *)argv[1];
	TServiceId		tsid(*(uint16 *)argv[2]);

	uint8	bStart, bEnd;
	bStart = 1;
	CMessage	msgOut(M_SC_MANAGEROOMS_LIST);

	msgOut.serial(FID, targetFID, bStart);

	uint32	nCurTime = (uint32)(GetDBTimeSecond() / 60);

	uint32	nRows;	resSet->serial(nRows);
	for(uint32 i = 0; i < nRows; i ++)
	{
		// Packet??辨捞???�公 辨绢�??�扼�??�辰�?
		if(msgOut.length() > 2500)
		{
			bEnd = 0;
			msgOut.serial(bEnd);
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);

			CMessage	msgtmp(M_SC_MANAGEROOMS_LIST);
			bStart = 0;
			msgtmp.serial(FID, targetFID, bStart);

			msgOut = msgtmp;
		}

		uint32		RoomID;			resSet->serial(RoomID);
		uint16		SceneID;		resSet->serial(SceneID);
		T_DATETIME	termtime;		resSet->serial(termtime);
		ucstring	RoomName;		resSet->serial(RoomName);
		uint8		OpenFlag;		resSet->serial(OpenFlag);
		uint32		Visits;			resSet->serial(Visits);
		uint8		Degree;			resSet->serial(Degree);
		uint32		Exp;			resSet->serial(Exp);
		uint32		OwnerFID;		resSet->serial(OwnerFID);
		ucstring	OwnerName;		resSet->serial(OwnerName);
		uint8		SurnameLen1;	resSet->serial(SurnameLen1);
		ucstring	DeadName1;		resSet->serial(DeadName1);
		uint8		SurnameLen2;	resSet->serial(SurnameLen2);
		ucstring	DeadName2;		resSet->serial(DeadName2);
		uint8		PhotoType;		resSet->serial(PhotoType);
		uint8		Permission;		resSet->serial(Permission);

		if(!termtime.empty())
		{
			uint32	nTermTime = getMinutesSince1970(termtime);
			if(nTermTime <= nCurTime)
				continue;
		}

		//uint16	exp_percent = CalcRoomExpPercent(Degree, Exp);

		msgOut.serial(RoomID, Permission, SceneID, RoomName, OpenFlag, Visits, Degree, Exp); // , exp_percent);
		msgOut.serial(OwnerFID, OwnerName, PhotoType, DeadName1, SurnameLen1, DeadName2, SurnameLen2);
	}

	bEnd = 1;
	msgOut.serial(bEnd);
	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

void cb_M_CS_MANAGEROOMS_GET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32		FID, targetFID, RoomID;

	_msgin.serial(FID, targetFID, RoomID);

	MAKEQUERY_GetOwnManageRoom(strQ, targetFID, RoomID);
	DBCaller->executeWithParam(strQ, DBCB_DBGetOwnManageRoom,
		"DDW",
		CB_,		FID,
		CB_,		targetFID,
		CB_,		_tsid.get());
}

//static void	DBCB_DBGetRecommendRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	uint32			FID			= *(uint32 *)argv[0];
//	TServiceId		tsid(*(uint16 *)argv[1]);
//
//	CMessage	msgOut(M_SC_RECOMMEND_ROOM);
//	msgOut.serial(FID);
//
//	uint32	nRows;	resSet->serial(nRows);
//	for(uint32 i = 0; i < nRows; i ++)
//	{
//		uint8		Index;
//		uint32		RoomID, PhotoID, Date;
//		resSet->serial(Index, RoomID, PhotoID, Date);
//
//		msgOut.serial(Index, RoomID, PhotoID, Date);
//	}
//
//	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//}

// Request recommend room list
//void cb_M_CS_RECOMMEND_ROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	uint32		FID;
//
//	_msgin.serial(FID);
//
//	MAKEQUERY_GetRecommendRoom(strQuery);
//	DBCaller->executeWithParam(strQuery, DBCB_DBGetRecommendRoom,
//		"DW",
//		CB_,		FID,
//		CB_,		_tsid.get());
//}

std::vector<uint32>	g_HolidayList;
bool	g_bIsHoliday = false;
void LoadHolidayList()
{
	g_HolidayList.clear();

	{
		MAKEQUERY_GetHolidayList(strQuery);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
		if (nQueryResult == true)
		{
			do
			{
				DB_QUERY_RESULT_FETCH( Stmt, row );
				if (IS_DB_ROW_VALID( row ))
				{
					COLUMN_DIGIT( row, 0, uint32, HolidayID );
					COLUMN_DATETIME(row, 1, Holiday);
					COLUMN_WSTR( row, 2, HolidayComment, 100);

					uint32	holiday = getDaysSince1970(Holiday);
					g_HolidayList.push_back(holiday);
				}
				else
					break;
			} while (true);
		}
	}
}

void CheckTodayHoliday()
{
	g_bIsHoliday = false;

	uint32	today = GetDBTimeDays();
	for (std::vector<uint32>::iterator it = g_HolidayList.begin(); it != g_HolidayList.end(); it ++)
	{
		if (*it == today)
		{
			g_bIsHoliday = true;
			return;
		}
	}
}

uint32 CWorld::FamilyItemUsed(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, uint8 ItemType, SIPNET::TServiceId _sid)
{
	uint32	ItemID = 0;
	const	MST_ITEM	*mstItem = NULL;
	if (!IsValidInvenPos(InvenPos))
	{
		// ?�过颇纳飘惯??
		ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos! UserFID=%d, InvenPos=%d", FID, InvenPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return 0;
	}

	_InvenItems*	pInven = GetFamilyInven(FID);
	if (pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL. FID=%d", FID);
		return 0;
	}
	int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
	ITEM_INFO	&item = pInven->Items[InvenIndex];
	ItemID = item.ItemID;

	mstItem = GetMstItemInfo(ItemID);
	if ((mstItem == NULL) || (mstItem->TypeID != ItemType))
	{
		sipwarning(L"Invalid Item. FID=%d, InvenPos=%d, SyncCode=%d, ItemID=%d, ReqItemType=%d", FID, InvenPos, SyncCode, ItemID, ItemType);
		return 0;
	}

	// Check OK
	// Delete from Inven
	if (1 < item.ItemCount)
	{
		item.ItemCount --;
	}
	else
	{
		item.ItemID = 0;
	}

	if (!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
		return 0;
	}

	DBLOG_UseItem(FID, 0, ItemID, 1, mstItem->Name);

	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
	}

	return ItemID;
}

void cb_M_NT_BLESSCARD_USED(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;

	_msgin.serial(FID);

	ChangeFamilyExp(Family_Exp_Type_Blesscard, FID);
}

void cb_M_CS_LARGEACT_USEITEM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint16		InvenPos;
	uint32		SyncCode;
	_msgin.serial(FID, InvenPos, SyncCode);

	CWorld *inManager = GetWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->on_M_CS_LARGEACT_USEITEM(FID, InvenPos, SyncCode, _tsid);
}

void	CWorld::on_M_CS_LARGEACT_USEITEM(T_FAMILYID FID, uint16 invenpos, uint32 synccode, SIPNET::TServiceId _sid)
{
	FamilyItemUsed(FID, invenpos, synccode, ITEMTYPE_SACRIF_JINGXIAN, _sid);
}

void	cb_M_CS_CHECKIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}
	std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = pFamilyInfo->m_mapCH.begin();
	if (it == pFamilyInfo->m_mapCH.end())
	{
		sipwarning("pFamilyInfo->m_mapCH is empty. FID=%d", FID);
		return;
	}

	CMessage msg(M_NT_REQCHECKIN);
	msg.serial(FID, it->second.m_ModelID);
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msg);
}

void	cb_M_NT_RESCHECKIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	sint8		nCheckinDays;
	uint8		nItemCount = 0;
	_msgin.serial(FID, nCheckinDays);

	FAMILY*	pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
		return;

	CMessage msg(M_SC_CHECKIN);
	msg.serial(FID, nCheckinDays);
	if (nCheckinDays != CIS_CANT_CHECK)
		_msgin.serial(nItemCount);

	_InvenItems	*pInven = GetFamilyInven(FID);
	if (pInven != NULL)
	{
		int		InvenIndex = 0;
		bool	bInvenChange = false;
		for (uint8 i = 0; i < nItemCount; i++)
		{
			T_ITEMSUBTYPEID	ItemID;	
			T_FITEMNUM		ItemNum;
			_msgin.serial(ItemID, ItemNum);
			while (InvenIndex < MAX_INVEN_NUM && pInven->Items[InvenIndex].ItemID != NOITEMID)
			{
				InvenIndex ++;
			}
			if (InvenIndex >= MAX_INVEN_NUM)
				break;
			pInven->Items[InvenIndex].ItemID = ItemID;
			pInven->Items[InvenIndex].ItemCount = ItemNum;
			pInven->Items[InvenIndex].GetDate = 0;
			pInven->Items[InvenIndex].UsedCharID = 0;
			pInven->Items[InvenIndex].MoneyType = MONEYTYPE_UMONEY;
			bInvenChange = true;

			T_FITEMPOS InvenPos = GetInvenPosFromInvenIndex(InvenIndex);
			msg.serial(ItemID, ItemNum, InvenPos);
			InvenIndex ++;

			// Log
			const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
			if ( mstItem == NULL )
			{
				sipwarning(" GetMstItemInfo = NULL. ItemID = %d", ItemID);
			}
			else
			{
				DBLOG_GetBankItem(FID, ItemID, ItemNum, mstItem->Name);
			}
		}
		if (bInvenChange)
		{
			if (!SaveFamilyInven(FID, pInven))
			{
				sipwarning("SaveFamilyInven failed. FID=%d", FID);
			}
		}
	}
	else
	{
		sipwarning("GetFamilyInven NULL FID=%d", FID);
	}

	CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msg);
}


//////////////////////////////////////
// ������� ���谪ó��
//////////////////////////////////////

void ChangeFamilyExpForAutoplay3(INFO_FAMILYINROOM &FamilyInfo, uint32 RoomID, ucstring &RoomName, 
								uint32 CountPerDay, uint32 YishiTaocanItemID, uint32 XianbaoTaocanItemID, uint32 YangyuItemID, uint32 Param,
								uint32 TianItemID, uint32 DiItemID)
{
	// ���: ���ֱ�, �ɵ帮��, ����帮��, ����������ֱ�, õ���ǽ�, �����¿��, �ϴ��ǽ�, ���ǽ�
	// Family_Exp_Type_Visitday, Family_Exp_Type_Visitroom - �̰� ���� �ʴ°����� ����!! (Family_Exp_Type_Visitroomó���� ��������)
	// Family_Exp_Type_Flower_Water, Family_Exp_Type_XingLi, Family_Exp_Type_Memo, 
	// Family_Exp_Type_Fish, Family_Exp_Type_Jisi_One, Family_Exp_Type_Xianbao

	uint32		addExp = 0;
	T_F_EXP		exp0 = FamilyInfo.m_Exp;
	T_F_LEVEL	level0 = FamilyInfo.m_Level;

	uint32	today = GetDBTimeDays();

	uint8	FamilyExpType;
	FamilyExpType = Family_Exp_Type_Flower_Water;	// ���ֱ� 1��
	if (Param & 1)	// ���ֱⰡ ����ǿ�����
	{
		if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;
		}

		if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count ++;
			addExp += mstFamilyExp[FamilyExpType].Exp;
		}
	}

	FamilyExpType = Family_Exp_Type_XingLi;	// �λ�帮�� 1��
	if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
	{
		if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;
		}

		FamilyInfo.m_FamilyExpHis[FamilyExpType].Count ++;
		addExp += mstFamilyExp[FamilyExpType].Exp;
	}

	FamilyExpType = Family_Exp_Type_Memo;	// ����帮�� 1��
	if (Param & 2)	// ����帮�Ⱑ ����ǿ�����
	{
		if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;
		}

		if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count ++;
			addExp += mstFamilyExp[FamilyExpType].Exp;
		}
	}

	FamilyExpType = Family_Exp_Type_Fish;	// ����������ֱ� CountPerDay��
	if (YangyuItemID != 0)
	{
		if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;
		}

		if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
		{
			uint32	count = mstFamilyExp[FamilyExpType].MaxCount - FamilyInfo.m_FamilyExpHis[FamilyExpType].Count;
			if (count > CountPerDay)
				count = CountPerDay;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count += count;
			addExp += (mstFamilyExp[FamilyExpType].Exp * count);
		}
	}

	FamilyExpType = Family_Exp_Type_Jisi_One;	// õ���ǽ� CountPerDay��
	if (YishiTaocanItemID != 0)
	{
		if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;
		}

		if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
		{
			uint32	count = mstFamilyExp[FamilyExpType].MaxCount - FamilyInfo.m_FamilyExpHis[FamilyExpType].Count;
			if (count > CountPerDay)
				count = CountPerDay;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count += count;
			addExp += (mstFamilyExp[FamilyExpType].Exp * count);
		}
	}

	FamilyExpType = Family_Exp_Type_Xianbao;	// �����¿�� CountPerDay��
	if (XianbaoTaocanItemID != 0)
	{
		if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;
		}

		if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
		{
			uint32	count = mstFamilyExp[FamilyExpType].MaxCount - FamilyInfo.m_FamilyExpHis[FamilyExpType].Count;
			if (count > CountPerDay)
				count = CountPerDay;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count += count;
			addExp += (mstFamilyExp[FamilyExpType].Exp * count);
		}
	}

	FamilyExpType = Family_Exp_Type_ReligionYiSiMaster;	// �ϴ��ǽ� CountPerDay��
	if (TianItemID != 0)
	{
		if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;
		}

		if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
		{
			uint32	count = mstFamilyExp[FamilyExpType].MaxCount - FamilyInfo.m_FamilyExpHis[FamilyExpType].Count;
			if (count > CountPerDay)
				count = CountPerDay;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count += count;
			addExp += (mstFamilyExp[FamilyExpType].Exp * count);
		}
	}

	FamilyExpType = Family_Exp_Type_ReligionYiSiMaster;	// ���ǽ� CountPerDay��
	if (DiItemID != 0)
	{
		if (today != FamilyInfo.m_FamilyExpHis[FamilyExpType].Day)
		{
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Day = today;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count = 0;
		}

		if (mstFamilyExp[FamilyExpType].MaxCount > FamilyInfo.m_FamilyExpHis[FamilyExpType].Count)
		{
			uint32	count = mstFamilyExp[FamilyExpType].MaxCount - FamilyInfo.m_FamilyExpHis[FamilyExpType].Count;
			if (count > CountPerDay)
				count = CountPerDay;
			FamilyInfo.m_FamilyExpHis[FamilyExpType].Count += count;
			addExp += (mstFamilyExp[FamilyExpType].Exp * count);
		}
	}

	if (addExp == 0)
		return;

	// 2�����ġ��
	if (g_bIsHoliday)
		addExp *= 2;

	FamilyInfo.m_Exp += addExp;
	bool	bLevelChanged = false;

	while (FamilyInfo.m_Level < FAMILY_LEVEL_MAX)
	{
		if (FamilyInfo.m_Exp < mstFamilyLevel[FamilyInfo.m_Level + 1].Exp)
			break;
		FamilyInfo.m_Level ++;

		bLevelChanged = true;

		// Add Family Point(GMoney)
		if (mstFamilyLevel[FamilyInfo.m_Level].AddPoint > 0)
			FamilyInfo.m_nGMoney += mstFamilyLevel[FamilyInfo.m_Level].AddPoint;
	}

	FamilyInfo.m_bFamilyExpDataChanged = true;
	if (bLevelChanged)
		FamilyInfo.m_nFamilyLevelChanged = 1;

	// Log
	ucstring ItemInfo = L"";
	DBLOG_CHFamilyExp(FamilyInfo.m_FID, FamilyInfo.m_Name, exp0, FamilyInfo.m_Exp, FamilyInfo.m_Level, 0, RoomID, RoomName, ItemInfo);
}

// ������翡 ���� ����ġ���� ���� LS->WS->MS->OROOM ([d:FID][d:YishiTimeMin][d:Autoplay3ID][d:RoomID][u:RoomName][d:CountPerDay][d:YishiTaocanItemID][d:XianbaoTaocanItemID][d:YangyuItemID][d:ItemID_Tian][d:ItemID_Di][d:Param])
void cb_M_NT_AUTOPLAY3_EXP_ADD_ONLINE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32	FID, YishiTimeMin, Autoplay3ID, RoomID, CountPerDay, YishiTaocanItemID, XianbaoTaocanItemID, YangyuItemID, TianItemID, DiItemID, Param;
	ucstring	FamilyName, RoomName;

	_msgin.serial(FID, YishiTimeMin, Autoplay3ID, FamilyName, RoomID, RoomName);
	_msgin.serial(CountPerDay, YishiTaocanItemID, XianbaoTaocanItemID, YangyuItemID, TianItemID, DiItemID, Param);

	CWorld*	pWorld = GetWorldFromFID(FID);
	if (pWorld == NULL)
	{
		sipinfo("Can't find CWorld !!! FID=%d", FID);
		return;
	}

	TRoomFamilys::iterator	it = pWorld->m_mapFamilys.find(FID);
	if (it == pWorld->m_mapFamilys.end())
	{
		sipwarning("FamilyID can't find in m_mapFamilys !!! FID=%d", FID);
		return;
	}
	INFO_FAMILYINROOM	&FamilyInfo = it->second;

	// ����ġ����
	uint32	prevGMoney = FamilyInfo.m_nGMoney;
	ChangeFamilyExpForAutoplay3(FamilyInfo, RoomID, RoomName, CountPerDay, YishiTaocanItemID, XianbaoTaocanItemID, YangyuItemID, Param, TianItemID, DiItemID);

	// Client�� ����
	if (FamilyInfo.m_bFamilyExpDataChanged)
	{
		FamilyInfo.SaveFamilyExpData(FamilyInfo.m_nFamilyLevelChanged ? true : false);

		SendFamilyProperty_Level(FamilyInfo.m_ServiceID, FamilyInfo.m_FID, FamilyInfo.m_Exp, FamilyInfo.m_Level);
	}
	if (prevGMoney != FamilyInfo.m_nGMoney)
	{
		{
			MAKEQUERY_SaveFamilyGMoney(strQ, FamilyInfo.m_FID, FamilyInfo.m_nGMoney);
			DBCaller->executeWithParam(strQ, NULL,
				"D",
				OUT_,			NULL);
		}

		SendFamilyProperty_Money(FamilyInfo.m_ServiceID, FamilyInfo.m_FID, FamilyInfo.m_nGMoney, FamilyInfo.m_nUMoney);
	}

	// LS�� ����
	{
		CMessage	msgOut(M_NT_AUTOPLAY3_EXP_ADD_OK);
		msgOut.serial(YishiTimeMin, Autoplay3ID);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}
}

static void	DBCB_DBLoadFamilyExpInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32			FID	= *(uint32 *)argv[0];
	uint32			YishiTimeMin	= *(uint32 *)argv[1];
	uint32			Autoplay3ID	= *(uint32 *)argv[2];
	ucstring		FamilyName	= (ucchar *)argv[3];
	uint32			RoomID	= *(uint32 *)argv[4];
	ucstring		RoomName	= (ucchar *)argv[5];
	uint32			CountPerDay	= *(uint32 *)argv[6];
	uint32			YishiTaocanItemID	= *(uint32 *)argv[7];
	uint32			XianbaoTaocanItemID	= *(uint32 *)argv[8];
	uint32			YangyuItemID	= *(uint32 *)argv[9];
	uint32			TianItemID	= *(uint32 *)argv[10];
	uint32			DiItemID	= *(uint32 *)argv[11];
	uint32			Param	= *(uint32 *)argv[12];

	uint32	nRows;

	// tbl_Family
	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("nRows != 1 in tbl_Family. FID=%d", FID);
		return;
	}
	T_F_LEVEL			uLevel;			resSet->serial(uLevel);
	T_ROOMEXP			exp;			resSet->serial(exp);
	uint32				GMoney;			resSet->serial(GMoney);

	// tbl_family_ExpHis
	uint32				exp_his_data_len = 0;
	uint8				exp_his_data[sizeof(_Family_Exp_His) * FAMILY_EXP_TYPE_MAX];
	resSet->serial(nRows);
	if (nRows == 1)
	{
		resSet->serial(exp_his_data_len);
		resSet->serialBuffer(exp_his_data, exp_his_data_len);
	}

	INFO_FAMILYINROOM	FamilyInfo(FID, 0, 0, FamilyName, uLevel, exp, GMoney, L"", 0, 0, TServiceId(0));

	// ExpHisData
	if (exp_his_data_len == sizeof(FamilyInfo.m_FamilyExpHis))
	{
		memcpy(&FamilyInfo.m_FamilyExpHis, exp_his_data, sizeof(FamilyInfo.m_FamilyExpHis));
	}
	else
	{
		if (exp_his_data_len != 0)
			sipwarning("exp_his_data_len != sizeof(pInfo->m_FamilyExpHis). FID=%d, %d, %d", FID, exp_his_data_len, sizeof(FamilyInfo.m_FamilyExpHis));
		memset(&FamilyInfo.m_FamilyExpHis, 0, sizeof(FamilyInfo.m_FamilyExpHis));
	}

	// ����ġ����
	uint32	prevGMoney = FamilyInfo.m_nGMoney;
	ChangeFamilyExpForAutoplay3(FamilyInfo, RoomID, RoomName, CountPerDay, YishiTaocanItemID, XianbaoTaocanItemID, YangyuItemID, Param, TianItemID, DiItemID);

	// �ڷẸ��
	if (FamilyInfo.m_bFamilyExpDataChanged)
	{
		FamilyInfo.SaveFamilyExpData(true);
	}

	if (prevGMoney != FamilyInfo.m_nGMoney)
	{
		MAKEQUERY_SaveFamilyGMoney(strQ, FamilyInfo.m_FID, FamilyInfo.m_nGMoney);
		DBCaller->executeWithParam(strQ, NULL,
			"D",
			OUT_,			NULL);
	}

	// LS�� ����
	{
		CMessage	msgOut(M_NT_AUTOPLAY3_EXP_ADD_OK);
		msgOut.serial(YishiTimeMin, Autoplay3ID);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}
}

// ������翡 ���� ����ġ���� ���� LS->OwnWS->MS->OROOM ([d:FID][d:YishiTimeMin][d:Autoplay3ID][d:RoomID][u:RoomName][d:CountPerDay][d:YishiTaocanItemID][d:XianbaoTaocanItemID][d:YangyuItemID][d:ItemID_Tian][d:ItemID_Di][d:Param])
void cb_M_NT_AUTOPLAY3_EXP_ADD_OFFLINE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32	FID, YishiTimeMin, Autoplay3ID, RoomID, CountPerDay, YishiTaocanItemID, XianbaoTaocanItemID, YangyuItemID, TianItemID, DiItemID, Param;
	ucstring	FamilyName, RoomName;

	_msgin.serial(FID, YishiTimeMin, Autoplay3ID, FamilyName, RoomID, RoomName);
	_msgin.serial(CountPerDay, YishiTaocanItemID, XianbaoTaocanItemID, YangyuItemID, TianItemID, DiItemID, Param);

	MAKEQUERY_LoadFamilyExpInfo(strQ, FID);
	DBCaller->executeWithParam(strQ, DBCB_DBLoadFamilyExpInfo,
		"DDDSDSDDDDDDD",
		CB_,		FID,
		CB_,		YishiTimeMin,
		CB_,		Autoplay3ID,
		CB_,		FamilyName.c_str(),
		CB_,		RoomID,
		CB_,		RoomName.c_str(),
		CB_,		CountPerDay,
		CB_,		YishiTaocanItemID,
		CB_,		XianbaoTaocanItemID,
		CB_,		YangyuItemID,
		CB_,		TianItemID,
		CB_,		DiItemID,
		CB_,		Param);
}

// ======================================================================
uint32		m_CheckDB_CallCount = 0;
T_FAMILYID	m_CheckDB_FID = 0;
T_F_LEVEL	m_CheckDB_Level = 0;
T_F_EXP		m_CheckDB_Exp = 0;
uint32		m_CheckDB_MaxCallCount = 0;

static void	DBCB_DBSaveFamilyLevelAndExpForCheckDB(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	m_CheckDB_CallCount --;
}

void updateDBCheckFunc()
{
	//if (m_CheckDB_FID == 0)
	//	return;

	//if ((m_CheckDB_MaxCallCount > 0) && (m_CheckDB_CallCount > m_CheckDB_MaxCallCount))
	//	return;

	//m_CheckDB_CallCount ++;

	//MAKEQUERY_SaveFamilyLevelAndExp(strQ, m_CheckDB_FID, m_CheckDB_Level, m_CheckDB_Exp);
	//DBCaller->executeWithParam(strQ, DBCB_DBSaveFamilyLevelAndExpForCheckDB,
	//	"D",
	//	OUT_,		NULL);
}

//static void	DBCB_DBGetCharacterForCheckDB(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	if (!nQueryResult)
//	{
//		sipinfo(L"failed.");
//		return;
//	}
//
//	T_FAMILYID			FID			= *(T_FAMILYID *)argv[0];
//
//	uint32 nRows;	resSet->serial(nRows);
//	if (nRows == 0)
//	{
//		sipinfo(L"Not found. FID=%d", FID);
//		return;
//	}
//	for (uint32 i = 0; i < nRows; i ++)
//	{
//		T_CHARACTERID	uid;			resSet->serial(uid);
//		T_COMMON_NAME	name;			resSet->serial(name);
//		T_MODELID		modelid;		resSet->serial(modelid);
//		T_DRESS			ddressid;		resSet->serial(ddressid);
//		T_FACEID		faceid;			resSet->serial(faceid);
//		T_HAIRID		hairid;			resSet->serial(hairid);
//		T_DATETIME		createdate;		resSet->serial(createdate);
//		T_ISMASTER		master;			resSet->serial(master);
//		T_DRESS			dressid;		resSet->serial(dressid);
//		T_DATETIME		changedate;		resSet->serial(changedate);
//		T_CHBIRTHDATE	birthDate;		resSet->serial(birthDate);
//		T_CHCITY		city;			resSet->serial(city);
//		T_COMMON_CONTENT resume;		resSet->serial(resume);
//		T_F_LEVEL		fLevel;			resSet->serial(fLevel);
//		T_F_EXP			fExp;			resSet->serial(fExp);
//
//		m_CheckDB_CallCount = 0;
//		m_CheckDB_FID = FID;
//		m_CheckDB_Level = fLevel;
//		m_CheckDB_Exp = fExp;
//	}
//}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, startDBCheck, "start DB repeat call", "FID MaxCallCount")
{
	if(args.size() < 2)	return false;

	//uint32	FID = atoui(args[0].c_str());
	//m_CheckDB_MaxCallCount = atoui(args[1].c_str());

	//MAKEQUERY_GetFamilyCharacters(strQ, FID);
	//DBCaller->execute(strQ, DBCB_DBGetCharacterForCheckDB,
	//	"D",
	//	CB_,		FID);

	return true;
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, stopDBCheck, "stop DB repeat call", "")
{
	m_CheckDB_FID = 0;

	return true;
}
