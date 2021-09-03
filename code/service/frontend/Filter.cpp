#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>
#include <net/SecurityServer.h>
#include <misc/stream.h>

#include <map>
#include <utility>

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"

#include "shard_client.h"
#include "main.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

CVariable<bool> LogPacketFilterFlag("FES","LogPacketFilterFlag", "Flag of received packet from client", 0, 0, true);

CFileDisplayer		PacketFilterFile;
CLog				PacketFilterLog;

#define PACKETFILTERLOG if ( SIPBASE::g_bLog ) createDebug(), PacketFilterLog.setPosition( __LINE__, __FILE__, __FUNCTION__ ), PacketFilterLog.displayNL

static	bool			bPacketFiltering = true;
static	const			uint32	DEFAULT_SERIAL_INTERVAL		= 500;	//ms
static	const			uint32	DEFAULT_MAX_RESPONSETIME	= 5000;	//second


// Packet filtering data structure
struct PACKETFILTERDATA 
{
#ifdef SIP_MSG_NAME_DWORD
	uint32		sInputPacket;			// Input packet
	uint32		sOutputPacket;			// Response packet
#else
	char		*sInputPacket;			// Input packet
	char		*sOutputPacket;			// Response packet
#endif // SIP_MSG_NAME_DWORD
	int			nMaxInputNum;			// Max count of input
	int			nSerialIntervalTime;	// Serial interval time(ms)
	int			nMaxSerialNum;			// Serial count
	const char	*ParamFormat;
};

