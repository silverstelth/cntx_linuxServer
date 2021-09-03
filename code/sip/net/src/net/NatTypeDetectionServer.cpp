/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/NatTypeDetectionServer.h"

using namespace std;
using namespace SIPBASE;

namespace	SIPNET	
{
#define		PLUGINNAME_NATTYPEDETECTIONSERVER	"PI_NTDS"

void	cbMessage(CMessage& msgin, TSockId from, CCallbackNetBase& servercb)
{
	ICallbackNetPlugin* pPlugin = servercb.findPlugin(PLUGINNAME_NATTYPEDETECTIONSERVER);
	if (!pPlugin)
		return;

	CNatTypeDetectionServer* pNTDS = (CNatTypeDetectionServer*)pPlugin;

	if (msgin.getName() == M_NAT_CS_REQUESTNATHAVE)
	{
		pNTDS->cbM_NAT_CS_REQUESTNATHAVE(msgin, from);
		return;
	}

	if (msgin.getName() == M_NAT_CS_COMPLETEDETECTION)
	{
		pNTDS->cbM_NAT_CS_COMPLETEDETECTION(msgin, from);
		return;
	}
}

static TCallbackItem	NTDSCallbackAry[] = 
{
	{ M_NAT_CS_REQUESTNATHAVE,		cbMessage },
	{ M_NAT_CS_COMPLETEDETECTION,	cbMessage },
};

void	cbUDP1Message(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg)
{
	CNatTypeDetectionServer *pServer = (CNatTypeDetectionServer *)pArg;

	if (msgin.getName() == M_NAT_CS_REQNATTYPE1)
	{
		pServer->cbM_NAT_CS_REQNATTYPE1(msgin, remoteaddr);
		return;
	}
	if (msgin.getName() == M_NAT_CS_REQCONETYPE)
	{
		pServer->cbM_NAT_CS_REQCONETYPE(msgin, remoteaddr);
		return;
	}
}

static TUDPMsgCallbackItem	NTDSUDP1CallbackAry[] =
{
	{ M_NAT_CS_REQNATTYPE1,	cbUDP1Message, NULL },
	{ M_NAT_CS_REQCONETYPE,	cbUDP1Message, NULL },
};

void	cbUDP2Message(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg)
{
	CNatTypeDetectionServer *pServer = (CNatTypeDetectionServer *)pArg;
	if (msgin.getName() == M_NAT_CS_REQNATTYPE2)
	{
		pServer->cbM_NAT_CS_REQNATTYPE2(msgin, remoteaddr);
		return;
	}
}

static TUDPMsgCallbackItem	NTDSUDP2CallbackAry[] =
{
	{ M_NAT_CS_REQNATTYPE2,	cbUDP2Message, NULL },
};
//////////////////////////////////////////////////////////////////////////
CNatTypeDetectionServer::CNatTypeDetectionServer() :
	ICallbackNetPlugin(PLUGINNAME_NATTYPEDETECTIONSERVER)
{
	m_pBaseServer		= NULL;
	m_UDPSock1			= NULL;
	m_UDPSock2			= NULL;
	m_PortConeUDPSock	= NULL;
}

CNatTypeDetectionServer::~CNatTypeDetectionServer()
{
	delete	m_UDPSock1;
	delete	m_UDPSock2;
	delete	m_PortConeUDPSock;
}

void	CNatTypeDetectionServer::Init(CCallbackServer* pServer, uint16 udpPort1, uint16 udpPort2, uint16 portconePort)
{
	if(m_pBaseServer != NULL)
		siperrornoex("This is second initialization of CNatTypeDetectionServer.");

	m_pBaseServer		= pServer;

	m_UDPSock1 = new CCallbackUdp(false);			m_UDPSock1->bind(udpPort1);
	m_UDPSock2 = new CCallbackUdp(false);			m_UDPSock2->bind(udpPort2);
	m_PortConeUDPSock = new CCallbackUdp(false);	m_PortConeUDPSock->bind(portconePort);

	m_pBaseServer->addPlugin(PLUGINNAME_NATTYPEDETECTIONSERVER, this);
	m_pBaseServer->addCallbackArray(NTDSCallbackAry, sizeof(NTDSCallbackAry)/sizeof(NTDSCallbackAry[0]));
	m_UDPSock1->addCallbackArray(NTDSUDP1CallbackAry, sizeof(NTDSUDP1CallbackAry)/sizeof(NTDSUDP1CallbackAry[0]), this);
	m_UDPSock2->addCallbackArray(NTDSUDP2CallbackAry, sizeof(NTDSUDP2CallbackAry)/sizeof(NTDSUDP2CallbackAry[0]), this);

	sipdebug("NTDS: NatTypeDetectionServer created : UDPPort1=%d, UDPPort2=%d, PortConeUDPPort=%d", udpPort1, udpPort2, portconePort);
}

void	CNatTypeDetectionServer::DisconnectionCallback(TTcpSockId from)
{
	RemoveNatClient(from);
}

void	CNatTypeDetectionServer::DestroyCallback()
{

}

void	CNatTypeDetectionServer::RemoveNatClient(TTcpSockId from)
{
	map<TTcpSockId, CNATClient>::iterator it = m_mapClientForSockt.find(from);
	if (it != m_mapClientForSockt.end())
	{
		uint32 clientGUID = it->second.m_ClientGUID;
		m_mapClientForSockt.erase(from);
		m_mapClientForGUID.erase(clientGUID);
	}
}

void	CNatTypeDetectionServer::Update()
{
	m_UDPSock1->update();
	m_UDPSock2->update();
}

static	uint32	CandiClientGUID = 1;
void	CNatTypeDetectionServer::cbM_NAT_CS_REQUESTNATHAVE(CMessage& msgin, TTcpSockId from)
{
	string	strClientPrivateIP;
	msgin.serial(strClientPrivateIP);

	string	strClientPublicIP = from->getTcpSock()->remoteAddr().ipAddress();

	bool		bHaveNat		= (strClientPrivateIP == strClientPublicIP) ? false : true;
	uint32		natClientGUID	= CandiClientGUID++;
	uint16		nUDPPort1		= m_UDPSock1->localAddr().port();
	uint16		nUDPPort2		= m_UDPSock2->localAddr().port();
	uint16		nPortConeUDPPort= m_PortConeUDPSock->localAddr().port();

	CMessage	msgOut(M_NAT_SC_REQUESTNATHAVE);
	msgOut.serial(bHaveNat, natClientGUID, nUDPPort1, nUDPPort2, nPortConeUDPPort);
	m_pBaseServer->send(msgOut, from);

	if (bHaveNat)
	{
		m_mapClientForSockt.insert(make_pair(from, CNATClient(natClientGUID, from, strClientPrivateIP, strClientPublicIP)));
		m_mapClientForGUID.insert(make_pair(natClientGUID, CNATClient(natClientGUID, from, strClientPrivateIP, strClientPublicIP)));
	}
	sipdebug("NTDS: new NatDetectionClient : NAT=%d, GUID=%d, PrivateIP=%s, PublicIP=%s", bHaveNat, natClientGUID, strClientPrivateIP.c_str(), strClientPublicIP.c_str());
}

void	CNatTypeDetectionServer::cbM_NAT_CS_COMPLETEDETECTION(CMessage& msgin, TTcpSockId from)
{
	uint32	nNatType;
	msgin.serial(nNatType);

	map<TTcpSockId, CNATClient>::iterator it = m_mapClientForSockt.find(from);
	if (it != m_mapClientForSockt.end())
	{
		CNATClient* client = &(it->second);
		sipdebug("NTDS: Complete NatDetection : GUID=%d, NATType=%d", client->m_ClientGUID, nNatType);
		RemoveNatClient(from);
	}
}

CNATClient*	CNatTypeDetectionServer::ConfirmUDPMessage(const string &msgName, uint32 clientGUID, const CInetAddress& msgUDPPrivateAddr, const CInetAddress& msgUDPPublicAddr)
{
	map<uint32, CNATClient>::iterator it = m_mapClientForGUID.find(clientGUID);
	if (it == m_mapClientForGUID.end())
	{
		sipwarning("NTDS: Invalid UDPMsg(%s)(No GUID) : GUID=%d, PrivateAddr=%s, PublicAddr=%s",
			msgName.c_str(), clientGUID, msgUDPPrivateAddr.asIPString().c_str(), msgUDPPublicAddr.asIPString().c_str());
		return NULL;
	}

	CNATClient* client = &(it->second);
	if (client->m_privateIP != msgUDPPrivateAddr.ipAddress() ||
		client->m_publicIP != msgUDPPublicAddr.ipAddress())
	{
		sipwarning("NTDS: Invalid UDPMsg(%s)(mismatch IP address) : GUID=%d, RegPrivateIP=%s, RegPublicIP=%s, msgPrivateIP=%s, msgPublicIP=%s",
			msgName.c_str(), clientGUID, client->m_privateIP.c_str(), client->m_publicIP.c_str(), msgUDPPrivateAddr.asIPString().c_str(), msgUDPPublicAddr.asIPString().c_str());
		return NULL;
	}
	return client;
}

#define CONFIRMUDPMESSAGE(msg, remoteaddr)	\
	uint32	clientGUID;						\
	string	strClientUDPPrivateAddr;		\
	msg.serial(clientGUID, strClientUDPPrivateAddr);	\
	CInetAddress	msgUDPPrivateAddr(strClientUDPPrivateAddr);	\
	CInetAddress	msgUDPPublicAddr(remoteaddr);	\
	CNATClient* pNatClient = ConfirmUDPMessage(msg.getNameAsString(), clientGUID, msgUDPPrivateAddr, msgUDPPublicAddr);	\
	if (!pNatClient)	\
		return;		\

void	CNatTypeDetectionServer::cbM_NAT_CS_REQNATTYPE1(CMessage& msgin, CInetAddress& remoteaddr)
{
	CONFIRMUDPMESSAGE(msgin, remoteaddr);

	pNatClient->m_publicUDPportToUDP1 = msgUDPPublicAddr.port();
	if (pNatClient->m_publicUDPportToUDP1 != 0 && pNatClient->m_publicUDPportToUDP2 != 0)
	{
		CMessage msgNatType(M_NAT_SC_REQNATTYPE);
		bool	bSymmetricNAT;
		if (pNatClient->m_publicUDPportToUDP1 == pNatClient->m_publicUDPportToUDP2)	// is cone nat
			bSymmetricNAT = false;
		else	// is symmetric nat
			bSymmetricNAT = true;
		msgNatType.serial(bSymmetricNAT);
		m_pBaseServer->send(msgNatType, pNatClient->m_MainSocket);

		if (bSymmetricNAT)
		{
			sipdebug("NTDS: Nat of GUID(%d) is  SymmetricNAT, publicUDPPort1=%d, publicUDPPort2=%d", pNatClient->m_ClientGUID, pNatClient->m_publicUDPportToUDP1, pNatClient->m_publicUDPportToUDP2);
		}
		else
		{
			sipdebug("NTDS: Nat of GUID(%d) is  ConeNAT", pNatClient->m_ClientGUID);
		}

		pNatClient->m_publicUDPportToUDP1 = 0;
		pNatClient->m_publicUDPportToUDP2 = 0;
	}
}

void	CNatTypeDetectionServer::cbM_NAT_CS_REQCONETYPE(CMessage& msgin, CInetAddress& remoteaddr)
{
	CONFIRMUDPMESSAGE(msgin, remoteaddr);

	CMessage	msgOut(M_NAT_SC_REQCONETYPE);
	msgOut.serial(pNatClient->m_ClientGUID);

	//m_UDPSock1->sendTo(msgOut.buffer(), msgOut.length(), remoteaddr);
	m_PortConeUDPSock->sendTo(msgOut.buffer(), msgOut.length(), remoteaddr);
}

void	CNatTypeDetectionServer::cbM_NAT_CS_REQNATTYPE2(CMessage& msgin, CInetAddress& remoteaddr)
{
	CONFIRMUDPMESSAGE(msgin, remoteaddr);
	pNatClient->m_publicUDPportToUDP2 = msgUDPPublicAddr.port();
	if (pNatClient->m_publicUDPportToUDP1 != 0 && pNatClient->m_publicUDPportToUDP2 != 0)
	{
		CMessage msgNatType(M_NAT_SC_REQNATTYPE);
		bool	bSymmetricNAT;
		if (pNatClient->m_publicUDPportToUDP1 == pNatClient->m_publicUDPportToUDP2)	// is cone nat
			bSymmetricNAT = false;
		else	// is symmetric nat
			bSymmetricNAT = true;
		msgNatType.serial(bSymmetricNAT);
		m_pBaseServer->send(msgNatType, pNatClient->m_MainSocket);

		if (bSymmetricNAT)
		{
			sipdebug("NTDS: Nat of GUID(%d) is  SymmetricNAT, publicUDPPort1=%d, publicUDPPort2=%d", pNatClient->m_ClientGUID, pNatClient->m_publicUDPportToUDP1, pNatClient->m_publicUDPportToUDP2);
		}
		else
		{
			sipdebug("NTDS: Nat of GUID(%d) is  ConeNAT", pNatClient->m_ClientGUID);
		}
		pNatClient->m_publicUDPportToUDP1 = 0;
		pNatClient->m_publicUDPportToUDP2 = 0;
	}
}

} // namespace	SIPNET
