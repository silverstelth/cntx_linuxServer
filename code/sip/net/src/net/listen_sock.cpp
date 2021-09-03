/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/listen_sock.h"

#ifdef SIP_OS_WINDOWS

#include <windows.h>
typedef sint socklen_t;

#elif defined SIP_OS_UNIX

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cerrno>
#include <fcntl.h>
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
typedef int SOCKET;

#endif


using namespace std;


namespace SIPNET
{


/*
 * Constructor
 */
CListenSock::CListenSock() : CTcpSock(), _Bound( false )
{
	// Create socket
	createSocket( SOCK_STREAM, IPPROTO_TCP );

	/// \todo cado: tune backlog value, not too small, not to big (20-200) to prevent SYN attacks (see http://www.cyberport.com/~tangent/programming/winsock/advanced.html)
	setBacklog( 200 );
}


/*
 * Prepares to receive connections on a specified port
 */
void CListenSock::init( uint16 port )
{
    // Use any address
	CInetAddress localaddr; // By default, INETADDR_ANY (useful for gateways that have several ip addresses)
	localaddr.setPort( port );
	init( localaddr );

	// Now set the address visible from outside
	_LocalAddr = CInetAddress::localHost();
	_LocalAddr.setPort( port );
	sipdebug( "LNETL0: Socket %d listen socket is at %s", _Sock, _LocalAddr.asString().c_str() );
}


/*
 * Prepares to receive connections on a specified address/port (useful when the host has several addresses)
 */
void CListenSock::init( const CInetAddress& addr )
{
	if ( ! addr.isValid() )
	{
		sipdebug( "LNETL0: Binding listen socket to any address, port %hu", addr.port() );
	}

#ifndef SIP_OS_WINDOWS
	// Set Reuse Address On (does not work on Win98 and is useless on Win2000)
	int value = true;
	if ( setsockopt( _Sock, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value) ) == SOCKET_ERROR )
	{
		throw ESocket( "ReuseAddr failed" );
	}
#else
	//use for P2P at WindowsXP
	//by NamJuSong 2008/5/1
	int value = true;
	if ( setsockopt( _Sock, SOL_SOCKET, SO_REUSEADDR, (char*)&value, sizeof(value) ) == SOCKET_ERROR )
	{
		sipwarning("LNETL0: setsockopt SO_REUSEADDR failed");
	}
#endif

	// Bind socket to port	
	if ( ::bind( _Sock, (const sockaddr *)addr.sockAddr(), sizeof(sockaddr_in) ) != 0 )
	{
		throw ESocket( "Unable to bind listen socket to port" );
	}
	_LocalAddr = addr;
	_Bound = true;

	// Listen
	if ( ::listen( _Sock, _BackLog ) != 0 ) // SOMAXCONN = maximum length of the queue of pending connections
	{
		throw ESocket( "Unable to listen on specified port" );
	}
//	sipdebug( "LNETL0: Socket %d listening at %s", _Sock, _LocalAddr.asString().c_str() );
}


/*
 * Accepts an incoming connection, and creates a new socket
 */
CTcpSock *CListenSock::accept()
{
	// Accept connection
	sockaddr_in saddr;
	socklen_t saddrlen = sizeof(saddr);
	SOCKET newsock = ::accept( _Sock, (sockaddr*)&saddr, &saddrlen );
	if ( newsock == INVALID_SOCKET )
	{
		if (_Sock == INVALID_SOCKET)
			// normal case, the listen sock have been closed, just return NULL.
			return NULL;

	  /*sipinfo( "LNETL0: Error accepting a connection");
	  // See accept() man on Linux
	  newsock = ::accept( _Sock, (sockaddr*)&saddr, &saddrlen );
	  if ( newsock == INVALID_SOCKET )*/
	    {
			throw ESocket( "Accept returned an invalid socket");
	    }
	}

	// Construct and save a CTcpSock object
	CInetAddress addr;
	addr.setSockAddr( &saddr );
	sipdebug( "LNETL0: Socket %d accepted an incoming connection from %s, opening socket %d", _Sock, addr.asString().c_str(), newsock );
	CTcpSock *connection = new CTcpSock( newsock, addr );
	return connection;
}


/*
 * Sets the number of the pending connections queue. -1 for the maximum possible value.
 */
void CListenSock::setBacklog( sint backlog )
{
	if ( backlog == -1 )
	{
		_BackLog = SOMAXCONN; // SOMAXCONN = maximum length of the queue of pending connections
	}
	else
	{
		_BackLog = backlog;
	}
	if ( _Bound )
	{
		if ( ::listen( _Sock, _BackLog ) != 0 )
		{
			throw ESocket( "Unable to listen on specified port, while changing backlog" );
		}
	}
}


} // SIPNET
