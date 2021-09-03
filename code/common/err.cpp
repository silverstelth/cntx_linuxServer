/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <vector>
#include <map>

#include "net/service.h"

#include "./common.h"
#include "./err.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

ucstring	ucErrAry[] =
{
		"success",									//	ERR_NOERR,

		"Can't Connect",							//	ERR_CANTCONNECT
		"memory err",								//	ERR_MEMORY
		"Data len wrong!",							//	ERR_DATAOVERFLOW
		"wrong user ID",							//	ERR_NOCONDBID,
		"improper User",							//	ERR_IMPROPERUSER,
		"wrong user Password",						//	ERR_BADPASSWORD,
		"Can't Register! Already registered",		//	ERR_ALREADYREGISTER
		"no user Permission",						//	ERR_PERMISSION,			
		"incorrect Socket ID",						//	ERR_INVALIDCONNECTION,	
		"the shard is improper",					//	ERR_IMPROPERSHARD,	
		"Already such a cookie exists",				//	ERR_EXISTCOOKIE,		
		"No front-end server available",			//	ERR_NOFRONTEND,			
		"the shard is full, retry in 5 minutes",	//	ERR_SHARDFULL,			
		"the shard is closed",						//	ERR_SHARDCLOSE,
		"the shard refused your login",				//	ERR_SHARDREFUSE,
		"the shard rejected your Connection",		//	ERR_SHARDREJECT,
		"the cookie is invalid",					//	ERR_INVALIDCOOKIE,	
		"There is no such cookie",					//	ERR_NOCOOKIE,		
		"There is no FTPServer to patch for such product/versionNo",	//	ERR_NOFTPSERVER
		"There is no FTPServer to newest version update",				// ERR_NONEWFTPSERVER
		"DB Processing Error Occured!",				//	ERR_DB
		"Auth improper! Incorrect DBID!",			//	ERR_AUTH
		"User Already Online! We'll disconnect you!",					//	ERR_ALREADYONLINE
		"User Already Offline",						//	ERR_ALREADYOFFLINE
		"User is Stoped User",						//	ERR_STOPEDUSER,

		"Time out",									//	ERR_TIMEOUT

		"Can't generate key",						//	ERR_KEYGENERATE
		"Can't init Security Module",				//	ERR_SECRETINIT
		"SSL function Err"							//	ERR_SSLERR
};



ERR_TYPE	typeErrAry[] =
{
	NONE,			// "success",								//	ERR_NOERR,

	ERR,			// "Can't connect"							//	ERR_CANTCONNECT
	ERR,			// "memory err"								//	ERR_MEMORY
	ERR,			// "Data len wrong!"						//	ERR_DATAOVERFLOW
	ERR,			// "wrong user ID",							//	ERR_NOCONDBID,
	ERR,			// "improper User",							//	ERR_IMPROPERUSER,
	ERR,			// "wrong user Password",					//	ERR_BADPASSWORD,
	ERR,			// ""Can't Register! Already registered"r"	//	ERR_ALREADYREGISTER
	ERR,			// "no user Permission",						//	ERR_PERMISSION,			
	FATALERR,		// "the shard is improper",					//	ERR_IMPROPERSHARD,		
	ERR,			// "incorrect Socket ID",					//	ERR_INVALIDCONNECTION,	
	ERR,			// "Already such a cookie exists",			//	ERR_EXISTCOOKIE,		
	INFO,			// "No front-end server available",			//	ERR_NOFRONTEND,			
	INFO,			// "the shard is full, retry in 5 minutes",	//	ERR_SHARDFULL,			
	INFO,			// "the shard is closed",					//	ERR_SHARDCLOSE,
	INFO,			// "the shard refused your login",			//	ERR_SHARDREFUSE,
	INFO,			// "the shard rejected your Connection",		//	ERR_SHARDREJECT,
	FATALERR,		// "the cookie is invalid",					//	ERR_INVALIDCOOKIE,	
	FATALERR,		// "There is no such cookie",				//	ERR_NOCOOKIE,		
	INFO,			// "There is no FTPServer to patch for such product/versionNo",	//	ERR_NOFTPSERVER
	INFO,			// "There is no FTPServer to newest version update",			// ERR_NONEWFTPSERVER
	INFO,			// "Can't Register UserData to DB",			//	ERR_DB
	ERR,			// "Auth Improper! Incorrect DBID!",		//	ERR_AUTH
	ERR,			// "User Already Online",					//	ERR_ALREADYONLINE
	ERR,			// "User Already Offline"					//	ERR_ALREADYOFFLINE
	INFO,			// "Time out"								//	ERR_TIMEOUT

	ERR,			// "Can't generate key,						//	ERR_KEYGENERATE
	ERR				// "Can't init Security Module"				//	ERR_SECRETINIT
};
