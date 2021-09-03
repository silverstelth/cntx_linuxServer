
#include "ManageComponent.h"
#include "QuerySet.h"
#include "misc/db_interface.h"
#include "../../common/Packet.h"
#include "../Common/Common.h"
#include "ManageAccount.h"
using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

MAP_REGSERVICEINFO	map_REGSERVICEINFO, map_UNREGSERVICEINFO;
MAP_REGHOSTINFO		map_REGHOSTINFO;
MAP_UNREGHOSTINFO	map_UNREGHOSTINFO;
MAP_REGSHARDINFO	map_REGSHARDINFO;
MAP_REGZONEINFO		map_REGZONEINFO;
MAP_REGPORT			map_REGPORT;

#define	FOR_START_MAP_REGSERVICEINFO(it)											\
	MAP_REGSERVICEINFO::iterator	it;												\
	for(it = map_REGSERVICEINFO.begin(); it != map_REGSERVICEINFO.end(); it++)		\

#define	FOR_START_MAP_UNREGSERVICEINFO(it)											\
	MAP_REGSERVICEINFO::iterator	it;												\
	for(it = map_UNREGSERVICEINFO.begin(); it != map_UNREGSERVICEINFO.end(); it++)	\

#define	FOR_START_MAP_REGHOSTINFO(it)											\
	MAP_REGHOSTINFO::iterator	it;												\
	for(it = map_REGHOSTINFO.begin(); it != map_REGHOSTINFO.end(); it++)		\

#define	FOR_START_MAP_UNREGHOSTINFO(it)											\
	MAP_REGHOSTINFO::iterator	it;												\
	for(it = map_UNREGHOSTINFO.begin(); it != map_UNREGHOSTINFO.end(); it++)	\

#define	FOR_START_MAP_REGSHARDINFO(it)											\
	MAP_REGSHARDINFO::iterator	it;												\
	for(it = map_REGSHARDINFO.begin(); it != map_REGSHARDINFO.end(); it++)		\

#define	FOR_START_MAP_REGZONEINFO(it)											\
	MAP_REGZONEINFO::iterator	it;												\
	for(it = map_REGZONEINFO.begin(); it != map_REGZONEINFO.end(); it++)		\

#define	FOR_START_MAP_MSTSERVICECONFIG(it)											\
	MAP_MSTSERVICECONFIG::iterator	it;												\
	for(it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it++)	\

// Host에서 써비스개수 얻기
static	uint32	GetRegServiceNumInHost(uint32 _uHostID)
{
	uint32	uNum = 0;
	FOR_START_MAP_REGSERVICEINFO(it)
	{
		uint32 uHostID = it->second.m_uHostID;
		if (uHostID != _uHostID)
			continue;
		uNum ++;
	}
	return uNum;}

static	uint32	GetUnRegServiceNumInHost(uint32 _uHostID)
{
	uint32	uNum = 0;
	FOR_START_MAP_UNREGSERVICEINFO(it)
	{
		uint32 uHostID = it->second.m_uHostID;
		if (uHostID != _uHostID)
			continue;
		uNum ++;
	}
	return uNum;
}

// 지역싸이트의 Host개수 얻기
static	uint32	GetRegHostNumInShard(uint32 _uShardID)
{
	uint32	uNum = 0;
	FOR_START_MAP_REGHOSTINFO(it)
	{
		uint32	uShardID = it->second.m_uShardID;
		if (uShardID != _uShardID)
			continue;
		uNum ++;
	}

	return uNum;
}

static	uint32	GetUnRegHostNumInShard(uint32 _uShardID)
{
	uint32	uNum = 0;
	FOR_START_MAP_UNREGHOSTINFO(itu)
	{
		uint32	uShardID = itu->second.m_uShardID;
		if (uShardID != _uShardID)
			continue;
		uNum ++;
	}

	return uNum;
}

// MST써비스이름얻기
static	ucstring	GetMSTServiceName(uint32 _uID)
{
	MAP_MSTSERVICECONFIG::iterator	it = map_MSTSERVICECONFIG.find(_uID);
	if (it == map_MSTSERVICECONFIG.end())
		return L"";
	return it->second.sAliasName;
}
// MST써비스Short이름얻기
string	GetMSTServiceShortName(uint32 _uMstID)
{
	MAP_MSTSERVICECONFIG::iterator	it = map_MSTSERVICECONFIG.find(_uMstID);
	if (it == map_MSTSERVICECONFIG.end())
		return "";
	return it->second.sShortName;
}

uint32	GetMSTServiceIDFromSName(string sShortName)
{
	MAP_MSTSERVICECONFIG::iterator	it;
	for ( it = map_MSTSERVICECONFIG.begin(); it != map_MSTSERVICECONFIG.end(); it ++ )
	{
		MSTSERVICECONFIG	&info = it->second;
		if ( info.sShortName == sShortName || strstr( sShortName.c_str(), info.sShortName.c_str() ) != NULL )
			break;
	}

	if ( it == map_MSTSERVICECONFIG.end() )
		return	0;
	else
		return	it->first;
}

