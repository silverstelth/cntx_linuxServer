
#ifndef OPENROOM_ROOM_H
#define OPENROOM_ROOM_H

#include "openroom.h"

#include "../../common/Packet.h"
#include "../../common/common.h"
#include "../Common/Common.h"
// Logoff
// extern	void	cb_M_CS_LOGIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	SendOwnRoomInfo(T_FAMILYID familyId, SIPNET::TServiceId _tsid);
//extern	void	SendOwnFestivalInfo(T_FAMILYID familyId, SIPNET::TServiceId _tsid);
extern	void	SendDeadInfo(T_FAMILYID _FID, T_ROOMID _RoomID, CRoomWorld *_RoomManager, SIPNET::TServiceId _tsid);

extern	uint32	UnDeleteRoom(T_FAMILYID _FID, T_ROOMID _roomid, T_MSTROOM_ID &roomTypeId, T_ROOMNAME &roomName);
extern	void	cb_M_CS_DELROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_UNDELROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// ¹æ°ü·Ã
extern	void	cb_M_CS_FINDROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// reserve festival
extern	void	cb_M_CS_REGFESTIVAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_MODIFYFESTIVAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_CS_DELFESTIVAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern  void	cb_M_CS_SET_FESTIVALALARM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern  void	cb_M_CS_ADD_FESTIVALALARM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

struct	FindRoomResult
{
	T_ROOMID	RoomId;
	T_ROOMLEVEL	level;
	T_ROOMEXP	exp;
	T_ID		FamilyId;
	T_NAME		RoomName;
	T_SCENEID	SceneId;
	T_FLAG		OpenFlag;
	T_NAME		MasterName;
	uint32		VisitCount;
	T_NAME		Domicile1;
	T_NAME		Nation1;
	T_NAME		DeadName1;
	uint8		DeadSurnamelen1;
	T_SEX		DeadSex1;
	uint32		Birthday1;
	uint32		Deadday1;
	T_NAME		Domicile2;
	T_NAME		Nation2;
	T_NAME		DeadName2;
	uint8		DeadSurnamelen2;
	T_SEX		DeadSex2;
	uint32		Birthday2;
	uint32		Deadday2;

	FindRoomResult()	{}
	FindRoomResult(T_ROOMID _rid, T_ROOMLEVEL _level, T_ROOMEXP _exp, T_ID _fid, T_NAME& _rname, T_SCENEID _sceneid, T_FLAG _oflag, T_NAME& _mname, uint32 _VisitCount, 
			T_NAME& _domicile1, T_NAME& _nation1, T_NAME& _DeadName1, uint8 _DeadSurnamelen1, T_SEX _dsex1, uint32 _birth1, uint32 _deadday1, 
			T_NAME& _domicile2, T_NAME& _nation2, T_NAME& _DeadName2, uint8 _DeadSurnamelen2, T_SEX _dsex2, uint32 _birth2, uint32 _deadday2) :
		RoomId(_rid), level(_level), exp(_exp), FamilyId(_fid), RoomName(_rname), SceneId(_sceneid), OpenFlag(_oflag), MasterName(_mname), VisitCount(_VisitCount), 
		Domicile1(_domicile1), Nation1(_nation1), DeadName1(_DeadName1), DeadSurnamelen1(_DeadSurnamelen1), DeadSex1(_dsex1), Birthday1(_birth1), Deadday1(_deadday1), 
		Domicile2(_domicile2), Nation2(_nation2), DeadName2(_DeadName2), DeadSurnamelen2(_DeadSurnamelen2), DeadSex2(_dsex2), Birthday2(_birth2), Deadday2(_deadday2) {}
};

//extern	void	NotifyChangeJingPinTianYuanRoommaster(uint32 RoomID, uint32 NewMasterFID, ucstring NewMasterName);
//extern	void	ChangeJingPinTianYuanRoommaster(uint32 RoomID, uint32 NewMasterFID, ucstring NewMasterName);
extern	void	NotifyChangeJingPinTianYuanInfo(uint32 RoomID);
extern	void	ChangeJingPinTianYuanInfo(uint32 RoomID);
extern SIPBASE::TTime GetUpdateNightTime(SIPBASE::TTime curTime);

//extern	void	ModifyJingPinTianYuanRoommasterName(uint32 FID, ucstring FName);

