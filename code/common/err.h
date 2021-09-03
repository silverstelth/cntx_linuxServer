//////////////////////////////////////////////////////////////////////////
/* Common User file for Project(FtpDown.cpp) Created by KSR, Date: 2007.10.12 */

#ifndef _ERR_H_
#define _ERR_H_



#define		MAX_ERR_STR_LEN		256
#define		T_ERR				uint16

enum ERR_TYPE
{
	NONE,
	INFO,		// only inform
	WARN,		// inform and init
	ERR,		// inform and exit
	FATALERR	// fatal error
};

enum ENUMERR
{
	ERR_NOERR			= 0,		// success	
	E_COMMON_FAILED		= 30000,	// Common fail
	E_DBERR				= 30001,	// DB error

/***********************************
	Main Site Error
***********************************/

	ERR_CANTCONNECT		= 31000,	// Can't Connect
	ERR_MEMORY,						// memory err
	ERR_DATAOVERFLOW,				// Data len wrong!
	ERR_NOCONDBID,					// user of this dbid, has no connection
	ERR_IMPROPERUSER,				// improper User
	ERR_BADPASSWORD,				// wrong user Password
	ERR_ALREADYREGISTER,			// Can't Register! Already registered
	ERR_PERMISSION,					// no user Permission
	ERR_INVALIDCONNECTION,			// incorrect Socket ID
	ERR_IMPROPERSHARD,				// the shard is improper
	ERR_EXISTCOOKIE,				// Already such a cookie exists
	ERR_NOFRONTEND,					// No front-end server available
	ERR_DISCONNECTION,				// Already Disconnected Socket ID
	ERR_BADPASSWORD2,				// wrong user Password2
	ERR_INVALIDROOMID,				// Invalid RoomID
	ERR_NOACTIVEACCOUNT,			// No active account
	ERR_INVALIDVERSION,				// Client version is invalid

	ERR_CARD_BADID		= 31050,	// Card ID not exist
	ERR_CARD_BADPWD,				// Card Password Invalid
	ERR_CARD_USED,					// Card already used
	ERR_CARD_TIMEOUT,				// Card already timeout
	ERR_CARD_STOP,					// Card stop

	// Shard
	ERR_SHARDFULL		= 31100,	// the shard is full, retry in some minutes
	ERR_SHARDCLOSE,					// the shard is closed
	ERR_NONEED_CHANGE_SHARD,		// Notice that no need to ChangeShard

	// Auth
	ERR_INVALIDLOGINCOOKIE = 31200,	// the LoginCookie is invalid
	ERR_NOCOOKIE,					// There is no such cookie	
	ERR_NOPATCHSERVER,				// There is no FTPServer to patch for such product/versionNo
	ERR_NONEWPATCHSERVER,			// There is no UpdatePatch Server to newest version update
	ERR_AUTH,						// "Auth Improper! Incorrect DBID!"
	ERR_ALREADYONLINE,				// "User Already Online! We'll disconnect old!"
	ERR_ALREADYONLINE_GM,			// "already in use with GM! We'll disconnect you!"
	ERR_ALREADYOFFLINE,				// "User Already Offline"
	ERR_STOPEDUSER,
	ERR_NONACTIVEUSER,				// User is not activated
	ERR_TIMEOUT,					// "Time out"
	ERR_INVALIDCONNECTIONCOOKIE,	// the ConnectCookie is invalid
	// error
	ERR_GM_AUTHIP,					// unregistered GM IP
	ERR_GM_NOACTIVATE,				// no activate GM
	ERR_GM_PRIV,					// no Privillige
	ERR_GM_NOTYPE,					// no GM
	ERR_GM_PRIV_EXPMAN,				// no ExpRoom Manager
	ERR_GM_LOBBYOP,					// In Lobby, invalid operation
	ERR_GM_NOTOUCHGM,				// No Touch GM

	// Master - Slave
//	ERR_UNKNOWNSLAVE,				// Unknown Slave SS

/***********************************
		Shard Error
***********************************/

	// Common
	E_ALREADY_EXIST = 32000,		// Already exist
    E_IMPROPER_ID,					// ID is improper
	E_NO_EXIST,						// No content
	E_NONUSEFUL,					// Improper request
	E_NOMONEY,						// No money

	E_MASTERDATA,					// MasterData Error
	E_ROOMDATA,						// RoomData Error
	E_LOWLEVELROOM,					// Can't promotion to low level room
	E_NOEMPTY,						// Can't promotion because room is not empty

