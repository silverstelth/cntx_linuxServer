/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CRYPTTASK_H__
#define __CRYPTTASK_H__

#include	"openssl/rsa.h"
#include	"openssl/des.h"
#include	"openssl/evp.h"

#include	"openssl/rand.h"
#include	"misc/stream.h"

#ifdef SIP_OS_WINDOWS
#if defined(_STLPORT_VERSION) || defined(_DLL)
#if _MSC_VER >= 1500
#pragma comment(lib, "fipscanister_md.lib")
#endif
#pragma comment(lib, "libeay32_md.lib") // library with multi-threaded dll run-time library
#else
#if _MSC_VER >= 1500
#pragma comment(lib, "fipscanister_mt.lib")
#endif
#pragma comment(lib, "libeay32_mt.lib") // library with multi-threaded static run-time library
#endif

#endif	//SIP_OS_WINDOWS
//#include	"Key.h"

// define macro
#define		AUTH_DIR				"../service/common/auth"
#define		RSA_KEYSIZE				512

#define		MAX_RSA_KEY_LENGTH		5120
#define		MAX_DES_KEY_LENGTH		512
#define		MAX_KEY_LENGTH			1024

#define		KEY_DIR					"./key"

#define		KEY_FILE_PEM			"key.pem"
#define		KEY_FILE_DER			"key.der"
#define		KEY_FILE_CREDIT			"sec.key"

#define		STR_DES_METHOD			"aes-128-ecb"

#define		PUBKEY_ENCRYPT
#define		PRIKEY_DECRYPT
#define		_RSA_KEY_PAIR_TOFILE_

#define		MAXNUM_CRYPTMETHOD		16

typedef		const unsigned char *	T_TEXT;

namespace SIPNET
{
// This enum value must be continues
enum TCRYPTMODE
{
	MIN_CRYPTMODE,
	DEFAULT = MIN_CRYPTMODE,	// only message use this type, but not securityServer and securityClient use!
	NON_CRYPT,

	KEYGENERATOR,
	METHOD_RAND_SSL = KEYGENERATOR,
	METHOD_RAND_TIME,
    METHOD_COOKIE,
	KEYGENERATOR_END = METHOD_COOKIE,

	SYM,
	METHOD_DES = SYM,
	METHOD_EVP,
	METHOD_MD,
	METHOD_WRAP,
	SYM_END = METHOD_WRAP,
	
	ASYM,
	METHOD_RSA = ASYM,
	ASYM_END = METHOD_RSA,

	MAX_CRYPTMODE = ASYM_END
};


enum	ECRYPT_INFO{
	METHOD_NAME,
	METHOD_ID,
	KEY_GEN_EACHCALL,
	KEY_LEN,
	PLAIN_LIMIT,
	KEY_FILENAME,
	KEY_FILETYPE
};

// From Config file
struct TCRYPT_INFO{
	TCRYPT_INFO()
	{
		methodID	= NON_CRYPT;
		bKeyGenEach	= false;
		bykeyLen	= 0;
		bylimitPlain= 0;
		strName			= "";
		strKeyFileName	= "";
		strKeyFileType	= "";
	}

	TCRYPT_INFO(TCRYPTMODE methodID, std::string strName, bool bKeyGenEach, uint16 bykeyLenm, uint16 bylimitPlain, std::string strKeyFileName, std::string strKeyFileType) :
		methodID(methodID), strName(strName), bKeyGenEach(bKeyGenEach), bykeyLen(bykeyLen), bylimitPlain(bylimitPlain), strKeyFileName(strKeyFileName), strKeyFileType(strKeyFileType)
	{
	}

	void	serial (SIPBASE::IStream &s)
	{
		s.serial ((uint8 &)methodID);
		s.serial (strName);
		s.serial (bKeyGenEach);
		s.serial (bykeyLen);
		s.serial (bylimitPlain);
		s.serial (strKeyFileName);
		s.serial (strKeyFileType);
	}