//struct	CRoom
//{
//	CRoom()
//	{
//	};
//
//	CRoom(uint32 _idRoom, ucstring _nameRoom, uint16 _idSubTypeId, uint32 _idMaster, ucstring _ucFamily, uint8 _freeUse, 
//		std::string _creationDay, std::string _backDay, uint8 _idOpenability, uint32 _nVisits, ucstring _ucDead1, ucstring _ucDead2, bool _bClosed, std::string _renewDay)
//	{
//		idRoom		= _idRoom;
//		nameRoom	= _nameRoom;
//		idSubTypeId	= _idSubTypeId;
//		idMaster	= _idMaster;
//		ucMaster	= _ucFamily;
//		freeUse		= _freeUse;
//
//		creationDay	= _creationDay;
//		backDay		= _backDay;
//		ucDead1		= _ucDead1;
//		ucDead2		= _ucDead2;
//		idOpenability = _idOpenability;
//		nVisits		= _nVisits;
//		bClosed		= _bClosed;
//		renewTime	= _renewDay;
//	};
//
//	uint32		idRoom;
//	ucstring	nameRoom;
//	uint16		idSubTypeId;
//	uint32		idMaster;
//	ucstring	ucMaster;
//	uint8		idOpenability;
//	uint32		nVisits;
//	uint8		freeUse;
//	bool		bClosed;
//	std::string		creationDay;
//	std::string		backDay;
//	ucstring	ucDead1;
//	ucstring	ucDead2;
//	std::string renewTime;
//};
/*
struct	CUserFestival
{
	CUserFestival()
	{
	};

	CUserFestival(uint32 _idFestival, uint32 _idUser, ucstring _ucName, uint32 _date)
	{
		idFestival	= _idFestival;
		idUser		= _idUser;
		ucName		= _ucName;
		date		= _date;		
	};

	uint32		idFestival;
	uint32		idUser;
	ucstring	ucName;
	uint32	    date;	
};
*/

//struct	FavorRoom
//{
//	T_ROOMID	RoomId;
//	T_ID		FamilyId;
//	T_SCENEID	SceneId;
//	T_ID		GroupId;
//	T_NAME		RoomName;
//	T_FLAG		nOpenFlag;
//	T_NAME		MasterName;
//	T_M1970	ExpireTime;
//	//	uint8		DeleteFlag;
//	uint32		DeleteRemainMinutes;
//	T_ROOMLEVEL	level;
//	T_ROOMEXP	exp;
//	uint32		VisitCount;
//	T_NAME		DeadName1;
//	uint8		DeadSurnamelen1;
//	T_NAME		DeadName2;
//	uint8		DeadSurnamelen2;
//
//	FavorRoom()	{}
//	FavorRoom(T_ROOMID _rid, T_ID _fid, T_SCENEID _sceneid, T_ID _gid, T_NAME& _rname, T_FLAG _oflag, 
//		T_NAME& _mname, T_NAME& _DeadName1, uint8 _DeadSurnamelen1, T_NAME& _DeadName2, uint8 _DeadSurnamelen2, T_M1970 _expire, uint32 _deleteremainminutes, T_ROOMLEVEL _level, T_ROOMEXP _exp, uint32 _VisitCount) :
//	RoomId(_rid), FamilyId(_fid), SceneId(_sceneid), GroupId(_gid), RoomName(_rname), nOpenFlag(_oflag), 
//		MasterName(_mname), DeadName1(_DeadName1), DeadSurnamelen1(_DeadSurnamelen1), DeadName2(_DeadName2), DeadSurnamelen2(_DeadSurnamelen2), ExpireTime(_expire), DeleteRemainMinutes(_deleteremainminutes), level(_level), exp(_exp), VisitCount(_VisitCount)
//	{}
//};
//
//#pragma pack(push, 1)
//struct _FavorroomGroupInfo
//{
//	uint32		GroupID;
//	ucchar		GroupName[MAX_FAVORGROUPNAME + 1];
//};
//#pragma pack(pop)
//
//typedef SDATA_LIST<_FavorroomGroupInfo, MAX_FAVORROOMGROUP>	FavorroomGroupList;
//
//void SaveFavorroomGroup(uint32 FID, FavorroomGroupList *pFavorroomGroupList);
//bool ReadFavorroomGroup(uint32 FID, FavorroomGroupList *pFavorroomGroupList, bool bAddDefault = false);

//extern	bool			g_IsMasterOroomService;

#endif // OPENROOM_FAMILY_H
