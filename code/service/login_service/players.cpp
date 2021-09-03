#include <misc/ucstring.h>
#include "net/service.h"

#include "misc/DBCaller.h"

#include "../../common/Macro.h"
#include "../../common/Packet.h"
#include "../Common/Lang.h"
#include "../Common/QuerySetForMain.h"
#include "../Common/Common.h"

//
// Namespaces
//

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

#include "net/login_cookie.h"
#include "login_service.h"
#include "players.h"
#include "connection_ws.h"

extern bool SendPacketToFID(T_FAMILYID FID, CMessage msgOut);

#pragma pack(push, 1)
struct _FriendGroupInfo
{
	uint32		GroupID;
	ucchar		GroupName[MAX_LEN_CONTACT_GRPNAME + 1];

#ifdef SIP_OS_UNIX
	static bool isDiffSize() { return true; }
	static uint32 getSize()
	{
		return sizeof(uint32) + sizeof(char16_t) * (MAX_LEN_CONTACT_GRPNAME + 1);
	}

	void serialize(uint8 *buf)
	{
		uint32 pos = 0;
		memcpy(&GroupID, buf + pos, sizeof(uint32));
		pos += sizeof(uint32);
		
		char16_t character;
		for (int i = 0; i < MAX_LEN_CONTACT_GRPNAME + 1; i++)
		{
			memcpy(&character, buf + pos, sizeof(char16_t));
			pos += sizeof(char16_t);

			GroupName[i] = (ucchar)character;
		}
	}
#endif
};
#pragma pack(pop)
typedef SDATA_LIST<_FriendGroupInfo, MAX_CONTGRPNUM + 1>	FriendGroupList;

struct _FriendInfo
{
	ucstring	FamilyName;
	uint32		Index;
	uint32		GroupID;
	uint32		ModelId;
	_FriendInfo(ucstring fname, uint32 index, uint32 group_id, uint32 model_id)
	{
		FamilyName = fname;
		Index = index;
		GroupID = group_id;
		ModelId = model_id;
	}
};

class _FamilyFriendListInfo
{
public:
	uint32	m_Changed_SN;

	FriendGroupList			m_FriendGroups;
	std::map<T_FAMILYID, _FriendInfo>	m_Friends;	// <FID, _FriendInfo>

	_FamilyFriendListInfo()
	{
		m_Changed_SN = 1;
	}
};
std::map<T_FAMILYID, _FamilyFriendListInfo>	g_FamilyFriends;


#pragma pack(push, 1)
struct _FavorroomGroupInfo
{
	uint32		GroupID;
	ucchar		GroupName[MAX_FAVORGROUPNAME + 1];
#ifdef SIP_OS_UNIX
	static bool isDiffSize() { return true; }
	static uint32 getSize()
	{
		return sizeof(uint32) + sizeof(char16_t) * (MAX_FAVORGROUPNAME + 1);		
	}

	void serialize(uint8 *buf)
	{
		uint32 pos = 0;
		memcpy(&GroupID, buf + pos, sizeof(uint32));
		pos += sizeof(uint32);
		
		char16_t character;
		for (int i = 0; i < MAX_FAVORGROUPNAME + 1; i++)
		{
			memcpy(&character, buf + pos, sizeof(char16_t));
			pos += sizeof(char16_t);

			GroupName[i] = (ucchar)character;
		}
	}
#endif
};
#pragma pack(pop)
typedef SDATA_LIST<_FavorroomGroupInfo, MAX_FAVORROOMGROUP>	FavorroomGroupList;

struct _FavorRoomInfo
{
	uint32		GroupID;
	uint32		SelDate;
	_FavorRoomInfo(uint32 group_id, uint32 sel_date)
	{
		GroupID = group_id;
		SelDate = sel_date;
	}
};
class _FamilyFavorRoomListInfo
{
public:
	uint32	m_Changed_SN;

	FavorroomGroupList					m_FavorRoomGroups;
	std::map<T_ROOMID, _FavorRoomInfo>	m_FavorRooms;	// <RoomID, _FavorRoomInfo>

	_FamilyFavorRoomListInfo()
	{
		m_Changed_SN = 1;
	}
};
std::map<T_FAMILYID, _FamilyFavorRoomListInfo>	g_FamilyFavorRooms;


uint32	getMinutesSince1970(std::string& date)
{
	MSSQLTIME	t;
	if (!t.SetTime(date))
		return 0;
	uint32	result = (uint32)(t.timevalue / 60);
	return result;
}


bool ReadFriendInfo(T_FAMILYID FID)
{
	if (g_FamilyFriends.find(FID) != g_FamilyFriends.end())
		return true;

	if (g_FamilyFriends.size() > 100000)
	{
		sipwarning("g_FamilyFriends.size() > 100,000! Check LS's memory! size=%d", g_FamilyFriends.size());
	}

	_FamilyFriendListInfo	data;

	bool	bSuccess = true;
	bool	bNewGroup = false;
	{
		MAKEQUERY_LoadFriendGroupList(strQuery, FID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult != true)
		{
			sipwarning("Failed Loading Shard List, Query : %s", strQuery);
			return false;
		}

		uint8	bFriendGroupList[MAX_CONTGRP_BUF_SIZE];
		long	lGroupBufSize = sizeof(bFriendGroupList);

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			data.m_FriendGroups.DataCount = 2;
			data.m_FriendGroups.Datas[0].GroupID = -1;
			wcscpy(data.m_FriendGroups.Datas[0].GroupName, L"Black");
			data.m_FriendGroups.Datas[1].GroupID = 1;
			wcscpy(data.m_FriendGroups.Datas[1].GroupName, GetLangText(ID_FRIEND_DEFAULTGROUP_NAME).c_str()); //L"\x597D\x53CB"); // L"好友"); //  

			bNewGroup = true;
		}
		else
		{
			row.GetData(1, bFriendGroupList, lGroupBufSize, &lGroupBufSize, SQL_C_BINARY);
			if(!data.m_FriendGroups.fromBuffer(bFriendGroupList, (uint32)lGroupBufSize))
			{
				sipwarning("FriendGroup.fromBuffer failed. FID=%d", FID);
				bSuccess = false;
			}
		}
	}
	if (!bSuccess)
		return false;

	{
		MAKEQUERY_GetFriendList(strQuery, FID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult != true)
		{
			sipwarning("Failed Loading Shard List, Query : %s", strQuery);
			return false;
		}

		do
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( !IS_DB_ROW_VALID(row) )
				break;

			COLUMN_DIGIT(row,	0,	uint32,		fid);
			COLUMN_WSTR(row,	1,	ucName,		MAX_LEN_FAMILY_NAME);
			COLUMN_DIGIT(row,	2,	uint32,		index);
			COLUMN_DIGIT(row,	3,	uint32,		group_id);
			COLUMN_DIGIT(row,	4,	uint32,		model_id);

			_FriendInfo	friend_data(ucName, index, group_id, model_id);

			data.m_Friends.insert(make_pair(fid, friend_data));
		}while ( true );
	}

	// 친구목록이 제대로 되지 않은 경우가 있으므로 정리가 필요하다!
	bool	bGroupChanged = false;
	std::list<T_FAMILYID>	overflowFIDs;
	// Step1 - 그룹별로 정리
	int		groupMemberCount[MAX_CONTGRPNUM + 1];
	for (int i = 0; i < MAX_CONTGRPNUM + 1; i ++)
		groupMemberCount[i] = 0;
	for (std::map<T_FAMILYID, _FriendInfo>::iterator it = data.m_Friends.begin(); it != data.m_Friends.end(); it ++)
	{
		uint32	curGroupID = it->second.GroupID;
		int		j = 0;
		for (; j < (int)data.m_FriendGroups.DataCount; j ++)
		{
			if (data.m_FriendGroups.Datas[j].GroupID == curGroupID)
				break;
		}
		if (j >= (int)data.m_FriendGroups.DataCount)
		{
			// 그룹목록에서 찾지 못함
			if (data.m_FriendGroups.DataCount >= MAX_CONTGRPNUM + 1)
			{
				// 그룹개수 초과
				overflowFIDs.push_back(it->first);
				continue;
			}

			// 새 그룹 창조, 그룹이름=번호
			data.m_FriendGroups.Datas[data.m_FriendGroups.DataCount].GroupID = curGroupID;
			smprintf(data.m_FriendGroups.Datas[data.m_FriendGroups.DataCount].GroupName, MAX_FAVORGROUPNAME + 1, L"%d", data.m_FriendGroups.DataCount);
			data.m_FriendGroups.DataCount ++;

			j = data.m_FriendGroups.DataCount - 1;

			bGroupChanged = true;
		}

		if (groupMemberCount[j] >= MAX_CONTACT_COUNT)
		{
			// 한 그룹에 들어가는 인원 초과
			overflowFIDs.push_back(it->first);
			continue;
		}

		groupMemberCount[j] ++;
	}

	// Step2 - 초과인원들 재분배
	for (std::list<T_FAMILYID>::iterator it = overflowFIDs.begin(); it != overflowFIDs.end(); it ++)
	{
		T_FAMILYID	friendFID = *it;

		int		j = 1;
		for (; j < (int)data.m_FriendGroups.DataCount; j ++)
		{
			if (groupMemberCount[j] < MAX_CONTACT_COUNT)
				break;
		}

		if (j >= (int)data.m_FriendGroups.DataCount)
		{
			// 인원수 모자라는 그룹 찾지 못함
			if (data.m_FriendGroups.DataCount >= MAX_CONTGRPNUM + 1)
			{
				// 그룹개수 초과, 이 친구는 삭제해야 함
				data.m_Friends.erase(friendFID);
				// DB처리
				{
					MAKEQUERY_DeleteFriend(strQuery, FID, friendFID);
					CDBConnectionRest	DB(MainDB);
					DB_EXE_QUERY(DB, Stmt, strQuery);
				}
				continue;
			}

			// 새 그룹 창조, 그룹이름=번호
			uint32	newGroupID = MAX_DEFAULT_SYSTEMGROUP_ID + 1;
			for (int i = 1; i < (int)data.m_FriendGroups.DataCount; i ++)
			{
				if (data.m_FriendGroups.Datas[i].GroupID >= newGroupID)
					newGroupID = data.m_FriendGroups.Datas[i].GroupID + 1;
			}
			data.m_FriendGroups.Datas[data.m_FriendGroups.DataCount].GroupID = newGroupID;
			smprintf(data.m_FriendGroups.Datas[data.m_FriendGroups.DataCount].GroupName, MAX_FAVORGROUPNAME + 1, L"%d", data.m_FriendGroups.DataCount);
			data.m_FriendGroups.DataCount ++;

			j = data.m_FriendGroups.DataCount - 1;

			bGroupChanged = true;
		}

		// 친구에 그룹 설정
		std::map<T_FAMILYID, _FriendInfo>::iterator	it2 = data.m_Friends.find(friendFID);
		if (it2 == data.m_Friends.end())
		{
			sipwarning("data.m_Friends.find failed. FID=%d, friendFID=%d", FID, friendFID);
			continue;
		}
		it2->second.GroupID = data.m_FriendGroups.Datas[j].GroupID;
		// DB처리
		{
			MAKEQUERY_ChangeFriendIndex(strQuery, FID, friendFID, it2->second.GroupID, it2->second.Index);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_QUERY(DB, Stmt, strQuery);
		}
	}

	g_FamilyFriends[FID] = data;

	if (bNewGroup || bGroupChanged)
		SaveFriendGroupList(FID);

	if (bGroupChanged || overflowFIDs.size() > 0)
	{
		sipwarning("FriendData fixed. FID=%d, bGroupChanged=%d, overflowFIDs.size()=%d", FID, bGroupChanged, overflowFIDs.size());
	}

	return true;
}

