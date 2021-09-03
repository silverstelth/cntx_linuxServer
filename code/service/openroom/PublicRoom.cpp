#include <misc/ucstring.h>
#include <net/service.h>

#include "PublicRoom.h"
#include "openroom.h"
#include "Character.h"
#include "Family.h"
#include "mst_room.h"
#include "outRoomKind.h"
#include "room.h"
#include "contact.h"
#include "Lobby.h"
#include "Inven.h"
#include "ChannelWorld.h"

#include "../Common/DBLog.h"
#include "../Common/netCommon.h"
#include "misc/DBCaller.h"
#include "ParseArgs.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller	*DBCaller;

enum	E_LARGEACT_STATE
{
	LARGEACT_STATE_WAIT,
	LARGEACT_STATE_PREPAREING,
	LARGEACT_STATE_DOING,
	LARGEACT_STATE_END,
};

#define		ACTENDSTEP					100

bool CreatePublicRoom(T_ROOMID RoomID)
{
	uint16	SceneID = GetReligionRoomSceneID(RoomID);
	if (SceneID == 0)
	{
		sipwarning("Invalid ReligionRoomID. RoomID=%d", RoomID);
		return false;
	}

	CPublicRoomWorld	*pRoom = new CPublicRoomWorld(RoomID);
	if (pRoom == NULL)
	{
		sipwarning("new CPublicRoomWorld = NULL.");
		return false;
	}

	pRoom->m_SceneID = SceneID;
	pRoom->InitMasterDataValues();

	pRoom->InitPublicRoom();

	for (uint32 iChannel = 1; iChannel <= pRoom->m_nDefaultChannelCount; iChannel ++)
	{
		pRoom->CreateChannel(iChannel);
		//채널창조 by krs
		pRoom->CreateChannel(iChannel + ROOM_UNITY_CHANNEL_START);
//		PublicRoomChannelData	data(pRoom, COUNT_PUBLIC_ACTPOS, RoomID, iChannel, pRoom->m_SceneID);
//		pRoom->m_mapChannels.insert(make_pair(iChannel, data));
	}

	{
		TChannelWorld	rw(pRoom);
		map_RoomWorlds.insert(make_pair(RoomID, rw));
	}

	pRoom->ReloadYiSiResultForToday();
	pRoom->SetRoomEventStatus(ROOMEVENT_STATUS_NONE);
	pRoom->SetRoomEvent();

	pRoom->RefreshTodayLuckList();

	pRoom->InitSharedActWorld();
/*	// FS에 통지한다.
	{
		uint8	flag = 1;	// Room Created
		CMessage	msgOut(M_NT_ROOM);
		msgOut.serial(RoomID, flag);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut, false);
	}

	// ManagerService에 통지한다.
	{
		ucstring	RoomName = pRoom->m_RoomName;
		uint32		MasterFID = 0, Result = 0;
		CMessage	msgOut(M_NT_ROOM_CREATED);
		msgOut.serial(RoomID, Result, SceneID, RoomName, MasterFID);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
	}

	TTime	curTime = GetDBTimeSecond();
	NextReligionCheckEveryWorkTimeSecond = GetUpdateNightTime(curTime);*/

	//sipinfo("Public Room Created. RoomID=%d", RoomID); byy krs

	return true;
}

static void	DBCB_DBGetFishFlowerData(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet);
static void	DBCB_DBGetLargeActList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet);

CVariable<uint32>	LargeActUserMaxNum("oroom","LargeActUserMaxNum", "LargeActUserMaxNum", 0, 0, true );

inline uint32 GetLargeActUserMaxNum()
{
	uint32 un = LargeActUserMaxNum.get();
	if (un == 0)
		return MAX_LARGEACT_USERCOUNT;
	return un;
}

void CPublicRoomWorld::InitPublicRoom()
{
	// 꽃/물고기정보를 얻는다.
	{
		MAKEQUERY_GetFishFlowerData(strQuery, m_RoomID);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetFishFlowerData,
			"D",
			CB_,		m_RoomID);
	}

	// 대형행사자료를 얻는다.
	{
		m_nLargeActCount = 0;
		MAKEQUERY_GetLargeActList(strQuery, m_RoomID);
		DBCaller->executeWithParam(strQuery, DBCB_DBGetLargeActList,
			"D",
			CB_,		m_RoomID);
	}

	InitLargeAct();
}

static CPublicRoomWorld* GetPublicWorld(T_ROOMID _rid)
{
	CChannelWorld* pChannelWorld = GetChannelWorld(_rid);
	if (pChannelWorld == NULL)
		return NULL;

	if (pChannelWorld->m_RoomID == ROOM_ID_PUBLICROOM || pChannelWorld->m_RoomID == ROOM_ID_KONGZI || pChannelWorld->m_RoomID == ROOM_ID_HUANGDI)
		return dynamic_cast<CPublicRoomWorld*>(pChannelWorld);

	return NULL;
}

static CPublicRoomWorld* GetPublicWorldFromFID(T_FAMILYID _fid)
{
	CChannelWorld* pChannelWorld = GetChannelWorldFromFID(_fid);
	if (pChannelWorld == NULL)
		return NULL;

	if (pChannelWorld->m_RoomID == ROOM_ID_PUBLICROOM || pChannelWorld->m_RoomID == ROOM_ID_KONGZI || pChannelWorld->m_RoomID == ROOM_ID_HUANGDI)
		return dynamic_cast<CPublicRoomWorld*>(pChannelWorld);

	return NULL;
}

static void	DBCB_DBGetFishFlowerData(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			RRoomID	= *(uint32 *)argv[0];

	if (!nQueryResult)
		return;

	CPublicRoomWorld *roomManager = GetPublicWorld(RRoomID);
	if(roomManager == NULL)
	{
		sipwarning("GetPublicWorld = NULL. RoomID=%d", RRoomID);
		return;
	}

	uint32	nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		uint8	FishLevel;	resSet->serial(FishLevel);
		resSet->serial(roomManager->m_FishExp);
		resSet->serial(roomManager->m_FlowerID);
		resSet->serial(roomManager->m_FlowerLevel);
		resSet->serial(roomManager->m_FlowerDelTime);
		resSet->serial(roomManager->m_FlowerFID);
		resSet->serial(roomManager->m_FlowerFamilyName);
	}
	else
	{
		roomManager->m_FishExp = 0;
		roomManager->m_FlowerID = 0;
		roomManager->m_FlowerLevel = 0;
		roomManager->m_FlowerDelTime = 0;
		roomManager->m_FlowerFID = 0;
		roomManager->m_FlowerFamilyName = L"";
	}
}

static void	DBCB_DBGetLargeActList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32			RRoomID	= *(uint32 *)argv[0];

	if (!nQueryResult)
		return;

	uint32	dataCount;
	uint32	bufsize;
	uint8	buf[MAX_LARGEACT_ITEMCOUNT * 4 + 10];
	uint8	bufPrize[120];
	uint8	bufVips[512];
	SIPBASE::MSSQLTIME	tempTime;
	string	AcceptTime;
	string	StartTime;

	resSet->serial(dataCount);
	CPublicRoomWorld *inManager = GetPublicWorld(RRoomID);
	if(inManager == NULL)
	{
		sipwarning("GetPublicWorld = NULL. RoomID=%d", RRoomID);
		return;
	}

	inManager->m_nLargeActCount = 0;
	LargeAct*	pLargeAct = inManager->m_LargeActs;
	for (uint32 i = 0; i < dataCount; i ++)
	{
		int nLargeActCount = inManager->m_nLargeActCount;
		pLargeAct[nLargeActCount].Init();

		resSet->serial(pLargeAct[nLargeActCount].ActID);
		resSet->serial(pLargeAct[nLargeActCount].Title);
		resSet->serial(pLargeAct[nLargeActCount].Comment);
		resSet->serial(pLargeAct[nLargeActCount].FlagText);
		resSet->serial(pLargeAct[nLargeActCount].ActMean);
		resSet->serial(pLargeAct[nLargeActCount].Option);
		resSet->serial(AcceptTime);	tempTime.SetTime(AcceptTime);	pLargeAct[nLargeActCount].AcceptTime = (uint32)(tempTime.timevalue / 60);
		resSet->serial(StartTime);	tempTime.SetTime(StartTime);	pLargeAct[nLargeActCount].StartTime = (uint32)(tempTime.timevalue / 60);

		resSet->serial(bufsize);	resSet->serialBuffer(buf, bufsize);	// ItemIDs
		if (bufsize != 4 * MAX_LARGEACT_ITEMCOUNT)
		{
			sipwarning("bufsize != 4 * MAX_LARGEACT_ITEMCOUNT. i=%d", i);
			continue;
		}
		for (int j = 0; j < MAX_LARGEACT_ITEMCOUNT; j ++)
		{
			pLargeAct[nLargeActCount].ItemIDs[j] = *((uint32*)(&buf[j * 4]));
		}

		resSet->serial(bufsize);	resSet->serialBuffer(buf, bufsize);	// PhotoIDs
		if (bufsize != 4 * MAX_LARGEACT_PHOTOCOUNT)
		{
			sipwarning("bufsize != 4 * MAX_LARGEACT_PHOTOCOUNT. i=%d", i);
			continue;
		}
		for (int j = 0; j < MAX_LARGEACT_PHOTOCOUNT; j ++)
		{
			pLargeAct[nLargeActCount].PhotoIDs[j] = *((uint32*)(&buf[j * 4]));
		}

		resSet->serial(pLargeAct[nLargeActCount].UserCount);
		// Prize info
		resSet->serial(bufsize);	resSet->serialBuffer(bufPrize, bufsize);
		if (bufsize >= 6)
		{
			pLargeAct[nLargeActCount].PrizeBlessMode = LARGEACTBLESSPRIZE_WHOLE;

			uint8 curpos = 0;
			for (int i = 0; i < MAX_BLESSCARD_ID; i++)
			{
				pLargeAct[nLargeActCount].PrizeBless[i] = *((uint8*)(&bufPrize[curpos]));	curpos++;
				//sipdebug("Load actid=%d: bless%d=%d", pLargeAct[nLargeActCount].ActID, (i+1), pLargeAct[nLargeActCount].PrizeBless[i]); byy krs
			}		
			pLargeAct[nLargeActCount].PrizeItemMode = LARGEACTITEMPRIZE_MODE(*((uint8*)(&bufPrize[curpos])));	curpos++;
			pLargeAct[nLargeActCount].PrizeItemNum = *((uint8*)(&bufPrize[curpos]));	curpos++;
			//sipdebug("Load actid=%d: Give mode=%d, ItemTypeNum=%d", pLargeAct[nLargeActCount].ActID, 
			//	pLargeAct[nLargeActCount].PrizeItemMode,
				//pLargeAct[nLargeActCount].PrizeItemNum); byy krs
			
			for (int i = 0; i < pLargeAct[nLargeActCount].PrizeItemNum; i++)
			{
				pLargeAct[nLargeActCount].PrizeInfo[i].ItemID = *((uint32*)(&bufPrize[curpos]));	curpos += 4;
				pLargeAct[nLargeActCount].PrizeInfo[i].ItemNum = *((uint16*)(&bufPrize[curpos]));	curpos += 2;
				//sipdebug("Load actid=%d: ItemID(%d)=%d", pLargeAct[nLargeActCount].ActID,
				//	pLargeAct[nLargeActCount].PrizeInfo[i].ItemID,
				//	pLargeAct[nLargeActCount].PrizeInfo[i].ItemNum); byy krs
			}
		}
		else
		{
			sipwarning("There is no largeact prize info actid=%d", pLargeAct[nLargeActCount].ActID);
		}
		// Vip info
		resSet->serial(bufsize);	resSet->serialBuffer(bufVips, bufsize);
		if (bufsize > 0)
		{
			uint8 curpos = 0;
			pLargeAct[nLargeActCount].VIPNum = *((uint8*)(&bufVips[curpos]));	curpos++;
			for (int vi = 0; vi < pLargeAct[nLargeActCount].VIPNum; vi++)
			{
				pLargeAct[nLargeActCount].VIPS[vi].FID = *((uint32*)(&bufVips[curpos]));	curpos += 4;
				//sipdebug("Load actid=%d: VipFID=%d", pLargeAct[nLargeActCount].ActID, pLargeAct[nLargeActCount].VIPS[vi].FID); byy krs
			}
		}
		else
		{
			sipwarning("There is no largeact vip info actid=%d", pLargeAct[nLargeActCount].ActID);
		}
		resSet->serial(pLargeAct[nLargeActCount].OpeningNotice);
		resSet->serial(pLargeAct[nLargeActCount].EndingNotice);

		inManager->m_nLargeActCount ++;
	}

	map<T_FAMILYID, T_FAMILYNAME>	nameBuf;
	for (uint32 i = 0; i < (uint32)inManager->m_nLargeActCount; i++)
	{
		LargeAct &curAct = pLargeAct[i];
		if (curAct.VIPNum == 0)
			continue;
		for (uint8 k = 0; k < curAct.VIPNum; k++)
		{
			T_FAMILYID fid = curAct.VIPS[k].FID;
			map<T_FAMILYID, T_FAMILYNAME>::iterator it = nameBuf.find(fid);
			if (it != nameBuf.end())
			{
				curAct.VIPS[k].FName = it->second;
				continue;
			}
			MAKEQUERY_FINDFAMILYWITHID(strQ, fid);
			CDBConnectionRest	DB(DBCaller);
			DB_EXE_PREPARESTMT(DB, Stmt, strQ);
			if (!nPrepareRet)
			{
				sipwarning("Shrd_Family_findFID: prepare statement failed!");
				continue;
			}
			SQLLEN len1 = 0;
			int	ret = 0;
			DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);
			if (!nBindResult)
			{
				sipwarning("Shrd_Family_findFID: bind failed!");
				continue;
			}
			DB_EXE_PARAMQUERY(Stmt, strQ);
			if (!nQueryResult)
			{
				sipwarning("Shrd_Family_findFID: execute failed!");
				continue;
			}
			if (ret != 0)
			{
				sipwarning("Shrd_Family_findFID: ret != 0");
				continue;
			}
			else
			{
				DB_QUERY_RESULT_FETCH(Stmt, row);
				if (!IS_DB_ROW_VALID(row))
				{
					sipwarning("Shrd_Family_findFID: return empty!");
					continue;
				}
				COLUMN_WSTR(row, 0, fname, MAX_LEN_FAMILY_NAME);
				nameBuf[fid] = fname;
				curAct.VIPS[k].FName = fname;
			}
		}
	}
}

void CPublicRoomWorld::ClearLargeActUserList()
{
	m_CurrentLargeActUserList.clear();

	for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		pChannelData->m_LargeActUserList.clear();
		pChannelData->m_nCurrentLargeActStep = 0;
	}
}

void CPublicRoomWorld::InitLargeAct()
{
	m_nCurrentLargeActIndex = -1;
	m_nCurrentLargeActStatus = LARGEACT_STATE_WAIT;
	ClearLargeActUserList();
}

