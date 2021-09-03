/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "misc/types_sip.h"
#include "net/callback_net_base.h"
#include "net/callback_client.h"

#ifdef USE_MESSAGE_RECORDER
#include "net/message_recorder.h"
#endif

using namespace std;
using namespace SIPBASE;
namespace SIPNET {

/*
 * Disconnection callback
 */

void cbcNewDisconnection (TTcpSockId from, void *data)
{
	sipassert (data != NULL);
	CCallbackClient *client = (CCallbackClient *)data;

	sipdebug("LNETL3C: cbcNewDisconnection()");

#ifdef USE_MESSAGE_RECORDER
	// Record or replay disconnection
	client->noticeDisconnection( from );
#endif

	client->NotifyDisconnectionToPlugin(from);
	// Call the client callback if necessary
	if (client->_DisconnectionCallback != NULL)
		client->_DisconnectionCallback (from, client->_DisconnectionCbArg);

}

void	cbM_KEYEXCHANGE(CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	CCallbackClient * client = (CCallbackClient *)(&clientcb);
	client->cbM_KEYEXCHANGE(msgin, from);
}

void	cbM_CERTIFICATE(CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	CCallbackClient * client = (CCallbackClient *)(&clientcb);
	client->cbM_CERTIFICATE(msgin, from);
}

void	cbM_HANDSHKOK(CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	CCallbackClient * client = (CCallbackClient *)(&clientcb);
	client->cbM_HANDSHKOK(msgin, from);
}

void	cbM_CYCLEPING(CMessage& msgin, TSockId from, CCallbackNetBase& clientcb)
{
	CCallbackClient * client = (CCallbackClient *)(&clientcb);
	client->cbM_CYCLEPING(msgin, from);
}

TCallbackItem	securityCallbackAry[] = 
{
	{ M_KEYEXCHANGE,	cbM_KEYEXCHANGE },
	{ M_CERTIFICATE,	cbM_CERTIFICATE },
	{ M_HANDSHKOK,		cbM_HANDSHKOK },
	{ M_CYCLEPING,		cbM_CYCLEPING },
};
/*
 * Constructor
 */
CCallbackClient::CCallbackClient( bool bHasUdp, bool bNoRecieve, TRecordingState rec, const std::string& recfilename, bool recordall, bool initPipeForDataAvailable ) :
	CCallbackNetBase( rec, recfilename, recordall ), CBufClient( true, rec==Replay, initPipeForDataAvailable, bHasUdp, bNoRecieve ), SendNextValue(0), ReceiveNextValue(0),
	m_bHandShakeOK(false),
	m_nDefaultCryptMethod(0),
	m_nPingCycleMS(0),
	m_tmLastRecvPing(0),
	m_nLastPingData(0),
	_DisconnectionCallback(NULL),
	_DisconnectionCbArg(NULL)
{
	LockDeletion = false;
	CBufClient::setDisconnectionCallback (cbcNewDisconnection, this);
	addCallbackArray(securityCallbackAry, sizeof(securityCallbackAry)/sizeof(securityCallbackAry[0]));
	
	_DefaultCallback = NULL;

	CCryptMethodTypeManager::getInstance().MakeAllCryptMethodObject(m_mapCryptMethod);
}

CCallbackClient::~CCallbackClient()
{
	sipassert(!LockDeletion);

	TMapCryptMethod::iterator it;
	for (it = m_mapCryptMethod.begin(); it != m_mapCryptMethod.end(); it++)
	{
		ICryptMethod* cm = it->second;
		delete	cm;
	}
	m_mapCryptMethod.clear();
}

/*
 * Send a message to the remote host (pushing to its send queue)
 * Recorded : YES
 * Replayed : MAYBE
 */
void	CCallbackClient::send (const CMessage &buffer, TTcpSockId hostid, uint8 bySendType)
{
	sipassert (hostid == InvalidSockId);	// should always be InvalidSockId on client
	checkThreadId ();
	sipassert (connected ());
	sipassert (buffer.length() != 0);
	sipassert (buffer.typeIsSet());

//////////////////////////////////////////////////////////////////////////
	const CMessage	*pFinalBuffer = &buffer;
	CMessage	cryptMsg;
	{
		uint32	nCryptMethod = m_nDefaultCryptMethod;
		TMapSpecMsgCryptMethod::iterator it = m_mapSpecSendMsgCryptMethod.find(buffer.getName());
		if (it != m_mapSpecSendMsgCryptMethod.end())
			nCryptMethod = it->second;
		if (nCryptMethod != 0)
		{
			buffer.makeDoubleTo(cryptMsg);
			
			TMapCryptMethod::iterator it = m_mapCryptMethod.find(nCryptMethod);
			if	(it != m_mapCryptMethod.end())
			{
				uint8*	pOriginal = const_cast<uint8*>(cryptMsg.buffer());
				pOriginal += cryptMsg.getHeaderSize();
				uint32	nOriginalLen = cryptMsg.length() - cryptMsg.getHeaderSize();
				if (nOriginalLen != 0)
				{
					ICryptMethod*	cryptMethod = it->second;
					uint8*	pEncodedData = NULL;
					uint32	nEncodedLen;
					bool	bDelAfterUsing;
					cryptMethod->Encode(pOriginal, nOriginalLen, &pEncodedData, nEncodedLen, bDelAfterUsing);

					if (pOriginal != pEncodedData || nOriginalLen != nEncodedLen)
						siperrornoex("There is mistaked client send crypt method.");
				}
			}
			cryptMsg.setCryptMode(static_cast<TCRYPTMODE>(nCryptMethod));
			pFinalBuffer = &cryptMsg;
		}
	}
//////////////////////////////////////////////////////////////////////////
	_BytesSent += pFinalBuffer->length ();

	// debug features, we number all packet to be sure that they are all sent and received
	// \todo remove this debug feature when ok
	// fill the number
	uint32 *val = (uint32*)pFinalBuffer->buffer ();
#ifdef SIP_BIG_ENDIAN
	*val = SIPBASE_BSWAP32(SendNextValue);
#else
	*val = SendNextValue;
#endif
	SendNextValue++;


#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Send
		CBufClient::send (*pFinalBuffer, bySendType);

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
 * Force to send all data pending in the send queue.
 * Recorded : NO
 * Replayed : NO
 */
bool CCallbackClient::flush (TTcpSockId hostid, uint *nbBytesRemaining) 
{
	sipassert (hostid == InvalidSockId);	// should always be InvalidSockId on client
	checkThreadId ();

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Flush sending (nothing to do in replay mode)
		return CBufClient::flush( nbBytesRemaining );
		
#ifdef USE_MESSAGE_RECORDER
	}
	else
	{
		return true;
	}
#endif
}


/*
 * Updates the network (call this method evenly)
 * Recorded : YES (in baseUpdate())
 * Replayed : YES (in baseUpdate())
 */
void CCallbackClient::update2 ( sint32 timeout, sint32 mintime, uint32 *RecvTime, uint32 *SendTime, uint32 *ProcNum )
{
	LockDeletion = true;
//	sipdebug ("L3: Client: update()");

	H_AUTO(L3UpdateClient2);

	checkThreadId ();

	baseUpdate2 (timeout, mintime); // first receive

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// L1-2 Update (nothing to do in replay mode)
		CBufClient::update (); // then send

#ifdef USE_MESSAGE_RECORDER
	}
#endif
	UpdatePingProc();
	LockDeletion = false;
}


