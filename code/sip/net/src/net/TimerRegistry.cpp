/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "misc/variable.h"
#include "net/TimerRegistry.h"
#include "net/UDPPeerClient.h"

SIPBASE::CVariable<uint32>	PKTimerWaitingGap("net", "PKTimerWaitingGap", "Waiting Gap of Packet Timer(millisecond)", 1000, 0, true);
SIPBASE::CVariable<uint32>	PKTimerRepeateNumber("net", "PKTimerRepeateNumber", "Repeating Number of Packet Timer", 5, 0, true);

namespace	SIPNET
{
// Singleton is not proper to MT [2/23/2010 KimSuRyon]
/*
SIPBASE_SAFE_SINGLETON_IMPL(CTimerRegistry)
*/

CTimerRegistry::CTimerRegistry(void)
{
	_uTimerCounter = 0;
}

CTimerRegistry::~CTimerRegistry(void)
{
}

CPacketTimerInfo *CTimerRegistry::getPacketTimer(uint32 nTimerNo)
{
	TARYTIMERIT it = _aryTimers.find(nTimerNo);
	if ( it == _aryTimers.end() )
		return NULL;

	CPacketTimerInfo * pTimer = (*it).second;

	return pTimer;
}

uint32 CTimerRegistry::getPacketRepeateNo(uint32 nTimerNo)
{
	TARYTIMERIT it = _aryTimers.find(nTimerNo);
	if ( it == _aryTimers.end() )
		return -1;

	CPacketTimerInfo * pTimer = (*it).second;
	return	pTimer->_pksubNo;
}

void	CTimerRegistry::registerTimer(SIPNET::CMessage & message, CInetAddress &addr)
{
	CPacketTimerInfo	*pTimer = new CPacketTimerInfo(_uTimerCounter, message, addr);
	_aryTimers.insert(std::make_pair(_uTimerCounter, pTimer));

	_uTimerCounter ++;
}

void	CTimerRegistry::unregisterTimer(uint32 pkNo)
{
    TARYTIMERIT it = _aryTimers.find(pkNo);
	if ( it != _aryTimers.end() )
	{
		CPacketTimerInfo * pTimer = (*it).second;
		_aryTimers.erase(it);

        delete pTimer;
	}
}

bool CTimerRegistry::update(CUDPPeerClient * client)
{
	SIPBASE::TTime curTime = SIPBASE::CTime::getLocalTime();

	//TARYTIMER	removeAry;
	TARYTIMERIT it;
	for ( it = _aryTimers.begin(); it != _aryTimers.end(); it ++ )
	{
		CPacketTimerInfo * pTimer = (*it).second;
		if ( curTime - pTimer->_sendTime > PKTimerWaitingGap.get() )
		{
			if (pTimer->_pksubNo >= PKTimerRepeateNumber.get())
			{
				//removeAry.insert(*it);
				continue;
			}

			pTimer->_pksubNo ++;
			pTimer->_sendTime = SIPBASE::CTime::getLocalTime();
			client->sendToWithTrust(pTimer->_message, pTimer->_addr, pTimer->_pkNo, pTimer->_pksubNo);
		}
	}

	/*
	for ( it = removeAry.begin(); it != removeAry.end(); it ++ )
	{
		CPacketTimerInfo * pTimer = (*it).second;
		unregisterTimer(pTimer->_pkNo);
	}
	*/

	return true;
}

bool CTimerRegistry::isWaiting(uint32 nTimerNo, uint32 nSubPKNo)
{
	TARYTIMERIT it = _aryTimers.find(nTimerNo);
	if ( it == _aryTimers.end() )
		return false;

	CPacketTimerInfo * pTimer = (*it).second;
	uint32	nSubNo = pTimer->_pksubNo;

	return true;
}
}
