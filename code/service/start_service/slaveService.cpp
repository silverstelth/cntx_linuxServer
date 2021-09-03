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
#include 	"misc/stop_watch.h"

#include 	"net/service.h"
#include 	"net/login_server.h"
#include 	"net/login_cookie.h"
#include	"net/Key.h"
#include 	"net/CryptEngine.h"
#include 	"net/SecurityServer.h"
#include 	"net/DelayServer.h"
#include	"net/UDPPeerServer.h"

#include 	"../../common/common.h"
#include 	"../../common/err.h"
#include	"../../common/Packet.h"
#include	"../../common/Macro.h"
#include 	"../Common/netCommon.h"
#include	"../Common/Common.h"
#include	"../Common/DBData.h"
#include 	"../Common/QuerySetForMain.h"
#include 	"../Common/QuerySetForAccount.h"

#include	"misc/DBCaller.h"

#include 	"start_service.h"
#include 	"UpdateServer.h"
// #include	"PerfMon.h"
#include	"slaveService.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;


bool		g_bLock = false;
CVariable<uint32>	SSLockFlag("SS", "SSLockFlag", "Show SSLockFlag, 0:Unlocked, 1:Locked", 0, 0, true);
CVariable<bool> UseUDP("net", "UseUDP", "this flag is set wheather if we use UDP communication", false, 0, true);
CVariable<bool>	UseSecurity("security", "UseSecurity", "whether if we use secret algorithm, false : no use, true : use", false, 0, true);
CVariable<unsigned int>	TestDelay("net",	"TestDelay",	"Test Delay Connection", 0, 0, true);
CVariable<unsigned int>	BoundIncommingConnections("net",	"BoundIncommingConnections",	"BoundIncommingConnections", 50, 0, true);
uint	QueryConnections = 0;
//TTUSERSHARDROOM tblShardVSUser;

class	CConnectionAuthInfo
{
public:
	CConnectionAuthInfo(CInetAddress addr, bool bGM)
	{
		_addr	= addr;
		_bGM	= bGM;
		_tmCreated = CTime::getLocalTime();
	}
	~CConnectionAuthInfo(){};

	CInetAddress	_addr;
	bool			_bGM;
	TTime			_tmCreated;
};

typedef	std::map<string, CConnectionAuthInfo>	_MAPCONNECTIONAUTHINFO;
typedef	std::map<string, CConnectionAuthInfo>::iterator	_ITMAPCONNECTIONAUTHINFO;
_MAPCONNECTIONAUTHINFO		_mapConnectionAuthInfo;

void	insertCookieInfo(SIPNET::CInetAddress udpAddr, bool bGM)
{
	string strCookie = udpAddr.asUniqueString();
	_ITMAPCONNECTIONAUTHINFO	it = _mapConnectionAuthInfo.find(strCookie);
	if (it != _mapConnectionAuthInfo.end())
		return;

	CConnectionAuthInfo		info(udpAddr, bGM);
	_mapConnectionAuthInfo.insert(std::make_pair(strCookie, info));
}

void	eraseCookieInfo(string strCookie)
{
	_mapConnectionAuthInfo.erase(strCookie);
}

bool	isGMType(uint8	userType)
{
	return (userType == USERTYPE_GAMEMANAGER || userType == USERTYPE_GAMEMANAGER_H 
		|| userType == USERTYPE_EXPROOMMANAGER || userType == USERTYPE_WEBMANAGER );
}

static void RefreshConnectionAuthInfo()
{
	vector<string>	deleteKeys;
	{
		_ITMAPCONNECTIONAUTHINFO	it;
		for (it = _mapConnectionAuthInfo.begin(); it != _mapConnectionAuthInfo.end(); it++)
		{
			CConnectionAuthInfo authInfo = it->second;
			TTime curTime = CTime::getLocalTime();
			if (curTime - authInfo._tmCreated > AuthorizeTime.get())
			{
				deleteKeys.push_back(it->first);
			}
		}
	}

	{
		vector<string>::iterator dk;
		for (dk = deleteKeys.begin(); dk != deleteKeys.end(); dk++)
			_mapConnectionAuthInfo.erase(*dk);
	}
}


void	cbUserAuth( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb );
void    cbForceLogin( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb );
void	cbUserChooseShard( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb );
void	cbUserDisconnect( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb);
void	cb_M_CS_CHECK_FASTENTERROOM( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb);
TCallbackItem UserCallbackArray[] = 
{
	{ M_SS_AUTH,				cbUserAuth },
	{ M_CS_FORCELOGIN,          cbForceLogin },
	{ M_ENTER_SHARD_REQUEST,		cbUserChooseShard },
	{ M_CLIENT_DISCONNECT,		cbUserDisconnect },
	{ M_CS_CHECK_FASTENTERROOM,		cb_M_CS_CHECK_FASTENTERROOM },
};

void	cbConnectionCookie(CMessage &msgin, const std::string &serviceName, TServiceId sid);
void	cbMasterSSReject(CMessage &msgin, const std::string &serviceName, TServiceId sid);
TUnifiedCallbackItem MasterCBArray[] =
{
	{ M_SS_NOTIFYCOOKIETOSLAVE,		cbConnectionCookie },
	{ M_SC_FAIL,					cbMasterSSReject }
};

static TSpecMsgCryptRequest	recvCryptMsgArray[] = 
{
	{ M_SS_AUTH,	SCPM_MEDIUM }
};

void	cbUserConnection( TTcpSockId from, void *arg );
void	cbUserDisconnection( TTcpSockId from, void *arg );

void	cbConnectToMaster(const std::string &serviceName, TServiceId sid, void *arg);


_pmap	ConnectPlayers;
_pmap   TempPlayerInfo;
CSlaveService * SlaveService = NULL;

CSlaveService::CSlaveService() :
	_bLocalSlaveWithMaster(false), _server(NULL)
{
};

CSlaveService::~CSlaveService()
{
};

// this service is alone SLAVE service
bool	CSlaveService::init(SIPNET::CInetAddress slaveAddrForClient, SIPNET::CInetAddress masterServiceAddr, SIPNET::CInetAddress slavePublicAddr, bool bIsMaster)
{
	_listenAddr = slaveAddrForClient; 
	_publicAddr = slavePublicAddr; 
	_bLocalSlaveWithMaster = bIsMaster;

	_server = new CCallbackServer(UseUDP.get(), UseSecurity.get());
	_server->setConnectionCallback(cbUserConnection, _server);
	_server->init(_listenAddr.port());
	_server->addCallbackArray(UserCallbackArray, sizeof(UserCallbackArray)/sizeof(UserCallbackArray[0]));
	_server->setDisconnectionCallback(cbUserDisconnection, NULL);
	_server->AddRecvPacketCryptMethod(recvCryptMsgArray, sizeof(recvCryptMsgArray)/sizeof(recvCryptMsgArray[0]));

	CUnifiedNetwork::getInstance()->addCallbackArray(MasterCBArray, sizeof(MasterCBArray)/sizeof(MasterCBArray[0]));
	
	if (!_bLocalSlaveWithMaster)
	{
		CUnifiedNetwork::getInstance()->setServiceUpCallback(SS_S_NAME, cbConnectToMaster, NULL);
		CUnifiedNetwork::getInstance()->addService(SS_S_NAME, masterServiceAddr);
	}

	// UpdateServer init
	if ( ! CUpdateServer::init(_server) )
		sipwarning("UpdateServer Failed");

	if ( TestDelay.get() )
		g_bLock = true;

	sipinfo("SLAVE start! address : %s, public : %s", _listenAddr.asIPString().c_str(), _publicAddr.asIPString().c_str());
	return true;
}

