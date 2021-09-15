 
#include "misc/types_sip.h"

#include <stdio.h>
#include <ctype.h>
#include <vector>
#include <map>
#include <string>

#include "misc/debug.h"
#include "misc/config_file.h"
#include "misc/displayer.h"
#include "misc/command.h"
#include "misc/log.h"
#include "misc/window_displayer.h"
#include "misc/path.h"
#include "misc/variable.h"
#include "misc/DBCaller.h"

#define NOMINMAX

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#endif

#include "../../common/common.h"
#include "../../common/Packet.h"
#include "../Common/QuerySetForMain.h"

#include "net/login_server.h"
#include "net/DelayServer.h"
#include "net/IProtocolServer.h"

#include "UpdateServer.h"
#include "start_service.h"
using namespace std;
using namespace SIPBASE;
using namespace SIPNET;


//static CCallbackServer	* _UpdateServer = NULL;
//static IProtocolServer	* _UpdateServer = NULL;

typedef std::map<ucstring, _entryAppLastVersion> TNEWVERSION;
TNEWVERSION		_newVersions;

typedef std::map<uint32, CPatchServerInfo> TUpdateServer;
TUpdateServer		_ServInfoVector;

typedef std::multimap<ucstring, _entryUpdateAppsInfo>	TPatchUpdateServerMap;// <Key, No>
typedef std::multimap<ucstring, _entryUpdateAppsInfo>::iterator	TPatchUpdateServerMapIT;// <Key, No>
TPatchUpdateServerMap		_PatchServerAry;
TPatchUpdateServerMap		_HighstPatchServerAry;

typedef	std::list<IPMnt>	LSTIPMnt;
LSTIPMnt					_lstIPMnt;

uint32	GetNetworkPart(const char* strIP)
{
	LSTIPMnt::iterator it;
	for (it = _lstIPMnt.begin(); it != _lstIPMnt.end(); it++)
	{
		const char *strStartIP = it->strIP.c_str();
		uint32		nNum = it->uNumber;
		uint32		nMnt = it->uMnt;

		if( CIPAddress::IsInRange(strIP, strStartIP, nNum) )
			return nMnt;
	}
	return NETWORK_UNKNOWN;
}

/**************************************************************************/
/*					Callbacks From Launcher								  */
/**************************************************************************/


static uint	SearchMinUserUpdatePatchServer(TPatchUpdateServerMap multi_map, ucstring ucKey, uint32 nNetworkPart)
{
	TPatchUpdateServerMap::iterator	start, end;
	start	= multi_map.lower_bound(ucKey);
	end		= multi_map.upper_bound(ucKey);
	// search the best Update Server
	_entryUpdateServer	info;
	uint	nUpdateServerNo = start->second.SrvId;
	uint	uMinUsers = _ServInfoVector[nUpdateServerNo].uOnlineUsers;

	int		nFitServerNo = -1;
	uint	uFitMinUsers = 0xFFFFFFFF;
	if (_ServInfoVector[start->second.SrvId].uNetworkPart == nNetworkPart)
	{
		nFitServerNo = (int)(start->second.SrvId);
		uFitMinUsers = _ServInfoVector[start->second.SrvId].uOnlineUsers;
	}

	while(start != end)
	{
		uint	nUsers = _ServInfoVector[start->second.SrvId].uOnlineUsers;
		if ( uMinUsers > nUsers )
		{
			uMinUsers = nUsers;
			nUpdateServerNo = start->second.SrvId;
		}
		if ( _ServInfoVector[start->second.SrvId].uNetworkPart == nNetworkPart &&
			 uFitMinUsers > nUsers)
		{
			uFitMinUsers = nUsers;
			nFitServerNo = (int)(start->second.SrvId);
		}

		*start++;
	}
	if (nFitServerNo != -1)
		nUpdateServerNo = (uint)nFitServerNo;

	_ServInfoVector[nUpdateServerNo].uOnlineUsers++;

	return	nUpdateServerNo;
}

void cbLauncherConnection( TTcpSockId from, void *arg )
{
	// commit
}

void cbLanucherDisconnection( TTcpSockId from, void *arg )
{
	// commit
}

static bool	FindNewPatch(ucstring ucProductName, std::string srcVersionNo, std::string dstVersionNo, uint &nServerNo, std::string &sPath, uint32 nNetworkPart)
{
	// Send latest version
    T_ERR	errNo = CUpdateServer::chooseNewUpdatePatchServer(ucProductName, srcVersionNo, &nServerNo, nNetworkPart);
	if ( errNo != ERR_NOERR )
	{
		sipdebug(L"There is no UpdateServer for Product:<%s>, Version:<%S>, So NoReply<UPDATE>", ucProductName.c_str(), srcVersionNo.c_str());
		return	false;
	}

	ucstring		ucKey = ucProductName + "/" + srcVersionNo;
	if ( _HighstPatchServerAry.count(ucKey) == 0 )
	{
		sipdebug(L"There is no patch Url for Product:<%s>, Version:<%S>, So NoReply<UPDATE>", ucProductName.c_str(), srcVersionNo.c_str());
		return	false;
	}

	bool	bFind = false;
	pair<TPatchUpdateServerMapIT, TPatchUpdateServerMapIT> range = _HighstPatchServerAry.equal_range(ucKey);
	for ( TPatchUpdateServerMapIT it = range.first; it != range.second; it ++ )
	{
		_entryUpdateAppsInfo	info = (*it).second;
		uint32	idApp = _newVersions[ucProductName].idApp;
		if ( info.SrvId == nServerNo && info.AppId == idApp && info.dstVersion == dstVersionNo)
		{
            sPath = info.Path;
			bFind = true;
			break;
		}
	}

	return	bFind;
}

