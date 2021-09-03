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

#include "ChannelWorld.h"
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

TChannelWorlds			map_RoomWorlds;
// 방관리자를 얻는다
CChannelWorld* GetChannelWorld(T_ROOMID _rid)
{
	TChannelWorlds::iterator ItResult = map_RoomWorlds.find(_rid);
	if (ItResult == map_RoomWorlds.end())
		return NULL;
	return ItResult->second.getPtr();
}
// 가족이 들어가 있는 방의 관리자를 얻는다.
// 방에 들어가 있지 않으면 NULL을 돌려보낸다.
extern CRoomWorld* GetOpenRoomWorldFromAutoPlayer(T_FAMILYID _fid);
CChannelWorld* GetChannelWorldFromFID(T_FAMILYID _FID)
{
	if ((_FID >= AUTOPLAY_FID_START) && (_FID < AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT))
	{
		return GetOpenRoomWorldFromAutoPlayer(_FID);
	}

	T_ROOMID rid = GetFamilyWorld(_FID);
	if (ISVALIDROOM(rid))
		return GetChannelWorld(rid);
	return NULL;
}

bool IsCommonRoom(T_ROOMID RoomID)
{
	if (RoomID >= MIN_RELIGIONROOMID && RoomID < MAX_RELIGIONROOMID)
		return true;
	return false;
}

void onDisconnectFES(const std::string &_serviceName, TServiceId _tsid, void *_arg)
{
	//sipwarning("Frontend is disconnected, FES=%d", _tsid.get()); byy krs

	MAP_FAMILY::iterator it;
	bool	bIsRemoveFamily = true;
	while (bIsRemoveFamily)
	{
		bIsRemoveFamily = false;
		for(it = map_family.begin(); it != map_family.end(); it++)
		{
			T_FAMILYID	FID = it->first;
			TServiceId	FES = it->second.m_FES;
			if (FES == _tsid)
			{
				RemoveFamily(FID);
				bIsRemoveFamily = true;
				break;
			}
		}
	}
}

void ProcRemoveFamilyForChannelWorld(T_FAMILYID FID)
{
	TChannelWorlds::iterator It;
	for (It = map_RoomWorlds.begin(); It != map_RoomWorlds.end(); It++)
	{
		CChannelWorld*		pChannel = It->second.getPtr();
		pChannel->ProcRemoveFamily(FID);
	}
}

#define DELEGATEPROC_TO_CHANNEL(packetname, _msgin, _tsid)	\
	T_FAMILYID	FID;	\
	_msgin.serial(FID);	\
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);	\
	if (inManager != NULL)	\
	{	\
		ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);	\
		if (pChannelData)	\
		{	\
			pChannelData->on_##packetname(FID, _msgin, _tsid);	\
		}	\
		else	\
		{	\
			sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);	\
			return;	\
		}	\
	}	\
	else	\
	{	\
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);	\
		return;		\
	}	\

void	cb_M_NT_LOGOUT_ChannelWorld(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
//void	cb_M_CS_LOGOUT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid);
TUnifiedCallbackItem ChannelWorldCallbackArray[] =
{
	//{ M_SELFSTATE,				cb_M_SELFSTATE },

	{ M_CS_CANCELREQEROOM,		cb_M_CS_CANCELREQEROOM },
	{ M_CS_OUTROOM,				cb_M_CS_OUTROOM },
	{ M_NT_LOGOUT,				cb_M_NT_LOGOUT_ChannelWorld	},
	//{ M_CS_LOGOUT,				cb_M_CS_LOGOUT },
	{ M_CS_CHANNELLIST,			cb_M_CS_CHANNELLIST },
	{ M_CS_ENTERROOM,			cb_M_CS_ENTERROOM },
	{ M_CS_CHANGECHANNELIN,		cb_M_CS_CHANGECHANNELIN },
	{ M_CS_CHANGECHANNELOUT,	cb_M_CS_CHANGECHANNELOUT },

	{ M_CS_CHAIR_SITDOWN,			cb_M_CS_CHAIR_SITDOWN },
	{ M_CS_CHAIR_STANDUP,			cb_M_CS_CHAIR_STANDUP },

	{ M_CS_MOVECH,				cb_M_CS_MOVECH },
	{ M_CS_STATECH,				cb_M_CS_STATECH },
	{ M_CS_ACTION,				cb_M_CS_ACTION },
	//{ M_CS_CHMETHOD,			cb_M_CS_CHMETHOD },
	//{ M_CS_HOLDITEM,			cb_M_CS_HOLDITEM },

	{ M_CS_EAINITINFO,			cb_M_CS_EAINITINFO },
	{ M_CS_EAMOVE,				cb_M_CS_EAMOVE },
//	{ M_CS_EAANIMATION,			cb_M_CS_EAANIMATION },
	{ M_CS_EABASEINFO,			cb_M_CS_EABASEINFO },
	{ M_CS_EAANISELECT,			cb_M_CS_EAANISELECT },
	
	{ M_CS_INVITE_REQ,			cb_M_CS_INVITE_REQ },
	{ M_CS_INVITE_ANS,			cb_M_CS_INVITE_ANS },

	//{ M_GMCS_RECOMMEND_SET,		cb_M_GMCS_RECOMMEND_SET },
	{ M_GMCS_ROOMEVENT_ADD,		cb_M_GMCS_ROOMEVENT_ADD },

	{ M_CS_LANTERN,				cb_M_CS_LANTERN			},
	{ M_CS_ITEM_MUSICITEM,			cb_M_CS_ITEM_MUSICITEM	},
	//{ M_CS_ITEM_FLUTES,			cb_M_CS_ITEM_MUSICITEM	},
	//{ M_CS_ITEM_VIOLIN,			cb_M_CS_ITEM_MUSICITEM	},
	//{ M_CS_ITEM_GUQIN,			cb_M_CS_ITEM_MUSICITEM },
	//{ M_CS_ITEM_DRUM,			cb_M_CS_ITEM_MUSICITEM },
	//{ M_CS_ITEM_FIRECRACK,		cb_M_CS_ITEM_MUSICITEM },
	{ M_CS_ITEM_RAIN,			cb_M_CS_ITEM_RAIN			},
	{ M_CS_ITEM_POINTCARD,		cb_M_CS_ITEM_POINTCARD			},

	{ M_CS_FISH_FOOD,			cb_M_CS_FISH_FOOD },
	{ M_CS_FLOWER_NEW,			cb_M_CS_FLOWER_NEW },
	{ M_CS_FLOWER_WATER,		cb_M_CS_FLOWER_WATER },
	{ M_CS_FLOWER_END,			cb_M_CS_FLOWER_END },
	{ M_CS_FLOWER_NEW_REQ,		cb_M_CS_FLOWER_NEW_REQ },
	{ M_CS_FLOWER_WATER_REQ,	cb_M_CS_FLOWER_WATER_REQ },
	{ M_CS_FLOWER_WATERTIME_REQ,cb_M_CS_FLOWER_WATERTIME_REQ },
	{ M_CS_MUSIC_START,			cb_M_CS_MUSIC_START },
	{ M_CS_MUSIC_END,			cb_M_CS_MUSIC_END },
	{ M_CS_DRUM_START,			cb_M_CS_DRUM_START },
	{ M_CS_DRUM_END,			cb_M_CS_DRUM_END },
	{ M_CS_BELL_START,			cb_M_CS_BELL_START },
	{ M_CS_BELL_END,			cb_M_CS_BELL_END },
	{ M_CS_SHENSHU_START,		cb_M_CS_SHENSHU_START }, // by krs

	{ M_CS_ANIMAL_APPROACH,		cb_M_CS_ANIMAL_APPROACH },
	{ M_CS_ANIMAL_STOP,			cb_M_CS_ANIMAL_STOP },
	{ M_CS_ANIMAL_SELECT,		cb_M_CS_ANIMAL_SELECT },
	{ M_CS_ANIMAL_SELECT_END,	cb_M_CS_ANIMAL_SELECT_END },

	{ M_CS_MULTILANTERN_REQ,	cb_M_CS_MULTILANTERN_REQ },
	{ M_CS_MULTILANTERN_JOIN,	cb_M_CS_MULTILANTERN_JOIN },
	{ M_CS_MULTILANTERN_START,	cb_M_CS_MULTILANTERN_START },
	{ M_CS_MULTILANTERN_END,	cb_M_CS_MULTILANTERN_END },
	{ M_CS_MULTILANTERN_CANCEL,	cb_M_CS_MULTILANTERN_CANCEL },
	{ M_CS_MULTILANTERN_OUTJOIN,cb_M_CS_MULTILANTERN_OUTJOIN },

	{ M_CS_MULTIACT_REQ,		cb_M_CS_MULTIACT_REQ },
	{ M_CS_MULTIACT_ANS,		cb_M_CS_MULTIACT_ANS },
	{ M_CS_MULTIACT_READY,		cb_M_CS_MULTIACT_READY },
	{ M_CS_MULTIACT_GO,			cb_M_CS_MULTIACT_GO },
	{ M_CS_MULTIACT_START,		cb_M_CS_MULTIACT_START },
	{ M_CS_MULTIACT_RELEASE,	cb_M_CS_MULTIACT_RELEASE },
	{ M_CS_MULTIACT_CANCEL,		cb_M_CS_MULTIACT_CANCEL },
	{ M_CS_MULTIACT_COMMAND,	cb_M_CS_MULTIACT_COMMAND },
	{ M_CS_MULTIACT_REPLY,		cb_M_CS_MULTIACT_REPLY },
	{ M_CS_MULTIACT_REQADD,		cb_M_CS_MULTIACT_REQADD },

	{ M_CS_CRACKER_BOMB,		cb_M_CS_CRACKER_BOMB },

	{ M_CS_ADD_TOUXIANG,				cb_M_CS_ADD_TOUXIANG },

	{ M_CS_ACT_STEP,				cb_M_CS_ACT_STEP },// 14/07/23
};

extern void onWSConnection(const std::string &serviceName, TServiceId tsid, void *arg);
void init_ChannelWorldService()
{
	init_WorldService();

	CUnifiedNetwork::getInstance()->setServiceDownCallback(FRONTEND_SHORT_NAME, onDisconnectFES, 0);
	CUnifiedNetwork::getInstance()->addCallbackArray(ChannelWorldCallbackArray, sizeof(ChannelWorldCallbackArray)/sizeof(ChannelWorldCallbackArray[0]));

	CUnifiedNetwork::getInstance()->setServiceUpCallback(WELCOME_SHORT_NAME, onWSConnection, NULL);

	InitBroker();
}

void update_ChannelWorldService()
{
	UpdateBroker();
	update_WorldService();
}

void release_ChannelWorldService()
{
	ReleaseBroker();
	release_WorldService();
}

void CChannelWorld::InitMasterDataValues()
{
	m_nMaxCharacterOfRoomKind = GetMaxCharacterOfRoomKind(m_SceneID);
}

uint32 CChannelWorld::GetFamilyChannelID(T_FAMILYID FID)
{
	if ((FID >= AUTOPLAY_FID_START) && (FID < AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT))
		return 1; // Auto의 Channel은 1이다.

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if(pFamilyInfo == NULL)
		return 0;
	return pFamilyInfo->m_nChannelID;
}

// 방선로목록을 요청한다.
void CChannelWorld::RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	FAMILY	*pFamily = GetFamilyData(_FID);
	if (pFamily == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", _FID);
		return;
	}
	uint8	bIsMobile = pFamily->m_bIsMobile;

	CMessage	msgOut(M_SC_CHANNELLIST);
	msgOut.serial(_FID, m_RoomID);

	// Channel목록 만들기
	uint32	numLimit = (uint32)m_nMaxCharacterOfRoomKind;
	if (bIsMobile == 1) 
		numLimit /= 10;	// 모바일선로는 PC선로의 최대인원의 10%로 한다.

	uint32	new_channelid = 0;
	uint8	channel_status;
	bool	bSmoothChannelExist = false;
	int		empty_channel_count = 0;

	// 선로중 비지 않은 원활한 선로가 있는가를 검사한다. 있으면 빈 선로를 삭제할수 있다. 1번선로는 삭제하지 않는다.
	TmapDwordToDword	user_count;
	uint32				usercount;
	for(ChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		uint32 channelid = pChannelData->m_ChannelID;
		if ((channelid >= ROOM_SPECIAL_CHANNEL_START)
			|| ((bIsMobile == 1) && (channelid < ROOM_MOBILE_CHANNEL_START))
			|| ((bIsMobile != 1) && (channelid >= ROOM_MOBILE_CHANNEL_START)))
			continue;

		usercount =  pChannelData->EstimateChannelUserCount(numLimit); // pChannelData->GetChannelUserCount();

		user_count[channelid] = usercount;
		if(usercount == 0)
			empty_channel_count ++;
		else if(usercount * 100 < numLimit * g_nRoomSmoothFamPercent)
			bSmoothChannelExist = true;
	}

	// 대기렬에 사람이 없는 경우, 원활한선로가 존재하는 경우에 빈선로를 삭제할수 있다.
	bool	bDeleteAvailable = false;
	if((map_EnterWait.size() <= 1) && (bSmoothChannelExist || (empty_channel_count > 1)))	// (본인은 물론 현재의 대기렬에 있으므로) 대기렬에 2명이상이면 선로삭제를 하지 않는다.
		bDeleteAvailable = true;

	int		cur_empty_channel = 0;
	bool	success_send = false; // by krs
	for(ChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; )
	{
		uint32	channelid = pChannelData->m_ChannelID;
		//sipwarning("@@@@@@@@@@@@@ 1  Channel : RoomID=%d, ChannelID=%d", m_RoomID, channelid);
		if ((bIsMobile == 2 && channelid < ROOM_UNITY_CHANNEL_START) || (bIsMobile == 0 && channelid > ROOM_UNITY_CHANNEL_START)) { // by krs
			if ((m_RoomID >= ROOM_ID_BUDDHISM && m_RoomID <= ROOM_ID_HUANGDI) || m_RoomID < MAX_EXPROOMID){
				pChannelData = GetNextChannel();
				continue;
			}
		}

		if ((channelid >= ROOM_SPECIAL_CHANNEL_START)
			|| ((bIsMobile == 1) && (channelid < ROOM_MOBILE_CHANNEL_START))
			|| ((bIsMobile != 1) && (channelid >= ROOM_MOBILE_CHANNEL_START)))
		{
			pChannelData = GetNextChannel();
			continue;
		}

		uint32	usercount = user_count[channelid];

		if(bDeleteAvailable && (usercount == 0)) // 빈선로는 삭제가능성 검사한다.
		{
			cur_empty_channel ++;
			if((channelid % ROOM_UNITY_CHANNEL_START) > m_nDefaultChannelCount) // default선로는 삭제하지 않는다.
			{
				if(bSmoothChannelExist || (cur_empty_channel > 1))
				{
					// Channel이 삭제됨을 현재 Channel의 사용자들에게 통지한다.
					NoticeChannelChanged(channelid, 0);

					//sipdebug("Channel Deleted. RoomID=%d, ChannelID=%d", m_RoomID, channelid); byy krs

					// 빈 선로를 삭제한다. 1번선로는 삭제하지 않는다.
					pChannelData = RemoveChannel();
					continue;
				}
			}
		}

		if(channelid > new_channelid)
			new_channelid = channelid;

		if(usercount >= numLimit)
			channel_status = ROOMCHANNEL_FULL;
		else if(usercount * 100 >= numLimit * g_nRoomSmoothFamPercent)
			channel_status = ROOMCHANNEL_COMPLEX;
		else
		{
			bSmoothChannelExist = true;
			channel_status = ROOMCHANNEL_SMOOTH;
		}

		//msgOut.serial(channelid, channel_status);

		//pChannelData = GetNextChannel();

		if ((m_RoomID >= ROOM_ID_BUDDHISM && m_RoomID <= ROOM_ID_HUANGDI) || m_RoomID < MAX_EXPROOMID) // by krs
			msgOut.serial(channelid, channel_status);
		else {
			if ((pFamily->m_bIsMobile == 2 && channelid > ROOM_UNITY_CHANNEL_START) || (pFamily->m_bIsMobile == 0 && channelid < ROOM_UNITY_CHANNEL_START)) {
				if (!success_send) {
					msgOut.serial(channelid, channel_status);
					success_send = true;
					//sipwarning("@@@@@@@@@@@@@ 2  Channel : RoomID=%d, ChannelID=%d, userType=%d", m_RoomID, channelid, pFamily->m_bIsMobile);
				}
			}				
			else {
				if (!success_send) {					
					//uint32	channel_id;
					if (pFamily->m_bIsMobile == 0)
						channelid = channelid - ROOM_UNITY_CHANNEL_START;
					if (pFamily->m_bIsMobile == 2)
						channelid = channelid + ROOM_UNITY_CHANNEL_START;
					
					CreateChannel(channelid);
					msgOut.serial(channelid, channel_status);
					success_send = true;
					//sipwarning("@@@@@@@@@@@@@ 3  Channel : RoomID=%d, ChanneldID=%d, userType=%d", m_RoomID, channelid, pFamily->m_bIsMobile);
				}
			}
		}

		pChannelData = GetNextChannel();
	}

	if(!bSmoothChannelExist && (empty_channel_count == 0))
	{
		// 원활한 선로가 없다면 하나 창조한다.
		new_channelid ++;

		int	new_channel_count = 1;
		// 첫 모바일사용자가 들어올 때 모바일전용 채널을 만든다.
		if ((bIsMobile == 1) && new_channelid == 1)
		{
			new_channelid = ROOM_MOBILE_CHANNEL_START + 1;
			//sipwarning("#######  New Channel ID. RoomID=%d, ChannelID=%d", m_RoomID, new_channelid);
			// 체험방은 모바일인 경우에도 default채널을 10개 창조해야 한다.
			if (m_RoomID < MAX_EXPROOMID)
				new_channel_count = EXPROOMROOM_DEFAULT_CHANNEL_COUNT;
		}

		channel_status = ROOMCHANNEL_SMOOTH;

		for (int i = 0; i < new_channel_count; i ++)
		{
			//if (bIsMobile == 0) // by krs
			//	new_channelid = new_channelid + ROOM_C_CHANNEL_START;

			CreateChannel(new_channelid);
			//sipwarning("#######  New Channel ID. RoomID=%d, ChannelID=%d", m_RoomID, new_channelid);
			msgOut.serial(new_channelid, channel_status);

			// Channel이 추가됨을 현재 Channel의 사용자들에게 통지한다.
			NoticeChannelChanged(new_channelid, 1);

			//sipdebug("Channel Created. RoomID=%d, ChannelID=%d", m_RoomID, new_channelid); byy krs

			new_channelid ++;
		}
	}

	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

// 진입대기목록을 정리한다
void	CChannelWorld::RefreshEnterWait()
{
	TTime	curtime = CTime::getLocalTime();
	vector<T_FAMILYID>	temp;
	MAP_ENTERROOMWAIT::iterator it;
	for (it = map_EnterWait.begin(); it != map_EnterWait.end(); it++)
	{
		if ((curtime - it->second.RequestTime) > 300000) // 5분대기
			temp.push_back(it->first);
	}

	vector<T_FAMILYID>::iterator tempit;
	for (tempit = temp.begin(); tempit != temp.end(); tempit++)
	{
		map_EnterWait.erase(*tempit);
	}
}

void	CChannelWorld::CancelRequestEnterRoom(T_FAMILYID _FID)
{
	//sipdebug(L"Cancel entering room : FID=%d, RoomID=%d", _FID, m_RoomID); byy krs
	map_EnterWait.erase(_FID);
}

// 방에서 내보낸다
void	CChannelWorld::OutRoom(T_FAMILYID _FID, BOOL bForce)
{
	TRoomFamilys::iterator it2 = m_mapFamilys.find(_FID);
	if (it2 == m_mapFamilys.end())
		return;

	// World공동처리를 진행한다.
	LeaveFamilyInWorld(_FID);

	// 사용자의 모든 활동자료를 초기화한다.
	ClearAllFamilyActions(_FID);

	// 캐릭터자료목록에서 삭제한다.
	map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator itch;
	for (itch = it2->second.m_mapCH.begin(); itch != it2->second.m_mapCH.end(); itch++)
		DelCharacterData(itch->first);

	// 진입대기목록에서 삭제한다
	// map_EnterWait.erase(_FID);

	// 접속써비스별 가족목록을 갱신한다.
	TServiceId sid = it2->second.m_ServiceID;
	TFesFamilyIds::iterator itFes = m_mapFesFamilys.find(sid);
	if (itFes != m_mapFesFamilys.end())
	{
		itFes->second.erase(_FID);
	}

	T_ID	ChannelID = GetFamilyChannelID(_FID);
	ChannelData*	pChannelData = GetFamilyChannel(_FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel() = NULL. FID=%d", _FID);
		return;
	}

	// 선로에 있는 모든 가족들에게 메쎄지를 보낸다
	if (bForce)
	{
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_FORCEOUTROOM)
			msgOut.serial(m_RoomID, _FID);
		FOR_END_FOR_FES_ALL()
	}
	else
	{
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_OUTROOM)
			msgOut.serial(m_RoomID, _FID);
		FOR_END_FOR_FES_ALL()
	}

	TFesFamilyIds::iterator it3 = pChannelData->m_mapFesChannelFamilys.find(sid);
	if (it3 != pChannelData->m_mapFesChannelFamilys.end())
	{
		it3->second.erase(_FID);
		if (pChannelData->m_MainFamilyID == _FID)
			pChannelData->ResetMainFamily();
	}
	m_mapFamilys.erase(_FID);

	//sipinfo(L"Out room : FID=%d, RoomID=%d", _FID, m_RoomID); byy krs

	SetFamilyWorld(_FID, NOROOMID);

	// 접속써비스들에 통지한다
	CMessage	msgNT(M_NTOUTROOM);
	msgNT.serial(_FID, m_RoomID);
	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgNT);

	// 사적자료완충써비스에 통지한다
	CMessage	msgHis(M_NT_DELFROMHIS);
	msgHis.serial(_FID, m_RoomID);
	CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgHis);

	// ManagerService에 통지
	{
		CMessage	msgOut(M_NT_PLAYER_LEAVEROOM);
		msgOut.serial(_FID, m_RoomID, ChannelID);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	}

	// Log
	DBLOG_OutRoom(_FID, m_RoomID, m_RoomName);

	//sipdebug("m_mapFamilys(Room:%d) Erased. size=%d", m_RoomID, m_mapFamilys.size()); byy krs
}

CChannelWorld::~CChannelWorld()
{
	uint32	zero = 0;
	ucstring	snull = L"";
	for (TRoomFamilys::iterator it = m_mapFamilys.begin(); it != m_mapFamilys.end(); it ++)
	{
		T_FAMILYID	FID = it->first;

		MAP_FAMILY::iterator	it1 = map_family.find(FID);
		if (it1 == map_family.end())
		{
			sipwarning("FamilyID not found : FamilyID=%d", FID);
			m_mapFamilys.erase(it);
			continue;
		}

		T_ROOMID	RoomID = it1->second.m_WorldId;
		T_ID		ChannelID = it->second.m_nChannelID;

		CMessage	msgout(M_SC_DISCONNECT_OTHERLOGIN);
		msgout.serial(FID);
		msgout.serial(FID);
		msgout.serial(it1->second.m_Name);
		CUnifiedNetwork::getInstance()->send(it1->second.m_FES, msgout);

		// 접속써비스들에 통지한다
		CMessage	msgNT(M_NTOUTROOM);
		msgNT.serial(FID, RoomID);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgNT);

		// 사적자료완충써비스에 통지한다
		CMessage	msgHis(M_NT_DELFROMHIS);
		msgHis.serial(FID, RoomID);
		CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgHis);

		// ManagerService에 통지
		{
			CMessage	msgOut(M_NT_PLAYER_LEAVEROOM);
			msgOut.serial(FID, RoomID, ChannelID);
			CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
		}
	}

	// FS에 통지한다.
	{
		uint8	flag = 0;	// Room Deleted
		CMessage	msgOut(M_NT_ROOM);
		msgOut.serial(m_RoomID, flag);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut, false);
	}

	// MS에 통지한다.
	{
		CMessage	msgOut(M_NT_ROOM_DELETED);
		msgOut.serial(m_RoomID);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	}
}