ucstring	GetMSTServiceAliasName(uint32 _uID)
{
	MAP_MSTSERVICECONFIG::iterator	it = map_MSTSERVICECONFIG.find(_uID);
	if (it == map_MSTSERVICECONFIG.end())
		return L"";
	return it->second.sAliasName;
}

// 동작써비스Short이름얻기
string	GetMSTServiceShortNameOfService(uint32 _uID, bool _bRegister)
{
	uint32	uMstSvcID, uIndex;
	if ( _bRegister )
	{
		MAP_REGSERVICEINFO::iterator	it = map_REGSERVICEINFO.find(_uID);
		if (it == map_REGSERVICEINFO.end())
			return "";
		uMstSvcID = it->second.m_uMstSvcID;
		uIndex = it->second.m_uIndex;
	}
	else
	{
		MAP_UNREGSERVICEINFO::iterator	it = map_UNREGSERVICEINFO.find(_uID);
		if (it == map_UNREGSERVICEINFO.end())
			return "";
		uMstSvcID = it->second.m_uMstSvcID;
		uIndex = it->second.m_uIndex;
	}

	std::string strMstSvcSName = GetMSTServiceShortName(uMstSvcID);
	if ( uIndex == 0 )
		return strMstSvcSName;
	else
		return strMstSvcSName + toStringA(uIndex);
}

// MST써비스Exe이름얻기
string	GetMSTServiceExeName(uint32 _uMstID)	// regID/MstID
{
	MAP_MSTSERVICECONFIG::iterator	it = map_MSTSERVICECONFIG.find(_uMstID);
	if (it == map_MSTSERVICECONFIG.end())
		return "";

	return it->second.sExeName;
}

string	GetServiceExeNameOfService(uint32 _uID)
{
	MAP_REGSERVICEINFO::iterator	it = map_REGSERVICEINFO.find(_uID);
	if (it == map_REGSERVICEINFO.end())
		return "";

	std::string	sExeName= GetMSTServiceExeName(it->second.m_uMstSvcID);
	uint32		uIndex	= it->second.m_uIndex;
	if ( uIndex == 0 )
		return sExeName;
	else
		return sExeName + toString(" -n%d", uIndex);
}

ucstring	GetMSTServiceAliasNameOfService(uint32 _uID)
{
	MAP_REGSERVICEINFO::iterator	it = map_REGSERVICEINFO.find(_uID);
	if (it == map_REGSERVICEINFO.end())
		return "";

	ucstring ucMstSvcAliasName = GetMSTServiceAliasName(it->second.m_uMstSvcID);
	uint32 uIndex = it->second.m_uIndex;
	if ( uIndex == 0 )
		return ucMstSvcAliasName;
	else
		return ucMstSvcAliasName + toStringW(uIndex);
}
// HOST이름얻기
string	GetRegHostName(uint32 _uID, bool _bRegister)
{
	if ( _bRegister )
	{
		MAP_REGHOSTINFO::iterator	it = map_REGHOSTINFO.find(_uID);
		if (it == map_REGHOSTINFO.end())
			return "";
		return it->second.m_sHostName;
	}
	else
	{
		MAP_UNREGHOSTINFO::iterator	it = map_UNREGHOSTINFO.find(_uID);
		if (it == map_UNREGHOSTINFO.end())
			return "";
		return it->second.m_sHostName;
	}
}
// Shard이름얻기
ucstring	GetRegShardName(uint32 _uID)
{
	MAP_REGSHARDINFO::iterator	it = map_REGSHARDINFO.find(_uID);
	if (it == map_REGSHARDINFO.end())
		return L"";
	return it->second.m_ucShardName;
}
// Zone이름얻기
static	ucstring	GetRegZoneName(uint32 _uID)
{
	MAP_REGZONEINFO::iterator	it = map_REGZONEINFO.find(_uID);
	if (it == map_REGZONEINFO.end())
		return L"";
	return it->second.m_ucZoneName;
}
// 동작써비스의 이름얻기
static	ucstring	GetRegServiceName(uint32 _uID)
{
	MAP_REGSERVICEINFO::iterator	it = map_REGSERVICEINFO.find(_uID);
	if (it == map_REGSERVICEINFO.end())
		return L"";

	ucstring strMstSvcName = GetMSTServiceName(it->second.m_uMstSvcID);
	if ( it->second.m_uIndex == 0 )
		return strMstSvcName;
	else
		return strMstSvcName + toStringW(it->second.m_uIndex);
}
// 동작써비스의 소속Host이름얻기
std::string	GetRegHostNameOfService(uint32 _uID, bool _bRegister)
{
	uint32	uHostID;
	bool	bRegister;
	if ( _bRegister )
	{
		MAP_REGSERVICEINFO::iterator	it = map_REGSERVICEINFO.find(_uID);
		if (it == map_REGSERVICEINFO.end())
			return "";
		uHostID = it->second.m_uHostID;
		bRegister = it->second.m_bRegisterHost;
	}
	else
	{
		MAP_UNREGSERVICEINFO::iterator	it = map_UNREGSERVICEINFO.find(_uID);
		if (it == map_UNREGSERVICEINFO.end())
			return "";
		uHostID = it->second.m_uHostID;
		bRegister = it->second.m_bRegisterHost;
	}

	return GetRegHostName(uHostID, bRegister);
}
// Host의 소속Shard이름얻기
ucstring	GetRegShardNameOfHost(uint32 _uID, bool	bRegister)
{
	uint32	uShardID;
	if ( bRegister )
	{
		MAP_REGHOSTINFO::iterator	it = map_REGHOSTINFO.find(_uID);
		if (it == map_REGHOSTINFO.end())
			return L"";

		uShardID = it->second.m_uShardID;
	}
	else
	{
		MAP_UNREGHOSTINFO::iterator	it = map_UNREGHOSTINFO.find(_uID);
		if (it == map_UNREGHOSTINFO.end())
			return L"";

		uShardID = it->second.m_uShardID;
	}

	return GetRegShardName(uShardID);
}
// Shard의 소속Zone이름얻기
ucstring	GetRegZoneNameOfShard(uint32 _uID)
{
	MAP_REGSHARDINFO::iterator	it = map_REGSHARDINFO.find(_uID);
	if (it == map_REGSHARDINFO.end())
		return L"";
	return GetRegZoneName(it->second.m_uZoneID);
}
// 동작써비스의 소속Shard이름얻기
ucstring	GetRegShardNameOfService(uint32 _uID, bool _bRegister)
{
	uint32	uHostID;
	bool	bRegister;
	if ( _bRegister )
	{
		MAP_REGSERVICEINFO::iterator	it = map_REGSERVICEINFO.find(_uID);
		if (it == map_REGSERVICEINFO.end())
			return L"";

		uHostID = it->second.m_uID;
		bRegister = true;
	}
	else
	{
		MAP_UNREGSERVICEINFO::iterator	it = map_UNREGSERVICEINFO.find(_uID);
		if (it == map_UNREGSERVICEINFO.end())
			return L"";

		uHostID = it->second.m_uID;
		bRegister = it->second.m_bRegisterHost;
	}

	return GetRegShardNameOfHost(uHostID, bRegister);
}
ucstring	GetRegZoneNameOfHost(uint32 _uID, bool bRegister)
{
	uint32	uShardID;
	if ( bRegister )
	{
		MAP_REGHOSTINFO::iterator	it = map_REGHOSTINFO.find(_uID);
		if (it == map_REGHOSTINFO.end())
			return L"";

		uShardID = it->second.m_uShardID;
	}
	else
	{
		MAP_UNREGHOSTINFO::iterator	it = map_UNREGHOSTINFO.find(_uID);
		if (it == map_UNREGHOSTINFO.end())
			return L"";

		uShardID = it->second.m_uShardID;
	}

	return GetRegZoneNameOfShard(uShardID);
}

