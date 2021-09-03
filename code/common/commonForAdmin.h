
#pragma once

#include "misc/types_sip.h"
#include "misc/variable.h"
#include "net/service.h"

// 관리자계정가입결과
enum ENUM_CONFIRM_MH	{CONFIRM_MH_ACCOUNT_SUCCESS, CONFIRM_MH_ACCOUNT_NO, CONFIRM_MH_ACCOUNT_MISS, CONFIRM_MH_ACCOUNT_DOUBLE, CONFIRM_MH_ACCOUNT_MAX};
// 관리자계정암호변경결과
enum ENUM_CHPWD_MH		{CHPWD_MH_SUCCESS = 1, CHPWD_MH_FAILED };
// 관리자계정이름변경결과
enum ENUM_CHNAME_MH		{CHNAME_MH_SUCCESS = 1, CHNAME_MH_FAILED };

#define	BEGINONEPOWERSYMBOL	"<"		// 권한문자렬시작기호
#define	ENDONEPOWERSYMBOL	">"		// 권한문자렬끝기호
#define	TAGSHARDPOWERSYMBOL	"S"		// 지역싸이트관리권한시작문자

#define	POWER_FULLMANAGER_ID		1		// Full관리자의 권한ID
#define	POWER_COMPONENTMANAGER_ID	2		// Component관리자의 권한ID
#define	POWER_GENERAL_ID			3		// 일반권한ID
#define	POWER_ACCOUNTMANAGER_ID		5		// 계정관리자의 권한ID

extern	bool	IsFullManager(std::string _sPower);
extern	bool	IsComponentManager(std::string _sPower);
extern	bool	IsAccountManager(std::string _sPower);
extern	bool	IsShardManager(std::string _sPower, uint32 _uShardID);
extern	bool	IsViewableShardInfo(std::string _sPower, uint32 _uShardID);
extern	std::string	MakePowerUnit(uint32 _uID);
extern	int		ComparePower(std::string _sP1, std::string _sP2);

// 관리자권한정보자료구조
struct	MANAGEPOWER
{
	MANAGEPOWER(uint32 _uID, uint32 _uType, ucstring _ucComment, uint32 _uV)	:
		m_uID(_uID), m_uType(_uType), m_ucComment(_ucComment), m_uValue(_uV) {}
	uint32		m_uID;
	uint32		m_uType;
	ucstring	m_ucComment;
	uint32		m_uValue;
};
typedef	std::map<uint32, MANAGEPOWER>	MAP_MANAGEPOWER;

// 관리자정보자료구조
struct	ACCOUNTINFO
{
	ACCOUNTINFO() : m_uID(0)	{}
	ACCOUNTINFO(uint32 _uID, std::string _sAccountID, std::string _sPassword, std::string  _sPower)	:
	m_uID(_uID), m_sAccount(_sAccountID), m_sPassword(_sPassword), m_sPower(_sPower) {}
		ACCOUNTINFO(uint32 _uID, std::string _sAccountID, std::string _sPassword, uint8 _uPower)	:
	m_uID(_uID), m_sAccount(_sAccountID), m_sPassword(_sPassword), m_uPower(_uPower) 
		{
			m_sPower = MakePowerUnit(_uPower);
		}

	uint32			m_uID;				// Manager ID(DB)
	std::string		m_sAccount;		
	std::string		m_sPassword;
	uint8			m_uPower;
	std::string		m_sPower;
};
typedef	std::map<uint32,	ACCOUNTINFO>		MAP_ACCOUNTINFO;

// RegService정보구조
struct	REGSERVICEINFO
{
	uint32			m_uID;			// DBID
	uint32			m_uMstSvcID;	// MasterService ID
	uint32			m_uIndex;
	uint32			m_uHostID;
	uint32			m_uShardID;		// Real ShardID
	bool			m_bRegisterHost;
	std::string		m_sSvcExe;

