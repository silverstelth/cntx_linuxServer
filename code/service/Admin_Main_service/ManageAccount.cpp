
#include "ManageAccount.h"
#include "QuerySet.h"
#include "misc/db_interface.h"
#include "../../common/Packet.h"
#include "ManageComponent.h"
#include "ManageAESConnection.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

CCallbackServer *	ManageHostServer = NULL;


CVariable<uint32> MaxManagerConnect("as","MaxManagerConnect", "MaxManagerConnect", 10, 0, true);

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Database module begin ///////////////////////////////////

// Update정보
SHARDUPDATEINFO		ShardUpdateInfo;

// 관리자정보
static	MAP_ACCOUNTINFO				map_ACCOUNTINFO;
static	MAP_ACCOUNTINFO				map_ACCOUNTINFODELETE;

#define	FOR_START_MAP_ACCOUNTINFO(it)											\
	MAP_ACCOUNTINFO::iterator	it;												\
	for(it = map_ACCOUNTINFO.begin(); it != map_ACCOUNTINFO.end(); it++)		\

#define	FOR_START_MAP_ACCOUNTINFODELETE(it)											\
	MAP_ACCOUNTINFO::iterator	it;												\
	for(it = map_ACCOUNTINFODELETE.begin(); it != map_ACCOUNTINFODELETE.end(); it++)		\

static	bool	IsManagerAccount(std::string _sAccount)
{
	FOR_START_MAP_ACCOUNTINFO(it)
	{
		if (_sAccount == it->second.m_sAccount)
			return true;
	}
	FOR_START_MAP_ACCOUNTINFODELETE(itDelete)
	{
		if (_sAccount == itDelete->second.m_sAccount)
			return true;
	}
	
	return false;
}

// Manager의 접속정보
struct	CONNECTMANAGER
{
	CONNECTMANAGER(TSockId _sockid) : m_SockId(_sockid)	{ m_ConnectTime = CTime::getLocalTime(); }
	TSockId				m_SockId;
	TTime				m_ConnectTime;
};

// Online Manager(m_uDBID가 령이면 인증을 거치지 않은 상태이다)
struct	ONLINEMANAGER
{
	ONLINEMANAGER(TSockId _sockid) : m_uDBID(0), m_Connection(CONNECTMANAGER(_sockid)) {}
	uint32				m_uDBID;
	CONNECTMANAGER		m_Connection;
};
typedef	map<TSockId, ONLINEMANAGER>		MAP_ONLINEMANAGER;
static MAP_ONLINEMANAGER				map_ONLINEMANAGER;

#define	FOR_START_MAP_ONLINEMANAGER(it)											\
	MAP_ONLINEMANAGER::iterator	it;												\
	for(it = map_ONLINEMANAGER.begin(); it != map_ONLINEMANAGER.end(); it++)	\

// 온라인된 관리자인가
static	bool	IsOnlineMansger(uint32 _uID)
{
	FOR_START_MAP_ONLINEMANAGER(it)
	{
		if (it->second.m_uDBID == _uID)
			return true;
	}
	return false;
}
static	ONLINEMANAGER *GetOnlineManager(uint32 _uID)
{
	FOR_START_MAP_ONLINEMANAGER(it)
	{
		if (it->second.m_uDBID == _uID)
			return &(it->second);
	}
	return NULL;
}
static void	DBCB_DBGetUpdateInfo(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows > 0)
	{
		ucstring	address;		resSet->serial(address);
		uint32		port;			resSet->serial(port);
		ucstring	name;			resSet->serial(name);
		string		password;		resSet->serial(password);	
		ucstring	path;			resSet->serial(path);

		ShardUpdateInfo.m_ucServerName		= address;
		ShardUpdateInfo.m_uPort				= (uint16)port;
		ShardUpdateInfo.m_ucUsername		= name;
		ShardUpdateInfo.m_ucPassword		= ucstring(password);
		ShardUpdateInfo.m_ucRemoteFileName	= path;
	}
}

