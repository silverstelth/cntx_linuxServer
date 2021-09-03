#pragma warning (disable: 4267)
#pragma warning (disable: 4244)

#include "misc/types_sip.h"

#include <fcntl.h>
#include <sys/stat.h>

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#	include <direct.h>
#	undef max
#	undef min
#else
#	include <unistd.h>
#	include <errno.h>
#endif

#include <string>
#include <list>

#include "misc/DBCaller.h"
#include "net/service.h"
#include "net/unified_network.h"
#include "net/naming_client.h"

#include "SharedActWorld.h"
#include "room.h"
#include "ReligionRoom.h"
#include "PublicRoom.h"
#include "../Common/DBLog.h"
#include "../Common/netCommon.h"
#include "World.h"
#include "misc/i_xml.h"
#include "outRoomKind.h"
#include "openroom.h"
#include "contact.h"
#include "Inven.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller	*DBCaller;

static void	DBCB_DBGetPublicroomFrameList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet);

/****************************************************************************
* CommonRoom class
****************************************************************************/

void onConnectFES(const std::string &serviceName, TServiceId tsid, void *arg)
{
	// 창조된 종교방목록을 FS에 통지한다.
	uint8	flag = 1;	// Room Created
	for(TChannelWorlds::iterator it = map_RoomWorlds.begin(); it != map_RoomWorlds.end(); it ++)
	{
		uint32	RoomID = it->first;
		CMessage	msgOut(M_NT_ROOM);
		msgOut.serial(RoomID, flag);
		CUnifiedNetwork::getInstance()->send(tsid, msgOut);
	}
}

// 모든 방정보를 보관하고 삭제한다.
void DestroyCommonRoomWorlds()
{
	TChannelWorlds::iterator it = map_RoomWorlds.begin();
	while (it != map_RoomWorlds.end())
	{
		map_RoomWorlds.erase(it);
		it = map_RoomWorlds.begin();
	}
}

static CSharedActWorld* GetSharedActWorld(T_ROOMID _rid)
{
	CChannelWorld* pChannelWorld = GetChannelWorld(_rid);
	if (pChannelWorld == NULL)
		return NULL;

	if (pChannelWorld->m_RoomID >= MIN_RELIGIONROOMID || pChannelWorld->m_RoomID <= MAX_RELIGIONROOMID)
		return dynamic_cast<CSharedActWorld*>(pChannelWorld);

	return NULL;
}

static CSharedActWorld* GetSharedActWorldFromFID(T_FAMILYID _fid)
{
	CChannelWorld* pChannelWorld = GetChannelWorldFromFID(_fid);
	if (pChannelWorld == NULL)
		return NULL;

	if (pChannelWorld->m_RoomID >= MIN_RELIGIONROOMID || pChannelWorld->m_RoomID < MAX_RELIGIONROOMID)
		return dynamic_cast<CSharedActWorld*>(pChannelWorld);

	return NULL;
}

TUnifiedCallbackItem CommonroomCallbackArray[] =
{
	{ M_CS_REQEROOM,			cb_M_CS_REQEROOM_Rroom },

	{ M_CS_ADDWISH,				cb_M_CS_ADDWISH },			// for ReligionRoom
	{ M_CS_WISH_LIST,			cb_M_CS_WISH_LIST },		// for ReligionRoom
	{ M_CS_MY_WISH_LIST,		cb_M_CS_MY_WISH_LIST },		// for ReligionRoom
	{ M_CS_MY_WISH_DATA,		cb_M_CS_MY_WISH_DATA },		// for ReligionRoom

	{ M_CS_ACT_REQ,				cb_M_CS_ACT_REQ },
	{ M_CS_ACT_WAIT_CANCEL,		cb_M_CS_ACT_WAIT_CANCEL },
	{ M_CS_ACT_RELEASE,			cb_M_CS_ACT_RELEASE },
	{ M_CS_ACT_START,			cb_M_CS_ACT_START },

	//{ M_CS_R_PAPER_START,		cb_M_CS_R_PAPER_START },
	//{ M_CS_R_PAPER_PLAY,		cb_M_CS_R_PAPER_PLAY },
	//{ M_CS_R_PAPER_END,			cb_M_CS_R_PAPER_END },

	{ M_CS_VIRTUE_LIST,			cb_M_CS_VIRTUE_LIST },		// for ReligionRoom
	{ M_CS_VIRTUE_MY,			cb_M_CS_VIRTUE_MY },		// for ReligionRoom

	{ M_CS_R_ACT_INSERT,		cb_M_CS_R_ACT_INSERT },
	{ M_CS_R_ACT_MODIFY,		cb_M_CS_R_ACT_MODIFY },
	{ M_CS_R_ACT_LIST,			cb_M_CS_R_ACT_LIST },
	{ M_CS_R_ACT_MY_LIST,		cb_M_CS_R_ACT_MY_LIST },
	{ M_CS_R_ACT_PRAY,			cb_M_CS_R_ACT_PRAY },

	{ M_NT_ROOM_CREATE_REQ,		cb_M_NT_ROOM_CREATE_REQ_Rroom },

	// 공공구역에서 Frame에 걸려있는 체험방자료
	{ M_GMCS_PUBLICROOM_FRAME_SET,		cb_M_GMCS_PUBLICROOM_FRAME_SET },	// for PublicRoom
	{ M_CS_PUBLICROOM_FRAME_LIST,		cb_M_CS_PUBLICROOM_FRAME_LIST },	// for PublicRoom
	{ M_GMCS_LARGEACT_SET,				cb_M_GMCS_LARGEACT_SET },			// for PublicRoom
	{ M_GMCS_LARGEACT_DETAIL,			cb_M_GMCS_LARGEACT_DETAIL },		// for PublicRoom

	// 공공구역에서 대형행사조직
	//{ M_CS_LARGEACT_LIST,				cb_M_CS_LARGEACT_LIST },			// for PublicRoom by krs
	{ M_CS_LARGEACT_REQUEST,			cb_M_CS_LARGEACT_REQUEST },			// for PublicRoom
	{ M_CS_LARGEACT_CANCEL,				cb_M_CS_LARGEACT_CANCEL },			// for PublicRoom
	{ M_CS_LARGEACT_STEP,				cb_M_CS_LARGEACT_STEP },			// for PublicRoom

	// 불교구역대형행사관련
	{ M_GMCS_SBY_LIST,					cb_M_GMCS_SBY_LIST },		// for ReligionRoom
	{ M_GMCS_SBY_DEL,					cb_M_GMCS_SBY_DEL },		// for ReligionRoom
	{ M_GMCS_SBY_EDIT,					cb_M_GMCS_SBY_EDIT },		// for ReligionRoom
	{ M_GMCS_SBY_ADD,					cb_M_GMCS_SBY_ADD },		// for ReligionRoom

	{ M_CS_SBY_ADDREQ,					cb_M_CS_SBY_ADDREQ },		// for ReligionRoom
	{ M_CS_SBY_EXIT,					cb_M_CS_SBY_EXIT },			// for ReligionRoom
	{ M_CS_SBY_OVER,					cb_M_CS_SBY_OVER },			// for ReligionRoom

	{ M_CS_PBY_MAKE,					cb_M_CS_PBY_MAKE },			// for ReligionRoom
	{ M_CS_PBY_SET,						cb_M_CS_PBY_SET },			// for ReligionRoom
	{ M_CS_PBY_JOINSET,					cb_M_CS_PBY_JOINSET },		// for ReligionRoom
	{ M_CS_PBY_ADDREQ,					cb_M_CS_PBY_ADDREQ },		// for ReligionRoom
	{ M_CS_PBY_EXIT,					cb_M_CS_PBY_EXIT },			// for ReligionRoom
	{ M_CS_PBY_OVERUSER,				cb_M_CS_PBY_OVERUSER },		// for ReligionRoom
	{ M_CS_PBY_CANCEL,					cb_M_CS_PBY_CANCEL },		// for ReligionRoom
	{ M_CS_PBY_UPDATESTATE,				cb_M_CS_PBY_UPDATESTATE },	// for ReligionRoom

	{ M_CS_PBY_HISLIST,					cb_M_CS_PBY_HISLIST },		// for ReligionRoom
	{ M_CS_PBY_MYHISLIST,				cb_M_CS_PBY_MYHISLIST },	// for ReligionRoom
	{ M_CS_PBY_HISINFO,					cb_M_CS_PBY_HISINFO },		// for ReligionRoom
	{ M_CS_PBY_SUBHISINFO,				cb_M_CS_PBY_SUBHISINFO },	// for ReligionRoom
	{ M_CS_PBY_HISMODIFY,				cb_M_CS_PBY_HISMODIFY },	// for ReligionRoom
	{ M_CS_PBY_SUBHISMODIFY,			cb_M_CS_PBY_SUBHISMODIFY },	// for ReligionRoom

	{ M_CS_REQLUCKINPUBROOM,			cb_M_CS_REQLUCKINPUBROOM },			// for PublicRoom
	{ M_CS_GETBLESSCARDINPUBROOM,		cb_M_CS_GETBLESSCARDINPUBROOM },	// for PublicRoom
	{ M_CS_HISLUCKINPUBROOM,			cb_M_CS_HISLUCKINPUBROOM },			// for PublicRoom

	{ M_NT_RESGIVEBLESSCARD,			cb_M_NT_RESGIVEBLESSCARD },			// for PublicRoom

	{ M_CS_NIANFO_BEGIN,		cb_M_CS_NIANFO_BEGIN },
	{ M_CS_NIANFO_END,			cb_M_CS_NIANFO_END },
	{ M_CS_NEINIANFO_BEGIN,		cb_M_CS_NEINIANFO_BEGIN },
	{ M_CS_NEINIANFO_END,		cb_M_CS_NEINIANFO_END },

	{ M_NT_AUTOPLAY3_TIANDI,	cb_M_NT_AUTOPLAY3_TIANDI },			// for PublicRoom
};

class CCommonRoomService : public IService
{
public:

	void init()
	{
		init_ShardActWorldService();

		CUnifiedNetwork::getInstance()->setServiceUpCallback(FRONTEND_SHORT_NAME, onConnectFES, 0);
	}

	bool update ()
	{
		RefreshReligionRoom();
		RefreshPublicRoom();
		//DoEverydayWork();

		update_ShardActWorldService();
		return true;
	}

	void release()
	{
		//sipinfo("CCommonRoomService release Enter"); byy krs
		DestroyCommonRoomWorlds();
		//sipinfo("CCommonRoomService release Doing"); byy krs

		release_ShardActWorldService();
		//sipinfo("CCommonRoomService release Leave"); byy krs
	}
};

void run_commonroom_service(int argc, const char **argv)
{
	CCommonRoomService *scn = new CCommonRoomService;
	scn->setArgs (argc, argv);
	createDebug (NULL, false);
	scn->setCallbackArray (CommonroomCallbackArray, sizeof(CommonroomCallbackArray)/sizeof(CommonroomCallbackArray[0]));
#ifdef SIP_OS_WINDOWS
#	if _MSC_VER >= 1900
		sint retval = scn->main(RROOM_SHORT_NAME, RROOM_LONG_NAME, "", 0, "", "", __DATE__ " " __TIME__);
#	else
		sint retval = scn->main(RROOM_SHORT_NAME, RROOM_LONG_NAME, "", 0, "", "", __DATE__" "__TIME__);
#	endif
		delete scn;
#else
	string strDateTime = string(__DATE__) + " " + string(__TIME__);
	sint retval = scn->main(RROOM_SHORT_NAME, RROOM_LONG_NAME, "", 0, "", "", strDateTime.c_str());
#endif
}

void run_commonroom_service(LPSTR lpCmdLine)
{
	CCommonRoomService *scn = new CCommonRoomService;
	scn->setArgs (lpCmdLine);
	createDebug (NULL, false);
	scn->setCallbackArray (CommonroomCallbackArray, sizeof(CommonroomCallbackArray)/sizeof(CommonroomCallbackArray[0]));
#ifdef SIP_OS_WINDOWS
#	if _MSC_VER >= 1900
		sint retval = scn->main(RROOM_SHORT_NAME, RROOM_LONG_NAME, "", 0, "", "", __DATE__ " " __TIME__);
#	else
		sint retval = scn->main(RROOM_SHORT_NAME, RROOM_LONG_NAME, "", 0, "", "", __DATE__" "__TIME__);
#	endif
		delete scn;
#else
	string strDateTime = string(__DATE__) + " " + string(__TIME__);
	sint retval = scn->main(RROOM_SHORT_NAME, RROOM_LONG_NAME, "", 0, "", "", strDateTime.c_str());
#endif
}

void cb_M_NT_ROOM_CREATE_REQ_Rroom(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_ROOMID		RoomID;

	_msgin.serial(RoomID);

	switch (RoomID)
	{
	case ROOM_ID_BUDDHISM:
		CreateReligionRoom(RoomID);
		break;
	case ROOM_ID_PUBLICROOM:
	case ROOM_ID_KONGZI:
	case ROOM_ID_HUANGDI:
		CreatePublicRoom(RoomID);
		break;
	default:
		sipwarning("Invalid CommonRoom ID. RoomID=%d", RoomID);
		break;
	}
}

void cb_M_CS_REQEROOM_Rroom(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	uint32			ChannelID;
	std::string		authkey;
	uint16			wsid;

	_msgin.serial(FID, RoomID);
	NULLSAFE_SERIAL_STR(_msgin, authkey, MAX_LEN_ROOM_VISIT_PASSWORD, FID);
	_msgin.serial(ChannelID, wsid);

	TServiceId	client_FsSid(wsid);

	CChannelWorld *roomManager = GetChannelWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetChannelWorld = NULL. RoomID=%d, FID=%d", RoomID, FID);
		return;
	}

	roomManager->RequestEnterRoom(FID, "", client_FsSid);
}

// Require Activity ([b:ActPos])
static void cb_M_CS_ACT_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		ActPos;		_msgin.serial(ActPos);

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_CS_ACT_REQ(FID, ActPos, _tsid);
	}
}

// Activity Wait Cancel ([b:ActPos])
static void cb_M_CS_ACT_WAIT_CANCEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		ActPos;		_msgin.serial(ActPos);

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_CS_ACT_WAIT_CANCEL(FID, ActPos);
	}
}

// Release NPC
static void cb_M_CS_ACT_RELEASE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_CS_ACT_RELEASE(FID, _tsid);
	}
}

