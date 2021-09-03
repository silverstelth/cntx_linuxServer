#define	NOMINIMAX

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#ifndef SIPNS_CONFIG
#define SIPNS_CONFIG ""
#endif // SIPNS_CONFIG

#ifndef SIPNS_LOGS
#define SIPNS_LOGS ""
#endif // SIPNS_LOGS

#include "misc/types_sip.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <list>

#include "misc/debug.h"
#include "misc/config_file.h"
#include "misc/displayer.h"
#include "misc/command.h"
#include "misc/variable.h"
#include "misc/log.h"
#include "misc/file.h"
#include "misc/path.h"
#include "net/service.h"
#include "misc/db_interface.h"
#include "misc/DBCaller.h"

#include "net/unified_network.h"
#include "net/login_cookie.h"

#include "../Common/DBData.h"
#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/netCommon.h"
#include "../Common/QuerySetForShard.h"
#include "../Common/Common.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

#include "welcome_service.h"

using namespace WS;

CDBCaller		*DBCaller = NULL;


// Forward declaration of callback cbShardOpen (see ShardOpen variable)
void	cbShardOpen(IVariable &var);
void	cbEnterUserType(IVariable &var);
void	cbShardName(IVariable &var);
void	cbShardComment(IVariable &var);
void	cbShardId(IVariable &var);
void	cbShardOpenStateFile(IVariable &var);
void	cbUsePatchMode(IVariable &var);

static bool	AllowDispatchMsgToLS = false;
static uint NbPlayers = 0;
uint8		ShardMainCode[25] = {0};	// ÇÏ³ªÀÇ Áö¿ªœá¹ö°¡ °¡ÁöŽÂ ShardCodežŠ 25°³°¡ ³ÑÁö ŸÊŽÂŽÙ°í °¡Á€ÇÔ.
ucstring	ShardCodeList;
std::map<string, ucstring>	g_mapServiceNameToShardCode;
std::map<uint8, string>		g_mapShardCodeToServiceName;

CVariable<sint>		PlayerLimit("ws","PlayerLimit", "Rough max number of players accepted on this shard (-1 for Unlimited)", 5000, 0, true );
CVariable<uint32>	ShardIdentifier("ws", "ShardIdentifier", "Indicates if shard identify number id", 0, // default : Admin Site
							0, true, cbShardId);
CVariable<uint8>	ShardOpen("ws", "ShardOpen", "Indicates if shard is open to public (1: Empty, 2: Good, 10: Full, 100: Restrict, 200: Close)", 1, 0, true, cbShardOpen);
CVariable<uint8>	EnterUserType("ws", "EnterUserType", "Indicates if shard is open to public (1: Common, 2: Middle, 10: Supper, 100: Private, 200: Developer)", 1, 0, true, cbEnterUserType);
CVariable<string>	ShardName("ws", "ShardName", "Indicates shard's name sended to client", "local", 0, true, cbShardName);
CVariable<string>	ShardComment("ws", "ShardComment", "Indicates shard's explaining content", "", 0, true, cbShardComment);
CVariable<uint>		ZoneID("ws", "ZoneID", "The nmber of Super Shards that this shards belong to", 0, 0, true);
CVariable<string>	ShardOpenStateFile("ws", "ShardOpenStateFile", "Name of the file that contains ShardOpen state", "", 0, true, cbShardOpenStateFile);
//CVariable<uint32>	VIPsNumber("ws", "VIPsNumber", "the number of VIPs", 0, 0, true);
CVariable<bool>		UsePatchMode("ws", "UsePatchMode", "Use Frontends as Patch servers (at FS startup)", true, 0, true, cbUsePatchMode );
CVariable<bool>		DontUseLS("ws", "DontUseLS", "Dont use the login service", false, 0, true);

//CCallbackServer		*WsCallbackServer = NULL;

uint8
	MOVE_USER_DATA_flagStart = 1, 
	MOVE_USER_DATA_flagEnd = 2, 
	MOVE_USER_DATA_flagContinue = 0;

uint8
	MOVE_USER_DATA_tbl_Family = 1,
	MOVE_USER_DATA_tbl_Family_ExpHis = 2,
	MOVE_USER_DATA_tbl_Character = 3,
	MOVE_USER_DATA_tbl_BlessCard = 4,
	MOVE_USER_DATA_tbl_Family_Virtue = 5,
	MOVE_USER_DATA_tbl_Item = 6,
	MOVE_USER_DATA_tbl_Family_RoomOrderInfo = 7,
	MOVE_USER_DATA_tbl_Family_Signin_Day = 8;

void DeleteNonusedFreeRooms();

/**
 * OpenFrontEndThreshold
 * The FS balance algorithm works like this:
 * - select the least loaded frontend
 * - if this frontend has more than the OpenFrontEndThreshold
 *   - try to open a new frontend
 *   - reselect least loaded frontend
 */
CVariable<uint>		OpenFrontEndThreshold("ws", "OpenFrontEndThreshold", "Limit number of players on all FS to decide to open a new FS", 800,	0, true );

/**
 * Using expected services and current running service instances, this class
 * reports a main "online status".
 */
class COnlineServices
{
public:

	/// Set expected instances. Ex: { "TICKS", "FS", "FS", "FS" }
	void		setExpectedInstances( CConfigFile::CVar& var )
	{
		// Reset "expected" counters (but don't clear the map, keep the running instances)
		CInstances::iterator ici;
		for ( ici=_Instances.begin(); ici!=_Instances.end(); ++ici )
		{
			(*ici).second.Expected = 0;
		}
		// Rebuild "expected" counters
		for ( uint i=0; i!=var.size(); ++i )
		{
			++_Instances[var.asString(i)].Expected;
		}
	}
	
	/// Add a service instance
	void		addInstance( const std::string& serviceName )
	{
		++_Instances[serviceName].Running;
	}

	/// Remove a service instance
	void		removeInstance( const std::string& serviceName ) 
	{
		CInstances::iterator ici = _Instances.find( serviceName );
		if ( ici != _Instances.end() )
		{
			--(*ici).second.Running;

			// Remove from the map only if not part of the expected list
			if ( ((*ici).second.Expected == 0) && ((*ici).second.Running == 0) )
			{
				_Instances.erase( ici );
			}
		}
		else
		{
			sipwarning( "Can't remove instance of %s", serviceName.c_str() );
		}
	}

	/// Check if all expected instances are online
	bool		getOnlineStatus() const
	{
		CInstances::const_iterator ici;
		for ( ici=_Instances.begin(); ici!=_Instances.end(); ++ici )
		{
			if ( ! ici->second.isOnlineAsExpected() )
				return false;
		}
		return true;
	}

	/// Display contents
	void		display( SIPBASE::CLog& log = *SIPBASE::DebugLog )
	{
		CInstances::const_iterator ici;
		for ( ici=_Instances.begin(); ici!=_Instances.end(); ++ici )
		{
			log.displayNL( "%s: %s (%u expected, %u running)",
				(*ici).first.c_str(),
				(*ici).second.Expected ? ((*ici).second.isOnlineAsExpected() ? "ONLINE" : "MISSING") : "OPTIONAL",
				(*ici).second.Expected, (*ici).second.Running );
		}
	}

private:

	struct TInstanceCounters
	{
		TInstanceCounters() : Expected(0), Running(0) {}

		// If not expected, count as online as well
		bool isOnlineAsExpected() const { return Running >= Expected; }

		uint	Expected;
		uint	Running;
	};

	typedef std::map< std::string, TInstanceCounters > CInstances;

	CInstances	_Instances;
};

/// Online services
COnlineServices OnlineServices;

/// Main online status
bool OnlineStatus;

/// Send changes of status to the LS
void reportOnlineStatus( bool newStatus )
{
	if ( newStatus != OnlineStatus && AllowDispatchMsgToLS )
	{
		if (!DontUseLS)
		{
			CMessage msgout(M_LS_OL_ST);
			msgout.serial( newStatus );
			CUnifiedNetwork::getInstance()->send( LS_S_NAME, msgout );
		}

		if (CWelcomeServiceMod::isInitialized())
		{
			// send a status report to welcome service client
			CWelcomeServiceMod::getInstance()->reportWSOpenState(newStatus);
		}

		OnlineStatus = newStatus;
	}
}


/// Set the version of the shard. you have to increase it each time the client-server protocol changes.
/// You have to increment the client too (the server and client version must be the same to run correctly)
static const uint32 ServerVersion = 1;

/// Contains the correspondance between userid and the FES connection where the userid is connected.
typedef		map<uint32, TServiceId>	TUSERIDASSOCIATION;
typedef		map<uint32, TServiceId>::iterator	TUSERIDASSOCIATIONIT;
TUSERIDASSOCIATION				UserIdSockAssociations;

typedef		multimap<uint32, TServiceId>	TGMASSOCIATION;
typedef		multimap<uint32, TServiceId>::iterator	TGMASSOCIATIONIT;
TGMASSOCIATION					GMUserIdSockAssociations;

// ubi hack
string FrontEndAddress;

SIPBASE::CVariable<std::string>		FrontendServiceName("WS", "FrontendServiceName", "Name of FES", "FS", 0, true);

enum TFESState
{
	PatchOnly,
	AcceptClientOnly
};
struct CFES
{
	CFES (TServiceId sid) : SId(sid), NbPendingUsers(0), NbEstimatedUser(0), NbUser(0), State(AcceptClientOnly) { }

	TServiceId	SId;				// Connection to the front end
	uint32		NbPendingUsers;		// Number of not yet connected users (but routed to this frontend)
	uint32		NbEstimatedUser;	// Number of user that already routed to this FES. This number could be different with the NbUser if
									// some users are not yet connected on the FES (used to equilibrate connection to all front end).
									// This number *never* decrease, it's just to fairly distribute user.
	uint32		NbUser;				// Number of user currently connected on this front end

	TFESState	State;				// State of frontend (patching/accepting clients)
	std::string	PatchAddress;		// Address of frontend patching server

	uint32		getUsersCountHeuristic() const
	{
		//return NbEstimatedUser;
		return NbUser + NbPendingUsers;
	}

	void		setToAcceptClients()
	{
		if (State == AcceptClientOnly)
			return;

		// tell FS to accept client
		State = AcceptClientOnly;
		CMessage	msgOpenFES(M_WS_FS_ACCEPT);
		CUnifiedNetwork::getInstance()->send(SId, msgOpenFES);

		// report state to LS
		bool	dummy;
		reportStateToLS(dummy, true);
	}

	void		reportStateToLS(bool& reportPatching, bool alive = true)
	{
		// report to LS

		bool	patching = (State == PatchOnly);
		if (alive && patching)
			reportPatching = true;

		if ( AllowDispatchMsgToLS )
		{
			if (!DontUseLS)
			{
				CMessage	msgout(M_LS_REPORT_FS_STATE);
				msgout.serial(SId);
				msgout.serial(alive);
				msgout.serial(patching);
				msgout.serial(PatchAddress);
				CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);
			}
		}
	}
};

list<CFES> FESList;


//
// Variables
//

SIPBASE_CATEGORISED_DYNVARIABLE(WS, uint32, OnlineUsersNumber, "number of connected users on this shard")
{
	// we can only read the value
	if (get)
	{
		uint32 nbusers = 0;
		for (list<CFES>::iterator it = FESList.begin(); it != FESList.end (); it++)
		{
			nbusers += (*it).NbUser;
		}
		*pointer = nbusers;
	}
}

/*
 * Find the best front end service for a new connecting user (return NULL if there is no suitable FES).
 * Additionally, calculate totalNbUsers.
 */
CFES *findBestFES ( uint& totalNbUsers )
{
	totalNbUsers = 0;

	CFES*	best = NULL;

	for (list<CFES>::iterator it=FESList.begin(); it!=FESList.end(); ++it)
	{
		CFES &fes = *it;
		if (fes.State == AcceptClientOnly)
		{
			if (best == NULL || best->getUsersCountHeuristic() > fes.getUsersCountHeuristic())
				best = &fes;

			totalNbUsers += fes.NbUser;
		}

	}

	return best;
}

/**
 * Select a frontend in patch mode to open
 * Returns true if a new FES was open, false if no FES could be open
 */
bool	openNewFES()
{
	for (list<CFES>::iterator it=FESList.begin(); it!=FESList.end(); ++it)
	{
		if ((*it).State == PatchOnly)
		{
			sipinfo("openNewFES: ask the FS %d to accept clients", it->SId.get());

			// switch FES to AcceptClientOnly
			(*it).setToAcceptClients();
			return true;
		}
	}

	return false;
}



void displayFES ()
{
	//sipinfo ("There's %d FES in the list:", FESList.size()); byy krs
	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		//sipinfo(" > %u NbUser:%d NbEstUser:%d", (*it).SId.get(), (*it).NbUser, (*it).NbEstimatedUser); byy krs
	}
	//sipinfo ("End of the list"); byy krs
}

/*
* Set Shard open state
*/
void	NotifyShardOpenState(ENUM_SHARDSTATEDEFINE state, ENUM_USERTYPE usertype, bool writeInVar = true)
{
	if (writeInVar)
	{
		ShardOpen.set(state);
		EnterUserType.set(usertype);
		//sipdebug("Shard OpenState:%u, EnterUserType:%u", state, usertype); byy krs
	}

	if ( AllowDispatchMsgToLS )
	{
		if (!DontUseLS)
		{
			// send to LS current shard state
			CMessage	msgout (M_LS_SET_SHARD_OPEN);
			uint8		shardOpenState	= (uint8)state;		msgout.serial (shardOpenState);
			uint8		enterUserType	= (uint8)usertype;	msgout.serial (enterUserType);
			CUnifiedNetwork::getInstance()->send (LS_S_NAME, msgout);
			//sipdebug("Shard OpenState:%u, EnterUserType:%u", shardOpenState, enterUserType); byy krs
		}
	}
}

// Common functions
MSSQLTIME		ServerStartTime_DB;
TTime			ServerStartTime_Local;

uint32	getMinutesSince1970(std::string& date)
{
	MSSQLTIME	t;
	if (!t.SetTime(date))
		return 0;
	uint32	result = (uint32)(t.timevalue / 60);
	return result;
}

uint32	getSecondsSince1970(std::string& date)
{
	MSSQLTIME	t;
	if (!t.SetTime(date))
		return 0;
	return (uint32)t.timevalue;
}

TTime	GetDBTimeSecond()
{
	TTime	workingtime = (CTime::getSecondsSince1970() - ServerStartTime_Local);
	TTime	curtime = ServerStartTime_DB.timevalue + workingtime;

	return curtime;
}

/**************************************************************************************************
//
//					Callback From FS
//
**************************************************************************************************/

// Packet : M_ENTER_SHARD_ANSWER
// FES -> WS
void cbFESShardChooseShard (CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	// the WS answer a user authorize
	T_ERR			wErrNo;
	CLoginCookie	cookie;
	string			addr;

	//
	// S09: receive "SCS" message from FES and send the "SCS" message to the LS
	//
	
	CMessage msgout (M_ENTER_SHARD_ANSWER);

	msgin.serial (wErrNo);	msgout.serial (wErrNo);
	msgin.serial (cookie);	msgout.serial (cookie);
	
	if ( wErrNo == ERR_NOERR )
	{
		msgin.serial (addr);

		// if we set the FontEndAddress in the welcome_service.cfg we use this address
		if (FrontEndAddress.empty())
			msgout.serial (addr);
		else
			msgout.serial (FrontEndAddress);
	}
	//sipdebug( "SCS recvd from %s-%hu, cookie %s, wErrNo=%d", serviceName.c_str(), sid.get(), cookie.toString().c_str(), wErrNo); byy krs

	// return the result to the LS
	if (!DontUseLS)
	{
		CUnifiedNetwork::getInstance()->send (LS_S_NAME, msgout);
	}
/*

	if (PendingFeResponse.find(cookie) != PendingFeResponse.end())
	{
		sipdebug( "ERLOG: SCS recvd from %s-%hu => sending %s to SU", serviceName.c_str(), sid.get(), cookie.toString().c_str());

		// this response is not waited by LS
		TPendingFEResponseInfo &pfri = PendingFeResponse.find(cookie)->second;

		pfri.WSMod->frontendResponse(pfri.WaiterModule, pfri.UserId, wErrNo, cookie, addr);
		// cleanup pending record
		PendingFeResponse.erase(cookie);
	}
	else
	{
		sipdebug( "ERLOG: SCS recvd from %s-%hu, but pending %s not found", serviceName.c_str(), sid.get(), cookie.toString().c_str());

		// return the result to the LS
		if (!DontUseLS)
		{
			CUnifiedNetwork::getInstance()->send (LS_S_NAME, msgout);
		}
	}	
	*/
}

static void	DBCB_DBFamilyLogout(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ACCOUNTID			userid	= *(T_ACCOUNTID *)argv[0];
	uint8				bIsGM	= *(uint8 *)argv[1];

	if ( !nQueryResult )
		return;

	T_F_VISITDAYS	VisitDays = 0;
	T_F_EXP			lUserExp = 0;
	T_F_VIRTUE		UserVirtue = 0;
	T_F_FAME		UserFame = 0;
	uint32			nGMoney = 0;
	uint32			nRooms = 0;		
	uint8			ExpRoom = 0;
	bool			bRightValue = false;
	uint32 nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		resSet->serial(VisitDays);
		resSet->serial(lUserExp);
//		resSet->serial(UserVirtue);
		resSet->serial(UserFame);
		resSet->serial(nGMoney);
		resSet->serial(nRooms);
		bRightValue = true;
	}
	uint16 UserExp = (uint16)lUserExp;

	if (!DontUseLS)
	{
		uint8 con = 0;
		uint32	shardId = ShardIdentifier.get();

		CMessage msgout (M_CLIENT_CONNECT_STATUS);
		msgout.serial(userid, con, bIsGM, shardId);
		msgout.serial(VisitDays, UserExp, UserVirtue, UserFame, nRooms, ExpRoom, nGMoney, bRightValue);

		CUnifiedNetwork::getInstance()->send (LS_S_NAME, msgout);

		//sipinfo("Send LS <LOGOFF> ShardId : %u, GMFlag :%u, FamilyID %u, UserExp %u, UserVirtue %u, UserFame %u, RoomNum %u, ExpRoom %u", shardId, bIsGM, userid, UserExp, UserVirtue, UserFame, nRooms, ExpRoom); byy krs
	}
}

static void LogoffClient(uint32 userid, TServiceId sid, uint8 bIsGM)
{
	// add or remove the user number really connected on this shard
	uint32 totalNbOnlineUsers = 0, totalNbPendingUsers = 0;
	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		if (it->SId == sid)
		{
			if ( (*it).NbUser != 0 )
				(*it).NbUser--;

			NbPlayers --;
		}
		totalNbOnlineUsers += (*it).NbUser;
		totalNbPendingUsers += (*it).NbPendingUsers;
	}

	if (CWelcomeServiceMod::isInitialized())
	{
		CWelcomeServiceMod::getInstance()->updateConnectedPlayerCount(totalNbOnlineUsers, totalNbPendingUsers);
	}

	// remove the user
	UserIdSockAssociations.erase (userid);
	uint32 nCount = static_cast<uint32>(GMUserIdSockAssociations.count(userid));
	if (nCount > 0)
	{
		bIsGM = true;
		GMUserIdSockAssociations.erase(userid);
	}

	//uint32	VisitDays = 0;
	//uint32	lUserExp = 0;
	//uint16	UserVirtue = 0;
	//uint16	UserFame = 0;
	//uint32	nRooms = 0;
	//uint8	ExpRoom = 0;

	MAKEQUERY_FAMILYLOGOUT(strQuery, userid);
	DBCaller->execute(strQuery, DBCB_DBFamilyLogout,
		"DB",
		CB_,		userid,
		CB_,		bIsGM);
}