// in	: productName, srcVersionNo
// out	: dstVersionNo, serverNo, serverPatchPath
static bool	FindPatch(ucstring ucProductName, std::string srcVersionNo, std::string& dstVersionNo, uint &nServerNo, std::string &sPath, uint32 nNetworkPart)
{
	dstVersionNo = "";
	T_ERR	errNo = CUpdateServer::chooseUpdatePatchServer(ucProductName, srcVersionNo, &nServerNo, nNetworkPart);
	if ( errNo != ERR_NOERR )
	{
		sipdebug(L"There is no UpdateServer for Product:<%s>, Version:<%S>, So NoReply<UPDATE>", ucProductName.c_str(), srcVersionNo.c_str());
		return	false;
	}
	ucstring		ucKey = ucProductName + "/" + srcVersionNo;
	if ( _PatchServerAry.count(ucKey) == 0 )
	{
		sipdebug(L"There is no patch Url for Product:<%s>, Version:<%S>, So NoReply<UPDATE>", ucProductName.c_str(), srcVersionNo.c_str());
		return	false;
	}

	bool	bFind = false;
    pair<TPatchUpdateServerMapIT, TPatchUpdateServerMapIT> range = _PatchServerAry.equal_range(ucKey);
	for ( TPatchUpdateServerMapIT it = range.first; it != range.second; it ++ )
	{
		_entryUpdateAppsInfo	info = (*it).second;
		uint32	idApp = _newVersions[ucProductName].idApp;
		if ( info.SrvId == nServerNo && info.AppId == idApp )
		{
			sPath = info.Path;
			dstVersionNo = info.dstVersion; 
			bFind = true;
			break;
		}
	}

	return bFind;
}
static ucstring MakeCorrectProductName(ucstring ucOldName)
{
	const int nDefaultLangCode = 0;
	static ucstring LangCode[] = { L"-zh-cn", L"-zh-tw", L"-ko-kr", L"" };

	ucstring ucNewName = ucOldName;
	wchar_t* pNewName = (wchar_t*)(ucNewName.c_str());
	int nLen = static_cast<int>(wcslen(pNewName));

	int nHadLangCode = -1;
	int nCurLang = 0;
	while (LangCode[nCurLang] != L"")
	{
		wchar_t* pLangCode = (wchar_t*)(LangCode[nCurLang].c_str());
		int nLangCodeLen = static_cast<int>(wcslen(pLangCode));
		if ( nLen > nLangCodeLen)
		{
			wchar_t* pLastS = pNewName + nLen - nLangCodeLen;
			if (wcscmp(pLastS, pLangCode) == 0)
			{
				nHadLangCode = nCurLang;
				break;
			}
		}
		nCurLang ++;
	}
	if (nHadLangCode == -1)
		ucNewName += LangCode[nDefaultLangCode];
	return ucNewName;
}

bool MakeUpdateInfo( CMessage& msgin, uint32 nNetworkPart, CMessage &msgout )
{
	ucstring	ucProductName;	msgin.serial(ucProductName);
	string		sVersionNo;		msgin.serial(sVersionNo);
	
	// ucstring	ucProductNameLower= wcslwr((wchar_t *)ucProductName.c_str());
	ucstring ucProductNameLower(ucProductName.c_str());
	std::transform(ucProductNameLower.begin(), ucProductNameLower.end(), ucProductNameLower.begin(), ::tolower);

	ucstring	uckeyProductName = MakeCorrectProductName(ucProductNameLower);
	std::string	sNewVersionNo	= _newVersions[uckeyProductName].sLastVersion;
	if ( sVersionNo == sNewVersionNo )
	{
		std::string		strNull = "";
		uint32			portNull = 0;

		msgout.serial(sNewVersionNo);
		msgout.serial(sNewVersionNo);
		msgout.serial(strNull);
		msgout.serial(portNull);
		msgout.serial(strNull);
		msgout.serial(strNull);
		msgout.serial(strNull);
		sipinfo("<UPDATE> Success! Produce:<%S>, Version:<%s> -> lastversion:<%s>, serverURL:<NULL>, serverPort:<NULL>, Path:<NULL>, userID:<NULL>, password:<NULL>",
			uckeyProductName.c_str(), sVersionNo.c_str(), sNewVersionNo.c_str());

		return true;
	}

	// Send latest version
	uint			newServerNo;
	std::string		sPath, sDstVersionNo = "";
    if ( !FindNewPatch(uckeyProductName, sVersionNo, sNewVersionNo, newServerNo, sPath, nNetworkPart) )
		if ( !FindPatch(uckeyProductName, sVersionNo, sDstVersionNo, newServerNo, sPath, nNetworkPart) )
			if ( !FindNewPatch(uckeyProductName, sNewVersionNo, sNewVersionNo, newServerNo, sPath, nNetworkPart) )
			{
				sipwarning("There is no Patch Path for Product:<%S>, Version:<%s>, So NoReply<UPDATE>", uckeyProductName.c_str(), sVersionNo.c_str());
				return false;
			}

	CPatchServerInfo	newServer		= _ServInfoVector[newServerNo];

    msgout.serial(sNewVersionNo);
	if ( sDstVersionNo == "" )
		sDstVersionNo = sNewVersionNo;
    msgout.serial(sDstVersionNo);
    msgout.serial(newServer.ServerInfo.sUri);
	msgout.serial(newServer.ServerInfo.port);
	msgout.serial(sPath);
	msgout.serial(newServer.ServerInfo.sUserID);
	msgout.serial(newServer.ServerInfo.sPassword);
	sipinfo(L"<UPDATE> Success! Produce:<%s>, Version:<%S> -> lastVersion<%S>, dstversion:<%S>, serverURL:<%S>, serverPort:<%u>, Path:<%S>, userID:<%S>, password:<%S>",
		uckeyProductName.c_str(), sVersionNo.c_str(), sNewVersionNo.c_str(), sDstVersionNo.c_str(), newServer.ServerInfo.sUri.c_str(), newServer.ServerInfo.port, 
		sPath.c_str(), newServer.ServerInfo.sUserID.c_str(), newServer.ServerInfo.sPassword.c_str());

	return true;
}

