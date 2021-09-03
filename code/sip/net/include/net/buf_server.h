/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __BUF_SERVER_H__
#define __BUF_SERVER_H__

#include "misc/types_sip.h"
#include "buf_net_base.h"
#include "listen_sock.h"
#include "buf_sock.h"
#include "buf_udpsock.h"
#include <list>
#include <set>
#include "misc/twin_map.h"

// POLL1 (ignore POLL comments)

namespace SIPNET {


class CInetAddress;
class CBufServer;


/**
 * Common part of CListenTask and CServerReceiveTask
 */
class CServerTask
{
public:

	/// Destructor
	virtual ~CServerTask();

	/// Tells the task to exit
	void	requireExit() { _ExitRequired = true; }

#ifdef SIP_OS_UNIX
	/// Wake the thread up, when blocked in select (Unix only)
	void	wakeUp();
#endif

	uint32 NbLoop;

protected:

	/// Constructor
	CServerTask();
	
	/// Returns true if the requireExit() has been called
	bool	exitRequired() const { return _ExitRequired; }

#ifdef SIP_OS_UNIX
	/// Pipe for select wake-up (Unix only)
	int		        _WakeUpPipeHandle [2];
#endif

private:

	volatile bool	_ExitRequired;	
};


/**
 * Code of listening thread
 */
class CListenTask : public SIPBASE::IRunnable, public CServerTask
{
public:

	/// Constructor
	CListenTask( CBufServer *server ) : CServerTask(), _Server(server) {}

	/// Begins to listen on the specified port (call before running thread)
	void			init( uint16 port, sint32 maxExpectedBlockSize );

	/// Run (exits when the listening socket disconnects)
	virtual void	run();

	/// Close listening socket
	void			close();

	/// Returns the listening address
	const CInetAddress&	localAddr() { return _ListenSock.localAddr(); }

private:

	CBufServer			*_Server;	
	CListenSock			_ListenSock;
	uint32				_MaxExpectedBlockSize;

};

class CUdpReceiveTask : public SIPBASE::IRunnable
{
public:

	/// Constructor
	CUdpReceiveTask( CBufServer *server ) : _Server(server), _ExitRequired(false) {}

	/// Tells the task to exit
	void	requireExit() { _ExitRequired = true; }

	/// Begins to listen on the specified port (call before running thread)
	void			init( uint16 port );

	/// Run (exits when the listening socket disconnects)
	virtual void	run();

protected:
	/// Returns true if the requireExit() has been called
	bool	exitRequired() const { return _ExitRequired; }
private:

	CBufServer			*_Server;	
	volatile bool	_ExitRequired;	
};


typedef std::vector<SIPBASE::IThread*> CThreadPool;


#define PRESET_BIG_SERVER

#ifdef PRESET_BIG_SERVER
// Big server
#define DEFAULT_STRATEGY SpreadSockets
#define DEFAULT_MAX_THREADS 64
#define DEFAULT_MAX_SOCKETS_PER_THREADS 64
#else
// Small server
#define DEFAULT_STRATEGY FillThreads
#define DEFAULT_MAX_THREADS 64
#define DEFAULT_MAX_SOCKETS_PER_THREADS 16
#endif


typedef std::set<TTcpSockId>					TTcpSet;
typedef std::map<CInetAddress, TUdpSockId>		TAddrAndUdp;
typedef std::map<TTcpSockId, TUdpSockId>		TTcpAndUdp;
typedef bool (*TGetMsgCheckNeedCallback) (const std::vector<uint8>& _buffer, uint32 *_mesID, MsgNameType*_sMesID, uint32 *_intervalTime, uint32 *_serialMaxNum);
typedef void (*TProcUnlawSerialCallback) (MsgNameType _sMesID, TTcpSockId _sock);

/**
 * Server class for layer 1
 *
 * Listening socket and accepted connetions, with packet scheme.
 * The provided buffers are sent raw (no endianness conversion).
 * By default, the size time trigger is disabled, the time trigger is set to 20 ms.
 *
 * Where do the methods take place:
 * \code
 * send(),	                           ->  send buffer   ->  update(), flush()
 * bytesSent(), newBytesSent()
 *
 * receive(), dataAvailable(),         <- receive buffer <-  receive thread,
 * dataAvailable(),
 * bytesReceived(), newBytesReceived(),
 * connection callback, disconnection callback
 * \endcode
 *
 * \date 2001
 */
class CBufServer : public CBufNetBase
{
public:

