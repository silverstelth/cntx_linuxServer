#include "misc/types_sip.h"

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <vector>
#include <map>

#include "misc/debug.h"
#include "misc/config_file.h"
#include "misc/displayer.h"
#include "misc/log.h"

#include "net/buf_server.h"
#include "net/login_cookie.h"

#include "login_service.h"

#include "../../common/err.h"
#include "../../common/Packet.h"
#include "../Common/netCommon.h"
#include "../Common/QuerySetForMain.h"

//
// Namespaces
//

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

#include "connection_ws.h"
#include "players.h"

//bool	g_bMasterBuild = false;

extern void SetUserLoginCookie(uint32 UID, CLoginCookie &cookie);

CStartServiceInfo	g_masterSS;
_SSLIST				g_slaveSSList;

//bool	IsShardMember(uint32 shardId, uint32 userid)
//{
//	bool	bShardMember = false;
//	pair<TSHARDVIPSIT, TSHARDVIPSIT> range = tblShardVIPs.equal_range(shardId);
//	TSHARDVIPSIT	it;
//	for ( it = range.first; it != range.second; it ++ )
//	{
//		uint32	VIPUserId = (*it).second;
//		if ( userid == VIPUserId )
//		{
//            bShardMember = true;
//			break;
//		}
//	}
//
//	return	bShardMember;
//}

