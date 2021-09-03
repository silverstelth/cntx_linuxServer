#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef SIPNS_CONFIG
#define SIPNS_CONFIG	""
#endif // SIPNS_CONFIG

#ifndef SIPNS_LOGS
#define SIPNS_LOGS	""
#endif // SIPNS_LOGS

#pragma warning(disable : 4267)
#include <string>
#include <list>
#include "misc/types_sip.h"

#include <fcntl.h>
#include <sys/stat.h>

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#include <direct.h>
#undef max
#undef min
#else
#include <unistd.h>
#include <errno.h>
#endif
#include "net/service.h"
#include "net/callback_server.h"
#include "net/UdpHolePunchingServer.h"
#include "net/callback_udp.h"
#include "../Common/Common.h"
#include "../../common/Packet.h"
#include "RTPParser.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

class CRelayServer
{
public:
	class	CConfirmedPeer
	{
	public:
		CConfirmedPeer(uint32 _PeerId, string _ConfirmedIp)	:
		m_PeerId(_PeerId),
		m_ConfirmedIp(_ConfirmedIp)
		{
		}
		uint32	m_PeerId;
		string	m_ConfirmedIp;
	};

	class	CRelayingPeer
	{
	public:
		CRelayingPeer(uint32 _OwnPeerId, uint32 _TargetPeerId, CInetAddress _PublicAddr) :
		m_OwnPeerId(_OwnPeerId),
		m_TargetPeerId(_TargetPeerId),
		m_PublicAddr(_PublicAddr)
		{
		}
		uint32			m_OwnPeerId;
		uint32			m_TargetPeerId;
		CInetAddress	m_PublicAddr;
	};

	CRelayServer(uint16 _Port);
	virtual ~CRelayServer();
	void	AddConfirmedPeer(uint32 _FamilyId, string _IpString);
	void	RemoveConfirmedPeer(uint32 _FamilyId);
	void	SetMustAddressConfirm(bool _Is)
	{
		m_IsMustAddressConfirm = _Is;
	}

	void update()
	{
		if (m_pRelayUDPServer)
		{
			m_pRelayUDPServer->update();
		}
	}

private:
	friend void					cbRelayUDPMassage_(CMessage&	   _MsgIn,
												   CCallbackUdp&   _LocalUDP,
												   CInetAddress&   _rRemoteAddr,
												   void*		   _pArg);
	void						MsgCB_M_P2P_CS_REGISTERTORELAY(CMessage& _MsgIn, CInetAddress& _rRemoteAddr);

	friend void					voiceDataCallback_(char*		   _pMsgData,
												   int			   _Length,
												   CInetAddress&   _rRemoteAddr,
												   void*		   _pArg);
	void						ProcVoiceChatting(char* _pMsgData, int _Length, CInetAddress& _rRemoteAddr);

	void						SendTo(uint8* _pData, int _Length, CInetAddress _Addr);

	CCallbackUdp*				m_pRelayUDPServer;
	map<uint32, CConfirmedPeer> m_ConfirmedPeers;
	map<uint32, CRelayingPeer>	m_RelayingPeers;
	bool						m_IsMustAddressConfirm;
};

void cbRelayUDPMassage_(CMessage& _MsgIn, CCallbackUdp& _LocalUDP, CInetAddress& _rRemoteAddr, void* _pArg)
{
	CRelayServer*	pRelayServer = (CRelayServer*)_pArg;
	if (M_P2P_CS_REGISTERTORELAY == _MsgIn.getName())
	{
		pRelayServer->MsgCB_M_P2P_CS_REGISTERTORELAY(_MsgIn, _rRemoteAddr);
		return;
	}
}

void voiceDataCallback_(char* _pMsgData, int _Length, CInetAddress& _rRemoteAddr, void* _pArg)
{
	CRelayServer*	pRelayServer = (CRelayServer*)_pArg;
	pRelayServer->ProcVoiceChatting(_pMsgData, _Length, _rRemoteAddr);
}

static TUDPMsgCallbackItem	s_UDPCallbackAry[] = { { M_P2P_CS_REGISTERTORELAY, cbRelayUDPMassage_, NULL }, };

