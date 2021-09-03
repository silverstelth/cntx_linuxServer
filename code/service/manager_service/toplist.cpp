#include <misc/ucstring.h>
#include <misc/DBCaller.h>
#include <net/service.h>

#include "../Common/QuerySetForShard.h"
#include "../openroom/mst_room.h"
#include "toplist.h"
#include "main.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller	*DBCaller;


//================================================
// Global Variables
//================================================
static TExpRoomGroups	aryExpRoomGroup;
static TExpRooms		aryExpRoom;

static TTopListRooms	aryTopListByRoomExp;
static TTopListRooms	aryTopListByRoomVisit;
static TTopListFamilys	aryTopListByFamilyExp;

static std::list<uint32>	g_BestSellItemList;


static void	DBCB_DBGetExpRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);

	for (uint32 i = 0; i < nRows; i ++)
	{
		T_ROOMID	roomid;				resSet->serial(roomid);
		T_ROOMNAME	roomname;			resSet->serial(roomname);
		uint8		deadSurnameLen1;	resSet->serial(deadSurnameLen1);
		T_DECEASED_NAME	deadname1;		resSet->serial(deadname1);
		uint8		deadSurnameLen2;	resSet->serial(deadSurnameLen2);
		T_DECEASED_NAME	deadname2;		resSet->serial(deadname2);
		T_FAMILYNAME	mastername;		resSet->serial(mastername);
		uint32		visit;				resSet->serial(visit);
		uint8		level;				resSet->serial(level);
		uint32		exp;				resSet->serial(exp);
		T_SCENEID	sceneid;			resSet->serial(sceneid);
		T_FLAG		openflag;			resSet->serial(openflag);
		T_FAMILYID	familyid;			resSet->serial(familyid);
		uint8		roomIndex;			resSet->serial(roomIndex);
		uint16		groupId;			resSet->serial(groupId);
		uint32		photoId;			resSet->serial(photoId);
		uint8		photoType;			resSet->serial(photoType);

		_ExpRoom data(groupId, roomIndex, photoId, roomid, familyid, roomname, sceneid, openflag, 
			deadname1, deadSurnameLen1, deadname2, deadSurnameLen2, mastername, visit, photoType);

		aryExpRoom.push_back(data);

//		sipdebug("Exproom New i=%d RoomID=%d", i, roomid);
	}
}

static void	DBCB_DBGetExpRoomGroup(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);

	for (uint32 i = 0; i < nRows; i ++)
	{
		uint16		groupId;			resSet->serial(groupId);
		ucstring	groupName;			resSet->serial(groupName);
		uint8		groupIndex;			resSet->serial(groupIndex);

		_ExpRoomGroup data(groupId, groupName, groupIndex);
		aryExpRoomGroup.push_back(data);
	}
}

void LoadExpRoomList()
{
	aryExpRoomGroup.clear();
	aryExpRoom.clear();

	{
		MAKEQUERY_GetExpRoomGroup(strQ);
		DBCaller->execute(strQ, DBCB_DBGetExpRoomGroup);
	}

	{
		MAKEQUERY_GetExpRoom(strQ);
		DBCaller->execute(strQ, DBCB_DBGetExpRoom);
	}
}

static void	DBCB_DBGetTopList_RoomExp(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	sipdebug("TopListByRoomExp count=%d", nRows);

	for (uint32 i = 0; i < nRows; i ++)
	{
		T_ROOMID	roomid;				resSet->serial(roomid);
		T_ROOMNAME	roomname;			resSet->serial(roomname);
		uint8		deadSurnameLen1;	resSet->serial(deadSurnameLen1);
		T_DECEASED_NAME	deadname1;		resSet->serial(deadname1);
		uint8		deadSurnameLen2;	resSet->serial(deadSurnameLen2);
		T_DECEASED_NAME	deadname2;		resSet->serial(deadname2);
		T_FAMILYNAME	mastername;		resSet->serial(mastername);
		uint32		visit;				resSet->serial(visit);
		T_ROOMLEVEL level;				resSet->serial(level);
		T_ROOMEXP	exp;				resSet->serial(exp);
		T_SCENEID	sceneid;			resSet->serial(sceneid);
		T_FLAG		openflag;			resSet->serial(openflag);
		T_FAMILYID	familyid;			resSet->serial(familyid);
		uint8		photoType;			resSet->serial(photoType);

		_TopListRoom data(roomid, familyid, roomname, sceneid, openflag, 
			deadname1, deadSurnameLen1, deadname2, deadSurnameLen2, mastername, level, exp, visit, photoType);
		aryTopListByRoomExp.push_back(data);

//		sipdebug("TopListByRoomExp i=%d RoomID=%d", i, roomid);
	}
}