	TCRYPTMODE		methodID;		// _RSA / _DES
	std::string		strName;		// "_RSA"
	bool			bKeyGenEach;
	uint16			bykeyLen;		// generated key Length
	uint16			bylimitPlain;	// the limit byte number of plain text
	std::string		strKeyFileName;	// type of key saved file
	std::string		strKeyFileType;	// type of key saved file
};

typedef struct	_KEY_INFO
{
	TCRYPTMODE		method;
	union
	{
		struct/* _ASYM_KEY */
		{
			uint8	pubKey[BUFSIZ*2];	// case _RSA : PublicKey
			uint32	pubkeySize;			// case _RSA : PublicKey
			uint8	priKey[BUFSIZ*2];	// case _RSA : PrivateKey
			uint32	prikeySize;			// case _RSA : PrivateKey Size
		} ;//asym_key
		struct  
		{
			uint8	secretKey[24];		// case _DES : SecretKey
			uint32	secretkeySize;		// case _DES : SecretKey Size
		} ;// sym_key
	};
	uint32			keyLen;				// RSA_size()

	_KEY_INFO()
	{
        memset(pubKey, 0, BUFSIZ * 2);
		pubkeySize = 0;
		memset(priKey, 0, BUFSIZ * 2);
		prikeySize = 0;
		memset(secretKey, 0, 24);
		secretkeySize = 0;
	}

	bool operator=(_KEY_INFO & info)
	{
		method = info.method;
		keyLen = info.keyLen;
		pubkeySize = info.pubkeySize;
		memcpy(pubKey, info.pubKey, pubkeySize);
		prikeySize = info.prikeySize;
		memcpy(priKey, info.priKey, prikeySize);
		secretkeySize = info.secretkeySize;
		memcpy(secretKey, info.secretKey, secretkeySize);
		
		return true;
	}
} KEY_INFO,	* PKEY_INFO;

enum	T_KEYFILE { NON, PEM_FILE, DER_FILE, DES_FILE, MD_FILE };

struct	ETimeout : public	SIPBASE::Exception
{
	ETimeout(const char *reason="", bool systemerr=true);
};

struct	ESecurity : public	SIPBASE::Exception
{
	ESecurity(const char *reason="", bool systemerr=true);
};

struct	ESecurityKeyManager : public ESecurity
{
	 ESecurityKeyManager(const char *s="", bool systemerr=true) :
		ESecurity((std::string("Key Manage:")+s).c_str()){};
};

struct	ESecurityKeyGenerator : public ESecurityKeyManager
{
	ESecurityKeyGenerator(const char *s="", bool systemerr=true) :
		ESecurityKeyManager((std::string("Key Generate Error :")+s).c_str()){};
};

struct	ESecurityKeyLoader : public	ESecurityKeyManager
{
	ESecurityKeyLoader(const char *s="", bool systemerr=true) :
		ESecurityKeyManager((std::string("Key Load Error :")+s).c_str()){};
};

struct	ESecurityEngineErr : public	ESecurity
{
	ESecurityEngineErr(const char *s="", bool systemerr=true) :
		ESecurity((std::string("Crypt Engine :")+s).c_str()){};
};

struct	ESecurityEngineEncoder : public	ESecurityEngineErr
{
	ESecurityEngineEncoder(const char *s="", bool systemerr=true) :
		ESecurityEngineErr((std::string("Encoder Error :")+s).c_str()){};
};

struct	ESecurityEngineDecoder : public	ESecurityEngineErr
{
	ESecurityEngineDecoder(const char *s="", bool systemerr=true) :
		ESecurityEngineErr((std::string("Decoder Error :")+s).c_str()){};
};

struct	ESecurityPlainText : public	ESecurity
{
	ESecurityPlainText(const char *s="", bool systemerr=true) :
		ESecurity((std::string("Input Plain Text Error :")+s).c_str()){};
};

// Strategy!
class	ICryptable // CCryptable
{
public:

	// info for factory
	struct TCtorParam
	{
		TCRYPT_INFO	info;
	};

	virtual		void	init() { m_binit = true;};
	virtual		void	release() {};

	virtual		std::string	AlgorithmName() const {return "unknown";}
	virtual		TCRYPTMODE	AlgorithmType() const {return	DEFAULT;}


	virtual		void	generateKey(KEY_INFO &key) {};

	virtual		void	encode(unsigned char * ucText,		unsigned long text_len,
							   unsigned char * ucEnctext,	unsigned long &enc_len) {};
	virtual		void	decode(unsigned char * ucEncText,	unsigned long enc_len,
							   unsigned char * ucDecText,	unsigned long &dec_len) {};
	
	void			setCryptInfo(TCRYPT_INFO & info) { m_cryptInfo = info;}
	TCRYPT_INFO		getCryptInfo() { return m_cryptInfo; }

protected:
	std::string		getKeyFileName();
public:
    bool		isInit() { return m_binit; };

	//CKey *		getKey() { return m_pKey; };
	//void		setKey(CKey	*pinfo) { memcpy(m_pKey, pinfo, sizeof(CKey)); };
	
	ICryptable(){ m_binit = false; };

	ICryptable(const ICryptable::TCtorParam & param) :
		m_binit(false), m_cryptInfo(param.info)
		{
		}

	virtual ~ICryptable(){};

protected:
	bool			m_binit;
	TCRYPT_INFO		m_cryptInfo;
//	CKey			*m_pKey;
};

class	_MSGWrap : public	ICryptable
{
public:
	void	init();
	void	release();

