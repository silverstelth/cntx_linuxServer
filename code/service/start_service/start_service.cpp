
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef SIPSS_CONFIG
#define SIPSS_CONFIG ""
#endif // SIPNS_CONFIG

#ifndef SIPSS_LOGS
#define SIPSS_LOGS ""
#endif // SIPNS_LOGS

#ifndef SIPSS_STATE
#define SIPSS_STATE ""
#endif // SIPNS_STATE

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

#include 	"../../common/common.h"
#include 	"../../common/err.h"
#include	"../../common/Packet.h"
#include	"../../common/Macro.h"
#include 	"../Common/netCommon.h"
#include	"../Common/DBData.h"
#include 	"../Common/QuerySetForMain.h"

#include	"misc/DBCaller.h"

#include	"slaveService.h"
#include	"masterService.h"
#include 	"start_service.h"
#include 	"UpdateServer.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

//
// Variables
//
SIPBASE::CLog	*Output = NULL;
CDBCaller		*MainDB = NULL;
CDBCaller		*UserDB = NULL;

bool	AcceptInvalidCookie = false;

CVariable<std::string>	SSMasterIPForClient("SS", "SSMasterIPForClient", "Host IP for client connection", "", 0, true);
CVariable<uint16>	SSMasterPortForClient("SS", "SSMasterPortForClient", "UDP Port of Delay SS for Clients", 50000, 0, true);

CVariable<std::string>	SSSlaveIPForClient("SS", "SSSlaveIPForClient", "Host IP for master connection", "", 0, true);
CVariable<uint16>	SSSlavePortForClient("SS", "SSSlavePortForClient", "TCP Port of SS for master", 50001, 0, true);

CVariable<std::string>	SSMasterIPForSlave("SS", "SSMasterIPForSlave", "Host IP of slave connection", "", 0, true);
CVariable<uint16>	SSServicePortForSS("SS", "SSServicePortForSS", "Service Port for all start_service", 50002, 0, true);

CVariable<std::string>	SSSlaveIPForPublic("SS", "SSSlaveIPForPublic", "Host IP for client connection", "", 0, true);
CVariable<uint16>	SSSlavePortForPublic("SS", "SSSlavePortForPublic", "TCP Port of SS for Clients", 0, 0, true);

struct	TZone
{
	TZone(uint8 _id, ucstring _name)
	{
		zoneId	= _id;
		name	= _name;
	};

	uint8		zoneId;
	ucstring	name;
};

typedef vector<TZone>							TZONEARY;

//TZONEARY		tblZones;
TGMADDRS		tblGMAddrs;

TONLINEGMS		g_onlineGMs;

//static std::vector<_entryShardNew> ShardsInfo;

/**************************************************************************/
/*									class	CURI						  */
/**************************************************************************/

CUriPath::CUriPath(string uriPath)
{
	string	strPath = uriPath;
	m_strURI		= uriPath;
	size_t pos = m_strURI.find_first_of("://");
	if (pos != string::npos)
	{
		setProtocolService(m_strURI.substr(0, pos));
		strPath = strPath.substr(pos + 3);
	}

	pos = strPath.find_first_of('/');
	if (pos != string::npos)
	{
		setServerAddr(strPath.substr(0, pos));
		strPath = strPath.substr(pos + 1);
	}

	setFileDir(strPath);
}

void	CUriPath::setProtocolService(string strService)
{
	if ( strService == "ftp" || strService == "FTP" )
	{
		m_protocol = SERVICE_FTP;
		return;
	}

	if ( strService == "http" || strService == "HTTP" )
	{
		m_protocol = SERVICE_HTTP;
		return;
	}

	m_protocol = SERVICE_NON;
}

