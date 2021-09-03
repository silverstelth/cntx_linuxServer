#ifndef TIAN_SERVER_CHBASE_H
#define TIAN_SERVER_CHBASE_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include <misc/types_sip.h>
#include "net/service.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

// ĳ����ID, ����ID, �̸������� �����ϴ� Ŭ��
struct TChBase 
{
	TChBase()	{}
	TChBase(ucstring _ucID, ucstring _ucName)
		:ucID(_ucID), ucName(_ucName)	{	}
		ucstring	ucID;		// ����ID
		ucstring	ucName;		// ĳ���̸�
};
class CCHBaseInfo
{
	SIPBASE_SAFE_SINGLETON_DECL_PTR(CCHBaseInfo);
	CCHBaseInfo()	{};
public:
	bool	Init()
	{
		// SQL������ ����� ������
		string strQuery = "SELECT "DB_CT_ID","DB_CT_USERID","DB_CT_NAME" FROM "DB_CHARACTABLE;
		DB_EXE_QUERY_WITHRESULT(strQuery);
		if (!bSuccess)
			return false;
		for (uint i = 0; i < uRowNum; i++)
		{
			MYSQL_ROW row = res.FetchRow();
			if (row == NULL)
				break;
			uint32		uChID		= atoui(string(row[0]).c_str());				// ĳ����ID
			ucstring	ucID		= ucstring::makeFromUtf8(string(row[1]));		// ID����
			ucstring	ucName		= ucstring::makeFromUtf8(string(row[2]));		// �̸�
			m_mapChBaseKeyCHID.insert(make_pair(uChID, TChBase(ucID, ucName)));
			m_mapChIDKeyucID.insert(make_pair(ucID, uChID));
		}
		return true;
	}
	// ĳ����ID�κ��� ĳ���ͱ��������� ��´�
	bool	GetBaseInfo(uint32 _uChID, TChBase &_baseOut)
	{
		map<uint32, TChBase>::iterator	ItResult = m_mapChBaseKeyCHID.find(_uChID);
		if (ItResult == m_mapChBaseKeyCHID.end())
			return false;

		_baseOut = ItResult->second;
		return true;
	}

	// ID���ڷķκ��� ĳ����ID�� ��´�
	bool	GetChIDOnID(ucstring _ucID, uint32 &_uChIDOut)
	{
		map<ucstring, uint32>::iterator	ItResult = m_mapChIDKeyucID.find(_ucID);
		if (ItResult == m_mapChIDKeyucID.end())
			return false;

		_uChIDOut = ItResult->second;
		return true;
	}

	// ���� â���� ĳ���� ����Ѵ�
	void	AddCh(uint32 _uID, ucstring _uLoginID, ucstring _ucName)
	{
		m_mapChBaseKeyCHID.insert(make_pair(_uID, TChBase(_uLoginID, _ucName)));
		m_mapChIDKeyucID.insert(make_pair(_uLoginID, _uID));

	}

protected:
	map<uint32, TChBase>		m_mapChBaseKeyCHID;			// key�� ĳ����ID�� ĳ���ͱ�������
	map<ucstring, uint32>	m_mapChIDKeyucID;			// key�� ID���ڷ��� ĳ����ID����
};

// �α����� ĳ������ ������ �����ϴ� Ŭ�� template
template<typename T>
class COnCHInfo
{
public:
	byte	GetOnlineState(uint32 _uChID)
	{
		map<uint32, T>::iterator ItResult = m_MapOnChInfo.find(_uChID);
		if (ItResult == m_MapOnChInfo.end())
			return CH_LOGOFF;
		return CH_LOGON;
	}
	bool	GetOnlineData(uint32 _uChID, T *_outV)
	{
		map<uint32, T>::iterator ItResult = m_MapOnChInfo.find(_uChID);
		if (ItResult == m_MapOnChInfo.end())
			return false;
		*_outV = ItResult->second;
		return true;
	}
	void	Insert(uint32 _uChID, const T &_inV)
	{
		m_MapOnChInfo.insert( make_pair(_uChID, _inV) );
	}
	void	Erase(uint32 _uChID)
	{
		m_MapOnChInfo.erase(_uChID);
	}
	T	*Find(uint32 _uChID)
	{
		map<uint32, T>::iterator	ItResult = m_MapOnChInfo.find(_uChID);
		if (ItResult == m_MapOnChInfo.end())
			return NULL;
		return &ItResult->second;
	}
	map<uint32, T>	*GetOnCHInfo() { return &m_MapOnChInfo; };
protected:
	map<uint32, T>	m_MapOnChInfo;
};

// 

#endif // TIAN_SERVER_CHBASE_H