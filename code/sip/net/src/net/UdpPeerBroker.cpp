/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/UdpPeerBroker.h"

namespace	SIPP2P	{

	CUdpPeerBroker::CUdpPeerBroker(void) :
		m_cb(NULL)
	{
	}

	CUdpPeerBroker::~CUdpPeerBroker(void)
	{
	}

	void	CUdpPeerBroker::init(uint16 port)
	{
		m_sock = new CUdpSock();
		CInetAddress	addr;
		addr.setPort(port);
		m_sock->bind(port);
		sipinfo("UDP broker init: %s", addr.asIPString().c_str());
	}

	void	CUdpPeerBroker::init(std::string sAddr)
	{
		m_sock = new CUdpSock();
		CInetAddress	addr(sAddr);
		m_sock->bind(addr);
		sipinfo("UDP broker init: %s", addr.asIPString().c_str());
	}

	bool	CUdpPeerBroker::update()
	{
		if ( m_sock->dataAvailable() )
		{
			uint8	buffer[512];
			uint	len = 512;
			CInetAddress	addr;
			if ( m_sock->receivedFrom(buffer, len, addr, false) )
			{
				CMessage msgin(M_SYS_EMPTY);
				msgin.serialBuffer(buffer, len);
				msgin.invert();

				if ( m_cb )
					m_cb(msgin, addr);
			}
		}
		return true;
	}

	void	CUdpPeerBroker::release()
	{
	}

	void	CUdpPeerBroker::sendTo(CInetAddress& destaddr, CMessage& msgout)
	{
		m_sock->sendTo(msgout.buffer(), msgout.length(), destaddr);
	}


}
