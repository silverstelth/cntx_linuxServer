#include <misc/types_sip.h>
#include "misc/DBCaller.h"
#include <net/service.h>

#include "../../common/Packet.h"
#include "../Common/Common.h"
#include "../Common/QuerySetForShard.h"
#include "silence.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern SIPBASE::TTime GetDBTimeSecond();

extern CDBCaller	*DBCaller;

static map<T_FAMILYID, uint32>	g_mapSilencePlayers;

void SendSilenceListToFS(TServiceId _tsid)
{
	uint32	now = (uint32)GetDBTimeSecond();
	CMessage msgOut(M_NT_SET_DBTIME);
	msgOut.serial(now);
	if (_tsid.get() == 0)
		CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
	else
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

	for (map<T_FAMILYID, uint32>::iterator it = g_mapSilencePlayers.begin(); it != g_mapSilencePlayers.end(); it ++)
	{
		CMessage msgOut(M_NT_SILENCE_SET);
		T_FAMILYID	FID = it->first;
		msgOut.serial(FID, it->second);
		if (_tsid.get() == 0)
			CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
		else
			CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}
}

static void	DBCB_DBGetSilenceList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i ++)
	{
		T_FAMILYID	FID;			resSet->serial(FID);
		uint32		Time;			resSet->serial(Time);

		g_mapSilencePlayers[FID] = Time;

		sipdebug("Silence Player : FID=%d, Time=%d", FID, Time);
	}

	SendSilenceListToFS(TServiceId(0));
}

void LoadSilenceList()
{
	uint32	now = (uint32)GetDBTimeSecond();

	MAKEQUERY_GetSilenceList(strQ, now);
	DBCaller->executeWithParam(strQ, DBCB_DBGetSilenceList, "");
}

void cb_M_GMCS_SILENCE_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	gmFID;
	T_FAMILYID	FID;
	uint32		RemainSeconds;
	_msgin.serial(gmFID, FID, RemainSeconds);

	uint32	Time;
	if (RemainSeconds == 0)
	{
		Time = 0;
		g_mapSilencePlayers.erase(FID);

		sipdebug("Delete Silence Player : FID=%d", FID);
	}
	else
	{
		Time = (uint32)GetDBTimeSecond() + RemainSeconds;
		g_mapSilencePlayers[FID] = Time;

		sipdebug("Add Silence Player : FID=%d, Time=%d", FID, Time);
	}

	MAKEQUERY_SetSilence(strQ, FID, Time);
	DBCaller->execute(strQ);

	CMessage msgOut(M_NT_SILENCE_SET);
	msgOut.serial(FID, Time);
	CUnifiedNetwork::getInstance()->send(FRONTEND_SHORT_NAME, msgOut);
}
