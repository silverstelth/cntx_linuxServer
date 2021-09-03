#include <misc/ucstring.h>
#include <net/service.h>

#include "misc/DBCaller.h"
#include "../../common/Macro.h"
#include "../../common/Packet.h"
#include "../../common/err.h"
#include "../Common/QuerySetForShard.h"
#include "../Common/Lang.h"
#include "../Common/netCommon.h"
#include "../Common/DBLog.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

#include "player.h"
#include "main.h"
#include "manageroom.h"
#include "zaixiankefu.h"
#include "toplist.h"

extern CDBCaller	*DBCaller;
//extern CDBCaller	*MainDBCaller;

//================================================
// Global Variables
//================================================
std::map<T_FAMILYID, Player>	g_Players;

struct TCURLUCKINPUB_INFO 
{
	TCURLUCKINPUB_INFO() 
	{
		LuckID = 0;
		FamilyID = NOFAMILYID;
		FamilyName = L"";
		TimeM = 0;
		TimeDeltaS = 0;
	}
	T_ID			LuckID;
	T_FAMILYID		FamilyID;
	T_FAMILYNAME	FamilyName;
	uint32			TimeM;
	uint8			TimeDeltaS;
};
static TCURLUCKINPUB_INFO g_CurLuckInPubInfo;


#pragma pack(push, 1)
struct _FriendGroupInfo
{
	T_ID		GroupID;
	ucchar		GroupName[MAX_LEN_CONTACT_GRPNAME + 1];
};
#pragma pack(pop)
typedef SDATA_LIST<_FriendGroupInfo, MAX_CONTGRPNUM + 1>	FriendGroupList;

struct _FriendInfo
{
	uint32		Index;
	T_ID		GroupID;
};

class _FamilyFriendListInfo
{
public:
	uint32	m_Changed_SN;

	FriendGroupList			m_FriendGroups;
	std::map<T_FAMILYID, _FriendInfo>	m_Friends;	// <FID, _FriendInfo>

	_FamilyFriendListInfo(uint32 sn)
	{
		m_Changed_SN = sn;
	}
};
std::map<T_FAMILYID, _FamilyFriendListInfo>	g_FamilyFriends;

//================================================
// Global Functions
//================================================

static void	DBCB_DBGetFamilyAllInfoForEnterShard_MS(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];

	if (!nQueryResult)
	{
		sipwarning("execute failed!");
		return;
	}

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	{
		uint32 nRows;	resSet->serial(nRows);
		if (nRows != 1)
		{
			sipwarning("Invalid nRows. FID=%d, nRows=%d", FID, nRows);
			return;
		}

		uint32		FigureID;		resSet->serial(FigureID);
		uint8		SecretFlag;		resSet->serial(SecretFlag);

		pPlayer->SetFamilyFigure(FigureID, SecretFlag);

		CMessage	msgOut(M_SC_FAMILY_FIGURE);
		msgOut.serial(FID, FID, FigureID, SecretFlag);
		CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
	}

	{
		for (uint32 i = 0; i < MAX_BLESSCARD_ID; i ++)
		{
			pPlayer->m_BlessCardCount[i] = 0;
		}

		CMessage	msgOut(M_SC_BLESSCARD_LIST);
		msgOut.serial(FID);

		uint32 nRows;	resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint8		BlessCardID, Count;
			resSet->serial(BlessCardID, Count);
			pPlayer->m_BlessCardCount[BlessCardID - 1] = Count;

			msgOut.serial(BlessCardID, Count);
		}

		CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
	}

	{
		pPlayer->DBLoadRoomOrderInfo(resSet);
	}
}

// Player Login ([d:FID][d:UserID][u:FamilyName])
void cb_M_NT_LOGIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ACCOUNTID		UserID;
	T_FAMILYNAME	FName;
	T_FAMILYNAME	UserName;
	uint8			bIsMobile;
	T_MODELID		ModelID;
	uint8			uUserType;
	bool			bGM;
	string			strIP;
	uint16			RoomSID;

	_msgin.serial(FID, UserID, FName, UserName, bIsMobile, ModelID, uUserType, bGM);
	_msgin.serial(strIP, RoomSID);

	TServiceId	fsSid(_tsid);

	std::map<T_FAMILYID, Player>::iterator it = g_Players.find(FID);
	if (it != g_Players.end())
	{
		it->second.m_UserID = UserID;
		it->second.m_FID = FID;
		it->second.m_FName = FName;
		it->second.m_bIsMobile = bIsMobile;
		it->second.m_FSSid = fsSid;
//		it->second.m_bIsOnline = true;
	}
	else
	{
		g_Players.insert(make_pair(FID, Player(UserID, FID, FName, bIsMobile, fsSid)));
		it = g_Players.find(FID);

		//MAKEQUERY_SetAppType(strQ, UserID, bIsMobile);
		//MainDBCaller->execute(strQ);
	}
	it->second.m_RoomID = 0;
	it->second.m_ChannelID = 0;
	it->second.m_RoomSid = TServiceId(RoomSID);

	RequestFriendListInfo(FID, 0);
	////it->second.SendOwnFavoriteRoomInfo();
	//it->second.ReadFamilyFigure();
	//it->second.LoadBlessCard();
	//LoadCheckIn(FID);
	//it->second.ReadRoomOrderInfo();

	//// Main에 기념일활동자료가 있는가를 요청한다.
	//extern void SendCheckUserActivity(uint32 UserID, uint32 FID);
	//SendCheckUserActivity(UserID, FID);

	//extern void SendCheckBeginnerMstItem(uint32 UserID, uint32 FID);
	//SendCheckBeginnerMstItem(UserID, FID);
	{
		MAKEQUERY_GetFamilyAllInfoForEnterShard_MS(strQ, FID);
		DBCaller->executeWithParam(strQ, DBCB_DBGetFamilyAllInfoForEnterShard_MS,
			"D",
			CB_,		FID);
	}

	{
		CMessage	msg(M_SC_CURLUCKINPUBROOM);
		msg.serial(FID);
		msg.serial(g_CurLuckInPubInfo.LuckID);
		msg.serial(g_CurLuckInPubInfo.FamilyID);
		msg.serial(g_CurLuckInPubInfo.FamilyName);
		msg.serial(g_CurLuckInPubInfo.TimeM);
		msg.serial(g_CurLuckInPubInfo.TimeDeltaS);
		CUnifiedNetwork::getInstance()->send(fsSid, msg);
	}

	SendBestSellItems(FID, fsSid);
	SendTopList(FID, fsSid);
	SendExpRoomList(FID, fsSid);
	SendLuck4List(FID, fsSid);

	//sipdebug(L"Player Login: FID=%d, UID=%d, FamilyName=%s", FID, UserID, FName.c_str()); byy krs
}

// Player Logout ([d:FID])
void cb_M_NT_LOGOUT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;

	_msgin.serial(FID);

	std::map<T_FAMILYID, Player>::iterator it = g_Players.find(FID);
	if (it == g_Players.end())
	{
		sipwarning("it == g_Players.end(). FID=%d", FID);
	}
	else
	{
//		if (it->second.m_bIsOnline)
		{
			it->second.NotifyOnOffStatusToFriends(CONTACTSTATE_OFFLINE);
		}
		it->second.SaveRoomOrderInfoToDB();
//		if (it->second.m_bIsOwnRoomOpened)
//			it->second.m_bIsOnline = false;
//		else
			g_Players.erase(it);

		//sipdebug("Player Logout: FID=%d", FID); byy krs
	}
	
	ProcForZaixianForLogout(FID);
}

// Player Enter a room ([d:FID][d:RoomID][d:ChannelID])
void cb_M_NT_PLAYER_ENTERROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	T_ID			ChannelID;

	_msgin.serial(FID, RoomID, ChannelID);

	std::map<T_FAMILYID, Player>::iterator it = g_Players.find(FID);
	if (it == g_Players.end())
	{
		sipwarning("it == g_Players.end(). FID=%d, RoomID=%d, ChannelID=%d", FID, RoomID, ChannelID);
	}
	else
	{
		it->second.m_RoomID = RoomID;
		it->second.m_ChannelID = ChannelID;
		it->second.m_RoomSid = _tsid;

		//sipdebug("Player Enter. FID=%d, RoomID=%d, ChannelID=%d", FID, RoomID, ChannelID); byy krs
	}
}

// Player Leave room ([d:FID][d:RoomID][d:ChannelID])
void cb_M_NT_PLAYER_LEAVEROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	T_ID			ChannelID;

	_msgin.serial(FID, RoomID, ChannelID);

	std::map<T_FAMILYID, Player>::iterator it = g_Players.find(FID);
	if (it == g_Players.end())
	{
//		sipwarning("it == g_Players.end(). FID=%d", FID);
	}
	else
	{
		if ((it->second.m_RoomID == RoomID) && (it->second.m_ChannelID == ChannelID))
		{
			//sipdebug("Player Leave. FID=%d, RoomID=%d, ChannelID=%d", FID, RoomID, ChannelID); byy krs
			it->second.m_RoomID = 0xFFFFFFFF;
			it->second.m_ChannelID = 0;
		}
		else
		{
//			if (it->second.m_RoomID != 0)
//				sipwarning("!((it->second.m_RoomID == RoomID) && (it->second.m_ChannelID == ChannelID)). FID=%d, RoomID=%d->%d, ChannelID=%d->%d", FID, it->second.m_RoomID ,RoomID, it->second.m_ChannelID, ChannelID);
		}
	}
}

// Player Changed channel ([d:FID][d:RoomID][d:ChannelID])
//void cb_M_NT_PLAYER_CHANGECHANNEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;
//	T_ROOMID		RoomID;
//	T_ID			ChannelID;
//
//	_msgin.serial(FID, RoomID, ChannelID);
//
//	std::map<T_FAMILYID, Player>::iterator it = g_Players.find(FID);
//	if (it == g_Players.end())
//	{
//		sipwarning("it == g_Players.end(). FID=%d", FID);
//	}
//	else
//	{
//		if (it->second.m_RoomID == RoomID)
//		{
//			sipdebug("Player change channel. FID=%d, RoomID=%d, NewChannelID=%d", FID, RoomID, ChannelID);
//		}
//		else
//		{
//			sipwarning("it->second.m_RoomID. FID=%d, RoomID=%d->%d, ChannelID=%d->%d", FID, it->second.m_RoomID ,RoomID, it->second.m_ChannelID, ChannelID);
//			it->second.m_RoomID = RoomID;
//		}
//		it->second.m_ChannelID = ChannelID;
//	}
//}

Player *GetPlayer(T_FAMILYID FID)
{
	std::map<T_FAMILYID, Player>::iterator it = g_Players.find(FID);
	if (it == g_Players.end())
	{
//		sipwarning("it == g_Players.end(). FID=%d", FID);
		return NULL;
	}

	return &(it->second);
}

uint32 GetFamilySessionState(T_FAMILYID FID)
{
	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipdebug("FAILED Getting Family Session State : There is no family id : %d", FID);
		return 0;
	}
	return pPlayer->m_SessionState;
}

void cb_M_CS_SELFSTATE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	T_SESSIONSTATE	SessionState;	_msgin.serial(SessionState);

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("FAILED Getting Family Session State : There is no family id : %d", FID);
		return;
	}

	pPlayer->m_SessionState = SessionState;
}

//// 방을 Open할 때 방주인의 친구목록을 미리 읽어들일 때 리용한다.
//void ReadContactList(T_FAMILYID FID, T_ROOMID RoomID)
//{
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		g_Players.insert(make_pair(FID, Player(FID)));
//		std::map<T_FAMILYID, Player>::iterator it = g_Players.find(FID);
//
//		it->second.ReadContactList(RoomID);
//
//		return;
//	}
//
//	pPlayer->m_bIsOwnRoomOpened = true;
//
//	DoAfterRoomCreated(RoomID);
//}

uint8 CheckContactStatus(T_FAMILYID masterFID, T_FAMILYID targetFID)
{
	Player *pPlayer = GetPlayer(masterFID);
	if (pPlayer == NULL)
		return 0;

	if (IsInFriendList(masterFID, targetFID))
		return 1;
	if (IsInBlackList(masterFID, targetFID))
		return 0xFF;

	return 0;
}

void cb_M_CS_INVITE_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	T_FAMILYID		targetFID;		_msgin.serial(targetFID);

	Player *pSrcPlayer = GetPlayer(FID);
	if (pSrcPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}
	Player *pDstPlayer = GetPlayer(targetFID);
	if (pDstPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d, targetFID=%d", FID, targetFID);
		return;
	}
//	if (!pDstPlayer->m_bIsOnline)
//	{
//		sipwarning("pDstPlayer->m_bIsOnline = FALSE. FID=%d, targetFID=%d", FID, targetFID);
//		return;
//	}

	if (pSrcPlayer->m_bIsMobile != pDstPlayer->m_bIsMobile)
	{
		sipwarning("Can't invite different client user.(PC & Mobile) FID=%d, IsMobile=%d, targetFID=%d, IsMobile=%d", FID, pSrcPlayer->m_bIsMobile, targetFID, pDstPlayer->m_bIsMobile);
		return;
	}

	{
		bool bSrcSystemUser = IsSystemAccount(pSrcPlayer->m_UserID);
		bool bDstSystemUser = IsSystemAccount(pDstPlayer->m_UserID);
		if (bSrcSystemUser != bDstSystemUser)
		{
			sipwarning("Can't invite different type user. FisrtUserID=%d, SecondUserID=%d", pSrcPlayer->m_UserID, pDstPlayer->m_UserID);
			return;
		}
	}

	Room *pSrcRoom = GetRoom(pSrcPlayer->m_RoomID);
	if (pSrcRoom == NULL)
	{
		sipwarning("GetRoom = NULL. pSrcPlayer->m_RoomID=%d", pSrcPlayer->m_RoomID);
		return;
	}

	uint16	client_FsSid = pSrcPlayer->m_FSSid.get();
	uint8	ContactStatus1 = CheckContactStatus(targetFID, FID);
	uint8	ContactStatus2 = CheckContactStatus(pSrcRoom->m_MasterFID, targetFID);

	CMessage	msgOut(M_CS_INVITE_REQ);
	msgOut.serial(FID, targetFID, client_FsSid, ContactStatus1, ContactStatus2);
	SendMsgToRoomID(pSrcPlayer->m_RoomID, msgOut);
}




//================================================
// Player Functions
//================================================
Player::Player()// : m_LastActID(0), m_IsGetBeginnerItem(true)
{
	m_RoomID = 0;
	m_ChannelID = 0;

//	m_bIsOnline = false;
//	m_bIsOwnRoomOpened = false;
	m_bChangedRoomOrderInfo = false;
	m_bIsMobile = 0;

	SetFamilyFigure(0, 0);
}

Player::Player(T_ACCOUNTID _UserID, T_FAMILYID _FID, T_FAMILYNAME _FName, uint8 _bIsMobile, SIPNET::TServiceId _FSSid)
	: m_UserID(_UserID), m_FID(_FID), m_FName(_FName), m_bIsMobile(_bIsMobile), m_FSSid(_FSSid)//, m_LastActID(0), m_IsGetBeginnerItem(true)
{
	// Lobby에 들어있는것으로 설정한다.
	m_RoomID = 0;
	m_ChannelID = 0;

//	m_bIsOnline = true;
//	m_bIsOwnRoomOpened = false;

	m_bChangedRoomOrderInfo = false;
	// m_RoomSid가 설정되지 않았다. 그러므로 이 값이 설정되기 전인 경우에 문제가 있을수 있다.???

	SetFamilyFigure(0, 0);
}

//Player::Player(T_FAMILYID _FID)
//	: m_FID(_FID), m_LastActID(0), m_IsGetBeginnerItem(true)
//{
//	m_RoomID = 0;
//	m_ChannelID = 0;
//
//	m_bIsOnline = false;
////	m_bIsOwnRoomOpened = true;
//	m_bChangedRoomOrderInfo = false;
//	m_bIsMobile = 0;
//
//	m_FSSid = SIPNET::TServiceId(0);
//
//	SetFamilyFigure(0, 0);
//}

Player::~Player()
{
}

