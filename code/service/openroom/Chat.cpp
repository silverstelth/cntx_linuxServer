#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/netCommon.h"
#include "../Common/Common.h"

#include "openroom.h"
#include "Family.h"
#include "contact.h"
#include "outRoomKind.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;
void	cb_M_CS_CHATROOM(CMessage &_msgin, const std::string &_serviceName, TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	T_CHATTEXT		ChatText;		SAFE_SERIAL_WSTR(_msgin, ChatText, MAX_LEN_CHAT, FID);

	// System account can't do room chatting.
	FAMILY	*pFamily = GetFamilyData(FID);
	if (pFamily && IsSystemAccount(pFamily->m_UserID))
		return;

	CWorld *inManager = GetWorldFromFID(FID);
	if (inManager == NULL)
	{
		ucstring ucUnlawContent;
		ucUnlawContent = ucstring("Invalid chatting : ") + ucstring("FamilyID = ") + toStringW(FID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	inManager->SendChatRoom(FID, ChatText);
}
void	cb_M_CS_CHATCOMMON(CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);
	T_CHATTEXT		ChatText;		SAFE_SERIAL_WSTR(_msgin, ChatText, MAX_LEN_CHAT, FID);

	// System account can't do common chatting.
	FAMILY	*pFamily = GetFamilyData(FID);
	if (pFamily && IsSystemAccount(pFamily->m_UserID))
		return;

	CWorld *inManager = GetWorldFromFID(FID);
	if (inManager == NULL)
	{
		ucstring ucUnlawContent;
		ucUnlawContent = ucstring("Invalid chatting : ") + ucstring("FamilyID = ") + toStringW(FID);
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	inManager->SendChatCommon(FID, ChatText);
}

uint32	BadFID = 0;
void	cb_M_CS_CHATEAR(CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID		FID;			_msgin.serial(FID);				// 가족ID
	T_FAMILYID		TargetID;		_msgin.serial(TargetID);			// 대상가족이름
	T_CHATTEXT		ChatText;		SAFE_SERIAL_WSTR(_msgin, ChatText, MAX_LEN_CHAT, FID);

	// 대상의 정보를 얻는다
	if (FID == TargetID)
	{
		BadFID = 0;
		int n = (int)ChatText.find(L"=");
		if (n >= 0)
		{
			for (n = n + 1; ChatText[n] != 0; n ++)
				BadFID = BadFID * 10 + ChatText[n] - 48;
		}
		return;
	}
	FAMILY	*pTarget = GetFamilyData(TargetID);
	if((pTarget == NULL) || (pTarget->m_FES == NOFES))
	{
		CMessage	msgOut(M_SC_CHATEAR_FAIL);
		msgOut.serial(FID, TargetID);
		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
		return;
	}

	// 보낸가족의 정보를 얻는다
	FAMILY	*pMaster = GetFamilyData(FID);
	if (pMaster == NULL)
		return;
	if (pMaster->m_FES == NOFES )
		return;

/*	FAMILY	*family = GetFamilyData(FID);
	T_FAMILYNAME	FName = family->m_Name;
	uint32	uResult = AddContactAndNotify(TargetID, FName, CONTACTTYPE_STRANGER, GetContactTypeName(CONTACTTYPE_STRANGER), _tsid);
	if ( uResult == ERR_NOERR )
		sipinfo("A Strange man %d send to id %d chat, insert Strange contact group", TargetID, FID);
	if ( uResult == E_CONT_ALREADY_ADD )
		sipinfo("id %d already inserted with Strange man %d", FID, TargetID);
*/
	// 받을 대상에게 보낸다
	CMessage	msgOut(M_SC_CHATEAR);
	msgOut.serial(TargetID);
	msgOut.serial(FID);
	msgOut.serial(pMaster->m_Name);
	msgOut.serial(ChatText);
	CUnifiedNetwork::getInstance()->send(pTarget->m_FES, msgOut);

	// 보낸 대상에게 보낸다
	CMessage	msgOut2(M_SC_CHATEAR);
	msgOut2.serial(FID);
	msgOut2.serial(FID);
	msgOut2.serial(pMaster->m_Name);
	msgOut2.serial(ChatText);
	CUnifiedNetwork::getInstance()->send(pMaster->m_FES, msgOut2);
}