bool SaveFriendGroupList(T_FAMILYID FID)
{
	if (FID == 0)
		return true;

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", FID);
		return false;
	}

	uint8	bFriendGroupList[MAX_CONTGRP_BUF_SIZE];
	uint32	GroupBufferSize = sizeof(bFriendGroupList);

	if (!it->second.m_FriendGroups.toBuffer(bFriendGroupList, &GroupBufferSize))
	{
		sipwarning("m_FriendGroups.toBuffer failed. FID=%d", FID);
		return false;
	}

	char str_buf[MAX_CONTGRP_BUF_SIZE * 2 + 10];
	for (uint32 i = 0; i < GroupBufferSize; i ++)
	{
		sprintf(&str_buf[i * 2], ("%02x"), bFriendGroupList[i]);
	}
	str_buf[GroupBufferSize * 2] = 0;

	{
		MAKEQUERY_SaveFriendGroupList(strQuery, FID, str_buf);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
	}
	return true;
}

void SendFriendListToSS(uint32 UID, uint32 FID, uint16 sid)
{
	if (!ReadFriendInfo(FID))
	{
		sipwarning("ReadFriendInfo failed. UID=%d, FID=%d", UID, FID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. UID=%d, FID=%d", UID, FID);
		return;
	}

	{
		CMessage	msgOut(M_SC_FRIEND_GROUP);
		uint8		count = (uint8) it->second.m_FriendGroups.DataCount;
		msgOut.serial(UID, count);
		for(uint8 i = 0; i < count; i ++)
		{
			ucstring	name(it->second.m_FriendGroups.Datas[i].GroupName);
			msgOut.serial(it->second.m_FriendGroups.Datas[i].GroupID, name);
		}
		if (sid == 0)
			CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgOut);
		else
			CUnifiedNetwork::getInstance()->send(TServiceId(sid), msgOut);
	}

	{
const uint8  Friend_Send_Count = 50;
		int friendCount = static_cast<int>(it->second.m_Friends.size());
		std::map<T_FAMILYID, _FriendInfo>::iterator it1 = it->second.m_Friends.begin();
		int i = 0;
		uint8	finished = 0;
		while (finished == 0)
		{
			uint8		count = 0;
			if (i + Friend_Send_Count < friendCount)
			{
				count = Friend_Send_Count;
			}
			else
			{
				count = (uint8)(friendCount - i);
				finished = 1;
			}

			CMessage	msgOut(M_SC_FRIEND_LIST);
			msgOut.serial(UID, count, finished);
			for (uint8 j = 0; j < count; j ++, i ++, it1 ++)
			{
				T_FAMILYID	friendFID = it1->first;
				msgOut.serial(it1->second.GroupID, friendFID, it1->second.FamilyName, it1->second.Index, it1->second.ModelId);
			}
			if (sid == 0)
				CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgOut);
			else
				CUnifiedNetwork::getInstance()->send(TServiceId(sid), msgOut);
		}
	}

	SendFavorRoomListToSS(UID, FID, sid);
}

// Add contact group([u:Name][d:GroupId])
void cb_M_CS_ADDCONTGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	T_FAMILYID	FID;			msgin.serial(FID);
	ucstring	ucGroup;		msgin.serial(ucGroup);
	uint32		GroupID;		msgin.serial(GroupID);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", FID);
		return;
	}
	FriendGroupList	&friendGroupList = it->second.m_FriendGroups;

	// CheckData
	if(friendGroupList.DataCount >= MAX_CONTGRPNUM + 1)
	{
		sipwarning("friendGroupList.DataCount overflow!! FID=%d", FID);
		return;
	}
	if((int)GroupID < MAX_DEFAULT_SYSTEMGROUP_ID)
	{
		sipwarning("Invalid GroupID. FID=%d, GroupID=%d", FID, GroupID);
		return;
	}

	// Insert New Group
	friendGroupList.Datas[friendGroupList.DataCount].GroupID = GroupID;
	wcscpy(friendGroupList.Datas[friendGroupList.DataCount].GroupName, ucGroup.c_str());
	friendGroupList.DataCount ++;

	// Save
	SaveFriendGroupList(FID);

	it->second.m_Changed_SN ++;

	// m_Changed_SN 통지
	Send_M_NT_FRIEND_SN(FID, it->second.m_Changed_SN, false, tsid);
}

// Delete contact group([d:GroupId])
void cb_M_CS_DELCONTGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	T_FAMILYID	FID;			msgin.serial(FID);
	uint32		GroupID;		msgin.serial(GroupID);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", FID);
		return;
	}
	FriendGroupList	&friendGroupList = it->second.m_FriendGroups;

	// GroupID=1,-1 : Default Group
	if ((GroupID < MAX_DEFAULT_SYSTEMGROUP_ID) || (GroupID == (uint32)(-1)))
	{
		sipwarning("Invalid GroupID!! GroupID=%d, FID=%d", GroupID, FID);
		return;
	}

	int	GroupIndex;
	for (GroupIndex = 0; GroupIndex < (int)friendGroupList.DataCount; GroupIndex ++)
	{
		if(friendGroupList.Datas[GroupIndex].GroupID == GroupID)
			break;
	}
	if(GroupIndex >= (int)friendGroupList.DataCount)
	{
		sipwarning("Invalid GroupID!! FID=%d, GroupID=%d", FID, GroupID);
		return;
	}

	for(int i = GroupIndex + 1; i < (int)friendGroupList.DataCount; i ++)
	{
		friendGroupList.Datas[i - 1] = friendGroupList.Datas[i];
	}
	friendGroupList.DataCount --;

	// Save
	SaveFriendGroupList(FID);

	it->second.m_Changed_SN ++;

	// m_Changed_SN 통지
	Send_M_NT_FRIEND_SN(FID, it->second.m_Changed_SN, false, tsid);
}

// Change contact group name([d:GroupId][u:new Group name])
void cb_M_CS_RENCONTGROUP(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	T_FAMILYID	FID;			msgin.serial(FID);
	uint32		GroupID;		msgin.serial(GroupID);
	ucstring	ucGroup;		msgin.serial(ucGroup);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", FID);
		return;
	}
	FriendGroupList	&friendGroupList = it->second.m_FriendGroups;

	int	GroupIndex = -1;
	for(int i = 0; i < (int)friendGroupList.DataCount; i ++)
	{
		if(friendGroupList.Datas[i].GroupID == GroupID)
		{
			GroupIndex = i;
			break;
		}
	}
	if(GroupIndex == -1)
	{
		sipwarning("Invalid GroupIndex!! FID=%d, GroupID=%d", FID, GroupID);
		return;
	}

	// Save
	wcscpy(friendGroupList.Datas[GroupIndex].GroupName, ucGroup.c_str());

	SaveFriendGroupList(FID);

	it->second.m_Changed_SN ++;

	// m_Changed_SN 통지
	Send_M_NT_FRIEND_SN(FID, it->second.m_Changed_SN, false, tsid);
}

