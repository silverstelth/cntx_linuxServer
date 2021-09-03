#include <misc/types_sip.h>
#include <net/service.h>
#include <misc/ucstring.h>

#include "../../common/Macro.h"
#include "../../common/Packet.h"
#include "../Common/Common.h"
#include "../Common/Lang.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

#include "main.h"
#include "AutoPlayer.h"
#include "manageroom.h"
#include "player.h"

static	uint32	snUnitID = 1;
TAutoPlayList	AutoPlayUnitListForRegister;
TAutoPlayList	AutoPlayUnitListForAct;
std::map<uint64, uint32>	AutoPlayUnitRemainCount;	// RoomID << 32 | FID, ACTPOS_YISHI Count << 16 | ACTPOS_XIANBAO Count << 8 | ACTPOS_AUTO2 Count

#define MAX_AUTOPLAYDATA_COUNT		100	// ActPosCount의 최대수와 같다. (현재 3개)
int				AutoPlayerDatasCount;
CAutoPlayData	AutoPlayerDatas[MAX_AUTOPLAYDATA_COUNT];
CAutoPlayer		AutoPlayers[AUTOPLAY_FID_COUNT];

std::map<uint64, TTime>	AutoPlayRegisterWaitingList;

//=========================================
// Callback Functions
//=========================================
// Packet: M_NT_AUTOPLAY_REGISTER
// Register a new AutoPlay, OpenRoomService->AutoPlayService ([d:RoomID][b:ActPos][d:IsXDG][d:FID][u:FName][d:ModelID][d:DressID][d:FaceID][d:HairID]
//															[u:Pray][b:Secret][ [d:ItemID] ][d:ItemID(for RoomExp)][b:MoneyType(for RoomExp)])
void cb_M_NT_AUTOPLAY_REGISTER(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ROOMID		RoomID;
	_msgin.serial(RoomID);
	if (!IsThisShardRoom(RoomID))
	{
		_msgin.invert();
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
		return;
	}

	uint8			ActPos;
	uint32			IsXDG;
	T_FAMILYID		FID;
	T_FAMILYNAME	FamilyName;
	T_MODELID		ModelID;
	T_DRESS			DressID;
	T_FACEID		FaceID;
	T_HAIRID		HairID;
	ucstring		Pray;
	uint8			SecretFlag;
	T_COMMON_ID		UsedItemID;
	uint8			UsedItemMoneyType;
	T_COMMON_ID		ItemID[9] = {0};

	_msgin.serial(ActPos, IsXDG, FID, FamilyName, ModelID, DressID, FaceID, HairID);
	_msgin.serial(Pray, SecretFlag);
	int nitemcount = 0;
	switch (ActPos)
	{
	case ACTPOS_YISHI:		nitemcount = 9;		break;
	case ACTPOS_XIANBAO:	nitemcount = 6;		break;
	case ACTPOS_AUTO2:		nitemcount = 4;		break;
	default:
		{
			sipwarning("Invalid ActPos. ActPos=%d", ActPos);
			return;
		}
	}
	for (int i = 0; i < nitemcount; i ++)
		_msgin.serial(ItemID[i]);

	_msgin.serial(UsedItemID, UsedItemMoneyType);

	// Register to AutoPlayUnitList
	AutoPlayUnit	data(FID, FamilyName, ModelID, DressID, FaceID, HairID, RoomID, ActPos, IsXDG, Pray, SecretFlag, ItemID, UsedItemID, UsedItemMoneyType);
	data.m_UnitID = snUnitID;		snUnitID ++;
	AutoPlayUnitListForRegister.push_back(data);

	//sipinfo("AutoPlayUnitListForRegister.size()=%d, UnitID=%d", AutoPlayUnitListForRegister.size(), snUnitID - 1); byy krs

	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_AUTOPLAY, data.m_UnitID, FID))
		RegisterActionMemo(data.m_UnitID);
}

void RegisterActionMemo(uint32 UnitID)
{
	for (TAutoPlayList::iterator it = AutoPlayUnitListForRegister.begin(); it != AutoPlayUnitListForRegister.end(); it ++)
	{
		if (it->m_UnitID == UnitID)
		{
			// M_NT_ADDACTIONMEMO 를 2번 련속해서 보내면 Shrd_Visit_Insert가 2번 련속해서 호출되는 문제가 있다.
			// 이를 퇴치하기 위하여 AutoPlayRegisterWaitingList 를 추가함.
			uint64	key = it->m_RoomID;
			key = (key << 32) | it->m_FID;

			TTime curTime = CTime::getLocalTime();
			if (AutoPlayRegisterWaitingList.find(key) != AutoPlayRegisterWaitingList.end())
			{
				if (curTime - AutoPlayRegisterWaitingList[key] < 120000)
					return;
			}
//			if (AutoPlayRegisterWaitingList.find(key) == AutoPlayRegisterWaitingList.end())
			{
				AutoPlayRegisterWaitingList[key] = CTime::getLocalTime();

				CMessage	msgOut(M_NT_ADDACTIONMEMO);
				msgOut.serial(it->m_UnitID, it->m_RoomID, it->m_FID, it->m_FamilyName, it->m_ActPos, it->m_IsXDG, it->m_Pray, it->m_SecretFlag);
				for (int i = 0; i < 9; i ++)
					msgOut.serial(it->m_ItemID[i]);
				msgOut.serial(it->m_UsedItemID, it->m_UsedItemMoneyType);
				SendMsgToRoomID(it->m_RoomID, msgOut);
			}
		}
	}
}

