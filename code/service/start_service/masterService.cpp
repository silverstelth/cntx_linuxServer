#include "misc/types_sip.h"

#include 	<stdio.h>
#include 	<ctype.h>
#include 	<math.h>
#include 	<vector>
#include 	<map>

#include 	"misc/debug.h"
#include 	"misc/config_file.h"
#include 	"misc/displayer.h"
#include 	"misc/command.h"
#include 	"misc/log.h"
#include 	"misc/window_displayer.h"
#include 	"misc/path.h"

#include 	"net/service.h"
#include 	"net/login_server.h"
#include 	"net/login_cookie.h"
#include 	"net/CryptEngine.h"
#include 	"net/SecurityServer.h"
#include 	"net/DelayServer.h"
#include	"net/UDPPeerServer.h"
#include	"net/cookie.h"

#include 	"../../common/common.h"
#include 	"../../common/err.h"
#include	"../../common/Packet.h"
#include 	"../Common/netCommon.h"
#include	"../Common/DBData.h"
#include 	"../Common/QuerySetForMain.h"

#include	"misc/DBCaller.h"

#include 	"start_service.h"
#include 	"UpdateServer.h"
#include	"slaveService.h"
#include	"masterService.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

CMasterService * MasterService = NULL;

CVariable<bool> ConnectOpen("SS", "ConnectOpen", "the flag for permiting about client's connection", true, 0, true);

enum	ENUMRQState { _FREE, _BUSY };

uint32	NbIncomingConnections = 0;
uint32	ProcTime = 0;

void	cbQueryDelayConnection( SIPNET::CMessage& msgin, SIPNET::CInetAddress& peeraddr, SIPNET::CUDPPeerServer * peer);
void	cbCancelDelayConnection( SIPNET::CMessage& msgin, SIPNET::CInetAddress& peeraddr, SIPNET::CUDPPeerServer * peer);
void	cbGMQueryDealyConnection( SIPNET::CMessage& msgin, SIPNET::CInetAddress& peeraddr, SIPNET::CUDPPeerServer * peer);
TUDPCallbackItem	aryClientCallback[] =
{
	// Common Client
	{ M_SECURITYCLIENT_CANCONNECT, cbQueryDelayConnection },
	{ M_SC_CANCELCONNECTION,		cbCancelDelayConnection },
    
	// GM Client
	{ M_SECURITYCLIENT_GMCONNECT,	cbGMQueryDealyConnection },
};

bool	g_bAcceptCookie = false;
void	cbSlaveIdentify(CMessage &msgin, const std::string &serviceName, TServiceId sid);
void	cbSlaveNotifyRQState(CMessage &msgin, const std::string &serviceName, TServiceId sid);
void	cbSlaveCookieReply(CMessage &msgin, const std::string &serviceName, TServiceId sid);

TUnifiedCallbackItem slaveSSCallbackArray[] =
{
	{ M_SS_SLAVEIDENT,			cbSlaveIdentify },
	{ M_SS_RQSTATE,				cbSlaveNotifyRQState },
	{ M_SS_RECEIVED_COOKIE,		cbSlaveCookieReply },
};

void	cbSSSlaveConnection(const std::string &serviceName, TServiceId sid, void *arg);
void	cbSSSlaveDisconnection(const std::string &serviceName, TServiceId sid, void *arg);


CMasterService::CMasterService(SIPNET::CInetAddress masterAddrForClient)
{
	_clientListenAddr = masterAddrForClient;

	uint16	nPort = masterAddrForClient.port();
	_server = new	CDelayServer;
	_server->init(nPort);
	_server->addCallbackArray(aryClientCallback, sizeof(aryClientCallback)/sizeof(aryClientCallback[0]));

	// UpdateServer init
	if ( ! CUpdateServer::init(_server) )
		sipwarning("UpdateServer Failed");

	CUnifiedNetwork::getInstance()->setServiceUpCallback(SS_S_NAME, cbSSSlaveConnection, NULL);
	CUnifiedNetwork::getInstance()->setServiceDownCallback(SS_S_NAME, cbSSSlaveDisconnection, NULL);
	CUnifiedNetwork::getInstance()->addCallbackArray(slaveSSCallbackArray, sizeof(slaveSSCallbackArray)/sizeof(slaveSSCallbackArray[0]));

	sipinfo("MASTER start! port : %d", nPort);
}

