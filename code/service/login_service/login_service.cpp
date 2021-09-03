#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef SIPNS_CONFIG
#define SIPNS_CONFIG ""
#endif // SIPNS_CONFIG

#ifndef SIPNS_LOGS
#define SIPNS_LOGS ""
#endif // SIPNS_LOGS

#ifndef SIPNS_STATE
#define SIPNS_STATE ""
#endif // SIPNS_STATE

#include "misc/types_sip.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <vector>
#include <map>

#include "misc/debug.h"
#include "misc/config_file.h"
#include "misc/displayer.h"
#include "misc/command.h"
#include "misc/log.h"
#include "misc/window_displayer.h"
#include "misc/path.h"
#include "misc/md5.h"

#include "net/service.h"
#include "net/login_cookie.h"

#include "login_service.h"
//#include "connection_client.h"

#include "../../common/common.h"
#include "../../common/err.h"
#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/Lang.h"
#include "../Common/netCommon.h"
#include "../Common/DBData.h"
#include "../Common/QuerySetForMain.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

#include "connection_ws.h"
#include "connection_ss.h"
#include "players.h"

//
// Variables
//

// store specific user information
SIPBASE::CFileDisplayer *Fd = NULL;
SIPBASE::CStdDisplayer Sd;
SIPBASE::CLog *Output = NULL;

//uint32 CUser::NextUserId = 1;	// 0 is reserved

// Variables

//vector<CUser>	Users;
//vector<CShard>	Shards;

IService *ServiceInstance = NULL;

string DatabaseName, DatabaseHost, DatabaseLogin, DatabasePassword;

CDBCaller		*MainDB = NULL;
//DBCONNECTION	MainDBConnection;
CDBCaller		*UserDB = NULL;

//TSHARDSTATETBL		tblShardState;
//TSHARDVIPS			tblShardVIPs;
//TSHARDLOBBY tblShardLobby;
//TSHARDTBL g_tblShardList;
//vector<CShard> g_Shards;
std::map<uint32, CShardInfo>	g_ShardList;
std::map<uint8, uint32>			g_mapShardCodeToShardID;	// <ShardCode, ShardID>

map<uint32, CShardCookie> TempCookies;

map<uint32, uint32> AbnormalLogoffUsers;
//map<uint32, uint16>  ShardIdVSSId;


// This Variable means that this game site is now running.
// by this Flag, it notes wheather if a new shard is dynamically connected to admin_login site!
CVariable<uint8>	ServerBuildComplete(
								   "LS",
								   "ServerBuildComplete",
								   "Indicates if All shards and manage Site is open (0 all is closed, 1 manage site and all shards is opened, 2 only all shards is opened)",
								   0,		// default value
								   0,		// nbmean value
								   false	// not use config file
					);

SIPBASE::CVariable<uint>		AcceptExternalShards("LS", "AcceptExternalShards", "Flag whether ls accept external shard", 0, 0, true);
SIPBASE::CVariable<std::string>	ForceDatabaseReconnection("LS", "ForceDatabaseReconnection", "Flag whether force reconnect to database", "", 0, true);


std::map<uint32, uint32> g_mapUIDtoFID;
std::map<uint32, uint32> g_mapFIDtoUID;

//
// Functions
//


// transform "192.168.1.1:80" into "192.168.1.1"
string removePort (const string &addr)
{
	return addr.substr (0, addr.find (":"));
}

void displayShards ()
{
	ICommand::execute ("shards", *InfoLog);
}

void displayUsers ()
{
	ICommand::execute ("users", *InfoLog);
}