void SendLargeActList(T_FAMILYID fid, SIPNET::TServiceId _tsid)
{
	CPublicRoomWorld *inManager;
	inManager = GetPublicWorld(ROOM_ID_PUBLICROOM);
	if(inManager != NULL)
	{
		inManager->SendLargeActList(fid, _tsid);
	}
	inManager = GetPublicWorld(ROOM_ID_KONGZI);
	if(inManager != NULL)
	{
		inManager->SendLargeActList(fid, _tsid);
	}
	inManager = GetPublicWorld(ROOM_ID_HUANGDI);
	if(inManager != NULL)
	{
		inManager->SendLargeActList(fid, _tsid);
	}
}

// Request LargeAct list.  by krs
//void cb_M_CS_LARGEACT_LIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;
//	_msgin.serial(FID);
//	sipdebug("$$$$$ LargAct_List  FID=%d  $$$$$$$$$$$$", FID);
//	SendLargeActList(FID, _tsid);
//}
void CPublicRoomWorld::SendLargeActList(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	uint8	flagStart, flagEnd;

	if (m_nLargeActCount == 0)
	{
		flagStart = 1;
		flagEnd = 1;
		CMessage	msgOut(M_SC_LARGEACT_LIST);
		msgOut.serial(FID, m_RoomID, flagStart, flagEnd);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
	else
	{
		FAMILY*	pFamily = GetFamilyData(FID);
		PublicRoomChannelData	*pChannelData = GetFamilyChannel(FID);
		for (int i = 0; i < m_nLargeActCount; i ++)
		{
			flagStart = (i == 0) ? 1 : 0;
			flagEnd = (i + 1 < m_nLargeActCount) ? 0 : 1;

			uint32	user_count = m_LargeActs[i].UserCount;
			if (pChannelData)
			{
				if (m_nCurrentLargeActIndex == i &&
					(m_nCurrentLargeActStatus == LARGEACT_STATE_WAIT ||	m_nCurrentLargeActStatus == LARGEACT_STATE_PREPAREING))
					user_count = pChannelData->m_LargeActUserList.size();
			}
			CMessage	msgOut(M_SC_LARGEACT_LIST);
			msgOut.serial(FID, m_RoomID, flagStart, m_LargeActs[i].ActID, m_LargeActs[i].Title, m_LargeActs[i].Comment, m_LargeActs[i].FlagText, m_LargeActs[i].ActMean);
			msgOut.serial(m_LargeActs[i].AcceptTime, m_LargeActs[i].StartTime, m_LargeActs[i].PhotoIDs[0], user_count);
			
			if (pFamily != NULL && pFamily->m_bGM)
			{
				for (int j = 0; j < MAX_BLESSCARD_ID; j++)
				{
					msgOut.serial(m_LargeActs[i].PrizeBless[j]);
				}
				uint8 PrizeItemMode = m_LargeActs[i].PrizeItemMode;
				uint8 PrizeItemNum = m_LargeActs[i].PrizeItemNum;
				msgOut.serial(PrizeItemMode, PrizeItemNum);
				for (int k = 0; k < PrizeItemNum; k++)
				{
					msgOut.serial(m_LargeActs[i].PrizeInfo[k].ItemID, m_LargeActs[i].PrizeInfo[k].ItemNum);
				}

				msgOut.serial(m_LargeActs[i].VIPNum);
				for (int m = 0; m < m_LargeActs[i].VIPNum; m++)
				{
					msgOut.serial(m_LargeActs[i].VIPS[m].FID);
					msgOut.serial(m_LargeActs[i].VIPS[m].FName);
				}
				msgOut.serial(m_LargeActs[i].OpeningNotice, m_LargeActs[i].EndingNotice);
			}

			msgOut.serial(flagEnd);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}
	}
}

#define		SEND_LARGEACTSETRESULT(bResult)		\
	bool	bR = bResult;						\
	CMessage	msgactresult(M_GMSC_LARGEACT_SET);	\
	msgactresult.serial(FID, bR);	\
	CUnifiedNetwork::getInstance()->send(_tsid, msgactresult);	\

static void	DBCB_DBInsertLargeAct(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32		ActID	= *(uint32 *)argv[0];
	uint32		RRoomID	= *(uint32 *)argv[1];
	uint32		ActIndex= *(uint32 *)argv[2];
	uint32		FID		= *(uint32 *)argv[3];
	uint32		currentActID = *(uint32 *)argv[4];
	TServiceId	_tsid(*(uint16 *)argv[5]);

	if (!nQueryResult)
		return;

	CPublicRoomWorld *inManager = GetPublicWorld(RRoomID);
	if(inManager == NULL)
	{
		sipwarning("GetPublicWorld = NULL. RoomID=%d", RRoomID);
		return;
	}
	LargeAct*	pLargeAct = inManager->m_LargeActs;
	pLargeAct[ActIndex].ActID = ActID;

	{
		CMessage	msgOut(M_GMSC_LARGEACT_NEWID);
		msgOut.serial(FID, ActID);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	{
		CMessage	msgNotify(M_SC_LARGEACT_INFOCHANGE);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgNotify);

		SEND_LARGEACTSETRESULT(true);
	}

	if (inManager->m_nCurrentLargeActStatus != LARGEACT_STATE_DOING)
	{
		uint32  nCandiActID = 0;
		int		nCandiIndex = inManager->GetCurrentCandiActIndex();
		if (nCandiIndex != -1)
			nCandiActID = pLargeAct[nCandiIndex].ActID;

		if (currentActID == nCandiActID)
		{
			inManager->m_nCurrentLargeActIndex = nCandiIndex;
		}
		else
		{
			inManager->ChangeCurrentLargeAct(nCandiIndex);
		}
	}
}

// Insert or Modify LargeAct ([d:RoomID][d:ActID,0=insert,other=modify][u:title][u:comment][d:option][M1970:accepttime][M1970:starttime][20x[itemids]][7x[photoids]])
void cb_M_GMCS_LARGEACT_SET(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		RoomID, ActID, Option, AcceptTimeM, StartTimeM;
	ucstring	Title, Comment, FlagText, ActMean;
	uint32		ItemIDs[MAX_LARGEACT_ITEMCOUNT], PhotoIDs[MAX_LARGEACT_PHOTOCOUNT];
	tchar		str_buf_itemid[MAX_LARGEACT_ITEMCOUNT * 8 + 10];
	tchar		str_buf_photoid[MAX_LARGEACT_PHOTOCOUNT * 8 + 10];
	
	uint8		PrizeBlessMode = LARGEACTITEMPRIZE_WHOLE;
	uint8		PrizeBless[MAX_BLESSCARD_ID];
	uint8		PrizeItemMode;
	uint8		PrizeItemNum;
	TLargeActPrizeItemInfo	PrizeItemInfo[MAX_LARGEACTPRIZENUM];
	ucchar		str_buf_prize[MAX_BLESSCARD_ID*2 + 2+2+ MAX_LARGEACTPRIZENUM * (8+4) + 10];
	uint8		VIPNum;
	TLargeActVIP	VIPS[MAX_LARGEACT_VIPCOUNT];
	ucchar		str_buf_vip[2+ MAX_LARGEACT_VIPCOUNT * 8 + 10];
	T_COMMON_CONTENT	OpeningNotice, EndingNotice;

	int			ActIndex = -1;

	_msgin.serial(FID, RoomID, ActID);
	NULLSAFE_SERIAL_WSTR(_msgin, Title, MAX_LARGEACT_NAME, FID);

	CPublicRoomWorld *inManager = GetPublicWorld(RoomID);
	if(inManager == NULL)
	{
		sipwarning("GetPublicWorld = NULL. RoomID=%d", RoomID);
		return;
	}
	LargeAct*	pLargeAct = inManager->m_LargeActs;

	uint32 currentActID = 0;
	if (inManager->m_nCurrentLargeActIndex != -1)
		currentActID = pLargeAct[inManager->m_nCurrentLargeActIndex].ActID;

	if (ActID == 0 && Title.empty())
	{
		sipwarning(L"actid = 0 & title empty");
		SEND_LARGEACTSETRESULT(false);
		return;
	}
	if (ActID > 0)
	{
		if (currentActID == ActID && inManager->m_nCurrentLargeActStatus == LARGEACT_STATE_DOING)
		{
			sipwarning(L"Can't delete current doing act=%d", currentActID);
			SEND_LARGEACTSETRESULT(false);
			return;
		}
		for (ActIndex = 0; ActIndex < inManager->m_nLargeActCount; ActIndex ++)
		{
			if (pLargeAct[ActIndex].ActID == ActID)
			{
				break;
			}
		}
		if (ActIndex >= inManager->m_nLargeActCount)
		{
			sipwarning("Can't find ActID. ActID=%d", ActID);
			SEND_LARGEACTSETRESULT(false);
			return;
		}
	}

	if (Title.empty())
	{
		// 삭제
		inManager->m_nLargeActCount --;
		for (int i = ActIndex; i < inManager->m_nLargeActCount; i ++)
		{
			pLargeAct[i] = pLargeAct[i + 1];
		}

		{
			MAKEQUERY_DeleteLargeAct(strQuery, ActID);
			DBCaller->execute(strQuery);
		}

		SEND_LARGEACTSETRESULT(true);
	}
	else
	{
		SAFE_SERIAL_WSTR(_msgin, Comment, MAX_LARGEACT_COMMENT, FID);
		NULLSAFE_SERIAL_STR(_msgin, FlagText, MAX_LARGEACT_FLAGNAME, FID);
		NULLSAFE_SERIAL_STR(_msgin, ActMean, MAX_LARGEACT_ACTMEAN, FID);
		_msgin.serial(Option, AcceptTimeM, StartTimeM);
		if (AcceptTimeM > StartTimeM - LARGEACTPREPARINGTIMEMINUTE)
		{
			sipwarning("AcceptTimeM >= StartTimeM - LARGEACTPREPARINGTIMEMINUTE");
			SEND_LARGEACTSETRESULT(false);
			return;
		}

		for (int i = 0; i < MAX_LARGEACT_ITEMCOUNT; i ++)
		{
			_msgin.serial(ItemIDs[i]);
			uint32	v = ItemIDs[i];
			smprintf((tchar*)(&str_buf_itemid[i * 8 + 0]), 2, _t("%02x"), (v >> 0) & 0xFF);
			smprintf((tchar*)(&str_buf_itemid[i * 8 + 2]), 2, _t("%02x"), (v >> 8) & 0xFF);
			smprintf((tchar*)(&str_buf_itemid[i * 8 + 4]), 2, _t("%02x"), (v >> 16) & 0xFF);
			smprintf((tchar*)(&str_buf_itemid[i * 8 + 6]), 2, _t("%02x"), (v >> 24) & 0xFF);
		}
		str_buf_itemid[MAX_LARGEACT_ITEMCOUNT * 8] = 0;
		for (int i = 0; i < MAX_LARGEACT_PHOTOCOUNT; i ++)
		{
			_msgin.serial(PhotoIDs[i]);
			uint32	v = PhotoIDs[i];
			smprintf((tchar*)(&str_buf_photoid[i * 8 + 0]), 2, _t("%02x"), (v >> 0) & 0xFF);
			smprintf((tchar*)(&str_buf_photoid[i * 8 + 2]), 2, _t("%02x"), (v >> 8) & 0xFF);
			smprintf((tchar*)(&str_buf_photoid[i * 8 + 4]), 2, _t("%02x"), (v >> 16) & 0xFF);
			smprintf((tchar*)(&str_buf_photoid[i * 8 + 6]), 2, _t("%02x"), (v >> 24) & 0xFF);
		}
		str_buf_photoid[MAX_LARGEACT_PHOTOCOUNT * 8] = 0;

		for (int i = 0; i < MAX_BLESSCARD_ID; i++)
		{
			_msgin.serial(PrizeBless[i]);
			uint8 v = PrizeBless[i];
			smprintf((&str_buf_prize[i*2]), 2, L"%02x", v);
			//sipdebug("Largeact prize : bless card(%d)=%d", i+1, v); byy krs
		}
		_msgin.serial(PrizeItemMode, PrizeItemNum);
		smprintf((&str_buf_prize[MAX_BLESSCARD_ID*2]), 2, L"%02x", PrizeItemMode);
		smprintf((&str_buf_prize[(MAX_BLESSCARD_ID+1)*2]), 2, L"%02x", PrizeItemNum);
		//sipdebug("Largeact prize : Give mode=%d, ItemTypeNum=%d", PrizeItemMode, PrizeItemNum); byy krs

		if (PrizeItemNum > MAX_LARGEACTPRIZENUM)
		{
			sipwarning("PrizeItemNum(%d) > MAX_LARGEACTPRIZENUM", PrizeItemNum);
			SEND_LARGEACTSETRESULT(false);
			return;
		}
		int curbufpos = (MAX_BLESSCARD_ID+2)*2;
		for (int i = 0; i < PrizeItemNum; i++)
		{
			_msgin.serial(PrizeItemInfo[i].ItemID, PrizeItemInfo[i].ItemNum);
			uint32 itemid = PrizeItemInfo[i].ItemID;
			uint16 itemnum = PrizeItemInfo[i].ItemNum;
			smprintf((&str_buf_prize[curbufpos+ i * 12 + 0]), 2, _t("%02x"), (itemid >> 0) & 0xFF);
			smprintf((&str_buf_prize[curbufpos+ i * 12 + 2]), 2, _t("%02x"), (itemid >> 8) & 0xFF);
			smprintf((&str_buf_prize[curbufpos+ i * 12 + 4]), 2, _t("%02x"), (itemid >> 16) & 0xFF);
			smprintf((&str_buf_prize[curbufpos+ i * 12 + 6]), 2, _t("%02x"), (itemid >> 24) & 0xFF);
			smprintf((&str_buf_prize[curbufpos+ i * 12 + 8]), 2, _t("%02x"), (itemnum >> 0) & 0xFF);
			smprintf((&str_buf_prize[curbufpos+ i * 12 + 10]), 2, _t("%02x"), (itemnum >> 8) & 0xFF);

			//sipdebug("Largeact prize : ItemID(%d)=%d, Num=%d", i+1, itemid, itemnum); byy krs
		}
		str_buf_prize[curbufpos+PrizeItemNum*12] = 0;

		_msgin.serial(VIPNum);
		smprintf((&str_buf_vip[0]), 2, L"%02x", VIPNum);
		for (int i = 0; i < VIPNum; i++)
		{
			_msgin.serial(VIPS[i].FID, VIPS[i].FName);
			uint32 fid = VIPS[i].FID;
			smprintf((&str_buf_vip[2+ i * 8 + 0]), 2, _t("%02x"), (fid >> 0) & 0xFF);
			smprintf((&str_buf_vip[2+ i * 8 + 2]), 2, _t("%02x"), (fid >> 8) & 0xFF);
			smprintf((&str_buf_vip[2+ i * 8 + 4]), 2, _t("%02x"), (fid >> 16) & 0xFF);
			smprintf((&str_buf_vip[2+ i * 8 + 6]), 2, _t("%02x"), (fid >> 24) & 0xFF);
			//sipdebug(L"Largeact vip : pos=%d, FID=%d, FName=%s", i+1, fid, VIPS[i].FName.c_str()); byy krs
		}
		str_buf_vip[2 + VIPNum * 8]=0;

		NULLSAFE_SERIAL_WSTR(_msgin, OpeningNotice, MAX_LARGEACT_NOTICE, FID);
		NULLSAFE_SERIAL_WSTR(_msgin, EndingNotice, MAX_LARGEACT_NOTICE, FID);
		//sipdebug(L"Large act opening notice : %s", OpeningNotice.c_str()); byy krs
		//sipdebug(L"Large act ending notice : %s", EndingNotice.c_str()); byy krs

		ucstring	sAcceptTime = GetTimeStrFromM1970(AcceptTimeM);
		ucstring	sStartTime = GetTimeStrFromM1970(StartTimeM);

		if (ActID == 0)
		{ // 추가
			for (ActIndex = 0; ActIndex < inManager->m_nLargeActCount; ActIndex ++)
			{
				if (pLargeAct[ActIndex].StartTime < StartTimeM)
					break;
			}

			// 앞뒤로 충분한 시간이 있도록 한다.
			if (ActIndex > 0) 
			{
//				if (pLargeAct[ActIndex - 1].StartTime - StartTimeM < ACTINTERALMINUTE)
//				{
//					sipwarning("pLargeAct[ActIndex - 1].StartTime - StartTimeM <= %d", ACTINTERALMINUTE);
//					return;
//				}
				if (pLargeAct[ActIndex - 1].AcceptTime < StartTimeM+LARGEACTDOINGTIMEMINUTE)
				{
					sipwarning("pLargeAct[ActIndex - 1].AcceptTime < StartTimeM+LARGEACTDOINGTIMEMINUTE");
					SEND_LARGEACTSETRESULT(false);
					return;
				}
			}
			if (ActIndex < inManager->m_nLargeActCount) 
			{
//				if (StartTimeM - pLargeAct[ActIndex].StartTime < ACTINTERALMINUTE)
//				{
//					sipwarning("StartTimeM - pLargeAct[ActIndex].StartTime <= %d", ACTINTERALMINUTE);
//					return;
//				}
				if (AcceptTimeM < pLargeAct[ActIndex].StartTime+LARGEACTDOINGTIMEMINUTE)
				{
					sipwarning("AcceptTimeM < pLargeAct[ActIndex].StartTime+LARGEACTDOINGTIMEMINUTE");
					SEND_LARGEACTSETRESULT(false);
					return;
				}
			}
			if (ActIndex == MAX_LARGEACT_COUNT)
			{
				for (int i = 1; i < MAX_LARGEACT_COUNT; i ++)
				{
					pLargeAct[i-1] = pLargeAct[i];
				}
				ActIndex--;
			}
			else
			{
				if (inManager->m_nLargeActCount >= MAX_LARGEACT_COUNT)
					inManager->m_nLargeActCount --;
				for (int i = inManager->m_nLargeActCount; i > ActIndex; i --)
				{
					pLargeAct[i] = pLargeAct[i - 1];
				}
				inManager->m_nLargeActCount ++;
			}

			// 메모리에 추가
			pLargeAct[ActIndex].Init();
			pLargeAct[ActIndex].Title = Title;
			pLargeAct[ActIndex].Comment = Comment;
			pLargeAct[ActIndex].FlagText = FlagText;
			pLargeAct[ActIndex].ActMean = ActMean;
			pLargeAct[ActIndex].Option = Option;
			pLargeAct[ActIndex].AcceptTime = AcceptTimeM;
			pLargeAct[ActIndex].StartTime = StartTimeM;
			for (int i = 0; i < MAX_LARGEACT_ITEMCOUNT; i ++)
			{
				pLargeAct[ActIndex].ItemIDs[i] = ItemIDs[i];
			}
			for (int i = 0; i < MAX_LARGEACT_PHOTOCOUNT; i ++)
			{
				pLargeAct[ActIndex].PhotoIDs[i] = PhotoIDs[i];
			}
			pLargeAct[ActIndex].PrizeBlessMode = (LARGEACTBLESSPRIZE_MODE)PrizeBlessMode;
			for (int i = 0; i < MAX_BLESSCARD_ID; i++)
				pLargeAct[ActIndex].PrizeBless[i] = PrizeBless[i];
			pLargeAct[ActIndex].PrizeItemMode = (LARGEACTITEMPRIZE_MODE)PrizeItemMode;
			pLargeAct[ActIndex].PrizeItemNum = PrizeItemNum;
			for (int i = 0; i < PrizeItemNum; i++)
			{
				pLargeAct[ActIndex].PrizeInfo[i] = PrizeItemInfo[i];
			}
			pLargeAct[ActIndex].VIPNum = VIPNum;
			for (int i = 0; i < VIPNum; i++)
			{
				pLargeAct[ActIndex].VIPS[i] = VIPS[i];
			}
			pLargeAct[ActIndex].OpeningNotice = OpeningNotice;
			pLargeAct[ActIndex].EndingNotice = EndingNotice;
			
			if (currentActID != 0)
				inManager->m_nCurrentLargeActIndex = inManager->GetIndexOfActID(currentActID);

			// DB에 추가
			MAKEQUERY_InsertLargeAct(strQuery, inManager->m_RoomID, Title, Comment, FlagText, ActMean, Option, sAcceptTime, sStartTime, str_buf_itemid, str_buf_photoid, str_buf_prize, str_buf_vip, OpeningNotice, EndingNotice);
			DBCaller->executeWithParam(strQuery, DBCB_DBInsertLargeAct,
				"DDDDDW",
				OUT_,		NULL,
				CB_,		inManager->m_RoomID,
				CB_,		ActIndex,
				CB_,		FID,
				CB_,		currentActID,
				CB_,		_tsid.get());
			return;
		}
		else
		{ // 수정
			// 앞뒤로 충분한 시간이 있도록 한다.
			for (int i = 0; i < inManager->m_nLargeActCount; i ++)
			{
				if (pLargeAct[ActIndex].ActID == ActID)
					continue;
//				if (abs((long double)StartTimeM - (long double)pLargeAct[i].StartTime) < ACTINTERALMINUTE)
//				{
//					sipwarning("StartTimeM is invalid1");
//					return;
//				}
				if (abs((long double)AcceptTimeM - (long double)pLargeAct[i].StartTime) < LARGEACTDOINGTIMEMINUTE)
				{
					sipwarning("StartTimeM is invalid");
					SEND_LARGEACTSETRESULT(false);
					return;
				}
				if (abs((long double)pLargeAct[i].AcceptTime - (long double)StartTimeM) < LARGEACTDOINGTIMEMINUTE)
				{
					sipwarning("StartTimeM is invalid2");
					SEND_LARGEACTSETRESULT(false);
					return;
				}
			}

			// 메모리에서 수정
			pLargeAct[ActIndex].Init();
			pLargeAct[ActIndex].ActID = ActID;
			pLargeAct[ActIndex].Title = Title;
			pLargeAct[ActIndex].Comment = Comment;
			pLargeAct[ActIndex].FlagText = FlagText;
			pLargeAct[ActIndex].ActMean = ActMean;
			pLargeAct[ActIndex].Option = Option;
			pLargeAct[ActIndex].AcceptTime = AcceptTimeM;
			pLargeAct[ActIndex].StartTime = StartTimeM;
			for (int i = 0; i < MAX_LARGEACT_ITEMCOUNT; i ++)
			{
				pLargeAct[ActIndex].ItemIDs[i] = ItemIDs[i];
			}
			for (int i = 0; i < MAX_LARGEACT_PHOTOCOUNT; i ++)
			{
				pLargeAct[ActIndex].PhotoIDs[i] = PhotoIDs[i];
			}
			pLargeAct[ActIndex].PrizeBlessMode = (LARGEACTBLESSPRIZE_MODE)PrizeBlessMode;
			for (int i = 0; i < MAX_BLESSCARD_ID; i++)
				pLargeAct[ActIndex].PrizeBless[i] = PrizeBless[i];
			pLargeAct[ActIndex].PrizeItemMode = (LARGEACTITEMPRIZE_MODE)PrizeItemMode;
			pLargeAct[ActIndex].PrizeItemNum = PrizeItemNum;
			for (int i = 0; i < PrizeItemNum; i++)
			{
				pLargeAct[ActIndex].PrizeInfo[i] = PrizeItemInfo[i];
			}
			pLargeAct[ActIndex].VIPNum = VIPNum;
			for (int i = 0; i < VIPNum; i++)
			{
				pLargeAct[ActIndex].VIPS[i] = VIPS[i];
			}
			pLargeAct[ActIndex].OpeningNotice = OpeningNotice;
			pLargeAct[ActIndex].EndingNotice = EndingNotice;

			// 날자에 따르는 재정리
			while ((ActIndex > 0) && (pLargeAct[ActIndex].StartTime > pLargeAct[ActIndex - 1].StartTime))
			{
				LargeAct	data = pLargeAct[ActIndex];
				pLargeAct[ActIndex] = pLargeAct[ActIndex - 1];
				pLargeAct[ActIndex - 1] = data;
				ActIndex --;
			}
			while ((ActIndex < inManager->m_nLargeActCount - 1) && (pLargeAct[ActIndex].StartTime < pLargeAct[ActIndex + 1].StartTime))
			{
				LargeAct	data = pLargeAct[ActIndex];
				pLargeAct[ActIndex] = pLargeAct[ActIndex + 1];
				pLargeAct[ActIndex + 1] = data;
				ActIndex ++;
			}

			if (currentActID != 0)
				inManager->m_nCurrentLargeActIndex = inManager->GetIndexOfActID(currentActID);
			
			// DB에서 수정
			MAKEQUERY_ModifyLargeAct(strQuery, ActID, Title, Comment, FlagText, ActMean, Option, sAcceptTime, sStartTime, str_buf_itemid, str_buf_photoid, str_buf_prize, str_buf_vip, OpeningNotice, EndingNotice);
			DBCaller->execute(strQuery);

			CMessage	msgNotify(M_SC_LARGEACT_INFOCHANGE);
			CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgNotify);

			SEND_LARGEACTSETRESULT(true);
		}
	}

	if (inManager->m_nCurrentLargeActStatus != LARGEACT_STATE_DOING)
	{
		uint32  nCandiActID = 0;
		int		nCandiIndex = inManager->GetCurrentCandiActIndex();
		if (nCandiIndex != -1)
			nCandiActID = pLargeAct[nCandiIndex].ActID;

		if (currentActID == nCandiActID)
		{
			inManager->m_nCurrentLargeActIndex = nCandiIndex;
			if (ActID != 0 && currentActID == ActID)
			{
				inManager->m_nCurrentLargeActStatus = LARGEACT_STATE_WAIT;
				inManager->SendCurrentLargeActStatus(0, TServiceId(0));
			}
		}
		else
		{
			inManager->ChangeCurrentLargeAct(nCandiIndex);
		}
	}
}