ucstring	GetRegZoneNameOfService(uint32 _uID, bool bRegister)
{
	uint32	uHostID;
	if ( bRegister )
	{
		MAP_REGSERVICEINFO::iterator	it = map_REGSERVICEINFO.find(_uID);
		if (it == map_REGSERVICEINFO.end())
			return L"";

		uHostID = it->second.m_uHostID;
	}
	else
	{
		MAP_REGSERVICEINFO::iterator	it = map_UNREGSERVICEINFO.find(_uID);
		if (it == map_UNREGSERVICEINFO.end())
			return L"";

		uHostID = it->second.m_uHostID;
	}
	return GetRegZoneNameOfHost(uHostID, bRegister);
}

// HostID와 Service shortname으로부터 Service ID얻기
uint32	GetServiceIDFromShortName(uint32 _uHostID, string _sShortName)
{
	FOR_START_MAP_REGSERVICEINFO(it)
	{
		uint32		uID		= it->second.m_uID;
		uint32		uHostID	= it->second.m_uHostID;
		uint32		uMSTID	= it->second.m_uMstSvcID;
		uint32		uIndex	= it->second.m_uIndex;
		if (uHostID != _uHostID)
			continue;
		string		sShort = GetMSTServiceShortName(uMSTID);
		if ( sShort == _sShortName || strstr( _sShortName.c_str(), sShort.c_str() ) != NULL )
			return uID;
	}
	return 0;
}
// Host의 소속ID얻기
uint32		GetShardIDOfHost(uint32 uID, bool bRegister)
{
	uint32	uShardID;
	if ( bRegister )
	{
		MAP_REGHOSTINFO::iterator	it = map_REGHOSTINFO.find(uID);
		if (it == map_REGHOSTINFO.end())
			return 0;

		uShardID = it->second.m_uShardID;
	}
	else
	{
		MAP_UNREGHOSTINFO::iterator	it = map_UNREGHOSTINFO.find(uID);
		if (it == map_UNREGHOSTINFO.end())
			return 0;

		uShardID = it->second.m_uShardID;
	}

	return uShardID;
}
/*// Service의 소속HostID얻기
uint32	GetHostIDOfService(uint32 _uID)
{
	MAP_REGSERVICEINFO::iterator	it = map_REGSERVICEINFO.find(_uID);
	if (it == map_REGSERVICEINFO.end())
		return 0;
	return it->second.m_uHostID;
}
*/
void	DeleteUnRegService(uint32 _uID)
{
	map_UNREGSERVICEINFO.erase(_uID);
}
// Host삭제
void	DeleteUnRegHost(uint32 _uID)
{
	map_UNREGHOSTINFO.erase(_uID);

	vector<uint32>	temp;
	FOR_START_MAP_UNREGSERVICEINFO(it)
	{
		uint32	uSID = it->second.m_uID;
		uint32	uHID = it->second.m_uHostID;
		if (uHID == _uID)
			temp.push_back(uSID);
	}

	vector<uint32>::iterator tempit;
	for (tempit = temp.begin(); tempit != temp.end(); tempit++)
	{
		uint32	uSID = (*tempit);
		DeleteUnRegService(uSID);
	}

}

