#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>
#include <misc/stream.h>
#include <misc/db_interface.h>
#include "misc/DBCaller.h"

#include <map>
#include <utility>

#include "../Common/DBData.h"
#include "../Common/DBLog.h"
#include "../Common/QuerySetForShard.h"

#include "Family.h"
#include "contact.h"
#include "outRoomKind.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller	*DBCaller;


//bool	IsBlackList(T_FAMILYID _FID1, T_FAMILYID _FID2)
//{
//	MAKEQUERY_CheckBlacklist(strQ, _FID1, _FID2);
//	CDBConnectionRest	DB(DBCaller);
//	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
//	if (!nPrepareRet)
//		return false;
//
//	int len1 = 0;
//	int	ret = 0;
//
//	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, sizeof(uint32), &len1);
//
//	if(!nBindResult)
//		return false;
//
//	DB_EXE_PARAMQUERY(Stmt, strQ);
//	if(!nQueryResult)
//		return false;
//
//	if(ret == 1035)
//	{
//		return true;
//	}
//	return false;
//}
