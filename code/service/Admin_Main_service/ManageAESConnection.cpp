
#include "ManageAESConnection.h"
#include "../../common/Packet.h"
#include "../Common/Common.h"
#include "ManageAccount.h"
#include "ManageComponent.h"
#include "ManageRequest.h"
#include "MonitorAccount.h"
#include "ManageMonitorSpec.h"
#include "../Common/QuerySetForMain.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

CVariable<uint16> ManagePort("as","ManagePort", "ManagePort", 40000, 0, true);
CVariable<uint16> MonitorPort("as","MonitorPort", "MonitorPort", 40010, 0, true);
CVariable<uint16> ASPort("as","ASPort", "ASPort", 49996, 0, true);

CVariable<uint32> PingTimeout("as","PingTimeout", "in seconds, time before services have to answer the ping message or will be killed", 120, 0, true);		// in seconds, timeout before killing the service
CVariable<uint32> PingFrequency("as","PingFrequency", "in seconds, time between each ping message", 60, 0, true);		// in seconds, frequency of the send ping to services
CVariable<bool> AcceptUnRegService("as","AcceptUnRegService", "if true, any unregistered service can run. otherwise, only registered service can run", true, 0, true);		// in seconds, frequency of the send ping to services

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// 변수관리모쥴 ///////////////////////////////////////////

static	list<AES_CONNECTION_INFO>					lst_NoConfirmAES;	// 인증이 되지 않는 AES Connection
MAP_ONLINE_AES										map_Online_AES;		// 인증된 AES Connection

#define	FOR_START_LST_NOCONFIRMAES(it)											\
	list<AES_CONNECTION_INFO>::iterator	it;										\
	for(it = lst_NoConfirmAES.begin(); it != lst_NoConfirmAES.end(); it++)		\

#define	FOR_START_MAP_ONLINE_AES(it)											\
	MAP_ONLINE_AES::iterator	it;												\
	for(it = map_Online_AES.begin(); it != map_Online_AES.end(); it++)			\

// Host의 소속ID얻기
uint32	ONLINE_AES::GetShardIDOfHost()
{
	return	::GetShardIDOfHost(m_uID, m_bRegister);
}

// Host의 소속Shard이름얻기
ucstring	ONLINE_AES::GetRegShardNameOfHost()
{
	return	::GetRegShardNameOfHost(m_uID, m_bRegister);
}

ucstring	ONLINE_AES::GetRegZoneNameOfHost()
{
	return ::GetRegZoneNameOfHost(m_uID, m_bRegister);
}

ucstring	ONLINE_AES::GetRegZoneNameOfService(uint16 _uSvcID)
{
	uint32	uID;
	bool	bRegister;
/*	if (_uSvcID == 0)
	{
		uID = _uID;
		bRegister = true;
	}
	else*/
	{
		MAP_ONLINESERVICE::iterator it = m_OnlineService.find(_uSvcID);
		if ( it == m_OnlineService.end() )
			return L"";
		
		uID			= it->second.m_RegID;
		bRegister	= it->second.m_bRegDB;
	}

	return ::GetRegZoneNameOfHost(uID, bRegister);
}

ucstring	ONLINE_AES::GetRegShardNameOfService(uint16 _uSvcID)
{
	MAP_ONLINESERVICE::iterator it = m_OnlineService.find(_uSvcID);
	if ( it == m_OnlineService.end() )
		return L"";

	uint32	uID			= it->second.m_RegID;
	bool	bRegister	= it->second.m_bRegDB;

	return ::GetRegShardNameOfHost(uID, bRegister);
}

std::string	ONLINE_AES::GetRegHostNameOfService(uint16 _uSvcID)
{
	MAP_ONLINESERVICE::iterator it = m_OnlineService.find(_uSvcID);
	if ( it == m_OnlineService.end() )
		return "";

	uint32	uID = it->second.m_OnHostSvcID;

	return this->GetRegHostName();
}

// 동작써비스Short이름얻기
string	ONLINE_AES::GetMSTServiceShortNameOfService(uint16 _uSvcID)
{
	SERVICE_ONLINE_INFO &info = GetOnlineService(_uSvcID);
	uint32	uid = info.m_RegID;
//	if ( uid == 0 )
//		return "";
	bool	bReg = info.m_bRegDB;

	return	::GetMSTServiceShortNameOfService(uid, bReg);
}

// HOST이름얻기
string	ONLINE_AES::GetRegHostName()
{
	return	::GetRegHostName(m_uID, m_bRegister);
}

uint32 ONLINE_AES::GetServiceIDFromDBID(uint32 _uID)
{
	uint32	uServiceID = 0;
	MAP_ONLINESERVICE::iterator its;
	for (its = m_OnlineService.begin(); its != m_OnlineService.end(); its++)
	{
		uint32	uOnID = its->second.m_RegID;
		if ( _uID == uOnID )
		{
			uServiceID = its->first;
			break;
		}
	}

	return uServiceID;
}

