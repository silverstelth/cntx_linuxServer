#include <misc/types_sip.h>

#include <misc/ucstring.h>
#include <net/service.h>

#include "Mails.h"

#include "misc/db_interface.h"
#include "misc/DBCaller.h"

#include "../../common/Packet.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"
#include "../Common/netCommon.h"
#include "../Common/DBLog.h"
#include "../Common/Lang.h"
#include "../Common/QuerySetForMain.h"

using namespace SIPBASE;
using namespace SIPNET;
using namespace std;

extern CDBCaller		*MainDB;

extern bool SendPacketToFID(T_FAMILYID FID, CMessage msgOut);
extern uint32 FindUIDfromFID(uint32 fid);
extern void SendChitList(T_FAMILYID FID);

// 사용자에게 Mailbox의 상태를 통지한다.
void Send_M_SC_MAILBOX_STATUS(T_FAMILYID FID, uint8 bExistNewMail, uint8 bSendMailboxOverflow, uint8 bReceiveMailboxOverflow)
{
	CMessage msgOut(M_SC_MAILBOX_STATUS);
	msgOut.serial(FID, bExistNewMail, bSendMailboxOverflow, bReceiveMailboxOverflow);

	SendPacketToFID(FID, msgOut);
}

int GetMailboxStatus(T_FAMILYID _FID, uint16 &_NewCount, uint16 &_SendCount, uint16 &_ReceiveCount)
{
	_NewCount = 0;
	_SendCount = 0;
	_ReceiveCount = 0;

	MAKEQUERY_GetMailboxStatus(strQ, _FID);
	CDBConnectionRest	DB(MainDB);
	DB_EXE_PREPARESTMT(DB, Stmt, strQ);
	if (!nPrepareRet)
	{
		sipwarning("Main_Mail_GetMailBoxStatus: prepare statement failed!");
		return 0;
	}

	SQLLEN len1 = 0;
	SQLLEN len2 = 0;
	SQLLEN len3 = 0;
	int len4 = 0;
	uint16	NewCount = 0;
	uint16	SendCount = 0;
	uint16	ReceiveCount = 0;

	DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &NewCount, 0, &len1);
	DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &SendCount, 0, &len2);
	DB_EXE_BINDPARAM_OUT(Stmt, 3, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &ReceiveCount, 0, &len3);

	if (!nBindResult)
	{
		sipwarning("Main_Mail_GetMailBoxStatus: bind failed!");
		return 0;
	}

	DB_EXE_PARAMQUERY(Stmt, strQ);
	if (!nQueryResult)
	{
		sipwarning("Main_Mail_GetMailBoxStatus: execute failed!");
		return 0;
	}

	_NewCount = NewCount;
	_SendCount = SendCount;
	_ReceiveCount = ReceiveCount;

	return 1;
}

// MailBox상태를 검사한다.
void cb_M_NT_REQ_MAILBOX_STATUS_CHECK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID	FID;
	uint16		fsSid;
	_msgin.serial(fsSid, FID);

	uint32	UID = FindUIDfromFID(FID);
	if (UID == 0)
	{
		sipwarning("FindUIDfromFID failed. FID=%d", FID);
		return;
	}

	uint16	NewCount = 0;
	uint16	SendCount = 0;
	uint16	ReceiveCount = 0;
	if (GetMailboxStatus(FID, NewCount, SendCount, ReceiveCount))
	{
		uint8	NewMailExist = (NewCount > 0) ? 1 : 0;
		uint8	SendMailboxOverflow = (SendCount >= MAX_MAIL_NUM) ? 1 : 0;
		uint8	ReceiveMailboxOverflow = (ReceiveCount >= MAX_MAIL_NUM) ? 1 : 0;
		if (NewMailExist || SendMailboxOverflow || ReceiveMailboxOverflow)
		{
			CMessage	msgOut(M_SC_MAILBOX_STATUS);
			msgOut.serial(UID, FID, NewMailExist, SendMailboxOverflow, ReceiveMailboxOverflow);
			CUnifiedNetwork::getInstance ()->send(_tsid, msgOut);
		}
	}

	// 여기서 Chit목록검사도 진행하자!!!
	SendChitList(FID);
}

// [b:MailType][d:page]
// 메일목록 얻기
struct _MailInfo
{
	uint32		mail_id;
	uint32		user_id;
	ucstring	user_name;
	uint8		mail_status;
	uint8		mail_kind;
	ucstring	mail_title;
	string		mail_datetime;
	uint8		mail_itemexist;
	_MailInfo(uint32 _mail_id, uint32 _user_id, ucstring _user_name, uint8 _mail_status,
		uint8 _mail_kind, ucstring _mail_title, string _mail_datetime, uint8 _mail_itemexist)
	{
		mail_id = _mail_id;
		user_id = _user_id;
		user_name = _user_name;
		mail_status = _mail_status;
		mail_kind = _mail_kind;
		mail_title = _mail_title;
		mail_datetime = _mail_datetime;
		mail_itemexist = _mail_itemexist;
	}
};