// packet : M_ENTER_SHARD_ANSWER
// LS -> SS
void cbLSShardChooseShard(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	WORD			wErrNo;
	string			sWSAddr;
	CLoginCookie	cookie;
	TTcpSockId		UserSock;

	CMessage		msgout(M_ENTER_SHARD_ANSWER);
	msgin.serial(wErrNo);
	msgout.serial(wErrNo);
	msgin.serial(cookie);
	
	sipdebug("Try to enter Shard result! result=%d, cookie=%s", wErrNo, cookie.toString().c_str()); // byy krs

	UserSock = reinterpret_cast<TSockId> (cookie.getUserAddr());
	T_ERR nerr = checkPendingUserConnection(UserSock, *(SlaveService->Server()), cookie.getUserId());
	if ( nerr != ERR_NOERR )
	{
		sipwarning("User(%u) is disconnected, while return from shard, so can't send replay Message <SCS>", cookie.getUserId());
		return;
	}

	if ( wErrNo == ERR_NOERR )
	{
		msgout.serial(cookie);
		msgin.serial(sWSAddr);	msgout.serial(sWSAddr);
//		sipinfo("Try to enter Shard SUCCESS! cookie %s, FESAddress %s", cookie.toString().c_str(), sWSAddr.c_str());
	}
	else
	{
		sipwarning("Try to Enter Shard Failed! errNo %u", wErrNo);
	}

	if ( SlaveService != NULL ) 
		SlaveService->send(msgout, UserSock);
//	sipinfo("CS Old Socket:%s", UserSock->asString().c_str());

	/*
	if ( wErrNo == ERR_NOERR )
	{
        DisconnectUser((CCallbackNetBase &) *LoginServer, UserSock, cookie.getUserId());
		sipinfo("disconnectUser : userAddress %s, cookie %s", UserSock->asString().c_str(), cookie.toString().c_str());
	}
	*/
}

// Meaningless!
// packet : M_CLIENT_CONNECT_STATUS
// LS -> SS
void cbLSClientConnected(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint32	userid;				msgin.serial(userid);
	uint8	con;				msgin.serial(con);
	if( con == 1 ) // Connect
	{
		// Connect User
		CLoginCookie	cookie;		msgin.serial(cookie);
		uint8	userType	= cookie.getUserType();
		uint8	bIsGM		= cookie.getIsGM();
	// 통합써버기능: GM다중로그인기능 삭제
		//if ( bIsGM && isGMType(userType) )
		//{
		//	sipdebug("User id:%u is GAMEMANAGER, So never disconnect", userid);
		//	return;
		//}

		_pmap::iterator info = ConnectPlayers.find(userid);
		if(info != ConnectPlayers.end())
		{
			TTcpSockId	from = info->second.con;

			// ADD_KSR 2010_2_18
			//DisconnectUser((CCallbackNetBase &) *(SlaveService->Server()), from, userid);
			ForceDisconnectUser((CCallbackNetBase &) *(SlaveService->Server()), from, userid);

			// comment blow statement, because when client actively close the connection, 
			// the TcpSockect object to which 'from' point should be deleted by CServerReceiveTask::clearClosedConnections() 
			// in back-ground thread, after this, 'from' is invalid point

			//sipinfo("User(userid %u) connected to FES, disconnect from SS", userid);				 byy krs
		}

		// delete from the user's shard list where he has room
//T		TUSERSHARDROOMIT itShard = tblShardVSUser.find(userid);
//T		if ( itShard != tblShardVSUser.end() )
//T		{
//T			tblShardVSUser.erase(itShard);
//T		}
	}
	else // Disconnect
	{
		// Disconnect User
		// Connect User
/*		_pmap::iterator	info = ConnectPlayers.find(userid);
		if(info != ConnectPlayers.end())
			ConnectPlayers.erase(info);
		TTcpSockId		sockid = info->second.con;

		CMessage	msgout("CC");
		msgout.serial(userid);
		LoginServer->send(msgout, sockid);
*/
		uint8	bIsGM;			msgin.serial(bIsGM);
		uint32	shardId;		msgin.serial(shardId);

		uint32	nCount = static_cast<uint32>(g_onlineGMs.count(userid));
		if ( nCount == 0)
			return;

		std::pair<TONLINEGMSIT, TONLINEGMSIT>	range = g_onlineGMs.equal_range(userid);
		for ( TONLINEGMSIT it = range.first; it != range.second; it ++ )
		{
			uint32	idshard = (*it).second;
			if ( idshard == shardId )
			{
				g_onlineGMs.erase(it);
				sipinfo("userId %u logoff to shard %u", userid, idshard);
				break;
			}
		}
	}
}

