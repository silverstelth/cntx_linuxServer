/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include	"stdnet.h"
#include	"openssl/rsa.h"
#include	"openssl/des.h"
#include	"openssl/evp.h"

#include	"openssl/bio.h"
#include	"openssl/err.h"
#include	"openssl/rand.h"
#include	"openssl/evp.h"
#include	"openssl/crypto.h"
#include	"openssl/ssl.h"

#include	"misc/path.h"
#include	"misc/string_common.h"
#include	"misc/factory.h"
#include	"misc/stop_watch.h"

#include	"net/CryptTask.h"

using namespace		std;
using namespace		SIPBASE;
using namespace		SIPNET;

#define		ERR_SSL(str) \
				unsigned long err = ERR_get_error();\
				siperror("ErrNo : %d, Error %s, Error = %s\n", err, str, ERR_reason_error_string(err));

const char		*	g_strMethod[MAXNUM_CRYPTMETHOD]	=	{ "Default", "NON", "RAND-SSL", "RAND-TIME", "COOKIE", "DES", "EVP", "MD", "WRAP", "RSA"};

int		strToMethod(std::string str)
{
	if ( str == "Default" )
		return	DEFAULT;
	if ( str == "NON" )
		return	NON_CRYPT;
	if ( str == "RAND-SSL" )
		return	METHOD_RAND_SSL;
	if ( str == "RAND-TIME" )
		return	METHOD_RAND_TIME;
	if ( str == "COOKIE" )
		return	METHOD_COOKIE;
	if ( str == "DES" )
		return	METHOD_DES;
	if ( str == "EVP" )
		return	METHOD_EVP;
	if ( str == "MD" )
		return	METHOD_MD;
	if ( str == "WRAP" )
		return	METHOD_WRAP;
	if ( str == "RSA" )
		return	METHOD_RSA;
    
	return	-1;
}