void	CSlaveService::update2(sint32 timeout, sint32 mintime)
{
	if ( QueryConnections > BoundIncommingConnections )
	{
		g_bLock = false;
	}

	if ( TestDelay.get() )
	{
		if ( !g_bLock )
			_server->CCallbackServer::update2(timeout, mintime);
	}
	else
		_server->CCallbackServer::update2(timeout, mintime);
//////////////////////////////////////////////////////////////////////////
	
	RefreshConnectionAuthInfo();

//////////////////////////////////////////////////////////////////////////
	static	SIPBASE::TTime	beforeTime = 0;
	SIPBASE::TTime	curTime = SIPBASE::CTime::getLocalTime();
	if ( beforeTime == 0 ||	(curTime - beforeTime) < NotifyInterval.get() )
	{
		if ( beforeTime == 0 )
			beforeTime = curTime;

		return;
	}
	//CPerfMonServer::update();
	beforeTime = curTime;

	if ( !_bLocalSlaveWithMaster )
	{
		uint32	RQCon = GetRQConnections();	
		uint32	ProcSpeedPerMS = GetProcSpeedPerMS();
		uint32	RQPKSize = GetReceiveQueueItem();

		CMessage	msgout(M_SS_RQSTATE);
		msgout.serial(_nState, RQCon, ProcSpeedPerMS, RQPKSize);
		CUnifiedNetwork::getInstance()->send(_masterSid, msgout);
	}
}


void	CSlaveService::release()
{
	if ( _server != NULL )
		delete _server;
	
	_server = NULL;

	//CPerfMonServer::release();

//	SlaveService = NULL;
}

void	CSlaveService::send (const CMessage &buffer, TTcpSockId hostid, uint8 bySendType)
{
	_server->send(buffer, hostid, bySendType);
}

void	cbMasterSSReject(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	std::string		strReason;
	msgin.serial(strReason);

    siperror(strReason.c_str());
}

// packet : M_SS_NOTIFYCOOKIETOSLAVE
// Master -> Slave
void	cbConnectionCookie(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	CInetAddress	addr;		msgin.serial(addr);

	insertCookieInfo(addr);

	CMessage	msgout(M_SS_RECEIVED_COOKIE);
	msgout.serial(addr);
	CUnifiedNetwork::getInstance()->send(sid, msgout);
}

// check if user can enter a shard!
T_ERR checkUserPermission(uint32 userid, uint32 shardID)
{
	T_ERR	errNo	= ERR_NOERR;

	return errNo;
}

// if from == InvalidSock, Disconnect All
void DisconnectUser(CCallbackNetBase& clientcb, TTcpSockId from, uint32	userid)
{
	if ( from == InvalidSockId )
		return;
	CMessage	msgout(M_CLIENT_DISCONNECT);
	msgout.serial(userid);
	SlaveService->Server()->send(msgout, from);
	clientcb.flush(from);
	clientcb.disconnect(from);

	// comment blow statement, because when client actively close the connection, 
	// the TcpSockect object to which 'from' point should be deleted by CServerReceiveTask::clearClosedConnections() 
	// in back-ground thread, after this, 'from' is invalid point

	sipdebug("Disconnected Client userid:%u", userid); // byy krs

    CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);

	_pmap::iterator info = ConnectPlayers.find(userid);
	if(info !=ConnectPlayers.end())
	{
		ConnectPlayers.erase(info);

		{
			// 클라이언트가 로그오프되였음을 tbl_UserBaseInfo에 설정한다.
			uint8	OnlineStatus = 0;
			MAKEQUERY_SetUserOnlineStatus(strQuery, userid, OnlineStatus);
			MainDB->execute(strQuery, NULL);
			// DB_EXE_QUERY(MainDBConnection, Stmt, strQuery);
		}
	}
	sipdebug("ConnectedPlayers %u", ConnectPlayers.size());

	// delete from the user's shard list where he has room
//T	TUSERSHARDROOMIT itShard = tblShardVSUser.find(userid);
//T	if ( itShard != tblShardVSUser.end() )
//T	{
//T		tblShardVSUser.erase(itShard);
//T	}

	// Added by RSC 2009/12/07
	CKeyManager::getInstance().unregisterKey(from);
}

// if from == InvalidSock, Disconnect All
void ForceDisconnectUser(CCallbackNetBase& clientcb, TTcpSockId from, uint32	userid)
{
	if ( from == InvalidSockId )
		return;
	CMessage	msgout(M_CLIENT_DISCONNECT);
	msgout.serial(userid);

	clientcb.flush(from);
	clientcb.forceClose(from);

	_pmap::iterator info = ConnectPlayers.find(userid);
	if(info !=ConnectPlayers.end())
	{
		ConnectPlayers.erase(info);

		{
			// 클라이언트가 로그오프되였음을 tbl_UserBaseInfo에 설정한다.
			uint8	OnlineStatus = 0;
			MAKEQUERY_SetUserOnlineStatus(strQuery, userid, OnlineStatus);
			MainDB->execute(strQuery, NULL);

			//DB_EXE_QUERY(MainDBConnection, Stmt, strQuery);
		}
	}

	// delete from the user's shard list where he has room
//T	TUSERSHARDROOMIT itShard = tblShardVSUser.find(userid);
//T	if ( itShard != tblShardVSUser.end() )
//T	{
//T		tblShardVSUser.erase(itShard);
//T	}
}

// Client -> SS
void cbUserConnection( TTcpSockId from, void *arg )
{
	CCallbackServer * server = (CCallbackServer *)arg;

	//CMessage msgout(M_HANDSHKOK);
	//server->send(msgout, from);

	server->authorizeOnly (M_SS_AUTH, from, AuthorizeTime.get());
}

void cbUserDisconnection( TTcpSockId from, void *arg )
{
	if ( from == InvalidSockId )
		return;

	_pmap::iterator it;
	for ( it = ConnectPlayers.begin(); it != ConnectPlayers.end(); it ++ )
	{
		CPlayer		info = (*it).second;
		if ( from->getTcpSock()->descriptor() == info.con->getTcpSock()->descriptor() )
			break;
	}
	if( it == ConnectPlayers.end() )
	{
		sipdebug("Unconnected Client is trying to disconnect, or already disconnect!");
		return;
	}
    
	uint32	userid = (*it).second.id;
	CMessage	msgout(M_CLIENT_DISCONNECT);
	msgout.serial(userid);
	SlaveService->Server()->send(msgout, from);
	sipdebug("Disconnected Client userid:%u, socket %s", userid, from->asString().c_str());

    CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);

	_pmap::iterator info = ConnectPlayers.find(userid);
	if(info !=ConnectPlayers.end())
	{
		ConnectPlayers.erase(info);

		{
			// 클라이언트가 로그오프되였음을 tbl_UserBaseInfo에 설정한다.
			uint8	OnlineStatus = 0;
			MAKEQUERY_SetUserOnlineStatus(strQuery, userid, OnlineStatus);
			MainDB->execute(strQuery, NULL);

			// DB_EXE_QUERY(MainDBConnection, Stmt, strQuery);
		}
	}

	// delete from the user's shard list where he has room
