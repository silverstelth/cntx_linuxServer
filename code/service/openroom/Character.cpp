#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>

#include "misc/db_interface.h"
#include "../Common/QuerySetForShard.h"
#include "misc/DBCaller.h"
#include "../Common/DBData.h"
#include "../Common/DBLog.h"
#include "Family.h"

#include "Character.h"
#include "Inven.h"
#include "World.h"
#include "room.h"
#include "mst_room.h"
#include "outRoomKind.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

//extern DBCONNECTION	DB;

MAP_CHARACTER				map_character;

// temp code
extern CDBCaller	*DBCaller;

//// 자료기지에서 캐릭터테이블을 load한다
//void	LoadCharacterData(T_CHARACTERID _CHID)
//{
//	if ( map_character.find(_CHID) != map_character.end() )
//		return;
//	//DelCharacterData(_CHID);
//
//	uint8 flag = 1;
//	MAKEQUERY_GetCHInfo(strQuery, _CHID, flag);
//	CDBConnectionRest	DB(DBCaller);
//	DB_EXE_QUERY(DB, Stmt, strQuery);
//	if (nQueryResult == TRUE)
//	{
//		do
//		{
//			DB_QUERY_RESULT_FETCH(Stmt, row);
//			if (IS_DB_ROW_VALID(row))
//			{
//				COLUMN_WSTR(row, 0, name, MAX_LEN_CHARACTER_NAME);
//				COLUMN_DIGIT(row, 1, T_FAMILYID, familyid);
//				COLUMN_DIGIT(row, 2, T_MODELID, modelid);
//				COLUMN_DIGIT(row, 3, T_DRESS, ddid);
//				COLUMN_DIGIT(row, 4, T_FACEID, faceid);
//				COLUMN_DIGIT(row, 5, T_HAIRID, hairid);
//				COLUMN_DIGIT(row, 6, T_DRESS, did);
//				COLUMN_DIGIT(row, 7, T_ISMASTER, ismaster);
//				COLUMN_DIGIT(row, 8, T_CHBIRTHDATE, birthDate);
//				COLUMN_DIGIT(row, 9, T_CHCITY, city);
//				COLUMN_WSTR(row, 10, resume, MAX_RESUME_NUM);
//
//				AddCharacterData( CHARACTER(_CHID, name, familyid, ismaster, modelid, ddid, faceid, hairid, did, birthDate, city, resume) );
//			}
//			else
//				break;
//		} while(TRUE);
//	}
//}

