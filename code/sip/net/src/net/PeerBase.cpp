/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/PeerBase.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

namespace	SIPP2P	{

	#define		RECV_BUFFER_SIZE	102400

	CPeerBase::CPeerBase() :
		m_sock(NULL),
		m_sync("CPeerBase::sync"),
		m_mutexSock("CPeerBase::m_mutexSock"),
		m_cbRecv(NULL),
		m_queueSysMessage("m_queueSysMessage"),
		m_bExit(false),
		m_state(stateNone),
		m_myID(0),
		m_peerID(0),
		m_bSameLocal(false)
	{
		m_recvBuffer.resize(RECV_BUFFER_SIZE);
	}

	CPeerBase::~CPeerBase()
	{
		mydisconnect();
		m_recvBuffer.clear();
	}

	bool	CPeerBase::init(CInetAddress& myaddr)
	{
		if ( ! myaddr.isValid() )
			return false;

		//set clien address
		m_clientaddr = myaddr;
		return true;
	}

	TPeerState	CPeerBase::getState()
	{
		TPeerState	s;
		m_sync.enter();
		s = m_state;
		m_sync.leave();
		return s;
	}

	void	CPeerBase::setState(TPeerState state)
	{
		m_sync.enter();
		m_state = state;
		m_sync.leave();
	}

	bool	CPeerBase::isConnected()
	{
		if ( isValidSocket() && getState() >= stateHalfConnected )
			return true;
		return false;
	}

	bool	CPeerBase::isValidSocket()
	{
		bool	b;
		m_sync.enter();
		b = m_sock ? true : false;
		m_sync.leave();
		return b;
	}

	bool	CPeerBase::isExit()
	{
		bool	b;
		m_sync.enter();
		b = m_bExit;
		m_sync.leave();
		return b;
	}

	void	CPeerBase::setExitState(bool b)
	{
		m_sync.enter();
		m_bExit = b;
		m_sync.leave();
	}

	bool	CPeerBase::connect(SIPNET::CInetAddress& pubaddr, SIPNET::CInetAddress& priaddr, uint32 myID, uint32 peerID, bool bSameLocal)
	{
		setState(stateNone);
		m_peer_pubaddr = pubaddr;
		m_peer_priaddr = priaddr;
		m_myID = myID;
		m_peerID = peerID;
		m_bSameLocal = bSameLocal;
		setExitState(false);
		return true;
	}

	void	CPeerBase::mydisconnect()
	{
		m_sync.enter();
		if ( m_sock == NULL )
		{
			m_sync.leave();
			return;
		}

		m_mutexSock.enter();
		m_sock->close();
		delete m_sock;
		m_sock = NULL;
		m_mutexSock.leave();

		clearSysMessages();

		m_bExit = true;
		if ( getState() != stateNone )
			onPeerDisconnection();

		m_cbRecv = NULL;
		m_sync.leave();

		setState(stateNone);
	}

	void	CPeerBase::disconnect()
	{
		mydisconnect();
	}

	CSock::TSockResult	CPeerBase::socket_send(const uint8* buffer, uint32& len)
	{
		if ( m_sock == NULL )	return CSock::Error;
		return m_sock->send( buffer, len, false );
	}

	CSock::TSockResult	CPeerBase::socket_recv(uint8* buffer, uint32& len)
	{
		if ( m_sock == NULL )	return CSock::Error;
		return m_sock->receive( buffer, len, false );
	}

	int	CPeerBase::send(const char* msg, uint32 len)
	{
		if ( ! isConnected() )	return false;

		if ( len == 0 )	len = strlen(msg) + 1;
		if ( CSock::Ok != socket_send((const uint8*)msg, len) )
			return -1;

		m_dwLastSendTime = CTime::getTick();
		return len;
	}