void	cbSSMasterIdentify(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cbSSChooseShard(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cbSSShardsList(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cbDisconnectClient(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_NT_REQ_FORCE_LOGOFF(CMessage &msgin, const std::string &serviceName, TServiceId tsid);
void	cb_M_CS_CHECK_FASTENTERROOM(CMessage &msgin, const std::string &serviceName, TServiceId tsid);


TUnifiedCallbackItem SSCallbackArray[] =
{
//	{ M_SS_MASTER,			cbSSMasterIdentify },
	{ M_ENTER_SHARD_REQUEST,		cbSSChooseShard },
	{ M_SC_SHARDS,			cbSSShardsList },
	{ M_CLIENT_DISCONNECT,	cbDisconnectClient },
	{ M_NT_REQ_FORCE_LOGOFF,	cb_M_NT_REQ_FORCE_LOGOFF },
	{ M_CS_CHECK_FASTENTERROOM,	cb_M_CS_CHECK_FASTENTERROOM }
};


// packet : M_ENTER_SHARD_REQUEST
// SS -> LS
void cbSSChooseShard(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32	userid;			msgin.serial(userid);
#ifdef SIP_OS_UNIX
	uint64	sockId;		
					msgin.serial((uint64 &)sockId);
#else
	uint32	sockId;		
					msgin.serial((uint32 &)sockId);
#endif
	uint32	shardId;		msgin.serial(shardId);
	uint8	uUserPriv;		msgin.serial(uUserPriv);
	ucstring	ucLogin;	msgin.serial(ucLogin);
	uint8		bIsGM;		msgin.serial(bIsGM);

	uint32	visitDays = 0;
	uint16	userExp= 0;
	uint16	userVirtue = 0;
	uint16	userFame = 0;
	uint8	userRoomExp = 0;

	std::map<uint32, CShardInfo>::iterator it = g_ShardList.find(shardId);
	if (it != g_ShardList.end() && it->second.m_wSId != 0)
	{
		CShardInfo	*pShardInfo = &(it->second);
		T_ERR	errNo = CanEnterShard(pShardInfo, userid, uUserPriv);
		if (errNo != ERR_NOERR)
		{
			sipwarning("Can't Enter Shard:%u, UserId:%u, User Permission:%u, (errNo : %u)", shardId, userid, uUserPriv, errNo);
			SendFailMessageToServer(errNo, tsid, userid);
			return;
		}
		// get addedJifen
		uint32	AddedJifen = 0;
//			if (!IsSiteActivityDuration())  이 코드는 이미전부터 리용하지 않았다. 아래의 조건문을 삭제하는 경우에도 이것을 열어놓지 말것.  by krs
		if (0) // by krs
		{
			MAKEQUERY_GetAddedJifen(strQuery, userid);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_QUERY( DB, Stmt, strQuery );
			if (nQueryResult == true)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT(row, 0, uint32, addpoint);
						AddedJifen += addpoint;
					}
					else
						break;
				} while (true);
			}
		}
		if (AddedJifen > 0)
		{
			MAKEQUERY_UpdateAddedJifen(strQuery, userid);
			MainDB->execute(strQuery);
		}
		
		// generate a cookie
		std::string	sUserName = ucLogin.toString();//   SIPBASE::toString("%s", ucLogin.c_str());
		CLoginCookie Cookie(sockId, userid, uUserPriv, sUserName, visitDays, userExp, userVirtue, userFame, userRoomExp, bIsGM, AddedJifen);

		// send message to the welcome service to see if it s ok and know the front end ip
		CMessage msgout(M_ENTER_SHARD_REQUEST);
		msgout.serial(Cookie);
		msgout.serial(uUserPriv);
		CUnifiedNetwork::getInstance()->send (TServiceId(pShardInfo->m_wSId), msgout);
		//sipdebug("Send enter shard : userid=%d, cookie=%s, shardid=%d", userid, Cookie.toString().c_str(), shardId); byy krs
//		beep(1000, 1, 100, 100); ----- what is this???????

		CLoginCookie Cookie1(sockId, userid, uUserPriv, sUserName, visitDays, userExp, userVirtue, userFame, userRoomExp, bIsGM, 0);
		SetUserLoginCookie(userid, Cookie1);

		// add the connection in temp cookie
		TempCookies.insert(make_pair(Cookie.getUserId(), CShardCookie(Cookie, shardId)));
		return;
	}

	// the shard is not available, denied the user
	//sipwarning("ShardId %d is not available, can't add the userID %u", shardId, userid); byy krs
	SendFailMessageToServer(ERR_IMPROPERSHARD, tsid, userid);
}

// packet : M_CLIENT_DISCONNECT
// SS -> LS
void cbDisconnectClient (CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32 userId;		msgin.serial(userId);
	uint32 shardId;
    if (TempCookies.find(userId) == TempCookies.end())
		return;

	shardId = TempCookies[userId].ShardId();
	//sipinfo ("<DC> StartService wants to disconnect userid %d, send request to the shard %d", userId, shardId); byy krs
	std::map<uint32, CShardInfo>::iterator it = g_ShardList.find(shardId);
	if (it != g_ShardList.end() && it->second.m_wSId != 0)
	{
		// ask the WS to disconnect the player from the shard
		CMessage msgout (M_CLIENT_DISCONNECT);
		msgout.serial(userId);
		CUnifiedNetwork::getInstance ()->send (TServiceId(it->second.m_wSId), msgout);
		//sipinfo("<DC> Disconnect User ShardId:%u, UserId:%u", shardId, userId); byy krs
		return;
	}

	//sipwarning("ShardId %d is not available, can't disconnect the UID=%d", shardId, userId); byy krs

	//CMessage msgout(M_CLIENT_DISCONNECT);
	//uint32 fake = 0;
	//msgout.serial(fake);
	//string reason = "ShardId "+toStringA(shardId)+"is not available, can't disconnect the userid"+toStringA(userId);
	//msgout.serial (reason);
	//CUnifiedNetwork::getInstance()->send(LS_S_NAME, msgout);
}

// Packet : M_SC_SHARDS
// SS -> LS
void cbSSShardsList(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32		userid, fid;
	msgin.serial(userid, fid);

	SendShardListToSS(userid, fid, tsid.get());
}

// Request Force Logoff by double login, SS->LS->WS
void cb_M_NT_REQ_FORCE_LOGOFF(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32		userid;		msgin.serial(userid);
	uint32		shardid;	msgin.serial(shardid);
	uint8		bIsGM;		msgin.serial(bIsGM);

	// shardid绰 瓜阑巴捞 给等促!! 促矫 沥犬茄 shardid甫 棱酒具 茄促!!! 弊矾唱 shard捞悼吝牢 版快 殿 咯矾啊瘤 版快甫 绊妨秦具 茄促.
	shardid = GetCurrentShardID(userid);

	AbnormalLogoffUsers.insert(std::make_pair(userid, shardid));

	CMessage	msgout(M_NT_REQ_FORCE_LOGOFF);
	msgout.serial(userid);
	msgout.serial(bIsGM);

	// SS俊 嘿绢乐绰 版快 (Shard甫 急琶窍瘤 臼篮 版快) SS甸俊 烹瘤窍咯 贸府窍霸 茄促. - SS啊 咯矾俺 乐绰 版快 鞘夸
//	if(!bIsGM)
	{
		uint32	gentime;	msgin.serial(gentime);
		msgout.serial(gentime);
		CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgout);
	}

	std::map<uint32, CShardInfo>::iterator it = g_ShardList.find(shardid);
	if (it == g_ShardList.end() || it->second.m_wSId == 0)
	{
		sipinfo("Invalid ShardId. UID=%d, shardid=%d, bIsGM=%d", userid, shardid, bIsGM);
		return;
	}

	uint16		sid = it->second.m_wSId;
	TServiceId	serverId;	serverId.set(sid);
	CUnifiedNetwork::getInstance()->send(serverId, msgout);

	//sipdebug("Send Shard Information (M_NT_REQ_FORCE_LOGOFF), UID=%d, shardid=%d", userid, shardid); byy krs
}

