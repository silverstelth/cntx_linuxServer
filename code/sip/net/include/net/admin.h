/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __ADMIN_H__
#define __ADMIN_H__


//
// Inlcudes
//

#include <string>
#include <vector>


namespace SIPNET {


//
// Structures
//

struct CAlarm
{
	CAlarm (const std::string &n, sint l, bool gt) : Name(n), Limit(l), GT(gt), Activated(false) { }
	
	std::string Name;		// variable name
	int	 Limit;				// limit value where the alarm is setted
	bool GT;				// true if the error is produce when var is greater than bound
	
	bool Activated;			// true if the limit is exceeded (mail is send everytimes the actived bool change from false to true)
};

struct CGraphUpdate
{
	CGraphUpdate (const std::string &n, sint u) : Name(n), Update(u), LastUpdate(0) { }
	
	std::string Name;		// variable name
	int	 Update;			// delta time in second when we have to check variable

	uint32	LastUpdate;		// in second
};

typedef void (*TRemoteClientCallback) (uint32 rid, const std::string &cmd, const std::string &entityNames);


//
// Externals
//

extern std::vector<CGraphUpdate> GraphUpdates;
extern std::vector<CAlarm> Alarms;


//
// Types
//

typedef std::vector<std::string> TAdminViewVarNames;
typedef std::vector<std::string> TAdminViewValues;
struct SAdminViewRow
{
	TAdminViewVarNames	VarNames;
	TAdminViewValues		Values;

	SAdminViewRow() {}
	SAdminViewRow(const TAdminViewVarNames& varNames, const TAdminViewValues& values): VarNames(varNames), Values(values) {}
};
typedef std::vector< SAdminViewRow > TAdminViewResult;


//
// Functions
//

void initAdmin (bool dontUseAES, uint16 aesport);

void updateAdmin ();

void setInformations (const std::vector<std::string> &alarms, const std::vector<std::string> &graphupdate);

void serviceGetView (uint32 rid, const std::string &rawvarpath, TAdminViewResult& answer, bool async=false);

void setRemoteClientCallback (TRemoteClientCallback cb);

void addRequestAnswer (uint32 rid, const TAdminViewVarNames& varNames, const TAdminViewValues& values);

} // SIPNET


#endif // SIP_ALARMS_H

/* End of email.h */