// Packet : M_CLIENT_CONNECT_STATUS - Login
static void	LoginClient(CMessage msgin, uint32 userid, TServiceId sid)
{
	CMessage msgout (M_CLIENT_CONNECT_STATUS);
	
	msgout.serial (userid);
	uint8 con = 1;	msgout.serial (con);

	std::string		strIP;		msgin.serial(strIP);	msgout.serial(strIP);
	CLoginCookie	cookie;		msgin.serial(cookie);	msgout.serial(cookie);

	uint8	bIsGM = cookie.getIsGM();
	ucstring	ucUserName(cookie.getUserName().c_str());

	MAKEQUERY_FAMILYLOGIN(strQuery, userid);
	CDBConnectionRest	DBConnection(DBCaller);
   	DB_EXE_QUERY_WITHONEROW(DBConnection, Stmt, strQuery);
	if ( !nQueryResult )
	{
		sipwarning("Query Failed! : %S.", strQuery);
		LogoffClient(userid, sid, bIsGM);
		sipinfo("While process <CC>(connection) error occured in saving active data! userid:%u, <CC>(con) -><CC>(disconnection) to LS", userid);

		CMessage msgout1 (M_CLIENT_DISCONNECT);
		msgout1.serial (userid);
		CUnifiedNetwork::getInstance()->send(sid, msgout1);
		sipinfo("Send FS <DC> Disconnect Same Account userid:%u", userid);

		return;
	}
	COLUMN_DIGIT(row, 0, uint32, nResult);
	COLUMN_DIGIT(row, 1, uint32, nGMoney);

	T_ERR	err = ERR_NOERR;
    switch ( nResult )
	{
		case 0:
			err = ERR_NOERR;
			//RESULT_SHOW(strQuery, ERR_NOERR); byy krs
			break;
		case 10:
			err = E_DBERR;
			ERR_SHOW(strQuery, E_DBERR);
			break;
		default:
			sipwarning(L"Unknown Error Query <%s>: %u", strQuery, nResult);
			break;
	}

	if ( err != ERR_NOERR )
	{
		LogoffClient(userid, sid, cookie.getIsGM());
		sipinfo("While process <CC>(connection) error occured! userid:%u, <CC>(con) -><CC>(disconnection) to LS", userid);

		CMessage msgout1 (M_CLIENT_DISCONNECT);
		msgout1.serial (userid);
		CUnifiedNetwork::getInstance()->send(sid, msgout1);
		sipinfo("Send FS <DC> Disconnect Same Account userid:%u", userid);

		return;
	}

	if (!DontUseLS)
	{
		msgout.serial(nGMoney);
		uint16	fsSid = sid.get();
		msgout.serial(fsSid);
		CUnifiedNetwork::getInstance()->send (LS_S_NAME, msgout);
	}

	// add or remove the user number really connected on this shard
	uint32 totalNbOnlineUsers = 0, totalNbPendingUsers = 0;
	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		if (it->SId == sid)
		{
			(*it).NbUser++;

			// the client connected, it's no longer pending
			if ((*it).NbPendingUsers > 0)
				(*it).NbPendingUsers--;

			NbPlayers ++;
		}
		totalNbOnlineUsers += (*it).NbUser;
		totalNbPendingUsers += (*it).NbPendingUsers;
	}

	if (CWelcomeServiceMod::isInitialized())
	{
		CWelcomeServiceMod::getInstance()->updateConnectedPlayerCount(totalNbOnlineUsers, totalNbPendingUsers);
	}

	UserIdSockAssociations.insert (make_pair (userid, sid));
	if ( bIsGM )
		GMUserIdSockAssociations.insert(make_pair (userid, sid));
}

// Packet : M_CLIENT_CONNECT_STATUS
// FS -> WS
// This function is call when a FS accepted a new client or lost a connection to a client
void cb_M_CLIENT_CONNECT_STATUS(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	uint32	userid;		msgin.serial (userid);
	uint8	con;		msgin.serial (con);
	if ( con )
	{
		//sipinfo("<CC> Connect Client userid : %u", userid); byy krs
		LoginClient(msgin, userid, sid);
	}
	else
	{
		//sipinfo("<CC> DisConnect Client userid : %u", userid); byy krs
		uint8	bIsGM;		msgin.serial (bIsGM);
		LogoffClient(userid, sid, bIsGM);
	}
}

struct	UserMoneyInfo
{
	uint32	_msgid;
	uint32	_userid;
	TServiceId	_serviceid;

	UserMoneyInfo() {};
	UserMoneyInfo(uint32 msgid, uint32 userid, TServiceId sid)
	{
		_msgid	= msgid;
		_userid	= userid;
		_serviceid	= sid;
	}
};
typedef	std::map<uint32, UserMoneyInfo>				ARYUSERMONEY;
typedef	std::map<uint32, UserMoneyInfo>::iterator	ITARYUSERMONEY;
ARYUSERMONEY	aryUserMoney;


// Packet : M_LS_EXPENDMONEY
// FES -> WS
void cbFESExpendMoney (CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	CMessage	msgout(M_LS_EXPENDMONEY);
	
	uint8	flag;			msgin.serial(flag);			msgout.serial(flag);
	uint32	money;			msgin.serial(money);		msgout.serial(money);
	uint16	maintype;		msgin.serial(maintype);		msgout.serial(maintype);
    uint8	subtype;		msgin.serial(subtype);		msgout.serial(subtype);
	uint32	userid;			msgin.serial(userid);		msgout.serial(userid);
	uint8	usertypeid;		msgin.serial(usertypeid);	msgout.serial(usertypeid);
	uint32	familyid;		msgin.serial(familyid);		msgout.serial(familyid);
	uint32	itemid;			msgin.serial(itemid);		msgout.serial(itemid);
	uint32	itemtypeid;		msgin.serial(itemtypeid);	msgout.serial(itemtypeid);
	uint32	v1;				msgin.serial(v1);			msgout.serial(v1);
	uint32	v2;				msgin.serial(v2);			msgout.serial(v2);
	uint32	v3;				msgin.serial(v3);			msgout.serial(v3);
	uint32	shardId = ShardIdentifier.get();			msgout.serial(shardId);

	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);
	//sipinfo("User(id:%u) consume(type :%u) Money(%u)", userid, flag, money); byy krs

	/*UserMoneyInfo	info(msgid, userid, sid);
	aryUserMoney.insert(std::make_pair(msgid, info));*/
}

//// callback function for the "CANUSEMONEY" from OS
//void cbOSCanUseMoney(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
//{
//	CMessage msgout(M_LS_CANUSEMONEY);
//	uint32	msgid;			msgin.serial(msgid);		msgout.serial(msgid);
//	uint8	flag;			msgin.serial(flag);			msgout.serial(flag);
//	uint32	money;			msgin.serial(money);		msgout.serial(money);
//	uint32  userid;         msgin.serial(userid);       msgout.serial(userid);
//
//	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);
//
//	UserMoneyInfo	info(msgid, userid, sid);
//	aryUserMoney.insert(std::make_pair(msgid, info));
//}

// Packet : RPC
// FES -> WS
// This function is called when a FES rejected a client' cookie
void	cbFESRemovedPendingCookie(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	CLoginCookie	cookie;
	msgin.serial(cookie);
	//sipdebug( "ERLOG: RPC recvd from %s-%hu => %s removed", serviceName.c_str(), sid.get(), cookie.toString().c_str(), cookie.toString().c_str()); byy krs

	// client' cookie rejected, no longer pending
	uint32 totalNbOnlineUsers = 0, totalNbPendingUsers = 0;
	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		if ((*it).SId == sid)
		{
			if ((*it).NbPendingUsers > 0)
				--(*it).NbPendingUsers;
		}
		totalNbOnlineUsers += (*it).NbUser;
		totalNbPendingUsers += (*it).NbPendingUsers;
	}

	if (CWelcomeServiceMod::isInitialized())
	{
		CWelcomeServiceMod::getInstance()->pendingUserLost(cookie);
		CWelcomeServiceMod::getInstance()->updateConnectedPlayerCount(totalNbOnlineUsers, totalNbPendingUsers);
	}

	CMessage msgout(M_LOGIN_SERVER_RPC);
	msgout.serial(cookie);
	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);
}

// This function is called by FES to setup its PatchAddress
void	cbFESPatchAddress(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	std::string	address;
	msgin.serial(address);

	bool		acceptClients;
	msgin.serial(acceptClients);

	//sipdebug("Received patch server address '%s' from service %s %d", address.c_str(), serviceName.c_str(), sid.get()); byy krs

	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		if ((*it).SId == sid)
		{
			//sipdebug("Affected patch server address '%s' to frontend %s %d", address.c_str(), serviceName.c_str(), sid.get()); byy krs

			if (!UsePatchMode.get() && !acceptClients)
			{
				// not in patch mode, force fs to accept clients
				acceptClients = true;
				(*it).setToAcceptClients();
			}

			(*it).PatchAddress = address;
			(*it).State = (acceptClients ? AcceptClientOnly : PatchOnly);
			//if (acceptClients) byy krs
			//	sipdebug("Frontend %s %d reported to accept client, patching unavailable for that server", address.c_str(), serviceName.c_str(), sid.get());
			//else
			//	sipdebug("Frontend %s %d reported to be in patching mode", address.c_str(), serviceName.c_str(), sid.get());

			bool	dummy;
			(*it).reportStateToLS(dummy);
			break;
		}
	}
}

// This function is called by FES to setup the right number of players (if FES was already present before WS launching)
void	cbFESNbPlayers(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	// *********** WARNING *******************
	// This version of the callback is deprecated, the system
	// now use cbFESNbPlayers2 that report the pending user count
	// as well as the number of connected players.
	// It is kept for backward compatibility only.
	// ***************************************

	uint32	nbPlayers;
	msgin.serial(nbPlayers);

	uint32 totalNbOnlineUsers = 0, totalNbPendingUsers = 0;
	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		if ((*it).SId == sid)
		{
			//sipdebug("Frontend '%d' reported %d online users", sid.get(), nbPlayers); byy krs
			(*it).NbUser = nbPlayers;
			if (nbPlayers != 0 && (*it).State == PatchOnly)
			{
				//sipwarning("Frontend %d is in state PatchOnly, yet reports to have online %d players, state AcceptClientOnly is forced (FS_ACCEPT message sent)"); byy krs
				(*it).setToAcceptClients();
			}
		}
		totalNbOnlineUsers += (*it).NbUser;
		totalNbPendingUsers += (*it).NbPendingUsers;
	}

	if (CWelcomeServiceMod::isInitialized())
	{
		CWelcomeServiceMod::getInstance()->updateConnectedPlayerCount(totalNbOnlineUsers, totalNbPendingUsers);
	}
}


// This function is called by FES to setup the right number of players (if FES was already present before WS launching)
void	cbFESNbPlayers2(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	uint32	nbPlayers;
	uint32	nbPendingPlayers;
	msgin.serial(nbPlayers);
	msgin.serial(nbPendingPlayers);

	uint32 totalNbOnlineUsers = 0, totalNbPendingUsers = 0;
	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		CFES &fes = *it;
		if (fes.SId == sid)
		{
			//sipdebug("Frontend '%d' reported %d online users", sid.get(), nbPlayers); byy krs
			fes.NbUser = nbPlayers;
			fes.NbPendingUsers = nbPendingPlayers;
			if (nbPlayers != 0 && fes.State == PatchOnly)
			{
				//sipwarning("Frontend %d is in state PatchOnly, yet reports to have online %d players, state AcceptClientOnly is forced (FS_ACCEPT message sent)"); byy krs
				(*it).setToAcceptClients();
			}
		}
		totalNbOnlineUsers += fes.NbUser;
		totalNbPendingUsers += fes.NbPendingUsers;
	}

	if (CWelcomeServiceMod::isInitialized())
	{
		CWelcomeServiceMod::getInstance()->updateConnectedPlayerCount(totalNbOnlineUsers, totalNbPendingUsers);
	}
}

// forward declaration to callback
void cbShardOpenStateFile(IVariable &var);

///*
// * Restore Shard Open state from config file or from file if found
// */
//void cbRestoreShardOpen(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
//{
//	// first restore state from config file
//	CConfigFile::CVar*		var;
//	var	= IService::getInstance()->ConfigFile.getVarPtr("ShardOpen");
//	ENUM_SHARDSTATEDEFINE	shardopen		= (ENUM_SHARDSTATEDEFINE) var->asInt();
//	var	= IService::getInstance()->ConfigFile.getVarPtr("EnterUserType");
//	ENUM_USERTYPE			enterusertype	= (ENUM_USERTYPE) var->asInt();
//	if (var != NULL)
//	{
//		NotifyShardOpenState(shardopen, enterusertype);
//	}
//
//	// then restore state from state file, if it exists
//	cbShardOpenStateFile(ShardOpenStateFile);
//}

void MakeShardCodeToServiceNameMap(const std::string &serviceName, bool bAdd)
{
	if (g_mapServiceNameToShardCode.find(serviceName) == g_mapServiceNameToShardCode.end())
		return;

	ucstring	ucShardCodeList = g_mapServiceNameToShardCode[serviceName];

	uint8	ShardCode;
	ucstring	strShardCode;
	int		pos;
	do
	{
		pos = ucShardCodeList.find(L",");
		if(pos >= 0)
		{
			strShardCode = ucShardCodeList.substr(0, pos);
			ucShardCodeList = ucShardCodeList.substr(pos + 1);
		}
		else
		{
			strShardCode = ucShardCodeList;
		}
		// ShardCode = (uint8)_wtoi(strShardCode.c_str());
		ShardCode = (uint8)std::wcstod(strShardCode.c_str(), NULL);
		if(ShardCode > 0)
		{
			if (bAdd)
			{
				std::map<uint8, string>::iterator it = g_mapShardCodeToServiceName.find(ShardCode);
				if(it != g_mapShardCodeToServiceName.end())
				{
					if (it->second != serviceName)
					{
						siperror("Same ShardCode exist!!! ShardCode=%d, oldServiceName=%s, newServiceName=%s", ShardCode, it->second.c_str(), serviceName.c_str());
						break;
					}
				}
				g_mapShardCodeToServiceName[ShardCode] = serviceName;
				//sipinfo("Add ShardCode=%d, serviceName=%s", ShardCode, serviceName.c_str()); byy krs
			}
			else
			{
				g_mapShardCodeToServiceName.erase(ShardCode);
				//sipinfo("Erase ShardCode=%d", ShardCode); byy krs
			}
		}
		else
		{
			siperror("Invalid ShardCode!!! ShardCode=%d", ShardCode);
			break;
		}
	} while(pos >= 0);
}

string GetShardNameFromShardCode(uint8 ShardCode)
{
	std::map<uint8, string>::iterator it = g_mapShardCodeToServiceName.find(ShardCode);
	if (it == g_mapShardCodeToServiceName.end())
	{
		sipwarning("Can't find ShardName of ShardCode. ShardCode=%d", ShardCode);
		return "";
	}
	return it->second;
}

string GetWSNameFromShardID(uint32 ShardID)
{
	return SIPBASE::toString("%s%d", WELCOME_SHORT_NAME, ShardID);
}

bool IsThisShard(uint8 ShardCode)
{
	std::map<uint8, string>::iterator it = g_mapShardCodeToServiceName.find(ShardCode);
	if (it == g_mapShardCodeToServiceName.end())
	{
		sipwarning("Can't find ShardName of ShardCode. ShardCode=%d", ShardCode);
		return false;
	}
	if (it->second == "0")
		return true;
	return false;
}

bool IsThisShardRoom(T_ROOMID RoomID)
{
	if (RoomID < 0x01000000)
		return true;

	uint8	ShardCode = (uint8)(RoomID >> 24);

	return IsThisShard(ShardCode);
}

bool IsThisShardFID(T_FAMILYID FID)
{
	uint8	ShardCode = (uint8)((FID >> 24) + 1);

	return IsThisShard(ShardCode);
}

void SendMsgToShardByRoomID(CMessage &msgOut, T_ROOMID RoomID)
{
	uint8	ShardCode = (uint8)(RoomID >> 24);
	string strShardWsName = GetShardNameFromShardCode(ShardCode);

	if (strShardWsName != "")
		CUnifiedNetwork::getInstance()->send(strShardWsName, msgOut);
}

void cbWSConnection(const std::string &serviceName, TServiceId  sid, void *arg)
{
	MakeShardCodeToServiceNameMap(serviceName, true);
}

void cbWSDisconnection(const std::string &serviceName, TServiceId  sid, void *arg)
{
	MakeShardCodeToServiceNameMap(serviceName, false);
}

//void SendShardInfoToManagerService()
//{
//	CMessage	msgOut(M_WM_SHARDCODELIST);
//	msgOut.serial(ShardCodeList);
//	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
//}

void cbMSConnection (const std::string &serviceName, TServiceId  sid, void *arg)
{
	//SendShardInfoToManagerService();
}

// packet : M_LW_SHARDINFO
void	cbShardInfo(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	uint32		shardId;		msgin.serial(shardId);
	ucstring	ucStrName;		msgin.serial(ucStrName);
	uint8		zoneId;			msgin.serial(zoneId);
	uint32		nUserLimit;		msgin.serial(nUserLimit);
	ucstring	ucContent;		msgin.serial(ucContent);
	uint8		activeflag;		msgin.serial(activeflag);
	uint32		nbPlayers;		msgin.serial(nbPlayers);
	msgin.serial(ShardMainCode[0]);
	msgin.serial(ShardCodeList);

	//sipinfo(L"Receive Shard Info. ShardId=%d, ShardName=%s, ShardMainCode=%d, ShardCodeList=%s", shardId, ucStrName.c_str(), ShardMainCode[0], ShardCodeList.c_str()); byy krs

	uint8	ShardCode;
	ucstring	strShardCode, originShardCodeList;
	originShardCodeList = ShardCodeList;
	int		pos;
	int		count = 0;
	do
	{
		pos = ShardCodeList.find(L",");
		if(pos >= 0)
		{
			strShardCode = ShardCodeList.substr(0, pos);
			ShardCodeList = ShardCodeList.substr(pos + 1);
		}
		else
		{
			strShardCode = ShardCodeList;
		}
		// ShardCode = (uint8)_wtoi(strShardCode.c_str());
		ShardCode = (uint8)std::wcstod(strShardCode.c_str(), NULL);

		ShardMainCode[count] = ShardCode;
		count ++;
	} while(pos >= 0);
	ShardMainCode[count] = 0;

	ShardIdentifier.set(shardId);
	ShardName.set(ucStrName.toUtf8());
	ZoneID.set(zoneId);
	PlayerLimit.set(nUserLimit);
	ShardComment.set(ucContent.toUtf8());
	NbPlayers = nbPlayers;

	CMessage	msgOut(M_W_SHARDCODE);
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		msgOut.serial(ShardMainCode[i]);
		if (ShardMainCode[i] == 0)
			break;
	}
	CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, msgOut);
	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgOut);
	CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, msgOut);
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
	CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgOut);

	string	sMyName = "0";
	g_mapServiceNameToShardCode[sMyName] = originShardCodeList;
	MakeShardCodeToServiceNameMap(sMyName, true);

	// Shard DB ÃÊ±âÈ­
	{
		MAKEQUERY_InitDB(strQuery, ShardMainCode[0]);
		CDBConnectionRest	DBConnection(DBCaller);
		DB_EXE_QUERY(DBConnection, Stmt, strQuery);
	}

	//// Manager Service¿¡ ShardCodeListžŠ ÅëÁöÇÏ¿© ŽÙž¥ Áö¿ªÀÇ ¹æµéÀº Ã¢Á¶µÇÁö ŸÊ°Ô ÇÑŽÙ.
	//SendShardInfoToManagerService();

	// 3°³¿ùµ¿ŸÈ »ç¿ëµÇÁö ŸÊÀº ¹«·á¹æÀ» »èÁŠÇÑŽÙ.
	DeleteNonusedFreeRooms();
}