static void	DBCB_DBGetRecvMailList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32			AllCount			= *(sint32 *)argv[0];
	sint32			NewCount			= *(sint32 *)argv[1];
	sint32			ret					= *(sint32 *)argv[2];
	T_FAMILYID		FID					= *(T_FAMILYID *)argv[3];
	uint8			MailType			= *(uint8 *)argv[4];
	uint32			Page				= *(uint32 *)argv[5];
	uint16			fsid				 = *(uint16 *)argv[6];
	uint16			wsid				 = *(uint16 *)argv[7];

	TServiceId		tsid(wsid);

	if (!nQueryResult)
		return;	// E_DBERR

	std::vector<_MailInfo>	aryMailInfo;

	if(ret == 0)	// if ret = 1010, no data
	{
		uint32	nRows;			resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i++)
		{
			T_COMMON_ID			mail_id;			resSet->serial(mail_id);
			T_COMMON_ID			user_id;			resSet->serial(user_id);
			T_NAME				user_name;			resSet->serial(user_name);
			uint8				mail_status;		resSet->serial(mail_status);
			uint8				mail_kind;			resSet->serial(mail_kind);
			T_COMMON_CONTENT	mail_title;			resSet->serial(mail_title);
			T_DATETIME			mail_datetime;		resSet->serial(mail_datetime);
			uint8				mail_itemexist;		resSet->serial(mail_itemexist);

			_MailInfo	data(mail_id, user_id, user_name, mail_status, mail_kind, mail_title, mail_datetime, mail_itemexist);
			aryMailInfo.push_back(data);
		}
	}
	else
	{
		AllCount = NewCount = 0;
	}

	CMessage	msgout(M_SC_MAIL_LIST);
	msgout.serial(fsid, FID, MailType, AllCount, NewCount, Page);
	size_t	count = aryMailInfo.size();
	for(size_t i = 0; i < count; i ++)
	{
		msgout.serial(aryMailInfo[i].mail_id, aryMailInfo[i].user_id, aryMailInfo[i].user_name, aryMailInfo[i].mail_kind,
			aryMailInfo[i].mail_status, aryMailInfo[i].mail_title, aryMailInfo[i].mail_datetime, aryMailInfo[i].mail_itemexist);
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

static void	DBCB_DBGetSendMailList(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32			AllCount			= *(sint32 *)argv[0];
	sint32			ret					= *(sint32 *)argv[1];
	T_FAMILYID		FID					= *(T_FAMILYID *)argv[2];
	uint8			MailType			= *(uint8 *)argv[3];
	uint32			Page				= *(uint32 *)argv[4];
	uint16			fsid				= *(uint16 *)argv[5];
	uint16			wsid				= *(uint16 *)argv[6];

	TServiceId		tsid(wsid);
	sint32			NewCount = 0;
	if (!nQueryResult)
		return;	// E_DBERR
	std::vector<_MailInfo>	aryMailInfo;

	if(ret == 0)	// if ret = 1010, no data
	{
		uint32	nRows;			resSet->serial(nRows);
		for (uint32 i = 0; i < nRows; i++)
		{
			T_COMMON_ID			mail_id;			resSet->serial(mail_id);
			T_COMMON_ID			user_id;			resSet->serial(user_id);
			T_NAME				user_name;			resSet->serial(user_name);
			uint8				mail_status;		resSet->serial(mail_status);
			uint8				mail_kind;			resSet->serial(mail_kind);
			T_COMMON_CONTENT	mail_title;			resSet->serial(mail_title);
			T_DATETIME			mail_datetime;		resSet->serial(mail_datetime);
			uint8				mail_itemexist;		resSet->serial(mail_itemexist);

			_MailInfo	data(mail_id, user_id, user_name, mail_status, mail_kind, mail_title, mail_datetime, mail_itemexist);
			aryMailInfo.push_back(data);
		}
	}
	else
	{
		AllCount = NewCount = 0;
	}

	CMessage	msgout(M_SC_MAIL_LIST);
	msgout.serial(fsid, FID, MailType, AllCount, NewCount, Page);
	size_t	count = aryMailInfo.size();
	for(size_t i = 0; i < count; i ++)
	{
		msgout.serial(aryMailInfo[i].mail_id, aryMailInfo[i].user_id, aryMailInfo[i].user_name, aryMailInfo[i].mail_kind,
			aryMailInfo[i].mail_status, aryMailInfo[i].mail_title, aryMailInfo[i].mail_datetime, aryMailInfo[i].mail_itemexist);
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

void	cb_M_CS_MAIL_GET_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint16				fsSid;				_msgin.serial(fsSid);
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	uint8				MailType;			_msgin.serial(MailType);
	uint32				Page;				_msgin.serial(Page);
	uint8				PageSize;			_msgin.serial(PageSize);

	if (MailType == MT_SENT_MAIL)
	{
		MAKEQUERY_GetSendMailList(strQ, FID, Page, PageSize);
		MainDB->executeWithParam(strQ, DBCB_DBGetSendMailList, 
			"DDDBDWW",
			OUT_,	NULL,
			OUT_,	NULL,
			CB_,	FID,
			CB_,	MailType,
			CB_,	Page,
			CB_,	fsSid,
			CB_,	_tsid.get());
	}
	else // MT_RECEIVED_MAIL
	{
		MAKEQUERY_GetReceiveMailList(strQ, FID, Page, PageSize);
		MainDB->executeWithParam(strQ, DBCB_DBGetRecvMailList, 
			"DDDDBDWW",
			OUT_,	NULL,
			OUT_,	NULL,
			OUT_,	NULL,
			CB_,	FID,
			CB_,	MailType,
			CB_,	Page,
			CB_,	fsSid,
			CB_,	_tsid.get());
	}
}

// 새메일 요청시 보낸사람/받는사람의 메일함의 크기를 검사한다.
void	cb_M_CS_MAIL_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint16		rsSid;
	T_FAMILYID	fromFID, toFID;
	uint32		ClientMailID;

	_msgin.serial(rsSid, fromFID, ClientMailID, toFID);

	// 메일함 크기 얻기
	uint16	fromSendCount = 0;
	uint16	toReceiveCount = 0;
	uint32	ret = 0;
	{
		MAKEQUERY_GetMailboxStatusForSend(strQ, fromFID, toFID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Main_Mail_GetMailBoxStatusForSend: prepare statement failed!");
			return;
		}

		SQLLEN len1 = 0;
		SQLLEN len2 = 0;
		SQLLEN len3 = 0;

		DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &fromSendCount, 0, &len1);
		DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_SHORT, SQL_SMALLINT, 0, 0, &toReceiveCount, 0, &len2);
		DB_EXE_BINDPARAM_OUT(Stmt, 3, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len3);

		if (!nBindResult)
		{
			sipwarning("Main_Mail_GetMailBoxStatusForSend: bind failed!");
			return;
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult)
		{
			sipwarning("Main_Mail_GetMailBoxStatusForSend: execute failed!");
			return;
		}
	}

	// 메일함크기 검사
	uint8	toFid_NoExxist = 0, fromFID_SendMailboxOverflow = 0, toFID_ReceiveMailboxOverflow = 0;
	if (ret != 0)
		toFid_NoExxist = 1;
	else
	{
		fromFID_SendMailboxOverflow = (fromSendCount >= MAX_MAIL_NUM) ? 1 : 0;
		toFID_ReceiveMailboxOverflow = (toReceiveCount >= MAX_MAIL_NUM) ? 1 : 0;

		// 받는사람에게 Mailbox가 다 차서 메일을 받을수 없다는것을 통지한다.
		if (toFID_ReceiveMailboxOverflow)
			Send_M_SC_MAILBOX_STATUS(toFID, 0, 0, 1);
	}

	// 검사결과 보내기
	CMessage	msgOut(M_NT_MAILBOX_STATUS_FOR_SEND);
	msgOut.serial(rsSid, toFid_NoExxist, fromFID_SendMailboxOverflow, toFID_ReceiveMailboxOverflow, fromFID, ClientMailID, toFID);

	ucstring	Title;
	ucstring	Content;
	uint8		count;
	uint16		ItemCount, InvenPos;

	_msgin.serial(Title, Content, count);
	msgOut.serial(Title, Content, count);

	for (uint8 i = 0; i < count; i ++)
	{
		_msgin.serial(ItemCount, InvenPos);
		msgOut.serial(ItemCount, InvenPos);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

// DB에 새 메일을 등록한다.
// SendMail Check success, register mail [d:FID][d:ClientMailID][d:toFID][u:Title][u:Content][b:count][[d:itemid][w:itemcount][b:MoneyType]]
void	cb_M_NT_MAIL_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint16		rsSid;				_msgin.serial(rsSid);
	T_FAMILYID	FID;				_msgin.serial(FID);
	uint32		ClientMailID;		_msgin.serial(ClientMailID);
	uint32		toFID;				_msgin.serial(toFID);
	ucstring	Title;				_msgin.serial(Title);
	ucstring	Content;			_msgin.serial(Content);
	uint8		count;				_msgin.serial(count);

	_MailItems	MailItems;
	uint8		i;
	for (i = 0; i < count; i ++)
	{
		_msgin.serial(MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount, MailItems.Items[i].MoneyType);
		MailItems.Items[i].GetDate = 0;
		MailItems.Items[i].UsedCharID = 0;
	}

	// 메일목록에 추가
	CMessage	msgOut(M_SC_MAIL_SEND);
	msgOut.serial(rsSid, FID, ClientMailID);
	{
		uint8	MailItemBuf[MAX_MAILITEMBUF_SIZE];
		uint32	MailItemBufSize = sizeof(MailItemBuf);

		if (!MailItems.toBuffer(MailItemBuf, &MailItemBufSize))
		{
			sipwarning("Can't toBuffer mail items!");
			return;
		}

		uint8	mail_kind = MK_NORMAL;
		uint8	item_exist = (count > 0) ? 1 : 0;
		MAKEQUERY_InsertMail(strQ, FID, toFID, Title, Content, mail_kind, item_exist);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Main_Mail_Insert: prepare statement failed!");
			return;	// E_DBERR
		}

		SQLLEN	len1 = MailItemBufSize, len2 = 0;
		sint32	ret = 0;

		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_INVENBUF_SIZE, 0, MailItemBuf, 0, &len1);
		DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len2);
		if (!nBindResult)
		{
			sipwarning("Main_Mail_Insert: bind failed!");
			return; // E_DBERR
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult)
		{
			sipwarning("Main_Mail_Insert: execute failed!");
			return;	// E_DBERR
		}

		if (ret != 0)
		{
			sipwarning("Invalid error. FID=%d, ret=%d", FID, ret);
			return;
		}
		else
		{
			DB_QUERY_RESULT_FETCH(Stmt, row);
			if (!IS_DB_ROW_VALID(row))
			{
				sipwarning("Shrd_Mail_Insert: return empty!");
				return;	// E_DBERR
			}

			COLUMN_DIGIT(row, 0, uint32, mail_id);
			COLUMN_DATETIME(row, 1, mail_datetime);
			COLUMN_WSTR(row, 2, to_name, MAX_LEN_FAMILY_NAME);
			COLUMN_DIGIT(row, 3, uint32, userId);
			msgOut.serial(ret, mail_id, toFID, to_name, mail_datetime, userId);
		}
	}
	msgOut.serial(count);
	for (i = 0; i < count; i ++)
	{
		msgOut.serial(MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount);
	}

	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);

	// 대방에게 새 메일이 왔음을 통지한다.
	Send_M_SC_MAILBOX_STATUS(toFID, 1, 0, 0);
}