void RegisterActionMemo(uint32 RoomID, uint32 FID)
{
	for (TAutoPlayList::iterator it = AutoPlayUnitListForRegister.begin(); it != AutoPlayUnitListForRegister.end(); it ++)
	{
		if ((it->m_RoomID == RoomID) && (it->m_FID == FID))
		{
			uint64	key = RoomID;
			key = (key << 32) | FID;

			AutoPlayRegisterWaitingList[key] = CTime::getLocalTime();

			CMessage	msgOut(M_NT_ADDACTIONMEMO);
			msgOut.serial(it->m_UnitID, it->m_RoomID, it->m_FID, it->m_FamilyName, it->m_ActPos, it->m_IsXDG, it->m_Pray, it->m_SecretFlag);
			for (int i = 0; i < 9; i ++)
				msgOut.serial(it->m_ItemID[i]);
			msgOut.serial(it->m_UsedItemID, it->m_UsedItemMoneyType);
			SendMsgToRoomID(it->m_RoomID, msgOut);

			return;
		}
	}
}

bool AddToAutoPlayUnitList(AutoPlayUnit &unit)
{
	bool	bAdded = false;

	uint64	key = unit.m_RoomID;
	key = (key << 32) | unit.m_FID;
	std::map<uint64, uint32>::iterator	it2 = AutoPlayUnitRemainCount.find(key);
	if (it2 == AutoPlayUnitRemainCount.end())
	{
		bAdded = true;
		switch (unit.m_ActPos)
		{
		case ACTPOS_YISHI:
			AutoPlayUnitRemainCount[key] = 0x00010000;
			break;
		case ACTPOS_XIANBAO:
			AutoPlayUnitRemainCount[key] = 0x00000100;
			break;
		case ACTPOS_AUTO2:
			AutoPlayUnitRemainCount[key] = 0x00000001;
			break;
		default:
			sipwarning("Invalid ActPos. ActPos=%d", unit.m_ActPos);
			break;
		}
	}
	else
	{
		switch (unit.m_ActPos)
		{
		case ACTPOS_YISHI:
			if ((it2->second & 0x00FF0000) < 0x00070000)	// 최대 대기개수 3->7개로 제한, 2017/08/07
			{
				bAdded = true;
				it2->second += 0x00010000;
			}
			break;
		case ACTPOS_XIANBAO:
			if ((it2->second & 0x0000FF00) < 0x00000700)	// 최대 대기개수 3->7개로 제한, 2017/08/07
			{
				bAdded = true;
				it2->second += 0x00000100;
			}
			break;
		case ACTPOS_AUTO2:
			if ((it2->second & 0x000000FF) < 0x00000007)	// 최대 대기개수 3->7개로 제한, 2017/08/07
			{
				bAdded = true;
				it2->second += 0x00000001;
			}
			break;
		default:
			sipwarning("Invalid ActPos. ActPos=%d", unit.m_ActPos);
			break;
		}
	}

	if (bAdded)
	{
		AutoPlayUnitListForAct.push_back(unit);
	}

	return bAdded;
}

void DeleteInfoFromAutoPlayUnitList(AutoPlayUnit &unit)
{
	uint64	key = unit.m_RoomID;
	key = (key << 32) | unit.m_FID;
	std::map<uint64, uint32>::iterator	it2 = AutoPlayUnitRemainCount.find(key);
	if (it2 == AutoPlayUnitRemainCount.end())
	{
		sipwarning("it2 == AutoPlayUnitRemainCount.end(). RoomID=%d, FID=%d, ActPos=%d", unit.m_RoomID, unit.m_FID, unit.m_ActPos);
	}
	else
	{
		switch (unit.m_ActPos)
		{
		case ACTPOS_YISHI:
			if ((it2->second & 0x00FF0000) > 0)
				it2->second -= 0x00010000;
			else
				sipwarning("(it2->second & 0x00FF0000) = 0. RoomID=%d, FID=%d, ActPos=%d", unit.m_RoomID, unit.m_FID, unit.m_ActPos);
			break;
		case ACTPOS_XIANBAO:
			if ((it2->second & 0x0000FF00) > 0)
				it2->second -= 0x00000100;
			else
				sipwarning("(it2->second & 0x0000FF00) = 0. RoomID=%d, FID=%d, ActPos=%d", unit.m_RoomID, unit.m_FID, unit.m_ActPos);
			break;
		case ACTPOS_AUTO2:
			if ((it2->second & 0x000000FF) > 0)
				it2->second -= 0x00000001;
			else
				sipwarning("(it2->second & 0x000000FF) = 0. RoomID=%d, FID=%d, ActPos=%d", unit.m_RoomID, unit.m_FID, unit.m_ActPos);
			break;
		default:
			sipwarning("Invalid ActPos. ActPos=%d", unit.m_ActPos);
			break;
		}

		if (it2->second == 0)
			AutoPlayUnitRemainCount.erase(it2);
	}
}

