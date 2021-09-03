#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>

#include "mst_room.h"
#include "../../common/Macro.h"

#include "misc/db_interface.h"
#include "misc/DBCaller.h"
#include "../Common/QuerySetForShard.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

//extern DBCONNECTION	DB;
extern CDBCaller		*DBCaller;

// ----------------------------------------------------------------------------
MAP_MST_ROOM				map_mstRoom;
MAP_MST_CONFIG				map_mstConfig;
MAP_MST_ITEM				map_MST_ITEM;

_Mst_Room_Exp_Type		mstRoomExp[ROOM_EXP_TYPE_MAX];
_Mst_Room_Level			mstRoomLevel[ROOM_LEVEL_MAX + 1];
_Mst_Family_Exp_Type	mstFamilyExp[FAMILY_EXP_TYPE_MAX];
_Mst_Family_Level		mstFamilyLevel[FAMILY_LEVEL_MAX + 1];
_Mst_Fish_Level			mstFishLevel[FISH_LEVEL_MAX + 1];

_Mst_Family_Virtue_Type	mstFamilyVirtue0[FAMILY_VIRTUE0_TYPE_MAX];

std::string			mstBadFIDList;

// Map에서 계속 참조하는것이 속도상 불리하기때문에 변수를 꺼내놓음.
int		g_nFishExpReducePercent;
uint32	g_nCharChangePrice;
uint32	g_nRoomSmoothFamPercent;
uint32	g_nRoomDeleteDay;
uint32	g_nSamePromotionPrice;	// 동급천원 승급시에만 적용되는 가격이다.
uint32	g_nSamePromotionCount;	// 동급천원승급가능회수
//uint32	g_nRoomcardBackRate;	// 천원카드사용시에 되돌려주는 자은비의 비률

// ??????? ???
MST_ROOM	*GetMstRoomData(T_MSTROOM_ID _ID)
{
	MAP_MST_ROOM::iterator it = map_mstRoom.find(_ID);
	if (it == map_mstRoom.end())
		return NULL;
	return &(it->second);
}

MST_ROOM	*GetMstRoomData_FromSceneID(uint16 SceneID, T_MSTROOM_YEARS _years, T_MSTROOM_MONTHS _months, T_MSTROOM_DAYS _days)
{
	MAP_MST_ROOM::iterator it;
	for (it = map_mstRoom.begin(); it != map_mstRoom.end(); it ++)
	{
		MST_ROOM	*room = &(it->second);
		if ( room->m_SceneID == SceneID &&
			room->m_Years == _years &&
			room->m_Months == _months &&
			room->m_Days == _days )

			return &(it->second);
	}

	return NULL;
}

MST_ROOM	*GetMstRoomData_FromSceneID_FreeRoom(uint16 SceneID)
{
	MAP_MST_ROOM::iterator it;
	for ( it = map_mstRoom.begin(); it != map_mstRoom.end(); it ++ )
	{
		MST_ROOM	*room = &(it->second);
		if ( room->m_SceneID == SceneID && room->GetRealPrice() == 0 )

			return &(it->second);
	}

	return NULL;
}

// ?? ??, ???? master??? ???
void	SendMstRoomData(T_FAMILYID	_FID, TServiceId _tsid)
{
	if (map_mstRoom.size() < 1)
		return;
	CMessage	msgOut(M_SC_MSTROOM);
	msgOut.serial(_FID);

	MAP_MST_ROOM::iterator	it;
	for (it = map_mstRoom.begin(); it != map_mstRoom.end(); it++)
		msgOut.serial(it->second.m_ID, it->second.m_SceneID, it->second.m_Price, it->second.m_Years, it->second.m_Months, it->second.m_Days, it->second.m_DiscPercent, it->second.m_HisSpace);

	CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
}
// ?? ???? ????? ???
//bool GetRoomPartInfoAll(T_MSTROOM_ID _roomid, T_ROOMPARTID _partid, ROOMPARTINFOALL *_buffer)
//{
//	T_ITEMSUBTYPEID ItemID=0;
//	pair<MAP_OUT_ROOMREPLACEABLEPARTS::iterator, MAP_OUT_ROOMREPLACEABLEPARTS::iterator>	range = map_OUT_ROOMREPLACEABLEPARTS.equal_range(_roomid);
//	for ( MAP_OUT_ROOMREPLACEABLEPARTS::iterator it = range.first; it != range.second; it ++ )
//	{
//		OUT_ROOMREPLACEABLEPARTS	&info = (*it).second;
//		if (info.PartId == _partid)
//		{
//			ItemID = info.ItemSubtypeId;
//			_buffer->m_PartId = _partid;
//			_buffer->m_NeedDegree = info.NeedDegree;
//			_buffer->m_Pos = info.Pos;
//			break;
//		}
//	}
//
//	MAP_MST_ITEM::iterator	ititem = map_MST_ITEM.find(ItemID);
//	if (ititem == map_MST_ITEM.end())
//	{
//		sipwarning(L"There is no master data of item(Itemid=%d)", ItemID);
//		return false;
//	}
//
//	_buffer->m_ItemSubtypeInfo = ititem->second;
//	return true;
//}