extern bool R_CheckActivityGongpinCount(uint8 ActPos, uint8 GongpinCount); // 2014/08/20
// Start Activity
static void cb_M_CS_ACT_START(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint8		ActPos;		_msgin.serial(ActPos);

	// 2014/06/27:start
	uint32		Param;			_msgin.serial(Param);
	uint8		GongpinCount;	_msgin.serial(GongpinCount);
	_ActivityStepParam	paramActStep;
	paramActStep.IsXDG = Param; // 2014/07/19
	paramActStep.GongpinCount = GongpinCount;
	//if (GongpinCount != 0)
	if ( R_CheckActivityGongpinCount(ActPos, GongpinCount) )
	{
		for (int i = 0; i < GongpinCount; i++)
		{
			uint32 GongpinItemID;
			_msgin.serial(GongpinItemID);
			paramActStep.GongpinsList.push_back(GongpinItemID);
		}
		_msgin.serial(paramActStep.BowType);
	}
	else
	{
		sipwarning(L"gongpin count is invalid. ActPos=%d, Count=%d", ActPos, GongpinCount);
		return;
	}
	// end

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_CS_ACT_START(FID, _tsid, &paramActStep); // 2014/06/11
	}
}

void cb_M_CS_R_ACT_INSERT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			SyncCode=0;
	uint8			ActionType, Secret, NpcID, ItemNum=0;
	ucstring		Pray = "";
	uint16			invenpos[MAX_ITEM_PER_ACTIVITY] = {0};

	_msgin.serial(FID, NpcID, ActionType, Secret);
	NULLSAFE_SERIAL_WSTR(_msgin, Pray, MAX_LEN_RELIGION_PRAYTEXT, FID);
	_msgin.serial(ItemNum);

	if (ItemNum > MAX_ITEM_PER_ACTIVITY)
	{
		// 비법발견
		ucstring ucUnlawContent = SIPBASE::_toString("Count is too big! count=%u", ItemNum).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	if (ItemNum > 0)
	{
		_msgin.serial(SyncCode);
		for (int i = 0; i < ItemNum; i ++)
		{
			_msgin.serial(invenpos[i]);
		}
	}

	// 방관리자를 얻는다
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	
	inManager->SaveYisiAct(FID, _tsid, NpcID, ActionType, Secret, Pray, ItemNum, SyncCode, invenpos, 0);

}

// Modify action's pray. ([d:ID][b:Secret][u:Pray])
void cb_M_CS_R_ACT_MODIFY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			PrayID;
	uint8			bSecret;
	ucstring		Pray = "";

	_msgin.serial(FID, PrayID, bSecret);
	NULLSAFE_SERIAL_WSTR(_msgin, Pray, MAX_LEN_RELIGION_PRAYTEXT, FID);
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	inManager->on_M_CS_R_ACT_MODIFY(FID, PrayID, bSecret, Pray);
}

// Request action list ([b:NpcID][d:Page][b:PageSize])
void cb_M_CS_R_ACT_LIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint8			NpcID, PageSize;
	uint32			Page;

	_msgin.serial(FID, NpcID, Page, PageSize);

	// 방관리자를 얻는다
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	inManager->on_M_CS_R_ACT_LIST(FID, NpcID, Page, PageSize, _tsid);
}

// Request my action list ([b:NpcID][d:Page])
void cb_M_CS_R_ACT_MY_LIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint8			NpcID, PageSize;
	uint32			Page;

	_msgin.serial(FID, NpcID, Page, PageSize);

	// 방관리자를 얻는다
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	inManager->on_M_CS_R_ACT_MY_LIST(FID, NpcID, Page, PageSize, _tsid);
}

// Request action's pray ([d:ID])
void cb_M_CS_R_ACT_PRAY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			ActID;

	_msgin.serial(FID, ActID);

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	inManager->on_M_CS_R_ACT_PRAY(FID, ActID, _tsid);
}


/****************************************************************************
*       ShardActWorld
****************************************************************************/

TUnifiedCallbackItem ShardActWorldCallbackArray[] =
{
	{ M_CS_MULTIACT2_ITEMS,		cb_M_CS_MULTIACT2_ITEMS },
	{ M_CS_MULTIACT2_ASK,		cb_M_CS_MULTIACT2_ASK },
	{ M_CS_MULTIACT2_JOIN,		cb_M_CS_MULTIACT2_JOIN },
};

void init_ShardActWorldService()
{
	init_ChannelWorldService();

	CUnifiedNetwork::getInstance()->addCallbackArray(ShardActWorldCallbackArray, sizeof(ShardActWorldCallbackArray)/sizeof(ShardActWorldCallbackArray[0]));
}

void update_ShardActWorldService()
{
	update_ChannelWorldService();
}

void release_ShardActWorldService()
{
	release_ChannelWorldService();
}


static int MakeActivityIndex(uint8 actpos, int deskpos, bool bActing)
{
	int flag = (bActing) ? 0x10000 : 0x20000;
	return	(actpos + (deskpos << 8) | flag);
}
static bool IsWatingFromActivityIndex(int index)
{
	if((index >> 16) != 1)
		return true;
	return false;
}
static uint8 GetActPosFromActivityIndex(int index)
{
	return (uint8)(index & 0xFF);
}
static int GetDeskPosFromActivityIndex(int index)
{
	return ((index >> 8) & 0xFF);
}

void	CSharedActChannelData::CancelActivityWaiting(uint8 ActPos)
{
	uint32	zero = 0;
	uint32	RemainCount = 1;// 의미가 없는 값이다
	uint32	MultiErrorCode = E_ACT_REMOVED;
	CSharedActivity &rActivity = m_Activitys[ActPos];
	for(TWaitFIDList::iterator it = rActivity.WaitFIDs.begin(); it != rActivity.WaitFIDs.end(); it++)
	{
		_RoomActivity_Wait curActivity = (*it);
		for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
		{
			if (curActivity.ActivityUsers[j].FID == 0)
				continue;

			FAMILY	*pMultiFamily = GetFamilyData(curActivity.ActivityUsers[j].FID);
			if (pMultiFamily == NULL)
			{
				sipwarning("GetFamilyData = NULL. FID=%d", curActivity.ActivityUsers[j].FID);
				continue;
			}
			
			if (IsMutiActionType(curActivity.ActivityType))
			{
				CMessage	msgOut(M_SC_MULTIACT_WAIT);
				msgOut.serial(curActivity.ActivityUsers[j].FID, ActPos, MultiErrorCode, RemainCount);
				CUnifiedNetwork::getInstance()->send(pMultiFamily->m_FES, msgOut);
			}
			else
			{
				uint8		DeskNo=0;
				CMessage	msgOut(M_SC_ACT_WAIT);
				msgOut.serial(curActivity.ActivityUsers[j].FID, ActPos, MultiErrorCode, RemainCount, DeskNo);
				CUnifiedNetwork::getInstance()->send(pMultiFamily->m_FES, msgOut);
			}
		}
	}
	rActivity.WaitFIDs.clear();
}

bool	CSharedActChannelData::IsDoingActivity(uint8 ActPos)
{
	CSharedActivity &rActivity = m_Activitys[ActPos];

	for(int i = 0; i < rActivity.ActivityDeskCount; i ++)
	{
		if(rActivity.ActivityDesk[i].FID != NOFAMILYID)
			return true;
	}
	return false;
}

int		CSharedActChannelData::GetActivityIndexOfUser(T_FAMILYID FID, uint8 ActPos)
{
	CSharedActivity &rActivity = m_Activitys[ActPos];

	// 행사진행중인가 검사
	for(int j = 0; j < rActivity.ActivityDeskCount; j ++)
	{
		if (rActivity.ActivityDesk[j].FID == 0)
			continue;
		if(rActivity.ActivityDesk[j].FID == FID)
			return MakeActivityIndex(ActPos, j, true);
		if(IsMutiActionType(rActivity.ActivityDesk[j].ActivityType))
		{
			for(int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
			{
				if (rActivity.ActivityDesk[j].ActivityUsers[i].FID == FID)
					return MakeActivityIndex(ActPos, j, true);
			}
		}
	}

	// 행사대기렬에서 검사
	for(uint32 j = 0; j < rActivity.WaitFIDs.size(); j ++)
	{
		_RoomActivity_Wait& info = rActivity.WaitFIDs[j];
		if(info.ActivityUsers[0].FID == FID)
			return MakeActivityIndex(ActPos, j, false);
		if(IsMutiActionType(info.ActivityType))
		{
			for(int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
			{
				if(info.ActivityUsers[i].FID == FID)
					return MakeActivityIndex(ActPos, j, false);
			}
		}

	}

	return -1;
}

int		CSharedActChannelData::GetActivityIndexOfUser(T_FAMILYID FID)
{
	for(int i = 0; i < m_ActivityCount; i ++)
	{
		int ret = GetActivityIndexOfUser(FID, i);
		if(ret >= 0)
			return ret;
	}
	return -1;
}

CSharedActivityDesk* CSharedActChannelData::GetActivityDesk(T_FAMILYID FID, uint8 *ActPos)
{
	int index = GetActivityIndexOfUser(FID);
	if(index < 0)
		return NULL;
	if(IsWatingFromActivityIndex(index))
		return NULL;

	*ActPos = GetActPosFromActivityIndex(index);
	int deskpos = GetDeskPosFromActivityIndex(index);
	return &m_Activitys[*ActPos].ActivityDesk[deskpos];
}

void	CSharedActChannelData::on_M_CS_ACT_WAIT_CANCEL(T_FAMILYID FID, uint8 ActPos)
{
	if(ActPos >= m_ActivityCount)
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"ActPos >= m_ActivityCount: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	ActWaitCancel(FID, ActPos, false, true);
}

void	CSharedActChannelData::on_M_CS_ACT_RELEASE(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	int		index = GetActivityIndexOfUser(FID);
	if(index < 0)
	{
		sipwarning("GetActivityIndexOfUser failed. ChannelID=%d, FID=%d, ret=%d", m_ChannelID, FID, index);
		return;
	}

	uint8	ActPos = GetActPosFromActivityIndex(index);

	ActWaitCancel(FID, ActPos, true, true);
}

void	CSharedActChannelData::on_M_CS_ACT_START(T_FAMILYID FID, SIPNET::TServiceId _tsid, _ActivityStepParam* pYisiParam)// 2014/06/11
{
	// 2014/06/11:start
	if (pYisiParam == NULL)
	{
		sipwarning("pYisiParam = NULL. FID=%d", FID);
		return;
	}
	// end

	uint8	ActPos;
	CSharedActivityDesk* rActivityDesk = GetActivityDesk(FID, &ActPos);
	if(rActivityDesk == NULL)
	{
		sipwarning("GetActivityDesk = NULL. ChannelID=%d, FID=%d, ActPos=%d", m_ChannelID, FID, ActPos);
		return;
	}

	// 2014/06/27:start:스텝정보들을 등록해둔다. 후에 들어오는 사람들에게 보내주기 위해
	rActivityDesk->ActivityStepParam = *pYisiParam;
//	if (pYisiParam->GongpinsList.size() != pYisiParam->GongpinCount || pYisiParam->GongpinCount == 0)
//		sipwarning(L"Gongpin count mismatch! listcount=%d, count=%d", pYisiParam->GongpinsList.size(), pYisiParam->GongpinCount);

// 	sipinfo(L"setting : FID=%d", FID); // must be deleted
// 	std::list<uint32>::iterator itlist;
// 	for (itlist = pYisiParam->GongpinsList.begin(); itlist != pYisiParam->GongpinsList.end(); itlist ++)
// 	{
// 		uint32 curitem = *itlist;
// 		sipinfo(L"gpid=%d", curitem);
// 	}
// 	sipinfo(L"goingpincount=%d, bowtype=%d, stepID=%d", pYisiParam->GongpinCount, pYisiParam->BowType, pYisiParam->StepID);
	// end

	//sipdebug("ActStart:FID=%d, ChannelID=%d, ActPos=%d, DeskNo=%d", FID, m_ChannelID, ActPos, rActivityDesk->DeskNo); byy krs
	if(1 >= rActivityDesk->bStartFlag)
	{
		rActivityDesk->bStartFlag = 2;
		uint8 acttype = rActivityDesk->ActivityType;
		{
			FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_ACT_START)
				msgOut.serial(ActPos, acttype, rActivityDesk->FID, rActivityDesk->DeskNo);
			FOR_END_FOR_FES_ALL()
		}

		// 2014/07/01:start
		{
			FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_ACT_NOW_START)
				msgOut.serial(ActPos, rActivityDesk->DeskNo, acttype, pYisiParam->IsXDG, rActivityDesk->FID);
				uint8 GongpinCount = pYisiParam->GongpinsList.size();
				msgOut.serial(GongpinCount);
				std::list<uint32>::iterator itlist;
				for (itlist = pYisiParam->GongpinsList.begin(); itlist != pYisiParam->GongpinsList.end(); itlist ++)
				{
					uint32 curitem = *itlist;
					msgOut.serial(curitem);
				}
				uint8	joinCount = 0;
				msgOut.serial(pYisiParam->BowType, joinCount, pYisiParam->StepID);
			FOR_END_FOR_FES_ALL()
		}
		// end
	}
	else
	{
		sipwarning("rActivityDesk->StartTime = 0");
	}
}

void CSharedActChannelData::on_M_CS_ACT_STEP(T_FAMILYID FID, uint8 ActPos, uint32 StepID)// 14/07/23
{
	//sipinfo(L"publicstart - Family (FID=%d) (ActPos:%d) (step %d)", FID, ActPos, StepID); // 14/07/23:must be deleted byy krs

	uint8	ActPos1;
	CSharedActivityDesk* rActivityDesk = GetActivityDesk(FID, &ActPos1);
	if(rActivityDesk == NULL)
	{
		sipwarning("GetActivityDesk = NULL. ChannelID=%d, FID=%d, ActPos=%d", m_ChannelID, FID, ActPos1);
		return;
	}

	if (ActPos1 != ActPos)
	{
		sipinfo(L"public actpos step error actpos1=%d and actpos=%d are different!", ActPos1, ActPos);
//		return;
	}

	rActivityDesk->ActivityStepParam.StepID = StepID;

	// 사용자들에게 통지
	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_ACT_STEP)
	{
		msgOut.serial(ActPos, StepID, FID, rActivityDesk->DeskNo);
	}
	FOR_END_FOR_FES_ALL()

//	sipinfo(L"publicend - Family (id:%d) (ActPos:%d) (DeskNo:%d) (step %d)", FID, ActPos, rActivityDesk->DeskNo, StepID); // 2014/06/09:must be deleted
}