PACKETFILTERDATA	PacketFilterData[] = 
{
	//{M_CS_LOGOUT,					M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_FAMILY,					M_SC_FAMILY,				-1,				-1,			5,	""	},
	{M_CS_NEWFAMILY,				M_SC_NEWFAMILY,				-1,				-1,			5,	""	},

	{M_CS_USERLIST,					M_SYS_EMPTY,				-1,				30000,		1,	""	},

	//{M_CS_FNAMECHECK,				M_SC_FNAMECHECK,			-1,				-1,			5,	""	},
	//{M_CS_DELFAMILY,				M_SC_DELFAMILY,				-1,				-1,			5,	""	},
	{M_CS_CHCHARACTER,				M_SYS_EMPTY,				-1,				5000,		2,	""	},

	//{M_CS_EXPROOMS,					M_SC_EXPROOMS,				-1,				-1,			5,	""  },	
	//{M_CS_TOPLIST,					M_SC_TOPLIST_FAMILYEXP,		-1,				-1,			5,	""  },	

	//{M_CS_BESTSELLITEM,				M_SC_BESTSELLITEM,			-1,				-1,			5,	""  },	
	//{M_CS_FITEMPOS,					M_SC_FITEMPOS,				-1,				-1,			5,	""	},
	//{M_CS_USEITEM,					M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_FPROPERTY_MONEY,			M_SYS_EMPTY,				-1,				50000,		10,	""	},
	{M_CS_INVENSYNC,				M_SC_INVENSYNC,				-1,				1000,		5,	""	},
	{M_CS_BUYITEM,					M_SC_BUYITEM,				-1,				-1,			5,	""	},
	//{M_CS_THROWITEM,				M_CS_THROWITEM,				-1,				-1,			5,	""	},
	//{M_CS_TIMEOUT_ITEM,				M_SC_TIMEOUT_ITEM,			-1,				-1,			5,	""	},
	//{M_CS_TRIMITEM,					M_SYS_EMPTY/*M_SC_TRIMITEM*/,				-1,				-1,			5,  ""  },

	//{M_CS_BANKITEM_LIST,			M_SC_BANKITEM_LIST,			-1,				-1,			5,  ""  },
	//{M_CS_BANKITEM_GET,				M_SYS_EMPTY,				-1,				500,		5,  ""  },

	{M_CS_BUYHISSPACE,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_EXPERIENCE,				M_SYS_EMPTY,				1,				500,		5,	""	},
	{M_CS_FINDFAMILY,				M_SC_FINDFAMILY,			-1,				-1,			5,	""	},
	//{M_CS_FAMILYCOMMENT,			M_SC_FAMILYCOMMENT,			-1,				-1,			5,	""	},
	{M_CS_RESCHIT,					M_SYS_EMPTY,				-1,				10,			5,	""	},

	{M_CS_NEWROOM,					M_SC_NEWROOM,				-1,				-1,			5,	""	},
	{M_CS_ROOMCARD_NEW,				M_SC_NEWROOM,				-1,				-1,			5,	""	},

	{M_CS_RENEWROOM,				M_SC_RENEWROOM,				-1,				-1,			5,	""	},
	//{M_CS_ROOMCARD_RENEW,			M_SC_RENEWROOM,				-1,				-1,			5,	""	},

	//{M_CS_PROMOTION_ROOM,			M_SC_PROMOTION_ROOM,		-1,				-1,			5,	""	},
	//{M_CS_ROOMCARD_PROMOTION,		M_SC_PROMOTION_ROOM,		-1,				-1,			5,	""	},
	{M_CS_GET_PROMOTION_PRICE,		M_SC_PROMOTION_PRICE,		-1,				-1,			5,	""	},


	//{M_CS_ROOMCARD_CHECK,			M_SC_ROOMCARD_CHECK,		-1,				10000,		5,	""	},

	{M_CS_FINDROOM,					M_SC_FINDROOMRESULT,		-1,				-1,			5,	""	},
	//{M_CS_SETROOMPWD,               M_SC_SETROOMPWD,            -1,             -1,         5,  ""  },
	{M_CS_ROOMINFO,					M_SC_ROOMINFO,				-1,				-1,			5,	""	},
	//{M_CS_INSFAVOR_CONTACTROOM,		M_SC_FAVORROOM,				-1,				-1,			5,	""	},
	//{M_CS_GETRMAN,					M_SC_GETRMAN,				-1,				-1,			5,	""	},
	{M_CS_ADDCONTGROUP,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_DELCONTGROUP,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_RENCONTGROUP,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_SETCONTGROUP_INDEX,       M_SYS_EMPTY,				-1,             500,         5,  ""	},
	{M_CS_ADDCONT,					M_SYS_EMPTY,				-1,				500,			5,	""	},
	{M_CS_DELCONT,					M_SYS_EMPTY,				-1,				500,			5,	""	},
	{M_CS_CHANGECONTGRP,			M_SYS_EMPTY,				-1,				500,			5,	""	},
	//{M_CS_CONTFAMILY,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_ROOMFORCONTACT,			M_SC_ROOMFORFAMILY,			-1,				-1,			5,	""	},
	{M_CS_SELFSTATE,				M_SYS_EMPTY,				1,				500,		5,	""	},
	{M_CS_REGFESTIVAL,				M_SC_REGFESTIVAL,			-1,				-1,			5,	""	},
	{M_CS_MODIFYFESTIVAL,			M_SC_MODIFYFESTIVAL,		-1,				-1,			5,	""	},
	{M_CS_DELFESTIVAL,				M_SC_DELFESTIVAL,			-1,				-1,			5,	""	},
	{M_CS_ENTER_LOBBY,				M_SC_ENTER_LOBBY,			-1,				-1,			5,	""	},
	{M_CS_LEAVE_LOBBY,				M_SC_LEAVE_LOBBY,			-1,				-1,			5,	""	},
	//{M_CS_REQ_CHANGECH,				M_SC_REQ_CHANGECH,			-1,				-1,			5,	""	},
	//{M_CS_CHECK_ENTERROOM,			M_SC_CHECK_ENTERROOM,		-1,				-1,			120,	""	},
	{M_CS_REQEROOM,					M_SC_REQEROOM,				-1,				-1,			5,	""	},
	{M_CS_CHANNELLIST,				M_SC_CHANNELLIST,			-1,				1000,		2,	""	},
	{M_CS_ENTERROOM,				M_SC_ENTERROOM,				-1,				-1,			5,	""	},
	{M_CS_CHANGECHANNELIN,			M_SC_CHANGECHANNELIN,		-1,				-1,			5,	""	},
	{M_CS_CHANGECHANNELOUT,			M_SC_OUTROOM,				-1,				-1,			5,	""	},
	{M_CS_OUTROOM,					M_SC_OUTROOM,				-1,				-1,			5,	""	},
	//{M_CS_MOVECH,					M_SYS_EMPTY,				-1,				300,		200,""},
	//{M_CS_STATECH,					M_SYS_EMPTY,				-1,				300,		200,""},
	{M_CS_ACTION,					M_SYS_EMPTY,				-1,				-1,			5,""},
	{M_CS_DECEASED,					M_SC_DECEASED,				-1,				-1,			5,	""	},
	{M_CS_CHATROOM,					M_SYS_EMPTY,				-1,				500,		12,	""	},
	{M_CS_CHATCOMMON,				M_SYS_EMPTY,				-1,				500,		12,	""	},
	{M_CS_CHATEAR,					M_SYS_EMPTY,				-1,				500,		12,	""	},
	//{M_CS_ROOMPARTCHANGE,			M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_SETROOMINFO,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_CHATALL,					M_SYS_EMPTY,				-1,				500,		12,	""	},
	{M_CS_SETDEAD,					M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_SET_DEAD_FIGURE,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_DELDEAD,					M_SC_DELDEAD,				-1,				-1,			5,	""	},

	{M_CS_INVITE_ANS,				M_SYS_EMPTY,				-1,				500,			5,	""	},
	{M_CS_INVITE_REQ,				M_SYS_EMPTY,				-1,				500,			5,	""	},
	{M_CS_BANISH,					M_SYS_EMPTY,				-1,				500,			5,	""	},

	{M_CS_MANAGER_ADD,				M_SYS_EMPTY,				-1,				1000,			10,	""	},
	{M_CS_MANAGER_ANS,				M_SYS_EMPTY,				-1,				1000,			2,	""	},
	{M_CS_MANAGER_DEL,				M_SYS_EMPTY,				-1,				1000,			10,	""	},
	{M_CS_MANAGER_GET,				M_SC_MANAGER_LIST,			-1,				500,			5,	""	},
	{M_CS_MANAGEROOMS_GET,			M_SC_MANAGEROOMS_LIST,		-1,				500,			5,	""	},
	{M_CS_MANAGER_SET_PERMISSION,	M_SYS_EMPTY,				-1,				-1,			5,	""	},

	//{M_CS_DELRMEMO,					M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_ADDRMAN,					M_SC_ADDRMAN,				-1,				-1,			5,	""	},
	//{M_CS_DELRMAN,					M_SC_DELRMAN,				-1,				-1,			5,	""	},
	{M_CS_TOMBSTONE,				M_SC_TOMBSTONE,				-1,				-1,			5,	""	},
	{M_CS_RUYIBEI_TEXT,				M_SC_RUYIBEI_TEXT,			-1,				-1,			5,	""	},
	//{M_CS_EVENT,					M_SC_EVENT,					-1,				-1,			5,	""	},
	//{M_CS_ADDEVENT,					M_SC_ADDEVENT,				-1,				-1,			5,	""	},
	//{M_CS_ENDEVENT,					M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_CHANGEMASTER,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_CHANGEMASTER_CANCEL,		M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_CHANGEMASTER_ANS,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_DELROOM,					M_SC_DELROOM,				-1,				-1,			5,	""	},
	{M_CS_UNDELROOM,				M_SC_UNDELROOM,				-1,				-1,			5,	""	},
	//{M_CS_HOLDITEM,					M_SC_HOLDITEM,				-1,				-1,			5,	""	},
	//{M_CS_PUTHOLEDITEM,				M_SYS_EMPTY,				-1,				500,		5,	""	},

	{M_CS_FISH_FOOD,				M_SC_UPDATE_FISH,			-1,				-1,			5,	""	},
	{M_CS_FLOWER_NEW,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_FLOWER_WATER,				M_SYS_EMPTY,				-1,				20000,		2,	""	},
	{M_CS_FLOWER_END,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_FLOWER_NEW_REQ,			M_SC_FLOWER_NEW_ANS,		-1,				-1,			5,	""	},
	{M_CS_FLOWER_WATER_REQ,			M_SC_FLOWER_WATER_ANS,		-1,				-1,			5,	""	},
	{M_CS_FLOWER_WATERTIME_REQ,		M_SC_FLOWER_WATERTIME_ANS,	-1,				-1,			5,	""	},
	{M_CS_GOLDFISH_CHANGE,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_NEWPET,					M_SC_NEWPET,				-1,				-1,			5,	""	},
	//{M_CS_CAREPET,					M_SYS_EMPTY,				-1,				500,		5,	""	},

	{M_CS_LUCK_REQ,					M_SYS_EMPTY,				-1,				5000,			1,	""	},
	//{M_CS_LUCK4_LIST,				M_SC_LUCK4_LIST,			-1,				5000,			1,	""	},

//	{M_CS_DRUM_START,				M_SYS_EMPTY,				-1,				3000,			1,	""	},
	{M_CS_DRUM_END,					M_SYS_EMPTY,				-1,				3000,			1,	""	},
//	{M_CS_BELL_START,				M_SYS_EMPTY,				-1,				5000,			1,	""	},
	{M_CS_BELL_END,					M_SYS_EMPTY,				-1,				5000,			1,	""	},
	{M_CS_MUSIC_START,				M_SYS_EMPTY,				-1,				5000,			1,	""	},
	{M_CS_MUSIC_END,				M_SYS_EMPTY,				-1,				5000,			1,	""	},

//	{M_CS_ANIMAL_APPROACH,			M_SYS_EMPTY,				-1,				500,		10,	""	},
//	{M_CS_ANIMAL_STOP,				M_SYS_EMPTY,				-1,				500,		20,	""	},
//	{M_CS_ANIMAL_SELECT,			M_SYS_EMPTY,				-1,				500,		20,	""	},
//	{M_CS_ANIMAL_SELECT_END,		M_SYS_EMPTY,				-1,				500,		20,	""	},

	{M_CS_EAINITINFO,				M_SYS_EMPTY,				-1,				500,		5,	""},
	{M_CS_EAMOVE,					M_SYS_EMPTY,				-1,				500,		5,	""},
//	{M_CS_EAANIMATION,				M_SYS_EMPTY,				-1,				500,		5,	""},
	{M_CS_EABASEINFO,				M_SYS_EMPTY,				-1,				500,		5,	""},
	{M_CS_EAANISELECT,				M_SYS_EMPTY,				-1,				500,		5,	""},

	//{M_CS_BURNITEM,					M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_ROOMMUSIC,				M_SC_ROOMMUSIC,				-1,				-1,			5,	""	},
	//{M_CS_ENTERTGATE,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_GETALLINROOM,				M_SC_GETALLINROOM,			-1,				-1,			5,	""	},
	//{M_CS_REQFORURL,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_FORCEOUT,					M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_ROOMFORFAMILY,			M_SC_ROOMFORFAMILY,			-1,				-1,			5,	""	},
	//{M_CS_PUTITEM,					M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_REQFORPHOTO,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_REQPORTRAIT,				M_SC_REQPORTRAIT,			-1,				-1,			5,	""	},
	//{M_CS_ROOMFAMILYCOMMENT,		M_SC_ROOMFAMILYCOMMENT,		-1,				-1,			5,	""	},
	//{M_CS_BURNITEMINS,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_FORCESTATECH,				M_SYS_EMPTY,				-1,				500,		100,	""	},
	{M_CS_NEWRMEMO,					M_SC_NEWRMEMO,				-1,				60000,		10,	""	},
	//{M_CS_MODIFYRMEMO,				M_SYS_EMPTY,				-1,             500,        5,  ""	},
	//{M_CS_REQRMEMO,					M_SC_ROOMMEMO,				-1,				-1,			5,	""	},
	//{M_CS_MODIFY_MEMO_START,		M_SYS_EMPTY,				-1,             500,        5,  ""	},
	//{M_CS_MODIFY_MEMO_END,			M_SYS_EMPTY,				-1,             500,        5,  ""	},
	//{M_CS_ROOMMEMO_GET,				M_SC_ROOMMEMO_DATA,			-1,				-1,			5,	""	},

	{M_CS_ACT_REQ,					M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_ACT_WAIT_CANCEL,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_ACT_RELEASE,				M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_ACT_START,				M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_ACT_STEP,					M_SYS_EMPTY,				-1,				2000,			15,	""	},

	{M_CS_MULTIACT2_ITEMS,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT2_ASK,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT2_JOIN,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_REQ,				M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_ANS,				M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_READY,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_GO,				M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_START,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_RELEASE,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_CANCEL,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_COMMAND,			M_SYS_EMPTY,				-1,				2000,			2,	""	},
	{M_CS_MULTIACT_REPLY,			M_SYS_EMPTY,				-1,				2000,			2,	""	},

	{M_CS_CRACKER_BOMB,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_LANTERN,					M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_ITEM_MUSICITEM,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_ITEM_FLUTES,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_ITEM_VIOLIN,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_ITEM_GUQIN,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_ITEM_DRUM,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_ITEM_FIRECRACK,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_ITEM_RAIN,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_ITEM_POINTCARD,			M_SYS_EMPTY,				-1,				-1,			5,	""	},

	{M_CS_CHAIR_SITDOWN,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_CHAIR_STANDUP,			M_SYS_EMPTY,				-1,				-1,			5,	""	},

//	{M_CS_REQRVISITLIST,			M_SC_ROOMVISITLIST,			-1,				-1,			5,	""	},
	{M_CS_ADDACTIONMEMO,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_VISIT_DETAIL_LIST,		M_SC_VISIT_DETAIL_LIST,		-1,				-1,			5,	""	},
	{M_CS_VISIT_DATA,				M_SC_VISIT_DATA,			-1,				-1,			5,	""	},
	{M_CS_VISIT_DATA_SHIELD,		M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_VISIT_DATA_DELETE,		M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_VISIT_DATA_MODIFY,		M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_GET_ROOM_VISIT_FID,		M_SC_ROOM_VISIT_FID,		-1,				-1,			5,	""	},

	{M_CS_REQ_PAPER_LIST,			M_SC_PAPER_LIST,			-1,				-1,			5,	""	},
	{M_CS_REQ_PAPER,				M_SC_PAPER,					-1,				-1,			5,	""	},
	{M_CS_NEW_PAPER,				M_SC_NEW_PAPER,				-1,				-1,			5,	""	},
	{M_CS_DEL_PAPER,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_MODIFY_PAPER,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_MODIFY_PAPER_START,		M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_MODIFY_PAPER_END,			M_SYS_EMPTY,				-1,				-1,			5,	""	},

	// Mail
	{M_CS_MAIL_GET_LIST,			M_SC_MAIL_LIST,				-1,				-1,			5,	""	},
	{M_CS_MAIL_GET,					M_SC_MAIL,					-1,				-1,			5,	""	},
	{M_CS_MAIL_SEND,				M_SC_MAIL_SEND,				-1,				-1,			5,	""	},
	{M_CS_MAIL_DELETE,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_MAIL_REJECT,				M_SC_MAIL_REJECT,			-1,				-1,			5,	""	},
	{M_CS_MAIL_ACCEPT_ITEM,			M_SYS_EMPTY,				-1,				500,			5,	""	},

	//{M_CS_NEWSACRIFICE,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	//{M_CS_CHMETHOD,					M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_REGISTER_PEER,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_UNREGISTER_PEER,			M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_CONNECT_PEER_OK,			M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_JOIN_SESSION,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_EXIT_SESSION,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_SPEAK_TO_SESSION,			M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_REQUEST_TALK,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_RESPONSE_TALK,			M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_HANGUP_TALK,				M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_REQ_CONNECT_PEER,			M_SYS_EMPTY,				-1,				500,		5,	""	},		
	{M_CS_RES_CONNECT_PEER,			M_SYS_EMPTY,				-1,				500,		5,	""	},

	//{M_CS_CHDESC,				    M_SC_CHDESC,                -1,             -1,         5,	""   },
	{M_CS_CHANGE_CHDESC,            M_SYS_EMPTY,				-1,             500,        5,	""   },

	{M_CS_SET_FAMILY_FIGURE,		M_SYS_EMPTY,				-1,             10000,		2,	""   },
	{M_CS_GET_FAMILY_FIGURE,		M_SYS_EMPTY,				-1,             500,		50,	""   },
	//{M_CS_SET_FAMILY_FACEMODEL,		M_SYS_EMPTY,				-1,             10000,		2,	""   },
	//{M_CS_GET_FAMILY_FACEMODEL,		M_SYS_EMPTY,				-1,             500,		50,	""   },

	{M_CS_REN_FAVORROOMGROUP,       M_SYS_EMPTY,				-1,             500,         5,	""   },
	{M_CS_DEL_FAVORROOMGROUP,       M_SYS_EMPTY,				-1,             500,         5,	""   },
	{M_CS_NEW_FAVORROOMGROUP,       M_SYS_EMPTY,				-1,             500,         5,	""   },
	{M_CS_MOVFAVORROOM,             M_SYS_EMPTY,				-1,				500,         5,	""   },
	{M_CS_INSFAVORROOM,				M_SYS_EMPTY,				-1,				500,			5,	""	},
	{M_CS_DELFAVORROOM,				M_SYS_EMPTY,				-1,				500,			5,	""	},
	{M_CS_SET_FESTIVALALARM,        M_SC_SET_FESTIVALALARM,     -1,             -1,         5,	""	},

	{M_CS_ADDWISH,					M_SYS_EMPTY,				-1,				500,		5,	""	},
	{M_CS_WISH_LIST,				M_SC_WISH_LIST,				-1,				500,		5,	""	},
	{M_CS_MY_WISH_LIST,				M_SC_MY_WISH_LIST,			-1,				500,		5,	""	},
	{M_CS_MY_WISH_DATA,				M_SC_MY_WISH_DATA,			-1,				500,		5,	""	},

	//{M_CS_R_PAPER_START,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_R_PAPER_PLAY,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_R_PAPER_END,				M_SYS_EMPTY,				-1,				-1,			5,	""	},

	{M_CS_VIRTUE_LIST,				M_SC_VIRTUE_LIST,			-1,				-1,			5,	""	},
	{M_CS_VIRTUE_MY,				M_SC_VIRTUE_MY,				-1,				-1,			5,	""	},

	{M_CS_R_ACT_INSERT,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_R_ACT_MODIFY,				M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_R_ACT_LIST,				M_SC_R_ACT_LIST,			-1,				-1,			5,	""	},
	{M_CS_R_ACT_MY_LIST,			M_SC_R_ACT_MY_LIST,			-1,				-1,			5,	""	},
	{M_CS_R_ACT_PRAY,				M_SC_R_ACT_PRAY,			-1,				-1,			5,	""	},

	{M_CS_ANCESTOR_TEXT_SET,		M_SC_ANCESTOR_TEXT,			-1,				-1,			5,	""	},
	{M_CS_ANCESTOR_DECEASED_SET,	M_SYS_EMPTY,				-1,				-1,			5,	""	},

	{M_CS_AUTOPLAY_REQ_BEGIN,		M_SYS_EMPTY,				-1,				-1,			5,	""	},
	//{M_CS_AUTOPLAY_REQ,				M_SYS_EMPTY,				-1,				-1,			120,	""	},

	{M_CS_AUTOPLAY2_REQ,			M_SYS_EMPTY,				-1,				-1,			5,	""	},

	{M_CS_REQ_ACTIVITY_ITEM,		M_SYS_EMPTY,				-1,				1000,		2,	""	},
	{M_CS_RECV_GIFTITEM,			M_SYS_EMPTY,				-1,				1000,		2,	""	},

	{M_CS_GET_ROOMSTORE,			M_SC_ROOMSTORE,				-1,				-1,			5,	""	},
	{M_CS_ADD_ROOMSTORE,			M_SC_ADD_ROOMSTORE_RESULT,	-1,				-1,			5,	""	},
	{M_CS_GET_ROOMSTORE_HISTORY,	M_SC_ROOMSTORE_HISTORY,		-1,				-1,			5,	""	},
	{M_CS_GET_ROOMSTORE_HISTORY_DETAIL,	M_SC_ROOMSTORE_HISTORY_DETAIL,	-1,				-1,			5,	""	},
	{M_CS_ROOMSTORE_LOCK,			M_SC_ROOMSTORE_LOCK,		-1,				-1,			5,	""	},
	{M_CS_ROOMSTORE_UNLOCK,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_ROOMSTORE_LASTITEM_SET,	M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_ROOMSTORE_HISTORY_MODIFY,	M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_ROOMSTORE_HISTORY_DELETE,	M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_ROOMSTORE_HISTORY_SHIELD,	M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_ROOMSTORE_GETITEM,		M_SYS_EMPTY,				-1,				-1,			5,	""	},

	{M_CS_BLESSCARD_GET,			M_SYS_EMPTY,				-1,             -1,			5,	""   },
	{M_CS_BLESSCARD_SEND,			M_SYS_EMPTY,				-1,             -1,			5,	""   },

	//{M_CS_REVIEW_LIST,				M_SC_REVIEW,				-1,             -1,			5,	""   },

	{M_CS_PUBLICROOM_FRAME_LIST,	M_SC_PUBLICROOM_FRAME_LIST,	-1,				-1,			5,	""	},

	//{M_CS_LARGEACT_LIST,			M_SYS_EMPTY,				-1,				-1,			5,	""	}, //by krs

	{M_CS_LARGEACT_REQUEST,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_LARGEACT_CANCEL,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_LARGEACT_STEP,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_LARGEACT_USEITEM,			M_SYS_EMPTY,				-1,				-1,			5,	""	},

	{M_CS_CHECKIN,					M_SC_CHECKIN,				-1,				-1,			5,	""	},
	{M_CS_REQLUCKINPUBROOM,			M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_GETBLESSCARDINPUBROOM,	M_SYS_EMPTY,				-1,				-1,			5,	""	},
	{M_CS_HISLUCKINPUBROOM,			M_SYS_EMPTY,				-1,				-1,			5,	""	},

	{ M_CS_NIANFO_BEGIN,			M_SC_NIANFO_BEGIN,			-1,				-1,			5,	""	},
	{ M_CS_NIANFO_END,				M_SC_NIANFO_END,			-1,				-1,			5,	""	},
	{ M_CS_NEINIANFO_BEGIN,			M_SC_NEINIANFO_BEGIN,		-1,				-1,			5,	""	},
	{ M_CS_NEINIANFO_END,			M_SC_NEINIANFO_END,			-1,				-1,			5,	""	},
};

