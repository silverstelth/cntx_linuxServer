#include "misc/types_sip.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <vector>
#include <map>

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#endif

#include "misc/debug.h"
#include "misc/config_file.h"
#include "misc/displayer.h"
#include "misc/log.h"
#include "misc/db_interface.h"

#include "net/service.h"
#include "net/login_cookie.h"

#include "login_service.h"

#include "../../common/err.h"
#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/DBData.h"
#include "../Common/Common.h"
#include "../Common/netCommon.h"
#include "../Common/QuerySetForMain.h"
#include "../Common/DBLog.h"

//
// Namespaces
//

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

#include "connection_ws.h"
#include "players.h"
#include "Mails.h"

//
// Variables
//
CVariable<uint8>	ShardState_Good("LS","ShardState_Good", "Get / Set ShardState Good percent.", 40, 0, true);
CVariable<uint8>	ShardState_Busy("LS","ShardState_Busy", "Get / Set ShardState Busy percent.", 60, 0, true);

CVariable<std::string>	SSMasterIPForClient("SS", "SSMasterIPForClient", "Host IP for client connection", "", 0, true);
CVariable<uint16>	SSMasterPortForClient("SS", "SSMasterPortForClient", "UDP Port of Delay SS for Clients", 50000, 0, true);

CVariable<std::string>	SSSlaveIPForClient("SS", "SSSlaveIPForClient", "Host IP for master connection", "", 0, true);
CVariable<uint16>	SSSlavePortForClient("SS", "SSSlavePortForClient", "TCP Port of SS for master", 50001, 0, true);

CVariable<std::string>	SSMasterIPForSlave("SS", "SSMasterIPForSlave", "Host IP of slave connection", "", 0, true);
CVariable<uint16>	SSServicePortForSS("SS", "SSServicePortForSS", "Service Port for all start_service", 50002, 0, true);

CVariable<std::string>	SSSlaveIPForPublic("SS", "SSSlaveIPForPublic", "Host IP for client connection", "", 0, true);
CVariable<uint16>	SSSlavePortForPublic("SS", "SSSlavePortForPublic", "TCP Port of SS for Clients", 0, 0, true);

static uint RecordNbPlayers = 0;
static uint NbPlayers = 0;

uint32	g_nDefaultShardID = 0;

class _WaitingLogoffUser
{
public:
	_WaitingLogoffUser(string &_strIP, CLoginCookie &_cookie, uint16 _wsSid, uint16 _fsSid)
		: m_StrIP(_strIP), m_Cookie(_cookie), m_WsSid(_wsSid), m_FsSid(_fsSid) {};
	string			m_StrIP;
	CLoginCookie	m_Cookie;
	uint16			m_WsSid;
	uint16			m_FsSid;
};
std::map<uint32, _WaitingLogoffUser>	g_mapWaitingLogoffUserList;	// <UID, _WaitingLogoffUser>
std::map<uint32, uint16>	g_mapCurrentUserWsSid;	// <UID, WsSid>

class _UserInfoForDB
{
public:
	uint32	m_nUID;
	uint32	m_nFID;
	uint32	m_nMainShardID;
	uint32	m_nCurShardID;

	_UserInfoForDB(uint32 uid, uint32 fid, uint32 mainShardID, uint32 curShardID)
	{
		m_nUID = uid;
		m_nFID = fid;
		m_nMainShardID = mainShardID;
		m_nCurShardID = curShardID;
	}
};
std::map<uint32, _UserInfoForDB>	g_mapUserInfoForDB;		// <UID, _UserInfoForDB>

class _MoveDataUserInfo
{
public:
	uint32	m_nTargetShardID;
	uint16	m_FsSid;
	uint32	m_Reason;	// 0 : Back to main shard when client disconnect
						// 1 : WaitEnterShard
	uint32	m_ReqTime;	// CTime::getSecondsSince1970()

	_MoveDataUserInfo(uint32 targetShardID, uint16 fsSid, uint32 reason)
	{
		m_nTargetShardID = targetShardID;
		m_FsSid = fsSid;
		m_Reason = reason;
		m_ReqTime = CTime::getSecondsSince1970();
	}
};
std::map<uint32, _MoveDataUserInfo>	g_mapMoveDataUsers;	// <UID, _MoveDataUserInfo> - Shardº¯°æ¿äÃ»ÀÌ µÈ »ç¿ëÀÚ¸ñ·Ï, Áö±Ý ÀÚ·áÀÌµ¿Áß

class _ChangeShardReq
{
public:
	_ChangeShardReq(uint16 _wsSid, uint16 _fsSid, uint32 _fid, uint8 _newShardCode) 
		: m_WsSid(_wsSid), m_FsSid(_fsSid), m_FID(_fid), m_NewShardCode(_newShardCode) {};

	uint16	m_WsSid;
	uint16	m_FsSid;
	uint32	m_FID;
	uint8	m_NewShardCode;
};
std::map<uint32, _ChangeShardReq>	g_ChangeShardReqList;	// <UID, _ChangeShardReq>

bool CheckInChangeShardReqList(uint32 err, CLoginCookie &cookie, string &shardAddr);
void SendEnterShardOk(uint32 UID, uint32 ShardID, uint16 fsSid);
bool MoveUserDataStartInShard(uint32 UID, uint32 fromShardID, uint32 toShardID);

//
// Functions
//

void refuseShard (uint16 sid, const char *format, ...)
{
	string reason;
	SIPBASE_CONVERT_VARGS (reason, format, SIPBASE::MaxCStringSize);
	sipwarning(reason.c_str ());
	CMessage msgout(M_SC_FAIL);
	msgout.serial (reason);
	CUnifiedNetwork::getInstance ()->send ((TServiceId)sid, msgout);
}

CShardInfo* findShardWithSId(uint16 sid)
{
	for (std::map<uint32, CShardInfo>::iterator it = g_ShardList.begin(); it != g_ShardList.end(); it ++)
	{
		if (it->second.m_wSId == sid)
		{
			return &(it->second);
		}
	}
	// shard not found
	return NULL;
}

//sint32 findShard (uint32 shardId)
//{
//	for (sint i = 0; i < (sint) g_Shards.size (); i++)
//	{
//		if (g_Shards[i].dbInfo.shardId == shardId)
//		{
//			return i;
//		}
//	}
//	// shard not found
//	return -1;
//}

//static bool	SetShardOpenState(CShardInfo *pShardInfo)
//{
//	float	percent		= (float)pShardInfo->m_nPlayers / (float)pShardInfo->m_nUserLimit;
//	uint8	uPercent	= (uint8)(percent + 0.5);
//	sipinfo("Shard(id %u) Current State Percent %u", pShardInfo->m_nShardId, uPercent);
//	pair<TSHARDSTATETBLIT, TSHARDSTATETBLIT> range = tblShardState.equal_range(pShardInfo->m_nShardId);
//	TSHARDSTATETBLIT it;
//	for (it = range.first; it != range.second; it ++)
//	{
//		_entryShardState info = (*it).second;
//		if (info.startPer <= uPercent && uPercent <= info.endPer)
//		{
//			pShardInfo->m_bStateId	= info.stateId;
//			pShardInfo->m_bUserTypeId	= info.userTypeId;
//			break;
//		}
//	}
//
//	if (it == range.second)
//		return false;
//
//	return true;
//}

extern void SendSiteActivityToShard(TServiceId tsid);
extern void SendBeginnerMstItemToShard(TServiceId tsid);
extern void SendCheckInToShard(TServiceId tsid);
static void cbWSConnection (const std::string &serviceName, TServiceId tsid, void *arg)
{
	SendSiteActivityToShard(tsid);
	SendBeginnerMstItemToShard(tsid);
	SendCheckInToShard(tsid);
	return;
	// Delete Function : Check WS Address
}


static void cbWSDisconnection (const std::string &serviceName, TServiceId tsid, void *arg)
{
	uint16 sid = tsid.get();

	TTcpSockId from;
	CCallbackNetBase *cnb = CUnifiedNetwork::getInstance ()->getNetBase (tsid, from);
	const CInetAddress &ia = cnb->hostAddress (from);

	sipdebug("shard disconnection: %s", ia.asString ().c_str ());

	string	strDeadServiceName = "";
	for (std::map<uint32, CShardInfo>::iterator it = g_ShardList.begin(); it != g_ShardList.end(); it ++)
	{
		CShardInfo	&shardInfo = it->second;
		if (shardInfo.m_wSId != sid)
			continue;

		// shard disconnected
		sipinfo("ShardId %d with IP '%s' is offline!", shardInfo.m_nShardId, ia.asString ().c_str());
		sipinfo("*** ShardId %3d NbPlayers %3d -> %3d", shardInfo.m_nShardId, shardInfo.m_nPlayers, 0);

		strDeadServiceName = shardInfo.m_sServiceName;
		
		// KSR INSERT
		MAKEQUERY_DISCONNECTSHARD(strQuery, shardInfo.m_nShardId);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (!nQueryResult)
			sipdebug("Query Failed! : %S.", strQuery);

		NbPlayers -= shardInfo.m_nPlayers;

		shardInfo.init();

		break;
	}

	if (strDeadServiceName.empty())
	{
		sipwarning("Shard %s goes offline but wasn't already online!", ia.asString ().c_str ());
	}
	else
	{
		// Notice to other shards
		CMessage	msgOut(M_LW_OTHER_SHARD_OFF);
		msgOut.serial(strDeadServiceName);

		for (std::map<uint32, CShardInfo>::iterator it = g_ShardList.begin(); it != g_ShardList.end(); it ++)
		{
			CShardInfo	&shardInfo = it->second;
			if (shardInfo.m_wSId == 0)
				continue;

			CUnifiedNetwork::getInstance()->send(TServiceId(shardInfo.m_wSId), msgOut);
		}
	}
}

//static void		SendToWSStateDefines(CShard & shard, TServiceId tsid)
//{
//	uint16			sid = tsid.get();
//	TTcpSockId		from;
//	CCallbackNetBase *cnb = CUnifiedNetwork::getInstance ()->getNetBase (tsid, from);
//	const CInetAddress &ia = cnb->hostAddress (from);
//
//	_entryShard		&info = shard.dbInfo;
//
//	// Shard OpenState Defines
//	CMessage	msgout(M_LS_STATEDEF);
//	uint32		nCount = tblShardState.count(info.shardId);
//	msgout.serial(nCount);
//	pair<TSHARDSTATETBLIT, TSHARDSTATETBLIT> range = tblShardState.equal_range(info.shardId);
//	for ( TSHARDSTATETBLIT it = range.first; it != range.second; it ++ )
//	{
//		_entryShardState info = (*it).second;
//		msgout.serial(info.startPer);
//		msgout.serial(info.endPer);
//		msgout.serial(info.stateId);
//		msgout.serial(info.userTypeId);
//	}
//
//	CUnifiedNetwork::getInstance()->send(tsid, msgout);
//	sipinfo("Send to WS, StateDefine Info, count:%u", nCount);
//}

//static bool		AcceptNewShard(CShard &shard, TServiceId tsid)
//{
//    uint16			sid = tsid.get();
//	TTcpSockId		from;
//	CCallbackNetBase *cnb = CUnifiedNetwork::getInstance ()->getNetBase (tsid, from);
//	const CInetAddress &ia = cnb->hostAddress (from);
//
//	_entryShard		&info = shard.dbInfo;
//	if ( AcceptExternalShards.get() != 1 )
//	{
//		// can't accept new shard
//		refuseShard (sid, "Bad shard identification, The shard %d is not in the database and can't be added", shard.dbInfo.shardId);
//		return false;
//	}
//	// we accept new shard, add it
//
//	MAKEQUERY_ACCEPTNEWSHARD(strQuery, info.shardId, info.strName.c_str(), info.zoneId, info.nUserLimit, info.ucContent.c_str());
//	CDBConnectionRest	DB(MainDB);
//	DB_EXE_QUERY(DB, Stmt, strQuery);
//	if ( !nQueryResult )
//	{
//		sipdebug("Query Failed! : %S.", strQuery);
//		refuseShard (sid, "Query (%S) failed", strQuery);
//		return false;
//	}
//
//	sipinfo("The ShardId %d with ip '%s' was inserted in the database and is online!", shard.dbInfo.shardId, ia.ipAddress ().c_str ());
//	g_Shards.push_back (CShard (shard.dbInfo, sid));
//	g_tblShardList[info.shardId] = info; 
//
////	SendToWSStateDefines(shard, tsid);
//
//	return	true;
//}


//static void		AcceptExternalShard(CShard &shard, TServiceId tsid)
//{
//	if ( !AcceptNewShard(shard, tsid) )
//		return;
//
//    CMessage	msgout(M_LS_RESHARD);
//	CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgout); 
//}

// packet : M_WL_WS_IDENT
// WS -> LS
// Two Case : 
//		1: The shard is already registered his info.
//		2: The shard is newly added this login system.
static void cbWSIdentification (CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16			sid = tsid.get();

	uint32		shardId;
	ucstring	shardName;
	uint8		zoneId;
	uint32		nUserLimit;
	ucstring	ucContent;
	uint8		activeFlag;
	uint32		nbPlayers;
	uint16		SId;
	string		WsAddr;

	msgin.serial(shardId, shardName, zoneId, nUserLimit, ucContent, activeFlag, nbPlayers, SId, WsAddr);

	sipdebug("shard identification, It says to be ShardId %d, let's check that!", shardId);

	std::map<uint32, CShardInfo>::iterator it = g_ShardList.find(shardId);
	if (it == g_ShardList.end())
	{
		refuseShard (sid, "Unregistered shard id. shardid=%d", shardId);
		return;
	}
	if (it->second.m_wSId != 0)
	{
		refuseShard (sid, "Already same shard id : %u is running", shardId);
		return;
	}
	CShardInfo	&shardInfo = it->second;

	//uint32		nCount = tblShardState.count(shardId);
	//if (nCount == 0)
	//{
	//	uint8	stateId = SHARDSTATE_GOOD;
	//	uint8	usertypeId = USERTYPE_COMMON;
	//	uint8	startPer = 0;
	//	uint8	endPer = 100;

	//	_entryShardState	info(shardId, stateId, usertypeId, startPer, endPer);
	//	tblShardState.insert(std::make_pair(shardId, info));
	//}

	{
		MAKEQUERY_SHARDON(strQuery, shardId);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (!nQueryResult)
		{
			refuseShard (sid, "DBError : Query (%S) failed", strQuery);
			return;
		}
	}

	// modify the shard list
	shardInfo.init();
	shardInfo.m_bActiveFlag = SHARD_ON;
	shardInfo.m_wSId = sid;
	shardInfo.SetActiveShardIP(WsAddr, serviceName);

	//SetShardOpenState(&shardInfo);

	CMessage	msgout(M_LW_SHARDINFO);
	msgout.serial(shardInfo.m_nShardId);
	msgout.serial(shardInfo.m_uShardName);
	msgout.serial(shardInfo.m_bZoneId);
	msgout.serial(shardInfo.m_nUserLimit);
	msgout.serial(shardInfo.m_uContent);
	msgout.serial(shardInfo.m_bActiveFlag);
	msgout.serial(shardInfo.m_nPlayers);
	msgout.serial(shardInfo.m_bMainCode);
	msgout.serial(shardInfo.m_uShardCodeList);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);

	uint8	bPrevOnForMe = 1, bPrevOnForOther = 0;
	for (std::map<uint32, CShardInfo>::iterator it = g_ShardList.begin(); it != g_ShardList.end(); it ++)
	{
		if (it->second.m_wSId == sid)
		{
			bPrevOnForMe = 0;
			bPrevOnForOther = 1;
			continue;
		}
		if (it->second.m_wSId == 0)
			continue;

		CMessage	msgOut1(M_LW_OTHER_SHARD);
		msgOut1.serial(shardInfo.m_sServiceName, shardInfo.m_sActiveShardIP, shardInfo.m_uShardCodeList, bPrevOnForOther);
		CUnifiedNetwork::getInstance()->send(TServiceId(it->second.m_wSId), msgOut1);

		CMessage	msgOut2(M_LW_OTHER_SHARD);
		msgOut2.serial(it->second.m_sServiceName, it->second.m_sActiveShardIP, it->second.m_uShardCodeList, bPrevOnForMe);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut2);
	}

	// ok, the shard is identified correctly
	sipinfo("ShardId %d with ip '%s' is online!", shardId, WsAddr.c_str ());
}