// Mode : TCP
// Packet : UPDATE
// Launcher -> SS
void cbUpdate_TCP( CMessage& msgin, TTcpSockId from, CCallbackNetBase& clientcb )
{
	string clientip = clientcb.hostAddress(from).ipAddress();
	uint32 nNetworkPart = GetNetworkPart(clientip.c_str());

	CMessage	msgout(M_SS_UPDATE);
	if (MakeUpdateInfo( msgin, nNetworkPart, msgout ))
		clientcb.send(msgout, from);

/*	ucstring	ucProductName;	msgin.serial(ucProductName);
	string		sVersionNo;		msgin.serial(sVersionNo);
	
	ucstring	ucProductNameLower= wcslwr((wchar_t *)ucProductName.c_str());
	ucstring	uckeyProductName = MakeCorrectProductName(ucProductNameLower);
	std::string	sNewVersionNo	= _newVersions[uckeyProductName].sLastVersion;
	if ( sVersionNo == sNewVersionNo )
	{
		std::string		strNull = "";
		uint32			portNull = 0;

		CMessage	msgout(M_SS_UPDATE);
		msgout.serial(sNewVersionNo);
		msgout.serial(sNewVersionNo);
		msgout.serial(strNull);
		msgout.serial(portNull);
		msgout.serial(strNull);
		msgout.serial(strNull);
		msgout.serial(strNull);
		clientcb.send(msgout, from);
		sipinfo(L"<UPDATE> Success! Produce:<%s>, Version:<%S> -> lastversion:<%S>, serverURL:<NULL>, serverPort:<NULL>, Path:<NULL>, userID:<NULL>, password:<NULL>",
		uckeyProductName.c_str(), sVersionNo.c_str(), sNewVersionNo.c_str());

		return;
	}
	string clientip = clientcb.hostAddress(from).ipAddress();
	uint32 nNetworkPart = GetNetworkPart(clientip.c_str());

	// Send latest version
	uint			newServerNo;
	std::string		sPath, sDstVersionNo = "";
    if ( !FindNewPatch(uckeyProductName, sVersionNo, sNewVersionNo, newServerNo, sPath, nNetworkPart) )
		if ( !FindPatch(uckeyProductName, sVersionNo, sDstVersionNo, newServerNo, sPath, nNetworkPart) )
			if ( !FindNewPatch(uckeyProductName, sNewVersionNo, sNewVersionNo, newServerNo, sPath, nNetworkPart) )
			{
				sipwarning(L"There is no Patch Path for Product:<%s>, Version:<%S>, So NoReply<UPDATE>", uckeyProductName.c_str(), sVersionNo.c_str());
				return;
			}

	CPatchServerInfo	newServer		= _ServInfoVector[newServerNo];

    CMessage	msgout(M_SS_UPDATE);
    msgout.serial(sNewVersionNo);
	if ( sDstVersionNo == "" )
		sDstVersionNo = sNewVersionNo;
    msgout.serial(sDstVersionNo);
    msgout.serial(newServer.ServerInfo.sUri);
	msgout.serial(newServer.ServerInfo.port);
	msgout.serial(sPath);
	msgout.serial(newServer.ServerInfo.sUserID);
	msgout.serial(newServer.ServerInfo.sPassword);
	sipinfo(L"<UPDATE> Success! Produce:<%s>, Version:<%S> -> lastVersion<%S>, dstversion:<%S>, serverURL:<%S>, serverPort:<%u>, Path:<%S>, userID:<%S>, password:<%S>",
		uckeyProductName.c_str(), sVersionNo.c_str(), sNewVersionNo.c_str(), sDstVersionNo.c_str(), newServer.ServerInfo.sUri.c_str(), newServer.ServerInfo.port, 
		sPath.c_str(), newServer.ServerInfo.sUserID.c_str(), newServer.ServerInfo.sPassword.c_str());

	clientcb.send(msgout, from);*/
}

// UDP Mode
// Launcher -> SS
void cbUpdate_UDP(SIPNET::CMessage &msgin, SIPNET::CInetAddress& peeraddr,	SIPNET::CUDPPeerServer *peer)
{
	string clientip = peeraddr.ipAddress();
	uint32 nNetworkPart = GetNetworkPart(clientip.c_str());

	CMessage	msgout(M_SS_UPDATE);
	if (MakeUpdateInfo( msgin, nNetworkPart, msgout ))
		peer->sendTo(msgout, peeraddr);

/*	ucstring	ucProductName;	msgin.serial(ucProductName);
	string		sVersionNo;		msgin.serial(sVersionNo);
	
	ucstring	ucProductNameLower= wcslwr((wchar_t *)ucProductName.c_str());
	ucstring	uckeyProductName = MakeCorrectProductName(ucProductNameLower);
	std::string	sNewVersionNo	= _newVersions[uckeyProductName].sLastVersion;
	if ( sVersionNo == sNewVersionNo )
	{
		std::string		strNull = "";
		uint32			portNull = 0;

		CMessage	msgout(M_SS_UPDATE);
		msgout.serial(sNewVersionNo);
		msgout.serial(sNewVersionNo);
		msgout.serial(strNull);
		msgout.serial(portNull);
		msgout.serial(strNull);
		msgout.serial(strNull);
		msgout.serial(strNull);
		peer->sendTo(msgout, peeraddr);
		sipinfo(L"<UPDATE> Success! Produce:<%s>, Version:<%S> -> lastversion:<%S>, serverURL:<NULL>, serverPort:<NULL>, Path:<NULL>, userID:<NULL>, password:<NULL>",
		uckeyProductName.c_str(), sVersionNo.c_str(), sNewVersionNo.c_str());

		return;
	}
	string clientip = peeraddr.ipAddress();
	uint32 nNetworkPart = GetNetworkPart(clientip.c_str());

	// Send latest version
	uint			newServerNo;
	std::string		sPath, sDstVersionNo = "";
    if ( !FindNewPatch(uckeyProductName, sVersionNo, sNewVersionNo, newServerNo, sPath, nNetworkPart) )
		if ( !FindPatch(uckeyProductName, sVersionNo, sDstVersionNo, newServerNo, sPath, nNetworkPart) )
			if ( !FindNewPatch(uckeyProductName, sNewVersionNo, sNewVersionNo, newServerNo, sPath, nNetworkPart) )
			{
				sipwarning(L"There is no Patch Path for Product:<%s>, Version:<%S>, So NoReply<UPDATE>", uckeyProductName.c_str(), sVersionNo.c_str());
				return;
			}

	CPatchServerInfo	newServer		= _ServInfoVector[newServerNo];

    CMessage	msgout(M_SS_UPDATE);
    msgout.serial(sNewVersionNo);
	if ( sDstVersionNo == "" )
		sDstVersionNo = sNewVersionNo;
    msgout.serial(sDstVersionNo);
    msgout.serial(newServer.ServerInfo.sUri);
	msgout.serial(newServer.ServerInfo.port);
	msgout.serial(sPath);
	msgout.serial(newServer.ServerInfo.sUserID);
	msgout.serial(newServer.ServerInfo.sPassword);
	sipinfo(L"<UPDATE> Success! Produce:<%s>, Version:<%S> -> lastVersion<%S>, dstversion:<%S>, serverURL:<%S>, serverPort:<%u>, Path:<%S>, userID:<%S>, password:<%S>",
		uckeyProductName.c_str(), sVersionNo.c_str(), sNewVersionNo.c_str(), sDstVersionNo.c_str(), newServer.ServerInfo.sUri.c_str(), newServer.ServerInfo.port, 
		sPath.c_str(), newServer.ServerInfo.sUserID.c_str(), newServer.ServerInfo.sPassword.c_str());

	peer->sendTo(msgout, peeraddr);*/
}

