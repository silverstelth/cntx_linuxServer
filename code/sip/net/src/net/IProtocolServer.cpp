/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/IProtocolServer.h"

using namespace std;
using namespace	SIPBASE;

namespace SIPNET
{

IProtocolServer::IProtocolServer(void)
{
}

IProtocolServer::IProtocolServer(INetServer * server)
{
	_netServer = server;
}

IProtocolServer::~IProtocolServer(void)
{
}

void	IProtocolServer::init (uint16 nTCPPort, uint16 nUDPPort)
{
}

void	IProtocolServer::release()
{
}

/*const	CInetAddress&	IProtocolServer::listenAddress() const
{
	return	_netServer->listenAddress();
}
*/

void	IProtocolServer::addCallbackArray (const SIPNET::TUDPCallbackItem *callbackarray, sint arraysize)
{
}

void	IProtocolServer::send (const SIPNET::CMessage &buffer, TTcpSockId hostid, uint8 bySendType)
{
	if ( !_netServer )
        _netServer->send(buffer, hostid, bySendType);
}

void	IProtocolServer::receive (SIPNET::CMessage &buffer, SIPNET::TSockId *hostid)
{
	if ( !_netServer )
        _netServer->receive(buffer, hostid);
}

void	IProtocolServer::update2 (sint32 timeout, sint32 mintime)
{
	if ( !_netServer )
        _netServer->update2(timeout, mintime);
}

void	IProtocolServer::update (sint32 timeout)
{
	if ( !_netServer )
        _netServer->update(timeout);
}

}// namespace SIPNET