static void	DBCB_DBGetMail(int nQueryResult, uint32 argn, void *argv[], CMemStream * resSet)
{
	sint32			ret					= *(sint32 *)argv[0];
	T_FAMILYID		FID					= *(T_FAMILYID *)argv[1];
	T_COMMON_ID		MailID				= *(T_COMMON_ID *)argv[2];
	uint16			fsSid				= *(uint16 *)argv[3];
	TServiceId		tsid(*(uint16 *)argv[4]);

	if (!nQueryResult || ret != 0)
		return;	// E_DBERR

	uint32	nRows;			resSet->serial(nRows);
	if (nRows < 1)
	{
		sipwarning("Shrd_Mail_list: return empty!");
		return;
	}

	uint8	mail_status;
	uint8	mail_itemexist;
	uint32	to_fid;
	ucstring	mail_content;

	uint8	MailItemBuf[MAX_MAILITEMBUF_SIZE];
	long	lMailItemBufSize = sizeof(MailItemBuf);

	int	row_index = 0;
	uint32			_from_fid;					resSet->serial(_from_fid);
	T_FAMILYNAME	from_name;					resSet->serial(from_name);
	uint32			_to_fid;					resSet->serial(_to_fid);
	T_FAMILYNAME	to_name;					resSet->serial(to_name);
	uint8			_mail_status;				resSet->serial(_mail_status);
	uint8			_mail_kind;					resSet->serial(_mail_kind);
	T_COMMON_CONTENT _mail_title;				resSet->serial(_mail_title);
	T_COMMON_CONTENT _mail_content;				resSet->serial(_mail_content);
	T_DATETIME		_mail_datetime;				resSet->serial(_mail_datetime);
	uint8			_mail_itemexist;			resSet->serial(_mail_itemexist);
	uint32			BufSize;			resSet->serial(BufSize);	lMailItemBufSize = BufSize; resSet->serialBuffer(MailItemBuf, BufSize);
	uint8			_deleteflag1;				resSet->serial(_deleteflag1);
	uint8			_deleteflag2;				resSet->serial(_deleteflag2);

	to_fid = _to_fid;
	mail_status = _mail_status;
	mail_itemexist = _mail_itemexist;
	mail_content = _mail_content;

	if((FID != to_fid) && (FID != _from_fid))
	{
		// 비법파케트발견
		sipwarning("User require other's mail! UserFID=%d, MailID=%d", FID, MailID);
		return;
	}
	if(((FID == _from_fid) && (_deleteflag1 != 0)) || ((FID == to_fid) && (_deleteflag2 != 0)))
	{
		// 비법파케트발견
		sipwarning("User require deleted mail! UserFID=%d, MailID=%d", FID, MailID);
		return;
	}

	uint8		item_count = 0;
	_MailItems	MailItems;
	if(mail_itemexist != 0 && !MailItems.fromBuffer(MailItemBuf, (uint32)lMailItemBufSize))
	{
		sipwarning("Can't fromBuffer mail items! MailID==%d", MailID);
		return;
	}

	if(mail_itemexist != 0)
	{
		for(int i = 0; i < MAX_MAIL_ITEMCOUNT; i ++)
		{
			if(MailItems.Items[i].ItemID > 0)
				item_count ++;
		}
	}

	// 받은메일이고, 새 메일이면 읽음표시를 한다.
	if((FID == to_fid) && (mail_status == MS_NEW_MAIL))
	{
		mail_status = MS_OPENED_MAIL;	// 읽음표시
		if(mail_itemexist == 0)	// 첨부아이템이 없으면
			mail_status = MS_RECEIVED_ITEM;	// 완성표시

		MAKEQUERY_SetMailStatus(strQ, MailID, mail_status);
		MainDB->execute(strQ);
	}

	// Packet보내기
	CMessage	msgout(M_SC_MAIL);
	msgout.serial(fsSid, FID, MailID, mail_status, mail_content, item_count);
	for(int i = 0; i < MAX_MAIL_ITEMCOUNT; i ++)
	{
		if(MailItems.Items[i].ItemID > 0)
			msgout.serial(MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount, MailItems.Items[i].MoneyType);
	}

	CUnifiedNetwork::getInstance()->send(tsid, msgout);
}

