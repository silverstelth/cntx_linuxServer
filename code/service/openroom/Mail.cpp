#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>

#include "Mail.h"
#include "Inven.h"

#include "misc/db_interface.h"
#include "misc/DBCaller.h"
#include "../Common/QuerySetForShard.h"

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"
#include "../Common/DBLog.h"
#include "../Common/netCommon.h"
#include "mst_room.h"
#include "openroom.h"
#include "Family.h"
#include "contact.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

//extern DBCONNECTION	DB;
extern CDBCaller		*DBCaller;

//// 사용자에게 Mailbox의 상태를 통지한다.
//void Send_M_SC_MAILBOX_STATUS(T_FAMILYID FID, uint8 bExistNewMail, uint8 bSendMailboxOverflow, uint8 bReceiveMailboxOverflow)
//{
//	CMessage msg(M_SC_MAILBOX_STATUS);
//	msg.serial(FID, bExistNewMail, bSendMailboxOverflow, bReceiveMailboxOverflow);
//	CUnifiedNetwork::getInstance ()->send(FRONTEND_SHORT_NAME, msg);
//	sipdebug(L"Send M_SC_MAILBOX_STATUS to FID=%d(%d,%d,%d)", FID, bExistNewMail, bSendMailboxOverflow, bReceiveMailboxOverflow);
//}

//int GetMailboxStatus(T_FAMILYID _FID, uint16 &_NewCount, uint16 &_SendCount, uint16 &_ReceiveCount)
//{
//	_NewCount = 0;
//	_SendCount = 0;
//	_ReceiveCount = 0;
//
//	MAKEQUERY_GetMailboxStatus(strQ, _FID);
//	CDBConnectionRest	DB(DBCaller);
//	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
//	if (!nPrepareRet)
//	{
//		sipwarning("Shrd_Mail_list_Receive_New: prepare statement failed!");
//		return 0;
//	}			
//
//	int len1 = 0;		
//	int len2 = 0;		
//	int len3 = 0;		
//	int len4 = 0;		
//	uint16	NewCount = 0;
//	uint16	SendCount = 0;
//	uint16	ReceiveCount = 0;
//	uint32	ret = 0;
//
//	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &NewCount, 0, &len1);
//	DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &SendCount, 0, &len2);
//	DB_EXE_BINDPARAM_OUT(Stmt, 3, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &ReceiveCount, 0, &len3);
//	DB_EXE_BINDPARAM_OUT(Stmt, 4, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len4);
//
//	if (!nBindResult)
//	{
//		sipwarning("Shrd_Mail_list_Receive_New: bind failed!");
//		return 0;
//	}
//
//	DB_EXE_PARAMQUERY(Stmt, strQ);
//	if (!nQueryResult || (ret != 0))
//	{
//		sipwarning("Shrd_Mail_list_Receive_New: execute failed!");
//		return 0;
//	}
//	_NewCount = NewCount;
//	_SendCount = SendCount;
//	_ReceiveCount = ReceiveCount;
//	return 1;
//}

// MailBox상태를 검사한다.
void SendCheckMailboxResult(T_FAMILYID FID, SIPNET::TServiceId _tsid)
{
	uint16	fsSid = _tsid.get();
	CMessage	msgOut(M_NT_REQ_MAILBOX_STATUS_CHECK);
	msgOut.serial(fsSid, FID);
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
}

//// [b:MailType][d:page]
//// 메일목록 얻기
//struct _MailInfo
//{
//	uint32		mail_id;
//	uint32		user_id;
//	ucstring	user_name;
//	uint8		mail_status;
//	uint8		mail_kind;
//	ucstring	mail_title;
//	string		mail_datetime;
//	uint8		mail_itemexist;
//	_MailInfo(uint32 _mail_id, uint32 _user_id, ucstring _user_name, uint8 _mail_status,
//		uint8 _mail_kind, ucstring _mail_title, string _mail_datetime, uint8 _mail_itemexist)
//	{
//		mail_id = _mail_id;
//		user_id = _user_id;
//		user_name = _user_name;
//		mail_status = _mail_status;
//		mail_kind = _mail_kind;
//		mail_title = _mail_title;
//		mail_datetime = _mail_datetime;
//		mail_itemexist = _mail_itemexist;
//	}
//};

//static void	DBCB_DBGetRecvMailList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	sint32			AllCount			= *(sint32 *)argv[0];
//	sint32			NewCount			= *(sint32 *)argv[1];
//	sint32			ret					= *(sint32 *)argv[2];
//	T_FAMILYID		FID					= *(T_FAMILYID *)argv[3];
//	uint8			MailType			= *(uint8 *)argv[4];
//	uint32			Page				= *(uint32 *)argv[5];
//	uint16			wsid				 = *(uint16 *)argv[6];
//	TServiceId		tsid(wsid);
//
//	if (!nQueryResult)
//		return;	// E_DBERR
//
//	std::vector<_MailInfo>	aryMailInfo;
//
//	if(ret == 0)	// if ret = 1010, no data
//	{
//		uint32	nRows;			resSet->serial(nRows);
//		for (uint32 i = 0; i < nRows; i++)
//		{
//			T_COMMON_ID			mail_id;			resSet->serial(mail_id);
//			T_COMMON_ID			user_id;			resSet->serial(user_id);
//			T_NAME				user_name;			resSet->serial(user_name);
//			uint8				mail_status;		resSet->serial(mail_status);
//			uint8				mail_kind;			resSet->serial(mail_kind);
//			T_COMMON_CONTENT	mail_title;			resSet->serial(mail_title);
//			T_DATETIME			mail_datetime;		resSet->serial(mail_datetime);
//			uint8				mail_itemexist;		resSet->serial(mail_itemexist);
//
//			_MailInfo	data(mail_id, user_id, user_name, mail_status, mail_kind, mail_title, mail_datetime, mail_itemexist);
//			aryMailInfo.push_back(data);
//		}
//	}
//	else
//	{
//		AllCount = NewCount = 0;
//	}
//
//	CMessage	msgout(M_SC_MAIL_LIST);
//	msgout.serial(FID, MailType, AllCount, NewCount, Page);
//	int	count = aryMailInfo.size();
//	for(int i = 0; i < count; i ++)
//	{
//		msgout.serial(aryMailInfo[i].mail_id, aryMailInfo[i].user_id, aryMailInfo[i].user_name, aryMailInfo[i].mail_kind,
//			aryMailInfo[i].mail_status, aryMailInfo[i].mail_title, aryMailInfo[i].mail_datetime, aryMailInfo[i].mail_itemexist);
//	}
//
//	CUnifiedNetwork::getInstance()->send(tsid, msgout);
//
//	sipinfo("cb_M_CS_MAIL_GET_LIST SUCCESS");
//}

