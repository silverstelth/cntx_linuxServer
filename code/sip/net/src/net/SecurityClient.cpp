/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "misc/hierarchical_timer.h"
#include "misc/stop_watch.h"
#include "net/CryptEngine.h"
#include "net/UDPPeerClient.h"
#include "net/cookie.h"
#include "net/SecurityClient.h"
#include "net/Key.h"


using namespace	std;
using namespace	SIPBASE;
using namespace	SIPNET;


// PreConnection
SIPBASE::CVariable<unsigned int>	BaseUDPPort("net",	"BaseUDPPort",	"BaseUDPPort", 60000, 0, true);

#ifdef SIP_OS_WINDOWS
	__declspec(thread)	bool	g_bAllowConnect		= false;
	__declspec(thread)	bool	g_bWaitConnect		= false;
	__declspec(thread)	bool	g_bRefuseConnect	= false;
#else
	bool	g_bAllowConnect		= false;
	bool	g_bWaitConnect		= false;
	bool	g_bRefuseConnect	= false;
#endif

// In Waiting State, <_Client> is used in Both Waiting Thread and Main Thread that called CanConnect()
//__declspec(thread)	CSecurityClient * _Client = NULL;

void	cbClientDelayConnect( CMessage& msgin, CUdpSock	*from, CInetAddress& peeraddr, void * arg );
void	cbClientDelayWait( CMessage& msgin, CUdpSock	*from, CInetAddress& peeraddr, void * arg );
void	cbClientDelayRefuse( CMessage& msgin, CUdpSock	*from, CInetAddress& peeraddr, void * arg );
TUDPCallbackItem	g_udpCallbackAry[] =
{
	{ M_CONNECT,				cbClientDelayConnect },
	{ M_WAIT,					cbClientDelayWait },
	{ M_SECURITYCLIENT_REFUSE,	cbClientDelayRefuse },
};

// PostConnection
bool	g_bConnect = false;