	enum TThreadStategy { SpreadSockets, FillThreads };

	/** Constructor
	 * Set nodelay to true to disable the Nagle buffering algorithm (see CTcpSock documentation)
	 * initPipeForDataAvailable is for Linux only. Set it to false if you provide an external pipe with
	 * setExternalPipeForDataAvailable().
	 */
	CBufServer( bool bUdpServer = false,
				bool bUserServer= false,
				uint32 max_frontnum = 1000,
				TThreadStategy strategy=DEFAULT_STRATEGY,
				uint16 max_threads=DEFAULT_MAX_THREADS,
				uint16 max_sockets_per_thread=DEFAULT_MAX_SOCKETS_PER_THREADS,
				bool nodelay=true, bool replaymode=false, bool initPipeForDataAvailable=true );
	/// Destructor
	virtual ~CBufServer();

	/// Listens on the specified port
	void	init( uint16 port );

	/** Disconnect a connection
	 * Set hostid to InvalidSockId to disconnect all connections.
	 * If hostid is not InvalidSockId and the socket is not connected, the method does nothing.
	 * If quick is true, any pending data will not be sent before disconnecting.
	 * If quick is false, a flush() will be called. In case of network congestion, the entire pending
	 * data may not be flushed. If this is a problem, call flush() multiple times until it returns 0
	 * before calling disconnect().
	 */
	void	disconnect( TTcpSockId hostid, bool quick=false );

	// KSR_ADD
	// close a connection without grateful shutdown
	void	forceClose( TTcpSockId hostid, bool quick=false );

	/// Sets callback for incoming connections (or NULL to disable callback)
	void	setConnectionCallback( TNetCallback cb, void* arg ) { _ConnectionCallback = cb; _ConnectionCbArg = arg; }

	
	/// Sets callback for accepting packet
	void	setGetMsgCheckNeedCallback( TGetMsgCheckNeedCallback cb ) { _GetMsgCheckNeedCallback = cb; }
	TGetMsgCheckNeedCallback		getGetMsgCheckNeedCallback() const { return _GetMsgCheckNeedCallback; }
	
	void	setProcUnlawSerialCallback( TProcUnlawSerialCallback cb ) { _ProcUnlawSerialCallback = cb; }
	TProcUnlawSerialCallback		getProcUnlawSerialCallback() const { return _ProcUnlawSerialCallback; }

	/** Send a message to the specified host, or to all hosts if hostid is InvalidSockId
	 */
	//void	send( const std::vector<uint8>& buffer, TTcpSockId hostid );
	void	send( const SIPBASE::CMemStream& buffer, TTcpSockId hostid, uint8 bySendType = REL_GOOD_ORDER );

	/** Checks if there is some data to receive. Returns false if the receive queue is empty.
	 * This is where the connection/disconnection callbacks can be called.
	 */
	bool	dataAvailable();
	void	RefillFromClientBuffer();
	void	RefreshSockBuffer();
#ifdef SIP_OS_UNIX
	/** Wait until the receive queue contains something to read (implemented with a select()).
	 * This is where the connection/disconnection callbacks can be called.
	 * If you use this method (blocking scheme), don't use dataAvailable() (non-blocking scheme).
	 * \param usecMax Max time to wait in microsecond (up to 1 sec)
	 */
	void	sleepUntilDataAvailable( uint usecMax=100000 );
#endif

	/** Receives next block of data in the specified (resizes the vector)
	 * You must call dataAvailable() or sleepUntilDataAvailable() before every call to receive()
	 */
	void	receive( SIPBASE::CMemStream& buffer, TTcpSockId* hostid );