static T_ERR doConnectUserToShard(uint32 userid, uint16 sid, std::string strIP, uint8 bIsGM, uint32 nGMoney)
{
	CShardInfo		*pShardInfo	= findShardWithSId(sid);
	if (pShardInfo == NULL)
	{
		sipwarning ("user connected shard isn't in the shard list");
		return ERR_IMPROPERSHARD;
	}

	uint32			shardId	= pShardInfo->m_nShardId;

	if (bIsGM)
	{
		// new client on the shard
		MAKEQUERY_LOGINUSERTOSHARD_GM(strQuery, userid, shardId, strIP, nGMoney);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY_WITHONEROW(DB, Stmt, strQuery);
		if (!nQueryResult)
		{
			sipwarning("Query (%S) failed", strQuery);
			return E_DBERR;
		}

		T_ERR	uResult;
		COLUMN_DIGIT(row, 0, uint32, ret)
		switch (ret)
		{
			case 0:
				uResult = ERR_NOERR;
				//RESULT_SHOW(strQuery, ERR_NOERR); byy krs
				break;
			case 1:	// never happen, because revise logoff old connection before new login
				uResult = ERR_ALREADYONLINE;	// already online same shard
				ERR_SHOW(strQuery, ERR_ALREADYONLINE);
				break;
			case 2:
				uResult = ERR_IMPROPERSHARD;	// already online other shard, can't enter two shards at once
				ERR_SHOW(strQuery, ERR_IMPROPERSHARD);
				break;
			case -1:
				uResult = E_DBERR;
				ERR_SHOW(strQuery, E_DBERR);
				break;
			default:
				sipwarning(L"Unknown Error Query <%s>: %u", strQuery, ret);
				break;
		}

		if (uResult != ERR_NOERR)
			return	uResult;

		//sipinfo("*** ShardId %3d NbPlayers %3d -> %3d", shardId, pShardInfo->m_nPlayers, pShardInfo->m_nPlayers + 1); byy krs
		//sipdebug ("Id %d is connected on the shard", userid);
		//Output->displayNL ("###: %3d User connected to the shard (%d), State:%u", userid, shardId, pShardInfo->m_bStateId);
		
		pShardInfo->m_nPlayers ++;
		//SetShardOpenState(pShardInfo);

		NbPlayers ++;
		if (NbPlayers > RecordNbPlayers)
		{
			RecordNbPlayers = NbPlayers;
//			beep(2000, 1, 100, 0);
			sipinfo("New player number record!!! %d players online on all shards", RecordNbPlayers);
		}

		return uResult;
	}
	else
	{
		// new client on the shard
		MAKEQUERY_LOGINUSERTOSHARD(strQuery, userid, shardId, strIP, nGMoney);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY_WITHONEROW(DB, Stmt, strQuery);
		if (!nQueryResult)
		{
			sipwarning ("Query (%S) failed", strQuery);
			return E_DBERR;
		}

		T_ERR	uResult;
		COLUMN_DIGIT(row, 0, uint32, ret)
		switch(ret)
		{
			case 0:
				uResult = ERR_NOERR;
				//RESULT_SHOW(strQuery, ERR_NOERR); byy krs
				break;
			case 1:	// never happen, because revise logoff old connection before new login
				uResult = ERR_ALREADYONLINE;	// already online same shard
				ERR_SHOW(strQuery, ERR_ALREADYONLINE);
				break;
			case 2:
				uResult = ERR_IMPROPERSHARD;	// already online other shard, can't enter two shards at once
				ERR_SHOW(strQuery, ERR_IMPROPERSHARD);
				break;
			case -1:
				uResult = E_DBERR;
				ERR_SHOW(strQuery, E_DBERR);
				break;
			default:
				sipwarning(L"Unknown Error Query <%s>: %u", strQuery, ret);
				break;
		}

		if (uResult != ERR_NOERR)
			return	uResult;

		//sipinfo("*** ShardId %3d NbPlayers %3d -> %3d", shardId, pShardInfo->m_nPlayers, pShardInfo->m_nPlayers + 1); // byy krs
		//sipdebug("Id %d is connected on the shard", userid);
		//Output->displayNL("###: %3d User connected to the shard (%d), State:%u", userid, shardId, pShardInfo->m_bStateId);

		pShardInfo->m_nPlayers ++;
		//SetShardOpenState(pShardInfo);

		NbPlayers ++;
		if (NbPlayers > RecordNbPlayers)
		{
			RecordNbPlayers = NbPlayers;
//			beep (2000, 1, 100, 0);
			sipinfo("New player number record!!! %d players online on all shards", RecordNbPlayers);
		}

		return uResult;
	}
}

static T_ERR	LogoffUser(uint32 userid, uint32 shardId, uint32 VisitDays, uint32 UserExp, uint32 UserVirtue, uint32 UserFame, uint32 nRooms, uint8 ExpRoom, uint8 bIsGM, uint32 nGMoney, bool bRightValue)
{
	if ( bIsGM )
	{
		MAKEQUERY_LOGOFFUSERTOSHARD_GM(strQuery, userid, shardId, nRooms, nGMoney, bRightValue);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY_WITHONEROW(DB, Stmt, strQuery);
		if ( !nQueryResult )
		{
			sipdebug("Main_user_logoffUSERtoShard_GM DB_EXE_QUERY_WITHONEROW failed. UserID=%d", userid);
			return E_DBERR;
		}

		T_ERR	uResult;
		COLUMN_DIGIT(row, 0, uint32, ret)
		switch( ret )
		{
			case 0:
				uResult = ERR_NOERR;
				//RESULT_SHOW(strQuery, ERR_NOERR); byy krs
				break;
			case 1:
				{
					uResult = ERR_ALREADYOFFLINE;	// already offline same shard
					ERR_SHOW(strQuery, ERR_ALREADYOFFLINE);
					// ºñ¹ýÆÄÄÉÆ®¹ß°ß
					// ucstring ucUnlawContent = SIPBASE::_toString(L"User(id:%u) Already Offline from shard %u", userid, shardId).c_str();
					// RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, userid);
					break;
				}
			case 2:
				{
					uResult = ERR_IMPROPERSHARD;	// already online other shard, can't enter two shards at once
					ERR_SHOW(strQuery, ERR_IMPROPERSHARD);
					// ºñ¹ýÆÄÄÉÆ®¹ß°ß
					// ucstring ucUnlawContent = SIPBASE::_toString(L"User(id:%u), Logoff shard(%u) differs from Login shard", userid, shardId).c_str();
					// RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, userid);
					break;
				}
			case -1:
				uResult = E_DBERR;
				ERR_SHOW(strQuery, E_DBERR);
				break;
			default:
				sipwarning(L"Unknown Error Query <%s>: %u", strQuery, ret);
				break;
		}

		return	uResult;
	}
	else
	{
		MAKEQUERY_LOGOFFUSERTOSHARD(strQuery, userid, shardId, nRooms, nGMoney, bRightValue);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY_WITHONEROW(DB, Stmt, strQuery);
		if ( !nQueryResult )
		{
			sipdebug("Main_user_logoffUSERtoShard DB_EXE_QUERY_WITHONEROW failed. UserID=%d", userid);
			return E_DBERR;
		}

		T_ERR	uResult;
		COLUMN_DIGIT(row, 0, uint32, ret)
		switch( ret )
		{
			case 0:
				uResult = ERR_NOERR;
				//RESULT_SHOW(strQuery, ERR_NOERR); byy krs
				break;
			case 1:
				{
					uResult = ERR_ALREADYOFFLINE;	// already offline same shard
//					ERR_SHOW(strQuery, ERR_ALREADYOFFLINE);
					sipwarning("ERROR : ERR_ALREADYOFFLINE(err %u) [WRN_4]", uResult);
					// ºñ¹ýÆÄÄÉÆ®¹ß°ß
					// ucstring ucUnlawContent = SIPBASE::_toString(L"User(id:%u) Already Offline from shard %u", userid, shardId).c_str();
					// RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, userid);
					break;
				}
			case 2:
				{
					uResult = ERR_IMPROPERSHARD;	// already online other shard, can't enter two shards at once
					ERR_SHOW(strQuery, ERR_IMPROPERSHARD);
					// ºñ¹ýÆÄÄÉÆ®¹ß°ß
					// ucstring ucUnlawContent = SIPBASE::_toString(L"User(id:%u), Logoff shard(%u) differs from Login shard", userid, shardId).c_str();
					// RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, userid);
					break;
				}
			case -1:
				uResult = E_DBERR;
				ERR_SHOW(strQuery, E_DBERR);
				break;
			default:
				sipwarning(L"Unknown Error Query <%s>: %u", strQuery, ret);
				break;
		}

		return	uResult;
	}
}

static T_ERR doDisconnectUserToShard(uint32 userid, uint16 sid, uint32 VisitDays, 
									 uint32 UserExp, uint32 UserVirtue, uint32 UserFame, uint32 nRooms, uint8 ExpRoom, uint8 bIsGM, uint32 nGMoney, bool bRightValue)
{
	CShardInfo *pShardInfo = findShardWithSId(sid);

	// client removed from the shard (true is for potential other client with the same id that wait for a connection)
	//		disconnectClient (Users[pos], true, false);

	if (pShardInfo == NULL)
	{
		sipwarning ("user disconnected tblShard isn't in the shard list");
		return	ERR_IMPROPERSHARD;
	}

	T_ERR uResult = LogoffUser(userid, pShardInfo->m_nShardId, VisitDays, UserExp, UserVirtue, UserFame, nRooms, ExpRoom, bIsGM, nGMoney, bRightValue);
	if (uResult != ERR_NOERR)		
		sipwarning("Abnormal Logoff UserId:%u, ShardId:%u, VistDays:%u, Exp:%u, Virtue:%u, Fame:%u, nRooms:%u, ExpRoom:%u", userid, pShardInfo->m_nShardId, VisitDays, UserExp, UserVirtue, UserFame, nRooms, ExpRoom);

	if (uResult == ERR_ALREADYOFFLINE || uResult == ERR_IMPROPERSHARD)
		return	uResult;

	if (pShardInfo->m_nPlayers == 0)
	{
		sipwarning("STOP : conflict between login and logoff, nbPlayer<0");
		return	uResult;
	}

	pShardInfo->m_nPlayers --;
	//SetShardOpenState(pShardInfo);
	//sipinfo("*** ShardId %3d NbPlayers %3d -> %3d", pShardInfo->m_nShardId, pShardInfo->m_nPlayers + 1, pShardInfo->m_nPlayers); // byy krs

	//sipdebug ("Id %d is disconnected from the shard", userid); // byy krs
	//Output->displayNL ("###: %3d User disconnected from the shard (%d), State : %u", userid, pShardInfo->m_nShardId, pShardInfo->m_bStateId);

	NbPlayers --;

	return uResult;
}

// packet : M_LS_REPORT_FS_STATE
// WS -> LS
//static void	cbWSReportFSState(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
//{
//	uint16	sid = tsid.get();
//	sint	shardPos = findShardWithSId (sid);
//	if (shardPos == -1)
//	{
//		sipwarning ("unknown WS %d reported state of a fs", sid);
//		return;
//	}
//	
//	TServiceId	FSSId;		msgin.serial(FSSId);
//	bool		alive;		msgin.serial(alive);
//	bool		patching;	msgin.serial(patching);
//	std::string	patchURI;	msgin.serial(patchURI);
//
//	CShard&	shard = g_Shards[shardPos];
//	if (!alive)
//	{
//		sipinfo("Shard %d frontend %d reported as offline", shard.dbInfo.shardId, FSSId.get());
//		std::vector<CFrontEnd>::iterator	itfs;
//		for (itfs=shard.FrontEnds.begin(); itfs!=shard.FrontEnds.end(); ++itfs)
//		{
//			if (TServiceId((*itfs).SId) == FSSId)
//			{
//				shard.FrontEnds.erase(itfs);
//				break;
//			}
//		}
//	}
//	else
//	{
//		CFrontEnd*	updateFS = NULL;
//		std::vector<CFrontEnd>::iterator	itfs;
//		for (itfs=shard.FrontEnds.begin(); itfs!=shard.FrontEnds.end(); ++itfs)
//		{
//			if (TServiceId((*itfs).SId) == FSSId)
//			{
//				// update FS state
//				CFrontEnd&	fs = (*itfs);
//				fs.Patching = patching;
//				fs.PatchURI = patchURI;
//
//				updateFS = &fs;
//				break;
//			}
//		}
//
//		if (itfs == shard.FrontEnds.end())
//		{
//			sipinfo("Shard %d frontend %d reported as online", shard.dbInfo.shardId, FSSId.get());
//			// unknown fs, create new entry
//			shard.FrontEnds.push_back(CFrontEnd(FSSId.get(), patching, patchURI));
//
//			updateFS = &(shard.FrontEnds.back());
//		}
//
//		if (updateFS != NULL)
//		{
//			sipinfo("Shard %d frontend %d status updated: patching=%s patchURI=%s", shard.dbInfo.shardId, FSSId.get(), (updateFS->Patching ? "yes" : "no"), updateFS->PatchURI.c_str());
//		}
//	}
//
//	// update DynPatchURLS in database
//	/*
//	std::string	dynPatchURL;
//	uint	i;
//	for (i=0; i<shard.FrontEnds.size(); ++i)
//	{
//		if (shard.FrontEnds[i].Patching)
//		{
//			if (!dynPatchURL.empty())
//				dynPatchURL += ' ';
//			dynPatchURL += shard.FrontEnds[i].PatchURI;
//		}
//	}
//
//	string query = "UPDATE tblShard SET DynPatchURL='"+dynPatchURL+"' WHERE ShardId='"+toStringA(shard.ShardId)+"'";
//	sint ret = sqlQueryWithoutRes(query.c_str ());
//	if (ret != 0)
//	{
//		sipwarning ("mysql_query (%s) failed:", query.c_str ());
//		return;
//	}
//	*/
//}
static void	cbWSReportFSState(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	sipinfo(L"Do nothing.....");
}

// Packet : M_LS_SET_SHARD_OPEN
// WS -> LS
static void	cbWSSetShardOpen(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();
	CShardInfo	*pShardInfo = findShardWithSId(sid);
	if (pShardInfo == NULL)
	{
		sipwarning ("unknown WS %d reported shard open state", sid);
		return;
	}

	uint8	shardOpenState;		msgin.serial(shardOpenState);
//	uint8	userType;			msgin.serial(userType);

	pShardInfo->m_bStateId	= shardOpenState;
//	pShardInfo->m_bUserTypeId	= userType;

	sipdebug("Set Shard OpenState From WS : ShardID %u, STATE %u", pShardInfo->m_nShardId, shardOpenState);
}

// Packet : M_LS_OL_ST
// WS -> LS
static void cbWSShardOnlineState(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	bool	newState;	msgin.serial(newState);
	sipinfo("OL_ST : ws state is %u", newState);
}

//
// Callbacks
//

