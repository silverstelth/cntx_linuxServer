/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CRYPTENGINE_H__
#define __CRYPTENGINE_H__

#include	"buf_net_base.h"
//#include	"net/Key.h"
#include	"misc/variable.h"

#include	"net/buf_net_base.h"
#include	"net/SecurityInfo.h"
#include	"net/CryptTask.h"

typedef		const unsigned char *	T_TEXT;

/*
template<class T>
class CCryptoSystem
{
};

template<class T>
class CEncoder
{
};

template<class T>
class CDecoder
{
};
*/

namespace SIPNET
{

class CCryptSystem // need refactoring, rethink to registry
{
public:
	static	ICryptable	* getInstance(const TCRYPT_INFO & info);
	static	void	init();
	static	void	init(const TCRYPT_INFO &info);
	static	void	release();

	static	void	encode(TCRYPTMODE method, SIPNET::TSockId hostid, unsigned char * ucText,		unsigned long text_len, 
							unsigned char * ucEncText,	unsigned long &enc_len, bool bWrap = true);
	static	void	decode(TCRYPTMODE method, SIPNET::TSockId hostid, unsigned char * ucEncText,	unsigned long enc_len, 
							unsigned char * ucDecText,	unsigned long &dec_len, bool bWrap = true);
	static	void	registerMethod(TCRYPTMODE method, ICryptable * algor);
	static	void	setAlgorKey(TCRYPTMODE method);

	static	void	resetSymContext(SIPNET::TSockId hostid = SIPNET::InvalidSockId);
	static	void	resetAsymContext(SIPNET::TSockId hostid = SIPNET::InvalidSockId);

	static	bool	IsAsymMode(TCRYPTMODE mode);
	static	bool	IsSymMode(TCRYPTMODE mode);
	static	bool	IsKeyGenerateMode(TCRYPTMODE mode);

	CCryptSystem(void);
	~CCryptSystem(void);

protected:
	static KEY_INFO 	m_keyInfo;

	typedef	std::map<TCRYPTMODE, ICryptable *>		TAvailableEngines;
	static TAvailableEngines	m_availableEngine;

// 	typedef	std::map<SIPNET::TSockId, CKey*>		TKEYS;
// 	static TKEYS	m_keyArray;
};
} // namespace SIPNET

// extern	SIPBASE::CVariable<bool>		UseSecurity;
extern	SIPBASE::CVariable<unsigned int>	ExchangeAlgorithm;
extern	SIPBASE::CVariable<std::string>		DefaultMode;

#endif //__CRYPTENGINE_H__