// 동작써비스삭제
static	void	DeleteRegService(uint32 _uID)
{
	map_REGSERVICEINFO.erase(_uID);
}
// Host삭제
static	void	DeleteRegHost(uint32 _uID)
{
	map_REGHOSTINFO.erase(_uID);

	vector<uint32>	temp;
	FOR_START_MAP_REGSERVICEINFO(it)
	{
		uint32	uSID = it->second.m_uID;
		uint32	uHID = it->second.m_uHostID;
		if (uHID == _uID)
			temp.push_back(uSID);
	}

	vector<uint32>::iterator tempit;
	for (tempit = temp.begin(); tempit != temp.end(); tempit++)
	{
		uint32	uSID = (*tempit);
		DeleteRegService(uSID);
	}
}
// Shard삭제
static	void	DeleteRegShard(uint32 _uID)
{
	map_REGSHARDINFO.erase(_uID);

	vector<uint32>	temp;
	FOR_START_MAP_REGHOSTINFO(it)
	{
		uint32	uHID = it->second.m_uID;
		uint32	uShardID = it->second.m_uShardID;
		if (uShardID == _uID)
			temp.push_back(uHID);
	}

	vector<uint32>::iterator tempit;
	for (tempit = temp.begin(); tempit != temp.end(); tempit++)
	{
		uint32	uHID = (*tempit);
		DeleteRegHost(uHID);
	}
}
// 둥록된 Host인가(return값이 령이면 등록되지 않은 Host이다)
uint32	GetRegisteredHostID(ucstring _ucShardName, string _sHostName)
{
	FOR_START_MAP_REGHOSTINFO(it)
	{
		string		sHost = it->second.m_sHostName;
		if (sHost != _sHostName)
			continue;

		uint32		uID = it->second.m_uID;
		return uID;
	}

	return 0;
}

uint16	GetHostID(ucstring _ucShardName, string _sHostName)
{
// 
// 	FOR_START_MAP_REGHOSTINFO(it)
// 	{
// 		string		sHost = it->second.m_sHostName;
// 		if (sHost != _sHostName)
// 			continue;
// 
// 		uint32		uID = it->second.m_uID;
// 		return uID;
// 	}
// 
// 	FOR_START_MAP_UNREGHOSTINFO(itu)
// 	{
// 		string		sHost = itu->second.m_sHostName;
// 		if (sHost != _sHostName)
// 			continue;
// 
// 		uint32		uID = itu->second.m_uID;
// 		return uID;
// 	}
// 
	return 0;
}


bool	UpdateHostVersion(uint32 _uHostID, string _sVersion)
{
	MAP_REGHOSTINFO::iterator	it = map_REGHOSTINFO.find(_uHostID);
	if (it == map_REGHOSTINFO.end())
		return false;

	if (_sVersion == it->second.m_sVersion)
		return false;

	MAKEQUERY_UPDATEVERSION(strQ, _uHostID, ucstring(_sVersion).c_str());
	DBCaller->execute(strQ);
	it->second.m_sVersion = _sVersion;
	return true;
}

static void	DBCB_DBGetAllService(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32 uID;				resSet->serial(uID);
		uint32 uServiceID;		resSet->serial(uServiceID);
		uint32 uServiceIndex;	resSet->serial(uServiceIndex);
		uint32 uHostID;			resSet->serial(uHostID);
		uint32 uShardID;		resSet->serial(uShardID);

		bool	bRegHost = true;
		std::string	sExeName= GetMSTServiceExeName(uServiceID);
		if ( uServiceIndex != 0 )
			sExeName = sExeName + toString(" -n%d", uServiceIndex);

		map_REGSERVICEINFO.insert( make_pair(uID, REGSERVICEINFO(uID, uServiceID, uServiceIndex, uHostID, uShardID, bRegHost, sExeName)) );
	}
}
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Master service정보 load
static	void	LoadREGSERVICEINFO()
{
	map_REGSERVICEINFO.clear();
	MAKEQUERY_GETALLSERVICE(strQ);
	DBCaller->execute(strQ, DBCB_DBGetAllService);
}