// Request LargeAct Detail Info ([d:RoomID][d:ActID])
void cb_M_GMCS_LARGEACT_DETAIL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint32		RoomID, ActID;
	int			ActIndex;

	_msgin.serial(FID, RoomID, ActID);

	CPublicRoomWorld *inManager = GetPublicWorld(RoomID);
	if(inManager == NULL)
	{
		sipwarning("GetPublicWorld = NULL. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}
	LargeAct*	pLargeAct = inManager->m_LargeActs;
	for (ActIndex = 0; ActIndex < inManager->m_nLargeActCount; ActIndex ++)
	{
		if (pLargeAct[ActIndex].ActID == ActID)
			break;
	}
	if (ActIndex >= inManager->m_nLargeActCount)
	{
		sipwarning("Can't find ActID. ActID=%d", ActID);
		return;
	}

	CMessage	msgOut(M_GMSC_LARGEACT_DETAIL);
	msgOut.serial(FID, ActID, pLargeAct[ActIndex].Option);
	for (int i = 0; i < MAX_LARGEACT_ITEMCOUNT; i ++)
	{
		msgOut.serial(pLargeAct[ActIndex].ItemIDs[i]);
	}
	for (int i = 0; i < MAX_LARGEACT_PHOTOCOUNT; i ++)
	{
		msgOut.serial(pLargeAct[ActIndex].PhotoIDs[i]);
	}
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

void CPublicRoomWorld::ChangeCurrentLargeAct(int nIndex)
{
	InitLargeAct();
	m_nCurrentLargeActIndex = nIndex;
	m_nCurrentLargeActStatus = LARGEACT_STATE_WAIT;
	if (nIndex != -1 )
	{
		LargeAct&	curLargeAct = m_LargeActs[m_nCurrentLargeActIndex];
		curLargeAct.ResetHis();
		uint32		curTime = GetDBTimeMinute();
		if (curTime >= curLargeAct.StartTime)
			m_nCurrentLargeActStatus = LARGEACT_STATE_END;
	}

	SendCurrentLargeActStatus(0, TServiceId(0));

	//sipdebug(L"Change current large act act index = %d", nIndex); byy krs
}

void CPublicRoomWorld::CheckCurrentLargeActStatus()
{
	// 대형행사자료가 한개도 없으면
	if (m_nLargeActCount <= 0)
		return;

	uint32		curTime = GetDBTimeMinute();

	// 현재 설정된 행사가 있으면
	if (m_nCurrentLargeActIndex != -1)
	{
		LargeAct&	curLargeAct = m_LargeActs[m_nCurrentLargeActIndex];
		if (m_nCurrentLargeActStatus == LARGEACT_STATE_WAIT ||
			m_nCurrentLargeActStatus == LARGEACT_STATE_PREPAREING)
		{
			uint32 nDeltaTime = 30;
			if (curTime >= (curLargeAct.StartTime - nDeltaTime) &&
				curTime < curLargeAct.StartTime &&
				curLargeAct.OpeningNoticeHis.find(curTime) == curLargeAct.OpeningNoticeHis.end() &&
				curLargeAct.OpeningNotice.size() > 0)
			{
				uint32 nGoTime = curTime - (curLargeAct.StartTime - nDeltaTime);
				if (nGoTime % 5 == 0)
				{
					SendSystemNotice(SN_LARGEACT, curLargeAct.OpeningNotice);
					curLargeAct.OpeningNoticeHis.insert(std::make_pair(curTime, curTime));
				}
			}
		}

		switch (m_nCurrentLargeActStatus)
		{
		case LARGEACT_STATE_WAIT:
			{
				if (!curLargeAct.bNotifyToVIP &&
					curLargeAct.VIPNum > 0 &&
					curTime >= (curLargeAct.AcceptTime-2))
				{
					SendReadyToVIP();
					curLargeAct.bNotifyToVIP = true;
				}
								
				if (curTime >= (curLargeAct.StartTime - LARGEACTPREPARINGTIMEMINUTE))
				{
					SendLargeActPreparePacket();
					m_nCurrentLargeActStatus = LARGEACT_STATE_PREPAREING;
					//sipdebug(L"Change large act state to preparing actid = %d, title = %s", curLargeAct.ActID, curLargeAct.Title.c_str()); byy krs
				}
			}
			break;
		case LARGEACT_STATE_PREPAREING:
			{
				if (curTime >= curLargeAct.StartTime)
				{
					bool bResult = StartLargeAct();
					if (bResult)
					{
						m_nCurrentLargeActStatus = LARGEACT_STATE_DOING;
						//sipdebug(L"Change large act state to doing actid = %d, title = %s", curLargeAct.ActID, curLargeAct.Title.c_str()); byy krs
					}
					else
					{
						m_nCurrentLargeActStatus = LARGEACT_STATE_END;
						//sipdebug(L"Change large act state to end(can't start) actid = %d, title = %s", curLargeAct.ActID, curLargeAct.Title.c_str()); byy krs
					}
				}
			}
			break;
		case LARGEACT_STATE_DOING:
			{
				if (curTime >= curLargeAct.StartTime + LARGEACTDOINGTIMEMINUTE)
				{
					m_nCurrentLargeActStatus = LARGEACT_STATE_END;
					ClearLargeActUserList();
					//sipdebug(L"Change large act state to end(auto) actid = %d, title = %s", curLargeAct.ActID, curLargeAct.Title.c_str()); byy krs
				}
			}
			break;
		case LARGEACT_STATE_END:
			{
				int nCandiIndex = GetCurrentCandiActIndex();
				if (m_nCurrentLargeActIndex != nCandiIndex)
					ChangeCurrentLargeAct(nCandiIndex);
			}
			break;
		}
		return;
	}
	else
	{
		int nCandiIndex = GetCurrentCandiActIndex();
		if (m_nCurrentLargeActIndex != nCandiIndex)
			ChangeCurrentLargeAct(nCandiIndex);
	}
/*
	int		newActIndex = -1, newActStatus = LARGEACT_CUR_NONE;

	uint32	curTime = GetDBTimeMinute();
	if (m_nLargeActCount > 0)
	{
		for (newActIndex = 0; newActIndex < m_nLargeActCount; newActIndex ++)
		{
			if (curTime >= m_LargeActs[newActIndex].StartTime)
				break;
		}
		if (newActIndex >= m_nLargeActCount)
		{
			newActIndex = m_nLargeActCount - 1;
			if (curTime >= m_LargeActs[newActIndex].AcceptTime)
			{
				newActStatus = LARGEACT_CUR_ACCEPTING1;
				if (curTime >= m_LargeActs[newActIndex].StartTime - 5)
					newActStatus = LARGEACT_CUR_PREPAREING;
			}
		}
		else if (newActIndex == 0)
		{
			if (!IsSameDay(curTime * 60, m_LargeActs[0].StartTime * 60))
			{
				newActIndex = -1;
			}
		}
		else
		{
			if (curTime >= m_LargeActs[newActIndex - 1].AcceptTime)
			{
				newActIndex --;
				newActStatus = LARGEACT_CUR_ACCEPTING1;
				if (curTime >= m_LargeActs[newActIndex].StartTime - 5)
					newActStatus = LARGEACT_CUR_PREPAREING;
			}
			else if (curTime < m_LargeActs[newActIndex].StartTime + 3)	// 시작후 30분동안을 행사진행중으로 평가한다.
				newActStatus = LARGEACT_CUR_DOING;
			else if (IsSameDay(m_LargeActs[newActIndex].StartTime * 60, m_LargeActs[newActIndex - 1].AcceptTime * 60))
			{
				if (curTime >= (m_LargeActs[newActIndex - 1].AcceptTime + m_LargeActs[newActIndex].StartTime) / 2)
					newActIndex --;
			}
			else if (!IsSameDay(m_LargeActs[newActIndex].StartTime * 60, curTime * 60))
			{
				newActIndex --;
			}
		}
	}

	if (((newActIndex == -1) && (g_nCurrentLargeActID != 0)) || (g_nCurrentLargeActID != m_LargeActs[newActIndex].ActID))
	{
		// 현재대형행사가 변함
		g_nCurrentLargeActID = m_LargeActs[newActIndex].ActID;
		m_nCurrentLargeActIndex = newActIndex;
		m_nCurrentLargeActStatus = LARGEACT_CUR_NONE;

		SendCurrentLargeActStatus(0, TServiceId(0));
	}
	else if (m_nCurrentLargeActStatus != newActStatus)
	{
		if ((m_nCurrentLargeActStatus == LARGEACT_CUR_NONE) && (newActStatus != LARGEACT_CUR_NONE))
		{
			ClearLargeActUserList();
			m_nCurrentLargeActStatus = LARGEACT_CUR_ACCEPTING1;
		}
		if ((m_nCurrentLargeActStatus == LARGEACT_CUR_ACCEPTING1) && (newActStatus != LARGEACT_CUR_ACCEPTING1))
		{
			SendLargeActPreparePacket();
			m_nCurrentLargeActStatus = LARGEACT_CUR_PREPAREING;
		}
		if ((m_nCurrentLargeActStatus == LARGEACT_CUR_PREPAREING) && (newActStatus != LARGEACT_CUR_PREPAREING))
		{
			StartLargeAct();
			m_nCurrentLargeActStatus = LARGEACT_CUR_DOING;
		}
		if ((m_nCurrentLargeActStatus == LARGEACT_CUR_DOING) && (newActStatus != LARGEACT_CUR_DOING))
		{
			m_nCurrentLargeActStatus = LARGEACT_CUR_NONE;
		}
	}
	*/
}

void SendReadyToVIPForLogin(T_FAMILYID fid, SIPNET::TServiceId _tsid)
{
	CPublicRoomWorld *inManager;
	inManager = GetPublicWorld(ROOM_ID_PUBLICROOM);
	if(inManager != NULL)
	{
		inManager->SendReadyToVIPForLogin(fid, _tsid);
	}
	inManager = GetPublicWorld(ROOM_ID_KONGZI);
	if(inManager != NULL)
	{
		inManager->SendReadyToVIPForLogin(fid, _tsid);
	}
	inManager = GetPublicWorld(ROOM_ID_HUANGDI);
	if(inManager != NULL)
	{
		inManager->SendReadyToVIPForLogin(fid, _tsid);
	}
}

void CPublicRoomWorld::SendReadyToVIPForLogin(T_FAMILYID fid, SIPNET::TServiceId _tsid)
{
	if (m_nCurrentLargeActIndex == -1)
		return;

	LargeAct&	curLargeAct = m_LargeActs[m_nCurrentLargeActIndex];
	if ((m_nCurrentLargeActStatus == LARGEACT_STATE_WAIT || m_nCurrentLargeActStatus == LARGEACT_STATE_PREPAREING) &&
		curLargeAct.bNotifyToVIP)
	{
		if (!curLargeAct.IsVIP(fid))
			return;

		//FAMILY*	pFamily = GetFamilyData(fid);
		//if (pFamily != NULL)
		{
			CMessage	msgOut(M_SC_LARGEACT_READYVIP);
			msgOut.serial(fid, curLargeAct.ActID);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}
	}
}

void CPublicRoomWorld::SendReadyToVIP()
{
	if (m_nCurrentLargeActIndex == -1)
		return;
	LargeAct&	curLargeAct = m_LargeActs[m_nCurrentLargeActIndex];
	PublicRoomChannelData	*pDefaultChannel = GetChannel(1);
	if (pDefaultChannel)
		pDefaultChannel->m_LargeActUserList.clear();
	for (uint8 i = 0; i < curLargeAct.VIPNum; i++)
	{
		FAMILY*	pFamily = GetFamilyData(curLargeAct.VIPS[i].FID);
		if (pFamily != NULL)
		{
			CMessage	msgOut(M_SC_LARGEACT_READYVIP);
			msgOut.serial(curLargeAct.VIPS[i].FID, curLargeAct.ActID);
			CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
		}

		if (pDefaultChannel)
		{
			T_FAMILYID FID = curLargeAct.VIPS[i].FID;
			uint16 nCurPos = (uint16)(pDefaultChannel->m_LargeActUserList.size());
			LargeActUser newVIP(FID, nCurPos);
			pDefaultChannel->m_LargeActUserList.push_back(newVIP);
			m_CurrentLargeActUserList[FID] = pDefaultChannel->m_ChannelID;
		}
	}
	uint32 totalUserNum = GetCurrentTotalLargeActUser();
	{
		for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
		{
			uint32 ucount = pChannelData->m_LargeActUserList.size();
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LARGEACT_USERNUM)
			{
				msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].ActID, ucount, totalUserNum);
			}
			FOR_END_FOR_FES_ALL()
		}
	}
}

