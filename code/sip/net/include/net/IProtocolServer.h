/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __IPROTOCOLSERVER_H__
#define __IPROTOCOLSERVER_H__

#include "INetServer.h"
#include "UDPPeerServer.h"
namespace	SIPNET
{
class IProtocolServer : public	SIPNET::INetServer
{
public:
	virtual	void	init (uint16 nTCPPort, uint16 nUDPPort=0);
	virtual	void	update2 (sint32 timeout=-1, sint32 mintime=0);
	virtual	void	update (sint32 timeout=0);
	virtual	void	release ();

	virtual	void	send (const CMessage &buffer, TTcpSockId hostid = InvalidSockId, uint8 bySendType = REL_GOOD_ORDER);
	virtual	void	receive (SIPNET::CMessage &buffer, SIPNET::TSockId *hostid);

	virtual	void	addCallbackArray (const SIPNET::TUDPCallbackItem *callbackarray, sint arraysize);
	//void	setDisconnectionCallback (TNetCallback cb, void *arg);
	//void	setConnectionCallback (TNetCallback cb, void *arg);

	//virtual const	CInetAddress&	listenAddress() const;

public:
	IProtocolServer();
	IProtocolServer(SIPNET::INetServer * server);
	virtual ~IProtocolServer(void);

protected:
	SIPNET::INetServer *	_netServer;
};
} // namespace SIPNET

#endif // __IPROTOCOLSERVER_H__