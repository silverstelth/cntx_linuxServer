/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include	"stdnet.h"
#include	"net/SecurityInfo.h"
#include	"net/CryptEngine.h"


using namespace SIPBASE;
using namespace SIPNET;


namespace SIPNET
{

SIPBASE_SAFE_SINGLETON_IMPL(CSecurityInfo)

CSecurityInfo::CSecurityInfo(void)
	:m_bInit(false), m_defMode(NON_CRYPT)
{
}

CSecurityInfo::~CSecurityInfo(void)
{
}

TCRYPT_INFO		CSecurityInfo::getCryptoInfo(TCRYPTMODE mode)
{
	sipassert(mode != NON_CRYPT);
	if ( CCryptSystem::IsAsymMode(mode) )
		return m_asymInfo;
	if ( CCryptSystem::IsSymMode(mode) )
		return m_symInfo;

	return m_torkenInfo;
}

void	CSecurityInfo::initSecurity(SIPBASE::CConfigFile &config)
{
	m_bInit = true;

	m_symInfo			= TCRYPT_INFO(METHOD_DES,	"DES",	1,	2,	3,	"",	"des");
	m_asymInfo			= TCRYPT_INFO(METHOD_RSA,	"RSA",	0,	1024,	1024,	"",	"pem");
	m_keyGenerateInfo	= TCRYPT_INFO(METHOD_RAND_TIME, "RAND-TIME",	0,	1,	1,	"",	"");
	m_torkenInfo		= TCRYPT_INFO(METHOD_WRAP,	"WRAP",	0,	1024,	1024,	"",	"torken");

	if ( config.exists("SymCryptInfo") )
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("SymCryptInfo");

		m_symInfo.strName		= varPtr->asString(METHOD_NAME);
		sipdebug("MethodName : %s", m_symInfo.strName.c_str());
		uint8	methodID= varPtr->asInt(METHOD_ID);
		m_symInfo.methodID	= (methodID == 0) ?  (TCRYPTMODE)strToMethod(m_symInfo.strName) : (TCRYPTMODE)methodID;
		if ( m_symInfo.methodID == DEFAULT || m_symInfo.methodID == -1 )
			siperror("Default mode error, unregistered security algorithm<%s>", m_symInfo.strName.c_str());

		m_symInfo.bKeyGenEach	= varPtr->asBool(KEY_GEN_EACHCALL);
		m_symInfo.bykeyLen	= varPtr->asInt(KEY_LEN);
		m_symInfo.bylimitPlain	= varPtr->asInt(PLAIN_LIMIT);
		m_symInfo.strKeyFileType	= varPtr->asString(KEY_FILENAME);
		m_symInfo.strKeyFileType	= varPtr->asString(KEY_FILETYPE);
	}

	if ( config.exists("AsymCryptInfo") )
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("AsymCryptInfo");

		m_asymInfo.strName	= varPtr->asString(METHOD_NAME);
		sipdebug("MethodName : %s", m_asymInfo.strName.c_str());
		uint8	methodID= varPtr->asInt(METHOD_ID);
		m_asymInfo.methodID	= (methodID == 0) ?  (TCRYPTMODE)strToMethod(m_asymInfo.strName) : (TCRYPTMODE)methodID;
		if ( m_asymInfo.methodID == DEFAULT || m_asymInfo.methodID == -1 )
			siperror("Default mode error, unregistered security algorithm<%s>", m_asymInfo.strName.c_str());
		m_asymInfo.bKeyGenEach= varPtr->asBool(KEY_GEN_EACHCALL);
		m_asymInfo.bykeyLen	= varPtr->asInt(KEY_LEN);
		m_asymInfo.bylimitPlain	= varPtr->asInt(PLAIN_LIMIT);
		m_asymInfo.strKeyFileType	= varPtr->asString(KEY_FILENAME);
		m_asymInfo.strKeyFileType	= varPtr->asString(KEY_FILETYPE);
	}

	if ( config.exists("KeyGenerater") )
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("KeyGenerater");

		m_keyGenerateInfo.strName	= varPtr->asString(METHOD_NAME);
		sipdebug("MethodName : %s", m_keyGenerateInfo.strName.c_str());
		uint8	methodID= varPtr->asInt(METHOD_ID);
		m_keyGenerateInfo.methodID	= (methodID == 0) ?  (TCRYPTMODE)strToMethod(m_keyGenerateInfo.strName) : (TCRYPTMODE)methodID;
		if ( m_keyGenerateInfo.methodID == DEFAULT || m_keyGenerateInfo.methodID == -1 )
			siperror("Default mode error, unregistered security algorithm<%s>", m_keyGenerateInfo.strName.c_str());

		m_keyGenerateInfo.bKeyGenEach= varPtr->asBool(KEY_GEN_EACHCALL);
		m_keyGenerateInfo.bykeyLen	= varPtr->asInt(KEY_LEN);
		m_keyGenerateInfo.bylimitPlain	= varPtr->asInt(PLAIN_LIMIT);
		m_keyGenerateInfo.strKeyFileType	= varPtr->asString(KEY_FILENAME);
		m_keyGenerateInfo.strKeyFileType	= varPtr->asString(KEY_FILETYPE);
	}

	if ( config.exists("WrapCryptInfo") )
	{
		CConfigFile::CVar *varPtr = config.getVarPtr("WrapCryptInfo");

		m_torkenInfo.strName	= varPtr->asString(METHOD_NAME);
		sipdebug("MethodName : %s", m_torkenInfo.strName.c_str());
		uint8	methodID= varPtr->asInt(METHOD_ID);
		m_torkenInfo.methodID	= (methodID == 0) ?  (TCRYPTMODE)strToMethod(m_torkenInfo.strName) : (TCRYPTMODE)methodID;
		if ( m_torkenInfo.methodID == DEFAULT || m_torkenInfo.methodID == -1 )
			siperror("Default mode error, unregistered security algorithm<%s>", m_torkenInfo.strName.c_str());

		m_torkenInfo.bKeyGenEach= varPtr->asBool(KEY_GEN_EACHCALL);
		m_torkenInfo.bykeyLen	= varPtr->asInt(KEY_LEN);
		m_torkenInfo.bylimitPlain	= varPtr->asInt(PLAIN_LIMIT);
		m_torkenInfo.strKeyFileType	= varPtr->asString(KEY_FILENAME);
		m_torkenInfo.strKeyFileType	= varPtr->asString(KEY_FILETYPE);
	}

	setDefMode(config);
	CCryptSystem::init();
}

void	CSecurityInfo::setDefMode(SIPBASE::CConfigFile &config)
{ 
	if ( config.exists("DefaultMode") )
	{
		std::string strDefMode = config.getVar("DefaultMode").asString();
		m_defMode = (TCRYPTMODE)strToMethod(strDefMode);
		if ( m_defMode == DEFAULT || m_defMode == -1 )
			siperror("Default mode error, unregistered security algorithm<%s>", strDefMode.c_str());
		sipinfo("Default Crypt Mode set %s", strDefMode.c_str());
	}
};

} // namespace SIPNET