/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UDPPEER_H__
#define __UDPPEER_H__

#include "PeerPrerequisites.h"
#include "PeerBase.h"
#include "misc/p_thread.h"

//이시간동안에 P2P접속이 이루어지지 않으면 실패한것으로 한다
#define	WAIT_PEER_CONNECT_TIME	8000	//ms

#define	RESPONSE_CON_INTERVAL	500		//ms

//정기적으로 파케트를 송신하여야 접속이 유지된다
#define	TOKEN_INTERVAL			3000	//ms

//일정한 시간후에도 파케트가 오는것이 없으면
//상대피어와의 접속이 끊어진 상태임
#define	NORECV_LIMIT_TIME		15000	//ms

namespace	SIPP2P	{

	class CUdpPeerSock : public SIPNET::CUdpSock
	{
	public:
		CUdpPeerSock();
		~CUdpPeerSock();

	public:
		bool	createPeerSocket(SIPNET::CInetAddress& clientaddr);
		bool	connectPeer(SIPNET::CInetAddress& peeraddr);
	};

	class CUdpPeer : public CPeerBase
	{
	public:
		CUdpPeer();
		virtual ~CUdpPeer();

	public:
		virtual	bool	init(SIPNET::CInetAddress& myaddr);
		virtual	bool	connect(SIPNET::CInetAddress& pubaddr, SIPNET::CInetAddress& priaddr, uint32 myID, uint32 peerID, bool bSameLocal = false);
		virtual	void	disconnect();
		virtual	void	update();

		virtual	bool	startRelayPeer(SIPNET::CInetAddress& relayaddr);
		virtual	void	stopRelayPeer();

		virtual	SIPNET::CSock::TSockResult	socket_send(const uint8* buffer, uint32& len);
		virtual	SIPNET::CSock::TSockResult	socket_recv(uint8* buffer, uint32& len);
		virtual	bool	procOwnMessage(const char* buffer, uint32 len);
		virtual	void	sendTo(SIPNET::CInetAddress& destaddr, SIPNET::CMessage& msgout);
		virtual	void	sendTo(SIPNET::CInetAddress& destaddr, const char* buffer, uint32 len = 0);

		void	peerConnectThread();
		void	peerWorkerThread();
		void	peerRelayThread();

		CUdpPeerSock*	getSock()	{ return (CUdpPeerSock*)m_sock; }
	protected:
		void			mydisconnect();
	private:
#ifdef SIP_OS_UNIX
		pthread_t		m_workerThread;
		pthread_t		m_relayThread;
#else
		HANDLE			m_hWorkerThread;
		HANDLE			m_hRelayThread;
#endif
		bool			m_bSameLocal;

	private:
		bool	isLocalPartner(SIPNET::CInetAddress& partner);
		bool	tryHolePunching(SIPNET::CInetAddress& destaddr);
		void	intervalSend(const char* msg, uint timeout);
		void	StopPeerWorkThread();
		void	StopRelayWorkThread();
	};

}


#endif // end of guard __UDPPEER_H__
