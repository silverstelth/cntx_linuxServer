/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "net/Key.h"
#include "net/SecurityInfo.h"
#include "net/CryptEngine.h"


//////////////////////////////////////////////////////////////////////////
// CKeyManager
//////////////////////////////////////////////////////////////////////////

#include "net/message.h"

namespace SIPNET
{

SIPBASE_SAFE_SINGLETON_IMPL(CKeyManager)
void	CKeyManager::init()
{
	clientKeys.clear();
}

void	CKeyManager::release()
{
	clientKeys.clear();
}

void	CKeyManager::insertKey(PKEY_INFO key, SIPNET::TSockId socket)
{
	clientKeys.insert(std::make_pair(socket, *key));
}

void	CKeyManager::removeKey(SIPNET::TSockId socket)
{
	clientKeys.erase(socket);
}

bool	CKeyManager::existCookie(SIPNET::TSockId socket)
{
	return	exist(KEYGENERATOR, socket);
}

bool	CKeyManager::exist(TCRYPTMODE	method, SIPNET::TSockId socket)
{
	TClientKeys::const_iterator it;
	if ( socket != SIPNET::InvalidSockId )
	{
		it = clientKeys.find(SIPNET::InvalidSockId);
		if ( it != clientKeys.end() )
			return true;
	}

	it = clientKeys.find(socket);
	if ( it == clientKeys.end() )
		return false;

	return true;
}

PKEY_INFO	CKeyManager::getKey(TCRYPTMODE	method, SIPNET::TSockId socket)
{
	PKEY_INFO key;

	if ( socket != SIPNET::InvalidSockId )
	{
		key = getKey(method, SIPNET::InvalidSockId);
        if ( key != NULL )
			return key;
	}

	TClientKeys::iterator		begin, end;

	begin	= clientKeys.lower_bound(socket);
	end		= clientKeys.upper_bound(socket);

	bool	bfind = false;

	while(begin != end)
	{
		TCRYPTMODE m = begin->second.method;

		if ( m == method )
		{
			bfind = true;
			break;
		}

		*begin ++;
	}

	if ( begin == end )
		return	NULL;

	key = &(begin->second);

	return key;
}

CSecretKey	CKeyManager::generateCookieKey(SIPNET::TSockId socket, bool bRegister)
{
	CSecretKey key;
	TCRYPT_INFO info = CSecurityInfo::getInstance().getKeyGenerateInfo();
	key.init(info);
	key.generateCookieKey();

	if ( bRegister )
	    insertKey(key.getKey(), socket);

	return	key;
}

bool	CKeyManager::checkPairKey(CSecretKey key1, CSecretKey key2)
{
	TCRYPT_INFO info = CSecurityInfo::getInstance().getKeyGenerateInfo();

	key1.init(info);
	key1.checkPairKey(key2);

	return	true;
}

CSecretKey	CKeyManager::generatePairKey(CSecretKey key1)
{
	CSecretKey key2;
	TCRYPT_INFO info = CSecurityInfo::getInstance().getKeyGenerateInfo();
	key2.init(info);
	key2.generatePairKey(key1);
//	memcpy(key2._pKeyInfo, info._pKeyInfo, 5);

//	key1 = CKeyManager::generatePairKey(key2);
	return	key2;
}

PKEY_INFO	CKeyManager::getAsymKey(SIPNET::TSockId socket)
{
	TCRYPT_INFO info = CSecurityInfo::getInstance().getASymInfo();
	return	getKey(info.methodID, socket);
}

PKEY_INFO	CKeyManager::getSymKey(SIPNET::TSockId socket)
{
	TCRYPT_INFO info = CSecurityInfo::getInstance().getSymInfo();
	return	getKey(info.methodID, socket);
}

void	CKeyManager::registerKey(PKEY_INFO key, SIPNET::TSockId socket)
{
	insertKey(key, socket);
}

void	CKeyManager::unregisterKey(SIPNET::TSockId socket)
{
	removeKey(socket);
}

CSecretKey	CKeyManager::generateSymKey(SIPNET::TSockId socket, bool bRegister)
{
	TCRYPT_INFO info = CSecurityInfo::getInstance().getSymInfo();

	CSecretKey	key;
	key.init(info);
	key.generateKey(info);

	if ( bRegister )
	    insertKey(key.getKey(), socket);

	return	key;
}

CSecretKey	CKeyManager::generateAsymKey(SIPNET::TSockId socket, bool bRegister)
{
	TCRYPT_INFO info = CSecurityInfo::getInstance().getASymInfo();

	CSecretKey	key;
	key.init(info);
	key.generateKey(info);

	if ( bRegister )
	    insertKey(key.getKey(), socket);

	return	key;
}

CKeyManager::CKeyManager()
{
}

CKeyManager::~CKeyManager()
{
}

} //namespace SIPNET