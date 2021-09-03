
#include "ManageRequest.h"
#include "ManageComponent.h"
#include "ManageAESConnection.h"
#include "ManageAccount.h"
#include "../../common/Packet.h"
#include "../Common/Common.h"

#include "net/varpath.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

struct CRequest
{
	CRequest (uint32 id, TSockId _sock, uint32 _uPlace) : Id(id), NbWaiting(0), NbReceived(0), NbRow(0), NbLines(1), From(_sock), uPlace(_uPlace)
	{
		Time = CTime::getSecondsSince1970 ();
	}

	uint32			Id;
	uint			NbWaiting;
	uint32			NbReceived;
	uint32			Time;			// when the request was ask
	uint32			NbRow;
	uint32			NbLines;
	
	TSockId			From;
	uint32			uPlace;

	vector<vector<string> > Array;	// it's the 2 dimensional array that will be send to the php for variables
	vector<string>	Log;			// this log contains the answer if a command was asked, othewise, Array contains the results

	uint32 getVariable(const string &variable)
	{
		for (uint32 i = 0; i < NbRow; i++)
			if (Array[i][0] == variable)
				return i;

		// need to add the variable
		vector<string> NewRow;
		NewRow.resize (NbLines);
		NewRow[0] = variable;
		Array.push_back (NewRow);
		return NbRow++;
	}

	void addLine ()
	{
		for (uint32 i = 0; i < NbRow; i++)
			Array[i].push_back("");

		NbLines++;
	}

	void display ()
	{
		if (Log.empty())
		{
			sipinfo ("Display answer array for request %d: %d row %d lines", Id, NbRow, NbLines);
			for (uint i = 0; i < NbLines; i++)
			{
				for (uint j = 0; j < NbRow; j++)
				{
					sipassert (Array.size () == NbRow);
					InfoLog->displayRaw ("%-20s", Array[j][i].c_str());
				}
				InfoLog->displayRawNL ("");
			}
			InfoLog->displayRawNL ("End of the array");
		}
		else
		{
			sipinfo ("Display the log for request %d: %d lines", Id, Log.size());
			for (uint i = 0; i < Log.size(); i++)
			{
				InfoLog->displayRaw ("%s", Log[i].c_str());
			}
			InfoLog->displayRawNL ("End of the log");
		}
	}
};

vector<CRequest> Requests;

static uint32 NewRequest(TSockId _from, uint32 _uPlace)
{
	static uint32 NextId = 5461231;

	Requests.push_back(CRequest(NextId, _from, _uPlace));

	return NextId++;
}

// Request를 보낸 써비스들의 개수를 설정한다
static void AddRequestWaitingNb (uint32 _rid, uint32 _uSendNum = 1)
{
	for (uint i = 0 ; i < Requests.size (); i++)
	{
		if (Requests[i].Id == _rid)
		{
			Requests[i].NbWaiting += _uSendNum;
			Requests[i].Time = CTime::getSecondsSince1970 ();
			return;
		}
	}
	sipwarning ("REQUEST: Received an answer from an unknown resquest %d (perhaps due to a AS timeout)", _rid);
}

static	void	AddRequestReceived (uint32 rid)
{
	for (uint i = 0 ; i < Requests.size (); i++)
	{
		if (Requests[i].Id == rid)
		{
			Requests[i].NbReceived++;
			return;
		}
	}
	sipwarning ("REQUEST: Received an answer from an unknown resquest %d (perhaps due to a AS timeout)", rid);
}

// Request에 대한 응답결과를 보관한다
static	void	AddRequestAnswer (uint32 rid, const vector<string> &variables, const vector<string> &values)
{
	for (uint i = 0 ; i < Requests.size (); i++)
	{
		Requests[i].addLine();
		if (Requests[i].Id == rid)
		{
			if (!variables.empty() && variables[0]=="__log")
			{
				sipassert (variables.size() == 1);

				for (uint j = 0; j < values.size(); j++)
				{
					Requests[i].Log.push_back (values[j]);
				}
			}
			else
			{
				sipassert (variables.size() == values.size ());
				for (uint j = 0; j < variables.size(); j++)
				{
					uint32 pos = Requests[i].getVariable (variables[j]);
					Requests[i].Array[pos][Requests[i].NbLines-1] = values[j];
				}
			}
			return;
		}
	}
	sipwarning ("REQUEST: Received an answer from an unknown resquest %d (perhaps due to a AS timeout)", rid);
}

