//////////////////////////////////////////////////////////////////////////
/* Common file for Project(FtpDown.cpp) Created by KSR, Date: 2007.10.12 */

#ifndef _COMMON_H_
#define _COMMON_H_

#pragma once

#include "err.h"

#define		INVALID_USER			0xFFFFFFFF
#define		INVALID_SHARD			0xFFFFFFFF
#define		MAX_PACKET_BUF_LEN		4096		// 4K


enum ENUM_USERSTATE
{ 
	STATE_NONE,
	STATE_CONNECT,
	STATE_WAITSSAUTH,
	STATE_SSAUTH,
	STATE_CONNECTSHARD,
	STATE_SHARDVALID,
	STATE_SHARDIDENT,
	STATE_DISCONNECT
};

enum ENUM_USERTYPE
{
	USERTYPE_COMMON = 1,
	USERTYPE_MIDDLE = 2,
	USERTYPE_SUPER = 10,
	USERTYPE_DEVELOPER = 100,
	USERTYPE_GAMEMANAGER = 200,
	USERTYPE_GAMEMANAGER_H = 201,
	USERTYPE_EXPROOMMANAGER = 210,
	USERTYPE_WEBMANAGER = 211,
};

enum ENUM_ADMIN_POWER	{ADMIN_POWER_FULL = 1, ADMIN_POWER_SHARD = 100};

#endif // _COMMON_H_