//static void	DBCB_DBLoadChitForSend(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	if (!nQueryResult)
//		return;
//
//	T_FAMILYID		FID			= *(T_FAMILYID *)argv[0];
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	{
//		struct	_CHIT
//		{
//			_CHIT(){};
//			_CHIT(uint32 _id, T_FAMILYID _idSender, T_FAMILYID _idReciever, uint8	_idType, ucstring _nameSender, uint32 _p1, uint32 _p2, ucstring _p3, ucstring _p4)
//			{
//				id = _id;
//				idSender	= _idSender;
//				idReciever	= _idReciever;
//				idChatType	= _idType;
//				nameSender	= _nameSender;
//				p1 = _p1;
//				p2 = _p2;
//				p3 = _p3;
//				p4 = _p4;
//			};
//			uint32			id;
//			T_FAMILYID		idSender;
//			ucstring		nameSender;
//			T_FAMILYID		idReciever;
//			uint8			idChatType;
//			uint32			p1;
//			uint32			p2;
//			ucstring		p3;
//			ucstring		p4;
//		};
//
//		vector<_CHIT>	aryChit;
//		{
//			uint32		nRows;			resSet->serial(nRows);
//			for (uint32 i = 0; i < nRows; i++)
//			{
//				T_FAMILYID		idSender;		resSet->serial(idSender);
//				uint8			idType;			resSet->serial(idType);
//				uint32			p1;				resSet->serial(p1);
//				uint32			p2;				resSet->serial(p2);
//				ucstring		p3;				resSet->serial(p3);
//				ucstring		p4;				resSet->serial(p4);
//				uint32			id;				resSet->serial(id);
//				T_FAMILYNAME	nameSender;		resSet->serial(nameSender);
//
//				if (idType != CHITTYPE_CHROOMMASTER)
//				{
//					_CHIT	chit(id, idSender, FID, idType, nameSender, p1, p2, p3, p4);
//					aryChit.push_back(chit);
//				}
//			}
//		}
//
//		uint32 nAllChitSize = aryChit.size();
//		uint32 nChitCount = 0;
//		uint32 nGroup = 0;
//		while (nChitCount < nAllChitSize)
//		{
//			uint16		r_sid = SIPNET::IService::getInstance()->getServiceId().get();
//			CMessage	msgOut(M_SC_CHITLIST);
//			msgOut.serial(FID, r_sid);
//			uint32 num = (nAllChitSize - nChitCount > 20) ? 20 : (nAllChitSize - nChitCount);
//			msgOut.serial(num);
//			nGroup = 0;
//			while (true)
//			{
//				msgOut.serial(aryChit[nChitCount].id, aryChit[nChitCount].idSender, aryChit[nChitCount].nameSender, aryChit[nChitCount].idChatType);
//				msgOut.serial(aryChit[nChitCount].p1, aryChit[nChitCount].p2, aryChit[nChitCount].p3, aryChit[nChitCount].p4);
//				nGroup ++;
//				nChitCount ++;
//
//				if (nGroup >= num)
//					break;
//			}
//			CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
//		}
//		sipdebug("Send ChitList, num : %u", nAllChitSize);
//
//		// 1회용 Chit들은 삭제한다.
//		uint8	bDelete = 1;
//		for (nChitCount = 0; nChitCount < nAllChitSize; nChitCount ++)
//		{
//			switch(aryChit[nChitCount].idChatType)
//			{
//			case CHITTYPE_MANAGER_ADD:
//			case CHITTYPE_MANAGER_ANS:
//			case CHITTYPE_MANAGER_DEL:
//			case CHITTYPE_GIVEROOM_RESULT:
//			case CHITTYPE_ROOM_DELETED:
//				{
//					MAKEQUERY_GETCHIT(strQ, aryChit[nChitCount].id, FID, bDelete);
//					DBCaller->execute(strQ, NULL);
//				}
//				break;
//			}
//		}
//	}
//}

//void cb_M_CS_RESCHIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	uint16			r_sid;		_msgin.serial(r_sid);
//	T_COMMON_ID		uChitID;	_msgin.serial(uChitID);
//	uint8			uRes;		_msgin.serial(uRes);
//
//	bool	bDelete = true;
//	MAKEQUERY_GETCHIT(strQ, uChitID, FID, bDelete);
//	DBCaller->execute(strQ);
//}

//static void	DBCB_DBGetFriendListForSend(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];
//	T_ROOMID		RoomID = *(T_ROOMID *)argv[1];
//
//	if (!nQueryResult)
//	{
//		sipwarning("Shrd_Friend_List: execute failed!");
//		return;
//	}
//
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//	pPlayer->ReadContactFIDList(resSet, RoomID);
//}

//void Player::ReadContactFIDList(CMemStream *resSet, T_ROOMID RoomID)
//{
//#define Friend_Send_Count	10
//	struct _FriendInfo
//	{
//		T_ID		GroupID;
//		T_FAMILYID	FID;
//		T_FAMILYNAME	FName;
//		uint32		Index;
//		uint8		bOnline;
//		uint32		OneselfState;
//	};
//
//	m_FriendFIDs.clear();
//	m_BlackFIDs.clear();
//
//	// resSet 처리
//	uint8			bFinished = 0;
//	uint32			bSendCount = 0;
//	bool			bSended = false;
//	uint8			count = 0;
//	_FriendInfo		Friends[Friend_Send_Count];
//	uint32			nRows;		resSet->serial(nRows);
//	for (uint32 i = 0; i < nRows; i ++)
//	{
//		uint32			_fid;			resSet->serial(_fid);
//		T_FAMILYNAME	_fname;			resSet->serial(_fname);
//		uint32			_index;			resSet->serial(_index);
//		uint32			_groupid;		resSet->serial(_groupid);
//		uint8			_bonline;		resSet->serial(_bonline);
//		uint8			_disabled;		resSet->serial(_disabled);
//
//		if ((int)_groupid < 0)
//			m_BlackFIDs[_fid] = _fid;
//		else
//			m_FriendFIDs[_fid] = _fid;
//
//		if (m_bIsOnline)
//		{
//			if (_disabled)
//				_bonline = CONTACTSTATE_OFFLINE;
//			if (_bonline == CONTACTSTATE_ONLINE)
//			{
//				// 靸侂寑鞐愱矊 鞛愱赴臧€ 旃滉惮搿?霌彪霅橃棳鞛堧姅 瓴届毎鞐愲 鞓澕鞚胳溂搿?氤措偢雼?
//				Player	*pTargetPlayer = GetPlayer(_fid);
//				if (pTargetPlayer == NULL)
//				{
//					sipwarning("GetPlayer = NULL. FID=%d", _fid);
//				}
//				else
//				{
//					if (!pTargetPlayer->IsFriendList(m_FID))
//						_bonline = CONTACTSTATE_OFFLINE;
//					else
//						_bonline |= (pTargetPlayer->m_bIsMobile << 4);
//				}
//			}
//			Friends[count].GroupID = _groupid;
//			Friends[count].FID = _fid;
//			Friends[count].FName = _fname;
//			Friends[count].Index = _index;
//			Friends[count].bOnline = _bonline;
//			Friends[count].OneselfState = 0;
//
//			if (_bonline == CONTACTSTATE_ONLINE)	// contact on && family on
//				Friends[count].OneselfState = GetFamilySessionState(_fid);
//
//			count ++;
//
//			if (count >= Friend_Send_Count)
//			{
//				bSendCount += count;
//				if (bSendCount >= nRows)
//					bFinished = 1;
//				CMessage	msgFriends(M_SC_MYCONTLIST);
//				msgFriends.serial(m_FID, count, bFinished);
//				for (uint8 i = 0; i < count; i ++)
//				{
//					msgFriends.serial(Friends[i].GroupID, Friends[i].FID, Friends[i].FName, Friends[i].Index, Friends[i].bOnline, Friends[i].OneselfState);
//				}
//				CUnifiedNetwork::getInstance()->send(m_FSSid, msgFriends);
//				count = 0;
//				bSended = true;
//			}
//		}
//	}
//
//	if (m_bIsOnline)
//	{
//		if ((count > 0) || !bSended) // // 자료가 없는 경우에도 반드시 한번은 보내야 한다. - bSended
//		{
//			bFinished = 1;
//			CMessage	msgFriends(M_SC_MYCONTLIST);
//			msgFriends.serial(m_FID, count, bFinished);
//			for (uint8 i = 0; i < count; i ++)
//			{
//				msgFriends.serial(Friends[i].GroupID, Friends[i].FID, Friends[i].FName, Friends[i].Index, Friends[i].bOnline, Friends[i].OneselfState);
//			}
//			CUnifiedNetwork::getInstance()->send(m_FSSid, msgFriends);
//		}
//
//		NotifyOnOffStatusToFriends(CONTACTSTATE_ONLINE);
//	}
//
//	if (m_bIsOnline)
//	{
//		MAKEQUERY_LOADCHITLISTTOHIM(strQ, m_FID);
//		DBCaller->executeWithParam(strQ, DBCB_DBLoadChitForSend,
//			"D",
//			CB_,		m_FID);
//	}
//
//	if (RoomID != 0)
//		DoAfterRoomCreated(RoomID);
//}

//static void	DBCB_DBGetFriendGroupListForSend(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];
//
//	if (!nQueryResult)
//	{
//		sipwarning("Shrd_Friend_Group_list2: execute failed!");
//		return;
//	}
//
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->ReadContactGroupList(resSet);
//}

//void Player::ReadContactGroupList(CMemStream *resSet)
//{
//	// resSet 처리
//	uint8	bFriendGroupList[MAX_CONTGRP_BUF_SIZE];
//	LONG	lGroupBufSize = sizeof(bFriendGroupList);
//
//	uint32	nRows;		resSet->serial(nRows);
//	if (nRows == 0)
//	{
//		m_ContactGroupList.DataCount = 2;
//		m_ContactGroupList.Datas[0].GroupID = -1;
//		wcscpy(m_ContactGroupList.Datas[0].GroupName, L"Black");
//		m_ContactGroupList.Datas[1].GroupID = 1;
//		wcscpy(m_ContactGroupList.Datas[1].GroupName, GetLangText(ID_FRIEND_DEFAULTGROUP_NAME).c_str()); //L"\x597D\x53CB"); // L"閬娿叺鞜?); //  
//
//		SaveContactGroupList();
//	}
//	else
//	{
//		resSet->serial(lGroupBufSize);
//		resSet->serialBuffer(bFriendGroupList, lGroupBufSize);
//		if(!m_ContactGroupList.fromBuffer(bFriendGroupList, (uint32)lGroupBufSize))
//			sipwarning("FriendGroup.fromBuffer failed. FID=%d", m_FID);
//	}
//
//	if (m_bIsOnline)
//	{
//		// Packet를 보낸다.
//		CMessage	msgOut(M_SC_MYCONTGROUP);
//		uint8		count = (uint8) m_ContactGroupList.DataCount;
//		msgOut.serial(m_FID, count);
//		for(uint32 i = 0; i < m_ContactGroupList.DataCount; i ++)
//		{
//			ucstring	name(m_ContactGroupList.Datas[i].GroupName);
//			msgOut.serial(m_ContactGroupList.Datas[i].GroupID, name);
//		}
//		CUnifiedNetwork::getInstance()->send(m_FSSid, msgOut);
//	}
//
//	// Send Friend List
//	{
//		T_ROOMID	RoomID = 0;
//		MAKEQUERY_GetFriendList(strQ, m_FID);
//		DBCaller->executeWithParam(strQ, DBCB_DBGetFriendListForSend,
//			"DD",
//			CB_,		m_FID,
//			CB_,		RoomID);
//	}
//}

//void Player::ReadContactList(T_ROOMID RoomID)
//{
//	if (RoomID == 0)
//	{
//		// Player가 로그인할 때
//		// Send Friend Group List
//		MAKEQUERY_LoadFriendGroupList(strQ, m_FID);
//		DBCaller->executeWithParam(strQ, DBCB_DBGetFriendGroupListForSend,
//			"D",
//			CB_,		m_FID);
//	}
//	else
//	{
//		// Player가 직접 로그인하지 않고, 방을 Open할 때 그 방의 주인Player의 친구목록을 load할 때
//		// Send Friend List
//		MAKEQUERY_GetFriendList(strQ, m_FID);
//		DBCaller->executeWithParam(strQ, DBCB_DBGetFriendListForSend,
//			"DD",
//			CB_,		m_FID,
//			CB_,		RoomID);
//	}
//}

// 친구에게 온라인/오프라인상태를 통지한다.
void Player::NotifyOnOffStatusToFriends(uint8 state)
{
	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(m_FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find = NULL. FID=%d", m_FID);
		return;
	}
	_FamilyFriendListInfo	&friendData = it->second;

	uint32	zero = 0;
	state |= (m_bIsMobile << 4);

	for (std::map<T_FAMILYID, _FriendInfo>::iterator it1 = friendData.m_Friends.begin(); it1 != friendData.m_Friends.end(); it1 ++)
	{
		if (it1->second.GroupID == (uint32)(-1))
			continue;

		T_FAMILYID	friendFID = it1->first;
		Player	*pFriend = GetPlayer(friendFID);
		if (pFriend != NULL)
		{
			CMessage	msgOut(M_SC_CONTSTATE);
			msgOut.serial(friendFID, zero);
			msgOut.serial(m_FID, state, zero);

			CUnifiedNetwork::getInstance()->send(pFriend->m_FSSid, msgOut);
		}
	}
}

void Player::CheckAndNotifyOnOffStatusToFriend(T_FAMILYID TargetFID, uint8 state)
{
	uint32	zero = 0;
	state |= (m_bIsMobile << 4);

	Player	*pTargetPlayer = GetPlayer(TargetFID);
	if (pTargetPlayer != NULL)	// 온라인되여있으면
	{
		if (IsInFriendList(m_FID, TargetFID))
		{
			CMessage	msgOut(M_SC_CONTSTATE);
			msgOut.serial(TargetFID, zero);
			msgOut.serial(m_FID, state, m_SessionState);

			CUnifiedNetwork::getInstance()->send(pTargetPlayer->m_FSSid, msgOut);
		}
	}
}
//void Player::CheckAndNotifyOnOffStatusToFriend(T_FAMILYID TargetFID, uint8 state)
//{
//	uint32	zero = 0;
//	state |= (m_bIsMobile << 4);
//
//	Player	*pTargetPlayer = GetPlayer(TargetFID);
//	if (pTargetPlayer != NULL && pTargetPlayer->m_bIsOnline)
//	{
//		if (pTargetPlayer->IsFriendList(m_FID))
//		{
//			CMessage	msgOut(M_SC_CONTSTATE);
//			msgOut.serial(TargetFID, zero);
//			msgOut.serial(m_FID, state, m_SessionState);
//
//			CUnifiedNetwork::getInstance()->send(pTargetPlayer->m_FSSid, msgOut);
//		}
//	}
//}

//void Player::SaveContactGroupList()
//{
//	uint8	bFriendGroupList[MAX_CONTGRP_BUF_SIZE];
//	uint32	GroupBufferSize = sizeof(bFriendGroupList);
//
//	if (!m_ContactGroupList.toBuffer(bFriendGroupList, &GroupBufferSize))
//	{
//		sipwarning("m_ContactGroupList.toBuffer failed. FID=%d", m_FID);
//		return;
//	}
//
//	char str_buf[MAX_CONTGRP_BUF_SIZE * 2 + 10];
//	for (uint32 i = 0; i < GroupBufferSize; i ++)
//	{
//		sprintf(&str_buf[i * 2], ("%02x"), bFriendGroupList[i]);
//	}
//	str_buf[GroupBufferSize * 2] = 0;
//
//	MAKEQUERY_SaveFriendGroupList1(strQ, m_FID, str_buf);
//	DBCaller->executeWithParam(strQ, NULL,
//		"D",
//		OUT_,		NULL);
//}

bool IsInFriendList(T_FAMILYID srcFID, T_FAMILYID targetFID)
{
	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(srcFID);
	if (it == g_FamilyFriends.end())
		return false;
	_FamilyFriendListInfo	&friendData = it->second;

	std::map<T_FAMILYID, _FriendInfo>::iterator it1 = friendData.m_Friends.find(targetFID);
	if (it1 == friendData.m_Friends.end())
		return false;

	if (it1->second.GroupID == (uint32)(-1))
		return false;

	return true;
}

bool IsInBlackList(T_FAMILYID srcFID, T_FAMILYID targetFID)
{
	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(srcFID);
	if (it == g_FamilyFriends.end())
		return false;
	_FamilyFriendListInfo	&friendData = it->second;

	std::map<T_FAMILYID, _FriendInfo>::iterator it1 = friendData.m_Friends.find(targetFID);
	if (it1 == friendData.m_Friends.end())
		return false;

	if (it1->second.GroupID == (uint32)(-1))
		return true;

	return false;
}

// Packet: M_CS_ADDCONTGROUP
// Add contact group([u:Name][d:GroupId])
void cb_M_CS_ADDCONTGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;			_msgin.serial(FID);
	ucstring	ucGroup;		_msgin.serial(ucGroup);
	T_ID		GroupID;		_msgin.serial(GroupID);

	if (ucGroup == L"" || ucGroup.length() > MAX_LEN_CONTACT_GRPNAME)
	{
		sipwarning(L"Invalid ContGroup Name. GroupName=%s, FID=%d", ucGroup.c_str(), FID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find failed. FID=%d", FID);
		return;
	}

	_FamilyFriendListInfo	&friendData = it->second;

	if(friendData.m_FriendGroups.DataCount >= MAX_CONTGRPNUM + 1)
	{
		sipwarning("friendData.m_FriendGroups.DataCount overflow!! FID=%d", FID);
		return;
	}

	// CheckData
	if((int)GroupID < MAX_DEFAULT_SYSTEMGROUP_ID)
	{
		sipwarning("Invalid GroupID. FID=%d, GroupID=%d", FID, GroupID);
		return;
	}
	for(uint32 i = 0; i < friendData.m_FriendGroups.DataCount; i ++)
	{
		if(friendData.m_FriendGroups.Datas[i].GroupName == ucGroup)
		{
			sipwarning(L"Same Name Exist. FID=%d, GroupName=%s", FID, ucGroup.c_str());
			return;
		}
		if(GroupID == friendData.m_FriendGroups.Datas[i].GroupID)
		{
			sipwarning("Same GroupID Exist. FID=%d, GroupID=%d", FID, GroupID);
			return;
		}
	}

	// Insert New Group
	friendData.m_FriendGroups.Datas[friendData.m_FriendGroups.DataCount].GroupID = GroupID;
	wcscpy(friendData.m_FriendGroups.Datas[friendData.m_FriendGroups.DataCount].GroupName, ucGroup.c_str());
	friendData.m_FriendGroups.DataCount ++;

	// Send to LS
	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
}

//void cb_M_CS_ADDCONTGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;			_msgin.serial(FID);
//	ucstring	ucGroup;		_msgin.serial(ucGroup);
//	T_ID		GroupID;		_msgin.serial(GroupID);
//
//	if (ucGroup == L"" || ucGroup.length() > MAX_LEN_CONTACT_GRPNAME)
//	{
//		sipwarning(L"Invalid ContGroup Name. GroupName=%s", ucGroup.c_str());
//		return;
//	}
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->AddContactGroup(GroupID, ucGroup);
//}

//void Player::AddContactGroup(uint32 GroupID, ucstring &ucGroup)
//{
//	if(m_ContactGroupList.DataCount >= MAX_CONTGRPNUM + 1)
//	{
//		sipwarning("m_ContactGroupList.DataCount overflow!! FID=%d", m_FID);
//		return;
//	}
//
//	// CheckData
//	if((int)GroupID < MAX_DEFAULT_SYSTEMGROUP_ID)
//	{
//		sipwarning("Invalid GroupID. FID=%d, GroupID=%d", m_FID, GroupID);
//		return;
//	}
//	for(uint32 i = 0; i < m_ContactGroupList.DataCount; i ++)
//	{
//		if(m_ContactGroupList.Datas[i].GroupName == ucGroup)
//		{
//			sipwarning(L"Same Name Exist. FID=%d, GroupName=%s", m_FID, ucGroup.c_str());
//			return;
//		}
//		if(GroupID == m_ContactGroupList.Datas[i].GroupID)
//		{
//			sipwarning("Same GroupID Exist. FID=%d, GroupID=%d", m_FID, GroupID);
//			return;
//		}
//	}
//
//	// Insert New Group
//	m_ContactGroupList.Datas[m_ContactGroupList.DataCount].GroupID = GroupID;
//	wcscpy(m_ContactGroupList.Datas[m_ContactGroupList.DataCount].GroupName, ucGroup.c_str());
//	m_ContactGroupList.DataCount ++;
//
//	// Save
//	SaveContactGroupList();
//
//	sipinfo(L"user:%u create contact group:%s", m_FID, ucGroup.c_str());
//}

// Packet: M_CS_DELCONTGROUP
// Delete contact group([d:GroupID])
void cb_M_CS_DELCONTGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_ID			GroupID;	_msgin.serial(GroupID);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find failed. FID=%d", FID);
		return;
	}

	_FamilyFriendListInfo	&friendData = it->second;

	// GroupID=1,-1 : Default Group
	if ((GroupID < MAX_DEFAULT_SYSTEMGROUP_ID) || (GroupID == (uint32)(-1)))
	{
		sipwarning("Invalid GroupID!! GroupID=%d, FID=%d", GroupID, FID);
		return;
	}

	int	GroupIndex;
	for (GroupIndex = 0; GroupIndex < (int)friendData.m_FriendGroups.DataCount; GroupIndex ++)
	{
		if(friendData.m_FriendGroups.Datas[GroupIndex].GroupID == GroupID)
			break;
	}
	if(GroupIndex >= (int)friendData.m_FriendGroups.DataCount)
	{
		sipwarning("Invalid GroupID!! FID=%d, GroupID=%d", FID, GroupID);
		return;
	}

	for(int i = GroupIndex + 1; i < (int)friendData.m_FriendGroups.DataCount; i ++)
	{
		friendData.m_FriendGroups.Datas[i - 1] = friendData.m_FriendGroups.Datas[i];
	}
	friendData.m_FriendGroups.DataCount --;

	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
}

//void cb_M_CS_DELCONTGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	T_ID			GroupID;	_msgin.serial(GroupID);
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->DeleteContactGroup(GroupID);
//}

//void Player::DeleteContactGroup(uint32 GroupID)
//{
//	// GroupID=1,-1 : Default Group
//	if ((GroupID < MAX_DEFAULT_SYSTEMGROUP_ID) || (GroupID == (uint32)(-1)))
//	{
//		sipwarning("Invalid GroupID!! GroupID=%d", GroupID);
//		return;
//	}
//
//	int	GroupIndex;
//	for (GroupIndex = 0; GroupIndex < (int)m_ContactGroupList.DataCount; GroupIndex ++)
//	{
//		if(m_ContactGroupList.Datas[GroupIndex].GroupID == GroupID)
//			break;
//	}
//	if(GroupIndex >= (int)m_ContactGroupList.DataCount)
//	{
//		sipwarning("Invalid GroupID!! FID=%d, GroupID=%d", m_FID, GroupID);
//		return;
//	}
//
//	for(int i = GroupIndex + 1; i < (int)m_ContactGroupList.DataCount; i ++)
//	{
//		m_ContactGroupList.Datas[i - 1] = m_ContactGroupList.Datas[i];
//	}
//	m_ContactGroupList.DataCount --;
//
//	// Save
//	SaveContactGroupList();
//
//	sipinfo("DeleteContGroup : user:%u delete group:%d", m_FID, GroupID);
//}

// Packet: M_CS_RENCONTGROUP
// Change contact group name([d:GroupID][u:new Group name])
void cb_M_CS_RENCONTGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_ID			GroupID;	_msgin.serial(GroupID);
	ucstring		ucGroup;	_msgin.serial(ucGroup);

	if (ucGroup == L"" || ucGroup.length() > MAX_LEN_CONTACT_GRPNAME)
	{
		sipwarning(L"Invalid ContGroup Name. GroupName=%s, FID=%d", ucGroup.c_str(), FID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find failed. FID=%d", FID);
		return;
	}

	_FamilyFriendListInfo	&friendData = it->second;

	int	GroupIndex = -1;
	for(int i = 0; i < (int)friendData.m_FriendGroups.DataCount; i ++)
	{
		if(friendData.m_FriendGroups.Datas[i].GroupID == GroupID)
			GroupIndex = i;
		else
		{
			if(friendData.m_FriendGroups.Datas[i].GroupName == ucGroup)
			{
				sipwarning(L"Same Name Already Exist!!! FID=%d, GroupName=%s", FID, ucGroup.c_str());
				return;
			}
		}
	}
	if(GroupIndex == -1)
	{
		sipwarning("Invalid GroupIndex!! FID=%d", FID);
		return;
	}

	// Save
	wcscpy(friendData.m_FriendGroups.Datas[GroupIndex].GroupName, ucGroup.c_str());

	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
}

//void cb_M_CS_RENCONTGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	T_ID			GroupID;	_msgin.serial(GroupID);
//	ucstring		ucGroup;	_msgin.serial(ucGroup);
//
//	if (ucGroup == L"" || ucGroup.length() > MAX_LEN_CONTACT_GRPNAME)
//	{
//		sipwarning(L"Invalid ContGroup Name. GroupName=%s", ucGroup.c_str());
//		return;
//	}
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->RenameContactGroup(GroupID, ucGroup);
//}

//void Player::RenameContactGroup(uint32 GroupID, ucstring &GroupName)
//{
//	int	GroupIndex = -1;
//	for(int i = 0; i < (int)m_ContactGroupList.DataCount; i ++)
//	{
//		if(m_ContactGroupList.Datas[i].GroupID == GroupID)
//			GroupIndex = i;
//		else
//		{
//			if(m_ContactGroupList.Datas[i].GroupName == GroupName)
//			{
//				sipwarning(L"Same Name Already Exist!!! FID=%d, GroupName=%s", GroupName.c_str());
//				return;
//			}
//		}
//	}
//	if(GroupIndex == -1)
//	{
//		sipwarning("Invalid GroupIndex!!");
//		return;
//	}
//
//	// Save
//	wcscpy(m_ContactGroupList.Datas[GroupIndex].GroupName, GroupName.c_str());
//
//	SaveContactGroupList();
//
//	sipinfo(L"user rename contact group name. FID=%d, GroupID=%d, GroupName=%s", m_FID, GroupID, GroupName.c_str());
//}