uint32	ONLINE_AES::GetRegIDFromSvcID(uint16 _uID)
{
	uint32	uServiceID = 0;
	MAP_ONLINESERVICE::iterator its = m_OnlineService.find(_uID);
	if (its != m_OnlineService.end())
		return its->second.m_RegID;

	return 0;
}

bool	ONLINE_AES::IsOnlineRegService(uint32 _uID)
{
	bool	bOnline = false;
	uint32	uRegID;
	MAP_ONLINESERVICE::iterator its;
	for (its = m_OnlineService.begin(); its != m_OnlineService.end(); its++)
	{
		if ( !its->second.m_bRegDB )
			continue;
		uRegID = its->second.m_RegID;
		if ( _uID == uRegID )
		{
			bOnline = true;
			break;
		}
	}

	return bOnline;
}

bool	ONLINE_AES::IsOnlineMstService(uint32 _uMstID)
{
	bool	bOnline = false;
	uint32	uRegID;
	MAP_ONLINESERVICE::iterator its;
	for (its = m_OnlineService.begin(); its != m_OnlineService.end(); its++)
	{
		uRegID = its->second.m_RegID;
		uint32	uMstID;
		if ( its->second.m_bRegDB )
			uMstID = map_REGSERVICEINFO[uRegID].m_uMstSvcID;
		else
			uMstID = map_UNREGSERVICEINFO[uRegID].m_uMstSvcID;

		if ( uMstID == _uMstID )
		{
			bOnline = true;
			break;
		}
	}

	return bOnline;
}

bool	ONLINE_AES::IsExistOnlineServiceOfShard(uint16 _uSvcID, uint32 _uMstID, uint32 _uShardID)
{
	bool	bOnline = false;
	uint32	uRegID, uShardID;
	MAP_ONLINESERVICE::iterator its;
	for (its = m_OnlineService.begin(); its != m_OnlineService.end(); its++)
	{
		if ( _uSvcID == its->second.m_OnSvcID ) // itself
			continue;

		uRegID = its->second.m_RegID;
		uShardID = its->second.m_ShardID;
		uint32	uMstID;
		if ( its->second.m_bRegDB )
			uMstID = map_REGSERVICEINFO[uRegID].m_uMstSvcID;
		else
			uMstID = map_UNREGSERVICEINFO[uRegID].m_uMstSvcID;

		if ( uMstID == _uMstID &&
			(uShardID == DEFAULT_SHARD_ID || uShardID == _uShardID ) )
		{
			bOnline = true;
			break;
		}
	}

	return bOnline;
}

bool	ONLINE_AES::AddOnlineService(uint16 _uOnID, std::string _sShortName)
{
	bool	bRegistered;
	//uint32	uID = GetServiceIDFromShortName(m_uID, _sShortName);
	uint32	uRegID = 0;
	FOR_START_MAP_REGSERVICEINFO(it)
	{
		uint32		uID		= it->second.m_uID;
		uint32		uHostID	= it->second.m_uHostID;
		uint32		uMSTID	= it->second.m_uMstSvcID;
		uint32		uIndex	= it->second.m_uIndex;

		if ( IsOnlineRegService(uID) )
			continue;
		if (uHostID != m_uID)
			continue;
		string		sShort = GetMSTServiceShortName(uMSTID);
		if (uIndex != 0)
			sShort += SIPBASE::toString("%d", uIndex);
		if ( sShort == _sShortName || ( sShort == WELCOME_SHORT_NAME && strstr( _sShortName.c_str(), sShort.c_str() ) != NULL) )
		{
			uRegID = uID;
			break;
		}
	}

	if ( uRegID != 0 ) //Registered Services in DB
	{
		bRegistered = true;
	}
	else
	{
		sipwarning("FindRegisterService failed: _uOnID=%d, _sShortName=%s, uRegID=%d", _uOnID, _sShortName.c_str(), uRegID);

		bRegistered = false;
		uint32 uMstID	= GetMSTServiceIDFromSName(_sShortName);
		if (uMstID == 0)
			sipwarning("Unknown Service(%s) is connected!", _sShortName.c_str());

		std::string	sExeName= GetMSTServiceExeName(uMstID);

		uRegID = static_cast<uint32>(map_UNREGSERVICEINFO.size());
		map_UNREGSERVICEINFO.insert(std::make_pair(uRegID, REGSERVICEINFO(uRegID,  uMstID, 0, m_uID, 0, m_bRegister, sExeName)));
	}

	m_OnlineService.insert(make_pair(_uOnID, SERVICE_ONLINE_INFO(bRegistered, _uOnID, m_ConnectionInfo.m_ServiceID.get(), uRegID)));

	return bRegistered;
}