// Packet: M_NT_ADDACTIONMEMO_OK
// Notice that Autoplay ready OpenRoomService->AutoPlayService ([d:UnitID][d:Param])
void cb_M_NT_ADDACTIONMEMO_OK(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32			UnitID, Param;

	_msgin.serial(UnitID, Param);

	for (TAutoPlayList::iterator it = AutoPlayUnitListForRegister.begin(); it != AutoPlayUnitListForRegister.end(); it ++)
	{
		if (it->m_UnitID == UnitID)
		{
			uint32 RoomID = it->m_RoomID;
			uint32 FID = it->m_FID;

			if (Param >= 0xFFFFFFF0)
			{
				// Autoplay3이 실패한 경우
				RemoveAutoplayDatas(it->m_RoomID, Param);
			}
			else
			{
				if (it->m_ActPos == ACTPOS_YISHI || it->m_ActPos == ACTPOS_XIANBAO || it->m_ActPos == ACTPOS_AUTO2)
				{
					// 한명이 한방에 너무 많은 특사를 보내는것은 금지한다!!! (FID, RoomID, ActPos)
					AddToAutoPlayUnitList(*it);
				}
				else	// ACTPOS_XINGLI_1
				{
					// Autoplay3 - 응답을 보낸다.
					CMessage	msgOut(M_NT_AUTOPLAY3_START_OK);
					msgOut.serial(it->m_ModelID, it->m_DressID, Param);
					CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
				}

				AutoPlayUnitListForRegister.erase(it);

				//sipinfo("AutoPlayUnitListForRegister.size()=%d, UnitID=%d", AutoPlayUnitListForRegister.size(), UnitID); byy krs
			}

			uint64	key = RoomID;
			key = (key << 32) | FID;
			std::map<uint64, TTime>::iterator it1 = AutoPlayRegisterWaitingList.find(key);
			if (it1 != AutoPlayRegisterWaitingList.end())
				AutoPlayRegisterWaitingList.erase(it1);

			RegisterActionMemo(RoomID, FID);

			return;
		}
	}
	sipwarning("Can't find in AutoPlayUnitListForRegister. UnitID=%d", UnitID);
}


//=========================================
// Global Functions
//=========================================
void InitAutoPlayers()
{
	for (int i = 0; i < AUTOPLAY_FID_COUNT; i ++)
	{
		AutoPlayers[i].init(AUTOPLAY_FID_START + i);
	}

	AutoPlayerDatasCount = 0;
	{
		AutoPlayerDatas[AutoPlayerDatasCount ++].Init(ACTPOS_YISHI, 0, g_AutoPlayData_ActPos1);
		AutoPlayerDatas[AutoPlayerDatasCount ++].Init(ACTPOS_XIANBAO, 0, g_AutoPlayData_ActPos3);
		AutoPlayerDatas[AutoPlayerDatasCount ++].Init(ACTPOS_AUTO2, 0, g_AutoPlayData_ActPos83);
		AutoPlayerDatas[AutoPlayerDatasCount ++].Init(ACTPOS_YISHI, 1, g_AutoPlayData_ActPos1_XDG);
	}
	if (AutoPlayerDatasCount > MAX_AUTOPLAYDATA_COUNT)
	{
		siperror("AutoPlayerDatasCount > MAX_AUTOPLAYDATA_COUNT.");
		return;
	}
}

