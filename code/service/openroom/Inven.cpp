#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>

#include "Inven.h"

#include "misc/db_interface.h"
#include "misc/DBCaller.h"
#include "../Common/QuerySetForShard.h"

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"
#include "../Common/DBLog.h"
#include "mst_room.h"
#include "openroom.h"
#include "Family.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

//extern DBCONNECTION	DB;
extern CDBCaller	*DBCaller;
//extern T_F_LEVEL MakeDegreeFromVDays(T_F_VISITDAYS _vdays);

// 인벤위치로부터 아이템type값을 얻는다
T_ITEMTYPEID	GetItemTypeForPos(T_FITEMPOS _pos)
{
	return (T_ITEMTYPEID)(_pos / INVEN_KIND_MAX);
}

bool	IsValidInvenPos(T_FITEMPOS _pos)
{
	uint32	uPos = _pos % INVEN_KIND_MAX;
	if (uPos < MAX_INVEN_NUM)
		return true;
	return false;
}

// 두인벤위치가 같은 종류의 아이템을 가리키는가 검사
bool	IsEqualItemForPos(T_FITEMPOS _pos1, T_FITEMPOS _pos2)
{
	T_ITEMTYPEID item1 = GetItemTypeForPos(_pos1);
	T_ITEMTYPEID item2 = GetItemTypeForPos(_pos2);

	if (item1 == item2)
		return true;

	return false;
}

// 아이템종류에 해당한 인벤토리Max값얻기
T_FITEMPOS	GetInvenMaxNumFromType(T_ITEMTYPEID _itypeid)
{
	return MAX_INVEN_NUM;
}

// Family의 인벤을 얻는다.
_InvenItems* GetFamilyInven(T_FAMILYID FID) // , bool bLoginUserOnly, bool *pIsLogin)
{
	CWorld	*pWorld = GetWorldFromFID(FID);
	if (pWorld)
	{
		return pWorld->GetFamilyInven(FID);
	}
	//sipwarning("Can't find World. FID=%d", FID); byy krs
	return NULL;
}
//_InvenItems* GetFamilyInven(T_FAMILYID FID, bool bLoginUserOnly, bool *pIsLogin)
//{
//	CWorld	*pWorld = GetWorldFromFID(FID);
//	if (pWorld)
//	{
//		if(pIsLogin)
//			(*pIsLogin) = true;
//
//		return pWorld->GetFamilyInven(FID);
//	}
//	if (pIsLogin)
//		(*pIsLogin) = false;
//	if (bLoginUserOnly)
//	{
//		sipwarning("Can't find World. FID=%d", FID);
//		return NULL;
//	}
//
//	// DB에서 읽는다.
//	static	_InvenItems	Inven;
//
//	if (!ReadFamilyInven(FID, &Inven))
//	{
//		sipwarning(L"ReadFamilyInven failed. FID=%d", FID);
//		return NULL;
//	}
//
//	return &Inven;
//}

int GetInvenIndexFromInvenPos(T_FITEMPOS InvenPos)
{
	return (int)(InvenPos % INVEN_KIND_MAX);
}

uint16 GetInvenPosFromInvenIndex(int InvenIndex)
{
	return (uint16)(InvenIndex + INVEN_KIND_MAX);
}

void	ItemRefresh(T_FAMILYID FID, SIPNET::TServiceId _tsid) // , T_ACCOUNTID uId, ucstring UserName, T_FAMILYNAME	ucFName)
{
	_InvenItems*	pInven = GetFamilyInven(FID);
	if (pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL. FID=%d", FID);
		return;
	}

	uint32	SyncCode = 0;
	CMessage	msgOut(M_SC_FAMILYITEM);
	msgOut.serial(FID, SyncCode);
//	CMessage	msgTimeoutItem(M_SC_TIMEOUT_ITEM);
//	msgTimeoutItem.serial(FID);

	int			TimeoutItemCount = 0;
	uint16		InvenPos;
	uint8		MoneyType_Timeout = 0;

	TID			CharacterDressID = 0;
	TTime		CurTime = GetDBTimeSecond();
	for (uint16 ItemIndex = 0; ItemIndex < MAX_INVEN_NUM; ItemIndex ++)
	{
		if (pInven->Items[ItemIndex].ItemID == 0)
			continue;

		uint32	ItemID = pInven->Items[ItemIndex].ItemID;

		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
		if (mstItem == NULL)
		{
			pInven->Items[ItemIndex].ItemID = 0;
			sipwarning(L"GetMstItemInfo failed. FID=%d, InvenIndex=%d, ItemID=%d", FID, ItemIndex, ItemID);
			continue;
		}

		// GetDate가 0인 아이템은 아직 착용하지 않은 새 아이템이므로 삭제하지 않는다.
		if ((pInven->Items[ItemIndex].GetDate > 0) && (mstItem->UseTime > 0) && ((uint64)CurTime > pInven->Items[ItemIndex].GetDate + mstItem->UseTime * 60))
		{
			InvenPos = GetInvenPosFromInvenIndex((int)ItemIndex);
			msgOut.serial(InvenPos, ItemID, pInven->Items[ItemIndex].ItemCount, pInven->Items[ItemIndex].GetDate, pInven->Items[ItemIndex].UsedCharID, MoneyType_Timeout);

//			msgTimeoutItem.serial(ItemID, pInven->Items[ItemIndex].ItemCount, ItemIndex);
			DBLOG_RefreshItem(FID, 0, ItemID, 1, mstItem->Name);

			pInven->Items[ItemIndex].ItemID = 0;

			TimeoutItemCount ++;
		}
		else
		{
			if (pInven->Items[ItemIndex].UsedCharID != 0)
				CharacterDressID = ItemID;

			InvenPos = GetInvenPosFromInvenIndex((int)ItemIndex);
			msgOut.serial(InvenPos, ItemID, pInven->Items[ItemIndex].ItemCount, pInven->Items[ItemIndex].GetDate, pInven->Items[ItemIndex].UsedCharID, pInven->Items[ItemIndex].MoneyType);
		}
	}

	// 입고있던 옷아이템이 변한 경우 처리
	CharacterDressChanged(FID, CharacterDressID);

//	if (bIs)
	{
		//sipinfo(L"Send family item info to account = %d", FID); byy krs
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
	if (TimeoutItemCount > 0)
	{
		if (!SaveFamilyInven(FID, pInven))
		{
			sipwarning("SaveFamilyInven failed. FID=%d", FID);
			return;
		}

//		CUnifiedNetwork::getInstance()->send(_tsid, msgTimeoutItem);
//		sipinfo(L"Send family timeout item info to account = %d", FID);
	}
}

void CharacterDressChanged(T_FAMILYID FID, T_ID NewDressID, T_CHARACTERID CHID)
{
	if (CHID == 0)
	{
		INFO_FAMILYINROOM	*pInfo = GetFamilyInfo(FID);
		if (pInfo == NULL)
		{
			sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
			return;
		}

		std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator	it = pInfo->m_mapCH.begin();
		if (it == pInfo->m_mapCH.end())
		{
			sipwarning("No Character. FID=%d", FID);
			return;
		}

		CHID = it->first;
		CHARACTER	*character = GetCharacterData(CHID);
		if (character == NULL)
		{
			sipwarning("GetCharacterData = NULL. CHID=%d", CHID);
			return;
		}

		if (NewDressID == character->m_DressID)
			return;
	}

	// DB에 캐릭터 아이템착용 설정
	{
		MAKEQUERY_UPDATECHDRESS(strQ, FID, CHID, NewDressID);
		// DB_EXE_QUERY(DB, Stmt, strQ);
		DBCaller->execute(strQ, NULL, "");
	}

	// 방에 들어온 상태이면 통보
	CChannelWorld *inManager = GetChannelWorldFromFID(FID);
	if (inManager != NULL)
		inManager->SetCharacterDress(FID, CHID, NewDressID);

	//FAMILY *pData = GetFamilyData(FID);
	//if(pData == NULL)
	//{
	//	sipwarning("GetFamilyData = NULL. FID=%d", FID);
	//	return;
	//}

	//CMessage	msgDress(M_SC_CHDRESS);
	//msgDress.serial(FID, CHID, NewDressID);
	//CUnifiedNetwork::getInstance()->send(pData->m_FES, msgDress);
}

void CheckRefreshMoney(T_FAMILYID FID)
{
	INFO_FAMILYINROOM*	pFamily = GetFamilyInfo(FID);
	if ( pFamily == NULL )
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return;
	}

	uint32	curTime = SIPBASE::CTime::getSecondsSince1970();

	if ( ( curTime - pFamily->m_RefreshMoneyTime ) > REQUEST_MONEY_INTERVAL )
	{
		RefreshFamilyMoney(FID);
		pFamily->m_RefreshMoneyTime = curTime;
	}
}

// Money갱신을 요청한다.
void	cb_M_CS_FPROPERTY_MONEY(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	uint8			bForce;			_msgin.serial(bForce);

	if (bForce)
		RefreshFamilyMoney(FID);
	else
		CheckRefreshMoney(FID);
}