// 캐릭터정보를 추가한다
void	AddCharacterData(CHARACTER& _newCH)
{
	//sipdebug("map_character insert. CHID=%d", _newCH.m_ID); byy krs
	map_character.insert( make_pair(_newCH.m_ID, _newCH) );
}
// 캐릭터정보를 삭제한다
void	DelCharacterData(T_CHARACTERID _CHID)
{
	MAP_CHARACTER::iterator it = map_character.find(_CHID);
	if (it != map_character.end())
	{
		//sipdebug("map_character erase. CHID=%d", _CHID); byy krs
		map_character.erase(it);
	}
	
}
// 가족에 속한 캐릭터가 맞는가를 검사한다
bool	IsInFamily(T_FAMILYID _FID, T_CHARACTERID _CHID)
{
	MAP_CHARACTER::iterator	it = map_character.find(_CHID);
	if (it == map_character.end())
		return false;
	if (it->second.m_FamilyID == _FID)
		return true;
	return false;
}
// 캐릭터정보를 얻는다
CHARACTER	*GetCharacterData(T_CHARACTERID _CHID)
{
	MAP_CHARACTER::iterator it = map_character.find(_CHID);
	if (it == map_character.end())
		return NULL;
	return &(it->second);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

/*
// 모든 캐릭터정보를 얻는다.
void	cb_M_CS_ALLCHARACTER(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_ACCOUNTID		uAccountID;					// account id
	_msgin.serial(uAccountID);

	CMessage	msgOut(M_SC_ALLCHARACTER);
	msgOut.serial( uAccountID );				// 응답을 받을 대상

	MAKEQUERY_GETCHARACTERS(strQ, uAccountID);
	DB_EXE_QUERY(DB, Stmt, strQ);
	if (nQueryResult == TRUE)
	{
		do 
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (IS_DB_ROW_VALID(row))
			{
				COLUMN_DIGIT(row, 0, T_CHARACTERID, uid);
				COLUMN_WSTR(row, 1, name, MAX_LEN_CHARACTER_NAME);
				COLUMN_DIGIT(row, 2, T_MODELID, modelid);
				COLUMN_DIGIT(row, 3, T_DRESS, ddressid);
				COLUMN_DIGIT(row, 4, T_HEADID, headid);
				COLUMN_DATETIME(row, 5, createdate);
				COLUMN_DIGIT(row, 6, T_ISMASTER, master);
				COLUMN_DIGIT(row, 7, T_DRESS, dressid);
				COLUMN_DATETIME(row, 8, changedate);
				msgOut.serial(uid, name, modelid, ddressid, headid, createdate, master);
				msgOut.serial(dressid, changedate);
			}
			else
				break;
		} while(TRUE);
	}
	sipinfo(L"Send character info to account = %d", uAccountID);
	CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
}
*/

//// 새로운 캐릭터를 창조한다.
//void	cb_M_CS_NEWCHARACTER(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
	//T_ACCOUNTID		uAccountID;		_msgin.serial(uAccountID);
	//T_CHARACTERNAME	ucName;			SAFE_SERIAL_WSTR(_msgin, ucName, MAX_LEN_CHARACTER_NAME, uAccountID);
	//T_MODELID		modelid;		_msgin.serial(modelid);
	//T_DRESS			dressid;		_msgin.serial(dressid);
	//T_HEADID		headid;			_msgin.serial(headid);

	//MAKEQUERY_NEWCHARACTER(strQuery, uAccountID, ucName.c_str(), modelid, dressid, headid);
	//DB_EXE_QUERY(DB, Stmt, strQuery);
	//if (nQueryResult == TRUE)
	//{
	//	DB_QUERY_RESULT_FETCH(Stmt, row);
	//	if (IS_DB_ROW_VALID(row))
	//	{
	//		COLUMN_DIGIT(row, 0, uint32, nResult);
	//		COLUMN_DIGIT(row, 1, uint32, nVal);

	//		uint32	nCode = E_DBERR;
	//		if (nResult == false)
	//		{
	//			switch( nVal )
	//			{
	//			case 1:			nCode = E_INVALID_CH_PARAM;						break;
	//			case 2:			nCode = E_CH_FULL;								break;
	//			case 3:			nCode = E_CHNAME_DOUBLE;						break;
	//			}
	//		}
	//		// 클라이언트에 통보한다
	//		CMessage	msgOut(M_SC_NEWCHARACTER);
	//		if (nResult == TRUE)
	//		{
	//			msgOut.serial(uAccountID, nResult, nVal);
	//			msgOut.serial(nVal, ucName, modelid, dressid, headid);
	//		}
	//		else
	//			msgOut.serial(uAccountID, nResult, nCode, ucName);
	//		sipinfo(L"New character : result = %d, value = %d, accountid = %d, name = %s", nResult, nVal, uAccountID, ucName.c_str());
	//		CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
	//		return;
	//	}
	//}
	//// DB오유
	//CMessage	msgOut(M_SC_NEWCHARACTER);
	//uint32 nResult = FALSE, nVal = DB_ERROR;
	//msgOut.serial(uAccountID, nResult, nVal, ucName);
	//sipwarning(L"DBError(QUERY_NEWCHARACTER) : accountid = %d, name = %s ", uAccountID, ucName.c_str());

	//CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
//}

//// 캐릭터를 삭제한다.
//void	cb_M_CS_DELCHARACTER(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
	//T_ACCOUNTID		uAccountID;					// account id
	//T_CHARACTERID	uCHID;						// character id
	//_msgin.serial(uAccountID, uCHID);

	//uint32 nResult = TRUE, nVal = uCHID;

	//MAKEQUERY_DELCHARACTER(strQ, uAccountID, uCHID);
	//DB_EXE_QUERY(DB, Stmt, strQ);
	//if (nQueryResult == TRUE)
	//{
	//	DB_QUERY_RESULT_FETCH(Stmt, row);
	//	if (IS_DB_ROW_VALID(row))
	//	{
	//		COLUMN_DIGIT(row, 0, uint32, nR);
	//		if (nR == ERR_NOERR)
	//		{
	//			// 클라이언트에게 통보한다
	//			CMessage	msgOut(M_SC_DELCHARACTER);
	//			msgOut.serial(uAccountID, nResult, nVal, uCHID);
	//			sipinfo(L"Del character : result = %d, value = %d, accountid = %d, CHID = %d", nResult, nVal, uAccountID, uCHID);
	//			CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
	//			return;
	//		}
	//	}
	//}
//}

extern 	void SetFamilyNameForOpenRoom(T_FAMILYID FID, T_FAMILYNAME FamilyName);

void cb_M_NT_CHANGE_NAME(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32		FID;
	ucstring	FamilyName;

	_msgin.serial(FID);
	SAFE_SERIAL_WSTR(_msgin, FamilyName, MAX_LEN_FAMILYUSER_NAME, FID);

	FAMILY	*pFamily = GetFamilyData(FID);
	if(pFamily)
		pFamily->m_Name = FamilyName;

	SetFamilyNameForOpenRoom(FID, FamilyName);
	AddFamilyNameData(FID, FamilyName);

	//ModifyJingPinTianYuanRoommasterName(FID, FamilyName);
}

static void	DBCB_DBUpdateCharacter(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ERR	err;
	if ( !nQueryResult )
		err = E_DBERR;

	T_FAMILYID			FID		= *(T_FAMILYID *)argv[0];
	T_CHARACTERID		CHID	= *(T_CHARACTERID *)argv[1];
	T_MODELID			modelid	= *(T_MODELID *)argv[2];
	T_DRESS				dressid	= *(T_DRESS *)argv[3];
	T_FACEID			faceid	= *(T_FACEID *)argv[4];
	T_HAIRID			hairid	= *(T_HAIRID *)argv[5];
	T_CHARACTERNAME		chname	= (ucchar *)argv[6];
	T_PRICE				UPrice	= *(T_PRICE *)argv[7];
	T_PRICE				GPrice	= *(T_PRICE *)argv[8];
	TServiceId			tsid(*(uint16 *)argv[9]);

	//Character정보 갱신
	CWorld	*pWorld = GetWorldFromFID(FID);
	if(pWorld != NULL)
	{
		TRoomFamilys::iterator	itFamily = pWorld->m_mapFamilys.find(FID);
		if(itFamily != pWorld->m_mapFamilys.end())
		{
			std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator	itCh = itFamily->second.m_mapCH.find(CHID);
			if(itCh != itFamily->second.m_mapCH.end())
			{
				itCh->second.m_ModelID = modelid;
			}
		}
	}

	// 이름이 바뀐 경우에만 게임돈을 리용한다.
	if((UPrice > 0) || (GPrice > 0))
	{
		// Family정보 갱신
		FAMILY	*pFamily = GetFamilyData(FID);
		if(pFamily)
			pFamily->m_Name = chname;

		AddFamilyNameData(FID, chname);
		// 다른 모든 써비스들에 통지한다.
		{
			CMessage	msgOut(M_NT_CHANGE_NAME);
			msgOut.serial(FID, chname);
			CUnifiedNetwork::getInstance()->send(OROOM_SHORT_NAME, msgOut);
			CUnifiedNetwork::getInstance()->send(RROOM_SHORT_NAME, msgOut);
			CUnifiedNetwork::getInstance()->send(LOBBY_SHORT_NAME, msgOut);
			CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
		}

		ExpendMoney(FID, UPrice, GPrice, 1, DBLOG_MAINTYPE_CHANE_CH, 0, 0, 0, 0, UPrice, 0);

		INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
		if ( pFamilyInfo == NULL )
		{
			sipdebug("GetFamilyInfo(%d) = NULL", FID);
			return;
		}

		SendFamilyProperty_Money(tsid, FID, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);
	}

	// Main에 통지 - 이름이 바뀐 경우, 성별이 바뀐 경우, 그러나 성별이 바뀐 경우를 구별하지 못하므로 늘 보낸다.
	{
		CMessage	msgOut(M_NT_CHANGE_NAME);
		msgOut.serial(FID, chname, modelid);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}

	// 보내지 않는것으로 토의함
	//uint32 nResult = ERR_NOERR;
	//CMessage mesOut(M_SC_CHCHARACTER);
	//mesOut.serial(FID, nResult, CHID, modelid, dressid, faceid, hairid, chname);
	//CUnifiedNetwork::getInstance ()->send(tsid, mesOut);

	DBLOG_ChangeCh(FID, UPrice, GPrice);
}

void	cb_M_CS_CHCHARACTER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;
	T_CHARACTERID		CHID;
	T_MODELID			modelid;
	T_DRESS				dressid;
	T_FACEID			faceid;
	T_HAIRID			hairid;
	uint32				city;
	ucstring			chname;
	_msgin.serial(FID, CHID, modelid, dressid, faceid, hairid, city);
	SAFE_SERIAL_WSTR(_msgin, chname, MAX_LEN_FAMILYUSER_NAME, FID);

	if (!IsThisShardFID(FID))
	{
		sipwarning("IsThisShardFID = false. FID=%d", FID);
		return;
	}

	T_PRICE		GPrice = g_nCharChangePrice;
	T_PRICE		UPrice = 0;

	FAMILY	*pFamily = GetFamilyData(FID);
	if(pFamily)
	{
		// 이름이 바뀌지 않은 경우에는 게임돈을 사용하지 않는다.
		if(pFamily->m_Name == chname)
		{
			GPrice = 0;
		}
	}
	if((GPrice > 0) || (UPrice > 0))
	{
		if ( ! CheckMoneySufficient(FID, UPrice, GPrice, MONEYTYPE_POINT) )
		{
			sipwarning("NoMoney User Request !!! FID=%d", FID);
			return;
		}
	}
	
	uint32		sexid = GetModelSex(modelid);
	MAKEQUERY_UpdateCharacter(strQ, FID, CHID, modelid, sexid, dressid, faceid, hairid, city, chname.c_str());
	DBCaller->execute(strQ, DBCB_DBUpdateCharacter,
		"DDDDDDSDDW",
		CB_,		FID,
		CB_,		CHID,
		CB_,		modelid,
		CB_,		dressid,
		CB_,		faceid,
		CB_,		hairid,
		CB_,		chname.c_str(),
		CB_,		UPrice,
		CB_,		GPrice,
		CB_,		_tsid.get());
}