	void	generateKey(KEY_INFO &key);
	void	encode(unsigned char * ucText,		unsigned long text_len,
				   unsigned char * ucEnctext,	unsigned long &enc_len);
	void	decode(unsigned char * ucEncText,	unsigned long enc_len, 
				   unsigned char * ucDecText,	unsigned long &dec_len);

	std::string	AlgorithmName() const {return "_MSGWrap";};
	TCRYPTMODE	AlgorithmType() const {return	METHOD_WRAP;};


	_MSGWrap();
	_MSGWrap(const ICryptable::TCtorParam & param);
	virtual ~_MSGWrap();
};

class	_EVP : public	ICryptable
{
public:
	void	init();
	void	release();
	
	std::string	AlgorithmName() const {return "_EVP";}
	TCRYPTMODE	AlgorithmType() const {return	METHOD_EVP;};

	void	generateKey(KEY_INFO &key);
	void	encode(unsigned char * ucText,		unsigned long text_len,
			       unsigned char * ucEnctext,	unsigned long &enc_len);
	void	decode(unsigned char * ucEncText,	unsigned long enc_len, 
				   unsigned char * ucDecText,	unsigned long &dec_len);

public:
	_EVP();
	_EVP(const ICryptable::TCtorParam & param);
	virtual ~_EVP();

protected:
	static EVP_CIPHER_CTX		m_ctx;
	static des_cblock		  * m_evpKey;
};


class	_SymCryptable : public ICryptable
{
public:
	virtual void	setSecretKey(unsigned char *key,	unsigned long len) {};
	virtual uint	getSecretKey(unsigned char *key) { return 0;};
	TCRYPTMODE		AlgorithmType() const {return	SYM;};


	_SymCryptable();
	_SymCryptable(const ICryptable::TCtorParam & param);
	virtual ~_SymCryptable();
};

class	_DES : public	_SymCryptable
{
public:
	void	init();
	void	release();

	std::string	AlgorithmName() const {return "_DES";}
	TCRYPTMODE		AlgorithmType() const {return	METHOD_DES;};

	void	generateKey(KEY_INFO &info);

	void	setSecretKey(unsigned char *key,	unsigned long len = 24);
	uint	getSecretKey(unsigned char *key);

	void	encode(unsigned char * ucText,		unsigned long text_len,
			       unsigned char * ucEnctext,	unsigned long &enc_len);
	void	decode(unsigned char * ucEncText,	unsigned long enc_len,
				   unsigned char * ucDecText,	unsigned long &dec_len);

protected:
	bool	setupKeySchedules(DES_key_schedule& ks1, DES_key_schedule& ks2, DES_key_schedule& ks3, unsigned char fullKey[24]);

public:
	_DES();
	_DES(const ICryptable::TCtorParam & param);
	virtual ~_DES();

protected:
	static des_cblock		  * m_desKey;
	static unsigned char		m_Key192[24]; // 192bit Key
};


class	_MD : public		_SymCryptable
{
public:
	void	init();
	void	release();

	std::string		AlgorithmName() const {return "_MD";};
	TCRYPTMODE		AlgorithmType() const {return	METHOD_MD;};

	void	generateKey(KEY_INFO &info);
	void	encode(unsigned char * ucText,		unsigned long text_len, 
			       unsigned char * ucEnctext,	unsigned long &enc_len);
	void	decode(unsigned char * ucEncText,	unsigned long enc_len, 
				   unsigned char * ucDecText,	unsigned long &dec_len);

	void	setSecretKey(unsigned char *key,	unsigned long len = 24) {};
	uint	getSecretKey(unsigned char *key) { return 1; };

public:
	_MD();
	_MD(const ICryptable::TCtorParam & param);
	virtual ~_MD();
protected:
	static EVP_MD_CTX			m_ctx;
};

class	_AsymCryptable : public ICryptable
{
public:
	virtual void	setPubKeyInfo(unsigned char *pucPubKeyData,		unsigned long KeyDataLen){};
	TCRYPTMODE		AlgorithmType() const {return	ASYM;};

protected:
	virtual void	setPrivKeyInfo(unsigned char *pucPriKeyData,	unsigned long KeyDataLen){};

	_AsymCryptable();
	_AsymCryptable(const ICryptable::TCtorParam & param);
	virtual ~_AsymCryptable();
};

class	_RSA : public _AsymCryptable
{
public:
	void	init();
	void	release();

	std::string	AlgorithmName() const {return "_RSA";};
	TCRYPTMODE		AlgorithmType() const {return	METHOD_RSA;};