void UpdateAutoPayers()
{
	static uint32	prevUpdateTime = 0;
	uint32	curTime = CTime::getSecondsSince1970();

	for (int i = 0; i < AUTOPLAY_FID_COUNT; i ++)
	{
		if (AutoPlayers[i].m_Status == CAutoPlayer::STATUS_PLAYING)
			AutoPlayers[i].update();
	}

	if (curTime - prevUpdateTime < 30)	// 30초에 한번씩 실행한다.
		return;
	prevUpdateTime = curTime;

	std::map<uint64, uint8>	AutoPlayReqList;	// RoomID << 32 | ActPos, 1

	for (int i = 0; i < AUTOPLAY_FID_COUNT; i ++)
	{
		if (AutoPlayers[i].m_Status == CAutoPlayer::STATUS_FINISHED)
		{
			AutoPlayers[i].m_Status = CAutoPlayer::STATUS_READY;
		}
		else if (AutoPlayers[i].m_Status == CAutoPlayer::STATUS_FAIL)
		{
//			AutoPlayers[i].m_AutoPlayUnit.m_LastTryTime = curTime;
			AddToAutoPlayUnitList(AutoPlayers[i].m_AutoPlayUnit);
			AutoPlayers[i].m_Status = CAutoPlayer::STATUS_READY;
		}

		if (AutoPlayers[i].m_Status != CAutoPlayer::STATUS_READY)
		{
			uint64	key = AutoPlayers[i].m_AutoPlayUnit.m_RoomID;
			key = (key << 32) | AutoPlayers[i].m_AutoPlayUnit.m_ActPos;
			AutoPlayReqList[key] = 1;
		}
	}

	int	nFreeAutoPlayerCount = 0;
	extern int	nFreeAutoPrayerCount;
	TAutoPlayList::iterator itCur = AutoPlayUnitListForAct.begin();
	for (int i = 0; i < AUTOPLAY_FID_COUNT; i ++)
	{
		if (AutoPlayers[i].m_Status == CAutoPlayer::STATUS_READY)
		{
			nFreeAutoPlayerCount ++;

			while (itCur != AutoPlayUnitListForAct.end())
			{
				uint64	key = itCur->m_RoomID;
				key = (key << 32) | itCur->m_ActPos;
//				if (curTime - itCur->m_LastTryTime >= 60)		// 한번 시도한 후에는 60초내에 다시 시도하지 않는다.
				if (AutoPlayReqList.find(key) == AutoPlayReqList.end())
				{
					AutoPlayReqList[key] = 1;

					AutoPlayers[i].start(*itCur);
					DeleteInfoFromAutoPlayUnitList(*itCur);
					itCur = AutoPlayUnitListForAct.erase(itCur);
					nFreeAutoPlayerCount --;
					break;
				}
				else
					itCur++;
			}
		}
	}

	AutoPlayReqList.clear();

	if (nFreeAutoPlayerCount == 0)
	{
		sipwarning("bIsAutoPlayerFull = true !!! RemainCount=%d", AutoPlayUnitListForAct.size());
	}
	else if (nFreeAutoPrayerCount != 0)
	{
		sipwarning("nFreeAutoPrayerCount != 0. nFreeAutoPrayerCount=%s", nFreeAutoPrayerCount);
	}
}

void OnPacketCallback_One(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		AutoFID;	_msgin.serial(AutoFID);
	if ((AutoFID < AUTOPLAY_FID_START) || (AutoFID >= AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT))
	{
		sipwarning("Invalid AutoFID!!! AutoFID=%d", AutoFID);
		return;
	}
	int index = AutoFID - AUTOPLAY_FID_START;

	switch (_msgin.getName())
	{
	case M_NT_AUTOPLAY_ANS:
		{
			uint32		result;		_msgin.serial(result);
			uint32		roomID;		_msgin.serial(roomID);
			uint16		sceneID;	_msgin.serial(sceneID);
			uint8		roomLevel;	_msgin.serial(roomLevel);
			AutoPlayers[index].on_M_NT_AUTOPLAY_ANS(result, roomID, sceneID, roomLevel);
		}
		break;
	}
}

void OnPacketCallback_List(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	std::list<T_FAMILYID>	AutoFIDs;
	while (true)
	{
		T_FAMILYID		AutoFID;
		_msgin.serial(AutoFID);
		if (AutoFID == 0)
			break;

		if ((AutoFID < AUTOPLAY_FID_START) || (AutoFID >= AUTOPLAY_FID_START + AUTOPLAY_FID_COUNT))
		{
			sipwarning("Invalid AutoFID!!! AutoFID=%d", AutoFID);
			return;
		}
		AutoFIDs.push_back(AutoFID);
	}

	if (AutoFIDs.empty())
		return;
}