void CPublicRoomWorld::SendLargeActPreparePacket(T_FAMILYID fid)
{
	if (m_nCurrentLargeActIndex == -1)
		return;

	for (std::map<uint32, uint32>::iterator it = m_CurrentLargeActUserList.begin(); it != m_CurrentLargeActUserList.end(); it ++)
	{
		uint32	curfid = it->first;
		if (fid != 0 && curfid != fid)
			continue;
		if (m_LargeActs[m_nCurrentLargeActIndex].IsVIP(curfid))
		{
			if (!m_LargeActs[m_nCurrentLargeActIndex].IsVIPRequest(curfid))
				continue;
		}

		FAMILY*	pFamily = GetFamilyData(curfid);
//		INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
		if (pFamily != NULL)
		{
			CMessage	msgOut(M_SC_LARGEACT_PREPARE);
			msgOut.serial(curfid);
			CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
		}
	}
}

uint32	CPublicRoomWorld::GetCurrentTotalLargeActUser()
{
	uint32	UserCount = 0;
	for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		UserCount += pChannelData->m_LargeActUserList.size();
	}
	return UserCount;
}

bool CPublicRoomWorld::StartLargeAct()
{
	if (m_nCurrentLargeActIndex == -1)
		return false;

	uint32	UserCount = 0;
	bool bResult = false;
	for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		// 현재 채널에 없는 사용자들을 목록에서 삭제한다.
		std::list<LargeActUser>::iterator it;
		for (it = pChannelData->m_LargeActUserList.begin(); it != pChannelData->m_LargeActUserList.end(); )
		{
			uint32	FID = it->FID;
			bool bRemove = false;
			bRemove = !IsInThisChannel(FID, pChannelData->m_ChannelID);
			if (!bRemove && 
				pChannelData->m_ChannelID == 1 &&
				m_LargeActs[m_nCurrentLargeActIndex].IsVIP(FID))
			{
				if (m_LargeActs[m_nCurrentLargeActIndex].IsVIPRequest(FID) == false)
					bRemove = true;
			}

			if (bRemove)
			{
				it = pChannelData->m_LargeActUserList.erase(it);
				std::map<uint32, uint32>::iterator	itCur = m_CurrentLargeActUserList.find(FID);
				if (itCur == m_CurrentLargeActUserList.end())
				{
					sipwarning("itCur == m_CurrentLargeActUserList.end(). FID=%d", FID);
				}
				else
				{
					m_CurrentLargeActUserList.erase(itCur);
				}
			}
			else
				it++;
		}
		uint16 curUserPos = 0;
		for (it = pChannelData->m_LargeActUserList.begin(); it != pChannelData->m_LargeActUserList.end(); it++)
		{
			it->ActPos = curUserPos;
			curUserPos ++;
		}

		uint32 channelUserCount = pChannelData->m_LargeActUserList.size();
		UserCount += channelUserCount;

		// 채널의 행사참가자가 하나도 없는 경우에는 행사를 하지 않는다.
		if (channelUserCount > 0)
		{
			LargeAct	*pCurrentLargeAct = &m_LargeActs[m_nCurrentLargeActIndex];
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LARGEACT_START)
			{
				msgOut.serial(pCurrentLargeAct->Option);
				for (int j = 0; j < MAX_LARGEACT_ITEMCOUNT; j ++)
				{
					msgOut.serial(pCurrentLargeAct->ItemIDs[j]);
				}
				msgOut.serial(channelUserCount);
				std::list<LargeActUser>::iterator itUser;
				for (itUser = pChannelData->m_LargeActUserList.begin(); itUser != pChannelData->m_LargeActUserList.end(); itUser++)
				{
					msgOut.serial(itUser->FID, itUser->ActPos);
				}
				msgOut.serial(pCurrentLargeAct->EndingNotice);
			}
			FOR_END_FOR_FES_ALL()

			bResult = true;
			//sipdebug(L"Starting large actid=%d, channel=%d, usercount=%d", m_LargeActs[m_nCurrentLargeActIndex].ActID, pChannelData->m_ChannelID, channelUserCount); byy krs
		}
		else
		{
			pChannelData->m_nCurrentLargeActStep = ACTENDSTEP;
		}
	}

	m_LargeActs[m_nCurrentLargeActIndex].UserCount = UserCount;
	// DB에 행사참가자수를 보관한다.
	{
		MAKEQUERY_SetLargeActUserCount(strQuery, m_LargeActs[m_nCurrentLargeActIndex].ActID, UserCount);
		DBCaller->execute(strQuery);
	}

	{
		for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
		{
			uint32 channelUserCount = pChannelData->m_LargeActUserList.size();
			FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LARGEACT_USERNUM)
			{
				msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].ActID, channelUserCount, UserCount);
			}
			FOR_END_FOR_FES_ALL()
		}
	}
	return bResult;
}

// FID = 0 : send to All
// FID != 0 : send to FID
void CPublicRoomWorld::SendCurrentLargeActStatus(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	uint32	zero = 0;
	if (m_nCurrentLargeActIndex == -1)
	{
		uint32 actid = 0;
		if (FID != 0)
		{
			CMessage	msgOut(M_SC_LARGEACT_CURRENT);
			msgOut.serial(FID, zero, actid);
			CUnifiedNetwork::getInstance()->send(sid, msgOut);
		}
		else
		{
			FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_LARGEACT_CURRENT)
				msgOut.serial(actid);
			FOR_END_FOR_FES_ALL()
		}
	}
	else
	{
		if (FID != 0)
		{
			uint32	user_count = m_LargeActs[m_nCurrentLargeActIndex].UserCount;
			if (m_nCurrentLargeActStatus == LARGEACT_STATE_WAIT ||
				m_nCurrentLargeActStatus == LARGEACT_STATE_PREPAREING)
			{
				PublicRoomChannelData	*pChannelData = GetFamilyChannel(FID);
				if (pChannelData == NULL)
				{
					sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
					return;
				}
				user_count = pChannelData->m_LargeActUserList.size();
			}

			CMessage	msgOut(M_SC_LARGEACT_CURRENT);
			msgOut.serial(FID, zero, m_LargeActs[m_nCurrentLargeActIndex].ActID, m_LargeActs[m_nCurrentLargeActIndex].Title, m_LargeActs[m_nCurrentLargeActIndex].Comment, m_LargeActs[m_nCurrentLargeActIndex].FlagText);
			msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].ActMean,	m_LargeActs[m_nCurrentLargeActIndex].AcceptTime, m_LargeActs[m_nCurrentLargeActIndex].StartTime, user_count);
			for (int i = 0; i < MAX_LARGEACT_PHOTOCOUNT; i ++)
			{
				msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].PhotoIDs[i]);
			}
			CUnifiedNetwork::getInstance()->send(sid, msgOut);
		}
		else
		{
			uint32	user_count = m_LargeActs[m_nCurrentLargeActIndex].UserCount;
			if (m_nCurrentLargeActStatus == LARGEACT_STATE_WAIT ||
				m_nCurrentLargeActStatus == LARGEACT_STATE_PREPAREING)
			{
				for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
				{
					user_count = pChannelData->m_LargeActUserList.size();

					FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LARGEACT_CURRENT)
					{
						msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].ActID, m_LargeActs[m_nCurrentLargeActIndex].Title, m_LargeActs[m_nCurrentLargeActIndex].Comment,m_LargeActs[m_nCurrentLargeActIndex].FlagText,m_LargeActs[m_nCurrentLargeActIndex].ActMean,
							m_LargeActs[m_nCurrentLargeActIndex].AcceptTime, m_LargeActs[m_nCurrentLargeActIndex].StartTime, user_count);
						for (int i = 0; i < MAX_LARGEACT_PHOTOCOUNT; i ++)
						{
							msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].PhotoIDs[i]);
						}
					}
					FOR_END_FOR_FES_ALL()
				}
			}
			else
			{
				FOR_START_FOR_FES_ALL(m_mapFesFamilys, msgOut, M_SC_LARGEACT_CURRENT)
				{
					msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].ActID, m_LargeActs[m_nCurrentLargeActIndex].Title, m_LargeActs[m_nCurrentLargeActIndex].Comment,m_LargeActs[m_nCurrentLargeActIndex].FlagText,m_LargeActs[m_nCurrentLargeActIndex].ActMean,
						m_LargeActs[m_nCurrentLargeActIndex].AcceptTime, m_LargeActs[m_nCurrentLargeActIndex].StartTime, user_count);
					for (int i = 0; i < MAX_LARGEACT_PHOTOCOUNT; i ++)
					{
						msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].PhotoIDs[i]);
					}
				}
				FOR_END_FOR_FES_ALL()
			}
		}
	}
}