//static void	DBCB_DBGetSendMailList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	sint32			AllCount			= *(sint32 *)argv[0];
//	sint32			ret					= *(sint32 *)argv[1];
//	T_FAMILYID		FID					= *(T_FAMILYID *)argv[2];
//	uint8			MailType			= *(uint8 *)argv[3];
//	uint32			Page				= *(uint32 *)argv[4];
//	uint16			wsid				= *(uint16 *)argv[5];
//	TServiceId		tsid(wsid);
//	sint32			NewCount = 0;
//	if (!nQueryResult)
//		return;	// E_DBERR
//	std::vector<_MailInfo>	aryMailInfo;
//
//	if(ret == 0)	// if ret = 1010, no data
//	{
//		uint32	nRows;			resSet->serial(nRows);
//		for (uint32 i = 0; i < nRows; i++)
//		{
//			T_COMMON_ID			mail_id;			resSet->serial(mail_id);
//			T_COMMON_ID			user_id;			resSet->serial(user_id);
//			T_NAME				user_name;			resSet->serial(user_name);
//			uint8				mail_status;		resSet->serial(mail_status);
//			uint8				mail_kind;			resSet->serial(mail_kind);
//			T_COMMON_CONTENT	mail_title;			resSet->serial(mail_title);
//			T_DATETIME			mail_datetime;		resSet->serial(mail_datetime);
//			uint8				mail_itemexist;		resSet->serial(mail_itemexist);
//
//			_MailInfo	data(mail_id, user_id, user_name, mail_status, mail_kind, mail_title, mail_datetime, mail_itemexist);
//			aryMailInfo.push_back(data);
//		}
//	}
//	else
//	{
//		AllCount = NewCount = 0;
//	}
//
//	CMessage	msgout(M_SC_MAIL_LIST);
//	msgout.serial(FID, MailType, AllCount, NewCount, Page);
//	int	count = aryMailInfo.size();
//	for(int i = 0; i < count; i ++)
//	{
//		msgout.serial(aryMailInfo[i].mail_id, aryMailInfo[i].user_id, aryMailInfo[i].user_name, aryMailInfo[i].mail_kind,
//			aryMailInfo[i].mail_status, aryMailInfo[i].mail_title, aryMailInfo[i].mail_datetime, aryMailInfo[i].mail_itemexist);
//	}
//
//	CUnifiedNetwork::getInstance()->send(tsid, msgout);
//
//	sipinfo("cb_M_CS_MAIL_GET_LIST SUCCESS");
//}

//void	cb_M_CS_MAIL_GET_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	uint8				MailType;			_msgin.serial(MailType);
//	uint32				Page;				_msgin.serial(Page);
//	uint8				PageSize;			_msgin.serial(PageSize);
//
//	if (MailType == MT_SENT_MAIL)
//	{
//		MAKEQUERY_GetSendMailList(strQ, FID, Page, PageSize);
//		DBCaller->executeWithParam(strQ, DBCB_DBGetSendMailList, 
//			"DDDBDW",
//			OUT_,	NULL,
//			OUT_,	NULL,
//			CB_,	FID,
//			CB_,	MailType,
//			CB_,	Page,
//			CB_,	_tsid.get());
//	}
//	else // MT_RECEIVED_MAIL
//	{
//		MAKEQUERY_GetReceiveMailList(strQ, FID, Page, PageSize);
//		DBCaller->executeWithParam(strQ, DBCB_DBGetRecvMailList, 
//			"DDDDBDW",
//			OUT_,	NULL,
//			OUT_,	NULL,
//			OUT_,	NULL,
//			CB_,	FID,
//			CB_,	MailType,
//			CB_,	Page,
//			CB_,	_tsid.get());
//	}
//}

// 새 메일을 작성하여 보낸다
// Send New Mail ([d:ClientMailID][d:toFID][u:Title][u:Content][b:count][ [w:itemcount][w:Inven pos] ]
void	cb_M_CS_MAIL_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
}

