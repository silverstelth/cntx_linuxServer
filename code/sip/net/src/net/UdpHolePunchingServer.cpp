/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/UdpHolePunchingServer.h"

using namespace std;
using namespace SIPBASE;

namespace	SIPNET	
{
#define		PLUGINNAME_HOLEPUNCHINGSERVER	"PI_UHPS"
#ifdef SIP_OS_UNIX
void	cbMessage(CMessage& msgin, TSockId from, CCallbackNetBase& servercb)
#else
static	void	cbMessage(CMessage& msgin, TSockId from, CCallbackNetBase& servercb)
#endif
{
	ICallbackNetPlugin* pPlugin = servercb.findPlugin(PLUGINNAME_HOLEPUNCHINGSERVER);
	if (!pPlugin)
		return;

	CUdpHolePunchingServer* pUHPS = (CUdpHolePunchingServer*)pPlugin;

	if (msgin.getName() == M_UHP_CS_CLIENTREGISTER)
	{
		pUHPS->cbM_UHP_CS_CLIENTREGISTER(msgin, from);
		return;
	}
	if (msgin.getName() == M_UHP_SC_CONNECTPRIVATE)
	{
		pUHPS->cbM_UHP_SC_CONNECTPRIVATE(msgin, from);
		return;
	}
	if (msgin.getName() == M_UHP_SC_CONNECTPUBLIC)
	{
		pUHPS->cbM_UHP_SC_CONNECTPUBLIC(msgin, from);
		return;
	}
	if (msgin.getName() == M_UHP_CS_HOLEPUNCHINGEND)
	{
		pUHPS->cbM_UHP_CS_HOLEPUNCHINGEND(msgin, from);
		return;
	}
	if (msgin.getName() == M_UHP_CS_HOLEPUNCHINGSUCCESS)
	{
		pUHPS->cbM_UHP_CS_HOLEPUNCHINGSUCCESS(msgin, from);
		return;
	}
	
}

static TCallbackItem	UHPSCallbackAry[] = 
{
	{ M_UHP_CS_CLIENTREGISTER,		cbMessage },
	{ M_UHP_SC_CONNECTPRIVATE,		cbMessage },
	{ M_UHP_SC_CONNECTPUBLIC,		cbMessage },
	{ M_UHP_CS_HOLEPUNCHINGEND,		cbMessage },
	{ M_UHP_CS_HOLEPUNCHINGSUCCESS,	cbMessage },
	
};

#ifdef SIP_OS_UNIX
void	cbUDPMessage(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg)
#else
static	void	cbUDPMessage(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg)
#endif
{
	CUdpHolePunchingServer *pServer = (CUdpHolePunchingServer *)pArg;

	if (msgin.getName() == M_UHP_CS_UDPPORTEXCHANGE)
	{
		pServer->cbM_UHP_CS_UDPPORTEXCHANGE(msgin, remoteaddr);
		return;
	}
}

static TUDPMsgCallbackItem	UHPSUDPCallbackAry[] =
{
	{ M_UHP_CS_UDPPORTEXCHANGE,	cbUDPMessage, NULL },
};

//////////////////////////////////////////////////////////////////////////
CUdpHolePunchingServer::CUdpHolePunchingServer() :
	ICallbackNetPlugin(PLUGINNAME_HOLEPUNCHINGSERVER)
{
	m_pBaseServer		= NULL;
	m_UDPServer			= NULL;
	m_cbRequestGUID		= NULL;
}

CUdpHolePunchingServer::~CUdpHolePunchingServer()
{
	delete	m_UDPServer;
}

void	CUdpHolePunchingServer::Init(CCallbackServer* pServer, uint16 udpPort, std::string strRelayAddress, TRequestGUIDCallback cbRequestGUID)
{
	if(m_pBaseServer != NULL)
		siperrornoex("This is second initialization of CUdpHolePunchingServer.");

	m_pBaseServer		= pServer;
	m_cbRequestGUID		= cbRequestGUID;
	m_strRelayAddress	= strRelayAddress;
	delete m_UDPServer;
	m_UDPServer = new CCallbackUdp(true);
	m_UDPServer->bind(udpPort);

	m_pBaseServer->addPlugin(GetPluginName(), this);
	m_pBaseServer->addCallbackArray(UHPSCallbackAry, sizeof(UHPSCallbackAry)/sizeof(UHPSCallbackAry[0]));
	m_UDPServer->addCallbackArray(UHPSUDPCallbackAry, sizeof(UHPSUDPCallbackAry)/sizeof(UHPSUDPCallbackAry[0]), this);
	
	sipdebug("UHPS: UdpHolePunchingServer created : UDPPort=%d", udpPort);
}

void	CUdpHolePunchingServer::Update()
{
	if (m_UDPServer)
		m_UDPServer->update();
}

void	CUdpHolePunchingServer::DisconnectionCallback(TTcpSockId from)
{
	RemoveHPClient(from);
}

void	CUdpHolePunchingServer::RemoveHPClient(TTcpSockId from)
{
	CHolePunchingClient* client = GetClientFromSock(from);
	if (client == NULL)
		return;

	uint32	TargetID = client->m_TargetGUID;
	map<TTcpSockId, uint32>::iterator it = m_mapClientForSockt.find(from);
	if (it != m_mapClientForSockt.end())
	{
		uint32 clientGUID = it->second;
		sipdebug("UHPS: Remove hole punching: GUID=%d, TargetGUID=%d", client->m_ClientGUID, client->m_TargetGUID);

		m_mapClientForSockt.erase(from);
		m_mapClientForGUID.erase(clientGUID);

		client = NULL;
	}

	map<uint32, CHolePunchingClient>::iterator it2 = m_mapClientForGUID.find(TargetID);
	if (it2 != m_mapClientForGUID.end())
	{
		CMessage	msg(M_UHP_SC_TARGETEXIT);
		m_pBaseServer->send(msg, it2->second.m_MainSocket);
	}
}

CHolePunchingClient*	CUdpHolePunchingServer::GetClientFromSock(TTcpSockId from)
{
	map<TTcpSockId, uint32>::iterator it = m_mapClientForSockt.find(from);
	if (it == m_mapClientForSockt.end())
		return NULL;

	map<uint32, CHolePunchingClient>::iterator it2 = m_mapClientForGUID.find(it->second);
	if (it2 == m_mapClientForGUID.end())
		return NULL;
	return &(it2->second);
}

void	CUdpHolePunchingServer::cbM_UHP_CS_CLIENTREGISTER(CMessage& msgin, TTcpSockId from)
{
	uint32		ClientGUID;
	string	strClientPrivateIP;
	msgin.serial(ClientGUID, strClientPrivateIP);

	// 이소켓에 해당한 client id를 검증할수 있으면 그것을 쓰고 검증할수 없으면 클라이언트에서 올라온 값을 그대로 사용한다
	if (m_cbRequestGUID)
	{
		int tempGUID = m_cbRequestGUID(from);
		if (tempGUID != ClientGUID)
			return;
	}

	bool		bSuccess	= true;
	uint16		nUDPPort	= m_UDPServer->localAddr().port();
	
	// 이미 holepunching중인가를 검사한다
	std::map<TTcpSockId, uint32>::iterator it = m_mapClientForSockt.find(from);
	if (it != m_mapClientForSockt.end())
	{
		bSuccess = false;
		sipwarning("UHPS: There is double hole punching registering");
	}

	CMessage	msgOut(M_UHP_SC_CLIENTREGISTER);
	msgOut.serial(bSuccess, ClientGUID, nUDPPort, m_strRelayAddress);
	m_pBaseServer->send(msgOut, from);

	string	strClientPublicIP = from->getTcpSock()->remoteAddr().ipAddress();
	if (bSuccess)
	{
		m_mapClientForSockt.insert(make_pair(from, ClientGUID));
		m_mapClientForGUID.insert(make_pair(ClientGUID, CHolePunchingClient(ClientGUID, from, strClientPrivateIP, strClientPublicIP)));
		sipdebug("UHPS: new hole punching registered: GUID=%d, PrivateIP=%s, PublicIP=%s", ClientGUID, strClientPrivateIP.c_str(), strClientPublicIP.c_str());
	}
}

void	CUdpHolePunchingServer::cbM_UHP_SC_CONNECTPRIVATE(CMessage& msgin, TTcpSockId from)
{	
	CHolePunchingClient* client = GetClientFromSock(from);
	if (client == NULL)
		return;

	std::map<uint32, CHolePunchingClient>::iterator it2 = m_mapClientForGUID.find(client->m_TargetGUID);
	if (it2 != m_mapClientForGUID.end())
	{
		msgin.invert();
		m_pBaseServer->send(msgin, it2->second.m_MainSocket);
	}
}

void	CUdpHolePunchingServer::cbM_UHP_SC_CONNECTPUBLIC(CMessage& msgin, TTcpSockId from)
{
	CHolePunchingClient* client = GetClientFromSock(from);
	if (client == NULL)
		return;
	std::map<uint32, CHolePunchingClient>::iterator it2 = m_mapClientForGUID.find(client->m_TargetGUID);
	if (it2 != m_mapClientForGUID.end())
	{
		msgin.invert();
		m_pBaseServer->send(msgin, it2->second.m_MainSocket);
	}
}

void	CUdpHolePunchingServer::cbM_UHP_CS_HOLEPUNCHINGEND(CMessage& msgin, TTcpSockId from)
{
	RemoveHPClient(from);
}

void	CUdpHolePunchingServer::cbM_UHP_CS_HOLEPUNCHINGSUCCESS(CMessage& msgin, TTcpSockId from)
{
	CHolePunchingClient* client = GetClientFromSock(from);
	if (client == NULL)
		return;

	string	strHolePunchedAddr;
	msgin.serial(strHolePunchedAddr);
	sipdebug("UHPS: Hole punching is success: GUID=%d, TargetGUID=%d, HolePunched address=%s", client->m_ClientGUID, client->m_TargetGUID, strHolePunchedAddr.c_str());
}

void	CUdpHolePunchingServer::cbM_UHP_CS_UDPPORTEXCHANGE(CMessage& msgin, CInetAddress& remoteaddr)
{
	uint32	nTargetClientGUID;
	uint32	nOwnClientGUID;
	string	strOwnPrivateAddr;

	msgin.serial(nTargetClientGUID, nOwnClientGUID, strOwnPrivateAddr);
	
	CInetAddress	OwnPrivateNetAddr(strOwnPrivateAddr);
	CInetAddress	OwnPublicNetAddr(remoteaddr);
	
	std::map<uint32, CHolePunchingClient>::iterator client1 = m_mapClientForGUID.find(nOwnClientGUID);
	if (client1 == m_mapClientForGUID.end())
	{
		sipwarning("UHPS: There is unregistered client's udp exchange: OwnGUID=%d, TargetGUID=%d", nOwnClientGUID, nTargetClientGUID);
		return;
	}

	if (client1->second.m_privateIP != OwnPrivateNetAddr.ipAddress())
	{
		sipwarning("UHPS: There is mismatched client's address: OwnGUID=%d, TargetGUID=%d", nOwnClientGUID, nTargetClientGUID);
		return;
	}
	client1->second.m_privateUDPport = OwnPrivateNetAddr.port();
	client1->second.m_publicUDPIP = OwnPublicNetAddr.ipAddress();
	client1->second.m_publicUDPport	 = OwnPublicNetAddr.port();
	client1->second.m_TargetGUID = nTargetClientGUID;

	std::map<uint32, CHolePunchingClient>::iterator client2 = m_mapClientForGUID.find(nTargetClientGUID);
	if (client2 == m_mapClientForGUID.end())
		return;
	if (client2->second.m_privateUDPport != 0 &&
		client2->second.m_publicUDPport != 0)
	{
		uint32		TargetHPClientGUID = nTargetClientGUID;
		string		TargetPrivateNetAddr = client2->second.GetPrivateAddr().asIPString();
		string		TargetRegisteredPublicNetAddr = client2->second.GetPublicAddr().asIPString();
		string		OwnRegisteredPublicNetAddr = client1->second.GetPublicAddr().asIPString();

		CMessage	msg(M_UHP_SC_UDPPORTEXCHANGE);
		msg.serial(TargetHPClientGUID, TargetPrivateNetAddr, TargetRegisteredPublicNetAddr, OwnRegisteredPublicNetAddr);
		m_pBaseServer->send(msg, client1->second.m_MainSocket);
		sipdebug("UHPS: UDP exchange: OwnGUID=%d, TargetGUID=%d", nOwnClientGUID, nTargetClientGUID);
	}
}

} // namespace	SIPNET
