#include "ManageAccount.h"

#include "misc/db_interface.h"
#include "net/varpath.h"

#include "../../common/Packet.h"
#include "QuerySet.h"

#include "ManageAESConnection.h"
#include "ManageComponent.h"
#include "MonitorAccount.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

MAP_MONITORVARIABLE	map_MONITORVARIABLE;

#define	FOR_START_MAP_MONITORVARIABLE(it)											\
	MAP_MONITORVARIABLE::iterator	it;												\
	for(it = map_MONITORVARIABLE.begin(); it != map_MONITORVARIABLE.end(); it++)	\

static void	DBCB_DBGetAllMonitor(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32	uID;			resSet->serial(uID);
		string sPath;			resSet->serial(sPath);
		ucstring ucCom;			resSet->serial(ucCom);
		uint32 uCycle;			resSet->serial(uCycle);
		uint32 uDefault;		resSet->serial(uDefault);
		uint32 uUplimit;		resSet->serial(uUplimit);
		uint32 uLowlimit;		resSet->serial(uLowlimit);

		CVarPath varpath1 (sPath);
		string sHost	= varpath1.Destination[0].first;
		CVarPath varpath2 (varpath1.Destination[0].second);
		string sService	= varpath2.Destination[0].first;
		string sVar		= varpath2.Destination[0].second;

		map_MONITORVARIABLE.insert( make_pair(uID, MONITORVARIABLE(uID, sHost, sService, sVar, ucCom, uCycle, uDefault, uUplimit, uLowlimit)) );
	}
}
// 감시변수자료를 load한다
static	void	LoadMonitorVariable()
{
	map_MONITORVARIABLE.clear();
	MAKEQUERY_GETALLMONITORVARIABLE(sQ);
	DBCaller->execute(sQ, DBCB_DBGetAllMonitor);
}

void	SendMonitorInfoToAES(TServiceId _TServiceId)
{
	MAP_ONLINE_AES::iterator itAes = map_Online_AES.find(_TServiceId);
	if (itAes == map_Online_AES.end())
		return;

	ONLINE_AES	info = itAes->second;
	
	uint32	_uHostID = info.m_uID;
	string	sHost = GetHostNameForMV(_TServiceId.get());
	
	vector<string> informations;
	CMessage msgout(M_AES_AES_INFO);

	informations.clear ();
	// 현재 주기적으로 값을 보내오는 형태의 감시만 진행한다
	msgout.serialCont (informations);

	informations.clear ();
	FOR_START_MAP_MONITORVARIABLE(it)
	{
		if (sHost != it->second.m_sHost && it->second.m_sHost != "*")
			continue;

		string strPath = it->second.m_sService + "." + it->second.m_sVariable;
		informations.push_back (strPath);
		string strG = toString("%u", it->second.m_uCycle);
		informations.push_back (strG);

	}
	msgout.serialCont (informations);

	CUnifiedNetwork::getInstance ()->send(_TServiceId, msgout);
}

// Send all monitor variables to all host
void	S_SendMonitorInfoToAllAES()
{
	FOR_START_MAP_ONLINE_AES(it)
	{
		TServiceId	svcID		= it->second.m_ConnectionInfo.m_ServiceID;
//		string		sHostName	= it->second.GetRegHostName();
		SendMonitorInfoToAES(svcID);
	}
}

// 감시정보변수를 파케트로 만든다
void	Make_MVARIABLE(CMessage *pMes)
{
	FOR_START_MAP_MONITORVARIABLE(it)
	{
		pMes->serial(it->second.m_uID, it->second.m_sHost, it->second.m_sService, it->second.m_sVariable, it->second.m_ucComment, it->second.m_uCycle);
		pMes->serial(it->second.m_uDefaultWarning, it->second.m_uUpLimit, it->second.m_uLowLimit);
	}
}
void cbGraphUpdate (CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32 CurrentTime;
	msgin.serial (CurrentTime);

	string	sHost = GetHostNameOfAES(tsid);
	string	stime = CTime::getMixedTime();
	
	CMessage	msgout(M_MO_VARIABLE);
	msgout.serial(CurrentTime);
	msgout.serial(sHost);
	msgout.serial(stime);

	while (msgin.getPos() < (sint32)msgin.length())
	{
		string var, service;
		sint32 val;
		msgin.serial (service, var, val);

		msgout.serial(service, var, val);
	}
	MonitorServer->send(msgout);
}
void cbServiceNotPong(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	string	sService;
	msgin.serial(sService);

	string	sHost = GetHostNameOfAES(tsid);

	CMessage	msgout(M_MO_NOTPING);
	msgout.serial(sHost);
	msgout.serial(sService);
	MonitorServer->send(msgout);
}