static void	DBCB_DBGetTopList_RoomVisit(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	sipdebug("TopListByRoomVisit count=%d", nRows);

	for (uint32 i = 0; i < nRows; i ++)
	{
		T_ROOMID	roomid;				resSet->serial(roomid);
		T_ROOMNAME	roomname;			resSet->serial(roomname);
		uint8		deadSurnameLen1;	resSet->serial(deadSurnameLen1);
		T_DECEASED_NAME	deadname1;		resSet->serial(deadname1);
		uint8		deadSurnameLen2;	resSet->serial(deadSurnameLen2);
		T_DECEASED_NAME	deadname2;		resSet->serial(deadname2);
		T_FAMILYNAME	mastername;		resSet->serial(mastername);
		uint32		visit;				resSet->serial(visit);
		T_ROOMLEVEL level;				resSet->serial(level);
		T_ROOMEXP	exp;				resSet->serial(exp);
		T_SCENEID	sceneid;			resSet->serial(sceneid);
		T_FLAG		openflag;			resSet->serial(openflag);
		T_FAMILYID	familyid;			resSet->serial(familyid);
		uint8		photoType;			resSet->serial(photoType);

		_TopListRoom data(roomid, familyid, roomname, sceneid, openflag, 
			deadname1, deadSurnameLen1, deadname2, deadSurnameLen2, mastername, level, exp, visit, photoType);
		aryTopListByRoomVisit.push_back(data);

//		sipdebug("TopListByRoomVisit i=%d RoomID=%d", i, roomid);
	}
}

static void	DBCB_DBGetTopList_FamilyExp(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	sipdebug("TopListByFamilyExp count=%d", nRows);

	for (uint32 i = 0; i < nRows; i ++)
	{
		T_FAMILYNAME	FamilyName;		resSet->serial(FamilyName);
		T_FAMILYID		FID;			resSet->serial(FID);
		T_SEX			Sex;			resSet->serial(Sex);
		uint32			City;			resSet->serial(City);
		T_F_LEVEL		Level;			resSet->serial(Level);
		T_F_EXP			Exp;			resSet->serial(Exp);

		_TopListFamily data(FID, FamilyName, Sex, City, Level, Exp);
		aryTopListByFamilyExp.push_back(data);

//		sipdebug("TopListByFamilyExp i=%d FID=%d", i, FID);
	}
}

void LoadTopList()
{
	if (ShardMainCode[0] == 0)
		return;

	aryTopListByRoomExp.clear();
	aryTopListByRoomVisit.clear();
	aryTopListByFamilyExp.clear();

	ucstring	roomCondition, familyCondition;
	roomCondition = L"a.roomid>6000 and (";
	familyCondition = L"a.[FamilyID]>6000 and (";

	tchar str0[30], str1[30], str2[30];
	for (int i = 0; i < sizeof(ShardMainCode); i ++)
	{
		if (ShardMainCode[i] == 0)
			break;
		uint32 id0 = (ShardMainCode[i] << 24);
		smprintf(str0, 30, _t("%d"), id0);
		smprintf(str1, 30, _t("%d"), id0 - 0x01000000);
		smprintf(str2, 30, _t("%d"), id0 + 0x01000000);

		if (i != 0)
		{
			roomCondition = roomCondition + L" or ";
			familyCondition = familyCondition + L" or ";
		}
		roomCondition = roomCondition + L"(a.roomid>=" + str0 + L" and a.roomid<" + str2 + L")";
		familyCondition = familyCondition + L"(a.[FamilyID]>=" + str1 + L" and a.[FamilyID]<" + str0 + L")";
	}
	roomCondition = roomCondition + L")";
	familyCondition = familyCondition + L")";

	{
		MAKEQUERY_GetTopList_RoomExp(strQ, roomCondition);
		DBCaller->execute(strQ, DBCB_DBGetTopList_RoomExp);
	}

	{
		MAKEQUERY_GetTopList_RoomVisit(strQ, roomCondition);
		DBCaller->execute(strQ, DBCB_DBGetTopList_RoomVisit);
	}

	{
		MAKEQUERY_GetTopList_FamilyExp(strQ, familyCondition);
		DBCaller->execute(strQ, DBCB_DBGetTopList_FamilyExp);
	}
}