//T	TUSERSHARDROOMIT itShard = tblShardVSUser.find(userid);
//T	if ( itShard != tblShardVSUser.end() )
//T	{
//T		tblShardVSUser.erase(itShard);
//T	}

	CKeyManager::getInstance().unregisterKey(from);
}


T_ERR DBGetAccountInfo(ucstring ucUserName, uint32 *pdbid, uint32 *pJifen, std::string sPassword, std::string &sPassword2)
{
	string	sUserName = ucUserName.toString();

	sipinfo("sUsername = %s, sPassword = %s, pdbid = %d, pJifen = %d", sUserName.c_str(), sPassword.c_str(), pdbid, pJifen);
	MAKEQUERY_GETACCOUNT(strQuery, sUserName.c_str(), sPassword.c_str());
	CDBConnectionRest	DB(UserDB);
	DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
	if ( !nPrepareRet )
		return E_DBERR;

	SQLLEN		num1 = 0, num2 = 0;			
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, pdbid, sizeof(uint32), &num1);
	DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_ULONG, SQL_INTEGER, 0, 0, pJifen, sizeof(uint32), &num2);

	if ( !nBindResult )
		return E_DBERR;

	DB_EXE_PARAMQUERY(Stmt, strQuery);
	if ( !nQueryResult )
	{
		sipdebug("DBERROR");
		return E_DBERR;
	}

	DB_QUERY_RESULT_FETCH(Stmt, row);
	if (!IS_DB_ROW_VALID(row))
	{
		return E_DBERR;
	}
	COLUMN_DIGIT(row, 0, sint32, ret);
	
	T_ERR	err;
	switch ( ret )
	{
	case 0:
		return ERR_NOERR;
		break;
	case 1:
		err = ERR_IMPROPERUSER;
//		ERR_SHOW("Failed DB User Auth", ERR_IMPROPERUSER);
		sipinfo(L"ERROR : Failed DB User Auth, Result(err %u): ERR_IMPROPERUSER, ucUserID=%s", err, ucUserName.c_str());
		break;
	case 2:
		err = ERR_BADPASSWORD;
//		ERR_SHOW("Invalid Password.", ERR_BADPASSWORD);
		sipinfo(L"ERROR : Invalid Password., Result(err %u): ERR_BADPASSWORD, ucUserID=%s", err, ucUserName.c_str());
		break;
	case 3:
		err = ERR_NOACTIVEACCOUNT;
//		ERR_SHOW("No activate.", ERR_NOACTIVEACCOUNT);
		sipinfo(L"ERROR : No activate., Result(err %u): ERR_NOACTIVEACCOUNT, ucUserID=%s", err, ucUserName.c_str());
		break;
		
	default:
		err = E_DBERR;
		ERR_SHOW("DB Error.", E_DBERR);
		break;
	}

	return	err;
}

void	DBUpdateNewUserRegisterState(uint32 nUserID)
{
	MAKEQUERY_UPDATEACCOUNTREGISTERSTATE(strQuery, nUserID);
	CDBConnectionRest	DB(UserDB);
	DB_EXE_QUERY(DB, Stmt, strQuery);
}

T_ERR	DBInsertAccountInfo(uint32 nUserID, uint32 addJifen, ucstring ucUserName, std::string sPassword, std::string sPassword2)
{
	string sUserName = ucUserName.toString().c_str();
	sint32	ret = 0;
	
	uint32	uDefaultMoney = 0;
	string	sDefaultMoney = GetGoldEncodeStringByAccount(sUserName, uDefaultMoney);
	MAKEQUERY_INSERTUSER(strQuery, nUserID, sUserName.c_str(), sPassword.c_str(), sPassword2.c_str(), uDefaultMoney, sDefaultMoney.c_str(), addJifen);
	CDBConnectionRest	DB(MainDB);

	DB_EXE_PREPARESTMT(DB, Stmt, strQuery);
	if ( !nPrepareRet )
		return E_DBERR;

	SQLLEN	len1 = sizeof(sint32);							
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, sizeof(sint32), &len1);
	if ( !nBindResult )
		return E_DBERR;

	DB_EXE_PARAMQUERY(Stmt, strQuery);
	if ( !nQueryResult )
	{
		sipdebug("DBERROR");
		return E_DBERR;
	}

	T_ERR	err;
	switch ( ret )
	{
	case 0:
	case 7:
		err = ERR_NOERR;
		sipinfo(L"Success insert new user(ID=%d, Name=%s)", nUserID, ucUserName.c_str());
		break;
	default:
		err = E_DBERR;
		ERR_SHOW("Insert AccountInfo to MainDB", E_DBERR);
		break;
	}

	return	err;
}

T_ERR DBAuthUser(ucstring ucUserName, std::string sPassword, uint32 *pdbid, uint32 *pfid, uint32 *pshardid, uint8 *puserTypeId, std::vector<uint32> &ShardList )
{
	sipinfo(L"CHECK : auth ucUserID %s, Password %S", ucUserName.c_str(), sPassword.c_str());

	size_t nSize = ucUserName.size();
	if ( nSize > MAX_LEN_USER_ID || nSize < 1 )
		return ERR_DATAOVERFLOW;

	nSize = sPassword.size();
	if ( nSize > MAX_LEN_USER_PASSWORD || nSize < 1 )
		return ERR_DATAOVERFLOW;
	
	MAKEQUERY_CHECKAUTH_2(strQuery);
	CDBConnectionRest	DB(MainDB);
    DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
	if ( !nPrepareRet )
		return E_DBERR;

	SQLWCHAR* sqlUserName = toSQLString(ucUserName.c_str());

	SQLLEN	num1 = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0;
    SQLLEN lenLogin = ucUserName.size() * sizeof(SQLWCHAR);
	DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_WCHAR, SQL_WLONGVARCHAR, ucUserName.size(), 0, sqlUserName/*(void *)ucUserName.c_str()*/, 0, &lenLogin)
	SQLLEN lenPW = sPassword.size();
	DB_EXE_BINDPARAM_IN(Stmt, 2, SQL_C_CHAR, SQL_VARCHAR, sPassword.size(), 0, (void *)sPassword.c_str(), 0, &lenPW)
	uint32	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 3, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &num1)
	DB_EXE_BINDPARAM_OUT(Stmt, 4, SQL_C_ULONG, SQL_INTEGER, 0, 0, pdbid, sizeof(uint32), &num2)
	DB_EXE_BINDPARAM_OUT(Stmt, 5, SQL_C_ULONG, SQL_INTEGER, 0, 0, pfid, sizeof(uint32), &num3)
	DB_EXE_BINDPARAM_OUT(Stmt, 6, SQL_C_ULONG, SQL_INTEGER, 0, 0, pshardid, sizeof(uint32), &num4)
	DB_EXE_BINDPARAM_OUT(Stmt, 7, SQL_C_TINYINT, SQL_TINYINT, 0, 0, puserTypeId, sizeof(uint8), &num5)
	if ( !nBindResult )
		return E_DBERR;

	DB_EXE_PARAMQUERY(Stmt, strQuery);
	if ( !nQueryResult )
	{
		sipdebug("DBERROR");
		return E_DBERR;
	}

	// fetch the shard list where this user has room