void CChannelWorld::ClearAllFamilyActions(T_FAMILYID _FID)
{
	OutRoom_Spec(_FID);

	ChannelData*	pChannelData = GetFamilyChannel(_FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel() = NULL. FID=%d", _FID);
		return;
	}
	if (pChannelData->m_ChannelID >= ROOM_SPECIAL_CHANNEL_START && pChannelData->GetChannelUserCount() <= 1)
	{
		AddRemoveSpecialChannel(pChannelData->m_ChannelID);
	}

	// 의자에 앉아있던 상태였다면 처리한다.
	{
		for(T_ID ChairID = 1; ChairID <= MAX_CHAIR_COUNT; ChairID ++)
		{
			if(pChannelData->m_ChairFIDs[ChairID] == _FID)
			{
				pChannelData->m_ChairFIDs[ChairID] = 0;
				break;
			}
		}
	}

	// 사용자가 Animal과 교감중이였다면 그것을 중지한다.
	AnimalSelectStop(_FID);

	// 사용자가 꽃을 관리하는 상태였다면 처리한다.
	if(pChannelData->m_FlowerUsingFID == _FID)
		pChannelData->m_FlowerUsingFID = 0;

	// 사용자가 북을 치던 상태였다면 
	for (int i = 0; i < MAX_DRUM_COUNT; i ++)
	{
		if (pChannelData->m_CurDrumData[i].FID == _FID)
		{
			T_ID	DrumID = pChannelData->m_CurDrumData[i].DrumID;

			pChannelData->m_CurDrumData[i].Reset();
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_DRUM_END)
				msgOut.serial(DrumID, _FID);
			FOR_END_FOR_FES_ALL()
		}
	}

	// 사용자가 종을 치던 상태였다면
	if(pChannelData->m_CurBellFID == _FID)
	{
		pChannelData->m_CurBellFID = 0;
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_BELL_END)
			FOR_END_FOR_FES_ALL()
	}

	// 집체공명등처리
	MAP_MULTILANTERNDATA::iterator it;
	for (it = pChannelData->m_MultiLanternList.begin(); it != pChannelData->m_MultiLanternList.end();)
	{
		_MultiLanternData &mdata = it->second;
		if (mdata.MasterID == _FID)
		{
			if (mdata.bStart)
			{
/*
				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTILANTERN_END)
					msgOut.serial(mdata.MultiLanterID);
				FOR_END_FOR_FES_ALL()
*/				
			}
			else
			{
				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTILANTERN_CANCEL)
					msgOut.serial(mdata.MultiLanterID);
				FOR_END_FOR_FES_ALL()
			}
			it = pChannelData->m_MultiLanternList.erase(it);
			continue;
		}
		else
		{
			for (int i = 0; i < MAX_MULTILANTERNJOINUSERS; i++)
			{
				if (mdata.JoinID[i] == 0 ||
					mdata.JoinID[i] != _FID)
					continue;
				mdata.JoinID[i] = 0;
			}
		}
		it++;
	}

	// FreezingPlayer목록에서 삭제한다.
	pChannelData->UnregisterActionPlayer(_FID);
}

void	cb_M_GMCS_OUTFAMILY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	sipwarning("Not supported.");	// 써버에서는 탈퇴처리를 진행하지만 클라이언트에서는 처리를 하지 않으므로 오유가 날수 있다!!!

	//T_FAMILYID			GMID;				_msgin.serial(GMID);			
	//T_FAMILYID			FID;				_msgin.serial(FID);				// 대상id

	//CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	//if (inManager != NULL)
	//{
	//	inManager->ForceOutRoom(GMID, FID, _tsid);

	//	// GMLog
	//	GMLOG_OutFamily(GMID, FID, inManager->m_RoomID, inManager->m_RoomName);
	//}
}

// 방에서 강제 내보낸다
//void	CChannelWorld::ForceOutRoom(T_FAMILYID _FID, T_FAMILYID _ObjectID, SIPNET::TServiceId _sid )
//{
//	OutRoom(_ObjectID, true);
//
//	uint32	uResult = ERR_NOERR;
//	CMessage	msg(M_SC_FORCEOUT);
//	msg.serial(_FID, uResult, m_RoomID, _ObjectID);
//	CUnifiedNetwork::getInstance ()->send(_sid, msg);
//}

void	cb_M_GMCS_ACTIVE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	uint8			bOn, utype;				_msgin.serial(bOn, utype);

	FAMILY		*pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipwarning("pFamily = NULL. FID=%d", FID);// 2014/04/08 
		return;
	}
	uint8	uShowType = CHSHOW_GENERAL;
	if (bOn)
		uShowType = CHSHOW_GM;
	else
		uShowType = CHSHOW_GENERAL;

	pFamily->m_uShowType = uShowType;

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->SendGMActive(FID, bOn, utype);
	}
// 	else // 2014/05/15:lobby에 있는 사용자에게는 보낼 필요 없다.
// 	{
// 	}
}

void	CChannelWorld::SendGMActive(T_FAMILYID _FID, uint8 _bOn, uint8 _utype)
{
	ChannelData	*pChannelData = GetFamilyChannel(_FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, _FID);
		return;
	}

	TFesFamilyIds::iterator	itFes;
	for(itFes = pChannelData->m_mapFesChannelFamilys.begin(); itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
	{
		if(itFes->second.size() == 0)
			continue;
		bool bIsFamily = false;
		CMessage	msgOut0(M_GMSC_ACTIVE);
		for(MAP_FAMILYID::iterator itF = itFes->second.begin(); itF != itFes->second.end(); itF ++)
		{
			T_FAMILYID fid = itF->first;
			if (fid == _FID)
				continue;
			msgOut0.serial(fid);
			bIsFamily = true;
		}
		if (!bIsFamily)
			continue;
		uint32 uZero = 0;
		msgOut0.serial(uZero);

		msgOut0.serial(_FID, _bOn, _utype);
		CUnifiedNetwork::getInstance()->send(itFes->first, msgOut0);
	}

}

void	cb_M_GMCS_SHOW(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	bool				bShow;				_msgin.serial(bShow);

	FAMILY		*pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
		return;
	uint8	uShowType = CHSHOW_GENERAL;
	if (bShow)
		uShowType = CHSHOW_GM;
	else
		uShowType = CHSHOW_HIDE;

	pFamily->m_uShowType = uShowType;

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->SendGMShow(FID, bShow);
	}

	// GM Log
	GMLOG_Show(FID, bShow);
}

void	CChannelWorld::SendGMShow(T_FAMILYID _FID, bool _bShow)
{
	ChannelData	*pChannelData = GetFamilyChannel(_FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, _FID);
		return;
	}
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_GMSC_SHOW)
		msgOut.serial(_FID, _bShow);
	FOR_END_FOR_FES_ALL();
	//sipinfo(L"Set GM show: FID=%d, Show=%d", _FID, _bShow); byy krs
}

void	cb_M_CS_CANCELREQEROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	_msgin.serial(FID, RoomID);

	CChannelWorld *roomManager = GetChannelWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetChannelWorld = NULL. RoomID=%d", RoomID);
		return;
	}
	roomManager->CancelRequestEnterRoom(FID);
}

void	cb_M_CS_OUTROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);			// 가족id

	// 들어가 있는 방관리자 얻기
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if(inManager != NULL)
	{
		inManager->OutRoom(FID);
	}
	else
	{
		// 비법발견
		// ucstring ucUnlawContent;
		// ucUnlawContent = ucstring("Invalid out room : ") + ucstring("FamilyID = ") + toStringW(FID);
		// RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
	}
}

void	cb_M_NT_LOGOUT_ChannelWorld(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID FID;	_msgin.serial(FID);

	RemoveFamily(FID);
}

//void	cb_M_CS_LOGOUT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID FID;	_msgin.serial(FID);
//
//	Family_M_CS_LOGOUT(FID, _tsid);
//	//Contact_M_CS_LOGOUT(FID);
//}

// 방진입을 요청한다
void	CChannelWorld::RequestEnterRoom(T_FAMILYID _FID, std::string password, SIPNET::TServiceId _sid, uint32 ChannelID, uint8 ContactStatus)
{
	RefreshEnterWait();

	// 방진입코드얻기
	uint32	code  = GetEnterRoomCode(_FID, password, ChannelID, ContactStatus);

	// 메쎄지생성
	CMessage	msgOut(M_SC_REQEROOM);
	msgOut.serial(_FID, code, m_RoomID, m_SceneID);
	//sipinfo(L"Entering room result : FID=%d, RoomID=%d, Code=%d", _FID, m_RoomID, code); byy krs

	if (code != ERR_NOERR)
	{
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
		return;
	}
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);

	// 진입대기목록갱신
	MAP_ENTERROOMWAIT::iterator it = map_EnterWait.find(_FID);
	if (it != map_EnterWait.end())
	{
		it->second.RequestTime = CTime::getLocalTime();
	}
	else
	{
		ENTERROOMWAIT	newenter;
		newenter.FID = _FID;		newenter.RequestTime = CTime::getLocalTime();
		map_EnterWait.insert( make_pair(_FID, newenter) );
	}
}

void	cb_M_CS_CHANNELLIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	T_ROOMID		RoomID;			_msgin.serial(RoomID);
	bool			bSpecial;		_msgin.serial(bSpecial);
	CChannelWorld *roomManager = GetChannelWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("GetChannelWorld = NULL. RoomID=%d", RoomID);
		return;
	}
	if (bSpecial)
	{
		roomManager->RequestSpecialChannelList(FID, _tsid);
	}
	else
	{
		roomManager->RequestChannelList(FID, _tsid);
	}
}

// Channel이 변했음을 현재 Channel의 사용자들에게 통지한다.
void CChannelWorld::NoticeChannelChanged(uint32 ChannelID, uint8 bAdd)
{
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_CHANNELCHANGED)
		msgOut.serial(ChannelID, bAdd);
	FOR_END_FOR_FES_ALL()
}

int CChannelWorld::CheckEnterWaitFamily(T_FAMILYID _FID, uint32 ChannelID)
{
	// 진입대기렬에 있는가 검사하고 있으면 삭제한다
	MAP_ENTERROOMWAIT::iterator it = map_EnterWait.find(_FID);
	if (it == map_EnterWait.end())
	{
		return 1;	// 대기렬에 없음.
	}
//	map_EnterWait.erase(_FID);

	// Channel이 진입가능한지 검사한다.
	ChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("No Channel ID. RoomID=%d, FID=%d, RegChannelID=%d", m_RoomID, _FID, ChannelID);
		return 2;	// ChannelID가 존재하지 않음.
	}

	uint32		user_count = pChannelData->GetChannelUserCount();
	if (user_count >= m_nMaxCharacterOfRoomKind)
		return 3;	// Channel이 Full

	return CheckEnterWaitFamily_Spec();
}

void	cb_M_CS_ENTERROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);			// 가족id
	T_ROOMID		RoomID;			_msgin.serial(RoomID);		// 방id
	T_ID			ChannelID;		_msgin.serial(ChannelID);
	T_CHARACTERID	CHID;			_msgin.serial(CHID);		// 캐릭터id
	T_CH_DIRECTION	CHDirection;	_msgin.serial(CHDirection);
	T_CH_X			CHX;			_msgin.serial(CHX);
	T_CH_Y			CHY;			_msgin.serial(CHY);
	T_CH_Z			CHZ;			_msgin.serial(CHZ);

	if (!IsThisShardRoom(RoomID))
	{
		sipwarning("IsThisShardRoom = false. RoomID=%d, FID=%d", RoomID, FID);
		return;
	}

	FAMILY	*pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d, RoomID=%d, ChannelID=%d", FID, RoomID, ChannelID);
		return;
	}
	if (pFamily->m_bIsMobile == 1)
	{
		if (ChannelID < ROOM_MOBILE_CHANNEL_START)
		{
			sipwarning("Mobile User request to enter PC channel. FID=%d, RoomID=%d, ChannelID=%d", FID, RoomID, ChannelID);
			return;
		}
	}
	else
	{
		if (ChannelID >= ROOM_MOBILE_CHANNEL_START && ChannelID < ROOM_SPECIAL_CHANNEL_START)
		{
			sipwarning("PC User request to enter Mobile channel. FID=%d, RoomID=%d, ChannelID=%d", FID, RoomID, ChannelID);
			return;
		}
		if (pFamily->m_bIsMobile == 2 && (RoomID >= ROOM_ID_BUDDHISM && RoomID <= ROOM_ID_HUANGDI) && ChannelID < ROOM_SPECIAL_CHANNEL_START) //by krs
		{
			if(ChannelID < ROOM_UNITY_CHANNEL_START)
				ChannelID = ChannelID + ROOM_UNITY_CHANNEL_START;
		}
	}

	CChannelWorld *roomManager = GetChannelWorld(RoomID);
	if (roomManager == NULL)
	{
		sipwarning("There is no enter room manager(RoomID=%d)", RoomID);
		return;
	}

	int	checkresult = roomManager->CheckEnterWaitFamily(FID, ChannelID);
	//sipwarning("++++++  fid = %d, channeldId = %d, checkresult = %d, maxCharector = %d  +++++", FID, ChannelID, checkresult, roomManager->m_nMaxCharacterOfRoomKind);

	if (checkresult == 2) {		// 이 경우는 새 천원을 창조할때에 생길것이라고 본다. 새 천원창조시 M_CS_REQ_CHANNELLIST를 호출하지 않는다.  by krs			
		uint8 bAdd = 1;
		if (pFamily->m_bIsMobile == 2)
			ChannelID = ChannelID + ROOM_UNITY_CHANNEL_START;
		roomManager->CreateChannel(ChannelID);
		//sipwarning("#######  New Channel ID. RoomID=%d, ChannelID=%d", RoomID, ChannelID);
		CMessage	msgOut(M_SC_CHANNELCHANGED);
		msgOut.serial(FID, ChannelID, bAdd);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		checkresult = 0;
			//sipwarning("No Channel ID. RoomID=%d, FID=%d, RegChannelID=%d", m_RoomID, _FID, ChannelID);
			//return 2;	// ChannelID가 존재하지 않음.
		
	}		
	//else byy krs
	//	sipdebug("FID=%d, Enter RoomID=%d, ChannelID=%d, checkresult=%d", FID, RoomID, ChannelID, checkresult);

	switch(checkresult)
	{
	case 0:
		// 방에 들여보낸다.
		roomManager->EnterFamily(ChannelID, FID, CHID, CHDirection, CHX, CHY, CHZ, _tsid);
		break;
	case 1:	// 대기렬에 없음 - 부정한 사용자
		break;
	case 2:	// ChannelID가 존재하지 않음.
	case 3:	// Channel이 Full
		{
			uint32	ChID = 0, err = E_ENTER_NO_FULL;
			CMessage	msgOut(M_SC_ENTERROOM);
			msgOut.serial(FID, RoomID, ChannelID, ChID, err);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}
		break;
	case 4:	// 양도중인 방에는 들어갈수 없다.
		{
			uint32	ChID = 0, err = E_ENTER_CHANGE_MASTER;
			CMessage	msgOut(M_SC_ENTERROOM);
			msgOut.serial(FID, RoomID, ChannelID, ChID, err);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}
		break;
	}
}

static uint32 g_serial_key = 0;
struct EnterFamilyData
{
	uint32	serial_key;
	T_ID	channel_id;
	T_CHARACTERID ch_id;
	T_CH_DIRECTION ch_direction;
	T_CH_X ch_x;
	T_CH_Y ch_y;
	T_CH_Z ch_z;

	EnterFamilyData() {};
	EnterFamilyData(T_ID ChannelID, T_CHARACTERID _CHID, T_CH_DIRECTION _CHDirection, T_CH_X _CHX, T_CH_Y _CHY, T_CH_Z _CHZ)
		: channel_id(ChannelID), ch_id(_CHID), ch_direction(_CHDirection), ch_x(_CHX), ch_y(_CHY), ch_z(_CHZ)
	{
		serial_key = ++g_serial_key;
	}
};
std::map<uint32, EnterFamilyData> g_EnterFamilyDatas;

void EnterFamilyInChannel_Callback(T_FAMILYID FID, CWorld* pWorld, uint32 serial_key)
{
	CChannelWorld	*pChannelWorld = dynamic_cast<CChannelWorld*>(pWorld);
	if (pChannelWorld == NULL)
	{
		sipwarning("dynamic_cast<CChannelWorld*>(pWorld) = NULL");
		return;
	}
	pChannelWorld->EnterFamilyInChannel_After(FID, serial_key);
}

void CChannelWorld::EnterFamilyInChannel_After(T_FAMILYID _FID, uint32 serial_key)
{
	T_ID ChannelID;
	T_CHARACTERID _CHID;
	T_CH_DIRECTION _CHDirection;
	T_CH_X _CHX;
	T_CH_Y _CHY;
	T_CH_Z _CHZ;
	{
		std::map<uint32, EnterFamilyData>::iterator it = g_EnterFamilyDatas.find(serial_key);
		if (it == g_EnterFamilyDatas.end())
		{
			sipwarning("it == g_EnterFamilyDatas.end(). FID=%d, RoomID=%d", _FID, m_RoomID);
			return;
		}

		ChannelID = it->second.channel_id;
		_CHID = it->second.ch_id;
		_CHDirection = it->second.ch_direction;
		_CHX = it->second.ch_x;
		_CHY = it->second.ch_y;
		_CHZ = it->second.ch_z;

		g_EnterFamilyDatas.erase(it);
	}

	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(_FID);
	if (pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", _FID);
		return;
	}
	SIPNET::TServiceId _sid = pInfo->m_ServiceID;

	// 가족에 속한 캐릭터가 맞는가를 검사한다
	if (!IsInFamily(_FID, _CHID))
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = ucstring("Invalid enter room : ") + ucstring("FamilyID = ") + toStringW(_FID) + ucstring(" CHID = ") + toStringW(_CHID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
		return;
	}

	FAMILY		*family		= GetFamilyData(_FID);
	CHARACTER	*character	= GetCharacterData(_CHID);
	if (family == NULL || character == NULL)
	{
		sipwarning("(family == NULL || character == NULL). FID=%d", _FID);
		return;
	}

	uint8	uRoomRole = GetFamilyRoleInRoom(_FID);
	pInfo->m_RoomRole = uRoomRole;
	pInfo->m_nChannelID = ChannelID;

	SetFamilyWorld(_FID, m_RoomID, WORLD_ROOM);

	// 캐릭터의 상태를 설정한다.
	ucstring	ucAniName = L"";
	T_CH_STATE	aniState = 0;
	T_FITEMID	holdItem = 0;
	SetCharacterState(_FID, _CHID, _CHDirection, _CHX, _CHY, _CHZ, ucAniName, aniState, holdItem);

	// 방에 들어오게 될 가족에게 응답을 보낸다
	CMessage	msgOut0(M_SC_ENTERROOM);
	uint32		code = ERR_NOERR;
	msgOut0.serial(_FID, m_RoomID, ChannelID, _CHID, code);

	CUnifiedNetwork::getInstance()->send(_sid, msgOut0);
	//sipinfo(L"Enter room : FID=%d, RoomID=%d", _FID, m_RoomID); byy krs

	ChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		return;
	}

	// 새가족의 입방을 통보한다
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_NEWENTER)
		msgOut.serial(m_RoomID, _FID, family->m_Name, pInfo->m_Level, pInfo->m_Exp, pInfo->m_RoomCount, family->m_bIsMobile);
		msgOut.serial(character->m_ID, character->m_Name, character->m_ModelId, character->m_DefaultDressID, character->m_FaceID, character->m_HairID, character->m_DressID);
		msgOut.serial(_CHX, _CHY, _CHZ, _CHDirection, ucAniName, aniState, holdItem);
	FOR_END_FOR_FES_ALL()

	// 접속써비스별 가족목록을 갱신한다.
	map_EnterWait.erase(_FID);
	TFesFamilyIds::iterator it3 = pChannelData->m_mapFesChannelFamilys.find(_sid);
	if(it3 == pChannelData->m_mapFesChannelFamilys.end())
	{
		MAP_FAMILYID	newMap;
		newMap.insert( make_pair(_FID, _FID) );
		pChannelData->m_mapFesChannelFamilys.insert(make_pair(_sid, newMap));
	}
	else
	{
		it3->second.insert( make_pair(_FID, _FID) );
	}

	// 접속써비스들에 통지한다
	CMessage	msgNT(M_NTENTERROOM);
	msgNT.serial(_FID, m_RoomID, m_RoomName, ChannelID);
	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgNT);

	// 방안의 사용자목록을 내려보낸다.
	SendAllFIDInRoomToOne(_FID, _sid);

	// 움직임 동물목록을 내려보낸다
	SendAnimal(_FID, _sid);

	if (pChannelData->m_MainFamilyID == NOFAMILYID)
		pChannelData->ResetMainFamily();

	CMessage	msgHis(M_NT_ADDTOHIS);
	uint16	fesid = _sid.get();
	msgHis.serial(_FID, m_RoomID, uRoomRole, fesid, family->m_bIsMobile); //by krs
	CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgHis);

	EnterFamily_Spec(_FID, _sid);

	// 타고 있는 상서동물정보
	SendMountAnimal(_FID, _sid);

	// 꽃, 물고기정보를 내려보낸다.
	SendFishFlowerInfo(_FID, _sid);

	// 방안의 집체공명등자료를 내려보낸다
	SendMultiLanterList(_FID, _sid);

	// 방안의 등불정보를 내려보낸다
	SendLanternList(_FID, _sid);

	// 북치는 상태를 내려보낸다.
	for (int i = 0; i < MAX_DRUM_COUNT; i ++)
	{
		if (pChannelData->m_CurDrumData[i].FID != 0)
		{
			CMessage	msgOut(M_SC_DRUM_START);
			uint32	zero = 0;
			T_COMMON_NAME	empty = L"";
			// 드람 치던 도중에 들어가는 사용자에게는 FID와 NPCName이 필요없으므로 빈값으로 보낸다.
			msgOut.serial(_FID, zero, pChannelData->m_CurDrumData[i].DrumID, zero, empty);
			CUnifiedNetwork::getInstance()->send(_sid, msgOut);
		}
	}

	// 종치는 상태를 내려보낸다.
	if (pChannelData->m_CurBellFID != 0)
	{
		CMessage	msgOut(M_SC_BELL_START);
		uint32	zero = 0;
		msgOut.serial(_FID, zero, pChannelData->m_CurBellFID);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
	}

	// 방안의 폭죽정보를 내려보낸다
	SendCrackerList(_FID, _sid);

	// PlayerManager에 통지
	{
		CMessage	msgOut(M_NT_PLAYER_ENTERROOM);
		msgOut.serial(_FID, m_RoomID, ChannelID);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	}

	// touxiang info
	pChannelData->SendTouXiangState(_FID, _sid);

	// Log
	DBLOG_EnterRoom(_FID, m_RoomID, m_RoomName);

	//sipdebug("m_mapFamilys(Room:%d) Added. size=%d", m_RoomID, m_mapFamilys.size()); byy krs
}

void CChannelWorld::EnterFamily(uint32 ChannelID, T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_DIRECTION _CHDirection, T_CH_X _CHX, T_CH_Y _CHY, T_CH_Z _CHZ, SIPNET::TServiceId _sid)
{
	EnterFamilyData data(ChannelID, _CHID, _CHDirection, _CHX, _CHY, _CHZ);
	uint32 param = data.serial_key;
	g_EnterFamilyDatas[param] = data;

	EnterFamilyInWorld(_FID, _sid, EnterFamilyInChannel_Callback, param);
}