// Filter data
map<MsgNameType, PACKETFILTERDATA>		mapPACKETFILTERDATA;
map<MsgNameType, uint8>					mapPacketAlwaysPass;
map<MsgNameType, uint8>					mapPacketOnlyNoFamilyPass;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// History of one packet
struct ONEPACKETHISTORY 
{
	TTime		tmLastReceived;
	uint32		uAllReceivedNum;
	MsgNameType		sWaitPacket;
};

// Waiting packet
struct WAITPACKET 
{
	MsgNameType		sWaitPacket;
	MsgNameType		sInputPacket;
};

// History of one client
struct RECEIVEDPACKET 
{
	map<MsgNameType, ONEPACKETHISTORY>	Packets;
	list<WAITPACKET>				CurWaitPackets;
	uint32							NoPassCount;	
	void	DeleteWait(MsgNameType _sWaitP, MsgNameType _sInputP)
	{
		bool	bRestart = true;
		while (bRestart)
		{
			bRestart = false;
			list<WAITPACKET>::iterator it;
			for(it = CurWaitPackets.begin(); it != CurWaitPackets.end(); it++)
			{
				if (it->sWaitPacket == _sWaitP)
				{
					if (_sInputP == M_SYS_EMPTY || _sInputP == it->sInputPacket)
					{
						bRestart = true;
						CurWaitPackets.erase(it);
						break;
					}
				}
			}
		}
	}
};

