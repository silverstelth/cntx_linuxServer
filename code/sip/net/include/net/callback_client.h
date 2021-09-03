/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CALLBACK_CLIENT_H__
#define __CALLBACK_CLIENT_H__

#include "misc/types_sip.h"
#include "misc/CryptSystemManager.h"

#include "callback_net_base.h"
#include "buf_client.h"
namespace SIPNET {


class CInetAddress;


/**
 * Client class for layer 3
 * \date 2001
 */
class CCallbackClient : public CCallbackNetBase, public CBufClient
{
public:

	/// Constructor
	CCallbackClient( bool bHasUdp=false, bool bNoRecieve=false, TRecordingState rec=Off, const std::string& recfilename="", bool recordall=true, bool initPipeForDataAvailable=true );
	~CCallbackClient();

	/// Sends a message to the remote host (the second parameter isn't used)
	virtual void	send (const CMessage &buffer, TTcpSockId hostid = InvalidSockId, uint8 bySendType = REL_GOOD_ORDER);
	/// Force to send all data pending in the send queue. hostid must be InvalidSockId here. See comment in CCallbackNetBase.
	bool	flush (TTcpSockId hostid = InvalidSockId, uint *nbBytesRemaining=NULL);
	
	/** Updates the network (call this method evenly).
	 * More info about timeout and mintime in the code of CCallbackNetBase::baseUpdate().
	 */
	void	update2 (sint32 timeout=-1, sint32 mintime=0, uint32 *RecvTime=NULL, uint32 *SendTime=NULL, uint32 *ProcNum=NULL );

	/// Updates the network (call this method evenly) (legacy)
	void	update (sint32 timeout=0);

	/// Connects to the specified host
	void	connect( const CInetAddress& addr, uint32 usec = 0 );

	/** Returns true if the connection is still connected (changed when a disconnection
	 * event has reached the front of the receive queue, just before calling the disconnection callback
	 * if there is one)
	 */
	virtual bool	connected () const { checkThreadId (); return CBufClient::connected (); } 

	virtual const CInetAddress&	hostAddress( TTcpSockId hostid ) { return remoteAddress(); }

	/** Disconnect a connection
	 * Unlike in CCallbackClient, you can call disconnect() on a socket that is already disconnected
	 * (it will do nothing)
	 */
	void	disconnect (TTcpSockId hostid = InvalidSockId);

	// should not call this function
	void	forceClose (TTcpSockId hostid = InvalidSockId);

	void	noCallback (std::string sMes, TTcpSockId tsid);

	/// Sets callback for disconnections (or NULL to disable callback)
	void	setDisconnectionCallback (TNetCallback cb, void *arg) { checkThreadId (); _DisconnectionCallback = cb; _DisconnectionCbArg = arg; }

	/// Returns the sockid
	virtual TTcpSockId	getTcpSockId (TTcpSockId hostid = InvalidSockId);

	uint64	getReceiveQueueSize () { return CBufClient::getReceiveQueueSize(); }
	uint64	getSendQueueSize () { return CBufClient::getSendQueueSize(); }

	void displayReceiveQueueStat (SIPBASE::CLog *log = SIPBASE::InfoLog) { CBufClient::displayReceiveQueueStat(log); }
	void displaySendQueueStat (SIPBASE::CLog *log = SIPBASE::InfoLog, TTcpSockId destid = InvalidSockId) { CBufClient::displaySendQueueStat(log); }

	void displayThreadStat (SIPBASE::CLog *log = SIPBASE::InfoLog) { CBufClient::displayThreadStat(log); }
	void	cbM_KEYEXCHANGE(CMessage& msgin, TSockId from);
	void	cbM_CERTIFICATE(CMessage& msgin, TSockId from);
	void	cbM_HANDSHKOK(CMessage& msgin, TSockId from);
	void	cbM_CYCLEPING(CMessage& msgin, TSockId from);

protected:	
	// debug features, we number all packet to be sure that they are all sent and received
	// \todo remove this debug feature when ok
	uint32 SendNextValue, ReceiveNextValue;

	SIPBASE::ICryptMethod*	GetCryptMethod(uint32 typeID);
private:

	/// These function is public in the base class and put it private here because user cannot use it in layer 2
	void	send (const SIPBASE::CMemStream &buffer) { sipstop; }

	/// Returns true if there are messages to read
	bool	dataAvailable ();
	virtual bool getDataAvailableFlagV() const { return dataAvailableFlag(); }

	virtual void receive (CMessage &buffer, TTcpSockId *hostid = NULL);

	TNetCallback	_DisconnectionCallback;
	void			*_DisconnectionCbArg;
	friend void		cbcNewDisconnection (TTcpSockId from, void *data);

	void			UpdatePingProc();

	bool						m_bHandShakeOK;
	uint32						m_nDefaultCryptMethod;
	SIPBASE::TMapCryptMethod	m_mapCryptMethod;
	TMapSpecMsgCryptMethod		m_mapSpecSendMsgCryptMethod;
	uint32						m_nPingCycleMS;
	SIPBASE::TTime				m_tmLastRecvPing;
	sint32						m_nLastPingData;
	// ---------------------------------------
#ifdef USE_MESSAGE_RECORDER
	virtual bool replaySystemCallbacks();
#endif
	

	bool LockDeletion;
};


} // SIPNET


#endif // __CALLBACK_CLIENT_H__

/* End of callback_client.h */
