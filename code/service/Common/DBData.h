#ifndef TIAN_SERVER_DBDATA_H
#define TIAN_SERVER_DBDATA_H

#include <misc/types_sip.h>

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "misc/ucstring.h"
#include "misc/stream.h"

#include "../../common/common.h"
#include "../Common/Common.h"

extern	ucstring	ucErrAry[];


#define		UCFROMUTF8(str)			ucstring::makeFromUtf8(str)
#define		ERR_SHOW(str, err)		sipwarning("ERROR : %s, Result(err %u): " #err, str, err)
#define		RESULT_SHOW(str, var)	sipdebug("RESULT : %s, Result(no %u): " #var, str, var)

#define		SAFE_SERIAL_STR(msgin, str, nMaxLen, idUser)	\
	msgin.serial(str);	\
	if ( str == "" )	\
	{	\
		ucstring ucUnlawContent = SIPBASE::_toString(#str" in Message <%s> is NULL String , FID=%u", msgin.toString().c_str(), idUser).c_str();	\
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idUser);	\
		return;	\
	}	\
	if ( str.size() > nMaxLen )	\
	{	\
		ucstring ucUnlawContent = SIPBASE::_toString(#str" in Message <%s> len(%u) overflow maxlimit %u, FID=%u", msgin.toString().c_str(), str.size(), nMaxLen, idUser).c_str();	\
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idUser);	\
		return;	\
	}	\

#define		SAFE_SERIAL_WSTR(msgin, str, nMaxLen, idUser)	\
	msgin.serial(str);	\
	if ( str == ucstring("") )	\
	{	\
		ucstring ucUnlawContent = SIPBASE::_toString(#str" in Message <%s> is NULL String , FID=%u", msgin.toString().c_str(), idUser).c_str();	\
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idUser);	\
		return;	\
	}	\
	if ( str.size() > nMaxLen )	\
	{	\
		ucstring ucUnlawContent = SIPBASE::_toString(#str" in Message <%s> len(%u) overflow maxlimit %u, FID=%u", msgin.toString().c_str(), str.size(), nMaxLen, idUser).c_str();	\
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idUser);	\
		return;	\
	}	\

#define		NULLSAFE_SERIAL_STR(msgin, str, nMaxLen, idUser)	\
	msgin.serial(str);	\
	if ( str.size() > nMaxLen )	\
	{	\
		ucstring ucUnlawContent = SIPBASE::_toString(#str" in Message <%s> len(%u) overflow maxlimit %u, FID=%u", msgin.toString().c_str(), str.size(), nMaxLen, idUser).c_str();	\
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idUser);	\
		return;	\
	}	\

#define		NULLSAFE_SERIAL_WSTR(msgin, str, nMaxLen, idUser)	\
	msgin.serial(str);	\
	if ( str.size() > nMaxLen )	\
	{	\
		ucstring ucUnlawContent = SIPBASE::_toString(#str" in Message <%s> len(%u) overflow maxlimit %u, FID=%u", msgin.toString().c_str(), str.size(), nMaxLen, idUser).c_str();	\
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, idUser);	\
		return;	\
	}	\

/************************************************************************/
/*								Admin Site DB					        */
/************************************************************************/

// Server DBCall Type

#define		MAX_SQL_LEN			1024
#define		MAX_NAME_LEN		256
#define		MAX_CONTENT_LEN		1024
#define		MAX_SHORT_TEXT_LEN	16
#define		MAX_ADDR_LEN		64

#define		UCtoUTF8(ucstr)		( ucstr.toUtf8() )
#define		UTF8toUC(str)		( ucstring::makeFromUtf8(str) )


enum ENUM_FTPSERVER	{ FTP_MAIN, FTP_LOCAL, FTP_NONE };


/************************************************************************/
/*								tblService		                        */
/************************************************************************/
#define DB_TBLSERVICE				"tblService"
#define DB_SERVICE_ID				"Id"	
#define DB_SERVICE_ALIASNAME		"AliasName"
#define DB_SERVICE_SERVERID			"ServerId"	
#define DB_SERVICE_SHARDID			"ShardId"	

