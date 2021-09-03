#include "ManageAccount.h"
#include "QuerySet.h"
#include "misc/db_interface.h"
#include "../../common/Packet.h"
#include "ManageComponent.h"
#include "ManageAESConnection.h"
#include "ManageMonitorSpec.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

CCallbackServer *	MonitorServer = NULL;

void	SendHostConnectionToMonitor(TServiceId tsid)
{
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(tsid);
	if (it == map_Online_AES.end())
		return;

	ONLINE_AES	info = it->second;
	if ( info.m_bRegister )
	{
		CMessage	mes(M_AD_REGAES_CONNECT);
		mes.serial(info);
		MonitorServer->send(mes);
	}
	else
	{
		CMessage	mes(M_AD_UNREGAES_CONNECT);
		uint32	uHostID = info.m_uID;
		REGHOSTINFO	& reginfo = map_UNREGHOSTINFO[uHostID];
		mes.serial(reginfo);
		mes.serial(info);
		MonitorServer->send(mes);
	}
}

void	SendHostDisConnectionToMonitor(TServiceId tsid)
{
	uint16	sid = tsid.get();
	CMessage	mes(M_AD_AES_DISCONNECT);
	mes.serial(sid);
	MonitorServer->send(mes);
}

void	SendServiceConnectionToMonitor(uint16 sid, TServiceId tsidAES)
{
	MAP_ONLINE_AES::iterator itAES = map_Online_AES.find(tsidAES);
	if (itAES == map_Online_AES.end())
		return;

	ONLINE_AES	aes		= itAES->second;
	MAP_ONLINESERVICE::iterator it = aes.m_OnlineService.find(sid);
	if (it == aes.m_OnlineService.end())
		return;

	SERVICE_ONLINE_INFO &info = it->second;
	bool	bRegister = info.m_bRegDB;
	if ( bRegister )
	{
		CMessage	mes(M_AD_REGSVC_CONNECT);
		mes.serial(info);
		MonitorServer->send(mes);
	}
	else
	{
		CMessage	mes(M_AD_UNREGSVC_CONNECT);
		REGSERVICEINFO	reginfo;
		if ( info.m_bRegDB )
			reginfo = map_REGSERVICEINFO[info.m_RegID];
		else
			reginfo = map_UNREGSERVICEINFO[info.m_RegID];
		mes.serial(reginfo);
		mes.serial(info);
		MonitorServer->send(mes);
	}
}

void	SendServiceDisConnectionToMonitor(uint16 sid, TServiceId tsidAES)
{
	CMessage	mes(M_AD_SERVICE_DISCONNECT);
	uint16	aesid = tsidAES.get();
	mes.serial(aesid, sid);
	MonitorServer->send(mes);
}

// Manager 立加
static void cbMonitorConnection (TSockId from, void *arg)
{
	CMessage	mes(M_MO_CONFIRM);

	MonitorServer->send(mes, from);
	
	sipinfo("New monitor connection");
}
// Manager 立加秦力
static void cbMonitorDisconnection( TSockId from, void *arg )
{
	int n = 0;
}

static void cb_M_MO_COMPINFO( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	CMessage	mes(M_MO_COMPINFO);
	Make_COMPINFO(&mes, "");
	MonitorServer->send(mes, from);

	CMessage	mesOnline(M_MH_ONLINEINFO);
	Make_ONLINEINFO(&mesOnline);
	MonitorServer->send(mesOnline, from);

	CMessage	mesMV(M_MO_MVARIABLE);
	Make_MVARIABLE(&mesMV);
	MonitorServer->send(mesMV, from);
}

static TCallbackItem CallbackArray[] =
{
	{ M_MO_COMPINFO,	cb_M_MO_COMPINFO },

};

void	InitMonitorAccount()
{
	MonitorServer = new CCallbackServer ();
	if (MonitorServer == NULL)
	{
		siperrornoex("Failed creating ManagerHost's server.");
		return;
	}
	MonitorServer->addCallbackArray (CallbackArray, sizeof(CallbackArray)/sizeof(TCallbackItem));
	MonitorServer->setConnectionCallback(cbMonitorConnection, 0);
	MonitorServer->setDisconnectionCallback (cbMonitorDisconnection, 0);

	uint16 port = 40010;
	if ( IService::getInstance()->ConfigFile.exists("MonitorPort") )
		port = IService::getInstance()->ConfigFile.getVar("MonitorPort").asInt();

	MonitorServer->init(port);
}

void	UpdateMonitorAccount()
{
	MonitorServer->update();
}

void	ReleaseMonitorAccount()
{
	if ( MonitorServer )
	{
		MonitorServer->disconnect(InvalidSockId);
		delete MonitorServer;
		MonitorServer = NULL;
	}

}