// packet : M_LW_OTHER_SHARD
void	cbOtherShardInfo(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	string		sServiceName;		msgin.serial(sServiceName);
	string		sNSAddress;			msgin.serial(sNSAddress);
	ucstring	ucShardCodeList;	msgin.serial(ucShardCodeList);
	uint8		bPrevOn;			msgin.serial(bPrevOn);

	//sipinfo("Receive Other Shard Info. ServiceName=%s, NSAddress=%s, bPrevOn=%d", sServiceName.c_str(), sNSAddress.c_str(), bPrevOn); byy krs

	bool	bIsRetry = false;
	if (g_mapServiceNameToShardCode.find(sServiceName) != g_mapServiceNameToShardCode.end())
		bIsRetry = true;

	g_mapServiceNameToShardCode[sServiceName] = ucShardCodeList;
	MakeShardCodeToServiceNameMap(sServiceName, true);

	if (bPrevOn && !bIsRetry)
	{
		CUnifiedNetwork::getInstance()->addService(sServiceName, sNSAddress);
	}
}

// packet : M_LW_OTHER_SHARD_OFF
void	cbOtherShardOff(CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	string		sServiceName;		msgin.serial(sServiceName);

	//sipinfo("Receive Other Shard Offline. ServiceName=%s", sServiceName.c_str()); byy krs

	if (g_mapServiceNameToShardCode.find(sServiceName) != g_mapServiceNameToShardCode.end())
	{
		g_mapServiceNameToShardCode.erase(sServiceName);

		for (std::map<uint8, string>::iterator it = g_mapShardCodeToServiceName.begin(); it != g_mapShardCodeToServiceName.end(); )
		{
			if (it->second == sServiceName)
				it = g_mapShardCodeToServiceName.erase(it);
			else
				it ++;
		}
	}

	CUnifiedNetwork::getInstance()->closeWSConnection(sServiceName);
}

// a new front end connecting to me, add it
void cbFESConnection (const std::string &serviceName, TServiceId  sid, void *arg)
{
	FESList.push_back (CFES ((TServiceId)sid));
	//sipdebug("new FES connection: sid %u", sid.get()); byy krs
	displayFES ();

	bool	dummy;
	FESList.back().reportStateToLS(dummy);

	// Reset NbEstimatedUser to NbUser for all front-ends
	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		(*it).NbEstimatedUser = (*it).NbUser;
	}

	if (!UsePatchMode.get())
	{
		FESList.back().setToAcceptClients();
	}
}


// a front end closes the connection, deconnect him
void cbFESDisconnection (const std::string &serviceName, TServiceId  sid, void *arg)
{
	//sipdebug("FES (%s) disconnection: sid %u", serviceName.c_str(), sid.get()); byy krs

	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end(); it++)
	{
		if ((*it).SId == sid)
		{
			// send a message to the LS to say that all players from this FES are offline
			TUSERIDASSOCIATION::iterator itc = UserIdSockAssociations.begin();
			TUSERIDASSOCIATION::iterator nitc = itc;
			while (itc != UserIdSockAssociations.end())
			{
				nitc++;
				if ((*itc).second == sid)
				{
					// bye bye little player
					uint32 userid = (*itc).first;
					//sipinfo ("Due to a frontend crash, removed the player %d", userid); byy krs
					if (!DontUseLS)
					{
						LogoffClient(userid, sid, false);
						//sipdebug("Disconnect FES! logoff user(id:%u)", userid); byy krs
					}
					else
					{
						UserIdSockAssociations.erase (itc);
					}
				}
				itc = nitc;
			}

			TGMASSOCIATION::iterator itgm = GMUserIdSockAssociations.begin();
			TGMASSOCIATION::iterator nitgm = itgm;
			while (itgm != GMUserIdSockAssociations.end())
			{
				nitgm++;
				if ((*itgm).second == sid)
				{
					// bye bye little player
					uint32 userid = (*itgm).first;
					//sipinfo ("Due to a frontend crash, removed the player %d", userid); byy krs
					if (!DontUseLS)
					{
						LogoffClient(userid, sid, true);
						//sipdebug("Disconnect FES! logoff user(id:%u)", userid); byy krs
					}
					else
					{
						GMUserIdSockAssociations.erase (itgm);
					}
				}
				itgm = nitgm;
			}

			bool	dummy;
			(*it).reportStateToLS(dummy, false);

			// remove the FES
			FESList.erase (it);

			break;
		}
	}

	displayFES ();
}


//
void cbServiceUp (const std::string &serviceName, TServiceId  sid, void *arg)
{
	if (sid.get() >= 256)	// if ExtSid, don't do this
		return;

	OnlineServices.addInstance( serviceName );
	bool online = OnlineServices.getOnlineStatus();
	reportOnlineStatus( online );

	// send shard id to service
	sint32 shardId;
	if (IService::getInstance()->haveArg('S'))
	{
		// use the command line param if set
		shardId = atoi(IService::getInstance()->getArg('S').c_str());
	}
	else if (IService::getInstance()->ConfigFile.exists ("ShardIdentifier"))
	{
		// use the config file param if set
		shardId = IService::getInstance()->ConfigFile.getVar ("ShardIdentifier").asInt();
	}
	else
	{
		shardId = DEFAULT_SHARD_ID;
	}

	if (shardId == DEFAULT_SHARD_ID)
	{
		siperror ("ShardIdentifier variable must be valid (>0)");
	}

	CMessage	msgout(M_SERVICE_R_SH_ID);
	msgout.serial(shardId);
	CUnifiedNetwork::getInstance()->send (sid, msgout);

	// ÇöÀç ÁøÇàÁßÀÎ ±â³äÀÏÈ°µ¿ÀÚ·ážŠ ºž³œŽÙ.
	extern void SendCurrentActivityData(const std::string &serviceName, TServiceId tsid);
	SendCurrentActivityData(serviceName, sid);
	extern void SendBeginnerMstItemToService(const std::string &serviceName, TServiceId tsid);
	SendBeginnerMstItemToService(serviceName, sid);

	extern void SendCurrentCheckInData(TServiceId tsid);
	if (strstr(serviceName.c_str(), MANAGER_SHORT_NAME))
	{
		SendCurrentCheckInData(sid);
	}
}


//
void cbServiceDown (const std::string &serviceName, TServiceId  sid, void *arg)
{
	if (sid.get() >= 256)	// if ExtSid, don't do this
		return;

	OnlineServices.removeInstance( serviceName );
	bool online = OnlineServices.getOnlineStatus();
	reportOnlineStatus( online );
}

void	cbToLoginService(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	msgin.invert();
	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgin);
}

void	cbToManagerService(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	msgin.invert();
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgin);
}

void	cbToLoginService_WithSID(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	CMessage	msgOut;
	uint16		lobby_sid = tsid.get();
	msgin.copyWithPrefix(lobby_sid, msgOut);
	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgOut);
}

void	cb_M_W_REQSHARDCODE(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	if(ShardMainCode[0] == 0)
	{
		sipwarning("ShardMainCode[0] = 0");
		return;
	}

	CMessage	msgOut(M_W_SHARDCODE);
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		msgOut.serial(ShardMainCode[i]);
		if (ShardMainCode[i] == 0)
			break;
	}
	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}


// Callback Array for message from FES
TUnifiedCallbackItem FESCallbackArray[] =
{
	{ M_ENTER_SHARD_ANSWER,		cbFESShardChooseShard },
	{ M_CLIENT_CONNECT_STATUS,		cb_M_CLIENT_CONNECT_STATUS },
	{ M_LS_EXPENDMONEY,			cbFESExpendMoney },
	//{ M_LS_CANUSEMONEY,			cbOSCanUseMoney },
	{ M_LOGIN_SERVER_RPC,		cbFESRemovedPendingCookie },
	{ M_WS_FEPA,				cbFESPatchAddress },
	{ M_WS_NBPLAYERS,			cbFESNbPlayers },
	{ M_WS_NBPLAYERS2,			cbFESNbPlayers2 },

	//{ M_WS_RESTORE_SHARD_OPEN,	cbRestoreShardOpen },

	{ M_NT_SHARD_LOBBYS,		cbToLoginService },
	{ M_LS_REFRESHMONEY,		cbToLoginService },
	{ M_LS_CHECKPWD2,			cbToLoginService },

	//{ M_SM_ROOMCARD_CHECK,		cbToLoginService },
	{ M_SM_ROOMCARD_USE,		cbToLoginService },

	//{ M_SM_BANKITEM_GET_NUSER,	cbToLoginService_WithSID },
	//{ M_CS_BANKITEM_LIST,		cbToLoginService_WithSID },
	//{ M_CS_BANKITEM_GET,		cbToLoginService_WithSID },
	//{ M_SM_BANKITEM_GET_FAIL,	cbToLoginService },

	{ M_W_REQSHARDCODE,			cb_M_W_REQSHARDCODE},

	{ M_SM_CHECK_USER_ACTIVITY,	cbToLoginService },
	{ M_SM_SET_USER_ACTIVITY,	cbToLoginService_WithSID },
	{ M_MS_SET_USER_ACTIVITY,	cb_ToSpecialService },
	{ M_SM_CHECK_BEGINNERMSTITEM, cbToLoginService },
	{ M_SM_SET_USER_BEGINNERMSTITEM,	cbToLoginService_WithSID },
	{ M_MS_SET_USER_BEGINNERMSTITEM,	cb_ToSpecialService },

	{ M_NT_NEWFAMILY,			cbToLoginService },

	{ M_CS_CHANGE_SHARD_REQ,	cb_M_CS_CHANGE_SHARD_REQ },

	{ M_CS_ADDCONTGROUP,		cbToLoginService },
	{ M_CS_DELCONTGROUP,		cbToLoginService },
	{ M_CS_RENCONTGROUP,		cbToLoginService },
	{ M_CS_SETCONTGROUP_INDEX,	cbToLoginService },
	{ M_CS_ADDCONT,				cbToLoginService },
	{ M_CS_DELCONT,				cbToLoginService },
	{ M_CS_CHANGECONTGRP,		cbToLoginService },
	{ M_NT_REQ_FRIEND_INFO,		cbToLoginService },
	{ M_NT_REQ_FAMILY_INFO,		cbToLoginService_WithSID },

	{ M_CS_ROOMINFO,			cb_M_CS_ROOMINFO },	
	{ M_CS_DECEASED,			cb_M_CS_DECEASED },	

	{ M_CS_INSFAVORROOM,		cbToLoginService },
	{ M_CS_DELFAVORROOM,		cbToLoginService },
	{ M_CS_MOVFAVORROOM,		cbToLoginService },
	{ M_CS_NEW_FAVORROOMGROUP,	cbToLoginService },
	{ M_CS_DEL_FAVORROOMGROUP,	cbToLoginService },
	{ M_CS_REN_FAVORROOMGROUP,	cbToLoginService },

	{ M_CS_REQ_FAVORROOM_INFO,	cb_M_CS_REQ_FAVORROOM_INFO },
	{ M_NT_REQ_FAVORROOM_INFO,	cb_M_NT_REQ_FAVORROOM_INFO },
	{ M_SC_FAVORROOM_INFO,		cb_ToSpecialService },
	{ M_SC_FAVORROOM_AUTODEL,	cb_ToSpecialService },

	{ M_NT_AUTOPLAY_REGISTER,	cb_M_NT_AUTOPLAY_REGISTER },

	// Mail
	{ M_NT_REQ_MAILBOX_STATUS_CHECK,	cbToLoginService },
	{ M_CS_MAIL_GET_LIST,		cbToLoginService_WithSID },
	{ M_CS_MAIL_GET,			cbToLoginService_WithSID },
	{ M_CS_MAIL_SEND,			cbToLoginService_WithSID },
	{ M_NT_MAIL_SEND,			cbToLoginService_WithSID },
	{ M_CS_MAIL_DELETE,			cbToLoginService },
	{ M_CS_MAIL_REJECT,			cbToLoginService_WithSID },
	{ M_CS_MAIL_ACCEPT_ITEM,	cbToLoginService_WithSID },
	{ M_NT_MAIL_ACCEPT_ITEM,	cbToLoginService },
	{ M_NT_TEMP_MAIL_ADDPOINT_TO_OLDROOM,	cbToLoginService },

	// Chit
	{ M_CS_RESCHIT,				cbToLoginService },
	{ M_NT_ADDCHIT,				cbToLoginService },

	// ¿¹ŸàÀÇœÄ
	{ M_NT_AUTOPLAY3_REGISTER,	cbToLoginService },
	{ M_NT_AUTOPLAY3_START_OK,	cbToLoginService },
	{ M_NT_AUTOPLAY3_EXP_ADD_OK,cbToLoginService },
	{ M_NT_AUTOPLAY3_FAIL,		cbToLoginService },
};

/**************************************************************************************************
//
//					Callback From LS
//
**************************************************************************************************/

static void	DBCB_DBCheckFastEnterroom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32	ret			= *(sint32 *)argv[0];
	uint16	ss_sid		= *(uint16 *)argv[1];
	uint32	UserAddr	= *(uint32 *)argv[2];
	uint32	UserID		= *(uint32 *)argv[3];
	uint32	RoomID		= *(uint32 *)argv[4];

	if ( !nQueryResult || ret == -1 )
	{
		sipwarning("DB_EXE_PREPARESTMT failed");
		return;
	}

	//sipdebug("M_CS_CHECK_FASTENTERROOM received. ss_sid=%d, UserAddr=%d, UserID=%d, RoomID=%d", ss_sid, UserAddr, UserID, RoomID); byy krs
	uint32	err;
	{
		switch(ret)
		{
		case 0:
			err = ERR_NOERR;
			break;
		case 1006:
		case 1003:
			err = ERR_INVALIDROOMID;
			break;
		case 1032:
			err = E_ENTER_EXPIRE;
			break;
		case 1035:
			err = E_ENTER_NO_NEIGHBOR;
			break;
		case 1036: // Password°¡ °Éž° ¹æÀº ÀÌ ¹æœÄÀž·Î µéŸî°¥Œö ŸøŽÙ.
			err = E_ENTER_NO_PASSWORD;
			break;
		case 1043:
			err = E_ENTER_NO_ONLYMASTER;
			break;
		default:
			sipwarning("Unknown Result...");
			return;
		}
	}

	uint32	ShardID = ShardIdentifier.get();

	CMessage	msgout(M_SC_CHECK_FASTENTERROOM);
	msgout.serial(ss_sid, UserAddr, err, RoomID, ShardID);
	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);

	//sipdebug("M_SC_CHECK_FASTENTERROOM send. ss_sid=%d, UserAddr=%d, err=%d, RoomID=%d, ShardID=%d", ss_sid, UserAddr, err, RoomID, ShardID); byy krs
}

// Request to Check Enter Room ([d:RoomID])
void cb_M_CS_CHECK_FASTENTERROOM(CMessage &msgin, const std::string &serviceName, TServiceId sid)
{
	uint16	ss_sid;
	uint32	UserAddr;
	uint32	UserID;
	uint32	RoomID;
	msgin.serial(ss_sid, UserAddr, UserID, RoomID);

	MAKEQUERY_CheckFastEnterroom(strQ, UserID, RoomID);
	DBCaller->executeWithParam(strQ, DBCB_DBCheckFastEnterroom,
		"DWDDD",
		OUT_,		NULL,
		CB_,		ss_sid,
		CB_,		UserAddr,
		CB_,		UserID,
		CB_,		RoomID);
}

// Packet : M_ENTER_SHARD_REQUEST
// LS -> WS
void cbLSChooseShard (CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	CLoginCookie cookie;	msgin.serial (cookie);
	uint8	userPriv;		msgin.serial (userPriv);

	//sipdebug( "CS recvd from %s-%hu, cookie=%s", serviceName.c_str(), sid.get(), cookie.toString().c_str()); byy krs

	T_ERR	ret		= ERR_NOERR;
	uint32	userId	= cookie.getUserId();
	uint8	bIsGM	= cookie.getIsGM();
	TUSERIDASSOCIATION::iterator it = UserIdSockAssociations.find(userId);
	if ( it != UserIdSockAssociations.end() )
	{
		uint32 nGMCount = static_cast<uint32>(GMUserIdSockAssociations.count(userId));
		if ( nGMCount > 0 )
		{
			if ( !bIsGM && ( userPriv == USERTYPE_GAMEMANAGER || userPriv == USERTYPE_GAMEMANAGER_H ||
				userPriv == USERTYPE_EXPROOMMANAGER || userPriv == USERTYPE_WEBMANAGER ) ) // Login of General Client with same as GM AccountID and Password
			{
				ret = ERR_ALREADYONLINE;
				CMessage msgout (M_ENTER_SHARD_ANSWER);
				msgout.serial (ret);
				msgout.serial (cookie);
				CUnifiedNetwork::getInstance()->send(sid, msgout);
				sipwarning("User %u can't enter shard, CS can't send to FS, send err to LS", cookie.getUserId());
				return;
			}
			else // Abnormal Logoff
			{
				// disconnect old GM
				sipwarning("Abnormal GM Logoff userid: %u", userId);
				disconnectClient(userId);
			}
		}
	}

	ret = lsChooseShard(cookie, userPriv, WS::TUserRole::ur_player, 0xffffffff, ~0);
	if (ret != ERR_NOERR)
	{
		// send back an error message to LS
		CMessage msgout (M_ENTER_SHARD_ANSWER);
		msgout.serial (ret);
		msgout.serial (cookie);
		CUnifiedNetwork::getInstance()->send(sid, msgout);
		sipwarning("User %u can't enter shard, CS can't send to FS, send err to LS", cookie.getUserId());
	}
}