struct _entryService
{
	uint16			uId;
	uint16			uServiceId;
	uint16			uServerId;
	uint16			uShardId;
};

struct _entryRegService
{
	uint16			uId;
	ucstring		ucAlias;
	ucstring		ucExe;
	ucstring		ucShort;
	ucstring		ucLong;
};

/************************************************************************/
/*								tblServer		                        */
/************************************************************************/

#define DB_TBLSERVER				"tblServer"
#define DB_SERVER_ID				"ServerId"
#define DB_SERVER_ALIASNAME			"AliasName"
#define DB_SERVER_HOST				"Host"
#define DB_SERVER_ADRESS			"Address"
#define DB_SERVER_SHARDID			"ShardId"

enum ENUMTBL_SERVER
{
	SERVER_UID,
	SERVER_ALIASNAME,
	SERVER_HOSTNAME,
	SERVER_ADRESS,
	SERVER_SHARDID
};

struct _entryServer
{
	_entryServer():
			uId(0), aliasName(""), hostName(""), adress(""), shardID(0)
			{};

	_entryServer(uint16 uId, std::string &aliasName, std::string &hostName, std::string &adress, uint8 shardID) :
			uId(uId), aliasName(aliasName), hostName(hostName), adress(adress), shardID(shardID)
			{};

	uint16			uId;
	ucstring		aliasName;
	std::string		hostName;
	std::string		adress;
	uint32			shardID;
};

/************************************************************************/
/*								tblUser		                        */
/************************************************************************/

#define DB_TBLUSER					"tblUser"
#define DB_USER_DBID				"UId"
#define DB_USER_ACCOUNTID			"LoginID"
#define DB_USER_PASSOWRD			"Password"
#define DB_USER_SHARDID				"ShardId"
#define DB_USER_STATE				"State"
#define DB_USER_PRIVILEGE			"Privilege"
#define DB_USER_PRESERVE			"UserExtension"
#define DB_USER_CLIENTAPP			"ClientApp"

enum ENUMTBL_USER
{
	USER_DBID,
	USER_USERID,
	USER_PASSWORD,
	USER_STATE,
	USER_SHARDID,
	USER_PRIVILEGE,
};

struct _entryUser
{
	uint32			dbID;
	ucstring		ucLoginID;
	std::string		sPassword;
	uint			shardID;
	ENUM_USERSTATE	state;
	ENUM_USERTYPE	privilege;
	uint32			GMoney;
	uint32			Money;
	uint16			NTamping;
};


/************************************************************************/
/*								tblShard		                        */
/************************************************************************/

//#define 	DB_TBLSHARD						"tblShard"
//#define 	DB_SHARD_UID					"UId"
//#define 	DB_SHARD_SHARDID				"ShardId"
//#define 	DB_SHARD_NAME					"Name"
//#define 	DB_SHARD_WSADDR					"WsAddr"
//#define 	DB_SHARD_NBPLAYERS				"NbPlayers"
//#define 	DB_SHARD_STATE					"Online"
//#define 	DB_SHARD_CLIENTAPP				"ClientApplication"
//#define 	DB_SHARD_USERLIMIT				"UserLimit"
//#define 	DB_SHARD_VERSION				"Version"
//#define 	DB_SHARD_CONTENT				"Content"
//#define 	DB_SHARD_DEFAULTPATCHURL		"DynPatchURL"

// Types of open state
/*enum TShardOpenState
{
	OpenForAll = 0,				// All
	OpenOnlyForAllowed = 1,		// OpenGroup
	ClosedForAll = 2,			// Only Dev
};*/

// Constantes
enum ENUM_SHARDONLINESTATE 
{ 
	SHARD_OFF,
	SHARD_ON
};

//enum ENUMTBL_ZONE
//{
//	ZONE_ID,
//	ZONE_NAME
//};