bool CheckInvenStatue(uint32 FID, CMessage &_msgin)
{
	uint8	DataCount;
	uint8	DelCount;
	uint16	InvenPos;
	uint32	ItemID;			// 4 bytes
	uint16	ItemCount;		// 2 bytes
	uint64	GetDate;		// 8 bytes
	uint32	UsedCharID;		// 4 bytes
	uint8	MoneyType;		// 1 bytes, MONEYTYPE_UMONEY = 1, MONEYTYPE_POINT = 2
	int		InvenIndex;

	_InvenItems	*pInven = GetFamilyInven(FID);
	if(pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL. FID=%d", FID);
		return false;
	}

	_msgin.serial(DataCount, DelCount);
	if ((DataCount > MAX_INVEN_NUM) || (DelCount != 0))
	{
		sipwarning("Data Count Error. Count=%d, DelCount=%d", DataCount, DelCount);
		return false;
	}

	for(int i = 0; i < DataCount; i ++)
	{
		_msgin.serial(InvenPos, ItemCount);
		if ( !IsValidInvenPos(InvenPos) )
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid inven position. InvenPos=%d", InvenPos);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return false;
		}

		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
		if (ItemCount == 0)
		{
			if(pInven->Items[InvenIndex].ItemID != 0)
			{
				sipwarning("IVS_CONFIRM failed.(ItemID invalid) FID=%d, i=%d, InvenPos=%d, ItemID=%d, InputCount=%d", FID, i, InvenPos, pInven->Items[InvenIndex].ItemID, ItemCount);
				return false;
			}
		}
		else
		{
			if(pInven->Items[InvenIndex].ItemCount != ItemCount)
			{
				sipwarning("IVS_CONFIRM failed.(ItemCount invalid) FID=%d, i=%d, InvenPos=%d, RealCount=%d, InputCount=%d", FID, i, InvenPos, pInven->Items[InvenIndex].ItemCount, ItemCount);
				return false;
			}

			_msgin.serial(ItemID, GetDate, UsedCharID, MoneyType);
			if((pInven->Items[InvenIndex].ItemID != ItemID) || (pInven->Items[InvenIndex].GetDate != GetDate) || (pInven->Items[InvenIndex].UsedCharID != UsedCharID) || (pInven->Items[InvenIndex].MoneyType != MoneyType))
			{
				sipwarning("IVS_CONFIRM failed.(ItemData Invalid) i=%d, InvenPos=%d", i, InvenPos);
				return false;
			}
		}
	}
	return true;
}

bool ApplyInvenChange(uint32 FID, uint16 *DressInvenPos, uint64 *DressGetDate, CMessage &_msgin)
{
	uint8	DataCount;
	uint8	DelCount;
	uint16	InvenPos;
	uint32	ItemID;			// 4 bytes
	uint16	ItemCount;		// 2 bytes
	uint64	GetDate;		// 8 bytes
	uint32	UsedCharID;		// 4 bytes
	uint8	MoneyType;		// 1 bytes, MONEYTYPE_UMONEY = 1, MONEYTYPE_POINT = 2
	uint8	DelType;		
	int		InvenIndex[MAX_INVEN_NUM];

	_InvenItems	*pInven = GetFamilyInven(FID);
	if (pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL. FID=%d", FID);
		return false;
	}

	_msgin.serial(DataCount, DelCount);
	if (DataCount > MAX_INVEN_NUM || DelCount > MAX_INVEN_NUM)
	{
		sipwarning("DataCount Error. FID=%d, DataCount=%d, DelCount=%d", FID, DataCount, DelCount);
		return false;
	}

	_InvenItems	tempInven;
	tempInven.CopyFrom(pInven);

	// 자료읽기
	int i, j;
	for (i = 0; i < DataCount; i ++)
	{
		_msgin.serial(InvenPos, ItemCount);
		if (!IsValidInvenPos(InvenPos))
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid inven position. FID=%d, InvenPos=%d", FID, InvenPos);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return false;
		}

		int	invenIndex = GetInvenIndexFromInvenPos(InvenPos);
		if (ItemCount > 0)
			_msgin.serial(ItemID, GetDate, UsedCharID, MoneyType);
		else
		{
			ItemID = 0;
			GetDate = 0;
			UsedCharID = 0;
			MoneyType = 0;
		}

		InvenIndex[i] = invenIndex;
		tempInven.Items[invenIndex].ItemID = ItemID;
		tempInven.Items[invenIndex].ItemCount = ItemCount;
		tempInven.Items[invenIndex].GetDate = GetDate;
		tempInven.Items[invenIndex].UsedCharID = UsedCharID;
		tempInven.Items[invenIndex].MoneyType = MoneyType;
	}

	_InvenItems	delInven;
	for (i = 0; i < DelCount; i ++)
	{
		_msgin.serial(ItemCount, ItemID, GetDate, MoneyType, DelType);

		//sipinfo("Check DelItem: FID=%d, DelCount=%d, ItemCount=%d, ItemID=%d, MoneyType=%d, DelType=%d", FID, DelCount, ItemCount, ItemID, MoneyType, DelType); byy krs

		// DelType 검사
		if (ItemCount == 0 || ((DelType != DELTYPE_TIMEOUT) && (DelType != DELTYPE_THROW)))
		{
			sipwarning("DelItem Error. FID=%d, DelCount=%d, i=%d, ItemID=%d, ItemCount=%d, DelType=%d", FID, DelCount, i, ItemID, ItemCount, DelType);
			return false;
		}

		delInven.Items[i].ItemID = ItemID;
		delInven.Items[i].ItemCount = ItemCount;
		delInven.Items[i].GetDate = GetDate;
		delInven.Items[i].UsedCharID = DelType;
		delInven.Items[i].MoneyType = MoneyType;
	}

	// 착용아이템은 family당 캐릭터가 1개뿐이라는 전제하에서 작성됨.
	int nPrevDressPos = -1, nNewDressPos = -1;

	// 자료검사
	_InvenItems	tempInven_check;
	tempInven_check.CopyFrom(pInven);
	for (i = 0; i < DataCount; i ++)
	{
		if (pInven->Items[InvenIndex[i]].UsedCharID != 0)
			nPrevDressPos = InvenIndex[i];

		ITEM_INFO	&tempInven_Item = tempInven.Items[InvenIndex[i]];
		int nRemainCount = tempInven_Item.ItemCount;

		if(nRemainCount == 0)
			continue;

		// 개수 검사
		for (j = 0; j < DataCount; j ++)
		{
			ITEM_INFO	&tempInven_check_Item = tempInven_check.Items[InvenIndex[j]];

			if ((tempInven_check_Item.ItemID == 0) || (tempInven_check_Item.ItemCount == 0))
				continue;

			if ((tempInven_Item.ItemID != tempInven_check_Item.ItemID)
				|| (tempInven_Item.GetDate != tempInven_check_Item.GetDate)
				|| (tempInven_Item.MoneyType != tempInven_check_Item.MoneyType))
				continue;

			if (nRemainCount <= tempInven_check_Item.ItemCount)
			{
				tempInven_check_Item.ItemCount -= nRemainCount;
				if (tempInven_check_Item.ItemCount == 0)
					tempInven_check_Item.ItemID = 0;
				break;
			}
			nRemainCount -= tempInven_check_Item.ItemCount;
			tempInven_check_Item.ItemCount = 0;
			tempInven_check_Item.ItemID = 0;
		}
		if (j >= DataCount)
		{
			// Log Error
			sipwarning("Data Error. FID=%d, DataCount=%d, DelCount=%d, i=%d, InvenIndex=%d, nRemainCount=%d", FID, DataCount, DelCount, i, InvenIndex[i], nRemainCount);
			for (i = 0; i < DataCount; i ++)
			{
				ITEM_INFO	&tempInven_Item = tempInven.Items[InvenIndex[i]];
				ITEM_INFO	&tempInven_check_Item = tempInven_check.Items[InvenIndex[i]];
				sipwarning("InvenIndex=%d, Client %d,%d,%d,%d Server %d,%d,%d,%d", InvenIndex[i], 
					tempInven_Item.ItemID, tempInven_Item.ItemCount, tempInven_Item.MoneyType, (int)tempInven_Item.GetDate,
					tempInven_check_Item.ItemID, tempInven_check_Item.ItemCount, tempInven_check_Item.MoneyType, (int)tempInven_check_Item.GetDate);
			}
			for (i = 0; i < DelCount; i ++)
			{
				ITEM_INFO	&tempInven_Item = delInven.Items[i];
				sipwarning("DelItem i=%d, Client %d,%d,%d,%d", i, 
					tempInven_Item.ItemID, tempInven_Item.ItemCount, tempInven_Item.MoneyType, (int)tempInven_Item.GetDate);
			}
			return false;
		}

		// 착용아이템 검사
		if (tempInven_Item.UsedCharID != 0)
		{
			if (tempInven_Item.ItemCount != 1)
			{
				sipwarning("Dress Item can't duplicated. FID=%d, i=%d", FID, i);
				return false;
			}

			for (j = 0; j < MAX_INVEN_NUM; j ++)
			{
				if ((j != InvenIndex[i]) && (tempInven.Items[j].ItemID != 0) && (tempInven_Item.UsedCharID == tempInven.Items[j].UsedCharID))
				{
					sipwarning("One character Dress 2 Items. FID=%d, i=%d, InvenI=%d, j=%d, ItemI=%d, ItemJ=%d", FID, i, InvenIndex[i], j, tempInven_Item.ItemID, tempInven.Items[j].ItemID);
					return false;
				}
			}

			nNewDressPos = InvenIndex[i];
		}
	}

	// 삭제아이템 검사
	for (i = 0; i < DelCount; i ++)
	{
		ITEM_INFO	&delInven_Item = delInven.Items[i];
		int nRemainCount = delInven_Item.ItemCount;

		// 개수 검사
		for (j = 0; j < DataCount; j ++)
		{
			ITEM_INFO	&tempInven_check_Item = tempInven_check.Items[InvenIndex[j]];

			if ((tempInven_check_Item.ItemID == 0) || (tempInven_check_Item.ItemCount == 0))
				continue;

			if ((delInven_Item.ItemID != tempInven_check_Item.ItemID)
				|| (delInven_Item.GetDate != tempInven_check_Item.GetDate)
				|| (delInven_Item.MoneyType != tempInven_check_Item.MoneyType))
				continue;

			if (nRemainCount <= tempInven_check_Item.ItemCount)
			{
				tempInven_check_Item.ItemCount -= nRemainCount;
				if(tempInven_check_Item.ItemCount == 0)
					tempInven_check_Item.ItemID = 0;
				break;
			}
			nRemainCount -= tempInven_check_Item.ItemCount;
			tempInven_check_Item.ItemCount = 0;
			tempInven_check_Item.ItemID = 0;
		}
		if (j >= DataCount)
		{
			// Log Error
			sipwarning("DelItem Data Error. FID=%d, DataCount=%d, DelCount=%d, i=%d, ItemID=%d, ItemCount=%d, GetDateLo=%d, MoneyType=%d", FID, DataCount, DelCount, i, delInven_Item.ItemID, delInven_Item.ItemCount, (int)delInven_Item.GetDate, delInven_Item.MoneyType);
			for (i = 0; i < DelCount; i ++)
			{
				ITEM_INFO	&tempInven_Item = delInven.Items[i];
				sipwarning("DelItem i=%d, Client %d,%d,%d,%d", i, 
					tempInven_Item.ItemID, tempInven_Item.ItemCount, tempInven_Item.MoneyType, (int)tempInven_Item.GetDate);
			}
			for (i = 0; i < DataCount; i ++)
			{
				ITEM_INFO	&tempInven_Item = tempInven.Items[InvenIndex[i]];
				ITEM_INFO	&tempInven_check_Item = tempInven_check.Items[InvenIndex[i]];
				sipwarning("InvenIndex=%d, Client %d,%d,%d,%d Server %d,%d,%d,%d", InvenIndex[i], 
					tempInven_Item.ItemID, tempInven_Item.ItemCount, tempInven_Item.MoneyType, (int)tempInven_Item.GetDate,
					tempInven_check_Item.ItemID, tempInven_check_Item.ItemCount, tempInven_check_Item.MoneyType, (int)tempInven_check_Item.GetDate);
			}
			return false;
		}
	}

	// 인벤아이템 + 삭제아이템 = 원래인벤아이템 검사
	for (j = 0; j < DataCount; j ++)
	{
		ITEM_INFO	&tempInvenCheck_Item = tempInven_check.Items[InvenIndex[j]];
		if ((tempInvenCheck_Item.ItemID != 0) && (tempInvenCheck_Item.ItemCount != 0))
		{
			sipwarning("Item Check Error. FID=%d, DataCount=%d, DelCount=%d, i=%d, InvenPos=%d, ItemID=%d, ItemCount=%d, GetDateLo=%d, MoneyType=%d", 
				FID, DataCount, DelCount, j, InvenIndex[j], tempInvenCheck_Item.ItemID, tempInvenCheck_Item.ItemCount, (int)tempInvenCheck_Item.GetDate, tempInvenCheck_Item.MoneyType);
			for (i = 0; i < DataCount; i ++)
			{
				ITEM_INFO	&tempInven_Item = tempInven.Items[InvenIndex[i]];
				ITEM_INFO	&tempInven_check_Item = tempInven_check.Items[InvenIndex[i]];
				sipwarning("InvenIndex=%d, Client %d,%d,%d,%d Server %d,%d,%d,%d", InvenIndex[i], 
					tempInven_Item.ItemID, tempInven_Item.ItemCount, tempInven_Item.MoneyType, (int)tempInven_Item.GetDate,
					tempInven_check_Item.ItemID, tempInven_check_Item.ItemCount, tempInven_check_Item.MoneyType, (int)tempInven_check_Item.GetDate);
			}
			for (i = 0; i < DelCount; i ++)
			{
				ITEM_INFO	&tempInven_Item = delInven.Items[i];
				sipwarning("DelItem i=%d, Client %d,%d,%d,%d", i, 
					tempInven_Item.ItemID, tempInven_Item.ItemCount, tempInven_Item.MoneyType, (int)tempInven_Item.GetDate);
			}
			return false;
		}
	}

	// 검사 OK
	// 착용아이템처리부터 한다.
	uint32	CHID = 0, DressID;
	bool	bSameDress = false;
	if (nNewDressPos < 0)
	{
		if (nPrevDressPos >= 0)
		{
			// 착용아이템 벗기처리
			DressID = 0;
			CharacterDressChanged(FID, DressID, pInven->Items[nPrevDressPos].UsedCharID);
		}
	}
	else // if(nNewDressPos >= 0
	{
		// 착용아이템 입기처리
		CHID = tempInven.Items[nNewDressPos].UsedCharID;
		DressID = tempInven.Items[nNewDressPos].ItemID;
		if ((nPrevDressPos >= 0) && (tempInven.Items[nNewDressPos].ItemID == pInven->Items[nPrevDressPos].ItemID))
		{
			bSameDress = true;
		}
	}

	// 착용가능아이템인가 검사
	if (CHID != 0)
	{
		INFO_FAMILYINROOM *pInfo = GetFamilyInfo(FID);
		if (pInfo == NULL)
		{
			sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
			return false;
		}

		std::map<T_CHARACTERID, INFO_CHARACTERINROOM>::iterator	itCh = pInfo->m_mapCH.find(CHID);
		if (itCh == pInfo->m_mapCH.end())
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid CharID!!!! FID=%d, CHID=%d", FID, CHID);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return false;
		}

		if (DressID != 0)
		{
			if (!bSameDress)
			{
				// ItemID가 해당 종류의 캐릭터가 착용할수 있는가를 검사한다.
				// 착용가능아이템들의 아이템ID는 ModelID * 100 + index 형식으로 되여있다.
				// (2012/05/18 수정)
				// 중년캐릭터가 추가되면서 하나의 복장을 청년,중년이 같이 입을수 있게 되였다.
				// 그리하여 복장아이템ID규칙이 달라지게 되여 착용가능성을 검사할수 없게 되였으므로 아래의 검사부분을 삭제한다.
//				if ((uint32)(DressID / 100) != itCh->second.m_ModelID)
//				{
//					ucstring ucUnlawContent = SIPBASE::_toString(L"This is invalid item!!!! ItemID=%d, ModelID=%d", DressID, itCh->second.m_ModelID);
//					RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//					return false;
//				}
			}

			// 착용아이템이 새 아이템이라면 쓰기 시작했다는 표시를 한다.
			if (tempInven.Items[nNewDressPos].GetDate == 0)
			{
				(*DressGetDate) = GetDBTimeSecond();
				tempInven.Items[nNewDressPos].GetDate = (*DressGetDate);
				(*DressInvenPos) = GetInvenPosFromInvenIndex(nNewDressPos);

				const MST_ITEM *mstItem = GetMstItemInfo(DressID);
				DBLOG_DressFirstUse(FID, 0, DressID, mstItem->Name);
			}
		}

		if (!bSameDress)
			CharacterDressChanged(FID, DressID, CHID);
	}

	// 인벤 갱신
	pInven->CopyFrom(&tempInven);

	// 삭제아이템 로그 출력
	for (i = 0; i < DelCount; i ++)
	{
		ITEM_INFO	&delInven_Item = delInven.Items[i];

		const MST_ITEM *mstItem = GetMstItemInfo(delInven_Item.ItemID);

		if (delInven_Item.UsedCharID == DELTYPE_TIMEOUT)
			DBLOG_RefreshItem(FID, 0, delInven_Item.ItemID, delInven_Item.ItemCount, mstItem->Name);
		else // if(delInven_Item.UsedCharID == DELTYPE_THROW)
			DBLOG_ThrowItem(FID, 0, delInven_Item.ItemID, delInven_Item.ItemCount, mstItem->Name);
	}

	return true;
}