//void cbLSChooseShard (CMessage &msgin, const std::string &serviceName, uint16 sid)
T_ERR lsChooseShard (const SIPNET::CLoginCookie &cookie,
					 uint8 &userPriv,
					 WS::TUserRole userRole,
					 uint32 instanceId,
					 uint32 charSlot)
{
	uint	totalNbUsers;
	CFES*	best = findBestFES( totalNbUsers );

	// could not find a good FES or best FES has more players than balance limit
	if (best == NULL || best->getUsersCountHeuristic() >= OpenFrontEndThreshold)
	{
		// open a new frontend
		openNewFES();

		// reselect best FES (will return newly open FES, or previous if no more FES available)
		best = findBestFES(totalNbUsers);

		// check there is a FES available
		if (best == NULL)
		{
			// answer the LS that we can't accept the user
			return ERR_NOFRONTEND;
		}
	}

	CMessage msgout (M_ENTER_SHARD_REQUEST);
	msgout.serial (const_cast<CLoginCookie&>(cookie));
	msgout.serial (userPriv);
	msgout.serial (instanceId);
	msgout.serial (charSlot);

	CUnifiedNetwork::getInstance()->send (best->SId, msgout);
	best->NbEstimatedUser++;
	best->NbPendingUsers++;

	// Update counts
	uint32 totalNbOnlineUsers = 0, totalNbPendingUsers = 0;
	for (list<CFES>::iterator it=FESList.begin(); it!=FESList.end(); ++it)
	{
		totalNbOnlineUsers += (*it).NbUser;
		totalNbPendingUsers += (*it).NbPendingUsers;
	}

	if (CWelcomeServiceMod::isInitialized())
	{
		CWelcomeServiceMod::getInstance()->updateConnectedPlayerCount(totalNbOnlineUsers, totalNbPendingUsers);
	}

	return ERR_NOERR;
}

void cbFailed (CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	// I can't connect to the Login Service, just siperror ();
	string reason;
	msgin.serial (reason);
	siperror (reason.c_str());
}


bool disconnectClient(uint32 userId)
{
	TUSERIDASSOCIATION::iterator it = UserIdSockAssociations.find (userId);
	if (it == UserIdSockAssociations.end ())
	{
		uint32 nGMCount = static_cast<uint32>(GMUserIdSockAssociations.count(userId));
		if ( nGMCount == 0 )
		{
			sipwarning ("Can't disconnect the UID=%d, he is not found [WRN_1]", userId);
			return false;
		}

		pair<TGMASSOCIATIONIT, TGMASSOCIATIONIT> range = GMUserIdSockAssociations.equal_range(userId);
		for ( TGMASSOCIATIONIT it = range.first; it != range.second; it ++ )
		{
			CMessage msgout (M_CLIENT_DISCONNECT);
			msgout.serial (userId);
			CUnifiedNetwork::getInstance()->send(it->second, msgout);
			//sipinfo("Send FS <DC> Disconnect Same GM UID=%u", userId); byy krs
		}
	}
	else
	{
		CMessage	msgOut(M_SC_DISCONNECT_OTHERLOGIN);
		msgOut.serial(userId);
		CUnifiedNetwork::getInstance()->send(it->second, msgOut);

		CMessage msgout (M_CLIENT_DISCONNECT);
		msgout.serial (userId);
		CUnifiedNetwork::getInstance()->send(it->second, msgout);
		//sipinfo("Send FS <DC> Disconnect Same Account UID=%u", userId); byy krs
	}

	return true;
}

// Packet : DC
// LS -> WS
void cbLSDisconnectClient (CMessage &msgin, const std::string &serviceName, TServiceId  sid)
{
	// the LS tells me that i have to disconnect a client

	uint32 userid;
	msgin.serial (userid);

	disconnectClient(userid);

}

// connection to the LS, send the identification message
void cbLSConnection (const std::string &serviceName, TServiceId  sid, void *arg)
{
	sint32 checkId;

	if (IService::getInstance()->haveArg('S'))
	{
		// use the command line param if set
		checkId = atoi(IService::getInstance()->getArg('S').c_str());
	}
	else if (IService::getInstance()->ConfigFile.exists ("ShardIdentifier"))
	{
		// use the config file param if set
		checkId = IService::getInstance()->ConfigFile.getVar ("ShardIdentifier").asInt();
	}
	else
	{
		checkId = -1;
	}

	if (checkId == -1)
	{
		siperror ("ShardIdentifier variable must be valid (>0)");
	}

	uint32		shardId		= ShardIdentifier;
	ucstring	shardName	= UTF8toUC(ShardName.get());
	uint8		zoneId		= ZoneID.get();
	uint32		nUserLimit	= (uint32) PlayerLimit.get();
	ucstring	ucContent	= UTF8toUC(ShardComment);
	uint8		activeFlag	= SHARD_ON;
	uint32		nbPlayers	= NbPlayers;
	uint16		SId			= IService::getInstance()->getServiceId().get();

	string WSAddr = IService::getInstance()->ConfigFile.getVar("WSHost").asString();
	uint16	uWSPort = 49995;
	if (WSAddr.find(":") == string::npos)
	{
		CConfigFile::CVar *var;
		if ((var = IService::getInstance()->ConfigFile.getVarPtr("WSPort")) != NULL)
			uWSPort = var->asInt ();
		WSAddr += ":" + toStringA(uWSPort);
	}

	CMessage	msgout (M_WL_WS_IDENT);
	msgout.serial(shardId, shardName, zoneId, nUserLimit, ucContent, activeFlag, nbPlayers, SId, WSAddr);
	CUnifiedNetwork::getInstance()->send (sid, msgout);
	//sipinfo ("Connected to %s-%hu and sent identification with shardId '%d'", serviceName.c_str(), sid.get(), shardId); byy krs
}

void cbLSDisconnection (const std::string &serviceName, TServiceId  sid, void *arg)
{
	g_mapServiceNameToShardCode.clear();

	for (std::map<uint8, string>::iterator it = g_mapShardCodeToServiceName.begin(); it != g_mapShardCodeToServiceName.end(); )
	{
		if (it->second != "0")
			it = g_mapShardCodeToServiceName.erase(it);
		else
			it ++;
	}
}

// Callback for detection of config file change about "ExpectedServices"
void cbUpdateExpectedServices( CConfigFile::CVar& var )
{
	OnlineServices.setExpectedInstances( var );
}


/*
 * ShardOpen update functions/callbacks etc.
 */

/**
 * updateShardOpenFromFile()
 * Update ShardOpen from a file.
 * Read a line of text in the file, converts it to int (atoi), then casts into bool for ShardOpen.
 */
void	updateShardOpenFromFile(const std::string& filename)
{
	CIFile	f;

	if (!f.open(filename))
	{
		sipwarning("Failed to update ShardOpen from file '%s', couldn't open file", filename.c_str());
		return;
	}

	try
	{
		char	readBuffer[256];
		f.getline(readBuffer, 256);
		ENUM_SHARDSTATEDEFINE shardopen = (ENUM_SHARDSTATEDEFINE)atoi(readBuffer);
		f.getline(readBuffer, 256);
		ENUM_USERTYPE usertype = (ENUM_USERTYPE)atoi(readBuffer);

		NotifyShardOpenState(shardopen, usertype);

		//sipinfo("Updated ShardOpen state to '%u' from file '%s'", ShardOpen.get(), filename.c_str()); byy krs
	}
	catch (Exception& e)
	{
		sipwarning("Failed to update ShardOpen from file '%s', exception raised while getline() '%s'", filename.c_str(), e.what());
	}
}

std::string	ShardOpenStateFileName;

/**
 * cbShardOpen()
 * Callback for ShardOpen
 */
void	cbShardOpen(IVariable &var)
{
	//NotifyShardOpenState((ENUM_SHARDSTATEDEFINE)(ShardOpen.get()), (ENUM_USERTYPE)(EnterUserType.get()), false);
}

void	cbEnterUserType(IVariable &var)
{
	//NotifyShardOpenState((ENUM_SHARDSTATEDEFINE)(ShardOpen.get()), (ENUM_USERTYPE)(EnterUserType.get()), false);
}

/**
 * cbShardName()
 * Callback for ShardName
 */

void	cbShardName(IVariable &var)
{
}

/**
 * cbShardComment()
 * Callback for ShardComment
 */
void	cbShardComment(IVariable &var)
{
}

/**
 * cbShardId()
 * Callback for ShardIdentifier
 */
void	cbShardId(IVariable &var)
{
}

/**
 * cbShardOpenStateFile()
 * Callback for ShardOpenStateFile
 */
void	cbShardOpenStateFile(IVariable &var)
{
	// remove previous file change callback
	if (!ShardOpenStateFileName.empty())
	{
		CFileA::removeFileChangeCallback(ShardOpenStateFileName);
		//sipinfo("Removed callback for ShardOpenStateFileName file '%s'", ShardOpenStateFileName.c_str()); byy krs
	}

	ShardOpenStateFileName = var.toString();

	if (!ShardOpenStateFileName.empty())
	{
		// set new callback for the file
		CFileA::addFileChangeCallback(ShardOpenStateFileName, updateShardOpenFromFile);
		//sipinfo("Set callback for ShardOpenStateFileName file '%s'", ShardOpenStateFileName.c_str()); byy krs

		// and update state from file...
		updateShardOpenFromFile(ShardOpenStateFileName);
	}
}

/**
 * cbUsePatchMode()
 * Callback for UsePatchMode
 */
void	cbUsePatchMode(IVariable &var)
{
	// if patch mode not set, set all fs in patching mode to accept clients now
	if (!UsePatchMode.get())
	{
		//sipinfo("UsePatchMode disabled, switch all patching servers to actual frontends"); byy krs

		list<CFES>::iterator	it;
		
		for (it=FESList.begin(); it!=FESList.end(); ++it)
		{
			if ((*it).State == PatchOnly)
			{
				(*it).setToAcceptClients();
			}
		}
	}
}

static void	DBCB_DBFamilyLogoutForForceLogoff(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ACCOUNTID			userid	= *(T_ACCOUNTID *)argv[0];
	uint8				bIsGM	= *(uint8 *)argv[1];
	TServiceId			tsid(*(uint16 *)argv[2]);

	if ( !nQueryResult )
		return;

	T_F_VISITDAYS	VisitDays = 0;
	T_F_EXP			lUserExp = 0;
	T_F_VIRTUE		UserVirtue = 0;
	T_F_FAME		UserFame = 0;
	uint32			nGMoney = 0;
	uint32			nRooms = 0;	
	uint8			ExpRoom = 0;
	bool			bRightValue = false;
	uint32 nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		resSet->serial(VisitDays);
		resSet->serial(lUserExp);
		resSet->serial(UserFame);
		resSet->serial(nGMoney);
		resSet->serial(nRooms);
		bRightValue = true;
	}
	uint16 UserExp = (uint16)lUserExp;

	CMessage	msgout(M_LS_RELOGOFF);
	msgout.serial(userid);
	msgout.serial(bIsGM);
	msgout.serial(VisitDays);
	msgout.serial(UserExp);
	msgout.serial(UserVirtue);
	msgout.serial(UserFame);
	msgout.serial(nRooms);
	msgout.serial(ExpRoom);
	msgout.serial(nGMoney);
	msgout.serial(bRightValue);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
	//sipinfo("<Abnormal LogOff> :Send LS <RELOGOFF> FamilyID %u, UserExp %u, UserVirtue %u, UserFame %u, RoomNum %u", userid, UserExp, UserVirtue, UserFame, nRooms); byy krs
}

// Request Force Logoff by double login, SS->LS->WS
void cb_M_NT_REQ_FORCE_LOGOFF(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32		userid;		msgin.serial(userid);
	uint8		bIsGM;		msgin.serial(bIsGM);
	uint8		uValidWorkData = 1;

	bool		bOnline;
	TUSERIDASSOCIATION::iterator it = UserIdSockAssociations.find(userid);
	if ( it == UserIdSockAssociations.end() )
		bOnline = false;
	else
		bOnline = true;

	// Same Account Login : send FS
	if ( bOnline )
	{
		disconnectClient(userid);
		//sipinfo("<Same Account Login> :Send FS <DC> Disconnect Same Account UID=%u", userid); byy krs
		return;
	}

	MAKEQUERY_FAMILYLOGOUT(strQuery, userid)
	DBCaller->execute(strQuery, DBCB_DBFamilyLogoutForForceLogoff,
		"DBW",
		CB_,		userid,
		CB_,		bIsGM,
		CB_,		tsid.get());
}

//// packet : CANUSEMONEYR
//// LS -> WS
//void cbLSCanUseMoney(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
//{
//	CMessage	msgout(M_WS_CANUSERMONEY);
//	uint32	msgid;		msgin.serial(msgid);			msgout.serial(msgid);
//	uint32	uResult;	msgin.serial(uResult);			msgout.serial(uResult);
//	uint32	userid;		msgin.serial(userid);			msgout.serial(userid);
//	uint8	flag;		msgin.serial(flag);				msgout.serial(flag);
//	uint32	moneyExpend;	msgin.serial(moneyExpend);	msgout.serial(moneyExpend);
//	uint32	money;			msgin.serial(money);		msgout.serial(money);
//
//	UserMoneyInfo	info;
//	ITARYUSERMONEY	it = aryUserMoney.find(msgid);
//	if ( it == aryUserMoney.end() )
//	{
//		sipwarning("There is no temporary user info");
//		return;
//	}
//	info = aryUserMoney[msgid];
//	TServiceId	serviceId = info._serviceid;
//	CUnifiedNetwork::getInstance()->send(serviceId, msgout);
//	sipinfo("Can use money : MesID=%d, UserID=%d, Money=%d, Result=%d", msgid, userid, money, uResult);
//	aryUserMoney.erase(it);
//}

// packet : USERMONEY, M_LS_CHECKPWD2_R
// LS -> WS
void cbToRoomService(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16	sid;
	CMessage msgout;
	msgin.copyWithoutPrefix(sid, msgout);

	TServiceId	dst_tsid(sid);
	CUnifiedNetwork::getInstance()->send(dst_tsid, msgout);
}

// packet : SC_CHATMAIN
// LS -> WS
void	cbSystemNotice(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	ucstring	ucNotice;
	msgin.serial(ucNotice);
	
	SendSystemNotice(SN_SYSTEM, ucNotice);
}

// Callback Array for message from LS
TUnifiedCallbackItem LSCallbackArray[] =
{
	{ M_CS_CHECK_FASTENTERROOM,	cb_M_CS_CHECK_FASTENTERROOM },

	{ M_ENTER_SHARD_REQUEST,	cbLSChooseShard },
	{ M_CLIENT_DISCONNECT,	cbLSDisconnectClient },
	{ M_SC_FAIL,			cbFailed },

	//{ M_LS_CANUSEMONEYR,    cbLSCanUseMoney	},
	{ M_LS_USERMONEY,		cbToRoomService },
	{ M_LS_CHECKPWD2_R,		cbToRoomService	},
	{ M_MS_ROOMCARD_USE,	cbToRoomService	},
	//{ M_MS_ROOMCARD_CHECK,	cbToRoomService	},

	{ M_LW_SHARDINFO,		cbShardInfo},
	{ M_LW_OTHER_SHARD,		cbOtherShardInfo},
	{ M_LW_OTHER_SHARD_OFF,		cbOtherShardOff},
	{ M_NT_REQ_FORCE_LOGOFF,	cb_M_NT_REQ_FORCE_LOGOFF },

	{ M_SC_CHATMAIN,		cbSystemNotice },

	//{ M_SC_BANKITEM_LIST,	cbToRoomService },
	//{ M_MS_BANKITEM_GET,	cbToRoomService },

	{ M_MS_CURRENT_ACTIVITY,	cb_M_MS_CURRENT_ACTIVITY },
	{ M_MS_CHECK_USER_ACTIVITY,	cb_M_MS_CHECK_USER_ACTIVITY }, // cb_M_MS_CHECK_USER_ACTIVITY = cbToManagerService
	{ M_MS_BEGINNERMSTITEM,		cb_M_MS_BEGINNERMSTITEM },
	{ M_MS_CHECK_BEGINNERMSTITEM, cbToManagerService },
	{ M_MS_CURRENT_CHECKIN,		cb_M_MS_CURRENT_CHECKIN },

	{ M_ENTER__SHARD,			cb_M_ENTER__SHARD },

	// Áö¿ªœá¹öÀýÈ¯¿ë
	{ M_SC_CHANGE_SHARD_ANS,		cbToRoomService },
	{ M_NT_MOVE_USER_DATA_REQ,		cb_M_NT_MOVE_USER_DATA_REQ },
	{ M_NT_SEND_USER_DATA,			cb_M_NT_SEND_USER_DATA },

	// Ä³ž¯ÅÍ ÀÌž§¹Ù²Ù±â - Áö¿ªœá¹ö»çÀÌ
	{ M_NT_CHANGE_NAME,			cb_M_NT_CHANGE_NAME },
	{ M_NT_CHANGE_USERNAME,		cb_M_NT_CHANGE_USERNAME },

	// Ä£±žžñ·Ï ºž³»±â
	{ M_NT_FRIEND_GROUP,		cbToManagerService },
	{ M_NT_FRIEND_LIST,			cbToManagerService },
	{ M_NT_FRIEND_SN,			cbToManagerService },
	{ M_NT_FRIEND_DELETED,		cbToManagerService },
	{ M_NT_ANS_FAMILY_INFO,		cb_ToSpecialService },

	// 
	{ M_NT_REQ_ROOMINFO,		cb_M_NT_REQ_ROOMINFO },
	{ M_SC_ROOMINFO,			cb_M_SC_ROOMINFO },
	{ M_NT_REQ_DECEASED,		cb_M_NT_REQ_DECEASED },
	{ M_SC_DECEASED,			cb_ToSpecialService },

	// Mail
	{ M_SC_MAIL_LIST,			cb_ToSpecialService },
	{ M_SC_MAIL,				cb_ToSpecialService },
	{ M_NT_MAILBOX_STATUS_FOR_SEND,	cb_ToSpecialService },
	{ M_SC_MAIL_SEND,			cb_ToSpecialService },
	{ M_SC_MAIL_REJECT,			cb_ToSpecialService },
	{ M_NT_MAIL_ITEM_FOR_ACCEPT,cb_ToSpecialService },
	{ M_SC_MAILBOX_STATUS,		cb_ToUID },

	// Chit
	{ M_SC_CHITLIST,			cb_ToUID },

	// ¿¹ŸàÀÇœÄ
	{ M_NT_AUTOPLAY3_START_REQ,	cbToManagerService },
	{ M_NT_AUTOPLAY3_EXP_ADD_ONLINE,	cbToManagerService },
	{ M_NT_AUTOPLAY3_EXP_ADD_OFFLINE,	cbToManagerService },
};

/*******************************************************************************
//
//			Wellcome Service Initialization
//
*******************************************************************************/

void	InitShardDB()
{
	string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Shard_DBName").asString ();
	string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Shard_DBHost").asString ();
	string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Shard_DBLogin").asString ();
	string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Shard_DBPassword").asString ();
/*
	BOOL bSuccess = DBConnection.Connect(	ucstring(strDatabaseLogin).c_str(),
									ucstring(strDatabasePassword).c_str(),
									ucstring(strDatabaseHost).c_str(),
									ucstring(strDatabaseName).c_str());
	if (!bSuccess)
	{
		siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
			strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
		return;
	}
	sipinfo("Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
*/
	//////////////////////////////////////////////////////////////////////////
	DBCaller = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
		ucstring(strDatabasePassword).c_str(),
		ucstring(strDatabaseHost).c_str(),
		ucstring(strDatabaseName).c_str());

	BOOL bSuccess = DBCaller->init(2);
	if (!bSuccess)
	{
		siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
			strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
		return;
	}
	//////////////////////////////////////////////////////////////////////////
}