void	CSharedActChannelData::ActWaitCancel(T_FAMILYID FID, uint8 ActPos, uint8 SuccessFlag, bool bUnconditional)
{
	int		ret = GetActivityIndexOfUser(FID, ActPos);
	if(ret < 0)
	{
		// sipdebug("Can't find in waiting list. FID=%d, ChannelID=%d, ActPos=%d", FID, ChannelID, ActPos);
		return;
	}

	CSharedActivity &rActivity = m_Activitys[ActPos];

	uint8	index = GetDeskPosFromActivityIndex(ret);
	uint8	DeskNo = 0xFF;
	if(!IsWatingFromActivityIndex(ret))
	{
		// 행사진행중, index = DeskNo
		if (FID == rActivity.ActivityDesk[index].FID)
		{
			DeskNo = index;
			CompleteActivity(ActPos, index, SuccessFlag);
		}
		else
		{
			if (IsMutiActionType(rActivity.ActivityDesk[index].ActivityType))
			{
				_RoomActivity_Family* pActivityUsers = rActivity.ActivityDesk[index].ActivityUsers;
				SendMULTIACT_CANCEL(ActPos, pActivityUsers, FID, bUnconditional);

				CheckMultiactAllStarted(ActPos, index);
				//if (0 == rActivity.ActivityDesk[index].bStartFlag)
				//{
				//	bool bAllStarted = true;
				//	for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
				//	{
				//		if((rActivity.ActivityDesk[index].ActivityUsers[i].FID != 0) && (rActivity.ActivityDesk[index].ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_START))
				//			bAllStarted = false;
				//	}
				//	if (bAllStarted)
				//		SetMultiActStarted(ActPos, rActivity.ActivityDesk[index]);
				//}
			}
		}
	}
	else
	{
		_RoomActivity_Wait& waitinfo = rActivity.WaitFIDs[index];
		if (IsMutiActionType(waitinfo.ActivityType))
		{
			if(waitinfo.ActivityUsers[0].FID == FID)
			{
//				SendMULTIACT_RELEASE(ActPos, waitinfo.ActivityUsers, SuccessFlag);
				// 집체행사의 Host가 탈퇴하였으므로 행사참가자들에게 M_SC_MULTIACT_RELEASE을 보낸다.
				uint8		Count = 0;
				for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
				{
					if(waitinfo.ActivityUsers[j].FID != 0)
						Count ++;
				}
				uint32	zero = 0;
				uint8	bZero = 0;
				for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
				{
					if (waitinfo.ActivityUsers[j].FID == 0)
						continue;

					FAMILY	*pFamily = GetFamilyData(waitinfo.ActivityUsers[j].FID);
					if (pFamily == NULL)
					{
						sipwarning("GetFamilyData = NULL. FID=%d", waitinfo.ActivityUsers[j].FID);
						continue;
					}

					CMessage	msgOut(M_SC_MULTIACT_RELEASE);
					msgOut.serial(waitinfo.ActivityUsers[j].FID, zero, ActPos, SuccessFlag, Count);
					for (int k = 0; k < MAX_MULTIACT_FAMILY_COUNT; k ++)
					{
						if(waitinfo.ActivityUsers[k].FID != 0)
							msgOut.serial(waitinfo.ActivityUsers[k].FID);
					}
					msgOut.serial(bZero);
					CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
				}
			}
			else
			{
				SendMULTIACT_CANCEL(ActPos, waitinfo.ActivityUsers, FID, bUnconditional);
			}
		}
	}

	// 대기렬 처리
	bool	bSendWaitPacket = false;
	uint32	ErrorCode = 0, RemainCount = 0;
	for(TWaitFIDList::iterator it = rActivity.WaitFIDs.begin(); it != rActivity.WaitFIDs.end(); )
	{
		uint32	nextFID = it->ActivityUsers[0].FID;
		_RoomActivity_Wait curActivity = (*it);
		FAMILY	*pFamily = NULL;

		if(DeskNo != 0xFF)
		{
			pFamily = GetFamilyData(nextFID);
			if(pFamily == NULL) // 만일을 위한 코드이다. - 행사할 사람이 진짜 있는지 검사
			{
				sipwarning("GetFamilyData = NULL. FID=%d", nextFID);

				bSendWaitPacket = true;

				it = rActivity.WaitFIDs.erase(it);

				continue;
			}
			else
			{
				// 행사진행상태로 만든다.
				rActivity.ActivityDesk[DeskNo].init(nextFID, curActivity.ActivityType, NULL);
				for(int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
				{
					rActivity.ActivityDesk[DeskNo].ActivityUsers[i] = curActivity.ActivityUsers[i];
				}

				bSendWaitPacket = true;

				it = rActivity.WaitFIDs.erase(it);
			}
		}
		else if(nextFID == FID)
		{
			bSendWaitPacket = true;
			it = rActivity.WaitFIDs.erase(it);
			continue;
		}
		else
		{
			RemainCount ++;
			it ++;
		}

		if(bSendWaitPacket)
		{
			if(pFamily == NULL)
				pFamily = GetFamilyData(nextFID);
			if(pFamily == NULL)
			{
				sipwarning("GetFamilyData = NULL for send packet: M_SC_ACT_WAIT. FID=%d", nextFID);

				RemainCount --;
				it --;
				it = rActivity.WaitFIDs.erase(it);
			}
			else
			{
				// 행사참가자들에게 M_SC_MULTIACT_WAIT 를 보낸다.
				if (IsMutiActionType(curActivity.ActivityType))
				{
					uint32		MultiErrorCode = 0;
					for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
					{
						if (curActivity.ActivityUsers[j].FID == 0)
							continue;

						FAMILY	*pMultiFamily = GetFamilyData(curActivity.ActivityUsers[j].FID);
						if (pMultiFamily == NULL)
						{
							sipwarning("GetFamilyData = NULL. FID=%d", curActivity.ActivityUsers[j].FID);
							continue;
						}

						CMessage	msgOut(M_SC_MULTIACT_WAIT);
						msgOut.serial(curActivity.ActivityUsers[j].FID, ActPos, MultiErrorCode, RemainCount);
						CUnifiedNetwork::getInstance()->send(pMultiFamily->m_FES, msgOut);
					}
					if (RemainCount == 0) // 시작이면
					{
//						SendMultiActAskToAll(ActPos);
					}
				}
				else
				{
					CMessage	msgOut(M_SC_ACT_WAIT);
					msgOut.serial(nextFID, ActPos, ErrorCode, RemainCount, DeskNo);
					CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
				}

				// 첫 한명만 행사할수 있다.
				sipassert((RemainCount > 0) || (DeskNo != 0xFF));
				DeskNo = 0xFF;
			}
		}
	}
}

void	CSharedActChannelData::CompleteActivity(uint8 ActPos, uint8 DeskNo, uint8 SuccessFlag)
{
	CSharedActivityDesk	&rActivityDesk = m_Activitys[ActPos].ActivityDesk[DeskNo];

	//for(int i = 0; i < MAX_SCOPEMODEL_PER_ACTIVITY; i ++)
	//{
	//	if(rActivityDesk.ScopeModel[i] != "")
	//	{
	//		FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_ACT_DEL_MODEL)
	//			msgOut.serial(ActPos, rActivityDesk.FID, rActivityDesk.ScopeModel[i]);
	//		FOR_END_FOR_FES_ALL()
	//	}
	//}

	if(rActivityDesk.FID > 0)
	{
		if (IsSingleActionType(rActivityDesk.ActivityType))
		{
			FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_ACT_RELEASE)
				msgOut.serial(ActPos, rActivityDesk.FID, rActivityDesk.DeskNo);//, Activity.FName);
			FOR_END_FOR_FES_ALL()
		}
		else
		{
			SendMULTIACT_RELEASE(ActPos, DeskNo, rActivityDesk.ActivityUsers, SuccessFlag);
		}
	}

	rActivityDesk.init(NOFAMILYID, AT_VISIT);

}

void	CSharedActChannelData::SendMULTIACT_CANCEL(uint8 ActPos, _RoomActivity_Family* pActivityUsers, T_FAMILYID FID, bool bUnconditional)
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
	if (!bUnconditional && pActivityUsers[pos].Status == MULTIACTIVITY_FAMILY_STATUS_NONE)
		return;

	pActivityUsers[pos].init();

	ucstring	FName = L"";
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if (pInfo != NULL)
		FName = pInfo->m_Name;

	// 행사참가자들에게 M_SC_MULTIACT_CANCEL를 보낸다.
	uint32	zero = 0;
	for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (pActivityUsers[i].FID == 0)
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
//	SendMultiActAskToAll(ActPos);
}