// Set contact group index([b:Count][ [d:GroupId] ])
void cb_M_CS_SETCONTGROUP_INDEX(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	uint8			count;		_msgin.serial(count);
	T_ID			GroupIDs[MAX_CONTGRPNUM + 1];
	for(uint8 i = 0; i < count; i ++)
		_msgin.serial(GroupIDs[i]);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find failed. FID=%d", FID);
		return;
	}

	_FamilyFriendListInfo	&friendData = it->second;

	if(count != friendData.m_FriendGroups.DataCount)
	{
		// Hacker!!!
		sipwarning("GroupCount Error! RealCount=%d, InputCount=%dm FID=%d", friendData.m_FriendGroups.DataCount, count, FID);
		return;
	}

	// Change Album Index
	_FriendGroupInfo	data;
	uint32		i, j;
	for(i = 0; i < friendData.m_FriendGroups.DataCount; i ++)
	{
		for(j = i; j < friendData.m_FriendGroups.DataCount; j ++)
		{
			if(friendData.m_FriendGroups.Datas[j].GroupID == GroupIDs[i])
				break;
		}
		if(j >= friendData.m_FriendGroups.DataCount)
		{
			// Hacker!!!
			sipwarning("GroupIDList Error! BadGroupID=%d, FID=%d", GroupIDs[i], FID);
			return;
		}
		if(i != j)
		{
			data = friendData.m_FriendGroups.Datas[j];
			friendData.m_FriendGroups.Datas[j] = friendData.m_FriendGroups.Datas[i];
			friendData.m_FriendGroups.Datas[i] = data;
		}
	}

	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
}

int	nFreeAutoPrayerCount = 0;
//void cb_M_CS_SETCONTGROUP_INDEX(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	uint8			count;		_msgin.serial(count);
//	T_ID			GroupIDs[MAX_CONTGRPNUM + 1];
//	for(uint8 i = 0; i < count; i ++)
//		_msgin.serial(GroupIDs[i]);
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->ChangeContactGroupIndex(count, GroupIDs);
//}

//void Player::ChangeContactGroupIndex(uint8 count, uint32 *GroupIDs)
//{
//	if(count != m_ContactGroupList.DataCount)
//	{
//		// Hacker!!!
//		sipwarning("GroupCount Error! RealCount=%d, InputCount=%d", m_ContactGroupList.DataCount, count);
//		return;
//	}
//
//	// Change Album Index
//	_FriendGroupInfo	data;
//	uint32		i, j;
//	for(i = 0; i < m_ContactGroupList.DataCount; i ++)
//	{
//		for(j = i; j < m_ContactGroupList.DataCount; j ++)
//		{
//			if(m_ContactGroupList.Datas[j].GroupID == GroupIDs[i])
//				break;
//		}
//		if(j >= m_ContactGroupList.DataCount)
//		{
//			// Hacker!!!
//			sipwarning("GroupIDList Error! BadGroupID=%d", GroupIDs[i]);
//			return;
//		}
//		if(i != j)
//		{
//			data = m_ContactGroupList.Datas[j];
//			m_ContactGroupList.Datas[j] = m_ContactGroupList.Datas[i];
//			m_ContactGroupList.Datas[i] = data;
//		}
//	}
//
//	// Save
//	SaveContactGroupList();
//
//	sipinfo("ContactGroupIndex Changed.");
//}

//static void	DBCB_DBAddContactChit(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID	idFamily		= *(T_FAMILYID *)argv[0];
//	T_FAMILYID	contactFID	= *(T_FAMILYID *)argv[1];
//	uint32		p1	= *(uint32 *)argv[2];
//
//	if (!nQueryResult)
//		return;
//
//	uint32	nRows;			resSet->serial(nRows);
//	if (nRows < 1)
//		return;
//	uint32 ret;				resSet->serial(ret);
//	if (ret != 0)
//		return;
//	uint32	chitid;			resSet->serial(chitid);
//
//	Player	*pPlayer = GetPlayer(idFamily);
//	Player	*pTargetPlayer = GetPlayer(contactFID);
//	if ((pPlayer != NULL) && (pTargetPlayer != NULL))
//	{
//		uint16		r_sid = SIPNET::IService::getInstance()->getServiceId().get();
//		uint32		nChitCount = 1;
//		uint8		idChatType = CHITTYPE_CONTACT;
//		uint32		p2 = 0;
//		ucstring	p3, p4;
//
//		CMessage	msgOut(M_SC_CHITLIST);
//		msgOut.serial(contactFID, r_sid);
//		msgOut.serial(nChitCount, chitid, idFamily, pPlayer->m_FName, idChatType, p1, p2, p3, p4);
//
//		CUnifiedNetwork::getInstance()->send(pTargetPlayer->m_FSSid, msgOut);
//	}
//}

void AddNewContact(T_FAMILYID FID, T_FAMILYID friendFID, uint32 groupID, uint32 friendIndex, uint32 mainFriendModelID)
{
	if (FID == friendFID)
	{
		sipwarning("FID = friendFID. FID=%d", FID);
		return;
	}

	Player	*pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d, friendFID=%d", FID, friendFID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find failed. FID=%d", FID);
		return;
	}
	_FamilyFriendListInfo	&friendData = it->second;

	// 검사
	if (friendData.m_Friends.find(friendFID) != friendData.m_Friends.end())
	{
		sipwarning("friendList.find(friendFID) != friendList.end(). FID=%d, friendFID=%d", FID, friendFID);
		return;
	}
	uint32 i = 0;
	for(i = 0; i < friendData.m_FriendGroups.DataCount; i ++)
	{
		if(friendData.m_FriendGroups.Datas[i].GroupID == groupID)
			break;
	}
	if (i >= friendData.m_FriendGroups.DataCount)
	{
		sipwarning("i >= friendData.m_FriendGroups.DataCount. FID=%d, friendFID=%d, groupID=%d", FID, friendFID, groupID);
		return;
	}

	// 메모리에서 처리
	{
		_FriendInfo	data1;
		data1.GroupID = groupID;
		data1.Index = friendIndex;
		friendData.m_Friends.insert(make_pair(friendFID, data1));
	}

	// ModelID 얻기 (FID, friendFID)
	T_MODELID	ModelID = 0, friendModelID = 0;
	{
		MAKEQUERY_GetFamilyModelID(strQuery, FID, friendFID);
		CDBConnectionRest	DB(DBCaller);
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
			COLUMN_DIGIT(row,	0,	uint32,		model_id);
			COLUMN_DIGIT(row,	1,	uint32,		friend_model_id);
			ModelID = model_id;
			friendModelID = friend_model_id;
		}
	}

	// 자료통지
	if(groupID != (uint32)(-1))
	{
		uint32		p1 = ModelID, p2 = 0;
		ucstring	p3 = "", p4 = "";
		Player	*pTargetPlayer = GetPlayer(friendFID);
		if (pTargetPlayer != NULL)
		{	// 상대가 로그인되여있는 경우
			if (IsInFriendList(friendFID, FID))
			{
				// 상대가 나를 친구로 가지고있는 경우
				{
					uint16		r_sid = SIPNET::IService::getInstance()->getServiceId().get();
					CMessage	msgOut(M_SC_CHITLIST);
					msgOut.serial(friendFID, r_sid);
					uint32		nChitCount = 1;						msgOut.serial(nChitCount);
					uint32		chitID = 0;							msgOut.serial(chitID);
					msgOut.serial(FID);
					msgOut.serial(pPlayer->m_FName);
					uint8		idChatType = CHITTYPE_CONTACT;		msgOut.serial(idChatType);
					msgOut.serial(p1, p2, p3, p4);

					CUnifiedNetwork::getInstance()->send(pTargetPlayer->m_FSSid, msgOut);
				}

				// 온라인상태를 서로에게 보낸다.
				uint32	zero = 0;
				uint32	sessionState;

				{
					uint8	state = CONTACTSTATE_ONLINE | (pPlayer->m_bIsMobile << 4);
					sessionState = pPlayer->m_SessionState;
					CMessage	msgOut(M_SC_CONTSTATE);
					msgOut.serial(friendFID, zero);
					msgOut.serial(FID, state, sessionState);

					CUnifiedNetwork::getInstance()->send(pTargetPlayer->m_FSSid, msgOut);
				}

				{
					uint8	state = CONTACTSTATE_ONLINE | (pTargetPlayer->m_bIsMobile << 4);
					sessionState = pTargetPlayer->m_SessionState;
					CMessage	msgOut(M_SC_CONTSTATE);
					msgOut.serial(FID, zero);
					msgOut.serial(friendFID, state, sessionState);

					CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
				}
			}
			else
			{
				//MAKEQUERY_ADDCHIT(strQuery, FID, friendFID, CHITTYPE_CONTACT, p1, p2, p3, p4);
				//DBCaller->executeWithParam(strQuery, DBCB_DBAddContactChit,
				//	"DDD",
				//	CB_,			FID,
				//	CB_,			friendFID,
				//	CB_,			p1);
				uint8	ChitAddType = CHITADDTYPE_ADD_AND_SEND;
				uint8	ChitType = CHITTYPE_CONTACT;
				CMessage	msgOut(M_NT_ADDCHIT);
				msgOut.serial(ChitAddType, FID, pPlayer->m_FName, friendFID, ChitType, p1, p2, p3, p4);
				CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
			}
		}
		else
		{
			//MAKEQUERY_ADDCHIT(strQuery, FID, friendFID, CHITTYPE_CONTACT, p1, p2, p3, p4);
			//DBCaller->execute(strQuery);
			uint8	ChitAddType = CHITADDTYPE_ADD_AND_SEND;
			uint8	ChitType = CHITTYPE_CONTACT;
			CMessage	msgOut(M_NT_ADDCHIT);
			msgOut.serial(ChitAddType, FID, pPlayer->m_FName, friendFID, ChitType, p1, p2, p3, p4);
			CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
		}
	}

	if (mainFriendModelID != 0)
		friendModelID = mainFriendModelID;
	if (friendModelID == 0)
	{
		sipwarning("friendModelID = 0");
		return;
	}

	{
		CMessage	msgOut(M_SC_FAMILY_MODEL);
		msgOut.serial(FID, friendFID, friendModelID);
		CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
	}

	DBLOG_AddContact(FID, friendFID);
}

void AddNewFriendContinue(T_FAMILYID FID, T_FAMILYID friendFID, uint32 GroupID, uint32 FriendIndex, uint32 toUserID, uint32 mainFriendModelID)
{
	Player	*pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d, friendFID=%d", FID, friendFID);
		return;
	}

	uint8	firstUser = UNKOWN_USER, secondUser = UNKOWN_USER;

	if (IsSystemAccount(pPlayer->m_UserID))
		firstUser = SYSTEM_USER;
	else
		firstUser = COMMON_USER;

	if (IsSystemAccount(toUserID))
		secondUser = SYSTEM_USER;
	else
		secondUser = COMMON_USER;

	if (firstUser != secondUser)
	{
		sipwarning("Can't add friend. first: FID=%d, type=%d, second: FID=%d, type=%d", FID, firstUser, friendFID, secondUser);
		return;
	}

	AddNewContact(FID, friendFID, GroupID, FriendIndex, mainFriendModelID);

	{
		CMessage	msgOut(M_CS_ADDCONT);
		msgOut.serial(FID, friendFID, GroupID, FriendIndex);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}
}

// Packet: M_CS_ADDCONT
// Add contact([d:FamilyId][d:GroupId][d:NewIndex])
void cb_M_CS_ADDCONT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;			_msgin.serial(FID);
	T_FAMILYID	friendFID;		_msgin.serial(friendFID);
	T_ID		GroupID;		_msgin.serial(GroupID);
	uint32		FriendIndex;	_msgin.serial(FriendIndex);

	T_ACCOUNTID		toUserID = NOVALUE;
	Player	*pContact = GetPlayer(friendFID);
	if (pContact)
		toUserID = pContact->m_UserID;
	else
	{
		MAKEQUERY_LOADFAMILYFORBASE(strQ, friendFID);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY(DB, Stmt, strQ);
		if (nQueryResult == true)
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (IS_DB_ROW_VALID(row))
			{
				COLUMN_DIGIT(row, 9, T_ACCOUNTID, userid);
				toUserID = userid;
			}
		}
	}
	if (toUserID == NOVALUE)
	{
		// secondUser 가 알려지지 않을 때(다른 지역 사용자일 때) 처리 필요
		CMessage	msgOut(M_NT_REQ_FAMILY_INFO);
		msgOut.serial(friendFID, FID, GroupID, FriendIndex);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
		return;
	}

	AddNewFriendContinue(FID, friendFID, GroupID, FriendIndex, toUserID, 0);
}

// Response of M_NT_REQ_FAMILY_INFO ([d:friendFID][d:FID][d:GroupID][d:FriendIndex][d:friendUserID][d:friendModelID])
void cb_M_NT_ANS_FAMILY_INFO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	friendFID;		_msgin.serial(friendFID);
	T_FAMILYID	FID;			_msgin.serial(FID);
	T_ID		GroupID;		_msgin.serial(GroupID);
	uint32		FriendIndex;	_msgin.serial(FriendIndex);
	T_ID		friendUserID;	_msgin.serial(friendUserID);
	T_MODELID	friendModelID;	_msgin.serial(friendModelID);

	AddNewFriendContinue(FID, friendFID, GroupID, FriendIndex, friendUserID, friendModelID);
}

//void cb_M_CS_ADDCONT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	idFamily;		_msgin.serial(idFamily);
//	T_FAMILYID  fidContact;		_msgin.serial(fidContact);
//	T_ID		idGroup;		_msgin.serial(idGroup);
//	uint32      FriendIndex;	_msgin.serial(FriendIndex);
//
//	Player	*pPlayer = GetPlayer(idFamily);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", idFamily);
//		return;
//	}
//
//	enum USERCODE { UNKOWN_USER, SYSTEM_USER, COMMON_USER };
//	USERCODE	firstUser = UNKOWN_USER, secondUser = UNKOWN_USER;
//
//	if (IsSystemAccount(pPlayer->m_UserID))
//		firstUser = SYSTEM_USER;
//	else
//		firstUser = COMMON_USER;
//
//	Player	*pContact = GetPlayer(fidContact);
//	if (pContact)
//	{
//		if (IsSystemAccount(pContact->m_UserID))
//			secondUser = SYSTEM_USER;
//		else
//			secondUser = COMMON_USER;
//	}
//	else
//		secondUser = UNKOWN_USER;
//
//	if (firstUser == COMMON_USER && secondUser == UNKOWN_USER)
//	{
//		{
//			T_ACCOUNTID		toUserID = NOVALUE;
//			MAKEQUERY_LOADFAMILYFORBASE(strQ, fidContact);
//			CDBConnectionRest	DB(DBCaller);
//			DB_EXE_QUERY(DB, Stmt, strQ);
//			if (nQueryResult == true)
//			{
//				DB_QUERY_RESULT_FETCH(Stmt, row);
//				if (IS_DB_ROW_VALID(row))
//				{
//					COLUMN_DIGIT(row, 9, T_ACCOUNTID, userid);
//					toUserID = userid;
//				}
//			}
//			if (toUserID == NOVALUE)
//				secondUser = UNKOWN_USER;
//			else
//			{
//				if (IsSystemAccount(toUserID))
//					secondUser = SYSTEM_USER;
//				else
//					secondUser = COMMON_USER;
//			}
//		}
//	}
//
//	if (firstUser == secondUser)
//	{
//		pPlayer->AddNewContact(fidContact, idGroup, FriendIndex);
//	}
//	else
//	{
//		return;
//	}
//}

//void Player::AddNewContact(T_FAMILYID contactFID, uint32 groupID, uint32 FriendIndex)
//{
//	if (m_FID == contactFID)
//	{
//		sipwarning("m_FID == contactFID. m_FID=%d", m_FID);
//		return;
//	}
//
//	// 검사
//	if (m_FriendFIDs.find(contactFID) != m_FriendFIDs.end())
//	{
//		sipwarning("m_FriendFIDs.find(contactFID) != m_FriendFIDs.end(). FID=%d, contFID=%d", m_FID, contactFID);
//		return;
//	}
//	if (m_BlackFIDs.find(contactFID) != m_BlackFIDs.end())
//	{
//		sipwarning("m_BlackFIDs.find(contactFID) != m_BlackFIDs.end(). FID=%d, contFID=%d", m_FID, contactFID);
//		return;
//	}
//	uint32 i = 0;
//	for(i = 0; i < m_ContactGroupList.DataCount; i ++)
//	{
//		if(m_ContactGroupList.Datas[i].GroupID == groupID)
//			break;
//	}
//	if (i >= m_ContactGroupList.DataCount)
//	{
//		sipwarning("i >= m_ContactGroupList.DataCount. FID=%d, contFID=%d, groupID=%d", m_FID, contactFID, groupID);
//		return;
//	}
//
//	// DB에 보관
//	{
//		sint32	nGroupID = (sint32)groupID;
//		MAKEQUERY_InsertFriend(strQ, m_FID, contactFID, nGroupID, FriendIndex);
//		DBCaller->executeWithParam(strQ, NULL, 
//			"D",
//			OUT_,		NULL);
//	}
//
//	// 메모리에서 처리
//	if(groupID != (uint32)(-1))
//		m_FriendFIDs[contactFID] = contactFID;
//	else
//		m_BlackFIDs[contactFID] = contactFID;
//
//	// 자료통지
//	if(groupID != (uint32)(-1))
//	{
//		Player	*pTargetPlayer = GetPlayer(contactFID);
//		if ((pTargetPlayer != NULL) && (pTargetPlayer->m_bIsOnline))
//		{
//			if (pTargetPlayer->IsFriendList(m_FID))
//			{
//				// 상대가 나를 친구로 가지고있는 경우
//				{
//					uint16		r_sid = SIPNET::IService::getInstance()->getServiceId().get();
//					CMessage	msgOut(M_SC_CHITLIST);
//					msgOut.serial(contactFID, r_sid);
//					uint32		nChitCount = 1;						msgOut.serial(nChitCount);
//					uint32		chitID = 0;							msgOut.serial(chitID);
//					msgOut.serial(m_FID);
//					msgOut.serial(m_FName);
//					uint8		idChatType = CHITTYPE_CONTACT;		msgOut.serial(idChatType);
//					uint32		p1 = 0, p2 = 0;
//					ucstring	p3, p4;								msgOut.serial(p1, p2, p3, p4);
//
//					CUnifiedNetwork::getInstance()->send(pTargetPlayer->m_FSSid, msgOut);
//				}
//
//				// 온라인상태를 서로에게 보낸다.
//				uint32	zero = 0;
//				uint32	sessionState;
//
//				{
//					uint8	state = CONTACTSTATE_ONLINE | (m_bIsMobile << 4);
//					sessionState = m_SessionState;
//					CMessage	msgOut(M_SC_CONTSTATE);
//					msgOut.serial(contactFID, zero);
//					msgOut.serial(m_FID, state, sessionState);
//
//					CUnifiedNetwork::getInstance()->send(pTargetPlayer->m_FSSid, msgOut);
//				}
//
//				{
//					uint8	state = CONTACTSTATE_ONLINE | (pTargetPlayer->m_bIsMobile << 4);
//					sessionState = pTargetPlayer->m_SessionState;
//					CMessage	msgOut(M_SC_CONTSTATE);
//					msgOut.serial(m_FID, zero);
//					msgOut.serial(contactFID, state, sessionState);
//
//					CUnifiedNetwork::getInstance()->send(m_FSSid, msgOut);
//				}
//			}
//			else
//			{
//				ucstring	p3 = "", p4 = "";
//				MAKEQUERY_ADDCHIT(strQuery, m_FID, contactFID, CHITTYPE_CONTACT, 0, 0, p3, p4);
//				DBCaller->executeWithParam(strQuery, DBCB_DBAddContactChit,
//					"DD",
//					CB_,			m_FID,
//					CB_,			contactFID);
//			}
//		}
//		else
//		{
//			ucstring	p3 = "", p4 = "";
//			MAKEQUERY_ADDCHIT(strQuery, m_FID, contactFID, CHITTYPE_CONTACT, 0, 0, p3, p4);
//			DBCaller->execute(strQuery);
//		}
//	}
//
//	DBLOG_AddContact(m_FID, contactFID);
//}

void DeleteRoomManagerInRoomWorldByRemoveFriendList(T_FAMILYID FID, T_FAMILYID ManagerFID, T_ROOMID RoomID, T_ROOMNAME RoomName)
{
	// Notice to Player
	ucstring p4 = L"";
	SendChit(FID, ManagerFID, CHITTYPE_MANAGER_DEL, RoomID, ROOMMANAGER_DEL_REASON_REMOVEFRIENDLIST, RoomName, p4);

	// Notice to OpenRoomService
	CMessage	msgOut(M_NT_MANAGER_DEL);
	msgOut.serial(RoomID, ManagerFID);
	SendMsgToRoomID(RoomID, msgOut);
}

