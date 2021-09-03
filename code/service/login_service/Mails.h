#ifndef _MAILS_H
#define _MAILS_H

#include "../../common/Packet.h"
#include "../../common/common.h"
#include "../../common/Macro.h"
#include "../Common/Common.h"


typedef ITEM_LIST<MAX_MAIL_ITEMCOUNT>	_MailItems;
#define		MAX_MAILITEMBUF_SIZE		150

void cb_M_NT_REQ_MAILBOX_STATUS_CHECK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_MAIL_GET_LIST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_MAIL_GET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_MAIL_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_MAIL_SEND(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_MAIL_DELETE(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_MAIL_REJECT(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_CS_MAIL_ACCEPT_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_MAIL_ACCEPT_ITEM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
void cb_M_NT_TEMP_MAIL_ADDPOINT_TO_OLDROOM(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
#endif // _MAILS_H