// [d:MailId]
// 하나의 메일을 읽는다
void	cb_M_CS_MAIL_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint16		fsSid;	_msgin.serial(fsSid);
	T_FAMILYID	FID;	_msgin.serial(FID);				// 가족id
	uint32		MailID;	_msgin.serial(MailID);

	MAKEQUERY_GetMail(strQ, MailID);
	MainDB->executeWithParam(strQ, DBCB_DBGetMail, 
		"DDDWW",
		OUT_,	NULL,
		CB_,	FID,
		CB_,	MailID,
		CB_,	fsSid,
		CB_,	_tsid.get());
}

// [b:count][ [d:MailID][b:MailType] ]
// 몇개의 메일을 삭제한다
void	cb_M_CS_MAIL_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	uint8				count;				_msgin.serial(count);
	uint32		MailId;
	uint8		MailType;

	for(uint8 i = 0; i < count; i ++)
	{
		_msgin.serial(MailId);
		_msgin.serial(MailType);

		MAKEQUERY_DeleteMail(strQ, MailId, FID, MailType);
		MainDB->executeWithParam(strQ, NULL, 
			"D",
			OUT_,	NULL);
	}
}

// [d:MailId]
// 메일을 거절한다.
void	cb_M_CS_MAIL_REJECT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint16		fsSid;	_msgin.serial(fsSid);
	T_FAMILYID	FID;	_msgin.serial(FID);				// 가족id
	uint32		MailID;	_msgin.serial(MailID);

	uint32	fromFID, toFID;
	uint8	mail_status = 0, mail_kind = 0, deleteflag1 = 0, deleteflag2 = 0;
	sint32	ret = 0;
	// 메일 상태 얻기
	{
		MAKEQUERY_GetMail(strQ, MailID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Main_Mail_Return: prepare statement failed!");
			return;	// E_DBERR
		}

		SQLLEN len1 = 0;
		sint32 ret = 0;

		DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len1);
		if (!nBindResult)
		{
			sipwarning("Main_Mail_Return: bind failed!");
			return; // E_DBERR
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult || ret != 0)
		{
			sipwarning("Main_Mail_Return: execute failed!");
			return;	// E_DBERR
		}

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (!IS_DB_ROW_VALID(row))
		{
			sipwarning("Main_Mail_Return: return empty!");
			return;	// E_DBERR
		}

		COLUMN_DIGIT(row, 0, uint32, from_fid);
		COLUMN_DIGIT(row, 2, uint32, _to_fid);
		COLUMN_DIGIT(row, 4, uint8, _mail_status);
		COLUMN_DIGIT(row, 5, uint8, _mail_kind);
		COLUMN_DIGIT(row, 11, uint8, _deleteflag1);
		COLUMN_DIGIT(row, 12, uint8, _deleteflag2);

		fromFID = from_fid;
		toFID = _to_fid;
		mail_status = _mail_status;
		mail_kind = _mail_kind;
		deleteflag1 = _deleteflag1;
		deleteflag2 = _deleteflag2;
	}

	// 거절가능성 검사 : 보통메일이고 읽음상태에 있는 메일만 거절할수 있다. toFID = FID 이면 System Mail이다.
	if((toFID != FID) || (mail_kind != MK_NORMAL) || (mail_status == MS_NEW_MAIL) || (deleteflag2 != 0))
	{
		// 비법파케트발견
		sipwarning("User refuse impossible mail! FID=%d, MailID=%d", FID, MailID);
		return;
	}

	//// 대방의 Mailbox에 있는 메일수 검사 - 거절시에는 받는 사람의 메일함을 검사하지 않는다.
	//uint16	NewCount = 0;
	//uint16	SendCount = 0;
	//uint16	ReceiveCount = 0;
	//if(GetMailboxStatus(fromFID, NewCount, SendCount, ReceiveCount))
	//{
	//	if(ReceiveCount >= MAX_MAIL_NUM)
	//	{
	//		uint32	ret = E_RMAILBOX_OVERFLOW;
	//		CMessage	msgout(M_SC_MAIL_REJECT);
	//		msgout.serial(FID, ret, MailID);
	//		CUnifiedNetwork::getInstance()->send(_tsid, msgout);

	//		// 받는사람에게 Mailbox가 다 차서 메일을 받을수 없다는것을 통지한다.
	//		Send_M_SC_MAILBOX_STATUS(fromFID, 0, 0, 1);
	//		return;
	//	}
	//}
	//else
	//{
	//	sipwarning("SendMail DB Error in GetMailboxStatus!");
	//	return;
	//}

	// 메일 거절 처리
	{
		MAKEQUERY_RejectMail(strQ, MailID, FID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Main_Mail_Return: prepare statement failed!");
			return;	// E_DBERR
		}

		SQLLEN len1 = 0;

		DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len1);
		if (!nBindResult)
		{
			sipwarning("Main_Mail_Return: bind failed!");
			return; // E_DBERR
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult)
		{
			sipwarning("Main_Mail_Return: execute failed!");
			return;	// E_DBERR
		}

		if (ret != 0)
		{
			// 비법파케트발견
			sipwarning("User refused other's mail! UserFID=%d, MailID=%d", FID, MailID);
			return;
		}

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (!IS_DB_ROW_VALID(row))
		{
			sipwarning("Main_Mail_Return: return empty!");
			return;	// E_DBERR
		}

		COLUMN_DATETIME(row, 1, mail_datetime);

		// 클라이언트에 거절처리완성 통지
		ret = 0;
		CMessage	msgOut(M_SC_MAIL_REJECT);
		msgOut.serial(fsSid, FID, ret, MailID, mail_datetime);

		CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
	}

	// 대방에게 새 메일이 왔음을 통지한다.
	Send_M_SC_MAILBOX_STATUS(fromFID, 1, 0, 0);
}