//enum ENUMTBL_SHARD
//{
//    SHARD_SHARDID,
//	SHARD_NAME,
//	SHARD_ZONEID,
//	SHARD_USERLIMIT,
//	SHARD_CONTENT,
//	SHARD_ACTIVEFLAG,
//	SHARD_CURUSERCOUNT,
//	SHARD_ENDPER,
//	SHARD_ROOMSHARDID
//};


// Structures
//struct _entryShard // Shard Structure From DB
//{
//	_entryShard():nbPlayers(0)
//	{
//		shardId		= 0;
//		strName		= "";
//		zoneId		= 0;
//		nUserLimit	= 0;
//		ucContent	= "";
//		activeFlag	= SHARD_OFF;
//		nbPlayers	= 0;
//		stateId		= 0;
//		userTypeId	= 0;
//		MainCode = 0;
//		ShardCodeList = "";
//	}
//
//	_entryShard(
//		uint32 _shardId, ucstring _strName, uint8 _zoneId, uint32 _nUserLimit, ucstring _ucContent, 
//		uint8 _activeFlag, uint32 _nbPlayers, uint8 _stateId, uint8 _userTypeId, uint8 _MainCode, ucstring _ShardCodeList)
//	{
//		shardId		= _shardId;
//		strName		= _strName;	
//		zoneId		= _zoneId;
//		nUserLimit	= _nUserLimit;
//		ucContent	= _ucContent;
//		activeFlag	= _activeFlag;
//		nbPlayers	= _nbPlayers;
//		stateId		= _stateId;
//		userTypeId	= _userTypeId;
//		MainCode = _MainCode;
//		ShardCodeList = _ShardCodeList;
//	}
//	void	SetActiveShardIP(std::string strIP) {activeShardIP = strIP;}
//	uint32			shardId; // from Config File
//	ucstring		strName;//[str_size(MAX_NAME_LEN)];
//	uint8			zoneId;
//	uint32			nUserLimit;
//	ucstring		ucContent;//[str_size(MAX_CONTENT_LEN)];
//	uint8			activeFlag;
//	uint32			nbPlayers;
//	uint8			stateId;
//	uint8			userTypeId;
//	uint8			MainCode;
//	ucstring		ShardCodeList;
//	std::string		activeShardIP;
//};


//struct _entryShardNew
//{
//	_entryShardNew()
//	{
//		shardId = 0;
//		strName = "";
//		activeFlag = SHARD_OFF;
//		stateId = 0;
//		ShardCodeList = "";
//		activeShardIP = "";
//	}
//
//	_entryShardNew(uint32 _shardId, ucstring _strName, uint8 _activeFlag, uint8 _stateID, ucstring _ShardCodeList, std::string strIP)
//	{
//		shardId	= _shardId;
//		strName	= _strName;
//		activeFlag	= _activeFlag;
//		stateId     = _stateID;
//		ShardCodeList = _ShardCodeList;
//		activeShardIP = strIP;
//	}
//
//	uint32			shardId; // from Config File
//	ucstring		strName;//[str_size(MAX_NAME_LEN)];
//	uint8			activeFlag;
//	uint8           stateId;
//	ucstring		ShardCodeList;
//	std::string		activeShardIP;
//};

enum	ENUMTBL_SHARDSTATEDEF
{
	SHARDSTATEDEF_SHARDID,
	SHARDSTATEDEF_STATEID,
	SHARDSTATEDEF_USERTYPEID,
	SHARDSTATEDEF_STARTPER,
	SHARDSTATEDEF_ENDPER
};

struct	_entryShardState
{
	_entryShardState()
	{};

	_entryShardState(uint32	_shardId, uint8 _stateId, uint8 _userTypeId, uint8 _startPer, uint8 _endPer)
	{
		shardId		= _shardId;	
		stateId		= _stateId;
		userTypeId	= _userTypeId;
		startPer	= _startPer;
		endPer		= _endPer;
	};

	uint32	shardId;
	uint8	stateId;
	uint8	userTypeId;
	uint8	startPer;
	uint8	endPer;
};