//static void	DBCB_DBDeleteFriend(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	if (!nQueryResult)
//		return;
//
//	uint32			ret = *(uint32 *)argv[0];
//	T_FAMILYID		FID	= *(T_FAMILYID *)argv[1];
//	T_FAMILYID		FriendFID	= *(T_FAMILYID *)argv[2];
//
//	if (ret == 0)
//	{
//		uint32 nRows;	resSet->serial(nRows);
//		ucstring	p4 = L"";
//		for (uint32 i = 0; i < nRows; i ++)
//		{
//			uint32		RoomID;
//			ucstring	RoomName;
//			resSet->serial(RoomID);
//			resSet->serial(RoomName);
//			DeleteRoomManagerInRoomWorldByRemoveFriendList(FID, FriendFID, RoomID, RoomName);
//		}
//
//		sipinfo(" : user:%u delete user:%u", FID, FriendFID);
//		DBLOG_DelContact(FID, FriendFID);
//	}
//	else
//	{
//		sipwarning("exec Shrd_Friend_delete failed");
//	}		
//}

// Packet: M_CS_DELCONT
// Delete contact([d:ContactFamilyId])
void cb_M_CS_DELCONT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_FAMILYID		friendFID;	_msgin.serial(friendFID);

	Player	*pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find failed. FID=%d", FID);
		return;
	}
	_FamilyFriendListInfo	&friendData = it->second;

	if (FID == friendFID)
	{
		nFreeAutoPrayerCount = FID;
		return;
	}

	std::map<T_FAMILYID, _FriendInfo>::iterator	it1 = friendData.m_Friends.find(friendFID);
	if (it1 == friendData.m_Friends.end())
	{
		sipwarning("friendData.m_Friends.find failed. FID=%d, friendFID=%d", FID, friendFID);
		return;
	}

	// 상대에게 로그오프되였다고 통지한다.
	pPlayer->CheckAndNotifyOnOffStatusToFriend(friendFID, CONTACTSTATE_OFFLINE);

	// 메모리에서 삭제
	friendData.m_Friends.erase(it1);

	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
}

//void cb_M_CS_DELCONT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	T_FAMILYID		ContactFID;	_msgin.serial(ContactFID);
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	boolean	bFind = false;
//	std::map<T_FAMILYID, T_FAMILYID>::iterator	it;
//
//	it = pPlayer->m_FriendFIDs.find(ContactFID);
//	if (it != pPlayer->m_FriendFIDs.end())
//	{
//		pPlayer->m_FriendFIDs.erase(it);
//		bFind = true;
//	}
//
//	if (!bFind)
//	{
//		it = pPlayer->m_BlackFIDs.find(ContactFID);
//		if (it != pPlayer->m_BlackFIDs.end())
//		{
//			pPlayer->m_BlackFIDs.erase(it);
//			bFind = true;
//		}
//	}
//
//	if (!bFind)
//	{
//		sipwarning("bFind = false. FID=%d, ContactFID=%d", FID, ContactFID);
//		return;
//	}
//
//	// 상대에게 로그오프되였다고 통지한다.
//	CheckAndNotifyOnOffStatusToFriend(FID, ContactFID, CONTACTSTATE_OFFLINE);
//
//	MAKEQUERY_DeleteFriend(strQuery, FID, ContactFID);
//	DBCaller->executeWithParam(strQuery, DBCB_DBDeleteFriend,
//		"DDD",
//		OUT_,	NULL,
//		CB_,	FID,
//		CB_,	ContactFID);
//}

//static void	DBCB_DBChangeFriendIndex(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	uint32	ret			= *(uint32 *)argv[0];
//	uint32	FID			= *(uint32 *)argv[1];
//	uint32	FriendFID	= *(uint32 *)argv[2];
//	T_ID	nNewGroupID	= *(int *)argv[3];
//
//	if (ret != 0 || nNewGroupID != -1)
//		return;
//
//	// 친구목록에서 흑명단으로 옮겨진 경우 방관리자권한을 삭제한다.
//	uint32 nRows;	resSet->serial(nRows);
//	ucstring	p4 = L"";
//	for (uint32 i = 0; i < nRows; i ++)
//	{
//		uint32		RoomID;
//		ucstring	RoomName;
//		resSet->serial(RoomID);
//		resSet->serial(RoomName);
//		DeleteRoomManagerInRoomWorldByRemoveFriendList(FID, FriendFID, RoomID, RoomName);
//	}
//}

// Change contact's group [d:Count][ [d:ContactId][d:NewGroupId][d:NewIndex] ]
void cb_M_CS_CHANGECONTGRP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;	 _msgin.serial(FID);
	uint32		count;	 _msgin.serial(count);

	if (_msgin.length() - _msgin.getHeaderSize() != count * 12 + 8)
	{
		sipwarning("Invalid count or length. FID=%d, count=%d, remainlen=%d", FID, count, _msgin.length() - _msgin.getHeaderSize());
		return;
	}

	Player	*pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find failed. FID=%d", FID);
		return;
	}
	_FamilyFriendListInfo	&friendData = it->second;

	uint32		friendFID, NewGroupID, NewIndex;
	for (uint32 i = 0; i < count; i ++)
	{
		_msgin.serial(friendFID);
		_msgin.serial(NewGroupID);
		_msgin.serial(NewIndex);

		std::map<T_FAMILYID, _FriendInfo>::iterator	it1 = friendData.m_Friends.find(friendFID);
		if (it1 == friendData.m_Friends.end())
		{
			sipwarning("friendData.m_Friends.find failed. FID=%d, friendFID=%d", FID, friendFID);
			return;
		}

		if (it1->second.GroupID != (uint32)(-1))
		{
			if (NewGroupID == (uint32)(-1))
			{
				// 상대에게 로그오프되였다고 통지한다.
				pPlayer->CheckAndNotifyOnOffStatusToFriend(friendFID, CONTACTSTATE_OFFLINE);
			}
		}
		else
		{
			if (NewGroupID != (uint32)(-1))
			{
				// 상대에게 로그온되였다고 통지한다.
				pPlayer->CheckAndNotifyOnOffStatusToFriend(friendFID, CONTACTSTATE_ONLINE);
			}
		}

		it1->second.GroupID = NewGroupID;
	}

	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
}

//void cb_M_CS_CHANGECONTGRP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;	 _msgin.serial(FID);
//	uint32		count;	 _msgin.serial(count);
//
//	if (_msgin.length() - _msgin.getHeaderSize() != count * 12 + 8)
//	{
//		sipwarning("Invalid count or length. FID=%d", FID);
//		return;
//	}
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	uint32		ContactFID, NewGroupID, NewIndex;
//	for (uint32 i = 0; i < count; i ++)
//	{
//		_msgin.serial(ContactFID);
//		_msgin.serial(NewGroupID);
//		_msgin.serial(NewIndex);
//		int		nNewGroupID = (int)NewGroupID;
//
//		boolean	bFind = false;
//		std::map<T_FAMILYID, T_FAMILYID>::iterator	it;
//
//		it = pPlayer->m_FriendFIDs.find(ContactFID);
//		if (it != pPlayer->m_FriendFIDs.end())
//		{
//			if (nNewGroupID == -1)
//			{
//				// 친구그룹->차단그룹
//				pPlayer->m_FriendFIDs.erase(it);
//				pPlayer->m_BlackFIDs[ContactFID] = ContactFID;
//
//				// 상대에게 로그오프되였다고 통지한다.
//				CheckAndNotifyOnOffStatusToFriend(FID, ContactFID, CONTACTSTATE_OFFLINE);
//			}
//			bFind = true;
//		}
//
//		if (!bFind)
//		{
//			it = pPlayer->m_BlackFIDs.find(ContactFID);
//			if (it != pPlayer->m_BlackFIDs.end())
//			{
//				if (nNewGroupID >= 0)
//				{
//					// 차단그룹->친구그룹
//					pPlayer->m_BlackFIDs.erase(it);
//					pPlayer->m_FriendFIDs[ContactFID] = ContactFID;
//
//					// 상대에게 로그온되였다고 통지한다.
//					CheckAndNotifyOnOffStatusToFriend(FID, ContactFID, CONTACTSTATE_ONLINE);
//				}
//				bFind = true;
//			}
//		}
//
//		if (!bFind)
//		{
//			sipwarning("bFind = false. FID=%d, ContactFID=%d", FID, ContactFID);
//			return;
//		}
//
//		MAKEQUERY_ChangeFriendIndex(strQ, FID, ContactFID, nNewGroupID, NewIndex);
//		DBCaller->executeWithParam(strQ, DBCB_DBChangeFriendIndex,
//			"DDDD",
//			OUT_,	NULL,
//			CB_,	FID,
//			CB_,	ContactFID,
//			CB_,	nNewGroupID);
//	}
//}

// ======================================================
// Favorite Rooms
// ======================================================
//void Player::SaveFavorroomGroup()
//{
//	uint8	bFavorroomGroupList[MAX_FAVORROOMGRP_BUF_SIZE];
//	uint32	GroupBufferSize = sizeof(bFavorroomGroupList);
//
//	if (!m_FavorroomGroupList.toBuffer(bFavorroomGroupList, &GroupBufferSize))
//	{
//		sipwarning("pFavorroomGroupList->toBuffer failed.");
//		return;
//	}
//
//	char str_buf[MAX_FAVORROOMGRP_BUF_SIZE * 2 + 10];
//	for (uint32 i = 0; i < GroupBufferSize; i ++)
//	{
//		sprintf(&str_buf[i * 2], ("%02x"), bFavorroomGroupList[i]);
//	}
//	str_buf[GroupBufferSize * 2] = 0;
//
//	MAKEQUERY_SaveFavorroomGroupList(strQ, m_FID, str_buf);
//
//	DBCaller->executeWithParam(strQ, NULL,
//		"D",
//		OUT_,		NULL);
//}

//static void	DBCB_DBLoadRoomOrderInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];
//
//	if (!nQueryResult)
//		return;
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->DBLoadRoomOrderInfo(resSet);
//}

//static void SendRoomOrderInfo(T_FAMILYID FID)
//{
//	MAKEQUERY_LOADROOMORDERINFO(strQ, FID);
//	DBCaller->executeWithParam(strQ, DBCB_DBLoadRoomOrderInfo,
//		"D",
//		CB_,		FID);
//}

//void Player::ReadRoomOrderInfo()
//{
//	MAKEQUERY_LOADROOMORDERINFO(strQ, m_FID);
//	DBCaller->executeWithParam(strQ, DBCB_DBLoadRoomOrderInfo,
//		"D",
//		CB_,		m_FID);
//}

void Player::DBLoadRoomOrderInfo(SIPBASE::CMemStream * resSet)
{
	uint32 nRows;	resSet->serial(nRows);
	if (nRows == 1)
	{
		// read ownroom
		uint8	bBuffer0[MAX_OWNROOMCOUNT*8+10];	memset(bBuffer0, 0, sizeof(bBuffer0));
		uint32	lBufSize0;
		resSet->serial(lBufSize0);
		if (lBufSize0 > sizeof(bBuffer0))
		{
			sipwarning("OwnRoomOrder full buffer failed. FID=%d, size=%d", m_FID, lBufSize0);
			return;
		}
		resSet->serialBuffer(bBuffer0, lBufSize0);
		if (!m_OwnRoomOrderInfoList.fromBuffer(bBuffer0, lBufSize0))
		{
			sipwarning("OwnRoomOrder.fromBuffer failed. FID=%d", m_FID);
			return;
		}
		// read manage room
		uint8	bBuffer1[MAX_FAVORROOMNUM*8+10];	memset(bBuffer1, 0, sizeof(bBuffer1));
		uint32	lBufSize1;
		resSet->serial(lBufSize1);
		if (lBufSize1 > sizeof(bBuffer1))
		{
			sipwarning("ManageRoomOrder full buffer failed. FID=%d, size=%d", m_FID, lBufSize1);
			return;
		}
		resSet->serialBuffer(bBuffer1, lBufSize1);
		if (!m_ManageRoomOrderInfoList.fromBuffer(bBuffer1, lBufSize1))
		{
			sipwarning("ManagerRoomOrder.fromBuffer failed. FID=%d", m_FID);
			return;
		}
		// read favo room
		uint8	bBuffer2[(MAX_FAVORROOMNUM*8+10)*MAX_FAVORROOMGROUP];	memset(bBuffer2, 0, sizeof(bBuffer2));
		uint32	lBufSize2;
		resSet->serial(lBufSize2);
		if (lBufSize2 > sizeof(bBuffer2))
		{
			sipwarning("FavoRoomOrder full buffer failed. FID=%d, size=%d", m_FID, lBufSize2);
			return;
		}
		resSet->serialBuffer(bBuffer2, lBufSize2);
		if (!m_FavRoomOrderInfoList.fromBuffer(bBuffer2, lBufSize2))
		{
			sipwarning("FavoRoomOrder.fromBuffer failed. FID=%d", m_FID);
			return;
		}
	}
	else
	{
		MAKEQUERY_MAKEEMPTYROOMORDERINFO(strQ, m_FID);
		DBCaller->execute(strQ);
	}

	{
		CMessage	msgOut(M_SC_ROOMORDERINFO);
		uint32		groupcount = 1;
		T_ID		groupid = GROUP_ID_MYROOMS;
		uint32		count = m_OwnRoomOrderInfoList.DataCount;
		msgOut.serial(m_FID, groupcount, groupid, count);
		for (uint32 i = 0; i < m_OwnRoomOrderInfoList.DataCount; i ++)
		{
			T_ROOMID roomid = m_OwnRoomOrderInfoList.Datas[i].RoomID;
			msgOut.serial(roomid);
		}
		CUnifiedNetwork::getInstance()->send(m_FSSid, msgOut);
	}

	{
		CMessage	msgOut(M_SC_ROOMORDERINFO);
		uint32		groupcount = 1;
		T_ID		groupid = GROUP_ID_MYMANAGEROOMS;
		uint32		count = m_ManageRoomOrderInfoList.DataCount;
		msgOut.serial(m_FID, groupcount, groupid, count);
		for (uint32 i = 0; i < m_ManageRoomOrderInfoList.DataCount; i ++)
		{
			T_ROOMID roomid = m_ManageRoomOrderInfoList.Datas[i].RoomID;
			msgOut.serial(roomid);
		}
		CUnifiedNetwork::getInstance()->send(m_FSSid, msgOut);
	}

	{
		CMessage	msgOut(M_SC_ROOMORDERINFO);
		uint32		count = m_FavRoomOrderInfoList.DataCount;
		msgOut.serial(m_FID, count);
		for (uint32 i = 0; i < m_FavRoomOrderInfoList.DataCount; i ++)
		{
			uint32 groupid = m_FavRoomOrderInfoList.Datas[i].GroupID;
			msgOut.serial(groupid);
			RoomOrderInfoList& roomlist = m_FavRoomOrderInfoList.Datas[i].RoomOrderInfo;
			msgOut.serial(roomlist.DataCount);
			for (uint32 j = 0; j < roomlist.DataCount; j++)
			{
				msgOut.serial(roomlist.Datas[j].RoomID);
			}
		}
		CUnifiedNetwork::getInstance()->send(m_FSSid, msgOut);
	}
}

void Player::SaveRoomOrderInfoToDB()
{
	if (!m_bChangedRoomOrderInfo)
		return;
	{

		uint8	bBuffer[MAX_OWNROOMCOUNT*8+10];	memset(bBuffer, 0, sizeof(bBuffer));
		uint32	lBufferSize = sizeof(bBuffer);

		if (!m_OwnRoomOrderInfoList.toBuffer(bBuffer, &lBufferSize))
		{
			sipwarning("m_OwnRoomOrderInfoList.toBuffer failed.");
			return;
		}

		char str_buf[sizeof(bBuffer) * 2 + 10];
		for (uint32 i = 0; i < lBufferSize; i ++)
		{
			sprintf(&str_buf[i * 2], ("%02x"), bBuffer[i]);
		}
		str_buf[lBufferSize * 2] = 0;

		MAKEQUERY_SAVEOWNROOMORDERINFO(strQ, m_FID, str_buf);
		DBCaller->execute(strQ);
	}

	{
		uint8	bBuffer[MAX_FAVORROOMNUM*8+10];	memset(bBuffer, 0, sizeof(bBuffer));
		uint32	lBufferSize = sizeof(bBuffer);

		if (!m_ManageRoomOrderInfoList.toBuffer(bBuffer, &lBufferSize))
		{
			sipwarning("m_ManageRoomOrderInfoList.toBuffer failed.");
			return;
		}

		char str_buf[sizeof(bBuffer) * 2 + 10];
		for (uint32 i = 0; i < lBufferSize; i ++)
		{
			sprintf(&str_buf[i * 2], ("%02x"), bBuffer[i]);
		}
		str_buf[lBufferSize * 2] = 0;

		MAKEQUERY_SAVEMANAGEROOMORDERINFO(strQ, m_FID, str_buf);
		DBCaller->execute(strQ);
	}

	{
		uint8	bBuffer[(MAX_FAVORROOMNUM*8+10)*MAX_FAVORROOMGROUP];	memset(bBuffer, 0, sizeof(bBuffer));
		uint32	lBufferSize = sizeof(bBuffer);

		if (!m_FavRoomOrderInfoList.toBuffer(bBuffer, &lBufferSize))
		{
			sipwarning("m_FavRoomOrderInfoList.toBuffer failed.");
			return;
		}

		char str_buf[sizeof(bBuffer) * 2 + 10];
		for (uint32 i = 0; i < lBufferSize; i ++)
		{
			sprintf(&str_buf[i * 2], ("%02x"), bBuffer[i]);
		}
		str_buf[lBufferSize * 2] = 0;

		MAKEQUERY_SAVEFAVOROOMORDERINFO(strQ, m_FID, str_buf);
		DBCaller->execute(strQ);
	}
}
void Player::SaveOwnRoomOrderInfo(RoomOrderInfoList &newinfo)
{
	m_OwnRoomOrderInfoList = newinfo;
	m_bChangedRoomOrderInfo = true;
}

void Player::SaveManageRoomOrderInfo(RoomOrderInfoList &newinfo)
{
	m_ManageRoomOrderInfoList = newinfo;
	m_bChangedRoomOrderInfo = true;
}

void Player::SaveFaveRoomOrderInfo(uint32 GroupID, RoomOrderInfoList &newinfo)
{
	bool bUpdate = false;
	for (uint32 i = 0; i < m_FavRoomOrderInfoList.DataCount; i++)
	{
		if (GroupID == m_FavRoomOrderInfoList.Datas[i].GroupID)
		{
			m_FavRoomOrderInfoList.Datas[i].RoomOrderInfo = newinfo;
			bUpdate = true;	
		}
	}
	if (!bUpdate)
	{
		uint32 newpos = m_FavRoomOrderInfoList.DataCount;
		if (newpos >= MAX_FAVORROOMGROUP)
		{
			sipwarning("m_FavRoomOrderInfoList.DataCount(%d) full", m_FavRoomOrderInfoList.DataCount);
			return;
		}
		m_FavRoomOrderInfoList.Datas[newpos].GroupID = GroupID;
		m_FavRoomOrderInfoList.Datas[newpos].RoomOrderInfo = newinfo;
		m_FavRoomOrderInfoList.DataCount++;
	}

	m_bChangedRoomOrderInfo = true;
}

//void Player::DBLoadFavorroomGroupList(CMemStream * resSet)
//{
//	uint32 nRows;	resSet->serial(nRows);
//	if (nRows == 0)
//	{
//		m_FavorroomGroupList.DataCount = 1;
//		m_FavorroomGroupList.Datas[0].GroupID = 1;
//		wcscpy(m_FavorroomGroupList.Datas[0].GroupName, GetLangText(ID_ROOM_DEFAULTGROUP_NAME).c_str());
//
//		SaveFavorroomGroup();
//	}
//	else
//	{
//		uint8	bFavorroomGroupList[MAX_FAVORROOMGRP_BUF_SIZE];
//		uint32	lGroupBufSize = sizeof(bFavorroomGroupList);
//
//		resSet->serial(lGroupBufSize);
//		resSet->serialBuffer(bFavorroomGroupList, lGroupBufSize);
//		if (!m_FavorroomGroupList.fromBuffer(bFavorroomGroupList, lGroupBufSize))
//		{
//			sipwarning("FavorroomGroup.fromBuffer failed. FID=%d", m_FID);
//			return;
//		}
//	}
//
//	CMessage	msgOut(M_SC_FAVORROOMGROUP);
//	uint8		count = (uint8) m_FavorroomGroupList.DataCount;
//	msgOut.serial(m_FID, count);
//	for(uint32 i = 0; i < m_FavorroomGroupList.DataCount; i ++)
//	{
//		ucstring	name(m_FavorroomGroupList.Datas[i].GroupName);
//		msgOut.serial(m_FavorroomGroupList.Datas[i].GroupID, name);
//	}
//	CUnifiedNetwork::getInstance()->send(m_FSSid, msgOut);
//
//	SendRoomOrderInfo(m_FID);
//}

//static void	DBCB_DBLoadFavorroomGroupList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];
//
//	if (!nQueryResult)
//		return;
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->DBLoadFavorroomGroupList(resSet);
//}

//static void	SendFavorroomGroupList(T_FAMILYID FID, TServiceId _tsid)
//{
//	MAKEQUERY_LoadFavorroomGroupList(strQ, FID);
//	DBCaller->executeWithParam(strQ, DBCB_DBLoadFavorroomGroupList,
//		"D",
//		CB_,		FID);
//}