void	CSharedActChannelData::SendMULTIACT_RELEASE(uint8 ActPos, uint8 DeskNo, _RoomActivity_Family* pActivityUsers, uint8 SuccessFlag)
{
	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_MULTIACT_RELEASE)
		uint8	Count = 0;
		for(int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
		{
			if(pActivityUsers[j].FID != 0)
				Count ++;
		}
		msgOut.serial(ActPos, SuccessFlag, Count);
		for(int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
		{
			if(pActivityUsers[j].FID != 0)
				msgOut.serial(pActivityUsers[j].FID);
		}
		msgOut.serial(DeskNo);
	FOR_END_FOR_FES_ALL()
}


// 2014/08/20:start : 아이템없이 진행하는 행사들(실례로 향드리기)을 갈라보기위한 함수
bool R_CheckActivityGongpinCount(uint8 ActPos, uint8 GongpinCount)
{
	if (GongpinCount == 0) // 향드리기, 아이템개수 0
	{
//		if ( ActPos == ACTPOS_R0_XIANG_1 || ActPos == ACTPOS_R0_XIANG_2 || ActPos == ACTPOS_R0_XIANG_3 )
//			return true;
		if ( ACTPOS_R0_XIANG_INDOOR_1 <= ActPos && ActPos <= ACTPOS_R0_XIANG_OUTDOOR_7 )
			return true;
	}
	if (GongpinCount > 0) // 다른 행사
		return true;
	return false;
}
// end

// 어느 한 사용자에게 현재 방의 활동들을 보내준다.
void CSharedActChannelData::SendActivityList(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	uint32	uZero = 0;
	for(uint8 ActPos = 0; ActPos < m_ActivityCount; ActPos ++)
	{
		CSharedActivity	&rActivity = m_Activitys[ActPos];

		for(int DeskNo = 0; DeskNo < rActivity.ActivityDeskCount; DeskNo ++)
		{
			CSharedActivityDesk	&rActivity_Desk = rActivity.ActivityDesk[DeskNo];

			if(rActivity_Desk.FID == 0)
				continue;

			if(2 == rActivity_Desk.bStartFlag)
			{
				CMessage msg2(M_SC_ACT_START);
				uint8 acttype = rActivity_Desk.ActivityType;
				msg2.serial(FID, uZero, ActPos, acttype, rActivity_Desk.FID, rActivity_Desk.DeskNo);//, Activity.FName);
				CUnifiedNetwork::getInstance()->send(_tsid, msg2);

				// 2014/07/01:start
				_ActivityStepParam* pYisiParam = &rActivity_Desk.ActivityStepParam;
				CMessage msg3(M_SC_ACT_NOW_START);
				msg3.serial(FID, uZero, ActPos, rActivity_Desk.DeskNo, acttype, pYisiParam->IsXDG, rActivity_Desk.FID);
				msg3.serial(pYisiParam->GongpinCount);
				//if (pYisiParam->GongpinCount != 0)
				if ( R_CheckActivityGongpinCount(ActPos, pYisiParam->GongpinCount) )
				{
					uint8	Count = 0;
					for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
					{
						if(rActivity_Desk.ActivityUsers[j].FID != 0)
							Count ++;
					}

					std::list<uint32>::iterator itlist;
					for (itlist = pYisiParam->GongpinsList.begin(); itlist != pYisiParam->GongpinsList.end(); itlist ++)
					{
						uint32 curitem = *itlist;
						msg3.serial(curitem);
					}
					msg3.serial(pYisiParam->BowType, Count);

					for (int j = 1; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
					{
						uint8		PosIndex = j - 1;
						if (rActivity_Desk.ActivityUsers[j].FID != 0)
						{
							msg3.serial(rActivity_Desk.ActivityUsers[j].FID, rActivity_Desk.ActivityUsers[j].ItemID, PosIndex);
						}
					}

					msg3.serial(pYisiParam->StepID);
				}
				else
				{
					sipwarning(L"Gongpin count is invalid! ActPos=%d, Count=%d", ActPos, pYisiParam->GongpinCount);
					continue;
				}
				CUnifiedNetwork::getInstance()->send(_tsid, msg3);
				// end
			}
			else if (ActPos == ACTPOS_PUB_YISHI_1 || ActPos == ACTPOS_PUB_YISHI_2)
			{
				Send_M_SC_MULTIACT2_ITEMS(ActPos, FID, _tsid);
			}
			else if(1 == rActivity_Desk.bStartFlag)
			{
				CMessage msg2(M_SC_ACT_START);
				uint8 acttype = rActivity_Desk.ActivityType;
				msg2.serial(FID, uZero, ActPos, acttype, rActivity_Desk.FID, rActivity_Desk.DeskNo);//, Activity.FName);
				CUnifiedNetwork::getInstance()->send(_tsid, msg2);
			}

			//for(int i = 0; i < MAX_SCOPEMODEL_PER_ACTIVITY; i ++)
			//{
			//	if(rActivity_Desk.ScopeModel[i] != "")
			//	{
			//		// 활동효과모델
			//		CMessage msg3(M_SC_ACT_ADD_MODEL);
			//		msg3.serial(FID, uZero, ActPos, rActivity_Desk.FID, rActivity_Desk.ScopeModel[i]);
			//		CUnifiedNetwork::getInstance()->send(_tsid, msg3);
			//	}
			//}

			// NPC정보
			//for(int npcid = 0; npcid < MAX_NPCCOUNT_PER_ACTIVITY; npcid ++)
			//{
			//	if(rActivity_Desk.NPCs[npcid].Name == "")
			//		continue;

			//	_RoomActivity_NPC	&Activity_NPC = rActivity_Desk.NPCs[npcid];

			//	// NPC보이기
			//	CMessage msg4(M_SC_ACT_NPC_SHOW);
			//	msg4.serial(FID, uZero);
			//	msg4.serial(ActPos, Activity_NPC.Name, Activity_NPC.X, Activity_NPC.Y, Activity_NPC.Z, Activity_NPC.Dir, rActivity_Desk.FID, Activity_NPC.ItemID);
			//	CUnifiedNetwork::getInstance()->send(_tsid, msg4);
			//}

			// 제상에 있는 아이템목록
			//if(rActivity_Desk.HoldItem_Count > 0)
			//{
			//	CMessage msg6(M_SC_ACT_ITEM_PUT);
			//	msg6.serial(FID, uZero, ActPos, rActivity_Desk.FID, rActivity_Desk.HoldItem_Count);
			//	for(uint8 i = 0; i < rActivity_Desk.HoldItem_Count; i ++)
			//	{
			//		msg6.serial(rActivity_Desk.HoldItems[i].X, rActivity_Desk.HoldItems[i].Y, rActivity_Desk.HoldItems[i].Z, rActivity_Desk.HoldItems[i].ItemID, 
			//			rActivity_Desk.HoldItems[i].dirX, rActivity_Desk.HoldItems[i].dirY, rActivity_Desk.HoldItems[i].dirZ);
			//	}
			//	CUnifiedNetwork::getInstance()->send(_tsid, msg6);
			//}
		}
	}
}

void	CSharedActChannelData::AllActCancel(T_FAMILYID FID, uint8 SuccessFlag, bool bUnconditional)
{
	for(uint8 ActPos = 0; ActPos < m_ActivityCount; ActPos ++)
	{
		ActWaitCancel(FID, ActPos, SuccessFlag, bUnconditional);
	}
}

//void	CSharedActChannelData::SendMultiActAskToAll(uint8 ActPos)
//{
//	T_FAMILYID	HostFID = NOFAMILYID;
//	T_M1970		ReqTimeM = 0;
//	uint8		ReqTimeS = 0;
//	uint8		bCount = 0;
//	bool		bStart = false;
//
//	CSharedActivity	&Activity = m_Activitys[ActPos];
//	int emptyDeskPos = Activity.GetEmptyDeskPos();
//	if (emptyDeskPos == -1)
//	{
//		HostFID = Activity.ActivityDesk[0].FID;
//		ReqTimeM = Activity.ActivityDesk[0].ReqTimeM;
//		ReqTimeS = Activity.ActivityDesk[0].ReqTimeS;
//		bCount = Activity.ActivityDesk[0].GetJoinFamilyCount();
//		bStart = Activity.ActivityDesk[0].bStarted;
//	}
//	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_MULTIACT2_ASK)
//		msgOut.serial(ActPos, HostFID, ReqTimeM, ReqTimeS, bCount, bStart);
//	FOR_END_FOR_FES_ALL()
//}

void	CSharedActChannelData::on_M_CS_MULTIACT2_ITEMS(T_FAMILYID FID, uint8 ActPos, _ActivityStepParam* pActParam, SIPNET::TServiceId _tsid)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	CSharedActivity	&Activity = m_Activitys[ActPos];
	CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[0];
	if (ActivityDesk.FID != FID)
	{
		sipwarning("No your turn !!! RoomID=%d, ActPos=%d, Activity.FID=%d, FID=%d", m_RoomID, ActPos, ActivityDesk.FID, FID);
		return;
	}

	ActivityDesk.ActivityStepParam = *pActParam;

	Send_M_SC_MULTIACT2_ITEMS(ActPos, 0, _tsid);
}

void CSharedActChannelData::Send_M_SC_MULTIACT2_ITEMS(uint8 ActPos, T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	CSharedActivity	&Activity = m_Activitys[ActPos];
	CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[0];
	if (ActivityDesk.FID == 0)
	{
		sipwarning("ActivityDesk.FID = 0. ActPos=%d, RequestFID=%d", ActPos, FID);
		return;
	}
	if (ActivityDesk.ActivityStepParam.GongpinCount == 0)
	{
		if (FID == 0)
			sipwarning("ActivityDesk.ActivityStepParam.GongpinCount = 0. ActPos=%d, MasterFID=%d", ActPos, ActivityDesk.FID);
		else
			sipinfo("ActivityDesk.ActivityStepParam.GongpinCount = 0. ActPos=%d, MasterFID=%d, RequestFID=%d (Resaon: User enter between M_SC_ACT_WAIT and M_CS_MULTIACT2_ITEMS)", ActPos, ActivityDesk.FID, FID);
		return;
	}

	std::list<uint32>::iterator itlist;
	uint32 curitem;

	if (FID == 0)
	{
		FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_MULTIACT2_ITEMS)
			msgOut.serial(ActPos, ActivityDesk.FID, ActivityDesk.ActivityStepParam.GongpinCount);
			for (itlist = ActivityDesk.ActivityStepParam.GongpinsList.begin(); itlist != ActivityDesk.ActivityStepParam.GongpinsList.end(); itlist ++)
			{
				curitem = *itlist;
				msgOut.serial(curitem);
			}
		FOR_END_FOR_FES_ALL()
	}
	else
	{
		uint32	zero = 0;
		CMessage	msgOut(M_SC_MULTIACT2_ITEMS);
		msgOut.serial(FID, zero, ActPos, ActivityDesk.FID, ActivityDesk.ActivityStepParam.GongpinCount);
		for (itlist = ActivityDesk.ActivityStepParam.GongpinsList.begin(); itlist != ActivityDesk.ActivityStepParam.GongpinsList.end(); itlist ++)
		{
			curitem = *itlist;
			msgOut.serial(curitem);
		}
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
}

void	CSharedActChannelData::on_M_CS_MULTIACT2_ASK(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	T_FAMILYID	HostFID = NOFAMILYID;
	T_M1970		ReqTimeM = 0;
	uint8		ReqTimeS = 0;
	uint8		bCount = 0;
	bool		bStart = false;

	CSharedActivity	&Activity = m_Activitys[ActPos];
	int emptyDeskPos = Activity.GetEmptyDeskPos();
	if (emptyDeskPos == -1)
	{
		HostFID = Activity.ActivityDesk[0].FID;
		ReqTimeM = Activity.ActivityDesk[0].ReqTimeM;
		ReqTimeS = Activity.ActivityDesk[0].ReqTimeS;
		bCount = Activity.ActivityDesk[0].GetJoinFamilyCount();
		bStart = Activity.ActivityDesk[0].bStartFlag ? 1 : 0;
	}

	uint32 zero = 0;
	CMessage	msgOut(M_SC_MULTIACT2_ASK);
	msgOut.serial(FID, zero, ActPos, HostFID, ReqTimeM, ReqTimeS, bCount, bStart);
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

void	CSharedActChannelData::on_M_CS_MULTIACT2_JOIN(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
		
	CSharedActivity& Activity = m_Activitys[ActPos];
	CSharedActivityDesk& ActivityDesk = Activity.ActivityDesk[0];

	uint32 errCode = ERR_NOERR;
	uint8 joinCount = ActivityDesk.GetJoinFamilyCount();
	if (ActivityDesk.FID == NOFAMILYID)
	{
		errCode = E_MULTIACT_NO;
	}
	else if (ActivityDesk.bStartFlag)
	{
		errCode = E_MULTIACT_STARTED;
	}
	else if (joinCount >= MAX_MULTIACT_FAMILY_COUNT-1)
	{
		errCode = E_MULTIACT_FULL;
	}
	if (errCode != ERR_NOERR)
	{		
		CMessage	msgOut(M_SC_MULTIACT2_JOIN);
		msgOut.serial(FID, ActPos, errCode);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		return;
	}

	joinCount ++;
	CMessage	msgOut(M_SC_MULTIACT2_JOIN);
	msgOut.serial(FID, ActPos, errCode, joinCount);
	
	bool	bJoined = false;
	uint8	JoinPos = 0;
	for (uint8 i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		uint8		PosIndex = i - 1;
		T_FAMILYID	CurFID = ActivityDesk.ActivityUsers[i].FID;
		if (CurFID == NOFAMILYID)
		{
			if (bJoined)
				continue;
			ActivityDesk.ActivityUsers[i].init(FID);
			uint8 JoinStatus = static_cast<uint8>(ActivityDesk.ActivityUsers[i].Status);
			msgOut.serial(PosIndex, FID, JoinStatus);
			bJoined = true;
			JoinPos = PosIndex;
		}
		else
		{
			uint8 JoinStatus = static_cast<uint8>(ActivityDesk.ActivityUsers[i].Status);
			msgOut.serial(PosIndex, CurFID, JoinStatus);
		}
	}
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

	for (uint8 i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		T_FAMILYID	CurFID = ActivityDesk.ActivityUsers[i].FID;
		if (CurFID != NOFAMILYID && CurFID != FID)
		{
			CMessage msg2(M_SC_MULTIACT2_JOINADD);
			msg2.serial(CurFID, ActPos, JoinPos, FID);
			FAMILY*	pHostInfo = GetFamilyData(CurFID);
			if (pHostInfo)
			{
				CUnifiedNetwork::getInstance()->send(pHostInfo->m_FES, msg2);
			}
		}
	}
//	SendMultiActAskToAll(ActPos);
}

void	CSharedActChannelData::on_M_CS_MULTIACT_REQ(T_FAMILYID* ActFIDs, uint8 ActPos, uint8 ActType, uint16 CountDownSec, SIPNET::TServiceId _tsid)
{
	uint32	HostFID = ActFIDs[0];
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", HostFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, HostFID);
		return;
	}
	{
		uint32 ErrorCode = IsPossibleActReq(ActPos);
		if (ErrorCode != ERR_NOERR)
		{
			uint32 RemainCount = 1;
			CMessage	msgOut(M_SC_MULTIACT_WAIT);
			msgOut.serial(HostFID, ActPos, ErrorCode, RemainCount);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
			return;
		}
	}

	int		i;
	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (ActFIDs[i] == 0)
			continue;

		FAMILY*	pFamily = GetFamilyData(ActFIDs[i]);
		if (pFamily)
		{
			// 1명의 User는 하나의 활동만 할수 있다.
			if (GetActivityIndexOfUser(ActFIDs[i]) >= 0)
			{
				sipdebug("GetActivityIndexOfUser(ChannelID, FID) >= 0. FID=%d", ActFIDs[i]);
			}
			else
			{
				continue;
			}
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
	CSharedActivity	&Activity = m_Activitys[ActPos];
	{
		uint32	RemainCount;
		int emptyDeskPos = Activity.GetEmptyDeskPos();
		if (emptyDeskPos >= 0)
		{
			// 현재 진행중이 아니면 행사시작가능
			RemainCount = 0;
			Activity.ActivityDesk[emptyDeskPos].init(HostFID, ActType, ActFIDs);
		}
		else
		{
			// 공황구역은 대기가 없다.
			if (m_RoomID == ROOM_ID_KONGZI || m_RoomID == ROOM_ID_HUANGDI)
			{
				CMessage	msgOut(M_SC_MULTIACT_WAIT);
				uint32	ErrorCode = E_KONGZI_HWANGDI_ACT_WAIT;
				msgOut.serial(HostFID, ActPos, ErrorCode);
				CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
				return;
			}

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

		// 새로운 행사시작일때 갱신통보를 보낸다
//		if (RemainCount == 0)
//			SendMultiActAskToAll(ActPos);
	}

	// 행사에 참가하는 Family들에 M_SC_MULTIACT_REQ 보내기
	{
		FAMILY*	pHostInfo = GetFamilyData(HostFID);
		if (pHostInfo == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", HostFID);
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
			msgOut.serial(HostFID, pHostInfo->m_Name, m_RoomID, m_ChannelWorld->m_RoomName, m_ChannelID, m_ChannelWorld->m_SceneID, Count);
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

void	CSharedActChannelData::on_M_CS_MULTIACT_ANS(T_FAMILYID JoinFID, uint8 ActPos, uint8 Answer)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActAns - Invalid ActPos!: FID=%d, ActPos=%d", JoinFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, JoinFID);
		return;
	}

	_RoomActivity_Family* pActivityUsers = FindJoinFID(ActPos, JoinFID);
	if (pActivityUsers == NULL)
	{
		sipinfo("FindJoinFID = NULL. RoomID=%d, ChannelID=%d, ActPos=%d, JoinFID=%d. (because ANS come after GO)", m_RoomID, m_ChannelID, ActPos, JoinFID);
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

_RoomActivity_Family*	CSharedActChannelData::FindJoinFID(uint8 ActPos, T_FAMILYID JoinFID, uint32* pRemainCount, int* pDeskNo)
{
	CSharedActivity	&Activity = m_Activitys[ActPos];
	{
		for (int DeskNo = 0; DeskNo < Activity.ActivityDeskCount; DeskNo ++)
		{
			CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[DeskNo];
			if(IsMutiActionType(ActivityDesk.ActivityType))
			{
				for(int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
				{
					if(ActivityDesk.ActivityUsers[i].FID == JoinFID)
					{
						if(pRemainCount)
							(*pRemainCount) = 0;
						if (pDeskNo)
							(*pDeskNo) = DeskNo;
						return &ActivityDesk.ActivityUsers[0];
					}
				}
			}
		}
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

void	CSharedActChannelData::on_M_CS_MULTIACT_READY(T_FAMILYID JoinFID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActReady - Invalid ActPos!: FID=%d, ActPos=%d", JoinFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, JoinFID);
		return;
	}

	uint32		RemainCount = 0;
	_RoomActivity_Family* pActivityUsers = FindJoinFID(ActPos, JoinFID, &RemainCount);
	if (pActivityUsers == NULL)
	{
		CMessage	msgOut(M_SC_MULTIACT_WAIT);
		uint32		err = E_MULTIACT_STARTED;
		msgOut.serial(JoinFID, ActPos, err, RemainCount);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

		sipinfo("FindJoinFID = NULL. RoomID=%d, ChannelID=%d, ActPos=%d, JoinFID=%d. (because READY come after GO)", m_RoomID, m_ChannelID, ActPos, JoinFID);
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
		sipwarning("pActivityUsers[pos].Status != MULTIACTIVITY_FAMILY_STATUS_ACCEPT. Status=%d", pActivityUsers[pos].Status);
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

void	CSharedActChannelData::on_M_CS_MULTIACT_GO(T_FAMILYID HostFID, uint8 ActPos, uint32 bXianDaGong, _ActivityStepParam* pActParam)
{
	int		i;
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActGo - Invalid ActPos!: FID=%d, ActPos=%d", HostFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, HostFID);
		return;
	}

	CSharedActivity	&Activity = m_Activitys[ActPos];

	int DeskNo = FindMultiActHostDeskNo(HostFID, ActPos);
	if (DeskNo < 0)
	{
		sipwarning("No your turn !!! RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, HostFID);
		return;
	}

	CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[DeskNo];
//	if (ActivityDesk.FID != HostFID)
//	{
//		sipwarning("No your turn !!! RoomID=%d, ActPos=%d, Activity.FID=%d, FID=%d", m_RoomID, ActPos, ActivityDesk.FID, HostFID);
//		return;
//	}
	if (!IsMutiActionType(ActivityDesk.ActivityType))
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

	ActivityDesk.ActivityStepParam = *pActParam;
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

	if (ActivityDesk.bStartFlag != 0)
	{
		sipwarning("ActivityDesk.bStartFlag != 0. ActivityDesk.bStartFlag=%d", ActivityDesk.bStartFlag);
		return;
	}
	ActivityDesk.bStartFlag = 1;

	uint8	Count = 0;
	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if ((ActivityDesk.ActivityUsers[i].FID != 0) && (ActivityDesk.ActivityUsers[i].Status == MULTIACTIVITY_FAMILY_STATUS_READY))
		{
			Count ++;
		}
	}

	uint8	byDeskNo = (uint8) DeskNo;
	// 행사참가자들에게 M_SC_MULTIACT_GO 를 보낸다.
	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		T_FAMILYID joinfid = ActivityDesk.ActivityUsers[i].FID;
		if(joinfid == 0)
			continue;
		FAMILY	*pFamily = GetFamilyData(joinfid);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. FID=%d", joinfid);
			ActivityDesk.ActivityUsers[i].FID = 0;
			continue;
		}

		CMessage	msgOut(M_SC_MULTIACT_GO);
		msgOut.serial(joinfid, ActPos, Count);
		for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
		{
			if ((ActivityDesk.ActivityUsers[j].FID != 0) && (ActivityDesk.ActivityUsers[j].Status == MULTIACTIVITY_FAMILY_STATUS_READY))
			{
				msgOut.serial(ActivityDesk.ActivityUsers[j].FID);
			}
		}
		msgOut.serial(bXianDaGong, byDeskNo);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
		if (ActivityDesk.ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_READY)
			ActivityDesk.ActivityUsers[i].FID = 0;

	}

	// 주의 사람들에게 행사가 시작됨을 통지한다. M_SC_MULTIACT_STARTED
	{
		FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_MULTIACT_STARTED)
			uint8		ActType = ActivityDesk.ActivityType;
			msgOut.serial(ActPos, ActType, Count);
			for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
			{
				if ((ActivityDesk.ActivityUsers[j].FID != 0) && (ActivityDesk.ActivityUsers[j].Status == MULTIACTIVITY_FAMILY_STATUS_READY))
				{
					msgOut.serial(ActivityDesk.ActivityUsers[j].FID);
				}
			}
			msgOut.serial(byDeskNo);
		FOR_END_FOR_FES_ALL()
	}

	// 한명만 남은 경우에는 개인행사형식으로 바꾼다.
	// 그러나 여러명등불올리기는 개인행사자체가 없으므로 단체행사로 그대로 둔다.
	if ((Count == 1))
	{
		switch (ActivityDesk.ActivityType)
		{
		case AT_PRAYMULTIRITE: 			ActivityDesk.ActivityType = AT_PRAYRITE;	break;
		case AT_PUBLUCK1_MULTIRITE: 	ActivityDesk.ActivityType = AT_PUBLUCK1;	break;
		case AT_PUBLUCK2_MULTIRITE: 	ActivityDesk.ActivityType = AT_PUBLUCK2;	break;
		case AT_PUBLUCK3_MULTIRITE: 	ActivityDesk.ActivityType = AT_PUBLUCK3;	break;
		case AT_PUBLUCK4_MULTIRITE: 	ActivityDesk.ActivityType = AT_PUBLUCK4;	break;
		}
	}
}

void	CSharedActChannelData::SetMultiActStarted(uint8 ActPos, CSharedActivityDesk &ActivityDesk)
{
	ActivityDesk.bStartFlag = 2;

	// 행사참가자들에게 M_SC_MULTIACT_START 를 보낸다.
	for (int i = MAX_MULTIACT_FAMILY_COUNT - 1; i >= 0; i --)
	{
		if (ActivityDesk.ActivityUsers[i].FID == 0)
			continue;

		//FAMILY	*pFamily = GetFamilyData(ActivityDesk.ActivityUsers[i].FID);
		//if (pFamily == NULL)
		//{
		//	sipwarning("GetFamilyData = NULL. FID=%d", ActivityDesk.ActivityUsers[i].FID);
		//	continue;
		//}

		CMessage	msgOut(M_SC_MULTIACT_START);
		for (int j = 0; j < MAX_MULTIACT_FAMILY_COUNT; j ++)
		{
			if(ActivityDesk.ActivityUsers[j].FID != 0)
				msgOut.serial(ActivityDesk.ActivityUsers[j].FID);
		}
		uint32	zero = 0;
//		uint8	ItemExistFlag = 0;
//		if (ActivityDesk.ActivityUsers[i].InvenPos != 0)
//			ItemExistFlag = 1;
		msgOut.serial(zero);
		msgOut.serial(ActPos, ActivityDesk.ActivityUsers[i].FID, ActivityDesk.ActivityUsers[i].Secret, ActivityDesk.ActivityUsers[i].ItemID, ActivityDesk.ActivityUsers[i].Pray);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
	}

	// 2014/07/02:start:행사참가자들을 포함한 방의 모든 사람들에게 M_SC_MULTIACT_NOW_START 를 보낸다
	uint32 HostFID = ActivityDesk.ActivityUsers[0].FID;
	if (HostFID == 0)
	{
		sipwarning(L"HostFID is 0!!");
		return;
	}

	_ActivityStepParam* pActParam = &ActivityDesk.ActivityStepParam;
//	if (pActParam->JoinCount != 0)
//	{
//		sipwarning(L"JoinInfo is not initialized! HostFID=%d", HostFID);
//		pActParam->JoinCount = 0;
//		pActParam->JoinList.clear();
// 		return;
//	}
	// 행사참가자(조인)들의 정보를 설정한다.
//	for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
//	{
//		if(ActivityDesk.ActivityUsers[i].FID != 0)
//		{
//			pActParam->JoinCount ++;
//			_JoinParam join;
//			join.init(ActivityDesk.ActivityUsers[i].FID, ActivityDesk.ActivityUsers[i].ItemID);
//			pActParam->JoinList.push_back(join);
//		}
//	}

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
			if(ActivityDesk.ActivityUsers[j].FID != 0)
				Count ++;
		}

		uint8		ActType = static_cast<uint8>(ActivityDesk.ActivityType);
		FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_ACT_NOW_START)
			msgOut.serial(ActPos, ActivityDesk.DeskNo, ActType, pActParam->IsXDG, HostFID);
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
				if (ActivityDesk.ActivityUsers[j].FID != 0)
				{
					msgOut.serial(ActivityDesk.ActivityUsers[j].FID, ActivityDesk.ActivityUsers[j].ItemID, PosIndex);
				}
			}

			msgOut.serial(pActParam->StepID);
		FOR_END_FOR_FES_ALL()
	}
	// end

//	SendMultiActAskToAll(ActPos);
}

void	CSharedActChannelData::on_M_CS_MULTIACT_START(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint16 InvenPos, uint32 SyncCode, uint8 Secret, ucstring Pray, SIPNET::TServiceId _tsid)
{
	int		i;

	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActStart - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	bool	/*bAllStarted = true,*/ bExistFID = false;
	CSharedActivity	&Activity = m_Activitys[ActPos];

	int DeskNo = FindMultiActDeskNo(FID, ActPos);
	if (DeskNo < 0)
	{
		sipwarning("No your turn !!! RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, FID);
		return;
	}

	CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[DeskNo];

	if (2 == ActivityDesk.bStartFlag)
	{
		sipwarning("MultiAct already is started. FID=%d, Actpos=%d", FID, ActPos);
		return;
	}
	for (i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if (ActivityDesk.ActivityUsers[i].FID == FID)
		{
			if (InvenPos == 0)
				ActivityDesk.ActivityUsers[i].ItemID = 0;
			else
			{
				_InvenItems*	pInven = GetFamilyInven(FID);
				if (pInven == NULL)
				{
					sipwarning("GetFamilyInven NULL. FID=%d", FID);
					return;
				}
				int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
				ActivityDesk.ActivityUsers[i].ItemID = pInven->Items[InvenIndex].ItemID;
			}

			ActivityDesk.ActivityUsers[i].StoreLockID = StoreLockID;
			ActivityDesk.ActivityUsers[i].InvenPos = InvenPos;
			ActivityDesk.ActivityUsers[i].SyncCode = SyncCode;
			ActivityDesk.ActivityUsers[i].Secret = Secret;
			ActivityDesk.ActivityUsers[i].Pray = Pray;
			ActivityDesk.ActivityUsers[i].Status = MULTIACTIVITY_FAMILY_STATUS_START;
			bExistFID = true;
		}
		//else if((ActivityDesk.ActivityUsers[i].FID != 0) && (ActivityDesk.ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_START))
		//	bAllStarted = false;
	}
	if (!bExistFID)
	{
		sipwarning("Unknown Error!!!! (13321) RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	CheckMultiactAllStarted(ActPos, DeskNo);

	//if (bAllStarted)
	//	SetMultiActStarted(ActPos, ActivityDesk);
}

void	CSharedActChannelData::CheckMultiactAllStarted(uint8 ActPos, int DeskNo)
{
	if (!IsValidMultiActPos(ActPos))
		return;

	CSharedActivity	&Activity = m_Activitys[ActPos];
	CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[DeskNo];
	if ((ActivityDesk.FID == 0) || (2 == ActivityDesk.bStartFlag))
		return;

	for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if((ActivityDesk.ActivityUsers[i].FID != 0) && (ActivityDesk.ActivityUsers[i].Status != MULTIACTIVITY_FAMILY_STATUS_START))
			return;
	}

	SetMultiActStarted(ActPos, ActivityDesk);
}

void	CSharedActChannelData::on_M_CS_MULTIACT_CANCEL(T_FAMILYID FID, uint8 ActPos)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActCancel - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	int	DeskNo = -1;
	_RoomActivity_Family* pActivityUsers = FindJoinFID(ActPos, FID, NULL, &DeskNo);
	if (pActivityUsers == NULL)
	{
		sipwarning("FindJoinFID = NULL. RoomID=%d, ChannelID=%d, ActPos=%d, JoinFID=%d", m_RoomID, m_ChannelID, ActPos, FID);
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
		ActWaitCancel(FID, ActPos, false, true);
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
			sipwarning("Unknown Error!!!!!(2007)");
			return;
		}

		pActivityUsers[pos].init();

		if (DeskNo >= 0)
			CheckMultiactAllStarted(ActPos, DeskNo);
	}
}

void	CSharedActChannelData::on_M_CS_MULTIACT_COMMAND(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID JoinFID, uint8 Command, uint32 Param)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActCommand - Invalid ActPos!: FID=%d, ActPos=%d", HostFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, HostFID);
		return;
	}

	CSharedActivity	&Activity = m_Activitys[ActPos];

	int DeskNo = FindMultiActHostDeskNo(HostFID, ActPos);
	if (DeskNo < 0)
	{
		sipwarning("No your turn !!! RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, HostFID);
		return;
	}

	CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[DeskNo];

	if ((!IsMutiActionType(ActivityDesk.ActivityType)) || (ActivityDesk.FID != HostFID))
	{
		sipwarning("Activity Type invalid or You are not current Host. ActivityType=%d, CurrentHostFID=%d, YourFID=%d", ActivityDesk.ActivityType, ActivityDesk.FID, HostFID);
		return;
	}

	for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		uint32	targetFID = ActivityDesk.ActivityUsers[i].FID;
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

int CSharedActChannelData::FindMultiActHostDeskNo(T_FAMILYID HostFID, uint8 ActPos)
{
	CSharedActivity	&Activity = m_Activitys[ActPos];

	for(int DeskNo = 0; DeskNo < Activity.ActivityDeskCount; DeskNo ++)
	{
		CSharedActivityDesk	&rActivity_Desk = Activity.ActivityDesk[DeskNo];

		if ((rActivity_Desk.FID == 0) || !IsMutiActionType(rActivity_Desk.ActivityType))
			continue;

		if (rActivity_Desk.FID == HostFID)
			return DeskNo;
	}

	sipwarning("Can;t find MultiActHostDeskNo. RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, HostFID);

	return -1;
}

int CSharedActChannelData::FindMultiActJoinDeskNo(T_FAMILYID JoinFID, uint8 ActPos)
{
	CSharedActivity	&Activity = m_Activitys[ActPos];

	int DeskNo = 0;
	for(DeskNo = 0; DeskNo < Activity.ActivityDeskCount; DeskNo ++)
	{
		CSharedActivityDesk	&rActivity_Desk = Activity.ActivityDesk[DeskNo];

		if ((rActivity_Desk.FID == 0) || !IsMutiActionType(rActivity_Desk.ActivityType))
			continue;

		for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if (rActivity_Desk.ActivityUsers[i].FID == JoinFID)
				return DeskNo;
		}
	}

	sipwarning("Can;t find MultiActJoinDeskNo. RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, JoinFID);

	return -1;
}

int CSharedActChannelData::FindMultiActDeskNo(T_FAMILYID FID, uint8 ActPos)
{
	CSharedActivity	&Activity = m_Activitys[ActPos];

	int DeskNo = 0;
	for(DeskNo = 0; DeskNo < Activity.ActivityDeskCount; DeskNo ++)
	{
		CSharedActivityDesk	&rActivity_Desk = Activity.ActivityDesk[DeskNo];

		if ((rActivity_Desk.FID == 0) || !IsMutiActionType(rActivity_Desk.ActivityType))
			continue;

		if (rActivity_Desk.FID == FID)
			return DeskNo;

		for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if (rActivity_Desk.ActivityUsers[i].FID == FID)
				return DeskNo;
		}
	}

	sipwarning("Can;t find MultiActDeskNo. RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, FID);

	return -1;
}

void	CSharedActChannelData::on_M_CS_MULTIACT_REPLY(T_FAMILYID JoinFID, uint8 ActPos, uint8 Command, uint32 Param)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActReply - Invalid ActPos!: FID=%d, ActPos=%d", JoinFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, JoinFID);
		return;
	}

	CSharedActivity	&Activity = m_Activitys[ActPos];

	int DeskNo = FindMultiActJoinDeskNo(JoinFID, ActPos);
	if (DeskNo < 0)
	{
		sipwarning("No your turn !!! RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, JoinFID);
		return;
	}

	CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[DeskNo];

	int i;
	for (i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
	{
		if(ActivityDesk.ActivityUsers[i].FID == JoinFID)
			break;
	}
	if (i >= MAX_MULTIACT_FAMILY_COUNT)
	{
		sipwarning("You are not current Join. YourFID=%d", JoinFID);
		return;
	}

	FAMILY*	pFamily = GetFamilyData(ActivityDesk.FID);
	if (pFamily == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", ActivityDesk.FID);
		return;
	}

	CMessage	msgOut(M_SC_MULTIACT_REPLY);
	msgOut.serial(ActivityDesk.FID, ActPos, JoinFID, Command, Param);
	CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
}

void	CSharedActChannelData::on_M_CS_MULTIACT_REQADD(T_FAMILYID HostFID, uint8 ActPos, T_FAMILYID* ActFIDs, uint16 CountDownSec, SIPNET::TServiceId _tsid)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActReply - Invalid ActPos!: FID=%d, ActPos=%d", HostFID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, HostFID);
		return;
	}

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
			if (GetActivityIndexOfUser(ActFIDs[i]) >= 0)
			{
				sipdebug("GetActivityIndexOfUser(ChannelID, FID) >= 0. FID=%d", ActFIDs[i]);
			}
			else
			{
//				if (IsChannelFamily(ActFIDs[i]))
					continue;
			}
		}

		uint8	Answer = 0;
		CMessage	msgOut(M_SC_MULTIACT_ANS);
		msgOut.serial(HostFID, ActPos, ActFIDs[i], Answer);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

		ActFIDs[i] = 0;
	}

	// 추가된 참가자들을 목록에 추가한다
	_RoomActivity_Family* pAllFamily = NULL;
	CSharedActivity	&Activity = m_Activitys[ActPos];
	CSharedActivityDesk *pActivityDesk = &Activity.ActivityDesk[0];

	if (m_RoomID == ROOM_ID_KONGZI || m_RoomID == ROOM_ID_HUANGDI)
	{
		int DeskNo = FindMultiActHostDeskNo(HostFID, ActPos);
		if (DeskNo < 0)
		{
			// 공황구역은 대기렬이 없다.
			sipwarning("No your turn !!! RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, HostFID);
			return;
		}
		pActivityDesk = &Activity.ActivityDesk[DeskNo];
	}

	if (pActivityDesk->FID == HostFID)
	{
		if (!IsMutiActionType(pActivityDesk->ActivityType))
		{
			sipwarning("on_M_CS_MULTIACT_REQADD: Activity.ActivityType=%d", pActivityDesk->ActivityType);
			return;
		}

		for (int i = 0; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
		{
			if (ActFIDs[i] == 0)
				continue;
			bool bResult = pActivityDesk->AddFamily(ActFIDs[i]);
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
				sipdebug("MultiActReqAdd Success: RoomID=%d, HostFID=%d, JoinFID=%d", m_RoomID, HostFID, ActFIDs[i]);
			}
		}
		pAllFamily = pActivityDesk->ActivityUsers;
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
					sipdebug("MultiActReqAddWait Success: RoomID=%d, HostFID=%d, JoinFID=%d", m_RoomID, HostFID, ActFIDs[i]);
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
			uint8		ActType = pActivityDesk->ActivityType;
			msgOut.serial(pAllFamily[i].FID, ActPos, ActType);
			msgOut.serial(HostFID, pHostInfo->m_Name, m_RoomID, m_ChannelWorld->m_RoomName, m_ChannelID, m_ChannelWorld->m_SceneID, Count);
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
//	SendMultiActAskToAll(ActPos);
}

void	CSharedActChannelData::on_M_CS_MULTIACT_RELEASE(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint32 SyncCode, uint8 Number, uint16 *pInvenPos, SIPNET::TServiceId _tsid)
{
	if (!IsValidMultiActPos(ActPos))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"MultiActRequire - Invalid ActPos!: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	CSharedActivity	&Activity = m_Activitys[ActPos];

	int DeskNo = FindMultiActHostDeskNo(FID, ActPos);
	if (DeskNo < 0)
	{
		sipwarning("No your turn !!! RoomID=%d, ActPos=%d, FID=%d", m_RoomID, ActPos, FID);
		return;
	}

	CSharedActivityDesk &ActivityDesk = Activity.ActivityDesk[DeskNo];

	CSharedActWorld *pWorld = dynamic_cast<CSharedActWorld *>(m_ChannelWorld);
	if (pWorld)
	{
		uint8	action_type = ActivityDesk.ActivityType;
		uint8	NpcID = ActPos;
		uint32 ActID = pWorld->SaveYisiAct(FID, _tsid, NpcID, action_type, 
			ActivityDesk.ActivityUsers[0].Secret, ActivityDesk.ActivityUsers[0].Pray, Number, 
			SyncCode, pInvenPos, 0);
		if (ActID > 0)
		{
			TServiceId	tsid_null(0);
			for (int i = 1; i < MAX_MULTIACT_FAMILY_COUNT; i ++)
			{
				uint32	JoinFID = ActivityDesk.ActivityUsers[i].FID;
				if(JoinFID == 0)
					continue;
				FAMILY	*pFamily = GetFamilyData(JoinFID);
				if (pFamily)
				{
					uint16 InvenPos = ActivityDesk.ActivityUsers[i].InvenPos;
					pWorld->SaveYisiAct(JoinFID, pFamily->m_FES, NpcID, action_type, 
						ActivityDesk.ActivityUsers[i].Secret, ActivityDesk.ActivityUsers[i].Pray, 1, 
						ActivityDesk.ActivityUsers[i].SyncCode, &InvenPos, ActID);			
				}
			}
		}
		else
		{
			sipwarning("ActID = 0!!!");
		}
	}
	else
	{
		sipwarning("m_ChannelWorld = NULL. FID", FID);
	}

	ActWaitCancel(FID, ActPos, true, true);
}

void	CSharedActChannelData::on_M_CS_NIANFO_BEGIN(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	uint32	NpcID;	_msgin.serial(NpcID);
	std::map<uint32, CNianFoInfo>::iterator it = m_NianFoInfo.find(NpcID);

	if (it != m_NianFoInfo.end())
	{
		if (it->second.FID != NOFAMILYID)
			return;
	}

	m_NianFoInfo[NpcID] = CNianFoInfo(FID);
	m_NianFoInfo[NpcID].StartTimeS = (uint32)GetDBTimeSecond();

	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_NIANFO_BEGIN)
	{
		uint8 nCount = 1;
		msgOut.serial(nCount, NpcID, FID);
	}
	FOR_END_FOR_FES_ALL()
}

