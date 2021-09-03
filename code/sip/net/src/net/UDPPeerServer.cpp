/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/UDPPeerServer.h"

using namespace	std;
using namespace	SIPBASE;

namespace	SIPNET
{
CUDPPeerServer::CUDPPeerServer(void):
	m_sock(NULL)
{
	_pkNo = 0;
	_pkRepeatNo = 0;
}

CUDPPeerServer::~CUDPPeerServer(void)
{
}

void	CUDPPeerServer::init(uint16 port)
{
	m_sock = new CUdpSock();
	CInetAddress	addr;
	addr.setPort(port);
	m_sock->bind(port);
	sipinfo("UDP Peer Server init: %s", addr.asIPString().c_str());
}

void	CUDPPeerServer::dispatchCallback(SIPNET::CMessage &msg, SIPNET::CInetAddress addr)
{
	MsgNameType name = msg.getName ();
	std::map<MsgNameType, TUDPCallbackItem>::iterator callbackfind;
	callbackfind = _CallbackArray.find(name);

	bool	bFind = true;
	if (callbackfind == _CallbackArray.end())
		bFind = false;

	TUDPMsgServerCallback	cb = NULL;
	if (bFind)
	{
		cb = callbackfind->second.Callback;

		if ( cb != NULL )
			cb(msg, addr, this);
	}
}

bool	CUDPPeerServer::update()
{
	if ( m_sock->dataAvailable() )
	{
		uint8	buffer[512];
		uint	len = 512;
		CInetAddress	addr;
		if ( m_sock->receivedFrom(buffer, len, addr, false) )
		{
			MsgNameType msgName = CMessage::GetMsgNameFromBuffer(buffer, len);

			map<MsgNameType, TUDPCallbackItem>::iterator callbackfind;
			callbackfind = _CallbackArray.find(msgName);

			if (callbackfind != _CallbackArray.end())
			{
				CMessage msg(M_SYS_EMPTY);
				msg.serialBuffer(buffer, len);
				msg.invert();
				msg.serial(_pkNo);
				msg.serial(_pkRepeatNo);
				uint8	*buff	= buffer + msg.getHeaderSize() + sizeof(uint32) + sizeof(uint32);
				len		-= ( msg.getHeaderSize() + sizeof(uint32) + sizeof(uint32) );

				CMessage msgin(msg.getName());
				msgin.serialBuffer(buff, len);
				msgin.invert();

				dispatchCallback(msgin, addr);
			}
		}
	}
	return true;
}

void	CUDPPeerServer::release()
{
}

// ksr
void	CUDPPeerServer::sendTo(SIPNET::CMessage	&msg, CInetAddress & addr)
{
	uint32		len			= msg.length();
	MsgNameType	sName		= msg.getName();

	CMessage	msgout(sName);
	msgout.serial(_pkNo);
	msgout.serial(_pkRepeatNo);
	uint8	* buffer0 = const_cast<uint8 *> (msg.buffer() + msg.getHeaderSize());
	msgout.serialBuffer(buffer0, len);
	uint8	* buffer = const_cast<uint8 *> (msgout.buffer());

	try
	{
		m_sock->sendTo(buffer, msgout.length(), addr);
	}
	catch(ESocket e)
	{
		sipdebug("UDPSocket Send Error: ");
	}

	//sipinfo("Send UDP Message <%s> To Client %s", msg.getName().c_str(), addr.asIPString().c_str());
}

void	CUDPPeerServer::addCallbackArray (const TUDPCallbackItem *callbackarray, sint arraysize)
{
	if (arraysize == 1 && callbackarray[0].Callback == NULL && M_SYS_EMPTY == callbackarray[0].Key)
	{
		// it's an empty array, ignore it
		return;
	}

	for (sint i = 0; i < arraysize; i++)
	{
		MsgNameType	strMes = callbackarray[i].Key;
		_CallbackArray.insert( std::make_pair(strMes, callbackarray[i]) );
	}
}
}