/* KSR_DEL
void	LoadShardState(CConfigFile & config)
{
	CShardOpenState		info;
	if ( config.exists("EmptyState"))
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("EmptyState");

		info.startPer	= varPtr->asInt(0);
		info.endPer		= varPtr->asInt(1);
		info.nUserType	= varPtr->asInt(2);
		info.shardState	= 1;
		aryShardOpenState.insert(make_pair(info.shardState, info));
	}

	if ( config.exists("GoodState"))
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("GoodState");

		info.startPer	= varPtr->asInt(0);
		info.endPer		= varPtr->asInt(1);
		info.nUserType	= varPtr->asInt(2);
		info.shardState	= 2;
		aryShardOpenState.insert(make_pair(info.shardState, info));
	}

	if ( config.exists("FullState"))
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("FullState");

		info.startPer	= varPtr->asInt(0);
		info.endPer		= varPtr->asInt(1);
		info.nUserType	= varPtr->asInt(2);
		info.shardState	= 10;
		aryShardOpenState.insert(make_pair(info.shardState, info));
	}

	if ( config.exists("PrivateState"))
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("PrivateState");

		info.startPer	= varPtr->asInt(0);
		info.endPer		= varPtr->asInt(1);
		info.nUserType	= varPtr->asInt(2);
		info.shardState	= 100;
		aryShardOpenState.insert(make_pair(info.shardState, info));
	}

	if ( config.exists("CloseState"))
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("CloseState");

		info.startPer	= varPtr->asInt(0);
		info.endPer		= varPtr->asInt(1);
		info.nUserType	= varPtr->asInt(2);
		info.shardState	= 200;
		aryShardOpenState.insert(make_pair(info.shardState, info));
	}
}

void	LoadShardVIPs(CConfigFile & config)
{
	if ( config.exists("VIPs"))
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("VIPs");

		for ( uint i = 0; i < VIPsNumber.get(); i ++ )
		{
			uint32	VIPID = varPtr->asInt(i);
			aryVIPs.push_back(VIPID);
		}
	}
}
*/

// Load Shard information from config file
void	LoadShardInConfig(CConfigFile & config)
{
	if (IService::getInstance()->haveArg('S'))
	{
		// use the command line param if set
		uint shardId = atoi(IService::getInstance()->getArg('S').c_str());

		//sipinfo(L"Using shard id %u from command line '%s'", shardId, IService::getInstance()->getArg('S').c_str()); byy krs
		IService::getInstance()->anticipateShardId(shardId);
	}
	else if (config.exists ("ShardIdentifier"))
	{
		// use the config file param if set
		uint shardId = config.getVar ("ShardIdentifier").asInt();

		//sipinfo(L"Using shard id %u from config file", shardId); byy krs
		IService::getInstance()->anticipateShardId(shardId);
	}

	if (config.exists ("ShardName"))
	{
		// use the config file param if set
		ucstring	ucShardName = UTF8toUC(config.getVar ("ShardName").asString());
		sipinfo(L"Using shard Name '%s' from config file", ucShardName.c_str());
	}

	if (config.exists ("ShardComment"))
	{
		// use the config file param if set
		ucstring	ucShardComment = UTF8toUC(config.getVar ("ShardComment").asString());
		sipinfo(L"Using shard Comment '%s' from config file", ucShardComment.c_str());
	}

	//LoadShardState(config);
	//LoadShardVIPs(config);
}

class CWelcomeService : public IService
{

public:

	/// Init the service, load the universal time.
	void init ()
	{
		try { FrontEndAddress = ConfigFile.getVar ("FrontEndAddress").asString(); } catch(Exception &) { }
		//sipinfo ("Waiting frontend services named '%s'", FrontendServiceName.get().c_str()); byy krs

		LoadShardInConfig(ConfigFile);
        
		CUnifiedNetwork::getInstance()->setServiceUpCallback(FrontendServiceName.get(), cbFESConnection, NULL);
		CUnifiedNetwork::getInstance()->setServiceDownCallback(FrontendServiceName.get(), cbFESDisconnection, NULL);
		CUnifiedNetwork::getInstance()->setServiceUpCallback(MANAGER_SHORT_NAME, cbMSConnection, NULL);
		CUnifiedNetwork::getInstance()->setServiceUpCallback("*", cbServiceUp, NULL);
		CUnifiedNetwork::getInstance()->setServiceDownCallback("*", cbServiceDown, NULL);

		InitShardDB();

		// add a connection to the LS
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

		AllowDispatchMsgToLS = true;

		if (ConfigFile.getVarPtr("DontUseLSService") == NULL 
			|| !ConfigFile.getVar("DontUseLSService").asBool())
		{
			// We are using SIP Login Service
			CUnifiedNetwork::getInstance()->addCallbackArray(LSCallbackArray, sizeof(LSCallbackArray)/sizeof(LSCallbackArray[0]));
			if (!DontUseLS)
			{
				CUnifiedNetwork::getInstance()->setServiceUpCallback(LS_S_NAME, cbLSConnection, NULL);
				CUnifiedNetwork::getInstance()->setServiceUpCallback(LS_S_NAME, cbLSDisconnection, NULL);
				CUnifiedNetwork::getInstance()->addService(LS_S_NAME, LSAddr);
			}
		}
		// List of expected service instances
		ConfigFile.setCallback( "ExpectedServices", cbUpdateExpectedServices );
		if ( ConfigFile.exists( "ExpectedServices") )
		{
			cbUpdateExpectedServices( ConfigFile.getVar( "ExpectedServices" ) );
		}

		/*
		 * read config variable ShardOpenStateFile to update
		 * 
		 */
		cbShardOpenStateFile(ShardOpenStateFile);

//		// create a welcome service module (for SU comm)
//		IModuleManager::getInstance().createModule("WelcomeService", "ws", "");
//		// plug the module in the default gateway
//		SIPBASE::CCommandRegistry::getInstance().execute("ws.plug wg", InfoLog());

		// For WS-WS network
//		uint16 wsport = 49995;
//		CConfigFile::CVar *var;
//		if ((var = ConfigFile.getVarPtr("WSPort")) != NULL)
//		{
//			wsport = var->asInt();
//		}
//		WsCallbackServer = new CCallbackServer;
//		WsCallbackServer->init(wsport);
//		WsCallbackServer->addCallbackArray(WsCallbackArray, sizeof(WsCallbackArray)/sizeof(WsCallbackArray[0]));
//		WsCallbackServer->setConnectionCallback(cbConnect, NULL);
//		WsCallbackServer->setDisconnectionCallback(cbDisconnect, NULL);

		CUnifiedNetwork::getInstance()->setServiceUpCallback(WELCOME_SHORT_NAME, cbWSConnection, NULL);
		CUnifiedNetwork::getInstance()->setServiceDownCallback(WELCOME_SHORT_NAME, cbWSDisconnection, NULL);

		// œÃ°£ Ÿò±â
		{
			T_DATETIME		StartTime_DB = "";

			// Get current time from server
			{
				MAKEQUERY_GETCURRENTTIME(strQ);
				CDBConnectionRest	DB(DBCaller);
				DB_EXE_QUERY(DB, Stmt, strQ);
				if ( nQueryResult == true )
				{
					DB_QUERY_RESULT_FETCH(Stmt, row);
					if (IS_DB_ROW_VALID(row))
					{
						COLUMN_DATETIME(row, 0, curtime);
						StartTime_DB = curtime;

						sipinfo("DBTime = %s", StartTime_DB.c_str());
					}
				}
			}

			if (StartTime_DB.length() < 1)
			{
				siperrornoex("Can't get current time of database server");
				return;
			}
			ServerStartTime_DB.SetTime(StartTime_DB);
			ServerStartTime_Local = CTime::getSecondsSince1970();
		}

		// 30ÀÏÀüÀÇ ¹æ¹®·ÏÀÚ·á ÃÊ±âÈ­
		{
			MAKEQUERY_Delete30DayDatas(strQuery);
			CDBConnectionRest	DBConnection(DBCaller);
			DB_EXE_QUERY(DBConnection, Stmt, strQuery);
		}

		// 30ÀÏÀüÀÇ Religion¹æ¹®·ÏÀÚ·á ÃÊ±âÈ­
		{
			uint32 curTime = (uint32)GetDBTimeSecond();
			MAKEQUERY_Delete30DayReligionDatas(strQuery, curTime);
			CDBConnectionRest	DBConnection(DBCaller);
			DB_EXE_QUERY(DBConnection, Stmt, strQuery);
		}
	}

	bool			update () 
	{ 
		removeStatusTag("DEV_ONLY");
		removeStatusTag("RESTRICTED");
		removeStatusTag("Open");

		if (ShardOpen == 0)
			addStatusTag("DEV_ONLY");
		else if (ShardOpen == 1)
			addStatusTag("RESTRICTED");
		else if (ShardOpen == 2)
			addStatusTag("Open");

//		WsCallbackServer->update();
		DBCaller->update();

		return true; 
	}

	void	release()
	{
//		if (WsCallbackServer != NULL)
//			delete WsCallbackServer;
//		WsCallbackServer = NULL;

		DBCaller->release();
		delete DBCaller;
		DBCaller = NULL;
	}
};


static const char* getCompleteServiceName(const IService* theService)
{
	static std::string s;
	s= WELCOME_LONG_NAME;

	if (theService->haveLongArg("wsname"))
	{
		s+= "_"+theService->getLongArg("wsname");
	}

	if (theService->haveLongArg("fullwsname"))
	{
		s= theService->getLongArg("fullwsname");
	}

	return s.c_str();
}

static const char* getShortServiceName(const IService* theService)
{
	static std::string s;
	s= WELCOME_SHORT_NAME;

	if (theService->haveLongArg("shortwsname"))
	{
		s= theService->getLongArg("shortwsname");
	}
	
	return s.c_str();
}

// Service instantiation
//SIPNET_SERVICE_MAIN( CWelcomeService, WELCOME_SHORT_NAME, WELCOME_LONG_NAME, "", 0, false, FESCallbackArray, SIPNS_CONFIG, SIPNS_LOGS);
SIPNET_SERVICE_MAIN( CWelcomeService, WELCOME_SHORT_NAME, WELCOME_LONG_NAME, "WSPort", 49995, false, FESCallbackArray, SIPNS_CONFIG, SIPNS_LOGS);


// welcome service module
//class CWelcomeServiceMod : 
//	public CEmptyModuleCommBehav<CEmptyModuleServiceBehav<CEmptySocketBehav<CModuleBase> > >,
//	public WS::CWelcomeServiceSkel
//{
//	void onProcessModuleMessage(IModuleProxy *sender, const CMessage &message)
//	{
//		if (CWelcomeServiceSkel::onDispatchMessage(sender, message))
//			return;
//
//		sipwarning("Unknown message '%s' received by '%s'", 
//			message.getName().c_str(),
//			getModuleName().c_str());
//	}
//
//
//	////// CWelcomeServiceSkel implementation 
//
//	// ask the welcome service to welcome a user
//	virtual void welcomeUser(SIPNET::IModuleProxy *sender, uint32 userId, const std::string &userName, const CLoginCookie &cookie, const std::string &priviledge, const std::string &exPriviledge, WS::TUserRole mode, uint32 instanceId)
//	{
//		string ret = lsChooseShard(userName,
//			cookie,
//			priviledge, 
//			exPriviledge, 
//			mode,
//			instanceId);
//
//		if (!ret.empty())
//		{
//			// TODO : correct this
//			string fsAddr;
//			CWelcomeServiceClientProxy wsc(sender);
//			wsc.welcomeUserResult(this, userId, ret.empty(), fsAddr);
//		}
//	}
//
//	// ask the welcome service to disconnect a user
//	virtual void disconnectUser(SIPNET::IModuleProxy *sender, uint32 userId)
//	{
//		sipstop;
//	}
//	
//};

namespace WS
{

	void CWelcomeServiceMod::onModuleUp(IModuleProxy *proxy)
	{
		if (proxy->getModuleClassName() == "RingSessionManager")
		{
			if (_RingSessionManager != NULL)
			{
				sipwarning("WelcomeServiceMod::onModuleUp : receiving module up for RingSessionManager '%s', but already have it as '%s', replacing it",
					proxy->getModuleName().c_str(),
					_RingSessionManager->getModuleName().c_str());
			}
			// store this module as the ring session manager
			_RingSessionManager = proxy;

			// say hello to our new friend (transmit fixed session id if set in config file)
			//sipinfo("Registering welcome service module into session manager '%s'", proxy->getModuleName().c_str()); byy krs
			uint32 sessionId = 0;
			CConfigFile::CVar *varFixedSessionId = IService::getInstance()->ConfigFile.getVarPtr( "FixedSessionId" );
			if ( varFixedSessionId )
				sessionId = varFixedSessionId->asInt();
			CWelcomeServiceClientProxy wscp(proxy);
			wscp.registerWS(this, IService::getInstance()->getShardId(), sessionId, OnlineServices.getOnlineStatus());

			// Send counts
			uint32 totalNbOnlineUsers = 0, totalNbPendingUsers = 0;
			for (list<CFES>::iterator it=FESList.begin(); it!=FESList.end(); ++it)
			{
				totalNbOnlineUsers += (*it).NbUser;
				totalNbPendingUsers += (*it).NbPendingUsers;
			}
			CWelcomeServiceMod::getInstance()->updateConnectedPlayerCount(totalNbOnlineUsers, totalNbPendingUsers);
		}
		else if (proxy->getModuleClassName() == "LoginService")
		{
			_LoginService = proxy;
		}
	}

	void CWelcomeServiceMod::onModuleDown(IModuleProxy *proxy)
	{
		if (_RingSessionManager == proxy)
		{
			// remove this module as the ring session manager
			_RingSessionManager = NULL;
		}
		else if (_LoginService == proxy)
			_LoginService = NULL;
	}
	

	void CWelcomeServiceMod::welcomeUser(SIPNET::IModuleProxy *sender, uint32 charId, const SIPNET::CLoginCookie &cookie, uint8 &priviledge, ucstring &exPriviledge, WS::TUserRole mode, uint32 instanceId)
	{
		//sipdebug( "ERLOG: welcomeUser(%u,%s,%s,%u,%s,%u,%u)", charId, cookie.toString().c_str(), priviledge, exPriviledge.c_str(), (uint)mode.getValue(), instanceId ); byy krs
		T_ERR err = lsChooseShard(cookie,
									priviledge, 
									mode,
									instanceId,
									charId & 0xF);

		uint32 userId = charId >> 4;
		if ( err != ERR_NOERR )
		{
			sipdebug( "ERLOG: lsChooseShard returned an error => welcomeUserResult");
			// TODO : correct this
			string fsAddr;
			CWelcomeServiceClientProxy wsc(sender);
			wsc.welcomeUserResult(this, userId, false, fsAddr, err);
		}
		else
		{
			//sipdebug( "ERLOG: lsChooseShard OK => adding to pending"); byy krs
			TPendingFEResponseInfo pfri;
			pfri.WSMod = this;
			pfri.UserId = userId;
			pfri.WaiterModule = sender;
			PendingFeResponse.insert(make_pair(cookie, pfri));
		}
	}

	void CWelcomeServiceMod::pendingUserLost(const SIPNET::CLoginCookie &cookie)
	{
		if (!_LoginService)
			return;

		CLoginServiceProxy ls(_LoginService);

		ls.pendingUserLost(this, cookie);
	}


	// register the module
	SIPNET_REGISTER_MODULE_FACTORY(CWelcomeServiceMod, "WelcomeService");

} // namespace WS


SIPBASE_CATEGORISED_COMMAND(WS, systemNotice, "Send system message to all clients within shard", "")
{
	if (args.size() < 1 )
		return false;
	string strMessage = args[0];
	uint8		uType = SN_SYSTEM;
	ucstring	ucMessage = ucstring::makeFromUtf8(strMessage);
	SendSystemNotice(uType, ucMessage);
	log.displayNL(L"SYSTEM MESSAGE : %s", ucMessage.c_str());
	return true;
}

SIPBASE_CATEGORISED_COMMAND(WS, frontends, "displays the list of all registered front ends", "")
{
	if(args.size() != 0) return false;

	log.displayNL ("Display the %d registered front end :", FESList.size());
	for (list<CFES>::iterator it = FESList.begin(); it != FESList.end (); it++)
	{
		log.displayNL ("> FE %u: nb estimated users: %u nb users: %u", (*it).SId.get(), (*it).NbEstimatedUser, (*it).NbUser );
	}
	log.displayNL ("End ot the list");

	return true;
}

SIPBASE_CATEGORISED_COMMAND(WS, users, "displays the list of all registered users", "")
{
	if(args.size() != 0) return false;

	log.displayNL ("Display the %d registered users :", UserIdSockAssociations.size());
	for (TUSERIDASSOCIATION::iterator it = UserIdSockAssociations.begin(); it != UserIdSockAssociations.end (); it++)
	{
		log.displayNL ("> %u SId=%u", (*it).first, (*it).second.get());
	}
	log.displayNL ("End ot the list");

	return true;
}

SIPBASE_CATEGORISED_COMMAND(WS, displayOnlineServices, "Display the online service instances", "" )
{
	OnlineServices.display( log );
	return true;
}

SIPBASE_CATEGORISED_VARIABLE(WS, bool, OnlineStatus, "Main online status of the shard" );

///////////////////////////////////////////
//  ±â³äÀÏÈ°µ¿°ü·Ã
///////////////////////////////////////////
SITE_ACTIVITY	g_SiteActivity;
SYSTEM_GIFTITEM g_BeginnerMstItem;
SITE_CHECKIN	g_CheckIn;

void SendCurrentActivityData(const std::string &serviceName, TServiceId tsid)
{
	CMessage	msgOut(M_NT_CURRENT_ACTIVITY);
	msgOut.serial(g_SiteActivity.m_ActID);
	msgOut.serial(g_SiteActivity.m_Text);
	msgOut.serial(g_SiteActivity.m_Count);
	for (uint8 i = 0; i < g_SiteActivity.m_Count; i ++)
	{
		msgOut.serial(g_SiteActivity.m_ActItemID[i]);
		msgOut.serial(g_SiteActivity.m_ActItemCount[i]);
	}
	if (tsid.get() == 0)
	{
		CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, msgOut);
		CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgOut);
		CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, msgOut);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
	}
	else
	{
		if (strstr(serviceName.c_str(), LOBBY_SHORT_NAME) || strstr(serviceName.c_str(), OROOM_SHORT_NAME)
			|| strstr(serviceName.c_str(), RROOM_SHORT_NAME) || strstr(serviceName.c_str(), MANAGER_SHORT_NAME))
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
}

void SendCurrentCheckInData(TServiceId tsid)
{
	CMessage	msgOut(M_NT_CURRENT_CHECKIN);
	msgOut.serial(g_CheckIn.m_ActID, g_CheckIn.m_bActive, g_CheckIn.m_Text, g_CheckIn.m_BeginTime.timevalue, g_CheckIn.m_EndTime.timevalue);
	for (uint8 i = 0; i < CIS_MAX_DAYS; i ++)
	{
		msgOut.serial(g_CheckIn.m_CheckInItems[i].m_Count);
		for (uint8 j = 0; j < g_CheckIn.m_CheckInItems[i].m_Count; j++)
		{
			msgOut.serial(g_CheckIn.m_CheckInItems[i].m_ItemID[j], g_CheckIn.m_CheckInItems[i].m_ItemCount[j]);
		}
	}
	if (tsid.get() == 0)
	{
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
	}
	else
	{
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
}

void SendBeginnerMstItemToService(const std::string &serviceName, TServiceId tsid)
{
	if (g_BeginnerMstItem.m_GiftID == 0)
		return;

	CMessage	msgOut(M_NT_BEGINNERMSTITEM);
	msgOut.serial(g_BeginnerMstItem.m_GiftID, g_BeginnerMstItem.m_Title, g_BeginnerMstItem.m_Content, g_BeginnerMstItem.m_Count);
	for (int i = 0; i < g_BeginnerMstItem.m_Count; i ++)
	{
		msgOut.serial(g_BeginnerMstItem.m_ItemID[i], g_BeginnerMstItem.m_ItemCount[i]);
	}
	if (tsid.get() == 0)
	{
		CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, msgOut);
		CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgOut);
		CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, msgOut);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
	}
	else
	{
		if (strstr(serviceName.c_str(), LOBBY_SHORT_NAME) || strstr(serviceName.c_str(), OROOM_SHORT_NAME)
			|| strstr(serviceName.c_str(), RROOM_SHORT_NAME) || strstr(serviceName.c_str(), MANAGER_SHORT_NAME))
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
}