map<uint32,	RECEIVEDPACKET>		mapReceivedPackets;

void	AddUserToPacketFilter(uint32 _uid)
{
	RECEIVEDPACKET	newData;
	newData.NoPassCount = 0;
	mapReceivedPackets.insert(make_pair(_uid, newData));
}

void	DelUserToPacketFilter(uint32 _uid)
{
	mapReceivedPackets.erase(_uid);
}

void	ResetPacketFilter(uint32 _uid)
{
	// filter
	map<uint32,	RECEIVEDPACKET>::iterator itAll = mapReceivedPackets.find(_uid);
	if (itAll != mapReceivedPackets.end())
	{
		itAll->second.Packets.clear();
		itAll->second.CurWaitPackets.clear();
		itAll->second.NoPassCount = 0;
	}

}

bool GetMsgCheckNeed(const std::vector<uint8>& _buffer, uint32 *_mesID, MsgNameType *_sMesID, uint32 *_intervalTime, uint32 *_serialMaxNum)
{
	if (!bPacketFiltering)
		return false;	

	uint32	uBufferSize = _buffer.size();

	const	uint32 uHeadeSize = 5;	// 5 = MessageNmber(4) + MessageType(1)
	const	uint32 MAXMESNAME = 50;

	uint32	uMessageLimitSize = uHeadeSize + sizeof(uint32);
	if (uBufferSize < uMessageLimitSize)
		return false;

#ifdef SIP_MSG_NAME_DWORD

	uint32	sPacket = *((uint32*)(&_buffer[uHeadeSize]));

#else

	uint32	uMessageNameLenPos = uHeadeSize;
	uint32	uMesNameLength = *(uint32*)(&_buffer[uMessageNameLenPos]);

	if (uMesNameLength > MAXMESNAME)
		return false;
	
//	uint32	uLengthToMesName = uMessageLimitSize + sizeof(uint32) + uMesNameLength;
//	if (uLengthToMesName > uBufferSize)
//		return false;

	uint32	uMessageNamePos = uMessageNameLenPos + sizeof(uint32);

	char mesname[MAXMESNAME+1];		memset(mesname, 0, sizeof(mesname));
	strncpy(mesname, (char *)(&_buffer[uMessageNamePos]), uMesNameLength);

	string	sPacket = mesname;

#endif // SIP_MSG_NAME_DWORD

	map<MsgNameType, PACKETFILTERDATA>::iterator itFilterData = mapPACKETFILTERDATA.find(sPacket);
	if (itFilterData != mapPACKETFILTERDATA.end())
	{
		uint32 FilterIntervalTime	= itFilterData->second.nSerialIntervalTime;
		if (itFilterData->second.nSerialIntervalTime == -1)
		{
			FilterIntervalTime = DEFAULT_SERIAL_INTERVAL;
		}
		if (FilterIntervalTime != -1)
		{
			*_mesID = (uint64)(&(itFilterData->first));
			*_intervalTime = FilterIntervalTime;
			*_sMesID = sPacket;
			*_serialMaxNum = itFilterData->second.nMaxSerialNum;
			return true;
		}
	}

	return false;
}