CMasterService::~CMasterService()
{
	release();
}

void	CMasterService::update(sint32 timeout)
{
	_server->update(timeout);

	CDelayServer *	delayServer		= MasterService->Server();
	CRQObserver		slaveinfo(CMasterService::ownSlaveId(), CMasterService::ownSlaveName(), SlaveService->ListenAddr(), SlaveService->PublicAddr(),
		SlaveService->state(), SlaveService->GetRQConnections(), 0, SlaveService->GetReceiveQueueItem());
	delayServer->notifyRQState(slaveinfo);
}

void	CMasterService::update2( sint32 timeout, sint32 mintime)
{
	_server->update(timeout);

	CDelayServer *	delayServer		= MasterService->Server();
	CRQObserver		slaveinfo(CMasterService::ownSlaveId(), CMasterService::ownSlaveName(), SlaveService->ListenAddr(), SlaveService->PublicAddr(),
		SlaveService->state(), SlaveService->GetRQConnections(), 0, SlaveService->GetReceiveQueueItem());
	delayServer->notifyRQState(slaveinfo);
}

void	CMasterService::release()
{
	if ( _server != NULL )
		delete	_server;
	_server = NULL;
}

void	CMasterService::send (const CMessage &buffer, TTcpSockId hostid, uint8 bySendType)
{
	_server->send(buffer, hostid, bySendType);
}

void	CMasterService::receive (SIPNET::CMessage &buffer, SIPNET::TSockId *hostid)
{
	_server->receive(buffer, hostid);
}

uint32	CMasterService::WQSize()
{
	return	_server->getWaitingQueueFIFOSize();
}

struct TWaitingInfo
{
	TWaitingInfo(uint32 _u, uint32 _t) : nWaitUsers(_u),tmInterval(_t) {}
	uint32	nWaitUsers;
	uint32	tmInterval;
};

typedef		std::map<std::string, std::string>				_TCOOKIEMAP;
typedef		std::map<std::string, std::string>::iterator	_TCOOKIEARYIT;
static _TCOOKIEMAP		_mapConnectTempCookie;

typedef		std::map<std::string, TWaitingInfo>				_TWAITCOOKIEMAP;
typedef		std::map<std::string, TWaitingInfo>::iterator	_TWAITCOOKIEARYIT;
static _TWAITCOOKIEMAP		_mapWaitTempCookie;

// Packet : M_SS_RECEIVED_COOKIE
// Slave ->Master
void	cbSlaveCookieReply(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	CDelayServer *	delayServer		= MasterService->Server();

	CInetAddress		peeraddr;	msgin.serial(peeraddr);

	string	strCookie = peeraddr.asUniqueString();
	{
		_TCOOKIEARYIT	itConnect = _mapConnectTempCookie.find(strCookie);
		if ( itConnect != _mapConnectTempCookie.end() )
		{
			_mapConnectTempCookie.erase(itConnect);
			delayServer->notifyConnectToClient(sid, peeraddr);
		}
	}

	_TWAITCOOKIEARYIT	itWait = _mapWaitTempCookie.find(strCookie);
	if ( itWait != _mapWaitTempCookie.end() )
	{
		_mapWaitTempCookie.erase(itWait);
		delayServer->notifyWaitToClient(sid, peeraddr, itWait->second.nWaitUsers, itWait->second.tmInterval);
	}
}