int CPublicRoomWorld::GetCurrentCandiActIndex()
{
	uint32		curTime = GetDBTimeMinute();
	for (int i = m_nLargeActCount-1; i >= 0; i --)
	{
		if (curTime < m_LargeActs[i].StartTime)
		{
			if (i == m_nLargeActCount-1)
			{
				return i;
			}
			else
			{
				uint32 nextLargeActAcceptMinute	= m_LargeActs[i].AcceptTime;
				uint32 curLargeActStartMinute = m_LargeActs[i+1].StartTime;
				if (IsSameDay(curLargeActStartMinute*60, nextLargeActAcceptMinute*60))
				{
					if (curTime >= (curLargeActStartMinute + nextLargeActAcceptMinute) / 2)
						return i;
					else
						return (i+1);
				}
				else 
				{
					if (!IsSameDay(curLargeActStartMinute * 60, curTime * 60))
						return i;
					else
						return (i+1);
				}
			}
			break;
		}
	}
	if (IsSameDay(m_LargeActs[0].StartTime*60, curTime*60))
		return 0;
	return -1;
}

int CPublicRoomWorld::GetIndexOfActID(uint32 actid)
{
	for (int i = 0; i < m_nLargeActCount; i ++)
	{
		if (m_LargeActs[i].ActID == actid)
		{
			return i;
		}
	}
	return -1;
}

void CPublicRoomWorld::RefreshLargeActStatus()
{
	CheckCurrentLargeActStatus();
}

// Request to enter LargeAct
void cb_M_CS_LARGEACT_REQUEST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	T_ROOMID		RoomID;
	uint32			SyncCode;
	_msgin.serial(FID, RoomID, SyncCode);

	// 방관리자를 얻는다
	CPublicRoomWorld *inManager = GetPublicWorld(RoomID);
	if (inManager == NULL)
	{
		sipwarning("GetPublicWorld = NULL. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	inManager->on_M_CS_LARGEACT_REQUEST(FID, SyncCode, _tsid);
}

void CPublicRoomWorld::on_M_CS_LARGEACT_REQUEST(T_FAMILYID FID, uint32 SyncCode, SIPNET::TServiceId _sid)
{
	uint32	ErrorCode = 0;
	uint32	UserCount = 0;
	uint32	ActChannelID = 0;
	if (m_nCurrentLargeActIndex == -1)
	{
		sipdebug("There is no current larger act.");
		return;
	}
	if (m_LargeActs[m_nCurrentLargeActIndex].IsVIP(FID))
	{
		PublicRoomChannelData	*pChannelData = GetChannel(1);
		CMessage	msgOut(M_SC_LARGEACT_REQUEST);
		if (m_nCurrentLargeActStatus != LARGEACT_STATE_WAIT &&
			m_nCurrentLargeActStatus != LARGEACT_STATE_PREPAREING)
			ErrorCode = E_LARGEACT_NOACCEPT;
		else
		{
			ErrorCode = 0;
			m_LargeActs[m_nCurrentLargeActIndex].SetVIPRequest(FID, true);
		}

		ActChannelID = 1;
		if (pChannelData)
			UserCount = pChannelData->m_LargeActUserList.size();
		else
			UserCount = 1;
		msgOut.serial(FID, ErrorCode, UserCount, ActChannelID, SyncCode);
		CUnifiedNetwork::getInstance()->send(_sid, msgOut);
		
		if (ErrorCode == 0 && m_nCurrentLargeActStatus == LARGEACT_STATE_PREPAREING)
		{
			SendLargeActPreparePacket(FID);
		}
		return;
	}

	PublicRoomChannelData	*pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}
	ActChannelID = pChannelData->m_ChannelID;
	switch (m_nCurrentLargeActStatus)
	{
	case LARGEACT_STATE_WAIT:
	case LARGEACT_STATE_PREPAREING:
		{
			uint32		curTime = GetDBTimeMinute();
			if (curTime < m_LargeActs[m_nCurrentLargeActIndex].AcceptTime-1)
			{
				ErrorCode = E_LARGEACT_NOACCEPTTIME;
				break;
			}
			if (m_CurrentLargeActUserList.find(FID) != m_CurrentLargeActUserList.end())
			{
				ErrorCode = E_LARGEACT_ALREADYREQUEST;
			}
			else
			{
				if (pChannelData->m_LargeActUserList.size() >= GetLargeActUserMaxNum())
				{
					FAMILY	*pFamily = GetFamilyData(FID); // by krs

					uint32	enterchannelid = 0;
//					uint32	enterchannelusernum = 0;
					uint32	emptychannelbeforeid = 0;
					// 신청여유가 있는 선로를 탐색한다
					TmapPublicRoomChannelData::iterator	itChannel;
					for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); itChannel++)
					{
						uint32	channelid = itChannel->first;

						if ((pFamily->m_bIsMobile == 2 && channelid < ROOM_UNITY_CHANNEL_START) || (pFamily->m_bIsMobile == 0 && channelid > ROOM_UNITY_CHANNEL_START)) // by krs
							continue;

						if (channelid == emptychannelbeforeid+1)
							emptychannelbeforeid = channelid;
						if (channelid == pChannelData->m_ChannelID)
							continue;
						PublicRoomChannelData	*pChannel = &itChannel->second;
						if (pChannel->m_LargeActUserList.size() < GetLargeActUserMaxNum())
						{
							enterchannelid = channelid;
//							enterchannelusernum = pChannel->m_LargeActUserList.size();
							break;
						}
					}
					// 신청여유가 있는 선로가 없으면 새선로를 창조한다
					if (enterchannelid == 0)
					{
						uint32 createchannelid = emptychannelbeforeid + 1;
						CreateChannel(createchannelid);
//						PublicRoomChannelData	data(this, COUNT_PUBLIC_ACTPOS, m_RoomID, createchannelid, m_SceneID);
//						m_mapChannels.insert(make_pair(createchannelid, data));
						// Channel이 추가됨을 현재 Channel의 사용자들에게 통지한다.
						NoticeChannelChanged(createchannelid, 1);
						//sipdebug("Channel Created(for new act request) ChannelID=%d", createchannelid); byy krs
						enterchannelid = createchannelid;
					}
					PublicRoomChannelData	*pEnterChannel = GetChannel(enterchannelid);
					uint16 nCurPos = (uint16)(pEnterChannel->m_LargeActUserList.size());
					LargeActUser	newUser(FID, nCurPos);
					pEnterChannel->m_LargeActUserList.push_back(newUser);
					m_CurrentLargeActUserList[FID] = pEnterChannel->m_ChannelID;
					UserCount = pEnterChannel->m_LargeActUserList.size();
					ActChannelID = enterchannelid;
				}
				else
				{
					uint16 nCurPos = (uint16)(pChannelData->m_LargeActUserList.size());
					LargeActUser	newUser(FID, nCurPos);
					pChannelData->m_LargeActUserList.push_back(newUser);
					m_CurrentLargeActUserList[FID] = pChannelData->m_ChannelID;
					UserCount = pChannelData->m_LargeActUserList.size();
				}
			}
		}
		break;
	default:
		ErrorCode = E_LARGEACT_NOACCEPT;
	}

	CMessage	msgOut(M_SC_LARGEACT_REQUEST);
	msgOut.serial(FID, ErrorCode, UserCount, ActChannelID, SyncCode);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);

	uint32 totalUserNum = GetCurrentTotalLargeActUser();
	//sipdebug(L"Larger act accepting result=%d, channelusercount=%d, channelid=%d, familyid=%d, SyncCode=%d, totalusercount=%d", ErrorCode, UserCount, ActChannelID, FID, SyncCode, totalUserNum); byy krs

	if (ErrorCode == 0)
	{
		PublicRoomChannelData	*pEnterChannel = GetChannel(ActChannelID);

		uint32 channelUserCount = pEnterChannel->m_LargeActUserList.size();
		FOR_START_FOR_FES_ALL(pEnterChannel->m_mapFesChannelFamilys, msgOut, M_SC_LARGEACT_USERNUM)
		{
			msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].ActID, channelUserCount, totalUserNum);
		}
		FOR_END_FOR_FES_ALL()
	}
	if (ErrorCode == 0 && m_nCurrentLargeActStatus == LARGEACT_STATE_PREPAREING)
	{
		SendLargeActPreparePacket(FID);
	}
}

// Cencel to enter LargeAct ([d:RoomID])
void cb_M_CS_LARGEACT_CANCEL(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			RoomID;
	_msgin.serial(FID, RoomID);

	// 방관리자를 얻는다
	CPublicRoomWorld *inManager = GetPublicWorld(RoomID);
	if (inManager == NULL)
	{
		sipwarning("GetPublicWorld = NULL. FID=%d, RoomID=%d", FID, RoomID);
		return;
	}

	inManager->RemoveFIDFromLargeActWaitList(FID);
}

void CPublicRoomWorld::RemoveFIDFromLargeActWaitList(T_FAMILYID FID)
{
	// VIP이면 처리를 하지 않는다
	if (m_nCurrentLargeActIndex != -1)
	{
		if (m_LargeActs[m_nCurrentLargeActIndex].IsVIP(FID) &&
			m_nCurrentLargeActStatus != LARGEACT_STATE_DOING)
		{
			m_LargeActs[m_nCurrentLargeActIndex].SetVIPRequest(FID, false);
			return;
		}
	}

	std::map<uint32, uint32>::iterator	it = m_CurrentLargeActUserList.find(FID);
	if (it == m_CurrentLargeActUserList.end())
	{
		return;
	}
	m_CurrentLargeActUserList.erase(FID);

	uint32	ChannelID = it->second;
	PublicRoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("GetChannel = NULL. FID=%d, ChannelID=%d", FID, ChannelID);
		return;
	}

	std::list<LargeActUser>::iterator itUser;
	for (itUser = pChannelData->m_LargeActUserList.begin(); itUser != pChannelData->m_LargeActUserList.end(); itUser++)
	{
		if (itUser->FID == FID)
		{
			pChannelData->m_LargeActUserList.erase(itUser);
			if (m_nCurrentLargeActIndex != -1)
			{
				uint32 totalUserNum = GetCurrentTotalLargeActUser();
				uint32 channelUserCount = pChannelData->m_LargeActUserList.size();
				//sipdebug("Remove largeact user : familyid=%d, channelid=%d, channelusercount=%d, totalusercount=%d", FID, pChannelData->m_ChannelID, channelUserCount, totalUserNum); byy krs
				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LARGEACT_USERNUM)
				{
					msgOut.serial(m_LargeActs[m_nCurrentLargeActIndex].ActID, channelUserCount, totalUserNum);
				}
				FOR_END_FOR_FES_ALL()
			}
			return;
		}
	}
	//sipwarning("Can't find in Channel's LargeActUserList. FID=%d, ChannelID=%d", FID, ChannelID); byy krs
}

// Send LargeAct Step Command ([d:Step,ACTENDSTEP-Finished])
void cb_M_CS_LARGEACT_STEP(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			Step;
	_msgin.serial(FID, Step);
	// 방관리자를 얻는다
	CPublicRoomWorld *inManager = GetPublicWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetPublicWorldFromFID = NULL. FID=%d", FID);
		return;
	}

	if (inManager->m_nCurrentLargeActIndex == -1)
	{
		sipwarning(L"no large act. FID=%d, step=%d", FID, Step);
		return;
	}
	if (inManager->m_nCurrentLargeActStatus != LARGEACT_STATE_DOING)
	{
		sipdebug("m_nCurrentLargeActStatus != LARGEACT_CUR_DOING. m_nCurrentLargeActStatus=%d", inManager->m_nCurrentLargeActStatus);
		return;
	}

	inManager->on_M_CS_LARGEACT_STEP(FID, Step);
}

void CPublicRoomWorld::on_M_CS_LARGEACT_STEP(T_FAMILYID FID, uint32 Step)
{
	uint32	ChannelID = GetFamilyChannelID(FID);

	SetLargeActStep(ChannelID, Step);
}