/*int TrimInven(uint32 FID)
{
	_InvenItems	*pInven = GetFamilyInven(FID);
	if(pInven == NULL)
	{
		sipwarning("GetFamilyInven = NULL. FID=%d", FID);
		return 0;
	}

	// 아이템 앞으로 몰기
	int		i, j, k, count = 0;
	for(i = 0; i < MAX_INVEN_NUM; i ++)
	{
		if(pInven->Items[i].ItemID != 0)
			continue;
		for(j = MAX_INVEN_NUM - 1; j > i; j --)
		{
			if(pInven->Items[j].ItemID != 0)
				break;
		}
		if(j <= i)
			break;
		pInven->Items[i] = pInven->Items[j];
		pInven->Items[j].ItemID = 0;
	}
	count = i;

	// 같은 종류의 아이템 합치기
	for(i = 0; i < count; i ++)
	{
		if(pInven->Items[i].ItemID == 0)
			continue;

		uint32	ItemID = pInven->Items[i].ItemID;
		uint8	MoneyType = pInven->Items[i].MoneyType;
		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
		if ( mstItem == NULL )
		{
			sipwarning(" GetMstItemInfo failed. ItemID = %d", pInven->Items[i].ItemID);
			continue;
		}
		if((!mstItem->OverlapFlag) || (mstItem->OverlapMaxNum <= pInven->Items[i].ItemCount))
			continue;

		for(j = i + 1; j < count; j ++)
		{
			if((pInven->Items[j].ItemID != ItemID) || (pInven->Items[j].MoneyType != MoneyType))
				continue;

			// 합치기
			if(pInven->Items[i].ItemCount + pInven->Items[j].ItemCount <= mstItem->OverlapMaxNum)
			{
				pInven->Items[i].ItemCount += pInven->Items[j].ItemCount;
				pInven->Items[j].ItemID = 0;
				if(pInven->Items[i].ItemCount >= mstItem->OverlapMaxNum)
					break;
			}
			else
			{
				pInven->Items[j].ItemCount -= (mstItem->OverlapMaxNum - pInven->Items[i].ItemCount);
				pInven->Items[i].ItemCount = mstItem->OverlapMaxNum;
				break;
			}
		}
	}

	// 아이템 sort
	for(i = 0; i < count; i ++)
	{
		for(k = i; k < count; k ++)
		{
			if(pInven->Items[k].ItemID > 0)
				break;
		}
		if(k >= count)
			break;
		for(j = k + 1; j < count; j ++)
		{
			if((pInven->Items[j].ItemID > 0) && (pInven->Items[j].ItemID < pInven->Items[k].ItemID))
				k = j;
		}
		if(i != k)
		{
			ITEM_INFO	data = pInven->Items[i];
			pInven->Items[i] = pInven->Items[k];
			pInven->Items[k] = data;
		}
	}

	return i;
}*/

