/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include <queue>

#include "misc/config_file.h"

#include "net/udp_sock.h"
#include "net/udp_sim_sock.h"
#include "misc/debug.h"

using namespace std;
using namespace SIPBASE;

namespace SIPNET {

//
// Class
//

struct CBufferizedOutPacket
{
	CBufferizedOutPacket (CUdpSock *client, const uint8 *packet, uint32 packetSize, uint32 delay, const CInetAddress *addr):
		Client(client), PacketSize(packetSize), Time(CTime::getLocalTime()+delay)
	{
		sipassert (packetSize > 0);
		sipassert (packet != NULL);
		sipassert (client != NULL);

		Packet = new uint8[packetSize];
		memcpy (Packet, packet, packetSize);

		if (addr != NULL)
		{
			Addr = new CInetAddress;
			*Addr = *addr;
		}
		else
		{
			Addr = NULL;
		}
	}

	~CBufferizedOutPacket ()
	{
		sipassert (Packet != NULL);
		delete [] Packet;
		Packet = NULL;
		Client = NULL;
		PacketSize = 0;
		Time = 0;
		if (Addr != NULL)
			delete Addr;
	}

	CUdpSock		*Client;
	uint8			*Packet;
	uint32			 PacketSize;
	TTime			 Time;
	CInetAddress	*Addr;
};


//
// Variables
//

//queue<CBufferizedOutPacket*> CUdpSimSock::BufferizedOutPackets;
//queue<CBufferizedOutPacket*> CUdpSimSock::BufferizedInPackets;

uint32	CUdpSimSock::_InLag = 0;
uint8	CUdpSimSock::_InPacketLoss = 0;

uint32	CUdpSimSock::_OutLag = 0;
uint8	CUdpSimSock::_OutPacketLoss = 0;
uint8	CUdpSimSock::_OutPacketDuplication = 0;
uint8	CUdpSimSock::_OutPacketDisordering = 0;

//
// Functions
//

void CUdpSimSock::sendUDPNow (const uint8 *buffer, uint32 len, const CInetAddress *addr)
{
	try
	{
		if (addr == NULL)
			UdpSock.send (buffer, len);
		else
			UdpSock.sendTo (buffer, len, *addr);

	}
	catch (ESocket& e)
	{
		sipwarning("LNETL0: UDP send error : %s", e.what());
		return;		
	}
}

void CUdpSimSock::sendUDP (const uint8 *buffer, uint32& len, const CInetAddress *addr)
{
	sipassert (buffer != NULL);
	sipassert (len > 0);

	if ((float)rand()/(float)(RAND_MAX)*100.0f >= _OutPacketLoss)
	{
		sint32 lag = _OutLag /*+ (rand()%40) - 20*/;// void disordering
		
		if (lag > 100)
		{
			// send the packet later

			CBufferizedOutPacket *bp = new CBufferizedOutPacket (&UdpSock, buffer, len, lag, addr);

			// duplicate the packet
			if ((float)rand()/(float)(RAND_MAX)*100.0f < _OutPacketDisordering && _BufferizedOutPackets.size() > 0)
			{
				CBufferizedOutPacket *bp2 = _BufferizedOutPackets.back();

				// exange the time
				TTime t = bp->Time;
				bp->Time = bp2->Time;
				bp2->Time = t;

				// exange packet in the buffer
				_BufferizedOutPackets.back() = bp;
				bp = bp2;
			}

			_BufferizedOutPackets.push (bp);

			// duplicate the packet
			if ((float)rand()/(float)(RAND_MAX)*100.0f < _OutPacketDuplication)
			{
				CBufferizedOutPacket *bp = new CBufferizedOutPacket (&UdpSock, buffer, len, lag, addr);
				_BufferizedOutPackets.push (bp);
			}
		}
		else
		{
			// send the packet NOW

			sendUDPNow (buffer, len, addr);

			// duplicate the packet
			if ((float)rand()/(float)(RAND_MAX)*100.0f < _OutPacketDuplication)
			{
				sendUDPNow (buffer, len, addr);
			}
		}
	}
}



void CUdpSimSock::updateBufferizedPackets ()
{
	TTime ct = CTime::getLocalTime ();
	while (!_BufferizedOutPackets.empty())
	{
		CBufferizedOutPacket *bp = _BufferizedOutPackets.front ();
		if (bp->Time <= ct)
		{
			// time to send the message
			sendUDPNow (bp->Packet, bp->PacketSize, bp->Addr);
			delete bp;
			_BufferizedOutPackets.pop ();
		}
		else
		{
			break;
		}
	}
}

void				cbSimVar (CConfigFile::CVar &var)
{
	     if (var.Name == "SimInLag") CUdpSimSock::_InLag = var.asInt ();
	else if (var.Name == "SimInPacketLost") CUdpSimSock::_InPacketLoss = var.asInt ();
	else if (var.Name == "SimOutLag") CUdpSimSock::_OutLag = var.asInt ();
	else if (var.Name == "SimOutPacketLost") CUdpSimSock::_OutPacketLoss = var.asInt ();
	else if (var.Name == "SimOutPacketDuplication") CUdpSimSock::_OutPacketDuplication = var.asInt ();
	else if (var.Name == "SimOutPacketDisordering") CUdpSimSock::_OutPacketDisordering = var.asInt ();
	else sipstop;
}

void				CUdpSimSock::setSimValues (SIPBASE::CConfigFile &cf)
{
	cf.setCallback ("SimInLag", cbSimVar);
	cf.setCallback ("SimInPacketLost", cbSimVar);
	cf.setCallback ("SimOutLag", cbSimVar);
	cf.setCallback ("SimOutPacketLost", cbSimVar);
	cf.setCallback ("SimOutPacketDuplication", cbSimVar);
	cf.setCallback ("SimOutPacketDisordering", cbSimVar);
	
	CConfigFile::CVar *pv;
	pv = cf.getVarPtr("SimInLag");
	if( pv )
		cbSimVar( *pv );
	pv = cf.getVarPtr("SimInPacketLost");
	if( pv )
		cbSimVar( *pv );
	pv = cf.getVarPtr("SimOutLag");
	if( pv )
		cbSimVar( *pv );
	pv = cf.getVarPtr("SimOutPacketLost");
	if( pv )
		cbSimVar( *pv );
	pv = cf.getVarPtr("SimOutPacketDuplication");
	if( pv )
		cbSimVar( *pv );
	pv = cf.getVarPtr("SimOutPacketDisordering");
	if( pv )
		cbSimVar( *pv );
}

void				CUdpSimSock::connect( const CInetAddress& addr )
{
	UdpSock.connect (addr);
}

void				CUdpSimSock::close()
{
	UdpSock.close ();
}

uint8 buffer [10000];

bool				CUdpSimSock::dataAvailable ()
{
	updateBufferizedPackets ();

	if (_InLag > 0)
	{
		while (UdpSock.dataAvailable ())
		{
			CInetAddress addr;
			uint len = 10000;
			UdpSock.receivedFrom (buffer, len, addr);

			if ((float)rand()/(float)(RAND_MAX)*100.0f >= _InPacketLoss)
			{
				CBufferizedOutPacket *bp = new CBufferizedOutPacket (&UdpSock, buffer, len, _InLag, &addr);
				_BufferizedInPackets.push (bp);
			}
		}

		TTime ct = CTime::getLocalTime ();
		if (!_BufferizedInPackets.empty() && _BufferizedInPackets.front ()->Time <= ct)
			return true;
		else
			return false;
	}
	else
	{
		if ((float)rand()/(float)(RAND_MAX)*100.0f >= _InPacketLoss)
		{
			return UdpSock.dataAvailable ();
		}
		else
		{
			// consume data
			if (UdpSock.dataAvailable ())
			{
				CInetAddress addr;
				uint len = 10000;
				UdpSock.receivedFrom (buffer, len, addr);
			}
			
			// packet lost
			return false;
		}
	}
}

bool				CUdpSimSock::receive (uint8 *buffer, uint32& len, bool throw_exception)
{
	if (_InLag> 0)
	{
		if (_BufferizedInPackets.empty())
		{
			if (throw_exception)
				throw Exception ("no data available");
			return false;
		}
		
		CBufferizedOutPacket *bp = _BufferizedInPackets.front ();
		uint32 s = min (len, bp->PacketSize);
		memcpy (buffer, bp->Packet, s);
		len = s;

		delete bp;
		_BufferizedInPackets.pop ();
		return true;
	}
	else
	{
		return UdpSock.receive(buffer, len, throw_exception);
	}
}

CSock::TSockResult	CUdpSimSock::send (const uint8 *buffer, uint32& len, bool throw_exception)
{
	sendUDP (buffer, len);
	return CSock::Ok;
}
	
void CUdpSimSock::sendTo (const uint8 *buffer, uint32& len, const CInetAddress& addr)
{
	sendUDP (buffer, len, &addr);
}

bool				CUdpSimSock::connected()
{
	return UdpSock.connected ();
}

} // SIPNET
