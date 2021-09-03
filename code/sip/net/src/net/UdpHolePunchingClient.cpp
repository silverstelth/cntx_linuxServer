/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/UdpHolePunchingClient.h"

using namespace std;
using namespace SIPBASE;

namespace	SIPNET
{
#define		PLUGINNAME_UDPHOLEPUNCHINGCLIENT	"PI_UDPHPCLIENT"
#ifdef SIP_OS_UNIX
void	_cbTcpMessage(CMessage& _Msg, TSockId _From, CCallbackNetBase& _rClientCB)
#else
static	void	_cbTcpMessage(CMessage& _Msg, TSockId _From, CCallbackNetBase& _rClientCB)
#endif
{
	ICallbackNetPlugin* pPlugin = _rClientCB.findPlugin(PLUGINNAME_UDPHOLEPUNCHINGCLIENT);
	if (!pPlugin)
		return;

	CUdpHolePunchingClient* pUHPC = (CUdpHolePunchingClient*)pPlugin;

	if (M_UHP_SC_CLIENTREGISTER == _Msg.getName())
	{
		pUHPC->MsgCB_M_UHP_SC_CLIENTREGISTER(_Msg);
		return;
	}
	if (M_UHP_SC_UDPPORTEXCHANGE == _Msg.getName())
	{
		pUHPC->MsgCB_M_UHP_SC_UDPPORTEXCHANGE(_Msg);
		return;
	}
	if (M_UHP_SC_CONNECTPRIVATE == _Msg.getName())
	{
		pUHPC->MsgCB_M_UHP_SC_CONNECTPRIVATE(_Msg);
		return;
	}
	if (M_UHP_SC_CONNECTPUBLIC == _Msg.getName())
	{
		pUHPC->MsgCB_M_UHP_SC_CONNECTPUBLIC(_Msg);
		return;
	}
	if (M_UHP_SC_TARGETEXIT == _Msg.getName())
	{
		pUHPC->MsgCB_M_UHP_SC_TARGETEXIT(_Msg);
		return;
	}
}

static TCallbackItem	s_UHPCTCPCallbackAry[] = 
{
	{ M_UHP_SC_CLIENTREGISTER,	_cbTcpMessage },
	{ M_UHP_SC_UDPPORTEXCHANGE, _cbTcpMessage },
	{ M_UHP_SC_CONNECTPRIVATE,	_cbTcpMessage },
	{ M_UHP_SC_CONNECTPUBLIC,	_cbTcpMessage },
	{ M_UHP_SC_TARGETEXIT,		_cbTcpMessage },
};

#ifdef SIP_OS_UNIX
void	_cbUDPMessage(CMessage& _Msg, CCallbackUdp& _rLocalUdp, CInetAddress& _rRemoteaddr, void* _pArg)
#else
static	void	_cbUDPMessage(CMessage& _Msg, CCallbackUdp& _rLocalUdp, CInetAddress& _rRemoteaddr, void* _pArg)
#endif
{
	CUdpHolePunchingClient* pUHPC = (CUdpHolePunchingClient*)_pArg;
	if (M_UHP_CS_CONNECTPRIVATE == _Msg.getName())
	{
		pUHPC->MsgCB_M_UHP_CS_CONNECTPRIVATE(_Msg, _rRemoteaddr);
		return;
	}
	if (M_UHP_CS_CONNECTPUBLIC == _Msg.getName())
	{
		pUHPC->MsgCB_M_UHP_CS_CONNECTPUBLIC(_Msg, _rRemoteaddr);
		return;
	}
}

static TUDPMsgCallbackItem	s_UHPCUDPCallbackAry[] =
{
	{ M_UHP_CS_CONNECTPRIVATE,	_cbUDPMessage, NULL },
	{ M_UHP_CS_CONNECTPUBLIC,	_cbUDPMessage, NULL },
};

//////////////////////////////////////////////////////////////////////////
CUdpHolePunchingClient::CUdpHolePunchingClient() : 
	ICallbackNetPlugin(PLUGINNAME_UDPHOLEPUNCHINGCLIENT)
{
	m_pBaseConnection			= NULL;
	m_pUDPSock					= NULL;
	m_HolePuncingNotifyCallback = NULL;
	m_pHolePuncingNotifyCbArg	= NULL;
	m_TargetExitCallback		= NULL;
	m_pTargetExitCallbackArg		= NULL;

	m_MyHPClientGUID		= 0;
	m_ServerUDPPort			= 0;
	m_TargetHPClientGUID	= 0;
	m_nCurrentState			= UDPHPC_NONE;
	m_tmState				= 0;
	m_tmLastUdpSend			= 0;

	m_bSuccessToTarget		= false;	
	m_bSuccessToMe			= false;	
}

CUdpHolePunchingClient::~CUdpHolePunchingClient()
{
	StopHolePunching();
	delete m_pUDPSock,	m_pUDPSock = NULL;
}

void	CUdpHolePunchingClient::StartHolePunching(CCallbackClient* _pBaseConnection, uint32 _OwnGUID, uint32 _TargetHPClientGUID, THolePunchingNotifyCallback _HolePunchingNotifyCallback, void* _pHolepunchingNotifyCbArg)
{
	m_pBaseConnection			= _pBaseConnection;
	m_TargetHPClientGUID		= _TargetHPClientGUID;
	m_HolePuncingNotifyCallback = _HolePunchingNotifyCallback;
	m_pHolePuncingNotifyCbArg	= _pHolepunchingNotifyCbArg;
	m_RelayAddress				= CInetAddress();
	
	// 메쎄지 callback함수들 설정
	m_pBaseConnection->addPlugin(GetPluginName(), this);
	m_pBaseConnection->addCallbackArray(s_UHPCTCPCallbackAry, sizeof(s_UHPCTCPCallbackAry)/sizeof(s_UHPCTCPCallbackAry[0]));

	// 등록 파케트송신
	string		strMyPrivateBaseIPAddress = GetMyBasePrivateNetAddr().ipAddress();
	CMessage	msgRegister(M_UHP_CS_CLIENTREGISTER);
	msgRegister.serial(_OwnGUID, strMyPrivateBaseIPAddress);
	m_pBaseConnection->send(msgRegister);

	// 상태설정
	SetState(UDPHPC_WAIT_CLIENTREGISTER);
	m_bSuccessToTarget		= false;
	m_bSuccessToMe			= false;	
}

void	CUdpHolePunchingClient::StopHolePunching()
{
	SetState(UDPHPC_NONE);
	NotifyHolePunchingEndToServer();

	if (m_pBaseConnection)
	{
		m_pBaseConnection->removePlugin(GetPluginName());
		m_pBaseConnection = NULL;
	}
}

void	CUdpHolePunchingClient::DisconnectionCallback(TTcpSockId _From)
{
	if (UDPHPC_WAIT_HOLEPUNCHED == m_nCurrentState)
	{
		if (m_TargetExitCallback)
		{
			m_TargetExitCallback(m_pTargetExitCallbackArg);
		}
	}

	StopHolePunching();
}

void	CUdpHolePunchingClient::DestroyCallback()
{
	m_pBaseConnection = NULL;
	StopHolePunching();

	if (UDPHPC_WAIT_HOLEPUNCHED == m_nCurrentState)
	{
		if (m_TargetExitCallback)
		{
			m_TargetExitCallback(m_pTargetExitCallbackArg);
		}
	}

}

void	CUdpHolePunchingClient::NotifyHolePunchedResult(EHolePunchingResult _Result)
{
	if (m_HolePuncingNotifyCallback)
	{
		m_HolePuncingNotifyCallback(_Result, m_pUDPSock, m_TargetHolePunchedNetAddr, m_RelayAddress, m_pHolePuncingNotifyCbArg);
	}
}

void	CUdpHolePunchingClient::NotifyHolePunchingEndToServer()
{
	if (m_pBaseConnection && m_pBaseConnection->connected())
	{
		string		strHolePunchedAddr = m_TargetHolePunchedNetAddr.asIPString();
		CMessage	msg(M_UHP_CS_HOLEPUNCHINGEND);
		msg.serial(strHolePunchedAddr);
		m_pBaseConnection->send(msg);
	}
}

void	CUdpHolePunchingClient::CheckCompleteHolePunching()
{
	if (!m_bSuccessToMe || !m_bSuccessToTarget)
	{
		return;
	}

	SetState(UDPHPC_WAIT_HOLEPUNCHED);
	NotifyHolePunchedResult(HP_SUCCESS);

	if (m_pBaseConnection && m_pBaseConnection->connected())
	{
		string		strHolePunchedAddr = m_TargetHolePunchedNetAddr.asIPString();
		CMessage	msg(M_UHP_CS_HOLEPUNCHINGSUCCESS);
		msg.serial(strHolePunchedAddr);
		m_pBaseConnection->send(msg);
	}

	m_pUDPSock->setKeepAlive(true, m_TargetHolePunchedNetAddr);
}

void	CUdpHolePunchingClient::ProcFailed()
{
	StopHolePunching();
	NotifyHolePunchedResult(HP_FALIED);
}

void	CUdpHolePunchingClient::ResendUdpPortExchange()
{
	TTime	currentTime = CTime::getLocalTime();
	if (currentTime - m_tmLastUdpSend < 50)
	{
		return;
	}

	uint32	nMyClientGUID		= m_MyHPClientGUID;
	string	strPrivateAddr		= GetMyUDPPrivateNetAddr().asIPString();
	uint32	nTargetClientGUID	= m_TargetHPClientGUID;
	{
		CMessage	msg(M_UHP_CS_UDPPORTEXCHANGE);
		msg.serial(nTargetClientGUID, nMyClientGUID, strPrivateAddr);
		CInetAddress	addrUDP = GetServerUDPNetAddr();
		SendToUDP(msg, addrUDP);
	}
	m_tmLastUdpSend = currentTime;
}

void	CUdpHolePunchingClient::ResendHolePunchingSYN()
{
	TTime	currentTime = CTime::getLocalTime();
	if (currentTime - m_tmLastUdpSend < 50)
	{
		return;
	}

	uint32	nMyClientGUID				= m_MyHPClientGUID;
	string	strMyPrivateAddr			= GetMyUDPPrivateNetAddr().asIPString();
	string	strMyRegisteredPublicAddr	= m_MyRegisteredPublicNetAddr.asIPString();
	uint32	nTargetClientGUID			= m_TargetHPClientGUID;
	string	strTargetPrivateAddr		= m_TargetPrivateNetAddr.asIPString();
	string	strTargetRegisteredPubAddr	= m_TargetRegisteredPublicNetAddr.asIPString();
	
	CMessage	msgPrivate(M_UHP_CS_CONNECTPRIVATE);
	msgPrivate.serial(nTargetClientGUID, strTargetPrivateAddr, nMyClientGUID, strMyPrivateAddr);
	CMessage	msgPublic(M_UHP_CS_CONNECTPUBLIC);
	msgPublic.serial(nTargetClientGUID, strTargetRegisteredPubAddr, nMyClientGUID, strMyRegisteredPublicAddr);

	if (m_TargetPrivateNetAddr == m_TargetRegisteredPublicNetAddr)	// 상대방쪽에 NAT가 없으면
	{
		SendToUDP(msgPublic, m_TargetHolePublicNetAddr);
	}
	else
	{
		if (m_MyRegisteredPublicNetAddr.ipAddress() != m_TargetRegisteredPublicNetAddr.ipAddress())	// 서로 다른 NAT안에 있으면
		{
			SendToUDP(msgPublic, m_TargetHolePublicNetAddr);
		}
		else	// 같은 NAT안에 있으면
		{
			SendToUDP(msgPrivate, m_TargetPrivateNetAddr);
			SendToUDP(msgPublic, m_TargetHolePublicNetAddr);
		}
	}
	m_tmLastUdpSend = currentTime;
}

void	CUdpHolePunchingClient::MsgCB_M_UHP_SC_CLIENTREGISTER(CMessage& _Msg)
{
	if (UDPHPC_WAIT_CLIENTREGISTER != m_nCurrentState)
		return;

	bool	bSuccess;
	uint32	nHPClientGUID;
	uint16	nServerUDPPort;
	string	strRelayAddrPort;
	_Msg.serial(bSuccess, nHPClientGUID, nServerUDPPort, strRelayAddrPort);

	if (!bSuccess)
	{
		ProcFailed();
		return;
	}

	m_MyHPClientGUID= nHPClientGUID;
	m_ServerUDPPort	= nServerUDPPort;
	m_RelayAddress.setNameAndPort(strRelayAddrPort);

	delete m_pUDPSock;
	m_pUDPSock = new CCallbackUdp(false);

	CInetAddress baseAddr = GetMyBasePrivateNetAddr();
	baseAddr.setPort(INADDR_ANY);
	m_pUDPSock->bind(baseAddr);

	m_pUDPSock->addCallbackArray(s_UHPCUDPCallbackAry, sizeof(s_UHPCUDPCallbackAry)/sizeof(s_UHPCUDPCallbackAry[0]), this);
	SetState(UDPHPC_WAIT_UDPPORTEXCHANGE);
}

void	CUdpHolePunchingClient::MsgCB_M_UHP_SC_UDPPORTEXCHANGE(CMessage& _Msg)
{
	if (UDPHPC_WAIT_UDPPORTEXCHANGE != m_nCurrentState)
	{
		return;
	}

	uint32		targetHPClientGUID;
	string		targetPrivateNetAddr;
	string		targetRegisteredPublicNetAddr;
	string		myRegisteredPublicNetAddr;
	_Msg.serial(targetHPClientGUID, targetPrivateNetAddr, targetRegisteredPublicNetAddr, myRegisteredPublicNetAddr);

	if (targetHPClientGUID != m_TargetHPClientGUID)
	{
		return;
	}

	m_TargetPrivateNetAddr.setNameAndPort(targetPrivateNetAddr);
	m_TargetRegisteredPublicNetAddr.setNameAndPort(targetRegisteredPublicNetAddr);
	m_TargetHolePublicNetAddr = m_TargetRegisteredPublicNetAddr;
	m_MyRegisteredPublicNetAddr.setNameAndPort(myRegisteredPublicNetAddr);

	SetState(UDPHPC_WAIT_HOLEPUNCHING);
}

void	CUdpHolePunchingClient::MsgCB_M_UHP_SC_CONNECTPRIVATE(CMessage& _Msg)
{
	if (UDPHPC_WAIT_HOLEPUNCHING != m_nCurrentState)
	{
		return;
	}

	uint32	nTargetClientGUID;
	string	strTargetPrivateAddr;
	uint32	nMyClientGUID;
	string	strMyPrivateAddr;

	_Msg.serial(nMyClientGUID, strMyPrivateAddr, nTargetClientGUID, strTargetPrivateAddr);

	if (nTargetClientGUID != m_TargetHPClientGUID ||
		strTargetPrivateAddr != m_TargetPrivateNetAddr.asIPString() ||
		nMyClientGUID != m_MyHPClientGUID ||
		strMyPrivateAddr != GetMyUDPPrivateNetAddr().asIPString())
	{
		return;
	}

	m_bSuccessToTarget = true;
	m_TargetHolePunchedNetAddr = m_TargetPrivateNetAddr;
	CheckCompleteHolePunching();
}

void	CUdpHolePunchingClient::MsgCB_M_UHP_SC_CONNECTPUBLIC(CMessage& _Msg)
{
	if (UDPHPC_WAIT_HOLEPUNCHING != m_nCurrentState)
	{
		return;
	}

	uint32	nTargetClientGUID;
	string	strTargetRegisteredPublicAddr;
	uint32	nMyClientGUID;
	string	strMyRegisteredPublicAddr;

	_Msg.serial(nMyClientGUID, strMyRegisteredPublicAddr, nTargetClientGUID, strTargetRegisteredPublicAddr);

	if (nTargetClientGUID != m_TargetHPClientGUID ||
		strTargetRegisteredPublicAddr != m_TargetRegisteredPublicNetAddr.asIPString() ||
		nMyClientGUID != m_MyHPClientGUID ||
		strMyRegisteredPublicAddr != m_MyRegisteredPublicNetAddr.asIPString())
		return;
	
	if (!m_bSuccessToTarget)
	{
		m_bSuccessToTarget = true;	
		m_TargetHolePunchedNetAddr = m_TargetHolePublicNetAddr;
	}

	CheckCompleteHolePunching();
}

void	CUdpHolePunchingClient::MsgCB_M_UHP_SC_TARGETEXIT(CMessage& _Msg)
{
	if (UDPHPC_NONE == m_nCurrentState)
	{
		return;
	}

	if (UDPHPC_WAIT_HOLEPUNCHED == m_nCurrentState)
	{
		if (m_TargetExitCallback)
			m_TargetExitCallback(m_pTargetExitCallbackArg);
		// notify disconnect
		StopHolePunching();
		return;
	}

	ProcFailed();
}

void	CUdpHolePunchingClient::MsgCB_M_UHP_CS_CONNECTPRIVATE(CMessage& _Msg, CInetAddress& _RemoteAddr)
{
	if (_RemoteAddr != m_TargetPrivateNetAddr)
	{
		return;
	}

	uint32	nTargetClientGUID;
	string	strTargetPrivateAddr;
	uint32	nMyClientGUID;
	string	strMyPrivateAddr;

	_Msg.serial(nMyClientGUID, strMyPrivateAddr, nTargetClientGUID, strTargetPrivateAddr);

	if (nTargetClientGUID != m_TargetHPClientGUID ||
		strTargetPrivateAddr != m_TargetPrivateNetAddr.asIPString() ||
		nMyClientGUID != m_MyHPClientGUID ||
		strMyPrivateAddr != GetMyUDPPrivateNetAddr().asIPString())
	{
		return;
	}

	CMessage	msgAck(M_UHP_SC_CONNECTPRIVATE);
	msgAck.serial(nTargetClientGUID, strTargetPrivateAddr, nMyClientGUID, strMyPrivateAddr);
	m_pBaseConnection->send(msgAck);

	m_bSuccessToMe = true;	
	CheckCompleteHolePunching();
}

void	CUdpHolePunchingClient::MsgCB_M_UHP_CS_CONNECTPUBLIC(CMessage& _Msg, CInetAddress& _RemoteAddr)
{
	if (_RemoteAddr.ipAddress() != m_TargetRegisteredPublicNetAddr.ipAddress() &&
		_RemoteAddr.ipAddress() != m_TargetPrivateNetAddr.ipAddress())
	{
		return;
	}

	uint32	nTargetClientGUID;
	string	strTargetRegisteredPublicAddr;
	uint32	nMyClientGUID;
	string	strMyRegisteredPublicAddr;

	_Msg.serial(nMyClientGUID, strMyRegisteredPublicAddr, nTargetClientGUID, strTargetRegisteredPublicAddr);

	if (nTargetClientGUID != m_TargetHPClientGUID ||
		strTargetRegisteredPublicAddr != m_TargetRegisteredPublicNetAddr.asIPString() ||
		nMyClientGUID != m_MyHPClientGUID ||
		strMyRegisteredPublicAddr != m_MyRegisteredPublicNetAddr.asIPString())
	{
		return;
	}

	CMessage	msgAck(M_UHP_SC_CONNECTPUBLIC);
	msgAck.serial(nTargetClientGUID, strTargetRegisteredPublicAddr, nMyClientGUID, strMyRegisteredPublicAddr);
	m_pBaseConnection->send(msgAck);

	if (UDPHPC_WAIT_HOLEPUNCHING == m_nCurrentState)
	{
		m_TargetHolePublicNetAddr = _RemoteAddr;
	}

	m_bSuccessToMe = true;	
	CheckCompleteHolePunching();
}

void	CUdpHolePunchingClient::Update()
{
	if (m_nCurrentState == UDPHPC_NONE)
	{
		return;
	}

	if (m_pUDPSock)
	{
		m_pUDPSock->update();
	}

	TTime	currentTime = CTime::getLocalTime();
	if (UDPHPC_WAIT_CLIENTREGISTER == m_nCurrentState)
	{
		if (currentTime - m_tmState > MAX_UDPNORESPONSETIME)
		{
			ProcFailed();
			return;
		}
	}	
	if (UDPHPC_WAIT_UDPPORTEXCHANGE == m_nCurrentState)
	{
		if (currentTime - m_tmState > MAX_UDPNORESPONSETIME)
		{
			ProcFailed();
			return;
		}
		ResendUdpPortExchange();
		return;
	}
	if (UDPHPC_WAIT_HOLEPUNCHING == m_nCurrentState)
	{
		if (currentTime - m_tmState > MAX_UDPNORESPONSETIME)
		{
			ProcFailed();
			return;
		}
		ResendHolePunchingSYN();
		return;
	}
}

} // namespace	SIPNET

