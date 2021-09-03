#include "zaixiankefu.h"
#include "../../common/Packet.h"
#include "player.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

struct ZAIXIANGM
{
	ZAIXIANGM(T_FAMILYID _gmid, T_FAMILYNAME _gmname, TServiceId _tsid) :
			ID(_gmid), Name(_gmname), FSID(_tsid), curClientNum(0), hisClientNum(0) {}

	void IncClient()	{ curClientNum++; }
	void DecClient()	
	{
		if (curClientNum > 0)
			curClientNum--; 
	}
	T_FAMILYID		ID;
	T_FAMILYNAME	Name;
	TServiceId		FSID;
	uint32			curClientNum;
	uint32			hisClientNum;
};
typedef map<T_FAMILYID, ZAIXIANGM> MAP_ZAIXIANGM;
static MAP_ZAIXIANGM	map_ZaiXianGM;

struct ZAIXIANCLIENT
{
	T_FAMILYID		FID;
	T_FAMILYNAME	Name;
	TServiceId		FSID;
	uint64			serviceID;
	T_FAMILYID		GMID;
};
typedef map<T_FAMILYID, ZAIXIANCLIENT> MAP_ZAIXIANCLIENT;
static MAP_ZAIXIANCLIENT	map_ZaiXianClient;

struct ZAIXIANHISTORY
{
	T_FAMILYID		FID;
	T_FAMILYID		GMID;
	TTime			lastTime;
};
typedef map<T_FAMILYID, ZAIXIANHISTORY> MAP_ZAIXIANHISTORY;
static MAP_ZAIXIANHISTORY	map_ZaiXianHistory;

static const ZAIXIANCLIENT*	GetZaiXianClient(T_FAMILYID _client)
{
	MAP_ZAIXIANCLIENT::iterator it = map_ZaiXianClient.find(_client);
	if (it == map_ZaiXianClient.end())
		return NULL;
	return &(it->second);
}
static const ZAIXIANGM*	GetZaiXianGM(T_FAMILYID _gm)
{
	MAP_ZAIXIANGM::iterator itGM = map_ZaiXianGM.find(_gm);
	if (itGM == map_ZaiXianGM.end())
		return NULL;
	return &(itGM->second);
}

static void AddZaiXianGM(T_FAMILYID _gmid, T_FAMILYNAME _gmname, TServiceId _tsid)
{
	MAP_ZAIXIANGM::iterator it = map_ZaiXianGM.find(_gmid);
	if (it != map_ZaiXianGM.end())
	{
		it->second.Name = _gmname;
		it->second.FSID = _tsid;
		sipwarning(L"Exchange zaixianGM! GMID=%d, Name=%s", _gmid, _gmname.c_str());
		return;
	}

	map_ZaiXianGM.insert(make_pair(_gmid, ZAIXIANGM(_gmid, _gmname, _tsid)));
	sipinfo(L"Register new zaixianGM : GMID=%d, Name=%s", _gmid, _gmname.c_str());
}

static void ProcZaiXianClientEnd(T_FAMILYID _fid)
{
	const ZAIXIANCLIENT*	pClient = GetZaiXianClient(_fid);
	if (pClient == NULL)
		return;

	ZAIXIANGM* pGM = const_cast<ZAIXIANGM*>(GetZaiXianGM(pClient->GMID));
	if (pGM != NULL)
	{
		T_FAMILYID gmid = pGM->ID;
		CMessage	mes(M_GMSC_ZAIXIAN_END);
		mes.serial(gmid, _fid);
		CUnifiedNetwork::getInstance()->send(pGM->FSID, mes);
		pGM->DecClient();

		sipdebug(L"Client is end ZaiXianKeFu : Family=%s, GMName=%s", pClient->Name.c_str(), pGM->Name.c_str());
	}
	map_ZaiXianClient.erase(_fid);
}

static void ProcZaiXianGMEnd(T_FAMILYID _gmid)
{
	const ZAIXIANGM* pGM = GetZaiXianGM(_gmid);
	if (pGM == NULL)
		return;

	vector<T_FAMILYID>	removeClient;
	MAP_ZAIXIANCLIENT::iterator itClient;
	for (itClient = map_ZaiXianClient.begin(); itClient != map_ZaiXianClient.end(); itClient++)
	{
		if (itClient->second.GMID != _gmid)
			continue;
		T_FAMILYID fid = itClient->first;
		CMessage mes(M_SC_ZAIXIAN_CONTROL);
		uint32	ncode = ZAIXIAN_EXITGM;
		mes.serial(fid, ncode);
		CUnifiedNetwork::getInstance()->send(itClient->second.FSID, mes);
		removeClient.push_back(fid);
	}

	vector<T_FAMILYID>::iterator itRemove;
	for (itRemove = removeClient.begin(); itRemove != removeClient.end(); itRemove++)
	{
		map_ZaiXianClient.erase((*itRemove));
	}
	sipdebug(L"ZaiXian GM is end : GMName=%s", pGM->Name.c_str());

	map_ZaiXianGM.erase(_gmid);
}