enum	ENUMTBL_SHARDVIPS
{
	SHARDVIP_SHARDID,
	SHARDVIP_USERID
};

struct	_entryShardVIPs 
{
	_entryShardVIPs() {};
	_entryShardVIPs(uint32 _shardId, uint32 _VIPUserId) 
	{
		shardId		= _shardId;
		VIPUserId	= _VIPUserId;
	};
	uint32	shardId;
	uint32	VIPUserId;
};

//struct LobbyInfo
//{
//	LobbyInfo() {};
//	LobbyInfo(uint32 _lobbyId, uint32 _Lobby_state)
//	{
//		lobbyId = _lobbyId;
//		lobby_state = _Lobby_state;
//	
//	};
//    uint32 lobbyId;
//	uint32 lobby_state;
//};

/************************************************************************/
/*								tbl_mstUpdateServers					*/
/************************************************************************/

//#define		DB_TBLFTPURI				"tblftpuri"
//#define		DB_FTP_ID					"ServerID"
//#define		DB_FTP_ALIASNAME			"AliasName"
//#define		DB_FTP_URI					"ServerUri"
//#define		DB_FTP_USERID				"UserID"
//#define		DB_FTP_USERPASSWORD			"UserPassword"

enum ENUMTBL_FTPURL
{
	UPDATESERVER_ID,
	UPDATESERVER_ALIASNAME,
	UPDATESERVER_ROOTURI,
	UPDATESERVER_PORT,
	UPDATESERVER_USERID,
	UPDATESERVER_USERPASSWORD,
};

struct _entryUpdateServer  // Can Generalize to _ServerInfo
{
	_entryUpdateServer():sUri(""), sUserID(""), sPassword("") {};
	_entryUpdateServer(uint32 _serverID, ucstring _ucAliasName, std::string _sUri, uint32 _port, std::string _sUserID, std::string _sPassword)
	{
		ServerID	= _serverID;
		sUri		= _sUri;
		port		= _port;
		sUserID		= _sUserID;
		sPassword	= _sPassword;
		ucAliasName	= _ucAliasName;
	}
	void setParm(uint32 _serverID, ucstring _ucAliasName, std::string _sUri, uint32 _port, std::string _sUserID, std::string _sPassword)
	{
		ServerID	= _serverID;
		sUri		= _sUri;		
		port		= _port;
		sUserID		= _sUserID;
		sPassword	= _sPassword;
		ucAliasName	= _ucAliasName;
	}

	uint32			ServerID;
	ucstring		ucAliasName;
	std::string		sUri;		// protocal://host:port/Dir/file
	uint32			port;
	std::string		sUserID;
	std::string		sPassword;
};

/************************************************************************/
/*								tbl_mstApplications		                */
/************************************************************************/

enum	ENUM_APPLASTVERSION
{
	LASTVERSION_APPID,
	LASTVERSION_APPNAME,
	LASTVERSION_LASTVERSION,
};

struct	_entryAppLastVersion
{
	_entryAppLastVersion()
	{};

	_entryAppLastVersion(uint32 _idApp, ucstring _nameApp, std::string _sLastVersion)
	{
		idApp = _idApp;
		nameApp = _nameApp;
		sLastVersion = _sLastVersion;
	}

	uint32		idApp;
	ucstring	nameApp;
	std::string	sLastVersion;
};

enum	ENUM_UPDATEAPPVERSION
{
	APPVERSION_SERVERID,
	APPVERSION_APPID,
	APPVERSION_SRCVERSION,
	APPVERSION_DSTVERSION,
	APPVERSION_PATH,
};

struct	_entryUpdateAppsInfo 
{
	_entryUpdateAppsInfo()
	{
	};

	_entryUpdateAppsInfo(uint32	_SrvId, uint32 _AppId, std::string _srcVersion, std::string _dstVersion, std::string _Path, ucstring _appName, std::string _sLastVersion)
	{
		SrvId		= _SrvId;
		AppId		= _AppId;
		srcVersion	= _srcVersion;
		dstVersion	= _dstVersion;
		Path		= _Path;
		AppName		= _appName;
		lastVersion	= _sLastVersion;
	};

