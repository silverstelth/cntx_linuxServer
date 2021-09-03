/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "misc/hierarchical_timer.h"

#include "net/buf_sock.h"
#include "net/callback_net_base.h"

#ifdef USE_MESSAGE_RECORDER
#ifdef SIP_OS_WINDOWS
#pragma message ( "SIP Net layer 3: message recorder enabled" )
#endif // SIP_OS_WINDOWS
#include "net/message_recorder.h"
#else
#ifdef SIP_OS_WINDOWS
#pragma message ( "SIP Net layer 3: message recorder disabled" )
#endif // SIP_OS_WINDOWS
#endif


using namespace std;
using namespace SIPBASE;

CVariable<int>	Timeout("net", "Timeout", "Login Timeout(ms)", TIME_INFINITE, 0, true);
CVariable<int>	SleepTime("net", "SleepTime", "Login SleepTime(ms) in Self Update Loop", 5, 0, true);
CVariable<int>	AuthorizeTime("net", "AuthorizeTime", "Authorize Callback ValidTime(ms) in Self Update Loop", 20000, 0, true);
CVariable<bool> LogRecvPacketFlag("net","LogRecvPacketFlag", "Flag of received packet from client", 0, 0, true);


namespace SIPNET {

void	CCallbackNetBase::setDefaultCallback(TMsgCallback defaultCallback)
{
	_DefaultCallback = defaultCallback;
}

/*
 * Constructor
 */
CCallbackNetBase::CCallbackNetBase(  TRecordingState rec, const string& recfilename, bool recordall ) 
	:	_FirstUpdate (true), 
		_UserData(NULL),
		_DefaultCallback(NULL),
		_PreDispatchCallback(NULL),
		_LogRecvPacketFlag(false),
		_MaxTimeOfCallbackProc(0),
		_ProcMessageTotalNum(0),
		_CalcableMessageNum(0),
		_CalcableMessageTime(0),
		_ProcMessageNumPerMS(1000)

#ifdef USE_MESSAGE_RECORDER
		, _MR_RecordingState(rec), _MR_UpdateCounter(0)
#endif
{
	_ThreadId = getThreadId ();

	_BytesSent = 0;
	_BytesReceived = 0;

	createDebug(); // for addNegativeFilter to work even in release and releasedebug modes

#ifdef USE_MESSAGE_RECORDER
	switch ( _MR_RecordingState )
	{
	case Record :
		_MR_Recorder.startRecord( recfilename, recordall );
		break;
	case Replay :
		_MR_Recorder.startReplay( recfilename );
		break;
	default:;
		// No recording
	}
#endif
}

/** Set the user data */
void CCallbackNetBase::setUserData(void *userData)
{
	_UserData = userData;
}

/** Get the user data */
void *CCallbackNetBase::getUserData()
{
	return _UserData;
}

/*
 *	Append callback array with the specified array
 */
void CCallbackNetBase::addCallbackArray (const TCallbackItem *callbackarray, sint arraysize)
{
	checkThreadId ();

	if (arraysize == 1 && callbackarray[0].Callback == NULL && M_SYS_EMPTY == callbackarray[0].Key)
	{
		// it's an empty array, ignore it
		return;
	}

	for (sint i = 0; i < arraysize; i++)
	{
		MsgNameType	strMes = callbackarray[i].Key;
		_CallbackArray.insert( make_pair(strMes, callbackarray[i]) );
	}
/*
	// resize the array
	sint oldsize = _CallbackArray.size();
	_CallbackArray.resize (oldsize + arraysize);
	for (sint i = 0; i < arraysize; i++)
	{
		sint ni = oldsize + i;
		_CallbackArray[ni] = callbackarray[i];

	}
	*/
}


uint32 g_DelayTestPacket = 0;
int g_DelayTestSecond = 0;
TTime g_DelayTestNextTime = 0;
CMessage g_DelayTestMsg;
TTcpSockId g_DelayTestSid;
/*
 * processOneMessage()
 */
void CCallbackNetBase::processOneMessage ()
{
	checkThreadId ();

	// slow down the layer H_AUTO (CCallbackNetBase_processOneMessage);

	CMessage msgin (M_SYS_EMPTY, NON_CRYPT, true);
	TTcpSockId tsid;
	receive (msgin, &tsid);

	if ((g_DelayTestPacket == 0) || (msgin.getName() != g_DelayTestPacket))
	{
		processOneMessage(msgin, tsid);
	}
	else
	{
		sipwarning("Start Delay. PacketID=%d, DelaySecond=%d", msgin.getName(), g_DelayTestSecond);

		g_DelayTestMsg = msgin;
		g_DelayTestSid = tsid;

		g_DelayTestNextTime = CTime::getLocalTime() + g_DelayTestSecond * 1000;

		g_DelayTestPacket = 0;
	}
}

void CCallbackNetBase::processOneMessage (CMessage &msgin, TTcpSockId &tsid)
{
	// now, we have to call the good callback
	MsgNameType name = msgin.getName ();

	if (name == M_SYS_EMPTY)
		return;

	_BytesReceived += msgin.length ();

	map<MsgNameType, TCallbackItem>::iterator callbackfind;
	callbackfind = _CallbackArray.find(name);
	bool	bFind = true;
	if (callbackfind == _CallbackArray.end())
		bFind = false;

	TMsgCallback	cb = NULL;
	if (!bFind)
	{
		if (_DefaultCallback == NULL)
		{
			sipwarning ("LNETL3NB_CB: Callback %s not found in _CallbackArray", msgin.toString().c_str());
		}
		else
		{
			cb = _DefaultCallback;
		}
	}
	else
	{
		cb = callbackfind->second.Callback;
	}

	TTcpSockId realid = getTcpSockId (tsid);

//	if (!realid->AuthorizedCallback.empty() && msgin.getName() != realid->AuthorizedCallback)
	if ((realid->AuthorizedCallback != M_SYS_EMPTY) && 
		(msgin.getName() != realid->AuthorizedCallback) && 
		((realid->AuthorizedCallback != M_SS_AUTH) || (msgin.getName() != M_SS_UPDATE)) &&
		(msgin.getName() != M_CYCLEPING))
	{
#ifdef SIP_MSG_NAME_DWORD
		sipwarning ("LNETL3NB_CB: %s try to call the callback %s but only %d is authorized. Disconnect him!", tsid->asString().c_str(), msgin.toString().c_str(), tsid->AuthorizedCallback);
#else
		sipwarning ("LNETL3NB_CB: %s try to call the callback %s but only %s is authorized. Disconnect him!", tsid->asString().c_str(), msgin.toString().c_str(), tsid->AuthorizedCallback.c_str());
#endif // SIP_MSG_NAME_DWORD
		forceClose(tsid);
		//disconnect(tsid);
	}
	else if (cb == NULL)
	{
		noCallback (msgin.toString(), tsid);
	}
	else
	{
		MsgNameType strMesName = msgin.getName();
		// remove authorizedCallback;
		realid->AuthorizedCallback = M_SYS_EMPTY;
		realid->AuthorizedStartTime = CTime::getLocalTime();
		realid->AuthorizedDuring = 0;

		TTime procbefore = CTime::getLocalTime();
		if (_PreDispatchCallback != NULL)
		{
			// call the pre dispatch callback
			_PreDispatchCallback(msgin, realid, *this);
		}
		if (LogRecvPacketFlag)
		{
			// if (_LogRecvPacketFlag)
#ifdef SIP_MSG_NAME_DWORD
			sipdebug ("############################# %d ###(Calling callback), socket %p", strMesName, getTcpSockId(tsid));
#else
			sipdebug ("############################# %s ###(Calling callback), socket %p", strMesName.c_str(), getTcpSockId(tsid));
#endif // SIP_MSG_NAME_DWORD
		}
		cb(msgin, realid, *this);
		TTime procafter = CTime::getLocalTime();

		TTime proctime = procafter - procbefore;
		if (proctime > _MaxTimeOfCallbackProc)
		{
			_MaxTimeOfCallbackProc = (uint32)proctime;
			_MaxTimePacket = strMesName;
		}
	}
	
/*
	if (pos < 0 || pos >= (sint16) _CallbackArray.size ())
	{
		if (_DefaultCallback == NULL)
			sipwarning ("LNETL3NB_CB: Callback %s not found in _CallbackArray", msgin.toString().c_str());
		else
		{
			// ...
		}
	}
	else
	{
		TTcpSockId realid = getTcpSockId (tsid);

		if (!realid->AuthorizedCallback.empty() && msgin.getName() != realid->AuthorizedCallback)
		{
			sipwarning ("LNETL3NB_CB: %s try to call the callback %s but only %s is authorized. Disconnect him!", tsid->asString().c_str(), msgin.toString().c_str(), tsid->AuthorizedCallback.c_str());
			disconnect (tsid);
		}
		else if (_CallbackArray[pos].Callback == NULL)
		{
			sipwarning ("LNETL3NB_CB: Callback %s is NULL, can't call it", msgin.toString().c_str());
		}
		else
		{
			sipdebug ("LNETL3NB_CB: Calling callback (%s)", _CallbackArray[pos].Key);
			_CallbackArray[pos].Callback (msgin, realid, *this);
		}
	}
*/
}


/*
 * baseUpdate
 * Recorded : YES
 * Replayed : YES
 */
void CCallbackNetBase::baseUpdate (sint32 timeout)
{
	H_AUTO(L3UpdateCallbackNetBase);
	checkThreadId ();
#ifdef SIP_DEBUG
	sipassert( timeout >= -1 );
#endif

	TTime t0 = CTime::getLocalTime();

	//
	// The first time, we init time counters
	//
	if (_FirstUpdate)
	{
//		sipdebug("LNETL3NB: First update()");
		_FirstUpdate = false;
		_LastUpdateTime = t0;
		_LastMovedStringArray = t0;
	}

	//
	// timeout -1    =>  read one message in the queue or nothing if no message in queue
	// timeout 0     =>  read all messages in the queue
	// timeout other =>  read all messages in the queue until timeout expired (at least all one time)
	//

	if (g_DelayTestNextTime == 0)
	{
		bool	exit = false;
		uint32	nMessage = 0;

		while ( ! exit )
		{
			bool	bDataAvailable = false;
			// process all messages in the queue
			while (dataAvailable ())
			{
				bDataAvailable = true;
				processOneMessage ();
				nMessage ++;
				if ( timeout == -1 )
				{
					exit = true;
					break;
				}
			}

			// need to exit?
			if ( timeout == 0 || (sint32)(CTime::getLocalTime() - t0) > timeout || ! bDataAvailable )
			{
				exit = true;
				break;
			}
			else
			{
				// enable multi threading on windows :-/
				// slow down the layer H_AUTO (CCallbackNetBase_baseUpdate_sipSleep);
				sipSleep (1);
			}
		}
	}
	else
	{
		if (t0 >= g_DelayTestNextTime)
		{
			sipwarning("End Delay. PacketID=%d", g_DelayTestMsg.getName());

			g_DelayTestNextTime = 0;

			processOneMessage(g_DelayTestMsg, g_DelayTestSid);
		}
	}

	updatePlugin();

#ifdef USE_MESSAGE_RECORDER
	_MR_UpdateCounter++;
#endif

}
/* Original
void CCallbackNetBase::baseUpdate (sint32 timeout)
{
	H_AUTO(L3UpdateCallbackNetBase);
	checkThreadId ();
#ifdef SIP_DEBUG
	sipassert( timeout >= -1 );
#endif

	TTime t0 = CTime::getLocalTime();

	//
	// The first time, we init time counters
	//
	if (_FirstUpdate)
	{
//		sipdebug("LNETL3NB: First update()");
		_FirstUpdate = false;
		_LastUpdateTime = t0;
		_LastMovedStringArray = t0;
	}

	//
	// timeout -1    =>  read one message in the queue or nothing if no message in queue
	// timeout 0     =>  read all messages in the queue
	// timeout other =>  read all messages in the queue until timeout expired (at least all one time)
	//

	bool	exit = false;
	uint32	nMessage = 0;

	while ( ! exit )
	{
		bool	bDataAvailable = false;
		// process all messages in the queue
		while (dataAvailable ())
		{
			bDataAvailable = true;
			processOneMessage ();
			nMessage ++;
			if ( timeout == -1 )
			{
				exit = true;
				break;
			}
		}

		// need to exit?
		if ( timeout == 0 || (sint32)(CTime::getLocalTime() - t0) > timeout || ! bDataAvailable )
		{
			exit = true;
			break;
		}
		else
		{
			// enable multi threading on windows :-/
			// slow down the layer H_AUTO (CCallbackNetBase_baseUpdate_sipSleep);
			sipSleep (1);
		}
	}
	updatePlugin();

#ifdef USE_MESSAGE_RECORDER
	_MR_UpdateCounter++;
#endif

}
*/

/*
 * baseUpdate
 * Recorded : YES
 * Replayed : YES
 */
uint32 CCallbackNetBase::baseUpdate2 (sint32 timeout, sint32 mintime)
{
	H_AUTO(L3UpdateCallbackNetBase2);
	checkThreadId ();
#ifdef SIP_DEBUG
	sipassert( timeout >= -1 );
#endif

	TTime t0 = CTime::getLocalTime();
	uint32	nMessage = 0;
	//
	// The first time, we init time counters
	//
	if (_FirstUpdate)
	{
//		sipdebug("LNETL3NB: First update()");
		_FirstUpdate = false;
		_LastUpdateTime = t0;
		_LastMovedStringArray = t0;
	}

	/*
	 * Controlling the blocking time of this update loop:
	 *
	 * "GREEDY" MODE (legacy default mode)
	 *   timeout -1   => While data are available, read all messages in the queue,
	 *   mintime t>=0    return only when the queue is empty (and mintime is reached/exceeded).
	 *
	 * "CONSTRAINED" MODE (used by layer 5 with mintime=0)
	 *   timeout t>0  => Read messages in the queue, exit when the timeout is reached/exceeded,
	 *   mintime t>=0    or when there are no more data (and mintime is reached/exceeded).
	 *
	 * "ONE-SHOT"/"HARE AND TORTOISE" MODE
	 *   timeout 0    => Read up to one message in the queue (nothing if empty), then return at
	 *   mintime t>=0    once, or, if mintime not reached, sleep (yield cpu) and start again.
	 *
	 * Backward compatibility:		baseUpdate():		To have the same behaviour with baseUpdate2():
	 *   Warning! The semantics		timeout t>0			timeout 0, mintime t>0
	 *   of the timeout argument	timeout -1			timeout 0, mintime 0
	 *   has changed				timeout 0			timeout -1, mintime 0
	 *
	 * About "Reached/exceeded"
	 *   This loop does not control the time of the user-code in the callback. Thus if some data
	 *   are available, the callback may take more time than specified in timeout. Then the loop
	 *   will end when the timeout is "exceeded" instead of "reached". When yielding cpu, some
	 *   more time than specified may be spent as well.
	 *   
	 * Flooding Detection Option (TODO)
	 *   _FloodingByteLimit => If setFloodingDetection() has been called, and the size of the
	 *                    queue exceeds the flooding limit, the connection will be dropped and
	 *                    the loop will return immediately.
	 *
	 * Message Frame Option (TODO)
 	 *   At the beginning of the loop, the number of pending messages would be read, and then
	 *   only these messages would be processed in this loop, no more. As a result, any messages
	 *   received in the meantime would be postponed until the next call.
	 *   However, to do this we need to add a fast method getMsgNb() in SIPBASE::CBufFifo
	 *   (see more information in the class header of CBufFifo).
	 *
	 * Implementation notes:
	 *   As CTime::getLocalTime() may be slow on certain platforms, we test it only
	 *   if timeout > 0 or mintime > 0.
	 *   As CTime::getLocalTime() is not an accurate time measure (ms unit, resolution on several
	 *   ms on certain platforms), we compare with timeout & mintime using "greater or equal".
	 *
	 * Testing:
	 *  See sip\tools\unit_test\net_ut\layer3_test.cpp
	 */

//	// TODO: Flooding Detection Option (would work best with the Message Frame Option, otherwise
//	// we won't detect here a flooding that would occur during the loop.
//	if ( _FloodingDetectionEnabled )
//	{
//		if ( getDataAvailableFlagV() )
//		{
//			uint64 nbBytesToHandle = getReceiveQueueSize(); // see above about a possible getMsgNb()
//			if ( nbBytesToHandle > _FloodingByteLimit )
//			{
//				// TODO: disconnect
//			}
//		}
//	}

	// Outer loop
	while ( true )
	{
		// Inner loop
		TTicks timeBefore = SIPBASE::CTime::getPerformanceTime();
		uint32	nPrevProcMesNum = nMessage;
		while ( dataAvailable () )
		{
			processOneMessage();
			nMessage ++;

			// ONE-SHOT MODE/"HARE AND TORTOISE" (or CONSTRAINED with no more time): break
			if ( timeout == 0 )
				break;
			// CONTRAINED MODE: break if timeout reached even if more data are available
			if ( (timeout > 0) && ((sint32)(CTime::getLocalTime() - t0) >= timeout) )
			{
				break;
			}
			// GREEDY MODE (timeout -1): loop until no more data are available
		}
		TTicks	timeAfter = SIPBASE::CTime::getPerformanceTime();
		TTime	timeProc = static_cast<TTime>(CTime::ticksToSecond(timeAfter - timeBefore) * 1000000);
		if (nMessage > nPrevProcMesNum && timeProc > 0)
		{
			_CalcableMessageNum += (nMessage - nPrevProcMesNum);
			_CalcableMessageTime += timeProc;
			if (_CalcableMessageTime > 0)
				_ProcMessageNumPerMS = static_cast<uint32>(((double)_CalcableMessageNum / _CalcableMessageTime) * 1000);
		}
		_ProcMessageTotalNum += nMessage;

		// sipSleep( 1 ); // yield cpu
		// If mintime provided, loop until mintime reached, otherwise exit
		if ( mintime == 0 )
			break;
		if (((sint32)(CTime::getLocalTime() - t0) >= mintime))
			break;
	}
	updatePlugin();
	
#ifdef USE_MESSAGE_RECORDER
	_MR_UpdateCounter++;
#endif
	return	nMessage;
}


const	CInetAddress& CCallbackNetBase::hostAddress (TTcpSockId hostid)
{
	// should never be called
	sipstop;
	static CInetAddress tmp;
	return tmp;
}

void	CCallbackNetBase::authorizeOnly (const MsgNameType &callbackName, TTcpSockId hostid, SIPBASE::TTime during)
{
	checkThreadId ();

#ifdef SIP_MSG_NAME_DWORD
	sipassert(callbackName != 0);
	sipdebug ("LNETL3NB: authorizeOnly (%d, %s)", callbackName, hostid->asString().c_str());
#else
	sipdebug ("LNETL3NB: authorizeOnly (%s, %s)", callbackName.c_str(), hostid->asString().c_str());
#endif // SIP_MSG_NAME_DWORD

	hostid = getTcpSockId (hostid);
	
	sipassert (hostid != InvalidSockId);

//	hostid->AuthorizedCallback = (callbackName == NULL) ? M_SYS_EMPTY : callbackName;
	hostid->AuthorizedCallback = callbackName;
	hostid->AuthorizedStartTime = CTime::getLocalTime();
	hostid->AuthorizedDuring = during;
}


#ifdef USE_MESSAGE_RECORDER

/*
 * Replay dataAvailable() in replay mode
 */
bool CCallbackNetBase::replayDataAvailable()
{
	sipassert( _MR_RecordingState == Replay );

	if ( _MR_Recorder.ReceivedMessages.empty() )
	{
		// Fill the queue of received messages related to the present update
		_MR_Recorder.replayNextDataAvailable( _MR_UpdateCounter );
	}

	return replaySystemCallbacks();
}


/*
 * Record or replay disconnection
 */
void CCallbackNetBase::noticeDisconnection( TTcpSockId hostid )
{
	sipassert (hostid != InvalidSockId);	// invalid hostid
	if ( _MR_RecordingState != Replay )
	{
		if ( _MR_RecordingState == Record )
		{
			// Record disconnection
			CMessage emptymsg;
			_MR_Recorder.recordNext( _MR_UpdateCounter, Disconnecting, hostid, emptymsg );
		}
	}
	else
	{
		// Replay disconnection
		hostid->disconnect( false );
	}
}

#endif // USE_MESSAGE_RECORDER

void	CCallbackNetBase::NotifyDestroyToPlugin()
{
	map<string, ICallbackNetPlugin*>::iterator it;
	for (it = m_mapPlugin.begin(); it != m_mapPlugin.end(); it++)
	{
		ICallbackNetPlugin* pPlugin = it->second;
		pPlugin->DestroyCallback();
	}
}

void	CCallbackNetBase::NotifyDisconnectionToPlugin(TTcpSockId from)
{
	map<string, ICallbackNetPlugin*>::iterator it;
	for (it = m_mapPlugin.begin(); it != m_mapPlugin.end(); it++)
	{
		ICallbackNetPlugin* pPlugin = it->second;
		pPlugin->DisconnectionCallback(from);
	}
}

void	CCallbackNetBase::updatePlugin()
{
	std::list<std::string>::iterator itRemoved;
	for (itRemoved = m_lstRemovedPlugin.begin(); itRemoved != m_lstRemovedPlugin.end(); itRemoved++)
	{
		m_mapPlugin.erase((*itRemoved));
	}
	m_lstRemovedPlugin.clear();

	map<string, ICallbackNetPlugin*>::iterator it;
	for (it = m_mapPlugin.begin(); it != m_mapPlugin.end(); it++)
	{
		ICallbackNetPlugin* pPlugin = it->second;
		pPlugin->Update();
	}
}

void	CCallbackNetBase::addPlugin(string strName, ICallbackNetPlugin* pPLugin)
{
	std::list<std::string>::iterator itRemoved;
	for (itRemoved = m_lstRemovedPlugin.begin(); itRemoved != m_lstRemovedPlugin.end(); itRemoved++)
	{
		m_mapPlugin.erase((*itRemoved));
	}
	m_lstRemovedPlugin.clear();
	m_mapPlugin.insert(make_pair(strName, pPLugin));
}

void	CCallbackNetBase::removePlugin(string strName)
{
	m_lstRemovedPlugin.push_back(strName);
}

ICallbackNetPlugin* CCallbackNetBase::findPlugin(std::string strName)
{
	map<string, ICallbackNetPlugin*>::iterator it = m_mapPlugin.find(strName);
	if (it == m_mapPlugin.end())
		return NULL;
	return it->second;
}


/*
 * checkThreadId
 */
void CCallbackNetBase::checkThreadId () const
{
/*	some people use this class in different thread but with a mutex to be sure to have
	no concurent access
	sipdebug("CallbackClient thread (%d) and Current Thread (%d)", _ThreadId, getThreadId());
	if (getThreadId () != _ThreadId)
	{
		//siperror ("You try to access to the same CCallbackClient or CCallbackServer with 2 differents thread (%d and %d)", _ThreadId, getThreadId());
	}
*/
}

SIPBASE_CATEGORISED_COMMAND(sip, resetCallbackProcTime, "Reset Callback proc time calc", "")
{
	return true;
}

} // SIPNET

