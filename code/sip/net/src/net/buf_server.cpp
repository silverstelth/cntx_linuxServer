/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "misc/hierarchical_timer.h"

#include "net/buf_server.h"
#include "misc/system_info.h"

#ifdef SIP_OS_WINDOWS
#include <windows.h>
//typedef sint socklen_t;

#elif defined SIP_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#endif


using namespace SIPBASE;
using namespace std;

namespace SIPNET {

uint32 	NbServerListenTask = 0;
uint32 	NbServerReceiveTask = 0;

/***************************************************************************************************
 * User main thread (initialization)
 **************************************************************************************************/
CVariable<uint32>	ServiceUseMemPercent("net", "ServiceUseMemPercent", "available memory percent in one computer", 0, 0, true);
CVariable<uint32>	ServiceUnUseMemMbyte("net", "ServiceUnUseMemMbyte", "unavailable memory Mbytes in one computer", 0, 0, true);


uint64	CBufServer::_uTotalMemByteOfComputer = 0;
uint64	CBufServer::_uAvailableMemByte = 0;
#define PERCENT(x, p) ( x * p / 100)
/*
 * Constructor
 */
CBufServer::CBufServer( bool bUdpServer, bool bUserServer, uint32 max_frontnum, TThreadStategy strategy,
	uint16 max_threads, uint16 max_sockets_per_thread, bool nodelay, bool replaymode, bool initPipeForDataAvailable ) :
#ifdef SIP_OS_UNIX
	CBufNetBase( initPipeForDataAvailable ),
#else
	CBufNetBase(),
#endif
	_UdpServer(bUdpServer),
	_FrontEnd(bUserServer),
	_MaxFrontClientNum(max_frontnum),
	_ThreadStrategy( strategy ),
	_MaxThreads( max_threads ),
	_MaxSocketsPerThread( max_sockets_per_thread ),
	_ListenTask( NULL ),
	_ListenThread( NULL ),
	_UdpReceiveTask( NULL ),
	_UdpReceiveThread( NULL ),
	_UdpServerSock( NULL ),
	_ThreadPool("CBufServer::_ThreadPool"),
	_ConnectionCallback( NULL ),
	_ConnectionCbArg( NULL ),
	_BytesPushedOut( 0 ),
	_BytesPoppedIn( 0 ),
	_PrevBytesPoppedIn( 0 ),
	_PrevBytesPushedOut( 0 ),
	_NbConnections (0),
	_NoDelay( nodelay ),
	_ReplayMode( replaymode ),
	_TcpAndUdpMap("CBufServer::_TcpAndUdpMap"),
	_bConnectionChange(false),
	_GetMsgCheckNeedCallback( NULL ),
	_ProcUnlawSerialCallback( NULL ),
	_LastRefreshBufferStateTime( 0 ),
	_AllClientRecvBufferSumByte(0),
	_AllClientRecvBufferItemNum(0),
	_RQConnections(0)
{
	{
		CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
		poolsync.value().reserve(_MaxThreads * _MaxThreads);
	}

	if ( ! _ReplayMode )
	{
		_ListenTask = new CListenTask( this );
		_ListenThread = IThread::create( _ListenTask, 1024*4*4 );
		if (bUdpServer == true)
		{
			_UdpReceiveTask = new CUdpReceiveTask( this );
			_UdpReceiveThread = IThread::create( _UdpReceiveTask, 1024*4*4 );
			_UdpServerSock = new CUdpSimSock( false );
		}

	}
	_uTotalMemByteOfComputer = CSystemInfo::totalPhysicalMemory();
	_uAvailableMemByte = (PERCENT(_uTotalMemByteOfComputer, 70));
	if (ServiceUseMemPercent != 0)
		_uAvailableMemByte = (PERCENT(_uTotalMemByteOfComputer, ServiceUseMemPercent));
	else if (ServiceUnUseMemMbyte != 0)
		_uAvailableMemByte = _uTotalMemByteOfComputer - (ServiceUnUseMemMbyte << 20); // (<< 20 = 1024*1024)

	/*{
		CSynchronized<uint32>::CAccessor syncbpi ( &_BytesPushedIn );
		syncbpi.value() = 0;
	}*/
}


/*
 * Listens on the specified port
 */
void CBufServer::init( uint16 port )
{
	if ( ! _ReplayMode )
	{
		_ListenTask->init( port, maxExpectedBlockSize() );
		_ListenThread->start();
		
		if ( _UdpServer == true )
		{
			_UdpReceiveTask->init( port );
			_UdpReceiveThread->start();
		}
	}
	else
	{
		sipdebug( "LNETL1: Binding listen socket to any address, port %hu", port );
	}
}


/*
 * Begins to listen on the specified port (call before running thread)
 */
void CListenTask::init( uint16 port, sint32 maxExpectedBlockSize )
{
	_ListenSock.init( port );
	_MaxExpectedBlockSize = maxExpectedBlockSize;
}


/***************************************************************************************************
 * User main thread (running)
 **************************************************************************************************/


/*
 * Constructor
 */
CServerTask::CServerTask() : NbLoop (0), _ExitRequired(false)
{
#ifdef SIP_OS_UNIX
	pipe( _WakeUpPipeHandle );
#endif
}



#ifdef SIP_OS_UNIX
/*
 * Wake the thread up, when blocked in select (Unix only)
 */
void CServerTask::wakeUp()
{
	uint8 b;
	if ( write( _WakeUpPipeHandle[PipeWrite], &b, 1 ) == -1 )
	{
		sipdebug( "LNETL1: In CServerTask::wakeUp(): write() failed" );
	}
}
#endif


/*
 * Destructor
 */
CServerTask::~CServerTask()
{
#ifdef SIP_OS_UNIX
	close( _WakeUpPipeHandle[PipeRead] );
	close( _WakeUpPipeHandle[PipeWrite] );
#endif
}


/*
 * Destructor
 */
CBufServer::~CBufServer()
{
	// Clean listen thread exit
	if ( ! _ReplayMode )
	{
		((CListenTask*)(_ListenThread->getRunnable()))->requireExit();
		((CListenTask*)(_ListenThread->getRunnable()))->close();
#ifdef SIP_OS_UNIX
		_ListenTask->wakeUp();
#endif
		_ListenThread->wait();
		delete _ListenThread;
		delete _ListenTask;
		
		if (_UdpReceiveThread != NULL)
		{
			((CUdpReceiveTask*)(_UdpReceiveThread->getRunnable()))->requireExit();
			_UdpReceiveThread->wait();
			delete _UdpReceiveThread;
		}
		if ( _UdpReceiveTask != NULL )
			delete _UdpReceiveTask;
		if ( _UdpServerSock != NULL )
			delete _UdpServerSock;
		// Clean receive thread exits
		CThreadPool::iterator ipt;
		{
			sipdebug( "LNETL1: Waiting for end of threads..." );
			CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
			for ( ipt=poolsync.value().begin(); ipt!=poolsync.value().end(); ++ipt )
			{
				// Tell the threads to exit and wake them up
				CServerReceiveTask *task = receiveTask(ipt);
				task->requireExit();

				// Wake the threads up
	#ifdef SIP_OS_UNIX
				task->wakeUp();
	#else
				CConnections::iterator ipb;
				{
					CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
					for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
					{
						(*ipb)->Sock->disconnect();
					}
				}
	#endif

			}
			
			for ( ipt=poolsync.value().begin(); ipt!=poolsync.value().end(); ++ipt )
			{
				// Wait until the threads have exited
				(*ipt)->wait();
			}

			sipdebug( "LNETL1: Deleting sockets, tasks and threads..." );
			for ( ipt=poolsync.value().begin(); ipt!=poolsync.value().end(); ++ipt )
			{
				// Delete the socket objects
				CServerReceiveTask *task = receiveTask(ipt);
				CConnections::iterator ipb;
				{
					CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
					for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
					{
						CServerBufSock *tcpsock = (CServerBufSock *)(*ipb);
						if (tcpsock->_UdpFriend != NULL)
							delete tcpsock->_UdpFriend;			
						delete (*ipb); // closes and deletes the socket
					}
				}

				// Delete the task objects
				delete task;

				// Delete the thread objects
				delete (*ipt);
			}

		}
	}

}


/*
 * Disconnect the specified host
 * Set hostid to NULL to disconnect all connections.
 * If hostid is not null and the socket is not connected, the method does nothing.
 * If quick is true, any pending data will not be sent before disconnecting.
 */
void CBufServer::disconnect( TTcpSockId hostid, bool quick )
{
	if ( hostid != InvalidSockId )
	{
		if (_ConnectedClients.find(hostid) == _ConnectedClients.end())
		{
			// this host is not connected
			return;
		}

		// Disconnect only if physically connected
		if ( hostid->Sock->connected() )
		{
			if ( ! quick )
			{
				hostid->flush();
			}
			hostid->Sock->disconnect(); // the connection will be removed by the next call of update()
		}
	}
	else
	{
		// Disconnect all
		CThreadPool::iterator ipt;
		{
			CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
			for ( ipt=poolsync.value().begin(); ipt!=poolsync.value().end(); ++ipt )
			{
				CServerReceiveTask *task = receiveTask(ipt);
				CConnections::iterator ipb;
				{
					CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
					for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
					{
						if ( (*ipb)->Sock->connected() )
						{
							if ( ! quick )
							{
								(*ipb)->flush();
							}
							(*ipb)->Sock->disconnect();
						}
					}
				}
			}
		}
	}
}

void CBufServer::forceClose( TTcpSockId hostid, bool quick )
{
	if ( hostid != InvalidSockId )
	{
		if (_ConnectedClients.find(hostid) == _ConnectedClients.end())
		{
			// this host is not connected
			return;
		}

		// Disconnect only if physically connected
		if ( hostid->Sock->connected() )
		{
			if ( ! quick )
			{
				hostid->flush();
			}
			hostid->Sock->forceclose(); // the connection will be removed by the next call of update()
		}
	}
	else
	{
		// Disconnect all
		CThreadPool::iterator ipt;
		{
			CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
			for ( ipt=poolsync.value().begin(); ipt!=poolsync.value().end(); ++ipt )
			{
				CServerReceiveTask *task = receiveTask(ipt);
				CConnections::iterator ipb;
				{
					CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
					for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
					{
						if ( (*ipb)->Sock->connected() )
						{
							if ( ! quick )
							{
								(*ipb)->flush();
							}
							(*ipb)->Sock->forceclose();
						}
						CServerBufSock *tcpsock = (CServerBufSock *)(*ipb);
						if (tcpsock->_UdpFriend != NULL)
							delete tcpsock->_UdpFriend;			
						delete (*ipb); // closes and deletes the socket
					}
				}
			}
		}
	}
}

/*
 * Send a message to the specified host
 */
void CBufServer::send( const CMemStream& buffer, TTcpSockId hostid, uint8 bySendType )
{
	sipassert( buffer.length() > 0 );
	sipassertex( buffer.length() <= maxSentBlockSize(), ("length=%u max=%u", buffer.length(), maxSentBlockSize()) );

	// slow down the layer H_AUTO (CBufServer_send);

	bool	bUdp = true;
	if (bySendType == REL_GOOD_ORDER )
		bUdp = false;

	if ( hostid != InvalidSockId )
	{
		if (_ConnectedClients.find(hostid) == _ConnectedClients.end())
		{
			// this host is not connected
			return;
		}
		// debug features, we number all packet to be sure that they are all sent and received
		// \todo remove this debug feature when ok
//		sipdebug ("send message number %u", hostid->SendNextValue);
#ifdef SIP_BIG_ENDIAN
		uint32 val = SIPBASE_BSWAP32(hostid->SendNextValue);
#else
		uint32 val = hostid->SendNextValue;
#endif

		*(uint32*)buffer.buffer() = val;
		hostid->SendNextValue++;

		if (bUdp)
		{
			sipassert(_UdpServer == true);
			CServerBufSock *tcpsock = (CServerBufSock *)hostid;
			tcpsock->_UdpFriend->pushBuffer(bySendType, buffer);
		}
		else
			pushBufferToHost( buffer, hostid );

	}
	else
	{
		// Push into all send queues
		CThreadPool::iterator ipt;
		{
			CThreadPool::iterator poolsyncBegin;
			CThreadPool::iterator poolsyncEnd;
			CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
			poolsyncBegin = poolsync.value().begin();
			poolsyncEnd = poolsync.value().end();
			for ( ipt=poolsyncBegin; ipt!=poolsyncEnd; ++ipt )
			{
				CServerReceiveTask *task = receiveTask(ipt);
				CConnections::iterator ipb;
				{
					CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
					for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
					{
						// Send only if the socket is logically connected
						if ( (*ipb)->connectedState() ) 
						{
							// debug features, we number all packet to be sure that they are all sent and received
							// \todo remove this debug feature when ok
//							sipdebug ("send message number %u", (*ipb)->SendNextValue);
#ifdef SIP_BIG_ENDIAN
							uint32 val = SIPBASE_BSWAP32((*ipb)->SendNextValue);
#else
							uint32 val = (*ipb)->SendNextValue;
#endif
							*(uint32*)buffer.buffer() = val;
							(*ipb)->SendNextValue++;

							if (bUdp)
							{
								sipassert(_UdpServer == true);
								CServerBufSock *tcpsock = (CServerBufSock *)(*ipb);
								tcpsock->_UdpFriend->pushBuffer(bySendType, buffer);
							}
							else
								pushBufferToHost( buffer, *ipb );
						}
					}
				}
			}
		}
	}
}

void	CBufServer::RefreshSockBuffer()
{
	uint32	uSockNum = _ConnectedClients.size();
	if (uSockNum < 1)
		return;

	//	uint64	uAvailableMemByteOfComputer = CSystemInfo::availablePhysicalMemory();
	//	uint32	uAvailableMemByte = (uint32)(PERCENT(uAvailableMemByteOfComputer, 60));
	//	uint32	uMaxBufferPerSock = uAvailableMemByte / uSockNum;
	
	// uint32	uMaxBufferPerSock = uAvailableMemByte / _MaxFrontClientNum;
	uint64	uMaxBufferPerSock = _uAvailableMemByte / uSockNum;
	
	TTcpSet::iterator it;
	for (it = _ConnectedClients.begin(); it != _ConnectedClients.end(); it++)
	{
		CServerBufSock *serverbufsock = static_cast<CServerBufSock*>(static_cast<CBufSock*>(*it));
		serverbufsock->SetMaxBufferLength(uMaxBufferPerSock);
	}
	sipinfo("Set server bufsock max buffer length = %dKbyte", (uMaxBufferPerSock >> 10) );
}

void	CBufServer::RefillFromClientBuffer()
{
	uint64	AllClientBufferSizeByte = 0;
	uint32	AllClientBufferItemNum = 0;
	bool	bRepeat = true;
	while (bRepeat)
	{
		AllClientBufferSizeByte = 0;
		AllClientBufferItemNum = 0;
		CThreadPool::iterator ipt;
		{
			//sipdebug( "UPD: Acquiring the Thread Pool" );
			CThreadPool::iterator poolsyncBegin;
			CThreadPool::iterator poolsyncEnd;
			CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
			poolsyncBegin = poolsync.value().begin();
			poolsyncEnd = poolsync.value().end();
			//sipdebug( "UPD: Acquired." );
			for ( ipt=poolsyncBegin; ipt!=poolsyncEnd; ++ipt )
			{
				// For each thread of the pool
				CServerReceiveTask *task = receiveTask(ipt);
				CConnections::iterator ipb;
				{
					CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
					for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
					{
						CServerBufSock *serverbufsock = static_cast<CServerBufSock*>(static_cast<CBufSock*>(*ipb));
						CFifoAccessor recvfifo( &(serverbufsock->receiveQueue()) );
						if ( recvfifo.value().empty() )
							continue;

						{
							uint8 *tmpbuffer;
							uint32 s;
							//						CMemStream buffer;
							//						recvfifo.value().front( buffer );
							recvfifo.value().front( tmpbuffer, s );
							pushMessageIntoReceiveQueue(tmpbuffer, s);
						}
						recvfifo.value().pop();
						AllClientBufferSizeByte += recvfifo.value().size();
						AllClientBufferItemNum += recvfifo.value().fifoSize();
					}
				}
			}
		}
		{
//			SIPBASE::CFifoAccessor recvfifo( &receiveQueue() );
//			uint32 uServerBufNum = recvfifo.value().fifoSize();
//			if (uServerBufNum < 2000 && AllClientBufferSizeByte > 0)
//				bRepeat = true;
//			else
//				bRepeat = false;
		bRepeat = false;
		}
	}
	_AllClientRecvBufferSumByte = AllClientBufferSizeByte;
	_AllClientRecvBufferItemNum = AllClientBufferItemNum;
}

/*
 * Checks if there are some data to receive
 */
bool CBufServer::dataAvailable()
{
	// slow down the layer H_AUTO (CBufServer_dataAvailable);
	{
		/* If no data available, enter the 'while' loop and return false (1 volatile test)
		 * If there are user data available, enter the 'while' and return true immediately (1 volatile test + 1 short locking)
		 * If there is a connection/disconnection event (rare), call the callback and loop
		 */
		if ( !dataAvailableFlag() )
			RefillFromClientBuffer();
		while ( dataAvailableFlag() )
		{
			// Because _DataAvailable is true, the receive queue is not empty at this point
			vector<uint8> buffer;
			uint8 val;
			{
				CFifoAccessor recvfifo( &receiveQueue() );
				val = recvfifo.value().frontLast();
				recvfifo.value().front( buffer );
				if ( val == CBufNetBase::User )
				{
					TTcpSockId phostid = *((TTcpSockId*)&(buffer[buffer.size()-sizeof(TTcpSockId)-1]));
					if (phostid->Sock->forceClosed())
					{
						recvfifo.value().pop();
						setDataAvailableFlag( ! recvfifo.value().empty() );
						continue;
					}
				}
			}

			/*sint32 mbsize = recvfifo.value().size() / 1048576;
			if ( mbsize > 0 )
			{
			  sipwarning( "The receive queue size exceeds %d MB", mbsize );
			}*/

			/*vector<uint8> buffer;
			recvfifo.value().front( buffer );*/

#ifdef SIP_OS_UNIX
			uint8 b;
			if ( read( _DataAvailablePipeHandle[PipeRead], &b, 1 ) == -1 )
				sipwarning( "LNETL1: Read pipe failed in dataAvailable" );
			//sipdebug( "Pipe: 1 byte read (server %p)", this );
#endif

			// Test if it the next block is a system event
			//switch ( buffer[buffer.size()-1] )
			switch ( val )
			{
				
			// Normal message available
			case CBufNetBase::User:
				{
					return true; // return immediately, do not extract the message
				}

			// Process disconnection event
			case CBufNetBase::Disconnection:
				{
					TTcpSockId sockid = *((TTcpSockId*)(&*buffer.begin()));
					sipdebug( "LNETL1: Disconnection event for %p %s", sockid, sockid->asString().c_str());

					sockid->setConnectedState( false );

					// Call callback if needed
					if ( disconnectionCallback() != NULL )
					{
						disconnectionCallback()( sockid, argOfDisconnectionCallback() );
					}
					if (_UdpServer)
					{
						CSynchronized<TTcpAndUdp>::CAccessor tcpAndUdp( &_TcpAndUdpMap );
						tcpAndUdp.value().erase(sockid);
					}

					// remove from the list of valid client
					sipverify(_ConnectedClients.erase(sockid));

					// Add socket object into the synchronized remove list
					sipdebug( "LNETL1: Adding the connection(%s) to the remove list", sockid->asString().c_str());
					sipassert( ((CServerBufSock*)sockid)->ownerTask() != NULL );
					((CServerBufSock*)sockid)->ownerTask()->addToRemoveSet( sockid );

					if (frontend())
						RefreshSockBuffer();
					break;
				}
			// Process connection event
			case CBufNetBase::Connection:
				{
					sipinfo("LNETL1: DATAABAILABLE CONNECTION");
					TTcpSockId sockid = *((TTcpSockId*)(&*buffer.begin()));
					sipdebug( "LNETL1: Connection event for %p %s", sockid, sockid->asString().c_str());

					// add this socket in the list of client
					sipverify(_ConnectedClients.insert(sockid).second);

					sockid->setConnectedState( true );

					if (_UdpServer)
					{
						CSynchronized<TTcpAndUdp>::CAccessor tcpAndUdp( &_TcpAndUdpMap );
						tcpAndUdp.value().insert(make_pair(sockid, ((CServerBufSock *)sockid)->_UdpFriend));

						// UDP\C1\A2\BC\D3\C0\BB Ȯ\C0\CE\C7ϱ\E2 \C0\A7\C7\D1 TCP ID\B0\AA\C0\BB Ŭ\B6\F3\C0̾\F0Ʈ\BF\A1\B0\D4 \BA\B8\B3\BD\B4\D9
						CMemStream sb;
						string	strMesID = UDPCONNECTION;
						uint32	uID = *(uint32*)sockid;
						sb.serial(strMesID);
						sb.serial(uID);
						pushBufferToHost( sb, sockid );
					}

					decReqConnection();
					// Call callback if needed
					if ( connectionCallback() != NULL )
					{
						connectionCallback()( sockid, argOfConnectionCallback() );
					}
					if (frontend())
						RefreshSockBuffer();
                    break;
				}
			default: // should not occur
				sipinfo( "LNETL1: Invalid block type: %hu (should be = to %hu", (uint16)(buffer[buffer.size()-1]), (uint16)(val) );
				sipinfo( "LNETL1: Buffer (%d B): [%s]", buffer.size(), stringFromVector(buffer).c_str() );
				sipinfo( "LNETL1: Receive queue:" );
				{
					CFifoAccessor recvfifo( &receiveQueue() );
					recvfifo.value().display();
				}
				siperror( "LNETL1: Invalid system event type in server receive queue" );

			}

			// Extract system event
			{
				CFifoAccessor recvfifo( &receiveQueue() );
				recvfifo.value().pop();
				setDataAvailableFlag( ! recvfifo.value().empty() );
			}
		}
		// _DataAvailable is false here
		return false;
	}
}


#ifdef SIP_OS_UNIX
/* Wait until the receive queue contains something to read (implemented with a select()).
 * This is where the connection/disconnection callbacks can be called.
 * \param usecMax Max time to wait in microsecond (up to 1 sec)
 */
void	CBufServer::sleepUntilDataAvailable( uint usecMax )
{
	// Prevent looping infinitely if the system time was changed
	if ( usecMax > 999999 ) // limit not told in Linux man but here: http://docs.hp.com/en/B9106-90009/select.2.html
		usecMax = 999999;

	fd_set readers;
	timeval tv;
	do
	{
		FD_ZERO( &readers );
		FD_SET( _DataAvailablePipeHandle[PipeRead], &readers );
		tv.tv_sec = 0;
		tv.tv_usec = usecMax;
		int res = ::select( _DataAvailablePipeHandle[PipeRead]+1, &readers, NULL, NULL, &tv );
		if ( res == -1 )
			siperror( "LNETL1: Select failed in sleepUntilDataAvailable (code %u)", CSock::getLastError() );
	}
	while ( ! dataAvailable() ); // will loop if only a connection/disconnection event was read
}
#endif

 
/*
 * Receives next block of data in the specified. The length and hostid are output arguments.
 * Precond: dataAvailable() has returned true, phostid not null
 */
void CBufServer::receive( CMemStream& buffer, TTcpSockId* phostid )
{
	//sipassert( dataAvailable() );
	sipassert( phostid != NULL );

	{
		CFifoAccessor recvfifo( &receiveQueue() );
		sipassert( ! recvfifo.value().empty() );
		recvfifo.value().front( buffer );
		recvfifo.value().pop();
		setDataAvailableFlag( ! recvfifo.value().empty() );
	}

	// Extract hostid (and event type)
	*phostid = *((TTcpSockId*)&(buffer.buffer()[buffer.size()-sizeof(TTcpSockId)-1]));
	sipassert( buffer.buffer()[buffer.size()-1] == CBufNetBase::User );

	// debug features, we number all packet to be sure that they are all sent and received
	// \todo remove this debug feature when ok
#ifdef SIP_BIG_ENDIAN
	uint32 val = SIPBASE_BSWAP32(*(uint32*)buffer.buffer());
#else
	uint32 val = *(uint32*)buffer.buffer();
#endif

	//	sipdebug ("receive message number %u", val);
	if ((*phostid)->ReceiveNextValue != val)
	{
		//sipwarning ("LNETL1: !!!LOST A MESSAGE!!! I received the message number %u but I'm waiting the message number %u (cnx %s), warn online3d@samilpo.com with the log now please", val, (*phostid)->ReceiveNextValue, (*phostid)->asString().c_str());
		// resync the message number
		(*phostid)->ReceiveNextValue = val;
	}

	(*phostid)->ReceiveNextValue++;

	buffer.resize( buffer.size()-sizeof(TTcpSockId)-1 );

	// TODO OPTIM remove the sipdebug for speed
	//commented for optimisation sipdebug( "LNETL1: Read buffer (%d+%d B) from %s", buffer.size(), sizeof(TTcpSockId)+1, /*stringFromVector(buffer).c_str(), */(*phostid)->asString().c_str() );

	// Statistics
	_BytesPoppedIn += buffer.size() + sizeof(TBlockSize);
}


/*
 * Update the network (call this method evenly)
 */
void CBufServer::update()
{
	_NbConnections = 0;

	// For each thread
	CThreadPool::iterator ipt;
	{
	  //sipdebug( "UPD: Acquiring the Thread Pool" );
		CThreadPool::iterator poolsyncBegin;
		CThreadPool::iterator poolsyncEnd;
		CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
		poolsyncBegin = poolsync.value().begin();
		poolsyncEnd = poolsync.value().end();

		//sipdebug( "UPD: Acquired." );
		for ( ipt=poolsyncBegin; ipt!=poolsyncEnd; ++ipt )
		{
			// For each thread of the pool
			CServerReceiveTask *task = receiveTask(ipt);
			CConnections::iterator ipb;
			{
				CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
				for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
				{
					CServerBufSock *serverbufsock = static_cast<CServerBufSock*>(static_cast<CBufSock*>(*ipb));

				    // For each socket of the thread, update sending
					bool	bConnected	= (*ipb)->Sock->connected();
					bool	bForceClosed= (*ipb)->Sock->forceClosed();
					bool	bUpdate = true;
					if ( bForceClosed )
						bUpdate = false;
					else
					{
						bUpdate = (*ipb)->update();
					}
				    if ( ! (bConnected && bUpdate) )
				    {
						// Update did not work or the socket is not connected anymore
						if ( bForceClosed )
					        sipdebug( "LNETL1: Socket %s is force - disconnected", (*ipb)->asString().c_str() );
						else
							sipdebug( "LNETL1: Socket %s is disconnected", (*ipb)->asString().c_str() );

						// Disconnection event if disconnected (known either from flush (in update) or when receiving data)
						// (*ipb)->advertiseDisconnection( this, *ipb );
						serverbufsock->advertiseDisconnection(*ipb);
				    }
				    else
				    {
						CServerBufSock *tcpsock = (CServerBufSock *)(*ipb);
						if (tcpsock->_UdpFriend != NULL)
							tcpsock->_UdpFriend->update();
						
						_NbConnections++;
				    }
				}
			}
		}
	}

}

uint32 CBufServer::getSendQueueSize( TTcpSockId destid )
{
	if ( destid != InvalidSockId )
	{
		if (_ConnectedClients.find(destid) == _ConnectedClients.end())
		{
			// this host is not connected
			return 0;
		}
		uint32	sendsize = destid->SendFifo.size();
		return sendsize;
	}
	else
	{
		// add all client buffers

		uint32 total = 0;

		// For each thread
		CThreadPool::iterator ipt;
		{
			CThreadPool::iterator poolsyncBegin;
			CThreadPool::iterator poolsyncEnd;
			CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
			poolsyncBegin = poolsync.value().begin();
			poolsyncEnd = poolsync.value().end();
			for ( ipt=poolsyncBegin; ipt!=poolsyncEnd; ++ipt )
			{
				// For each thread of the pool
				CServerReceiveTask *task = receiveTask(ipt);
				CConnections::iterator ipb;
				{
					CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
					for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
					{
						// For each socket of the thread, update sending
						total += (*ipb)->SendFifo.size();
					}
				}
			}
		}
		return total;
	}
}

void CBufServer::displayThreadStat (SIPBASE::CLog *log)
{
	// For each thread
	CThreadPool::iterator ipt;
	{
		CThreadPool::iterator poolsyncBegin;
		CThreadPool::iterator poolsyncEnd;
		CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
		poolsyncBegin = poolsync.value().begin();
		poolsyncEnd = poolsync.value().end();
		for ( ipt=poolsyncBegin; ipt!=poolsyncEnd; ++ipt )
		{
			// For each thread of the pool
			CServerReceiveTask *task = receiveTask(ipt);
			// For each socket of the thread, update sending
			log->displayNL ("server receive thread %p nbloop %d", task, task->NbLoop);
		}
	}

	log->displayNL ("server listen thread %p nbloop %d", _ListenTask, _ListenTask->NbLoop);
}

void CBufServer::setTimeFlushTrigger( TTcpSockId destid, sint32 ms ) 
{ 
	sipassert( destid != InvalidSockId ); 
	if (_ConnectedClients.find(destid) != _ConnectedClients.end()) 
		destid->setTimeFlushTrigger( ms ); 
}

void CBufServer::setSizeFlushTrigger( TTcpSockId destid, sint32 size ) 
{ 
	sipassert( destid != InvalidSockId ); 
	if (_ConnectedClients.find(destid) != _ConnectedClients.end()) 
		destid->setSizeFlushTrigger( size ); 
}

bool CBufServer::flush( TTcpSockId destid, uint *nbBytesRemaining)
{ 
	sipassert( destid != InvalidSockId ); 
	if (_ConnectedClients.find(destid) != _ConnectedClients.end()) 
		return destid->flush( nbBytesRemaining ); 
	else
		return true;
}
const CInetAddress& CBufServer::hostAddress( TTcpSockId hostid ) 
{ 
	sipassert( hostid != InvalidSockId ); 
	if (_ConnectedClients.find(hostid) != _ConnectedClients.end()) 
		return hostid->Sock->remoteAddr(); 

	static CInetAddress nullAddr;
	return nullAddr;
}

void CBufServer::displaySendQueueStat (SIPBASE::CLog *log, TTcpSockId destid)
{
	if ( destid != InvalidSockId )
	{
		if (_ConnectedClients.find(destid) == _ConnectedClients.end())
		{
			// this host is not connected
			return;
		}
		{
			destid->SendFifo.displayStats(log);
		}
		
	}
	else
	{
		// add all client buffers
		
		// For each thread
		CThreadPool::iterator ipt;
		{
			CThreadPool::iterator poolsyncBegin;
			CThreadPool::iterator poolsyncEnd;
			CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
			poolsyncBegin = poolsync.value().begin();
			poolsyncEnd = poolsync.value().end();
			for ( ipt=poolsyncBegin; ipt!=poolsyncEnd; ++ipt )
			{
				// For each thread of the pool
				CServerReceiveTask *task = receiveTask(ipt);
				CConnections::iterator ipb;
				{
					CSynchronized<CConnections>::CAccessor connectionssync( &task->_Connections );
					for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
					{
						// For each socket of the thread, update sending
						(*ipb)->SendFifo.displayStats(log);
					}
				}
			}
		}
	}
}


/*
 * Returns the number of bytes received since the previous call to this method
 */
uint64 CBufServer::newBytesReceived()
{
	uint64 b = bytesReceived();
	uint64 nbrecvd = b - _PrevBytesPoppedIn;
	_PrevBytesPoppedIn = b;
	return nbrecvd;
}

/*
 * Returns the number of bytes sent since the previous call to this method
 */
uint64 CBufServer::newBytesSent()
{
	uint64 b = bytesSent();
	uint64 nbsent = b - _PrevBytesPushedOut;
	_PrevBytesPushedOut = b;
	return nbsent;
}

uint32	CBufServer::QueueIncomingConnections() const
{
	return _RQConnections;
}

/***************************************************************************************************
 * Listen thread
 **************************************************************************************************/


/*
 * Code of listening thread
 */
void CListenTask::run()
{
	NbNetworkTask++;
	NbServerListenTask++;

	fd_set readers;
#ifdef SIP_OS_UNIX
	SOCKET descmax;
	descmax = _ListenSock.descriptor()>_WakeUpPipeHandle[PipeRead]?_ListenSock.descriptor():_WakeUpPipeHandle[PipeRead];
#endif

	// Accept connections
	while ( ! exitRequired() )
	{
		try
		{
			sipdebug( "LNETL1: Waiting incoming connection..." );
			// Get and setup the new socket
#ifdef SIP_OS_UNIX
			FD_ZERO( &readers );
			FD_SET( _ListenSock.descriptor(), &readers );
			FD_SET( _WakeUpPipeHandle[PipeRead], &readers );
			int res = ::select( descmax+1, &readers, NULL, NULL, NULL ); /// Wait indefinitely

			switch ( res )
			{
			//case  0 : continue; // time-out expired, no results
			case -1 :
				// we'll ignore message (Interrupted system call) caused by a CTRL-C
				if (CSock::getLastError() == 4)
				{
					sipdebug ("LNETL1: Select failed (in listen thread): %s (code %u) but IGNORED", CSock::errorString( CSock::getLastError() ).c_str(), CSock::getLastError());
					continue;
				}
				siperror( "LNETL1: Select failed (in listen thread): %s (code %u)", CSock::errorString( CSock::getLastError() ).c_str(), CSock::getLastError() );
			}

			if ( FD_ISSET( _WakeUpPipeHandle[PipeRead], &readers ) )
			{
				uint8 b;
				if ( read( _WakeUpPipeHandle[PipeRead], &b, 1 ) == -1 ) // we were woken-up by the wake-up pipe
				{
					sipdebug( "LNETL1: In CListenTask::run(): read() failed" );
				}
				sipdebug( "LNETL1: listen thread select woken-up" );
				continue;
			}
#elif defined SIP_OS_WINDOWS
			FD_ZERO( &readers );
			FD_SET( _ListenSock.descriptor(), &readers );
			int res = ::select( 1, &readers, NULL, NULL, NULL ); /// Wait indefinitely

			if ( res == -1)
			{
				siperror( "LNETL1: Select failed (in listen thread): %s (code %u)", CSock::errorString( CSock::getLastError() ).c_str(), CSock::getLastError() );
				continue;
			}
#endif
			sipdebug( "LNETL1: Accepting an incoming connection..." );
			CTcpSock *newSock = _ListenSock.accept();
			if (newSock != NULL)
			{
				newSock->setKeepAlive(true);
				CServerBufSock *bufsock = new CServerBufSock( newSock );
				if (_Server->_UdpServer)
				{
					CBufUdpServer *bufudpsock = new CBufUdpServer( _Server->_UdpServerSock, CInetAddress());
					bufudpsock->TcpFriend = bufsock;
					bufsock->_UdpFriend = bufudpsock;
				}

				sipdebug( "LNETL1: New connection : %s", bufsock->asString().c_str() );
				bufsock->setNonBlocking();
				bufsock->setMaxExpectedBlockSize( _MaxExpectedBlockSize );
				if ( _Server->noDelay() )
				{
					bufsock->Sock->setNoDelay( true );
				}

				// Notify the new connection
				bufsock->advertiseConnection( _Server );
				_Server->incReqConnection();

				// Dispatch the socket into the thread pool
				_Server->dispatchNewSocket( bufsock );
			}

			NbLoop++;
		}
		catch ( ESocket& e )
		{
			sipinfo( "LNETL1: Exception in listen thread: %s", e.what() ); 
			// It can occur when too many sockets are open (e.g. 885 connections)
		}
	}

	NbServerListenTask--;
	NbNetworkTask--;
}

/// Close listening socket
void CListenTask::close() 
{ 
	_ListenSock.close();
//	_ListenSock.disconnect();
}


/*
 * Binds a new socket and send buffer to an existing or a new thread
 * Note: this method is called in the listening thread.
 */
void CBufServer::dispatchNewSocket( CServerBufSock *bufsock )
{
	CSynchronized<CThreadPool>::CAccessor poolsync( &_ThreadPool );
	if ( _ThreadStrategy == SpreadSockets )	
	{
		// Find the thread with the smallest number of connections and check if all
		// threads do not have the same number of connections
		uint min = 0xFFFFFFFF;
		uint max = 0;
		CThreadPool::iterator ipt, iptmin, iptmax;
		for ( iptmin=iptmax=ipt=poolsync.value().begin(); ipt!=poolsync.value().end(); ++ipt )
		{
			uint noc = receiveTask(ipt)->numberOfConnections();
			if ( noc < min )
			{
				min = noc;
				iptmin = ipt;
			}
			if ( noc > max )
			{
				max = noc;
				iptmax = ipt;
			}
		}

		// Check if we make the pool of threads grow (if we have not found vacant room
		// and if it is allowed to)
		if ( (poolsync.value().empty()) ||
			 ((min == max) && (poolsync.value().size() < _MaxThreads)) )
		{
			addNewThread( poolsync.value(), bufsock );
		}
		else
		{
			// Dispatch socket to an existing thread of the pool
			CServerReceiveTask *task = receiveTask(iptmin);
			bufsock->setOwnerTask( task );
			task->addNewSocket( bufsock );
#ifdef SIP_OS_UNIX
			task->wakeUp();
#endif			
			
			if ( min >= (uint)_MaxSocketsPerThread )
			{
				sipwarning( "LNETL1: Exceeding the maximum number of sockets per thread" );
			}
			sipdebug( "LNETL1: New socket dispatched to thread %d", iptmin-poolsync.value().begin() );
		}

	}
	else // _ThreadStrategy == FillThreads
	{
		CThreadPool::iterator ipt;
		for ( ipt=poolsync.value().begin(); ipt!=poolsync.value().end(); ++ipt )
		{
			uint noc = receiveTask(ipt)->numberOfConnections();
			if ( noc < _MaxSocketsPerThread )
			{
				break;
			}
		}

		// Check if we have to make the thread pool grow (if we have not found vacant room)
		if ( ipt == poolsync.value().end() )
		{
			if ( poolsync.value().size() == _MaxThreads )
			{
				sipwarning( "LNETL1: Exceeding the maximum number of threads" );
			}
			addNewThread( poolsync.value(), bufsock );
		}
		else
		{
			// Dispatch socket to an existing thread of the pool
			CServerReceiveTask *task = receiveTask(ipt);
			bufsock->setOwnerTask( task );
			task->addNewSocket( bufsock );
#ifdef SIP_OS_UNIX
			task->wakeUp();
#endif			
			sipdebug( "LNETL1: New socket dispatched to thread %d", ipt-poolsync.value().begin() );
		}
	}
}


/*
 * Creates a new task and run a new thread for it
 * Precond: bufsock not null
 */
void CBufServer::addNewThread( CThreadPool& threadpool, CServerBufSock *bufsock )
{
	sipassert( bufsock != NULL );

	// Create new task and dispatch the socket to it
	CServerReceiveTask *task = new CServerReceiveTask( this );
	bufsock->setOwnerTask( task );
	task->addNewSocket( bufsock );

	// Add a new thread to the pool, with this task
	IThread *thr = IThread::create( task, 1024*4*4 );
	{
		threadpool.push_back( thr );
		thr->start();
		sipdebug( "LNETL1: Added a new thread; pool size is %d", threadpool.size() );
		sipdebug( "LNETL1: New socket dispatched to thread %d", threadpool.size()-1 );
	}
}

/***************************************************************************************************
 * Receive threads
 **************************************************************************************************/
/*
 * Code of receiving threads for servers
 */
void CServerReceiveTask::run()
{
	NbNetworkTask++;
	NbServerReceiveTask++;

	SOCKET descmax;
	fd_set readers;

#if defined SIP_OS_UNIX
	// POLL7
	nice( 2 ); // is this really useful as long as select() sleeps?
#endif // SIP_OS_UNIX
	
	// Copy of _Connections
	vector<TTcpSockId>	connections_copy;	

	while ( ! exitRequired() )
	{
		// 1. Remove closed connections
		clearClosedConnections();

		// POLL8

		// 2-SELECT-VERSION : select() on the sockets handled in the present thread

		descmax = 0;
		FD_ZERO( &readers );
		bool skip;
		bool alldisconnected = true;
		CConnections::iterator ipb;
		{
			// Lock _Connections
			CSynchronized<CConnections>::CAccessor connectionssync( &_Connections );

			// Prepare to avoid select if there is no connection
			skip = connectionssync.value().empty();

			// Fill the select array and copy _Connections
			connections_copy.clear();
			for ( ipb=connectionssync.value().begin(); ipb!=connectionssync.value().end(); ++ipb )
			{
				if ( (*ipb)->Sock->connected() ) // exclude disconnected sockets that are not deleted
				{
					if ( (*ipb)->Sock->forceClosed())
					{
						sipwarning("LNETL1: socket %s is forceclosed, so restrict to not selecting for recieve message", (*ipb)->asString().c_str());
						continue;
					}

					alldisconnected = false;
					// Copy _Connections element
					connections_copy.push_back( *ipb );

					// Add socket descriptor to the select array
					FD_SET( (*ipb)->Sock->descriptor(), &readers );

					// Calculate descmax for select
					if ( (*ipb)->Sock->descriptor() > descmax )
					{
						descmax = (*ipb)->Sock->descriptor();
					}
				}
			}

#ifdef SIP_OS_UNIX
			// Add the wake-up pipe into the select array
			FD_SET( _WakeUpPipeHandle[PipeRead], &readers );
			if ( _WakeUpPipeHandle[PipeRead]>descmax )
			{
				descmax = _WakeUpPipeHandle[PipeRead];
			}
#endif
			
			// Unlock _Connections, use connections_copy instead
		}

#ifndef SIP_OS_UNIX
		// Avoid select if there is no connection (Windows only)
		if ( skip || alldisconnected )
		{
			sipSleep( 1 ); // nice
			continue;
		}
#endif

#ifdef SIP_OS_WINDOWS
		TIMEVAL tv;
		tv.tv_sec = 0; // short time because the newly added connections can't be added to the select fd_set
		tv.tv_usec = 10000;

		// Call select
		int res = ::select( descmax+1, &readers, NULL, NULL, &tv );

#elif defined SIP_OS_UNIX

		// Call select
		int res = ::select( descmax+1, &readers, NULL, NULL, NULL );

#endif // SIP_OS_WINDOWS


		// POLL9

		// 3. Test the result
		switch ( res )
		{
#ifdef SIP_OS_WINDOWS
			case  0 : continue; // time-out expired, no results
#endif
			/// \todo the error code is not properly retrieved
			case -1 :
				// we'll ignore message (Interrupted system call) caused by a CTRL-C
				/*if (CSock::getLastError() == 4)
				{
					sipdebug ("LNETL1: Select failed (in receive thread): %s (code %u) but IGNORED", CSock::errorString( CSock::getLastError() ).c_str(), CSock::getLastError());
					continue;
				}*/
				//siperror( "LNETL1: Select failed (in receive thread): %s (code %u)", CSock::errorString( CSock::getLastError() ).c_str(), CSock::getLastError() );
				sipwarning( "LNETL1: Select failed (in receive thread): %s (code %u)", CSock::errorString( CSock::getLastError() ).c_str(), CSock::getLastError() );
				goto end;
		}

		// 4. Get results

		vector<TTcpSockId>::iterator ic;
		for ( ic=connections_copy.begin(); ic!=connections_copy.end(); ++ic )
		{
			if ( FD_ISSET( (*ic)->Sock->descriptor(), &readers ) != 0 )
			{
				CServerBufSock *serverbufsock = static_cast<CServerBufSock*>(static_cast<CBufSock*>(*ic));
				try
				{
					if ( serverbufsock->forceClosed())
					{
						sipwarning("LNETL1: socket %s is forceclosed, so restrict to not recieve message", serverbufsock->asString().c_str());
						continue;
					}
					// 4. Receive data
					if ( serverbufsock->receivePart( sizeof(TTcpSockId) + 1 ) ) // +1 for the event type
					{
						TGetMsgCheckNeedCallback		GetMsgCheckNeedCallback = _Server->getGetMsgCheckNeedCallback();
						if (GetMsgCheckNeedCallback != NULL)
						{
							uint32	uMesID = 0, uIntervalTime = 0, userialMaxNum = 0;
							MsgNameType	sMesID;
							bool	bCheckNeed = GetMsgCheckNeedCallback(serverbufsock->receivedBuffer(), &uMesID, &sMesID, &uIntervalTime, &userialMaxNum);
							if (bCheckNeed)
							{
								bool	bAccept = (*ic)->procAcceptMsg(uMesID, (TTime)uIntervalTime, userialMaxNum);
								if (!bAccept)
								{
								/*
									TProcUnlawSerialCallback ProcUnlawSerialCallback = _Server->getProcUnlawSerialCallback();
									if (ProcUnlawSerialCallback != NULL)
									{
										ProcUnlawSerialCallback(sMesID, *ic);
									}
									_Server->forceClose(*ic, true);
								*/
#ifdef SIP_MSG_NAME_DWORD
									sipwarning("LNETL1: Unlaw serial packet: Socket=%p, Packet=%d", *ic, sMesID);
#else
									sipwarning("LNETL1: Unlaw serial packet: Socket=%p, Packet=%s", *ic, sMesID.c_str());
#endif // SIP_MSG_NAME_DWORD
								
								//	continue;
								}
							}
						}
						serverbufsock->fillSockIdAndEventType( *ic );

						// _Server->pushMessageIntoReceiveQueue( serverbufsock->receivedBuffer() );
						serverbufsock->pushMessageIntoReceiveQueue( serverbufsock->receivedBuffer() );
						
						//_Server->displayReceiveQueueStat();
						//bufsize = serverbufsock->receivedBuffer().size();
						//mbsize = recvfifo.value().size() / 1048576;
						//sipdebug( "RCV: Released." );
						/*if ( mbsize > 1 )
						{
							sipwarning( "The receive queue size exceeds %d MB", mbsize );
						}*/
						/*
						// Statistics
						{
							CSynchronized<uint32>::CAccessor syncbpi ( &_Server->syncBytesPushedIn() );
							syncbpi.value() += bufsize;
						}
						*/
					}
				}
//				catch ( ESocketConnectionClosed& )
//				{
//					sipdebug( "LNETL1: Connection %s closed", serverbufsock->asString().c_str() );
//					// The socket went to _Connected=false when throwing the exception
//				}
				catch ( ESocket& )
				{
					sipwarning( "LNETL1: Connection %s broken", serverbufsock->asString().c_str() );
					(*ic)->Sock->disconnect();
				}
/*
#ifdef SIP_OS_UNIX
				skip = true; // don't check _WakeUpPipeHandle (yes, check it to read any written byte)
#endif

*/
			}
		}

#ifdef SIP_OS_UNIX
		// Test wake-up pipe
		if ( (!skip) && (FD_ISSET( _WakeUpPipeHandle[PipeRead], &readers )) )
		{
			uint8 b;
			if ( read( _WakeUpPipeHandle[PipeRead], &b, 1 ) == -1 ) // we were woken-up by the wake-up pipe
			{
				sipdebug( "LNETL1: In CServerReceiveTask::run(): read() failed" );
			}
			sipdebug( "LNETL1: Receive thread select woken-up" );
		}
#endif

		NbLoop++;
	}
end:
	NbServerReceiveTask--;
	NbNetworkTask--;
}


/*
 * Delete all connections referenced in the remove list (double-mutexed)
 */

void CServerReceiveTask::clearClosedConnections()
{
	CConnections::iterator ic;
	{
		SIPBASE::CSynchronized<CConnections>::CAccessor removesetsync( &_RemoveSet );
		{
			if ( ! removesetsync.value().empty() )
			{
				// Delete closed connections
				SIPBASE::CSynchronized<CConnections>::CAccessor connectionssync( &_Connections );
				for ( ic=removesetsync.value().begin(); ic!=removesetsync.value().end(); ++ic )
				{
					TTcpSockId sid = (*ic);
					sipdebug("LNETL1: Removing a connection ( %s )", sid->asString().c_str());

					// Remove from the connection list
					connectionssync.value().erase( *ic );

					CServerBufSock *tcpsock = (CServerBufSock *)sid;
					if (tcpsock->_UdpFriend != NULL)
						delete tcpsock->_UdpFriend;

					// Delete the socket object
					delete sid;
				}
				// Clear remove list
				removesetsync.value().clear();
			}
		}
	}
}



void CUdpReceiveTask::init( uint16 port )
{
	_Server->_UdpServerSock->bind( port );
}

void CUdpReceiveTask::run()
{
	CInetAddress addr;
	uint8 buffer[MAX_UDPDATAGRAMLEN];
	while ( ! exitRequired() )
	{
		if (_Server->_UdpServerSock->UdpSock.dataAvailable())
		{
			uint len = MAX_UDPDATAGRAMLEN;
			try
			{
				_Server->_UdpServerSock->UdpSock.receivedFrom((uint8*)buffer, len, addr);
			}
			catch (Exception &e)
			{
				sipwarning ("LNETL1: UDP receive exception catched: '%s'", e.what());
				sipSleep(1);
				continue;
			}
			if (len < UDPMESPOS+sizeof(TBlockSize))
			{
				// \C6\C4\C4\C9Ʈ\C0\C7 \B1\B8\BC\BA\C0\CC \BA\F1\C1\A4\BB\F3\C0̴\D9
				sipwarning ("LNETL1: UDP received is invalid length");
				sipSleep(1);
				continue;
			}

			TTcpSockId sock = (TTcpSockId)(uintptr_t)buffer;
			CSynchronized<TTcpAndUdp>::CAccessor tcpAndUdp( &_Server->_TcpAndUdpMap );
			map<TTcpSockId, TUdpSockId>::iterator sockit = tcpAndUdp.value().find(sock);
			if (sockit == tcpAndUdp.value().end())
			{
				sipwarning ("LNETL1: UDP received don't have own tcp.");
				sipSleep(1);
				continue;
			}
			sockit->second->setClientAddress(addr);
			sockit->second->AddRecvPacket(buffer, len);
			sockit->second->RefreshRecvAllPacket();
			while ( sockit->second->receivePart( sizeof(TTcpSockId) + 1 ) ) // +1 for the event type
			{
				TTcpSockId tid = sockit->second->TcpFriend;
				if (tid != NULL)
				{
					TGetMsgCheckNeedCallback		GetMsgCheckNeedCallback = _Server->getGetMsgCheckNeedCallback();
					if (GetMsgCheckNeedCallback != NULL)
					{
						uint32	uMesID = 0, uIntervalTime = 0, userialMaxNum = 0;
						MsgNameType	sMesID;
						bool	bCheckNeed = GetMsgCheckNeedCallback(sockit->second->receivedBuffer(), &uMesID, &sMesID, &uIntervalTime, &userialMaxNum);
						if (bCheckNeed)
						{
							bool	bAccept = tid->procAcceptMsg(uMesID, (TTime)uIntervalTime, userialMaxNum);
							if (!bAccept)
							{
								TProcUnlawSerialCallback ProcUnlawSerialCallback = _Server->getProcUnlawSerialCallback();
								if (ProcUnlawSerialCallback != NULL)
								{
									ProcUnlawSerialCallback(sMesID, tid);
								}
								_Server->forceClose(tid);
#ifdef SIP_MSG_NAME_DWORD
								sipwarning("LNETL1: Client is force closed for serial packet: Socket=%p, Packet=%d", tid, sMesID);
#else
								sipwarning("LNETL1: Client is force closed for serial packet: Socket=%p, Packet=%s", tid, sMesID.c_str());
#endif // SIP_MSG_NAME_DWORD
								continue;
							}
						}
					}

					sockit->second->fillSockIdAndEventType( tid );
					
					_Server->pushMessageIntoReceiveQueue( sockit->second->receivedBuffer() );
					// CServerBufSock *serverbufsock = static_cast<CServerBufSock*>(static_cast<CBufSock*>(tid));
					// serverbufsock->pushMessageIntoReceiveQueue( sockit->second->receivedBuffer() );
				}
				else
				{
					// \B4\EB\C0\C0\C0\CC \C0̷\E7\BE\EE\C1\F6\C1\F6 \BE\CA\C0\BA UDP\C5\EB\BD\C5\C0\CC \C1\B8\C0\E7\C7Ѵ\D9
				}
			}
		}
		else
			sipSleep(1);
	}
}

SIPBASE_CATEGORISED_VARIABLE(sip, uint32, NbServerListenTask, "Number of server listen thread");
SIPBASE_CATEGORISED_VARIABLE(sip, uint32, NbServerReceiveTask, "Number of server receive thread");

} // SIPNET
