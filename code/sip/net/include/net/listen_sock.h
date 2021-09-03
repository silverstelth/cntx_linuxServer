/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __LISTEN_SOCK_H__
#define __LISTEN_SOCK_H__


#include "tcp_sock.h"

namespace SIPNET
{


/**
 * CListenSock: listening socket for servers.
 * How to accept connections in a simple server:
 * -# Create a CListenSock object
 * -# Listen on the port you want the clients to connect
 * -# In a loop, accept a connection and store the new socket
 *
 * \date 2000-2001
 */
class CListenSock : public CTcpSock
{
public:

	/// Constructor
	CListenSock();

	///@name Socket setup
	//@{

	/// Prepares to receive connections on a specified port (bind+listen)
	void			init( uint16 port );

	/// Prepares to receive connections on a specified address/port (useful when the host has several addresses)
	void			init( const CInetAddress& addr );

	/// Sets the number of the pending connections queue, or -1 for the maximum possible value.
	void			setBacklog( sint backlog );

	/// Returns the pending connections queue.
	sint			backlog() const { return _BackLog; }

	//@}

	/// Blocks until an incoming connection is requested, accepts it, and creates a new socket (you have to delete it after use)
	CTcpSock		*accept();

private:

	bool			_Bound;

	sint			_BackLog;
};

} // SIPNET

#endif // __LISTEN_SOCK_H__

/* End of listen_sock.h */
