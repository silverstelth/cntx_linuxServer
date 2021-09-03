/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/callback_udp.h"

using namespace std;
using namespace SIPBASE;
namespace SIPNET 
{
static	void	cbUDPMessage(CMessage& msgin, CCallbackUdp& localudp, CInetAddress& remoteaddr, void* pArg)
{
}

static TUDPMsgCallbackItem	UDPCallbackAry[] =
{
	{ M_UDP_KEEPALIVEPUBLIC,	cbUDPMessage, NULL },
};

CCallbackUdp::CCallbackUdp(bool logging) : 
	CUdpSock(logging), _DefaultPacketCallback(NULL), _DefaultPacketParam(NULL) 
{
	_bKeepAlive			= false;
	_tmLastKeepAlive	= 0;
	
	addCallbackArray(UDPCallbackAry, sizeof(UDPCallbackAry)/sizeof(UDPCallbackAry[0]), this);
}

void	CCallbackUdp::update()
{
	try
	{
		if ( dataAvailable() )
		{
			uint8	buffer[10240];
			uint	len = sizeof(buffer);
			CInetAddress	remoteaddr;
			if ( receivedFrom(buffer, len, remoteaddr, false) )
			{
				MsgNameType msgName = CMessage::GetMsgNameFromBuffer(buffer, len);

				map<MsgNameType, TUDPMsgCallbackItem>::iterator callbackfind;
				callbackfind = _CallbackArray.find(msgName);

				if (callbackfind != _CallbackArray.end())
				{
					CMessage msgin(M_SYS_EMPTY);
					msgin.serialBuffer(buffer, len);
					msgin.invert();
					TUDPMsgCallback	cb = callbackfind->second.Callback;
					void* pArg = callbackfind->second.pArg;
					cb(msgin, *this, remoteaddr, pArg);
				}
				else
				{
					if (_DefaultPacketCallback)
					{
						_DefaultPacketCallback((char*)buffer, len, remoteaddr, _DefaultPacketParam);
					}
				}
			}
		}

		// keep alive
		if (_bKeepAlive)
		{
			TTime	currentTime = CTime::getLocalTime();
			if (currentTime - _tmLastKeepAlive > 2000)
			{
				CMessage	msg(M_UDP_KEEPALIVEPUBLIC);
				msg.serial(currentTime);
				sendTo(msg.buffer(), msg.length(), _KeepAddr);

				_tmLastKeepAlive = currentTime;
			}
		}
	}
	catch (...)
	{
		
	}
}

void	CCallbackUdp::addCallbackArray(const TUDPMsgCallbackItem *callbackarray, sint arraysize, void* pArg)
{
	for (sint i = 0; i < arraysize; i++)
	{
		TUDPMsgCallbackItem	ci = callbackarray[i];
		ci.pArg = pArg;
		MsgNameType	nameMes = ci.name;
		_CallbackArray.insert( make_pair(nameMes, ci) );
	}
}

} // SIPNET