// Inven처리
// [d:SyncCode][b:ReqType][b:count][b:DelCount][[w:Inven position][w:Item Count][d:subtypeid][q:Getting time][d:Using CH id][b:MoneyType]][[w:Item Count][d:subtypeid][q:Getting time][b:MoneyType][b:DelType]]
void	cb_M_CS_INVENSYNC(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	uint32			SyncCode;		_msgin.serial(SyncCode);
	uint8			ReqType;		_msgin.serial(ReqType);

	uint16			DerssInvenPos = 0;
	uint64			DressGetDate = 0;

	uint32		ret = ERR_NOERR;

	//sipdebug("InvenSync Request. FID=%d, SyncCode=%d, ReqType=%d", FID, SyncCode, ReqType); byy krs
	switch(ReqType)
	{
	case IVS_CONFIRM:
		if(!CheckInvenStatue(FID, _msgin))
			ret = E_COMMON_FAILED;
		break;
	case IVS_APPLY:
		if(!ApplyInvenChange(FID, &DerssInvenPos, &DressGetDate, _msgin))
			ret = E_COMMON_FAILED;
		break;
/*	case IVS_TRIM:
		if(!ApplyInvenChange(FID, &DerssInvenPos, &DressGetDate, _msgin))
		{
			ret = E_COMMON_FAILED;
			break;
		}
		{
			int count = TrimInven(FID);

			_InvenItems	*pInven = GetFamilyInven(FID);
			if(pInven == NULL)
			{
				sipwarning("GetFamilyInven = NULL. FID=%d", FID);
				return;
			}

			// 보내기
			CMessage	msgOut(M_SC_FAMILYITEM);
			msgOut.serial(FID, SyncCode);

			uint16		InvenPos;

			for(uint16 ItemIndex = 0; ItemIndex < count; ItemIndex ++)
			{
				uint32	ItemID = pInven->Items[ItemIndex].ItemID;

				InvenPos = GetInvenPosFromInvenIndex((int)ItemIndex);
				msgOut.serial(InvenPos, ItemID, pInven->Items[ItemIndex].ItemCount, pInven->Items[ItemIndex].GetDate, pInven->Items[ItemIndex].UsedCharID, pInven->Items[ItemIndex].MoneyType);
			}

			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		}
		break;*/
	default:
		sipwarning("Invalid ReqType. (%d)", ReqType);
		return;
	}

	if(ReqType != IVS_CONFIRM && ret == ERR_NOERR)
	{
		_InvenItems	*pInven = GetFamilyInven(FID);
		if(pInven == NULL)
		{
			sipwarning("GetFamilyInven-Save = NULL. FID=%d", FID);
			return;
		}

		if(!SaveFamilyInven(FID, pInven))
		{
			sipwarning("SaveFamilyInven failed. FID=%d", FID);
			return;
		}
	}

//	if(ReqType != IVS_TRIM || ret != ERR_NOERR)
//	{
		CMessage	msgOut(M_SC_INVENSYNC);
		msgOut.serial(FID, SyncCode, ret);
		if(DerssInvenPos != 0)
		{
			msgOut.serial(DerssInvenPos, DressGetDate);
			//sipdebug("Item Use Start. FID=%d", FID);
		}
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
//	}

	if (ret != 0)
		sipwarning("FID=%d, ReqType=%d, Result=%d", FID, ReqType, ret);
}

// 아이템사기
// Buy item([b:MoneyType,1:UMoney,2:Point][b:Number][ [w:Inven pos][d:Item id][w:Count] ])
void	cb_M_CS_BUYITEM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	uint8			MoneyType;		_msgin.serial(MoneyType);
	uint8			uInvenPosNum;	_msgin.serial(uInvenPosNum);

	INFO_FAMILYINROOM*	pFamilyInfo = GetFamilyInfo(FID);
	if (pFamilyInfo == NULL)
	{
		sipwarning("GetFamilyInfo = NULL. FID=%d", FID);
		return ;
	}

	if ((uInvenPosNum == 0) || (uInvenPosNum > MAX_INVEN_NUM))
	{
		ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid inven pos number for buying item. FID=%d, uInvenPosNum=%d", FID, uInvenPosNum);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	_InvenItems	*pInven = GetFamilyInven(FID);
	if(pInven == NULL)
	{
		sipwarning("GetFamilyInven NULL. FID=%d", FID);
		return;
	}

	BUYIVENTEMPINFO		buyInfo[MAX_INVEN_NUM];
	T_PRICE totalUPrice = 0;
	T_PRICE totalGPrice = 0;

	// Check Packet & Inven
	for(uint8 i = 0; i < uInvenPosNum; i++)
	{
		_msgin.serial(buyInfo[i].InvenPos, buyInfo[i].ItemID, buyInfo[i].ItemNum);
		if ( !IsValidInvenPos(buyInfo[i].InvenPos) )
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid inven position for buying item. FID=%d, InvenPos=%d", FID, buyInfo[i].InvenPos);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}

		const MST_ITEM *tempMstItem = GetMstItemInfo(buyInfo[i].ItemID);
		if ( tempMstItem == NULL )
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid itemid for buying item : FID=%d, ItemID=%d", FID, buyInfo[i].ItemID);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}

		int		InvenIndex = GetInvenIndexFromInvenPos(buyInfo[i].InvenPos);
		if(buyInfo[i].ItemNum == 0) 
		{
			ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid ItemCount for buying item. FID=%d, ItemID=%d, ItemCount=%d, InvenPos=%d", FID, buyInfo[i].ItemID, buyInfo[i].ItemNum, buyInfo[i].InvenPos);
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;		
		}
		if(pInven->Items[InvenIndex].ItemID != 0)
		{
			if(tempMstItem->OverlapFlag == 0)
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"This item can't overlap!! FID=%d, ItemID=%d", FID, buyInfo[i].ItemID);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
			if((pInven->Items[InvenIndex].ItemID != buyInfo[i].ItemID))
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid inven pos - not equal item!! FID=%d, InvenPos=%d, ItemID=%d, MoneyType=%d, OldItemID=%d, OldItemMoneyType=%d", 
					FID, InvenIndex, buyInfo[i].ItemID, MoneyType, pInven->Items[InvenIndex].ItemID, pInven->Items[InvenIndex].MoneyType);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
			if(buyInfo[i].ItemNum + pInven->Items[InvenIndex].ItemCount > tempMstItem->OverlapMaxNum)
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"Invalid ItemCount for buying item. FID=%d, ItemID=%d, ItemCount=%d, InvenPos=%d", FID, buyInfo[i].ItemID, buyInfo[i].ItemNum, buyInfo[i].InvenPos);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;		
			}
		}

		if(MoneyType == MONEYTYPE_POINT)
		{
			if(tempMstItem->GMoney == 0)
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"Cannot this item using Point!! ItemID=%d", buyInfo[i].ItemID);
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
		}

		for(uint8 j = 0; j < i; j ++)
		{
			if(buyInfo[i].InvenPos == buyInfo[j].InvenPos)
			{
				ucstring ucUnlawContent = SIPBASE::_toString(L"Duplicate Invenpos!!!");
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;		
			}
		}

		totalUPrice += tempMstItem->GetRealUPrice() * buyInfo[i].ItemNum;
		totalGPrice += tempMstItem->GetRealGPrice() * buyInfo[i].ItemNum;			
	}

	if(MoneyType == MONEYTYPE_POINT)
		totalUPrice = 0;
	else
		totalGPrice = 0;

	if ( ! CheckMoneySufficient(FID, totalUPrice, totalGPrice, MoneyType) )
	{
//		ucstring ucUnlawContent = SIPBASE::_toString(L"There isn't enough money for buying items.");
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	// Check OK
//	CMessage	msgInven(M_SC_SETINVEN);
//	msgInven.serial(FID);
//	uint32 InitialLength1 = msgInven.length();

//	CMessage	msgInvenNum(M_SC_SETINVENNUM);
//	msgInvenNum.serial(FID);
//	uint32 InitialLength2 = msgInvenNum.length();

	CMessage	msgResult(M_SC_BUYITEM);
	uint32		uResult = ERR_NOERR;
	msgResult.serial(FID, uResult);
	uint32 InitialLength3 = msgResult.length();

	// the list of item, which is bought succeed;
	std::vector<T_FITEMPOS> hasBuyItemPosList;

	uint32	addpoint = 0;
	for(uint8 j = 0; j < uInvenPosNum; j++)
	{
		T_FITEMPOS		InvenPos = buyInfo[j].InvenPos;
		T_ITEMSUBTYPEID	ItemID = buyInfo[j].ItemID;
		T_FITEMNUM		ItemNum = buyInfo[j].ItemNum;

		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
		if ( mstItem == NULL )
		{
			sipwarning(L"GetMstItemInfo failed. ItemID = %d", ItemID);
			return;
		}

		T_PRICE		GPrice = 0;
		T_PRICE		UPrice = 0;
		if(MoneyType == MONEYTYPE_POINT)
			GPrice = mstItem->GetRealGPrice() * ItemNum;
		else
			UPrice = mstItem->GetRealUPrice() * ItemNum;

		addpoint += (mstItem->AddPoint * ItemNum);

		int		InvenIndex = GetInvenIndexFromInvenPos(InvenPos);
		if(pInven->Items[InvenIndex].ItemID == 0)
		{
			pInven->Items[InvenIndex].ItemID = ItemID;
			pInven->Items[InvenIndex].ItemCount = ItemNum;
			pInven->Items[InvenIndex].GetDate = 0; // getDate; - 처음엔 미사용아이템으로 설정한다.
			pInven->Items[InvenIndex].UsedCharID = 0;
			pInven->Items[InvenIndex].MoneyType = MONEYTYPE_UMONEY;

//			FAMILY_ITEM	fitem;
//			fitem.m_SubTypeId = ItemID;
//			fitem.m_bDelete = 0;
//			fitem.m_Num = ItemNum;
//			fitem.m_GetDate = pInven->Items[InvenIndex].GetDate;
//			fitem.m_Pos = InvenPos;
//			fitem.m_CHID = 0;
//			fitem.m_MoneyType = MoneyType;

//			SERIALINVEN(msgInven, (&fitem));
		}
		else
		{
			pInven->Items[InvenIndex].ItemCount += ItemNum;
			pInven->Items[InvenIndex].GetDate = 0; // getDate; - 처음엔 미사용아이템으로 설정한다.
			pInven->Items[InvenIndex].UsedCharID = 0;

//			msgInvenNum.serial(InvenPos, pInven->Items[InvenIndex].ItemCount);
		}

		// DB에서 Item판매수량 증가
		{
			MAKEQUERY_AddItemSellCount(strQ, ItemID, ItemNum);
			DBCaller->execute(strQ);
		}

		// Log
		DBLOG_BuyItem(FID, 0, ItemID, ItemNum, mstItem->Name, UPrice, GPrice);

		// Notify LS to subtract the user's money by sending message
		ExpendMoney(FID, UPrice, GPrice, 1, DBLOG_MAINTYPE_BUYITEM, DBLSUB_BUYITEM_NEW,
			mstItem->TypeID, ItemID, ItemNum, UPrice, 0);
	}

	if(!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
		return;
	}

	// 보내지 않는것으로 토의함
	//if ( msgInven.length() > InitialLength1 )
	//{
	//	CUnifiedNetwork::getInstance()->send(_tsid, msgInven);
	//}
	//if ( msgInvenNum.length() > InitialLength2 )
	//{
	//	CUnifiedNetwork::getInstance()->send(_tsid, msgInvenNum);
	//}

	CUnifiedNetwork::getInstance()->send(_tsid, msgResult);

	// 실지돈을 사용한 경우 경험값처리
	if((MoneyType == MONEYTYPE_UMONEY) && (addpoint > 0))
	{
		// ChangeFamilyExp(Family_Exp_Type_UseMoney, FID, totalUPrice, addpoint);
		ExpendMoney(FID, 0, addpoint, 2, DBLOG_MAINTYPE_CHFGMoney, 0, 0, 0);
	}

	SendFamilyProperty_Money(_tsid, FID, pFamilyInfo->m_nGMoney, pFamilyInfo->m_nUMoney);
}