// ----------------------------------------------------------------------------
MAP_MST_HISSPACE							map_mstHisSpace;
void	SendMstHisSpaceData(T_FAMILYID	_FID, SIPNET::TServiceId _tsid)
{
	if (map_mstHisSpace.size() < 1)
		return;
	CMessage	msgOut(M_SC_MSTHISSPACE);
	msgOut.serial(_FID);

	MAP_MST_HISSPACE::iterator	it;
	for (it = map_mstHisSpace.begin(); it != map_mstHisSpace.end(); it++)
		msgOut.serial(it->second.m_ID, it->second.m_HisSpace, it->second.m_GMoney, it->second.m_DiscGMoney, it->second.m_UMoney, it->second.m_DiscUMoney, it->second.m_AddPoint);

	CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
}

// ?????????? ???
MST_HISSPACE	*GetMstHisSpace(T_COMMON_ID _ID)
{
	MAP_MST_HISSPACE::iterator it = map_mstHisSpace.find(_ID);
	if (it == map_mstHisSpace.end())
		return NULL;
	return &(it->second);
}

void	SendMstRoomLevelExpData(T_FAMILYID _FID, SIPNET::TServiceId _tsid)
{
	CMessage	msgOut(M_SC_MSTROOMLEVELEXP);
	msgOut.serial(_FID);

	for (int i = 1; i <= ROOM_LEVEL_MAX; i ++)
	{
		msgOut.serial(mstRoomLevel[i].Level, mstRoomLevel[i].Exp);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

void	SendMstFamilyLevelExpData(T_FAMILYID _FID, SIPNET::TServiceId _tsid)
{
	CMessage	msgOut(M_SC_MSTFAMILYLEVELEXP);
	msgOut.serial(_FID);

	for (int i = 1; i <= FAMILY_LEVEL_MAX; i ++)
	{
		msgOut.serial(mstFamilyLevel[i].Level, mstFamilyLevel[i].Exp);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

//uint16 CalcRoomExpPercent(T_ROOMLEVEL level, T_ROOMEXP exp)
//{
//	if(level >= ROOM_LEVEL_MAX)
//		return 10000;
//	if(exp <= mstRoomLevel[level].Exp)
//		return 0;
//	if(exp >= mstRoomLevel[level + 1].Exp)
//		return 10000;
//	return (uint16)((exp - mstRoomLevel[level].Exp) * 10000 / (mstRoomLevel[level + 1].Exp - mstRoomLevel[level].Exp));
//}

//uint16 CalcFamilyExpPercent(T_ROOMLEVEL level, T_ROOMEXP exp, uint32 *pMaxExp)
//{
//	if(level >= FAMILY_LEVEL_MAX)
//	{
//		if(pMaxExp != NULL)
//			*pMaxExp = mstFamilyLevel[FAMILY_LEVEL_MAX].Exp;
//		return 10000;
//	}
//	if(pMaxExp != NULL)
//		*pMaxExp = mstFamilyLevel[level + 1].Exp;
//	if(exp <= mstFamilyLevel[level].Exp)
//		return 0;
//	if(exp >= mstFamilyLevel[level + 1].Exp)
//		return 10000;
//	return (uint16)((exp - mstFamilyLevel[level].Exp) * 10000 / (mstFamilyLevel[level + 1].Exp - mstFamilyLevel[level].Exp));
//}

//uint16 CalcFishExpPercent(T_ROOMLEVEL level, T_ROOMEXP exp)
//{
//	if(level >= FISH_LEVEL_MAX)
//		return 10000;
//	if(exp <= mstFishLevel[level].Exp)
//		return 0;
//	if(exp >= mstFishLevel[level + 1].Exp)
//		return 10000;
//	return (uint16)((exp - mstFishLevel[level].Exp) * 10000 / (mstFishLevel[level + 1].Exp - mstFishLevel[level].Exp));
//}

MAP_MST_FISH		map_mstFish;
//MST_FISH *GetMstFish(uint8 FishID)
//{
//	MAP_MST_FISH::iterator it = map_mstFish.find(FishID);
//	if (it == map_mstFish.end())
//		return NULL;
//	return &(it->second);
//}

// Fish master정보를 보낸다
void SendMstFishData(T_FAMILYID _FID, TServiceId _tsid)
{
	CMessage	msgOut(M_SC_MSTFISH);
	msgOut.serial(_FID);
	for (int i = 1; i <= FISH_LEVEL_MAX; i ++)
	{
		msgOut.serial(mstFishLevel[i].Level, mstFishLevel[i].Exp);
	}
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

uint32 GetFishLevelFromExp(uint32 exp)
{
	if (exp == 0)
		return 1;
	for (int i = 2; i <= FISH_LEVEL_MAX; i ++)
	{
		if (exp < mstFishLevel[i].Exp)
		{
			return (i-1);
		}
	}
	return FISH_LEVEL_MAX;
}

//////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
void	LoadDBMstData()
{
	map_mstRoom.clear();
	map_MST_ITEM.clear();
	map_mstHisSpace.clear();
	map_mstFish.clear();
	memset(mstRoomExp, 0, sizeof(mstRoomExp));
	memset(mstRoomLevel, 0, sizeof(mstRoomLevel));
	memset(mstFamilyExp, 0, sizeof(mstFamilyExp));
	memset(mstFamilyLevel, 0, sizeof(mstFamilyLevel));
	{
		MAKEQUERY_LOADMSTDATA1(strQuery);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
		if (nQueryResult == true)
		{
			// Load item master data
			do
			{
				DB_QUERY_RESULT_FETCH( Stmt, row );
				if (IS_DB_ROW_VALID( row ))
				{
					COLUMN_DIGIT( row, 0, T_ITEMSUBTYPEID,		subid );
					COLUMN_DIGIT( row, 1, T_PRICE,				gmoney );
					COLUMN_DIGIT( row, 2, T_PRICEDISC,			discgmoney );
					COLUMN_DIGIT( row, 3, T_PRICE,				umoney );
					COLUMN_DIGIT( row, 4, T_PRICEDISC,			discumoney );
					COLUMN_DIGIT( row, 5, T_F_EXP,				expinc );
					COLUMN_DIGIT( row, 6, T_F_VIRTUE,			virinc );
					COLUMN_DIGIT( row, 7, T_F_FAME,				fameinc );
					COLUMN_DIGIT( row, 8, T_ROOMEXP,			roomexpinc );
					COLUMN_DIGIT( row, 9, T_ROOMVIRTUE,			roomvirinc );
					COLUMN_DIGIT( row, 10, T_ROOMFAME,			roomfameinc );
					COLUMN_DIGIT( row, 11, T_ROOMAPPRECIATION,	roomapprinc );
					COLUMN_DIGIT( row, 12, T_USEDTIME,			utime );
					COLUMN_WSTR( row, 13, itemname, 32);
					COLUMN_DIGIT( row, 14, T_ITEMTYPEID,		tid );
					COLUMN_DIGIT( row, 15, T_FLAG,				oflag );
					COLUMN_DIGIT( row, 16, T_FITEMNUM,			omaxnum );
					COLUMN_DIGIT( row, 17, T_USEDTIME,			termtime );
					COLUMN_DIGIT( row, 18, uint32,				addpoint );
					COLUMN_DIGIT( row, 19, uint32,				item_level );

					if(!oflag)
						omaxnum = 1;
					MST_ITEM item;
					item.SubID = subid;					
					item.GMoney = gmoney;				item.UMoney = umoney;
					item.DiscGMoney = discgmoney;		item.DiscUMoney = discumoney;	item.AddPoint = addpoint;
					item.ExpInc = expinc;				item.VirtueInc = virinc;
					item.FameInc = fameinc;				item.RoomExpInc = roomexpinc;
					item.RoomVirtueInc = roomvirinc;	item.RoomFameInc = roomfameinc;
					item.RoomApprInc = roomapprinc;		item.UseTime = utime;
					item.Name = itemname;				item.TypeID = tid;
					item.OverlapFlag = oflag;			item.OverlapMaxNum = omaxnum;
					item.TermTime = termtime;			item.Level = item_level;
					map_MST_ITEM.insert( make_pair( subid, item ) );
				}
				else
					break;
			} while (true);
			if(map_MST_ITEM.empty())
			{
				siperror("[mstItemBufInfo] load failed.");
				return;
			}

			// Load room master data
//			short  sMore1 = SQLMoreResults( Stmt );
			BOOL bMore1 = Stmt.MoreResults();
			if (bMore1)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, T_MSTROOM_ID,		id );
						COLUMN_DIGIT( row, 1, T_MSTROOM_ID,		sceneid );
						COLUMN_WSTR( row, 2, roomname, MAX_LEN_MROOM_NAME);
						COLUMN_DIGIT( row, 3, T_MSTROOM_YEARS,	years );
						COLUMN_DIGIT( row, 4, T_MSTROOM_MONTHS,	months );
						COLUMN_DIGIT( row, 5, T_MSTROOM_DAYS,	days );
						COLUMN_DIGIT( row, 6, T_PRICE,			price );
						COLUMN_DIGIT( row, 7, T_PRICEDISC,		discPer );
						COLUMN_DIGIT( row, 8, uint32,			hisspace );
						COLUMN_DIGIT( row, 9, uint16,			cardtype );

						MST_ROOM room;
						room.m_ID				= id;
						room.m_SceneID			= sceneid;
						room.m_Name				= roomname;
						room.m_Years			= years;
						room.m_Months			= months;
						room.m_Days				= days;
						room.m_Price			= price;
						room.m_DiscPercent		= discPer;
						room.m_HisSpace			= hisspace;
						room.m_CardType			= cardtype;
						map_mstRoom.insert( make_pair( id, room ) );
					}
					else
						break;
				} while (true);
			}
			if(map_mstRoom.empty())
			{
				sipwarning("[mstRoomKind] load failed.");
				return;
			}

			// Load hisspace master data
//			short  sMore2 = SQLMoreResults( Stmt );
			BOOL bMore2 = Stmt.MoreResults();
			if (bMore2)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, T_COMMON_ID,		id );
						COLUMN_DIGIT( row, 1, T_HISSPACE,		hisspace );
						COLUMN_DIGIT( row, 2, T_PRICE,			gmoney );
						COLUMN_DIGIT( row, 3, T_PRICEDISC,		discgmoney );
						COLUMN_DIGIT( row, 4, T_PRICE,			umoney );
						COLUMN_DIGIT( row, 5, T_PRICEDISC,		discumoney );
						COLUMN_DIGIT( row, 6, uint32,			addpoint );

						map_mstHisSpace.insert( make_pair( id, MST_HISSPACE(id, hisspace, gmoney, discgmoney, umoney, discumoney, addpoint) ) );
					}
					else
						break;
				} while (true);
			}
			if(map_mstHisSpace.empty())
			{
				sipwarning("[mstHisSpace] load failed.");
				return;
			}

			// Load mstConfig