//T	std::vector<uint32> ShardList;
	do
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
			break;

		COLUMN_DIGIT(row,	0,	uint32,	_ShardId);
		ShardList.push_back(_ShardId);

	}while ( true );

//T	if(ShardList.size() != 0)
//T	{
//T		tblShardVSUser[*pdbid] = ShardList;
//T	}

	T_ERR	err;
	switch ( ret )
	{
	case 0:
		err = ERR_NOERR;
		RESULT_SHOW("Success DB User Auth", ERR_NOERR);
		break;
	case 1:
		err = ERR_IMPROPERUSER;
//		ERR_SHOW("Failed DB User Auth", ERR_IMPROPERUSER);
		sipinfo(L"ERROR : Failed DB User Auth, Result(err %u): ERR_IMPROPERUSER, ucUserID=%s", err, ucUserName.c_str());
		break;
	case 2:
		err = ERR_BADPASSWORD;
//		ERR_SHOW("Failed DB User Auth", ERR_BADPASSWORD);
		sipinfo(L"ERROR : Failed DB User Auth, Result(err %u): ERR_BADPASSWORD, ucUserID=%s", err, ucUserName.c_str());
		break;
	case 3:
		err = ERR_ALREADYONLINE;
//		ERR_SHOW("Failed DB User Auth", ERR_ALREADYONLINE);
		sipinfo(L"ERROR : Failed DB User Auth, Result(err %u): ERR_ALREADYONLINE, ucUserID=%s", err, ucUserName.c_str());
		break;
	case 4:
		err = ERR_STOPEDUSER;
//		ERR_SHOW("Failed DB User Auth", ERR_STOPEDUSER);
		sipinfo(L"ERROR : Failed DB User Auth, Result(err %u): ERR_STOPEDUSER, ucUserID=%s", err, ucUserName.c_str());
		break;
	case 9:
		err = ERR_NONACTIVEUSER;
//		ERR_SHOW("Failed DB User Auth", ERR_NONACTIVEUSER);
		sipinfo(L"ERROR : Failed DB User Auth, Result(err %u): ERR_NONACTIVEUSER, ucUserID=%s", err, ucUserName.c_str());
		break;
	default:
		err = (T_ERR) ret;
		sipwarning("Unknown Error : %u", ret);
		break;
	}
	
	return	err;
}

T_ERR DBAuthGM(ucstring ucUserName, std::string sPassword, uint32 *pdbid, uint32 *pfid, uint32 *pshardid, uint8 *puserTypeId, std::vector<uint32> &ShardList )
{
	sipinfo(L"CHECK : GM Auth ucUserID %s, Password %S", ucUserName.c_str(), sPassword.c_str());

	size_t nSize = ucUserName.size();
	if ( nSize > MAX_LEN_USER_ID || nSize < 1 )
		return ERR_DATAOVERFLOW;

	nSize = sPassword.size();
	if ( nSize > MAX_LEN_USER_PASSWORD || nSize < 1 )
		return ERR_DATAOVERFLOW;

	MAKEQUERY_CHECKAUTH_GM_2(strQuery);
	CDBConnectionRest	DB(MainDB);
	DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
	if ( !nPrepareRet )
		return E_DBERR;

	SQLLEN	num1 = 0, num2 = 0, num3 = 0, num4 = 0, num5 = 0;
	SQLLEN lenLogin = ucUserName.size() * sizeof(SQLWCHAR);
	DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_WCHAR, SQL_WLONGVARCHAR, ucUserName.size(), 0, (void *)ucUserName.c_str(), 0, &lenLogin);
	SQLLEN lenPW = sPassword.size();
	DB_EXE_BINDPARAM_IN(Stmt, 2, SQL_C_CHAR, SQL_VARCHAR, sPassword.size(), 0, (void *)sPassword.c_str(), 0, &lenPW);
	uint32	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 3, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &num1)
	DB_EXE_BINDPARAM_OUT(Stmt, 4, SQL_C_ULONG, SQL_INTEGER, 0, 0, pdbid, sizeof(uint32), &num2)
	DB_EXE_BINDPARAM_OUT(Stmt, 5, SQL_C_ULONG, SQL_INTEGER, 0, 0, pfid, sizeof(uint32), &num3)
	DB_EXE_BINDPARAM_OUT(Stmt, 6, SQL_C_ULONG, SQL_INTEGER, 0, 0, pshardid, sizeof(uint32), &num4)
	DB_EXE_BINDPARAM_OUT(Stmt, 7, SQL_C_TINYINT, SQL_TINYINT, 0, 0, puserTypeId, sizeof(uint8), &num5)
	if ( !nBindResult )
		return E_DBERR;

	DB_EXE_PARAMQUERY(Stmt, strQuery);
	if ( !nQueryResult )
	{
		sipdebug("DBERROR");
		return E_DBERR;
	}

	if(ret != 0)
	{
		switch ( ret )
		{
		case 1:
			ERR_SHOW("Failed DB User Auth", ERR_IMPROPERUSER);
			return ERR_IMPROPERUSER;
		case 2:
			ERR_SHOW("Failed DB User Auth", ERR_BADPASSWORD);
			return ERR_BADPASSWORD;		
		case 4:
			ERR_SHOW("Failed DB User Auth", ERR_STOPEDUSER);
			return ERR_STOPEDUSER;
			break;
		case 9:
			ERR_SHOW("Failed DB User Auth", ERR_NONACTIVEUSER);
			return ERR_NONACTIVEUSER;
		default:
			sipwarning("Unknown Error : %u", ret);
			return E_DBERR;
		}
	}

	bool bAlreadyOnline = false;
	vector<uint16>		shardList;
	DB_QUERY_RESULT_FETCH(Stmt, row);
	if (IS_DB_ROW_VALID(row))
	{
		bAlreadyOnline = true;		// 통합써버기능: GM 다중로그인기능 삭제
		do
		{
			COLUMN_DIGIT(row, 0, uint16, shardId);
			shardList.push_back(shardId);

			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (!IS_DB_ROW_VALID(row))
				break;
		}while (true);
	}

