/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __BUF_NET_BASE_H__
#define __BUF_NET_BASE_H__

#include "misc/types_sip.h"
#include "misc/mutex.h"
#include "misc/buf_fifo.h"
#include "misc/thread.h"
#include "misc/debug.h"
#include "misc/common.h"
#include "misc/variable.h"

namespace SIPNET {

#define MAX_UDPDATAGRAMLEN	1000	// UDP Datagram의 최대바이트수

	typedef	uint32	TUDPTCPID;
	typedef	uint16	TUDPALLINDEX;
	typedef	uint8	TUDPTYPE;
	typedef	uint16	TUDPINDEX;
#define UDPHEADERPOS1	0
#define UDPHEADERPOS2	(UDPHEADERPOS1+sizeof(TUDPTCPID))
#define UDPHEADERPOS3	(UDPHEADERPOS2+sizeof(TUDPALLINDEX))
#define UDPHEADERPOS4	(UDPHEADERPOS3+sizeof(TUDPTYPE))
#define UDPMESPOS		(UDPHEADERPOS4+sizeof(TUDPINDEX))

	// UDP통신을 리용하는 파케트type값은 반드시 순차를 가지는 enum형이여야 한다
	enum {	UDP_TYPESTART = 0, 
		UDP_UNRELIABILITY,	// UDP소켓특성 그대로인 신뢰성 없는 파케트
		UDP_REL_NOORDER,	// 신뢰성이 보장되고 순서에 무관계한 파케트
		UDP_REL_UPORDER,	// 신뢰성이 보장되고 이전 순서의 파케트를 무시하는 파케트
		UDP_REL_ORDER,		// 신뢰성이 보장되고 순서성도 보장되여야 하는 파케트
		UDP_RESPONSE,		// 수신된 파케트에 대한 응답파케트
		UDP_TYPE_END };

#define UDPTYPENUM UDP_TYPE_END-UDP_TYPESTART-1	// UDP통신을 리용하는 파케트류형의 개수
#define UINDEX(type) type-UDP_TYPESTART-1			// UDP통신을 리용하는 파케트류형의 령기초 Index를 얻기 위한 마크로

#define UDPCONNECTION	"UDPCONNECTION"
extern	uint32	nUdpAndTcpConnectionPSize;


#define UNRELIABILITY	UDP_UNRELIABILITY	// 파케트가 송신되지 않아도 무방하다
#define REL_NOORDER		UDP_REL_NOORDER		// 신뢰성이 보장되고 순서에 무관계하다
#define REL_UPORDER		UDP_REL_UPORDER		// 신뢰성이 보장되고 이전 순서의 파케트를 무시한다
#define REL_ORDER		UDP_REL_ORDER		// 신뢰성이 보장되고 순서성도 보장되여야 한다
#define REL_GOOD_ORDER	200					// 신뢰성이 보장되고 순서성도 완벽하게 보장되여야 한다

class CBufSock;
class CBufUdpSock;
class CBufUdpServer;

/// Socket identifier
typedef CBufSock		*TTcpSockId;
typedef CBufSock		*TSockId;
typedef CBufUdpServer	*TUdpSockId;

static const TTcpSockId InvalidSockId = (TTcpSockId) NULL;

/// Callback function for message processing
typedef void (*TNetCallback) ( TTcpSockId from, void *arg );

/// Storing a TNetCallback call for future call
typedef std::pair<TNetCallback,TTcpSockId> TStoredNetCallback;

/// Size of a block
typedef uint32 TBlockSize;

extern uint32 	NbNetworkTask;

#ifdef SIP_OS_UNIX
/// Access to the wake-up pipe (Unix only)
enum TPipeWay { PipeRead, PipeWrite };
#endif

/**
 * Layer 1
 *
 * Base class for CBufClient and CBufServer.
 * The max block sizes for sending and receiving are controlled by setMaxSentBlockSize()
 * and setMaxExpectedBlockSize(). Their default value is the maximum number contained in a sint32,
 * that is 2^31-1 (i.e. 0x7FFFFFFF). The limit for sending is checked only in debug mode.
 *
 * \date 2001
 */
class CBufNetBase
{
public:

	/// Type of incoming events (max 256)
	enum TEventType { User = 'U', Connection = 'C', Disconnection = 'D' };

	/// Destructor
	virtual ~CBufNetBase();

#ifdef SIP_OS_UNIX
	/** Init the pipe for data available with an external pipe.
	 * Call it only if you set initPipeForDataAvailable to false in the constructor.
	 * Then don't call sleepUntilDataAvailable() but use select() on the pipe.
	 * The pipe will be written one byte when receiving a message.
	 */
	void	setExternalPipeForDataAvailable( int *twoPipeHandles )
	{
		_DataAvailablePipeHandle[PipeRead] = twoPipeHandles[PipeRead];
		_DataAvailablePipeHandle[PipeWrite] = twoPipeHandles[PipeWrite];
	}
#endif

	/// Sets callback for detecting a disconnection (or NULL to disable callback)
	void	setDisconnectionCallback( TNetCallback cb, void* arg ) { _DisconnectionCallback = cb; _DisconnectionCbArg = arg; }