	E_FLOWERWATER_WAIT,				// Give water to flower per 3 hours
	E_FLOWER_USING,					// Other User is using flower.

	E_LIUYAN_WAIT,					// Liuyan can 10 timer per day in a room.

	E_BLESSLAMP_OVER,				// Blesslamp can 1 timer per day in a room.
	E_BLESSLAMP_WAIT,				// no desk for BlessLamp

	E_LETTER_USING,					// Other User is writting letter.
	E_LETTER_MODIFYING,				// Other User is modifying letter.
	E_LETTER_DELETED,				// Letter is deleted.

	E_MEMO_MODIFYING,				// Other User is modifying memo.

	E_ANCESTOR_DECEASED_USING,		// Other User is acting in ancestore deceased.

	E_KONGZI_HWANGDI_ACT_WAIT,		// Kongzi / Huangdi act is full

	// ë°????
	E_OVER_ROOMNUM = 32100,			// Room count is over
	E_NOT_MASTER,					// Isn't maste of room
	E_IMPROPER_ROOMID,				// Room id is invalid
	E_IMPROPER_ROOMKINDID,			// Room kind is invalid
	E_ROOM_FULLOFDEAD,				// Dead of room is over
	E_NODELFORONEIS,				// There is family in room
	E_EXCEED_MAXDAYS,				// Exceed max used days of room
	E_NOT_MANAGER,					// Isn't manager of room
	E_NODELFOROPEN,					// Can't delete room because room is open
	E_NOUNDELFOREXP,				// Can't undelete room because 7 day experienced
	E_EXPROOM_ALREADY,				// Already set exproom.
	E_EXPROOM_NOCREATE,				// Not created room, so can't set exproom.
	E_CREATEROOMID_FAIL,			// Create RoomID failed in CreateRoom.

	// ?¨ê³„?¸ê???
	E_CONT_ALREADY_ADD = 32200,		// Contact is already
	E_CONT_ADD_SELF,				// Oneself cannot be contact
	E_CONT_NOEXIST_USERGRP,			// No contact group
	E_FAMILY_NOEXIST,				// No family
	E_CONT_NOTDONE_REQUEST,			// No add contact reqest
	E_CONT_NO_CONTACT,				// Cannot change group, because no contact
	E_CONT_CHANGE_SAMEGRP,			// Group is equal
	E_CONT_GRPNAME_DEF,				// Group name is alredy
	E_CONT_NONEIGHBOR,				// No contact
	E_CONT_INVALIDCONTTYPE,			// Invalid Contact Type
	E_CONT_EXCEED_MAXNUM,			// Exceed limit number of contact

	// ê°?ì¡±ê???
	E_ALREADY_FAMILY = 32300,		// Family is already
	E_FAMILY_DOUBLE,				// Family name is double
	E_INVALID_CH_PARAM,				// ID is invalid
	E_CH_FULL,						// Character is full
	E_CHNAME_DOUBLE,				// Charcater name is double

	// ë°??ë¦????
	E_NOCONT_FORADDMANAGER = 32400,	// Cannot be manager, because no contact
	E_ALREADY_MANAGER,				// Already manager
	E_NOONLINEFORCHMASTER,			// Offline family
	E_NOCONT_FORCHMASTER,			// Cannot be master, because no contact

	// ë°?§„?…ê???
	E_ENTER_NO_FULL = 32500,		// Room is full
	E_ENTER_NO_ONLYMASTER,			// Not master of room
	E_ENTER_NO_NEEDACCEPT,			// No Accept
	E_ENTER_NO_NEIGHBOR,			// No neiboure
	E_ENTER_NO_BREAK,				// You are break
	E_ENTER_EXPIRE,					// Room is expire
	E_ENTER_CLOSE,					// Room is freeze
	E_ENTER_CHANGE_MASTER,			// Room is in change master
	E_ENTER_DELETED,				// Room is deleted
	E_ENTER_NO_PASSWORD,			// Room can't FastEnter because room is protected by password
	E_ENTER_FENGTING,

	// ë°?–‘?„ê???
	E_ROOMGIVE_LIMITTIME = 32600,	// Limit time for give room
	E_ROOMGIVE_ALREADY,				// Giving room is doing already
	E_ROOMGIVE_NOCONTACT,			// Cannot give room, because no contact
	E_ROOMGIVE_NOEMPTY,				// Cannot give room, because room is not empty
	E_ROOMGIVE_SYSTEMACCOUNT,		// System account can't giveroom
	E_ROOMGIVE_PWD2_OK,				// Password2 OK

