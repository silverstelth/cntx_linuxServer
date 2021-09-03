/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/TimerRegistry.h"
#include "net/callback_client.h"
#include "net/UDPPeerClient.h"

namespace	SIPNET
{
CUDPPeerClient::CUDPPeerClient(CCallbackClient * client):
	m_sock(NULL), m_client(client), m_pTimerRegistry(NULL)
{
}

CUDPPeerClient::CUDPPeerClient():
	m_sock(NULL), m_client(NULL), m_pTimerRegistry(NULL)
{
}

CUDPPeerClient::~CUDPPeerClient(void)
{
}

void	CUDPPeerClient::init(CCallbackClient * client, uint16 port)
{
	m_client = client;
	m_pTimerRegistry = new CTimerRegistry;

	CInetAddress	addr;
	try
	{
		// Modified by RSC 2009/11/26
		if(m_sock == NULL)
			m_sock = new CUdpSock();

		addr.setPort(port);
		m_sock->bind(port);
		sipinfo("UDP Peer Client init: %s", addr.asIPString().c_str());
	}
	catch(ESocket & e)
	{
		sipinfo("UDP Socket Bind to Address:%s, Error, %s", addr.asIPString().c_str(), e.what());
	}
}

void	CUDPPeerClient::init()
{
	CInetAddress	addr;
	m_pTimerRegistry = new CTimerRegistry;

	try
	{
		// Modified by RSC 2009/11/26
		if(m_sock == NULL)
			m_sock = new CUdpSock();
	}
	catch(ESocket & e)
	{
		sipinfo("UDP Socket Bind to Address:%s, Error, %s", addr.asIPString().c_str(), e.what());
	}
}

bool	CUDPPeerClient::update()
{
	if ( m_sock->dataAvailable() )
	{
		uint8	buffer[512];
		uint	len = 512;
		CInetAddress	addr;
		if ( m_sock->receivedFrom(buffer, len, addr, false) )
		{
			CMessage msg(M_SYS_EMPTY);
			msg.serialBuffer(buffer, len);
			msg.invert();
			uint32	nTimerNo;	msg.serial(nTimerNo);
			uint32	nSubPKNo;	msg.serial(nSubPKNo);
			uint8	*buff	= buffer + msg.getHeaderSize() + sizeof(uint32) + sizeof(uint32);
			len		-= ( msg.getHeaderSize() + sizeof(uint32) + sizeof(uint32) );

			CMessage msgin(msg.getName());
			msgin.serialBuffer(buff, len);
			msgin.invert();

			//  Modify TimerRegistry singleton -> member object, for MT [2/23/2010 KimSuRyon]
			sipassert( m_pTimerRegistry );
//			bool	bTimer = m_pTimerRegistry->isWaiting(nTimerNo, nSubPKNo);
//			if ( bTimer )
			{
				dispatchCallback(msgin, addr);
				//  Modify TimerRegistry singleton -> member object, for MT [2/23/2010 KimSuRyon]
//				m_pTimerRegistry->unregisterTimer(nTimerNo);
			}
		}
	}

	//  Modify TimerRegistry singleton -> member object, for MT [2/23/2010 KimSuRyon]
	m_pTimerRegistry->update(this);

	return true;
}

void	CUDPPeerClient::release()
{
	delete m_pTimerRegistry;
}

void	CUDPPeerClient::connectPeer(CInetAddress& peeraddr)
{
	m_sock->connect(peeraddr);
}

bool	CUDPPeerClient::sendTo(CMessage	&msg, CInetAddress & addr)
{
	sipdebug("CUDPPeerClient::sendTo enter");

	//  Modify TimerRegistry singleton -> member object, for MT [2/23/2010 KimSuRyon]
	sipassert(m_pTimerRegistry);
	uint32	nTimerCounter	= m_pTimerRegistry->getTimerCounter();
	uint32	nRepeateNo		= 0;

//	m_pTimerRegistry->registerTimer(msg, addr);

	sendToWithTrust(msg, addr, nTimerCounter, nRepeateNo);
	
	bool	bSendSuccess = false;
	while(true)
	{
		// repeatly send
		update();

		//  Modify TimerRegistry singleton -> member object, for MT [2/23/2010 KimSuRyon]
		uint32 nRepeateNo = m_pTimerRegistry->getPacketRepeateNo(nTimerCounter);
		if ( nRepeateNo == -1 )
		{
			sipdebug("CUDPPeerClient::sendTo nRepeateNo=-1");
			bSendSuccess = true;
			break;
		}
		if ( nRepeateNo >= PKTimerRepeateNumber.get() )
		{
			sipdebug("CUDPPeerClient::sendTo nRepeateNo >= PKTimerRepeateNumber.get()");

			bSendSuccess = false;
			m_pTimerRegistry->unregisterTimer(nTimerCounter);

			break;
		}
	}

	sipdebug("CUDPPeerClient::sendTo leave, ret=%d", bSendSuccess);

	return	bSendSuccess;
}

void	CUDPPeerClient::sendToWithTrust(CMessage &msg, CInetAddress & addr, uint32 nTimer, uint32 nRepeat)
{
	uint32		len			= msg.length() - msg.getHeaderSize();
	MsgNameType	sName		= msg.getName();

	CMessage	msgout(sName);
	msgout.serial(nTimer);
	msgout.serial(nRepeat);
	uint8	* buffer0 = const_cast<uint8 *> (msg.buffer() + msg.getHeaderSize());
	msgout.serialBuffer(buffer0, len);
	uint8	* buffer = const_cast<uint8 *> (msgout.buffer());
	try
	{
		sipinfo("socket send start");
		m_sock->sendTo(buffer, msgout.length(), addr);
		sipinfo("Send UDP Message <%s> To Client %s", msg.toString().c_str(), addr.asString().c_str());
	}
	catch(ESocket e)
	{
		sipinfo("Failed Send UDP Message <%s> To Client %s", msg.toString().c_str(), addr.asString().c_str());
	}
}

void	CUDPPeerClient::dispatchCallback(SIPNET::CMessage &msg, SIPNET::CInetAddress addr)
{
	MsgNameType name = msg.getName ();
	std::map<MsgNameType, TUDPCallbackItem>::iterator callbackfind;
	callbackfind = _CallbackArray.find(name);

	bool	bFind = true;
	if (callbackfind == _CallbackArray.end())
		bFind = false;

	TUDPMsgClientCallback	cb = NULL;
	if (bFind)
	{
		cb = callbackfind->second.Callback;

		if ( cb != NULL )
			cb(msg, m_sock, addr, m_client);
	}
}

void	CUDPPeerClient::addCallbackArray (const TUDPCallbackItem *callbackarray, sint arraysize)
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

