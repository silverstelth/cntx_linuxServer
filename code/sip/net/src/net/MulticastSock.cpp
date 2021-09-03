/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/MulticastSock.h"

namespace	SIPNET	{

	CMulticastSock::CMulticastSock()
	{
	}

	CMulticastSock::~CMulticastSock()
	{
	}

	#define IS_MCASTADDR(x)		(((x)&0xF0000000) == 0xE0000000)

	bool	CMulticastSock::joinMulticastGroup(SIPNET::CInetAddress& groupaddr)
	{
//return false;
		uint32	ip = groupaddr.sockAddr()->sin_addr.s_addr;
		ip = ntohl(ip);
		uint16	port = groupaddr.port();

		if ( ! IS_MCASTADDR(ip) )
		{
			sipinfo("not multicast address: %s", groupaddr.asIPString().c_str());
			return false;
		}

		bind(port);

		int value = true;
		setsockopt( _Sock, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value) );

		/**
		*   You can send the data to any multicast group
		*   without becoming the member of that group.
		*   But in order to receive data from the group you
		*   must be the member of that group.
		*/
		memset(&m_mreq, 0, sizeof(ip_mreq));
		m_mreq.imr_multiaddr.s_addr = htonl(ip);			//group addr
		m_mreq.imr_interface.s_addr = htons(INADDR_ANY);	//use default

#ifdef SIP_OS_UNIX
		if ( setsockopt(_Sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &m_mreq, sizeof(m_mreq)) != 0 )
#else
		if ( setsockopt(_Sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char far*)&m_mreq, sizeof(m_mreq)) != 0 )
#endif
		{
			sipinfo("join multicast group failed");
			return false;
		}

		if ( ! setMulticastTTL(8) )
			return false;

		m_addrGroup = groupaddr;
		sipinfo("join multicast group success: %s", groupaddr.asIPString().c_str());
		return true;
	}

	void	CMulticastSock::leaveMulticastGroup()
	{
#ifdef SIP_OS_UNIX
		if ( setsockopt(_Sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, &m_mreq, sizeof(m_mreq)) != 0 )
#else
		if ( setsockopt(_Sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char FAR *)&m_mreq, sizeof(m_mreq)) != 0 )
#endif
		{
			sipinfo("Leave :: Failed to drop membership");
			return;
		}
		sipinfo("leave from multicast group: %s", m_addrGroup.asIPString().c_str());
	}

	bool	CMulticastSock::setMulticastTTL(int ttl)
	{
		if ( setsockopt(_Sock, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ttl, sizeof(int)) != 0 )
			return false;
		return true;
	}

	bool	CMulticastSock::supportsMulticasting()
	{
		return true;
	}

	bool	CMulticastSock::sendToGroup(char* buffer, int len)
	{
		sendTo((uint8*)buffer, len, m_addrGroup);
		return true;
	}

}
