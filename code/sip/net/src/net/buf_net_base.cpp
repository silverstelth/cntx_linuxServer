/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/buf_net_base.h"

using namespace SIPBASE;
using namespace std;


namespace SIPNET {

uint32 	NbNetworkTask = 0;

// The value that will be used if setMaxExpectedBlockSize() is not called (or called with a negative argument)
uint32 CBufNetBase::DefaultMaxExpectedBlockSize = 1048576; // 10 M unless changed by a SIP variable

// The value that will be used if setMaxSentBlockSize() is not called (or called with a negative argument)
uint32 CBufNetBase::DefaultMaxSentBlockSize = 1048576; // 10 M unless changed by a SIP variable


uint32	nUdpAndTcpConnectionPSize = 0;

/***************************************************************************************************
 * User main thread 
 **************************************************************************************************/

 
/*
 * Constructor
 */
#ifdef SIP_OS_UNIX
CBufNetBase::CBufNetBase( bool isDataAvailablePipeSelfManaged ) :
#else
CBufNetBase::CBufNetBase() :
#endif
	_RecvFifo("CBufNetBase::_RecvFifo"),
	_DisconnectionCallback( NULL ),
	_DisconnectionCbArg( NULL ),
	_MaxExpectedBlockSize( DefaultMaxExpectedBlockSize ),
	_MaxSentBlockSize( DefaultMaxSentBlockSize ),
	_DataAvailable( false )
{
	// Debug info for mutexes
#ifdef MUTEX_DEBUG
	initAcquireTimeMap();
#endif
#ifdef SIP_OS_UNIX
	_IsDataAvailablePipeSelfManaged = isDataAvailablePipeSelfManaged;
	if ( _IsDataAvailablePipeSelfManaged )
	{
		if ( ::pipe( _DataAvailablePipeHandle ) != 0 )
			sipwarning( "LNETL1: Unable to create D.A. pipe" );
	}
#endif
	nUdpAndTcpConnectionPSize = 4 + string(UDPCONNECTION).size() + 4; // size, mesid, tcpid

}


/*
 * Destructor
 */
CBufNetBase::~CBufNetBase()
{
#ifdef SIP_OS_UNIX
	if ( _IsDataAvailablePipeSelfManaged )
	{
		::close( _DataAvailablePipeHandle[PipeRead] );
		::close( _DataAvailablePipeHandle[PipeWrite] );
	}
#endif
}


/*
 * Push message into receive queue (mutexed)
 * TODO OPTIM never use this function
 */
void	CBufNetBase::pushMessageIntoReceiveQueue( const std::vector<uint8>& buffer )
{
	//sint32 mbsize;
	{
		//sipdebug( "BNB: Acquiring the receive queue... ");
		CFifoAccessor recvfifo( &_RecvFifo );
		//sipdebug( "BNB: Acquired, pushing the received buffer... ");
		recvfifo.value().push( buffer );
		//sipdebug( "BNB: Pushed, releasing the receive queue..." );
		//mbsize = recvfifo.value().size() / 1048576;
		setDataAvailableFlag( true );
	}

#ifdef SIP_OS_UNIX
	// Wake-up main thread (outside the critical section of CFifoAccessor, to allow main thread to be
	// read the fifo; if the main thread sees the Data Available flag is true but the pipe not written
	// yet, it will block on read()).
	uint8 b=0;
	if ( write( _DataAvailablePipeHandle[PipeWrite], &b, 1 ) == -1 )
	{
		sipwarning( "LNETL1: Write pipe failed in pushMessageIntoReceiveQueue" );
	}
	//sipdebug( "Pipe: 1 byte written (%p)", this );
#endif
	//sipdebug( "BNB: Released." );
	//if ( mbsize > 1 )
	//{
	//	sipwarning( "The receive queue size exceeds %d MB", mbsize );
	//}
}

/*
 * Push message into receive queue (mutexed)
 */
void	CBufNetBase::pushMessageIntoReceiveQueue( const uint8 *buffer, uint32 size )
{
	//sint32 mbsize;
	{
		//sipdebug( "BNB: Acquiring the receive queue... ");
		CFifoAccessor recvfifo( &_RecvFifo );
		//sipdebug( "BNB: Acquired, pushing the received buffer... ");
		recvfifo.value().push( buffer, size );
		//sipdebug( "BNB: Pushed, releasing the receive queue..." );
		//mbsize = recvfifo.value().size() / 1048576;
		setDataAvailableFlag( true );
#ifdef SIP_OS_UNIX
		// Wake-up main thread
		uint8 b=0;
		if ( write( _DataAvailablePipeHandle[PipeWrite], &b, 1 ) == -1 )
		{
			sipwarning( "LNETL1: Write pipe failed in pushMessageIntoReceiveQueue" );
		}
		sipdebug( "Pipe: 1 byte written" );
#endif
	}
	//sipdebug( "BNB: Released." );
	/*if ( mbsize > 1 )
	{
		sipwarning( "The receive queue size exceeds %d MB", mbsize );
	}*/
}

void	CBufNetBase::clearReceiveQueue()
{
	CFifoAccessor recvfifo( &_RecvFifo );
	recvfifo.value().clear();

	_DataAvailable = false;
}


SIPBASE_CATEGORISED_VARIABLE(sip, uint32, NbNetworkTask, "Number of server and client thread");
	
} // SIPNET