static void	DBCB_DBGetAllServer(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;
	
	uint32	uShardID = 1;
	uint32 nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32		uID;			resSet->serial(uID);
		ucstring	ucAliasName;	resSet->serial(ucAliasName);
		string		sHostName;		resSet->serial(sHostName);
		string		sAddress;		resSet->serial(sAddress);
		string		sVersion;		resSet->serial(sVersion);
		map_REGHOSTINFO.insert( make_pair (uID, REGHOSTINFO(uID, ucAliasName, sAddress, uShardID, sVersion)) );
	}
}
static	void	LoadREGHOSTINFO()
{
	map_REGHOSTINFO.clear();

	MAKEQUERY_GETALLSERVER(strQ);
	DBCaller->execute(strQ, DBCB_DBGetAllServer);
}
static	void	LoadREGSHARDINFO()
{
	map_REGSHARDINFO.clear();

	uint32		uShardID = 1;
	ucstring	ucName = L"";
	uint32		uZoneID = 1;
	map_REGSHARDINFO.insert( make_pair (uShardID, REGSHARDINFO(uShardID, ucName, uZoneID)) );
}
static	void	LoadREGZONEINFO()
{
	map_REGZONEINFO.clear();

	uint32		uID = 1;
	ucstring	ucName = L"";
	map_REGZONEINFO.insert( make_pair (uID, REGZONEINFO(uID, ucName)) );
}

// Zone에서 권한에 따르는 싸이트개수 얻기
static	uint32	GetShardNumForPowerInZone(uint32 _uZoneID, string _sPower)
{
/////////////////////////////////
	uint32	uNum = 0;
	FOR_START_MAP_REGSHARDINFO(itREGSHARDINFO)
	{
		uint32	uShardID	= itREGSHARDINFO->second.m_uID;
		uint32	uZoneID		= itREGSHARDINFO->second.m_uZoneID;
		if (uZoneID != _uZoneID)
			continue;
		if (IsViewableShardInfo(_sPower, uShardID))
			uNum ++;
	}
	return uNum;
}
// Component정보를 보낸다
void	Make_COMPINFO(CMessage *pMes, string _sPower)
{
	uint32		uMstServiceNum = static_cast<uint32>(map_MSTSERVICECONFIG.size());
	pMes->serial(uMstServiceNum);						// Master Service개수넣기
	FOR_START_MAP_MSTSERVICECONFIG(itMSTSERVICECONFIG)
	{
		MSTSERVICECONFIG	cfg = itMSTSERVICECONFIG->second;
		pMes->serial(cfg);	// Master service정보넣기
	}

	uint32		uZoneNum = static_cast<uint32>(map_REGZONEINFO.size());
	pMes->serial(uZoneNum);							// Zone개수넣기
	FOR_START_MAP_REGZONEINFO(itREGZONEINFO)
	{
		uint32		uZoneID		= itREGZONEINFO->second.m_uID;
		ucstring	ucZoneName	= itREGZONEINFO->second.m_ucZoneName;
		pMes->serial(	uZoneID, ucZoneName);			// Zone정보넣기

		uint32	uShardNum = GetShardNumForPowerInZone(uZoneID, _sPower);
		pMes->serial(uShardNum);						// 권한에 따르는 싸이트개수넣기
		FOR_START_MAP_REGSHARDINFO(itREGSHARDINFO)
		{
			uint32		uShardID	= itREGSHARDINFO->second.m_uID;
			ucstring	ucShardName	= itREGSHARDINFO->second.m_ucShardName;
			uint32		uZID		= itREGSHARDINFO->second.m_uZoneID;
			if (uZID != uZoneID)
				continue;
			if (!IsViewableShardInfo(_sPower, uShardID))
				continue;
			pMes->serial(uShardID, ucShardName);	// 지역싸이트정보넣기

			uint32	uHostNum = GetRegHostNumInShard(uShardID);
			pMes->serial(uHostNum);				// 지역싸이트내의 Host개수넣기
			FOR_START_MAP_REGHOSTINFO(itREGHOSTINFO)
			{
				REGHOSTINFO		&aes = itREGHOSTINFO->second;
				uint32	uShID = aes.m_uShardID;
				uint32	uHostID = aes.m_uID;
				if (uShardID != uShID)
					continue;
				pMes->serial(aes);	// Host정보넣기

				uint32	uServiceNum = GetRegServiceNumInHost(uHostID);
				pMes->serial(uServiceNum);						// Host내의 써비스개수넣기
				FOR_START_MAP_REGSERVICEINFO(it)
				{
					REGSERVICEINFO &info = it->second;
					uint32 uHID = info.m_uHostID;
					if (uHostID != uHID)
						continue;
					pMes->serial(info);		// 써비스정보넣기
				}

				uServiceNum = GetUnRegServiceNumInHost(uHostID);
				pMes->serial(uServiceNum);						// Host내의 써비스개수넣기
				FOR_START_MAP_UNREGSERVICEINFO(itu)
				{
					REGSERVICEINFO &info = itu->second;
					uint32 uHID = info.m_uHostID;
					if (uHostID != uHID)
						continue;
					pMes->serial(info);		// 써비스정보넣기
				}
			}

			uHostNum = GetUnRegServiceNumInHost(uShardID);
			pMes->serial(uHostNum);				// 지역싸이트내의 Host개수넣기
			FOR_START_MAP_UNREGHOSTINFO(itUNREGHOSTINFO)
			{
				REGHOSTINFO		&aes = itUNREGHOSTINFO->second;
				uint32	uShID = aes.m_uShardID;
				uint32	uHostID = aes.m_uID;
				if (uShardID != uShID)
					continue;
				pMes->serial(aes);	// Host정보넣기

				uint32	uServiceNum = GetUnRegServiceNumInHost(uHostID);
				pMes->serial(uServiceNum);						// Host내의 써비스개수넣기
				FOR_START_MAP_UNREGSERVICEINFO(itu)
				{
					REGSERVICEINFO &info = itu->second;
					uint32 uHID = info.m_uHostID;
					if (uHostID != uHID)
						continue;
					pMes->serial(info);		// 써비스정보넣기
				}
			}
		}
	}

}

