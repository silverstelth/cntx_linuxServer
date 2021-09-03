/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UDPHOLEPUNCHINGSERVER_H__
#define __UDPHOLEPUNCHINGSERVER_H__

#include <misc/types_sip.h>
#include "net/callback_server.h"
#include "net/UdpHolePunchingClient.h"

namespace	SIPNET	
{
	class CHolePunchingClient
	{
	public:
		CHolePunchingClient(uint32 gid, TTcpSockId sc, std::string priIP, std::string pubIP) :
		  m_ClientGUID(gid), m_MainSocket(sc), m_privateIP(priIP), m_publicTCPIP(pubIP), m_publicUDPport(0), m_privateUDPport(0), m_TargetGUID(0) {}
		  uint32			m_ClientGUID;
		  TTcpSockId		m_MainSocket;
		  std::string		m_privateIP;
		  std::string		m_publicTCPIP;
		  std::string		m_publicUDPIP;
		  uint16			m_publicUDPport;
		  uint16			m_privateUDPport;

		  uint32			m_TargetGUID;
		  CInetAddress		GetPrivateAddr()
		  {
			  return CInetAddress(m_privateIP, m_privateUDPport);
		  }
		  CInetAddress		GetPublicAddr()
		  {
			  return CInetAddress(m_publicUDPIP, m_publicUDPport);
		  }
	};

	typedef uint32 (*TRequestGUIDCallback) ( TTcpSockId from );

	class CUdpHolePunchingServer : ICallbackNetPlugin
	{
	public:
		CUdpHolePunchingServer();
		~CUdpHolePunchingServer();
		void		Init(CCallbackServer* pServer, uint16 udpPort, std::string strRelayAddress, TRequestGUIDCallback cbRequestGUID);

	protected:
		virtual		void	Update();
		virtual		void	DisconnectionCallback(TTcpSockId from);
		virtual		void	DestroyCallback() {};

	private:	
		void	RemoveHPClient(TTcpSockId from);
		friend	void	cbMessage(CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
		void	cbM_UHP_CS_CLIENTREGISTER(CMessage& msgin, TTcpSockId from);
		void	cbM_UHP_SC_CONNECTPRIVATE(CMessage& msgin, TTcpSockId from);
		void	cbM_UHP_SC_CONNECTPUBLIC(CMessage& msgin, TTcpSockId from);
		void	cbM_UHP_CS_HOLEPUNCHINGEND(CMessage& msgin, TTcpSockId from);
		void	cbM_UHP_CS_HOLEPUNCHINGSUCCESS(CMessage& msgin, TTcpSockId from);
		friend	void	cbUDPMessage(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg);
		void	cbM_UHP_CS_UDPPORTEXCHANGE(CMessage& msgin, CInetAddress& remoteaddr);

		CCallbackServer*	m_pBaseServer;
		CCallbackUdp*		m_UDPServer;
		std::string			m_strRelayAddress;
		std::map<TTcpSockId, uint32>				m_mapClientForSockt;
		std::map<uint32, CHolePunchingClient>		m_mapClientForGUID;
		CHolePunchingClient*		GetClientFromSock(TTcpSockId from);

		TRequestGUIDCallback						m_cbRequestGUID;
	};
} // namespace	SIPNET
#endif // end of guard __UDPHOLEPUNCHINGSERVER_H__