// 방에 들여보낸다
//void	CChannelWorld::EnterFamily(uint32 ChannelID, T_FAMILYID _FID, T_CHARACTERID _CHID, T_CH_STATE _CHState, T_CH_DIRECTION _CHDirection, T_CH_X _CHX, T_CH_Y _CHY, T_CH_Z _CHZ, SIPNET::TServiceId _sid)
//{
//	// 캐릭터정보를 얻는다
//	LoadCharacterData(_CHID);
//
//	// 가족에 속한 캐릭터가 맞는가를 검사한다
//	if (!IsInFamily(_FID, _CHID))
//	{
//		// 비법발견
//		ucstring ucUnlawContent;
//		ucUnlawContent = ucstring("Invalid enter room : ") + ucstring("FamilyID = ") + toStringW(_FID) + ucstring(" CHID = ") + toStringW(_CHID);
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
//		return;
//	}
//	FAMILY		*family		= GetFamilyData(_FID);
//	CHARACTER	*character	= GetCharacterData(_CHID);
//	if (family == NULL || character == NULL)
//	{
//		sipwarning("(family == NULL || character == NULL)");
//		return;
//	}
//
//	T_FAMILYNAME	FamilyName;
//	T_F_LEVEL	uLevel = 0;
//	T_F_EXP		exp = 0;
//	T_PRICE		GMoney;
//	uint16		room_count = 0;
//	ucstring	FamilyComment;
//	{
//		MAKEQUERY_LOADFAMILYFORBASE(strQ, _FID);
//		CDBConnectionRest	DB(DBCaller);
//		DB_EXE_QUERY(DB, Stmt, strQ);
//		if (nQueryResult == true)
//		{
//			DB_QUERY_RESULT_FETCH(Stmt, row);
//			if (IS_DB_ROW_VALID(row))
//			{
//				COLUMN_WSTR(row, 1,	_FamilyName,	MAX_LEN_FAMILY_NAME);
//				COLUMN_DIGIT(row, 3, T_F_VISITDAYS, vdays);
//				COLUMN_WSTR(row, 4,	FComment,	MAX_LEN_FAMILYCOMMENT);
//				COLUMN_DIGIT(row, 5, T_F_LEVEL, _uLevel);
//				COLUMN_DIGIT(row, 6, T_F_EXP, _exp);
//				COLUMN_DIGIT(row, 7, T_PRICE, _GMoney);
//				COLUMN_DIGIT(row, 8, uint16, _room_count);
//				FamilyName = _FamilyName;
//				FamilyComment = FComment;
//				uLevel = _uLevel;
//				exp = _exp;
//				GMoney = _GMoney;
//				room_count = _room_count;
//			}
//		}
//	}
//
//	uint8	uRoomRole = GetFamilyRoleInRoom(_FID);
//
//	// tbl_Contact Table이 없어졌으므로 ROOM_CONTACT권한이 있는가 아닌가는 다른 방법으로 판정해야 한다.
//	//	if (uRoomRole == ROOM_NONE) 
//	//		if (IsNeighborObject(_FID))
//	//			uRoomRole = ROOM_CONTACT;
//
//	INFO_FAMILYINROOM	fi(_FID, _CHID, character->m_ModelId, FamilyName, uLevel, exp, GMoney, FamilyComment, uRoomRole, room_count, _sid);
//	fi.m_nChannelID = ChannelID;
//	m_mapFamilys.insert( make_pair(_FID, fi) );
//	SetFamilyWorld(_FID, m_RoomID, WORLD_ROOM);
//
//	// 캐릭터의 상태를 설정한다.
//	SetCharacterState(_FID, _CHID, _CHState, _CHDirection, _CHX, _CHY, _CHZ);
//
//	// 방에 들어오게 될 가족에게 응답을 보낸다
//	CMessage	msgOut0(M_SC_ENTERROOM);
//	uint32		code = ERR_NOERR;
//	msgOut0.serial(_FID, m_RoomID, ChannelID, _CHID, code);
//
//	CUnifiedNetwork::getInstance()->send(_sid, msgOut0);
//	sipinfo(L"Enter room : FID=%d, RoomID=%d, CHID=%d", _FID, m_RoomID, _CHID);
//
//	ChannelData	*pChannelData = GetChannel(ChannelID);
//	if(pChannelData == NULL)
//	{
//		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
//		return;
//	}
//
//	// 새가족의 입방을 통보한다
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_NEWENTER)
//		msgOut.serial(m_RoomID, _FID, family->m_Name, uLevel, room_count);
//		msgOut.serial(character->m_ID, character->m_Name, character->m_ModelId, character->m_DefaultDressID, character->m_FaceID, character->m_HairID, character->m_DressID);
//		msgOut.serial(_CHState, _CHDirection, _CHX, _CHY, _CHZ);
//	FOR_END_FOR_FES_ALL()
//
//	// 접속써비스별 가족목록을 갱신한다.
//	TFesFamilyIds::iterator it0 = m_mapFesFamilys.find(_sid);
//	if (it0 == m_mapFesFamilys.end())
//	{
//		MAP_FAMILYID	newMap;
//		newMap.insert( make_pair(_FID, _FID) );
//		m_mapFesFamilys.insert(make_pair(_sid, newMap));
//	}
//	else
//	{
//		it0->second.insert( make_pair(_FID, _FID) );
//	}
//
//	TFesFamilyIds::iterator it3 = pChannelData->m_mapFesChannelFamilys.find(_sid);
//	if(it3 == pChannelData->m_mapFesChannelFamilys.end())
//	{
//		MAP_FAMILYID	newMap;
//		newMap.insert( make_pair(_FID, _FID) );
//		pChannelData->m_mapFesChannelFamilys.insert(make_pair(_sid, newMap));
//	}
//	else
//	{
//		it3->second.insert( make_pair(_FID, _FID) );
//	}
//
//	// 접속써비스들에 통지한다
//	CMessage	msgNT(M_NTENTERROOM);
//	msgNT.serial(_FID, m_RoomID, m_RoomName, ChannelID);
//	CUnifiedNetwork::getInstance ()->send(FRONTEND_SHORT_NAME, msgNT);
//
//	// World 공동처리를 진행한다.
//	EnterFamilyInWorld(_FID);
//
//	// 방안의 사용자목록을 내려보낸다.
//	SendAllFIDInRoomToOne(_FID, _sid);
//
//	// 움직임 동물목록을 내려보낸다
//	SendAnimal(_FID, _sid);
//
//	if (pChannelData->m_MainFamilyID == NOFAMILYID)
//		pChannelData->ResetMainFamily();
//
//	CMessage	msgHis(M_NT_ADDTOHIS);
//	uint16	fesid = _sid.get();
//	msgHis.serial(_FID, m_RoomID, uRoomRole, fesid);
//	CUnifiedNetwork::getInstance()->send(HISMANAGER_SHORT_NAME, msgHis);
//
//	EnterFamily_Spec(_FID, _sid);
//
//	// PlayerManager에 통지
//	{
//		CMessage	msgOut(M_NT_PLAYER_ENTERROOM);
//		msgOut.serial(_FID, m_RoomID, ChannelID);
//		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
//	}
//
//	// Log
//	DBLOG_EnterRoom(_FID, m_RoomID, m_RoomName);
//
//	sipdebug("m_mapFamilys(Room:%d) Added. size=%d", m_RoomID, m_mapFamilys.size());
//}

// 방안의 모든 캐릭터들의 정보를 보낸다
void	CChannelWorld::SendAllFIDInRoomToOne(T_FAMILYID _FID, TServiceId sid)
{
	T_ID	ChannelID = GetFamilyChannelID(_FID);
	ChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. ChannelID=%d, FID=%d", ChannelID, _FID);
		return;
	}

	{
		// 메쎄지 만들기
		CMessage	mesOut(M_SC_ALLINROOM);
		mesOut.serial(_FID, m_RoomID);
		map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator	it0;
		for (it0 = m_mapFamilys.begin(); it0 != m_mapFamilys.end(); it0++)
		{
			// 같은 선로에 있는 family의 정보만 보낸다.
			if(it0->second.m_nChannelID != ChannelID)
				continue;

			T_FAMILYID fid = it0->first;
			FAMILY		*family		= GetFamilyData(fid);
			if (family == NULL)
			{
				sipwarning("There is no family data targetFID=%d, toFID=%d, RoomID=%d", fid, _FID, m_RoomID);
				continue;
			}
			uint8	uShowType = family->m_uShowType;

			// 가족id, 가족이름
			mesOut.serial(family->m_ID, family->m_Name, it0->second.m_Level, it0->second.m_Exp, it0->second.m_RoomCount, family->m_bIsMobile);

			// 캐릭터수
			uint8	uChNum = it0->second.m_mapCH.size();

			map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator	it1;
			for (it1 = it0->second.m_mapCH.begin(); it1 != it0->second.m_mapCH.end(); it1 ++)
			{
				T_CHARACTERID cid = it1->first;
				CHARACTER	*character = GetCharacterData(cid);
				if (character == NULL)
				{
					sipwarning("There is no character data, FID=%d CHID=%d", fid, cid);
					break;
				}
				mesOut.serial(uChNum);

				mesOut.serial(character->m_ID, character->m_Name, character->m_ModelId, //////////////////////////////////////////////////////////////////////////
					character->m_DefaultDressID, character->m_FaceID, character->m_HairID, character->m_DressID);
				mesOut.serial(it1->second.m_X, it1->second.m_Y, it1->second.m_Z, it1->second.m_Direction, it1->second.m_ucAniName, it1->second.m_AniState, it1->second.m_HoldItemID, uShowType);

				break;
			}
			if(mesOut.length() > MAX_PACKET_BUF_LEN - 500)
			{
				CUnifiedNetwork::getInstance()->send(sid, mesOut);	

				CMessage	mesTemp(M_SC_ALLINROOM);
				mesTemp.serial(_FID, m_RoomID);
				mesOut = mesTemp;
			}
		}
		// 메쎄지송신
		CUnifiedNetwork::getInstance()->send(sid, mesOut);
	}

	// 방안의 의자에 앉아있는 사용자 목록을 내려보낸다.
	{
		CMessage	msgOut(M_SC_CHAIR_LIST);
		msgOut.serial(_FID);
		for (uint32 i = 1; i <= MAX_CHAIR_COUNT; i ++)
		{
			if (pChannelData->m_ChairFIDs[i] != 0)
			{
				msgOut.serial(i, pChannelData->m_ChairFIDs[i]);
			}
		}
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}
}

// 타고 있는 상서동물목록을 내려보낸다
void	CChannelWorld::SendMountAnimal(T_FAMILYID _FID, TServiceId sid)
{
	ChannelData	*pChannelData = GetFamilyChannel(_FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel=NULL. RoomID=%, FID=%d", m_RoomID, _FID);
		return;
	}

	// 타고 있는 상서동물 정보
	if (pChannelData->m_MountLuck3FID != 0)
	{
		uint32	zero = 0;
		bool	bSuccess = true;
		CMessage	msg(M_SC_MOUNT_LUCKANIMAL);
		msg.serial(_FID, zero);
		msg.serial(pChannelData->m_MountLuck3FID, pChannelData->m_MountLuck3AID, bSuccess);
		CUnifiedNetwork::getInstance()->send(sid, msg);
	}
}

// 움직임 동물목록을 내려보낸다
void	CChannelWorld::SendAnimal(T_FAMILYID _FID, TServiceId sid)
{
	ChannelData	*pChannelData = GetFamilyChannel(_FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel=NULL. RoomID=%, FID=%d", m_RoomID, _FID);
		return;
	}

	if (pChannelData->m_RoomAnimal.size() > 0)
	{
		// 메쎄지 만들기
		CMessage	mesOut(M_SC_ANIMAL_ADD);
		mesOut.serial(_FID);

		MAP_ROOMANIMAL::iterator	it;
		for (it = pChannelData->m_RoomAnimal.begin(); it != pChannelData->m_RoomAnimal.end(); it ++)
		{
			mesOut.serial(it->second.m_ID, it->second.m_TypeID, it->second.m_PosX, it->second.m_PosY, it->second.m_PosZ, it->second.m_Direction, it->second.m_ServerControlType);			
		}
		// 메쎄지송신
		CUnifiedNetwork::getInstance()->send(sid, mesOut);
	}

	// for EditedAnimal
	pChannelData->SendEditAnimalInfo(_FID, sid);
}

// Family Sitdown on a chair ([d:ChairID])
void	cb_M_CS_CHAIR_SITDOWN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ID			ChairID;
	_msgin.serial(FID, ChairID);

	// 방관리자를 얻는다
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
		return;
	}

	if(ChairID < 1 || ChairID > MAX_CHAIR_COUNT)
	{
		sipwarning("Invalid ChairID. ChairID=%d, FID=%d", ChairID, FID);
		return;
	}
	if(pChannelData->m_ChairFIDs[ChairID] != 0)
	{
		sipdebug("pChannelData->m_ChairFIDs[ChairID] != 0. ChairID=%d, FID=%d, PrevFID=%d", ChairID, FID, pChannelData->m_ChairFIDs[ChairID]);
		return;
	}

	// 이미 앉아있는 경우 처리
	for(int i = 1; i <= MAX_CHAIR_COUNT; i ++)
	{
		if(pChannelData->m_ChairFIDs[i] == FID)
		{
			sipwarning("Already sitdown in %d. FID=%d, RequestChairID=%d", i, FID, ChairID);
			return;
		}
	}

	// OK
	pChannelData->m_ChairFIDs[ChairID] = FID;

	// 방안의 모든 가족들에게 통보한다
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_CHAIR_SITDOWN)
		msgOut.serial(FID, ChairID);
	FOR_END_FOR_FES_ALL()
}

// Family Standup ([d:ChairID])
void	cb_M_CS_CHAIR_STANDUP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ID			ChairID;
	_msgin.serial(FID, ChairID);

	// 방관리자를 얻는다
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	// 방안의 모든 가족들에게 통보한다
	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
		return;
	}

	if(ChairID < 1 || ChairID > MAX_CHAIR_COUNT)
	{
		sipwarning("Invalid ChairID. ChairID=%d, FID=%d", ChairID, FID);
		return;
	}
	if(pChannelData->m_ChairFIDs[ChairID] != FID)
	{
		sipdebug("pChannelData->m_ChairFIDs[ChairID] != FID. ChairID=%d, FID=%d, PrevFID=%d", ChairID, FID, pChannelData->m_ChairFIDs[ChairID]);
		return;
	}
	pChannelData->m_ChairFIDs[ChairID] = 0;

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_CHAIR_STANDUP)
		msgOut.serial(FID, ChairID);
	FOR_END_FOR_FES_ALL()
}

void	cb_M_CS_MOVECH(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint8			uNum;

	_msgin.serial(FID, uNum);

	if (uNum < 1)
		return;

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
	{
		return;
	}

	bool	bNoWait = false;
	for (uint8 i = 0; i < uNum;	i++)
	{
		T_CHARACTERID CHID;		_msgin.serial(CHID);
		T_CH_X X;				_msgin.serial(X);
		T_CH_Y Y;				_msgin.serial(Y);
		T_CH_Z Z;				_msgin.serial(Z);
		inManager->SetCharacterPos(FID, CHID, X, Y, Z);
	}

}
void	cb_M_CS_STATECH(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint8			uNum;

	_msgin.serial(FID, uNum);

	if (uNum < 1)
		return;

	// 방관리자를 얻는다
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
	{
		// 비법발견
		//		ucstring ucUnlawContent;
		//		ucUnlawContent = ucstring("Invalid character state : ") + ucstring("FamilyID = ") + toStringW(FID);
		//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	// 정보를 갱신한다
	bool	bNoWait = false;
	for (uint8 i = 0; i < uNum;	i++)
	{
		T_CHARACTERID CHID;		_msgin.serial(CHID);
		T_CH_X X;				_msgin.serial(X);
		T_CH_Y Y;				_msgin.serial(Y);
		T_CH_Z Z;				_msgin.serial(Z);
		T_CH_DIRECTION Dir;		_msgin.serial(Dir);
		ucstring	ucAniName;	NULLSAFE_SERIAL_WSTR(_msgin, ucAniName, MAX_LEN_ANIMATION_NAME, FID);
		T_CH_STATE	aniState;	_msgin.serial(aniState);
		T_FITEMID	itemid;		_msgin.serial(itemid);
/*
		if(State & FLAG_MOVESTATE_NOWAIT)
		{
			bNoWait = true;
			State &= (~FLAG_MOVESTATE_NOWAIT);
		}
*/
		inManager->SetCharacterState(FID, CHID, Dir, X, Y, Z, ucAniName, aniState, itemid);
	}

	// 방안의 모든 가족들에게 통보한다
	inManager->NotifyFamilyState(FID);
}

void	cb_M_CS_ACTION(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID, ActionID, TargetFID;
	_msgin.serial(FID, ActionID, TargetFID);

	// 방관리자를 얻는다
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	// 방안의 모든 가족들에게 통보한다
	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
		return;
	}

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ACTION)
		msgOut.serial(FID, ActionID, TargetFID);
	FOR_END_FOR_FES_ALL()
}

// 방안의 모든 가족들에게 _FID가족의 상태를 알린다
void	CChannelWorld::NotifyFamilyState(T_FAMILYID _FID)
{
	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator	itF = m_mapFamilys.find(_FID);
	if (itF == m_mapFamilys.end())
		return;

	uint32	ChannelID = itF->second.m_nChannelID;
	ChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		return;
	}

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_STATECH)
		msgOut.serial(m_RoomID, _FID);
		uint8	uChNum = itF->second.m_mapCH.size();
		msgOut.serial(uChNum);
		map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator	itC;
		for (itC = itF->second.m_mapCH.begin(); itC != itF->second.m_mapCH.end(); itC ++)
		{
			msgOut.serial(itC->second.m_CHID, itC->second.m_X, itC->second.m_Y, itC->second.m_Z, itC->second.m_Direction);
			msgOut.serial(itC->second.m_ucAniName, itC->second.m_AniState, itC->second.m_HoldItemID);
		}
	FOR_END_FOR_FES_ALL()
}

//// 캐릭터의 메쏘드를 실행시킨다
//void	cb_M_CS_CHMETHOD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;			_msgin.serial(FID);			// 가족id
//	T_CHARACTERID		CHID;			_msgin.serial(CHID);		// 캐릭터ID
//	ucstring			ucMethod;		SAFE_SERIAL_WSTR(_msgin, ucMethod, MAX_LEN_ANIMATION_NAME, FID);
//	uint32				uParam;			_msgin.serial(uParam);
//
//	CChannelWorld *	room = GetChannelWorldFromFID(FID);
//	if(room == NULL)
//	{
//		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
//		return;
//	}
//	room->ExeMethod(FID, CHID, ucMethod, uParam);
//}

//// execute method
//void CChannelWorld::ExeMethod(T_FAMILYID _FID, T_CHARACTERID _CHID, ucstring _ucMethod, uint32 _uParam)
//{
//	if(!IsInFamily(_FID, _CHID))
//	{
//		sipwarning("Failed execute method(No character %d in family %d)", _CHID, _FID);
//		return;
//	}
//
//	ChannelData	*pChannelData = GetFamilyChannel(_FID);
//	if(pChannelData == NULL)
//	{
//		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, _FID);
//		return;
//	}
//
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_CHMETHOD)
//		msgOut.serial(_FID, _CHID, _ucMethod, _uParam);
//	FOR_END_FOR_FES_ALL()
//}

//int CChannelWorld::CheckChangeChannelFamily(T_FAMILYID FID, uint32 ChannelID)
//{
///*
//	// Channel이 진입가능한지 검사한다.
//	uint32	OldChannelID = GetFamilyChannelID(FID);
//	if(OldChannelID == 0)
//		return 1;	// 방에 없음.
//
//	if(OldChannelID == ChannelID)
//		return 2;	// 같은 Channel임 - ChannelID 오유
//*/
//	ChannelData	*pChannelData = GetChannel(ChannelID);
//	if(pChannelData == NULL)
//		return 2;	// Channel이 존재하지 않음. - ChannelID 오유
//
//	int		user_count = 0;
//	for(TFesFamilyIds::iterator itFes = pChannelData->m_mapFesChannelFamilys.begin();
//		itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
//	{
//		user_count += itFes->second.size();
//	}
//	if (user_count >= m_nMaxCharacterOfRoomKind)
//		return 3;	// Channel Full
//
//	return 0;
//}

//void	CChannelWorld::OutChannel(T_FAMILYID FID, SIPNET::TServiceId sid)
//{
//	INFO_FAMILYINROOM	*familyInfo = GetFamilyInfo(FID);
//	if(familyInfo == NULL)
//	{
//		sipwarning("GetFamilyInfo = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
//		return;
//	}
//	uint32	OldChannelID = familyInfo->m_nChannelID;
//
//	ChannelData	*pOldChannelData = GetChannel(OldChannelID);
//	if(pOldChannelData == NULL)
//	{
//		sipwarning("pOldChannelData = NULL. RoomID=%d, OldChannelID=%d", m_RoomID, OldChannelID);
//		return;
//	}
//	if (OldChannelID >= ROOM_SPECIAL_CHANNEL_START && pOldChannelData->GetChannelUserCount() <= 1)
//	{
//		AddRemoveSpecialChannel(OldChannelID);
//	}
//
//	// 사용자의 모든 활동자료를 초기화한다.
//	ClearAllFamilyActions(FID);
//	// 현재 선로의 사용자들에게 통지한다.
//	{
//		FOR_START_FOR_FES_ALL(pOldChannelData->m_mapFesChannelFamilys, msgOut, M_SC_OUTROOM)
//			msgOut.serial(m_RoomID, FID);
//		FOR_END_FOR_FES_ALL()
//	}
//
//	uint32 newChannelID = 0;
//	familyInfo->m_nChannelID = newChannelID;
//
//	// 접속써비스들에 통지한다
//	CMessage	msgTOFS(M_NTCHANGEROOMCHANNEL);
//	msgTOFS.serial(FID, m_RoomID, newChannelID);
//	CUnifiedNetwork::getInstance ()->send(FRONTEND_SHORT_NAME, msgTOFS);
//
//	// 접속써비스별 가족목록을 갱신한다.
//	{
//		TFesFamilyIds::iterator it3 = pOldChannelData->m_mapFesChannelFamilys.find(sid);
//		if(it3 == pOldChannelData->m_mapFesChannelFamilys.end())
//		{
//			sipwarning("Can't find FES in OldChannel. RoomID=%d, OldChannelID=%d", m_RoomID, OldChannelID);
//		}
//		else
//		{
//			it3->second.erase(FID);
//			if (pOldChannelData->m_MainFamilyID == FID)
//				pOldChannelData->ResetMainFamily();
//		}
//	}
//	sipinfo(L"Out Channel : FID=%d, RoomID=%d, ChannelID=%d", FID, m_RoomID, OldChannelID);
//}

// Channel을 변경시킨다.
//void CChannelWorld::ChangeChannelFamily(uint32 ChannelID, T_FAMILYID FID, T_CHARACTERID CHID, T_CH_DIRECTION CHDirection, T_CH_X CHX, T_CH_Y CHY, T_CH_Z CHZ, SIPNET::TServiceId sid)
//{
//	INFO_FAMILYINROOM	*familyInfo = GetFamilyInfo(FID);
//	if(familyInfo == NULL)
//	{
//		sipwarning("GetFamilyInfo = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
//		return;
//	}
//
//	FAMILY		*family		= GetFamilyData(FID);
//	CHARACTER	*character	= GetCharacterData(CHID);
//	if (family == NULL || character == NULL)
//	{
//		sipwarning("(family == NULL || character == NULL)");
//		return;
//	}
//
//	ChannelData	*pChannelData = GetChannel(ChannelID);
//	if(pChannelData == NULL)
//	{
//		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
//		return;
//	}
//
//	familyInfo->m_nChannelID = ChannelID;
//
//	ucstring	ucAniName = L"";
//	T_CH_STATE	aniState = 0;
//	T_FITEMID	holdItem = 0;
//	SetCharacterState(FID, CHID, CHDirection, CHX, CHY, CHZ, ucAniName, aniState, holdItem);
//
//	// 방에 들어오게 될 가족에게 응답을 보낸다
//	CMessage	msgOut0(M_SC_CHANGECHANNELIN);
//	uint32		code = ERR_NOERR;
//	msgOut0.serial(FID, ChannelID, CHID, code);
//	CUnifiedNetwork::getInstance()->send(sid, msgOut0);
//	sipinfo(L"Change Channel : FID=%d, RoomID=%d, CHID=%d, ChannelID=%d", FID, m_RoomID, CHID, ChannelID);
//
//	// 접속써비스들에 통지한다
//	CMessage	msgTOFS(M_NTCHANGEROOMCHANNEL);
//	msgTOFS.serial(FID, m_RoomID, ChannelID);
//	CUnifiedNetwork::getInstance ()->send(FRONTEND_SHORT_NAME, msgTOFS);
//
//	// 새가족의 입방을 통보한다
//	{
//		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_NEWENTER)
//			msgOut.serial(m_RoomID, FID, family->m_Name, familyInfo->m_Level, familyInfo->m_Exp, familyInfo->m_RoomCount, family->m_bIsMobile);
//			msgOut.serial(character->m_ID, character->m_Name, character->m_ModelId, character->m_DefaultDressID, character->m_FaceID, character->m_HairID, character->m_DressID);
//			msgOut.serial(CHX, CHY, CHZ, CHDirection, ucAniName, aniState, holdItem);
//		FOR_END_FOR_FES_ALL()
//	}
//
//	{
//		TFesFamilyIds::iterator it3 = pChannelData->m_mapFesChannelFamilys.find(sid);
//		if(it3 == pChannelData->m_mapFesChannelFamilys.end())
//		{
//			MAP_FAMILYID	mapFes;
//			mapFes.insert(make_pair(FID, FID));
//			pChannelData->m_mapFesChannelFamilys.insert(make_pair(sid, mapFes));
//		}
//		else
//		{
//			it3->second.insert(make_pair(FID, FID));
//		}
//	}
//
//	// 방안의 사용자목록을 내려보낸다.
//	SendAllFIDInRoomToOne(FID, sid);
//
//	// 움직임 동물목록을 내려보낸다
//	SendAnimal(FID, sid);
//	SendMountAnimal(FID, sid);
//
//	if (pChannelData->m_MainFamilyID == NOFAMILYID)
//		pChannelData->ResetMainFamily();
//
//	ChangeChannelFamily_Spec(FID, sid);
//}

// 현재 리용되지 않는다!!! - 그러므로 삭제함. 2015/06/08 by RSC
void cb_M_CS_CHANGECHANNELOUT(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	sipwarning("Not supported!!!");
	//T_FAMILYID		FID;			_msgin.serial(FID);			// 가족id
	//uint32			ChannelID;		_msgin.serial(ChannelID);

	//CChannelWorld *roomManager = GetChannelWorldFromFID(FID);
	//if (roomManager == NULL)
	//{
	//	sipwarning("There is no enter room manager(FID=%d)", FID);
	//	return;
	//}
	//roomManager->OutChannel(FID, _tsid);

	//// ManagerService에 통지
	//{
	//	uint32 newchannelid = 0;
	//	CMessage	msgOut(M_NT_PLAYER_CHANGECHANNEL);
	//	msgOut.serial(FID, roomManager->m_RoomID, newchannelid);
	//	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	//}
}

// 현재 리용되지 않는다!!! - 그러므로 삭제함. 2015/06/08 by RSC
void cb_M_CS_CHANGECHANNELIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	sipwarning("Not supported!!!");
	//T_FAMILYID		FID;			_msgin.serial(FID);			// 가족id
	//uint32			ChannelID;		_msgin.serial(ChannelID);
	//T_CHARACTERID	CHID;			_msgin.serial(CHID);		// 캐릭터id
	//T_CH_DIRECTION	CHDirection;	_msgin.serial(CHDirection);
	//T_CH_X			CHX;			_msgin.serial(CHX);
	//T_CH_Y			CHY;			_msgin.serial(CHY);
	//T_CH_Z			CHZ;			_msgin.serial(CHZ);

	//sipinfo("character id:%d Change Room Channel Direction %f, Position(%f, %f, %f)", CHID, CHDirection, CHX, CHY, CHZ);

	//CChannelWorld *roomManager = GetChannelWorldFromFID(FID);
	//if (roomManager == NULL)
	//{
	//	sipwarning("There is no enter room manager(FID=%d)", FID);
	//	return;
	//}
	//int	checkresult = roomManager->CheckChangeChannelFamily(FID, ChannelID);

	//switch(checkresult)
	//{
	//case 0:
	//	// 방에 들여보낸다.
	//	roomManager->ChangeChannelFamily(ChannelID, FID, CHID, CHDirection, CHX, CHY, CHZ, _tsid);

	//	// ManagerService에 통지
	//	{
	//		CMessage	msgOut(M_NT_PLAYER_CHANGECHANNEL);
	//		msgOut.serial(FID, roomManager->m_RoomID, ChannelID);
	//		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	//	}
	//	break;
	//case 1:	// 현재의 방에 없음 - 부정한 사용자.
	//	break;
	//case 2:	// ChannelID 오유
	//case 3:	// Channel Full
	//	{
	//		uint32	ChID = 0, err = E_ENTER_NO_FULL;
	//		CMessage	msgOut(M_SC_CHANGECHANNELIN);
	//		msgOut.serial(FID, ChannelID, ChID, err);
	//		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	//	}
	//	break;
	//}
}

