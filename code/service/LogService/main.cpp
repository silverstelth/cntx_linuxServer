
#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>
#include <misc/stream.h>
#include "misc/DBCaller.h"

#include <map>
#include <utility>

#include "../../common/Packet.h"
#include "../Common/Common.h"
#include "../Common/QuerySetForShard.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

CDBCaller		*DBCaller = NULL;

void	cb_M_CS_SHARDLOG(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32				MainType, SubType;
	uint32				PID;
	uint32				UID;
	uint32				ItemID;
	uint32				ItemTypeID;
	sint32				VI1, VI2, VI3, VI4;
	ucstring			PName;
	ucstring			ItemName;

	_msgin.serial(MainType, SubType, PID, UID, ItemID, ItemTypeID);
	_msgin.serial(VI1, VI2, VI3, VI4, PName, ItemName);

	//sipinfo(L"MT=%d, ST=%d, PID=%d, UID=%d, ItemID=%d, ItemTypeID=%d, VI1=%d, VI2=%d, VI3=%d, VI4=%d, PName=%s, ItemName=%s",
	//		MainType, SubType, PID, UID, ItemID, ItemTypeID, VI1, VI2, VI3, VI4, PName.c_str(), ItemName.c_str()); byy krs

	MAKEQUERY_NEWLOG(strQ, MainType, SubType, PID, UID, ItemID, ItemTypeID, VI1, VI2, VI3, VI4, PName.c_str(), ItemName.c_str());
	DBCaller->execute(strQ);
	//CDBConnectionRest	DBConnection(DBCaller);
	//DB_EXE_QUERY(DBConnection, stmt, strQ);
}

void	cb_M_CS_UNLAWLOG(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint8		UnlawType;
	ucstring	UnlawContent;
	uint32		FID;

	_msgin.serial(UnlawType, UnlawContent, FID);

	MAKEQUERY_UNLAWLOG(strQ, UnlawType, UnlawContent.c_str(), FID);
	DBCaller->execute(strQ);
	//CDBConnectionRest	DBConnection(DBCaller);
	//DB_EXE_QUERY(DBConnection, stmt, strQ);
}

void	cb_M_CS_GMLOG(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32				MainType;
	uint32				PID;
	uint32				UID;
	uint32				PID2;
	uint32				UID2;
	sint32				VI1, VI2, VI3;
	ucstring			VU1, UName, ObjectName;

	_msgin.serial(MainType, PID, UID, PID2, UID2);
	_msgin.serial(VI1, VI2, VI3, VU1, UName, ObjectName);

	MAKEQUERY_GMNEWLOG(strQ, MainType, PID, UID, PID2, UID2, VI1, VI2, VI3, VU1.c_str(), UName.c_str(), ObjectName.c_str());
	DBCaller->execute(strQ);
	//CDBConnectionRest	DBConnection(DBCaller);
	//DB_EXE_QUERY(DBConnection, stmt, strQ);
}

TUnifiedCallbackItem CallbackArray[] =
{
	{ M_CS_SHARDLOG,			cb_M_CS_SHARDLOG },
	{ M_CS_UNLAWLOG,			cb_M_CS_UNLAWLOG },
	{ M_CS_GMLOG,				cb_M_CS_GMLOG },
};

class CLogService : public IService
{
public:

	// ÃÊ±âÈ­19
	void init()
	{
		string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Shardlog_DBName").asString ();
		string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Shardlog_DBHost").asString ();
		string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Shardlog_DBLogin").asString ();
		string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Shardlog_DBPassword").asString ();
/*
		BOOL bSuccess = DBConnection.Connect(	ucstring(sDatabaseLogin).c_str(),
												ucstring(sDatabasePassword).c_str(),
												ucstring(sDatabaseHost).c_str(),
												ucstring(sDatabaseName).c_str());
		if (!bSuccess)
		{
			siperrornoex("ShardLog Database connection failed to '%s' with login '%s' and database name '%s'",
				sDatabaseHost.c_str(), sDatabaseLogin.c_str(), sDatabaseName.c_str());
			return;
		}
		sipinfo("ShardLog Database connection successed  to '%s' with login '%s' and database name '%s'", sDatabaseHost.c_str(), sDatabaseLogin.c_str(), sDatabaseName.c_str());
*/		
		//////////////////////////////////////////////////////////////////////////
		DBCaller = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
			ucstring(strDatabasePassword).c_str(),
			ucstring(strDatabaseHost).c_str(),
			ucstring(strDatabaseName).c_str());

		BOOL bSuccess = DBCaller->init(1);
		if (!bSuccess)
		{
			siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
				strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
			return;
		}
		//////////////////////////////////////////////////////////////////////////
		{
			MAKEQUERY_CREATELOG(strQ);
			DBCaller->execute(strQ);
			//CDBConnectionRest	DBConnection(DBCaller);
			//DB_EXE_QUERY(DBConnection, stmt, strQ);
		}
		{
			MAKEQUERY_CREATELOG1(strQ);
			DBCaller->execute(strQ);
			//CDBConnectionRest	DBConnection(DBCaller);
			//DB_EXE_QUERY(DBConnection, stmt, strQ);
		}
	}

	bool update ()
	{
		DBCaller->update();
		return true;
	}

	void release()
	{
		DBCaller->release();
		delete DBCaller;
		DBCaller = NULL;
	}
};

SIPNET_SERVICE_MAIN (CLogService, LOG_SHORT_NAME, LOG_LONG_NAME, "", 0, false, CallbackArray, "", "")