void Send_M_MH_IDENT()
{
	CMessage	msgout(M_MH_IDENT);
	uint32		nSize = static_cast<uint32>(map_REGPORT.size());	msgout.serial(nSize);
	MAP_REGPORT::iterator	it;
	for ( it = map_REGPORT.begin(); it != map_REGPORT.end(); it ++ )
	{
		REGPORTINFO	info = it->second;
		msgout.serial(info);
	}
	ManageHostServer->send(msgout);
	sipinfo("Send Port List(size:%d) To Admin Manager.", nSize);
}

static	void	Send_M_MH_COMPINFO(TSockId from, string _sPower)
{
	CMessage	mes(M_MH_COMPINFO);
	Make_COMPINFO(&mes, _sPower);
	ManageHostServer->send(mes, from);
}

// Component정보얻기
static void cb_M_MH_COMPINFO( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	string	sPower = pManagerInfo->m_sPower;

	Send_M_MH_COMPINFO(from, sPower);
}

static void	DBCB_DBAddService(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32	uHostID	= *(uint32 *)argv[0];
	uint32	uMSID	= *(uint32 *)argv[1];
	uint32	uShardID	= *(uint32 *)argv[2];
	TSockId	from	= *(TSockId *)argv[3];
	
	if (!nQueryResult)
		return;
	uint32	nRows;		resSet->serial(nRows);
	if (nRows <= 0)
		return;

	REGSERVICEINFO	info;
	uint32 uServerIndex;		resSet->serial(uServerIndex);
	uint32 uNewID;				resSet->serial(uNewID);

	bool	bRegHost = true;
	std::string	sExeName= GetMSTServiceExeName(uMSID);
	if ( uServerIndex != 0 )
		sExeName = sExeName + toString(" -n%d", uServerIndex);

	info = REGSERVICEINFO(uNewID, uMSID, uServerIndex, uHostID, uShardID, bRegHost, sExeName);
	map_REGSERVICEINFO.insert( make_pair(uNewID, info) );

	// Send
	CMessage	mes(M_MH_ADDSERVICE);
	mes.serial(info);
	ManageHostServer->send(mes, from);
	// Log기록
	bool	bRegister = true;
	ucstring	ucZone	= GetRegZoneNameOfHost(uHostID, bRegister);
	ucstring	ucShard	= GetRegShardNameOfHost(uHostID, bRegister);
	ucstring	ucHost	= ucstring(GetRegHostName(uHostID, bRegister));
	ucstring	ucSName	= GetMSTServiceName(uMSID);
	ucstring	ucLog	= L"Add service(ID=" +toStringW(uNewID) + L") : Host=" + ucHost + L", Service=" + ucSName + toStringW(uServerIndex);
	AddManageHostLog(LOG_MANAGE_ADDSERVICE, from, ucLog);
}

// Service추가
static void cb_M_MH_ADDSERVICE( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	string	sPower = pManagerInfo->m_sPower;

	// 편집가능한 관리자인가
	if (!IsComponentManager(sPower))
		return;
	
	uint32	uHostID, uMSID, uShardID;
	msgin.serial(uHostID, uMSID, uShardID);

	MAKEQUERY_ADDSERVICE(strQ, uMSID, uHostID, uShardID);
	DBCaller->executeWithParam(strQ, DBCB_DBAddService,
		"DDDD",
		CB_,	uHostID,
		CB_,	uMSID,
		CB_,	uShardID,
		CB_,	from);
}
// Service삭제
static void cb_M_MH_DELSERVICE( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	string	sPower = pManagerInfo->m_sPower;
	
	// 편집가능한 관리자인가
	if (!IsComponentManager(sPower))
		return;

	uint32	uServiceID;
	msgin.serial(uServiceID);

	// DB에서 삭제
	MAKEQUERY_DELETESERVICE(strQ, uServiceID);
	DBCaller->execute(strQ);
/*	
	DB_EXE_QUERY(DBForManage, Stmt, strQ);
	if (nQueryResult != TRUE)
		return;
*/
	// Log기록
	bool	bRegister = true;
	ucstring	ucZone	= GetRegZoneNameOfService(uServiceID, bRegister);
	ucstring	ucShard	= GetRegShardNameOfService(uServiceID, bRegister);
	ucstring	ucHost	= ucstring(GetRegHostNameOfService(uServiceID, bRegister));
	ucstring	ucSName	= GetRegServiceName(uServiceID);
	ucstring	ucLog	= L"Delete service : Host=" + ucHost + L", Service=" + ucSName;
	AddManageHostLog(LOG_MANAGE_DELSERVICE, from, ucLog);

	// 자료에서 삭제
	DeleteRegService(uServiceID);

	// 접속한 관리자들에게 통지
	CMessage	mes(M_MH_DELSERVICE);
	mes.serial(uServiceID);
	ManageHostServer->send(mes, from);

}

