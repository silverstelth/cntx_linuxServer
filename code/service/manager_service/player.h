#ifndef MANAGER_PLAYER_H
#define MANAGER_PLAYER_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include "../Common/Common.h"

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
//	uint8		PhotoType;
//	uint32		seldate;
//
//	FavorRoom()	{}
//	FavorRoom(T_ROOMID _rid, T_ID _fid, T_SCENEID _sceneid, T_ID _gid, T_NAME& _rname, T_FLAG _oflag, 
//		T_NAME& _mname, T_NAME& _DeadName1, uint8 _DeadSurnamelen1, T_NAME& _DeadName2, uint8 _DeadSurnamelen2, T_M1970 _expire, uint32 _deleteremainminutes, T_ROOMLEVEL _level, T_ROOMEXP _exp, uint32 _VisitCount, uint8 _photoType, uint32 _seldate) :
//	RoomId(_rid), FamilyId(_fid), SceneId(_sceneid), GroupId(_gid), RoomName(_rname), nOpenFlag(_oflag), 
//		MasterName(_mname), DeadName1(_DeadName1), DeadSurnamelen1(_DeadSurnamelen1), DeadName2(_DeadName2), DeadSurnamelen2(_DeadSurnamelen2), ExpireTime(_expire), DeleteRemainMinutes(_deleteremainminutes), level(_level), exp(_exp), VisitCount(_VisitCount), PhotoType(_photoType), seldate(_seldate)
//	{}
//};

#pragma pack(push, 1)
//struct _FavorroomGroupInfo
//{
//	uint32		GroupID;
//	ucchar		GroupName[MAX_FAVORGROUPNAME + 1];
//};
struct _RoomOrderInfo
{
	T_ROOMID	RoomID;

#ifdef SIP_OS_UNIX
	static bool isDiffSize() { return false; }
	static uint32 getSize() { return 0; }
	void serialize(uint8 *buf) { };
#endif
};
//typedef SDATA_LIST<_FavorroomGroupInfo, MAX_FAVORROOMGROUP>	FavorroomGroupList;
typedef SDATA_LIST<_RoomOrderInfo, MAX_FAVORROOMNUM>		RoomOrderInfoList;

struct _GroupRoomOrderInfo
{
	uint32				GroupID;
	RoomOrderInfoList	RoomOrderInfo;

#ifdef SIP_OS_UNIX
	static bool isDiffSize() { return false; }
	static uint32 getSize() { return 0; }
	void serialize(uint8 *buf) { }
#endif
};
typedef SDATA_LIST<_GroupRoomOrderInfo, MAX_FAVORROOMGROUP>		GroupRoomOrderInfoList;

#pragma pack(pop)

//class PlayerCheckinInfo
//{
//public:
//	PlayerCheckinInfo() : m_nCheckinDays(0), m_nLastCheckinDay(0) {}
//	~PlayerCheckinInfo() {}
//	uint8	m_nCheckinDays;
//	uint32	m_nLastCheckinDay;	// days from 1970.1.1
//};

class Player
{
public:
	Player();
	~Player();
	Player(T_ACCOUNTID _UserID, T_FAMILYID _FID, T_FAMILYNAME _FName, uint8 _bIsMobile, SIPNET::TServiceId _FSSid);
//	Player(T_FAMILYID _FID);

//	bool				m_bIsOnline;
//	bool				m_bIsOwnRoomOpened;

	// 일반정보
	T_ACCOUNTID			m_UserID;
	T_FAMILYID			m_FID;
	T_FAMILYNAME		m_FName;
	SIPNET::TServiceId	m_FSSid;
	uint8				m_bIsMobile;
	uint32				m_SessionState;

//	// Friend정보
//	FriendGroupList			m_ContactGroupList;
//	std::map<T_FAMILYID, T_FAMILYID>	m_FriendFIDs;
//	std::map<T_FAMILYID, T_FAMILYID>	m_BlackFIDs;

//	void ReadContactList(T_ROOMID RoomID = 0);
//	void SaveContactGroupList();
//	void ReadContactGroupList(SIPBASE::CMemStream *resSet);
//	void ReadContactFIDList(SIPBASE::CMemStream *resSet, T_ROOMID RoomID);
	void NotifyOnOffStatusToFriends(uint8 state);
	void CheckAndNotifyOnOffStatusToFriend(T_FAMILYID TargetFID, uint8 state);

