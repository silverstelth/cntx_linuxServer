/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UDP_SIM_SOCK_H__
#define __UDP_SIM_SOCK_H__

#include "misc/config_file.h"
#include <queue>

#include "sock.h"
#include "udp_sock.h"


namespace SIPNET {


struct CBufferizedOutPacket;

/**
 * CUdpSimSock: Unreliable datagram socket via UDP but packet lost, lag simulation.
 * See class CUdpSock.
 *
 * Notes: InLag must be >0 to use the InPacketLoss variable
 *
 * \date 2002
 */
class CUdpSimSock
{
public:
	
	CUdpSimSock (bool logging = true) : UdpSock(logging) { }

	// this function is to call to set the simulation values
	static void			setSimValues (SIPBASE::CConfigFile &cf);

	// CUdpSock functions wrapping
	void				connect( const CInetAddress& addr );
	void				close();
	bool				dataAvailable();
	bool				receive( uint8 *buffer, uint32& len, bool throw_exception=true );
	CSock::TSockResult	send( const uint8 *buffer, uint32& len, bool throw_exception=true );
	void				sendTo (const uint8 *buffer, uint32& len, const CInetAddress& addr);
	bool				connected();
	const CInetAddress&	localAddr() const {	return UdpSock.localAddr(); }
	void	setNonBlocking() { UdpSock.setNonBlockingMode( true ); }
	void				bind( uint16 port ) { UdpSock.bind(port); }

	// Used to call CUdpSock functions that are not wrapped in this class
	CUdpSock			UdpSock;
	CUdpSock*			getUdpSock()	{ return &UdpSock; }

private:

	std::queue<CBufferizedOutPacket*> _BufferizedOutPackets;
	std::queue<CBufferizedOutPacket*> _BufferizedInPackets;

	static uint32	_InLag;
	static uint8	_InPacketLoss;

	static uint32	_OutLag;
	static uint8	_OutPacketLoss;
	static uint8	_OutPacketDuplication;
	static uint8	_OutPacketDisordering;

	void updateBufferizedPackets ();
	void sendUDP (const uint8 *buffer, uint32& len, const CInetAddress *addr = NULL);
	void sendUDPNow (const uint8 *buffer, uint32 len, const CInetAddress *addr = NULL);

	friend void cbSimVar (SIPBASE::CConfigFile::CVar &var);
};


} // SIPNET


#endif // SIP_SIM_SOCK_H

/* End of udp_sim_sock.h */
