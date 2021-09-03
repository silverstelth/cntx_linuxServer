/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UDPPEERCLIENT_H__
#define __UDPPEERCLIENT_H__

#include "net/udp_sock.h"
#include "net/message.h"

namespace	SIPNET
{
typedef void (*TUDPMsgClientCallback) ( CMessage& msgin, CUdpSock	*from, CInetAddress& peeraddr, void * arg );
typedef struct
{
	/// Key C string. It is a message type name, or "C" for connection or "D" for disconnection
#ifdef SIP_MSG_NAME_DWORD
	uint32			Key;
#else
	char			*Key;
#endif // SIP_MSG_NAME_DWORD
	/// The callback function
	TUDPMsgClientCallback	Callback;
} TUDPCallbackItem;

class	CCallbackClient;
class	CTimerRegistry;
class CUDPPeerClient
{
public:
	void	init(CCallbackClient * client, uint16 port);
	void	init();
	bool	update();
	void	release();

	void	connectPeer(CInetAddress& peeraddr);

	void	setMsgCallback(TUDPMsgClientCallback cb)	{ m_cb = cb; }
	bool	sendTo(CMessage	&msg, CInetAddress & addr);
	void	sendToWithTrust(CMessage	&msg, CInetAddress & addr, uint32 nTimer, uint32 nRepeat);
	CUdpSock	*UdpSock() { return m_sock.getPtr(); };
	bool	IsInited() { return m_sock != NULL; };

	void	dispatchCallback(CMessage &msg, SIPNET::CInetAddress addr);
	void	addCallbackArray (const TUDPCallbackItem *callbackarray, sint arraysize);

	bool	UupdateTimerRegistry(CMessage & message);

protected:
	SIPBASE::CSmartPtr<CUdpSock>	m_sock;
	TUDPMsgClientCallback	m_cb;
	CCallbackClient		*	m_client;
	CTimerRegistry		*	m_pTimerRegistry;

	std::map<MsgNameType, TUDPCallbackItem>	_CallbackArray;

public:
	CUDPPeerClient();
	CUDPPeerClient(CCallbackClient * client);
	~CUDPPeerClient(void);
};
}

#endif //__UDPPEERCLIENT_H__