//=========================================
// CAutoPlayData Functions
//=========================================
void CAutoPlayData::Init(uint8 _ActPos, uint32 _IsXDG, ucstring *pAutoPlayData0)
{
	m_ActPos = _ActPos;
	m_IsXDG = _IsXDG;

	int		pos1, pos2;
	uint32	time0 = 0, time;
	ucstring strPacketName;

	for (int i = 0; pAutoPlayData0[i][0] != 0; i ++)
	{
		ucstring	pAutoPlayData = pAutoPlayData0[i];
		for (pos1 = 0; pAutoPlayData[pos1] != L','; pos1 ++)
		{
		}
		pAutoPlayData[pos1] = 0;
		// time = _wtoi(&pAutoPlayData[0]);
		time = (uint32)std::wcstod(&pAutoPlayData[0], NULL);
		if (time0 == 0)
			time0 = time;

		for (pos2 = pos1 + 1; pAutoPlayData[pos2] != L','; pos2 ++)
		{
			if (pAutoPlayData[pos2] == 0)
				break;
		}
		pAutoPlayData[pos2] = 0;

		strPacketName = ucstring(&pAutoPlayData[pos1 + 1]);

		if (strPacketName == L"M_CS_ACT_START")
		{
			CAutoPlayPacket_M_CS_ACT_START	*data = new CAutoPlayPacket_M_CS_ACT_START(time - time0);
			data->InitFromString(&pAutoPlayData[pos2 + 1]);
			m_AutoPlayPacketList.push_back(data);
		}
		else if (strPacketName == L"M_CS_ACT_RELEASE")
		{
			CAutoPlayPacket_M_CS_ACT_RELEASE	*data = new CAutoPlayPacket_M_CS_ACT_RELEASE(time - time0);
			m_AutoPlayPacketList.push_back(data);
		}
		else if (strPacketName == L"M_CS_ACT_STEP")
		{
			CAutoPlayPacket_M_CS_ACT_STEP	*data = new CAutoPlayPacket_M_CS_ACT_STEP(time - time0);
			data->InitFromString(&pAutoPlayData[pos2 + 1]);
			m_AutoPlayPacketList.push_back(data);
		}
		else
		{
			sipwarning(L"Invalid Packet Name in AutoPlayData: %s", strPacketName.c_str());
		}
	}
}


//=========================================
// CAutoPlayer Functions
//=========================================
CAutoPlayer::CAutoPlayer(void)
{
}

CAutoPlayer::~CAutoPlayer(void)
{
}

void CAutoPlayer::init(T_FAMILYID _AutoPlayerFID)
{
	m_AutoPlayerFID = _AutoPlayerFID;
	m_Status = STATUS_READY;
}

void CAutoPlayer::start(AutoPlayUnit &data)
{
	m_AutoPlayUnit = data;

	// Send Start Packet To ...
	CMessage	msgOut(M_NT_AUTOPLAY_REQ);
	msgOut.serial(m_AutoPlayUnit.m_RoomID, m_AutoPlayerFID, m_AutoPlayUnit.m_ActPos, m_AutoPlayUnit.m_FID);
	SendMsgToRoomID(m_AutoPlayUnit.m_RoomID, msgOut);

	m_Status = STATUS_CHECKING;
}

void CAutoPlayer::update()
{
	if (m_Status != STATUS_PLAYING)
		return;

	uint32 diffTime = (uint32)(CTime::getLocalTime() - m_StartTime);

	for (; m_CurPacketIndex < m_pAutoPlayData->m_AutoPlayPacketList.size(); m_CurPacketIndex ++)
	{
		CAutoPlayPacketBase	*packet_data = m_pAutoPlayData->m_AutoPlayPacketList[m_CurPacketIndex];
		if (packet_data->m_Time > diffTime)
			break;
		CMessage	msgOut(packet_data->m_PacketName);
		msgOut.serial(m_AutoPlayerFID);
		packet_data->serial(msgOut, m_AutoPlayUnit.m_ItemID, NULL); // m_pDeskName);
		SendMsgToRoomID(m_AutoPlayUnit.m_RoomID, msgOut);
	}

	if (m_CurPacketIndex >= m_pAutoPlayData->m_AutoPlayPacketList.size())
	{
		// Notice Owner that Autoplay finished.
		Player *pPlayer = GetPlayer(m_AutoPlayUnit.m_FID);
		if (pPlayer != NULL)
		{
			uint32	status = AUTOPLAY_STATUS_END;
			CMessage	msgOut1(M_SC_AUTOPLAY_STATUS);
			msgOut1.serial(m_AutoPlayUnit.m_FID, m_AutoPlayUnit.m_RoomID, m_AutoPlayUnit.m_ActPos, status);
			CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut1);
		}

		CMessage	msgOut(M_NT_AUTOPLAY_END);
		msgOut.serial(m_AutoPlayUnit.m_RoomID, m_AutoPlayerFID, m_AutoPlayUnit.m_ActPos);
		SendMsgToRoomID(m_AutoPlayUnit.m_RoomID, msgOut);
		m_Status = STATUS_FINISHED; // 원래는 STATUS_READY였지만 UpdateAutoPlayers함수에서 통합관리를 하기 위하여 STATUS_FINISHED로 설정한다.

		//sipdebug("Packet Send Finished. AutoFID=%d", m_AutoPlayerFID); byy krs
	}
}

