/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __NET_LOG_H__
#define __NET_LOG_H__

#include "misc/types_sip.h"
#include "misc/log.h"
#include <string>

namespace SIPNET {


/**
 * Logger for network transfers
 * \date 2000
 */
class CNetLog : public SIPBASE::CLog
{
public:

	/// Constructor
	CNetLog();

	/// Sets the name of the running service
	void	setServiceName( const char *name );

	/// Log an output transfer (send)
	void	output( const char *srchost, uint8 msgnum, const char *desthost,
					const char *msgname, uint32 msgsize );

	/// Log an input transfer (receive)
	void	input( const char *srchost, uint8 msgnum, const char *desthost );

	/*/// Enables or disable logging.
	void	setLogging( bool logging )
	{
		_Logging = logging;
	}*/

private:

	std::string _ProcessId;

};


extern CNetLog NetLog;

#define sipsetnet (servicename,displayer)
#define sipnetoutput NetLog.output
#define sipnetinput NetLog.input


} // SIPNET


#endif // __NET_LOG_H__

/* End of net_log.h */