// 아이템삭제
//void	cb_M_CS_THROWITEM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;			_msgin.serial(FID);					// 가족ID
//	T_FITEMPOS		ItemPos;		_msgin.serial(ItemPos);				// 아이템위치
//	T_FITEMNUM		ItemNum;		_msgin.serial(ItemNum);
//
//	if (ItemNum == 0)
//	{
//		ucstring ucUnlawContent = SIPBASE::_toString(L"Be Thrown item 0 number");
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//	if(!IsValidInvenPos(ItemPos))
//	{
//		// 비법파케트발견!!
//		ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos!!!").c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//
//	_InvenItems	*pInven = GetFamilyInven(FID);
//	if(pInven == NULL)
//	{
//		sipwarning("GetFamilyInven NULL");
//		return;
//	}
//
//	int		InvenIndex = GetInvenIndexFromInvenPos(ItemPos);
//	if(ItemNum > pInven->Items[InvenIndex].ItemCount)
//	{
//		ucstring ucUnlawContent = SIPBASE::_toString(L"Throw item error! ItemCount too big.");
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//
//	// 착용중인 복장도 버릴수 있다.
////	if(pInven->Items[InvenIndex].UsedCharID != 0)
////	{
////		ucstring ucUnlawContent = SIPBASE::_toString(L"Can't throw using item.");
////		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
////		return;
////	}
//	bool	bCharacterDressChanged = false;
//	if(pInven->Items[InvenIndex].UsedCharID != 0)
//		bCharacterDressChanged = true;
//
//	// Check OK
//	// Log
//	ucstring name = L"";
//	const MST_ITEM *mstItem = GetMstItemInfo(pInven->Items[InvenIndex].ItemID);					
//	if ( mstItem != NULL )
//	{
//		name = mstItem->Name;
//	}
//	DBLOG_ThrowItem(FID, 0, pInven->Items[InvenIndex].ItemID, ItemNum, name);
//
//	pInven->Items[InvenIndex].ItemCount -= ItemNum;
//	if(pInven->Items[InvenIndex].ItemCount <= 0)
//		pInven->Items[InvenIndex].ItemID = 0;
//
//	if(!SaveFamilyInven(FID, pInven))
//	{
//		sipwarning("SaveFamilyInven failed. FID=%d", FID);
//		return;
//	}
//
//	// 보내지 않는것으로 토의함
//	//if(pInven->Items[InvenIndex].ItemCount > 0)
//	//{
//	//	CMessage	msgInven(M_SC_SETINVENNUM);
//	//	msgInven.serial(FID);
//	//	msgInven.serial(ItemPos, pInven->Items[InvenIndex].ItemCount);
//	//	CUnifiedNetwork::getInstance()->send(_tsid, msgInven);
//	//}
//	//else
//	//{
//	//	uint8		bDelete = 1;
//	//	CMessage	msgInven(M_SC_SETINVEN);
//	//	msgInven.serial(FID);
//	//	msgInven.serial(ItemPos, bDelete);
//	//	CUnifiedNetwork::getInstance()->send(_tsid, msgInven);
//	//}
//
//	// Client에 보내지 않고 FS에서 끝나야 하는 Packet는 M_PROCPACKET에 담아서 보낸다.
//	CMessage msgRes(M_PROCPACKET);
//	MsgNameType	sPacket = M_CS_THROWITEM;
//	msgRes.serial(FID, sPacket);
//	CUnifiedNetwork::getInstance()->send(_tsid, msgRes);
//
//	// 착용중인 복장아이템을 버린 경우 해당한 처리를 하여야 한다.
//	if(bCharacterDressChanged)
//		CharacterDressChanged(FID, 0);
//}

// Timeout된 Item을 인벤에서 삭제하기
// Client에서 Timeout된 Item을 판단하여 삭제할것을 써버에 요청한다.
struct _TimeoutItemInfo
{
	uint32		ItemID;
	uint16		ItemCount;
	uint16		InvenPos;
	_TimeoutItemInfo(uint32 _ItemID, uint16 _ItemCount, uint16 _InvenPos) : 
	ItemID(_ItemID), ItemCount(_ItemCount), InvenPos(_InvenPos)
	{}
};
// Item timeout ([b:Number][ [w:InvenPos] ])
//void	cb_M_CS_TIMEOUT_ITEM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	std::list<_TimeoutItemInfo>	TimeoutItems;
//	std::list<_TimeoutItemInfo>::iterator it;
//
//	T_FAMILYID	FID;			_msgin.serial(FID);
//	uint8		count;			_msgin.serial(count);
//
//	if(count <= 0)
//		return;
//
//	uint16		InvenPos;
//	for(uint8 i = 0; i < count; i ++)
//	{
//		_msgin.serial(InvenPos);
//		if(!IsValidInvenPos(InvenPos))
//		{
//			// 비법파케트발견!!
//			ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos!!!").c_str();
//			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//			return;
//		}
//
//		for(it = TimeoutItems.begin(); it != TimeoutItems.end(); it ++)
//		{
//			if(InvenPos == it->InvenPos)
//			{
//				// 비법파케트발견!! - 같은 인벤위치가 있다.
//				ucstring ucUnlawContent = SIPBASE::_toString("Same InvenPos exist!!!").c_str();
//				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//				return;
//			}
//		}
//		_TimeoutItemInfo	info(0, 0, InvenPos);
//		TimeoutItems.push_back(info);
//	}
//
//	_InvenItems	*pInven = GetFamilyInven(FID);
//	if(pInven == NULL)
//	{
//		sipwarning("GetFamilyInven NULL");
//		return;
//	}
//
//	TTime	CurTime = GetDBTimeSecond();
//
//	for ( it = TimeoutItems.begin(); it != TimeoutItems.end(); it ++ )
//	{
//		int				InvenIndex = GetInvenIndexFromInvenPos(it->InvenPos);
//		ITEM_INFO		&item = pInven->Items[InvenIndex];
//		const MST_ITEM	*mstItem = GetMstItemInfo(item.ItemID);
//
//		if (mstItem && (mstItem->TermTime > 0))
//		{
//			if ((uint64)CurTime + 3 < item.GetDate + mstItem->TermTime * 60)	// 3초정도 여유를 준다.
//			{
//				// 비법파케트발견!!
//				ucstring ucUnlawContent = SIPBASE::_toString("M_CS_TIMEOUT_ITEM Invalid Data! UserFID=%d", FID).c_str();
//				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//				return;
//			}
//			it->ItemID = item.ItemID;
//			it->ItemCount = item.ItemCount;
//		}
//	}
//
//	// Check OK
//	// 아이템처리
//	bool	bCharacterDressChanged = false;
////	CMessage	msgTimeoutItem(M_SC_TIMEOUT_ITEM);
////	msgTimeoutItem.serial(FID);
//	uint32		ItemID = 0;		// M_SC_TIMEOUT_ITEM에서 ItemID와 ItemCount를 0으로 설정하고 보낸다.
//	uint16		ItemCount = 0;	// Client에서는 InvenPos만 가지고 검사한다.
//	for ( it = TimeoutItems.begin(); it != TimeoutItems.end(); it ++ )
//	{
//		int		InvenIndex = GetInvenIndexFromInvenPos(it->InvenPos);
//
//		if(pInven->Items[InvenIndex].UsedCharID != 0)
//			bCharacterDressChanged = true;
//
//		pInven->Items[InvenIndex].ItemID = 0;
////==	DBLOG_RefreshItem(FID, ItemAll[i].m_Id, ItemAll[i].m_SubTypeId, ItemAll[i].m_Num, mstItem->Name, uId, UserName, ucFName);
//
////		msgTimeoutItem.serial(ItemID, ItemCount, it->InvenPos);
//	}
//
//	if(!SaveFamilyInven(FID, pInven))
//	{
//		sipwarning("SaveFamilyInven failed. FID=%d", FID);
//		return;
//	}
//
//	if(bCharacterDressChanged)
//		CharacterDressChanged(FID, 0);
//
//	// 클라이언트에서 자체로 삭제하고, 보내지 않는다.
////	CUnifiedNetwork::getInstance()->send(_tsid, msgTimeoutItem);
//}