void	cbServerHello( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
void	cbServerCertificate( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
void	cbServerKeyExchange( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
void	cbHandShakeOK( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
void	cbHandShakeFail( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);

TCallbackItem	g_postConnectionClientCBAry[] = 
{
	{ M_SERVERHELLO,	cbServerHello },
	{ M_CERTIFICATE,	cbServerCertificate },
	{ M_KEYEXCHANGE,	cbServerKeyExchange },
	{ M_HANDSHKOK,	cbHandShakeOK },
	{ M_HANDSHKFAIL,	cbHandShakeFail }
};

G_WATCH_DECL(SecureConnect);


/************************************************************************************/
// PostConnection Handshake
/************************************************************************************/

// packet : SERVERHELLO
// Server -> Client
void cbServerHello( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	AUTO_WATCH(ServerHello)
    H_AUTO(SERVERHELLO);

//	UseSecurity.set(true);

	CSecurityClient * client = dynamic_cast<CSecurityClient *>(&clientcb);

	TCRYPT_INFO		info;
	CSecurityInfo & security = CSecurityInfo::getInstance();

	// Case Client, Set Information from Message from Server
	msgin.serial(info);	security.setSymInfo(info);
	msgin.serial(info);	security.setAsymInfo(info);
	msgin.serial(info);	security.setKeyGenerateInfo(info);
	msgin.serial(info);	security.setWrapInfo(info);
	uint8	nDefMode;
	msgin.serial(nDefMode);	security.setDefMode((TCRYPTMODE) nDefMode);
	security.setInited();

	sipinfo("<-SERVER HELLO> msg came in");

	CMessage msgout(M_CLIENTHELLO, NON_CRYPT);
	uint8	init = 1;

	try
	{
		CCryptSystem::init();
	}
	catch (ESecurity& e)
	{
		init = 0;
		msgout.serial(init);
		client->send(msgout);
		siperror("CryptSystem Init Failed : reason %s", e.what());
	}
	msgout.serial(init);

	client->send(msgout);
}

// packet : CERTIFICATE
// Server -> Client
void cbServerCertificate( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	AUTO_WATCH(ServerCertificate);
    H_AUTO(SERVERCERTIFICATE);

	CSecurityClient * client = dynamic_cast<CSecurityClient *>(&clientcb);
	CSecretKey	keyRecieve, keySend;
	CMessage	msgout(M_CERTIFICATE, NON_CRYPT);

	try
	{
		sipinfo("<-CERTIFICATE> msg came in");
		msgin.serial(keyRecieve);
		{
			AUTO_WATCH(ClientGenPairKey);
            keySend = CKeyManager::getInstance().generatePairKey(keyRecieve);
		}
	}
	catch (ESecurity& e)
	{
		CMessage msgout(M_HANDSHKFAIL, NON_CRYPT);
		sipwarning("Certificate Failed, reason : %s", e.what());
		client->send(msgout);
	}
	msgout.serial(keySend);
	msgout.serial(keyRecieve);

	client->send(msgout);
}

// packet : KEYEXCHANGE
// Server -> Client
void cbServerKeyExchange( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	AUTO_WATCH(KeyExchange);
    H_AUTO(SERVERHELLO);

	CSecurityClient * client = dynamic_cast<CSecurityClient *>(&clientcb);

	try
	{
		CSecretKey	cookie;
		msgin.serial(cookie);

		CAsymKey	asymKey;
		uint32	size;						msgin.serial(size);
		uint8	keybuff[MAX_KEY_LENGTH];	msgin.serialBuffer(keybuff, size);

		asymKey.setPubKey(keybuff, size);
		CKeyManager::getInstance().registerKey(asymKey.getKey(), from);
		CCryptSystem::resetAsymContext(from);
		sipdebug("Recive Public Key!");

		CMessage	msgout(M_KEYEXCHANGE, NON_CRYPT);
		msgout.serial(cookie);
		CSecretKey	secretKey;
		{
			AUTO_WATCH(GenSecretKey);
			secretKey = CKeyManager::getInstance().generateSymKey(from, true);
		}
		size = secretKey.getSecretKeySize();
		msgout.serial(size);
		msgout.serialBuffer(secretKey.getSecretKey(), size);
		CKeyManager::getInstance().registerKey(secretKey.getKey(), from);

		client->send(msgout);
	}
	catch (ESecurity & e)
	{
		CMessage msgout(M_HANDSHKFAIL, NON_CRYPT);
		sipwarning("Key exchange error! reason : %s", e.what());
		client->send(msgout);
	}
}

// packet : HANDSHAKEOK
// Server -> Client
void cbHandShakeOK( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	CSecurityClient * client = dynamic_cast<CSecurityClient *>(&clientcb);

	g_bConnect = true;
	G_WATCH_END(SecureConnect);

	if ( client->_SecurityConnectionCallback )
		client->_SecurityConnectionCallback(from, client->_SecurityConnectionCbArg);
}

// packet : HANDSHAKEFAIL
// Server -> Client
void cbHandShakeFail( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	g_bConnect = true;
	G_WATCH_END(SecureConnect);
}

/************************************************************************************/
// PreConnection Handshake
/************************************************************************************/
uint32	g_nbWaitingUsers = 0;
uint32	g_timeCycle = 0;

// Packet : M_CONNECT
// Client -> Server
void	cbClientDelayConnect( CMessage& msgin, CUdpSock	*from, CInetAddress& peeraddr, void * arg )
{
	CSecurityClient * _Client = (CSecurityClient *) (arg);

	CCookie			cookie;		msgin.serial(cookie);
	CInetAddress	addr;		msgin.serial(addr);

	sipinfo("DELAYCONNECT : recieved <CONNECT> to client, cookie : %s, listenAddress : %s", cookie.toString().c_str(), addr.asIPString().c_str());

	if ( g_nbWaitingUsers > 1 ) // If left connecting users, waiting internally
	{
		CMessage	msgout(M_WAIT);
		g_nbWaitingUsers = 1;	msgout.serial(g_nbWaitingUsers);
		g_timeCycle = 0;		msgout.serial(g_timeCycle);
		_Client->notifyWaitingCallback( msgout, from, peeraddr, _Client );
	}
	g_bAllowConnect = true;
//	if ( _Client->IsWaiting() ) // If this "CONNECT" is after "WAIT", that is not first "CONNECT"
	{
		CMessage	msgout(M_CONNECT);
		msgout.serial(cookie);
		msgout.serial(addr);
		msgout.invert();
		_Client->notifyConnectCallback(msgout, from, peeraddr, _Client);

		if ( _Client->IsWaiting() )
			_Client->CancelWaitingConnect(peeraddr);
	}
}

// Packet : M_WAIT
// Client -> Server
void	cbClientDelayWait( CMessage& msgin, CUdpSock	*from, CInetAddress& peeraddr, void * arg )
{
	CSecurityClient * _Client = (CSecurityClient *) (arg);

	msgin.serial(g_nbWaitingUsers);
	msgin.serial(g_timeCycle);
	_Client->setWaiting();
	if ( _Client->notifyWaitingCallback )
		_Client->notifyWaitingCallback( msgin, from, peeraddr, arg );
	g_bWaitConnect = true;
}

// Packet : M_REFUSE
// Client -> Server
void	cbClientDelayRefuse( CMessage& msgin, CUdpSock	*from, CInetAddress& peeraddr, void * arg )
{
	CSecurityClient * _Client = (CSecurityClient *) (arg);

	g_bRefuseConnect = true;
}



static void		UpdateNotifyWaitingConnecions(CSecurityClient * client)
{
	static TTime t0			= CTime::getLocalTime();
	static uint32 nbOldConnectUser	= 0;
	uint32 nbConnectUser	= 0;
	uint32 nbWaitUser		= 0;
	uint32 timeInterval		= 0;

	if ( CTime::getLocalTime() - t0 < g_timeCycle )
		return;

	if ( g_nbWaitingUsers <= 1 )
		return;
	
	g_nbWaitingUsers --;

	CMessage	msgout(M_WAIT);
	msgout.serial(g_nbWaitingUsers);
	msgout.serial(g_timeCycle);

	CUdpSock	*from		= client->UDPClient()->UdpSock();
	CInetAddress &peeraddr	= const_cast<CInetAddress &> (client->UDPClient()->UdpSock()->localAddr());
	if ( client->notifyWaitingCallback )
		client->notifyWaitingCallback( msgout, from, peeraddr, client );

	t0 = CTime::getLocalTime();
}

namespace SIPNET
{

// CWaitConnectingTask
CWaitConnectingTask::CWaitConnectingTask(void)
{
	m_bExitRequired = false;
	m_threadID = 0;
}

CWaitConnectingTask::~CWaitConnectingTask(void)
{
}

void CWaitConnectingTask::init(SIPNET::CSecurityClient	*pClient)
{
	m_pClient	= pClient;
//	_Client		= pClient;
}

void CWaitConnectingTask::update()
{
}

void CWaitConnectingTask::run()
{
#ifdef SIP_OS_WINDOWS
	m_threadID = ::GetCurrentThreadId();
#elif defined SIP_OS_UNIX
	sipwarning("not tested : CWaitConnectingTask::run()");
	pthread_self();
//	getpid();	// on linux kernel 2.6
#endif // SIP_OS_WINDOWS

	H_AUTO(LWaitSSConOK);
	TTime t0 = CTime::getLocalTime();

	g_bAllowConnect = false;
	g_bRefuseConnect = false;

	sint32 timeout = m_pClient->getTimeout();
	// Recieve and Really Send
	while ( !exitRequired() )
	{
		if ( m_pClient->NoRecieve() )
		{
			m_pClient->UDPClient()->update();
			UpdateNotifyWaitingConnecions(m_pClient);
		}
		if ( g_bRefuseConnect || g_bAllowConnect )
		{
			requirExit();
			break;
		}
		if (( timeout != TIME_INFINITE ) && ( CTime::getLocalTime() - t0 > timeout ))
		{
            throw	ETimeout("SecurityConnection Timeout");
			break;
		}

		sipSleep(SleepTime.get());
	}
}

void CWaitConnectingTask::close()
{
}

// CSecurityClient
CSecurityClient::CSecurityClient( TCRYPTMODE cryptmode, bool bHasUdp, bool bConnectDelay, bool	bNoRecieve, CCallbackNetBase::TRecordingState rec, const std::string& recfilename, bool recordall, bool initPipeForDataAvailable):
	CCallbackClient(bHasUdp, bNoRecieve, rec, recfilename, recordall, initPipeForDataAvailable),
	_UDPClient(NULL)

{
	_bSelfUpdate = false;
	_bConnectDelay = bConnectDelay;
	if ( bConnectDelay )
	{
		_bWaiting = false;
        _UDPClient = new CUDPPeerClient(this);
		_UDPClient->addCallbackArray(g_udpCallbackAry, sizeof(g_udpCallbackAry)/sizeof(g_udpCallbackAry[0]));
	}

	_SecurityConnectionCallback = NULL;
	_SecurityConnectionCbArg	= NULL;

	CCallbackClient::addCallbackArray(g_postConnectionClientCBAry, sizeof(g_postConnectionClientCBAry)/sizeof(g_postConnectionClientCBAry[0]));
	_DefCryptSendMode = cryptmode;
	_WaitingThread = NULL;
}

void	CSecurityClient::setDefCryptMode(TCRYPTMODE mode)
{
	_DefCryptSendMode = mode;
}

CSecurityClient::~CSecurityClient(void)
{
	if ( _UDPClient )
	{
		_UDPClient->release();
		delete	_UDPClient;
	}
}

bool	CSecurityClient::CancelWaitingConnect( const CInetAddress& addr )
{
	CWaitConnectingTask	*pTask = (CWaitConnectingTask *) _WaitingThread->getRunnable();
	pTask->requirExit();
	pTask->close();

	_WaitingThread->wait();

	delete pTask;
	delete _WaitingThread;

	_WaitingThread = NULL;

	return	true;
}

void	CSecurityClient::sendTo (const SIPNET::CMessage &buffer, const CInetAddress& addrUdp)
{
	_UDPClient->sendTo(const_cast<CMessage &> (buffer), const_cast<CInetAddress &> (addrUdp));
}

int		CSecurityClient::CanConnect( const CInetAddress& addrUdp )
{
	sipdebug("CanConnect enter");

	static	uint16	peerport = BaseUDPPort.get();
	peerport ++;
	_UDPClient->init(this, peerport);

	int		nReply = -1;

	CMessage	msgout(M_SECURITYCLIENT_CANCONNECT);
	uint32	len = msgout.length();

	bool bSend = _UDPClient->sendTo(msgout, const_cast<CInetAddress &> (addrUdp));
	sipdebug("_UDPClient->sendTo result=%d", bSend);

	if ( bSend )
		nReply = CON_CONNECT;
	else
		nReply = CON_FAIL;

	//  Temparally comment //  [2/21/2010 KimSuRyon]
	return nReply;
//////////////////////////////////////////////////////////////////////////
	sint32 timeout = getTimeout();
	H_AUTO(LWaitSSConOK);
	TTime t0 = CTime::getLocalTime();

	if ( _NoRecieve )
		return	CON_CONNECT;

	g_bAllowConnect = false;
	g_bRefuseConnect = false;
	g_bWaitConnect = false;

	CWaitConnectingTask	*pWaitingTask	= NULL;
	// recieve first response message
   	while ( !g_bAllowConnect && !g_bRefuseConnect && !g_bWaitConnect )
	{
		_UDPClient->update();
		UpdateNotifyWaitingConnecions(this);

		if ( g_bRefuseConnect )
		{
			nReply = CON_REFUSE;
			break;
		}
		if ( g_bAllowConnect )
		{
			nReply = CON_CONNECT;
			break;
		}
		if ( g_bWaitConnect )
		{
			nReply = CON_WAIT;

			pWaitingTask	= new CWaitConnectingTask;
			pWaitingTask->init(this);
			_WaitingThread	= SIPBASE::IThread::create(pWaitingTask);
			_WaitingThread->start();
			break;
		}

		if (( _Timeout != TIME_INFINITE ) && ( CTime::getLocalTime() - t0 > _Timeout ))
		{
            throw	ETimeout("SecurityConnection Timeout");
			break;
		}

		sipSleep(SleepTime.get());
	}

	if ( nReply == -1 )
		nReply = CON_FAIL;

	return nReply;
}


int		CSecurityClient::GMCanConnect( const CInetAddress& addrUdp )
{
	static	uint16	peerport = BaseUDPPort.get();
	peerport ++;
	_UDPClient->init(this, peerport);

	int		nReply = -1;

	CMessage	msgout(M_SECURITYCLIENT_GMCONNECT);
	uint32	len = msgout.length();
	bool bSend = _UDPClient->sendTo(msgout, const_cast<CInetAddress &> (addrUdp));
	if ( bSend )
		nReply = CON_CONNECT;
	else
		nReply = CON_FAIL;

	// Temparally comment //  [2/21/2010 KimSuRyon]
	return nReply;

	//////////////////////////////////////////////////////////////////////////
	sint32 timeout = getTimeout();
	H_AUTO(LWaitSSConOK);
	TTime t0 = CTime::getLocalTime();

	if ( _NoRecieve )
		return	CON_CONNECT;

	g_bAllowConnect = false;
	g_bRefuseConnect = false;
	g_bWaitConnect = false;

	CWaitConnectingTask	*pWaitingTask	= NULL;
	// recieve first response message
	while ( !g_bAllowConnect && !g_bRefuseConnect && !g_bWaitConnect )
	{
		_UDPClient->update();
		UpdateNotifyWaitingConnecions(this);

		if ( g_bRefuseConnect )
		{
			nReply = CON_REFUSE;
			break;
		}
		if ( g_bAllowConnect )
		{
			nReply = CON_CONNECT;
			break;
		}
		if ( g_bWaitConnect )
		{
			nReply = CON_WAIT;

			pWaitingTask	= new CWaitConnectingTask;
			pWaitingTask->init(this);
			_WaitingThread	= SIPBASE::IThread::create(pWaitingTask);
			_WaitingThread->start();
			break;
		}

		if (( _Timeout != TIME_INFINITE ) && ( CTime::getLocalTime() - t0 > _Timeout ))
		{
			throw	ETimeout("SecurityConnection Timeout");
			break;
		}

		sipSleep(SleepTime.get());
	}

	if ( nReply == -1 )
		nReply = CON_FAIL;

	return nReply;
}

void	CSecurityClient::addSecurityCallbackArray (const TDefSecurityCallbackItem *callbackarray, sint arraysize)
{
	checkThreadId ();

	if (arraysize == 1 && callbackarray[0].mode == NON_CRYPT && M_SYS_EMPTY == callbackarray[0].Key)
	{
		// it's an empty array, ignore it
		return;
	}

	for (sint i = 0; i < arraysize; i++)
	{
		MsgNameType	strMes = callbackarray[i].Key;
		_SecurityCallbackArray.insert( std::make_pair(strMes, callbackarray[i]) );
	}
}
void	CSecurityClient::connect( const CInetAddress& addr, uint32 usec )
{
	G_WATCH_BEGIN(SecureConnect);

	CCallbackClient::connect(addr, usec);

	H_AUTO(LWaitSSConOK);
	TTime t0 = CTime::getLocalTime();

	// KSR_ADD
//	if ( ! UseSecurity )
//	{
//		_bWaiting = false;
//	}

	if ( !_NoRecieve )
	{
		g_bConnect = false;
		while ( CCallbackClient::connected() && !g_bConnect )
		{
			//if ( _NoRecieve )
				CCallbackClient::update2();
			if ( g_bConnect )
				break;
			if (( _Timeout != TIME_INFINITE ) && ( CTime::getLocalTime() - t0 > _Timeout ))
			{
				throw	ETimeout("SecurityConnection Timeout");
				break;
			}
			sipSleep(SleepTime.get());
		}
	}

	if ( g_bConnect )
	{
		setDefCryptMode(CSecurityInfo::getInstance().getDefMode());
		_bWaiting = false;
	}

	// 	catch (ESecurity& e)
	// 	{
	// 		siperror("Security Connection Failed to Security Server, reason : %s", e.what());
	// 	}
}

/*
* Send a message to the remote host (pushing to its send queue)
* Recorded : YES
* Replayed : MAYBE
*/
void	CSecurityClient::send (const CMessage &buffer, TTcpSockId hostid, uint8 bySendType)
{
	CMessage	mes(buffer);
	sipassert (hostid == InvalidSockId);	// should always be InvalidSockId on client
	checkThreadId ();
	sipassert (connected ());
	sipassert (mes.length() != 0);
	sipassert (mes.typeIsSet());

	// Securty층에 설정된 Default암호화방식을 사용하여 message에 설정한다.
	if ( mes.getCryptMode() == DEFAULT )
		mes.setCryptMode(_DefCryptSendMode);

	// 특정의 파케트에 대한 암호화방식으로 무조건 암호화시킨다
	TSECURITYCBIT it = _SecurityCallbackArray.find(mes.getName());
	if ( it != _SecurityCallbackArray.end() )
	{
		TDefSecurityCallbackItem cbitem = (*it).second;
		mes.setCryptMode(cbitem.mode);
	}

	// 메쎄지에 설정된 암호화방식으로 암호화한다.
	if ( mes.getCryptMode() != NON_CRYPT )
		mes.encodeBody(_BufSock);

	_BytesSent += mes.length ();

	// debug features, we number all packet to be sure that they are all sent and received
	// \todo remove this debug feature when ok
	// fill the number
	uint32 *val = (uint32*)mes.buffer ();
#ifdef SIP_BIG_ENDIAN
	*val = SIPBASE_BSWAP32(SendNextValue);
#else
	*val = SendNextValue;
#endif
	SendNextValue++;


#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Send
		CBufClient::send (mes, bySendType);

#ifdef USE_MESSAGE_RECORDER
		if ( _MR_RecordingState == Record )
		{
			// Record sent message
			_MR_Recorder.recordNext( _MR_UpdateCounter, Sending, hostid, const_cast<CMessage&>(mes) );
		}
	}
	else
	{
		/// \todo cado: check that the next sending is the same
	}
#endif
}

/*
* Read the next message in the receive queue
* Recorded : YES
* Replayed : YES
*/
void CSecurityClient::receive (CMessage &buffer, TSockId *hostid)
{
	checkThreadId ();
	//	sipassert (connected ());
	*hostid = InvalidSockId;

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Receive
		CBufClient::receive (buffer);

		// debug features, we number all packet to be sure that they are all sent and received
		// \todo remove this debug feature when ok
#ifdef SIP_BIG_ENDIAN
		uint32 val = SIPBASE_BSWAP32(*(uint32*)buffer.buffer ());
#else
		uint32 val = *(uint32*)buffer.buffer ();
#endif

		//		sipdebug ("receive message number %u", val);
		if (ReceiveNextValue != val)
		{
//			sipstopex (("LNETL3C: !!!LOST A MESSAGE!!! I received the message number %u but I'm waiting the message number %u (cnx %s), warn online3d@samilpo.com with the log now please", val, ReceiveNextValue, id()->asString().c_str()));
			// reSYM the message number
			ReceiveNextValue = val;
		}
		ReceiveNextValue++;

#ifdef USE_MESSAGE_RECORDER
		if ( _MR_RecordingState == Record )
		{
			// Record received message
			_MR_Recorder.recordNext( _MR_UpdateCounter, Receiving, *hostid, const_cast<CMessage&>(buffer) );
		}
	}
	else
	{
		// Retrieve received message loaded by dataAvailable()
		buffer = _MR_Recorder.ReceivedMessages.front().Message;
		_MR_Recorder.ReceivedMessages.pop();
	}
#endif

	buffer.readType ();
	
	if ( buffer.getCryptMode() != NON_CRYPT )
	{
		buffer.decodeBody(_BufSock);
	}
}

void CSecurityClient::cleanKey()
{
	CKeyManager::getInstance().unregisterKey(_BufSock);
}

} // namespace SIPNET