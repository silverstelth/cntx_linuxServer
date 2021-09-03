/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __LOGIN_SERVER_H__
#define __LOGIN_SERVER_H__

#include "misc/types_sip.h"

#include <string>
#include <vector>

#include "login_cookie.h"
#include "SecurityServer.h"
#include "./../../../common/common.h"
#include "./../../../common/err.h"


namespace SIPBASE
{
	class CConfigFile;
}

namespace SIPNET
{

/// Callback function type called when a new client is identified (with the login password procedure)
typedef void (*TNewClientCallback) (TTcpSockId from, const CLoginCookie &cookie);

typedef void (*TForceConnectionCallback) (TTcpSockId from, uint32	userid);

/// Callback function type called when a client need to be disconnected (double login...)
typedef void (*TDisconnectClientCallback) (uint32 userId, const std::string &reqServiceName);

class CUdpSock;

/** This class is the server part of the Login System. It is used in the Front End Service.
 * At the begining, it connects to the WS. When a new player comes in and is authenticated, a
 * callback is called to warn the user code that a new player is here.
 * Example:
 * \code
 * \endcode
 * \date 2001
 */
class CLoginServer {
public:

	/// Create the connection to the Welcome Service and install callbacks to the callback server (for a TCP connection)
	/// init() will try to find the ListenAddress in the config file and it will be used to say to the client
	/// the address to connect to this frontend (using the login system). You can modify this in real time in
	/// the config file or with the ls_listen_address command
	/// The ListenAddress must be in the form of "www.samilpo.org:38000" (ip+port)
	static void init (CCallbackServer *server, CInetAddress & listenAddr, TNewClientCallback ncl);

	static void	setForceConnectionCallback (CCallbackServer *server, TForceConnectionCallback cb);

	/// Create the connection to the Welcome Service for an UDP connection
	/// the dc will be call when the Welcome Service decides to disconnect a player (double login...)
	static void init (CUdpSock &server, TDisconnectClientCallback dc);

	/// Create the connection to the Welcome Service for a connection
	/// the dc will be call when the Welcome Service decides to disconnect a player (double login...)
	static void init (const std::string &listenAddr, TDisconnectClientCallback dc);

	static void init (CCallbackServer *server); // temparary

	/// Used only in UDP, check if the cookie is valid. return empty string if valid, reason otherwise
	static T_ERR isValidCookie (const CLoginCookie &lc, uint8 &userPriv, uint32 &instanceId, uint32 &charSlot, TTcpSockId from, CCallbackNetBase &netbase);

	/// Call this method when a user is disconnected or the server disconnect the user.
	/// This method will warn the login system that the user is not here anymore
	static void onClientDisconnected (uint32 userId, uint8 bIsGM);

	/*
	static void onClientConnected (TSockId from, void *arg);

	// HELLO Reply
	static void onMyclientReply(SIPNET::CMessage msgin, TSockId from, CCallbackNetBase& clientcb);

	*/
	static void disconnectUser(TSockId from);
	
	/// Call this method to retrieve the listen address
	static const std::string &getListenAddress();

	/// Return true if we are in 'dev' mode
	static bool acceptsInvalidCookie();

	/// Set the actual listen address
	static void setListenAddress(const std::string &la);

	/// Return the number of pending client connection.
	static uint32 getNbPendingUsers();

	/// Refresh the list of pending cookies to remove outdated one 
	/// (i.e. cookies for users that never connect)
	static void refreshPendingList();

private:

	/// This function is used by init() to create the connection to the Welcome Service
	static void connectToWS ();

	// called by other init()
	static void init (const std::string &listenAddress);	
};


} // SIPNET

#endif // __LOGIN_SERVER_H__

/* End of login_server.h */
