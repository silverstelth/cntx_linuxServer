/** \file login_service.h
 * <File description>
 *
 * $Id: login_service.h,v 1.38 2016/09/08 01:17:09 RyuSangChol Exp $
 */


#ifndef SIP_LOGIN_SERVICE_H
#define SIP_LOGIN_SERVICE_H

// we have to include windows.h because mysql.h uses it but not include it
#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "misc/types_sip.h"

#include "misc/debug.h"
#include "misc/config_file.h"
#include "misc/displayer.h"
#include "misc/log.h"
#include "misc/DBCaller.h"

#include "net/service.h"
#include "../Common/DBData.h"


// Constantes

class	CStartServiceInfo
{
public:
	CStartServiceInfo() : Name(""), SId (0), Addr("") { };
	CStartServiceInfo(std::string n, SIPNET::TServiceId s, SIPNET::CInetAddress &a) : Name(n), SId (s), Addr(a) { };
	
	// name of the service
	std::string				Name;
	// id of the service
	SIPNET::TServiceId		SId;
	// address to send to the service who wants to lookup this service (could have more than one)
	SIPNET::CInetAddress	Addr;
};

typedef std::list<CStartServiceInfo>		_SSLIST;
typedef std::list<CStartServiceInfo>::iterator	_SSLISTIT;

// Structures
// uint32 is the hostid to the StartService connection
class CShardCookie
{
public:
	CShardCookie()
	{};
	CShardCookie(SIPNET::CLoginCookie & cookie, uint32	shardId)
	{
		_cookie.set(cookie.getUserAddr(), cookie.generateKey(), cookie.getUserId(), cookie.getUserType(), cookie.getUserName(), cookie.getVisitDays(), cookie.getUserExp(), cookie.getUserVirtue(), cookie.getUserFame(), cookie.getExpRoom(), cookie.getIsGM(), cookie.getAddJifen());
		_shardId = shardId;
	};
	SIPNET::CLoginCookie &	Cookie() { return	_cookie; };
	uint32	ShardId() { return	_shardId; };

protected:
	SIPNET::CLoginCookie	_cookie;
	uint32			_shardId;
};

// Variables
extern SIPBASE::CLog*				Output;
// uint32 is the hostid to the StartService connection
extern std::map<uint32, CShardCookie> TempCookies;
extern std::map<uint32, uint32> AbnormalLogoffUsers;

extern SIPBASE::CVariable<uint8>	ServerBuildComplete;
extern SIPBASE::CVariable<uint>		AcceptExternalShards;
extern SIPBASE::CVariable<std::string>	ForceDatabaseReconnection;

extern	SIPBASE::CDBCaller *MainDB;
//extern	DBCONNECTION	MainDBConnection;
extern	SIPBASE::CDBCaller *UserDB;

typedef		std::multimap<uint32, _entryShardState>		TSHARDSTATETBL;
typedef		std::multimap<uint32, _entryShardState>::iterator		TSHARDSTATETBLIT;
//extern TSHARDSTATETBL				tblShardState;
//extern std::map<uint32, uint16>      ShardIdVSSId;

//typedef		std::multimap<uint32, uint32>	TSHARDVIPS;
//typedef		std::multimap<uint32, uint32>::iterator		TSHARDVIPSIT;
//extern TSHARDVIPS					tblShardVIPs;

//typedef std::map<uint32, std::vector<LobbyInfo> > TSHARDLOBBY;
//typedef std::map<uint32, std::vector<LobbyInfo> >::iterator TSHARDLOBBYIT;
//extern TSHARDLOBBY tblShardLobby;

//typedef std::map<uint32, _entryShard> TSHARDTBL;
//typedef std::map<uint32, _entryShard>::iterator TSHARDTBLIT;
//extern TSHARDTBL g_tblShardList;
//extern std::vector<CShard>			g_Shards;

struct CShardInfo
{
public:
	uint32			m_nShardId;
	ucstring		m_uShardName;
	uint8			m_bZoneId;
	uint32			m_nUserLimit;
	ucstring		m_uContent;
	uint8			m_bMainCode;
	ucstring		m_uShardCodeList;

	uint8			m_bUserTypeId;
	uint8			m_bActiveFlag;
	uint8			m_bStateId;
	std::string		m_sActiveShardIP;
	std::string		m_sServiceName;
	uint32			m_nPlayers;
	uint16			m_wSId;
	uint8			m_bIsDefaultShard;

	CShardInfo()
	{
		init();

		m_nShardId		= 0;
		m_uShardName	= L"";
		m_bZoneId		= 0;
		m_nUserLimit	= 0;
		m_uContent	= L"";
		m_bMainCode = 0;
		m_uShardCodeList = L"";
		m_bIsDefaultShard = 0;
	}

	CShardInfo(uint32 _shardId, ucstring _shardName, uint8 _zoneId, uint32 _userLimit, ucstring _content, uint8 _mainCode, ucstring _shardCodeList, uint8 _defaultFlag)
	{
		init();

		m_nShardId		= _shardId;
		m_uShardName	= _shardName;
		m_bZoneId		= _zoneId;
		m_nUserLimit	= _userLimit;
		m_uContent		= _content;
		m_bMainCode		= _mainCode;
		m_uShardCodeList = _shardCodeList;
		m_bIsDefaultShard = _defaultFlag;
	}

	void init()
	{
		m_bUserTypeId	= USERTYPE_COMMON;
		m_bActiveFlag	= SHARD_OFF;
		m_bStateId		= 0;
		m_sActiveShardIP = "";
		m_sServiceName = "";
		m_nPlayers = 0;
		m_wSId = 0;
	}

	void	SetActiveShardIP(std::string strIP, std::string strServiceName) { m_sActiveShardIP = strIP; m_sServiceName = strServiceName; }
};
extern	std::map<uint32, CShardInfo>	g_ShardList;
extern	std::map<uint8, uint32>		g_mapShardCodeToShardID;


// Functions

void displayShards ();
void displayUsers ();
void beep (uint freq = 400, uint nb = 2, uint beepDuration = 100, uint pauseDuration = 100);

//void LoadDBSharedState();
//void LoadDBSharedVIPs();
void LoadDBShardList();

//sint32 findShard (uint32 shardId);

extern void	SendShardListToSS(uint32 userid, uint32 fid, uint16 sid = 0);
extern T_ERR	CanEnterShard(CShardInfo *pShardInfo, uint32 userid, uint8 uUserPriv);

extern uint32 getMainShardIDfromFID(uint32 FID);
extern uint32 getMainShardIDfromUID(uint32 UID);

extern void AddUIDtoFIDmap(uint32 userid, uint32 fid);
extern uint32 FindFIDfromUID(uint32 userid);
extern uint32 FindUIDfromFID(uint32 fid);

extern uint32	g_nDefaultShardID;

#endif // SIP_LOGIN_SERVICE_H

/* End of login_service.h */