namespace SIPNET
{
//////////////////////////////////////////////////////////////////////////
// Exception
//////////////////////////////////////////////////////////////////////////
ETimeout::ETimeout(const char *reason, bool systemerr)
{
	_Reason = "TIMEOUT ERROR: ";
	_Reason += reason;

	sipwarning( "LNETL0: Exception will be launched: %s", _Reason.c_str() );
}

ESecurity::ESecurity(const char *reason, bool systemerr)
{
	_Reason = "SECURITY ERROR: ";
	_Reason += reason;

	if ( systemerr )
	{
		_Reason += " (";

		unsigned long noerr = ERR_get_error();
		char str[256];
		smprintf( str, 256, "%d", noerr );
		_Reason += str;
		if ( noerr != 0 )
		{
			_Reason += ": ";
			_Reason += ERR_reason_error_string(noerr);
		}

		_Reason += ")";
	}
	sipwarning( "LNETL0: Exception will be launched: %s", _Reason.c_str() );
}

//////////////////////////////////////////////////////////////////////////
//	ICryptable
//////////////////////////////////////////////////////////////////////////

#define	_FILENAME_DATE	"yyyy'-'MM'-'dd' '"
#define	_FILENAME_TIME	"HH'-'mm"

static std::string	getMixedTime()
{
	std::string	strTime;
#ifdef SIP_OS_WINDOWS
	char str[16];
	GetDateFormatA(LOCALE_USER_DEFAULT, NULL, NULL, _FILENAME_DATE, str, 16);
	strTime += str;
	strTime += "_";
	GetTimeFormatA(LOCALE_USER_DEFAULT, TIME_FORCE24HOURFORMAT, NULL, _FILENAME_TIME, str, 16);
	strTime += str;
#else
	sipwarning("not tested : getMixedTime()");
	char str[128];
	time_t curtime;
	time(&curtime);
	tm *curtm = localtime(&curtime);
	strftime(str, 128, "%Y-%m-%d %H-%M", curtm);
	strTime = str;
#endif // SIP_OS_WINDOWS

	return strTime;
}

std::string		ICryptable::getKeyFileName()
{
	std::string		strFile;
	string	sfiletype = m_cryptInfo.strKeyFileType;
	string	sfilename = m_cryptInfo.strKeyFileName;

	if ( sfilename.compare("") != 0 )
		return	sfilename + "." + sfiletype;

	//strFile = KEY_DIR;

	strFile	= AlgorithmName();
	strFile	+= "_" + getMixedTime();
	strFile += "." + sfiletype;

	return	strFile;
}


_AsymCryptable::_AsymCryptable()
{

}

_AsymCryptable::_AsymCryptable(const ICryptable::TCtorParam & param)
: ICryptable(param)
{

}

_AsymCryptable::~_AsymCryptable()
{

}

_SymCryptable::_SymCryptable()
{

}

_SymCryptable::_SymCryptable(const ICryptable::TCtorParam & param)
: ICryptable(param)
{

}

_SymCryptable::~_SymCryptable()
{

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	_RSA
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RSA	*	_RSA::m_pRSA		= NULL;
RSA *	_RSA::m_pRSAPub		= NULL;
RSA	*	_RSA::m_pRSAPriv	= NULL;

//SIPBASE_REGISTER_OBJECT(ICryptable, _RSA, std::string, std::string(g_strMethod[METHOD_RSA]));
SIPBASE_REGISTER_OBJECT(ICryptable, _RSA, TCRYPTMODE, METHOD_RSA);

_RSA::_RSA()
{
}

_RSA::_RSA(const ICryptable::TCtorParam & param)
:_AsymCryptable(param)
{
}

_RSA::~_RSA()
{
}

void	_RSA::init()
{
	ERR_load_crypto_strings();
//	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	m_pRSA		= RSA_new();

	m_pRSAPriv	= RSA_new();
	m_pRSAPub	= RSA_new();

	ICryptable::init();
}

void	_RSA::release()
{
	RSA_free(m_pRSAPriv);
	m_pRSAPriv	= NULL;

	RSA_free(m_pRSAPub);
	m_pRSAPub	= NULL;

	RSA_free(m_pRSA);
	m_pRSA		= NULL;
}


// bSure : means throughly regenerate
void	_RSA::generateKey(KEY_INFO & key)
{
	try
	{
		if ( CPath::isEmptyPath(KEY_DIR) || m_cryptInfo.bKeyGenEach )
			generateKeyPair(key);
		else
			loadKeyPair(key);
		
		if( !RSA_check_key(m_pRSAPriv) )
			throw ESecurityKeyGenerator("Private Key not matched : RSA_check_key()");
	}
	catch(ESecurity &e)
	{
		sipwarning("Generate Key Failed : %s", e.what());
	}
}

int		_RSA::getKeyLen()
{
	int len = RSA_size(m_pRSA);
	return	len;
}

// [in] strKeyFile	: from config file
// [in] keyType		: file type : *.PEM, *.DER from config file
void	_RSA::generateKeyPair(KEY_INFO	&key)
{
	AUTO_WATCH(RSA_GenerateKeyPair);

#ifdef SIP_OS_WINDOWS
	//RAND_screen();
#else
	sipwarning("not prepared : _RSA::generateKeyPair()");
#endif // SIP_OS_WINDOWS
	m_pRSA = RSA_generate_key(RSA_KEYSIZE, RSA_F4, NULL, NULL);
	if ( !m_pRSA )
		throw ESecurityKeyGenerator("RSG Generate Key Err");

	key.keyLen = RSA_size(m_pRSA);
	unsigned char keybuf[BUFSIZ*10], *pbuf;
	int len;

	pbuf = keybuf;
	len = i2d_RSAPublicKey(m_pRSA, &pbuf);
	len += i2d_RSAPrivateKey(m_pRSA, &pbuf);

	/* get separated rsa key pair */
	pbuf = keybuf;
	m_pRSAPub = d2i_RSAPublicKey(NULL, (const unsigned char **)&pbuf, (long) len);
	if ( !m_pRSAPub )
		throw ESecurityKeyGenerator("get RSA from KeyBuffer!");
	len -= pbuf - keybuf;
	m_pRSAPriv = d2i_RSAPrivateKey(NULL, (const unsigned char **)&pbuf, (long) len);
	if ( !m_pRSAPriv )
		throw ESecurityKeyGenerator("get RSA from KeyBuffer!");

	//////////////////////////////////////////////////////////////////////////
	// Save To Memory
	key.method = METHOD_RSA;
	pbuf = key.pubKey;
	len = i2d_RSAPublicKey(m_pRSA, &pbuf);
	key.pubkeySize = len;

	pbuf = key.priKey;
	len = key.pubkeySize;
	len = i2d_RSAPrivateKey(m_pRSA, &pbuf);
	key.prikeySize = len;
	//////////////////////////////////////////////////////////////////////////
	// Save To File
	string	sfiletype = m_cryptInfo.strKeyFileType;
	string	sfilename = m_cryptInfo.strKeyFileName;
#ifdef _RSA_KEY_PAIR_TOFILE_

	if ( sfiletype == "pem" )
	{
		FILE * fp = fopen(getKeyFileName().c_str(), "w");

		if ( fp == NULL )
			throw ESecurityKeyGenerator("FileOpenError", false);
		
		//uint8 numwritten = fwrite( "test", sizeof( char ), 25, fp );

		if ( ! fp )
			throw ESecurityKeyGenerator("file open err!");
		if ( ! PEM_write_RSAPrivateKey(fp, m_pRSA, NULL, NULL, 0, NULL, NULL) )
			throw ESecurityKeyGenerator("PEM_write_RSAPrivateKey()");
		if ( ! PEM_write_RSA_PUBKEY(fp, m_pRSA) )
			throw ESecurityKeyGenerator("PEM_write_RSA_PUBKEY()");
		if ( ! fp )
	        fclose(fp);
		sipinfo("PEM MODE KeyFile %s Created!", m_strKeyFile.c_str()); 
	}

	if ( sfiletype == "der" )
	{
		// Public Key
		std::string strFile	= string("pub") + getKeyFileName();
		FILE * fpubkey	= fopen(strFile.c_str(), "wb");
		if(fpubkey == NULL)
		{
			throw ESecurityKeyGenerator("i2d_RSAPrivateKey()");
    		return;
		}
		fwrite(key.pubKey, 1, key.pubkeySize, fpubkey);
		fclose(fpubkey);

		// Private Key
		strFile	= string("priv") + getKeyFileName();
		FILE * fprivkey		= fopen(strFile.c_str(), "wb");
		if(fprivkey == NULL)
		{
			throw ESecurityKeyGenerator("i2d_RSAPrivateKey()");
    		return;
		}
		fwrite(key.priKey, 1, key.prikeySize, fprivkey);
		fclose(fprivkey);

		sipdebug("DER MODE KeyFile %s Created!", strFile.c_str());
	}

#endif //_RSA_KEY_PAIR_TOFILE_

	if( !RSA_check_key(m_pRSAPriv) )
		throw ESecurityKeyGenerator("Private Key not matched : RSA_check_key()");
}


// in	: 
// out	: m_pRSAPub, m_pRSAPriv

// GenerateKey : thoroughly generate key, but 
// loadKeyPair : conditionaly generate key

void	_RSA::loadKeyPair(KEY_INFO & key)
{
	AUTO_WATCH(RSA_LoadKeyPair);
    
	m_cryptInfo.methodID = METHOD_RSA;

	std::string strKeyFile = m_cryptInfo.strKeyFileName;
	if ( m_cryptInfo.strKeyFileType == "pem" )
	{
		m_strKeyFile = KEY_DIR;
		m_strKeyFile += ( strcmp(strKeyFile.c_str(), "") == 0 ) ? KEY_FILE_PEM : strKeyFile;
		FILE *fp	= fopen(m_strKeyFile.c_str(), "r");

		if ( ! PEM_read_RSAPrivateKey(fp, &m_pRSAPriv,NULL, NULL) )
			throw ESecurityKeyLoader("PEM_read_RSAPrivateKey()");

		if ( ! PEM_read_RSA_PUBKEY(fp, &m_pRSAPub,NULL, NULL) )
			throw ESecurityKeyLoader("PEM_read_RSA_PUBKEY()");

		/*		unsigned char keybuf[BUFSIZ*10], *pbuf;
		pbuf	= keybuf;
		int		len;
		len		= i2d_RSAPublicKey(rsa, &pbuf);
		len		+= i2d_RSAPrivateKey(rsa, &pbuf);
		*/
		// RSAinfo -> buf
		unsigned char	ucPubKey[MAX_RSA_KEY_LENGTH] = {0},
						ucPriKey[MAX_RSA_KEY_LENGTH] = {0};
		unsigned char	*pt1, *pt2;
		int				len1, len2;
		len1	= i2d_RSAPublicKey(m_pRSAPub, NULL);
		pt1		= ucPubKey;
		len1	= i2d_RSAPublicKey(m_pRSAPub, &pt1);

		len2	= i2d_RSAPrivateKey(m_pRSAPriv, NULL);
		pt2		= ucPriKey;
		len2	= i2d_RSAPrivateKey(m_pRSAPriv, &pt2);
		if ( !fp )
			fclose(fp);
	}

	if ( m_cryptInfo.strKeyFileType == "der" )
	{
		FILE *fp	= fopen(getKeyFileName().c_str(), "r");

		unsigned char ucPriKey[MAX_RSA_KEY_LENGTH] = {0}, ucPubKey[MAX_RSA_KEY_LENGTH] = {0};
		string			strFile;

		uint32	&priKeySize = key.prikeySize;
		uint32	&pubKeySize = key.pubkeySize;

		// Priv Key
		FILE *fprikey	= NULL;
		strFile			= string("priv_") + getKeyFileName();
		fprikey			= fopen(strFile.c_str(), "rb");
		if( ! fprikey )
			throw ESecurityKeyLoader("fopen prikey.key failed!");
		fseek(fprikey, 0, SEEK_END);
		priKeySize	= (uint32)ftell(fprikey);
		fseek(fprikey, 0, SEEK_SET);
		fread(ucPriKey, 1, priKeySize, fprikey);
		fclose(fprikey);

		autoPtr<RSA> pRsaPriKey(RSA_new());
		RSA * temp = pRsaPriKey.get();
		getPrivKeyInfo(key.priKey, priKeySize, &temp);
		pRsaPriKey.reset(temp);

		// Pub Key
		FILE *fpubkey	= NULL;
		strFile			= string("pub") + KEY_FILE_DER;
		fpubkey			= fopen(strFile.c_str(), "rb");
		if ( ! fpubkey )
			throw ESecurityKeyLoader("fopen pubkey.key failed!");
		fseek(fpubkey, 0, SEEK_END);
		pubKeySize	= (uint32)ftell(fprikey);
		fseek(fpubkey, 0, SEEK_SET);
		fread(ucPubKey, 1, pubKeySize, fpubkey);
		fclose(fpubkey);

		autoPtr<RSA> pRsaPubKey(RSA_new());
		temp = pRsaPubKey.get();
		getPubKeyInfo(key.pubKey, pubKeySize, &temp);
		pRsaPubKey.reset(temp);

		if ( !fp )
			fclose(fp);
	}
}

// pre generate : true,
bool	_RSA::alreadyKeyGenerated()
{
	if ( CPath::isEmptyPath(AUTH_DIR) )
		return	false;

	return true;
}

void	_RSA::getPrivKeyInfo(unsigned char *pucPriKeyData, unsigned long KeyDataLen, RSA* *priRsa)
{
	unsigned char * Pt	= pucPriKeyData;
	*priRsa		= d2i_RSAPrivateKey(NULL, (const unsigned char **) &Pt, KeyDataLen);
	if(priRsa == NULL)
		throw ESecurityKeyManager("Get Private Key");
}


void	_RSA::getPubKeyInfo(unsigned char *pucPubKeyData,unsigned long KeyDataLen, RSA* *pubRsa)
{
	unsigned char * Pt	= pucPubKeyData;
	*pubRsa		= d2i_RSAPublicKey(NULL, (const unsigned char **) &Pt, KeyDataLen);
	if(pubRsa == NULL)
		throw ESecurityKeyManager("Get Public Key");
}

/*
* keybuf(privateKey) ---------------------------------------|
*					|  pubkey        |		privkey			|
							    	 |-----	 KeyDataLen  ---|
								* pucPriKeyData
outside this function,
example:
		pkey = keybuf;
		memcpy(pkey, key.pubKey, key.pubkeySize);
		pkey = pkey + key.pubkeySize;
		memcpy(pkey, key.priKey, key.prikeySize);
		m_pRSAPriv = d2i_RSAPrivateKey(NULL, (const unsigned char **)&pkey, (long) key.prikeySize);
		if ( !m_pRSAPriv )
		throw("get RSA Error!");
*/
void	_RSA::setPrivKeyInfo(unsigned char *pucPriKeyData,	unsigned long KeyDataLen)
{
	unsigned char * Pt	= pucPriKeyData;
	m_pRSAPriv			= d2i_RSAPrivateKey(NULL, (const unsigned char **) &Pt, KeyDataLen);
	if(m_pRSAPriv == NULL)
		throw ESecurityKeyManager("Get Private Key");
}

/*
outside this function,
example:
	unsigned char * pkey = key.pubKey;
	m_pRSAPub = d2i_RSAPublicKey(NULL, (const unsigned char **)&pkey, key.pubkeySize);
*/
void	_RSA::setPubKeyInfo(unsigned char *pucPubKeyData,	unsigned long KeyDataLen)
{
	unsigned char * Pt	= pucPubKeyData;
	m_pRSAPub			= d2i_RSAPublicKey(NULL, (const unsigned char **) &Pt, KeyDataLen);
	if(m_pRSAPub == NULL)
		throw ESecurityKeyManager("Get Public Key");
}

void	_RSA::encode(unsigned char * ucText,		unsigned long text_len,
							  unsigned char * ucEnctext,	unsigned long &enc_len)
{
	AUTO_WATCH(RSA_Encode);

#ifdef PUBKEY_ENCRYPT
	if ( ! m_pRSAPub )
		throw	ESecurityKeyManager("Encryption Failed! : PubKey is not generated, Regenerate or load already generated key", false);

	int ret = encSessionKeybyRsaPubKey(m_pRSAPub, ucText, text_len, ucEnctext, &enc_len);
	if ( ret == -1 || enc_len <= 0 )
		throw	ESecurityEngineEncoder();

#else
#error	This function is not prepared!
#endif
}

void	_RSA::decode(unsigned char * ucEncText,	unsigned long enc_len,  
							  unsigned char * ucDecText,	unsigned long &dec_len)
{
	AUTO_WATCH(RSA_Decode);

#ifdef PRIKEY_DECRYPT
	if ( ! m_pRSAPriv )
		throw	ESecurityKeyManager("Error :Private key context is not already prepared!", false);

	int ret = decSessionKeybyRsaPriKey(m_pRSAPriv, ucEncText, enc_len, ucDecText, &dec_len);
	if ( ret == -1 || dec_len <= 0 )
		throw ESecurityEngineDecoder();

#else
#error This function is not prepared!
#endif
}

// in : rsa, ucKey, ulKeyLen,
// out : outData, unsigned long *pulOutLen
int		_RSA::encSessionKeybyRsaPubKey(RSA *rsa, unsigned char *ucKey, unsigned long ulKeyLen,
													  unsigned char *outData, unsigned long *pulOutLen)
{
	unsigned char pad = RSA_PKCS1_PADDING;
	return (*pulOutLen = RSA_public_encrypt(ulKeyLen, ucKey, outData, rsa, pad));
}

// in : rsa, unsigned char *InData, unsigned long ulDataLen
// out : ucKey, pulKeyLen
int		_RSA::decSessionKeybyRsaPriKey(RSA *rsa, unsigned char *InData, unsigned long ulDataLen,
													  unsigned char *ucKey, unsigned long *pulKeyLen)
{
	unsigned char pad = RSA_PKCS1_PADDING;
    return (*pulKeyLen = RSA_private_decrypt(ulDataLen, InData, ucKey, rsa, pad));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	_EVP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EVP_CIPHER_CTX	_EVP::m_ctx		= {};
des_cblock *	_EVP::m_evpKey	= NULL;

SIPBASE_REGISTER_OBJECT(ICryptable, _EVP, TCRYPTMODE, METHOD_EVP);

_EVP::_EVP()
{
}

_EVP::_EVP(const ICryptable::TCtorParam & param)
:ICryptable(param)
{
}

_EVP::~_EVP()
{
}


void	_EVP::init()
{
	ERR_load_crypto_strings();
//	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	EVP_CIPHER_CTX_init(&m_ctx);

	ICryptable::init();
}

void	_EVP::release()
{
	EVP_CIPHER_CTX_cleanup(&m_ctx);
}

void	_EVP::generateKey(KEY_INFO &key)
{
	//keyType = (keyType == NON) ? DES_FILE : keyType;

	key.method = METHOD_DES;
	unsigned char	sessionKey[] = "67hh568jl812dgsdv336chg39(r";

	RAND_seed(sessionKey, sizeof(sessionKey));
	m_evpKey = (des_cblock *)malloc(sizeof(des_cblock));

	des_random_key(m_evpKey);
    if(des_is_weak_key(m_evpKey))
	{
		ESecurityKeyGenerator("WEAK KEY!!!");
		return ;
	}
	memcpy(key.priKey, m_evpKey, sizeof(key.priKey));
	key.prikeySize = sizeof(m_evpKey);
}

void	_EVP::encode(unsigned char * ucText,		unsigned long text_len,
							  unsigned char * ucEnctext,	unsigned long &enc_len)
{
	AUTO_WATCH(EVP_Encode);

	unsigned char key[]	= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	unsigned char iv[]	= {1,2,3,4,5,6,7,8};

	EVP_EncryptInit_ex(&m_ctx, EVP_get_cipherbyname(STR_DES_METHOD), NULL, key, iv);

	if( !EVP_EncryptUpdate( &m_ctx, ucEnctext, (int *)&enc_len, ucText, text_len) )
	{
		ESecurityEngineEncoder("EVP_EncryptUpdate");
		return;
	}

	int	tmplen;
	if(!EVP_EncryptFinal_ex(&m_ctx, ucEnctext + enc_len, &tmplen))
	{
		ESecurityEngineEncoder("EVP_EncryptFinal_ex");
		return;
	}

	enc_len += tmplen;
}

void	_EVP::decode(unsigned char * ucEncText,	unsigned long enc_len, 
							  unsigned char * ucDecText,	unsigned long &dec_len)
{
	AUTO_WATCH(EVP_Decode);

	unsigned char key[]	= {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	unsigned char iv[]	= {1,2,3,4,5,6,7,8};

	EVP_DecryptInit_ex(&m_ctx, EVP_get_cipherbyname(STR_DES_METHOD), NULL, key, iv);

	if( !EVP_DecryptUpdate(&m_ctx, ucDecText, (int *)&dec_len, ucEncText, enc_len) )
	{
		ESecurityEngineDecoder("EVP_DecryptUpdate");
		return;
	}

	int	tmplen;
    if(!EVP_DecryptFinal_ex(&m_ctx, ucDecText + dec_len, &tmplen))
	{
		ESecurityEngineDecoder("EVP_DecryptFinal_ex");
		return;
	}

	dec_len += tmplen;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	_DES
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned char	_DES::m_Key192[24] = {0}; // 192bit Key

SIPBASE_REGISTER_OBJECT(ICryptable, _DES, TCRYPTMODE, METHOD_DES);

void	_DES::init()
{
	ERR_load_crypto_strings();
//	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	ICryptable::init();
}

void	_DES::release()
{
}

bool	_DES::setupKeySchedules(DES_key_schedule& ks1, DES_key_schedule& ks2, DES_key_schedule& ks3, unsigned char fullKey[24])
{
	des_cblock key1, key2, key3;
	memcpy(key1,fullKey,8);
	memcpy(key2,fullKey+8,8);
	memcpy(key3,fullKey+16,8);

	DES_set_odd_parity((DES_cblock *)key1);
	DES_set_odd_parity((DES_cblock *)key2);
	DES_set_odd_parity((DES_cblock *)key3);

	if(DES_set_key_checked((DES_cblock *)key1, &ks1) != 0)
	{
		throw	ESecurityKeyGenerator("DES Key not prepared!");
		return false;
	}

	if(DES_set_key_checked((DES_cblock *)key2, &ks2) != 0)
	{
		throw	ESecurityKeyGenerator("DES Key not prepared!");
		return false;
    }

	if(DES_set_key_checked((DES_cblock *)key3, &ks3) != 0)
	{
		throw	ESecurityKeyGenerator("DES Key not prepared!");
		return false;
	}

//	des_set_key_unchecked(&key1,ks1);
//	des_set_key_unchecked(&key2,ks2);
//	des_set_key_unchecked(&key3,ks3);

	return true;
}



void	_DES::generateKey(KEY_INFO &key)
{
    AUTO_WATCH(DES_GenerateKey);
	//Make a random 192 bit key (With odd Parity)
	rand();

	key.method = METHOD_DES;

	des_cblock randomKeyPart;
	des_random_key(&randomKeyPart);
	des_set_odd_parity(&randomKeyPart);
	memcpy(m_Key192, randomKeyPart, 8);

	des_random_key(&randomKeyPart);
	des_set_odd_parity(&randomKeyPart);
	memcpy(m_Key192+8, randomKeyPart, 8);

	des_random_key(&randomKeyPart);
	des_set_odd_parity(&randomKeyPart);
	memcpy(m_Key192 + 16, randomKeyPart, 8);

	key.secretkeySize = 24;
	memcpy(key.secretKey, m_Key192, key.secretkeySize);

		/************************************************************************/
		/*					Prepare a vector for encryption						*/
		/************************************************************************/
		/*
			des_cblock ivec;
			des_random_key(&ivec);//Setup a random initialization vector.
			des_cblock ivec_tmp;
			memcpy(ivec_tmp,ivec,8);//Copy it so it doesn't get messed with.
		*/
}


void	_DES::setSecretKey(unsigned char *key,	unsigned long len)
{
	memcpy(m_Key192, key, 24 * sizeof(unsigned char));
}

uint	_DES::getSecretKey(unsigned char *key)
{
	return 0;
}

void	_DES::encode(unsigned char * ucText,		unsigned long text_len,
							  unsigned char * ucEnctext,	unsigned long &enc_len)
{
	AUTO_WATCH(DES_Encode);

	//Fill in the key schedules
	des_key_schedule ks1, ks2, ks3;
	setupKeySchedules(ks1, ks2, ks3, m_Key192);

	/************************************************************************/
	/*							8byte encoding								*/
	/************************************************************************/
	//des_ecb3_encrypt((const unsigned char *)ucText,(unsigned char *)ucEnctext, ks1, ks2, ks3, DES_ENCRYPT);

	// block encoding
	//unsigned char ivec_tmp[1024];
	//memset(ivec_tmp, 0, sizeof(unsigned char) * text_len);
	des_cblock   ivec_tmp;
	memset(ivec_tmp, 0, sizeof(des_cblock));
	if ( text_len % 8 )
        enc_len = ( text_len / 8 + 1 ) * 8;
	else
        enc_len = text_len;

	memset(ucEnctext, 0, enc_len * sizeof(unsigned char));
	des_ede3_cbc_encrypt(ucText,ucEnctext,text_len, ks1, ks2, ks3, (DES_cblock *) &ivec_tmp, DES_ENCRYPT);
}

void	_DES::decode(unsigned char * ucEncText,	unsigned long enc_len, 
				   unsigned char * ucDecText,	unsigned long &dec_len)
{
	AUTO_WATCH(DES_Decode);

	des_key_schedule ks1, ks2, ks3;
	setupKeySchedules(ks1, ks2, ks3, m_Key192);

	/************************************************************************/
	/*							8byte decoding								*/
	/************************************************************************/
	// 8byte decoding
	//des_ecb3_encrypt((const unsigned char *)ucEncText,(unsigned char *)ucDecText, ks1, ks2, ks3, DES_DECRYPT);

	// block decoding
	des_cblock   ivec_tmp;
	memset(ivec_tmp, 0, sizeof(des_cblock));
	dec_len = enc_len;
    memset(ucDecText, 0, dec_len * sizeof(unsigned char));
	des_ede3_cbc_encrypt(ucEncText, ucDecText, enc_len, ks1, ks2, ks3, (DES_cblock *) &ivec_tmp, DES_DECRYPT);
}

_DES::_DES()
{
}

_DES::_DES(const ICryptable::TCtorParam & param)
	:_SymCryptable(param)
{
}

_DES::~_DES()
{
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	_MD
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EVP_MD_CTX	_MD::m_ctx = {};
SIPBASE_REGISTER_OBJECT(ICryptable, _MD, TCRYPTMODE, METHOD_MD);

_MD::_MD()
{
}

_MD::_MD(const ICryptable::TCtorParam & param)
:_SymCryptable(param)
{
}

_MD::~_MD()
{
}

void	_MD::init()
{
	ERR_load_crypto_strings();
//	SSL_load_error_strings();
	ERR_load_BIO_strings();
	OpenSSL_add_all_algorithms();

	ICryptable::init();
}

void	_MD::release()
{
}

void	_MD::generateKey(KEY_INFO & info)
{
	//keyType = (keyType == NON) ? MD_FILE : keyType;

	info.method = METHOD_MD;
}

void	_MD::encode(unsigned char * ucText,	unsigned long text_len, 
						     unsigned char * ucEnctext,	unsigned long &enc_len)
{

}

void	_MD::decode(unsigned char * ucEncText,	unsigned long enc_len, 
							 unsigned char * ucDecText,	unsigned long &dec_len)
{
}



//////////////////////////////////////////////////////////////////////////
//	_MSGWrap : Wrap the main information with padding data
//////////////////////////////////////////////////////////////////////////

SIPBASE_REGISTER_OBJECT(ICryptable, _MSGWrap, TCRYPTMODE, METHOD_WRAP);

_MSGWrap::_MSGWrap()
{
}

_MSGWrap::_MSGWrap(const ICryptable::TCtorParam & param)
:ICryptable(param)
{
}

_MSGWrap::~_MSGWrap()
{
}

void	_MSGWrap::init()
{
	ICryptable::init();
}

void	_MSGWrap::release()
{
}

void	_MSGWrap::generateKey(KEY_INFO & info)
{
	//keyType = (keyType == NON) ? MD_FILE : keyType;

	info.method = METHOD_MD;

}

void	_MSGWrap::encode(unsigned char * ucText,	unsigned long text_len, 
							 unsigned char * ucEnctext,	unsigned long &enc_len)
{
}

void	_MSGWrap::decode(unsigned char * ucEncText,	unsigned long enc_len, 
							 unsigned char * ucDecText,	unsigned long &dec_len)
{
}


//////////////////////////////////////////////////////////////////////////
//	_CookieKey : Check Auth Key Pair
//////////////////////////////////////////////////////////////////////////

SIPBASE_REGISTER_OBJECT(ICryptable, _CookieKey, TCRYPTMODE, METHOD_COOKIE);

_CookieKey::_CookieKey()
{
}

_CookieKey::_CookieKey(const ICryptable::TCtorParam & param)
:_RandomKeyGenerator(param)
{
}

_CookieKey::~_CookieKey()
{
}

void	_CookieKey::generateKey(KEY_INFO &info)
{
    AUTO_WATCH(CookieKey_GenerateKey);

	uint32 t = (uint32)time (NULL);
	srand (t);

	uint32 r = rand ();
	static uint32 n = 0;
	n++;

	// 12bits for the time (in second) => loop in 1 hour
	//  8bits for random => 256 case
	// 12bits for the inc number => can generate 4096 keys per second without any problem (if you generate more than this number, you could have 2 same keys)
	uint32	ret = (t&0xFFF)<<20 | (r&0xFF)<<12 | (n&0xFFF);

	// 12bits for the time (in second) => loop in 1 hour
	// 20bits for the inc number => can generate more than 1 million keys per second without any problem (never exceed on my computer)
	//	return (t&0xFFF)<<20 | (n&0xFFFFF);
}

//////////////////////////////////////////////////////////////////////////
//	_RandomKeyGenerator : Check Auth Key Pair
//////////////////////////////////////////////////////////////////////////

/*
byte	_RandomKeyGenerator::GenerateByte()
{
}
*/

_RandomKeyGenerator::_RandomKeyGenerator()
{

}

_RandomKeyGenerator::_RandomKeyGenerator(const ICryptable::TCtorParam & param)
:ICryptable(param)
{

}

_RandomKeyGenerator::~_RandomKeyGenerator()
{

}

void	_RandomKeyGenerator::generateKey(KEY_INFO &info)
{
}

uint	_RandomKeyGenerator::GenerateBit()
{
	return 0;
}

//! generate a random 32 bit word in the range min to max, inclusive
uint32	_RandomKeyGenerator::GenerateWord32(uint32 a, uint32 b)
{
	return 0;
}

//! generate random array of bytes
/*! Default implementation is to call GenerateByte() size times. */
void	_RandomKeyGenerator::GenerateBlock(uint8 *output, unsigned int size)
{
	while (size--)
		*output++ = GenerateByte();
}

//! generate and discard n bytes
/*! Default implementation is to call GenerateByte() n times. */
void	_RandomKeyGenerator::DiscardBytes(unsigned int n)
{
}

SIPBASE_REGISTER_OBJECT(ICryptable, _sslRanKeyGenerator, TCRYPTMODE, METHOD_RAND_SSL);

_sslRanKeyGenerator::_sslRanKeyGenerator()
{

}

_sslRanKeyGenerator::_sslRanKeyGenerator(const ICryptable::TCtorParam & param)
:_RandomKeyGenerator(param)
{

}

_sslRanKeyGenerator::~_sslRanKeyGenerator()
{

}

void	_sslRanKeyGenerator::generateKey(KEY_INFO &info)
{
	AUTO_WATCH(SSL_RanKeyGenerator);

	GenerateBlock(info.secretKey, m_cryptInfo.bykeyLen);
	info.secretkeySize = m_cryptInfo.bykeyLen;
}


uint8	_sslRanKeyGenerator::GenerateByte()
{
	uint8	randKey[1] = {0};///?
	
	AUTO_WATCH(SSLGenerateByte);
	// changed by ysg 2009-05-20
#ifdef SIP_OS_WIINDOWS
	RAND_screen();
#else
	sipwarning("not prepared : _RSA::generateKeyPair()");
#endif // SIP_OS_WIINDOWS
	RAND_bytes(static_cast<unsigned char *>(randKey), 1);

	return	randKey[0];
}


SIPBASE_REGISTER_OBJECT(ICryptable, _timeRandKeyGenerator, TCRYPTMODE, METHOD_RAND_TIME);

void	_timeRandKeyGenerator::generateKey(KEY_INFO &info)
{
    AUTO_WATCH(TIME_RanKeyGenerator);
    
	GenerateBlock(info.secretKey, m_cryptInfo.bykeyLen);
	info.secretkeySize = m_cryptInfo.bykeyLen;
}

uint8	_timeRandKeyGenerator::GenerateByte()
{
	uint32	t = (uint32)time (NULL);
	srand (t);

	uint32	r = rand ();
	uint8 	ret = (uint8) r;

	return	ret;
}

_timeRandKeyGenerator::_timeRandKeyGenerator()
{
}

_timeRandKeyGenerator::_timeRandKeyGenerator(const ICryptable::TCtorParam & param)
:_RandomKeyGenerator(param)
{
}

_timeRandKeyGenerator::~_timeRandKeyGenerator()
{
}

} // namespace SIPNET
