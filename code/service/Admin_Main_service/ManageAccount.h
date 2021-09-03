#ifndef ADMIN_MANAGE_ACCOUNT_H
#define ADMIN_MANAGE_ACCOUNT_H

#include "admin_main_service.h"

extern SIPNET::CCallbackServer *	ManageHostServer;

extern	void	InitManageAccount();
extern	void	UpdateManageAccount();
extern	void	ReleaseManageAccount();

extern	const	ACCOUNTINFO * GetManagerInfo(SIPNET::TSockId from);
extern	void	AddManageHostLog(uint32 _uType, SIPNET::TSockId from, ucstring _ucContent);

enum	ENUM_MANAGELOG_TYPE {	LOG_MANAGE_ADDSERVICE = 1, LOG_MANAGE_DELSERVICE, LOG_MANAGE_ADDHOST, LOG_MANAGE_DELHOST, LOG_MANAGE_ADDSHARD, LOG_MANAGE_DELSHARD, 
								LOG_MANAGE_REQUEST, LOG_MANAGE_MANAGER, LOG_MANAGE_UPDATE, LOG_MANAGE_CHUPDATE, LOG_MANAGE_CONNECT, LOG_MANAGE_DISCONNECT};
#endif // ADMIN_MANAGE_ACCOUNT_H

