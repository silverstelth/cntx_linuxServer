#ifndef SIP_START_SERVICE_H
#define SIP_START_SERVICE_H

// now put off;
struct WaitingUser
{
    
};

enum ENUM_PROTOCOL{ SERVICE_NON, SERVICE_HTTP, SERVICE_FTP, SERVICE_TCPIP };

class CUriPath
{
public:
	ENUM_PROTOCOL	getProtocolService() { return m_protocol; };
	std::string		getServerAddr() { return m_strAddr; };
	std::string		getFileDir() { return m_strFileDir; };

	CUriPath(std::string uriPath);
	CUriPath(ENUM_PROTOCOL service, std::string strAddr, std::string strFileDir)
	{
		m_protocol = service;
		m_strAddr = strAddr;
		m_strURI  = strFileDir;
	}

	CUriPath() {};
	~CUriPath() {};

protected:
	void	setProtocolService(std::string strService);
	void	setServerAddr(std::string strAddr)
	{
		m_strAddr = strAddr;
	}
	void	setFileDir(std::string strFileDir)
	{
		m_strFileDir = strFileDir;
	}

private:
	std::string			m_strURI;
	ENUM_PROTOCOL		m_protocol;
	std::string			m_strFileDir;
	std::string			m_strAddr;
};

typedef		std::multimap<uint32, uint32>			TONLINEGMS;
typedef		std::multimap<uint32, uint32>::iterator	TONLINEGMSIT;
extern TONLINEGMS			g_onlineGMs;


// Variables
//extern CLog*				Output;
extern	SIPBASE::CDBCaller		*MainDB;
extern	SIPBASE::CDBCaller		*UserDB;
//extern	DBCONNECTION	MainDBConnection;
//extern	DBCONNECTION	UserDBConnection;

// Functions
extern	SIPBASE::CVariable<std::string>	SSLoginHost;
extern	SIPBASE::CVariable<uint16>	SSDelayPort;
extern	SIPBASE::CVariable<uint16>	SSLoginPort;
extern	SIPBASE::CVariable<uint16>	SSMasterPort;

extern	SIPBASE::CVariable<unsigned int>	TestDelay;
extern	uint	QueryConnections;

typedef	std::multimap<uint32, std::string>				TGMADDRS;
typedef	std::multimap<uint32, std::string>::iterator	TGMADDRSIT;
extern	TGMADDRS		tblGMAddrs;

extern	T_ERR	DBAuthUser( ucstring ucLogin, std::string sPassword, SIPNET::TTcpSockId from, SIPNET::CCallbackNetBase& clientcb, uint32 *pdbid, uint32 *pshardid, uint8 *puserTypeId );
extern	bool	AcceptInvalidCookie;

#endif // SIP_START_SERVICE_H

/* End of login_service.h */