// UDP Mode
// Launcher -> SS
/*void cbQueryFtpUri(SIPNET::CMessage &msgin, SIPNET::CInetAddress& peeraddr,	SIPNET::CUDPPeerServer *peer)
{
	ucstring	ucProductName;	msgin.serial(ucProductName);
	string		sVersionNo;		msgin.serial(sVersionNo);

	ucstring	uckeyProductName= wcslwr((wchar_t *)ucProductName.c_str());
	std::string	sNewVersionNo	= _newVersions[uckeyProductName].sLastVersion;
	if ( sVersionNo == sNewVersionNo )
	{
		std::string		strNull = "";
		uint32			portNull = 0;

		CMessage	msgout(M_FTPURI);
		msgout.serial(sNewVersionNo);
		msgout.serial(sNewVersionNo);
		msgout.serial(strNull);
		msgout.serial(portNull);
		msgout.serial(strNull);
		msgout.serial(strNull);
		msgout.serial(strNull);
		peer->sendTo(msgout, peeraddr);
		sipinfo(L"<FTPURI> Success! Produce:<%s>, Version:<%S> -> lastversion:<%S>, serverURL:<NULL>, serverPort:<NULL>, Path:<NULL>, userID:<NULL>, password:<NULL>",
			ucProductName.c_str(), sVersionNo.c_str(), sNewVersionNo.c_str());

		return;
	}
	string clientip = peeraddr.ipAddress();
	uint32 nNetworkPart = GetNetworkPart(clientip.c_str());

	// Send latest version
	uint			newServerNo;
	std::string		sPath, sDstVersionNo = "";
	if ( !FindNewPatch(uckeyProductName, sVersionNo, sNewVersionNo, newServerNo, sPath, nNetworkPart) )
		if ( !FindPatch(uckeyProductName, sVersionNo, sDstVersionNo, newServerNo, sPath, nNetworkPart) )
			if ( !FindNewPatch(uckeyProductName, sNewVersionNo, sNewVersionNo, newServerNo, sPath, nNetworkPart) )
			{
				sipwarning(L"There is no Patch Path for Product:<%s>, Version:<%S>, So NoReply<FTPURI>", ucProductName.c_str(), sVersionNo.c_str());
				return;
			}

			CPatchServerInfo	newServer		= _ServInfoVector[newServerNo];

			CMessage	msgout(M_FTPURI);
			msgout.serial(sNewVersionNo);
			if ( sDstVersionNo == "" )
				sDstVersionNo = sNewVersionNo;
			msgout.serial(sDstVersionNo);
			msgout.serial(newServer.ServerInfo.sUri);
			msgout.serial(newServer.ServerInfo.port);
			msgout.serial(sPath);
			msgout.serial(newServer.ServerInfo.sUserID);
			msgout.serial(newServer.ServerInfo.sPassword);
			sipinfo(L"<FTPURI> Success! Produce:<%s>, Version:<%S> -> lastVersion<%S>, dstversion:<%S>, serverURL:<%S>, serverPort:<%u>, Path:<%S>, userID:<%S>, password:<%S>",
				ucProductName.c_str(), sVersionNo.c_str(), sNewVersionNo.c_str(), sDstVersionNo.c_str(), newServer.ServerInfo.sUri.c_str(), newServer.ServerInfo.port, 
				sPath.c_str(), newServer.ServerInfo.sUserID.c_str(), newServer.ServerInfo.sPassword.c_str());

			peer->sendTo(msgout, peeraddr);

			//if (sVersionNo == "")
			//{
			//std::string		strNull = "";
			//uint32			portNull = 0;
			//msgout.serial(strNull);
			//msgout.serial(portNull);
			//msgout.serial(strNull);
			//msgout.serial(strNull);
			//msgout.serial(strNull);
			//clientcb.send(msgout, from);
			//return;
			//}

			//// if launcher, no meaning, but only client
			//errNo = CUpdateServer::chooseUpdatePatchServer(ucProductName, sVersionNo, &serverNo);
			//if ( errNo != ERR_NOERR )
			//{
			//sipwarning("UPDATE Err: There is no UPDATE : product %S version Number %s", ucProductName.c_str(), sVersionNo.c_str());
			//return;
			//}
			//patchServer	= _ServInfoVector[serverNo];

			//// if Launcher, Send null string automatically
			//msgout.serial(patchServer.ServerInfo.sUri);
			//msgout.serial(patchServer.ServerInfo.port);
			//ucKey = ucProductName + "/" + sVersionNo;
			//range = _PatchServerAry.equal_range(ucKey);
			//for ( TPatchUpdateServerMapIT it = range.first; it != range.second; it ++ )
			//{
			//_entryUpdateAppsInfo	info = (*it).second;
			//uint32	idApp = _newVersions[ucProductName].idApp;
			//if ( info.SrvId == serverNo && info.AppId == idApp && info.Version == sVersionNo )
			//{
			//sPath = info.Path;
			//break;
			//}
			//}
			//msgout.serial(sPath);
			//msgout.serial(patchServer.ServerInfo.sUserID);
			//msgout.serial(patchServer.ServerInfo.sPassword);
}*/

TUDPCallbackItem	LauncherCallbackAry_UDP[] =
{
//	{ M_FTPURI, cbQueryFtpUri },
	{ M_SS_UPDATE, cbUpdate_UDP },
};

TCallbackItem	LauncherCallbackAry_TCP[] =
{
	//	{ M_FTPURI, cbQueryFtpUri },
	{ M_SS_UPDATE, cbUpdate_TCP },
};

BOOL CUpdateServer::m_bInit = false;

CUpdateServer::CUpdateServer()
{
}

CUpdateServer::~CUpdateServer()
{
}

BOOL	CUpdateServer::init(CCallbackServer * server)
{
	// \C0̹\CC UDP\C2ʿ\A1\BC\AD \C3ʱ\E2ȭ\B5ǿ\A9\C0\D6\C0\B8\B9Ƿ\CE \BF\A9\B1⼭\B4\C2 \B4ٽ\C3 \C3ʱ\E2ȭ\C7\CF\C1\F6 \BEʴ´\D9.
//	if ( !LoadDBUpdateInfo())
//		return	false;

//	LoadIPMnt();
//	m_bInit = true;
	//_UpdateServer = server;
	server->addCallbackArray(LauncherCallbackAry_TCP, sizeof(LauncherCallbackAry_TCP)/sizeof(LauncherCallbackAry_TCP[0]));
	//_UpdateServer->setConnectionCallback(cbLauncherConnection, NULL);
	//_UpdateServer->setDisconnectionCallback(cbLanucherDisconnection, NULL);

	sipinfo("UpdateServer initialization finished completely - TCP");    

	return true;
}

