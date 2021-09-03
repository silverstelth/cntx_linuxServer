#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>
#include <misc/stream.h>
#include <misc/db_interface.h>
#include "misc/DBCaller.h"

#include <map>
#include <utility>

#include "../Common/DBData.h"
#include "../Common/QuerySetForShard.h"
#include "../Common/DBLog.h"
#include "../Common/netCommon.h"

#include "mst_room.h"
#include "Family.h"
#include "room.h"
#include "outRoomKind.h"
#include "Character.h"

//extern DBCONNECTION	DB;

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller	*DBCaller;

extern	void	SendFamilyProperty_Money(SIPNET::TServiceId _sid, T_FAMILYID _FID, T_PRICE _GMoney, T_PRICE _UMoney);

void	cb_M_CS_LOGIN(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
}

static void	DBCB_DBDelRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32				FID = (*(uint32 *)argv[0]);
	uint32				RoomID = (*(uint32 *)argv[1]);
	TServiceId			_tsid(*(uint16 *)argv[2]);

	if (nQueryResult != true)
		return;

	uint32	uResult = E_COMMON_FAILED;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows != 1)
		return;

	uint32			ret;				resSet->serial(ret);

	switch (ret)
	{
	case 1003:
		{
			uResult = E_NOT_MASTER;
			// ë¹„ë²•?Œì??¸ë°œê²?
			ucstring ucUnlawContent = SIPBASE::_toString("Cannot Delete Room, For not it's master, FID=%u, RoomID=%d", FID, RoomID).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		}
		return;
	case 1032:
		{
			uResult = E_NODELFOROPEN;
			// ë¹„ë²•?Œì??¸ë°œê²?
			ucstring ucUnlawContent = SIPBASE::_toString("Cannot Delete Room, For room is now open, FID=%u, RoomID=%d", FID, RoomID).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		}
		return;
	case 0:
		break;
	default:
		sipwarning("Unknown Error. ret=%d", ret);
		return;
	}

	uResult = ERR_NOERR;

	T_ROOMNAME		roomName;				resSet->serial(roomName);
	uint32			DeleteDay;				resSet->serial(DeleteDay);

	CMessage	msgout(M_SC_DELROOM);
	msgout.serial(FID, uResult, RoomID, DeleteDay);

	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
	//sipinfo(L"->[M_SC_DELROOM] Delete Room : master=%d, Result=%d, room=%d", FID, uResult, RoomID); byy krs

	// Log
	DBLOG_DelRoom(FID, RoomID, roomName);
}

// ¹æÀ» »èÁ¦ÇÑ´Ù
extern CRoomWorld* GetOpenRoomWorld(T_ROOMID _rid);
void	cb_M_CS_DELROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// °¡Á·id
	T_ROOMID			RoomID;				_msgin.serial(RoomID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	CRoomWorld *room = GetOpenRoomWorld(RoomID);
	if (room != NULL)
	{
		if (room->IsAnyFamilyOnRoomlife())
		{
			uint32	result = E_NOEMPTY;
			CMessage	msgout(M_SC_DELROOM);
			msgout.serial(FID, result, RoomID);

			CUnifiedNetwork::getInstance()->send(_tsid, msgout);
			return;
		}

		DeleteRoomWorld(RoomID);
		//sipdebug("Room Deleted. RoomID=%d", RoomID); byy krs
	}

	MAKEQUERY_DelRoom(strQuery, FID, RoomID, g_nRoomDeleteDay);
	DBCaller->executeWithParam(strQuery, DBCB_DBDelRoom,
		"DDW",
		CB_,			FID,
		CB_,			RoomID,
		CB_,			_tsid.get());
}