// Request to Check Enter Room ([d:RoomID])
void cb_M_CS_CHECK_FASTENTERROOM(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint32		UserAddr;	msgin.serial(UserAddr);
	uint32		UserID;		msgin.serial(UserID);
	uint32		RoomID;		msgin.serial(RoomID);

	//sipdebug("M_CS_CHECK_FASTENTERROOM received. UserAddr=%d, UserID=%d, RoomID=%d", UserAddr, UserID, RoomID); byy krs

	uint8		ShardCode = (uint8)(RoomID >> 24);
	uint32		ShardID = 0;

	std::map<uint8, uint32>::iterator	it = g_mapShardCodeToShardID.find(ShardCode);
	if(it == g_mapShardCodeToShardID.end())
	{
		uint32		err = ERR_INVALIDROOMID;
		CMessage	msgout(M_SC_CHECK_FASTENTERROOM);
		msgout.serial(UserAddr, err, RoomID, ShardID);
		CUnifiedNetwork::getInstance()->send(tsid, msgout);
		//sipdebug("M_SC_CHECK_FASTENTERROOM send. UserAddr=%d, err=%d, RoomID=%d, ShardID=%d", UserAddr, err, RoomID, ShardID); byy krs
		return;
	}

	ShardID = it->second;
	std::map<uint32, CShardInfo>::iterator it1 = g_ShardList.find(ShardID);
	if (it1 != g_ShardList.end() && it1->second.m_wSId != 0)
	{
		uint16		ss_sid = tsid.get();
		CMessage	msgout(M_CS_CHECK_FASTENTERROOM);
		msgout.serial(ss_sid, UserAddr, UserID, RoomID);
		CUnifiedNetwork::getInstance()->send(TServiceId(it1->second.m_wSId), msgout);
		return;
	}
	//sipwarning("Can't find Shard ServiceID!!! ShardID=%d", ShardID); byy krs
	return;
}

// Check Enter Room Result ([d:ErrorCode][d:RoomID][d:ShardID])
void cb_M_SC_CHECK_FASTENTERROOM (CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16		ss_sid;
	CMessage	msgout;
	msgin.copyWithoutPrefix(ss_sid, msgout);
	TServiceId	ss(ss_sid);
	CUnifiedNetwork::getInstance()->send(ss, msgout);
	//sipdebug("M_SC_CHECK_FASTENTERROOM send."); byy krs
}

