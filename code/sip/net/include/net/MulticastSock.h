/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MULTICASTSOCK_H__
#define __MULTICASTSOCK_H__

#include "net/udp_sock.h"

#ifdef SIP_OS_WINDOWS
#	include <winsock2.h>
#	include <Ws2tcpip.h>
#endif

namespace	SIPNET	{

	class CMulticastSock : public CUdpSock
	{
	public:
		CMulticastSock();
		~CMulticastSock();

	public:
		bool	joinMulticastGroup(CInetAddress& groupaddr);
		void	leaveMulticastGroup();
		bool	setMulticastTTL(int ttl);
		bool	supportsMulticasting();
		bool	sendToGroup(char* buffer, int len);

	private:
		CInetAddress		m_addrGroup;
		struct ip_mreq		m_mreq;
	};

}

#endif //__MULTICASTSOCK_H__