// Request trim family's items 
//void cb_M_CS_TRIMITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;			_msgin.serial(FID);
//
//	_InvenItems	*pInven = GetFamilyInven(FID);
//	if(pInven == NULL)
//	{
//		sipwarning("GetFamilyInven = NULL. FID=%d", FID);
//		return;
//	}
//
//	// 아이템 앞으로 몰기
//	int		i, j, k, count = 0;
//	for(i = 0; i < MAX_INVEN_NUM; i ++)
//	{
//		if(pInven->Items[i].ItemID != 0)
//			continue;
//		for(j = MAX_INVEN_NUM - 1; j > i; j --)
//		{
//			if(pInven->Items[j].ItemID != 0)
//				break;
//		}
//		if(j <= i)
//			break;
//		pInven->Items[i] = pInven->Items[j];
//		pInven->Items[j].ItemID = 0;
//	}
//	count = i;
//
//	// 같은 종류의 아이템 합치기
//	for(i = 0; i < count; i ++)
//	{
//		if(pInven->Items[i].ItemID == 0)
//			continue;
//
//		uint32	ItemID = pInven->Items[i].ItemID;
//		uint8	MoneyType = pInven->Items[i].MoneyType;
//		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
//		if ( mstItem == NULL )
//		{
//			sipwarning(" GetMstItemInfo failed. ItemID = %d", pInven->Items[i].ItemID);
//			continue;
//		}
//		if((!mstItem->OverlapFlag) || (mstItem->OverlapMaxNum <= pInven->Items[i].ItemCount))
//			continue;
//
//		for(j = i + 1; j < count; j ++)
//		{
//			if((pInven->Items[j].ItemID != ItemID) || (pInven->Items[j].MoneyType != MoneyType))
//				continue;
//
//			// 합치기
//			if(pInven->Items[i].ItemCount + pInven->Items[j].ItemCount <= mstItem->OverlapMaxNum)
//			{
//				pInven->Items[i].ItemCount += pInven->Items[j].ItemCount;
//				pInven->Items[j].ItemID = 0;
//				if(pInven->Items[i].ItemCount >= mstItem->OverlapMaxNum)
//					break;
//			}
//			else
//			{
//				pInven->Items[j].ItemCount -= (mstItem->OverlapMaxNum - pInven->Items[i].ItemCount);
//				pInven->Items[i].ItemCount = mstItem->OverlapMaxNum;
//				break;
//			}
//		}
//	}
//
//	// 아이템 sort
//	for(i = 0; i < count; i ++)
//	{
//		for(k = i; k < count; k ++)
//		{
//			if(pInven->Items[k].ItemID > 0)
//				break;
//		}
//		if(k >= count)
//			break;
//		for(j = k + 1; j < count; j ++)
//		{
//			if((pInven->Items[j].ItemID > 0) && (pInven->Items[j].ItemID < pInven->Items[k].ItemID))
//				k = j;
//		}
//		if(i != k)
//		{
//			ITEM_INFO	data = pInven->Items[i];
//			pInven->Items[i] = pInven->Items[k];
//			pInven->Items[k] = data;
//		}
//	}
//	count = i;
//
//	//CMessage	msgOut(M_SC_TRIMITEM);
//	//msgOut.serial(FID);
//	//CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
//
//	uint16	InvenPos;
//	uint8	bDelete = 2;
//	CMessage	msgInven(M_SC_SETINVEN);
//	msgInven.serial(FID);
//	for(i = 0; i < count; i ++)
//	{
//		InvenPos = GetInvenPosFromInvenIndex(i);
//		msgInven.serial(InvenPos, bDelete, pInven->Items[i].ItemID, pInven->Items[i].ItemCount, pInven->Items[i].GetDate, pInven->Items[i].UsedCharID, pInven->Items[i].MoneyType);
//	}
//	CUnifiedNetwork::getInstance()->send(_tsid, msgInven);
//
//	sipinfo(L"Family(%d)'s trim his items' position ", FID);
//}

//// Get BankItem List
//void cb_M_CS_BANKITEM_LIST(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
//{
//	T_FAMILYID		FID;			_msgin.serial(FID);	
//
//	FAMILY	*pFamily = GetFamilyData(FID);
//	if(pFamily == NULL)
//	{
//		sipwarning("GetFamilyData = NULL. FID=%d", FID);
//		return;
//	}
//	uint32		UID = pFamily->m_UserID;
//	uint16		FSSid = _tsid.get();
//
//	CMessage	msgOut(M_CS_BANKITEM_LIST);
//	msgOut.serial(FSSid, FID, UID);
//	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
//}

//void cb_M_CS_BANKITEM_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		FID;			_msgin.serial(FID);	
//
//	FAMILY	*pFamily = GetFamilyData(FID);
//	if(pFamily == NULL)
//	{
//		sipwarning("GetFamilyData = NULL. FID=%d", FID);
//		return;
//	}
//	uint32		UID = pFamily->m_UserID;
//
//	CMessage	msgOut;
//	_msgin.copyWithPrefix(UID, msgOut);
//	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
//}

//// 은행아이템목록
//void cb_M_SC_BANKITEM_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	CMessage	msgOut;
//	uint16		FSSid;
//	_msgin.copyWithoutPrefix(FSSid, msgOut);
//	TServiceId	FS(FSSid);
//	CUnifiedNetwork::getInstance()->send(FS, msgOut);
//}

//// 은행아이템 받기
//// [b:Count][[d:IndexID][d:ItemID][d:ItemCount]]
//void cb_M_MS_BANKITEM_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	uint32		FID;		_msgin.serial(FID);
//	uint32		UID;		_msgin.serial(UID);
//
//	FAMILY*	pFamily = GetFamilyData(FID);
//	if(pFamily == NULL)
//	{
//		sipdebug("GetFamilyData = NULL. FID=%d", FID);
//		return;
//	}
//
//	_InvenItems	*pInven = GetFamilyInven(FID);
//	if(pInven == NULL)
//	{
//		sipwarning("GetFamilyInven = NULL. FID=%d", FID);
//		return;
//	}
//
//	uint8		count;		_msgin.serial(count);
//	int			emptypos_index = 0;
//	for(uint8 i = 0; i < count; i ++)
//	{
//		uint32	IndexID;	_msgin.serial(IndexID);
//		uint32	ItemID;		_msgin.serial(ItemID);
//		uint32	ItemCount;	_msgin.serial(ItemCount);
//
//		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
//		if ( mstItem == NULL )
//		{
//			sipwarning(" GetMstItemInfo = NULL. ItemID = %d", ItemID);
//			continue;
//		}
//
//		// 인벤에 넣기
//		uint32		addcount_all = 0;
//		for(int j = 0; j < MAX_INVEN_NUM; j ++)
//		{
//			uint32		addcount = 0;
//			if(mstItem->OverlapFlag)
//			{
//				if(pInven->Items[j].ItemID == 0)
//					addcount = mstItem->OverlapMaxNum;
///* 빈자리에만 넣을수 있는것으로 토의함 ---
//				else if(ItemID == pInven->Items[j].ItemID)
//					addcount = mstItem->OverlapMaxNum - pInven->Items[j].ItemCount;
//*/
//			}
//			else
//			{
//				if(pInven->Items[j].ItemID == 0)
//					addcount = 1;
//			}
//			if(addcount == 0)
//				continue;
//
//			// OK addcount개수만큼 넣을수 있다.
//			uint16	InvenPos = GetInvenPosFromInvenIndex(j);
//			if(addcount > ItemCount)
//				addcount = ItemCount;
//			if(pInven->Items[j].ItemID == 0)
//			{
//				// Item 빈칸에 추가
//				pInven->Items[j].ItemID = ItemID;
//				pInven->Items[j].ItemCount = (uint16)addcount;
//				pInven->Items[j].UsedCharID = 0;
//				pInven->Items[j].GetDate = 0; // GetDBTimeSecond();  - 처음엔 미사용아이템으로 설정한다.
//				pInven->Items[j].MoneyType = MONEYTYPE_POINT;
//
//				// Client에 통지
//				FAMILY_ITEM	fitem;
//				fitem.m_SubTypeId = ItemID;
//				fitem.m_bDelete = 0;
//				fitem.m_Num = (uint16)addcount;
//				fitem.m_GetDate = pInven->Items[j].GetDate;
//				fitem.m_Pos = InvenPos;
//				fitem.m_CHID = 0;
//				fitem.m_MoneyType = MONEYTYPE_POINT;
//
//				CMessage	msgInven(M_SC_SETINVEN);
//				msgInven.serial(FID);
//				SERIALINVEN(msgInven, (&fitem));
//				CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgInven);
//			}
///* 빈자리에만 넣을수 있는것으로 토의함 ---
//			else
//			{
//				// Item 이미 있던칸에 겹치기
//				pInven->Items[j].ItemCount += (uint16)addcount;
//
//				CMessage	msgInven(M_SC_SETINVENNUM);
//				msgInven.serial(FID);
//				msgInven.serial(InvenPos, pInven->Items[j].ItemCount);
//				CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgInven);
//			}
//*/
//			ItemCount -= addcount;
//			addcount_all += addcount;
//
//			if(ItemCount <= 0)
//				break;
//		}
//
//		// Log
//		DBLOG_GetBankItem(FID, ItemID, addcount_all, mstItem->Name);
//
//		// 인벤에 다 넣지 못했다면 다시 MainServer로 통지한다.
//		if(ItemCount > 0)
//		{
//			uint8	fail_count = 1;
//			CMessage	msgOut(M_SM_BANKITEM_GET_FAIL);
//			msgOut.serial(UID, fail_count, IndexID, ItemCount);
//			CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
//		}
//	}
//
//	if(!SaveFamilyInven(FID, pInven))
//	{
//		sipwarning("SaveFamilyInven failed. FID=%d", FID);
//		return;
//	}
//}

