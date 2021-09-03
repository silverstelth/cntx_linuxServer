/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __BUF_SOCK_H__
#define __BUF_SOCK_H__

#include "misc/types_sip.h"
#include "misc/hierarchical_timer.h"
#include "buf_net_base.h"
#include "tcp_sock.h"
#include "message.h"

namespace SIPNET {


#define sipnettrace(__msg) //sipdebug("SIPNETL1: %s",__msg);


class CTcpSock;
class CServerReceiveTask;
class CBufNetBase;
class CBufUdpServer;

typedef uint32 TMesID;
struct PHISDATA 
{
	SIPBASE::TTime	tmLastAccept;
	uint32			uSerialNum;
};

/**
 * CBufSock
 * A socket and its sending buffer
 */
class CBufSock
{
public:

	/// Destructor
	virtual ~CBufSock();

	/// Sets the application identifier
	void					setAppId( uint64 id ) { _AppId = id; }

	/// Returns the application identifier
	uint64					appId() const { return _AppId; }

	/// Returns a string with the characteristics of the object
	std::string				asString() const;

	/// get the TCP sock object
	const CTcpSock			*getTcpSock() const { return Sock;}

	bool					procAcceptMsg(uint32 MesID, SIPBASE::TTime IntervalTime, uint32 SerialMaxNum);

	TBlockSize				GetRemainSendBufferSize() { return (SendFifo.size() + (_ReadyToSendBuffer.size() - _RTSBIndex)); }

	void					SetMaxSendFifoSizeByte(uint32 uMax) { _MaxSendBufferByte = uMax; }
	/// Little tricky but this string is used by Layer4 to know which callback is authorized.
	/// This is empty when all callback are authorized.
	MsgNameType				AuthorizedCallback;
	SIPBASE::TTime			AuthorizedStartTime;
	SIPBASE::TTime			AuthorizedDuring;
	// debug features, we number all packet to be sure that they are all sent and received
	// \todo remove this debug feature when ok
	uint32					SendNextValue, ReceiveNextValue;

	// Prevents from pushing a connection/disconnection event twice
	bool				_KnowConnected;

protected:

	friend class CBufClient;
	friend class CBufServer;
	friend class CClientReceiveTask;
	friend class CServerReceiveTask;

	friend class CCallbackClient;
	friend class CCallbackServer;
	friend class CCallbackNetBase;

	/** Constructor
	 * \param sock To provide an external socket. Set it to NULL to create it internally.
	 */
	CBufSock( CTcpSock *sock=NULL );

	///@name Sending data
	//@{
	
	/// Update the network sending (call this method evenly). Returns false if an error occured.
	bool	update();

	/** Sets the time flush trigger (in millisecond). When this time is elapsed,
	 * all data in the send queue is automatically sent (-1 to disable this trigger)
	 */
	void	setTimeFlushTrigger( sint32 ms );

	/** Sets the size flush trigger. When the size of the send queue reaches or exceeds this
	 * calue, all data in the send queue is automatically sent (-1 to disable this trigger )
	 */
	void	setSizeFlushTrigger( sint32 size ) { _TriggerSize = size; }

	/** Force to send data pending in the send queue now. In the case of a non-blocking socket
	 * (see CNonBlockingBufSock), if all the data could not be sent immediately,
	 * the returned nbBytesRemaining value is non-zero.
	 * \param nbBytesRemaining If the pointer is not NULL, the method sets the number of bytes still pending after the flush attempt.
	 * \returns False if an error has occured (e.g. the remote host is disconnected).
	 * To retrieve the reason of the error, call CSock::getLastError() and/or CSock::errorString()
	 */
	bool	flush( uint *nbBytesRemaining=NULL );
	
	//@}

	/// Returns "CLT " (client)
	virtual std::string typeStr() const { return "CLT "; }

	/** Pushes a disconnection message into bnb's receive queue, if it has not already been done
	 * (returns true in this case). You can either specify a sockid (for server) or InvalidSockId (for client)
	 */
	bool advertiseDisconnection( CBufNetBase *bnb, TTcpSockId sockid )
	{
#ifdef SIP_DEBUG
		if ( sockid != InvalidSockId )
		{
			sipassert( sockid == this );
		}
#endif
		return advertiseSystemEvent( bnb, sockid, _KnowConnected, true, CBufNetBase::Disconnection );
	}

	
	/** Pushes a system message into bnb's receive queue, if the flags meets the condition, then
	 * resets the flag and returns true. You can either specify a sockid (for server) or InvalidSockId (for client).
	 */
	bool advertiseSystemEvent(
		CBufNetBase *bnb, TTcpSockId sockid, bool& flag, bool condition, CBufNetBase::TEventType event )
	{
#ifdef SIP_DEBUG
		if ( sockid != InvalidSockId )
		{
			sipassert( sockid == this );
		}
#endif
		if ( flag==condition )
		{
			sipdebug( "LNETL1: Pushing event to %s", asString().c_str() );
			std::vector<uint8> buffer;
			if ( sockid == InvalidSockId )
			{
				// Client: event type only
				buffer.resize( 1 );
				buffer[0] = event;
			}
			else
			{
				// Server: sockid + event type
				/*
				buffer.resize( sizeof(TTcpSockId) + 1 );
				memcpy( &*buffer.begin(), &sockid, sizeof(TTcpSockId) );
				buffer[sizeof(TTcpSockId)] = event;
				*/

				// KSR_ADD
				buffer.resize( sizeof(TTcpSockId) + sizeof(uint32) + 1 );

				memcpy( &*buffer.begin(), &sockid, sizeof(TTcpSockId) );

				uint32	curtime = SIPBASE::CTime::getTick();
				if ( event == CBufNetBase::Connection )
					memcpy( &*buffer.begin() + sizeof(TTcpSockId), &curtime, sizeof(uint32) );

				buffer[sizeof(TTcpSockId) + sizeof(uint32)] = event;
				// KSR_ADD
			}
			// Push
			bnb->pushMessageIntoReceiveQueue( buffer );

			// Reset flag
			flag = !condition;
			return true;
		}
		else
		{
			return false;
		}
	}