// [d:MailId][b:count][ [w:InvenPos] ]
// 메일에서 아이템을 수집한다.
void	cb_M_CS_MAIL_ACCEPT_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint16				rsSid;				_msgin.serial(rsSid);
	T_FAMILYID			FID;				_msgin.serial(FID);				// 가족id
	uint32				SyncCode;			_msgin.serial(SyncCode);
	uint32				MailID;				_msgin.serial(MailID);
	uint8				count;				_msgin.serial(count);

	if(count <= 0 || count > MAX_MAIL_ITEMCOUNT)
	{
		// 비법파케트발견
		sipwarning("Invalid Count! FID=%d, MailID=%d, Count=%d", FID, MailID, count);
		return;
	}

	uint16		InvenPos[MAX_MAIL_ITEMCOUNT];
	memset(InvenPos, 0, sizeof(InvenPos));
	uint8		i;
	for(i = 0; i < count; i ++)
	{
		_msgin.serial(InvenPos[i]);
	}

	_MailItems		MailItems;
	uint8	MailItemBuf[MAX_MAILITEMBUF_SIZE];
	SQLLEN	lMailItemBufSize = sizeof(MailItemBuf);

	uint8	mail_status = 0, mail_kind = 0;
	sint32	ret = 0;
	uint8	mail_itemexist;

	T_FAMILYID fromFid = 0;
	T_ACCOUNTID fromUId = 0;
	ucstring fromName = L"";
	// 메일 상태 얻기
	{
		MAKEQUERY_GetMail(strQ, MailID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Main_Mail_Get: prepare statement failed!");
			return;	// E_DBERR
		}

		SQLLEN len1 = 0;

		DB_EXE_BINDPARAM_OUT(Stmt, 1, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len1);
		if (!nBindResult)
		{
			sipwarning("Main_Mail_Get: bind failed!");
			return; // E_DBERR
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult || ret != 0)
		{
			sipwarning("Main_Mail_Get: execute failed!");
			return;	// E_DBERR
		}

		DB_QUERY_RESULT_FETCH(Stmt, row);
		if (!IS_DB_ROW_VALID(row))
		{
			sipwarning("Main_Mail_Get: return empty!");
			return;	// E_DBERR
		}

		COLUMN_DIGIT(row, 0, uint32, from_fid);
		COLUMN_WSTR(row, 1, from_name, MAX_LEN_FAMILY_NAME);
		COLUMN_DIGIT(row, 2, uint32, to_fid);
		COLUMN_DIGIT(row, 4, uint8, _mail_status);
		COLUMN_DIGIT(row, 5, uint8, _mail_kind);
		COLUMN_DIGIT(row, 9, uint8, _mail_itemexist);
		row.GetData(10+1, MailItemBuf, static_cast<ULONG>(lMailItemBufSize), &lMailItemBufSize, SQL_C_BINARY);
		COLUMN_DIGIT(row, 11, uint8, _deleteflag1);
		COLUMN_DIGIT(row, 12, uint8, _deleteflag2);
		COLUMN_DIGIT(row, 13, uint32, from_uid);

		mail_status = _mail_status;
		mail_kind = _mail_kind;
		mail_itemexist = _mail_itemexist;

		fromFid = from_fid;
		fromName = from_name;
		fromUId = from_uid;

		if (FID != to_fid)
		{
			// 비법파케트발견
			sipwarning("User accept other's mail! UserFID=%d, MailID=%d", FID, MailID);
			return;
		}
		if (((FID == from_fid) && (_deleteflag1 != 0)) || ((FID == to_fid) && (_deleteflag2 != 0)))
		{
			// 비법파케트발견
			sipwarning("User require deleted mail! UserFID=%d, MailID=%d", FID, MailID);
			return;
		}

		if (!MailItems.fromBuffer(MailItemBuf, (uint32)lMailItemBufSize))
		{
			sipwarning("fromBuffer failed. FID=%d, MailID=%d", FID, MailID);
			return;
		}
	}

	// 아이템수집가능성 검사 : 읽음상태에 있는 메일만 아이템수집 가능하다.
	if (mail_status != MS_OPENED_MAIL)
	{
		// 비법파케트발견
		sipwarning("User accept impossible mail! UserFID=%d, MailID=%d", FID, MailID);
		return;
	}

	// 아이템목록을 보낸다.
	CMessage	msgOut(M_NT_MAIL_ITEM_FOR_ACCEPT);
	msgOut.serial(rsSid, FID, SyncCode, MailID, count);
	for(i = 0; i < count; i ++)
	{
		msgOut.serial(InvenPos[i]);
	}
	msgOut.serial(fromFid, fromUId, mail_kind, fromName);
	for(i = 0; i < MAX_MAIL_ITEMCOUNT; i ++)
	{
		msgOut.serial(MailItems.Items[i].ItemID, MailItems.Items[i].ItemCount, MailItems.Items[i].MoneyType);
	}
	CUnifiedNetwork::getInstance()->send(_tsid, msgOut);
}

