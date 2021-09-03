/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UDPPEERBROKER_H__
#define __UDPPEERBROKER_H__

#include "PeerPrerequisites.h"

using namespace SIPNET;

namespace	SIPP2P	{

	typedef void (*TBrokerMsgCallback) (CMessage &msgin, CInetAddress& peeraddr);

	class CUdpPeerBroker
	{
	public:
		CUdpPeerBroker(void);
		~CUdpPeerBroker(void);

	public:
		void	init(uint16 port);
		void	init(std::string sAddr);
		bool	update();
		void	release();
		void	setMsgCallback(TBrokerMsgCallback cb)	{ m_cb = cb; }
		void	sendTo(CInetAddress& destaddr, CMessage& msgout);

	protected:
		SIPBASE::CSmartPtr<CUdpSock>	m_sock;
		TBrokerMsgCallback	m_cb;
	};

}

#endif // end of guard __UDPPEERBROKER_H__