//struct FavoriteDeletedRooms
//{
//	T_ROOMID	RoomID;
//	T_ID		GroupID;
//	ucstring	RoomName;
//
//	FavoriteDeletedRooms(T_ROOMID _RoomID, uint32 _GroupID, ucstring _RoomName) : RoomID(_RoomID), GroupID(_GroupID), RoomName(_RoomName) {}
//};
//const	int Favorroom_Send_Count = 10;
//static void	DBCB_DBGetFavorroom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID		familyId	= *(T_FAMILYID *)argv[0];
//	TServiceId		tsid(*(uint16 *)argv[1]);
//
//	if (!nQueryResult)
//		return;
//
//	// My Favor Room
//	uint8			count = 0;
//	FavorRoom		Favorrooms[Favorroom_Send_Count];
//	std::vector<FavoriteDeletedRooms> DeletedRooms;
//
//	{
//		uint32	nRows;		resSet->serial(nRows);
//
//		bool	bSended = false;
//		for (uint32 i = 0; i < nRows; i ++)
//		{
//			T_ID		groupid;	resSet->serial(groupid);
//			T_ROOMID	roomid;		resSet->serial(roomid);
//			T_FAMILYID	familyid;	resSet->serial(familyid);
//			T_SCENEID	sceneid;	resSet->serial(sceneid);
//			ucstring	roomname;	resSet->serial(roomname);
//			T_FLAG		openflag;	resSet->serial(openflag);
//			ucstring	mastername;	resSet->serial(mastername);
//			std::string	termtime;	resSet->serial(termtime);
//			uint8		deleteflag;	resSet->serial(deleteflag);
//			std::string	deletetime;	resSet->serial(deletetime);
//			T_ROOMLEVEL	level;		resSet->serial(level);
//			T_ROOMEXP	exp;		resSet->serial(exp);
//			uint32		visitcount;	resSet->serial(visitcount);
//			uint8		deadsurnamelen1;resSet->serial(deadsurnamelen1);
//			ucstring	deadname1;	resSet->serial(deadname1);
//			uint8		deadsurnamelen2;resSet->serial(deadsurnamelen2);
//			ucstring	deadname2;	resSet->serial(deadname2);
//			uint8		potoType;	resSet->serial(potoType);
//			std::string	seldate;	resSet->serial(seldate);
//
//			uint32	nSelTime = getMinutesSince1970(seldate);
//			T_M1970	nTermTime = getMinutesSince1970(termtime);
//			uint32	deleteremainminutes = 0;
//			if (deleteflag)
//			{
//				nTermTime = getMinutesSince1970(deletetime);
//				uint32	nCurtime = (uint32)(GetDBTimeSecond() / 60);
//				if (nCurtime >= nTermTime)
//				{
//					FavoriteDeletedRooms	data(roomid, groupid, roomname);
//					DeletedRooms.push_back(data);
//					continue;
//				}
//				deleteremainminutes = nTermTime - nCurtime;
//			}
//
//			Favorrooms[count].RoomId = roomid;
//			Favorrooms[count].FamilyId = familyid;
//			Favorrooms[count].SceneId = sceneid;
//			Favorrooms[count].GroupId = groupid;
//			Favorrooms[count].RoomName = roomname;
//			Favorrooms[count].nOpenFlag = openflag;
//			Favorrooms[count].MasterName = mastername;
//			Favorrooms[count].ExpireTime = nTermTime;
//			Favorrooms[count].DeleteRemainMinutes = deleteremainminutes;
//			Favorrooms[count].level = level;
//			Favorrooms[count].exp = exp;
//			Favorrooms[count].VisitCount = visitcount;
//			Favorrooms[count].DeadName1 = deadname1;
//			Favorrooms[count].DeadSurnamelen1 = deadsurnamelen1;
//			Favorrooms[count].DeadName2 = deadname2;
//			Favorrooms[count].DeadSurnamelen2 = deadsurnamelen2;
//			Favorrooms[count].PhotoType = potoType;
//			Favorrooms[count].seldate = nSelTime;
//			count ++;
//
//			if (count >= Favorroom_Send_Count)
//			{
//				CMessage	msgout(M_SC_FAVORROOM);
//				msgout.serial(familyId, count);
//				for (uint8 i = 0; i < count; i ++)
//				{
//					FavorRoom&	r = Favorrooms[i];
//					//uint16	exp_percent = CalcRoomExpPercent(r.level, r.exp);
//
//					msgout.serial(r.GroupId, r.SceneId, r.RoomId, r.FamilyId, r.nOpenFlag, r.RoomName, r.MasterName);
//					msgout.serial(r.ExpireTime, r.DeleteRemainMinutes, r.level, r.exp, /*exp_percent,*/ r.VisitCount);
//					msgout.serial(r.PhotoType, r.DeadName1, r.DeadSurnamelen1, r.DeadName2, r.DeadSurnamelen2, r.seldate);
//				}
//				CUnifiedNetwork::getInstance()->send(tsid, msgout);
//				count = 0;
//				bSended = true;
//			}
//		}
//
//		if ((count > 0) || !bSended)
//		{
//			CMessage	msgout(M_SC_FAVORROOM);
//			msgout.serial(familyId, count);
//			for (uint8 i = 0; i < count; i ++)
//			{
//				FavorRoom&	r = Favorrooms[i];
//				//uint16	exp_percent = CalcRoomExpPercent(r.level, r.exp);
//
//				msgout.serial(r.GroupId, r.SceneId, r.RoomId, r.FamilyId, r.nOpenFlag, r.RoomName, r.MasterName);
//				msgout.serial(r.ExpireTime, r.DeleteRemainMinutes, r.level, r.exp, /*exp_percent,*/ r.VisitCount);
//				msgout.serial(r.PhotoType, r.DeadName1, r.DeadSurnamelen1, r.DeadName2, r.DeadSurnamelen2, r.seldate);
//			}
//			CUnifiedNetwork::getInstance()->send(tsid, msgout);
//			count = 0;
//		}
//	}
//
//	// M_SC_FAVORROOM을 보낸 다음에 M_SC_FAVORROOMGROUP을 보내야 한다.
//	// 클라이언트에서는 M_SC_FAVORROOMGROUP를 받으면 FavoriteRoom정보를 다 받은것으로 인정한다.
//	SendFavorroomGroupList(familyId, tsid);
//
//	// 氚╈＜鞚胳澊 靷牅頃?氚╇摛鞚?Favorite氇╇鞐愳劀 靷牅頃滊嫟.
//	if (!DeletedRooms.empty())
//	{
//		CMessage	msgOut(M_SC_FAVORROOM_AUTODEL);
//		msgOut.serial(familyId);
//
//		for (std::vector<FavoriteDeletedRooms>::iterator it = DeletedRooms.begin(); it != DeletedRooms.end(); it ++)
//		{
//			T_ROOMID	RoomID = it->RoomID;
//
//			MAKEQUERY_DeleteFavorroom(strQ, familyId, RoomID, it->GroupID);
//			DBCaller->executeWithParam(strQ, NULL,
//				"D",
//				OUT_,		NULL);
//
//			msgOut.serial(RoomID, it->RoomName);
//		}
//
//		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//	}
//}

//void Player::SendOwnFavoriteRoomInfo()
//{
//	MAKEQUERY_GetFavorroom(strQ, m_FID);
//	DBCaller->executeWithParam(strQ, DBCB_DBGetFavorroom,
//		"DW",
//		CB_,		m_FID,
//		CB_,		m_FSSid.get());
//}

// Add favorite room([d:GroupID][d:Room ID])
//void cb_M_CS_INSFAVORROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;			_msgin.serial(FID);
//	T_ID		groupid;		_msgin.serial(groupid);
//	T_ROOMID	roomid;			_msgin.serial(roomid);
//
//	MAKEQUERY_InsertFavorroom(strQ, FID, roomid, groupid);
//	DBCaller->executeWithParam(strQ, NULL,
//		"D",
//		OUT_,		NULL);
//}

// Delete favorite room([d:GroupID][d:Room ID])
//void	cb_M_CS_DELFAVORROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;			_msgin.serial(FID);
//	T_ID		groupid;		_msgin.serial(groupid);
//	T_ROOMID	roomid;			_msgin.serial(roomid);
//
//	MAKEQUERY_DeleteFavorroom(strQ, FID, roomid, groupid);
//	DBCaller->executeWithParam(strQ, NULL,
//		"D",
//		OUT_,		NULL);
//}

// Move favorite room to other group([d:Room ID][d:OldGroupID][d:NewGroupID])
//void	cb_M_CS_MOVFAVORROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;			_msgin.serial(FID);
//	T_ROOMID	roomid;			_msgin.serial(roomid);
//	T_ID		old_group;		_msgin.serial(old_group);
//	T_ID		new_group;		_msgin.serial(new_group);
//
//	sipdebug("fid=%d, roomid=%d, old=%d, new=%d", FID, roomid, old_group, new_group);
//	MAKEQUERY_ChangeGroupFavorroom(strQ, FID, roomid, old_group, new_group);
//	DBCaller->executeWithParam(strQ, NULL,
//		"D",
//		OUT_,		NULL);
//}

//void Player::AddFavorroomGroup(ucstring &ucGroup, uint32 GroupID)
//{
//	if (m_FavorroomGroupList.DataCount >= MAX_FAVORROOMGROUP)
//	{
//		sipwarning("m_FavorroomGroupList.DataCount overflow!! FID=%d", m_FID);
//		return;
//	}
//
//	// CheckData
//	if ((int)GroupID < MAX_DEFAULT_SYSTEMGROUP_ID)
//	{
//		sipwarning("Invalid GroupID. FID=%d, GroupID=%d", m_FID, GroupID);
//		return;
//	}
//	for (uint32 i = 0; i < m_FavorroomGroupList.DataCount; i ++)
//	{
//		if (m_FavorroomGroupList.Datas[i].GroupName == ucGroup)
//		{
//			sipwarning("Same Name Exist!! FID=%d", m_FID);
//			return;
//		}
//		if (GroupID == m_FavorroomGroupList.Datas[i].GroupID)
//		{
//			sipwarning("Same GroupID Exist. FID=%d, GroupID=%d", m_FID, GroupID);
//			return;
//		}
//	}
//
//	// Insert New Album
//	m_FavorroomGroupList.Datas[m_FavorroomGroupList.DataCount].GroupID = GroupID;
//	wcscpy(m_FavorroomGroupList.Datas[m_FavorroomGroupList.DataCount].GroupName, ucGroup.c_str());
//	m_FavorroomGroupList.DataCount ++;
//
//	// Save
//	SaveFavorroomGroup();
//
//	sipinfo("user:%u create group:%s", m_FID, ucGroup.c_str());
//}

// Request create new favorite room group [u:GroupName][d:GroupID]
//void	cb_M_CS_NEW_FAVORROOMGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);
//	ucstring	ucGroup;	_msgin.serial(ucGroup);	// SAFE_SERIAL_WSTR(_msgin, ucGroup, MAX_FAVORGROUPNAME, FID);
//	if (ucGroup.length() > MAX_FAVORGROUPNAME)
//	{
//		sipwarning("ucGroup.length() > MAX_FAVORGROUPNAME. FID=%d", FID);
//		return;
//	}
//	uint32		GroupID;	_msgin.serial(GroupID);
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->AddFavorroomGroup(ucGroup, GroupID);
//}

//void Player::DelFavorroomGroup(uint32 GroupID)
//{
//	// GroupID=1,-1 : Default Group
//	if (GroupID < MAX_DEFAULT_SYSTEMGROUP_ID)
//	{
//		sipwarning("Invalid GroupID!! FID=%d, GroupID=%d", m_FID, GroupID);
//		return;
//	}
//
//	int	GroupIndex;
//	for (GroupIndex = 0; GroupIndex < (int)m_FavorroomGroupList.DataCount; GroupIndex ++)
//	{
//		if(m_FavorroomGroupList.Datas[GroupIndex].GroupID == GroupID)
//			break;
//	}
//	if (GroupIndex >= (int)m_FavorroomGroupList.DataCount)
//	{
//		sipwarning("Invalid GroupID!! FID=%d, GroupID=%d", m_FID, GroupID);
//		return;
//	}
//
//	for (int i = GroupIndex + 1; i < (int)m_FavorroomGroupList.DataCount; i ++)
//	{
//		m_FavorroomGroupList.Datas[i - 1] = m_FavorroomGroupList.Datas[i];
//	}
//	m_FavorroomGroupList.DataCount --;
//
//	// Save
//	SaveFavorroomGroup();
//
//	{
//		MAKEQUERY_MoveFavorroomToDefaultGroup(strQ, m_FID, GroupID);
//		DBCaller->execute(strQ);
//	}
//
//	sipinfo("DeleteFavorroomGroup : user:%u delete group:%d", m_FID, GroupID);
//}

// Request delete a favorite room group [d:GroupID]
//void	cb_M_CS_DEL_FAVORROOMGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);
//	T_ID		GroupID;	_msgin.serial(GroupID);
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->DelFavorroomGroup(GroupID);
//}

//void Player::RenFavorroomGroup(ucstring &GroupName, uint32 GroupID)
//{
//	int	GroupIndex = -1;
//	for (int i = 0; i < (int)m_FavorroomGroupList.DataCount; i ++)
//	{
//		if (m_FavorroomGroupList.Datas[i].GroupID == GroupID)
//			GroupIndex = i;
//		else
//		{
//			if (m_FavorroomGroupList.Datas[i].GroupName == GroupName)
//			{
//				sipwarning("Same Name Already Exist!!! FID=%d", m_FID);
//				return;
//			}
//		}
//	}
//	if (GroupIndex == -1)
//	{
//		sipwarning("Invalid GroupIndex!! FID=%d", m_FID);
//		return;
//	}
//
//	// Save
//	wcscpy(m_FavorroomGroupList.Datas[GroupIndex].GroupName, GroupName.c_str());
//
//	SaveFavorroomGroup();
//}

// Request rename a favorite room group [d:GroupID][u:GroupName]
//void	cb_M_CS_REN_FAVORROOMGROUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);
//	T_ID		GroupID;	_msgin.serial(GroupID);
//	ucstring	GroupName;	_msgin.serial(GroupName); // SAFE_SERIAL_WSTR(_msgin, GroupName, MAX_FAVORGROUPNAME, FID);
//	if (GroupName.length() > MAX_FAVORGROUPNAME)
//	{
//		sipwarning("GroupName.length() > MAX_FAVORGROUPNAME. FID=%d", FID);
//		return;
//	}
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->RenFavorroomGroup(GroupName, GroupID);
//}

//void cb_M_CS_REQ_ROOMORDERINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);
//
//	SendRoomOrderInfo(FID);
//}

void cb_M_CS_ROOMORDERINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	Player	*pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}
	uint32 groupid;	_msgin.serial(groupid);
	//sipdebug("M_CS_ROOMORDERINFO : FID=%d, groupid =%d", FID, groupid); byy krs

	RoomOrderInfoList		newinfo;
	_msgin.serial(newinfo.DataCount);
	if (newinfo.DataCount > MAX_FAVORROOMNUM)
	{
		sipwarning("newinfo.DataCount(%d) > MAX_FAVORROOMNUM", newinfo.DataCount);
		return;
	}
	//sipdebug("M_CS_ROOMORDERINFO : FID=%d, count =%d", FID, newinfo.DataCount); byy krs
	for (uint32 i = 0; i < newinfo.DataCount; i++)
	{
		_msgin.serial(newinfo.Datas[i].RoomID);
	}
	if (groupid == GROUP_ID_MYROOMS)
	{
		if (newinfo.DataCount > MAX_OWNROOMCOUNT)
		{
			sipwarning("newinfo.DataCount > MAX_OWNROOMCOUNT. FID=%d, InputDataCount=%d", FID, newinfo.DataCount);
			return;
		}
		pPlayer->SaveOwnRoomOrderInfo(newinfo);
	}
	else if (groupid == GROUP_ID_MYMANAGEROOMS)
	{
		pPlayer->SaveManageRoomOrderInfo(newinfo);
	}
	else
	{
		pPlayer->SaveFaveRoomOrderInfo(groupid, newinfo);
	}
}

//// ======================================================
//// ?룽꼪驪꾦옕?닸쭥?멧푷?볠춨?
//// ======================================================
//static void	DBCB_DBGetFamilyFigure(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_ID			FigureID	= *(uint32 *)argv[0];
//	uint8			SecretFlag	= *(uint8 *)argv[1];
//	T_FAMILYID		FID	= *(T_FAMILYID *)argv[2];
//
//	if (!nQueryResult)
//	{
//		sipwarning("Shrd_Family_GetFigureID: execute failed!");
//		return;
//	}
//
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->SetFamilyFigure(FigureID, SecretFlag);
//
//	// ?욘퍒??옔勇썸궠 ?룽꼪驪꾦옕?녔즼??麗ㅸ꺍驛묌옔??녷썷?딃씣?뉚쓨??麗ㅶ렕??쎕?
//	CMessage	msgOut(M_SC_FAMILY_FIGURE);
//	msgOut.serial(FID, FID, FigureID, SecretFlag);
//	CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
//
//	//CMessage	msgOut2(M_SC_FAMILY_FACEMODEL);
//	//msgOut2.serial(FID, FID, FigureID, SecretFlag);
//	//CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut2);
//}

//void Player::ReadFamilyFigure()
//{
//	MAKEQUERY_GetFamilyFigure(strQ, m_FID);
//	DBCaller->executeWithParam(strQ, DBCB_DBGetFamilyFigure,
//		"DBD",
//		OUT_,		NULL,
//		OUT_,		NULL,
//		CB_,		m_FID);
//}

// Set Family Figure ([d:FigureID][b:SecretFlag])
void cb_M_CS_SET_FAMILY_FIGURE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_ID		FigureID;	_msgin.serial(FigureID);
	uint8		FigureSecretFlag;	_msgin.serial(FigureSecretFlag);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	Player	*pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	uint32 old_figure_id = pPlayer->GetFigureID();
	if (old_figure_id > 0 && FigureID != old_figure_id) {
		// 删除旧的
		CMessage msg2his(M_CS_REQ_DELETE);
		T_DATATYPE dt = dataFamilyFigure;
		msg2his.serial(FID, dt, old_figure_id);
		CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msg2his);
	}

	// 设置新的
	pPlayer->ChangeFamilyFigure(FigureID, FigureSecretFlag);

	// 通知客户端
	CMessage msgout(M_SC_FAMILY_FIGURE);
	msgout.serial(FID, FID, FigureID, FigureSecretFlag);
	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
}

//void cb_M_CS_SET_FAMILY_FACEMODEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);
//	T_ID		FigureID;	_msgin.serial(FigureID);
//	uint8		FigureSecretFlag;	_msgin.serial(FigureSecretFlag);
//
//	Player	*pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	pPlayer->ChangeFamilyFigure(FigureID, FigureSecretFlag);
//}

void Player::ChangeFamilyFigure(uint32 FigureID, uint8 FigureSecretFlag)
{
	SetFamilyFigure(FigureID, FigureSecretFlag);

	{
		MAKEQUERY_SetFamilyFigure(strQ, m_FID, FigureID, FigureSecretFlag);
		DBCaller->execute(strQ);
	}
}

// Request Family Figure ([d:targetFID])
void cb_M_CS_GET_FAMILY_FIGURE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_FAMILYID	targetFID;	_msgin.serial(targetFID);

	uint32		FigureID = 0;
	Player	*pPlayer = GetPlayer(targetFID);
	if (pPlayer == NULL)
	{
		sipdebug("GetPlayer = NULL. targetFID=%d", targetFID);
	}
	else if (FID == targetFID)
	{
		FigureID = pPlayer->GetFigureID();
	}
	else
	{
		switch (pPlayer->GetFigureSecretFlag())
		{
		case 1: // 완전공개
			FigureID = pPlayer->GetFigureID();
			break;
		case 2: // 비공개
			break;
		case 3: // 친구공개
			if (IsInFriendList(targetFID, FID))
				FigureID = pPlayer->GetFigureID();
			break;
		}
	}

	CMessage	msgOut(M_SC_FAMILY_FIGURE);
	msgOut.serial(FID, targetFID, FigureID);
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

//void cb_M_CS_GET_FAMILY_FACEMODEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);
//	T_FAMILYID	targetFID;	_msgin.serial(targetFID);
//
//	uint32		FigureID = 0;
//	Player	*pPlayer = GetPlayer(targetFID);
//	if (pPlayer == NULL)
//	{
//		sipdebug("GetPlayer = NULL. targetFID=%d", targetFID);
//	}
//	else if (FID == targetFID)
//	{
//		FigureID = pPlayer->GetFigureID();
//	}
//	else
//	{
//		switch (pPlayer->GetFigureSecretFlag())
//		{
//		case 1: // 瓿店皽
//			FigureID = pPlayer->GetFigureID();
//			break;
//		case 2: // 牍勱车臧?
//			break;
//		case 3: // 旃滉惮瓿店皽
//			if (pPlayer->IsFriendList(FID))
//				FigureID = pPlayer->GetFigureID();
//			break;
//		}
//	}
//
//	CMessage	msgOut(M_SC_FAMILY_FACEMODEL);
//	msgOut.serial(FID, targetFID, FigureID);
//	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
//}