void	CSharedActChannelData::on_M_CS_NIANFO_END(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	ExitNianFo(FID);
}

void	CSharedActChannelData::SendNianFoState(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	uint8 nCount = 0;
	std::map<uint32, CNianFoInfo>::iterator it;
	for (it = m_NianFoInfo.begin(); it != m_NianFoInfo.end(); it++)
	{
		if (it->second.FID != NOFAMILYID)
			nCount ++;
	}
	if (nCount == 0)
		return;

	CMessage	msgOut(M_SC_NIANFO_BEGIN);
	uint32	zero = 0;
	msgOut.serial(FID, zero);
	msgOut.serial(nCount);
	for (it = m_NianFoInfo.begin(); it != m_NianFoInfo.end(); it++)
	{
		if (it->second.FID != NOFAMILYID)
		{
			uint32			npcid = it->first;
			T_FAMILYID	fd = it->second.FID;
			msgOut.serial(npcid, fd);
		}
	}
	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

void	CSharedActChannelData::ExitNianFo(T_FAMILYID FID)
{
	std::map<uint32, CNianFoInfo>::iterator it;
	for (it = m_NianFoInfo.begin(); it != m_NianFoInfo.end(); it++)
	{
		if (it->second.FID == FID)
		{
			uint32 NpcID = it->first;
			uint32 doingTimeS = uint32(GetDBTimeSecond() - it->second.StartTimeS);

			ChangeFamilyExp(Family_Exp_Type_NianFo, FID, doingTimeS);

			it->second = CNianFoInfo(NOFAMILYID);
			FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_NIANFO_END)
			{
				msgOut.serial(NpcID, FID);
			}
			FOR_END_FOR_FES_ALL()
		}
	}
}