static void	DBCB_DBUnDelRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32				FID = (*(uint32 *)argv[0]);
	uint32				RoomID = (*(uint32 *)argv[1]);
	TServiceId			_tsid(*(uint16 *)argv[2]);

	if (nQueryResult != true)
		return;

	uint32	uResult = E_COMMON_FAILED;

	uint32 nRows;	resSet->serial(nRows);
	if(nRows != 1)
		return;

	uint32			ret;				resSet->serial(ret);

	switch(ret)
	{
	case 1003:
		{
			uResult = E_NOT_MASTER;
			// ë¹„ë²•?Œì??¸ë°œê²?
			ucstring ucUnlawContent = SIPBASE::_toString("Cannot UnDelete Room, For not it's master, FID=%u, RoomID=%d", FID, RoomID).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		}
		return;
	case 1034:
		{
			sipwarning("Cannot UnDelete Room, For not room is 7day experienced, FID=%u, RoomID=%d", FID, RoomID);
			//uResult = E_NOUNDELFOREXP;
			//// ë¹„ë²•?Œì??¸ë°œê²?
			//ucstring ucUnlawContent = SIPBASE::_toString("Cannot UnDelete Room, For not room is 7day experienced, Family ID:%u, RoomID:%d", FID, RoomID).c_str();
			//RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		}
		return;
	case 0:
		break;
	default:
		sipwarning("Unknown Error. ret=%d", ret);
		return;
	}

	uResult = ERR_NOERR;

	T_ROOMNAME		roomName;				resSet->serial(roomName);

	CMessage	msgout(M_SC_UNDELROOM);
	msgout.serial(FID, uResult, RoomID);
	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
	//sipinfo(L"->[M_SC_UNDELROOM] UnDelete Room : master=%d, Result=%d, room=%d", FID, uResult, RoomID); byy krs

	// Log
	DBLOG_UnDelRoom(FID, RoomID, roomName);
}

// ¹æ»èÁ¦¸¦ Ãë¼ÒÇÑ´Ù
void	cb_M_CS_UNDELROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// °¡Á·id
	T_ROOMID			RoomID;				_msgin.serial(RoomID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShard = false. FID=%d", FID);
		return;
	}

	MAKEQUERY_UnDelRoom(strQuery, FID, RoomID);
	DBCaller->executeWithParam(strQuery, DBCB_DBUnDelRoom,
		"DDW",
		CB_,			FID,
		CB_,			RoomID,
		CB_,			_tsid.get());
}

void	DBCB_DBGetDeceasedForRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_FAMILYID			_FID	= *(T_FAMILYID *)argv[0];
	T_ROOMID			_RoomID	= *(T_ROOMID *)argv[1];
	TServiceId			tsid(*(uint16 *)argv[2]);

	CMessage	msgout(M_SC_DECEASED);
	msgout.serial(_FID);
	uint32	uResult = E_NO_EXIST;
	if (nQueryResult == true)
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

			uResult = ERR_NOERR;
			msgout.serial(uResult, _RoomID, roomname);
			msgout.serial(deadname1, surnamelen1, sex1, birthday1, deadday1, his1, nation1, domicile1, photoid1);
			msgout.serial(phototype);
			msgout.serial(deadname2, surnamelen2, sex2, birthday2, deadday2, his2, nation2, domicile2, photoid2);
		}
		else
		{
			uResult = E_DECEASED_NODATA;
		}
	}
	if (uResult != ERR_NOERR)
	{
		msgout.serial(uResult, _RoomID);
	}

	CUnifiedNetwork::getInstance ()->send(tsid, msgout);
}

void	SendDeadInfo(T_FAMILYID _FID, T_ROOMID _RoomID, CRoomWorld *_RoomManager, TServiceId _tsid)
{
	if (_RoomManager != NULL)
	{
		CMessage	msgout(M_SC_DECEASED);
		msgout.serial(_FID);
		DECEASED *obj = &_RoomManager->m_Deceased1;
		if (obj->IsValid())
		{
			uint32	uResult = ERR_NOERR;
			msgout.serial(uResult, _RoomID, _RoomManager->m_RoomName);
			msgout.serial(obj->m_Name, obj->m_Surnamelen, obj->m_Sex, obj->m_Birthday, obj->m_Deadday, obj->m_His, obj->m_Nation, obj->m_Domicile, obj->m_PhotoId);
			msgout.serial(_RoomManager->m_PhotoType);
			DECEASED *obj2 = &_RoomManager->m_Deceased2;
			msgout.serial(obj2->m_Name, obj2->m_Surnamelen, obj2->m_Sex, obj2->m_Birthday, obj2->m_Deadday, obj2->m_His, obj2->m_Nation, obj2->m_Domicile, obj2->m_PhotoId);
		}
		else
		{
			uint32	uResult = E_NO_EXIST;
			msgout.serial(uResult, _RoomID);
		}
		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
		return;
	}
	else
	{
		MAKEQUERY_GetDeceasedForRoom(strQ, _RoomID);
		DBCaller->executeWithParam(strQ, DBCB_DBGetDeceasedForRoom,
			"DDW",
			CB_,			_FID,
			CB_,			_RoomID,
			CB_,			_tsid.get());
	}
}

