/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/dummy_tcp_sock.h"

using namespace SIPBASE;


namespace SIPNET {


/*
 * Set only the remote address
 */
void CDummyTcpSock::connect( const CInetAddress& addr )
{
	_RemoteAddr = addr;
	_Sock = 100;

	_BytesReceived = 0;
	_BytesSent = 0;

	//CSynchronized<bool>::CAccessor sync( &_SyncConnected );
	//sync.value() = true;
	_Connected = true;

	sipdebug( "LNETL0: Socket connected to %s", addr.asString().c_str() );
}


/*
 *Dummy disconnection
 */
void CDummyTcpSock::disconnect()
{
	sipdebug( "LNETL0: Socket disconnecting from %s...", _RemoteAddr.asString().c_str() );

	//CSynchronized<bool>::CAccessor sync( &_SyncConnected );
	//sync.value() = false;
	_Connected = false;
}



} // SIPNET
