/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/callback_server.h"

#ifdef USE_MESSAGE_RECORDER
#include "net/dummy_tcp_sock.h"
#endif

using namespace std;
using namespace SIPBASE;

namespace SIPNET {

/*
 * Connection callback
 */
// static void cbsNewConnection (TTcpSockId from, void *data)
void cbsNewConnection (TTcpSockId from, void *data)
{
	sipassert (data != NULL);
	CCallbackServer *server = (CCallbackServer *)data;

	sipdebug("LNETL3S: newConnection()");

#ifdef USE_MESSAGE_RECORDER
	// Record connection
	server->noticeConnection( from );
#endif

	server->NotifyNewConnection(from);
}

/*
 * Disconnection callback
 */
// static void cbsNewDisconnection (TTcpSockId from, void *data)
void cbsNewDisconnection (TTcpSockId from, void *data)
{
	sipassert (data != NULL);
	CCallbackServer *server = (CCallbackServer *)data;

	sipdebug("LNETL3S: cbsNewDisconnection()");

#ifdef USE_MESSAGE_RECORDER
	// Record or replay disconnection
	server->noticeDisconnection( from );
#endif

	server->NotifyDisConnection(from);
}

static void	cbClientCertificate( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	CCallbackServer *server = (CCallbackServer *)&clientcb;
	server->cbClientCertificate(msgin, from);
}
static void	cbClientCyclePing( CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	CCallbackServer *server = (CCallbackServer *)&clientcb;
	server->cbClientCyclePing(msgin, from);
}

static TCallbackItem	securityMsgCallbackArry[] = 
{
	{ M_CERTIFICATE,	cbClientCertificate },
	{ M_CYCLEPING,		cbClientCyclePing },
};

static TSpecMsgCryptRequest	sendCryptMsgArray[] = 
{
	{ M_KEYEXCHANGE,	SCPM_NONE	},
	{ M_CERTIFICATE,	SCPM_NONE	},
	{ M_CYCLEPING,		SCPM_NONE	}
};

static TSpecMsgCryptRequest	recvCryptMsgArray[] = 
{
	{ M_CERTIFICATE,	SCPM_NONE	}
};
/*
 * Constructor
 */
CCallbackServer::CCallbackServer( bool bUdpServer, bool bSecurity ,bool bClientServer, uint32 max_frontnum, TRecordingState rec, const string& recfilename, bool recordall, bool initPipeForDataAvailable ) :
	CCallbackNetBase( rec, recfilename, recordall ),
	CBufServer( bUdpServer, bClientServer, max_frontnum, DEFAULT_STRATEGY, DEFAULT_MAX_THREADS, DEFAULT_MAX_SOCKETS_PER_THREADS, true, rec==Replay, initPipeForDataAvailable ),
	m_security(bSecurity),
	_ConnectionCallback(NULL),
	_ConnectionCbArg(NULL),
	_DisconnectionCallback(NULL),
	_DisconnectionCbArg(NULL),
	_WarningMessageLengthForSend(0)
{
#ifndef USE_MESSAGE_RECORDER
	sipassertex( rec==Off, ("LNETL3S: The message recorder is disabled at compilation time ; switch the recording state Off") );
#endif
	
	CBufServer::setConnectionCallback (cbsNewConnection, this);
	CBufServer::setDisconnectionCallback (cbsNewDisconnection, this);
	
	addCallbackArray(securityMsgCallbackArry, sizeof(securityMsgCallbackArry)/sizeof(securityMsgCallbackArry[0]));
	m_security.AddSendPacketCryptMethod(sendCryptMsgArray, sizeof(sendCryptMsgArray)/sizeof(sendCryptMsgArray[0]));
	m_security.AddRecvPacketCryptMethod(recvCryptMsgArray, sizeof(recvCryptMsgArray)/sizeof(recvCryptMsgArray[0]));
}

void	CCallbackServer::HandShakeOK(TTcpSockId from)
{
	CMessage msgout(M_HANDSHKOK);
	send(msgout, from);

	if (_ConnectionCallback != NULL)
		_ConnectionCallback (from, _ConnectionCbArg);

	if (m_security.IsSecurityProcFlag())
	{
		CMessage	msgCyclePing(M_CYCLEPING);
		m_security.MakeCyclePingMsg(msgCyclePing, from);
		send(msgCyclePing, from);
	}
}

void	CCallbackServer::NotifyNewConnection(TTcpSockId from)
{
	if (m_security.IsSecurityProcFlag())
	{
		m_security.AddNewClient(from);

		CMessage	msgNotifyKey(M_KEYEXCHANGE);
		m_security.MakeNotifyKeyMsg(msgNotifyKey, from);
		send(msgNotifyKey, from);
	}
	
	if (m_security.IsSecurityConnectionProc())
	{
		CMessage	msgCertificate(M_CERTIFICATE);
		m_security.MakeCertificateMsg(msgCertificate, from);
		send(msgCertificate, from);
		authorizeOnly (M_CERTIFICATE, from, 60000);
	}
	else
	{
		HandShakeOK(from);
	}
}

void	CCallbackServer::NotifyDisConnection(TTcpSockId from)
{
// Notify to security part
	if (m_security.IsSecurityProcFlag())
		m_security.DisConnectClient(from);
// Notify to plugins
	NotifyDisconnectionToPlugin(from);
// Notify to function server
	if (_DisconnectionCallback != NULL)
		_DisconnectionCallback (from, _DisconnectionCbArg);
}

void	CCallbackServer::cbClientCertificate( CMessage& msgin, TSockId from )
{
	if (m_security.IsSecurityConnectionProc())
	{
		bool bOK = m_security.ClientCertificate(msgin, from);
		if (!bOK)
		{
			disconnect(from);
			return;
		}
	}
	HandShakeOK(from);
}

void	CCallbackServer::cbClientCyclePing( CMessage& msgin, TSockId from )
{
	if (!m_security.CheckClientCyclePing(msgin, from))
	{
		sipwarning("There is incorrect cycle ping from 0x%x", from);
		disconnect(from);
	}
	else
	{
		CMessage	msgCyclePing(M_CYCLEPING);
		m_security.MakeCyclePingMsg(msgCyclePing, from);
		send(msgCyclePing, from);
	}
}

/*
 * Send a message to the specified host (pushing to its send queue)
 * Recorded : YES
 * Replayed : MAYBE
 */
void	CCallbackServer::send (const CMessage &buffer, TTcpSockId hostid, uint8 bySendType)
{
	checkThreadId ();
	if (_WarningMessageLengthForSend > 0 &&
		buffer.length() > _WarningMessageLengthForSend)
	{
		sipwarning("Packet length is bigger than warning value(%d). packet=%s (This is only WARNING!!!)", _WarningMessageLengthForSend, buffer.getNameAsString().c_str());
	}

	const CMessage	*pFinalBuffer = &buffer;

//////////////////////////////////////////////////////////////////////////
	CMessage	cryptMsg;
	if (m_security.IsCryptPacketProcFlag())
	{
		if (hostid != InvalidSockId)
		{
			buffer.makeDoubleTo(cryptMsg);
			cryptMsg.setCryptMode(static_cast<TCRYPTMODE>(0));
			m_security.EncodeSendPacket(cryptMsg, hostid);
			pFinalBuffer = &cryptMsg;
		}
	}
//////////////////////////////////////////////////////////////////////////
	sipassert (connected ());
	sipassert (pFinalBuffer->length() != 0);
	sipassert (pFinalBuffer->typeIsSet());

	if (hostid == InvalidSockId)
	{
		// broadcast
		sint nb = nbConnections ();
		_BytesSent += pFinalBuffer->length () * nb;
	}
	else
	{
		_BytesSent += pFinalBuffer->length ();
	}


#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Send
		CBufServer::send (*pFinalBuffer, hostid, bySendType);

#ifdef USE_MESSAGE_RECORDER
		if ( _MR_RecordingState == Record )
		{
			// Record sent message
			_MR_Recorder.recordNext( _MR_UpdateCounter, Sending, hostid, const_cast<CMessage&>(*pFinalBuffer) );
		}
	}
	else
	{	
		/// \todo cado: check that the next sending is the same
	}
#endif
}