void	SendShardListToSS(uint32 userid, uint32 fid, uint16 sid)
{
	// 친구목록을 먼저 보낸다.
	SendFriendListToSS(userid, fid, sid);

	SendYuyueListToSS(userid, fid, sid);

	AddUIDtoFIDmap(userid, fid);

	uint32	mainShardId = getMainShardIDfromFID(fid);

	CMessage	msgout(M_SC_SHARDS);
	msgout.serial(userid, mainShardId);

	uint32		uShards = g_ShardList.size();
	msgout.serial(uShards);
	for (std::map<uint32, CShardInfo>::iterator it = g_ShardList.begin(); it != g_ShardList.end(); it ++)
	{
		msgout.serial(it->second.m_nShardId);
		msgout.serial(it->second.m_uShardName);
		msgout.serial(it->second.m_bActiveFlag);
		msgout.serial(it->second.m_bStateId);
		msgout.serial(it->second.m_uShardCodeList);
		msgout.serial(it->second.m_sActiveShardIP);
		msgout.serial(it->second.m_bIsDefaultShard);
	}

	if (sid == 0)
		CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgout);
	else
		CUnifiedNetwork::getInstance()->send(TServiceId(sid), msgout);

	ResetSomeVariablesWhenLogin(userid);
}

uint32 getMainShardIDfromFID(uint32 FID)
{
	if (FID == 0)
		return 0;

	uint8 code = (uint8)(((FID >> 24) & 0x000000FF) + 1);

	std::map<uint8, uint32>::iterator	it = g_mapShardCodeToShardID.find(code);
	if (it != g_mapShardCodeToShardID.end())
		return it->second;

	sipwarning("getMainShardIDfromFID failed. FID=%d", FID);
	return 0;
}

uint32 getMainShardIDfromUID(uint32 UID)
{
	uint32	FID = FindFIDfromUID(UID);
	if (FID == 0)
		return 0;

	uint8 code = (uint8)(((FID >> 24) & 0x000000FF) + 1);

	std::map<uint8, uint32>::iterator	it = g_mapShardCodeToShardID.find(code);
	if (it != g_mapShardCodeToShardID.end())
		return it->second;

	sipwarning("getMainShardIDfromUID failed. UID=%d, FID=%d", UID, FID);
	return 0;
}

void AddUIDtoFIDmap(uint32 userid, uint32 fid)
{
	{
		std::map<uint32, uint32>::iterator	it = g_mapUIDtoFID.find(userid);
		if (it == g_mapUIDtoFID.end())
		{
			g_mapUIDtoFID[userid] = fid;
		}
		else if (it->second != fid)
		{
			if (it->second != 0)
			{
				sipwarning("Invalid UID-FID map! UID=%d, OldFID=%d, NewFID=%d", userid, it->second, fid);
				return;
			}
			g_mapUIDtoFID[userid] = fid;
		}
	}

	if (fid != 0)
	{
		std::map<uint32, uint32>::iterator	it = g_mapFIDtoUID.find(fid);
		if (it == g_mapFIDtoUID.end())
		{
			g_mapFIDtoUID[fid] = userid;
		}
		else if (it->second != userid)
		{
			if (it->second != 0)
			{
				sipwarning("Invalid FID-UID map! FID=%d, OldUID=%d, NewUID=%d", fid, it->second, userid);
				return;
			}
			g_mapFIDtoUID[fid] = userid;
		}
	}
}

uint32 FindFIDfromUID(uint32 userid)
{
	std::map<uint32, uint32>::iterator	it = g_mapUIDtoFID.find(userid);
	if (it == g_mapUIDtoFID.end())
	{
		sipwarning("find g_mapUIDtoFID failed. UserID=%d", userid);
		return 0;
	}
	return it->second;
}