// 메일아이템받기가 완료됨을 통지한다. ([d:MailID])
void	cb_M_NT_MAIL_ACCEPT_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32				MailID;				_msgin.serial(MailID);

	// 메일 처리 완료표시를 한다.
	{
		uint8	mail_status = MS_RECEIVED_ITEM;	// 메일 처리 완료 표시

		MAKEQUERY_SetMailStatus(strQ, MailID, mail_status);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Main_Mail_SetStatus: prepare statement failed!");
			return;	// E_DBERR
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult)
		{
			sipwarning("Main_Mail_SetStatus: execute failed!");
			return;	// E_DBERR
		}
	}
}

void SendTestItemMail(T_FAMILYID toFID, ucstring Title, ucstring Content, uint8 count, _MailItems &MailItems) 
{
	// 메일목록에 추가
	{
		uint8	MailItemBuf[MAX_MAILITEMBUF_SIZE];
		uint32	MailItemBufSize = sizeof(MailItemBuf);

		if(!MailItems.toBuffer(MailItemBuf, &MailItemBufSize))
		{
			sipwarning("Can't toBuffer mail items!");
			return;
		}

		uint8	item_exist = (count > 0) ? 1 : 0;
		MAKEQUERY_InsertMail_GM(strQ, toFID, Title, Content, item_exist);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Shrd_Mail_Insert_GM: prepare statement failed!");
			return;	// E_DBERR
		}

		SQLLEN	len1 = MailItemBufSize, len2 = 0;
		sint32	ret = 0;

		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_INVENBUF_SIZE, 0, MailItemBuf, 0, &len1);
		DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len2);
		if (!nBindResult)
		{
			sipwarning("Shrd_Mail_Insert_GM: bind failed!");
			return; // E_DBERR
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult)
		{
			sipwarning("Shrd_Mail_Insert_GM: execute failed!");
			return;	// E_DBERR
		}
	}

	// 대방에게 새 메일이 왔음을 통지한다.
	Send_M_SC_MAILBOX_STATUS(toFID, 1, 0, 0);

	sipinfo("SendTestItemMail SUCCESS");
}