// LS¿¡Œ­ WS¿¡ ÇöÀçÀÇ ±â³äÀÏÈ°µ¿ÀÚ·ážŠ ºž³œŽÙ. ([d:ActID][u:Text][b:Count][ [d:ItemID][b:ItemCount] ])
void	cb_M_MS_CURRENT_ACTIVITY(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	msgin.serial(g_SiteActivity.m_ActID);
	msgin.serial(g_SiteActivity.m_Text);
	msgin.serial(g_SiteActivity.m_Count);
	for (uint8 i = 0; i < g_SiteActivity.m_Count; i ++)
	{
		msgin.serial(g_SiteActivity.m_ActItemID[i]);
		msgin.serial(g_SiteActivity.m_ActItemCount[i]);
	}

	SendCurrentActivityData("", TServiceId(0));
}

void	cb_M_MS_CHECK_USER_ACTIVITY(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	msgin.invert();
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgin);
}

void	cb_M_MS_BEGINNERMSTITEM(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	msgin.serial(g_BeginnerMstItem.m_GiftID);
	msgin.serial(g_BeginnerMstItem.m_Title);
	msgin.serial(g_BeginnerMstItem.m_Content);
	msgin.serial(g_BeginnerMstItem.m_Count);

	for (uint8 i = 0; i < g_BeginnerMstItem.m_Count; i ++)
	{
		msgin.serial(g_BeginnerMstItem.m_ItemID[i]);
		msgin.serial(g_BeginnerMstItem.m_ItemCount[i]);
	}

	SendBeginnerMstItemToService("", TServiceId(0));
}

void	cb_M_MS_CURRENT_CHECKIN(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	msgin.serial(g_CheckIn.m_ActID, g_CheckIn.m_bActive, g_CheckIn.m_Text, g_CheckIn.m_BeginTime.timevalue, g_CheckIn.m_EndTime.timevalue);
	for (uint8 i = 0; i < CIS_MAX_DAYS; i ++)
	{
		msgin.serial(g_CheckIn.m_CheckInItems[i].m_Count);
		for (uint8 j = 0; j < g_CheckIn.m_CheckInItems[i].m_Count; j++)
		{
			msgin.serial(g_CheckIn.m_CheckInItems[i].m_ItemID[j], g_CheckIn.m_CheckInItems[i].m_ItemCount[j]);
		}
	}
	SendCurrentCheckInData(TServiceId(0));
}

void cb_M_ENTER__SHARD(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	UID;
	uint16	fsSid;
	msgin.serial(UID, fsSid);

	CMessage	msgOut(M_ENTER__SHARD);
	msgOut.serial(UID);
	CUnifiedNetwork::getInstance()->send(TServiceId(fsSid), msgOut);
}

// Request to change shard ([b:NewShardCode]) Client->FS->WS->LS
void cb_M_CS_CHANGE_SHARD_REQ(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16	FsSid;
	uint32	FID;
	uint8	NewShardCode;

	FsSid = tsid.get();
	msgin.serial(FID, NewShardCode);

	if (IsThisShard(NewShardCode))
	{
		//sipwarning("No need to change shard. FID=%d, NewShardCode=%d", FID, NewShardCode); byy krs
//		uint32	err = ERR_NONEED_CHANGE_SHARD;
//		CMessage	msgOut(M_SC_CHANGE_SHARD_ANS);
//		msgOut.serial(FID, NewShardCode, err);
//		CUnifiedNetwork::getInstance()->send(TServiceId(FsSid), msgOut);
		return;
	}

	CMessage	msgOut(M_CS_CHANGE_SHARD_REQ);
	msgOut.serial(FsSid, FID, NewShardCode);
	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgOut);
}


static void	DBCB_DBGetUserMoveData(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if ( !nQueryResult )
		return;

	uint32	UID	= *(uint32 *)argv[0];
	uint32	FID	= *(uint32 *)argv[1];
	uint32	toShardID	= *(uint32 *)argv[2];

	string	toShardName = GetWSNameFromShardID(toShardID);

	CMessage	msgOut1(M_NT_SEND_USER_DATA);
	msgOut1.serial(UID, FID, MOVE_USER_DATA_flagStart);
//	select GMoney, FLevel, UserExp  from tbl_Family  where FamilyId=@fid
	{
		uint32 nRows;
		resSet->serial(nRows);
		if (nRows != 1)
		{
			sipwarning("Invalid data in tbl_Family. UID=%d, FID=%d, toShardID=%d, nRows=%d", UID, FID, toShardID, nRows);
			return;
		}
		uint32	GMoney, UserExp;
		uint8	FLevel;
		resSet->serial(GMoney);
		resSet->serial(FLevel);
		resSet->serial(UserExp);

		msgOut1.serial(MOVE_USER_DATA_tbl_Family);
		msgOut1.serial(GMoney, FLevel, UserExp);
	}
//	select ExpHisData from tbl_Family_ExpHis where FamilyId=@fid
	{
		uint32 nRows;
		resSet->serial(nRows);
		if (nRows == 1)
		{
			uint32	len;		resSet->serial(len);
			uint8	Buf[40 * 8];	// sizeof(_Family_Exp_His) * FAMILY_EXP_TYPE_MAX
			resSet->serialBuffer(Buf, len);

			msgOut1.serial(MOVE_USER_DATA_tbl_Family_ExpHis);
			msgOut1.serial(len);
			msgOut1.serialBuffer(Buf, len);
		}
	}
//	select  Name, ModelId, SexId, DefaultDressId, FaceId, HairId, DressId, BirthDate, City, [Resume] from tbl_Character  where FamilyId=@fid
	{
		uint32 nRows;
		resSet->serial(nRows);
		if (nRows != 1)
		{
			sipwarning("Invalid data in tbl_Character. UID=%d, FID=%d, toShardID=%d, nRows=%d", UID, FID, toShardID, nRows);
			return;
		}
		ucstring	Name, Resume;
		uint32		ModelID, DefaultDressID, FaceID, HairID, DressID, Birthday, City;
		uint8		SexID;
		resSet->serial(Name);
		resSet->serial(ModelID);
		resSet->serial(SexID);
		resSet->serial(DefaultDressID);
		resSet->serial(FaceID);
		resSet->serial(HairID);
		resSet->serial(DressID);
		resSet->serial(Birthday);
		resSet->serial(City);
		resSet->serial(Resume);

		msgOut1.serial(MOVE_USER_DATA_tbl_Character);
		msgOut1.serial(Name, ModelID, SexID, DefaultDressID, FaceID);
		msgOut1.serial(HairID, DressID, Birthday, City, Resume);
	}
//	select  BlessCardID, BCount from tbl_BlessCard  where FamilyID=@fid
	{
		uint32 nRows;
		resSet->serial(nRows);

		msgOut1.serial(MOVE_USER_DATA_tbl_BlessCard);
		msgOut1.serial(nRows);
		uint8		BlessCardID, BCount;
		for (uint32 i = 0; i < nRows; i ++)
		{
			resSet->serial(BlessCardID);
			resSet->serial(BCount);

			msgOut1.serial(BlessCardID, BCount);
		}
	}
//	select  QFVirtue, QFVirtueHis from  tbl_Family_Virtue  where FamilyID=@fid
	{
		uint32 nRows;
		resSet->serial(nRows);
		if (nRows == 1)
		{
			uint32	QFVirtue;
			resSet->serial(QFVirtue);
			uint32	len;		resSet->serial(len);
			uint8	Buf[8 * 10];	// sizeof(_Family_Exp_His) * FAMILY_VIRTUE_TYPE_MAX
			resSet->serialBuffer(Buf, len);

			msgOut1.serial(MOVE_USER_DATA_tbl_Family_Virtue);
			msgOut1.serial(QFVirtue, len);
			msgOut1.serialBuffer(Buf, len);
		}
	}
//	select ItemData from  tbl_Item  where Fid = @fid
	CMessage	msgOut2(M_NT_SEND_USER_DATA);
	msgOut2.serial(UID, FID, MOVE_USER_DATA_flagContinue);
	{
		uint32 nRows;
		resSet->serial(nRows);
		if (nRows == 1)
		{
			uint32	len;		resSet->serial(len);
			uint8	InvenBuf[MAX_INVENBUF_SIZE];
			resSet->serialBuffer(InvenBuf, len);

			msgOut2.serial(MOVE_USER_DATA_tbl_Item);
			msgOut2.serial(len);
			msgOut2.serialBuffer(InvenBuf, len);
		}
	}
//	select OwnRoomOrder, ManageRoomOrder, FavoRoomOrder from tbl_Family_RoomOrderInfo  where FamilyId=@fid
	CMessage	msgOut4(M_NT_SEND_USER_DATA);
	msgOut4.serial(UID, FID, MOVE_USER_DATA_flagContinue);
	{
		uint32 nRows;
		resSet->serial(nRows);
		if (nRows == 1)
		{
			uint32	len1;		resSet->serial(len1);
			uint8	Buf1[MAX_OWNROOMCOUNT * 5];
			resSet->serialBuffer(Buf1, len1);

			uint32	len2;		resSet->serial(len2);
			uint8	Buf2[MAX_FAVORROOMNUM * 5];
			resSet->serialBuffer(Buf2, len2);

			uint32	len3;		resSet->serial(len3);
			uint8	Buf3[MAX_FAVORROOMNUM * MAX_FAVORROOMGROUP * 5];
			resSet->serialBuffer(Buf3, len3);

			msgOut4.serial(MOVE_USER_DATA_tbl_Family_RoomOrderInfo);
			msgOut4.serial(len1);
			msgOut4.serialBuffer(Buf1, len1);
			msgOut4.serial(len2);
			msgOut4.serialBuffer(Buf2, len2);
			msgOut4.serial(len3);
			msgOut4.serialBuffer(Buf3, len3);
		}
	}
//	select SigninData, SigninCount from ttbl_Family_Signin_Day  where FID=@fid
	CMessage	msgOut5(M_NT_SEND_USER_DATA);
	msgOut5.serial(UID, FID, MOVE_USER_DATA_flagEnd);
	{
		uint32 nRows;
		resSet->serial(nRows);
		if (nRows == 1)
		{
			string	SigninData;
			uint8	SigninCount;
			resSet->serial(SigninData);
			resSet->serial(SigninCount);

			msgOut5.serial(MOVE_USER_DATA_tbl_Family_Signin_Day);
			msgOut5.serial(SigninData, SigninCount);
		}
	}

	CUnifiedNetwork::getInstance()->send(toShardName, msgOut1);
	CUnifiedNetwork::getInstance()->send(toShardName, msgOut2);
	CUnifiedNetwork::getInstance()->send(toShardName, msgOut4);
	CUnifiedNetwork::getInstance()->send(toShardName, msgOut5);
}

// Request to move UserData from shard to shard ([d:UID][d:FID][d:toShardID]) LS->fromWS
void cb_M_NT_MOVE_USER_DATA_REQ(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	UID, FID, toShardID;
	msgin.serial(UID, FID, toShardID);

	uint8 userMainShardCode = (uint8)((FID >> 24) + 1);
	bool	isThisShardUser = IsThisShard(userMainShardCode);

	MAKEQUERY_GetUserMoveData(strQuery, FID);
	DBCaller->execute(strQuery, DBCB_DBGetUserMoveData,
		"DDD",
		CB_,		UID,
		CB_,		FID,
		CB_,		toShardID);
}

void binToWstr(uint8* src, int num1, ucchar* dst)
{
	for (int i = 0; i < num1; i ++)
	{
		swprintf((ucchar*)(&dst[i * 2]), sizeof(uint8), _t("%02x"), src[i]);
	}
	dst[num1 * 2] = 0;
}

// Send UserData ([d:UID][d:FID][b:flag,1-Start,2-End,0-Continue][...]) fromWS->toWS
void cb_M_NT_SEND_USER_DATA(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	UID, FID;
	uint8	flag;
	msgin.serial(UID, FID, flag);

	if (flag == MOVE_USER_DATA_flagStart)
	{
		MAKEQUERY_Shrd_Family_SaveMovableInfo_reset(strQuery, FID);
		CDBConnectionRest	DBConnection(DBCaller);
		DB_EXE_QUERY(DBConnection, Stmt, strQuery);
	}

	while (msgin.getPos() < (int)msgin.lengthR())
	{
		uint8	type;
		msgin.serial(type);

		if (type == MOVE_USER_DATA_tbl_Family)
		{
			uint32	GMoney, UserExp;
			uint8	FLevel;

			msgin.serial(GMoney, FLevel, UserExp);

			{
				MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family(strQuery, FID, UID, GMoney, UserExp, FLevel);
				CDBConnectionRest	DBConnection(DBCaller);
				DB_EXE_QUERY(DBConnection, Stmt, strQuery);
			}
		}
		else if (type == MOVE_USER_DATA_tbl_Family_ExpHis)
		{
			uint32	len;
			uint8	Buf[8 * 40];	// sizeof(_Family_Exp_His) * FAMILY_EXP_TYPE_MAX

			msgin.serial(len);
			msgin.serialBuffer(Buf, len);

			ucchar	wBuf[sizeof(Buf) * 2 + 10];
			binToWstr(Buf, len, wBuf);
			{
				MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family_ExpHis(strQuery, FID, wBuf);
				CDBConnectionRest	DBConnection(DBCaller);
				DB_EXE_QUERY(DBConnection, Stmt, strQuery);
			}
		}
		else if (type == MOVE_USER_DATA_tbl_Character)
		{
			ucstring	Name, Resume;
			uint32		ModelID, DefaultDressID, FaceID, HairID, DressID, Birthday, City;
			uint8		SexID;

			msgin.serial(Name, ModelID, SexID, DefaultDressID, FaceID);
			msgin.serial(HairID, DressID, Birthday, City, Resume);

			{
				MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Character(strQuery, FID, Name, ModelID, SexID, DefaultDressID, FaceID, HairID, DressID, Birthday, City, Resume);
				CDBConnectionRest	DBConnection(DBCaller);
				DB_EXE_QUERY(DBConnection, Stmt, strQuery);
			}
		}
		else if (type == MOVE_USER_DATA_tbl_BlessCard)
		{
			uint32	nRows;
			uint8	BlessCardID, BCount;

			msgin.serial(nRows);
			for (uint32 i = 0; i < nRows; i ++)
			{
				msgin.serial(BlessCardID, BCount);

				{
					MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_BlessCard(strQuery, FID, BlessCardID, BCount);
					CDBConnectionRest	DBConnection(DBCaller);
					DB_EXE_QUERY(DBConnection, Stmt, strQuery);
				}
			}
		}
		else if (type == MOVE_USER_DATA_tbl_Family_Virtue)
		{
			uint32	QFVirtue;
			uint32	len;
			uint8	Buf[8 * 10];	// sizeof(_Family_Exp_His) * FAMILY_VIRTUE_TYPE_MAX

			msgin.serial(QFVirtue, len);
			msgin.serialBuffer(Buf, len);

			ucchar	wBuf[sizeof(Buf) * 2 + 10];
			binToWstr(Buf, len, wBuf);
			{
				MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family_Virtue(strQuery, FID, QFVirtue, wBuf);
				CDBConnectionRest	DBConnection(DBCaller);
				DB_EXE_QUERY(DBConnection, Stmt, strQuery);
			}
		}
		else if (type == MOVE_USER_DATA_tbl_Item)
		{
			uint32	len;
			uint8	Buf[MAX_INVENBUF_SIZE];

			msgin.serial(len);
			msgin.serialBuffer(Buf, len);

			ucchar	wBuf[sizeof(Buf) * 2 + 10];
			binToWstr(Buf, len, wBuf);
			{
				MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Item(strQuery, FID, wBuf);
				CDBConnectionRest	DBConnection(DBCaller);
				DB_EXE_QUERY(DBConnection, Stmt, strQuery);
			}
		}
		else if (type == MOVE_USER_DATA_tbl_Family_RoomOrderInfo)
		{
			uint32	len1;
			uint8	Buf1[MAX_OWNROOMCOUNT * 5];
			uint32	len2;
			uint8	Buf2[MAX_FAVORROOMNUM * 5];
			uint32	len3;
			uint8	Buf3[MAX_FAVORROOMNUM * MAX_FAVORROOMGROUP * 5];

			msgin.serial(len1);
			msgin.serialBuffer(Buf1, len1);
			msgin.serial(len2);
			msgin.serialBuffer(Buf2, len2);
			msgin.serial(len3);
			msgin.serialBuffer(Buf3, len3);

			ucchar	wBuf1[sizeof(Buf1) * 2 + 10];
			binToWstr(Buf1, len1, wBuf1);
			ucchar	wBuf2[sizeof(Buf2) * 2 + 10];
			binToWstr(Buf2, len2, wBuf2);
			ucchar	wBuf3[sizeof(Buf3) * 2 + 10];
			binToWstr(Buf3, len3, wBuf3);

			{
				MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family_RoomOrderInfo(strQuery, FID, wBuf1, wBuf2, wBuf3);
				CDBConnectionRest	DBConnection(DBCaller);
				DB_EXE_QUERY(DBConnection, Stmt, strQuery);
			}
		}
		else if (type == MOVE_USER_DATA_tbl_Family_Signin_Day)
		{
			string	SigninData;
			uint8	SigninCount;

			msgin.serial(SigninData, SigninCount);

			{
				MAKEQUERY_Shrd_Family_SaveMovableInfo_tbl_Family_Signin_Day(strQuery, FID, SigninData, SigninCount);
				CDBConnectionRest	DBConnection(DBCaller);
				DB_EXE_QUERY(DBConnection, Stmt, strQuery);
			}
		}
		else
		{
			sipwarning("Invalid MOVE_USER_DATA type. type=%d", type);
			break;
		}
	}

	if (flag == MOVE_USER_DATA_flagEnd)
	{
		CMessage	msgOut(M_NT_MOVE_USER_DATA_END);
		msgOut.serial(UID, FID);
		CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgOut);
	}
}

// Notice that character's name changed to OROOM & LOBBY ([d:FID][u:Family name][d:ModelID])	LS->ORoom, RRoom, Lobby, FS, WS
void cb_M_NT_CHANGE_NAME(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32		FID;
	ucstring	FamilyName;
	uint32		ModelId;

	msgin.serial(FID, FamilyName, ModelId);

	CMessage	msgOut(M_NT_CHANGE_USERNAME);
	msgOut.serial(FID, FamilyName, ModelId);

	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgOut);
}

