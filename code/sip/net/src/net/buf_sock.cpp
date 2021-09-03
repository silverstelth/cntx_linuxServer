/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "misc/hierarchical_timer.h"
#include "net/buf_sock.h"
#include "net/buf_server.h"


#ifdef SIP_OS_WINDOWS
#include <windows.h>
#elif defined SIP_OS_UNIX
#include <netinet/in.h>
#endif

using namespace SIPBASE;
using namespace std;


namespace SIPNET {


SIPBASE::CMutex nettrace_mutex("nettrace_mutex");


/*
 * Constructor
 */
CBufSock::CBufSock( CTcpSock *sock ) :
	SendNextValue(0),
	ReceiveNextValue(0),
	Sock( sock ),
	_KnowConnected( false ),
	_LastFlushTime( 0 ),
	_TriggerTime( 20 ),
	_TriggerSize( -1 ),
	_RTSBIndex( 0 ),
	_AppId( 0 ),
	AuthorizedDuring(0),
	_ConnectedState( false ),
	_MaxSendBufferByte(1024 * 1024 * 100)	// 100M for client
{
	// Added by RSC 2009/12/01
	AuthorizedCallback = M_SYS_EMPTY;

	if ( Sock == NULL )
	  {
		Sock = new CTcpSock();
	  }

#ifdef SIP_DEBUG
	_FlushTrigger = FTManual;
#endif
	_LastFlushTime = CTime::getLocalTime();

	_bOccurSendbufferOverflow = false;
	_bOccurRecvbufferOverflow = false;
	_bOccurSendZero = false;
	_SendbufferOverflowNumber = 0;
	_RecvbufferOverflowNumber = 0;
	_SendZeroNumber = 0;

}


/*
 * Destructor
 */
CBufSock::~CBufSock()
{
	sipassert (this != InvalidSockId);	// invalid bufsock

	delete Sock; // the socket disconnects automatically if needed
	
	// destroy the structur to be sure that other people will not access to this anymore
	AuthorizedCallback = M_SYS_EMPTY;
	Sock = NULL;
	_KnowConnected = false;
	_LastFlushTime = 0;
	_TriggerTime = 0;
	_TriggerSize = 0;
	_ReadyToSendBuffer.clear ();
	_RTSBIndex = 0;
	_AppId = 0;
	_ConnectedState = false;
}


/*
 * Returns a readable string from a vector of bytes, beginning from pos, limited to 'len' characters. '\0' are replaced by ' '
 */
string stringFromVectorPart( const vector<uint8>& v, uint32 pos, uint32 len )
{
	sipassertex( pos+len <= v.size(), ("pos=%u len=%u size=%u", pos, len, v.size()) );

	string s;
	if ( (! v.empty()) && (len!=0) )
	{
		// Copy contents
		s.resize( len );
		memcpy( &*s.begin(), &*v.begin()+pos, len );

		// Replace '\0' characters
		string::iterator is;
		for ( is=s.begin(); is!=s.end(); ++is )
		{
			if ( ! isprint((uint8)(*is)) || (*is) == '%' )
			{
				(*is) = '?';
			}
		}
	}

	return s;
}


/*
 * Force to send data pending in the send queue now. In the case of a non-blocking socket
 * (see CNonBlockingBufSock), if all the data could not be sent immediately,
 * the returned nbBytesRemaining value is non-zero.
 * \param nbBytesRemaining If the pointer is not NULL, the method sets the number of bytes still pending after the flush attempt.
 * \returns false if an error has occured (e.g. the remote host is disconnected).
 * To retrieve the reason of the error, call CSock::getLastError() and/or CSock::errorString()
 *
 * Note: this method works with both blocking and non-blocking sockets
 * Precond: the send queue should not contain an empty block
 */
bool CBufSock::flush( uint *nbBytesRemaining )
{
	sipassert (this != InvalidSockId);	// invalid bufsock

	// Copy data from the send queue to _ReadyToSendBuffer
	TBlockSize netlen;
//	vector<uint8> tmpbuffer;

	// Process each element in the send queue
	if ( _ReadyToSendBuffer.size() == 0 )
	{
		_ReadyToSendBuffer.resize( SendFifo.size() );
		int nCurSize = 0;
		while ( ! SendFifo.empty() )
		{
			uint8 *tmpbuffer;
			uint32 size;
			SendFifo.front( tmpbuffer, size );

			// Compute the size and add it into the beginning of the buffer
			netlen = htonl( (TBlockSize)size );
			uint32 oldBufferSize = nCurSize;
			nCurSize = (oldBufferSize+sizeof(TBlockSize)+size);
			*(TBlockSize*)&(_ReadyToSendBuffer[oldBufferSize])=netlen;
			//sipdebug( "O-%u %u+L%u (0x%x)", Sock->descriptor(), oldBufferSize, size, size );


			// Append the temporary buffer to the global buffer
			CFastMem::memcpy (&_ReadyToSendBuffer[oldBufferSize+sizeof(TBlockSize)], tmpbuffer, size);
			SendFifo.pop();
		}
	}

	// Actual sending of _ReadyToSendBuffer
	//if ( ! _ReadyToSendBuffer.empty() )
	if ( _ReadyToSendBuffer.size() != 0 )
	{		
		// Send
		CSock::TSockResult res;
		TBlockSize len = _ReadyToSendBuffer.size() - _RTSBIndex;
		res = Sock->send( _ReadyToSendBuffer.getPtr()+_RTSBIndex, len, false );
		if ( res == CSock::Ok )
		{
			// TODO OPTIM remove the zdebug for speed
			//commented for optimisation sipdebug( "LNETL1: %s sent effectively %u/%u bytes (pos %u wantedsend %u)", asString().c_str(), len, _ReadyToSendBuffer.size(), _RTSBIndex, realLen/*, stringFromVectorPart(_ReadyToSendBuffer,_RTSBIndex,len).c_str()*/ );
			if (len == 0)
			{
				_SendZeroNumber ++;
//				if (_bOccurSendZero == false)
//					sipwarning( "LNETL1: %s failed to send, len = 0, number=%d", asString().c_str(), _SendZeroNumber );
				_bOccurSendZero = true;
			}
			else
			{
//				if (_bOccurSendZero)
//					sipwarning( "LNETL1: %s sending come back normality, number=%d", asString().c_str(), _SendZeroNumber );
				_bOccurSendZero = false;
			}

			if ( _RTSBIndex+len == _ReadyToSendBuffer.size() ) // for non-blocking mode
			{
				// If sending is ok, clear the global buffer
				//sipdebug( "O-%u all %u bytes (%u to %u) sent", Sock->descriptor(), len, _RTSBIndex, _ReadyToSendBuffer.size() );
				_ReadyToSendBuffer.clear();
				_RTSBIndex = 0;
				if ( nbBytesRemaining )
					*nbBytesRemaining = 0;
			}
			else
			{
				// Or clear only the data that was actually sent
				sipassertex( _RTSBIndex+len < _ReadyToSendBuffer.size(), ("index=%u len=%u size=%u", _RTSBIndex, len, _ReadyToSendBuffer.size()) );
				_RTSBIndex += len;
				if ( nbBytesRemaining )
					*nbBytesRemaining = _ReadyToSendBuffer.size() - _RTSBIndex;
				/*
				if ( _ReadyToSendBuffer.size() > 20480 ) // if big, clear data already sent
				{
				uint nbcpy = _ReadyToSendBuffer.size() - _RTSBIndex;
				for (uint i = 0; i < nbcpy; i++)
				{
				_ReadyToSendBuffer[i] = _ReadyToSendBuffer[i+_RTSBIndex];
				}
				_ReadyToSendBuffer.resize(nbcpy);
				_RTSBIndex = 0;
				}
				*/
			}
		}
		else
		{
#ifdef SIP_DEBUG
			// Can happen in a normal behavior if, for example, the other side is not connected anymore
			sipdebug( "LNETL1: %s failed to send effectively a buffer of %d bytes", asString().c_str(), _ReadyToSendBuffer.size() );
#endif
			// NEW: Clearing (loosing) the buffer if the sending can't be performed at all
			_ReadyToSendBuffer.clear();
			_RTSBIndex = 0;
			if ( nbBytesRemaining )
				*nbBytesRemaining = 0;
			return false;
		}
	}
	else
	{
		if ( nbBytesRemaining )
			*nbBytesRemaining = 0;
	}

	return true;
}


/* Sets the time flush trigger (in millisecond). When this time is elapsed,
 * all data in the send queue is automatically sent (-1 to disable this trigger)
 */
void CBufSock::setTimeFlushTrigger( sint32 ms )
{
	sipassert (this != InvalidSockId);	// invalid bufsock
	_TriggerTime = ms;
	_LastFlushTime = CTime::getLocalTime();
}


/*
 * Update the network sending (call this method evenly). Returns false if an error occured.
 */
bool CBufSock::update()
{
	sipassert (this != InvalidSockId);	// invalid bufsock

	// check authorized packet during

//	if (!AuthorizedCallback.empty() && AuthorizedDuring != 0)
	if (AuthorizedCallback != M_SYS_EMPTY && AuthorizedDuring != 0)
	{
		TTime curtime = CTime::getLocalTime();
		if (curtime - AuthorizedStartTime > AuthorizedDuring)
		{
#ifdef SIP_MSG_NAME_DWORD
			sipwarning("LNETL1: Time out for authorized packet(%d)", AuthorizedCallback);
#else
			sipwarning("LNETL1: Time out for authorized packet(%s)", AuthorizedCallback.c_str());
#endif // SIP_MSG_NAME_DWORD
			Sock->forceclose();
			//Sock->disconnect();
		}
	}

	// Time trigger
	if ( _TriggerTime != -1 )
	{
		TTime now = CTime::getLocalTime();
		if ( (sint32)(now-_LastFlushTime) >= _TriggerTime )
		{
#ifdef SIP_DEBUG
			_FlushTrigger = FTTime;
#endif
			if ( flush() )
			{
				_LastFlushTime = now;
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	// Size trigger
	if ( _TriggerSize != -1 )
	{
		sint32 sendfifosize = SendFifo.size();
		if ( sendfifosize > _TriggerSize )
		{
#ifdef SIP_DEBUG
			_FlushTrigger = FTSize;
#endif
			return flush();
		}
	}
	return true;
}


/*
 * Connects to the specified addr; set connectedstate to true if no connection advertising is needed
 * Precond: not connected
 */
void CBufSock::connect( const CInetAddress& addr, bool nodelay, bool connectedstate, uint32 sectime)
{
	sipassert (this != InvalidSockId);	// invalid bufsock
	sipassert( ! Sock->connected() );

	SendNextValue = ReceiveNextValue = 0;
	
	if ( sectime == 0 )
		Sock->connect( addr );
	else
		Sock->connect_timeout( addr, sectime );

	_ConnectedState = connectedstate;
	_KnowConnected = connectedstate;
	if ( nodelay )
	{
		Sock->setNoDelay( true );
	}
	_ReadyToSendBuffer.clear();
	_RTSBIndex = 0;
}


/*
 * Disconnects; set connectedstate to false if no disconnection advertising is needed
 */
void CBufSock::disconnect( bool connectedstate )
{
	sipassert (this != InvalidSockId);	// invalid bufsock
	Sock->disconnect();
	_ConnectedState = connectedstate;
	_KnowConnected = connectedstate;

	SendNextValue = ReceiveNextValue = 0;
}

void CBufSock::forceClose()
{
    sipassert(this != InvalidSockId);
	Sock->forceclose();
	//_ConnectedState = false;
	//_KnowConnected = false;
}

/*
 * Returns a string with the characteristics of the object
 */
string CBufSock::asString() const
{
//	stringstream ss;
	string str;
	if (this == InvalidSockId) // tricky
		str = "<null>";
	else
	{
		// if it crashs here, it means that the CBufSock was deleted and you try to access to the virtual table that is empty
		// because the object is destroyed.
		str += typeStr();
		str += SIPBASE::toStringPtr(this) + " (socket ";
		
		if (Sock == NULL)
			str += "<null>";
		else
			str += SIPBASE::toStringA(Sock->descriptor());

		str += ")";
	}
	return str;
}

bool	CBufSock::procAcceptMsg(uint32 MesID, TTime IntervalTime, uint32 SerialMaxNum)
{
	TTime	tmCur = CTime::getLocalTime();
	std::map<TMesID, PHISDATA>::iterator it = _MsgAcceptHis.find(MesID);
	if (it == _MsgAcceptHis.end())
	{
		PHISDATA	newMes;
		newMes.tmLastAccept	= tmCur;
		newMes.uSerialNum = 0;
		_MsgAcceptHis.insert(make_pair(MesID, newMes));
		return true;
	}
	TTime	tmLast = it->second.tmLastAccept;
	TTime	tmInterval = (tmCur - tmLast);

	it->second.tmLastAccept = tmCur;

	if (tmInterval < IntervalTime)
	{
		it->second.uSerialNum ++;
		if (it->second.uSerialNum > SerialMaxNum)
			return false;
	}
	else
	{
		it->second.uSerialNum = 0;
	}

	return true;
}


/*
 * Constructor
 */
CNonBlockingBufSock::CNonBlockingBufSock( CTcpSock *sock, uint32 maxExpectedBlockSize ) :
	CBufSock( sock ),
	_MaxExpectedBlockSize( maxExpectedBlockSize ),
	_NowReadingBuffer( false ),
	_BytesRead( 0 ),
	_Length( 0 )
{
}


/*
 * Constructor with an existing socket (created by an accept())
 */
CServerBufSock::CServerBufSock( CTcpSock *sock ) :
	CNonBlockingBufSock( sock ),
	_RecvFifo("CServerBufSock::_RecvFifo"),
	_Advertised( false ),
	_OwnerTask( NULL ),
	_UdpFriend( NULL ),
	_MaxRecvBufferByte(1024*1024*100)
{
	sipassert (this != InvalidSockId);	// invalid bufsock
}

bool	CServerBufSock::CheckReceiveBufferFull()
{
	CFifoAccessor recvfifo( &_RecvFifo );
	uint32	uBufferLength = recvfifo.value().size();

	if (uBufferLength + 4096 >= _MaxRecvBufferByte)
		return true;
	return false;
}

void	CServerBufSock::pushMessageIntoReceiveQueue( const std::vector<uint8>& buffer, bool bForce )
{
	if (!bForce && CheckReceiveBufferFull())
	{
		_RecvbufferOverflowNumber ++;
		if (!_bOccurRecvbufferOverflow)
		{
			sipwarning("LNETL1: socket %s 's receive buffer is full, number=%d, MaxBufferByte=%d", asString().c_str(), _RecvbufferOverflowNumber, _MaxRecvBufferByte);
			forceClose();
		}
		_bOccurRecvbufferOverflow = true;
		return;
	}
	_bOccurRecvbufferOverflow = false;
	{
		CFifoAccessor recvfifo( &_RecvFifo );
		recvfifo.value().push( buffer );
	}
}

bool	CServerBufSock::advertiseDisconnection( TTcpSockId sockid )
{
	bool flag = sockid->_KnowConnected;
	bool condition = true;
	CBufNetBase::TEventType event = CBufNetBase::Disconnection;

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
			// KSR_ADD
			buffer.resize( sizeof(TTcpSockId) + sizeof(uint32) + 1 );

			memcpy( &*buffer.begin(), &sockid, sizeof(TTcpSockId) );

			uint32	curtime = SIPBASE::CTime::getTick();
			if ( event == CBufNetBase::Connection )
				memcpy( &*buffer.begin() + sizeof(TTcpSockId), &curtime, sizeof(uint32) );

			buffer[sizeof(TTcpSockId) + sizeof(uint32)] = event;
			// KSR_ADD
		}
		pushMessageIntoReceiveQueue( buffer, true );

		// Reset flag
		sockid->_KnowConnected = !condition;
		return true;
	}
	else
	{
		return false;
	}

}

// In Receive Threads:


/*
 * Receives a part of a message (nonblocking socket only)
 */
bool CNonBlockingBufSock::receivePart( uint32 nbExtraBytes )
{
	sipassert (this != InvalidSockId);	// invalid bufsock

	TBlockSize actuallen;
	if ( ! _NowReadingBuffer )
	{
		// Receiving length prefix
		actuallen = sizeof(_Length)-_BytesRead;
		CSock :: TSockResult ret = Sock->receive( (uint8*)(&_Length)+_BytesRead, actuallen, false );
		if (ret == CSock::ConnectionClosed)
		{
			sipdebug( "LNETL1: Connection %s closed", asString().c_str() );
			return false;
		}
		else if (ret == CSock::Error)
		{
			sipdebug( "LNETL1: Socket error for %s", asString().c_str() );
			Sock->disconnect();
			return false;
		}

		_BytesRead += actuallen;
		if ( _BytesRead == sizeof(_Length ) )
		{
			if ( _Length != 0 )
			{
				_Length = ntohl( _Length );
				//sipdebug( "I-%u L%u (0x%x) a%u", Sock->descriptor(), _Length, _Length, actuallen );

				// Test size limit
				if ( _Length > _MaxExpectedBlockSize )
				{
					sipwarning( "LNETL1: Socket %s received header length %u exceeding max expected %u... Disconnecting,IP=%s", asString().c_str(), _Length, _MaxExpectedBlockSize, getTcpSock()->remoteAddr().ipAddress().c_str() );
					throw ESocket( toString( "Received length %u exceeding max expected %u from %s", _Length, _MaxExpectedBlockSize, Sock->remoteAddr().asString().c_str() ).c_str(), false );
				}

				_NowReadingBuffer = true;
				_ReceiveBuffer.resize( _Length + nbExtraBytes );
			}
			else
			{
				sipwarning( "LNETL1: Socket %s received null length in block header", asString().c_str() );
			}
			_BytesRead = 0;
		}
	}

	if ( _NowReadingBuffer )
	{
		// Receiving payload buffer
		actuallen = _Length-_BytesRead;
		Sock->receive( &*_ReceiveBuffer.begin()+_BytesRead, actuallen );
		_BytesRead += actuallen;

		if ( _BytesRead == _Length )
		{
#ifdef SIP_DEBUG
			sipdebug( "LNETL1: %s received buffer (%u bytes): [%s]", asString().c_str(), _ReceiveBuffer.size(), stringFromVector(_ReceiveBuffer).c_str() );
#endif
			_NowReadingBuffer = false;
			//sipdebug( "I-%u all %u B on %u", Sock->descriptor(), actuallen );
			_BytesRead = 0;
			return true;
		}
		//else
		//{
		//	sipdebug( "I-%u only %u B on %u", actuallen, Sock->descriptor(), _Length-(_BytesRead-actuallen) );
		//}
	}

	return false;
}


} // SIPNET