void SendTestItemMail(T_FAMILYID toFID, ucstring Title, ucstring Content, uint8 count, _MailItems &MailItems, bool notice)
{
	// 메일목록에 추가
	{
		uint8	MailItemBuf[MAX_MAILITEMBUF_SIZE];
		uint32	MailItemBufSize = sizeof(MailItemBuf);

		if (!MailItems.toBuffer(MailItemBuf, &MailItemBufSize))
		{
			sipwarning("Can't toBuffer mail items!");
			return;
		}

		uint8	item_exist = (count > 0) ? 1 : 0;
		MAKEQUERY_InsertMail_GM(strQ, toFID, Title, Content, item_exist);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Shrd_Mail_Insert_GM: prepare statement failed!");
			return;	// E_DBERR
		}

		SQLLEN	len1 = MailItemBufSize, len2 = 0;
		sint32	ret = 0;

		DB_EXE_BINDPARAM_IN(Stmt, 1, SQL_C_BINARY, SQL_VARBINARY, MAX_INVENBUF_SIZE, 0, MailItemBuf, 0, &len1);
		DB_EXE_BINDPARAM_OUT(Stmt, 2, SQL_C_ULONG, SQL_INTEGER, 0, 0, &ret, 0, &len2);
		if (!nBindResult)
		{
			sipwarning("Shrd_Mail_Insert_GM: bind failed!");
			return; // E_DBERR
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult)
		{
			sipwarning("Shrd_Mail_Insert_GM: execute failed!");
			return;	// E_DBERR
		}
	}

	if (notice)
	{
		// 대방에게 새 메일이 왔음을 통지한다.
		Send_M_SC_MAILBOX_STATUS(toFID, 1, 0, 0);

		sipinfo("SendTestItemMail SUCCESS");
	}
}

