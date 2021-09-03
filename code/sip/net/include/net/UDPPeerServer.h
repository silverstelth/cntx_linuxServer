/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __UDPPEERSERVER_H__
#define __UDPPEERSERVER_H__

#include "net/udp_sock.h"
#include "net/message.h"

namespace	SIPNET
{
class CUDPPeerServer;
typedef void (*TUDPMsgServerCallback) (SIPNET::CMessage &msgin, SIPNET::CInetAddress& peeraddr,	SIPNET::CUDPPeerServer *peer);
typedef struct
{
	/// Key C string. It is a message type name, or "C" for connection or "D" for disconnection
#ifdef SIP_MSG_NAME_DWORD
	uint32			Key;
#else
	char			*Key;
#endif // SIP_MSG_NAME_DWORD
	/// The callback function
	TUDPMsgServerCallback	Callback;
} TUDPCallbackItem;

class CUDPPeerServer
{
public:
	void	init(uint16 port);
	bool	update();
	void	release();
	void	setMsgCallback(TUDPMsgServerCallback cb)	{ m_cb = cb; }
	void	sendTo(CMessage	&msg, CInetAddress & addr); // KSR

	void	dispatchCallback(CMessage &msg, CInetAddress addr);

	CUdpSock	*UdpSock() { return m_sock.getPtr(); };
	bool	IsInited() { return m_sock != NULL; };

	void	addCallbackArray (const TUDPCallbackItem *callbackarray, sint arraysize);

protected:
	SIPBASE::CSmartPtr<CUdpSock>	m_sock;
	TUDPMsgServerCallback	m_cb;

	uint32					_pkNo;
	uint32					_pkRepeatNo;

	std::map<MsgNameType, TUDPCallbackItem>	_CallbackArray;

public:
	CUDPPeerServer(void);
	~CUDPPeerServer(void);
};
}

#endif //__UDPPEERSERVER_H__