void	cb_M_CS_NEINIANFO_BEGIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		CSharedActChannelData	*pChannelData = (CSharedActChannelData*)inManager->GetFamilyChannel(FID);
		if (pChannelData)
		{
			pChannelData->on_M_CS_NEINIANFO_BEGIN(FID, _msgin, _tsid);
		}
		else
		{
			sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
			return;
		}
	}
	else
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
}

void	cb_M_CS_NEINIANFO_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		CSharedActChannelData	*pChannelData = (CSharedActChannelData*)inManager->GetFamilyChannel(FID);
		if (pChannelData)
		{
			pChannelData->on_M_CS_NEINIANFO_END(FID, _msgin, _tsid);
		}
		else
		{
			sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
			return;
		}
	}
	else
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
}

void	CSharedActChannelData::on_M_CS_NEINIANFO_BEGIN(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	uint32	NpcID;	_msgin.serial(NpcID);
	std::map<uint32, CNianFoInfo>::iterator it = m_NeiNianFoInfo.find(NpcID);

	if (it != m_NeiNianFoInfo.end())
	{
		if (it->second.FID != NOFAMILYID)
			return;
	}

	m_NeiNianFoInfo[NpcID] = CNianFoInfo(FID);
	m_NeiNianFoInfo[NpcID].StartTimeS = (uint32)GetDBTimeSecond();

	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_NEINIANFO_BEGIN)
	{
		uint8 nCount = 1;
		msgOut.serial(nCount, NpcID, FID);
	}
	FOR_END_FOR_FES_ALL()
}

void	CSharedActChannelData::on_M_CS_NEINIANFO_END(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	ExitNeiNianFo(FID);
}

void	CSharedActChannelData::SendNeiNianFoState(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	uint8 nCount = 0;
	std::map<uint32, CNianFoInfo>::iterator it;
	for (it = m_NeiNianFoInfo.begin(); it != m_NeiNianFoInfo.end(); it++)
	{
		if (it->second.FID != NOFAMILYID)
			nCount ++;
	}
	if (nCount == 0)
		return;

	CMessage	msgOut(M_SC_NEINIANFO_BEGIN);
	uint32	zero = 0;
	msgOut.serial(FID, zero);
	msgOut.serial(nCount);
	for (it = m_NeiNianFoInfo.begin(); it != m_NeiNianFoInfo.end(); it++)
	{
		if (it->second.FID != NOFAMILYID)
		{
			uint32			npcid = it->first;
			T_FAMILYID	fd = it->second.FID;
			msgOut.serial(npcid, fd);
		}
	}
	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

void	CSharedActChannelData::ExitNeiNianFo(T_FAMILYID FID)
{
	std::map<uint32, CNianFoInfo>::iterator it;
	for (it = m_NeiNianFoInfo.begin(); it != m_NeiNianFoInfo.end(); it++)
	{
		if (it->second.FID == FID)
		{
			uint32 NpcID = it->first;
			uint32 doingTimeS = uint32(GetDBTimeSecond() - it->second.StartTimeS);

			ChangeFamilyExp(Family_Exp_Type_NeiNianFo, FID);

			it->second = CNianFoInfo(NOFAMILYID);
			FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_NEINIANFO_END)
			{
				msgOut.serial(NpcID, FID);
			}
			FOR_END_FOR_FES_ALL()
		}
	}
}


void CSharedActWorld::InitSharedActWorld()
{
	// 벽에 붙이는그림자료를 얻는다. - 삭제?
	{
		MAKEQUERY_GetPublicroomFrameList(strQuery);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetPublicroomFrameList,
			"D",
			CB_,		m_RoomID);
	}

	// FS에 통지한다.
	{
		uint8	flag = 1;	// Room Created
		CMessage	msgOut(M_NT_ROOM);
		msgOut.serial(m_RoomID, flag);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut, false);
	}

	// ManagerService에 통지한다.
	{
		uint32		MasterFID = 0, Result = 0;
		CMessage	msgOut(M_NT_ROOM_CREATED);
		msgOut.serial(m_RoomID, Result, m_SceneID, m_RoomName, MasterFID);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	}

	TTime	curTime = GetDBTimeSecond();
	m_NextReligionCheckEveryWorkTimeSecond = GetUpdateNightTime(curTime);
}

void	CSharedActWorld::on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_ACT_REQ(FID, ActPos, _tsid);
}

void	CSharedActWorld::on_M_CS_ACT_WAIT_CANCEL(T_FAMILYID FID, uint8 ActPos)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_ACT_WAIT_CANCEL(FID, ActPos);
}

void	CSharedActWorld::on_M_CS_ACT_RELEASE(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_ACT_RELEASE(FID, _tsid);
}

void	CSharedActWorld::on_M_CS_ACT_START(T_FAMILYID FID, SIPNET::TServiceId _tsid, _ActivityStepParam* pYisiParam)// 2014/06/11
{
	// 2014/06/11:start
	if (pYisiParam == NULL)
	{
		sipwarning("pYisiParam = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	// end

	ChannelData	*pData = GetFamilyChannel(FID);
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	pChannelData->on_M_CS_ACT_START(FID, _tsid, pYisiParam); // 2014/06/11
}

void	CSharedActWorld::on_M_CS_MULTIACT2_ITEMS(T_FAMILYID FID, uint8 ActPos, _ActivityStepParam* pYisiParam, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT2_ITEMS(FID, ActPos, pYisiParam, _tsid);
}

void	CSharedActWorld::on_M_CS_MULTIACT2_ASK(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT2_ASK(FID, ActPos, _tsid);
}

void	CSharedActWorld::on_M_CS_MULTIACT2_JOIN(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT2_JOIN(FID, ActPos, _tsid);
}

void	CSharedActWorld::on_M_CS_MULTIACT_REQ(T_FAMILYID* ActFIDs, uint8 ActPos, uint8 ActType, uint16 CountDownSec, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(ActFIDs[0]);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, ActFIDs[0]);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_REQ(ActFIDs, ActPos, ActType, CountDownSec, _tsid);
}

void	CSharedActWorld::on_M_CS_MULTIACT_ANS(T_FAMILYID JoinFID, uint32 ChannelID, uint8 ActPos, uint8 Answer)
{
	ChannelData	*pData = GetChannel(ChannelID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, JoinFID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_ANS(JoinFID, ActPos, Answer);
}

void	CSharedActWorld::on_M_CS_MULTIACT_READY(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_READY(FID, ActPos, _tsid);
}

void	CSharedActWorld::on_M_CS_MULTIACT_GO(T_FAMILYID FID, uint8 ActPos, uint32 bXianDaGong, _ActivityStepParam* pActParam)
{
	// 2014/07/02:start
	if (pActParam == NULL)
	{
		sipwarning("pActParam = NULL. RoomID=%d, FID=%d, ActPos=%d, XianDaGong=%d", m_RoomID, FID, ActPos, bXianDaGong);
		return;
	}
	// end

	ChannelData	*pData = GetFamilyChannel(FID);
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	pChannelData->on_M_CS_MULTIACT_GO(FID, ActPos, bXianDaGong, pActParam);
}

void CSharedActWorld::on_M_CS_ACT_STEP(T_FAMILYID FID, uint8 ActPos, uint32 StepID) // 14/07/23
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_ACT_STEP(FID, ActPos, StepID);
}

void	CSharedActWorld::on_M_CS_MULTIACT_START(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint16 InvenPos, uint32 SyncCode, uint8 Secret, ucstring Pray, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_START(FID, ActPos, StoreLockID, InvenPos, SyncCode, Secret, Pray, _tsid);
}

void	CSharedActWorld::on_M_CS_MULTIACT_RELEASE(T_FAMILYID FID, uint8 ActPos, uint32 StoreLockID, uint32 SyncCode, uint8 Number, uint16 *pInvenPos, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_RELEASE(FID, ActPos, StoreLockID, SyncCode, Number, pInvenPos, _tsid);
}

void	CSharedActWorld::on_M_CS_MULTIACT_CANCEL(T_FAMILYID FID, uint32 ChannelID, uint8 ActPos)
{
	ChannelData	*pData = GetChannel(ChannelID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_CANCEL(FID, ActPos);
}

void	CSharedActWorld::on_M_CS_MULTIACT_COMMAND(T_FAMILYID FID, uint8 ActPos, T_FAMILYID JoinFID, uint8 Command, uint32 Param)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_COMMAND(FID, ActPos, JoinFID, Command, Param);
}

void	CSharedActWorld::on_M_CS_MULTIACT_REPLY(T_FAMILYID FID, uint8 ActPos, uint8 Command, uint32 Param)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_REPLY(FID, ActPos, Command, Param);
}