	/// Returns the size of the receive queue (mutexed)
	uint64	getReceiveQueueSize()
	{
		SIPBASE::CFifoAccessor recvfifo( &receiveQueue() );
		return (recvfifo.value().size());
	}
	uint32	getReceiveQueueItem()
	{
		SIPBASE::CFifoAccessor recvfifo( &receiveQueue() );
		return (recvfifo.value().fifoSize());
	}
	
	void displayReceiveQueueStat (SIPBASE::CLog *log = SIPBASE::InfoLog)
	{
		SIPBASE::CFifoAccessor syncfifo( &_RecvFifo );
		syncfifo.value().displayStats(log);
	}

	void displayReceiveQueue ()
	{
		SIPBASE::CFifoAccessor syncfifo( &_RecvFifo );
		syncfifo.value().display();
	}

	
	/**
	 * Sets the max size of the received messages.
	 * If receiving a message bigger than the limit, the connection will be dropped.
	 *
	 * Default value: CBufNetBase::DefaultMaxExpectedBlockSize
	 * If you put a negative number as limit, the max size is reset to the default value.
	 * Warning: you can call this method only at initialization time, before connecting (for a client)
	 * or calling init() (for a server) !
	 */
	void	setMaxExpectedBlockSize( sint32 limit )
	{
		if ( limit < 0 )
			_MaxExpectedBlockSize = DefaultMaxExpectedBlockSize;
		else
			_MaxExpectedBlockSize = (uint32)limit;
	}

	/**
	 * Sets the max size of the sent messages.
	 * Any bigger sent block will produce an assertion failure, currently.
	 *
	 * Default value: CBufNetBase::DefaultMaxSentBlockSize
	 * If you put a negative number as limit, the max size is reset to the default value.
	 * Warning: you can call this method only at initialization time, before connecting (for a client)
	 * or calling init() (for a server) !
	 */
	void	setMaxSentBlockSize( sint32 limit )
	{
		if ( limit < 0 )
			_MaxSentBlockSize = DefaultMaxSentBlockSize;
		else
			_MaxSentBlockSize = (uint32)limit;
	}

	/// Returns the max size of the received messages (default: 2^31-1)
	uint32	maxExpectedBlockSize() const
	{
		return _MaxExpectedBlockSize;
	}

	/// Returns the max size of the sent messages (default: 2^31-1)
	uint32	maxSentBlockSize() const
	{
		return _MaxSentBlockSize;
	}

	void				clearReceiveQueue();

#ifdef SIP_OS_UNIX
	/**
	 * Return the handle for reading the 'data available pipe'. Use it if you want to do a select on
	 * multiple CBufNetClient/CBufNetServer objects (then, don't call sleepUntilDataAvailable() on them).
	 */
	int		dataAvailablePipeReadHandle() const { return _DataAvailablePipeHandle[PipeRead]; }
#endif

	/// The value that will be used if setMaxExpectedBlockSize() is not called (or called with a negative argument)
	static uint32 DefaultMaxExpectedBlockSize;

	/// The value that will be used if setMaxSentBlockSize() is not called (or called with a negative argument)
	static uint32 DefaultMaxSentBlockSize;

protected:

	friend class SIPNET::CBufSock;

#ifdef SIP_OS_UNIX
	/// Constructor
	CBufNetBase( bool isDataAvailablePipeSelfManaged );
#else
	/// Constructor
	CBufNetBase();
#endif

	/// Access to the receive queue
	SIPBASE::CSynchronizedFIFO&	receiveQueue() { return _RecvFifo; }

	/// Returns the disconnection callback
	TNetCallback		disconnectionCallback() const { return _DisconnectionCallback; }

	/// Returns the argument of the disconnection callback
	void*				argOfDisconnectionCallback() const { return _DisconnectionCbArg; }

	/// Push message into receive queue (mutexed)
	// TODO OPTIM never use this function
	void				pushMessageIntoReceiveQueue( const std::vector<uint8>& buffer );
	/// Push message into receive queue (mutexed)
	void				pushMessageIntoReceiveQueue( const uint8 *buffer, uint32 size );

	/// Sets _DataAvailable
	void				setDataAvailableFlag( bool da ) { _DataAvailable = da; }

	/// Return _DataAvailable
	volatile bool		dataAvailableFlag() const { return _DataAvailable; }

#ifdef SIP_OS_UNIX
	/// Pipe to select() on data available
	int					_DataAvailablePipeHandle [2];
#endif

private:

	/// The receive queue for tcp, protected by a mutex-like device
	SIPBASE::CSynchronizedFIFO	_RecvFifo;
	
	/// Callback for disconnection
	TNetCallback		_DisconnectionCallback;

	/// Argument of the disconnection callback
	void*				_DisconnectionCbArg;

	/// Max size of received messages (limited by the user)
	uint32				_MaxExpectedBlockSize;

	/// Max size of sent messages (limited by the user)
	uint32				_MaxSentBlockSize;

	/// True if there is data available (avoids locking a mutex)
	volatile bool		_DataAvailable;

#ifdef SIP_OS_UNIX
	bool _IsDataAvailablePipeSelfManaged;
#endif
};
} // SIPNET


#endif // __BUF_NET_BASE_H__

/* End of buf_net_base.h */
