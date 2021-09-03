/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __DUMMY_TCP_SOCK_H__
#define __DUMMY_TCP_SOCK_H__

#include "misc/types_sip.h"
#include "tcp_sock.h"


namespace SIPNET {


/**
 * Dummy CTcpSock replacement for replay mode
 * \date 2001
 */
class CDummyTcpSock : public CTcpSock
{
public:

	// Constructor
	CDummyTcpSock( bool logging = true ) : CTcpSock(logging) {}

	// Dummy connection
	virtual void			connect( const CInetAddress& addr );

	// Dummy disconnection
	virtual void			disconnect();

	// Nothing
	virtual void			setNoDelay( bool value ) {}
	
	// Nothing
	virtual void			close() {}

};


} // SIPNET


#endif // __DUMMY_TCP_SOCK_H__

/* End of dummy_tcp_sock.h */