// 이전에 구입한 방에 대하여 추가포인트가 지불되였음. ([d:FID][d:RoomID][u:RoomName][d:AddPoint])
void cb_M_NT_TEMP_MAIL_ADDPOINT_TO_OLDROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
{
	uint32	FID;			_msgin.serial(FID);
	uint32	RoomID;			_msgin.serial(RoomID);
	ucstring	RoomName;	_msgin.serial(RoomName);
	uint32	AddPoint;		_msgin.serial(AddPoint);

	tchar buf1[512] = _t("");
	tchar buf2[512] = _t("");
	smprintf(buf1, 512, L"%s(%c%d)", RoomName.c_str(), L'A' + ((RoomID >> 24) & 0xFF) - 1, (RoomID & 0x00FFFFFF));
	smprintf(buf2, 512, GetLangText(ID_TEST_ADDPOINT_MAIL_CONTENT).c_str(), buf1, AddPoint);

	_MailItems	MailItems;
	SendTestItemMail(FID, GetLangText(ID_TEST_ADDPOINT_MAIL_TITLE), buf2, 0, MailItems, false);

	// 2020/05/30 방관련통지를 위한 처리
	{
		MAKEQUERY_Temp_RegisterNoticeUser(strQ, FID);
		CDBConnectionRest	DB(MainDB);
		DB_EXE_PREPARESTMT(DB, Stmt, strQ);
		if (!nPrepareRet)
		{
			sipwarning("Main_temp_RegisterNoticeUser: prepare statement failed!");
			return;	// E_DBERR
		}

		DB_EXE_PARAMQUERY(Stmt, strQ);
		if (!nQueryResult)
		{
			sipwarning("Main_temp_RegisterNoticeUser: execute failed!");
			return;	// E_DBERR
		}
	}
}

SIPBASE_CATEGORISED_COMMAND(OROOM_S, sendTestItemMail, "send item to family on mailing", "FID ItemID ItemCount")
{
	if (args.size() < 3)
		return false;

	string v0	= args[0];
	uint32 fid = atoui(v0.c_str());
	string v1	= args[1];
	uint32 itemid = atoui(v1.c_str());
	string v2	= args[2];
	uint32 itemcount = atoui(v2.c_str());

	_MailItems	MailItems;
	MailItems.Items[0].ItemID = itemid;
	MailItems.Items[0].ItemCount = (uint16)itemcount;

	SendTestItemMail(fid, L"Item", L"Send item for testing", (uint8)itemcount, MailItems, true);
	log.displayNL ("send test item(%d) to family(%d).", itemid, fid);

	return true;
}

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

//void	cb_M_GMCS_GET_GMONEY(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid)
//{
//	sipwarning("This is not supported!!!");
//	return;
//	//T_FAMILYID		GMID;		_msgin.serial(GMID);
//	//T_ACCOUNTID		userID;		_msgin.serial(userID);
//
//	//// get fid with the user ID
//	//MAKEQUERY_FAMILYINFOFROMNAME_USERID(strQuery, userID);
//	//DBCaller->execute(strQuery, DBCB_DBFamilyInfoForGMGetGMoney,
//	//	"DDW",
//	//	CB_,		GMID,
//	//	CB_,		userID,
//	//	CB_,		_tsid.get());
//}
