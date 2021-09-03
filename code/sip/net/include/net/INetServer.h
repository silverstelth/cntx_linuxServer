/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __INETSERVER_H__
#define __INETSERVER_H__

#include "misc/types_sip.h"

#include "callback_net_base.h"
#include "buf_server.h"

namespace	SIPNET
{
class INetServer
{
public:
	virtual	void	init (uint16 nTCPPort, uint16 nUDPPort=0) = 0;
	virtual	void	update2 (sint32 timeout=-1, sint32 mintime=0) = 0;
	virtual	void	update (sint32 timeout=0) = 0;
	virtual	void	release () = 0;

	virtual void	send (const CMessage &buffer, TTcpSockId hostid = InvalidSockId, uint8 bySendType = REL_GOOD_ORDER) = 0;
	virtual	void	receive (SIPNET::CMessage &buffer, SIPNET::TSockId *hostid) = 0;

	//virtual const	CInetAddress&	listenAddress() const = 0;
	//virtual void	addCallbackArray (const TCallbackItem *callbackarray, sint arraysize) = 0;
	//virtual void	setDisconnectionCallback (TNetCallback cb, void *arg) = 0;

public:
	INetServer(void);
	virtual ~INetServer(void);
};

}
#endif // __INETSERVER_H__