void	sendConnectToClient(TServiceId serviceId, SIPNET::CInetAddress& peeraddr)
{
	CDelayServer *	delayServer		= MasterService->Server();
	CInetAddress	listenAddr = delayServer->getSlaveListenAddress(serviceId);
	std::string		ssHost = MasterService->clientListenAddr().ipAddress();

	if ( listenAddr.ipAddress() == ssHost )
	{
		insertCookieInfo(peeraddr);
		delayServer->notifyConnectToClient(serviceId, peeraddr);
		return;
	}

	string	strCookie = peeraddr.asUniqueString();
	_TCOOKIEARYIT	it = _mapConnectTempCookie.find(strCookie);
	if ( it != _mapConnectTempCookie.end() )
		return;

	// send a cookie to server
	SIPNET::CMessage	msgToSlave(M_SS_NOTIFYCOOKIETOSLAVE);
	msgToSlave.serial(peeraddr);
	CUnifiedNetwork::getInstance()->send(serviceId, msgToSlave);

	_mapConnectTempCookie.insert(std::make_pair(strCookie, strCookie));
    // send a ccokie to server
	//delayServer->notifyConnectToClient(serviceName, peeraddr);
}

void	sendWaitToClient(TServiceId serviceId, SIPNET::CInetAddress& peeraddr, uint32 nbWaitUser, uint32 timeInterval)
{
/*
	CDelayServer *	delayServer = MasterService->Server();
	uint32	time = SIPBASE::CTime::getTick();
	delayServer->_Waitings.insert(std::make_pair(peeraddr, time));

	std::vector<uint8> buffer;
	const uint8 lenIPAddr = 16;
	std::string	hostName = peeraddr.ipAddress();
	uint16		port = peeraddr.port();
	buffer.resize( lenIPAddr + sizeof(uint16) + sizeof(uint32) );
	memcpy( &*buffer.begin(), hostName.c_str(), lenIPAddr );
	memcpy( &*buffer.begin() + lenIPAddr, &port, sizeof(uint16) );
	memcpy( &*buffer.begin() + lenIPAddr + sizeof(uint16), &time, sizeof(uint32) );
	delayServer->pushMessageIntoWaitingQueue(buffer);

	CMessage	msgout(M_WAIT);
	msgout.serial(nbWaitUser, timeInterval);
	delayServer->sendTo(msgout, peeraddr);
*/
	CDelayServer *	delayServer		= MasterService->Server();
	CInetAddress	listenAddr = delayServer->getSlaveListenAddress(serviceId);
	std::string		ssHost = MasterService->clientListenAddr().ipAddress();

	if ( listenAddr.ipAddress() == ssHost )
	{
		insertCookieInfo(peeraddr);
		delayServer->notifyWaitToClient(serviceId, peeraddr, nbWaitUser, timeInterval);
		return;
	}

	string	strCookie = peeraddr.asUniqueString();
	_TWAITCOOKIEARYIT	it = _mapWaitTempCookie.find(strCookie);
	if ( it != _mapWaitTempCookie.end() )
		return;

	// send a cookie to server
	SIPNET::CMessage	msgToSlave(M_SS_NOTIFYCOOKIETOSLAVE);
	msgToSlave.serial(peeraddr);
	CUnifiedNetwork::getInstance()->send(serviceId, msgToSlave);

	_mapWaitTempCookie.insert(std::make_pair(strCookie, TWaitingInfo(nbWaitUser, timeInterval)));
}

void	sendRefuseToClient(SIPNET::CInetAddress& peeraddr)
{
	CDelayServer *	delayServer = MasterService->Server();

	CMessage	msgout(M_SECURITYCLIENT_REFUSE);
	delayServer->sendTo(msgout, peeraddr);
}

