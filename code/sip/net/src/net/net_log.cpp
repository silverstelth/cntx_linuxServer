/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/net_log.h"
#include "net/unitime.h"



using namespace std;
using namespace SIPBASE;


namespace SIPNET {


CNetLog NetLog;


/*
 * Constructor
 */
CNetLog::CNetLog() :
	CLog()
{
}


/*
 * Log an output transfer (send)
 */
void CNetLog::output( const char *srchost, uint8 msgnum,
					  const char *desthost, const char *msgname, uint32 msgsize )
{
/*OLD	char line [1024];
	smprintf( line, 1024, "@@%"SIP_I64"d@%s@%hu@%s@%s@%s@%u@", (CUniTime::Sync?CUniTime::getUniTime():(TTime)0),
		srchost, (uint16)msgnum, _ProcessName.c_str(), desthost, msgname, msgsize );

	displayRawNL( line );
	*/
/*	displayRawNL( "@@%"SIP_I64"d@%s@%hu@%s@%s@%s@%u@", (CUniTime::Sync?CUniTime::getUniTime():(TTime)0),
		srchost, (uint16)msgnum, _ProcessName.c_str(), desthost, msgname, msgsize );
*/
	displayRawNL( "@@0@%s@%hu@%s@%s@%s@%u@",
		srchost, (uint16)msgnum, (_ProcessName).c_str(), desthost, msgname, msgsize );
}


/*
 * Log an input transfer (receive)
 */
void CNetLog::input( const char *srchost, uint8 msgnum, const char *desthost )
{
/*OLD	char line [1024];
	smprintf( line, 1024, "##%"SIP_I64"d#%s#%hu#%s#%s#", (CUniTime::Sync?CUniTime::getUniTime():(TTime)0),
			  srchost, msgnum, _ProcessName.c_str(), desthost );
	displayRawNL( line );
*/
/*	displayRawNL( "##%"SIP_I64"d#%s#%hu#%s#%s#", (CUniTime::Sync?CUniTime::getUniTime():(TTime)0),
		  srchost, msgnum, _ProcessName.c_str(), desthost );
*/
	displayRawNL( "##0#%s#%hu#%s#%s#", 
		  srchost, msgnum, (_ProcessName).c_str(), desthost );
}


} // SIPNET
