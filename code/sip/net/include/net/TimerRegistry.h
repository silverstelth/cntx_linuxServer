/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TIMERREGISTRY_H__
#define __TIMERREGISTRY_H__

#include "net/inet_address.h"
#include "net/message.h"
namespace	SIPNET
{
class	CUDPPeerClient;
// This function is used in side of Client, not Server
struct CPacketTimerInfo
{
public:
	CPacketTimerInfo( uint32 pkNo, CMessage message, SIPNET::CInetAddress addr)
	{
		_pkNo				= pkNo;			
		_pksubNo			= 0;		
		_pkName				= message.getName();		
		_sendTime			= SIPBASE::CTime::getLocalTime();		
		_message			= message;
		_addr				= addr;
	}

public:
	uint32			_pkNo;				// a unique number of send packet
	uint32			_pksubNo;			// a number send repeately
	MsgNameType		_pkName;			// a name of send packet
	SIPBASE::TTime	_sendTime;			// a time of sent : milisecond
	SIPNET::CMessage		_message;	// message
	SIPNET::CInetAddress	_addr;		// address
};

class CTimerRegistry
{
/*
// Singleton is not proper to MT [2/23/2010 KimSuRyon]
SIPBASE_SAFE_SINGLETON_DECL(CTimerRegistry)
*/

protected:
	typedef		std::map<uint32, CPacketTimerInfo *>			TARYTIMER;
	typedef		std::map<uint32, CPacketTimerInfo *>::iterator	TARYTIMERIT;
	TARYTIMER	_aryTimers;
	
	uint32		_uTimerCounter;

public:
	void	registerTimer(SIPNET::CMessage & message, SIPNET::CInetAddress & addr);
	void	unregisterTimer(uint32 pkNo);

	bool	update(CUDPPeerClient * client);

	CPacketTimerInfo	*getPacketTimer(uint32 nTimerNo);
	uint32	getPacketRepeateNo(uint32 nTimerNo);
	uint32	getTimerCounter() { return _uTimerCounter; };

	bool isWaiting(uint32 nTimerNo, uint32 nSubPKNo);

public:
	CTimerRegistry(void);
	~CTimerRegistry(void);
};
}

extern SIPBASE::CVariable<uint32>	PKTimerWaitingGap;
extern SIPBASE::CVariable<uint32>	PKTimerRepeateNumber;

#endif // end of guard __TIMERREGISTRY_H__
