/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/P2P.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

const int	P2P_PORT = 9500;

namespace SIPP2P	
{

#define		PLUGINNAME_P2PCLIENT	"PI_P2PCLIENT"

CP2P::CP2P(void) :
	ICallbackNetPlugin(PLUGINNAME_P2PCLIENT),
	m_BaseClient(NULL),
	m_HolePunchingClient(NULL),
	m_peerid(0),
	m_partner_id(0),
	m_cbPeerConnectCallback(NULL),
	m_cbPeerDisconnectCallback(NULL),
	m_cbRecv(NULL),
	m_cbRecvParam(NULL),
	m_ConnectedUDPSock("CP2P::m_ConnectedUDPSock"),
	m_UdpRelay(NULL)
{
	SetState(P2P_NONE);
	m_tmLastUdpSend = 0;

	CSynchronized<CCallbackUdp*>::CAccessor connectedudp( &m_ConnectedUDPSock );
	connectedudp.value() = NULL;

}

CP2P::~CP2P(void)
{
	release();
}

bool	CP2P::init(CCallbackClient* broker_client)
{
	//broker client is already connected to broker server
	sipassert(broker_client);
	m_BaseClient = broker_client;
	m_BaseClient->addPlugin(GetPluginName(), this);
	SetState(P2P_NONE);
	return true;
}

void	CP2P::registerPeer()
{
}

void	CP2P::unregisterPeer()
{
}

void	CP2P::release()
{
	delete m_HolePunchingClient;
	m_HolePunchingClient = NULL;

	if (m_BaseClient)
		m_BaseClient->removePlugin(GetPluginName());

	delete m_UdpRelay;
	m_UdpRelay = NULL;

	SetState(P2P_NONE);
}

#ifdef SIP_OS_UNIX
void HolePunchingEnd(EHolePunchingResult _HolePunchingResult, CCallbackUdp* _pUdpSock, CInetAddress _ConnectedAddr, CInetAddress _RelayAddr, void* _pArg)
#else
static void HolePunchingEnd(EHolePunchingResult _HolePunchingResult, CCallbackUdp* _pUdpSock, CInetAddress _ConnectedAddr, CInetAddress _RelayAddr, void* _pArg)
#endif
{
	CP2P* pP2P = (CP2P*)_pArg;
	pP2P->EndHolePunching(_HolePunchingResult, _pUdpSock, _ConnectedAddr, _RelayAddr);
}

#ifdef SIP_OS_UNIX
void	RecvUDPPacket(char *pData, int nLength, CInetAddress &remote, void* param)
#else
static	void	RecvUDPPacket(char *pData, int nLength, CInetAddress &remote, void* param)
#endif
{
	CP2P* pP2P = (CP2P*)param;
	pP2P->RecvUDPPacket(pData, nLength);
}

#ifdef SIP_OS_UNIX
void	TargetExit(void* param)
#else
static	void	TargetExit(void* param)
#endif
{
	CP2P* pP2P = (CP2P*)param;
	pP2P->TargetExit();
}

void	CP2P::EndHolePunching(EHolePunchingResult _HolePunchingResult, CCallbackUdp* _pUdpSock, CInetAddress _ConnectedAddr, CInetAddress _RelayAddr)
{
	if (_HolePunchingResult == HP_SUCCESS)
	{
		CSynchronized<CCallbackUdp*>::CAccessor connectedudp( &m_ConnectedUDPSock );
		connectedudp.value() = _pUdpSock;
		connectedudp.value()->setDefaultCallback(SIPP2P::RecvUDPPacket, this);
		m_ConnectedAddr = _ConnectedAddr;

		if (m_cbPeerConnectCallback)
			m_cbPeerConnectCallback(NULL, connectP2P);
		
		m_HolePunchingClient->SetTargetExitCallback(SIPP2P::TargetExit, this);
		SetState(P2P_HOLEPUNCHED);
	}
}

void	CP2P::RecvUDPPacket(char *pData, int nLength)
{
	if (m_cbRecv)
	{
		CInetAddress inetAddress;
		m_cbRecv((const uint8*)pData, nLength, inetAddress, m_cbRecvParam);
	}
}

void	CP2P::TargetExit()
{
	if (m_nState == P2P_HOLEPUNCHED)
	{
		if (m_cbPeerDisconnectCallback)
			m_cbPeerDisconnectCallback(NULL);

		SetState(P2P_NONE);
	}
}

bool	CP2P::connectPeer(uint32 ownid, uint32 dst_peer, std::string relayaddr)
{
	CSynchronized<CCallbackUdp*>::CAccessor connectedudp( &m_ConnectedUDPSock );
	connectedudp.value() = NULL;
	
	m_peerid = ownid;
	m_partner_id = dst_peer;
	m_RelayAddr = CInetAddress(relayaddr);
	startRelayPeer();
	
	delete m_HolePunchingClient;
	m_HolePunchingClient = new SIPNET::CUdpHolePunchingClient();
	m_HolePunchingClient->StartHolePunching(m_BaseClient, ownid, dst_peer, HolePunchingEnd, this);

	return true;
}

void	CP2P::disconnectPeer()
{
	if (m_HolePunchingClient)
		m_HolePunchingClient->StopHolePunching();

	if (m_UdpRelay)
	{
		delete m_UdpRelay;
		m_UdpRelay = NULL;
	}

	CSynchronized<CCallbackUdp*>::CAccessor connectedudp( &m_ConnectedUDPSock );
	connectedudp.value() = NULL;

	SetState(P2P_NONE);
}

bool	CP2P::isConnectedPeer()
{
	if (m_nState == P2P_HOLEPUNCHED)
		return true;
	if (m_nState == P2P_RELAYED)
		return true;

	return false;
}

void	CP2P::setRecvCallback(TPeerRecvCallback cb, void* param)
{
	m_cbRecv		= cb;
	m_cbRecvParam	= param;
}

void	CP2P::setConnectionCallback(TPeerConnectCallback cb)
{
	m_cbPeerConnectCallback = cb;
}

void	CP2P::setDisconnectionCallback(TPeerDisconnectCallback cb)
{
	m_cbPeerDisconnectCallback = cb;
}

bool	CP2P::sendPeer(char* msg, int len)
{
	try
	{
		CSynchronized<CCallbackUdp*>::CAccessor connectedudp( &m_ConnectedUDPSock );
		if (connectedudp.value())
			connectedudp.value()->sendTo((const uint8*)msg, len, m_ConnectedAddr);
	}
	catch (...)
	{
	}
	
	return true;
}

bool	CP2P::sendBroker(CMessage& msgout)
{
	return true;
}

