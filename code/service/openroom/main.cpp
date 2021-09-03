
#include <misc/types_sip.h>
#include <net/service.h>

#include "openroom.h"
#include "Character.h"
#include "contact.h"
#include "Inven.h"
#include "Family.h"
#include "room.h"
#include "mst_room.h"
#include "mst_item.h"
#include "outRoomKind.h"
#include "../Common/DBLog.h"
#include "../Common/Lang.h"
#include "ParseArgs.h"

#include "misc/DBCaller.h"

//#include <vld.h>

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

CDBCaller		*DBCaller = NULL;
CDBCaller		*MainDBCaller = NULL;

ucstring	OutDataRootPath;

//SIPBASE::CVariable<std::string>	RoomInfoFile("openroom", "RoomInfoFile", "filename of roominfo mst data", "RoomInfo.xml", 0, true);
//SIPBASE::CVariable<std::string>	MiscInfoFile("openroom", "MiscInfoFile", "filename of miscinfo mst data", "MiscInfo.xml", 0, true);
//SIPBASE::CVariable<std::string>	ModelInfoFile("openroom", "ModelInfoFile", "filename of modelinfo mst data", "ModelInfo.xml", 0, true);
SIPBASE::CVariable<std::string>	ServerInfoFile("openroom", "ServerInfoFile", "filename of serverinfo mst data", "ServerInfo.xml", 0, true);

bool	LoadXML(CIXml *xmlInfo, string conf, ucstring OutDataRootPath)
{
	ucstring	InfoFile;
	if (IService::getInstance()->ConfigFile.exists(conf))
		InfoFile = IService::getInstance()->ConfigFile.getVar(conf).asString();
	else
		return false;
	ucstring	FullPath = OutDataRootPath + InfoFile;
	if (!xmlInfo->init(FullPath))
		return false;
	return true;
}

bool	LoadXML(CIXml *xmlInfo, CVariable<std::string> &varConf, ucstring OutDataRootPath)
{
	ucstring	InfoFile;
	if (IService::getInstance()->ConfigFile.exists(varConf.getName()))
		InfoFile = IService::getInstance()->ConfigFile.getVar(varConf.getName()).asString();
	else
		InfoFile = varConf.get();

	ucstring	FullPath = OutDataRootPath + InfoFile;
	if (!xmlInfo->init(FullPath))
		return false;
	return true;
}


bool	LoadOutData()
{
	if (IService::getInstance()->ConfigFile.exists("OutDataRootPath"))
		OutDataRootPath = IService::getInstance()->ConfigFile.getVar("OutDataRootPath").asString();
	else
	{
// #ifdef	_UNICODE
#ifdef	SIP_USE_UNICODE
		OutDataRootPath = CPath::getExePathW();
#else
		OutDataRootPath = CPath::getExePathA();
#endif
		OutDataRootPath += "OutData/";
	}
	sipinfo(L"Out data's root path is '%s'", OutDataRootPath.c_str());

	// Ini file path
	ucstring	ucOutIniFile = IService::getInstance()->getConfigDirectory() + "OutData.ini";

	bool bLoad = true;

	{
		// Pet outdata0
		CIXml	xmlServerInfo;
		if (!LoadXML(&xmlServerInfo, ServerInfoFile, OutDataRootPath))
		{
			siperrornoex("Can't load ModelInfoFile");
			return false;
		}
		bLoad &= LoadAnimalParam(&xmlServerInfo, ucOutIniFile);

		// Pet outdata
		bLoad &= LoadPetData(&xmlServerInfo, ucOutIniFile);
		bLoad &= LoadPetCare(&xmlServerInfo, ucOutIniFile);
		bLoad &= LoadModelSex(&xmlServerInfo, ucOutIniFile);
		bLoad &= LoadTaocanForAutoplay(&xmlServerInfo, ucOutIniFile);

		// Room outdata
		bLoad &= LoadRoomKind(&xmlServerInfo, ucOutIniFile); // must be here first.
		bLoad &= LoadRoomScope(&xmlServerInfo, ucOutIniFile); // must be here second.
//		bLoad &= LoadRoomPetMax(&xmlServerInfo, ucOutIniFile);
		bLoad &= LoadRoomDefaultPet(&xmlServerInfo, ucOutIniFile);
		//bLoad &= LoadRoomGate(&xmlRoomInfo, ucOutIniFile);
		//bLoad &= LoadServerScope(&xmlRoomInfo, ucOutIniFile);
		bLoad &= LoadReligionRoom(&xmlServerInfo, ucOutIniFile);

		// Luck outdata
		bLoad &= LoadLuckData(&xmlServerInfo, ucOutIniFile);
		bLoad &= LoadPrizeData(&xmlServerInfo, ucOutIniFile);
		bLoad &= LoadPointCardData(&xmlServerInfo, ucOutIniFile);
		bLoad &= LoadFishYuWangTeXiao(&xmlServerInfo, ucOutIniFile);
	}

	// Lang data
	{
		if (!LoadLangData(CPath::getExePathW()))
		{
			siperrornoex("LoadLangData failed.");
			return false;
		}
	}

	if (!bLoad)
	{
		siperrornoex("Failed load Outdata");
		return false;
	}
	return true;
}