	//void AddContactGroup(uint32 groupID, ucstring &groupName);
	//void DeleteContactGroup(uint32 groupID);
	//void RenameContactGroup(uint32 groupID, ucstring &groupName);
	//void ChangeContactGroupIndex(uint8 count, uint32 *GroupIDs);
	//void AddNewContact(T_FAMILYID contactFID, uint32 groupID, uint32 FriendIndex);
	//void DeleteContact(T_FAMILYID contactFID);

	//bool IsFriendList(T_FAMILYID targetFID) { return (m_FriendFIDs.find(targetFID) != m_FriendFIDs.end()) ? true : false; }
	//bool IsBlackList(T_FAMILYID targetFID) { return (m_BlackFIDs.find(targetFID) != m_BlackFIDs.end()) ? true : false; }

	// FavoriteRoomGroup정보
	//FavorroomGroupList	m_FavorroomGroupList;

	//void SendOwnFavoriteRoomInfo();
	//void DBLoadFavorroomGroupList(SIPBASE::CMemStream * resSet);
	//void SaveFavorroomGroup();
	//void AddFavorroomGroup(ucstring &ucGroup, uint32 GroupID);
	//void DelFavorroomGroup(uint32 GroupID);
	//void RenFavorroomGroup(ucstring &GroupName, uint32 GroupID);

	// RoomOrder정보
	bool					m_bChangedRoomOrderInfo;
	RoomOrderInfoList		m_OwnRoomOrderInfoList;
	RoomOrderInfoList		m_ManageRoomOrderInfoList;
	GroupRoomOrderInfoList	m_FavRoomOrderInfoList;

	//void ReadRoomOrderInfo();
	void DBLoadRoomOrderInfo(SIPBASE::CMemStream * resSet);
	void SaveOwnRoomOrderInfo(RoomOrderInfoList &newinfo);
	void SaveManageRoomOrderInfo(RoomOrderInfoList &newinfo);
	void SaveFaveRoomOrderInfo(uint32 GroupID, RoomOrderInfoList &newinfo);
	void SaveRoomOrderInfoToDB();

	// 현재 들어가있는 방정보
	T_ROOMID			m_RoomID;		// 0 :Lobby, 
	uint32				m_ChannelID;
	SIPNET::TServiceId	m_RoomSid;

	// 사용자형상사진정보
	uint32				m_FigureID;
	uint8				m_FigureSecretFlag;
	//void ReadFamilyFigure();
	void SetFamilyFigure(uint32 FigureID, uint8 FigureSecretFlag) { m_FigureID = FigureID; m_FigureSecretFlag = FigureSecretFlag; }
	void ChangeFamilyFigure(uint32 FigureID, uint8 FigureSecretFlag);
	uint8 GetFigureSecretFlag() { return m_FigureSecretFlag; }
	uint32 GetFigureID() { return m_FigureID; }

	//// 현재 진행중인 기념일활동정보
	//uint32		m_LastActID;

	//bool		m_IsGetBeginnerItem;

	// 축복카드
	uint8		m_BlessCardCount[MAX_BLESSCARD_ID];
	//void		LoadBlessCard();
	
	//PlayerCheckinInfo	m_CheckInInfo;
	//void		LoadCheckIn();
};

Player *GetPlayer(T_FAMILYID FID);
uint32 GetFamilySessionState(T_FAMILYID FID);
uint8 CheckContactStatus(T_FAMILYID masterFID, T_FAMILYID targetFID);
SIPBASE::TTime GetDBTimeSecond();
uint32 GetDBTimeDays();
uint32 getMinutesSince1970(std::string& date);
void	LoadLuckHistory();
void	SaveAndRefreshLuckHistory();
void	LoadBlessCardRecvHistory();
void	SaveBlessCardRecvHistory();
void	RecalLuck4Data(uint32 nNumber, bool bTest=false);
void	ResetLuck4List();
//void cb_M_NT_LUCK4_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void	SendLuck4List(T_FAMILYID FID, SIPNET::TServiceId _tsid);

// For Contact
void cb_M_CS_ADDCONTGROUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_DELCONTGROUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_RENCONTGROUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_SETCONTGROUP_INDEX(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_ADDCONT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_DELCONT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_CHANGECONTGRP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_SELFSTATE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void cb_M_CS_INVITE_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void cb_M_NT_FRIEND_GROUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_FRIEND_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_FRIEND_SN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_FRIEND_DELETED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_ANS_FAMILY_INFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void RequestFriendListInfo(T_FAMILYID FID, T_ROOMID RoomID);
bool IsInFriendList(T_FAMILYID srcFID, T_FAMILYID targetFID);
bool IsInBlackList(T_FAMILYID srcFID, T_FAMILYID targetFID);

//void LoadCheckIn(T_FAMILYID FID);

#endif // MANAGER_PLAYER_H
