/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/NatTypeDetectionClient.h"

using namespace std;
using namespace SIPBASE;

namespace	SIPNET
{

#define		PLUGINNAME_NATTYPEDETECTIONCLIENT	"PI_NTDC"

map<string, CNatType>		CNatTypeDetectionClient::m_mapDetectedType;

const CNatType* CNatTypeDetectionClient::GetAlreadyDetectedNatType(string strDetectionServerAddress)
{
	map<string, CNatType>::const_iterator it = m_mapDetectedType.find(strDetectionServerAddress);
	if (it != m_mapDetectedType.end())
		return &(it->second);

	return NULL;
}

void	CNatTypeDetectionClient::RegisterDetectedNatType(std::string strDetectionServerAddress, const CNatType& natType)
{
	m_mapDetectedType.insert(std::make_pair(strDetectionServerAddress, natType));
}

void	cbMessage(CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	ICallbackNetPlugin* pPlugin = clientcb.findPlugin(PLUGINNAME_NATTYPEDETECTIONCLIENT);
	if (!pPlugin)
		return;

	CNatTypeDetectionClient* pNTDC = (CNatTypeDetectionClient*)pPlugin;
	
	if (msgin.getName() == M_NAT_SC_REQUESTNATHAVE)
	{
		pNTDC->cbM_NAT_SC_REQUESTNATHAVE(msgin);
		return;
	}
	if (msgin.getName() == M_NAT_SC_REQNATTYPE)
	{
		pNTDC->cbM_NAT_SC_REQNATTYPE(msgin);
		return;
	}
}
static TCallbackItem	NTDCCallbackAry[] = 
{
	{ M_NAT_SC_REQUESTNATHAVE,	cbMessage },
	{ M_NAT_SC_REQNATTYPE,		cbMessage },
};

void	cbUDPMessage(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg)
{
	CNatTypeDetectionClient* pNTDC = (CNatTypeDetectionClient*)pArg;
	if (msgin.getName() == M_NAT_SC_REQCONETYPE)
	{
		pNTDC->cbM_NAT_SC_REQCONETYPE(msgin, remoteaddr);
		return;
	}
}

static TUDPMsgCallbackItem	NTDCUDPCallbackAry[] =
{
	{ M_NAT_SC_REQCONETYPE,	cbUDPMessage, NULL },
};

//////////////////////////////////////////////////////////////////////////
CNatTypeDetectionClient::CNatTypeDetectionClient() : 
	ICallbackNetPlugin(PLUGINNAME_NATTYPEDETECTIONCLIENT)
{
	m_pBaseConnection		= NULL;
	m_NatClientGUID			= 0;
	m_ServerUDPPort1		= 0;
	m_ServerUDPPort2		= 0;
	m_ServerPortconeUDPPort	= 0;
	m_tmLastUdpSend			= 0;
	m_UDPSock				= NULL;
	m_DetectionNotifyCallback	= NULL;
	m_DetectionNotifyCbArg		= NULL;
	SetState(NTDC_NONE);
}

CNatTypeDetectionClient::~CNatTypeDetectionClient()
{
	StopNatDetection();
	if (m_pBaseConnection)
	{
		m_pBaseConnection->removePlugin(GetPluginName());
		m_pBaseConnection = NULL;
	}
}

void	CNatTypeDetectionClient::NotifyDectectionResult(const CNatType &natType)
{
	StopNatDetection();

	m_NatTypeForBaseConnection = natType;
	CInetAddress	addrDetectionServer = GetDetectionServerNetAddr();
	CNatTypeDetectionClient::RegisterDetectedNatType(addrDetectionServer.ipAddress(), m_NatTypeForBaseConnection);

	if (m_DetectionNotifyCallback)
	{
		m_DetectionNotifyCallback(natType, addrDetectionServer, m_DetectionNotifyCbArg);
	}
	if (m_pBaseConnection && m_pBaseConnection->connected())
	{
		CMessage	msg(M_NAT_CS_COMPLETEDETECTION);
		msg.serial(m_NatTypeForBaseConnection.m_nNatType);
		m_pBaseConnection->send(msg);
	}

}

CInetAddress CNatTypeDetectionClient::GetDetectionServerNetAddr() const
{
	if (m_pBaseConnection)
	{
		return m_pBaseConnection->remoteAddress();
	}
	return CInetAddress();
}

CInetAddress CNatTypeDetectionClient::GetServerUDP1NetAddr() const
{
	CInetAddress	addrServer = GetDetectionServerNetAddr();
	addrServer.setPort(m_ServerUDPPort1);
	return addrServer;
}

CInetAddress CNatTypeDetectionClient::GetServerUDP2NetAddr() const
{
	CInetAddress	addrServer = GetDetectionServerNetAddr();
	addrServer.setPort(m_ServerUDPPort2);
	return addrServer;
}

CInetAddress CNatTypeDetectionClient::GetServerPortConeUDPNetAddr() const
{
	CInetAddress	addrServer = GetDetectionServerNetAddr();
	addrServer.setPort(m_ServerPortconeUDPPort);
	return addrServer;
}

CInetAddress CNatTypeDetectionClient::GetMyBasePrivateNetAddr() const
{
	if (m_pBaseConnection)
	{
		return m_pBaseConnection->id()->getTcpSock()->localAddr();
	}
	return CInetAddress();
}

CInetAddress CNatTypeDetectionClient::GetMyUDPPrivateNetAddr() const
{
	return m_UDPSock->localAddr();
}

void	CNatTypeDetectionClient::StartNatDetection(CCallbackClient *pBaseConnection, TDetectionNotifyCallback detectionNotifyCallback, void *detectionNotifyCbArg)
{
	sipassert(pBaseConnection);
	StopNatDetection();

// 이미 검출 되여 있는가 확인
	const string	strDetectionServerAddress = GetDetectionServerNetAddr().ipAddress();
	const CNatType *natTypeAlready = CNatTypeDetectionClient::GetAlreadyDetectedNatType(strDetectionServerAddress);
	if (natTypeAlready != NULL)
	{
		NotifyDectectionResult(*natTypeAlready);
		return;
	}

// 설정
	m_pBaseConnection			= pBaseConnection;
	m_DetectionNotifyCallback	= detectionNotifyCallback;
	m_DetectionNotifyCbArg		= detectionNotifyCbArg;

// 메쎄지 callback함수들 설정
	m_pBaseConnection->addPlugin(GetPluginName(), this);
	m_pBaseConnection->addCallbackArray(NTDCCallbackAry, sizeof(NTDCCallbackAry)/sizeof(NTDCCallbackAry[0]));

// Nat유무문의 파케트송신
	string	strMyPrivateBaseIPAddress = GetMyBasePrivateNetAddr().ipAddress();
	CMessage	msgRequest(M_NAT_CS_REQUESTNATHAVE);
	msgRequest.serial(strMyPrivateBaseIPAddress);
	m_pBaseConnection->send(msgRequest);

// 상태설정
	SetState(NTDC_WAIT_REQUESTNATHAVE);
}

void	CNatTypeDetectionClient::StopNatDetection()
{
	delete m_UDPSock;
	m_UDPSock = NULL;
	SetState(NTDC_NONE);
}

void	CNatTypeDetectionClient::Update()
{
//////////////////////////////////////////////////////////////////////////
	if (NTDC_WAIT_CONETYPE == m_nCurrentState)
		m_UDPSock->update();


	TTime	currentTime = CTime::getLocalTime();
	if (NTDC_WAIT_NATTYPE == m_nCurrentState)
	{
		if (currentTime - m_tmState > 20000)
		{
			NotifyDectectionResult(CNatType::NAT_DONTKNOW);
			return;
		}
		ResendReqNatType();
	}

	if (NTDC_WAIT_CONETYPE == m_nCurrentState)
	{
		if (currentTime - m_tmState > 10000)
		{
			NotifyDectectionResult(CNatType::NAT_PORTCONE);
			return;
		}
		ResendReqConeType();
	}
}

void	CNatTypeDetectionClient::DisconnectionCallback(TTcpSockId from)
{
	StopNatDetection();
}

void	CNatTypeDetectionClient::DestroyCallback()
{
	m_pBaseConnection = NULL;
	StopNatDetection();
}

void	CNatTypeDetectionClient::ResendReqNatType()
{
	TTime	currentTime = CTime::getLocalTime();
	if (currentTime - m_tmLastUdpSend > 100)
	{
		uint32	nClientGUID = m_NatClientGUID;
		string	strPrivateAddr = GetMyUDPPrivateNetAddr().asIPString();
		{
			CInetAddress	addrUDP1 = GetServerUDP1NetAddr();
			CMessage	msgReqNatType1(M_NAT_CS_REQNATTYPE1);
			msgReqNatType1.serial(nClientGUID, strPrivateAddr);
			SendToUDP(msgReqNatType1, addrUDP1);
		}
		{
			CInetAddress	addrUDP2 = GetServerUDP2NetAddr();
			CMessage	msgReqNatType2(M_NAT_CS_REQNATTYPE2);
			msgReqNatType2.serial(nClientGUID, strPrivateAddr);
			SendToUDP(msgReqNatType2, addrUDP2);
		}
		m_tmLastUdpSend = currentTime;
	}

}

void	CNatTypeDetectionClient::ResendReqConeType()
{
	TTime	currentTime = CTime::getLocalTime();
	if (currentTime - m_tmLastUdpSend > 50)
	{
		string	strPrivateAddr = GetMyUDPPrivateNetAddr().asIPString();
		uint32	nClientGUID = m_NatClientGUID;
		{
			CInetAddress	addrUDP1 = GetServerUDP1NetAddr();
			CMessage	msgReqNatType1(M_NAT_CS_REQCONETYPE);
			msgReqNatType1.serial(nClientGUID, strPrivateAddr);
			SendToUDP(msgReqNatType1, addrUDP1);
		}
		m_tmLastUdpSend = currentTime;
	}
}

void	CNatTypeDetectionClient::SendToUDP(CMessage& msgout, CInetAddress& destAddre)
{
	try
	{
		m_UDPSock->sendTo(msgout.buffer(), msgout.length(), destAddre);
	}
	catch (...)
	{
	}
}

void	CNatTypeDetectionClient::cbM_NAT_SC_REQUESTNATHAVE(CMessage& msgin)
{
	bool		bHaveNat;
	uint32		natClientGUID;
	uint16		nUDPPort1;
	uint16		nUDPPort2;
	uint16		nPortConeUDPPort;
	msgin.serial(bHaveNat, natClientGUID, nUDPPort1, nUDPPort2, nPortConeUDPPort);
	
	if (!bHaveNat)
	{
		NotifyDectectionResult(CNatType::NAT_DONTHAVE);
		return;
	}

	m_NatClientGUID			= natClientGUID;
	m_ServerUDPPort1		= nUDPPort1;
	m_ServerUDPPort2		= nUDPPort2;
	m_ServerPortconeUDPPort	= nPortConeUDPPort;

	delete m_UDPSock;
	m_UDPSock = new CCallbackUdp(false);
	
	CInetAddress baseAddr = GetMyBasePrivateNetAddr();
	baseAddr.setPort(INADDR_ANY);
	m_UDPSock->bind(baseAddr);

	m_UDPSock->addCallbackArray(NTDCUDPCallbackAry, sizeof(NTDCUDPCallbackAry)/sizeof(NTDCUDPCallbackAry[0]), this);
 
	SetState(NTDC_WAIT_NATTYPE);
}

void	CNatTypeDetectionClient::cbM_NAT_SC_REQNATTYPE(CMessage& msgin)
{
	if (NTDC_WAIT_NATTYPE != m_nCurrentState)
		return;

	bool		bSymmetricNAT;
	msgin.serial(bSymmetricNAT);
	
	if (bSymmetricNAT)
	{
		NotifyDectectionResult(CNatType::NAT_SYMMETRIC);
		return;
	}
	else
	{
		NotifyDectectionResult(CNatType::NAT_ADDRESSCONE);
		return;
	}
	// NAT가 PortCone인 경우에 제3의 UDP주소로부터 파케트가 들어오는가를 기다려야 한다
	// 이시간이 프로그람의 동작에 지장을 줄수 있으므로 현재는 PortCone검출을 하지 않는다.
	// 후에 필요한때에 우의 else부분을 막고 아래의 SetState(NTDC_WAIT_CONETYPE); 를 열도록 한다.
	// SetState(NTDC_WAIT_CONETYPE);
}

void	CNatTypeDetectionClient::cbM_NAT_SC_REQCONETYPE(CMessage& msgin, CInetAddress& remoteaddr)
{
	if (NTDC_WAIT_CONETYPE != m_nCurrentState)
		return;

	uint32		confirmGUID;
	msgin.serial(confirmGUID);
	if (confirmGUID == m_NatClientGUID)
	{
		if (GetServerPortConeUDPNetAddr().asIPString() == remoteaddr.asIPString())
		{
			NotifyDectectionResult(CNatType::NAT_ADDRESSCONE);
			return;
		}
	}
}
} // namespace	SIPNET

