#ifndef MANAGER_ROOM_H
#define MANAGER_ROOM_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "../Common/Common.h"

class Room
{
public:
	Room();
	~Room();
	Room(T_ROOMID _RoomID, T_SCENEID _SceneID, ucstring	_RoomName, T_FAMILYID _MasterFID, TServiceId _tsid);

	// 일반정보
	T_ROOMID	m_RoomID;
	T_SCENEID	m_SceneID;
	ucstring	m_RoomName;
	T_FAMILYID	m_MasterFID;
	TServiceId	m_RoomSID;
	int			m_nPlayerCount;
};

class RoomCreateWait
{
public:
	enum CREATE_ROOM_REASON
	{
		CREATE_ROOM_REASON_ENTER = 1,
		CREATE_ROOM_REASON_CHANGEMASTER_REQUEST,
		CREATE_ROOM_REASON_CHANGEMASTER_CANCEL,
		CREATE_ROOM_REASON_CHANGEMASTER_ANSWER,
		CREATE_ROOM_REASON_AUTOPLAY,
		CREATE_ROOM_REASON_CHECK_ENTER,
		CREATE_ROOM_REASON_CHANGEMASTER_FORCE,
	};

	RoomCreateWait();
	~RoomCreateWait();
	RoomCreateWait(T_ROOMID _RoomID, uint8 _Reason, uint32 _Param);

	T_ROOMID	m_RoomID;
	uint8		m_Reason;
	uint32		m_Param;
};

class RoomEnterWait
{
public:
	RoomEnterWait() {};
	~RoomEnterWait() {};
	RoomEnterWait(T_FAMILYID _FID, T_ROOMID _RoomID, string _AuthKey, uint32 _ChannelID)
		: m_FID(_FID), m_RoomID(_RoomID), m_AuthKey(_AuthKey), m_ChannelID(_ChannelID) {}

	T_FAMILYID	m_FID;
	T_ROOMID	m_RoomID;
	string		m_AuthKey;
	uint32		m_ChannelID;
};

class RoomChangemasterWait
{
public:
	RoomChangemasterWait() {};
	~RoomChangemasterWait() {};
	RoomChangemasterWait(T_FAMILYID _FID, T_ROOMID _RoomID, T_FAMILYID _toFID, string _TwoPassword)
		: m_FID(_FID), m_RoomID(_RoomID), m_toFID(_toFID), m_TwoPassword(_TwoPassword) {}

	T_FAMILYID	m_FID;
	T_ROOMID	m_RoomID;
	T_FAMILYID	m_toFID;
	string		m_TwoPassword;
};

class RoomChangemasterAnsWait
{
public:
	RoomChangemasterAnsWait() {};
	~RoomChangemasterAnsWait() {};
	RoomChangemasterAnsWait(T_FAMILYID _FID, T_ROOMID _RoomID, uint8 _Answer)
		: m_FID(_FID), m_RoomID(_RoomID), m_Answer(_Answer) {}

	T_FAMILYID	m_FID;
	T_ROOMID	m_RoomID;
	uint8		m_Answer;
};

class RoomChangemasterForceWait
{
public:
	RoomChangemasterForceWait() {};
	~RoomChangemasterForceWait() {};
	RoomChangemasterForceWait(T_FAMILYID _FID, T_ROOMID _RoomID)
		: m_FID(_FID), m_RoomID(_RoomID) {}

	T_FAMILYID	m_FID;
	T_ROOMID	m_RoomID;
};

class RoomChangemasterCancelWait
{
public:
	RoomChangemasterCancelWait() {};
	~RoomChangemasterCancelWait() {};
	RoomChangemasterCancelWait(T_FAMILYID _FID, T_ROOMID _RoomID, T_FAMILYID _toFID)
		: m_FID(_FID), m_RoomID(_RoomID), m_toFID(_toFID) {}

	T_FAMILYID	m_FID;
	T_ROOMID	m_RoomID;
	T_FAMILYID	m_toFID;
};

bool CreateOpenRoom(T_ROOMID RoomID, uint8 Reason, uint32 Param = 0, T_FAMILYID FID = 0);
bool IsOpenroom(uint32 RoomID);
void DoAfterRoomCreated(uint32 RoomID);
Room* GetRoom(T_ROOMID RoomID);

#endif // MANAGER_ROOM_H
