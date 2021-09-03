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
#include "net/UDPPeerServer.h"
#include "net/SecurityServer.h"
#include "net/Key.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

CSecurityServer	*_Server = NULL;

void	cbClientHello( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
void	cbClientCertificate( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);
void	cbClientKeyExchange( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb);

TCallbackItem	g_postConnectionServerCBAry[] = 
{
	{ M_CLIENTHELLO,	cbClientHello },
	{ M_CERTIFICATE,	cbClientCertificate },
	{ M_KEYEXCHANGE,	cbClientKeyExchange },
};

namespace SIPNET
{

CSecurityServer::CSecurityServer( TCRYPTMODE cryptmode, bool bUdpServer, bool bClientServer, uint32 max_frontnum, CCallbackNetBase::TRecordingState rec, const std::string& recfilename, bool recordall, bool initPipeForDataAvailable ) :
	CCallbackServer(bUdpServer, true, bClientServer, max_frontnum, rec, recfilename, recordall, initPipeForDataAvailable)
{
	_DefCryptSendMode = cryptmode;
}

CSecurityServer::~CSecurityServer(void)
{
	_SecurityConnectionCallback = NULL;
	_SecurityConnectionCbArg	= NULL;
}

void	CSecurityServer::setDefCryptMode(TCRYPTMODE mode)
{
	_DefCryptSendMode = mode;
}

void	CSecurityServer::init(uint16 nTCPPort)
{
	CCallbackServer::init(nTCPPort);

	if ( _DefCryptSendMode == NON_CRYPT )
		return;

	CCallbackServer::addCallbackArray(g_postConnectionServerCBAry, sizeof(g_postConnectionServerCBAry)/sizeof(g_postConnectionServerCBAry[0]));
	CCallbackServer::setConnectionCallback(cbClientConnection, this);

	try
	{
		CKeyManager::getInstance().generateAsymKey(SIPNET::InvalidSockId, true);			
	}
	catch(ESecurity& e)
	{
		siperror("SecurityServer Init Error reason : %s", e.what());
	}
}

void	CSecurityServer::addSecurityCallbackArray (const TDefSecurityCallbackItem *callbackarray, sint arraysize)
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

void	CSecurityServer::send (const CMessage &buffer, TSockId hostid, uint8 bySendType)
{
	CMessage	mes(buffer);
	checkThreadId ();
	sipassert (connected ());
	sipassert (mes.length() != 0);
	sipassert (mes.typeIsSet());

	// Securty층에 설정된 Default암호화방식을 사용하여 message에 설정한다.
	if ( mes.getCryptMode() == DEFAULT )
		mes.setCryptMode(_DefCryptSendMode);
	// 메쎄지에 설정된 암호화방식으로 암호화한다.
	TCRYPTMODE	mode = buffer.getCryptMode();
	if ( mode < MIN_CRYPTMODE || mode > MAX_CRYPTMODE )
	{
		sipwarning("STOP: Message<%s> Unknown crypt mode! There is a conflict in server securitysystem", buffer.toString().c_str());
		return;
	}

	// 특정의 파케트에 대한 암호화방식으로 무조건 암호화시킨다
    TSECURITYCBIT it = _SecurityCallbackArray.find(mes.getName());
	if ( it != _SecurityCallbackArray.end() )
	{
		TDefSecurityCallbackItem cbitem = (*it).second;
		mes.setCryptMode(cbitem.mode);
	}

//	if ( UseSecurity.get() )
//		if ( mes.getCryptMode() != NON_CRYPT )
//			mes.encodeBody(hostid);

	if (hostid == InvalidSockId)
	{
		// broadcast
		sint nb = nbConnections ();
		_BytesSent += mes.length () * nb;
	}
	else
	{
		_BytesSent += mes.length ();
	}

#ifdef USE_MESSAGE_RECORDER	
	if ( _MR_RecordingState != Replay )
	{
#endif
		// Send
		CBufServer::send (mes, hostid, bySendType);

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
void	CSecurityServer::receive (CMessage &buffer, TSockId *hostid)
{
	checkThreadId ();
	sipassert (connected ());

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Receive
		CBufServer::receive (buffer, hostid);

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
		*hostid = _MR_Recorder.ReceivedMessages.front().SockId;
		_MR_Recorder.ReceivedMessages.pop();
	}
#endif

	buffer.readType ();

//	if ( !UseSecurity.get() )
//		return;

	// check valid cryptmode
	TCRYPTMODE	mode = buffer.getCryptMode();
	if ( mode < MIN_CRYPTMODE || mode > MAX_CRYPTMODE || mode == DEFAULT )
	{
		sipwarning("STOP: Message<%s> Unknown crypt mode! Server disconenct client", buffer.toString().c_str());
		disconnect(*hostid);
		return;
	}

	// for special packet, check valid cryptmode
    TSECURITYCBIT it = _SecurityCallbackArray.find(buffer.getName());
	if ( it != _SecurityCallbackArray.end() )
	{
		TDefSecurityCallbackItem cbitem = (*it).second;
		if ( buffer.getCryptMode() != cbitem.mode )
		{
#ifdef SIP_MSG_NAME_DWORD
			sipwarning("STOP: CryptMode of Message<%d> must crypt mode (%u)! Current CryptMode :%u, Server disconenct client", buffer.getName(), cbitem.mode, buffer.getCryptMode());
#else
			sipwarning("STOP: CryptMode of Message<%s> must crypt mode (%u)! Current CryptMode :%u, Server disconenct client", buffer.getName().c_str(), cbitem.mode, buffer.getCryptMode());
#endif // SIP_MSG_NAME_DWORD
			//disconnect(*hostid); // add_KSR 2009_11
			forceClose(*hostid);
			return;
		}
	}

	// cryption
	if ( mode != NON_CRYPT )
	{
		buffer.decodeBody(*hostid);
	}
}

// Common Connection
void CSecurityServer::cbClientConnection( TSockId from, void *arg )
{
	AUTO_WATCH(ClientConnection);
	H_AUTO(SecurityConnection);

	CMessage	msgout(M_SERVERHELLO, NON_CRYPT);

	CSecurityInfo & info = CSecurityInfo::getInstance();
//	msgout.serial(info.getSymInfo());
//	msgout.serial(info.getASymInfo());
//	msgout.serial(info.getKeyGenerateInfo());
//	msgout.serial(info.getWrapInfo());
	TCRYPT_INFO crypt_info;
	crypt_info = info.getSymInfo();
	msgout.serial(crypt_info);
	crypt_info = info.getASymInfo();
	msgout.serial(crypt_info);
	crypt_info = info.getKeyGenerateInfo();
	msgout.serial(crypt_info);
	crypt_info = info.getWrapInfo();
	msgout.serial(crypt_info);

	uint8	nDefMode = (uint8) info.getDefMode();
	msgout.serial(nDefMode);

	CSecurityServer * Server = (CSecurityServer *)arg;
	sipinfo("<-CONNECTION> A Client appID : [%u] Connection Accepted!", from->appId());
	Server->send(msgout, from);
	sipinfo("<SERVER HELLO-> Message Send To [%u]!", from->appId());

	Server->authorizeOnly (M_CLIENTHELLO, from, AuthorizeTime.get());
}

void	CSecurityServer::update ( sint32 timeout )
{
	CCallbackServer::update(timeout);
}

void	CSecurityServer::release()
{
}

} // namespace SIPNET

/************************************************************************************/
// PostConnection Handshake
/************************************************************************************/
// packet : CLIENTHELLO
// Client -> Server
void cbClientHello( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	AUTO_WATCH(CLIENTHELLO);
    H_AUTO(CLIENTHELLO);

	CSecurityServer * server = dynamic_cast<CSecurityServer *>(&clientcb);

	uint8	initSuccess;
	msgin.serial(initSuccess);

	if ( initSuccess == 0 )
	{
		CMessage msgout(M_HANDSHKFAIL, NON_CRYPT);
		server->send(msgout, from);
		return;
	}

	CMessage	msgout(M_CERTIFICATE, NON_CRYPT);
	CSecretKey	key = CKeyManager::getInstance().generateCookieKey(from, true);
	msgout.serial(key);

	server->send(msgout, from);
	server->authorizeOnly (M_CERTIFICATE, from, AuthorizeTime.get());
}

// packet : CERTIFICATE
// Client -> Server
void cbClientCertificate( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	AUTO_WATCH(CERTIFICATE);
    H_AUTO(CERTIFICATE);

	sipinfo("<-CLIENT CERTIFICATE> A Client appID : [%u] Client Certification comes!", from->appId());
	CSecurityServer * server = dynamic_cast<CSecurityServer *>(&clientcb);

	try
	{
		CSecretKey	keySend, keyReply;
		msgin.serial(keySend);
		msgin.serial(keyReply);

		CMessage	msgout(M_KEYEXCHANGE, NON_CRYPT);

		if( !CKeyManager::getInstance().checkPairKey(keySend, keyReply) )
			throw ESecurity("Invalid Key Pair!");

		// Certificate OK!
		CSecretKey	cookie = CKeyManager::getInstance().generateCookieKey(from, true);
		msgout.serial(cookie);

		PKEY_INFO	asymkey = CKeyManager::getInstance().getAsymKey(from);
		uint32 size = asymkey->pubkeySize;
		msgout.serial(size);
		msgout.serialBuffer(asymkey->pubKey, size);

		server->send(msgout, from);
		sipdebug("Send Public Key! <%d>, size:%d", asymkey->pubKey, size);

		server->authorizeOnly (M_KEYEXCHANGE, from, AuthorizeTime.get());
	}
	catch (ESecurity& e)
	{
		CMessage msgout(M_HANDSHKFAIL, NON_CRYPT);
		sipwarning("Certificate Failed, reason : %s", e.what());
		server->send(msgout, from);
		server->disconnect(from);
	}
	catch (ETimeout& e)
	{
		server->disconnect(from);
		sipdebug("ERROR : SecureConnection, %s", e.what());
	}
}

// packet : KEYEXCHANGE
// Client -> Server>	frontend_d.exe!cbClientCertificate(SIPNET::CMessage & msgin={...}, SIPNET::CBufSock * from=0x00eaac58, SIPNET::CCallbackNetBase & clientcb={...})  Line 89	C++

void cbClientKeyExchange( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	AUTO_WATCH(KEYEXCHANGE);
    H_AUTO(KEYEXCHANGE);

	sipinfo("<-CLIENT KEYEXCHANGE> A Client appID : [%u] Client Key Exchanged!", from->appId());

	CSecurityServer * server = dynamic_cast<CSecurityServer *>(&clientcb);

	try
	{
		CSecretKey	cookie;
		msgin.serial(cookie);
		if ( ! CKeyManager::getInstance().existCookie(from) )
			throw ESecurity("There is no cookie");

		CSymKey	secretKey;
		uint32	size;						msgin.serial(size);
		uint8	keybuff[MAX_KEY_LENGTH];	msgin.serialBuffer(keybuff, size);
		secretKey.setSecretKey(keybuff, size);
		CKeyManager::getInstance().registerKey(secretKey.getKey(), from);
		sipdebug("Recive Secret Key!<%d>, size:%d", keybuff, size);
	}
	catch (ESecurity& e)
	{
		CMessage msgout(M_HANDSHKFAIL, NON_CRYPT);
		sipwarning("Certificate Failed, reason : %s", e.what());
		server->send(msgout, from);
		server->disconnect(from);
	}

	// Send "FINISH"
	CMessage msgout(M_HANDSHKOK, NON_CRYPT);
	server->send(msgout, from);
	sipinfo("<HANDSHKOK-> Message Send To [%u]!", from->appId());
	sipinfo("Security Connection Completed successfully!");

	if ( server->_SecurityConnectionCallback )
		server->_SecurityConnectionCallback(from, server->_SecurityConnectionCbArg);
}