// 새로운 Request를 보낸다
static	void	AddRequest (const string &_command, TSockId _from = NULL, uint32 _uPlace = 0)
{
	if(_command.empty ())
		return;

	CVarPath varpath (_command);
	uint32 rid = NewRequest(_from, _uPlace);
	for	(uint k = 0; k < varpath.Destination.size (); k++)
	{
		string host = varpath.Destination[k].first;

		CMessage msgout(M_AES_AES_GET_VIEW);
		msgout.serial (rid);
		msgout.serial (varpath.Destination[k].second);

		uint32	uSendNum = SendMessageToAES(L"*", L"*", host, &msgout);
		AddRequestWaitingNb(rid, uSendNum);
	}

/*
	for	(uint k = 0; k < varpath.Destination.size (); k++)
	{
		string		sZone	= varpath.Destination[k].first;
		ucstring	ucZone	= ucstring::makeFromUtf8(sZone);
		CVarPath varpath2 (varpath.Destination[k].second);
		for (uint i = 0; i < varpath2.Destination.size (); i++)
		{
			string		shard	= varpath2.Destination[i].first;
			ucstring	ucShard	= ucstring::makeFromUtf8(shard);
			CVarPath subvarpath (varpath2.Destination[i].second);

			for (uint j = 0; j < subvarpath.Destination.size (); j++)
			{
				string host = subvarpath.Destination[j].first;

				CMessage msgout("AES_GET_VIEW");
				msgout.serial (rid);
				msgout.serial (subvarpath.Destination[j].second);

				uint32	uSendNum = SendMessageToAES(ucZone, ucShard, host, &msgout);
				AddRequestWaitingNb(rid, uSendNum);
			}
		}
	}
	*/
}

static	void	AddRequest2(const string &_command)
{
	if(_command.empty ())
		return;

	CVarPath varpath (_command);
	uint32 rid = NewRequest(NULL, 0);
	for	(uint k = 0; k < varpath.Destination.size (); k++)
	{
		string realhost = varpath.Destination[k].first;
		SIPNET::TServiceId sid = GetAESIDOfHostName(realhost);
		string host = GetHostNameForMV(sid.get());
		CMessage msgout(M_AES_AES_GET_VIEW);
		msgout.serial (rid);
		msgout.serial (varpath.Destination[k].second);

		uint32	uSendNum = SendMessageToAES(L"*", L"*", host, &msgout);
		AddRequestWaitingNb(rid, uSendNum);
	}
}
void SendAdminPortOpen(uint16 nPort, std::string strAlias)
{
	std::string strCommand = SIPBASE::toString("*.%s.aesSystem netsh firewall add portopening TCP %d %s Enable All", AES_S_NAME, nPort, strAlias.c_str());
	AddRequest(strCommand);
}

static	uint32	RequestTimeout = 10;	// in second
static void varRequestTimeout(CConfigFile::CVar &var)
{
	RequestTimeout = var.asInt();
	sipinfo ("Request timeout is now after %d seconds", RequestTimeout);
}

static	void	SendReportToManagers(TSockId to, uint32 uPlace, const string &res)
{
	if (to == NULL)
		return;

	CMessage	msgout(M_MH_REQUEST);
	msgout.serial(uPlace);
	msgout.serial(const_cast<string&>(res));

	ManageHostServer->send(msgout, to);
}

