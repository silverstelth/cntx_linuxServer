/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include	"stdnet.h"

#include	"misc/debug.h"
#include	"misc/factory.h"

//#include	"misc/path.h"
//#include	"misc/string_common.h"
#include	"net/CryptEngine.h"
#include	"net/SecurityInfo.h"
#include	"net/Key.h"

using namespace SIPBASE;
using namespace SIPNET;

CCryptSystem::TAvailableEngines	CCryptSystem::m_availableEngine;

//CVariable<bool>	UseSecurity("security", "UseSecurity", "whether if we use secret algorithm, false : no use, true : use", false, 0, true);
CVariable<unsigned int>	ExchangeAlgorithm("security", "ExchangeAlgorithm", "whether if we can exchange secret algorithm, 0 : can't exchange, 1 : can exchange", 0, 0, true);
CVariable<std::string>	DefaultMode("security", "DefaultMode", "Default crypt encode/decode mode", "NON", 0, true);

namespace SIPNET
{
CCryptSystem::CCryptSystem(void)
{
}

CCryptSystem::~CCryptSystem(void)
{
}	

void	CCryptSystem::init()
{
	CSecurityInfo & cryptInfo = CSecurityInfo::getInstance();

	if ( !cryptInfo.IsInit() )
		return;
	
	init(cryptInfo.getSymInfo());
	init(cryptInfo.getASymInfo());
	init(cryptInfo.getKeyGenerateInfo());
	init(cryptInfo.getWrapInfo());
}

void	CCryptSystem::init(const TCRYPT_INFO &info)
{
	if ( info.methodID == NON_CRYPT )
	{
		throw	ESecurity("Invalid Security Configuration");
		return;
	}
	ICryptable * dynTask = getInstance(info);

	if ( !dynTask )
	{
		throw	ESecurity("Invalid Security Configuration");
		return;
	}

	dynTask->init();
	sipdebug("CryptEngine is initialized! METHOD : %s", g_strMethod[info.methodID]);
}

ICryptable	* CCryptSystem::getInstance(const TCRYPT_INFO & info)
{
	ICryptable * dynTask;
    ICryptable::TCtorParam param;
	param.info = info;

	// method -> m_dynTask
	TAvailableEngines::iterator it = m_availableEngine.find(info.methodID);
	if ( it != m_availableEngine.end() )
		return	(*it).second;

	if ( info.methodID != NON_CRYPT)
		dynTask = SIPBASE_GET_FACTORY(ICryptable, TCRYPTMODE).createObject(info.methodID, param);

	if ( dynTask == NULL )
	{
		throw	ESecurity(SIPBASE::toString("STOP!: unknown cryptmode(%u)!, dynamic created task is unavailable", info.methodID).c_str());
	}

#ifdef _DEBUG
	sipassert(dynTask != NULL);
#else
	if ( dynTask == NULL )
	{
		sipwarning("STOP!: unknown cryptmode(%u)!, dynamic created task is unavailable", info.methodID);
		return NULL;
	}
#endif

	registerMethod(info.methodID, dynTask);

	return dynTask;
}

bool	CCryptSystem::IsAsymMode(TCRYPTMODE mode)
{
	if ( ASYM <= mode && mode <= ASYM_END )
		return	true;

	return	false;
}

bool	CCryptSystem::IsSymMode(TCRYPTMODE mode)
{
	if ( SYM <= mode && mode <= SYM_END )
		return	true;

	return	false;
}

bool	CCryptSystem::IsKeyGenerateMode(TCRYPTMODE mode)
{
	if ( KEYGENERATOR <= mode && mode <= KEYGENERATOR_END )
		return	true;

	return	false;
}

void	CCryptSystem::release()
{

}

void	CCryptSystem::resetSymContext(TSockId hostid)
{
	ICryptable * algo = getInstance(CSecurityInfo::getInstance().getCryptoInfo(SYM));
	if ( !algo )
		throw	ESecurity("unknown cryptmode!, dynamic created task is unavailable");

	PKEY_INFO key = CKeyManager::getInstance().getSymKey(hostid);
	if ( key == NULL || key->secretkeySize == 0 )
		throw	ESecurityKeyGenerator("Error : Not prepared Key Context!");
	_SymCryptable * sym = dynamic_cast<_SymCryptable *> (algo);
	sym->setSecretKey(key->secretKey, key->secretkeySize);
}

void	CCryptSystem::resetAsymContext(TSockId hostid)
{
	ICryptable * algo = getInstance(CSecurityInfo::getInstance().getCryptoInfo(ASYM));
	if ( !algo )
		throw	ESecurity("unknown cryptmode!, dynamic created task is unavailable");

	PKEY_INFO key = CKeyManager::getInstance().getAsymKey(hostid);
	if ( key == NULL || key->pubkeySize == 0 )
		throw	ESecurityKeyGenerator("Error : Not prepared Key Context!");
	_AsymCryptable * asym = dynamic_cast<_AsymCryptable *> (algo);
	asym->setPubKeyInfo(key->pubKey, key->pubkeySize);
}

void	CCryptSystem::encode(TCRYPTMODE method, TSockId hostid, unsigned char * ucText,		unsigned long text_len, 
					   unsigned char * ucEncText,	unsigned long &enc_len, bool bWrap)
{
// 	if ( bWrap )
// 	{
// 		bWrap = false;
// 		encode(METHOD_WRAP, hostid, ucText, text_len, ucEncText, enc_len, bWrap);
// 	}

	ICryptable * algo = getInstance(CSecurityInfo::getInstance().getCryptoInfo(method));
	if ( !algo )
		throw	ESecurity("unknown cryptmode!, dynamic created task is unavailable");


	if ( IsSymMode(method) )
		resetSymContext(hostid);

	algo->encode(ucText, text_len, ucEncText, enc_len);
}

void	CCryptSystem::decode(TCRYPTMODE method, TSockId hostid, unsigned char * ucEncText,	unsigned long enc_len, 
							 unsigned char * ucDecText,	unsigned long &dec_len, bool bWrap)
{
	ICryptable * algo = getInstance(CSecurityInfo::getInstance().getCryptoInfo(method));
	if ( !algo )
		throw	ESecurity("unknown cryptmode!, dynamic created task is unavailable");


	if ( IsSymMode(method) )
		resetSymContext(hostid);

	algo->decode(ucEncText, enc_len, ucDecText, dec_len);

	sipdebug("DecodeText %s, len %d", ucDecText, dec_len);

// 	if ( bWrap )
// 	{
// 		bWrap = false;
// 		decode(METHOD_WRAP, hostid, ucEncText, enc_len, ucDecText, dec_len, bWrap);
// 	}
}

void	CCryptSystem::registerMethod(TCRYPTMODE method, ICryptable * algor)
{
	if ( algor == NULL )
	{
		sipwarning("Created task is unavailable!");
		return;
	}
	TAvailableEngines::iterator	it = m_availableEngine.find(method);
	if ( it == m_availableEngine.end() )
        m_availableEngine.insert(std::make_pair(method, algor));
}

} // namespace SIPNET