uint32 FindUIDfromFID(uint32 fid)
{
	std::map<uint32, uint32>::iterator	it = g_mapFIDtoUID.find(fid);
	if (it == g_mapFIDtoUID.end())
	{
		sipinfo("find g_mapFIDtoUID failed. FID=%d", fid);
		return 0;
	}
	return it->second;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
/////////////// SERVICE IMPLEMENTATION /////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

void beep (uint freq, uint nb, uint beepDuration, uint pauseDuration)
{
return;
#ifdef SIP_OS_WINDOWS
	if (ServiceInstance == NULL)
		return;

	try
	{
		if (ServiceInstance->ConfigFile.getVar ("Beep").asInt() == 1)
		{
			for (uint i = 0; i < nb; i++)
			{
				Beep (freq, beepDuration);
				sipSleep (pauseDuration);
			}
		}
	}
	catch (Exception &)
	{
	}
#endif // SIP_OS_WINDOWS
}

struct	TTEMPUSERINFO
{
	TTEMPUSERINFO(uint32 _uid, const char *un, const char *p1, const char *p2)
	{
		Userid = _uid;
		strcpy(UserName, un);
		strcpy(UPassword, p1);
		strcpy(TwoPassword, p2);
	}
	uint32	Userid;
	char	UserName[MAX_LEN_USER_ID+1];
	char	UPassword[MAX_LEN_USER_PASSWORD+1];
	char	TwoPassword[MAX_LEN_USER_PASSWORD+1];
};

struct	TTEMPUSERMONEYINFO
{
	TTEMPUSERMONEYINFO(uint32 _uid, string &_un)
	{
		Userid		= _uid;
		sUserName	= _un;
	}
	void	SetMoney(uint32 _um, string &_sm)
	{
		uMoney = _um;
		sMoney = _sm;
	}
	uint32	Userid;
	string	sUserName;
	uint32	uMoney;
	string	sMoney;
};

bool	ConvertAllPasswordToMD5(uint32 uStartID, uint32 uEndID)
{
//	if uStartID == 0 and uEndID == 0, convert all user

	if (uEndID < uStartID)
		return false;
	if (uStartID > 7000 || uEndID > 7000)
		return false;

	std::list<TTEMPUSERINFO>	AllUser;

	{
		tchar strQ[512] = _t("");
		if (uStartID == 0 && uEndID == 0)
			smprintf(strQ, 512, _t("select Userid, UPassword, TwoPassword from tbl_user"));
		else
			smprintf(strQ, 512, _t("select Userid, UPassword, TwoPassword from tbl_user where Userid >= %d and Userid <= %d"), uStartID, uEndID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQ);
		if (nQueryResult != true)
			return false;

		do
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( !IS_DB_ROW_VALID(row) )
				break;

			COLUMN_DIGIT(row, 0, uint32, userid);
			COLUMN_STR(row, 1, upassword, 32);
			COLUMN_STR(row, 2, twopassword, 32);

			CHashKeyMD5 MD51 = getMD5((const uint8 *)(upassword.c_str()), upassword.size());
			string	md5pass1 = MD51.toString();

			CHashKeyMD5 MD52 = getMD5((const uint8 *)(twopassword.c_str()), twopassword.size());
			string	md5pass2 = MD52.toString();

			AllUser.push_back(TTEMPUSERINFO(userid, "temp", md5pass1.c_str(), md5pass2.c_str()));
		}while ( true );

	}

	{
		std::list<TTEMPUSERINFO>::iterator	it;
		for (it = AllUser.begin(); it != AllUser.end(); it++)
		{
			tchar strQ2[512] = _t("");
			ucstring up = string(it->UPassword);
			ucstring tp = string(it->TwoPassword);
			smprintf(strQ2, 512, _t("update tbl_user set UPassword='%s', TwoPassword='%s' where Userid=%d"), up.c_str(), tp.c_str(), it->Userid);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_QUERY(DB, Stmt, strQ2);
		}
	}
	return true;
}