static void	DBCB_DBGetCharacterForSend(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ERR	err;
	if (!nQueryResult)
		err = E_DBERR;

	T_FAMILYID			FID			= *(T_FAMILYID *)argv[0];
	T_FAMILYID			TargetId	= *(T_FAMILYID *)argv[1];
	TServiceId			tsid(*(uint16 *)argv[2]);

	CMessage	msgOut(M_SC_ALLCHARACTER);
	msgOut.serial(FID, TargetId);
	uint32 nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i ++)
	{
		T_CHARACTERID	uid;			resSet->serial(uid);
		T_COMMON_NAME	name;			resSet->serial(name);
		T_MODELID		modelid;		resSet->serial(modelid);
		T_DRESS			ddressid;		resSet->serial(ddressid);
		T_FACEID		faceid;			resSet->serial(faceid);
		T_HAIRID		hairid;			resSet->serial(hairid);
		T_DATETIME		createdate;		resSet->serial(createdate);
		T_ISMASTER		master;			resSet->serial(master);
		T_DRESS			dressid;		resSet->serial(dressid);
		T_DATETIME		changedate;		resSet->serial(changedate);
		T_CHBIRTHDATE	birthDate;		resSet->serial(birthDate);
		T_CHCITY		city;			resSet->serial(city);
		T_COMMON_CONTENT resume;		resSet->serial(resume);
		T_F_LEVEL		fLevel;			resSet->serial(fLevel);
		T_F_EXP			fExp;			resSet->serial(fExp);
		//uint16	ExpPercent = CalcFamilyExpPercent(fLevel, fExp);

		msgOut.serial(fLevel, fExp);//, ExpPercent);
		msgOut.serial(uid, name, modelid, ddressid, faceid, hairid, createdate, master);
		msgOut.serial(dressid, changedate);
		msgOut.serial(birthDate, city, resume);
	}
	//sipinfo(L"Send character info %u to %u", TargetId, FID); byy krs
	CUnifiedNetwork::getInstance ()->send(tsid, msgOut);
}

