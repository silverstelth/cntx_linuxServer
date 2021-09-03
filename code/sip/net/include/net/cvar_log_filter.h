/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CVAR_LOG_FILTER_H__
#define __CVAR_LOG_FILTER_H__

#include "misc/config_file.h"
#include "net/service.h"

/** Declare an info loging function that works as sipinfo but that is activated with the given service config file variable
  * Example of use :
  * DECLARE_CVAR_INFO_LOG_FUNCTION(my_info, MyInfoEnabled)
  *
  * my_info("my_message"); // no-op if "MyInfoEnabled = 0;" Is found in the service config file
  */
#ifdef SIP_RELEASE
	#define SIP_DECLARE_CVAR_INFO_LOG_FUNCTION(func, cvar, defaultValue)	inline void func(const char *format, ...) {}
#else
	#define SIP_DECLARE_CVAR_INFO_LOG_FUNCTION(func, cvar, defaultValue)                                              \
	inline void func(const char *format, ...)                                                                        \
	{                                                                                                                \
		bool logWanted = (defaultValue);                                                                             \
		SIPBASE::CConfigFile::CVar *logWantedPtr = SIPNET::IService::getInstance()->ConfigFile.getVarPtr(#cvar);       \
		if (logWantedPtr)                                                                                            \
		{                                                                                                            \
			logWanted = logWantedPtr->asInt() != 0;                                                                  \
		}                                                                                                            \
		if (logWanted)                                                                                               \
		{                                                                                                            \
			char *out;																								 \
			SIPBASE_CONVERT_VARGS(out, format, 256);                                                                  \
			sipinfo(out);                                                                                             \
		}                                                                                                            \
	}
#endif


#endif