BOOL	CUpdateServer::init(CDelayServer * server)
{
	if ( !LoadDBUpdateInfo())
		return	false;

	LoadIPMnt();
	m_bInit = true;
	//_UpdateServer = server;
	server->addCallbackArray(LauncherCallbackAry_UDP, sizeof(LauncherCallbackAry_UDP)/sizeof(LauncherCallbackAry_UDP[0]));
	//_UpdateServer->setConnectionCallback(cbLauncherConnection, NULL);
	//_UpdateServer->setDisconnectionCallback(cbLanucherDisconnection, NULL);

	sipinfo("UpdateServer initialization finished completely - UDP");    

	return true;
}

/*void	CUpdateServer::update()
{
	if ( m_bInit )
	{
		_UpdateServer->update();
	}
}

void	CUpdateServer::release()
{
	if ( _UpdateServer )
	{
		_UpdateServer = NULL;
	}
}*/

static void	DBCB_LoadUpdateInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if ( !nQueryResult )
		return;

	std::string strNull = "";
	TPatchUpdateServerMap		aryPatch;
	uint32	uRowNum;		resSet->serial(uRowNum);
	if ( uRowNum == 0 )
		sipwarning("There is no Update Info in Server");

	for ( uint32 i = 0; i < uRowNum; i ++ )
	{
		uint32	idServer;		resSet->serial(idServer);
		uint32	idApp;			resSet->serial(idApp);
		std::string	sSrcVer;	resSet->serial(sSrcVer);
		std::string	sDstVer;	resSet->serial(sDstVer);
		std::string	Path;		resSet->serial(Path);
		ucstring	ucAppName;	resSet->serial(ucAppName);
		std::string	sLastVersion;	resSet->serial(sLastVersion);

		// ucstring	ucnameAppLower= wcslwr((wchar_t *)ucAppName.c_str());
		ucstring ucnameAppLower(ucAppName.c_str());
		std::transform(ucnameAppLower.begin(), ucnameAppLower.end(), ucnameAppLower.begin(), ::tolower);

		ucstring	ucCorrectNameApp = MakeCorrectProductName(ucnameAppLower);

		if ( sSrcVer == strNull ||
			sDstVer == strNull ||
			!wcscmp(ucCorrectNameApp.c_str(), L"") ||
			sLastVersion == strNull )
			siperror(L"Update data string is null");

		_entryUpdateAppsInfo	info(idServer, idApp, sSrcVer, sDstVer, Path, ucCorrectNameApp, sLastVersion);
		ucstring	ucKey = ucCorrectNameApp + "/" + sSrcVer;
		std::transform(ucKey.begin(), ucKey.end(), ucKey.begin(), ::tolower);
		if ( sDstVer == sLastVersion )
		{
			// _HighstPatchServerAry.insert(std::make_pair(wcslwr((wchar_t *)ucKey.c_str()), info));
			// aryPatch.insert(std::make_pair(wcslwr((wchar_t *)ucKey.c_str()), info));
			_HighstPatchServerAry.insert(std::make_pair(ucKey.c_str(), info));
			aryPatch.insert(std::make_pair(ucKey.c_str(), info));
		}
		else
		{
			if ( sSrcVer != sLastVersion && sSrcVer != sDstVer )
			{
				// _PatchServerAry.insert(std::make_pair(wcslwr((wchar_t *)ucKey.c_str()), info));
				// aryPatch.insert(std::make_pair(wcslwr((wchar_t *)ucKey.c_str()), info));
				_PatchServerAry.insert(std::make_pair(ucKey.c_str(), info));
				aryPatch.insert(std::make_pair(ucKey.c_str(), info));
			}
		}
	}
	sipdebug("Load UpdateApplicationVersion, rownum : %u", uRowNum);

	// Check existance of new version
	sipdebug("Check DB existance of most new version patch");
	if (_HighstPatchServerAry.size() == 0)
		siperror("No existance of most new version patch");

	TPatchUpdateServerMapIT	it;
	it = _HighstPatchServerAry.begin();
	_entryUpdateAppsInfo	info= (*it).second;
	ucstring	key = info.AppName + "/" + info.dstVersion;
	// key = wcslwr((wchar_t *) key.c_str());
	std::transform(key.begin(), key.end(), key.begin(), ::tolower);
	it = _HighstPatchServerAry.find(key);
	if ( it == _HighstPatchServerAry.end() )
		siperror(L"No existance of most new version patch, key : %s", key.c_str());

	// Check DB Data Validation
	sipdebug("Check DB Data Overlapping and Validation : UpdateApplicationVersion ");
	TPatchUpdateServerMapIT	begin	= aryPatch.begin();
	TPatchUpdateServerMapIT	end		= aryPatch.end();
	for ( TPatchUpdateServerMapIT it = begin; it != end; it ++ )
	{
		ucstring				key	= (*it).first;
		_entryUpdateAppsInfo	info= (*it).second;

		std::string	dstVersion, lastVersion;
		dstVersion = strNull;

		// check overlapping
		int		nCount	= static_cast<int>(aryPatch.count(key));
		if ( nCount != 1 )
		{
			pair<TPatchUpdateServerMapIT, TPatchUpdateServerMapIT> range = aryPatch.equal_range(key);
			for ( TPatchUpdateServerMapIT it = range.first; it != range.second; it ++ )
			{
				_entryUpdateAppsInfo	info = (*it).second;
				if ( strcmp(info.dstVersion.c_str(), dstVersion.c_str()) )	// dstVersion0 != dstVersion1
				{
					if ( !strcmp(dstVersion.c_str(), strNull.c_str()) )		// dstVersion1 = ""
					{
						dstVersion = info.dstVersion;
						continue;
					}

					siperror(L"Overlapped Various way to dstversion from srcversion <%s> ", key.c_str());
				}
			}
		}

		// check valid : there is way to last version
		bool	bWayToLastVersion = true;
		_entryUpdateAppsInfo	updateInfo = (*it).second;
		while(true)
		{
			dstVersion	= updateInfo.dstVersion;
			lastVersion	= updateInfo.lastVersion;
			if ( dstVersion == lastVersion )
				break;

			ucstring	ucKey = updateInfo.AppName + "/" + dstVersion;
			std::transform(ucKey.begin(), ucKey.end(), ucKey.begin(), ::tolower);
			TPatchUpdateServerMapIT it = aryPatch.find(ucKey.c_str());
			if ( it == aryPatch.end() )
			{
				sipdebug(L"Invalid Data: No way to lastversion, startsrcVersion <%s>, cursrcVersion <%s>", key.c_str(), ucKey.c_str());
				bWayToLastVersion = false;
				break;
			}

			updateInfo = (*it).second;
		}

		// if there is no way to last version
		if ( ! bWayToLastVersion )
		{
			pair<TPatchUpdateServerMapIT, TPatchUpdateServerMapIT> range = _PatchServerAry.equal_range(key);
			for ( TPatchUpdateServerMapIT it = range.first; it != range.second; it ++ )
			{
				_entryUpdateAppsInfo	&infoOther = (*it).second;
				if ( info.SrvId		== infoOther.SrvId && 
					info.AppId		== infoOther.AppId &&
					info.srcVersion	== infoOther.srcVersion &&
					info.dstVersion	== infoOther.dstVersion &&
					info.Path		== infoOther.Path &&
					info.AppName	== infoOther.AppName &&
					info.lastVersion== infoOther.lastVersion )
				{
					//infoOther.dstVersion = "";
					_PatchServerAry.erase(it);
					break;
				}
			}
		}
		if ( dstVersion == lastVersion )
			continue;
	}
	aryPatch.clear();
}