//	short  sMore1 = SQLMoreResults( Stmt );
	BOOL bMore1 = Stmt.MoreResults();
	if (bMore1)
	{
		do
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( !IS_DB_ROW_VALID(row) )
				break;

			COLUMN_DIGIT(row,	0,	uint32,	_ShardId);
			ShardList.push_back(_ShardId);

		}while ( true );
	}

	if (!bAlreadyOnline)
	{
		for ( vector<uint16>::iterator it = shardList.begin(); it != shardList.end(); it ++ )
		{
			uint16	shardId = *it;

			bool bOnline = false;
			std::pair<TONLINEGMSIT, TONLINEGMSIT>	range = g_onlineGMs.equal_range(*pdbid);
			for ( TONLINEGMSIT it = range.first; it != range.second; it ++ )
			{
				uint32	idshard = (*it).second;
				if ( idshard == shardId )
					bOnline = true;
			}

			if ( ! bOnline )
				g_onlineGMs.insert(std::make_pair(*pdbid, shardId));
		}

		RESULT_SHOW("Success DB User Auth", ERR_NOERR);
	}

	// fetch the shard list where this user has room
//T	std::vector<uint32> ShardList;

//T	if(ShardList.size() != 0)
//T	{
//T		tblShardVSUser[*pdbid] = ShardList;
//T	}

	if (bAlreadyOnline)
		return ERR_ALREADYONLINE;
	return	ERR_NOERR;
}

T_ERR	getUserInfoFromAccountInfo( ucstring ucUser, std::string sPassword, CPlayer &info )
{
	T_ERR		err = ERR_NOERR;
	uint32		userID = 0;
	_pmap::iterator	it;
	for ( it = ConnectPlayers.begin(); it != ConnectPlayers.end(); it ++ )
	{
		info = (*it).second;
		/*if ( info == NULL )
			return NULL;*/
		if ( ucUser == info.ucUser && sPassword == info.sPassword )
		{
			err = ERR_NOERR;
			break;
		}
		if ( ucUser != info.ucUser )
		{
			err = ERR_IMPROPERUSER;
			continue;
		}
		if ( sPassword != info.sPassword )
		{
			err = ERR_BADPASSWORD;
			continue;
		}
	}

	if ( err == ERR_NOERR && it == ConnectPlayers.end() )
	{
		err = ERR_ALREADYOFFLINE;
		//info = NULL;
	}

    return	err;
}

T_ERR checkPendingUserConnection( TTcpSockId from, CCallbackNetBase& clientcb, uint32 userid )
{
	if ( from == InvalidSockId )
		return	ERR_INVALIDCONNECTION;

	// for special userid, there is no connection
	_pmap::iterator it = ConnectPlayers.find(userid);
	if ( it == ConnectPlayers.end() )
		return	ERR_NOCONDBID;

	// This connection is invalid
	TTcpSockId	sock = it->second.con;
	if ( from != sock )
		return	ERR_INVALIDCONNECTION;

	CTcpSock * tcpsock = const_cast<CTcpSock *> (sock->getTcpSock());
	if ( !tcpsock->connected() )
		return	ERR_DISCONNECTION;

	return ERR_NOERR;
}

bool checkConnectionCookie( std::string strCookie, TTcpSockId from, CCallbackNetBase& clientcb)
{
	bool	bAuthCookie = false;

	_ITMAPCONNECTIONAUTHINFO	it = _mapConnectionAuthInfo.find(strCookie);
	if ( it != _mapConnectionAuthInfo.end() )
	{
		if (from->getTcpSock()->remoteAddr().ipAddress() != it->second._addr.ipAddress())
		{
			sipinfo("Mismatch ip address, TCP=%s, UDP=%s", from->getTcpSock()->remoteAddr().ipAddress().c_str(), it->second._addr.ipAddress().c_str());
		}
		{
			bAuthCookie = true;
			eraseCookieInfo(strCookie);
		}
	}

	if ( AcceptInvalidCookie )
		return true;

	if ( ! bAuthCookie )
	{
		sipwarning("Unauthed user try to connection, Invalid Connection Cookie=%s", strCookie.c_str());
		SendFailMessageToClient(ERR_INVALIDCONNECTIONCOOKIE, from, clientcb);

		clientcb.forceClose(from);
		return false;
	}

	return true;
}

T_ERR checkUserAuth(ucstring ucUserName, std::string sPassword, TTcpSockId from, CCallbackNetBase &clientcb, bool bIsGM)
{
	uint32	userid, lastShardID, fid;
	uint8	userTypeId;
	std::string sPassword2;
	std::vector<uint32> ShardList;

	T_ERR	errNo = DBAuthUser(ucUserName, sPassword, &userid, &fid, &lastShardID, &userTypeId, ShardList);
	sipinfo("AUTH userId %u, lastId %u, UserType %u", userid, lastShardID, userTypeId);
	if ( errNo == ERR_IMPROPERUSER )
	{
		uint32 addJifen = 0;
		errNo = DBGetAccountInfo(ucUserName, &userid, &addJifen, sPassword, sPassword2);
		if ( errNo == ERR_NOERR )
		{
			T_ERR errInsert = DBInsertAccountInfo(userid, addJifen, ucUserName, sPassword, sPassword2);
			if (errInsert == ERR_NOERR)
			{
				DBUpdateNewUserRegisterState(userid);
				errNo = DBAuthUser(ucUserName, sPassword, &userid, &fid, &lastShardID, &userTypeId, ShardList);
			}
		}
	}

	if ( errNo == ERR_NOERR )
	{
		// MainServer에 로그인되여있지만 Shard에 들어가있지 않은 상태인지 검사한다.
		MAKEQUERY_GetUserOnlineStatus(strQuery, userid);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (!nQueryResult)
		{
			sipwarning("Main_UserBaseinfo_Oline_List exec failed.");
			return E_DBERR;
		}

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			sipwarning("Main_UserBaseinfo_Oline_List returned empty.");
			return E_DBERR;
		}

		COLUMN_DIGIT(row,	0,	uint8,	online_status);
		if(online_status != 0)
		{
			errNo = ERR_ALREADYONLINE;
		}
	}

	if ( errNo == ERR_NOERR )
	{
		// Check UserAddress
		_pmap::iterator it = ConnectPlayers.find(userid);
		if ( it != ConnectPlayers.end() )
		{
			sipwarning(L"ConnectPlayers List Duplicated!!! ucUserName=%s, userid=%d", ucUserName.c_str(), userid);
			ConnectPlayers.erase(it);
		}

		{
			// 클라이언트가 로그인되였음을 tbl_UserBaseInfo에 설정한다.
			uint8	OnlineStatus = 1;
			MAKEQUERY_SetUserOnlineStatus(strQuery, userid, OnlineStatus);
			MainDB->execute(strQuery, NULL);
		}

		ConnectPlayers.insert(std::make_pair(userid, CPlayer(userid, fid, from, userTypeId, lastShardID, ucUserName, sPassword, ShardList, bIsGM)));
//		sipdebug("ConnectedPlayers %u", ConnectPlayers.size());

		from->setAppId((uint64)userid);

		CMessage	msgout1(M_SC_SHARDS);
		msgout1.serial(userid, fid);
		CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout1);
