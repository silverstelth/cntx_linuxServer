/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/net_displayer.h"
#include "net/message.h"
#include "net/naming_client.h"


using namespace std;
using namespace SIPBASE;

namespace SIPNET {


/* This index must correspond to the index for "LOG" in CallbackArray in the Logging Service
 * (see CNetDisplayer::display())
 */
const sint16 LOG_CBINDEX = 0;


/*
 * Constructor
 */
CNetDisplayerA::CNetDisplayerA(bool autoConnect) :
	_Server(NULL), _ServerAllocated (false) // disable logging otherwise an infinite recursion may occur
{
	if (autoConnect) findAndConnect();
}


/*
 * Find the server (using the NS) and connect
 */
void CNetDisplayerA::findAndConnect()
{
	if (_Server == NULL)
	{
		_Server = new CCallbackClient();
		_ServerAllocated = true;
	}

	if ( CNamingClient::lookupAndConnect( "LOGS", *_Server ) )
	{
		sipdebug( "Connected to logging service" );
	}
}

/*
 * Sets logging server address
 */
void CNetDisplayerA::setLogServer (const CInetAddress& logServerAddr)
{
	if (_Server != NULL && _Server->connected()) return;

	_ServerAddr = logServerAddr;

	if (_Server == NULL)
	{
		_Server = new CCallbackClient();
		_ServerAllocated = true;
	}
	
	try
	{
		_Server->connect (_ServerAddr);
	}
	catch( ESocket& )
	{
		// Silence
	}
}

void CNetDisplayerA::setLogServer (CCallbackClient *server)
{
	if (_Server != NULL && _Server->connected()) return;

	_Server = server;
}


/*
 * Destructor
 */
CNetDisplayerA::~CNetDisplayerA ()
{
	if (_ServerAllocated)
	{
		_Server->disconnect ();
		delete _Server;
	}
}


/*
 * Sends the string to the logging server
 *
 * Log format: "2000/01/15 12:05:30 <LogType> <ProcessName>: <Msg>"
 */
void CNetDisplayerA::doDisplay ( const CLogA::TDisplayInfo& args, const char *message)
{
	try
	{
		if (_Server == NULL || !_Server->connected())
		{
			return;
		}

		bool needSpace = false;
		//stringstream ss;
		string str;

		if (args.Date != 0)
		{
			str += dateToHumanString(args.Date);
			needSpace = true;
		}

		if (args.LogType != CLog::LOG_NO)
		{
			if (needSpace) { str += " "; needSpace = false; }
			str += logTypeToString(args.LogType);
			needSpace = true;
		}

		if (!args.ProcessName.empty())
		{
			if (needSpace) { str += " "; needSpace = false; }
			str += args.ProcessName;
			needSpace = true;
		}
		
		if (needSpace) { str += ": "; needSpace = false; }

		str += message;

		CMessage msg(M_NET_DISPLAYER_LOG);
		string s = str;
		msg.serial( s );
		_Server->send (msg, 0, false);
	}
	catch( SIPBASE::Exception& )
	{
		// Silence
	}
}

// unicode version
CNetDisplayerW::CNetDisplayerW(bool autoConnect) :
	_Server(NULL), _ServerAllocated (false) // disable logging otherwise an infinite recursion may occur
{
	if (autoConnect) findAndConnect();
}


/*
 * Find the server (using the NS) and connect
 */
void CNetDisplayerW::findAndConnect()
{
	if (_Server == NULL)
	{
		_Server = new CCallbackClient();
		_ServerAllocated = true;
	}

	if ( CNamingClient::lookupAndConnect( "LOGS", *_Server ) )
	{
		sipdebug( "Connected to logging service" );
	}
}

/*
 * Sets logging server address
 */
void CNetDisplayerW::setLogServer (const CInetAddress& logServerAddr)
{
	if (_Server != NULL && _Server->connected()) return;

	_ServerAddr = logServerAddr;

	if (_Server == NULL)
	{
		_Server = new CCallbackClient();
		_ServerAllocated = true;
	}
	
	try
	{
		_Server->connect (_ServerAddr);
	}
	catch( ESocket& )
	{
		// Silence
	}
}

void CNetDisplayerW::setLogServer (CCallbackClient *server)
{
	if (_Server != NULL && _Server->connected()) return;

	_Server = server;
}


/*
 * Destructor
 */
CNetDisplayerW::~CNetDisplayerW ()
{
	if (_ServerAllocated)
	{
		_Server->disconnect ();
		delete _Server;
	}
}


/*
 * Sends the string to the logging server
 *
 * Log format: "2000/01/15 12:05:30 <LogType> <ProcessName>: <Msg>"
 */
void CNetDisplayerW::doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message)
{
	try
	{
		if (_Server == NULL || !_Server->connected())
		{
			return;
		}

		bool needSpace = false;
		//stringstream ss;
		basic_string<ucchar> str;

		if (args.Date != 0)
		{
			str += dateToHumanString(args.Date);
			needSpace = true;
		}

		if (args.LogType != CLogW::LOG_NO)
		{
			if (needSpace) { str += L" "; needSpace = false; }
			str += logTypeToString(args.LogType);
			needSpace = true;
		}

		if (!args.ProcessName.empty())
		{
			if (needSpace) { str += L" "; needSpace = false; }
			str += args.ProcessName;
			needSpace = true;
		}
		
		if (needSpace) { str += L": "; needSpace = false; }

		str += message;

		CMessage msg(M_NET_DISPLAYER_LOG);
		string s = SIPBASE::toString("%S", str.c_str());
		msg.serial( s );
		_Server->send (msg, 0, false);
	}
	catch( SIPBASE::Exception& )
	{
		// Silence
	}
}

} // SIPNET