/*
 * Updates the network (call this method evenly)
 * Recorded : YES (in baseUpdate())
 * Replayed : YES (in baseUpdate())
 */
void CCallbackServer::update2 ( sint32 timeout, sint32 mintime, uint32 *RecvTime, uint32 *SendTime, uint32 *ProcNum )
{
	H_AUTO(L3UpdateServer);

	checkThreadId ();
	sipassert (connected ());

	TTime R0 = CTime::getLocalTime();
	uint32	uMesNum = baseUpdate2 ( timeout, mintime ); // first receive
	TTime R1 = CTime::getLocalTime();
	if (RecvTime != NULL)
		*RecvTime = (uint32)(R1 - R0);
	if (ProcNum != NULL)
		*ProcNum = uMesNum;

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif
		// L1-2 Update (nothing to do in replay mode)
		TTime S0 = CTime::getLocalTime();
		CBufServer::update (); // then send
		TTime S1 = CTime::getLocalTime();
		if (SendTime != NULL)
			*SendTime = (uint32)(S1 - S0);
		
		if (m_security.IsSecurityProcFlag())
			updatePing();

#ifdef USE_MESSAGE_RECORDER
	}
#endif

}

/*
 * Updates the network (call this method evenly) (legacy)
 * Recorded : YES (in baseUpdate())
 * Replayed : YES (in baseUpdate())
 */
