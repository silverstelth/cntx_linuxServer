/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __NET_DISPLAYER_H__
#define __NET_DISPLAYER_H__

#include "misc/log.h"
#include "misc/displayer.h"

#include "callback_client.h"

namespace SIPNET {


/**
 * Net Displayer. Sends the strings to a logger server (LOGS).
 * \ref log_howto
 * \bug When siperror is called in a catch block, a connected NetDisplayer becomes an IDisplayer => pure virtual call
 * \date 2000
 */

class CNetDisplayerA : public SIPBASE::IDisplayerA
{
public:

	/// Constructor
	CNetDisplayerA(bool autoConnect = true);

	/** Sets logging server address. Call this method from outside only if you want to use a LOGS not registered within the NS.
	 * It does nothing if the displayer is already connected to a server.
	 */
	void setLogServer( const CInetAddress& logServerAddr );

	/** Sets logging server with an already connected server.
	 */
	void setLogServer( CCallbackClient *server );

	/// Returns true if the displayer is connected to a Logging Service.
	bool connected () { return _Server->connected(); }

	/// Destructor
	virtual ~CNetDisplayerA();
		
protected:

	/** Sends the string to the logging server
	 * \warning If not connected, tries to connect to the logging server each call. It can slow down your program a lot.
	 */
	virtual void doDisplay ( const SIPBASE::CLogA::TDisplayInfo& args, const char *message);

	 /// Find the server (using the NS) and connect
	void findAndConnect();

private:

	CInetAddress	_ServerAddr;
//	CCallbackClient	_Server;
	CCallbackClient	*_Server;
	bool			_ServerAllocated;
//	uint32			_ServerNumber;
};

class CNetDisplayerW : public SIPBASE::IDisplayerW
{
public:

	/// Constructor
	CNetDisplayerW(bool autoConnect = true);

	/** Sets logging server address. Call this method from outside only if you want to use a LOGS not registered within the NS.
	 * It does nothing if the displayer is already connected to a server.
	 */
	void setLogServer( const CInetAddress& logServerAddr );

	/** Sets logging server with an already connected server.
	 */
	void setLogServer( CCallbackClient *server );

	/// Returns true if the displayer is connected to a Logging Service.
	bool connected () { return _Server->connected(); }

	/// Destructor
	virtual ~CNetDisplayerW();
		
protected:

	/** Sends the string to the logging server
	 * \warning If not connected, tries to connect to the logging server each call. It can slow down your program a lot.
	 */
	virtual void doDisplay ( const SIPBASE::CLogW::TDisplayInfo& args, const ucchar *message);

	 /// Find the server (using the NS) and connect
	void findAndConnect();

private:

	CInetAddress	_ServerAddr;
//	CCallbackClient	_Server;
	CCallbackClient	*_Server;
	bool			_ServerAllocated;
//	uint32			_ServerNumber;
};

} // SIPNET


#endif // __NET_DISPLAYER_H__

/* End of net_displayer.h */