static void cb_M_MO_DELMV( CMessage& _msgin, TSockId _from, CCallbackNetBase& _cnb )
{
	uint32	uMVID;
	_msgin.serial(uMVID);

	MAKEQUERY_DELETEMV(sQ, uMVID);
	DBCaller->execute(sQ);
	map_MONITORVARIABLE.erase(uMVID);

	S_SendMonitorInfoToAllAES();

	CMessage	msgOut(M_MO_DELMV);
	msgOut.serial(uMVID);
	MonitorServer->send(msgOut);

	sipinfo("Delete monitor variable : ID=%d", uMVID);
/*
	MAKEQUERY_DELETEMV(sQ, uMVID);
	DB_EXE_QUERY(DBForManage, Stmt, sQ);
	if (nQueryResult == TRUE)
	{
		map_MONITORVARIABLE.erase(uMVID);

		S_SendMonitorInfoToAllAES();
		
		CMessage	msgOut(M_MO_DELMV);
		msgOut.serial(uMVID);
		MonitorServer->send(msgOut);

		sipinfo("Delete monitor variable : ID=%d", uMVID);
	}
	*/
}

static void	DBCB_DBAddMV(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32	uCycle		= *(uint32 *)argv[0];
	uint32	uDefault	= *(uint32 *)argv[1];
	uint32	uUp			= *(uint32 *)argv[2];
	uint32	uLow		= *(uint32 *)argv[3];
	string	sPath		= (char *)argv[4];

	if (!nQueryResult)
		return;
	uint32	nRows;		resSet->serial(nRows);
	if (nRows <= 0)
		return;

	uint32 uID;			resSet->serial(uID);

	CVarPath varpath1 (sPath);
	string sHost	= varpath1.Destination[0].first;
	CVarPath varpath2 (varpath1.Destination[0].second);
	string sService	= varpath2.Destination[0].first;
	string sVar		= varpath2.Destination[0].second;

	map_MONITORVARIABLE.insert( make_pair(uID, MONITORVARIABLE(uID, sHost, sService, sVar, L"", uCycle, uDefault, uUp, uLow)) );

	S_SendMonitorInfoToAllAES();

	CMessage	msgOut(M_MO_ADDMV);
	msgOut.serial(uID, sHost, sService, sVar, uCycle, uDefault, uUp, uLow);
	MonitorServer->send(msgOut);

	sipinfo("Add monitor variable : Path=%s, Cycle=%d", sPath.c_str(), uCycle);

}

static void cb_M_MO_ADDMV( CMessage& _msgin, TSockId _from, CCallbackNetBase& _cnb )
{
	string sHost, sPath;
	uint32 uCycle, uDefault, uUp, uLow;
	_msgin.serial(sHost, sPath, uCycle, uDefault, uUp, uLow);
	
	uint32 uHostID = GetRegisteredHostID(L"*", sHost);
	string sHostMV = GetHostNameForMV(uHostID);
	sPath = sHostMV + "." + sPath;

	MAKEQUERY_ADDMV(sQ, ucstring(sPath).c_str(), uCycle, uDefault, uUp, uLow);
	DBCaller->executeWithParam(sQ, DBCB_DBAddMV,
		"DDDDs",
		CB_,		uCycle,
		CB_,		uDefault,
		CB_,		uUp,
		CB_,		uLow,
		CB_,		sPath.c_str());
}

static void cb_M_MO_PING( CMessage& _msgin, TSockId _from, CCallbackNetBase& _cnb )
{
	CMessage	msgOut(M_MO_PONG);
	MonitorServer->send(msgOut, _from);
}

static TCallbackItem CallbackArray[] =
{
	{ M_MO_DELMV,		cb_M_MO_DELMV },
	{ M_MO_ADDMV,		cb_M_MO_ADDMV },
	{ M_MO_PING,		cb_M_MO_PING },
	
};

void	InitManageMonitorSpec()
{
	LoadMonitorVariable();
	MonitorServer->addCallbackArray (CallbackArray, sizeof(CallbackArray)/sizeof(TCallbackItem));
}
