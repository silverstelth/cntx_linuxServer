/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/UdpPeer.h"
#include <assert.h>

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

namespace	SIPP2P	{

#ifdef SIP_OS_UNIX
	uint GetTickCount(void) 
	{
		struct timespec now;
		if (clock_gettime(CLOCK_MONOTONIC, &now))
			return 0;
		return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
	}
#endif


	//*****************************************************************************************************************
	//*********************			CUdpPeerSock class		***********************************************************
	//*****************************************************************************************************************
	static const	char *TOKEN_REQEUST = "REQUEST_CON";
	static const	char *TOKEN_RESPONSE = "RESPONSE_CON";
	static const	char *TOKEN_REPLY = "REPLY_CON";
	static const	char *TOKEN_DISCON = "DISCONNECT";
	static const	char *TOKEN_PING = "TOKEN";

	CUdpPeerSock::CUdpPeerSock()
		: CUdpSock(false)
	{
	}

	CUdpPeerSock::~CUdpPeerSock()
	{
	}

	bool	CUdpPeerSock::createPeerSocket(CInetAddress& clientaddr)
	{
		int	reuse = 1;
		if ( ::setsockopt(_Sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == SOCKET_ERROR_NUM )
		{
			sipwarning("setsockopt <SO_REUSEADDR> failed");
			return false;
		}

		bind(clientaddr);
		setTimeOutValue(1, 0);

		return true;
	}

	bool	CUdpPeerSock::connectPeer(CInetAddress& addr)
	{
		try
		{
			connect(addr);
		}
		catch (ESocket& e)
		{
			sipwarning(e.what());
			return false;
		}
		return true;
	}

	//*****************************************************************************************************************
	//*********************			CUdpPeer class			***********************************************************
	//*****************************************************************************************************************

	CUdpPeer::CUdpPeer(void)
	{
#ifndef SIP_OS_UNIX
		m_hRelayThread = NULL;
		m_hWorkerThread = NULL;
#endif
	}

	CUdpPeer::~CUdpPeer(void)
	{
		mydisconnect();
	}

#ifdef SIP_OS_UNIX
	void*	__udp_peer_connect_1(void*pThis)
	{
		((CUdpPeer*)pThis)->peerConnectThread();
	}

	void*	__udp_peer_worker_1(void*pThis)
	{
		((CUdpPeer*)pThis)->peerWorkerThread();
	}
#else
	void	__udp_peer_connect(void*pThis)
	{
		((CUdpPeer*)pThis)->peerConnectThread();
	}

	void	__udp_peer_worker(void*pThis)
	{
		((CUdpPeer*)pThis)->peerWorkerThread();
	}
#endif

	bool	CUdpPeer::isLocalPartner(CInetAddress& partner)
	{
//		return false;
		return m_bSameLocal;

		const sockaddr_in*	src = m_clientaddr.sockAddr();
		const sockaddr_in*	dst = partner.sockAddr();
#ifdef SIP_OS_UNIX
		u_char	sb1 = src->sin_addr.s_addr;
		u_char	db1 = dst->sin_addr.s_addr;

		if ( sb1 == db1 )
			return true;
#else
		u_char	sb1 = src->sin_addr.S_un.S_un_b.s_b1;
		u_char	sb2 = src->sin_addr.S_un.S_un_b.s_b2;
		u_char	sb3 = src->sin_addr.S_un.S_un_b.s_b3;
		u_char	db1 = dst->sin_addr.S_un.S_un_b.s_b1;
		u_char	db2 = dst->sin_addr.S_un.S_un_b.s_b2;
		u_char	db3 = dst->sin_addr.S_un.S_un_b.s_b3;

		if ( sb1 == db1 && sb2 == db2 && sb3 == db3 )
			return true;
#endif

		return false;
	}

	bool	CUdpPeer::tryHolePunching(CInetAddress& destaddr)
	{
		static char	msgout[50] = {0};
		sprintf(msgout, "%s%d", TOKEN_REQEUST, m_myID);

		sipinfo("try connect to %s", destaddr.asIPString().c_str());
		for ( int i = 0; i < 10; i ++ )
		{
			if ( ! isValidSocket() )
				return false;

			sendTo(destaddr, msgout);
			sleep(500);
			if ( isConnected() )
				return true;
		}
		sipinfo("failed connect to %s", destaddr.asIPString().c_str());
		return false;
	}

	void	CUdpPeer::peerConnectThread()
	{
		if ( isLocalPartner(m_peer_priaddr) )
		{
			sipinfo("partner is same local network");

			//send to peer with private address
			if ( tryHolePunching(m_peer_priaddr) )
				return;

			//if global addr and local addr are different
			if ( m_peer_pubaddr == m_peer_priaddr )
			{

			}
			else
			{
				//send to peer with public address
				if ( tryHolePunching(m_peer_pubaddr) )
					return;
			}
		}
		else
		{
			sipinfo("partner is different global network");

			//send to peer with public address
			if ( tryHolePunching(m_peer_pubaddr) )
				return;

			//if global addr and local addr are different
			if ( m_peer_pubaddr == m_peer_priaddr )
			{

			}
			else
			{
				//send to peer with private address
				if ( tryHolePunching(m_peer_priaddr) )
					return;
			}
		}

		// Failed HolePunching
		OnPeerConnectionFailed();
	}
	
	bool	CUdpPeer::init(CInetAddress& myaddr)
	{
		if ( m_sock )	return true;	// [6/28/2008 NamJuSong]

		if ( ! CPeerBase::init(myaddr) )
		{
			sipinfo("init failed");
			return false;
		}

		{
			CAutoMutex<>	mutexSock(m_mutexSock);
			if ( m_sock )
			{
				delete m_sock;
				m_sock = NULL;
			}

			m_sock = new CUdpPeerSock();
			if ( ! getSock()->createPeerSocket(myaddr) )
			{
				sipwarning("createPeerSocket() failed");
				return false;
			}
		}
		return true;
	}

	void	CUdpPeer::sendTo(CInetAddress& destaddr, CMessage& msgout)
	{
		try
		{
			if ( ! isValidSocket() )	return;
			getSock()->sendTo(msgout.buffer(), msgout.length(), destaddr);
		}
		catch (ESocket& e)
		{
			sipdebug(e.what());
			return;
		}
	}

	void	CUdpPeer::sendTo(SIPNET::CInetAddress& destaddr, const char* buffer, uint32 len)
	{
		try
		{
			if ( ! isValidSocket() )	return;
			if ( len == 0 )	len = strlen(buffer) + 1;
			getSock()->sendTo((const uint8*)buffer, len, destaddr);
		}
		catch (ESocket& e)
		{
			sipdebug(e.what());
			return;
		}
	}

	bool	CUdpPeer::connect(CInetAddress& pubaddr, CInetAddress& priaddr, uint32 myID, uint32 peerID, bool bSameLocal)
	{
		CPeerBase::connect(pubaddr, priaddr, myID, peerID, bSameLocal);
		
#ifdef SIP_OS_UNIX
		if (pthread_create(&m_workerThread, 0, __udp_peer_worker_1, this) != 0 ||
			pthread_create(NULL, 0, __udp_peer_connect_1, this) != 0)
		{
			throw EThread("Cannot start new thread");
		}
#else
		try
		{
			m_hWorkerThread = (HANDLE)_beginthread(__udp_peer_worker, 0, this);
			_beginthread(__udp_peer_connect, 0, this);
		}
		catch (ESocket& e)
		{
			sipwarning( e.what() );
		}
		catch (char* str)
		{
			sipwarning("%s %s:: %s\n", __FILE__, __FUNCTION__, str);
			return false;
		}
#endif
		return true;
	}

	CSock::TSockResult	CUdpPeer::socket_send(const uint8* buffer, uint32& len0)
	{
		try
		{
			uint	len = len0;
			getSock()->sendTo(buffer, len, m_peer_conaddr);
			len0 = len;
		}
		catch ( ESocket& e)
		{
			sipwarning(e.what());
			return CSock::Error;				 
		}

		return CSock::Ok;
	}

	CSock::TSockResult	CUdpPeer::socket_recv(uint8* buffer, uint32& len0)
	{
		try
		{
			CInetAddress	recvaddr;
			uint			len = len0;
			if ( ! getSock()->receivedFrom(buffer, len, recvaddr) )
				return CSock::Error;
			len0 = len;

			uint32	recvip = recvaddr.internalIPAddress();
			uint32	connip = m_peer_conaddr.internalIPAddress();

			if ( ! ( recvip == connip ) )
			{
				sipinfo("recv addr is different with connect addr: %s", recvaddr.asIPString().c_str());
				return CSock::Error;
			}
		}
		catch (ESocket& e)
		{
			sipwarning(e.what());
			return CSock::Error;
			return CSock::ConnectionClosed;
		}
		return CSock::Ok;
	}

	void	CUdpPeer::peerWorkerThread()
	{
		uint	dwStartTime = GetTickCount();

		while ( ! isExit() )
		{
			try
			{
				if ( ! isConnected() && (GetTickCount() - dwStartTime) > WAIT_PEER_CONNECT_TIME )
				{
					sipinfo("wait listen peer timeout");
					break;
				}

				CAutoMutex<>	mutexSock(m_mutexSock);
				if (m_sock == NULL)
				{
					break;
				}
				if ( ! getSock()->dataAvailable() )
				{
					continue;
				}

				if ( ! isConnected() )
				{
					char	buffer[512];
					uint	len = 512;
					CInetAddress	recvaddr;
					m_dwLastRecvTime = CTime::getTick();

					if ( getSock()->receivedFrom((uint8*)buffer, len, recvaddr) )
					{
						buffer[len] = 0;
						sipinfo("[recv] [%dB] %s from %s", len, buffer, recvaddr.asIPString().c_str());
						
						if ( isPartnerAddress(recvaddr) )
						{
							m_peer_conaddr = recvaddr;
							procOwnMessage(buffer, len);
						}
					}
				}
				else
				{
					//receive packet
					receive();
					sleep(0);
				}
			}
			catch (ESocket& e)
			{
				sipwarning(e.what());
				break;
			}
		}

		sipinfo("finish");
	}

	void	CUdpPeer::StopPeerWorkThread()
	{
#ifdef SIP_OS_UNIX
		if ( m_workerThread != 0)
		{
			setExitState(true);
			if ( pthread_join(m_workerThread, NULL) != 0 )
			{
				pthread_cancel(m_workerThread);
			}
			m_workerThread = 0;
		}
#else
		if ( m_hWorkerThread )
		{
			setExitState(true);
			if ( WaitForSingleObject(m_hWorkerThread, 8000) == WAIT_TIMEOUT )
			{
				// sipassert(0);
				TerminateThread(m_hWorkerThread, 0);
			}
			m_hWorkerThread = NULL;
		}
#endif
	}

	void	CUdpPeer::mydisconnect()
	{
		if ( isConnected() )
			send(TOKEN_DISCON);

		StopPeerWorkThread();
		StopRelayWorkThread();

		CPeerBase::disconnect();

		sipinfo("finish");
	}

	void	CUdpPeer::disconnect()
	{
		mydisconnect();
	}

	void	CUdpPeer::intervalSend(const char* msg, uint timeout)
	{
		if ( (CTime::getTick() - m_dwLastSendTime) > timeout )
		{
			send(msg);
			sipinfo(msg);
		}
	}

	void	CUdpPeer::update()
	{
		baseUpdate();

		if ( isConnected() )
		{
#ifdef NDEBUG
#ifndef SIP_RELEASE_DEBUG
			if ( (CTime::getTick() - m_dwLastRecvTime) > NORECV_LIMIT_TIME )
			{
				sipwarning("no recv for long time");
				disconnect();
				return;
			}
#endif //SIP_RELEASE_DEBUG
#endif //NDEBUG

			if ( getState() == stateHalfConnected )
			{
				intervalSend(TOKEN_RESPONSE, RESPONSE_CON_INTERVAL);
			}
			else
			{
				intervalSend(TOKEN_PING, TOKEN_INTERVAL);
			}
		}
	}

	bool	CUdpPeer::procOwnMessage(const char* buffer, uint32 len)
	{
		if ( strncmp(buffer, TOKEN_REQEUST, strlen(TOKEN_REQEUST)) == 0 &&
			len > strlen(TOKEN_REQEUST))
		{
			const char *cid = buffer + strlen(TOKEN_REQEUST);
			uint32 uContactID = atoi( cid );
			if (uContactID != m_peerID)
				return false;

			sipinfo("[recv] <%s> %d", buffer, getState());
			if ( getState() == stateNone )
				setState(stateHalfConnected);

			sendTo(m_peer_conaddr, TOKEN_RESPONSE);
			sipinfo("P2P half connect with %s", m_peer_conaddr.asIPString().c_str());
			return true;
		}

		if ( strncmp(buffer, TOKEN_RESPONSE, strlen(TOKEN_RESPONSE)) == 0 )
		{
			sipinfo("[recv] <%s> %d", buffer, getState());
			if ( getState() <= stateHalfConnected )
				onPeerConnection();
			
			sendTo(m_peer_conaddr, TOKEN_REPLY);
			return true;
		}

		if ( strncmp(buffer, TOKEN_REPLY, strlen(TOKEN_REPLY)) == 0 )
		{
			sipinfo("[recv] <%s> %d", buffer, getState());
			if ( getState() <= stateHalfConnected )
				onPeerConnection();
			return true;
		}

		if ( strncmp(buffer, TOKEN_DISCON, strlen(TOKEN_DISCON)) == 0 )
		{
			//disconnect();
			setExitState(true);
			return true;
		}
		return false;
	}

	//***************************************************************************************************************
	//**********************	Communication with RelayService *****************************************************
	//***************************************************************************************************************

#ifdef SIP_OS_UNIX
	void*	__peer_relay_worker_1(void*pThis)
	{
		((CUdpPeer*)pThis)->peerRelayThread();
	}
#else
	void	__peer_relay_worker(void*pThis)
	{
		((CUdpPeer*)pThis)->peerRelayThread();
	}
#endif

	void	CUdpPeer::peerRelayThread()
	{
		while ( ! isExit() )
		{
			try
			{
				CAutoMutex<>	mutexSock(m_mutexSock);
				if (m_sock == NULL)
				{
					break;
				}
				if ( ! getSock()->dataAvailable() )
				{
					continue;
				}

				//receive packet
				receive();
				sleep(0);
			}
			catch (ESocket& e)
			{
				sipwarning(e.what());
				break;
			}
		}
		sipinfo("finish");
	}

	bool	CUdpPeer::startRelayPeer(CInetAddress& relayaddr)
	{
		StopPeerWorkThread();

		setExitState(false);
		m_relayaddr = relayaddr;
		m_peer_conaddr = m_relayaddr;
#ifdef SIP_OS_UNIX
		pthread_create(&m_relayThread, 0, __peer_relay_worker_1, this);
#else
		m_hRelayThread = (HANDLE)_beginthread(__peer_relay_worker, 0, this);
#endif

		OnRelayConnection();

		return true;
	}

	void	CUdpPeer::StopRelayWorkThread()
	{
#ifdef SIP_OS_UNIX
		if ( m_relayThread != 0)
		{
			setExitState(true);
			if ( pthread_join(m_relayThread, NULL) != 0 )
			{
				pthread_cancel(m_relayThread);
			}
			m_relayThread = 0;
		}
#else
		if ( m_hRelayThread )
		{
			setExitState(true);
			WaitForSingleObject(m_hRelayThread, 3000);
			m_hRelayThread = NULL;
		}
#endif
	}

	void	CUdpPeer::stopRelayPeer()
	{
		StopRelayWorkThread();
	}
}