/*
 * Updates the network (call this method evenly) (legacy)
 * Recorded : YES (in baseUpdate())
 * Replayed : YES (in baseUpdate())
 */
void CCallbackClient::update ( sint32 timeout )
{
	LockDeletion = true;
//	sipdebug ("L3: Client: update()");

	H_AUTO(L3UpdateClient);

	checkThreadId ();

	baseUpdate (timeout); // first receive

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// L1-2 Update (nothing to do in replay mode)
		CBufClient::update (); // then send

#ifdef USE_MESSAGE_RECORDER
	}
#endif
	UpdatePingProc();
	
	LockDeletion = false;
}

void	CCallbackClient::UpdatePingProc()
{
	if (m_tmLastRecvPing == 0)
		return;

	if (CTime::getLocalTime() - m_tmLastRecvPing > m_nPingCycleMS &&
		connected())
	{
		CMessage msg(M_CYCLEPING);
		msg.serial(m_nLastPingData);
		send(msg);

		m_tmLastRecvPing = 0;
	}
}

/*
 * Returns true if there are messages to read
 * Recorded : NO
 * Replayed : YES
 */
bool CCallbackClient::dataAvailable ()
{
	checkThreadId ();

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif

		// Real dataAvailable()
		//sipdebug("DATAAVAILABLE socket %s", this->getTcpSockId()->asString().c_str());
        return CBufClient::dataAvailable (); 

#ifdef USE_MESSAGE_RECORDER
	}
	else
	{
		// Simulated dataAvailable()
		return CCallbackNetBase::replayDataAvailable();
	}
#endif
}


/*
 * Read the next message in the receive queue
 * Recorded : YES
 * Replayed : YES
 */