bool	ConvertAllUserIDToLower(uint32 uStartID, uint32 uEndID)
{
//	if uStartID == 0 and uEndID == 0, convert all user

	if (uEndID < uStartID)
		return false;

	std::list<TTEMPUSERINFO>	AllUser;

	{
		tchar strQ[512] = _t("");
		if (uStartID == 0 && uEndID == 0)
			smprintf(strQ, 512, _t("select Userid, UserName from tbl_user"));
		else
			smprintf(strQ, 512, _t("select Userid, UserName from tbl_user where Userid >= %d and Userid <= %d"), uStartID, uEndID);

		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQ);
		if (nQueryResult != true)
			return false;

		do
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( !IS_DB_ROW_VALID(row) )
				break;

			COLUMN_DIGIT(row, 0, uint32, userid);
			COLUMN_STR(row, 1, uname, MAX_LEN_USER_ID);

			// _strlwr(tempuname);
			std::string strUname(tempuname);
			std::transform(strUname.begin(), strUname.end(), strUname.begin(), ::tolower);
			AllUser.push_back(TTEMPUSERINFO(userid, strUname.c_str(), "temp", "temp"));
		}while ( true );

	}

	{
		std::list<TTEMPUSERINFO>::iterator	it;
		for (it = AllUser.begin(); it != AllUser.end(); it++)
		{
			tchar strQ2[512] = _t("");
			ucstring un = string(it->UserName);
			smprintf(strQ2, 512, _t("update tbl_user set UserName='%s' where Userid=%d"), un.c_str(), it->Userid);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_QUERY(DB, Stmt, strQ2);
		}
	}
	return true;
}

bool	convertAllUserMoneyInfo(uint32 uStartID, uint32 uEndID)
{
//	if uStartID == 0 and uEndID == 0, convert all user

	if (uEndID < uStartID)
		return false;

	std::list<TTEMPUSERMONEYINFO>	AllUser;

	{
		tchar strQ[512] = _t("");
		if (uStartID == 0 && uEndID == 0)
			smprintf(strQ, 512, _t("select Userid, UserName from tbl_user"));
		else
			smprintf(strQ, 512, _t("select Userid, UserName from tbl_user where Userid >= %d and Userid <= %d"), uStartID, uEndID);

		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQ);
		if (nQueryResult != true)
			return false;

		do
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( !IS_DB_ROW_VALID(row) )
				break;

			COLUMN_DIGIT(row, 0, uint32, userid);
			COLUMN_STR(row, 1, uname, MAX_LEN_USER_ID);

			// _strlwr(tempuname);
			std::string strUname(tempuname);
			std::transform(strUname.begin(), strUname.end(), strUname.begin(), ::tolower);
			AllUser.push_back(TTEMPUSERMONEYINFO(userid, strUname));
		}while ( true );
	}

	std::list<TTEMPUSERMONEYINFO>::iterator	it;
	for (it = AllUser.begin(); it != AllUser.end(); it++)
	{
		uint32 uUserID = it->Userid;
		string sUserName = it->sUserName;
		tchar strQ[512] = _t("");
		smprintf(strQ, 512, _t("select Money from tbl_userbaseinfo where userid = %d"), uUserID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQ);
		if (nQueryResult != true)
			continue;
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
			continue;
		COLUMN_DIGIT(row, 0, uint32, umoney);
		string sMoney = GetGoldEncodeStringByAccount(sUserName, umoney);
		it->SetMoney(umoney, sMoney);
	}

	{
		std::list<TTEMPUSERMONEYINFO>::iterator	it;
		for (it = AllUser.begin(); it != AllUser.end(); it++)
		{
			ucstring sm = string(it->sMoney);
			tchar strQ2[512] = _t("");
			smprintf(strQ2, 512, _t("update tbl_userbaseinfo set sMoney='%s' where Userid=%d"), sm.c_str(), it->Userid);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_QUERY(DB, Stmt, strQ2);
		}
	}
	return true;
}


