/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __NATTYPEDETECTIONSERVER_H__
#define __NATTYPEDETECTIONSERVER_H__

#include <misc/types_sip.h>
#include "net/callback_server.h"
#include "net/NatTypeDetectionClient.h"

namespace	SIPNET	
{

class CNATClient
{
public:
	CNATClient(uint32 gid, TTcpSockId sc, std::string priIP, std::string pubIP) :
		m_ClientGUID(gid), m_MainSocket(sc), m_privateIP(priIP), m_publicIP(pubIP), m_publicUDPportToUDP1(0), m_publicUDPportToUDP2(0) {}
	uint32			m_ClientGUID;
	TTcpSockId		m_MainSocket;
	std::string		m_privateIP;
	std::string		m_publicIP;
	uint16			m_publicUDPportToUDP1;
	uint16			m_publicUDPportToUDP2;
};

class CNatTypeDetectionServer : ICallbackNetPlugin
{
public:
	CNatTypeDetectionServer();
	~CNatTypeDetectionServer();
	void		Init(CCallbackServer* pServer, uint16 udpPort1, uint16 udpPort2, uint16 portconePort);

protected:
	virtual		void	Update();
	virtual		void	DisconnectionCallback(TTcpSockId from);
	virtual		void	DestroyCallback();

private:	
	CCallbackServer*	m_pBaseServer;

	CCallbackUdp*		m_UDPSock1;
	CCallbackUdp*		m_UDPSock2;
	CCallbackUdp*		m_PortConeUDPSock;

	std::map<TTcpSockId, CNATClient>	m_mapClientForSockt;
	std::map<uint32, CNATClient>		m_mapClientForGUID;

	friend	void	cbMessage(CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
	void	cbM_NAT_CS_REQUESTNATHAVE(CMessage& msgin, TTcpSockId from);
	void	cbM_NAT_CS_COMPLETEDETECTION(CMessage& msgin, TTcpSockId from);

	friend	void	cbUDP1Message(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg);
	void	cbM_NAT_CS_REQNATTYPE1(CMessage& msgin, CInetAddress& remoteaddr);
	void	cbM_NAT_CS_REQCONETYPE(CMessage& msgin, CInetAddress& remoteaddr);
	
	friend	void	cbUDP2Message(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg);
	void	cbM_NAT_CS_REQNATTYPE2(CMessage& msgin, CInetAddress& remoteaddr);

	void	RemoveNatClient(TTcpSockId from);
	CNATClient*	ConfirmUDPMessage(const std::string &msgName, uint32 clientGUID, const CInetAddress& msgUDPPrivateAddr, const CInetAddress& msgUDPPublicAddr);
};

} // namespace	SIPNET
#endif // end of guard __NATTYPEDETECTIONSERVER_H__