// Packet :M_SC_FAIL
// LS -> SS
void cbLSFailed(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint16	errNo;	msgin.serial(errNo);
	uint32	userid;	msgin.serial(userid);

	sipwarning("Send Error to Client. UID=%d, Eror=%d", userid, errNo);
	_pmap::iterator pUserIt = ConnectPlayers.find(userid);
	if (pUserIt != ConnectPlayers.end())
	{
		TTcpSockId	from = pUserIt->second.con;

		SendFailMessageToClient(errNo, from, *(SlaveService->Server()));
	}
	g_onlineGMs.erase(userid);
}

// Packet : M_SC_SHARDS
// LS -> SS
void cbLSDisplayShardListNew(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint32	userid, mainShardId;
	TTcpSockId	from;
	uint8 usertypeId;
	uint32 lastSharId;
	uint32 ShardCount = 0;

	msgin.serial(userid, mainShardId);
	_pmap::const_iterator pUserIt = ConnectPlayers.find(userid);
	if ( pUserIt == ConnectPlayers.end() )
	{
		sipwarning("UID=%u is disconnected, while return from shard, so can't send replay Message <SHARDS>", userid);
		return;
	}

	from = pUserIt->second.con;
	string	sClientIP = from->getTcpSock()->remoteAddr().ipAddress();
	uint32	nClientNetworkPart = GetNetworkPart(sClientIP.c_str());

	usertypeId = pUserIt->second.usertypeid;
	lastSharId = pUserIt->second.lastShardid;

	CMessage	msgout(M_SC_SHARDS);

	msgin.serial(ShardCount);
	msgout.serial(userid, mainShardId);
	msgout.serial(lastSharId);					//For LastShardId
	msgout.serial(nClientNetworkPart);			//For Client NetworkPartID
	msgout.serial(ShardCount);					//For the number of Shard
	for (uint32 i = 0; i < ShardCount; i++)
	{
		uint32			TempShardId = 0;      msgin.serial(TempShardId);
		ucstring		strNameTemp = "";     msgin.serial(strNameTemp);
		uint8           TempActiveFlag = 0;   msgin.serial(TempActiveFlag);
		uint8           stateId = 0;          msgin.serial(stateId);
		ucstring		ShardCodeList = "";     msgin.serial(ShardCodeList);
		string			activeShardIP = "";		msgin.serial(activeShardIP);
		uint8           isDefaultShard = 0;		msgin.serial(isDefaultShard);

		uint32	hasRoomFlag  = 0;
		std::vector<uint32> Temp = pUserIt->second.ShardList;
		if( find(Temp.begin(), Temp.end(), TempShardId) != Temp.end() )
		{
			hasRoomFlag = 1;
		}
		msgout.serial(TempShardId);					// For ShardId
		msgout.serial(strNameTemp);			    // For shard name
		msgout.serial(hasRoomFlag);		        // For has space or not
		msgout.serial(TempActiveFlag);			// For active flag: 0 for maintenance state,1 for use
		msgout.serial(stateId);				// For state information		
		msgout.serial(ShardCodeList);
		msgout.serial(isDefaultShard);
		uint32	nShardNetworkPart = GetNetworkPart(activeShardIP.c_str());
		msgout.serial(nShardNetworkPart);		// For NetworkPartID
	}

	// 2020/05/30 방관련통지를 위한 처리
	uint32	flagNotics = 0;
	{
		// MainServer에 로그인되여있지만 Shard에 들어가있지 않은 상태인지 검사한다.
		MAKEQUERY_Temp_CheckNoticeUser(strQuery, userid);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (!nQueryResult)
		{
			sipwarning("Main_temp_CheckNoticeUser exec failed.");
			return;
		}

		DB_QUERY_RESULT_FETCH(Stmt, row);
		
		if (!IS_DB_ROW_VALID(row))		
			flagNotics = 1;
		
	}
	msgout.serial(flagNotics);

	SlaveService->send(msgout, from);
}