	// Lobby
	E_LOBBY_FULL		= 32700,	// Lobby to enter is full, so wait
	E_LOBBY_CLOSED,					// Lobby to enter is closed that wait user is also full

	//Upload/Download
	E_PHOTO_SERVER_BUSY	= 32800,	// Data server connection big than limit, so can't upload/download
	E_PHOTO_NO_SERVER,				// Data server no exist, so can't upload/download
	E_PHOTO_HISSPACE_FULL,			// His Space Full
	E_FILE_DOWNLOADING,				// Can't delete bcause file is downloading.
	E_FILE_NOCHECKED,				// Photo or Video file no checked.

	// Deceased Data
	E_DECEASED_NODATA = 32900,		// Deceased data not exist

	// Mail
	E_RMAILBOX_OVERFLOW = 33000,	// Receiver's Mailbox Overflow
	E_CANNOTTSENDITEMTOGENERAL,		// Can't send item to general user
	E_INBLACKLIST,					// Can't send mail because he register you in black list

	// 
	E_PAPER_R_ALREADY_SIT,			// Already manager

	// RoomStore
	E_ROOMSTORE_FULL = 33100,		// RoomStore is full
	E_ROOMSTORE_USING,				// Other User is adding roomstore items.
	E_ROOMSTORE_LOCK_NOPERMISSION,	// No permission for use
	E_ROOMSTORE_LOCK_NOITEM,		// Not items for use
	E_ROOMSTORE_GET_NOPERMISSION,	// No permission for get
	E_ROOMSTORE_GET_NOITEM,			// Not items for use

	// BlessCard
	E_BLESSCARD_BADROOM = 33200,	// room is not Blessed room.
	E_BLESSCARD_ALREADY_GET,		// already get blesscard in this room.
	E_BLESSCARD_FULL,				// blesscard count > MAX_BLESSCARD_COUNT
	E_BLESSCARD_SEND_NOTARGET,		// when send blesscard, target no exist
	E_BLESSCARD_SEND_NOCARD,		// when send blesscard, blesscard no exist

	// LargeAct
	E_LARGEACT_NOACCEPT = 33300,	// LargeAct is not accepting
	E_LARGEACT_ALREADYREQUEST,		// LargeAct is already request
	E_LARGEACT_FULL,				// LargeAct is full
	E_LARGEACT_NOACCEPTTIME,		// LargeAct is not accepting time
/***********************************
		Data server error
***********************************/
	E_UPLOAD_FAILED					= 34000,
	E_CANT_CONVERT_MEDIA			= 34001,
	E_FILE_SIZE_TOO_BIG				= 34002,
	E_UPLOADING_MEDIA				= 34003,
	E_UPLOADING_PHOTO				= 34004,
	E_DOWNLOAD_FAILED				= 34010,

	E_MULTILANTERN_INVALIDREQ		= 34100,	// Invalid request
	E_MULTILANTERN_INVALIDPOS,	// Invalid multilantern position	
	E_MULTILANTERN_NOEXIST,		// Invalid multilanter id
	E_MULTILANTERN_NOJOIN,		// Can't join

	E_CANT_DEL_SBY	= 35000,	
	E_CANT_EDIT_SBY = 35001,	
	E_CANT_ADD_SBY = 35002,		
	E_SBY_USERFULL = 35100,		
	E_SBY_CANTJOIN = 35102,
	E_SBY_ALREADYJOIN = 35103,
	E_PBY_CANTMAKE = 35200,	
	E_PBY_ALIVE = 35201,
	E_PBY_CANTJOIN = 35202,
	E_PBY_CANTSET = 35203,

	E_MULTIACT_NO = 35300,
	E_MULTIACT_FULL = 35301,
	E_MULTIACT_STARTED = 35302,

	E_ALREADY_FOJIAOYISHI = 36000,
	E_ALREADY_FOJIAOTOUXIANG = 36001,

	E_ALREADY_JISIYISHI = 36002, // by krs
	E_ALREADY_JISITOUXIANG = 36003,

	E_ACT_CANTREQ = 37000,
	E_ACT_REMOVED = 37001,

	/***********************************
		Hunqing error
	***********************************/
	E_HUNQING_NOTKOEN = 40000,
};



extern ERR_TYPE	typeErrAry[];


#endif //_ERR_H_
