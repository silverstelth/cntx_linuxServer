#pragma once

#define MAKEQUERY_GETACCOUNT(strQ, sUserName, sPassword)	\
	tchar strQ[512] = _t("");								\
	SIPBASE::smprintf(strQ, 512, _t("{CALL CeUser_User_loginUSERtoMain('%s','%s',?,?)}"), sUserName, sPassword);

#define MAKEQUERY_UPDATEACCOUNTREGISTERSTATE(strQ, UserID)	\
	tchar strQ[512] = _t("");								\
	SIPBASE::smprintf(strQ, 512, _t("{CALL CeUser_User_UserUPdate(%d)}"), UserID);