// Send MaiboxCheckResult LS->WS->OROOM ([b:toFID_NoExist][b:fromFID_SendMailboxOverflow][b:toFID_ReceiveMailboxOverflow][d:FID][params of M_CS_MAIL_SEND])
void	cb_M_NT_MAILBOX_STATUS_FOR_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint8	toFID_NoExist;					_msgin.serial(toFID_NoExist);
	uint8	fromFID_SendMailboxOverflow;	_msgin.serial(fromFID_SendMailboxOverflow);
	uint8	toFID_ReceiveMailboxOverflow;	_msgin.serial(toFID_ReceiveMailboxOverflow);
	T_FAMILYID			FID;				_msgin.serial(FID);
	uint32				ClientMailID;		_msgin.serial(ClientMailID);
	uint32				toFID;				_msgin.serial(toFID);
	ucstring			Title;				SAFE_SERIAL_WSTR(_msgin, Title, MAX_LEN_MAIL_TITLE, FID);
	ucstring			Content;			NULLSAFE_SERIAL_WSTR(_msgin, Content, MAX_LEN_MAIL_CONTENT, FID);
	uint8				count;				_msgin.serial(count);

	if (count > MAX_MAIL_ITEMCOUNT)
	{
		sipwarning("count > MAX_MAIL_ITEMCOUNT. FID=%d, count=%d", FID, count);
		return;
	}

	if (FID == toFID)
	{
		// 비법파케트발견
		ucstring ucUnlawContent = SIPBASE::_toString("Mail FromFID=toFID! FID=%d", FID).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	FAMILY *pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipinfo("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	// GM이나 일반사용자나 꼭같이 하는것으로 토의하고 아래 검사부분을 없앤다.
	//bool	bSendItem = (count == 0) ? false : true;
	//if (bSendItem && IsSystemAccount(pFamily->m_UserID))
	//{
	//	// SystemUser는 일반사용자에게 아이템메일을 보낼수 없다.
	//	T_ACCOUNTID		toUserID = NOVALUE;
	//	MAKEQUERY_LOADFAMILYFORBASE(strQ, toFID);
	//	CDBConnectionRest	DB(DBCaller);
	//	DB_EXE_QUERY(DB, Stmt, strQ);
	//	if (nQueryResult == TRUE)
	//	{
	//		DB_QUERY_RESULT_FETCH(Stmt, row);
	//		if (IS_DB_ROW_VALID(row))
	//		{
	//			COLUMN_DIGIT(row, 9, T_ACCOUNTID, userid);
	//			toUserID = userid;
	//		}
	//	}
	//	if (toUserID == NOVALUE)
	//		return;
	//	if (!IsSystemAccount(toUserID))
	//	{
	//		sint32	ret = E_CANNOTTSENDITEMTOGENERAL;
	//		CMessage	msgout(M_SC_MAIL_SEND);
	//		msgout.serial(FID, ClientMailID, ret);
	//		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
	//		return;
	//	}
	//}

	//// 상대가 나를 흑명단에 등록한 경우 메일을 보낼수 없다. - 후에 정말로 이 사양이 필요하다면 다시 추가한다. BlackList검사는 MS에서만 할수 있다.
	//if (IsBlackList(toFID, FID))
	//{
	//	sint32	ret = E_INBLACKLIST;
	//	CMessage	msgout(M_SC_MAIL_SEND);
	//	msgout.serial(FID, ClientMailID, ret);
	//	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
	//	return;
	//}

	_MailItems	MailItems;
	uint16		InvenPos[MAX_MAIL_ITEMCOUNT];
	uint8		i;
	for (i = 0; i < count; i ++)
	{
		_msgin.serial(MailItems.Items[i].ItemCount);
		_msgin.serial(InvenPos[i]);
		if (MailItems.Items[i].ItemCount == 0)
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("NewMail ItemCount Error! UserFID=%d, InvenPos=%d, Count=%d", FID, InvenPos[i], MailItems.Items[i].ItemCount).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
	}

	// Mailbox에 있는 메일수 검사
	if (toFID_NoExist)
	{
		// toFID가 존재하지 않는다.
		CMessage	msgout(M_SC_MAIL_SEND);
		msgout.serial(FID, ClientMailID);
		uint32	ret = E_FAMILY_NOEXIST;
		msgout.serial(ret);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgout);

		return;
	}
	if (fromFID_SendMailboxOverflow)
	{
		// 비법파케트발견 - Client에서 이미 검사되여야 한다.
		ucstring ucUnlawContent = SIPBASE::_toString("SendMail Count Overflow! UserFID=%d", FID).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}
	if (toFID_ReceiveMailboxOverflow)
	{
		// 받을사람의 메일함 초과
		sint32	ret = E_RMAILBOX_OVERFLOW;
		CMessage	msgout(M_SC_MAIL_SEND);
		msgout.serial(FID, ClientMailID, ret);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgout);
		return;
	}

	// 인벤관리
	_InvenItems		*pOldInven;
	int		InvenIndex;
	if (count > 0)
	{
		// 인벤 얻기
		pOldInven = GetFamilyInven(FID);
		if (pOldInven == NULL)
		{
			sipwarning("GetFamilyInven(Old) failed ! FID=%d", FID);
			return;
		}

		// 인벤에서 검사
		for (i = 0; i < count; i ++)
		{
			if (!IsValidInvenPos(InvenPos[i]))
			{
				// 비법파케트발견
				ucstring ucUnlawContent = SIPBASE::_toString("InvenPos Error! UserFID=%d, InvenPos=%d, Count=%d", FID, InvenPos[i], MailItems.Items[i].ItemCount).c_str();
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}

			InvenIndex = GetInvenIndexFromInvenPos(InvenPos[i]);
			if ((pOldInven->Items[InvenIndex].ItemID == 0) 
				|| (pOldInven->Items[InvenIndex].GetDate > 0)  // 새 아이템이 아니면 보낼수 없다.
				|| (pOldInven->Items[InvenIndex].MoneyType != MONEYTYPE_UMONEY && !(GetPrizeItemLevel(pOldInven->Items[InvenIndex].ItemID) == PRIZE_LEVEL_1) && !(GetPrizeItemLevel(pOldInven->Items[InvenIndex].ItemID) == PRIZE_LEVEL_ANIMAL)) 
				|| (pOldInven->Items[InvenIndex].ItemCount < MailItems.Items[i].ItemCount) 
				|| (pOldInven->Items[InvenIndex].UsedCharID > 0))
			{
				// 비법파케트발견
				ucstring ucUnlawContent = SIPBASE::_toString("Invalid ItemCount! UserFID=%d, InvenPos=%d, ItemID=%d, GetDate=%d, MoneyType=%d, ItemCount=%d:%d, UsedCharID=%d", 
					FID, InvenPos[i], pOldInven->Items[InvenIndex].ItemID, (DWORD)(pOldInven->Items[InvenIndex].GetDate), pOldInven->Items[InvenIndex].MoneyType, 
					pOldInven->Items[InvenIndex].ItemCount, MailItems.Items[i].ItemCount, pOldInven->Items[InvenIndex].UsedCharID).c_str();
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}

			MailItems.Items[i].ItemID = pOldInven->Items[InvenIndex].ItemID;
			MailItems.Items[i].GetDate = pOldInven->Items[InvenIndex].GetDate;
			MailItems.Items[i].UsedCharID = 0;
			MailItems.Items[i].MoneyType = pOldInven->Items[InvenIndex].MoneyType;
		}
	}

	// 메일목록에 추가
	CMessage	msgMail(M_NT_MAIL_SEND);
	msgMail.serial(FID, ClientMailID, toFID, Title, Content, count);
	if (count > 0)
	{
		// 원래 인벤에서 삭제
		for (i = 0; i < count; i ++)
		{
			InvenIndex = GetInvenIndexFromInvenPos(InvenPos[i]);
			if (pOldInven->Items[InvenIndex].ItemCount > MailItems.Items[i].ItemCount)
			{
				pOldInven->Items[InvenIndex].ItemCount -= MailItems.Items[i].ItemCount;
			}
			else
			{
				pOldInven->Items[InvenIndex].ItemID = 0;
			}

			msgMail.serial(MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount, MailItems.Items[i].MoneyType);
		}

		if (!SaveFamilyInven(FID, pOldInven))
		{
			sipwarning("SaveFamilyInven failed. FID=%d", FID);
			return;
		}
	}
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgMail);
}

// Response for M_CS_MAIL_SEND ([d:FID][d:ClientMailID][d:result][d:MailId][d:toFID][u:toFamilyName][s:DateTime] + [d:toUserID][b:count][[d:itemid][w:itemcount]])
void	cb_M_SC_MAIL_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);
	uint32				ClientMailID;		_msgin.serial(ClientMailID);
	uint32				result;				_msgin.serial(result);
	uint32				MailID;				_msgin.serial(MailID);
	T_FAMILYID			toFID;				_msgin.serial(toFID);
	ucstring			toFamilyName;		_msgin.serial(toFamilyName);
	string				mailTime;			_msgin.serial(mailTime);
	uint32				toUID;				_msgin.serial(toUID);
	uint8				count;				_msgin.serial(count);

	uint8		i;
	for (i = 0; i < count; i ++)
	{
		uint32	ItemID;
		uint16	ItemCount;
		_msgin.serial(ItemID, ItemCount);

		// Log
		ucstring name = L"";
		const MST_ITEM *mstItem = GetMstItemInfo(ItemID);					
		if (mstItem != NULL)
		{
			name = mstItem->Name;
		}

		DBLOG_SendMailItem(FID, MailID, ItemID, ItemCount, name, toFID, toUID, toFamilyName);					
	}

	FAMILY *pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipinfo("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	CMessage	msgOut(M_SC_MAIL_SEND);
	msgOut.serial(FID, ClientMailID, result, MailID, toFID, toFamilyName, mailTime);

	CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
}