void cb_ToUserID(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint32	UID;
	CMessage	msgOut;
	msgin.copyWithoutPrefix(UID, msgOut);

	_pmap::const_iterator pUserIt = ConnectPlayers.find(UID);
	if ( pUserIt == ConnectPlayers.end() )
	{
		sipwarning("Can't send packet because UID is not in ConnectPlayers. UID=%d, Packet=%d", UID, msgOut.getName());
		return;
	}

	TTcpSockId from = pUserIt->second.con;

	SlaveService->send(msgOut, from);
}

/****************************************************************************************************************************
	LS - SS Connection
****************************************************************************************************************************/

void cb_M_NT_REQ_FORCE_LOGOFF(CMessage &msgin, const std::string &serviceName, TServiceId sid);
void cb_M_SC_CHECK_FASTENTERROOM(CMessage &msgin, const std::string &serviceName, TServiceId sid);
TUnifiedCallbackItem LSCallbackArray[] = 
{
	{M_SC_FAIL,				cbLSFailed},
	{M_ENTER_SHARD_ANSWER,	cbLSShardChooseShard},
	{M_SC_CHECK_FASTENTERROOM,	cb_M_SC_CHECK_FASTENTERROOM},
	{M_CLIENT_CONNECT_STATUS,		cbLSClientConnected},
	{M_SC_SHARDS,			cbLSDisplayShardListNew},
	{M_NT_REQ_FORCE_LOGOFF,		cb_M_NT_REQ_FORCE_LOGOFF},

	{M_SC_FRIEND_GROUP,		cb_ToUserID},
	{M_SC_FRIEND_LIST,		cb_ToUserID},

	{M_SC_FAVORROOM_GROUP,		cb_ToUserID},
	{M_SC_FAVORROOM_LIST,		cb_ToUserID},

	{M_SC_AUTOPLAY3_LIST,		cb_ToUserID},
};

static void	DBCB_LoadGMList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if ( !nQueryResult )
		return;

	tblGMAddrs.clear();
	uint32	uRows;	resSet->serial(uRows);
	for( uint32 i = 0; i < uRows; i ++ )
	{
		uint32		GMClientID;		resSet->serial(GMClientID);
		std::string	GMClientAddr;	resSet->serial(GMClientAddr);

		tblGMAddrs.insert(make_pair(GMClientID, GMClientAddr));
	}
}

void LoadGMList()
{
	MAKEQUERY_LOADGMADDR(strQuery);
	MainDB->execute(strQuery, DBCB_LoadGMList);
}

//static void	DBCB_LoadZoneList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	if ( !nQueryResult )
//		return;
//
//	tblZones.clear();
//	uint32	uRows;		resSet->serial(uRows);
//	for ( uint32 i = 0; i < uRows; i ++ )
//	{
//		uint8		zoneId;		resSet->serial(zoneId);
//		ucstring	ucName;		resSet->serial(ucName);
//
//		TZone	zone(zoneId, ucName);
//		tblZones.push_back(zone);
//	};
//
//	sipinfo("Successed Loading Zone List");
//}

//void LoadZoneList()
//{
//	// Load Zone
//	MAKEQUERY_LOADZONE(strQuery);
//	MainDB->execute(strQuery, DBCB_LoadZoneList);
//}