// Normal Chatting
void CChannelWorld::SendChatCommon(T_FAMILYID FID, T_CHATTEXT Chat)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
		return;

	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if(pInfo == NULL)
		return;

	T_CH_DIRECTION	dir;
	T_CH_X			x, y, z, x1, y1, z1;

	if(!pInfo->GetFirstCHState(dir, x, y, z))
		return;

	uint32 uZero = 0;
	TFesFamilyIds::iterator it;
	for(it = pChannelData->m_mapFesChannelFamilys.begin(); it != pChannelData->m_mapFesChannelFamilys.end(); it ++)
	{
		if(it->second.size() == 0)
			continue;

		int	count = 0;
		CMessage	msgOut(M_SC_CHATCOMMON);
		MAP_FAMILYID::iterator f;
		for(f = it->second.begin(); f != it->second.end(); f ++)
		{
			T_FAMILYID fid = f->first;

			pInfo = GetFamilyInfo(fid);
			if(pInfo == NULL)
			{
				sipwarning("GetFamilyInfo = NULL. FID=%d", fid);
				continue;
			}

			if(!pInfo->GetFirstCHState(dir, x1, y1, z1))
				continue;

			if(CalcDistance2(x, y, z, x1, y1, z1) > COMMON_CHAT_DISTANCE * COMMON_CHAT_DISTANCE)
				continue;

			count ++;
			msgOut.serial(fid);
		}
		if(count == 0)
			continue;

		msgOut.serial(uZero);

		msgOut.serial(FID, Chat);

		CUnifiedNetwork::getInstance()->send(it->first, msgOut);
	}
}

// Room Chatting
void CChannelWorld::SendChatRoom(T_FAMILYID FID, T_CHATTEXT Chat)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
		return;

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_CHATROOM)
		msgOut.serial(FID, Chat);
	FOR_END_FOR_FES_ALL()

	if (!IsSystemRoom(m_RoomID))
		return;

	FAMILY	*pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
		return;
	
	// 방챠팅내용을 GM에게 보낸다.
	CMessage msgGM(M_GMSC_ROOMCHAT);
	msgGM.serial(m_RoomID, pChannelData->m_ChannelID, m_RoomName, FID, pFamily->m_Name, Chat);
	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgGM);
}

// Set recommend room ([b:Index][d:RoomID][d:PhotoID][d:Date]), Index: 0~4
//void cb_M_GMCS_RECOMMEND_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	uint8		Index;
//	uint32		GMFID, RoomID, PhotoID, Date;
//
//	_msgin.serial(GMFID, Index, RoomID, PhotoID, Date);
//
//	MAKEQUERY_SetRecommendRoom(strQuery, Index, RoomID, PhotoID, Date);
//	DBCaller->execute(strQuery);
//}

// Add a Room Daily Event ([d:RoomID][b:EventType])
void cb_M_GMCS_ROOMEVENT_ADD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMFID;
	T_ROOMID		RoomID;
	uint8			RoomEventType;

	_msgin.serial(GMFID, RoomID, RoomEventType);

	CChannelWorld *inManager = GetChannelWorld(RoomID);
	if (inManager == NULL)
	{
		sipwarning("GetOpenRoomWorld = NULL. GMFID=%d, RoomID=%d", GMFID, RoomID);
		return;
	}

	inManager->StartRoomEventByGM(RoomEventType);
}

// 방의 동물들을 움직인다.
void	CChannelWorld::RefreshAnimal()
{
	if ( m_mapFamilys.empty() )
		return;

	MAP_OUT_ROOMKIND::iterator itroomkind = map_OUT_ROOMKIND.find(m_SceneID);
	if (itroomkind == map_OUT_ROOMKIND.end())
	{
		sipwarning("Room kind info = NULL. SceneID=%d", m_SceneID);
		return;
	}

	TTime	curtime = SIPBASE::CTime::getLocalTime();
	//	static	TTime	LastTime = 0;
	//	if ( curtime > LastTime && curtime - LastTime < ANIMAL_UPDATE_INTERVAL )
	//		return;
	//	LastTime = curtime;

	for(ChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		if ( pChannelData->m_RoomAnimal.empty() )
			continue;

		std::vector<T_COMMON_ID>	changes;
		MAP_ROOMANIMAL::iterator	it;
		for (it = pChannelData->m_RoomAnimal.begin(); it != pChannelData->m_RoomAnimal.end(); it++)
		{
			ROOMANIMAL *CurAnimal = &(it->second);
			if (CurAnimal->m_SelFID != 0)
				continue;
			if (CurAnimal->m_NextMoveTime.timevalue > curtime)
				continue;
			if (CurAnimal->m_ServerControlType == SCT_NO)
				continue;

			const REGIONSCOPE* scope = itroomkind->second.GetRegionScope(CurAnimal->m_RegionID, pChannelData->getChannelType()); // by krs
			if (scope == NULL)
			{
				sipwarning("GetRegionScope of Room kind info = NULL. SceneID=%d, RegionID=%d", m_SceneID, CurAnimal->m_RegionID);
				continue;
			}

			OUT_ANIMALPARAM			*AnimalParam = GetOutAnimalParam(CurAnimal->m_TypeID);
			if (AnimalParam == NULL)
				continue;
			sipassert(AnimalParam->MoveParam > 0);
			int	interval = AnimalParam->MoveParam * 1000;
			CurAnimal->m_NextMoveTime.timevalue = (TTime)(curtime + interval + Irand(0, (int)(interval * ANIMAL_MOVE_DELTATIME)));

			T_POSX	NextX = UNKNOWN_POS;
			T_POSY	NextY = UNKNOWN_POS;
			T_POSZ	NextZ = UNKNOWN_POS;

			float	MaxMoveDistance = AnimalParam->MaxMoveDis;
			if (pChannelData->getChannelType() != 0) // by krs
			{
				MaxMoveDistance /= PC_MOBILE_DISTANCE_RATE;
			}

			if ( ! scope->GetMoveablePos(CurAnimal->m_PosX, CurAnimal->m_PosY, CurAnimal->m_PosZ, MaxMoveDistance,
				&NextX, &NextY, &NextZ) )
				continue;

			T_POSZ	deltaZ = (NextZ - CurAnimal->m_PosZ) * (-1.0f);
			T_POSX	deltaX = (NextX - CurAnimal->m_PosX) * (-1.0f);
			if (deltaZ == 0)
				CurAnimal->m_Direction = 0.0f;
			else
				CurAnimal->m_Direction = atan2(deltaX, deltaZ);
			CurAnimal->m_PosX = NextX;
			CurAnimal->m_PosY = NextY;
			CurAnimal->m_PosZ = NextZ;

			changes.push_back(CurAnimal->m_ID);
		}
		if ( !changes.empty() )
		{
			static uint8	AnimalMoveType = 0;
			extern uint32	NextFishFoodTime;
			if (NextFishFoodTime > 0)
			{
				if (NextFishFoodTime < (uint32)GetDBTimeSecond())
				{
					NextFishFoodTime /= 10000;
					sipwarning("NextFishFoodTime overflow. RoomID=%d, NextFishFoodTime=%s", m_RoomID, NextFishFoodTime);
					NextFishFoodTime = 0;
				}
			}
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ANIMALMOVE)
				uint8	bIsEscape = 0;
				std::vector<T_COMMON_ID>::iterator itm;
				for ( itm = changes.begin(); itm != changes.end(); itm ++ )
				{
					T_COMMON_ID	id = *itm;
					ROOMANIMAL &	animal = pChannelData->m_RoomAnimal[id];
					msgOut.serial(animal.m_ID, animal.m_PosX, animal.m_PosY, animal.m_PosZ, AnimalMoveType, bIsEscape);
					AnimalMoveType ++;
				}
			FOR_END_FOR_FES_ALL();
		}
	}
}