void ProcForZaixianForLogout(T_FAMILYID _fid)
{
	ProcZaiXianClientEnd(_fid);
	ProcZaiXianGMEnd(_fid);
}

void cb_M_NT_ZAIXIANGM_LOGON(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		GMID;
	T_FAMILYNAME	GMName;
	_msgin.serial(GMID, GMName);
	AddZaiXianGM(GMID, GMName, _tsid);
}

static void SendZaiXianResponse(T_FAMILYID fid, uint32 code, T_FAMILYNAME gname, SIPNET::TServiceId _tsid)
{
	CMessage	mes(M_SC_ZAIXIAN_RESPONSE);
	mes.serial(fid, code, gname);
	CUnifiedNetwork::getInstance()->send(_tsid, mes);
}

static void RefreshHisClientNumOfGM()
{
	MAP_ZAIXIANGM::iterator itGM;
	for (itGM = map_ZaiXianGM.begin(); itGM != map_ZaiXianGM.end(); itGM++)
	{
		itGM->second.hisClientNum = 0;
	}
	MAP_ZAIXIANHISTORY::iterator itHis;
	for (itHis = map_ZaiXianHistory.begin(); itHis != map_ZaiXianHistory.end(); itHis++)
	{
		T_FAMILYID GMID = itHis->second.GMID;
		ZAIXIANGM* pGM = const_cast<ZAIXIANGM*>(GetZaiXianGM(GMID));
		if (pGM)
		{
			pGM->hisClientNum ++;
		}
	}
}

static T_FAMILYID GetBestCandiGM()
{
	if (map_ZaiXianGM.size() == 1)
	{
		MAP_ZAIXIANGM::iterator front = map_ZaiXianGM.begin();
		return front->first;
	}

	uint32		nMinClientNum = INT_MAX;
	MAP_ZAIXIANGM::iterator itGM;
	for (itGM = map_ZaiXianGM.begin(); itGM != map_ZaiXianGM.end(); itGM++)
	{
		if (itGM->second.curClientNum < nMinClientNum)
			nMinClientNum = itGM->second.curClientNum;
	}
	if (nMinClientNum == INT_MAX)
		return 0;

	vector<T_FAMILYID>	candiGMS;
	for (itGM = map_ZaiXianGM.begin(); itGM != map_ZaiXianGM.end(); itGM++)
	{
		if (itGM->second.curClientNum == nMinClientNum)
			candiGMS.push_back(itGM->first);
	}
	if (candiGMS.size() == 1)
	{
		return candiGMS.front();
	}
	
	uint32		nMinHisNum = INT_MAX;
	T_FAMILYID	MinHisGM = 0;
	vector<T_FAMILYID>::iterator itTemp;
	for (itTemp = candiGMS.begin(); itTemp != candiGMS.end(); itTemp++)
	{
		T_FAMILYID curid = (*itTemp);
		const ZAIXIANGM* pGM = GetZaiXianGM(curid);
		if (pGM)
		{
			if (pGM->hisClientNum < nMinHisNum)
			{
				nMinHisNum = pGM->hisClientNum;
				MinHisGM = curid;
			}
		}
	}
	return MinHisGM;
}
static	uint64	lastServiceID = 0;
void cb_M_CS_ZAIXIAN_REQUEST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID fid;
	_msgin.serial(fid);
	Player *pPlayer = GetPlayer(fid);
	if (pPlayer == NULL)
	{
		sipwarning("There is no player's info : FID=%d", fid);
		return;
	}

	uint32			uCode;
	T_FAMILYNAME	gmName = L"";
	if (map_ZaiXianGM.size() == 0)
	{
		SendZaiXianResponse(fid, ZAIXIAN_NOGM, L"", _tsid);
		return;
	}
	const ZAIXIANCLIENT*	pClient = GetZaiXianClient(fid);
	if (pClient != NULL)
	{
		map_ZaiXianClient.erase(fid);
	}

	T_FAMILYID	candiGM1 = 0;
	TTime	tmCurrent = CTime::getLocalTime();
	// Check zaixian history
	MAP_ZAIXIANHISTORY::iterator itHis = map_ZaiXianHistory.find(fid);
	if (itHis != map_ZaiXianHistory.end())
	{
		candiGM1 = itHis->second.GMID;
	}

	T_FAMILYID	candiGM2;
	const ZAIXIANGM* pGM1 = GetZaiXianGM(candiGM1);
	if (pGM1 != NULL)
		candiGM2 = candiGM1;
	else
	{
		candiGM2 = GetBestCandiGM();
	}
	
	ZAIXIANGM* pGM = const_cast<ZAIXIANGM*>(GetZaiXianGM(candiGM2));
	if (pGM == NULL)
	{
		SendZaiXianResponse(fid, ZAIXIAN_NOGM, L"", _tsid);
		return;
	}
	// add history
	if (itHis != map_ZaiXianHistory.end())
	{
		itHis->second.GMID = candiGM2;
		itHis->second.lastTime = tmCurrent;
	}
	else
	{
		ZAIXIANHISTORY newhis;
		newhis.FID = fid;
		newhis.GMID = candiGM2;
		newhis.lastTime = tmCurrent;
		map_ZaiXianHistory.insert(make_pair(fid, newhis));
	}
	RefreshHisClientNumOfGM();

	ZAIXIANCLIENT	newclient;
	newclient.FID = fid;
	newclient.GMID = candiGM2;
	newclient.FSID = _tsid;
	newclient.Name = pPlayer->m_FName;
	uint64 candiserviceID = CTime::getCurrentTime()*1000;
	if (lastServiceID < candiserviceID)
		lastServiceID = candiserviceID;
	else
		lastServiceID ++;
	newclient.serviceID = lastServiceID;

	uCode = ZAIXIAN_NEWOK;
	gmName = pGM->Name;
	map_ZaiXianClient.insert(make_pair(fid, newclient));
	sipinfo(L"New ZaiXianKefu client : FamilyName=%s, GMName=%s", pPlayer->m_FName.c_str(), gmName.c_str());

	SendZaiXianResponse(fid, ZAIXIAN_NEWOK, gmName, _tsid);
	
	pGM->IncClient();
}