// Set contact group index([b:Count][ [d:GroupId] ])
void cb_M_CS_SETCONTGROUP_INDEX(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	T_FAMILYID		FID;		msgin.serial(FID);
	uint8			count;		msgin.serial(count);
	uint32			GroupIDs[MAX_CONTGRPNUM + 1];
	for(uint8 i = 0; i < count; i ++)
		msgin.serial(GroupIDs[i]);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", FID);
		return;
	}
	FriendGroupList	&friendGroupList = it->second.m_FriendGroups;

	if(count != friendGroupList.DataCount)
	{
		// Hacker!!!
		sipwarning("GroupCount Error! RealCount=%d, InputCount=%d, FID=%d", friendGroupList.DataCount, count, FID);
		return;
	}

	// Change Album Index
	_FriendGroupInfo	data;
	uint32		i, j;
	for(i = 0; i < friendGroupList.DataCount; i ++)
	{
		for(j = i; j < friendGroupList.DataCount; j ++)
		{
			if(friendGroupList.Datas[j].GroupID == GroupIDs[i])
				break;
		}
		if(j >= friendGroupList.DataCount)
		{
			// Hacker!!!
			sipwarning("GroupIDList Error! BadGroupID=%d, FID=%d", GroupIDs[i], FID);
			return;
		}
		if(i != j)
		{
			data = friendGroupList.Datas[j];
			friendGroupList.Datas[j] = friendGroupList.Datas[i];
			friendGroupList.Datas[i] = data;
		}
	}

	// Save
	SaveFriendGroupList(FID);

	it->second.m_Changed_SN ++;

	// m_Changed_SN 통지
	Send_M_NT_FRIEND_SN(FID, it->second.m_Changed_SN, false, tsid);
}

// Add contact([d:FamilyId][d:GroupId])
void cb_M_CS_ADDCONT(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	T_FAMILYID	FID;		msgin.serial(FID);
	T_FAMILYID  friendFID;	msgin.serial(friendFID);
	uint32      GroupID;	msgin.serial(GroupID);
	uint32      FriendIndex;msgin.serial(FriendIndex);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", FID);
		return;
	}
	FriendGroupList	&friendGroupList = it->second.m_FriendGroups;
	std::map<T_FAMILYID, _FriendInfo>	&friendList = it->second.m_Friends;

	if (FID == friendFID)
	{
		sipwarning("FID == friendFID. FID=%d", FID);
		return;
	}

	// 검사
	if (friendList.find(friendFID) != friendList.end())
	{
		sipwarning("friendList.find(friendFID) != friendList.end(). FID=%d, friendFID=%d", FID, friendFID);
		return;
	}
	uint32 i = 0;
	for(i = 0; i < friendGroupList.DataCount; i ++)
	{
		if(friendGroupList.Datas[i].GroupID == GroupID)
			break;
	}
	if (i >= friendGroupList.DataCount)
	{
		sipwarning("i >= friendGroupList.DataCount. FID=%d, friendFID=%d, GroupID=%d", FID, friendFID, GroupID);
		return;
	}

	// DB에 보관
	ucstring	friendName = L"";
	uint32		ModelId = 3;
	{
		sint32	nGroupID = (sint32)GroupID;
		MAKEQUERY_InsertFriend(strQuery, FID, friendFID, nGroupID, FriendIndex);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult != true)
		{
			sipwarning(L"Failed Query : %s", strQuery);
			return;
		}

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			sipwarning(L"FETCH failed. Query: %s", strQuery);
			return;
		}
		else
		{
			COLUMN_WSTR(row,	0,	ucName,		MAX_LEN_FAMILY_NAME);
			COLUMN_DIGIT(row,	1,	uint32,		model_id);
			friendName = ucName;
			ModelId = model_id;
		}
	}

	// 메모리에서 처리
	{
		_FriendInfo	data(friendName, FriendIndex, GroupID, ModelId);
		friendList.insert(make_pair(friendFID, data));
	}
	it->second.m_Changed_SN ++;

	bool	bNeedUpdateMainShard = false;
	if (GroupID != (uint32)(-1))
		bNeedUpdateMainShard = true;

	// m_Changed_SN 통지
	Send_M_NT_FRIEND_SN(FID, it->second.m_Changed_SN, bNeedUpdateMainShard, tsid);
}

// Delete contact([d:ContactFamilyId])
void cb_M_CS_DELCONT(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	T_FAMILYID		FID;		msgin.serial(FID);
	T_FAMILYID		friendFID;	msgin.serial(friendFID);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", FID);
		return;
	}
	FriendGroupList	&friendGroupList = it->second.m_FriendGroups;
	std::map<T_FAMILYID, _FriendInfo>	&friendList = it->second.m_Friends;

	// 검사
	std::map<T_FAMILYID, _FriendInfo>::iterator it1 = friendList.find(friendFID);
	if (it1 == friendList.end())
	{
		sipwarning("friendList.find(friendFID) == friendList.end(). FID=%d, friendFID=%d", FID, friendFID);
		return;
	}

	bool	bNeedUpdateMainShard = false;
	if (it1->second.GroupID != (uint32)(-1))
	{
		// 일반친구가 삭제되였다면
		Send_M_NT_FRIEND_DELETED(FID, friendFID);

		bNeedUpdateMainShard = true;
	}

	// DB에 보관
	{
		MAKEQUERY_DeleteFriend(strQuery, FID, friendFID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
	}

	// 메모리에서 처리
	{
		friendList.erase(friendFID);
	}
	it->second.m_Changed_SN ++;

	// m_Changed_SN 통지
	Send_M_NT_FRIEND_SN(FID, it->second.m_Changed_SN, bNeedUpdateMainShard, tsid);
}

// Change contact's group [d:Count][ [d:ContactId][d:NewGroupId][d:NewIndex] ]
void cb_M_CS_CHANGECONTGRP(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	T_FAMILYID	FID;	 msgin.serial(FID);
	uint32		count;	 msgin.serial(count);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", FID);
		return;
	}
	FriendGroupList	&friendGroupList = it->second.m_FriendGroups;
	std::map<T_FAMILYID, _FriendInfo>	&friendList = it->second.m_Friends;

	bool	bChanged = false;
	bool	bNeedUpdateMainShard = false;
	uint32		friendFID, NewGroupID, NewIndex;
	for (uint32 i = 0; i < count; i ++)
	{
		msgin.serial(friendFID);
		msgin.serial(NewGroupID);
		msgin.serial(NewIndex);
		int		nNewGroupID = (int)NewGroupID;

		std::map<T_FAMILYID, _FriendInfo>::iterator	it1 = friendList.find(friendFID);
		if (it1 == friendList.end())
		{
			sipwarning("bFind = false. FID=%d, friendFID=%d", FID, friendFID);
			break;
		}

		if ((it1->second.GroupID != (uint32)(-1)) && (NewGroupID == (uint32)(-1)))
		{
			// 일반친구가 흑명단에 들어갔다면
			Send_M_NT_FRIEND_DELETED(FID, friendFID);

			bNeedUpdateMainShard = true;
		}
		else if ((it1->second.GroupID == (uint32)(-1)) && (NewGroupID != (uint32)(-1)))
		{
			bNeedUpdateMainShard = true;
		}

		it1->second.GroupID = NewGroupID;
		it1->second.Index = NewIndex;

		{
			MAKEQUERY_ChangeFriendIndex(strQuery, FID, friendFID, nNewGroupID, NewIndex);
			CDBConnectionRest	DB(MainDB);
			DB_EXE_QUERY(DB, Stmt, strQuery);
		}

		bChanged = true;
	}

	if (!bChanged)
		return;

	it->second.m_Changed_SN ++;

	// m_Changed_SN 통지
	Send_M_NT_FRIEND_SN(FID, it->second.m_Changed_SN, bNeedUpdateMainShard, tsid);
}

// Request Friend Info [d:FID][d:SN] MS->WS->LS
void cb_M_NT_REQ_FRIEND_INFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	T_FAMILYID	FID;		msgin.serial(FID);
	uint32		changed_sn;	msgin.serial(changed_sn);

	if (!ReadFriendInfo(FID))
	{
		sipwarning("ReadFriendInfo failed. FID=%d, sn=%d", FID, changed_sn);
		return;
	}

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d, sn=%d", FID, changed_sn);
		return;
	}

	if (it->second.m_Changed_SN == changed_sn)
	{
		uint32	zero = 0;
		{
			CMessage	msgOut(M_NT_FRIEND_GROUP);
			msgOut.serial(FID, zero);
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		}
		{
			CMessage	msgOut(M_NT_FRIEND_LIST);
			msgOut.serial(FID, zero);
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		}
	}
	else
	{
		{
			CMessage	msgOut(M_NT_FRIEND_GROUP);
			uint8		count = (uint8) it->second.m_FriendGroups.DataCount;
			msgOut.serial(FID, it->second.m_Changed_SN, count);
			for(uint8 i = 0; i < count; i ++)
			{
				ucstring	name(it->second.m_FriendGroups.Datas[i].GroupName);
				msgOut.serial(it->second.m_FriendGroups.Datas[i].GroupID, name);
			}
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		}

		{
	const uint8  Friend_Send_Count = 50;
			int friendCount = static_cast<int>(it->second.m_Friends.size());
			std::map<T_FAMILYID, _FriendInfo>::iterator it1 = it->second.m_Friends.begin();
			int i = 0;
			uint8	finished = 0;
			while (finished == 0)
			{
				uint8		count = 0;
				if (i + Friend_Send_Count < friendCount)
				{
					count = Friend_Send_Count;
				}
				else
				{
					count = (uint8)(friendCount - i);
					finished = 1;
				}

				CMessage	msgOut(M_NT_FRIEND_LIST);
				msgOut.serial(FID, it->second.m_Changed_SN, count, finished);
				for (uint8 j = 0; j < count; j ++, i ++, it1 ++)
				{
					T_FAMILYID	friendFID = it1->first;
					msgOut.serial(it1->second.GroupID, friendFID, it1->second.Index);
				}
				CUnifiedNetwork::getInstance()->send(tsid, msgOut);
			}
		}
	}
}

