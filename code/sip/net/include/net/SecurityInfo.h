/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SECURITYINFO_H__
#define __SECURITYINFO_H__

#include	"misc/variable.h"
#include	"net/CryptTask.h"

namespace SIPNET
{

// load Security Information from config
class CSecurityInfo
{
	SIPBASE_SAFE_SINGLETON_DECL(CSecurityInfo)

public:
	void			initSecurity(SIPBASE::CConfigFile &config);
	TCRYPT_INFO		getCryptoInfo(TCRYPTMODE mode);
	
	TCRYPT_INFO		getSymInfo() {return m_symInfo;};
	void			setSymInfo(TCRYPT_INFO & info) { m_symInfo = info; };

	TCRYPT_INFO		getASymInfo(){return m_asymInfo;};
	void			setAsymInfo(TCRYPT_INFO & info) { m_asymInfo = info; };

	TCRYPT_INFO		getWrapInfo(){return m_torkenInfo;};
	void			setWrapInfo(TCRYPT_INFO & info) { m_torkenInfo = info; };

	TCRYPT_INFO		getKeyGenerateInfo(){ return m_keyGenerateInfo; };
	void			setKeyGenerateInfo(TCRYPT_INFO & info) { m_keyGenerateInfo = info; };
    
	TCRYPTMODE		getDefMode() { return	m_defMode; };
	void			setDefMode(TCRYPTMODE mode) { m_defMode = mode; };
	void			setDefMode(SIPBASE::CConfigFile &config);

	bool			IsInit() { return m_bInit; };
	void			setInited() { m_bInit = true; };


	CSecurityInfo(void);
	virtual ~CSecurityInfo(void);

protected:
	TCRYPTMODE		m_defMode;

	TCRYPT_INFO		m_symInfo;
	TCRYPT_INFO		m_asymInfo;
	TCRYPT_INFO		m_torkenInfo;
	TCRYPT_INFO		m_keyGenerateInfo;
	
	bool			m_bInit;
};

} // namespace SIPNET
extern	SIPBASE::CVariable<unsigned int>	UseSecret;
extern	SIPBASE::CVariable<unsigned int>	ExchangeAlgorithm;

#endif //__SECURITYINFO_H__

