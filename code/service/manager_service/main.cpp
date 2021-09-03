
#include <misc/types_sip.h>
#include "misc/DBCaller.h"
#include <net/service.h>

#include "../../common/Macro.h"
#include "../../common/Packet.h"
#include "../Common/Common.h"
#include "../Common/netCommon.h"
#include "../Common/QuerySetForShard.h"
#include "../Common/Lang.h"
#include "../frontend/MultiService.h"
#include "../openroom/mst_room.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

#include "main.h"
#include "AutoPlayer.h"
#include "player.h"
#include "zaixiankefu.h"
#include "silence.h"
#include "toplist.h"

#include <list>

CDBCaller		*DBCaller = NULL;
//CDBCaller		*MainDBCaller = NULL;

CMultiService<uint32>	OpenRoomService(OROOM_SHORT_NAME);

// Common Functions
SIPBASE::MSSQLTIME		ServerStartTime_DB;
SIPBASE::TTime			ServerStartTime_Local;
void InitTime()
{
	// Get current time from server
	T_DATETIME		StartTime_DB = "";
	{
		MAKEQUERY_GETCURRENTTIME(strQ);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY(DB, Stmt, strQ);
		if (nQueryResult == true)
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (IS_DB_ROW_VALID(row))
			{
				COLUMN_DATETIME(row, 0, curtime);
				StartTime_DB = curtime;			

				sipinfo("DBTime = %s", StartTime_DB.c_str());
			}
		}
	}

	if (StartTime_DB.length() < 1)
	{
		siperrornoex("Can't get current time of database server");
		return;
	}
	ServerStartTime_DB.SetTime(StartTime_DB);
	ServerStartTime_Local = CTime::getSecondsSince1970();
}

TTime GetDBTimeSecond()
{
	TTime	workingtime = (CTime::getSecondsSince1970() - ServerStartTime_Local);
	TTime	curtime = ServerStartTime_DB.timevalue + workingtime;

	return curtime;
}

uint32	GetDBTimeMinute()
{
	TTime	workingtime = (CTime::getSecondsSince1970() - ServerStartTime_Local);
	TTime	curtime = ServerStartTime_DB.timevalue + workingtime;

	return (uint32)(curtime / 60);
}

uint32	GetDBTimeDays()
{
	TTime	workingtime = (CTime::getSecondsSince1970() - ServerStartTime_Local);
	TTime	curtime = ServerStartTime_DB.timevalue + workingtime;

	return (uint32)(curtime / SECOND_PERONEDAY);
	
}

uint32	getMinutesSince1970(std::string& date)
{
	MSSQLTIME	t;
	if (!t.SetTime(date))
		return 0;
	uint32	result = (uint32)(t.timevalue / 60);
	return result;
}

std::string GetTodayTimeStringFromDBSecond(TTime tmCur)
{
	int nDeltaTime = tmCur  % SECOND_PERONEDAY;
	int nHour = nDeltaTime/3600;
	int nMinute = (nDeltaTime - nHour * 3600) / 60;
	int nSecond = nDeltaTime - nHour * 3600 - nMinute * 60;
	char chTime[32];	memset(chTime, 0, sizeof(chTime));
	sprintf(chTime, "%02d:%02d:%02d", nHour, nMinute, nSecond);
	string strTime = chTime;
	return strTime;
}