void	CChannelWorld::RefreshCharacterState()
{
	if ( m_mapFamilys.size() == 0 )
		return;
	TTime	curtime = SIPBASE::CTime::getLocalTime();
	if ( curtime > m_tmLastUpdateCharacterState && curtime - m_tmLastUpdateCharacterState < 1000 )
		return;
	bool	bAllRefresh = false;
//	if ( curtime - m_tmLastRefreshCharacterState > 180000 )
//	{
//		bAllRefresh = true;
//		m_tmLastRefreshCharacterState = curtime;
//	}
	const size_t szLenPerFamily = sizeof(T_FAMILYID) + sizeof(uint8) + sizeof(T_CHARACTERID) + 
		sizeof(T_CH_POS) * 3;

	const uint32 nFNumPerPacket = (MAX_PACKET_BUF_LEN - 500) / szLenPerFamily;
	sipassert(nFNumPerPacket > 0);

	for(ChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		// Packet자료를 만든다.
#define MSG_TMP_COUNT	10	// 10개면 충분하다...
		CMessage	msgChInfo[MSG_TMP_COUNT];
		uint32		nGroupCount = 0;
		uint32		nFamilyCount = 0;
		TFesFamilyIds::iterator	itFes;
		for(itFes = pChannelData->m_mapFesChannelFamilys.begin(); itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
		{
			for(MAP_FAMILYID::iterator itF = itFes->second.begin(); itF != itFes->second.end(); itF ++)
			{
				T_FAMILYID curFamilyID = itF->first;
				INFO_FAMILYINROOM	*pFamily = GetFamilyInfo(curFamilyID);
				if(pFamily == NULL)
				{
					sipwarning("GetFamilyInfo = NULL. FID=%d, RoomID=%d, ChannelID=%d", curFamilyID, m_RoomID, pChannelData->m_ChannelID);
					continue;
				}

				if(!pFamily->m_bStatusChanged && !bAllRefresh)
					continue;

				pFamily->m_bStatusChanged = false;

				if(nFamilyCount == 0)
				{
					msgChInfo[nGroupCount] = CMessage(M_SC_MOVECH);
					msgChInfo[nGroupCount].serial(m_RoomID);
				}
				msgChInfo[nGroupCount].serial(curFamilyID);
				uint8	uChNum = pFamily->m_mapCH.size();
				msgChInfo[nGroupCount].serial(uChNum);
				map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator	itC;
				for (itC = pFamily->m_mapCH.begin(); itC != pFamily->m_mapCH.end(); itC ++)
					msgChInfo[nGroupCount].serial(itC->second.m_CHID, itC->second.m_X, itC->second.m_Y, itC->second.m_Z);

				nFamilyCount++;

				// if (nFamilyCount >= nFNumPerPacket)
				if (msgChInfo[nGroupCount].length() >= (MAX_PACKET_BUF_LEN - 1024 - szLenPerFamily))
				{
					nGroupCount ++;
					nFamilyCount = 0;
					if(nGroupCount > MSG_TMP_COUNT)
					{
						sipwarning("nGroupCount > 10");
						break;
					}
				}
			}
		}
		if(nFamilyCount > 0)
			nGroupCount ++;

		if(nGroupCount == 0)
			continue;

		// Packet를 보낸다.
		for(itFes = pChannelData->m_mapFesChannelFamilys.begin(); itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
		{
			if(itFes->second.size() == 0)
				continue;
			CMessage	msgOut0(M_SC_MOVECH);
			for(MAP_FAMILYID::iterator itF = itFes->second.begin(); itF != itFes->second.end(); itF ++)
			{
				T_FAMILYID fid = itF->first;
				msgOut0.serial(fid);
			}
			uint32 uZero = 0;
			msgOut0.serial(uZero);

			for(uint32 i = 0; i < nGroupCount; i ++)
			{
				CMessage	msgOut = msgOut0;
				msgChInfo[i].appendBufferTo(msgOut);
				CUnifiedNetwork::getInstance()->send(itFes->first, msgOut);
			}
		}
	}

	m_tmLastUpdateCharacterState = curtime;
}

//// 아이템을 든다
//void	cb_M_CS_HOLDITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	T_CHARACTERID		CHID;				_msgin.serial(CHID);			// 캐릭터id
//	T_FITEMID			ItemID;				_msgin.serial(ItemID);			// ItemID
//
//	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
//	if (inManager == NULL)
//		return;			// 비법발견
//
//	inManager->HoldItem(FID, CHID, ItemID, _tsid);
//}

//void	CChannelWorld::HoldItem(T_FAMILYID _FID,  T_CHARACTERID _CHID, T_FITEMID ItemID, SIPNET::TServiceId _sid)
//{
//	// OK
//	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator	itF = m_mapFamilys.find(_FID);
//	if (itF == m_mapFamilys.end())
//	{
//		sipwarning("Can't find in m_mapFamilys.");
//		return;
//	}
//
//	INFO_FAMILYINROOM *f = &(itF->second);
//	f->SetCharacterHoldItem(_CHID, ItemID);
//
//	uint32	ChannelID = itF->second.m_nChannelID;
//	ChannelData	*pChannelData = GetChannel(ChannelID);
//	if(pChannelData == NULL)
//	{
//		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
//		return;
//	}
//
//	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_HOLDITEM)
//		msgOut.serial(m_RoomID, _FID, _CHID, ItemID);
//	FOR_END_FOR_FES_ALL()
//}

void	ChannelData::ResetMainFamily()
{
	T_FAMILYID MainFamilyID = NOFAMILYID;
	TServiceId mainsr;
	for(TFesFamilyIds::iterator itFes = m_mapFesChannelFamilys.begin();	itFes != m_mapFesChannelFamilys.end(); itFes ++)
	{
		if (itFes->second.size() > 0)
		{
			MAP_FAMILYID::iterator itm = itFes->second.begin();
			MainFamilyID = itm->first;
			mainsr.set(itFes->first.get());
			break;
		}
	}
	if (MainFamilyID != NOFAMILYID)
	{
		CMessage msg(M_SC_EANOTIFYMAINCLIENT);
		msg.serial(MainFamilyID);
		bool bRequestInit = false;
		if (!m_bValidEditAnimal)
		{
			bRequestInit = true;
			msg.serial(bRequestInit);
		}
		else
		{
			msg.serial(bRequestInit);
			MAP_ROOMEDITANIMALACTIONINFO::iterator itg;
			for (itg = m_RoomEditAnimalActionInfo.begin(); itg != m_RoomEditAnimalActionInfo.end(); itg++)
			{
				uint16 gid = static_cast<uint16>(itg->first);
				uint16 infolength = static_cast<uint16>(itg->second.m_ActionInfo.size());
				msg.serial(gid, infolength);
				uint8* buf = (uint8*)&(*itg->second.m_ActionInfo.begin());
				msg.serialBuffer(buf, infolength);
			}
		}
		CUnifiedNetwork::getInstance()->send(mainsr, msg);
	}
	m_MainFamilyID = MainFamilyID;
	//sipdebug("Change main client to family=%d, RoomID=%d, ChannelID=%d", m_MainFamilyID, m_RoomID, m_ChannelID); byy krs
}

void	ChannelData::SendEditAnimalInfo(T_FAMILYID _FID, TServiceId sid)
{
	if (!m_bValidEditAnimal)
		return;

	CMessage msg(M_SC_EASETPOS);
	uint32 uZero = 0;
	msg.serial(_FID, uZero);

	// Group base information
	{
		uint16 num = static_cast<uint16>(m_RoomEditAnimalActionInfo.size());
		msg.serial(num);
		MAP_ROOMEDITANIMALACTIONINFO::iterator it;
		for (it = m_RoomEditAnimalActionInfo.begin(); it != m_RoomEditAnimalActionInfo.end(); it++)
		{
			ROOMEDITANIMALACTIONINFO& group = it->second;
			uint16 groupid = static_cast<uint16>(group.m_GroupID);
			uint16 infolen = static_cast<uint16>(group.m_ActionInfo.size());
			msg.serial(groupid, infolen);
			uint8* buf = (uint8*)&(*group.m_ActionInfo.begin());
			msg.serialBuffer(buf, infolen);
		}
	}
	{
		// Animal init position
		uint16 num = static_cast<uint16>(m_RoomEditAnimal.size());
		msg.serial(num);

		MAP_ROOMEDITANIMAL::iterator it;
		for (it = m_RoomEditAnimal.begin(); it != m_RoomEditAnimal.end(); it ++)
		{
			uint16 uid = static_cast<uint16>(it->first);
			msg.serial(uid, it->second.m_PosX, it->second.m_PosY, it->second.m_PosZ, it->second.m_Direction);
		}
	}
	CUnifiedNetwork::getInstance()->send(sid, msg);
}

void	cb_M_CS_EAINITINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if (pChannelData == NULL)
		return;

	if (pChannelData->m_MainFamilyID != FID)
	{
		sipwarning("Received from nonMainClient, RoomID=%d, FamilyID=%d", inManager->m_RoomID, FID);
		return;
	}
	if (pChannelData->m_bValidEditAnimal)
	{
		sipwarning("Double setting EditAnimal init info, RoomID=%d, FamilyID=%d", inManager->m_RoomID, FID);
		return;
	}

	uint16 groupnum;
	_msgin.serial(groupnum);
	if (groupnum > MAX_EDITANIMALGROUPNUM || groupnum == 0)
	{
		sipwarning("Unlaw EditAnimal group number=%d, FamilyID=%d", groupnum, FID);
		ucstring ucUnlawContent = SIPBASE::_toString(L"Unlaw EditAnimal group number=%d, FamilyID=%d", groupnum);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	pChannelData->m_RoomEditAnimalActionInfo.clear();
	for (uint16 i = 0; i < groupnum; i++)
	{
		uint16	groupid, infolen;
		_msgin.serial(groupid, infolen);

		if (infolen > MAX_EDITANIMALGROUPBASELEN)
		{
			sipwarning("Unlaw EditAnimal group info length groupid=%d, length=%d, FamilyID=%d", groupid, infolen, FID);
			return;
		}

		ROOMEDITANIMALACTIONINFO newgroup;
		newgroup.m_GroupID = groupid;
		newgroup.m_ActionInfo.resize(infolen);
		
		uint8* buf = (uint8*)&(*newgroup.m_ActionInfo.begin());
		_msgin.serialBuffer(buf, infolen);
		pChannelData->m_RoomEditAnimalActionInfo.insert(make_pair(newgroup.m_GroupID, newgroup));
		sipdebug("EditAnimal init groupinfo(RoomID=%d) : Groupid=%d, Length=%d", inManager->m_RoomID, groupid, infolen);
	}

	uint16 aninum;
	_msgin.serial(aninum);
	if (aninum > MAX_EDITANIMALNUM || aninum == 0)
	{
		sipwarning("Unlaw EditAnimal animal number=%d, FamilyID=%d", aninum, FID);
		ucstring ucUnlawContent = SIPBASE::_toString(L"Unlaw EditAnimal animal number=%d", aninum);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	pChannelData->m_RoomEditAnimal.clear();
	for (uint16 j = 0; j < aninum; j++)
	{
		uint16	aniid;
		T_POSX	posx;
		T_POSY	posy;
		T_POSZ	posz;
		T_CH_DIRECTION		dir;
		_msgin.serial(aniid, posx, posy, posz, dir);

		ROOMANIMALBASE newani;
		newani.m_ID = aniid;
		newani.m_PosX = posx;	newani.m_PosY = posy; newani.m_PosZ = posz; newani.m_Direction = dir;
		pChannelData->m_RoomEditAnimal.insert(make_pair(newani.m_ID, newani));
		//sipdebug("EditAnimal init position(RoomID=%d) : AnimalID=%d, X=%f Y=%f, Z=%f, Dir=%f", inManager->m_RoomID, newani.m_ID, newani.m_PosX, newani.m_PosY, newani.m_PosZ, newani.m_Direction);byy krs
	}
	pChannelData->m_bValidEditAnimal = true;
	
	T_MSTROOM_ID SceneID = inManager->m_SceneID;
	MAP_OUT_ROOMKIND::iterator itroomkind = map_OUT_ROOMKIND.find(SceneID);
	if (itroomkind != map_OUT_ROOMKIND.end())
	{
		OUT_ROOMKIND& outinfo = itroomkind->second;
		if (pChannelData->getChannelType() != 1) // by krs
		{
			if (!outinfo.EditAnimalIniFlag_PC)
			{
				outinfo.RoomEditAnimalDefault_PC = pChannelData->m_RoomEditAnimal;
				outinfo.RoomEditAnimalActionInfo_PC = pChannelData->m_RoomEditAnimalActionInfo;
				outinfo.EditAnimalIniFlag_PC = true;
				//sipdebug("Set scene EditAnimal in PC, SceneID=%d : FamilyID=%d", SceneID, FID); byy krs
			}
		}
		else
		{
			if (!outinfo.EditAnimalIniFlag_Mobile)
			{
				outinfo.RoomEditAnimalDefault_Mobile = pChannelData->m_RoomEditAnimal;
				outinfo.RoomEditAnimalActionInfo_Mobile = pChannelData->m_RoomEditAnimalActionInfo;
				outinfo.EditAnimalIniFlag_Mobile = true;
				//sipdebug("Set scene EditAnimal in Mobile, SceneID=%d : FamilyID=%d", SceneID, FID); byy krs
			}
		}
	}

	TFesFamilyIds::iterator	itFes;
	for(itFes = pChannelData->m_mapFesChannelFamilys.begin(); itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
	{
		if(itFes->second.size() == 0)
			continue;
		bool bIsFamily = false;
		for(MAP_FAMILYID::iterator itF = itFes->second.begin(); itF != itFes->second.end(); itF ++)
		{
			T_FAMILYID fid = itF->first;
			if (fid == FID)
				continue;
			pChannelData->SendEditAnimalInfo(fid, _tsid);
			//sipdebug("Send editanimal init position for early enterd family=%d", fid); byy krs
		}
	}

}

void	cb_M_CS_EAMOVE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	CMessage	msgOut;
	T_FAMILYID		FID;
	_msgin.copyWithoutPrefix(FID, msgOut);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if (pChannelData == NULL)
		return;

	if (pChannelData->m_MainFamilyID != FID)
	{
		sipwarning("Received from nonMainClient, RoomID=%d, FamilyID=%d", inManager->m_RoomID, FID);
		return;
	}

	T_MSTROOM_ID SceneID = inManager->m_SceneID;

	while (_msgin.getPos() < static_cast<sint32>(_msgin.length()))
	{
		uint8	actionType;
		_msgin.serial(actionType);
		switch (actionType)
		{
		case	EA_MOVE:
			{
				uint16	aniid;
				T_POSX	posx;
				T_POSY	posy;
				T_POSZ	posz;
				T_CH_DIRECTION		dir;
				uint8	movetype;
				_msgin.serial(aniid, posx, posy, posz, dir, movetype);

				MAP_ROOMEDITANIMAL::iterator it = pChannelData->m_RoomEditAnimal.find(aniid);
				if (it == pChannelData->m_RoomEditAnimal.end())
				{
					sipwarning("No animal id, RoomID=%d, SceneID=%d, AnimalID=%d, FamilyID=%d", inManager->m_RoomID, SceneID, aniid, FID);
					ucstring ucUnlawContent = SIPBASE::_toString(L"No animal id moving");
					RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
					return;
				}
				ROOMANIMALBASE& anim = it->second;
				anim.m_PosX = posx;	anim.m_PosY = posy;	anim.m_PosZ = posz;	anim.m_Direction = dir;
			}
			break;
		case	EA_ANIMATION:
			{
				uint16	aniid;		
				string	actionname;
				uint8	loopflag;
				_msgin.serial(aniid, actionname, loopflag);
			}
			break;
		default:
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"Unlaw EditAnimal action type=%d", actionType);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
		}
	}
	
/*
	uint16 aninum;
	_msgin.serial(aninum);
	if (aninum > MAX_EDITANIMALNUM || aninum == 0)
	{
		sipwarning("Unlaw EditAnimal animal number=%d, FamilyID=%d", aninum, FID);
		ucstring ucUnlawContent = SIPBASE::_toString(L"Unlaw EditAnimal animal number=%d", aninum);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	for (uint16 j = 0; j < aninum; j++)
	{
		uint16	aniid;
		T_POSX	posx;
		T_POSY	posy;
		T_POSZ	posz;
		T_CH_DIRECTION		dir;
		uint8	movetype;
		_msgin.serial(aniid, posx, posy, posz, dir, movetype);

		MAP_ROOMEDITANIMAL::iterator it = pChannelData->m_RoomEditAnimal.find(aniid);
		if (it == pChannelData->m_RoomEditAnimal.end())
		{
			sipwarning("No animal id, RoomID=%d, SceneID=%d, AnimalID=%d, FamilyID=%d", inManager->m_RoomID, SceneID, aniid, FID);
			ucstring ucUnlawContent = SIPBASE::_toString(L"No animal id moving");
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		ROOMANIMALBASE& group = it->second;
		group.m_PosX = posx;	group.m_PosY = posy;	group.m_PosZ = posz;	group.m_Direction = dir;
	}
*/
	TFesFamilyIds::iterator	itFes;
	for(itFes = pChannelData->m_mapFesChannelFamilys.begin(); itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
	{
		if(itFes->second.size() == 0)
			continue;
		bool bIsFamily = false;
		CMessage	msgOut0(M_SC_EAMOVE);
		for(MAP_FAMILYID::iterator itF = itFes->second.begin(); itF != itFes->second.end(); itF ++)
		{
			T_FAMILYID fid = itF->first;
			if (fid == FID)
				continue;
			msgOut0.serial(fid);
			bIsFamily = true;
		}
		if (!bIsFamily)
			continue;
		uint32 uZero = 0;
		msgOut0.serial(uZero);

		msgOut.appendBufferTo(msgOut0);
		CUnifiedNetwork::getInstance()->send(itFes->first, msgOut0);
	}

}

//void	cb_M_CS_EAANIMATION(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	CMessage		msgOut;
//	T_FAMILYID		FID;
//	_msgin.copyWithoutPrefix(FID, msgOut);
//
//	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
//	if (inManager == NULL)
//		return;
//
//	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
//	if (pChannelData == NULL)
//		return;
//
//	if (pChannelData->m_MainFamilyID != FID)
//	{
//		sipwarning("Received from nonMainClient, RoomID=%d, FamilyID=%d", inManager->m_RoomID, FID);
//		return;
//	}
//
//	TFesFamilyIds::iterator	itFes;
//	for(itFes = pChannelData->m_mapFesChannelFamilys.begin(); itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
//	{
//		if(itFes->second.size() == 0)
//			continue;
//		bool bIsFamily = false;
//		CMessage	msgOut0(M_SC_EAANIMATION);
//		for(MAP_FAMILYID::iterator itF = itFes->second.begin(); itF != itFes->second.end(); itF ++)
//		{
//			T_FAMILYID fid = itF->first;
//			if (fid == FID)
//				continue;
//			msgOut0.serial(fid);
//			bIsFamily = true;
//		}
//		if (!bIsFamily)
//			continue;
//		uint32 uZero = 0;
//		msgOut0.serial(uZero);
//
//		msgOut.appendBufferTo(msgOut0);
//		CUnifiedNetwork::getInstance()->send(itFes->first, msgOut0);
//	}
//}

void	cb_M_CS_EABASEINFO(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	CMessage		msgOut;
	T_FAMILYID		FID;
	_msgin.copyWithoutPrefix(FID, msgOut);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if (pChannelData == NULL)
		return;

	if (pChannelData->m_MainFamilyID != FID)
	{
		sipwarning("Received from nonMainClient, RoomID=%d, FamilyID=%d", inManager->m_RoomID, FID);
		return;
	}

	T_MSTROOM_ID SceneID = inManager->m_SceneID;
	uint16 groupnum;
	_msgin.serial(groupnum);
	if (groupnum > MAX_EDITANIMALGROUPNUM || groupnum == 0)
	{
		sipwarning("Unlaw EditAnimal group number=%d, FamilyID=%d", groupnum, FID);
		ucstring ucUnlawContent = SIPBASE::_toString(L"Unlaw EditAnimal group number=%d", groupnum);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	for (uint16 i = 0; i < groupnum; i++)
	{
		uint16	groupid, infolen;
		_msgin.serial(groupid, infolen);

		if (infolen > MAX_EDITANIMALGROUPBASELEN || infolen == 0)
		{
			sipwarning("Unlaw EditAnimal group info length groupid=%d, length=%d, FamilyID=%d", groupid, infolen, FID);
			return;
		}
		
		MAP_ROOMEDITANIMALACTIONINFO::iterator it = pChannelData->m_RoomEditAnimalActionInfo.find(groupid);
		if (it == pChannelData->m_RoomEditAnimalActionInfo.end())
		{
			sipwarning("No groupid, RoomID=%d, SceneID=%d, GroupID=%d, FamilyID=%d", inManager->m_RoomID, SceneID, groupid, FID);
			ucstring ucUnlawContent = SIPBASE::_toString(L"No animal group id setting");
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		ROOMEDITANIMALACTIONINFO& group = it->second;
		if (group.m_ActionInfo.size() != infolen)
		{
			group.m_ActionInfo.resize(infolen);
		}
		uint8* buf = (uint8*)&(*group.m_ActionInfo.begin());
		_msgin.serialBuffer(buf, infolen);
	}

	// send other client
	TFesFamilyIds::iterator	itFes;
	for(itFes = pChannelData->m_mapFesChannelFamilys.begin(); itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
	{
		if(itFes->second.size() == 0)
			continue;
		bool bIsFamily = false;
		CMessage	msgOut0(M_SC_EABASEINFO);
		for(MAP_FAMILYID::iterator itF = itFes->second.begin(); itF != itFes->second.end(); itF ++)
		{
			T_FAMILYID fid = itF->first;
			if (fid == FID)
				continue;
			msgOut0.serial(fid);
			bIsFamily = true;
		}
		if (!bIsFamily)
			continue;
		uint32 uZero = 0;
		msgOut0.serial(uZero);

		msgOut.appendBufferTo(msgOut0);
		CUnifiedNetwork::getInstance()->send(itFes->first, msgOut0);
	}
}

void	cb_M_CS_EAANISELECT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if (pChannelData == NULL)
		return;

	uint16	aniid;
	T_POSX	posx;
	T_POSY	posy;
	T_POSZ	posz;
	T_CH_DIRECTION		dir;
	_msgin.serial(aniid, posx, posy, posz, dir);

	// set animal pos
	{
		MAP_ROOMEDITANIMAL::iterator it = pChannelData->m_RoomEditAnimal.find(aniid);
		if (it == pChannelData->m_RoomEditAnimal.end())
		{
			sipwarning("No animal id, RoomID=%d, AnimalID=%d, FamilyID=%d", inManager->m_RoomID, aniid, FID);
			ucstring ucUnlawContent = SIPBASE::_toString(L"No animal id selecting");
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		ROOMANIMALBASE& anim = it->second;
		anim.m_PosX = posx;	anim.m_PosY = posy;	anim.m_PosZ = posz;	anim.m_Direction = dir;

	}
	// send other client
	{
		TFesFamilyIds::iterator	itFes;
		for(itFes = pChannelData->m_mapFesChannelFamilys.begin(); itFes != pChannelData->m_mapFesChannelFamilys.end(); itFes ++)
		{
			if(itFes->second.size() == 0)
				continue;
			bool bIsFamily = false;
			CMessage	msgOut0(M_SC_EAANISELECT);
			for(MAP_FAMILYID::iterator itF = itFes->second.begin(); itF != itFes->second.end(); itF ++)
			{
				T_FAMILYID fid = itF->first;
				if (fid == FID)
					continue;
				msgOut0.serial(fid);
				bIsFamily = true;
			}
			if (!bIsFamily)
				continue;
			uint32 uZero = 0;
			msgOut0.serial(uZero);

			msgOut0.serial(FID, aniid, posx, posy, posz, dir);
			CUnifiedNetwork::getInstance()->send(itFes->first, msgOut0);
		}
	}
}

void	cb_M_CS_INVITE_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID, targetFID;
	uint16			fsSid;
	uint8			ContactStatus1, ContactStatus2;

	_msgin.serial(FID, targetFID);
	_msgin.serial(fsSid, ContactStatus1, ContactStatus2);
	TServiceId	clientFsSid(fsSid);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);

	if (inManager == NULL)
		return;

	// 방주인이 아닌 사람도 초대할수 있다.
	//if(FID == inManager->m_MasterID)
	inManager->InviteUser_Req(FID, targetFID, clientFsSid, ContactStatus1, ContactStatus2);
	//else
	//{
	//	// 비법발견
	//	ucstring ucUnlawContent;
	//	ucUnlawContent = SIPBASE::_toString(L"No roommaster invite other user!: FID=%d, RoomID=%d", FID, inManager->m_RoomID);
	//	RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
	//}
}

void	cb_M_CS_INVITE_ANS(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		targetFID, srcFID;
	T_ROOMID		RoomID;
	T_ID			ChannelID;
	uint8			Answer;

	_msgin.serial(targetFID, RoomID, ChannelID, srcFID, Answer);

	CChannelWorld *inManager = GetChannelWorld(RoomID);
	if (inManager != NULL)
	{
		inManager->InviteUser_Ans(targetFID, srcFID, ChannelID, Answer);
	}
	else
	{
		sipwarning("GetChannelWorld = NULL : RoomID=%d", RoomID);
	}
}

void CChannelWorld::RefreshInviteWait()
{
	SIPBASE::TTime curtime = CTime::getLocalTime();
	for(ChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		VEC_INVITELIST::iterator it;
		VEC_INVITELIST&	invite_list = pChannelData->m_InviteList;
		for (it = invite_list.begin(); it != invite_list.end(); )
		{
			// 초청대기목록을 정리한다
			if ((curtime - it->RequestTime) > INVITE_WAIT_TIME)
			{
				it = invite_list.erase(it);
				continue;
			}

			// 이미 logoff하였는데 초청대상목록에 있는 경우 삭제한다.
			if (map_family.find(it->targetFID) == map_family.end())
				it = invite_list.erase(it);
			else
				it ++;
		}
	}
}

// 사용자를 자기 방에 초청하는 처리
void CChannelWorld::InviteUser_Req(T_FAMILYID srcFID, T_FAMILYID targetFID, SIPNET::TServiceId _tsid, uint8 ContactStatus1, uint8 ContactStatus2)
{
	FAMILY* pTarget = GetFamilyData(targetFID);
	if (pTarget == NULL)
	{
		ucstring	FamilyName = L"";
		uint8		Answer = 2;	// NotExist
		CMessage msg(M_SC_INVITE_ANS);
		msg.serial(srcFID, targetFID, FamilyName, Answer);
		CUnifiedNetwork::getInstance()->send(_tsid, msg);
		return;
	}

	// 상대가 나를 흑명단에 등록한 경우 요청을 보낼수 없다.
	//	if (IsBlackList(targetFID, srcFID))
	if (ContactStatus1 == 0xFF)
	{
		uint8	Answer = 4;	// BlackList
		CMessage	msgout(M_SC_INVITE_ANS);
		msgout.serial(srcFID, targetFID, pTarget->m_Name, Answer);
		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
		return;
	}

	if (ProcCheckingRoomRequest(srcFID, targetFID, ContactStatus1, ContactStatus2))
		return;

	T_ID		ChannelID = GetFamilyChannelID(srcFID);
	ChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, srcFID);
		return;
	}

	MAP_FAMILY::iterator	itSrc = map_family.find(srcFID);
	if (itSrc == map_family.end())
	{
		sipwarning("Can't find srcFID in map_family. srcFID=%d", srcFID);
		return;
	}

	// 초청대기목록 정리
	RefreshInviteWait();

	// targetFID 검사: 현재 로그인상태여야 하며, 현재의 방, 현재의 선로에 없어야 한다.
	TRoomFamilys::iterator	itInRoom = m_mapFamilys.find(targetFID);
	if ((itInRoom != m_mapFamilys.end()) && (itInRoom->second.m_nChannelID == ChannelID))
	{
		ucstring	FamilyName = L"";
		uint8		Answer = 3;	// InCurrentRoom
		CMessage msg(M_SC_INVITE_ANS);
		msg.serial(srcFID, targetFID, FamilyName, Answer);
		CUnifiedNetwork::getInstance()->send(_tsid, msg);
		return;
	}

	// 이미전의 초청이 있는지 검사
	VEC_INVITELIST	&invite_list = pChannelData->m_InviteList;
	for (VEC_INVITELIST::iterator it = invite_list.begin(); it != invite_list.end(); it ++)
	{
		if (it->srcFID == srcFID && it->targetFID == targetFID)
		{
			sipdebug("Already Invited. srcFID=%d, tergetFID=%d", srcFID, targetFID);
			return;
		}
	}

	// 초청대기목록에 추가
	_Invite_Data	data(srcFID, targetFID);
	data.RequestTime = CTime::getLocalTime();
	invite_list.push_back(data);

	// 초청Packet 보내기
	CMessage msg(M_SC_INVITE_REQ);
	msg.serial(targetFID, srcFID, itSrc->second.m_Name, m_RoomID, ChannelID, m_RoomName, m_SceneID);
	CUnifiedNetwork::getInstance()->send(pTarget->m_FES, msg);
}

// 대방의 사용자초청에 대한 응답처리
void CChannelWorld::InviteUser_Ans(T_FAMILYID targetFID, T_FAMILYID srcFID, uint32 ChannelID, uint8 Answer)
{
	ChannelData	*pChannelData = GetChannel(ChannelID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		return;
	}

	FAMILY *pF = GetFamilyData(targetFID);
	if(pF == NULL)
	{
		sipwarning("GetFamilyData=NULL : FamilyID=%d", targetFID);
		return;
	}

	// 초청대기목록에서 검사
	VEC_INVITELIST& invite_list = pChannelData->m_InviteList;
	VEC_INVITELIST::iterator it;
	for(it = invite_list.begin(); it != invite_list.end(); it ++)
	{
		if(it->srcFID == srcFID && it->targetFID == targetFID)
			break;
	}
	if (it == invite_list.end())
	{
		sipwarning("targetFID not found in map_InviteWait: targetFID=%d", targetFID);
		// 비법발견
		//		ucstring ucUnlawContent;
		//		ucUnlawContent = SIPBASE::_toString(L"Not Invited User!: FID=%d, RoomID=%d", targetFID, m_RoomID);
		//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, targetFID);
		return;
	}
	invite_list.erase(it);

	// 초청을 수락했을때 처리
	if(Answer == 1)
	{
		RefreshEnterWait();

		bool bIsOtherPlace = true;
		if (pChannelData->IsChannelFamily(targetFID))
			bIsOtherPlace = false;
		if (bIsOtherPlace)
		{
			// 진입대기목록갱신
			MAP_ENTERROOMWAIT::iterator it = map_EnterWait.find(targetFID);
			if (it != map_EnterWait.end())
			{
				it->second.RequestTime = CTime::getLocalTime();
			}
			else
			{
				ENTERROOMWAIT	newenter;
				newenter.FID = targetFID;		newenter.RequestTime = CTime::getLocalTime();
				map_EnterWait.insert( make_pair(targetFID, newenter) ); 
			}
		}
	}

	// 응답Packet 보내기
	FAMILY *pSrcF = GetFamilyData(srcFID);
	if(pSrcF == NULL)
	{
		// 초청자가 없는 경우에 례외처리를 하지 못한다.
		//		왜? 응답자가 응답하기 전에 초청자가 탈퇴할수 있으므로.
		// 이를 리용해서 해커가 임의로 방에 들어올수 있는 가능성이 생긴다.
		sipdebug("FamilyID not found : FamilyID=%d", srcFID);
	}
	else
	{
		CMessage msg(M_SC_INVITE_ANS);
		msg.serial(srcFID, targetFID, pF->m_Name, Answer);
		CUnifiedNetwork::getInstance()->send(pSrcF->m_FES, msg);
	}
}


//////////////////////////////////////////////////////////
//  등불기능
void SendLanternList(uint32 FID, SIPNET::TServiceId _tsid)
{
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if(inManager == NULL)
	{
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
		return;
	}

	VEC_LANTERNLIST&	list = pChannelData->m_LanternList;
	SIPBASE::TTime	curTime = CTime::getLocalTime();

	uint32	zero = 0;
	for(VEC_LANTERNLIST::iterator it = list.begin(); it != list.end();)
	{
		uint32	time = (uint32)((curTime - it->CreateTime) / 1000);
		if (time >= LANTERN_FLY_TIME)
		{
			it = list.erase(it);
			continue;
		}

		CMessage	msgOut(M_SC_LANTERN);
		msgOut.serial(FID, zero, it->X, it->Y, it->Z, it->Dir, it->ItemID, time, zero);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

		it ++;
	}
}

// Add new lantern ([d:SyncCode][w:InvenPos][d:X][d:Y][d:Z][d:Dir])
void cb_M_CS_LANTERN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint32		SyncCode;	_msgin.serial(SyncCode);
	uint16		InvenPos;	_msgin.serial(InvenPos);
	uint32		X, Y, Z, Dir;	_msgin.serial(X, Y, Z, Dir);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if(inManager == NULL)
	{
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->LanternCreate(FID, InvenPos, SyncCode, X, Y, Z, Dir, _tsid);
}

void CChannelWorld::LanternCreate(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, uint32 X, uint32 Y, uint32 Z, uint32 Dir, SIPNET::TServiceId _sid)
{
	uint32	ItemID = FamilyItemUsed(FID, InvenPos, SyncCode, ITEMTYPE_OTHER, _sid);
	if (ItemID == 0)
	{
		return;
	}
	// 경험값추가
	ChangeFamilyExp(Family_Exp_Type_LanternMaster, FID);

	AddNewLantern(FID, X, Y, Z, Dir, ItemID);
}

void CChannelWorld::AddNewLantern(T_FAMILYID FID, uint32 X, uint32 Y, uint32 Z, uint32 Dir, uint32 ItemID, bool bPacketSend)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}
	VEC_LANTERNLIST&	list = pChannelData->m_LanternList;

	// 새로 추가
	_Lantern_Data	data(X, Y, Z, Dir, ItemID);
	data.CreateTime = CTime::getLocalTime();
	list.push_back(data);

	if (bPacketSend)
	{
		// 사용자들에게 통지
		uint32	zero = 0;
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LANTERN)
			msgOut.serial(X, Y, Z, Dir, ItemID, zero, FID);
		FOR_END_FOR_FES_ALL()
	}

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo)
	{
		pFamilyInfo->DonActInRoom(ROOMACT_KONGMINGDENG);
	}

	// Log
	//	DBLOG_ItemUsed(DBLSUB_USEITEM_FISHFOOD, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

// Use Flutes Item ([d:SyncCode][w:InvenPos])
void cb_M_CS_ITEM_MUSICITEM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint32		SyncCode;	_msgin.serial(SyncCode);
	uint16		InvenPos;	_msgin.serial(InvenPos);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if(inManager == NULL)
	{
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->UseMusicTool(FID, InvenPos, SyncCode, _tsid);
}

void CChannelWorld::UseMusicTool(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	T_ITEMSUBTYPEID ItemID = FamilyItemUsed(FID, InvenPos, SyncCode, ITEMTYPE_OTHER, _sid);
	if (ItemID == 0)
		return;

	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo)
	{
		pFamilyInfo->DonActInRoom(ROOMACT_ITEMUSED);
	}
	// 경험값증가
	ChangeFamilyExp(Family_Exp_Type_UseMusicTool, FID);
}

// Use Rain Item ([d:SyncCode][w:InvenPos])
void cb_M_CS_ITEM_RAIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint32		SyncCode;	_msgin.serial(SyncCode);
	uint16		InvenPos;	_msgin.serial(InvenPos);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if(inManager == NULL)
	{
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->UseRainItem(FID, InvenPos, SyncCode, _tsid);
}

void CChannelWorld::UseRainItem(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	T_ITEMSUBTYPEID ItemID = FamilyItemUsed(FID, InvenPos, SyncCode, ITEMTYPE_OTHER, _sid);
	if (ItemID == 0)
		return;

	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	// 사용자들에게 통지
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ITEM_RAIN)
		msgOut.serial(FID);
	FOR_END_FOR_FES_ALL()

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo)
	{
		pFamilyInfo->DonActInRoom(ROOMACT_ITEMUSED);
	}

	// Log
	//	DBLOG_ItemUsed(DBLSUB_USEITEM_FISHFOOD, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

// Use PointCard Item ([d:SyncCode][w:InvenPos])
void cb_M_CS_ITEM_POINTCARD(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	uint32		SyncCode;	_msgin.serial(SyncCode);
	uint16		InvenPos;	_msgin.serial(InvenPos);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if(inManager == NULL)
	{
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	inManager->UsePointCardItem(FID, InvenPos, SyncCode, _tsid);
}

void CChannelWorld::UsePointCardItem(T_FAMILYID FID, T_FITEMPOS InvenPos, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	T_ID	ItemID = 0;
	const	MST_ITEM	*mstItem = NULL;
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
	if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_OTHER))
	{
		sipwarning(L"Invalid Item. FID=%d, ItemID=%d", FID, ItemID);
		return;
	}

	// Check OK
	OUT_POINTCARD	*pPointCard = GetPointCardData();
	if (pPointCard == NULL)
	{
		sipwarning("GetPointCardData = NULL.");
		return;
	}

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

	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
	}

	ExpendMoney(FID, 0, pPointCard->AddPoint, 2, DBLOG_MAINTYPE_CHFGMoney, 0, 0, 0);

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if ( pFamilyInfo == NULL )
	{
		sipdebug("GetFamilyInfo = NULL. FID=%d", FID);
	}
	else
	{
		SendFamilyProperty_Money(_sid, FID, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);
	}

	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		return;
	}

	// 사용자들에게 통지
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ITEM_POINTCARD)
		msgOut.serial(FID, pPointCard->CardID);
	FOR_END_FOR_FES_ALL()

	// Log
	//	DBLOG_ItemUsed(DBLSUB_USEITEM_FISHFOOD, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

// Give food to fish ([d:StoreLockID][d:SyncCode][w:InvenPos][d:FishScopeID])
void	cb_M_CS_FISH_FOOD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	uint32			StoreLockID;	_msgin.serial(StoreLockID);
	uint32			SyncCode;		_msgin.serial(SyncCode);
	T_FITEMPOS		InvenPos;		_msgin.serial(InvenPos);
	T_ITEMSUBTYPEID ItemID;			_msgin.serial(ItemID);
	uint32			FishScopeID;	_msgin.serial(FishScopeID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_FISH_FOOD(FID, StoreLockID, InvenPos, ItemID, FishScopeID, SyncCode, _tsid);
}

// Set New Flower ([d:StoreLockID][d:SyncCode][w:InvenPos])
void	cb_M_CS_FLOWER_NEW(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	uint32			StoreLockID;	_msgin.serial(StoreLockID);
	uint32			SyncCode;		_msgin.serial(SyncCode);
	T_FITEMPOS		InvenPos;		_msgin.serial(InvenPos);
	T_ITEMSUBTYPEID ItemID;			_msgin.serial(ItemID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_FLOWER_NEW(FID, StoreLockID, InvenPos, ItemID, SyncCode, _tsid);
}

// Give water to flower
void	cb_M_CS_FLOWER_WATER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_FLOWER_WATER(FID, _tsid);
}

// Flower Action Finished
void	cb_M_CS_FLOWER_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_FLOWER_END(FID, _tsid);
}

// Request New Flower
void	cb_M_CS_FLOWER_NEW_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	uint32			CallKey;		_msgin.serial(CallKey);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_FLOWER_NEW_REQ(FID, CallKey, _tsid);
}

// Request Give water to flower
void	cb_M_CS_FLOWER_WATER_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	uint32			CallKey;	_msgin.serial(CallKey);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_FLOWER_WATER_REQ(FID, CallKey, _tsid);
}

void	cb_M_CS_FLOWER_WATERTIME_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_FLOWER_WATERTIME_REQ(FID, _tsid);
}

// Music start ([d:MusicPosID])
void cb_M_CS_MUSIC_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	uint32			MusicPosID;		_msgin.serial(MusicPosID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_MUSIC_START(FID, MusicPosID);
}

void CChannelWorld::on_M_CS_MUSIC_START(T_FAMILYID FID, uint32 MusicPosID)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	for (std::map<T_FAMILYID, uint32>::iterator it = pChannelData->m_CurMusicFIDs.begin(); it != pChannelData->m_CurMusicFIDs.begin(); it ++)
	{
		if (it->first == FID)
		{
			sipwarning("it->first == FID. FID=%d, PrevMusicPos=%d, NewMusicPos=%d", FID, it->second, MusicPosID);
			return;
		}
		if (it->second == MusicPosID)
		{
			sipwarning("it->second == MusicPosID. FID=%d, MusicPos=%d, PrevFID=%d", FID, MusicPosID, it->first);
			return;
		}
	}

	pChannelData->m_CurMusicFIDs[FID] = MusicPosID;

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MUSIC_START)
		msgOut.serial(FID, MusicPosID);
	FOR_END_FOR_FES_ALL()

	INFO_FAMILYINROOM	*pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo)
		pFamilyInfo->DonActInRoom(ROOMACT_MUSIC);

	// 경험값증가
	ChangeFamilyExp(Family_Exp_Type_MusicStart, FID);
}

// Music end
void cb_M_CS_MUSIC_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_MUSIC_END(FID);
}

void CChannelWorld::on_M_CS_MUSIC_END(T_FAMILYID FID)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	std::map<T_FAMILYID, uint32>::iterator itMusic = pChannelData->m_CurMusicFIDs.find(FID);
	if (itMusic == pChannelData->m_CurMusicFIDs.end())
	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("m_CurMusicFID != FID. FID=%d, RoomID=%d", FID, m_RoomID).c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		sipinfo("m_CurMusicFID != FID. FID=%d, RoomID=%d", FID, m_RoomID);
		return;
	}

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MUSIC_END)
		msgOut.serial(FID, itMusic->second);
	FOR_END_FOR_FES_ALL()

	pChannelData->m_CurMusicFIDs.erase(itMusic);
}