void Send_M_NT_FRIEND_DELETED(T_FAMILYID FID, T_FAMILYID friendFID)
{
	uint32 mainShardID = getMainShardIDfromFID(FID);
	if (mainShardID == 0)
	{
		sipwarning("getMainShardIDfromFID = 0. FID=%d", FID);
		return;
	}

	CMessage	msgOut(M_NT_FRIEND_DELETED);
	msgOut.serial(FID, friendFID);
	SendMsgToShardByID(msgOut, mainShardID);
}

void Send_M_NT_FRIEND_SN(T_FAMILYID FID, uint32 sn, bool sendToMainShard, TServiceId tsid)
{
	{
		uint8	flag = 0;
		CMessage	msgOut(M_NT_FRIEND_SN);
		msgOut.serial(FID, sn, flag);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}

	if (!sendToMainShard)
		return;

	// MainShard와 tsid가 같은지 검사한다.
	uint32 mainShardID = getMainShardIDfromFID(FID);
	if (mainShardID == 0)
	{
		sipwarning("getMainShardIDfromFID = 0. FID=%d", FID);
		return;
	}
	CShardInfo* pShard = findShardWithSId(tsid.get());
	if (pShard == NULL)
	{
		sipwarning("findShardWithSId = NULL. FID=%d, tsid=%d", FID, tsid.get());
		return;
	}
	if (mainShardID == pShard->m_nShardId)
		return;

	// MainShard와 tsid가 다른 경우.
	{
		uint8	flag = 1;
		CMessage	msgOut(M_NT_FRIEND_SN);
		msgOut.serial(FID, sn, flag);
		SendMsgToShardByID(msgOut, mainShardID);
	}
}

// Request Family Info for AddCont ([d:friendFID][d:FID][d:GroupID][d:FriendIndex])
void cb_M_NT_REQ_FAMILY_INFO(CMessage &msgin, const std::string &serviceName, TServiceId tsid)
{
	uint16		rsSid;
	T_FAMILYID	friendFID, FID;
	uint32		GroupID, FriendIndex;

	msgin.serial(rsSid, friendFID, FID, GroupID, FriendIndex);

	uint32		friendUID, friendModelID;
	{
		MAKEQUERY_GetFamilyInfo(strQuery, friendFID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult != true)
		{
			sipwarning(L"Failed Query : %s", strQuery);
			return;
		}

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (!IS_DB_ROW_VALID(row))
		{
			sipwarning(L"FETCH failed. Query: %s", strQuery);
			return;
		}
		COLUMN_DIGIT(row,	0,	uint32,		user_id);
		COLUMN_DIGIT(row,	1,	uint32,		model_id);
		friendUID = user_id;
		friendModelID = model_id;
	}

	CMessage	msgOut(M_NT_ANS_FAMILY_INFO);
	msgOut.serial(rsSid, friendFID, FID, GroupID, FriendIndex, friendUID, friendModelID);
	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}



// ==============FavorRoom

bool ReadFavorRoomInfo(T_FAMILYID FID)
{
	if (g_FamilyFavorRooms.find(FID) != g_FamilyFavorRooms.end())
		return true;

	if (g_FamilyFavorRooms.size() > 100000)
	{
		sipwarning("g_FamilyFavorRooms.size() > 100,000! Check LS's memory! size=%d", g_FamilyFavorRooms.size());
	}

	_FamilyFavorRoomListInfo	data;

	bool	bSuccess = true;
	bool	bNewGroup = false;
	{
		MAKEQUERY_LoadFavorroomGroupList(strQuery, FID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult != true)
		{
			sipwarning("Failed Loading Shard List, Query : %s", strQuery);
			return false;
		}

		uint8	bFavorRoomGroupList[MAX_FAVORROOMGRP_BUF_SIZE];
		SQLLEN	lGroupBufSize = sizeof(bFavorRoomGroupList);

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			data.m_FavorRoomGroups.DataCount = 1;
			data.m_FavorRoomGroups.Datas[0].GroupID = 1;
			wcscpy(data.m_FavorRoomGroups.Datas[0].GroupName, GetLangText(ID_ROOM_DEFAULTGROUP_NAME).c_str());

			bNewGroup = true;
		}
		else
		{
			row.GetData(1, bFavorRoomGroupList, static_cast<ULONG>(lGroupBufSize), &lGroupBufSize, SQL_C_BINARY);
			if(!data.m_FavorRoomGroups.fromBuffer(bFavorRoomGroupList, (uint32)lGroupBufSize))
			{
				sipwarning("FavorRoomGroups.fromBuffer failed. FID=%d", FID);
				bSuccess = false;
			}
		}
	}
	if (!bSuccess)
		return false;

	{
		MAKEQUERY_GetFavorroom(strQuery, FID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult != true)
		{
			sipwarning("Failed Loading Shard List, Query : %s", strQuery);
			return false;
		}

		do
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if ( !IS_DB_ROW_VALID(row) )
				break;

			COLUMN_DIGIT(row,	0,	uint32,		room_id);
			COLUMN_DIGIT(row,	1,	uint32,		group_id);
			COLUMN_STR(row,		2,	seldate,	MAX_LEN_DATE);

			uint32	nSelTime = getMinutesSince1970(seldate);

			_FavorRoomInfo	info(group_id, nSelTime);

			data.m_FavorRooms.insert(make_pair(room_id, info));
		}while ( true );
	}

	g_FamilyFavorRooms[FID] = data;

	if (bNewGroup)
		SaveFavorRoomGroupList(FID);

	return true;
}

bool SaveFavorRoomGroupList(T_FAMILYID FID)
{
	if (FID == 0)
		return true;

	std::map<T_FAMILYID, _FamilyFavorRoomListInfo>::iterator it = g_FamilyFavorRooms.find(FID);
	if (it == g_FamilyFavorRooms.end())
	{
		sipwarning("g_FamilyFavorRooms.find = NULL. FID=%d", FID);
		return false;
	}

	uint8	bFavorRoomGroupList[MAX_FAVORROOMGRP_BUF_SIZE];
	uint32	GroupBufferSize = sizeof(bFavorRoomGroupList);

	if (!it->second.m_FavorRoomGroups.toBuffer(bFavorRoomGroupList, &GroupBufferSize))
	{
		sipwarning("m_FavorRoomGroups.toBuffer failed. FID=%d", FID);
		return false;
	}

	char str_buf[MAX_CONTGRP_BUF_SIZE * 2 + 10];
	for (uint32 i = 0; i < GroupBufferSize; i ++)
	{
		sprintf(&str_buf[i * 2], ("%02x"), bFavorRoomGroupList[i]);
	}
	str_buf[GroupBufferSize * 2] = 0;

	{
		MAKEQUERY_SaveFavorroomGroupList(strQuery, FID, str_buf);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
	}
	return true;
}

void SendFavorRoomListToSS(uint32 UID, uint32 FID, uint16 sid)
{
	if (!ReadFavorRoomInfo(FID))
	{
		sipwarning("ReadFavorRoomInfo failed. UID=%d, FID=%d", UID, FID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFavorRoomListInfo>::iterator it = g_FamilyFavorRooms.find(FID);
	if (it == g_FamilyFavorRooms.end())
	{
		sipwarning("g_FamilyFavorRooms.find = NULL. UID=%d, FID=%d", UID, FID);
		return;
	}

	{
		CMessage	msgOut(M_SC_FAVORROOM_GROUP);
		uint8		count = (uint8) it->second.m_FavorRoomGroups.DataCount;
		msgOut.serial(UID, count);
		for(uint8 i = 0; i < count; i ++)
		{
			ucstring	name(it->second.m_FavorRoomGroups.Datas[i].GroupName);
			msgOut.serial(it->second.m_FavorRoomGroups.Datas[i].GroupID, name);
		}
		if (sid == 0)
			CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgOut);
		else
			CUnifiedNetwork::getInstance()->send(TServiceId(sid), msgOut);
	}

	{
const uint8  FavorRoom_Send_Count = 200;
		int favorRoomCount = static_cast<int>(it->second.m_FavorRooms.size());
		std::map<T_ROOMID, _FavorRoomInfo>::iterator it1 = it->second.m_FavorRooms.begin();
		int i = 0;
		uint8	finished = 0;
		while (finished == 0)
		{
			uint8		count = 0;
			if (i + FavorRoom_Send_Count < favorRoomCount)
			{
				count = FavorRoom_Send_Count;
			}
			else
			{
				count = (uint8)(favorRoomCount - i);
				finished = 1;
			}

			CMessage	msgOut(M_SC_FAVORROOM_LIST);
			msgOut.serial(UID, count, finished);
			for (uint8 j = 0; j < count; j ++, i ++, it1 ++)
			{
				T_ROOMID	roomID = it1->first;
				msgOut.serial(it1->second.GroupID, roomID, it1->second.SelDate);
			}
			if (sid == 0)
				CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgOut);
			else
				CUnifiedNetwork::getInstance()->send(TServiceId(sid), msgOut);
		}
	}
}

