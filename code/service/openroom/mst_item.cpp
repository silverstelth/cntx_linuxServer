#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>

#include "mst_item.h"

#include "misc/db_interface.h"
#include "../Common/QuerySetForShard.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

//extern DBCONNECTION	DB;

extern	MAP_MST_ITEM				map_MST_ITEM;

// 아이템 master정보를 보낸다
void	SendMstItemData(T_FAMILYID _FID, TServiceId _tsid)
{
	uint32 nSize = map_MST_ITEM.size();
	if (nSize < 1)
		return;

	MAP_MST_ITEM::iterator	it = map_MST_ITEM.begin();
	while (it != map_MST_ITEM.end())
	{
		uint32 uNum = 0;
		CMessage	msgOut(M_SC_MSTITEM);
		msgOut.serial(_FID);
		while (it != map_MST_ITEM.end())
		{
			msgOut.serial(it->second.SubID, it->second.GMoney, it->second.DiscGMoney, it->second.UMoney, it->second.DiscUMoney, it->second.AddPoint);
			msgOut.serial(it->second.ExpInc, it->second.VirtueInc, it->second.FameInc, it->second.UseTime, it->second.TermTime);
			msgOut.serial(it->second.OverlapFlag, it->second.OverlapMaxNum, it->second.Level);
			it ++;
			uNum ++;
			if (uNum >= 50)
				break;
		}
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
}

// 아이템 master정보를 얻는다
const MST_ITEM *GetMstItemInfo(T_ITEMSUBTYPEID _subid)
{
	MAP_MST_ITEM::iterator	it;
	it = map_MST_ITEM.find(_subid);
	if (it == map_MST_ITEM.end())
		return NULL;
	return &(it->second);
}