void CCallbackClient::receive (CMessage &buffer, TTcpSockId *hostid)
{
	checkThreadId ();
//	sipassert (connected ());
	*hostid = InvalidSockId;

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
#endif
		
		// Receive
		CBufClient::receive (buffer);

		// debug features, we number all packet to be sure that they are all sent and received
		// \todo remove this debug feature when ok
#ifdef SIP_BIG_ENDIAN
		uint32 val = SIPBASE_BSWAP32(*(uint32*)buffer.buffer ());
#else
		uint32 val = *(uint32*)buffer.buffer ();
#endif

//		sipdebug ("receive message number %u", val);
		if (ReceiveNextValue != val)
		{
//			sipstopex (("LNETL3C: !!!LOST A MESSAGE!!! I received the message number %u but I'm waiting the message number %u (cnx %s), warn online3d@samilpo.com with the log now please", val, ReceiveNextValue, id()->asString().c_str()));
			// resync the message number
			ReceiveNextValue = val;
		}
		ReceiveNextValue++;

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
		_MR_Recorder.ReceivedMessages.pop();
	}
#endif

	buffer.readType ();

	uint32 cryptMethodID = buffer.getCryptMode();
	if ( cryptMethodID != 0 )
	{
		TMapCryptMethod::iterator it = m_mapCryptMethod.find(cryptMethodID);
		if	(it != m_mapCryptMethod.end())
		{
			uint8*	pOriginal = const_cast<uint8*>(buffer.buffer());
			pOriginal += buffer.getHeaderSize();
			uint32	nOriginalLen = buffer.length() - buffer.getHeaderSize();
			if (nOriginalLen > 0)
			{
				ICryptMethod*	cryptMethod = it->second;
				uint8*	pDecodedData = NULL;
				uint32	nDecodedLen;
				bool	bDelAfterUsing;
				cryptMethod->Decode(pOriginal, nOriginalLen, &pDecodedData, nDecodedLen, bDelAfterUsing);

				if (pOriginal != pDecodedData || nOriginalLen != nDecodedLen)
					siperrornoex("There is mistaked client recv crypt method.");
			}
		}
		else
			siperrornoex("There is no client recv crypt method.");
	}
}

/*
 *
 */
TTcpSockId	CCallbackClient::getTcpSockId (TTcpSockId hostid)
{
	sipassert (hostid == InvalidSockId);
	checkThreadId ();

	return id ();
}

/*
 * Connect to the specified host
 * Recorded : YES
 * Replayed : YES
 */
void	CCallbackClient::connect( const CInetAddress& addr, uint32 usec )
{
	checkThreadId ();

	SendNextValue = ReceiveNextValue = 0;

#ifdef USE_MESSAGE_RECORDER
	if ( _MR_RecordingState != Replay )
	{
		try
		{
#endif

			// Connect
			m_bHandShakeOK = false;
			CBufClient::connect( addr, usec );
			while(!m_bHandShakeOK)
			{
				if (dataAvailable ())
					processOneMessage ();
				CBufClient::update ();
				sipSleep(1);
			}
#ifdef USE_MESSAGE_RECORDER
			if ( _MR_RecordingState == Record )
			{
				// Record connection
				CMessage addrmsg;
				addrmsg.serial( const_cast<CInetAddress&>(addr) );
				_MR_Recorder.recordNext( _MR_UpdateCounter, Connecting, _BufSock, addrmsg );
			}
		}
		catch ( ESocketConnectionFailed& )
		{
			if ( _MR_RecordingState == Record )
			{
				// Record connection
				CMessage addrmsg;
				addrmsg.serial( const_cast<CInetAddress&>(addr) );
				_MR_Recorder.recordNext( _MR_UpdateCounter, ConnFailing, _BufSock, addrmsg );
			}
			throw;
		}
	}
	else
	{
		// Check the connection : failure or not
		TNetworkEvent event = _MR_Recorder.replayConnectionAttempt( addr );
		switch ( event )
		{
		case Connecting :
			// Set the remote address
			sipassert( ! _BufSock->Sock->connected() );
			_BufSock->connect( addr, _NoDelay, true );
			_PrevBytesDownloaded = 0;
			_PrevBytesUploaded = 0;
			/*_PrevBytesReceived = 0;
			_PrevBytesSent = 0;*/
			break;
		case ConnFailing :
			throw ESocketConnectionFailed( addr );
			//break;
		default :
			sipwarning( "LNETL3C: No connection event in replay data, at update #%" SIP_I64 "u", _MR_UpdateCounter );
		}
	}
#endif
}

/*
 * Disconnect a connection
 * Recorded : YES
 * Replayed : YES
 */