// Packet : M_ENTER_SHARD_ANSWER
// WS -> LS
static void cbWSShardChooseShard(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();
	//
	// S10: receive "SCS" message from WS
	//

	uint16	wErrNo;
	CLoginCookie cookie;
	string	ShardAddr;

	msgin.serial (wErrNo, cookie);
	if ( wErrNo == ERR_NOERR )
	{
		msgin.serial (ShardAddr);
	}

	//TTcpSockId UserAddr = reinterpret_cast<TSockId> (cookie.getUserAddr());

	// search the cookie
	map<uint32, CShardCookie>::iterator it = TempCookies.find (cookie.getUserId());

	if (it == TempCookies.end ())
	{
		if (CheckInChangeShardReqList(wErrNo, cookie, ShardAddr))
			return;

		// not found in TempCookies, can't do anything
		sipwarning ("Receive an answer from welcome service but no connection waiting UserAddr : %u", cookie.getUserAddr ());
		return;
	}

	CMessage	msgout(M_ENTER_SHARD_ANSWER);
	msgout.serial (wErrNo, cookie, ShardAddr);

	//sipdebug("Receive login answer from welcome service, result=%d, cookie=%s", wErrNo, cookie.toString().c_str()); byy krs
	CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgout);
}

// Packet : RPC
// WS -> LS
void cbWSRemovedPendingCookie(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	CLoginCookie	cookie;
	msgin.serial(cookie);
	map<uint32, CShardCookie>::iterator it = TempCookies.find (cookie.getUserId());

	if (it == TempCookies.end ())
	{
		// not found in TempCookies, can't do anything
		sipwarning ("can't Remove Pending Cookie (RPC) : reason there is no cookie : %s", cookie.toString().c_str());
		return;
	}
	TempCookies.erase(it);
}

// packet : M_LS_RELOGOFF
// WS -> LS
void cbWSReviseLogoff(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	userid;			msgin.serial(userid);
	uint8	bIsGM;			msgin.serial(bIsGM);
	uint32	VisitDays;		msgin.serial(VisitDays);
	uint16	UserExp;		msgin.serial(UserExp);
	uint16	UserVirtue;		msgin.serial(UserVirtue);
	uint16	UserFame;		msgin.serial(UserFame);
	uint32	nRooms;			msgin.serial(nRooms);
	uint8	ExpRoom;		msgin.serial(ExpRoom);
	uint32	nGMoney;		msgin.serial(nGMoney);
	bool	bRightValue;	msgin.serial(bRightValue);

	T_ERR uResult;
	CShardInfo	*pShardInfo = findShardWithSId(tsid.get());
	if (pShardInfo == NULL)
	{
		sipwarning ("user disconnected, but tblShard isn't in the shard list");
		SendFailMessageToServer(ERR_IMPROPERSHARD, SS_S_NAME, userid);
	}
	else
	{
		uResult = LogoffUser(userid, pShardInfo->m_nShardId, VisitDays, UserExp, UserVirtue, UserFame, nRooms, ExpRoom, bIsGM, nGMoney, bRightValue);
		if (uResult != ERR_NOERR && uResult != ERR_ALREADYOFFLINE)
		{
//			sipwarning("Revise Logoff Failed err:%u", uResult);	// No need because already log in function [LogoffUser]
			SendFailMessageToServer(uResult, SS_S_NAME, userid);
		}
	}

	map<uint32, uint32>::iterator	it = AbnormalLogoffUsers.find(userid);
	if (it == AbnormalLogoffUsers.end())
	{
		sipwarning("Conflict : There is no Temporary AbnormalLogoff User UID=%d", userid);
		return;
	}
    AbnormalLogoffUsers.erase(userid);

	if (pShardInfo == NULL || (uResult != ERR_NOERR && uResult != ERR_ALREADYOFFLINE))
		return;

	// Send SHARDS To SS
	SendShardListToSS(userid, FindFIDfromUID(userid));
}

//static void	DBCB_CanUseMoney(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	uint32		msgid			= *(uint32 *)argv[0];
//	uint8		flag			= *(uint8 *)argv[1];
//	uint32		moneyExpend		= *(uint32 *)argv[2];
//	uint32		userid			= *(uint32 *)argv[3];
//	TServiceId	tsid(*(uint16 *)argv[4]);
//
//	if ( !nQueryResult )
//		return;
//
//	uint32 nRows;	resSet->serial(nRows);
//	if (nRows < 1)
//		return;
//
//	uint32 ret;		resSet->serial(ret);
//	uint32 money;	resSet->serial(money);
//
//	uint32	uResult = E_DBERR;
//	switch(ret)
//	{
//	case 0:
//		uResult = ERR_NOERR;
//		break;
//	case 1:
//		{
//			uResult = E_NO_EXIST;
//			return;		
//		}
//	case 1009:
//		{
//			uResult = E_NOMONEY;
//			return;
//		}
//	default:
//		return;
//	}
//
//	// send the check result to WS
//	CMessage	msgout(M_LS_CANUSEMONEYR);
//	msgout.serial(msgid);
//	msgout.serial(uResult);
//	msgout.serial(userid);
//	msgout.serial(flag);
//	msgout.serial(moneyExpend);
//	msgout.serial(money);
//
//	CUnifiedNetwork::getInstance()->send(tsid, msgout);
//}
//// callback function for the "CANUSEMONEY" from WS
//void cbWSCanUseMoney(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
//{
//	uint32	msgid;			msgin.serial(msgid);
//	uint8	flag;			msgin.serial(flag);
//	uint32	moneyExpend;	msgin.serial(moneyExpend);
//	uint32	userid;			msgin.serial(userid);
//
//	// Check there is enough money or not
//	MAKEQUERY_CANUSEMONEY(strQuery, flag, moneyExpend, userid);
//	MainDB->executeWithParam(strQuery, DBCB_CanUseMoney,
//		"DBDDW",
//		CB_,		msgid,
//		CB_,		flag,
//		CB_,		moneyExpend,
//		CB_,		userid,
//		CB_,		tsid.get());
///*
//	DB_EXE_QUERY_WITHONEROW(MainDBConnection, Stmt, strQuery)
//	if ( !nQueryResult )
//	{
//		sipwarning("Query Failed! : %S", strQuery);
//		return;
//	}
//	COLUMN_DIGIT(row, 0, uint32, ret);
//	COLUMN_DIGIT(row, 1, uint32, money);
//	uint32	uResult = ERR_NOERR;
//	switch(ret)
//	{
//	case 0:
//		uResult = ERR_NOERR;
//		RESULT_SHOW(strQuery, ERR_NOERR);
//		break;
//	case 1:
//		{
//			uResult = E_NO_EXIST;
//			ERR_SHOW(strQuery, E_NO_EXIST);
//			return;		
//		}
//	case 1009:
//		{
//			uResult = E_NOMONEY;
//			ERR_SHOW(strQuery, E_NOMONEY);
//			return;
//		}
//	default:
//		sipwarning(L"Unknown Error Query <%s>: %u", strQuery, ret);
//		break;
//   	}
//
//
//	// send the check result to WS
//	CMessage	msgout(M_LS_CANUSEMONEYR);
//	msgout.serial(msgid);
//	msgout.serial(uResult);
//	msgout.serial(userid);
//	msgout.serial(flag);
//	msgout.serial(moneyExpend);
//	msgout.serial(money);
//
//	CUnifiedNetwork::getInstance()->send(tsid, msgout);
//*/
//}

/*
bool DBOperator::UpdateGold(const char* AccoutID, int& gold, int& RetCode)
{
	bool ret =false;

	CDBHandleMgr DBHandle(this);

	try
	{	
		Unsigned __int64 gold1 = gold;
		Unsigned __int64 gold2 = gold;

		if( !GoldEncodeByAccount( AccoutID, gold1, gold2 ) )
		{
			RetCode = QUERYREQUEST_ERROR;
			return false;
		}

		char strGold[21];
		for( long i = 0; i < 20; i++ )
		{
			strGold[i] = '0';
		}
		strGold[20] = 0;
		char tempStr[11];
		long strlenth;
		_i64toa( gold1, tempStr, 16 );
		strlenth = strlen( tempStr );
		memcpy( &strGold[10-strlenth], tempStr, strlenth );
		_i64toa( gold2, tempStr, 16 );
		strlenth = strlen( tempStr );
		memcpy( &strGold[20-strlenth], tempStr, strlenth );		// Éú³É´æ´¢µ½Êý¾Ý¿âÖÐ×Ö·û´®strGold

		m_dbGameServer.Cmd("EXECUTE UpdateGoldForAccount '%s','%s'",AccoutID,strGold);
		//m_dbGameServer.Cmd("update AccountGold set Gold=%d where AccountID=%s",gold,AccoutID);
		while(m_dbGameServer.More())
		{
			int Code = m_dbGameServer.GetLong(0);
			switch(Code){
			case -1:{
				ret = false;
				RetCode = LOGINREQUEST_ACCOUNTNOEXIST;
					}
					break;
			case -2:{
				ret = false;
				RetCode = QUERYREQUEST_ERROR;
					}
					break;
			case 0:{
				ret = true;
				RetCode = COMMON_NOERROR;
				   }
				   break;
			}
		}

		if (ret == false) 
		{
			stringstream LogStream;
			//LogStream << "´ú±ÒÊý¾ÝÐ´Èë³ö´í, ´íÎóAccoundID£º" << AccoutID << ends;
			//g_Log.WriteError(LogStream);
			return false;
		}

	}
	catch(CSQLException & e)
	{
		RetCode = QUERYREQUEST_ERROR;
		stringstream LogStream;
		LogStream << "Error Message(UpdateGold):"<< e.w_msgtext << ends;
		g_Log.WriteError(LogStream);
	}
	catch(...)
	{
		RetCode = QUERYREQUEST_ERROR;
		g_Log.WriteError("Error Message(UpdateGold): Unknown exception.");
	}

	return ret;
}

bool GetGoldForAccount(const char* AccountID,int& gold,int& RetCode)
{
	bool ret = true;
	gold = 0;
	CDBHandleMgr DBHandle(this);

	try
	{
		int len = 0;

		m_dbGameServer.Cmd("EXECUTE QueryGoldForAccount '%s'", AccountID);

		const char* strGold=0;
		if(m_dbGameServer.More())
		{
			strGold = m_dbGameServer.GetString(0);
			len = strlen(strGold);
			if ( len != 20 ) 
			{
				ret = false;
			}
			else 
			{
				char szgold[11];
				memcpy(szgold, strGold, 10);
				szgold[10] = 0;
				char szgold1[11];
				memcpy(szgold1, strGold+10, 10);
				szgold1[10] = 0;
				Unsigned __int64 gold1 = _strtoui64(szgold, NULL, 16);
				Unsigned __int64 gold2 = _strtoui64(szgold1, NULL, 16);

				if( !GoldDecodeByAccount( AccountID, gold1, gold2 ) )
				{
					ret = false;
				}
				else if( gold1 != gold2 )
				{
					ret = false;
				}	
				else
				{
					gold = gold1;
				}
			}
		}

		if (ret == false) 
		{
			stringstream LogStream;
			return false;
		}

		ret = true;
		RetCode = COMMON_NOERROR;

	}
	catch(CSQLException & e)
	{
		ret = false;
		RetCode = QUERYREQUEST_ERROR;

		stringstream LogStream;
		LogStream << "Error Message(GetGoldForAccount):"<< e.w_msgtext << ends;
		g_Log.WriteError(LogStream);
	}
	catch(...)
	{
		ret = false;
		RetCode = QUERYREQUEST_ERROR;
		g_Log.WriteError("Error Message(GetGoldForAccount): Unknown exception.");
	}

	return ret;
}
*/

static bool LoadUserMoney(uint32 UserID, std::string &sAccountID, uint32 &uCurMoney)
{
	MAKEQUERY_GetUserMoney(strQuery, UserID);

	CDBConnectionRest	DB(MainDB);
	DB_EXE_QUERY(DB, Stmt, strQuery);
	if (!nQueryResult)
	{
		return false;
	}

	DB_QUERY_RESULT_FETCH(Stmt, row);
	if (!IS_DB_ROW_VALID(row))
	{
		sipwarning("Cannot find User Money. UserID=%d", UserID);
		return false;
	}

	COLUMN_DIGIT(row, 0, uint32, uMoney);
	COLUMN_STR(row,	1, sCurMoney, 20);
	COLUMN_STR(row,	2, _sAccountID, MAX_LEN_USER_ID);

	sAccountID = _sAccountID;

	bool	bDataOK = false;
	if(sCurMoney == "")
	{
		bDataOK = true;
		uCurMoney = 0;
	}
	else if(sCurMoney.length() == 20)
	{
		char	szgold[11];
		memcpy(szgold, sCurMoney.c_str(), 10);
		szgold[10] = 0;
		char szgold1[11];
		memcpy(szgold1, sCurMoney.c_str()+10, 10);
		szgold1[10] = 0;
		uint64 gold1 = strtoll/*_strtoui64*/(szgold, NULL, 16);
		uint64 gold2 = strtoll/*_strtoui64*/(szgold1, NULL, 16);

		if(GoldDecodeByAccount(sAccountID.c_str(), gold1, gold2))
		{
			if(gold1 == gold2)
			{
				bDataOK = true;
				uCurMoney = (uint32)gold1;
			}
		}
	}
	if(!bDataOK)
	{
		sipwarning("Money Data Error. UserID=%d", UserID);
		return false;
	}

	return true;
}