// Update정보를 load한다
static	void	LoadUpdateInfo()
{
	{
		MAKEQUERY_GETUPDATEINFO(strQ);
		DBCaller->execute(strQ, DBCB_DBGetUpdateInfo);
		/*
		MAKEQUERY_GETUPDATEINFO(strQ);
		DB_EXE_QUERY(DBForManage, Stmt, strQ);
		if (nQueryResult != TRUE)
		{
			sipwarning("No update information");
			return;
		}
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (IS_DB_ROW_VALID(row))
		{
			COLUMN_WSTR(row, 0, address, 100);
			COLUMN_DIGIT(row, 1, uint32, port);
			COLUMN_WSTR(row, 2, name, 100);
			COLUMN_STR(row, 3, password, 100);
			COLUMN_WSTR(row, 4, path, 100);

			ShardUpdateInfo.m_ucServerName		= address;
			ShardUpdateInfo.m_uPort				= (uint16)port;
			ShardUpdateInfo.m_ucUsername		= name;
			ShardUpdateInfo.m_ucPassword		= ucstring(password);
			ShardUpdateInfo.m_ucRemoteFileName	= path;
		}
		else
		{
			sipwarning("No update information");
			return;
		}
		*/
	}

}

static void	DBCB_DBGetAllManger(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	for (uint32 i = 0; i < nRows; i++)
	{
		uint32 uID;				resSet->serial(uID);
		string sName;			resSet->serial(sName);
		string sPassword;		resSet->serial(sPassword);
		uint8 uPower;			resSet->serial(uPower);
		uint32 uDelete;			resSet->serial(uDelete);
		if (uDelete == 0)
			map_ACCOUNTINFO.insert( make_pair(uID, ACCOUNTINFO(uID, sName, sPassword, uPower)) );
		else
			map_ACCOUNTINFODELETE.insert( make_pair(uID, ACCOUNTINFO(uID, sName, sPassword, uPower)) );
	}
}

// 관리자정보를 load한다
static	void	LoadAccountInfo()
{
	map_ACCOUNTINFO.clear();
	MAKEQUERY_GETALLMANAGER(strQ);
	DBCaller->execute(strQ, DBCB_DBGetAllManger);
/*	
	MAKEQUERY_GETALLMANAGER(sQ);
	DB_EXE_QUERY(DBForManage, Stmt, sQ);

	if (nQueryResult == TRUE)
	{
		do 
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (IS_DB_ROW_VALID(row))
			{
				COLUMN_DIGIT(row, 0, uint32, uID);
				COLUMN_STR(row, 1, sName, 100);
				COLUMN_STR(row, 2, sPassword, 100);
				COLUMN_DIGIT(row, 3, uint8, uPower);
				COLUMN_DIGIT(row, 4, uint32, uDelete);
				if (uDelete == 0)
					map_ACCOUNTINFO.insert( make_pair(uID, ACCOUNTINFO(uID, sName, sPassword, uPower)) );
				else
					map_ACCOUNTINFODELETE.insert( make_pair(uID, ACCOUNTINFO(uID, sName, sPassword, uPower)) );
			}
			else
				break;
		} while(TRUE);
	}
*/
}
// 관리자정보얻기
static	const	ACCOUNTINFO * GetManagerInfo(uint32 _uDBID)
{
	MAP_ACCOUNTINFO::iterator	it = map_ACCOUNTINFO.find(_uDBID);
	if (it == map_ACCOUNTINFO.end())
		return NULL;

	return &(it->second);
}
// ID와 Password에 의한 인증
static	uint32	ConfirmAccount(string _sAccount, string _sPassword, uint32 &_uDBID)
{
	FOR_START_MAP_ACCOUNTINFO(it)
	{
		string	sAccount = it->second.m_sAccount;
		if (sAccount != _sAccount)
			continue;

		string	sPassword = it->second.m_sPassword;
		if (sPassword != _sPassword)
		{
			sipwarning("Failed account confirming(Account=%s, Password=%s)", sAccount.c_str(), _sPassword.c_str());
			return CONFIRM_MH_ACCOUNT_MISS;
		}
		
		_uDBID = it->second.m_uID;
		if (IsOnlineMansger(_uDBID))
			return CONFIRM_MH_ACCOUNT_DOUBLE;
		return CONFIRM_MH_ACCOUNT_SUCCESS;
	}
	return CONFIRM_MH_ACCOUNT_NO;
}