static void	DBCB_DBAddHost(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32	uShardID	= *(uint32 *)argv[0];
	string	sHostName	= (char *)argv[1];
	TSockId	from	= *(TSockId *)argv[2];

	if (!nQueryResult)
		return;
	uint32	nRows;		resSet->serial(nRows);
	if (nRows <= 0)
		return;

	uint32 uNewID;		resSet->serial(uNewID);
	ucstring	ucAlias = L"";
	string		sVersion = "Unknown";
	map_REGHOSTINFO.insert( make_pair(uNewID, REGHOSTINFO(uNewID, ucAlias, sHostName, uShardID, sVersion)) );

	// Send
	CMessage	mes(M_MH_ADDHOST);
	mes.serial(uNewID, ucAlias, sHostName, uShardID, sVersion);
	ManageHostServer->send(mes, from);
	// Log기록
	ucstring	ucZone	= GetRegZoneNameOfShard(uShardID);
	ucstring	ucShard	= GetRegShardName(uShardID);
	ucstring	ucHost	= ucstring(sHostName);
	ucstring	ucLog	= L"Add Host(ID=" +toStringW(uNewID) + L") : Host=" + ucHost;
	AddManageHostLog(LOG_MANAGE_ADDHOST, from, ucLog);
}
// Host추가
static void cb_M_MH_ADDHOST( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	string	sPower = pManagerInfo->m_sPower;

	// 편집가능한 관리자인가
	if (!IsComponentManager(sPower))
		return;

	uint32	uShardID;
	string	sHostName;
	msgin.serial(uShardID, sHostName);

	MAKEQUERY_ADDHOST(strQ, ucstring(sHostName).c_str());
	DBCaller->executeWithParam(strQ, DBCB_DBAddHost,
		"DsD",
		CB_,		uShardID,
		CB_,		sHostName.c_str(),
		CB_,		from);

/*
	uint32	uNewID;
	{
		MAKEQUERY_ADDHOST(strQ, ucstring(sHostName).c_str());
		DB_EXE_QUERY(DBForManage, Stmt, strQ);
		if (nQueryResult != TRUE)
			return;
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (IS_DB_ROW_VALID(row))
		{
			COLUMN_DIGIT(row, 0, uint32, uID);
			uNewID = uID;

			ucstring	ucAlias = L"";
			string		sVersion = "Unknown";
			map_REGHOSTINFO.insert( make_pair(uNewID, REGHOSTINFO(uNewID, ucAlias, sHostName, uShardID, sVersion)) );

			// Send
			CMessage	mes(M_MH_ADDHOST);
			mes.serial(uNewID, ucAlias, sHostName, uShardID, sVersion);
			ManageHostServer->send(mes, from);
		}
	}
	// Log기록
	ucstring	ucZone	= GetRegZoneNameOfShard(uShardID);
	ucstring	ucShard	= GetRegShardName(uShardID);
	ucstring	ucHost	= ucstring(sHostName);
	ucstring	ucLog	= L"Add Host(ID=" +toStringW(uNewID) + L") : Host=" + ucHost;
	AddManageHostLog(LOG_MANAGE_ADDHOST, from, ucLog);
*/
}
// Host삭제
static void cb_M_MH_DELHOST( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	string	sPower = pManagerInfo->m_sPower;

	// 편집가능한 관리자인가
	if (!IsComponentManager(sPower))
		return;
	
	uint32	uHostID;
	msgin.serial(uHostID);

	// DB에서 삭제
	MAKEQUERY_DELETESERVER(strQ, uHostID);
	DBCaller->execute(strQ);
/*	
	DB_EXE_QUERY(DBForManage, Stmt, strQ);
	if (nQueryResult != TRUE)
		return;
*/
	// Log기록
	bool	bRegister = true;
	ucstring	ucZone	= GetRegZoneNameOfHost(uHostID, bRegister);
	ucstring	ucShard	= GetRegShardNameOfHost(uHostID, bRegister);
	ucstring	ucHost	= ucstring(GetRegHostName(uHostID, bRegister));
	ucstring	ucLog	= L"Delete host : Host=" + ucHost;
	AddManageHostLog(LOG_MANAGE_DELHOST, from, ucLog);

	// 자료에서 삭제
	DeleteRegHost(uHostID);

	// 접속한 관리자들에게 통지
	CMessage	mes(M_MH_DELHOST);
	mes.serial(uHostID);
	ManageHostServer->send(mes, from);
}