//
// Functions
//
void cbSSConnect(const std::string &serviceName, TServiceId sid, void *arg)
{
/*
	ServerBuildComplete = 1;
	sipdebug("SS is connected, ServiceID %u", sid.get());

	TSockId from;
	CCallbackNetBase *cnb = CUnifiedNetwork::getInstance ()->getNetBase(sid, from);
	const CInetAddress &addr = cnb->hostAddress(from);

	string masterSSAddr = IService::getInstance()->ConfigFile.getVar("SSLoginHost").asString();

	CMessage	msgout;
	if ( addr.ipAddress() == masterSSAddr )
	{
		msgout.setType(M_SS_MASTER);

		g_masterSS.Name	= serviceName;
		g_masterSS.SId	= sid;
		g_masterSS.Addr = addr;

		sipinfo("MASTER started! name :%s, ServiceId : %d, Address : %s", g_masterSS.Name.c_str(), g_masterSS.SId.get(), g_masterSS.Addr.asIPString().c_str());
	}
	else
	{
		msgout.setType(M_SS_SLAVE);

		// g_masterSS 蔼甸捞 檬扁拳登瘤 臼篮 版快档 乐阑巴 鞍篮单???????????
		// 促矫富窍搁 Master SS啊 焊辰 M_SS_MASTER 甫 酒流 LS啊 贸府窍瘤 给茄 版快俊 绢痘霸 瞪啊????????
		std::string				masterName	= g_masterSS.Name;		msgout.serial(masterName);
		SIPNET::TServiceId		masterSid	= g_masterSS.SId;		msgout.serial(masterSid);
		SIPNET::CInetAddress	masterAddr	= g_masterSS.Addr;		msgout.serial(masterAddr);

		CStartServiceInfo	slave(masterName, masterSid, masterAddr);
		g_slaveSSList.push_back(slave);

		sipinfo("SLAVE started! name :%s, ServiceId : %d", serviceName.c_str(), sid.get());
		sipinfo("send MASTER Info! mastername :%s, masterServiceId : %d, masterAddress : %s", g_masterSS.Name.c_str(), g_masterSS.SId.get(), g_masterSS.Addr.asIPString().c_str());
	}

	CUnifiedNetwork::getInstance()->send(sid, msgout);
	*/
}

// packet : M_SS_MASTER
// Master SS -> LS
void	cbSSMasterIdentify(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
/*
	SIPNET::CInetAddress	addr;	msgin.serial(addr);
	g_masterSS.Addr	= addr;
*/
}

void cbSSDisconnect(const std::string &serviceName, TServiceId sid, void *arg)
{
/*
	ServerBuildComplete = 0;

	if ( serviceName == g_masterSS.Name &&
		sid == g_masterSS.SId )
	{
//		g_bMasterBuild = false;
	}
	else
	{
		for ( _SSLISTIT it = g_slaveSSList.begin(); it != g_slaveSSList.end(); it ++ )
		{
			if ( (*it).Name == serviceName && (*it).SId == sid )
				g_slaveSSList.erase(it);
		}
	}
	sipdebug("SS is disconnected, ServiceID %u", sid.get());
	*/
}

void connectionSSInit ()
{
	// catch the messages from Welcome Service to know if the user can connect or not
	CUnifiedNetwork::getInstance ()->addCallbackArray (SSCallbackArray, sizeof(SSCallbackArray)/sizeof(SSCallbackArray[0]));
	CUnifiedNetwork::getInstance()->setServiceUpCallback(SS_S_NAME, cbSSConnect);
	CUnifiedNetwork::getInstance()->setServiceDownCallback(SS_S_NAME, cbSSDisconnect);
}

void connectionSSUpdate ()
{

}

void connectionSSRelease ()
{
}
