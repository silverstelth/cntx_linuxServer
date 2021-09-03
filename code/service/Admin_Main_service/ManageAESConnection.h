
#ifndef ADMIN_MANAGE_AES_H
#define ADMIN_MANAGE_AES_H

#include "admin_main_service.h"

// AES의 Connection정보
struct	AES_CONNECTION_INFO
{
	AES_CONNECTION_INFO() {};
	AES_CONNECTION_INFO(SIPNET::TServiceId _sid)	:
		m_ServiceID(_sid)	{	m_ConnectionTime = SIPBASE::CTime::getLocalTime(); }
		
	SIPNET::TServiceId		m_ServiceID;			// ServiceID;
	SIPBASE::TTime			m_ConnectionTime;		// 접속한 시간
};

typedef		std::map<uint16, SERVICE_ONLINE_INFO>			MAP_ONLINESERVICE;

// 인증을 거친 AES
struct	ONLINE_AES
{
	ONLINE_AES() {};
	ONLINE_AES(uint32 _uID, SIPNET::TServiceId _sid, bool _bRegister=true) :
		m_bRegister(_bRegister), m_uID(_uID), m_ConnectionInfo(AES_CONNECTION_INFO(_sid)), m_uLastPing(0)	{}
	bool					m_bRegister;
	uint32					m_uID;				// DB RegisterID
	AES_CONNECTION_INFO		m_ConnectionInfo;	// Connection Info
	MAP_ONLINESERVICE		m_OnlineService;	// Online Service
	uint32					m_uLastPing;		// time in seconds of the last ping sent. If 0, means that the service already pong

	bool	AddOnlineService(uint16 _uOnID, std::string _sShortName);

	//uint32	AddOnlineService(uint32 _uOnID, std::string _sShortName);
	uint32	DelOnlineService(uint16 _uOnID);
	SERVICE_ONLINE_INFO& GetOnlineService(uint16 _uSvcID);
	bool	IsOnlineRegService(uint32 _uID);
	uint32	GetServiceIDFromDBID(uint32 _uID);
	uint32	GetRegIDFromSvcID(uint16 _uID);
	bool	IsOnlineMstService(uint32 _uMstID);
	bool	IsExistOnlineServiceOfShard(uint16 _uSvcID, uint32 _uMstID, uint32 _uShardID);

	uint32		GetShardIDOfHost();
	ucstring	GetRegShardNameOfHost();
	ucstring	GetRegZoneNameOfHost();
	ucstring	GetRegZoneNameOfService(uint16 _uSvcID);
	ucstring	GetRegShardNameOfService(uint16 _uSvcID);
	std::string	GetRegHostNameOfService(uint16 _uSvcID);
	std::string	GetMSTServiceShortNameOfService(uint16 _uSvcID);
	std::string	GetRegHostName();

	void serial (SIPBASE::IStream &s)
	{
		s.serial(m_bRegister);
		s.serial(m_uID);
		uint16	svcId = m_ConnectionInfo.m_ServiceID.get();
		s.serial(svcId);
	}
};
typedef std::map<SIPNET::TServiceId, ONLINE_AES>			MAP_ONLINE_AES;
extern	MAP_ONLINE_AES								map_Online_AES;		// 인증된 AES Connection

#define	FOR_START_MAP_ONLINE_AES(it)											\
	MAP_ONLINE_AES::iterator	it;												\
	for(it = map_Online_AES.begin(); it != map_Online_AES.end(); it++)			\


extern	uint32		SendMessageToAES(ucstring _zone, ucstring _shard, std::string _host, const SIPNET::CMessage *_pMes);
extern	uint32		GetSIdOfService(uint32 _uID);
extern	ucstring	GetShardNameOfAES(const SIPNET::TServiceId &_sid);
extern	std::string	GetHostNameOfAES(const SIPNET::TServiceId &_sid);
extern	SIPNET::TServiceId	GetAESIDOfHostName(std::string _sHostName);

extern	void		Make_ONLINEINFO(SIPNET::CMessage *pMes);

extern	void		InitManageAESConnection();
extern	void		UpdateManageAESConnection();

extern	SIPBASE::CVariable<uint16> ManagePort;
extern	SIPBASE::CVariable<uint16> MonitorPort;

#endif // ADMIN_MANAGE_AES_H