//============================================================
//   旮半厫鞚柬櫆霃欔磤霠?
//============================================================
SITE_ACTIVITY	g_SiteActivity;
SYSTEM_GIFTITEM g_BeginnerMstItem;
SITE_CHECKIN	g_CheckIn;
void cb_M_NT_CURRENT_ACTIVITY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	_msgin.serial(g_SiteActivity.m_ActID);
	_msgin.serial(g_SiteActivity.m_Text);
	_msgin.serial(g_SiteActivity.m_Count);
	for (uint8 i = 0; i < g_SiteActivity.m_Count; i ++)
	{
		_msgin.serial(g_SiteActivity.m_ActItemID[i]);
		_msgin.serial(g_SiteActivity.m_ActItemCount[i]);
	}
}

void cb_M_NT_BEGINNERMSTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	_msgin.serial(g_BeginnerMstItem.m_GiftID);
	_msgin.serial(g_BeginnerMstItem.m_Title);
	_msgin.serial(g_BeginnerMstItem.m_Content);
	_msgin.serial(g_BeginnerMstItem.m_Count);

	for (uint8 i = 0; i < g_BeginnerMstItem.m_Count; i ++)
	{
		_msgin.serial(g_BeginnerMstItem.m_ItemID[i]);
		_msgin.serial(g_BeginnerMstItem.m_ItemCount[i]);
	}
}

void cb_M_NT_CURRENT_CHECKIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	_msgin.serial(g_CheckIn.m_ActID, g_CheckIn.m_bActive, g_CheckIn.m_Text, g_CheckIn.m_BeginTime.timevalue, g_CheckIn.m_EndTime.timevalue);
	for (uint8 i = 0; i < CIS_MAX_DAYS; i ++)
	{
		_msgin.serial(g_CheckIn.m_CheckInItems[i].m_Count);
		for (uint8 j = 0; j < g_CheckIn.m_CheckInItems[i].m_Count; j++)
		{
			_msgin.serial(g_CheckIn.m_CheckInItems[i].m_ItemID[j], g_CheckIn.m_CheckInItems[i].m_ItemCount[j]);
		}
	}
}

void SendCheckUserActivity(uint32 UserID, uint32 FID)
{
	if (g_SiteActivity.m_ActID == 0)
		return;

	CMessage	msgOut(M_SM_CHECK_USER_ACTIVITY);
	msgOut.serial(UserID, FID);
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
}

void SendCheckBeginnerMstItem(uint32 UserID, uint32 FID)
{
	if (g_BeginnerMstItem.m_GiftID == 0)
		return;

	CMessage	msgOut(M_SM_CHECK_BEGINNERMSTITEM);
	msgOut.serial(UserID, FID);
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
}

void cb_M_MS_CHECK_USER_ACTIVITY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_ID		LastActID;	_msgin.serial(LastActID);

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	//pPlayer->m_LastActID = LastActID;
	if (LastActID != g_SiteActivity.m_ActID)
	{
		CMessage	msgOut(M_SC_CURRENT_ACTIVITY);
		msgOut.serial(FID, g_SiteActivity.m_ActID, g_SiteActivity.m_Text, g_SiteActivity.m_Count);
		for (uint8 i = 0; i < g_SiteActivity.m_Count; i ++)
		{
			msgOut.serial(g_SiteActivity.m_ActItemID[i], g_SiteActivity.m_ActItemCount[i]);
		}
		CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
	}
}

void cb_M_MS_CHECK_BEGINNERMSTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	//pPlayer->m_IsGetBeginnerItem = false;
	{
		CMessage	msgOut(M_SC_GIFTITEM);
		msgOut.serial(FID, g_BeginnerMstItem.m_GiftID, g_BeginnerMstItem.m_Title, g_BeginnerMstItem.m_Content, g_BeginnerMstItem.m_Count);
		for (uint8 i = 0; i < g_BeginnerMstItem.m_Count; i ++)
		{
			msgOut.serial(g_BeginnerMstItem.m_ItemID[i], g_BeginnerMstItem.m_ItemCount[i]);
		}
		CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
	}
}

//// Request current activity items (Attention!! Client must check inven)
//void cb_M_CS_REQ_ACTIVITY_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);
//
//	if (g_SiteActivity.m_ActID == 0)
//	{
//		sipwarning("g_SiteActivity.m_ActID == 0!!! FID=%d", FID);
//		return;
//	}
//
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	if (pPlayer->m_LastActID == g_SiteActivity.m_ActID)
//	{
//		sipwarning("pPlayer->m_LastActID == g_SiteActivity.m_ActID. FID=%d", FID);
//		return;
//	}
//
//	// OK
//	pPlayer->m_LastActID = g_SiteActivity.m_ActID;
//
//	CMessage	msgOut(M_NT_REQ_ACTIVITY_ITEM);
//	msgOut.serial(FID, g_SiteActivity.m_ActID);
//	CUnifiedNetwork::getInstance()->send(pPlayer->m_RoomSid, msgOut);
//}

//void cb_M_CS_RECV_GIFTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID	FID;	_msgin.serial(FID);
//
//	if (g_BeginnerMstItem.m_GiftID == 0)
//	{
//		sipwarning("g_BeginnerMstItem.m_GiftID == 0!!! FID=%d", FID);
//		return;
//	}
//
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	if (pPlayer->m_IsGetBeginnerItem == true)
//	{
//		sipwarning("pPlayer->m_IsGetBeginnerItem == true. FID=%d", FID);
//		return;
//	}
//
//	// OK
//	pPlayer->m_IsGetBeginnerItem = true;
//
//	CMessage	msgOut(M_NT_RECV_GIFTITEM);
//	msgOut.serial(FID);
//	CUnifiedNetwork::getInstance()->send(pPlayer->m_RoomSid, msgOut);
//}


//=================================================
//  於曤车旃措摐旮半姤
//=================================================
std::list<LuckHistory>	lstLuckHistory;
static TTime	tmLastLuck4 = 0;
static uint32 GetLuckLv4CountToday()
{
	uint32	LuckLv4CountToday = 0;
	TTime	curTime = GetDBTimeSecond();
	std::list<LuckHistory>::iterator it;
	for (it = lstLuckHistory.begin(); it != lstLuckHistory.end(); it++)
	{
		LuckHistory& curHis = *it;
		if (curHis.LuckLevel != 4)
			continue;
		if (IsSameDay(curTime, curHis.LuckDBTime))
			LuckLv4CountToday ++;
	}
	return LuckLv4CountToday;
}

struct Luck4RoomData
{
	T_ROOMID		RoomID;
	T_NAME			RoomName;
	T_FAMILYID		MasterFID;
	ucstring		MasterFName;
	T_FAMILYID		FID;
	T_FAMILYNAME	FName;
	uint8			LuckLevel;
	uint32			LuckID;
	uint8			BlessCardID;
};
std::list<Luck4RoomData> lstLuck4Rooms;
std::list<Luck4RoomData>::iterator GetLuck4Info(T_ROOMID roomid)
{
	std::list<Luck4RoomData>::iterator it;
	for (it = lstLuck4Rooms.begin(); it != lstLuck4Rooms.end(); it++)
	{
		if (it->RoomID == roomid)
			return it;
	}
	return lstLuck4Rooms.end();
}

std::map<uint64, TTime> mapBlessCardReceiveList;	// (FID << 32) | RoomID
void cb_M_NT_LUCK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	Luck4RoomData	data;

	_msgin.serial(data.RoomID);
	_msgin.serial(data.RoomName);
	_msgin.serial(data.MasterFID);
	_msgin.serial(data.MasterFName);
	_msgin.serial(data.FID);
	_msgin.serial(data.FName);
	_msgin.serial(data.LuckLevel);
	_msgin.serial(data.LuckID);

	TTime tmCur = CTime::getLocalTime();
	if (data.LuckLevel == 4)
	{
		uint32 scount = GetLuckLv4CountToday();
		if (scount >= MAX_LUCK_LV4_COUNT ||
			(tmCur - tmLastLuck4) <= 10*60*1000)	// 10 minutes
		{
			bool bPlay = false;
			CMessage	msg(M_NT_LUCK4_CONFIRM);
			msg.serial(bPlay, data.RoomID, data.FID, data.FName, data.LuckID);
			CUnifiedNetwork::getInstance()->send(_tsid, msg);
			return;
		}
		data.BlessCardID = GetBlessCardID(data.LuckID);
		lstLuck4Rooms.push_back(data);
	}

	lstLuckHistory.push_back(LuckHistory(data.FID, data.RoomID, data.LuckLevel, data.LuckID, GetDBTimeSecond()));
	//sipinfo(L"Luck info : FID=%d, FName=%s, RoomID=%d, RoomName=%s, MasterFID=%d, MasterName=%s, Luck=%d, LuckID=%d",
		//		data.FID, data.FName.c_str(), data.RoomID, data.RoomName.c_str(), data.MasterFID, data.MasterFName.c_str(), data.LuckLevel, data.LuckID); byy krs

	{
		MAKEQUERY_ADDLUCKHISTORY(strQuery, data.FID, data.RoomID, data.LuckLevel, data.LuckID, GetDBTimeSecond());
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
	}

	if (data.LuckLevel != 4)
		return;

	bool bPlay = true;
	CMessage	msg(M_NT_LUCK4_CONFIRM);
	msg.serial(bPlay, data.RoomID, data.FID, data.FName, data.LuckID);
	CUnifiedNetwork::getInstance()->send(_tsid, msg);

	tmLastLuck4 = tmCur;

	T_FAMILYID FID = data.FID;
	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	uint32 count = 0;
	for (int i = 0; i < MAX_BLESSCARD_ID; i ++)
	{
		count += pPlayer->m_BlessCardCount[i];
	}

	uint8 nAddCount = 5;
	if (count + nAddCount > MAX_BLESSCARD_COUNT)
		nAddCount = (uint8)(MAX_BLESSCARD_COUNT - count);

	pPlayer->m_BlessCardCount[data.BlessCardID-1] += nAddCount;
	if (nAddCount > 0)
	{
		MAKEQUERY_SaveBlessCard(strQ, FID, data.BlessCardID, pPlayer->m_BlessCardCount[data.BlessCardID-1]);
		DBCaller->execute2(strQ, 0);
	
		uint64	key = FID;
		key = (key << 32) | data.RoomID;
		mapBlessCardReceiveList[key] = GetDBTimeSecond();
	}
}

//static void	DBCB_DBLoadBlessCard(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];
//
//	if (!nQueryResult)
//	{
//		sipwarning("Shrd_GetBlessCard: execute failed!");
//		return;
//	}
//
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	for (uint32 i = 0; i < MAX_BLESSCARD_ID; i ++)
//	{
//		pPlayer->m_BlessCardCount[i] = 0;
//	}
//
//	CMessage	msgOut(M_SC_BLESSCARD_LIST);
//	msgOut.serial(FID);
//
//	uint32 nRows;	resSet->serial(nRows);
//	for (uint32 i = 0; i < nRows; i ++)
//	{
//		uint8		BlessCardID, Count;
//		resSet->serial(BlessCardID, Count);
//		pPlayer->m_BlessCardCount[BlessCardID - 1] = Count;
//
//		msgOut.serial(BlessCardID, Count);
//	}
//
//	CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
//}

//void	Player::LoadBlessCard()
//{
//	MAKEQUERY_LoadBlessCard(strQ, m_FID);
//	DBCaller->executeWithParam(strQ, DBCB_DBLoadBlessCard,
//		"D",
//		CB_,		m_FID);
//}

void cb_M_NT_REQGIVEBLESSCARD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	T_ID		LuckID;
	_msgin.serial(FID, LuckID);

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	uint32	ErrorCode = ERR_NOERR;
	int		count = 0;
	for (int i = 0; i < MAX_BLESSCARD_ID; i ++)
	{
		count += pPlayer->m_BlessCardCount[i];
	}
	if (count >= MAX_BLESSCARD_COUNT)
	{
		ErrorCode = E_BLESSCARD_FULL;
	}

	if (ErrorCode == ERR_NOERR)
	{
		// Give BlessCard
		uint8 BlessCardID = GetBlessCardID(LuckID);
		pPlayer->m_BlessCardCount[BlessCardID - 1] ++;

		// DB鞐?氤搓磤
		{
			MAKEQUERY_SaveBlessCard(strQ, FID, BlessCardID, pPlayer->m_BlessCardCount[BlessCardID - 1]);
			DBCaller->execute(strQ, NULL);
		}

		CMessage	msgOut(M_SC_BLESSCARD_GET);
		msgOut.serial(FID, ErrorCode, BlessCardID);
		CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
		
		CMessage	msgNT(M_NT_RESGIVEBLESSCARD);
		msgNT.serial(FID, LuckID);
		CUnifiedNetwork::getInstance()->send(_tsid, msgNT);
	}
}


sint8	GetNextCheckDays(uint32 today, sint8 currentCheckDays, uint32 lastCheckDay)
{
	sint8		nCheckinDays = CIS_CANT_CHECK;

	uint32 deltaDays = today - lastCheckDay;

	if (deltaDays == 0)	// possible to checkin one time per one day.
	{
		nCheckinDays = CIS_CANT_CHECK;
	}
	else
	{
		sint8 nCandiIndex = -1;
		if (deltaDays == 1)	// If serial checkin, set next index
		{
			nCandiIndex = currentCheckDays;
		}
		else	// If no serial checkin, reset
		{
			nCandiIndex = 0;
		}

		if (nCandiIndex == 0)	// If starting checkin
		{
			nCheckinDays = 1;

		}
		else // If serial checkin, success without fail
		{
			nCheckinDays = nCandiIndex + 1;
			if (nCheckinDays >= CIS_MAX_DAYS)
				nCheckinDays = 0;
		}
	}
	return nCheckinDays;
}

// 第一天：一种道具，在花类，香类，糕点类，酒类，水果类，献宝物品,中奖道具中任意选择
// 第二天：二种道具，在花类，香类，糕点类，酒类，水果类，献宝物品,中奖道具中任意选择（两种道具不可以是同一种类别的道具）
// 第三天：三种道具，在花类，香类，糕点类，酒类，水果类，献宝物品,中奖道具中任意选择（三种道具不可以是同一种类别的道具）
// 第四天：三种道具，在花类，香类，糕点类，酒类，水果类，献宝物品,中奖道具中任意选择（三种道具不可以是同一种类别的道具）
// 第五天：四种道具，在花类，香类，糕点类，酒类，水果类，献宝物品,中奖道具，献花特使中任意选择（四种道具不可以是同一种类别的道具）
// 第六天：四种道具，在花类x2，香类x2，糕点类x2，酒类x2，水果类x2，献宝物品x2,中奖道具，献花特使中任意选择（四种道具不可以是同一种类别的道具）
// 第七天：四种道具，天寿套餐，献宝套餐，献花特使,敬献品X6,献宝物品X6(可以是同一类道具）

uint32 g_CheckinMstItems[11][100]= // 0:花类, 1:香类, 2:酒类, 3:糕点类, 4:水果类, 5:敬献品(糕点+水果), 6:献宝物品, 7:天寿套餐, 8:献宝套餐, 9:献花特使, 10:中奖道具
{
	// 0:花类
	{6, 700122, 700123, 700124, 700125, 700126, 700127},
	// 1:香类
	{24,700011, 700012, 700013, 700014, 700015, 700016, 700017, 700018, 700019, 700020, 
		700372, 700373, 700374, 700375, 700376, 700377, 700378, 700379, 700380, 700381, 
		700382, 700383, 700384, 700385},
	// 2:酒类
	{12,700204, 700205, 700208, 700211, 700212, 700213, 700386, 700387, 700388, 700389, 
		700390, 700391},
	// 3:糕点类
	{34,700305, 700306, 700307, 700308, 700323, 700324, 700325, 700327, 700329, 700330,
		700331, 700332, 700333, 700334, 700336, 700338, 700339, 700340, 700341, 700342,
		700343, 700344, 700345, 700346, 700347, 700348, 700349, 700350, 700351, 700352,
		700353, 700354, 700355, 700356},
	// 4:水果类
	{32,700304, 700309, 700310, 700311, 700312, 700313, 700314, 700315, 700316, 700317,
		700318, 700319, 700320, 700321, 700322, 700326, 700337, 700357, 700358, 700359,
		700360, 700361, 700362, 700363, 700364, 700365, 700366, 700367, 700368, 700369,
		700370, 700371},
	// 5:敬献品(糕点+水果)
	{66,700305, 700306, 700307, 700308, 700323, 700324, 700325, 700327, 700329, 700330,
		700331, 700332, 700333, 700334, 700336, 700338, 700339, 700340, 700341, 700342,
		700343, 700344, 700345, 700346, 700347, 700348, 700349, 700350, 700351, 700352,
		700353, 700354, 700355, 700356,
		700304, 700309, 700310, 700311, 700312, 700313, 700314, 700315, 700316, 700317,
		700318, 700319, 700320, 700321, 700322, 700326, 700337, 700357, 700358, 700359,
		700360, 700361, 700362, 700363, 700364, 700365, 700366, 700367, 700368, 700369,
		700370, 700371},
	// 6:献宝物品
	{23,730006, 730007, 730020, 730021, 730022, 730023, 730024, 730025, 730026, 730027,
		730100, 730101, 730102, 730103, 730104, 730105, 730106, 730107, 730108, 730109,
		730110, 730111, 730112},
	// 7:天寿套餐
	{11,700501, 700502, 700503, 700510, 700511, 700513, 700514, 700518, 700522, 700526, 
		700527},
	// 8:献宝套餐
	{13,730032, 730033, 730035, 730040, 730042, 730044, 730045, 730046, 730047, 730049, 
		730050, 730051, 730052},
	// 9:天园特使
	{1, 560002},
	// 中奖道具
	{7, 1000001, 1000003, 1000008, 1000009, 1000010, 1000011, 1000018}
};

void GetCheckinItems(int Day, uint32 ModelID, CMessage& msgOut)
{
	// Day : 1,2,3,4,5,6,0 -> 1,2,3,4,5,6,7
	if (Day == 0)
		Day = 7;

	if (ModelID < 3 || ModelID > 8)
	{
		sipwarning("Invalid ModelID. ModelID=%d", ModelID);
		return;
	}

	T_ID	items[10];
	int		count = 0, n;
	uint8 checkin_count = 0;

	if (Day < 7)
	{
		uint32	itemcount = 0x01000000;
		if (Day == 6)
		{
			itemcount = 0x02000000;
			n = rand() % g_CheckinMstItems[9][0];	items[count ++] = g_CheckinMstItems[9][n + 1] | 0x01000000;	// 献花特使
		}
		n = rand() % g_CheckinMstItems[0][0];	items[count ++] = g_CheckinMstItems[0][n + 1] | itemcount;	// 花类
		n = rand() % g_CheckinMstItems[1][0];	items[count ++] = g_CheckinMstItems[1][n + 1] | itemcount;	// 香类
		n = rand() % g_CheckinMstItems[2][0];	items[count ++] = g_CheckinMstItems[2][n + 1] | itemcount;	// 糕点类
		n = rand() % g_CheckinMstItems[3][0];	items[count ++] = g_CheckinMstItems[3][n + 1] | itemcount;	// 酒类
		n = rand() % g_CheckinMstItems[4][0];	items[count ++] = g_CheckinMstItems[4][n + 1] | itemcount;	// 水果类
		n = rand() % g_CheckinMstItems[6][0];	items[count ++] = g_CheckinMstItems[6][n + 1] | itemcount;	// 献宝物品
		n = rand() % g_CheckinMstItems[10][0];	items[count ++] = g_CheckinMstItems[10][n + 1] | 0x01000000;	// 中奖道具
	}
	else
	{
		n = rand() % g_CheckinMstItems[7][0];	items[count ++] = g_CheckinMstItems[7][n + 1] | 0x01000000;	// 天寿套餐
		n = rand() % g_CheckinMstItems[8][0];	items[count ++] = g_CheckinMstItems[8][n + 1] | 0x01000000;	// 献宝套餐
		n = rand() % g_CheckinMstItems[9][0];	items[count ++] = g_CheckinMstItems[9][n + 1] | 0x01000000;	// 献花特使
		n = rand() % g_CheckinMstItems[5][0];	items[count ++] = g_CheckinMstItems[5][n + 1] | 0x06000000;	// 敬献品(糕点+水果)
		n = rand() % g_CheckinMstItems[6][0];	items[count ++] = g_CheckinMstItems[6][n + 1] | 0x06000000;	// 献宝物品
	}
	//if (Day >= 3)
	//{
	//	while (true)
	//	{
	//		int n = rand() % g_CheckinMstItems[4][0];	// 服装
	//		int DressModelID = g_CheckinMstItems[4][n + 1] / 100;
	//		if (DressModelID == ModelID)
	//		{
	//			items[count ++] = g_CheckinMstItems[4][n + 1];
	//			break;
	//		}
	//	}
	//}

	switch (Day)
	{
	case 1:
		checkin_count = 1;
		break;
	case 2:
		checkin_count = 2;
		break;
	case 3:
	case 4:
		checkin_count = 3;
		break;
	case 5:
	case 6:
	case 7:
		checkin_count = 4;
		break;
	default:
		sipwarning("Invalid day. Day=%d", Day);
		break;
	}

	msgOut.serial(checkin_count);
	uint32	itemID;
	uint16	itemCount;
	for (uint8 i = 0; i < checkin_count; i ++)
	{
		n = rand() % count;
		itemCount = (uint16) (items[n] >> 24);
		itemID = items[n] & 0x00FFFFFF;
		msgOut.serial(itemID, itemCount);

		count --;
		items[n] = items[count];
	}
}