// Drum start ([d:DrumID])
void cb_M_CS_DRUM_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_ID			DrumID;		_msgin.serial(DrumID);
	T_COMMON_NAME	NPCName;	_msgin.serial(NPCName);
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_DRUM_START(FID, DrumID, NPCName);
}

void CChannelWorld::on_M_CS_DRUM_START(T_FAMILYID FID, uint32 DrumID, T_COMMON_NAME NPCName)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}
	_Drum_Data* pCurDrumData = pChannelData->m_CurDrumData;

	int n = -1;
	for (int i = 0; i < MAX_DRUM_COUNT; i ++)
	{
		// 해당 드람이 쳐지고 있는 상태라면
		if (pCurDrumData[i].DrumID == DrumID)
		{
			if (pCurDrumData[i].FID != FID)
				sipdebug("Can't drum, because there is already doing. RoomID=%d, FID=%d, DrumID=%d, DrumFID=%d", m_RoomID, FID, DrumID, pCurDrumData[i].FID);
			return;
		}
		if (MIN_RELIGIONROOMID <= m_RoomID && m_RoomID <= MAX_RELIGIONROOMID)
		{
			// 일반방에서는 한사람이 여러개의 드람을 칠수 있다.
			// 공동방에서 이미 자신이 드람을 치고 있다면
			if (pCurDrumData[i].FID == FID)
			{
				sipwarning("pCurDrumData[i].FID == FID. RoomID=%d, FID=%d, PrevDrumID=%d, NewDrumID=%d", m_RoomID, FID, pCurDrumData[i].DrumID, DrumID);
				return;
			}
		}

		if (pCurDrumData[i].DrumID == 0)
		{
			n = i;
		}
	}

	if (n < 0)
	{
		sipwarning("n < 0. RoomID=%d, FID=%d, ReqDrumID=%d", m_RoomID, FID, DrumID);
		return;
	}
	if (IsCommonRoom(m_RoomID))
	{
		TTime tmCur = CTime::getLocalTime();
		if (tmCur - pCurDrumData[n].LastTime < BELL_DRUM_AFTEREND_EFFECT_TIME*1000)
		{
			sipdebug("DrumTime is incorrect. RoomID=%d, FID=%d, ReqDrumID=%d", m_RoomID, FID, DrumID);
			return;
		}
	}

	pCurDrumData[n].FID = FID;
	pCurDrumData[n].DrumID = DrumID;
//	pCurDrumData[n].NPCName = NPCName;

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_DRUM_START)
		msgOut.serial(DrumID, FID, NPCName);
	FOR_END_FOR_FES_ALL()
}

// Drum end
void cb_M_CS_DRUM_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_ID			DrumID;		_msgin.serial(DrumID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_DRUM_END(FID, DrumID);
}

void CChannelWorld::on_M_CS_DRUM_END(T_FAMILYID FID, uint32 DrumID)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	for (int i = 0; i < MAX_DRUM_COUNT; i ++)
	{
		if (pChannelData->m_CurDrumData[i].DrumID == DrumID)
		{
			if (pChannelData->m_CurDrumData[i].FID == FID)
			{
				uint32	DrumID = pChannelData->m_CurDrumData[i].DrumID;

				pChannelData->m_CurDrumData[i].Reset();
				pChannelData->m_CurDrumData[i].LastTime = CTime::getLocalTime();

				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_DRUM_END)
					msgOut.serial(DrumID, FID);
				FOR_END_FOR_FES_ALL()

				return;
			}
			//sipwarning("pChannelData->m_CurDrumData[i].FID != FID. RoomID=%d, FID=%d, DrumID=%d, pChannelData->m_CurDrumData[i].FID=%d",
			//	m_RoomID, FID, DrumID, pChannelData->m_CurDrumData[i].FID); byy krs
			break;
		}
	}
}

// ShenShu start 
void cb_M_CS_SHENSHU_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);	
	T_COMMON_NAME	NPCName;	_msgin.serial(NPCName);
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_SHENSHU_START(FID, NPCName);
}

void CChannelWorld::on_M_CS_SHENSHU_START(T_FAMILYID FID, T_COMMON_NAME NPCName)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}
		
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_SHENSHU_START)
		msgOut.serial(FID, NPCName);
	FOR_END_FOR_FES_ALL()
}

// Bell start
void cb_M_CS_BELL_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_BELL_START(FID);
}

void CChannelWorld::on_M_CS_BELL_START(T_FAMILYID FID)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	if (pChannelData->m_CurBellFID != 0)
	{
		// 비법파케트발견
		//ucstring ucUnlawContent = SIPBASE::_toString("m_CurBellFID != 0. FID=%d, RoomID=%d, m_CurBellFID=%d", FID, m_RoomID, pChannelData->m_CurBellFID).c_str();
		//RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	if (IsCommonRoom(m_RoomID))
	{
		if (CTime::getLocalTime() - pChannelData->m_BellLastTime < BELL_DRUM_AFTEREND_EFFECT_TIME*1000)
		{
			return;
		}
	}

	pChannelData->m_CurBellFID = FID;

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_BELL_START)
		msgOut.serial(FID);
	FOR_END_FOR_FES_ALL()
}

// Bell end
void cb_M_CS_BELL_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_BELL_END(FID);
}

void CChannelWorld::on_M_CS_BELL_END(T_FAMILYID FID)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	if (pChannelData->m_CurBellFID != FID)
	{
		// 비법파케트발견
		ucstring ucUnlawContent = SIPBASE::_toString("m_CurBellFID != FID. FID=%d, RoomID=%d, m_CurBellFID=%d", FID, m_RoomID, pChannelData->m_CurBellFID).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	pChannelData->m_CurBellFID = 0;
	pChannelData->m_BellLastTime = CTime::getLocalTime();

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_BELL_END)
	FOR_END_FOR_FES_ALL()
}

// notice that family approach to an animal(squirrel) ([d:AnimalId])
void	cb_M_CS_ANIMAL_APPROACH(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				AnimalID;			_msgin.serial(AnimalID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("inManager == NULL. FID=%d, AnimalID=%d", FID, AnimalID);
		return;			// 비법발견
	}

	inManager->AnimalApproached(FID, AnimalID);
}

void CChannelWorld::AnimalApproached(T_FAMILYID FID, uint32 AnimalID)
{
	TTime	curtime = SIPBASE::CTime::getLocalTime();

	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	if ( pChannelData->m_RoomAnimal.empty() )
		return;

	// 동물 검사
	MAP_ROOMANIMAL::iterator	itAnimal = pChannelData->m_RoomAnimal.find(AnimalID);
	if(itAnimal == pChannelData->m_RoomAnimal.end())
	{
		sipwarning("Can't find Animal. AnimalID=%d", AnimalID);
		return;
	}

	ROOMANIMAL *CurAnimal = &(itAnimal->second);
	if(CurAnimal->m_ServerControlType != SCT_ESCAPE)
	{ // 도망치는 동물이 아니다.
		//sipdebug("Invalid ServerControlType. AnimalID=%d, ServerControlType=%d", AnimalID, CurAnimal->m_ServerControlType); byy krs
		return;
	}
	if (CurAnimal->m_SelFID != 0)
	{ // 동물이 교감중이다.
		//sipdebug("Animal Selected. AnimalID=%d", AnimalID); byy krs
		return;
	}
	// 동물이 서있는 상태인지를 검사해야 하는데 이것이 힘들다???
	//if (CurAnimal->m_NextMoveTime.timevalue > curtime)
	//{
	//	sipdebug("Animal Moving. AnimalID=%d", AnimalID);
	//	return;
	//}

	// 사람과의 거리 검사
	INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
	if(pInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	T_CH_DIRECTION	familyDir;
	T_CH_X			familyX, familyY, familyZ;
	if(!pInfo->GetFirstCHState(familyDir, familyX, familyY, familyZ))
	{
		sipwarning("GetFirstCHState fail. FID=%d", FID);
		return;
	}

	if(CalcDistance2(familyX, familyY, familyZ, CurAnimal->m_PosX, CurAnimal->m_PosY, CurAnimal->m_PosZ) 
		> 4 * ANIMAL_ESCAPE_DISTANCE * ANIMAL_ESCAPE_DISTANCE) // 충분한 거리 검사
	{
		//sipdebug("Distance of Animal & Family too far. FID=%d, RoomID=%d, AnimalID=%d, Family:%f,%f,%f, Animal:%f,%f,%f [WRN_7]", FID, m_RoomID, AnimalID, familyX, familyY, familyZ, CurAnimal->m_PosX, CurAnimal->m_PosY, CurAnimal->m_PosZ); byy krs
		return;
	}

	OUT_ANIMALPARAM			*AnimalParam = GetOutAnimalParam(CurAnimal->m_TypeID);
	if (AnimalParam == NULL)
	{
		sipdebug("GetOutAnimalParam = NULL. AnimalID=%d, m_TypeID=%d", AnimalID, CurAnimal->m_TypeID);
		return;
	}
	sipassert(AnimalParam->MoveParam > 0);

	// 다음 이동할 위치를 찾는다.
	T_POSX	NextX = familyX;
	T_POSY	NextY = familyY;
	T_POSZ	NextZ = familyZ;

	MAP_OUT_ROOMKIND::iterator itroomkind = map_OUT_ROOMKIND.find(m_SceneID);
	if (itroomkind == map_OUT_ROOMKIND.end())
	{
		sipwarning("Room kind info = NULL. SceneID=%d", m_SceneID);
		return;
	}
	const REGIONSCOPE* scope = itroomkind->second.GetRegionScope(CurAnimal->m_RegionID, pChannelData->getChannelType()); // by krs
	if (scope == NULL)
	{
		sipwarning("GetRegionScope of Room kind info = NULL. SceneID=%d, RegionID=%d", m_SceneID, CurAnimal->m_RegionID);
		return;
	}
	if ( ! scope->GetMoveablePos(CurAnimal->m_PosX, CurAnimal->m_PosY, CurAnimal->m_PosZ, AnimalParam->MaxMoveDis,
		&NextX, &NextY, &NextZ) )
	{
		sipdebug("GetMoveablePos failed.");
		return;
	}

	int	interval = AnimalParam->MoveParam * 1000;
	CurAnimal->m_NextMoveTime.timevalue = (TTime)(curtime + interval + Irand(0, (int)(interval * ANIMAL_MOVE_DELTATIME)));

	T_POSZ	deltaZ = (NextZ - CurAnimal->m_PosZ) * (-1.0f);
	T_POSX	deltaX = (NextX - CurAnimal->m_PosX) * (-1.0f);
	if (deltaZ == 0)
		CurAnimal->m_Direction = 0.0f;
	else
		CurAnimal->m_Direction = atan2(deltaX, deltaZ);
	CurAnimal->m_PosX = NextX;
	CurAnimal->m_PosY = NextY;
	CurAnimal->m_PosZ = NextZ;

	uint8	AnimalMoveType = 0, bIsEscape = 1;
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ANIMALMOVE)
		msgOut.serial(AnimalID, CurAnimal->m_PosX, CurAnimal->m_PosY, CurAnimal->m_PosZ, AnimalMoveType, bIsEscape);
		AnimalMoveType ++;
	FOR_END_FOR_FES_ALL();
}

// Stop Animal ([d:Animal ID][d:X][d:Y][d:Z])
void	cb_M_CS_ANIMAL_STOP(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				AnimalID;			_msgin.serial(AnimalID);
	float				X;					_msgin.serial(X);
	float				Y;					_msgin.serial(Y);
	float				Z;					_msgin.serial(Z);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("inManager == NULL. FID=%d, AnimalID=%d", FID, AnimalID);
		return;			// 비법발견
	}

	inManager->AnimalStop(FID, AnimalID, X, Y, Z);
}

void CChannelWorld::AnimalStop(T_FAMILYID FID, uint32 AnimalID, float X, float Y, float Z)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FamilyID=%d", m_RoomID, FID);
		return;
	}

	MAP_ROOMANIMAL::iterator	itAnimal = pChannelData->m_RoomAnimal.find(AnimalID);
	if(itAnimal == pChannelData->m_RoomAnimal.end())
	{
		sipwarning("Can't find Animal! AnimalID=%d", AnimalID);
		return;
	}
	ROOMANIMAL *CurAnimal = &(itAnimal->second);
	if(CurAnimal->m_SelFID)
	{
		//sipdebug("CurAnimal->m_SelFID = %d", CurAnimal->m_SelFID); byy krs
		return;	// 현재 다른 사용자와 교감중이다.
	}

	CurAnimal->m_PosX = X;
	CurAnimal->m_PosY = Y;
	CurAnimal->m_PosZ = Z;
	uint8	MoveType = 0, bIsEscape = 0;
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ANIMALMOVE)
		msgOut.serial(AnimalID, X, Y, Z, MoveType, bIsEscape);
	FOR_END_FOR_FES_ALL();
}

// Animal consensus start ([d:AnimalID][d:targetX][d:targetY][d:targetZ])
void	cb_M_CS_ANIMAL_SELECT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				AnimalID;			_msgin.serial(AnimalID);
	float				X;					_msgin.serial(X);
	float				Y;					_msgin.serial(Y);
	float				Z;					_msgin.serial(Z);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->AnimalSelect(FID, AnimalID, X, Y, Z, _tsid);
}

void CChannelWorld::AnimalSelect(T_FAMILYID FID, uint32 AnimalID, float X, float Y, float Z, SIPNET::TServiceId _sid)
{
	//sipdebug("AnimalSelect: RoomID=%d, FID=%d, AnimalID=%d", m_RoomID, FID, AnimalID); byy krs

	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FamilyID=%d", m_RoomID, FID);
		return;
	}

	MAP_ROOMANIMAL::iterator	itAnimal = pChannelData->m_RoomAnimal.find(AnimalID);
	if(itAnimal == pChannelData->m_RoomAnimal.end())
	{
		sipwarning("Can't find Animal! AnimalID=%d", AnimalID);
		return;
	}
	ROOMANIMAL *CurAnimal = &(itAnimal->second);
	if(CurAnimal->m_SelFID)
	{
		sipdebug("Now Selected. RoomID=%d, AnimalID=%d, SelFID=%d", m_RoomID, AnimalID, CurAnimal->m_SelFID);
		return;	// 현재 다른 사용자와 교감중이다.
	}

	MAP_OUT_ROOMKIND::iterator itroomkind = map_OUT_ROOMKIND.find(m_SceneID);
	if (itroomkind == map_OUT_ROOMKIND.end())
	{
		sipwarning("Room kind info = NULL. SceneID=%d", m_SceneID);
		return;
	}
	const REGIONSCOPE* scope = itroomkind->second.GetRegionScope(CurAnimal->m_RegionID, pChannelData->getChannelType()); // by krs
	if (scope == NULL)
	{
		sipwarning("GetRegionScope of Room kind info = NULL. SceneID=%d, RegionID=%d", m_SceneID, CurAnimal->m_RegionID);
		return;
	}
	if(!scope->IsInScope(X, Y, Z))
	{
		sipdebug("Out of Region. RoomID=%d, AnimalID=%d, RegionID=%d", m_RoomID, AnimalID, CurAnimal->m_RegionID);
		return;
	}

	// 교감 시작
	CurAnimal->m_PosX = X;
	CurAnimal->m_PosY = Y;
	CurAnimal->m_PosZ = Z;
	CurAnimal->m_SelFID = FID;

	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_ANIMAL_SELECT)
		msgOut.serial(FID, AnimalID);
	FOR_END_FOR_FES_ALL();

	// 경험값증가
	ChangeFamilyExp(Family_Exp_Type_SelectAnimal, FID);

	// FreezingPlayer검사등록
	ucstring	str;
	str = SIPBASE::_toString(L"AnimalSelect: FID=%d, RoomID=%d, AnimalID=%d", FID, m_RoomID, AnimalID);
	pChannelData->RegisterActionPlayer(FID, 5, str);	// 5분
}

void CChannelWorld::AnimalSelectStop(T_FAMILYID FID, uint32 AnimalID)
{
	//sipdebug("AnimalSelectStop: RoomID=%d, FID=%d, AnimalID=%d", m_RoomID, FID, AnimalID); byy krs

	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FamilyID=%d", m_RoomID, FID);
		return;
	}

	ROOMANIMAL *CurAnimal = NULL;
	if(AnimalID > 0)
	{
		MAP_ROOMANIMAL::iterator	it = pChannelData->m_RoomAnimal.find(AnimalID);
		if(it == pChannelData->m_RoomAnimal.end())
		{
			sipwarning("Can't find Animal! AnimalID=%d", AnimalID);
			return;
		}
		CurAnimal = &(it->second);
		if(CurAnimal->m_SelFID != FID)
		{
			if (CurAnimal->m_SelFID != 0)
				sipinfo("CurAnimal->m_SelFID != FID!!! CurAnimal->m_SelFID=%d, FID=%d, RoomID=%d [WRN_2]", CurAnimal->m_SelFID, FID, m_RoomID);
			return;
		}
	}
	else
	{
		for(MAP_ROOMANIMAL::iterator it = pChannelData->m_RoomAnimal.begin(); it != pChannelData->m_RoomAnimal.end(); it ++)
		{
			if(it->second.m_SelFID == FID)
			{
				CurAnimal = &(it->second);
				break;
			}
		}
		if(CurAnimal == NULL)
			return;
	}

	// 교감 끝
	CurAnimal->m_SelFID = 0;

	// 중장
	CheckPrize(FID, PRIZE_TYPE_ANIMAL, 1);	// 1%

	// FreezingPlayer검사해제
	pChannelData->UnregisterActionPlayer(FID);
}

// Animal consensus stop ([d:AnimalID])
void	cb_M_CS_ANIMAL_SELECT_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				AnimalID;			_msgin.serial(AnimalID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->AnimalSelectStop(FID, AnimalID);
}

void	cb_M_CS_MULTILANTERN_REQ(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	float				dX, dY, dZ;			_msgin.serial(dX, dY, dZ);
	T_ITEMSUBTYPEID		ItemID;				_msgin.serial(ItemID);
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_MULTILANTERN_REQ(FID, dX, dY, dZ, ItemID, _tsid);
}

uint32	CandiMultiLanternID = 1;
void	CChannelWorld::on_M_CS_MULTILANTERN_REQ(T_FAMILYID FID, float dX, float dY, float dZ, T_ITEMSUBTYPEID ItemID, SIPNET::TServiceId _tsid)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	uint32	code = ERR_NOERR;

	MAP_MULTILANTERNDATA::iterator it;
	for (it = pChannelData->m_MultiLanternList.begin(); it != pChannelData->m_MultiLanternList.end(); it++)
	{
		_MultiLanternData &md = it->second;
		// 적당한 위치인가 검사
		if (((md.dX-dX)*(md.dX-dX) + (md.dZ-dZ)*(md.dZ-dZ)) < MIN_MULTILANERN_POSITION_DIST_SQ)
		{
			code = E_MULTILANTERN_INVALIDPOS;
			break;
		}
		
		// 반복창조인가 검사
		if (md.MasterID == FID)
		{
			code = E_MULTILANTERN_INVALIDREQ;
			break;
		}
	}

	if (code == ERR_NOERR)
	{
		_MultiLanternData mdata(FID, dX, dY, dZ, ItemID);
		pChannelData->m_MultiLanternList.insert(make_pair(mdata.MultiLanterID, mdata));
		CMessage	msg(M_SC_MULTILANTERN_RES);
		msg.serial(FID, code, mdata.MultiLanterID);
		CUnifiedNetwork::getInstance()->send(_tsid, msg);

		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTILANTERN_NEW)
			msgOut.serial(mdata.MultiLanterID, mdata.dX, mdata.dY, mdata.dZ, mdata.ReqItemID, mdata.MasterID);
		FOR_END_FOR_FES_ALL()

		//sipdebug("New multi lantern: ID=%d, Master=%d, X=%f, Y=%f, Z=%f, ItemID=%d", mdata.MultiLanterID, mdata.MasterID, mdata.dX, mdata.dY, mdata.dZ, ItemID); byy krs
	}
	else
	{
		uint32 mid = 0;
		CMessage	msg(M_SC_MULTILANTERN_RES);
		msg.serial(FID, code, mid);
		CUnifiedNetwork::getInstance()->send(_tsid, msg);
	}
}

void	cb_M_CS_MULTILANTERN_JOIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				MID;				_msgin.serial(MID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_MULTILANTERN_JOIN(FID, MID, _tsid);
}

void	CChannelWorld::on_M_CS_MULTILANTERN_JOIN(T_FAMILYID FID, uint32 MID, SIPNET::TServiceId _tsid)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	uint32	code = ERR_NOERR;
	uint8	joinpos = MAX_MULTILANTERNJOINUSERS;
	MAP_MULTILANTERNDATA::iterator it = pChannelData->m_MultiLanternList.find(MID);
	if (it == pChannelData->m_MultiLanternList.end())
	{
		code = E_MULTILANTERN_NOEXIST;
	}
	else
	{
		_MultiLanternData& mdata = it->second;
		if (mdata.bStart)
		{
			code = E_MULTILANTERN_NOJOIN;
		}
		else
		{
			for (int i = 0; i < MAX_MULTILANTERNJOINUSERS; i++)
			{
				if (mdata.JoinID[i] == 0 ||
					mdata.JoinID[i] == FID)
				{
					mdata.JoinID[i] = FID;
					joinpos = i;
					break;
				}
			}
			if (joinpos == MAX_MULTILANTERNJOINUSERS)
				code = E_MULTILANTERN_NOJOIN;
		}
	}
	
	CMessage msg(M_SC_MULTILANTERN_JOIN);
	msg.serial(FID, code, joinpos);
	CUnifiedNetwork::getInstance()->send(_tsid, msg);

}

void	cb_M_CS_MULTILANTERN_START(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				MID;				_msgin.serial(MID);
	uint32				SyncCode;			_msgin.serial(SyncCode);
	T_FITEMPOS			invenPos;			_msgin.serial(invenPos);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_MULTILANTERN_START(FID, MID, SyncCode, invenPos, _tsid);
}

void	CChannelWorld::on_M_CS_MULTILANTERN_START(T_FAMILYID FID, uint32 MID, uint32 syncCode, T_FITEMPOS invenPos, SIPNET::TServiceId _tsid)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	MAP_MULTILANTERNDATA::iterator it = pChannelData->m_MultiLanternList.find(MID);
	if (it == pChannelData->m_MultiLanternList.end())
	{
		sipwarning("Can't multi lantern start(no id), RoomID=%d, ChannelID=%d, MultiLanternID=%d, FID=%d", m_RoomID, pChannelData->m_ChannelID, MID, FID);
		return;
	}

	_MultiLanternData& mdata = it->second;
	if (mdata.MasterID != FID)
	{
		sipwarning("Can't multi lantern start(no master),, RoomID=%d, ChannelID=%d, MultiLanternID=%d, MasterID=%d, FID=%d", m_RoomID, pChannelData->m_ChannelID, MID, mdata.MasterID, FID);
		return;
	}

	T_ID	ItemID = FamilyItemUsed(FID, invenPos, syncCode, ITEMTYPE_OTHER, _tsid);
	if (ItemID == 0)
	{
		sipwarning("Invalid ItemID: FDI=%d, InvenPos=%d, SyncCode=%d", FID, invenPos, syncCode);
		// 집체공명등 취소통보
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTILANTERN_CANCEL)
			msgOut.serial(mdata.MultiLanterID);
		FOR_END_FOR_FES_ALL()

		pChannelData->m_MultiLanternList.erase(MID);
		return;
	}

	{
		// 집체공명등 시작통보
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTILANTERN_START)
			msgOut.serial(mdata.MultiLanterID, mdata.dX, mdata.dY, mdata.dZ, ItemID, mdata.MasterID);
			for (int i = 0; i < MAX_MULTILANTERNJOINUSERS; i++)
			{
				if (mdata.JoinID[i] != 0)
				{
					uint8 jp = i;
					msgOut.serial(mdata.JoinID[i], jp);
				}
			}
		FOR_END_FOR_FES_ALL()
		// 경험값추가
		ChangeFamilyExp(Family_Exp_Type_LanternMaster, FID);
		for (int i = 0; i < MAX_MULTILANTERNJOINUSERS; i++)
		{
			if (mdata.JoinID[i] != 0)
				ChangeFamilyExp(Family_Exp_Type_LanternSlaver, mdata.JoinID[i]);
		}
	}

	mdata.bStart = true;
	AddNewLantern(mdata.MasterID, mdata.dX, mdata.dY, mdata.dZ, 0.0f, ItemID, false);
}

void	cb_M_CS_MULTILANTERN_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				MID;				_msgin.serial(MID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_MULTILANTERN_END(FID, MID, _tsid);
}

void	CChannelWorld::on_M_CS_MULTILANTERN_END(T_FAMILYID FID, uint32 MID, SIPNET::TServiceId _tsid)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}
/*
	{
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTILANTERN_END)
			msgOut.serial(MID);
		FOR_END_FOR_FES_ALL()
	}
*/
	pChannelData->m_MultiLanternList.erase(MID);
}

void	cb_M_CS_MULTILANTERN_CANCEL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				MID;				_msgin.serial(MID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_MULTILANTERN_CANCEL(FID, MID, _tsid);
}

