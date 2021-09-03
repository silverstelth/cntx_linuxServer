/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "misc/hierarchical_timer.h"

#include "net/buf_client.h"
#include "misc/thread.h"
#include "net/dummy_tcp_sock.h"

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#elif defined SIP_OS_UNIX
#include <netinet/in.h>
#endif

using namespace SIPBASE;
using namespace std;


namespace SIPNET {


uint32 	NbClientReceiveTask = 0;
	
/***************************************************************************************************
 * User main thread (initialization)
 **************************************************************************************************/

/*
 * Constructor
 */
CBufClient::CBufClient( bool nodelay, bool replaymode, bool initPipeForDataAvailable, bool bHasUdp, bool bNoRecieve ) :
#ifdef SIP_OS_UNIX
	CBufNetBase( initPipeForDataAvailable ),
#else
	CBufNetBase(),
#endif
	_NoDelay( nodelay ),
	_NoRecieve(bNoRecieve),
	_PrevBytesDownloaded( 0 ),
	_PrevBytesUploaded( 0 ),
	_RecvTask( NULL ),
	_RecvThread( NULL ),
	_RecvTaskUdp( NULL ),
	_RecvThreadUdp( NULL ),
	_RecvTcpConfirm(false),
	_FinishTcpConfirm(false)

	/*_PrevBytesReceived( 0 ),
	_PrevBytesSent( 0 )*/
{
	if ( replaymode )
	{
		_BufSock = new CNonBlockingBufSock( new CDummyTcpSock(), CBufNetBase::DefaultMaxExpectedBlockSize );
	}
	else
	{
		_BufSock = new CNonBlockingBufSock( NULL, CBufNetBase::DefaultMaxExpectedBlockSize );
		if (bHasUdp == true)
		{
			_BufUdpSock = new CBufUdpClient();
			_RecvTaskUdp = new CClientReceiveTaskUdp( this, _BufSock, _BufUdpSock );
		}
		else
			_BufUdpSock = NULL;

		if ( !_NoRecieve )
            _RecvTask = new CClientReceiveTask( this, _BufSock, _BufUdpSock );
		
	}
	if (_BufSock)
	{
		_BufSock->setTimeFlushTrigger(0);
	}
}


/*
 * Connects to the specified host
 * Precond: not connected
 */
void CBufClient::connect( const CInetAddress& addr, uint32 usec )
{
	sipassert( ! _BufSock->Sock->connected() );
	_BufSock->setMaxExpectedBlockSize( maxExpectedBlockSize() );
	_BufSock->connect( addr, _NoDelay, true, usec );
	_BufSock->setNonBlocking(); // ADDED: non-blocking client connection

	if (_BufUdpSock != NULL)
	{
		_BufUdpSock->connect(addr);
		_BufUdpSock->setNonBlocking();
	}

	_PrevBytesDownloaded = 0;
	_PrevBytesUploaded = 0;
	/*_PrevBytesReceived = 0;
	_PrevBytesSent = 0;*/

	// Allow reconnection
	if ( _RecvThread != NULL )
	{
		delete _RecvThread;
	}
	if ( _RecvThreadUdp != NULL )
	{
		delete _RecvThreadUdp;
	}
	
	if ( !_NoRecieve )
	{
		_RecvThread = IThread::create( _RecvTask, 1024*4*4 );
		_RecvThread->start();
	}
	
	if (_RecvTaskUdp)
	{
		_RecvThreadUdp = IThread::create( _RecvTaskUdp, 1024*4*4 );
		_RecvThreadUdp->start();
	}	
}


/***************************************************************************************************
 * User main thread (running)
 **************************************************************************************************/

void CBufClient::displayThreadStat (SIPBASE::CLog *log)
{
	if ( !_NoRecieve )
		log->displayNL ("client thread %p nbloop %d", _RecvTask, _RecvTask->NbLoop);
}


/*
 * Sends a message to the remote host
 */
void CBufClient::send( const SIPBASE::CMemStream& buffer, uint8 bySendType )
{
	sipassert( buffer.length() > 0 );
	sipassert( buffer.length() <= maxSentBlockSize() );

	bool	bUdp = true;
	if (bySendType == REL_GOOD_ORDER )
		bUdp = false;

	// slow down the layer H_AUTO (CBufServer_send);
	if (bUdp == false)
	{
		_BufSock->pushBuffer( buffer );
	}
	else
	{
		if (_FinishTcpConfirm == true)
		{
			sipassert(_BufUdpSock != NULL);
			_BufUdpSock->pushBuffer(bySendType, buffer);
		}
		else
			sipwarning("LNETL1: Can't data on udp, because there is no tcp confirm!!!");
	}
}


/*
 * Checks if there are some data to receive
 */
bool CBufClient::dataAvailable()
{
	// slow down the layer H_AUTO (CBufClient_dataAvailable);
	{
		/* If no data available, enter the 'while' loop and return false (1 volatile test)
		 * If there are user data available, enter the 'while' and return true immediately (1 volatile test + 1 short locking)
		 * If there is a disconnection event (rare), call the callback and loop
		 */
		while ( dataAvailableFlag() )
		{
			// Because _DataAvailable is true, the receive queue is not empty at this point
			uint8 val;
			{
				CFifoAccessor recvfifo( &receiveQueue() );
				// displayReceiveQueue(); // KSR ADD
				val = recvfifo.value().frontLast ();
			}

#ifdef SIP_OS_UNIX
			uint8 b;
			if ( read( _DataAvailablePipeHandle[PipeRead], &b, 1 ) == -1 )
				sipwarning( "LNETL1: Read pipe failed in dataAvailable" );
			//sipdebug( "Pipe: 1 byte read (client %p)", this );
#endif

			// Test if it the next block is a system event
			switch ( val )
			{
				
			// Normal message available
			case CBufNetBase::User:
				// UDP 소켓이 TCP와의 쌍을 이루었는가를 검토한다
				{
					bool bReturn = true;
					if (_BufUdpSock != NULL)
					{
						// 써버로부터 TCP ID를 받지 못하였으면
						if (_FinishTcpConfirm != true)
						{
							CMemStream buffer(true);
							CFifoAccessor recvfifo( &receiveQueue() );
							sipassert( ! recvfifo.value().empty() );
							recvfifo.value().front( buffer );
							if (buffer.size() == nUdpAndTcpConnectionPSize + 1)
							{
								string	strP;
								uint32	uID;
								buffer.serial(strP, uID);
								if (strP == UDPCONNECTION)
								{
									bReturn = false;
									_FinishTcpConfirm = true;
									_BufUdpSock->idInServer = uID;
								}
								else
								{
									// 련결인증이 되지 않았는데 파케트가 왔다
								}
							}
						}
					}
					if (bReturn)
						return true; // return immediatly, do not extract the message
				}
				break;
			// Process disconnection event
			case CBufNetBase::Disconnection:

				sipdebug( "LNETL1: Disconnection event" );
				_BufSock->setConnectedState( false );

				// Call callback if needed
				if ( disconnectionCallback() != NULL )
				{
					disconnectionCallback()( id(), argOfDisconnectionCallback() );
				}

				// Unlike the server version, we do not delete the CBufSock object here,
				// it will be done in the destructor of CBufClient

				break;
			default: // should not occur
				{
					CFifoAccessor recvfifo( &receiveQueue() );
					vector<uint8> buffer;
					recvfifo.value().front (buffer);
					sipinfo( "LNETL1: Invalid block type: %hu (should be = %hu)", (uint16)(buffer[buffer.size()-1]), (uint16)val );
					sipinfo( "LNETL1: Buffer (%d B): [%s]", buffer.size(), stringFromVector(buffer).c_str() );
					sipinfo( "LNETL1: Receive queue:" );
					recvfifo.value().display();
					siperror( "LNETL1: Invalid system event type in client receive queue" );
				}
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
void	CBufClient::sleepUntilDataAvailable( uint usecMax )
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
 * Receives next block of data in the specified buffer (resizes the vector)
 * Precond: dataAvailable() has returned true
 */
void CBufClient::receive( SIPBASE::CMemStream& buffer )
{
	//sipassert( dataAvailable() );

	// Extract buffer from the receive queue
	{
		CFifoAccessor recvfifo( &receiveQueue() );
		//recvfifo.value().display();
		sipassert( ! recvfifo.value().empty() );
		recvfifo.value().front( buffer );
		recvfifo.value().pop();
		setDataAvailableFlag( ! recvfifo.value().empty() );
	}

	// Extract event type
	sipassert( buffer.buffer()[buffer.size()-1] == CBufNetBase::User );
	//commented for optimisation sipdebug( "LNETL1: Client read buffer (%d+%d B)", buffer.size(), sizeof(TTcpSockId)+1 );
	buffer.resize( buffer.size()-1 );
}


/*
 * Update the network (call this method evenly)
 */
void CBufClient::update()
{
	// Update sending
	if (_BufUdpSock != NULL)
	{
		if (_FinishTcpConfirm == true)
			_BufUdpSock->update();
	}

	_BufSock->update();

	// Disconnection event if disconnected
	if ( !_BufSock->Sock->connected())
	{
		_BufSock->advertiseDisconnection( this, NULL );
	}
}


/*
 * Disconnect the remote host
 */
void CBufClient::disconnect( bool quick )
{
	// Do not allow to disconnect a socket that is not connected
	//sipassert( _BufSock->connectedState() );

	// When the NS tells us to remove this connection AND the connection has physically
	// disconnected but not yet logically (i.e. disconnection event not processed yet),
	// skip flushing and physical active disconnection
	if ( _BufSock->Sock->connected() )
	{
		// Flush sending is asked for
		if ( ! quick )
		{
			_BufSock->flush();
		}

		// Disconnect and prevent from advertising the disconnection
		_BufSock->disconnect( false );
	}

	// Empty the receive queue
	{
		CFifoAccessor recvfifo( &receiveQueue() );
		recvfifo.value().clear();
		setDataAvailableFlag( false );
	}
}

void	CBufClient::forceClose( bool quick )
{
	if ( _BufSock->Sock->connected() )
	{
		// Flush sending is asked for
		if ( ! quick )
		{
			_BufSock->flush();
		}

		// Disconnect and prevent from advertising the disconnection
		_BufSock->forceClose();
	}

	// Empty the receive queue
	{
		CFifoAccessor recvfifo( &receiveQueue() );
		recvfifo.value().clear();
		setDataAvailableFlag( false );
	}
}

void	CBufClient::stopReceive()
{
	if ( _RecvThread != NULL )
	{
		_RecvTask->requireExit();
		_RecvThread->wait();
	}
	if ( _RecvTask != NULL )
		delete _RecvTask;

	if ( _RecvThread != NULL )
		delete _RecvThread;

	_RecvThread	= NULL;
	_RecvTask	= NULL;
	clearReceiveQueue();
}

// Utility function for newBytes...()
inline uint64 updateStatCounter( uint64& counter, uint64 newvalue )
{
	uint64 result = newvalue - counter;
	counter = newvalue;
	return result;
}


/*
 * Returns the number of bytes downloaded since the previous call to this method
 */
uint64 CBufClient::newBytesDownloaded()
{
	return updateStatCounter( _PrevBytesDownloaded, bytesDownloaded() );
}


/*
 * Returns the number of bytes uploaded since the previous call to this method
 */
uint64 CBufClient::newBytesUploaded()
{
	return updateStatCounter( _PrevBytesUploaded, bytesUploaded() );
}


/*
 * Returns the number of bytes popped by receive() since the previous call to this method
 */
/*uint64 CBufClient::newBytesReceived()
{
	return updateStatCounter( _PrevBytesReceived, bytesReceived() );
}*/


/*
 * Returns the number of bytes pushed by send() since the previous call to this method
 */
/*uint64 CBufClient::newBytesSent()
{
	return updateStatCounter( _PrevBytesSent, bytesSent() );
}*/


/*
 * Destructor
 */
CBufClient::~CBufClient()
{
	// Disconnect if not done
	// He disconnect me(founded in update thread), but I don't disconnect my socket(in recieve thread)
	if ( _BufSock->Sock->connected() )
	{
		//sipassert( _BufSock->connectedState() );

		disconnect( true );
	}

	// Clean thread termination
	if ( _RecvThread != NULL )
	{
		_RecvThread->wait();
	}
	if ( _RecvThreadUdp != NULL )
	{
		_RecvThreadUdp->wait();
	}

	if ( _RecvTask != NULL )
		delete _RecvTask;

	if ( _RecvThread != NULL )
		delete _RecvThread;

	if ( _RecvTaskUdp != NULL )
		delete _RecvTaskUdp;

	if ( _RecvThreadUdp != NULL )
		delete _RecvThreadUdp;

	if ( _BufSock != NULL )
		delete _BufSock;

	if ( _BufUdpSock != NULL )
		delete _BufUdpSock;

}


/***************************************************************************************************
 * Receive thread 
 **************************************************************************************************/


/*
 * Code of receiving thread for clients
 */
void CClientReceiveTask::run()
{
	NbClientReceiveTask++;
	NbNetworkTask++;

	// 18/08/2005 : sonix : Changed time out from 60s to 1s, in some case, it
	//						can generate a 60 s wait on destruction of the CBufSock
	//						By the way, checking every 1s is not a time consuming
	_NBBufSock->Sock->setTimeOutValue( 1, 0 );

	bool connected = true;
	while ( ! exitRequired() &&connected && _NBBufSock->Sock->connected())
	{
		try
		{
			// Wait until some data are received (sleepin' select inside)
			TTime	startSelect = CTime::getLocalTime();
			while ( ! _NBBufSock->Sock->dataAvailable() )
			{
				if ( ! _NBBufSock->Sock->connected() )
				{
					sipdebug( "LNETL1: Client connection %s closed", sockId()->asString().c_str() );
					// The socket went to _Connected=false when throwing the exception
					connected = false;
					break;
				}
				if ( exitRequired())
					break;
				if( CTime::getLocalTime() - startSelect > 60000 )
					break;
			}
			if ( exitRequired())
				break;

			// Process the data received
			if ( _NBBufSock->receivePart( 1 ) ) // 1 for the event type
			{
				//commented out for optimisation: sipdebug( "LNETL1: Client %s received buffer (%u bytes)", _SockId->asString().c_str(), buffer.size()/*, stringFromVector(buffer).c_str()*/ );
				// Add event type
				_NBBufSock->fillEventTypeOnly();

				// Push message into receive queue
				_Client->pushMessageIntoReceiveQueue( _NBBufSock->receivedBuffer() );
			}
			NbLoop++;
		}
		catch ( ESocket& )
		{
			sipdebug( "LNETL1: Client connection %s broken", sockId()->asString().c_str() );
			sockId()->Sock->disconnect();
			connected = false;
		}
	}

	NbClientReceiveTask--;
	NbNetworkTask--;
}

void CClientReceiveTaskUdp::run()
{
	bool connected = true;
	uint8	packet[MAX_UDPDATAGRAMLEN];
	while ( _NBBufSock->Sock->connected())
	{
		uint32	psize = MAX_UDPDATAGRAMLEN;
		if (_BufUdpSock != NULL)
		{
			if (_BufUdpSock->Sock->dataAvailable())
			{
				psize = MAX_UDPDATAGRAMLEN;
				try
				{
					_BufUdpSock->Sock->receive (packet, psize);
					if (psize > 0)
					{
						_BufUdpSock->AddRecvPacket(packet, psize);
						_BufUdpSock->RefreshRecvAllPacket();

					}
					//					sipdebug ("Received UDP datagram size %d bytes from %s", psize, UdpSock->localAddr().asString().c_str());
				}
				catch ( Exception& e )
				{
					sipwarning ("Received failed: %s", e.what());
				}
				while (_BufUdpSock->receivePart(1))
				{
					_BufUdpSock->fillEventTypeOnly();
					_Client->pushMessageIntoReceiveQueue( _BufUdpSock->receivedBuffer() );
				}
			}
			else
				sipSleep(1);
		}
		NbLoop++;
	}
}
SIPBASE_CATEGORISED_VARIABLE(sip, uint32, NbClientReceiveTask, "Number of client receive thread");



} // SIPNET