// Add favorite room([d:GroupID][d:Room ID])
void cb_M_CS_INSFAVORROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;			_msgin.serial(FID);
	T_ID		groupid;		_msgin.serial(groupid);
	T_ROOMID	roomid;			_msgin.serial(roomid);

	std::map<T_FAMILYID, _FamilyFavorRoomListInfo>::iterator it = g_FamilyFavorRooms.find(FID);
	if (it == g_FamilyFavorRooms.end())
	{
		sipwarning("g_FamilyFavorRooms.find = NULL. FID=%d, RoomID=%d", FID, roomid);
		return;
	}
	if (it->second.m_FavorRooms.find(roomid) != it->second.m_FavorRooms.end())
	{
		sipwarning("m_FavorRooms.find != NULL. FID=%d, RoomID=%d", FID, roomid);
		return;
	}

	{
		MAKEQUERY_InsertFavorroom(strQuery, FID, roomid, groupid);
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

			COLUMN_STR(row,		0,	seldate,	MAX_LEN_DATE);

			uint32	nSelTime = getMinutesSince1970(seldate);

			_FavorRoomInfo	info(groupid, nSelTime);

			it->second.m_FavorRooms.insert(make_pair(roomid, info));
		}while ( true );
	}
}

// Delete favorite room([d:Room ID])
void cb_M_CS_DELFAVORROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;			_msgin.serial(FID);
	T_ROOMID	roomid;			_msgin.serial(roomid);

	std::map<T_FAMILYID, _FamilyFavorRoomListInfo>::iterator it = g_FamilyFavorRooms.find(FID);
	if (it == g_FamilyFavorRooms.end())
	{
		sipwarning("g_FamilyFavorRooms.find = NULL. FID=%d, RoomID=%d", FID, roomid);
		return;
	}
	std::map<T_ROOMID, _FavorRoomInfo>::iterator it1 = it->second.m_FavorRooms.find(roomid);
	if (it1 == it->second.m_FavorRooms.end())
	{
		sipwarning("m_FavorRooms.find == NULL. FID=%d, RoomID=%d", FID, roomid);
		return;
	}

	it->second.m_FavorRooms.erase(it1);
	{
		MAKEQUERY_DeleteFavorroom(strQ, FID, roomid);
		MainDB->execute(strQ, NULL);
	}
}

// Move favorite room to other group([d:Room ID][d:OldGroupID][d:NewGroupID])
void cb_M_CS_MOVFAVORROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;			_msgin.serial(FID);
	T_ROOMID	roomid;			_msgin.serial(roomid);
	T_ID		old_group;		_msgin.serial(old_group);
	T_ID		new_group;		_msgin.serial(new_group);

	std::map<T_FAMILYID, _FamilyFavorRoomListInfo>::iterator it = g_FamilyFavorRooms.find(FID);
	if (it == g_FamilyFavorRooms.end())
	{
		sipwarning("g_FamilyFavorRooms.find = NULL. FID=%d, RoomID=%d", FID, roomid);
		return;
	}
	std::map<T_ROOMID, _FavorRoomInfo>::iterator it1 = it->second.m_FavorRooms.find(roomid);
	if (it1 == it->second.m_FavorRooms.end())
	{
		sipwarning("m_FavorRooms.find == NULL. FID=%d, RoomID=%d", FID, roomid);
		return;
	}

	it1->second.GroupID = new_group;
	{
		MAKEQUERY_ChangeGroupFavorroom(strQ, FID, roomid, old_group, new_group);
		MainDB->execute(strQ, NULL);
	}
}

// Request create new favorite room group [u:GroupName][d:GroupID]
void	cb_M_CS_NEW_FAVORROOMGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	ucstring	ucGroup;	_msgin.serial(ucGroup);	// SAFE_SERIAL_WSTR(_msgin, ucGroup, MAX_FAVORGROUPNAME, FID);
	if (ucGroup.length() > MAX_FAVORGROUPNAME)
	{
		sipwarning("ucGroup.length() > MAX_FAVORGROUPNAME. FID=%d", FID);
		return;
	}
	uint32		GroupID;	_msgin.serial(GroupID);

	std::map<T_FAMILYID, _FamilyFavorRoomListInfo>::iterator it = g_FamilyFavorRooms.find(FID);
	if (it == g_FamilyFavorRooms.end())
	{
		sipwarning("g_FamilyFavorRooms.find = NULL. FID=%d", FID);
		return;
	}

	if (it->second.m_FavorRoomGroups.DataCount >= MAX_FAVORROOMGROUP)
	{
		sipwarning("m_FavorRoomGroups.DataCount overflow!! FID=%d, DataCount=%d", FID, it->second.m_FavorRoomGroups.DataCount);
		return;
	}

	// CheckData
	if ((int)GroupID < MAX_DEFAULT_SYSTEMGROUP_ID)
	{
		sipwarning("Invalid GroupID. FID=%d, GroupID=%d", FID, GroupID);
		return;
	}
	for (uint32 i = 0; i < it->second.m_FavorRoomGroups.DataCount; i ++)
	{
		if (it->second.m_FavorRoomGroups.Datas[i].GroupName == ucGroup)
		{
			sipwarning("Same Name Exist!! FID=%d", FID);
			return;
		}
		if (GroupID == it->second.m_FavorRoomGroups.Datas[i].GroupID)
		{
			sipwarning("Same GroupID Exist. FID=%d, GroupID=%d", FID, GroupID);
			return;
		}
	}

	// Insert New Album
	it->second.m_FavorRoomGroups.Datas[it->second.m_FavorRoomGroups.DataCount].GroupID = GroupID;
	wcscpy(it->second.m_FavorRoomGroups.Datas[it->second.m_FavorRoomGroups.DataCount].GroupName, ucGroup.c_str());
	it->second.m_FavorRoomGroups.DataCount ++;

	// Save
	SaveFavorRoomGroupList(FID);
}

// Request delete a favorite room group [d:GroupID]
void	cb_M_CS_DEL_FAVORROOMGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_ID		GroupID;	_msgin.serial(GroupID);

	std::map<T_FAMILYID, _FamilyFavorRoomListInfo>::iterator it = g_FamilyFavorRooms.find(FID);
	if (it == g_FamilyFavorRooms.end())
	{
		sipwarning("g_FamilyFavorRooms.find = NULL. FID=%d", FID);
		return;
	}

	// GroupID=1,-1 : Default Group
	if (GroupID < MAX_DEFAULT_SYSTEMGROUP_ID)
	{
		sipwarning("Invalid GroupID!! FID=%d, GroupID=%d", FID, GroupID);
		return;
	}

	int	GroupIndex;
	for (GroupIndex = 0; GroupIndex < (int)it->second.m_FavorRoomGroups.DataCount; GroupIndex ++)
	{
		if(it->second.m_FavorRoomGroups.Datas[GroupIndex].GroupID == GroupID)
			break;
	}
	if (GroupIndex >= (int)it->second.m_FavorRoomGroups.DataCount)
	{
		sipwarning("Invalid GroupID!! FID=%d, GroupID=%d", FID, GroupID);
		return;
	}

	for (int i = GroupIndex + 1; i < (int)it->second.m_FavorRoomGroups.DataCount; i ++)
	{
		it->second.m_FavorRoomGroups.Datas[i - 1] = it->second.m_FavorRoomGroups.Datas[i];
	}
	it->second.m_FavorRoomGroups.DataCount --;

	// Save
	SaveFavorRoomGroupList(FID);

	for (std::map<T_ROOMID, _FavorRoomInfo>::iterator it1 = it->second.m_FavorRooms.begin(); it1 != it->second.m_FavorRooms.end(); it1 ++)
	{
		if (it1->second.GroupID == GroupID)
			it1->second.GroupID = 1;
	}
	{
		MAKEQUERY_MoveFavorroomToDefaultGroup(strQ, FID, GroupID);
		MainDB->execute(strQ);
	}
}

