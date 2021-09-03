#ifndef CHARACTER_INVEN_H
#define CHARACTER_INVEN_H

#include "../../common/Packet.h"
#include "../../common/common.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"
#include "Family.h"

// 가족Item자료
struct FAMILY_ITEM 
{
	T_ITEMSUBTYPEID			m_SubTypeId;		// Item작은 분류 Id
	uint8					m_bDelete;			// 1 if delete, 0 if add or modify
	T_FITEMNUM				m_Num;				// Item개수
	uint64					m_GetDate;			// 구입날자
	T_FITEMPOS				m_Pos;				// 위치
	T_CHARACTERID			m_CHID;				// 사용캐릭터id
	uint8					m_MoneyType;		// 
};

_InvenItems* GetFamilyInven(T_FAMILYID FID); // , bool bLoginUserOnly = true, bool *pIsLogin = NULL);
int GetInvenIndexFromInvenPos(T_FITEMPOS InvenPos);
uint16 GetInvenPosFromInvenIndex(int InvenIndex);
bool	IsValidInvenPos(T_FITEMPOS _pos);

// callback
extern void	cb_M_CS_FPROPERTY_MONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_INVENSYNC(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_FITEMPOS(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_USEITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_BUYITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_THROWITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_TIMEOUT_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void cb_M_CS_TRIMITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern void	cb_M_CS_BANKITEM_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_CS_BANKITEM_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

//extern void	cb_M_SC_BANKITEM_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
//extern void	cb_M_MS_BANKITEM_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern	void	ItemRefresh(T_FAMILYID _FID, SIPNET::TServiceId _tsid); // , T_ACCOUNTID uId, ucstring UserName, T_FAMILYNAME	ucFName);

void CharacterDressChanged(T_FAMILYID FID, T_ID NewDressID, T_CHARACTERID CHID=0);

#define SERIALINVEN(msg, inven)	\
	msg.serial(inven->m_Pos, inven->m_bDelete, inven->m_SubTypeId, inven->m_Num, inven->m_GetDate, inven->m_CHID, inven->m_MoneyType);

extern void	cb_M_NT_CURRENT_ACTIVITY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_REQ_ACTIVITY_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_MS_SET_USER_ACTIVITY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_NT_BEGINNERMSTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_CS_RECV_GIFTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void	cb_M_MS_SET_USER_BEGINNERMSTITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

struct	BUYIVENTEMPINFO
{
	T_FITEMPOS		InvenPos;
	T_ITEMSUBTYPEID	ItemID;
	T_FITEMNUM		ItemNum;
};

void CheckRefreshMoney(T_FAMILYID FID);

#endif // CHARACTER_INVEN_H

