/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SECURITYSERVER_H__
#define __SECURITYSERVER_H__

#include "net/callback_server.h"
#include "net/IProtocolServer.h"

namespace SIPNET
{
typedef struct
{
#ifdef SIP_MSG_NAME_DWORD
	uint32		Key;
#else
	char		*Key;
#endif // SIP_MSG_NAME_DWORD
    TCRYPTMODE	mode;
} TDefSecurityCallbackItem;

class CSecurityServer :
	public SIPNET::CCallbackServer, public IProtocolServer
{
public:
	void	init( uint16 tcpPort );
	void	update( sint32 timeout = 0 );
	void	release();

	void	send (const SIPNET::CMessage &buffer, SIPNET::TSockId hostid, uint8 bySendType = REL_GOOD_ORDER);
	void	receive (SIPNET::CMessage &buffer, SIPNET::TSockId *hostid);

public:
	CSecurityServer(TCRYPTMODE cryptmode=NON_CRYPT, bool bUdpServer= false, bool bClientServer = true, uint32 max_frontnum=1000, TRecordingState rec=Off, const std::string& recfilename="", bool recordall=true, bool initPipeForDataAvailable=true );
	virtual ~CSecurityServer(void);

public:
	void	addSecurityCallbackArray (const TDefSecurityCallbackItem *callbackarray, sint arraysize);
    
protected:
	typedef	std::map<MsgNameType, TDefSecurityCallbackItem>	TSECURITYCB;
	typedef TSECURITYCB::iterator	TSECURITYCBIT;
	TSECURITYCB			_SecurityCallbackArray;

/*****************************************************************/
// Post Connection Handshake
/*****************************************************************/
public:
	void	setDefCryptMode(TCRYPTMODE mode);
	void	setSecurityConnectionCallback( SIPNET::TNetCallback cb, void* arg ) { _SecurityConnectionCallback = cb; _SecurityConnectionCbArg = arg; };
	static void	cbClientConnection( SIPNET::TSockId from, void *arg );

public:
	SIPNET::TNetCallback	_SecurityConnectionCallback;
	void					*_SecurityConnectionCbArg;

protected:
	TCRYPTMODE				_DefCryptSendMode; // When Send (not Recieve), Default Crypt Mode
};

} // namespace SIPNET

#endif // __SECURITYSERVER_H__