// Request rename a favorite room group [d:GroupID][u:GroupName]
void	cb_M_CS_REN_FAVORROOMGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_ID		GroupID;	_msgin.serial(GroupID);
	ucstring	GroupName;	_msgin.serial(GroupName); // SAFE_SERIAL_WSTR(_msgin, GroupName, MAX_FAVORGROUPNAME, FID);
	if (GroupName.length() > MAX_FAVORGROUPNAME)
	{
		sipwarning("GroupName.length() > MAX_FAVORGROUPNAME. FID=%d", FID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFavorRoomListInfo>::iterator it = g_FamilyFavorRooms.find(FID);
	if (it == g_FamilyFavorRooms.end())
	{
		sipwarning("g_FamilyFavorRooms.find = NULL. FID=%d", FID);
		return;
	}

	int	GroupIndex = -1;
	for (int i = 0; i < (int)it->second.m_FavorRoomGroups.DataCount; i ++)
	{
		if (it->second.m_FavorRoomGroups.Datas[i].GroupID == GroupID)
			GroupIndex = i;
		else
		{
			if (it->second.m_FavorRoomGroups.Datas[i].GroupName == GroupName)
			{
				sipwarning("Same Name Already Exist!!! FID=%d", FID);
				return;
			}
		}
	}
	if (GroupIndex == -1)
	{
		sipwarning("Invalid GroupIndex!! FID=%d", FID);
		return;
	}

	// Save
	wcscpy(it->second.m_FavorRoomGroups.Datas[GroupIndex].GroupName, GroupName.c_str());

	SaveFavorRoomGroupList(FID);
}

static void	DBCB_DBLoadChitForSend(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	T_FAMILYID		FID			= *(T_FAMILYID *)argv[0];

	{
		struct	_CHIT
		{
			_CHIT(){};
			_CHIT(uint32 _id, T_FAMILYID _idSender, T_FAMILYID _idReciever, uint8	_idType, ucstring _nameSender, uint32 _p1, uint32 _p2, ucstring _p3, ucstring _p4)
			{
				id = _id;
				idSender	= _idSender;
				idReciever	= _idReciever;
				idChatType	= _idType;
				nameSender	= _nameSender;
				p1 = _p1;
				p2 = _p2;
				p3 = _p3;
				p4 = _p4;
			};
			uint32			id;
			T_FAMILYID		idSender;
			ucstring		nameSender;
			T_FAMILYID		idReciever;
			uint8			idChatType;
			uint32			p1;
			uint32			p2;
			ucstring		p3;
			ucstring		p4;
		};

		vector<_CHIT>	aryChit;
		{
			uint32		nRows;			resSet->serial(nRows);
			for (uint32 i = 0; i < nRows; i++)
			{
				T_FAMILYID		idSender;		resSet->serial(idSender);
				uint8			idType;			resSet->serial(idType);
				uint32			p1;				resSet->serial(p1);
				uint32			p2;				resSet->serial(p2);
				ucstring		p3;				resSet->serial(p3);
				ucstring		p4;				resSet->serial(p4);
				uint32			id;				resSet->serial(id);
				T_FAMILYNAME	nameSender;		resSet->serial(nameSender);

				if (idType != CHITTYPE_CHROOMMASTER)
				{
					_CHIT	chit(id, idSender, FID, idType, nameSender, p1, p2, p3, p4);
					aryChit.push_back(chit);
				}
			}
		}

		uint32 nAllChitSize = static_cast<uint32>(aryChit.size());
		uint32 nChitCount = 0;
		uint32 nGroup = 0;
		while (nChitCount < nAllChitSize)
		{
			uint16		r_sid = 0; // SIPNET::IService::getInstance()->getServiceId().get();
			CMessage	msgOut(M_SC_CHITLIST);
			msgOut.serial(FID, r_sid);
			uint32 num = (nAllChitSize - nChitCount > 20) ? 20 : (nAllChitSize - nChitCount);
			msgOut.serial(num);
			nGroup = 0;
			while (true)
			{
				msgOut.serial(aryChit[nChitCount].id, aryChit[nChitCount].idSender, aryChit[nChitCount].nameSender, aryChit[nChitCount].idChatType);
				msgOut.serial(aryChit[nChitCount].p1, aryChit[nChitCount].p2, aryChit[nChitCount].p3, aryChit[nChitCount].p4);
				nGroup ++;
				nChitCount ++;

				if (nGroup >= num)
					break;
			}
			SendPacketToFID(FID, msgOut);
		}
		//sipdebug("Send ChitList, num : %u", nAllChitSize); byy krs

		// 1회용 Chit들은 삭제한다.
		uint8	bDelete = 1;
		for (nChitCount = 0; nChitCount < nAllChitSize; nChitCount ++)
		{
			switch (aryChit[nChitCount].idChatType)
			{
			case CHITTYPE_MANAGER_ADD:
			case CHITTYPE_MANAGER_ANS:
			case CHITTYPE_MANAGER_DEL:
			case CHITTYPE_GIVEROOM_RESULT:
			case CHITTYPE_ROOM_DELETED:
				{
					MAKEQUERY_DeleteChit(strQ, aryChit[nChitCount].id);
					MainDB->execute(strQ);
				}
				break;
			}
		}
	}
}

void SendChitList(T_FAMILYID FID)
{
	MAKEQUERY_LoadChitList(strQ, FID);
	MainDB->executeWithParam(strQ, DBCB_DBLoadChitForSend,
		"D",
		CB_,		FID);
}

static void	DBCB_DBAddChit(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID	srcFID		= *(T_FAMILYID *)argv[0];
	ucstring	srcName		= (ucchar *)argv[1];
	T_FAMILYID	targetFID	= *(T_FAMILYID *)argv[2];
	uint8		ChitType	= *(uint8 *)argv[3];
	uint32		p1	= *(uint32 *)argv[4];
	uint32		p2	= *(uint32 *)argv[5];
	ucstring	p3	= (ucchar *)argv[6];
	ucstring	p4	= (ucchar *)argv[7];

	if (!nQueryResult)
		return;

	uint32	nRows;			resSet->serial(nRows);
	if (nRows < 1)
	{
		sipwarning("nRows < 1. srcFID=%d, targetFID=%d, ChitType=%d", srcFID, targetFID, ChitType);
		return;
	}
	uint32 ret;				resSet->serial(ret);
	if (ret != 0)
	{
		sipwarning("ret != 0. ret=%d, srcFID=%d, targetFID=%d, ChitType=%d", ret, srcFID, targetFID, ChitType);
		return;
	}
	uint32	chitid;			resSet->serial(chitid);

	{
		uint16		r_sid = 0;
		uint32		nChitCount = 1;

		CMessage	msgOut(M_SC_CHITLIST);
		msgOut.serial(targetFID, r_sid);
		msgOut.serial(nChitCount, chitid, srcFID, srcName, ChitType, p1, p2, p3, p4);

		SendPacketToFID(targetFID, msgOut);
	}
}

// Register new chit from shard to main ([b:ChitAddType, CHITADDTYPE_xxx][d:srcFID][u:srcName][d:targetFID][b:ChitType][d:p1][d:p2][u:p3][u:p4])
void	cb_M_NT_ADDCHIT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint8		ChitAddType, ChitType;
	T_FAMILYID	srcFID, targetFID;
	uint32		p1, p2;
	ucstring	srcName, p3, p4;

	_msgin.serial(ChitAddType, srcFID, srcName, targetFID, ChitType, p1, p2, p3, p4);

	if (ChitAddType == CHITADDTYPE_ADD_IF_OFFLINE)
	{
		uint16		r_sid = 0;
		uint32		nChit = 1;
		uint32		chitID = 0;

		CMessage	msgOut(M_SC_CHITLIST);
		msgOut.serial(targetFID, r_sid, nChit, chitID);
		msgOut.serial(srcFID, srcName, ChitType, p1, p2, p3, p4);
		if (SendPacketToFID(targetFID, msgOut))
			return;
	}

	MAKEQUERY_AddChit(strQuery, srcFID, targetFID, ChitType, p1, p2, p3, p4);
	if (ChitAddType == CHITADDTYPE_ADD_AND_SEND)
	{
		MainDB->executeWithParam(strQuery, DBCB_DBAddChit,
			"DSDBDDSS",
			CB_,			srcFID,
			CB_,			srcName.c_str(),
			CB_,			targetFID,
			CB_,			ChitType,
			CB_,			p1,
			CB_,			p2,
			CB_,			p3.c_str(),
			CB_,			p4.c_str());
	}
	else
	{
		MainDB->execute(strQuery);
	}
}

// Response for chit([w:r_sid][d:Chit id][b:response code])
void	cb_M_CS_RESCHIT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	uint16			r_sid;		_msgin.serial(r_sid);
	T_COMMON_ID		uChitID;	_msgin.serial(uChitID);
	uint8			uRes;		_msgin.serial(uRes);

	MAKEQUERY_DeleteChit(strQ, uChitID);
	MainDB->execute(strQ);
}

////////////////////////////////////////////////
//    예약의식
////////////////////////////////////////////////
struct YUYUE_DATA
{
public:
	uint32		RoomID;
	ucstring	RoomName;
	uint32		FID;
	ucstring	FamilyName;
	uint32		ModelID;
	uint32		DressID;
	uint32		FaceID;
	uint32		HairID;
	uint32		StartTime;
	uint32		YuyueDays;
	uint32		CountPerDay;
	uint32		YishiItemID;
	uint32		XianbaoItemID;
	uint32		YangyuItemID;
	uint32		TianItemID;
	uint32		DiItemID;
	uint32		FinishedDays;

	uint32		ReqTimeMin;

	YUYUE_DATA(uint32 _RoomID, ucstring _RoomName, uint32 _FID, ucstring _FamilyName, 
			uint32 _ModelID, uint32 _DressID, uint32 _FaceID, uint32 _HairID,
			uint32 _StartTime, uint32 _YuyueDays, uint32 _CountPerDay, 
			uint32 _YishiItemID, uint32 _XianbaoItemID, uint32 _YangyuItemID, uint32 _TianItemID, uint32 _DiItemID, uint32 _FinishedDays) : 
		RoomID(_RoomID), RoomName(_RoomName), FID(_FID), FamilyName(_FamilyName), 
			ModelID(_ModelID), DressID(_DressID), FaceID(_FaceID), HairID(_HairID), 
			StartTime(_StartTime), YuyueDays(_YuyueDays), CountPerDay(_CountPerDay), 
			YishiItemID(_YishiItemID), XianbaoItemID(_XianbaoItemID), YangyuItemID(_YangyuItemID), TianItemID(_TianItemID), DiItemID(_DiItemID), FinishedDays(_FinishedDays),
			ReqTimeMin(0) {};
};
std::map<uint64, YUYUE_DATA>	g_YuyueDatas;

struct YUYUE_EXP_DATA
{
public:
	uint32		YishiTimeMin;
	uint32		Autoplay3ID;
	uint32		RoomID;
	ucstring	RoomName;
	uint32		FID;
	ucstring	FamilyName;
	uint32		CountPerDay;
	uint32		YishiItemID;
	uint32		XianbaoItemID;
	uint32		YangyuItemID;
	uint32		TianItemID;
	uint32		DiItemID;

	uint32		Param;

	uint32		ReqTime;

