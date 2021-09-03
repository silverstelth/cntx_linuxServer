/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CALLBACK_UDP_H__
#define __CALLBACK_UDP_H__

#include "misc/types_sip.h"
#include "net/message.h"
#include "net/udp_sock.h"

namespace SIPNET 
{
class CCallbackUdp;
typedef void (*TUDPMsgCallback) (CMessage &msgin, CCallbackUdp &local, CInetAddress &remote, void* pArg);
typedef void (*TUDPPacketCallback)(char* pData, int nLength, CInetAddress &remote, void* param);
//typedef struct
struct TUDPMsgCallbackItem
{
public:
	MsgNameType		name;
	TUDPMsgCallback	Callback;
	void*			pArg;
};

#define		MAX_UDPNORESPONSETIME	20000	// max time UDP response
class CCallbackUdp : public CUdpSock
{
public:
	CCallbackUdp(bool logging);

	void	update();
	void	addCallbackArray(const TUDPMsgCallbackItem *callbackarray, sint arraysize, void* pArg);
	void	setDefaultCallback(TUDPPacketCallback cb, void* param) { _DefaultPacketCallback = cb; _DefaultPacketParam = param; }
	void	setKeepAlive(bool bKeep, CInetAddress addr)
	{
		_bKeepAlive = bKeep;
		_KeepAddr	= addr;
	}
private:

	std::map<MsgNameType, TUDPMsgCallbackItem>	_CallbackArray;
	TUDPPacketCallback						_DefaultPacketCallback;
	void*									_DefaultPacketParam;

	bool									_bKeepAlive;
	CInetAddress							_KeepAddr;
	SIPBASE::TTime							_tmLastKeepAlive;
};

} // SIPNET

#endif // end of guard __CALLBACK_UDP_H__