static void	DBCB_DBFindRoom(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32				ret		= *(sint32 *)argv[0];
	uint32				nAll	= *(uint32 *)argv[1];
	uint32				curcount= *(uint32 *)argv[2];
	T_FAMILYID			FID		= *(T_FAMILYID *)argv[3];
	uint8				target	= *(uint8 *)argv[4];
	ucstring			ucFind	= (ucchar *)argv[5];
	uint16				reqPageNo	= *(uint16 *)argv[6];
	TServiceId			tsid(*(uint16 *)argv[7]);

	if ( ! nQueryResult )
	{
		sendErrorResult(M_SC_FINDROOMRESULT, FID, E_DBERR, tsid);
		return;
	}

	if ( ret != 0 )
	{
		if(ret == 1010) // No Data
		{
			CMessage	msgout(M_SC_FINDROOMRESULT);
			uint32	error = ERR_NOERR;
			msgout.serial(FID, error, target, ucFind, nAll, reqPageNo);
			CUnifiedNetwork::getInstance()->send(tsid, msgout);
		}
		else
			sendErrorResult(M_SC_FINDROOMRESULT, FID, ret, tsid);
		return;
	}

	CMessage	msgout(M_SC_FINDROOMRESULT);
	uint32	error = ERR_NOERR;
	msgout.serial(FID, error, target, ucFind, nAll, reqPageNo);

	uint32 nRows;	resSet->serial(nRows);
	for ( uint32 i = 0; i < nRows; i ++)
	{
		uint32			VisitCount;				resSet->serial(VisitCount);
		T_ROOMLEVEL		level;					resSet->serial(level);
		T_ROOMEXP		exp;					resSet->serial(exp);
		T_ROOMNAME		roomname;				resSet->serial(roomname);
		T_ROOMID		roomid;					resSet->serial(roomid);
		T_ID			familyid;				resSet->serial(familyid);
		T_SCENEID		sceneid;				resSet->serial(sceneid);
		T_FLAG			openflag;				resSet->serial(openflag);
		T_FAMILYNAME	mastername;				resSet->serial(mastername);
		uint8			deadSurnameLen1;		resSet->serial(deadSurnameLen1);
		T_DECEASED_NAME	deadname1;				resSet->serial(deadname1);
		ucstring		domicile1;				resSet->serial(domicile1);
		ucstring		nation1;				resSet->serial(nation1);
		T_SEX			deadsex1;				resSet->serial(deadsex1);
		uint32			birthday1;				resSet->serial(birthday1);
		uint32			deadday1;				resSet->serial(deadday1);
		uint8			deadSurnameLen2;		resSet->serial(deadSurnameLen2);
		T_DECEASED_NAME	deadname2;				resSet->serial(deadname2);
		ucstring		domicile2;				resSet->serial(domicile2);
		ucstring		nation2;				resSet->serial(nation2);
		T_SEX			deadsex2;				resSet->serial(deadsex2);
		uint32			birthday2;				resSet->serial(birthday2);
		uint32			deadday2;				resSet->serial(deadday2);
		T_DATETIME		termtime;				resSet->serial(termtime);
		uint8			PhotoType;				resSet->serial(PhotoType);

		uint32	nTermTime = getMinutesSince1970(termtime);
		//uint16	ExpPercent = CalcRoomExpPercent(level, exp);

		msgout.serial(roomid, level, exp); // , ExpPercent);
		msgout.serial(familyid, sceneid, roomname, openflag, mastername, VisitCount, nTermTime, PhotoType);
		msgout.serial(deadname1, deadSurnameLen1, deadsex1, domicile1, nation1, birthday1, deadday1);
		msgout.serial(deadname2, deadSurnameLen2, deadsex2, domicile2, nation2, birthday2, deadday2);
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

void	cb_M_CS_FINDROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	uint32		FID;		_msgin.serial(FID);
	uint8		target;		_msgin.serial(target);
	uint16		reqPageNo;	_msgin.serial(reqPageNo);
	uint16		numPerPage;	_msgin.serial(numPerPage);
	uint32      lpageNo = (uint32)reqPageNo;
	uint32      lnumPerPage = (uint32)numPerPage;
	const		uint16	limitNumPerPage = 30;
	if ( numPerPage >= limitNumPerPage )
	{
		ucstring ucUnlawContent = SIPBASE::_toString("Cannot Find Room, Because Search result number per Page(num:%u) is larger than Limit(num:%u)", numPerPage, limitNumPerPage).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	ucstring	ucFind;		SAFE_SERIAL_WSTR(_msgin, ucFind, MAX_LEN_ROOM_FINDSTRING, FID)
	//sipinfo(L"User(FamilyId:%u) require Finding Rooms, Target:%u, RequestPageNo:%u, numPerPage:%u, FindString:%s", FID, target, reqPageNo, numPerPage, ucFind.c_str()); byy krs

	MAKEQUERY_FindRoom(strQuery, target, FIND_CORRECT, lpageNo, ucFind, lnumPerPage);
	DBCaller->executeWithParam2(strQuery, 10000, DBCB_DBFindRoom,
		"DDDDBSWW",
		OUT_,		NULL,
		OUT_,		NULL,
		OUT_,		NULL,
		CB_,		FID,
		CB_,		target,
		CB_,		ucFind.c_str(),
		CB_,		reqPageNo,
		CB_,		_tsid.get());
}

/****************************************************************************
* ì§€?¹ë“¤
****************************************************************************/

static void	DBCB_DBAddFestival(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32		ret			= *(sint32 *)argv[0];
	T_FAMILYID	idUser		= *(T_FAMILYID *)argv[1];
	TServiceId	tsid(*(uint16 *)argv[2]);

	uint32		uResult = ERR_NOERR;
	try
	{
		if ( ! nQueryResult || ret < 0 )
			throw "Shrd_festival_add: execute failed!";

		uint32	idFestival = ret;
		CMessage	msgout(M_SC_REGFESTIVAL);
		msgout.serial(idUser);
		msgout.serial(uResult);
		msgout.serial(idFestival);
		CUnifiedNetwork::getInstance ()->send(tsid, msgout);
		//sipinfo("Registration Festival success:%d, id : %d", uResult, idFestival); byy krs
	}
	catch(char * str)
	{
		sipwarning("[FID=%d] %s", idUser, str);
		sendErrorResult(M_SC_REGFESTIVAL, idUser, E_DBERR, tsid);
	}
}
// Festival
// packet : M_CS_REGFESTIVAL
// client -> ORS
void	cb_M_CS_REGFESTIVAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32			idUser;			_msgin.serial(idUser);
	T_DATE4			date;			_msgin.serial(date);
	ucstring		nameFestival;	SAFE_SERIAL_WSTR(_msgin, nameFestival, MAX_LEN_FESTIVAL_NAME, idUser);
	uint8			alarmdays;		_msgin.serial(alarmdays);

	MAKEQUERY_ADDFESTIVAL(strQ, idUser, nameFestival, date, alarmdays);
	DBCaller->executeWithParam(strQ, DBCB_DBAddFestival,
		"DDW",
		OUT_,			NULL,
		CB_,		idUser,
		CB_,			_tsid.get());
}

// packet : M_CS_MODIFYFESTIVAL
// client -> ORS
void	cb_M_CS_MODIFYFESTIVAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32			idUser;			_msgin.serial(idUser);
	uint32			idFestival;		_msgin.serial(idFestival);
    T_DATATIME      date;			_msgin.serial(date);
	ucstring		nameFestival;	SAFE_SERIAL_WSTR(_msgin, nameFestival, MAX_LEN_FESTIVAL_NAME, idUser);
	uint8			alarmdays;		_msgin.serial(alarmdays);

	MAKEQUERY_MODIFYFESTIVAL(strQ, idFestival, nameFestival, date, alarmdays);
	DBCaller->executeWithParam(strQ, NULL,
		"D",
		OUT_,		NULL);

	uint32		uResult = ERR_NOERR;
	CMessage	msgout(M_SC_MODIFYFESTIVAL);
	msgout.serial(idUser);
	msgout.serial(uResult);
	msgout.serial(idFestival);
	CUnifiedNetwork::getInstance ()->send(_tsid, msgout);
}