//void cb_M_CS_TOPLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);

void SendTopList(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	{
		uint8	bStart, bEnd;
		bStart = 1;
		CMessage msgOut(M_SC_TOPLIST_ROOMEXP);
		msgOut.serial(FID, bStart);
		for (uint32 i = 0; i < aryTopListByRoomExp.size(); i ++)
		{
			// Packet??길이가 ?�무 길어???�라??보낸??
			if (msgOut.length() > 2500)
			{
				bEnd = 0;
				msgOut.serial(bEnd);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

				CMessage	msgtmp(M_SC_TOPLIST_ROOMEXP);
				bStart = 0;
				msgtmp.serial(FID, bStart);

				msgOut = msgtmp;
			}

			_TopListRoom &data = aryTopListByRoomExp[i];

			msgOut.serial(data.RoomId, data.SceneId, data.RoomName, data.MasterName, data.FamilyId, data.visits);
			msgOut.serial(data.level, data.exp, /*data.ExpPercent,*/ data.OpenFlag, data.PhotoType);
			msgOut.serial(data.DeadSurnamelen1, data.DeadName1, data.DeadSurnamelen2, data.DeadName2);
		}

		bEnd = 1;
		msgOut.serial(bEnd);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	{
		uint8	bStart, bEnd;
		bStart = 1;
		CMessage msgOut(M_SC_TOPLIST_ROOMVISIT);
		msgOut.serial(FID, bStart);
		for (uint32 i = 0; i < aryTopListByRoomVisit.size(); i ++)
		{
			// Packet??길이가 ?�무 길어???�라??보낸??
			if (msgOut.length() > 2500)
			{
				bEnd = 0;
				msgOut.serial(bEnd);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

				CMessage	msgtmp(M_SC_TOPLIST_ROOMVISIT);
				bStart = 0;
				msgtmp.serial(FID, bStart);

				msgOut = msgtmp;
			}

			_TopListRoom &data = aryTopListByRoomVisit[i];

			msgOut.serial(data.RoomId, data.SceneId, data.RoomName, data.MasterName, data.FamilyId, data.visits);
			msgOut.serial(data.level, data.exp, /*data.ExpPercent,*/ data.OpenFlag, data.PhotoType);
			msgOut.serial(data.DeadSurnamelen1, data.DeadName1, data.DeadSurnamelen2, data.DeadName2);
		}

		bEnd = 1;
		msgOut.serial(bEnd);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	{
		uint8	bStart, bEnd;
		bStart = 1;
		CMessage msgOut(M_SC_TOPLIST_FAMILYEXP);
		msgOut.serial(FID, bStart);
		for (uint32 i = 0; i < aryTopListByFamilyExp.size(); i ++)
		{
			// Packet??길이가 ?�무 길어???�라??보낸??
			if (msgOut.length() > 2500)
			{
				bEnd = 0;
				msgOut.serial(bEnd);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

				CMessage	msgtmp(M_SC_TOPLIST_FAMILYEXP);
				bStart = 0;
				msgtmp.serial(FID, bStart);

				msgOut = msgtmp;
			}

			_TopListFamily &data = aryTopListByFamilyExp[i];

			msgOut.serial(data.FID, data.FamilyName, data.Sex, data.City, data.Level, data.Exp); // , data.MaxExp, data.ExpPercent);
		}

		bEnd = 1;
		msgOut.serial(bEnd);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
}

//void cb_M_CS_EXPROOMS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);

void SendExpRoomList(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	{
		CMessage msgOut(M_SC_EXPROOM_GROUP);
		msgOut.serial(FID);
		for (uint32 i = 0; i < aryExpRoomGroup.size(); i ++)
		{
			_ExpRoomGroup &data = aryExpRoomGroup[i];
			msgOut.serial(data.groupId, data.groupName, data.groupIndex);
		}

		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	{
		uint8	bStart, bEnd;
		bStart = 1;
		CMessage msgOut(M_SC_EXPROOMS);
		msgOut.serial(FID, bStart);
		for (uint32 i = 0; i < aryExpRoom.size(); i ++)
		{
			// Packet??길이가 ?�무 길어???�라??보낸??
			if (msgOut.length() > 2500)
			{
				bEnd = 0;
				msgOut.serial(bEnd);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

				CMessage	msgtmp(M_SC_EXPROOMS);
				bStart = 0;
				msgtmp.serial(FID, bStart);

				msgOut = msgtmp;
			}

			_ExpRoom &data = aryExpRoom[i];

			msgOut.serial(data.groupId, data.photoId, data.roomIndex);
			msgOut.serial(data.RoomId, data.SceneId, data.RoomName, data.MasterName);
			msgOut.serial(data.FamilyId, data.visits);
			msgOut.serial(data.OpenFlag, data.PhotoType);
			msgOut.serial(data.DeadName1, data.DeadSurnamelen1, data.DeadName2, data.DeadSurnamelen2);
		}

		bEnd = 1;
		msgOut.serial(bEnd);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
}

void cb_M_GMCS_EXPROOM_GROUP_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	ucstring		GroupName;	_msgin.serial(GroupName);
	uint8			GroupIndex;	_msgin.serial(GroupIndex);
	uint16			GroupID;	_msgin.serial(GroupID);

	for (TExpRoomGroups::iterator it = aryExpRoomGroup.begin(); it != aryExpRoomGroup.end(); it ++)
	{
		if (it->groupId == GroupID)
		{
			sipwarning("Exist Same GroupID!!! GroupIndex=%d, GroupID=%d", GroupIndex, GroupID);
			return;
		}
	}

	_ExpRoomGroup	data(GroupID, GroupName, GroupIndex);
	aryExpRoomGroup.push_back(data);

	MAKEQUERY_InsertExpRoomGroup(strQ, GroupName, GroupIndex, GroupID);
	DBCaller->execute(strQ);
}

void cb_M_GMCS_EXPROOM_GROUP_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	uint16			GroupID;	_msgin.serial(GroupID);
	ucstring		GroupName;	_msgin.serial(GroupName);
	uint8			GroupIndex;	_msgin.serial(GroupIndex);

	for (TExpRoomGroups::iterator it = aryExpRoomGroup.begin(); it != aryExpRoomGroup.end(); it ++)
	{
		if (it->groupId == GroupID)
		{
			it->groupName = GroupName;
			it->groupIndex = GroupIndex;

			MAKEQUERY_ModifyExpRoomGroup(strQ, GroupID, GroupName, GroupIndex);
			DBCaller->execute(strQ);

			break;
		}
	}
}

void cb_M_GMCS_EXPROOM_GROUP_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	uint16			GroupID;	_msgin.serial(GroupID);

	for (TExpRoomGroups::iterator it = aryExpRoomGroup.begin(); it != aryExpRoomGroup.end(); it ++)
	{
		if (it->groupId == GroupID)
		{
			aryExpRoomGroup.erase(it);

			MAKEQUERY_DeleteExpRoomGroup(strQ, GroupID);
			DBCaller->execute(strQ);

			break;
		}
	}
}

static void	DBCB_DBInsertExpRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	T_ROOMID	RoomID = *(T_ROOMID *)argv[0];
	uint16		GroupId = *(uint16 *)argv[1];
	uint8		RoomIndex = *(uint8 *)argv[2];
	uint32		PhotoID = *(uint32 *)argv[3];

	uint32		nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("Invalid nRows. nRows=%d", nRows);
		return;
	}

	{
		T_ROOMID	roomid;				resSet->serial(roomid);
		T_ROOMNAME	roomname;			resSet->serial(roomname);
		uint8		deadSurnameLen1;	resSet->serial(deadSurnameLen1);
		T_DECEASED_NAME	deadname1;		resSet->serial(deadname1);
		uint8		deadSurnameLen2;	resSet->serial(deadSurnameLen2);
		T_DECEASED_NAME	deadname2;		resSet->serial(deadname2);
		T_FAMILYNAME	mastername;		resSet->serial(mastername);
		uint32		visit;				resSet->serial(visit);
		uint8		level;				resSet->serial(level);
		uint32		exp;				resSet->serial(exp);
		T_SCENEID	sceneid;			resSet->serial(sceneid);
		T_FLAG		openflag;			resSet->serial(openflag);
		T_FAMILYID	familyid;			resSet->serial(familyid);
		uint8		photoType;			resSet->serial(photoType);

		_ExpRoom data(GroupId, RoomIndex, PhotoID, roomid, familyid, roomname, sceneid, openflag, 
			deadname1, deadSurnameLen1, deadname2, deadSurnameLen2, mastername, visit, photoType);

		aryExpRoom.push_back(data);
	}
}