void CAutoPlayer::on_M_NT_AUTOPLAY_ANS(uint32 result, uint32 roomID, uint16 sceneID, uint8 roomLevel)
{
	//sipdebug("AutoFID=%d, result=%d, roomID=%d, sceneID=%d", m_AutoPlayerFID, result, roomID, sceneID); byy krs

	if (result != 0)
	{
		m_Status = STATUS_FAIL;
		return;
	}

	m_Status = STATUS_PLAYING;
	m_SceneID = sceneID;
	m_RoomLevel = roomLevel;

	m_StartTime = CTime::getLocalTime();
	m_CurPacketIndex = 0;

	// m_pAutoPlayData
	int i = 0;
	for (i = 0; i < AutoPlayerDatasCount; i ++)
	{
		if ((AutoPlayerDatas[i].m_ActPos == m_AutoPlayUnit.m_ActPos) && (AutoPlayerDatas[i].m_IsXDG == m_AutoPlayUnit.m_IsXDG))
			break;
	}
	if (i >= AutoPlayerDatasCount)
	{
		sipwarning("it == AutoPlayerDatas.end(). ActPos=%d, IsXDG=%d", m_AutoPlayUnit.m_ActPos, m_AutoPlayUnit.m_IsXDG);
		m_Status = STATUS_FAIL;
		return;
	}
	m_pAutoPlayData = &AutoPlayerDatas[i];

	// Send Start Packet To OpenroomService
	CMessage	msgOut(M_NT_AUTOPLAY_START);
	msgOut.serial(m_AutoPlayUnit.m_RoomID, m_AutoPlayerFID, m_AutoPlayUnit.m_ActPos, m_AutoPlayUnit.m_FID, m_AutoPlayUnit.m_FamilyName);
	msgOut.serial(m_AutoPlayUnit.m_ModelID, m_AutoPlayUnit.m_DressID, m_AutoPlayUnit.m_FaceID, m_AutoPlayUnit.m_HairID);
	SendMsgToRoomID(m_AutoPlayUnit.m_RoomID, msgOut);

	// Notice Owner that Autoplay stated.
	Player *pPlayer = GetPlayer(m_AutoPlayUnit.m_FID);
	if (pPlayer != NULL)
	{
		uint32	status = AUTOPLAY_STATUS_START;
		CMessage	msgOut1(M_SC_AUTOPLAY_STATUS);
		msgOut1.serial(m_AutoPlayUnit.m_FID, m_AutoPlayUnit.m_RoomID, m_AutoPlayUnit.m_ActPos, status);
		CUnifiedNetwork::getInstance()->send(pPlayer->m_FSSid, msgOut1);
	}
}

void testAutoplay(uint32 RoomID, uint8 ActPos, uint32 IsXDG)
{
	T_FAMILYID		FID = 7133;
	T_FAMILYNAME	FamilyName = L"123";
	T_MODELID		ModelID = 4;
	T_DRESS			DressID = 404;
	T_FACEID		FaceID = 404;
	T_HAIRID		HairID = 404;
	ucstring		Pray = L"Pray Text";
	uint8			SecretFlag = 0;
	T_ID			ItemID[9];
	T_ID			ItemID_1[9] = {700305, 700329, 700305, 700329, 700305, 700329, 700126, 700017, 700211};
	T_ID			ItemID_1_XDG[9] = {700304, 700304, 700340, 700340, 700320, 700320, 700128, 700372, 700390};
	T_ID			ItemID_3[6] = {730020, 730021, 730023, 730006, 730022, 730024};
	T_ID			UsedItemID;
	if (ActPos == ACTPOS_YISHI)
	{
		if (!IsXDG)
		{
			memcpy(ItemID, ItemID_1, sizeof(ItemID_1));
			UsedItemID = 700501;
		}
		else
		{
			memcpy(ItemID, ItemID_1_XDG, sizeof(ItemID_1_XDG));
			UsedItemID = 700470;
		}
	}
	else if (ActPos == ACTPOS_XIANBAO)
	{
		memcpy(ItemID, ItemID_3, sizeof(ItemID_3));
		UsedItemID = 730030;
	}
	else
		return;

	extern	TAutoPlayList	AutoPlayUnitListTmp;
	AutoPlayUnit	data(FID, FamilyName, ModelID, DressID, FaceID, HairID, RoomID, ActPos, IsXDG, Pray, SecretFlag, ItemID, UsedItemID, 1);
	data.m_UnitID = 0;
	AutoPlayUnitListForRegister.push_back(data);

	//sipinfo("AutoPlayUnitListForRegister.size() = %d", AutoPlayUnitListForRegister.size()); byy krs

	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_AUTOPLAY, data.m_UnitID, FID))
		RegisterActionMemo(data.m_UnitID);
}

