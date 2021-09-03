/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __PEERBASE_H__
#define __PEERBASE_H__

#include "PeerPrerequisites.h"
#include <vector>
#include "misc/smart_ptr.h"
#include "misc/mutex.h"

namespace	SIPP2P	{

	class	CPeerBase;
	typedef	CPeerBase*	TPeerId;

	enum	TConnectResult	{
		connectFail,		//접속실패
		connectP2PFail,
		connectP2P,			//P2P 접속
		connectRelay,		//Relay 서비스 리용
	};

	typedef void	(*TPeerRecvCallback) (const uint8* buffer, uint32 len, SIPNET::CInetAddress& recvaddr, void* param);
	typedef	void	(*TPeerCallback)(TPeerId peer, void* param);
	typedef	void	(*TPeerConnectCallback)(TPeerId peer, TConnectResult result);
	typedef	void	(*TPeerDisconnectCallback)(TPeerId peer);

	enum	TPeerState	{
		stateNone,
		stateHalfConnected,		//상대피어로부터 접속파케트를 받은 상태
		stateFullConnected,		//상대피어로부터 나의 접속파케트를 받았다는 응답을 받은 상태
		stateConnected,			//접속상태
	};

	enum	TPeerSysMessage	{
		SYS_NONE,
		SYS_PEER_CONNECT,
		SYS_PEER_DISCONNECT,
		SYS_PEER_CONNECTFAILED,
		SYS_RELAY_CONNECT,
	};
	typedef	SIPBASE::CSynchronized< std::list<TPeerSysMessage> >	TSysMessageQueue;

	//**********************************************************************************************************
	//*****************			Common Interface of Peer and Client				********************************
	//**********************************************************************************************************
	class CPeerBase
	{
	public:
		CPeerBase();
		virtual ~CPeerBase();

	public:
		virtual	bool	connect(SIPNET::CInetAddress& pubaddr, SIPNET::CInetAddress& priaddr, uint32 myID, uint32 peerID, bool bSameLocal = false);
		virtual	void	disconnect();
		virtual	void	update() = 0;

		virtual	bool	startRelayPeer(SIPNET::CInetAddress& relayaddr)	{ return false; }
		virtual	void	stopRelayPeer()	{}

		virtual	SIPNET::CSock::TSockResult	socket_send(const uint8* buffer, uint32& len);
		virtual	SIPNET::CSock::TSockResult	socket_recv(uint8* buffer, uint32& len);

	public:
		virtual	bool	init(SIPNET::CInetAddress& myaddr);
		virtual	bool	procOwnMessage(const char* buffer, uint32 len) = 0;

		virtual	int		send(const char* buffer, uint32 len = 0);
		virtual	void	sendTo(SIPNET::CInetAddress& destaddr, SIPNET::CMessage& msgout)		{}
		virtual	void	sendTo(SIPNET::CInetAddress& destaddr, const char* buffer, uint32 len = 0)	{}
		void	baseUpdate();
		int		receive();
		
		void	setRecvCallback(TPeerRecvCallback cb, void* param = NULL);
		void	setConnectionCallback(TPeerConnectCallback cb);
		void	setDisconnectionCallback(TPeerDisconnectCallback cb);

		bool	isConnected();
		bool	isExit();
		void	setExitState(bool b);
		bool	isValidSocket();

	protected:
		TPeerState	m_state;
		TPeerState	getState();
		void		setState(TPeerState state);
		void		mydisconnect();
		SIPNET::CSock*			m_sock;
		SIPNET::CInetAddress	m_clientaddr;
		SIPNET::CInetAddress	m_peer_pubaddr;
		SIPNET::CInetAddress	m_peer_priaddr;
		SIPNET::CInetAddress	m_peer_conaddr;
		SIPNET::CInetAddress	m_relayaddr;
		uint32					m_myID;
		uint32					m_peerID;
		bool					m_bSameLocal;

		TSysMessageQueue		m_queueSysMessage;

		TPeerRecvCallback		m_cbRecv;
		void*					m_pRecvParam;

		TPeerConnectCallback	m_cbPeerConnectCallback;
		TPeerDisconnectCallback	m_cbPeerDisconnectCallback;

		SIPBASE::CObjectVector<uint8>	m_recvBuffer;

		// DWORD	m_dwLastSendTime;
		// DWORD	m_dwLastRecvTime;
		uint	m_dwLastSendTime;
		uint	m_dwLastRecvTime;
		SIPBASE::CUnfairMutex	m_sync;

		SIPBASE::CFairMutex		m_mutexSock;
	private:
		bool					m_bExit;

	protected:
		void	onPeerConnection();
		void	onPeerDisconnection();
		void	OnPeerConnectionFailed();
		void	OnRelayConnection();

		TPeerSysMessage	popSysMessage();
		void	pushSysMessage(TPeerSysMessage	msg);
		void	clearSysMessages();
		bool	isPartnerAddress(SIPNET::CInetAddress& addr);
	};

}

#endif // end of guard __PEERBASE_H__