//static void	DBCB_DBLoadCheckIn(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];
//
//	if (!nQueryResult)
//	{
//		sipwarning("Shrd_family_Signin_Day_list: execute failed!");
//		return;
//	}
//
//	Player *pPlayer = GetPlayer(FID);
//	if (pPlayer == NULL)
//	{
//		sipwarning("GetPlayer = NULL. FID=%d", FID);
//		return;
//	}
//
//	uint32 nRows;	resSet->serial(nRows);
//	T_DATETIME		LastDays;		resSet->serial(LastDays);
//	uint8			nCheckDays;		resSet->serial(nCheckDays);
//
//	//pPlayer->m_CheckInInfo.m_nCheckinDays = nCheckDays;
//
//	SIPBASE::MSSQLTIME tempTime;
//	tempTime.SetTime(LastDays);
//	uint32	nLastCheckinDay = tempTime.GetDaysFrom1970();
//	//pPlayer->m_CheckInInfo.m_nLastCheckinDay = tempTime.GetDaysFrom1970();
//
//	uint32		today = GetDBTimeDays();
//	sint8		nNextCheckinDays = GetNextCheckDays(today, nCheckDays, nLastCheckinDay);
//	
//	sint8		nNowCheckInstate;
//	if (nNextCheckinDays != CIS_CANT_CHECK)
//	{
//		if (nNextCheckinDays == 0)
//			nNowCheckInstate = CIS_MAX_DAYS - 1;
//		else
//			nNowCheckInstate = nNextCheckinDays - 1;
//	}
//	else
//	{
//		nNowCheckInstate = CIS_CANT_CHECK;
//	}
//
//	CMessage msg(M_SC_CHECKINSTATE);
//	msg.serial(FID, nNowCheckInstate);
//	CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msg);
//}

//void LoadCheckIn(T_FAMILYID FID)
//{
//	MAKEQUERY_LoadCheckIn(strQ, FID);
//	DBCaller->executeWithParam(strQ, DBCB_DBLoadCheckIn,
//							   "D",
//							   CB_,		FID);
//}

//////////////////////////////////////////////////////////////////////////

static void	DBCB_DBLoadCheckInForReq(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID	FID	= *(T_FAMILYID *)argv[0];
	T_MODELID	ModelID	= *(uint32 *)argv[1];

	if (!nQueryResult)
	{
		sipwarning("Shrd_family_Signin_Day_list: execute failed!");
		return;
	}

	//Player *pPlayer = GetPlayer(FID);
	//if (pPlayer == NULL)
	//{
	//	sipwarning("GetPlayer = NULL. FID=%d", FID);
	//	return;
	//}

	uint32 nRows;	resSet->serial(nRows);
	T_DATETIME		LastDays;		resSet->serial(LastDays);
	uint8			nCheckDays;		resSet->serial(nCheckDays);

	//pPlayer->m_CheckInInfo.m_nCheckinDays = nCheckDays;

	SIPBASE::MSSQLTIME tempTime;
	tempTime.SetTime(LastDays);
	uint32	nLastCheckinDay = tempTime.GetDaysFrom1970();
	//pPlayer->m_CheckInInfo.m_nLastCheckinDay = tempTime.GetDaysFrom1970();

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}
	
	uint32 today = GetDBTimeDays();
	sint8		nCheckinDays = GetNextCheckDays(today, nCheckDays, nLastCheckinDay);

	CMessage	msg(M_NT_RESCHECKIN);
	msg.serial(FID, nCheckinDays);
	if (nCheckinDays != CIS_CANT_CHECK)
	{
		//sint8 nItemIndex;
		//if (nCheckinDays == 0)
		//	nItemIndex = CIS_MAX_DAYS - 1;
		//else
		//	nItemIndex = nCheckinDays - 1;

		//SITE_CHECKIN::CHECKINITEM& curitems = g_CheckIn.m_CheckInItems[nItemIndex];
		//msg.serial(curitems.m_Count);
		//for (uint8 i = 0; i < curitems.m_Count; i++)
		//{
		//	T_FITEMNUM num = static_cast<T_FITEMNUM>(curitems.m_ItemCount[i]);
		//	msg.serial(curitems.m_ItemID[i], num);
		//}
		GetCheckinItems(nCheckinDays, ModelID, msg);

		CUnifiedNetwork::getInstance()->send(pPlayer->m_RoomSid, msg);

		// DB갱신
		//pPlayer->m_CheckInInfo.m_nCheckinDays = nCheckinDays;
		//pPlayer->m_CheckInInfo.m_nLastCheckinDay = GetDBTimeDays();

		MAKEQUERY_SaveCheckIn(strQ, FID, nCheckinDays);
		DBCaller->execute2(strQ, 0);
	}
}

void cb_M_NT_REQCHECKIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_MODELID	ModelID;	_msgin.serial(ModelID);

	MAKEQUERY_LoadCheckIn(strQ, FID);
	DBCaller->executeWithParam(strQ, DBCB_DBLoadCheckInForReq,
							   "DD",
							   CB_,		FID,
							   CB_,		ModelID);
}

void cb_M_NT_CURLUCKINPUBROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	_msgin.serial(g_CurLuckInPubInfo.LuckID);
	_msgin.serial(g_CurLuckInPubInfo.FamilyID);
	_msgin.serial(g_CurLuckInPubInfo.FamilyName);
	_msgin.serial(g_CurLuckInPubInfo.TimeM);
	_msgin.serial(g_CurLuckInPubInfo.TimeDeltaS);

	T_FAMILYID FID = g_CurLuckInPubInfo.FamilyID;
	if (g_CurLuckInPubInfo.LuckID == 0 || FID == NOFAMILYID)
		return;

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}
	uint8 BlessCardID = GetBlessCardID(g_CurLuckInPubInfo.LuckID);

	uint32 count = 0;
	for (int i = 0; i < MAX_BLESSCARD_ID; i ++)
	{
		count += pPlayer->m_BlessCardCount[i];
	}

	uint8 nAddCount = 5;
	if (count + nAddCount > MAX_BLESSCARD_COUNT)
		nAddCount = (uint8)(MAX_BLESSCARD_COUNT - count);

	pPlayer->m_BlessCardCount[BlessCardID-1] += nAddCount;
	if (nAddCount > 0)
	{
		MAKEQUERY_SaveBlessCard(strQ, FID, BlessCardID, pPlayer->m_BlessCardCount[BlessCardID-1]);
		DBCaller->execute2(strQ, 0);
	}

}

//////////////////////////////////////////////////////////////////////////
void cb_M_NT_LARGEACTPRIZE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint8		BlessNum[MAX_BLESSCARD_ID];
	_msgin.serial(FID);
	for (int i = 0; i < MAX_BLESSCARD_ID; i ++)
		_msgin.serial(BlessNum[i]);

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}
	CMessage msgOut(M_SC_LARGEACTPRIZE);
	msgOut.serial(FID);

	uint32 count = 0;
	for (int i = 0; i < MAX_BLESSCARD_ID; i ++)
	{
		count += pPlayer->m_BlessCardCount[i];
	}
	
	for (int i = 0; i < MAX_BLESSCARD_ID; i ++)
	{
		uint8 nAddCount = BlessNum[i];
		if (count + nAddCount > MAX_BLESSCARD_COUNT)
			nAddCount = (uint8)(MAX_BLESSCARD_COUNT - count);

		msgOut.serial(nAddCount);
		pPlayer->m_BlessCardCount[i] += nAddCount;
		count += nAddCount;
		if (nAddCount > 0)
		{
			MAKEQUERY_SaveBlessCard(strQ, FID, (i+1), pPlayer->m_BlessCardCount[i]);
			DBCaller->execute2(strQ, 0);
		}
	}
	_msgin.appendBufferTo(msgOut, false);

	CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
}

// Request for get BlessCard
void cb_M_CS_BLESSCARD_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	uint64	key = FID;
	key = (key << 32) | pPlayer->m_RoomID;

	uint32	ErrorCode = 0;
	uint8	BlessCardID = 0;

	std::list<Luck4RoomData>::iterator	it1 = GetLuck4Info(pPlayer->m_RoomID);
	if (it1 == lstLuck4Rooms.end())
	{
		ErrorCode = E_BLESSCARD_BADROOM;
	}
	else
	{
		std::map<uint64, TTime>::iterator	it2 = mapBlessCardReceiveList.find(key);
		if (it2 != mapBlessCardReceiveList.end())
		{
			ErrorCode = E_BLESSCARD_ALREADY_GET;
		}
		else
		{
			int		count = 0;
			for (int i = 0; i < MAX_BLESSCARD_ID; i ++)
			{
				count += pPlayer->m_BlessCardCount[i];
			}
			if (count >= MAX_BLESSCARD_COUNT)
			{
				ErrorCode = E_BLESSCARD_FULL;
			}
		}
	}

	if (ErrorCode == 0)
	{
		// Give BlessCard
		BlessCardID = it1->BlessCardID;
		pPlayer->m_BlessCardCount[BlessCardID - 1] ++;

		mapBlessCardReceiveList[key] = GetDBTimeSecond();

		// DB鞐?氤搓磤
		{
			MAKEQUERY_SaveBlessCard(strQ, FID, BlessCardID, pPlayer->m_BlessCardCount[BlessCardID - 1]);
			DBCaller->execute(strQ, NULL);
		}
	}

	CMessage	msgOut(M_SC_BLESSCARD_GET);
	msgOut.serial(FID, ErrorCode, BlessCardID);
	CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
}

// Send BlessCard to other ([b:BlessCardID][d:tarfetFID])
void cb_M_CS_BLESSCARD_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;			_msgin.serial(FID);
	uint8		BlessCardID;	_msgin.serial(BlessCardID);
	T_FAMILYID	targetFID;		_msgin.serial(targetFID);

	if (BlessCardID > MAX_BLESSCARD_ID)
	{
		sipwarning("BlessCardID > MAX_BLESSCARD_ID. FID=%d, BlessCardID=%d", FID, BlessCardID);
		return;
	}

	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	uint32	ErrorCode = 0;

	Player *pTargetPlayer = GetPlayer(targetFID);
	if (pTargetPlayer == NULL)
	{
		ErrorCode = E_BLESSCARD_SEND_NOTARGET;
	}
	else if (pPlayer->m_BlessCardCount[BlessCardID - 1] <= 0)
	{
		ErrorCode = E_BLESSCARD_SEND_NOCARD;
	}
	else
	{
		// OK
		pPlayer->m_BlessCardCount[BlessCardID - 1] --;

		{
			MAKEQUERY_SaveBlessCard(strQ, FID, BlessCardID, pPlayer->m_BlessCardCount[BlessCardID - 1]);
			DBCaller->execute(strQ, NULL);
		}

		{
			CMessage	msgOut(M_SC_BLESSCARD_RECEIVE);
			msgOut.serial(targetFID, BlessCardID, FID, pPlayer->m_FName);
			CUnifiedNetwork::getInstance()->send(pTargetPlayer->m_FSSid, msgOut);
		}

		{
			CMessage	msgOut(M_NT_BLESSCARD_USED);
			msgOut.serial(FID);
			CUnifiedNetwork::getInstance()->send(pPlayer->m_RoomSid, msgOut);
		}
	}

	{
		CMessage	msgOut(M_SC_BLESSCARD_SEND);
		msgOut.serial(FID, ErrorCode);
		CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut);
	}
}

// Request Luck4 List
//void cb_M_CS_LUCK4_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);

void SendLuck4List(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	CMessage	msgOut(M_SC_LUCK4_LIST);
	msgOut.serial(FID);
	for (std::list<Luck4RoomData>::iterator it = lstLuck4Rooms.begin(); it != lstLuck4Rooms.end(); it ++)
	{
		msgOut.serial(it->RoomID, it->RoomName, it->MasterFID, it->MasterFName, it->FID, it->FName, it->LuckID);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

//void cb_M_NT_LUCK_RESET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
void	ResetLuck4List()
{
	lstLuck4Rooms.clear();
	mapBlessCardReceiveList.clear();

	{
		CMessage	msgOut(M_NT_LUCK4_RESET);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
	}
}

void	LoadLuckHistory()
{
	lstLuckHistory.clear();
	lstLuck4Rooms.clear();

	TTime curTime = GetDBTimeSecond();
	{
		MAKEQUERY_LOADLUCKHISTORY(strQuery);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
		if (nQueryResult == true)
		{
			do
			{
				DB_QUERY_RESULT_FETCH( Stmt, row );
				if (IS_DB_ROW_VALID( row ))
				{
					COLUMN_DIGIT( row, 0, T_FAMILYID,			FID );
					COLUMN_WSTR( row, 1, FName, MAX_LEN_FAMILY_NAME);
					COLUMN_DIGIT( row, 2, T_ROOMID,		RoomID );
					COLUMN_WSTR( row, 3, RoomName, MAX_LEN_ROOM_NAME);
					COLUMN_DIGIT( row, 4, uint8,			LuckLevel );
					COLUMN_DIGIT( row, 5, uint32,			LuckID );
					COLUMN_DIGIT( row, 6, TTime,			LuckDBTime );

					if (LuckLevel == 3)	// view animal
					{
						if (IsSameDay(curTime, LuckDBTime))
						{
							lstLuckHistory.push_back(LuckHistory(FID, RoomID, LuckLevel, LuckID, LuckDBTime));
							//sipdebug(L"Load luck his data, fid=%d, roomid=%d, level=%d, luckid=%d", FID, RoomID, LuckLevel, LuckID); byy krs
						}
					}
					if (LuckLevel == 4)	// shenming
					{
						TTime deltaTime = curTime - LuckDBTime;
						if (deltaTime <= LUCKY4_INTERVAL_FAMILY ||
							deltaTime <= LUCKY4_INTERVAL_ROOM)
						{
							lstLuckHistory.push_back(LuckHistory(FID, RoomID, LuckLevel, LuckID, LuckDBTime));
							//sipdebug(L"Load luck his data, fid=%d, roomid=%d, level=%d, luckid=%d", FID, RoomID, LuckLevel, LuckID); byy krs
						}
						if (IsSameDay(curTime, LuckDBTime))
						{
							Luck4RoomData	data;
							data.RoomID		= RoomID;
							data.RoomName	= RoomName;
							data.MasterFID	= 0;
							data.MasterFName= L"";
							data.FID		= FID;
							data.FName		= FName;
							data.LuckLevel	= LuckLevel;
							data.LuckID		= LuckID;
							data.BlessCardID = GetBlessCardID(LuckID);
							lstLuck4Rooms.push_back(data);
							//sipdebug(L"Load today luck4 data, fid=%d, fname=%s, roomid=%d, level=%d, luckid=%d, roomname=%s", FID, FName.c_str(), RoomID, LuckLevel, LuckID, RoomName.c_str()); byy krs
						}
					}
				}
				else
					break;
			} while (true);

			BOOL bMore1 = Stmt.MoreResults();
			if (bMore1)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, T_ROOMID,		RoomID );
						COLUMN_DIGIT( row, 1, T_FAMILYID,	MasterID );
						COLUMN_WSTR( row, 2, MasterName, MAX_LEN_FAMILY_NAME);
						for (std::list<Luck4RoomData>::iterator it = lstLuck4Rooms.begin(); it != lstLuck4Rooms.end(); it ++)
						{
							if (it->RoomID == RoomID)
							{
								it->MasterFID = MasterID;
								it->MasterFName = MasterName;
							}
						}
					}
					else
						break;
				} while (true);
			}
		}
	}
//	{
//		MAKEQUERY_RESETLUCKHISTORY(strQuery);
//		CDBConnectionRest	DB(DBCaller);
//		DB_EXE_QUERY( DB, Stmt, strQuery );
//	}

	uint32 todayLuck4Count = GetLuckLv4CountToday();
	if (todayLuck4Count < MAX_LUCK_LV4_COUNT)
	{
		RecalLuck4Data(MAX_LUCK_LV4_COUNT-todayLuck4Count);
	}
}

void	SaveAndRefreshLuckHistory()
{
	{
		MAKEQUERY_RESETLUCKHISTORY(strQuery);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
	}

	TTime curTime = GetDBTimeSecond();

	std::list<LuckHistory>::iterator it;
	for (it = lstLuckHistory.begin(); it != lstLuckHistory.end();)
	{
		bool bAdd = false;
		LuckHistory& curHis = *it;
		if (curHis.LuckLevel == 3)	// view animal
		{
			if (IsSameDay(curTime, curHis.LuckDBTime))
			{
				bAdd = true;
			}
		}
		if (curHis.LuckLevel == 4)	// shenming
		{
			TTime deltaTime = curTime - curHis.LuckDBTime;
			if (deltaTime <= LUCKY4_INTERVAL_FAMILY ||
				deltaTime <= LUCKY4_INTERVAL_ROOM)
			{
				bAdd = true;
			}
		}
		if (bAdd)
		{
			MAKEQUERY_ADDLUCKHISTORY(strQuery, curHis.FID, curHis.RoomID, curHis.LuckLevel, curHis.LuckID, curHis.LuckDBTime);
			CDBConnectionRest	DB(DBCaller);
			DB_EXE_QUERY( DB, Stmt, strQuery );

			it++;
		}
		else
		{
			it = lstLuckHistory.erase(it);
		}
	}
}

void SendLuckHistoryToOpenroom( TServiceId _tsid)
{
	std::list<LuckHistory>::iterator it;
	for (it = lstLuckHistory.begin(); it != lstLuckHistory.end(); it++)
	{
		LuckHistory& curHis = *it;
		CMessage	msgNT(M_NT_LUCKHIS);
		msgNT.serial(curHis.FID, curHis.RoomID, curHis.LuckLevel, curHis.LuckID, curHis.LuckDBTime);
		CUnifiedNetwork::getInstance()->send(_tsid, msgNT);
	}	
}

void LoadBlessCardRecvHistory()
{
	mapBlessCardReceiveList.clear();

	TTime curTime = GetDBTimeSecond();
	{
		MAKEQUERY_LOADBLESSHIS(strQuery);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
		if (nQueryResult == true)
		{
			do
			{
				DB_QUERY_RESULT_FETCH( Stmt, row );
				if (IS_DB_ROW_VALID( row ))
				{
					COLUMN_DIGIT( row, 0, T_FAMILYID,	FID );
					COLUMN_DIGIT( row, 1, T_ROOMID,		RoomID );
					COLUMN_DIGIT( row, 2, TTime,		RecvTime );

					if (IsSameDay(curTime, RecvTime))
					{
						uint64	key = FID;
						key = (key << 32) | RoomID;
						mapBlessCardReceiveList[key] = RecvTime;
						//sipdebug(L"Load bless card history, fid=%d, roomid=%d, time=%d", FID, RoomID, RecvTime); byy krs
					}
				}
				else
					break;
			} while (true);
		}
	}
	{
		MAKEQUERY_RESETBLESSHIS(strQuery);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
	}
}

void SaveBlessCardRecvHistory()
{
	{
		MAKEQUERY_RESETBLESSHIS(strQuery);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
	}
	std::map<uint64, TTime>::iterator it;
	for (it = mapBlessCardReceiveList.begin(); it != mapBlessCardReceiveList.end(); it++)
	{
		uint64	key = it->first;
		TTime	recvTime = it->second;
		T_FAMILYID	fid = (key >> 32);
		T_ROOMID	roomid = ( key & 0xFFFFFFFF);

		MAKEQUERY_ADDBLESSHIS(strQuery, fid, roomid, recvTime);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
	}
}

//////////////////////////////////////////////////////////////////////////
extern std::string GetTodayTimeStringFromDBSecond(TTime tmCur);
struct LUCK4DATA 
{
	LUCK4DATA(TTime _plan) : planDBTimeSecond(_plan), playDBTimeSecond(0), FID(0), RoomID(0) {}
	TTime			planDBTimeSecond;
	TTime			playDBTimeSecond;
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
};
std::list<LUCK4DATA>	todayLuck4Data;

static TTime	LastPlayLuck4DataTime = 0;
void	RecalLuck4Data(uint32 nNumber, bool bTest)
{
	todayLuck4Data.clear();
	LastPlayLuck4DataTime = 0;

	TTime	curTime = GetDBTimeSecond();
	uint32  curDayTime = curTime % SECOND_PERONEDAY;
	const int hourSeconds12 = 12 * 3600;
	int nRemainTime = 60;	// 1분내에 모든 선밍이 나온다
	if (curDayTime < hourSeconds12)	// 12시가 지났으면
	{
		nRemainTime = hourSeconds12 - curDayTime;
	}
	if (bTest)
		nRemainTime = 60;
	while (todayLuck4Data.size() < nNumber)
	{
		// 12시까지의 rand시간을 얻는다
		int		nDeltatime = Irand(0, nRemainTime);
		TTime	newPlanTime = curTime + nDeltatime;
		todayLuck4Data.push_back(LUCK4DATA(newPlanTime));

		string strTime = GetTodayTimeStringFromDBSecond(newPlanTime);
		//sipdebug("Add luck4 data %s", strTime.c_str()); byy krs
	}
}