	int	CPeerBase::receive()
	{
		TBlockSize	actuallen = m_recvBuffer.size();
		uint8*		buffer = m_recvBuffer.getPtr();

		CSock::TSockResult	re = socket_recv(buffer, actuallen);
		m_dwLastRecvTime = CTime::getTick();

		switch ( re )
		{
		case CSock::Ok:
			{
				if ( procOwnMessage((char*)buffer, actuallen) )
					return actuallen;

				if ( m_cbRecv )
					m_cbRecv(buffer, actuallen, m_peer_conaddr, m_pRecvParam);

				return actuallen;
			}
			break;
		case CSock::ConnectionClosed:
			{
				throw ESocket("ConnectionClosed");
			}
		    break;
		}
		
		return -1;
	}

	void	CPeerBase::setRecvCallback(TPeerRecvCallback cb, void* param)
	{
		m_cbRecv = cb;
		m_pRecvParam = param;
	}

	void	CPeerBase::setConnectionCallback(TPeerConnectCallback cb)
	{
		m_cbPeerConnectCallback = cb;
	}

	void	CPeerBase::setDisconnectionCallback(TPeerDisconnectCallback cb)
	{
		m_cbPeerDisconnectCallback = cb;
	}

	void	CPeerBase::onPeerConnection()
	{
		if ( getState() >= stateFullConnected )
			return;

		setState(stateFullConnected);
		sipinfo("peer connection completed.");
		m_sock->setTimeOutValue(1, 0);
		pushSysMessage(SYS_PEER_CONNECT);
	}

	void	CPeerBase::onPeerDisconnection()
	{
		sipinfo("peer disconnect.");
		setExitState(true);
		pushSysMessage(SYS_PEER_DISCONNECT);
	}

	void	CPeerBase::OnPeerConnectionFailed()
	{
		setState(stateNone);
		pushSysMessage(SYS_PEER_CONNECTFAILED);
	}

	void	CPeerBase::OnRelayConnection()
	{
		pushSysMessage(SYS_RELAY_CONNECT);
	}

	void	CPeerBase::clearSysMessages()
	{
		TSysMessageQueue::CAccessor	sync(&m_queueSysMessage);
		sync.value().clear();
	}

	TPeerSysMessage	CPeerBase::popSysMessage()
	{
		TSysMessageQueue::CAccessor	sync(&m_queueSysMessage);
		if ( sync.value().empty() )
			return SYS_NONE;
		
		TPeerSysMessage	msg = sync.value().front();
		sync.value().pop_front();
		return msg;
	}

	void	CPeerBase::pushSysMessage(TPeerSysMessage msg)
	{
		TSysMessageQueue::CAccessor	sync(&m_queueSysMessage);
		sync.value().push_back(msg);
	}

	void	CPeerBase::baseUpdate()
	{
		TPeerSysMessage	msg = popSysMessage();
		if ( msg == SYS_NONE )	return;

		switch ( msg )
		{
		case SYS_PEER_CONNECT:
			{
				setState(stateConnected);
				if ( m_cbPeerConnectCallback )
					m_cbPeerConnectCallback(this, connectP2P);
			}
			break;
		case SYS_PEER_DISCONNECT:
			{
				setState(stateNone);
				if ( m_cbPeerDisconnectCallback )
					m_cbPeerDisconnectCallback(this);
			}
			break;
		case SYS_PEER_CONNECTFAILED:
			{
				setState(stateNone);
				if ( m_cbPeerConnectCallback )
					m_cbPeerConnectCallback(this, connectP2PFail);
			}
			break;
		case SYS_RELAY_CONNECT:
			{
				setState(stateConnected);
				if ( m_cbPeerConnectCallback )
					m_cbPeerConnectCallback(this, connectRelay);
			}
			break;
		}
	}

	bool	CPeerBase::isPartnerAddress(CInetAddress& addr)
	{
		//sipinfo("%s, %s", m_peer_pubaddr.asIPString().c_str(), m_peer_priaddr.asIPString().c_str());

		//상대피어가 서로 다른 국부망에 있는 경우
		//포구는 상대피어가 자기가 속한 NAT를 통과하면서 달라질수 있다
		if ( addr.ipAddress() == m_peer_pubaddr.ipAddress() )
			return true;

		//상대피어가 같은 국부망에 있는 경우
		//아이피와 포구가 모두 일치하여야 한다
		if ( addr == m_peer_priaddr )
			return true;

		return false;
	}

}
