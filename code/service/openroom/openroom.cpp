#include <misc/ucstring.h>
#include <net/service.h>

#include "openroom.h"
#include "Character.h"
#include "Family.h"
#include "mst_room.h"
#include "outRoomKind.h"
#include "room.h"
#include "contact.h"
#include "Lobby.h"
#include "Inven.h"
#include "Mail.h"
#include "ChannelWorld.h"

#include "../Common/DBLog.h"
#include "../Common/netCommon.h"
#include "../Common/Lang.h"
#include "misc/DBCaller.h"
#include "ParseArgs.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller	*DBCaller;

// 상수표
#define	VALIDTIME_FOREVENTAUTOEND				600000		// 행사 자동완료시간(10분 : ms)
#define	VALIDTIME_FORTGATEMAKEDURING			3600000		// 신비한문 생성에 사용하는 기간수값(1시간 : ms)
#define ROOMGIVELIMITTIME						604800		// 양도제한시간(7일 second수)
#define ROOMWISHDELTIME							60 // 방의 소원유지시간(second)
#define BLESS_LAMP_PUT_COUNT					1 // 한방에서 길상등을 올릴수 있는 최대개수

// 레벨별 물고기자료, 
static int	fish_index_list[11][2] = {{0, 0}, {0, 1}, {1, 3}, {3, 5}, {5, 7}, {7, 9}, {9, 11}, {11, 13}, {13, 15}, {15, 16}, {16, 16}};

// AutoPlay를 위한 림시보관변수
static std::map<uint32, AutoPlayUnitInfo>	s_AutoPlayUnitInfoes;
static std::map<T_FAMILYID, T_ROOMID>		s_AutoPlayerRoomID;

// ActionGroupID를 보관하기 위한 변수
std::map<uint64, uint64>	s_mapActionGroupIDs;	// RoomID << 32 | FID, curDay << 32 | ActionGroupID

// 방승급시에 꽃에 물주기한 사용자목록 보관 변수
static std::map<uint32, TmapDwordToDword> g_mapFamilyFlowerWaterTimeOfRoomForPromotion;

static void	cb_M_CS_ACT_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void	cb_M_CS_ACT_WAIT_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void	cb_M_CS_ACT_RELEASE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void	cb_M_CS_ACT_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void cb_M_CS_MOUNT_LUCKANIMAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void cb_M_CS_UNMOUNT_LUCKANIMAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
static void cb_M_CS_ACT_STEP_OPENROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);// 2014/06/09

/****************************************************************************
* CallbackArray - OpenRoom
****************************************************************************/
TUnifiedCallbackItem OroomCallbackArray[] =
{
	//{ M_CS_SELFSTATE,			cb_M_CS_SELFSTATE },

	{ M_CS_REQEROOM,			cb_M_CS_REQEROOM_Oroom		},
	
	{ M_CS_CHECK_ENTERROOM,		cb_M_CS_CHECK_ENTERROOM		},
	{ M_CS_DELROOM,				cb_M_CS_DELROOM },

	//{ M_CS_PROLONG_PASS,		cb_M_CS_PROLONG_PASS		},

	{ M_CS_BANISH,				cb_M_CS_BANISH			},

	{ M_CS_MANAGER_ADD,			cb_M_CS_MANAGER_ADD			},
	{ M_CS_MANAGER_ANS,			cb_M_CS_MANAGER_ANS			},
	{ M_CS_MANAGER_DEL,			cb_M_CS_MANAGER_DEL			},
	{ M_CS_MANAGER_GET,			cb_M_CS_MANAGER_GET			},
	{ M_NT_MANAGER_DEL,			cb_M_NT_MANAGER_DEL			},
	{ M_CS_MANAGER_SET_PERMISSION,			cb_M_CS_MANAGER_SET_PERMISSION			},

	//{ M_CS_ROOMPARTCHANGE,		cb_M_CS_ROOMPARTCHANGE	},
	{ M_CS_SETROOMINFO,			cb_M_CS_SETROOMINFO },
	{ M_CS_DECEASED,			cb_M_CS_DECEASED },
	{ M_CS_SETDEAD,				cb_M_CS_SETDEAD	 },
	{ M_CS_SET_DEAD_FIGURE,		cb_M_CS_SET_DEAD_FIGURE	 },
	//{ M_CS_DELDEAD,				cb_M_CS_DELDEAD	 },
	//{ M_CS_DELRMEMO,			cb_M_CS_DELRMEMO },
	{ M_CS_NEWRMEMO,			cb_M_CS_NEWRMEMO },
	//{ M_CS_MODIFYRMEMO,			cb_M_CS_MODIFYRMEMO },
	//{ M_CS_REQRMEMO,			cb_M_CS_REQRMEMO },
	//{ M_CS_MODIFY_MEMO_START,	cb_M_CS_MODIFY_MEMO_START },
	//{ M_CS_MODIFY_MEMO_END,		cb_M_CS_MODIFY_MEMO_END },
	//{ M_CS_ROOMMEMO_GET,		cb_M_CS_ROOMMEMO_GET },

	{ M_CS_REQRVISITLIST,		cb_M_CS_REQRVISITLIST },
	{ M_CS_ADDACTIONMEMO,		cb_M_CS_ADDACTIONMEMO },

	{ M_CS_VISIT_DETAIL_LIST,	cb_M_CS_VISIT_DETAIL_LIST },
	{ M_CS_VISIT_DATA,			cb_M_CS_VISIT_DATA },
	{ M_CS_VISIT_DATA_SHIELD,	cb_M_CS_VISIT_DATA_SHIELD },
	{ M_CS_VISIT_DATA_DELETE,	cb_M_CS_VISIT_DATA_DELETE },
	{ M_CS_VISIT_DATA_MODIFY,	cb_M_CS_VISIT_DATA_MODIFY },
	{ M_CS_GET_ROOM_VISIT_FID,	cb_M_CS_GET_ROOM_VISIT_FID },

	{ M_CS_REQ_PAPER_LIST,		cb_M_CS_REQ_PAPER_LIST },
	{ M_CS_REQ_PAPER,			cb_M_CS_REQ_PAPER },
	{ M_CS_NEW_PAPER,			cb_M_CS_NEW_PAPER },
	{ M_CS_DEL_PAPER,			cb_M_CS_DEL_PAPER },
	{ M_CS_MODIFY_PAPER,		cb_M_CS_MODIFY_PAPER },
	{ M_CS_MODIFY_PAPER_START,	cb_M_CS_MODIFY_PAPER_START },
	{ M_CS_MODIFY_PAPER_END,	cb_M_CS_MODIFY_PAPER_END },

	{ M_CS_TOMBSTONE,			cb_M_CS_TOMBSTONE },

	{ M_CS_RUYIBEI_TEXT,		cb_M_CS_RUYIBEI_TEXT },

	{ M_CS_GOLDFISH_CHANGE,		cb_M_CS_GOLDFISH_CHANGE },

	{ M_CS_LUCK_REQ,			cb_M_CS_LUCK_REQ },
	{ M_NT_LUCK,				cb_M_NT_LUCK },
	{ M_NT_LUCKHIS,				cb_M_NT_LUCKHIS },
//	{ M_NT_LUCK4_RES,			cb_M_NT_LUCK4_RES },
	{ M_NT_LUCK4_CONFIRM,		cb_M_NT_LUCK4_CONFIRM },

	//{ M_CS_FORCEOUT,			cb_M_CS_FORCEOUT },

	{ M_CS_ACT_REQ,				cb_M_CS_ACT_REQ },
	{ M_CS_ACT_WAIT_CANCEL,		cb_M_CS_ACT_WAIT_CANCEL },
	{ M_CS_ACT_RELEASE,			cb_M_CS_ACT_RELEASE },
	{ M_CS_ACT_START,			cb_M_CS_ACT_START },
	{ M_CS_ACT_STEP,			cb_M_CS_ACT_STEP_OPENROOM }, // 2014/06/09

	//{ M_NT_SETDEAD,				cb_M_NT_SETDEAD	 },

	//{ M_WS_CANUSERMONEY,		cb_CANUSEMONEY },

	{ M_CS_PROMOTION_ROOM,		cb_M_CS_PROMOTION_ROOM_Oroom },

	//{ M_CS_ROOMCARD_CHECK,			cb_M_CS_ROOMCARD_CHECK			},

	{ M_GMCS_LISTCHANNEL,		cb_M_GMCS_LISTCHANNEL },

	{ M_NT_ROOM_RENEW,				cb_M_NT_ROOM_RENEW			},
	{ M_NT_RELOAD_ROOMINFO,			cb_M_NT_RELOAD_ROOMINFO			},

	{ M_NT_ADDACTIONMEMO,			cb_M_NT_ADDACTIONMEMO		},
	{ M_NT_AUTOPLAY_REQ,			cb_M_NT_AUTOPLAY_REQ		},
	{ M_NT_AUTOPLAY_START,			cb_M_NT_AUTOPLAY_START		},
	{ M_NT_AUTOPLAY_END,			cb_M_NT_AUTOPLAY_END		},
	//{ M_NT_AUTOPLAY_STATECH,		cb_M_NT_AUTOPLAY_STATECH		},
	//{ M_NT_AUTOPLAY_CHMETHOD,		cb_M_NT_AUTOPLAY_CHMETHOD		},
	//{ M_NT_AUTOPLAY_HOLDITEM,		cb_M_NT_AUTOPLAY_HOLDITEM		},
	//{ M_NT_AUTOPLAY_MOVECH,			cb_M_NT_AUTOPLAY_MOVECH		},

	{ M_NT_ROOM_CREATE_REQ,			cb_M_NT_ROOM_CREATE_REQ		},

	{ M_CS_GET_ROOMSTORE,			cb_M_CS_GET_ROOMSTORE },
	{ M_CS_ADD_ROOMSTORE,			cb_M_CS_ADD_ROOMSTORE },
	{ M_CS_GET_ROOMSTORE_HISTORY,	cb_M_CS_GET_ROOMSTORE_HISTORY },
	{ M_CS_GET_ROOMSTORE_HISTORY_DETAIL,	cb_M_CS_GET_ROOMSTORE_HISTORY_DETAIL },
	{ M_CS_ROOMSTORE_LOCK,			cb_M_CS_ROOMSTORE_LOCK },
	{ M_CS_ROOMSTORE_UNLOCK,		cb_M_CS_ROOMSTORE_UNLOCK },
	{ M_CS_ROOMSTORE_LASTITEM_SET,	cb_M_CS_ROOMSTORE_LASTITEM_SET },
	{ M_CS_ROOMSTORE_HISTORY_MODIFY,	cb_M_CS_ROOMSTORE_HISTORY_MODIFY },
	{ M_CS_ROOMSTORE_HISTORY_DELETE,	cb_M_CS_ROOMSTORE_HISTORY_DELETE },
	{ M_CS_ROOMSTORE_HISTORY_SHIELD,	cb_M_CS_ROOMSTORE_HISTORY_SHIELD },
	{ M_CS_ROOMSTORE_GETITEM,		cb_M_CS_ROOMSTORE_GETITEM },

	{ M_CS_MOUNT_LUCKANIMAL,		cb_M_CS_MOUNT_LUCKANIMAL },
	{ M_CS_UNMOUNT_LUCKANIMAL,		cb_M_CS_UNMOUNT_LUCKANIMAL },

};

/****************************************************************************
* COpenRoomService class
****************************************************************************/
class COpenRoomService : public IService
{
public:

	void init()
	{
		init_ChannelWorldService();
	}

	bool update ()
	{
		RefreshRoom();
		DoEverydayWork();

		update_ChannelWorldService();

		return true;
	}

	void release()
	{
		DestroyRoomWorlds();

		release_ChannelWorldService();
	}
};

void run_openroom_service(int argc, const char **argv)
{
	COpenRoomService *scn = new COpenRoomService;
	scn->setArgs (argc, argv);
	createDebug (NULL, false);
	scn->setCallbackArray (OroomCallbackArray, sizeof(OroomCallbackArray)/sizeof(OroomCallbackArray[0]));
#ifdef SIP_OS_WINDOWS
#	if _MSC_VER >= 1900
		sint retval = scn->main(OROOM_SHORT_NAME, OROOM_LONG_NAME, "", 0, "", "", __DATE__ " " __TIME__);
#	else
		sint retval = scn->main(OROOM_SHORT_NAME, OROOM_LONG_NAME, "", 0, "", "", __DATE__" "__TIME__);
#	endif
#else
	string strDateTime = string(__DATE__) + " " + string(__TIME__);
	sint retval = scn->main(OROOM_SHORT_NAME, OROOM_LONG_NAME, "", 0, "", "", strDateTime.c_str());
#endif
	delete scn;
}

void run_openroom_service(LPSTR lpCmdLine)
{
	COpenRoomService *scn = new COpenRoomService;
	scn->setArgs (lpCmdLine);
	createDebug (NULL, false);
	scn->setCallbackArray (OroomCallbackArray, sizeof(OroomCallbackArray)/sizeof(OroomCallbackArray[0]));
#ifdef SIP_OS_WINDOWS
#	if _MSC_VER >= 1900
		sint retval = scn->main(OROOM_SHORT_NAME, OROOM_LONG_NAME, "", 0, "", "", __DATE__ " " __TIME__);
#	else
		sint retval = scn->main(OROOM_SHORT_NAME, OROOM_LONG_NAME, "", 0, "", "", __DATE__" "__TIME__);
#	endif
#else
	string strDateTime = string(__DATE__) + " " + string(__TIME__);
	sint retval = scn->main(OROOM_SHORT_NAME, OROOM_LONG_NAME, "", 0, "", "", strDateTime.c_str());
#endif
	
	delete scn;
}


//==================================================================
// Class:ActionResult

ActionResult::ActionResult()
{
	m_nRoomID = 0;
	memset(m_nCurIndex, 0, sizeof(m_nCurIndex));
	memset(m_nIDs, 0, sizeof(m_nIDs));
	memset(m_byIDs, 0, sizeof(m_byIDs));
	memset(m_FIDs, 0, sizeof(m_FIDs));
	for (int i = 0; i < ActionResultType_MAX; i ++)
	{
		for (int j = 0; j < MAX_ACTIONRESULT_ITEM_COUNT; j ++)
		{
			m_strNames[i][j] = "";
		}
	}
	uint32 curTime = (uint32)GetDBTimeSecond();
	for (int i = 0; i < sizeof(m_nTime) / sizeof(m_nTime[0]); i ++)
		m_nTime[i] = curTime;
	m_nTime[ActionResultType_Xianbao] = 0;
}

void ActionResult::setRoomInfo(int nRoomID, int nAncestorDeceasedCount)
{
	m_nRoomID = nRoomID;
	m_AncestorDeceasedCount = nAncestorDeceasedCount;

	for (int i = m_AncestorDeceasedCount; i < sizeof(m_nTime) / sizeof(m_nTime[0]); i ++)
		m_nTime[i] = 0;
}


void ActionResult::init(uint8 type, uint32 time, uint8* buf)
{
	if (type >= ActionResultType_MAX)
	{
		sipwarning("type >= ActionResultType_MAX!! type=%d", type);
		return;
	}
	uint32	curTime = (uint32)GetDBTimeSecond();

	tchar	name_buf[MAX_LEN_CHARACTER_NAME + 1];

	m_nTime[type] = time;

	if (!IsSameDay(curTime, m_nTime[type]))
		clearPrevData(type, curTime);
	else
	{
		if (type == ActionResultType_Xianbao)
		{
		}
		else if ((type == ActionResultType_Yishi) || (type == ActionResultType_Ancsetor_Jisi))
		{
			m_nCurIndex[type] = *buf;	buf ++;
			for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT; i ++)
			{
				m_byIDs[type][i] = *buf;	buf ++;
				m_nIDs[type][i] = *((uint32*)buf);	buf += sizeof(uint32);
				m_FIDs[type][i] = *((uint32*)buf);	buf += sizeof(uint32);
				memset(name_buf, 0, sizeof(name_buf));
				memcpy(name_buf, buf, MAX_LEN_CHARACTER_NAME * 2);	buf += (MAX_LEN_CHARACTER_NAME * 2);
				m_strNames[type][i] = ucstring(name_buf);
			}
		}
		else if (type == ActionResultType_Auto2)
		{
			m_nCurIndex[type] = *buf;	buf ++;
			m_nIDs[type][ACTIONRESULT_AUTO2_ITEMCOUNT] = *((uint32*)buf);	buf += sizeof(uint32);	// AllCount
			for (int i = 0; i < ACTIONRESULT_AUTO2_ITEMCOUNT; i ++)
			{
				m_nIDs[type][i] = *((uint32*)buf);	buf += sizeof(uint32);
				m_FIDs[type][i] = *((uint32*)buf);	buf += sizeof(uint32);
				memset(name_buf, 0, sizeof(name_buf));
				memcpy(name_buf, buf, MAX_LEN_CHARACTER_NAME * 2);	buf += (MAX_LEN_CHARACTER_NAME * 2);
				m_strNames[type][i] = ucstring(name_buf);
			}
		}
		else if ((type >= ActionResultType_AncestorDeceased_Hua) && (type < ActionResultType_AncestorDeceased_Hua + MAX_ANCESTOR_DECEASED_COUNT))
		{
			m_nCurIndex[type] = *buf;	buf ++;
			for (int i = 0; i < ACTIONRESULT_DECEASED_HUA_ITEMCOUNT; i ++)
			{
				m_nIDs[type][i] = *((uint32*)buf);	buf += sizeof(uint32);
				memset(name_buf, 0, sizeof(name_buf));
				memcpy(name_buf, buf, MAX_LEN_CHARACTER_NAME * 2);	buf += (MAX_LEN_CHARACTER_NAME * 2);
				m_strNames[type][i] = ucstring(name_buf);
			}
		}
		else if ((type >= ActionResultType_AncestorDeceased_Xiang) && (type < ActionResultType_AncestorDeceased_Xiang + MAX_ANCESTOR_DECEASED_COUNT))
		{
			m_nCurIndex[type] = *buf;	buf ++;
			for (int i = 0; i < ACTIONRESULT_DECEASED_XIANG_ITEMCOUNT; i ++)
			{
				m_nIDs[type][i] = *((uint32*)buf);	buf += sizeof(uint32);
				memset(name_buf, 0, sizeof(name_buf));
				memcpy(name_buf, buf, MAX_LEN_CHARACTER_NAME * 2);	buf += (MAX_LEN_CHARACTER_NAME * 2);
				m_strNames[type][i] = ucstring(name_buf);
			}
		}
	}
}

void ActionResult::saveActionResultData(int type, uint8 *buf, int bufsize)
{
	tchar str_buf[MAX_ACTIONRESULT_BUFSIZE * 2 + 10];
	for (int i = 0; i < bufsize; i ++)
	{
		smprintf((tchar*)(&str_buf[i * 2]), 2, _t("%02x"), buf[i]);
	}
	str_buf[bufsize * 2] = 0;

	int		nTime = (int)m_nTime[type];

	MAKEQUERY_SaveRoomActionResult(strQ, m_nRoomID, type, nTime, str_buf);
	DBCaller->execute(strQ);
}

void ActionResult::clearPrevData(int type, uint32 curTime)
{
	if (m_nTime[type] == 0) // 사용하지 않음.
		return;

	if (type == ActionResultType_Xianbao) // XianBao Result는 처리가 필요없음.
		return;

	m_nTime[type] = curTime;
	m_nCurIndex[type] = 0;
	bool	bChanged = false;
	for (int j = 0; j < MAX_ACTIONRESULT_ITEM_COUNT; j ++)
	{
		if (m_nIDs[type][j] != 0)
			bChanged = true;

		m_byIDs[type][j] = 0;
		m_nIDs[type][j] = 0;
		m_FIDs[type][j] = 0;
		m_strNames[type][j] = "";
	}

	if (!bChanged)
		return;

	if (type == ActionResultType_Yishi)
	{
		sendYishiResultData();
	}
	else if (type == ActionResultType_Auto2)
	{
		sendAuto2ResultData();
	}
	else if (type == ActionResultType_Ancsetor_Jisi)
	{
		sendAncestorJisiResultData();
	}
	else if ((type >= ActionResultType_AncestorDeceased_Xiang) && (type < ActionResultType_AncestorDeceased_Xiang + MAX_ANCESTOR_DECEASED_COUNT))
	{
		sendAncestorDeceasedXiangResultData(type);
	}
	else if ((type >= ActionResultType_AncestorDeceased_Hua) && (type < ActionResultType_AncestorDeceased_Hua + MAX_ANCESTOR_DECEASED_COUNT))
	{
		sendAncestorDeceasedHuaResultData(type);
	}
}

void ActionResult::clearPrevData()
{
	uint32	curTime = (uint32)GetDBTimeSecond();
	for (int type = 0; type < ActionResultType_MAX; type ++)
	{
		clearPrevData(type, curTime);
	}
}


void ActionResult::registerXianbaoResult(int xianbao_type, uint32 action_id, T_FAMILYID FID, ucstring name)
{
	m_nTime[ActionResultType_Xianbao] = (uint32)GetDBTimeSecond();

	saveXianbaoResultData();

	// Xianbao결과물도 Yishi결과물이 있는곳에 함께 놓는다.
	registerYishiResult(xianbao_type, action_id, FID, name);
}

void ActionResult::saveXianbaoResultData()
{
	uint8 buf[1];

	// Save to DB
	saveActionResultData(ActionResultType_Xianbao, buf, 0);

	// Send to AllInRoom
	sendXianbaoResultData();
}

void ActionResult::sendXianbaoResultData()
{
	CRoomWorld		*roomManager = GetOpenRoomWorld(m_nRoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", m_nRoomID);
		return;
	}
	roomManager->SendXianbaoResultDataToAll();
}


void ActionResult::registerYishiResult(int yishi_type, uint32 action_id, T_FAMILYID FID, ucstring name, BOOL bIsAuto)
{
	int curIndex = m_nCurIndex[ActionResultType_Yishi];
	m_nCurIndex[ActionResultType_Yishi] ++;
	if(m_nCurIndex[ActionResultType_Yishi] >= ACTIONRESULT_YISHI_ITEMCOUNT)
		m_nCurIndex[ActionResultType_Yishi] = 0;

	if (m_nTime[ActionResultType_Yishi] == 0)
		m_nTime[ActionResultType_Yishi] = (uint32)GetDBTimeSecond();

	m_byIDs[ActionResultType_Yishi][curIndex] = (uint8)yishi_type;
	m_nIDs[ActionResultType_Yishi][curIndex] = action_id;
	m_FIDs[ActionResultType_Yishi][curIndex] = FID;
	m_strNames[ActionResultType_Yishi][curIndex] = name;

	saveYishiResultData();
}

void ActionResult::saveYishiResultData()
{
	// Make Data
	uint8 buf[MAX_ACTIONRESULT_BUFSIZE];
	uint8 *pbuf = buf;

	memset(buf, 0, sizeof(buf));

	*pbuf = (uint8) m_nCurIndex[ActionResultType_Yishi];	pbuf ++;
	for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT; i ++)
	{
		*pbuf = m_byIDs[ActionResultType_Yishi][i];	pbuf ++;
		*((uint32*)pbuf) = m_nIDs[ActionResultType_Yishi][i];	pbuf += sizeof(uint32);
		*((uint32*)pbuf) = m_FIDs[ActionResultType_Yishi][i];	pbuf += sizeof(uint32);
		memcpy(pbuf, m_strNames[ActionResultType_Yishi][i].c_str(), m_strNames[ActionResultType_Yishi][i].length() * 2);	pbuf += (MAX_LEN_CHARACTER_NAME * 2);
	}

	// Save to DB
	int bufsize = pbuf - buf;
	saveActionResultData(ActionResultType_Yishi, buf, bufsize);

	// Send to AllInRoom
	sendYishiResultData();
}

void ActionResult::sendYishiResultData()
{
	CRoomWorld		*roomManager = GetOpenRoomWorld(m_nRoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", m_nRoomID);
		return;
	}
	roomManager->SendYishiResultDataToAll();
}

void ActionResult::deleteYishiResult(uint32 action_id)
{
	if (action_id == 0)	// Invalid
		return;

	// search
	int	delIndex = 0;
	for (delIndex = 0; delIndex < ACTIONRESULT_YISHI_ITEMCOUNT; delIndex ++)
	{
		if (action_id == m_nIDs[ActionResultType_Yishi][delIndex])
			break;
	}
	if (delIndex >= ACTIONRESULT_YISHI_ITEMCOUNT)
		return;

	// delete
	if (m_nCurIndex[ActionResultType_Yishi] == 0)
		m_nCurIndex[ActionResultType_Yishi] = ACTIONRESULT_YISHI_ITEMCOUNT - 1;
	else
		m_nCurIndex[ActionResultType_Yishi] --;
	int curIndex = m_nCurIndex[ActionResultType_Yishi];

	if (curIndex == delIndex || m_nIDs[ActionResultType_Yishi][curIndex] == 0)
	{
		m_byIDs[ActionResultType_Yishi][delIndex] = 0;
		m_nIDs[ActionResultType_Yishi][delIndex] = 0;
		m_FIDs[ActionResultType_Yishi][delIndex] = 0;
		m_strNames[ActionResultType_Yishi][delIndex] = "";
	}
	else
	{
		m_byIDs[ActionResultType_Yishi][delIndex] = m_byIDs[ActionResultType_Yishi][curIndex];
		m_nIDs[ActionResultType_Yishi][delIndex] = m_nIDs[ActionResultType_Yishi][curIndex];
		m_FIDs[ActionResultType_Yishi][delIndex] = m_FIDs[ActionResultType_Yishi][curIndex];
		m_strNames[ActionResultType_Yishi][delIndex] = m_strNames[ActionResultType_Yishi][curIndex];

		m_byIDs[ActionResultType_Yishi][curIndex] = 0;
		m_nIDs[ActionResultType_Yishi][curIndex] = 0;
		m_FIDs[ActionResultType_Yishi][curIndex] = 0;
		m_strNames[ActionResultType_Yishi][curIndex] = "";
	}

	saveYishiResultData();
}


void ActionResult::registerAuto2Result(uint32 action_id, T_FAMILYID FID, ucstring name)
{
	int curIndex = m_nCurIndex[ActionResultType_Auto2];
	m_nCurIndex[ActionResultType_Auto2] ++;
	if(m_nCurIndex[ActionResultType_Auto2] >= ACTIONRESULT_AUTO2_ITEMCOUNT)
		m_nCurIndex[ActionResultType_Auto2] = 0;

	if (m_nTime[ActionResultType_Auto2] == 0)
		m_nTime[ActionResultType_Auto2] = (uint32)GetDBTimeSecond();

	m_byIDs[ActionResultType_Auto2][curIndex] = 0;
	m_nIDs[ActionResultType_Auto2][curIndex] = action_id;
	m_FIDs[ActionResultType_Auto2][curIndex] = FID;
	m_strNames[ActionResultType_Auto2][curIndex] = name;

	m_nIDs[ActionResultType_Auto2][ACTIONRESULT_AUTO2_ITEMCOUNT] ++;
	saveAuto2ResultData();
}

void ActionResult::saveAuto2ResultData()
{
	// Make Data
	uint8 buf[MAX_ACTIONRESULT_BUFSIZE];
	uint8 *pbuf = buf;

	memset(buf, 0, sizeof(buf));

	*pbuf = (uint8) m_nCurIndex[ActionResultType_Auto2];	pbuf ++;
	*((uint32*)pbuf) = m_nIDs[ActionResultType_Auto2][ACTIONRESULT_AUTO2_ITEMCOUNT];	pbuf += sizeof(uint32);
	for (int i = 0; i < ACTIONRESULT_AUTO2_ITEMCOUNT; i ++)
	{
		*((uint32*)pbuf) = m_nIDs[ActionResultType_Auto2][i];	pbuf += sizeof(uint32);
		*((uint32*)pbuf) = m_FIDs[ActionResultType_Auto2][i];	pbuf += sizeof(uint32);
		memcpy(pbuf, m_strNames[ActionResultType_Auto2][i].c_str(), m_strNames[ActionResultType_Auto2][i].length() * 2);	pbuf += (MAX_LEN_CHARACTER_NAME * 2);
	}

	// Save to DB
	int	bufsize = pbuf - buf;
	saveActionResultData(ActionResultType_Auto2, buf, bufsize);

	// Send to AllInRoom
	sendAuto2ResultData();
}

void ActionResult::sendAuto2ResultData()
{
	CRoomWorld		*roomManager = GetOpenRoomWorld(m_nRoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", m_nRoomID);
		return;
	}
	roomManager->SendAuto2ResultDataTo(0, TServiceId(0));
}


void ActionResult::registerAncestorJisiResult(int yishi_type, uint32 action_id, T_FAMILYID FID, ucstring name)
{
	int curIndex = m_nCurIndex[ActionResultType_Ancsetor_Jisi];
	m_nCurIndex[ActionResultType_Ancsetor_Jisi] ++;
	if(m_nCurIndex[ActionResultType_Ancsetor_Jisi] >= ACTIONRESULT_YISHI_ITEMCOUNT)
		m_nCurIndex[ActionResultType_Ancsetor_Jisi] = 0;

	if (m_nTime[ActionResultType_Ancsetor_Jisi] == 0)
		m_nTime[ActionResultType_Ancsetor_Jisi] = (uint32)GetDBTimeSecond();

	m_byIDs[ActionResultType_Ancsetor_Jisi][curIndex] = (uint8)yishi_type;
	m_nIDs[ActionResultType_Ancsetor_Jisi][curIndex] = action_id;
	m_FIDs[ActionResultType_Ancsetor_Jisi][curIndex] = FID;
	m_strNames[ActionResultType_Ancsetor_Jisi][curIndex] = name;

	saveAncestorJisiResultData();
}

void ActionResult::saveAncestorJisiResultData()
{
	// Make Data
	uint8 buf[MAX_ACTIONRESULT_BUFSIZE];
	uint8 *pbuf = buf;

	memset(buf, 0, sizeof(buf));

	*pbuf = (uint8) m_nCurIndex[ActionResultType_Ancsetor_Jisi];	pbuf ++;
	for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT; i ++)
	{
		*pbuf = m_byIDs[ActionResultType_Ancsetor_Jisi][i];	pbuf ++;
		*((uint32*)pbuf) = m_nIDs[ActionResultType_Ancsetor_Jisi][i];	pbuf += sizeof(uint32);
		*((uint32*)pbuf) = m_FIDs[ActionResultType_Ancsetor_Jisi][i];	pbuf += sizeof(uint32);
		memcpy(pbuf, m_strNames[ActionResultType_Ancsetor_Jisi][i].c_str(), m_strNames[ActionResultType_Ancsetor_Jisi][i].length() * 2);	pbuf += (MAX_LEN_CHARACTER_NAME * 2);
	}

	// Save to DB
	int bufsize = pbuf - buf;
	saveActionResultData(ActionResultType_Ancsetor_Jisi, buf, bufsize);

	// Send to AllInRoom
	sendAncestorJisiResultData();
}

void ActionResult::sendAncestorJisiResultData()
{
	CRoomWorld		*roomManager = GetOpenRoomWorld(m_nRoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", m_nRoomID);
		return;
	}
	roomManager->SendAncestorJisiResultDataToAll();
}


void ActionResult::registerAncestorDeceasedXiangResult(int deceased_index, int itemID, ucstring name)
{
	int type = ActionResultType_AncestorDeceased_Xiang + deceased_index;

	// 아이템정보 얻기
	const MST_ITEM	*pItem = GetMstItemInfo(itemID);
	if (pItem == NULL)
	{
		sipwarning("GetMstItemInfo = NULL. ItemID=%d", itemID);
		return;
	}
	if ((pItem->Level < 1) || (pItem->Level > 3))
	{
		sipwarning("Invalid Item Level. ItemID=%d, Level=%d", itemID, pItem->Level);
		return;
	}
	int itemLevel = pItem->Level - 1;

	// curIndex 계산
	int curItemCount[3];
	curItemCount[0] = m_nCurIndex[type] & 0xFF;
	curItemCount[1] = (m_nCurIndex[type] >> 8) & 0xFF;
	curItemCount[2] = (m_nCurIndex[type] >> 16) & 0xFF;

	static int levelItemCount[3] = {4, 4, 3};

	int curIndex = 0;
	for (int i = 0; i < itemLevel; i ++)
		curIndex += levelItemCount[i];
	curIndex += curItemCount[itemLevel];

	// m_nCurIndex 갱신
	curItemCount[itemLevel] ++;
	if (curItemCount[itemLevel] >= levelItemCount[itemLevel])
		curItemCount[itemLevel] = 0;
	m_nCurIndex[type] = curItemCount[0] | (curItemCount[1] << 8) | (curItemCount[2] << 16);

	// 자료기록
	if (m_nTime[type] == 0)
		m_nTime[type] = (uint32)GetDBTimeSecond();

	m_nIDs[type][curIndex] = itemID;
	m_strNames[type][curIndex] = name;

	saveAncestorDeceasedXiangResultData(type);
}

void ActionResult::saveAncestorDeceasedXiangResultData(int type)
{
	// Make Data
	uint8 buf[1 + ACTIONRESULT_DECEASED_XIANG_ITEMCOUNT * (4 + 2 * MAX_LEN_CHARACTER_NAME)];
	uint8 *pbuf = buf;

	memset(buf, 0, sizeof(buf));

	*pbuf = (uint8)m_nCurIndex[type];	pbuf ++;
	for (int i = 0; i < ACTIONRESULT_DECEASED_XIANG_ITEMCOUNT; i ++)
	{
		*((uint32*)pbuf) = m_nIDs[type][i];	pbuf += sizeof(uint32);
		memcpy(pbuf, m_strNames[type][i].c_str(), m_strNames[type][i].length() * 2);	pbuf += (MAX_LEN_CHARACTER_NAME * 2);
	}

	// Save to DB
	int bufsize = pbuf - buf;
	saveActionResultData(type, buf, bufsize);

	// Send to AllInRoom
	sendAncestorDeceasedXiangResultData(type);
}

void ActionResult::sendAncestorDeceasedXiangResultData(int type)
{
	CRoomWorld		*roomManager = GetOpenRoomWorld(m_nRoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", m_nRoomID);
		return;
	}
	roomManager->SendAncestorDeceasedXiangResultDataToAll(type);
}


void ActionResult::registerAncestorDeceasedHuaResult(int deceased_index, int itemID, ucstring name)
{
	int type = ActionResultType_AncestorDeceased_Hua + deceased_index;

	int curIndex = m_nCurIndex[type];
	m_nCurIndex[type] ++;
	if(m_nCurIndex[type] >= ACTIONRESULT_DECEASED_HUA_ITEMCOUNT)
		m_nCurIndex[type] = 0;

	if (m_nTime[type] == 0)
		m_nTime[type] = (uint32)GetDBTimeSecond();

	m_nIDs[type][curIndex] = itemID;
	m_strNames[type][curIndex] = name;

	saveAncestorDeceasedHuaResultData(type);
}

void ActionResult::saveAncestorDeceasedHuaResultData(int type)
{
	// Make Data
	uint8 buf[1 + ACTIONRESULT_DECEASED_HUA_ITEMCOUNT * (4 + 2 * MAX_LEN_CHARACTER_NAME)];
	uint8 *pbuf = buf;

	memset(buf, 0, sizeof(buf));

	*pbuf = (uint8) m_nCurIndex[type];	pbuf ++;
	for (int i = 0; i < ACTIONRESULT_DECEASED_HUA_ITEMCOUNT; i ++)
	{
		*((uint32*)pbuf) = m_nIDs[type][i];	pbuf += sizeof(uint32);
		memcpy(pbuf, m_strNames[type][i].c_str(), m_strNames[type][i].length() * 2);	pbuf += (MAX_LEN_CHARACTER_NAME * 2);
	}

	// Save to DB
	int bufsize = pbuf - buf;
	saveActionResultData(type, buf, bufsize);

	// Send to AllInRoom
	sendAncestorDeceasedHuaResultData(type);
}

void ActionResult::sendAncestorDeceasedHuaResultData(int type)
{
	CRoomWorld		*roomManager = GetOpenRoomWorld(m_nRoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", m_nRoomID);
		return;
	}
	roomManager->SendAncestorDeceasedHuaResultDataToAll(type);
}

int ActionResult::removeAncestorDeceasedCurrentHua(int deceased_index)
{
	int type = ActionResultType_AncestorDeceased_Hua + deceased_index;

	int curIndex = m_nCurIndex[type];
	if (m_nIDs[type][curIndex] == 0)
		return curIndex;

	m_nIDs[type][curIndex] = 0;
	m_strNames[type][curIndex] = "";

	saveAncestorDeceasedHuaResultData(type);

	return curIndex;
}


CRoomWorld* GetOpenRoomWorld(T_ROOMID _rid)
{
	CChannelWorld* pChannelWorld = GetChannelWorld(_rid);
	if (pChannelWorld == NULL)
		return NULL;

	if (pChannelWorld->m_WorldType == WORLD_ROOM)
		return (CRoomWorld*)pChannelWorld;

	return NULL;
}

static CRoomWorld* GetOpenRoomWorldFromFID(T_FAMILYID _fid)
{
	CChannelWorld* pChannelWorld = GetChannelWorldFromFID(_fid);
	if (pChannelWorld == NULL)
		return NULL;

	if (pChannelWorld->m_WorldType == WORLD_ROOM)
		return (CRoomWorld*)pChannelWorld;

	return NULL;
}

CRoomWorld* GetOpenRoomWorldFromAutoPlayer(T_FAMILYID _fid)
{
	std::map<T_FAMILYID, T_ROOMID>::iterator it = s_AutoPlayerRoomID.find(_fid);
	if (it == s_AutoPlayerRoomID.end())
		return NULL;
	return GetOpenRoomWorld(it->second);
}

void CRoomWorld::SendActionResults(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	// Xianbao결과물
	{
		CMessage	msgOut(M_SC_ACT_RESULT_XIANBAO);
		uint32	zero = 0;
		msgOut.serial(FID, zero);
		msgOut.serial(m_ActionResults.m_nTime[ActionResultType_Xianbao]);
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}

	// Yishi결과물
	{
		CMessage	msgOut(M_SC_ACT_RESULT_YISHI);
		uint32	zero = 0;
		uint8	NPCID = 0;
		msgOut.serial(FID, zero, m_RoomID, NPCID);
		for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT; i ++)
		{
			msgOut.serial(m_ActionResults.m_byIDs[ActionResultType_Yishi][i], m_ActionResults.m_nIDs[ActionResultType_Yishi][i], 
				m_ActionResults.m_FIDs[ActionResultType_Yishi][i], m_ActionResults.m_strNames[ActionResultType_Yishi][i]);
		}
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}
	/*
	FAMILY	*pFamily = GetFamilyData(FID);
	if(pFamily->m_bIsMobile == 2)  //Unity판본에만 조상결과물 내려보낸다.
	{
		// 조상탑 행사 결과물
		CMessage	msgOut(M_SC_ACT_RESULT_ANCESTOR_JISI);
		uint32	zero = 0;
		msgOut.serial(FID, zero);
		for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT; i ++)
		{
			msgOut.serial(m_ActionResults.m_byIDs[ActionResultType_Ancsetor_Jisi][i], m_ActionResults.m_nIDs[ActionResultType_Ancsetor_Jisi][i], 
				m_ActionResults.m_FIDs[ActionResultType_Ancsetor_Jisi][i], m_ActionResults.m_strNames[ActionResultType_Ancsetor_Jisi][i]);
		}
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	

		// 조상탑 고인 향드리기행사 결과물	
		for (uint8 index = 0; index < MAX_ANCESTOR_DECEASED_COUNT; index ++)
		{
			int type = ActionResultType_AncestorDeceased_Xiang + index;
			if (m_ActionResults.m_nTime[type] == 0)
				continue;
			CMessage	msgOut(M_SC_ACT_RESULT_ANCESTOR_DECEASED_XIANG);
			uint32	zero = 0;
			msgOut.serial(FID, zero, index);
			for (int i = 0; i < ACTIONRESULT_DECEASED_XIANG_ITEMCOUNT; i ++)
			{
				msgOut.serial(m_ActionResults.m_nIDs[type][i], m_ActionResults.m_strNames[type][i]);
			}
			CUnifiedNetwork::getInstance()->send(sid, msgOut);
		}
	
		// 조상탑 고인 꽃행사 결과물	
		for (uint8 index = 0; index < MAX_ANCESTOR_DECEASED_COUNT; index ++)
		{
			int type = ActionResultType_AncestorDeceased_Hua + index;
			if (m_ActionResults.m_nTime[type] == 0)
				continue;
			CMessage	msgOut(M_SC_ACT_RESULT_ANCESTOR_DECEASED_HUA);
			uint32	zero = 0;
			msgOut.serial(FID, zero, index);
			for (int i = 0; i < ACTIONRESULT_DECEASED_HUA_ITEMCOUNT; i ++)
			{
				msgOut.serial(m_ActionResults.m_nIDs[type][i], m_ActionResults.m_strNames[type][i]);
			}
			CUnifiedNetwork::getInstance()->send(sid, msgOut);
		}
	}*/
}

void CRoomWorld::SendXianbaoResultDataToAll()
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ACT_RESULT_XIANBAO)
		msgOut.serial(m_ActionResults.m_nTime[ActionResultType_Xianbao]);
	FOR_END_FOR_FES_ALL()
}

void CRoomWorld::SendYishiResultDataToAll()
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ACT_RESULT_YISHI)
		uint8 NPCID = 0;
		msgOut.serial(m_RoomID, NPCID);
		for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT; i ++)
		{
			msgOut.serial(m_ActionResults.m_byIDs[ActionResultType_Yishi][i], m_ActionResults.m_nIDs[ActionResultType_Yishi][i], 
				m_ActionResults.m_FIDs[ActionResultType_Yishi][i], m_ActionResults.m_strNames[ActionResultType_Yishi][i]);
		}
	FOR_END_FOR_FES_ALL()
}

#define MACRO_SendAuto2ResultDataTo	\
	msgOut.serial(m_ActionResults.m_nIDs[ActionResultType_Auto2][ACTIONRESULT_AUTO2_ITEMCOUNT]);	\
	for (int i = 0; i < ACTIONRESULT_AUTO2_ITEMCOUNT; i ++)	\
	{	\
		msgOut.serial(m_ActionResults.m_nIDs[ActionResultType_Auto2][i],	\
		m_ActionResults.m_FIDs[ActionResultType_Auto2][i], m_ActionResults.m_strNames[ActionResultType_Auto2][i]);	\
	}	\

void CRoomWorld::SendAuto2ResultDataTo(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	if (FID != 0)
	{
		uint32 zero =0;
		CMessage msgOut(M_SC_ACT_RESULT_AUTO2);
		msgOut.serial(FID, zero);
		MACRO_SendAuto2ResultDataTo
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}
	else
	{
		FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ACT_RESULT_AUTO2)
			MACRO_SendAuto2ResultDataTo
		FOR_END_FOR_FES_ALL()
	}
}

void CRoomWorld::SendAncestorJisiResultDataToAll()
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ACT_RESULT_ANCESTOR_JISI)
		for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT; i ++)
		{
			msgOut.serial(m_ActionResults.m_byIDs[ActionResultType_Ancsetor_Jisi][i], m_ActionResults.m_nIDs[ActionResultType_Ancsetor_Jisi][i], 
				m_ActionResults.m_FIDs[ActionResultType_Ancsetor_Jisi][i], m_ActionResults.m_strNames[ActionResultType_Ancsetor_Jisi][i]);
		}
	FOR_END_FOR_FES_ALL()
}

void CRoomWorld::SendAncestorDeceasedXiangResultDataToAll(int type)
{
	uint8	index = (uint8)(type - ActionResultType_AncestorDeceased_Xiang);
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ACT_RESULT_ANCESTOR_DECEASED_XIANG)
		msgOut.serial(index);
		for (int i = 0; i < ACTIONRESULT_DECEASED_XIANG_ITEMCOUNT; i ++)
		{
			msgOut.serial(m_ActionResults.m_nIDs[type][i], m_ActionResults.m_strNames[type][i]);
		}
	FOR_END_FOR_FES_ALL()
}

void CRoomWorld::SendAncestorDeceasedHuaResultDataToAll(int type)
{
	uint8	index = (uint8)(type - ActionResultType_AncestorDeceased_Hua);
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ACT_RESULT_ANCESTOR_DECEASED_HUA)
		msgOut.serial(index);
		for (int i = 0; i < ACTIONRESULT_DECEASED_HUA_ITEMCOUNT; i ++)
		{
			msgOut.serial(m_ActionResults.m_nIDs[type][i], m_ActionResults.m_strNames[type][i]);
		}
	FOR_END_FOR_FES_ALL()
}


extern	MSSQLTIME		ServerStartTime_DB;
extern	TTime			ServerStartTime_Local;

static std::list<LuckHistory>	lstLuckHistory;
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
static TTime GetLastLuck4TimeForRoom(T_ROOMID RoomID)
{
	TTime lasttime = 0;
	std::list<LuckHistory>::iterator it;
	for (it = lstLuckHistory.begin(); it != lstLuckHistory.end(); it++)
	{
		LuckHistory& curHis = *it;
		if (RoomID != curHis.RoomID || curHis.LuckLevel != 4)
			continue;
		if (curHis.LuckDBTime > (uint64)lasttime)
		{
			lasttime = curHis.LuckDBTime;
		}
	}
	return lasttime;
}
extern uint32 nAlwaysLuck3ID;
static bool IsEnableLv3LuckCountOfFID(T_FAMILYID FID)
{
	if (nAlwaysLuck3ID > 0)
		return true;
	int nCount = 0;
	TTime	curTime = GetDBTimeSecond();
	std::list<LuckHistory>::iterator it;
	for (it = lstLuckHistory.begin(); it != lstLuckHistory.end(); it++)
	{
		LuckHistory& curHis = *it;
		if (FID != curHis.FID || curHis.LuckLevel != 3)
			continue;
		if (IsSameDay(curTime, curHis.LuckDBTime))
			nCount++;
	}

	if (nCount < LUCKY3_COUNT_PER_USER)
		return true;
	return false;
}
static TTime GetLastLuck4TimeForFID(T_FAMILYID FID)
{
	TTime lasttime = 0;
	std::list<LuckHistory>::iterator it;
	for (it = lstLuckHistory.begin(); it != lstLuckHistory.end(); it++)
	{
		LuckHistory& curHis = *it;
		if (FID != curHis.FID || curHis.LuckLevel != 4)
			continue;
		if (curHis.LuckDBTime > (uint64)lasttime)
		{
			lasttime = curHis.LuckDBTime;
		}
	}
	return lasttime;
}
static void GetTodayLuck3ForRoom(T_ROOMID RoomID, uint32* pIDBuffer)
{
	*pIDBuffer = 0;
	TTime	curTime = GetDBTimeSecond();
	std::list<LuckHistory>::iterator it;
	for (it = lstLuckHistory.begin(); it != lstLuckHistory.end(); it++)
	{
		LuckHistory& curHis = *it;
		if (RoomID != curHis.RoomID || curHis.LuckLevel != 3)
			continue;
		if (IsSameDay(curTime, curHis.LuckDBTime))
		{
			*pIDBuffer = curHis.LuckID;
			return;
		}
	}
}
static void GetTodayLuck4ForRoom(T_ROOMID RoomID, uint32* pIDBuffer, TTime* pTimeBuffer)
{
	*pIDBuffer = 0;
	*pTimeBuffer = 0;

	TTime	curTime = GetDBTimeSecond();
	std::list<LuckHistory>::iterator it;
	for (it = lstLuckHistory.begin(); it != lstLuckHistory.end(); it++)
	{
		LuckHistory& curHis = *it;
		if (RoomID != curHis.RoomID || curHis.LuckLevel != 4)
			continue;
		if (IsSameDay(curTime, curHis.LuckDBTime))
		{
			*pIDBuffer = curHis.LuckID;
			*pTimeBuffer = curHis.LuckDBTime;
			return;
		}
	}
}

static void RegisterLv3_4Luck(T_ROOMID RoomID, T_FAMILYID FID, uint8 LuckLevel, uint32 LuckID, TTime LuckDBTime)
{
	{
		lstLuckHistory.push_back(LuckHistory(FID, RoomID, LuckLevel, LuckID, LuckDBTime));
		//sipdebug(L"Add luck his data, fid=%d, roomid=%d, level=%d, luckid=%d", FID, RoomID, LuckLevel, LuckID); byy krs
	}
}

struct _entryDeadHis
{
	_entryDeadHis() {};
	_entryDeadHis(uint32 _history_id, uint8 _dead_id, uint32 _start_year, uint32 _end_year, ucstring _title)
	{
		history_id			= _history_id;
		dead_id				= _dead_id;
		start_year			= _start_year;
		end_year			= _end_year;
		title				= _title;
	};
	uint32			history_id;
	uint8			dead_id;
	uint32			start_year;
	uint32			end_year;
	ucstring		title;
};

extern TTime GetUpdateNightTime(TTime curTime);

int CalcDistance2(float x1, float y1, float z1, float x2, float y2, float z2)
{
	int dx = (int)x1 - (int)x2;
	int dy = (int)y1 - (int)y2;
	int dz = (int)z1 - (int)z2;

	return dx * dx + dy * dy + dz * dz;
}

TTime	GetDBTimeSecond()
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

uint32	getDaysSince1970(std::string& date)
{
	MSSQLTIME	t;
	if (!t.SetTime(date))
		return 0;
	uint32	result = (uint32)(t.timevalue / 60 / 60 / 24);
	return result;
}

uint32	getMinutesSince1970(std::string& date)
{
	MSSQLTIME	t;
	if (!t.SetTime(date))
		return 0;
	uint32	result = (uint32)(t.timevalue / 60);
	return result;
}

uint32	getSecondsSince1970(std::string& date)
{
	MSSQLTIME	t;
	if (!t.SetTime(date))
		return 0;
	return (uint32)t.timevalue;
}

void sendErrorResult(const MsgNameType &_name, T_FAMILYID _fid, uint32 _flag, SIPNET::TServiceId _tsid)
{
	CMessage msgout(_name);
	msgout.serial(_fid, _flag);
	CUnifiedNetwork::getInstance()->send( _tsid, msgout);
}

CWorld* GetWorldFromFID(T_FAMILYID _FID)
{
	TWorldType	type;
	T_ROOMID rid = GetFamilyWorld(_FID, &type);
	if(type == WORLD_LOBBY)
		return getLobbyWorld();
	if((type == WORLD_ROOM) || (type == WORLD_COMMON))
		return GetChannelWorldFromFID(_FID);
	return NULL;
}

RoomChannelData* CRoomWorld::GetChannel(uint32 ChannelID)
{
	TmapRoomChannelData::iterator	itChannel = m_mapChannels.find(ChannelID);
	if(itChannel == m_mapChannels.end())
		return NULL;
	return &itChannel->second;
}

RoomChannelData* CRoomWorld::GetFamilyChannel(T_FAMILYID FID)
{
	T_ID	ChannelID = GetFamilyChannelID(FID);
	if(ChannelID == 0)
		return NULL;
	TmapRoomChannelData::iterator	itChannel = m_mapChannels.find(ChannelID);
	if(itChannel == m_mapChannels.end())
		return NULL;
	return &itChannel->second;
}

// 사적자료인증값을 생성한다(관리자용)
uint32	MakeRoomHistoryPassword(T_ROOMID _rid)
{
	return (_rid + 2008) * 4 - 21;
}

// 사적자료인증값을 생성한다(련계인용)
uint32	MakeRoomHistoryPasswordForContact(T_ROOMID _rid)
{
	return (_rid + 2008) * 12 - 27;
}

bool CRoomWorld::SaveRoomLevelAndExp(bool bMustSave)
{
	if (m_bRoomExpChanged)
	{
		uint32	now = CTime::getSecondsSince1970();
		if (bMustSave || (now - m_dwRoomExpSavedTime > 300))	// 5분이 지났으면 보존한다.
		{
			MAKEQUERY_SaveRoomLevelAndExp(strQ, m_RoomID, m_Level, m_Exp);
			DBCaller->executeWithParam(strQ, NULL,
				"D",
				OUT_,		NULL);

			m_bRoomExpChanged = false;
			m_dwRoomExpSavedTime = now;
		}
		else
		{
			//sipinfo("RoomExp changed but not saved. RoomID=%d, Level=%d, Exp=%d", m_RoomID, m_Level, m_Exp); byy krs
		}
	}

	return true;
}

//bool	CRoomWorld::ReloadDeadInfo()
//{
//	// 고인정보 load
//	MAKEQUERY_GetDeceasedForRoom(strQ, m_RoomID);
//	CDBConnectionRest	DB(DBCaller);
//	DB_EXE_QUERY(DB, Stmt, strQ);
//	if ( nQueryResult != true )
//	{
//		sipwarning("load dead info failed: RoomId=%d", m_RoomID);
//		return false;
//	}
//
//	DB_QUERY_RESULT_FETCH(Stmt, row);
//	if ( IS_DB_ROW_VALID(row) )
//	{
//		COLUMN_DIGIT(row, 1, uint8, surnamelen1);
//		COLUMN_WSTR(row, 2, deadname1, MAX_LEN_DEAD_NAME);
//		COLUMN_DIGIT(row, 3, T_SEX, sex1);
//		COLUMN_DIGIT(row, 4, uint32, birthday1);
//		COLUMN_DIGIT(row, 5, uint32, deadday1);
//		COLUMN_WSTR(row, 6, his1, MAX_LEN_DEAD_BFHISTORY);
//		COLUMN_WSTR(row, 8, domicile1, MAX_LEN_DOMICILE);
//		COLUMN_WSTR(row, 9, nation1, MAX_LEN_NATION);
//		COLUMN_DIGIT(row, 10, uint32, photoid1);
//		COLUMN_DIGIT(row, 11, uint8, phototype);
//		COLUMN_DIGIT(row, 12, uint8, surnamelen2);
//		COLUMN_WSTR(row, 13, deadname2, MAX_LEN_DEAD_NAME);
//		COLUMN_DIGIT(row, 14, T_SEX, sex2);
//		COLUMN_DIGIT(row, 15, uint32, birthday2);
//		COLUMN_DIGIT(row, 16, uint32, deadday2);
//		COLUMN_WSTR(row, 17, his2, MAX_LEN_DEAD_BFHISTORY);
//		COLUMN_WSTR(row, 19, domicile2, MAX_LEN_DOMICILE);
//		COLUMN_WSTR(row, 20, nation2, MAX_LEN_NATION);
//		COLUMN_DIGIT(row, 21, uint32, photoid2);
//
//		m_Deceased1.m_Pos = 1;
//		m_Deceased1.m_Name = deadname1;
//		m_Deceased1.m_Surnamelen = surnamelen1;
//		m_Deceased1.m_Sex = sex1;
//		m_Deceased1.m_Birthday = birthday1;
//		m_Deceased1.m_Deadday = deadday1;
//		m_Deceased1.m_His = his1;
//		m_Deceased1.m_Domicile = domicile1;
//		m_Deceased1.m_Nation = nation1;
//		m_Deceased1.m_PhotoId = photoid1;
//		m_Deceased1.m_PhotoType = phototype;
//		m_Deceased1.m_bValid = true;
//
//		m_Deceased2.m_Pos = 2;
//		m_Deceased2.m_Name = deadname2;
//		m_Deceased2.m_Surnamelen = surnamelen2;
//		m_Deceased2.m_Sex = sex2;
//		m_Deceased2.m_Birthday = birthday2;
//		m_Deceased2.m_Deadday = deadday2;
//		m_Deceased2.m_His = his2;
//		m_Deceased2.m_Domicile = domicile2;
//		m_Deceased2.m_Nation = nation2;
//		m_Deceased2.m_PhotoId = photoid2;
//		m_Deceased2.m_PhotoType = 0;
//		m_Deceased2.m_bValid = true;
//	}
//	return true;
//}

static void	DBCB_DBGetRoomInfoForOpen(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		ret			= *(uint32*)argv[0];
	T_ROOMID	RoomID		= *(uint32*)argv[1];
	T_FAMILYID	FID			= *(uint32*)argv[2];

	if (!nQueryResult)
		ret = 99999999;

	if (ret != 0)
	{
		// ManagerService에 통지
		{
			CMessage	msgOut(M_NT_ROOM_CREATED);
			msgOut.serial(RoomID, ret);
			CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
		}

		sipwarning("CreateRoomWorld failed. RoomID=%d, ret=%d", RoomID, ret);
		return;
	}

	uint32	curTime = (uint32)GetDBTimeSecond();

	T_ROOMVIRTUE roomvirtue = 0;
	CRoomWorld		*newManager = GetOpenRoomWorld(RoomID);
	if (newManager == NULL)
	{
		TChannelWorld	rw = new CRoomWorld;
		newManager = (CRoomWorld*)(rw.getPtr());

		newManager->init(RoomID);

		uint32 nRows, i;
		bool	bUpdateFlower = false, bUpdateFish = false;
		resSet->serial(nRows);
		for (i = 0; i < nRows; i ++)
		{
			T_ROOMNAME	R_Name;			resSet->serial(R_Name);
			uint16		Kindid;			resSet->serial(Kindid);
			T_FAMILYID	Creatorid;		resSet->serial(Creatorid);
			T_FAMILYID	Masterid;		resSet->serial(Masterid);
			string		Creationday;	resSet->serial(Creationday);
			string		Backday;		resSet->serial(Backday);
			uint8		Powerid;		resSet->serial(Powerid);
			ucstring	rcomment;		resSet->serial(rcomment);
			uint32		Visits;			resSet->serial(Visits);
			uint32		VDays;			resSet->serial(VDays);
			uint32		Exp;			resSet->serial(Exp);
			uint16		Virtue;			resSet->serial(Virtue);
			uint16		Fame;			resSet->serial(Fame);
			uint16		Appreciation;	resSet->serial(Appreciation);
			uint8		Level;			resSet->serial(Level);
			string		LastIntime;		resSet->serial(LastIntime);
			uint8		bclosed;		resSet->serial(bclosed);
			T_FAMILYID	curgiveid;		resSet->serial(curgiveid);
			string		curgivetime;	resSet->serial(curgivetime);
			string		lastgivetime;	resSet->serial(lastgivetime);
			string		renewtime;		resSet->serial(renewtime);
			string		authkey;		resSet->serial(authkey);
			uint8		deleteflag;		resSet->serial(deleteflag);
			string		deletetime;		resSet->serial(deletetime);
			uint8		fish_level;		resSet->serial(fish_level);
			uint32		fish_exp;		resSet->serial(fish_exp);
			uint32		fish_nextupdatetime;	resSet->serial(fish_nextupdatetime);
			uint32		flower_id;		resSet->serial(flower_id);
			uint32		flower_level;	resSet->serial(flower_level);
			uint32		flower_deltime;	resSet->serial(flower_deltime);
			uint32		flower_fid;		resSet->serial(flower_fid);
			ucstring	flower_fname;	resSet->serial(flower_fname);
			uint8		freeuse;		resSet->serial(freeuse);
			uint32		spacecapacity;	resSet->serial(spacecapacity);
			uint32		spaceused;		resSet->serial(spaceused);

			if (deleteflag)
			{
				{
					ret = 99999997;
					CMessage	msgOut(M_NT_ROOM_CREATED);
					msgOut.serial(RoomID, ret);
					CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
				}

				//sipwarning("deleteflag seted. RoomID=%d", RoomID); byy krs
				return;
			}

			newManager->m_RoomID		= RoomID;
			newManager->m_CreatorID		= Creatorid;
			newManager->m_MasterID		= Masterid;
			newManager->m_RoomName		= R_Name;
			newManager->m_SceneID		= Kindid;
			newManager->m_PowerID		= Powerid;
			newManager->m_CreationDay	= Creationday;
			newManager->m_BackDay		= Backday;
			newManager->m_BackTime.SetTime(Backday);
			newManager->m_RoomComment	= rcomment;
			newManager->m_Visits		= Visits;
			newManager->m_VDays			= VDays;
			newManager->m_Exp			= Exp;
			roomvirtue = Virtue;
			newManager->m_Fame			= Fame;
			newManager->m_Appreciation	= Appreciation;
			newManager->m_Level			= Level;
			newManager->m_LastIncTime.SetTime(LastIntime);
			newManager->m_URLPassword	= MakeRoomHistoryPassword(RoomID);
			newManager->m_URLPasswordForContact	= MakeRoomHistoryPasswordForContact(RoomID);
			newManager->m_AuthKey		= authkey;

			newManager->m_bClosed		= (bclosed == 0) ? false : true;
			newManager->m_LastGiveTime.SetTime(lastgivetime);
			newManager->m_RenewTime		= renewtime;

/*			// 꽃 Update - 체험방에서 꽃이 무한으로 있어야 한다는 사양때문에 이 처리를 하지 않는다.
			if ((flower_id > 0) && (curtime > flower_deltime))
			{
				bUpdateFlower = true;
				flower_id = 0;
				flower_level = 0;
				flower_fid = 0;
				flower_fname = "";
			}*/

			// 물고기 Update
			//if ((fish_nextupdatetime > 0) && (fish_nextupdatetime < curtime))
			//{
			//	int		reduce_percent = g_nFishExpReducePercent;
			//	while (fish_nextupdatetime < curtime)
			//	{
			//		if(fish_exp > mstFishLevel[fish_level].Exp)
			//		{
			//			fish_exp -= mstFishLevel[fish_level].Exp;
			//			fish_exp = (100 - reduce_percent) * fish_exp / 100;
			//			fish_exp += mstFishLevel[fish_level].Exp;
			//		}
			//		else
			//		{
			//			fish_exp = mstFishLevel[fish_level].Exp;
			//		}
			//		fish_nextupdatetime = (uint32)GetUpdateNightTime(fish_nextupdatetime);
			//	}
			//	bUpdateFish = true;
			//}
			//else
			//{
			//	fish_nextupdatetime = (uint32)GetUpdateNightTime(curtime);

			//newManager->m_FishLevel = fish_level;
			newManager->SetFishExp(fish_exp);
			//newManager->m_FishNextUpdateTime = fish_nextupdatetime;
			newManager->m_FlowerID = flower_id;
			newManager->m_FlowerLevel = flower_level;
			newManager->m_FlowerDelTime = flower_deltime;
			newManager->m_FlowerFID = flower_fid;
			newManager->m_FlowerFamilyName = flower_fname;
			newManager->m_FreeFlag = freeuse;
			newManager->m_HisSpaceCapacity = spacecapacity;
			newManager->m_HisSpaceUsed = spaceused;
			//sipdebug("open room space info : roomid=%d, capacity=%d, used=%d", RoomID, spacecapacity, spaceused); byy krs
		}

		if (!newManager->IsValid())
		{
			{
				ret = 99999998;
				CMessage	msgOut(M_NT_ROOM_CREATED);
				msgOut.serial(RoomID, ret);
				CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
			}

			sipwarning("newManager->IsValid() = false. RoomID=%d", RoomID);
			return;
		}

		// 꽃 Update
		if (bUpdateFlower)
		{
			MAKEQUERY_SaveFlowerLevel(strQ, RoomID, 0, 0, 0, 0);

			DBCaller->execute(strQ);
		}

		// 물고기 Update
		if (bUpdateFish)
		{
			MAKEQUERY_SaveFishExp(strQ, RoomID, 0, newManager->m_FishExp, 0); // newManager->m_FishLevel, newManager->m_FishExp, newManager->m_FishNextUpdateTime);

			DBCaller->execute(strQ);
		}

		// 주인정보얻기
		resSet->serial(nRows);
		if (nRows > 0)
		{
			ucstring	name;		resSet->serial(name);
			uint32		userid;		resSet->serial(userid);

			newManager->m_MasterName = name;
			newManager->m_MasterUserID = userid;
		}

		// 고인정보 load
		//newManager->ReloadDeadInfo();
		resSet->serial(nRows);
		if (nRows > 0)
		{
			uint8		surnamelen1;	resSet->serial(surnamelen1);
			ucstring	deadname1;		resSet->serial(deadname1);
			uint8		sex1;			resSet->serial(sex1);
			uint32		birthday1;		resSet->serial(birthday1);
			uint32		deadday1;		resSet->serial(deadday1);
			ucstring	his1;			resSet->serial(his1);
			ucstring	domicile1;		resSet->serial(domicile1);
			ucstring	nation1;		resSet->serial(nation1);
			uint32		photoid1;		resSet->serial(photoid1);
			uint8		phototype;		resSet->serial(phototype);
			uint8		surnamelen2;	resSet->serial(surnamelen2);
			ucstring	deadname2;		resSet->serial(deadname2);
			uint8		sex2;			resSet->serial(sex2);
			uint32		birthday2;		resSet->serial(birthday2);
			uint32		deadday2;		resSet->serial(deadday2);
			ucstring	his2;			resSet->serial(his2);
			ucstring	domicile2;		resSet->serial(domicile2);
			ucstring	nation2;		resSet->serial(nation2);
			uint32		photoid2;		resSet->serial(photoid2);

			newManager->m_Deceased1.m_Pos = 1;
			newManager->m_Deceased1.m_Name = deadname1;
			newManager->m_Deceased1.m_Surnamelen = surnamelen1;
			newManager->m_Deceased1.m_Sex = sex1;
			newManager->m_Deceased1.m_Birthday = birthday1;
			newManager->m_Deceased1.m_Deadday = deadday1;
			newManager->m_Deceased1.m_His = his1;
			newManager->m_Deceased1.m_Domicile = domicile1;
			newManager->m_Deceased1.m_Nation = nation1;
			newManager->m_Deceased1.m_PhotoId = photoid1;
			newManager->m_Deceased1.m_bValid = true;

			newManager->m_Deceased2.m_Pos = 2;
			newManager->m_Deceased2.m_Name = deadname2;
			newManager->m_Deceased2.m_Surnamelen = surnamelen2;
			newManager->m_Deceased2.m_Sex = sex2;
			newManager->m_Deceased2.m_Birthday = birthday2;
			newManager->m_Deceased2.m_Deadday = deadday2;
			newManager->m_Deceased2.m_His = his2;
			newManager->m_Deceased2.m_Domicile = domicile2;
			newManager->m_Deceased2.m_Nation = nation2;
			newManager->m_Deceased2.m_PhotoId = photoid2;
			newManager->m_Deceased2.m_bValid = true;

			newManager->m_PhotoType = phototype;
		}

		// 방의 비석목록 load
		resSet->serial(nRows);
		if (nRows > 0)
		{
			uint16		id;				resSet->serial(id);
			ucstring	tombcont;		resSet->serial(tombcont);

			newManager->m_RoomTomb.insert( make_pair(id, tombcont) );
		}

		// 방의 Ruyibei목록 load
		resSet->serial(nRows);
		if (nRows > 0)
		{
			uint32		itemid;		resSet->serial(itemid);
			ucstring	cont;		resSet->serial(cont);

			newManager->m_RoomRuyibei.ItemID = itemid;
			newManager->m_RoomRuyibei.Ruyibei = cont;
		}
		else
		{
			newManager->m_RoomRuyibei.ItemID = 0;
			newManager->m_RoomRuyibei.Ruyibei = "";
		}

		// 방관리자목록 load
		resSet->serial(nRows);
		for (i = 0; i < nRows; i ++)
		{
			uint32		ManagerFID;		resSet->serial(ManagerFID);
			uint8		Permission;		resSet->serial(Permission);
			if (i < MAX_MANAGER_COUNT)
			{
				newManager->m_RoomManagers[i].FID = ManagerFID;
				newManager->m_RoomManagers[i].Permission = Permission;
				newManager->m_RoomManagerCount = i + 1;
			}
			else
			{
				sipwarning("i >= MAX_MANAGER_COUNT. RoomID=%d, i=%d", RoomID, i);
			}
		}

		// 방의 행사진행결과물 load
		newManager->m_ActionResults.setRoomInfo(RoomID, GetMaxAncestorDeceasedCount(newManager->m_SceneID));
		uint8		buf[MAX_ACTIONRESULT_BUFSIZE];
		resSet->serial(nRows);
		for (i = 0; i < nRows; i ++)
		{
			uint8		type;		resSet->serial(type);
			uint32		time;		resSet->serial(time);
			uint32		BufSize;	resSet->serial(BufSize);	resSet->serialBuffer(buf, BufSize);

			newManager->m_ActionResults.init(type, time, buf);
		}

		// 조상탑Text load
		newManager->m_AncestorText = "";
		resSet->serial(nRows);
		for (i = 0; i < nRows; i ++)
		{
			resSet->serial(newManager->m_AncestorText);
		}

		// 조상탑에 모신 고인정보 load
		newManager->m_nAncestorDeceasedCount = 0;
		resSet->serial(nRows);
		if (nRows > MAX_ANCESTOR_DECEASED_COUNT)
		{
			sipwarning("nRows > MAX_ANCESTOR_DECEASED_COUNT. nRows=%d", nRows);
			nRows = MAX_ANCESTOR_DECEASED_COUNT;
		}
		for (i = 0; i < nRows; i ++)
		{
			uint8		_AncestorIndex;		resSet->serial(_AncestorIndex);
			uint8		_DeceasedID;		resSet->serial(_DeceasedID);
			uint8		_Surnamelen;		resSet->serial(_Surnamelen);
			ucstring	_Name;				resSet->serial(_Name);
			T_SEX		_Sex;				resSet->serial(_Sex);
			T_DATE4		_Birthday;			resSet->serial(_Birthday);
			T_DATE4		_Deadday;			resSet->serial(_Deadday);
			ucstring	_His;				resSet->serial(_His);
			ucstring	_Domicile;			resSet->serial(_Domicile);
			ucstring	_Nation;			resSet->serial(_Nation);
			uint32		_PhotoId;			resSet->serial(_PhotoId);
			uint8		_PhotoType;			resSet->serial(_PhotoType);
			if (_AncestorIndex >= MAX_ANCESTOR_DECEASED_COUNT)
			{
				sipwarning("_AncestorIndex >= MAX_ANCESTOR_DECEASED_COUNT. _AncestorIndex=%d", _AncestorIndex);
				continue;
			}

			newManager->m_AncestorDeceased[newManager->m_nAncestorDeceasedCount].init(_AncestorIndex, _DeceasedID, _Surnamelen, _Name, 
				_Sex, _Birthday, _Deadday, _His, _Domicile, _Nation, _PhotoId, _PhotoType);
			newManager->m_nAncestorDeceasedCount ++;
		}

		// 기념관 보물창고정보
		resSet->serial(nRows);
		if (nRows == 1)
		{
			uint32	len;		resSet->serial(len);
			uint8	buff[MAX_ROOMSTORE_COUNT * 6 + 10];		resSet->serialBuffer(buff, len);
			newManager->m_RoomStore.init(buff, len);
		}

		// 금붕어자료
		resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint8	buf[100], buf2[250];	// DB와 바퍼크기를 일치시킨다
			const uint32 putfishfamilyunitbyte = sizeof(T_FAMILYID) + MAX_LEN_FAMILY_NAME*2; // 20
			uint32	ScopeID;	resSet->serial(ScopeID);
			uint32	BufSize;	resSet->serial(BufSize);	resSet->serialBuffer(buf, BufSize);
			uint32	BufSize2;	resSet->serial(BufSize2);	resSet->serialBuffer(buf2, BufSize2);

			GoldFishData	data;
			int		count;
			for (count = 0; count * 8 + 8 <= (int)BufSize; count ++)
			{
				data.m_GoldFishs[count].GoldFishID = *((uint32*)(&buf[count * 8]));
				data.m_GoldFishs[count].GoldFishDelTime = *((uint32*)(&buf[count * 8 + 4]));
			}
			if ((int)BufSize2 < count * putfishfamilyunitbyte)	// 20 = 4 + 2 * MAX_LEN_FAMILY_NAME
			{
				// 물고기를 넣은 사람의 자료가 없으면 방주인으로 설정한다.
				for (int i = 0; i < count; i ++)
				{
					data.m_GoldFishs[i].FID = newManager->m_MasterID;
					data.m_GoldFishs[i].FName = newManager->m_MasterName;
				}
			}
			else
			{
				ucchar	FName[MAX_LEN_FAMILY_NAME+1];
				for (int i = 0; i < count; i ++)
				{
					data.m_GoldFishs[i].FID = *((uint32*)(&buf2[i * putfishfamilyunitbyte]));
					memset(FName, 0, sizeof(FName));
					memcpy(FName, &buf2[i * putfishfamilyunitbyte + 4], MAX_LEN_FAMILY_NAME*2);
					data.m_GoldFishs[i].FName = FName;
				}
			}
			newManager->m_GoldFishData[ScopeID] = data;
		}

		newManager->InitMasterDataValues();

		// default로 1번채널을 만든다.
		// C판본을 위한 코드 by krs
		FAMILY	*pFamily = GetFamilyData(FID);
		uint8 channelType = 0;
		if (pFamily == NULL)
		{
			//sipwarning("GetFamilyData = NULL. FID=%d", FID); byy krs
			//return;
		}
		else
			channelType = pFamily->m_bIsMobile;
		
		uint32 channelID = 1;
		if (channelType == 2)
			channelID = 1;// +ROOM_UNITY_CHANNEL_START; by krs
		RoomChannelData	channelData(newManager, newManager->m_RoomID, channelID, newManager->m_SceneID);
		//sipwarning("ChannelID=%d, FID=%d", channelID, FID);
		// 길상등탁자료 - 1번채널에만 초기자료를 설정한다. 다른 채널의 길상등탁초기자료는 없는것으로 한다.
		// 자료는 마지막으로 놓은것이 제일 먼저 나온다. 탁에 배치하는것은 제일 오래된것을 제일 앞에 배치한다.
		resSet->serial(nRows);
		int blesslamp_pos = nRows - 1;

		const OUT_ROOMKIND	*outRoomInfo = GetOutRoomKind(newManager->m_SceneID);
		if (outRoomInfo == NULL)
		{
			if (blesslamp_pos >= MAX_BLESS_LAMP_COUNT)
				blesslamp_pos = MAX_BLESS_LAMP_COUNT - 1;
		}
		else
		{
			if (blesslamp_pos >= (int)outRoomInfo->BlessLampCount)
				blesslamp_pos = outRoomInfo->BlessLampCount - 1;
		}

		for (uint32 i = 0; i < nRows; i ++, blesslamp_pos --)
		{
			uint32	MemoID;		resSet->serial(MemoID);
			uint32	vFID;		resSet->serial(vFID);
			ucstring	vFName;	resSet->serial(vFName);
			T_DATETIME	MemoDate;	resSet->serial(MemoDate);
			uint8	Protected;	resSet->serial(Protected);
			if (blesslamp_pos < 0)
				continue;
			if (i > MAX_BLESS_LAMP_COUNT)
			{
				sipwarning("nRows > MAX_BLESS_LAMP_COUNT. RoomID=%d, nRows=%d", RoomID, nRows);
				continue;
			}

			channelData.m_BlessLamp[blesslamp_pos].MemoID = MemoID;
			channelData.m_BlessLamp[blesslamp_pos].FID = vFID;
			channelData.m_BlessLamp[blesslamp_pos].FName = vFName;
			channelData.m_BlessLamp[blesslamp_pos].Time = getSecondsSince1970(MemoDate);

			// 기한이 지난 길상등은 삭제한다.
			if (curTime - channelData.m_BlessLamp[blesslamp_pos].Time >= SHOW_BLESS_LAMP_TIME)
				blesslamp_pos ++;
		}
		
		if (RoomID > MAX_EXPROOMID) // by krs
			newManager->m_mapChannels.insert(make_pair(channelID, channelData));			
		
		// 체험방은 Default로 10개의 선로가 나오게 한다.
		if (RoomID < MAX_EXPROOMID)
		{	
			newManager->m_nDefaultChannelCount = EXPROOMROOM_DEFAULT_CHANNEL_COUNT;
			for (i = 1; i <= newManager->m_nDefaultChannelCount; i ++) // by krs
			{
				RoomChannelData	channelData1(newManager, newManager->m_RoomID, i, newManager->m_SceneID);
				//newManager->m_mapChannels.insert(make_pair(i, channelData1));

				//채널창조 by krs				
				RoomChannelData	channelData2(newManager, newManager->m_RoomID, i + ROOM_UNITY_CHANNEL_START, newManager->m_SceneID);
				//newManager->m_mapChannels.insert(make_pair(i + ROOM_C_CHANNEL_START, channelData2));

				if (i == 1) {
					for (uint32 j = 0; j < MAX_BLESS_LAMP_COUNT; j++) {
						channelData1.m_BlessLamp[j] = channelData.m_BlessLamp[j];
						channelData2.m_BlessLamp[j] = channelData.m_BlessLamp[j];
					}
				}

				newManager->m_mapChannels.insert(make_pair(i, channelData1));
				newManager->m_mapChannels.insert(make_pair(i + ROOM_UNITY_CHANNEL_START, channelData2));
			}
		}

		// 방승급시에 보관했던 꽃에 물주기한 사용자목록을 회복한다.
		std::map<uint32, TmapDwordToDword>::iterator it = g_mapFamilyFlowerWaterTimeOfRoomForPromotion.find(RoomID);
		if (it != g_mapFamilyFlowerWaterTimeOfRoomForPromotion.end())
		{
			newManager->m_mapFamilyFlowerWaterTime = it->second;
			g_mapFamilyFlowerWaterTimeOfRoomForPromotion.erase(it);
		}
		GetTodayLuck3ForRoom(newManager->m_RoomID, &(newManager->m_LuckID_Lv3));
		GetTodayLuck4ForRoom(newManager->m_RoomID, &(newManager->m_LuckID_Lv4), &(newManager->m_LuckID_Lv4_ShowTime));

		map_RoomWorlds.insert( make_pair(RoomID, rw) );

		newManager->CheckRoomcreateHoliday();

		// FS에 통지한다.
		uint8	flag = 1;	// Room Created
		CMessage	msgOut(M_NT_ROOM);
		msgOut.serial(RoomID, flag);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut, false);
	}

	// ManagerService에 통지
	{
		CMessage	msgOut(M_NT_ROOM_CREATED);
		msgOut.serial(RoomID, ret, newManager->m_SceneID, newManager->m_RoomName, newManager->m_MasterID);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	}
}

void CRoomWorld::CreateChannel(uint32 channelid)
{
	RoomChannelData	data(this, m_RoomID, channelid, m_SceneID);
	RoomChannelData	*data1 = NULL;
	if(channelid < ROOM_UNITY_CHANNEL_START && channelid > 1) // by krs
		data1 = GetChannel(1);
	else if((channelid > ROOM_UNITY_CHANNEL_START) && channelid < ROOM_MOBILE_CHANNEL_START)
	{
		data1 = GetChannel(5001);
	}

	if (data1 == NULL)
	{
		//sipwarning("GetChannel(1) = NULL. RoomID=%d, ChannelID=%d", m_RoomID, channelid);
	}
	else
	{
		for (int i = 0; i < MAX_BLESS_LAMP_COUNT; i ++)
		{
			data.m_BlessLamp[i] = data1->m_BlessLamp[i];
		}
	}
	m_mapChannels.insert(make_pair(channelid, data));
}

// 새로운 방관리자를 만든다
void CreateRoomWorld1(T_ROOMID RoomID, T_FAMILYID FID)
{
/*	
	if (RoomID == 25565091)
	{
		uint32 ret = 99999998;
		CMessage	msgOut(M_NT_ROOM_CREATED);
		msgOut.serial(RoomID, ret);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
sipwarning("Refuse openroom(25565091)");
		return;
	}
*/
	MAKEQUERY_GetRoomInfoForOpen(strQ,RoomID);
	DBCaller->executeWithParam(strQ, DBCB_DBGetRoomInfoForOpen,
		"DDD",
		OUT_,					NULL,
		CB_,					RoomID,
		CB_,					FID);
}

CRoomWorld::~CRoomWorld()
{
	SaveRoomLevelAndExp(true);
}

void	DeleteRoomWorld(TID _roomid)
{
	TChannelWorlds::iterator it = map_RoomWorlds.find(_roomid);
	if(it != map_RoomWorlds.end())
	{
		//_DeleteRoomWorld((CRoomWorld*)(it->second.getPtr()));
		map_RoomWorlds.erase(it);
	}
}

// 모든 방정보를 보관하고 삭제한다.
void DestroyRoomWorlds()
{
	TChannelWorlds::iterator it = map_RoomWorlds.begin();
	while (it != map_RoomWorlds.end())
	{
		map_RoomWorlds.erase(it);
		it = map_RoomWorlds.begin();
	}
}

// 방주인이름변경을 반영한다
void SetFamilyNameForOpenRoom(T_FAMILYID FID, T_FAMILYNAME FamilyName)
{
	TChannelWorlds::iterator It;
	for (It = map_RoomWorlds.begin(); It != map_RoomWorlds.end(); It++)
	{
		CChannelWorld*		pChannel = It->second.getPtr();
		if (pChannel->m_WorldType != WORLD_ROOM)
			continue;
		CRoomWorld	*rw = (CRoomWorld*)(pChannel);
		rw->SetFamilyNameForOpenRoom(FID, FamilyName);
	}
}

// 방의 정보들 갱신
void	RefreshRoom()
{
	//static TTime	LastUpdateRefreshSacrificeTime = 0;
	//static TTime	LastUpdateRoomOrderEventTime = 0;
	//static TTime	LastUpdateRoomPetTime = 0;
	//static TTime	LastUpdateRoomWishTime = 0;
	//static TTime	LastUpdateTGateTime = 0;
	static TTime	LastUpdateGiveTime = 0;
	static TTime	LastUpdateActivityTime = 0;
	static TTime	LastUpdateAnimal = 0;
	static TTime	LastUpdateOther = 0;

	bool	/*bRefreshSacrifice = true, bPet = true, bWish = true, bTGate = true,*/ bGive = true, bAnimal = true, bActivity = true, bOther = true;

	TTime curTime = CTime::getLocalTime();
	//if (curTime - LastUpdateRefreshSacrificeTime < 100)
	//	bRefreshSacrifice = false;
	//if (curTime - LastUpdateRoomOrderEventTime < 60000)
	//	bOrderEvent = false;
	//if (curTime - LastUpdateRoomPetTime < 60000)
	//	bPet = false;
	//if (curTime - LastUpdateRoomWishTime < 60000)
	//	bWish = false;
	//if (curTime - LastUpdateTGateTime < 60000)
	//	bTGate = false;
	if (curTime - LastUpdateGiveTime < 10000)
		bGive = false;
	if (curTime - LastUpdateActivityTime < 10000)
		bActivity = false;
	if (curTime - LastUpdateAnimal < 1000)
		bAnimal = false;
	if (curTime - LastUpdateOther < 60000)
		bOther = false;
	
	TChannelWorlds::iterator It;
	for (It = map_RoomWorlds.begin(); It != map_RoomWorlds.end(); It++)
	{
		CChannelWorld*		pChannel = It->second.getPtr();
		if (pChannel->m_WorldType != WORLD_ROOM)
			continue;
		CRoomWorld	*rw = (CRoomWorld*)(pChannel);
		//if(bRefreshSacrifice)
		//	rw->RefreshSacrifice();
		//if(bOrderEvent)
		//	rw->RefreshOrderEvent();
		//if(bPet)
		//	rw->RefreshPet();
		//if(bWish)
		//	rw->RefreshWish();
		//if(bTGate)
		//	rw->RefreshTGate();
		//if(bGive)
		//	rw->RefreshGiveProperty(); // 삭제-Offline양도기능
		if(bActivity)
			rw->RefreshActivity();
		if (bAnimal)
		{
			rw->RefreshAnimal();

			// 폭죽관련
			rw->RefreshCracker();
		}

		rw->RefreshCharacterState();
		if (bOther)
		{
			rw->RefreshEnterWait();
			rw->RefreshInviteWait();
			rw->RefreshTouXiang();
			rw->RefreshInviteWait();
			rw->RefreshFreezingPlayers();
		}

		if (rw->GetRoomEventStatus() == ROOMEVENT_STATUS_DOING)
			rw->RefreshRoomEvent();
		else
		{
			if (bOther)
				rw->RefreshRoomEvent();
		}
	}

	curTime = CTime::getLocalTime();
	//if (bRefreshSacrifice)
	//	LastUpdateRefreshSacrificeTime = curTime;
	//if(bOrderEvent)
	//	LastUpdateRoomOrderEventTime = curTime;
	//if(bPet)
	//	LastUpdateRoomPetTime = curTime;
	//if(bWish)
	//	LastUpdateRoomWishTime = curTime;
	//if(bTGate)
	//	LastUpdateTGateTime = curTime;
	if(bGive)
		LastUpdateGiveTime = curTime;
	if(bActivity)
		LastUpdateActivityTime = curTime;
	if(bAnimal)
		LastUpdateAnimal = curTime;
	if(bOther)
		LastUpdateOther = curTime;
}

void DoEverydayWorkInRoom()
{
	TChannelWorlds::iterator It;
	for (It = map_RoomWorlds.begin(); It != map_RoomWorlds.end(); It++)
	{
		CChannelWorld*		pChannel = It->second.getPtr();
		if (pChannel->m_WorldType != WORLD_ROOM)
			continue;
		CRoomWorld	*rw = (CRoomWorld*)(pChannel);
		rw->DoEverydayWorkInRoom();
	}

	//// Lucky자료 초기화
	//{
	//	CMessage	msgNT(M_NT_LUCK_RESET);
	//	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgNT);
	//}

	// 2배경험치날인가 검사
	CheckTodayHoliday();

	// s_mapActionGroupIDs 초기화
	s_mapActionGroupIDs.clear();
}

void CRoomWorld::init(T_ROOMID RoomID)
{
	m_RoomID = RoomID;

	memset(m_RoomExpHis, 0, sizeof(m_RoomExpHis));

	m_RoomManagerCount = 0;
//	memset(m_RoomManagers, 0, sizeof(m_RoomManagers));

	m_LuckID_Lv3 = 0;
	m_LuckID_Lv4 = 0;

	SetRoomEventStatus(ROOMEVENT_STATUS_NONE);

	// 깜짝이벤트를 설정한다.
	SetRoomEvent();
}

// 방에 들어갈수 있는가를 검사한다
uint32	CRoomWorld::GetEnterRoomCode(T_FAMILYID _FID, std::string password, uint32 ChannelID, uint8 ContactStatus)
{
	// 방의 사용기간이 다 되였는가를 검사
	TTime limittime = m_BackTime.timevalue;
	if (limittime != 0)
	{
		TTime	curtime = GetDBTimeSecond();
		if (curtime > limittime)
			return E_ENTER_EXPIRE;
	}

	// 방이 동결되였으면
	if (m_bClosed)
		return E_ENTER_CLOSE;

	// 특수채널에는 한사람만 들어올수 있다.
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData != NULL)
	{
		if (pChannelData->m_ChannelID >= ROOM_SPECIAL_CHANNEL_START && pChannelData->GetChannelUserCount() > 0)
			return E_ENTER_NO_FULL;
	}

	// 방주인이면
	if (m_MasterID == _FID)
		return ERR_NOERR;

	if (m_PowerID == ROOMOPEN_FENGTING)
		return E_ENTER_FENGTING;

	// 차단객으로 등록되였으면
//	if (IsBlackList(m_MasterID, _FID))
//		return E_ENTER_NO_BREAK;
	if (ContactStatus == 0xFF)
		return E_ENTER_NO_BREAK;

	// 방관리자이면
	if (IsRoomManager(_FID))
		return ERR_NOERR;

	// GM이면
	FAMILY *pFamily = GetFamilyData(_FID);
	if (pFamily != NULL)
	{
		if (pFamily->m_bGM == true)
			return ERR_NOERR;
	}

	if(ChannelID != 0)
	{
		// 집체행사대기목록에 있으면 OK
		_RoomActivity_Family* pActivityUsers = FindJoinFIDStatus(ChannelID, _FID);
		if(pActivityUsers != NULL)
		{
			return ERR_NOERR;
		}
	}

	// 공개되지 않은 방이면
	if (m_PowerID == ROOMOPEN_TONONE)
		return E_ENTER_NO_ONLYMASTER;

	// 체계사용자들이 창조한 방에는 일반 사용자들이 들어올수 없다
	if (IsSystemAccount(m_MasterUserID) &&								// 체계사용자가 창조한 방가운데서
		!(m_RoomID >= SYSTEMROOM_START && m_RoomID <= SYSTEMROOM_END) &&	// 체계공동구역이 아니고
		!(m_RoomID >= MIN_RELIGIONROOMID &&	m_RoomID <= MAX_RELIGIONROOMID)) // 종교구역도 아니면	
	{
		if (pFamily != NULL &&
			!IsSystemAccount(pFamily->m_UserID))	// 체계사용자가 아니면 들어올수 없다.
			return 	E_ENTER_NO_ONLYMASTER;			
	}

	// 모두에게 공개한 방이면
	if (m_PowerID == ROOMOPEN_TOALL)
		return ERR_NOERR;

	// 련계인들에게 공개한 방이면
	if (m_PowerID == ROOMOPEN_TONEIGHBOR)
	{
		if (ContactStatus != 1)
			return E_ENTER_NO_NEIGHBOR;
//		if (!IsFriendList(m_MasterID, _FID))
//			return E_ENTER_NO_NEIGHBOR;
	}

	if (m_PowerID == ROOMOPEN_PASSWORD)
	{
		if(password != m_AuthKey)
			return ERR_BADPASSWORD;
	}

	return ERR_NOERR;
}

bool	CRoomWorld::IsAnyFamilyOnRoomlife()
{
	if (!map_EnterWait.empty() || !m_mapFamilys.empty())
		return true;

	for(TmapRoomChannelData::iterator it = m_mapChannels.begin(); it != m_mapChannels.end(); it ++)
	{
		if(!it->second.m_InviteList.empty())
			return true;
	}

	return false;
}

// 방특수선로를 요청한다
void	CRoomWorld::RequestSpecialChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	ProcRemovedSpecialChannel();
	
	T_ID	channelid = m_nCandiSpecialCannel;
	uint8 channel_status = ROOMCHANNEL_SMOOTH;
	CMessage	msgOut(M_SC_CHANNELLIST);
	msgOut.serial(_FID, m_RoomID, channelid, channel_status);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);

	m_nCandiSpecialCannel++;
	RoomChannelData	data(this, m_RoomID, channelid, m_SceneID);
	m_mapChannels.insert(make_pair(channelid, data));
	//sipdebug("Special Channel Created. RoomID=%d, ChannelID=%d, FID=%d", m_RoomID, channelid, _FID); byy krs
}

// 방선로목록을 요청한다.
void	CRoomWorld::RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	ProcRemovedSpecialChannel();

	CChannelWorld::RequestChannelList(_FID, _sid);
}

// 방의 현재 가족수 얻기
T_MSTROOM_CHNUM	CRoomWorld::GetCurCharacterNum()
{
	T_MSTROOM_CHNUM	num = 0;
	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator it;

	for (it = m_mapFamilys.begin(); it != m_mapFamilys.end(); it++)
		num += it->second.GetCurCharacterNum();
	return num;
}

// 방의 현재 가족수 얻기(주인, 관리자제외)
T_MSTROOM_CHNUM	CRoomWorld::GetCurCharacterNumForOther()
{
	T_MSTROOM_CHNUM	num = 0;
	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator it;

	for (it = m_mapFamilys.begin(); it != m_mapFamilys.end(); it++)
	{
		uint32	uRole = it->second.m_RoomRole;

		if (uRole != ROOM_MANAGER && uRole != ROOM_MASTER)
			num += it->second.GetCurCharacterNum();
	}
	return num;
}

void CRoomWorld::ProcRemoveFamily(T_FAMILYID FID)
{
	for (RoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		TServiceId	tmpsid(0);
		for(uint8 i = 0; i < COUNT_ACTPOS - 1; i ++)
		{
			if (i + 1 != ACTPOS_AUTO2)
				ActWaitCancel(FID, (uint8)(i + 1), tmpsid, 0, pChannelData->m_ChannelID, true);
		}
	}
}

void CRoomWorld::OutRoom_Spec(T_FAMILYID _FID)
{
	// 사용자가 창조하였던 혹은 대기하였던 활동을 삭제한다.
	TServiceId	tmpsid(0);
	for(uint8 i = 0; i < COUNT_ACTPOS - 1; i ++)
	{
		ActWaitCancel(_FID, (uint8)(i + 1), tmpsid, 0);
	}

	RoomChannelData*	pChannelData = GetFamilyChannel(_FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel() = NULL. FID=%d", _FID);
		return;
	}

	// 사용자가 편지를 수정하던 상태였다면 처리한다.
	{
		VEC_LETTER_MODIFY_LIST::iterator it;
		for(it = m_LetterModifyList.begin(); it != m_LetterModifyList.end(); )
		{
			if(it->FID == _FID)
				it = m_LetterModifyList.erase(it);
			else
				it ++;
		}
	}

	//// 사용자가 방문록을 수정하던 상태였다면 처리한다.
	//{
	//	VEC_LETTER_MODIFY_LIST::iterator it;
	//	for(it = m_MemoModifyList.begin(); it != m_MemoModifyList.end(); )
	//	{
	//		if(it->FID == _FID)
	//			it = m_MemoModifyList.erase(it);
	//		else
	//			it ++;
	//	}
	//}

	// 사용자가 음악을 치던 상태였다면
	{
		std::map<T_FAMILYID, uint32>::iterator itMusic = pChannelData->m_CurMusicFIDs.find(_FID);
		if (itMusic != pChannelData->m_CurMusicFIDs.end())
		{
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MUSIC_END)
				msgOut.serial(_FID, itMusic->second);
			FOR_END_FOR_FES_ALL()
			pChannelData->m_CurMusicFIDs.erase(itMusic);
		}
	}

	// 사용자가 기념관창고아이템을 Lock한 상태였다면 취소한다.
	for (std::map<uint32, RoomStoreLockData>::iterator it = m_RoomStore.mapLockData.begin(); it != m_RoomStore.mapLockData.end(); it ++)
	{
		if (it->second.FID == _FID)
		{
			m_RoomStore.mapLockData.erase(it);
			break;
		}
	}

	// 사용자가 상서동물을 타고 있던 상태이면 해제한다
	if (pChannelData->m_MountLuck3FID == _FID)
	{
		pChannelData->ResetMountAnimalInfo();
	}

	// 방경험값 보관
	SaveRoomLevelAndExp(true);
}

uint8	CRoomWorld::GetFamilyRoleInRoom(T_FAMILYID _FID)
{
	if (_FID == m_MasterID)
		return ROOM_MASTER;
	if (IsRoomManager(_FID))
		return ROOM_MANAGER;
	return ROOM_NONE;
}

//void CRoomWorld::ChangeChannelFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid)
//{
//	SendRoomChannelData(FID, sid);
//}

void CRoomWorld::SendRoomChannelData(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	RoomChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	// 행사효과모델목록을 내려보낸다.
	SendActivityList(FID, sid);

	// 음악치는 상태를 내려보낸다.
	if (!pChannelData->m_CurMusicFIDs.empty())
	{
		CMessage	msgOut(M_SC_MUSIC_START);
		uint32	zero = 0;
		msgOut.serial(FID, zero);
		for (std::map<T_FAMILYID, uint32>::iterator it = pChannelData->m_CurMusicFIDs.begin(); it != pChannelData->m_CurMusicFIDs.end(); it ++)
		{
			T_FAMILYID	_CurFID = it->first;
			uint32		_CurMusicPos = it->second;
			msgOut.serial(_CurFID, _CurMusicPos);
		}
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}

	// 길상등탁자료를 내려보낸다.
	{
		const OUT_ROOMKIND	*outRoomInfo = GetOutRoomKind(m_SceneID);

		if (outRoomInfo != NULL)
		{
			CMessage	msgOut(M_SC_ROOMMEMO_LIST);
			msgOut.serial(FID);
			for (uint8 DeskPos = 0; DeskPos < outRoomInfo->BlessLampCount; DeskPos ++)
			{
				if (pChannelData->m_BlessLamp[DeskPos].MemoID != 0)
				{
					msgOut.serial(DeskPos, pChannelData->m_BlessLamp[DeskPos].MemoID, pChannelData->m_BlessLamp[DeskPos].FID, pChannelData->m_BlessLamp[DeskPos].FName, pChannelData->m_BlessLamp[DeskPos].Time);
				}
			}
			CUnifiedNetwork::getInstance()->send(sid, msgOut);
		}
	}
}

int CRoomWorld::IsRoomManager(T_FAMILYID FID)
{
	for(int i = 0; i < m_RoomManagerCount; i ++)
	{
		if(m_RoomManagers[i].FID == FID)
			return 0x80 | m_RoomManagers[i].Permission;
	}
	return 0;
}

// 가족이 접속한 접속써비스 Id를 얻는다
TServiceId	CRoomWorld::GetServiceId(T_FAMILYID _FID)
{
	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator	it0 = m_mapFamilys.find(_FID);
	if (it0 == m_mapFamilys.end())
		return TServiceId(0);
	return it0->second.m_ServiceID;
}

// 방주인이나 방관리자가 들어올때 사적자료공간정보를 DB로부터 refresh한다
void	CRoomWorld::UpdateHisSpaceInfoFroEnter(T_FAMILYID _FID)
{
	if((_FID != m_MasterID) && !IsRoomManager(_FID))
		return;

	tchar	szQuery[1024];
	MAKEQUERY_CheckSpace(szQuery, m_RoomID, 0);
	CDBConnectionRest	DB(DBCaller);
	DB_EXE_PREPARESTMT(DB, Stmt, szQuery);
	if (!nPrepareRet)
	{
		return;
	}
	SQLLEN	num1 = 0;
	int	ret = 0;
	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_LONG, SQL_INTEGER, 0, 0, &ret, 0, &num1);
	if (!nBindResult)
	{
		return;
	}
	DB_EXE_PARAMQUERY (Stmt, szQuery);
	if (!nQueryResult)
	{
		return;
	}
	if (ret != -1)
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (!IS_DB_ROW_VALID(row))
		{
			return;
		}

		COLUMN_DIGIT(row, 0, uint32, spacecapacity);
		COLUMN_DIGIT(row, 1, uint32, spaceused);
		m_HisSpaceCapacity = spacecapacity;
		m_HisSpaceUsed = spaceused;
		//sipdebug("enter room space info : roomid=%d, capacity=%d, used=%d", m_RoomID, spacecapacity, spaceused); byy krs
	}
}

// 방안의 모든 정보를 보낸다
void	CRoomWorld::SendAllInRoomToOne(T_FAMILYID _FID, TServiceId sid)
{
	// 방정보
	CMessage	msgout(M_SC_ROOMINFO);
	uint32			uResult = ERR_NOERR;
	T_ROOMID		roomid = m_RoomID;
	T_ROOMLEVEL		level = m_Level;
	T_ROOMEXP		exp = m_Exp;

	T_MSTROOM_ID	sceneId = m_SceneID;
	T_ROOMNAME		roomName = m_RoomName;
	T_FAMILYID		fid = m_MasterID;
	T_FAMILYNAME	familyName = m_MasterName;
	uint32			nCreateTime = getMinutesSince1970(m_CreationDay);
	uint32			nTermTime = getMinutesSince1970(m_BackDay);
	T_ROOMPOWERID	idOpenability = m_PowerID;
	T_COMMON_CONTENT comment = m_RoomComment;
	uint8			freeFlag = m_FreeFlag;
	T_ROOMVISITS	visits = m_Visits;
	uint8			masterFlag = (_FID == m_MasterID) ? true : false;
	uint32			nRenewTime = getMinutesSince1970(m_RenewTime);
	uint32			hisspaceall = m_HisSpaceCapacity;
	uint32			hisspaceused = m_HisSpaceUsed;
	uint8			deletedFlag = 0;
	
	uint8			phototype = m_PhotoType;
	T_DECEASED_NAME	deadname1 = m_Deceased1.m_Name;
	uint8			surnameLen1 = m_Deceased1.m_Surnamelen;
	T_DECEASED_NAME	deadname2 = m_Deceased2.m_Name;
	uint8			surnameLen2 = m_Deceased2.m_Surnamelen;

	uint32	nCurtime = (uint32)(GetDBTimeSecond() / 60);
	if(m_bClosed)
		uResult = E_ENTER_CLOSE;
	else if(nTermTime > 0 && nCurtime > nTermTime)
		uResult = E_ENTER_EXPIRE;

	msgout.serial(_FID);
	msgout.serial(uResult, roomid, level, exp);
	msgout.serial(sceneId, roomName, fid, familyName, nCreateTime, nTermTime);
	msgout.serial(idOpenability, comment, freeFlag, visits, masterFlag, nRenewTime);
	msgout.serial(hisspaceall, hisspaceused, deletedFlag, phototype, deadname1, surnameLen1, deadname2, surnameLen2);
	CUnifiedNetwork::getInstance ()->send(sid, msgout);
	//sipdebug("send room space info : roomid=%d, capacity=%d, used=%d", roomid, hisspaceall, hisspaceused); byy krs
	

	// 방의 속성
/*
	CMessage	mesOutPro(M_SC_ROOMPRO);
	uint32 uZero = 0;
	uint16	ExpPercent = CalcRoomExpPercent(m_Level, m_Exp);
	mesOutPro.serial(_FID, uZero);
	mesOutPro.serial(m_RoomID, m_Exp, m_Level, ExpPercent);
	CUnifiedNetwork::getInstance()->send(sid, mesOutPro);
*/
}

uint32 GetActionGroupID(uint32 RoomID, uint32 FID)
{
	uint32	curTimeMinute = GetDBTimeMinute();
	uint32	dayMinute = curTimeMinute % 1440;
	if (dayMinute < 5 || dayMinute > 1440 - 5)
		return 0;

	uint32	curTimeDay = (uint32)(curTimeMinute / 1440);
	uint64	key = RoomID;
	key = (key << 32) | FID;
	std::map<uint64, uint64>::iterator	it = s_mapActionGroupIDs.find(key);
	if (it == s_mapActionGroupIDs.end())
		return 0;
	if ((it->second >> 32) != curTimeDay)
		return 0;
	return (uint32)(it->second);	// return Low 32 bits
}

void SetActionGroupID(uint32 RoomID, uint32 FID, uint32 ActionGroupID)
{
	uint32	curTimeMinute = GetDBTimeMinute();
	uint32	dayMinute = curTimeMinute % 1440;
	if (dayMinute < 5 || dayMinute > 1440 - 5)
		return;

	uint32	curTimeDay = (uint32)(curTimeMinute / 1440);
	uint64	key = RoomID;
	key = (key << 32) | FID;

	uint64	value = curTimeDay;
	value = (value << 32 ) | ActionGroupID;

	s_mapActionGroupIDs[key] = value;
}

static void	DBCB_DBShrd_Visit_Insert(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		nCurActionGroup	= *(uint32*)argv[0];
	uint32		isInc			= *(uint32*)argv[1];
	T_FAMILYID	FID				= *(uint32*)argv[2];
	T_ROOMID	RoomID			= *(uint32*)argv[3];

	if (!nQueryResult)
		return;

	if (nCurActionGroup == 0 || nCurActionGroup == (uint32)(-1))
	{
		sipwarning(L"DB_EXE_PARAMQUERY return error failed in Shrd_Visit_Insert. nCurActionGroup=%d", nCurActionGroup);
		return;
	}

	INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	pFamilyInfo->m_nCurActionGroup = nCurActionGroup;
	SetActionGroupID(RoomID, FID, nCurActionGroup);

	if (isInc > 0)
	{
		CRoomWorld	*pRoomWorld = GetOpenRoomWorld(RoomID);
		if (pRoomWorld == NULL)
		{
			sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
			return;
		}

		pRoomWorld->m_Visits ++;
	}
}

void CRoomWorld::EnterFamily_Spec(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	// 사적자료공간정보 갱신
	UpdateHisSpaceInfoFroEnter(_FID);
	
	// 방안의 정보를 내려보낸다
	SendAllInRoomToOne(_FID, _sid);

	// 경험치변경처리
	if (CheckIfFamilyExpIncWhenFamilyEnter(_FID))
		ChangeFamilyExp(Family_Exp_Type_Visitroom, _FID);

	// the room exp message should be sent after the room info message.
	ChangeRoomExp(Room_Exp_Type_Visitroom, _FID);
	FamilyEnteredForExp(_FID);

	//// 방안의 부분품정보를 내려보낸다
	//SendItemPartInRoomToOne(_FID);

	// 방안의 고인정보를 내려보낸다
	SendDeceasedDataTo(_FID);

	//// send the open room's dead history data to client
	//SendDeceasedHisDataTo(_FID);

	// 제상의 제물목록을 내려보낸다
	//SendSacrifice(_FID);

	// 비석목록을 내려보낸다.
	SendTomb(_FID, _sid);

	// Ruyibei자료를 내려보낸다.
	SendRuyibei(_FID, _sid);

	// 금붕어정보를 내려보낸다
	SendGoldFishInfo(_FID, _sid);

	// RoomStore 의 상태정보
	SendRoomstoreStatus(_FID, _sid);

	// 행운자료를 내려보낸다.
	SendLuckList(_FID, _sid);

	// 조상탑정보를 내려보낸다.
	SendAncestorData(_FID, _sid);

	// 소원목록을 내려보낸다
	//SendWish(_FID);

	// 신비한 문정보를 내려보낸다
	//SendTGate(_FID);

	// Channel고유정보를 내려보낸다.
	SendRoomChannelData(_FID, _sid);

	// 방의 행사결과정보를 보낸다.
	SendActionResults(_FID, _sid);

	// 방의 공작새정보를 보낸다.
	SendPeacockType(_FID, _sid);

	// 방의 깜짝이벤트정보를 내려보낸다.
	SendRoomEventData(_FID, _sid);

	// 천원특사정보를 내려보낸다
	SendAuto2ResultDataTo(_FID, _sid);
	// 방의 방문회수를 증가시킨다.
	INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(_FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d, RoomID=%d", _FID, m_RoomID);
		return;
	}
	pFamilyInfo->m_nCurActionGroup = 0;

	if (CheckIfActSave(_FID))
	{
		pFamilyInfo->m_nCurActionGroup = GetActionGroupID(m_RoomID, _FID);

		if (pFamilyInfo->m_nCurActionGroup == 0)
		{
			MAKEQUERY_Shrd_Visit_Insert(strQ, m_RoomID, _FID);

			DBCaller->executeWithParam(strQ, DBCB_DBShrd_Visit_Insert,
				"DDDD",
				OUT_,			NULL,
				OUT_,			NULL,
				CB_,			_FID,
				CB_,			m_RoomID);
		}
	}
}

// 방의 고인정보를 내려보낸다
void	CRoomWorld::SendDeceasedDataTo(T_FAMILYID _FID)
{
	// 접속써비스를 얻는다
	TServiceId sid = GetServiceId(_FID);
	SendDeadInfo(_FID, m_RoomID, this, sid);
}

//static void	DBCB_DBDeadHisList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	uint32			ret		= *(uint32 *)argv[0];
//	T_ROOMID		RoomID	= *(uint32 *)argv[1];
//	T_FAMILYID		_FID	= *(T_FAMILYID *)argv[2];
//
//	FAMILY*	family = GetFamilyData(_FID);
//	if(family == NULL)
//		return;
//
//	TServiceId sid = family->m_FES;
//
//	std::vector<_entryDeadHis>	aryDeadHis;
//	uint16	count = 0;
//
//	if(nQueryResult)
//	{
//		if(ret >= 0)
//		{
//			uint32 nRows;	resSet->serial(nRows);
//			for ( uint32 i = 0; i < nRows; i ++)
//			{
//				uint32 _history_id;			resSet->serial(_history_id);
//				uint8	_dead_id;			resSet->serial(_dead_id);
//				uint32 _start_year;			resSet->serial(_start_year);
//				uint32 _end_year;			resSet->serial(_end_year);
//				T_COMMON_CONTENT _title;	resSet->serial(_title);
//
//				_entryDeadHis	info(_history_id, _dead_id, _start_year, _end_year, _title);
//				aryDeadHis.push_back(info);
//
//				count ++;
//			}
//		}
//	}
//
//	CMessage		msgout(M_SC_DECEASED_HIS_LIST);
//	msgout.serial(_FID);
//	msgout.serial(RoomID, count);
//	for(uint i = 0; i < count; i ++)
//	{
//		_entryDeadHis	&info = aryDeadHis[i];
//		msgout.serial(info.history_id, info.dead_id, info.start_year, info.end_year, info.title);
//	}
//
//	CUnifiedNetwork::getInstance()->send(sid, msgout);
//}

//// send the open room's dead history data to client
//void  CRoomWorld::SendDeceasedHisDataTo(T_FAMILYID _FID)
//{
//	uint8	deadid = 1;
//	MAKEQUERY_DeadHisList(strQ, m_RoomID, deadid);
//	DBCaller->executeWithParam(strQ, DBCB_DBDeadHisList,
//		"DDD",
//		OUT_,		NULL,
//		CB_,		m_RoomID,
//		CB_,		_FID);
//}

// 비석목록을 내려보낸다
void	CRoomWorld::SendTomb(T_FAMILYID _FID, SIPNET::TServiceId sid)
{
	// 메쎄지 만들기
	CMessage	mesOut(M_SC_TOMBSTONE);
	uint32 uZero = 0;
	mesOut.serial(_FID, uZero);	
	mesOut.serial(m_RoomID);

	MAP_ROOMTOMB::iterator	it;
	for (it = m_RoomTomb.begin(); it != m_RoomTomb.end(); it ++)
	{
		T_TOMBID			tombid = it->first;
		T_COMMON_CONTENT	tomb = it->second;
		mesOut.serial(tombid, tomb);
	}

	// 메쎄지송신
	CUnifiedNetwork::getInstance ()->send(sid, mesOut);
}

void CRoomWorld::SendGoldFishInfo(T_FAMILYID _FID, TServiceId sid)
{
	uint32	uZero = 0;

	TTime	curTime = GetDBTimeSecond();

	// 금붕어정보
	{
		for (std::map<uint32, GoldFishData>::iterator it = m_GoldFishData.begin(); it != m_GoldFishData.end(); it ++)
		{
			uint32	GoldFishScopeID = it->first;
			uint32	RemainSecond[MAX_GOLDFISH_COUNT];
			for (int i = 0; i < MAX_GOLDFISH_COUNT; i ++)
				RemainSecond[i] = it->second.m_GoldFishs[i].GoldFishDelTime - (uint32)curTime;

			CMessage	msgOut(M_SC_UPDATE_GOLDFISH);
			msgOut.serial(_FID, uZero, GoldFishScopeID);
			for (int i = 0; i < MAX_GOLDFISH_COUNT; i ++)
			{
				msgOut.serial(it->second.m_GoldFishs[i].GoldFishID, RemainSecond[i], it->second.m_GoldFishs[i].FID, it->second.m_GoldFishs[i].FName);
			}
			CUnifiedNetwork::getInstance()->send(sid, msgOut);
		}
	}
}

// 방의 행운상태를 내려보낸다.
void CRoomWorld::SendLuckList(T_FAMILYID FID, TServiceId sid)
{
	if ((m_LuckID_Lv3 == 0) && (m_LuckID_Lv4 == 0))
		return;

	uint32	diffSecond = 0;
	if (m_LuckID_Lv4 != 0)
		diffSecond = (uint32)(GetDBTimeSecond() - m_LuckID_Lv4_ShowTime);

	// 방의 행운상태를 내려보낸다.
	CMessage	msgOut(M_SC_LUCK_LIST);
	msgOut.serial(FID, m_LuckID_Lv3, m_LuckID_Lv4, diffSecond);
	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

void	CRoomWorld::SetRoomInfo( T_FAMILYID _FID, T_ROOMID	_RoomID, T_ROOMNAME	_RoomName, T_ROOMPOWERID _OpenProp, T_COMMON_CONTENT _RCom, std::string AuthKey)
{
	if (_FID != m_MasterID)
		return;

	m_RoomName = _RoomName;
	m_PowerID = _OpenProp;
	m_RoomComment = _RCom;
	m_AuthKey = AuthKey;

	NotifyChangeRoomInfoForManager();
}

void	CRoomWorld::SetDeadInfo( const _entryDead *_info)
{
	DECEASED *obj = NULL;
	if (_info->index == 1)
		obj = &m_Deceased1;
	if (_info->index == 2)
		obj = &m_Deceased2;
	if (obj == NULL)
		return;
	obj->m_bValid = true;
	obj->m_Pos = _info->index;
	obj->m_Name = _info->ucName;
	obj->m_Surnamelen = _info->surnamelen;
	obj->m_Sex = _info->sex;
	obj->m_Birthday = _info->birthDay;
	obj->m_Deadday = _info->deadDay;
    obj->m_His = _info->briefHistory;
	obj->m_Domicile = _info->domicile;
	obj->m_Nation = _info->nation;
	m_PhotoType = _info->photoType;

	NotifyChangeRoomInfoForManager();
}

void	CRoomWorld::DeleteDeadInfo( uint8 _index )
{
	DECEASED *obj = NULL;
	if (_index == 1)
		obj = &m_Deceased1;
	if (_index == 2)
		obj = &m_Deceased2;
	if (obj == NULL)
		return;
	obj->m_bValid = false;
	obj->m_Name = L"";
}


static void	DBCB_DBRoomMemorial(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	struct ROOMMEMO
	{
		T_COMMON_ID           memId;
		T_FAMILYID            vFId;
		T_FAMILYNAME          vFname;
		std::string           date;
		ucstring              memContent;
		uint8                 bSecret;

		ROOMMEMO(T_COMMON_ID _memId, T_FAMILYID  _vFId,	T_FAMILYNAME _vFname, std::string _date, ucstring _memContent, uint8 _bSecret) : memId(_memId),
			vFId(_vFId), vFname(_vFname), date(_date), memContent(_memContent), bSecret(_bSecret)  {}
	};
	typedef std::vector<ROOMMEMO> TROOMMEMOS;

	T_ROOMID		RoomID			= *(T_ROOMID *)argv[0];
	uint32			_PagePos		= *(uint32 *)argv[1];
	uint8			SearchType		= *(uint8 *)argv[2];
	T_FAMILYID		_FID			= *(T_FAMILYID *)argv[3];
	uint16			year			= *(uint16 *)argv[4];
	uint8			month			= *(uint8 *)argv[5];
	sint32			ret				= *(sint32 *)argv[6];
	uint32			allcount		= *(uint32 *)argv[7];
	uint32			curTypeCount	= *(uint32 *)argv[8];
	TServiceId		_sid(*(uint16 *)argv[9]);

	if ( !nQueryResult || ret == -1)
		return;	

	TROOMMEMOS roomMemoList;
	uint32 nRows;	resSet->serial(nRows);
	for ( uint32 i = 0; i < nRows; i ++)
	{
		T_COMMON_ID mid;				resSet->serial(mid);
		T_FAMILYID fid;					resSet->serial(fid);
		T_NAME	vname;					resSet->serial(vname);
		T_DATETIME mday;				resSet->serial(mday);
		T_COMMON_CONTENT content;		resSet->serial(content);
		uint8 bProtect;					resSet->serial(bProtect);

		ROOMMEMO memo( mid, fid, vname, mday, content, bProtect);
		roomMemoList.push_back(memo);				
	}

	CMessage msg(M_SC_ROOMMEMO);
	msg.serial(_FID, RoomID, _PagePos);
	msg.serial(allcount);
	msg.serial(curTypeCount);
	for ( uint i = 0; i < roomMemoList.size(); i++ )
	{
		msg.serial(roomMemoList[i].memId);
		msg.serial(roomMemoList[i].vFId);
		msg.serial(roomMemoList[i].vFname);
		msg.serial(roomMemoList[i].date);
		msg.serial(roomMemoList[i].memContent);
		msg.serial(roomMemoList[i].bSecret);
	}
	CUnifiedNetwork::getInstance ()->send(_sid, msg);
	//sipinfo(L"Send room memorial : FID=%d, RoomID=%d, PagePos=%d, Number=%d", _FID, RoomID, _PagePos, allcount); byy krs

}

//// 방에 남긴글을 요구한다
//void	CRoomWorld::ReqRoomMemorial(T_FAMILYID _FID, uint32 _PagePos, SIPNET::TServiceId _sid, uint32 year_month, uint8 SearchType)
//{
//	uint32	lyear, lmonth;
//	lyear = (year_month >> 16) & 0x00003FFF;
//	lmonth = (year_month >> 8) & 0x000000FF;
//	uint16 year = (uint16)lyear;
//	uint8  month = (uint8)lmonth;
//
//	MAKEQUERY_ROOMMEMORIAL(strQ);
//	DBCaller->executeWithParam(strQ, DBCB_DBRoomMemorial,
//		"DDBDWBDDDW",
//		IN_ | CB_,				m_RoomID,
//		IN_ | CB_,				_PagePos,
//		IN_ | CB_,				SearchType,
//		IN_ | CB_,				_FID,
//		IN_ | CB_,				year,
//		IN_ | CB_,				month,
//		OUT_,					NULL,
//		OUT_,					NULL,
//		OUT_,					NULL,
//		CB_,					_sid.get());
//}

//// 방의 페트상태를 갱신한다
//void	CRoomWorld::RefreshPet()
//{
//	uint32	curtime = (uint32)GetDBTimeSecond();
//
//	//// 물고기 매일 밤1시마다 일정한 값씩 경험치 감소한다. - 사양 없어짐
//	//if(curtime > m_FishNextUpdateTime)
//	//{
//	//	m_FishNextUpdateTime = (uint32)GetUpdateNightTime((TTime)curtime);
//	//	ChangeFishExp(-g_nFishExpReducePercent);
//	//}
//
//	// 꽃 DeleteTime 이 지나면 없어진다. - 체험방에서 꽃이 무한으로 있어야 한다는 사양때문에 이 처리를 하지 않는다.
///*	if((m_FlowerID > 0) && (curtime > m_FlowerDelTime))
//	{
//		m_FlowerID = 0;
//		m_FlowerFID = 0;
//		m_FlowerFamilyName = "";
//		ChangeFlowerLevel(0, 0);
//	}*/
//}

// 삭제 - Offline양도기능
//// 양도속성갱신
//void	CRoomWorld::RefreshGiveProperty()
//{
//	if (m_CurGiveID == 0)
//		return;
//
//	// 양도해제상태 검사
//	TTime	curtime = CTime::getLocalTime();
//	if (curtime < m_CurGiveTime + CHANGEMASTER_WAIT_TIME)	// 1분동안 응답이 없으면 양도 해제처리
//		return;
//
//	T_FAMILYID	_NextId = m_CurGiveID;
//	m_CurGiveID = 0;
//	FAMILY*	pInfo = GetFamilyData(m_MasterID);
//	if(pInfo == NULL)
//	{
//		sipwarning("GetFamilyData = NULL.  FID=%d", m_MasterID);
//		return;
//	}
//
//	uint32	uResult = 0; // 상대가 동의하자 읺았다는 메쎄지로 처리한다.
//	CMessage	msgOut(M_SC_CHANGEMASTER);
//	msgOut.serial(m_MasterID);
//	msgOut.serial(uResult, m_RoomID, _NextId);
//	CUnifiedNetwork::getInstance()->send(pInfo->m_FES, msgOut);
//}

void CRoomWorld::SendPeacockType(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	if (m_mapFamilys.size() <= 1)
		m_nPeacockType ++;

	CMessage	msgOut(M_SC_PEACOCK_TYPE);
	msgOut.serial(FID, m_nPeacockType);
	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

// 방의 유효방문일수를 증가시킨다
//void	CRoomWorld::IncRoomValidVisitsdays()
//{
//	T_ROOMVDAYS	olddays = m_VDays;
//	T_ROOMLEVEL uLevel0 = MakeRoomDegree(olddays);
//	
//	MAKEQUERY_INCROOMVISITSDAYS(strQ, m_RoomID);
//	DB_EXE_QUERY(DB, Stmt, strQ);
//	if (nQueryResult == true)
//	{
//		m_VDays ++;
//		uint32	curtime = (uint32)GetDBTimeSecond();
//		m_LastIncTime.timevalue = curtime;
//	}
//
//	T_ROOMLEVEL uLevel1 = MakeRoomDegree(m_VDays);
//	if (uLevel1 != uLevel0)
//	{
//		FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ROOMPRO)
//			msgOut.serial(m_RoomID, m_Exp, m_Virtue, m_Fame, m_Appreciation, uLevel1);
//		FOR_END_FOR_FES_ALL()
//
//		sipinfo(L"Send room degree change:RoomID=%d, Degree=%d", m_RoomID, uLevel1);
//	}
//}

//////////////////////////////////////////////////////////////////////////
void	CRoomWorld::SetTomb(T_FAMILYID _FID, T_TOMBID _tombid, T_COMMON_CONTENT _tomb, SIPNET::TServiceId _sid)
{
	if (_FID != m_MasterID)
	{
		FAMILY* pFamily = GetFamilyData(_FID);
		if (pFamily == NULL || !pFamily->m_bGM)	// GM은 Tomb를 변경시킬수 있게 한다. - Tomb의 날자를 /gm SetMonumentDate 2016-11-7 형식으로 설정할수 있다.
		{
			// 비법
			ucstring ucUnlawContent = SIPBASE::_toString(L"No room manager(SetTomb) : RoomID=%d", m_RoomID);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
			return;
		}
	}
	int nR;
	if (m_RoomTomb.size() >= MAX_TOMBNUMINONEROOM)
	{
		// 비법
		ucstring ucUnlawContent = SIPBASE::_toString(L"Too many tomb : RoomID=%d", m_RoomID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
		return;
	}

	MAP_ROOMTOMB::iterator	it = m_RoomTomb.find(_tombid);
	if (it != m_RoomTomb.end())
	{
		MAKEQUERY_UPDATETOMB(strQ, m_RoomID, _tombid, _tomb.c_str());
		DBCaller->execute(strQ);
		it->second = _tomb;
		nR = true;
	}
	else
	{
		MAKEQUERY_ADDTOMB(strQ, m_RoomID, _tombid, _tomb.c_str());
		DBCaller->execute(strQ);
		m_RoomTomb.insert( make_pair(_tombid, _tomb) );
		nR = true;
	}

	if (nR == true)
	{
		// 보내지 않는것으로 토의함
		//FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_TOMBSTONE)
		//	msgOut.serial(m_RoomID, _tombid, _tomb);
		//FOR_END_FOR_FES_ALL()

		//sipinfo(L"Set room tomb RoomID=%d, FID=%d", m_RoomID, _FID);
	}
}

void	CRoomWorld::SetRuyibei(uint32 _itemid, T_COMMON_CONTENT _ruyibei)
{
	{
		MAKEQUERY_MODIFYRUYIBEI(strQ, m_RoomID, _itemid, _ruyibei.c_str());
		DBCaller->execute(strQ);
	}

	m_RoomRuyibei.ItemID = _itemid;
	m_RoomRuyibei.Ruyibei = _ruyibei;

	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_RUYIBEI)
		msgOut.serial(_itemid);
	FOR_END_FOR_FES_ALL()
}

void	CRoomWorld::SendRuyibei(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	uint32	zero = 0;
	CMessage	msgOut(M_SC_RUYIBEI);
	msgOut.serial(_FID, zero, m_RoomRuyibei.ItemID);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

void	CRoomWorld::SendRuyibeiText(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	if (_FID != m_MasterID)
	{
		sipwarning("_FID != m_MasterID. RoomID=%d, MasterFID=%d, FID=%d", m_RoomID, m_MasterID, _FID);
		return;
	}

	CMessage	msgOut(M_SC_RUYIBEI_TEXT);
	msgOut.serial(_FID, m_RoomRuyibei.Ruyibei);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

//////////////////////////////////////////////////////////////////////////
void SendGiveRoomReq(T_FAMILYID toFID, T_ROOMID RoomID, T_SCENEID SceneID, T_COMMON_NAME RoomName, T_FAMILYID OwnerID, T_FAMILYNAME OwnerName, uint32 DBTimeMinute, TServiceId tsid)
{
	CMessage	msgOut(M_SC_CHANGEMASTER_REQ);
	msgOut.serial(toFID, RoomID, SceneID, RoomName, OwnerID, OwnerName, DBTimeMinute);
	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

uint32	NextFishFoodTime = 0;
//static void	DBCB_DBGetReceiveRooms(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID	FID		= *(T_FAMILYID *)argv[0];
//	TServiceId	tsid(*(uint16 *)argv[1]);
//
//	if (!nQueryResult)
//		return;
//
//	uint32 nRows;	resSet->serial(nRows);
//	for (uint32 i = 0; i < nRows; i ++)
//	{
//		T_ROOMID		RoomID;		resSet->serial(RoomID);
//		T_SCENEID		SceneID;	resSet->serial(SceneID);
//		T_COMMON_NAME	RoomName;	resSet->serial(RoomName);
//		T_FAMILYID		OwnerID;	resSet->serial(OwnerID);
//		T_FAMILYNAME	OwnerName;	resSet->serial(OwnerName);
//		T_DATETIME		curGiveTime;resSet->serial(curGiveTime);
//		uint32 nCurGiveTime = getMinutesSince1970(curGiveTime);
//
//		SendGiveRoomReq(FID, RoomID, SceneID, RoomName, OwnerID, OwnerName, nCurGiveTime, tsid);
//	}
//}

//// 사용자가 지역써버에 접속할때 양도를 받을것을 신청된 방목록을 보낸다.
//void SendReceiveRooms(T_FAMILYID FID, SIPNET::TServiceId tsid)
//{
//	MAKEQUERY_GetReceiveRooms(strQ, FID);
//	DBCaller->executeWithParam(strQ, DBCB_DBGetReceiveRooms,
//		"DW",
//		CB_,		FID,
//		CB_,		tsid.get());
//}

// 사용자가 기념관양도를 신청한 경우
void CRoomWorld::ChangeRoomMaster_Step1(T_FAMILYID _FID, T_FAMILYID _NextId, std::string &password2, SIPNET::TServiceId _tsid)
{
	if (_FID != m_MasterID)
	{
		// 비법발견
		ucstring ucUnlawContent = SIPBASE::_toString(L"You are not RoomMaster : RoomID=%d, FID=%d", m_RoomID, _FID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
		return;
	}

	// Offline양도기능
	//FAMILY *pNextInfo = GetFamilyData(_NextId);
	//if (pNextInfo == NULL)
	//	return;

	// Offline양도기능때문에 - SystemUser는 양도를 할수 없게 함.
	if (IsSystemAccount(m_MasterUserID))
	{
		// 방주인에게 통지
		uint32		result = E_ROOMGIVE_SYSTEMACCOUNT;
		CMessage	msgOut(M_SC_CHANGEMASTER);
		msgOut.serial(_FID, result, m_RoomID, _NextId);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		return;
	}

	uint32	uResult = ERR_NOERR;

	// 공짜방은 양도할수 없다는 사양이 있지만, 써버에서 검사하지 않아도 다른 문제가 없을것 같아 처리하지 않음.

	// 양도시간제한
	if (m_LastGiveTime.timevalue != 0)
	{
		TTime releasetime = m_LastGiveTime.timevalue + ROOMGIVELIMITTIME;
		TTime	curtime = GetDBTimeSecond();
		if (curtime < releasetime)
		{
			uResult = E_ROOMGIVE_LIMITTIME;
		}
	}

	// Offline 양도기능이 들어가면서 이 검사가 필요없어짐
	//if ((uResult == ERR_NOERR) && (m_CurGiveID != 0))
	//{
	//	// 현재 양도중이면,,,
	//	uResult = E_ROOMGIVE_ALREADY;
	//}
	//if((uResult == ERR_NOERR) && IsAnyFamilyOnRoomlife())
	//{
	//	// 현재 방에 사람이 있으면
	//	uResult = E_ROOMGIVE_NOEMPTY;
	//}

	if (uResult == ERR_NOERR)
	{
		m_CurGiveID = _NextId;
		m_CurGiveTime = CTime::getLocalTime();
	}

	if (uResult == ERR_NOERR)
	{
		FAMILY *	pFamily = GetFamilyData(_FID);
		if (pFamily == NULL)
		{
			sipwarning("pFamily = NULL. FID=%d", _FID);
			return;
		}
		ChangeRoomMaster_Step2(_FID, _NextId, ERR_NOERR);
		// TwoPassword 검사를 위한 파케트를 Welcome Service에 보낸다.
		/*
		CMessage	mesOut(M_LS_CHECKPWD2);
		uint8		reason = 1;
		uint16	sid = SIPNET::IService::getInstance()->getServiceId().get();
		mesOut.serial(pFamily->m_UserID, _FID, password2, reason, m_RoomID, _NextId, sid);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, mesOut);
		*/
	}
	else
	{
		// 오유처리
		CMessage	mesOut(M_SC_CHANGEMASTER);
		mesOut.serial(_FID);
		mesOut.serial(uResult, m_RoomID, _NextId);
		CUnifiedNetwork::getInstance()->send(_tsid, mesOut);
	}
}

static void	DBCB_DBGiveRoomStart(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			ret			= *(uint32 *)argv[0];
	T_FAMILYID		_NextId		= *(T_FAMILYID *)argv[1];
	T_ROOMID		RoomID		= *(T_ROOMID *)argv[2];

	if (!nQueryResult)
		return;

	if (ret != 0)
	{
		//switch (ret)
		//{
		//default:
			sipwarning("Shrd_Room_Give_Start return=%d", ret);
		//	break;
		//}
		return;
	}

	// 양도받는 사람이 로그인상태라면 직접 통지한다.
	FAMILY*	pTargetFamily = GetFamilyData(_NextId);
	if (pTargetFamily != NULL)
	{
		CRoomWorld* roomManager = GetOpenRoomWorld(RoomID);
		if (roomManager == NULL)
		{
			sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
			return;
		}
		SendGiveRoomReq(_NextId, RoomID, roomManager->m_SceneID, roomManager->m_RoomName, roomManager->m_MasterID, roomManager->m_MasterName, GetDBTimeMinute(), pTargetFamily->m_FES);
	}
}

// Welcome Service에서 TwoPassword검사결과를 받았다.
void CRoomWorld::ChangeRoomMaster_Step2(T_FAMILYID _FID, T_FAMILYID _NextId, uint32 uResult)
{
	m_CurGiveID = 0;

	if (_FID != m_MasterID)
	{
		// 비법발견
		sipwarning("_FID != m_MasterID, FID=%d, MasterID=%d", _FID, m_MasterID);
		return;
	}

	FAMILY*	pFamily = GetFamilyData(_FID);
	if(pFamily == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", _FID);
		return;
	}

	// Password2 가 틀린다면...
	if(uResult != ERR_NOERR)
	{
		CMessage	mesOut(M_SC_CHANGEMASTER);
		mesOut.serial(_FID);
		mesOut.serial(uResult, m_RoomID, _NextId);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, mesOut);
		return;
	}
	else
	{
		// Password2가 맞는 경우에도 응답Packet를 보내기로 함
		uResult = E_ROOMGIVE_PWD2_OK;
		CMessage	mesOut(M_SC_CHANGEMASTER);
		mesOut.serial(_FID);
		mesOut.serial(uResult, m_RoomID, _NextId);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, mesOut);
	}

	MAKEQUERY_GiveRoomStart(strQ, m_RoomID, _NextId);
	DBCaller->executeWithParam(strQ, DBCB_DBGiveRoomStart,
		"DDD",
		OUT_,		NULL,
		CB_,		_NextId,
		CB_,		m_RoomID);
}

// Answer의 웃 16비트는 SceneID이다.
void SendGiveRoomResultToMaster(T_ROOMID RoomID, T_ROOMNAME RoomName, T_FAMILYID masterFID, T_FAMILYID _NextId, uint32 Answer)
{
	FAMILY*	pFamily = GetFamilyData(masterFID);
	if (pFamily == NULL)
	{
		// Chit 에 써넣기
		ucstring	p4 = L"";
		SendChit(_NextId, masterFID, CHITTYPE_GIVEROOM_RESULT, RoomID, Answer, RoomName, p4);
	}
	else
	{
		// 직접 메쎄지 보내기
		Answer &= 0xFFFF; // SceneID 삭제
		CMessage	msgOut(M_SC_CHANGEMASTER);
		msgOut.serial(masterFID, Answer, RoomID, _NextId);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
	}

	//sipinfo(L"Change room master request: RoomID=%d, MasterFID=%d, ToFamily=%d, Result(SceneID<<16)=%d", RoomID, masterFID, _NextId, Answer); byy krs
}

static void	DBCB_DBGetOwnRoomOneForChangeMaster(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		_NextId			= *(T_FAMILYID *)argv[0];

	if (!nQueryResult)
		return;

	FAMILY*	pTargetFamily = GetFamilyData(_NextId);
	if (pTargetFamily == NULL)
		return;

	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows < 1)
		return;	

	T_SCENEID	idRoomType;					resSet->serial(idRoomType);
	T_DATETIME	creationtime;				resSet->serial(creationtime);
	T_DATETIME	termtime;					resSet->serial(termtime);
	T_ID		idRoom;						resSet->serial(idRoom);
	T_ROOMNAME	roomName;					resSet->serial(roomName);
	T_FLAG	idOpenability;					resSet->serial(idOpenability);
	string			openPwd;				resSet->serial(openPwd);
	T_FLAG	freeUse;						resSet->serial(freeUse);
	bool	bclosed;						resSet->serial(bclosed);
	T_DATETIME	renewtime;					resSet->serial(renewtime);
	T_COMMON_CONTENT roomCom;				resSet->serial(roomCom);
	uint32	vistis;							resSet->serial(vistis);
	uint8	deleteflag;						resSet->serial(deleteflag);
	T_DATETIME	deletetime;					resSet->serial(deletetime);
	uint32	SpaceCapacity;					resSet->serial(SpaceCapacity);
	uint32	SpaceUsed;						resSet->serial(SpaceUsed);
	T_ROOMLEVEL		level;					resSet->serial(level);
	T_ROOMEXP		exp;					resSet->serial(exp);
	uint8 deadSurnameLen1;					resSet->serial(deadSurnameLen1);
	T_DECEASED_NAME	deadname1;				resSet->serial(deadname1);
	uint32	deadBirthday1;					resSet->serial(deadBirthday1);
	uint32	deadDeadday1;					resSet->serial(deadDeadday1);
	uint8 deadSurnameLen2;					resSet->serial(deadSurnameLen2);
	T_DECEASED_NAME	deadname2;				resSet->serial(deadname2);
	uint32	deadBirthday2;					resSet->serial(deadBirthday2);
	uint32	deadDeadday2;					resSet->serial(deadDeadday2);
	uint8	_SamePromotionCount;			resSet->serial(_SamePromotionCount);
	uint8	PhotoType;						resSet->serial(PhotoType);

	uint32	nCreateTime = getMinutesSince1970(creationtime);
	uint32	nRenewTime = getMinutesSince1970(renewtime);
	uint32	nTermTime = getMinutesSince1970(termtime);
	//uint16		exp_percent = CalcRoomExpPercent(level, exp);

	T_FLAG	uState = bclosed ? ROOMSTATE_CLOSED : ROOMSTATE_NORMAL;

	CMessage msg(M_SC_SETROOMMASTER);
	msg.serial(_NextId);
	msg.serial(idRoom, idRoomType, nCreateTime, nTermTime, nRenewTime);
	msg.serial(roomName, idOpenability, openPwd, freeUse, uState, vistis, roomCom, deleteflag);
	msg.serial(SpaceCapacity, SpaceUsed, level, exp, PhotoType); // , exp_percent);
	msg.serial(deadname1, deadSurnameLen1, deadBirthday1, deadDeadday1);
	msg.serial(deadname2, deadSurnameLen2, deadBirthday2, deadDeadday2);
	uint32	SamePromotionCount = (uint32)_SamePromotionCount;
	msg.serial(SamePromotionCount);

	CUnifiedNetwork::getInstance()->send(pTargetFamily->m_FES, msg);
}

static void	DBCB_DBSetRoomMaster(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		ret			= *(uint32 *)argv[0];
	T_ROOMID	RoomID		= *(uint32 *)argv[1];
	T_FAMILYID	_NextId		= *(uint32 *)argv[2];
	TServiceId	tsid(*(uint16 *)argv[3]);

	if (!nQueryResult)
		return;

	if (ret != 0)
	{
		//switch (ret)
		//{
		//default:
			sipwarning("Shrd_room_setMaster return code=%d", ret);
		//	break;
		//}

		// 방을 넘겨받는데 실패한 경우에 넘겨받는 사람에게 알려준다.
		if (tsid.get() != 0)
		{
			uint16	room_kind = 0;	// M_SC_SETROOMMASTER 파케트에서 room_kind = 0 으로 보내는것으로 토의함
			CMessage	msgOut(M_SC_SETROOMMASTER);
			msgOut.serial(_NextId, RoomID, room_kind);
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		}

		return;
	}

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	// 원래의 방관리자들에게 방주인이 바뀜을 통지
	uint32 nRows;	resSet->serial(nRows);
	T_FAMILYID		FID;
	ucstring	p4 = L"";
	for (uint32 i = 0; i < nRows; i ++)
	{
		resSet->serial(FID);
		SendChit(FID, FID, CHITTYPE_MANAGER_DEL, RoomID, ROOMMANAGER_DEL_REASON_ROOMMASTERCHANGED, roomManager->m_RoomName, p4);
	}

	// log
	DBLOG_CHRoomMaster(roomManager->m_MasterID, roomManager->m_MasterName, L"", 0, _NextId, RoomID, roomManager->m_RoomName );

	// 방주인에게 통보
	SendGiveRoomResultToMaster(RoomID, roomManager->m_RoomName, roomManager->m_MasterID, _NextId, (roomManager->m_SceneID << 16) | 1);

	// Openroom 갱신
	resSet->serial(nRows);
	uint32	NewUserID = 0;
	T_FAMILYNAME	NewUserName = "";
	if (nRows > 0)
	{
		resSet->serial(NewUserID);
		resSet->serial(NewUserName);
	}
	else
	{
		sipwarning("nRows = 0!!!");
	}
	roomManager->m_MasterID = _NextId;
	roomManager->m_MasterUserID = NewUserID;
	roomManager->m_MasterName = NewUserName;
	roomManager->m_LastGiveTime.timevalue = (TTime)GetDBTimeSecond();

	// 방관리자목록 초기화
	roomManager->m_RoomManagerCount = 0;
//	memset(roomManager->m_RoomManagers, 0, sizeof(roomManager->m_RoomManagers));

	// 체험공간 목록에 있는 경우 정보 변경
	roomManager->NotifyChangeRoomInfoForManager();

	// 양도받는 사람에게 방정보를 보낸다.
	{
		MAKEQUERY_GetOwnRoomOne(strQuery, RoomID);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetOwnRoomOneForChangeMaster,
			"D",
			CB_,		_NextId);
	}
}

// 양도받는 사람에게서 응답을 받았다.
void CRoomWorld::ChangeRoomMaster_Step3(T_FAMILYID _NextId, uint32 Answer, SIPNET::TServiceId _tsid)
{
	if ((Answer == 0) || (Answer == 2))
	{
		// 상대가 동의하지 않음. 혹은 Timeout
		MAKEQUERY_GiveRoomCancel(strQ, m_RoomID);
		DBCaller->executeWithParam(strQ, NULL, 
			"D",
			OUT_,	NULL);

		// 방주인에게 통지
		SendGiveRoomResultToMaster(m_RoomID, m_RoomName, m_MasterID, _NextId, (m_SceneID << 16) | Answer);
		return;
	}

	// 상대가 동의한 경우
	MAKEQUERY_SetRoomMaster(strQ, m_RoomID, _NextId);
	DBCaller->executeWithParam(strQ, DBCB_DBSetRoomMaster,
		"DDDW",
		OUT_,		NULL,
		CB_,		m_RoomID,
		CB_,		_NextId,
		CB_,		_tsid.get());

	//if (_NextId != m_CurGiveID)
	//	return;

	//if (_FID != m_MasterID)
	//{
	//	// 비법발견
	//	sipwarning("_FID != m_MasterID, FID=%d, MasterID=%d", _FID, m_MasterID);
	//	return;
	//}

	//m_CurGiveID = 0;

	//FAMILY*	pSrcFamily = GetFamilyData(_FID);
	//if(pSrcFamily == NULL)
	//{
	//	sipwarning("GetFamilyData = NULL. FID=%d", _FID);
	//	return;
	//}

	//FAMILY*	pTargetFamily = GetFamilyData(_NextId);
	//if(pTargetFamily == NULL)
	//{
	//	sipwarning("GetFamilyData = NULL. FID=%d", _NextId);
	//	return;
	//}

	//if(bAnswer)
	//{
	//	// 상대가 동의한 경우
	//	{
	//		// DB 변경
	//		MAKEQUERY_SetRoomMaster(strQ, m_RoomID, _NextId);
	//		DBCaller->executeWithParam(strQ, DBCB_DBSetRoomMaster,
	//			"DS",
	//			CB_,		m_RoomID,
	//			CB_,		m_RoomName.c_str());
	//	}

	//	// Openroom 갱신
	//	m_MasterID = _NextId;
	//	m_MasterUserID = pTargetFamily->m_UserID;
	//	m_MasterName = pTargetFamily->m_Name;
	//	m_LastGiveTime.timevalue = (TTime)GetDBTimeSecond();

	//	memset(m_Managers, 0, sizeof(m_Managers));

	//	// 체험공간 목록에 있는 경우 정보 변경
	//	NotifyChangeJingPinTianYuanInfo(m_RoomID);

	//	// 양도받는 사람에게 방정보를 보낸다.
	//	{
	//		MAKEQUERY_GetOwnRoomOne(strQuery, m_RoomID);
	//		DBCaller->executeWithParam(strQuery, DBCB_DBGetOwnRoomOneForChangeMaster,
	//			"D",
	//			CB_,		_NextId);
	//	}

	//	// log
	//	DBLOG_CHRoomMaster(_FID, pSrcFamily->m_Name, L"", 0, _NextId, m_RoomID, m_RoomName );
	//}

	//// 방주인에게 통지
	//uint32		result = bAnswer;
	//CMessage	msgOut(M_SC_CHANGEMASTER);
	//msgOut.serial(_FID, result, m_RoomID, _NextId);
	//CUnifiedNetwork::getInstance()->send(pSrcFamily->m_FES, msgOut);

	//sipinfo(L"Change room master request: RoomID=%d, MasterID=%d, ToFamily=%d, Result=%d", m_RoomID, _FID, _NextId, bAnswer);
}

static void	DBCB_DBGiveRoomCancel(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		ret			= *(uint32 *)argv[0];
	T_ROOMID	RoomID		= *(uint32 *)argv[1];
	T_FAMILYID	FID			= *(uint32 *)argv[2];
	T_FAMILYID	_NextId		= *(uint32 *)argv[3];
	TServiceId	tsid(*(uint16 *)argv[4]);

	if (!nQueryResult)
		return;

	if (ret != 0)
	{
		//switch (ret)
		//{
		//default:
			sipwarning ("Shrd_Room_Give_Cancel return=%d", ret);
		//	break;
		//}
	}
	else
	{
		uint32		result = 2; // 시간이 지나서 혹은 다른 리유로 하여 방주인에 의하여 취소됨
		CMessage	msgOut(M_SC_CHANGEMASTER);
		msgOut.serial(FID, result, RoomID, _NextId);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
}

// 상대가 응답이 없어서 혹은 방주인이 양도를 취소하였다.
void CRoomWorld::ChangeRoomMaster_Cancel(T_FAMILYID _FID, T_FAMILYID _NextId, SIPNET::TServiceId _tsid)
{
	MAKEQUERY_GiveRoomCancel(strQ, m_RoomID);
	DBCaller->executeWithParam(strQ, DBCB_DBGiveRoomCancel, 
		"DDDDW",
		OUT_,	NULL,
		CB_,	m_RoomID,
		CB_,	_FID,
		CB_,	_NextId,
		CB_,	_tsid.get());
}

// 방주인변경
// Give room([d:Room id][d:toFID][s:TwoPassword])
void	cb_M_CS_CHANGEMASTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	T_ROOMID			RoomID;				_msgin.serial(RoomID);				
	T_FAMILYID			NextID;				_msgin.serial(NextID);			// 넘겨주려는 가족id
	string				password2;			NULLSAFE_SERIAL_STR(_msgin, password2, MAX_LEN_USER_PASSWORD, FID);
	uint16				wsid;				_msgin.serial(wsid);

	TServiceId	client_FsSid(wsid);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		roomManager->ChangeRoomMaster_Step1(FID, NextID, password2, client_FsSid);
	}
}

// Answer of M_SC_CHANGEMASTER_REQ ([d:Room id][b:Answer, 1:Yes,0:No])
void	cb_M_CS_CHANGEMASTER_ANS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ROOMID			RoomID;				_msgin.serial(RoomID);				
	uint8				Answer;				_msgin.serial(Answer);
	uint16				wsid;				_msgin.serial(wsid);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	TServiceId	client_FsSid(wsid);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		roomManager->ChangeRoomMaster_Step3(FID, Answer, client_FsSid);
	}
}

// Cancel giveroom (by timeout, etc...) ([d:Room id][d:toFID])
void	cb_M_CS_CHANGEMASTER_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ROOMID			RoomID;				_msgin.serial(RoomID);				
	T_FAMILYID			NextID;				_msgin.serial(NextID);
	uint16				wsid;				_msgin.serial(wsid);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	TServiceId	client_FsSid(wsid);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		roomManager->ChangeRoomMaster_Cancel(FID, NextID, client_FsSid);
	}
}

static void	DBCB_DBGiveRoomStart_Force(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			ret			= *(uint32 *)argv[0];
	T_FAMILYID		_NextId		= *(T_FAMILYID *)argv[1];
	T_ROOMID		RoomID		= *(T_ROOMID *)argv[2];

	if (!nQueryResult)
		return;
	if (ret != 0)
	{
		sipwarning("Shrd_Room_Give_Start return=%d", ret);
		return;
	}

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		roomManager->ChangeRoomMaster_Step3(_NextId, 1, TServiceId(0));
	}
}

void	cb_M_NT_CHANGEMASTER_FORCE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			_NextId;			_msgin.serial(_NextId);
	T_ROOMID			RoomID;				_msgin.serial(RoomID);

	MAKEQUERY_GiveRoomStart(strQ, RoomID, _NextId);
	DBCaller->executeWithParam(strQ, DBCB_DBGiveRoomStart_Force,
		"DDD",
		OUT_,		NULL,
		CB_,		_NextId,
		CB_,		RoomID);
}

void CRoomWorld::on_M_CS_FISH_FOOD(T_FAMILYID FID, uint32 StoreLockID, T_FITEMPOS InvenPos, T_ITEMSUBTYPEID _ItemID, uint32 FishScopeID, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	T_ID	ItemID = 0;
	const	MST_ITEM	*mstItem = NULL;
	const	OUT_PETCARE	*mstPetCare = NULL;
	uint32	UsedUMoney = 0, UsedGMoney = 0;
	if (StoreLockID != 0)
	{
		RoomStoreLockData*	lockData = GetStoreLockData(FID, StoreLockID);
		if (lockData == NULL)
		{
			sipwarning("GetStoreLockData = NULL. FID=%d, StoreLockID=%d", FID, StoreLockID);
			return;
		}
		if (lockData->ItemCount != 1)
		{
			sipwarning("lockData->ItemCount != 1. FID=%d, StoreLockID=%d, lockData->ItemCount=%d", FID, StoreLockID, lockData->ItemCount);
			return;
		}

		ItemID = lockData->ItemIDs[0];
		mstItem = GetMstItemInfo(ItemID);
		mstPetCare = GetOutPetCare(ItemID);
		if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_PET_TOOL) || (mstPetCare == NULL) || (mstPetCare->PetType != PETTYPE_FISHGROUP))
		{
			sipwarning(L"Invalid Item in Store. FID=%d, ItemID=%d", FID, ItemID);
			return;
		}

		// Check OK
		RoomStoreItemUsed(FID, StoreLockID);
	}
	else
	{
		if (InvenPos != 0xFFFF)
		{
			if (!IsValidInvenPos(InvenPos))
			{
				// 비법파케트발견
				ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos! UserFID=%d, InvenPos=%d", FID, InvenPos);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}

			_InvenItems*	pInven = GetFamilyInven(FID);
			if (pInven == NULL)
			{
				sipwarning("GetFamilyInven NULL");
				return;
			}
			int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
			ITEM_INFO	&item = pInven->Items[InvenIndex];
			ItemID = item.ItemID;

			mstItem = GetMstItemInfo(ItemID);
			mstPetCare = GetOutPetCare(ItemID);
			if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_PET_TOOL) || (mstPetCare == NULL) || (mstPetCare->PetType != PETTYPE_FISHGROUP))
			{
				sipwarning(L"Invalid Item. FID=%d, ItemID=%d", FID, ItemID);
				return;
			}

			// Check OK
			if(item.MoneyType == MONEYTYPE_UMONEY)
				UsedUMoney = mstItem->UMoney;
			else
				UsedGMoney = mstItem->GMoney;

			// Delete from Inven
			if (1 < item.ItemCount)
			{
				item.ItemCount --;
			}
			else
			{
				item.ItemID = 0;
			}

			if (!SaveFamilyInven(FID, pInven))
			{
				sipwarning("SaveFamilyInven failed. FID=%d", FID);
				return;
			}
		}
		else
		{
			ItemID = _ItemID;

			mstItem = GetMstItemInfo(ItemID);
			mstPetCare = GetOutPetCare(ItemID);
			if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_PET_TOOL) || (mstPetCare == NULL) || (mstPetCare->PetType != PETTYPE_FISHGROUP))
			{
				sipwarning(L"Invalid Item. FID=%d, ItemID=%d", FID, ItemID);
				return;
			}
		}
	}

	//// Prize처리 - M_SC_INVENSYNC 다음으로!!
	//CheckPrize(FID, PRIZE_TYPE_FISH, 2);
	//CheckYuWangPrize(FID);

	// 경험값 변경
	ChangeFamilyExp(Family_Exp_Type_Fish, FID);

	// 하루에 한방에서 물고기먹이를 줄수 있는 회수는 MAX_FISHFOOD_COUNT로 제한된다.
	if (m_nFishFoodCount < MAX_FISHFOOD_COUNT)
	{
		m_nFishFoodCount ++;
		ChangeFishExp(mstPetCare->IncLife, FID, FishScopeID, ItemID);
	}
	else
	{
		FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_UPDATE_FISH)
			msgOut.serial(m_FishExp, FID, FishScopeID, ItemID, m_nFishFoodCount);
		FOR_END_FOR_FES_ALL()
	}
	ChangeRoomExp(Room_Exp_Type_Fish, FID);

	if ((UsedUMoney != 0) || (UsedGMoney != 0))
	{
		ucstring itemInfo = mstItem->Name;
		ChangeRoomExp(Room_Exp_Type_UseMoney, FID, UsedUMoney, UsedGMoney, itemInfo);
	}

	// 기록 보관
	INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning(L"GetFamilyInfo = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
	}
	else
	{
		pFamilyInfo->DonActInRoom(ROOMACT_FOODFISH);

		MAKEQUERY_AddFishFlowerVisitData(strQ, pFamilyInfo->m_nCurActionGroup, FF_FISH_FOOD, ItemID, 0);
		DBCaller->execute(strQ);

		if ((m_RoomID > 0x01000000) && (m_mapFamilys.size() >= 2) && (m_FlowerDelTime > 0) && (m_FlowerDelTime + 34560000 < GetDBTimeSecond()))
		{
			//sipinfo("Give Fish Food to Old Room. RoomID=%d", m_RoomID); byy krs
			NextFishFoodTime = (uint32)GetDBTimeSecond() + 3600;
		}
	}

	if (SyncCode != 0)
	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
	}

	// Prize처리
	CheckPrize(FID, PRIZE_TYPE_FISH, 2);
	CheckYuWangPrize(FID);

	// Log
	DBLOG_ItemUsed(DBLSUB_USEITEM_FISHFOOD, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

BOOL CRoomWorld::IsChangeableFlower(T_FAMILYID FID)
{
	if((FID != m_MasterID) && (!IsRoomManager(FID)))
		return false;
	return true;
}

void CRoomWorld::on_M_CS_FLOWER_NEW(T_FAMILYID FID, uint32 StoreLockID, T_FITEMPOS InvenPos, T_ITEMSUBTYPEID _ItemID, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	// 꽃 새로 심기는 방주인이나 관리자가 아니더라도 할수 있다. 손님은 꽃이 없는 경우에만 새로 심을수 있다.
	if (IfFlowerExist())
	{
		if(!IsChangeableFlower(FID))
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("IsChangeableFlower failed (3)! RoomID=%d, FID=%d", m_RoomID, FID);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
	}

	RoomChannelData*	pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. FID=%d", FID);
		return;
	}

	if (pChannelData->m_FlowerUsingFID != FID)
	{
		// 비법파케트발견
		sipwarning("Flower Current Using!!! RoomID=%d, UsingFID=%d, FID=%d", m_RoomID, pChannelData->m_FlowerUsingFID, FID);
		return;
	}

	uint32	ItemID = 0;
	const	MST_ITEM	*mstItem=NULL;
	uint32	UsedUMoney = 0, UsedGMoney = 0;
	if (StoreLockID != 0)
	{
		RoomStoreLockData*	lockData = GetStoreLockData(FID, StoreLockID);
		if (lockData == NULL)
		{
			sipwarning("GetStoreLockData = NULL. FID=%d, StoreLockID=%d", FID, StoreLockID);
			return;
		}
		if (lockData->ItemCount != 1)
		{
			sipwarning("lockData->ItemCount != 1. FID=%d, StoreLockID=%d, lockData->ItemCount=%d", FID, StoreLockID, lockData->ItemCount);
			return;
		}

		ItemID = lockData->ItemIDs[0];
		mstItem = GetMstItemInfo(ItemID);
		const	OUT_PETSET	*mstPet = GetOutPetSet(ItemID);
		if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_PET) || (mstPet == NULL) || (mstPet->TyepID != PETTYPE_FLOWER))
		{
			sipwarning(L"Invalid Item in Store. FID=%d, ItemID=%d", FID, ItemID);
			return;
		}

		// Check OK
		RoomStoreItemUsed(FID, StoreLockID);
	}
	else
	{
		if (InvenPos != 0xFFFF)
		{
			if (!IsValidInvenPos(InvenPos))
			{
				// 비법파케트발견
				ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos! UserFID=%d, InvenPos=%d", FID, InvenPos);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}

			_InvenItems*	pInven = GetFamilyInven(FID);
			if (pInven == NULL)
			{
				sipwarning("GetFamilyInven NULL");
				return;
			}
			int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
			ITEM_INFO	&item = pInven->Items[InvenIndex];
			ItemID = item.ItemID;

			mstItem = GetMstItemInfo(ItemID);
			const	OUT_PETSET	*mstPet = GetOutPetSet(ItemID);
			if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_PET) || (mstPet == NULL) || (mstPet->TyepID != PETTYPE_FLOWER))
			{
				sipwarning(L"Invalid Item. FID=%d, ItemID=%d", FID, ItemID);
				return;
			}

			// Check OK
			if (item.MoneyType == MONEYTYPE_UMONEY)
				UsedUMoney = mstItem->UMoney;
			else
				UsedGMoney = mstItem->GMoney;

			// Delete from Inven
			if (1 < item.ItemCount)
			{
				item.ItemCount --;
			}
			else
			{
				item.ItemID = 0;
			}

			if (!SaveFamilyInven(FID, pInven))
			{
				sipwarning("SaveFamilyInven failed. FID=%d", FID);
				return;
			}
		}
		else
		{
			ItemID = _ItemID;
			mstItem = GetMstItemInfo(ItemID);
			const	OUT_PETSET	*mstPet = GetOutPetSet(ItemID);
			if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_PET) || (mstPet == NULL) || (mstPet->TyepID != PETTYPE_FLOWER))
			{
				sipwarning(L"Invalid Item. FID=%d, ItemID=%d", FID, ItemID);
				return;
			}
		}
	}

	if (SyncCode != 0)
	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
	}

	uint32	OldFlowerID = m_FlowerID;
	if ((m_FlowerID > 0) && (GetDBTimeSecond() >= m_FlowerDelTime))
		OldFlowerID = 0;
	// 꽃 추가
	{
		m_FlowerID = ItemID;
		m_FlowerLevel = 0;
		m_FlowerDelTime = (uint32)GetDBTimeSecond() + mstItem->UseTime * 60;	// mstItem->UseTime은 분단위이므로 초단위로 계산한다.
		m_FlowerFID = FID;
		m_FlowerFamilyName = "";
		INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
		if (pInfo != NULL)
			m_FlowerFamilyName = pInfo->m_Name;

		ChangeFlowerLevel(0, FID);
	}

	// 이전에 물준 흔적을 없앤다. 
	// - 2011/11/18 물준흔적은 그대로 둬두어야 한다. 
	// - 2011/12/08 원래대로 회복
	m_mapFamilyFlowerWaterTime.clear();

	// Prize처리
	CheckPrize(FID, PRIZE_TYPE_FLOWER, 5);

	// 경험값 변경
	ChangeFamilyExp(Family_Exp_Type_Flower_Set, FID);

	ChangeRoomExp(Room_Exp_Type_Flower_Set, FID);

	if ((UsedUMoney != 0) || (UsedGMoney != 0))
	{
		ucstring itemInfo = mstItem->Name;
		ChangeRoomExp(Room_Exp_Type_UseMoney, FID, UsedUMoney, UsedGMoney, itemInfo);
	}

	// 기록 보관
	INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning(L"GetFamilyInfo = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
	}
	else
	{
		pFamilyInfo->DonActInRoom(ROOMACT_NEWFLOWER);

		MAKEQUERY_AddFishFlowerVisitData(strQ, pFamilyInfo->m_nCurActionGroup, FF_FLOWER_NEW, ItemID, OldFlowerID);
		DBCaller->execute(strQ);
	}

	// Log
	DBLOG_ItemUsed(DBLSUB_USEITEM_NEWFLOWER, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

//////////////////////////////////////////////////////////////////////////
void	CRoomWorld::SendRoomTermTimeToAll(T_DATETIME _termTime)
{
	m_BackDay		= _termTime;
	m_BackTime.SetTime(_termTime);

	// 보내지 않는것으로 토의함
//	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ROOMTERMTIME)
//		msgOut.serial(m_RoomID, _termTime);
//	FOR_END_FOR_FES_ALL();
//	sipinfo(L"Send room term time : RoomID=%d, Termtime=%s", m_RoomID, ucstring(_termTime).c_str());
}

T_ROOMNAME		CRoomWorld::GetRoomNameForLog()
{
	T_ROOMNAME	roomName = m_RoomName;
	if (roomName == L"")
	{
		MST_ROOM	*room	= GetMstRoomData(m_SceneID);
		if ( room != NULL )
			roomName = room->m_Name;
	}
	return roomName;
}

void	CRoomWorld::PullFamily(T_FAMILYID _GMID, T_FAMILYID _FID, T_CH_DIRECTION _Dir, T_CH_X _X, T_CH_Y _Y, T_CH_Z _Z)
{
	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator	itObject = m_mapFamilys.find(_FID);
	if (itObject == m_mapFamilys.end())
		return;
	INFO_FAMILYINROOM *pObject = &(itObject->second);
	bool bR2 = pObject->SetFirstCHState(_Dir, _X, _Y, _Z);
	if (!bR2)
		return;

	uint32	ChannelID = pObject->m_nChannelID;
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		return;
	}
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_GMSC_MOVETO)
		map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator	itC = itObject->second.m_mapCH.begin();
		msgOut.serial(itC->second.m_CHID, itC->second.m_Direction, itC->second.m_X, itC->second.m_Y, itC->second.m_Z);
	FOR_END_FOR_FES_ALL()

	// GMLog
	GMLOG_PullFamily(_GMID, _FID, m_RoomID, GetRoomNameForLog());
}

//void	CRoomWorld::ForceOut(T_FAMILYID _FID, T_FAMILYID _ObjectID, SIPNET::TServiceId _sid)
//{
//	if((_FID != m_MasterID) && !IsRoomManager(_FID))
//	{
//		// 비법
//		ucstring ucUnlawContent = SIPBASE::_toString(L"No room manager(ForceOut) : RoomID=%d", m_RoomID);
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
//		return;
//	}	
//
//	if ((_ObjectID != m_MasterID) || IsRoomManager(_ObjectID))
//		sipwarning(L"Can't force out. He is RoomManager. : ObjectID=%d", _ObjectID);
//	else
//		ForceOutRoom(_FID, _ObjectID, _sid);
//}

static void	DBCB_DBFamilyCreateRooms(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			FID			= *(T_FAMILYID *)argv[0];
	T_FAMILYID			ObjectID	= *(T_FAMILYID *)argv[1];
	TServiceId			tsid(*(uint16 *)argv[2]);

	if (!nQueryResult)
		return;

	uint8	flagStart, flagEnd;

	flagStart = 1;
	CMessage mes(M_SC_ROOMFORFAMILY);
	mes.serial(FID, ObjectID, flagStart);

	uint32 nRows;	resSet->serial(nRows);
	for ( uint32 i = 0; i < nRows; i ++)
	{
		T_MSTROOM_ID	SceneID;				resSet->serial(SceneID);
		T_DATETIME		creattime;				resSet->serial(creattime);
		T_DATETIME		termtime;				resSet->serial(termtime);
		T_ROOMID		id;						resSet->serial(id);
		T_ROOMNAME		roomName;				resSet->serial(roomName);
		T_ROOMPOWERID	opentype;				resSet->serial(opentype);
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
		uint8				DeadSurnameLen2;	resSet->serial(DeadSurnameLen2);
		T_DECEASED_NAME		Deadname2;			resSet->serial(Deadname2);
		uint8				PhotoType;			resSet->serial(PhotoType);

		if(bclosed || deleteflag)
			continue;

		if(mes.length() > 2500)
		{
			flagEnd = 0;
			mes.serial(flagEnd);
			CUnifiedNetwork::getInstance()->send(tsid, mes);

			CMessage mesTemp(M_SC_ROOMFORFAMILY);
			flagStart = 0;
			mesTemp.serial(FID, ObjectID, flagStart);

			mes = mesTemp;
		}

		//uint16	exp_percent = CalcRoomExpPercent(level, exp);
		mes.serial(id, SceneID, roomName, opentype);
		mes.serial(level, exp, /*exp_percent,*/ visits, PhotoType);
		mes.serial(Deadname1, DeadSurnameLen1, Deadname2, DeadSurnameLen2);

	}

	flagEnd = 1;
	mes.serial(flagEnd);
	CUnifiedNetwork::getInstance()->send(tsid, mes);
}

// void	GetRoomInfoForFamily(T_FAMILYID _FID, T_FAMILYID _ObjectID, SIPNET::TServiceId _sid)
// {
// 	CMessage mes(M_SC_ROOMFORFAMILY);
// 	mes.serial(_FID, _ObjectID);
// 	{
// 		MAKEQUERY_GETOWNCREATEROOM(strQuery, _ObjectID);
// 		DB_EXE_QUERY(DB, Stmt, strQuery);
// 		do
// 		{
// 			DB_QUERY_RESULT_FETCH(Stmt, row);
// 			if (IS_DB_ROW_VALID(row))
// 			{
// 				COLUMN_DIGIT(row, 0, T_MSTROOM_ID, SceneID);
// 				COLUMN_DIGIT(row, 3, T_ROOMID,	id);
// 				COLUMN_WSTR(row, 4,	roomName,	MAX_LEN_ROOM_NAME);
// 				COLUMN_DIGIT(row, 5, T_ROOMPOWERID,	opentype);
// 				COLUMN_DIGIT(row,	7,	bool,	bclosed);
// 				COLUMN_DIGIT(row, 10, uint32, visits);
// 				COLUMN_DIGIT(row,	11,	uint8,	deleteflag);
// 				COLUMN_DIGIT(row, 13, uint8, DeadSurnameLen);
// 				COLUMN_WSTR(row, 14, Deadname,	MAX_LEN_DEAD_NAME);
// 				COLUMN_DIGIT(row,	17,	T_ROOMLEVEL,	level);
// 				COLUMN_DIGIT(row,	18,	T_ROOMEXP,	exp);
// 
// 				if(bclosed || deleteflag)
// 					continue;
// 
// 				uint16	exp_percent = CalcRoomExpPercent(level, exp);
// 				mes.serial(id, SceneID, roomName, opentype, Deadname, DeadSurnameLen);
// 				mes.serial(level, exp, exp_percent, visits);
// 			}
// 			else
// 				break;
// 		} while(true);
// 	}
// 
// 	CUnifiedNetwork::getInstance ()->send(_sid, mes);
// }

//void	CRoomWorld::GetFamilyComment(T_FAMILYID _FID, T_FAMILYID _ObjectID, SIPNET::TServiceId _sid)
//{
//	TRoomFamilys::iterator it = m_mapFamilys.find(_ObjectID);
//	if (it == m_mapFamilys.end())
//		return;
//	
//	CMessage mes(M_SC_ROOMFAMILYCOMMENT);
//	mes.serial(_FID, _ObjectID, it->second.m_ucComment);
//	CUnifiedNetwork::getInstance ()->send(_sid, mes);
//}

void	CRoomWorld::SetFamilyComment(T_FAMILYID _FID, ucstring _ucComment)
{
	TRoomFamilys::iterator it = m_mapFamilys.find(_FID);
	if (it == m_mapFamilys.end())
		return;
	it->second.m_ucComment = _ucComment;
}

//void	CRoomWorld::GiveRoomResult(T_FAMILYID _RecvID, bool _bRecv)
//{
//	m_bClosed = false;
//	//m_CurGiveID = NOFAMILYID;
//	//m_CurGiveTime.timevalue = 0;
//	if (_bRecv)
//	{
//		m_MasterID = _RecvID;
//		m_LastGiveTime.timevalue = (TTime)GetDBTimeSecond();
//		FAMILY *family = GetFamilyData(_RecvID);
//		if (family)
//			m_MasterName = family->m_Name;
//	}
//}

struct _VisitMemo
{
	uint32		visit_id;
	uint8		action_type;
	string		datetime;
	uint8		secret;
	uint8		isshilded;
	ucstring	pray;
	uint8		item_count;
	uint32		items[MAX_ITEM_PER_ACTIVITY];
	_VisitMemo(uint32 _visit_id, uint8 _action_type, string _datetime, uint8 _secret, ucstring _pray,
		uint8 _item_count, uint32 *_items, uint8 _isshilded)
	{
		visit_id = _visit_id;
		action_type = _action_type;
		datetime = _datetime;
		secret = _secret;
		isshilded = _isshilded;
		pray = _pray;
		item_count = _item_count;
		memcpy((uint8*)(&items[0]), (uint8*)_items, sizeof(uint32) * _item_count);
	}
};

uint32 CRoomWorld::AddActionMemo(T_FAMILYID FID, uint8 action_type, uint32 ParentID, ucstring pray, uint8 secret, uint32 StoreLockID, uint16* invenpos, T_ITEMSUBTYPEID* _itemid, uint32 SyncCode, SIPNET::TServiceId _tsid)
{
//	CMessage msg(M_SC_ADDRACTIONMEMO);
	INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning(L"GetFamilyInfo = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return 0;
	}

	uint32	VisitID = 0;

	uint32	UsedUMoney = 0, UsedGMoney = 0;
	if (action_type == AT_VISIT)
	{
	}
	else
	{
		// 소비된 돈 계산
		uint8	itembuf[MAX_ITEM_PER_ACTIVITY * sizeof(uint32) + 1];
		itembuf[0] = 0;
		uint32	*itemlist = (uint32*)(&itembuf[1]);
		std::map<ucstring, uint8> itemInfoList;

		_InvenItems*	pInven = GetFamilyInven(FID);
		if (pInven == NULL)
		{
			sipwarning("GetFamilyInven NULL");
			return 0;
		}

		int		i;
		int		item_count = 0;
		bool	bSetItem = false;
		for (i = 0; i < MAX_ITEM_PER_ACTIVITY; i ++)
		{
			// 집체행사인 경우 참가자는 아이템 하나만 사용할수 있다.
			if (ParentID != 0 && i > 0)
				break;

			if (invenpos[i] == 0)
				continue;
			T_ITEMSUBTYPEID ItemID=0;
			if (invenpos[i] != 0xFFFF)
			{
				if (!IsValidInvenPos(invenpos[i]))
				{
					// 비법파케트발견
					ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos !!!! Pos=%d", invenpos[i]);
					RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
					return 0;
				}

				int	InvenIndex = GetInvenIndexFromInvenPos(invenpos[i]);
				ITEM_INFO	&item = pInven->Items[InvenIndex];

				ItemID = item.ItemID;
				
				const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
				if (mstItem == NULL)
				{
					sipwarning(L"GetMstItemInfo = NULL. ItemID=%d, FID=%d, action_type=%d, StoreLockID=%d, i=%d, invenpos[i]=%d, InvenIndex=%d", ItemID, FID, action_type, StoreLockID, i, invenpos[i], InvenIndex);
					continue;
				}

				if (item.MoneyType == MONEYTYPE_UMONEY)
					UsedUMoney += mstItem->UMoney;
				else
					UsedGMoney += mstItem->GMoney;
			}
			else
			{
				if (_itemid != NULL)
					ItemID = _itemid[i];
			}

			const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
			if (mstItem == NULL)
			{
				sipwarning(L"GetMstItemInfo = NULL. ItemID=%d", ItemID);
				continue;
			}

			if (GetTaocanItemInfo(ItemID) != NULL)
				bSetItem = true;

			*itemlist = ItemID;
			itembuf[0] ++;
			itemlist ++;

			if (itemInfoList.find(mstItem->Name) == itemInfoList.end())
			{
				itemInfoList.insert( std::make_pair(mstItem->Name, 1) );
			}
			else
			{
				itemInfoList[mstItem->Name]++;			
			}

			item_count ++;
		}
		// Store Lock Items
		if (StoreLockID != 0)
		{
			RoomStoreLockData*	lockData = GetStoreLockData(FID, StoreLockID);
			if (lockData == NULL)
				return 0;

			for (i = 0; i < lockData->ItemCount; i ++)
			{
				// 집체행사인 경우 참가자는 아이템 하나만 사용할수 있다.
				if (ParentID != 0 && i > 0)
				{
					// StoreLockData는 따로따로 처리할수 없다;;
					sipwarning("ParentID != 0 && item_count > 0. FID=%d", FID);
					return 0;
				}

				const MST_ITEM *mstItem = GetMstItemInfo(lockData->ItemIDs[i]);
				if (mstItem == NULL)
				{
					sipwarning(L"GetMstItemInfo = NULL in StoreLockData. FID=%d, ItemID=%d", FID, lockData->ItemIDs[i]);
					continue;
				}

				if (GetTaocanItemInfo(lockData->ItemIDs[i]) != NULL)
					bSetItem = true;

				// 창고아이템은 반드시 UserMoney아이템이다.
				UsedUMoney += mstItem->UMoney;

				*itemlist = lockData->ItemIDs[i];
				itembuf[0] ++;
				itemlist ++;

				if (itemInfoList.find(mstItem->Name) == itemInfoList.end())
				{
					itemInfoList.insert( std::make_pair(mstItem->Name, 1) );
				}
				else
				{
					itemInfoList[mstItem->Name]++;			
				}

				item_count ++;
			}
		}

		bool	bActSave = CheckIfActSave(FID);
		if (action_type == AT_RUYIBEI)	// Ruyibei글쓰기
		{
			if (FID != m_MasterID)
			{
				sipwarning("Not RoomMaster (AT_RUYIBEI). MasterFID=%d, ReqFID=%d", m_MasterID, FID);
				return 0;
			}

			uint32	__ItemID = 0;
			if (itembuf[0] != 0)
			{
				itemlist = (uint32*)(&itembuf[1]);
				__ItemID = itemlist[0]; // 첫번째 아이템
			}
			if (__ItemID == 0)
			{
				sipwarning("Invalid ItemID. FID=%d, InvenPos=%d, ItemID=%d", FID, invenpos[0], __ItemID);
				return 0;
			}

			SetRuyibei(__ItemID, pray);
		}
		else if (action_type != AT_RUYIBEI && bActSave)	// Ruyibei글쓰기는 기록을 남기지 않는다.
		{
			uint32	__ItemID = 0;
			if (itembuf[0] != 0)
			{
				itemlist = (uint32*)(&itembuf[1]);
				__ItemID = itemlist[0]; // 첫번째 아이템
			}

			if (ParentID != 0)
			{
				tchar strQuery[1024] = _t("");
				if (IsSystemRoom(m_RoomID))
				{
					MAKEQUERY_AddMultiVisitMemo_Aud(strQ, pFamilyInfo->m_nCurActionGroup, ParentID, FID, secret, __ItemID, pray);
					wcscpy(strQuery, strQ);
				}
				else
				{
					MAKEQUERY_AddMultiVisitMemo(strQ, pFamilyInfo->m_nCurActionGroup, ParentID, FID, secret, __ItemID, pray);
					wcscpy(strQuery, strQ);
				}
				DBCaller->execute(strQuery);
			}
			else if (pFamilyInfo->m_nCurActionGroup != 0)
			{
				tchar strQuery[1024] = _t("");
				if (IsSystemRoom(m_RoomID))
				{
					MAKEQUERY_AddVisitMemo_Aud(strQ, pFamilyInfo->m_nCurActionGroup, action_type, pray, secret);
					wcscpy(strQuery, strQ);
				}
				else
				{
					MAKEQUERY_AddVisitMemo(strQ, pFamilyInfo->m_nCurActionGroup, action_type, pray, secret);
					wcscpy(strQuery, strQ);
				}
				
				CDBConnectionRest	DB(DBCaller);
				DB_EXE_PREPARESTMT(DB, Stmt, strQuery);
				if (!nPrepareRet)
				{
					sipwarning(L"DB_EXE_PREPARESTMT failed in Shrd_Visit_List_Insert.");
					return 0;
				}

				SQLLEN	len1 = itembuf[0] * sizeof(uint32) + 1, len2 = 0;
				uint32	ret = 0;

				DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_ALBUM_BUF_SIZE, 0, itembuf, 0, &len1);
				DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len2);

				if (!nBindResult)
				{
					sipwarning(L"DB_EXE_BINDPARAM_IN failed in Shrd_Visit_List_Insert.");
					return 0;
				}

				DB_EXE_PARAMQUERY(Stmt, strQuery);
				if (!nQueryResult)
				{
					sipwarning(L"DB_EXE_PARAMQUERY failed in Shrd_Visit_List_Insert.");
					return 0;
				}

				if(ret == 0 || ret == (uint32)(-1))
				{
					sipwarning(L"DB_EXE_PARAMQUERY return error failed in Shrd_Visit_List_Insert.");
					return 0;
				}
				VisitID = ret;
			}

			// Action결과물 처리
			if (action_type == AT_BURNING)
			{
				if (item_count > 0)
				{
					int xianbao_result_type = XianbaoResultType_Large;
					if (!bSetItem) // 세트아이템인 경우에는 대형으로 처리한다.
					{
						if (item_count <= 3)
							xianbao_result_type = XianbaoResultType_Small;
					}
					m_ActionResults.registerXianbaoResult(xianbao_result_type, VisitID, FID, pFamilyInfo->m_Name);
				}
			}
			else if (action_type == AT_SIMPLERITE)
			{
				if (item_count > 0)
				{
					int yishi_result_type = YishiResultType_Large;
					if (!bSetItem) // 세트아이템인 경우에는 대형으로 처리한다.
					{
						if (item_count <= 3)
							yishi_result_type = YishiResultType_Small;
						else if (item_count <= 6)
							yishi_result_type = YishiResultType_Medium;
					}
					m_ActionResults.registerYishiResult(yishi_result_type, VisitID, FID, pFamilyInfo->m_Name);
				}
			}
			else if (action_type == AT_XDGRITE)
			{
				m_ActionResults.registerYishiResult(YishiResultType_XDG, VisitID, FID, pFamilyInfo->m_Name);
			}
			else if (action_type == AT_MULTIRITE)
			{
				if (ParentID == 0) // 집체행사의 원주만 행사결과물을 남긴다.
					m_ActionResults.registerYishiResult(YishiResultType_Multi, VisitID, FID, pFamilyInfo->m_Name);
			}
			else if (action_type == AT_XDGMULTIRITE)
			{
				if (ParentID == 0) // 집체행사의 원주만 행사결과물을 남긴다.
					m_ActionResults.registerYishiResult(YishiResultType_XDG_Multi, VisitID, FID, pFamilyInfo->m_Name);
			}
			else if (action_type == AT_ANCESTOR_JISI)
			{
				if (item_count > 0)
				{
					int yishi_result_type = YishiResultType_Large;
					if (!bSetItem) // 세트아이템인 경우에는 대형으로 처리한다.
					{
						if (item_count <= 3)
							yishi_result_type = YishiResultType_Small;
						else if (item_count <= 6)
							yishi_result_type = YishiResultType_Medium;
					}
					m_ActionResults.registerAncestorJisiResult(yishi_result_type, VisitID, FID, pFamilyInfo->m_Name);
				}
			}
			else if (action_type == AT_ANCESTOR_MULTIJISI)
			{
				if (ParentID == 0) // 집체행사의 원주만 행사결과물을 남긴다.
					m_ActionResults.registerAncestorJisiResult(YishiResultType_Multi, VisitID, FID, pFamilyInfo->m_Name);
			}
			else if ((action_type >= AT_ANCESTOR_DECEASED_XIANG) && (action_type < AT_ANCESTOR_DECEASED_XIANG + MAX_ANCESTOR_DECEASED_COUNT))
			{
				m_ActionResults.registerAncestorDeceasedXiangResult(action_type - AT_ANCESTOR_DECEASED_XIANG, __ItemID, pFamilyInfo->m_Name);
			}
			else if ((action_type >= AT_ANCESTOR_DECEASED_HUA) && (action_type < AT_ANCESTOR_DECEASED_HUA + MAX_ANCESTOR_DECEASED_COUNT))
			{
				m_ActionResults.registerAncestorDeceasedHuaResult(action_type - AT_ANCESTOR_DECEASED_HUA, __ItemID, pFamilyInfo->m_Name);
			}
			else if (action_type == AT_TIANYUANXIANG)
			{
			}
			else if (action_type == AT_MUSICRITE)
			{
			}
			else
			{
				sipwarning("Unknown ActionType. action_type=%d", action_type);
			}
		}

		// Delete from Inven
		bool	bInvenChanged = false;
		for (i = 0; i < MAX_ITEM_PER_ACTIVITY; i ++)
		{
			if (ParentID != 0 && i > 0)
				break;

			if (invenpos[i] == 0 || invenpos[i] == 0xFFFF)
				continue;

			if (!IsValidInvenPos(invenpos[i]))
			{
				continue;
			}

			int	InvenIndex = GetInvenIndexFromInvenPos(invenpos[i]);
			ITEM_INFO	&item = pInven->Items[InvenIndex];

			// Log
			const MST_ITEM *mstItem = GetMstItemInfo(item.ItemID);
			if (mstItem == NULL)
				continue;

			bInvenChanged = true;
			if (action_type == AT_SIMPLERITE || action_type == AT_XDGRITE)
				DBLOG_ItemUsed(DBLSUB_USEITEM_SIMPLERITE, FID, 0, item.ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			else if (action_type == AT_BURNING)
				DBLOG_ItemUsed(DBLSUB_USEITEM_BURN, FID, 0, item.ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			else if (action_type == AT_MULTIRITE || action_type == AT_XDGMULTIRITE)
				DBLOG_ItemUsed(DBLSUB_USEITEM_MULTIRITE, FID, 0, item.ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			else if (action_type == AT_ANCESTOR_JISI)
				DBLOG_ItemUsed(DBLSUB_USEITEM_SIMPLEJISI, FID, 0, item.ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			else if (action_type == AT_ANCESTOR_MULTIJISI)
				DBLOG_ItemUsed(DBLSUB_USEITEM_MULTIJISI, FID, 0, item.ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			else if (action_type == AT_ANCESTOR_DECEASED_HUA)
				DBLOG_ItemUsed(DBLSUB_USEITEM_ANCESTOR_HUA, FID, 0, item.ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			else if (action_type == AT_ANCESTOR_DECEASED_XIANG)
				DBLOG_ItemUsed(DBLSUB_USEITEM_ANCESTOR_XIANG, FID, 0, item.ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			else if (action_type == AT_RUYIBEI)
				DBLOG_ItemUsed(DBLSUB_USEITEM_RUYIBEI, FID, 0, item.ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);

			if (1 < item.ItemCount)
			{
				item.ItemCount --;

				//sipdebug("InvenIndex=%d, item.ItemCount=%d", InvenIndex, item.ItemCount); byy krs
			}
			else
			{
				if (item.ItemID == 0)
				{
					sipwarning("=======; item.ItemID = 0 ;==========");
				}
				else
				{
					item.ItemID = 0;
					sipdebug("InvenIndex=%d, item.ItemCount=0", InvenIndex);
				}
			}
		}
		if (bInvenChanged)
		{
			if (!SaveFamilyInven(FID, pInven))
			{
				sipwarning("SaveFamilyInven failed. FID=%d", FID);
			}
		}

		// StoreLockData 처리
		if (StoreLockID != 0)
			RoomStoreItemUsed(FID, StoreLockID);

		//// Prize 처리 - M_SC_INVENSYNC 다음으로!!
		//if (action_type == AT_BURNING)
		//{
		//	CheckPrize(FID, PRIZE_TYPE_XIANBAO, UsedUMoney);
		//}
		//else if (action_type == AT_SIMPLERITE)
		//{
		//	CheckPrize(FID, PRIZE_TYPE_YISHI, UsedUMoney);
		//}

		// For EXP

		// 경험값 변경
		if ((action_type == AT_SIMPLERITE) || action_type == AT_ANCESTOR_JISI || action_type == AT_XDGRITE)
		{
			// 개별행사
			ChangeRoomExp(Room_Exp_Type_Jisi_One, FID);
			ChangeFamilyExp(Family_Exp_Type_Jisi_One, FID);
			pFamilyInfo->DonActInRoom(ROOMACT_YISHI);
		}
		else if (action_type == AT_BURNING)
		{
			// 개별물건태우기
			ChangeRoomExp(Room_Exp_Type_Xianbao, FID);
			ChangeFamilyExp(Family_Exp_Type_Xianbao, FID);
			pFamilyInfo->DonActInRoom(ROOMACT_XIANBAO);
		}
		else if ((action_type == AT_MULTIRITE) || action_type == AT_ANCESTOR_MULTIJISI || action_type == AT_XDGMULTIRITE)
		{
			// 개별행사
			if (ParentID == 0)
				ChangeRoomExp(Room_Exp_Type_Jisi_One, FID);

			if (ParentID == 0)
				ChangeFamilyExp(Family_Exp_Type_Jisi_One, FID);
			else
				ChangeFamilyExp(Family_Exp_Type_Jisi_One_Guest, FID);

			pFamilyInfo->DonActInRoom(ROOMACT_YISHI);
		}
		else if (action_type == AT_TIANYUANXIANG)
		{
			ChangeFamilyExp(Family_Exp_Type_ReligionTouXiang, FID);
		}
		if (((UsedUMoney != 0) || (UsedGMoney != 0)))
		{
			ucstring itemInfo = "";
			for (std::map<ucstring, uint8>::iterator it = itemInfoList.begin(); it != itemInfoList.end(); ++it)
			{
				itemInfo += L"[" + it->first + L"]" + SIPBASE::toString("%u", it->second);
			}
			ChangeRoomExp(Room_Exp_Type_UseMoney, FID, UsedUMoney, UsedGMoney, itemInfo);
		}
	}

	if ((SyncCode > 0) && (_tsid.get() != 0))
	{
		CMessage msgOut(M_SC_INVENSYNC);
		uint32		ret = ERR_NOERR;
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	{
		// Prize 처리
		if (action_type == AT_BURNING)
		{
			CheckPrize(FID, PRIZE_TYPE_XIANBAO, UsedUMoney);
		}
		else if (action_type == AT_SIMPLERITE)
		{
			CheckPrize(FID, PRIZE_TYPE_YISHI, UsedUMoney);
		}
	}

	return VisitID;
}

bool CRoomWorld::DelLetter(T_FAMILYID FID, uint32 letter_id)
{
	MAKEQUERY_DeletePaper(strQ, letter_id, FID);
	DBCaller->execute(strQ);

	return true;
}

void CRoomWorld::ModifyLetter(T_FAMILYID FID, uint32 letter_id, ucstring title, ucstring content, uint8 opened, SIPNET::TServiceId _tsid)
{
	VEC_LETTER_MODIFY_LIST::iterator it;
	for(it = m_LetterModifyList.begin(); it != m_LetterModifyList.end(); it ++)
	{
		if((it->FID == FID) && (it->LetterID == letter_id))
			break;
	}
	if(it == m_LetterModifyList.end())
	{
		sipwarning("it == m_LetterModifyList.end(). FID=%d, LetterID=%d", FID, letter_id);
		return;
	}
	m_LetterModifyList.erase(it);

	MAKEQUERY_UpdatePaper(strQ, letter_id, title, content, opened, FID);
	DBCaller->executeWithParam(strQ, NULL,
		"D",
		OUT_,		NULL);
}

// Activity Time처리
void CRoomWorld::RefreshActivity()
{
	TTime curTime = CTime::getLocalTime();
	TServiceId	sidTemp(0);
	TmapRoomChannelData::iterator	itChannel;
	for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); itChannel ++)
	{
		RoomChannelData	*pChannelData = &itChannel->second;
		for(int index = 0; index < COUNT_ACTPOS - 1; index ++)
		{
			_RoomActivity	&Activity = pChannelData->m_Activitys[index];
			if(Activity.FID == 0)
				continue;
			if(Activity.StartTime == 0)	// 행사진행중이면
				continue;
			// 행사차례가 되였는데 계속 하지 않고있는 사용자 절단
			if(curTime - Activity.StartTime < MAX_ACTCAPTURE_TIME)
				continue;
			if (index == ACTPOS_MULTILANTERN)	// ACTPOS_MULTILANTERN은 무한대기한다.
				continue;
			ActWaitCancel(Activity.FID, (uint8)(index + 1), sidTemp, 0);
		}
	}
}

// 2014/08/20:start : 아이템없이 진행하는 행사들(실례로 향드리기나 여의비보기)을 갈라보기위한 함수
bool CheckActivityGongpinCount(uint8 ActPos, uint8 GongpinCount)
{return true;	// 미타미타한 코드이다. 정확하지 않으면 막는다!
	if (GongpinCount == 0)
	{
		if (ACTPOS_TOUXIANG_1 <= ActPos && ActPos <= ACTPOS_RUYIBEI) // 여의비, 향드리기, 아이템개수 0
			return true;
		if (ActPos == ACTPOS_WRITELETTER) // 2014/09/04:편지쓰기도 아이템 없어요
			return true;
	}
	if (GongpinCount > 0) // 다른 행사
		return true;
	return false;
}
// end

// 어느 한 사용자에게 현재 방의 활동들을 보내준다.
void CRoomWorld::SendActivityList(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	T_ID	ChannelID = GetFamilyChannelID(FID);
	if (ChannelID == 0)
	{
		sipwarning("GetFamilyChannelID = 0. FID=%d", FID);
		return;
	}

	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. RoomID=%d, FID=%d, ChannelID=%d", m_RoomID, FID, ChannelID);
		return;
	}

	if ((ChannelID == 1) && (m_AutoPlayUserList.size() > 0))
	{
		// 현재 방의 Auto목록을 내려보낸다.
		uint32	zero = 0;
		for (uint32 i = 0; i < m_AutoPlayUserList.size(); i ++)
		{
			AutoPlayUserInfo	&data = m_AutoPlayUserList[i];
			CMessage	msgOut(M_SC_AUTO_ENTER);
			msgOut.serial(FID, zero);
			msgOut.serial(data.AutoFID, data.FID, data.FamilyName, data.ModelID, data.DressID, data.FaceID, data.HairID);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}
	}

	uint32	uZero = 0;
	uint8	bZero = 0;
	for (int index = 0; index < COUNT_ACTPOS - 1; index ++)
	{
		_RoomActivity	&Activity = pChannelData->m_Activitys[index];
		if (Activity.FID == 0)
			continue;

		uint8	ActPos = (uint8)(index + 1);

		// 인사드리기활동은 내려보내지 않는다.
		if ((ActPos >= ACTPOS_XINGLI_1) && (ActPos <= ACTPOS_XINGLI_2))
			continue;

		// 조상탑 고인행사는 내려보내지 않는다.
		if ((ActPos >= ACTPOS_ANCESTOR_DECEASED_START) && (ActPos < ACTPOS_ANCESTOR_DECEASED_START + MAX_ANCESTOR_DECEASED_COUNT))
			continue;

		if (Activity.StartTime > 0)
		{
			if (IsSingleActionType(Activity.ActivityType))
				continue;
			if (Activity.ActivityStepParam.GongpinCount == 0)	// GongpinCount<>0 : M_CS_MULTIACT_GO상태임, M_SC_MULTIACT_STARTED를 보내야 한다.
				continue;
		}

		// 활동시작
		if (IsSingleActionType(Activity.ActivityType))
		{
			uint8 acttype = static_cast<uint8>(Activity.ActivityType);
 			uint8 bZero = 0;
			CMessage msg2(M_SC_ACT_START);
			msg2.serial(FID, uZero, ActPos, acttype, Activity.FID, bZero);//, Activity.FName);
			CUnifiedNetwork::getInstance()->send(_tsid, msg2);

			// 2014/07/01:start
			_ActivityStepParam* pYisiParam = &Activity.ActivityStepParam;
			CMessage msg3(M_SC_ACT_NOW_START);
			msg3.serial(FID, uZero, ActPos, bZero, acttype, pYisiParam->IsXDG, Activity.FID);
			msg3.serial(pYisiParam->GongpinCount);
			//if (pYisiParam->GongpinCount != 0)
			if ( CheckActivityGongpinCount(ActPos, pYisiParam->GongpinCount) )
			{
				std::list<uint32>::iterator itlist;
				for (itlist = pYisiParam->GongpinsList.begin(); itlist != pYisiParam->GongpinsList.end(); itlist ++)
				{
					uint32 curitem = *itlist;
					msg3.serial(curitem);
				}
				uint8	joinCount = 0;
				msg3.serial(pYisiParam->BowType, joinCount, pYisiParam->StepID);
			}
			else
			{
				sipwarning(L"gongpin count is invalid! ActPos=%d, Count=%d", ActPos, pYisiParam->GongpinCount);
				continue;
			}
			CUnifiedNetwork::getInstance()->send(_tsid, msg3);
			// end
		}
		else // if(Activity.ActivityType == ACTIVITY_TYPE_MULTI)
		{
			CMessage msg2(M_SC_MULTIACT_STARTED);
			uint8	Count = 0;
			for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if(Activity.ActivityUsers[j].FID != 0)
					Count ++;
			}

			uint32	zero = 0;
			uint8		ActType = static_cast<uint8>(Activity.ActivityType);
			msg2.serial(FID, zero, ActPos, ActType, Count);
			for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if (Activity.ActivityUsers[j].FID != 0)
				{
					msg2.serial(Activity.ActivityUsers[j].FID);
				}
			}
			uint8	zeroDeskNo = 0;
			msg2.serial(zeroDeskNo);

			CUnifiedNetwork::getInstance()->send(_tsid, msg2);

			// 2014/07/02:start
			if (Activity.StartTime == 0)
			{
				_ActivityStepParam* pActParam = &Activity.ActivityStepParam;
				if (pActParam->GongpinCount != 0)
				{
					uint32	uZero = 0;
					uint8	bZero = 0;
					CMessage msgOut(M_SC_ACT_NOW_START);
					msgOut.serial(FID, uZero, ActPos, bZero, ActType, pActParam->IsXDG, Activity.FID, pActParam->GongpinCount);
					std::list<uint32>::iterator itlist;
					for (itlist = pActParam->GongpinsList.begin(); itlist != pActParam->GongpinsList.end(); itlist ++)
					{
						uint32 curitem = *itlist;
						msgOut.serial(curitem);
					}
					Count --;	// Because ActivityUsers[0] = Host
					msgOut.serial(pActParam->BowType, Count);

					for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
					{
						uint8		PosIndex = j - 1;
						if (Activity.ActivityUsers[j].FID != 0)
						{
							msgOut.serial(Activity.ActivityUsers[j].FID, Activity.ActivityUsers[j].ItemID, PosIndex);
						}
					}

					msgOut.serial(pActParam->StepID);
					CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				}
				else
				{
					sipwarning(L"gongpin count is zero!");
// 					return;
				}
			}
			// end
		}

		//for (int i = 0; i < MAX_SCOPEMODEL_PER_ACTIVITY; i ++)
		//{
		//	if (Activity.ScopeModel[i] != "")
		//	{
		//		// 활동효과모델
		//		CMessage msg3(M_SC_ACT_ADD_MODEL);
		//		msg3.serial(FID, uZero, ActPos, Activity.FID, Activity.ScopeModel[i]);
		//		CUnifiedNetwork::getInstance()->send(_tsid, msg3);
		//	}
		//}

		// NPC정보
		//for (int npcid = 0; npcid < MAX_NPCCOUNT_PER_ACTIVITY; npcid ++)
		//{
		//	if(Activity.NPCs[npcid].Name == "")
		//		continue;

		//	_RoomActivity_NPC	&Activity_NPC = Activity.NPCs[npcid];

		//	// NPC보이기
		//	CMessage msg4(M_SC_ACT_NPC_SHOW);
		//	msg4.serial(FID, uZero);
		//	msg4.serial(ActPos, Activity_NPC.Name, Activity_NPC.X, Activity_NPC.Y, Activity_NPC.Z, Activity_NPC.Dir, Activity.FID, Activity_NPC.ItemID);
		//	CUnifiedNetwork::getInstance()->send(_tsid, msg4);
		//}

		// 제상에 있는 아이템목록
		//uint8 HoldItem_Count = Activity.HoldItem.size();
		//if (HoldItem_Count > 0)
		//{
		//	CMessage msg6(M_SC_ACT_ITEM_PUT);
		//	msg6.serial(FID, uZero, ActPos, uZero, HoldItem_Count);
		//	std::list<_RoomActivity_HoldedItem>::iterator itlist;
		//	for (itlist = Activity.HoldItem.begin(); itlist != Activity.HoldItem.end(); itlist ++)
		//	{
		//		_RoomActivity_HoldedItem& curitem = *itlist;
		//		msg6.serial(curitem.X, curitem.Y, curitem.Z, curitem.ItemID, 
		//			curitem.dirX, curitem.dirY, curitem.dirZ);
		//	}
		//	CUnifiedNetwork::getInstance()->send(_tsid, msg6);
		//}
	}
}

// 하나의 활동을 삭제하고, 그것을 사용자들에게 통지한다.
void CRoomWorld::CompleteActivity(uint32 ChannelID, int index, uint8 SuccessFlag)
{
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);		
		return;
	}

	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	uint8	ActPos = (uint8)(index + 1);
	//int		i;

	//for(i = 0; i < MAX_NPCCOUNT_PER_ACTIVITY; i ++)
	//{
	//	if(Activity.NPCs[i].Name != "")
	//	{
	//		// 사용자들에게 통지
	//		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACT_NPC_HIDE)
	//			msgOut.serial(ActPos, Activity.NPCs[i].Name, Activity.FID);
	//		FOR_END_FOR_FES_ALL()
	//	}
	//}

	//for(i = 0; i < MAX_SCOPEMODEL_PER_ACTIVITY; i ++)
	//{
	//	if(Activity.ScopeModel[i] != "")
	//	{
	//		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACT_DEL_MODEL)
	//			msgOut.serial(ActPos, Activity.FID, Activity.ScopeModel[i]);
	//		FOR_END_FOR_FES_ALL()
	//	}
	//}

	if(Activity.FID > 0)
	{
		uint8	bZero = 0;
		if(IsSingleActionType(Activity.ActivityType))
		{
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACT_RELEASE)
				msgOut.serial(ActPos, Activity.FID, bZero);//, Activity.FName);
			FOR_END_FOR_FES_ALL()
		}
		else if(IsMutiActionType(Activity.ActivityType))
		{
			uint8	Count = 0;
			for(int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if(Activity.ActivityUsers[j].FID != 0)
					Count ++;
			}
			{
				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTIACT_RELEASE)
					msgOut.serial(ActPos, SuccessFlag, Count);
					for(int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
					{
						if(Activity.ActivityUsers[j].FID != 0)
							msgOut.serial(Activity.ActivityUsers[j].FID);
					}
					msgOut.serial(bZero);
				FOR_END_FOR_FES_ALL()
			}
			/*{
				//uint32	zero = 0;
				//for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
				//{
				//	if (Activity.ActivityUsers[j].FID == 0)
				//		continue;
				//	if (pChannelData->IsChannelFamily(Activity.ActivityUsers[j].FID))
				//		continue;

				//	FAMILY	*pFamily = GetFamilyData(Activity.ActivityUsers[j].FID);
				//	if (pFamily == NULL)
				//	{
				//		sipwarning("GetFamilyData = NULL. FID=%d", Activity.ActivityUsers[j].FID);
				//		continue;
				//	}

				//	CMessage	msgOut(M_SC_MULTIACT_RELEASE);
				//	msgOut.serial(Activity.ActivityUsers[j].FID, zero, ActPos, SuccessFlag, Count);
				//	for (int k = 0; k < MAX_MULTIACT_FAMILY_COUNT; k ++)
				//	{
				//		if(Activity.ActivityUsers[k].FID != 0)
				//			msgOut.serial(Activity.ActivityUsers[k].FID);
				//	}
				//	CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
				//}
			}*/
		}
	}

	Activity.init(NOFAMILYID, AT_VISIT, 0,0);
}

int CRoomWorld::GetActivitingIndexOfUser(uint32 ChannelID, T_FAMILYID FID, bool bHostOnly)
{
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		//sipwarning("GetChannel = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		if ((FID >= AUTOPLAY_FID_START) && (FID < AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT)) { // by krs
			pChannelData = GetChannel(5001);
			if (pChannelData == NULL) {
				//sipwarning("GetChannel = NULL. RoomID=%d, ChannelID=5001", m_RoomID);
				return -1;
			}
		}
		
	}

	for(int i = 0; i < COUNT_ACTPOS - 1; i ++)
	{
		if(pChannelData->m_Activitys[i].FID == FID)
			return i;
		if(!bHostOnly && (IsMutiActionType(pChannelData->m_Activitys[i].ActivityType)))
		{
			for(int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if(pChannelData->m_Activitys[i].ActivityUsers[j].FID == FID)
					return i + (j << 16);
			}
		}
	}
	return -1;
}

int CRoomWorld::GetWaitingActivity(uint32 ChannelID, T_FAMILYID FID)
{
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		return -1;
	}

	TWaitFIDList::iterator	it;
	for(int i = 0; i < COUNT_ACTPOS - 1; i ++)
	{
		for(it = pChannelData->m_Activitys[i].WaitFIDs.begin(); it != pChannelData->m_Activitys[i].WaitFIDs.end(); it ++)
		{
			if(it->ActivityUsers[0].FID == FID)
				return i;
			if(IsMutiActionType(it->ActivityType))
			{
				for(int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
				{
					if(it->ActivityUsers[j].FID == FID)
						return i + (j << 16);
				}
			}
		}
	}
	return -1;
}

//int CRoomWorld::GetNpcidOfAct(uint32 ChannelID, int index, std::string &NPCName)
//{
//	RoomChannelData	*pChannelData = GetChannel(ChannelID);
//	if(pChannelData == NULL)
//	{
//		sipwarning("GetChannel = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
//		return -1;
//	}
//
//	int		npcid;
//	for(npcid = 0; npcid < MAX_NPCCOUNT_PER_ACTIVITY; npcid ++)
//	{
//		if(pChannelData->m_Activitys[index].NPCs[npcid].Name == NPCName)
//			return npcid;
//	}
//	return -1;
//}

void CRoomWorld::ChackActPosForActWaitCancel(T_FAMILYID FID, uint8 &ActPos)
{
	if (ActPos == ACTPOS_BLESS_LAMP)
	{
		RoomChannelData	*pChannelData = GetChannel(GetFamilyChannelID(FID));
		if (pChannelData == NULL)
		{
			return;
		}

		for (int i = 1; i < MAX_BLESS_LAMP_COUNT; i ++)
		{
			_RoomActivity	*Activity = &pChannelData->m_Activitys[ACTPOS_BLESS_LAMP + i - 1];
			if (Activity->FID == FID)
			{
				ActPos = ACTPOS_BLESS_LAMP + i;
				//sipinfo(L"ActPos Changed. %d->%d", ACTPOS_BLESS_LAMP, ActPos); byy krs
				break;
			}
		}
	}
}

// 활동대기 취소
void CRoomWorld::ActWaitCancel(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid, uint8 SuccessFlag, uint32 ChannelID, bool bUnconditional)
{
	if ((ActPos <= 0) || (ActPos >= COUNT_ACTPOS))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"ActWaitCancel - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	if (ChannelID == 0)
		ChannelID = GetFamilyChannelID(FID);
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		if ((FID >= AUTOPLAY_FID_START) && (FID < AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT)) { // by krs
			pChannelData = GetChannel(5001);
			ChannelID = 5001;
			if (pChannelData == NULL) {
				//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=5001", m_RoomID);
				return;
			}
		}
	}

	uint8	ActPos_Base = ActPos;
	int		index = (int)ActPos - 1;
	_RoomActivity	*Activity_Cur = &pChannelData->m_Activitys[index];
	_RoomActivity	*Activity_Base = &pChannelData->m_Activitys[index];
	TWaitFIDList::iterator	it, itTarget;
	uint32		RemainCount = 1;
	bool		bFounded = false, bValidItTarget = false;

	//sipdebug("RoomID=%d, ChannelID=%d, FID=%d, ActPos=%d", m_RoomID, ChannelID, FID, ActPos);

	if ((ActPos > ACTPOS_WRITEMEMO_START) && (ActPos < ACTPOS_WRITEMEMO_START + MAX_MEMO_DESKS_COUNT))
	{
		ActPos_Base = ACTPOS_WRITEMEMO_START;
		Activity_Base = &pChannelData->m_Activitys[ACTPOS_WRITEMEMO_START - 1];
	}
	else if ((ActPos > ACTPOS_BLESS_LAMP) && (ActPos < ACTPOS_BLESS_LAMP + MAX_BLESS_LAMP_COUNT))
	{
		ActPos_Base = ACTPOS_BLESS_LAMP;
		Activity_Base = &pChannelData->m_Activitys[ACTPOS_BLESS_LAMP - 1];
	}

	// 행사자체관리
	if (Activity_Cur->FID == FID)
	{
		// 행사의 Host가 탈퇴한 경우
		//sipdebug("ActWait Canceled:WaitingUser - CurrentUser. ActPos=%d, FID=%d", ActPos, FID); byy krs
		CompleteActivity(ChannelID, index, SuccessFlag);

		bFounded = true;
		RemainCount = 0;
	}
	else if (IsMutiActionType(Activity_Cur->ActivityType))
	{
		// 집체행사에서 Join이 탈퇴한 경우
		if (!IsValidMultiActPos(ActPos))
		{
			sipwarning("Unknown Error!!! (8078)");
			return;
		}

		// 행사참가자가 탈퇴한 경우 다른 행사참가자들뿐아니라 구경군들에게도 M_SC_MULTIACT_CANCEL 를 보낸다.
		for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if (Activity_Cur->ActivityUsers[i].FID == FID &&
				(bUnconditional || Activity_Cur->ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_NONE))
			{
				Activity_Cur->ActivityUsers[i].FID = 0;

				ucstring	FName = L"";
				INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
				if(pInfo != NULL)
					FName = pInfo->m_Name;

				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTIACT_CANCEL)
					msgOut.serial(m_RoomID, ActPos, FID, FName);
				FOR_END_FOR_FES_ALL()

				CheckMultiactAllStarted(ChannelID, ActPos);

				//sipdebug("MultiAct User Canceled in current MultiAction. RoomID=%d, FID=%d", m_RoomID, FID); byy krs
				return;
			}
		}
	}

	// 대기렬관리
	uint8	waitActPos = ActPos;
	uint8	waitActPos_Base = ActPos_Base;
	int		waitindex = index;
	_RoomActivity	*waitActivity_Cur = Activity_Cur;
	_RoomActivity	*waitActivity_Base = Activity_Base;
	if (ActPos == ACTPOS_AUTO2)
	{
		waitActPos = waitActPos_Base = ACTPOS_YISHI;
		waitindex = ACTPOS_YISHI - 1;
		waitActivity_Cur = &pChannelData->m_Activitys[waitindex];
		waitActivity_Base = &pChannelData->m_Activitys[waitindex];
		bFounded = true;
	}
	for (it = waitActivity_Base->WaitFIDs.begin(); it != waitActivity_Base->WaitFIDs.end(); it ++)
	{
		if (!bFounded)
		{
			if (it->ActivityUsers[0].FID == FID)
			{
				bFounded = true;
				bValidItTarget = true;
				itTarget = it;
				RemainCount --;

				if (IsMutiActionType(it->ActivityType))
				{
					// 집체행사의 Host가 탈퇴하였으므로 행사참가자들에게 M_SC_MULTIACT_RELEASE을 보낸다.
					uint8		Count = 0;
					for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
					{
						if(it->ActivityUsers[j].FID != 0)
							Count ++;
					}
					uint32	zero = 0;
					uint8	bZero = 0;
					for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
					{
						if (it->ActivityUsers[j].FID == 0)
							continue;

						FAMILY	*pFamily = GetFamilyData(it->ActivityUsers[j].FID);
						if (pFamily == NULL)
						{
							sipwarning("GetFamilyData = NULL. FID=%d", it->ActivityUsers[j].FID);
							continue;
						}

						CMessage	msgOut(M_SC_MULTIACT_RELEASE);
						msgOut.serial(it->ActivityUsers[j].FID, zero, waitActPos, SuccessFlag, Count);
						for (int k = 0; k < MAX_MULTIACT_FAMILY_COUNT; k ++)
						{
							if(it->ActivityUsers[k].FID != 0)
								msgOut.serial(it->ActivityUsers[k].FID);
						}
						msgOut.serial(bZero);
						CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
					}
				}
			}
			else if (IsMutiActionType(it->ActivityType))
			{ // 집체행사인 경우
				if ((waitActPos != ACTPOS_YISHI) && (waitActPos != ACTPOS_ANCESTOR_JISI))
				{
					sipwarning("Unknown Error!!! (8127)");
					return;
				}

				for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
				{
					if (it->ActivityUsers[i].FID == FID &&
						(bUnconditional || it->ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_NONE))
					{
						// 행사대기자가 탈퇴한 경우 다른 행사대기자들에게 M_SC_MULTIACT_CANCEL 를 보낸다.
						it->ActivityUsers[i].FID = 0;

						ucstring	FName = L"";
						INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
						if(pInfo != NULL)
							FName = pInfo->m_Name;

						uint32	zero = 0;
						for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
						{
							if(it->ActivityUsers[j].FID == 0)
								continue;

							FAMILY	*pFamily = GetFamilyData(it->ActivityUsers[j].FID);
							if (pFamily == NULL)
							{
								sipwarning("GetFamilyData = NULL. FID=%d", it->ActivityUsers[j].FID);
								continue;
							}

							CMessage	msgOut(M_SC_MULTIACT_CANCEL);
							msgOut.serial(it->ActivityUsers[j].FID, zero, m_RoomID, waitActPos, FID, FName);
							CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
						}

						//sipdebug("MultiAct User Canceled in wait MultiAction. RoomID=%d, FID=%d", m_RoomID, FID); byy krs
						return;
					}
				}
			}
		}
		else
		{
			if (IsSingleActionType(it->ActivityType))
			{ // 개별행사인 경우
				CMessage	msgOut(M_SC_ACT_WAIT);
				uint32		ErrorCode = 0;
				uint32	WaitFID = it->ActivityUsers[0].FID;
				FAMILY	*pFamily = GetFamilyData(WaitFID);
				if (pFamily == NULL)
				{
					sipwarning("GetFamilyData = NULL. FID=%d", WaitFID);
				}
				else
				{
					msgOut.serial(WaitFID);
					if (RemainCount != 0)
						msgOut.serial(waitActPos_Base, ErrorCode, RemainCount);
					else
					{
						// 다음 대기자에게 행사시작 통지
						waitActivity_Cur->init(WaitFID, it->ActivityType,0,0);//, family->m_Name);
						bValidItTarget = true;
						itTarget = it;
						{
							msgOut.serial(waitActPos, ErrorCode, RemainCount);

							//sipdebug("Start Activity: ActPos=%d, FID=%d", waitActPos, it->ActivityUsers[0].FID); byy krs
						}
					}

					CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
				}
			}
			else if (IsMutiActionType(it->ActivityType))
			{ // 집체행사인 경우
				if (RemainCount == 0)
				{
					// 행사시작상태로 설정한다.
//BUG					waitActivity_Cur->init(it->ActivityUsers[0].FID, AT_MULTIRITE,0,0);
					waitActivity_Cur->init(it->ActivityUsers[0].FID, it->ActivityType,0,0);
					for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
					{
						if(it->ActivityUsers[j].FID != 0)
							waitActivity_Cur->ActivityUsers[j] = it->ActivityUsers[j];
					}

					bValidItTarget = true;
					itTarget = it;
				}

				// 행사참가자들에게 M_SC_MULTIACT_WAIT 를 보낸다.
				uint32		ErrorCode = 0;
				for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
				{
					if (it->ActivityUsers[j].FID == 0)
						continue;

					FAMILY	*pFamily = GetFamilyData(it->ActivityUsers[j].FID);
					if (pFamily == NULL)
					{
						sipwarning("GetFamilyData = NULL. FID=%d", it->ActivityUsers[j].FID);
						continue;
					}

					CMessage	msgOut(M_SC_MULTIACT_WAIT);
					msgOut.serial(it->ActivityUsers[j].FID, waitActPos, ErrorCode, RemainCount);
					CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
				}
			}
		}
		RemainCount ++;
	}
	if (bFounded)
	{
		// sipdebug("ActWait Canceled - WaitingUser. ActPos=%d, FID=%d", waitActPos, FID);
		if (bValidItTarget)
		{
			waitActivity_Base->WaitFIDs.erase(itTarget);
		}
	}
	else
	{
		if (_tsid.get() != 0)
			sipwarning("bFounded = false !!! RoomID=%d, ChannelID=%d, FID=%d, ActPos=%d, SuccessFlag=%d, bUnconditional=%d", m_RoomID, ChannelID, FID, ActPos, SuccessFlag, bUnconditional);
	}

	pChannelData->UnregisterActionPlayer(FID);	// 행사끝검사목록에서 삭제한다.
}

// 기념관의 경험치변경 처리함수
int CRoomWorld::ChangeRoomExp(uint8 RoomExpType, T_FAMILYID FID, uint32 UMoney, uint32 GMoney, ucstring itemInfo)
{
	if (!CheckIfActSave(FID))
		return -1;
	
	bool	bChanged = false;

	uint32		addExp = 0;
	T_ROOMEXP	exp0 = m_Exp;
	T_ROOMLEVEL	level0 = m_Level;

	// 경험치 증가
	switch(RoomExpType)
	{
	case Room_Exp_Type_UseMoney:
		addExp = UMoney + (GMoney / 4);
		//sipdebug("Room_Exp_Type_UseMoney : UMoney=%d, GMoney=%d", UMoney, GMoney); byy krs
		break;

	case Room_Exp_Type_Visitroom:
		{
			if(CheckIfRoomExpIncWhenFamilyEnter(FID))
			{
				addExp = mstRoomExp[Room_Exp_Type_Visitroom].Exp;
				//sipdebug("Room_Exp_Type_Visitroom : Exp = %d", mstRoomExp[Room_Exp_Type_Visitroom].Exp); byy krs
			}
			else
			{
//				sipdebug("Room_Exp_Type_Visitroom fail.");
			}
		}
		break;

	default:
		{
			uint32	today = GetDBTimeDays();

			if(today != m_RoomExpHis[RoomExpType].Day)
			{
				m_RoomExpHis[RoomExpType].Day = today;
				m_RoomExpHis[RoomExpType].Count = 0;
			}
			if(mstRoomExp[RoomExpType].MaxCount > m_RoomExpHis[RoomExpType].Count)
			{
				m_RoomExpHis[RoomExpType].Count ++;
				addExp = mstRoomExp[RoomExpType].Exp;

				//sipdebug("Room Exp Increased : RoomID=%d, Type = %d, Exp = %d, FID=%d", m_RoomID, RoomExpType, mstRoomExp[RoomExpType].Exp, FID); byy krs
			}
			else
			{
				//sipdebug("Room Exp failed : Type = %d", RoomExpType);
			}
		}
		break;
	}

	// 2배경험일 처리
	if (g_bIsHoliday || m_bIsRoomcreateDay)
		addExp *= 2;

	m_Exp += addExp;
	// 통지
	if(exp0 != m_Exp)
	{
		bChanged = true;
		m_bRoomExpChanged = true;

		while(m_Level < ROOM_LEVEL_MAX)
		{
			if(m_Exp < mstRoomLevel[m_Level + 1].Exp)
				break;
			m_Level ++;
		}

		// Save to DB
		if(!SaveRoomLevelAndExp(level0 != m_Level))		// 급수가 올랐으면 보존한다.
		{
			//sipwarning("SaveRoomLevelAndExp failed."); byy krs
			return 0;
		}

		// Log change the room's Exp
		DBLOG_ChangeRoomExp(FID, m_RoomID, m_RoomName, exp0, m_Exp, m_Level, RoomExpType, itemInfo);

		if(RoomExpType != Room_Exp_Type_Visitroom)
		{
			//uint16	ExpPercent = CalcRoomExpPercent(m_Level, m_Exp);
			FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ROOMPRO)
				msgOut.serial(m_RoomID, m_Exp, m_Level); // , ExpPercent);
			FOR_END_FOR_FES_ALL()
		}
	}

	return bChanged;
}

bool	CRoomWorld::ProcCheckingRoomRequest(T_FAMILYID srcFID, T_FAMILYID targetFID, uint8 ContactStatus1, uint8 ContactStatus2)
{
	FAMILY* pSrc = GetFamilyData(srcFID);
	if (pSrc == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", srcFID);
		return true;
	}
	// 친구공개방이고, 초청자가 관리원이 아닌 경우에는 방주인의 친구로 되는 사람만 초청할수 있다.
	if ((srcFID != m_MasterID) && (targetFID != m_MasterID))
	{
		if (m_PowerID == ROOMOPEN_TONEIGHBOR && /*srcFID != m_MasterID &&*/ !IsRoomManager(srcFID))
		{
			//		if (!IsFriendList(m_MasterID, targetFID))
			if (ContactStatus2 != 1)
			{
				ucstring	FamilyName = L"";
				uint8		Answer = 5;	// NoMasterFriend
				CMessage msg(M_SC_INVITE_ANS);
				msg.serial(srcFID, targetFID, FamilyName, Answer);
				CUnifiedNetwork::getInstance()->send(pSrc->m_FES, msg);
				return true;
			}
		}
		if (m_PowerID == ROOMOPEN_TONONE)
		{
			sipwarning("m_PowerID == ROOMOPEN_TONONE. FID=%d, RoomID=%d [WRN_3]", srcFID, m_RoomID);
			return true;
		}
	}
	return false;
}

// 방에서 강퇴시킬수 있는가를 검사한다.
int CRoomWorld::CanBanish(uint32 FID, uint32 targetFID)
{
	if ((FID != m_MasterID) && !IsRoomManager(FID))	// 비법발견
	{
		// 비법
		ucstring ucUnlawContent = SIPBASE::_toString(L"Cannot banish because no room manager: FID=%d, targetFID=%d", FID, targetFID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return 1;
	}

	//// 방주인이나 관리자는 강퇴시킬수 없다.
	// Client에서 방관리자를 평가하는데 좀 어려움이 있어서 손정혁과 약속하고 관리자도 내보낼수 있게 함. 2016/01/20
	//if ((targetFID == m_MasterID) || IsRoomManager(targetFID))	// 비법발견
	//{
	//	sipdebug("Can't Banish RoomMaster and RoomManager. FID=%d, targetFID=%d", FID, targetFID);
	//	return 2;
	//}

	// targetFID가 방에 있어야 한다.
	TRoomFamilys::iterator	itRoom = m_mapFamilys.find(targetFID);
	if (itRoom == m_mapFamilys.end())
	{
		// 비법
		ucstring ucUnlawContent = SIPBASE::_toString(L"Cannot banish because targetFID is not in room: FID=%d, targetFID=%d", FID, targetFID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return 3;
	}

	// 행사중인 경우 강퇴시킬수 없다.
	uint32	ChannelID = GetFamilyChannelID(targetFID);
	int		index = GetActivitingIndexOfUser(ChannelID, targetFID);
	if (index >= 0)
		return 4;

	return 0;
}

void	cb_M_CS_REQEROOM_Oroom(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	T_ID			ChannelID;
	std::string		authkey;
	uint16			wsid;
	uint8			ContactStatus;

	_msgin.serial(FID, RoomID);
	NULLSAFE_SERIAL_STR(_msgin, authkey, MAX_LEN_ROOM_VISIT_PASSWORD, FID);
	_msgin.serial(ChannelID, wsid, ContactStatus);

	TServiceId	client_FsSid(wsid);

	sipdebug("Request Enter Room. FID=%d, RoomID=%d, ChannelID=%d", FID, RoomID, ChannelID); // byy krs

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		// Offline양도기능이 들어가면서 삭제됨
		//if(roomManager->m_CurGiveID != 0)
		//{
		//	// 양도중인 방에는 들어갈수 없다.
		//	sendErrorResult(M_SC_REQEROOM, FID, E_ENTER_CHANGE_MASTER, _tsid);
		//	return;
		//}

		roomManager->RequestEnterRoom(FID, authkey, client_FsSid, ChannelID, ContactStatus);
	}
}

void	cb_M_CS_CHECK_ENTERROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	std::string		authkey;
	uint16			wsid;
	uint8			ContactStatus;

	_msgin.serial(FID, RoomID);
	NULLSAFE_SERIAL_STR(_msgin, authkey, MAX_LEN_ROOM_VISIT_PASSWORD, FID);
	_msgin.serial(wsid, ContactStatus);

	TServiceId	client_FsSid(wsid);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		uint32 code = roomManager->GetEnterRoomCode(FID, authkey, 0, ContactStatus);

		CMessage	msgOut(M_SC_CHECK_ENTERROOM);
		msgOut.serial(FID, code, RoomID);
		CUnifiedNetwork::getInstance()->send(client_FsSid, msgOut);
	}
}

int CRoomWorld::CheckEnterWaitFamily_Spec()
{
	// 오프라인양도기능을 넣으면서 양도중인 방에 못들어간다는 사양이 없어짐
	//if(m_CurGiveID != 0)
	//	return 4;

	return 0;
}

//static void	DBCB_DBProlongFreeRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID		FID				= *(T_FAMILYID *)argv[0];
//	T_ROOMID		RoomID			= *(T_ROOMID *)argv[1];
//	TServiceId		tsid(*(uint16 *)argv[2]);
//
//	if (!nQueryResult)
//	{
//		return;	// E_DBERR
//	}
//
//	uint32		nRows;			resSet->serial(nRows);
//	if (nRows != 1)
//	{
//		sipwarning("nRows != 1. Rows=%d", nRows);
//		return;
//	}
//
//	uint32	uResult = E_COMMON_FAILED;
//	sint32	ret;	resSet->serial(ret);
//	string termtime;
//
//	switch (ret)
//	{
//	case 0:
//		uResult = ERR_NOERR;
//		resSet->serial(termtime);
//		break;
//	case 1003:
//		{
//			// 비법파케트발견
//			ucstring ucUnlawContent = SIPBASE::_toString("Cannot prolong Room, Improper RoomID , room ID:%u", RoomID).c_str();
//			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//			return;
//		}
//	case 1061:
//		{
//			// 비법파케트발견
//			ucstring ucUnlawContent = SIPBASE::_toString("Cannot prolong Room, because room is deleted, room ID:%u", RoomID).c_str();
//			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//			return;
//		}
//	case 1066:
//		{
//			// 비법파케트발견
//			ucstring ucUnlawContent = SIPBASE::_toString("Cannot prolong Room, because room is not ganenyuan, room ID:%u", RoomID).c_str();
//			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//			return;
//		}
//	default:
//		sipwarning(L"Unknown Error Query %u", ret);
//		break;
//	}
//
//	// 방관리자에게 통보
//	{
//		bool bResult = true;
//		uint32	nTermTime = getMinutesSince1970(termtime);
//		CMessage	msgout(M_SC_PROLONG_PASS);
//		msgout.serial(FID, bResult, RoomID, nTermTime);
//		CUnifiedNetwork::getInstance()->send(tsid, msgout);
//		sipinfo("Prolong success family(id:%u) Renew room(id:%u)", FID, RoomID);
//	}
//
//	if (uResult == ERR_NOERR)
//	{
//		// 현재 Open되여있는 경우 방정보를 수정한다.
//		CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
//		if (roomManager)
//			roomManager->SendRoomTermTimeToAll(termtime);
//	}
//}

//static void	DBCB_DBGetRoomForProlong(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	uint8			Flag			= *(uint8 *)argv[0];
//	uint32			_OldSceneID		= *(uint32 *)argv[1];
//	sint32			ret				= *(sint32 *)argv[2];
//	T_FAMILYID		FID				= *(T_FAMILYID *)argv[3];
//	T_ROOMID		RoomID			= *(T_ROOMID *)argv[4];
//	TServiceId		tsid(*(uint16 *)argv[5]);
//
//	if (!nQueryResult || ret == -1)
//	{
//		sipwarning("Shrd_Room_Renew_History_List: execute failed!");
//		return;	// E_DBERR
//	}
//	uint16			OldSceneID		= (uint16)_OldSceneID;
//
//	if (OldSceneID != 6)
//	{
//		sipwarning("Can't prolong. SceneID=%d", OldSceneID);
//		return;
//	}
//
//	if (Flag == 2)
//	{
//		// 무한방은 연장할수 없다.
//		ucstring ucUnlawContent = SIPBASE::_toString("Cannot renew room because unlimited. FID=%d, RoomID=%d", FID, RoomID).c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//
//	// 방기한연장
//	MAKEQUERY_PROLONGFREEROOM(strQuery, RoomID, 0, 3, 0);
//	DBCaller->executeWithParam(strQuery, DBCB_DBProlongFreeRoom,
//		"DDW",
//		CB_,	FID,
//		CB_,	RoomID,
//		CB_,	tsid.get());
//}

//void	cb_M_CS_PROLONG_PASS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;
//	T_ROOMID		RoomID;
//	_msgin.serial(FID, RoomID);
//
//	CRoomWorld *inManager = GetOpenRoomWorld(RoomID);
//	if (inManager != NULL)
//	{
//		inManager->ProlongRequestFreeRoom(FID, _tsid);
//	}
//	else
//	{
//		MAKEQUERY_GetRoomRenewHistory(strQ, RoomID);
//		DBCaller->executeWithParam(strQ, DBCB_DBGetRoomForProlong, 
//			"BDDDDW",
//			OUT_,	NULL,
//			OUT_,	NULL,
//			OUT_,	NULL,
//			CB_,	FID,
//			CB_,	RoomID,
//			CB_,	_tsid.get());
//	}
//}

//void	CRoomWorld::ProlongRequestFreeRoom(T_FAMILYID _FID, SIPNET::TServiceId _sid)
//{
//	if (m_MasterID != _FID)
//	{
//		sipwarning("No master for free prolong. FID=%d", _FID);
//		return;
//	}
//	if (m_SceneID != 6)
//	{
//		sipwarning("Can't prolong. SceneID=%d", m_SceneID);
//		return;
//	}
//
//	MAKEQUERY_PROLONGFREEROOM(strQuery, m_RoomID, 0, 3, 0);
//	DBCaller->executeWithParam(strQuery, DBCB_DBProlongFreeRoom,
//		"DDW",
//		CB_,	_FID,
//		CB_,	m_RoomID,
//		CB_,	_sid.get());
//}

void	cb_M_CS_BANISH(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID, targetFID;
	_msgin.serial(FID, targetFID);

	// 들어가 있는 방관리자 얻기
	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager != NULL)
	{
		int	check = inManager->CanBanish(FID, targetFID);
		if (check != 0)
		{
			sipwarning("Can't banish. FID=%d, targetFID=%d, RoomID=%d, Reason=%d", FID, targetFID, inManager->m_RoomID, check);
			return;
		}

		MAP_FAMILY::iterator	it = map_family.find(FID);
		if (it == map_family.end())
		{
			sipwarning("FamilyID not found : FamilyID=%d", FID);
			return;
		}
		MAP_FAMILY::iterator	itTarget = map_family.find(targetFID);
		if (itTarget == map_family.end())
		{
			sipwarning("targetFamilyID not found : targetFamilyID=%d", targetFID);
			return;
		}
		CMessage	msgout(M_SC_BANISH);
		msgout.serial(targetFID);
		msgout.serial(FID);
		msgout.serial(it->second.m_Name);
		CUnifiedNetwork::getInstance()->send(itTarget->second.m_FES, msgout);

		inManager->OutRoom(targetFID, false);
	}
	else
	{
		// 비법발견
		// ucstring ucUnlawContent;
		// ucUnlawContent = ucstring("Invalid out room : ") + ucstring("FamilyID = ") + toStringW(FID);
		// RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
	}
}

void SendChit(uint32 srcFID, uint32 targetFID, uint8 ChitType, uint32 p1, uint32 p2, ucstring &p3, ucstring &p4)
{
	ucstring	srcName = L"";
	if (srcFID != targetFID)
	{
		FAMILY*	pFamily = GetFamilyData(srcFID);
		if(pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", srcFID);
			return;
		}
		srcName = pFamily->m_Name;
	}

	FAMILY*	ptargetFamily = GetFamilyData(targetFID);
	if(ptargetFamily != NULL)
	{
		uint16		r_sid = SIPNET::IService::getInstance()->getServiceId().get();
		CMessage	msgout(M_SC_CHITLIST);
		msgout.serial(targetFID, r_sid);
		uint32		nChitCount = 1;						msgout.serial(nChitCount);
		uint32		chitID = 0;							msgout.serial(chitID);
		msgout.serial(srcFID, srcName, ChitType);
		msgout.serial(p1, p2, p3, p4);

		CUnifiedNetwork::getInstance()->send(ptargetFamily->m_FES, msgout);
	}
	else
	{
		uint8	ChitAddType = CHITADDTYPE_ADD_IF_OFFLINE;
		CMessage	msgOut(M_NT_ADDCHIT);
		msgOut.serial(ChitAddType, srcFID, srcName, targetFID, ChitType, p1, p2, p3, p4);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}
}

static void	DBCB_DBRoomManagerAdd(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			ret			= *(uint32 *)argv[0];
	T_FAMILYID		FID			= *(uint32 *)argv[1];
	T_FAMILYID		targetFID	= *(uint32 *)argv[2];
	T_ROOMID		RoomID		= *(uint32 *)argv[3];

	switch(ret)
	{
	case 0: // OK
		uint32 nRows;	resSet->serial(nRows);
		if (nRows == 1)
		{
			ucstring	RoomName;			resSet->serial(RoomName);
			ucstring	p4 = L"";
			SendChit(FID, targetFID, CHITTYPE_MANAGER_ADD, RoomID, 0, RoomName, p4);

			// HisManager에 통지
			uint8	uRoomRole = ROOM_MANAGER;
			CMessage	msgOut(M_NT_CHANGEROLE);
			msgOut.serial(targetFID, RoomID, uRoomRole);
			CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgOut);
		}
		else
		{
			sipwarning("Returned Record Count = 0");
		}
		break;
	case 1014: // 방주인이 아닌데?
		sipwarning("You are not RoomMaster. FID=%d, RoomID=%d", FID, RoomID);
		break;
	case 1062: // 한방의 관리원이 10명 초과
		sipwarning("RoomManager's count Overflow! FID=%d, RoomID=%d", FID, RoomID);
		break;
	case 1013: // 이미 등록되여있다.
		sipwarning("Already registered RoomManager! FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		break;
	default:
		sipwarning("Unknown Error. ErrorCode=%d", ret);
		break;
	}
}

bool AddRoomManagerInRoomWorld(uint32 RoomID, uint32 MasterFID, uint32 TargetFID)
{
	CRoomWorld*	pRoom = GetOpenRoomWorld(RoomID);
	if(pRoom == NULL)
		return true;

	// 이미 방이 open되여있으면 방에서 직접 검사한다.
	if(MasterFID != pRoom->m_MasterID)
	{
		// 방주인이 아니다.
		sipwarning("You are not RoomMaster. FID=%d, RoomID=%d", MasterFID, RoomID);
		return false;
	}
	if(pRoom->m_RoomManagerCount >= MAX_MANAGER_COUNT)
	{
		sipwarning("RoomManager's count Overflow! FID=%d, RoomID=%d", MasterFID, RoomID);
		return false;
	}
	int j = -1;
	for(int i = 0; i < pRoom->m_RoomManagerCount; i ++)
	{
		if(pRoom->m_RoomManagers[i].FID == TargetFID)
		{
			sipwarning("Already registered RoomManager! FID=%d, RoomID=%d, targetFID=%d", MasterFID, RoomID, TargetFID);
			return false;
		}
	}

	// 검사 성공함.
	pRoom->m_RoomManagers[pRoom->m_RoomManagerCount].FID = TargetFID;
	pRoom->m_RoomManagers[pRoom->m_RoomManagerCount].Permission = MANAGER_PERMISSION_USE;
	pRoom->m_RoomManagerCount ++;
	//sipdebug("RoomManager added in memory. FID=%d, RoomID=%d, targetFID=%d", MasterFID, RoomID, TargetFID); byy krs

	return true;
}

void cb_M_CS_MANAGER_ADD(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID, targetFID;
	T_ROOMID		RoomID;

	_msgin.serial(FID, RoomID, targetFID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false for FID. FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		return;
	}
	if (!IsThisShardFID(targetFID))
	{
		sipwarning("IsThisShardFID = false for targetFID. FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		return;
	}
	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		return;
	}

	if(!AddRoomManagerInRoomWorld(RoomID, FID, targetFID))
		return;

	MAKEQUERY_RoomManager_Insert(strQ, FID, RoomID, targetFID);
	DBCaller->executeWithParam(strQ, DBCB_DBRoomManagerAdd,
		"DDDD",
		OUT_,		NULL,
		CB_,		FID,
		CB_,		targetFID,
		CB_,		RoomID);
}

bool DeleteRoomManagerInRoomWorld(uint32 RoomID, uint32 MasterFID, uint32 TargetFID)
{
	CRoomWorld*	pRoom = GetOpenRoomWorld(RoomID);
	if (pRoom == NULL)
		return true;

	// 이미 방이 open되여있으면 방에서 직접 검사한다.
	if (MasterFID != pRoom->m_MasterID)
	{
		// 방주인이 아니다.
		sipwarning("You are not RoomMaster. FID=%d, RoomID=%d", MasterFID, RoomID);
		return false;
	}

	for (int i = 0; i < pRoom->m_RoomManagerCount; i ++)
	{
		if (pRoom->m_RoomManagers[i].FID == TargetFID)
		{
			pRoom->m_RoomManagerCount --;
			pRoom->m_RoomManagers[i] = pRoom->m_RoomManagers[pRoom->m_RoomManagerCount];

			//sipdebug("RoomManager deleted in Memory. FID=%d, RoomID=%d, targetFID=%d", MasterFID, RoomID, TargetFID); byy krs
			return true;
		}
	}

	//sipwarning("Can't find RoomManager in Memory! FID=%d, RoomID=%d, targetFID=%d", MasterFID, RoomID, TargetFID); byy krs
	return false;
}

static void	DBCB_DBRoomManagerDelete_Ans(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			ret			= *(uint32 *)argv[0];
	T_FAMILYID		FID			= *(uint32 *)argv[1];
	T_FAMILYID		targetFID	= *(uint32 *)argv[2];
	T_ROOMID		RoomID		= *(uint32 *)argv[3];
	uint8			answer		= *(uint8 *)argv[4];

	switch(ret)
	{
	case 0: // OK
		uint32 nRows;	resSet->serial(nRows);
		if (nRows == 1)
		{
			ucstring	RoomName;			resSet->serial(RoomName);
			ucstring	p4 = L"";
			SendChit(targetFID, FID, CHITTYPE_MANAGER_ANS, RoomID, answer, RoomName, p4);

			uint8	uRoomRole = ROOM_NONE;
			CMessage	msgOut(M_NT_CHANGEROLE);
			msgOut.serial(targetFID, RoomID, uRoomRole);
			CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgOut);
		}
		else
		{
			sipwarning("Returned Record Count = 0");
		}
		break;
	case 1014: // 방주인이 아닌데?
		sipwarning("You are not RoomMaster. FID=%d, RoomID=%d", FID, RoomID);
		break;
	default:
		sipwarning("Unknown Error. ErrorCode=%d", ret);
		break;
	}
}

void cb_M_CS_MANAGER_ANS(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID, masterFID;
	T_ROOMID	RoomID;
	uint8		answer;
	ucstring	RoomName;

	_msgin.serial(FID, RoomID, RoomName, masterFID, answer);

	if(answer == ROOMMANAGER_ANS_NO || answer == ROOMMANAGER_ANS_DELETE)
	{
		if(!DeleteRoomManagerInRoomWorld(RoomID, masterFID, FID))
			return;

		MAKEQUERY_RoomManager_Delete(strQ, masterFID, RoomID, FID);
		DBCaller->executeWithParam(strQ, DBCB_DBRoomManagerDelete_Ans,
			"DDDDB",
			OUT_,		NULL,
			CB_,		masterFID,
			CB_,		FID,
			CB_,		RoomID,
			CB_,		answer);
	}
	else if(answer == ROOMMANAGER_ANS_YES)
	{
		ucstring	p4 = L"";
		SendChit(FID, masterFID, CHITTYPE_MANAGER_ANS, RoomID, answer, RoomName, p4);
	}
	else
	{
		sipwarning("Unknown answer. answer=%d", answer);
	}
}

static void	DBCB_DBRoomManagerDelete_Del(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			ret			= *(uint32 *)argv[0];
	T_FAMILYID		FID			= *(uint32 *)argv[1];
	T_FAMILYID		targetFID	= *(uint32 *)argv[2];
	T_ROOMID		RoomID		= *(uint32 *)argv[3];

	switch(ret)
	{
	case 0: // OK
		uint32 nRows;	resSet->serial(nRows);
		if (nRows == 1)
		{
			ucstring	RoomName;			resSet->serial(RoomName);
			ucstring	p4 = L"";
			SendChit(FID, targetFID, CHITTYPE_MANAGER_DEL, RoomID, ROOMMANAGER_DEL_REASON_OWNERDEL, RoomName, p4);

			uint8	uRoomRole = ROOM_NONE;
			CMessage	msgOut(M_NT_CHANGEROLE);
			msgOut.serial(targetFID, RoomID, uRoomRole);
			CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgOut);
		}
		else
		{
			sipwarning("Returned Record Count = 0");
		}
		break;
	case 1014: // 방주인이 아닌데?
		sipwarning("You are not RoomMaster. FID=%d, RoomID=%d", FID, RoomID);
		break;
	default:
		sipwarning("Unknown Error. ErrorCode=%d", ret);
		break;
	}
}

void cb_M_CS_MANAGER_DEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID, targetFID;
	T_ROOMID	RoomID;

	_msgin.serial(FID, RoomID, targetFID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false for FID. FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		return;
	}
	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		return;
	}

	if(!DeleteRoomManagerInRoomWorld(RoomID, FID, targetFID))
		return;

	MAKEQUERY_RoomManager_Delete(strQ, FID, RoomID, targetFID);
	DBCaller->executeWithParam(strQ, DBCB_DBRoomManagerDelete_Del,
		"DDDD",
		OUT_,		NULL,
		CB_,		FID,
		CB_,		targetFID,
		CB_,		RoomID);
}

static void	DBCB_DBRoomManagerGet(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			ret			= *(uint32 *)argv[0];
	T_FAMILYID		masterFID	= *(uint32 *)argv[1];
	T_ROOMID		RoomID		= *(uint32 *)argv[2];
	TServiceId		tsid(*(uint16 *)argv[3]);

	switch(ret)
	{
	case 0: // OK
		{
			CMessage	msgOut(M_SC_MANAGER_LIST);

			uint32	nRows;	resSet->serial(nRows);
			uint8	byCount = (uint8)nRows;
			msgOut.serial(masterFID, RoomID, byCount);
			for(uint32 i = 0; i < nRows; i ++)
			{
				uint32	FID;		resSet->serial(FID);
				uint32	CHID;		resSet->serial(CHID);
				ucstring FName;		resSet->serial(FName);
				uint32	ModelID;	resSet->serial(ModelID);
				uint32	Birthdate;	resSet->serial(Birthdate);
				uint32	City;		resSet->serial(City);
				uint8	Permission;	resSet->serial(Permission);

				msgOut.serial(FID, Permission, CHID, FName, ModelID, Birthdate, City);
			}

			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		}
		break;
	case 1014: // 방주인이 아닌데?
		sipwarning("You are not RoomMaster. FID=%d, RoomID=%d", masterFID, RoomID);
		break;
	default:
		sipwarning("Unknown Error. ErrorCode=%d", ret);
		break;
	}
}

void cb_M_CS_MANAGER_GET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	T_ROOMID	RoomID;

	_msgin.serial(FID, RoomID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false for FID. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}
	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	MAKEQUERY_RoomManager_List(strQ, FID, RoomID);
	DBCaller->executeWithParam(strQ, DBCB_DBRoomManagerGet,
		"DDDW",
		OUT_,		NULL,
		CB_,		FID,
		CB_,		RoomID,
		CB_,		_tsid.get());
}

// 친구목록에서 삭제할 때 방관리자에서 자동삭제된다.
void cb_M_NT_MANAGER_DEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	ManagerFID;
	T_ROOMID	RoomID;

	_msgin.serial(RoomID, ManagerFID);

	CRoomWorld*	pRoom = GetOpenRoomWorld(RoomID);
	if (pRoom == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	for (int i = 0; i < pRoom->m_RoomManagerCount; i ++)
	{
		if (pRoom->m_RoomManagers[i].FID == ManagerFID)
		{
			pRoom->m_RoomManagerCount --;
			pRoom->m_RoomManagers[i] = pRoom->m_RoomManagers[pRoom->m_RoomManagerCount];

			//sipdebug("RoomManager deleted in Memory. RoomID=%d, targetFID=%d", RoomID, ManagerFID); byy krs
			return;
		}
	}

	//sipwarning("Can't find RoomManager in Memory! RoomID=%d, targetFID=%d", RoomID, ManagerFID); byy krs
}

// Set Room Manager's Permission ([d:Room id][d:targetFID][b:Permission])
void cb_M_CS_MANAGER_SET_PERMISSION(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID, targetFID;
	T_ROOMID	RoomID;
	uint8		Permission;

	_msgin.serial(FID, RoomID, targetFID, Permission);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false for FID. FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		return;
	}
	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		return;
	}
	if (!IsThisShardFID(targetFID))
	{
		sipwarning("IsThisShardFID = false for targetFID. FID=%d, RoomID=%d, targetFID=%d", FID, RoomID, targetFID);
		return;
	}

	CRoomWorld*	pRoom = GetOpenRoomWorld(RoomID);
	if (pRoom == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	pRoom->SetRoomManagerPermission(FID, targetFID, Permission);
}

void CRoomWorld::SetRoomManagerPermission(T_FAMILYID FID, T_FAMILYID targetFID, uint8 Permission)
{
	if (m_MasterID != FID)
	{
		sipwarning("m_MasterID != FID. m_MasterID=%d, FID=%d", m_MasterID, FID);
		return;
	}

	for (int i = 0; i < m_RoomManagerCount; i ++)
	{
		if (m_RoomManagers[i].FID == targetFID)
		{
			m_RoomManagers[i].Permission = Permission;

			MAKEQUERY_ChangeRoomManagerPermission(strQ, m_RoomID, targetFID, Permission);
			DBCaller->execute(strQ);

			return;
		}
	}

	//sipwarning("i < m_RoomManagerCount. RoomID=%d, FID=%d, targetFID=%d, Permission=%d", m_RoomID, FID, targetFID, Permission); byy krs
}

void	CRoomWorld::SetFamilyNameForOpenRoom(T_FAMILYID FID, T_FAMILYNAME FamilyName)
{
	if (m_MasterID == FID)
		m_MasterName = FamilyName;
	if (m_FlowerFID == FID)
		m_FlowerFamilyName = FamilyName;

	for(RoomChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		pChannelData->SetFamilyName(FID, FamilyName);
	}
	for (std::map<uint32, GoldFishData>::iterator it = m_GoldFishData.begin(); it != m_GoldFishData.end(); it ++)
	{
		for (int i = 0; i < MAX_GOLDFISH_COUNT; i ++)
		{
			if (it->second.m_GoldFishs[i].FID == FID)
			{
				it->second.m_GoldFishs[i].FName = FamilyName;
			}
		}
	}	
}

static void	DBCB_DBSetRoomInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			FID			= *(T_FAMILYID *)argv[0];
	T_ROOMID			idRoom		= *(T_ROOMID *)argv[1];
	T_ROOMNAME			RoomName	= (ucchar *)argv[2];
	T_ROOMPOWERID		OpenProp	= *(T_ROOMPOWERID *)argv[3];
	T_COMMON_CONTENT	RCom		= (ucchar *)argv[4];
	std::string			authkey		= (char *)argv[5];
	TServiceId			tsid(*(uint16 *)argv[6]);

	uint32	uResult = E_COMMON_FAILED;
	{
		if (nQueryResult == true)
		{
			uint32 nRows;	resSet->serial(nRows);
			if (nRows == 1)
			{
				uint32 ret;			resSet->serial(ret);
				if (ret == 0)
				{
					// 보내지 않는것으로 토의함
					uResult = ERR_NOERR;
					//CMessage	msgout(M_SC_SETROOMINFO);
					//msgout.serial(FID);
					//msgout.serial(uResult, idRoom);
					//msgout.serial(RoomName);
					//msgout.serial(OpenProp);
					//msgout.serial(RCom);
					//CUnifiedNetwork::getInstance ()->send(tsid, msgout);
					//sipinfo (L"SETROOMINFO success : RoomID=%d, FamilyID=%d", idRoom, FID); byy krs
				}
				if (ret == 1003)
				{
					// 비법
					ucstring ucUnlawContent = SIPBASE::_toString(L"Cannot update room(No owner):RoomID=%d", idRoom);
					RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
					return;
				}
			}
		}
	}
	if (uResult == ERR_NOERR)
	{
		CRoomWorld *roomManager = GetOpenRoomWorld(idRoom);
		if (roomManager)
			roomManager->SetRoomInfo(FID, idRoom, RoomName, OpenProp, RCom, authkey);
		else
			NotifyChangeJingPinTianYuanInfo(idRoom);
	}
	else
	{
		// 실패 - 보내지 않는것으로 토의함
		//uResult = 	E_COMMON_FAILED;
		//CMessage	msgout(M_SC_SETROOMINFO);
		//msgout.serial(FID);
		//msgout.serial(uResult);
		//CUnifiedNetwork::getInstance ()->send(tsid, msgout);
		//sipwarning ("SETROOMINFO failed : RoomID=%d, FamilyID=%d", idRoom, FID); byy krs
	}
}

void	cb_M_CS_SETROOMINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	T_ROOMID		idRoom;			_msgin.serial(idRoom);
	T_ROOMNAME		RoomName;		NULLSAFE_SERIAL_WSTR(_msgin, RoomName, MAX_LEN_ROOM_NAME, FID);
	T_ROOMPOWERID	OpenProp;		_msgin.serial(OpenProp);
	T_COMMON_CONTENT	RCom;		NULLSAFE_SERIAL_WSTR(_msgin, RCom, MAX_LEN_ROOM_COMMENT, FID);
	std::string			authkey;	NULLSAFE_SERIAL_STR(_msgin, authkey, MAX_LEN_ROOM_VISIT_PASSWORD, FID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	CRoomWorld *roomManager = GetOpenRoomWorld(idRoom);
	if (roomManager)
	{
		if (roomManager->m_LuckID_Lv4 != 0)
		{
			OpenProp = ROOMOPEN_TOALL;
			authkey = "";
		}
	}

	MAKEQUERY_SETROOMINFO(strQ, FID, idRoom, RoomName, OpenProp, RCom, authkey);
	DBCaller->executeWithParam(strQ, DBCB_DBSetRoomInfo,
		"DDSBSsW",
		CB_,			FID,
		CB_,			idRoom,
		CB_,			RoomName.c_str(),
		CB_,			OpenProp,
		CB_,			RCom.c_str(),
		CB_,			authkey.c_str(),
		CB_,			_tsid.get());
}

//////////////////////////////////////////////////////////////////////////
// Set dead([d:Room id][u:Room name][b:DeadID][u:dead name][b:Sex][Date4:birthday][Date4:Death day][u:Brief history][u:Nation][u:domicile])
void	cb_M_CS_SETDEAD(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_ROOMID	RoomID;		_msgin.serial(RoomID);
	ucstring	ucRoomName;	SAFE_SERIAL_WSTR(_msgin, ucRoomName, MAX_LEN_ROOM_NAME, FID);
	uint8		deadid;		_msgin.serial(deadid);
	ucstring	ucDeadname;	NULLSAFE_SERIAL_WSTR(_msgin, ucDeadname, MAX_LEN_DEAD_NAME, FID);
	uint8		deadsurnamelen;	_msgin.serial(deadsurnamelen);
	uint8		Sex;		_msgin.serial(Sex);
	uint32		Birthday;	_msgin.serial(Birthday);
	uint32		Deadday;	_msgin.serial(Deadday);
	ucstring	briefHistory;	NULLSAFE_SERIAL_WSTR(_msgin, briefHistory, MAX_LEN_DEAD_BFHISTORY, FID);
	ucstring	nation;		NULLSAFE_SERIAL_WSTR(_msgin, nation, MAX_LEN_NATION, FID);
	ucstring	domicile;	NULLSAFE_SERIAL_WSTR(_msgin, domicile, MAX_LEN_DOMICILE, FID);
	uint8		PhotoType;	_msgin.serial(PhotoType);

	CRoomWorld*	pRoom = GetOpenRoomWorld(RoomID);
	if (pRoom == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	if (pRoom->m_MasterID != FID)
	{
		sipwarning("RoomMaster can set dead! RoomID=%d, FID=%d, MasterFID=%d", RoomID, FID, pRoom->m_MasterID);
		return;
	}

	_entryDead	info(ucRoomName, ucDeadname, deadsurnamelen, RoomID, deadid, Sex, Birthday, Deadday, briefHistory, nation, domicile, PhotoType);
	//info.idRoom = RoomID;=
	//info.index = deadid;=
	//info.ucRoomName = ucRoomName;=
	//info.ucName = ucDeadname;=
	//info.surnamelen = deadsurnamelen;=
	//info.sex = Sex;=
	//info.birthDay = Birthday;=
	//info.deadDay = Deadday;=
	//info.briefHistory = briefHistory;=
	//info.nation = nation;=
	//info.domicile = domicile;=
	//info.pho

	{
		MAKEQUERY_AddDead(strQ, info.idRoom, info.ucRoomName, deadid, deadsurnamelen, ucDeadname, info.sex, info.birthDay, info.deadDay, info.briefHistory, info.domicile, info.nation, PhotoType);
		DBCaller->execute(strQ);
	}

	{
		pRoom->m_RoomName = info.ucRoomName;
		pRoom->SetDeadInfo(&info);

		DBLOG_AddDead(FID, RoomID, info.ucRoomName, info.index);
	}

	//sipinfo(L"Set Dead To Room, FID:%u, RoomId:%u, DeadIndex:%u", FID, info.idRoom, info.index); byy krs
}
//void	cb_M_CS_SETDEAD(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	FID;		_msgin.serial(FID);
//	T_ROOMID	RoomID;		_msgin.serial(RoomID);
//	ucstring	ucRoomName;	SAFE_SERIAL_WSTR(_msgin, ucRoomName, MAX_LEN_ROOM_NAME, FID);
//	uint8		deadid;		_msgin.serial(deadid);
//	ucstring	ucDeadname;	NULLSAFE_SERIAL_WSTR(_msgin, ucDeadname, MAX_LEN_DEAD_NAME, FID);
//	uint8		deadsurnamelen;	_msgin.serial(deadsurnamelen);
//	uint8		Sex;		_msgin.serial(Sex);
//	uint32		Birthday;	_msgin.serial(Birthday);
//	uint32		Deadday;	_msgin.serial(Deadday);
//	ucstring	briefHistory;	NULLSAFE_SERIAL_WSTR(_msgin, briefHistory, MAX_LEN_DEAD_BFHISTORY, FID);
//	ucstring	nation;		NULLSAFE_SERIAL_WSTR(_msgin, nation, MAX_LEN_NATION, FID);
//	ucstring	domicile;	NULLSAFE_SERIAL_WSTR(_msgin, domicile, MAX_LEN_DOMICILE, FID);
//
//	CRoomWorld*	pRoom = GetOpenRoomWorld(RoomID);
//	if (pRoom == NULL)
//	{
//		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
//		return;
//	}
//
//	if (pRoom->m_MasterID != FID)
//	{
//		sipwarning("RoomMaster can set dead! RoomID=%d, FID=%d, MasterFID=%d", RoomID, FID, pRoom->m_MasterID);
//		return;
//	}
//
//	uint32		uResult = E_COMMON_FAILED;
//
//	_entryDead	info;
//	info.idRoom = RoomID;
//	info.index = deadid;
//	info.ucRoomName = ucRoomName;
//	info.ucName = ucDeadname;
//	info.surnamelen = deadsurnamelen;
//	info.sex = Sex;
//	info.birthDay = Birthday;
//	info.deadDay = Deadday;
//	info.briefHistory = briefHistory;
//	info.nation = nation;
//	info.domicile = domicile;
//
//	try
//	{
//		MAKEQUERY_AddDead(strQ, info.idRoom, info.ucRoomName, deadid, deadsurnamelen, ucDeadname, info.sex, info.birthDay, info.deadDay, info.briefHistory, info.domicile, info.nation);
//		CDBConnectionRest	DB(DBCaller);
//		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
//		if (!nPrepareRet)
//			throw "cb_M_CS_SETDEAD: prepare statement failed!!";
//
//		int len1 = 0;	
//		sint32	ret = 0;
//
//		DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);
//
//		if (!nBindResult)
//			throw "cb_M_CS_SETDEAD: bind failed!";
//
//		DB_EXE_PARAMQUERY(Stmt, strQ);
//		if (!nQueryResult)
//		{
//			throw "cb_M_CS_SETDEAD: execute failed!";
//		}
//
//		if (ret != 0)
//			uResult = E_DBERR;
//		else
//			uResult = ERR_NOERR;
//	}
//	catch (char * str)
//	{
//		sipwarning("[FID=%d] %s", FID, str);
//		sendErrorResult(M_SC_DECEASED, FID, E_DBERR, _tsid);
//		return;
//	} 
//
//	TID	roomid = info.idRoom;
//	if (uResult == ERR_NOERR)
//	{
//		CRoomWorld *	world = GetOpenRoomWorldFromFID(FID);
//		if (world != NULL)
//		{
//			world->m_RoomName = info.ucRoomName;
//			world->SetDeadInfo(&info);
//
//			// 응답을 보내지 않는것으로 토의함
//			//world->SendDeceasedDataTo(FID);
//		}
//		//else
//		//{
//		//	CMessage msgout(M_NT_SETDEAD);
//		//	msgout.serial(roomid, FID);
//		//	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgout);
//
//		//	SendDeadInfo(FID, roomid, NULL, _tsid);
//		//}
//		DBLOG_AddDead(FID, roomid, info.ucRoomName, info.index);
//
//		// 정품천원인 경우 정보 갱신
//		NotifyChangeJingPinTianYuanInfo(roomid);
//	}
//	else
//	{
//		// 응답을 보내지 않는것으로 토의함
//		//CMessage	msgout(M_SC_DECEASED);
//		//msgout.serial(FID);
//		//msgout.serial(uResult, roomid);
//		//CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//	}
//
//	sipinfo(L"Set Dead To Room, Result:%u, FID:%u, RoomId:%u, DeadIndex:%u", uResult, FID, info.idRoom, info.index);
//}

// Set dead Figure ([d:Room id][b:DeadID][d:PhotoId][b:PhotoType])
void cb_M_CS_SET_DEAD_FIGURE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_ROOMID	RoomID;		_msgin.serial(RoomID);
	uint8		DeadID;		_msgin.serial(DeadID);
	uint32		PhotoID;	_msgin.serial(PhotoID);

	CRoomWorld*	pRoom = GetOpenRoomWorld(RoomID);
	if(pRoom == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
		return;
	}

	if(pRoom->m_MasterID != FID)
	{
		sipwarning("RoomMaster can set deadFigure! RoomID=%d, FID=%d, MasterFID=%d", RoomID, FID, pRoom->m_MasterID);
		return;
	}

	uint32		uResult = ERR_NOERR;
	try
	{
		MAKEQUERY_SetDeceasedPhoto(strQ, RoomID, DeadID, PhotoID);
		DBCaller->executeWithParam(strQ, NULL,
			"D",
			OUT_,		NULL);

		uResult = ERR_NOERR;
		CRoomWorld*	pRoomWorld = GetOpenRoomWorld(RoomID);
		if(pRoomWorld)
		{
			if(DeadID == 1)
			{
				if(pRoomWorld->m_Deceased1.m_bValid)
				{
					pRoomWorld->m_Deceased1.m_PhotoId = PhotoID;
				}
			}
			else
			{
				if(pRoomWorld->m_Deceased2.m_bValid)
				{
					pRoomWorld->m_Deceased2.m_PhotoId = PhotoID;
				}
			}
		}
	}
	catch(char * str)
	{
		str;
		uResult = E_DBERR;
	}
	sendErrorResult(M_SC_SET_DEAD_FIGURE, FID, uResult, _tsid);
}

static void	DBCB_DBGetDeceasedInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32		ret			= *(sint32 *)argv[0];
	T_FAMILYID	FID			= *(T_FAMILYID *)argv[1];
	T_ROOMID	RoomID		= *(T_ROOMID *)argv[2];
	TServiceId	tsid(*(uint16 *)argv[3]);

	uint32 uResult = ERR_NOERR;
	try
	{
		if (!nQueryResult || ret == -1)
			throw "Shrd_room_getDeceased2: execute failed!";

		CMessage	msgout(M_SC_DECEASED);
		msgout.serial(FID);	

		switch (ret)
		{
		case 0:
			{
				uint32 nRows;	resSet->serial(nRows);
				if (nRows > 0)
				{
					T_ROOMNAME		roomname;				resSet->serial(roomname);
					uint8			surnamelen1;			resSet->serial(surnamelen1);
					T_DECEASED_NAME deadname1;				resSet->serial(deadname1);
					T_SEX			sex1;					resSet->serial(sex1);
					uint32			birthday1;				resSet->serial(birthday1);
					uint32			deadday1;				resSet->serial(deadday1);
					T_COMMON_CONTENT his1;					resSet->serial(his1);
					T_DATETIME		updatetime1;			resSet->serial(updatetime1);
					ucstring		domicile1;				resSet->serial(domicile1);
					ucstring		nation1;				resSet->serial(nation1);
					uint32			photoid1;				resSet->serial(photoid1);
					uint8			phototype;				resSet->serial(phototype);
					uint8			surnamelen2;			resSet->serial(surnamelen2);
					T_DECEASED_NAME deadname2;				resSet->serial(deadname2);
					T_SEX			sex2;					resSet->serial(sex2);
					uint32			birthday2;				resSet->serial(birthday2);
					uint32			deadday2;				resSet->serial(deadday2);
					T_COMMON_CONTENT his2;					resSet->serial(his2);
					T_DATETIME		updatetime2;			resSet->serial(updatetime2);
					ucstring		domicile2;				resSet->serial(domicile2);
					ucstring		nation2;				resSet->serial(nation2);
					uint32			photoid2;				resSet->serial(photoid2);

					msgout.serial(uResult, RoomID, roomname);
					msgout.serial(deadname1, surnamelen1, sex1, birthday1, deadday1, his1, nation1, domicile1, photoid1);
					msgout.serial(phototype);
					msgout.serial(deadname2, surnamelen2, sex2, birthday2, deadday2, his2, nation2, domicile2, photoid2);
				}
				break;
			}
		case 1043:
			{
				uResult = E_DECEASED_NODATA;
				//sipdebug("the room:%u has no dead information", RoomID);		byy krs		
				break;			
			}
		case 1014:
			{
				uResult = E_ENTER_NO_NEIGHBOR;
				//sipdebug("the room:%u open type is ROOMOPEN_TONEIGHBOR, the user:%u is not contact of the room's master", RoomID, FID);				 byy krs
				break;
			}
		case 1037:
			{
				uResult = ERR_BADPASSWORD;
				sipwarning("the room:%u open type is ROOMOPEN_PASSWORD", RoomID);
				break;			
			}
		case 1044: // 삭제된방
			{
				uResult = E_ENTER_DELETED;
				//sipdebug("the room:%u has no dead information", RoomID);				 byy krs
				break;			
			}
		case 1045: // 비공개방
			{
				uResult = E_ENTER_NO_ONLYMASTER;
				//sipdebug("the room:%u has no dead information", RoomID);				byy krs
				break;			
			}
		case 1046: // 기한지난방
			{
				uResult = E_ENTER_EXPIRE;
				//sipdebug("the room:%u has no dead information", RoomID);				 byy krs
				break;			
			}
		default:
			break;
		}

		if (uResult != ERR_NOERR)
		{
			msgout.serial(uResult, RoomID);
		}
		CUnifiedNetwork::getInstance()->send(tsid, msgout);
		//sipinfo("Send room(id:%u) info to userid(id:%u), Result=%d", RoomID, FID, uResult); byy krs
	}
	catch(char * str)
	{
		uResult = E_DBERR;
		CMessage	msgout(M_SC_DECEASED);
		msgout.serial(FID);
		msgout.serial(uResult, RoomID);
		CUnifiedNetwork::getInstance()->send(tsid, msgout);
		sipwarning("[FID=%d] %s", FID, str);			
	}
}
//////////////////////////////////////////////////////////////////////////
extern void	DBCB_DBGetDeceasedForRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet);
void	cb_M_CS_DECEASED(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	T_ROOMID		RoomID;			_msgin.serial(RoomID);
	std::string		passWord;		NULLSAFE_SERIAL_STR(_msgin, passWord, MAX_LEN_ROOM_VISIT_PASSWORD, FID);

	uint32 uResult = ERR_NOERR;

	// 현재 방에 들어가있으며, 그 방의 고인정보를 보려고 하는 경우에는 Password를 물어보지 않는다.
	T_FAMILYID	FID_temp = FID;
	CRoomWorld *	inManager = GetOpenRoomWorld(RoomID);
	if(inManager != NULL)
	{
		TRoomFamilys::iterator	it = inManager->m_mapFamilys.find(FID);
		if(it != inManager->m_mapFamilys.end())
			FID_temp = inManager->m_MasterID;
	}
	else if (!IsThisShardRoom(RoomID))
	{
		uint16	fsSid = _tsid.get();
		CMessage	msgOut(M_CS_DECEASED);
		msgOut.serial(fsSid, FID, RoomID, passWord);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
		return;
	}
	// 요청한 사용자가 GM이면 모두 허가한다
	FAMILY* pFamily = GetFamilyData(FID);
	if (pFamily != NULL && pFamily->m_bGM)
	{
		MAKEQUERY_GetDeceasedForRoom(strQ, RoomID);
		DBCaller->executeWithParam(strQ, DBCB_DBGetDeceasedForRoom,
			"DDW",
			CB_,			FID,
			CB_,			RoomID,
			CB_,			_tsid.get());
	}
	else
	{
		MAKEQUERY_GetDeceasedInfo(strQ, RoomID, FID_temp, passWord);
		DBCaller->executeWithParam(strQ, DBCB_DBGetDeceasedInfo,
			"DDDW",
			OUT_,			NULL,
			CB_,			FID,
			CB_,			RoomID,
			CB_,			_tsid.get());
	}
}

//static void	DBCB_DBGetDeadMemorialDaysInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID			familyId		= *(T_FAMILYID *)argv[0];
//	TServiceId			tsid(*(uint16 *)argv[1]);
//
//	if (nQueryResult != true)
//		return;
//
//	std::vector<_entryDead>	aryDeads;
//	uint16	nCount = 0;
//	uint32 nRows;	resSet->serial(nRows);
//	for ( uint32 i = 0; i < nRows; i ++)
//	{
//		uint32				idRoom;			resSet->serial(idRoom);
//		uint8				index;			resSet->serial(index);
//		uint8				surnamelen;		resSet->serial(surnamelen);
//		T_DECEASED_NAME		deadname;		resSet->serial(deadname);
//		T_DATE4				birthday;		resSet->serial(birthday);
//		T_DATE4				deadday;		resSet->serial(deadday);
//		ucstring			roomname;		resSet->serial(roomname);
//
//		_entryDead	info(roomname, deadname, surnamelen, idRoom,index, 0, birthday, deadday, "", "", "", "");
//		aryDeads.push_back(info);
//
//		nCount ++;
//	}
//	CMessage	msgout(M_SC_DEADSMEMORIALDAY);
//	msgout.serial(familyId, nCount);
//	for ( uint i = 0; i < nCount; i ++ )
//	{
//		_entryDead	&dead = aryDeads[i];
//		msgout.serial(dead.idRoom);
//		msgout.serial(dead.index);
//		msgout.serial(dead.ucName);
//		msgout.serial(dead.surnamelen);
//		msgout.serial(dead.birthDay);
//		msgout.serial(dead.deadDay);
//		msgout.serial(dead.ucRoomName);
//	}
//	CUnifiedNetwork::getInstance ()->send(tsid, msgout);
//	sipinfo(L"-> [M_SC_DEADSMEMORIALDAY] Send User %d Dead's MemorialDays days: %u", familyId, nCount);
//}
// packet : M_SC_DEADSMEMORIALDAY
// login
//void	SendOwnDeadMemorialDaysInfo(T_FAMILYID familyId, SIPNET::TServiceId _tsid)
//{
//	MAKEQUERY_GETDEADMEMORIALDAY(strQuery, familyId);
//	DBCaller->executeWithParam(strQuery, DBCB_DBGetDeadMemorialDaysInfo,
//		"DW",
//		CB_,		familyId,
//		CB_,		_tsid.get());
///*
//	CMessage	msgout(M_SC_DEADSMEMORIALDAY);
//	msgout.serial(familyId);
//
//	MAKEQUERY_GETDEADMEMORIALDAY(strQuery, familyId);
//	DB_EXE_QUERY(DB, Stmt, strQuery);
//	if (nQueryResult != true)
//		return;
//
//	std::vector<_entryDead>	aryDeads;
//	uint16	nCount = 0;
//	do
//	{
//		DB_QUERY_RESULT_FETCH(Stmt, row);
//		if ( !IS_DB_ROW_VALID(row) )
//			break;
//
//		COLUMN_DIGIT(row,	0,	uint32,	idRoom);
//		COLUMN_DIGIT(row,	1,	uint8,	index);
//		COLUMN_DIGIT(row,	2,	uint8,	surnamelen);
//		COLUMN_WSTR(row,	3,	deadname,	MAX_LEN_DEAD_NAME);
//        COLUMN_DIGIT(row,	4, 	T_DATE4, birthday);
//		COLUMN_DIGIT(row,	5, 	T_DATE4, deadday);
//
//		/////////////////////////////////////?????/////////////////////////////////
//		_entryDead	info("", deadname, surnamelen, idRoom,index, 0, birthday, deadday, "", "", "", "");
//		aryDeads.push_back(info);
//
//		nCount ++;
//	}while ( true );
//
//
//	msgout.serial(nCount);
//	for ( uint i = 0; i < nCount; i ++ )
//	{
//		_entryDead	&dead = aryDeads[i];
//		msgout.serial(dead.idRoom);
//        msgout.serial(dead.index);
//		msgout.serial(dead.ucName);
//		msgout.serial(dead.surnamelen);
//		msgout.serial(dead.birthDay);
//		msgout.serial(dead.deadDay);
//	}
//	CUnifiedNetwork::getInstance ()->send(_tsid, msgout);
//	sipinfo(L"-> [M_SC_DEADSMEMORIALDAY] Send User %d Dead's MemorialDays days: %u", familyId, nCount);
//	*/
//}

//// 방에 남긴글 얻기
//void	cb_M_CS_REQRMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
//	uint32              year_moth;      _msgin.serial(year_moth);
//	uint32				PagePos;		_msgin.serial(PagePos);
//	uint8               SearchType;     _msgin.serial(SearchType);
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//	{
//		// 비법발견
//		return;
//	}
//	inManager->ReqRoomMemorial(FID, PagePos, _tsid, year_moth, SearchType);
//}

// 방에 글을 남긴다
static void	DBCB_DBAddRoomMemorial(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			FID	= *(T_FAMILYID *)argv[0];
	T_ROOMID			RoomID	= *(T_ROOMID *)argv[1];
	uint8				DeskIndex = *(uint8 *)argv[2];
	T_COMMON_CONTENT	_memo	= (ucchar *)argv[3];
	uint8				bSecret	= *(uint8 *)argv[4];
	TServiceId			tsid(*(uint16 *)argv[5]);

	if (!nQueryResult)
		return;

	CRoomWorld *inManager = GetOpenRoomWorld(RoomID);
	if (inManager == NULL)
		return;

	T_ROOMVDAYS oldvdays = inManager->m_VDays;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		uint32 ret;		resSet->serial(ret);
		if (ret == 0)
		{
			T_COMMON_ID mid;		resSet->serial(mid);
			T_DATETIME	mday;		resSet->serial(mday);
			T_ROOMVDAYS vdays;		resSet->serial(vdays);

			FAMILY	*family = GetFamilyData(FID);
			if ( !family )
			{
				sipwarning(L"No such family : FID=%d", FID);
				return;
			}

			CMessage msg(M_SC_NEWRMEMO);
			msg.serial(FID, RoomID);
			ucstring vname = family->m_Name;
			inManager->m_VDays = vdays;
			msg.serial(mid, FID, vname, mday, _memo, bSecret);
			CUnifiedNetwork::getInstance()->send(tsid, msg);

			// 길상등탁 처리
			RoomChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
			if (pChannelData == NULL)
			{
				sipwarning("GetFamilyChannel = NULL. FID=%d, RoomID=%d", FID, inManager->m_RoomID);
				return;
			}
			pChannelData->m_BlessLamp[DeskIndex].MemoID = mid;
			pChannelData->m_BlessLamp[DeskIndex].FID = FID;
			pChannelData->m_BlessLamp[DeskIndex].FName = vname;
			pChannelData->m_BlessLamp[DeskIndex].Time = (uint32)GetDBTimeSecond();
			{
				uint8	flag = 0;
				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ROOMMEMO_UPDATE)
					msgOut.serial(DeskIndex, mid, FID, vname, flag);
				FOR_END_FOR_FES_ALL()
			}
			//sipinfo("Blessslamp Added. FID=%d, RoomID=%d, DeskIndex=%d", FID, RoomID, DeskIndex); byy krs

			// For EXP
			ChangeFamilyExp(Family_Exp_Type_Memo, FID);
			inManager->ChangeRoomExp(Room_Exp_Type_Memo, FID);

			// 기록 보관
			INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(FID);
			if (pFamilyInfo == NULL)
			{
				sipwarning(L"GetFamilyInfo = NULL. RoomID=%d, FID=%d", RoomID, FID);
			}
			else
			{
				pFamilyInfo->DonActInRoom(ROOMACT_JIXIANGDENG);

				MAKEQUERY_AddFishFlowerVisitData(strQ, pFamilyInfo->m_nCurActionGroup, FF_BLESSLAMP, mid, 0);
				DBCaller->execute(strQ);
			}

			//sipinfo(L"Add room memorial result : FID=%d, RoomID=%d", FID, RoomID); byy krs
		}
		else
			sipwarning(L"Failed add room memorial : FID=%d, RoomID=%d, Result=%d", FID, RoomID, ret);
	}	

	//if (oldvdays != m_VDays)
	//{
	//	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ROOMPRO)
	//		msgOut.serial(m_RoomID, m_Exp, m_Virtue, m_Fame, m_Appreciation, m_Level);
	//	FOR_END_FOR_FES_ALL()
	//}
}

static bool bJiShangDengTest = false;
// 방에 글 남기기
void	cb_M_CS_NEWRMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
	T_COMMON_CONTENT	memo;			SAFE_SERIAL_WSTR(_msgin, memo, MAX_LEN_ROOM_MEMORIAL, FID);
	uint8               bSecret;        _msgin.serial(bSecret);                  

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	if (!inManager->CheckIfActSave(FID))
		return;

	// 이 처리는 행사중이여야만 한다. 왜? ActPos를 얻기 위하여
	uint32	ChannelID = inManager->GetFamilyChannelID(FID);
	int		index = inManager->GetActivitingIndexOfUser(ChannelID, FID);
	if(index < 0)
	{
		sipwarning("GetActivitingIndexOfUser failed. FID=%d", FID);
		return;
	}
	if ((index + 1 < ACTPOS_BLESS_LAMP) || (index >= ACTPOS_BLESS_LAMP + MAX_BLESS_LAMP_COUNT))
	{
		sipwarning("Invalid ActPos. RoomID=%d, FID=%d, ActPos=%d", inManager->m_RoomID, FID, index + 1);
		return;
	}

	uint8	DeskIndex = (uint8)(index + 1 - ACTPOS_BLESS_LAMP);

	// 한사람은 하루에 한 기념관에서 1개까지의 길상등을 남길수 있다.
	TmapDwordToDword::iterator it = inManager->m_mapFamilyBlesslampTime.find(FID);
	if(it != inManager->m_mapFamilyBlesslampTime.end())
	{
		if(it->second >= BLESS_LAMP_PUT_COUNT && !bJiShangDengTest)
		{
			return;
		}
		it->second ++;
	}
	else
	{
		inManager->m_mapFamilyBlesslampTime[FID] = 1;
	}

	T_ROOMID	RoomID = inManager->m_RoomID;
	MAKEQUERY_ADDROOMMEMORIAL(strQ, FID, RoomID, memo, bSecret);
	DBCaller->executeWithParam(strQ, DBCB_DBAddRoomMemorial,
		"DDBSBW",
		CB_,			FID,
		CB_,			RoomID,
		CB_,			DeskIndex,
		CB_,			memo.c_str(),
		CB_,			bSecret,
		CB_,			_tsid.get());
}

//// modify room's memorial
//void    cb_M_CS_MODIFYRMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
//    T_COMMON_ID         memorialId;     _msgin.serial(memorialId);
//	T_COMMON_CONTENT	memo;			SAFE_SERIAL_WSTR(_msgin, memo, MAX_LEN_ROOM_MEMORIAL, FID);
//	uint8               bSecret;        _msgin.serial(bSecret); 
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	VEC_LETTER_MODIFY_LIST::iterator it;
//	for(it = inManager->m_MemoModifyList.begin(); it != inManager->m_MemoModifyList.end(); it ++)
//	{
//		if((it->FID == FID) && (it->LetterID == memorialId))
//			break;
//	}
//	if(it == inManager->m_MemoModifyList.end())
//	{
//		sipwarning("it == inManager->m_MemoModifyList.end(). FID=%d, MemoID=%d", FID, memorialId);
//		return;
//	}
//	inManager->m_MemoModifyList.erase(it);
//
//	MAKEQUERY_ModifyMemo(strQ, FID, memorialId, memo, bSecret);
//	DBCaller->executeWithParam(strQ, NULL,
//		"D",
//		OUT_,			NULL);
//}

//// 방글 삭제
//void	cb_M_CS_DELRMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
//	T_COMMON_ID			memoid;			_msgin.serial(memoid);
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->DelRoomMemorial(FID, memoid, _tsid);
//}

//// 방글 삭제
//void	CRoomWorld::DelRoomMemorial(T_FAMILYID FID, T_COMMON_ID _memoid, SIPNET::TServiceId _sid)
//{
//	if (_memoid == 0)
//		return;
//
//	uint32	err = ERR_NOERR;
//	{
//		VEC_LETTER_MODIFY_LIST::iterator it;
//		for(it = m_MemoModifyList.begin(); it != m_MemoModifyList.end(); it ++)
//		{
//			if(it->LetterID == _memoid)
//			{
//				err = E_MEMO_MODIFYING;
//				CMessage	msgOut(M_SC_DELRMEMO);
//				msgOut.serial(FID, m_RoomID, _memoid, err);
//				CUnifiedNetwork::getInstance()->send(_sid, msgOut);
//				return;
//			}
//		}
//	}
//
//	if((FID == m_MasterID) || (IsRoomManager(FID)))	// 관리원은 방문록의 글을 삭제할수 있다.
//	{
//		MAKEQUERY_DELROOMMEMORIALFORMASTER(strQ, _memoid, m_RoomID);
//		DBCaller->execute(strQ);
//	}
//	else
//	{
//		MAKEQUERY_DELROOMMEMORIAL(strQ, _memoid, FID);
//		DBCaller->execute(strQ);
//	}
//
//	CMessage	msgOut(M_SC_DELRMEMO);
//	msgOut.serial(FID, m_RoomID, _memoid, err);
//	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
//
//	// 길상등탁에 있는 경우 삭제
//	RoomChannelData	*pChannelData = GetFamilyChannel(FID);
//	if (pChannelData == NULL)
//	{
//		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
//		return;
//	}
//	for (uint8 DeskPos = 0; DeskPos < MAX_BLESS_LAMP_COUNT; DeskPos ++)
//	{
//		if (pChannelData->m_BlessLamp[DeskPos].MemoID == _memoid)
//		{
//			pChannelData->m_BlessLamp[DeskPos].MemoID = 0;
//			{
//				uint32	zero = 0;
//				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ROOMMEMO_UPDATE)
//					msgOut.serial(DeskPos, zero);
//				FOR_END_FOR_FES_ALL()
//			}
//			break;
//		}
//	}
//}

//// Modify memo Start ([d:MemoID])
//void    cb_M_CS_MODIFY_MEMO_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;
//    T_COMMON_ID         MemoID;
//	_msgin.serial(FID, MemoID);
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;
//
//	_Letter_Modify_Data	data;
//	data.FID = FID;
//	data.LetterID = MemoID;
//	inManager->m_MemoModifyList.push_back(data);
//}

//// Modify memo End ([d:MemoID])
//void    cb_M_CS_MODIFY_MEMO_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;
//	T_COMMON_ID         MemoID;
//	_msgin.serial(FID, MemoID);
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;
//
//	VEC_LETTER_MODIFY_LIST::iterator it;
//	for(it = inManager->m_MemoModifyList.begin(); it != inManager->m_MemoModifyList.end(); it ++)
//	{
//		if((it->FID == FID) && (it->LetterID == MemoID))
//		{
//			inManager->m_MemoModifyList.erase(it);
//			break;
//		}
//	}
//}

//static void	DBCB_DBGetOneMemorisl(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID	FID		= *(T_FAMILYID *)argv[0];
//	uint32		MemoID	= *(uint32 *)argv[1];
//	TServiceId	tsid(*(uint16 *)argv[2]);
//
//	if ( !nQueryResult )
//		return;
//
//	uint32 nRows;	resSet->serial(nRows);
//	if (nRows != 1)
//	{
//		sipwarning(L"nRows != 1. FID=%d, MemoID=%d, nRows=%d", FID, MemoID, nRows);
//		return;
//	}
//
//	resSet->serial(MemoID);
//	uint32			vFID;		resSet->serial(vFID);
//	T_FAMILYNAME	vFName;		resSet->serial(vFName);
//	T_DATETIME		MemoDate;	resSet->serial(MemoDate);
//	ucstring		Memo;		resSet->serial(Memo);
//	uint8			Protected;	resSet->serial(Protected);
//
//	CMessage	msgOut(M_SC_ROOMMEMO_DATA);
//	msgOut.serial(FID, MemoID, vFID, vFName, MemoDate, Memo, Protected);
//	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
//}

//// Request RoomMemo Data ([d:MemoID])
//void	cb_M_CS_ROOMMEMO_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	uint32			MemoID;		_msgin.serial(MemoID);
//
//	MAKEQUERY_GetOneMemorisl(strQ, MemoID);
//	DBCaller->executeWithParam(strQ, DBCB_DBGetOneMemorisl,
//		"DDW",
//		CB_,		FID,
//		CB_,		MemoID,
//		CB_,		_tsid.get());
//}

static void	DBCB_DBGetVisitList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			FID				= *(T_FAMILYID *)argv[0];
	T_ROOMID			RoomID			= *(T_ROOMID *)argv[1];
	uint32				page			= *(uint32 *)argv[2];
	uint8				bOnlyOwnData	= *(uint8 *)argv[3];
	TServiceId			tsid(*(uint16 *)argv[4]);

	if ( !nQueryResult )
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipdebug(L"CRoomWorld::ReqVisitList failed(Can't get allcount).");
		return;
	}

	uint32	ret;
	CMessage msg(M_SC_ROOMVISITLIST);
	msg.serial(FID, RoomID, bOnlyOwnData);

	uint32 allcount;			resSet->serial(allcount);
	uint32 mmcount;				resSet->serial(mmcount);
	uint32 _ret;				resSet->serial(_ret);

	msg.serial(allcount, mmcount, page);
	ret = _ret;

	if (ret == 0)
	{
		uint32 nRows2;	resSet->serial(nRows2);
		for ( uint32 i = 0; i < nRows2; i ++)
		{
			T_ID			GroupId;			resSet->serial(GroupId);
			T_FAMILYID		FamilyId;			resSet->serial(FamilyId);
			T_FAMILYNAME	FamilyName;			resSet->serial(FamilyName);
			uint16			Year;				resSet->serial(Year);
			uint8			Month;				resSet->serial(Month);
			uint8			Day;				resSet->serial(Day);
			uint16			HasAction;			resSet->serial(HasAction);// 2014/06/19 클라이언트에서는 uint8 범위까지만의 목록을 사용하는것으로한다. 
			uint16			HasYishi;			resSet->serial(HasYishi);// 2014/06/19 클라이언트에서는 uint8 범위까지만의 목록을 사용하는것으로한다.

			uint32 date = (Year << 16) | (Month << 8) | Day;

			// 2014/06/19:start:한방에서 한사람이 256번이상 행사할수도 있다! 그러나 파키트수정을 안하기 위해 200개만 볼수 있도록 한다.
			uint8 	bHasAction = 0, bHasYishi = 0;
			if ( HasAction >= 200 )
				bHasAction = 200;
			else
				bHasAction = (uint8)HasAction;
			if ( HasYishi >= 200 )
				bHasYishi = 200;
			else
				bHasYishi = (uint8)HasYishi;

// 			msg.serial(GroupId, FamilyId, FamilyName, date, HasAction, HasYishi);
			msg.serial(GroupId, FamilyId, FamilyName, date, bHasAction, bHasYishi);
			// end
		}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msg);

	//sipdebug(L"CRoomWorld::ReqVisitList success."); byy krs
}

// 방문기록그룹목록 얻기
// [d:page][b:bOnlyOwnData]
void	cb_M_CS_REQRVISITLIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);
	uint32				page;			_msgin.serial(page);
	uint8				bOnlyOwnData;	_msgin.serial(bOnlyOwnData);
	uint8				PageSize;		_msgin.serial(PageSize);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

//	inManager->ReqVisitList(FID, year, month, page, _tsid, bOnlyOwnData);

	tchar strQuery[300] = _t("");
	if(bOnlyOwnData)
	{
		MAKEQUERY_GetVisitList_FID(strQ, inManager->m_RoomID, page, FID, PageSize);
		wcscpy(strQuery, strQ);
	}
	else
	{
		MAKEQUERY_GetVisitList(strQ, inManager->m_RoomID, page, PageSize);
		wcscpy(strQuery, strQ);
	}
	DBCaller->executeWithParam(strQuery, DBCB_DBGetVisitList,
		"DDDBW",
		CB_,			FID,
		CB_,			inManager->m_RoomID,
		CB_,			page,
		CB_,			bOnlyOwnData,
		CB_,			_tsid.get());
}

// 방문기록 추가
// [b:action type][u:pray text][b:is secret][d:StoreLockID][b:Number]<[d:SyncCode][ [d:InvenPos] ]>
void	cb_M_CS_ADDACTIONMEMO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
	uint8				action_type;	_msgin.serial(action_type);
	ucstring			pray = "";
	uint8				secret = 0;
	uint32				StoreLockID = 0;
	uint8				number = 0;
	uint32				SyncCode = 0;
	uint16				invenpos[MAX_ITEM_PER_ACTIVITY] = {0};
	T_ITEMSUBTYPEID		itemid[MAX_ITEM_PER_ACTIVITY] = {0};

	if (action_type != AT_VISIT)
	{
		NULLSAFE_SERIAL_WSTR(_msgin, pray, (MAX_LEN_ROOM_PRAYTEXT+MAX_LEN_DEAD_NAME+10), FID);
		_msgin.serial(secret, StoreLockID, number);
		if (number > MAX_ITEM_PER_ACTIVITY)
		{
			// 비법발견
			ucstring ucUnlawContent = SIPBASE::_toString("Count is too big! count=%u", number).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		uint8 i = 0;
		if (number > 0)
		{
			_msgin.serial(SyncCode);
			for (i = 0; i < number; i ++)
			{
				_msgin.serial(invenpos[i]);
				_msgin.serial(itemid[i]);
			}
		}
		else
		{
			// 조상탑 고인에게 향/꽃드리기인 경우에는 아이템이 꼭 있어야 한다.
			if ((action_type >= AT_ANCESTOR_DECEASED_XIANG) && (action_type <= AT_ANCESTOR_DECEASED_HUA + MAX_ANCESTOR_DECEASED_COUNT))
			{
				sipwarning("Item Number = 0!!! action_type=%d", action_type);
				return;
			}
		}
	}

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	//sipdebug("FID=%d, RoomID=%d, ActionType=%d, Secret=%d, StoreID=%d, ItemCount=%d, SyncCode=%d, ItemPos[0]=%d, itemid[0]=%d", FID, inManager->m_RoomID, action_type, secret, StoreLockID, number, SyncCode, invenpos[0], itemid[0]); byy krs

	inManager->AddActionMemo(FID, action_type, 0, pray, secret, StoreLockID, &invenpos[0], itemid, SyncCode, _tsid);
}

static void	DBCB_DBGetVisitGroup1(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			FID		= *(T_FAMILYID *)argv[0];
	T_ID				GroupID	= *(uint32 *)argv[1];
	T_ROOMID			RoomID	= *(T_ROOMID *)argv[2];
	TServiceId			tsid(*(uint16 *)argv[3]);

	if(!nQueryResult)
		return;
	{
		CMessage msgOut(M_SC_VISIT_DETAIL_LIST);
		uint32 nRows;	resSet->serial(nRows);
		uint32 nRowsReal = nRows;
		if (nRowsReal > 200) // 만일을 위한 검사이다. 현실적으로 한 방에서 한 사용자가 하루에 255번이상 행사를 하는 경우가 있겠는지? - 있다!!!
		{
			sipwarning("M_SC_VISIT_DETAIL_LIST : nRows > 300. RoomID=%d, GroupID=%d", RoomID, GroupID);
			nRowsReal = 200;
		}
		msgOut.serial(FID, RoomID, GroupID, nRowsReal);
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint32		ActionId;				resSet->serial(ActionId);
			T_DATETIME	DateTime;				resSet->serial(DateTime);
			uint8		ActionType;				resSet->serial(ActionType);

			if (i < nRowsReal)
			{
				uint32	time_min = getMinutesSince1970(DateTime);
				msgOut.serial(ActionId, time_min, ActionType);
			}
		}

		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}

	{
		CMessage msgOut(M_SC_VISIT_FISHFLOWER_LIST);
		uint32 nRows;	resSet->serial(nRows);
		if (nRows > 300) // 만일을 위한 검사이다. 현실적으로 한 방에서 한 사용자가 하루에 300번이상 물고기먹이주기 하는 경우가 있겠는지?
		{
			sipwarning("M_SC_VISIT_FISHFLOWER_LIST : nRows > 300. RoomID=%d, GroupID=%d", RoomID, GroupID);
			nRows = 300;
		}
		msgOut.serial(FID, RoomID, GroupID);
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint8		Type;				resSet->serial(Type);
			uint32		ItemID;				resSet->serial(ItemID);
			T_DATETIME	DateTime;			resSet->serial(DateTime);
			uint32		OldFlowerID;		resSet->serial(OldFlowerID);

			uint32	time_min = getMinutesSince1970(DateTime);

			msgOut.serial(time_min, Type, ItemID, OldFlowerID);
		}

		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}

	//sipdebug(L"CRoomWorld::ReqActionMemo success."); byy krs
}

// Request room visit detail list ([w:Year][d:GroupID])
void	cb_M_CS_VISIT_DETAIL_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;
	uint16				year;
	uint32				GroupID;
	_msgin.serial(FID, year, GroupID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	MAKEQUERY_GetVisitGroup1(strQ, GroupID);
	DBCaller->executeWithParam(strQ, DBCB_DBGetVisitGroup1,
		"DDDW",
		CB_,			FID,
		CB_,			GroupID,
		CB_,			inManager->m_RoomID,
		CB_,			_tsid.get());
}

static void	DBCB_DBGetVisitData(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			FID				= *(T_FAMILYID *)argv[0];
	T_ROOMID			RoomID			= *(T_ROOMID *)argv[1];
	uint32				visit_id		= *(uint32 *)argv[2];
	TServiceId			tsid(*(uint16 *)argv[3]);

	if ( !nQueryResult )
		return;

	uint32	nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("Can't find VisitData. FID=%d, RoomID=%d, VisitID=%d", FID, RoomID, visit_id);
		return;
	}

	uint8	byItemBuf[MAX_ITEM_PER_ACTIVITY * sizeof(uint32) + 1];

	byItemBuf[0] = 0;

	uint32		HostFID;		resSet->serial(HostFID);
	ucstring	HostFName;		resSet->serial(HostFName);
	T_DATETIME	DateTime;		resSet->serial(DateTime);
	uint8		ActionType;		resSet->serial(ActionType);
	ucstring	Pray;			resSet->serial(Pray);
	uint8		Secret;			resSet->serial(Secret);
	uint32		len=0;			resSet->serialBufferWithSize(byItemBuf, len);
	uint8		isShielded;		resSet->serial(isShielded);
	uint32		GroupID;		resSet->serial(GroupID);

	_VisitMemo	data(visit_id, ActionType, DateTime, Secret, Pray, byItemBuf[0], (uint32*)(&byItemBuf[1]), isShielded);

	uint8		finished;
	if ((ActionType != AT_MULTIRITE) && (ActionType != AT_ANCESTOR_MULTIJISI) && (ActionType != AT_XDGMULTIRITE))
	{
		uint32	visit_subid = 0;
		CMessage	msgOut(M_SC_VISIT_DATA);
		finished = 1;
		msgOut.serial(FID);
		msgOut.serial(visit_id, visit_subid, GroupID, ActionType, finished);
		msgOut.serial(HostFID, HostFName, Pray, Secret, isShielded);
		msgOut.serial(data.item_count);
		for(uint j = 0; j < data.item_count; j ++)
			msgOut.serial(data.items[j]);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
	else
	{
		// 단체행사인 경우
		nRows;	resSet->serial(nRows);
		if(nRows == 0)
			finished = 1;
		else
			finished = 0;

		// HostFID자료 보내기
		{
			uint32	visit_subid = 0;
			CMessage	msgOut(M_SC_VISIT_DATA);
			msgOut.serial(FID);
			msgOut.serial(visit_id, visit_subid, GroupID, ActionType, finished);
			msgOut.serial(HostFID, HostFName, Pray, Secret, isShielded);
			msgOut.serial(data.item_count);
			for(uint j = 0; j < data.item_count; j ++)
				msgOut.serial(data.items[j]);
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		}

		// JoinFID자료 보내기
		for(uint32 i = 0; i < nRows; i ++)
		{
			uint32		Id, JoinFID, ItemID;
			uint8		Secret, Shield;
			ucstring	JoinFName, Pray;
			uint8		Count;

			resSet->serial(Id, JoinFID, JoinFName, Secret, Shield, ItemID, Pray, GroupID);

			if(i + 1 == nRows)
				finished = 1;
			else
				finished = 0;
			CMessage	msgOut(M_SC_VISIT_DATA);
			msgOut.serial(FID);
			msgOut.serial(visit_id, Id, GroupID, ActionType, finished);
			msgOut.serial(JoinFID, JoinFName, Pray, Secret, Shield);
			if(ItemID == 0)
			{
				Count = 0;
				msgOut.serial(Count);
			}
			else
			{
				Count = 1;
				msgOut.serial(Count, ItemID);
			}

			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
		}
	}
}

// Request room action data of activity ([d:action id])
void	cb_M_CS_VISIT_DATA(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint16			year;
	uint32			action_id;
	_msgin.serial(FID, year, action_id);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;

	MAKEQUERY_GetVisitData(strQ, action_id);
	DBCaller->executeWithParam(strQ, DBCB_DBGetVisitData,
		"DDDW",
		CB_,			FID,
		CB_,			inManager->m_RoomID,
		CB_,			action_id,
		CB_,			_tsid.get());
}

// Shield action memo([d:action id][d:action subid][b:ShildFlag])
void	cb_M_CS_VISIT_DATA_SHIELD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;
	uint16				year;
	uint32				action_subid, action_id;
	uint8				shieldflag;		
	_msgin.serial(FID, year, action_id, action_subid, shieldflag);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;

	if((FID != inManager->m_MasterID) && (!inManager->IsRoomManager(FID)))	// 방관리자도 Shield를 할수 있다.
	{
		sipwarning(L"Can't Shield MultiActionMemo in CRoomWorld::SheildActionMemo, because you are not master.");
		return;
	}

	MAKEQUERY_ShieldVisitData(strQ, action_id, action_subid, shieldflag);
	DBCaller->execute(strQ);
}

static void	DBCB_DBGetVisitDataSubID(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID				= *(T_FAMILYID *)argv[0];
	T_ROOMID		RoomID			= *(T_ROOMID *)argv[1];
	uint32			visit_id		= *(uint32 *)argv[2];
	uint16			year			= *(uint16 *)argv[3];
	TServiceId		tsid(*(uint16 *)argv[4]);

	if ( !nQueryResult )
		return;

	// 단체행사인 경우 Join들의 정보를 삭제한다.
	{
		uint32 nRows;	resSet->serial(nRows);

		for(uint32 i = 0; i < nRows; i ++)
		{
			uint32		Id;

			resSet->serial(Id);

			MAKEQUERY_DeleteVisitData(strQ, visit_id, Id);
			DBCaller->execute(strQ);
		}
	}

	// Host의 정보를 삭제한다.
	{
		MAKEQUERY_DeleteVisitData(strQ, visit_id, 0);
		DBCaller->execute(strQ);
	}

	// 행사결과물에서 삭제한다.
	CRoomWorld *inManager = GetOpenRoomWorld(RoomID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL, RoomID=%d", RoomID);
		return;
	}
	inManager->m_ActionResults.deleteYishiResult(visit_id);
}

// Delete action memo([d:action id][d:action subid])
void	cb_M_CS_VISIT_DATA_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;
	uint16				year;
	uint32				action_subid, action_id;
	_msgin.serial(FID, year, action_id, action_subid);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	if(FID != inManager->m_MasterID)	// 방원주만 삭제할수 있다.
	{
		sipwarning(L"Can't Delete MultiActionMemo in CRoomWorld::SheildActionMemo, because you are not master.");
		return;
	}

	if(action_subid != 0)
	{
		MAKEQUERY_DeleteVisitData(strQ, action_id, action_subid);
		DBCaller->execute(strQ);
	}
	else
	{
		MAKEQUERY_GetVisitDataSubID(strQ, action_id);
		DBCaller->executeWithParam(strQ, DBCB_DBGetVisitDataSubID,
			"DDDWW",
			CB_,			FID,
			CB_,			inManager->m_RoomID,
			CB_,			action_id,
			CB_,			year,
			CB_,			_tsid.get());
	}
}

// Modify action memo([d:action id][d:action subid][b:secret][u:pray text])
void	cb_M_CS_VISIT_DATA_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint16		year;
	uint32		action_subid, action_id;
	uint8		secret, action_type;
	ucstring	pray = "";

	_msgin.serial(FID, year, action_id, action_subid, action_type, secret);
	NULLSAFE_SERIAL_WSTR(_msgin, pray, MAX_LEN_ROOM_PRAYTEXT, FID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;

	T_ROOMID roomid = inManager->m_RoomID;
	if (IsSystemRoom(roomid))
	{
		MAKEQUERY_ModifyVisitData_Aud(strQ, FID, action_id, action_subid, secret, pray, action_type);
		DBCaller->execute(strQ);
	}
	else
	{
		MAKEQUERY_ModifyVisitData(strQ, FID, action_id, action_subid, secret, pray);
		DBCaller->execute(strQ);
	}
}

static void	DBCB_DBGetVisitFIDInOneDay(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID		= *(T_FAMILYID *)argv[0];
	uint16			year	= *(uint16 *)argv[1];
	uint8			month	= *(uint8 *)argv[2];
	uint8			day		= *(uint8 *)argv[3];
	TServiceId		tsid(*(uint16 *)argv[4]);

	if(!nQueryResult)
		return;

	uint32	nRows;			resSet->serial(nRows);

	uint8	bStart = 1, bEnd = 0;
	int		count = 0;
	CMessage msgOut(M_SC_ROOM_VISIT_FID);
	msgOut.serial(FID, year, month, day, bStart);

	for ( uint32 i = 0; i < nRows; i ++)
	{
		if (count >= 100)
		{
			bEnd = 0;
			msgOut.serial(bEnd);
			CUnifiedNetwork::getInstance()->send(tsid, msgOut);
			count = 0;

			CMessage	msgtmp(M_SC_ROOM_VISIT_FID);
			bStart = 0;
			msgtmp.serial(FID, year, month, day, bStart);

			msgOut = msgtmp;
		}

		uint32		VisitFID;		resSet->serial(VisitFID);
		ucstring	VisitFName;		resSet->serial(VisitFName);
		uint16		wHasYishi;		resSet->serial(wHasYishi);

		uint8		HasYishi = (wHasYishi > 255) ? 255 : wHasYishi;

		msgOut.serial(VisitFID, VisitFName, HasYishi);
		count ++;
	}

	bEnd = 1;
	msgOut.serial(bEnd);
	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

// Request Room's Visit FID List For Autoplay 
// ([w:Year][b:Month][b:Day])
void cb_M_CS_GET_ROOM_VISIT_FID(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;	_msgin.serial(FID);
	uint16		year;	_msgin.serial(year);
	uint8		month;	_msgin.serial(month);
	uint8		day;	_msgin.serial(day);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	MAKEQUERY_GetRoomVisitFIDInOneDay(strQuery, inManager->m_RoomID, year, month, day);
	DBCaller->executeWithParam(strQuery, DBCB_DBGetVisitFIDInOneDay,
		"DWBBW",
		CB_,			FID,
		CB_,			year,
		CB_,			month,
		CB_,			day,
		CB_,			_tsid.get());
}

static void	DBCB_DBGetPapetList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			ret			= *(uint32*)argv[0];
	uint32			allcount	= *(uint32*)argv[1];
	uint32			yearcount	= *(uint32*)argv[2];
	T_FAMILYID		FID			= *(T_FAMILYID *)argv[3];
	T_ROOMID		RoomID		= *(T_ROOMID *)argv[4];
	uint16			year		= *(uint16 *)argv[5];
	uint8			bOnlyOwnData= *(uint8 *)argv[6];
	uint8			bSort		= *(uint8 *)argv[7];
	uint32			Page		= *(uint32 *)argv[8];
	TServiceId		tsid(*(uint16 *)argv[9]);

	if(!nQueryResult)
		return;
	if(ret != 0 && ret != 1010)
		return;

	CMessage msgOut(M_SC_PAPER_LIST);
	msgOut.serial(FID, RoomID, year, bOnlyOwnData, bSort, Page, allcount, yearcount);

	if(ret == 0)
	{
		uint32	nRows2;			resSet->serial(nRows2);
		for ( uint32 i = 0; i < nRows2; i ++)
		{
			uint32				LetterId;		resSet->serial(LetterId);
			T_COMMON_CONTENT	Title;			resSet->serial(Title);
			T_DATETIME			datetime;		resSet->serial(datetime);
			uint8				Opened;			resSet->serial(Opened);
			uint32				FID;			resSet->serial(FID);
			ucstring			FName;			resSet->serial(FName);
			uint32				ReadCount;		resSet->serial(ReadCount);

			msgOut.serial(LetterId, datetime, Title, Opened, FID, FName, ReadCount);
		}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);

	//sipdebug(L"CRoomWorld::GetLetterList success."); byy krs
}

// Request room paper list([d:Year][b:bOnlyOwnData][b:bSort][d:Page])
void	cb_M_CS_REQ_PAPER_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;
	uint16				year;
	uint32				page;
	uint8				bOnlyOwnData, bSort, pageSize;
	_msgin.serial(FID, year, bOnlyOwnData, bSort, page, pageSize);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	uint32	FID1 = FID;
	if(bOnlyOwnData == 0)
		FID1 = 0;

	MAKEQUERY_GetPaperList(strQ, inManager->m_RoomID, year, FID1, bSort, page, pageSize);
	DBCaller->executeWithParam(strQ, DBCB_DBGetPapetList,
		"DDDDDWBBDW",
		OUT_,		NULL,
		OUT_,		NULL,
		OUT_,		NULL,
		CB_,		FID,
		CB_,		inManager->m_RoomID,
		CB_,		year,
		CB_,		bOnlyOwnData,
		CB_,		bSort,
		CB_,		page,
		CB_,		_tsid.get());

	//inManager->GetLetterList(FID, year_month >> 16, (year_month >> 8) & 0xFF, _tsid);

}

static void	DBCB_DBGetPaperContent(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			FID				= *(T_FAMILYID *)argv[0];
	T_COMMON_ID			letter_id		= *(T_COMMON_ID *)argv[1];
	TServiceId			tsid(*(uint16 *)argv[2]);

	if ( ! nQueryResult )
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
		return;

	T_COMMON_CONTENT	content;	
	uint32				WriterFID;
	uint8				secret, del;
	uint32				read_count;
	resSet->serial(content, WriterFID, secret, del, read_count);

	uint32	err = ERR_NOERR;
	if(del != 0)
	{
		err = E_LETTER_DELETED;
		CMessage msg(M_SC_PAPER);
		msg.serial(FID, letter_id, err);
		CUnifiedNetwork::getInstance()->send(tsid, msg);
	}
	else
	{
		// secret 검사
		if(secret != 0)
		{
			CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
			if (inManager == NULL)
				return;
			if( (FID != WriterFID) && (FID != inManager->m_MasterID) && (!inManager->IsRoomManager(FID)))
			{
				// 볼 권한이 없는데 요청했다??? - 비법파케트발견
				sipwarning("You have no access authority. RoomID=%d, LetterID=%d, FID=%d", inManager->m_RoomID, letter_id, FID);
				return;
			}
		}
		CMessage msg(M_SC_PAPER);
		msg.serial(FID, letter_id, err, content, read_count);
		CUnifiedNetwork::getInstance()->send(tsid, msg);
	}
}

// 편지내용 얻기
// [d:Paper id]
void	cb_M_CS_REQ_PAPER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
	T_COMMON_ID			letter_id;		_msgin.serial(letter_id);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	MAKEQUERY_GetPaperContent(strQ, letter_id);
	DBCaller->executeWithParam(strQ, DBCB_DBGetPaperContent,
		"DDW",
		CB_,		FID,
		CB_,		letter_id,
		CB_,		_tsid.get());
}

static void	DBCB_DBInsertPaper(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			FID				= *(T_FAMILYID *)argv[0];
	T_COMMON_CONTENT	title			= (ucchar *)argv[1];
	T_COMMON_CONTENT	content			= (ucchar *)argv[2];
	uint8				opened			= *(uint8 *)argv[3];
	T_ROOMID			RoomID			= *(T_ROOMID *)argv[4];
	TServiceId			tsid(*(uint16 *)argv[5]);

	if ( ! nQueryResult )
		return;
	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
		return;

	sint32		letter_id;		resSet->serial(letter_id);
	T_DATETIME	datetime;		resSet->serial(datetime);

	if(letter_id == 0 || letter_id == -1)
	{
		sipwarning(L"DB_EXE_PARAMQUERY return error failed in CRoomWorld::AddNewLetter.");
		return;
	}

	CMessage msg(M_SC_NEW_PAPER);
	msg.serial(FID, RoomID, letter_id, datetime, title, content, opened);
	CUnifiedNetwork::getInstance()->send(tsid, msg);

	// For EXP
	CRoomWorld	*inManager = GetOpenRoomWorld(RoomID);
	if (inManager)
	{
		inManager->ChangeRoomExp(Room_Exp_Type_Letter, FID);
	}
	ChangeFamilyExp(Family_Exp_Type_Letter, FID);

	// 기록 보관
	INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning(L"GetFamilyInfo = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
	}
	else
	{
		pFamilyInfo->DonActInRoom(ROOMACT_XIEXIN);

		MAKEQUERY_AddFishFlowerVisitData(strQ, pFamilyInfo->m_nCurActionGroup, FF_LETTER, letter_id, 0);
		DBCaller->execute(strQ);
	}

	//sipdebug(L"CRoomWorld::AddNewLetter success."); byy krs
}

// 새편지 추가
// [u:Title][u:Content][b:opened]
void	cb_M_CS_NEW_PAPER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
	ucstring			title;			NULLSAFE_SERIAL_WSTR(_msgin, title, MAX_LEN_LETTER_TITLE, FID);
	ucstring			content;		NULLSAFE_SERIAL_WSTR(_msgin, content, MAX_LEN_LETTER_CONTENT, FID);
	uint8				opened;			_msgin.serial(opened);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	RoomChannelData*	pChannelData = inManager->GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. FID=%d", FID);
		return;
	}

	MAKEQUERY_InsertPaper(strQ, title, content, inManager->m_RoomID, opened, FID);
	DBCaller->executeWithParam(strQ, DBCB_DBInsertPaper,
		"DSSBDW",
		CB_,			FID,
		CB_,			title.c_str(),
		CB_,			content.c_str(),
		CB_,			opened,
		CB_,			inManager->m_RoomID,
		CB_,			_tsid.get());
}

// 편지 삭제
// [b:count][ [d:Paper id] ]
void	cb_M_CS_DEL_PAPER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
	uint8				count;			_msgin.serial(count);
	uint32				letter_id;	

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	// 자기가 쓴 편지 혹은 방주인 혹은 관리원만 삭제할수 있다.
	T_FAMILYID			FID1 = FID;
	if((FID == inManager->m_MasterID) || inManager->IsRoomManager(FID))
		FID1 = 0;

	//CMessage	msgout(M_SC_DEL_PAPER);
	//msgout.serial(FID);
	//msgout.serial(inManager->m_RoomID);
	//uint32 initialLength = msgout.length();

	for ( uint8 i = 0; i < count; i++ )
	{
		_msgin.serial(letter_id);

		// 현재 편집중이면 삭제할수 없다.
		{
			uint32	err = ERR_NOERR;
			VEC_LETTER_MODIFY_LIST::iterator it;
			for(it = inManager->m_LetterModifyList.begin(); it != inManager->m_LetterModifyList.end(); it ++)
			{
				if(it->LetterID == letter_id)
				{
					err = E_LETTER_MODIFYING;
					break;
				}
			}
			CMessage	msgOut(M_SC_DEL_PAPER);
			msgOut.serial(FID, letter_id, err);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

			if(err != ERR_NOERR)
				continue;
		}

		if ( inManager->DelLetter(FID1, letter_id) )
		{
			//msgout.serial(letter_id);
		}			
	}

	//if( msgout.length() > initialLength )
	//{
	//	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
	//}
}

// 편지 수정
// [d:Paper id][u:Title][u:Content][b:opened]
void	cb_M_CS_MODIFY_PAPER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
	uint32				letter_id;		_msgin.serial(letter_id);
	ucstring			title;			NULLSAFE_SERIAL_WSTR(_msgin, title, MAX_LEN_LETTER_TITLE, FID);
	ucstring			content;		NULLSAFE_SERIAL_WSTR(_msgin, content, MAX_LEN_LETTER_CONTENT, FID);
	uint8				opened;			_msgin.serial(opened);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->ModifyLetter(FID, letter_id, title, content, opened, _tsid);
}

void CRoomWorld::ModifyLetterStart(T_FAMILYID FID, uint32 letter_id)
{
	_Letter_Modify_Data	data;
	data.FID = FID;
	data.LetterID = letter_id;
	m_LetterModifyList.push_back(data);
}

// Modify paper Start ([d:Paper id])
void	cb_M_CS_MODIFY_PAPER_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
	uint32				letter_id;		_msgin.serial(letter_id);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->ModifyLetterStart(FID, letter_id);
}

void CRoomWorld::ModifyLetterEnd(T_FAMILYID FID, uint32 letter_id)
{
	VEC_LETTER_MODIFY_LIST::iterator it;
	for(it = m_LetterModifyList.begin(); it != m_LetterModifyList.end(); it ++)
	{
		if((it->FID == FID) && (it->LetterID == letter_id))
		{
			m_LetterModifyList.erase(it);
			break;
		}
	}
}

// Modify paper End ([d:Paper id])
void	cb_M_CS_MODIFY_PAPER_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
	uint32				letter_id;		_msgin.serial(letter_id);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->ModifyLetterEnd(FID, letter_id);
}

// 제상에 제물을 올린다
//void	cb_M_CS_NEWSACRIFICE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
//	uint8				num;			_msgin.serial(num);
//	if (num < 1)
//		return;
//	ROOMSACRIFICE		sacrifice[50];
//	for(uint8 i = 0; i < num; i++)
//	{
//		sacrifice[i].m_SacID = 0;
//		_msgin.serial(sacrifice[i].m_SubTypeId);	// 아이템ID
//		_msgin.serial(sacrifice[i].m_Num);			// 아이템개수
//		_msgin.serial(sacrifice[i].m_X);			// 위치X
//		_msgin.serial(sacrifice[i].m_Y);			// 위치Y
//	}
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->AddRoomSacrifice(FID, num, sacrifice, _tsid);
//}

// 방의 비석설정
void	cb_M_CS_TOMBSTONE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	T_TOMBID			tombid;				_msgin.serial(tombid);
	T_COMMON_CONTENT	tomb;				NULLSAFE_SERIAL_WSTR(_msgin, tomb, MAX_LEN_ROOM_TOMB, FID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->SetTomb(FID, tombid, tomb, _tsid);
}

// 방의 비석설정
void	cb_M_CS_RUYIBEI_TEXT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->SendRuyibeiText(FID, _tsid);
}

//// 방의 예약행사목록 얻기
//void	cb_M_CS_EVENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->GetOrderEvents(FID, _tsid);
//}

//// 행사예약
//void	cb_M_CS_ADDEVENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	T_ROOMEVENTTYPE		EventType;			_msgin.serial(EventType);		
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->AddEvent(FID, EventType, _tsid);
//}

//// 행사끝
//void	cb_M_CS_ENDEVENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->EndEvent(FID, _tsid);
//}

// 손에 든 아이템을 제상에 놓는다
//void	cb_M_CS_PUTHOLEDITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	T_CHARACTERID		CHID;				_msgin.serial(CHID);			// 캐릭터id
//	T_SACRIFICEX		X;					_msgin.serial(X);				// 위치X
//	T_SACRIFICEY		Y;					_msgin.serial(Y);				// 위치Y
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->PutHoldedItem(FID, CHID, X, Y, _tsid);
//}

//// 인벤 아이템을 제상에 놓는다
//void	cb_M_CS_PUTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	T_CHARACTERID		CHID;				_msgin.serial(CHID);			// 캐릭터id
//	T_FITEMID			ItemID;				_msgin.serial(ItemID);			// 인벤ID
//	T_FITEMNUM			ItemNum;			_msgin.serial(ItemNum);			// 개수
//	T_SACRIFICEX		X;					_msgin.serial(X);				// 위치X
//	T_SACRIFICEY		Y;					_msgin.serial(Y);				// 위치Y
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->PutItem(FID, CHID, ItemID, ItemNum, X, Y, _tsid);
//}

// Fish Change ([d:GoldFishScopeID][b:GoldFishIndex, 0~][d:StoreLockID][d:SyncCode][w:InvenPos])
void	cb_M_CS_GOLDFISH_CHANGE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;				_msgin.serial(FID);
	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	uint32		GoldFishScopeID;	_msgin.serial(GoldFishScopeID);
	uint8		GoldFishNumber;		_msgin.serial(GoldFishNumber);
	if (GoldFishNumber > MAX_GOLDFISH_COUNT || GoldFishNumber <= 0)
		return;

	for (uint8 i = 0; i < GoldFishNumber; i++)
	{
		uint8		GoldFishIndex;		_msgin.serial(GoldFishIndex);
		uint32		StoreLockID;		_msgin.serial(StoreLockID);
		uint32		SyncCode;			_msgin.serial(SyncCode);
		T_FITEMPOS	InvenPos;			_msgin.serial(InvenPos);
		inManager->GoldFishChange(FID, StoreLockID, InvenPos, GoldFishScopeID, GoldFishIndex, SyncCode, _tsid);
	}

	// DB보관 & 사용자통지
	inManager->ChangeGoldFish(GoldFishScopeID);

	// 경험값 변경
	inManager->ChangeFamilyExp(Family_Exp_Type_ChangeFish, FID);
	inManager->ChangeRoomExp(Room_Exp_Type_ChangeFish, FID);
}

void CRoomWorld::GoldFishChange(T_FAMILYID FID, uint32 StoreLockID, T_FITEMPOS InvenPos, uint32 GoldFishScopeID, uint8 GoldFishIndex, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	if (GoldFishIndex >= MAX_GOLDFISH_COUNT)
	{
		sipwarning("GoldFishIndex >= MAX_GOLDFISH_COUNT. GoldFishIndex=%d", GoldFishIndex);
		return;
	}

	// 금붕어가 없을 때에는 임의의 사람이 넣을수 있으며, 이미 있을 때 교체는 방주인 & 방관리자만 할수 있다. - 중요하지 않은 문제이므로 Client에 넘기고 써버에서는 검사하지 않는다.
	//if ((m_GoldFishData[GoldFishScopeID].m_GoldFishs[GoldFishIndex].GoldFishID != 0) && (GetDBTimeSecond() < m_GoldFishData[GoldFishScopeID].m_GoldFishs[GoldFishIndex].GoldFishDelTime))
	//{
	//	if ((FID != m_MasterID) && (!IsRoomManager(FID)))
	//	{
	//		sipwarning("Only master or manager can change GoldFish!. RoomID=%d, MasterFID=%d, FID=%d", m_RoomID, m_MasterID, FID);
	//		return;
	//	}
	//}

	uint32	ItemID = 0;
	const	MST_ITEM	*mstItem = NULL;
	uint32	UsedUMoney = 0, UsedGMoney = 0;
	if (StoreLockID != 0)
	{
		RoomStoreLockData*	lockData = GetStoreLockData(FID, StoreLockID);
		if (lockData == NULL)
		{
			sipwarning("GetStoreLockData = NULL. FID=%d, StoreLockID=%d", FID, StoreLockID);
			return;
		}
		if (lockData->ItemCount != 1)
		{
			sipwarning("lockData->ItemCount != 1. FID=%d, StoreLockID=%d, lockData->ItemCount=%d", FID, StoreLockID, lockData->ItemCount);
			return;
		}

		ItemID = lockData->ItemIDs[0];
		mstItem = GetMstItemInfo(ItemID);
		if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_GUANSHANGYU))
		{
			sipwarning(L"Invalid Item Type in Store. FID=%d, ItemID=%d", FID, ItemID);
			return;
		}

		// Check OK
		RoomStoreItemUsed(FID, StoreLockID);
	}
	else
	{
		if (!IsValidInvenPos(InvenPos))
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos! FID=%d, InvenPos=%d", FID, InvenPos);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}

		_InvenItems*	pInven = GetFamilyInven(FID);
		if (pInven == NULL)
		{
			sipwarning("GetFamilyInven NULL");
			return;
		}
		int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
		ITEM_INFO	&item = pInven->Items[InvenIndex];
		ItemID = item.ItemID;
 
		mstItem = GetMstItemInfo(ItemID);
		if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_GUANSHANGYU))
		{
			sipwarning(L"Invalid Item Type. FID=%d, ItemID=%d", FID, ItemID);
			return;
		}

		// Check OK
		if(item.MoneyType == MONEYTYPE_UMONEY)
			UsedUMoney = mstItem->UMoney;
		else
			UsedGMoney = mstItem->GMoney;

		// Delete from Inven
		if (1 < item.ItemCount)
		{
			item.ItemCount --;
		}
		else
		{
			item.ItemID = 0;
		}

		if (!SaveFamilyInven(FID, pInven))
		{
			sipwarning("SaveFamilyInven failed. FID=%d", FID);
			return;
		}
	}

	// 금붕어정보 변경
	m_GoldFishData[GoldFishScopeID].m_GoldFishs[GoldFishIndex].GoldFishID = ItemID;
	m_GoldFishData[GoldFishScopeID].m_GoldFishs[GoldFishIndex].GoldFishDelTime = (uint32)GetDBTimeSecond() + mstItem->UseTime * 60;	// mstItem->UseTime은 분단위이므로 초단위로 계산한다.
	m_GoldFishData[GoldFishScopeID].m_GoldFishs[GoldFishIndex].FID = FID;
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		m_GoldFishData[GoldFishScopeID].m_GoldFishs[GoldFishIndex].FName = L"";
	}
	else
	{
		m_GoldFishData[GoldFishScopeID].m_GoldFishs[GoldFishIndex].FName = pInfo->m_Name;
	}

	if ((UsedUMoney != 0) || (UsedGMoney != 0))
	{
		ucstring itemInfo = mstItem->Name;
		ChangeRoomExp(Room_Exp_Type_UseMoney, FID, UsedUMoney, UsedGMoney, itemInfo);
	}

	// 기록 보관
	{
		pInfo->DonActInRoom(ROOMACT_GUANSHANGYU);

		MAKEQUERY_AddFishFlowerVisitData(strQ, pInfo->m_nCurActionGroup, FF_GOLDFISH, ItemID, 0);
		DBCaller->execute(strQ);
	}

	uint32	ret = ERR_NOERR;
	CMessage	msgOut(M_SC_INVENSYNC);
	msgOut.serial(FID, SyncCode, ret);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);

	// Log
//	DBLOG_ItemUsed(DBLSUB_USEITEM_FISHFOOD, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

void CRoomWorld::ChangeGoldFish(uint32 GoldFishScopeID)
{
	// DB에 보관
	{
		ucchar str_buf[MAX_GOLDFISH_COUNT * 16 + 1];
		const uint32 strbuf2unit = (sizeof(uint32)+(MAX_LEN_FAMILY_NAME)*2)*2; // 40
		ucchar str_buf2[MAX_GOLDFISH_COUNT * strbuf2unit+1];		memset(str_buf2, 0, sizeof(str_buf2));
		for (uint32 i = 0; i < MAX_GOLDFISH_COUNT; i ++)
		{
			uint32	v = m_GoldFishData[GoldFishScopeID].m_GoldFishs[i].GoldFishID;
			smprintf((ucchar*)(&str_buf[i * 16 + 0]), 2, _t("%02x"), (v >> 0) & 0xFF);
			smprintf((ucchar*)(&str_buf[i * 16 + 2]), 2, _t("%02x"), (v >> 8) & 0xFF);
			smprintf((ucchar*)(&str_buf[i * 16 + 4]), 2, _t("%02x"), (v >> 16) & 0xFF);
			smprintf((ucchar*)(&str_buf[i * 16 + 6]), 2, _t("%02x"), (v >> 24) & 0xFF);
			v = m_GoldFishData[GoldFishScopeID].m_GoldFishs[i].GoldFishDelTime;
			smprintf((ucchar*)(&str_buf[i * 16 + 8]), 2, _t("%02x"), (v >> 0) & 0xFF);
			smprintf((ucchar*)(&str_buf[i * 16 + 10]), 2, _t("%02x"), (v >> 8) & 0xFF);
			smprintf((ucchar*)(&str_buf[i * 16 + 12]), 2, _t("%02x"), (v >> 16) & 0xFF);
			smprintf((ucchar*)(&str_buf[i * 16 + 14]), 2, _t("%02x"), (v >> 24) & 0xFF);

			v = m_GoldFishData[GoldFishScopeID].m_GoldFishs[i].FID;
			smprintf((ucchar*)(&str_buf2[i * strbuf2unit + 0]), 2, _t("%02x"), (v >> 0) & 0xFF);
			smprintf((ucchar*)(&str_buf2[i * strbuf2unit + 2]), 2, _t("%02x"), (v >> 8) & 0xFF);
			smprintf((ucchar*)(&str_buf2[i * strbuf2unit + 4]), 2, _t("%02x"), (v >> 16) & 0xFF);
			smprintf((ucchar*)(&str_buf2[i * strbuf2unit + 6]), 2, _t("%02x"), (v >> 24) & 0xFF);
			ucchar	namebuffer[MAX_LEN_FAMILY_NAME+1];	memset(namebuffer, 0, sizeof(namebuffer));
			wcscpy(namebuffer, m_GoldFishData[GoldFishScopeID].m_GoldFishs[i].FName.c_str());
			for (int j = 0; j < MAX_LEN_FAMILY_NAME; j ++)
			{
				v = namebuffer[j];
				smprintf((ucchar*)(&str_buf2[i * strbuf2unit + 8 + 4 * j]), 2, _t("%02x"), (v >> 0) & 0xFF);
				smprintf((ucchar*)(&str_buf2[i * strbuf2unit + 8 + 4 * j + 2]), 2, _t("%02x"), (v >> 8) & 0xFF);
			}
		}
		str_buf[MAX_GOLDFISH_COUNT * 16] = 0;
		str_buf2[MAX_GOLDFISH_COUNT * strbuf2unit] = 0;

		MAKEQUERY_SaveGoldFishData(strQ, m_RoomID, GoldFishScopeID, str_buf, str_buf2);
		DBCaller->execute(strQ);
	}

	// 방에 있는 전체 사용자들에게 통지
	{
		TTime	curTime = GetDBTimeSecond();
		uint32	RemainSecond[MAX_GOLDFISH_COUNT];
		for (int i = 0; i < MAX_GOLDFISH_COUNT; i ++)
			RemainSecond[i] = m_GoldFishData[GoldFishScopeID].m_GoldFishs[i].GoldFishDelTime - (uint32)curTime;
		FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_UPDATE_GOLDFISH)
			msgOut.serial(GoldFishScopeID);
			for (int i = 0; i < MAX_GOLDFISH_COUNT; i ++)
			{
				msgOut.serial(m_GoldFishData[GoldFishScopeID].m_GoldFishs[i].GoldFishID, RemainSecond[i],
					m_GoldFishData[GoldFishScopeID].m_GoldFishs[i].FID, m_GoldFishData[GoldFishScopeID].m_GoldFishs[i].FName);
			}
		FOR_END_FOR_FES_ALL()
	}
}

//// 페트를 새로 추가한다
//void	cb_M_CS_NEWPET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	T_PETID				PetID;				_msgin.serial(PetID);			// 페트id
//	T_PETCOUNT			PetNum;				_msgin.serial(PetNum);			// 페트개수
//	T_PETREGIONID		PetRegion;			_msgin.serial(PetRegion);		// 페트령역
//	T_PETPOS			PetPos;				_msgin.serial(PetPos);			// 페트위치
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->NewPet(FID, PetID, PetNum, PetRegion, PetPos, _tsid);
//}

//// 페트가꾸기
//void	cb_M_CS_CAREPET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족ID
//	T_PETREGIONID		RegionID;			_msgin.serial(RegionID);		// 령역ID
//	T_PETCAREID			CareID;				_msgin.serial(CareID);			// 가꾸기ID
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->CarePet(FID, RegionID, CareID, _tsid);
//}

// Request Luck
void	cb_M_CS_LUCK_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_LUCK_REQ(FID, _tsid);
}

extern uint32 nAlwaysLuck4ID;
bool IsEnableLv4LuckCountOfFID(T_FAMILYID FID, T_ROOMID RoomID)
{
	// Lucky4의 최대개수는 MAX_LUCK_LV4_COUNT를 넘을수 없다.
	uint32 LuckLv4CountToday = GetLuckLv4CountToday();
	if (LuckLv4CountToday >= MAX_LUCK_LV4_COUNT)
		return false;

	if (nAlwaysLuck4ID > 0)	// for dev
		return true;

	// Lucky4는 새벽5시까지 3차, 9시까지 6차, 그 이후에 4차가 나온다.
	uint32	curTime = (uint32) GetDBTimeSecond();
	uint32  curDayTime = curTime % SECOND_PERONEDAY;
	if (curDayTime < 5*3600)
	{
		if (LuckLv4CountToday >= 3)
			return false;
	}
	else if (curDayTime < 9*3600)
	{
		if (LuckLv4CountToday >= 6)
			return false;
	}
	// 한명의 사용자는 2일에 한번 Lucky4를 받을수 있다.
	TTime lastLv4TimeFID = GetLastLuck4TimeForFID(FID);
	if (curTime - lastLv4TimeFID < LUCKY4_INTERVAL_FAMILY)
		return false;

	// 한방에서는 2일에 한번 Lucky4가 나올수 있다.
	TTime lastLv4Time = GetLastLuck4TimeForRoom(RoomID);
	if (curTime - lastLv4Time < LUCKY4_INTERVAL_ROOM)
		return false;

	return true;
}

void CRoomWorld::on_M_CS_LUCK_REQ(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}
	FAMILY	*family = GetFamilyData(FID);
	if (family == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", FID);
		return;
	}
	if (IsSystemAccount(family->m_UserID))
	{
		sipwarning("impossible system user's luck request. FID=%d, UserID=%d", FID, family->m_UserID);
		return;
	}
	uint32	curTime = (uint32) GetDBTimeSecond();
	if (curTime - pInfo->m_LuckTime < (TM_SEND_LUCK_REQ_MIN_CYCLE - 30))	// 5분 - 30초여유
	{
		// 비법파케트발견
		ucstring ucUnlawContent = SIPBASE::_toString("Luck request interval too short. FID=%d, RoomID=%d, interval=%d", FID, m_RoomID, (uint32)(curTime - pInfo->m_LuckTime)).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	pInfo->m_LuckTime = curTime;

	RoomChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	uint32	LuckID = 0;
	OUT_LUCK	*pLuck = NULL;

	// 여러명등불올리기가 신청된 경우에는 행운이 발생하지 않도록 한다.
	// 밤12시를 기준으로 +-10분사이에는 행운을 정리하는것과 겹칠수 있기때문에 행운이 발생하지 않도록 한다.
	if (pChannelData->m_Activitys[ACTPOS_MULTILANTERN - 1].FID == 0)
	{
		uint32 curDayTime = curTime % SECOND_PERONEDAY;
		if (curDayTime > 600 && curDayTime < SECOND_PERONEDAY - 600)
			pLuck = GetLuck();
	}
	if (pLuck != NULL)
	{
		LuckID = pLuck->LuckID;
		if (pLuck->LuckLevel == 3)
		{
			// Luck Lv3 처리
			if ((m_LuckID_Lv3 == 0) && IsEnableLv3LuckCountOfFID(FID))
			{
				m_LuckID_Lv3 = LuckID;
				RegisterLv3_4Luck(m_RoomID, FID, pLuck->LuckLevel, LuckID, curTime);
				{
					// OpenRoom써비스들에 통지한다
					CMessage	msgNT(M_NT_LUCK);
					msgNT.serial(m_RoomID, m_RoomName, m_MasterID, m_MasterName, FID, family->m_Name, pLuck->LuckLevel, LuckID);
					CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgNT);
					CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgNT);
				}
			}
			else
				LuckID = 0;
		}
		else if (pLuck->LuckLevel == 4)
		{
			// Luck Lv4 처리
			if ((m_LuckID_Lv4 == 0) && 
				(m_PowerID == ROOMOPEN_TOALL) &&
				!IsSystemAccount(m_MasterUserID) &&
				IsEnableLv4LuckCountOfFID(FID, m_RoomID) &&
				pInfo->IsDoneAnyAct())
			{
				// m_LuckID_Lv4 = LuckID;
				// m_LuckID_Lv4_ShowTime = curTime;
				CMessage	msgNT(M_NT_LUCK);
				msgNT.serial(m_RoomID, m_RoomName, m_MasterID, m_MasterName, FID, family->m_Name, pLuck->LuckLevel, LuckID);
				CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgNT);
			}
			else
				LuckID = 0;
		}
	}

	if (LuckID == 0)
	{
		/*
		if (!IsSystemAccount(m_MasterUserID) && m_LuckID_Lv4 == 0)
		{
			uint32 LuckLv4CountToday = GetLuckLv4CountToday();
			if (LuckLv4CountToday < MAX_LUCK_LV4_COUNT)
			{
				CMessage	msgOut(M_NT_LUCK4_REQ);
				msgOut.serial(FID, family->m_Name, m_RoomID);
				CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
				return;
			}
		}
		*/
		uint32		zero = 0;
		CMessage	msgOut(M_SC_LUCK);
		msgOut.serial(FID, zero, FID, pInfo->m_Name, LuckID, m_RoomID, m_RoomName, m_SceneID);
		msgOut.serial(m_MasterID, m_MasterName);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		return;
	}



	//// Prize처리
	//if (pLuck->LuckLevel == 4)
	//{
	//	CheckPrize(FID, PRIZE_TYPE_LUCK, 0);
	//}

	switch (pLuck->LuckLevel)
	{
	case 1:	// 일반통보
	case 2: // 지역통보
		{
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LUCK)
				msgOut.serial(FID, pInfo->m_Name, LuckID, m_RoomID, m_RoomName, m_SceneID, m_MasterID, m_MasterName);
			FOR_END_FOR_FES_ALL()
		}
		break;
	case 3: // 전체통보
		{
			CMessage	msgOut(M_SC_LUCK_TOALL);
			msgOut.serial(FID, pInfo->m_Name, LuckID, m_RoomID, m_RoomName, m_SceneID, m_MasterID, m_MasterName);
			CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
		}
		// 기록남기기
		{
			MAKEQUERY_AddFishFlowerVisitData(strQ, pInfo->m_nCurActionGroup, FF_LUCK, LuckID, 0);
			DBCaller->execute(strQ);
		}
		// 경험값증가
		{
			ChangeFamilyExp(Family_Exp_Type_ShenMiDongWu, FID);
		}
		break;
	}
}

void	CRoomWorld::cb_M_NT_LUCK4_CONFIRM(bool bPlay, T_ROOMID RoomID, T_FAMILYID FID, T_FAMILYNAME FName, uint32 LuckID)
{
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (!bPlay)
	{
		if (pInfo)
		{
			uint32		zero = 0;
			uint32		LuckID = 0;
			CMessage	msgOut(M_SC_LUCK);
			msgOut.serial(FID, zero, FID, pInfo->m_Name, LuckID, m_RoomID, m_RoomName, m_SceneID);
			msgOut.serial(m_MasterID, m_MasterName);
			CUnifiedNetwork::getInstance()->send(pInfo->m_ServiceID, msgOut);
		}
		return;
	}
	TTime curTime = GetDBTimeSecond();
	m_LuckID_Lv4 = LuckID;
	m_LuckID_Lv4_ShowTime = curTime;
	uint8 LuckLevel = 4;
	RegisterLv3_4Luck(m_RoomID, FID, LuckLevel, LuckID, curTime);

	CheckPrize(FID, PRIZE_TYPE_LUCK, 0);

	CMessage	msgNT(M_NT_LUCK);
	msgNT.serial(m_RoomID, m_RoomName, m_MasterID, m_MasterName, FID, FName, LuckLevel, LuckID);
	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgNT);

	CMessage	msgOut(M_SC_LUCK_TOALL);
	msgOut.serial(FID, FName, LuckID, m_RoomID, m_RoomName, m_SceneID, m_MasterID, m_MasterName);
	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);

	if (pInfo)	// 기록남기기
	{
		MAKEQUERY_AddFishFlowerVisitData(strQ, pInfo->m_nCurActionGroup, FF_LUCK, LuckID, 0);
		DBCaller->execute(strQ);
	}
	else
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
	}
	// 경험값증가
	ChangeFamilyExp(Family_Exp_Type_Shenming, FID);
}

//void	CRoomWorld::cb_M_NT_LUCK4_RES(bool bPlay, T_FAMILYID FID, T_FAMILYNAME	FName)
//{
//	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
//	if (!bPlay)
//	{
//		if (pInfo)
//		{
//			uint32		zero = 0;
//			uint32		LuckID = 0;
//			CMessage	msgOut(M_SC_LUCK);
//			msgOut.serial(FID, zero, FID, pInfo->m_Name, LuckID, m_RoomID, m_RoomName, m_SceneID);
//			msgOut.serial(m_MasterID, m_MasterName);
//			CUnifiedNetwork::getInstance()->send(pInfo->m_ServiceID, msgOut);
//		}
//		return;
//	}
//
//	OUT_LUCK	*pLuck = GetLuck4();
//	if (pLuck == NULL)
//	{
//		sipwarning("There is no luck4");
//		return;
//	}
//
//	TTime curTime = GetDBTimeSecond();
//	m_LuckID_Lv4 = pLuck->LuckID;
//	m_LuckID_Lv4_ShowTime = curTime;
//
//	RegisterLv3_4Luck(m_RoomID, FID, pLuck->LuckLevel, pLuck->LuckID, curTime);
//
//	CheckPrize(FID, PRIZE_TYPE_LUCK, 0);
//
//	CMessage	msgNT(M_NT_LUCK);
//	msgNT.serial(m_RoomID, m_RoomName, m_MasterID, m_MasterName, FID, FName, pLuck->LuckLevel, pLuck->LuckID);
//	CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgNT);
//	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgNT);
//
//	CMessage	msgOut(M_SC_LUCK_TOALL);
//	msgOut.serial(FID, FName, pLuck->LuckID, m_RoomID, m_RoomName, m_SceneID, m_MasterID, m_MasterName);
//	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
//
//	if (pInfo)	// 기록남기기
//	{
//		MAKEQUERY_AddFishFlowerVisitData(strQ, pInfo->m_nCurActionGroup, FF_LUCK, pLuck->LuckID, 0);
//		DBCaller->execute(strQ);
//	}
//	else
//	{
//		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
//	}
//}

void	cb_M_NT_LUCK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_ROOMID		RoomID;		_msgin.serial(RoomID);
	ucstring		RoomName;	_msgin.serial(RoomName);
	T_FAMILYID		MasterFID;	_msgin.serial(MasterFID);
	ucstring		MasterFName;_msgin.serial(MasterFName);
	T_FAMILYID		FID;		_msgin.serial(FID);
	ucstring		FName;		_msgin.serial(FName);
	uint8			Level;		_msgin.serial(Level);
	uint32			LuckID;		_msgin.serial(LuckID);

	RegisterLv3_4Luck(RoomID, FID, Level, LuckID, GetDBTimeSecond());
}

void	cb_M_NT_LUCKHIS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	TTime curTime = GetDBTimeSecond();
	
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_ROOMID		RoomID;		_msgin.serial(RoomID);
	uint8			LuckLevel;	_msgin.serial(LuckLevel);
	uint32			LuckID;		_msgin.serial(LuckID);
	TTime			LuckDBTime;	_msgin.serial(LuckDBTime);
	
	RegisterLv3_4Luck(RoomID, FID, LuckLevel, LuckID, LuckDBTime);
}

void	cb_M_NT_LUCK4_CONFIRM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	bool			bPlay;		
	T_ROOMID		RoomID;
	T_FAMILYID		FID;
	T_FAMILYNAME	FName;
	uint32			LuckID;
	_msgin.serial(bPlay);
	_msgin.serial(RoomID);
	_msgin.serial(FID);
	_msgin.serial(FName);
	_msgin.serial(LuckID);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	roomManager->cb_M_NT_LUCK4_CONFIRM(bPlay, RoomID, FID, FName, LuckID);
}

//void	cb_M_NT_LUCK4_RES(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	bool			bPlay;		_msgin.serial(bPlay);
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	T_FAMILYNAME	FName;		_msgin.serial(FName);
//	T_ROOMID		RoomID;		_msgin.serial(RoomID);
//
//	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
//	if (roomManager == NULL)
//	{
//		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
//	}
//	roomManager->cb_M_NT_LUCK4_RES(bPlay, FID, FName);
//}

// 방에서 내보내기
//void	cb_M_CS_FORCEOUT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	T_ROOMID			RoomID;				_msgin.serial(RoomID);			
//	T_FAMILYID			ObjetcID;			_msgin.serial(ObjetcID);		// 대상가족id
//
//	CRoomWorld *inManager = GetOpenRoomWorld(RoomID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->ForceOut(FID, ObjetcID, _tsid);
//}

// 방에 들어온 가족이 창조한 방목록 보기
void	cb_M_CS_ROOMFORFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	T_FAMILYID			ObjectID;			_msgin.serial(ObjectID);		// 대상가족id

	//CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	//if (inManager == NULL)
	//	return;			// 비법발견

	MAKEQUERY_GetFamilyCreateRooms(strQuery, ObjectID);
	DBCaller->executeWithParam(strQuery, DBCB_DBFamilyCreateRooms,
		"DDW",
		CB_,			FID,
		CB_,			ObjectID,
		CB_,			_tsid.get());
}

// 방에 들어온 가족의 설명얻기
//void	cb_M_CS_ROOMFAMILYCOMMENT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	T_FAMILYID			ObjetcID;			_msgin.serial(ObjetcID);		// 대상가족id
//
//	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->GetFamilyComment(FID, ObjetcID, _tsid);
//}

//void	cb_M_CS_GETALLINROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID	FID;				_msgin.serial(FID);
//	T_ROOMID	RoomID;				_msgin.serial(RoomID);
//
//	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
//	if (roomManager == NULL)
//	{
//		CMessage	mesOut(M_SC_GETALLINROOM);
//		mesOut.serial(FID, RoomID);
//		CUnifiedNetwork::getInstance ()->send(_tsid, mesOut);
//	}
//	else
//	{
//		roomManager->SendAllFamilyInRoom(FID, _tsid);
//	}
//}

static void	DBCB_DBRenewRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID				= *(T_FAMILYID *)argv[0];
	T_ROOMID		RoomID			= *(T_ROOMID *)argv[1];
	uint16			OldSceneID		= *(uint16 *)argv[2];
	uint32			uDays			= *(uint32 *)argv[3];
	uint32			UPrice			= *(uint32 *)argv[4];
	TServiceId		tsid(*(uint16 *)argv[5]);

	if (!nQueryResult)
	{
		sipwarning("Shrd_room_renew: execute failed!");
		return;	// E_DBERR
	}

	uint32		nRows;			resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("nRows != 1. Rows=%d", nRows);
		return;
	}

	uint32	uResult = E_COMMON_FAILED;
	sint32	ret;	resSet->serial(ret);
	string termtime;

	switch (ret)
	{
	case 0:
		uResult = ERR_NOERR;
		resSet->serial(termtime);
		break;
	case 1003:
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("Cannot renew Room, Improper RoomID , room ID:%u", RoomID).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
	case 1061:
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("Cannot renew Room, because room is deleted, room ID:%u", RoomID).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
	default:
		sipwarning(L"Unknown Error Query %u", ret);
		break;
	}

	// 방관리자에게 통보
	{
		CMessage	msgout(M_SC_RENEWROOM);
		msgout.serial(FID, uResult, RoomID);
		//			if (uResult == ERR_NOERR)
		//				msgout.serial(termtime);

		CUnifiedNetwork::getInstance()->send(tsid, msgout);
		//sipinfo("SUCCESS user(id:%u) Renew room(id:%u)", FID, RoomID); byy krs
	}

	if (uResult == ERR_NOERR)
	{
		{
			// 현재 Open되여있는 경우 방정보를 수정한다.
			CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
			if (roomManager)
				roomManager->SendRoomTermTimeToAll(termtime);
			else
			{
				CMessage	msgOut(M_NT_ROOM_RENEW);
				msgOut.serial(RoomID, termtime);
				CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgOut);
			}
		}

		// Notify LS to subtract the user's money by sending message
		ExpendMoney(FID, UPrice, 0, 1, DBLOG_MAINTYPE_BUYROOM, DBLSUB_BUYROOM_RENEW,
			0, OldSceneID, uDays, UPrice, RoomID);

		// 2020/04/15 Add point(=UPrice) when renew room
		ExpendMoney(FID, 0, UPrice, 2, DBLOG_MAINTYPE_BUYROOM, DBLSUB_BUYROOM_RENEW, 0, OldSceneID);

		// 자금변화정형통보
		INFO_FAMILYINROOM	*pFamilyInfo = GetFamilyInfo(FID);
		if (pFamilyInfo != NULL)
		{
			SendFamilyProperty_Money(tsid, FID, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);
		}

		// Log
		DBLOG_RenewRoom(FID, RoomID, OldSceneID, L"", uDays, UPrice, 0);
	}
}

static void	DBCB_DBGetRoomRenewHistoryForRenew(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint8			Flag			= *(uint8 *)argv[0];
	uint32			_OldSceneID		= *(uint32 *)argv[1];
	sint32			ret				= *(sint32 *)argv[2];
	T_FAMILYID		FID				= *(T_FAMILYID *)argv[3];
	T_ROOMID		RoomID			= *(T_ROOMID *)argv[4];
	uint16			PriceID			= *(uint16 *)argv[5];
	TServiceId		tsid(*(uint16 *)argv[6]);

	uint16			OldSceneID		= (uint16)_OldSceneID;

	if (!nQueryResult || ret == -1)
	{
		sipwarning("Shrd_Room_Renew_History_List: execute failed!");
		return;	// E_DBERR
	}

	// get family info
	INFO_FAMILYINROOM	*pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return ;
	}

	// Calc Price
	uint32	curtime_min = (uint32)(GetDBTimeSecond() / 60);

	if (Flag == 2)
	{
		// 무한방은 연장할수 없다.
		ucstring ucUnlawContent = SIPBASE::_toString("Cannot renew room because unlimited. FID=%d, RoomID=%d", FID, RoomID).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	MST_ROOM	*room = GetMstRoomData(PriceID);
	if (room == NULL)
	{
		// 비법파케트발견
		ucstring	ucUnlawContent = SIPBASE::_toString("GetMstRoomData = NULL. PriceID=%d", PriceID).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	if (room->m_Price == 0)
	{
		// 비법파케트발견
		ucstring	ucUnlawContent = SIPBASE::_toString("room->m_Price = 0. PriceID=%d", PriceID).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	uint32		uDays = room->m_Years * 365 + room->m_Months * 30 + room->m_Days;
	T_PRICE		UPrice = 0;

	if (uDays == 0)
	{
		// 무한방으로의 연장인 경우 가격계산
		uint32		nRows;			resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint32			rhis_id;		resSet->serial(rhis_id);
			T_DATETIME		begintime;		resSet->serial(begintime);
			T_DATETIME		endtime;		resSet->serial(endtime);
			uint32			price;			resSet->serial(price);
			uint8			years;			resSet->serial(years);
			uint8			months;			resSet->serial(months);
			uint8			days;			resSet->serial(days);
			uint16			sceneid;		resSet->serial(sceneid);

			MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID((uint16)sceneid, years, months, days);
			if (mst_room_old == NULL)
			{
				sipwarning("GetMstRoomData_FromSceneID failed! - Master Data Error (%d,%d,%d,%d)", sceneid, years, months, days);
				return; // E_MASTERDATA
			}

			uint32	begintime_min = getMinutesSince1970(begintime);
			uint32	endtime_min = getMinutesSince1970(endtime);
			uint32 uAllMinutes = endtime_min - begintime_min;
			if (endtime_min <= curtime_min)
			{
				sipassert(false);
				continue;
			}
			if (begintime_min < curtime_min)
				begintime_min = curtime_min;

			UPrice += (uint32)(1.0 * (endtime_min - begintime_min) * mst_room_old->m_Price / uAllMinutes);
		}

		UPrice = (room->m_Price - UPrice) * room->m_DiscPercent / 100;
	}
	else
	{
		UPrice = room->GetRealPrice();
	}
	if (!CheckMoneySufficient(FID, UPrice, 0, MONEYTYPE_UMONEY))
		return;

	uint32	addhisspace = 0;
	if (Flag == 1)
	{
		// 원래 공짜방인 경우 HisSpace를 추가해주어야 한다.
		MST_ROOM	*mst_room_old	= GetMstRoomData_FromSceneID_FreeRoom(OldSceneID);
		if (mst_room_old == NULL)
		{
			sipwarning("GetMstRoomData_FromSceneID_FreeRoom = NULL. SceneID=%d", OldSceneID);
			return;
		}
		addhisspace = room->m_HisSpace - mst_room_old->m_HisSpace;
	}

	// 방기한연장
	{
		MAKEQUERY_RenewRoom(strQuery, FID, RoomID, OldSceneID, room->m_Years, room->m_Months, room->m_Days, room->m_Price, addhisspace);
		DBCaller->executeWithParam(strQuery, DBCB_DBRenewRoom,
			"DDWDDW",
			CB_,	FID,
			CB_,	RoomID,
			CB_,	OldSceneID,
			CB_,	uDays,
			CB_,	UPrice,
			CB_,	tsid.get());
	}
}

void cb_M_NT_ROOM_RENEW(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_ROOMID	RoomID;		_msgin.serial(RoomID);
	string		termtime;	_msgin.serial(termtime);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager)
		roomManager->SendRoomTermTimeToAll(termtime);
}

/****************************************************************************
* 함수:   M_CS_RENEWROOM
*         방사용기간을 연장한다
****************************************************************************/
// 방기한연장 ([d:Room id][w:Room price id])
void	cb_M_CS_RENEWROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		idMaster;		_msgin.serial(idMaster);
	T_ROOMID		idRoom;			_msgin.serial(idRoom);
	T_MSTROOM_ID	idRoomTypeByPrice;		_msgin.serial(idRoomTypeByPrice);

	if (!IsThisShardFID(idMaster))
	{
		sipwarning("IsThisShardFID = false. FID=%d", idMaster);
		return;
	}

	MAKEQUERY_GetRoomRenewHistory(strQ, idRoom);
	DBCaller->executeWithParam(strQ, DBCB_DBGetRoomRenewHistoryForRenew, 
		"BDDDDWW",
		OUT_,	NULL,
		OUT_,	NULL,
		OUT_,	NULL,
		CB_,	idMaster,
		CB_,	idRoom,
		CB_,	idRoomTypeByPrice,
		CB_,	_tsid.get());
}

// 천원카드에 의한 방기한연장 ([s:CardID][s:CardPWD][d:RoomID])
//void	cb_M_CS_ROOMCARD_RENEW(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	//uint32		FID, RoomID;
//	//string		CardID, CardPwd;
//	//_msgin.serial(FID, CardID, CardPwd, RoomID);
//
//	//if(CardID.empty() || CardID.length() > MAX_CARD_ID_LEN)
//	//{
//	//	sipwarning("Invalid CardID. CardID=%s", CardID.c_str());
//	//	return;
//	//}
//	//if(CardPwd.empty() || CardPwd.length() > MAX_CARD_PWD_LEN)
//	//{
//	//	sipwarning("Invalid CardID. CardPwd=%s", CardPwd.c_str());
//	//	return;
//	//}
//
//	//uint8	flag = 2;	// RenewRoom
//	//CMessage	msgOut(M_SM_ROOMCARD_CHECK);
//	//uint16	sid = SIPNET::IService::getInstance()->getServiceId().get();
//	//msgOut.serial(FID, flag, RoomID, CardID, CardPwd, sid);
//	//CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
//}

//void	cb_M_GIVEROOMRESULT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_ROOMID	RoomID;		_msgin.serial(RoomID);
//	T_FAMILYID	RecvID;		_msgin.serial(RecvID);
//	bool		bRecv;		_msgin.serial(bRecv);
//
//	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
//	if (roomManager)
//		roomManager->GiveRoomResult(RecvID, bRecv);
//}

void	cb_M_GMCS_PULLFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			GMID;				_msgin.serial(GMID);			// GMid
	T_FAMILYID			FID;				_msgin.serial(FID);				// 대상id
	T_CH_DIRECTION		Dir;				_msgin.serial(Dir);
	T_CH_X				X;					_msgin.serial(X);
	T_CH_Y				Y;					_msgin.serial(Y);
	T_CH_Z				Z;					_msgin.serial(Z);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(GMID);
	if (inManager != NULL)
	{
		inManager->PullFamily(GMID, FID, Dir, X, Y, Z);
	}
}

void DBCB_DBGetDeceasedForReloadInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ROOMID	RoomID	= *(T_ROOMID *)argv[0];

	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		T_ROOMNAME		roomname;				resSet->serial(roomname);
		uint8			surnamelen1;			resSet->serial(surnamelen1);
		T_DECEASED_NAME deadname1;				resSet->serial(deadname1);
		T_SEX			sex1;					resSet->serial(sex1);
		uint32			birthday1;				resSet->serial(birthday1);
		uint32			deadday1;				resSet->serial(deadday1);
		T_COMMON_CONTENT his1;					resSet->serial(his1);
		T_DATETIME		updatetime1;			resSet->serial(updatetime1);
		ucstring		domicile1;				resSet->serial(domicile1);
		ucstring		nation1;				resSet->serial(nation1);
		uint32			photoid1;				resSet->serial(photoid1);
		uint8			phototype;				resSet->serial(phototype);
		uint8			surnamelen2;			resSet->serial(surnamelen2);
		T_DECEASED_NAME deadname2;				resSet->serial(deadname2);
		T_SEX			sex2;					resSet->serial(sex2);
		uint32			birthday2;				resSet->serial(birthday2);
		uint32			deadday2;				resSet->serial(deadday2);
		T_COMMON_CONTENT his2;					resSet->serial(his2);
		T_DATETIME		updatetime2;			resSet->serial(updatetime2);
		ucstring		domicile2;				resSet->serial(domicile2);
		ucstring		nation2;				resSet->serial(nation2);
		uint32			photoid2;				resSet->serial(photoid2);

		CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
		if (roomManager == NULL)
		{
			sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
		}
		else
		{
			roomManager->m_Deceased1.m_Nation = nation1;
			roomManager->m_Deceased1.m_Domicile = domicile1;
			roomManager->m_Deceased1.m_His = his1;
			roomManager->m_Deceased2.m_Nation = nation2;
			roomManager->m_Deceased2.m_Domicile = domicile2;
			roomManager->m_Deceased2.m_His = his2;
		}
	}
}

// Notice that reload room info because of Auditing Error ([RoomID])
void cb_M_NT_RELOAD_ROOMINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_ROOMID	RoomID;		_msgin.serial(RoomID);

	MAKEQUERY_GetDeceasedForRoom(strQ, RoomID);
	DBCaller->executeWithParam(strQ, DBCB_DBGetDeceasedForReloadInfo,
		"D",
		CB_,			RoomID);
}

void	cb_M_GMCS_LISTCHANNEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	T_ROOMID		RoomID;		_msgin.serial(RoomID);

	CRoomWorld *room = GetOpenRoomWorld(RoomID);
	if(room != NULL)
	{
		CMessage mes(M_GMSC_LISTCHANNEL);
		mes.serial(GMID, RoomID);
		uint32	uNum = room->m_mapChannels.size();
		mes.serial(uNum);
		TmapRoomChannelData::iterator	itChannel;
		for(itChannel = room->m_mapChannels.begin(); itChannel != room->m_mapChannels.end(); itChannel ++)
		{
			T_ROOMCHANNELID	uChannelID	= itChannel->first;
			mes.serial(uChannelID);
		}
		CUnifiedNetwork::getInstance()->send(_tsid, mes);
	}
	else
	{
		uint32 uNum = 0;
		CMessage mes(M_GMSC_LISTCHANNEL);
		mes.serial(GMID, RoomID, uNum);
		CUnifiedNetwork::getInstance()->send(_tsid, mes);
	}
}

void	cb_M_CS_PROMOTION_ROOM_Oroom(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_ROOMID		RoomID;		_msgin.serial(RoomID);
	uint16			SceneID;	_msgin.serial(SceneID);
	uint16			PriceID;	_msgin.serial(PriceID);
	string			CardID;		_msgin.serial(CardID);
	uint16			LobbySID;	_msgin.serial(LobbySID);
	uint16			FsSID;		_msgin.serial(FsSID);

	CRoomWorld *room = GetOpenRoomWorld(RoomID);
	if(room != NULL)
	{
		if(room->IsAnyFamilyOnRoomlife())
		{
			uint32	result = E_NOEMPTY;
			CMessage	msgout(M_SC_PROMOTION_ROOM);
			msgout.serial(FID, result, RoomID);

			TServiceId	fs(FsSID);
			CUnifiedNetwork::getInstance()->send(fs, msgout);
			return;
		}

		// 꽃에 물주기를 한 사용자들의 목록을 보관한다.
		g_mapFamilyFlowerWaterTimeOfRoomForPromotion[RoomID] = room->m_mapFamilyFlowerWaterTime;

		DeleteRoomWorld(RoomID);
		//sipdebug("Room Deleted. RoomID=%d", RoomID); byy krs
	}
	else
		sipdebug("Can't find Room Object. RoomID=%d", RoomID);

	CMessage	msgout(M_CS_PROMOTION_ROOM);
	msgout.serial(FID, RoomID, SceneID, PriceID, CardID);

	TServiceId	Lobby(LobbySID);
	CUnifiedNetwork::getInstance()->send(Lobby, msgout);
}

// Require Activity ([b:ActPos])
static void cb_M_CS_ACT_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		ActPos;		_msgin.serial(ActPos);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_CS_ACT_REQ(FID, ActPos, _tsid);
	}
}

//by krs
#define CHECK_JISI_YISHIANDTOUXIANG_DOUBLE(act1, act2, errco) \
	if (ActPos == act1 && pChannelData->m_Activitys[act2-1].FID != NOFAMILYID)	\
{	\
	uint32		ErrorCode = errco;	\
	uint32		RemainCount = 0;	\
	uint8		DeskNo = 0;	\
	CMessage	msgOut(M_SC_ACT_WAIT);	\
	msgOut.serial(FID, ActPos, ErrorCode);	\
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);	\
	return;	\
}	

// 활동시작준비
void CRoomWorld::on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	uint32	curTime = (uint32)GetDBTimeSecond();

	int		index = (int)ActPos - 1;
	if ((ActPos <= 0) || (ActPos >= COUNT_ACTPOS))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"ActRequire - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	uint8		ActPos0 = ActPos; // 인사드리기를 위해서 필요하다.

	uint32	ChannelID = GetFamilyChannelID(FID);

	//if ((FID >= AUTOPLAY_FID_START) && (FID < AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT)) // by krs
	//	ChannelID = 5001;

	// 1명의 User는 하나의 활동만 할수 있다.
	int		ret = GetActivitingIndexOfUser(ChannelID, FID, false);
	if (ret >= 0)
	{
		// 비법발견
		//ucstring ucUnlawContent;
		//ucUnlawContent = SIPBASE::_toString(L"User require Two activities!: FID=%d", FID);
		//RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		sipwarning("GetActivitingIndexOfUser(ChannelID, FID) >= 0. FID=%d, RoomID=%d, ChannelID=%d, ActPos=%d, ret=%d", FID, m_RoomID, ChannelID, ActPos, ret);
		return;
	}
	ret = GetWaitingActivity(ChannelID, FID);
	if (ret >= 0)
	{
		// 비법발견
		//ucstring ucUnlawContent;
		//ucUnlawContent = SIPBASE::_toString(L"User require Two activities!: FID=%d", FID);
		//RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		sipwarning("GetWaitingActivity(ChannelID, FID) >= 0, =%d, RoomID=%d, ChannelID=%d, FID=%d", ret, m_RoomID, ChannelID, FID);
		return;
	}

	RoomChannelData	*pChannelData = GetChannel(ChannelID);

	int		realindex = -1;
	uint32	RemainCount = 0;

	if ((ActPos >= ACTPOS_WRITEMEMO_START) && (ActPos < ACTPOS_WRITEMEMO_START + MAX_MEMO_DESKS_COUNT))
	{
		// 방문록쓰기
		if (ActPos != ACTPOS_WRITEMEMO_START)
		{
			sipwarning("ActPos != ACTPOS_WRITEMEMO_START!!!! ActPos=%d", ActPos);
			return;
		}

		//// 한사람은 하루에 한 기념관에서 10개까지의 방문록을 남길수 있다.
		//uint32	today = GetDBTimeDays();
		//if (today != m_LiuyanDay)
		//{
		//	m_LiuyanDay = today;
		//	m_mapFamilyLiuyanTime.clear();
		//}
		//TmapDwordToDword::iterator it = m_mapFamilyLiuyanTime.find(FID);
		//if (it != m_mapFamilyLiuyanTime.end())
		//{
		//	if (it->second >= MAX_ROOM_MEMORIAL_COUNT)
		//	{
		//		CMessage	msgOut(M_SC_ACT_WAIT);
		//		uint32		ErrorCode = E_LIUYAN_WAIT;
		//		msgOut.serial(FID, ActPos, ErrorCode);
		//		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		//		return;
		//	}
		//}

		const OUT_ROOMKIND	*outRoomInfo = GetOutRoomKind(m_SceneID);
		if (outRoomInfo == NULL)
		{
			sipwarning("GetOutRoomKind = NULL! m_SceneID=%d", m_SceneID);
			return;
		}
		for (uint32 i = ACTPOS_WRITEMEMO_START - 1; i < ACTPOS_WRITEMEMO_START - 1 + outRoomInfo->LiuYanDesks; i ++)
		{
			_RoomActivity	&Activity = pChannelData->m_Activitys[i];
			if (Activity.FID == 0)
			{
				realindex = i;
				break;
			}
		}
		if (realindex < 0)
		{
			// 대기렬은 ACTPOS_WRITEMEMO_START 에만 있다.
			RemainCount += (1 + pChannelData->m_Activitys[ACTPOS_WRITEMEMO_START - 1].WaitFIDs.size());
		}
	}
	else if ((ActPos >= ACTPOS_BLESS_LAMP) && (ActPos < ACTPOS_BLESS_LAMP + MAX_BLESS_LAMP_COUNT))
	{
		// 길상등드리기
		if (ActPos != ACTPOS_BLESS_LAMP)
		{
			sipwarning("ActPos != ACTPOS_BLESS_LAMP!!!! ActPos=%d", ActPos);
			return;
		}

		// 한사람은 하루에 한 기념관에서 1개까지의 길상등을 남길수 있다.
		uint32	today = GetDBTimeDays();
		if (today != m_BlesslampDay)
		{
			m_BlesslampDay = today;
			m_mapFamilyBlesslampTime.clear();
		}
		TmapDwordToDword::iterator it = m_mapFamilyBlesslampTime.find(FID);
		if (it != m_mapFamilyBlesslampTime.end())
		{
			if (it->second >= BLESS_LAMP_PUT_COUNT && !bJiShangDengTest)
			{
				CMessage	msgOut(M_SC_ACT_WAIT);
				uint32		ErrorCode = E_BLESSLAMP_OVER;
				msgOut.serial(FID, ActPos, ErrorCode);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				return;
			}
		}

		realindex = FindNextBlesslampPos(ChannelID);
		if (realindex < 0)
		{
			// 길상등은 대기가 없다.
			CMessage	msgOut(M_SC_ACT_WAIT);
			uint32		ErrorCode = E_BLESSLAMP_WAIT;
			msgOut.serial(FID, ActPos, ErrorCode);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
			return;
		}
	}
	else if (ActPos == ACTPOS_AUTO2)
	{
		// 천원특사
		_RoomActivity	&Activity = pChannelData->m_Activitys[ACTPOS_YISHI - 1];
		if (Activity.FID == 0)
		{
			Activity.init(FID, AT_AUTO2, 0, 0);

			CMessage	msgOut(M_SC_ACT_WAIT);
			uint32		ErrorCode = 0;
			msgOut.serial(FID, ActPos, ErrorCode);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}
		else
		{
			sipwarning("Can't do this because it's test ActPos and other user doing it. FID=%d", FID);
		}
		return;
	}
	else if (ActPos == ACTPOS_RUYIBEI)
	{
		// Ruyibei글쓰기는 방주인만 할수 있다.
		if (FID != m_MasterID)
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"No room manager(Setruyibei) : RoomID=%d, MasterFID=%d, FID=%d", m_RoomID, m_MasterID, FID);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}

		_RoomActivity	&Activity = pChannelData->m_Activitys[index];
		if (Activity.FID != 0)
		{
			sipwarning("Unknown Error. Activity.FID != 0. RoomID=%d, FID=%d, Activity.FID=%d", m_RoomID, FID, Activity.FID);
			return;
		}

		Activity.init(FID, AT_RUYIBEI, 0, 0);

		CMessage	msgOut(M_SC_ACT_WAIT);
		uint32		ErrorCode = 0;
		msgOut.serial(FID, ActPos, ErrorCode);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

		return;
	}
	else // if ((ActPos < ACTPOS_WRITEMEMO_START) || ((ActPos >= ACTPOS_ANCESTOR) && (ActPos < ACTPOS_ANCESTOR_DECEASED_START + MAX_ANCESTOR_DECEASED_COUNT)))
	{
		// 의식, 편지쓰기, 물건태우기, 조상탑제사, 조상탑고인 향&꽃드리기, 방에 touxiang드리기
		_RoomActivity	&Activity = pChannelData->m_Activitys[index];

		CHECK_JISI_YISHIANDTOUXIANG_DOUBLE(ACTPOS_TOUXIANG_5, ACTPOS_TOUXIANG_5, E_ALREADY_JISITOUXIANG);
		CHECK_JISI_YISHIANDTOUXIANG_DOUBLE(ACTPOS_TOUXIANG_5, ACTPOS_ANCESTOR_JISI, E_ALREADY_JISIYISHI);
		CHECK_JISI_YISHIANDTOUXIANG_DOUBLE(ACTPOS_ANCESTOR_JISI, ACTPOS_TOUXIANG_5, E_ALREADY_JISITOUXIANG);

		if (Activity.FID == 0)
		{
			if (ActPos == ACTPOS_YISHI && 
				( pChannelData->m_Activitys[ACTPOS_AUTO2-1].FID != 0 ||
				  IsRoomMusicEventDoing() ))
			{
				RemainCount = 1 + Activity.WaitFIDs.size();
			}
			else
				realindex = index;
		}
		else
		{
			if (ActPos == ACTPOS_WRITELETTER)
			{
				// 편지쓰기는 대기가 없다.
				CMessage	msgOut(M_SC_ACT_WAIT);
				uint32		ErrorCode = E_LETTER_USING;
				msgOut.serial(FID, ActPos, ErrorCode);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				return;
			}
			if ((ActPos >= ACTPOS_ANCESTOR_DECEASED_START) && (ActPos < ACTPOS_ANCESTOR_DECEASED_START + MAX_ANCESTOR_DECEASED_COUNT))
			{
				// 조상탑고인 향&꽃드리기 는 대기가 없다.
				CMessage	msgOut(M_SC_ACT_WAIT);
				uint32		ErrorCode = E_ANCESTOR_DECEASED_USING;
				msgOut.serial(FID, ActPos, ErrorCode);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				return;
			}
			if (ActPos == ACTPOS_ROOMSTORE)
			{
				// 기념관창고추가행사는 대기가 없다.
				CMessage	msgOut(M_SC_ACT_WAIT);
				uint32		ErrorCode = E_ROOMSTORE_USING;
				msgOut.serial(FID, ActPos, ErrorCode);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				return;
			}
			if (ActPos >= ACTPOS_TOUXIANG_1 && ActPos <= ACTPOS_TOUXIANG_5)
			{
				// 방에 touxiang드리기는 대기가 없다.
				CMessage	msgOut(M_SC_ACT_WAIT);
				uint32		ErrorCode = E_ALREADY_FOJIAOTOUXIANG;
				msgOut.serial(FID, ActPos, ErrorCode);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				return;
			}
			RemainCount = 1 + Activity.WaitFIDs.size();
		}
	}

	CMessage	msgOut(M_SC_ACT_WAIT);
	uint32		ErrorCode = 0;
	msgOut.serial(FID);
	if (realindex >= 0)
	{
		FAMILY	*family = GetFamilyData(FID);

		_RoomActivity	&Activity = pChannelData->m_Activitys[realindex];
		// 행사 시작 가능
		ActPos = (uint8)(realindex + 1);
		RemainCount = 0;
		Activity.init(FID,AT_SIMPLERITE,0,0);//, family->m_Name);

		msgOut.serial(ActPos, ErrorCode, RemainCount);

		// 조상탑고인의 향/꽃드리기 인 경우 DeskNo를 더 보낸다.
		if ((ActPos >= ACTPOS_ANCESTOR_DECEASED_START) && (ActPos < ACTPOS_ANCESTOR_DECEASED_START + MAX_ANCESTOR_DECEASED_COUNT))
		{
			uint8	DeskNo = (uint8) m_ActionResults.getCurIndex(ActPos - ACTPOS_ANCESTOR_DECEASED_START + ActionResultType_AncestorDeceased_Hua);
			msgOut.serial(DeskNo);
		}

		//sipdebug("Start Activity: RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, FID); byy krs
	}
	else
	{
		_RoomActivity	&Activity = pChannelData->m_Activitys[index];

		_RoomActivity_Wait	data(FID,AT_SIMPLERITE,0,0);
		Activity.WaitFIDs.push_back(data);

		msgOut.serial(ActPos, ErrorCode, RemainCount);
		//sipdebug("Wait Activity: RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, FID); byy krs
	}
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

int CRoomWorld::FindNextBlesslampPos(uint32 ChannelID)
{
	int		realindex = -1;

	const OUT_ROOMKIND	*outRoomInfo = GetOutRoomKind(m_SceneID);
	if (outRoomInfo == NULL)
	{
		sipwarning("GetOutRoomKind = NULL! m_SceneID=%d", m_SceneID);
		return -1;
	}

	uint32	curTime = (uint32)GetDBTimeSecond();
	RoomChannelData	*pChannelData = GetChannel(ChannelID);

	int		FirstIndex = -1;
	uint32	FirstTime = 0;
	for (uint32 i = ACTPOS_BLESS_LAMP - 1; i < ACTPOS_BLESS_LAMP - 1 + outRoomInfo->BlessLampCount; i ++)
	{
		_RoomActivity	&Activity = pChannelData->m_Activitys[i];
		if (Activity.FID == 0)
		{
			uint32	j = i - (ACTPOS_BLESS_LAMP - 1);
			// 시간이 지났으면 길상등을 삭제
			if ((pChannelData->m_BlessLamp[j].MemoID != 0) && (curTime - pChannelData->m_BlessLamp[j].Time >= SHOW_BLESS_LAMP_TIME))
			{
				pChannelData->m_BlessLamp[j].MemoID = 0;
			}
			// 길상등이 없는 탁이라면 행사진행
			if (pChannelData->m_BlessLamp[j].MemoID == 0)
			{
				realindex = i;
				break;
			}
			// 길상등이 다 차있다면 제일 오래된 자리에서 하도록 한다.
			if ((FirstTime == 0) || (FirstTime > pChannelData->m_BlessLamp[j].Time))
			{
				FirstIndex = i;
				FirstTime = pChannelData->m_BlessLamp[j].Time;
			}
		}
	}
	if (realindex < 0)
	{
		// 모든 자리에서 행사가 진행중이라면 더 할수 없다.
		if (FirstIndex < 0)
			return -2;
		// 제일 오래된 자리에서 행사 진행
		realindex = FirstIndex;
	}

	return realindex;
}

// Activity Wait Cancel ([b:ActPos])
static void cb_M_CS_ACT_WAIT_CANCEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		ActPos;		_msgin.serial(ActPos);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->ChackActPosForActWaitCancel(FID, ActPos);
		inManager->ActWaitCancel(FID, ActPos, _tsid, 0);
	}
}

// Release NPC
static void cb_M_CS_ACT_RELEASE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->ActRelease(FID, _tsid);
	}
}

// 활동해제
void CRoomWorld::ActRelease(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	uint32	ChannelID = GetFamilyChannelID(FID);
	//sipdebug("ActRelease: FID=%d, RoomID=%d, ChannelID=%d", FID, m_RoomID, ChannelID); byy krs
	int		index = GetActivitingIndexOfUser(ChannelID, FID);
	if(index < 0)
	{
		sipwarning("GetActivitingIndexOfUser failed. FID=%d", FID);
		return;
	}

	ActWaitCancel(FID, (uint8)(index + 1), _tsid, 1);

	// XingLi인 경우 기록남기기
	if (index + 1 == ACTPOS_XINGLI_1 || index + 1 == ACTPOS_XINGLI_2)
	{
		INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
		if (pInfo == NULL)
		{
			sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
			return;
		}
		{
			pInfo->DonActInRoom(ROOMACT_XIANHUAXINGLI);

			MAKEQUERY_AddFishFlowerVisitData(strQ, pInfo->m_nCurActionGroup, FF_XINGLI, 0, 0);
			DBCaller->execute(strQ);
		}
		// 경험값증가
		ChangeFamilyExp(Family_Exp_Type_XingLi, FID);
	}
}

// Start Activity
static void cb_M_CS_ACT_START(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		ActPos;		_msgin.serial(ActPos);
	uint32		Param;		_msgin.serial(Param);

	// 2014/06/27:start
	uint8		GongpinCount;		_msgin.serial(GongpinCount);
	_ActivityStepParam	paramYisi;
	paramYisi.IsXDG = Param; // 2014/07/19
	paramYisi.GongpinCount = GongpinCount;
// 	if (GongpinCount != 0)
	if ( CheckActivityGongpinCount(ActPos, GongpinCount) ) 
	{
		for (int i = 0; i < GongpinCount; i++)
		{
			uint32 GongpinItemID;
			_msgin.serial(GongpinItemID);
			paramYisi.GongpinsList.push_back(GongpinItemID);
		}
		_msgin.serial(paramYisi.BowType);
	}
	else
	{
		sipwarning(L"gongpin count is invalid. ActPos=%d, Count=%d", ActPos, GongpinCount);
		return;
	}
	// end

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->ActStart(FID, ActPos, Param, _tsid, &paramYisi);
	}
}

bool IsImportantActpos(uint8 ActPos)
{
	if (ActPos == ACTPOS_YISHI || ActPos == ACTPOS_XIANBAO || ActPos == ACTPOS_XINGLI_1 || ActPos == ACTPOS_XINGLI_2
		|| ActPos == ACTPOS_ROOMSTORE || (ACTPOS_TOUXIANG_1 <= ActPos && ActPos <= ACTPOS_TOUXIANG_5))
		return true;
	return false;
}

// 활동 시작
void CRoomWorld::ActStart(T_FAMILYID FID, uint8 ActPos, uint32 Param, SIPNET::TServiceId _tsid, _ActivityStepParam* pYisiParam)// 2014/06/11
{
	if ((ActPos <= 0) || (ActPos >= COUNT_ACTPOS))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"ActStart - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	uint32	ChannelID = GetFamilyChannelID(FID);
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		ChannelID = 5001;
		pChannelData = GetChannel(ChannelID); // by krs
		if (pChannelData == NULL)
		{
			//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=5001", m_RoomID);			
			return;
		}		
	}

	int		index = (int)(ActPos - 1);
	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	if (Activity.FID != FID)
	{
		sipwarning("You are not turn!! ActPos=%d, CurTurnFID=%d, FID=%d", ActPos, Activity.FID, FID);
		return;
	}

	// 2014/06/11:start
	if (pYisiParam == NULL)
	{
		sipwarning("pYisiParam = NULL. FID=%d", FID);
		return;
	}
	// end

	// 2014/06/27:start:스텝정보들을 등록해둔다. 후에 들어오는 사람들에게 보내주기 위해
	Activity.ActivityStepParam = *pYisiParam;

// 	sipinfo(L"setting : FID=%d", FID); // must be deleted
// 	std::list<uint32>::iterator itlist;
// 	for (itlist = pYisiParam->GongpinsList.begin(); itlist != pYisiParam->GongpinsList.end(); itlist ++)
// 	{
// 		uint32 curitem = *itlist;
// 		sipinfo(L"gpid=%d", curitem);
// 	}
// 	sipinfo(L"gongpincount=%d, bowtype=%d, stepID=%d", pYisiParam->GongpinCount, pYisiParam->BowType, pYisiParam->StepID);
	// end

	//sipdebug("ActStart:FID=%d, RoomID=%d, ChannelID=%d, ActPos=%d, IsXDG=%d", FID, m_RoomID, ChannelID, ActPos, pYisiParam->IsXDG); byy krs

	if (Activity.StartTime > 0)
	{
		Activity.StartTime = 0;
		if (IsImportantActpos(ActPos))
		{
			ucstring	str;
			str = SIPBASE::_toString(L"ActStart: FID=%d, RoomID=%d, ChannelID=%d, ActPos=%d, IsXDG=%d", FID, m_RoomID, ChannelID, ActPos, pYisiParam->IsXDG);
			pChannelData->RegisterActionPlayer(FID, 30, str);	// 30분내에 행사가 끝나야 한다!
		}

		uint8	HuaPos = 0xFF;
		if ((ActPos >= ACTPOS_ANCESTOR_DECEASED_START) && (ActPos < ACTPOS_ANCESTOR_DECEASED_START + MAX_ANCESTOR_DECEASED_COUNT))
		{
			// Param : 1->꽃드리기, 2->향드리기
			if (Param == 1)
			{
				// 꽃드리기인 경우에는 현재 자리에 꽃이 있으면 그것을 삭제한다.
				HuaPos = (uint8) m_ActionResults.removeAncestorDeceasedCurrentHua(ActPos - ACTPOS_ANCESTOR_DECEASED_START);
			}
		}
		{
			uint8 acttype = static_cast<uint8>(Activity.ActivityType);
			{
				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACT_START)
					msgOut.serial(ActPos, acttype, Activity.FID, HuaPos);//, Activity.FName);
				FOR_END_FOR_FES_ALL()
			}

			// 2014/06/27:start
			{
				uint8	bZero = 0;
				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACT_NOW_START)
					msgOut.serial(ActPos, bZero, acttype, pYisiParam->IsXDG, Activity.FID);
					msgOut.serial(pYisiParam->GongpinCount);
					//if (pYisiParam->GongpinCount != 0)
					if ( CheckActivityGongpinCount(ActPos, pYisiParam->GongpinCount) )
					{
						std::list<uint32>::iterator itlist;
						for (itlist = pYisiParam->GongpinsList.begin(); itlist != pYisiParam->GongpinsList.end(); itlist ++)
						{
							uint32 curitem = *itlist;
							msgOut.serial(curitem);
						}
						uint8	joinCount = 0;
						msgOut.serial(pYisiParam->BowType, joinCount, pYisiParam->StepID);
					}
					else
					{
						sipwarning(L"gongpin count is invalid!! ActPos=%d, Count=%d", ActPos, pYisiParam->GongpinCount);
						continue;
					}
				FOR_END_FOR_FES_ALL()
			}
			// end

		}
	}
	else
	{
		sipwarning("Activity.StartTime = 0");
	}
}

// Send Act Step Command ([b:ActPos][d:StepID]) : 2014/06/09:start // 2014/06/27
static void cb_M_CS_ACT_STEP_OPENROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint8			ActPos;
	uint32			StepID;
	_msgin.serial(FID, ActPos, StepID);
	// 방관리자를 얻는다
	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager != NULL)
	{
 		inManager->on_M_CS_ACT_STEP(FID, ActPos, StepID);
	}
}

void CRoomWorld::on_M_CS_ACT_STEP(T_FAMILYID FID, uint8 ActPos, uint32 StepID)
{
	//sipinfo(L"RoomID=%d, FID=%d (ActPos:%d) (step %d)", m_RoomID, FID, ActPos, StepID); byy krs

	uint32	ChannelID = GetFamilyChannelID(FID);

	int		index = GetActivitingIndexOfUser(ChannelID, FID, false);
	if(index < 0)
	{
		if (!(FID >= AUTOPLAY_FID_START) && (FID < AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT)) { // by krs
			//sipwarning("GetActivitingIndexOfUser failed. FID=%d", FID);
			return;
		}		
	}
	index &= 0xFFFF;

	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		if ((FID >= AUTOPLAY_FID_START) && (FID < AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT)) { // by krs
			pChannelData = GetChannel(5001);
			if (pChannelData == NULL) {
				//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=5001", m_RoomID);
				return;
			}
		}		
	}

	// 2014/06/12:start:스텝정보들을 등록해둔다. 후에 들어오는 사람들에게 보내주기 위해
	int		index1 = (int)(ActPos - 1);
	_RoomActivity	&Activity = pChannelData->m_Activitys[index1];
	if (Activity.FID != FID)
	{
		sipwarning("You are not turn!! ActPos=%d, CurTurnFID=%d, FID=%d", ActPos, Activity.FID, FID);
		return;
	}
	Activity.ActivityStepParam.StepID = StepID;
	// end

	// 사용자들에게 통지
	uint8	bZero = 0;
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACT_STEP)
	{
		msgOut.serial(ActPos, StepID, FID, bZero);
	}
	FOR_END_FOR_FES_ALL()
}
// end

void CRoomWorld::on_M_CS_MULTIACT_REQADD(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID* ActFIDs, uint16 CountDownSec, SIPNET::TServiceId _tsid)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActReqAdd - Invalid ActPos!: FID=%d, ActPos=%d", HostFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, HostFID);
		return;
	}

	uint32	ChannelID = GetFamilyChannelID(HostFID);
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		//sipwarning("GetChannel = NULL. RoomID=%d, FID=%d, ChannehID=%d", m_RoomID, HostFID, ChannelID);
		return;
	}

	int		index = ActPos - 1;
	ucstring	addActFNames[MAX_MULTIACT_FAMILY_COUNT];

	// 이미 로그아우트되였거나 다른 행사중인 참가자들에 대하여 취소통보를 보낸다
	for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (ActFIDs[i] == 0)
			continue;

		FAMILY*	pFamily = GetFamilyData(ActFIDs[i]);
		if (pFamily)
		{
			addActFNames[i] = pFamily->m_Name;
			// 1명의 User는 하나의 활동만 할수 있다.
			if (GetActivitingIndexOfUser(ChannelID, ActFIDs[i], false) >= 0)
			{
				sipdebug("GetActivitingIndexOfUser(ChannelID, FID) >= 0. FID=%d", ActFIDs[i]);
			}
			else if (GetWaitingActivity(ChannelID, ActFIDs[i]) >= 0)
			{
				sipdebug("GetWaitingActivity(ChannelID, FID) >= 0, RoomID=%d, ChannelID=%d, FID=%d", m_RoomID, ChannelID, ActFIDs[i]);
			}
			else
				continue;
		}

		uint8	Answer = 0;
		CMessage	msgOut(M_SC_MULTIACT_ANS);
		msgOut.serial(HostFID, ActPos, ActFIDs[i], Answer);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

		ActFIDs[i] = 0;
	}

	// 추가된 참가자들을 목록에 추가한다
	_RoomActivity_Family* pAllFamily = NULL;
	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	if (Activity.FID == HostFID)
	{
		if (!IsMutiActionType(Activity.ActivityType))
		{
			sipwarning("on_M_CS_MULTIACT_REQADD: Activity.ActivityType=%d", Activity.ActivityType);
			return;
		}
		
		for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if (ActFIDs[i] == 0)
				continue;
			bool bResult = Activity.AddFamily(ActFIDs[i]);
			if (!bResult)
			{
				uint8	Answer = 0;
				CMessage	msgOut(M_SC_MULTIACT_ANS);
				msgOut.serial(HostFID, ActPos, ActFIDs[i], Answer);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				sipdebug("MultiActReqAdd Failed: RoomID=%d, HostFID=%d, JoinFID=%d", m_RoomID, HostFID, ActFIDs[i]);
				ActFIDs[i] = 0;
			}
			else
			{
				//sipdebug("MultiActReqAdd Success: RoomID=%d, HostFID=%d, JoinFID=%d", m_RoomID, HostFID, ActFIDs[i]); byy krs
			}
		}
		pAllFamily = Activity.ActivityUsers;
	}
	else
	{
		TWaitFIDList::iterator	it;
		for (it = Activity.WaitFIDs.begin(); it != Activity.WaitFIDs.end(); it ++)
		{
			if (!IsMutiActionType(it->ActivityType))
				continue;
			if (it->ActivityUsers[0].FID != HostFID)
				continue;
			
			for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
			{
				if (ActFIDs[i] == 0)
					continue;

				bool bResult = it->AddFamily(ActFIDs[i]);
				if (!bResult)
				{
					uint8	Answer = 0;
					CMessage	msgOut(M_SC_MULTIACT_ANS);
					msgOut.serial(HostFID, ActPos, ActFIDs[i], Answer);
					CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
					sipdebug("MultiActReqAddWait Failed: RoomID=%d, HostFID=%d, JoinFID=%d", m_RoomID, HostFID, ActFIDs[i]);
					ActFIDs[i] = 0;
				}
				else
				{
					//sipdebug("MultiActReqAddWait Success: RoomID=%d, HostFID=%d, JoinFID=%d", m_RoomID, HostFID, ActFIDs[i]); byy krs
				}
			}		
			pAllFamily = it->ActivityUsers;
			break;
		}
	}

	if (pAllFamily == NULL)
	{
		sipwarning("pAllFamily == NULL. HostFID=%d, ActPos=%d", HostFID, ActPos);
		return;
	}

	INFO_FAMILYINROOM	*pHostInfo = GetFamilyInfo(HostFID);
	if (pHostInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", HostFID);
		return;
	}
	// 전체참가자들의 이름을 얻는다
	ucstring	ActFNames[MAX_MULTIACT_FAMILY_COUNT];
	uint8	Count = 0;
	for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (pAllFamily[i].FID != 0)
		{
			FAMILY*	pFamily = GetFamilyData(pAllFamily[i].FID);
			if (pFamily == NULL)
			{
				sipwarning("GetFamilyData = NULL. FID=%d", pAllFamily[i].FID);
				pAllFamily[i].FID = 0;
				continue;
			}
			ActFNames[i] = pFamily->m_Name;
			Count ++;
		}
	}

	// 집체행사요청통보를 보낸다
	for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if(pAllFamily[i].FID == 0)
			continue;

		FAMILY*	pFamily = GetFamilyData(pAllFamily[i].FID);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", pAllFamily[i].FID);
			continue;
		}

		bool bAddFamily = false;
		for (int k = 0; k < MAX_MULTIACT_FAMILY_COUNT; k++)
		{
			if (ActFIDs[k] == pAllFamily[i].FID)
			{
				bAddFamily = true;
				break;
			}
		}
		if (bAddFamily)
		{
			CMessage	msgOut(M_SC_MULTIACT_REQ);
			uint8	actty = AT_ANCESTOR_MULTIJISI;
			if (ActPos == ACTPOS_YISHI)
				actty = AT_MULTIRITE;
			msgOut.serial(pAllFamily[i].FID, ActPos, actty);
			msgOut.serial(HostFID, pHostInfo->m_Name, m_RoomID, m_RoomName, ChannelID, m_SceneID, Count);
			for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if (pAllFamily[j].FID != 0)
				{
					msgOut.serial(pAllFamily[j].FID, ActFNames[j]);
					uint8 JoinStatus = static_cast<uint8>(pAllFamily[j].Status);
					msgOut.serial(JoinStatus);
				}
			}
			msgOut.serial(CountDownSec);
			CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
		}
		else
		{
			CMessage	msgOut(M_SC_MULTIACT_REQADD);
			msgOut.serial(pAllFamily[i].FID);
			for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if (ActFIDs[j] != 0)
				{
					msgOut.serial(ActFIDs[j], addActFNames[j]);
				}
			}
			CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
		}
	}

}

// 집체활동시작준비
void CRoomWorld::on_M_CS_MULTIACT_REQ(T_FAMILYID* ActFIDs, uint8 ActPos, uint8 ActType, uint16 CountDownSec, SIPNET::TServiceId _tsid)
{
	uint32	HostFID = ActFIDs[0];

	int		index = (int)ActPos - 1;
	int		i;

	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", HostFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, HostFID);
		return;
	}

	// 아래의 코드는 제사지내기처리부이다.
	uint32	ChannelID = GetFamilyChannelID(HostFID);
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. RoomID=%d, FID=%d, ChannehID=%d", m_RoomID, HostFID, ChannelID);
		return;
	}

	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (ActFIDs[i] == 0)
			continue;

		FAMILY*	pFamily = GetFamilyData(ActFIDs[i]);
		if (pFamily)
		{
			// 1명의 User는 하나의 활동만 할수 있다.
			if (GetActivitingIndexOfUser(ChannelID, ActFIDs[i], false) >= 0)
			{
				sipdebug("GetActivitingIndexOfUser(ChannelID, FID) >= 0. FID=%d", ActFIDs[i]);
			}
			else if (GetWaitingActivity(ChannelID, ActFIDs[i]) >= 0)
			{
				sipdebug("GetWaitingActivity(ChannelID, FID) >= 0, RoomID=%d, ChannelID=%d, FID=%d", m_RoomID, ChannelID, ActFIDs[i]);
			}
			else
				continue;
		}

		if (i == 0)
		{
			sipwarning("Action Request Error.");
			return;
		}

		uint8	Answer = 0;
		CMessage	msgOut(M_SC_MULTIACT_ANS);
		msgOut.serial(HostFID, ActPos, ActFIDs[i], Answer);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

		ActFIDs[i] = 0;
	}

	// 행사설정
	{
		_RoomActivity	&Activity = pChannelData->m_Activitys[index];
		uint32	RemainCount;
		if (Activity.FID == 0)
		{
			if (ActPos == ACTPOS_YISHI && 
				(pChannelData->m_Activitys[ACTPOS_AUTO2-1].FID != 0 ||
				IsRoomMusicEventDoing()))
			{
				RemainCount = 1 + Activity.WaitFIDs.size();
				_RoomActivity_Wait	data(HostFID, ActType,0,0,ActFIDs);
				Activity.WaitFIDs.push_back(data);
			}
			else
			{
				// 현재 진행중이 아니면 행사시작가능
				RemainCount = 0;
				Activity.init(HostFID, ActType,0,0,ActFIDs);
			}
		}
		else
		{
			// 현재 진행중이면 대기
			RemainCount = 1 + Activity.WaitFIDs.size();

			_RoomActivity_Wait	data(HostFID, ActType, 0,0,ActFIDs);
			Activity.WaitFIDs.push_back(data);
		}

		// 본인에게 M_SC_MULTIACT_WAIT 보내기
		CMessage	msgOut(M_SC_MULTIACT_WAIT);
		uint32		err = 0;
		msgOut.serial(HostFID, ActPos, err, RemainCount);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	// 행사에 참가하는 Family들에 M_SC_MULTIACT_REQ 보내기
	{
		INFO_FAMILYINROOM	*pHostInfo = GetFamilyInfo(HostFID);
		if (pHostInfo == NULL)
		{
			sipwarning("GetFamilyInfo = NULL. FID=%d", HostFID);
			return;
		}

		ucstring	ActFNames[MAX_MULTIACT_FAMILY_COUNT];
		uint8	Count = 0;
		for (i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if (ActFIDs[i] != 0)
			{
				FAMILY*	pFamily = GetFamilyData(ActFIDs[i]);
				if (pFamily == NULL)
				{
					sipwarning("GetFamilyData = NULL. FID=%d", ActFIDs[i]);
					ActFIDs[i] = 0;
					continue;
				}
				ActFNames[i] = pFamily->m_Name;
				Count ++;
			}
		}

		for (i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if(ActFIDs[i] == 0)
				continue;

			FAMILY*	pFamily = GetFamilyData(ActFIDs[i]);
			if (pFamily == NULL)
			{
				sipwarning("GetFamilyData = NULL. FID=%d", ActFIDs[i]);
				continue;
			}

			CMessage	msgOut(M_SC_MULTIACT_REQ);
			msgOut.serial(ActFIDs[i], ActPos, ActType);
			msgOut.serial(HostFID, pHostInfo->m_Name, m_RoomID, m_RoomName, ChannelID, m_SceneID, Count);
			for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if (ActFIDs[j] != 0)
				{
					msgOut.serial(ActFIDs[j], ActFNames[j]);
					uint8 JoinStatus = MULTIACTIVITY_FAMILY_STATUS_NONE;
					msgOut.serial(JoinStatus);
				}
			}
			msgOut.serial(CountDownSec);
			CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
		}
	}
}

_RoomActivity_Family* CRoomWorld::FindJoinFIDStatus(uint32 ChannelID, T_FAMILYID JoinFID)
{
	const int MULTIACT_AVILABLE_ACTPOS[] = {ACTPOS_YISHI, ACTPOS_ANCESTOR_JISI};

	for (int actindex = 0; actindex < sizeof(MULTIACT_AVILABLE_ACTPOS) / sizeof(MULTIACT_AVILABLE_ACTPOS[0]); actindex ++)
	{
		int		index = MULTIACT_AVILABLE_ACTPOS[actindex] - 1;

		RoomChannelData	*pChannelData = GetChannel(ChannelID);
		if (pChannelData == NULL)
		{
			sipwarning("GetChannel = NULL. ChannelID=%d", ChannelID);
			return NULL;
		}

		_RoomActivity	&Activity = pChannelData->m_Activitys[index];

		if (IsMutiActionType(Activity.ActivityType))
		{
			if (Activity.StartTime != 0)
			{
				// 행사준비중...
				for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
				{
					if (Activity.ActivityUsers[i].FID == JoinFID)
					{
						return &Activity.ActivityUsers[0];
					}
				}
			}
		}

		TWaitFIDList::iterator	it;
		for (it = Activity.WaitFIDs.begin(); it != Activity.WaitFIDs.end(); it ++)
		{
			if (!IsMutiActionType(it->ActivityType))
				continue;
			for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if (it->ActivityUsers[j].FID == JoinFID)
				{
					return &it->ActivityUsers[0];
				}
			}
		}
	}

	return NULL;
}

_RoomActivity_Family* CRoomWorld::FindJoinFID(uint32 ChannelID, uint8 ActPos, T_FAMILYID JoinFID, uint32* pRemainCount)
{
	int		index = (int)ActPos - 1;

	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. ChannelID=%d", ChannelID);
		return NULL;
	}

	_RoomActivity	&Activity = pChannelData->m_Activitys[index];

	if(IsMutiActionType(Activity.ActivityType))
	{
		//if(Activity.StartTime != 0)
		//{
		//	// 행사준비중...
			for(int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
			{
				if(Activity.ActivityUsers[i].FID == JoinFID)
				{
					if(pRemainCount)
						(*pRemainCount) = 0;
					return &Activity.ActivityUsers[0];
				}
			}
		//}
	}

	TWaitFIDList::iterator	it;
	uint32		RemainCount = 1;
	for(it = Activity.WaitFIDs.begin(); it != Activity.WaitFIDs.end(); it ++)
	{
		if(!IsMutiActionType(it->ActivityType))
			continue;
		for(int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
		{
			if(it->ActivityUsers[j].FID == JoinFID)
			{
				if(pRemainCount)
					(*pRemainCount) = RemainCount;
				return &it->ActivityUsers[0];
			}
		}
		RemainCount ++;
	}

	return NULL;
}

void CRoomWorld::on_M_CS_MULTIACT_ANS(T_FAMILYID JoinFID, uint32 ChannelID, uint8 ActPos, uint8 Answer)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", JoinFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, JoinFID);
		return;
	}

	// 아래의 코드는 제사지내기처리부이다.
/*	if(Answer == 1)
	{
		// 만일 Family가 현재의 방에 없다면 들어올수 있게 초청상태로 만들어준다.
		if(GetFamilyChannel(JoinFID) == NULL)
		{
			// 진입대기목록갱신
			MAP_ENTERROOMWAIT::iterator it = map_EnterWait.find(JoinFID);
			if (it != map_EnterWait.end())
			{
				it->second.RequestTime = CTime::getLocalTime();
			}
			else
			{
				ENTERROOMWAIT	newenter;
				newenter.FID = JoinFID;		newenter.RequestTime = CTime::getLocalTime();
				map_EnterWait.insert( make_pair(JoinFID, newenter) );
			}
		}
	}
*/
	_RoomActivity_Family* pActivityUsers = FindJoinFID(ChannelID, ActPos, JoinFID);
	if (pActivityUsers == NULL)
	{
		sipwarning("FindJoinFID = NULL. RoomID=%d, ChannelID=%d, ActPos=%d, JoinFID=%d. (Reason : already started or canceled?)", m_RoomID, ChannelID, ActPos, JoinFID);
		return;
	}

	int pos;
	for (pos = 1; pos < MAX_MULTIACT_FAMILY_COUNT; pos ++)
	{
		if(pActivityUsers[pos].FID == JoinFID)
			break;
	}
	if (pos >= MAX_MULTIACT_FAMILY_COUNT)
	{
		sipwarning("Unknown Error!!!!!(13013). FID=%d, RoomID=%d, ActPos=%d", JoinFID, m_RoomID, ActPos);
		return;
	}
	if (pActivityUsers[pos].Status != MULTIACTIVITY_FAMILY_STATUS_NONE)
	{
		sipwarning("pActivityUsers[pos].Status != MULTIACTIVITY_FAMILY_STATUS_NONE. FID=%d, RoomID=%d, Status=%d", JoinFID, m_RoomID, pActivityUsers[pos].Status);
		return;
	}

	// 검사 OK
	if (Answer == 1)
	{
		pActivityUsers[pos].Status = MULTIACTIVITY_FAMILY_STATUS_ACCEPT;
	}
	else
		pActivityUsers[pos].FID = 0;

	// M_SC_MULTIACT_ANS를 보낸다.
	for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (i == pos)
			continue;
		if (pActivityUsers[i].FID == 0)
			continue;

		FAMILY*	pFamily = GetFamilyData(pActivityUsers[i].FID);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", pActivityUsers[i].FID);
			continue;
		}

		CMessage	msgOut(M_SC_MULTIACT_ANS);
		msgOut.serial(pActivityUsers[i].FID, ActPos, JoinFID, Answer);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
	}
}

void CRoomWorld::on_M_CS_MULTIACT_READY(T_FAMILYID JoinFID, uint8 ActPos, TServiceId _tsid)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", JoinFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, JoinFID);
		return;
	}

	// 아래의 코드는 제사지내기처리부이다.
	uint32	ChannelID = GetFamilyChannelID(JoinFID);
	if (ChannelID == 0)
	{
		sipwarning("GetFamilyChannelID = 0. RoomID=%d, FID=%d", m_RoomID, JoinFID);
		return;
	}

	uint32		RemainCount = 0;
	_RoomActivity_Family* pActivityUsers = FindJoinFID(ChannelID, ActPos, JoinFID, &RemainCount);
	if (pActivityUsers == NULL)
	{
		CMessage	msgOut(M_SC_MULTIACT_WAIT);
		uint32		err = E_MULTIACT_STARTED;
		msgOut.serial(JoinFID, ActPos, err, RemainCount);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

		sipwarning("FindJoinFID = NULL. RoomID=%d, ChannelID=%d, ActPos=%d, JoinFID=%d. (Reason : already started or canceled?)", m_RoomID, ChannelID, ActPos, JoinFID);
		return;
	}

	int pos;
	for (pos = 1; pos < MAX_MULTIACT_FAMILY_COUNT; pos ++)
	{
		if(pActivityUsers[pos].FID == JoinFID)
			break;
	}
	if (pos >= MAX_MULTIACT_FAMILY_COUNT)
	{
		sipwarning("Unknown Error!!!!!(13127)");
		return;
	}
	if (pActivityUsers[pos].Status != MULTIACTIVITY_FAMILY_STATUS_ACCEPT)
	{
		sipwarning("pActivityUsers[pos].Status != MULTIACTIVITY_FAMILY_STATUS_ACCEPT. Status=%d, RoomID=%d, ChannelID=%d, ActPos=%d, FID=%d", pActivityUsers[pos].Status, m_RoomID, ChannelID, ActPos, JoinFID);
		return;
	}

	// 검사 OK
	pActivityUsers[pos].Status = MULTIACTIVITY_FAMILY_STATUS_READY;

	// M_SC_MULTIACT_WAIT 를 보낸다.
	{
		CMessage	msgOut(M_SC_MULTIACT_WAIT);
		uint32		err = 0;
		msgOut.serial(JoinFID, ActPos, err, RemainCount);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	// 행사참가자들에게 M_SC_MULTIACT_READY를 보낸다.
	for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (i == pos)
			continue;
		if (pActivityUsers[i].FID == 0)
			continue;

		FAMILY*	pFamily = GetFamilyData(pActivityUsers[i].FID);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", pActivityUsers[i].FID);
			continue;
		}

		CMessage	msgOut(M_SC_MULTIACT_READY);
		msgOut.serial(pActivityUsers[i].FID, ActPos, JoinFID);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
	}
}

// 
void CRoomWorld::on_M_CS_MULTIACT_GO(T_FAMILYID HostFID, uint8 ActPos, uint32 bXianDaGong, _ActivityStepParam* pActParam)
{
	int		index = (int)ActPos - 1;
	int		i;

	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", HostFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, HostFID);
		return;
	}

	// 아래의 코드는 제사지내기처리부이다.
	uint32	ChannelID = GetFamilyChannelID(HostFID);
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. RoomID=%d, FID=%d, ChannehID=%d", m_RoomID, HostFID, ChannelID);
		return;
	}

	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	if (Activity.FID != HostFID)
	{
		sipwarning("No your turn !!! RoomID=%d, ActPos=%d, Activity.FID=%d, FID=%d", m_RoomID, ActPos, Activity.FID, HostFID);
		return;
	}
	if (!IsMutiActionType(Activity.ActivityType))
	{
		sipwarning("This is not MultiActivity. FID=%d", HostFID);
		return;
	}

	// 2014/07/02:start:스텝정보들을 등록해둔다. 후에 들어오는 사람들에게 보내주기 위해
	if (pActParam == NULL)
	{
		sipwarning("pActParam = NULL. RoomID=%d, FID=%d, ActPos=%d, XianDaGong=%d", m_RoomID, HostFID, ActPos, bXianDaGong);
		return;
	}

	Activity.ActivityStepParam = *pActParam;// 현재는 아이템관련정보만 설정된다. 행사참가자관련은 알지 못한다.
	if (pActParam->GongpinsList.size() != pActParam->GongpinCount || pActParam->GongpinCount == 0)
		sipwarning(L"Gongpin count mismatch! listcount=%d, count=%d", pActParam->GongpinsList.size(), pActParam->GongpinCount);

// 	sipinfo(L"setting : FID=%d", HostFID); // must be deleted
// 	std::list<uint32>::iterator itlist;
// 	for (itlist = pActParam->GongpinsList.begin(); itlist != pActParam->GongpinsList.end(); itlist ++)
// 	{
// 		uint32 curitem = *itlist;
// 		sipinfo(L"gpid=%d", curitem);
// 	}
// 	sipinfo(L"goingpincount=%d, bowtype=%d, stepID=%d", pActParam->GongpinCount, pActParam->BowType, pActParam->StepID);
	// end

	uint8	Count = 0;
	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if ((Activity.ActivityUsers[i].FID != 0) && (Activity.ActivityUsers[i].Status == MULTIACTIVITY_FAMILY_STATUS_READY))
		{
			Count ++;
		}
	}

	uint8	zeroDeskNo = 0;
	// 행사참가자들에게 M_SC_MULTIACT_GO 를 보낸다.
	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if(Activity.ActivityUsers[i].FID == 0)
			continue;

		FAMILY	*pFamily = GetFamilyData(Activity.ActivityUsers[i].FID);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", Activity.ActivityUsers[i].FID);
			Activity.ActivityUsers[i].FID = 0;
			continue;
		}

		CMessage	msgOut(M_SC_MULTIACT_GO);
		msgOut.serial(Activity.ActivityUsers[i].FID, ActPos, Count);
		for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
		{
			if ((Activity.ActivityUsers[j].FID != 0) && (Activity.ActivityUsers[j].Status == MULTIACTIVITY_FAMILY_STATUS_READY))
			{
				msgOut.serial(Activity.ActivityUsers[j].FID);
			}
		}
		msgOut.serial(bXianDaGong, zeroDeskNo);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);

		if (Activity.ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_READY)
			Activity.ActivityUsers[i].FID = 0;
		else {
			//sipdebug("MultiActGo:HostFID=%d, JoinFID=%d, RoomID=%d, ChannelID=%d, ActPos=%d, IsXDG=%d", HostFID, Activity.ActivityUsers[i].FID, m_RoomID, ChannelID, ActPos, pActParam->IsXDG); byy krs
		}			
	}

	// 주의 사람들에게 행사가 시작됨을 통지한다. M_SC_MULTIACT_STARTED
	{
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTIACT_STARTED)
			uint8		ActType = static_cast<uint8>(Activity.ActivityType);
			msgOut.serial(ActPos, ActType, Count);
			for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if ((Activity.ActivityUsers[j].FID != 0) && (Activity.ActivityUsers[j].Status == MULTIACTIVITY_FAMILY_STATUS_READY))
				{
					msgOut.serial(Activity.ActivityUsers[j].FID);
				}
			}
			msgOut.serial(zeroDeskNo);
		FOR_END_FOR_FES_ALL()
	}

	// 한명만 남은 경우에는 개인행사형식으로 바꾼다.
	// 그러나 여러명등불올리기는 개인행사자체가 없으므로 단체행사로 그대로 둔다.
	if ((Count == 1) && (ActPos != ACTPOS_MULTILANTERN))
	{
		Activity.ActivityType = AT_SIMPLERITE;
	}
}

// 
void CRoomWorld::on_M_CS_MULTIACT_START(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint16 InvenPos, uint32 SyncCode, uint8 Secret, ucstring Pray, TServiceId _tsid)
{
	int		index = (int)ActPos - 1;
	int		i;

	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	uint32	ChannelID = GetFamilyChannelID(FID);
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. RoomID=%d, FID=%d, ChannehID=%d", m_RoomID, FID, ChannelID);
		return;
	}

	bool	/*bAllStarted = true,*/ bExistFID = false;
	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	if (!IsMutiActionType(Activity.ActivityType))
	{
		sipwarning("This is not MultiActivity. FID=%d", FID);
		return;
	}

	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (Activity.ActivityUsers[i].FID == FID)
		{
			if (InvenPos == 0)
				Activity.ActivityUsers[i].ItemID = 0;
			else
			{
				_InvenItems*	pInven = GetFamilyInven(FID);
				if (pInven == NULL)
				{
					sipwarning("GetFamilyInven NULL. FID=%d", FID);
					return;
				}
				int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
				Activity.ActivityUsers[i].ItemID = pInven->Items[InvenIndex].ItemID;
			}

			Activity.ActivityUsers[i].StoreLockID = StoreLockID;
			Activity.ActivityUsers[i].InvenPos = InvenPos;
			Activity.ActivityUsers[i].SyncCode = SyncCode;
			Activity.ActivityUsers[i].Secret = Secret;
			Activity.ActivityUsers[i].Pray = Pray;
			Activity.ActivityUsers[i].Status = MULTIACTIVITY_FAMILY_STATUS_START;
			bExistFID = true;
		}
		//else if((Activity.ActivityUsers[i].FID != 0) && (Activity.ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_START))
		//	bAllStarted = false;
	}
	if (!bExistFID)
	{
		sipwarning("Unknown Error!!!! (13321) RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	CheckMultiactAllStarted(ChannelID, ActPos);

//	if (bAllStarted)
//	{
//		if (ActPos == ACTPOS_MULTILANTERN)
//		{
//			// ACTPOS_MULTILANTERN인 경우 여기서 기본처리를 한다.
//			INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(Activity.ActivityUsers[0].FID);
//			if (pInfo == NULL)
//			{
//				sipwarning("GetFamilyInfo = NULL. Activity.ActivityUsers[0].FID=%d", Activity.ActivityUsers[0].FID);
//				return;
//			}
//			uint32	ItemID = FamilyItemUsed(Activity.ActivityUsers[0].FID, Activity.ActivityUsers[0].InvenPos, Activity.ActivityUsers[0].SyncCode, ITEMTYPE_OTHER, pInfo->m_ServiceID);
//			if (ItemID == 0)
//			{
//				return;
//			}
//
//			for (i = MAX_MULTIACT_FAMILY_COUNT - 1; i >= 0; i --)
//			{
//				uint32	actFID = Activity.ActivityUsers[i].FID;
//				if (actFID == 0)
//					continue;
//
//				INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(Activity.ActivityUsers[i].FID);
//				if (pInfo == NULL)
//				{
//					sipwarning("GetFamilyInfo = NULL. FID=%d", actFID);
//					continue;
//				}
//
//				float	actfDir, actfX, actfY, actfZ;
//				if (!pInfo->GetFirstCHState(actfDir, actfX, actfY, actfZ))
//				{
//					sipwarning("GetFirstCHState failed. FID=%d", actFID);
//					continue;
//				}
//
//				uint32	actDir = *((uint32*)(&actfDir));
//				uint32	actX = *((uint32*)(&actfX));
//				uint32	actY = *((uint32*)(&actfY));
//				uint32	actZ = *((uint32*)(&actfZ));
//				AddNewLantern(actFID, actX, actY, actZ, actDir, ItemID);
//			}
//		}
//
//		Activity.StartTime = 0;
//		if (IsImportantActpos(ActPos))
//			pChannelData->RegisterActionPlayer(Activity.ActivityUsers[0].FID, 30);	// 30분내에 행사가 끝나야 한다!
//
//		// 행사참가자들에게 M_SC_MULTIACT_START 를 보낸다.
//		for (i = MAX_MULTIACT_FAMILY_COUNT - 1; i >= 0; i --)
//		{
//			if (Activity.ActivityUsers[i].FID == 0)
//				continue;
//
//			//FAMILY	*pFamily = GetFamilyData(Activity.ActivityUsers[i].FID);
//			//if (pFamily == NULL)
//			//{
//			//	sipwarning("GetFamilyData = NULL. FID=%d", Activity.ActivityUsers[i].FID);
//			//	continue;
//			//}
//
//			CMessage	msgOut(M_SC_MULTIACT_START);
//			for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
//			{
//				if(Activity.ActivityUsers[j].FID != 0)
//					msgOut.serial(Activity.ActivityUsers[j].FID);
//			}
//			uint32	zero = 0;
////			uint8	ItemExistFlag = 0;
////			if (Activity.ActivityUsers[i].InvenPos != 0)
////				ItemExistFlag = 1;
//			msgOut.serial(zero);
//			msgOut.serial(ActPos, Activity.ActivityUsers[i].FID, Activity.ActivityUsers[i].Secret, Activity.ActivityUsers[i].ItemID, Activity.ActivityUsers[i].Pray);
//			CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
//		}
//
//		// 2014/07/02:start:행사참가자들을 포함한 방의 모든 사람들에게 M_SC_MULTIACT_NOW_START 를 보낸다
//		uint32 HostFID = Activity.ActivityUsers[0].FID;
//		if (HostFID == 0)
//		{
//			sipwarning(L"HostFID is 0!!");
//			return;
//		}
//
//		_ActivityStepParam* pActParam = &Activity.ActivityStepParam;
////		if (pActParam->JoinCount != 0)
////		{
////			sipwarning(L"JoinInfo is not initialized! HostFID=%d", HostFID);
////			pActParam->JoinCount = 0;
////			pActParam->JoinList.clear();
//// 			return;
////		}
//		// 행사참가자(조인)들의 정보를 설정한다.
////		for (i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
////		{
////			if(Activity.ActivityUsers[i].FID != 0)
////			{
////				pActParam->JoinCount ++;
////				_JoinParam join;
////				join.init(Activity.ActivityUsers[i].FID, Activity.ActivityUsers[i].ItemID);
////				pActParam->JoinList.push_back(join);
////			}
////		}
//		
//		// 
//		if (pActParam->GongpinCount == 0)
//		{
//			sipwarning(L"gongpin count is zero!");
//			return;
//		}
//		{
//			uint8	Count = 0;
//			for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
//			{
//				if(Activity.ActivityUsers[j].FID != 0)
//					Count ++;
//			}
//
//			uint8	bZero = 0;
//			uint8		ActType = static_cast<uint8>(Activity.ActivityType);
//			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACT_NOW_START)
//				msgOut.serial(ActPos, bZero, ActType, pActParam->IsXDG, HostFID);
//				msgOut.serial(pActParam->GongpinCount);
//				std::list<uint32>::iterator itlist;
//				for (itlist = pActParam->GongpinsList.begin(); itlist != pActParam->GongpinsList.end(); itlist ++)
//				{
//					uint32 curitem = *itlist;
//					msgOut.serial(curitem);
//				}
//				msgOut.serial(pActParam->BowType, Count);
//
//				for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
//				{
//					uint8		PosIndex = j - 1;
//					if (Activity.ActivityUsers[j].FID != 0)
//					{
//						msgOut.serial(Activity.ActivityUsers[j].FID, Activity.ActivityUsers[j].ItemID, PosIndex);
//					}
//				}
//
//				msgOut.serial(pActParam->StepID);
//			FOR_END_FOR_FES_ALL()
//		}
//		// end
//	}
}

void CRoomWorld::CheckMultiactAllStarted(uint32 ChannelID, uint8 ActPos)
{
	int		index = (int)ActPos - 1;
	int		i;
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. RoomID=%d, ChannehID=%d, ActPos=%d", m_RoomID, ChannelID, ActPos);
		return;
	}
	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	if ((Activity.FID == 0) || (!IsMutiActionType(Activity.ActivityType)) || (Activity.StartTime == 0))
		return;

	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if((Activity.ActivityUsers[i].FID != 0) && (Activity.ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_START))
			return;
	}

	// All Started
	{
		if (ActPos == ACTPOS_MULTILANTERN)
		{
			// ACTPOS_MULTILANTERN인 경우 여기서 기본처리를 한다.
			INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(Activity.ActivityUsers[0].FID);
			if (pInfo == NULL)
			{
				sipwarning("GetFamilyInfo = NULL. Activity.ActivityUsers[0].FID=%d", Activity.ActivityUsers[0].FID);
				return;
			}
			uint32	ItemID = FamilyItemUsed(Activity.ActivityUsers[0].FID, Activity.ActivityUsers[0].InvenPos, Activity.ActivityUsers[0].SyncCode, ITEMTYPE_OTHER, pInfo->m_ServiceID);
			if (ItemID == 0)
			{
				sipwarning("ACTPOS_MULTILANTERN, ItemID=0?");
				return;
			}

			for (i = MAX_MULTIACT_FAMILY_COUNT - 1; i >= 0; i --)
			{
				uint32	actFID = Activity.ActivityUsers[i].FID;
				if (actFID == 0)
					continue;

				INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(Activity.ActivityUsers[i].FID);
				if (pInfo == NULL)
				{
					sipwarning("GetFamilyInfo = NULL. FID=%d", actFID);
					continue;
				}

				float	actfDir, actfX, actfY, actfZ;
				if (!pInfo->GetFirstCHState(actfDir, actfX, actfY, actfZ))
				{
					sipwarning("GetFirstCHState failed. FID=%d", actFID);
					continue;
				}

				uint32	actDir = *((uint32*)(&actfDir));
				uint32	actX = *((uint32*)(&actfX));
				uint32	actY = *((uint32*)(&actfY));
				uint32	actZ = *((uint32*)(&actfZ));
				AddNewLantern(actFID, actX, actY, actZ, actDir, ItemID);
			}
		}

		Activity.StartTime = 0;
		if (IsImportantActpos(ActPos))
		{
			ucstring	str;
			str = SIPBASE::_toString(L"MultiActStart: FID=%d, ChannelID=%d, ActPos=%d", Activity.ActivityUsers[0].FID, ChannelID, ActPos);
			pChannelData->RegisterActionPlayer(Activity.ActivityUsers[0].FID, 30, str);	// 30분내에 행사가 끝나야 한다!
		}

		// 행사참가자들에게 M_SC_MULTIACT_START 를 보낸다.
		for (i = MAX_MULTIACT_FAMILY_COUNT - 1; i >= 0; i --)
		{
			if (Activity.ActivityUsers[i].FID == 0)
				continue;

			//FAMILY	*pFamily = GetFamilyData(Activity.ActivityUsers[i].FID);
			//if (pFamily == NULL)
			//{
			//	sipwarning("GetFamilyData = NULL. FID=%d", Activity.ActivityUsers[i].FID);
			//	continue;
			//}

			CMessage	msgOut(M_SC_MULTIACT_START);
			for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if(Activity.ActivityUsers[j].FID != 0)
					msgOut.serial(Activity.ActivityUsers[j].FID);
			}
			uint32	zero = 0;
//			uint8	ItemExistFlag = 0;
//			if (Activity.ActivityUsers[i].InvenPos != 0)
//				ItemExistFlag = 1;
			msgOut.serial(zero);
			msgOut.serial(ActPos, Activity.ActivityUsers[i].FID, Activity.ActivityUsers[i].Secret, Activity.ActivityUsers[i].ItemID, Activity.ActivityUsers[i].Pray);
			CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
		}

		// 2014/07/02:start:행사참가자들을 포함한 방의 모든 사람들에게 M_SC_MULTIACT_NOW_START 를 보낸다
		uint32 HostFID = Activity.ActivityUsers[0].FID;
		if (HostFID == 0)
		{
			sipwarning(L"HostFID is 0!!");
			return;
		}

		_ActivityStepParam* pActParam = &Activity.ActivityStepParam;
//		if (pActParam->JoinCount != 0)
//		{
//			sipwarning(L"JoinInfo is not initialized! HostFID=%d", HostFID);
//			pActParam->JoinCount = 0;
//			pActParam->JoinList.clear();
// 			return;
//		}
		// 행사참가자(조인)들의 정보를 설정한다.
//		for (i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
//		{
//			if(Activity.ActivityUsers[i].FID != 0)
//			{
//				pActParam->JoinCount ++;
//				_JoinParam join;
//				join.init(Activity.ActivityUsers[i].FID, Activity.ActivityUsers[i].ItemID);
//				pActParam->JoinList.push_back(join);
//			}
//		}
		
		// 
		if (pActParam->GongpinCount == 0)
		{
			sipwarning(L"gongpin count is zero!");
			return;
		}
		{
			uint8	Count = 0;
			for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if(Activity.ActivityUsers[j].FID != 0)
					Count ++;
			}

			uint8	bZero = 0;
			uint8		ActType = static_cast<uint8>(Activity.ActivityType);
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACT_NOW_START)
				msgOut.serial(ActPos, bZero, ActType, pActParam->IsXDG, HostFID);
				msgOut.serial(pActParam->GongpinCount);
				std::list<uint32>::iterator itlist;
				for (itlist = pActParam->GongpinsList.begin(); itlist != pActParam->GongpinsList.end(); itlist ++)
				{
					uint32 curitem = *itlist;
					msgOut.serial(curitem);
				}
				msgOut.serial(pActParam->BowType, Count);

				for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
				{
					uint8		PosIndex = j - 1;
					if (Activity.ActivityUsers[j].FID != 0)
					{
						msgOut.serial(Activity.ActivityUsers[j].FID, Activity.ActivityUsers[j].ItemID, PosIndex);
					}
				}

				msgOut.serial(pActParam->StepID);
			FOR_END_FOR_FES_ALL()
		}
		// end
	}
}

// 
void CRoomWorld::on_M_CS_MULTIACT_RELEASE(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint32 SyncCode, uint8 Number, uint16 *pInvenPos, TServiceId _tsid)
{
	int		index = (int)ActPos - 1;
	int		i;

	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	// 아래의 코드는 제사지내기처리부이다.
	uint32	ChannelID = GetFamilyChannelID(FID);
	RoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. RoomID=%d, FID=%d, ChannehID=%d", m_RoomID, FID, ChannelID);
		return;
	}

	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	if (Activity.FID != FID)
	{
		sipwarning("Activity.FID != FID..   RoomID=%d, ActPos=%d, Activity.FID=%d, FID=%d", m_RoomID, ActPos, Activity.FID, FID);
		return;
	}
	if (!IsMutiActionType(Activity.ActivityType))
	{
		sipwarning("This is not MultiActivity. FID=%d", FID);
		return;
	}

	if (ActPos != ACTPOS_MULTILANTERN)	// ACTPOS_MULTILANTERN처리는 여기서 하지 않고 M_CS_MULTIACT_START에서 한다.
	{
		uint8	action_type = static_cast<uint8>(Activity.ActivityType);

		// 기록을 남긴다.
		uint32	VisitID = AddActionMemo(FID, action_type, 0, Activity.ActivityUsers[0].Pray, Activity.ActivityUsers[0].Secret, StoreLockID, pInvenPos, NULL, SyncCode, _tsid);
		if (VisitID != 0)
		{
			TServiceId	tsid_null(0);
			for (i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
			{
				uint32	JoinFID = Activity.ActivityUsers[i].FID;
				if(JoinFID == 0)
					continue;

				AddActionMemo(JoinFID, action_type, VisitID, Activity.ActivityUsers[i].Pray, Activity.ActivityUsers[i].Secret, Activity.ActivityUsers[i].StoreLockID, &Activity.ActivityUsers[i].InvenPos, NULL, Activity.ActivityUsers[i].SyncCode, tsid_null);
			}
		}
		else
		{
			sipwarning("VisitID = 0!!!");
		}
	}

	ActWaitCancel(FID, ActPos, _tsid, 1);
}

void CRoomWorld::on_M_CS_MULTIACT_CANCEL(T_FAMILYID FID, uint32 ChannelID, uint8 ActPos)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	// 아래의 코드는 제사지내기처리부이다.
	_RoomActivity_Family* pActivityUsers = FindJoinFID(ChannelID, ActPos, FID);
	if (pActivityUsers == NULL)
	{
		sipwarning("FindJoinFID = NULL. RoomID=%d, ChannelID=%d, ActPos=%d, JoinFID=%d. (Reason : already started or canceled?)", m_RoomID, ChannelID, ActPos, FID);
		return;
	}

	// 행사참가자들에게 M_SC_MULTIACT_CANCEL를 보낸다.
	ucstring	FName = L"";
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo != NULL)
		FName = pInfo->m_Name;
	uint32	zero = 0;
	for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (pActivityUsers[i].FID == 0 || pActivityUsers[i].FID == FID)
			continue;

		FAMILY*	pFamily = GetFamilyData(pActivityUsers[i].FID);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", pActivityUsers[i].FID);
			continue;
		}

		CMessage	msgOut(M_SC_MULTIACT_CANCEL);
		msgOut.serial(pActivityUsers[i].FID, zero, m_RoomID, ActPos, FID, FName);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
	}

	if (pActivityUsers[0].FID == FID)
	{
		// HostFID가 Cancel한 경우에는 행사를 취소한다.
		TServiceId	tmpsid(0);
		ActWaitCancel(FID, ActPos, tmpsid, 0);
	}
	else
	{
		int pos;
		for (pos = 1; pos < MAX_MULTIACT_FAMILY_COUNT; pos ++)
		{
			if(pActivityUsers[pos].FID == FID)
				break;
		}
		if (pos >= MAX_MULTIACT_FAMILY_COUNT)
		{
			sipwarning("Unknown Error!!!!!(13667)");
			return;
		}

		pActivityUsers[pos].init();

		CheckMultiactAllStarted(ChannelID, ActPos);
	}
}

void CRoomWorld::on_M_CS_MULTIACT_COMMAND(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID JoinFID, uint8 Command, uint32 Param)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", HostFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, HostFID);
		return;
	}

	// 아래의 코드는 제사지내기처리부이다.
	RoomChannelData	*pChannelData = GetFamilyChannel(HostFID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, HostFID);
		return;
	}

	int		index = (int)ActPos - 1;
	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	if ((!IsMutiActionType(Activity.ActivityType)) || (Activity.FID != HostFID))
	{
		sipwarning("Activity Type invalid or You are not current Host. ActivityType=%d, CurrentHostFID=%d, YourFID=%d", Activity.ActivityType, Activity.FID, HostFID);
		return;
	}

	for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		uint32	targetFID = Activity.ActivityUsers[i].FID;
		if((targetFID == 0) || (JoinFID != 0 && JoinFID != targetFID))
			continue;

		FAMILY*	pFamily = GetFamilyData(targetFID);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", targetFID);
			continue;
		}

		CMessage	msgOut(M_SC_MULTIACT_COMMAND);
		msgOut.serial(targetFID, ActPos, Command, Param);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
	}
}

void CRoomWorld::on_M_CS_MULTIACT_REPLY(T_FAMILYID JoinFID, uint8 ActPos, uint8 Command, uint32 Param)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", JoinFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, JoinFID);
		return;
	}

	// 아래의 코드는 제사지내기처리부이다.
	RoomChannelData	*pChannelData = GetFamilyChannel(JoinFID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, JoinFID);
		return;
	}

	int		index = (int)ActPos - 1;
	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	if ((!IsMutiActionType(Activity.ActivityType)) || (Activity.FID == 0) || (Activity.ActivityUsers[0].FID == 0))
	{
		sipwarning("Activity Type invalid. ActivityType=%d, %d, %d", Activity.ActivityType, Activity.FID, Activity.ActivityUsers[0].FID);
		return;
	}

	int i;
	for (i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if(Activity.ActivityUsers[i].FID == JoinFID)
			break;
	}
	if (i >= MAX_MULTIACT_FAMILY_COUNT)
	{
		sipwarning("You are not current Join. YourFID=%d", JoinFID);
		return;
	}

	FAMILY*	pFamily = GetFamilyData(Activity.FID);
	if (pFamily == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", Activity.FID);
		return;
	}

	CMessage	msgOut(M_SC_MULTIACT_REPLY);
	msgOut.serial(Activity.FID, ActPos, JoinFID, Command, Param);
	CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
}

bool CRoomWorld::CheckIfActSave(T_FAMILYID FID)
{
	FAMILY *pF = GetFamilyData(FID);
	if (pF == NULL)
		return true; // FID가 Offline일 때에는 비체계사용자라고 본다!!! 체계사용자는 특사(예약행사)아이템을 사용할수 없게 만듬. false;
	T_ACCOUNTID userid = pF->m_UserID;

	// 비체계사용자의 활동은 제한을 받지 않는다
	if (!IsSystemAccount(userid))
		return true;

	// 체계사용자의 활동은 체계공동 일반기념관에 영향을 줄수 없다
	if (m_RoomID >= SYSTEMROOM_START &&
		m_RoomID <= SYSTEMROOM_END)
		return false;

	// 체계사용자의 활동은 체계공동 종교구역에 영향을 줄수 없다
	if (m_RoomID >= MIN_RELIGIONROOMID &&
		m_RoomID <= MAX_RELIGIONROOMID)
		return false;

	// 체계사용자의 활동은 주인이 체계사용자인 기념관에 영향을 주어도 된다
	if (IsSystemAccount(m_MasterUserID))
		return true;

	// 체계사용자의 활동은 기타 다른 기념관에 영향을 줄수 없다
	return false;
}

void CRoomWorld::DoEverydayWorkInRoom()
{
	// 행사결과물정보를 초기화한다.
	m_ActionResults.clearPrevData();

	// 방의 행운출현상태를 초기화한다.
	m_LuckID_Lv3 = 0;
	for(RoomChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		pChannelData->ResetMountAnimalInfo();
	}
	m_LuckID_Lv4 = 0;

	// 방창조날인가를 검사한다.
	CheckRoomcreateHoliday();

	// 깜짝이벤트를 설정한다.
	SetRoomEvent();

	// 방에 물고기먹이를 준 회수 초기화
	m_nFishFoodCount = 0;
}



//==============================================================
//  조상탑기능
//==============================================================
void CRoomWorld::SendAncestorData(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	// 조상탑의 고인정보
	{
		for (uint8 i = 0; i < m_nAncestorDeceasedCount; i ++)
		{
			CMessage	msgOut(M_SC_ANCESTOR_DECEASED);
			uint32	zero = 0;
			msgOut.serial(FID, zero);
			ANCESTOR_DECEASED	&data = m_AncestorDeceased[i];
			msgOut.serial(data.m_AncestorIndex, data.m_DeceasedID, data.m_Surnamelen, data.m_Name, data.m_Sex);
			msgOut.serial(data.m_Birthday, data.m_Deadday, data.m_His, data.m_Domicile, data.m_Nation, data.m_PhotoId, data.m_PhotoType);
			CUnifiedNetwork::getInstance()->send(sid, msgOut);
		}
	}

	// 조상탑 Text - 조상탑 고인정보보다 후에 보내야 한다. Client에서는 이 packet를 받으면 조상탑 고인정보를 다 받은것으로 인정한다.
	{
		CMessage	msgOut(M_SC_ANCESTOR_TEXT);
		uint32	zero = 0;
		msgOut.serial(FID, zero);
		msgOut.serial(m_AncestorText);
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}
}

void CRoomWorld::SendAncestorTextToAll()
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ANCESTOR_TEXT)
		msgOut.serial(m_AncestorText);
	FOR_END_FOR_FES_ALL()
}

void CRoomWorld::SendAncestorDeceasedToAll(int i)
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ANCESTOR_DECEASED)
	{
		ANCESTOR_DECEASED	&data = m_AncestorDeceased[i];
		msgOut.serial(data.m_AncestorIndex, data.m_DeceasedID, data.m_Surnamelen, data.m_Name, data.m_Sex);
		msgOut.serial(data.m_Birthday, data.m_Deadday, data.m_His, data.m_Domicile, data.m_Nation, data.m_PhotoId, data.m_PhotoType);
	}
	FOR_END_FOR_FES_ALL()
}

// Set Ancestor Text ([u:Text])
void cb_M_CS_ANCESTOR_TEXT_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_ANCESTOR_TEXT_SET(FID, _msgin);
}

void CRoomWorld::on_M_CS_ANCESTOR_TEXT_SET(T_FAMILYID FID, SIPNET::CMessage &_msgin)
{
	// 방주인검사
	if (m_MasterID != FID)
	{
		sipwarning("You are not RoomMaster! RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	T_ROOMNAME		text;		NULLSAFE_SERIAL_WSTR(_msgin, text, MAX_LEN_ANCESTOR_TEXT, FID);

	// DB에서 설정
	MAKEQUERY_SaveAncestorText(strQ, m_RoomID, text);
	DBCaller->execute(strQ);

	// 메모리에서 변경
	m_AncestorText = text;

	// 현재 방의 사용자들에게 통지
	SendAncestorTextToAll();
}

// Set Ancestor Deceased Info ([b:AncestorID][b:DeadID][b:SurName][u:Name][b:SexID][d:BirthDate][d:DeadDate][u:BriefHistory][u:Domicile][u:Nation][d:PhotoID][b:PhotoType])
void cb_M_CS_ANCESTOR_DECEASED_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_ANCESTOR_DECEASED_SET(FID, _msgin);
}

void CRoomWorld::on_M_CS_ANCESTOR_DECEASED_SET(T_FAMILYID FID, SIPNET::CMessage &_msgin)
{
	// 방주인검사
	if (m_MasterID != FID)
	{
		sipwarning("You are not RoomMaster! RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	uint8	_AncestorIndex;		_msgin.serial(_AncestorIndex);
	uint8	_DeceasedID;		_msgin.serial(_DeceasedID);
	uint8	_Surnamelen;		_msgin.serial(_Surnamelen);
	T_FAMILYNAME	_Name;		NULLSAFE_SERIAL_WSTR(_msgin, _Name, MAX_LEN_FAMILY_NAME, FID);
	uint8	_Sex;				_msgin.serial(_Sex);
	uint32	_Birthday;			_msgin.serial(_Birthday);
	uint32	_Deadday;			_msgin.serial(_Deadday);
	ucstring	_His;			NULLSAFE_SERIAL_WSTR(_msgin, _His, MAX_LEN_DEAD_BFHISTORY, FID);
	ucstring	_Domicile;		NULLSAFE_SERIAL_WSTR(_msgin, _Domicile, MAX_LEN_DOMICILE, FID);
	ucstring	_Nation;		NULLSAFE_SERIAL_WSTR(_msgin, _Nation, MAX_LEN_NATION, FID);
	uint32	_PhotoId;			_msgin.serial(_PhotoId);
	uint8	_PhotoType;			_msgin.serial(_PhotoType);

	int		i = 0;
	for (i = 0; i < m_nAncestorDeceasedCount; i ++)
	{
		if ((_AncestorIndex == m_AncestorDeceased[i].m_AncestorIndex) && (_DeceasedID == m_AncestorDeceased[i].m_DeceasedID))
			break;
	}
	if (i >= m_nAncestorDeceasedCount)
	{
		if (m_nAncestorDeceasedCount >= MAX_ANCESTOR_DECEASED_COUNT * 2)
		{
			sipwarning("m_nAncestorDeceasedCount >= MAX_ANCESTOR_DECEASED_COUNT * 2!!! RoomID=%d", m_RoomID);
			return;
		}
		m_nAncestorDeceasedCount ++;
	}

	// DB에서 설정
	MAKEQUERY_SaveAncestorDeceased(strQ, m_RoomID, _AncestorIndex, _DeceasedID, _Surnamelen, _Name, _Sex, 
		_Birthday, _Deadday, _His, _Domicile, _Nation, _PhotoId, _PhotoType);
	DBCaller->execute(strQ);

	// 메모리에서 변경
	m_AncestorDeceased[i].init(_AncestorIndex, _DeceasedID, _Surnamelen, _Name, _Sex, 
		_Birthday, _Deadday, _His, _Domicile, _Nation, _PhotoId, _PhotoType);

	// 현재 방의 사용자들에게 통지
	SendAncestorDeceasedToAll(i);
}

//==============================================================
//  AutoPlay기능
//==============================================================
// Request AutoPlay Start ([d:SyncCode][w:InvenPos(AutoPlayItem)][b:AutoPlayCount][ [w:InvenPos(ActionItem,종합아이템)] ])
void cb_M_CS_AUTOPLAY_REQ_BEGIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	{
		// 체계사용자는 Autoplay를 보낼수 없게 제한한다!!!
		// 왜냐하면 Offline상태검사때문이다. CheckIfActSave함수에서 Offline일 때 return true를 넣기 위해서이다.
		FAMILY *pF = GetFamilyData(FID);
		if (pF == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", FID);
			return;
		}

		T_ACCOUNTID userid = pF->m_UserID;
		if (IsSystemAccount(userid))
		{
			sipwarning("System account can't use Autoplay. FID=%d, UID=%d", FID, userid);
			return;
		}
	}

	// 사용자정보 얻기
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	// 현재 AutoPlay 파케트를 받는중이면;
	if (pInfo->m_AutoPlayCount != 0)
	{
		sipwarning("pInfo->m_AutoPlayCount != 0!! pInfo->m_AutoPlayCount=%d", pInfo->m_AutoPlayCount);
		return;
	}

	_msgin.serial(pInfo->m_AutoPlaySyncCode, pInfo->m_AutoPlayInvenPos[0]);
	if (!IsValidInvenPos(pInfo->m_AutoPlayInvenPos[0]))
	{
		ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos[0]. FID=%d, InvenPos=%d", FID, pInfo->m_AutoPlayInvenPos[0]);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	uint8	Count;	_msgin.serial(Count);
	if ((Count < 1) || (Count > MAX_AUTOPLAY_ROOM_COUNT * 2))
	{
		ucstring ucUnlawContent = SIPBASE::_toString("Count > MAX_AUTOPLAY_ROOM_COUNT * 2!!! Count=%d", Count);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	for (int i = 1; i <= Count; i ++)
	{
		_msgin.serial(pInfo->m_AutoPlayInvenPos[i]);
		if (!IsValidInvenPos(pInfo->m_AutoPlayInvenPos[i]))
		{
			ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos[%d]. FID=%d, InvenPos=%d", i, FID, pInfo->m_AutoPlayInvenPos[i]);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
	}

	_InvenItems	tempInven;
	tempInven.CopyFrom(&pInfo->m_InvenItems);

	// Autoplay아이템을 검사한다.
	int	InvenIndex = GetInvenIndexFromInvenPos(pInfo->m_AutoPlayInvenPos[0]);
	ITEM_INFO	&item = pInfo->m_InvenItems.Items[InvenIndex];
	const MST_ITEM	*pmst_item = GetMstItemInfo(item.ItemID);
	if (pmst_item == NULL)
	{
		sipwarning("GetMstItemInfo = NULL. InvenPos=%d, ItemID=%d", pInfo->m_AutoPlayInvenPos[0], item.ItemID);
		return;
	}
	if (pmst_item->TypeID != ITEMTYPE_ACTDELEGATE)
	{
		ucstring ucUnlawContent = SIPBASE::_toString("pmst_item->TypeID != ITEMTYPE_ACTDELEGATE. InvenPos=%d, ItemID=%d", pInfo->m_AutoPlayInvenPos[0], item.ItemID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	tempInven.Items[InvenIndex].ItemCount --;

	// 행사용아이템들 개수만 검사한다.
	for (int i = 1; i <= Count; i ++)
	{
		InvenIndex = GetInvenIndexFromInvenPos(pInfo->m_AutoPlayInvenPos[i]);
		if (tempInven.Items[InvenIndex].ItemCount <= 0)
		{
			// 오유 로그 출력
			{
				for (int j = 0; j <= Count; j ++)
				{
					int InvenIndex1 = GetInvenIndexFromInvenPos(pInfo->m_AutoPlayInvenPos[j]);
					sipwarning("Invalid Autoplay ItemCount j=%d, InvenPos=%d, ItemID=%d, Count=%d, ErrIndex=%d", j, pInfo->m_AutoPlayInvenPos[j], pInfo->m_InvenItems.Items[InvenIndex1].ItemID, pInfo->m_InvenItems.Items[InvenIndex1].ItemCount, i);
				}
			}
			ucstring ucUnlawContent = SIPBASE::_toString("Invalid Autoplay ItemCount!!!, InvenIndex=%d", InvenIndex);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		tempInven.Items[InvenIndex].ItemCount --;
	}

	// 검사 OK
	pInfo->m_AutoPlayCount = Count;
	pInfo->m_AutoPlayCurCount = 0;
	pInfo->m_AutoPlay2Count = 0;
}

static uint32 s_TeshiKarNpcItem[4] = {700155, 700156, 700155, 700156};
// Send AutoPlay Data ([d:RoomID][b:ActPos][u:Pray][b:Secret])
void cb_M_CS_AUTOPLAY_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	T_ROOMID	RoomID;
	uint8		ActPos, SecretFlag;
	ucstring	Pray;

	_msgin.serial(FID, RoomID, ActPos);
	NULLSAFE_SERIAL_WSTR(_msgin, Pray, MAX_LEN_ROOM_PRAYTEXT, FID);
	_msgin.serial(SecretFlag);

	// 사용자정보 얻기
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	// 현재 파케트를 받는 상태가 아니면
	if (pInfo->m_AutoPlayCount == 0)
	{
		sipwarning("pInfo->m_AutoPlayCount == 0. FID=%d", FID);
		return;
	}

	_InvenItems	*pInven = &pInfo->m_InvenItems;
	// 검사
	int InvenIndex = GetInvenIndexFromInvenPos(pInfo->m_AutoPlayInvenPos[pInfo->m_AutoPlayCurCount + 1]);
	ITEM_INFO	&item = pInven->Items[InvenIndex];
	OUT_TAOCAN_FOR_AUTOPLAY*	pTaocanItemInfo = GetTaocanItemInfo(item.ItemID);
	if (pTaocanItemInfo == NULL)
	{
		sipwarning("Invalid Taocan Item. InvenPos=%d", pInfo->m_AutoPlayInvenPos[pInfo->m_AutoPlayCurCount + 1]);
		pInfo->m_AutoPlayCount = 0;
		return;
	}

	uint32	isXDG = 0;
	if (ActPos == ACTPOS_YISHI)
	{
		if (pTaocanItemInfo->TaocanType == SACRFTYPE_TAOCAN_ROOM)
		{
		}
		else if (pTaocanItemInfo->TaocanType == SACRFTYPE_TAOCAN_ROOM_XDG)
		{
			isXDG = 1;
		}
		else
		{
			sipwarning("pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_ROOM!!!");
			pInfo->m_AutoPlayCount = 0;
			return;
		}
	}
	else if (ActPos == ACTPOS_XIANBAO)
	{
		if (pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_XIANBAO)
		{
			sipwarning("pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_XIANBAO!!!");
			pInfo->m_AutoPlayCount = 0;
			return;
		}
	}
	else
	{
		sipwarning("Invalid ActPos!! ActPos=%d", ActPos);
		pInfo->m_AutoPlayCount = 0;
		return;
	}

	pInfo->m_AutoPlayRequestList[pInfo->m_AutoPlayCurCount].init(RoomID, ActPos, isXDG, Pray, SecretFlag);

	pInfo->m_AutoPlayCurCount ++;
	if (pInfo->m_AutoPlayCurCount < pInfo->m_AutoPlayCount)
		return;

	// 자료받기 끝, AutoPlay처리
	{
		std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = pInfo->m_mapCH.begin();
		if (it == pInfo->m_mapCH.end())
		{
			sipwarning("it == pInfo->m_mapCH.end()!!!");
			return;
		}
		T_CHARACTERID			character_id = it->first;
		CHARACTER	*character = GetCharacterData(character_id);
		if (character == NULL)
		{
			sipwarning("GetCharacterData = NULL. FID=%d, CHID=%d", FID, character_id);
			return;
		}

		// 착용복장 검사
		uint32	DressID = character->m_DefaultDressID;
		for (int j = 0; j < MAX_INVEN_NUM; j ++)
		{
			if ((pInven->Items[j].ItemID != 0) && (pInven->Items[j].UsedCharID == character_id))
			{
				DressID = pInven->Items[j].ItemID;
				break;
			}
		}

		// 인벤에서 아이템 삭제처리
		int		YishiCount = 0, XianbaoCount = 0;
		for (int i = 0; i <= pInfo->m_AutoPlayCount; i ++)
		{
			int InvenIndex = GetInvenIndexFromInvenPos(pInfo->m_AutoPlayInvenPos[i]);
			ITEM_INFO	&item = pInven->Items[InvenIndex];
			uint32		ItemID = item.ItemID;

			item.ItemCount --;
			if (item.ItemCount <= 0)
				item.ItemID = 0;

			const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
			if (mstItem == NULL)
			{
				sipwarning(L"GetMstItemInfo = NULL. InvenPos=%d, ItemID=%d", pInfo->m_AutoPlayInvenPos[i], item.ItemID);
				continue;
			}

			DBLOG_ItemUsed(DBLSUB_USEITEM_AUTOPLAY, FID, 0, ItemID, 1, mstItem->Name, "", 0);

			if (i == 0) // AutoPlay 아이템
				continue;

			// 행사용 Taocan(종합)아이템
			int i1 = i - 1;

			OUT_TAOCAN_FOR_AUTOPLAY*	pTaocanItemInfo = GetTaocanItemInfo(ItemID);

			// AutoPlay등록처리
			{
				CMessage	msgOut(M_NT_AUTOPLAY_REGISTER);
				msgOut.serial(pInfo->m_AutoPlayRequestList[i1].RoomID, pInfo->m_AutoPlayRequestList[i1].ActPos, pInfo->m_AutoPlayRequestList[i1].IsXDG, FID, pInfo->m_Name);
				msgOut.serial(character->m_ModelId, DressID, character->m_FaceID, character->m_HairID);
				msgOut.serial(pInfo->m_AutoPlayRequestList[i1].Pray, pInfo->m_AutoPlayRequestList[i1].SecretFlag);
				int item_count = 6;
				if (pInfo->m_AutoPlayRequestList[i1].ActPos == ACTPOS_YISHI)
				{
					item_count = 9;
					YishiCount ++;
				}
				else
				{
					XianbaoCount ++;
				}
				for (int j = 0; j < item_count; j ++)
					msgOut.serial(pTaocanItemInfo->ItemIDs[j]);
				msgOut.serial(ItemID, item.MoneyType);
				CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
			}
		}
		uint32		ItemID0 = 0;
		uint8		MoneyType0 = 0;
		for (int i = 0; i < pInfo->m_AutoPlay2Count; i ++)
		{
			int i1 = i + pInfo->m_AutoPlayCount;

			// AutoPlay등록처리
			{
				CMessage	msgOut(M_NT_AUTOPLAY_REGISTER);
				msgOut.serial(pInfo->m_AutoPlayRequestList[i1].RoomID, pInfo->m_AutoPlayRequestList[i1].ActPos, pInfo->m_AutoPlayRequestList[i1].IsXDG, FID, pInfo->m_Name);
				msgOut.serial(character->m_ModelId, DressID, character->m_FaceID, character->m_HairID);
				msgOut.serial(pInfo->m_AutoPlayRequestList[i1].Pray, pInfo->m_AutoPlayRequestList[i1].SecretFlag);
				for (int j = 0; j < 4; j ++)
					msgOut.serial(s_TeshiKarNpcItem[j]);
				msgOut.serial(ItemID0, MoneyType0);
				CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
			}
		}
		if (!SaveFamilyInven(FID, pInven))
		{
			sipwarning("SaveFamilyInven failed. FID=%d", FID);
		}
		if ((pInfo->m_AutoPlaySyncCode > 0) && (_tsid.get() != 0))
		{
			CMessage msgOut(M_SC_INVENSYNC);
			uint32		ret = ERR_NOERR;
			msgOut.serial(FID, pInfo->m_AutoPlaySyncCode, ret);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}

		// Family경험값 변경은 여기서 하며, Room경험값변경은 M_NT_ADDACTIONMEMO에서 한다.
//		int	nSaveDBFlag = 0;
		for (int i = 0; i < YishiCount; i ++)
		{
			// nSaveDBFlag |= ChangeFamilyExp(Family_Exp_Type_Jisi_One, FID, 0, false);
			ChangeFamilyExp(Family_Exp_Type_Jisi_One, FID);
		}
		for (int i = 0; i < XianbaoCount; i ++)
		{
			// nSaveDBFlag |= ChangeFamilyExp(Family_Exp_Type_Xianbao, FID, 0, false);
			ChangeFamilyExp(Family_Exp_Type_Xianbao, FID);
		}
		for (int i = 0; i < pInfo->m_AutoPlay2Count; i ++)
		{
			// nSaveDBFlag |= ChangeFamilyExp(Family_Exp_Type_Auto2, FID, 0, false);
			ChangeFamilyExp(Family_Exp_Type_Auto2, FID);
		}
		//if (nSaveDBFlag & 0x0F)
		//{
		//	if (!pInfo->SaveFamilyLevelAndExp())
		//	{
		//		sipwarning("SaveFamilyLevelAndExp failed. FID=%d", FID);
		//	}
		//}
		//if (nSaveDBFlag & 0xF0)
		//{
		//	if (!pInfo->SaveFamilyExpHistory())
		//	{
		//		sipwarning("SaveFamilyExpHistory failed. FID=%d", FID);
		//	}
		//}

		// 초기화
		pInfo->m_AutoPlayCount = 0;
		pInfo->DonActInRoom(ROOMACT_TESHIITEM);
	}
}

// Send AutoPlay2 Data ([d:SyncCode][w:InvenPos(AutoPlay2Item)][b:AutoPlay2Count][[d:RoomID]])
void cb_M_CS_AUTOPLAY2_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		SyncCode;
	uint16		InvenPos;
	uint8		Count;

	_msgin.serial(FID, SyncCode, InvenPos, Count);

	{
		// 체계사용자는 Autoplay를 보낼수 없게 제한한다!!!
		// 왜냐하면 Offline상태검사때문이다. CheckIfActSave함수에서 Offline일 때 return true를 넣기 위해서이다.
		FAMILY *pF = GetFamilyData(FID);
		if (pF == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", FID);
			return;
		}

		T_ACCOUNTID userid = pF->m_UserID;
		if (IsSystemAccount(userid))
		{
			sipwarning("System account can't use Autoplay. FID=%d, UID=%d", FID, userid);
			return;
		}
	}

	if (Count > MAX_AUTOPLAY_ROOM_COUNT)
	{
		sipwarning("Count > MAX_AUTOPLAY_ROOM_COUNT. FID=%d, Count=%d", FID, Count);
		return;
	}

	// 사용자정보 얻기
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	_InvenItems	*pInven = &pInfo->m_InvenItems;
	// 검사
	if (!IsValidInvenPos(InvenPos))
	{
		sipwarning("Invalid InvenPos. FID=%d, InvenPos=%d", FID, InvenPos);
		return;
	}
	int InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
	ITEM_INFO	&item = pInven->Items[InvenIndex];
	uint32		ItemID = item.ItemID;
	const MST_ITEM	*pmst_item = GetMstItemInfo(ItemID);
	if (pmst_item == NULL)
	{
		sipwarning("GetMstItemInfo = NULL. FID=%d, InvenPos=%d, ItemID=%d", FID, InvenPos, ItemID);
		return;
	}
	if (pmst_item->TypeID != ITEMTYPE_ACTDELEGATE)
	{
		sipwarning("pmst_item->TypeID != ITEMTYPE_ACTDELEGATE. FID=%d, InvenPos=%d, ItemID=%d", FID, InvenPos, ItemID);
		return;
	}

	// OK, 처리
	uint8		ActPos = ACTPOS_AUTO2;
	ucstring	Pray = L"";
	uint8		SecretFlag = 0;

	// 爱心特使인 경우
	if (pInfo->m_AutoPlayCount > 0)
	{
		pInfo->m_AutoPlay2Count = Count;
		for (int j = 0; j < (int)Count; j ++)
		{
			uint32		RoomID;
			_msgin.serial(RoomID);
			pInfo->m_AutoPlayRequestList[pInfo->m_AutoPlayCount + j].init(RoomID, ActPos, 0, Pray, SecretFlag);
		}
		return;
	}

	// 天园特使인 경우
	std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = pInfo->m_mapCH.begin();
	if (it == pInfo->m_mapCH.end())
	{
		sipwarning("it == pInfo->m_mapCH.end()!!!");
		return;
	}
	T_CHARACTERID			character_id = it->first;
	CHARACTER	*character = GetCharacterData(character_id);
	if (character == NULL)
	{
		sipwarning("GetCharacterData = NULL. FID=%d, CHID=%d", FID, character_id);
		return;
	}

	// 착용복장 검사
	uint32	DressID = character->m_DefaultDressID;
	for (int j = 0; j < MAX_INVEN_NUM; j ++)
	{
		if ((pInven->Items[j].ItemID != 0) && (pInven->Items[j].UsedCharID == character_id))
		{
			DressID = pInven->Items[j].ItemID;
			break;
		}
	}

	// 인벤에서 아이템 삭제처리
	item.ItemCount --;
	if (item.ItemCount <= 0)
		item.ItemID = 0;

	DBLOG_ItemUsed(DBLSUB_USEITEM_AUTOPLAY, FID, 0, ItemID, 1, pmst_item->Name, "", 0);

	// AutoPlay등록처리
	uint32		ItemID0 = 0;
	uint8		MoneyType0 = 0;
	uint32		IsXDG = 0;
	for (int j = 0; j < (int)Count; j ++)
	{
		uint32		RoomID;
		_msgin.serial(RoomID);

		CMessage	msgOut(M_NT_AUTOPLAY_REGISTER);
		msgOut.serial(RoomID, ActPos, IsXDG, FID, pInfo->m_Name);
		msgOut.serial(character->m_ModelId, DressID, character->m_FaceID, character->m_HairID);
		msgOut.serial(Pray, SecretFlag);
		for (int j = 0; j < 4; j ++)
			msgOut.serial(s_TeshiKarNpcItem[j]);
		msgOut.serial(ItemID0, MoneyType0);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
	}

	if (!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
	}
	if ((SyncCode > 0) && (_tsid.get() != 0))
	{
		CMessage msgOut(M_SC_INVENSYNC);
		uint32		ret = ERR_NOERR;
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	// Family경험값 변경은 여기서 하며, Room경험값변경은 M_NT_ADDACTIONMEMO에서 한다.
//	int	nSaveDBFlag = 0;
	for (int i = 0; i < (int)Count; i ++)
	{
		// nSaveDBFlag |= ChangeFamilyExp(Family_Exp_Type_Auto2, FID, 0, false);
		ChangeFamilyExp(Family_Exp_Type_Auto2, FID);
	}
	//if (nSaveDBFlag & 0x0F)
	//{
	//	if (!pInfo->SaveFamilyLevelAndExp())
	//	{
	//		sipwarning("SaveFamilyLevelAndExp failed. FID=%d", FID);
	//	}
	//}
	//if (nSaveDBFlag & 0xF0)
	//{
	//	if (!pInfo->SaveFamilyExpHistory())
	//	{
	//		sipwarning("SaveFamilyExpHistory failed. FID=%d", FID);
	//	}
	//}

	pInfo->DonActInRoom(ROOMACT_TESHICARD);
}

// Set Auto character state ([d:AutoPlayerFID][d:X][d:Y][d:Z][d:direction][u:AnimationName][d:AniState][d:HoldItem])
//void cb_M_NT_AUTOPLAY_STATECH(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	AutoFID;
//	uint32	AniState, Dir, X, Y, Z, HoldItem;
//	ucstring AniName;
//	_msgin.serial(AutoFID, X, Y, Z, Dir, AniName, AniState, HoldItem);
//
//	CRoomWorld *roomManager = GetOpenRoomWorldFromFID(AutoFID);
//	if (roomManager == NULL)
//	{
//		sipwarning("GetOpenRoomWorldFromFID = NULL. AutoFID=%d", AutoFID);
//	}
//	else
//	{
//		roomManager->ChangeAutoCharSTATECH(AutoFID, X, Y, Z, Dir, AniName, AniState, HoldItem);
//	}
//}

//void CRoomWorld::ChangeAutoCharSTATECH(T_FAMILYID AutoFID, uint32 X, uint32 Y, uint32 Z, uint32 Dir, ucstring AniName, uint32 AniState, uint32 HoldItem)
//{
//	ChannelData	*pChannelData = GetChannel(1);	// 1번채널
//	if (pChannelData == NULL)
//	{
//		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=1", m_RoomID);
//		return;
//	}
//
//	{
//		for (std::vector<AutoPlayUserInfo>::iterator it = m_AutoPlayUserList.begin(); it != m_AutoPlayUserList.end(); it ++)
//		{
//			if (it->AutoFID == AutoFID)
//			{
////				it->State = State;
//				it->Dir = Dir;
//				it->X = X;
//				it->Y = Y;
//				it->Z = Z;
//				break;
//			}
//		}
//	}
//
//	// 방에 있는 사람들에게 통지
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_AUTO_STATECH)
//		msgOut.serial(AutoFID, X, Y, Z, Dir, AniName, AniState, HoldItem);
//	FOR_END_FOR_FES_ALL()
//}

// Move Auto character ([d:AutoPlayerFID][d:X][d:Y][d:Z])
//void cb_M_NT_AUTOPLAY_MOVECH(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	AutoFID;
//	uint32	X, Y, Z;
//	_msgin.serial(AutoFID, X, Y, Z);
//
//	CRoomWorld *roomManager = GetOpenRoomWorldFromFID(AutoFID);
//	if (roomManager == NULL)
//	{
//		sipwarning("GetOpenRoomWorldFromFID = NULL. AutoFID=%d", AutoFID);
//	}
//	else
//	{
//		roomManager->ChangeAutoCharMOVECH(AutoFID, X, Y, Z);
//	}
//}

//void CRoomWorld::ChangeAutoCharMOVECH(T_FAMILYID AutoFID, uint32 X, uint32 Y, uint32 Z)
//{
//	ChannelData	*pChannelData = GetChannel(1);	// 1번채널
//	if (pChannelData == NULL)
//	{
//		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=1", m_RoomID);
//		return;
//	}
//
//	{
//		for (std::vector<AutoPlayUserInfo>::iterator it = m_AutoPlayUserList.begin(); it != m_AutoPlayUserList.end(); it ++)
//		{
//			if (it->AutoFID == AutoFID)
//			{
//				it->X = X;
//				it->Y = Y;
//				it->Z = Z;
//				break;
//			}
//		}
//	}
//
//	// 방에 있는 사람들에게 통지
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_AUTO_MOVECH)
//		msgOut.serial(AutoFID, X, Y, Z);
//	FOR_END_FOR_FES_ALL()
//}

//// Auto Character animation [u:Method name][d:Parameter]
//void cb_M_NT_AUTOPLAY_CHMETHOD(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	AutoFID;
//	ucstring	MethodName;
//	uint32		Param;
//	_msgin.serial(AutoFID, MethodName, Param);
//
//	CRoomWorld *roomManager = GetOpenRoomWorldFromFID(AutoFID);
//	if (roomManager == NULL)
//	{
//		sipwarning("GetOpenRoomWorldFromFID = NULL. AutoFID=%d", AutoFID);
//	}
//	else
//	{
//		roomManager->ChangeAutoCharCHMETHOD(AutoFID, MethodName, Param);
//	}
//}

//void CRoomWorld::ChangeAutoCharCHMETHOD(T_FAMILYID AutoFID, ucstring MethodName, uint32 Param)
//{
//	ChannelData	*pChannelData = GetChannel(1);	// 1번채널
//	if (pChannelData == NULL)
//	{
//		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=1", m_RoomID);
//		return;
//	}
//
//	// 새가족의 입방을 통보한다
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_AUTO_CHMETHOD)
//		msgOut.serial(AutoFID, MethodName, Param);
//	FOR_END_FOR_FES_ALL()
//}

//// Auto Character holditem [d:ItemID]
//void cb_M_NT_AUTOPLAY_HOLDITEM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID	AutoFID;
//	uint32		ItemID;
//	_msgin.serial(AutoFID, ItemID);
//
//	CRoomWorld *roomManager = GetOpenRoomWorldFromFID(AutoFID);
//	if (roomManager == NULL)
//	{
//		sipwarning("GetOpenRoomWorldFromFID = NULL. AutoFID=%d", AutoFID);
//	}
//	else
//	{
//		roomManager->ChangeAutoCharHOLDITEM(AutoFID, ItemID);
//	}
//}

//void CRoomWorld::ChangeAutoCharHOLDITEM(T_FAMILYID AutoFID, uint32 ItemID)
//{
//	ChannelData	*pChannelData = GetChannel(1);	// 1번채널
//	if (pChannelData == NULL)
//	{
//		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=1", m_RoomID);
//		return;
//	}
//
//	// 새가족의 입방을 통보한다
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_AUTO_HOLDITEM)
//		msgOut.serial(AutoFID, ItemID);
//	FOR_END_FOR_FES_ALL()
//}

// Add new action memo AutoPlayService->OpenRoomService ([d:UnitID][d:RoomID][d:FID][b:ActPos][u:Pray][b:Secret][ [d:ItemID] ][d:UsedItemID(for RoomExp)][b:UsedItemMoneyType(for RoomExp)])
void cb_M_NT_ADDACTIONMEMO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	static	uint32	serial_key = 1;

	AutoPlayUnitInfo	data;
	T_FAMILYID			FID;

	_msgin.serial(data.UnitID, data.RoomID, FID, data.FamilyName, data.ActPos, data.IsXDG, data.Pray, data.SecretFlag);
	for (int i = 0; i < 9; i ++)
		_msgin.serial(data.ItemID[i]);
	_msgin.serial(data.UsedItemID, data.UsedItemMoneyType);

	uint32	serial_key_cur = serial_key;
	s_AutoPlayUnitInfoes[serial_key_cur] = data;
	serial_key ++;

	CRoomWorld *roomManager = GetOpenRoomWorld(data.RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", data.RoomID);
	}
	else
	{
		roomManager->AddAutoplayActionMemo(FID, serial_key_cur);
	}
}

static void	DBCB_DBAddVisitMemo1(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		VisitID	= *(uint32*)argv[0];
	uint32		autoplay_key	= *(uint32*)argv[1];
	uint32		FID				= *(uint32*)argv[2];

	if (VisitID == (uint32)(-1))
	{
		sipwarning("Invalid VisitID.");
		return;
	}

	std::map<uint32, AutoPlayUnitInfo>::iterator it = s_AutoPlayUnitInfoes.find(autoplay_key);
	if (it == s_AutoPlayUnitInfoes.end())
	{
		sipwarning("it == s_AutoPlayUnitInfoes.end()!! FID=%d, key=%d", FID, autoplay_key);
		return;
	}

	// OK통지
	{
		uint32	Param = 0;
		CMessage	msgOut(M_NT_ADDACTIONMEMO_OK);
		msgOut.serial(it->second.UnitID, Param);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
	}

	CRoomWorld *roomManager = GetOpenRoomWorld(it->second.RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. it->second.RoomID=%d", it->second.RoomID);
	}
	else
	{
		// CheckIfActSave 함수는 FID가 현재 로그인되여있는 경우만 유용하다.
		// 그러므로 사용자가 Auto설정을 한 후 빨리 로그오프한 경우에는 기록이 남지 않을수도 있다.
		// 현재 Autoplay행사기록은 Autoplay를 설정한 후 실지 행사를 하기 전에 하도록 되여있다.
		bool	bActSave = roomManager->CheckIfActSave(FID);
		if (bActSave)
		{
			// 행사결과물 처리
			if (it->second.ActPos == ACTPOS_XIANBAO)
			{
				int xianbao_result_type = XianbaoResultType_Large; // Auto는 세트아이템으로만 할수 있으므로 대형으로 처리한다.
				roomManager->m_ActionResults.registerXianbaoResult(xianbao_result_type, VisitID, FID, it->second.FamilyName);
			}
			else if (it->second.ActPos == ACTPOS_YISHI)
			{
				int yishi_result_type = YishiResultType_Large; // Auto는 세트아이템으로만 할수 있으므로 대형으로 처리한다.
				if (it->second.IsXDG == 1)
					yishi_result_type = YishiResultType_XDG;	// XDG아이템인 경우
				roomManager->m_ActionResults.registerYishiResult(yishi_result_type, VisitID, FID, it->second.FamilyName, true);
			}
			else if (it->second.ActPos == ACTPOS_AUTO2)
			{
				roomManager->m_ActionResults.registerAuto2Result(VisitID, FID, it->second.FamilyName);
			}

			// 방의 경험값 변경
			if (it->second.ActPos == ACTPOS_XIANBAO)
				roomManager->ChangeRoomExp(Room_Exp_Type_Xianbao, FID);
			else if (it->second.ActPos == ACTPOS_YISHI)
				roomManager->ChangeRoomExp(Room_Exp_Type_Jisi_One, FID);
			else if (it->second.ActPos == ACTPOS_AUTO2)
				roomManager->ChangeRoomExp(Room_Exp_Type_Auto2, FID);

			if (it->second.ActPos != ACTPOS_AUTO2)	// 천원카드인 경우에는 아이템소비가 되지 않는다.
			{
				const MST_ITEM *mstItem = GetMstItemInfo(it->second.UsedItemID);
				if (mstItem == NULL)
				{
					sipwarning(L"GetMstItemInfo = NULL. it->second.UsedItemID=%d", it->second.UsedItemID);
				}
				else
				{
					uint32	UsedUMoney = 0, UsedGMoney = 0;
					if (it->second.UsedItemMoneyType == MONEYTYPE_UMONEY)
						UsedUMoney += mstItem->UMoney;
					else
						UsedGMoney += mstItem->GMoney;

					if ((UsedUMoney != 0) || (UsedGMoney != 0))
					{
						ucstring itemInfo = L"[" + mstItem->Name + L"]1";
						roomManager->ChangeRoomExp(Room_Exp_Type_UseMoney, FID, UsedUMoney, UsedGMoney, itemInfo);
					}
				}
			}
		}
	}

	s_AutoPlayUnitInfoes.erase(it);
}

void AddAutoplayActionMemoAfter(uint32 RoomID, uint32 FID, uint32 autoplay_key, uint32 nCurActionGroup)
{
	std::map<uint32, AutoPlayUnitInfo>::iterator it = s_AutoPlayUnitInfoes.find(autoplay_key);
	if (it == s_AutoPlayUnitInfoes.end())
	{
		sipwarning("it == s_AutoPlayUnitInfoes.end()!! FID=%d, key=%d", FID, autoplay_key);
		return;
	}

	int		item_count = 1;
	uint8	action_type;

	if (it->second.ActPos == ACTPOS_XINGLI_1)
	{
		// 예약행사: 물주기, 꽃드리기, 길상등드리기, 물고기먹이주기 - ACTPOS_XINGLI_1로 일괄처리한다.
		CRoomWorld *roomManager = GetOpenRoomWorld(it->second.RoomID);
		if (roomManager == NULL)
		{
			sipwarning("GetOpenRoomWorld = NULL. FID=%d, RoomID=%d", FID, it->second.RoomID);
		}
		else
		{
			uint32	Param = roomManager->YuyueYishiRegActionMemo(FID, it->second.FamilyName, it->second.UsedItemID, it->second.ItemID[0], nCurActionGroup);

			// 처리완료
			{
				CMessage	msgOut(M_NT_ADDACTIONMEMO_OK);
				msgOut.serial(it->second.UnitID, Param);
				CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
			}
		}
		s_AutoPlayUnitInfoes.erase(it);
		return;
	}

	if (it->second.ActPos == ACTPOS_YISHI)
	{
//		item_count = 9;
		if (it->second.IsXDG == 0)
			action_type = AT_SIMPLERITE;
		else
			action_type = AT_XDGRITE;
	}
	else if (it->second.ActPos == ACTPOS_AUTO2)
	{
		action_type = AT_AUTO2;
	}
	else	// ACTPOS_XIANBAO
	{
//		item_count = 6;
		action_type = AT_BURNING;
	}

	char str_buf[80];
	sprintf((&str_buf[0]), ("%02x"), item_count);
	for (int i = 0; i < item_count; i ++)
	{
		uint32	item_id = it->second.UsedItemID; //  it->second.ItemID[i];
		sprintf((&str_buf[i * 8 + 2]), ("%02x"), (item_id) & 0xFF);
		sprintf((&str_buf[i * 8 + 4]), ("%02x"), (item_id >> 8) & 0xFF);
		sprintf((&str_buf[i * 8 + 6]), ("%02x"), (item_id >> 16) & 0xFF);
		sprintf((&str_buf[i * 8 + 8]), ("%02x"), (item_id >> 24) & 0xFF);
	}
	str_buf[item_count * 8 + 2] = 0;

	uint8	secret = it->second.SecretFlag | 0x10;	// Auto는 secret에 0x10으로 표시한다.
	
	tchar strQuery[1024] = _t("");
	if (IsSystemRoom(RoomID))
	{
		MAKEQUERY_AddVisitMemo1_Aud(strQ, nCurActionGroup, action_type, it->second.Pray, secret, str_buf);
		wcscpy(strQuery, strQ);
	}
	else
	{
		MAKEQUERY_AddVisitMemo1(strQ, nCurActionGroup, action_type, it->second.Pray, secret, str_buf);
		wcscpy(strQuery, strQ);
	}
	DBCaller->executeWithParam(strQuery, DBCB_DBAddVisitMemo1,
		"DDD",
		OUT_,		NULL,
		CB_,		autoplay_key,
		CB_,		FID);
}

static void	DBCB_DBShrd_Visit_Insert_ForAutoplay(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		nCurActionGroup	= *(uint32*)argv[0];
	uint32		isInc			= *(uint32*)argv[1];
	uint32		FID				= *(uint32*)argv[2];
	uint32		autoplay_key	= *(uint32*)argv[3];
	uint32		RoomID			= *(uint32*)argv[4];

	if (nCurActionGroup == 0 || nCurActionGroup == (uint32)(-1))
	{
		sipwarning(L"DB_EXE_PARAMQUERY return error failed in Shrd_Visit_Insert. nCurActionGroup=%d", nCurActionGroup);
		return;
	}

	SetActionGroupID(RoomID, FID, nCurActionGroup);
	if (isInc > 0)
	{
		CRoomWorld	*pRoomWorld = GetOpenRoomWorld(RoomID);
		if (pRoomWorld == NULL)
		{
			sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
			return;
		}

		pRoomWorld->m_Visits ++;
//		sipdebug("Roomid=%d, Visits=%d", RoomID, pRoomWorld->m_Visits);
	}

	AddAutoplayActionMemoAfter(RoomID, FID, autoplay_key, nCurActionGroup);
}

void CRoomWorld::AddAutoplayActionMemo(T_FAMILYID FID, uint32 autoplay_key)
{
	uint32	nCurActionGroup = GetActionGroupID(m_RoomID, FID);
	if (nCurActionGroup == 0)
	{
		MAKEQUERY_Shrd_Visit_Insert(strQ, m_RoomID, FID);
		DBCaller->executeWithParam(strQ, DBCB_DBShrd_Visit_Insert_ForAutoplay,
			"DDDDD",
			OUT_,					NULL,
			OUT_,					NULL,
			CB_,					FID,
			CB_,					autoplay_key,
			CB_,					m_RoomID);
	}
	else
	{
		AddAutoplayActionMemoAfter(m_RoomID, FID, autoplay_key, nCurActionGroup);
	}
}

// Request AutoPlay, AutoPlayService->OpenRoomService ([d:RoomID][d:AutoPlayerFID][b:ActPos])
void cb_M_NT_AUTOPLAY_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID	RoomID;
	T_FAMILYID	AutoFID, FID;
	uint8		ActPos;

	_msgin.serial(RoomID, AutoFID, ActPos, FID);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		roomManager->AutoPlayRequest(AutoFID, ActPos, FID, _tsid);
	}
}

void CRoomWorld::AutoPlayRequest(T_FAMILYID AutoFID, uint8 ActPos, T_FAMILYID FID, TServiceId _tsid)
{
	int		index = (int)ActPos - 1;

	RoomChannelData	*pChannelData = GetChannel(1);	// 1번채널
	if (pChannelData == NULL) // by krs
	{
		//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=1", m_RoomID);
		pChannelData = GetChannel(5001);
		if (pChannelData == NULL)
		{
			//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=5001", m_RoomID);
			return;
		}
	}

	uint32	ret = 0;

	_RoomActivity	&Activity = pChannelData->m_Activitys[index];
	bool	bAble = true;
	if (ActPos == ACTPOS_YISHI || ActPos == ACTPOS_AUTO2)
	{
		if (pChannelData->m_Activitys[ACTPOS_YISHI-1].FID != 0 ||
			pChannelData->m_Activitys[ACTPOS_AUTO2-1].FID != 0 ||
			IsRoomMusicEventDoing())
		{
			sipdebug("AutoPlayRequest failed:AutoFID=%d, yishiFID=%d, auto2FID=%d, RoomID=%d, Actpos=%d, FID=%d, RoomEventIndex=%d", 
				AutoFID, pChannelData->m_Activitys[ACTPOS_YISHI-1].FID, pChannelData->m_Activitys[ACTPOS_AUTO2-1].FID,
				m_RoomID, ActPos, FID, GetCurrentDoingRoomEventIndex());
			bAble = false;
		}
	}
	else
	{
		if (Activity.FID != 0)
		{
			sipdebug("AutoPlayRequest failed:AutoFID=%d, ActivityFID=%d, RoomID=%d, Actpos=%d, FID=%d", 
				AutoFID, Activity.FID, m_RoomID, ActPos, FID);
			bAble = false;
		}
	}

	if (bAble)
	{
		// 행사시작대기상태임
		Activity.init(AutoFID,AT_AUTO2,0,0);
	}
	else
	{
		ret = 1;
	}

	CMessage	msgOut(M_NT_AUTOPLAY_ANS);
	msgOut.serial(AutoFID, ret, m_RoomID, m_SceneID, m_Level);
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

// Start AutoPlay, AutoPlayService->OpenRoomService ([d:RoomID][d:AutoPlayerFID][b:ActPos][d:FID][u:FName][d:ModelID][d:DressID][d:FaceID][d:HairID][d:state][d:dir][d:X][d:Y][d:Z])
void cb_M_NT_AUTOPLAY_START(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID		RoomID;
	T_FAMILYID		AutoFID, FID;
	uint8			ActPos;
	T_FAMILYNAME	FamilyName;
	T_MODELID		ModelID;
	T_DRESS			DressID;
	T_FACEID		FaceID;
	T_HAIRID		HairID;

	_msgin.serial(RoomID, AutoFID, ActPos, FID, FamilyName, ModelID, DressID, FaceID, HairID);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		roomManager->AutoPlayStart(AutoFID, ActPos, FID, FamilyName, ModelID, DressID, FaceID, HairID, _tsid);
	}
}

void CRoomWorld::AutoPlayStart(T_FAMILYID AutoFID, uint8 ActPos, T_FAMILYID FID, T_FAMILYNAME FamilyName, 
		T_MODELID ModelID, T_DRESS DressID, T_FACEID FaceID, T_HAIRID HairID, SIPNET::TServiceId _tsid)
{
	ChannelData	*pChannelData = GetChannel(1);	// 1번채널
	if (pChannelData == NULL) // by krs
	{
		//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=1", m_RoomID);
		pChannelData = GetChannel(5001);
		if (pChannelData == NULL)
		{
			//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=5001", m_RoomID);
			return;
		}
	}	

	s_AutoPlayerRoomID[AutoFID] = m_RoomID;

	AutoPlayUserInfo	data(AutoFID, ActPos, FID, FamilyName, ModelID, DressID, FaceID, HairID);
	m_AutoPlayUserList.push_back(data);
	
	// AutoPlayer가 들어오는것을 통보한다
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_AUTO_ENTER)
		msgOut.serial(AutoFID, FID, FamilyName, ModelID, DressID, FaceID, HairID);
	FOR_END_FOR_FES_ALL()
}

// End AutoPlay, AutoPlayService->OpenRoomService ([d:RoomID][d:AutoPlayerFID][b:ActPos])
void cb_M_NT_AUTOPLAY_END(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID		RoomID;
	T_FAMILYID		AutoFID;
	uint8			ActPos;

	_msgin.serial(RoomID, AutoFID, ActPos);

	CRoomWorld *roomManager = GetOpenRoomWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. RoomID=%d", RoomID);
	}
	else
	{
		roomManager->AutoPlayEnd(AutoFID, ActPos);
	}
}

void CRoomWorld::AutoPlayEnd(T_FAMILYID AutoFID, uint8 ActPos)
{
	ChannelData	*pChannelData = GetChannel(1);	// 1번채널
	if (pChannelData == NULL) // by krs
	{
		//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=1", m_RoomID);
		pChannelData = GetChannel(5001);
		if (pChannelData == NULL)
		{
			//sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=5001", m_RoomID);			
			return;
		}		
	}

	// AutoPlayer가 나가는것을 통보한다
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_AUTO_LEAVE)
		msgOut.serial(AutoFID);
	FOR_END_FOR_FES_ALL()

	{
		std::map<T_FAMILYID, T_ROOMID>::iterator it = s_AutoPlayerRoomID.find(AutoFID);
		if (it != s_AutoPlayerRoomID.end())
			s_AutoPlayerRoomID.erase(it);
		else
			sipwarning("it != s_AutoPlayerRoomID.end()!! AutoFID=%d", AutoFID);
	}
	{
		for (std::vector<AutoPlayUserInfo>::iterator it = m_AutoPlayUserList.begin(); it != m_AutoPlayUserList.end(); it ++)
		{
			if (it->AutoFID == AutoFID)
			{
				m_AutoPlayUserList.erase(it);
				break;
			}
		}
	}
}

// 예약행사자료를 등록한다. ([d:RoomID][u:RoomName][M1970:StartTime][d:YuyueDays][d:CountPerDay][d:ItemID_Yishi][d:ItemID_Xianbao][d:ItemID_Fishfood][d:ItemID_Tian][d:ItemID_Di])
// 2020/04/15 [d:MoneyType] 추가
void cb_M_CS_AUTOPLAY3_REGISTER(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint16		InvenPos;
	uint32		SyncCode;
	uint32		RoomID, StartTime, YuyueDays, CountPerDay, ItemID_Yishi, ItemID_Xianbao, ItemID_Fishfood, ItemID_Tian, ItemID_Di, MoneyType;
	ucstring	RoomName;

	_msgin.serial(FID, InvenPos, SyncCode);
	_msgin.serial(RoomID, RoomName, StartTime, YuyueDays, CountPerDay, ItemID_Yishi, ItemID_Xianbao, ItemID_Fishfood);
	_msgin.serial(ItemID_Tian, ItemID_Di, MoneyType);

	{
		// 체계사용자는 Autoplay를 보낼수 없게 제한한다!!!
		// 왜냐하면 Offline상태검사때문이다. CheckIfActSave함수에서 Offline일 때 return true를 넣기 위해서이다.
		FAMILY *pF = GetFamilyData(FID);
		if (pF == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", FID);
			return;
		}

		T_ACCOUNTID userid = pF->m_UserID;
		if (IsSystemAccount(userid))
		{
			sipwarning("System account can't use Autoplay. FID=%d, UID=%d", FID, userid);
			return;
		}
	}

	if (CountPerDay <= 0)
	{
		sipwarning("Invalid CountPerDay. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	// 사용자정보 얻기
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	// 인벤 검사
	if (!IsValidInvenPos(InvenPos))
	{
		ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos[0]. FID=%d, InvenPos=%d", InvenPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	int	InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
	ITEM_INFO	&item = pInfo->m_InvenItems.Items[InvenIndex];
	const MST_ITEM	*pmst_item = GetMstItemInfo(item.ItemID);
	if (pmst_item == NULL)
	{
		sipwarning("GetMstItemInfo = NULL. InvenPos=%d, ItemID=%d", InvenPos, item.ItemID);
		return;
	}
	if (pmst_item->TypeID != ITEMTYPE_ACTDELEGATE)
	{
		ucstring ucUnlawContent = SIPBASE::_toString("pmst_item->TypeID != ITEMTYPE_ACTDELEGATE. InvenPos=%d, ItemID=%d", pInfo->m_AutoPlayInvenPos[0], item.ItemID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	// 아이템ID 검사
	int	UMoney = 0, GMoney = 0, addPoint = 0;
	if (ItemID_Yishi != 0)
	{
		OUT_TAOCAN_FOR_AUTOPLAY*	pTaocanItemInfo = GetTaocanItemInfo(ItemID_Yishi);
		if (pTaocanItemInfo == NULL)
		{
			sipwarning("GetTaocanItemInfo = NULL for Yishi. FID=%d", FID);
			return;
		}
		if (pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_ROOM && pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_ROOM_XDG)
		{
			sipwarning("pinvalid TaocanType for Yishi. FID=%d, ItemID=%d, TaocanType=%d", FID, ItemID_Yishi, pTaocanItemInfo->TaocanType);
			return;
		}

		const MST_ITEM *mstItem = GetMstItemInfo(ItemID_Yishi);
		if (mstItem == NULL)
		{
			sipwarning(L"GetMstItemInfo = NULL for Yishi. FID=%d, ItemID=%d", FID, ItemID_Yishi);
			return;
		}
		UMoney += mstItem->UMoney;
		GMoney += mstItem->GMoney;
		addPoint += mstItem->AddPoint;
		if (mstItem->GMoney == 0)
		{
			sipwarning("Item's GMoney=0. ItemID=%d", ItemID_Yishi);
		}

	}

	if (ItemID_Xianbao != 0)
	{
		OUT_TAOCAN_FOR_AUTOPLAY*	pTaocanItemInfo = GetTaocanItemInfo(ItemID_Xianbao);
		if (pTaocanItemInfo == NULL)
		{
			sipwarning("GetTaocanItemInfo = NULL for Xianbao. FID=%d", FID);
			return;
		}
		if (pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_XIANBAO)
		{
			sipwarning("pinvalid TaocanType for Xianbao. FID=%d, ItemID=%d, TaocanType=%d", FID, ItemID_Xianbao, pTaocanItemInfo->TaocanType);
			return;
		}

		const MST_ITEM *mstItem = GetMstItemInfo(ItemID_Xianbao);
		if (mstItem == NULL)
		{
			sipwarning(L"GetMstItemInfo = NULL for Xianbao. FID=%d, ItemID=%d", FID, ItemID_Xianbao);
			return;
		}

		UMoney += mstItem->UMoney;
		GMoney += mstItem->GMoney;
		addPoint += mstItem->AddPoint;
		if (mstItem->GMoney == 0)
		{
			sipwarning("Item's GMoney=0. ItemID=%d", ItemID_Xianbao);
		}
	}

	if (ItemID_Fishfood != 0)
	{
		const MST_ITEM *mstItem = GetMstItemInfo(ItemID_Fishfood);
		if (mstItem == NULL)
		{
			sipwarning(L"GetMstItemInfo = NULL for Yangyu. FID=%d, ItemID=%d", FID, ItemID_Fishfood);
			return;
		}
		if (mstItem->TypeID != ITEMTYPE_PET_TOOL)
		{
			sipwarning(L"Invalid FishFood TypeID. FID=%d, ItemID=%d", FID, ItemID_Fishfood);
			return;
		}

		UMoney += mstItem->UMoney;
		GMoney += mstItem->GMoney;
		addPoint += mstItem->AddPoint;
		if (mstItem->GMoney == 0)
		{
			sipwarning("Item's GMoney=0. ItemID=%d", ItemID_Fishfood);
		}
	}

	if (ItemID_Tian != 0)
	{
		OUT_TAOCAN_FOR_AUTOPLAY*	pTaocanItemInfo = GetTaocanItemInfo(ItemID_Tian);
		if (pTaocanItemInfo == NULL)
		{
			sipwarning("GetTaocanItemInfo = NULL for Tian. FID=%d", FID);
			return;
		}
		if (pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_ROOM)
		{
			sipwarning("pinvalid TaocanType for Tian. FID=%d, ItemID=%d, TaocanType=%d", FID, ItemID_Tian, pTaocanItemInfo->TaocanType);
			return;
		}

		const MST_ITEM *mstItem = GetMstItemInfo(ItemID_Tian);
		if (mstItem == NULL)
		{
			sipwarning(L"GetMstItemInfo = NULL for Tian. FID=%d, ItemID=%d", FID, ItemID_Tian);
			return;
		}

		UMoney += mstItem->UMoney;
		GMoney += mstItem->GMoney;
		addPoint += mstItem->AddPoint;
		if (mstItem->GMoney == 0)
		{
			sipwarning("Item's GMoney=0. ItemID=%d", ItemID_Tian);
		}

	}

	if (ItemID_Di != 0)
	{
		OUT_TAOCAN_FOR_AUTOPLAY*	pTaocanItemInfo = GetTaocanItemInfo(ItemID_Di);
		if (pTaocanItemInfo == NULL)
		{
			sipwarning("GetTaocanItemInfo = NULL for Di. FID=%d", FID);
			return;
		}
		if (pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_ROOM)
		{
			sipwarning("pinvalid TaocanType for Di. FID=%d, ItemID=%d, TaocanType=%d", FID, ItemID_Di, pTaocanItemInfo->TaocanType);
			return;
		}

		const MST_ITEM *mstItem = GetMstItemInfo(ItemID_Di);
		if (mstItem == NULL)
		{
			sipwarning(L"GetMstItemInfo = NULL for Di. FID=%d, ItemID=%d", FID, ItemID_Di);
			return;
		}
		UMoney += mstItem->UMoney;
		GMoney += mstItem->GMoney;
		addPoint += mstItem->AddPoint;
		if (mstItem->GMoney == 0)
		{
			sipwarning("Item's GMoney=0. ItemID=%d", ItemID_Di);
		}
	}

	UMoney *= (YuyueDays * CountPerDay);
	GMoney *= (YuyueDays * CountPerDay);
	addPoint *= (YuyueDays * CountPerDay);

	// Money검사
	if (MoneyType == MONEYTYPE_UMONEY)
	{
		if (UMoney > 0)
		{
			if (!CheckMoneySufficient(FID, UMoney, 0, MONEYTYPE_UMONEY))
			{
				sipwarning("CheckMoneySufficient failed. FID=%d, CurUMoney=%d, ReqUMoney=%d", FID, pInfo->m_nUMoney, UMoney);
				return;
			}
		}
	}
	else // MONEYTYPE_POINT
	{
		if (GMoney > 0)
		{
			if (!CheckMoneySufficient(FID, 0, GMoney, MONEYTYPE_POINT))
			{
				sipwarning("CheckMoneySufficient failed. FID=%d, CurGMoney=%d, ReqGMoney=%d", FID, pInfo->m_nGMoney, GMoney);
				return;
			}
		}
	}


	// OK, 처리
	std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator it = pInfo->m_mapCH.begin();
	if (it == pInfo->m_mapCH.end())
	{
		sipwarning("it == pInfo->m_mapCH.end()!!! FID=%d", FID);
		return;
	}
	T_CHARACTERID			character_id = it->first;
	CHARACTER	*character = GetCharacterData(character_id);
	if (character == NULL)
	{
		sipwarning("GetCharacterData = NULL. FID=%d, CHID=%d", FID, character_id);
		return;
	}

	// 착용복장 얻기
	uint32	DressID = character->m_DefaultDressID;
	for (int j = 0; j < MAX_INVEN_NUM; j ++)
	{
		if ((pInfo->m_InvenItems.Items[j].ItemID != 0) && (pInfo->m_InvenItems.Items[j].UsedCharID == character_id))
		{
			DressID = pInfo->m_InvenItems.Items[j].ItemID;
			break;
		}
	}

	// Money삭제
	if (MoneyType == MONEYTYPE_UMONEY)
	{
		if (UMoney > 0)
		{
			// MainType是41的时候，传入的参数itemId和ItemTypeId与MainType是40时传入值位置相反。为了网站展现正常，需与40的统一，因为暂时服务器不能发版本，数据库接口Main_user_money_set做了修改
			ExpendMoney(FID, UMoney, 0, 1, DBLOG_MAINTYPE_USEITEM, DBLSUB_USEITEM_AUTOPLAY,
				pmst_item->SubID, pmst_item->TypeID, 1, UMoney, RoomID);
			ExpendMoney(FID, 0, addPoint, 2, DBLOG_MAINTYPE_USEITEM, DBLSUB_USEITEM_AUTOPLAY,
				pmst_item->SubID, pmst_item->TypeID, 1, UMoney, RoomID);

			SendFamilyProperty_Money(_tsid, FID, pInfo->m_nGMoney, pInfo->m_nUMoney);
		}
	}
	else // MONEYTYPE_POINT
	{
		if (GMoney > 0)
		{
			// MainType是41的时候，传入的参数itemId和ItemTypeId与MainType是40时传入值位置相反。为了网站展现正常，需与40的统一，因为暂时服务器不能发版本，数据库接口Main_user_money_set做了修改
			ExpendMoney(FID, 0, GMoney, 1, DBLOG_MAINTYPE_USEITEM, DBLSUB_USEITEM_AUTOPLAY,
				pmst_item->SubID, pmst_item->TypeID);

			SendFamilyProperty_Money(_tsid, FID, pInfo->m_nGMoney, pInfo->m_nUMoney);
		}
	}

	// Item삭제
	{
		item.ItemCount --;
		if (item.ItemCount <= 0)
			item.ItemID = 0;

		if (!SaveFamilyInven(FID, &pInfo->m_InvenItems))
		{
			sipwarning("SaveFamilyInven failed. FID=%d", FID);
			return;
		}

		DBLOG_ItemUsed(DBLSUB_USEITEM_AUTOPLAY, FID, 0, pmst_item->SubID, 1, pmst_item->Name, "", 0);

		if ((SyncCode > 0) && (_tsid.get() != 0))
		{
			CMessage msgOut(M_SC_INVENSYNC);
			uint32		ret = ERR_NOERR;
			msgOut.serial(FID, SyncCode, ret);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}
	}

	// Main에 통지 등록
	{
		CMessage	msgOut(M_NT_AUTOPLAY3_REGISTER);
		msgOut.serial(RoomID, RoomName, FID, pInfo->m_Name, character->m_ModelId, DressID, character->m_FaceID, character->m_HairID);
		msgOut.serial(StartTime, YuyueDays, CountPerDay, ItemID_Yishi, ItemID_Xianbao, ItemID_Fishfood, ItemID_Tian, ItemID_Di);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}
}

// Request create room. ManagerService->OpenRoomService ([d:RoomID])
void	cb_M_NT_ROOM_CREATE_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID		RoomID;
	T_FAMILYID		FID;

	_msgin.serial(RoomID);
	_msgin.serial(FID);

	CreateRoomWorld1(RoomID, FID);
}

static void	DBCB_DBAddRoomMemorialForYuyue(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		nCurActionGroup	= *(uint32 *)argv[0];
	T_ROOMID	RoomID = *(T_ROOMID *)argv[1];
	T_FAMILYID	FID = *(T_FAMILYID *)argv[2];
	T_FAMILYNAME	FamilyName	= (ucchar *)argv[3];

	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		uint32 ret;		resSet->serial(ret);
		if (ret == 0)
		{
			T_COMMON_ID mid;		resSet->serial(mid);
			T_DATETIME	mday;		resSet->serial(mday);
			T_ROOMVDAYS vdays;		resSet->serial(vdays);

			{
				MAKEQUERY_AddFishFlowerVisitData(strQ, nCurActionGroup, FF_BLESSLAMP, mid, 0);
				DBCaller->execute(strQ);
			}

			// 선로1에 길상등을 추가해주자!!!
			CRoomWorld *inManager = GetOpenRoomWorld(RoomID);
			if (inManager == NULL)
			{
				sipwarning("GetOpenRoomWorld = NULL. ROomID=%d, FID=%d", RoomID, FID);
			}
			else
			{
				RoomChannelData	*pChannelData = inManager->GetChannel(1);
				if (pChannelData == NULL)
				{
					//sipwarning("GetFamilyChannel(1) = NULL. FID=%d, RoomID=%d, ChannleID=1", FID, RoomID);
					pChannelData = inManager->GetChannel(5001); // by krs
					if (pChannelData == NULL)
					{
						//sipwarning("GetFamilyChannel(5001) = NULL. FID=%d, RoomID=%d, ChannleID=5001", FID, RoomID);						
					}
				}
				else
				{
					int realindex = inManager->FindNextBlesslampPos(1);
					if (realindex >= 0)
					{
						uint8	DeskIndex = (uint8)(realindex + 1 - ACTPOS_BLESS_LAMP);

						pChannelData->m_BlessLamp[DeskIndex].MemoID = mid;
						pChannelData->m_BlessLamp[DeskIndex].FID = FID;
						pChannelData->m_BlessLamp[DeskIndex].FName = FamilyName;
						pChannelData->m_BlessLamp[DeskIndex].Time = (uint32)GetDBTimeSecond();
						{
							uint8	flag = 1;
							FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ROOMMEMO_UPDATE)
								msgOut.serial(DeskIndex, mid, FID, FamilyName, flag);
							FOR_END_FOR_FES_ALL()
						}

						//sipinfo("Blessslamp Added(YuYue). FID=%d, RoomID=%d, DeskIndex=%d", FID, RoomID, DeskIndex); byy krs
					}
				}
			}
		}
		else
			sipwarning(L"Failed add room memorial : nCurActionGroup=%d", nCurActionGroup);
	}	
}

uint32 CRoomWorld::YuyueYishiRegActionMemo(T_FAMILYID FID, T_FAMILYNAME FamilyName, uint32 YangyuItemID, uint32 CountPerDay, uint32 nCurActionGroup)
{
	// 방의 사용기간이 다 되였는가를 검사
	TTime limittime = m_BackTime.timevalue;
	if (limittime != 0)
	{
		TTime	curtime = GetDBTimeSecond();
		if (curtime > limittime)
			return 0xFFFFFFFF;
	}

	// 방이 동결되였으면
	if (m_bClosed)
		return 0xFFFFFFFE;

	// 예약행사: 물주기
	bool	bFlowerSuccess = true;
	// 현재 꽃이 있는지 검사.
	if (!IfFlowerExist())
	{
		bFlowerSuccess = false;
	}

	// 한 사용자는 한 방에서 3시간에 한번씩 물을 줄수 있다.
	uint32		curTime = (uint32)GetDBTimeSecond();
	if (bFlowerSuccess)
	{
		TmapDwordToDword::iterator	it = m_mapFamilyFlowerWaterTime.find(FID);
		if (it != m_mapFamilyFlowerWaterTime.end())
		{
			const int WaterTime = 3 * 3600; // 3시간
			if ((it->second > 0) && (curTime - it->second < WaterTime - 10)) // 10초정도 여유를 준다.
			{
				bFlowerSuccess = false;
			}
		}
	}

	if (bFlowerSuccess)
	{
		// 이 처리에서 Prize는 없는것으로 한다.
		m_mapFamilyFlowerWaterTime[FID] = curTime;
		ChangeFlowerLevel(1, FID, 1);
		ChangeRoomExp(Room_Exp_Type_Flower_Water, FID);
		// ChangeFamilyExp(Family_Exp_Type_Flower_Water, FID); ---- 이 처리를 어떻게 하지???????

		// 기록 보관
		{
			MAKEQUERY_AddFishFlowerVisitData(strQ, nCurActionGroup, FF_FLOWER_WATER, 0, 0);
			DBCaller->execute(strQ);
		}
	}

	// 예약행사: 꽃드리기
	{
		// ChangeFamilyExp(Family_Exp_Type_XingLi, FID); ---- 이 처리를 어떻게 하지???????
		{
			MAKEQUERY_AddFishFlowerVisitData(strQ, nCurActionGroup, FF_XINGLI, 0, 0);
			DBCaller->execute(strQ);
		}
	}

	// 예약행사: 길상등드리기
	bool	bBlesslampSuccess = false;
	if (!m_FreeFlag)	// 체험방에는 길상등이 없다.
	{
		///* 길상등이 나타나지 않는다고 하여 방문록상에서도 없애는것으로 한다. 2017/08/07
		bBlesslampSuccess = true;
		// 한사람은 하루에 한 기념관에서 1개까지의 길상등을 남길수 있다.
		uint32	today = GetDBTimeDays();
		if (today != m_BlesslampDay)
		{
			m_BlesslampDay = today;
			m_mapFamilyBlesslampTime.clear();
		}
		TmapDwordToDword::iterator it = m_mapFamilyBlesslampTime.find(FID);
		if (it != m_mapFamilyBlesslampTime.end())
		{
			if (it->second >= BLESS_LAMP_PUT_COUNT && !bJiShangDengTest)
			{
				bBlesslampSuccess = false;
			}
			else
			{
				it->second ++;
			}
		}
		else
		{
			m_mapFamilyBlesslampTime.insert(make_pair(FID, 1));
		}//*/
	}

	if (bBlesslampSuccess)
	{
		// 길상등정보 보관
		{
			ucstring	memo = L"0";
			uint8		bSecret = 0;
			MAKEQUERY_ADDROOMMEMORIAL(strQ, FID, m_RoomID, memo, bSecret);
			DBCaller->executeWithParam(strQ, DBCB_DBAddRoomMemorialForYuyue,
				"DDDS",
				CB_,	nCurActionGroup,
				CB_,	m_RoomID,
				CB_,	FID,
				CB_,	FamilyName.c_str());
		}

		// ChangeFamilyExp(Family_Exp_Type_Memo, FID); ---- 이 처리를 어떻게 하지???????
		ChangeRoomExp(Room_Exp_Type_Memo, FID);
	}

	// 예약행사: 물고기먹이주기
	if (YangyuItemID != 0)
	{
		const	OUT_PETCARE	*mstPetCare = GetOutPetCare(YangyuItemID);
		const	MST_ITEM	*mstItem = GetMstItemInfo(YangyuItemID);
		if (mstPetCare == NULL || GetMstItemInfo == NULL)
		{
			sipwarning(L"mstPetCare = NULL or GetMstItemInfo = NULL. FID=%d, ItemID=%d", FID, YangyuItemID);
		}
		else
		{
			// 경험값 변경 
			// ChangeFamilyExp(Family_Exp_Type_Fish, FID);  * CountPerDay ---- 이 처리를 어떻게 하지???????

			// 하루에 한방에서 물고기먹이를 줄수 있는 회수는 MAX_FISHFOOD_COUNT로 제한된다.
			if (m_nFishFoodCount + CountPerDay > MAX_FISHFOOD_COUNT)
				CountPerDay = MAX_FISHFOOD_COUNT - m_nFishFoodCount;
			if (CountPerDay > 0)
			{
				int exp = mstPetCare->IncLife * CountPerDay;
				m_nFishFoodCount += CountPerDay;
				uint32	FishScopeID = 0, FishFoodID = 0;

				// if (CheckIfActSave(FID)) FID가 offline인 상태에서는 이 함수를 쓸수 없다.
				{
					if (exp > 0)
					{
						SetFishExp(m_FishExp + exp);
					}

					{
						MAKEQUERY_SaveFishExp(strQ, m_RoomID, 0, m_FishExp, 0); // m_FishLevel, m_FishExp, m_FishNextUpdateTime);
						DBCaller->execute(strQ);
					}
				}

				FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_UPDATE_FISH)
					msgOut.serial(m_FishExp, FID, FishScopeID, FishFoodID, m_nFishFoodCount);
				FOR_END_FOR_FES_ALL()

				ucstring itemInfo = mstItem->Name;
				for (uint32 i = 0; i < CountPerDay; i ++)
				{
					ChangeRoomExp(Room_Exp_Type_Fish, FID);
					ChangeRoomExp(Room_Exp_Type_UseMoney, FID, mstItem->UMoney, 0, itemInfo);

					// 기록 보관
					{
						MAKEQUERY_AddFishFlowerVisitData(strQ, nCurActionGroup, FF_FISH_FOOD, YangyuItemID, 0);
						DBCaller->execute(strQ);
					}
				}
			}
		}
	}

	uint32	Param = 0;
	if (bFlowerSuccess)
		Param |= 1;
	if (bBlesslampSuccess)
		Param |= 2;

	return Param;
}

//============================================================
//  기념관보물창고기능
//============================================================
RoomStoreLockData::RoomStoreLockData(uint32 _LockID, T_FAMILYID _FID, uint8 _ItemCount, uint32* pItemIDs)
{
	LockID = _LockID;
	FID = _FID;
	ItemCount = _ItemCount;
	for (int i = 0; i < ItemCount; i ++)
		ItemIDs[i] = pItemIDs[i];
	LockTime = (uint32)GetDBTimeSecond();
}

void RoomStore::init(uint8 *buff, uint32 len)
{
	AllItemCount = 0;

	if ((len % 6) != 0)
	{
		sipwarning("RoomStore Init Error. ((len % 6) != 0). len=%d", len);
		return;
	}
	int Count = (uint16)(len / 6);
	for (int i = 0; i < Count; i ++)
	{
		uint32 ItemID = *((uint32*)(buff + i * 6));
		uint16 ItemCount = *((uint16*)(buff + i * 6 + 4));
		mapStoreItems[ItemID] = ItemCount;

		AllItemCount += ItemCount;
	}

	StoreSyncCode = (uint32)GetDBTimeSecond();
}
void RoomStore::init(RoomStore &data)
{
	mapStoreItems.clear();
	AllItemCount = 0;

	StoreSyncCode = data.StoreSyncCode;
	for (std::map<uint32, uint16>::iterator it = data.mapStoreItems.begin(); it != data.mapStoreItems.end(); it ++)
	{
		mapStoreItems[it->first] = it->second;

		AllItemCount += it->second;
	}
}
int RoomStore::GetStoreBoxCount()
{
	int count = 0;
	for (std::map<uint32, uint16>::iterator it = mapStoreItems.begin(); it != mapStoreItems.end(); it ++)
	{
//		const	MST_ITEM	*mstItem = GetMstItemInfo(it->first);
//		if (mstItem->OverlapFlag == 0)
//			count += it->second;
//		else
//			count += ((it->second + mstItem->OverlapMaxNum - 1) / mstItem->OverlapMaxNum);
		// 2012/06/14 - 천원창고의 아이템은 100개단위로 겹쳐놓을수 있다.
		count += ((it->second + 99) / 100);
	}
	return count;
}

void CRoomWorld::SaveRoomStore()
{
	tchar str_buf[MAX_ROOMSTORE_COUNT * 12 + 10];
	int count = 0;
	for (std::map<uint32, uint16>::iterator it = m_RoomStore.mapStoreItems.begin(); it != m_RoomStore.mapStoreItems.end(); it ++)
	{
		uint32	v = it->first;
		smprintf((tchar*)(&str_buf[count * 12 + 0]), 2, _t("%02x"), (v >> 0) & 0xFF);
		smprintf((tchar*)(&str_buf[count * 12 + 2]), 2, _t("%02x"), (v >> 8) & 0xFF);
		smprintf((tchar*)(&str_buf[count * 12 + 4]), 2, _t("%02x"), (v >> 16) & 0xFF);
		smprintf((tchar*)(&str_buf[count * 12 + 6]), 2, _t("%02x"), (v >> 24) & 0xFF);
		uint16 v1 = it->second;
		smprintf((tchar*)(&str_buf[count * 12 + 8]), 2, _t("%02x"), (v1 >> 0) & 0xFF);
		smprintf((tchar*)(&str_buf[count * 12 + 10]), 2, _t("%02x"), (v1 >> 8) & 0xFF);
		count ++;
	}
	str_buf[count * 12] = 0;

	MAKEQUERY_SaveRoomStore(strQ, m_RoomID, str_buf);
	DBCaller->execute(strQ);
}

static void	DBCB_DBSaveRoomStoreHistory(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		StoreID = *((uint32 *)argv[0]);
	uint8		flag = *((uint8 *)argv[1]);
	uint32		RoomID = *((uint32 *)argv[2]);
	uint32		FID = *((uint32 *)argv[3]);

	if (!nQueryResult)
		return;

	if (flag == 1)
	{
		// 기록 보관
		INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(FID);
		if (pFamilyInfo == NULL)
		{
			sipwarning(L"GetFamilyInfo = NULL. RoomID=%d, FID=%d", RoomID, FID);
		}
		else
		{
			pFamilyInfo->DonActInRoom(ROOMACT_ROOMSTORE);

			MAKEQUERY_AddFishFlowerVisitData(strQ, pFamilyInfo->m_nCurActionGroup, FF_ROOMSTORE, StoreID, 0);
			DBCaller->execute(strQ);
		}
	}
}

void CRoomWorld::SaveRoomStoreHistory(uint8 flag, T_FAMILYID FID, uint8 Protected, ucstring Comment, uint8 Count, uint32 *pItemIDs, uint8 *pItemCount)
{
	tchar str_buf[MAX_ROOMSTORE_SAVE_COUNT * 10 + 10];
	for (int i = 0; i < Count; i ++)
	{
		uint32	v = pItemIDs[i];
		smprintf((tchar*)(&str_buf[i * 10 + 0]), 2, _t("%02x"), (v >> 0) & 0xFF);
		smprintf((tchar*)(&str_buf[i * 10 + 2]), 2, _t("%02x"), (v >> 8) & 0xFF);
		smprintf((tchar*)(&str_buf[i * 10 + 4]), 2, _t("%02x"), (v >> 16) & 0xFF);
		smprintf((tchar*)(&str_buf[i * 10 + 6]), 2, _t("%02x"), (v >> 24) & 0xFF);
		uint8 v1 = pItemCount[i];
		smprintf((tchar*)(&str_buf[i * 10 + 8]), 2, _t("%02x"), (v1 >> 0) & 0xFF);
	}
	str_buf[Count * 10] = 0;

	MAKEQUERY_SaveRoomStoreHistory(strQ, m_RoomID, FID, flag, str_buf, Protected, Comment);
	DBCaller->executeWithParam(strQ, DBCB_DBSaveRoomStoreHistory,
		"DBDD",
		OUT_,		NULL,
		CB_,		flag,
		CB_,		m_RoomID,
		CB_,		FID);
}

//BOOL CRoomWorld::CheckRoomStoreUseable(uint8 Count, uint32 *pItemIDs, uint8 *pItemCount)
//{
//	RoomStore	tempStore;
//	tempStore.init(m_RoomStore);
//	for (int i = 0; i < Count; i ++)
//	{
//		std::map<uint32, uint16>::iterator it = tempStore.mapStoreItems.find(pItemIDs[i]);
//		if (it == tempStore.mapStoreItems.end())
//			return false;
//		if (it->second < pItemCount[i])
//			return false;
//		it->second -= pItemCount[i];
//	}
//	return true;
//}

// Request RoomStore ([d:StoreSyncCode])
void cb_M_CS_GET_ROOMSTORE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		StoreSyncCode;

	_msgin.serial(FID, StoreSyncCode);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->GetRoomStore(FID, StoreSyncCode, _tsid);
}

void CRoomWorld::GetRoomStore(T_FAMILYID FID, uint32 StoreSyncCode, SIPNET::TServiceId _tsid)
{
	CMessage	msgOut(M_SC_ROOMSTORE);
	msgOut.serial(FID, m_RoomStore.StoreSyncCode);
	if (StoreSyncCode != m_RoomStore.StoreSyncCode)
	{
		for (std::map<uint32, uint16>::iterator it = m_RoomStore.mapStoreItems.begin(); it != m_RoomStore.mapStoreItems.end(); it ++)
		{
			uint32	ItemID = it->first;
			uint16	ItemCount = it->second;
			msgOut.serial(ItemID, ItemCount);
		}
	}
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

// Add Item To RoomStore ([d:SyncCode][b:protected][u:comment][b:Count][ [w:InvenPos][b:ItemCount] ])
void cb_M_CS_ADD_ROOMSTORE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			InvenSyncCode;
	uint8			Protected;
	ucstring		Comment;
	uint8			Count;
	uint16			InvenPos[MAX_ROOMSTORE_SAVE_COUNT];
	uint8			ItemCount[MAX_ROOMSTORE_SAVE_COUNT];

	_msgin.serial(FID, InvenSyncCode, Protected, Comment, Count);
	if ((Count <= 0) || (Count > sizeof(ItemCount)))
	{
		sipwarning("Invalid Count. FID=%d, Count=%d", FID, Count);
		return;
	}
	for (uint8 i = 0; i < Count; i ++)
	{
		_msgin.serial(InvenPos[i], ItemCount[i]);
	}

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->AddRoomStore(FID, InvenSyncCode, Protected, Comment, Count, InvenPos, ItemCount, _tsid);
}

void CRoomWorld::AddRoomStore(T_FAMILYID FID, uint32 InvenSyncCode, uint8 Protected, ucstring Comment, uint8 Count, uint16 *pInvenPos, uint8 *pItemCount, SIPNET::TServiceId _tsid)
{
	uint32	ItemIDs[MAX_ROOMSTORE_SAVE_COUNT];
	for (int i = 0; i < Count; i ++)
	{
		if (!IsValidInvenPos(pInvenPos[i]))
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos! FID=%d, InvenPos=%d", FID, pInvenPos[i]);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		for (int j = 0; j < i; j ++)
		{
			if (pInvenPos[i] == pInvenPos[j])
			{
				// 비법파케트발견
				ucstring ucUnlawContent = SIPBASE::_toString("Same Invalid Exist! FID=%d, InvenPos=%d", FID, pInvenPos[i]);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
		}
	}

	_InvenItems*	pInven = GetFamilyInven(FID);
	if (pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL");
		return;
	}

	// Item개수, ItemType 검사 - 행사용품, 물건태우기용품, 물고기먹이, 꽃씨, 금붕어
//	uint32	UsedUMoney = 0, UsedGMoney = 0;
	for (int i = 0; i < Count; i ++)
	{
		int		InvenIndex = GetInvenIndexFromInvenPos(pInvenPos[i]);
		ITEM_INFO	&item = pInven->Items[InvenIndex];
		uint32	ItemID = item.ItemID;
		ItemIDs[i] = ItemID;

		const	MST_ITEM	*mstItem = GetMstItemInfo(ItemID);
		if (mstItem == NULL)
		{
			sipwarning(L"GetMstItemInfo = NULL. FID=%d, ItemID=%d", FID, ItemID);
			return;
		}
		if (pItemCount[i] > item.ItemCount)
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("pItemCount[i] > item.ItemCount! FID=%d, InvenPos=%d, ItemCount=%d>%d", FID, pInvenPos[i], pItemCount[i], item.ItemCount);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		if (item.MoneyType != MONEYTYPE_UMONEY)
		{
			// 실지돈으로 산 아이템만 천원창고에 넣을수 있다.
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("item.MoneyType != MONEYTYPE_UMONEY! FID=%d, InvenPos=%d", FID, pInvenPos[i], pItemCount[i]);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		if ((mstItem->TypeID != ITEMTYPE_SACRIF_JINGXIAN) && (mstItem->TypeID != ITEMTYPE_SACRIF_XIANBAO) 
			&& (mstItem->TypeID != ITEMTYPE_PET_TOOL) && (mstItem->TypeID != ITEMTYPE_PET)
			&& (mstItem->TypeID != ITEMTYPE_GUANSHANGYU) && (mstItem->TypeID != ITEMTYPE_AUTORITE))
		{
			sipwarning(L"Invalid Item Type. FID=%d, ItemID=%d, Type=%d", FID, ItemID, mstItem->TypeID);
			return;
		}

//		if(item.MoneyType == MONEYTYPE_UMONEY)
//			UsedUMoney += (mstItem->UMoney * pItemCount[i]);
//		else
//			UsedGMoney += (mstItem->GMoney * pItemCount[i]);
	}

	uint32	AddItemCount = 0;
	// tempStore에 추가
	RoomStore	tempStore;
	tempStore.init(m_RoomStore);
	for (int i = 0; i < Count; i ++)
	{
		int		InvenIndex = GetInvenIndexFromInvenPos(pInvenPos[i]);
		ITEM_INFO	&item = pInven->Items[InvenIndex];
		uint32	ItemID = item.ItemID;

		std::map<uint32, uint16>::iterator it = tempStore.mapStoreItems.find(ItemID);
		if (it != tempStore.mapStoreItems.end())
			it->second += pItemCount[i];
		else
			tempStore.mapStoreItems[ItemID] = pItemCount[i];

		AddItemCount += pItemCount[i];
	}

	// tempStore가 다 찼는지 검사
	if (tempStore.GetStoreBoxCount() > MAX_ROOMSTORE_COUNT)
	{
		uint32	result = E_ROOMSTORE_FULL;
		CMessage	msgOut(M_SC_ADD_ROOMSTORE_RESULT);
		msgOut.serial(FID, InvenSyncCode, result);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		return;
	}

	// Check OK
	// 인벤에서 삭제
	for (int i = 0; i < Count; i ++)
	{
		int		InvenIndex = GetInvenIndexFromInvenPos(pInvenPos[i]);
		ITEM_INFO	&item = pInven->Items[InvenIndex];

		item.ItemCount -= pItemCount[i];
		if (item.ItemCount <= 0)
			item.ItemID = 0;
	}

	if (!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
		return;
	}

	// m_RoomStore 변경
	m_RoomStore.init(tempStore);
	m_RoomStore.StoreSyncCode = (uint32)GetDBTimeSecond();
	SaveRoomStore();

	// RoomStore의 총아이템개수가 변했기때문에 원리적으로는 M_SC_ROOMSTORE_STATUS를 내려보내야 한다.
	// 그러나 창고에 아이템을 넣는 행사가 끝난 후 M_CS_ROOMSTORE_LASTITEM_SET 처리부에서 M_SC_ROOMSTORE_STATUS가 내려가기때문에
	// 여기서는 M_SC_ROOMSTORE_STATUS를 보내지 않는다.

	// 경험값 변경
//	ChangeFamilyExp(Family_Exp_Type_Fish, FID);

//	if ((UsedUMoney != 0) || (UsedGMoney != 0))
//	{
//		ucstring itemInfo = L"ITEM_TO_STORE";
//		ChangeRoomExp(Room_Exp_Type_UseMoney, FID, UsedUMoney, UsedGMoney, itemInfo);
//	}

	// 기록 보관
	SaveRoomStoreHistory(ROOMSTORE_HISTORY_ADD, FID, Protected, Comment, Count, ItemIDs, pItemCount);

	// 처리끝
	uint32	ret = ERR_NOERR;
	{
		CMessage	msgOut(M_SC_ADD_ROOMSTORE_RESULT);
		msgOut.serial(FID, InvenSyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
	{
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, InvenSyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	// Log
//	DBLOG_ItemUsed(DBLSUB_USEITEM_FISHFOOD, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

// Request RoomStore History ([w:Year][b:Month][b:Flag, 0:All,1:Add,2:Use][b:OwnOnlyData][d:Page])
void cb_M_CS_GET_ROOMSTORE_HISTORY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID, Page;
	uint16		Year;
	uint8		Month, Flag, bOwnDataOnly;

	_msgin.serial(FID, Year, Month, Flag, bOwnDataOnly, Page);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->GetRoomStoreHistory(FID, Year, Month, Flag, bOwnDataOnly, Page, _tsid);
}

static void	DBCB_DBGetRoomStoreHistory(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		AllCount2 = *((uint32 *)argv[0]);
	uint32		AllCount = *((uint32 *)argv[1]);
	uint32		Code = *((uint32 *)argv[2]);
	uint32		FID = *((uint32 *)argv[3]);
	uint32		RoomID = *((uint32 *)argv[4]);
	uint16		Year = *((uint16 *)argv[5]);
	uint8		Month = *((uint8 *)argv[6]);
	uint8		Flag = *((uint8 *)argv[7]);
	uint8		OwnDataFlag = *((uint8 *)argv[8]);
	uint32		Page = *((uint32 *)argv[9]);
	TServiceId	_sid(*(uint16 *)argv[10]);

	if (!nQueryResult)
		return;

	CMessage	msgOut(M_SC_ROOMSTORE_HISTORY);
	msgOut.serial(FID, RoomID, Year, Month, Flag, OwnDataFlag, Page, AllCount);

	if (Code == 0)
	{
		uint32	nRows;
		resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint32	ListID;		resSet->serial(ListID);
			uint32	playerFID;	resSet->serial(playerFID);
			ucstring playerName;resSet->serial(playerName);
			uint8	flag;		resSet->serial(flag);
			string	datetime;	resSet->serial(datetime);

			uint32	utime = getMinutesSince1970(datetime);

			msgOut.serial(ListID, playerFID, playerName, flag, utime);
		}
	}

	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

void CRoomWorld::GetRoomStoreHistory(T_FAMILYID FID, uint16 Year, uint8 Month, uint8 Flag, uint8 bOwnDataOnly, uint32 Page, SIPNET::TServiceId _sid)
{
	uint32	actorFID = (bOwnDataOnly != 0) ? FID : 0;
	MAKEQUERY_GetRoomStoreHistory(strQ, m_RoomID, Year, Month, Flag, actorFID, Page);
	DBCaller->executeWithParam(strQ, DBCB_DBGetRoomStoreHistory,
		"DDDDDWBBBDW",
		OUT_,		NULL,
		OUT_,		NULL,
		OUT_,		NULL,
		CB_,		FID,
		CB_,		m_RoomID,
		CB_,		Year,
		CB_,		Month,
		CB_,		Flag,
		CB_,		bOwnDataOnly,
		CB_,		Page,
		CB_,		_sid.get());
}

// Request RoomStore History Detail Info ([d:ListID])
void cb_M_CS_GET_ROOMSTORE_HISTORY_DETAIL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		ListID;

	_msgin.serial(FID, ListID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->GetRoomStoreHistoryDetail(FID, ListID, _tsid);
}

static void	DBCB_DBGetRoomStoreHistoryDetail(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		FID = *((uint32 *)argv[0]);
	uint32		ListID = *((uint32 *)argv[1]);
	TServiceId	_sid(*(uint16 *)argv[2]);

	if (!nQueryResult)
		return;

	uint32	nRows;
	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipwarning("nRows != 1. FID=%d, ListID=%d", FID, ListID);
		return;
	}
	uint32	len;	resSet->serial(len);
	uint8	buf[MAX_ROOMSTORE_SAVE_COUNT*5+10];
	resSet->serialBuffer(buf, len);
	ucstring	Comment;	resSet->serial(Comment);
	uint8		Protected;	resSet->serial(Protected);
	uint8		Shield;		resSet->serial(Shield);

	if ((len % 5) != 0)
	{
		sipwarning("(len % 5) != 0. ListID=%d, len=%d", ListID, len);
		return;
	}
	len = len / 5;

	CMessage	msgOut(M_SC_ROOMSTORE_HISTORY_DETAIL);
	msgOut.serial(FID, ListID, Protected, Comment, Shield);
	for (uint32 i = 0; i < len; i ++)
	{
		uint32	ItemID = *((uint32*)&buf[5 * i]);
		uint8	ItemCount = buf[5 * i + 4];
		msgOut.serial(ItemID, ItemCount);
	}

	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

void CRoomWorld::GetRoomStoreHistoryDetail(T_FAMILYID FID, uint32 ListID, SIPNET::TServiceId _sid)
{
	MAKEQUERY_GetRoomStoreHistoryDetail(strQ, ListID);
	DBCaller->executeWithParam(strQ, DBCB_DBGetRoomStoreHistoryDetail,
		"DDW",
		CB_,			FID,
		CB_,			ListID,
		CB_,			_sid.get());
}

// Lock RoomStore Items ([b:Count][ [d:ItemID] ])
void cb_M_CS_ROOMSTORE_LOCK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint8		ItemCount;
	uint32		ItemIDs[MAX_ITEM_PER_ACTIVITY];

	_msgin.serial(FID, ItemCount);
	if ((ItemCount <= 0) || (ItemCount > MAX_ITEM_PER_ACTIVITY))
	{
		// 부정한 사용자
		sipwarning("ItemCount > MAX_ITEM_PER_ACTIVITY. FID=%d, ItemCount=%d", FID, ItemCount);
		return;
	}
	for (int i = 0; i < ItemCount; i ++)
	{
		_msgin.serial(ItemIDs[i]);
	}

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->LockRoomStoreItems(FID, ItemCount, ItemIDs, _tsid);
}

void CRoomWorld::LockRoomStoreItems(T_FAMILYID FID, uint8 Count, uint32* pItemIDs, SIPNET::TServiceId _sid)
{
	static uint32 LockID = 1;

	// Store의 아이템은 방관리자들만 쓸수 있다.
	int	ManagerPermission = IsRoomManager(FID);
	if ((FID != m_MasterID) && !(ManagerPermission & MANAGER_PERMISSION_USE))
	{
		uint32	ZeroLockID = 0, ErrorCode = E_ROOMSTORE_LOCK_NOPERMISSION;
		CMessage	msgOut(M_SC_ROOMSTORE_LOCK);
		msgOut.serial(FID, ErrorCode, ZeroLockID);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
		return;
	}

	RoomStore	tempStore;
	tempStore.init(m_RoomStore);

	// tempStore에서 이미 Lock된 Item들을 삭제한다.
	for (std::map<uint32, RoomStoreLockData>::iterator it = m_RoomStore.mapLockData.begin(); it != m_RoomStore.mapLockData.end(); it ++)
	{
		// 한명의 사용자는 Lock를 한번밖에 할수 없다.
		if (it->second.FID == FID)
		{
			sipwarning("Already Locked. FID=%d, PrevLockID=%d. 20150113 to sjh", FID, it->first);
			return;
		}
		for (int i = 0; i < it->second.ItemCount; i ++)
		{
			std::map<uint32, uint16>::iterator it1 = tempStore.mapStoreItems.find(it->second.ItemIDs[i]);
			if (it1 == tempStore.mapStoreItems.end())
			{
				sipwarning("it1 == tempStore.mapStoreItems.end(). ItemID=%d", it->second.ItemIDs[i]);
				return;
			}
			if (it1->second < 1)
			{
				sipwarning("it1->second < 1. ItemID=%d", it->second.ItemIDs[i]);
				return;
			}
			it1->second --;
		}
	}

	// 새 아이템들을 Lock할수 있는지 검사한다.
	for (int i = 0; i < Count; i ++)
	{
		std::map<uint32, uint16>::iterator it1 = tempStore.mapStoreItems.find(pItemIDs[i]);
		if ((it1 == tempStore.mapStoreItems.end()) || (it1->second < 1))
		{
			// Lock 실패
			uint32	ZeroLockID = 0, ErrorCode = E_ROOMSTORE_LOCK_NOITEM;
			CMessage	msgOut(M_SC_ROOMSTORE_LOCK);
			msgOut.serial(FID, ErrorCode, ZeroLockID);
			CUnifiedNetwork::getInstance()->send(_sid, msgOut);
			return;
		}
		it1->second --;
	}

	// Lock 성공
	RoomStoreLockData	data(LockID, FID, Count, pItemIDs);
	m_RoomStore.mapLockData[LockID] = data;

	uint32	ErrorCode = 0;
	CMessage	msgOut(M_SC_ROOMSTORE_LOCK);
	msgOut.serial(FID, ErrorCode, LockID);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);

	LockID ++;
}

// Unlock RoomStore Items ([d:StoreLockID])
void cb_M_CS_ROOMSTORE_UNLOCK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		StoreLockID;

	_msgin.serial(FID, StoreLockID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->UnlockRoomStoreItems(FID, StoreLockID);
}

void CRoomWorld::UnlockRoomStoreItems(T_FAMILYID FID, uint32 StoreLockID)
{
	std::map<uint32, RoomStoreLockData>::iterator it = m_RoomStore.mapLockData.find(StoreLockID);
	if (it == m_RoomStore.mapLockData.end())
	{
		sipwarning("it == m_RoomStore.mapLockData.end(). FID=%d, StoreLockID=%d", FID, StoreLockID);
	}
	else
	{
		if (it->second.FID != FID)
		{
			sipwarning("it->second.FID != FID. FID=%d, StoreLockID=%d, it->second.FID=%d", FID, StoreLockID, it->second.FID);
		}
		m_RoomStore.mapLockData.erase(it);
	}
}

RoomStoreLockData* CRoomWorld::GetStoreLockData(T_FAMILYID FID, uint32 StoreLockID)
{
	std::map<uint32, RoomStoreLockData>::iterator it1 = m_RoomStore.mapLockData.find(StoreLockID);
	if (it1 == m_RoomStore.mapLockData.end())
	{
		sipwarning("it1 == m_RoomStore.mapLockData.end(). StoreLockID=%d", StoreLockID);
		return NULL;
	}
	if (it1->second.FID != FID)
	{
		sipwarning("it1->second.FID != FID. FID=%d, StoreLockID=%d, it1->second.FID=%d", FID, StoreLockID, it1->second.FID);
		return NULL;
	}
	return &it1->second;
}

void CRoomWorld::RoomStoreItemUsed(T_FAMILYID FID, uint32 StoreLockID)
{
	std::map<uint32, RoomStoreLockData>::iterator it1 = m_RoomStore.mapLockData.find(StoreLockID);
	if (it1 == m_RoomStore.mapLockData.end())
	{
		sipwarning("it1 == m_RoomStore.mapLockData.end(). StoreLockID=%d", StoreLockID);
		return;
	}

	for (int i = 0; i < it1->second.ItemCount; i ++)
	{
		std::map<uint32, uint16>::iterator it = m_RoomStore.mapStoreItems.find(it1->second.ItemIDs[i]);
		if (it == m_RoomStore.mapStoreItems.end())
		{
			sipwarning("it == m_RoomStore.mapItems.end(). FID=%d, ItemID=%d", FID, it1->second.ItemIDs[i]);
			return;
		}
		it->second --;
		if (it->second <= 0)
			m_RoomStore.mapStoreItems.erase(it);

		m_RoomStore.AllItemCount --;
	}

	m_RoomStore.StoreSyncCode = (uint32)GetDBTimeSecond();

	// RoomStore DB에 보관
	SaveRoomStore();

	// RoomStore가 변함을 사용자들에게 통지
	SendRoomstoreStatus();

	// RoomStore사용기록 갱신
	uint8	ItemCount[MAX_ITEM_PER_ACTIVITY];
	for (int i = 0; i < it1->second.ItemCount; i ++)
		ItemCount[i] = 1;
	SaveRoomStoreHistory(ROOMSTORE_HISTORY_USE, FID, 0, L"", it1->second.ItemCount, it1->second.ItemIDs, ItemCount);

	// Lock자료 삭제
	m_RoomStore.mapLockData.erase(it1);
}

// Set RoomStore's Last Item (In Activity) ([d:LastItemID])
void cb_M_CS_ROOMSTORE_LASTITEM_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint8		Count;
	uint32		LastItemID[MAX_ROOMSTORE_SAVE_COUNT];

	_msgin.serial(FID, Count);
	if (Count > MAX_ROOMSTORE_SAVE_COUNT)
	{
		sipwarning("Count > MAX_ROOMSTORE_SAVE_COUNT. FID=%d, Count=%d", FID, Count);
		return;
	}
	for (int i = 0; i < Count; i ++)
		_msgin.serial(LastItemID[i]);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->SetRoomStoreLastItem(FID, Count, LastItemID);
}

void CRoomWorld::SetRoomStoreLastItem(T_FAMILYID FID, uint8 Count, uint32* LastItemID)
{
	m_RoomStore.LastItemCount = Count;
	for (int i = 0; i < Count; i ++)
		m_RoomStore.LastItemID[i] = LastItemID[i];

	SendRoomstoreStatus();
}

void CRoomWorld::SendRoomstoreStatus()
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ROOMSTORE_STATUS)
		msgOut.serial(m_RoomStore.AllItemCount);
		for (int i = 0; i < m_RoomStore.LastItemCount; i ++)
			msgOut.serial(m_RoomStore.LastItemID[i]);
	FOR_END_FOR_FES_ALL()
}

void CRoomWorld::SendRoomstoreStatus(T_FAMILYID FID, SIPNET::TServiceId _sid)
{
	uint32	zero = 0;
	CMessage	msgOut(M_SC_ROOMSTORE_STATUS);
	msgOut.serial(FID, zero, m_RoomStore.AllItemCount);
	for (int i = 0; i < m_RoomStore.LastItemCount; i ++)
		msgOut.serial(m_RoomStore.LastItemID[i]);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

// Modify RoomStore Info ([d:ListID][b:protected][u:comment])
void cb_M_CS_ROOMSTORE_HISTORY_MODIFY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			ListID;
	uint8			Protected;
	ucstring		Comment;

	_msgin.serial(FID, ListID, Protected, Comment);

	MAKEQUERY_ModifyRoomStoreHistory(strQ, ListID, Protected, Comment);
	DBCaller->execute(strQ);
}

// Delete RoomStore Info ([d:ListID])
void cb_M_CS_ROOMSTORE_HISTORY_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			ListID;

	_msgin.serial(FID, ListID);

	MAKEQUERY_DeleteRoomStoreHistory(strQ, ListID);
	DBCaller->execute(strQ);
}

// Shield or unshield RoomStore Info ([d:ListID][b:shield])
void cb_M_CS_ROOMSTORE_HISTORY_SHIELD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			ListID;
	uint8			Shield;

	_msgin.serial(FID, ListID, Shield);

	MAKEQUERY_ShieldRoomStoreHistory(strQ, ListID, Shield);
	DBCaller->execute(strQ);
}

// Get Items from RoomStore ([b:Count][ [w:InvenPos][d:ItemID][w:ItemCount] ])
void cb_M_CS_ROOMSTORE_GETITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint8			Count;
	uint16			InvenPoses[MAX_ITEM_PER_ACTIVITY];
	uint32			ItemIDs[MAX_ITEM_PER_ACTIVITY];
	uint16			ItemCounts[MAX_ITEM_PER_ACTIVITY];

	_msgin.serial(FID, Count);
	if ((Count <= 0) || (Count > MAX_ITEM_PER_ACTIVITY))
	{
		// 부정한 사용자
		sipwarning("ItemCount > MAX_ITEM_PER_ACTIVITY. FID=%d, Count=%d", FID, Count);
		return;
	}
	for (int i = 0; i < Count; i ++)
	{
		_msgin.serial(InvenPoses[i], ItemIDs[i], ItemCounts[i]);
	}

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->GetRoomStoreItems(FID, Count, InvenPoses, ItemIDs, ItemCounts, _tsid);
}

void CRoomWorld::GetRoomStoreItems(T_FAMILYID FID, uint8 Count, uint16 *pInvenPoses, uint32* pItemIDs, uint16 *pItemCounts, SIPNET::TServiceId _sid)
{
	// Store의 아이템은 방관리자들만 쓸수 있다.
	int	ManagerPermission = IsRoomManager(FID);
	if ((FID != m_MasterID) && !(ManagerPermission & MANAGER_PERMISSION_GET))
	{
		uint32	ErrorCode = E_ROOMSTORE_GET_NOPERMISSION;
		CMessage	msgOut(M_SC_ROOMSTORE_GETITEM);
		msgOut.serial(FID, ErrorCode);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
		return;
	}

	_InvenItems	*pInven = GetFamilyInven(FID);
	if(pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL. FID=%d", FID);
		return;
	}

	RoomStore	tempStore;
	tempStore.init(m_RoomStore);

	// tempStore에서 이미 Lock된 Item들을 삭제한다.
	for (std::map<uint32, RoomStoreLockData>::iterator it = m_RoomStore.mapLockData.begin(); it != m_RoomStore.mapLockData.end(); it ++)
	{
		for (int i = 0; i < it->second.ItemCount; i ++)
		{
			std::map<uint32, uint16>::iterator it1 = tempStore.mapStoreItems.find(it->second.ItemIDs[i]);
			if (it1 == tempStore.mapStoreItems.end())
			{
				sipwarning("it1 == tempStore.mapStoreItems.end(). ItemID=%d", it->second.ItemIDs[i]);
				return;
			}
			if (it1->second < 1)
			{
				sipwarning("it1->second < 1. ItemID=%d", it->second.ItemIDs[i]);
				return;
			}
			it1->second --;
		}
	}

	// 검사
	for (int i = 0; i < Count; i ++)
	{
		// 인벤검사
		if ( !IsValidInvenPos(pInvenPoses[i]) )
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid inven position for buying item. FID=%d, InvenPos=%d", FID, pInvenPoses[i]);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		int		InvenIndex = GetInvenIndexFromInvenPos(pInvenPoses[i]);

		const MST_ITEM *tempMstItem = GetMstItemInfo(pItemIDs[i]);
		if ( tempMstItem == NULL )
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid itemid for buying item : FID=%d, ItemID=%d", FID, pItemIDs[i]);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}

		if(pItemCounts[i] == 0) 
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid ItemCount for buying item. FID=%d, ItemID=%d, ItemCount=%d, InvenPos=%d", FID, pItemIDs[i], pItemCounts[i], pInvenPoses[i]);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;		
		}
		if(pInven->Items[InvenIndex].ItemID != 0)
		{
			if(tempMstItem->OverlapFlag == 0)
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"This item can't overlap!! FID=%d, ItemID=%d", FID, pItemIDs[i]);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
			if((pInven->Items[InvenIndex].ItemID != pItemIDs[i]) || (pInven->Items[InvenIndex].MoneyType != MONEYTYPE_UMONEY))
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid inven pos - not equal item!! FID=%d, InvenPos=%d, ItemID=%d, OldItemID=%d, OldItemMoneyType=%d", 
					FID, InvenIndex, pItemIDs[i], pInven->Items[InvenIndex].ItemID, pInven->Items[InvenIndex].MoneyType);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
			if(pItemCounts[i] + pInven->Items[InvenIndex].ItemCount > tempMstItem->OverlapMaxNum)
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid ItemCount for buying item. FID=%d, ItemID=%d, ItemCount=%d, InvenPos=%d", FID, pItemIDs[i], pItemCounts[i], pInvenPoses[i]);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;		
			}
		}

		for(int j = 0; j < i; j ++)
		{
			if(pInvenPoses[i] == pInvenPoses[j])
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"Duplicate Invenpos!!!");
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;		
			}
		}

		// RoomStore검사
		std::map<uint32, uint16>::iterator it1 = tempStore.mapStoreItems.find(pItemIDs[i]);
		if ((it1 == tempStore.mapStoreItems.end()) || (it1->second < pItemCounts[i]))
		{
			// 실패
			uint32	ErrorCode = E_ROOMSTORE_GET_NOITEM;
			CMessage	msgOut(M_SC_ROOMSTORE_GETITEM);
			msgOut.serial(FID, ErrorCode);
			CUnifiedNetwork::getInstance()->send(_sid, msgOut);
			return;
		}
		it1->second -= pItemCounts[i];
	}

	// 검사OK
	// RoomStore 처리
	for (int i = 0; i < Count; i ++)
	{
		std::map<uint32, uint16>::iterator it = m_RoomStore.mapStoreItems.find(pItemIDs[i]);
		if (it == m_RoomStore.mapStoreItems.end())
		{
			sipwarning("it == m_RoomStore.mapItems.end(). FID=%d, ItemID=%d", FID, pItemIDs[i]);
			return;
		}
		if (it->second < pItemCounts[i])
		{
			sipwarning("it->second < pItemCounts[i]. FID=%d, ItemID=%d", FID, pItemIDs[i]);
			return;
		}
		it->second -= pItemCounts[i];
		if (it->second <= 0)
			m_RoomStore.mapStoreItems.erase(it);

		m_RoomStore.AllItemCount -= pItemCounts[i];
	}

	m_RoomStore.StoreSyncCode = (uint32)GetDBTimeSecond();

	// RoomStore DB에 보관
	SaveRoomStore();

	// RoomStore가 변함을 사용자들에게 통지
	SendRoomstoreStatus();

	// RoomStore사용기록 갱신
	uint8	ItemCount[MAX_ITEM_PER_ACTIVITY];
	for (int i = 0; i < Count; i ++)
		ItemCount[i] = (uint8) pItemCounts[i];
	SaveRoomStoreHistory(ROOMSTORE_HISTORY_GET, FID, 0, L"", Count, pItemIDs, ItemCount);


	// 인벤처리
	for(int i = 0; i < Count; i++)
	{
		int		InvenIndex = GetInvenIndexFromInvenPos(pInvenPoses[i]);
		if(pInven->Items[InvenIndex].ItemID == 0)
		{
			pInven->Items[InvenIndex].ItemID = pItemIDs[i];
			pInven->Items[InvenIndex].ItemCount = pItemCounts[i];
			pInven->Items[InvenIndex].GetDate = 0; // getDate; - 처음엔 미사용아이템으로 설정한다.
			pInven->Items[InvenIndex].UsedCharID = 0;
			pInven->Items[InvenIndex].MoneyType = MONEYTYPE_UMONEY;
		}
		else
		{
			pInven->Items[InvenIndex].ItemCount += pItemCounts[i];
			pInven->Items[InvenIndex].GetDate = 0; // getDate; - 처음엔 미사용아이템으로 설정한다.
			pInven->Items[InvenIndex].UsedCharID = 0;
		}

		// Log
/*		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
		if ( mstItem == NULL )
		{
			sipwarning(L"GetMstItemInfo failed. ItemID = %d", ItemID);
			return;
		}
		DBLOG_BuyItem(FID, 0, ItemID, ItemNum, mstItem->Name, UPrice, GPrice);*/
	}
	if(!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
		return;
	}

	uint32	ErrorCode = 0;
	CMessage	msgOut(M_SC_ROOMSTORE_GETITEM);
	msgOut.serial(FID, ErrorCode);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

void CRoomWorld::CheckRoomcreateHoliday()
{
	m_bIsRoomcreateDay = false;

	std::string	strDate;
	char	bufDate[50];

	uint32	today = GetDBTimeDays();
	int		year = (today / 365) + 1970;

	sprintf(bufDate, "%d%s", year, m_CreationDay.c_str() + 4);
	strDate = bufDate;
	if (today == getDaysSince1970(strDate))
		m_bIsRoomcreateDay = true;
	else
	{
		year ++;
		sprintf(bufDate, "%d%s", year, m_CreationDay.c_str() + 4);
		strDate = bufDate;
		if (today == getDaysSince1970(strDate))
			m_bIsRoomcreateDay = true;
	}
}

void CRoomWorld::RefreshRoomEvent()
{
	uint32	curTime = (uint32) GetDBTimeSecond();
	int		i = 0;

	ROOMEVENT_STATUS st = GetRoomEventStatus();
	switch (st)
	{
	case ROOMEVENT_STATUS_NONE:
		break;
	case ROOMEVENT_STATUS_READY:
		for (i = 0; i < MAX_ROOMEVENT_COUNT; i ++)
		{
			if (curTime < m_RoomEvent[i].nRoomEventStartTime)
				break;
			if (curTime < m_RoomEvent[i].nRoomEventStartTime + m_RoomEvent[i].nRoomEventTimeDuration)
			{
				bool bCanStart = true;
				if (IsMusicEvent(m_RoomEvent[i].byRoomEventType)) // (m_RoomEvent[i].byRoomEventType == ROOM_EVENT_MUSIC)
				{
					for(RoomChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
					{
						if (pChannelData->m_Activitys[ACTPOS_YISHI-1].FID != 0 ||
							pChannelData->m_Activitys[ACTPOS_AUTO2-1].FID != 0 ||
							pChannelData->m_Activitys[ACTPOS_YISHI-1].WaitFIDs.size() > 0)
						{
							bCanStart = false;
							break;
						}
					}
				}
				if (!bCanStart)
				{
					m_RoomEvent[i].nRoomEventStartTime = curTime + 30;
					break;
				}

				StartRoomEvent(i);
				break;
			}
		}
		if (i >= MAX_ROOMEVENT_COUNT)
			SetRoomEventStatus(ROOMEVENT_STATUS_NONE);
		break;
	case ROOMEVENT_STATUS_DOING:
		{
			int nI = GetCurrentDoingRoomEventIndex();
			if (curTime >= m_RoomEvent[nI].nRoomEventStartTime + m_RoomEvent[nI].nRoomEventTimeDuration)
			{
				{
					FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ROOMEVENT_END)
					FOR_END_FOR_FES_ALL()
				}

				if (IsMusicEvent(m_RoomEvent[nI].byRoomEventType)) // (m_RoomEvent[nI].byRoomEventType == ROOM_EVENT_MUSIC)
				{
					for(RoomChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
					{
						_RoomActivity	*waitActivity = &pChannelData->m_Activitys[ACTPOS_YISHI - 1];
						uint8	waitActPos = ACTPOS_YISHI;
						bool bValidItTarget = false;
						uint32		RemainCount = 0;
						TWaitFIDList::iterator	it, itTarget;
						for (it = waitActivity->WaitFIDs.begin(); it != waitActivity->WaitFIDs.end(); it ++)
						{
							if (IsSingleActionType(it->ActivityType))
							{ // 개별행사인 경우
								CMessage	msgOut(M_SC_ACT_WAIT);
								uint32		ErrorCode = 0;
								uint32	WaitFID = it->ActivityUsers[0].FID;
								FAMILY	*pFamily = GetFamilyData(WaitFID);
								if (pFamily == NULL)
								{
									sipwarning("GetFamilyData = NULL. FID=%d", WaitFID);
								}
								else
								{
									msgOut.serial(WaitFID);
									if (RemainCount != 0)
										msgOut.serial(waitActPos, ErrorCode, RemainCount);
									else
									{
										// 다음 대기자에게 행사시작 통지
										waitActivity->init(WaitFID,AT_SIMPLERITE,0,0);//, family->m_Name);
										bValidItTarget = true;
										itTarget = it;
										{
											msgOut.serial(waitActPos, ErrorCode, RemainCount);

											//sipdebug("Start Activity: ActPos=%d, FID=%d", waitActPos, it->ActivityUsers[0].FID); byy krs
										}
									}

									CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
								}
							}
							else if (IsMutiActionType(it->ActivityType))
							{ // 집체행사인 경우
								if (RemainCount == 0)
								{
									// 행사시작상태로 설정한다.
//BUG									waitActivity->init(it->ActivityUsers[0].FID, AT_MULTIRITE,0,0);
									waitActivity->init(it->ActivityUsers[0].FID, it->ActivityType,0,0);
									for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
									{
										if(it->ActivityUsers[j].FID != 0)
											waitActivity->ActivityUsers[j] = it->ActivityUsers[j];
									}

									bValidItTarget = true;
									itTarget = it;
								}

								// 행사참가자들에게 M_SC_MULTIACT_WAIT 를 보낸다.
								uint32		ErrorCode = 0;
								for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
								{
									if (it->ActivityUsers[j].FID == 0)
										continue;

									FAMILY	*pFamily = GetFamilyData(it->ActivityUsers[j].FID);
									if (pFamily == NULL)
									{
										sipwarning("GetFamilyData = NULL. FID=%d", it->ActivityUsers[j].FID);
										continue;
									}

									CMessage	msgOut(M_SC_MULTIACT_WAIT);
									msgOut.serial(it->ActivityUsers[j].FID, waitActPos, ErrorCode, RemainCount);
									CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
								}
							}
							RemainCount ++;
						}
						if (bValidItTarget)
						{
							waitActivity->WaitFIDs.erase(itTarget);
						}
					}
				}
				extern	uint32	BadFID;
				extern	MAP_OUT_TAOCAN_FOR_AUTOPLAY	map_OUT_TAOCAN_FOR_AUTOPLAY;
				if (BadFID != 0)
				{
					if (map_OUT_TAOCAN_FOR_AUTOPLAY.find(BadFID) != map_OUT_TAOCAN_FOR_AUTOPLAY.end())
						map_OUT_TAOCAN_FOR_AUTOPLAY.erase(BadFID);
				}
				SetRoomEventStatus(ROOMEVENT_STATUS_READY);
			}
		}
		break;
	}
}

void CRoomWorld::StartRoomEvent(int i)
{
	SetRoomEventStatus(ROOMEVENT_STATUS_DOING, i);

	m_RoomEvent[i].nRoomEventStartTime = (uint32) GetDBTimeSecond();

	if (IsMusicEvent(m_RoomEvent[i].byRoomEventType)) // (m_RoomEvent[i].byRoomEventType == ROOM_EVENT_MUSIC)
	{
		// 선녀악기치기 인 경우 방에 악기를 치는 사용자가 있으면 다른것으로 교체한다.
		for(RoomChannelData	*pChannel = GetFirstChannel(); pChannel != NULL; pChannel = GetNextChannel())
		{
			if (!pChannel->m_CurMusicFIDs.empty())
			{
				//m_RoomEvent[i].byRoomEventType = (int)(rand() * (ROOM_EVENT_COUNT - 1) / RAND_MAX);
				//if (m_RoomEvent[i].byRoomEventType >= ROOM_EVENT_MUSIC)
				//{
				//	m_RoomEvent[i].SetInfo((m_RoomEvent[i].byRoomEventType+1), m_RoomEvent[i].byRoomEventPos, m_RoomEvent[i].nRoomEventStartTime);
				//}
				m_RoomEvent[i].SetInfo(5, m_RoomEvent[i].byRoomEventPos, m_RoomEvent[i].nRoomEventStartTime, false);	// 5 = Random Value, not MusicEvent
				break;
			}
		}
	}

	// 방의 사용자들에게 통지한다.
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ROOMEVENT)
		msgOut.serial(m_RoomEvent[i].byRoomEventType, m_RoomEvent[i].byRoomEventPos, m_RoomEvent[i].nRoomEventStartTime);
	FOR_END_FOR_FES_ALL()

	//sipdebug("Start room event : RoomID=%d, EventType=%d, EvenetPos=%d", m_RoomID, m_RoomEvent[i].byRoomEventType, m_RoomEvent[i].byRoomEventPos) ; byy krs
}

void	CRoomWorld::NotifyChangeRoomInfoForManager()
{
	CMessage	msgOut(M_NT_EXPROOM_INFO_CHANGED);
	msgOut.serial(m_RoomID, m_RoomName);
	msgOut.serial(m_SceneID, m_MasterID, m_MasterName, m_PowerID, m_Exp, m_Level);
	msgOut.serial(m_Deceased1.m_Surnamelen, m_Deceased1.m_Name, m_Deceased2.m_Surnamelen, m_Deceased2.m_Name, m_PhotoType);
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
}

void cb_M_CS_MOUNT_LUCKANIMAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	T_ANIMALID	AID;
	_msgin.serial(FID, AID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	inManager->on_M_CS_MOUNT_LUCKANIMAL(FID, AID, _tsid);
}
void cb_M_CS_UNMOUNT_LUCKANIMAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);

	CRoomWorld *inManager = GetOpenRoomWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	inManager->on_M_CS_UNMOUNT_LUCKANIMAL(FID, _tsid);
}

void	CRoomWorld::on_M_CS_MOUNT_LUCKANIMAL(T_FAMILYID _FID, T_ANIMALID AID, SIPNET::TServiceId _tsid)
{
	RoomChannelData*	pChannelData = GetFamilyChannel(_FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel() = NULL. FID=%d", _FID);
		return;
	}

	bool bSuccess = true;
	if (m_LuckID_Lv3 == 0)
	{
		bSuccess = false;
	}
	{
		if (pChannelData->m_MountLuck3FID != 0)
		{
			bSuccess = false;
		}
		else
		{
			MAP_ROOMANIMAL::iterator itAnimal = pChannelData->m_RoomAnimal.find(AID);
			if (itAnimal != pChannelData->m_RoomAnimal.end())
			{
				bSuccess = true;
				pChannelData->m_MountLuck3FID = _FID;
				pChannelData->m_MountLuck3AID = AID;
				ROOMANIMAL &animal = itAnimal->second;
				pChannelData->SetAnimalControlType(AID, SCT_NO);
			}
			else
			{
				sipwarning("There is no animal : fid=%d, roomid=%d, animalid=%d", _FID, m_RoomID, AID);
			}
		}
	}

	if (bSuccess == true)
	{
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MOUNT_LUCKANIMAL)
			msgOut.serial(_FID, AID, bSuccess);
		FOR_END_FOR_FES_ALL()
	}
	else
	{
		uint32	zero = 0;
		CMessage	msg(M_SC_MOUNT_LUCKANIMAL);
		msg.serial(_FID, zero);
		msg.serial(_FID, AID, bSuccess);
		CUnifiedNetwork::getInstance()->send(_tsid, msg);
	}

}

void	CRoomWorld::on_M_CS_UNMOUNT_LUCKANIMAL(T_FAMILYID _FID, SIPNET::TServiceId _tsid)
{
	RoomChannelData*	pChannelData = GetFamilyChannel(_FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel() = NULL. FID=%d", _FID);
		return;
	}
	if (pChannelData->m_MountLuck3FID == _FID)
	{
		pChannelData->ResetMountAnimalInfo();
	}
}



// Command
SIPBASE_CATEGORISED_COMMAND(OROOM_S, opinfo, "displays all information of open room", "")
{
	uint32	uRoomNum = map_RoomWorlds.size();
	log.displayNL ("OpenRoom number = %d :", uRoomNum);

	TChannelWorlds::iterator It;
	for (It = map_RoomWorlds.begin(); It != map_RoomWorlds.end(); It++)
	{
		CChannelWorld*		pChannel = It->second.getPtr();
		if (pChannel->m_WorldType != WORLD_ROOM)
			continue;
		CRoomWorld	*rw = (CRoomWorld*)(pChannel);
		log.displayNL (L"------ Room(%d), Name(%s) ------ :", rw->m_RoomID, rw->m_RoomName.c_str());
		uint32	uFamilyNum = rw->m_mapFamilys.size();
		log.displayNL (L"    Family number = %d :", uFamilyNum);
		map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator itfamily;
		for (itfamily = rw->m_mapFamilys.begin(); itfamily != rw->m_mapFamilys.end(); itfamily++)
		{
			FAMILY	*f = GetFamilyData(itfamily->first);
			if (f == NULL)
				continue;

			log.displayNL (L"        Family name = %s :", f->m_Name.c_str());
		}
	}
	return true;
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, setTestRoomEvent, "set music event to room after 1 minute", "RoomID EventID")
{
	if(args.size() < 1)	return false;

	string v1	= args[0];
	uint32 roomID = atoui(v1.c_str());
	string v2	= args[1];
	uint32 eventID = atoui(v2.c_str());

	CChannelWorld		*roomManager = GetChannelWorld(roomID);
	if (roomManager == NULL)
	{
		log.displayNL ("No RoomManager=%d", roomID);
		return true;
	}
	roomManager->StartRoomEventByGM((uint8)eventID);

	log.displayNL ("OK");
	return true;
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, setJiShangDengTest, "set JiShangDeng test state", "")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	uint32 val = atoui(v.c_str());

	if (val == 0)
	{
		bJiShangDengTest = false;
	}
	else
		bJiShangDengTest = true;

	log.displayNL ("bJiShangDengTest = %d", bJiShangDengTest);
	return true;
}

//SIPBASE_CATEGORISED_COMMAND(OROOM_S, testSpStart, "test SP with Shrd_Family_UpdateLevelExp", "FID Level Exp")
//{
//	if(args.size() < 3)	return false;
//
//	string v	= args[0];
//	uint32 FID = atoui(v.c_str());
//	v	= args[1];
//	uint32 Level = atoui(v.c_str());
//	v	= args[2];
//	uint32 Exp = atoui(v.c_str());
//
//	bTestSP = true;
//	testSPProc(FID, Level, Exp);
//
//	return true;
//}
//
//SIPBASE_CATEGORISED_COMMAND(OROOM_S, testSpStop, "test SP stop", "")
//{
//	bTestSP = false;
//	return true;
//}
