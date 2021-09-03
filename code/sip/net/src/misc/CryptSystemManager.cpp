/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include <time.h>
#include "misc/CryptSystemManager.h"

using namespace std;
namespace SIPBASE
{

SIPBASE_SAFE_SINGLETON_IMPL(CCryptMethodTypeManager);

#define REGISTER_CRYPTMETHOD(idType, methodClass)	\
	CCryptMethodType<methodClass>	*CryptMethod_##methodClass = new CCryptMethodType<methodClass>(idType); \
	RegisterCryptMethodType(CryptMethod_##methodClass);

CCryptMethodTypeManager::CCryptMethodTypeManager()
{
	REGISTER_CRYPTMETHOD(1, CCryptMethodSimpleXor1);

	REGISTER_CRYPTMETHOD(2, CCryptMethodMediumXor);
}

CCryptMethodTypeManager::~CCryptMethodTypeManager()
{
	TMapCryptMethodType::iterator it;
	for (it = m_mapCryptMethodType.begin(); it != m_mapCryptMethodType.end(); it++)
	{
		ICryptMethodType* pType = it->second;
		delete pType;
	}
	m_mapCryptMethodType.clear();
}

void	CCryptMethodTypeManager::RegisterCryptMethodType(const ICryptMethodType* _pMethodType)
{
	uint32 typeID = _pMethodType->GetTypeId();
	TMapCryptMethodType::iterator it = m_mapCryptMethodType.find(typeID);
	if (it == m_mapCryptMethodType.end())
	{
		m_mapCryptMethodType.insert( std::make_pair(typeID, const_cast<ICryptMethodType *>(_pMethodType)) );
	}
	else
		siperrornoex("Double register crypt method type");
}

void	CCryptMethodTypeManager::UnRegisterCryptMethodType(const ICryptMethodType* _pMethodType)
{
	uint32 typeID = _pMethodType->GetTypeId();
	m_mapCryptMethodType.erase(typeID);
}

ICryptMethod*	CCryptMethodTypeManager::MakeCryptMethodObject(uint32 _TypeID)
{
	TMapCryptMethodType::iterator it = m_mapCryptMethodType.find(_TypeID);
	if (it == m_mapCryptMethodType.end())
		return NULL;
	ICryptMethodType* pMethodType = it->second;
	
	ICryptMethod* pNew = pMethodType->MakeNewCryptMethodObject();

	return pNew;
}

void	CCryptMethodTypeManager::MakeAllCryptMethodObject(OUT TMapCryptMethod& _rMethodBuf)
{
	TMapCryptMethodType::iterator it;
	for (it = m_mapCryptMethodType.begin(); it != m_mapCryptMethodType.end(); it++)
	{
		uint32				typeID = it->first;
		ICryptMethodType*	pMethodType = it->second;
		ICryptMethod*		pNew = pMethodType->MakeNewCryptMethodObject();
		
		_rMethodBuf.insert(std::make_pair(typeID, pNew));
	}
}

uint32	CCryptMethodTypeManager::GetRandomCryptMethodIDOnlevel(E_CRYPTLEVEL _CryptLevel)
{
	vector<uint32>	candiMethod;
	TMapCryptMethodType::iterator it;
	for (it = m_mapCryptMethodType.begin(); it != m_mapCryptMethodType.end(); it++)
	{
		uint32	methodID = it->first;
		ICryptMethodType* cm = it->second;
		if (_CryptLevel == cm->GetCryptLevel())
			candiMethod.push_back(methodID);
	}

	int	nNumber = candiMethod.size();
	if (nNumber == 0)
		return 0;

	srand ((uint32)time (NULL));
	uint32 candiPos = SIPBASE::Irand(0, nNumber-1);
	return candiMethod[candiPos];
}


} // namespace SIPBASE
