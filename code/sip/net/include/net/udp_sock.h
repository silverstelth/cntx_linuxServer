/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UDP_SOCK_H__
#define __UDP_SOCK_H__

#include "sock.h"


namespace SIPNET {


/**
 * CUdpSock: Unreliable datagram socket via UDP.
 * See base class CSock.
 * \date 2000-2001
 */
class CUdpSock : public CSock
{
public:

	/// @name Socket setup
	//@{

	/**
	 * Constructor.
	 * \param logging Disable logging if the server socket object is used by the logging system, to avoid infinite recursion
	 */
	CUdpSock( bool logging = true );

	/** Binds the socket to the specified port. Call bind() for an unreliable socket if the host acts as a server and expects to receive
	 * messages. If the host acts as a client, call directly sendTo(), in this case you need not bind the socket.
	 */
	void				bind( uint16 port );

	/// Same as bind(uint16) but binds on a specified address/port (useful when the host has several addresses)
	void				bind( const CInetAddress& addr );

	//@}

	/// @name Receiving data
	//@{

	/**  Receives data from the peer. (blocking function)
	 * The socket must be pseudo-connected.
	 */
	bool				receive( uint8 *buffer, uint32& len, bool throw_exception=true );

	/** Receives data and say who the sender is. (blocking function)
	 * The socket must have been bound before, by calling either bind() or sendTo().
	 * \param buffer [in] Address of buffer
	 * \param len [in/out] Requested length of buffer, and actual number of bytes received
	 * \param addr [out] Address of sender
	 */
	bool				receivedFrom( uint8 *buffer, uint& len, CInetAddress& addr, bool throw_exception=true );

	//@}
	

	/// @name Sending data
	//@{

	/// Sends data to the specified host (unreliable sockets only)
	void				sendTo( const uint8 *buffer, uint len, const CInetAddress& addr );

	//@}

private:

	/// True after calling bind() or sendTo()
	bool				_Bound;

};


} // SIPNET


#endif // __UDP_SOCK_H__

/* End of udp_sock.h */