void	CSharedActWorld::on_M_CS_MULTIACT_REQADD(T_FAMILYID FID, uint8 ActPos, T_FAMILYID* ActFIDs, uint16 CountDownSec, SIPNET::TServiceId _tsid)
{
	ChannelData	*pData = GetFamilyChannel(FID);
	if (pData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	CSharedActChannelData* pChannelData = dynamic_cast<CSharedActChannelData*>(pData);
	pChannelData->on_M_CS_MULTIACT_REQADD(FID, ActPos, ActFIDs, CountDownSec, _tsid);
}

static void	DBCB_DBReligion_GetActList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			AllCount	= *(uint32 *)argv[0];
	uint32			ret			= *(uint32 *)argv[1];
	uint32			roomID		= *(uint32*)(argv[2]);
	uint32			FID			= *(uint32 *)argv[3];
	uint8			NpcID		= (uint8)(*(uint32 *)argv[4]);
	uint32			Page		= *(uint32 *)argv[5];
	TServiceId		tsid(*(uint16 *)argv[6]);

	if (!nQueryResult)
		return;
	CSharedActWorld* pWorld		= GetSharedActWorld(roomID);
	if (pWorld == NULL)
	{
		sipwarning("GetSharedActWorld = NULL. RoomID=%d", roomID);
		return;
	}

	CMessage	msgOut(M_SC_R_ACT_LIST);
	msgOut.serial(FID, NpcID, AllCount, Page);

	if(ret != 0)
	{
		if(ret != 1010)
		{
			sipwarning("Invalid return code. ret=%d", ret);
			return;
		}
	}
	else
	{
		SharedActPageInfo newPageInfo;
		newPageInfo.bValid = true;

		uint32	nRows;	resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i ++)
		{
			uint32		ActID, actFID, ModelID;
			ucstring	actFName;
			T_DATETIME	Date;
			uint8		ActionType, bSecret;

			resSet->serial(ActID, actFID, actFName, ActionType, bSecret, Date, ModelID);
			AddFamilyNameData(actFID, actFName);
			uint32	Date_Second = getSecondsSince1970(Date);

			msgOut.serial(ActID, ActionType, actFID, actFName, ModelID, bSecret, Date_Second);

			//// PC와 모바일이 페지크기가 다르므로 이 방식을 쓸수 없다.
			//SharedActInfo newActInfo(ActID, actFID, actFName, ModelID, ActionType, bSecret, Date_Second);
			//newPageInfo.actList.push_back(ActID);

			//pWorld->AddActInfo(newActInfo);
		}

		//// PC와 모바일이 페지크기가 다르므로 이 방식을 쓸수 없다.
		//uint32 mapIndex = Page+1;	// For page is started from 0
		//std::map<uint32, SharedActListHistory>::iterator itHistory = pWorld->m_ReligionActListHistory.find(NpcID);
		//if (itHistory != pWorld->m_ReligionActListHistory.end())
		//{
		//	SharedActListHistory	&dataHistory = itHistory->second;
		//	std::map<uint32, SharedActPageInfo>::iterator itPage = dataHistory.PageInfo.find(mapIndex);
		//	if (itPage != dataHistory.PageInfo.end())
		//	{
		//		dataHistory.AllCount = AllCount;
		//		dataHistory.PageInfo[mapIndex] = newPageInfo;
		//	}
		//	else
		//	{
		//		sipinfo("dataHistory.PageInfo.find(mapIndex) = NULL. FID=%d, NpcID=%d, Page=%d", FID, NpcID, Page);
		//	}
		//}
		//else
		//{
		//	sipwarning("pWorld->m_ReligionActListHistory.find(NpcID) = NULL. FID=%d, NpcID=%d, Page=%d", FID, NpcID, Page);
		//}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);

}

void CSharedActWorld::AddActInfo(SharedActInfo& actInfo)
{
	m_ActInfo[actInfo.ActID] = actInfo;
}

void	CSharedActWorld::on_M_CS_R_ACT_LIST(T_FAMILYID FID, uint8 NpcID, uint32 Page, uint8 PageSize, SIPNET::TServiceId _tsid)
{
	//// PC와 모바일이 페지크기가 다르므로 이 방식을 쓸수 없다.
	//uint32 mapIndex = Page+1;	// For page is started from 0

	//std::map<uint32, SharedActListHistory>::iterator itHistory = m_ReligionActListHistory.find(NpcID);
	//if (itHistory == m_ReligionActListHistory.end())
	//{
	//	SharedActListHistory	dataHistory;
	//	dataHistory.Reset();
	//	m_ReligionActListHistory[NpcID] = dataHistory;
	//	itHistory = m_ReligionActListHistory.find(NpcID);
	//}

	//std::map<uint32, SharedActPageInfo>	&pages = itHistory->second.PageInfo;
	//std::map<uint32, SharedActPageInfo>::iterator itPage = pages.find(mapIndex);
	//if (itPage != pages.end())
	//{
	//	SharedActPageInfo& pageInfo = itPage->second;
	//	if (pageInfo.bValid)
	//	{
	//		CMessage	msgOut(M_SC_R_ACT_LIST);
	//		msgOut.serial(FID, NpcID, itHistory->second.AllCount, Page);
	//		std::list<uint32>::iterator lstit;
	//		for (lstit = pageInfo.actList.begin(); lstit != pageInfo.actList.end(); lstit++)
	//		{
	//			std::map<uint32, SharedActInfo>::iterator itAct = m_ActInfo.find(*lstit);
	//			if (itAct != m_ActInfo.end())
	//			{
	//				SharedActInfo& actInfo = itAct->second;
	//				uint32			ActID = actInfo.ActID;
	//				T_FAMILYID		actFID = actInfo.actFID;
	//				T_FAMILYNAME	actFName = GetFamilyNameFromBuffer(actFID);
	//				uint8			ActionType = actInfo.ActionType;
	//				uint8			bSecret = actInfo.bSecret;
	//				uint32			Date_Second = actInfo.DateSecond;

	//				msgOut.serial(ActID, ActionType, actFID, actFName, actInfo.ModelID, bSecret, Date_Second);
	//			}
	//			else
	//			{
	//				sipwarning("m_ActInfo.find(*lstit) = NULL. FID=%d, NpcID=%d, Page=%d, ActID=%d", FID, NpcID, Page, *lstit);
	//			}
	//		}
	//		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

	//		return;
	//	}
	//}
	//else
	//{
	//	SharedActPageInfo newPageInfo;
	//	pages[mapIndex] = newPageInfo;
	//}

	{
		MAKEQUERY_Religion_GetActList(strQ, m_SharedPlaceType, NpcID, Page, PageSize);	// 0이면 불교구역을 의미한다. // PageSize = 10 
		DBCaller->executeWithParam(strQ, DBCB_DBReligion_GetActList,
			"DDDDDDW",
			OUT_,		NULL,
			OUT_,		NULL,
			CB_,		m_RoomID,
			CB_,		FID,
			CB_,		NpcID,
			CB_,		Page,
			CB_,		_tsid.get());
	}
}