	/** Pushes a buffer to the send queue and update,
	 * or returns false if the socket is not physically connected the or an error occured during sending
	 */
	bool pushBuffer( const SIPBASE::CMemStream& buffer )
	{
		sipassert (this != InvalidSockId);	// invalid bufsock
//		sipdebug( "LNETL1: Pushing buffer to %s", asString().c_str() );

		static uint32 biggerBufferSize = 64000;
		if (buffer.length() > biggerBufferSize)
		{
			biggerBufferSize = buffer.length();
			sipdebug ("LNETL1: new record! bigger network message pushed (sent) is %u bytes", biggerBufferSize);
		}

		if ( Sock->connected() )
		{
			// Push into host's send queue
			{
				//sipdebug( "BNB: Acquired, pushing the received buffer... ");
				if (SendFifo.size() > _MaxSendBufferByte)
				{
					_SendbufferOverflowNumber ++;
					if (!_bOccurSendbufferOverflow)
					{
						sipwarning ("LNETL1: Forceclosed socket for sending overflow sock=%p, appid=%d, maxbuffer=%d, number=%d", this, (uint32)_AppId, _MaxSendBufferByte, _SendbufferOverflowNumber);
						forceClose();
					}
					_bOccurSendbufferOverflow = true;
					return false;
				}

				_bOccurSendbufferOverflow = false;
				SendFifo.push( buffer );
			}

			// Update sending
			// bool res = update ();
			// return res; // not checking the result as in CBufServer::update()
			return true;
		}
		return false;
	}

	/*bool pushBuffer( const std::vector<uint8>& buffer )
	{
		sipassert (this != InvalidSockId);	// invalid bufsock
//		sipdebug( "LNETL1: Pushing buffer to %s", asString().c_str() );

		static uint32 biggerBufferSize = 64000;
		if (buffer.size() > biggerBufferSize)
		{
			biggerBufferSize = buffer.size();
			sipwarning ("LNETL1: new record! bigger network message pushed (sent) is %u bytes", biggerBufferSize);
		}

		if ( Sock->connected() )
		{
			// Push into host's send queue
			SendFifo.push( buffer );

			// Update sending
			bool res = update ();
			return res; // not checking the result as in CBufServer::update()
		}
		return false;
	}*/


	/// Connects to the specified addr; set connectedstate to true if no connection advertising is needed
	void connect( const CInetAddress& addr, bool nodelay, bool connectedstate, uint32 sectime = 0);

	/// Disconnects; set connectedstate to false if no disconnection advertising is needed
	void disconnect( bool connectedstate );

	void forceClose();

	bool forceClosed() { return Sock->forceClosed(); };

	/// Sets the "logically connected" state (changed when processing a connection/disconnection callback)
	void setConnectedState( bool connectedstate ) { _ConnectedState = connectedstate; } 

	/// Returns the "logically connected" state (changed when processing a connection/disconnection callback)
	bool connectedState() const { return _ConnectedState; }

	// Send queue
	SIPBASE::CBufFIFO	SendFifo;
	

	// Socket (pointer because it can be allocated by an accept())
	CTcpSock			*Sock;

	bool				_bOccurSendbufferOverflow;
	bool				_bOccurRecvbufferOverflow;
	bool				_bOccurSendZero;
	uint32				_SendbufferOverflowNumber;
	uint32				_RecvbufferOverflowNumber;
	uint32				_SendZeroNumber;

private:

#ifdef SIP_DEBUG
	enum TFlushTrigger { FTTime, FTSize, FTManual };
	TFlushTrigger		_FlushTrigger;
#endif

	SIPBASE::TTime		_LastFlushTime; // updated only if time trigger is enabled (TriggerTime!=-1)
	SIPBASE::TTime		_TriggerTime;
	sint32				_TriggerSize;

	SIPBASE::CObjectVector<uint8> _ReadyToSendBuffer;
	TBlockSize			_RTSBIndex;

	uint64				_AppId;

	// Connected state (from the user's point of view, i.e. changed when the connection/disconnection event is at the front of the receive queue)
	bool				_ConnectedState;

