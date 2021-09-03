#ifndef PLAYERMANAGER_PLAYER_H
#define PLAYERMANAGER_PLAYER_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "../Common/Common.h"

struct _ExpRoomGroup
{
	uint16		groupId;
	ucstring	groupName;
	uint8		groupIndex;

	_ExpRoomGroup(uint16 _groupId, ucstring &_groupName, uint8 _groupIndex)
		: groupId(_groupId), groupIndex(_groupIndex)
	{
		groupName = _groupName;
	}
};
typedef std::vector<_ExpRoomGroup> TExpRoomGroups;

struct _ExpRoom
{
	uint16		groupId;
	uint8		roomIndex;
	uint32		photoId;

	T_ROOMID	RoomId;
	T_FAMILYID	FamilyId;
	T_NAME		RoomName;
	T_SCENEID	SceneId;
	T_FLAG		OpenFlag;
	T_NAME		MasterName;
	uint32		visits;

	T_NAME		DeadName1;
	uint8		DeadSurnamelen1;
	T_NAME		DeadName2;
	uint8		DeadSurnamelen2;

	uint8		PhotoType;

	_ExpRoom(uint16 _groupId, uint8 _roomIndex, uint32 _photoId,
		T_ROOMID _roomId, T_FAMILYID _fId, T_NAME _roomName, T_SCENEID _sceneId, 
		T_FLAG _openFlag, T_NAME _DeadName1, uint8 _DeadSurnamelen1, T_NAME _DeadName2, uint8 _DeadSurnamelen2, T_NAME _fName, uint32 _visit, uint8 _photoType)
		:	groupId(_groupId), roomIndex(_roomIndex), photoId(_photoId),
		RoomId(_roomId), FamilyId(_fId), RoomName(_roomName), SceneId(_sceneId),
		OpenFlag(_openFlag), MasterName(_fName), visits(_visit), PhotoType(_photoType)
	{
		DeadName1 = _DeadName1;
		DeadSurnamelen1 = _DeadSurnamelen1;
		DeadName2 = _DeadName2;
		DeadSurnamelen2 = _DeadSurnamelen2;
	}
};
typedef std::vector<_ExpRoom> TExpRooms;

struct _TopListRoom
{
	T_ROOMID	RoomId;
	T_FAMILYID	FamilyId;
	T_NAME		RoomName;
	T_SCENEID	SceneId;
	T_FLAG		OpenFlag;
	T_NAME		MasterName;
	T_ROOMLEVEL	level;
	T_ROOMEXP	exp;
	uint32		visits;
	//uint16		ExpPercent;

	T_NAME		DeadName1;
	uint8		DeadSurnamelen1;
	T_NAME		DeadName2;
	uint8		DeadSurnamelen2;

	uint8		PhotoType;

	_TopListRoom()   {}
	_TopListRoom(T_ROOMID _roomId, T_FAMILYID _fId, T_NAME _roomName, T_SCENEID _sceneId, 
		T_FLAG _openFlag, T_NAME _DeadName1, uint8 _DeadSurnamelen1, T_NAME _DeadName2, uint8 _DeadSurnamelen2, T_NAME _fName, T_ROOMLEVEL _level, T_ROOMEXP _exp, uint32 _visit, uint8 _photoType)
		:	RoomId(_roomId), FamilyId(_fId), RoomName(_roomName), SceneId(_sceneId),
		OpenFlag(_openFlag), MasterName(_fName), level(_level), exp(_exp), visits(_visit), PhotoType(_photoType)
	{
		DeadName1 = _DeadName1;
		DeadSurnamelen1 = _DeadSurnamelen1;
		DeadName2 = _DeadName2;
		DeadSurnamelen2 = _DeadSurnamelen2;
		//ExpPercent = CalcRoomExpPercent(level, exp);
	}
};
typedef std::vector<_TopListRoom> TTopListRooms;

struct _TopListFamily
{
	T_FAMILYID	FID;
	T_NAME		FamilyName;
	T_SEX		Sex;
	uint32		City;
	T_F_LEVEL	Level;
	T_F_EXP		Exp;
	//uint16		ExpPercent;
	//uint32		MaxExp;

	_TopListFamily()   {}
	_TopListFamily(T_FAMILYID _FID, T_FAMILYNAME _FamilyName, T_SEX _Sex, uint32 _City, T_F_LEVEL _Level, T_F_EXP _Exp)
		:	FID(_FID), Sex(_Sex), City(_City), Level(_Level), Exp(_Exp)
	{
		FamilyName = _FamilyName;
		//ExpPercent = CalcFamilyExpPercent(_Level, _Exp, &MaxExp);
	}
};
typedef std::vector<_TopListFamily> TTopListFamilys;

void LoadTopList();
void SendTopList(T_FAMILYID FID, SIPNET::TServiceId _tsid);

void LoadBestsellItemList();
void SendBestSellItems(T_FAMILYID FID, SIPNET::TServiceId _tsid);

void LoadExpRoomList();
void SendExpRoomList(T_FAMILYID FID, SIPNET::TServiceId _tsid);

#endif //PLAYERMANAGER_PLAYER_H
