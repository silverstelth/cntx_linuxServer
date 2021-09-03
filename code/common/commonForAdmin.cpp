
#include "commonForAdmin.h"

using namespace std;
using namespace SIPBASE;

// �ϳ��� ���ѿ� ���� �������ڷ� �����
string	MakePowerUnit(uint32 _uID)
{
	string	sID = toStringA(_uID);
	string	sResult = BEGINONEPOWERSYMBOL + sID + ENDONEPOWERSYMBOL;

	return sResult;
}
// ��������Ʈ���ѹ��ڷ� �����
static	string	MakePowerUnitOfShard(uint32 _uShardID)
{
	string	sID = toStringA(_uShardID);
	string	sResult = BEGINONEPOWERSYMBOL;
	sResult += TAGSHARDPOWERSYMBOL + sID + ENDONEPOWERSYMBOL;

	return sResult;
}
// Full�������ΰ�
bool	IsFullManager(string _sPower)
{
	string	sP = MakePowerUnit(POWER_FULLMANAGER_ID);
	int nPos = _sPower.find(sP);
	return (nPos == -1) ? false : true ;
}
// Component�������ΰ�
bool	IsComponentManager(string _sPower)
{
	if (IsFullManager(_sPower))
		return true;
	string	sP = MakePowerUnit(POWER_COMPONENTMANAGER_ID);
	int nPos = _sPower.find(sP);
	return (nPos == -1) ? false : true ;
}
// �����������ΰ�
bool	IsAccountManager(string _sPower)
{
	return IsComponentManager(_sPower);
}
// ��������Ʈ�������ΰ�
bool	IsShardManager(string _sPower, uint32 _uShardID)
{
return true;
	if (IsFullManager(_sPower))
		return true;
	string	sP = MakePowerUnitOfShard(_uShardID);
	int nPos = _sPower.find(sP);
	return (nPos == -1) ? false : true ;
}
// ��������Ʈ������ �����Ҽ� �ִ� ������ �ִ°�
bool	IsViewableShardInfo(string _sPower, uint32 _uShardID)
{
return true;
	if (IsComponentManager(_sPower))
		return true;
	if (IsShardManager(_sPower, _uShardID))
		return true;
	return false;
}

// ���� �� 1: ũ��, 0: ����, -1: �۴�
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

// �𹾺���̩ ����˾ ̡�� �����
MAP_MSTSERVICECONFIG		map_MSTSERVICECONFIG;
