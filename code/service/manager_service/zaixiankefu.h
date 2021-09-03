#ifndef ZAIXIAN_PLAYER_H
#define ZAIXIAN_PLAYER_H

#include "misc/types_sip.h"
#include "misc/ucstring.h"
#include "net/service.h"
#include "../Common/Common.h"

extern void cb_M_NT_ZAIXIANGM_LOGON(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void cb_M_CS_ZAIXIAN_REQUEST(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void cb_M_CS_ZAIXIAN_END(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void cb_M_CS_ZAIXIAN_ASK(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void cb_M_GMCS_ZAIXIAN_ANSWER(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);
extern void cb_M_GMCS_ZAIXIAN_CONTROL(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void ProcForZaixianForLogout(T_FAMILYID _fid);

#endif // ZAIXIAN_PLAYER_H