// Notice that character's name changed to OROOM & LOBBY ([d:FID][u:Family name])	WS->other WS
void cb_M_NT_CHANGE_USERNAME(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32		FID;
	ucstring	FamilyName;

	msgin.serial(FID, FamilyName);

	{
		MAKEQUERY_Shrd_Merge_Character_UpdateName(strQuery, FID, FamilyName);
		CDBConnectionRest	DBConnection(DBCaller);
		DB_EXE_QUERY(DBConnection, Stmt, strQuery);
	}
}

static void	DBCB_DBGetRoomInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			_FID	= *(T_FAMILYID *)argv[0];
	T_ROOMID			_RoomID	= *(T_ROOMID *)argv[1];
	uint16				fsSid = *(uint16*)argv[2];
	TServiceId			tsid(*(uint16 *)argv[3]);

	CMessage	msgout(M_SC_ROOMINFO);
	if (fsSid == 0)
	{
		msgout.serial(_FID);
	}
	else
	{
		msgout.serial(fsSid, _FID);
	}
	uint32	uResult = E_NO_EXIST;
	{
		if (nQueryResult == true)
		{
			uint32 nRows;	resSet->serial(nRows);
			if (nRows > 0)
			{
				uint16			sceneId;				resSet->serial(sceneId);
				T_ROOMNAME		roomName;				resSet->serial(roomName);
				T_FAMILYNAME	familyName;				resSet->serial(familyName);
				T_DATETIME		creationtime;			resSet->serial(creationtime);
				T_DATETIME		termtime;				resSet->serial(termtime);
				uint8			idOpenability;			resSet->serial(idOpenability);
				T_COMMON_CONTENT comment;				resSet->serial(comment);
				uint8			freeFlag;				resSet->serial(freeFlag);
				uint32			nVisits;				resSet->serial(nVisits);
				uint8			manageFlag;				resSet->serial(manageFlag);
				T_DATETIME		renewtime;				resSet->serial(renewtime);
				uint32			fid;					resSet->serial(fid);
				uint32			HisSpace_All;			resSet->serial(HisSpace_All);
				uint32			HisSpace_Used;			resSet->serial(HisSpace_Used);
				uint8			deletedFlag;			resSet->serial(deletedFlag);
				T_DATETIME		deletedTime;			resSet->serial(deletedTime);
				uint8			bClosed;				resSet->serial(bClosed);
				T_ROOMLEVEL		level;					resSet->serial(level);
				T_ROOMEXP		exp;					resSet->serial(exp);
				uint8			surnameLen1;			resSet->serial(surnameLen1);
				T_DECEASED_NAME	deadname1;				resSet->serial(deadname1);
				uint8			surnameLen2;			resSet->serial(surnameLen2);
				T_DECEASED_NAME	deadname2;				resSet->serial(deadname2);
				uint8			PhotoType;				resSet->serial(PhotoType);

				uint32	nTermTime = getMinutesSince1970(termtime);
				uint32	nCurtime = (uint32)(GetDBTimeSecond() / 60);
				if(deletedFlag)
				{
					uResult = E_NO_EXIST;
				}
				else if(bClosed)
				{
					uResult = E_ENTER_CLOSE;
				}
				else if(nTermTime > 0 && nCurtime > nTermTime)
				{
					uResult = E_ENTER_EXPIRE;
				}
				else
				{
					//CRoomWorld	*_RoomManager = GetOpenRoomWorld(_RoomID);
					//if(_RoomManager)
					//{
					//	level = _RoomManager->m_Level;
					//	exp = _RoomManager->m_Exp;
					//}

					uint32	nCreateTime = getMinutesSince1970(creationtime);
					uint32	nTermTime = getMinutesSince1970(termtime);
					uint32	nRenewTime = getMinutesSince1970(renewtime);
					if ( deletedFlag )	nTermTime = getMinutesSince1970(deletedTime);
					//uint16		exp_percent = CalcRoomExpPercent(level, exp);

					uResult = ERR_NOERR;
					msgout.serial(uResult, _RoomID, level, exp); // , exp_percent);
					msgout.serial(sceneId, roomName, fid, familyName, nCreateTime, nTermTime);
					msgout.serial(idOpenability, comment, freeFlag, nVisits, manageFlag, nRenewTime);
					msgout.serial(HisSpace_All, HisSpace_Used, deletedFlag, PhotoType, deadname1, surnameLen1, deadname2, surnameLen2);
				}
			}
		}
	}
	if (uResult != ERR_NOERR)
	{
		msgout.serial(uResult, _RoomID);
	}

	CUnifiedNetwork::getInstance ()->send(tsid, msgout);
	//sipinfo("Send room(id:%u) info to userid(id:%u), Result=%d", _RoomID, _FID, uResult); byy krs
}

// Request room info ([d:RoomId])
void	cb_M_CS_ROOMINFO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_ROOMID	RoomID;		_msgin.serial(RoomID);
	/*T_ID    passWord;  	_msgin.serial(passWord);

	uint32 uResult = ERR_NOERR;*/

	if (IsThisShardRoom(RoomID))
	{
		uint16	fsSid = 0;
		MAKEQUERY_GetRoomInfo(strQuery, FID, RoomID);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetRoomInfo,
			"DDWW",
			CB_,			FID,
			CB_,			RoomID,
			CB_,			fsSid,
			CB_,			_tsid.get());

		return;
	}

	{
		CMessage	msgOut(M_NT_REQ_ROOMINFO);
		uint16	fsSid = _tsid.get();
		msgOut.serial(fsSid, FID, RoomID);

		SendMsgToShardByRoomID(msgOut, RoomID);
	}
}

// Request room info request shard ([w:fsSid][d:FID][d:RoomID]) WS->WS
void cb_M_NT_REQ_ROOMINFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16		fsSid;
	T_FAMILYID	FID;
	T_ROOMID	RoomID;

	msgin.serial(fsSid, FID, RoomID);

	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom failed. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	{
		MAKEQUERY_GetRoomInfo(strQuery, FID, RoomID);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetRoomInfo,
			"DDWW",
			CB_,			FID,
			CB_,			RoomID,
			CB_,			fsSid,
			CB_,			tsid.get());
	}
}

// Response of M_NT_REQ_ROOMINFO ([w:fsSid][d:FID][d:Result code][d:RoomId][b:Level][d:Exp][w:SceneId][u:Name][d:MasterId][u:MasterName][M1970:CreationTime][M1970:TermTime][b:OpenFlag][u:Comment][b:FreeFlag][d:VisitCount][b:Authority][M1970:RenewTime][d:HisSpaceSize][d:HisSpaceUsed][b:DeletedFlag][b:PhotoType][u:Dead name1][b:Dead surnamelen1][u:Dead name2][b:Dead surnamelen2]) WS->WS
void cb_M_SC_ROOMINFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16		fsSid;
	CMessage	msgOut;
	msgin.copyWithoutPrefix(fsSid, msgOut);

	CUnifiedNetwork::getInstance()->send(TServiceId(fsSid), msgOut);
}

// Request dead info([w:fsSid][d:FID][d:Room id][s:password]) OROOM->WS
void	cb_M_CS_DECEASED(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint16			fsSid;			_msgin.serial(fsSid);
	T_FAMILYID		FID;			_msgin.serial(FID);
	T_ROOMID		RoomID;			_msgin.serial(RoomID);
	std::string		passWord;		_msgin.serial(passWord);

	CMessage	msgOut(M_NT_REQ_DECEASED);
	msgOut.serial(fsSid, FID, RoomID, passWord);

	SendMsgToShardByRoomID(msgOut, RoomID);
}


static void	DBCB_DBGetDeceasedInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32		ret			= *(sint32 *)argv[0];
	T_FAMILYID	FID			= *(T_FAMILYID *)argv[1];
	T_ROOMID	RoomID		= *(T_ROOMID *)argv[2];
	uint16		fsSid		= *(uint16 *)argv[3];
	TServiceId	tsid(*(uint16 *)argv[4]);

	uint32 uResult = ERR_NOERR;
	try
	{
		if (!nQueryResult || ret == -1)
			throw "Shrd_room_getDeceased2: execute failed!";

		CMessage	msgout(M_SC_DECEASED);
		msgout.serial(fsSid, FID);	

		switch (ret)
		{
		case 0:
			{
				uint32 nRows;	resSet->serial(nRows);
				if (nRows > 0)
				{
					T_ROOMNAME		roomname;				resSet->serial(roomname);
					uint8			surnamelen1;			resSet->serial(surnamelen1);
					T_DECEASED_NAME deadname1;				resSet->serial(deadname1);
					T_SEX			sex1;					resSet->serial(sex1);
					uint32			birthday1;				resSet->serial(birthday1);
					uint32			deadday1;				resSet->serial(deadday1);
					T_COMMON_CONTENT his1;					resSet->serial(his1);
					T_DATETIME		updatetime1;			resSet->serial(updatetime1);
					ucstring		domicile1;				resSet->serial(domicile1);
					ucstring		nation1;				resSet->serial(nation1);
					uint32			photoid1;				resSet->serial(photoid1);
					uint8			phototype;				resSet->serial(phototype);
					uint8			surnamelen2;			resSet->serial(surnamelen2);
					T_DECEASED_NAME deadname2;				resSet->serial(deadname2);
					T_SEX			sex2;					resSet->serial(sex2);
					uint32			birthday2;				resSet->serial(birthday2);
					uint32			deadday2;				resSet->serial(deadday2);
					T_COMMON_CONTENT his2;					resSet->serial(his2);
					T_DATETIME		updatetime2;			resSet->serial(updatetime2);
					ucstring		domicile2;				resSet->serial(domicile2);
					ucstring		nation2;				resSet->serial(nation2);
					uint32			photoid2;				resSet->serial(photoid2);

					msgout.serial(uResult, RoomID, roomname);
					msgout.serial(deadname1, surnamelen1, sex1, birthday1, deadday1, his1, nation1, domicile1, photoid1);
					msgout.serial(phototype);
					msgout.serial(deadname2, surnamelen2, sex2, birthday2, deadday2, his2, nation2, domicile2, photoid2);
				}
				break;
			}
		case 1043:
			{
				uResult = E_DECEASED_NODATA;
				sipdebug("the room:%u has no dead information", RoomID);				
				break;			
			}
		case 1014:
			{
				uResult = E_ENTER_NO_NEIGHBOR;
				sipdebug("the room:%u open type is ROOMOPEN_TONEIGHBOR, the user:%u is not contact of the room's master", RoomID, FID);				
				break;
			}
		case 1037:
			{
				uResult = ERR_BADPASSWORD;
				sipwarning("the room:%u open type is ROOMOPEN_PASSWORD", RoomID);
				break;			
			}
		case 1044: // »èÁŠµÈ¹æ
			{
				uResult = E_ENTER_DELETED;
				sipdebug("the room:%u has no dead information", RoomID);				
				break;			
			}
		case 1045: // ºñ°ø°³¹æ
			{
				uResult = E_ENTER_NO_ONLYMASTER;
				sipdebug("the room:%u has no dead information", RoomID);				
				break;			
			}
		case 1046: // ±âÇÑÁö³­¹æ
			{
				uResult = E_ENTER_EXPIRE;
				sipdebug("the room:%u has no dead information", RoomID);				
				break;			
			}
		default:
			break;
		}

		if (uResult != ERR_NOERR)
		{
			msgout.serial(uResult, RoomID);
		}
		CUnifiedNetwork::getInstance()->send(tsid, msgout);
		//sipinfo("Send room(id:%u) info to userid(id:%u), Result=%d", RoomID, FID, uResult); byy krs
	}
	catch(char * str)
	{
		uResult = E_DBERR;
		CMessage	msgout(M_SC_DECEASED);
		msgout.serial(fsSid, FID);
		msgout.serial(uResult, RoomID);
		CUnifiedNetwork::getInstance()->send(tsid, msgout);
		sipwarning("[FID=%d] %s", FID, str);			
	}
}

// Request dead info([w:fsSid][d:FID][d:Room id][s:password]) WS->WS
void cb_M_NT_REQ_DECEASED(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16		fsSid;
	T_FAMILYID	FID;
	T_ROOMID	RoomID;
	std::string	passWord;

	msgin.serial(fsSid, FID, RoomID, passWord);

	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom failed. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	{
		MAKEQUERY_GetDeceasedInfo(strQ, RoomID, FID, passWord);
		DBCaller->executeWithParam(strQ, DBCB_DBGetDeceasedInfo,
			"DDDWW",
			OUT_,			NULL,
			CB_,			FID,
			CB_,			RoomID,
			CB_,			fsSid,
			CB_,			tsid.get());
	}
}

// Get dead([d:Result code][d:Room id][u:Room name][u:Dead name1][b:Dead surnamelen1][b:Sex1][d:Birthday1][d:Death day1][u:Brief history1][u:Nation1][u:domicile1][d:PhotoId1][b:PhotoType][u:Dead name2][b:Dead surnamelen2][b:Sex2][d:Birthday2][d:Death day2][u:Brief history2][u:Nation2][u:domicile2][d:PhotoId2])
void cb_ToSpecialService(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16		fsSid;
	CMessage	msgOut;
	msgin.copyWithoutPrefix(fsSid, msgOut);

	CUnifiedNetwork::getInstance()->send(TServiceId(fsSid), msgOut);
}

void cb_ToUID(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32		UID;
	CMessage	msgOut;
	msgin.copyWithoutPrefix(UID, msgOut);

	map<uint32, TServiceId>::iterator	it = UserIdSockAssociations.find(UID);
	if (it != UserIdSockAssociations.end())
	{
		CUnifiedNetwork::getInstance()->send(it->second, msgOut);
	}
	else
	{
//		sipinfo("UserIdSockAssociations failed. UID=%d, MsgID=%d", UID, msgOut.getName());
		return;
	}
}


struct	FavorRoom
{
	T_ROOMID	RoomId;
	T_ID		FamilyId;
	T_SCENEID	SceneId;
	T_NAME		RoomName;
	T_FLAG		nOpenFlag;
	T_NAME		MasterName;
	T_M1970	ExpireTime;
	//	uint8		DeleteFlag;
	uint32		DeleteRemainMinutes;
	T_ROOMLEVEL	level;
	T_ROOMEXP	exp;
	uint32		VisitCount;
	T_NAME		DeadName1;
	uint8		DeadSurnamelen1;
	T_NAME		DeadName2;
	uint8		DeadSurnamelen2;

	uint8		PhotoType;

	FavorRoom()	{}
	FavorRoom(T_ROOMID _rid, T_ID _fid, T_SCENEID _sceneid, T_NAME& _rname, T_FLAG _oflag, 
		T_NAME& _mname, T_NAME& _DeadName1, uint8 _DeadSurnamelen1, T_NAME& _DeadName2, uint8 _DeadSurnamelen2, T_M1970 _expire, uint32 _deleteremainminutes, T_ROOMLEVEL _level, T_ROOMEXP _exp, uint32 _VisitCount, uint8 _photoType) :
	RoomId(_rid), FamilyId(_fid), SceneId(_sceneid), RoomName(_rname), nOpenFlag(_oflag), 
		MasterName(_mname), DeadName1(_DeadName1), DeadSurnamelen1(_DeadSurnamelen1), DeadName2(_DeadName2), DeadSurnamelen2(_DeadSurnamelen2), ExpireTime(_expire), DeleteRemainMinutes(_deleteremainminutes), level(_level), exp(_exp), VisitCount(_VisitCount), PhotoType(_photoType)
	{}
};
struct FavoriteDeletedRooms
{
	T_ROOMID	RoomID;
	ucstring	RoomName;

	FavoriteDeletedRooms(T_ROOMID _RoomID, ucstring _RoomName) : RoomID(_RoomID), RoomName(_RoomName) {}
};
const	int Favorroom_Send_Count = 10;
static void	DBCB_GetRoomListInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID	FID			= *(T_FAMILYID *)argv[0];
	uint16		fsSid		= *(uint16 *)argv[1];
	TServiceId	tsid(*(uint16 *)argv[2]);

	if (!nQueryResult)
		return;

	// My Favor Room
	uint8			count = 0;
	FavorRoom		Favorrooms[Favorroom_Send_Count];
	std::vector<FavoriteDeletedRooms> DeletedRooms;

	{
		uint32	nRows;		resSet->serial(nRows);

		bool	bSended = false;
		for (uint32 i = 0; i < nRows; i ++)
		{
			T_ROOMID	roomid;		resSet->serial(roomid);
			T_ID		familyid;	resSet->serial(familyid);
			T_SCENEID	sceneid;	resSet->serial(sceneid);
			ucstring	roomname;	resSet->serial(roomname);
			T_FLAG		openflag;	resSet->serial(openflag);
			ucstring	mastername;	resSet->serial(mastername);
			std::string	termtime;	resSet->serial(termtime);
			uint8		deleteflag;	resSet->serial(deleteflag);
			std::string	deletetime;	resSet->serial(deletetime);
			T_ROOMLEVEL	level;		resSet->serial(level);
			T_ROOMEXP	exp;		resSet->serial(exp);
			uint32		visitcount;	resSet->serial(visitcount);
			uint8		deadsurnamelen1;resSet->serial(deadsurnamelen1);
			ucstring	deadname1;	resSet->serial(deadname1);
			uint8		deadsurnamelen2;resSet->serial(deadsurnamelen2);
			ucstring	deadname2;	resSet->serial(deadname2);
			uint8		potoType;	resSet->serial(potoType);

			T_M1970	nTermTime = getMinutesSince1970(termtime);
			uint32	deleteremainminutes = 0;
			if (deleteflag)
			{
				nTermTime = getMinutesSince1970(deletetime);
				uint32	nCurtime = (uint32)(GetDBTimeSecond() / 60);
				if (nCurtime >= nTermTime)
				{
					FavoriteDeletedRooms	data(roomid, roomname);
					DeletedRooms.push_back(data);
					continue;
				}
				deleteremainminutes = nTermTime - nCurtime;
			}

			Favorrooms[count].RoomId = roomid;
			Favorrooms[count].FamilyId = familyid;
			Favorrooms[count].SceneId = sceneid;
			Favorrooms[count].RoomName = roomname;
			Favorrooms[count].nOpenFlag = openflag;
			Favorrooms[count].MasterName = mastername;
			Favorrooms[count].ExpireTime = nTermTime;
			Favorrooms[count].DeleteRemainMinutes = deleteremainminutes;
			Favorrooms[count].level = level;
			Favorrooms[count].exp = exp;
			Favorrooms[count].VisitCount = visitcount;
			Favorrooms[count].DeadName1 = deadname1;
			Favorrooms[count].DeadSurnamelen1 = deadsurnamelen1;
			Favorrooms[count].DeadName2 = deadname2;
			Favorrooms[count].DeadSurnamelen2 = deadsurnamelen2;
			Favorrooms[count].PhotoType = potoType;
			count ++;

			if (count >= Favorroom_Send_Count)
			{
				CMessage	msgout(M_SC_FAVORROOM_INFO);
				if (fsSid == 0)
					msgout.serial(FID, count);
				else
					msgout.serial(fsSid, FID, count);
				for (uint8 i = 0; i < count; i ++)
				{
					FavorRoom&	r = Favorrooms[i];

					msgout.serial(r.SceneId, r.RoomId, r.FamilyId, r.nOpenFlag, r.RoomName, r.MasterName);
					msgout.serial(r.ExpireTime, r.DeleteRemainMinutes, r.level, r.exp, r.VisitCount);
					msgout.serial(r.PhotoType, r.DeadName1, r.DeadSurnamelen1, r.DeadName2, r.DeadSurnamelen2);
				}
				CUnifiedNetwork::getInstance()->send(tsid, msgout);
				count = 0;
				bSended = true;
			}
		}

		if ((count > 0) || !bSended)
		{
			CMessage	msgout(M_SC_FAVORROOM_INFO);
			if (fsSid == 0)
				msgout.serial(FID, count);
			else
				msgout.serial(fsSid, FID, count);
			for (uint8 i = 0; i < count; i ++)
			{
				FavorRoom&	r = Favorrooms[i];

				msgout.serial(r.SceneId, r.RoomId, r.FamilyId, r.nOpenFlag, r.RoomName, r.MasterName);
				msgout.serial(r.ExpireTime, r.DeleteRemainMinutes, r.level, r.exp, r.VisitCount);
				msgout.serial(r.PhotoType, r.DeadName1, r.DeadSurnamelen1, r.DeadName2, r.DeadSurnamelen2);
			}
			CUnifiedNetwork::getInstance()->send(tsid, msgout);
			count = 0;
		}
	}

	if (!DeletedRooms.empty())
	{
		CMessage	msgOut(M_SC_FAVORROOM_AUTODEL);
		if (fsSid == 0)
			msgOut.serial(FID);
		else
			msgOut.serial(fsSid, FID);

		for (std::vector<FavoriteDeletedRooms>::iterator it = DeletedRooms.begin(); it != DeletedRooms.end(); it ++)
		{
			T_ROOMID	RoomID = it->RoomID;

			msgOut.serial(RoomID, it->RoomName);

			// LS¿¡ ºž³»¿© Ã³ž®
			{
				CMessage	msgOut1(M_CS_DELFAVORROOM);
				msgOut1.serial(FID, RoomID);
				CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgOut1);
			}
		}

		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
}