/////////////////////////////////////////////////////
//    예약의식
/////////////////////////////////////////////////////
// AutoPlay3이 시간이 되여 시작하라는것을 통지한다. LS->WS->MS 
// ([d:YishiTimeMin][d:Autoplay3ID][d:RoomID][d:FID][u:FamilyName][d:ModelID][d:DressID][d:FaceID][d:HairID][d:CountPerDay][d:YishiTaocanItemID][d:XianbaoTaocanItemID][d:YangyuItemID][d:ItemID_Tian][d:ItemID_Di])
void cb_M_NT_AUTOPLAY3_START_REQ(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32	YishiTimeMin, Autoplay3ID, RoomID, FID, ModelID, DressID, FaceID, HairID, CountPerDay, YishiTaocanItemID, XianbaoTaocanItemID, YangyuItemID, TianItemID, DiItemID;
	ucstring	FamilyName;

	_msgin.serial(YishiTimeMin, Autoplay3ID, RoomID, FID, FamilyName, ModelID, DressID, FaceID, HairID);
	_msgin.serial(CountPerDay, YishiTaocanItemID, XianbaoTaocanItemID, YangyuItemID, TianItemID, DiItemID);

	uint32	firstUnitID = snUnitID;

	ucstring	emptyStr = L"";
	// 행사: 물주기, 꽃드리기, 길상등드리기, 물고기먹이주기, 천수의식, 물건태우기
	{
		// 행사: 물주기, 꽃드리기, 길상등드리기, 물고기먹이주기 - ACTPOS_XINGLI_1로 일괄처리한다.
		uint32			ItemID[9] = {0};
		ItemID[0] = CountPerDay;
		AutoPlayUnit	data(FID, FamilyName, YishiTimeMin, Autoplay3ID, 0, 0, RoomID, ACTPOS_XINGLI_1, 0, emptyStr, 0, ItemID, YangyuItemID, 0);
		data.m_UnitID = snUnitID;		snUnitID ++;
		AutoPlayUnitListForRegister.push_back(data);

		//sipinfo("AutoPlayUnitListForRegister.size()=%d, UnitID=%d, Autoplay3ID=%d, Other", AutoPlayUnitListForRegister.size(), snUnitID - 1, Autoplay3ID); byy krs
	}

	if (YishiTaocanItemID != 0)
	{
		// 천수의식 - CountPerDay개수만큼.
		uint32	isXDG = 0xFF;
		OUT_TAOCAN_FOR_AUTOPLAY3*	pTaocanItemInfo = GetAutoplay3TaocanItemInfo(YishiTaocanItemID);
		if (pTaocanItemInfo == NULL)
		{
			sipwarning("Invalid Taocan Item. FID=%d, YishiTaocanItemID=%d", FID, YishiTaocanItemID);
		}
		else
		{
			if (pTaocanItemInfo->TaocanType == SACRFTYPE_TAOCAN_ROOM)
			{
				isXDG = 0;
			}
			else if (pTaocanItemInfo->TaocanType == SACRFTYPE_TAOCAN_ROOM_XDG)
			{
				isXDG = 1;
			}
			else
			{
				sipwarning("pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_ROOM!!! FID=%d, YishiTaocanItemID=%d", FID, YishiTaocanItemID);
			}
		}

		if (isXDG <= 1)
		{
			for (uint32 i = 0; i < CountPerDay; i ++)
			{
				AutoPlayUnit	data(FID, FamilyName, ModelID, DressID, FaceID, HairID, RoomID, ACTPOS_YISHI, isXDG, pTaocanItemInfo->PrayText, 0, pTaocanItemInfo->ItemIDs, YishiTaocanItemID, MONEYTYPE_UMONEY);
				data.m_UnitID = snUnitID;		snUnitID ++;
				AutoPlayUnitListForRegister.push_back(data);

				//sipinfo("AutoPlayUnitListForRegister.size()=%d, UnitID=%d, Autoplay3ID=%d, Yishi", AutoPlayUnitListForRegister.size(), snUnitID - 1, Autoplay3ID); byy krs
			}
		}
	}

	if (XianbaoTaocanItemID != 0)
	{
		// 물건태우기 - CountPerDay개수만큼.
		uint32	isXDG = 0xFF;
		OUT_TAOCAN_FOR_AUTOPLAY3*	pTaocanItemInfo = GetAutoplay3TaocanItemInfo(XianbaoTaocanItemID);
		if (pTaocanItemInfo == NULL)
		{
			sipwarning("Invalid Taocan Item. FID=%d, XianbaoTaocanItemID=%d", FID, XianbaoTaocanItemID);
		}
		else
		{
			if (pTaocanItemInfo->TaocanType == SACRFTYPE_TAOCAN_XIANBAO)
			{
				isXDG = 0;
			}
			else
			{
				sipwarning("pTaocanItemInfo->TaocanType != SACRFTYPE_TAOCAN_XIANBAO!!! FID=%d, XianbaoTaocanItemID=%d", FID, XianbaoTaocanItemID);
			}
		}

		if (isXDG <= 1)
		{
			for (uint32 i = 0; i < CountPerDay; i ++)
			{
				AutoPlayUnit	data(FID, FamilyName, ModelID, DressID, FaceID, HairID, RoomID, ACTPOS_XIANBAO, isXDG, pTaocanItemInfo->PrayText, 0, pTaocanItemInfo->ItemIDs, XianbaoTaocanItemID, MONEYTYPE_UMONEY);
				data.m_UnitID = snUnitID;		snUnitID ++;
				AutoPlayUnitListForRegister.push_back(data);

				//sipinfo("AutoPlayUnitListForRegister.size()=%d, UnitID=%d, Autoplay3ID=%d, Xianbao", AutoPlayUnitListForRegister.size(), snUnitID - 1, Autoplay3ID); byy krs
			}
		}
	}

	if ((TianItemID != 0) || (DiItemID != 0))
	{
		ucstring	TianPray = L"", DiPray = L"";
		if (TianItemID != 0)
		{
			OUT_TAOCAN_FOR_AUTOPLAY3*	pTaocanItemInfo = GetAutoplay3TaocanItemInfo(TianItemID);
			if (pTaocanItemInfo == NULL)
			{
				sipwarning("Invalid Taocan Item. FID=%d, TianItemID=%d", FID, TianItemID);
			}
			else
			{
				TianPray = pTaocanItemInfo->PrayText;
			}
		}
		if (DiItemID != 0)
		{
			OUT_TAOCAN_FOR_AUTOPLAY3*	pTaocanItemInfo = GetAutoplay3TaocanItemInfo(DiItemID);
			if (pTaocanItemInfo == NULL)
			{
				sipwarning("Invalid Taocan Item. FID=%d, DiItemID=%d", FID, DiItemID);
			}
			else
			{
				DiPray = pTaocanItemInfo->PrayText;
			}
		}
		CMessage	msgOut(M_NT_AUTOPLAY3_TIANDI);
		msgOut.serial(FID, FamilyName, TianItemID, TianPray, DiItemID, DiPray, CountPerDay);
		CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, msgOut);
	}

	// 시작
	if (CreateOpenRoom(RoomID, RoomCreateWait::CREATE_ROOM_REASON_AUTOPLAY, firstUnitID, FID))
		RegisterActionMemo(firstUnitID);
}