//			short  sMore5 = SQLMoreResults( Stmt );
			BOOL bMore5 = Stmt.MoreResults();
			if (bMore5)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_STR(row, 0, Name, MAX_LEN_CONFIGNAME);
						COLUMN_DIGIT( row, 1, uint32, ValueINT );
	
						map_mstConfig.insert( make_pair( Name, MST_CONFIG(Name, ValueINT) ) );
						sipdebug("%s=%d", Name.c_str(), ValueINT);
					}
					else
						break;
				} while (true);
			}
			if(map_mstConfig.empty())
			{
				sipwarning("[mstConfig] load failed.");
				return;
			}

			// Load mstRoomLevel
//			short  sMore6 = SQLMoreResults( Stmt );
			BOOL bMore6 = Stmt.MoreResults();
			if (bMore6)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, T_ROOMLEVEL, Level );
						COLUMN_DIGIT( row, 1, T_ROOMEXP, Exp );
						COLUMN_DIGIT( row, 2, uint8, ShowLevel );

						if(Level > ROOM_LEVEL_MAX)
						{
							siperror("[mstRoomLevel] invalid room level. level=%d, ROOM_LEVEL_MAX=%d", Level, ROOM_LEVEL_MAX);
							return;
						}

						mstRoomLevel[Level].Level = Level;
						mstRoomLevel[Level].Exp = Exp;
						mstRoomLevel[Level].ShowLevel = ShowLevel;
						//sipdebug("mstRoomLevel=%d", Level); byy krs
					}
					else
						break;
				} while (true);
			}
			else
			{
				sipwarning("[mstRoomLevel] load failed.");
				return;
			}

			// Load mstRoomExp
