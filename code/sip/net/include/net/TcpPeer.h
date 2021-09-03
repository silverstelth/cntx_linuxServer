/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TCPPEER_H__
#define __TCPPEER_H__

#include "PeerPrerequisites.h"
#include "PeerBase.h"

namespace	SIPP2P	{

	class CTcpPeerSock : public SIPNET::CTcpSock
	{
	public:
		CTcpPeerSock();
		CTcpPeerSock(SIPNET::CTcpSock& other);
		virtual ~CTcpPeerSock();

	public:
		bool	createPeerSocket(SIPNET::CInetAddress& clientaddr);
		bool	connectPeer(SIPNET::CInetAddress& peeraddr);
	};

	class CTcpPeer : public CPeerBase
	{
	public:
		CTcpPeer();
		~CTcpPeer();

	public:
		virtual	bool	connect(SIPNET::CInetAddress& pubaddr, SIPNET::CInetAddress& priaddr, uint32 myID, uint32 peerID, bool bSameLocal = false);
		virtual	void	disconnect();
		virtual	void	update();
		virtual	bool	procOwnMessage(const char* buffer, uint32 len);

		void	peerWorkerThread();
		void	peerConnectThread();

		CTcpPeerSock*	getSock()	{ return (CTcpPeerSock*)m_sock; }

	private:
		bool	tryHolePunching(SIPNET::CInetAddress& destaddr, CTcpPeerSock* sock);

		SIPBASE::CSmartPtr<SIPNET::CListenSock>		m_listensock;
	};

}


#endif // end of guard __TCPPEER_H__