SERVICE_ONLINE_INFO& ONLINE_AES::GetOnlineService(uint16 _uSvcID)
{
	static	SERVICE_ONLINE_INFO temp;
	MAP_ONLINESERVICE::iterator it = m_OnlineService.find(_uSvcID);
	if ( it == m_OnlineService.end() )
		return temp;

	return	it->second;
}


void	DeleteServicePort(uint16 _uOnHostID, uint16 _uOnSvcID)
{
	MAP_REGPORT::iterator	it;
	std::vector<uint32>		temp;
	for ( it = map_REGPORT.begin(); it != map_REGPORT.end(); it ++ )
	{
		REGPORTINFO	info = it->second;
		if ( _uOnHostID == info._uAESSvcID &&
			_uOnSvcID == info._uSvcID )
			temp.push_back(it->first);
	}

	uint32	nSize = static_cast<uint32>(temp.size());
	for ( uint32 i = 0; i < nSize; i ++ )
	{
		uint32	nIndex = temp[i];
		map_REGPORT.erase(nIndex);		
	}

	temp.clear();
}

void	DeleteHostePort(uint16 _uOnHostID)
{
	MAP_REGPORT::iterator	it;
	std::vector<uint32>		temp;
	for ( it = map_REGPORT.begin(); it != map_REGPORT.end(); it ++ )
	{
		REGPORTINFO	info = it->second;
		if ( _uOnHostID == info._uAESSvcID )
			temp.push_back(it->first);
	}

	uint32	nSize = static_cast<uint32>(temp.size());
	for ( uint32 i = 0; i < nSize; i ++ )
	{
		uint32	nIndex = temp[i];
		map_REGPORT.erase(nIndex);		
	}

	temp.clear();
}

uint32	ONLINE_AES::DelOnlineService(uint16 _uOnID)
{
	MAP_ONLINESERVICE::iterator it = m_OnlineService.find(_uOnID);
	if (it == m_OnlineService.end())
		return 0;

	uint32	uServiceID	= it->second.m_OnSvcID;
	uint32	uID	= it->second.m_RegID;

	DeleteServicePort(m_ConnectionInfo.m_ServiceID.get(), _uOnID);

	bool	bRegister	= it->second.m_bRegDB;
	/*if ( !bRegister )
		DeleteUnRegService(uID);
	*/
	m_OnlineService.erase(_uOnID);

	return uServiceID;
}

// RemoveNoConfirmAES
static	void	RemoveNoConfirmAES(const TServiceId &_sid)
{
	FOR_START_LST_NOCONFIRMAES(it)	
	{
		if ((*it).m_ServiceID == _sid)
		{
			lst_NoConfirmAES.erase(it);
			break;
		}
	}
}
// RemoveOnlineAES
static	void	RemoveOnlineAES(const TServiceId &_sid)
{
	MAP_ONLINE_AES::iterator	it = map_Online_AES.find(_sid);
	if ( it == map_Online_AES.end() )
		return;

	ONLINE_AES	&aes = it->second;
	/*if ( ! aes.m_bRegister )
		DeleteUnRegHost(it->second.GetRegIDFromSvcID(_sid.get()));*/
	map_Online_AES.erase(_sid);

}
// IsNoConfirmAES
static	bool	IsNoConfirmAES(const TServiceId &_sid)
{
	FOR_START_LST_NOCONFIRMAES(it)	
	{
		if ((*it).m_ServiceID == _sid)
			return true;
	}
	return false;
}

// IsOnlineAES
static	bool	IsOnlineAES(const TServiceId &_sid)
{
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(_sid);
	if (it == map_Online_AES.end())
		return false;
	return true;
}

static	bool	IsOnlineAES(uint32 _uID)
{
	FOR_START_MAP_ONLINE_AES(it)
	{
		ONLINE_AES *pAES = &(it->second);
		if (pAES->m_uID == _uID)
			return true;
	}
	return false;
}

uint32	GetSIdOfService(uint32 _uID)
{
	uint32	serviceID = 0;
	FOR_START_MAP_ONLINE_AES(it)
	{
		ONLINE_AES *pAES = &(it->second);
		serviceID = pAES->GetServiceIDFromDBID(_uID);
		if ( serviceID != 0 )
			break;		
	}
	return serviceID;
}

ucstring	GetShardNameOfAES(const TServiceId &_sid)
{
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(_sid);
	if (it == map_Online_AES.end())
		return L"";
	return	it->second.GetRegShardNameOfHost();
}
string	GetHostNameOfAES(const TServiceId &_sid)
{
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(_sid);
	if (it == map_Online_AES.end())
		return "";

	return it->second.GetRegHostName();
}
SIPNET::TServiceId	GetAESIDOfHostName(std::string _sHostName)
{
	SIPNET::TServiceId sid(0);
	FOR_START_MAP_ONLINE_AES(it)
	{
		ONLINE_AES *pAES = &(it->second);
		if (pAES->GetRegHostName() == _sHostName)
			return pAES->m_ConnectionInfo.m_ServiceID;
	}
	return sid;
}