void CCallbackServer::update ( sint32 timeout )
{
	H_AUTO(L3UpdateServer);

	checkThreadId ();
	sipassert (connected ());

	//	sipdebug ("LNETL3S: Client: update()");
	baseUpdate ( timeout ); // first receive

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif
		// L1-2 Update (nothing to do in replay mode)
		CBufServer::update (); // then send
		
		if (m_security.IsSecurityProcFlag())
			updatePing();

#ifdef USE_MESSAGE_RECORDER
	}
#endif
}

void CCallbackServer::updatePing()
{
	std::vector<SIPNET::TTcpSockId> uncheckBuffer;
	m_security.UpdatePing(uncheckBuffer);

	std::vector<SIPNET::TTcpSockId>::iterator it;
	for (it = uncheckBuffer.begin(); it != uncheckBuffer.end(); it++)
	{
		disconnect(*it);
	}
}

/*
 * Read the next message in the receive queue
 * Recorded : YES
 * Replayed : YES
 */
void CCallbackServer::receive (CMessage &buffer, TTcpSockId *hostid)
{
	checkThreadId ();
	sipassert (connected ());

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Receive
		CBufServer::receive (buffer, hostid);

#ifdef USE_MESSAGE_RECORDER
		if ( _MR_RecordingState == Record )
		{
			// Record received message
			_MR_Recorder.recordNext( _MR_UpdateCounter, Receiving, *hostid, const_cast<CMessage&>(buffer) );
		}
	}
	else
	{
		// Retrieve received message loaded by dataAvailable()
		buffer = _MR_Recorder.ReceivedMessages.front().Message;
		*hostid = _MR_Recorder.ReceivedMessages.front().SockId;
		_MR_Recorder.ReceivedMessages.pop();
	}
#endif
	
	buffer.readType ();

	if (m_security.IsCryptPacketProcFlag())
	{
		if (!m_security.DecodeRecvPacket(buffer, *hostid))
		{
			sipwarning("There is incorrect crypted message from %p:message=%s", hostid, buffer.getNameAsString().c_str());
			disconnect(*hostid);
		}
	}
}


/*
 * Disconnect a connection
 * Set hostid to InvalidSockId to disconnect all connections.
 * If hostid is not null and the socket is not connected, the method does nothing.
 * Before disconnecting, any pending data is actually sent.
 * Recorded : YES in noticeDisconnection called in the disconnection callback
 * Replayed : YES in noticeDisconnection called in the disconnection callback
 */
void CCallbackServer::disconnect( TTcpSockId hostid )
{
	checkThreadId ();

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif
		// Disconnect
		CBufServer::disconnect( hostid );

#ifdef USE_MESSAGE_RECORDER
	}
	// else, no need to manually replay the disconnection, such as in CCallbackClient,
	// it will be replayed during the next update()
#endif
}

