/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CALLBACK_SERVER_H__
#define __CALLBACK_SERVER_H__

#include "misc/types_sip.h"
#include "net/security_server.h"

namespace SIPNET {


/**
 * Server class for layer 3
 * \date 2001
 */
class CCallbackServer : public CCallbackNetBase, public CBufServer
{
public:

	/// Constructor
	CCallbackServer( bool bUdpServer= false, bool bSecurity=false, bool bClientServer=false, uint32 max_frontnum=1000, TRecordingState rec=Off, const std::string& recfilename="", bool recordall=true, bool initPipeForDataAvailable=true );

	/// Sends a message to the specified host
	virtual void	send (const CMessage &buffer, TTcpSockId hostid = InvalidSockId, uint8 bySendType = REL_GOOD_ORDER);

	/// Force to send all data pending in the send queue. See comment in CCallbackNetBase.
	bool	flush (TTcpSockId destid, uint *nbBytesRemaining=NULL) { checkThreadId (); sipassert( destid != InvalidSockId ); return CBufServer::flush(destid, nbBytesRemaining); }

	/** Updates the network (call this method evenly).
	 * More info about timeout and mintime in the code of CCallbackNetBase::baseUpdate().
	 */
	void	update2 (sint32 timeout=-1, sint32 mintime=0, uint32 *RecvTime=NULL, uint32 *SendTime=NULL, uint32 *ProcNum=NULL);

	/// Updates the network (call this method evenly) (legacy)
	void	update (sint32 timeout=0);

	/// Sets callback for incoming connections (or NULL to disable callback)
	void	setConnectionCallback (TNetCallback cb, void *arg) { checkThreadId (); _ConnectionCallback = cb; _ConnectionCbArg = arg; }

	/// Sets callback for disconnections (or NULL to disable callback)
	void	setDisconnectionCallback (TNetCallback cb, void *arg) { checkThreadId (); _DisconnectionCallback = cb; _DisconnectionCbArg = arg; }

	/// Returns true if the connection is still connected. on server, we always "connected"
	bool	connected () const { checkThreadId (); return true; } 

	void	NotifyNewConnection(TTcpSockId hostid);
	void	NotifyDisConnection(TTcpSockId hostid);
	

	/** Disconnect a connection
	 * Set hostid to InvalidSockId to disconnect all connections.
	 * If hostid is not InvalidSockId and the socket is not connected, the method does nothing.
	 * Before disconnecting, any pending data is actually sent.
	 */
	void	disconnect (TTcpSockId hostid);

	// force close connection without graceful connection
	void	forceClose (TTcpSockId hostid);

	void	noCallback (std::string sMes, TTcpSockId tsid);

	/// Returns the address of the specified host
	const CInetAddress& hostAddress (TTcpSockId hostid) { sipassert(hostid!=InvalidSockId); checkThreadId(); return CBufServer::hostAddress (hostid); }

	/// Returns the sockid (cf. CCallbackClient)
	virtual TTcpSockId	getTcpSockId (TTcpSockId hostid = InvalidSockId);

	uint64	getReceiveQueueSize () { return CBufServer::getReceiveQueueSize(); }
	uint64	getSendQueueSize () { return CBufServer::getSendQueueSize(0); }

	void displayReceiveQueueStat (SIPBASE::CLog *log = SIPBASE::InfoLog) { CBufServer::displayReceiveQueueStat(log); }
	void displaySendQueueStat (SIPBASE::CLog *log = SIPBASE::InfoLog, TTcpSockId destid = InvalidSockId) { CBufServer::displaySendQueueStat(log, destid); }
	
	void displayThreadStat (SIPBASE::CLog *log = SIPBASE::InfoLog) { CBufServer::displayThreadStat(log); }

	void	cbClientCertificate( CMessage& msgin, TSockId from );
	void	cbClientCyclePing( CMessage& msgin, TSockId from );
	
	CSecurityFunction	m_security;
	void		AddSendPacketCryptMethod(const TSpecMsgCryptRequest *requestarray, uint32 arraysize)
	{
		m_security.AddSendPacketCryptMethod(requestarray, arraysize);
	}
	void		AddRecvPacketCryptMethod(const TSpecMsgCryptRequest *requestarray, uint32 arraysize)
	{
		m_security.AddRecvPacketCryptMethod(requestarray, arraysize);
	}

	void		SetWarningMessageLengthForSend(uint32 s) { _WarningMessageLengthForSend = s; }
private:

	/// This function is public in the base class and put it private here because user cannot use it in layer 2
	void			send (const SIPBASE::CMemStream &buffer, TTcpSockId hostid) { sipstop; }

	bool			dataAvailable ();
	virtual bool	getDataAvailableFlagV() const { return dataAvailableFlag(); }

	virtual void	receive (CMessage &buffer, TTcpSockId *hostid);

	void			sendAllMyAssociations (TTcpSockId to);
	void			HandShakeOK(TTcpSockId from);

	TNetCallback	_ConnectionCallback;
	void			*_ConnectionCbArg;
	friend void		cbsNewConnection (TTcpSockId from, void *data);

	TNetCallback	_DisconnectionCallback;
	void			*_DisconnectionCbArg;
	friend void		cbsNewDisconnection (TTcpSockId from, void *data);

	uint32			_WarningMessageLengthForSend;
	void						updatePing();

	// ---------------------------------------
#ifdef USE_MESSAGE_RECORDER
	void						noticeConnection( TTcpSockId hostid );
	virtual						bool replaySystemCallbacks();
	std::vector<CBufSock*>		_MR_Connections;
	std::map<TTcpSockId,TTcpSockId>	_MR_SockIds; // first=sockid in file; second=CBufSock*
#endif
	// ---------------------------------------
	
};


} // SIPNET


#endif // __CALLBACK_SERVER_H__

/* End of callback_server.h */