//////////////////////////// Database module end ////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
//////////////// 접속된 관리자 관리모쥴 begin ///////////////////////////////////////
// OnlineManager얻기
const	ACCOUNTINFO * GetManagerInfo(TSockId from)
{
	MAP_ONLINEMANAGER::iterator	it = map_ONLINEMANAGER.find(from);
	if (it == map_ONLINEMANAGER.end())
		return NULL;

	uint32	uDBID = it->second.m_uDBID;

	return GetManagerInfo(uDBID);
}

// 관리자 암호설정
static void	SetManagerPassword(TSockId _from, string _sPassword)
{
	MAP_ONLINEMANAGER::iterator	it = map_ONLINEMANAGER.find(_from);
	if (it == map_ONLINEMANAGER.end())
		return;

	MAP_ACCOUNTINFO::iterator	itAccount = map_ACCOUNTINFO.find(it->second.m_uDBID);
	if (itAccount == map_ACCOUNTINFO.end())
		return;

	itAccount->second.m_sPassword = _sPassword;
}

static void	SetManagerName(TSockId _from, string _sName)
{
	MAP_ONLINEMANAGER::iterator	it = map_ONLINEMANAGER.find(_from);
	if (it == map_ONLINEMANAGER.end())
		return;

	MAP_ACCOUNTINFO::iterator	itAccount = map_ACCOUNTINFO.find(it->second.m_uDBID);
	if (itAccount == map_ACCOUNTINFO.end())
		return;

	itAccount->second.m_sAccount = _sName;
}

// Manager 접속
static void cbManagerConnection (TSockId from, void *arg)
{
	map_ONLINEMANAGER.insert( make_pair(from, ONLINEMANAGER(from)) );
}

static void	Send_M_MH_MANAGERONLINE(uint32 _uID, bool _bOnline);
// Manager 접속해제
static void cbManagerDisconnection( TSockId from, void *arg )
{
	// Log
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo != NULL)
	{
		sipinfo("Manager(%s) is disconnected.", pManagerInfo->m_sAccount.c_str());
		ucstring	ucLog	= L"Manager(" + ucstring(pManagerInfo->m_sAccount) + L") is disconnected.";
		AddManageHostLog(LOG_MANAGE_DISCONNECT, from, ucLog);

		Send_M_MH_MANAGERONLINE(pManagerInfo->m_uID, false);
	}

	map_ONLINEMANAGER.erase( from );
}
// 인증이 되지 않은 접속들을 차단한다
static	void	UpdateConnection()
{
	static	TTime limitTime = 10000;	// 10초를 허용대기시간으로 한다

	TTime	CurTime = CTime::getLocalTime();
	FOR_START_MAP_ONLINEMANAGER(it)
	{
		if (it->second.m_uDBID != 0)
			continue;

		TTime connectTime = it->second.m_Connection.m_ConnectTime;
		if (CurTime - connectTime > limitTime )
		{
			sipwarning("Remove non-confirm connection.");
			ManageHostServer->disconnect(it->first);
			map_ONLINEMANAGER.erase(it);
			break;
		}
	}
}
// 온라인접속에 userid설정
static	void	SetDBID( TSockId from, uint32 _uDBID )
{
	MAP_ONLINEMANAGER::iterator	it = map_ONLINEMANAGER.find(from);
	if (it == map_ONLINEMANAGER.end())
		return;
	it->second.m_uDBID = _uDBID;
}

//////////////// 접속된 관리자 관리모쥴 end /////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Send module begin //////////////////////////////////////
// 인증결과 보내기
static void	Send_M_MH_LOGIN_MAN_HOST(TSockId from, uint32 _uResult, uint32 _uDBID)
{
	CMessage	mesResult(M_MH_LOGIN_MAN_HOST);
	mesResult.serial(_uResult, _uDBID);
	ManageHostServer->send(mesResult, from);
}
// 암호변경결과 보내기
static void	Send_M_MH_CHPWD(TSockId _from, string _sPassword, int _nResult)
{
	uint32		uResult = CHPWD_MH_SUCCESS;
	if (_nResult == 0)
		uResult = CHPWD_MH_FAILED;

	CMessage	mesResult(M_MH_CHPWD);
	mesResult.serial(uResult, _sPassword);
	ManageHostServer->send(mesResult, _from);
}
static void	Send_M_MH_CHNAME(TSockId _from, string _sName, int _nResult)
{
	uint32		uResult = CHNAME_MH_SUCCESS;
	if (_nResult == 0)
		uResult = CHNAME_MH_FAILED;

	CMessage	mesResult(M_MH_CHNAME);
	mesResult.serial(uResult, _sName);
	ManageHostServer->send(mesResult, _from);
}