	YUYUE_EXP_DATA(uint32 _YishiTimeMin, uint32 _Autoplay3ID, uint32 _RoomID, ucstring _RoomName, uint32 _FID, ucstring _FamilyName, 
			uint32 _CountPerDay, uint32 _YishiItemID, uint32 _XianbaoItemID, uint32 _YangyuItemID, uint32 _TianItemID, uint32 _DiItemID, uint32 _Param) : 
		YishiTimeMin(_YishiTimeMin), Autoplay3ID(_Autoplay3ID), RoomID(_RoomID), RoomName(_RoomName), FID(_FID), FamilyName(_FamilyName), 
			CountPerDay(_CountPerDay), YishiItemID(_YishiItemID), XianbaoItemID(_XianbaoItemID), YangyuItemID(_YangyuItemID), TianItemID(_TianItemID), DiItemID(_DiItemID), Param(_Param), 
			ReqTime(0) {};
};
std::list<YUYUE_EXP_DATA>	g_YuyueExpDatas;

void SendYuyueListToSS(uint32 UID, uint32 FID, uint16 sid)
{
const int  Yuyue_Send_Count = 30;
	{
		MAKEQUERY_GetYuyueList_FID(strQuery, FID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_QUERY(DB, Stmt, strQuery);
		if (nQueryResult != true)
		{
			sipwarning("Failed Loading Shard List, Query : %s", strQuery);
			return;
		}

		uint8	bStart = 1;
		while (true)
		{
			CMessage	msgOut(M_SC_AUTOPLAY3_LIST);
			msgOut.serial(UID, bStart);
			int	i = 0;

			for (; i < Yuyue_Send_Count; i ++)	// 30개씩 잘라서 보낸다.
			{
				DB_QUERY_RESULT_FETCH(Stmt, row);
				if ( !IS_DB_ROW_VALID(row) )
					break;

				COLUMN_DIGIT(row,	0,	uint32,		RoomID);
				COLUMN_WSTR(row,	1,	RoomName,	32);
				COLUMN_DIGIT(row,	2,	uint32,		StartTime);
				COLUMN_DIGIT(row,	3,	uint32,		YuyueDays);
				COLUMN_DIGIT(row,	4,	uint32,		CountPerDay);
				COLUMN_DIGIT(row,	5,	uint32,		YishiItemID);
				COLUMN_DIGIT(row,	6,	uint32,		XianbaoItemID);
				COLUMN_DIGIT(row,	7,	uint32,		YangyuItemID);
				COLUMN_DIGIT(row,	8,	uint32,		TianItemID);
				COLUMN_DIGIT(row,	9,	uint32,		DiItemID);
				COLUMN_DIGIT(row,	10,	uint32,		FinishedDays);

				msgOut.serial(RoomID, RoomName, StartTime, YuyueDays, CountPerDay, YishiItemID, XianbaoItemID, YangyuItemID);
				msgOut.serial(TianItemID, DiItemID, FinishedDays);
			}

			if (bStart || (i > 0))
			{
				if (sid == 0)
					CUnifiedNetwork::getInstance()->send(SS_S_NAME, msgOut);
				else
					CUnifiedNetwork::getInstance()->send(TServiceId(sid), msgOut);
			}

			bStart = 0;

			if (i < Yuyue_Send_Count)
				break;
		}
	}
}

static void	DBCB_DBAddYuyue(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32		RoomID		= *(uint32 *)argv[0];
	ucstring	RoomName	= (ucchar *)argv[1];
	uint32		FID			= *(uint32 *)argv[2];
	ucstring	FamilyName	= (ucchar *)argv[3];
	uint32		ModelID		= *(uint32 *)argv[4];
	uint32		DressID		= *(uint32 *)argv[5];
	uint32		FaceID		= *(uint32 *)argv[6];
	uint32		HairID		= *(uint32 *)argv[7];
	uint32		StartTime	= *(uint32 *)argv[8];
	uint32		YuyueDays	= *(uint32 *)argv[9];
	uint32		CountPerDay	= *(uint32 *)argv[10];
	uint32		YishiItemID	= *(uint32 *)argv[11];
	uint32		XianbaoItemID	= *(uint32 *)argv[12];
	uint32		YangyuItemID	= *(uint32 *)argv[13];
	uint32		TianItemID	= *(uint32 *)argv[14];
	uint32		DiItemID	= *(uint32 *)argv[15];

	uint32		nRows;			resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32		id;				resSet->serial(id);

		YUYUE_DATA	data(RoomID, RoomName, FID, FamilyName, ModelID, DressID, FaceID, HairID, 
						StartTime, YuyueDays, CountPerDay, YishiItemID, XianbaoItemID, YangyuItemID, TianItemID, DiItemID, 0);
		uint64	next_time = StartTime + 0 * 60 * 24;
		uint64	key = (next_time << 32) | id;
		g_YuyueDatas.insert(make_pair(key, data));

		//sipinfo("g_YuyueDatas.size() = %d, Autoplay3ID=%d", g_YuyueDatas.size(), id); byy krs
	}
}

// Register a AutoPlay3, OpenRoomService->WS->LS ([d:RoomID][u:RoomName][d:FID][u:FName][d:ModelID][d:DressID][d:FaceID][d:HairID][M1970:StartTime][d:YuyueDays][d:CountPerDay][d:YishiTaocanItemID][d:XianbaoTaocanItemID][d:YanhyuItemID][d:ItemID_Tian][d:ItemID_Di])
void	cb_M_NT_AUTOPLAY3_REGISTER(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	RoomID, FID, ModelID, DressID, FaceID, HairID, StartTime, YuyueDays, CountPerDay, YishiItemID, XianbaoItemID, YangyuItemID, TianItemID, DiItemID;
	ucstring	RoomName, FamilyName;

	_msgin.serial(RoomID, RoomName, FID, FamilyName, ModelID, DressID, FaceID, HairID);
	_msgin.serial(StartTime, YuyueDays, CountPerDay, YishiItemID, XianbaoItemID, YangyuItemID, TianItemID, DiItemID);

	{
		MAKEQUERY_AddYuyue(strQ, RoomID, RoomName, FID, FamilyName, ModelID, DressID, FaceID, HairID,
				StartTime, YuyueDays, CountPerDay, YishiItemID, XianbaoItemID, YangyuItemID, TianItemID, DiItemID);
		MainDB->executeWithParam(strQ, DBCB_DBAddYuyue,
			"DSDSDDDDDDDDDDDD",
			CB_,		RoomID,
			CB_,		RoomName.c_str(),
			CB_,		FID,
			CB_,		FamilyName.c_str(),
			CB_,		ModelID,
			CB_,		DressID,
			CB_,		FaceID,
			CB_,		HairID,
			CB_,		StartTime,
			CB_,		YuyueDays,
			CB_,		CountPerDay,
			CB_,		YishiItemID,
			CB_,		XianbaoItemID,
			CB_,		YangyuItemID,
			CB_,		TianItemID,
			CB_,		DiItemID);
	}
}

static void	DBCB_DBGetYuyueList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32		nRows;			resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32		id;				resSet->serial(id);
		uint32		RoomID;			resSet->serial(RoomID);
		ucstring	RoomName;		resSet->serial(RoomName);
		uint32		FID;			resSet->serial(FID);
		ucstring	FamilyName;		resSet->serial(FamilyName);
		uint32		ModelID;		resSet->serial(ModelID);
		uint32		DressID;		resSet->serial(DressID);
		uint32		FaceID;			resSet->serial(FaceID);
		uint32		HairID;			resSet->serial(HairID);
		uint32		StartTime;		resSet->serial(StartTime);
		uint32		YuyueDays;		resSet->serial(YuyueDays);
		uint32		CountPerDay;	resSet->serial(CountPerDay);
		uint32		YishiItemID;	resSet->serial(YishiItemID);
		uint32		XianbaoItemID;	resSet->serial(XianbaoItemID);
		uint32		YangyuItemID;	resSet->serial(YangyuItemID);
		uint32		TianItemID;		resSet->serial(TianItemID);
		uint32		DiItemID;		resSet->serial(DiItemID);
		uint32		FinishedDays;	resSet->serial(FinishedDays);

		if (YuyueDays <= FinishedDays)
		{
			uint32	zero = 0;
			{
				MAKEQUERY_YuyueFailed(strQuery, id, CHITTYPE_AUTOPLAY3_FAIL, zero);
				MainDB->executeWithParam(strQuery, NULL, "");
			}
			continue;
		}

		YUYUE_DATA	data(RoomID, RoomName, FID, FamilyName, ModelID, DressID, FaceID, HairID, 
						StartTime, YuyueDays, CountPerDay, YishiItemID, XianbaoItemID, YangyuItemID, TianItemID, DiItemID, FinishedDays);
		uint64	next_time = StartTime + FinishedDays * 60 * 24;
		uint64	key = (next_time << 32) | id;
		g_YuyueDatas.insert(make_pair(key, data));
	}
	//sipinfo("g_YuyueDatas.size() = %d", g_YuyueDatas.size()); byy krs
}

void LoadAllYuyueDatas()
{
	g_YuyueDatas.clear();

	MAKEQUERY_GetYuyueList(strQ);
	MainDB->execute(strQ, DBCB_DBGetYuyueList);
}