//		sipdebug("send <SHARDS> to LS");
	}
	else
	{
		if ( errNo != ERR_ALREADYONLINE )
		{
//			ERR_SHOW("LoginAuth Failed! ", errNo);
			sipinfo("ERROR : LoginAuth Failed! , Result(err %u): ", errNo);
			SendFailMessageToClient(errNo, from, clientcb);
			return errNo;
		}
		// CASE : Same Account Already Connected To SS
		clientcb.authorizeOnly(M_CS_FORCELOGIN, from, 0);

		CMessage msgout(M_SC_FAIL);
		msgout.serial(errNo);
		msgout.serial(userid);

		CCallbackServer * server = dynamic_cast<CCallbackServer *>( &clientcb );
		server->send(msgout, from);	

		if ( TempPlayerInfo.find(userid) == TempPlayerInfo.end() )
			TempPlayerInfo.insert( std::make_pair(userid, CPlayer(userid, fid, from, userTypeId, lastShardID, ucUserName, sPassword, ShardList, bIsGM)));
	}

	return errNo;
}

T_ERR checkGMAuth(ucstring ucUserName, std::string sPassword, TTcpSockId from, CCallbackNetBase &clientcb)
{
	T_ERR	errNo = ERR_NOERR;
	uint32	userid, lastShardID, fid;
	uint8	userTypeId;
	std::string sPassword2;
	std::vector<uint32> ShardList;

	bool	bIsGM = true;
	errNo = DBAuthGM(ucUserName, sPassword, &userid, &fid, &lastShardID, &userTypeId, ShardList);
	sipinfo("GMAUTH errNo=%d, userId=%u, lastId=%u, UserType=%u, FID=%d", errNo, userid, lastShardID, userTypeId, fid); // byy krs
	if ( errNo == ERR_IMPROPERUSER )
	{
		uint32 addJifen = 0;
		errNo = DBGetAccountInfo(ucUserName, &userid, &addJifen, sPassword, sPassword2);
		if ( errNo == ERR_NOERR )
		{
			T_ERR errInsert = DBInsertAccountInfo(userid, addJifen, ucUserName, sPassword, sPassword2);
			if (errInsert == ERR_NOERR)
			{
				DBUpdateNewUserRegisterState(userid);
				errNo = DBAuthUser(ucUserName, sPassword, &userid, &fid, &lastShardID, &userTypeId, ShardList);
			}
		}
	}

	if ( errNo != ERR_NOERR )
	{
		if ( errNo != ERR_ALREADYONLINE )
		{
			ERR_SHOW("LoginAuth Failed! Wrong userid", errNo);
			SendFailMessageToClient(errNo, from, clientcb);
			g_onlineGMs.erase(userid);
			return errNo;
		}
		// 통합써버기능: GM다중로그인기능 삭제
		//else
		//{
		//	// case GM, impossible
		//	// CASE : Same Account Already Connected To SS
		//	errNo = checkPendingUserConnection(from, clientcb, userid);

		//	{
		//		_pmap::iterator it = ConnectPlayers.find(userid);
		//		if ( it != ConnectPlayers.end() )
		//		{
		//			TTcpSockId	old = it->second.con;
		//			DisconnectUser(clientcb, old, userid);
		//		}
		//		sipinfo("<DC>: Client wants disconnect client userid:%u", userid);
		//		g_onlineGMs.erase(userid);
		//		return errNo;
		//	}
		//}
	}
	// 통합써버기능: GM다중로그인기능 삭제
	if ( errNo == ERR_NOERR )
	{
		// MainServer에 로그인되여있지만 Shard에 들어가있지 않은 상태인지 검사한다.
		MAKEQUERY_GetUserOnlineStatus(strQuery, userid);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (!nQueryResult)
		{
			sipwarning("Main_UserBaseinfo_Oline_List exec failed.");
			return E_DBERR;
		}

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			sipwarning("Main_UserBaseinfo_Oline_List returned empty.");
			return E_DBERR;
		}

		COLUMN_DIGIT(row,	0,	uint8,	online_status);
		if(online_status != 0)
		{
			errNo = ERR_ALREADYONLINE;
		}
	}
	// 통합써버기능: GM다중로그인기능 삭제
	if (errNo == ERR_ALREADYONLINE)
	{
		ERR_SHOW("LoginAuth Failed! Already Online.", errNo);

		// CASE : Same Account Already Connected To SS
		clientcb.authorizeOnly(M_CS_FORCELOGIN, from, 0);

		CMessage msgout(M_SC_FAIL);
		msgout.serial(errNo);
		msgout.serial(userid);

		CCallbackServer * server = dynamic_cast<CCallbackServer *>( &clientcb );
		server->send(msgout, from);	

		if ( TempPlayerInfo.find(userid) == TempPlayerInfo.end() )
			TempPlayerInfo.insert( std::make_pair(userid, CPlayer(userid, fid, from, userTypeId, lastShardID, ucUserName, sPassword, ShardList, bIsGM)));

		return errNo;
	}

	if ( !isGMType(userTypeId) )
	{
		DisconnectUser(clientcb, from, userid);
		sipinfo("Client %u is not GMType and try GMlogin, So Disconnect", userid);
		g_onlineGMs.erase(userid);

		return errNo;
	}

	// if GM, Check UserAddress
	bool	bGMAuth = false;
	CInetAddress	addr = clientcb.hostAddress(from);
	std::pair<TGMADDRSIT, TGMADDRSIT> range = tblGMAddrs.equal_range(userid);
	uint32	nCount = tblGMAddrs.count(userid);
	if ( nCount == 0 )
		bGMAuth = true;

	for ( TGMADDRSIT it = range.first; it != range.second; it ++ )
	{
		std::string	strGMAddr = (*it).second;
		if ( addr.ipAddress() == strGMAddr )
		{
			bGMAuth = true;
			break;
		}
	}
	if ( !bGMAuth )
	{
		errNo = ERR_GM_AUTHIP;
		sipwarning("GMClient's IPAddress is not allowed, curIPAddress is %s", addr.ipAddress().c_str());
		g_onlineGMs.erase(userid);

		return errNo;
	}

	// 통합써버기능: GM다중로그인기능 삭제
	{
		// 클라이언트가 로그인되였음을 tbl_UserBaseInfo에 설정한다.
		uint8	OnlineStatus = 1;
		MAKEQUERY_SetUserOnlineStatus(strQuery, userid, OnlineStatus);
		MainDB->execute(strQuery, NULL);
	}

	// Check UserAddress
	_pmap::iterator it = ConnectPlayers.find(userid);
	if ( it != ConnectPlayers.end() )
		ConnectPlayers.erase(it);

	ConnectPlayers.insert(std::make_pair(userid, CPlayer(userid, fid, from, userTypeId, lastShardID, ucUserName, sPassword, ShardList, bGMAuth)));
	sipdebug("ConnectedPlayers %u", ConnectPlayers.size());

	if ( errNo == ERR_NOERR )
	{
		CMessage	msgout1(M_SC_SHARDS);
		msgout1.serial(userid, fid);
		CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout1);
		sipdebug("send <SHARDS> to LS");
	}

	return errNo;
}

