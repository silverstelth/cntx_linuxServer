/** \file admin_service.h
 * <File description>
 *
 * $Id: admin_main_service.h,v 1.4 2015/11/06 00:23:48 RyuSangChol Exp $
 */


#ifndef SIP_ADMIN_SERVICE_H
#define SIP_ADMIN_SERVICE_H

#include "misc/types_sip.h"
#include "net/service.h"
#include "misc/db_interface.h"
#include "../../common/commonForAdmin.h"
#include "misc/DBCaller.h"

extern	SIPBASE::CDBCaller		*DBCaller;
extern	SIPBASE::CDBCaller		*DBMainCaller;

#endif // SIP_ADMIN_SERVICE_H

/* End of admin_service.h */