//			short  sMore7 = SQLMoreResults( Stmt );
			BOOL bMore7 = Stmt.MoreResults();
			if (bMore7)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, uint8, TypeID );
						COLUMN_DIGIT( row, 1, T_ROOMEXP, Exp );
						COLUMN_DIGIT( row, 2, uint8, MaxCount );

						if(TypeID >= ROOM_EXP_TYPE_MAX)
						{
							siperror("[mstRoomExp] invalid typeid.");
							return;
						}

						mstRoomExp[TypeID].TypeID = TypeID;
						mstRoomExp[TypeID].Exp = Exp;
						mstRoomExp[TypeID].MaxCount = MaxCount;
						//sipdebug("mstRoomExp[TypeID]=%d", TypeID); byy krs
					}
					else
						break;
				} while (true);
//				if(mstRoomExp[Room_Exp_Type_Visitroom].MaxCount > MAX_ROOM_VISIT_HIS_COUNT)
//				{
//					siperror("mstRoomExp[Room_Exp_Type_Visitroom].MaxCount > MAX_ROOM_VISIT_HIS_COUNT(%d)", MAX_ROOM_VISIT_HIS_COUNT);
//					return;
//				}
			}
			else
			{
				sipwarning("[mstRoomExp] load failed.");
				return;
			}

			// Load mstFamilyExp