static void	Send_M_MH_ACCOUNTINFO(TSockId from)
{
	// 관리자권한정보보내기
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	CMessage	mesPower(M_MH_ACCOUNTINFO);

	uint32		tempID		= pManagerInfo->m_uID;
	string		tempAccount	= pManagerInfo->m_sAccount;
	string		tempPass	= pManagerInfo->m_sPassword;
	string		tempPower	= pManagerInfo->m_sPower;

	mesPower.serial(tempID, tempAccount, tempPass, tempPower);
	
	ManageHostServer->send(mesPower, from);
}

// 관리자목록정보 보내기
static void	Send_M_MH_MANAGEINFO(TSockId from)
{
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	string		sPower = pManagerInfo->m_sPower;
	
	CMessage	mesOut(M_MH_MANAGEINFO);
	FOR_START_MAP_ACCOUNTINFO(it)
	{
		string	sCurPower	= it->second.m_sPower;
		int		nCompare	= ComparePower(sPower, sCurPower);
		if (nCompare == -1)
			continue;
		bool	bOnline	= IsOnlineMansger(it->second.m_uID);
		mesOut.serial(it->second.m_uID, it->second.m_sAccount, it->second.m_sPower, bOnline);
	}
	ManageHostServer->send(mesOut, from);
}

static void	Send_M_MH_MANAGERONLINE(uint32 _uID, bool _bOnline)
{
	CMessage	mesOut(M_MH_MANAGERONLINE);
	mesOut.serial(_uID, _bOnline);
	ManageHostServer->send(mesOut);
}

static void	Send_M_MH_CHUPDATE(TSockId from)
{
	CMessage	mesOut(M_MH_CHUPDATE);
	string		sPass = ShardUpdateInfo.m_ucPassword.toString();
	mesOut.serial(ShardUpdateInfo.m_ucServerName, ShardUpdateInfo.m_uPort, ShardUpdateInfo.m_ucUsername, sPass, ShardUpdateInfo.m_ucRemoteFileName);
	ManageHostServer->send(mesOut, from);
}


//////////////////////////// Send module end ////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// Callback모쥴 begin /////////////////////////////////////
// Manager로그인
static void cb_M_MH_LOGIN_MAN_HOST( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	string	sAccount, sPassword;
	msgin.serial(sAccount, sPassword);

	uint32	uDBID = 0;
	uint32	uResult;
	uint32	uCurNum = static_cast<uint32>(map_ONLINEMANAGER.size());
	if (uCurNum > MaxManagerConnect)
		uResult = CONFIRM_MH_ACCOUNT_MAX;
	else
		uResult = ConfirmAccount(sAccount, sPassword, uDBID);

	Send_M_MH_LOGIN_MAN_HOST(from, uResult, uDBID);

	// 인증에 성공한 경우 목록갱신
	if (uResult != CONFIRM_MH_ACCOUNT_SUCCESS)
	{
		ManageHostServer->disconnect(from);
		map_ONLINEMANAGER.erase(from);
		return;
	}

	// userid설정
	SetDBID(from, uDBID);
	Send_M_MH_MANAGERONLINE(uDBID, true);

	const CInetAddress	&ia =	cnb.hostAddress (from);

	sipinfo("Manager('%s') is connected(%s).", sAccount.c_str(), ia.asString().c_str());

	ucstring	ucLog	= L"Manager(" + ucstring(sAccount) + L") is connected(" + ucstring(ia.asString()) +L")";
	AddManageHostLog(LOG_MANAGE_CONNECT, from, ucLog);

}