//// Send MaiboxCheckResult LS->WS->OROOM ([b:toFID_NoExist][b:fromFID_SendMailboxOverflow][b:toFID_ReceiveMailboxOverflow][d:FID][params of M_CS_MAIL_SEND])
//void	cb_M_NT_MAILBOX_STATUS_FOR_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	uint8	toFID_NoExist;					_msgin.serial(toFID_NoExist);
//	uint8	fromFID_SendMailboxOverflow;	_msgin.serial(fromFID_SendMailboxOverflow);
//	uint8	toFID_ReceiveMailboxOverflow;	_msgin.serial(toFID_ReceiveMailboxOverflow);
//	T_FAMILYID			FID;				_msgin.serial(FID);
//	uint32				ClientMailID;		_msgin.serial(ClientMailID);
//	uint32				toFID;				_msgin.serial(toFID);
//	ucstring			Title;				SAFE_SERIAL_WSTR(_msgin, Title, MAX_LEN_MAIL_TITLE, FID);
//	ucstring			Content;			NULLSAFE_SERIAL_WSTR(_msgin, Content, MAX_LEN_MAIL_CONTENT, FID);
//	uint8				count;				_msgin.serial(count);
//
//	if (count > MAX_MAIL_ITEMCOUNT)
//	{
//		sipwarning("count > MAX_MAIL_ITEMCOUNT. FID=%d, count=%d", FID, count);
//		return;
//	}
//
//	if (FID == toFID)
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("Mail FromFID=toFID! FID=%d", FID).c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//
//	FAMILY *pFamily = GetFamilyData(FID);
//	if (pFamily == NULL)
//	{
//		sipinfo("GetFamilyData = NULL. FID=%d", FID);
//		return;
//	}
//
//	// GM이나 일반사용자나 꼭같이 하는것으로 토의하고 아래 검사부분을 없앤다.
//	//bool	bSendItem = (count == 0) ? false : true;
//	//if (bSendItem && IsSystemAccount(pFamily->m_UserID))
//	//{
//	//	// SystemUser는 일반사용자에게 아이템메일을 보낼수 없다.
//	//	T_ACCOUNTID		toUserID = NOVALUE;
//	//	MAKEQUERY_LOADFAMILYFORBASE(strQ, toFID);
//	//	CDBConnectionRest	DB(DBCaller);
//	//	DB_EXE_QUERY(DB, Stmt, strQ);
//	//	if (nQueryResult == TRUE)
//	//	{
//	//		DB_QUERY_RESULT_FETCH(Stmt, row);
//	//		if (IS_DB_ROW_VALID(row))
//	//		{
//	//			COLUMN_DIGIT(row, 9, T_ACCOUNTID, userid);
//	//			toUserID = userid;
//	//		}
//	//	}
//	//	if (toUserID == NOVALUE)
//	//		return;
//	//	if (!IsSystemAccount(toUserID))
//	//	{
//	//		sint32	ret = E_CANNOTTSENDITEMTOGENERAL;
//	//		CMessage	msgout(M_SC_MAIL_SEND);
//	//		msgout.serial(FID, ClientMailID, ret);
//	//		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//	//		return;
//	//	}
//	//}
//
//	//// 상대가 나를 흑명단에 등록한 경우 메일을 보낼수 없다. - 후에 정말로 이 사양이 필요하다면 다시 추가한다. BlackList검사는 MS에서만 할수 있다.
//	//if (IsBlackList(toFID, FID))
//	//{
//	//	sint32	ret = E_INBLACKLIST;
//	//	CMessage	msgout(M_SC_MAIL_SEND);
//	//	msgout.serial(FID, ClientMailID, ret);
//	//	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//	//	return;
//	//}
//
//	_MailItems	MailItems;
//	uint16		InvenPos[MAX_MAIL_ITEMCOUNT];
//	uint8		i;
//	for (i = 0; i < count; i ++)
//	{
//		_msgin.serial(MailItems.Items[i].ItemCount);
//		_msgin.serial(InvenPos[i]);
//		if (MailItems.Items[i].ItemCount == 0)
//		{
//			// 비법파케트발견
//			ucstring ucUnlawContent = SIPBASE::_toString("NewMail ItemCount Error! UserFID=%d, InvenPos=%d, Count=%d", FID, InvenPos[i], MailItems.Items[i].ItemCount).c_str();
//			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//			return;
//		}
//	}
//
//	// Mailbox에 있는 메일수 검사
//	if (toFID_NoExist)
//	{
//		// toFID가 존재하지 않는다.
//		CMessage	msgout(M_SC_MAIL_SEND);
//		msgout.serial(FID, ClientMailID);
//		uint32	ret = E_FAMILY_NOEXIST;
//		msgout.serial(ret);
//		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//
//		return;
//	}
//	if (fromFID_SendMailboxOverflow)
//	{
//		// 비법파케트발견 - Client에서 이미 검사되여야 한다.
//		ucstring ucUnlawContent = SIPBASE::_toString("SendMail Count Overflow! UserFID=%d", FID).c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//	if (toFID_ReceiveMailboxOverflow)
//	{
//		// 받을사람의 메일함 초과
//		sint32	ret = E_RMAILBOX_OVERFLOW;
//		CMessage	msgout(M_SC_MAIL_SEND);
//		msgout.serial(FID, ClientMailID, ret);
//		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//
//		// 받는사람에게 Mailbox가 다 차서 메일을 받을수 없다는것을 통지한다.
//		Send_M_SC_MAILBOX_STATUS(toFID, 0, 0, 1);
//		return;
//	}
//
//	// 인벤관리
//	_InvenItems		*pOldInven;
//	int		InvenIndex;
//	if (count > 0)
//	{
//		// 인벤 얻기
//		pOldInven = GetFamilyInven(FID);
//		if (pOldInven == NULL)
//		{
//			sipwarning("GetFamilyInven(Old) failed !");
//			return;
//		}
//
//		// 인벤에서 검사
//		for (i = 0; i < count; i ++)
//		{
//			if (!IsValidInvenPos(InvenPos[i]))
//			{
//				// 비법파케트발견
//				ucstring ucUnlawContent = SIPBASE::_toString("InvenPos Error! UserFID=%d, InvenPos=%d, Count=%d", FID, InvenPos[i], MailItems.Items[i].ItemCount).c_str();
//				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//				return;
//			}
//
//			InvenIndex = GetInvenIndexFromInvenPos(InvenPos[i]);
//			if ((pOldInven->Items[InvenIndex].ItemID == 0) 
//				|| (pOldInven->Items[InvenIndex].GetDate > 0)  // 새 아이템이 아니면 보낼수 없다.
//				|| (pOldInven->Items[InvenIndex].MoneyType != MONEYTYPE_UMONEY && !(GetPrizeItemLevel(pOldInven->Items[InvenIndex].ItemID) == PRIZE_LEVEL_1) && !(GetPrizeItemLevel(pOldInven->Items[InvenIndex].ItemID) == PRIZE_LEVEL_ANIMAL)) 
//				|| (pOldInven->Items[InvenIndex].ItemCount < MailItems.Items[i].ItemCount) 
//				|| (pOldInven->Items[InvenIndex].UsedCharID > 0))
//			{
//				// 비법파케트발견
//				ucstring ucUnlawContent = SIPBASE::_toString("Invalid ItemCount! UserFID=%d, InvenPos=%d, ItemID=%d, GetDate=%d, MoneyType=%d, ItemCount=%d:%d, UsedCharID=%d", 
//					FID, InvenPos[i], pOldInven->Items[InvenIndex].ItemID, (DWORD)(pOldInven->Items[InvenIndex].GetDate), pOldInven->Items[InvenIndex].MoneyType, 
//					pOldInven->Items[InvenIndex].ItemCount, MailItems.Items[i].ItemCount, pOldInven->Items[InvenIndex].UsedCharID).c_str();
//				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//				return;
//			}
//
//			MailItems.Items[i].ItemID = pOldInven->Items[InvenIndex].ItemID;
//			MailItems.Items[i].GetDate = pOldInven->Items[InvenIndex].GetDate;
//			MailItems.Items[i].UsedCharID = 0;
//			MailItems.Items[i].MoneyType = pOldInven->Items[InvenIndex].MoneyType;
//		}
//	}
//
//	// 메일목록에 추가
//	{
//		uint8	MailItemBuf[MAX_MAILITEMBUF_SIZE];
//		uint32	MailItemBufSize = sizeof(MailItemBuf);
//
//		if (!MailItems.toBuffer(MailItemBuf, &MailItemBufSize))
//		{
//			sipwarning("Can't toBuffer mail items!");
//			return;
//		}
//
//		uint8	mail_kind = MK_NORMAL;
//		uint8	item_exist = (count > 0) ? 1 : 0;
//		MAKEQUERY_InsertMail(strQ, FID, toFID, Title, Content, mail_kind, item_exist);
//		CDBConnectionRest	DB(DBCaller);
//		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
//		if (!nPrepareRet)
//		{
//			sipwarning("Shrd_Mail_Insert: prepare statement failed!");
//			return;	// E_DBERR
//		}
//
//		int	len1 = MailItemBufSize, len2 = 0;
//		sint32	ret = 0;
//
//		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_INVENBUF_SIZE, 0, MailItemBuf, 0, &len1);
//		DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len2);
//		if (!nBindResult)
//		{
//			sipwarning("Shrd_Mail_Insert: bind failed!");
//			return; // E_DBERR
//		}
//
//		DB_EXE_PARAMQUERY(Stmt, strQ);
//		if (!nQueryResult)
//		{
//			sipwarning("Shrd_Mail_Insert: execute failed!");
//			return;	// E_DBERR
//		}
//
//		CMessage	msgout(M_SC_MAIL_SEND);
//		msgout.serial(FID, ClientMailID);
//		if (ret != 0)
//		{
//			ret = E_FAMILY_NOEXIST;
//
//			msgout.serial(ret);
//		}
//		else
//		{
//			DB_QUERY_RESULT_FETCH(Stmt, row);
//			if (!IS_DB_ROW_VALID(row))
//			{
//				sipwarning("Shrd_Mail_Insert: return empty!");
//				return;	// E_DBERR
//			}
//
//			COLUMN_DIGIT(row, 0, uint32, mail_id);
//			COLUMN_DATETIME(row, 1, mail_datetime);
//			COLUMN_WSTR(row, 2, to_name, MAX_LEN_FAMILY_NAME);
//			COLUMN_DIGIT(row, 3, uint32, userId);
//
//			ucstring ItemInfoForLog = L"";
//
//			if (count > 0)
//			{
//				// 원래 인벤에서 삭제
//				for (i = 0; i < count; i ++)
//				{
//					InvenIndex = GetInvenIndexFromInvenPos(InvenPos[i]);
//					if (pOldInven->Items[InvenIndex].ItemCount > MailItems.Items[i].ItemCount)
//					{
//						pOldInven->Items[InvenIndex].ItemCount -= MailItems.Items[i].ItemCount;
//					}
//					else
//					{
//						pOldInven->Items[InvenIndex].ItemID = 0;
//					}
//
//					// Log
//					ucstring name = L"";
//					const MST_ITEM *mstItem = GetMstItemInfo(MailItems.Items[i].ItemID);					
//					if (mstItem != NULL)
//					{
//						name = mstItem->Name;
//					}
//
//					DBLOG_SendMailItem(FID, mail_id, MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount, name, toFID, userId, to_name);					
//				}
//
//				if (!SaveFamilyInven(FID, pOldInven))
//				{
//					sipwarning("SaveFamilyInven failed. FID=%d", FID);
//					return;
//				}
//			}
//			msgout.serial(ret, mail_id, toFID, to_name, mail_datetime);
//		}
//		CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//	}
//
//	// 대방에게 새 메일이 왔음을 통지한다.
//	Send_M_SC_MAILBOX_STATUS(toFID, 1, 0, 0);
//}

