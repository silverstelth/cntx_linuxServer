#ifndef	_SLAVESERVER_H_
#define _SLAVESERVER_H_

class	CSlaveService
{
public:
	// this server is alone slave service
	bool	init(SIPNET::CInetAddress slaveAddrForClient, SIPNET::CInetAddress masterServiceAddr, SIPNET::CInetAddress slavePublicAddr, bool bIsMaster);
	// this server exists with master service
//	bool	init(uint nPort);
//	bool	init(SIPNET::CInetAddress addr);

	void	update2( sint32 timeout = -1, sint32 mintime = 0);

	void	release();

	void	send (const SIPNET::CMessage &buffer, SIPNET::TTcpSockId hostid = SIPNET::InvalidSockId, uint8 bySendType = REL_GOOD_ORDER);

	SIPNET::CCallbackServer	* Server() { return _server; };
	bool	IsBlocking();

	SIPNET::CInetAddress	&ListenAddr() { return	_listenAddr; };
	SIPNET::CInetAddress	&PublicAddr() { return	_publicAddr; };
//	SIPNET::TServiceId		ServiceId() { return _sid; };
//	std::string				ServiceName() { return _serviceName; };

	uint8			state()			{ return _nState; };

	bool		isLocalWithMaster()		{ return _bLocalSlaveWithMaster; };

	uint32		GetReceiveQueueItem() { return (uint32)(_server->getReceiveQueueItem()); }
	uint32		GetRQConnections() { return (_server->QueueIncomingConnections() + _server->getConnectedNum()); }
	uint32		GetProcSpeedPerMS() { return (uint32)_server->getProcMessageNumPerMS(); }
public:
	CSlaveService();
	CSlaveService(SIPNET::TServiceId sid, std::string serviceName, uint8 nState, uint32 nbPlayers, uint32 procTime, uint32 sizeRQ, uint32 activeConnections, uint32 allConnections, uint32 allDisconnections);

	~CSlaveService();


	SIPNET::TServiceId		_masterSid;
protected:
//	SIPNET::TServiceId		_sid;
//	std::string				_serviceName;
	SIPNET::CInetAddress	_listenAddr;
	SIPNET::CInetAddress	_publicAddr;

	uint8			_nState;

	bool			_bLocalSlaveWithMaster;

protected:
	SIPNET::CCallbackServer		*_server;
};

struct CPlayer
{
	CPlayer(){}

	CPlayer(uint32 Id, uint32 fId, SIPNET::TTcpSockId Con, uint8 usertypeid_in, uint32 lastShardid_in, ucstring ucUser_in, std::string sPassword_in, std::vector<uint32> _ShardList, bool bGM_in = false, uint32 _gen_time = 0 )
		: id(Id), fid(fId), con(Con), usertypeid(usertypeid_in), lastShardid(lastShardid_in), ucUser(ucUser_in), sPassword(sPassword_in), ShardList(_ShardList), bGM(bGM_in), gen_time(_gen_time) { }

	CPlayer(const CPlayer & player)
	{
		id			= player.id;
		fid			= player.fid;
		con			= player.con;
		usertypeid	= player.usertypeid;
		lastShardid	= player.lastShardid;
		ucUser		= player.ucUser;
		sPassword	= player.sPassword;
		ShardList = player.ShardList;
		bGM			= player.bGM;

		gen_time	= player.gen_time;
	}

	CPlayer & operator= ( const CPlayer & player)
	{
		id			= player.id;
		fid			= player.fid;
		con			= player.con;
		usertypeid	= player.usertypeid;
		lastShardid	= player.lastShardid;
		ucUser		= player.ucUser;
		sPassword	= player.sPassword;
		ShardList = player.ShardList;
		bGM			= player.bGM;

		gen_time	= player.gen_time;

        return *this;
	}

	uint32		id;
	uint32		fid;
	SIPNET::TTcpSockId  con;
	uint8		usertypeid;
	uint32		lastShardid;
	ucstring	ucUser;
	std::string	sPassword;
	std::vector<uint32> ShardList;
	bool		bGM;

	uint32		gen_time;
};

typedef std::map<uint32, CPlayer> _pmap;
extern _pmap	ConnectPlayers;
extern _pmap    TempPlayerInfo;

extern CSlaveService * SlaveService;

extern void DisconnectUser(SIPNET::CCallbackNetBase& clientcb, SIPNET::TTcpSockId from, uint32	userid);
// ADD_KSR 2010_2_18
extern void ForceDisconnectUser(SIPNET::CCallbackNetBase& clientcb, SIPNET::TTcpSockId from, uint32	userid);

extern T_ERR checkPendingUserConnection( SIPNET::TTcpSockId from, SIPNET::CCallbackNetBase& clientcb, uint32 userid );

extern void	insertCookieInfo(SIPNET::CInetAddress udpAddr, bool bGM = false);
extern void	eraseCookieInfo(SIPNET::CInetAddress udpAddr);

extern bool	isGMType(uint8	userType);
extern	SIPBASE::CVariable<unsigned int>	BoundIncommingConnections;
extern	bool	g_bLock;

typedef std::map<uint32, std::vector<uint32> >  TUSERSHARDROOM;
typedef std::map<uint32, std::vector<uint32> >::iterator  TUSERSHARDROOMIT;
//Textern  TUSERSHARDROOM tblShardVSUser;



#endif	// _SLAVESERVER_H_