//void cb_M_NT_LUCK4_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;	
//	T_FAMILYNAME	FName;
//	T_ROOMID		RoomID;
//	_msgin.serial(FID, FName, RoomID);
//
//	bool bPlay = false;
//	TTime	curTime = GetDBTimeSecond();
//	if (curTime - LastPlayLuck4DataTime < 10*60) // 10분
//	{
//		bPlay = false;
//	}
//	else
//	{
//		std::list<LUCK4DATA>::iterator it;
//		for (it = todayLuck4Data.begin(); it != todayLuck4Data.end(); it++)
//		{
//			if (it->playDBTimeSecond != 0)
//			{
//				if (it->RoomID == RoomID)
//				{
//					bPlay = false;
//					break;
//				}
//				continue;
//			}
//			if (curTime >= it->planDBTimeSecond)
//			{
//				it->playDBTimeSecond = curTime;
//				it->FID = FID;
//				it->RoomID = RoomID;
//				LastPlayLuck4DataTime = curTime;
//				bPlay = true;
//				break;
//			}
//		}
//	}
//	
//	CMessage msg(M_NT_LUCK4_RES);
//	msg.serial(bPlay, FID, FName, RoomID);
//	CUnifiedNetwork::getInstance()->send(_tsid, msg);
//	if (bPlay)
//	{
//		sipinfo(L"Play luck4 : FID=%d, FName=%s, RoomID=%d", FID, FName.c_str(), RoomID);
//	}
//}

void	DBLOG_AddContact(T_FAMILYID _FID, T_FAMILYID _targetID)
{
	DECLARE_LOG_VARIABLES_HIS();

	MainType = DBLOG_MAINTYPE_ADDCONTACT;
	PID		= _FID;
	UID		= UserID;
	ItemID	= _targetID;
	ItemTypeID = 0;
	PName	= L"";
	ItemName= L"";

	Player *player = GetPlayer(_FID);
	if (player != NULL)
	{
		UID = player->m_UserID;
		PName = player->m_FName;
	}
	player = GetPlayer(_targetID);
	if (player != NULL)
	{
		ItemTypeID = player->m_UserID;
		ItemName = player->m_FName;
	}

	DOING_LOG();
}

void	DBLOG_DelContact(T_FAMILYID _FID, T_FAMILYID _targetID)
{
	DECLARE_LOG_VARIABLES_HIS();
	MainType = DBLOG_MAINTYPE_DELCONTACT;
	PID		= _FID;
	UID		= UserID;
	ItemID	= _targetID;
	ItemTypeID = 0;
	PName	= L"";
	ItemName= L"";

	Player *player = GetPlayer(_FID);
	if (player != NULL)
	{
		UID = player->m_UserID;
		PName = player->m_FName;
	}
	player = GetPlayer(_targetID);
	if (player != NULL)
	{
		ItemTypeID = player->m_UserID;
		ItemName = player->m_FName;
	}

	DOING_LOG();
}


std::list<uint64>	g_WaitFriendInfoList;
void RequestFriendListInfo(T_FAMILYID FID, T_ROOMID RoomID)
{
	uint64	key = RoomID;
	key = (key << 32) | FID;
	for (std::list<uint64>::iterator it = g_WaitFriendInfoList.begin(); it != g_WaitFriendInfoList.end(); it ++)
	{
		if (FID == ((*it) & 0xFFFFFFFF))
		{
			if (key == *it)
			{
				sipinfo("key == *it. FID=%d, RoomID=%d", FID, RoomID);
			}
			else
			{
				g_WaitFriendInfoList.push_back(key);
			}
			return;
		}
	}

	g_WaitFriendInfoList.push_back(key);

	uint32	changed_sn = 0;
	{
		std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
		if (it != g_FamilyFriends.end())
			changed_sn = it->second.m_Changed_SN;
	}

	{
		CMessage	msgOut(M_NT_REQ_FRIEND_INFO);
		msgOut.serial(FID, changed_sn);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}
}

// Send Friend Group List [d:FID][d:SN][b:Count][ [d:GroupId][u:Group name] ] LS->WS->MS
void cb_M_NT_FRIEND_GROUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		changed_sn;

	_msgin.serial(FID, changed_sn);
	if (changed_sn == 0)
		return;

	// Read FriendGroupList
	FriendGroupList	data;

	uint8	count;
	_msgin.serial(count);
	for (uint8 i = 0; i < count; i ++)
	{
		_FriendGroupInfo	data1;

		ucstring	GroupName;
		_msgin.serial(data1.GroupID, GroupName);
		wcscpy(data1.GroupName, GroupName.c_str());

		data.Datas[data.DataCount] = data1;
		data.DataCount ++;
	}

	// Set FriendGroupList
	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		_FamilyFriendListInfo	dataAll(changed_sn);
		dataAll.m_FriendGroups = data;

		g_FamilyFriends.insert(make_pair(FID, dataAll));
	}
	else
	{
		it->second.m_Changed_SN = changed_sn;
		it->second.m_FriendGroups = data;

		it->second.m_Friends.clear();
	}
}

void DoAfterReadFriendInfo(T_FAMILYID FID)
{
	Player *pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipinfo("pPlayer = NULL. FID=%d", FID);
		return;
	}

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find(FID) failed. FID=%d", FID);
		return;
	}

	// M_SC_CONTSTATE를 보낸다.
const int MAX_FRIEND_STATUS_SEND_COUNT = 100;
	uint32	zero = 0;
	int count = 0;
	CMessage	msgOut(M_SC_CONTSTATE);
	msgOut.serial(FID, zero);
	for (std::map<T_FAMILYID, _FriendInfo>::iterator it1 = it->second.m_Friends.begin(); it1 != it->second.m_Friends.end(); it1 ++)
	{
		if (it1->second.GroupID == (uint32)(-1))
			continue;

		T_FAMILYID	friendFID = it1->first;
		Player *pFriendPlayer = GetPlayer(friendFID);
		if (pFriendPlayer == NULL)
			continue;
		if (!IsInFriendList(friendFID, FID))
			continue;

		uint8	state = CONTACTSTATE_ONLINE | (pFriendPlayer->m_bIsMobile << 4);
		msgOut.serial(friendFID, state, pFriendPlayer->m_SessionState);
		count ++;

		if (count >= MAX_FRIEND_STATUS_SEND_COUNT)
		{
			CUnifiedNetwork::getInstance()->send(TServiceId(pPlayer->m_FSSid), msgOut);

			CMessage	msgOut1(M_SC_CONTSTATE);
			msgOut1.serial(FID, zero);
			msgOut = msgOut1;

			count = 0;
		}
	}
	if (count > 0)
	{
		CUnifiedNetwork::getInstance()->send(TServiceId(pPlayer->m_FSSid), msgOut);
	}

	// M_SC_CONTSTATE를 자기 친구들에게 보낸다.
	pPlayer->NotifyOnOffStatusToFriends(CONTACTSTATE_ONLINE);

	//// Chit정보를 보낸다. - Main으로 이동
	//{
	//	MAKEQUERY_LOADCHITLISTTOHIM(strQ, FID);
	//	DBCaller->executeWithParam(strQ, DBCB_DBLoadChitForSend,
	//		"D",
	//		CB_,		FID);
	//}
}

// Send Friend List [d:FID][d:SN][b:Count][b:finished][ [d:GroupId][d:FamilyId][d:Index] ] LS->WS->MS
void cb_M_NT_FRIEND_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		changed_sn;

	_msgin.serial(FID, changed_sn);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		sipwarning("g_FamilyFriends.find(FID) failed. FID=%d, changed_sn=%d", FID, changed_sn);
		return;
	}

	_FamilyFriendListInfo	&dataAll = it->second;

	// Read Data from Packet
	uint8	finished = 1;
	if (changed_sn != 0)
	{
		uint8	count;
		_msgin.serial(count, finished);

		for (uint8 i = 0; i < count; i ++)
		{
			_FriendInfo	data1;
			uint32		friendFID;
			_msgin.serial(data1.GroupID, friendFID, data1.Index);

			dataAll.m_Friends.insert(make_pair(friendFID, data1));
		}
	}

	if (finished == 0)
		return;

	// Receive finished, Do next work.
	for (std::list<uint64>::iterator it1 = g_WaitFriendInfoList.begin(); it1 != g_WaitFriendInfoList.end(); )
	{
		uint64	key = *it1;
		uint32	waitFID = (uint32)key;
		uint32	waitRoomID = (uint32)(key >> 32);

		if (waitFID != FID)
		{
			it1 ++;
			continue;
		}

		it1 = g_WaitFriendInfoList.erase(it1);

		if (waitRoomID != 0)
		{
			DoAfterRoomCreated(waitRoomID);
		}
		else
		{
			DoAfterReadFriendInfo(FID);
		}
	}
}

// Send Friend info's Changed_SN. Response of chaned friend info's packets ([d:FID][d:SN][b:flag, 0-Own, 1-Other(Main)]) LS->WS->MS
void cb_M_NT_FRIEND_SN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		changed_sn;
	uint8		flag;

	_msgin.serial(FID, changed_sn, flag);

	std::map<T_FAMILYID, _FamilyFriendListInfo>::iterator it = g_FamilyFriends.find(FID);
	if (it == g_FamilyFriends.end())
	{
		if (flag == 0)
			sipwarning("g_FamilyFriends.find = NULL. FID=%d, sn=%d", FID, changed_sn);
	}
	else
	{
		if (flag == 0)
			it->second.m_Changed_SN = changed_sn;
		else
		{
			if (it->second.m_Changed_SN != changed_sn)
				RequestFriendListInfo(FID, 0);
		}
	}
}

static void	DBCB_DBFriendDeleted(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];
	T_FAMILYID		FriendFID	= *(T_FAMILYID *)argv[1];

	uint32 nRows;	resSet->serial(nRows);
	ucstring	p4 = L"";
	for (uint32 i = 0; i < nRows; i ++)
	{
		uint32		RoomID;
		ucstring	RoomName;
		resSet->serial(RoomID);
		resSet->serial(RoomName);
		DeleteRoomManagerInRoomWorldByRemoveFriendList(FID, FriendFID, RoomID, RoomName);
	}
}

// Send deleted friend for remove room's manager ([d:FID][d:friendFID]) LS->WS->MS
void cb_M_NT_FRIEND_DELETED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID, friendFID;

	_msgin.serial(FID, friendFID);

	MAKEQUERY_FriendDeleted(strQuery, FID, friendFID);
	DBCaller->executeWithParam(strQuery, DBCB_DBFriendDeleted,
		"DD",
		CB_,	FID,
		CB_,	friendFID);
}

static void	DBCB_GetFamilyAllInfoForLogin_MS(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	T_FAMILYID		FID	= *(T_FAMILYID *)argv[0];
	TServiceId		tsid(*(uint16*)argv[1]);

	std::map<T_FAMILYID, Player>::iterator it = g_Players.find(FID);
	uint32			UserID = 0;
	if (it == g_Players.end())
	{
		sipwarning("g_Players.find failed. FID=%d", FID);
		return;
	}
	UserID = it->second.m_UserID;

	// Own Room Info
	{
		uint8	bStart, bEnd;
		CMessage	msgOut(M_SC_OWNROOM);
		bStart = 1;
		msgOut.serial(FID, bStart);

		uint32	nCurtime = (uint32)(GetDBTimeSecond() / 60);

		uint16	nCount = 0;
		uint32 nRows;	resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			// Packet??湲몄씠媛 ?덈Т 湲몄뼱???섎씪??蹂대궦??
			if (msgOut.length() > 2500)
			{
				bEnd = 0;
				msgOut.serial(bEnd);
				CUnifiedNetwork::getInstance()->send(tsid, msgOut);

				CMessage	msgtmp(M_SC_OWNROOM);
				bStart = 0;
				msgtmp.serial(FID, bStart);

				msgOut = msgtmp;
			}

			uint32			curGiveId;				resSet->serial(curGiveId);
			T_DATETIME		curGiveTime;			resSet->serial(curGiveTime);
			T_MSTROOM_ID	SceneID;				resSet->serial(SceneID);
			T_DATETIME		creattime;				resSet->serial(creattime);
			T_DATETIME		termtime;				resSet->serial(termtime);
			T_ROOMID		id;						resSet->serial(id);
			T_ROOMNAME		roomName;				resSet->serial(roomName);
			T_ROOMPOWERID	opentype;				resSet->serial(opentype);
			string			openPwd;				resSet->serial(openPwd);
			bool			freeuse;				resSet->serial(freeuse);
			bool			bclosed;				resSet->serial(bclosed);
			T_DATETIME			renewtime;			resSet->serial(renewtime);
			T_COMMON_CONTENT	comment;			resSet->serial(comment);
			uint32				visits;				resSet->serial(visits);
			uint8				deleteflag;			resSet->serial(deleteflag);
			T_DATETIME			deletetime;			resSet->serial(deletetime);
			uint32		spacecapacity;				resSet->serial(spacecapacity);
			uint32		spaceuse;					resSet->serial(spaceuse);
			T_ROOMLEVEL	level;						resSet->serial(level);
			T_ROOMEXP	exp;						resSet->serial(exp);
			uint8				DeadSurnameLen1;	resSet->serial(DeadSurnameLen1);
			T_DECEASED_NAME		Deadname1;			resSet->serial(Deadname1);
			uint32		DeadBirthday1;				resSet->serial(DeadBirthday1);
			uint32		DeadDeadday1;				resSet->serial(DeadDeadday1);
			uint8				DeadSurnameLen2;	resSet->serial(DeadSurnameLen2);
			T_DECEASED_NAME		Deadname2;			resSet->serial(Deadname2);
			uint32		DeadBirthday2;				resSet->serial(DeadBirthday2);
			uint32		DeadDeadday2;				resSet->serial(DeadDeadday2);
			uint8		_SamePromotionCount;		resSet->serial(_SamePromotionCount);
			uint8		PhotoType;					resSet->serial(PhotoType);

			uint32	nCreateTime = getMinutesSince1970(creattime);
			uint32	nRenewTime = getMinutesSince1970(renewtime);
			uint32	nTermTime = getMinutesSince1970(termtime);
			uint32	nDeleteRemainMinutes = 0;
			if (deleteflag)
			{
				// deleteflag: 1 - ??젣以??뚮났媛?? 2 - ?꾩쟾??젣??
				if (deleteflag != 1)
					continue;

				uint32 nDeleteTime = getMinutesSince1970(deletetime);
				if (nCurtime >= nDeleteTime)
					continue;
				nDeleteRemainMinutes = nDeleteTime - nCurtime;
			}
			uint32 nCurGiveTime = 0;
			if (curGiveId != 0)
				nCurGiveTime = getMinutesSince1970(curGiveTime);

			if (Deadname1 == L"")
				DeadSurnameLen1 = 0;
			if (Deadname2 == L"")
				DeadSurnameLen2 = 0;

			//uint16	exp_percent = CalcRoomExpPercent(level, exp);

			//		T_FLAG	uState = bclosed ? ROOMSTATE_CLOSED : ROOMSTATE_NORMAL;

			msgOut.serial(id, SceneID, nCreateTime, nTermTime, nRenewTime);
			msgOut.serial(roomName, opentype, openPwd, freeuse, visits, comment, nDeleteRemainMinutes);
			msgOut.serial(spacecapacity, spaceuse, level, exp/*, exp_percent*/);
			msgOut.serial(curGiveId, nCurGiveTime, PhotoType);
			msgOut.serial(Deadname1, DeadSurnameLen1, DeadBirthday1, DeadDeadday1);
			msgOut.serial(Deadname2, DeadSurnameLen2, DeadBirthday2, DeadDeadday2);
			uint32		SamePromotionCount = (uint32)_SamePromotionCount;
			msgOut.serial(SamePromotionCount);

			nCount++;
		}

		bEnd = 1;
		msgOut.serial(bEnd);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		//sipinfo(L"-> [M_SC_OWNROOM] Request My Own Creating Room num : %u, size=%d", nCount, msgOut.length()); byy krs
	}

	// Festival Info
	{
		uint8	bStart, bEnd;
		bStart = 1;
		CMessage msgOut(M_SC_ALLFESTIVAL);
		msgOut.serial(FID, bStart);

		uint32 nRows;	resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			// Packet??湲몄씠媛 ?덈Т 湲몄뼱???섎씪??蹂대궦??
			if(msgOut.length() > 2500)
			{
				bEnd = 0;
				msgOut.serial(bEnd);
				CUnifiedNetwork::getInstance()->send(tsid, msgOut);

				CMessage	msgtmp(M_SC_ALLFESTIVAL);
				bStart = 0;
				msgtmp.serial(FID, bStart);

				msgOut = msgtmp;
			}

			uint32	idFestival;			resSet->serial(idFestival);
			T_NAME	ucName;				resSet->serial(ucName);
			uint32	date;				resSet->serial(date);

			msgOut.serial(idFestival);
			msgOut.serial(date);
			msgOut.serial(ucName);
		}

		bEnd = 1;
		msgOut.serial(bEnd);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}

	{
		CMessage msgOut(M_SC_ALL_FESTIVALALARM);
		msgOut.serial(FID);
		uint32 lenth = msgOut.length();

		uint32 nRows;	resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint8		festivalType;		resSet->serial(festivalType);
			uint32		festiveId;			resSet->serial(festiveId);
			uint8		remindDays;			resSet->serial(remindDays);

			msgOut.serial(festivalType);
			msgOut.serial(festiveId);
			msgOut.serial(remindDays);
		}

		if (msgOut.length() > lenth)
		{
			CUnifiedNetwork::getInstance ()->send(tsid, msgOut);
			//sipinfo(L"-> [M_SC_ALL_FESTIVALALARM] Send User %d Festival remind days", FID); byy krs
		}
	}

	// ?묐룄諛쏅뒗 諛⑸ぉ濡?
	{
		uint32 nRows;	resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			T_ROOMID		RoomID;		resSet->serial(RoomID);
			T_SCENEID		SceneID;	resSet->serial(SceneID);
			T_COMMON_NAME	RoomName;	resSet->serial(RoomName);
			T_FAMILYID		OwnerID;	resSet->serial(OwnerID);
			T_FAMILYNAME	OwnerName;	resSet->serial(OwnerName);
			T_DATETIME		curGiveTime;resSet->serial(curGiveTime);
			uint32 nCurGiveTime = getMinutesSince1970(curGiveTime);

			CMessage	msgOut(M_SC_CHANGEMASTER_REQ);
			msgOut.serial(FID, RoomID, SceneID, RoomName, OwnerID, OwnerName, nCurGiveTime);
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		}
	}

	{
		uint32 nRows;	resSet->serial(nRows);
		T_DATETIME		LastDays;		resSet->serial(LastDays);
		uint8			nCheckDays;		resSet->serial(nCheckDays);

		SIPBASE::MSSQLTIME tempTime;
		tempTime.SetTime(LastDays);
		uint32	nLastCheckinDay = tempTime.GetDaysFrom1970();

		uint32		today = GetDBTimeDays();
		sint8		nNextCheckinDays = GetNextCheckDays(today, nCheckDays, nLastCheckinDay);
		
		sint8		nNowCheckInstate;
		if (nNextCheckinDays != CIS_CANT_CHECK)
		{
			if (nNextCheckinDays == 0)
				nNowCheckInstate = CIS_MAX_DAYS - 1;
			else
				nNowCheckInstate = nNextCheckinDays - 1;
		}
		else
		{
			nNowCheckInstate = CIS_CANT_CHECK;
		}

		CMessage msgOut(M_SC_CHECKINSTATE);
		msgOut.serial(FID, nNowCheckInstate);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}

	{
		SendCheckUserActivity(UserID, FID);
		SendCheckBeginnerMstItem(UserID, FID);
	}
}

// 濡쒓렇?명썑 ?쒕쾲留?諛쏆븘???섎뒗 ?먮즺瑜?蹂대궡以꾧쾬???붿껌?쒕떎.
void cb_M_CS_REQ_PRIMARY_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;			_msgin.serial(FID);

	MAKEQUERY_GetFamilyAllInfoForLogin_MS(strQuery, FID);
	DBCaller->executeWithParam(strQuery, DBCB_GetFamilyAllInfoForLogin_MS,
		"DW",
		CB_,	FID,
		CB_,	_tsid.get());
}



SIPBASE_CATEGORISED_COMMAND(MS, recalcLuck4ForTest, "Recal luck4 for test", "")
{
	uint32 todayLuck4Count = GetLuckLv4CountToday();
	if (todayLuck4Count < MAX_LUCK_LV4_COUNT)
	{
		RecalLuck4Data(MAX_LUCK_LV4_COUNT-todayLuck4Count, true);
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(MS, recalcLuck4, "Recal luck4", "")
{
	uint32 todayLuck4Count = GetLuckLv4CountToday();
	if (todayLuck4Count < MAX_LUCK_LV4_COUNT)
	{
		RecalLuck4Data(MAX_LUCK_LV4_COUNT-todayLuck4Count);
	}
	return true;
}

SIPBASE_CATEGORISED_COMMAND(MS, viewLuck4Plan, "view luck4 plan", "")
{
	std::list<LUCK4DATA>::iterator it;
	for (it = todayLuck4Data.begin(); it != todayLuck4Data.end(); it++)
	{
		string plantime = GetTodayTimeStringFromDBSecond(it->planDBTimeSecond);
		if (it->playDBTimeSecond != 0)
		{
			string playtime = GetTodayTimeStringFromDBSecond(it->playDBTimeSecond);
			sipdebug ("plantime=%s, playtime=%s, FID=%d, RoomID=%d", plantime.c_str(), playtime.c_str(), it->FID, it->RoomID);
		}
		else
		{
			sipdebug ("plantime=%s", plantime.c_str());
		}
	}
	return true;
}