//static void	DBCB_DBGetMail(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	sint32			ret					= *(sint32 *)argv[0];
//	T_FAMILYID		FID					= *(T_FAMILYID *)argv[1];
//	T_COMMON_ID		MailID				= *(T_COMMON_ID *)argv[2];
//	TServiceId		tsid(*(uint16 *)argv[3]);
//
//	if (!nQueryResult || ret != 0)
//		return;	// E_DBERR
//
//	uint32	nRows;			resSet->serial(nRows);
//	if (nRows < 1)
//	{
//		sipwarning("Shrd_Mail_list: return empty!");
//		return;
//	}
//
//	uint8	mail_status;
//	uint8	mail_itemexist;
//	uint32	to_fid;
//	ucstring	mail_content;
//
//	uint8	MailItemBuf[MAX_MAILITEMBUF_SIZE];
//	LONG	lMailItemBufSize = sizeof(MailItemBuf);
//
//	int	row_index = 0;
//	uint32			_from_fid;					resSet->serial(_from_fid);
//	T_FAMILYNAME	from_name;					resSet->serial(from_name);
//	uint32			_to_fid;					resSet->serial(_to_fid);
//	T_FAMILYNAME	to_name;					resSet->serial(to_name);
//	uint8			_mail_status;				resSet->serial(_mail_status);
//	uint8			_mail_kind;					resSet->serial(_mail_kind);
//	T_COMMON_CONTENT _mail_title;				resSet->serial(_mail_title);
//	T_COMMON_CONTENT _mail_content;				resSet->serial(_mail_content);
//	T_DATETIME		_mail_datetime;				resSet->serial(_mail_datetime);
//	uint8			_mail_itemexist;			resSet->serial(_mail_itemexist);
//	uint32			BufSize;			resSet->serial(BufSize);	lMailItemBufSize = BufSize; resSet->serialBuffer(MailItemBuf, BufSize);
//	uint8			_deleteflag1;				resSet->serial(_deleteflag1);
//	uint8			_deleteflag2;				resSet->serial(_deleteflag2);
//
//	to_fid = _to_fid;
//	mail_status = _mail_status;
//	mail_itemexist = _mail_itemexist;
//	mail_content = _mail_content;
//
//	if((FID != to_fid) && (FID != _from_fid))
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("User require other's mail! UserFID=%d, MailID=%d", FID, MailID).c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//	if(((FID == _from_fid) && (_deleteflag1 != 0)) || ((FID == to_fid) && (_deleteflag2 != 0)))
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("User require deleted mail! UserFID=%d, MailID=%d", FID, MailID).c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//
//	uint8		item_count = 0;
//	_MailItems	MailItems;
//	if(mail_itemexist != 0 && !MailItems.fromBuffer(MailItemBuf, (uint32)lMailItemBufSize))
//	{
//		sipwarning("Can't fromBuffer mail items! MailID==%d", MailID);
//		return;
//	}
//
//	if(mail_itemexist != 0)
//	{
//		for(int i = 0; i < MAX_MAIL_ITEMCOUNT; i ++)
//		{
//			if(MailItems.Items[i].ItemID > 0)
//				item_count ++;
//		}
//	}
//
//	// 받은메일이고, 새 메일이면 읽음표시를 한다.
//	if((FID == to_fid) && (mail_status == MS_NEW_MAIL))
//	{
//		mail_status = MS_OPENED_MAIL;	// 읽음표시
//		if(mail_itemexist == 0)	// 첨부아이템이 없으면
//			mail_status = MS_RECEIVED_ITEM;	// 완성표시
//
//		MAKEQUERY_SetMailStatus(strQ, MailID, mail_status);
//		DBCaller->execute(strQ);
//	}
//
//	// Packet보내기
//	CMessage	msgout(M_SC_MAIL);
//	msgout.serial(FID, MailID, mail_status, mail_content, item_count);
//	for(int i = 0; i < MAX_MAIL_ITEMCOUNT; i ++)
//	{
//		if(MailItems.Items[i].ItemID > 0)
//			msgout.serial(MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount, MailItems.Items[i].MoneyType);
//	}
//
//	CUnifiedNetwork::getInstance()->send(tsid, msgout);
//
//	sipinfo("cb_M_CS_MAIL_GET SUCCESS");
//}