// GM이 아이템주기
// Give item([d:FamilyID][d:Item id][d:Count])
//void	cb_M_GMCS_GIVEITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID		GMID;		_msgin.serial(GMID);
//	T_FAMILYID		FID;		_msgin.serial(FID);
//	T_ITEMSUBTYPEID	ItemSubID;	_msgin.serial(ItemSubID);
//	uint32			_ItemNum;	_msgin.serial(_ItemNum);
//	uint16			ItemNum = (uint16)_ItemNum;
//	uint8			MoneyType;	_msgin.serial(MoneyType);
//
//	const MST_ITEM *mstItem = GetMstItemInfo(ItemSubID);
//	if (mstItem == NULL)
//	{
//		sipinfo(L"There is no item info for receiving GM's item = %d", ItemSubID);
//		return;
//	}
//
//	ucstring		FName = "";
//	bool			bIsLogin;
//	_InvenItems*	pInven = GetFamilyInven(FID, false, &bIsLogin);
//	if(pInven == NULL)
//	{
//		uint8	bFamilyIs = 0, bEmptyIs = 0;
//		CMessage mes(M_GMSC_GETINVENEMPTY);
//		mes.serial(GMID, FID, bFamilyIs, FName, bEmptyIs);
//		CUnifiedNetwork::getInstance ()->send(_tsid, mes);
//		sipinfo(L"There is no family for receiving GM's item.");
//		return;
//	}
//
//	FAMILY		*pFamily = NULL;
//	if(bIsLogin)
//	{
//		pFamily = GetFamilyData(FID);
//		if (pFamily == NULL)
//		{
//			sipinfo(L"GetFamilyData failed.");
//			return;
//		}
//		FName = pFamily->m_Name;
//	}
//
//	// 빈자리찾기
//	bool	bNew = false;
//	int		InvenIndex;
//	for(InvenIndex = 0; InvenIndex < MAX_INVEN_NUM; InvenIndex ++)
//	{
//		if(pInven->Items[InvenIndex].ItemID == 0)
//		{
//			bNew = true;
//			break;
//		}
//		if((pInven->Items[InvenIndex].ItemID == ItemSubID) 
//			&& (mstItem->OverlapFlag)
//			&& (pInven->Items[InvenIndex].MoneyType == MoneyType)
//			&& (mstItem->OverlapMaxNum >= pInven->Items[InvenIndex].ItemCount + ItemNum))
//			break;
//	}
//	if(InvenIndex >= MAX_INVEN_NUM)
//	{
//		uint8	bFamilyIs = 1, bEmptyIs = 0;
//		CMessage mes(M_GMSC_GETINVENEMPTY);
//		mes.serial(GMID, FID, bFamilyIs, FName, bEmptyIs);
//		CUnifiedNetwork::getInstance ()->send(_tsid, mes);
//		sipinfo(L"There is no empty pos for receiving GM's item, FName=%s", FName.c_str());
//		return;
//	}
//
//	// Item주기
//	if(bNew)
//	{
//		pInven->Items[InvenIndex].ItemID = ItemSubID;
//		pInven->Items[InvenIndex].ItemCount = ItemNum;
//		pInven->Items[InvenIndex].GetDate = GetDBTimeSecond();
//		pInven->Items[InvenIndex].MoneyType = MoneyType;
//
//		if(pFamily)
//		{
//			FAMILY_ITEM	fitem;
//			fitem.m_SubTypeId = ItemSubID;
//			fitem.m_bDelete = 0;
//			fitem.m_Num = ItemNum;
//			fitem.m_GetDate = pInven->Items[InvenIndex].GetDate;
//			fitem.m_Pos = GetInvenPosFromInvenIndex(InvenIndex);
//			fitem.m_CHID = 0;
//			fitem.m_MoneyType = MoneyType;
//
//			CMessage	msgInven(M_SC_SETINVEN);
//			msgInven.serial(FID);
//			SERIALINVEN(msgInven, (&fitem));
//			CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgInven);
//		}
//	}
//	else
//	{
//		pInven->Items[InvenIndex].ItemCount += ItemNum;
//		pInven->Items[InvenIndex].GetDate = GetDBTimeSecond();
//
//		if(pFamily)
//		{
//			uint16	candipos = GetInvenPosFromInvenIndex(InvenIndex);
//			CMessage	msgInven(M_SC_SETINVENNUM);
//			msgInven.serial(FID);
//			msgInven.serial(candipos, pInven->Items[InvenIndex].ItemCount);
//			CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgInven);
//		}
//	}
//
//	if(!bIsLogin)
//	{
//		if(!SaveFamilyInven(FID, pInven))
//		{
//			sipinfo("SaveFamilyInven of a logoff user failed. %d=", FID);
//			return;
//		}
//	}
//
//	uint8	bIs = 1;
//	CMessage mes(M_GMSC_GETITEM);
//	mes.serial(GMID, FID, bIs, FName, ItemSubID, _ItemNum);
//	CUnifiedNetwork::getInstance()->send(_tsid, mes);
//
//	// GM Log
//	uint32		Uid = 0;
//	ucstring	UName = "";
//	GMLOG_GiveItem(GMID, FID, Uid, FName, UName, ItemSubID, ItemNum, mstItem->Name);
//}

// 아이템개수얻기 - 정확한 service에 보내지 못하면 오유가 있을수 있음.
// Get item count([d:FamilyID][d:Item id])
void	cb_M_GMCS_GETITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_ITEMSUBTYPEID	ItemID;		_msgin.serial(ItemID);

	uint8	bIs = 0;
	uint32	uNum = 0;
	T_FAMILYNAME	FName = L"";

	bool			bIsLogin = 0;
	_InvenItems*	pInven = GetFamilyInven(FID); // , false, &bIsLogin);
	if (pInven == NULL)
	{
		sipinfo(L"There is no family.");
	}
	else
	{
		bIsLogin = 1;
		FAMILY		*pFamily = NULL;
		if (bIsLogin)
		{
			pFamily = GetFamilyData(FID);
			if (pFamily == NULL)
			{
				sipinfo(L"GetFamilyData failed.");
				return;
			}
			FName = pFamily->m_Name;
		}

		bIs = 1;
		for (int InvenIndex = 0; InvenIndex < MAX_INVEN_NUM; InvenIndex ++)
		{
			if (pInven->Items[InvenIndex].ItemID == ItemID)
				uNum += pInven->Items[InvenIndex].ItemCount;
		}
	}

	CMessage mes(M_GMSC_GETITEM);
	mes.serial(GMID, FID, bIs, FName, ItemID, uNum);
	CUnifiedNetwork::getInstance()->send(_tsid, mes);
}

// 배낭에 빈자리 있는가 알아보기 - 정확한 service에 보내지 못하면 오유가 있을수 있음.
// Is there empty inventory position([d:FamilyID])
void	cb_M_GMCS_GETINVENEMPTY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;		_msgin.serial(GMID);
	T_FAMILYID		FID;		_msgin.serial(FID);
	T_FAMILYNAME	FName = L"";

	uint8	bIs = 0;
	uint8	bEmptyIs = 0;

	bool			bIsLogin = true;
	_InvenItems*	pInven = GetFamilyInven(FID); // , false, &bIsLogin);
	if (pInven == NULL)
	{
		sipinfo(L"There is no family.");
	}
	else
	{
		bEmptyIs = 1;

		FAMILY		*pFamily = NULL;
		if (bIsLogin)
		{
			pFamily = GetFamilyData(FID);
			if (pFamily == NULL)
			{
				sipinfo(L"GetFamilyData failed.");
				return;
			}
			FName = pFamily->m_Name;
		}

		bIs = 1;
		for (int InvenIndex = 0; InvenIndex < MAX_INVEN_NUM; InvenIndex ++)
		{
			if (pInven->Items[InvenIndex].ItemID == 0)
			{
				bEmptyIs = 1;
				break;
			}
		}
	}

	CMessage mes(M_GMSC_GETINVENEMPTY);
	mes.serial(GMID, FID, bIs, FName, bEmptyIs);
	CUnifiedNetwork::getInstance ()->send(_tsid, mes);
}