void cb_M_GMCS_EXPROOM_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	uint32			RoomID;		_msgin.serial(RoomID);
	uint16			GroupId;	_msgin.serial(GroupId);
	uint8			RoomIndex;	_msgin.serial(RoomIndex);
	uint32			PhotoID;	_msgin.serial(PhotoID);

	for (TExpRooms::iterator it = aryExpRoom.begin(); it != aryExpRoom.end(); it ++)
	{
		if (it->RoomId == RoomID)
		{
			sipwarning("Already exist. RoomID=%d", RoomID);
			return;
		}
	}

	MAKEQUERY_InsertExpRoom(strQ, RoomID, GroupId, RoomIndex, PhotoID);
	DBCaller->executeWithParam(strQ, DBCB_DBInsertExpRoom,
		"DWBD",
		CB_,	RoomID,
		CB_,	GroupId,
		CB_,	RoomIndex,
		CB_,	PhotoID);
}

void cb_M_GMCS_EXPROOM_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	uint32			RoomID;		_msgin.serial(RoomID);
	uint16			GroupID;	_msgin.serial(GroupID);
	uint8			RoomIndex;	_msgin.serial(RoomIndex);
	uint32			PhotoID;	_msgin.serial(PhotoID);

	for (TExpRooms::iterator it = aryExpRoom.begin(); it != aryExpRoom.end(); it ++)
	{
		if (it->RoomId == RoomID)
		{
			it->groupId = GroupID;
			it->roomIndex = RoomIndex;
			it->photoId = PhotoID;

			MAKEQUERY_ModifyExpRoom(strQ, RoomID, GroupID, RoomIndex, PhotoID);
			DBCaller->execute(strQ);
			break;
		}
	}
}