//			short  sMore8 = SQLMoreResults( Stmt );
			BOOL bMore8 = Stmt.MoreResults();
			if (bMore8)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, uint8, TypeID );
						COLUMN_DIGIT( row, 1, T_F_EXP, Exp );
						COLUMN_DIGIT( row, 2, uint8, MaxCount );

						if(TypeID >= FAMILY_EXP_TYPE_MAX)
						{
							siperror("[mstFamilyExp] invalid typeid.");
							return;
						}

						mstFamilyExp[TypeID].TypeID = TypeID;
						mstFamilyExp[TypeID].Exp = Exp;
						mstFamilyExp[TypeID].MaxCount = MaxCount;
					}
					else
						break;
				} while (true);
//				if(mstFamilyExp[Family_Exp_Type_Visitroom].MaxCount > MAX_ROOM_VISIT_HIS_COUNT)
//				{
//					siperror("mstFamilyExp[Family_Exp_Type_Visitroom].MaxCount > MAX_ROOM_VISIT_HIS_COUNT(%d)", MAX_ROOM_VISIT_HIS_COUNT);
//					return;
//				}
			}
			else
			{
				siperror("[mstFamilyExp] load failed.");
				return;
			}

			// Load mstFamilyLevel
//			short  sMore9 = SQLMoreResults( Stmt );
			BOOL bMore9 = Stmt.MoreResults();
			if (bMore9)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, uint32, Level );
						COLUMN_DIGIT( row, 1, T_F_EXP, Exp );
						COLUMN_DIGIT( row, 2, uint32, AddPoint );

						if(Level > FAMILY_LEVEL_MAX)
						{
							siperror("[mstFamilyLevel] invalid family level.");
							return;
						}

						mstFamilyLevel[Level].Level = static_cast<T_F_LEVEL>(Level);
						mstFamilyLevel[Level].Exp = Exp;
						mstFamilyLevel[Level].AddPoint = AddPoint;
					}
					else
						break;
				} while (true);
			}
			else
			{
				siperror("[mstFamilyLevel] load failed.");
				return;
			}

			// Load mstFishLevel
