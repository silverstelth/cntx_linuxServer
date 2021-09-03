/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __KEY_H__
#define __KEY_H__

#include	"net/buf_net_base.h"
#include	"net/SecurityInfo.h"
#include	"net/CryptTask.h"
#include	"net/CryptEngine.h"

namespace SIPNET
{
template <class TAlgor>
class CKey :public TAlgor
{
	TAlgor		*_Algorithm;

public:
	void	serial (SIPBASE::IStream &s);
	bool	isValid() const { return _Valid; }

	void	init(TCRYPT_INFO info);

	void	generateKey(TCRYPT_INFO info);
	void	generateCookieKey();
	bool	checkPairKey(CKey<TAlgor> keyNew);
	CKey	generatePairKey(CKey<TAlgor> keyOld);


	void	setAlgorithm(ICryptable * palgo){ _Algorithm = palgo; };
	ICryptable *	getAlgorithm(){ return	_Algorithm; };
  
	PKEY_INFO	getKey() { return _pKeyInfo; };
	uint8 *	getPubKey();
	uint32	getPubKeySize();
	uint8 *	getPrivKey();
	uint32	getPrivKeySize();
	uint8 *	getSecretKey();
	uint32	getSecretKeySize();

	void	setPubKey(uint8 * key, uint32 len);
	void	setPrivKey(uint8 * key, uint32 len);
	void	setSecretKey(uint8 * key, uint32 len);

	CKey<TAlgor>();
	CKey<TAlgor>(const KEY_INFO	&info);	

	CKey(const CKey<TAlgor> &info);
	virtual ~CKey(void);

	template<class T>
	friend void operator== (const CKey<T> &k1, const CKey<T> &k2);

	bool operator== (const CKey<TAlgor> &other) const;
	bool operator< (const CKey<TAlgor> &other) const;
	CKey<TAlgor> &operator= (const CKey<TAlgor> &other);

private:
	bool		_Valid;
	uint64		_TimeStamp;

