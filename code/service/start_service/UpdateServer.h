#ifndef __UPDATE_SERVER__
#define __UPDATE_SERVER__

#include	"../Common/DBData.h"
#include	"../../common/Macro.h"
#include "IPData.h"
extern uint32	GetNetworkPart(const char* strIP);

struct  IPMnt
{
	IPMnt(std::string _ip, uint32 _num, uint32 _mnt) : strIP(_ip), uNumber(_num), uMnt(_mnt) {};
	std::string		strIP;
	uint32			uNumber;
	uint32			uMnt;
};

struct	CPatchServerInfo
{
	CPatchServerInfo():uOnlineUsers(0), uNetworkPart(NETWORK_UNKNOWN) {};
	CPatchServerInfo(_entryUpdateServer	&uri, uint16 nUsers) :ServerInfo(uri), uOnlineUsers(nUsers)
	{
		SetNetworkPartFromUrl(uri.sUri);
	};

	CPatchServerInfo(uint8 ServerID, std::string sAddr, uint32 port, std::string sUserID, std::string sPassword, ucstring ucAliasName, uint16 nOnlineUsers)
	{
			ServerInfo.ServerID		= ServerID;
			ServerInfo.sUri			= sAddr;
			ServerInfo.port			= port;
			ServerInfo.sUserID		= sUserID;
			ServerInfo.sPassword	= sPassword;
			ServerInfo.ucAliasName	= ucAliasName;
			nOnlineUsers			= nOnlineUsers;
			SetNetworkPartFromUrl(sAddr);
	};
	
	void	SetNetworkPartFromUrl(std::string sAddr)
	{
		uNetworkPart = NETWORK_UNKNOWN;
		char	strIPAddress[MAX_LEN_FTPURI_SERVERURI];
		bool	bIPAddress = CIPAddress::GetIPAddressFromHttpURL(sAddr.c_str(), strIPAddress);
		if (bIPAddress)
			uNetworkPart = GetNetworkPart(strIPAddress);
	}

	_entryUpdateServer	ServerInfo;
	uint			uOnlineUsers;
	uint32			uNetworkPart;
};

struct	InfoProduct
{
	ucstring		ucProduct;
	std::string		sNewVersion;
};

#define	MAX_INITXT_LEN	256

void	fDownloadNotify(float fProgress, void* pParam, DWORD dwNowSize, void* pMemoryPtr);

//class CFtpDown;

class CUpdateServer
{
public:
	static BOOL		init(SIPNET::CCallbackServer * server);
	static BOOL		init(SIPNET::CDelayServer * server);
	static void		release();
	static void		update();
	static T_ERR	chooseNewUpdatePatchServer(ucstring ucProductName, std::string sNewVersionNo, uint *pNewServerNo, uint32 nNetworkPart);
	static T_ERR	chooseUpdatePatchServer(ucstring ucProductName, std::string sVersionNo, uint *pServerNo, uint32 nNetworkPart);

	static BOOL		LoadDBUpdateInfo();
	static BOOL		LoadIPMnt();
	CUpdateServer();
	~CUpdateServer();

protected:

private:
	static BOOL		m_bInit;
};

#endif // __UPDATE_SERVER__