bool	setSystemUserMoney(uint32 uStartID, uint32 uEndID, uint32 uGiveMoney)
{
//	if uStartID == 0 and uEndID == 0, convert all user

	if (uEndID < uStartID)
		return false;
	if (uStartID > 7000 || uEndID > 7000)
		return false;

	std::list<TTEMPUSERMONEYINFO>	AllUser;

	{
		tchar strQ[512] = _t("");
		if (uStartID == 0 && uEndID == 0)
			smprintf(strQ, 512, _t("select Userid, UserName from tbl_user"));
		else
			smprintf(strQ, 512, _t("select Userid, UserName from tbl_user where Userid >= %d and Userid <= %d"), uStartID, uEndID);

		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQ);
		if (nQueryResult != true)
			return false;

		do
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( !IS_DB_ROW_VALID(row) )
				break;

			COLUMN_DIGIT(row, 0, uint32, userid);
			COLUMN_STR(row, 1, uname, MAX_LEN_USER_ID);

			// _strlwr(tempuname);
			std::string strUname(tempuname);
			std::transform(strUname.begin(), strUname.end(), strUname.begin(), ::tolower);
			AllUser.push_back(TTEMPUSERMONEYINFO(userid, strUname));
		}while ( true );
	}

	{
		std::list<TTEMPUSERMONEYINFO>::iterator	it;
		for (it = AllUser.begin(); it != AllUser.end(); it++)
		{
			string	sGiveMoney = GetGoldEncodeStringByAccount(it->sUserName, uGiveMoney);
			
			ucstring sm = string(sGiveMoney);
			tchar strQ2[512] = _t("");
			smprintf(strQ2, 512, _t("update tbl_userbaseinfo set Money = %d, sMoney='%s' where Userid=%d"), uGiveMoney, sm.c_str(), it->Userid);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_QUERY(DB, Stmt, strQ2);
		}
	}
	return true;
}