void CCallbackClient::disconnect( TTcpSockId hostid )
{
	sipassert (hostid == InvalidSockId);	// should always be InvalidSockId on client
	checkThreadId ();

	SendNextValue = ReceiveNextValue = 0;

	// Disconnect only if connected (same as physically connected for the client)
	if ( _BufSock->connectedState() )
	{
		
#ifdef USE_MESSAGE_RECORDER
		if ( _MR_RecordingState != Replay )
		{
#endif
			NotifyDisconnectionToPlugin(hostid);
			// Disconnect
			CBufClient::disconnect ();
#ifdef USE_MESSAGE_RECORDER
		}
		else
		{
			// Read (skip) disconnection in the file
			if ( ! (_MR_Recorder.checkNextOne( _MR_UpdateCounter ) == Disconnecting) )
			{
				sipwarning( "LNETL3C: No disconnection event in the replay data, at update #%" SIP_I64 "u", _MR_UpdateCounter );
			}
		}
		// Record or replay disconnection (because disconnect() in the client does not push a disc. event)
		noticeDisconnection( _BufSock );
#endif
	}
}

void CCallbackClient::forceClose( TTcpSockId hostid )
{
	sipassert (hostid == InvalidSockId);	// should always be InvalidSockId on client
	checkThreadId ();

	SendNextValue = ReceiveNextValue = 0;

	// Disconnect only if connected (same as physically connected for the client)
	if ( _BufSock->connectedState() )
	{

#ifdef USE_MESSAGE_RECORDER
		if ( _MR_RecordingState != Replay )
		{
#endif
			NotifyDisconnectionToPlugin(hostid);
			// Disconnect
			CBufClient::forceClose();
#ifdef USE_MESSAGE_RECORDER
		}
		else
		{
			// Read (skip) disconnection in the file
			if ( ! (_MR_Recorder.checkNextOne( _MR_UpdateCounter ) == Disconnecting) )
			{
				sipwarning( "LNETL3C: No disconnection event in the replay data, at update #%" SIP_I64 "u", _MR_UpdateCounter );
			}
		}
		// Record or replay disconnection (because disconnect() in the client does not push a disc. event)
		noticeDisconnection( _BufSock );
#endif
	}
}

void	CCallbackClient::noCallback (std::string sMes, TTcpSockId tsid)
{
	sipwarning ("LNETL3NB_CB: Callback %s is NULL, can't call it", sMes.c_str());
}

ICryptMethod*	CCallbackClient::GetCryptMethod(uint32 typeID)
{
	TMapCryptMethod::iterator it = m_mapCryptMethod.find(typeID);
	if (it == m_mapCryptMethod.end())
		return NULL;
	return it->second;
}

void	CCallbackClient::cbM_KEYEXCHANGE(CMessage& msgin, TSockId from)
{
	msgin.serial(m_nDefaultCryptMethod, m_nPingCycleMS);
	{
		uint32	mSize;
		msgin.serial(mSize);
		for (uint32 i = 0; i < mSize; i++)
		{
			uint32	cryptMethodID;
			msgin.serial(cryptMethodID);

			ICryptMethod *method = GetCryptMethod(cryptMethodID);
			if (method)
			{
				IKey* key = method->GetCryptKey();
				if (key)
					key->serial(msgin);
			}
		}
	}
	{
		uint32	pSize;
		msgin.serial(pSize);
		for (uint32 i = 0; i < pSize; i++ )
		{
			MsgNameType mName;
			uint32		cm;
			msgin.serial(mName, cm);
			m_mapSpecSendMsgCryptMethod.insert(make_pair(mName, cm));
		}
	}
/*
	while ( (uint32)msgin.getPos() < msgin.length() )
	{
	}
	*/
}

void	CCallbackClient::cbM_CERTIFICATE(CMessage& msgin, TSockId from)
{
	uint32	nMethodID;	msgin.serial(nMethodID);
	uint32	nDataLen;	msgin.serial(nDataLen);
	if (nDataLen == 0)
		return;

	uint8* pData = new uint8[nDataLen];
	msgin.serialBuffer(pData, nDataLen);

	ICryptMethod* method = GetCryptMethod(nMethodID);
	if (method)
	{
		uint8*	pEncodedData = NULL;
		uint32	nEncodedLen;
		bool	bDelAfterUsing;
		method->Encode(pData, nDataLen, &pEncodedData, nEncodedLen, bDelAfterUsing);
		
		CMessage	msg(M_CERTIFICATE);
		msg.serial(nEncodedLen);
		msg.serialBuffer(pEncodedData, nEncodedLen);
		send(msg);
		if (bDelAfterUsing)
			delete [] pEncodedData;
	}
	delete [] pData;
}

void	CCallbackClient::cbM_HANDSHKOK(CMessage& msgin, TSockId from)
{
	m_bHandShakeOK = true;
}

void	CCallbackClient::cbM_CYCLEPING(CMessage& msgin, TSockId from)
{
	msgin.serial(m_nLastPingData);
	m_tmLastRecvPing = CTime::getLocalTime();
}

} // SIPNET