void	LoadUpdateInfo()
{
	MAKEQUERY_LOADUPDATEINFO(strQuery);
	MainDB->execute(strQuery, DBCB_LoadUpdateInfo);
}

static void	DBCB_LoadDBUpdateServerList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if ( !nQueryResult )
		return;

	uint32	uRowNum;	resSet->serial(uRowNum);
	if ( uRowNum == 0 )
		sipwarning("There is no Data in DataServer");

	for ( uint32 i = 0; i < uRowNum; i ++ )
	{
		uint32		idServer;		resSet->serial(idServer);
		ucstring	aliasName;		resSet->serial(aliasName);
		std::string	rootUri;		resSet->serial(rootUri);
		uint32		port;			resSet->serial(port);
		std::string	sUserId;		resSet->serial(sUserId);
		std::string	sUserPassword;	resSet->serial(sUserPassword);

		_entryUpdateServer	info(idServer, aliasName, rootUri, port, sUserId, sUserPassword);
		_ServInfoVector.insert(std::make_pair(idServer, CPatchServerInfo(info, 0)));
	};
	sipdebug("Load UpdateServerList, rownum : %u", uRowNum);

	LoadUpdateInfo();
}

BOOL	LoadDBUpdateServerList()
{
	_ServInfoVector.clear();
	MAKEQUERY_LOADUPDATESERVERS(strQuery);
	MainDB->execute(strQuery, DBCB_LoadDBUpdateServerList);

/*
	_ServInfoVector.clear();
	MAKEQUERY_LOADUPDATESERVERS(strQuery)
	DB_EXE_QUERY(MainDBConnection, Stmt, strQuery)
	if ( !nQueryResult )
	{
		sipinfo("DB query Failed : The reason = ?");
		return false;
	}

	uint	uRowNum = 0;
	do
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			if ( uRowNum == 0 )
            	sipwarning("There is no Data in DataServer");
			break;
		}

		COLUMN_DIGIT(row,	UPDATESERVER_ID,			uint32,		idServer);
		COLUMN_WSTR(row,	UPDATESERVER_ALIASNAME,		aliasName,	MAX_LEN_FTPURI_ALIASNAME);
		COLUMN_STR(row,		UPDATESERVER_ROOTURI,		rootUri,	MAX_LEN_FTPURI_SERVERURI);
		COLUMN_DIGIT(row,	UPDATESERVER_PORT,			uint32,		port);
		COLUMN_STR(row,		UPDATESERVER_USERID,		sUserId,	MAX_LEN_FTPURI_ID);
		COLUMN_STR(row,		UPDATESERVER_USERPASSWORD,	sUserPassword,	MAX_LEN_FTPURI_PASSWORD);

		_entryUpdateServer	info(idServer, aliasName, rootUri, port, sUserId, sUserPassword);
		_ServInfoVector.insert(std::make_pair(idServer, CPatchServerInfo(info, 0)));

		uRowNum ++;
	}while ( true );
	sipdebug("Load UpdateServerList, rownum : %u", uRowNum);
*/
	return true;
}

static void	DBCB_LoadDBLastVersionInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if ( !nQueryResult )
		return;

	uint32 uRowNum;		resSet->serial(uRowNum);
	for( uint32 i=0; i < uRowNum; i ++ )
	{
		uint32		idApp;			resSet->serial(idApp);
		ucstring	nameApp;		resSet->serial(nameApp);
		std::string	lastVersion;	resSet->serial(lastVersion);
		std::string	publishtime;	resSet->serial(publishtime);
		
		// ucstring	ucnameAppLower= wcslwr((wchar_t *)nameApp.c_str());
		ucstring	ucnameAppLower(nameApp.c_str());
		std::transform(ucnameAppLower.begin(), ucnameAppLower.end(), ucnameAppLower.begin(), ::tolower);
		ucstring	ucCorrectNameApp = MakeCorrectProductName(ucnameAppLower);

		_entryAppLastVersion	info(idApp, ucCorrectNameApp, lastVersion);
		_newVersions.insert(std::make_pair(ucCorrectNameApp, info));
		sipinfo("Add last version info:App=%S, ID=%d, Lastversion=%s", ucCorrectNameApp.c_str(), idApp, lastVersion.c_str());
		// sipdebug(L"Add last version info:App=%S, ID=%d, Lastversion=%s", ucCorrectNameApp.c_str(), idApp, lastVersion.c_str());
	};
	sipdebug("Load DBLastVersionInfo, rownum : %u", uRowNum);

	LoadDBUpdateServerList();
}