	/// Update the network (call this method evenly)
	void	update();

	// Returns the size in bytes of the data stored in the send queue.
	uint32	getSendQueueSize( TTcpSockId destid );

	void	displaySendQueueStat( SIPBASE::CLog *log = SIPBASE::InfoLog, TTcpSockId destid = InvalidSockId);

	void displayThreadStat (SIPBASE::CLog *log = SIPBASE::InfoLog);
		

	/** Sets the time flush trigger (in millisecond). When this time is elapsed,
	 * all data in the send queue is automatically sent (-1 to disable this trigger)
	 */
	void	setTimeFlushTrigger( TTcpSockId destid, sint32 ms );

	/** Sets the size flush trigger. When the size of the send queue reaches or exceeds this
	 * value, all data in the send queue is automatically sent (-1 to disable this trigger )
	 */
	void	setSizeFlushTrigger( TTcpSockId destid, sint32 size );

	/** Force to send data pending in the send queue now. If all the data could not be sent immediately,
	 * the returned nbBytesRemaining value is non-zero.
	 * \param destid The identifier of the destination connection.
	 * \param nbBytesRemaining If the pointer is not NULL, the method sets the number of bytes still pending after the flush attempt.
	 * \returns False if an error has occured (e.g. the remote host is disconnected).
	 * To retrieve the reason of the error, call CSock::getLastError() and/or CSock::errorString()
	 */
	bool	flush( TTcpSockId destid, uint *nbBytesRemaining=NULL );

	/// Returns the internet address of the listening socket
	const CInetAddress&	listenAddress() const { return _ListenTask->localAddr(); }

	/// Returns the address of the specified host
	const CInetAddress& hostAddress( TTcpSockId hostid );

	/*
	/// Returns the number of bytes pushed into the receive queue since the beginning (mutexed)
	uint32	bytesDownloaded()
	{
		SIPBASE::CSynchronized<uint32>::CAccessor syncbpi ( &_BytesPushedIn );
		return syncbpi.value();
	}
	/// Returns the number of bytes downloaded since the previous call to this method
	uint32	newBytesDownloaded();
	*/

	/// Returns the number of bytes popped by receive() since the beginning
	uint64	bytesReceived() const { return _BytesPoppedIn; }

	/// Returns the number of bytes popped by receive() since the previous call to this method
	uint64	newBytesReceived();

	/// Returns the number of bytes pushed by send() since the beginning
	uint64	bytesSent() const { return _BytesPushedOut; }

	/// Returns the number of bytes pushed by send() since the previous call to this method
	uint64	newBytesSent();

	/// Returns the number of connections (at the last update())
	uint32	nbConnections() const { return _NbConnections; }

	bool	frontend() const { return _FrontEnd; }

	uint32	QueueIncomingConnections() const;

	uint64	getReceiveQueueSize()
	{
		SIPBASE::CFifoAccessor recvfifo( &receiveQueue() );
		return (recvfifo.value().size() + _AllClientRecvBufferSumByte);
	}
	uint32	getReceiveQueueItem()
	{
		SIPBASE::CFifoAccessor recvfifo( &receiveQueue() );
		return (recvfifo.value().fifoSize() + _AllClientRecvBufferItemNum);
	}
	uint32	getConnectedNum() { return _ConnectedClients.size(); }

protected:

	friend class CServerBufSock;
	friend class CListenTask;
	friend class CServerReceiveTask;
	friend class CUdpReceiveTask;
	/// Returns the TCP_NODELAY flag
	bool				noDelay() const { return _NoDelay; }

	/** Binds a new socket and send buffer to an existing or a new thread (that starts)
	 * Note: this method is called in the listening thread.
	 */
	void				dispatchNewSocket( CServerBufSock *bufsock );

