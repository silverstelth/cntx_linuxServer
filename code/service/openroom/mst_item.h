
#ifndef MST_ITEM_H
#define MST_ITEM_H

#include "../../common/Packet.h"
#include "../../common/common.h"
#include "../Common/Common.h"

// 아이템마스터자료
struct MST_ITEM 
{
	T_ITEMSUBTYPEID							SubID;						// subtype id
	T_ITEMTYPEID							TypeID;
	T_COMMON_NAME							Name;
	T_PRICE									GMoney;
	T_PRICEDISC								DiscGMoney;
	T_PRICE									UMoney;
	T_PRICEDISC								DiscUMoney;
	uint32									AddPoint;
	T_F_EXP									ExpInc;
	T_F_VIRTUE								VirtueInc;
	T_F_FAME								FameInc;
	T_ROOMEXP								RoomExpInc;
	T_ROOMVIRTUE							RoomVirtueInc;
	T_ROOMFAME								RoomFameInc;
	T_ROOMAPPRECIATION						RoomApprInc;
	uint32									Level;
	T_USEDTIME								UseTime;					// minutes
	T_USEDTIME								TermTime;					// minutes
	T_FLAG									OverlapFlag;
	T_FITEMNUM								OverlapMaxNum;
	T_PRICE		GetRealGPrice()				const
	{
		return (GMoney * DiscGMoney) / 100;
	}
	T_PRICE		GetRealUPrice()				const
	{
		return (UMoney * DiscUMoney) / 100;
	}
};
typedef		std::map<T_ITEMSUBTYPEID,	MST_ITEM>					MAP_MST_ITEM;
extern		MAP_MST_ITEM				map_MST_ITEM;

extern	void	SendMstItemData(T_FAMILYID	_FID, SIPNET::TServiceId _tsid);
extern	const	MST_ITEM *GetMstItemInfo(T_ITEMSUBTYPEID _subid);

#endif // MST_ITEM_H