void cb_M_GMCS_EXPROOM_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	uint32			RoomID;		_msgin.serial(RoomID);

	for (TExpRooms::iterator it = aryExpRoom.begin(); it != aryExpRoom.end(); it ++)
	{
		if (it->RoomId == RoomID)
		{
			aryExpRoom.erase(it);

			MAKEQUERY_DeleteExpRoom(strQ, RoomID);
			DBCaller->execute(strQ);

			break;
		}
	}
}

static void	DBCB_DBGetRoomInfoForJingPinTianYuan(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ROOMID	RoomID	= *(T_ROOMID *)argv[0];

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

			for(uint32 j = 0; j < aryTopListByRoomExp.size(); j ++)
			{
				_TopListRoom &tempRoom = aryTopListByRoomExp[j];
				if(tempRoom.RoomId == RoomID)
				{
					tempRoom.DeadSurnamelen1 = surnameLen1;
					tempRoom.DeadName1 = deadname1;
					tempRoom.DeadSurnamelen2 = surnameLen2;
					tempRoom.DeadName2 = deadname2;
					tempRoom.FamilyId = fid;
					tempRoom.MasterName = familyName;
					tempRoom.OpenFlag = idOpenability;
					tempRoom.RoomName = roomName;
					tempRoom.SceneId = sceneId;
					tempRoom.exp = exp;
					tempRoom.level = level;
					tempRoom.PhotoType = PhotoType;
					break;
				}
			}
			for(uint32 j = 0; j < aryTopListByRoomVisit.size(); j ++)
			{
				_TopListRoom &tempRoom = aryTopListByRoomVisit[j];
				if(tempRoom.RoomId == RoomID)
				{
					tempRoom.DeadSurnamelen1 = surnameLen1;
					tempRoom.DeadName1 = deadname1;
					tempRoom.DeadSurnamelen2 = surnameLen2;
					tempRoom.DeadName2 = deadname2;
					tempRoom.FamilyId = fid;
					tempRoom.MasterName = familyName;
					tempRoom.OpenFlag = idOpenability;
					tempRoom.RoomName = roomName;
					tempRoom.SceneId = sceneId;
					tempRoom.exp = exp;
					tempRoom.level = level;
					tempRoom.PhotoType = PhotoType;
					break;
				}
			}
		}
	}
}