// packet : M_CS_DELFESTIVAL
// client -> ORS
void	cb_M_CS_DELFESTIVAL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32			idUser;			_msgin.serial(idUser);
	uint32			idFestival;		_msgin.serial(idFestival);

	MAKEQUERY_DELFESTIVAL(strQ);
	DBCaller->executeWithParam(strQ, NULL,
		"DD",
		IN_,		idFestival,
		OUT_,		NULL);

	uint32		uResult = ERR_NOERR;
	CMessage	msgout(M_SC_DELFESTIVAL);
	msgout.serial(idUser);
	msgout.serial(uResult);
	msgout.serial(idFestival);

	CUnifiedNetwork::getInstance ()->send(_tsid, msgout);
	//sipinfo("Delete Festival success:%d, id : %d", uResult, idFestival); byy krs
}

// packet : M_CS_SET_FESTIVALALARM
// client -> ORS/Lobby
void cb_M_CS_SET_FESTIVALALARM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID = 0;		 _msgin.serial(FID);
	uint8  festivalType = 0;     _msgin.serial(festivalType);
	uint32 festivalId = 0;       _msgin.serial(festivalId);
	uint8  remindDays = 0;       _msgin.serial(remindDays);
	//sipinfo("fid=%u, %d, %d, %d", FID, festivalType, festivalId, remindDays); byy krs

	MAKEQUERY_INSERTFESTIVALREMIND(strQ, FID, festivalType, festivalId, remindDays);
	DBCaller->executeWithParam(strQ, NULL,
		"D",
		OUT_,		NULL);

	uint32 uResult = ERR_NOERR;
	CMessage msgout(M_SC_SET_FESTIVALALARM);
	msgout.serial(FID);
	msgout.serial(uResult);
	CUnifiedNetwork::getInstance ()->send(_tsid, msgout);
}