CRelayServer::CRelayServer(uint16 _Port) :
	m_pRelayUDPServer(NULL),
	m_IsMustAddressConfirm(true)
{
	m_pRelayUDPServer = new CCallbackUdp(false);
	m_pRelayUDPServer->bind(_Port);
	m_pRelayUDPServer->addCallbackArray(s_UDPCallbackAry, sizeof(s_UDPCallbackAry) / sizeof(s_UDPCallbackAry[0]), this);
	m_pRelayUDPServer->setDefaultCallback(voiceDataCallback_, this);
}

CRelayServer::~CRelayServer()
{
	delete m_pRelayUDPServer;
	m_pRelayUDPServer = NULL;
}

void CRelayServer::AddConfirmedPeer(uint32 _FamilyId, string _IpString)
{
	map<uint32, CConfirmedPeer>::iterator	itConfirm = m_ConfirmedPeers.find(_FamilyId);
	if (itConfirm == m_ConfirmedPeers.end())
	{
		m_ConfirmedPeers.insert(make_pair(_FamilyId, CConfirmedPeer(_FamilyId, _IpString)));
	}
	else
	{
		CConfirmedPeer*		pConfirmPeer = &(itConfirm->second);
		pConfirmPeer->m_ConfirmedIp = _IpString;
	}
}

void CRelayServer::RemoveConfirmedPeer(uint32 _FamilyId)
{
	m_ConfirmedPeers.erase(_FamilyId);
	m_RelayingPeers.erase(_FamilyId);
}

void CRelayServer::MsgCB_M_P2P_CS_REGISTERTORELAY(CMessage& _MsgIn, CInetAddress& _rRemoteAddr)
{
	uint32	ownId, targetId;
	_MsgIn.serial(ownId, targetId);

	if (m_IsMustAddressConfirm)
	{
		map<uint32, CConfirmedPeer>::iterator	itConfirm = m_ConfirmedPeers.find(ownId);
		if (itConfirm == m_ConfirmedPeers.end())
		{
			return;
		}
		else
		{
			/*
			CConfirmedPeer*		pConfirmedPeer = &(itConfirm->second);
			string				remoteIp = _rRemoteAddr.ipAddress();
			if (remoteIp != pConfirmedPeer->m_ConfirmedIp)
			{
				return;
			}
			*/
		}
	}

	map<uint32, CRelayingPeer>::iterator	itPeer = m_RelayingPeers.find(ownId);
	if (itPeer == m_RelayingPeers.end())
	{
		m_RelayingPeers.insert(make_pair(ownId, CRelayingPeer(ownId, targetId, _rRemoteAddr)));
	}
	else
	{
		CRelayingPeer*	pOwnPeer = &(itPeer->second);
		pOwnPeer->m_TargetPeerId = targetId;
		pOwnPeer->m_PublicAddr = _rRemoteAddr;
	}

	map<uint32, CRelayingPeer>::iterator	itPeerTarget = m_RelayingPeers.find(targetId);
	if (itPeerTarget == m_RelayingPeers.end())
	{
		return;
	}

	if (itPeerTarget->second.m_TargetPeerId != ownId)
	{
		return;
	}

	CMessage	msg(M_P2P_SC_REGISTERTORELAY);
	bool		isConnect = true;
	msg.serial(isConnect);

	SendTo (const_cast<uint8*>(msg.buffer ()), msg.length (), _rRemoteAddr);
}

void CRelayServer::ProcVoiceChatting(char* _pMsgData, int _Length, CInetAddress& _rRemoteAddr)
{
	uint8*	pData = reinterpret_cast < uint8 * > (_pMsgData);
	uint32	srcId = CRTPParser::getSyncSource(pData);

	bool	bReportAddr = _Length <= 12;
	if (bReportAddr)
	{
		return;
	}

	map<uint32, CRelayingPeer>::iterator	itSource = m_RelayingPeers.find(srcId);
	if (itSource == m_RelayingPeers.end())
	{
		return;
	}

	CRelayingPeer*	pSourcePeer = &(itSource->second);

	int				destCount = CRTPParser::getContribSrcCount(pData);
	if (destCount > 1)
	{
		destCount = 1;
	}

	for (int i = 0; i < destCount; i++)
	{
		uint32									destId = CRTPParser::getContribSource(pData, i);
		map<uint32, CRelayingPeer>::iterator	itDest = m_RelayingPeers.find(destId);

		if (itDest != m_RelayingPeers.end())
		{
			CRelayingPeer*	pDestPeer = &(itDest->second);
			if (pSourcePeer->m_TargetPeerId == destId && pDestPeer->m_TargetPeerId == srcId)
			{
				SendTo(pData, _Length, pDestPeer->m_PublicAddr);
			}
		}
	}
}

