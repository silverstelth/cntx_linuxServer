/** \file admin_service.cpp
 * Admin Service (AS)
 */


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include "admin_main_service.h"
#include "ManageAESConnection.h"
#include "ManageRequest.h"
#include "ManageAccount.h"
#include "MonitorAccount.h"
#include "ManageComponent.h"
#include "ManagePower.h"
#include "ManageMonitorSpec.h"

#include "../Common/Common.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

CDBCaller		*DBCaller = NULL;
CDBCaller		*DBMainCaller = NULL;

static void DatabaseInit()
{
	{
		string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Manager_DBName").asString ();
		string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Manager_DBHost").asString ();
		string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Manager_DBLogin").asString ();
		string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Manager_DBPassword").asString ();

		DBCaller = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
			ucstring(strDatabasePassword).c_str(),
			ucstring(strDatabaseHost).c_str(),
			ucstring(strDatabaseName).c_str());

		BOOL bSuccess = DBCaller->init(2);
		if (!bSuccess)
		{
			siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
				strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
			return;
		}
		sipinfo("Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
	}

	{
		string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Main_DBName").asString ();
		string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Main_DBHost").asString ();
		string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Main_DBLogin").asString ();
		string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Main_DBPassword").asString ();

		DBMainCaller = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
			ucstring(strDatabasePassword).c_str(),
			ucstring(strDatabaseHost).c_str(),
			ucstring(strDatabaseName).c_str());

		BOOL bSuccess = DBMainCaller->init(2);
		if (!bSuccess)
		{
			siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
				strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
			return;
		}
		sipinfo("Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
	}
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////////////////// SERVICE IMPLEMENTATION ///////////////////////////////

class CAdminService : public IService
{
public:

	/// Init the service, load the universal time.
	void init ()
	{
		DatabaseInit();
		InitManageAccount();
		InitMonitorAccount();
		InitManageAESConnection();
		InitManageRequest();
		InitManageComponent();
		InitManagePower();
		InitManageMonitorSpec();
	}

	bool update ()
	{
		UpdateManageAESConnection();
		UpdateManageRequest();
		UpdateManageAccount();
		UpdateMonitorAccount();

		DBCaller->update();
		DBMainCaller->update();
		return true;
	}

	void release ()
	{
		ReleaseManageAccount();
		ReleaseMonitorAccount();
		
		DBCaller->release();
		delete DBCaller;
		DBCaller = NULL;
		
		DBMainCaller->release();
		delete DBMainCaller;
		DBMainCaller = NULL;
	}
};


/// Admin Service
SIPNET_SERVICE_MAIN (CAdminService, AS_S_NAME, AS_L_NAME, "ASPort", 49996, true, EmptyCallbackArray, "", "");