static void	DBCB_DBAddShard(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		uZoneID		= *(uint32 *)argv[0];
	ucstring	ucShardName	= (ucchar *)argv[1];
	TSockId	from			= *(TSockId *)argv[2];

	if (!nQueryResult)
		return;
	uint32	nRows;		resSet->serial(nRows);
	if (nRows <= 0)
		return;

	uint32 uNewID;		resSet->serial(uNewID);
	map_REGSHARDINFO.insert( make_pair(uNewID, REGSHARDINFO(uNewID, ucShardName, uZoneID)) );

	// Send
	CMessage	mes(M_MH_ADDSHARD);
	mes.serial(uNewID, ucShardName, uZoneID);
	ManageHostServer->send(mes, from);
	// Log기록
	ucstring	ucZone	= GetRegZoneName(uZoneID);
	ucstring	ucLog	= L"Add Shard(ID=" +toStringW(uNewID) + L") : Zone=" + ucZone + L", Shard=" + ucShardName;
	AddManageHostLog(LOG_MANAGE_ADDSHARD, from, ucLog);
}
// Shard추가
static void cb_M_MH_ADDSHARD( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	string	sPower = pManagerInfo->m_sPower;

	// 편집가능한 관리자인가
	if (!IsComponentManager(sPower))
		return;

	uint32		uZoneID;
	ucstring	ucShardName;
	msgin.serial(uZoneID, ucShardName);

	MAKEQUERY_ADDSHARD(strQ, ucShardName.c_str(), uZoneID);
	DBCaller->executeWithParam(strQ, DBCB_DBAddShard,
		"DSD",
		CB_,		uZoneID,
		CB_,		ucShardName.c_str(),
		CB_,		from);
}
// Shard삭제
static void cb_M_MH_DELSHARD( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	string	sPower = pManagerInfo->m_sPower;

	// 편집가능한 관리자인가
	if (!IsComponentManager(sPower))
		return;

	uint32	uID;
	msgin.serial(uID);

	// DB에서 삭제
	MAKEQUERY_DELSHARD(strQ, uID);
	DBCaller->execute(strQ);
/*	
	DB_EXE_QUERY(DBForManage, Stmt, strQ);
	if (nQueryResult != TRUE)
		return;
*/
	// Log기록
	ucstring	ucZone	= GetRegZoneNameOfShard(uID);
	ucstring	ucShard	= GetRegShardName(uID);
	ucstring	ucLog	= L"Delete shard : Zone=" + ucZone + L", Shard=" + ucShard;
	AddManageHostLog(LOG_MANAGE_DELSHARD, from, ucLog);

	// 자료에서 삭제
	DeleteRegShard(uID);

	// 접속한 관리자들에게 통지
	CMessage	mes(M_MH_DELSHARD);
	mes.serial(uID);
	ManageHostServer->send(mes, from);

}
static TCallbackItem CallbackArray[] =
{
	{ M_MH_COMPINFO,		cb_M_MH_COMPINFO },
	{ M_MH_ADDSERVICE,		cb_M_MH_ADDSERVICE },
	{ M_MH_DELSERVICE,		cb_M_MH_DELSERVICE },
	{ M_MH_ADDHOST,			cb_M_MH_ADDHOST },
	{ M_MH_DELHOST,			cb_M_MH_DELHOST },
	{ M_MH_ADDSHARD,		cb_M_MH_ADDSHARD },
	{ M_MH_DELSHARD,		cb_M_MH_DELSHARD },
};

void	LoadAdminConfig()
{
	map_MSTSERVICECONFIG.clear();

	CConfigFile::CVar* varExe	= IService::getInstance()->ConfigFile.getVarPtr ("RegExeInfoList");

	if (varExe == NULL)
	{
		sipwarning("There is no 'RegExeInfoList' config.");
		return;
	}
	uint32	uExeNum		= varExe->size();
	for (uint32 i = 0; i < uExeNum; i= i+5)
	{
		std::string		sAlias		= varExe->asString(i);
		std::string		sShortName	= varExe->asString(i+1);
		std::string		sExe	= varExe->asString(i+2);
		std::string		sPath	= varExe->asString(i+3);
		uint32			nCfgFlag= varExe->asInt(i+4);

		if ( sShortName == AS_S_NAME || sShortName == AES_S_NAME )
			continue;

		uint32			uIndex = i / 5;
		MSTSERVICECONFIG	info(uIndex, sAlias, sShortName, sExe, sPath, nCfgFlag);
		map_MSTSERVICECONFIG.insert(make_pair(uIndex, info));
	}
}

void	InitManageComponent()
{
	ManageHostServer->addCallbackArray (CallbackArray, sizeof(CallbackArray)/sizeof(TCallbackItem));
	
	LoadAdminConfig();
	LoadREGSERVICEINFO();
	LoadREGHOSTINFO();
	LoadREGSHARDINFO();
	LoadREGZONEINFO();
}

string	GetHostNameForMV(uint32 _uHostSvcID)
{
	if (_uHostSvcID == 0)
		return "*";
	return toStringA(_uHostSvcID);
}
uint32	GetHostIDFromMVName(string _sHost)
{
	if (_sHost != "*")
	{
		return atoi(_sHost.c_str());
	}
	return 0;
}