//			short  sMore10 = SQLMoreResults( Stmt );
			BOOL bMore10 = Stmt.MoreResults();
			if (bMore10)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, uint8, Level );
						COLUMN_DIGIT( row, 1, uint32, Exp );

						if(Level > FISH_LEVEL_MAX)
						{
							siperror("[mstFishLevel] invalid fish level.");
							return;
						}

						mstFishLevel[Level].Level = Level;
						mstFishLevel[Level].Exp = Exp;
					}
					else
						break;
				} while (true);
			}
			else
			{
				sipwarning("[mstFishLevel] load failed.");
				return;
			}
		}
		else
		{
			siperror("Shrd_room_loadmstdata1 failed.");
			return;
		}
	}
	{
		MAKEQUERY_LOADMSTDATA2(strQuery);
		CDBConnectionRest	DB(DBCaller);
		DB_EXE_QUERY( DB, Stmt, strQuery );
		if (nQueryResult == true)
		{
			// Load mstBadFISList
			{
				int		count = 0;
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_STR(row, 0, _FIDKey, MAX_LEN_CHARACTER_ID);

						string FIDKey = trim(_FIDKey) + "-";
						if(count == 0)
							FIDKey = "-" + FIDKey;

						mstBadFIDList += FIDKey;

						count ++;
					}
					else
						break;
				} while (true);
				sipdebug("Bad FID List Count=%d", count);
			}
//			else
//			{
//				siperror("[mstBadFISList] load failed.");
//				return;
//			}

			// Load mstFamilyVirtue0
//			short  sMore12 = SQLMoreResults( Stmt );
			BOOL bMore12 = Stmt.MoreResults();
			if (bMore12)
			{
				do
				{
					DB_QUERY_RESULT_FETCH( Stmt, row );
					if (IS_DB_ROW_VALID( row ))
					{
						COLUMN_DIGIT( row, 0, uint8, TypeID );
						COLUMN_DIGIT( row, 1, uint8, ReligionRoomType );
						COLUMN_DIGIT( row, 2, uint32, Virtue );
						COLUMN_DIGIT( row, 3, uint8, MaxCount );

						if(ReligionRoomType == 0)
						{
							if(TypeID >= FAMILY_VIRTUE0_TYPE_MAX)
							{
								siperror("[mstFamilyVirtue0] invalid typeid.");
								return;
							}

							mstFamilyVirtue0[TypeID].TypeID = TypeID;
							mstFamilyVirtue0[TypeID].Virtue = Virtue;
							mstFamilyVirtue0[TypeID].MaxCount = MaxCount;
						}
						else
						{
							siperror("Invalid ReligionRoomType. ReligionRoomType=%d", ReligionRoomType);
						}
					}
					else
						break;
				} while (true);
			}
			else
			{
				sipwarning("[mstFamilyVirtue0] load failed.");
				return;
			}

			// Load tbl_mstFish
