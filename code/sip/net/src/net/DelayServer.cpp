/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/cookie.h"
#include "net/DelayServer.h"

using namespace		std;
using namespace		SIPBASE;

SIPNET::CDelayServer	*_Server = NULL;

SIPBASE::CVariable<unsigned int>	NotifyInterval("net",	"NotifyInterval",	"NotifyInterval", 1000, 0, true);
SIPBASE::CVariable<unsigned int>	CycleInterval("net",	"CycleInterval",	"CycleInterval", 10, 0, true);

SIPBASE::CVariable<unsigned int>	limitIncomingConnections("net",	"limitIncomingConnections",	"limitIncomingConnections", 10000, 0, true);
SIPBASE::CVariable<unsigned int>	limitRecieveQueueSize("net",	"limitRecieveQueueSize",	"Size limit of RecieveQueue", 160000, 0, true);
SIPBASE::CVariable<unsigned int>	limitRefuseConnections("net",	"limitRefuseConnections",	"limitRefuseConnections", 20000, 0, true);


namespace	SIPNET
{
CRQObserver::CRQObserver() :
	_nState(0),
	_SizeRQ(0),
	_RQConnections(0),
	_ProcSpeedPerMS(1000)
{
    _NotifyTimeGap = NotifyInterval.get();
	_CycleTimeout = CycleInterval.get();
}

CRQObserver::CRQObserver(SIPNET::TServiceId sid, std::string serviceName, CInetAddress & listenAddr, CInetAddress & publicAddr, uint8 nState, uint32 nbConnections, uint32 procSpeed, uint64 sizeRQ)
{
	_sid		= sid;
	_serviceName= serviceName;
	_listenAddr	= listenAddr;	
	_publicAddr	= publicAddr;	

	_nState		= nState;
	_SizeRQ		= sizeRQ;
	_RQConnections = nbConnections;
	_ProcSpeedPerMS = procSpeed;

    _NotifyTimeGap = NotifyInterval.get();
	_CycleTimeout = CycleInterval.get();
}

CRQObserver::CRQObserver(const CRQObserver &other)
{
	_sid			= other._sid;
	_serviceName	= other._serviceName;
	_listenAddr		= other._listenAddr;
	_publicAddr		= other._publicAddr;

	_nState			= other._nState;
	_SizeRQ			= other._SizeRQ;
	_ProcSpeedPerMS	= other._ProcSpeedPerMS;
	_RQConnections	= other._RQConnections;
    _CycleTimeout	= other._CycleTimeout;
	_NotifyTimeGap	= other._NotifyTimeGap;
}

CRQObserver::~CRQObserver()
{
}

CRQObserver & CRQObserver::operator=(const CRQObserver & rqObserver)
{
	_sid			= rqObserver._sid;
	_serviceName	= rqObserver._serviceName;
	_listenAddr		= rqObserver._listenAddr;
	_publicAddr		= rqObserver._publicAddr;

	_nState			= rqObserver._nState;
	_SizeRQ			= rqObserver._SizeRQ;
	_ProcSpeedPerMS	= rqObserver._ProcSpeedPerMS;
	_listenAddr		= rqObserver._listenAddr;
	_RQConnections	= rqObserver._RQConnections;
    _CycleTimeout	= rqObserver._CycleTimeout;
	_NotifyTimeGap	= rqObserver._NotifyTimeGap;

	return	*this;
}

void	CRQObserver::modifyRQState(CRQObserver &rqObserver)
{
	_nState			= rqObserver._nState;
	_SizeRQ			= rqObserver._SizeRQ;
	_ProcSpeedPerMS	= rqObserver._ProcSpeedPerMS;
	_listenAddr		= rqObserver._listenAddr;
	_RQConnections	= rqObserver._RQConnections;
    _CycleTimeout	= rqObserver._CycleTimeout;
	_NotifyTimeGap	= rqObserver._NotifyTimeGap;
}

// Can Changeable!
bool	CRQObserver::isBusyRecieveQueueState(uint32 &nbWaitingUser)
{
	nbWaitingUser	= QueueIncomingConnections();
	if ( nbWaitingUser >= limitIncomingConnections.get() ||			// if connection waiting user is larger than limit
			getSizeRQ() >= limitRecieveQueueSize.get() )			// if recieve queue is overflow
		return	true;

	return false;
}


/****************************************************************************************************
//	CDelayServer
****************************************************************************************************/

/*CDelayServer::CDelayServer(SIPNET::INetServer *	server)
	:IProtocolServer(server),
	_WaitingFifo("CDelayServer::_ConnectFifo"),
	_bConnectDelay(true),
	_UDPServer(NULL)
{
	_netServer = server;

	if ( _bConnectDelay )
	{
		_UDPServer = new CUDPPeerServer;
		_CycleTimeout = CycleInterval.get();
	}
}*/

CDelayServer::CDelayServer():
	_WaitingFifo("CDelayServer::_ConnectFifo"),
	_UDPServer(NULL)
{
	_UDPServer = new CUDPPeerServer;
}

CDelayServer::~CDelayServer(void)
{
	if ( _UDPServer )
	{
		delete	_UDPServer;
		_UDPServer = NULL;
	}
}

void CDelayServer::init(uint16 nUDPPort)
{
	//CCallbackServer::init(tcpPort);
	_UDPServer->init(nUDPPort);
	//_UDPServer->setMsgCallback(calback);

	_Server = this;
}

void	CDelayServer::addCallbackArray (const TUDPCallbackItem *callbackarray, sint arraysize)
{
	_UDPServer->addCallbackArray(callbackarray, arraysize);
}

bool	CDelayServer::registerRQObserver(CRQObserver slave)
{
	uint16		sid = slave.getServiceID().get();
	_ITARYSLAVES it = _mapSlaves.find(sid);
	if ( it != _mapSlaves.end() )
	{
		siperrornoex("There is double register slave server : serverId = %d", sid);
		return	false;
	}

	slave.setNotifyTimeGap(CTime::getSecondsSince1970());
	_mapSlaves.insert(std::make_pair(sid, slave));

	return	true;
}

bool	CDelayServer::unregisterRQObserver(TServiceId serviceId)
{
	uint16	sid = serviceId.get();
	_ITARYSLAVES it = _mapSlaves.find(sid);
	if ( it == _mapSlaves.end() )
		return	false;

	_mapSlaves.erase(it);

	return	true;
}

void	CDelayServer::notifyRQState(CRQObserver &slave)
{
	uint16	sid = slave.getServiceID().get();

	_ITARYSLAVES it = _mapSlaves.find(sid);
	if ( it == _mapSlaves.end() )
	{
		sipwarning("Recieved state message from unregistered slave");
	}
	else
	{
		CRQObserver & slaveinfo	= (*it).second;
		slaveinfo.modifyRQState(slave);
	}
}

bool	CDelayServer::isSlaveEmpty()
{
	uint	nCount = _mapSlaves.size();
	return ( nCount == 0 );
}

/*
void	CDelayServer::notifyRQState(SIPNET::TServiceId sid, std::string serviceName, uint8 nState, uint32 nbPlayers, uint32 procTime, uint32 sizeRQ)
{
	_ITARYSLAVES it = _mapSlaves.find(serviceName);
	if ( it == _mapSlaves.end() )
	{
		CRQObserver slave(sid, serviceName, nState, nbPlayers, procTime, sizeRQ);
		_mapSlaves.insert(std::make_pair(serviceName, slave));
	}
	else
	{
		CRQObserver & slave	= (*it).second;
		slave._nState		= nState;
		slave._nbPlayers	= nbPlayers;
		slave._procTime		= procTime;
		slave._SizeRQ		= sizeRQ;
		slave._NotifyTimeGap= SIPBASE::CTime::getSecondsSince1970();
	}
}
*/

void CDelayServer::update ( sint32 timeout )
{
	if ( _UDPServer->IsInited() )
	{
        _UDPServer->update();
	}
}

void	CDelayServer::sendTo(SIPNET::CMessage	&msg, CInetAddress & addr)
{
	_UDPServer->sendTo(msg, addr);
}

CRQObserver	&CDelayServer::getSlaveService(TServiceId serviceId)
{
	return	_mapSlaves[serviceId.get()];
}

void	CDelayServer::notifyConnectToClient(TServiceId	serviceId, CInetAddress& peeraddr)
{
	// send a cookie to client
//	CInetAddress		listenAddr = getSlaveListenAddress(serviceId);
	CInetAddress		listenAddr = getSlavePublicAddress(serviceId);

	string	strCookie = peeraddr.asUniqueString();
	string	strUDPAddr = peeraddr.asIPString();
	SIPNET::CMessage	msgout(M_CONNECT);
    msgout.serial(strCookie);
	msgout.serial(listenAddr);

	sendTo(msgout, peeraddr);
	sipinfo("DELAYCONNECT : notify connect to client, strCookie=%s, udpAddress=%s, listenAddress=%s, msgLength=%d", strCookie.c_str(), strUDPAddr.c_str(), listenAddr.asString().c_str(), msgout.length());
}

void	CDelayServer::notifyWaitToClient(TServiceId	serviceId, CInetAddress& peeraddr, uint32 nbWaitUser, uint32 timeInterval)
{
	// send a cookie to client
//	CInetAddress		listenAddr = getSlaveListenAddress(serviceId);
	CInetAddress		listenAddr = getSlavePublicAddress(serviceId);

	string	strCookie = peeraddr.asUniqueString();
	string	strUDPAddr = peeraddr.asIPString();
	SIPNET::CMessage	msgout(M_WAIT);
	msgout.serial(strCookie, listenAddr, nbWaitUser, timeInterval);
	sendTo(msgout, peeraddr);
	sipinfo("DELAYCONNECT : notify wait to client, strCookie=%s, udpAddress=%s, listenAddress=%s, WaitUser=%d", strCookie.c_str(), strUDPAddr.c_str(), listenAddr.asString().c_str(), nbWaitUser);
}

CInetAddress &	CDelayServer::getSlaveListenAddress(TServiceId	serviceId)
{
    return	_mapSlaves[serviceId.get()].getListenAddr();
}

CInetAddress &	CDelayServer::getSlavePublicAddress(TServiceId	serviceId)
{
    return	_mapSlaves[serviceId.get()].getPublicAddr();
}

#define	MAX_UINT32		0xFFFFFFFF
_DELAYCONNECTRESULT	CDelayServer::calcQueueState(uint32 &nbWaitingUser, uint32 &timeInterval, TServiceId &serviceId)
{
	uint32	minNbUser = MAX_UINT32, minTimeInterval = 0;

    uint32	nbUser1 = 0, nbUser2 = 0;
	uint32	time1 = 0, time2 = 0;
	bool	bBusySelSlave = false;

	// For Receive queue, lookup the best slave service throughout list of slave services
	for ( _ITARYSLAVES it = _mapSlaves.begin(); it != _mapSlaves.end(); it ++ )
	{	
		CRQObserver	 slave = (*it).second;
		bool bState = slave.isBusyRecieveQueueState(nbUser1);
		if ( nbUser1 < minNbUser )
		{
			serviceId		= slave.getServiceID();
    		nbWaitingUser	= minNbUser		= nbUser1;
			timeInterval	= minTimeInterval = slave.CalcTotalProcTimeMS();

			bBusySelSlave	= bState;
		}
	}

	if ( bBusySelSlave )
	{
		if (nbWaitingUser >= limitRefuseConnections.get())
			return _REFUSE;
		return	_WAIT;
	}

	return _CONNECT;
}

void	CDelayServer::pushMessageIntoWaitingQueue( const std::vector<uint8>& buffer )
{
	//sipdebug( "BNB: Acquiring the receive queue... ");
	CFifoAccessor recvfifo( &_WaitingFifo );
	//sipdebug( "BNB: Acquired, pushing the received buffer... ");
	recvfifo.value().push( buffer );
	//sipdebug( "BNB: Pushed, releasing the receive queue..." );
	//mbsize = recvfifo.value().size() / 1048576;
}

void	CDelayServer::pushMessageIntoWaitingQueue( const uint8 *buffer, uint32 size )
{
	//sipdebug( "BNB: Acquiring the receive queue... ");
	CFifoAccessor waitfifo( &_WaitingFifo );
	//sipdebug( "BNB: Acquired, pushing the received buffer... ");
	waitfifo.value().push( buffer, size );
	//sipdebug( "BNB: Pushed, releasing the receive queue..." );
	//mbsize = recvfifo.value().size() / 1048576;
}

void	CDelayServer::PopMessageFromWaitingQueue(CInetAddress &addr, uint32 &time)
{
	std::vector<uint8> buffer;
	// Server: sockid + time
	buffer.resize( sizeof(CInetAddress) + sizeof(uint32) );

    {
		CFifoAccessor waitfifo( &_WaitingFifo );
		sipassert( ! waitfifo.value().empty() );
		waitfifo.value().front( buffer );
		waitfifo.value().pop();
	}

	//addr = *((CInetAddress *)&(*buffer.begin()));

	const uint8 lenIPAddr = 16;
	char hostname[lenIPAddr];
	memcpy( hostname, &*buffer.begin(), lenIPAddr);
	
	uint16	port;
#ifdef SIP_BIG_ENDIAN
    port = SIPBASE_BSWAP16(*(uint16*)&(*(buffer.begin() + lenIPAddr)));
    time = SIPBASE_BSWAP32(*(uint32*)&(*(buffer.begin() + sizeof(uint16))));
#else
	port = *(uint16*)&(*(buffer.begin() + lenIPAddr));
	time = *(uint32*)&(*(buffer.begin() + lenIPAddr + sizeof(uint16)));
#endif

	addr = CInetAddress(hostname, port);
}


}// namespace SIPNET