void CRelayServer::SendTo(uint8* _pData, int _Length, CInetAddress _Addr)
{
	try
	{
		if (m_pRelayUDPServer)
		{
			m_pRelayUDPServer->sendTo(_pData, _Length, _Addr);
		}
	}
	catch(...)
	{
	}
}

static CRelayServer*			s_pRelayServer = NULL;
static CUdpHolePunchingServer*	s_pHolePunchingUDPServer = NULL;
static CCallbackServer*			s_pHolePunchingTCPServer = NULL;
static string					s_RelayHostAddress = "";

static void _cbM_NT_LOGIN(CMessage& _MsgIn, const std::string& _ServiceName, TServiceId _SId)
{
	T_FAMILYID		familyId;
	T_ACCOUNTID		userId;
	T_FAMILYNAME	familyName;
	ucstring		userName;
	uint8			bIsMobile;
	uint8			userType;
	bool			isGM;
	string			ipString;
	_MsgIn.serial(familyId, userId, familyName, userName, bIsMobile, userType, isGM, ipString);

	s_pRelayServer->AddConfirmedPeer(familyId, ipString);
}

static void _cbM_NT_LOGOUT(CMessage& _MsgIn, const std::string& _ServiceName, TServiceId _SId)
{
	T_FAMILYID	familyId;
	_MsgIn.serial(familyId);

	s_pRelayServer->RemoveConfirmedPeer(familyId);
}

static TUnifiedCallbackItem s_ServiceCallbackArry[] =
{
	{ M_NT_LOGIN, _cbM_NT_LOGIN },	// From FS
	{ M_NT_LOGOUT, _cbM_NT_LOGOUT },// From FS
};

class CRelayService :
	public SIPNET::IService
{
public:
	void preInit()
	{
		s_RelayHostAddress = GET_FROM_SERVICECONFIG("RelayHost", "localhost:9600", asString);

		CInetAddress		addrTemp(s_RelayHostAddress);
		uint16				uPort = addrTemp.port();
		static TRegPortItem portAry[] = { { "TIANGUO_RELAYEPort", PORT_UDP, uPort, FILTER_NONE }, };
		setRegPortAry(portAry, sizeof(portAry) / sizeof(portAry[0]));
	}

	void init()
	{
		int		peer_max = GET_FROM_SERVICECONFIG("RelayMaxCount", 1000, asInt);
		bool	isUDPHolePunchingServer = GET_FROM_SERVICECONFIG("UDPHolePunchingServer", false, asBool);
		try
		{
			CInetAddress	addrTemp(s_RelayHostAddress);
			uint16			uPort = addrTemp.port();

			// UDP server
			s_pRelayServer = new CRelayServer(uPort);
			s_pRelayServer->SetMustAddressConfirm(!isUDPHolePunchingServer);

			if (isUDPHolePunchingServer)
			{
				s_pHolePunchingTCPServer = new CCallbackServer;
				s_pHolePunchingTCPServer->init(uPort);
				s_pHolePunchingUDPServer = new CUdpHolePunchingServer();
				s_pHolePunchingUDPServer->Init(s_pHolePunchingTCPServer, 30001, s_RelayHostAddress, NULL);
			}
		}
		catch(...)
		{
			siperrornoex("Failed init");
		}
	}
	
	bool update()
	{
		if (s_pHolePunchingTCPServer)
		{
			s_pHolePunchingTCPServer->update();
		}

		s_pRelayServer->update();
		return (true);
	}

	void release()
	{
		delete s_pRelayServer;
		s_pRelayServer = NULL;

		delete s_pHolePunchingTCPServer;
		s_pHolePunchingTCPServer = NULL;

		delete s_pHolePunchingUDPServer;
		s_pHolePunchingUDPServer = NULL;
	}
};

SIPNET_SERVICE_MAIN(CRelayService, RELAY_SHORT_NAME, RELAY_LONG_NAME, "", 0, false, s_ServiceCallbackArry, "", "")