void NotifyChangeJingPinTianYuanInfo(uint32 RoomID)
{
	// Master OROOM ½áºñ½º¿¡ ÅëÁö
	CMessage	msgOut(M_NT_EXPROOM_INFO_RELOAD);
	msgOut.serial(RoomID);
	CUnifiedNetwork::getInstance()->send(MANAGER_SHORT_NAME, msgOut, false);
}

TTime GetUpdateNightTime(TTime curTime)
{
	return ((TTime)(curTime / SECOND_PERONEDAY) + 1) * (TTime)SECOND_PERONEDAY + 0;	// ¹ã12½Ã 0ºÐ
}
static TTime	NextUpdateJingPinTianYuanTime = 0;
static TTime	LastUpdateJingPinTianYuanTryTime = 0;
void DoEverydayWork()
{
	// ¹ã12½Ã¿¡ Á¤Ç°Ãµ¿øÀÚ·á¸¦ °»½ÅÇÑ´Ù.
	TTime	curTime = GetDBTimeSecond();

//	// ¹ã12½ÃºÎÅÍ 30ºÐµ¿¾È 5ºÐ°£°ÝÀ¸·Î ½ÃµµÇØº»´Ù.
	if(NextUpdateJingPinTianYuanTime == 0)
	{
		NextUpdateJingPinTianYuanTime = GetUpdateNightTime(curTime);
		return;
	}
	if(NextUpdateJingPinTianYuanTime >= curTime)
	{
		return;
	}
	if(NextUpdateJingPinTianYuanTime + 1800 < curTime)
	{
		NextUpdateJingPinTianYuanTime = GetUpdateNightTime(curTime);
		return;
	}

	if(LastUpdateJingPinTianYuanTryTime + 300 < curTime)
	{
		LastUpdateJingPinTianYuanTryTime = curTime;
		// ÇöÀç Á¢¼ÓÀÚ¼ö°¡ 10¸í¹Ì¸¸ÀÏ¶§¸¸ ÇÑ´Ù.
//		if(map_family.size() < 10) ==== ¹ã12½Ã¿¡ ¹Ýµå½Ã ÇØ¾ß ÇÏ´Â ÀÛ¾÷µµ ¸¹À¸¹Ç·Î ...;;
		{
			NextUpdateJingPinTianYuanTime = GetUpdateNightTime(curTime);

			DoEverydayWorkInRoom();
		}
	}
}


// Command
SIPBASE_CATEGORISED_COMMAND(OROOM_S, optest, "displays all information of open room", "")
{
	return true;
}

