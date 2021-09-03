/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TCP_SOCK_H__
#define __TCP_SOCK_H__

#include "sock.h"


namespace SIPNET {


/**
 * CTcpSock: Reliable socket via TCP.
 * See base class CSock.
 * 
 * When to set No Delay mode on ?
 * Set TCP_NODELAY (call setNoDelay(true)) *only* if you have to send small buffers that need to
 * be sent *immediately*. It should only be set for applications that send frequent small bursts
 * of information without getting an immediate response, where timely delivery of data is
 * required (the canonical example is mouse movements). Setting TCP_NODELAY on increases
 * the network traffic (more overhead).
 * In the normal behavior of CSock, TCP_NODELAY is off i.e. the Nagle buffering algorithm is enabled.
 *
 * \date 2000-2001
 */
class CTcpSock : public CSock
{
public:

	/// @name Socket setup
	//@{

	/**
	 * Constructor.
	 * \param logging Disable logging if the server socket object is used by the logging system, to avoid infinite recursion
	 */
	CTcpSock( bool logging = true );

	/// Construct a CTcpSock object using an already connected socket descriptor and its associated remote address
	CTcpSock( SOCKET sock, const CInetAddress& remoteaddr );

	/** Connection. You can reconnect a socket after being disconnected.
	 * This method does not return a boolean, otherwise a programmer could ignore the result and no
	 * exception would be thrown if connection fails :
	 * - If addr is not valid, an exception ESocket is thrown
	 * - If connect() fails for another reason, an exception ESocketConnectionFailed is thrown
	 */
	virtual void		connect( const CInetAddress& addr );

	void				connect_timeout( const CInetAddress& addr, uint32 sectime );

	/** Sets a custom TCP Window size (SO_RCVBUF and SO_SNDBUF).
	 * You must close the socket is necessary, before calling this method.
	 *
	 * See http://www.ncsa.uiuc.edu/People/vwelch/net_perf/tcp_windows.html
	 */
	void				connectWithCustomWindowSize( const CInetAddress& addr, int windowsize );

	/// Returns the TCP Window Size for the current socket
	uint32				getWindowSize();

	/** Sets/unsets SO_KEEPALIVE (true by default).
	 */
	void				setKeepAlive( bool keepAlive);

	/** Sets/unsets SO_LINGER (by default, it is off)
	*/
	void				setLinger( bool linger, sint16 timeout = 500 );

	/** Sets/unsets TCP_NODELAY (by default, it is off, i.e. the Nagle buffering algorithm is enabled).
	 * You must call this method *after* connect().
	 */
	virtual void		setNoDelay( bool value );

	/// Active disconnection for download way only (partial shutdown)
	void				shutdownReceiving();

	/// Active disconnection for upload way only (partial shutdown)
	void				shutdownSending();

	/// Active disconnection (shutdown) (mutexed). connected() becomes false.
	virtual void		disconnect();

	// hard close
	void				forceclose();

	//@}

};


} // SIPNET


#endif // __TCP_SOCK_H__

/* End of tcp_sock.h */