BOOL	LoadDBLastVersionInfo()
{
	_newVersions.clear();
	MAKEQUERY_LOADLASTVERSIONS(strQuery);
	MainDB->execute(strQuery, DBCB_LoadDBLastVersionInfo);

/*
	DB_EXE_QUERY(MainDBConnection, Stmt, strQuery)
	if ( !nQueryResult )
	{
		sipinfo("DB query Failed : The reason = ?");
		return false;
	}
	uint uRowNum = 0;
	do
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			if ( uRowNum == 0 )
                sipwarning("There is no Update DBLastVersion Info from Server");
			break;
		}

		COLUMN_DIGIT(row,	LASTVERSION_APPID,		uint32,		idApp);
		COLUMN_WSTR(row,	LASTVERSION_APPNAME,	nameApp,	MAX_LEN_APPNAME);
		COLUMN_STR(row,	LASTVERSION_LASTVERSION,	lastVersion,	MAX_LEN_LASTVERSION);

		_entryAppLastVersion	info(idApp, nameApp, lastVersion);
		_newVersions.insert(std::make_pair(wcslwr((wchar_t *)nameApp.c_str()), info));

		uRowNum ++;
	}while ( true );
	sipdebug("Load DBLastVersionInfo, rownum : %u", uRowNum);
*/
	return true;
}

static void	DBCB_LoadIPMnt(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if ( !nQueryResult )
		return;

	uint32	uRows;		resSet->serial(uRows);
	for( uint32 i = 0; i < uRows; i ++ )
	{
		uint32			idIPMnt;	resSet->serial(idIPMnt);
		std::string		strIP;		resSet->serial(strIP);
		uint32			num;		resSet->serial(num);
		uint8			mnt;		resSet->serial(mnt);

		_lstIPMnt.push_back(IPMnt(strIP, num, mnt));
	};
}

BOOL	CUpdateServer::LoadIPMnt()
{
	_lstIPMnt.clear();
	MAKEQUERY_LOADIPMNT(strQuery);
	MainDB->execute(strQuery, DBCB_LoadIPMnt);

/*
	DB_EXE_QUERY(MainDBConnection, Stmt, strQuery);
	if ( !nQueryResult )
		return false;
	do
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
			break;

		COLUMN_DIGIT(row,	0,	uint32,		idIPMnt);
		COLUMN_STR(row,		1,	strIP,	20);
		COLUMN_DIGIT(row,	2,	uint32,		num);
		COLUMN_DIGIT(row,	3,	uint8,		mnt);
		_lstIPMnt.push_back(IPMnt(strIP, num, mnt));

	}while ( true );
*/	
	return true;
}

BOOL	CUpdateServer::LoadDBUpdateInfo()
{
	_newVersions.clear();
	_ServInfoVector.clear();
	_PatchServerAry.clear();
	_HighstPatchServerAry.clear();

	LoadDBLastVersionInfo();
// Load DB Update Data
/*
	if	( !LoadDBLastVersionInfo() )
	{
		siperror("There is no updatedata(lastVersion Info) in DB, so can't start");
		return	false;
	}
	if	( !LoadDBUpdateServerList() )
		return	false;

	MAKEQUERY_LOADUPDATEINFO(strQuery)
	DB_EXE_QUERY(MainDBConnection, Stmt, strQuery)
	if ( !nQueryResult )
	{
		sipinfo("DB query Failed : The reason = ?");
		return false;
	}

	std::string strNull = "";
	TPatchUpdateServerMap		aryPatch;
	uint	uRowNum = 0;
	do
	{
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if ( !IS_DB_ROW_VALID(row) )
		{
			if ( uRowNum == 0 )
				sipwarning("There is no Update Info in Server");
			break;
		}

		COLUMN_DIGIT(row,	APPVERSION_SERVERID,	uint32,		idServer);
		COLUMN_DIGIT(row,	APPVERSION_APPID,		uint32,		idApp);
		COLUMN_STR(row,		APPVERSION_SRCVERSION,	sSrcVer,	MAX_LEN_UPDATE_SRCVER);
		COLUMN_STR(row,		APPVERSION_DSTVERSION,	sDstVer,	MAX_LEN_UPDATE_DSTVER);
		COLUMN_STR(row,		APPVERSION_PATH,		Path,		MAX_LEN_FTPURI_SERVERURI);
		COLUMN_WSTR(row,	APPVERSION_PATH + 1,	ucAppName,	MAX_LEN_APPNAME);
		COLUMN_STR(row,		APPVERSION_PATH + 2,	sLastVersion,	MAX_LEN_LASTVERSION);

		if ( sSrcVer == strNull ||
			sDstVer == strNull ||
			Path == strNull ||
			!wcscmp(ucAppName.c_str(), L"") ||
			sLastVersion == strNull )
		siperror(L"Update data string is null");

		_entryUpdateAppsInfo	info(idServer, idApp, sSrcVer, sDstVer, Path, ucAppName, sLastVersion);
		ucstring	ucKey = ucAppName + "/" + sSrcVer;
		if ( sDstVer == sLastVersion )
		{
			_HighstPatchServerAry.insert(std::make_pair(wcslwr((wchar_t *)ucKey.c_str()), info));
			aryPatch.insert(std::make_pair(wcslwr((wchar_t *)ucKey.c_str()), info));
		}
		else
		{
			if ( sSrcVer != sLastVersion && sSrcVer != sDstVer )
			{
				_PatchServerAry.insert(std::make_pair(wcslwr((wchar_t *)ucKey.c_str()), info));
				aryPatch.insert(std::make_pair(wcslwr((wchar_t *)ucKey.c_str()), info));
			}
		}

		uRowNum ++;
	}while ( true );
	sipdebug("Load UpdateApplicationVersion, rownum : %u", uRowNum);

// Check existance of new version
    sipdebug("Check DB existance of most new version patch");
	if (_HighstPatchServerAry.size() == 0)
		siperror("No existance of most new version patch");
	
	TPatchUpdateServerMapIT	it;
	it = _HighstPatchServerAry.begin();
	_entryUpdateAppsInfo	info= (*it).second;
	ucstring	key = info.AppName + "/" + info.dstVersion;
	key = wcslwr((wchar_t *) key.c_str());
	it = _HighstPatchServerAry.find(key);
	if ( it == _HighstPatchServerAry.end() )
		siperror("No existance of most new version patch, key : %s", key.c_str());

// Check DB Data Validation
    sipdebug("Check DB Data Overlapping and Validation : UpdateApplicationVersion ");
	TPatchUpdateServerMapIT	begin	= aryPatch.begin();
	TPatchUpdateServerMapIT	end		= aryPatch.end();
	for ( TPatchUpdateServerMapIT it = begin; it != end; it ++ )
	{
		ucstring				key	= (*it).first;
		_entryUpdateAppsInfo	info= (*it).second;

		std::string	dstVersion, lastVersion;
		dstVersion = strNull;

		// check overlapping
		int		nCount	= aryPatch.count(key);
		if ( nCount != 1 )
		{
			pair<TPatchUpdateServerMapIT, TPatchUpdateServerMapIT> range = aryPatch.equal_range(key);
			for ( TPatchUpdateServerMapIT it = range.first; it != range.second; it ++ )
			{
				_entryUpdateAppsInfo	info = (*it).second;
				if ( strcmp(info.dstVersion.c_str(), dstVersion.c_str()) )	// dstVersion0 != dstVersion1
				{
					if ( !strcmp(dstVersion.c_str(), strNull.c_str()) )		// dstVersion1 = ""
					{
						dstVersion = info.dstVersion;
						continue;
					}

					siperror(L"Overlapped Various way to dstversion from srcversion <%s> ", key.c_str());
				}
			}
		}

		// check valid : there is way to last version
		bool	bWayToLastVersion = true;
		_entryUpdateAppsInfo	updateInfo = (*it).second;
        while(true)
		{
			dstVersion	= updateInfo.dstVersion;
			lastVersion	= updateInfo.lastVersion;
			if ( dstVersion == lastVersion )
				break;

			ucstring	ucKey = updateInfo.AppName + "/" + dstVersion;
			TPatchUpdateServerMapIT it = aryPatch.find(wcslwr((wchar_t *)ucKey.c_str()));
			if ( it == aryPatch.end() )
			{
				sipdebug(L"Invalid Data: No way to lastversion, startsrcVersion <%s>, cursrcVersion <%s>", key.c_str(), ucKey.c_str());
				bWayToLastVersion = false;
				break;
			}

			updateInfo = (*it).second;
		}

		// if there is no way to last version
		if ( ! bWayToLastVersion )
		{
			pair<TPatchUpdateServerMapIT, TPatchUpdateServerMapIT> range = _PatchServerAry.equal_range(key);
			for ( TPatchUpdateServerMapIT it = range.first; it != range.second; it ++ )
			{
				_entryUpdateAppsInfo	&infoOther = (*it).second;
				if ( info.SrvId		== infoOther.SrvId && 
					info.AppId		== infoOther.AppId &&
					info.srcVersion	== infoOther.srcVersion &&
					info.dstVersion	== infoOther.dstVersion &&
					info.Path		== infoOther.Path &&
					info.AppName	== infoOther.AppName &&
					info.lastVersion== infoOther.lastVersion )
					{
						//infoOther.dstVersion = "";
						_PatchServerAry.erase(it);
						break;
					}
			}
		}
		if ( dstVersion == lastVersion )
			continue;
	}
	aryPatch.clear();
*/
	return true;
}