	void serial (SIPBASE::IStream &s)
	{
		s.serial(m_uID);
		s.serial(m_uMstSvcID);
		s.serial(m_uIndex);
		s.serial(m_uHostID);
		s.serial(m_uShardID);
		s.serial(m_bRegisterHost);
		s.serial(m_sSvcExe);
	}
	REGSERVICEINFO(){}
	REGSERVICEINFO(uint32 _uID, uint32 _uServiceID, uint32 _uServiceIndex, uint32 _uHostID, uint32 _uShardID, bool _bRegisterHost, std::string _sExeName) :
	m_uID(_uID), m_uMstSvcID(_uServiceID), m_uIndex(_uServiceIndex), m_uHostID(_uHostID), m_uShardID(_uShardID), m_bRegisterHost(_bRegisterHost), m_sSvcExe(_sExeName)	{}
};
typedef	std::map<uint32,	REGSERVICEINFO>			MAP_REGSERVICEINFO;
typedef	std::map<uint32,	REGSERVICEINFO>			MAP_UNREGSERVICEINFO;

struct	SERVICE_ONLINE_INFO
{
	bool		m_bRegDB;
	uint16		m_OnSvcID;
	uint16		m_OnHostSvcID;
	uint32		m_RegID;

	uint32		m_ShardID;	// Only Use in Server

	SERVICE_ONLINE_INFO()
	{
		m_bRegDB	= true;
		m_OnSvcID	= 0;
		m_OnHostSvcID = 0;
		m_RegID		= 0;
		m_ShardID	= 0;
	}
	SERVICE_ONLINE_INFO(bool _bReg, uint16 _onSvcID, uint16 _onAESID, uint32 _regID) :
	m_bRegDB(_bReg), m_OnSvcID(_onSvcID), m_OnHostSvcID(_onAESID), m_RegID(_regID)	{}

	void serial (SIPBASE::IStream &s)
	{
		s.serial(m_bRegDB);
		s.serial(m_OnHostSvcID);
		s.serial(m_OnSvcID);
		s.serial(m_RegID);
	}
};

struct	REGPORTINFO : public SIPNET::TRegPortItem
{
	REGPORTINFO()
	{
		_uSvcID = 0;
	}

	REGPORTINFO(uint16 _uAESid, uint16 _uid, SIPNET::TRegPortItem info)
		: _uAESSvcID(_uAESid), _uSvcID(_uid)
	{
		_sPortAlias	= info._sPortAlias;
		_typePort	= info._typePort;
		_uPort		= info._uPort;
	}

	void serial (SIPBASE::IStream &s)
	{
		s.serial(_uAESSvcID);
		s.serial(_uSvcID);
		s.serial(_sPortAlias);
		s.serial(_typePort);
		s.serial(_uPort);
	}

	uint16					_uSvcID;	//OnlineServiceID
	uint16					_uAESSvcID;	//OnlineServiceID
};
typedef	std::map<uint32,	REGPORTINFO>		MAP_REGPORT;


// RegHost정보구조
struct	REGHOSTINFO
{
	uint32			m_uID;
	ucstring		m_ucAliasName;
	std::string		m_sHostName;
	uint32			m_uShardID;
	std::string		m_sVersion;

	void serial (SIPBASE::IStream &s)
	{
		s.serial(m_uID);
		s.serial(m_ucAliasName);
		s.serial(m_sHostName);
		s.serial(m_uShardID);
		s.serial(m_sVersion);
	}
	REGHOSTINFO(){}
	REGHOSTINFO(uint32 _uID, ucstring _ucAliasName, std::string _sHostName, uint32 _uShardID, std::string _sVersion) :
	m_uID(_uID), m_ucAliasName(_ucAliasName), m_sHostName(_sHostName), m_uShardID(_uShardID), m_sVersion(_sVersion) {}
};
typedef	std::map<uint32,	REGHOSTINFO>			MAP_REGHOSTINFO;
typedef	std::map<uint32,	REGHOSTINFO>			MAP_UNREGHOSTINFO;