	void	generateKey(KEY_INFO &info);
	void	encode(unsigned char * ucText,		unsigned long text_len,
					unsigned char * ucEnctext,	unsigned long &enc_len);
	void	decode(unsigned char * ucEncText,	unsigned long enc_len, 
					unsigned char * ucDecText,	unsigned long &dec_len);

	int		getKeyLen();
	// in CCryptClient, instead generateKey, use this function
	void	setPrivKeyInfo(unsigned char *pucPriKeyData,	unsigned long KeyDataLen);
	void	setPubKeyInfo(unsigned char *pucPubKeyData,		unsigned long KeyDataLen);

protected:
	void	getPrivKeyInfo(unsigned char *pucPriKeyData,	unsigned long KeyDataLen, RSA* *priRsa);
	void	getPubKeyInfo(unsigned char *pucPubKeyData,		unsigned long KeyDataLen, RSA* *pubRsa);

	int		encSessionKeybyRsaPubKey(RSA *rsa,	unsigned char *ucKey,	unsigned long ulKeyLen,
									unsigned char *outData,	unsigned long *pulOutLen);
	int		decSessionKeybyRsaPriKey(RSA *rsa,	unsigned char *InData,	unsigned long ulDataLen,
									unsigned char *ucKey,	unsigned long *pulKeyLen);

	void	loadKeyPair(KEY_INFO &info);
	void	generateKeyPair(KEY_INFO &key);

	bool	alreadyKeyGenerated();

public:
	_RSA();
	_RSA(const ICryptable::TCtorParam & param);
	virtual ~_RSA();

protected:
	static RSA		*m_pRSA;
	static RSA		*m_pRSAPub;
	static RSA		*m_pRSAPriv;
	std::string		m_strKeyFile;
};

class _RandomKeyGenerator : public ICryptable
{
	TCRYPTMODE		AlgorithmType() const {return	KEYGENERATOR;};

	virtual void	generateKey(KEY_INFO &info);

protected:
	//! generate new random byte and return it
	virtual uint8	GenerateByte()=0;
	//virtual byte	GenerateByte();

	//! generate new random bit and return it
	/*! Default implementation is to call GenerateByte() and return its parity. */
	virtual uint	GenerateBit();

	//! generate a random 32 bit word in the range min to max, inclusive
	virtual uint32	GenerateWord32(uint32 a=0, uint32 b=0xffffffff);

	//! generate random array of bytes
	/*! Default implementation is to call GenerateByte() size times. */
	virtual void	GenerateBlock(uint8 *output, unsigned int size);

	//! generate and discard n bytes
	/*! Default implementation is to call GenerateByte() n times. */
	virtual void	DiscardBytes(unsigned int n);

	//! randomly shuffle the specified array, resulting permutation is uniformly distributed
	template <class IT> void Shuffle(IT begin, IT end)
	{
		for (; begin != end; ++begin)
			std::iter_swap(begin, begin + GenerateWord32(0, end-begin-1));
	}

public:
	_RandomKeyGenerator();
	_RandomKeyGenerator(const ICryptable::TCtorParam & param);
	virtual ~_RandomKeyGenerator();
};

class _sslRanKeyGenerator : public _RandomKeyGenerator
{
public:
	virtual void	generateKey(KEY_INFO &info);
	std::string		AlgorithmName() const {return "_sslRandKeyGenerator";}
	TCRYPTMODE		AlgorithmType() const {return	METHOD_RAND_SSL;};

private:
    uint8	GenerateByte();

public:
	_sslRanKeyGenerator();
	_sslRanKeyGenerator(const ICryptable::TCtorParam & param);
	virtual ~_sslRanKeyGenerator();
};

class	_CookieKey : public	_RandomKeyGenerator
{
public:
	virtual	void	generateKey(KEY_INFO &key);
	std::string		AlgorithmName() const {return "_CookieKey";}
	TCRYPTMODE		AlgorithmType() const {return	METHOD_COOKIE;};

	uint8	GenerateByte(){ return 0;};

	_CookieKey();
	_CookieKey(const ICryptable::TCtorParam & param);
	virtual ~_CookieKey();
};

class _timeRandKeyGenerator : public _RandomKeyGenerator
{
public:
	virtual void	generateKey(KEY_INFO &info);
	std::string		AlgorithmName() const {return "_timeRandKeyGenerator";}
	TCRYPTMODE		AlgorithmType() const {return	METHOD_RAND_TIME;};

private:
    uint8	GenerateByte();

public:
	_timeRandKeyGenerator();
	_timeRandKeyGenerator(const ICryptable::TCtorParam & param);
	virtual ~_timeRandKeyGenerator();
};

} // namespace SIPNET

extern		const char *	g_strMethod[];
extern		int		strToMethod(std::string str);

#endif //__CRYPTTASK_H__
