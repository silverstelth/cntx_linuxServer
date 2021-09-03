
#include "commonForAdmin.h"

using namespace std;
using namespace SIPBASE;

// 하나의 권한에 대한 단위문자렬 만들기
string	MakePowerUnit(uint32 _uID)
{
	string	sID = toStringA(_uID);
	string	sResult = BEGINONEPOWERSYMBOL + sID + ENDONEPOWERSYMBOL;

	return sResult;
}
// 지역싸이트권한문자렬 만들기
static	string	MakePowerUnitOfShard(uint32 _uShardID)
{
	string	sID = toStringA(_uShardID);
	string	sResult = BEGINONEPOWERSYMBOL;
	sResult += TAGSHARDPOWERSYMBOL + sID + ENDONEPOWERSYMBOL;

	return sResult;
}
// Full관리자인가
bool	IsFullManager(string _sPower)
{
	string	sP = MakePowerUnit(POWER_FULLMANAGER_ID);
	int nPos = _sPower.find(sP);
	return (nPos == -1) ? false : true ;
}
// Component관리자인가
bool	IsComponentManager(string _sPower)
{
	if (IsFullManager(_sPower))
		return true;
	string	sP = MakePowerUnit(POWER_COMPONENTMANAGER_ID);
	int nPos = _sPower.find(sP);
	return (nPos == -1) ? false : true ;
}
// 계정관리자인가
bool	IsAccountManager(string _sPower)
{
	return IsComponentManager(_sPower);
}
// 지역싸이트관리자인가
bool	IsShardManager(string _sPower, uint32 _uShardID)
{
return true;
	if (IsFullManager(_sPower))
		return true;
	string	sP = MakePowerUnitOfShard(_uShardID);
	int nPos = _sPower.find(sP);
	return (nPos == -1) ? false : true ;
}
// 지역싸이트정보를 열람할수 있는 권한이 있는가
bool	IsViewableShardInfo(string _sPower, uint32 _uShardID)
{
return true;
	if (IsComponentManager(_sPower))
		return true;
	if (IsShardManager(_sPower, _uShardID))
		return true;
	return false;
}

// 권한 비교 1: 크다, 0: 같다, -1: 작다
int	ComparePower(string _sP1, string _sP2)
{
	if (_sP1 == _sP2)
		return 0;
	if (IsFullManager(_sP1))
		return 1;
	if (IsFullManager(_sP2))
		return -1;
	if (IsComponentManager(_sP1))
		return 1;
	if (IsComponentManager(_sP2))
		return -1;
	return 0;
}

// 헴뭬빕능揭 괩눼鱇 瞼쬔 설몬능
MAP_MSTSERVICECONFIG		map_MSTSERVICECONFIG;