void ChangeJingPinTianYuanInfo(uint32 RoomID)
{
	for(uint32 j = 0; j < aryTopListByRoomExp.size(); j ++)
	{
		_TopListRoom &tempRoom = aryTopListByRoomExp[j];
		if(tempRoom.RoomId == RoomID)
		{
			MAKEQUERY_GetRoomInfo(strQuery, 0, RoomID);
			DBCaller->executeWithParam(strQuery, DBCB_DBGetRoomInfoForJingPinTianYuan,
				"D",
				CB_,			RoomID);
			return;
		}
	}
	for(uint32 j = 0; j < aryTopListByRoomVisit.size(); j ++)
	{
		_TopListRoom &tempRoom = aryTopListByRoomVisit[j];
		if(tempRoom.RoomId == RoomID)
		{
			MAKEQUERY_GetRoomInfo(strQuery, 0, RoomID);
			DBCaller->executeWithParam(strQuery, DBCB_DBGetRoomInfoForJingPinTianYuan,
				"D",
				CB_,			RoomID);
			return;
		}
	}
}

void cb_M_NT_EXPROOM_INFO_RELOAD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32		RoomID;			_msgin.serial(RoomID);

	ChangeJingPinTianYuanInfo(RoomID);
}

void cb_M_NT_EXPROOM_INFO_CHANGED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_ROOMID		roomId;
	T_ROOMNAME		roomName;
	T_SCENEID		sceneId;
	T_FAMILYID		masterId;
	T_FAMILYNAME	masterName;
	T_ROOMPOWERID	idOpenability;
	T_ROOMEXP		exp;
	T_ROOMLEVEL		level;
	uint8			surnameLen1;
	T_DECEASED_NAME	deadname1;
	uint8			surnameLen2;
	T_DECEASED_NAME	deadname2;
	uint8			PhotoType;

	_msgin.serial(roomId, roomName);
	_msgin.serial(sceneId, masterId, masterName, idOpenability, exp, level);
	_msgin.serial(surnameLen1, deadname1, surnameLen2, deadname2, PhotoType);

	for(uint32 j = 0; j < aryTopListByRoomExp.size(); j ++)
	{
		_TopListRoom &tempRoom = aryTopListByRoomExp[j];
		if(tempRoom.RoomId == roomId)
		{
			tempRoom.DeadSurnamelen1 = surnameLen1;
			tempRoom.DeadName1 = deadname1;
			tempRoom.DeadSurnamelen2 = surnameLen2;
			tempRoom.DeadName2 = deadname2;
			tempRoom.FamilyId = masterId;
			tempRoom.MasterName = masterName;
			tempRoom.OpenFlag = idOpenability;
			tempRoom.RoomName = roomName;
			tempRoom.SceneId = sceneId;
			tempRoom.exp = exp;
			tempRoom.level = level;
			tempRoom.PhotoType = PhotoType;
			break;
		}
	}
	for(uint32 j = 0; j < aryTopListByRoomVisit.size(); j ++)
	{
		_TopListRoom &tempRoom = aryTopListByRoomVisit[j];
		if(tempRoom.RoomId == roomId)
		{
			tempRoom.DeadSurnamelen1 = surnameLen1;
			tempRoom.DeadName1 = deadname1;
			tempRoom.DeadSurnamelen2 = surnameLen2;
			tempRoom.DeadName2 = deadname2;
			tempRoom.FamilyId = masterId;
			tempRoom.MasterName = masterName;
			tempRoom.OpenFlag = idOpenability;
			tempRoom.RoomName = roomName;
			tempRoom.SceneId = sceneId;
			tempRoom.exp = exp;
			tempRoom.level = level;
			tempRoom.PhotoType = PhotoType;
			break;
		}
	}
}

