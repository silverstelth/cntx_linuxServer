#ifndef	_MASTERSERVER_H_
#define	_MASTERSERVER_H_

#include	"net/DelayServer.h"

class	CSlaveService;
class	CMasterService
{
public:
	CMasterService(SIPNET::CInetAddress masterAddrForClient);
	~CMasterService();
	void	update(sint32 timeout = 0);
	void	update2( sint32 timeout = -1, sint32 mintime = 0);
	void	release();

	void	send (const SIPNET::CMessage &buffer, SIPNET::TTcpSockId hostid = SIPNET::InvalidSockId, uint8 bySendType = REL_GOOD_ORDER);
	void	receive (SIPNET::CMessage &buffer, SIPNET::TSockId *hostid);

	void	insertWTQueue();
	void	eraseWTQueue();

	uint32	WQSize();


	SIPNET::CDelayServer*	Server() { return _server; };
	SIPNET::CInetAddress	&clientListenAddr() { return	_clientListenAddr; };
	void	setClientListenAddr(SIPNET::CInetAddress & addr) { _clientListenAddr = addr; };
	static 	SIPNET::TServiceId		ownSlaveId() { return SIPNET::TServiceId(0xFFFF); };
	static 	std::string				ownSlaveName() { return SS_S_NAME; };

protected:
	SIPNET::CDelayServer	*_server;
	SIPNET::CInetAddress	_clientListenAddr;
};

extern CMasterService * MasterService;

#endif	//_MASTERSERVER_H_