void OnPacketCallback_One(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
void OnPacketCallback_List(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);

// For Player
void cb_M_NT_LOGIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
void cb_M_NT_LOGOUT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
void cb_M_NT_PLAYER_ENTERROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
void cb_M_NT_PLAYER_LEAVEROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
//void cb_M_NT_PLAYER_CHANGECHANNEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);

// For TopList
//void cb_M_CS_TOPLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//void cb_M_CS_EXPROOMS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_GMCS_EXPROOM_GROUP_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_GMCS_EXPROOM_GROUP_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_GMCS_EXPROOM_GROUP_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_GMCS_EXPROOM_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_GMCS_EXPROOM_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_GMCS_EXPROOM_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_EXPROOM_INFO_CHANGED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_EXPROOM_INFO_RELOAD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_BESTSELLITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// For Room
void CreateReligionRooms();
void cb_M_NT_ROOM_CREATED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_ROOM_DELETED(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_REQEROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_CANCELREQEROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_CHECK_ENTERROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_CHANGEMASTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_CHANGEMASTER_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_CHANGEMASTER_ANS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void LoadAuditErrRooms();

//void DeleteNonusedFreeRooms();

// For Player's Favorite Room
//void cb_M_CS_INSFAVORROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_DELFAVORROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_MOVFAVORROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_NEW_FAVORROOMGROUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_DEL_FAVORROOMGROUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_REN_FAVORROOMGROUP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//void cb_M_CS_REQ_ROOMORDERINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_ROOMORDERINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// 사용자형상사진
void cb_M_CS_SET_FAMILY_FIGURE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_GET_FAMILY_FIGURE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_SET_FAMILY_FACEMODEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_GET_FAMILY_FACEMODEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// 기념일활동관련
void cb_M_NT_CURRENT_ACTIVITY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_MS_CHECK_USER_ACTIVITY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_REQ_ACTIVITY_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_BEGINNERMSTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_MS_CHECK_BEGINNERMSTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_CS_RECV_GIFTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void cb_M_NT_CURRENT_CHECKIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_REQCHECKIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// 축복카드기능
void cb_M_NT_LUCK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_NT_LUCK_RESET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_BLESSCARD_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_BLESSCARD_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_LARGEACTPRIZE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// Luck4목록
//void cb_M_CS_LUCK4_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// 사용후기
//void LoadReviewList();
//void cb_M_CS_REVIEW_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_GMCS_REVIEW_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_GMCS_REVIEW_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//void cb_M_GMCS_REVIEW_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void cb_M_NT_REQGIVEBLESSCARD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_CURLUCKINPUBROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

// for Promotion Room
extern	void	cb_M_CS_PROMOTION_ROOM_Manager(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern	void	cb_M_CS_ROOMCARD_PROMOTION_Manager(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern void cb_M_CS_RESCHIT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern	void	cb_M_WM_SHARDCODELIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern	void	cb_M_W_SHARDCODE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

void cb_M_CS_REQ_PRIMARY_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

TUnifiedCallbackItem CallbackArray[] =
{
	// For Autoplay
	{ M_NT_AUTOPLAY_REGISTER,	cb_M_NT_AUTOPLAY_REGISTER },
	{ M_NT_ADDACTIONMEMO_OK,	cb_M_NT_ADDACTIONMEMO_OK },
	{ M_NT_AUTOPLAY_ANS,		OnPacketCallback_One },

	// For Player
	{ M_NT_LOGIN,					cb_M_NT_LOGIN },
	{ M_NT_LOGOUT,					cb_M_NT_LOGOUT },
	{ M_NT_PLAYER_ENTERROOM,		cb_M_NT_PLAYER_ENTERROOM },
	{ M_NT_PLAYER_LEAVEROOM,		cb_M_NT_PLAYER_LEAVEROOM },
	//{ M_NT_PLAYER_CHANGECHANNEL,	cb_M_NT_PLAYER_CHANGECHANNEL },
	{ M_NT_ZAIXIANGM_LOGON,			cb_M_NT_ZAIXIANGM_LOGON },

	// For TopList
	//{ M_CS_TOPLIST,					cb_M_CS_TOPLIST },
	//{ M_CS_EXPROOMS,				cb_M_CS_EXPROOMS },
	{ M_GMCS_EXPROOM_GROUP_ADD,		cb_M_GMCS_EXPROOM_GROUP_ADD },
	{ M_GMCS_EXPROOM_GROUP_MODIFY,	cb_M_GMCS_EXPROOM_GROUP_MODIFY },
	{ M_GMCS_EXPROOM_GROUP_DELETE,	cb_M_GMCS_EXPROOM_GROUP_DELETE },
	{ M_GMCS_EXPROOM_ADD,			cb_M_GMCS_EXPROOM_ADD },
	{ M_GMCS_EXPROOM_MODIFY,		cb_M_GMCS_EXPROOM_MODIFY },
	{ M_GMCS_EXPROOM_DELETE,		cb_M_GMCS_EXPROOM_DELETE },
	{ M_NT_EXPROOM_INFO_CHANGED,	cb_M_NT_EXPROOM_INFO_CHANGED },
	{ M_NT_EXPROOM_INFO_RELOAD,		cb_M_NT_EXPROOM_INFO_RELOAD },
	{ M_GMCS_SILENCE_SET,			cb_M_GMCS_SILENCE_SET },
	//{ M_CS_BESTSELLITEM,			cb_M_CS_BESTSELLITEM },

	// For Room
	{ M_NT_ROOM_CREATED,			cb_M_NT_ROOM_CREATED },
	{ M_NT_ROOM_DELETED,			cb_M_NT_ROOM_DELETED },
	{ M_CS_REQEROOM,				cb_M_CS_REQEROOM },
	{ M_CS_CANCELREQEROOM,			cb_M_CS_CANCELREQEROOM },
	
	{ M_CS_CHECK_ENTERROOM,			cb_M_CS_CHECK_ENTERROOM },
	{ M_CS_CHANGEMASTER	,			cb_M_CS_CHANGEMASTER },
	{ M_CS_CHANGEMASTER_CANCEL	,	cb_M_CS_CHANGEMASTER_CANCEL },
	{ M_CS_CHANGEMASTER_ANS	,		cb_M_CS_CHANGEMASTER_ANS },

	// For Player's Favorite Room
	//{ M_CS_INSFAVORROOM,		cb_M_CS_INSFAVORROOM },
	//{ M_CS_DELFAVORROOM,		cb_M_CS_DELFAVORROOM },
	//{ M_CS_MOVFAVORROOM,		cb_M_CS_MOVFAVORROOM },
	//{ M_CS_NEW_FAVORROOMGROUP,	cb_M_CS_NEW_FAVORROOMGROUP },
	//{ M_CS_DEL_FAVORROOMGROUP,	cb_M_CS_DEL_FAVORROOMGROUP },
	//{ M_CS_REN_FAVORROOMGROUP,	cb_M_CS_REN_FAVORROOMGROUP },

	{ M_CS_ROOMORDERINFO,		cb_M_CS_ROOMORDERINFO },
	//{ M_CS_REQ_ROOMORDERINFO,	cb_M_CS_REQ_ROOMORDERINFO },

	// 사용자형상사진
	{ M_CS_SET_FAMILY_FIGURE,	cb_M_CS_SET_FAMILY_FIGURE },
	{ M_CS_GET_FAMILY_FIGURE,	cb_M_CS_GET_FAMILY_FIGURE },
	//{ M_CS_SET_FAMILY_FACEMODEL,cb_M_CS_SET_FAMILY_FACEMODEL },
	//{ M_CS_GET_FAMILY_FACEMODEL,cb_M_CS_GET_FAMILY_FACEMODEL },

	// 기념일활동관련
	{ M_NT_CURRENT_ACTIVITY,	cb_M_NT_CURRENT_ACTIVITY },
	{ M_MS_CHECK_USER_ACTIVITY,	cb_M_MS_CHECK_USER_ACTIVITY },
	//{ M_CS_REQ_ACTIVITY_ITEM,	cb_M_CS_REQ_ACTIVITY_ITEM },
	{ M_NT_BEGINNERMSTITEM,		cb_M_NT_BEGINNERMSTITEM },
	{ M_MS_CHECK_BEGINNERMSTITEM, cb_M_MS_CHECK_BEGINNERMSTITEM },
	//{ M_CS_RECV_GIFTITEM,		cb_M_CS_RECV_GIFTITEM },
	// 축복카드기능
	{ M_NT_LUCK,				cb_M_NT_LUCK },
	//{ M_NT_LUCK_RESET,			cb_M_NT_LUCK_RESET },
//	{ M_NT_LUCK4_REQ,			cb_M_NT_LUCK4_REQ },
	{ M_CS_BLESSCARD_GET,		cb_M_CS_BLESSCARD_GET },
	{ M_CS_BLESSCARD_SEND,		cb_M_CS_BLESSCARD_SEND },
	{ M_NT_LARGEACTPRIZE,		cb_M_NT_LARGEACTPRIZE },
	// Luck4목록
	//{ M_CS_LUCK4_LIST,			cb_M_CS_LUCK4_LIST },

	// 사용후기관리
	//{ M_CS_REVIEW_LIST,			cb_M_CS_REVIEW_LIST },
	//{ M_GMCS_REVIEW_ADD,		cb_M_GMCS_REVIEW_ADD },
	//{ M_GMCS_REVIEW_MODIFY,		cb_M_GMCS_REVIEW_MODIFY },
	//{ M_GMCS_REVIEW_DELETE,		cb_M_GMCS_REVIEW_DELETE },

	{ M_CS_ZAIXIAN_REQUEST,		cb_M_CS_ZAIXIAN_REQUEST },
	{ M_CS_ZAIXIAN_END,			cb_M_CS_ZAIXIAN_END },
	{ M_CS_ZAIXIAN_ASK,			cb_M_CS_ZAIXIAN_ASK },
	{ M_GMCS_ZAIXIAN_ANSWER,	cb_M_GMCS_ZAIXIAN_ANSWER },
	{ M_GMCS_ZAIXIAN_CONTROL,	cb_M_GMCS_ZAIXIAN_CONTROL },

	{ M_NT_CURRENT_CHECKIN,		cb_M_NT_CURRENT_CHECKIN },
	{ M_NT_REQCHECKIN,			cb_M_NT_REQCHECKIN },
	{ M_NT_REQGIVEBLESSCARD,	cb_M_NT_REQGIVEBLESSCARD },
	{ M_NT_CURLUCKINPUBROOM,	cb_M_NT_CURLUCKINPUBROOM },

	{ M_CS_PROMOTION_ROOM,		cb_M_CS_PROMOTION_ROOM_Manager },
	{ M_CS_ROOMCARD_PROMOTION,	cb_M_CS_PROMOTION_ROOM_Manager },

	//{ M_CS_RESCHIT,				cb_M_CS_RESCHIT },

	// 
	//{ M_WM_SHARDCODELIST,		cb_M_WM_SHARDCODELIST },

	{ M_W_SHARDCODE,			cb_M_W_SHARDCODE },

	// For Contact
	{ M_CS_ADDCONTGROUP,		cb_M_CS_ADDCONTGROUP },
	{ M_CS_DELCONTGROUP,		cb_M_CS_DELCONTGROUP },
	{ M_CS_RENCONTGROUP,		cb_M_CS_RENCONTGROUP },
	{ M_CS_ADDCONT,				cb_M_CS_ADDCONT },
	{ M_CS_DELCONT,				cb_M_CS_DELCONT },	
	{ M_CS_CHANGECONTGRP,		cb_M_CS_CHANGECONTGRP },
	{ M_CS_SETCONTGROUP_INDEX,  cb_M_CS_SETCONTGROUP_INDEX },
	{ M_CS_SELFSTATE,			cb_M_CS_SELFSTATE },

	{ M_CS_INVITE_REQ,			cb_M_CS_INVITE_REQ },

	{ M_NT_FRIEND_GROUP,		cb_M_NT_FRIEND_GROUP },
	{ M_NT_FRIEND_LIST,			cb_M_NT_FRIEND_LIST },
	{ M_NT_FRIEND_SN,			cb_M_NT_FRIEND_SN },
	{ M_NT_FRIEND_DELETED,		cb_M_NT_FRIEND_DELETED },
	{ M_NT_ANS_FAMILY_INFO,		cb_M_NT_ANS_FAMILY_INFO },

	{ M_CS_REQ_PRIMARY_DATA,	cb_M_CS_REQ_PRIMARY_DATA },

	// 예약의식
	{ M_NT_AUTOPLAY3_START_REQ,	cb_M_NT_AUTOPLAY3_START_REQ },
	{ M_NT_AUTOPLAY3_EXP_ADD_ONLINE,	cb_M_NT_AUTOPLAY3_EXP_ADD_ONLINE },
	{ M_NT_AUTOPLAY3_EXP_ADD_OFFLINE,	cb_M_NT_AUTOPLAY3_EXP_ADD_OFFLINE },
};

void SendMsgToOpenroomForCreate(T_ROOMID RoomID, CMessage &msgOut)
{
	TServiceId sid;
	sid = OpenRoomService.GetNewServiceIDForRoomID(RoomID);

	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

extern void SendLuckHistoryToOpenroom(SIPNET::TServiceId _tsid);
void onReconnectOpenRoom(const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	OpenRoomService.AddService(_tsid);
	SendLuckHistoryToOpenroom(_tsid);
}

void onDisconnectOpenRoom(const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	OpenRoomService.RemoveService(_tsid);
}

void onReconnectReligionRoom(const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	CreateReligionRooms();
}

void onReconnectFrontend(const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	SendSilenceListToFS(_tsid);
}

void onWSConnection(const std::string &serviceName, TServiceId tsid, void *arg)
{
	// Get ShardCode
	CMessage	msgout(M_W_REQSHARDCODE);
	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

TServiceId	lobbySidForAutoplay3 = TServiceId(0);
void onLobbyConnection(const std::string &serviceName, TServiceId tsid, void *arg)
{
	lobbySidForAutoplay3 = tsid;
}


void DoEverydayWork()
{
	LoadTopList();

	LoadAuditErrRooms();

	SaveAndRefreshLuckHistory();

	RecalLuck4Data(MAX_LUCK_LV4_COUNT);
	ResetLuck4List();

	// 30일 이전의 방문록자료들을 삭제한다. by krs
	{
		MAKEQUERY_Delete30DayDatas(strQuery);
		CDBConnectionRest	DBConnection(DBCaller);
		DB_EXE_QUERY(DBConnection, Stmt, strQuery);
	}

	// 30일 이전의 종교방방문록과 소원자료를 삭제한다. by krs
	{
		uint32 curTime = (uint32)GetDBTimeSecond();
		MAKEQUERY_Delete30DayReligionDatas(strQuery, curTime);
		CDBConnectionRest	DBConnection(DBCaller);
		DB_EXE_QUERY(DBConnection, Stmt, strQuery);
	}
}

TTime GetUpdateNightTime(TTime curTime)
{
	TTime	ret = ((TTime)(curTime / SECOND_PERONEDAY)) * (TTime)SECOND_PERONEDAY + 0;	// 밤12시 0분;
	if (ret <= curTime)
		ret += SECOND_PERONEDAY;
	return ret;
}

static TTime	NextUpdateJingPinTianYuanTime = 0;
void CheckTimer()
{
	// 밤12시 검사
	TTime	curTime = GetDBTimeSecond();

	if (NextUpdateJingPinTianYuanTime == 0)
	{
		NextUpdateJingPinTianYuanTime = GetUpdateNightTime(curTime);
		return;
	}
	if (NextUpdateJingPinTianYuanTime < curTime)
	{
		DoEverydayWork();

		NextUpdateJingPinTianYuanTime = GetUpdateNightTime(curTime);

		return;
	}
}

void SendChit(uint32 srcFID, uint32 targetFID, uint8 ChitType, uint32 p1, uint32 p2, ucstring &p3, ucstring &p4)
{
	Player	*pPlayer = GetPlayer(srcFID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", srcFID);
		return;
	}

	Player	*ptargetPlayer = GetPlayer(targetFID);
	if (ptargetPlayer != NULL && ptargetPlayer->m_FSSid.get() != 0)
	{
		uint16		r_sid = SIPNET::IService::getInstance()->getServiceId().get();
		CMessage	msgOut(M_SC_CHITLIST);
		msgOut.serial(targetFID, r_sid);
		uint32		nChit = 1;							msgOut.serial(nChit);
		uint32		chitID = 0;							msgOut.serial(chitID);
		msgOut.serial(srcFID, pPlayer->m_FName, ChitType);
		msgOut.serial(p1, p2, p3, p4);

		CUnifiedNetwork::getInstance()->send(ptargetPlayer->m_FSSid, msgOut);
	}
	else
	{
		uint8	ChitAddType = CHITADDTYPE_ADD_IF_OFFLINE;
		CMessage	msgOut(M_NT_ADDCHIT);
		msgOut.serial(ChitAddType, srcFID, pPlayer->m_FName, targetFID, ChitType, p1, p2, p3, p4);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}
}

/****************************************************************************
 * CManagerService
 ****************************************************************************/
class CManagerService : public IService
{
public:
	// Initialisation
	void init()
	{
		// Lang data
		{
			if (!LoadLangData(CPath::getExePathW()))
			{
				siperrornoex("LoadLangData failed.");
				return;
			}
		}

		{
			//if (!LoadXianbaoEffectModelName(CPath::getExePathW()))
			//{
			//	siperrornoex("LoadXianbaoEffectModelName failed.");
			//	return;
			//}
			if (!LoadBlessCardID(CPath::getExePathW()))
			{
				siperrornoex("LoadBlessCardID failed.");
				return;
			}
			if (!LoadTaocanForAutoplay3(CPath::getExePathW()))
			{
				siperrornoex("LoadTaocanForAutoplay3 failed.");
				return;
			}
		}

		string strDatabaseName = IService::getInstance ()->ConfigFile.getVar("Shard_DBName").asString ();
		string strDatabaseHost = IService::getInstance ()->ConfigFile.getVar("Shard_DBHost").asString ();
		string strDatabaseLogin = IService::getInstance ()->ConfigFile.getVar("Shard_DBLogin").asString ();
		string strDatabasePassword = IService::getInstance ()->ConfigFile.getVar("Shard_DBPassword").asString ();

		DBCaller = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
			ucstring(strDatabasePassword).c_str(),
			ucstring(strDatabaseHost).c_str(),
			ucstring(strDatabaseName).c_str());

		BOOL bSuccess = DBCaller->init(1);
		if (!bSuccess)
		{
			siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
				strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
			return;
		}
/*
		strDatabaseName = IService::getInstance()->ConfigFile.getVar("Main_DBName").asString();
		strDatabaseHost = IService::getInstance()->ConfigFile.getVar("Main_DBHost").asString();
		strDatabaseLogin = IService::getInstance()->ConfigFile.getVar("Main_DBLogin").asString();
		strDatabasePassword = IService::getInstance()->ConfigFile.getVar("Main_DBPassword").asString();
		
		MainDBCaller = new CDBCaller(ucstring(strDatabaseLogin).c_str(),
			ucstring(strDatabasePassword).c_str(),
			ucstring(strDatabaseHost).c_str(),
			ucstring(strDatabaseName).c_str());

		bSuccess = MainDBCaller->init(4);
		if (!bSuccess)
		{
			siperrornoex("Database connection failed to '%s' with login '%s' and database name '%s'",
				strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());
			return;
		}
*/
		sipwarning("#############  Database connection successed  to '%s' with login '%s' and database name '%s'", strDatabaseHost.c_str(), strDatabaseLogin.c_str(), strDatabaseName.c_str());

		CUnifiedNetwork::getInstance()->setServiceUpCallback(OROOM_SHORT_NAME, onReconnectOpenRoom, 0);
		CUnifiedNetwork::getInstance()->setServiceDownCallback(OROOM_SHORT_NAME, onDisconnectOpenRoom, 0);

		CUnifiedNetwork::getInstance()->setServiceUpCallback(RROOM_SHORT_NAME, onReconnectReligionRoom, 0);

		CUnifiedNetwork::getInstance()->setServiceUpCallback(FRONTEND_SHORT_NAME, onReconnectFrontend, 0);
		CUnifiedNetwork::getInstance()->setServiceUpCallback(WELCOME_SHORT_NAME, onWSConnection, NULL);

		CUnifiedNetwork::getInstance()->setServiceUpCallback(LOBBY_SHORT_NAME, onLobbyConnection, 0);

		InitTime();

		InitAutoPlayers();

		LoadDBMstData();

		LoadExpRoomList();
		LoadTopList();

		LoadBestsellItemList();

		//LoadReviewList();
		LoadLuckHistory();
		LoadBlessCardRecvHistory();

		LoadSilenceList();

		//DeleteNonusedFreeRooms();

		// 삭제된 편지들을 완전삭제한다.
		{
			MAKEQUERY_ClearPaper(strQ);
			DBCaller->execute(strQ);
		}

		//// Timeout된 메일들을 검사하여 돌려보낸다.
		//{
		//	MAKEQUERY_AutoReturnMail(strQ);
		//	DBCaller->execute(strQ);
		//}
	}

	bool update()
	{
		DBCaller->update();

		UpdateAutoPayers();

		CheckTimer();

		return true;
	}

	void release()
	{
		// SaveLuckHistory();
		SaveBlessCardRecvHistory();

		DBCaller->release();
		delete DBCaller;
		DBCaller = NULL;

		//MainDBCaller->release();
		//delete MainDBCaller;
		//MainDBCaller = NULL;
	}
};

SIPNET_SERVICE_MAIN (CManagerService, MANAGER_SHORT_NAME, MANAGER_LONG_NAME, "", 0, false, CallbackArray, "", "")

// Command
SIPBASE_CATEGORISED_COMMAND(APS, testplay, "autoplay test", "<RoomID> <ActPos> <IsXDG>")
{
	if (args.size () != 3)
		return false;

	string strRoomID = args[0];
	uint32 RoomID = atoi (strRoomID.c_str());
	string strActPos = args[1];
	uint8 ActPos = atoi (strActPos.c_str());
	string strIsXDG = args[2];
	uint32 IsXDG = atoi (strIsXDG.c_str());

	extern void testAutoplay(uint32 RoomID, uint8 ActPos, uint32 IsXDG);
	testAutoplay(RoomID, ActPos, IsXDG);

	return true;
}

SIPBASE_CATEGORISED_COMMAND(sip, changeRoomMaster, "Change room's master", "<RoomID> <NewMasterFID>")
{
	if (args.size () != 2)
		return false;

	string strRoomID = args[0];
	uint32 RoomID = atoi (strRoomID.c_str());
	string strNewFID = args[1];
	uint32 NewFID = atoi (strNewFID.c_str());

	extern void ChangeRoomMasterForce(T_ROOMID RoomID, T_FAMILYID NewFID);
	ChangeRoomMasterForce(RoomID, NewFID);

	return true;
}