void	fDownloadNotify(float fProgress, void* pParam, DWORD dwNowSize, void* pMemoryPtr)
{
	// sipinfo	: use notify
}

// in : strPAth
// out: sProductName, sVersion
static	void _spliteProdInfo(ucstring strPath, ucstring &sProductName, string &sVersion)
{
	ucstring	ucTemp;
	size_t pos = strPath.find_first_of('/');
	if (pos != ucstring::npos)
	{
		sProductName = strPath.substr(0, pos);
		ucTemp		 = strPath.substr(pos + 1);
		sVersion	 = ucTemp.toUtf8();
	}
}

T_ERR	CUpdateServer::chooseNewUpdatePatchServer(ucstring ucProductName, std::string sNewVersionNo, uint *pNewServerNo, uint32 nNetworkPart)
{
	ucstring	ucKey;
	ucKey = ucProductName + "/" + sNewVersionNo;

	int counter = static_cast<int>(_HighstPatchServerAry.count(ucKey));
	if ( counter == 0 )
	{
		sipdebug("There is no new version Update Server with such a product, or incorect product name");
		return ERR_NONEWPATCHSERVER;
	}
	
	*pNewServerNo	= SearchMinUserUpdatePatchServer(_HighstPatchServerAry, ucKey, nNetworkPart);

	sipdebug("Newest Version UpdateServer is choosed!, NewUpdateServer %u", *pNewServerNo);

	return ERR_NOERR;
}


T_ERR	CUpdateServer::chooseUpdatePatchServer(ucstring ucProductName, string sVersionNo, uint *pServerNo, uint32 nNetworkPart)
{
	ucstring	ucKey;
	ucKey = ucProductName + "/" + sVersionNo;

	int counter = static_cast<int>(_PatchServerAry.count(ucKey));
	if ( counter == 0 )
	{
		sipdebug("There is no UpdatePatch Server with such a product and version Number");
		return ERR_NOPATCHSERVER;
	}
	*pServerNo		= SearchMinUserUpdatePatchServer(_PatchServerAry, ucKey, nNetworkPart);

    sipdebug("UpdateServer is choosed!, PatchServerNo %u", *pServerNo);

	return ERR_NOERR;
}

SIPBASE_CATEGORISED_COMMAND(SS, dbUpdateData, "Display DBUpdateData", "")
{
	sipinfo("***************** Display DB Update Data *****************");

	TPatchUpdateServerMapIT	it;
	for ( it = _PatchServerAry.begin(); it != _PatchServerAry.end(); it ++ )
	{
		ucstring				key	= (*it).first;
		_entryUpdateAppsInfo	info= (*it).second;
		sipinfo(L"key<%s> -> AppID<%u>, srcVersion<%S>, dstVersion<%S>, Path<%S>, AppName<%s>, lastVersion<%S>",
			key.c_str(), info.AppId, info.srcVersion.c_str(), info.dstVersion.c_str(), info.Path.c_str(), info.AppName.c_str(), info.lastVersion.c_str());
	}

	for ( it = _HighstPatchServerAry.begin(); it != _HighstPatchServerAry.end(); it ++ )
	{
        ucstring				key	= (*it).first;
		_entryUpdateAppsInfo	info= (*it).second;
		sipinfo(L"key<%s> -> AppID<%u>, srcVersion<%S>, dstVersion<%S>, Path<%S>, AppName<%s>, lastVersion<%S>",
			key.c_str(), info.AppId, info.srcVersion.c_str(), info.dstVersion.c_str(), info.Path.c_str(), info.AppName.c_str(), info.lastVersion.c_str());
	}

	return true;
}

SIPBASE_CATEGORISED_COMMAND(SS, reLoadDBUpdateData, "Reload DBUpdateData from DB", "")
{
	sipinfo("***************** Display DB Update Data *****************");

	if ( !CUpdateServer::LoadDBUpdateInfo() )
		return	false;

	sipinfo("Reload DB UpdateData");

	return	true;
}