CFairMutex	ProcUnlawMutex;
struct UNLAWACTION 
{
	UNLAWACTION(uint8 _UnlawType, ucstring _UnlawContent, T_FAMILYID _FID) :
		UnlawType(_UnlawType), UnlawContent(_UnlawContent), FID(_FID) {}
	uint8 UnlawType;
	ucstring UnlawContent;
	T_FAMILYID FID;
};

list<UNLAWACTION>	lstUnlawAction;

void ProcUnlaw(uint8 UnlawType, ucstring UnlawContent, T_FAMILYID FID)
{
	ProcUnlawMutex.enter();

	lstUnlawAction.push_back( UNLAWACTION(UnlawType, UnlawContent, FID) );

	ProcUnlawMutex.leave();
}

void	LogUnlawAction()
{
	ProcUnlawMutex.enter();

	while (!lstUnlawAction.empty())
	{
		UNLAWACTION action = lstUnlawAction.front();

		CMessage			mesLog(M_CS_UNLAWLOG);
		mesLog.serial(action.UnlawType, action.UnlawContent, action.FID);
		CUnifiedNetwork::getInstance ()->send( LOG_SHORT_NAME, mesLog );
		
		lstUnlawAction.pop_front();
	}
	ProcUnlawMutex.leave();
}

static	bool	checkPacket()
{
	return true;
}

