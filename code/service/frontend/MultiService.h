#ifndef TIAN_FRONTEND_MULTISERVICE_H
#define TIAN_FRONTEND_MULTISERVICE_H

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif

#include <misc/types_sip.h>
#include "net/service.h"
#include "../../common/common.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

template<typename T>
class CMultiService
{
public:
	CMultiService(string _servicename)
		:m_strServiceName(_servicename)	{};

	void		AddService(TServiceId _tsid);					// 새로운 기능써비스 추가
	void		RemoveService(TServiceId _tsid);				// 기능써비스 삭제
	void        AddLobbyService(TServiceId _tsid);              // add the serviceId to the lobby serviceId list
	void        RemoveLobbyService(TServiceId _tsid);           // erase the serviceId from the lobby serviceId list
	//TServiceId	GetBestServiceID(const T &_load);				// 부하에 적합한 기능써비스ID를 얻는다
	TServiceId  GetNextLobbySIDForFID(const T &_load);          // get the next lobby service Id for the certain FID
	bool		AddFamilyWithLoad(TServiceId _tsid, T_FAMILYID _uFID, const T &_load);// 부하를 주는 가족을 추가 한다
	void		RemoveFamily(T_FAMILYID _uFID);					// 부하를 주던 가족을 삭제한다
	TServiceId	GetSIDFromFID(T_FAMILYID _uFID);				// 가족이 주는 부하를 담당한 써비스얻기
	TServiceId	GetBestServiceID();								// 부하가 제일적은 써비스를 얻는다
	TServiceId	GetNewServiceIDForRoomID(uint32 index);			// 방이 새로 창조될때 창조되여야 할 써비스를 얻는다.
	TServiceId	GetMasterServiceID();							// 주기능써비스를 얻는다
	void		SendAllService(CMessage	&msgout);								// Send Packet To all Service //  [3/20/2010 KimSuRyon]
	int			GetOrderSIDCount() { return m_orderSID.size(); }

protected:
	string						m_strServiceName;				// 써비스짧은 이름
	map<T_FAMILYID, TServiceId>	m_mapLoadForFamily;				// 가족ID를 키로하여 해당한 부하요소를 얻는다
	vector<TServiceId>			m_orderSID;						// 기능써비스들의 순서렬
	vector<TServiceId>          m_LobbySID;                     // the list of all the lobby service Id
};

/************************************************************************/
//	새로운 기능써비스 추가
/************************************************************************/
template<typename T>
void	CMultiService<T>::AddService(TServiceId _tsid)
{
	// 써비스ID순차대로 정리한다
	uint16	serviceid = _tsid.get();
	bool	bInsert = false;
	vector<TServiceId>::iterator orderit;
	for (orderit = m_orderSID.begin(); orderit != m_orderSID.end(); orderit++)
	{
		uint16 curid = (*orderit).get();
		if (serviceid < curid)
		{
			bInsert = true;
			m_orderSID.insert(orderit, _tsid);
			break;
		}
	}
	if (!bInsert)
		m_orderSID.push_back(_tsid);
}
/************************************************************************/
// 기능써비스 삭제
/************************************************************************/
template<typename T>
void	CMultiService<T>::RemoveService(TServiceId _tsid)
{
	vector<TServiceId>::iterator orderit;
	for (orderit = m_orderSID.begin(); orderit != m_orderSID.end(); orderit++)
	{
		if(*orderit == _tsid)
		{
			m_orderSID.erase(orderit);
			break;
		}
	}

	vector<T_FAMILYID>	temp;
	map<T_FAMILYID, TServiceId>::iterator it;
	for (it = m_mapLoadForFamily.begin(); it != m_mapLoadForFamily.end(); it++)
	{
		if (it->second == _tsid)
			temp.push_back(it->first);
	}
	vector<T_FAMILYID>::iterator tempit;
	for (tempit = temp.begin(); tempit != temp.end(); tempit++)
		m_mapLoadForFamily.erase(*tempit);
}

/************************************************************************/
// add the serviceId to the lobby serviceId list
/************************************************************************/
template<typename T>
void	CMultiService<T>::AddLobbyService(TServiceId _tsid)
{
	uint16	serviceid = _tsid.get();
	bool	bInsert = false;
	vector<TServiceId>::iterator orderit;
	for (orderit = m_LobbySID.begin(); orderit != m_LobbySID.end(); orderit++)
	{
		uint16 curid = (*orderit).get();
		if (serviceid < curid)
		{
			bInsert = true;
			m_LobbySID.insert(orderit, _tsid);
			break;
		}
	}
	if (!bInsert)
		m_LobbySID.push_back(_tsid);
}

