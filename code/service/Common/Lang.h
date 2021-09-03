#pragma once

#include "misc/types_sip.h"
#include "misc/i_xml.h"
#include "ExcelXML.h"

// Lang
struct OUT_LANG
{
	uint32		ID;
	ucstring	Text;
};
typedef		std::map<uint32,	OUT_LANG>					MAP_OUT_LANG;
extern		MAP_OUT_LANG			map_OUT_LANG;
extern	bool			LoadLangData(ucstring ExePath);
extern	ucstring		GetLangText(uint32 _id);

extern int GetColumnPos(const ucchar * section, const ucchar * key, ucstring ucOutIniFile, xmlNodePtr pSheet);
extern xmlNodePtr GetSheet(const SIPBASE::CIXml *xmlInfo, const ucchar * section, ucstring ucOutIniFile);

// Langage ID
enum
{
	ID_FRIEND_DEFAULTGROUP_NAME = 1,
	ID_ROOM_DEFAULTGROUP_NAME = 2,
	ID_PHOTO_DEFAULTGROUP_NAME = 3,
	ID_VIDEO_PLAYGROUP_NAME = 4,
	ID_VIDEO_DEFAULTGROUP_NAME = 5,
	ID_PRIZE_MAIL_TITLE = 6,
	ID_PRIZE_MAIL_CONTENT = 7,
	ID_TEST_ADDPOINT_MAIL_TITLE = 8,
	ID_TEST_ADDPOINT_MAIL_CONTENT = 9,
};

//====================================================================
// ManagerService에서 Xianbao시에 효과모델자료
//====================================================================
//extern bool LoadXianbaoEffectModelName(ucstring ExePath);
//extern std::string GetModelEffectName(uint32 ItemID);

//====================================================================
// ManagerService에서 축복카드ID자료
//====================================================================
extern bool LoadBlessCardID(ucstring ExePath);
extern uint8 GetBlessCardID(uint32 LuckID);

//====================================================================
// ManagerService에서 예약행사시의 TaocanItem정보
//====================================================================
struct OUT_TAOCAN_FOR_AUTOPLAY3
{
	uint32		TaocanType;
	uint32		ItemIDs[9];
	ucstring	PrayText;
};
bool LoadTaocanForAutoplay3(ucstring ExePath);
OUT_TAOCAN_FOR_AUTOPLAY3*	GetAutoplay3TaocanItemInfo(uint32 ItemID);
