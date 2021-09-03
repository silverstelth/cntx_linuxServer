#include "ManageComponent.h"
#include "QuerySet.h"
#include "misc/db_interface.h"
#include "../../common/Packet.h"
#include "ManageAccount.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;


static	MAP_MANAGEPOWER				map_MANAGEPOWER;

#define	FOR_START_MAP_MANAGEPOWER(it)											\
	MAP_MANAGEPOWER::iterator	it;												\
	for(it = map_MANAGEPOWER.begin(); it != map_MANAGEPOWER.end(); it++)		\


static void	DBCB_DBGetAllManageHostPower(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32 uID;				resSet->serial(uID);
		uint32 uType;			resSet->serial(uType);
		ucstring ucComment;		resSet->serial(ucComment);
		uint32 uValue;			resSet->serial(uValue);

		map_MANAGEPOWER.insert( make_pair(uID, MANAGEPOWER(uID, uType, ucComment, uValue)) );
	}
}
//// 관리자권한정보표를 load한다
//static	void	LoadManagePower()
//{
//	map_MANAGEPOWER.clear();
//	MAKEQUERY_GETALLMANAGEHOSTPOWER(sQ);
//	DBCaller->execute(sQ, DBCB_DBGetAllManageHostPower);
//}

static void	Send_M_MH_MANAGEPOWER(TSockId from)
{
	CMessage	mes(M_MH_MANAGEPOWER);

	FOR_START_MAP_MANAGEPOWER(it)
	{
		mes.serial(it->second.m_uID, it->second.m_uType, it->second.m_ucComment, it->second.m_uValue);
	}

	ManageHostServer->send(mes, from);
}

// 관리자권한정보표얻기
static void cb_M_MH_MANAGEPOWER( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	Send_M_MH_MANAGEPOWER(from);
}

static TCallbackItem CallbackArray[] =
{
	{ M_MH_MANAGEPOWER,		cb_M_MH_MANAGEPOWER },
};

void	InitManagePower()
{
	ManageHostServer->addCallbackArray (CallbackArray, sizeof(CallbackArray)/sizeof(TCallbackItem));
//	LoadManagePower();
}