void CCallbackServer::forceClose( TTcpSockId hostid )
{
	checkThreadId ();

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif
		// Disconnect
		CBufServer::forceClose( hostid );

#ifdef USE_MESSAGE_RECORDER
	}
	// else, no need to manually replay the disconnection, such as in CCallbackClient,
	// it will be replayed during the next update()
#endif
}

void	CCallbackServer::noCallback (std::string sMes, TTcpSockId tsid)
{
	sipwarning ("LNETL3S: Callback %s is NULL, can't call it", sMes.c_str());
	disconnect (tsid);
}

/*
 *
 */
TTcpSockId CCallbackServer::getTcpSockId (TTcpSockId hostid)
{
	sipassert (hostid != InvalidSockId);	// invalid hostid
	checkThreadId ();
	sipassert (connected ());
	sipassert (hostid != NULL);
	return hostid;
}


/*
 * Returns true if there are messages to read
 * Recorded : NO
 * Replayed : YES
 */
bool CCallbackServer::dataAvailable ()
{
	checkThreadId ();

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Real dataAvailable()
		return CBufServer::dataAvailable (); 

#ifdef USE_MESSAGE_RECORDER
	}
	else
	{
		// Simulated dataAvailable()
		return CCallbackNetBase::replayDataAvailable();
	}
#endif
}


//-------------------------
#ifdef USE_MESSAGE_RECORDER

  
/*
 * Replay connection and disconnection callbacks, client version
 */
bool CCallbackServer::replaySystemCallbacks()
{
	do
	{
		if ( _MR_Recorder.ReceivedMessages.empty() )
		{
			return false;
		}
		else
		{
			// Translate the stored sockid to the replayed sockid
			TTcpSockId sockid;
			std::map<TTcpSockId,TTcpSockId>::iterator isi = _MR_SockIds.find( _MR_Recorder.ReceivedMessages.front().SockId );
			if ( isi != _MR_SockIds.end() )
			{
				// The sockid is found in the map if the connection already exists
				sockid = (*isi).second;
				_MR_Recorder.ReceivedMessages.front().SockId = sockid;
			}

			// Test event type
			switch( _MR_Recorder.ReceivedMessages.front().Event )
			{
			case Receiving:
				return true;

			case Disconnecting:
				sipdebug( "LNETL3S: Disconnection event for %p", sockid );
				sockid->Sock->disconnect();
				sockid->setConnectedState( false );
	
				// Call callback if needed
				if ( disconnectionCallback() != NULL )
				{
					disconnectionCallback()( sockid, argOfDisconnectionCallback() );
				}
				break;

			case Accepting:
				{
				// Replay connection:

				// Get remote address
				CInetAddress addr;
				_MR_Recorder.ReceivedMessages.front().Message.serial( addr );

				// Create a new connection 
				sockid = new CBufSock( new CDummyTcpSock() );
				sockid->Sock->connect( addr );
				_MR_Connections.push_back( sockid );

				// Bind it to the "old" sockid
				_MR_SockIds.insert( make_pair( _MR_Recorder.ReceivedMessages.front().SockId, sockid ) );

				sipdebug( "LNETL3S: Connection event for %p", sockid );
				sockid->setConnectedState( true );
					
				// Call callback if needed
				if ( connectionCallback() != NULL )
				{
					connectionCallback()( sockid, argOfConnectionCallback() );
				}
				break;
				}
			default:
				siperror( "LNETL3S: Invalid system event type in client receive queue" );
			}
			// Extract system event
			_MR_Recorder.ReceivedMessages.pop();
		}
	}
	while ( true );
}


/*
 * Record or replay connection
 */
void CCallbackServer::noticeConnection( TTcpSockId hostid )
{
	sipassert (hostid != InvalidSockId);	// invalid hostid
	if ( _MR_RecordingState != Replay )
	{
		if ( _MR_RecordingState == Record )
		{
			// Record connection
			CMessage addrmsg;
			addrmsg.serial( const_cast<CInetAddress&>(hostAddress(hostid)) );
			_MR_Recorder.recordNext( _MR_UpdateCounter, Accepting, hostid, addrmsg );
		}
	}
	else
	{
		/// \todo cado: connection stats
	}
}

#endif // USE_MESSAGE_RECORDER


} // SIPNET