// 관리자암호변경
static void cb_M_MH_CHPWD( CMessage& _msgin, TSockId _from, CCallbackNetBase& _cnb )
{
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(_from);
	if (pManagerInfo == NULL)
		return;

	string	sPassword;
	_msgin.serial(sPassword);

	MAKEQUERY_UPDATEPWD(strQ, pManagerInfo->m_uID, ucstring(sPassword).c_str() );
	DBCaller->execute(strQ);
	Send_M_MH_CHPWD(_from, sPassword, true);
	SetManagerPassword(_from, sPassword);
	sipinfo("Manager's password changing : result=%d.", true);
	/*
	MAKEQUERY_UPDATEPWD(strQ, pManagerInfo->m_uID, ucstring(sPassword).c_str() );
	DB_EXE_QUERY(DBForManage, Stmt, strQ);

	Send_M_MH_CHPWD(_from, sPassword, nQueryResult);

	if (nQueryResult == TRUE)
		SetManagerPassword(_from, sPassword);

	sipinfo("Manager's password changing : result=%d.", nQueryResult);
	*/
}

static void cb_M_MH_CHNAME( CMessage& _msgin, TSockId _from, CCallbackNetBase& _cnb )
{
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(_from);
	if (pManagerInfo == NULL)
		return;

	string	sName;
	_msgin.serial(sName);

	MAKEQUERY_UPDATENAME(strQ, pManagerInfo->m_uID, ucstring(sName).c_str() );
	DBCaller->execute(strQ);
	Send_M_MH_CHNAME(_from, sName, true);
	SetManagerName(_from, sName);
	sipinfo("Manager's name changing : result=%d.", true);
/*
	MAKEQUERY_UPDATENAME(strQ, pManagerInfo->m_uID, ucstring(sName).c_str() );
	DB_EXE_QUERY(DBForManage, Stmt, strQ);

	Send_M_MH_CHNAME(_from, sName, nQueryResult);

	if (nQueryResult == TRUE)
		SetManagerName(_from, sName);

	sipinfo("Manager's name changing : result=%d.", nQueryResult);
	*/
}

static void	DBCB_DBLog(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	uint32	uPage	= *(uint32 *)argv[0];
	TSockId	from	= *(TSockId *)argv[1];
	if (!nQueryResult)
		return;

	uint32 nRows;	resSet->serial(nRows);
	if (nRows <= 0)
		return;

	uint32 ret;		resSet->serial(ret);
	uint32 uNum;	resSet->serial(uNum);

	CMessage msg(M_MH_LOG);
	msg.serial(uPage);
	msg.serial(uNum);
	if (ret == 1010)
	{
		ManageHostServer->send(msg, from);
		return;
	}
	if (ret == 0)
	{
		uint32 nRows2;	resSet->serial(nRows2);
		for (uint32 i = 0; i < nRows2; i++)
		{
			ucstring	cont;		resSet->serial(cont);
			string		name;		resSet->serial(name);
			string		ltime;		resSet->serial(ltime);
			msg.serial(cont, name, ltime);
		}
		ManageHostServer->send(msg, from);
	}
}

static void cb_M_MH_LOG( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	uint32	uPage;
	msgin.serial(uPage);
	uint8	uPower = pManagerInfo->m_uPower;

	MAKEQUERY_LOG(strQ, uPage, uPower);
	DBCaller->executeWithParam(strQ, DBCB_DBLog,
		"DD",
		CB_,	uPage,
		CB_,	from);
/*
	CMessage msg(M_MH_LOG);
	msg.serial(uPage);

	MAKEQUERY_LOG(strQ, uPage, uPower);
	DB_EXE_QUERY(DBForManage, Stmt, strQ);
	if ( nQueryResult == TRUE )
	{
		// 결과 얻기
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (IS_DB_ROW_VALID(row))
		{
			COLUMN_DIGIT(row, 0, uint32, ret);
			COLUMN_DIGIT(row, 1, uint32, uNum);
			msg.serial(uNum);
			if (ret == 1010)
			{
				ManageHostServer->send(msg, from);
				return;
			}
			if (ret == 0)
			{
				short  sMore = SQLMoreResults(Stmt);
				if (sMore == SQL_SUCCESS || sMore == SQL_SUCCESS_WITH_INFO)
				{
					do
					{
						DB_QUERY_RESULT_FETCH(Stmt, row2);
						if (IS_DB_ROW_VALID(row2))
						{
							COLUMN_WSTR(row2, 0, cont, 100);
							COLUMN_STR(row2, 1, name, 100);
							COLUMN_DATETIME(row2, 2, ltime);
							msg.serial(cont, name, ltime);
						}
						else
							break;
					} while(TRUE);
					ManageHostServer->send(msg, from);
				}
			}
		}
	}
*/	
}
static void cb_M_MH_LOGRESET( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	
	if (!IsFullManager(pManagerInfo->m_sPower))
		return;

	MAKEQUERY_LOGRESET(strQ);
	DBCaller->execute(strQ);

	CMessage msg(M_MH_LOGRESET);
	ManageHostServer->send(msg);
}