static	uint32 GetHostIDOfAES(const TServiceId &_sid)
{
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(_sid);
	if (it == map_Online_AES.end())
		return 0;

	return	it->second.m_uID;
}
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// 일반모쥴 ///////////////////////////////////////////
static	void	DisconnectAES(const TServiceId &tsid)
{
	TSockId from;
	CCallbackNetBase *cnb = CUnifiedNetwork::getInstance ()->getNetBase (tsid, from);
	cnb->disconnect(from);
}

static	string	GetInetAddressOfAES(const TServiceId &tsid)
{
	TSockId from;
	CCallbackNetBase	*cnb =	CUnifiedNetwork::getInstance ()->getNetBase (tsid, from);
	const CInetAddress	&ia =	cnb->hostAddress (from);
	return ia.asString();
}

static	string	GetIPAddressOfAES(const TServiceId &tsid)
{
	TSockId from;
	CCallbackNetBase	*cnb =	CUnifiedNetwork::getInstance ()->getNetBase (tsid, from);
	const CInetAddress	&ia =	cnb->hostAddress (from);
	return ia.ipAddress();
}

uint32	SendMessageToAES(ucstring _zone, ucstring _shard, string _host, const SIPNET::CMessage *_pMes)
{
	uint32 uHostID = GetHostIDFromMVName(_host);
	uint32	uSendNum = 0;

	FOR_START_MAP_ONLINE_AES(it)
	{
		ONLINE_AES & aes = it->second;
		uint32		uSvcID		= aes.m_ConnectionInfo.m_ServiceID.get();
		ucstring	curZone		= aes.GetRegZoneNameOfHost();
		ucstring	curShard	= aes.GetRegShardNameOfHost();
		string		curHost		= aes.GetRegHostName();
		TServiceId	curSID		= it->second.m_ConnectionInfo.m_ServiceID;

		if (_zone != L"*" && _zone != curZone)
			continue;
		if (_shard != L"*" && _shard != curShard)
			continue;
		if (uHostID != 0 && uSvcID != uHostID)
			continue;

		CUnifiedNetwork::getInstance ()->send ( curSID, *_pMes );
		uSendNum ++;
	}

	return uSendNum;
}

static	void	SendHostConnection(TServiceId tsid)
{
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(tsid);
	if (it == map_Online_AES.end())
		return;

	ONLINE_AES	info = it->second;
	if ( info.m_bRegister )
	{
		CMessage	mes(M_AD_REGAES_CONNECT);
		mes.serial(info);
		ManageHostServer->send(mes);
	}
	else
	{
		CMessage	mes(M_AD_UNREGAES_CONNECT);
		uint32	uHostID = info.m_uID;
		REGHOSTINFO	& reginfo = map_UNREGHOSTINFO[uHostID];
		mes.serial(reginfo);
		mes.serial(info);
		ManageHostServer->send(mes);
	}

}

static	void	SendHostDisConnection(TServiceId tsid)
{
	uint16	sid = tsid.get();
	CMessage	mes(M_AD_AES_DISCONNECT);
	mes.serial(sid);
	ManageHostServer->send(mes);
}

static	void	SendHostVersionUpdate(uint32 _uID, string _sVersion)
{
	CMessage	mes(M_MH_CHVERSION);
	mes.serial(_uID, _sVersion);
	ManageHostServer->send(mes);
	MonitorServer->send(mes);
}

static	void	SendServiceConnection(uint16 sid, TServiceId tsidAES)
{
	MAP_ONLINE_AES::iterator itAES = map_Online_AES.find(tsidAES);
	if (itAES == map_Online_AES.end())
		return;

	ONLINE_AES	aes		= itAES->second;
	MAP_ONLINESERVICE::iterator it = aes.m_OnlineService.find(sid);
	if (it == aes.m_OnlineService.end())
		return;

	SERVICE_ONLINE_INFO &info = it->second;
	bool bRegister = info.m_bRegDB;
	if ( bRegister )
	{
		CMessage	mes(M_AD_REGSVC_CONNECT);
		mes.serial(info);
		ManageHostServer->send(mes);
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
		ManageHostServer->send(mes);
	}
}

static	void	SendServiceDisconnection(uint16 sid, TServiceId tsidAES)
{
	CMessage	mes(M_AD_SERVICE_DISCONNECT);
	uint16	aesid = tsidAES.get();
	mes.serial(aesid, sid);
	ManageHostServer->send(mes);
}