void ReadFavorRoomInfo(T_FAMILYID FID, uint32 RoomCount, T_ROOMID* ptargetRoomIDs, uint16 fsSid, TServiceId _tsid)
{
	ucstring	strRooms;
	tchar		buff[32] = _t("");
	for (uint32 i = 0; i < RoomCount; i ++)
	{
		if (i == 0)
			SIPBASE::smprintf(buff, 32, _t("%d"), ptargetRoomIDs[i]);
		else
			SIPBASE::smprintf(buff, 32, _t(",%d"), ptargetRoomIDs[i]);
		strRooms += buff;
	}

	{
		MAKEQUERY_Shrd_GetRoomListInfo(strQ, strRooms);
		DBCaller->executeWithParam(strQ, DBCB_GetRoomListInfo,
			"DWW",
			CB_,			FID,
			CB_,			fsSid,
			CB_,			_tsid.get());
	}
}

// Request FavorRoom's Info ([d:RoomCount][[d:RoomID]])
void	cb_M_CS_REQ_FAVORROOM_INFO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	int		targetShardCount = 0;
	string	targetShardNames[26];
	uint32	targetRoomCount[26];
	uint32	targetRoomIDs[26][MAX_FAVORROOMGROUP * MAX_FAVORROOMNUM];

	T_FAMILYID	FID;		_msgin.serial(FID);
	uint32		RoomCount;	_msgin.serial(RoomCount);
	T_ROOMID	RoomID;

	for (uint32 i = 0; i < RoomCount; i ++)
	{
		_msgin.serial(RoomID);
		uint8	ShardCode = (uint8)(RoomID >> 24);
		string	shardName;
		if (ShardCode == 0)
			shardName = "0";
		else
		{
			shardName = GetShardNameFromShardCode(ShardCode);
			if (shardName == "")	// Unknown Shard
				continue;
		}

		int j = 0;
		for (j = 0; j < targetShardCount; j ++)
		{
			if (targetShardNames[j] == shardName)
			{
				targetRoomIDs[j][targetRoomCount[j]] = RoomID;
				targetRoomCount[j] ++;
				break;
			}
		}
		if (j >= targetShardCount)
		{
			targetShardNames[targetShardCount] = shardName;
			targetRoomIDs[targetShardCount][0] = RoomID;
			targetRoomCount[targetShardCount] = 1;
			targetShardCount ++;
		}
	}

	// Read Packet Finished
	uint16	fsSid = _tsid.get();
	for (int i = 0; i < targetShardCount; i ++)
	{
		if (targetShardNames[i] == "0")	// OwnShard
		{
			ReadFavorRoomInfo(FID, targetRoomCount[i], targetRoomIDs[i], 0, _tsid);
		}
		else
		{
			CMessage	msgOut(M_NT_REQ_FAVORROOM_INFO);
			msgOut.serial(fsSid, FID, targetRoomCount[i]);
			for (uint32 j = 0; j < targetRoomCount[i]; j ++)
			{
				msgOut.serial(targetRoomIDs[i][j]);
			}

			CUnifiedNetwork::getInstance()->send(targetShardNames[i], msgOut);
		}
	}
}

// Request FavorRoom's Info ([w:fsSid][d:FID][d:RoomCount][[d:RoomID]])
void	cb_M_NT_REQ_FAVORROOM_INFO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint16		fsSid;		_msgin.serial(fsSid);
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint32		RoomCount;	_msgin.serial(RoomCount);

	uint32	targetRoomIDs[MAX_FAVORROOMGROUP * MAX_FAVORROOMNUM];
	for (uint32 i = 0; i < RoomCount; i ++)
	{
		_msgin.serial(targetRoomIDs[i]);
	}

	ReadFavorRoomInfo(FID, RoomCount, targetRoomIDs, fsSid, _tsid);
}


void	cb_M_NT_AUTOPLAY_REGISTER(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID		RoomID;
	_msgin.serial(RoomID);

	uint8	ShardCode = (uint8)(RoomID >> 24);
	string	shardName;
	if (ShardCode == 0)
	{
		sipwarning("Invalid RoomID. RoomID=%d", RoomID);
		return;
	}
	else
	{
		shardName = GetShardNameFromShardCode(ShardCode);
		if (shardName == "")	// Unknown Shard
		{
			sipwarning("Invalid ShardName. RoomID=%d", RoomID);
			return;
		}
	}

	_msgin.invert();
	if (shardName == "0")	// Own Shard
	{
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, _msgin);
	}
	else
	{
		CUnifiedNetwork::getInstance()->send(shardName, _msgin);
	}
}


static void	DBCB_DBDeleteNonusedFreeRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	ucstring	empty = L"";
	uint8		ChitAddType = CHITADDTYPE_ADD_ONLY;
	uint8		ChitType = CHITTYPE_ROOM_DELETED;
	uint32		zero = 0;

	uint32 nRows, i;
	resSet->serial(nRows);
	for (i = 0; i < nRows; i ++)
	{
		uint32		RoomID;		resSet->serial(RoomID);
		ucstring	RoomName;	resSet->serial(RoomName);
		uint32		FID;		resSet->serial(FID);

		{
			CMessage	msgOut(M_NT_ADDCHIT);
			msgOut.serial(ChitAddType, FID, empty, FID, ChitType, RoomID, zero, RoomName, empty);
			CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgOut);
		}

		//sipinfo(L"Room Deleted. RoomID=%d, MasterFID=%d", RoomID, FID); byy krs
	}
}

void DeleteNonusedFreeRooms()
{
	MAKEQUERY_DeleteNonusedFreeRoom(strQ);
	DBCaller->executeWithParam(strQ, DBCB_DBDeleteNonusedFreeRoom,
		"");
}

////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////
////////////////////////////////////////////////////
SIPBASE_CATEGORISED_COMMAND(WS, sendTestPacketToWS, "Send test packet(28000) to WS", "ShardCode" )
{
	if(args.size() < 1)	return false;

	string	v0	= args[0];
	uint8		shardCode = (uint8)atoui(v0.c_str());

	std::map<uint8, string>::iterator it = g_mapShardCodeToServiceName.find(shardCode);
	if (it == g_mapShardCodeToServiceName.end())
	{
		sipinfo("Can't find WS. shardCode=%d", shardCode);
	}
	else if (it->second == "0")
	{
		sipinfo("It's me. shardCode=%d", shardCode);
	}
	else
	{
		CMessage	msgOut(M_TEST);
		CUnifiedNetwork::getInstance()->send(it->second, msgOut);

		sipinfo("Send message success. shardCode=%d, WSName=%s", shardCode, it->second.c_str());
	}
	return true;
}

static ucstring ConvertRoomID_DBID2ClientID(uint32 nRoomID)
{
	ucstring	strRoomClientID;

	tchar strQ[300] = _t("");
	if(nRoomID >= 0x1000000)
	{
		int nRoomCode = (int)(nRoomID / 0x1000000);
		SIPBASE::smprintf(strQ, 300, _t("%c%d"), _t('A') + nRoomCode - 1, nRoomID % 0x1000000);
	}
	else
	{
		SIPBASE::smprintf(strQ, 300, _t("%d"), nRoomID);
	}
	strRoomClientID = strQ;
	return strRoomClientID;
}

struct RefundRoomMoneyInfo
{
	T_FAMILYID		FID;
	T_ACCOUNTID		UserID;
	T_ROOMID		RoomID;
	T_SCENEID		SceneID;
	T_ROOMNAME		RoomName;
	uint32			UMoney;
};

struct GanEnYuanInfo
{
	T_ROOMID		RoomID;
	T_FAMILYID		FID;
	T_ROOMNAME		RoomName;
	uint8			FreeUse;
	T_DATETIME		TermTime;
};

SIPBASE_CATEGORISED_COMMAND(WS, adjustRoom2013, "", "" )
{
	// delete expired ganenyuans

	{
		std::map<T_ROOMID, T_FAMILYID>	delrooms;
		{
			CDBConnectionRest	DBConnection(DBCaller);
			tchar strQ[1000] = _t("");
			SIPBASE::smprintf(strQ, 1000, _t("SELECT RoomID, OwnerID FROM tbl_room WHERE RoomID > 6000 AND SceneID=6 AND freeuse=1 AND termtime < '2012-01-01'"));
			DB_EXE_QUERY(DBConnection, Stmt, strQ);
			if (nQueryResult == true)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, T_ROOMID,		roomid );
						COLUMN_DIGIT( row, 1, T_FAMILYID,	ownerid );
						delrooms[roomid] = ownerid;
					}
					else
						break;
				} while (true);
			}		
		}
		{
			std::map<T_ROOMID, T_FAMILYID>::iterator it;
			for (it = delrooms.begin(); it != delrooms.end(); it++)
			{
				T_ROOMID roomid = it->first;
				CDBConnectionRest	DBConnection(DBCaller);
				tchar strQ[300] = _t("");
				SIPBASE::smprintf(strQ, 300, _t("UPDATE tbl_room SET deleteflag=1, deletetime=getdate( ) WHERE RoomID=%d"), roomid);
				DB_EXE_QUERY(DBConnection, Stmt, strQ);
			}
		}
		{
			std::map<T_ROOMID, T_FAMILYID>::iterator it;
			for (it = delrooms.begin(); it != delrooms.end(); it++)
			{
				T_FAMILYID fid = it->second;
				CDBConnectionRest	DBConnection(DBCaller);
				tchar strQ[300] = _t("");
				SIPBASE::smprintf(strQ, 300, _t("UPDATE tbl_family SET roomcount=roomcount-1 WHERE FamilyID=%d AND roomcount > 0"), fid);
				DB_EXE_QUERY(DBConnection, Stmt, strQ);
			}
		}
	}

	{
		CDBConnectionRest	DBConnection(DBCaller);
		tchar strQ[300] = _t("");
		SIPBASE::smprintf(strQ, 300, _t("UPDATE tbl_room SET TermTime=NULL, Upgrade=0 WHERE RoomID > 6000 AND SceneID=6 AND deleteflag = 0"));
		DB_EXE_QUERY(DBConnection, Stmt, strQ);
	}
	{
		CDBConnectionRest	DBConnection(DBCaller);
		tchar strQ[300] = _t("");
		SIPBASE::smprintf(strQ, 300, _t("UPDATE tbl_family SET FreeUsed=0"));
		DB_EXE_QUERY(DBConnection, Stmt, strQ);
	}
	std::list<GanEnYuanInfo>	GanEnYuan;
	{
		CDBConnectionRest	DBConnection(DBCaller);
		tchar strQ[1000] = _t("");
		SIPBASE::smprintf(strQ, 1000, _t("SELECT RoomID, OwnerID, Name, FreeUse, Termtime FROM tbl_room WHERE RoomID > 6000 AND SceneID=6 AND deleteflag = 0"));
		DB_EXE_QUERY(DBConnection, Stmt, strQ);
		if (nQueryResult == true)
		{
			do
			{
				DB_QUERY_RESULT_FETCH( Stmt, row );
				if (IS_DB_ROW_VALID( row ))
				{
					COLUMN_DIGIT( row, 0, T_ROOMID,		roomid );
					COLUMN_DIGIT( row, 1, T_FAMILYID,	ownerid );
					COLUMN_WSTR(row, 2, rname, MAX_LEN_ROOM_NAME);
					COLUMN_DIGIT( row, 3, uint8,	fu );
					COLUMN_DATETIME(row, 4,				termtime);
					
					GanEnYuanInfo newinfo;
					newinfo.RoomID = roomid;
					newinfo.FID = ownerid;
					newinfo.RoomName = rname;
					newinfo.FreeUse = fu;
					newinfo.TermTime = termtime;
					GanEnYuan.push_back(newinfo);
				}
				else
					break;
			} while (true);
		}		
	}
	{
		std::list<GanEnYuanInfo>::iterator it;
		for (it = GanEnYuan.begin(); it != GanEnYuan.end(); it++)
		{
			GanEnYuanInfo& info = (*it);
			if (info.FreeUse != 1)
			{
				{
					CDBConnectionRest	DBConnection(DBCaller);
					tchar strQ[1000] = _t("");
					SIPBASE::smprintf(strQ, 1000, _t("UPDATE tbl_room SET SceneID=7 WHERE RoomID = %d"), info.RoomID);
					DB_EXE_QUERY(DBConnection, Stmt, strQ);
				}
				{
					CDBConnectionRest	DBConnection(DBCaller);
					tchar strQ[1000] = _t("");
					SIPBASE::smprintf(strQ, 1000, _t("DELETE FROM tbl_room_renew_history WHERE RoomID = %d"), info.RoomID);
					DB_EXE_QUERY(DBConnection, Stmt, strQ);
				}
				if (info.TermTime.length() > 0)
				{
					CDBConnectionRest	DBConnection(DBCaller);
					ucstring	Title = L"幸运奖励";
					ucstring	RoomIDText = ConvertRoomID_DBID2ClientID(info.RoomID);
					ucchar		contemp[150] = L"";
					SIPBASE::smprintf(contemp, 150, L"恭喜您的天园：%s（%s）被系统抽中为“幸运”天园，《慈恩天下》将您的感恩园升级为无限期感恩园。感谢您一直以来的支持与信任，祝您新春愉快！", info.RoomName.c_str(), RoomIDText.c_str());
					ucstring	Content = contemp;

					tchar strQ[1000] = _t("");
					SIPBASE::smprintf(strQ, 1000, _t("INSERT INTO tbl_Mail(FFID, RFID, Title, Content, Status, MType, ItemExist, ItemData) VALUES (%d, %d, N'%s', N'%s', 0, 5, 0, 0)"), info.FID, info.FID, Title.c_str(), Content.c_str());
					DB_EXE_QUERY(DBConnection, Stmt, strQ);
				}
			}
			else
			{
				CDBConnectionRest	DBConnection(DBCaller);
				tchar strQ[300] = _t("");
				SIPBASE::smprintf(strQ, 300, _t("UPDATE tbl_family SET FreeUsed=1 WHERE FamilyID=%d"), info.FID);
				DB_EXE_QUERY(DBConnection, Stmt, strQ);
			}
		}
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(WS, adjustRoom2013ForMail, "", "" )
{
	std::map<T_ROOMID, T_ROOMID>	renewRooms;
	{
		CDBConnectionRest	DBConnection(DBCaller);
		tchar strQ[1000] = _t("");
		SIPBASE::smprintf(strQ, 1000, _t("SELECT RoomID FROM tbl_room_renew_history WHERE SceneID=6"));
		DB_EXE_QUERY(DBConnection, Stmt, strQ);
		if (nQueryResult == true)
		{
			do
			{
				DB_QUERY_RESULT_FETCH( Stmt, row );
				if (IS_DB_ROW_VALID( row ))
				{
					COLUMN_DIGIT( row, 0, T_ROOMID,		roomid );
					renewRooms[roomid] = roomid;
					sipdebug("room_renew_history %d", roomid);
				}
				else
					break;
			} while (true);
		}		
	}

	std::list<GanEnYuanInfo>	GanEnYuan;
	{
		CDBConnectionRest	DBConnection(DBCaller);
		tchar strQ[1000] = _t("");
		SIPBASE::smprintf(strQ, 1000, _t("SELECT RoomID, OwnerID, Name FROM tbl_room WHERE RoomID > 6000 AND SceneID=7 AND deleteflag = 0"));
		DB_EXE_QUERY(DBConnection, Stmt, strQ);
		if (nQueryResult == true)
		{
			do
			{
				DB_QUERY_RESULT_FETCH( Stmt, row );
				if (IS_DB_ROW_VALID( row ))
				{
					COLUMN_DIGIT( row, 0, T_ROOMID,		roomid );
					COLUMN_DIGIT( row, 1, T_FAMILYID,	ownerid );
					COLUMN_WSTR(row, 2, rname, MAX_LEN_ROOM_NAME);
					
					sipdebug("tbl_room %d", roomid);
					std::map<T_ROOMID, T_ROOMID>::iterator itRenew = renewRooms.find(roomid);
					if (itRenew != renewRooms.end())
					{
						GanEnYuanInfo newinfo;
						newinfo.RoomID = roomid;
						newinfo.FID = ownerid;
						newinfo.RoomName = rname;
						newinfo.FreeUse = 0;
						newinfo.TermTime = "";
						GanEnYuan.push_back(newinfo);
						sipdebug("mail insert %d", roomid);
					}
				}
				else
					break;
			} while (true);
		}		
	}
	{
		std::list<GanEnYuanInfo>::iterator it;
		for (it = GanEnYuan.begin(); it != GanEnYuan.end(); it++)
		{
			GanEnYuanInfo& info = (*it);
			CDBConnectionRest	DBConnection(DBCaller);
			ucstring	Title = L"幸运奖励";
			ucstring	RoomIDText = ConvertRoomID_DBID2ClientID(info.RoomID);
			ucchar		contemp[150] = L"";
			SIPBASE::smprintf(contemp, 150, L"恭喜您的天园：%s（%s）被系统抽中为“幸运”天园，《慈恩天下》将您的感恩园升级为无限期感恩园。感谢您一直以来的支持与信任，祝您新春愉快！", info.RoomName.c_str(), RoomIDText.c_str());
			ucstring	Content = contemp;

			tchar strQ[1000] = _t("");
			SIPBASE::smprintf(strQ, 1000, _t("INSERT INTO tbl_Mail(FFID, RFID, Title, Content, Status, MType, ItemExist, ItemData) VALUES (%d, %d, N'%s', N'%s', 0, 5, 0, 0)"), info.FID, info.FID, Title.c_str(), Content.c_str());
			DB_EXE_QUERY(DBConnection, Stmt, strQ);
		}
	}

	return true;
}