// 관리자정보얻기(자기자체)
static void cb_M_MH_ACCOUNTINFO( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	Send_M_MH_ACCOUNTINFO(from);

	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;
	if (IsFullManager(pManagerInfo->m_sPower))
		Send_M_MH_CHUPDATE(from);
}

// 관리자목록정보
static void cb_M_MH_MANAGEINFO( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	Send_M_MH_MANAGEINFO(from);
}

static void	DBCB_DBInsertManager(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	string		sAccount	= (char *)argv[0];
	string		sPassword	= (char *)argv[1];
	uint8		uPower		= *(uint8 *)argv[2];
	TSockId		from		= *(TSockId *)argv[3];
	if (!nQueryResult)
		return;
	
	uint32	nRows;		resSet->serial(nRows);
	if (nRows <= 0)
		return;

	uint32 uNewID;		resSet->serial(uNewID);
	map_ACCOUNTINFO.insert( make_pair(uNewID, ACCOUNTINFO(uNewID, sAccount, sPassword, uPower)) );

	// Send
	string	sPower = toStringA(uPower);
	CMessage	mes(M_MH_ADDMANAGER);
	mes.serial(uNewID, sAccount, sPower);
	ManageHostServer->send(mes);

	// Log기록
	ucstring	ucLog	= L"Add Manager(ID=" +toStringW(uNewID) + L") Name=" + ucstring(sAccount) + L", Power=" + ucstring(sPower);
	AddManageHostLog(LOG_MANAGE_MANAGER, from, ucLog);
}

// 관리자추가
static void cb_M_MH_ADDMANAGER( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	string	sAccount, sPassword, sPower;
	uint8	uPower;
	msgin.serial(sAccount, sPassword, uPower);

	if (sAccount.size() < 1 || sPassword.size() < 1)
		return;

	sPower = MakePowerUnit(uPower);
	
	int nCompare = ComparePower(pManagerInfo->m_sPower, sPower);
	if (nCompare != 1)
		return;

	if (IsFullManager(sPower))
		return;

	if (IsManagerAccount(sAccount))
		return;

	MAKEQUERY_INSERTMANAGER(strQ, ucstring(sAccount).c_str(), ucstring(sPassword).c_str(), uPower);
	DBCaller->executeWithParam(strQ, DBCB_DBInsertManager,
		"ssBD",
		CB_,		sAccount.c_str(),
		CB_,		sPassword.c_str(),
		CB_,		uPower,
		CB_,		from);
	/*
	uint32	uNewID;
	{
		MAKEQUERY_INSERTMANAGER(strQ, ucstring(sAccount).c_str(), ucstring(sPassword).c_str(), uPower);
		DB_EXE_QUERY(DBForManage, Stmt, strQ);
		if (nQueryResult != TRUE)
			return;
		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (IS_DB_ROW_VALID(row))
		{
			COLUMN_DIGIT(row, 0, uint32, uID);
			uNewID = uID;
			map_ACCOUNTINFO.insert( make_pair(uNewID, ACCOUNTINFO(uID, sAccount, sPassword, uPower)) );

			// Send
			CMessage	mes(M_MH_ADDMANAGER);
			mes.serial(uNewID, sAccount, sPower);
			ManageHostServer->send(mes);
		}
	}

	// Log기록
	ucstring	ucLog	= L"Add Manager(ID=" +toStringW(uNewID) + L") Name=" + ucstring(sAccount) + L", Power=" + ucstring(sPower);
	AddManageHostLog(LOG_MANAGE_MANAGER, from, ucLog);
	*/
}

