#ifndef ADMIN_MANAGE_MONITOR_SPEC_H
#define ADMIN_MANAGE_MONITOR_SPEC_H

#include "admin_main_service.h"

extern	void	InitManageMonitorSpec();

extern	void	Make_MVARIABLE(SIPNET::CMessage *pMes);
extern	void	SendMonitorInfoToAES(SIPNET::TServiceId _TServiceId);
extern	void	cbGraphUpdate (SIPNET::CMessage &msgin, const std::string &serviceName, SIPNET::TServiceId tsid);
extern	void	cbServiceNotPong(SIPNET::CMessage &msgin, const std::string &serviceName, SIPNET::TServiceId tsid);

#endif	// ADMIN_MANAGE_MONITOR_SPEC_H