	PKEY_INFO	_pKeyInfo;
};

typedef		CKey<ICryptable>		CSecretKey;
typedef		CKey<_SymCryptable>		CSymKey;
typedef		CKey<_AsymCryptable>	CAsymKey;

typedef		CKey<_RSA>				CKeyRSA;
typedef		CKey<_DES>				CKeyDES;
typedef		CKey<_MD>				CKeyMD;
typedef		CKey<_MSGWrap>			CKeyMSGWrap;

typedef		CKey<_CookieKey>		CKeyCheck;
typedef		CKey<_sslRanKeyGenerator>	CKeyCookie;

template <class TAlgor>
CKey<TAlgor>::CKey()
{
	_pKeyInfo = new KEY_INFO;
	if ( _pKeyInfo )
		// changed by ysg 2009-05-18
//		_pKeyInfo->method = AlgorithmType();
		_pKeyInfo->method = TAlgor::AlgorithmType();
}

template <class TAlgor>
CKey<TAlgor>::CKey(const CKey<TAlgor> &info)
{
	_pKeyInfo = new KEY_INFO;
	memcpy(_pKeyInfo, info._pKeyInfo, sizeof(KEY_INFO));
}

template <class TAlgor>
CKey<TAlgor>::~CKey(void)
{
	if ( _pKeyInfo )
		delete	_pKeyInfo;
}

template <class TAlgor>
void CKey<TAlgor>::init(TCRYPT_INFO info)
{
	ICryptable * ptask = CCryptSystem::getInstance(info);
	if ( ptask == NULL )
	{
		sipinfo("unknown cryptmode!, dynamic created task is unavailable");
		return;
	}

	setAlgorithm(CCryptSystem::getInstance(info));
	TAlgor::setCryptInfo(info);
	_pKeyInfo->method = info.methodID;
}

template <class TAlgor>
void CKey<TAlgor>::serial(SIPBASE::IStream &s)
{
	// verify that we initialized the cookie before writing it
	if (!s.isReading() && !_Valid) sipwarning ("LC: serialize a non valid key");

	if ( _pKeyInfo->method >= ASYM )
	{
		s.serial (_pKeyInfo->pubkeySize);
		s.serialBuffer (_pKeyInfo->pubKey, _pKeyInfo->pubkeySize);
		s.serial (_pKeyInfo->prikeySize);
		s.serialBuffer (_pKeyInfo->priKey, _pKeyInfo->prikeySize);
	}
	else
	{
		s.serial (_pKeyInfo->secretkeySize);
		s.serialBuffer (_pKeyInfo->secretKey, _pKeyInfo->secretkeySize);
	}

	if ( s.isReading() )
		_Valid = false;
}

template <class TAlgor>
void	CKey<TAlgor>::generateKey(TCRYPT_INFO info)
{
	_Algorithm->generateKey(*_pKeyInfo);

	_TimeStamp = SIPBASE::CTime::getCurrentTime();
}

template <class TAlgor>
void	CKey<TAlgor>::generateCookieKey()
{
	KEY_INFO	key;

	TCRYPT_INFO info = CSecurityInfo::getInstance().getKeyGenerateInfo();
	init(info);

	_Algorithm->generateKey(key);

	setSecretKey(key.secretKey, key.secretkeySize);
	
	_TimeStamp = SIPBASE::CTime::getCurrentTime();
}

template <class TAlgor>
bool	CKey<TAlgor>::checkPairKey(CKey<TAlgor> keyNew)
{
	uint	keyLen = keyNew.getSecretKeySize();
	autoPtr<uint8>	pairKey(new uint8[getSecretKeySize()]);

	_Algorithm->decode(keyNew.getSecretKey(), keyLen, pairKey.get(), (unsigned long&)keyLen);

	setSecretKey(pairKey.get(), keyLen);

	if ( memcmp( getSecretKey(), keyNew.getSecretKey(), keyNew.getSecretKeySize() ) == 0 )
		return	true;

	return	false;
}

template <class TAlgor>
CKey<TAlgor>	CKey<TAlgor>::generatePairKey(CKey<TAlgor> keyOld)
{
	uint	keyLen = keyOld.getSecretKeySize();
	autoPtr<uint8>	pairKey(new uint8[getSecretKeySize()]);

	//typeid(TAlgor)::encode(keyOld.getSecretKey(), keyLen, pairKey.get(), keyLen);
	_Algorithm->encode(getSecretKey(), keyLen, pairKey.get(), (unsigned long&)keyLen);

	setSecretKey(pairKey.get(), keyLen);

	return	(*this);
}

template <class T>
bool	operator== (const CKey<T> &k1, const CKey<T> &k2)
{
	if ( memcmp( k1.getPubKey(), k2.getPubKey(), k1.pubkeySize ) != 0 )
		return	false;

	if ( memcmp( k1.getPrivKey(), k2.getPrivKey(), k1.prikeySize ) != 0 )
		return	false;

	if ( memcmp( k1.getSecretKey(), k2.getSecretKey(), k1.secretkeySize ) != 0 )
		return	false;

	return true;
}

template <class TAlgor>
bool	CKey<TAlgor>::operator== (const CKey<TAlgor> &other) const
{
	if ( memcmp( getPubKey(), other.getPubKey(), getPubKeySize() ) != 0 )
		return	false;

	if ( memcmp( getPrivKey(), other.getPrivKey(), other.prikeySize() ) != 0 )
		return	false;

	if ( memcmp( getSecretKey(), other.getSecretKey(), other.secretkeySize() ) != 0 )
		return	false;

	return true;
}

template <class TAlgor>
bool	CKey<TAlgor>::operator< (const CKey<TAlgor> &other) const
{
	return ( _TimeStamp < other._TimeStamp );
}

template <class TAlgor>
CKey<TAlgor> &	CKey<TAlgor>::operator= (const CKey<TAlgor> &other)
{
	if ( this == &other )
		return	*this;
	
	delete	_pKeyInfo;
	_pKeyInfo = new KEY_INFO;
	memcpy(_pKeyInfo, other._pKeyInfo, sizeof(KEY_INFO));

	return	*this;
}

template <class TAlgor>
uint8 * CKey<TAlgor>::getPubKey()
{
	if ( !_pKeyInfo )
		return NULL;
    
	return _pKeyInfo->pubKey;
};

template <class TAlgor>
uint32	CKey<TAlgor>::getPubKeySize()
{
	if ( !_pKeyInfo )
		return 0;

    return _pKeyInfo->pubkeySize;
}

template <class TAlgor>
uint8 * CKey<TAlgor>::getPrivKey()
{
	if ( !_pKeyInfo )
		return NULL;
    
	return _pKeyInfo->priKey;
}

template <class TAlgor>
uint32	CKey<TAlgor>::getPrivKeySize()
{
	if ( !_pKeyInfo )
		return 0;
    
	return _pKeyInfo->prikeySize;
}

template <class TAlgor>
uint8 *	CKey<TAlgor>::getSecretKey()
{
	if ( !_pKeyInfo )
		return NULL;

    return _pKeyInfo->secretKey;
}

template <class TAlgor>
uint32	CKey<TAlgor>::getSecretKeySize()
{
	if ( !_pKeyInfo )
		return	-1;

    return _pKeyInfo->secretkeySize;
}

template <class TAlgor>
void	CKey<TAlgor>::setPubKey(uint8 * key, uint32 len)
{
	_pKeyInfo->pubkeySize = len;
	if ( !_pKeyInfo )
		return;
    
	memcpy(_pKeyInfo->pubKey, key, len);    
}

template <class TAlgor>
void	CKey<TAlgor>::setPrivKey(uint8 * key, uint32 len)
{
	_pKeyInfo->prikeySize = len;
	if ( !_pKeyInfo )
		return;

	memcpy(_pKeyInfo->priKey, key, len);    
}

template <class TAlgor>
void	CKey<TAlgor>::setSecretKey(uint8 * key, uint32 len)
{
	_pKeyInfo->secretkeySize = len;
	if ( !_pKeyInfo )
		return;

    memcpy(_pKeyInfo->secretKey, key, len);    
}

// singleton KeyManager
class CKeyManager
{

SIPBASE_SAFE_SINGLETON_DECL(CKeyManager)
public:
	void		init();
	void		release();