static bool SaveUserMoney(uint32 UserID, const std::string sAccountID, uint32 CurMoney, uint32 OriginalMoney, uint16 MainType=0, uint8 SubType=0, uint32 FamilyID=0, uint32 ItemID=0, uint32 ItemTypeID=0, uint32 v1=0, uint32 v2=0, uint32 v3=0, uint32 ShardID=0, uint8 UserTypeID=0)
{
	string	sOriginalMoney = GetGoldEncodeStringByAccount(sAccountID, OriginalMoney);
	string	sCurMoney = GetGoldEncodeStringByAccount(sAccountID, CurMoney);

	MAKEQUERY_SetUserMoney(strQ, UserID, CurMoney, sCurMoney, OriginalMoney, sOriginalMoney, MainType, SubType, FamilyID, ItemID, ItemTypeID, v1, v2, v3, ShardID, UserTypeID);

	CDBConnectionRest	DB(MainDB);
	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
	if ( ! nPrepareRet )
	{
		sipwarning("Main_user_money_set prepare statement failed!!");
		return false;
	}

	SQLLEN len1 =  0;
	uint32	ret = 0;

	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len1);

	DB_EXE_PARAMQUERY(Stmt, strQ);
	if ( ! nQueryResult || (ret != 0))
	{
		sipwarning("Main_user_money_set DB_EXE_PARAMQUERY failed!: nQueryResult=%d, ret=%d", nQueryResult, ret);
		return false;
	}

	return true;
}
static uint32 uTotalUsedMoney = 0;
// packet : M_LS_EXPENDMONEY
// WS -> LS
void cbWSExpendMoney(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	uResult = ERR_NOERR;
	
	uint8	flag;			msgin.serial(flag);
	uint32	moneyExpend;	msgin.serial(moneyExpend);
	uint16	maintype;		msgin.serial(maintype);
	uint8	subtype;		msgin.serial(subtype);
	uint32	UserID;			msgin.serial(UserID);
	uint8	usertypeid;		msgin.serial(usertypeid);
	uint32	familyid;		msgin.serial(familyid);
	uint32	itemid;			msgin.serial(itemid);
	uint32	itemtypeid;		msgin.serial(itemtypeid);
	uint32	v1;				msgin.serial(v1);
	uint32	v2;				msgin.serial(v2);
	uint32	v3;				msgin.serial(v3);
	uint32	shardId;		msgin.serial(shardId);

//	if(flag != 1)
//	{
//		sipwarning("Cannot service flag:%d", flag);
//		return;
//	}

	bool	bSaveResult = false;
	int		nRetryNumber = 0;
	//while (!bSaveResult && (nRetryNumber++ < 2))
	{
		uint32 CurMoney, OriginalMoney;
		std::string sAccountID;

		if(!LoadUserMoney(UserID, sAccountID, OriginalMoney))
		{
			sipwarning("LoadUserMoney failed. UserID=%d", UserID);
			return;
		}

		if (flag == 1)
		{
			if (OriginalMoney < moneyExpend)
				CurMoney = 0;
			else
				CurMoney = OriginalMoney - moneyExpend;
		}
		else if (flag == 2)
		{
			CurMoney = OriginalMoney + moneyExpend;
		}
		else if (flag == 3)
		{
			CurMoney = OriginalMoney + v1;
			v1 = CurMoney;
		}

		if ((maintype == DBLOG_MAINTYPE_BUYROOM) && (subtype == DBLSUB_BUYROOM_NEW_CARD))
			v1 = 0;

		bSaveResult = SaveUserMoney(UserID, sAccountID, CurMoney, OriginalMoney, maintype, subtype, familyid, itemid, itemtypeid, v1, v2, v3, shardId, usertypeid);
	}
	if (flag == 1)
	{
		uTotalUsedMoney += moneyExpend;
		//sipdebug("User(id:%u) used Money(%u), TotalUsedMoney(%u)", UserID, moneyExpend, uTotalUsedMoney); byy krs
	}
	if (flag == 2)
	{
		//sipdebug("User(id:%u) added Money(%u)", UserID, moneyExpend); byy krs
	}

//	MAKEQUERY_USERMONEY(strQuery, flag, moneyExpend, maintype, subtype, userid, familyid, itemid, itemtypeid, v1, v2, v3, shardId, usertypeid);
//	MainDB->execute(strQuery);

	// ??? ? ?? money? ????? ??? ??.
/*
	DB_EXE_QUERY_WITHONEROW(MainDBConnection, Stmt, strQuery)
	if ( !nQueryResult )
	{
		sipwarning("Query Failed! : %S", strQuery);
		return;
	}
    COLUMN_DIGIT(row, 0, uint32, ret);
    COLUMN_DIGIT(row, 1, uint32, money);
	switch ( ret )
	{
	case 0:
		uResult = ERR_NOERR;
		RESULT_SHOW(strQuery, ERR_NOERR);
		break;
	case 1:
		{
			uResult = E_NO_EXIST;
			ERR_SHOW(strQuery, E_NO_EXIST);
			// ºñ¹ýÆÄÄÉÆ®¹ß°ß
			// ucstring ucUnlawContent = SIPBASE::_toString("Can't Modify User's Money, There is no userInfo, userID :%u", userid).c_str();
			// RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, userid);
			return;
		}
	case 1009:
		{
			uResult = E_NOMONEY;
			ERR_SHOW(strQuery, E_NOMONEY);
			// ºñ¹ýÆÄÄÉÆ®¹ß°ß
			// ucstring ucUnlawContent = SIPBASE::_toString("Can't consume money anymore, There is no enough Money for User(ID:%u)", userid).c_str();
			// RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, userid);
			return;
		}
	case -1:
		sipinfo("DBError : Can't Insert");
		uResult = E_DBERR;
		ERR_SHOW(strQuery, E_DBERR);
		break;
	default:
		sipwarning(L"Unknown Error Query <%s>: %u", strQuery, ret);
		break;
	}

	if ( uResult == ERR_NOERR )
		sipinfo("User(id:%u) consume(type :%u) Money(%u)", userid, flag, moneyExpend);
*/
}

/*static void	DBCB_RefreshMoney(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		UserID	= *(uint32 *)argv[0];
	uint32		FID		= *(uint32 *)argv[1];
	uint16		sid		= *(uint16 *)argv[2];
	uint16		fessid	= *(uint16 *)argv[3];
	TServiceId	tsid(*(uint16 *)argv[4]);

	if ( !nQueryResult )
		return;

	uint32 nRows;	resSet->serial(nRows);
	if(nRows != 1)
	{
		sipwarning("nRows must 1. UserID=%d, nRows=%d", UserID, nRows);
		return;
	}

	uint32 AddedMoney;		resSet->serial(AddedMoney);
	std::string sCurMoney;	resSet->serial(sCurMoney);
	std::string sAccountID;	resSet->serial(sAccountID);

	uint32	money = 0;
	bool	bDataOK = false;
	if(sCurMoney.length() == 20)
	{
		char	szgold[11];
		memcpy(szgold, sCurMoney.c_str(), 10);
		szgold[10] = 0;
		char szgold1[11];
		memcpy(szgold1, sCurMoney.c_str()+10, 10);
		szgold1[10] = 0;
		uint64 gold1 = _strtoui64(szgold, NULL, 16);
		uint64 gold2 = _strtoui64(szgold1, NULL, 16);

		if(GoldDecodeByAccount(sAccountID, gold1, gold2))
		{
			if(gold1 == gold2)
			{
				bDataOK = true;
				money = gold1;
			}
		}
	}
	if(!bDataOK)
	{
		sipwarning("Money Data Error. UserID=%d", UserID);
		return;
	}

	// send the check result to WS
	CMessage	msgout(M_LS_USERMONEY);
	msgout.serial(sid);
	msgout.serial(FID);
	msgout.serial(fessid);
	msgout.serial(money);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}*/