	uint32			SrvId;
	uint32			AppId;
	std::string		srcVersion;
	std::string		dstVersion;
	std::string		Path;
	ucstring		AppName;
	std::string		lastVersion;
};

/************************************************************************/
/*								tblvariables	                        */
/************************************************************************/

//#define  DB_TBLVARIABLES						"tblvariables"
//#define  DB_VAR_PATH							"Path"
//#define  DB_VAR_ERRBOUND						"Error_bound"
//#define  DB_VAR_ALARMORDER						"Alerm_order"
//#define  DB_VAR_GROUPUPDATE						"Graph_update"

//enum ENUMTBL_VARIABLE
//{
//	VAR_PATH,
//	VAR_ERRBOUND,
//	VAR_ALARMORDER,
//	VAR_GROUPUPDATE
//};


/************************************************************************/
/*								Stored Procedures                       */
/************************************************************************/

//#define DB_SP_GETSHARD_NAME						"SP_getSHARD_name"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Structure

//struct CFrontEnd
//{
//	CFrontEnd() : SId(0), Patching(false), PatchURI("")	{}
//	CFrontEnd(uint16 sid, bool patching = false, const std::string& patchingURI="") : SId(sid), Patching(patching), PatchURI(patchingURI)	{}
//
//	uint16		SId;		// Service Id of the frontend on the remote shard
//	bool		Patching;	// Is FS in patching mode
//	std::string	PatchURI;	// Patch URI
//};

//struct CShard
//{
//	_entryShard		dbInfo;
//
//	uint16			SId;		// uniq service id used to identify the connection when a callback happen
//	std::vector<CFrontEnd>	FrontEnds;	// frontends on the shard
//
//	CShard(){
//		//dbInfo.state = 1;		
//	};
//
//	CShard(_entryShard &shardInfo, uint16 sid)
//		: dbInfo(shardInfo), SId(sid)
//	{};
//
//	// not tested
//	void serial (SIPBASE::IStream &s)
//	{
//		s.serial(dbInfo.shardId);
//		s.serial(dbInfo.strName);
//		s.serial(dbInfo.zoneId);	
//		s.serial(dbInfo.nUserLimit);
//		s.serial(dbInfo.ucContent);
//		s.serial(dbInfo.activeFlag);
//		s.serial(dbInfo.nbPlayers);
//		s.serial(dbInfo.stateId);
//		s.serial(dbInfo.userTypeId);
//		s.serial(SId);
//	}
//
//	CShard & operator= ( const CShard & shard)
//	{
//		dbInfo.shardId			= shard.dbInfo.shardId;		
//		dbInfo.strName			= shard.dbInfo.strName;
//		dbInfo.zoneId			= shard.dbInfo.zoneId;
//		dbInfo.nUserLimit		= shard.dbInfo.nUserLimit;
//		dbInfo.ucContent		= shard.dbInfo.ucContent;
//		dbInfo.activeFlag		= shard.dbInfo.activeFlag;
//		dbInfo.nbPlayers		= shard.dbInfo.nbPlayers;
//		dbInfo.stateId			= shard.dbInfo.stateId;
//		dbInfo.userTypeId		= shard.dbInfo.userTypeId;
//		SId						= shard.SId;
//
//		uint	size = shard.FrontEnds.size();
//		for ( uint i = 0; i < size; i ++ )
//		{
//			FrontEnds.push_back(shard.FrontEnds[i]);
//		}
//
//		return	*this;
//	}
//};


/************************************************************************/
/*                            Shard Table Entry                         */
/************************************************************************/

// Room User
//enum ENUMTBL_ROOMUSER
//{
//	ROOMUSER_ID,
//	ROOMUSER_ROOMKINDID,
//	ROOMUSER_ROOMID,
//	ROOMUSER_RELATIONID,
//	ROOMUSER_VISITRECORD
//};

