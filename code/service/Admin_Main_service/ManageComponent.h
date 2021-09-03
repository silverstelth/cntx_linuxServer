#ifndef ADMIN_MANAGE_COMPONENT_H
#define ADMIN_MANAGE_COMPONENT_H

#include "admin_main_service.h"

extern	MAP_REGSERVICEINFO		map_REGSERVICEINFO;
extern	MAP_REGSERVICEINFO		map_UNREGSERVICEINFO;
extern	MAP_REGHOSTINFO			map_REGHOSTINFO;
extern	MAP_UNREGHOSTINFO		map_UNREGHOSTINFO;
extern	MAP_REGSHARDINFO		map_REGSHARDINFO;
extern	MAP_REGZONEINFO			map_REGZONEINFO;
extern	MAP_REGPORT				map_REGPORT;

#define	FOR_START_MAP_REGSERVICEINFO(it)											\
	MAP_REGSERVICEINFO::iterator	it;												\
	for(it = map_REGSERVICEINFO.begin(); it != map_REGSERVICEINFO.end(); it++)		\

#define	FOR_START_MAP_UNREGSERVICEINFO(it)											\
	MAP_REGSERVICEINFO::iterator	it;												\
	for(it = map_UNREGSERVICEINFO.begin(); it != map_UNREGSERVICEINFO.end(); it++)	\

#define	FOR_START_MAP_REGHOSTINFO(it)											\
	MAP_REGHOSTINFO::iterator	it;												\
	for(it = map_REGHOSTINFO.begin(); it != map_REGHOSTINFO.end(); it++)		\

#define	FOR_START_MAP_REGSHARDINFO(it)											\
	MAP_REGSHARDINFO::iterator	it;												\
	for(it = map_REGSHARDINFO.begin(); it != map_REGSHARDINFO.end(); it++)		\

#define	FOR_START_MAP_REGZONEINFO(it)											\
	MAP_REGZONEINFO::iterator	it;												\
	for(it = map_REGZONEINFO.begin(); it != map_REGZONEINFO.end(); it++)		\

extern	void		InitManageComponent();
extern	void		Send_M_MH_MSTHOSTINFO(SIPNET::TSockId from, std::string _sPower);
extern	void		Send_M_MH_IDENT();

extern	uint16		GetHostID(ucstring _ucShardName, std::string _sHostName);
extern	uint32		GetRegisteredHostID(ucstring _ucShardName, std::string _sHostName);
extern	ucstring	GetRegZoneNameOfShard(uint32 _uID);
extern	ucstring	GetRegShardName(uint32 _uID);
extern	ucstring	GetRegShardNameOfHost(uint32 _uID, bool	bRegister);
extern	ucstring	GetRegZoneNameOfHost(uint32 _uID, bool bRegister);
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//extern	uint32		GetHostIDOfService(uint16 _uSvcID, uint32 _uID);
extern	uint32		GetShardIDOfHost(uint32 uID, bool _bRegister);

extern	ucstring	GetRegZoneNameOfService(uint32 _uID, bool _bRegister);
extern	ucstring	GetRegShardNameOfService(uint32 _uID, bool _bRegister);
extern	std::string	GetRegHostNameOfService(uint32 _uID, bool _bRegister);
extern	std::string	GetMSTServiceShortName(uint32 _uMstID);
extern	std::string	GetMSTServiceExeName(uint32 _uMstID);
extern	uint32		GetMSTServiceIDFromSName(std::string sShortName);
extern	std::string	GetServiceExeNameOfService(uint32 _uID);
extern	std::string	GetMSTServiceShortNameOfService(uint32 _uID, bool _bRegister);
extern	ucstring	GetMSTServiceAliasNameOfService(uint32 _uID);

extern	std::string	GetRegHostName(uint32 _uID, bool _bRegister);
extern	uint32		GetServiceIDFromShortName(uint32 _uHostID, std::string _sShortName);

extern	void		Make_COMPINFO(SIPNET::CMessage *pMes, std::string _sPower);
extern	std::string	GetHostNameForMV(uint32 _uHostSvcID);
extern	uint32		GetHostIDFromMVName(std::string _sHost);
extern	bool		UpdateHostVersion(uint32 _uHostID, std::string _sVersion);

extern	void		DeleteUnRegService(uint32 _uID);
extern	void		DeleteUnRegHost(uint32 _uID);

#endif	// ADMIN_MANAGE_COMPONENT_H