#ifndef _MAIL_H
#define _MAIL_H

#include "../../common/Packet.h"
#include "../../common/common.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"


typedef ITEM_LIST<MAX_MAIL_ITEMCOUNT>	_MailItems;
#define		MAX_MAILITEMBUF_SIZE		150

void SendCheckMailboxResult(T_FAMILYID _FID, SIPNET::TServiceId _tsid);
void cb_M_CS_MAIL_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_MAILBOX_STATUS_FOR_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_SC_MAIL_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_MAIL_ACCEPT_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_MAIL_ITEM_FOR_ACCEPT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

#endif // _MAIL_H