// Packet filtering
bool	ProcPacketFilter(MsgNameType _sPacket, TTcpSockId _from, CCallbackNetBase& _clientcb )
{
	uint32		uAccountID	= GetIdentifiedID(_from);
	if (uAccountID == 0)
	{
		sipwarning("uAccountID = 0. packet=%d", _sPacket);
		return false;
	}

	ucstring	uAccountName	= GetUserName(uAccountID);
	if (LogPacketFilterFlag)
	{
#ifdef SIP_MSG_NAME_DWORD
		// sipdebug(L"FILTER Received packet=%d, from=%s, time=%d", _sPacket, uAccountName.c_str(), CTime::getLocalTime());
		PACKETFILTERLOG(L"From=%s\t\tpacket=%d", uAccountName.c_str(), _sPacket);
#else
		// sipdebug(L"FILTER Received packet=%s, from=%s, time=%d", ucstring(_sPacket).c_str(), uAccountName.c_str(), CTime::getLocalTime());
		PACKETFILTERLOG(L"From=%s\t\tpacket=%s", uAccountName.c_str(), ucstring(_sPacket).c_str());
#endif // SIP_MSG_NAME_DWORD
	}

	if (!bPacketFiltering)
		return true;

	T_FAMILYID	FID = GetFamilyID(_from);

	// No Family
	if (FID == NOFAMILYID)
	{
		bool	bPacketAlwaysPass		= false;
		bool	bPacketOnlyNoFamilyPass	= false;

		map<MsgNameType, uint8>::iterator it1 = mapPacketAlwaysPass.find(_sPacket);
		if (it1 != mapPacketAlwaysPass.end())
			bPacketAlwaysPass = true;

		map<MsgNameType, uint8>::iterator it2 = mapPacketOnlyNoFamilyPass.find(_sPacket);
		if (it2 != mapPacketOnlyNoFamilyPass.end())
			bPacketOnlyNoFamilyPass = true;

		if (!bPacketAlwaysPass && !bPacketOnlyNoFamilyPass)
		{
#ifdef SIP_MSG_NAME_DWORD
			ucstring ucUnlawContent = SIPBASE::_toString(L"Packet(%d) cannot pass on no family : user=%s", _sPacket, uAccountName.c_str());
			sipwarning(L"Packet(%d) cannot pass on no family : user=%s", _sPacket, uAccountName.c_str());
#else
			ucstring ucUnlawContent = SIPBASE::_toString(L"Packet(%s) cannot pass on no family : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
			sipwarning(L"Packet(%s) cannot pass on no family : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
#endif // SIP_MSG_NAME_DWORD
			_clientcb.forceClose(_from);
			ProcUnlaw(UNLAW_REMARK, ucUnlawContent, 0);
			return false;
		}
	}
	else
	{
		map<MsgNameType, uint8>::iterator it1 = mapPacketOnlyNoFamilyPass.find(_sPacket);
		if (it1 != mapPacketOnlyNoFamilyPass.end())
		{
#ifdef SIP_MSG_NAME_DWORD
			ucstring ucUnlawContent = SIPBASE::_toString(L"Packet(%d) cannott pass on valid family : user=%s", _sPacket, uAccountName.c_str());
			sipwarning(L"Packet(%d) cannott pass on valid family : user=%s", _sPacket, uAccountName.c_str());
#else
			ucstring ucUnlawContent = SIPBASE::_toString(L"Packet(%s) cannott pass on valid family : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
			sipwarning(L"Packet(%s) cannott pass on valid family : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
#endif // SIP_MSG_NAME_DWORD
			_clientcb.forceClose(_from);
			ProcUnlaw(UNLAW_REMARK, ucUnlawContent, 0);
			return false;
		}
	}
	
	map<uint32,	RECEIVEDPACKET>::iterator itAll = mapReceivedPackets.find(uAccountID);
	if (itAll == mapReceivedPackets.end())
	{
		sipwarning(L"There is no packet filter : user=%s", uAccountName.c_str());
		_clientcb.forceClose(_from);
		return false;
	}

	MsgNameType	FilterResPacket = M_SYS_EMPTY;
	int		FilterMaxNum = -1;
	int		FilterIntervalTime = DEFAULT_SERIAL_INTERVAL;

	map<MsgNameType, PACKETFILTERDATA>::iterator itFilterData = mapPACKETFILTERDATA.find(_sPacket);
	if (itFilterData != mapPACKETFILTERDATA.end())
	{
		FilterResPacket		= itFilterData->second.sOutputPacket;
		FilterMaxNum		= itFilterData->second.nMaxInputNum;
		FilterIntervalTime	= itFilterData->second.nSerialIntervalTime;
	}

	RECEIVEDPACKET	*ReceiveHis = &(itAll->second);
	map<MsgNameType, ONEPACKETHISTORY> *Packets = &(itAll->second.Packets);
	map<MsgNameType, ONEPACKETHISTORY>::iterator itPacket = Packets->find(_sPacket);
	if (itPacket == Packets->end())
	{
		ONEPACKETHISTORY newData;
		newData.tmLastReceived = CTime::getLocalTime();
		newData.uAllReceivedNum = 1;
		newData.sWaitPacket = FilterResPacket;
		Packets->insert(make_pair(_sPacket, newData));

		if (newData.sWaitPacket != M_SYS_EMPTY)
		{
			WAITPACKET	newWaitPacket;
			newWaitPacket.sWaitPacket	= newData.sWaitPacket;
			newWaitPacket.sInputPacket	= _sPacket;
			itAll->second.CurWaitPackets.push_back(newWaitPacket);
		}
	}
	else
	{
		ONEPACKETHISTORY *hisPacket = &(itPacket->second);

		// Filter with max count of input
		if (FilterMaxNum != -1)
		{
			uint32	uAllNum = hisPacket->uAllReceivedNum + 1;
			if (uAllNum > (uint32)FilterMaxNum)
			{
#ifdef SIP_MSG_NAME_DWORD
				ucstring ucUnlawContent = SIPBASE::_toString(L"Over received packet(%d) : user=%s", _sPacket, uAccountName.c_str());
				sipwarning(L"Over received packet(%d) : user=%s", _sPacket, uAccountName.c_str());
#else
				ucstring ucUnlawContent = SIPBASE::_toString(L"Over received packet(%s) : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
				sipwarning(L"Over received packet(%s) : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
#endif // SIP_MSG_NAME_DWORD
				_clientcb.forceClose(_from);
				ProcUnlaw(UNLAW_REMARK, ucUnlawContent, 0);
				return false;
			}
		}

		TTime tmCur = CTime::getLocalTime();

		// Filter with response packet
		if (hisPacket->sWaitPacket != M_SYS_EMPTY)
		{
			TTime	tmWait = tmCur - hisPacket->tmLastReceived;
			if (tmWait > DEFAULT_MAX_RESPONSETIME)
			{
#ifdef SIP_MSG_NAME_DWORD
				// sipwarning(L"Timeout waiting packet(%d) : user=%s", _sPacket, uAccountName.c_str());
#else
				// sipwarning(L"Timeout waiting packet(%s) : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
#endif // SIP_MSG_NAME_DWORD
				itAll->second.DeleteWait(hisPacket->sWaitPacket, _sPacket);
			}
			else
			{
#ifdef SIP_MSG_NAME_DWORD
				//sipwarning(L"Re-received packet(%d) on no response : user=%s", _sPacket, uAccountName.c_str());
#else
				//sipwarning(L"Re-received packet(%s) on no response : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
#endif // SIP_MSG_NAME_DWORD
				ReceiveHis->NoPassCount ++;
				if (ReceiveHis->NoPassCount > 5)
				{
#ifdef SIP_MSG_NAME_DWORD
					//ucstring ucUnlawContent = SIPBASE::_toString(L"Re-received packet(%d) on no response : user=%s", _sPacket, uAccountName.c_str());
#else
					//ucstring ucUnlawContent = SIPBASE::_toString(L"Re-received packet(%s) on no response : user=%s", ucstring(_sPacket).c_str(), uAccountName.c_str());
#endif // SIP_MSG_NAME_DWORD
					//_clientcb.forceClose(_from);
					//ProcUnlaw(UNLAW_REMARK, ucUnlawContent, 0);
				}
				return false;
			}
		}

		hisPacket->tmLastReceived = tmCur;
		hisPacket->uAllReceivedNum ++;
		hisPacket->sWaitPacket = FilterResPacket;
		if (hisPacket->sWaitPacket != M_SYS_EMPTY)
		{
			WAITPACKET	newWaitPacket;
			newWaitPacket.sWaitPacket	= hisPacket->sWaitPacket;
			newWaitPacket.sInputPacket	= _sPacket;
			itAll->second.CurWaitPackets.push_back(newWaitPacket);
		}
	}

	return true;
}

// Process response packet
void	ProcResponse(MsgNameType _sPacket, uint32 _uAccountID)
{
	map<uint32,	RECEIVEDPACKET>::iterator itAll = mapReceivedPackets.find(_uAccountID);
	if (itAll == mapReceivedPackets.end())
		return;

	ucstring	uAccountName	= GetUserName(_uAccountID);
	if (LogPacketFilterFlag)
	{
#ifdef SIP_MSG_NAME_DWORD
		// sipdebug(L"FILTER Send packet=%d, to=%s, time=%d", _sPacket, uAccountName.c_str(), CTime::getLocalTime());
		PACKETFILTERLOG(L"To=%s\t\tpacket=%d", uAccountName.c_str(), _sPacket);
#else
		// sipdebug(L"FILTER Send packet=%s, to=%s, time=%d", ucstring(_sPacket).c_str(), uAccountName.c_str(), CTime::getLocalTime());
		PACKETFILTERLOG(L"To=%s\t\tpacket=%s", uAccountName.c_str(), ucstring(_sPacket).c_str());
#endif // SIP_MSG_NAME_DWORD
	}

	if (!bPacketFiltering)
		return;

	list<WAITPACKET>::iterator itWait;
	for(itWait = itAll->second.CurWaitPackets.begin(); itWait != itAll->second.CurWaitPackets.end(); itWait++)
	{
		if (_sPacket == itWait->sWaitPacket)
		{
			map<MsgNameType, ONEPACKETHISTORY> *Packets = &(itAll->second.Packets);
			MsgNameType	sInputPacket = itWait->sInputPacket;

			map<MsgNameType, ONEPACKETHISTORY>::iterator itPacket = Packets->find(sInputPacket);
			if (itPacket != Packets->end())
			{
				ONEPACKETHISTORY *hisPacket = &(itPacket->second);
				hisPacket->sWaitPacket = M_SYS_EMPTY;
			}
		}
	}

	itAll->second.DeleteWait(_sPacket, M_SYS_EMPTY);

}

void ProcUnlawSerialCallback(MsgNameType _sMesID, TTcpSockId _sock)
{
	uint32		uAccountID		= GetIdentifiedID(_sock);
	if (uAccountID == 0)
		return;

	ucstring	UserName = GetUserName(uAccountID);
	T_FAMILYID	FID = 0;
	uint8		UnlawType = UNLAW_REMARK;
#ifdef SIP_MSG_NAME_DWORD
	ucstring ucUnlawContent = SIPBASE::_toString(L"Unlaw serial packet=%d, User=%s", _sMesID, UserName.c_str());
#else
	ucstring ucUnlawContent = SIPBASE::_toString(L"Unlaw serial packet=%s, User=%s", ucstring(_sMesID).c_str(), UserName.c_str());
#endif // SIP_MSG_NAME_DWORD

	ProcUnlaw(UnlawType, ucUnlawContent, FID);
}

void	cbNegPacketFilters(CConfigFile::CVar &var)
{
	uint32	count = var.size();
	for (uint i = 0; i < count; i++)
	{
		PacketFilterLog.addNegativeFilter (var.asString(i).c_str());
	}
}

void	InitFilter()
{
	bPacketFiltering = true;
	CConfigFile &configFile = IService::getInstance()->ConfigFile;
	if (configFile.exists("AcceptInvalidCookie"))
		bPacketFiltering = !(configFile.getVar("AcceptInvalidCookie").asBool());
sipdebug("Packet Filter = %d", bPacketFiltering);

	PacketFilterFile.setParam("PacketFilterLog.log", false);
	PacketFilterLog.addDisplayer(&PacketFilterFile);
	
	CConfigFile::CVar *	varNeg = configFile.getVarPtr ("NegPacketFilters");
	if ( varNeg != NULL )
	{
		configFile.setCallback ("NegPacketFilters", cbNegPacketFilters);
		cbNegPacketFilters(*varNeg);
	}

	int	nDataNum = sizeof(PacketFilterData) / sizeof(PacketFilterData[0]);
	for (int i = 0; i < nDataNum; i++)
	{
		MsgNameType	sPacket = PacketFilterData[i].sInputPacket;
		mapPACKETFILTERDATA.insert(make_pair(sPacket, PacketFilterData[i]));
	}

	//mapPacketAlwaysPass.insert(make_pair(M_CS_FNAMECHECK, 1));
	mapPacketAlwaysPass.insert(make_pair(M_GMCS_NOTICE, 1));

	mapPacketOnlyNoFamilyPass.insert(make_pair(M_CS_FAMILY, 1));
	mapPacketOnlyNoFamilyPass.insert(make_pair(M_CS_NEWFAMILY, 1));

	Clients->setGetMsgCheckNeedCallback( GetMsgCheckNeed );
	Clients->setProcUnlawSerialCallback( ProcUnlawSerialCallback );
}

SIPBASE_CATEGORISED_COMMAND(FES, setPacketAttackFilter, "set/unset packet filtering", "")
{
	if(args.size() < 1)	return false;

	string strSet = args[0];
	uint32 uSet = atoui(strSet.c_str());

	if (uSet == 0)
	{
		bPacketFiltering = false;
		log.displayNL ("Set packet filtering to false");
	}
	else
	{
		bPacketFiltering = true;
		log.displayNL ("Set packet filtering to true");
	}

	return true;
}