static	void	CleanRequest ()
{
	uint32 currentTime = CTime::getSecondsSince1970 ();

	bool timeout;

	for (uint i = 0 ; i < Requests.size ();)
	{
		// the AES doesn't answer quickly
		timeout = (currentTime >= Requests[i].Time + RequestTimeout);

		if (Requests[i].NbWaiting <= Requests[i].NbReceived || timeout)	// the request is over
		{
			string str;
			if (timeout)
				sipwarning ("REQUEST: Request %d timeouted, only %d on %d services have replied", Requests[i].Id, Requests[i].NbReceived, Requests[i].NbWaiting);

			if (Requests[i].Log.empty())
			{
				if (Requests[i].NbRow == 0 && timeout)
				{
					str = "1 ((TIMEOUT))";
					InfoLog->displayRaw(str.c_str());
				}
				else
				{
//					str = toStringA(Requests[i].NbRow) + " ";
					InfoLog->displayRaw(str.c_str());
					for (uint k = 0; k < Requests[i].NbLines; k++)
					{
						for (uint j = 0; j < Requests[i].NbRow; j++)
						{
							sipassert (Requests[i].Array.size () == Requests[i].NbRow);
							if (Requests[i].Array[j][k].empty ())
							{
								InfoLog->displayRaw("??? ");
								str += "??? ";
							}
							else
							{
								InfoLog->displayRaw(Requests[i].Array[j][k].c_str());
								str += Requests[i].Array[j][k];
								if (timeout)
								{
									InfoLog->displayRaw("((TIMEOUT))");
									str += "((TIMEOUT))";
								}
								InfoLog->displayRaw(" ");
								str += " ";
							}
						}
					}
				}
			}
			else
			{
				for (uint k = 0; k < Requests[i].Log.size(); k++)
				{
					InfoLog->displayRaw(Requests[i].Log[k].c_str());
					str += Requests[i].Log[k];
					if (timeout)
					{
						InfoLog->displayRaw("((TIMEOUT))");
						str += "((TIMEOUT))";
					}
				}
			}
			InfoLog->displayNL("");

			SendReportToManagers(Requests[i].From, Requests[i].uPlace,  str);

			// set to 0 to erase it
			Requests[i].NbWaiting = 0;
		}

		if (Requests[i].NbWaiting == 0)
		{
			Requests.erase (Requests.begin ()+i);
		}
		else
		{
			i++;
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Callback모쥴 ///////////////////////////////////////////
static	void	cbView (CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32 rid;		
	msgin.serial (rid);

	string	shardName	= GetShardNameOfAES(tsid).toUtf8();
	string	hostName	= GetHostNameOfAES(tsid);

	vector<string> vara, vala;

	while ((uint32)msgin.getPos() < msgin.length())
	{
		vara.clear ();
		vala.clear ();

		// adding default row
//		vara.push_back ("shard");
		vara.push_back ("server");

//		vala.push_back (shardName);
		vala.push_back (hostName);

		uint32 i, nb;
		string var, val;

		msgin.serial (nb);
		for (i = 0; i < nb; i++)
		{
			msgin.serial (var);
			if (var == "__log")
			{
				vara.clear ();
				vala.clear ();
			}
			vara.push_back (var);
		}

		if (vara.size() > 0 && vara[0] == "__log")
			vala.push_back ("----- Result from Server "+hostName+"\n");

		msgin.serial (nb);
		for (i = 0; i < nb; i++)
		{
			msgin.serial (val);
			vala.push_back (val);
		}
		AddRequestAnswer(rid, vara, vala);
	}

	// inc the NbReceived counter
	AddRequestReceived(rid);
}

static	TUnifiedCallbackItem CallbackArray[] =
{
	{ M_ADMIN_VIEW, cbView },
};

static void cb_M_MH_REQUEST( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	string sRequest;
	uint16 uHostSvcID, uOnSvcID;
	uint32 uPlace;
	msgin.serial(uHostSvcID, uOnSvcID, sRequest, uPlace);

	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	string	sPower = pManagerInfo->m_sPower;
	if (!IsFullManager(sPower))
		return;

	TServiceId	_TSvcID(uHostSvcID);
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(_TSvcID);
	if (it == map_Online_AES.end())
		return;

	ONLINE_AES	& aes = it->second;

	uint32	uShardID = aes.GetShardIDOfHost();
	if (!IsShardManager(pManagerInfo->m_sPower, uShardID))
		return;

	string		sHost		= GetHostNameForMV(uHostSvcID);
	string		sEncodeName	= aes.GetMSTServiceShortNameOfService(uOnSvcID) + SIPBASE::toString("-%d", uOnSvcID);
	string		sAllRequest	= sHost + "." + sEncodeName + "." + sRequest;
	string		sHostLog	= aes.GetRegHostName();
	string		sAllRequestLog	= "[" + sHostLog + "]" + "." + sEncodeName + "." + sRequest;

	AddRequest(sAllRequest, from, uPlace);
	// Log기록
	ucstring	ucLog	= ucstring::makeFromUtf8(sAllRequestLog);
	AddManageHostLog(LOG_MANAGE_REQUEST, from, ucLog);

	
}
static void cb_M_MH_REQUESTHOST( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	string sRequest;
	uint16 uHostSvcID;
	uint32 uPlace;
	msgin.serial(uHostSvcID, sRequest, uPlace);

	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	TServiceId	_TSvcID(uHostSvcID);
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(_TSvcID);
	if (it == map_Online_AES.end())
	{
		sipwarning("map_Online_AES.find failed. uHostSvcID=%d, sRequest=%s, uPlace=%d", uHostSvcID, sRequest.c_str(), uPlace);
		return;
	}

	ONLINE_AES	& aes = it->second;

	uint32	uShardID = aes.GetShardIDOfHost();
	if (!IsShardManager(pManagerInfo->m_sPower, uShardID))
		return;
	
	ucstring	ucZone		= aes.GetRegZoneNameOfHost();		string		sZone		= ucZone.toUtf8();
	ucstring	ucShard		= aes.GetRegShardNameOfHost();		string		sShard		= ucShard.toUtf8();
	string		sHost		= GetHostNameForMV(uHostSvcID);
	string		sHostLog	= aes.GetRegHostName();
//	string		sAllRequest	= sZone + "." + sShard + "." + sHost + "." + AES_S_NAME + "." + sRequest;
	string		sAllRequest	= sHost + "." + AES_S_NAME + "." + sRequest;
	string		sAllRequestLog	= "[" + sHostLog + "]" + "." + AES_S_NAME + "." + sRequest;

	AddRequest(sAllRequest, from, uPlace);
	// Log기록
	ucstring	ucLog	= ucstring::makeFromUtf8(sAllRequestLog);
	AddManageHostLog(LOG_MANAGE_REQUEST, from, ucLog);
}

static void cb_M_MH_STOPSERVICE( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	uint16	uHostSvcID, uSvcID;
	uint32	uPlace;
	msgin.serial(uHostSvcID, uSvcID, uPlace);
	
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	TServiceId	tSvcID(uHostSvcID);
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(tSvcID);
	if (it == map_Online_AES.end())
		return;

	ONLINE_AES	& aes = it->second;

	uint32	uShardID = aes.GetShardIDOfHost();
	if (!IsShardManager(pManagerInfo->m_sPower, uShardID))
		return;

	ucstring	ucZone		= aes.GetRegZoneNameOfService(uSvcID);					string		sZone		= ucZone.toUtf8();
	ucstring	ucShard		= aes.GetRegShardNameOfService(uSvcID);		string		sShard		= ucShard.toUtf8();
	string		sHost		= GetHostNameForMV(uHostSvcID);
	string		sHostLog	= aes.GetRegHostName();
	string		sEncodeName	= aes.GetMSTServiceShortNameOfService(uSvcID) + SIPBASE::toString("-%d", uSvcID);

//	string		sAllRequest	= sZone + "." + sShard + "." + sHost + "." + sShortName + ".quit";
	string		sAllRequest	= sHost + "." + sEncodeName + ".quit";
	string		sAllRequestLog	= "[" + sHostLog + "]" + "." + sEncodeName + ".quit";

	AddRequest(sAllRequest, from, uPlace);
	// Log기록
	ucstring	ucLog	= ucstring::makeFromUtf8(sAllRequestLog);
	AddManageHostLog(LOG_MANAGE_REQUEST, from, ucLog);
}

static void cb_M_MH_OPENSHARD( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	uint32	uShard, uPlace;
	msgin.serial(uShard, uPlace);

	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	if (!IsShardManager(pManagerInfo->m_sPower, uShard))
		return;

	ucstring	ucShard;
	FOR_START_MAP_REGHOSTINFO(it)
	{
		uint32		uHostID		= it->second.m_uID;
		uint32		uShardID	= it->second.m_uShardID;
		if (uShardID != uShard)
			continue;

		ucstring	ucZone		= GetRegZoneNameOfShard(uShardID);
		string		sZone		= ucZone.toUtf8();
		ucShard					= GetRegShardName(uShardID);
		string		sShard		= ucShard.toUtf8();

		FOR_START_MAP_ONLINE_AES(itAES)
		{
			if ( uHostID == itAES->second.m_uID )
				break;
		}
		if ( itAES == map_Online_AES.end() )
			continue;

		uint16	uOnSvcID = itAES->first.get();
		string		sHost		= GetHostNameForMV(uOnSvcID);

//		string		sAllRequest1	= sHost + "." + AES_S_NAME + ".quitAllService";
//		AddRequest(sAllRequest1, from, uPlace);

		FOR_START_MAP_REGSERVICEINFO(itService)
		{
			uint32		uID	= itService->second.m_uID;
			if (uHostID != itService->second.m_uHostID)
				continue;

			//ucstring		sAliasName	= GetMSTServiceAliasNameOfService(uServiceID);
			//ucstring		sAliasName	= GetServiceExeNameOfService(uServiceID);
			string		sSvcExe	= GetServiceExeNameOfService(uID);

//			string		sAllRequest2	= sZone + "." + sShard + "." + sHost + "." + AES_S_NAME + ".launchService " + sShortName;
			string		sAllRequest2	= sHost + "." + AES_S_NAME + ".launchService " + sSvcExe;

			AddRequest(sAllRequest2, from, uPlace);
		}
	}
	// Log기록
	ucstring	ucLog	= L"Open shard";
	AddManageHostLog(LOG_MANAGE_REQUEST, from, ucLog);
}
static void cb_M_MH_CLOSESHARD( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	uint32	uShard, uPlace;
	msgin.serial(uShard, uPlace);

	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	if (!IsShardManager(pManagerInfo->m_sPower, uShard))
		return;

	ucstring	ucShard;
	FOR_START_MAP_ONLINE_AES(itAES)
	{
		uint32		uHostID		= itAES->second.m_uID;
		uint32		uShardID	= itAES->second.GetShardIDOfHost();
		uint16		uOnSvcID	= itAES->second.m_ConnectionInfo.m_ServiceID.get();
		if (uShardID != uShard)
			continue;

		ucstring	ucZone		= GetRegZoneNameOfShard(uShardID);
		string		sZone		= ucZone.toUtf8();
		ucShard					= GetRegShardName(uShardID);
		string		sShard		= ucShard.toUtf8();
		string		sHost		= GetHostNameForMV(uOnSvcID);

		//		string		sAllRequest	= sZone + "." + sShard + "." + sHost + "." + AES_S_NAME + ".quitAllService";
		string		sAllRequest	= sHost + "." + AES_S_NAME + ".quitAllService";
		AddRequest(sAllRequest, from, uPlace);
	}

	// Log기록
	ucstring	ucLog	= L"Close shard";
	AddManageHostLog(LOG_MANAGE_REQUEST, from, ucLog);
}
static void cb_M_MH_LOCKSHARD( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	uint32	uShard, uPlace;
	msgin.serial(uShard, uPlace);

	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	if (!IsShardManager(pManagerInfo->m_sPower, uShard))
		return;

	ucstring	ucShard;
	FOR_START_MAP_ONLINE_AES(itAES)
	{
		uint32		uHostID		= itAES->second.m_uID;
		uint32		uShardID	= itAES->second.GetShardIDOfHost();
		uint16		uOnSvcID	= itAES->second.m_ConnectionInfo.m_ServiceID.get();
		if (uShardID != uShard)
			continue;

		ucstring	ucZone		= GetRegZoneNameOfShard(uShardID);
		string		sZone		= ucZone.toUtf8();
		ucShard					= GetRegShardName(uShardID);
		string		sShard		= ucShard.toUtf8();
		string		sHost		= GetHostNameForMV(uOnSvcID);

		//		string		sAllRequest	= sZone + "." + sShard + "." + sHost + "." + AES_S_NAME + ".quitAllService";
		string		sAllRequest	= sHost + "." + AES_S_NAME + ".lockShard";
		AddRequest(sAllRequest, from, uPlace);
	}

	// Log기록
	ucstring	ucLog	= L"Close shard";
	AddManageHostLog(LOG_MANAGE_REQUEST, from, ucLog);
}
static void cb_M_MH_UNLOCKSHARD( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	uint32	uShard, uPlace;
	msgin.serial(uShard, uPlace);

	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	if (!IsShardManager(pManagerInfo->m_sPower, uShard))
		return;

	ucstring	ucShard;
	FOR_START_MAP_ONLINE_AES(itAES)
	{
		uint32		uHostID		= itAES->second.m_uID;
		uint32		uShardID	= itAES->second.GetShardIDOfHost();
		uint16		uOnSvcID	= itAES->second.m_ConnectionInfo.m_ServiceID.get();
		if (uShardID != uShard)
			continue;

		ucstring	ucZone		= GetRegZoneNameOfShard(uShardID);
		string		sZone		= ucZone.toUtf8();
		ucShard					= GetRegShardName(uShardID);
		string		sShard		= ucShard.toUtf8();
		string		sHost		= GetHostNameForMV(uOnSvcID);

		//		string		sAllRequest	= sZone + "." + sShard + "." + sHost + "." + AES_S_NAME + ".quitAllService";
		string		sAllRequest	= sHost + "." + AES_S_NAME + ".unlockShard";
		AddRequest(sAllRequest, from, uPlace);
	}

	// Log기록
	ucstring	ucLog	= L"Close shard";
	AddManageHostLog(LOG_MANAGE_REQUEST, from, ucLog);
}

static TCallbackItem MHCallbackArray[] =
{
	{ M_MH_REQUEST,			cb_M_MH_REQUEST },
	{ M_MH_REQUESTHOST,		cb_M_MH_REQUESTHOST },
	{ M_MH_STOPSERVICE,		cb_M_MH_STOPSERVICE },
	{ M_MH_OPENSHARD,		cb_M_MH_OPENSHARD },
	{ M_MH_CLOSESHARD,		cb_M_MH_CLOSESHARD },
	{ M_MH_LOCKSHARD,		cb_M_MH_LOCKSHARD },
	{ M_MH_UNLOCKSHARD,		cb_M_MH_UNLOCKSHARD },
	
};

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// extern 모쥴 ///////////////////////////////////////////

// 초기화
void	InitManageRequest()
{
	CUnifiedNetwork::getInstance ()->addCallbackArray(CallbackArray, sizeof(CallbackArray)/sizeof(CallbackArray[0]));

	varRequestTimeout (IService::getInstance ()->ConfigFile.getVar ("RequestTimeout"));
	IService::getInstance ()->ConfigFile.setCallback("RequestTimeout", &varRequestTimeout);

	ManageHostServer->addCallbackArray (MHCallbackArray, sizeof(MHCallbackArray)/sizeof(TCallbackItem));

}

// Update
void	UpdateManageRequest()
{
	CleanRequest ();
}
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Command 모쥴 ///////////////////////////////////////////

SIPBASE_CATEGORISED_COMMAND(AS, getViewAS, "send a view and receive an array as result", "<[hostname].serviceshortname.command>")
{
	string cmd;
	for (uint i = 0; i < args.size(); i++)
	{
		if (i != 0) cmd += " ";
		cmd += args[i];
	}

	AddRequest2(cmd);

	return true;
}