//////////////////////////////////////////////////////////////////////////
//void	CRoomWorld::BurnItem(T_FAMILYID _FID,  T_FITEMPOS _Pos, T_FITEMNUM _Num, SIPNET::TServiceId _sid)
//{
//	if ( !IsValidInvenPos(_Pos) )
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos !!!! Pos=%d", _Pos);
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
//		return;
//	}
//
//	_InvenItems*	pInven = GetFamilyInven(_FID);
//	if(pInven == NULL)
//	{
//		sipwarning("GetFamilyInven NULL");
//		return;
//	}
//
//	int	InvenIndex = GetInvenIndexFromInvenPos(_Pos);
//	ITEM_INFO	&item = pInven->Items[InvenIndex];
//
//	if((item.ItemID == 0) || (_Num == 0) || (_Num > item.ItemCount))
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("Invalid Parameter !!");
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, _FID);
//		return;
//	}
//
//	// OK
//	uint32	ItemID = item.ItemID;
//	const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
//	if (mstItem == NULL)
//	{
//		sipinfo(L"There is no item info for used item = %d", ItemID);
//		return;
//	}
//	if(_Num < item.ItemCount)
//	{
//		item.ItemCount -= _Num;
//
//		CMessage	msgInven(M_SC_SETINVENNUM);
//		msgInven.serial(_FID);
//		msgInven.serial(_Pos, item.ItemCount);
//		CUnifiedNetwork::getInstance()->send(_sid, msgInven);
//	}
//	else
//	{
//		item.ItemID = 0;
//
//		uint8		bDelete = 1;
//		CMessage	msgInven(M_SC_SETINVEN);
//		msgInven.serial(_FID);
//		msgInven.serial(_Pos, bDelete);
//		CUnifiedNetwork::getInstance()->send(_sid, msgInven);
//	}
//
//	// Log
//	DBLOG_BurnItem(_FID, 0, ItemID, _Num, mstItem->Name, m_RoomName, m_RoomID);
//}

//============================================================
//   기념일활동관련
//============================================================
SITE_ACTIVITY	g_SiteActivity;
SYSTEM_GIFTITEM	g_BeginnerMstItem;
// WS에서 WorldService,MS에 현재의 기념일활동자료를 보낸다. ([d:ActID][u:Text][b:Count][ [d:ItemID][b:ItemCount] ])
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

// Request current activity items (Attention!! Client must check inven)
void cb_M_CS_REQ_ACTIVITY_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32	FID;		_msgin.serial(FID);

	FAMILY*	pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipdebug("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	if (g_SiteActivity.m_ActID == 0)
	{
		sipwarning("g_SiteActivity.m_ActID == 0!!! FID=%d", FID);
		return;
	}

	CMessage	msgOut(M_SM_SET_USER_ACTIVITY);
	msgOut.serial(pFamily->m_UserID, FID, g_SiteActivity.m_ActID);
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
}

// LS->WS->WorldService, 사용자가  기념일활동아이템 받기 요청에 대한 응답 ([d:FID])
void cb_M_MS_SET_USER_ACTIVITY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32	FID;	_msgin.serial(FID);

	FAMILY*	pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipdebug("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	_InvenItems	*pInven = GetFamilyInven(FID);
	if (pInven == NULL)
	{
		sipwarning("GetFamilyInven = NULL. FID=%d", FID);
		return;
	}

	for (uint8 i = 0; i < g_SiteActivity.m_Count; i ++)
	{
		uint32	ItemID = g_SiteActivity.m_ActItemID[i];
		uint32	ItemCount = g_SiteActivity.m_ActItemCount[i];

		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
		if (mstItem == NULL)
		{
			sipwarning("GetMstItemInfo = NULL. ItemID = %d", ItemID);
			continue;
		}

		int j;
		for (j = 0; j < MAX_INVEN_NUM; j ++)
		{
			uint32		addcount = 0;

			// 빈자리에만 넣을수 있다.
			if (mstItem->OverlapFlag)
			{
				if (pInven->Items[j].ItemID == 0)
					addcount = mstItem->OverlapMaxNum;
			}
			else
			{
				if (pInven->Items[j].ItemID == 0)
					addcount = 1;
			}
			if (addcount == 0)
				continue;

			// OK addcount개수만큼 넣을수 있다.
			uint16	InvenPos = GetInvenPosFromInvenIndex(j);
			if(addcount > ItemCount)
				addcount = ItemCount;
			if (pInven->Items[j].ItemID == 0)
			{
				// Item 빈칸에 추가
				pInven->Items[j].ItemID = ItemID;
				pInven->Items[j].ItemCount = (uint16)addcount;
				pInven->Items[j].UsedCharID = 0;
				pInven->Items[j].GetDate = 0; // 미사용아이템으로 설정한다.
				pInven->Items[j].MoneyType = MONEYTYPE_UMONEY;

				// Client에 통지
				FAMILY_ITEM	fitem;
				fitem.m_SubTypeId = ItemID;
				fitem.m_bDelete = 0;
				fitem.m_Num = (uint16)addcount;
				fitem.m_GetDate = pInven->Items[j].GetDate;
				fitem.m_Pos = InvenPos;
				fitem.m_CHID = 0;
				fitem.m_MoneyType = MONEYTYPE_UMONEY;

				CMessage	msgInven(M_SC_SETINVEN);
				msgInven.serial(FID);
				SERIALINVEN(msgInven, (&fitem));
				CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgInven);

			}
			ItemCount -= addcount;

			if(ItemCount <= 0)
				break;
		}
		if (j >= MAX_INVEN_NUM)
		{
			sipwarning("j >= MAX_INVEN_NUM. FID=%d", FID);
			break;
		}
		// Log
		DBLOG_GetBankItem(FID, ItemID, ItemCount, mstItem->Name);
	}

	if(!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
		return;
	}

	//sipinfo("User receive Activity Item. FID=%d, ActID=%d", FID, g_SiteActivity.m_ActID); byy krs
}

// Receive gift item([d:GiftId])
void cb_M_CS_RECV_GIFTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32	FID;	_msgin.serial(FID);

	FAMILY*	pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipdebug("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	if (g_BeginnerMstItem.m_GiftID == 0)
	{
		sipwarning("g_BeginnerMstItem.m_GiftID == 0!!! FID=%d", FID);
		return;
	}

	CMessage	msgOut(M_SM_SET_USER_BEGINNERMSTITEM);
	msgOut.serial(pFamily->m_UserID, FID);
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
}

// LS->WS->WorldService, 사용자가  새사용자아이템을 받았음을 설정한다. ([d:FID])
void	cb_M_MS_SET_USER_BEGINNERMSTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32	FID;	_msgin.serial(FID);

	FAMILY*	pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipdebug("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	_InvenItems	*pInven = GetFamilyInven(FID);
	if (pInven == NULL)
	{
		sipwarning("GetFamilyInven = NULL. FID=%d", FID);
		return;
	}

	for (uint8 i = 0; i < g_BeginnerMstItem.m_Count; i ++)
	{
		uint32	ItemID = g_BeginnerMstItem.m_ItemID[i];
		uint32	ItemCount = g_BeginnerMstItem.m_ItemCount[i];

		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);
		if (mstItem == NULL)
		{
			sipwarning("GetMstItemInfo = NULL. ItemID = %d", ItemID);
			continue;
		}

		int j;
		for (j = 0; j < MAX_INVEN_NUM; j ++)
		{
			uint32		addcount = 0;

			// 빈자리에만 넣을수 있다.
			if (mstItem->OverlapFlag)
			{
				if (pInven->Items[j].ItemID == 0)
					addcount = mstItem->OverlapMaxNum;
			}
			else
			{
				if (pInven->Items[j].ItemID == 0)
					addcount = 1;
			}
			if (addcount == 0)
				continue;

			// OK addcount개수만큼 넣을수 있다.
			uint16	InvenPos = GetInvenPosFromInvenIndex(j);
			if(addcount > ItemCount)
				addcount = ItemCount;
			if (pInven->Items[j].ItemID == 0)
			{
				// Item 빈칸에 추가
				pInven->Items[j].ItemID = ItemID;
				pInven->Items[j].ItemCount = (uint16)addcount;
				pInven->Items[j].UsedCharID = 0;
				pInven->Items[j].GetDate = 0; // 미사용아이템으로 설정한다.
				pInven->Items[j].MoneyType = MONEYTYPE_UMONEY;

				// Client에 통지
				FAMILY_ITEM	fitem;
				fitem.m_SubTypeId = ItemID;
				fitem.m_bDelete = 0;
				fitem.m_Num = (uint16)addcount;
				fitem.m_GetDate = pInven->Items[j].GetDate;
				fitem.m_Pos = InvenPos;
				fitem.m_CHID = 0;
				fitem.m_MoneyType = MONEYTYPE_UMONEY;

				CMessage	msgInven(M_SC_SETINVEN);
				msgInven.serial(FID);
				SERIALINVEN(msgInven, (&fitem));
				CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgInven);
			}
			ItemCount -= addcount;

			if(ItemCount <= 0)
				break;
		}
		if (j >= MAX_INVEN_NUM)
		{
			sipwarning("j >= MAX_INVEN_NUM. FID=%d", FID);
			break;
		}
		// Log
		DBLOG_GetBankItem(FID, ItemID, ItemCount, mstItem->Name);
	}

	if(!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
		return;
	}

	//sipinfo("User receive Beginner Item. FID=%d", FID); byy krs
}