// 관리자삭제
static void cb_M_MH_DELMANAGER( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	if (!IsAccountManager(pManagerInfo->m_sPower))
		return;

	uint32	uID;
	msgin.serial(uID);

	if (IsOnlineMansger(uID))
		return;

	CMessage	mes(M_MH_DELMANAGER);
	mes.serial(uID);

	MAP_ACCOUNTINFO::iterator	it = map_ACCOUNTINFO.find(uID);
	if (it == map_ACCOUNTINFO.end())
	{
		ManageHostServer->send(mes);
		return;
	}

	// DB에서 삭제
	MAKEQUERY_DELETEMANAGER(strQ, uID);
	DBCaller->execute(strQ);
	/*
	MAKEQUERY_DELETEMANAGER(strQ, uID);
	DB_EXE_QUERY(DBForManage, Stmt, strQ);
	if (nQueryResult != TRUE)
		return;
	*/
	map_ACCOUNTINFODELETE.insert( make_pair(uID, ACCOUNTINFO(uID, it->second.m_sAccount, it->second.m_sPassword, it->second.m_uPower)) );
	map_ACCOUNTINFO.erase(uID);
	ManageHostServer->send(mes);

	// Log기록
	ucstring	ucLog	= L"Delete Manager(ID=" +toStringW(uID) + L")";
	AddManageHostLog(LOG_MANAGE_MANAGER, from, ucLog);
}

static void cb_M_MH_DISCONNECT( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	uint32	uID;
	msgin.serial(uID);

	ONLINEMANAGER *pOnline = GetOnlineManager(uID);
	if (pOnline == NULL)
		return;

	const	ACCOUNTINFO *pManager = GetManagerInfo(uID);

	int nCompare = ComparePower(pManagerInfo->m_sPower, pManager->m_sPower);
	if (nCompare != 1)
		return;

	ManageHostServer->disconnect(pOnline->m_Connection.m_SockId);
}

static void cb_M_MH_UPDATE( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	uint16	uHostID;
	msgin.serial(uHostID);

	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	if (!IsFullManager(pManagerInfo->m_sPower))
	{
		sipwarning("Update is done only by administrator");
		return;
	}

	ONLINE_AES *pAES = NULL;
	FOR_START_MAP_ONLINE_AES(it)
	{
		if (it->second.m_ConnectionInfo.m_ServiceID.get() != uHostID)
			continue;
		pAES = &(it->second);
		break;
	}
	if (pAES == NULL)
		return;

	bool	bIsConnected = ( pAES->m_OnlineService.size() == 0 ) ? false : true;
	if (bIsConnected)
	{
		sipwarning("Update is unable, because being onlined service");
		return;
	}

	ucstring		ucServerName		= ShardUpdateInfo.m_ucServerName;
	uint16			uPort				= ShardUpdateInfo.m_uPort;
	ucstring		ucUsername			= ShardUpdateInfo.m_ucUsername;	
	ucstring		ucPassword			= ShardUpdateInfo.m_ucPassword;
	ucstring		ucRemoteFileName	= ShardUpdateInfo.m_ucRemoteFileName;

	TServiceId	curSID		= pAES->m_ConnectionInfo.m_ServiceID;
	CMessage		msgOut(M_MH_UPDATE);
	msgOut.serial(ucServerName, uPort, ucUsername, ucPassword, ucRemoteFileName);
	CUnifiedNetwork::getInstance ()->send ( curSID, msgOut );

	// Log기록
	ucstring	ucLog	= L"Update Host : ID=" + toStringW(uHostID);
	AddManageHostLog(LOG_MANAGE_UPDATE, from, ucLog);
}