	/// Returns the receive task corresponding to a particular thread
	CServerReceiveTask	*receiveTask( std::vector<SIPBASE::IThread*>::iterator ipt )
	{
		return ((CServerReceiveTask*)((*ipt)->getRunnable()));
	}

	/// Pushes a buffer to the specified host's send queue and update (unless not connected)
	/*void pushBufferToHost( const std::vector<uint8>& buffer, TTcpSockId hostid )
	{
		if ( hostid->pushBuffer( buffer ) )
		{
			_BytesPushedOut += buffer.size() + sizeof(TBlockSize); // statistics
		}
	}*/

	void pushBufferToHost( const SIPBASE::CMemStream& buffer, TTcpSockId hostid )
	{
		sipassert( hostid != InvalidSockId );
		if ( hostid->pushBuffer( buffer ) )
		{
			_BytesPushedOut += buffer.length() + sizeof(TBlockSize); // statistics
		}
	}

	// Creates a new task and run a new thread for it
	void				addNewThread( CThreadPool& threadpool, CServerBufSock *bufsock );

	/// Returns the connection callback
	TNetCallback		connectionCallback() const { return _ConnectionCallback; }

	/// Returns the argument of the connection callback
	void*				argOfConnectionCallback() const { return _ConnectionCbArg; }

	/*/// Returns the synchronized number of bytes pushed into the receive queue
	SIPBASE::CSynchronized<uint32>&	syncBytesPushedIn() { return _BytesPushedIn; }
	*/
	void	incReqConnection() { _RQConnections ++; }
	void	decReqConnection() { if (_RQConnections > 0)  _RQConnections --; }
private:
	bool			_bConnectionChange;

	/// List of currently connected client
	TTcpSet						_ConnectedClients;

	uint32						_RQConnections;
	uint32						_ProcAvgTimePerConnection;

	/// Thread socket-handling strategy
	TThreadStategy					_ThreadStrategy;

	/// Max number of threads
	uint16							_MaxThreads;

	/// Max number of sockets handled by one thread
	uint16							_MaxSocketsPerThread;

	/// Listen task
	CListenTask						*_ListenTask;
	/// Listen thread
	SIPBASE::IThread					*_ListenThread;

	/// Udp receive task and thread
	CUdpReceiveTask					*_UdpReceiveTask;
	SIPBASE::IThread				*_UdpReceiveThread;
	CUdpSimSock						*_UdpServerSock;

	/// relation table between TCP and UDP
	SIPBASE::CSynchronized<TTcpAndUdp>			_TcpAndUdpMap;
	
	/* Vector of receiving threads.
	 * Thread: thread control
	 * Thread->Runnable: access to the CServerReceiveTask object
	 * Thread->getRunnable()->sock(): access to the socket
	 * The purpose of this list is to delete the objects after use.
	 */
	SIPBASE::CSynchronized<CThreadPool>		_ThreadPool;

	/// Connection callback
	TNetCallback					_ConnectionCallback;

	/// Argument of the connection callback
	void*							_ConnectionCbArg;

	/// Number of bytes pushed by send() since the beginning
	uint64							_BytesPushedOut;

	/// Number of bytes popped by receive() since the beginning
	uint64							_BytesPoppedIn;

	/// Previous number of bytes received
	uint64							_PrevBytesPoppedIn;

	/// Previous number of bytes sent
	uint64							_PrevBytesPushedOut;

	/// Number of connections (debug stat)
	uint32							_NbConnections;

	/// TCP_NODELAY
	bool							_NoDelay;

	/// Replay mode flag
	bool							_ReplayMode;

	/// Udp server falg
	bool							_UdpServer;

	/// For monitor
	bool							_FrontEnd;
	uint32							_MaxFrontClientNum;
	uint64							_AllClientRecvBufferSumByte;
	uint32							_AllClientRecvBufferItemNum;

	static	uint64					_uTotalMemByteOfComputer;
	static	uint64					_uAvailableMemByte;