void	CChannelWorld::on_M_CS_MULTILANTERN_CANCEL(T_FAMILYID FID, uint32 MID, SIPNET::TServiceId _tsid)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	MAP_MULTILANTERNDATA::iterator it;
	for (it = pChannelData->m_MultiLanternList.begin(); it != pChannelData->m_MultiLanternList.end(); it++)
	{
		_MultiLanternData& mdata = it->second;
		if (mdata.MasterID == FID)
		{
			// 집체공명등취소통보
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTILANTERN_CANCEL)
				msgOut.serial(mdata.MultiLanterID);
			FOR_END_FOR_FES_ALL()

			pChannelData->m_MultiLanternList.erase(mdata.MultiLanterID);
			return;
		}
	}
	{
		sipwarning("Can't multi lantern cancel(no master), RoomID=%d, ChannelID=%d, FID=%d", m_RoomID, pChannelData->m_ChannelID, FID);
		return;
	}

/*
	MAP_MULTILANTERNDATA::iterator it = pChannelData->m_MultiLanternList.find(MID);
	if (it == pChannelData->m_MultiLanternList.end())
	{
		sipwarning("Can't multi lantern cancel(no id), RoomID=%d, ChannelID=%d, MultiLanternID=%d, FID=%d", m_RoomID, pChannelData->m_ChannelID, MID, FID);
		return;
	}

	_MultiLanternData& mdata = it->second;
	if (mdata.MasterID != FID)
	{
		sipwarning("Can't multi lantern cancel(no master),, RoomID=%d, ChannelID=%d, MultiLanternID=%d, MasterID=%d, FID=%d", m_RoomID, pChannelData->m_ChannelID, MID, mdata.MasterID, FID);
		return;
	}

	{
		// 집체공명등취소통보
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_MULTILANTERN_CANCEL)
			msgOut.serial(mdata.MultiLanterID);
		FOR_END_FOR_FES_ALL()
	}

	pChannelData->m_MultiLanternList.erase(MID);
	*/
}

void	cb_M_CS_MULTILANTERN_OUTJOIN(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	T_ID				MID;				_msgin.serial(MID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;			// 비법발견

	inManager->on_M_CS_MULTILANTERN_OUTJOIN(FID, MID, _tsid);
}

void	CChannelWorld::on_M_CS_MULTILANTERN_OUTJOIN(T_FAMILYID FID, uint32 MID, SIPNET::TServiceId _tsid)
{
	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	MAP_MULTILANTERNDATA::iterator it = pChannelData->m_MultiLanternList.find(MID);
	if (it == pChannelData->m_MultiLanternList.end())
	{
		sipwarning("Can't multi lantern outjoin(no id), RoomID=%d, ChannelID=%d, MultiLanternID=%d, FID=%d", m_RoomID, pChannelData->m_ChannelID, MID, FID);
		return;
	}
	
	_MultiLanternData& mdata = it->second;
	for (int i = 0; i < MAX_MULTILANTERNJOINUSERS; i++)
	{
		if (mdata.JoinID[i] == FID)
		{
			mdata.JoinID[i] = 0;
		}
	}
}

void CChannelWorld::on_M_CS_FISH_FOOD(T_FAMILYID FID, uint32 StoreLockID, T_FITEMPOS InvenPos, T_ITEMSUBTYPEID _ItemID, uint32 FishScopeID, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	T_ID	ItemID = 0;
	const	MST_ITEM	*mstItem = NULL;
	const	OUT_PETCARE	*mstPetCare = NULL;

	if (StoreLockID != 0)
	{
		sipwarning("StoreLockID != 0. RoomID=%d, FID=%d, StoreLockID=%d", m_RoomID, FID, StoreLockID);
		return;
	}
	else
	{
		if (InvenPos != 0xFFFF) // 인벤사용이면
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
				sipwarning("GetFamilyInven NULL FID=%d", FID);
				return;
			}
			int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
			ITEM_INFO	&item = pInven->Items[InvenIndex];
			ItemID = item.ItemID;

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
		else // 아이템직접 사용이면
		{
			ItemID = _ItemID;
		}
	}

	mstItem = GetMstItemInfo(ItemID);
	mstPetCare = GetOutPetCare(ItemID);
	if ((mstItem == NULL) || (mstItem->TypeID != ITEMTYPE_PET_TOOL) || (mstPetCare == NULL) || (mstPetCare->PetType != PETTYPE_FISHGROUP))
	{
		sipwarning(L"Invalid Item. FID=%d, ItemID=%d", FID, ItemID);
		return;
	}
	//// Prize처리 - M_SC_INVENSYNC다음으로!
	//if (IsPossiblePrize(PRIZE_TYPE_FISH))
	//	CheckPrize(FID, PRIZE_TYPE_FISH, 2);

	// 경험값 변경
	ChangeFamilyExp(Family_Exp_Type_Fish, FID);

	ChangeFishExp(mstPetCare->IncLife, FID, FishScopeID, ItemID);

	if (SyncCode != 0)
	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
	}

	// Prize처리
	if (IsPossiblePrize(PRIZE_TYPE_FISH))
		CheckPrize(FID, PRIZE_TYPE_FISH, 2);

	// Log
	DBLOG_ItemUsed(DBLSUB_USEITEM_FISHFOOD, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

bool CChannelWorld::CheckIfActSave(T_FAMILYID FID)
{
	FAMILY *pF = GetFamilyData(FID);
	if (pF == NULL)
		return true; // FID가 Offline일 때에는 비체계사용자라고 본다!!! 체계사용자는 특사(예약행사)아이템을 사용할수 없게 만듬. false;
	T_ACCOUNTID userid = pF->m_UserID;

	// 비체계사용자의 활동은 제한을 받지 않는다
	if (!IsSystemAccount(userid))
		return true;

	// 체계사용자의 활동은 기타 다른 기념관에 영향을 줄수 없다
	return false;
}

BOOL CChannelWorld::IsChangeableFlower(T_FAMILYID FID)
{
	FAMILY *pF = GetFamilyData(FID);
	if (pF == NULL)
		return false;
	T_ACCOUNTID userid = pF->m_UserID;

	if (IsSystemAccount(userid))
		return true;

	return false;
}

void CChannelWorld::on_M_CS_FLOWER_NEW(T_FAMILYID FID, uint32 StoreLockID, T_FITEMPOS InvenPos, T_ITEMSUBTYPEID _ItemID, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	// 꽃 새로 심기는 방주인이나 관리자가 아니더라도 할수 있다. 손님은 꽃이 없는 경우에만 새로 심을수 있다.
	if (IfFlowerExist())
	{
		if(!IsChangeableFlower(FID))
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("IsChangeableFlower failed (2)! RoomID=%d, FID=%d", m_RoomID, FID);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
	}

	ChannelData*	pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	if (pChannelData->m_FlowerUsingFID != FID)
	{
		// 비법파케트발견
		sipwarning("Flower Current Using!!! UsingFID=%d, FID=%d", pChannelData->m_FlowerUsingFID, FID);
		return;
	}

	T_ID	ItemID = 0;
	const	MST_ITEM	*mstItem=NULL;
	if (StoreLockID != 0)
	{
		sipwarning("StoreLockID != 0. RoomID=%d, FID=%d, StoreLockID=%d", m_RoomID, FID, StoreLockID);
		return;
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
	if (IsPossiblePrize(PRIZE_TYPE_FLOWER))
		CheckPrize(FID, PRIZE_TYPE_FLOWER, 5);

	// 경험값 변경
	ChangeFamilyExp(Family_Exp_Type_Flower_Set, FID);

	// Log
	DBLOG_ItemUsed(DBLSUB_USEITEM_NEWFLOWER, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
}

BOOL CChannelWorld::IfFlowerExist()
{
	if(m_FlowerID == 0)
		return false;

	if (!(m_RoomID < ROOM_ID_VIRTUAL)
		&& !IsCommonRoom(m_RoomID) )
	{
		TTime	curTime = GetDBTimeSecond();
		if (m_FlowerDelTime < curTime)
		{
			m_FlowerID = 0;
			return false;
		}
	}

	return true;
}

void CChannelWorld::on_M_CS_FLOWER_NEW_REQ(T_FAMILYID FID, uint32 CallKey,  SIPNET::TServiceId _sid)
{
	uint32	errcode = ERR_NOERR;

	ChannelData*	pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. FID=%d", FID);
		return;
	}

	if(pChannelData->m_FlowerUsingFID != 0)
		errcode = E_FLOWER_USING;
	else
	{
		// 꽃 새로 심기는 방주인이나 관리자가 아니더라도 할수 있다. 손님은 꽃이 없는 경우에만 새로 심을수 있다.
		if(IfFlowerExist())
		{
			if(!IsChangeableFlower(FID))
			{
				// 비법파케트발견
				uint32	curTime = GetDBTimeSecond();
				ucstring ucUnlawContent = SIPBASE::_toString("IsChangeableFlower failed (1)! RoomID=%d, FID=%d, m_FlowerID=%d, m_FlowerDelTime=%d, now=%d", m_RoomID, FID, m_FlowerID, m_FlowerDelTime, curTime);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
		}

		pChannelData->m_FlowerUsingFID = FID;
	}

	CMessage	msgOut(M_SC_FLOWER_NEW_ANS);
	msgOut.serial(FID, errcode, CallKey);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

static bool bAlwaysWater = false;
void CChannelWorld::on_M_CS_FLOWER_WATER_REQ(T_FAMILYID FID, uint32 CallKey, SIPNET::TServiceId _sid)
{
	// 현재 꽃이 있는지 검사.
	if (!IfFlowerExist())
	{
		sipwarning("There is No Flower! FID=%d, RoomID=%d", FID, m_RoomID);
		return;
	}

	uint32	errcode = ERR_NOERR;
	uint32	remainsecond = 0;

	ChannelData*	pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. FID=%d, RoomID=%d", FID, m_RoomID);
		return;
	}

	uint32		curTime = (uint32)GetDBTimeSecond();
	TmapDwordToDword::iterator	it = m_mapFamilyFlowerWaterTime.find(FID);
	if (it != m_mapFamilyFlowerWaterTime.end())
	{
		const int WaterTime = 3 * 3600; // 3시간
		if ((it->second > 0) && (curTime - it->second < WaterTime - 10)) // 10초정도 여유를 준다.
		{
			remainsecond = WaterTime - (curTime - it->second);
		}
	}

	// 한 사용자는 한 방에서 3시간에 한번씩 물을 줄수 있다.
	if (pChannelData->m_FlowerUsingFID != 0)
	{
		errcode = E_FLOWER_USING;
	}
	else
	{
		if (!bAlwaysWater)
		{
			if (remainsecond > 0)
			{
				errcode = E_FLOWERWATER_WAIT;
			}
		}
	}
	if (errcode == ERR_NOERR)
		pChannelData->m_FlowerUsingFID = FID;
	else
	{
		sipinfo("Can't flower give water. RoomID=%d, FID=%d, err=%d, remain=%d", m_RoomID, FID, errcode, remainsecond);
	}

	CMessage	msgOut(M_SC_FLOWER_WATER_ANS);
	msgOut.serial(FID, errcode, remainsecond, CallKey);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
	return;
}

void CChannelWorld::on_M_CS_FLOWER_WATERTIME_REQ(T_FAMILYID FID, SIPNET::TServiceId _sid)
{
	// 현재 꽃이 있는지 검사.
	if (!IfFlowerExist())
	{
		sipwarning("There is No Flower! FID=%d, RoomID=%d", FID, m_RoomID);
		return;
	}

	uint32	remainsecond = 0;

	ChannelData*	pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. FID=%d, RoomID=%d", FID, m_RoomID);
		return;
	}

	uint32		curTime = (uint32)GetDBTimeSecond();
	if (!bAlwaysWater)
	{
		TmapDwordToDword::iterator	it = m_mapFamilyFlowerWaterTime.find(FID);
		if (it != m_mapFamilyFlowerWaterTime.end())
		{
			const int WaterTime = 3 * 3600; // 3시간
			if ((it->second > 0) && (curTime - it->second < WaterTime - 10)) // 10초정도 여유를 준다.
			{
				remainsecond = WaterTime - (curTime - it->second);
			}
		}
	}

	CMessage	msgOut(M_SC_FLOWER_WATERTIME_ANS);
	msgOut.serial(FID, remainsecond);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
}

void CChannelWorld::on_M_CS_FLOWER_WATER(T_FAMILYID FID, SIPNET::TServiceId _sid)
{
	// 현재 꽃이 있는지 검사.
	if (!IfFlowerExist())
	{
		sipwarning("There is No Flower! FID=%d, RoomID=%d", FID, m_RoomID);
		return;
	}

	// 한 사용자는 한 방에서 3시간에 한번씩 물을 줄수 있다.
	uint32		curTime = (uint32)GetDBTimeSecond();
	if (!bAlwaysWater)
	{
		TmapDwordToDword::iterator	it = m_mapFamilyFlowerWaterTime.find(FID);
		if (it != m_mapFamilyFlowerWaterTime.end())
		{
			const int WaterTime = 3 * 3600; // 3시간
			if ((it->second > 0) && (curTime - it->second < WaterTime - 10)) // 10초정도 여유를 준다.
			{
				sipwarning("GiveWaterFlower Time Error!!! RoomID=%d, FID=%d, DeltaTime=%d", m_RoomID, FID, curTime - it->second);
				//uint32	errcode = E_FLOWERWATER_WAIT;
				//uint32	remainsecond = WaterTime - (curTime - it->second);
				//CMessage	msgout(M_SC_FLOWER_FAIL);
				//msgout.serial(FID, errcode, remainsecond);
				//CUnifiedNetwork::getInstance()->send(_sid, msgout);
				return;
			}
		}
	}

	ChannelData*	pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. FID=%d", FID);
		return;
	}

	if (pChannelData->m_FlowerUsingFID != FID)
	{
		sipwarning("Flower Current Using!!! UsingFID=%d, FID=%d", pChannelData->m_FlowerUsingFID, FID);
		//uint32	errcode = E_FLOWER_USING;
		//CMessage	msgout(M_SC_FLOWER_FAIL);
		//msgout.serial(FID, errcode);
		//CUnifiedNetwork::getInstance()->send(_sid, msgout);
		return;
	}

	m_mapFamilyFlowerWaterTime[FID] = curTime;

	ChangeFlowerLevel(1, FID);

	// Prize처리
	if (IsPossiblePrize(PRIZE_TYPE_FLOWER))
		CheckPrize(FID, PRIZE_TYPE_FLOWER, 1);

	// 경험값 변경
	ChangeRoomExp(Room_Exp_Type_Flower_Water, FID);
	ChangeFamilyExp(Family_Exp_Type_Flower_Water, FID);

	// 기록 보관
	if (m_RoomID < MIN_RELIGIONROOMID || m_RoomID > MAX_RELIGIONROOMID)
	{
		INFO_FAMILYINROOM* pFamilyInfo = GetFamilyInfo(FID);
		if (pFamilyInfo == NULL)
		{
			sipwarning(L"GetFamilyInfo = NULL. RoomID=%d, FID=%d", m_RoomID, FID);
		}
		else
		{
			pFamilyInfo->DonActInRoom(ROOMACT_GIVEWATER);

			MAKEQUERY_AddFishFlowerVisitData(strQ, pFamilyInfo->m_nCurActionGroup, FF_FLOWER_WATER, 0, 0);
			DBCaller->execute(strQ);
		}
	}

	// Log
	DBLOG_CarePet(FID, 0, L"Water", 0, 0, m_RoomName, m_RoomID);
}

void CChannelWorld::on_M_CS_FLOWER_END(T_FAMILYID FID,  SIPNET::TServiceId _sid)
{
	ChannelData*	pChannelData = GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. FID=%d", FID);
		return;
	}

	if(pChannelData->m_FlowerUsingFID != FID)
	{
		sipwarning("You are not using flower. RoomID=%d, FID=%d, CurFlowerFID=%d", m_RoomID, FID, pChannelData->m_FlowerUsingFID);
		return;
	}

	pChannelData->m_FlowerUsingFID = 0;
}

void CChannelWorld::ChangeFishExp(int exp, uint32 FID, uint32 FishScopeID, uint32 FishFoodID)
{
	if (exp == 0)
		return;

	if (CheckIfActSave(FID))
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
}

void CChannelWorld::ChangeFlowerLevel(int count, uint32 FID, uint8 flag)
{
	bool	bSave = false;
	if (m_RoomID == ROOM_ID_PUBLICROOM || m_RoomID == ROOM_ID_BUDDHISM || m_RoomID == ROOM_ID_KONGZI || m_RoomID == ROOM_ID_HUANGDI)
	{
		bSave = true;
	}
	else
	{
		if (CheckIfActSave(FID))
			bSave = true;
	}
	if (bSave)
	{
		m_FlowerLevel += count;
		{
			MAKEQUERY_SaveFlowerLevel(strQ, m_RoomID, m_FlowerID, m_FlowerLevel, m_FlowerDelTime, m_FlowerFID);
			DBCaller->execute(strQ);
		}
	}

	//sipdebug("RoomID=%d,FlowerID=%d,FlowerLevel=%d", m_RoomID, m_FlowerID, m_FlowerLevel); byy krs

	uint32	RemainSecond = m_FlowerDelTime - (uint32)GetDBTimeSecond();
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_UPDATE_FLOWER)
		msgOut.serial(m_FlowerID, m_FlowerLevel, RemainSecond, FID, m_FlowerFID, m_FlowerFamilyName, flag);
	FOR_END_FOR_FES_ALL()
}

void CChannelWorld::SendFishFlowerInfo(T_FAMILYID _FID, TServiceId sid)
{
	uint32		uZero = 0;
	T_FAMILYID	fid = 0;

	TTime	curTime = GetDBTimeSecond();

	// 물고기정보
	{
		//uint32	MaxExp = mstFishLevel[m_FishLevel + 1].Exp;
		//uint16	percent = CalcFishExpPercent(m_FishLevel, m_FishExp);
		CMessage	msgOut(M_SC_UPDATE_FISH);
		uint32	FishScopeID = 0, FishFoodID = 0;
		msgOut.serial(_FID, uZero, m_FishExp, fid, FishScopeID, FishFoodID, m_nFishFoodCount);
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}

	// 꽃정보
	{
		uint8	flag = 0;
		uint32	RemainSecond = m_FlowerDelTime - (uint32)curTime;
		CMessage	msgOut(M_SC_UPDATE_FLOWER);
		msgOut.serial(_FID, uZero, m_FlowerID, m_FlowerLevel, RemainSecond, fid, m_FlowerFID, m_FlowerFamilyName, flag);
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}
}

void	CChannelWorld::SetFishExp(uint32 exp)
{
	m_FishExp = exp;
	m_FishLevel = GetFishLevelFromExp(m_FishExp);
}

void	CChannelWorld::SendMultiLanterList(T_FAMILYID _FID, SIPNET::TServiceId sid)
{
	ChannelData	*pChannelData = GetFamilyChannel(_FID);
	if(pChannelData == NULL)
		return;

	bool	bExist = false;
	CMessage msg(M_SC_MULTILANTERN_READYLIST);
	msg.serial(_FID);
	MAP_MULTILANTERNDATA::iterator it;
	for (it = pChannelData->m_MultiLanternList.begin(); it != pChannelData->m_MultiLanternList.end(); it++)
	{
		_MultiLanternData &mdata = it->second;
		if (mdata.bStart)
			continue;

		msg.serial(mdata.MultiLanterID, mdata.dX, mdata.dY, mdata.dZ, mdata.ReqItemID, mdata.MasterID);
		bExist = true;
	}

	if (bExist)
		CUnifiedNetwork::getInstance()->send(sid, msg);
}

uint32 GetMaxPriceOfTaocanForYishi()
{
	uint32	maxPrice = 0;
	for (MAP_OUT_TAOCAN_FOR_AUTOPLAY::iterator it = map_OUT_TAOCAN_FOR_AUTOPLAY.begin(); it != map_OUT_TAOCAN_FOR_AUTOPLAY.end(); it ++)
	{
		if (it->second.TaocanType == SACRFTYPE_TAOCAN_ROOM)
		{
			const MST_ITEM *mstItem = GetMstItemInfo(it->first);
			if (mstItem == NULL)
			{
				sipwarning(L"GetMstItemInfo = NULL. ItemID=%d", it->first);
			}
			else
			{
				if ((mstItem->UMoney > 0) && (mstItem->UMoney > maxPrice))
					maxPrice = mstItem->UMoney;
			}
		}
	}
	return maxPrice;
}

uint32 GetMaxPriceOfTaocanForXianbao()
{
	uint32	maxPrice = 0;
	for (MAP_OUT_TAOCAN_FOR_AUTOPLAY::iterator it = map_OUT_TAOCAN_FOR_AUTOPLAY.begin(); it != map_OUT_TAOCAN_FOR_AUTOPLAY.end(); it ++)
	{
		if (it->second.TaocanType == SACRFTYPE_TAOCAN_XIANBAO)
		{
			const MST_ITEM *mstItem = GetMstItemInfo(it->first);
			if (mstItem == NULL)
			{
				sipwarning(L"GetMstItemInfo = NULL. ItemID=%d", it->first);
			}
			else
			{
				if ((mstItem->UMoney > 0) && (mstItem->UMoney > maxPrice))
					maxPrice = mstItem->UMoney;
			}
		}
	}
	return maxPrice;
}

bool CChannelWorld::IsPossiblePrize(uint8 PrizeType)
{
	return true;
}

static bool		bAlwaysPrizeForDev = false;
bool CChannelWorld::CheckPrize(T_FAMILYID FID, uint8 PrizeType, uint32 Param, bool bOnlyOne)
{
	static uint32	maxPriceForYishi = 0;
	static uint32	maxPriceForXianbao = 0;

	if (maxPriceForYishi == 0)
		maxPriceForYishi = GetMaxPriceOfTaocanForYishi();
	if (maxPriceForXianbao == 0)
		maxPriceForXianbao = GetMaxPriceOfTaocanForXianbao();

	FAMILY	*pFamilyInfo = GetFamilyData(FID);
	uint32	curTime1970 = CTime::getSecondsSince1970();

	uint8	PrizeLevel = 0;

	if (PrizeType == PRIZE_TYPE_LUCK)
	{
		PrizeLevel = PRIZE_LEVEL_1;
	}
	else
	{
		uint32	maxValue;
		switch(PrizeType)
		{
		case PRIZE_TYPE_YISHI:
			if (rand() & 1)
			{
				PrizeLevel = PRIZE_LEVEL_1;
				maxValue = maxPriceForYishi * 5;
			}
			else
			{
				PrizeLevel = PRIZE_LEVEL_YISHI3;
				maxValue = maxPriceForYishi * 10;
			}
			break;
		case PRIZE_TYPE_XIANBAO:
			if (rand() & 1)
			{
				PrizeLevel = PRIZE_LEVEL_1;
				maxValue = maxPriceForXianbao * 5;
			}
			else
			{
				PrizeLevel = PRIZE_LEVEL_XIANBAO3;
				maxValue = maxPriceForXianbao * 10;
			}
			break;
		case PRIZE_TYPE_FLOWER:
			PrizeLevel = PRIZE_LEVEL_1;
			maxValue = 100;

			// 모바일에서는 PRIZE_TYPE_FLOWER, PRIZE_TYPE_FISH의 中奖이 없다!!!
			if (pFamilyInfo != NULL && pFamilyInfo->m_bIsMobile == 1)
			{
				Param = 0;
			}
			break;
		case PRIZE_TYPE_FISH:
			PrizeLevel = PRIZE_LEVEL_1;
			maxValue = 100;

			// 이 방에서 60초전에 中奖이 나왔다면 Param을 0으로 설정하여 다시 나오지 않게 한다. (한자리에서 두개의 캐릭터가 동시에 中奖이 나오는것을 방지)
			if (curTime1970 - m_LastPrizeForFishTime < 60  && !bAlwaysPrizeForDev)
			{
				Param = 0;
			}
			// 모바일에서는 PRIZE_TYPE_FLOWER, PRIZE_TYPE_FISH의 中奖이 없다!!!
			if (pFamilyInfo != NULL && pFamilyInfo->m_bIsMobile == 1)
			{
				Param = 0;
			}
			break;
		case PRIZE_TYPE_BUDDHISM_YISHI:
			if (rand() & 1)
			{
				PrizeLevel = PRIZE_LEVEL_1;
				maxValue = maxPriceForYishi * 5;
			}
			else
			{
				PrizeLevel = PRIZE_LEVEL_BUDDHISM_YISHI;
				maxValue = maxPriceForYishi * 10;
			}
			break;
		case PRIZE_TYPE_BUDDHISM_BIGYISHI:
			{
				PrizeLevel = PRIZE_LEVEL_BUDDHISM_BIGYISHI;
				maxValue = 0;
				Param = RAND_MAX;
			}
			break;
		case PRIZE_TYPE_ANIMAL:		// 1%
			PrizeLevel = PRIZE_LEVEL_ANIMAL;
			maxValue = 100;
			Param = 5;//changed from 1 to 5 by krs

			// 한 방(한 써버)에서 한 사용자는 PRIZE_TYPE_ANIMAL 中奖을 10분에 한번씩만 받을수 있다.
			if (pFamilyInfo != NULL && curTime1970 - pFamilyInfo->m_LastAnimalPrizeTime < 600)
				Param = 0;
			break;
		default:
			return false;
		}

		uint32	rnd_value = rand() * maxValue / RAND_MAX;

		//sipdebug("PRIZE : FID=%d, PrizeType=%d, PrizeLevel=%d, maxValue=%d, Param=%d, rnd_value=%d, maxPriceForYishi=%d, maxPriceForXianbao=%d", FID, PrizeType, PrizeLevel, maxValue, Param, rnd_value, maxPriceForYishi, maxPriceForXianbao); byy krs
		if (rnd_value >= Param && !bAlwaysPrizeForDev)
		{
			// 中奖없음
			if (pFamilyInfo != NULL)
			{
				uint32		zero = 0;
				uint8		zero8 = 0;	// PrizeType = 0 : No Prize
				CMessage	msgOut(M_SC_PRIZE);
				msgOut.serial(FID, zero, FID, zero8);
				CUnifiedNetwork::getInstance()->send(pFamilyInfo->m_FES, msgOut);
			}
			return false;
		}

		if (PrizeType == PRIZE_TYPE_FISH)
		{
			m_LastPrizeForFishTime = curTime1970;
		}
		else if (PrizeType == PRIZE_TYPE_ANIMAL)
		{
			if (pFamilyInfo != NULL)
				pFamilyInfo->m_LastAnimalPrizeTime = curTime1970;
		}
	}

	// 中奖있음
	OUT_PRIZE *data = GetPrize(PrizeLevel);
	if (data == NULL)
	{
		sipwarning("GetPrize = NULL. PrizeLevel = %d", PrizeLevel);
		return false;
	}

	_InvenItems	*pInven = GetFamilyInven(FID);
	if (pInven == NULL)
	{
		sipwarning("GetFamilyInven failed. FID=%d", FID);
		return false;
	}

	// 아이템을 인벤에 넣어준다.
	uint16	InvenPos = 0xFFFF;
	for (int InvenIndex = 0; InvenIndex < MAX_INVEN_NUM; InvenIndex ++)
	{
		if (pInven->Items[InvenIndex].ItemID == 0)
		{
			pInven->Items[InvenIndex].ItemID = data->PrizeItemID;
			pInven->Items[InvenIndex].ItemCount = 1;
			pInven->Items[InvenIndex].GetDate = 0;
			pInven->Items[InvenIndex].UsedCharID = 0;
			pInven->Items[InvenIndex].MoneyType = MONEYTYPE_UMONEY;

			if (!SaveFamilyInven(FID, pInven))
			{
				sipwarning("SaveFamilyInven failed. FID=%d", FID);
				return false;
			}

			InvenPos = GetInvenPosFromInvenIndex(InvenIndex);

			// Log
			const MST_ITEM *mstItem = GetMstItemInfo(data->PrizeItemID);
			if ( mstItem == NULL )
			{
				sipwarning(" GetMstItemInfo = NULL. ItemID = %d", data->PrizeItemID);
			}
			else
			{
				DBLOG_GetBankItem(FID, data->PrizeItemID, 1, mstItem->Name);
			}

			sipinfo("Send Prize: FID=%d, ItemID=%d", FID, data->PrizeItemID);

			break;
		}
	}
	if (InvenPos == 0xFFFF)
	{
		// 인벤에 빈자리가 없는 경우 아이템을 메일로 보내준다.
		// 2012/06/13 메일로 받은 아이템은 다시 보낼수 있는데 중장에서 받은 아이템은 다시 보내면 안되기때분에 이 부분을 삭제한다.
		//		_MailItems	MailItems;
		//		MailItems.Items[0].ItemID = data->PrizeItemID;
		//		MailItems.Items[0].ItemCount = 1;
		//		SendSystemMail(SYSTEM_MAIL_FID, FID, GetLangText(ID_PRIZE_MAIL_TITLE), GetLangText(ID_PRIZE_MAIL_CONTENT), 1, MailItems, TServiceId(0));
	}

	if (bOnlyOne)
	{
		uint32	zero = 0;
		if (pFamilyInfo != NULL)
		{
			CMessage	msgOut(M_SC_PRIZE);
			msgOut.serial(FID, zero, FID, PrizeType, data->PrizeID, InvenPos);
			CUnifiedNetwork::getInstance()->send(pFamilyInfo->m_FES, msgOut);
		}
	}
	else
	{
		// 방에 있는 사용자들에게 통지한다.
		ChannelData	*pChannelData = GetFamilyChannel(FID);
		if (pChannelData == NULL)
		{
			sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
			return false;
		}
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_PRIZE)
			msgOut.serial(FID, PrizeType, data->PrizeID, InvenPos);
		FOR_END_FOR_FES_ALL()
	}

	return true;
}