// RegShard정보구조
struct	REGSHARDINFO
{
	REGSHARDINFO(uint32 _uID, ucstring _ucShardName, uint32 _uZoneID) :
		m_uID(_uID), m_ucShardName(_ucShardName), m_uZoneID(_uZoneID) {}
	uint32			m_uID;
	ucstring		m_ucShardName;
	uint32			m_uZoneID;
};
typedef	std::map<uint32,	REGSHARDINFO>		MAP_REGSHARDINFO;

// RegZone정보구조
struct	REGZONEINFO
{
	REGZONEINFO(uint32 _uID, ucstring _ucZoneName) :
		m_uID(_uID), m_ucZoneName(_ucZoneName) {}
	uint32			m_uID;
	ucstring		m_ucZoneName;
};
typedef	std::map<uint32,	REGZONEINFO>		MAP_REGZONEINFO;

// 감시변수자료구조
struct	MONITORVARIABLE
{
	MONITORVARIABLE(uint32 _uID, std::string _sHost, std::string _sService, std::string _sVariable, ucstring _ucCom, uint32 _uCycle, uint32 _uDefaultWarning, uint32 _uUpLimit, uint32 _uLowLimit) :
		m_uID(_uID), m_sHost(_sHost), m_sService(_sService), m_sVariable(_sVariable), m_ucComment(_ucCom), m_uCycle(_uCycle), m_uDefaultWarning(_uDefaultWarning), m_uUpLimit(_uUpLimit), m_uLowLimit(_uLowLimit) {}

	uint32			m_uID;
	std::string		m_sHost;
	std::string		m_sService;
	std::string		m_sVariable;
	ucstring		m_ucComment;
	uint32		m_uCycle;
	uint32		m_uDefaultWarning;	// bool과 같다
	uint32		m_uUpLimit;
	uint32		m_uLowLimit;
};
typedef	std::map<uint32, MONITORVARIABLE>	MAP_MONITORVARIABLE;

// Update정보자료구조
struct	SHARDUPDATEINFO
{
	SHARDUPDATEINFO() : m_uPort(0) {}
	SHARDUPDATEINFO(ucstring _ucServerName, uint16 _uPort, ucstring _ucUsername, ucstring _ucPassword, ucstring _ucRemoteFileName) :
		m_ucServerName(_ucServerName), m_uPort(_uPort), m_ucUsername(_ucUsername), m_ucPassword(_ucPassword), m_ucRemoteFileName(_ucRemoteFileName) {}
	ucstring		m_ucServerName;
	uint16			m_uPort;
	ucstring		m_ucUsername;
	ucstring		m_ucPassword;
	ucstring		m_ucRemoteFileName;
};
extern	SHARDUPDATEINFO		ShardUpdateInfo;

// 헴뭬빕능揭 root path
extern std::string sServiceRootPath;
// 헴뭬빕능揭 괩눼鱇 瞼쬔 설몬능

enum { CONF_ANY, CONF_ADMIN, CONF_SHARD, CONF_HOST };

struct MSTSERVICECONFIG 
{
	uint32		uMstID;
	std::string	sAliasName;
	std::string	sShortName;
	std::string	sExeName;
	std::string	sRelativePath;
	uint32	nCfgFlag;

	void serial (SIPBASE::IStream &s)
	{
		s.serial(uMstID);
		s.serial(sAliasName);
		s.serial(sShortName);
		s.serial(sExeName);
		s.serial(sRelativePath);
		s.serial(nCfgFlag);
	}

	MSTSERVICECONFIG()
		:uMstID(0), sAliasName	(""), sShortName(""), sExeName(""), sRelativePath(""), nCfgFlag(CONF_ANY)
	{}
	MSTSERVICECONFIG(uint32 _uMstID, std::string _sAliasName, std::string _sShortName, std::string _sExeName, std::string _sRelativePath, uint32 _nCfgFlag)
		:uMstID(_uMstID), sAliasName	(_sAliasName), sShortName(_sShortName), sExeName(_sExeName), sRelativePath(_sRelativePath), nCfgFlag(_nCfgFlag)
	{}
};

typedef std::map<uint32, MSTSERVICECONFIG>		MAP_MSTSERVICECONFIG;
extern MAP_MSTSERVICECONFIG		map_MSTSERVICECONFIG;