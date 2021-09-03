/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SECURITYCLIENT_H__
#define __SECURITYCLIENT_H__

#include "net/callback_client.h"
#include "net/cookie.h"
#include "net/UDPPeerClient.h"

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


class	CSecurityClient;

// This task is only for looking update waiting infos softly in client
class CWaitConnectingTask :
	public SIPBASE::IRunnable
{
public:
	virtual void	run();		// with his own thread
	void			init(SIPNET::CSecurityClient	*pClient);
	void			update();	// without his own thread
	void			close();

	void			requirExit() { m_bExitRequired = true; };
	bool			exitRequired() { return m_bExitRequired; };

	uint32			ThreadID(){ return m_threadID;};

	CWaitConnectingTask(void);
	~CWaitConnectingTask(void);

protected:
	bool			m_bExitRequired;
	uint32			m_threadID;
	SIPNET::CSecurityClient		*m_pClient;
};

enum CONNECT_REPLY
{
	CON_WAIT,
	CON_CONNECT,
	CON_REFUSE,
	CON_FAIL,
	CON_SUCCESS
};

class CCookie;
class CInetAddress;
class CUDPPeerClient;
class CSecurityClient :
	public SIPNET::CCallbackClient
{
public:
	void	connect( const CInetAddress& addr, uint32 usec = 0 );

	void	send (const SIPNET::CMessage &buffer, SIPNET::TTcpSockId hostid = SIPNET::InvalidSockId, uint8 bySendType = REL_GOOD_ORDER);
	void	sendTo (const SIPNET::CMessage &buffer, const CInetAddress& addrUdp);
	void	receive (SIPNET::CMessage &buffer, SIPNET::TSockId *hostid = NULL);

public:
	CSecurityClient( TCRYPTMODE cryptmode=NON_CRYPT, bool bHasUdp=false, bool bConnectDelay=false, bool	bNoRecieve=false, TRecordingState rec=Off, const std::string& recfilename="", bool recordall=true, bool initPipeForDataAvailable=true );
	virtual ~CSecurityClient(void);

	void	setSecurityConnectionCallback( SIPNET::TNetCallback cb, void* arg ) { _SecurityConnectionCallback = cb; _SecurityConnectionCbArg = arg; };

/*****************************************************************/
// Pre Connection Handshake
/*****************************************************************/
public:
	int		CanConnect( const CInetAddress& addr );
	bool	CancelWaitingConnect( const CInetAddress& addr );
	bool	IsConnectDelayMode() { return _bConnectDelay; };
	bool	IsWaiting() { return _bWaiting; };
	void	setWaiting() { _bWaiting = true; };

	// GMClient
	int		GMCanConnect( const CInetAddress& addr );
	
	bool	SelfUpdate() { return _bSelfUpdate; };
	void	setSelfUpdate() { _bSelfUpdate = true; };
	
	CUDPPeerClient	*UDPClient() { return _UDPClient; };

	void	setNotifyWaitingCallback(TUDPMsgClientCallback cb, void *arg = NULL) { notifyWaitingCallback = cb; _notifyWaitingCbArg = arg; };
	void	setNotifyConnectCallback(TUDPMsgClientCallback cb, void *arg = NULL) { notifyConnectCallback = cb; _notifyConnectCbArg = arg; };
	void	addSecurityCallbackArray(const TDefSecurityCallbackItem *callbackarray, sint arraysize);

	CCookie &		getConnectionCookie() { return _conCookie; };
	void	setConnectionCookie(CCookie & cookie) { _conCookie = cookie; };
	CInetAddress&	getConnectionAddress() { return _conAddr; };
	void	setConnectionAddress(CInetAddress& addr) { _conAddr = addr; };

	void	setTimeout(sint32	time) { _Timeout = time; };
	sint32	getTimeout() { return _Timeout; };

public:
	TUDPMsgClientCallback	notifyWaitingCallback;
	void					*_notifyWaitingCbArg;

	TUDPMsgClientCallback	notifyConnectCallback;
	void					*_notifyConnectCbArg;

protected:
	bool				_bConnectDelay;
	bool				_bWaiting;
	bool				_bSelfUpdate;
	CUDPPeerClient		*_UDPClient;
	SIPBASE::IThread	*_WaitingThread;

	typedef	std::map<MsgNameType, TDefSecurityCallbackItem>	TSECURITYCB;
	typedef TSECURITYCB::iterator	TSECURITYCBIT;
	TSECURITYCB			_SecurityCallbackArray;

	sint32				_Timeout;

/*****************************************************************/
// Post Connection Handshake
/*****************************************************************/
public:
	void	setDefCryptMode(TCRYPTMODE mode);
	void	cleanKey();

	SIPNET::TNetCallback	_SecurityConnectionCallback;
	void					*_SecurityConnectionCbArg;

protected:
	TCRYPTMODE				_DefCryptSendMode; // When Send (not Recieve), Default Crypt Mode

	CCookie					_conCookie;
	CInetAddress			_conAddr;
};

} // namespace SIPNET

#endif // __SECURITYCLIENT_H__