extern	void run_lobby_service(LPSTR lpCmdLine);
extern	void run_lobby_service(int argc, const char **argv);
extern	void run_openroom_service(LPSTR lpCmdLine);
extern	void run_openroom_service(int argc, const char **argv);
extern	void run_commonroom_service(LPSTR lpCmdLine);
extern	void run_commonroom_service(int argc, const char **argv);

enum TGameServiceType	{
	GAMESERVICE_OPENROOM,
	GAMESERVICE_LOBBY,
	GAMESERVICE_RELIGIONROOM,
};

#ifdef SIP_OS_WINDOWS
	int APIENTRY WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
#else
	int main(int argc, const char **argv)
#endif
{
	SIPBASE::CApplicationContext	serviceContext;

	// parse config file
	SIPBASE::CConfigFile	config;

	// parse command line
	TGameServiceType gtype = GAMESERVICE_OPENROOM;
#ifdef SIP_OS_WINDOWS
	CParseArgs::setArgs(lpCmdLine);
#else
	CParseArgs::setArgs(argc, argv);
#endif

	if ( CParseArgs::haveArg('f') )
	{
		std::string func = CParseArgs::getArg('f');
		if ( func == "lobby" )
			gtype = GAMESERVICE_LOBBY;
	}

	if ( CParseArgs::haveLongArg("func") )
	{
		std::string func = CParseArgs::getLongArg("func");
		if ( func == "lobby" )
			gtype = GAMESERVICE_LOBBY;
	}

	if ( CParseArgs::haveArg('r') )
	{
		gtype = GAMESERVICE_RELIGIONROOM;
	}

	if ( CParseArgs::haveLongArg("rrooms") )
	{
		gtype = GAMESERVICE_RELIGIONROOM;
	}

	switch ( gtype )
	{
	case GAMESERVICE_LOBBY:
#ifdef SIP_OS_WINDOWS
		run_lobby_service(lpCmdLine);
#else
		run_lobby_service(argc, argv);
#endif
		break;
	case GAMESERVICE_OPENROOM:
#ifdef SIP_OS_WINDOWS
		run_openroom_service(lpCmdLine);
#else
		run_openroom_service(argc, argv);
#endif
		break;
	case GAMESERVICE_RELIGIONROOM:
#ifdef SIP_OS_WINDOWS
		run_commonroom_service(lpCmdLine);
#else
		run_commonroom_service(argc, argv);
#endif
		break;
	default:
		sipwarning("Invalid Service Type!!!");
		break;
	}

	return 0;
}

