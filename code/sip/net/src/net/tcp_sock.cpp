/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/tcp_sock.h"

#ifdef SIP_OS_WINDOWS
//# ifdef SIP_COMP_VC8
#   include <WinSock2.h>
//# endif
# include <windows.h>
# define socklen_t int
# define ERROR_NUM WSAGetLastError()

#elif defined SIP_OS_UNIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <cerrno>
#ifdef __UNIXWARE__
#include <sys/filio.h>
#endif
#define SOCKET_ERROR -1
#define INVALID_SOCKET -1
#define ERROR_NUM errno
#define ERROR_MSG strerror(errno)
typedef int SOCKET;

#endif

using namespace SIPBASE;

namespace SIPNET {


/*
 * Constructor
 */
CTcpSock::CTcpSock( bool logging ) :
	CSock( logging )
{}


/*
 * Construct a CTcpSocket object using an already connected socket 
 */
CTcpSock::CTcpSock( SOCKET sock, const CInetAddress& remoteaddr ) :
	CSock( sock, remoteaddr )
{}


/* Connection. You can reconnect a socket after being disconnected.
 * This method does not return a boolean, otherwise a programmer could ignore the result and no
 * exception would be thrown if connection fails :
 * - If addr is not valid, an exception ESocket is thrown
 * - If connect() fails for another reason, an exception ESocketConnectionFailed is thrown
 */
void CTcpSock::connect( const CInetAddress& addr )
{
	// Create a new socket
	if ( _Sock != INVALID_SOCKET )
	{
	  if ( _Logging )
	    {
//		sipdebug( "LNETL0: Closing socket %d before reconnecting", _Sock );
	    }
	  close();
	}	
	createSocket( SOCK_STREAM, IPPROTO_TCP );

	// activate keep alive
	setKeepAlive(true);

	// Connection
	CSock::connect( addr );
}

void CTcpSock::connect_timeout( const CInetAddress& addr, uint32 sectime )
{
	// Create a new socket
	if ( _Sock != INVALID_SOCKET )
	{
		if ( _Logging )
		{
			//		sipdebug( "LNETL0: Closing socket %d before reconnecting", _Sock );
		}
		close();
	}	
	createSocket( SOCK_STREAM, IPPROTO_TCP );

	// activate keep alive
	setKeepAlive(true);

	// Connection
	CSock::connect_timeout( addr, sectime );
}


/*
 * Active disconnection. After disconnecting, you can't connect back with the same socket.
 */
void CTcpSock::disconnect()
{
	sipdebug( "LNETL0: Socket %d disconnecting from %s...", _Sock, _RemoteAddr.asString().c_str() );

	// This shutdown resets the connection immediatly (not a graceful closure)
#ifdef SIP_OS_WINDOWS
	::shutdown( _Sock, SD_BOTH );
#elif defined SIP_OS_UNIX
	::shutdown( _Sock, SHUT_RDWR );
#endif
	/*CSynchronized<bool>::CAccessor sync( &_SyncConnected );
	sync.value() = false;*/

	setLinger(1);
	_Connected = false;
}

void	CTcpSock::forceclose()
{
	if ( _Logging )
	{
		sipdebug( "LNETL0: Socket %d force closing for %s at %s", _Sock, _RemoteAddr.asString().c_str(), _LocalAddr.asString().c_str() );
	}
	SOCKET sockToClose = _Sock;

	// hard closing...
	setLinger(1, 0);
	_Connected = false;
	_ForceClosed = true;

/*#ifdef SIP_OS_WINDOWS
	closesocket( sockToClose );
#elif defined SIP_OS_UNIX
	::close( sockToClose );
#endif
	_Connected = false;

	// preset to invalid to bypass exception in listen thread
	_Sock = INVALID_SOCKET;
	*/
}

/*
 * Active disconnection for download way only
 */
void CTcpSock::shutdownReceiving()
{
#ifdef SIP_OS_WINDOWS
	::shutdown( _Sock, SD_RECEIVE );
#elif defined SIP_OS_UNIX
	::shutdown( _Sock, SHUT_RD );
#endif
}


/*
 * Active disconnection for upload way only
 */
void CTcpSock::shutdownSending()
{
#ifdef SIP_OS_WINDOWS
	::shutdown( _Sock, SD_SEND );
#elif defined SIP_OS_UNIX
	::shutdown( _Sock, SHUT_WR );
#endif
}

#ifdef SIP_OS_WINDOWS
struct TCP_KEEPALIVE
{
	u_long	onoff;
	u_long	keepalivetime;
	u_long	keepaliveinterval;
};
#endif	//SIP_OS_WINDOWS
	
// #define SIO_KEEPALIVE_VALS IOC_IN | IOC_VENDOR | 4
#define SIO_KEEPALIVE_VALS _WSAIOW(IOC_VENDOR, 4)
void CTcpSock::setKeepAlive( bool keepAlive)
{
	sipassert(_Sock != INVALID_SOCKET);
	int b = keepAlive?1:0;
	if ( setsockopt( _Sock, SOL_SOCKET, SO_KEEPALIVE, (char*)&b, sizeof(b) ) != 0 )
	{
		throw ESocket( "setKeepAlive failed" );
	}
	else
	{
#ifdef SIP_OS_WINDOWS
		if (b)
		{
			TCP_KEEPALIVE inKeepAlive = {0};
			DWORD	dwInLen = sizeof(TCP_KEEPALIVE);
			TCP_KEEPALIVE outKeepAlive = {0};
			DWORD	dwOutLen = sizeof(TCP_KEEPALIVE);
			DWORD	dwReturn = 0;

			// Modified by RSC 2009/11/26
			inKeepAlive.onoff = 1;
			inKeepAlive.keepalivetime = 5000;
			inKeepAlive.keepaliveinterval = 1000;
			if (WSAIoctl(_Sock, SIO_KEEPALIVE_VALS, (LPVOID)&inKeepAlive, dwInLen, (LPVOID)&outKeepAlive, dwOutLen, &dwReturn, NULL, NULL) == SOCKET_ERROR)
			{
				throw ESocket( "SIO_KEEPALIVE_VALS failed" );
			}
		}
#else
		if (b)
		{
			ioctl(_Sock, FIOCLEX, (char *)&b, sizeof(b));
		}
#endif // SIP_OS_WINDOWS
	}
}

void	CTcpSock::setLinger( bool linger, sint16 timeout )
{
	sipassert(_Sock != INVALID_SOCKET);

	struct linger   ling; 

	ling.l_onoff = linger; 
	ling.l_linger = timeout;      /* 0 for abortive disconnect */ 

	if ( setsockopt(_Sock, SOL_SOCKET, SO_LINGER, (const char *)&ling, sizeof(ling)) )
	{
		throw ESocket( "setLinger failed" );
	}
}


/*
 * Sets/unsets TCP_NODELAY
 */
void CTcpSock::setNoDelay( bool value )
{
	int b = value?1:0;
	if ( setsockopt( _Sock, IPPROTO_TCP, TCP_NODELAY, (char*)&b, sizeof(b) ) != 0 )
	{
		throw ESocket( "setNoDelay failed" );
	}
}


/* Sets a custom TCP Window size (SO_RCVBUF and SO_SNDBUF).
 * You must close the socket is necessary, before calling this method.
 *
 * See http://www.ncsa.uiuc.edu/People/vwelch/net_perf/tcp_windows.html
 */
void CTcpSock::connectWithCustomWindowSize( const CInetAddress& addr, int windowsize )
{
	// Create socket
	if ( _Sock != INVALID_SOCKET )
	{
		siperror( "Cannot connect with custom window size when already connected" );
	}
	createSocket( SOCK_STREAM, IPPROTO_TCP );

	// Change window size
	if ( setsockopt( _Sock, SOL_SOCKET, SO_SNDBUF, (char*)&windowsize, sizeof(windowsize) ) != 0
	  || setsockopt( _Sock, SOL_SOCKET, SO_RCVBUF, (char*)&windowsize, sizeof(windowsize) ) != 0 )
	{
		throw ESocket( "setWindowSize failed" );
	}
	
	// Connection
	CSock::connect( addr );
}


/*
 * Returns the TCP Window Size for the current socket
 */
uint32 CTcpSock::getWindowSize()
{
	int windowsize = 0;
	socklen_t len = sizeof( windowsize );

	/* send buffer -- query for buffer size */
	if ( getsockopt( _Sock, SOL_SOCKET, SO_SNDBUF, (char*) &windowsize, &len ) == 0 )
	{
		return windowsize;
	}
	else
	{
		return 0;
	}
}
} // SIPNET