//enum ENUMTBL_ROOMUSERRELATION
//{
//	RELATION_MASTER,
//	RELATION_MANAGER,
//	RELATION_FAVORER,
//	RELATION_GUEST,
//	RELATION_ISOLATOR
//};

// tbl_Room
//enum ENUMTBL_ROOM
//{
//	ROOM_ID,
//	ROOM_NAME,
//	ROOM_ROOMSUBTYPEID,
//	ROOM_OWNERID,
//	ROOM_CREATIONDAY,
//	ROOM_BACKDAY,
//	ROOM_DATAURLID,
//	ROOM_OPENABILITYID,
//	ROOM_VISITS,
//	ROOM_MEMORIALS,
//	ROOM_VISITDAYS,
//	ROOM_EXP,
//	ROOM_VIRTUE,
//	ROOM_FAME,
//	ROOM_APPRECIATION,
//	ROOM_DEGREE
//};

//struct	_entryRoom
//{
//	_entryRoom()	{};
//	// When create room
//	_entryRoom(uint32 _idRoom, uint32 _idMaster, ucstring _nameRoom, uint16 _idSubTypeId, std::string _creationDay, std::string	_backDay, uint8 _Degree,
//		uint32 _idDead1 = 0, uint32 _idDead2 = 0)
//
//	{
//		idRoom			= _idRoom;
//		nameRoom		= _nameRoom;
//		idSubTypeId		= _idSubTypeId;
//		idMaster		= _idMaster;		
//		creationDay		= _creationDay;
//		backDay			= _backDay;
//		Degree			= _Degree;
//		idDead1			= _idDead1;
//		idDead2			= _idDead2;
//	}
//
//	uint32		idRoom;
//	ucstring	nameRoom;
//	uint16		idSubTypeId;
//	uint32		idMaster;
//	std::string		creationDay;
//	std::string		backDay;
//	uint8		Degree;
//	uint32		idDead1;
//	uint32		idDead2;
//};

//enum ENUMTBL_ROOMKIND
//{
//	ROOMKIND_ROOMSUBTYPEID,
//	ROOMKIND_NAME,
//	ROOMKIND_STYLEID,
//	ROOMKIND_FAITHID,
//	ROOMKIND_CLASS,
//	ROOMKIND_PRICE,
//	ROOMKIND_DAYS,
//	ROOMKIND_ADDPRICE,
//	ROOMKIND_ADDDAYS,
//	ROOMKIND_MAXCHARS,
//	ROOMKIND_OWNERCHARS,
//	ROOMKIND_KINCHARS,
//	ROOMKIND_FRIENDCHARS,
//	ROOMKIND_NUMCHARACTERS,
//	ROOMKIND_PTX,
//	ROOMKIND_PTY,
//	ROOMKIND_PTZ,
//	ROOMKIND_DIR,
//	ROOMKIND_CONTENT
//};

//struct	_entryRoomKind
//{
//	_entryRoomKind()
//	{};
//	_entryRoomKind(	uint16 _id, ucstring	_name, uint8 _idSubType, uint8 _idFaith, uint8 _level, uint32 _price, uint32 _days, uint32 _addPrice,
//		uint32 _addDays, uint8 _maxChars, uint8	_ownerChars, uint8 _kinChars, uint8 _friendChars, uint8 _numCharacters, float _x, float _y, float _z, float _dir, ucstring _ucContent)
//	{
//		id				= _id;
//		name			= _name;
//		idSubType		= _idSubType;
//		idFaith			= _idFaith;
//		level			= _level;
//		price			= _price;
//		days			= _days;
//		addPrice		= _addPrice;
//		addDays			= _addDays;
//		maxChars		= _maxChars;
//		ownerChars		= _ownerChars;
//		kinChars		= _kinChars;
//		friendChars		= _friendChars;
//		numCharacters	= _numCharacters;
//		x				= _x;
//		y				= _y;
//		z				= _z;
//		dir				= _dir;
//		ucContent		= _ucContent;
//	}
//	uint16		id;
//	ucstring	name;
//	uint8		idSubType;
//	uint8		idFaith;
//	uint8		level;
//	uint32		price;
//	uint32		days;
//	uint32		addPrice;
//	uint32		addDays;
//	uint8		maxChars;
//	uint8		ownerChars;
//	uint8		kinChars;
//	uint8		friendChars;
//	uint8		numCharacters;
//	float		x;
//	float		y;
//	float		z;
//	float		dir;
//	ucstring	ucContent;
//};