/************************************************************************/
// erase the serviceId from the lobby serviceId list
/************************************************************************/
template<typename T>
void	CMultiService<T>::RemoveLobbyService(TServiceId _tsid)
{
	vector<TServiceId>::iterator orderit;
	for (orderit = m_LobbySID.begin(); orderit != m_LobbySID.end(); orderit++)
	{
		if(*orderit == _tsid)
		{
			m_LobbySID.erase(orderit);
			break;
		}
	}

	/*vector<T_FAMILYID>	temp;
	map<T_FAMILYID, TServiceId>::iterator it;
	for (it = m_mapLoadForFamily.begin(); it != m_mapLoadForFamily.end(); it++)
	{
		if (it->second == _tsid)
			temp.push_back(it->first);
	}
	vector<T_FAMILYID>::iterator tempit;
	for (tempit = temp.begin(); tempit != temp.end(); tempit++)
		m_mapLoadForFamily.erase(*tempit);*/
}


/************************************************************************/
/************************************************************************/
// 임의의 써비스를 얻는다
/************************************************************************/
template<typename T>
TServiceId	CMultiService<T>::GetBestServiceID()
{
	TServiceId	candiID(0);
	int		count = m_orderSID.size();
	if (count < 1)
		return candiID;

	return m_orderSID[rand() % count];
}

/************************************************************************/
// 임의의 써비스를 얻는다
/************************************************************************/
template<typename T>
TServiceId	CMultiService<T>::GetNewServiceIDForRoomID(uint32 index)
{
	TServiceId	candiID(0);
	int		count = m_orderSID.size();
	if (count < 1)
		return candiID;

	return m_orderSID[index % count];
}

/************************************************************************/
// 주기능 써비스를 얻는다
/************************************************************************/
template<typename T>
TServiceId	CMultiService<T>::GetMasterServiceID()
{
	TServiceId	candiID(0);
	int		count = m_orderSID.size();
	if (count < 1)
		return candiID;

	return m_orderSID[0];
}

/************************************************************************/
// 부하에 적합한 기능써비스ID를 얻는다
/************************************************************************/
//template<typename T>
//TServiceId	CMultiService<T>::GetBestServiceID(const T &_load)
//{
//	TServiceId	candiID(0);
//	if (m_orderSID.size() < 1)
//		return candiID;
//	uint32	nSNum = m_orderSID.size();
//	uint32 nR = _load % nSNum;
//	return m_orderSID[nR];
//}

/************************************************************************/
// get the lobby service Id for the certain load
/************************************************************************/
template<typename T>
TServiceId	CMultiService<T>::GetNextLobbySIDForFID(const T &_load)
{
	TServiceId	candiID(0);
	if ( m_LobbySID.size() < 1 )
		return candiID;
	uint32	nSNum = m_LobbySID.size();
	uint32 nR = _load % nSNum;
	return m_LobbySID[nR];
}

/************************************************************************/
// 부하를 추가한다
/************************************************************************/
template<typename T>
bool	CMultiService<T>::AddFamilyWithLoad(TServiceId _tsid, T_FAMILYID _uFID, const T &_load)
{
	// 이미 캐릭이 부하를 주고 있는가 검사
	map<T_FAMILYID, TServiceId>::iterator it = m_mapLoadForFamily.find(_uFID);
	if (it != m_mapLoadForFamily.end())
		m_mapLoadForFamily.erase(_uFID);
	
	m_mapLoadForFamily.insert(make_pair(_uFID, _tsid));
	return true;
}

/************************************************************************/
// 부하를 주던 캐릭을 삭제한다
/************************************************************************/
template<typename T>
void	CMultiService<T>::RemoveFamily(T_FAMILYID _uFID)
{
	map<T_FAMILYID, TServiceId>::iterator it = m_mapLoadForFamily.find(_uFID);
	if (it != m_mapLoadForFamily.end())
		m_mapLoadForFamily.erase(_uFID);
}

/************************************************************************/
// 캐릭이 주는 부하를 담당한 써비스얻기
/************************************************************************/
template<typename T>
TServiceId	CMultiService<T>::GetSIDFromFID(uint32 _uFID)
{
	map<T_FAMILYID, TServiceId>::iterator it = m_mapLoadForFamily.find(_uFID);
	if (it == m_mapLoadForFamily.end())
		return TServiceId(0);
	return it->second;
}

template<typename T>
void CMultiService<T>::SendAllService(SIPNET::CMessage &msgout)
{
	vector<TServiceId>::iterator orderit;
	for (orderit = m_orderSID.begin(); orderit != m_orderSID.end(); orderit++)
	{
		TServiceId	sid = (*orderit);
		CUnifiedNetwork::getInstance()->send(sid, msgout);
	}
}

#endif	// TIAN_FRONTEND_MULTISERVICE_H