static void	DBCB_DBGetBestsellItemList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i ++)
	{
		uint32		ItemID;		resSet->serial(ItemID);

		g_BestSellItemList.push_back(ItemID);
	}
}

void LoadBestsellItemList()
{
	g_BestSellItemList.clear();
	
	MAKEQUERY_GetBestsellItemList(strQ);
	DBCaller->execute(strQ, DBCB_DBGetBestsellItemList);

	return;
}

//void cb_M_CS_BESTSELLITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//
//	CMessage msgOut(M_SC_BESTSELLITEM);
//	msgOut.serial(FID);
//	for (std::list<uint32>::iterator i = g_BestSellItemList.begin(); i != g_BestSellItemList.end(); i ++)
//	{
//		uint32 itemid = (*i);
//		msgOut.serial(itemid);
//	}
//	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
//}

void SendBestSellItems(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	CMessage msgOut(M_SC_BESTSELLITEM);
	msgOut.serial(FID);
	for (std::list<uint32>::iterator i = g_BestSellItemList.begin(); i != g_BestSellItemList.end(); i ++)
	{
		uint32 itemid = (*i);
		msgOut.serial(itemid);
	}
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

//========== ?�용?�기 ==============
//class _Review_
//{
//public:
//	uint8		ReviewID;
//	ucstring	Name;
//	ucstring	Content;
//	ucstring	Url;
//};
//uint32	g_nReviewCount;
//static _Review_	g_ReviewList[MAX_REVIEW_COUNT];
//
//static void	DBCB_DBLoadReviewList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	if (!nQueryResult)
//		return;
//
//	resSet->serial(g_nReviewCount);
//	if (g_nReviewCount > MAX_REVIEW_COUNT)
//		g_nReviewCount = MAX_REVIEW_COUNT;
//	for (uint32 i = 0; i < g_nReviewCount; i ++)
//	{
//		resSet->serial(g_ReviewList[i].ReviewID);
//		resSet->serial(g_ReviewList[i].Name);
//		resSet->serial(g_ReviewList[i].Content);
//		resSet->serial(g_ReviewList[i].Url);
//	}
//}
//
//void LoadReviewList()
//{
//	MAKEQUERY_LoadReviewList(strQ);
//	DBCaller->execute(strQ, DBCB_DBLoadReviewList);
//
//	return;
//}
//
//void cb_M_CS_REVIEW_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//
//	if (g_nReviewCount == 0)
//	{
//		CMessage	msgOut(M_SC_REVIEW);
//		msgOut.serial(FID);
//		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
//	}
//	else
//	{
//		uint8		startflag, endflag;
//		for (uint32 i = 0; i < g_nReviewCount; i ++)
//		{
//			CMessage	msgOut(M_SC_REVIEW);
//			startflag = (i == 0) ? 1 : 0;
//			endflag = (i == g_nReviewCount - 1) ? 1 : 0;
//			msgOut.serial(FID, startflag, g_ReviewList[i].ReviewID, g_ReviewList[i].Name, g_ReviewList[i].Content, g_ReviewList[i].Url, endflag);
//			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
//		}
//	}
//}
//
//void cb_M_GMCS_REVIEW_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		GMID;		_msgin.serial(GMID);
//	uint8			ReviewID;	_msgin.serial(ReviewID);
//	ucstring		Name;		_msgin.serial(Name);
//	ucstring		Content;	_msgin.serial(Content);
//	ucstring		Url;		_msgin.serial(Url);
//
//	if ((Name.length() > 32) || (Content.length() > TGNEWS_TXT_MAX_LEN) || (Url.length() > 100))
//	{
//		sipwarning("Invalid Text Length. GMID=%d", GMID);
//		return;
//	}
//
//	for (uint32 i = 0; i < g_nReviewCount; i ++)
//	{
//		if (g_ReviewList[i].ReviewID == ReviewID)
//		{
//			sipwarning("Exist Same ReviewID!!! ReviewID=%d", ReviewID);
//			return;
//		}
//	}
//	if (g_nReviewCount >= MAX_REVIEW_COUNT)
//	{
//		sipwarning("g_nReviewCount >= MAX_REVIEW_COUNT. g_nReviewCount=%d", g_nReviewCount);
//		return;
//	}
//
//	g_ReviewList[g_nReviewCount].ReviewID = ReviewID;
//	g_ReviewList[g_nReviewCount].Name = Name;
//	g_ReviewList[g_nReviewCount].Content = Content;
//	g_ReviewList[g_nReviewCount].Url = Url;
//	g_nReviewCount ++;
//
//	MAKEQUERY_InsertReview(strQ, ReviewID, Name, Content, Url);
//	DBCaller->execute(strQ);
//}
//
//void cb_M_GMCS_REVIEW_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		GMID;		_msgin.serial(GMID);
//	uint8			ReviewID;	_msgin.serial(ReviewID);
//	ucstring		Name;		_msgin.serial(Name);
//	ucstring		Content;	_msgin.serial(Content);
//	ucstring		Url;		_msgin.serial(Url);
//
//	uint32	i = 0;
//	for (i = 0; i < g_nReviewCount; i ++)
//	{
//		if (g_ReviewList[i].ReviewID == ReviewID)
//		{
//			g_ReviewList[i].ReviewID = ReviewID;
//			g_ReviewList[i].Name = Name;
//			g_ReviewList[i].Content = Content;
//			g_ReviewList[i].Url = Url;
//			break;
//		}
//	}
//	if (i >= g_nReviewCount)
//	{
//		sipwarning("i >= g_nReviewCount. ReviewID=%d", ReviewID);
//		return;
//	}
//
//	MAKEQUERY_ModifyReview(strQ, ReviewID, Name, Content, Url);
//	DBCaller->execute(strQ);
//}
//
//void cb_M_GMCS_REVIEW_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		GMID;		_msgin.serial(GMID);
//	uint8			ReviewID;	_msgin.serial(ReviewID);
//
//	BOOL	bFound = FALSE;
//	for (uint32 i = 0; i < g_nReviewCount; i ++)
//	{
//		if (g_ReviewList[i].ReviewID == ReviewID)
//		{
//			bFound = true;
//
//			g_nReviewCount --;
//			g_ReviewList[i].ReviewID = g_ReviewList[g_nReviewCount].ReviewID;
//			g_ReviewList[i].Name = g_ReviewList[g_nReviewCount].Name;
//			g_ReviewList[i].Content = g_ReviewList[g_nReviewCount].Content;
//			g_ReviewList[i].Url = g_ReviewList[g_nReviewCount].Url;
//			break;
//		}
//	}
//	if (!bFound)
//	{
//		sipwarning("bFound = FALSE. ReviewID=%d", ReviewID);
//		return;
//	}
//
//	MAKEQUERY_DeleteReview(strQ, ReviewID);
//	DBCaller->execute(strQ);
//}