void	SendCharacterInfo(T_FAMILYID _FID, SIPNET::TServiceId _tsid, T_FAMILYID _TargetId)
{
	if (_TargetId == 0)
		_TargetId = _FID;

	MAKEQUERY_GetFamilyCharacters(strQ, _TargetId);
	DBCaller->execute(strQ, DBCB_DBGetCharacterForSend,
		"DDW",
		CB_,		_FID,
		CB_,		_TargetId,
		CB_,		_tsid.get());
}

void	cb_M_CS_REQ_FAMILYCHS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_FAMILYID		TargetId;	_msgin.serial(TargetId);
	SendCharacterInfo(FID, _tsid, TargetId);
}

static void	DBCB_DBChangeCHDesc(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	T_ERR	err;
	if ( !nQueryResult )
		err = E_DBERR;

	T_CHARACTERID		chId		= *(T_CHARACTERID *)argv[0];
	T_CHBIRTHDATE		BirthDate	= *(T_CHBIRTHDATE *)argv[1];
	T_CHCITY			city		= *(T_CHCITY *)argv[2];
	T_CHRESUME			resume		= (ucchar *)argv[3];
	T_FAMILYID			fid			= *(T_FAMILYID *)argv[4];
	TServiceId			tsid(*(uint16 *)argv[5]);

	uint32 nRows;	resSet->serial(nRows);
	if (nRows == 1)
	{
		sint32	resultCode;		resSet->serial(resultCode);

		// 보내지 않는것으로 토의함
		//CMessage msgout(M_SC_CHANGE_CHDESC);
		//msgout.serial(fid);
		//msgout.serial(resultCode);
		//msgout.serial(chId);
		//CUnifiedNetwork::getInstance()->send(tsid, msgout);

		if ( resultCode == ERR_NOERR )
		{
			CHARACTER	*character	= GetCharacterData(chId);
			if ( !character )
			{
				sipwarning("GetCharacterData = NULL. CHID=%d", chId);
				return;
			}

			character->m_BirthDate = BirthDate;
			character->m_City = city;
			character->m_Resume = resume;
		}
	}
	else
	{
		sipwarning("[FID=%d] %s", fid, "Shrd_Character_Update_Birthdate: fetch failed!");
		// 보내지 않는것으로 토의함
		//CMessage msgout(M_SC_CHANGE_CHDESC);
		//msgout.serial(fid);
		//uint32 resultCode = E_DBERR;
		//msgout.serial(resultCode);
		//msgout.serial(chId);	
		//CUnifiedNetwork::getInstance ()->send(tsid, msgout);
	}
}