	bool		existCookie(SIPNET::TSockId socket = SIPNET::InvalidSockId);
	bool		exist(TCRYPTMODE	method, SIPNET::TSockId socket = SIPNET::InvalidSockId);

	bool		checkPairKey(CSecretKey key1, CSecretKey key2);
	CSecretKey	generatePairKey(CSecretKey key1);

	// in most case, socket must Not be <socket = InvalidSockId>
	CSecretKey	generateCookieKey(SIPNET::TSockId socket, bool bRegister = true);
	// in server, may be <socket = InvalidSockId> for common key of all client
	// in client, must Not be <socket = InvalidSockId>
	CSecretKey	generateSymKey(SIPNET::TSockId socket = SIPNET::InvalidSockId, bool bRegister = true);
	// in server, may be <socket = InvalidSockId> for common key of all client
	// in client, must Not be <socket = InvalidSockId>
	CSecretKey	generateAsymKey(SIPNET::TSockId socket = SIPNET::InvalidSockId, bool bRegister = true);

	void		registerKey(PKEY_INFO key, SIPNET::TSockId socket = SIPNET::InvalidSockId);
	void		unregisterKey(SIPNET::TSockId socket);

	PKEY_INFO	getKey(TCRYPTMODE	method, SIPNET::TSockId socket = SIPNET::InvalidSockId);
	PKEY_INFO	getAsymKey(SIPNET::TSockId socket = SIPNET::InvalidSockId);
	PKEY_INFO	getSymKey(SIPNET::TSockId socket = SIPNET::InvalidSockId);

protected:
	// INVALID_SOCKET
	// server : Only use one key for all clients
	// client : always be INVALID_SOCKET!
	void		insertKey(PKEY_INFO key, SIPNET::TSockId socket = SIPNET::InvalidSockId);
	void		removeKey(SIPNET::TSockId socket = SIPNET::InvalidSockId);

	CKeyManager();
	~CKeyManager();

protected:
	// Server use this structure : if TSockId = INVALID_SOCKET -> use same key about all clients
	typedef		std::multimap<SIPNET::TSockId, KEY_INFO>	TClientKeys;
	TClientKeys	clientKeys;

	// Client use this structure
	typedef		std::multimap<TCRYPTMODE, PKEY_INFO>		TModeKeys;
	TModeKeys	serverKeys;
};


} // namespace SIPNET
#endif // __KEY_H__