	std::map<TMesID, PHISDATA>	_MsgAcceptHis;
	uint32				_MaxSendBufferByte;

};


/**
 * CNonBlockingBufSock
 * A socket, its send buffer plus a nonblocking receiving system
 */
class CNonBlockingBufSock : public CBufSock
{
protected:

	friend class CBufClient;
	friend class CClientReceiveTask;
	friend class CClientReceiveTaskUdp;

	/** Constructor
     * \param sock To provide an external socket. Set it to NULL to create it internally.
     * \maxExpectedBlockSize Default value: receiving limited to 10 M per block)
     */
	CNonBlockingBufSock( CTcpSock *sock=NULL, uint32 maxExpectedBlockSize=10485760 );

	/** Call this method after connecting (for a client connection) to set the non-blocking mode.
	 * For a server connection, call it as soon as the object is constructed
	 */
	void						setNonBlocking() { Sock->setNonBlockingMode( true ); }

	/// Set the size limit for received blocks
	void						setMaxExpectedBlockSize( sint32 limit ) { _MaxExpectedBlockSize = limit; }

	/** Receives a part of a message (nonblocking socket only)
	 * \param nbExtraBytes Number of bytes to reserve for extra information such as the event type
	 * \return True if the message has been completely received
	 */
	bool						receivePart( uint32 nbExtraBytes );

	/// Fill the event type byte at pos length()(for a client connection)
	void						fillEventTypeOnly() { _ReceiveBuffer[_Length] = (uint8)CBufNetBase::User; }

	/** Return the length of the received block (call after receivePart() returns true).
	 * The total size of received buffer is length() + nbExtraBytes (passed to receivePart()).
	 */
	uint32						length() const { return _Length; }

	/** Returns the filled buffer (call after receivePart() returns true).
	 * Its size is length()+1.
	 */
	const std::vector<uint8>	receivedBuffer() const { sipnettrace( "CServerBufSock::receivedBuffer" ); return _ReceiveBuffer; }

	// Buffer for nonblocking receives
	std::vector<uint8>			_ReceiveBuffer;

	// Max payload size than can be received in a block
	uint32						_MaxExpectedBlockSize;

private:

	// True if the length prefix has already been read
	bool						_NowReadingBuffer;

	// Counts the number of bytes read for the current element (length prefix or buffer)
	TBlockSize					_BytesRead;

	// Length of buffer to read
	TBlockSize					_Length;

};


class CBufServer;


/**
 * CServerBufSock
 * A socket, its send buffer plus a nonblocking receiving system for a server connection
 */
class CServerBufSock : public CNonBlockingBufSock
{
protected:
	friend class CBufServer;
	friend class CListenTask;
	friend class CServerReceiveTask;
	friend class CUdpReceiveTask;

	/** Constructor with an existing socket (created by an accept()).
	 * Don't forget to call setOwnerTask().
	 */
	CServerBufSock( CTcpSock *sock );

	/// Sets the task that "owns" the CServerBufSock object
	void						setOwnerTask( CServerReceiveTask* owner ) { _OwnerTask = owner; }

	/// Returns the task that "owns" the CServerBufSock object
	CServerReceiveTask			*ownerTask() { return _OwnerTask; }

	/** Pushes a connection message into bnb's receive queue, if it has not already been done
	 * (returns true in this case).
	 */
	bool						advertiseConnection( CBufServer *bnb )
	{
		return advertiseSystemEvent( (CBufNetBase*)bnb, this, _KnowConnected, false, CBufNetBase::Connection );
	}

	/// Returns "SRV " (server)
	virtual std::string			typeStr() const { return "SRV "; }

	/// Fill the sockid and the event type byte at the end of the buffer
	void						fillSockIdAndEventType( TTcpSockId sockId )
	{
		memcpy( (&*_ReceiveBuffer.begin()) + length(), &sockId, sizeof(TTcpSockId) );
		_ReceiveBuffer[length() + sizeof(TTcpSockId)] = (uint8)CBufNetBase::User;
	}

	CBufUdpServer	*_UdpFriend;
	uint32			_MaxRecvBufferByte;	// byte
public:
	CBufUdpServer*	getUdpSock()	{ return _UdpFriend; }
	SIPBASE::CSynchronizedFIFO&	receiveQueue() { return _RecvFifo; }
	void				pushMessageIntoReceiveQueue( const std::vector<uint8>& buffer, bool bForce = false );
	bool				advertiseDisconnection( TTcpSockId sockid );
	void				SetMaxBufferLength(uint64 len) 
	{ 
		_MaxRecvBufferByte = (uint32)(len >> 2); 
		SetMaxSendFifoSizeByte((uint32)(len >> 2)); 
	}
	bool				CheckReceiveBufferFull();
private:

	/// True after a connection callback has been sent to the user, for this connection
	bool				_Advertised;

	// The task that "owns" the CServerBufSock object
	CServerReceiveTask	*_OwnerTask;
	
	SIPBASE::CSynchronizedFIFO	_RecvFifo;
};


} // SIPNET

#endif // __BUF_SOCK_H__

/* End of buf_sock.h */