// CANCONNECT
void	cbQueryDelayConnection( SIPNET::CMessage& msgin, SIPNET::CInetAddress& peeraddr, SIPNET::CUDPPeerServer * peer)
{
	if (!ConnectOpen.get())
		return;

	CDelayServer *	delayServer = MasterService->Server();
	uint32			nbWaitUser = 0;
	uint32			timeInterval = 0;
	SIPNET::TServiceId		serviceId;

	if ( TestDelay.get() )
		QueryConnections ++;

	_DELAYCONNECTRESULT		nState = delayServer->calcQueueState(nbWaitUser, timeInterval, serviceId);
    if ( nState == _CONNECT )
	{
		sendConnectToClient(serviceId, peeraddr);
	}

	if ( nState == _WAIT )
	{
		sendWaitToClient(serviceId, peeraddr, nbWaitUser, timeInterval);
	}

	if ( nState == _REFUSE )
	{
		sendRefuseToClient(peeraddr);
	}
}

// packet : GMCONNECT
void	cbGMQueryDealyConnection( SIPNET::CMessage& msgin, SIPNET::CInetAddress& peeraddr, SIPNET::CUDPPeerServer * peer)
{
	CDelayServer *	delayServer = MasterService->Server();

	if ( TestDelay.get() )
		QueryConnections ++;

	delayServer->notifyConnectToClient(CMasterService::ownSlaveId(), peeraddr);

	sipinfo("DELAYCONNECT : <GMCANCONNECT> UDP Peer:%s", peeraddr.asIPString().c_str());
}

// packet : CANCEL
// client -> server
void	cbCancelDelayConnection( SIPNET::CMessage& msgin, SIPNET::CInetAddress& peeraddr, SIPNET::CUDPPeerServer * peer)
{
	CDelayServer * delayServer = MasterService->Server();

	delayServer->_Waitings.erase(peeraddr);
	sipinfo("DELAYCONNECT : <CANCEL> UDP Peer:%s is canceled", peeraddr.asString().c_str());
}

// packet : RQSTATE
// slave -> master
void cbSlaveNotifyRQState(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint8	nState;			msgin.serial(nState);
	uint32	nbPlayers;		msgin.serial(nbPlayers);
	uint32	procSpeedPerMS;	msgin.serial(procSpeedPerMS);
	uint32	sizeRQ;			msgin.serial(sizeRQ);

	NbIncomingConnections = nbPlayers;

	CDelayServer *	delayServer = MasterService->Server();

	SIPNET::CInetAddress	addr;
	CRQObserver		slave(sid, serviceName, addr, addr, nState, nbPlayers, procSpeedPerMS, sizeRQ);
	delayServer->notifyRQState(slave);
}

void	RefuseSlaveConnection(const std::string &serviceName, TServiceId sid)
{
	std::string	reason = "Already same slave service is running";
	sipwarning(reason.c_str ());

	CMessage msgout(M_SC_FAIL);
	msgout.serial (reason);
	CUnifiedNetwork::getInstance()->send ((TServiceId)sid, msgout);
}

// Slave Connection
void cbSSSlaveConnection(const std::string &serviceName, TServiceId sid, void *arg)
{
	sipinfo("Slave Service connected from (name:%s, sid:%d)", serviceName.c_str(), sid.get());
}

// packet : SLAVEIDENT
// slave -> master
void cbSlaveIdentify(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	CDelayServer * delayServer = MasterService->Server();
	SIPNET::CInetAddress	addr;		msgin.serial(addr);
	SIPNET::CInetAddress	publicAddr;		msgin.serial(publicAddr);

	CRQObserver slave(sid, serviceName, addr, publicAddr);
	bool bSuccess = delayServer->registerRQObserver(slave);
	if ( !bSuccess )
		RefuseSlaveConnection(serviceName, sid);
}

// Slave Disconnection
void cbSSSlaveDisconnection(const std::string &serviceName, TServiceId sid, void *arg)
{
	CDelayServer * delayServer = MasterService->Server();

	delayServer->unregisterRQObserver(sid);
}