	/// Last refresh buffer state time
	SIPBASE::TTime					_LastRefreshBufferStateTime;
  /*
	/// Number of bytes pushed into the receive queue (by the receive threads) since the beginning.
	SIPBASE::CSynchronized<uint32>	_BytesPushedIn;

	/// Previous number of bytes received
	uint32							_PrevBytesPushedIn;
	*/

	/// Check callback for acceptable packet
	TGetMsgCheckNeedCallback			_GetMsgCheckNeedCallback;

	TProcUnlawSerialCallback			_ProcUnlawSerialCallback;
};


typedef std::set<TTcpSockId>					CConnections;

/*
// Workaround for Internal Compiler Error in debug mode with MSVC6
#ifdef SIP_RELEASE
typedef CConnections::iterator				ItConnection;
#else
typedef TTcpSockId								ItConnection;
#endif
// Set of iterators to connections (set because we must not add an element twice) 
typedef std::set<ItConnection>			CItConnections;
*/


// POLL2


/**
 * Code of receiving threads for servers.
 * Note: the methods locations in the classes do not correspond to the threads where they are
 * executed, but to the data they use.
 */
class CServerReceiveTask : public SIPBASE::IRunnable, public CServerTask
{
public:

	/// Constructor
	CServerReceiveTask( CBufServer *server ) : CServerTask(), _Server(server), _Connections("CServerReceiveTask::_Connections"), _RemoveSet("CServerReceiveTask::_RemoveSet") {}

	/// Run
	virtual void run();

	/// Returns the number of connections handled by the thread (mutexed on _Connections)
	uint	numberOfConnections()
	{
		uint nb;
		{
			SIPBASE::CSynchronized<CConnections>::CAccessor connectionssync( &_Connections );
			nb = connectionssync.value().size();
		}
		return nb;
	}

	/// Add a new connection into this thread (mutexed on _Connections)
	void	addNewSocket( TTcpSockId sockid )
	{
		//sipnettrace( "CServerReceiveTask::addNewSocket" );
		sipassert( sockid != InvalidSockId );
		{
			SIPBASE::CSynchronized<CConnections>::CAccessor connectionssync( &_Connections );
			connectionssync.value().insert( sockid );
		}
		// POLL3
	}

// POLL4

	/** Add connection to the remove set (mutexed on _RemoveSet)
	 * Note: you must not call this method within a mutual exclusion on _Connections, or
	 * there will be a deadlock (see clearClosedConnection())
	 */
	void	addToRemoveSet( TTcpSockId sockid )
	{
		sipnettrace( "CServerReceiveTask::addToRemoveSet" );
		sipassert( sockid != InvalidSockId );
		{
			// Three possibilities :
			// - The value is inserted into the set.
			// - The value is already present in the set.
			// - The set is locked by a receive thread which is removing the closed connections.
			//   When the set gets unlocked, it is empty so the value is inserted. It means the
			//   value could be already in the set before it was cleared.
			//   Note: with a fonction such as tryAcquire(), we could avoid to enter the mutex
			//   when it is already locked
			// See clearClosedConnections().
			SIPBASE::CSynchronized<CConnections>::CAccessor removesetsync( &_RemoveSet );
			removesetsync.value().insert( sockid );
			//sipdebug( "LNETL1: ic: %p - RemoveSet.size(): %d", ic, removesetsync.value().size() );
		}
#ifdef SIP_OS_UNIX
		wakeUp();
#endif
	}

	/// Delete all connections referenced in the remove list (mutexed on _RemoveSet and on _Connections)
	void	clearClosedConnections();

	/// Access to the server
	CBufServer	*server()	{ return _Server; }

	friend	class CBufServer;

private:

	CBufServer								*_Server;

	/* List of sockets and send buffer.
	 * A TTcpSockId is a pointer to a CBufSock object
	 */
	SIPBASE::CSynchronized<CConnections>		_Connections;

	// Connections to remove
	SIPBASE::CSynchronized<CConnections>		_RemoveSet;

	// POLL5

};


} // SIPNET


#endif // __BUF_SERVER_H__

/* End of buf_server.h */