//enum ENUMTBL_MOURNROOM
//{
//	MOURNROOM_ID,
//	MOURNROOM_MASTERID,
//	MOURNROOM_MASTERNAME,
//	MOURNROOM_RELATION,
//	MOURNROOM_CREATIONDAY,
//	MOURNROOM_DEADNAME,
//	MOURNROOM_SEXID,
//	MOURNROOM_BIRTHDATE,
//	MOURNROOM_ORIGIN,
//	MOURNROOM_DEADDATE,
//	MOURNROOM_PLACEID,
//	MOURNROOM_BRIEFHISTORY,
//	MOURNROOM_MEMORIALNOTE,
//	MOURNROOM_LASTVISITDAYS,
//	MOURNROOM_FLOWERS,
//	MOURNROOM_SPIRITES,
//	MOURNROOM_CANDLES,
//	MOURNROOM_INCENSES,
//	MOURNROOM_MUSICS,
//	MOURNROOM_PRAYS,
//	MOURNROOM_LASTMEMORIALID
//};

//struct	_entryMournRoom
//{
//	_entryMournRoom()
//	{
//	}
//
//	_entryMournRoom(uint32 _idRoom, uint32 _idMaster, ucstring _ucMaster, ucstring _ucRelation, std::string _createDate, std::string _deadName, uint8 _sex,
//		std::string	_birthDate, ucstring _origin, std::string _deadDate, uint16 _placeID, ucstring _ucBriefHistory, ucstring _ucMemorialNote, std::string _lastVisitDays,
//		uint32 _flowers, uint32 _spirites, uint32 _candles, uint32 _incenses, uint32 _musics, uint32 _prays, uint32 _lastMemorilId)
//	{
//		idRoom				= _idRoom;
//		idMaster			= _idMaster;
//		ucMaster			= _ucMaster;
//		ucRelation			= _ucRelation;
//		createDate			= _createDate;
//		deadName			= _deadName;
//		sex					= _sex;
//		birthDate			= _birthDate;
//		origin				= _origin;
//		deadDate			= _deadDate;
//		placeID				= _placeID;
//		ucBriefHistory		= _ucBriefHistory;
//		ucMemorialNote		= _ucMemorialNote;
//		lastVisitDays		= _lastVisitDays;
//		flowers				= _flowers;
//		spirites			= _spirites;
//		candles				= _candles;
//		incenses			= _incenses;
//		musics				= _musics;
//		prays				= _prays;
//		lastMemorilId		= _lastMemorilId;
//	};
//
//	uint32			idRoom;
//	uint32			idMaster;
//	ucstring		ucMaster;
//	ucstring		ucRelation;
//	std::string		createDate;
//	std::string		deadName;
//	uint8			sex;
//	std::string		birthDate;
//	ucstring		origin;
//	std::string		deadDate;
//	uint16			placeID;
//	ucstring		ucBriefHistory;
//	ucstring		ucMemorialNote;
//	std::string		lastVisitDays;
//	uint32			flowers;
//	uint32			spirites;
//	uint32			candles;
//	uint32			incenses;
//	uint32			musics;
//	uint32			prays;
//	uint32			lastMemorilId;
//};


//enum ENUMTBL_DEAD
//{
//	DEAD_ROOMID,
//	DEAD_INDEX,
//	DEAD_NAME,
//	DEAD_SEX,
//	DEAD_BIRTHDAY,
//	DEAD_DEADDAY,
//	DEAD_BRIEFHISTORY,
//	DEAD_UPDATETIME
//};