void cb_M_CS_ZAIXIAN_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID fid;
	_msgin.serial(fid);
	ProcZaiXianClientEnd(fid);
}

void cb_M_CS_ZAIXIAN_ASK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID fid;
	T_COMMON_CONTENT	text;
	_msgin.serial(fid, text);

	const ZAIXIANCLIENT*	pClient = GetZaiXianClient(fid);
	if (pClient == NULL)
		return;

	const ZAIXIANGM* pGM = GetZaiXianGM(pClient->GMID);
	if (pGM == NULL)
		return;

	T_FAMILYID		gmid = pGM->ID;
	T_FAMILYNAME	fname = pClient->Name;
	CMessage mes(M_GMSC_ZAIXIAN_ASK);
	mes.serial(gmid, fid, fname, text);

	CUnifiedNetwork::getInstance()->send(pGM->FSID, mes);
}

void cb_M_GMCS_ZAIXIAN_ANSWER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	gmid;
	T_FAMILYID	fid;
	T_COMMON_CONTENT		text;
	_msgin.serial(gmid, fid, text);

	const ZAIXIANGM* pGM = GetZaiXianGM(gmid);
	if (pGM == NULL)
		return;

	const ZAIXIANCLIENT*	pClient = GetZaiXianClient(fid);
	if (pClient == NULL)
		return;

	if (pClient->GMID != gmid)
		return;

	CMessage mes(M_SC_ZAIXIAN_ANSWER);
	mes.serial(fid, text);
	CUnifiedNetwork::getInstance()->send(pClient->FSID, mes);
}

void cb_M_GMCS_ZAIXIAN_CONTROL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	gmid;
	T_FAMILYID	fid;
	uint32		code;
	_msgin.serial(gmid, fid, code);

	const ZAIXIANGM* pGM = GetZaiXianGM(gmid);
	if (pGM == NULL)
		return;

	const ZAIXIANCLIENT*	pClient = GetZaiXianClient(fid);
	if (pClient == NULL)
		return;

	if (pClient->GMID != gmid)
		return;

	CMessage mes(M_SC_ZAIXIAN_CONTROL);
	mes.serial(fid, code);
	CUnifiedNetwork::getInstance()->send(pClient->FSID, mes);
}
