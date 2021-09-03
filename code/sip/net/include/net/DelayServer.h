/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef		__DELAYSERVER_H__
#define		__DELAYSERVER_H__

#include "net/SecurityServer.h"
#include "net/UDPPeerServer.h"
#include "net/IProtocolServer.h"
#include "net/unified_network.h"

namespace	SIPNET
{

class	CUDPPeerServer;
class	TServiceId;

typedef void (*TUDPCallback) ( CMessage& msgin, CInetAddress& peeraddr, CUDPPeerServer *peer );

class	CRQObserver
{
public:
	bool	IsBlocking();

	CRQObserver();
	CRQObserver(TServiceId sid, std::string serviceName, CInetAddress & listenAddr, CInetAddress & publicAddr, uint8 nState=0, uint32 nbConnections=0, uint32 procSpeed=0, uint64 sizeRQ=0);
	CRQObserver(const CRQObserver &other);
	~CRQObserver();

	CRQObserver & operator=(const CRQObserver & rqObserver);

	void		modifyRQState(CRQObserver &	rqObserver);

public:
	TServiceId	getServiceID() { return _sid; };
	void		setServiceID(TServiceId	 sid) { _sid = sid; };

	std::string	getServiceName() { return _serviceName; };
	void		getServiceName(std::string serviceName) { _serviceName = serviceName; };

	uint8		getState() { return _nState; };
	void		setState(uint8	state) { _nState = state; };

	uint64		getSizeRQ() { return _SizeRQ; };
	void		setSizeRQ(uint64 size) { _SizeRQ = size; };

	uint32		getNotifyTimeGap() { return _NotifyTimeGap; };
	void		setNotifyTimeGap(uint32	time) { _NotifyTimeGap = time; };

	uint32		QueueIncomingConnections() const { return _RQConnections; };
	void		setQueueIncomingConnections(uint32 nConnections) { _RQConnections = nConnections; };

	uint32		CalcTotalProcTimeMS() const 
	{ 
		if (_ProcSpeedPerMS > 0)
			return (uint32)(_SizeRQ / _ProcSpeedPerMS);
		return (uint32)(_SizeRQ / 1000);
	};

	void		setCycleTimeout(sint32	timeout) { _CycleTimeout = timeout; };
	sint32		CycleTimeout() { return		_CycleTimeout; };

	void		setListenAddr(CInetAddress & addr) { _listenAddr = addr; };
	CInetAddress	&getListenAddr() { return _listenAddr; };

	CInetAddress	&getPublicAddr() { return _publicAddr; };

	bool		isBusyRecieveQueueState(uint32 &nbWaitingUser);

protected:
	TServiceId		_sid;
	std::string		_serviceName;
	CInetAddress	_listenAddr;
	CInetAddress	_publicAddr;

	uint8			_nState;
	uint64			_SizeRQ;
	uint32			_RQConnections;
	uint32			_ProcSpeedPerMS;

	sint32			_CycleTimeout;
    uint32			_NotifyTimeGap;
};

enum	_DELAYCONNECTRESULT { _WAIT, _CONNECT, _REFUSE };
class CDelayServer : public IProtocolServer
{
/*****************************************************************/
// Pre Connection Handshake
/*****************************************************************/

public:
	void	init(uint16 nUDPPort);
	void	update ( sint32 timeout = 0 );
	void	sendTo(CMessage	&msg, CInetAddress & addr);

	void	addCallbackArray (const TUDPCallbackItem *callbackarray, sint arraysize);
	void	notifyConnectToClient(TServiceId	serviceId, SIPNET::CInetAddress& peeraddr);
	void	notifyWaitToClient(TServiceId	serviceId, CInetAddress& peeraddr, uint32 nbWaitUser, uint32 timeInterval);

	bool	registerRQObserver(CRQObserver slave);
	bool	unregisterRQObserver(TServiceId serviceId);
	void	notifyRQState(CRQObserver &slave);
	//void	notifyRQState(TServiceId sid, std::string serviceName, uint8 nState, uint32 nbPlayers, uint32 procTime, uint32 sizeRQ);

public:
	bool	isSlaveEmpty();
	void	displayWaitingQueueStat (SIPBASE::CLog *log = SIPBASE::InfoLog)
	{
		SIPBASE::CFifoAccessor syncfifo( &_WaitingFifo );
		syncfifo.value().displayStats(log);
	}

	CUDPPeerServer	*UDPServer() { return	_UDPServer; };
	bool	IsConnectDelayMode() { return _bConnectDelay; };
	SIPBASE::CSynchronizedFIFO&	waitingQueue() { return _WaitingFifo; };

	_DELAYCONNECTRESULT	calcQueueState(uint32 &nbWaitingUser, uint32 &timeInterval, TServiceId &serviceId);
	void	pushMessageIntoWaitingQueue( const std::vector<uint8>& buffer );
	void	pushMessageIntoWaitingQueue( const uint8 *buffer, uint32 size );

	CInetAddress &	getSlaveListenAddress(TServiceId serviceId);
	CInetAddress &	getSlavePublicAddress(TServiceId serviceId);
	CRQObserver	&	getSlaveService(TServiceId serviceId);

	uint32	getWaitingQueueSize()
	{
		SIPBASE::CFifoAccessor syncfifo( &_WaitingFifo );
		return	syncfifo.value().size();
	}
	uint32	getWaitingQueueFIFOSize()
	{
		SIPBASE::CFifoAccessor syncfifo( &_WaitingFifo );
		return	syncfifo.value().fifoSize();
	}

protected:
	void	PopMessageFromWaitingQueue(CInetAddress &addr, uint32 &time);

public:
	CDelayServer();
	virtual ~CDelayServer(void);

public:
	typedef	std::map<CInetAddress, uint32>	TWAITINGS;
	TWAITINGS		_Waitings;
	typedef	std::map<CInetAddress, uint32>::iterator	ITWAITINGS;

protected:
	bool				_bConnectDelay;

	SIPBASE::CSynchronizedFIFO	_WaitingFifo;
	CUDPPeerServer		*_UDPServer;

	typedef		std::map<uint16, CRQObserver>				_ARYSLAVES;
	typedef		std::map<uint16, CRQObserver>::iterator		_ITARYSLAVES;
	_ARYSLAVES			_mapSlaves;
};

}// namespace SIPNET

extern		SIPBASE::CVariable<unsigned int>	NotifyInterval;
extern		SIPBASE::CVariable<unsigned int>	CycleInterval;
extern		SIPBASE::CVariable<unsigned int>	limitProcTime;
extern		SIPBASE::CVariable<unsigned int>	limitIncomingConnections;
extern		SIPBASE::CVariable<unsigned int>	limitRecieveQueueSize;
extern		SIPBASE::CVariable<unsigned int>	limitWaitingQueueSize;
extern		SIPBASE::CVariable<unsigned int>	limitNotifyInterval;

#endif	//__DELAYSERVER_H__