static void cb_M_MH_CHUPDATE( CMessage& msgin, TSockId from, CCallbackNetBase& cnb )
{
	const	ACCOUNTINFO * pManagerInfo = GetManagerInfo(from);
	if (pManagerInfo == NULL)
		return;

	if (!IsFullManager(pManagerInfo->m_sPower))
		return;

	ucstring		ucServerName;
	uint16			uPort;
	ucstring		ucUsername;	
	string			sPassword;
	ucstring		ucRemoteFileName;
	
	msgin.serial(ucServerName, uPort, ucUsername, sPassword, ucRemoteFileName);

	MAKEQUERY_CHUPDATEINFO(strQ, ucServerName.c_str(), uPort, ucUsername.c_str(), ucstring(sPassword).c_str(), ucRemoteFileName.c_str());
	DBCaller->execute(strQ);
/*
	DB_EXE_QUERY(DBForManage, Stmt, strQ);
	if (nQueryResult != TRUE)
	{
		return;
	}
*/
	ShardUpdateInfo.m_ucServerName		= ucServerName;
	ShardUpdateInfo.m_uPort				= uPort;
	ShardUpdateInfo.m_ucUsername		= ucUsername;	
	ShardUpdateInfo.m_ucPassword		= ucstring(sPassword);
	ShardUpdateInfo.m_ucRemoteFileName	= ucRemoteFileName;

	Send_M_MH_CHUPDATE(NULL);

	// Log기록
	ucstring	ucLog	= L"Change update info";
	AddManageHostLog(LOG_MANAGE_CHUPDATE, from, ucLog);
}

static TCallbackItem CallbackArray[] =
{
	{ M_MH_LOGIN_MAN_HOST,	cb_M_MH_LOGIN_MAN_HOST },
	{ M_MH_CHPWD,			cb_M_MH_CHPWD },
	{ M_MH_CHNAME,			cb_M_MH_CHNAME },
	{ M_MH_LOG,				cb_M_MH_LOG },
	{ M_MH_LOGRESET,		cb_M_MH_LOGRESET },
	{ M_MH_ACCOUNTINFO,		cb_M_MH_ACCOUNTINFO },
	{ M_MH_MANAGEINFO,		cb_M_MH_MANAGEINFO },
	{ M_MH_ADDMANAGER,		cb_M_MH_ADDMANAGER },
	{ M_MH_DELMANAGER,		cb_M_MH_DELMANAGER },
	{ M_MH_DISCONNECT,		cb_M_MH_DISCONNECT },
	{ M_MH_UPDATE,			cb_M_MH_UPDATE },
	{ M_MH_CHUPDATE,		cb_M_MH_CHUPDATE },
};
//////////////////////////// Callback모쥴 end ///////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
//////////////////////////// extern 모쥴 ///////////////////////////////////////////

// 초기화
void	InitManageAccount()
{
	ManageHostServer = new CCallbackServer ();
	if (ManageHostServer == NULL)
	{
		siperrornoex("Failed creating ManagerHost's server.");
		return;
	}
	ManageHostServer->addCallbackArray (CallbackArray, sizeof(CallbackArray)/sizeof(TCallbackItem));
	ManageHostServer->setConnectionCallback(cbManagerConnection, 0);
	ManageHostServer->setDisconnectionCallback (cbManagerDisconnection, 0);

	uint16 port = 40000;
	if ( IService::getInstance()->ConfigFile.exists("ManagePort") )
		port = IService::getInstance()->ConfigFile.getVar("ManagePort").asInt();

	ManageHostServer->init(port);
	
	LoadAccountInfo();
	LoadUpdateInfo();
}

// Update
void	UpdateManageAccount()
{
	ManageHostServer->update();
	UpdateConnection();
}

void	ReleaseManageAccount()
{
	if ( ManageHostServer )
	{
		ManageHostServer->disconnect(InvalidSockId);
		delete ManageHostServer;
		ManageHostServer = NULL;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

void	AddManageHostLog(uint32 _uType, TSockId from, ucstring _ucContent)
{
	string	sAccount = "";
	const	ACCOUNTINFO * pManagerInfo	= GetManagerInfo(from);
	if (pManagerInfo != NULL)
		sAccount = pManagerInfo->m_sAccount;
	uint8	uPower = pManagerInfo->m_uPower;
	MAKEQUERY_ADDLOG(sLogQ, _uType, _ucContent.c_str(), uPower, ucstring(sAccount).c_str());
	DBCaller->execute(sLogQ);
	//DB_EXE_QUERY(DBForManage, LogStmt, sLogQ);
}