// Packet : M_SS_AUTH
// Client -> SS
void cbUserAuth( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb )
{
	if (SSLockFlag)
		return;

	std::string strCookie;		msgin.serial(strCookie);
	ucstring	ucUserName;	msgin.serial(ucUserName);
	std::string sPassword;	msgin.serial(sPassword);
	uint32		pkt_version = 0;msgin.serial(pkt_version);

	if (ucUserName.size() < 1 || ucUserName.size() > MAX_LEN_USER_ID)
		return;
	if (sPassword.size() < 1 || sPassword.size() > MAX_LEN_USER_PASSWORD)
		return;

	bool bIsGM = (ucUserName[0] == L'%') ? true : false;

	bool	isChechPacketVersion = GET_FROM_SERVICECONFIG("CheckPacketVersion", true, asBool);
	if (isChechPacketVersion == true && pkt_version != 0 && // why can client select this ?????? - because of GMTool
		pkt_version != PACKET_VERSION)
	{
//		ERR_SHOW("LoginAuth Failed! ", ERR_INVALIDVERSION);
		sipwarning(L"current version = %u, user version=%u, UserID=%s", PACKET_VERSION, pkt_version, ucUserName.c_str());
		SendFailMessageToClient(ERR_INVALIDVERSION, from, clientcb);
		return;
	}
	CCallbackServer * server = dynamic_cast<CCallbackServer *>( &clientcb );

	if(!bIsGM && strCookie != "")
	{
		if ( !checkConnectionCookie(strCookie, from, clientcb) )
			return;
	}
// CheckAuth ID and Password
	T_ERR	errNo = ERR_NOERR;
	if ( bIsGM )
	{
		errNo = checkGMAuth(ucUserName, sPassword, from, clientcb);
		if ( errNo == ERR_GM_AUTHIP )
			errNo = checkUserAuth(ucUserName, sPassword, from, clientcb, bIsGM);

		return;
	}
	else
		checkUserAuth(ucUserName, sPassword, from, clientcb, bIsGM);
}

bool ForceDisconnectByAnotherLogin(uint32 userId, uint32 gen_time)
{
	_pmap::iterator it = ConnectPlayers.find(userId);
	if (it == ConnectPlayers.end())
		return false;

	TTcpSockId	from = it->second.con;

	if((SlaveService != NULL) && (from != InvalidSockId))
	{
		if((gen_time == 0) || (gen_time != it->second.gen_time))
		{
			CMessage	msgout(M_SC_DISCONNECT_OTHERLOGIN);
			SlaveService->Server()->send(msgout, from);

			SlaveService->Server()->flush(from);
			SlaveService->Server()->disconnect(from);

			ConnectPlayers.erase(it);

			// Added by RSC 2009/12/07
			CKeyManager::getInstance().unregisterKey(from);

			sipinfo("<DC>: Client wants disconnect client userid:%u", userId);
		}
	}
	
	return true;
}

// Request Force Logoff by double login
void cb_M_NT_REQ_FORCE_LOGOFF(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint32	userid;		msgin.serial(userid);
	uint8	bIsGM;		msgin.serial(bIsGM);
	uint32	gen_time;	msgin.serial(gen_time);

	ForceDisconnectByAnotherLogin(userid, gen_time);
}

// Packet : M_CS_FORCELOGIN
void cbForceLogin( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb )
{
	uint32 userId = 0;          msgin.serial(userId);

	// TempPlayerInfo에 없으면 해커이다!!!
	_pmap::iterator	it1 = TempPlayerInfo.find(userId);
	if ( it1 == TempPlayerInfo.end())
	{
		sipwarning("Find Invalid ForceConnection!!! UserID=%d", userId);
		return;
	}

	uint32	curtime = (uint32)CTime::getLocalTime();
	bool	bForceLogouted = ForceDisconnectByAnotherLogin(userId, 0);
	if(!bForceLogouted)
	{
		uint32 lastShardId = TempPlayerInfo[userId].lastShardid;

		CMessage	msgout2(M_NT_REQ_FORCE_LOGOFF);
		msgout2.serial(userId);
		msgout2.serial(lastShardId);
		uint8	bIsGM = TempPlayerInfo[userId].bGM;
		msgout2.serial(bIsGM);
		msgout2.serial(curtime);

		CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout2);

		sipdebug("send user (UID=%u) <M_NT_REQ_FORCE_LOGOFF> to LS, lastShardID : %u", userId, lastShardId);
	}

	// Check UserAddress
	_pmap::iterator it = ConnectPlayers.find(userId);
	if ( it != ConnectPlayers.end() )
		ConnectPlayers.erase(it);

	{
		// 클라이언트가 로그인되였음을 tbl_UserBaseInfo에 설정한다.
		uint8	OnlineStatus = 1;
		MAKEQUERY_SetUserOnlineStatus(strQuery, userId, OnlineStatus);
		MainDB->execute(strQuery, NULL);

		// DB_EXE_QUERY(MainDBConnection, Stmt, strQuery);
	}

	ConnectPlayers.insert(std::make_pair(userId, CPlayer(userId, it1->second.fid, from, TempPlayerInfo[userId].usertypeid, TempPlayerInfo[userId].lastShardid, TempPlayerInfo[userId].ucUser, TempPlayerInfo[userId].sPassword, TempPlayerInfo[userId].ShardList, TempPlayerInfo[userId].bGM, curtime)));
	sipdebug("ConnectedPlayers UID=%d, size=%u", userId, ConnectPlayers.size());

	TempPlayerInfo.erase(userId);

	//////////////////////////////////////////
	from->setAppId((uint64)userId);

	if (bForceLogouted)
	{
		CMessage	msgout1(M_SC_SHARDS);
		msgout1.serial(userId, it1->second.fid);
		CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout1);
	}
}

void	cbConnectToMaster(const std::string &serviceName, TServiceId sid, void *arg)
{
	SlaveService->_masterSid = sid;
	CInetAddress	listenAddr = SlaveService->ListenAddr();
	CInetAddress	publicAddr = SlaveService->PublicAddr();
	CMessage		msgout(M_SS_SLAVEIDENT);
	msgout.serial(listenAddr, publicAddr);

	CUnifiedNetwork::getInstance()->send(sid, msgout);
}