	//***************************************************************************************************************
	//**********************	Communication with RelayService *****************************************************
	//***************************************************************************************************************
#ifdef SIP_OS_UNIX
void	cbUDPMessage(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg)
#else
static	void	cbUDPMessage(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg)
#endif
{
	CP2P* pP2P = (CP2P*)pArg;
	if (msgin.getName() == M_P2P_SC_REGISTERTORELAY)
	{
		pP2P->cbM_P2P_SC_REGISTERTORELAY(msgin, remoteaddr);
		return;
	}
}

static TUDPMsgCallbackItem	UDPCallbackAry[] =
{
	{ M_P2P_SC_REGISTERTORELAY,	cbUDPMessage, NULL },
};

bool	CP2P::sendRelay(char* msg, int len)
{
	if (!m_UdpRelay)
		return false;

	try
	{
		m_UdpRelay->sendTo((const uint8*)msg, len, m_RelayAddr);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

//Relay서비스를 통한 통신부 가동
bool	CP2P::startRelayPeer()
{
	if (m_RelayAddr.port() == 0)
	{
		if (m_cbPeerConnectCallback)
			m_cbPeerConnectCallback(NULL, connectFail);
		SetState(P2P_NONE);
	}
	else
	{
		delete m_UdpRelay;
		m_UdpRelay = new CCallbackUdp(false);
		CInetAddress baseAddr = m_BaseClient->id()->getTcpSock()->localAddr();
		baseAddr.setPort(INADDR_ANY);
		m_UdpRelay->bind(baseAddr);

		m_UdpRelay->addCallbackArray(UDPCallbackAry, sizeof(UDPCallbackAry)/sizeof(UDPCallbackAry[0]), this);
		m_UdpRelay->setDefaultCallback(SIPP2P::RecvUDPPacket, this);

		SetState(P2P_WAIT_REGRELAY);
	}
	return true;
}

//Relay서비스를 통한 통신부 중지
void	CP2P::stopRelayPeer()
{
}

void	CP2P::Update()
{
	if (P2P_WAIT_REGRELAY == m_nState ||
		P2P_RELAYED == m_nState)
	{
		if (m_UdpRelay)
			m_UdpRelay->update();
	}

	TTime	currentTime = CTime::getLocalTime();
	if (P2P_WAIT_REGRELAY == m_nState)
	{
		if (currentTime - m_tmState > MAX_UDPNORESPONSETIME)
		{
			if (m_cbPeerConnectCallback)
				m_cbPeerConnectCallback(NULL, connectFail);
			SetState(P2P_NONE);
			return;
		}
		ResendRegisterRelay();
		return;
	}
}

void	CP2P::DisconnectionCallback(TTcpSockId from)
{
	m_BaseClient = NULL;
}

void	CP2P::DestroyCallback()
{
	m_BaseClient = NULL;
}

void	CP2P::ResendRegisterRelay()
{
	TTime	currentTime = CTime::getLocalTime();
	if (currentTime - m_tmLastUdpSend < 50)
		return;

	CMessage	msg(M_P2P_CS_REGISTERTORELAY);
	msg.serial(m_peerid, m_partner_id);
	sendRelay((char*)(msg.buffer()), msg.length());

	m_tmLastUdpSend = currentTime;
}

void	CP2P::cbM_P2P_SC_REGISTERTORELAY(CMessage& msgin, CInetAddress& remoteaddr)
{
	if (m_nState != P2P_WAIT_REGRELAY)
		return;

	if (remoteaddr != m_RelayAddr)
		return;

	bool	bConnected;
	msgin.serial(bConnected);
	if (!bConnected)
	{
		if (m_cbPeerConnectCallback)
			m_cbPeerConnectCallback(NULL, connectFail);
		SetState(P2P_NONE);
	}
	else
	{
		CSynchronized<CCallbackUdp*>::CAccessor connectedudp( &m_ConnectedUDPSock );
		connectedudp.value() = m_UdpRelay;
		connectedudp.value()->setDefaultCallback(SIPP2P::RecvUDPPacket, this);
		m_ConnectedAddr = m_RelayAddr;

		if (m_cbPeerConnectCallback)
			m_cbPeerConnectCallback(NULL, connectRelay);

		SetState(P2P_RELAYED);
		m_UdpRelay->setKeepAlive(true, m_RelayAddr);
	}
}

} // namespace SIPP2P
