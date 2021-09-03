/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/udp_sock.h"

#ifdef SIP_OS_WINDOWS
//# ifdef SIP_COMP_VC8
#  include <WinSock2.h>
//# endif
# include <windows.h>
# define socklen_t int
# define ERROR_NUM WSAGetLastError()

#elif defined SIP_OS_UNIX
# include <unistd.h>
# include <sys/types.h>
# include <sys/time.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
# include <cerrno>
//#include <fcntl.h>
# define SOCKET_ERROR -1
# define INVALID_SOCKET -1
# define ERROR_NUM errno
# define ERROR_MSG strerror(errno)
typedef int SOCKET;

#endif

using namespace SIPBASE;

namespace SIPNET {


/*
 * Constructor
 */
CUdpSock::CUdpSock( bool logging ) :
	CSock( logging ),
	_Bound( false )
{
	// Socket creation
	createSocket( SOCK_DGRAM, IPPROTO_UDP );
}


/** Binds the socket to the specified port. Call bind() for an unreliable socket if the host acts as a server and waits for
 * messages. If the host acts as a client, call sendTo(), there is no need to bind the socket.
 */
void CUdpSock::bind( uint16 port )
{
	CInetAddress addr; // any IP address
	addr.setPort( port );
	bind( addr );
	setLocalAddress(); // will not set the address if the host is multihomed, use bind(CInetAddress) instead
}


/*
 * Same as bind(uint16) but binds on a specified address/port (useful when the host has several addresses)
 */
void CUdpSock::bind( const CInetAddress& addr )
{
#ifndef SIP_OS_WINDOWS
	// Set Reuse Address On (does not work on Win98 and is useless on Win2000)
	int value = true;
	if ( setsockopt( _Sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value) ) == SOCKET_ERROR )
	{
		throw ESocket( "ReuseAddr failed" );
	}
#endif

	_LocalAddr = addr;

	// Bind the socket
	if ( ::bind( _Sock, (sockaddr*)(_LocalAddr.sockAddr()), sizeof(sockaddr) ) == SOCKET_ERROR )
	{
		throw ESocket( "Bind failed" );
	}
	_Bound = true;
	if ( _Logging )
	{
		sipdebug( "LNETL0: Socket %d bound at %s", _Sock, _LocalAddr.asString().c_str() );
	}

	if (_LocalAddr.port() == INADDR_ANY)
		setLocalAddress();
}


/*
 * Sends a message
 */
void CUdpSock::sendTo( const uint8 *buffer, uint len, const CInetAddress& addr )
{
	//  Send
	if ( ::sendto( _Sock, (const char*)buffer, len, 0, (sockaddr*)(addr.sockAddr()), sizeof(sockaddr) ) != (sint32)len )
	{
		throw ESocket( "Unable to send datagram" );
	}
	_BytesSent += len;

	if ( _Logging )
	{
		sipdebug( "LNETL0: Socket %d sent %d bytes to %s", _Sock, len, addr.asString().c_str() );
	}

	// If socket is unbound, retrieve local address
	if ( ! _Bound )
	{
		setLocalAddress();
		_Bound = true;
	}

#ifdef SIP_OS_WINDOWS
	// temporary by ace to know size of SO_MAX_MSG_SIZE
	static bool first = true;
	if (first)
	{
		uint MMS, SB;
		int  size = sizeof (MMS);
		getsockopt (_Sock, SOL_SOCKET, SO_SNDBUF, (char *)&SB, &size);
		getsockopt (_Sock, SOL_SOCKET, SO_MAX_MSG_SIZE, (char *)&MMS, &size);
		sipinfo ("LNETL0: The udp SO_MAX_MSG_SIZE=%u, SO_SNDBUF=%u", MMS, SB);
		first = false;
	}
#endif
}


/*
 * Receives data from the peer. (blocking function)
 */
bool CUdpSock::receive( uint8 *buffer, uint32& len, bool throw_exception )
{
	sipassert( _Connected && (buffer!=NULL) );

	// Receive incoming message
	len = ::recv( _Sock, (char*)buffer, len , 0 );

	// Check for errors (after setting the address)
	if ( ((int)len) == SOCKET_ERROR )
	{
		if ( throw_exception )
			throw ESocket( "Cannot receive data" );
		return false;
	}

	_BytesReceived += len;
	if ( _Logging )
	{
		sipdebug( "LNETL0: Socket %d received %d bytes from peer %s", _Sock, len, _RemoteAddr.asString().c_str() );
	}
	return true;
}


/*
 * Receives data and say who the sender is. (blocking function)
 */
bool CUdpSock::receivedFrom( uint8 *buffer, uint& len, CInetAddress& addr, bool throw_exception )
{
	// Receive incoming message
	sockaddr_in saddr;
	socklen_t saddrlen = sizeof(saddr);

	len = ::recvfrom( _Sock, (char*)buffer, len , 0, (sockaddr*)&saddr, &saddrlen );

	// If an error occurs, the saddr is not valid
	// When the remote socket is closed, get sender's address to know who is quitting
	addr.setSockAddr( &saddr );

	// Check for errors (after setting the address)
	if ( ((int)len) == SOCKET_ERROR )
	{
		if (throw_exception)
			throw ESocket( "Cannot receive data" );
		return false;
	}

	_BytesReceived += len;
	if ( _Logging )
	{
		sipdebug( "LNETL0: Socket %d received %d bytes from %s", _Sock, len, addr.asString().c_str() );
	}
	return true;
}


} // SIPNET