// Packet : M_ENTER_SHARD_REQUEST
// Client -> SS
void	cbUserChooseShard( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb )
{
	ucstring	ucUserName;	msgin.serial(ucUserName);
	std::string 	sPassword;	msgin.serial(sPassword);
	uint32		dbID0;		msgin.serial(dbID0);
	uint32		shardID;	msgin.serial(shardID);

	CPlayer	info;
	T_ERR	errNo = getUserInfoFromAccountInfo(ucUserName, sPassword, info);
	uint32	userid = info.id;
	uint32	lastShardID = info.lastShardid;
	uint8	userTypeId	= info.usertypeid;
	
	if ( errNo != ERR_NOERR )
		if ( errNo == ERR_IMPROPERUSER || errNo == ERR_BADPASSWORD )
		{
			ERR_SHOW("Try to Enter Shard Failed! Wrong userAccountId or Wrong Password", errNo);
			SendFailMessageToClient(errNo, from, clientcb);
			return;
		}

	if ( userid != dbID0 )
	{
		//ERR_SHOW("Try to Enter Shard Failed! Wrong userDBID", ERR_AUTH);
		sipwarning(L"ERROR : Try to Enter Shard Failed! Wrong userDBID, ucUserID=%s, userid=%d, dbID0=%d", ucUserName.c_str(), userid, dbID0);
		SendFailMessageToClient(ERR_INVALIDCONNECTIONCOOKIE, from, clientcb);
		ForceDisconnectUser(clientcb, from, userid);
		return;
	}

	//socket ? from.Sock->_Sock ?
	errNo = checkPendingUserConnection(from, clientcb, userid);
	if ( errNo != ERR_NOERR )
	{
		sipwarning("Try to Enter Shard Failed! reson : Already disconnect user or unauthorized user! errNo %u", errNo);
		SendFailMessageToClient(errNo, from, clientcb);
		DisconnectUser(clientcb, from, userid);
		return;
	}

	errNo = checkUserPermission(userid, shardID);
	if ( errNo != ERR_NOERR )
	{
		sipwarning("Try to Enter Shard Failed! errNo %u", errNo);
		SendFailMessageToClient(errNo, from, clientcb);
		return;
	}

	uint8 bIsGM = false;
	_pmap::iterator it = ConnectPlayers.find(userid);
	if (it != ConnectPlayers.end())
	{
		CPlayer info = it->second;
		bIsGM = info.bGM;
	}

	std::pair<TONLINEGMSIT, TONLINEGMSIT>	range = g_onlineGMs.equal_range(userid);
	for ( TONLINEGMSIT it = range.first; it != range.second; it ++ )
	{
		uint32	idshard = (*it).second;
		if ( idshard == shardID )
		{
			errNo = ERR_ALREADYONLINE;
			sipwarning("Try to Enter Shard Failed! GM already online userId %u, errNo %u", userid, errNo);
			SendFailMessageToClient(errNo, from, clientcb);
			return;

			//CMessage	msgout2(M_SS_REVISELOGOFF);
			//msgout2.serial(userid);
			//msgout2.serial(idshard);
			//uint8	bIsGM = 1;
			//msgout2.serial(bIsGM);
			//g_onlineGMs.erase(it);

			//CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout2);
			//sipdebug("send user (id %u) <REVISELOGOFF> to LS, lastShardID : %u", userid, lastShardID);
			//
			//break;
		}
	}

	// CS OK!
	CMessage msgout(M_ENTER_SHARD_REQUEST);
	msgout.serial(userid);
#ifdef SIP_OS_UNIX
	msgout.serial((uint64 &)from);
#else
	msgout.serial((uint32 &)from);
#endif
	msgout.serial(shardID);
	msgout.serial(userTypeId);
	msgout.serial(ucUserName);
	msgout.serial(bIsGM);
	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);
}

// Packet : DC
// Client -> SS
void cbUserDisconnect( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb)
{
	uint32	userid;
	
	msgin.serial(userid);
	T_ERR errNo = checkPendingUserConnection(from, clientcb, userid);
	if ( errNo != ERR_NOERR )
        return;
	sipinfo("Recieved DisconnectionMessage <DC> from Client (userid : %u)", userid);

	DisconnectUser(clientcb, from, userid);
}

// must define in Common
BOOL	keyPair(string key)
{
	return true;
}

// Request to Check Enter Room ([d:RoomID])
void cb_M_CS_CHECK_FASTENTERROOM(CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb)
{
	uint32	roomid;		msgin.serial(roomid);
	uint32	userid = (uint32)from->appId();

	sipdebug("M_CS_CHECK_FASTENTERROOM received. RoomID=%d", roomid);
	CMessage	msgOut(M_CS_CHECK_FASTENTERROOM);
	msgOut.serial((uint32&)from, userid, roomid);
	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgOut);
}

// Check Enter Room Result ([d:ErrorCode][d:RoomID][d:ShardID])
void cb_M_SC_CHECK_FASTENTERROOM(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	CMessage	msgout;
	uint32		UserAddr;
	TTcpSockId	UserSock;

	msgin.copyWithoutPrefix(UserAddr, msgout);
	UserSock = reinterpret_cast<TSockId> (UserAddr);

	if ( SlaveService != NULL )
		SlaveService->send(msgout, UserSock);
	sipdebug("M_SC_CHECK_FASTENTERROOM send. UserAddr=%d", UserAddr);
}

void DisconnectAllClients()
{
	_pmap::iterator pUserIt;
	for (_pmap::iterator pUserIt = ConnectPlayers.begin(); pUserIt != ConnectPlayers.end(); pUserIt ++)
	{
		uint32		userid	= pUserIt->first;
		TTcpSockId	con		= pUserIt->second.con;

		SlaveService->Server()->disconnect(con);
		sipwarning(L"Client(UID=%d) is closed by force of DisconnectAllClients.", userid);
	}
}

SIPBASE_CATEGORISED_DYNVARIABLE(SS, uint32, Connections, "display number of all connections")
{
	if (get)
	{
		uint32 nbusers = ConnectPlayers.size();
		*pointer = nbusers;
	}
}

SIPBASE_CATEGORISED_COMMAND(SS, connectPlayers, "display information about all connect Players", "")
{
	int size = ConnectPlayers.size();
	sipinfo("Number of Pending Connected Players %u", size);

	_pmap::iterator pUserIt;
	for ( pUserIt = ConnectPlayers.begin(); pUserIt != ConnectPlayers.end(); pUserIt ++ )
	{
		uint32		userid	= pUserIt->first;
		TTcpSockId	con		= pUserIt->second.con;
		sipinfo("ConnectPlayer userid : %u, Sock : %s", userid, con->asString().c_str());
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(SS, stopUpdate, "Enter TestDelay Mode, Stop Update Frame", "<nbIncommingConnections>")
{
	if(args.size() != 1) return false;
	BoundIncommingConnections = atoi (args[0].c_str());
	if ( ! TestDelay.get() )
	{
		TestDelay.set(true);
		sipinfo("Enter To TestDelay mode!");
	}
	else
	{
		sipwarning("Already Enter To TestDelay mode!");
	}

	QueryConnections = 0;
	g_bLock = true;

	return	true;
}

SIPBASE_CATEGORISED_COMMAND(SS, startUpdate, "Leave TestDelay Mode, Start Update Frame", "")
{
	if ( TestDelay.get() )
	{
		TestDelay.set(false);
		sipinfo("Leave From TestDelay mode!");
	}
	else
	{
		sipwarning("Already Leave From Enter TestDelay mode!");
	}

	g_bLock = false;

	return	true;
}

SIPBASE_CATEGORISED_COMMAND(SS, onlineGM, "online GM List", "")
{
	for ( TONLINEGMSIT	it = g_onlineGMs.begin(); it != g_onlineGMs.end(); it ++ )
	{
		uint32	userid = (*it).first;
		uint32	shardid = (*it).second;
		sipinfo("UserId %u, ShardId %u", userid, shardid);
	}

	return	true;
}

SIPBASE_CATEGORISED_COMMAND(SS, LockSS, "Lock SS services. (Disconnect all users and users can't connect to SS.)", "")
{
	SSLockFlag = 1;

	DisconnectAllClients();

	return true;
}

SIPBASE_CATEGORISED_COMMAND(SS, UnlockSS, "Unlock SS services.", "")
{
	SSLockFlag = 0;
	return true;
}