struct _entryDead
{
	_entryDead()
	{};
	_entryDead(	ucstring _ucRoomName, ucstring _ucname, uint8 _surnamelen, uint32 _roomId, uint8 _index, uint8 _sex, T_DATE4 _birthDay, T_DATE4 _deadDay, ucstring _briefHistory, /*std::string _updateTime,*/ ucstring _nation, ucstring _domicile, uint8 _photoType)
	{
		ucRoomName		= _ucRoomName;
		idRoom			= _roomId;
		index			= _index;
		ucName			= _ucname;
		surnamelen		= _surnamelen;
		sex				= _sex;
		birthDay		= _birthDay;
		deadDay			= _deadDay;
		briefHistory	= _briefHistory;
//		updateTime		= _updateTime;
		nation			= _nation;
		domicile		= _domicile;
		photoType		= _photoType;
	};
	ucstring		ucRoomName;
	ucstring		ucName;
	uint8			surnamelen;
	uint32			idRoom;
	uint8			index;
	uint8			sex;
	T_DATE4			birthDay;
	T_DATE4			deadDay;
	ucstring		briefHistory;
//	std::string		updateTime;
	ucstring		nation;
	ucstring		domicile;
	uint8			photoType;
};

//enum ENUMTBL_NEIGHBOR
//{
//	NEIGHBOR_USERID,
//	NEIGHBOR_NEIGHBORID,
//	NEIGHBOR_RELATEID,
//	NEIGHBOR_RELATENAME,
//	NEIGHBOR_STATE
//};

//struct _entryContact
//{
//	_entryContact()
//	{};
//
//	_entryContact(	uint32 _userID, uint32 _neighborID, uint8 _RelationID, ucstring _GroupName, uint8 _state)
//	{
//		userID				= _userID;
//		idNeighbor			= _neighborID;
//		RelationID			= _RelationID;
//		GroupName			= _GroupName;
//		state				= _state;
//	};
//
//	uint32		userID;
//	uint32		idNeighbor;
//	uint8		RelationID;
//	ucstring	GroupName;
//	uint8		state;
//};

/************************************************************************/
/*							tbl_Family                                  */
/************************************************************************/

//enum ENUMTBL_FAMILY
//{
//	FAMILY_USERID,
//	FAMILY_UCFAMILY,
//	FAMILY_UCNAME,
//	FAMILY_USERSEXID,
//	FAMILY_USERNOTE,
//	FAMILY_EXP,
//	FAMILY_VIRTUE,
//	FAMILY_FAME,
//	FAMILY_NVISITDAY,
//	FAMILY_LEVEL,
//	FAMILY_HORNORID
//};

//struct _entryFamily
//{
//	_entryFamily()
//	{};
//
//	_entryFamily( uint32 _userID, ucstring _ucFamily, ucstring _ucName, uint8 _userSexId, ucstring _userNote,
//		uint16 _exp, uint16 _virtue, uint16 _fame, uint16 _nVisitdays, uint8 _level, uint8 _hornorId )
//	{
//		userID			= _userID;
//		ucFamily		= _ucFamily;
//		ucName			= _ucName;
//		userSexId		= _userSexId;
//		userNote		= _userNote;
//		exp				= _exp;
//		virtue			= _virtue;
//		fame			= _fame;
//		nVisitdays		= _nVisitdays;
//		level			= _level;
//		hornorId		= _hornorId;
//	};
//	uint32		userID;
//	ucstring	ucFamily;
//	ucstring	ucName;
//	uint8		userSexId;
//	ucstring	userNote;
//	uint16		exp;
//	uint16		virtue;
//	uint16		fame;
//	uint16		nVisitdays;
//	uint8		level;
//	uint8		hornorId;
//};

//extern	TCHAR	g_szQuery[MAX_SQL_LEN];

/********************************* END COMMON ***************************************/

#endif // TIAN_SERVER_DBDATA_H