SIPBASE_CATEGORISED_DYNVARIABLE(OROOM_S, uint32, CurrentDBRequestNumber, "number of current DB request")
{
	if (get)
	{
		*pointer = DBCaller->GetCurrentDBRequestNumber();
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(OROOM_S, uint64, MisProcedDBRequestNumber, "number of current DB request")
{
	if (get)
	{
		*pointer = DBCaller->GetMisProcedDBRequestNumber();
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(OROOM_S, uint32, CurrentDBRProcSpeedPerOne, "speed of DB processing")
{
	if (get)
	{
		uint64		tn = DBCaller->GetProcedDBRequestNumber();
		uint64		tt = DBCaller->GetProcedDBRequestTime();
		if (tn > 0)
			*pointer = (uint32)(tt/tn);
		else
			*pointer = 0;
	}
}

//SIPNET_SERVICE_MAIN (COpenRoomService, OROOM_SHORT_NAME, OROOM_LONG_NAME, "", 0, false, CallbackArray, "", "")

/****************************************************************************
* Commands
****************************************************************************/

// System notice
SIPBASE_CATEGORISED_COMMAND(OROOM_S, getServerTime, "Show server Time", "")
{
	uint64 a64;
	a64 = ServerStartTime_DB.timevalue;
	log.displayNL("ServerStartTime_DB=%d:%d", (int)(a64>>32),(int)a64);

	a64 = ServerStartTime_Local;
	log.displayNL("ServerStartTime_Local=%d:%d", (int)(a64>>32),(int)a64);

	a64 = CTime::getLocalTime();
	log.displayNL("getLocalTime=%d:%d", (int)(a64>>32),(int)a64);

	return true;
}



SIPBASE_CATEGORISED_COMMAND(sip, test_ChangePointItemType, "Change point items to normal items", "")
{
	CDBConnectionRest	DB(DBCaller);
	uint32	FID;
	SQLLEN	InvenBufSize;
	uint32	len;
	uint8	InvenBuf[MAX_INVENBUF_SIZE];
	ucchar	st_str_buf[MAX_INVENBUF_SIZE * 2 + 10];

	while (true)
	{
		{
			MAKEQUERY_Test_GetFamilyInven(strQuery);
			DB_EXE_QUERY(DB, Stmt, strQuery);
			if (nQueryResult != true)
				break;

			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (!IS_DB_ROW_VALID(row))
				break;

			COLUMN_DIGIT(row, 0, uint32, fid);
			row.GetData(1 + 1, InvenBuf, sizeof(InvenBuf), &InvenBufSize, SQL_C_BINARY);

			FID = fid;
			len = (uint32)InvenBufSize;
		}

		_InvenItems	inven;
		if (!inven.fromBuffer(InvenBuf, len))
		{
			sipwarning("failed fromBuffer inven items! FID=%d", FID);
		}

		bool changed = false;
		for (uint16 ItemIndex = 0; ItemIndex < MAX_INVEN_NUM; ItemIndex++)
		{
			if (inven.Items[ItemIndex].ItemID == 0)
				continue;
			if (inven.Items[ItemIndex].MoneyType != MONEYTYPE_UMONEY)
			{
				inven.Items[ItemIndex].MoneyType = MONEYTYPE_UMONEY;
				changed = true;
			}
		}

		while (changed)
		{
			len = sizeof(InvenBuf);
			if (!inven.toBuffer(InvenBuf, &len))
			{
				sipwarning("Can't toBuffer inven items! FID=%d", FID);
			}
			else
			{
				for (uint32 i = 0; i < len; i++)
				{
					smprintf((ucchar*)(&st_str_buf[i * 2]), 2, _t("%02x"), InvenBuf[i]);
				}
				st_str_buf[InvenBufSize * 2] = 0;

				{
					MAKEQUERY_SaveFamilyInven(strQ, FID, st_str_buf);
					DB_EXE_PREPARESTMT(DB, Stmt, strQ);
					if (!nPrepareRet)
						break;

					SQLLEN	num2 = 0;
					int	ret = 0;
					DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num2);
					if (!nBindResult)
						break;

					DB_EXE_PARAMQUERY(Stmt, strQ);
					if (!nQueryResult || ret == -1)
						break;
				}
			}
			break;
		}

		{
			MAKEQUERY_Test_SetFamilyInvenChecked(strQ, FID);
			DB_EXE_QUERY(DB, Stmt, strQ);
			if (nQueryResult != true)
				break;
		}
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(sip, test_GivePointsToOldRoom, "Give points to old room's owners", "")
{
	CDBConnectionRest	DB(DBCaller);

	while (true)
	{
		uint32	RoomID, FID, GMoney;
		uint16	SceneID;
		ucstring RoomName;

		{
			MAKEQUERY_Test_GetRoom(strQuery);
			DB_EXE_QUERY(DB, Stmt, strQuery);
			if (nQueryResult != true)
				break;

			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (!IS_DB_ROW_VALID(row))
				break;

			COLUMN_DIGIT(row, 0, uint32, roomid);
			COLUMN_WSTR(row, 1, ucName, MAX_LEN_ROOM_NAME);
			COLUMN_DIGIT(row, 2, uint16, sceneid);
			COLUMN_DIGIT(row, 3, uint32, fid);
			COLUMN_DIGIT(row, 4, uint32, gmoney);

			RoomID = roomid;
			RoomName = ucName;
			SceneID = sceneid;
			FID = fid;
			GMoney = gmoney;
		}

		MST_ROOM	*room = GetMstRoomData_FromSceneID(SceneID, 0, 0, 0);
		if (room == NULL)
		{
			sipwarning("GetMstRoomData = NULL. RoomID=%d, SceneID=%d", RoomID, SceneID);
			break;
		}
		T_PRICE	UPrice = room->GetRealPrice();

		if (RoomName.empty())
			RoomName = room->m_Name;

		//DBLOG_CHFamilyGMoney(FID, Name, GMoney, GMoney + UPrice);

		{
			CMessage	msgOut(M_NT_TEMP_MAIL_ADDPOINT_TO_OLDROOM);
			msgOut.serial(FID, RoomID, RoomName, UPrice);
			CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
		}

		{
			MAKEQUERY_Test_SetRoomChecked(strQ, RoomID, FID, GMoney + UPrice);
			DB_EXE_QUERY(DB, Stmt, strQ);
			if (nQueryResult != true)
				break;
		}
	}

	return true;
}