void CPublicRoomWorld::SetLargeActStep(uint32 ChannelID, uint32 Step)
{
	PublicRoomChannelData	*pChannelData = GetChannel(ChannelID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. ChannelID=%d", ChannelID);
		return;
	}

	if (pChannelData->m_nCurrentLargeActStep < Step)
	{
		pChannelData->m_nCurrentLargeActStep = Step;

		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LARGEACT_STEP)
		{
			msgOut.serial(Step);
		}
		FOR_END_FOR_FES_ALL()

		if (Step >= ACTENDSTEP)
		{
			bool bAllEnd = true;
			for (PublicRoomChannelData *pChannel = GetFirstChannel(); pChannel != NULL; pChannel = GetNextChannel())
			{
				if (pChannel->m_nCurrentLargeActStep < ACTENDSTEP)
					bAllEnd = false;
			}
			pChannelData->m_LargeActUserList.clear();
			for (std::map<uint32, uint32>::iterator it = m_CurrentLargeActUserList.begin(); it != m_CurrentLargeActUserList.end(); )
			{
				T_FAMILYID LargetActFID = it->first;
				if (it->second != ChannelID)
				{
					it++;
					continue;
				}
				it = m_CurrentLargeActUserList.erase(it);
				
				ChangeFamilyExp(Family_Exp_Type_Daxingyishi, LargetActFID);

				// 선물아이템주기
				if (m_nCurrentLargeActIndex == -1)
					continue;
				LargeAct	*pCurrentLargeAct = &m_LargeActs[m_nCurrentLargeActIndex];

				_InvenItems	*pInven = GetFamilyInven(LargetActFID);
				if (pInven == NULL)
				{
					sipwarning("GetFamilyInven NULL FID=%d", LargetActFID);
					continue;
				}
				
				CMessage	msgPrize(M_NT_LARGEACTPRIZE);
				msgPrize.serial(LargetActFID);
				int nBlessID = -1;
				if (pCurrentLargeAct->PrizeBlessMode == LARGEACTBLESSPRIZE_RANDOMONE)
				{
					nBlessID = Irand(0, MAX_BLESSCARD_ID-1);
				}
				for (uint32 ii = 0; ii < MAX_BLESSCARD_ID; ii++)
				{
					uint8	givenum = pCurrentLargeAct->PrizeBless[ii];
					if (nBlessID != -1 && ii != nBlessID)
						givenum = 0;
					msgPrize.serial(givenum);
				}
				if (pCurrentLargeAct->PrizeItemNum > 0)
				{
					int nGiveID = -1;
					if (pCurrentLargeAct->PrizeItemMode == LARGEACTITEMPRIZE_RANDOMONE)
					{
						nGiveID = Irand(0, pCurrentLargeAct->PrizeItemNum-1);
					}

					int InvenIndex = 0;
					for (uint32 i = 0; i < pCurrentLargeAct->PrizeItemNum; i++)
					{
						if (nGiveID != -1 && i != nGiveID)
							continue;
						bool bGive = false;
						for (InvenIndex = InvenIndex; InvenIndex < MAX_INVEN_NUM; InvenIndex ++)
						{
							if (pInven->Items[InvenIndex].ItemID != 0)
								continue;
							T_ITEMSUBTYPEID	ItemID = pCurrentLargeAct->PrizeInfo[i].ItemID;
							uint8			Num = (uint8)pCurrentLargeAct->PrizeInfo[i].ItemNum;
							pInven->Items[InvenIndex].ItemID = ItemID;
							pInven->Items[InvenIndex].ItemCount = Num;
							pInven->Items[InvenIndex].GetDate = 0;
							pInven->Items[InvenIndex].UsedCharID = 0;
							pInven->Items[InvenIndex].MoneyType = MONEYTYPE_UMONEY;

							if (!SaveFamilyInven(LargetActFID, pInven, false))
							{
								sipwarning("SaveFamilyInven failed. FID=%d", LargetActFID);
								continue;
							}
							uint16 InvenPos = GetInvenPosFromInvenIndex(InvenIndex);
							msgPrize.serial(InvenPos, ItemID, Num);
							bGive = true;
							break;;
						}
						if (!bGive)
							break;
					}
				}
				CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgPrize);
				
			}
			if (bAllEnd)
			{
				m_nCurrentLargeActStatus = LARGEACT_STATE_END;
				ClearLargeActUserList();
				//sipdebug(L"Change large act state to all end"); byy krs
				if (m_nCurrentLargeActIndex != -1 &&
					m_LargeActs[m_nCurrentLargeActIndex].EndingNotice.size() > 0)
					SendSystemNotice(SN_LARGEACT, m_LargeActs[m_nCurrentLargeActIndex].EndingNotice);
			}
		}
		//sipdebug(L"SetLargeActStep=%d, channelid=%d", Step, ChannelID); byy krs
	}
}

static uint32 s_nPublicRoomLuckTimeMS = 7200000;	// 공공구역에서 선밍이 남아있는 시간
static uint32 s_nPublicRoomLuckActLimitTimeMS = 300000;
void	CPublicRoomWorld::RefreshLuck()
{
	if (m_nLuckID == 0)
		return;
	if (CTime::getLocalTime() - m_LuckStartTime > s_nPublicRoomLuckTimeMS)
	{
		bool bActing = false;
		{
			for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
			{
				pChannelData->CancelActivityWaiting(ACTPOS_PUB_LUCK);
				if (pChannelData->IsDoingActivity(ACTPOS_PUB_LUCK))
					bActing = true;
			}
			if (bActing)
				return;
		}
		/*
		for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
		{
			{
				FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_DELLUCKINPUBROOM)
				{
					msgOut.serial(m_nLuckID);
				}
				FOR_END_FOR_FES_ALL()
			}
		}
		*/
		//sipinfo("Removed public room's luck=%d", m_nLuckID); byy krs
		m_nLuckID = 0;
		m_LuckFID = NOFAMILYID;
		m_LuckFName = L"";
		m_LuckTimeMin = 0;
		m_LuckTimeSec = 0;
		m_ReceiveLuckFamily.clear();

		// 선밍이 없어질 때에는 LuckID를 1000으로 보내야 한다.
		uint32		nTempLuckID = 1000;

		CMessage	msgOut(M_NT_CURLUCKINPUBROOM);
		msgOut.serial(nTempLuckID, m_LuckFID, m_LuckFName, m_LuckTimeMin, m_LuckTimeSec);
		CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
	}
}

void	CPublicRoomWorld::RequestLuck(T_FAMILYID FID)
{
	if (m_nLuckID != 0)
		return;
	// 날이 바뀌기 10분전에는 받을수 없다
	TTime nextDeltaDayS = (GetDBTimeDays()+1) * SECOND_PERONEDAY - GetDBTimeSecond();
	if (nextDeltaDayS < 600)
		return;

	for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		if (pChannelData->IsDoingActivity(ACTPOS_PUB_LUCK))
			return;
	}

	OUT_LUCK	*pLuck = GetLuck(true);
	if (pLuck == NULL)
		return;

	if (pLuck->LuckLevel != 4)
		return;

	m_LuckStartTime = CTime::getLocalTime();
	m_LuckTimeMin = GetDBTimeMinute();
	m_LuckTimeSec = (uint8)(GetDBTimeSecond() - m_LuckTimeMin*60);
	m_nLuckID = pLuck->LuckID;
	m_LuckFID = FID;
	m_LuckFName = GetFamilyNameFromBuffer(FID);
	m_ReceiveLuckFamily.clear();
	m_LuckHisInfo.clear();
	//sipinfo("new public room's luck=%d, FID=%d", m_nLuckID, m_LuckFID); byy krs

	{
		MAKEQUERY_GetPublicRoomLuckHisInsert(strQ, m_LuckFID, m_nLuckID);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY(DB, Stmt, strQ);
	}
	ChangeFamilyExp(Family_Exp_Type_Shenming, FID);
	CMessage	msgOut(M_NT_CURLUCKINPUBROOM);
	msgOut.serial(m_nLuckID, m_LuckFID, m_LuckFName, m_LuckTimeMin, m_LuckTimeSec);
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut);
	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);

/*	
	for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_NEWLUCKINPUBROOM)
		{
			msgOut.serial(m_nLuckID, m_LuckFID, m_LuckFName, m_LuckTimeMin, m_LuckTimeSec);
		}
		FOR_END_FOR_FES_ALL()
	}
	{
		CMessage	msgOut(M_SC_LUCK_TOALL);
		T_FAMILYID	 tempMasterID = NOFAMILYID;
		T_FAMILYNAME tempMasterName = L"";
		msgOut.serial(m_LuckFID, m_LuckFName, m_nLuckID, m_RoomID, m_RoomName, m_SceneID, tempMasterID, tempMasterName);
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
	}
*/
}

void	CPublicRoomWorld::RequestBlessCard(T_FAMILYID FID)
{
	if (m_nLuckID == 0)
		return;

	map<T_FAMILYID, T_FAMILYID>::iterator it = m_ReceiveLuckFamily.find(FID);
	if (it != m_ReceiveLuckFamily.end())
		return;

	CMessage msg(M_NT_REQGIVEBLESSCARD);
	msg.serial(FID, m_nLuckID);
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msg);
}

void	CPublicRoomWorld::NotifySendedBlesscard(T_FAMILYID FID, uint32 LuckID)
{
	if (m_nLuckID != LuckID)
		return;

	m_ReceiveLuckFamily.insert(make_pair(FID, FID));
}

static void	DBCB_DBGetPublicRoomLuckHisList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID			= *(uint32 *)argv[0];
	uint32			Page		= *(uint32 *)argv[1];
	TServiceId		tsid(*(uint16 *)argv[2]);
	uint32			roomID		= *(uint32*)(argv[3]);
	if (!nQueryResult)
		return;
	CPublicRoomWorld* pWolrd		= GetPublicWorld(roomID);
	if (pWolrd == NULL)
	{
		sipwarning("GetPublicWorld = NULL. RoomID=%d", roomID);
		return;
	}

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipdebug(L"DBCB_DBGetPublicRoomLuckHisList failed(Can't get allcount).");
		return;
	}
	uint32 allcount;			resSet->serial(allcount);

	CMessage	msgOut(M_SC_HISLUCKINPUBROOM);
	msgOut.serial(FID, allcount, Page);
	{
		PLuckHisPageInfo newPageInfo;
		newPageInfo.bValid = true;
		newPageInfo.HisAllCount = allcount;

		uint32	nRows2;	resSet->serial(nRows2);
		for(uint32 i = 0; i < nRows2; i ++)
		{
			uint32			HisID;			resSet->serial(HisID);
			uint32			LuckID;			resSet->serial(LuckID);
			T_FAMILYID		fid;			resSet->serial(fid);
			T_FAMILYNAME	FName;			resSet->serial(FName);
			T_DATETIME		LuckTime;		resSet->serial(LuckTime);
			uint32			ModelID;		resSet->serial(ModelID);
			SIPBASE::MSSQLTIME tempTime;
			tempTime.SetTime(LuckTime);
			uint32 sTime = static_cast<uint32>(tempTime.timevalue);

			msgOut.serial(LuckID, fid, FName, ModelID, sTime);
			AddFamilyNameData(fid, FName);
			newPageInfo.HisList.push_back(PLuckHisInfo(HisID, LuckID, fid, FName, ModelID, sTime));
		}

		uint32 mapIndex = Page+1;	// For page is started from 0
		TPLuckHisPageInfo::iterator itPage = pWolrd->m_LuckHisInfo.find(mapIndex);
		if (itPage != pWolrd->m_LuckHisInfo.end())
		{
			pWolrd->m_LuckHisInfo[mapIndex] = newPageInfo;
		}
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgOut);
}

void	CPublicRoomWorld::RequestLuckHisPageInfo(T_FAMILYID FID, uint32 Page, uint8 PageSize, SIPNET::TServiceId _tsid)
{
	uint32 mapIndex = Page+1;	// For page is started from 0
	TPLuckHisPageInfo::iterator itPage = m_LuckHisInfo.find(mapIndex);
	if (itPage != m_LuckHisInfo.end())
	{
		// 모바일과 PC가 PageSize가 다르기때문에 이 방법을 쓸수 없다.
		//PLuckHisPageInfo& pageInfo = itPage->second;
		//if (pageInfo.bValid)
		//{
		//	CMessage	msgOut(M_SC_HISLUCKINPUBROOM);
		//	msgOut.serial(FID, pageInfo.HisAllCount, Page);
		//	std::list<PLuckHisInfo>::iterator lstit;
		//	for (lstit = pageInfo.HisList.begin(); lstit != pageInfo.HisList.end(); lstit++)
		//	{
		//		PLuckHisInfo& HisInfo = *lstit;
		//		uint32			LuckID = HisInfo.LuckID;
		//		T_FAMILYID		HisFamilyID = HisInfo.FamilyID;
		//		T_FAMILYNAME	HisFName = GetFamilyNameFromBuffer(HisFamilyID);
		//		T_S1970			HisTime = HisInfo.LuckTime;

		//		msgOut.serial(LuckID, HisFamilyID, HisFName, HisInfo.ModelID, HisTime);
		//	}
		//	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		//	return;
		//}
	}
	else
	{
		PLuckHisPageInfo newPageInfo;
		m_LuckHisInfo[mapIndex] = newPageInfo;
	}

	RefreshTodayLuckList(); // by krs
	MAKEQUERY_GetPublicRoomLuckHisList(strQuery, Page, PageSize);
	DBCaller->executeWithParam(strQuery, DBCB_DBGetPublicRoomLuckHisList,
		"DDWD",
		CB_,		FID,
		CB_,		Page,
		CB_,		_tsid.get(),
		CB_,		m_RoomID);
}

void	cb_M_CS_REQLUCKINPUBROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	_msgin.serial(FID);

	// 방관리자를 얻는다
	CPublicRoomWorld *inManager = GetPublicWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetPublicWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	inManager->RequestLuck(FID);
}

void	cb_M_CS_GETBLESSCARDINPUBROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	_msgin.serial(FID);
	CPublicRoomWorld *inManager = GetPublicWorldFromFID(FID);
	if (inManager == NULL)
	{
		sipwarning("GetPublicWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	inManager->RequestBlessCard(FID);
}

// Request luck history in  public room([d:page][b:PageSize])
void	cb_M_CS_HISLUCKINPUBROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			Page;
	uint8			PageSize;

	_msgin.serial(FID, Page, PageSize);

	CPublicRoomWorld* pWolrd = GetPublicWorldFromFID(FID);
	if(pWolrd == NULL)
	{
		sipwarning("GetPublicWorldFromFID = NULL. FID=%d", FID);
		return;
	}
	pWolrd->RequestLuckHisPageInfo(FID, Page, PageSize, _tsid);
}

void	cb_M_NT_RESGIVEBLESSCARD(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	uint32			LuckID;
	_msgin.serial(FID, LuckID);

	CPublicRoomWorld *inManager = GetPublicWorld(ROOM_ID_PUBLICROOM);
	if (!inManager)
		return;

	inManager->NotifySendedBlesscard(FID, LuckID);

}