// 예약행사에 의한 경험치증가 통지 LS->WS->MS->OROOM ([d:FID][d:YishiTimeMin][d:Autoplay3ID][d:RoomID][u:RoomName][d:CountPerDay][d:YishiTaocanItemID][d:XianbaoTaocanItemID][d:YangyuItemID][d:ItemID_Tian][d:ItemID_Di][d:Param])
void cb_M_NT_AUTOPLAY3_EXP_ADD_ONLINE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	_msgin.serial(FID);

	Player*	pPlayer = GetPlayer(FID);
	if (pPlayer == NULL)
	{
		sipwarning("GetPlayer = NULL. FID=%d", FID);
		return;
	}

	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(pPlayer->m_RoomSid, _msgin);
}

// 예약행사에 의한 경험치증가 통지 LS->OwnWS->MS->OROOM ([d:FID][d:YishiTimeMin][d:Autoplay3ID][d:RoomID][u:RoomName][d:CountPerDay][d:YishiTaocanItemID][d:XianbaoTaocanItemID][d:YangyuItemID][d:ItemID_Tian][d:ItemID_Di][d:Param])
void cb_M_NT_AUTOPLAY3_EXP_ADD_OFFLINE(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	extern TServiceId	lobbySidForAutoplay3;

	if (lobbySidForAutoplay3.get() != 0)
	{
		_msgin.invert();
		CUnifiedNetwork::getInstance()->send(lobbySidForAutoplay3, _msgin);
	}
	else
	{
		sipwarning("lobbySidForAutoplay3.get() = 0");
	}
}

// Param = 1 : M_NT_ROOM_CREATED failed
//			0xFFFFFFFE : Autoplay3 - M_NT_ADDACTIONMEMO_OK failed (m_bClosed)
//			0xFFFFFFFF : Autoplay3 - M_NT_ADDACTIONMEMO_OK failed (방의 사용기간 초과)
void RemoveAutoplayDatas(uint32 RoomID, uint32 Param)
{
	bool	bIsAutoplayer3 = false;
	for (TAutoPlayList::iterator it = AutoPlayUnitListForRegister.begin(); it != AutoPlayUnitListForRegister.end(); )
	{
		if (it->m_RoomID != RoomID)
		{
			it ++;
			continue;
		}

		if (it->m_ActPos == ACTPOS_XINGLI_1)
			bIsAutoplayer3 = true;

		sipwarning("Autoplay Failed, UnitID=%d, RoomID=%d, FID=%d, ActPos=%d, Reason=%d. AutoPlayUnitListForRegister", it->m_UnitID, it->m_RoomID, it->m_FID, it->m_ActPos, Param);

		it = AutoPlayUnitListForRegister.erase(it);
	}

	//sipinfo("AutoPlayUnitListForRegister.size()=%d", AutoPlayUnitListForRegister.size()); byy krs

	if (bIsAutoplayer3 && (Param != 99999999))	// when Param=99999999, it's DB error so need retry
	{
		if (Param == 0xFFFFFFFF)
			Param = 2;
		else
			Param = 1;

		CMessage	msgOut(M_NT_AUTOPLAY3_FAIL);
		msgOut.serial(RoomID, Param);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}
}