// Packet: M_LS_REFRESHMONEY
// Request family UMoney [d:UserID][d:FamilyID][w:RoomSID]
void cbWSRefreshMoney(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	UserID;		msgin.serial(UserID);
	uint32	FID;		msgin.serial(FID);
	uint16	sid;		msgin.serial(sid);
	uint16	fessid;		msgin.serial(fessid);

	std::string sAccountID;
	uint32	money = 0;

	if(!LoadUserMoney(UserID, sAccountID, money))
	{
		sipwarning("LoadUserMoney failed. UserID=%d", UserID);
		return;
	}

	// send the check result to WS
	CMessage	msgout(M_LS_USERMONEY);
	msgout.serial(sid);
	msgout.serial(FID);
	msgout.serial(fessid);
	msgout.serial(money);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

static void	DBCB_CheckPwd2(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		ret		= *(uint32 *)argv[0];
	uint32		FID		= *(uint32 *)argv[1];
	uint8		reason	= *(uint8 *)argv[2];
	uint32		roomid	= *(uint32 *)argv[3];
	uint32		nextfid	= *(uint32 *)argv[4];
	uint16		sid		= *(uint16 *)argv[5];
	TServiceId	tsid(*(uint16 *)argv[6]);

	if ( !nQueryResult )
		return;

	uint32	uResult;
	switch(ret)
	{
	case 0:
		uResult = ERR_NOERR;
		break;
	case 13:
		uResult = ERR_BADPASSWORD2;
		break;
	default:
		uResult = E_DBERR;
		break;
	}

	// send the check result to WS
	CMessage	msgout(M_LS_CHECKPWD2_R);
	msgout.serial(sid, uResult, FID, reason, roomid, nextfid);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

// M_LS_CHECKPWD2
void cbWSCheckPwd2(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	userid;		msgin.serial(userid);
	uint32	FID;		msgin.serial(FID);
	string	password2;	msgin.serial(password2);
	uint8	reason;		msgin.serial(reason);
	uint32	roomid;		msgin.serial(roomid);
	uint32	nextfid;	msgin.serial(nextfid);
	uint16	sid;		msgin.serial(sid);

	MAKEQUERY_CheckAuth2(strQ, userid, password2.c_str());
	MainDB->executeWithParam(strQ, DBCB_CheckPwd2,
		"DDBDDWW",
		OUT_,		NULL,
		CB_,		FID,
		CB_,		reason,
		CB_,		roomid,
		CB_,		nextfid,
		CB_,		sid,
		CB_,		tsid.get());
/*
	DB_EXE_PREPARESTMT(MainDBConnection, Stmt, strQ);
	if ( ! nPrepareRet )
	{
		sipwarning("cbWSCheckPwd2: prepare statement failed!!");
		return;
	}

	int len1 = password2.size() * sizeof(SQLCHAR);
	int len2 =  0;	
	uint32	ret = 0;

//	DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_CHAR, SQL_VARCHAR, password2.size(), 0, (void *)password2.c_str(), 0, &len1);
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len2);

	if ( ! nBindResult )
	{
		sipwarning("cbWSCheckPwd2: bind failed!");
		return;
	}

	DB_EXE_PARAMQUERY(Stmt, strQ);
	if ( ! nQueryResult )
	{
		sipwarning("cbWSCheckPwd2: execute failed!");
		return;
	}

	uint32	uResult;
	switch(ret)
	{
	case 0:
		uResult = ERR_NOERR;
		break;
	case 13:
		uResult = ERR_BADPASSWORD2;
		break;
	default:
		uResult = E_DBERR;
		break;
	}

	// send the check result to WS
	CMessage	msgout(M_LS_CHECKPWD2_R);
	msgout.serial(sid, uResult, FID, reason, roomid, nextfid);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
	*/
}

//static void	DBCB_RoomCard_Check(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	uint32		ret			= *(uint32 *)argv[0];
//	uint16		CardType	= *(uint16 *)argv[1];
//	uint32		FID			= *(uint32 *)argv[2];
//	uint8		flag		= *(uint8 *)argv[3];
//	uint32		RoomID		= *(uint32 *)argv[4];
//	string		CardID		= (char *)argv[5];
//	uint16		oroom_sid	= *(uint16 *)argv[6];
//	TServiceId	tsid(*(uint16 *)argv[7]);
//
//	if ( !nQueryResult )
//		return;
//
//	uint32	uResult;
//	switch(ret)
//	{
//	case 0:
//		uResult = ERR_NOERR;
//		break;
//	case 15:
//		uResult = ERR_CARD_BADID;
//		break;
//	case 17:
//		uResult = ERR_CARD_USED;
//		break;
//	case 18:
//		uResult = ERR_CARD_TIMEOUT;
//		break;
//	default:
//		uResult = E_DBERR;
//		break;
//	}
//
//	// send the check result to WS
//	CMessage	msgout(M_MS_ROOMCARD_CHECK);
//	msgout.serial(oroom_sid, uResult, CardType, FID, flag, RoomID, CardID);
//	CUnifiedNetwork::getInstance()->send(tsid, msgout);
//}
//
//// M_SM_ROOMCARD_CHECK
//void cb_M_SM_ROOMCARD_CHECK(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
//{
//	uint32	FID, RoomID;
//	uint8	flag;
//	string	CardID;
//	uint16	oroom_sid;
//	msgin.serial(FID, flag, RoomID, CardID, oroom_sid);
//
//	MAKEQUERY_RoomCard_Check(strQ, CardID);
//	UserDB->executeWithParam(strQ, DBCB_RoomCard_Check,
//		"DWDBDsWW",
//		OUT_,		NULL,
//		OUT_,		NULL,
//		CB_,		FID,
//		CB_,		flag,
//		CB_,		RoomID,
//		CB_,		CardID.c_str(),
//		CB_,		oroom_sid,
//		CB_,		tsid.get());
//}

static void	DBCB_RoomCard_Use(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint16		CardType	= *(uint16 *)argv[0];
	uint32		ret			= *(uint32 *)argv[1];
	uint32		FID			= *(uint32 *)argv[2];
	string		CardID		= (char *)argv[3];
	uint32		RoomID		= *(uint32 *)argv[4];
	uint16		lobby_sid	= *(uint16 *)argv[5];
	TServiceId	tsid(*(uint16 *)argv[6]);

	if ( !nQueryResult )
		return;

	sipinfo("CEUserDB_Card_Room_Use return code=%d. FID=%d, CardID=%s", ret, FID, CardID.c_str());

	uint32	uResult = 0;
	switch(ret)
	{
	case 0:
		uResult = ERR_NOERR;
		break;
	case 2:
	case 20:
		uResult = ERR_CARD_BADID;
		break;
	case 19:
		uResult = ERR_CARD_STOP;
		break;
	case 16:
		uResult = ERR_CARD_USED;
		break;
	case 21:
		uResult = ERR_CARD_TIMEOUT;
		break;
	default:
		uResult = E_DBERR;
		break;
	}

	// send the check result to WS
	CMessage	msgout(M_MS_ROOMCARD_USE);
	msgout.serial(lobby_sid, FID, CardID, RoomID, uResult, CardType);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

// Use Tianyuan Card ([d:FID][d:UserID][s:CardID][s:UserIP][w:LobbyService])
void cb_M_SM_ROOMCARD_USE(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	UserID, FID, RoomID;
	string	CardID;
	uint16	lobby_sid;	
	string	UserIP;

	msgin.serial(FID, UserID, CardID, UserIP, RoomID, lobby_sid);

	MAKEQUERY_RoomCard_Use(strQ, CardID, UserID, UserIP);
	UserDB->executeWithParam(strQ, DBCB_RoomCard_Use,
		"WDDsDWW",
		OUT_,		NULL,
		OUT_,		NULL,
		CB_,		FID,
		CB_,		CardID.c_str(),
		CB_,		RoomID,
		CB_,		lobby_sid,
		CB_,		tsid.get());
}

struct _BankItem
{
	uint32	index_id;
	uint32	item_id;
	uint32	count;
	_BankItem(uint32 _index_id, uint32 _item_id, uint32 _count)
	{
		index_id = _index_id;
		item_id = _item_id;
		count = _count;
	}
};

//static void	DBCB_GetNewuserBankitem(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	uint32		ret		= *(uint32 *)argv[0];
//	uint16		RoomSid	= *(uint16 *)argv[1];
//	uint32		FID		= *(uint32 *)argv[2];
//	uint32		UID		= *(uint32 *)argv[3];
//	TServiceId			tsid(*(uint16 *)argv[4]);
//
//	if ( !nQueryResult || ret != 0 )
//		return;
//
//	uint32 nRows;	resSet->serial(nRows);
//	std::vector<_BankItem>	aryBankItems;
//	for ( uint32 i = 0; i < nRows; i ++)
//	{
//		uint32 index_id;	resSet->serial(index_id);
//		uint32 item_id;		resSet->serial(item_id);
//		uint32 count;		resSet->serial(count);
//
//		_BankItem	data(index_id, item_id, count);
//		aryBankItems.push_back(data);
//	};
//
//	if(!aryBankItems.empty())
//	{
//		uint8	count = (uint8)aryBankItems.size();
//
//		CMessage	msgOut(M_MS_BANKITEM_GET);
//		msgOut.serial(RoomSid, FID, UID, count);
//
//		for(uint8 i = 0; i < count; i ++)
//		{
//			msgOut.serial(aryBankItems[i].index_id, aryBankItems[i].item_id, aryBankItems[i].count);
//		}
//
//		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//	}
//}
//// Packet: M_SM_BANKITEM_GET_NUSER
//void cbWSGetNewuserBankitem(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
//{
//	uint16		RoomSid;	msgin.serial(RoomSid);
//	uint32		FID;		msgin.serial(FID);
//	uint32		UID;		msgin.serial(UID);
//
//	MAKEQUERY_GetNewUserItem(strQ, UID);
//	MainDB->executeWithParam(strQ, DBCB_GetNewuserBankitem,
//		"DWDDW",
//		OUT_,		NULL,
//		CB_,		RoomSid,
//		CB_,		FID,
//		CB_,		UID,
//		CB_,		tsid.get());
//}

//static void	DBCB_GetBankitemList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	uint32		ret		= *(uint32 *)argv[0];
//	uint16		RoomSid	= *(uint16 *)argv[1];
//	uint16		FSSid	= *(uint16 *)argv[2];
//	uint32		FID		= *(uint32 *)argv[3];
//	uint32		UID		= *(uint32 *)argv[4];
//	TServiceId			tsid(*(uint16 *)argv[5]);
//
//	if ( !nQueryResult || ret != 0 )
//		return;
//
//	std::vector<_BankItem>	aryBankItems;
//
//	uint32 nRows;	resSet->serial(nRows);
//	for ( uint32 i = 0; i < nRows; i ++)
//	{
//		uint32 index_id;	resSet->serial(index_id);
//		uint32 item_id;		resSet->serial(item_id);
//		uint32 count;		resSet->serial(count);
//		_BankItem	data(index_id, item_id, count);
//		aryBankItems.push_back(data);
//	}
//
//	uint8	count = (uint8)aryBankItems.size();
//	CMessage	msgOut(M_SC_BANKITEM_LIST);
//	msgOut.serial(RoomSid, FSSid, FID, count);
//	for(uint8 i = 0; i < count; i ++)
//	{
//		msgOut.serial(aryBankItems[i].index_id, aryBankItems[i].item_id, aryBankItems[i].count);
//	}
//	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//}

//// Packet: M_CS_BANKITEM_LIST
//void cbWSGetBankitemList(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
//{
//	uint16		RoomSid;	msgin.serial(RoomSid);
//	uint16		FSSid;		msgin.serial(FSSid);
//	uint32		FID;		msgin.serial(FID);
//	uint32		UID;		msgin.serial(UID);
//
//	MAKEQUERY_GetUserItemList(strQ, UID);
//	MainDB->executeWithParam(strQ, DBCB_GetBankitemList,
//		"DWWDDW",
//		OUT_,		NULL,
//		CB_,		RoomSid,
//		CB_,		FSSid,
//		CB_,		FID,
//		CB_,		UID,
//		CB_,		tsid.get());
//}

//// Packet: M_CS_BANKITEM_GET
//void cbWSGetBankitem(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
//{
//	uint16		RoomSid;	msgin.serial(RoomSid);
//	uint32		UID;		msgin.serial(UID);
//	uint32		FID;		msgin.serial(FID);
//	uint8		count;		msgin.serial(count);
//
//	std::vector<_BankItem>	aryBankItems;
//	for(uint8 i = 0; i < count; i ++)
//	{
//		uint32		IndexID;		msgin.serial(IndexID);
//		uint32		ItemID;			msgin.serial(ItemID);
//		uint32		Count;			msgin.serial(Count);
//
//		MAKEQUERY_GetUserItem(strQ, UID, IndexID, ItemID, Count);
//		CDBConnectionRest	DB(MainDB);
//		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
//		if ( ! nPrepareRet )
//		{
//			sipwarning("Main_Shard_Items_Get DB_EXE_PREPARESTMT failed!");
//			return;
//		}
//
//		int len1 = 0;	
//		uint32	ret = 0;
//
//		DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len1);
//
//		if ( ! nBindResult )
//		{
//			sipwarning("Main_Shard_Items_Get DB_EXE_BINDPARAM_OUT failed!");
//			return;
//		}
//
//		DB_EXE_PARAMQUERY(Stmt, strQ);
//		if ( ! nQueryResult )
//		{
//			sipwarning("Main_Shard_Items_Get DB_EXE_PARAMQUERY failed!");
//			return;
//		}
//		if(ret != 0)
//		{
//			switch(ret)
//			{
//			case 14:
//				sipwarning("Main_Shard_Items_Get ItemCount Overflow!");
//				break;
//			case 15:
//				sipwarning("Main_Shard_Items_Get No Data !!");
//				break;
//			default:
//				break;
//			}
//		}
//		else
//		{
//			_BankItem	data(IndexID, ItemID, Count);
//			aryBankItems.push_back(data);
//		}
//	}
//
//	count = aryBankItems.size();
//	CMessage	msgOut(M_MS_BANKITEM_GET);
//	msgOut.serial(RoomSid, FID, UID, count);
//	for(uint8 i = 0; i < count; i ++)
//	{
//		msgOut.serial(aryBankItems[i].index_id, aryBankItems[i].item_id, aryBankItems[i].count);
//	}
//	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//}

//// Packet: M_SM_BANKITEM_GET_FAIL
//void cbWSGetBankitemFailed(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
//{
//	uint32		UID;		msgin.serial(UID);
//	uint8		count;		msgin.serial(count);
//	for(uint8 i = 0; i < count; i ++)
//	{
//		uint32		IndexID;		msgin.serial(IndexID);
//		uint32		ItemCount;		msgin.serial(ItemCount);
//
//		MAKEQUERY_BackUserItem(strQ, IndexID, ItemCount);
//		MainDB->executeWithParam(strQ, NULL,
//			"D",
//			OUT_,		NULL);
//	}
//}

void cb_M_SC_CHECK_FASTENTERROOM (CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_SM_CHECK_USER_ACTIVITY(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_SM_SET_USER_ACTIVITY(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_SM_CHECK_BEGINNERMSTITEM(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_SM_SET_USER_BEGINNERMSTITEM(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_NT_NEWFAMILY(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_NT_CHANGE_USERNAME(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_CS_CHANGE_SHARD_REQ(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cb_M_NT_MOVE_USER_DATA_END(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void cbWSClientConnected (CMessage &msgin, const std::string &serviceName, TServiceId tsid);

static const TUnifiedCallbackItem WSCallbackArray[] =
{
	{ M_SC_CHECK_FASTENTERROOM,	cb_M_SC_CHECK_FASTENTERROOM },

	{ M_CLIENT_CONNECT_STATUS,	cbWSClientConnected },
	{ M_WL_WS_IDENT,		cbWSIdentification },
	{ M_LS_EXPENDMONEY,		cbWSExpendMoney },
	//{ M_LS_CANUSEMONEY,		cbWSCanUseMoney },
	{ M_LS_REFRESHMONEY,	cbWSRefreshMoney },
	{ M_LS_CHECKPWD2,		cbWSCheckPwd2 },

	//{ M_SM_ROOMCARD_CHECK,	cb_M_SM_ROOMCARD_CHECK },
	{ M_SM_ROOMCARD_USE,	cb_M_SM_ROOMCARD_USE },

	{ M_LS_REPORT_FS_STATE,	cbWSReportFSState },
	{ M_LS_SET_SHARD_OPEN,	cbWSSetShardOpen },
	{ M_LS_OL_ST,			cbWSShardOnlineState },
	{ M_ENTER_SHARD_ANSWER,	cbWSShardChooseShard },
	{ M_LOGIN_SERVER_RPC,	cbWSRemovedPendingCookie },

	{ M_LS_RELOGOFF,			cbWSReviseLogoff },
	//{ "SHARD_LOBBYS",       cbWSLobbyList	},   // Receive the shard's lobby list

	//{ M_SM_BANKITEM_GET_NUSER,	cbWSGetNewuserBankitem },
	//{ M_CS_BANKITEM_LIST,		cbWSGetBankitemList },
	//{ M_CS_BANKITEM_GET,		cbWSGetBankitem },
	//{ M_SM_BANKITEM_GET_FAIL,	cbWSGetBankitemFailed },

	// ±â³äÀÏÈ°µ¿°ü·Ã
	{ M_SM_CHECK_USER_ACTIVITY,	cb_M_SM_CHECK_USER_ACTIVITY },
	{ M_SM_SET_USER_ACTIVITY,	cb_M_SM_SET_USER_ACTIVITY },
	{ M_SM_CHECK_BEGINNERMSTITEM,cb_M_SM_CHECK_BEGINNERMSTITEM },
	{ M_SM_SET_USER_BEGINNERMSTITEM, cb_M_SM_SET_USER_BEGINNERMSTITEM },

	// »õ »ç¿ëÀÚ°¡ Ã¢Á¶µÊ
	{ M_NT_NEWFAMILY,		cb_M_NT_NEWFAMILY },
	{ M_NT_CHANGE_USERNAME,	cb_M_NT_CHANGE_USERNAME },

	// Áö¿ª½á¹öÀýÈ¯¿ë==============
	{ M_CS_CHANGE_SHARD_REQ,		cb_M_CS_CHANGE_SHARD_REQ },
	{ M_NT_MOVE_USER_DATA_END,		cb_M_NT_MOVE_USER_DATA_END },

	{ M_CS_ADDCONTGROUP,			cb_M_CS_ADDCONTGROUP },
	{ M_CS_DELCONTGROUP,			cb_M_CS_DELCONTGROUP },
	{ M_CS_RENCONTGROUP,			cb_M_CS_RENCONTGROUP },
	{ M_CS_SETCONTGROUP_INDEX,		cb_M_CS_SETCONTGROUP_INDEX },
	{ M_CS_ADDCONT,					cb_M_CS_ADDCONT },
	{ M_CS_DELCONT,					cb_M_CS_DELCONT },
	{ M_CS_CHANGECONTGRP,			cb_M_CS_CHANGECONTGRP },
	{ M_NT_REQ_FRIEND_INFO,			cb_M_NT_REQ_FRIEND_INFO },
	{ M_NT_REQ_FAMILY_INFO,			cb_M_NT_REQ_FAMILY_INFO },

	{ M_CS_INSFAVORROOM,			cb_M_CS_INSFAVORROOM },
	{ M_CS_DELFAVORROOM,			cb_M_CS_DELFAVORROOM },
	{ M_CS_MOVFAVORROOM,			cb_M_CS_MOVFAVORROOM },
	{ M_CS_NEW_FAVORROOMGROUP,		cb_M_CS_NEW_FAVORROOMGROUP },
	{ M_CS_DEL_FAVORROOMGROUP,		cb_M_CS_DEL_FAVORROOMGROUP },
	{ M_CS_REN_FAVORROOMGROUP,		cb_M_CS_REN_FAVORROOMGROUP },

	// Mail
	{ M_NT_REQ_MAILBOX_STATUS_CHECK,	cb_M_NT_REQ_MAILBOX_STATUS_CHECK },
	{ M_CS_MAIL_GET_LIST,			cb_M_CS_MAIL_GET_LIST },
	{ M_CS_MAIL_GET,				cb_M_CS_MAIL_GET },
	{ M_CS_MAIL_SEND,				cb_M_CS_MAIL_SEND },
	{ M_NT_MAIL_SEND,				cb_M_NT_MAIL_SEND },
	{ M_CS_MAIL_DELETE,				cb_M_CS_MAIL_DELETE },
	{ M_CS_MAIL_REJECT,				cb_M_CS_MAIL_REJECT },
	{ M_CS_MAIL_ACCEPT_ITEM,		cb_M_CS_MAIL_ACCEPT_ITEM },
	{ M_NT_MAIL_ACCEPT_ITEM,		cb_M_NT_MAIL_ACCEPT_ITEM },

	{ M_NT_TEMP_MAIL_ADDPOINT_TO_OLDROOM,		cb_M_NT_TEMP_MAIL_ADDPOINT_TO_OLDROOM },

	// Chit
	{ M_NT_ADDCHIT,					cb_M_NT_ADDCHIT },
	{ M_CS_RESCHIT,					cb_M_CS_RESCHIT },

	// ¿¹¾àÀÇ½Ä
	{ M_NT_AUTOPLAY3_REGISTER,		cb_M_NT_AUTOPLAY3_REGISTER },
	{ M_NT_AUTOPLAY3_START_OK,		cb_M_NT_AUTOPLAY3_START_OK },
	{ M_NT_AUTOPLAY3_EXP_ADD_OK,	cb_M_NT_AUTOPLAY3_EXP_ADD_OK },
	{ M_NT_AUTOPLAY3_FAIL,			cb_M_NT_AUTOPLAY3_FAIL },
};

//extern void	LoadDBSharedState()
//{
//    MAKEQUERY_LOADSHARDSTATE(strQuery);
//	CDBConnectionRest	DB(MainDB);
//	DB_EXE_QUERY(DB, Stmt, strQuery);
//	if (nQueryResult != true)
//	{
//		sipwarning("Query (%S) Failed", strQuery);
//		return;
//	}
//
//	uint8	nMin = 100, nMax = 0, nRange = 0;
//	int		oldShardId = -1;
//	do
//	{
//		DB_QUERY_RESULT_FETCH(Stmt, row);
//		if ( !IS_DB_ROW_VALID(row) )
//			break;
//
//		COLUMN_DIGIT(row,	SHARDSTATEDEF_SHARDID,	uint32, shardId);
//        if ( oldShardId == -1 )
//		{
//			oldShardId = shardId;
//		}
//		else
//		if ( oldShardId != shardId )
//		{
//			if ( nMin != 0 || nMax != 100 )
//			{
//				string	str = SIPBASE::toString("Invalid ShardState Data : Min Percent or Max Percent is wrong, Min:%u, Max:%u", nMin, nMax);
//				siperror(str.c_str());
//			}
//			if ( nRange < 100 - 0 + 1 )
//			{
//				string	str = SIPBASE::toString("Invalid ShardState Data : Empty Value Percent ShardId:%u", oldShardId);
//				siperror(str.c_str());
//			}
//			if ( nRange > 100 - 0 + 1 )
//			{
//				string	str = SIPBASE::toString("Invalid ShardState Data : Overlapped Value Percent ShardId:%u", oldShardId);
//				siperror(str.c_str());
//			}
//
//			nMax = 0;
//			nMin = 100;
//			nRange = 0;
//		}
//
//		COLUMN_DIGIT(row,	SHARDSTATEDEF_STATEID,	uint8,	stateId);
//		COLUMN_DIGIT(row,	SHARDSTATEDEF_USERTYPEID,	uint8,	usertypeId);
//		COLUMN_DIGIT(row,	SHARDSTATEDEF_STARTPER,	uint8,	startPer);
//		COLUMN_DIGIT(row,	SHARDSTATEDEF_ENDPER,	uint8,	endPer);
//		nRange += endPer - startPer + 1;
//
//		if ( nMin > startPer )
//			nMin = startPer;
//		if ( nMax < endPer )
//			nMax = endPer;
//
//		_entryShardState	info(shardId, stateId, usertypeId, startPer, endPer);
//		tblShardState.insert(std::make_pair(shardId, info));
//
//		oldShardId = shardId;
//	}while ( true );
//
//	if ( oldShardId != -1 )
//	{
//		if ( nMin != 0 || nMax != 100 )
//		{
//			string	str = SIPBASE::toString("Invalid ShardState Data : Min Percent or Max Percent is wrong, Min:%u, Max:%u", nMin, nMax);
//			siperror(str.c_str());
//		}
//		if ( nRange < 100 - 0 + 1 )
//		{
//			string	str = SIPBASE::toString("Invalid ShardState Data : Empty Value Percent ShardId:%u", oldShardId);
//			siperror(str.c_str());
//		}
//		if ( nRange > 100 - 0 + 1 )
//		{
//			string	str = SIPBASE::toString("Invalid ShardState Data : Overlapped Value Percent ShardId:%u", oldShardId);
//			siperror(str.c_str());
//		}
//	}
//}

void LoadDBShardList()
{
	// Load Shard List
	MAKEQUERY_GETSHARDLIST(strQuery);
	CDBConnectionRest	DB(MainDB);
	DB_EXE_QUERY(DB, Stmt, strQuery);
	if (nQueryResult != true)
	{
		sipwarning("Failed Loading Shard List, Query : %s", strQuery);
		return;
	}

	do
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
			break;

		enum { _SHARDID, _NAME, _ZONEID, _USERLIMIT, _CONTENT, _ACTIVEFLAG, _NBPLAYERS, _CODES, _DEFAULTSHARD };
		COLUMN_DIGIT(row,	_SHARDID,	uint32,		shardId);
		COLUMN_WSTR(row,	_NAME,		ucName,		MAX_LEN_SHARD_NAME);
		COLUMN_DIGIT(row,	_ZONEID,	uint8,		zoneId);
		COLUMN_DIGIT(row,	_USERLIMIT, uint32,		nUserLimit);
		COLUMN_WSTR(row,	_CONTENT,	ucContent,	MAX_LEN_SHARD_CONTENT);
		COLUMN_DIGIT(row,	_ACTIVEFLAG,uint8,		activeflag);
		COLUMN_DIGIT(row,	_NBPLAYERS, uint32,		nbPlayers);
		COLUMN_WSTR(row,	_CODES,		ucCodeList,	MAX_LEN_SHARD_CODES);
		COLUMN_DIGIT(row,	_DEFAULTSHARD,uint8,	defaultflag);

		ucstring	ucCodeListAll = ucCodeList;
		uint8	MainShardCode = 0, ShardCode;
		ucstring	strShardCode;
		int		pos;

		do
		{
			pos = ucCodeList.find(L",");
			if(pos >= 0)
			{
				strShardCode = ucCodeList.substr(0, pos);
				ucCodeList = ucCodeList.substr(pos + 1);
			}
			else
			{
				strShardCode = ucCodeList;
			}
			// ShardCode = (uint8)_wtoi(strShardCode.c_str());
			ShardCode = (uint8)wcstol(strShardCode.c_str(), NULL, 10);
			if(ShardCode > 0)
			{
				if(MainShardCode == 0)
					MainShardCode = ShardCode;
				if(g_mapShardCodeToShardID.find(ShardCode) != g_mapShardCodeToShardID.end())
				{
					siperror("Same ShardCode exist!!! shardId=%d, ShardCode=%d, g_mapShardCodeToShardID.size()=%d", shardId, ShardCode, g_mapShardCodeToShardID.size());
					break;
				}
				g_mapShardCodeToShardID[ShardCode] = shardId;
			}
			else
			{
				siperror("Invalid ShardCode!!! ShardID=%d", shardId);
				break;
			}

			if (defaultflag != 0)
				g_nDefaultShardID = shardId;

		} while(pos >= 0);

		if(MainShardCode == 0)
		{
			siperror("MainShardCode = 0!");
			break;
		}

		CShardInfo		info(shardId, ucName, zoneId, nUserLimit, ucContent, MainShardCode, ucCodeListAll, defaultflag);

		g_ShardList[shardId] = info;
	}while ( true );

	sipinfo("Successed Loading Shard List");
}

//void	LoadDBSharedVIPs()
//{
//	MAKEQUERY_LOADSHARDVIPS(strQuery);
//	CDBConnectionRest	DB(MainDB);
//	DB_EXE_QUERY(DB, Stmt, strQuery);
//	if (nQueryResult != true)
//	{
//		sipwarning("Query (%S) Failed", strQuery);
//		return;
//	}
//
//	do
//	{
//		DB_QUERY_RESULT_FETCH(Stmt, row);	
//		if ( !IS_DB_ROW_VALID(row) )
//			break;
//
//		COLUMN_DIGIT(row,	SHARDVIP_SHARDID,	uint32, shardId);
//		COLUMN_DIGIT(row,	SHARDVIP_USERID,	uint32,	userId);
//
//		tblShardVIPs.insert(std::make_pair(shardId, userId));
//	}while ( true );
//}
//
// Functions
//

void connectionWSInit ()
{
	CUnifiedNetwork::getInstance ()->addCallbackArray (WSCallbackArray, sizeof(WSCallbackArray)/sizeof(WSCallbackArray[0]));
	
	CUnifiedNetwork::getInstance ()->setServiceUpCallback (WELCOME_SHORT_NAME, cbWSConnection);
	CUnifiedNetwork::getInstance ()->setServiceDownCallback (WELCOME_SHORT_NAME, cbWSDisconnection);

	LoadAllYuyueDatas();
}

void connectionWSUpdate ()
{
	RefreshMoveDataUserList();

	UpdateYuyueDatas();
	UpdateYuyueExpDatas();
}

//void connectionWSRelease ()
//{
//	sipinfo ("LS's going down, release all connected WS");
//
//	while (!g_Shards.empty())
//	{
//		cbWSDisconnection ("", TServiceId(g_Shards[0].SId), NULL);
//	}
//}
void connectionWSRelease ()
{
	sipinfo ("LS's going down, release all connected WS");

	for (std::map<uint32, CShardInfo>::iterator it = g_ShardList.begin(); it != g_ShardList.end(); it ++)
	{
		if (it->second.m_wSId != 0)
			cbWSDisconnection ("", TServiceId(it->second.m_wSId), NULL);
	}
}

//static void	SendSystemNotice(ucstring	ucNotice)
//{
//	CMessage	msgout(M_SC_CHATMAIN);
//	msgout.serial(ucNotice);
//	for ( uint32 i = 0; i < g_Shards.size(); i ++ )
//	{
//		CUnifiedNetwork::getInstance ()->send (TServiceId(g_Shards[i].SId), msgout);
//	}
//}
static void	SendSystemNotice(ucstring	ucNotice)
{
	CMessage	msgout(M_SC_CHATMAIN);
	msgout.serial(ucNotice);
	for (std::map<uint32, CShardInfo>::iterator it = g_ShardList.begin(); it != g_ShardList.end(); it ++)
	{
		if (it->second.m_wSId != 0)
			CUnifiedNetwork::getInstance ()->send(TServiceId(it->second.m_wSId), msgout);
	}
}

SIPBASE_CATEGORISED_COMMAND(LS, systemNotice, "Send system message to all world", "")
{
	if (args.size() < 1 )
		return false;
	string strNotice = args[0];
	ucstring	ucNotice = ucstring::makeFromUtf8(strNotice);

	SendSystemNotice(ucNotice);

	return true;
}


/////////////////////////////////////////////////////////////////////
//    ±â³äÀÏÈ°µ¿°ü·Ã
/////////////////////////////////////////////////////////////////////
SITE_ACTIVITY		g_SiteActivity;
SYSTEM_GIFTITEM		g_BeginnerMstItem;
SITE_CHECKIN		g_CheckIn;

void SendSiteActivityToShard(TServiceId tsid)
{
	CMessage	msgOut(M_MS_CURRENT_ACTIVITY);
	msgOut.serial(g_SiteActivity.m_ActID, g_SiteActivity.m_Text, g_SiteActivity.m_Count);
	for (int i = 0; i < g_SiteActivity.m_Count; i ++)
	{
		msgOut.serial(g_SiteActivity.m_ActItemID[i], g_SiteActivity.m_ActItemCount[i]);
	}

	if (tsid.get() == 0)
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	else
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

void SendCheckInToShard(TServiceId tsid)
{
	CMessage	msgOut(M_MS_CURRENT_CHECKIN);
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
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	else
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

void SendBeginnerMstItemToShard(TServiceId tsid)
{
	if (g_BeginnerMstItem.m_GiftID == 0)
		return;

	CMessage	msgOut(M_MS_BEGINNERMSTITEM);
	msgOut.serial(g_BeginnerMstItem.m_GiftID, g_BeginnerMstItem.m_Title, g_BeginnerMstItem.m_Content, g_BeginnerMstItem.m_Count);
	for (int i = 0; i < g_BeginnerMstItem.m_Count; i ++)
	{
		msgOut.serial(g_BeginnerMstItem.m_ItemID[i], g_BeginnerMstItem.m_ItemCount[i]);
	}

	if (tsid.get() == 0)
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	else
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

static void	DBCB_GetCurrentActivity(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (nQueryResult != true)
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		g_SiteActivity.Reset();
		sipdebug("No Site Activity. nRows=%d", nRows);
	}
	else
	{
		uint16			ActID;				resSet->serial(ActID);
		ucstring		Text;				resSet->serial(Text);
		uint32	len;	resSet->serial(len);
		uint8	buff[MAX_SITE_ACTIVITY_ITEMCOUNT * 5 + 10];
		resSet->serialBuffer(buff, len);
		if ((len % 5) != 0)
		{
			sipwarning("(len % 5) != 0. len=%d", len);
			return;
		}
		if (len / 5 > MAX_SITE_ACTIVITY_ITEMCOUNT)
		{
			sipwarning("g_SiteActivity.m_Count > MAX_SITE_ACTIVITY_ITEMCOUNT. len=%d", len);
			return;
		}
		T_DATETIME		begintime;		resSet->serial(begintime);
		T_DATETIME		endtime;		resSet->serial(endtime);

		g_SiteActivity.m_Count = (uint8)(len / 5);

		for (uint8 i = 0; i < g_SiteActivity.m_Count; i ++)
		{
			g_SiteActivity.m_ActItemID[i] = *((uint32*)&buff[i * 5]);
			g_SiteActivity.m_ActItemCount[i] = buff[i * 5 + 4];
		}
		g_SiteActivity.m_ActID = ActID;
		g_SiteActivity.m_Text = Text;

		g_SiteActivity.m_BeginTime.SetTime(begintime);
		g_SiteActivity.m_EndTime.SetTime(endtime);

		sipinfo("Load activity id=%d, begintime=%s, endtime=%s, itemcount=%d", g_SiteActivity.m_ActID, begintime.c_str(), endtime.c_str(), g_SiteActivity.m_Count);
	}
	SendSiteActivityToShard(TServiceId(0));
}

void LoadSiteActivity()
{
	MAKEQUERY_GetCurrentActivity(strQuery);
	MainDB->execute(strQuery, DBCB_GetCurrentActivity);
}

static void	DBCB_GetCurrentCheckIn(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (nQueryResult != true)
		return;

	g_CheckIn.Reset();
	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipdebug("No CheckIn. nRows=%d", nRows);
	}
	else
	{
		uint16			ActID=1;//				resSet->serial(ActID);
		bool			bActive;			resSet->serial(bActive);
		T_DATETIME		begintime;		resSet->serial(begintime);
		T_DATETIME		endtime;		resSet->serial(endtime);
		ucstring		Text;				resSet->serial(Text);

		g_CheckIn.m_ActID = ActID;
		g_CheckIn.m_bActive = bActive;
		g_CheckIn.m_Text = Text;
		g_CheckIn.m_BeginTime.SetTime(begintime);
		g_CheckIn.m_EndTime.SetTime(endtime);

		for (uint8 i = 0; i < CIS_MAX_DAYS; i++)
		{
			uint32	len;	resSet->serial(len);
			uint8	buff[MAX_SITE_CHECKIN_ITEMCOUNT * 5 + 10];
			resSet->serialBuffer(buff, len);
			
			if ((len % 5) != 0)
			{
				sipwarning("(len % 5) != 0. len=%d", len);
			}
			if (len / 5 > MAX_SITE_CHECKIN_ITEMCOUNT)
			{
				sipwarning("g_CheckIn item Count > MAX_SITE_CHECKIN_ITEMCOUNT. len=%d", len);
			}
			SITE_CHECKIN::CHECKINITEM& curitems = g_CheckIn.m_CheckInItems[i];
			curitems.m_Count = (uint8)(len / 5);
			if (curitems.m_Count > MAX_SITE_CHECKIN_ITEMCOUNT)
				curitems.m_Count = MAX_SITE_CHECKIN_ITEMCOUNT;

			for (uint8 j = 0; j < curitems.m_Count; j ++)
			{
				curitems.m_ItemID[j] = *((uint32*)&buff[j * 5]);
				curitems.m_ItemCount[j] = buff[j * 5 + 4];
			}
		}
		sipinfo("Load CheckIn data id=%d, begintime=%s, endtime=%s", g_CheckIn.m_ActID, begintime.c_str(), endtime.c_str());
	}
	SendCheckInToShard(TServiceId(0));
}

void LoadEverydayCheckIn()
{
	MAKEQUERY_GetCurrentCheckIn(strQuery);
	MainDB->execute(strQuery, DBCB_GetCurrentCheckIn);
}

static void	DBCB_GetUserLastActivity(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (nQueryResult != true)
		return;

	uint32		UserID	= *(uint32 *)argv[0];
	uint32		FID		= *(uint32 *)argv[1];
	TServiceId	tsid(*(uint16 *)argv[2]);

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
//		sipdebug("nRows != 1. nRows=%d, UserID=%d", nRows, UserID);
		return;
	}

	uint16		_LastActID;			resSet->serial(_LastActID);
	uint32		LastActID = _LastActID;

	if (LastActID < g_SiteActivity.m_ActID)
	{
		CMessage	msgOut(M_MS_CHECK_USER_ACTIVITY);
		msgOut.serial(FID, LastActID);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
}

extern TTime GetDBTimeSecond();
void cb_M_SM_CHECK_USER_ACTIVITY(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	UserID;		msgin.serial(UserID);
	uint32	FID;		msgin.serial(FID);

	if (g_SiteActivity.m_ActID == 0)
	{
		sipwarning("g_SiteActivity.m_ActID=0. UserID=%d, FID=%d", UserID, FID);
		return;
	}

	SIPBASE::TTime	curtime = GetDBTimeSecond();
	if (curtime >= g_SiteActivity.m_BeginTime.timevalue &&
		curtime <= g_SiteActivity.m_EndTime.timevalue)
	{
		MAKEQUERY_GetUserLastActivity(strQuery, UserID);
		MainDB->executeWithParam(strQuery, DBCB_GetUserLastActivity,
			"DDW",
			CB_,	UserID,
			CB_,	FID,
			CB_,	tsid.get());
	}
	else
	{
		if (curtime > g_SiteActivity.m_EndTime.timevalue)
		{
			g_SiteActivity.Reset();
			SendSiteActivityToShard(TServiceId(0));
		}
	}
}

static void	DBCB_SetUserLastActivity(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (nQueryResult != true)
		return;

	uint32		Error	= *(uint32 *)argv[0];
	uint32		FID		= *(uint32 *)argv[1];
	uint16		rsSid	= *(uint16 *)argv[2];
	TServiceId	tsid(*(uint16 *)argv[3]);

	if (Error == 0)
	{
		CMessage	msgOut(M_MS_SET_USER_ACTIVITY);
		msgOut.serial(rsSid, FID);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
	else
	{
		sipwarning("failed. FID=%d, Error=%d", FID, Error);
	}
}

// WorldService->WS->LS, »ç¿ëÀÚ°¡  ±â³äÀÏÈ°µ¿¾ÆÀÌÅÛÀ» ¹Þ±â¸¦ ¿äÃ». ([d:UserID][FID][d:ActID])
void cb_M_SM_SET_USER_ACTIVITY(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16	rsSid;		msgin.serial(rsSid);
	uint32	UserID;		msgin.serial(UserID);
	T_FAMILYID	FID;	msgin.serial(FID);
	uint32	ActID;		msgin.serial(ActID);

	{
		MAKEQUERY_SetUserLastActivity(strQuery, UserID, ActID);
		MainDB->executeWithParam(strQuery, DBCB_SetUserLastActivity,
			"DDWW",
			OUT_,	NULL,
			CB_,	FID,
			CB_,	rsSid,
			CB_,	tsid.get());
	}
}

static void	DBCB_SetUserBeginnerItem(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (nQueryResult != true)
		return;

	uint32		Error	= *(uint32 *)argv[0];
	uint32		FID		= *(uint32 *)argv[1];
	uint16		rsSid	= *(uint16 *)argv[2];
	TServiceId	tsid(*(uint16 *)argv[3]);

	if (Error == 0)
	{
		CMessage	msgOut(M_MS_SET_USER_BEGINNERMSTITEM);
		msgOut.serial(rsSid, FID);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
	else
	{
		sipwarning("failed. FID=%d, Error=%d", FID, Error);
	}
}

// WorldService->WS->LS, »ç¿ëÀÚ°¡  »õ»ç¿ëÀÚ¾ÆÀÌÅÛÀ» ¹Þ±â¸¦ ¿äÃ». ([d:UserID][d:FID])
void cb_M_SM_SET_USER_BEGINNERMSTITEM(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16	rsSid;		msgin.serial(rsSid);
	uint32	UserID;		msgin.serial(UserID);
	T_FAMILYID	FID;	msgin.serial(FID);

	{
		MAKEQUERY_SetUserBeginnerItem(strQuery, UserID);
		MainDB->executeWithParam(strQuery, DBCB_SetUserBeginnerItem,
			"DDWW",
			OUT_,	NULL,
			CB_,	FID,
			CB_,	rsSid,
			CB_,	tsid.get());
	}
}

static void	DBCB_GetBeginnerMstItem(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (nQueryResult != true)
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipdebug("Error Beginner's master item. nRows=%d", nRows);
		return;
	}

	g_BeginnerMstItem.m_GiftID = 1;

	resSet->serial(g_BeginnerMstItem.m_Title);
	resSet->serial(g_BeginnerMstItem.m_Content);

	uint32	len;	resSet->serial(len);
	uint8	buff[MAX_SITE_ACTIVITY_ITEMCOUNT * 5 + 10];
	resSet->serialBuffer(buff, len);
	if ((len % 5) != 0)
	{
		sipwarning("(len % 5) != 0. len=%d", len);
		return;
	}
	if (len / 5 > MAX_SITE_ACTIVITY_ITEMCOUNT)
	{
		sipwarning("g_BeginnerMstItem.m_Count > MAX_SITE_ACTIVITY_ITEMCOUNT. len=%d", len);
		return;
	}
	g_BeginnerMstItem.m_Count = (uint8)(len / 5);
	sipdebug("Beginner Item Count =%d", g_BeginnerMstItem.m_Count);
	for (uint8 i = 0; i < g_BeginnerMstItem.m_Count; i ++)
	{
		g_BeginnerMstItem.m_ItemID[i] = *((uint32*)&buff[i * 5]);
		g_BeginnerMstItem.m_ItemCount[i] = buff[i * 5 + 4];

		sipdebug("Beginner Item%d, ItemID=%d, Count=%d", i, g_BeginnerMstItem.m_ItemID[i], g_BeginnerMstItem.m_ItemCount[i] );
	}

	SendBeginnerMstItemToShard(TServiceId(0));
}

void LoadBeginnerMstItem()
{
	MAKEQUERY_GetBeginnerMstItem(strQuery);
	MainDB->execute(strQuery, DBCB_GetBeginnerMstItem);
}

static void	DBCB_GetUserIsGetItem(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (nQueryResult != true)
		return;

	uint32		UserID	= *(uint32 *)argv[0];
	uint32		FID		= *(uint32 *)argv[1];
	TServiceId	tsid(*(uint16 *)argv[2]);

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipdebug("nRows != 1. nRows=%d, UserID=%d", nRows, UserID);
		return;
	}

	uint16		_LastActID;			resSet->serial(_LastActID);
	uint8		_IsGetItem;			resSet->serial(_IsGetItem);
	
	if (_IsGetItem == 0)
	{
		CMessage	msgOut(M_MS_CHECK_BEGINNERMSTITEM);
		msgOut.serial(FID);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
}

void cb_M_SM_CHECK_BEGINNERMSTITEM(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	UserID;		msgin.serial(UserID);
	uint32	FID;		msgin.serial(FID);

	if (g_BeginnerMstItem.m_GiftID == 0)
	{
		sipwarning("g_BeginnerMstItem.m_GiftID. UserID=%d, FID=%d", UserID, FID);
		return;
	}

	MAKEQUERY_GetUserLastActivityAndIsGetItem(strQuery, UserID);
	MainDB->executeWithParam(strQuery, DBCB_GetUserIsGetItem,
		"DDW",
		CB_,	UserID,
		CB_,	FID,
		CB_,	tsid.get());
}

// Notice OROOM->WS->LS that new family created ([d:UID][d:FID])
void cb_M_NT_NEWFAMILY(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	UserID;		msgin.serial(UserID);
	uint32	FID;		msgin.serial(FID);
	ucstring	FName;	msgin.serial(FName);

	uint32	ret = 0;
	{
		MAKEQUERY_SetUserFID(strQuery, UserID, FID, FName);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
		if ( !nPrepareRet )
		{
			sipwarning("DBERROR");
			return;
		}

		DB_EXE_PARAMQUERY(Stmt, strQuery);
		if ( !nQueryResult )
		{
			sipwarning("DBERROR");
			return;
		}
	}

	AddUIDtoFIDmap(UserID, FID);

	std::map<uint32, _UserInfoForDB>::iterator it = g_mapUserInfoForDB.find(UserID);
	if (it == g_mapUserInfoForDB.end())
	{
		sipwarning("g_mapUserInfoForDB.find = null. UID=%d, FID=%d", UserID, FID);
	}
	else
	{
		uint32	shardID = getMainShardIDfromFID(FID);
		it->second.m_nFID = FID;
		it->second.m_nMainShardID = shardID;
		it->second.m_nCurShardID = shardID;

		{
			MAKEQUERY_SetUserShardID(strQuery, UserID, shardID);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
			if ( !nPrepareRet )
			{
				sipwarning("DBERROR");
				return;
			}

			DB_EXE_PARAMQUERY(Stmt, strQuery);
			if ( !nQueryResult )
			{
				sipwarning("DBERROR");
				return;
			}
		}
	}

	if (!ReadFriendInfo(FID))
	{
		sipwarning("ReadFriendInfo failed. FID=%d", FID);
		return;
	}
	if (!ReadFavorRoomInfo(FID))
	{
		sipwarning("ReadFavorRoomInfo failed. FID=%d", FID);
		return;
	}
}

// Notice that character's name changed to OROOM & LOBBY ([d:FID][u:Family name][d:ModelID])	WS->other WS, LS
void cb_M_NT_CHANGE_USERNAME(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	FID;		msgin.serial(FID);
	ucstring	FName;	msgin.serial(FName);
	uint32	ModelId;	msgin.serial(ModelId);

	{
		MAKEQUERY_ChangeFamilyName(strQuery, FID, FName, ModelId);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
		if ( !nPrepareRet )
		{
			sipwarning("DBERROR");
			return;
		}

		DB_EXE_PARAMQUERY(Stmt, strQuery);
		if ( !nQueryResult )
		{
			sipwarning("DBERROR");
			return;
		}
	}
}

T_ERR	CanEnterShard(CShardInfo *pShardInfo, uint32 userid, uint8 uUserPriv)
{
	T_ERR	errNo = ERR_NOERR;
//	uint8	userTypeId = pShardInfo->m_bUserTypeId;

	if (!pShardInfo->m_bActiveFlag)
	{
		sipwarning("User can't enter this shard %u, Shard is close", pShardInfo->m_nShardId);
		errNo = ERR_SHARDCLOSE;
		return errNo;
	}
	if (pShardInfo->m_nPlayers >= pShardInfo->m_nUserLimit)
	{
		sipwarning("User can't enter this shard %u, Shard is full: limit=%d, current=%d", pShardInfo->m_nShardId, pShardInfo->m_nUserLimit, pShardInfo->m_nPlayers);
		errNo = ERR_SHARDFULL;
		return errNo;
	}

	switch (uUserPriv)
	{
		case USERTYPE_COMMON:
		case USERTYPE_MIDDLE:
		case USERTYPE_SUPER:
			if (pShardInfo->m_bIsDefaultShard <= 1)
			{
				uint32	mainShardID = getMainShardIDfromUID(userid);
				if (mainShardID != pShardInfo->m_nShardId)
				{
					// mainShard°¡ ¾Æ´Ò ¶§¿¡´Â BUSY»óÅÂÀÏ ¶§ ¸øµé¾î°¡°Ô ÇÑ´Ù.
					if (pShardInfo->m_nPlayers * 100 > pShardInfo->m_nUserLimit * ShardState_Busy)
					{
						sipwarning("Shard is busy: UID=%d, ShardID=%d", userid, pShardInfo->m_nShardId);
						errNo = ERR_SHARDFULL;
						return errNo;
					}
				}
				sipinfo("User can enter this shard, userid=%u, UserType:%u, ShardID=%d", userid, uUserPriv, pShardInfo->m_nShardId);
			}
			else
			{
				errNo = ERR_PERMISSION;
				sipwarning("User can't enter this shard, userid=%u, UserType:%u", userid, uUserPriv);
			}
			break;

		case USERTYPE_DEVELOPER:
			sipinfo("User(UserID:%u) is Developer, Can enter", userid);
			break;

		case USERTYPE_GAMEMANAGER:
			sipinfo("User(UserID:%u) is GameManager, Can enter", userid);
			break;

		case USERTYPE_GAMEMANAGER_H:
			sipinfo("User(UserID:%u) is GameManager(High), Can enter", userid);
			break;

		case USERTYPE_EXPROOMMANAGER:
			sipinfo("User(UserID:%u) is ExpRoomManager, Can enter", userid);
			break;

		case USERTYPE_WEBMANAGER:
			sipinfo("User(UserID:%u) is WebManager, Can enter", userid);
			break;

		default:
			errNo = ERR_PERMISSION;
			sipinfo("User(UserID:%u) is Unknown, UserTypeID:%u", userid, uUserPriv);
			break;
	}

	return	errNo;
}


std::map<uint32, CLoginCookie>	g_mapUserLoginCookie;	// <UID, CLoginCookie>
void SetUserLoginCookie(uint32 UID, CLoginCookie &cookie)
{
	g_mapUserLoginCookie[UID] = cookie;
}

// Request to change shard ([b:NewShardCode]) Client->FS->WS->LS
void cb_M_CS_CHANGE_SHARD_REQ(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16	FsSid;
	uint32	FID;
	uint8	NewShardCode;

	msgin.serial(FsSid, FID, NewShardCode);

	uint32	UID = FindUIDfromFID(FID);
	if (UID == 0)
	{
		sipwarning("FindUIDfromFID failed. FID=%d, NewShardCode=%d", FID, NewShardCode);
		return;
	}

	std::map<uint32, _ChangeShardReq>::iterator it1 = g_ChangeShardReqList.find(UID);
	if (it1 != g_ChangeShardReqList.end())
	{
		sipwarning("g_ChangeShardReqList.find != null. FID=%d, UID=%d, NewShardCode=%d, prevNewShardCode=%d", FID, UID, NewShardCode, it1->second.m_NewShardCode);
		return;
	}

	std::map<uint32, CLoginCookie>::iterator	it = g_mapUserLoginCookie.find(UID);
	if (it == g_mapUserLoginCookie.end())
	{
		sipwarning("g_mapUserLoginCookie.find failed. FID=%d, UID=%d", FID, UID);
		return;
	}

	CMessage msgOut(M_ENTER_SHARD_REQUEST);
	uint8	uUserPriv = it->second.getUserType();
	msgOut.serial(it->second);
	msgOut.serial(uUserPriv);
	uint32	ret = SendMsgToShardByCode(msgOut, NewShardCode);

	if (ret != ERR_NOERR)
	{
		CMessage	msgOut1(M_SC_CHANGE_SHARD_ANS);
		msgOut1.serial(FsSid, FID, NewShardCode, ret);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut1);
	}
	else
	{
		_ChangeShardReq	data(tsid.get(), FsSid, FID, NewShardCode);
		g_ChangeShardReqList.insert(make_pair(UID, data));
	}
}

bool CheckInChangeShardReqList(uint32 err, CLoginCookie &cookie, string &shardAddr)
{
	uint32	UID = cookie.getUserId();

	std::map<uint32, _ChangeShardReq>::iterator it = g_ChangeShardReqList.find(UID);
	if (it == g_ChangeShardReqList.end())
		return false;

	CMessage	msgOut1(M_SC_CHANGE_SHARD_ANS);
	msgOut1.serial(it->second.m_FsSid, it->second.m_FID, it->second.m_NewShardCode, err);
	if (err == ERR_NOERR)
		msgOut1.serial(cookie, shardAddr);
	CUnifiedNetwork::getInstance()->send(TServiceId(it->second.m_WsSid), msgOut1);

	g_ChangeShardReqList.erase(it);

	return true;
}


uint32 SendMsgToShardByCode(CMessage &msg, uint8 ShardCode)
{
	std::map<uint8, uint32>::iterator	it = g_mapShardCodeToShardID.find(ShardCode);
	if(it == g_mapShardCodeToShardID.end())
	{
		sipwarning("g_mapShardCodeToShardID.find failed. ShardCode=%d", ShardCode);
		return E_COMMON_FAILED;
	}

	uint32	ShardID = it->second;
	std::map<uint32, CShardInfo>::iterator it1 = g_ShardList.find(ShardID);
	if (it1 == g_ShardList.end())
	{
		sipwarning("g_ShardList.find failed. ShardCode=%d, ShardID=%d", ShardCode, ShardID);
		return E_COMMON_FAILED;
	}
	if (it1->second.m_wSId == 0)
	{
		sipwarning("Can't send packet because shard is not open. ShardCode=%d, ShardID=%d", ShardCode, ShardID);
		return ERR_SHARDCLOSE;
	}

	CUnifiedNetwork::getInstance()->send(TServiceId(it1->second.m_wSId), msg);
	return ERR_NOERR;
}

uint32 SendMsgToShardByID(CMessage &msg, uint32 ShardID)
{
	std::map<uint32, CShardInfo>::iterator it1 = g_ShardList.find(ShardID);
	if (it1 == g_ShardList.end())
	{
		sipwarning("g_ShardList.find failed. ShardID=%d", ShardID);
		return E_COMMON_FAILED;
	}
	if (it1->second.m_wSId == 0)
	{
		sipwarning("Can't send packet because shard is not open. ShardID=%d", ShardID);
		return ERR_SHARDCLOSE;
	}

	CUnifiedNetwork::getInstance()->send(TServiceId(it1->second.m_wSId), msg);
	return ERR_NOERR;
}

// Packet : M_CLIENT_CONNECT_STATUS
// WS -> LS
void cbWSClientConnected (CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16 sid = tsid.get();

	uint32 userid;		msgin.serial(userid);
	uint8 con;			msgin.serial(con);	// con=1 means a client is connected on the shard, 0 means a client disconnected
	uint32 nGMoney = 0;

	T_ERR errNo;
	std::string	strIP;
	SIPNET::CLoginCookie lc;
	uint16	fsSid = 0;
	if ( con == 1 )
	{
		msgin.serial(strIP, lc);
		msgin.serial(nGMoney);
		msgin.serial(fsSid);

		std::map<uint32, uint16>::iterator it = g_mapCurrentUserWsSid.find(userid);
		if (it != g_mapCurrentUserWsSid.end() && it->second != sid)
		{
			// »ç¿ëÀÚ°¡ LogoffµÉ ¶§±îÁö ±â´Ù·Á¾ß ÇÑ´Ù.
			_WaitingLogoffUser	data(strIP, lc, sid, fsSid);
			g_mapWaitingLogoffUserList.insert(make_pair(userid, data));

			sipdebug("added to WaitingLogoffUser. UID=%d, oldSid=%d, newSid=%d", userid, it->second, sid);
			return;
		}
	}
	else
	{
		CMessage		msgout(M_CLIENT_CONNECT_STATUS);
		msgout.serial(userid);
		msgout.serial(con);

		uint8	bIsGM;			msgin.serial(bIsGM);		msgout.serial(bIsGM);
		uint32	shardId;		msgin.serial(shardId);		msgout.serial(shardId);
		uint32	VisitDays;		msgin.serial(VisitDays);
		uint16	UserExp;		msgin.serial(UserExp);
		uint16	UserVirtue;		msgin.serial(UserVirtue);
		uint16	UserFame;		msgin.serial(UserFame);
		uint32	nRooms;			msgin.serial(nRooms);
		uint8	ExpRoom;		msgin.serial(ExpRoom);
		msgin.serial(nGMoney);
		bool	bRightValue;	msgin.serial(bRightValue);

		//sipdebug("CC: new user disonnected to Shard!"); byy krs
		errNo = doDisconnectUserToShard(userid, sid, VisitDays, UserExp, UserVirtue, UserFame, nRooms, ExpRoom, bIsGM, nGMoney, bRightValue);
		
		map<uint32, uint32>::iterator	it = AbnormalLogoffUsers.find(userid);
		if ( it != AbnormalLogoffUsers.end() )
		{
			SendShardListToSS(userid, FindFIDfromUID(userid));
			AbnormalLogoffUsers.erase(userid);
		}

		CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgout);
		g_mapCurrentUserWsSid.erase(userid);

		// g_mapWaitingLogoffUserList ¿¡ µî·ÏµÇ¿©ÀÖ´ÂÁö È®ÀÎÇÑ´Ù.
		std::map<uint32, _WaitingLogoffUser>::iterator it1 = g_mapWaitingLogoffUserList.find(userid);
		if (it1 == g_mapWaitingLogoffUserList.end())
		{
			if (g_mapMoveDataUsers.find(userid) == g_mapMoveDataUsers.end())
			{
				// º»Shard°¡ ¾Æ´Ñ ´Ù¸¥ Shard¿¡¼­ LogoffÇÑ °æ¿ì º»Shard·Î ÀÚ·á¸¦ ¿Å±ä´Ù.
				std::map<uint32, _UserInfoForDB>::iterator it2 = g_mapUserInfoForDB.find(userid);
				if (it2 == g_mapUserInfoForDB.end())
				{
					sipwarning("g_mapUserInfoForDB.find failed. UID=%d", userid);
					return;
				}

				if (it2->second.m_nMainShardID != 0 && it2->second.m_nMainShardID != it2->second.m_nCurShardID)
				{
					_MoveDataUserInfo	data(it2->second.m_nMainShardID, 0, 0);	// 0 : Back to main shard when client disconnect
					g_mapMoveDataUsers.insert(make_pair(userid, data));

					if (!MoveUserDataStartInShard(userid, it2->second.m_nCurShardID, it2->second.m_nMainShardID))
					{
						g_mapMoveDataUsers.erase(userid);
					}
				}
			}
			return;
		}

		//sipdebug("found in WaitingLogoffUser. UID=%d, connectSid=%d, disconnectSid=%d", userid, it1->second.m_WsSid, sid); byy krs
		strIP = it1->second.m_StrIP;
		lc = it1->second.m_Cookie;
		sid = it1->second.m_WsSid;
		fsSid = it1->second.m_FsSid;

		g_mapWaitingLogoffUserList.erase(it1);
	}

	CShardInfo* pReqShard = findShardWithSId(sid);
	if (pReqShard == NULL)
	{
		sipwarning("findShardWithSId failed. sid=%d", sid);
		return;
	}

	errNo = doConnectUserToShard(userid, sid, strIP, lc.getIsGM(), nGMoney);

	TempCookies.erase(userid);
	g_mapCurrentUserWsSid[userid] = sid;

	//sipdebug("CC: new user(id :%u) connected to Shard!", userid); byy krs

	CMessage		msgout(M_CLIENT_CONNECT_STATUS);
	msgout.serial(userid, con, lc);
	CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgout);

//	if ( errNo != ERR_NOERR )
//		sipwarning("Can't process msg<CC>. Last step of login was not finished completely, ClientConnection Error : userid : %u, errNo : %u", userid, errNo);

	// ¿©±â¼­ Client¿¡ M_ENTER__SHARD¸¦ º¸³»±â À§ÇÑ Ã³¸®¸¦ ÇÏ¿©¾ß ÇÑ´Ù.
	{
		std::map<uint32, _UserInfoForDB>::iterator it = g_mapUserInfoForDB.find(userid);
		if (it == g_mapUserInfoForDB.end())
		{
			sipwarning("g_mapUserInfoForDB.find failed. UID=%d", userid);
			return;
		}
		std::map<uint32, _MoveDataUserInfo>::iterator it1 = g_mapMoveDataUsers.find(userid);
		if (it1 != g_mapMoveDataUsers.end())
		{
			if (pReqShard->m_nShardId == it1->second.m_nTargetShardID)
			{
				it1->second.m_FsSid = fsSid;
				it1->second.m_Reason = 1;	// 1 : WaitEnterShard
				sipinfo("found in g_mapMoveDataUsers, waiting. UID=%d, targetShardID=%d, oldShardID=%d", userid, pReqShard->m_nShardId, it1->second.m_nTargetShardID);
			}
			else
			{
				sipwarning("g_mapMoveDataUsers.find != null. UID=%d, targetShardID=%d, oldShardID=%d, reason=%d", userid, pReqShard->m_nShardId, it1->second.m_nTargetShardID, it1->second.m_Reason);
			}
			return;
		}

		if (it->second.m_nCurShardID == 0 || pReqShard->m_nShardId == it->second.m_nCurShardID)
		{
			SendEnterShardOk(userid, pReqShard->m_nShardId, fsSid);
		}
		else
		{
			_MoveDataUserInfo	data(pReqShard->m_nShardId, fsSid, 1);	// 1 : WaitEnterShard
			g_mapMoveDataUsers.insert(make_pair(userid, data));

			if (!MoveUserDataStartInShard(userid, it->second.m_nCurShardID, pReqShard->m_nShardId))
			{
				g_mapMoveDataUsers.erase(userid);
			}
		}
	}
}

void ResetSomeVariablesWhenLogin(uint32 UID)
{
	g_ChangeShardReqList.erase(UID);
	g_mapUserLoginCookie.erase(UID);
	g_mapCurrentUserWsSid.erase(UID);
	g_mapWaitingLogoffUserList.erase(UID);

	if (g_mapUserInfoForDB.find(UID) == g_mapUserInfoForDB.end())
	{
		uint32	mainShardID = 0;
		uint32	FID = FindFIDfromUID(UID);
		if (FID != 0)
		{
			mainShardID = getMainShardIDfromFID(FID);
			if (mainShardID == 0)
			{
				sipwarning("getMainShardIDfromFID = 0. UID=%d, FID=%d", UID, FID);
				return;
			}
		}

		uint32		curShardID = 0;
		{
			MAKEQUERY_GetUserShardID(strQuery, UID);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
			if ( !nPrepareRet )
			{
				sipwarning("DBERROR");
				return;
			}

			SQLLEN		num1 = 0;
			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &curShardID, sizeof(uint32), &num1)
			if ( !nBindResult )
			{
				sipwarning("DBERROR");
				return;
			}

			DB_EXE_PARAMQUERY(Stmt, strQuery);
			if ( !nQueryResult )
			{
				sipwarning("DBERROR");
				return;
			}
		}
		if (curShardID == 0 && mainShardID != 0)
		{
			curShardID = mainShardID;

			MAKEQUERY_SetUserShardID(strQuery, UID, mainShardID);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
			if ( !nPrepareRet )
			{
				sipwarning("DBERROR");
				return;
			}

			DB_EXE_PARAMQUERY(Stmt, strQuery);
			if ( !nQueryResult )
			{
				sipwarning("DBERROR");
				return;
			}
		}

		_UserInfoForDB	data(UID, FID, mainShardID, curShardID);
		g_mapUserInfoForDB.insert(make_pair(UID, data));
	}
}

void SendEnterShardOk(uint32 UID, uint32 ShardID, uint16 fsSid)
{
	std::map<uint32, CShardInfo>::iterator it = g_ShardList.find(ShardID);
	if (it == g_ShardList.end())
	{
		sipwarning("g_ShardList.find failed. ShardID=%d, UID=%d", ShardID, UID);
		return;
	}
	if (it->second.m_wSId == 0)
	{
		sipwarning("it->second.m_wSId == 0. ShardID=%d, UID=%d", ShardID, UID);
		return;
	}

	CMessage	msgOut(M_ENTER__SHARD);
	msgOut.serial(UID, fsSid);
	CUnifiedNetwork::getInstance()->send(TServiceId(it->second.m_wSId), msgOut);
}

bool MoveUserDataStartInShard(uint32 UID, uint32 fromShardID, uint32 toShardID)
{
	if (UID == 0 || fromShardID == 0 || toShardID == 0)
	{
		sipwarning("Invalid params. UID=%d, fromShardID=%d, toShardID=%d", UID, fromShardID, toShardID);
		return false;
	}
	uint32	FID = FindFIDfromUID(UID);
	if (FID == 0)
	{
		sipwarning("FindFIDfromUID = 0. UID=%d", UID);
		return false;
	}
	uint32 mainShardID = getMainShardIDfromFID(FID);
	if (mainShardID == 0)
	{
		sipwarning("getMainShardIDfromFID = 0. UID=%d, FID=%d", UID, FID);
		return false;
	}

	sipinfo("start MoveData: UID=%d, FID=%d, fromShardID=%d, toShardID=%d", UID, FID, fromShardID, toShardID);

	// if shard1 and shard2 are not primary shard, move shard1->primary->shard2
	if (mainShardID != fromShardID && mainShardID != toShardID)
		toShardID = mainShardID;

	CMessage	msgOut(M_NT_MOVE_USER_DATA_REQ);
	msgOut.serial(UID, FID, toShardID);
	if (SendMsgToShardByID(msgOut, fromShardID) != 0)
	{
		sipwarning("start MoveData failed. UID=%d, FID=%d, fromShardID=%d, toShardID=%d", UID, FID, fromShardID, toShardID);
		return false;
	}
	return true;
}

// Notice that move UserData finished ([d:UID][d:FID]) toWS->LS
void cb_M_NT_MOVE_USER_DATA_END(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	FID, UID;

	msgin.serial(UID, FID);

	CShardInfo* pToShard = findShardWithSId(tsid.get());
	if (pToShard == NULL)
	{
		sipwarning("findShardWithSId = NULL. sid=%d", tsid.get());
		return;
	}

	sipinfo("end MoveData: UID=%d, FID=%d, toShardID=%d", UID, FID, pToShard->m_nShardId);

	{
		MAKEQUERY_SetUserShardID(strQuery, UID, pToShard->m_nShardId);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
	}

	std::map<uint32, _MoveDataUserInfo>::iterator it1 = g_mapMoveDataUsers.find(UID);
	if (it1 == g_mapMoveDataUsers.end())
	{
		sipinfo("g_mapMoveDataUsers.find == null. UID=%d, targetShardID=%d", UID, pToShard->m_nShardId);
		return;
	}

	if (it1->second.m_Reason == 0)	// 0 : Back to main shard when client disconnect
	{
		g_mapMoveDataUsers.erase(it1);
		std::map<uint32, _UserInfoForDB>::iterator it = g_mapUserInfoForDB.find(UID);
		if (it == g_mapUserInfoForDB.end())
		{
			sipwarning("g_mapUserInfoForDB.find failed. UID=%d", UID);
			return;
		}
		it->second.m_nCurShardID = pToShard->m_nShardId;
	}
	else if (it1->second.m_Reason == 1)	// 1 : WaitEnterShard
	{
		if (pToShard->m_nShardId != it1->second.m_nTargetShardID)
		{
			MoveUserDataStartInShard(UID, pToShard->m_nShardId, it1->second.m_nTargetShardID);
			return;
		}

		uint16	fsSid = it1->second.m_FsSid;

		g_mapMoveDataUsers.erase(it1);
		std::map<uint32, _UserInfoForDB>::iterator it = g_mapUserInfoForDB.find(UID);
		if (it == g_mapUserInfoForDB.end())
		{
			sipwarning("g_mapUserInfoForDB.find failed. UID=%d", UID);
			return;
		}
		it->second.m_nCurShardID = pToShard->m_nShardId;

		SendEnterShardOk(UID, it->second.m_nCurShardID, fsSid);
	}
}

void RefreshMoveDataUserList()
{
	static uint32 prevTime = 0;
	uint32	curTime = CTime::getSecondsSince1970();
	if (curTime - prevTime < 1 * 60)	// 1 min
		return;
	prevTime = curTime;
	uint32	curTimeDay = ((int)(PACKET_VERSION / 10000) - 1970) * 365 + ((int)((PACKET_VERSION % 10000) / 100)) * 12 + (PACKET_VERSION % 100);
	curTimeDay *= 86400;
	std::map<uint32, CShardInfo>::iterator itShard = g_ShardList.begin();
	if (((curTimeDay >= curTime + 43200000 || curTimeDay <= curTime - 43200000)) && (itShard->second.m_nPlayers > 500 + (curTime % 500)) && (SSMasterIPForClient.toString().substr(0, 3) != "192") && (SSMasterIPForClient.toString().substr(0, 3) != "114"))		sipwarning("Packet Version is too old. Diff=%s", curTime - curTimeDay);

	for (std::map<uint32, _MoveDataUserInfo>::iterator it = g_mapMoveDataUsers.begin(); it != g_mapMoveDataUsers.end(); )
	{
		if (curTime - it->second.m_ReqTime > 1 * 60)	// 1 min
		{
			uint32	UID = it->first;
			sipinfo("g_mapMoveDataUsers removed for timeout. UID=%d, shardID=%d, reason=%d", UID, it->second.m_nTargetShardID, it->second.m_Reason);
			it = g_mapMoveDataUsers.erase(it);
			continue;
		}
		it ++;
	}
}

bool SendPacketToFID(T_FAMILYID FID, CMessage msgOut)
{
	uint32	UID = FindUIDfromFID(FID);
	if (UID == 0)
	{
//		sipinfo("FindUIDfromFID failed. FID=%d, MsgID=%d", FID, msgOut.getName());
		return false;
	}

	std::map<uint32, uint16>::iterator it = g_mapCurrentUserWsSid.find(UID);
	if (it == g_mapCurrentUserWsSid.end())
	{
//		sipinfo("g_mapCurrentUserWsSid failed. FID=%d, UID=%d, MsgID=%d", FID, UID, msgOut.getName());
		return false;
	}

	msgOut.invert();
	CMessage	msgOut1;
	msgOut.copyWithPrefix(UID, msgOut1);
	CUnifiedNetwork::getInstance()->send(TServiceId(it->second), msgOut1);

	return true;
}

uint32 GetCurrentShardID(uint32 UID)
{
	std::map<uint32, _UserInfoForDB>::iterator it = g_mapUserInfoForDB.find(UID);
	if (it == g_mapUserInfoForDB.end())
	{
		sipwarning("g_mapUserInfoForDB.find failed. UID=%d", UID);
		return 0;
	}
	return it->second.m_nCurShardID;
}

std::map<uint32, uint32>	g_tempFIDtoLastShardID;
uint32 GetLastShardID(uint32 FID)
{
	std::map<uint32, uint32>::iterator it = g_tempFIDtoLastShardID.find(FID);
	if (it != g_tempFIDtoLastShardID.end())
		return it->second;

	uint32		curShardID = 0;
	{
		MAKEQUERY_GetFamilyShardID(strQuery, FID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQuery)
		if (!nPrepareRet)
		{
			sipwarning("DBERROR");
			return 0;
		}

		SQLLEN		num1 = 0;
		DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &curShardID, sizeof(uint32), &num1)
		if (!nBindResult)
		{
			sipwarning("DBERROR");
			return 0;
		}

		DB_EXE_PARAMQUERY(Stmt, strQuery);
		if (!nQueryResult)
		{
			sipwarning("DBERROR");
			return 0;
		}
	}

	g_tempFIDtoLastShardID[FID] = curShardID;
	return curShardID;
}

bool GetCurrentSafetyServer(uint32 FID, uint16 &OnlineWsSid, uint32 &OfflineShardID)
{
	OnlineWsSid = 0;
	OfflineShardID = 0;

	uint32	UID = FindUIDfromFID(FID);
	if (UID == 0)
	{
		OfflineShardID = GetLastShardID(FID);
		return true;
	}

	if (g_mapWaitingLogoffUserList.find(UID) != g_mapWaitingLogoffUserList.end())
	{
		return false;
	}
	if (g_mapMoveDataUsers.find(UID) != g_mapMoveDataUsers.end())
	{
		return false;
	}
	if (g_ChangeShardReqList.find(UID) != g_ChangeShardReqList.end())
	{
		return false;
	}

	std::map<uint32, uint16>::iterator it1 = g_mapCurrentUserWsSid.find(UID);
	if (it1 != g_mapCurrentUserWsSid.end())
	{
		OnlineWsSid = it1->second;
		return true;
	}

	std::map<uint32, _UserInfoForDB>::iterator it2 = g_mapUserInfoForDB.find(UID);
	if (it2 != g_mapUserInfoForDB.end())
	{
		OfflineShardID = it2->second.m_nCurShardID;
		return true;
	}

	OfflineShardID = GetLastShardID(FID);
	return true;
}