uint32 SharedRoomMaxUsers = 0;
// 공공구역에서는 대형행사요청자들이 방에 없어도 있는것으로 처리한다.
//void CPublicRoomWorld::RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
//{
//	CMessage	msgOut(M_SC_CHANNELLIST);
//	msgOut.serial(_FID, m_RoomID);
//
//	// Channel목록 만들기
//	uint32	numLimit = (uint32)m_nMaxCharacterOfRoomKind;
//if (SharedRoomMaxUsers > 0)
//	numLimit = SharedRoomMaxUsers;
//	uint32	new_channelid = 0;
//	uint8	channel_status;
//	bool	bSmoothChannelExist = false;
//	int		empty_channel_count = 0;
//
//	// 선로중 비지 않은 원활한 선로가 있는가를 검사한다. 있으면 빈 선로를 삭제할수 있다. 1번선로는 삭제하지 않는다.
//	TmapPublicRoomChannelData::iterator	itChannel;
//	TmapDwordToDword	user_count;
//	uint32				usercount;
//	for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); itChannel ++)
//	{
//		PublicRoomChannelData	*pChannel = &itChannel->second;
//		usercount = pChannel->m_LargeActUserList.size();
//		std::list<LargeActUser>::iterator itUser;
//		for (itUser = pChannel->m_LargeActUserList.begin(); itUser != pChannel->m_LargeActUserList.end(); itUser ++)
//		{
//			uint32	userFID = itUser->FID;
//			if (pChannel->IsChannelFamily(userFID))
//				usercount --;
//		}
//		usercount += pChannel->GetChannelUserCount();
//		user_count[itChannel->first] = usercount;
//		if(usercount == 0)
//			empty_channel_count ++;
//		else if(usercount * 100 < numLimit * g_nRoomSmoothFamPercent)
//			{
//				if(pChannel->m_LargeActUserList.size() < GetLargeActUserMaxNum())
//					bSmoothChannelExist = true;
//			}
//	}
//
//	// 대기렬에 사람이 없는 경우, 원활한선로가 존재하는 경우에 빈선로를 삭제할수 있다.
//	bool	bDeleteAvailable = false;
//	if((map_EnterWait.size() <= 1) && (bSmoothChannelExist || (empty_channel_count > 1)))	// (본인은 물론 현재의 대기렬에 있으므로) 대기렬에 2명이상이면 선로삭제를 하지 않는다.
//		bDeleteAvailable = true;
//
//	int		cur_empty_channel = 0;
//	for(itChannel = m_mapChannels.begin(); itChannel != m_mapChannels.end(); )
//	{
//		uint32	channelid = itChannel->first;
//		uint32	usercount = user_count[channelid];
//
//		if(bDeleteAvailable && (usercount == 0)) // 빈선로는 삭제가능성 검사한다.
//		{
//			cur_empty_channel ++;
//			if(channelid > m_nDefaultChannelCount) // default선로는 삭제하지 않는다.
//			{
//				if(bSmoothChannelExist || (cur_empty_channel > 1))
//				{
//					// Channel이 삭제됨을 현재 Channel의 사용자들에게 통지한다.
//					NoticeChannelChanged(channelid, 0);
//
//					sipdebug("Channel Deleted. ChannelID=%d", channelid);
//
//					// 빈 선로를 삭제한다. 1번선로는 삭제하지 않는다.
//					itChannel = m_mapChannels.erase(itChannel);
//					continue;
//				}
//			}
//		}
//
//		if(channelid == new_channelid+1)
//			new_channelid = channelid;
//
//		PublicRoomChannelData	*pChannel = &itChannel->second;
//		if(usercount >= numLimit)
//			channel_status = ROOMCHANNEL_FULL;
//		else if(usercount * 100 >= numLimit * g_nRoomSmoothFamPercent)
//			channel_status = ROOMCHANNEL_COMPLEX;
//		else
//		{
//			// bSmoothChannelExist = true;
//			if(pChannel->m_LargeActUserList.size() < GetLargeActUserMaxNum())
//				channel_status = ROOMCHANNEL_SMOOTH;
//			else
//				channel_status = ROOMCHANNEL_COMPLEX;
//		}
//
//		msgOut.serial(channelid, channel_status);
//
//		itChannel ++;
//	}
//
//	if(!bSmoothChannelExist && (empty_channel_count == 0))
//	{
//		// 원활한 선로가 없다면 하나 창조한다.
//		new_channelid ++;
//
//		PublicRoomChannelData	data(this, COUNT_PUBLIC_ACTPOS, m_RoomID, new_channelid, m_SceneID);
//		if (m_nCurrentLargeActStatus == LARGEACT_STATE_DOING)
//		{
//			data.m_nCurrentLargeActStep = ACTENDSTEP;
//		}
//
//		m_mapChannels.insert(make_pair(new_channelid, data));
//
//		channel_status = ROOMCHANNEL_SMOOTH;
//
//		msgOut.serial(new_channelid, channel_status);
//
//		// Channel이 추가됨을 현재 Channel의 사용자들에게 통지한다.
//		NoticeChannelChanged(new_channelid, 1);
//
//		sipdebug("Channel Created. ChannelID=%d", new_channelid);
//	}
//
//	CUnifiedNetwork::getInstance()->send(_sid, msgOut);
//}

uint32 PublicRoomChannelData::EstimateChannelUserCount(uint32 maxCount)
{
	uint32	usercount = GetChannelUserCount() + m_LargeActUserList.size();
	std::list<LargeActUser>::iterator itUser;
	for (itUser = m_LargeActUserList.begin(); itUser != m_LargeActUserList.end(); itUser ++)
	{
		if (IsChannelFamily(itUser->FID))
			usercount --;
	}

	// 행사진행시에는 행사인원이 다 차면 복잡한 선로로 표시해야 한다.
	if (m_LargeActUserList.size() >= GetLargeActUserMaxNum())
	{
		if (usercount < maxCount)
		{
			if (maxCount > 1)
				usercount = maxCount - 1;
			else
				usercount = 1;
		}
	}

	return usercount;
}

void	PublicRoomChannelData::on_M_CS_ACT_REQ(T_FAMILYID FID, uint8 ActPos, SIPNET::TServiceId _tsid)
{
	if(ActPos >= m_ActivityCount)
	{
		// 비법발견
		ucstring ucUnlawContent;
		ucUnlawContent = SIPBASE::_toString(L"ActPos >= m_ActivityCount: FID=%d, ActPos=%d", FID, ActPos);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	uint32		ErrorCode = ERR_NOERR, RemainCount = -1;
	uint8		DeskNo=0;
	ErrorCode = IsPossibleActReq(ActPos);
	if (ErrorCode != ERR_NOERR)
	{
		CMessage	msgOut(M_SC_ACT_WAIT);
		msgOut.serial(FID, ActPos, ErrorCode, RemainCount, DeskNo);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		return;
	}

	uint32	ChannelID = m_ChannelID;

	// 1명의 User는 하나의 활동 혹은 대기만 할수 있다.
	int		ret = GetActivityIndexOfUser(FID);
	if(ret >= 0)
	{
		// 비법발견
		//ucstring ucUnlawContent;
		//ucUnlawContent = SIPBASE::_toString(L"User require Two activities!: FID=%d", FID);
		//RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		sipwarning("GetActivityIndexOfUser(ChannelID, FID) >= 0. FID=%d, RoomID=%d, ReqActPos=%d, ret=0x%x", FID, m_RoomID, ActPos, ret);
		return;
	}

	CSharedActivity &rActivity = m_Activitys[ActPos];

	for(int j = 0; j < rActivity.ActivityDeskCount; j ++)
	{
		if(rActivity.ActivityDesk[j].FID == 0)
		{
			// 행사시작
			rActivity.ActivityDesk[j].init(FID, AT_SIMPLERITE);

			RemainCount = 0;
			DeskNo = (uint8)j;
			break;
		}
	}

	if(RemainCount == -1)
	{
		// 공황구역은 대기가 없다.
		if (m_RoomID == ROOM_ID_KONGZI || m_RoomID == ROOM_ID_HUANGDI)
		{
			CMessage	msgOut(M_SC_ACT_WAIT);
			ErrorCode = E_KONGZI_HWANGDI_ACT_WAIT;
			msgOut.serial(FID, ActPos, ErrorCode);
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
			return;
		}

		// 행사대기
		RemainCount = 1 + rActivity.WaitFIDs.size();
		_RoomActivity_Wait	data(FID, AT_SIMPLERITE,0,0);
		rActivity.WaitFIDs.push_back(data);
	}

	CMessage	msgOut(M_SC_ACT_WAIT);
	msgOut.serial(FID, ActPos, ErrorCode, RemainCount, DeskNo);
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

void CPublicRoomWorld::CreateChannel(uint32 channelid)
{
	PublicRoomChannelData	data(this, COUNT_PUBLIC_ACTPOS, m_RoomID, channelid, m_SceneID);
	if (m_nCurrentLargeActStatus == LARGEACT_STATE_DOING)
	{
		data.m_nCurrentLargeActStep = ACTENDSTEP;
	}

	m_mapChannels.insert(make_pair(channelid, data));
}

void	CPublicRoomWorld::ProcRemoveFamily(T_FAMILYID FID) 
{ 
	RemoveFIDFromLargeActWaitList(FID); 
	for (PublicRoomChannelData *pChannelData = GetFirstChannel(); pChannelData != NULL; pChannelData = GetNextChannel())
	{
		pChannelData->AllActCancel(FID, false, true);
	}
}

void CPublicRoomWorld::OutRoom_Spec(T_FAMILYID FID)
{
	PublicRoomChannelData*	pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}

	// 사용자가 창조하였던 혹은 대기하였던 활동을 삭제한다.
	pChannelData->AllActCancel(FID, false, false);

	// 념불에서 탈퇴
	pChannelData->ExitNianFo(FID);
	pChannelData->ExitNeiNianFo(FID);

	if ((m_nCurrentLargeActStatus == LARGEACT_STATE_DOING) && (pChannelData->m_nCurrentLargeActStep < ACTENDSTEP))
	{
		std::list<LargeActUser>::iterator itUser;
		for (itUser = pChannelData->m_LargeActUserList.begin(); itUser != pChannelData->m_LargeActUserList.end(); itUser ++)
		{
			if (itUser->FID == FID)
			{
				pChannelData->m_LargeActUserList.erase(itUser);
				if (pChannelData->m_LargeActUserList.size() == 0)
				{
					SetLargeActStep(pChannelData->m_ChannelID, ACTENDSTEP);	// ACTENDSTEP - LargeAct Finished
				}
				else
				{
					FOR_START_FOR_FES_ALL(pChannelData->m_mapFesChannelFamilys, msgOut, M_SC_LARGEACT_OUT)
					{
						msgOut.serial(FID);
					}
					FOR_END_FOR_FES_ALL()
				}
				m_CurrentLargeActUserList.erase(FID);
				return;
			}
		}
	}
}

void CPublicRoomWorld::EnterFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid)
{
	SendCurrentLargeActStatus(FID, sid);
	SendYiSiResult(FID, sid);
	SendRoomEventData(FID, sid);

	PublicRoomChannelData*	pChannelData = GetFamilyChannel(FID);
	if (pChannelData == NULL)
	{
		sipwarning("GetFamilyChannel = NULL. FID=%d", FID);
		return;
	}
	pChannelData->SendActivityList(FID, sid);

	if (m_nCurrentLargeActIndex != -1 &&
		m_nCurrentLargeActStatus == LARGEACT_STATE_DOING &&
		pChannelData->m_nCurrentLargeActStep < ACTENDSTEP &&
		pChannelData->m_LargeActUserList.size() > 0)
	{
		LargeAct	*pCurrentLargeAct = &m_LargeActs[m_nCurrentLargeActIndex];

		uint32 zero = 0;
		CMessage msgOut(M_SC_LARGEACT_START);
		msgOut.serial(FID, zero);
		msgOut.serial(pCurrentLargeAct->Option);
		for (int j = 0; j < MAX_LARGEACT_ITEMCOUNT; j ++)
		{
			msgOut.serial(pCurrentLargeAct->ItemIDs[j]);
		}

		uint32 channelUserCount = pChannelData->m_LargeActUserList.size();
		msgOut.serial(channelUserCount);
		std::list<LargeActUser>::iterator itUser;
		for (itUser = pChannelData->m_LargeActUserList.begin(); itUser != pChannelData->m_LargeActUserList.end(); itUser++)
		{
			msgOut.serial(itUser->FID, itUser->ActPos);
		}
		msgOut.serial(pCurrentLargeAct->EndingNotice);
		CUnifiedNetwork::getInstance()->send(sid, msgOut);
	}

	// 념불읽기정보를 내려보낸다.
	pChannelData->SendNianFoState(FID, sid);
	pChannelData->SendNeiNianFoState(FID, sid);
}

//void CPublicRoomWorld::ChangeChannelFamily_Spec(T_FAMILYID FID, SIPNET::TServiceId sid)
//{
//	EnterFamily_Spec(FID, sid);
//}

uint32	PublicRoomChannelData::IsPossibleActReq(uint8 ActPos)
{
	CPublicRoomWorld *pWorld = dynamic_cast<CPublicRoomWorld *>(m_ChannelWorld);
	if (pWorld == NULL)
		return ERR_NOERR;

	return pWorld->IsPossibleActReq(ActPos);
}

uint32	CPublicRoomWorld::IsPossibleActReq(uint8 ActPos)
{
	if (m_RoomID != ROOM_ID_PUBLICROOM)
		return ERR_NOERR;

	if (ActPos != ACTPOS_PUB_LUCK)
		return ERR_NOERR;
	if (m_nLuckID == 0)
		return E_ACT_CANTREQ;

	if (CTime::getLocalTime() > m_LuckStartTime + s_nPublicRoomLuckTimeMS - s_nPublicRoomLuckActLimitTimeMS) // 선밍없어지기 5분전에는 행사를 요청할수 없다
		return E_ACT_CANTREQ;
	return ERR_NOERR;
}

void	CPublicRoomWorld::RefreshTodayLuckList()
{
	MAKEQUERY_GetPublicRoomLuckRefresh(strQ);
	CDBConnectionRest	DB(DBCaller);
	DB_EXE_QUERY(DB, Stmt, strQ);
}

void CPublicRoomWorld::CheckDoEverydayWork()
{
	TTime	curTime = GetDBTimeSecond();
	if (m_NextReligionCheckEveryWorkTimeSecond > curTime)
	{
		return;
	}
	ResetSharedYiSiResult();
	SendSharedYiSiResultToAll();
	SetRoomEvent();
	m_LuckStartTime = 0;	// 지금 있는 선밍을 없애기 위한 조작이다
	m_LuckHisInfo.clear();
	RefreshTodayLuckList();
	m_NextReligionCheckEveryWorkTimeSecond = GetUpdateNightTime(curTime);
}

void RefreshPublicRoom()
{
	static TTime	LastUpdateAnimal = 0;
	static TTime	LastUpdateLargeAct = 0;
	static TTime	LastOneMinuteCycleWork = 0;

	bool	bAnimal = true, bLargeAct = true, bOneMinuteCycleWork = true;

	TTime curTime = CTime::getLocalTime();
	if (curTime - LastUpdateAnimal < 1000)
		bAnimal = false;
	if (curTime - LastUpdateLargeAct < 1000)
		bLargeAct = false;
	if (curTime - LastOneMinuteCycleWork < 60000)
		bOneMinuteCycleWork = false;

	TChannelWorlds::iterator It;
	for (It = map_RoomWorlds.begin(); It != map_RoomWorlds.end(); It++)
	{
		CChannelWorld*		pChannel = It->second.getPtr();
		if (pChannel->m_RoomID != ROOM_ID_PUBLICROOM && pChannel->m_RoomID != ROOM_ID_KONGZI && pChannel->m_RoomID != ROOM_ID_HUANGDI)
			continue;
		CPublicRoomWorld	*rw = dynamic_cast<CPublicRoomWorld*>(pChannel);
		if (bAnimal)
		{
			rw->RefreshAnimal();
			// 폭죽관련
			rw->RefreshCracker();
		}
		if (bOneMinuteCycleWork)
		{
			rw->CheckDoEverydayWork();
			rw->RefreshLuck();	// 반드시 CheckDoEverydayWork후에 호출하여야 한다
			rw->RefreshFreezingPlayers();
		}

		if (bLargeAct)
			rw->RefreshLargeActStatus();
		rw->RefreshCharacterState();

		if (rw->GetRoomEventStatus() == ROOMEVENT_STATUS_DOING)
			rw->RefreshRoomEvent();
		else
		{
			if (bOneMinuteCycleWork)
				rw->RefreshRoomEvent();
		}
	}

	curTime = CTime::getLocalTime();
	if(bAnimal)
		LastUpdateAnimal = curTime;
	if(bLargeAct)
		LastUpdateLargeAct = curTime;
	if (bOneMinuteCycleWork)
		LastOneMinuteCycleWork = curTime;
}