extern TTime GetDBTimeSecond();
void UpdateYuyueDatas()
{
	static uint32 prevTime = 0;
	uint32	curTime = CTime::getSecondsSince1970();
	if (prevTime == 0)
	{
		prevTime = curTime + 5 * 60;	// 메인써버가 기동한 후 지역써버기동시간을 충분히 5분으로 준다.
		return;
	}
	if (curTime < prevTime + 1 * 60)	// 1 min
		return;
	prevTime = curTime;

	uint32	curTimeMin = (uint32)(GetDBTimeSecond() / 60);
	uint32  sendCnt = 0;
	for (std::map<uint64, YUYUE_DATA>::iterator it = g_YuyueDatas.begin(); it != g_YuyueDatas.end(); it ++)
	{
		if (it->second.ReqTimeMin > 0)
		{
			if (curTimeMin - it->second.ReqTimeMin < 10)	// 10 minutes
				continue;
			it->second.ReqTimeMin = 0;
		}

		uint64	key = it->first;
		uint32	yishiTimeMin = (uint32)(key >> 32);
		uint32	id = (uint32) key;
		// g_YuyueDatas의 iterator가 key순서대로 나온다는것을 전제로 한다.
		if (yishiTimeMin > curTimeMin)
			return;

		// try Yishi
		{
			CMessage	msgOut(M_NT_AUTOPLAY3_START_REQ);
			msgOut.serial(yishiTimeMin, id, it->second.RoomID, it->second.FID, it->second.FamilyName);
			msgOut.serial(it->second.ModelID, it->second.DressID, it->second.FaceID, it->second.HairID);
			msgOut.serial(it->second.CountPerDay, it->second.YishiItemID, it->second.XianbaoItemID, it->second.YangyuItemID, it->second.TianItemID, it->second.DiItemID);

			uint8 ShardCode = (uint8)(it->second.RoomID >> 24);
			if (ShardCode == 0)
			{
				// 명인방인 경우에는 본인이 있는 써버로 보내는것으로 한다.
				ShardCode = (uint8)(it->second.FID >> 24) + 1;
			}
			SendMsgToShardByCode(msgOut, ShardCode);
			it->second.ReqTimeMin = curTimeMin;
			sendCnt++;
		}

		if (sendCnt >= 100) {
			return;
		}
	}
}

// M_NT_AUTOPLAY3_START_REQ의 응답  MS->WS->LS ([d:YishiTimeMin][d:Autoplay3ID][d:Param])
void	cb_M_NT_AUTOPLAY3_START_OK(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	YishiTimeMin, Autoplay3ID, Param;

	_msgin.serial(YishiTimeMin, Autoplay3ID, Param);

	uint64	key = YishiTimeMin;
	key = (key << 32) | Autoplay3ID;

	std::map<uint64, YUYUE_DATA>::iterator it = g_YuyueDatas.find(key);
	if (it == g_YuyueDatas.end())
	{
		sipwarning("g_YuyueDatas.find failed. YishiTimeMin=%d, Autoplay3ID=%d", YishiTimeMin, Autoplay3ID);
		return;
	}

	uint32		curRoomID = it->second.RoomID;
	ucstring	curRoomName = it->second.RoomName;
	uint32		curFID = it->second.FID;
	ucstring	curFamilyName = it->second.FamilyName;
	uint32		curCountPerDay = it->second.CountPerDay;
	uint32		curYishiItemID = it->second.YishiItemID;
	uint32		curXianbaoItemID = it->second.XianbaoItemID;
	uint32		curYangyuItemID = it->second.YangyuItemID;
	uint32		curTianItemID = it->second.TianItemID;
	uint32		curDiItemID = it->second.DiItemID;

	it->second.FinishedDays ++;
	if (it->second.FinishedDays < it->second.YuyueDays)
	{
		// DB처리
		{
			MAKEQUERY_YuyuePlayed(strQ, Autoplay3ID);
			MainDB->execute(strQ);
		}

		// 메모리처리
		YUYUE_DATA	data(it->second.RoomID, it->second.RoomName, it->second.FID, it->second.FamilyName, 
						it->second.ModelID, it->second.DressID, it->second.FaceID, it->second.HairID, 
						it->second.StartTime, it->second.YuyueDays, it->second.CountPerDay, 
						it->second.YishiItemID, it->second.XianbaoItemID, it->second.YangyuItemID, it->second.TianItemID, it->second.DiItemID, it->second.FinishedDays);
		uint64	next_time = it->second.StartTime + it->second.FinishedDays * 60 * 24;
		uint64	key1 = (next_time << 32) | Autoplay3ID;
		g_YuyueDatas.insert(make_pair(key1, data));
	}
	else
	{
		uint32	zero = 0;
		{
			MAKEQUERY_YuyueFailed(strQuery, Autoplay3ID, CHITTYPE_AUTOPLAY3_FAIL, zero);
			MainDB->executeWithParam(strQuery, NULL, "");
		}
	}

	// 경험값처리를 위한 등록
	{
		YUYUE_EXP_DATA	data(YishiTimeMin, Autoplay3ID, curRoomID, curRoomName, curFID, curFamilyName, 
			curCountPerDay, curYishiItemID, curXianbaoItemID, curYangyuItemID, curTianItemID, curDiItemID, Param);
		g_YuyueExpDatas.push_back(data);
	}

	g_YuyueDatas.erase(key);
	//sipinfo("g_YuyueDatas.size()=%d, g_YuyueExpDatas.size()=%d, Autoplay3ID=%d", g_YuyueDatas.size(), g_YuyueExpDatas.size(), Autoplay3ID); byy krs
}

void UpdateYuyueExpDatas()
{
	static uint32 prevTime = 0;
	uint32	curTime = CTime::getSecondsSince1970();
	if (curTime - prevTime < 1 * 10)	// 10 second
		return;
	prevTime = curTime;

	for (std::list<YUYUE_EXP_DATA>::iterator it = g_YuyueExpDatas.begin(); it != g_YuyueExpDatas.end(); it ++)
	{
		if (it->ReqTime > 0)
		{
			if (curTime - it->ReqTime < 180)	// 180 seconds
				continue;
			it->ReqTime = 0;
		}

		uint16	OnlineWsSid;
		uint32	OfflineShardID;
		if (!GetCurrentSafetyServer(it->FID, OnlineWsSid, OfflineShardID))
			continue;

		if (OnlineWsSid != 0)
		{
			CMessage	msgOut(M_NT_AUTOPLAY3_EXP_ADD_ONLINE);
			msgOut.serial(it->FID, it->YishiTimeMin, it->Autoplay3ID, it->FamilyName, it->RoomID, it->RoomName);
			msgOut.serial(it->CountPerDay, it->YishiItemID, it->XianbaoItemID, it->YangyuItemID, it->TianItemID, it->DiItemID, it->Param);
			CUnifiedNetwork::getInstance()->send(TServiceId(OnlineWsSid), msgOut);
		}
		else
		{
			CMessage	msgOut(M_NT_AUTOPLAY3_EXP_ADD_OFFLINE);
			msgOut.serial(it->FID, it->YishiTimeMin, it->Autoplay3ID, it->FamilyName, it->RoomID, it->RoomName);
			msgOut.serial(it->CountPerDay, it->YishiItemID, it->XianbaoItemID, it->YangyuItemID, it->TianItemID, it->DiItemID, it->Param);
			if (OfflineShardID != 0)
				SendMsgToShardByID(msgOut, OfflineShardID);
			else
			{
				uint8	ShardCode = (uint8)(it->FID >> 24) + 1;
				SendMsgToShardByCode(msgOut, ShardCode);
			}
		}

		it->ReqTime = curTime;
	}
}

// M_NT_AUTOPLAY3_EXP_ADD_ONLINE & M_NT_AUTOPLAY3_EXP_ADD_OFFLINE 의 응답 OROOM->WS->LS ([d:YishiTimeMin][d:Autoplay3ID])
void cb_M_NT_AUTOPLAY3_EXP_ADD_OK(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	YishiTimeMin, Autoplay3ID;
	_msgin.serial(YishiTimeMin, Autoplay3ID);

	for (std::list<YUYUE_EXP_DATA>::iterator it = g_YuyueExpDatas.begin(); it != g_YuyueExpDatas.end(); it ++)
	{
		if (it->YishiTimeMin == YishiTimeMin && it->Autoplay3ID == Autoplay3ID)
		{
			g_YuyueExpDatas.erase(it);
			//sipinfo("g_YuyueExpDatas.size()=%d, YishiTimeMin=%d, Autoplay3ID=%d", g_YuyueExpDatas.size(), YishiTimeMin, Autoplay3ID); byy krs
			return;
		}
	}
	//sipwarning("Can't find data. YishiTimeMin=%d, Autoplay3ID=%d", YishiTimeMin, Autoplay3ID); byy krs
}

// Autoplay3이 실패하였음을 LS에 통지한다. MS->WS->LS([d:RoomID][d:Param, 1-방열기 실패 , 2-방기한지남])
void cb_M_NT_AUTOPLAY3_FAIL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	RoomID, Param;
	_msgin.serial(RoomID, Param);

	// g_YuyueDatas 에서 RoomID관련 자료들을 다 처리한다.
	for (std::map<uint64, YUYUE_DATA>::iterator it = g_YuyueDatas.begin(); it != g_YuyueDatas.end(); )
	{
		if (it->second.RoomID != RoomID)
		{
			it ++;
			continue;
		}

		uint32	Autoplay3ID = (uint32)(it->first);
		{
			MAKEQUERY_YuyueFailed(strQuery, Autoplay3ID, CHITTYPE_AUTOPLAY3_FAIL, Param);
			MainDB->executeWithParam(strQuery, NULL, "");
		}

		it = g_YuyueDatas.erase(it);
	}
}