//			short  sMore13 = SQLMoreResults(Stmt);
			BOOL bMore13 = Stmt.MoreResults();
			if (bMore13)
			{
				do
				{
					DB_QUERY_RESULT_FETCH(Stmt, row);
					if (IS_DB_ROW_VALID(row))
					{
						COLUMN_DIGIT(row, 0, uint8, FishID);
						COLUMN_DIGIT(row, 1, uint8, FishLevel);
						COLUMN_DIGIT(row, 2, uint32, FishLevelExp1);
						COLUMN_DIGIT(row, 3, uint32, FishLevelExp2);
						COLUMN_DIGIT(row, 4, uint32, FishLevelExp3);
						COLUMN_DIGIT(row, 5, uint32, FishLevelExp4);
						COLUMN_DIGIT(row, 6, uint32, FishLevelExp5);
						COLUMN_DIGIT(row, 7, uint32, FishLevelExp6);
						COLUMN_DIGIT(row, 8, uint32, FishLevelExp7);

						uint32	FishLevelExp[FISH_LEVEL_MAX + 1];
						FishLevelExp[0] = 0;
						FishLevelExp[1] = FishLevelExp1;
						FishLevelExp[2] = FishLevelExp2;
						FishLevelExp[3] = FishLevelExp3;
						FishLevelExp[4] = FishLevelExp4;
						FishLevelExp[5] = FishLevelExp5;
						FishLevelExp[6] = FishLevelExp6;
						FishLevelExp[7] = FishLevelExp7;
						map_mstFish.insert(make_pair(FishID, MST_FISH(FishID, FishLevel, FishLevelExp)));
					}
					else
						break;
				} while (true);
			}
			else
			{
				sipwarning("[tbl_mstFish] load failed.");
				return;
			}
		}
		else
		{
			siperror("Shrd_room_loadmstdata2 failed.");
			return;
		}
	}

	// map_mstRoom 자료를 검사한다.
	{
		#define MAX_SCENE_COUNT	20
		#define MAX_PRICE_COUNT_PER_SCENE	10
		int priceid[MAX_SCENE_COUNT][MAX_PRICE_COUNT_PER_SCENE + 1], sceneid[MAX_SCENE_COUNT];
		for (int i = 0; i < MAX_SCENE_COUNT; i ++)
			sceneid[i] = 0;

		bool bExistFree = false;
		for (MAP_MST_ROOM::iterator it = map_mstRoom.begin(); it != map_mstRoom.end(); it ++)
		{
			if (it->second.m_Price == 0)
			{
				if (bExistFree)
				{
					// 체험방은 하나뿐이여야 한다.
					siperror("too many FreeRoom.");
					return;
				}
				bExistFree = true;
			}
			else
			{
				if ((it->second.m_Months != 0) || (it->second.m_Days != 0))
				{
					// 체험방이 아닌 방은 Months=0, Days=0 이여야 한다.
					siperror("Invalid Months, Days. PriceID=%d", it->second.m_ID);
					return;
				}

				int n = 0;
				for (n = 0; n < MAX_SCENE_COUNT; n ++)
				{
					if (sceneid[n] == 0)
					{
						sceneid[n] = it->second.m_SceneID;
						priceid[n][0] = 0;
						break;
					}
					if (sceneid[n] == it->second.m_SceneID)
						break;
				}
				if (n >= MAX_SCENE_COUNT)
				{
					siperror("MAX_SCENE_COUNT too small(code problem) !!!");
					return;
				}
				if (priceid[n][0] >= MAX_PRICE_COUNT_PER_SCENE)
				{
					siperror("MAX_PRICE_COUNT_PER_SCENE too small(code problem) !!!");
					return;
				}
				priceid[n][priceid[n][0] + 1] = it->second.m_ID;
				priceid[n][0] ++;
			}
		}
		// priceid 정렬
		for (int i = 0; i < MAX_SCENE_COUNT; i ++)
		{
			if (sceneid[i] == 0)
				break;
			for (int j = 1; j <= priceid[i][0]; j ++)
			{
				int year = map_mstRoom[priceid[i][j]].m_Years;
				int jj = j;
				for (int k = j + 1; k <= priceid[i][0]; k ++)
				{
					int year1 = map_mstRoom[priceid[i][k]].m_Years;
					if (year1 == 0) // 무한방 = 100년방
						year1 = 100;
					if (year1 < year)
					{
						year = year1;
						jj = k;
					}
				}
				if (jj != j)
				{
					int temp = priceid[i][jj];
					priceid[i][jj] = priceid[i][j];
					priceid[i][j] = temp;
				}
			}
		}
/*
		for (int i = 0; i < MAX_SCENE_COUNT; i ++)
		{
			if (sceneid[i] == 0)
				break;
			if (priceid[i][0] != 3)
			{
				// 1년, 3년, 무한방 3개이여야 한다.
				siperror("priceid[i][0] != 3. SceneID=%d", sceneid[i]);
				return;
			}
			if (map_mstRoom[priceid[i][1]].m_Years != 1)
			{
				// 1-1년
				siperror("map_mstRoom[priceid[i][1]].m_Years != 1. SceneID=%d", sceneid[i]);
				return;
			}
			if (map_mstRoom[priceid[i][2]].m_Years != 3)
			{
				// 2-3년
				siperror("map_mstRoom[priceid[i][2]].m_Years != 3. SceneID=%d", sceneid[i]);
				return;
			}
			if (map_mstRoom[priceid[i][3]].m_Years != 0)
			{
				// 3-무한
				siperror("map_mstRoom[priceid[i][3]].m_Years != 0. SceneID=%d", sceneid[i]);
				return;
			}
			for (int j = 1; j < priceid[i][0]; j ++)
			{
				if (map_mstRoom[priceid[i][j]].m_Years == map_mstRoom[priceid[i][j + 1]].m_Years)
				{
					// 같은 SceneID, 기일이 없어야 한다.
					siperror("Years Error. SceneID=%d", sceneid[i]);
					return;
				}
				if (map_mstRoom[priceid[i][j]].m_HisSpace != map_mstRoom[priceid[i][j + 1]].m_HisSpace)
				{
					// 같은 SceneID이면 SpaceCapacity가 같아야 한다.
					siperror("HisSpace Error. SceneID=%d", sceneid[i]);
					return;
				}
				if (map_mstRoom[priceid[i][j]].m_Price >= map_mstRoom[priceid[i][j + 1]].m_Price)
				{
					// 가격이 기일에 따라서 증가해야 한다.
					siperror("Price Error. SceneID=%d", sceneid[i]);
					return;
				}
			}
		}
		*/
		/*
		// 1년짜리 방의 가격이 같으면 다른 모든 정보도 같아야 한다.
		for (int i = 0; i < MAX_SCENE_COUNT; i ++)
		{
			if (sceneid[i] == 0)
				break;
			for (int j = i + 1; j < MAX_SCENE_COUNT; j ++)
			{
				if (sceneid[j] == 0)
					break;
				if (map_mstRoom[priceid[i][1]].m_Price != map_mstRoom[priceid[j][1]].m_Price)
					continue;
				if (map_mstRoom[priceid[i][1]].m_HisSpace != map_mstRoom[priceid[j][1]].m_HisSpace)
				{
					siperror("map_mstRoom[priceid[i][1]].m_HisSpace != map_mstRoom[priceid[j][1]].m_HisSpace. SceneID1=%d, SceneID2=%d", sceneid[i], sceneid[j]);
					return;
				}
				if (priceid[i][0] != priceid[j][0])
				{
					siperror("priceid[i][0] != priceid[j][0]. SceneID1=%d, SceneID2=%d", sceneid[i], sceneid[j]);
					return;
				}
				for (int k = 1; k <= priceid[i][0]; k ++)
				{
					if (map_mstRoom[priceid[i][k]].m_Price != map_mstRoom[priceid[j][k]].m_Price)
					{
						siperror("map_mstRoom[priceid[i][k]].m_Price != map_mstRoom[priceid[j][k]].m_Price. SceneID1=%d, SceneID2=%d", sceneid[i], sceneid[j]);
						return;
					}
				}
			}
		}
		*/
	}

	// Map에서 계속 참조하는것이 속도상 불리하기때문에 변수를 꺼내놓음.
	g_nFishExpReducePercent = (int)GetMstConfigData("FishExpReducePercent")->ValueINT;
	g_nCharChangePrice = GetMstConfigData("CharChangePrice")->ValueINT;
	g_nRoomSmoothFamPercent = GetMstConfigData("RoomSmoothFamPercent")->ValueINT;
	g_nRoomDeleteDay = GetMstConfigData("RoomDeleteDay")->ValueINT;
	g_nSamePromotionPrice = GetMstConfigData("SamePromotionPrice")->ValueINT;
	g_nSamePromotionCount = GetMstConfigData("SamePromotionCount")->ValueINT;
	//g_nRoomcardBackRate = GetMstConfigData("RoomcardBackRate")->ValueINT;
}

// 
MST_CONFIG* GetMstConfigData(std::string Name)
{
	MAP_MST_CONFIG::iterator it = map_mstConfig.find(Name);
	if (it == map_mstConfig.end())
	{
		siperror("Can't find ConfigData: %s", Name.c_str());
		return NULL;
	}
	return &(it->second);
}

void SendMstConfigData(T_FAMILYID _FID, SIPNET::TServiceId _tsid)
{
	CMessage	msgOut(M_SC_MSTCONFIG);
	msgOut.serial(_FID);

	msgOut.serial(g_nCharChangePrice, g_nSamePromotionPrice, g_nSamePromotionCount);

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}
