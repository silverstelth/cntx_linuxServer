/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CRYPTSYSTEMMANAGER_H__
#define __CRYPTSYSTEMMANAGER_H__

#include "misc/types_sip.h"
#include "misc/CryptMethod.h"
#include <map>

namespace SIPBASE
{
typedef	std::map<uint32, ICryptMethodType*> TMapCryptMethodType;
typedef	std::map<uint32, ICryptMethod*>		TMapCryptMethod;

class CCryptMethodTypeManager
{
	SIPBASE_SAFE_SINGLETON_DECL(CCryptMethodTypeManager);
public:
	void			RegisterCryptMethodType(const ICryptMethodType* _pMethodType);
	void			UnRegisterCryptMethodType(const ICryptMethodType* _pMethodType);

	ICryptMethod*	MakeCryptMethodObject(uint32 _TypeID);
	void			MakeAllCryptMethodObject(OUT TMapCryptMethod& _rMethodBuf);
	uint32			GetRandomCryptMethodIDOnlevel(E_CRYPTLEVEL _CryptLevel);

private:
	TMapCryptMethodType			m_mapCryptMethodType;
	
	CCryptMethodTypeManager();
	~CCryptMethodTypeManager();
};
	
} // namespace SIPBASE
#endif // end of guard __CRYPTSYSTEMMANAGER_H__