//// [d:MailId]
//// 하나의 메일을 읽는다
//void	cb_M_CS_MAIL_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	uint32				MailID;				_msgin.serial(MailID);
//
//	MAKEQUERY_GetMail(strQ, MailID);
//	DBCaller->executeWithParam(strQ, DBCB_DBGetMail, 
//		"DDDW",
//		OUT_,	NULL,
//		CB_,	FID,
//		CB_,	MailID,
//		CB_,	_tsid.get());
//}

//// [b:count][ [d:MailID][b:MailType] ]
//// 몇개의 메일을 삭제한다
//void	cb_M_CS_MAIL_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	uint8				count;				_msgin.serial(count);
//	uint32		MailId;
//	uint8		MailType;
//
////	CMessage	msgout(M_SC_MAIL_DELETE);
////	msgout.serial(FID);
//	for(uint8 i = 0; i < count; i ++)
//	{
//		_msgin.serial(MailId);
//		_msgin.serial(MailType);
//
//		MAKEQUERY_DeleteMail(strQ, MailId, FID, MailType);
//		DBCaller->executeWithParam(strQ, NULL, 
//			"D",
//			OUT_,	NULL);
////		msgout.serial(MailId, MailType);
//	}
//
////	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//
//	sipinfo("cb_M_CS_MAIL_DELETE SUCCESS");
//}

//// [d:MailId]
//// 메일을 거절한다.
//void	cb_M_CS_MAIL_REJECT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
//	uint32				MailID;				_msgin.serial(MailID);
//
//	uint32	fromFID, toFID;
//	uint8	mail_status = 0, mail_kind = 0, deleteflag1 = 0, deleteflag2 = 0;
//	sint32	ret = 0;
//	// 메일 상태 얻기
//	{
//		MAKEQUERY_GetMail(strQ, MailID);
//		CDBConnectionRest	DB(DBCaller);
//		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
//		if ( ! nPrepareRet )
//		{
//			sipwarning("Shrd_Mail_list: prepare statement failed!");
//			return;	// E_DBERR
//		}
//
//		int len1 = 0;
//		sint32 ret = 0;
//
//		DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len1);
//		if ( ! nBindResult )
//		{
//			sipwarning("Shrd_Mail_list: bind failed!");
//			return; // E_DBERR
//		}
//
//		DB_EXE_PARAMQUERY(Stmt, strQ);
//		if ( !nQueryResult || ret != 0)
//		{
//			sipwarning("Shrd_Mail_list: execute failed!");
//			return;	// E_DBERR
//		}
//
//		DB_QUERY_RESULT_FETCH(Stmt, row);
//		if (!IS_DB_ROW_VALID(row))
//		{
//			sipwarning("Shrd_Mail_list: return empty!");
//			return;	// E_DBERR
//		}
//
//		COLUMN_DIGIT(row, 0, uint32, from_fid);
//		COLUMN_DIGIT(row, 2, uint32, _to_fid);
//		COLUMN_DIGIT(row, 4, uint8, _mail_status);
//		COLUMN_DIGIT(row, 5, uint8, _mail_kind);
//		COLUMN_DIGIT(row, 11, uint8, _deleteflag1);
//		COLUMN_DIGIT(row, 12, uint8, _deleteflag2);
//
//		fromFID = from_fid;
//		toFID = _to_fid;
//		mail_status = _mail_status;
//		mail_kind = _mail_kind;
//		deleteflag1 = _deleteflag1;
//		deleteflag2 = _deleteflag2;
//	}
//
//	// 거절가능성 검사 : 보통메일이고 읽음상태에 있는 메일만 거절할수 있다. toFID = FID 이면 System Mail이다.
//	if((toFID != FID) || (mail_kind != MK_NORMAL) || (mail_status == MS_NEW_MAIL) || (deleteflag2 != 0))
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("User refuse impossible mail! UserFID=%d, MailID=%d", FID, MailID).c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//
//	// 대방의 Mailbox에 있는 메일수 검사
//	uint16	NewCount = 0;
//	uint16	SendCount = 0;
//	uint16	ReceiveCount = 0;
//	if(GetMailboxStatus(fromFID, NewCount, SendCount, ReceiveCount))
//	{
//		if(ReceiveCount >= MAX_MAIL_NUM)
//		{
//			uint32	ret = E_RMAILBOX_OVERFLOW;
//			CMessage	msgout(M_SC_MAIL_REJECT);
//			msgout.serial(FID, ret, MailID);
//			CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//
//			// 받는사람에게 Mailbox가 다 차서 메일을 받을수 없다는것을 통지한다.
//			Send_M_SC_MAILBOX_STATUS(fromFID, 0, 0, 1);
//			return;
//		}
//	}
//	else
//	{
//		sipwarning("SendMail DB Error in GetMailboxStatus!");
//		return;
//	}
//
//	// 메일 거절 처리
//	MAKEQUERY_RejectMail(strQ, MailID, FID);
//	CDBConnectionRest	DB(DBCaller);
//	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
//	if ( ! nPrepareRet )
//	{
//		sipwarning("Shrd_Mail_Return: prepare statement failed!");
//		return;	// E_DBERR
//	}
//
//	int len1 = 0;
//
//	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len1);
//	if ( ! nBindResult )
//	{
//		sipwarning("Shrd_Mail_Return: bind failed!");
//		return; // E_DBERR
//	}
//
//	DB_EXE_PARAMQUERY(Stmt, strQ);
//	if ( !nQueryResult )
//	{
//		sipwarning("Shrd_Mail_Return: execute failed!");
//		return;	// E_DBERR
//	}
//
//	if (ret != 0)
//	{
//		// 비법파케트발견
//		ucstring ucUnlawContent = SIPBASE::_toString("User refused other's mail! UserFID=%d, MailID=%d", FID, MailID).c_str();
//		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
//		return;
//	}
//
//	DB_QUERY_RESULT_FETCH(Stmt, row);
//	if (!IS_DB_ROW_VALID(row))
//	{
//		sipwarning("Shrd_Mail_Return: return empty!");
//		return;	// E_DBERR
//	}
//
//	COLUMN_DATETIME(row, 1, mail_datetime);
//
//	// 클라이언트에 거절처리완성 통지
//	ret = 0;
//	CMessage	msgout(M_SC_MAIL_REJECT);
//	msgout.serial(FID, ret, MailID, mail_datetime);
//
//	CUnifiedNetwork::getInstance()->send(_tsid, msgout);
//
//	// 대방에게 새 메일이 왔음을 통지한다.
//	Send_M_SC_MAILBOX_STATUS(fromFID, 1, 0, 0);
//
//	sipinfo("cb_M_CS_MAIL_REJECT SUCCESS");
//}