void	cb_M_CS_CHANGE_CHDESC(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		fid = 0;
	T_CHARACTERID	chId = 0;
	T_CHBIRTHDATE	BirthDate = 0;
	T_CHCITY		city = 0;
	ucstring		resume; 

	_msgin.serial(fid, chId, BirthDate, city);
	NULLSAFE_SERIAL_WSTR(_msgin, resume, MAX_LEN_FAMILYCOMMENT, fid);

	if (!IsThisShardFID(fid))
	{
		sipwarning("IsThisShardFID = false. FID=%d", fid);
		return;
	}

	MAKEQUERY_UpdateCHInfo(strQ, fid, BirthDate, city, resume);
	DBCaller->executeWithParam(strQ, DBCB_DBChangeCHDesc,
		"DDDSDW",
		CB_,		chId,
		CB_,		BirthDate,
		CB_,		city,
		CB_,		resume.c_str(),
		CB_,		fid,
		CB_,		_tsid.get());
}

/****************************************************************************
* 함수:   M_CS_CHDESC
*         수집방을 추가한다
****************************************************************************/
//void	cb_M_CS_CHDESC(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID fid = 0;
//	T_CHARACTERID chId = 0;
//	_msgin.serial(fid);
//	_msgin.serial(chId);
//
//	CHARACTER	*character	= GetCharacterData(chId);
//	if( !character )
//		return;
//
//	CMessage msgout(M_SC_CHDESC);
//	msgout.serial(fid);
//	msgout.serial(chId);
//	msgout.serial(character->m_BirthDate);
//	msgout.serial(character->m_City);
//	msgout.serial(character->m_Resume);
//
//	CUnifiedNetwork::getInstance ()->send(_tsid, msgout);
//}