bool CheckPrize(T_FAMILYID FID, uint8 PrizeType, uint32 Param, bool bOnlyOne)
{
	CChannelWorld* pWorld = GetChannelWorldFromFID(FID);
	if (pWorld == NULL)
		return false;
	return pWorld->CheckPrize(FID, PrizeType, Param, bOnlyOne);
}

void CChannelWorld::CheckYuWangPrize(T_FAMILYID FID)
{
	uint32	curTime = CTime::getSecondsSince1970();
	if (curTime - m_LastPrizeForFishTime < 60 && !bAlwaysPrizeForDev)
		return;

	uint32 TeXiaoPercent = GetYuWangTeXiaoPercent(m_FishLevel);
	uint32	rnd_value = rand() * 100 / RAND_MAX;
	if (rnd_value >= TeXiaoPercent && !bAlwaysPrizeForDev)
		return;

	ChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}
	uint32	param = rand();
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_YUWANGPRIZE)
		msgOut.serial(FID, param);
	FOR_END_FOR_FES_ALL()

	m_LastPrizeForFishTime = curTime;
}

bool CChannelWorld::IsInThisChannel(T_FAMILYID FID, uint32 ChannelID)
{
	map<T_FAMILYID,	INFO_FAMILYINROOM>::iterator	it;
	it = m_mapFamilys.find(FID);
	if (it == m_mapFamilys.end())
		return false;
	if(it->second.m_nChannelID != ChannelID)
		return false;
	return true;
}

// 캐릭터의 복장을 갱신하고 통보한다
void	CChannelWorld::SetCharacterDress(T_FAMILYID _FID, T_CHARACTERID _CHID, T_DRESS _DressID)
{
	CHARACTER	*character = GetCharacterData(_CHID);
	if (character == NULL)
		return;
	character->m_DressID = _DressID;

	uint32	ChannelID = GetFamilyChannelID(_FID);
	ChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, ChannelID=%d", m_RoomID, ChannelID);
		return;
	}
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_CHANGECHDRESS)
		msgOut.serial(m_RoomID, _FID, _CHID, _DressID);
	FOR_END_FOR_FES_ALL()
}

void cb_M_CS_MULTIACT_REQADD(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	uint8		ActPos;		_msgin.serial(ActPos);

	uint8		Count;		_msgin.serial(Count);
	if(Count >= MAX_MULTIACT_FAMILY_COUNT)
	{
		sipwarning("Count Error!!! Count=%d", Count);
		return;
	}

	uint32	ActFIDs[MAX_MULTIACT_FAMILY_COUNT];
	memset(ActFIDs, 0, sizeof(ActFIDs));

	FAMILY	*pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", FID);
		return;
	}
	uint8	bIsMobile = pFamily->m_bIsMobile;

	uint32	JoinFID;
	int	j = 0;
	for(int i = 0; i < (int)Count; i ++)
	{
		_msgin.serial(JoinFID);

		pFamily = GetFamilyData(JoinFID);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. JoinFID=%d", JoinFID);
			return;
		}
		if (bIsMobile != pFamily->m_bIsMobile)
		{
			sipwarning("Can't do multiact with different client(PC&Mobile). FID=%d, bIsMobile=%d, JoinFID=%d, bIsMobile=%d", FID, bIsMobile, JoinFID, pFamily->m_bIsMobile);
			continue;
		}

		ActFIDs[j]  = JoinFID;
		j ++;
	}

	uint16	CountDownSec;
	_msgin.serial(CountDownSec);

	inManager->on_M_CS_MULTIACT_REQADD(FID, ActPos, ActFIDs, CountDownSec, _tsid);
}

void cb_M_CS_MULTIACT_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	uint8		ActPos;		_msgin.serial(ActPos);
	uint8		ActType;	_msgin.serial(ActType);
	uint8		Count;		_msgin.serial(Count);
	if(Count >= MAX_MULTIACT_FAMILY_COUNT)
	{
		sipwarning("Count Error!!! Count=%d", Count);
		return;
	}

	uint32	ActFIDs[MAX_MULTIACT_FAMILY_COUNT];
	memset(ActFIDs, 0, sizeof(ActFIDs));

	FAMILY	*pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipwarning("GetFamilyData = NULL. FID=%d", FID);
		return;
	}
	uint8	bIsMobile = pFamily->m_bIsMobile;

	uint32	JoinFID;
	ActFIDs[0] = FID;
	int	j = 0;
	for(int i = 0; i < (int)Count; i ++)
	{
		_msgin.serial(JoinFID);

		pFamily = GetFamilyData(JoinFID);
		if (pFamily == NULL)
		{
			sipwarning("GetFamilyData = NULL. JoinFID=%d", JoinFID);
			return;
		}
		if (bIsMobile != pFamily->m_bIsMobile)
		{
			sipwarning("Can't do multiact with different client(PC&Mobile). FID=%d, bIsMobile=%d, JoinFID=%d, bIsMobile=%d", FID, bIsMobile, JoinFID, pFamily->m_bIsMobile);
			continue;
		}

		j ++;
		ActFIDs[j] = JoinFID;
	}

	uint16	CountDownSec;
	_msgin.serial(CountDownSec);

	inManager->on_M_CS_MULTIACT_REQ(ActFIDs, ActPos, ActType, CountDownSec, _tsid);
}

// Answer MultiActivity Request([d:RoomID][d:ChannelID][b:ActPos][b:Answer,Yes-1,No-0])
void cb_M_CS_MULTIACT_ANS(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	T_ID		RoomID, ChannelID;
	uint8		ActPos, Answer;

	_msgin.serial(FID, RoomID, ChannelID, ActPos, Answer);

	CChannelWorld *inManager = GetChannelWorld(RoomID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_MULTIACT_ANS(FID, ChannelID, ActPos, Answer);
}

// Ready MultiActivity([b:ActPos])
void cb_M_CS_MULTIACT_READY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint8		ActPos;

	_msgin.serial(FID, ActPos);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_MULTIACT_READY(FID, ActPos, _tsid);
}

// Host Family want to start
void cb_M_CS_MULTIACT_GO(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	uint8		ActPos;
	uint32		bXianDaGong;
	_msgin.serial(ActPos, bXianDaGong);

	// 2014/07/02:start
	uint8		GongpinCount;		_msgin.serial(GongpinCount);
	_ActivityStepParam	paramYisi;
	paramYisi.IsXDG = bXianDaGong; // 2014/07/04
	paramYisi.GongpinCount = GongpinCount;
	if (GongpinCount != 0)
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
		sipwarning(L"gongpin count is zero.");
		return;
	}
	// end

	inManager->on_M_CS_MULTIACT_GO(FID, ActPos, bXianDaGong, &paramYisi);
}

// activity step : 14/07/23
void cb_M_CS_ACT_STEP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint8			ActPos;
	uint32			StepID;
	_msgin.serial(FID, ActPos, StepID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_ACT_STEP(FID, ActPos, StepID);
}


// Start MultiActivity ([b:ActPos][d:StoreLockID][w:InvenPos][d:SyncCode][b:Secret][u:Pray])
void cb_M_CS_MULTIACT_START(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint8		ActPos, Secret;
	uint16		InvenPos;
	uint32		SyncCode, StoreLockID;
	ucstring	Pray;
	_msgin.serial(FID, ActPos, StoreLockID, InvenPos, SyncCode, Secret, Pray);

	if (!IsValidInvenPos(InvenPos))
	{
		// 원래 처리는 WRN로그출력만 하는것이다.
		// 그러나 클라이언트에서 InvenPos=0xFFFF를 올려보내는 바그를 찾지 못했기때문에 짤라버리는것으로 처리함
//		sipwarning("Invalid InvenPos. FID=%d, InvenPos=%d, ActPos=%d", FID, InvenPos, ActPos);
//		return;
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"Invalid InvenPos. FID=%d, InvenPos=%d, ActPos=%d, StoreLockID=%d, SyncCode=%d. Reason: 2 Multiact with RoomStore and Inven items", FID, InvenPos, ActPos, StoreLockID, SyncCode);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_MULTIACT_START(FID, ActPos, StoreLockID, InvenPos, SyncCode, Secret, Pray, _tsid);
}

// Finish MultiActivity ([b:ActPos][d:StoreLockID][b:Number][d:SyncCode]<[[w:InvenPos]]>) Number=0이면 뒤의 값들이 의미를 가지지 않는다.
void cb_M_CS_MULTIACT_RELEASE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint8		ActPos, Number;
	uint32		SyncCode = 0, StoreLockID;
	uint16		InvenPos[MAX_ITEM_PER_ACTIVITY] = {0};

	_msgin.serial(FID, ActPos, StoreLockID, Number);
	if(Number > MAX_ITEM_PER_ACTIVITY)
	{
		// 비법발견
		ucstring ucUnlawContent = SIPBASE::_toString("Count is too big! count=%d", Number).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	if(Number != 0)
		_msgin.serial(SyncCode);
	uint8 i = 0;
	for(i = 0; i < Number; i ++)
	{
		_msgin.serial(InvenPos[i]);
	}
	for(; i < MAX_ITEM_PER_ACTIVITY; i ++)
		InvenPos[i] = 0;

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager != NULL)
	{
		inManager->on_M_CS_MULTIACT_RELEASE(FID, ActPos, StoreLockID, SyncCode, Number, &InvenPos[0], _tsid);
	}
}

// MultiActivity Wait Cancel ([d:RoomID][d:ChannelID][b:ActPos])
void cb_M_CS_MULTIACT_CANCEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	T_ID		RoomID, ChannelID;
	uint8		ActPos;

	_msgin.serial(FID, RoomID, ChannelID, ActPos);

	CChannelWorld *inManager = GetChannelWorld(RoomID);
	if (inManager == NULL)
		return;

	inManager->on_M_CS_MULTIACT_CANCEL(FID, ChannelID, ActPos);
}

// Send MultiActivity Command from Host to Join ([b:ActPos][d:JoinFID][b:Command][d:Param])
void cb_M_CS_MULTIACT_COMMAND(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	HostFID;
	uint8		ActPos, Command;
	uint32		JoinFID, Param;

	_msgin.serial(HostFID, ActPos, JoinFID, Command, Param);

	CChannelWorld *inManager = GetChannelWorldFromFID(HostFID);
	if (inManager != NULL)
	{
		inManager->on_M_CS_MULTIACT_COMMAND(HostFID, ActPos, JoinFID, Command, Param);
	}
}

// Send MultiActivity Reply from Join to Host ([b:ActPos][b:Command][d:Param])
void cb_M_CS_MULTIACT_REPLY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	JoinFID;
	uint8		ActPos, Command;
	uint32		Param;

	_msgin.serial(JoinFID, ActPos, Command, Param);

	CChannelWorld *inManager = GetChannelWorldFromFID(JoinFID);
	if (inManager != NULL)
	{
		inManager->on_M_CS_MULTIACT_REPLY(JoinFID, ActPos, Command, Param);
	}
}

// 폭죽을 재생한다.
void	CChannelWorld::RefreshCracker()
{
	TTime	curtime = SIPBASE::CTime::getLocalTime();

	for(ChannelData	*pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		VEC_CRACKERLIST&	list = pChannelData->m_CracketList;

		VEC_CRACKERLIST::iterator it1 = list.begin();
		while(it1 != list.end())
		{
			if(curtime - it1->DeletedTime < CRACKER_CREATE_TIME)	// 폭죽은 60초에 한번씩 재생된다.
				break;

			uint32	CrackerID = it1->CrackerID;
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_CRACKER_CREATE)
				msgOut.serial(CrackerID);
			FOR_END_FOR_FES_ALL()

			//sipdebug("Cracker Created. RoomID=%d, CrackerID=%d", m_RoomID, CrackerID); byy krs

			list.erase(it1);
			it1 = list.begin();
		}
	}
}

// Request Deletes Cracker List
void SendCrackerList(uint32 FID, SIPNET::TServiceId _tsid)
{
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if(inManager == NULL)
	{
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
		return;
	}
	VEC_CRACKERLIST&	list = pChannelData->m_CracketList;

	CMessage	msgOut(M_SC_CRACKER_LIST);
	uint32	Count = list.size();
	msgOut.serial(FID, Count);
	for(VEC_CRACKERLIST::iterator it = list.begin(); it != list.end(); it ++)
	{
		msgOut.serial(it->CrackerID);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

// Cracker Bomb Request ([d:CrackerID])
void cb_M_CS_CRACKER_BOMB(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;		_msgin.serial(FID);
	T_ID		CrackerID;	_msgin.serial(CrackerID);

	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if(inManager == NULL)
	{
		sipwarning("GetChannelWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	ChannelData	*pChannelData = inManager->GetFamilyChannel(FID);
	if(pChannelData == NULL)
	{
		sipwarning("pChannelData = NULL. RoomID=%d, FID=%d", inManager->m_RoomID, FID);
		return;
	}
	VEC_CRACKERLIST&	list = pChannelData->m_CracketList;

	// 이미 삭제된것인지 검사
	for(VEC_CRACKERLIST::iterator it = list.begin(); it != list.end(); it ++)
	{
		if(it->CrackerID == CrackerID)
		{
			sipdebug("Already Deleted. RoomID=%d, CrackerID=%d", inManager->m_RoomID, CrackerID);
			return;
		}
	}

	// 새로 추가
	_Cracker_Data	data;
	data.CrackerID = CrackerID;
	data.DeletedTime = CTime::getLocalTime();
	list.push_back(data);

	// 삭제됨을 사용자들에게 통지
	FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_CRACKER_BOMB)
		msgOut.serial(CrackerID);
	FOR_END_FOR_FES_ALL()

	//sipdebug("Cracker Deleted. RoomID=%d, CrackerID=%d", inManager->m_RoomID, CrackerID); byy krs
}

void	cb_M_CS_ADD_TOUXIANG(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	DELEGATEPROC_TO_CHANNEL(M_CS_ADD_TOUXIANG, _msgin, _tsid);
}

void ChannelData::on_M_CS_ADD_TOUXIANG(T_FAMILYID FID, SIPNET::CMessage &_msgin, SIPNET::TServiceId _tsid)
{
	uint8			ActPos;
	T_ITEMSUBTYPEID	ItemID;
	_msgin.serial(ActPos, ItemID);

	std::map<uint8, CTouXiangInfo>::iterator it = m_TouXiangInfo.find(ActPos);
	if (it != m_TouXiangInfo.end())
	{
		uint32 now = (uint32)GetDBTimeSecond();
		sipwarning("There is already touxiang : RoomID=%d, ChannelID=%d, ActPos=%d, OldFID=%d, NewFID=%d, it->second.StartTimeS=%d, now=%d", m_RoomID, m_ChannelID, ActPos, it->second.FID, FID, it->second.StartTimeS, now);
		return;
	}
	T_FAMILYNAME FName = GetFamilyNameFromBuffer(FID);
	m_TouXiangInfo[ActPos] = CTouXiangInfo(ActPos, FID, FName, ItemID);

	FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_ADD_TOUXIANG)
	{
		uint8 nCount = 1;
		msgOut.serial(nCount, ActPos, ItemID, FName, m_TouXiangInfo[ActPos].StartTimeS);
	}
	FOR_END_FOR_FES_ALL()

	//sipdebug("Added touxiang : ActPos=%d, FID=%d, ItemID=%d", ActPos, FID, ItemID); byy krs
}

void ChannelData::SendTouXiangState(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	uint8 nCount = (uint8)m_TouXiangInfo.size();
	std::map<uint8, CTouXiangInfo>::iterator it;
	CMessage	msgOut(M_SC_ADD_TOUXIANG);
	uint32	zero = 0;
	msgOut.serial(FID, zero);
	msgOut.serial(nCount);
	for (it = m_TouXiangInfo.begin(); it != m_TouXiangInfo.end(); it++)
	{
		msgOut.serial(it->second.ActPos, it->second.ItemID, it->second.FName, it->second.StartTimeS);
	}
	CUnifiedNetwork::getInstance()->send(sid, msgOut);
}

void ChannelData::RefreshTouXiang()
{
	uint32 curTime = (uint32)GetDBTimeSecond();
	std::map<uint8, CTouXiangInfo>::iterator it;
	for (it = m_TouXiangInfo.begin(); it != m_TouXiangInfo.end(); )
	{
		if (curTime - it->second.StartTimeS >= 3600*3)	// 3시간
		{
			FOR_START_FOR_FES_ALL(m_mapFesChannelFamilys, msgOut, M_SC_DEL_TOUXIANG)
			{
				uint8 nCount = 1;
				msgOut.serial(nCount, it->second.ActPos);
			}
			FOR_END_FOR_FES_ALL()

			it = m_TouXiangInfo.erase(it);
			continue;
		}
		it++;
	}
}

void CChannelWorld::RefreshTouXiang()
{
	for (ChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		pChannelData->RefreshTouXiang();
	}
}

void CChannelWorld::SetRoomEvent()
{
	SetRoomEventStatus(ROOMEVENT_STATUS_READY);

	uint32	curDay = (uint32)(GetDBTimeSecond() / SECOND_PERONEDAY);
	uint32	tm = (SECOND_PERONEDAY - 1200) / MAX_ROOMEVENT_COUNT;	// 0:10~23:50사이에 나오게 한다.

	for (int i = 0; i < MAX_ROOMEVENT_COUNT; i ++)
	{
		uint8 ty = (uint8)rand(); // (int)(rand() * ROOM_EVENT_COUNT / RAND_MAX);		// 
		uint8 po = (uint8)rand(); // (int)(rand() * 256 / RAND_MAX);
		uint32 st = (int)(rand() * (tm - ROOM_EVENT_MAX_TIME) / RAND_MAX + tm * i) + 600;
		st += (curDay * SECOND_PERONEDAY);
		m_RoomEvent[i].SetInfo(ty, po, st, IsMusicEvent(ty));

		ucstring timestr = GetTimeStrFromS1970(st);
		//sipdebug(L"Set room event : RoomID=%d, EventType=%d, EventPos=%d, time=%s", m_RoomID, ty, po, timestr.c_str()); byy krs
	}
}

void CChannelWorld::RefreshRoomEvent()
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
				SetRoomEventStatus(ROOMEVENT_STATUS_READY);
			}
		}
		break;
	}
}

void CChannelWorld::StartRoomEvent(int i)
{
	SetRoomEventStatus(ROOMEVENT_STATUS_DOING, i);

	m_RoomEvent[i].nRoomEventStartTime = (uint32) GetDBTimeSecond();

	// 방의 사용자들에게 통지한다.
	FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_ROOMEVENT)
		msgOut.serial(m_RoomEvent[i].byRoomEventType, m_RoomEvent[i].byRoomEventPos, m_RoomEvent[i].nRoomEventStartTime);
	FOR_END_FOR_FES_ALL()

	//sipdebug("Start room event : RoomID=%d, EventType=%d, EvenetPos=%d", m_RoomID, m_RoomEvent[i].byRoomEventType, m_RoomEvent[i].byRoomEventPos) ; byy krs
}

void CChannelWorld::SendRoomEventData(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	int i = GetCurrentDoingRoomEventIndex();
	if (i >= 0)
	{
		uint32	zero = 0;
		CMessage	msgOut(M_SC_ROOMEVENT);
		msgOut.serial(FID, zero, m_RoomEvent[i].byRoomEventType, m_RoomEvent[i].byRoomEventPos, m_RoomEvent[i].nRoomEventStartTime);
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}
}

void CChannelWorld::StartRoomEventByGM(uint8 RoomEventType)
{
	//	sipwarning("Not supproted.");
	//	return;
	if (GetRoomEventStatus() == ROOMEVENT_STATUS_DOING)
	{
		sipwarning("Can't start RoomEvent. RoomID=%d, m_nRoomEventStatus=%d", m_RoomID, m_nRoomEventStatus);
		return;
	}

	SetRoomEventStatus(ROOMEVENT_STATUS_READY);
	m_RoomEvent[0].SetInfo(RoomEventType, 1, (uint32)GetDBTimeSecond() + 60, IsMusicEvent(RoomEventType));

	//	StartRoomEvent(0);
}

void CChannelWorld::RefreshFreezingPlayers()
{
	for (ChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		T_FAMILYID	FID = pChannelData->FindFreezingPlayer();
		if (FID != 0)
		{
			// 비법발견
			ucstring ucUnlawContent = SIPBASE::_toString(L"FreezingPlayer : RoomID=%d, FID=%d", m_RoomID, FID);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		}
	}
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, setAlwaysPrize, "Set always prize propert 0/1 for dev", "")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	uint32 uv = atoui(v.c_str());
	if (uv == 0)
		bAlwaysPrizeForDev = false;
	else
		bAlwaysPrizeForDev = true;
	return true;
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, setAlwaysWater, "Set always water 0/1 for dev", "")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	uint32 uv = atoui(v.c_str());
	if (uv == 0)
		bAlwaysWater = false;
	else
		bAlwaysWater = true;
	return true;
}

//SIPBASE_CATEGORISED_COMMAND(OROOM_S, outRoomAll, "Out all users from this room", "")
//{
//	if(args.size() < 1)	return false;
//
//	string v	= args[0];
//	uint32 rid = atoui(v.c_str());
//
//	CChannelWorld* pWorld = GetChannelWorld(rid);
//	if (pWorld != NULL)
//		pWorld->OutRoomAll();
//
//	return true;
//}