// [d:MailId][b:count][ [w:InvenPos] ]
// 메일에서 아이템을 수집한다.
void	cb_M_CS_MAIL_ACCEPT_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	_msgin.invert();
	CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, _msgin);
}

void	cb_M_NT_MAIL_ITEM_FOR_ACCEPT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	uint32				SyncCode;			_msgin.serial(SyncCode);
	uint32				MailID;				_msgin.serial(MailID);
	uint8				count;				_msgin.serial(count);

	FAMILY *pFamily = GetFamilyData(FID);
	if (pFamily == NULL)
	{
		sipinfo("GetFamilyData = NULL. FID=%d", FID);
		return;
	}

	uint16		InvenPos[MAX_MAIL_ITEMCOUNT];
	memset(InvenPos, 0, sizeof(InvenPos));
	uint8		i;
	for(i = 0; i < count; i ++)
	{
		_msgin.serial(InvenPos[i]);
		if (InvenPos[i] == 0)
			continue;
		if (!IsValidInvenPos(InvenPos[i]))
		{
			// 비법파케트발견
			ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos in M_CS_MAIL_ACCEPT_ITEM! UserFID=%d, MailID=%d, InvenPos=%d", FID, MailID, InvenPos[i]).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		// InvenPos 겹치는거 검사
		for (uint8 j = 0; j < i; j ++)
		{
			if (InvenPos[i] == InvenPos[j])
			{
				// 비법파케트발견
				ucstring ucUnlawContent = SIPBASE::_toString("Invalid InvenPos in M_CS_MAIL_ACCEPT_ITEM! UserFID=%d, MailID=%d, InvenPos=%d", FID, MailID, InvenPos[i]).c_str();
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
		}
	}

	T_FAMILYID fromFid = 0;
	T_ACCOUNTID fromUId = 0;
	uint8	mail_kind = 0;
	ucstring fromName = L"";
	_msgin.serial(fromFid, fromUId, mail_kind, fromName);

	_MailItems		MailItems;
	_InvenItems		*pInven;
	int		InvenIndex;

	// 인벤검사
	pInven = GetFamilyInven(FID);
	if (pInven == NULL)
	{
		sipinfo("GetFamilyInven failed. FID=%d");
		return;
	}
	int		ItemCount = 0;
	for(i = 0; i < MAX_MAIL_ITEMCOUNT; i ++)
	{
		_msgin.serial(MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount, MailItems.Items[i].MoneyType);
		MailItems.Items[i].GetDate = 0;
		MailItems.Items[i].UsedCharID = 0;

		if(MailItems.Items[i].ItemID == 0)
			continue;

		if (InvenPos[i] == 0)
		{
			// 비법파케트발견 - 빈자리 부족
			ucstring ucUnlawContent = SIPBASE::_toString("Need InvenPos! UserFID=%d, MailID=%d", FID, MailID).c_str();
			RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
			return;
		}
		InvenIndex = GetInvenIndexFromInvenPos(InvenPos[i]);
		if (pInven->Items[InvenIndex].ItemID > 0)
		{
			bool	bOK = false;
			//// 같은 아이템이고, 더 추가할수 있어야 한다. - 련속 2번 올라올 때 동기화처리때문에 빈자리에만 넣는것으로 한다.
			//if ((pInven->Items[InvenIndex].ItemID == MailItems.Items[i].ItemID) && (pInven->Items[InvenIndex].MoneyType == MailItems.Items[i].MoneyType))
			//{
			//	const MST_ITEM	*mstItem = GetMstItemInfo(MailItems.Items[i].ItemID);
			//	if(mstItem)
			//	{
			//		if(mstItem->OverlapFlag)
			//		{
			//			if(mstItem->OverlapMaxNum >= pInven->Items[InvenIndex].ItemCount + MailItems.Items[i].ItemCount)
			//				bOK = true;
			//		}
			//	}
			//}
			if (!bOK)
			{
				// 비법파케트발견 - 빈자리가 아니다!!
				ucstring ucUnlawContent = SIPBASE::_toString("Not empty InvenPos! UserFID=%d, MailID=%d, InvenPos=%d", FID, MailID, InvenPos[i]).c_str();
				RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
				return;
			}
		}
		ItemCount ++;
	}

	if (ItemCount <= 0)
	{
		// 비법파케트발견 - 아이템이 없다!!
		ucstring ucUnlawContent = SIPBASE::_toString("No Item in Mail! UserFID=%d, MailID=%d", FID, MailID).c_str();
		RECORD_UNLAW_EVENT(UNLAW_REMARK, ucUnlawContent, FID);
		return;
	}

	// 아이템수집 처리
	for (i = 0; i < MAX_MAIL_ITEMCOUNT; i ++)
	{
		if (MailItems.Items[i].ItemID == 0)
			continue;

		InvenIndex = GetInvenIndexFromInvenPos(InvenPos[i]);
		if (pInven->Items[InvenIndex].ItemID == MailItems.Items[i].ItemID)
			pInven->Items[InvenIndex].ItemCount += MailItems.Items[i].ItemCount;
		else
		{
			pInven->Items[InvenIndex] = MailItems.Items[i];
			pInven->Items[InvenIndex].MoneyType = MailItems.Items[i].MoneyType;
		}

		// Log
		ucstring name = L"";		
		const MST_ITEM *mstItem = GetMstItemInfo(MailItems.Items[i].ItemID);					
		if (mstItem != NULL)
		{
			name = mstItem->Name;
		}

		DBLOG_GetMailItem(FID, MailID, MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount, name, fromFid, fromUId, mail_kind, fromName );
	}

	if (!SaveFamilyInven(FID, pInven))
	{
		sipwarning("SaveFamilyInven failed. FID=%d", FID);
		return;
	}

	// 메일 처리 완료표시를 한다.
	{
		CMessage	msgOut(M_NT_MAIL_ACCEPT_ITEM);
		msgOut.serial(MailID);
		CUnifiedNetwork::getInstance()->send(WELCOME_SHORT_NAME, msgOut);
	}

	// 처리끝
	{
		uint32	ret = ERR_NOERR;
		CMessage	msgOut(M_SC_INVENSYNC);//M_SC_MAIL_ACCEPT_ITEM);
		msgOut.serial(FID, SyncCode, ret);
		CUnifiedNetwork::getInstance()->send(pFamily->m_FES, msgOut);
	}
}

//void SendTestItemMail(T_FAMILYID GMID, T_FAMILYID toFID, ucstring Title, ucstring Content, uint8 count, _MailItems &MailItems)
//{
//	// 메일목록에 추가
//	{
//		uint8	MailItemBuf[MAX_MAILITEMBUF_SIZE];
//		uint32	MailItemBufSize = sizeof(MailItemBuf);
//
//		if(!MailItems.toBuffer(MailItemBuf, &MailItemBufSize))
//		{
//			sipwarning("Can't toBuffer mail items!");
//			return;
//		}
//
//		uint8	item_exist = (count > 0) ? 1 : 0;
//		MAKEQUERY_InsertMail_GM(strQ, toFID, Title, Content, item_exist);
//		CDBConnectionRest	DB(DBCaller);
//		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
//		if ( ! nPrepareRet )
//		{
//			sipwarning("Shrd_Mail_Insert_GM: prepare statement failed!");
//			return;	// E_DBERR
//		}
//
//		int	len1 = MailItemBufSize, len2 = 0;
//		sint32	ret = 0;
//
//		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_INVENBUF_SIZE, 0, MailItemBuf, 0, &len1);
//		DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len2);
//		if ( ! nBindResult )
//		{
//			sipwarning("Shrd_Mail_Insert_GM: bind failed!");
//			return; // E_DBERR
//		}
//
//		DB_EXE_PARAMQUERY(Stmt, strQ);
//		if ( !nQueryResult )
//		{
//			sipwarning("Shrd_Mail_Insert_GM: execute failed!");
//			return;	// E_DBERR
//		}
//	}
//
//	// 대방에게 새 메일이 왔음을 통지한다.
//	Send_M_SC_MAILBOX_STATUS(toFID, 1, 0, 0);
//
//	sipinfo("SendTestItemMail SUCCESS");
//}

//SIPBASE_CATEGORISED_COMMAND(OROOM_S, sendItemMail, "send item to family on mailing", "FID ItemID ItemCount")
//{
//	if(args.size() < 3)	return false;
//
//	string v0	= args[0];
//	uint32 fid = atoui(v0.c_str());
//	string v1	= args[1];
//	uint32 itemid = atoui(v1.c_str());
//	string v2	= args[2];
//	uint32 itemcount = atoui(v2.c_str());
//
//
//	_MailItems	MailItems;
//	MailItems.Items[0].ItemID = itemid;
//	MailItems.Items[0].ItemCount = (uint16)itemcount;
//
//	SendTestItemMail(1, fid, L"Item", L"Send item for testing", (uint8)itemcount, MailItems);
//	log.displayNL ("send item(%d) to family(%d).", itemid, fid);
//	return true;
//}

//static void	DBCB_DBFamilyInfoForGMGetGMoney(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
//{
//	T_FAMILYID				GMID		= *(T_FAMILYID *)argv[0];
//	T_ACCOUNTID				userID		= *(T_ACCOUNTID *)argv[1];
//	TServiceId				tsid(*(uint16 *)argv[2]);
//
//	if (!nQueryResult)
//	{
//		sipwarning("nQueryResult is null!");
//		return;
//	}
//
//	uint32		nRows;		resSet->serial(nRows);
//	if (nRows < 1)
//	{
//		sipwarning("nRows = %d", nRows);
//		return;
//	}
//
//	CMessage mes(M_GMSC_GET_GMONEY);
//	mes.serial(GMID, userID);
//
//	bool bIs = true;
//	uint32	uResult;		resSet->serial(uResult);
//	if (uResult == 1001)
//	{
//		bIs = false;
//		mes.serial(bIs);
//	}
//	else
//	{
//		T_PRICE			GMoney;			resSet->serial(GMoney);
//		T_F_VISITDAYS	vdays;			resSet->serial(vdays);
//		T_F_LEVEL		uLevel;			resSet->serial(uLevel);
//		bIs = true;
//		mes.serial(bIs, GMoney);
//	}
//	CUnifiedNetwork::getInstance()->send(tsid, mes);
//}

void	cb_M_GMCS_GET_GMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	sipwarning("This is not supported!!!");
	return;
	//T_FAMILYID		GMID;		_msgin.serial(GMID);
	//T_ACCOUNTID		userID;		_msgin.serial(userID);

	//// get fid with the user ID
	//MAKEQUERY_FAMILYINFOFROMNAME_USERID(strQuery, userID);
	//DBCaller->execute(strQuery, DBCB_DBFamilyInfoForGMGetGMoney,
	//	"DDW",
	//	CB_,		GMID,
	//	CB_,		userID,
	//	CB_,		_tsid.get());
}