static	uint32	s_uLastPing = 0;
static void CheckPingPong()
{
	uint32 d = CTime::getSecondsSince1970();

	bool allPonged = true;
	bool haveService = false;

	FOR_START_MAP_ONLINE_AES(it)
	{
		ONLINE_AES *pAES = &(it->second);
		haveService = true;
		if (pAES->m_uLastPing != 0)
		{
			allPonged = false;
			if (d > pAES->m_uLastPing + PingTimeout)
			{
				string sAES = pAES->GetRegHostName();
				sipwarning("Server %s seems dead, no answer for the last %d second", sAES.c_str(), (uint32)PingTimeout);
				pAES->m_uLastPing = 0;

				CMessage	msgout(M_MO_HOSTNOTPING);
				msgout.serial(sAES);
				MonitorServer->send(msgout);
			}
		}
	}

	if (d > s_uLastPing + PingFrequency)
	{
		s_uLastPing = d;
		FOR_START_MAP_ONLINE_AES(it)
		{
			if (it->second.m_uLastPing != 0)
				continue;

			it->second.m_uLastPing = d;
			CMessage msgout(M_ADMIN_ADMIN_PING);
			CUnifiedNetwork::getInstance()->send(it->second.m_ConnectionInfo.m_ServiceID, msgout);

			string sAES = it->second.GetRegHostName();
			sipdebug("send ping to host %s, , time = %d", sAES.c_str(), d);
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Callback모쥴 ///////////////////////////////////////////

// AES가 AS에 써비스로 접속할때의 Callback
static void cbNewAESConnection (const std::string &serviceName, TServiceId tsid, void *arg)
{
	lst_NoConfirmAES.push_back(AES_CONNECTION_INFO(tsid));

	CMessage msgout(M_AD_AES_CONNECT);
	CUnifiedNetwork::getInstance ()->send (tsid, msgout);
}

// AES가 AS와의 접속을 해제할때의 Callback
static void cbNewAESDisconnection (const std::string &serviceName, TServiceId tsid, void *arg)
{
	MAP_ONLINE_AES::iterator it = map_Online_AES.find(tsid);
	if (it == map_Online_AES.end())
		return;

	TServiceId	uHostID = it->second.m_ConnectionInfo.m_ServiceID;
	DeleteHostePort(uHostID.get());
	SendHostDisConnection(uHostID);
	SendHostDisConnectionToMonitor(uHostID);
	
	RemoveOnlineAES(tsid);
	RemoveNoConfirmAES(tsid);
}

// AES가 AS에 소개정보를 보내올때의 Callback
static void cb_M_AD_AES_CONNECT(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	ucstring			ucShardName;
	string				sVersion;
	vector<string>		sHostName;
	uint32				uHostNum;
	msgin.serial( ucShardName, sVersion, uHostNum );

	for (uint32 i = 0; i < uHostNum; i++)
	{
		string	sTemp;
		msgin.serial(sTemp);
		sHostName.push_back(sTemp);
	}

	string strInetAddress = GetInetAddressOfAES(tsid);
	string strAESIPAddress = GetIPAddressOfAES(tsid);
	// 이미 Online된 AES이면
	if (IsOnlineAES(tsid))
	{
		sipwarning ("AES ServiceID(%d) is already connected (IP=%s)", tsid.get(), strInetAddress.c_str ());
		return;
	}

	// 등록된 AES인가 확인
	bool	bRegister = false;
	uint32	uHostID = 0;
	for (uint32 i = 0; i < uHostNum; i++)
	{
		uint32 uTempHostID = GetRegisteredHostID(ucShardName, sHostName[i]);
		if (uTempHostID == 0)
			continue;
		uHostID = uTempHostID;
		bRegister = true;
		break;
	}

	if ( uHostID == 0 )
		if ( !AcceptUnRegService )
		{
			sipwarning ("Disconnect unregistered AES(IP=%s)", strInetAddress.c_str());
			DisconnectAES(tsid);
			return;
		}
		else
		{
			uHostID = static_cast<uint32>(map_UNREGHOSTINFO.size())+1;
			uint32	uShardId = 1;
			REGHOSTINFO		info(uHostID, "UnRegisterAES", strAESIPAddress, uShardId, sVersion);
			map_UNREGHOSTINFO.insert(make_pair(uHostID, info));
			sipinfo("Unregisted AES is connected(IP=%s)(HostID=%d)(TSID=%d)", strAESIPAddress.c_str(), uHostID, tsid.get());
			bRegister = false;
		}

	if (IsOnlineAES(uHostID))
	{
		sipwarning ("AES HostID(%d) is already connected (IP=%s)", uHostID, strInetAddress.c_str ());
		return;
	}

	// NoConfirmAES목록에서 삭제
	RemoveNoConfirmAES(tsid);
	// Online목록에 추가
	map_Online_AES.insert( make_pair ((SIPNET::TServiceId)tsid.get(), ONLINE_AES(uHostID, tsid, bRegister)) );
	sipinfo("AES is connected(IP=%s)(HostID=%d)(TSID=%d)", strAESIPAddress.c_str(), uHostID, tsid.get());

	// Version검사
	bool	bUpdateVersion = UpdateHostVersion(uHostID, sVersion);
	if (bUpdateVersion)
	{
		SendHostVersionUpdate(uHostID, sVersion);
	}

	// 기대관리자들에게 통보
	SendHostConnection(tsid);
	
	// 감시자들에게 통보
	SendHostConnectionToMonitor(tsid);

	// 감시정보를 내려보낸다
	SendMonitorInfoToAES(tsid);

	TRegPortItem	PortAry[] =
	{
		{"TIANGUO_ASManagePort",	PORT_TCP, ManagePort.get(), FILTER_ALL},
		{"TIANGUO_ASMonitorPort",	PORT_TCP, MonitorPort.get(), FILTER_ALL},
		{"TIANGUO_ASPort",			PORT_TCP, ASPort.get(), FILTER_SUBNET},
	};

	SendAdminPortOpen(ManagePort.get(), "ManagePort");
	SendAdminPortOpen(MonitorPort.get(), "MonitorPort");
}

bool	CheckShardConfig(uint32	uMstID, uint16 uShardID, SIPNET::TServiceId sidAES, SIPNET::TServiceId sidSvc)
{
	MAP_MSTSERVICECONFIG::iterator itCfg = map_MSTSERVICECONFIG.find(uMstID);
	if ( itCfg == map_MSTSERVICECONFIG.end() )
	{
		sipwarning("Unknown Service Type id %d - Svc id %d", uMstID, sidSvc.get());
		return false;
	}

	uint8	CfgFlag		= (uint8)itCfg->second.nCfgFlag;
	if ( CfgFlag != CONF_SHARD )
		return	true;
	if ( uShardID == DEFAULT_SHARD_ID )
		return true;

	bool	bCanConnect = false;
	bool	bOnline = false;
	FOR_START_MAP_ONLINE_AES(it)
	{
		bOnline = it->second.IsExistOnlineServiceOfShard(sidSvc.get(), uMstID, uShardID);
		if ( bOnline )
		{
			bCanConnect = false;
			break;
		}
	}
	if ( !bOnline )
		bCanConnect = true;

	return bCanConnect;
}

bool	CheckAdminConfig(std::string sShortName, SIPNET::TServiceId sidAES, SIPNET::TServiceId sidSvc)
{
	uint32	uMstID = GetMSTServiceIDFromSName(sShortName);
	MAP_MSTSERVICECONFIG::iterator itCfg = map_MSTSERVICECONFIG.find(uMstID);
	if ( itCfg == map_MSTSERVICECONFIG.end() )
	{
		sipwarning("Unknown Service Type %s - %d", sShortName.c_str(), sidSvc.get());
		return false;
	}

	uint8	CfgFlag		= (uint8)itCfg->second.nCfgFlag;
	if ( CfgFlag == CONF_SHARD )
		return	true;

	bool	bCanConnect = false;
	switch ( CfgFlag )
	{
	case CONF_ANY:
		{
			bCanConnect = true;
		}
		break;
	case CONF_ADMIN:
		{
			bool	bOnline = false;
			FOR_START_MAP_ONLINE_AES(it)
			{
				bOnline = it->second.IsOnlineMstService(uMstID);
				if ( bOnline )
					break;
			}
			bCanConnect = ! bOnline;
		}
		break;
	case CONF_HOST:
		{
			MAP_ONLINE_AES::iterator it = map_Online_AES.find(sidAES);
			if (it != map_Online_AES.end())
			{
				ONLINE_AES	aes = it->second;
				bool bOnline = aes.IsOnlineMstService(uMstID);
				bCanConnect = ! bOnline;
			}
			else
				bCanConnect = false;

		}
		break;
	default:
		bCanConnect = false;
	}

	return bCanConnect;
}

// AES가 AS에 써비스접속정보를 보내올때
static	void	cb_M_AES_SERVICE_CONNECT(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16	sid = 0;		// AES에 등록된 Service의 ID(접속할때마다 달라진다)
	string	sShortName;
	msgin.serial(sShortName, sid);

	MAP_ONLINE_AES::iterator	it = map_Online_AES.find(tsid);
	if ( it == map_Online_AES.end() )
		return;
	
	TServiceId	svcID;	svcID.set(sid);
	if ( !CheckAdminConfig(sShortName, tsid, svcID) )
	{
		sipwarning("Disconnect Service %s - %d, Because it's unavailable for AdminConfigSystem", sShortName.c_str(), sid);
		CMessage msgout(M_AES_DISCONNECT_SERVICE);
		msgout.serial(sShortName, sid);
		CUnifiedNetwork::getInstance ()->send ( tsid, msgout );

		return;
	}

	bool	bRegister = it->second.AddOnlineService(sid, sShortName);
	if ( !bRegister && !AcceptUnRegService.get() ) // disconnect
	{
		sipwarning("Disconnect Register new service %s - %d, Because of full registered service! Try register", sShortName.c_str(), sid);
		CMessage msgout(M_AES_DISCONNECT_SERVICE);
		msgout.serial(sShortName, sid);
		CUnifiedNetwork::getInstance ()->send ( tsid, msgout );

		return;
	}

	uint32	uHostID = it->second.m_uID;
	SendServiceConnection(sid, tsid);
	SendServiceConnectionToMonitor(sid, tsid);
}

static bool		isAlreadyRegPort(REGPORTINFO & _info)
{
	bool	bExist = false;
	MAP_REGPORT::iterator	it;
	for ( it = map_REGPORT.begin(); it != map_REGPORT.end(); it ++ )
	{
		REGPORTINFO		info = it->second;
		if ( info._sPortAlias == _info._sPortAlias &&
			info._uSvcID == _info._uSvcID &&
			info._uAESSvcID == _info._uAESSvcID )
		{
			bExist = true;
			break;
		}
	}

	return bExist;
}

static void		cb_M_AD_SERVICE_IDENTIFY(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32			nSize;			msgin.serial(nSize);
	for( uint32 i = 0; i < nSize; i ++ )
	{
		uint16			sid;			msgin.serial(sid);
		TREGPORTARY		aryRegPort;		msgin.serialCont(aryRegPort);

		MAP_ONLINE_AES::iterator	itAES	= map_Online_AES.find(tsid);
		if ( itAES == map_Online_AES.end() )
			return;

		static	uint32 g_regPortCount = 0;

		uint32	nCount	= static_cast<uint32>(aryRegPort.size());
		for ( uint j = 0; j < nCount; j ++ )
		{
			TRegPortItem	item = aryRegPort[j];

			REGPORTINFO		info(tsid.get(), sid, item);
			if ( isAlreadyRegPort(info) )
				continue;

			map_REGPORT.insert(std::make_pair(g_regPortCount ++, info));
			sipinfo("RegPort : All Count %d : alias %S, port %d", g_regPortCount, info._sPortAlias.c_str(), info._uPort);
		}
	}

	Send_M_MH_IDENT();
}

// AES가 AS에 써비스접속차단정보를 보내올때
static	void	cb_M_AES_SERVICE_DISCONNECT(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16	sid = 0;
	string	sShortName;
	msgin.serial(sShortName, sid);

	sipwarning("Service Disconnected. ServiceName=%s, sid=%d", sShortName.c_str(), sid);

	{
		string strLog = "Service Disconnected. ServiceName=" + sShortName;
		MAKEQUERY_SERVER_CRASH_LOG(strQ, strLog);
		DBMainCaller->execute(strQ);
	}

	MAP_ONLINE_AES::iterator	it = map_Online_AES.find(tsid);
	if (it == map_Online_AES.end())
		return;

	uint32	uServiceID = it->second.DelOnlineService(sid);

	SendServiceDisconnection(sid, tsid);
	SendServiceDisConnectionToMonitor(sid, tsid);
}

static void cbAdminPong(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	FOR_START_MAP_ONLINE_AES(it)
	{
		ONLINE_AES *pAES = &(it->second);
		if(pAES->m_ConnectionInfo.m_ServiceID == tsid)
		{
			string sAES = pAES->GetRegHostName();
			sipdebug("receive pong from %s", sAES.c_str());
			pAES->m_uLastPing = 0;
			return;
		}
	}
}

static void cbAdminShardID(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	MAP_ONLINE_AES::iterator	itAES = map_Online_AES.find(tsid);
	if ( itAES == map_Online_AES.end() )
		return;

	uint32	nSize;				msgin.serial(nSize);
	for ( uint32 i = 0; i < nSize; i ++ )
	{
		uint16	uOnSvcID;
		uint32	uShardID;
		msgin.serial(uOnSvcID, uShardID);
		SERVICE_ONLINE_INFO	& service = itAES->second.GetOnlineService(uOnSvcID);
		service.m_ShardID = uShardID;

		uint32	uMstSvcID;
		if ( service.m_bRegDB )
			uMstSvcID = map_REGSERVICEINFO[service.m_RegID].m_uMstSvcID;
		else
			uMstSvcID = map_UNREGSERVICEINFO[service.m_RegID].m_uMstSvcID;

		std::string sShortName = map_MSTSERVICECONFIG[uMstSvcID].sShortName;
		TServiceId	sid;	sid.set(uOnSvcID);
		if ( !CheckShardConfig(uMstSvcID, (uint16)uShardID, tsid, sid) )
		{
			sipwarning("Disconnect Service %s - %d, Because it's unavailable for ShardConfigSystem", sShortName.c_str(), uOnSvcID);
			CMessage msgout(M_AES_DISCONNECT_SERVICE);
			msgout.serial(sShortName, sid);
			CUnifiedNetwork::getInstance ()->send ( tsid, msgout );

			return;
		}
	}
}

// packet : M_AES_REG_SVCS
// AES -> AS
static void cb_M_AES_REG_SVCS(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32 size;		msgin.serial(size);
	for( uint i = 0; i < size; i ++ )
	{
		std::string	strAlias;	msgin.serial(strAlias);
		std::string	strExe;		msgin.serial(strExe);
		
		FOR_START_MAP_REGSERVICEINFO(it)
		{
			ucstring	ucSvcName = GetMSTServiceAliasNameOfService(it->second.m_uID);
			if ( ! strcmp(ucSvcName.toString().c_str(), strAlias.c_str()) )
				it->second.m_sSvcExe = strExe;
		}
	}
}


static	TUnifiedCallbackItem CallbackArray[] =
{
	{ M_AD_AES_CONNECT,			cb_M_AD_AES_CONNECT },
	{ M_AES_SERVICE_CONNECT,	cb_M_AES_SERVICE_CONNECT },
	{ M_AD_SERVICE_IDENTIFY,	cb_M_AD_SERVICE_IDENTIFY},
	{ M_AES_SERVICE_DISCONNECT,	cb_M_AES_SERVICE_DISCONNECT },
	{ M_ADMIN_GRAPH_UPDATE,		cbGraphUpdate },
	{ M_AES_SERVICE_NOTPONG,	cbServiceNotPong },
	{ M_ADMIN_ADMIN_PONG,		cbAdminPong },
	{ M_ADMIN_SHARDID,			cbAdminShardID },
	{ M_AES_REG_SVCS,			cb_M_AES_REG_SVCS },
};

void	Make_ONLINEINFO(CMessage *pMes)
{
	uint32	uOnlineHostNum = static_cast<uint32>(map_Online_AES.size());
	pMes->serial(uOnlineHostNum);
	FOR_START_MAP_ONLINE_AES(it)
	{
		ONLINE_AES	& info = it->second;
		pMes->serial(info);

		uint32	uOnlineServiceNum = static_cast<uint32>(it->second.m_OnlineService.size());
		pMes->serial(uOnlineServiceNum );
		MAP_ONLINESERVICE::iterator its;
		for (its = it->second.m_OnlineService.begin(); its != it->second.m_OnlineService.end(); its++)
		{
			SERVICE_ONLINE_INFO		& info = its->second;
			pMes->serial(info);
		}
	}
}

static void cb_M_MH_ONLINEINFO( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	CMessage	mes(M_MH_ONLINEINFO);
	Make_ONLINEINFO(&mes);
	ManageHostServer->send(mes, from);

	Send_M_MH_IDENT();
}

static TCallbackItem MHCallbackArray[] =
{
	{ M_MH_ONLINEINFO,		cb_M_MH_ONLINEINFO },
};

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// extern 모쥴 ///////////////////////////////////////////

// 초기화
void	InitManageAESConnection()
{
	CUnifiedNetwork::getInstance ()->addCallbackArray(CallbackArray, sizeof(CallbackArray)/sizeof(CallbackArray[0]));
	CUnifiedNetwork::getInstance ()->setServiceUpCallback (AES_S_NAME, cbNewAESConnection);
	CUnifiedNetwork::getInstance ()->setServiceDownCallback (AES_S_NAME, cbNewAESDisconnection);

	ManageHostServer->addCallbackArray (MHCallbackArray, sizeof(MHCallbackArray)/sizeof(TCallbackItem));
}

// Update
void	UpdateManageAESConnection()
{
	// 일정한 시간동안 인증이 되지 않은 AES들은 접속 차단
	TTime curTime = CTime::getLocalTime();

	FOR_START_LST_NOCONFIRMAES(it)	
	{
		TTime deltaTime = curTime - (*it).m_ConnectionTime;
		if (  deltaTime > 20000)	// 20초
		{
			DisconnectAES( (*it).m_ServiceID );
			lst_NoConfirmAES.erase(it);
			break;	
		}
	}

	CheckPingPong();
}
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

SIPBASE_CATEGORISED_COMMAND(AS, onlineHosts, "display information about all online hosts", "")
{
	int nSize = static_cast<int>(map_Online_AES.size());
	sipinfo("Number of OnlineHosts %d", nSize);

	FOR_START_MAP_ONLINE_AES(it)
	{
		std::string		sHostName	= it->second.GetRegHostName();
		bool			bRegister	= it->second.m_bRegister;
		uint16			uSvcID		= it->second.m_ConnectionInfo.m_ServiceID.get();
		
		sipdebug("Host %s Register %d SvcId %d", sHostName.c_str(), bRegister, uSvcID);
	}
	
	return true;
}