void	DBInit()
{
	string strDatabaseName, strDatabaseHost, strDatabaseLogin, strDatabasePassword;

	// Connect Main DB
	strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Main_DBName").asString ();
	strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Main_DBHost").asString ();
	strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Main_DBLogin").asString ();
	strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Main_DBPassword").asString ();

	MainDB = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
		ucstring(strDatabasePassword).c_str(),
		ucstring(strDatabaseHost).c_str(),
		ucstring(strDatabaseName).c_str());

	BOOL bSuccess = MainDB->init(4);
	if (!bSuccess)
	{
		siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
			strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
		return;
	}
	sipinfo("Main Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());

	// Connect UserDB
	strDatabaseName = IService::getInstance ()->ConfigFile.getVar("User_DBName").asString ();
	strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("User_DBHost").asString ();
	strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("User_DBLogin").asString ();
	strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("User_DBPassword").asString ();

	UserDB = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
		ucstring(strDatabasePassword).c_str(),
		ucstring(strDatabaseHost).c_str(),
		ucstring(strDatabaseName).c_str());
	if ( !UserDB )
	{
		siperrornoex("Init Failed UserDBCaller Memory Allocation Error");
		return;
	}
	bSuccess = UserDB->init(2);

	if (!bSuccess)
	{
		siperrornoex("User Database connection failed to '%s' with login '%s' and database name '%s'",
			strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
	}
	else
	{
		sipinfo("User Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
	}

	//LoadZoneList();
	LoadGMList();

	{ // 90일이전의 메일과 치트자료를 삭제한다. by krs
		MAKEQUERY_Main_Mail_Chit_Delete90days(strQuery);
		CDBConnectionRest	DBConnection(MainDB);
		DB_EXE_QUERY(DBConnection, Stmt, strQuery);
	}
}

void	DBRelease()
{
	MainDB->release();
	delete MainDB;
	MainDB = NULL;

	UserDB->release();
	delete UserDB;
	UserDB = NULL;
}

void cfcbAcceptInvalidCookie(CConfigFile::CVar &var)
{
	// set the new ListenAddr
	AcceptInvalidCookie = var.asInt() == 1;
	sipinfo("SS: This service %saccept invalid cookie", AcceptInvalidCookie?"":"doesn't ");
}

TRegPortItem	PortAry[] =
{
	{"TIANGUO_SSPingPort",	PORT_UDP, SSMasterPortForClient.get(), FILTER_NONE},
	{"TIANGUO_SSLoginPort",	PORT_TCP, SSSlavePortForClient.get(), FILTER_NONE}
};

// Class
void releaseClientServer()
{
	if ( MasterService != NULL )
		delete	MasterService;
	MasterService = NULL;

	if ( SlaveService != NULL )
		delete	SlaveService;
	SlaveService = NULL;
}

// Security : Remove needless Connection
void updateClientServer( sint32 timeout, sint32 mintime)
{
	if ( SlaveService != NULL )
		SlaveService->update2(timeout, mintime);
	if ( MasterService != NULL )
		MasterService->update2(timeout, mintime);
}

class CStartService : public	IService
{
public:
	void preInit()
	{
		setRegPortAry(PortAry, sizeof(PortAry)/sizeof(PortAry[0]));
	}

	void init()
	{
		// DB Connection
		DBInit();

// add a connection to the LS
//////////////////////////////////////////////////////////////////////////
		string LSAddr;
		uint16	uLSPort = 49993;
		if (haveArg('T'))
		{
			LSAddr = getArg('T');			// use the command line param if set
		}
		else if (ConfigFile.exists ("LSHost"))
		{
			LSAddr = ConfigFile.getVar("LSHost").asString();	// use the config file param if set
		}
		// the config file must have a valid address where the login service is
		sipassert(!LSAddr.empty());

		// add default port if not set by the config file
		if (LSAddr.find (":") == string::npos)
		{
			CConfigFile::CVar *var;
			if ((var = ConfigFile.getVarPtr ("LSPort")) != NULL)
				uLSPort = var->asInt ();
			LSAddr += ":" + toStringA(uLSPort);
		}

		if ( ConfigFile.exists("AcceptInvalidCookie"))
            cfcbAcceptInvalidCookie (ConfigFile.getVar("AcceptInvalidCookie"));
		ConfigFile.setCallback("AcceptInvalidCookie", cfcbAcceptInvalidCookie);

		if (ConfigFile.getVarPtr("DontUseLSService") == NULL 
			|| !ConfigFile.getVar("DontUseLSService").asBool())
		{
			// We are using SIP Login Service
			CUnifiedNetwork::getInstance()->addCallbackArray(LSCallbackArray, sizeof(LSCallbackArray)/sizeof(LSCallbackArray[0]));
			CUnifiedNetwork::getInstance()->addService(LS_S_NAME, LSAddr);
		}
		sipinfo("LS Addr:%s Connect Success!", LSAddr.c_str());
//////////////////////////////////////////////////////////////////////////
		vector<CInetAddress> hostaddrs = CInetAddress::localAddresses();
		bool	bHaveMasterIPForClient = false;
		bool	bHaveMasterIPForSlave = false;
		bool	bHaveSlaveIPForClient = false;
		for (uint i = 0; i < hostaddrs.size(); i++)
		{
			string	strHostAddr = hostaddrs[i].ipAddress();
			if (strHostAddr == SSMasterIPForClient.get())
				bHaveMasterIPForClient = true;
			if (strHostAddr == SSMasterIPForSlave.get())
				bHaveMasterIPForSlave = true;
			if (strHostAddr == SSSlaveIPForClient.get())
				bHaveSlaveIPForClient = true;
		}
		if (!bHaveSlaveIPForClient)
		{
			siperrornoex("This computer have not slave ip address for client.");
			return;
		}
		if (bHaveMasterIPForSlave != bHaveMasterIPForClient)
		{
			siperrornoex("Mismatched master ip address between client and slave.");
			return;
		}

		string	strSlavePublicAddr = SSSlaveIPForPublic.get();
		uint16	strSlavePublicPort = SSSlavePortForPublic.get();
		if (strSlavePublicAddr == "")
		{
			strSlavePublicAddr = SSSlaveIPForClient.get();
			strSlavePublicPort = SSSlavePortForClient.get();
		}
		CInetAddress	slavePublicAddr(strSlavePublicAddr, strSlavePublicPort);

		bool	bMasterHost = bHaveMasterIPForClient;
		// create SLAVE Server
		CInetAddress	masterAddrForSlave(SSMasterIPForSlave.get(), SSServicePortForSS.get());
		CInetAddress	slaveListenAddr(SSSlaveIPForClient.get(), SSSlavePortForClient.get());
		SlaveService = new	CSlaveService;
		SlaveService->init(slaveListenAddr, masterAddrForSlave, slavePublicAddr, bMasterHost);

		if (bMasterHost)
		{
			CInetAddress	masterListenAddr(SSMasterIPForClient.get(), SSMasterPortForClient.get());
			// create MASTER Server
			MasterService = new	CMasterService(masterListenAddr);

			CDelayServer * delayServer = MasterService->Server();
			CRQObserver slave(CMasterService::ownSlaveId(), CMasterService::ownSlaveName(), SlaveService->ListenAddr(), slavePublicAddr);
			delayServer->registerRQObserver(slave);
		}
	}

	bool update()
	{
		updateClientServer(SIPNET::UserUpdateTime.get(), SIPNET::MinUserUpdateTime.get());

		MainDB->update();
		UserDB->update();
		return true;
	}

	void release()
	{
		releaseClientServer();

		DBRelease();

#ifdef	PERFMONITORING
		CPerfSystem::getInstance().release();
#endif
	}
private:
};

SIPNET_SERVICE_MAIN(CStartService, SS_S_NAME, SS_L_NAME, "SSServicePortForSS", 50002, false, EmptyCallbackArray, SIPSS_CONFIG, SIPSS_LOGS);

SIPBASE_CATEGORISED_COMMAND(SS, displayShardInfo, "display information about all shards", "")
{
	if(args.size() != 0) return false;

	//log.displayNL ("Shard Number %u",	ShardsInfo.size());

	//for ( std::vector<_entryShardNew>::iterator it = ShardsInfo.begin(); it != ShardsInfo.end(); it ++ )
	//{
	//	log.displayNL ("****************************************");
	//	log.displayNL (L"Shard name		%s",		it->strName.c_str());
	//	log.displayNL ("Shard shardId	%u",		it->shardId);
	//	log.displayNL ("Shard state		%u",		it->activeFlag);
	//}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(SS, reloadMstDB, "Reload Master DB", "")
{
	//LoadZoneList();
	LoadGMList();
	
	return true;
}

SIPBASE_CATEGORISED_DYNVARIABLE(SS, uint32, CurrentDBRequestNumber, "number of current DB request")
{
	if (get)
	{
		*pointer = MainDB->GetCurrentDBRequestNumber();
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(SS, uint32, CurrentDBRProcSpeedPerOne, "speed of DB processing")
{
	if (get)
	{
		uint64		tn = MainDB->GetProcedDBRequestNumber();
		uint64		tt = MainDB->GetProcedDBRequestTime();
		if (tn > 0)
			*pointer = (uint32)(tt/tn);
		else
			*pointer = 0;
	}
}
