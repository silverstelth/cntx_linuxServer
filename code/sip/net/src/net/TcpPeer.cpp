/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/TcpPeer.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

namespace	SIPP2P	{

	//*****************************************************************************************************************
	//*********************			CTcpPeerSock class		***********************************************************
	//*****************************************************************************************************************
	CTcpPeerSock::CTcpPeerSock()
	{
	}

	CTcpPeerSock::CTcpPeerSock(CTcpSock& other) :
		CTcpSock(other.descriptor(), other.remoteAddr())
	{
	}

	CTcpPeerSock::~CTcpPeerSock()
	{
	}

	bool	CTcpPeerSock::createPeerSocket(CInetAddress& clientaddr)
	{
		createSocket(SOCK_STREAM, IPPROTO_TCP);

		int	reuse = 1;
		if ( ::setsockopt(_Sock, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int)) == SOCKET_ERROR_NUM)
		{
			sipwarning("setsockopt <SO_REUSEADDR> failed");
			return false;
		}

		if ( ::bind(_Sock, (struct sockaddr *)clientaddr.sockAddr(), sizeof(sockaddr_in)) == SOCKET_ERROR_NUM)
		{
			sipwarning("peer socket bind() failed");
			return false;
		}

		setTimeOutValue(1, 0);
		return true;
	}

	bool	CTcpPeerSock::connectPeer(CInetAddress& addr)
	{
		// Connection (when _Sock is a datagram socket, connect establishes a default destination address)
		if ( ::connect( _Sock, (const sockaddr *)(addr.sockAddr()), sizeof(sockaddr_in) ) != 0 )
			return false;

		setLocalAddress();
		if ( _Logging )
		{
			sipdebug( "Socket %d connected to %s (local %s)", _Sock, addr.asString().c_str(), _LocalAddr.asString().c_str() );
		}	
		_RemoteAddr = addr;

		_BytesReceived = 0;
		_BytesSent = 0;

		_Connected = true;
		return true;
	}

	//*****************************************************************************************************************
	//*********************			CTcpPeer class			***********************************************************
	//*****************************************************************************************************************

	CTcpPeer::CTcpPeer()
	{
		m_listensock = NULL;
	}

	CTcpPeer::~CTcpPeer()
	{
	}

#ifdef SIP_OS_UNIX
	void*	__tcp_peer_connect_1(void*pThis)
	{
		((CTcpPeer*)pThis)->peerConnectThread();
	}

	void*	__tcp_peer_worker_1(void*pThis)
	{
		((CTcpPeer*)pThis)->peerWorkerThread();
	}

	uint GetTickCount(void) 
	{
		struct timespec now;
		if (clock_gettime(CLOCK_MONOTONIC, &now))
			return 0;
		return now.tv_sec * 1000.0 + now.tv_nsec / 1000000.0;
	}
#else
	void	__tcp_peer_connect(void*pThis)
	{
		((CTcpPeer*)pThis)->peerConnectThread();
	}

	void	__tcp_peer_worker(void*pThis)
	{
		((CTcpPeer*)pThis)->peerWorkerThread();
	}
#endif

	bool	CTcpPeer::tryHolePunching(CInetAddress& destaddr, CTcpPeerSock* sock)
	{
		sipinfo("try connect to %s", destaddr.asIPString().c_str());
		for ( int i = 0; i < 2; i ++ )
		{
			if ( sock->connectPeer(destaddr) )
			{
				m_sock = sock;
				onPeerConnection();
				return true;
			}
			if ( isConnected() )
				return true;
		}
		sipinfo("failed connect to %s", destaddr.asIPString().c_str());
		return false;
	}

	void	CTcpPeer::peerConnectThread()
	{
		int		nTryCount = 0;
		CTcpPeerSock*	sock = NULL;

		try
		{
			//create peer socket
			sock = new CTcpPeerSock();

			if ( ! sock->createPeerSocket(m_clientaddr) )
				throw "createPeerSocket() failed";

			if ( m_bSameLocal )
			{
				sipinfo("partner is same local network");

				//send to peer with private address
				if ( tryHolePunching(m_peer_priaddr, sock) )
					return;

				//if global addr and local addr are different
				if ( m_peer_pubaddr == m_peer_priaddr )
					throw "global and local addr are same";

				//send to peer with public address
				if ( tryHolePunching(m_peer_pubaddr, sock) )
					return;
			}
			else
			{
				sipinfo("partner is different global network");

				//send to peer with public address
				if ( tryHolePunching(m_peer_pubaddr, sock) )
					return;

				//if global addr and local addr are different
				if ( m_peer_pubaddr == m_peer_priaddr )
					throw "global and local addr are same";

				//send to peer with private address
				if ( tryHolePunching(m_peer_priaddr, sock) )
					return;
			}
		}
		catch (ESocket& e)
		{
			sipwarning( e.what() );
		}
		catch (char* str)
		{
			sipwarning(str);
		}
		if ( sock )	delete sock;
	}

	bool	CTcpPeer::connect(CInetAddress& pubaddr, CInetAddress& priaddr, uint32 myID, uint32 peerID, bool bSameLocal)
	{
		CPeerBase::connect(pubaddr, priaddr, myID, peerID, bSameLocal);
		try
		{
			//create listen socket
			m_listensock = new CListenSock;
			m_listensock->init(m_clientaddr);

#ifdef SIP_OS_UNIX
			pthread_create(NULL, 0, __tcp_peer_worker_1, this);
			pthread_create(NULL, 0, __tcp_peer_connect_1, this);
#else
			_beginthread(__tcp_peer_worker, 0, this);
			_beginthread(__tcp_peer_connect, 0, this);
#endif
		}
		catch (ESocket& e)
		{
			sipwarning(e.what());
			return false;
		}
		return true;
	}

	void	CTcpPeer::disconnect()
	{
		CPeerBase::disconnect();

		if ( m_listensock )
		{
			m_listensock->close();
			m_listensock = NULL;
		}
	}

	void	CTcpPeer::update()
	{
		baseUpdate();
	}

	void	CTcpPeer::peerWorkerThread()
	{
		// DWORD	dwStartTime = GetTickCount();
		uint	dwStartTime = GetTickCount();

		while ( ! isExit() )
		{
			//listen peer
			if ( ! isConnected() )
			{
				//check timeout of peer connection
				//if ( ! isConnected() && (GetTickCount() - dwStartTime) > 8000 )
				//{
				//	printf("wait listen peer timeout");
				//	break;
				//}

				if ( m_listensock->dataAvailable() )
				{
					try
					{
						CTcpSock*	recvsock = m_listensock->accept();
						if ( recvsock )
						{
							CInetAddress	recvaddr = recvsock->remoteAddr();
							sipinfo("Listen - accepted peer connection <%s>\n", recvaddr.asIPString().c_str()); 

							if ( isPartnerAddress(recvaddr) )
							{
								if ( ! isConnected() )
								{
									sipinfo("Listen - peer connection established\n");
									m_sock = new CTcpPeerSock(*recvsock);
									onPeerConnection();
								}
								else
								{
									sipinfo("Listen - peer ignored\n");
								}
							}
						}
					}
					catch (ESocket& e)
					{
						sipwarning( e.what() );
					}
				}
			}
			else
			{
				try
				{
					if ( m_sock->dataAvailable() )
					{
						//receive packet
						receive();
					}
				}
				catch (ESocket& e)
				{
					sipwarning(e.what());
					disconnect();
				}
			}
		}
	}

	bool	CTcpPeer::procOwnMessage(const char* buffer, uint32 len)
	{
		return false;
	}
}