SIPBASE::MSSQLTIME		ServerStartTime_DB;
SIPBASE::TTime			ServerStartTime_Local;
void InitTime()
{
	// Get current time from server
	T_DATETIME		StartTime_DB = "";
	{
		tchar strQ[300] = _t("");
		smprintf(strQ, 300, _t("SELECT GETDATE();"));
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQ);

		if (nQueryResult == true)
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (IS_DB_ROW_VALID(row))
			{
				COLUMN_DATETIME(row, 0, curtime);
				StartTime_DB = curtime;			
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
}

TTime GetDBTimeSecond()
{
	TTime	workingtime = (CTime::getSecondsSince1970() - ServerStartTime_Local);
	TTime	curtime = ServerStartTime_DB.timevalue + workingtime;

	return curtime;
}

extern void LoadSiteActivity();
extern void LoadEverydayCheckIn();

void	DBInit()
{
	string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Main_DBName").asString ();
	string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Main_DBHost").asString ();
	string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Main_DBLogin").asString ();
	string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Main_DBPassword").asString ();

//	BOOL bSuccess = MainDBConnection.Connect(	ucstring(strDatabaseLogin).c_str(),
//									ucstring(strDatabasePassword).c_str(),
//									ucstring(strDatabaseHost).c_str(),
//									ucstring(strDatabaseName).c_str());

	MainDB = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
		ucstring(strDatabasePassword).c_str(),
		ucstring(strDatabaseHost).c_str(),
		ucstring(strDatabaseName).c_str());

	BOOL bSuccess = MainDB->init(4);
	if (!bSuccess)
	{
		siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
			strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
		return;
	}
	sipinfo("Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());

	// Connect UserDB
	if (IService::getInstance ()->ConfigFile.exists ("User_DBName"))
		strDatabaseName = IService::getInstance ()->ConfigFile.getVar("User_DBName").asString ();
	if (IService::getInstance ()->ConfigFile.exists ("User_DBHost"))
		strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("User_DBHost").asString ();
	if (IService::getInstance ()->ConfigFile.exists ("User_DBLogin"))
		strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("User_DBLogin").asString ();
	if (IService::getInstance ()->ConfigFile.exists ("User_DBPassword"))
		strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("User_DBPassword").asString ();

	UserDB = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
		ucstring(strDatabasePassword).c_str(),
		ucstring(strDatabaseHost).c_str(),
		ucstring(strDatabaseName).c_str());
	if ( !UserDB )
	{
		siperrornoex("Init Failed UserDBCaller Memory Allocation Error");
		return;
	}
	bSuccess = UserDB->init(2);

	if (!bSuccess)
	{
		sipwarning("User Database connection failed to '%s' with login '%s' and database name '%s'",
			strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
	}
	else
		sipinfo("User Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());

	InitTime();

	//LoadDBSharedState();
	LoadDBShardList();
	//LoadDBSharedVIPs();

	// 현재 진행중인 기념일활동자료를 읽는다.
	LoadSiteActivity();
	LoadEverydayCheckIn();

	extern void LoadBeginnerMstItem();
	LoadBeginnerMstItem();
}

void cbDatabaseVar(CConfigFile::CVar &var)
{
	DBInit();
}

void cleanDB()
{
	sipinfo("Cleaning DB");
	MAKEQUERY_CLEANDB(strQuery);
	MainDB->execute(strQuery);
	//DB_EXE_QUERY_WITHONEROW(MainDBConnection, Stmt, strQuery)
}

void	DBRelease()
{
	MainDB->release();
	delete MainDB;
	MainDB = NULL;

	//MainDBConnection.Disconnect();

	UserDB->release();
	delete UserDB;
	UserDB = NULL;
}

class CLoginService : public IService
{
	bool Init;

public:

	CLoginService () : Init(false) {};

	/// Init the service, load the universal time.
	void init ()
	{
		ServiceInstance = this;

		beep ();

		Output = new CLog;
		string fn = IService::getInstance()->SaveFilesDirectory;
		fn += "login_service.stat";
		sipinfo("Login stat in directory '%s'", fn.c_str());
		Fd = new SIPBASE::CFileDisplayer(fn);
		Output->addDisplayer (Fd);
		if (WindowDisplayer != NULL)
			Output->addDisplayer (WindowDisplayer);

		// Initialize the database access
		ConfigFile.setCallback ("ForceDatabaseReconnection", cbDatabaseVar);

		CConfigFile::CVar	varNull;
		varNull.forceAsString("");
		if ( ConfigFile.exists("ForceDatabaseReconnection") )
			cbDatabaseVar (ConfigFile.getVar ("ForceDatabaseReconnection"));
		else
			cbDatabaseVar (varNull);

		cleanDB();

		connectionWSInit ();
		connectionSSInit ();

		Init = true;
		// Lang data
		{
			if (!LoadLangData(CPath::getExePathW()))
			{
				siperrornoex("LoadLangData failed.");
				return;
			}
		}

		// Timeout된 메일들을 검사하여 돌려보낸다.
		{
			MAKEQUERY_AutoReturnMail(strQ);
			MainDB->execute(strQ);
		}

		Output->displayNL ("Login Service initialised");
	}

	bool update ()
	{
		connectionWSUpdate ();
		connectionSSUpdate ();
		
		MainDB->update();
		UserDB->update();

		return true;
	}

	/// release the service, save the universal time
	void release ()
	{
		if (Init)
		{
			//writePlayerDatabase ();
		}

		connectionWSRelease ();
		connectionSSRelease ();
		
		DBRelease();

		Output->displayNL ("Login Service released");
	}
};

// Service instanciation
SIPNET_SERVICE_MAIN (CLoginService, LS_S_NAME, LS_L_NAME, "LSPort", 49993, false, EmptyCallbackArray, SIPNS_CONFIG, SIPNS_LOGS);


//
// Variables
//

//
// Commands
//

//SIPBASE_CATEGORISED_COMMAND(LS, shards, "displays the list of all registered shards", "")
//{
//	if(args.size() != 0) return false;
//
//	log.displayNL ("Display the %d registered shards :", g_Shards.size());
//	for (uint i = 0; i < g_Shards.size(); i++)
//	{
//		log.displayNL ("* ShardId: %d SId: %d NbPlayers: %d", g_Shards[i].dbInfo.shardId, g_Shards[i].SId, g_Shards[i].dbInfo.nbPlayers);
//		CUnifiedNetwork::getInstance()->displayUnifiedConnection (TServiceId(g_Shards[i].SId), &log);
//	}
//	log.displayNL ("End of the list");
//
//	return true;
//}

/* start
SIPBASE_CATEGORISED_COMMAND(LS, shards, "displays the list of all registered shards", "")
{
	if(args.size() != 0) return false;

	log.displayNL ("Display registered shards :");
	for (std::map<uint32, CShardInfo>::iterator it = g_ShardList.begin(); it != g_ShardList.end(); it ++)
	{
		if (it->second.m_wSId != 0)
		{
			log.displayNL ("* ShardId: %d SId: %d NbPlayers: %d", it->second.m_nShardId, it->second.m_wSId, it->second.m_nPlayers);
			CUnifiedNetwork::getInstance()->displayUnifiedConnection (TServiceId(it->second.m_wSId), &log);
		}
	}
	log.displayNL ("End of the list");

	return true;
}

SIPBASE_CATEGORISED_COMMAND(LS, convertAllPasswordToMD5, "convert all password to md5", "startID | end ID")
{
	uint32 uStartID = 0, uEndID = 0;
	if(args.size() == 2)
	{
		string sStartID = args[0];
		uStartID		= atoui(sStartID.c_str());
		
		string sEndID	= args[1];
		uEndID			= atoui(sEndID.c_str());
	}
	else
		return false;
	bool bResult = ConvertAllPasswordToMD5(uStartID, uEndID);
	if (bResult)
		log.displayNL ("Converting is OK!");
	else
		log.displayNL ("Converting is failed.");

	return true;
}
end */

/*
SIPBASE_CATEGORISED_COMMAND(LS, convertAllUserIDToLower, "convert all user id to lower", "startID | end ID")
{
	uint32 uStartID = 0, uEndID = 0;
	if(args.size() == 2)
	{
		string sStartID = args[0];
		uStartID		= atoui(sStartID.c_str());

		string sEndID	= args[1];
		uEndID			= atoui(sEndID.c_str());
	}
	bool bResult = ConvertAllUserIDToLower(uStartID, uEndID);
	if (bResult)
		log.displayNL ("Converting is OK!");
	else
		log.displayNL ("Converting is failed.");

	return true;
}
*/

/*
SIPBASE_CATEGORISED_COMMAND(LS, convertAllUserMoneyInfo, "convert money info to string money", "startID | end ID")
{

	uint32 uStartID = 0, uEndID = 0;
	if(args.size() == 2)
	{
		string sStartID = args[0];
		uStartID		= atoui(sStartID.c_str());

		string sEndID	= args[1];
		uEndID			= atoui(sEndID.c_str());
	}
	else
		return false;
	bool bResult = convertAllUserMoneyInfo(uStartID, uEndID);
	if (bResult)
		log.displayNL ("Converting is OK!");
	else
		log.displayNL ("Converting is failed.");
	return true;
}
*/

/* start
SIPBASE_CATEGORISED_COMMAND(LS, setSystemUserMoney, "set money to system user(1~7000)", "startID | end ID | money")
{
	uint32 uStartID = 0, uEndID = 0, uMoney = 1000000;
	if(args.size() == 3)
	{
		string sStartID = args[0];
		uStartID		= atoui(sStartID.c_str());

		string sEndID	= args[1];
		uEndID			= atoui(sEndID.c_str());

		string sMoney	= args[2];
		uMoney			= atoui(sMoney.c_str());
	}
	else
		return false;

	bool bResult = setSystemUserMoney(uStartID, uEndID, uMoney);
	if (bResult)
		log.displayNL ("Converting is OK!");
	else
		log.displayNL ("Converting is failed.");

	return true;
}

SIPBASE_CATEGORISED_COMMAND(LS, reloadActivity, "Reload current activity", "")
{
	LoadSiteActivity();
	log.displayNL ("Reload activity OK!");
	return true;
}

SIPBASE_CATEGORISED_COMMAND(LS, reloadCheckIn, "Reload CheckIn activity", "")
{
	LoadEverydayCheckIn();
	log.displayNL ("Reload CheckIn OK!");
	return true;
}

SIPBASE_CATEGORISED_COMMAND(LS, reloadYuyueDatas, "Reload YuyueDatas", "")
{
	LoadAllYuyueDatas();
	log.displayNL ("Reload YuyueData OK!");
	return true;
}
end */