uint32	CPublicRoomWorld::SaveYisiAct(T_FAMILYID FID, TServiceId _tsid, uint8 NpcID, uint8 ActionType, uint8 Secret, ucstring Pray, uint8 ItemNum, uint32 SyncCode, uint16* invenpos, uint32 ActID)
{
	uint8	itembuf[MAX_ITEM_PER_ACTIVITY * sizeof(uint32) + 1];
	itembuf[0] = 0;
	uint32	*itemlist = (uint32*)(&itembuf[1]);
	_InvenItems*	pInven = GetFamilyInven(FID);
	if(pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL:FID=%d", FID);
		return ActID;
	}
	bool	bYiSiMaster = (ActID == 0);
	uint32	UsedUMoney = 0, UsedGMoney = 0;
	bool bChangeInven = false;
	bool bTaoCanItem = false;
	for (int i = 0; i < ItemNum; i ++)
	{
		uint16 InvenPos = invenpos[i];
		if (InvenPos == 0)
			continue;

		uint32	ItemID = 0;
		// InvenPos 검사
		if(!IsValidInvenPos(InvenPos))
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos! FID=%d, InvenPos=%d", FID, InvenPos);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return ActID;
		}
		int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
		ITEM_INFO	&item = pInven->Items[InvenIndex];
		ItemID = item.ItemID;
		// ItemID의 정확성은 검사하지 않는다.
		const	MST_ITEM	*mstItem = GetMstItemInfo(ItemID);
		if(mstItem == NULL)
		{
			sipwarning(L"Invalid Item. FID=%d, ItemID=%d, InvenPos=%d, InvenIndex=%d", FID, ItemID, InvenPos, InvenIndex);
			return ActID;
		}
		if (GetTaocanItemInfo(ItemID) != NULL)
			bTaoCanItem = true;

		// Delete from Inven
		if(1 < item.ItemCount)
		{
			item.ItemCount --;
		}
		else
		{
			item.ItemID = 0;
		}
		// 공덕값 변경
		if(item.MoneyType == MONEYTYPE_UMONEY)
			UsedUMoney += mstItem->UMoney;
		else
			UsedGMoney += mstItem->GMoney;

		// Log
		switch(ActionType)
		{
		case AT_PRAYRITE:
		case AT_PRAYMULTIRITE:
		case AT_PUBLUCK1:
		case AT_PUBLUCK2:
		case AT_PUBLUCK3:
		case AT_PUBLUCK4:
		case AT_PUBLUCK1_MULTIRITE:
		case AT_PUBLUCK2_MULTIRITE:
		case AT_PUBLUCK3_MULTIRITE:
		case AT_PUBLUCK4_MULTIRITE:
			DBLOG_ItemUsed(DBLSUB_USEITEM_R0_TAOCAN, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			break;
		case AT_PRAYXIANG:
			DBLOG_ItemUsed(DBLSUB_USEITEM_R0_XIANG, FID, 0, ItemID, 1, mstItem->Name, m_RoomName, m_RoomID);
			break;
		}
		*itemlist = ItemID;
		itembuf[0] ++;
		itemlist ++;

		bChangeInven = true;
	}
	if (bChangeInven)
	{
		if(!SaveFamilyInven(FID, pInven))
		{
			sipwarning("SaveFamilyInven failed. FID=%d", FID);
			return ActID;
		}
	}
	//// AT_PRAYRITE : 하늘땅의식, 하늘땅의식중에는 Prize를 없애기로 한다. 왜? 형상이 되지 않았으므로. - M_SC_INVENSYNC 다음으로!!
	//if (//ActionType == AT_PRAYRITE || 
	//	ActionType == AT_PUBLUCK1 ||
	//	ActionType == AT_PUBLUCK2 ||
	//	ActionType == AT_PUBLUCK3 ||
	//	ActionType == AT_PUBLUCK4)
	//{
	//	CheckPrize(FID, PRIZE_TYPE_YISHI, UsedUMoney);
	//}

	uint32 ActPos = NpcID;
	// 경험값증가
	switch (ActionType)
	{
	case AT_PRAYRITE:
	case AT_PUBLUCK1:
	case AT_PUBLUCK2:
	case AT_PUBLUCK3:
	case AT_PUBLUCK4:
		if (bYiSiMaster)
			ChangeFamilyExp(Family_Exp_Type_ReligionYiSiMaster, FID);
		break;
	case AT_PRAYMULTIRITE:
	case AT_PUBLUCK1_MULTIRITE:
	case AT_PUBLUCK2_MULTIRITE:
	case AT_PUBLUCK3_MULTIRITE:
	case AT_PUBLUCK4_MULTIRITE:
		if (bYiSiMaster)
			ChangeFamilyExp(Family_Exp_Type_ReligionYiSiMaster, FID);
		else
			ChangeFamilyExp(Family_Exp_Type_ReligionYiSiSlaver, FID);
		break;
	case AT_PRAYXIANG:
		if (ActPos >= ACTPOS_R0_XIANG_INDOOR_1 && ActPos <= ACTPOS_R0_XIANG_INDOOR_6)
			ChangeFamilyExp(Family_Exp_Type_ReligionTouXiang, FID);
		if (ActPos >= ACTPOS_R0_XIANG_OUTDOOR_1 && ActPos <= ACTPOS_R0_XIANG_OUTDOOR_7)
			ChangeFamilyExp(Family_Exp_Type_ReligionPuTongXiang, FID);
		break;
	}

	// Inven처리정형 통보
	if (SyncCode > 0)
	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	// AT_PRAYRITE : 하늘땅의식, 하늘땅의식중에는 Prize를 없애기로 한다. 왜? 형상이 되지 않았으므로.
	if (//ActionType == AT_PRAYRITE || 
		ActionType == AT_PUBLUCK1 ||
		ActionType == AT_PUBLUCK2 ||
		ActionType == AT_PUBLUCK3 ||
		ActionType == AT_PUBLUCK4)
	{
		CheckPrize(FID, PRIZE_TYPE_YISHI, UsedUMoney);
	}

	// 행사를 DB에 보관
	if (ActID == 0)
	{
		{
			MAKEQUERY_Religion_InsertAct(strQ, m_SharedPlaceType, FID, NpcID, ActionType, Secret); // 0이면 불교구역을 의미한다.
			CDBConnectionRest	DB(DBCaller);
			DB_EXE_QUERY(DB, Stmt, strQ);
			if (nQueryResult == true)
			{
				DB_QUERY_RESULT_FETCH(Stmt, row);
				if ( IS_DB_ROW_VALID(row) )
				{
					COLUMN_DIGIT(row, 0, uint32, ai);
					ActID = ai;
				}
			}
		}
		m_ReligionActListHistory[NpcID].Reset();
	}
	if (ActID > 0)
	{
		MAKEQUERY_Religion_InsertPray(strQ, ActID, FID, Secret, Pray);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning(L"DB_EXE_PREPARESTMT failed");
			return ActID;
		}
		SQLLEN	len1 = itembuf[0] * sizeof(uint32) + 1;
		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, sizeof(itembuf), 0, itembuf, 0, &len1);
		if (!nBindResult)
		{
			sipwarning(L"DB_EXE_BINDPARAM_IN failed");
			return ActID;
		}
		DB_EXE_PARAMQUERY(Stmt, strQ);
	}

	if ((ActionType == AT_PRAYRITE || ActionType == AT_PRAYMULTIRITE ||
		ActionType == AT_PUBLUCK1 || ActionType == AT_PUBLUCK1_MULTIRITE || 
		ActionType == AT_PUBLUCK2 || ActionType == AT_PUBLUCK2_MULTIRITE || 
		ActionType == AT_PUBLUCK3 || ActionType == AT_PUBLUCK3_MULTIRITE || 
		ActionType == AT_PUBLUCK4 || ActionType == AT_PUBLUCK4_MULTIRITE ) &&
		bYiSiMaster && ItemNum > 0)
	{
		// 행사결과를 위한 행사종류결정
		_YishiResultType	yisiType = YishiResultType_Large;
		if (ActionType == AT_PRAYMULTIRITE || ActionType == AT_PUBLUCK1_MULTIRITE || ActionType == AT_PUBLUCK2_MULTIRITE || ActionType == AT_PUBLUCK3_MULTIRITE || ActionType == AT_PUBLUCK4_MULTIRITE)
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

		FAMILY	*pFamily = GetFamilyData(FID);
		if (pFamily)
			AddSharedYiSiResult(NpcID, yisiType, ActID, FID, pFamily->m_Name);
	}

	return ActID;
}

// 방특수선로를 요청한다
void	CPublicRoomWorld::RequestSpecialChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	ProcRemovedSpecialChannel();
	
	uint32	channelid = m_nCandiSpecialCannel;
	uint8 channel_status = ROOMCHANNEL_SMOOTH;
	CMessage	msgOut(M_SC_CHANNELLIST);
	msgOut.serial(_FID, m_RoomID, channelid, channel_status);
	CUnifiedNetwork::getInstance()->send(_sid, msgOut);

	m_nCandiSpecialCannel++;
	PublicRoomChannelData	data(this, COUNT_PUBLIC_ACTPOS, m_RoomID, channelid, m_SceneID);
	m_mapChannels.insert(make_pair(channelid, data));
	//sipdebug("Special Channel Created. RoomID=%d, ChannelID=%d, FID=%d", m_RoomID, channelid, _FID); byy krs
}

// 방선로목록을 요청한다.
void	CPublicRoomWorld::RequestChannelList(T_FAMILYID _FID, SIPNET::TServiceId _sid)
{
	ProcRemovedSpecialChannel();

	CChannelWorld::RequestChannelList(_FID, _sid);
}

void	cb_M_NT_AUTOPLAY3_TIANDI(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;
	ucstring		FamilyName, TianPray, DiPray;
	uint32			TianItemID, DiItemID, CountPerDay;
	_msgin.serial(FID, FamilyName, TianItemID, TianPray, DiItemID, DiPray, CountPerDay);

	CPublicRoomWorld *inManager = GetPublicWorld(ROOM_ID_PUBLICROOM);
	if (!inManager)
		return;

	inManager->cb_M_NT_AUTOPLAY3_TIANDI(FID, FamilyName, TianItemID, TianPray, DiItemID, DiPray, CountPerDay);
}

void	CPublicRoomWorld::cb_M_NT_AUTOPLAY3_TIANDI(T_FAMILYID FID, ucstring &FamilyName, uint32 TianItemID, ucstring &TianPray, uint32 DiItemID, ucstring &DiPray, uint32 CountPerDay)
{
	if (TianItemID != 0)
	{
		for (uint32 i = 0; i < CountPerDay; i ++)
			SaveAutoplay3Log(FID, FamilyName, ACTPOS_PUB_YISHI_2, TianItemID, TianPray);
	}
	if (DiItemID != 0)
	{
		for (uint32 i = 0; i < CountPerDay; i ++)
			SaveAutoplay3Log(FID, FamilyName, ACTPOS_PUB_YISHI_1, DiItemID, DiPray);
	}
}

static void	DBCB_DBReligion_InsertAct_ForAutoplay3(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID		FID			= *(uint32 *)argv[0];
	T_FAMILYNAME	FamilyName	= (ucchar *)argv[1];
	uint8			NpcID		= *(uint8 *)argv[2];
	uint32			ItemID		= *(uint32 *)argv[3];
	T_FAMILYNAME	Pray		= (ucchar *)argv[4];

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
	{
		sipdebug(L"DBCB_DBReligion_InsertAct_ForAutoplay3 failed. nRows=%d", nRows);
		return;
	}
	uint32 ActID;			resSet->serial(ActID);

	CPublicRoomWorld* pWolrd = GetPublicWorld(ROOM_ID_PUBLICROOM);
	if (!nQueryResult)
		return;

	pWolrd->SaveAutoplay3Log_2(FID, FamilyName, NpcID, ActID, ItemID, Pray);
}

void	CPublicRoomWorld::SaveAutoplay3Log(T_FAMILYID FID, ucstring &FamilyName, uint8 NpcID, uint32 ItemID, ucstring &Pray)
{
	MAKEQUERY_Religion_InsertAct(strQ, m_SharedPlaceType, FID, NpcID, AT_PRAYRITE, 0);
	DBCaller->executeWithParam(strQ, DBCB_DBReligion_InsertAct_ForAutoplay3,
		"DSBDS",
		CB_,		FID,
		CB_,		FamilyName.c_str(),
		CB_,		NpcID,
		CB_,		ItemID,
		CB_,		Pray.c_str()
		);
}

void	CPublicRoomWorld::SaveAutoplay3Log_2(T_FAMILYID FID, ucstring &FamilyName, uint8 NpcID, uint32 ActID, uint32 ItemID, ucstring &Pray)
{
	if (ActID > 0)
	{
		uint8	itembuf[MAX_ITEM_PER_ACTIVITY * sizeof(uint32) + 1];
		itembuf[0] = 1;
		uint32	*itemlist = (uint32*)(&itembuf[1]);
		(*itemlist) = ItemID;

		MAKEQUERY_Religion_InsertPray(strQ, ActID, FID, 0, Pray);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning(L"DB_EXE_PREPARESTMT failed");
			return;
		}
		SQLLEN	len1 = itembuf[0] * sizeof(uint32) + 1;
		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, sizeof(itembuf), 0, itembuf, 0, &len1);
		if (!nBindResult)
		{
			sipwarning(L"DB_EXE_BINDPARAM_IN failed");
			return;
		}
		DB_EXE_PARAMQUERY(Stmt, strQ);
	}

	AddSharedYiSiResult(NpcID, YishiResultType_Large, ActID, FID, FamilyName);
}

//SIPBASE_CATEGORISED_COMMAND(PUBROOM_S, viewLargeAct, "view public act state", "")
//{
//	CPublicRoomWorld *inManager = GetPublicWorld(ROOM_ID_PUBLICROOM);
//	if (!inManager)
//		return true;
//
//	log.displayNL("m_nCurrentLargeActIndex = %d :", inManager->m_nCurrentLargeActIndex);
//	log.displayNL("m_nCurrentLargeActStatus = %d :", inManager->m_nCurrentLargeActStatus);
//	log.displayNL("m_nLargeActCount = %d :", inManager->m_nLargeActCount);
//
//	{
//		LargeAct*	pLargeAct = inManager->m_LargeActs;
//		for (int i = 0; i < inManager->m_nLargeActCount; i ++)
//		{
//			log.displayNL(L"index(%d) -- ActID=%d, Name=%s", i, inManager->m_LargeActs[i].ActID, inManager->m_LargeActs[i].Title.c_str());
//		}
//	}
//	return true;
//}

SIPBASE_CATEGORISED_COMMAND(PUBROOM_S, setLargeActUserMaxNum, "Set large act user max number", "")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	uint32 largeactUserMaxNum = atoui(v.c_str());
	LargeActUserMaxNum.set(largeactUserMaxNum);
	return true;
}

SIPBASE_CATEGORISED_COMMAND(PUBROOM_S, setSharedRoomMaxUsers, "Set shared room user max number", "")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	SharedRoomMaxUsers = atoui(v.c_str());
	return true;
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, setPublicRoomLuckTimeMS, "Set publicroom luck alive time milliseconds", "")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	uint32 uv = atoui(v.c_str());
	s_nPublicRoomLuckTimeMS = uv;
	return true;
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, setPublicRoomLuckActLimitTimeMS, "Set publicroom luck act limit time milliseconds", "")
{
	if(args.size() < 1)	return false;

	string v	= args[0];
	uint32 uv = atoui(v.c_str());
	s_nPublicRoomLuckActLimitTimeMS = uv;
	return true;
}