static void	DBCB_DBReligion_GetActList_FID(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			AllCount	= *(uint32 *)argv[0];
	uint32			ret			= *(uint32 *)argv[1];
	uint32			roomID		= *(uint32*)(argv[2]);
	uint32			FID			= *(uint32 *)argv[3];
	uint8			NpcID		= (uint8)(*(uint32 *)argv[4]);
	uint32			Page		= *(uint32 *)argv[5];
	TServiceId		tsid(*(uint16 *)argv[6]);

	if (!nQueryResult)
		return;
	CSharedActWorld* pWorld		= GetSharedActWorld(roomID);
	if (pWorld == NULL)
	{
		sipwarning("GetSharedActWorld = NULL. RoomID=%d", roomID);
		return;
	}

	CMessage	msgOut(M_SC_R_ACT_MY_LIST);
	msgOut.serial(FID, NpcID, AllCount, Page);

	if(ret != 0)
	{
		if(ret != 1010)
		{
			sipwarning("Invalid return code. ret=%d", ret);
			return;
		}
	}
	else
	{
		uint32	nRows;	resSet->serial(nRows);
		for(uint32 i = 0; i < nRows; i ++)
		{
			uint32		ActID;
			T_DATETIME	Date;
			uint8		ActionType, bSecret;

			resSet->serial(ActID, ActionType, bSecret, Date);

			uint32	Date_Second = getSecondsSince1970(Date);

			msgOut.serial(ActID, ActionType, bSecret, Date_Second);
		}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

void	CSharedActWorld::on_M_CS_R_ACT_MY_LIST(T_FAMILYID FID, uint8 NpcID, uint32 Page, uint8 PageSize, SIPNET::TServiceId _tsid)
{
	MAKEQUERY_Religion_GetActList_FID(strQ, m_SharedPlaceType, FID, NpcID, Page, PageSize);	// 0이면 불교구역을 의미한다.
	DBCaller->executeWithParam(strQ, DBCB_DBReligion_GetActList_FID,
			"DDDDDDW",
			OUT_,		NULL,
			OUT_,		NULL,
			CB_,		m_RoomID,
			CB_,		FID,
			CB_,		NpcID,
			CB_,		Page,
			CB_,		_tsid.get());
}

void	CSharedActWorld::on_M_CS_R_ACT_MODIFY(T_FAMILYID FID, uint32 PrayID, uint8 bSecret, ucstring Pray)
{
	std::map<uint32, SharedPrayInfo>::iterator pit = m_PrayInfo.find(PrayID);
	if (pit != m_PrayInfo.end())
	{
		SharedPrayInfo& pInfo = pit->second;
		if (pInfo.actFID == FID)
		{
			//// PC와 모바일이 페지크기가 다르므로 이 방식을 쓸수 없다.
			//if (pInfo.bSecret != bSecret)
			//{
			//	static std::map<uint32, std::list<uint32>>::iterator itPer = m_PraySetPerAct.find(pInfo.ActID);
			//	if (itPer != m_PraySetPerAct.end())
			//	{
			//		if (itPer->second.size() == 1)
			//		{
			//			std::map<uint32, SharedActInfo>::iterator itAct = m_ActInfo.find(pInfo.ActID);
			//			if (itAct != m_ActInfo.end())
			//			{
			//				itAct->second.bSecret = bSecret;
			//			}
			//			else
			//			{
			//				sipwarning("m_ActInfo.find(pInfo.ActID) = NULL. FID=%d, PrayID=%d, ActID=%d", FID, PrayID, pInfo.ActID);
			//			}
			//		}
			//	}
			//	else
			//	{
			//		sipwarning("m_PraySetPerAct.find(pInfo.ActID) = NULL. FID=%d, PrayID=%d, ActID=%d", FID, PrayID, pInfo.ActID);
			//	}
			//}

			pInfo.bSecret = bSecret;
			pInfo.Pray = Pray;
		}
		else
		{
			sipwarning("pInfo.actFID != FID. FID=%d, PrayID=%d, pInfo.actFID=%d", FID, PrayID, pInfo.actFID);
		}
	}
	// DB처리
	{
		MAKEQUERY_Religion_UpdatePray(strQ, PrayID, FID, bSecret, Pray);
		DBCaller->execute(strQ);
	}
}

static void	DBCB_DBReligion_GetActPray(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			roomID	= *(uint32*)(argv[0]);
	uint32			FID	= *(uint32 *)argv[1];
	uint32			ActID	= *(uint32 *)argv[2];
	TServiceId		tsid(*(uint16 *)argv[3]);

	if (!nQueryResult)
		return;
	CSharedActWorld* pWorld		= GetSharedActWorld(roomID);
	if (pWorld == NULL)
	{
		sipwarning("GetSharedActWorld = NULL. RoomID=%d", roomID);
		return;
	}

	uint32	nRows;	resSet->serial(nRows);

	CMessage	msgOut(M_SC_R_ACT_PRAY);
	msgOut.serial(FID, ActID);

	std::list<uint32>	prayset;
	for(uint32 i = 0; i < nRows; i ++)
	{
		uint32		PrayID, actFID;
		ucstring	actFName;
		uint8		bSecret;
		ucstring	Pray;
		resSet->serial(PrayID, actFID, actFName, bSecret, Pray);

		AddFamilyNameData(actFID, actFName);
		msgOut.serial(PrayID, actFID, actFName, bSecret, Pray);

		SharedPrayInfo pi(PrayID, ActID, actFID, actFName, bSecret, Pray);

		uint8	byItemBuf[MAX_ITEM_PER_ACTIVITY * sizeof(uint32) + 1];
		uint32		len=0;			
		resSet->serialBufferWithSize(byItemBuf, len);
		uint8	ItemNum = byItemBuf[0];
		msgOut.serial(ItemNum);
		pi.ItemNum = ItemNum;

		uint32* pItemID = (uint32*)(&byItemBuf[1]);
		for (int i = 0; i < ItemNum; i++)
		{
			uint32 ItemID = *pItemID;
			msgOut.serial(ItemID);
			pi.ItemID[i] = ItemID;

			pItemID++;
		}
		pWorld->m_PrayInfo[PrayID] = pi;
		
		prayset.push_back(PrayID);
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);

	pWorld->m_PraySetPerAct[ActID] = prayset;
}

void	CSharedActWorld::on_M_CS_R_ACT_PRAY(T_FAMILYID FID, uint32 ActID, SIPNET::TServiceId _tsid)
{
	TTime curTime = CTime::getLocalTime();
	std::map<uint32, std::list<uint32>>::iterator it = m_PraySetPerAct.find(ActID);
	if (it != m_PraySetPerAct.end())
	{
		bool bValid = true;
		CMessage	msgOut(M_SC_R_ACT_PRAY);
		msgOut.serial(FID, ActID);
		std::list<uint32>::iterator lstit;
		for (lstit = it->second.begin(); lstit != it->second.end(); lstit++)
		{
			uint32 PrayID = *lstit;
			std::map<uint32, SharedPrayInfo>::iterator pit = m_PrayInfo.find(PrayID);
			if (pit != m_PrayInfo.end())
			{
				SharedPrayInfo& pInfo = pit->second;
				T_FAMILYNAME fname = GetFamilyNameFromBuffer(pInfo.actFID);
				msgOut.serial(PrayID, pInfo.actFID, fname, pInfo.bSecret, pInfo.Pray, pInfo.ItemNum);
				for (uint32 i = 0; i < pInfo.ItemNum; i++)
				{
					msgOut.serial(pInfo.ItemID[i]);
				}
			}
		}
		if (bValid)
		{
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
			return;
		}
	}

	{
		MAKEQUERY_Religion_GetActPray(strQ, ActID);
		DBCaller->executeWithParam(strQ, DBCB_DBReligion_GetActPray,
			"DDDW",
			CB_,		m_RoomID,
			CB_,		FID,
			CB_,		ActID,
			CB_,		_tsid.get());
	}
}

static void	DBCB_DBGetYiSiResultForToday(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ROOMID	roomID = *((T_ROOMID*)argv[0]);

	CSharedActWorld* pWorld		= GetSharedActWorld(roomID);
	if (pWorld == NULL)
	{
		sipwarning("GetSharedActWorld = NULL. RoomID=%d", roomID);
		return;
	}

	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	for(uint32 i = 0; i < nRows; i ++)
	{
		uint32			actid;
		T_FAMILYID		fid;
		T_FAMILYNAME	fname;
		uint8		acttype, npcid;

		resSet->serial(actid, fid, fname, acttype, npcid);
		AddFamilyNameData(fid, fname);
		uint8	byItemBuf[MAX_ITEM_PER_ACTIVITY * sizeof(uint32) + 1];
		memset(byItemBuf, 0, sizeof(byItemBuf));
		uint32		len=0;			
		resSet->serialBufferWithSize(byItemBuf, len);

		if (acttype != AT_PRAYMULTIRITE &&
			acttype != AT_PRAYRITE &&
			acttype != AT_PUBLUCK1 && acttype != AT_PUBLUCK2 && acttype != AT_PUBLUCK3 && acttype != AT_PUBLUCK4 &&
			acttype != AT_PUBLUCK1_MULTIRITE && acttype != AT_PUBLUCK2_MULTIRITE && acttype != AT_PUBLUCK3_MULTIRITE && acttype != AT_PUBLUCK4_MULTIRITE )
			continue;

		uint8	ItemNum = byItemBuf[0];
		if (ItemNum == 0)
			continue;

		bool bTaoCanItem = false;
		uint32* pItemID = (uint32*)(&byItemBuf[1]);
		for (int i = 0; i < ItemNum; i++)
		{
			uint32 ItemID = *pItemID;
			if (GetTaocanItemInfo(ItemID) != NULL)
				bTaoCanItem = true;

			pItemID++;
		}

		_YishiResultType	yisiType = YishiResultType_Large;
		if (acttype == AT_PRAYMULTIRITE || acttype == AT_PUBLUCK1_MULTIRITE || acttype == AT_PUBLUCK2_MULTIRITE || acttype == AT_PUBLUCK3_MULTIRITE || acttype == AT_PUBLUCK4_MULTIRITE)
			yisiType = YishiResultType_Multi;
		else
		{
			if (!bTaoCanItem) // 세트아이템인 경우에는 대형으로 처리한다.
			{
				if (ItemNum <= 3)
					yisiType = YishiResultType_Small;
				else if (ItemNum <= 6)
					yisiType = YishiResultType_Medium;
			}
		}
		pWorld->AddSharedYiSiResult(npcid, yisiType, actid, fid, fname, false);
	}
}

void	CSharedActWorld::ReloadYiSiResultForToday()
{
	ResetSharedYiSiResult();
	MAKEQUERY_Religion_GetAct_OnlyToday(strQuery, m_SharedPlaceType);
	DBCaller->execute(strQuery, DBCB_DBGetYiSiResultForToday,
		"D",
		CB_,		m_RoomID);
}

void	CSharedActWorld::ResetSharedYiSiResult()
{
	TSharedYiSiResultSet::iterator it;
	for (it = m_SharedYiSiResultPerNPC.begin(); it != m_SharedYiSiResultPerNPC.end(); it++)
	{
		CSharedYiSiResultSet& rset = it->second;
		rset.Reset();
	}
}

void	CSharedActWorld::SendYiSiResult(T_FAMILYID FID, TServiceId _tsid)
{
	TSharedYiSiResultSet::iterator it;
	for (it = m_SharedYiSiResultPerNPC.begin(); it != m_SharedYiSiResultPerNPC.end(); it++)
	{
		uint8 npcid = (uint8)it->first;
		CSharedYiSiResultSet& lst = it->second;

		CMessage	msgOut(M_SC_ACT_RESULT_YISHI);
		uint32	zero = 0;
		msgOut.serial(FID, zero, m_RoomID, npcid);
		for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT_RELIGION; i++)
		{
			T_FAMILYNAME fname = GetFamilyNameFromBuffer(lst.m_Result[i].m_FID);
			msgOut.serial(lst.m_Result[i].m_YisiType, lst.m_Result[i].m_ActID, lst.m_Result[i].m_FID, fname);
		}
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
}

void	CSharedActWorld::SendSharedYiSiResultToAll(uint32 NPCID)
{
	if (NPCID != 0)
	{
		TSharedYiSiResultSet::iterator it = m_SharedYiSiResultPerNPC.find(NPCID);
		CSharedYiSiResultSet& lst = it->second;
		uint8 npcid = (uint8)NPCID;
		FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ACT_RESULT_YISHI)
			msgOut.serial(m_RoomID, npcid);
			for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT_RELIGION; i++)
			{
				T_FAMILYNAME fname = GetFamilyNameFromBuffer(lst.m_Result[i].m_FID);
				msgOut.serial(lst.m_Result[i].m_YisiType, lst.m_Result[i].m_ActID, lst.m_Result[i].m_FID, fname);
			}
		FOR_END_FOR_FES_ALL()
	}
	else
	{
		TSharedYiSiResultSet::iterator it;
		for (it = m_SharedYiSiResultPerNPC.begin(); it != m_SharedYiSiResultPerNPC.end(); it++)
		{
			uint8 npcid = (uint8)it->first;
			CSharedYiSiResultSet& lst = it->second;
			FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ACT_RESULT_YISHI)
				msgOut.serial(m_RoomID, npcid);
				for (int i = 0; i < ACTIONRESULT_YISHI_ITEMCOUNT_RELIGION; i++)
				{
					T_FAMILYNAME fname = GetFamilyNameFromBuffer(lst.m_Result[i].m_FID);
					msgOut.serial(lst.m_Result[i].m_YisiType, lst.m_Result[i].m_ActID, lst.m_Result[i].m_FID, fname);
				}
			FOR_END_FOR_FES_ALL()
		}
	}
}

void	CSharedActWorld::AddSharedYiSiResult(uint32 NPCID, uint8 YiSiType, uint32 ActID, T_FAMILYID FID, T_FAMILYNAME FName, bool bNotify)
{
	CSharedYiSiResult newResult(YiSiType, ActID, FID, FName);

	TSharedYiSiResultSet::iterator it = m_SharedYiSiResultPerNPC.find(NPCID);
	if (it != m_SharedYiSiResultPerNPC.end())
	{
		CSharedYiSiResultSet& rset = it->second;
		rset.AddResult(newResult);
	}
	else
	{
		CSharedYiSiResultSet newLst;
		newLst.AddResult(newResult);
		m_SharedYiSiResultPerNPC[NPCID] = newLst;
	}
	if (bNotify)
		SendSharedYiSiResultToAll(NPCID);
}

void cb_M_CS_MULTIACT2_ITEMS(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	uint8	ActPos, GongpinCount;
	uint32	GongpinItemID;
	_ActivityStepParam	paramYisi;

	_msgin.serial(ActPos, GongpinCount);
	paramYisi.GongpinCount = GongpinCount;
	if (GongpinCount != 0)
	{
		for (int i = 0; i < GongpinCount; i++)
		{
			_msgin.serial(GongpinItemID);
			paramYisi.GongpinsList.push_back(GongpinItemID);
		}
	}
	else
	{
		sipwarning(L"gongpin count is zero.");
		return;
	}
	// end

	inManager->on_M_CS_MULTIACT2_ITEMS(FID, ActPos, &paramYisi, _tsid);
}

void cb_M_CS_MULTIACT2_ASK(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	uint8		ActPos;		_msgin.serial(ActPos);
	inManager->on_M_CS_MULTIACT2_ASK(FID, ActPos, _tsid);
}

void cb_M_CS_MULTIACT2_JOIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	uint8		ActPos;		_msgin.serial(ActPos);
	inManager->on_M_CS_MULTIACT2_JOIN(FID, ActPos, _tsid);
}

void	cb_M_CS_NIANFO_BEGIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		CSharedActChannelData	*pChannelData = (CSharedActChannelData*)inManager->GetFamilyChannel(FID);
		if (pChannelData)
		{
			pChannelData->on_M_CS_NIANFO_BEGIN(FID, _msgin, _tsid);
		}
		else
		{
			sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
			return;
		}
	}
	else
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
}

void	cb_M_CS_NIANFO_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		CSharedActChannelData	*pChannelData = (CSharedActChannelData*)inManager->GetFamilyChannel(FID);
		if (pChannelData)
		{
			pChannelData->on_M_CS_NIANFO_END(FID, _msgin, _tsid);
		}
		else
		{
			sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
			return;
		}
	}
	else
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}
}

// Set Publicroom's frame ([b:Index][d:PhotoID][d:ExpRoomID])
void cb_M_GMCS_PUBLICROOM_FRAME_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		RoomID;
	uint8		Index;
	uint32		PhotoID, ExpRoomID;

	_msgin.serial(FID, RoomID, Index, PhotoID, ExpRoomID);

	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_GMCS_PUBLICROOM_FRAME_SET(FID, Index, PhotoID, ExpRoomID);
	}
}

void CSharedActWorld::on_M_GMCS_PUBLICROOM_FRAME_SET(T_FAMILYID FID, uint8 Index, uint32 PhotoID, uint32 ExpRoomID)
{
	uint32	i = 0;
	for (i = 0; i < m_nPublicroomFrameCount; i ++)
	{
		if (Index == m_PublicroomFrameList[i].Index)
		{
			m_PublicroomFrameList[i].PhotoID = PhotoID;
			m_PublicroomFrameList[i].RoomID = ExpRoomID;
			break;
		}
	}
	if (i >= m_nPublicroomFrameCount)
	{
		if (m_nPublicroomFrameCount >= MAX_PUBLICROOM_FRAME_COUNT)
		{
			sipwarning("m_nPublicroomFrameCount >= MAX_PUBLICROOM_FRAME_COUNT. m_nPublicroomFrameCount=%d", m_nPublicroomFrameCount);
			return;
		}
		m_PublicroomFrameList[m_nPublicroomFrameCount].Index = Index;
		m_PublicroomFrameList[m_nPublicroomFrameCount].PhotoID = PhotoID;
		m_PublicroomFrameList[m_nPublicroomFrameCount].RoomID = ExpRoomID;
		m_nPublicroomFrameCount ++;
	}

	{
		MAKEQUERY_SetPublicroomFrame(strQuery, Index, PhotoID, ExpRoomID);
		DBCaller->execute(strQuery);
	}
}

static void	DBCB_DBGetPublicroomFrameList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			RRoomID	= *(uint32 *)argv[0];

	if (!nQueryResult)
		return;

	CSharedActWorld *roomManager = GetSharedActWorld(RRoomID);
	if(roomManager == NULL)
	{
		sipwarning("GetSharedActWorld = NULL. RoomID=%d", RRoomID);
		return;
	}

	resSet->serial(roomManager->m_nPublicroomFrameCount);
	if (roomManager->m_nPublicroomFrameCount > MAX_PUBLICROOM_FRAME_COUNT)
	{
		sipwarning("roomManager->m_nPublicroomFrameCount > MAX_PUBLICROOM_FRAME_COUNT. roomManager->m_nPublicroomFrameCount=%d", roomManager->m_nPublicroomFrameCount);
		roomManager->m_nPublicroomFrameCount = 0;
	}
	else
	{
		for (uint32 i = 0; i < roomManager->m_nPublicroomFrameCount; i ++)
		{
			resSet->serial(roomManager->m_PublicroomFrameList[i].Index);
			resSet->serial(roomManager->m_PublicroomFrameList[i].PhotoID);
			resSet->serial(roomManager->m_PublicroomFrameList[i].RoomID);

			if (roomManager->m_PublicroomFrameList[i].RoomID == 0)
			{
				i --;
				roomManager->m_nPublicroomFrameCount --;
			}
		}
	}
}

// Request Publicroom's frame list
void cb_M_CS_PUBLICROOM_FRAME_LIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	_msgin.serial(FID);

	// 방관리자를 얻는다
	CSharedActWorld *inManager = GetSharedActWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetSharedActWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	CMessage	msgOut(M_SC_PUBLICROOM_FRAME_LIST);
	msgOut.serial(FID);

	for(uint32 i = 0; i < inManager->m_nPublicroomFrameCount; i ++)
	{
		msgOut.serial(inManager->m_PublicroomFrameList[i].Index, inManager->m_PublicroomFrameList[i].PhotoID, inManager->m_PublicroomFrameList[i].RoomID);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

uint8	CSharedActWorld::GetFamilyRoleInRoom(T_FAMILYID _FID) 
{ 
	FAMILY *pF = GetFamilyData(_FID);
	if (pF == NULL)
		return ROOM_NONE;
	if (pF->m_bGM)
		return ROOM_MASTER;
	return ROOM_NONE;
};
