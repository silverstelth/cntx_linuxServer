#ifndef ADMIN_MONITOR_ACCOUNT_H
#define ADMIN_MONITOR_ACCOUNT_H

#include "admin_main_service.h"

extern SIPNET::CCallbackServer *	MonitorServer;

extern	void	InitMonitorAccount();
extern	void	UpdateMonitorAccount();
extern	void	ReleaseMonitorAccount();

extern	void	SendHostConnectionToMonitor(SIPNET::TServiceId tsid);
extern	void	SendHostDisConnectionToMonitor(SIPNET::TServiceId tsid);
extern	void	SendServiceConnectionToMonitor(uint16 sid, SIPNET::TServiceId tsidAES);
extern	void	SendServiceDisConnectionToMonitor(uint16 sid, SIPNET::TServiceId tsidAES);

#endif	// ADMIN_MONITOR_ACCOUNT_H