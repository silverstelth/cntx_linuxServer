/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CALLBACK_NET_BASE_H__
#define __CALLBACK_NET_BASE_H__

#undef USE_MESSAGE_RECORDER

#include "misc/types_sip.h"
#include "misc/time_sip.h"
#include "misc/variable.h"

#include "buf_net_base.h"
#include "message.h"
#include "inet_address.h"

#ifdef USE_MESSAGE_RECORDER
#include "message_recorder.h"
#include <queue>
#endif

#include <vector>

#define		TIME_INFINITE			0xffffffff
extern	SIPBASE::CVariable<int>			Timeout;
extern	SIPBASE::CVariable<int>			SleepTime;
extern	SIPBASE::CVariable<int>			AuthorizeTime;

namespace SIPNET {

class CCallbackNetBase;

/** Callback function type for message processing
 *
 * msgin contains parameters of the message
 * from is the SockId of the connection, for a client, from is always the same value
 */
typedef void (*TMsgCallback) (CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase);


/// Callback items. See CMsgSocket::update() for an explanation on how the callbacks are called.
typedef struct
{
	/// Key C string. It is a message type name, or "C" for connection or "D" for disconnection
#ifdef SIP_MSG_NAME_DWORD
	uint32			Key;
#else
	char			*Key;
#endif // SIP_MSG_NAME_DWORD
	/// The callback function
	TMsgCallback	Callback;
} TCallbackItem;

enum EPORT{ PORT_TCP, PORT_UDP };
enum EFILTER{ FILTER_ALL, FILTER_SUBNET, FILTER_CUSTOM, FILTER_NONE };
typedef struct
{
	ucstring		_sPortAlias;
	uint8			_typePort;
	uint32			_uPort;
	uint32			_uFilter;

	void serial (SIPBASE::IStream &s)
	{
		s.serial (_sPortAlias);
		s.serial (_typePort);
		s.serial (_uPort);
		s.serial (_uFilter);
	}
} TRegPortItem;

typedef std::vector<TRegPortItem>	TREGPORTARY;

class ICallbackNetPlugin
{
	friend	class	CCallbackNetBase;
public:
	ICallbackNetPlugin(std::string namePlugin) : m_namePlugin(namePlugin) {}
	std::string		GetPluginName() const	{ return m_namePlugin; }
	
protected:
	virtual		void	Update() = 0;
	virtual		void	DisconnectionCallback(TTcpSockId from) {}
	virtual		void	DestroyCallback() {}
	std::string		m_namePlugin;
};
/**
 * Layer 3
 * \date 2001
 */
class CCallbackNetBase
{
public:

	virtual ~CCallbackNetBase() { NotifyDestroyToPlugin(); }

	/** Set the user data */
	void setUserData(void *userData);

	/** Get the user data */
	void *getUserData();

	/** Sends a message to special connection.
	 * On a client, the hostid isn't used.
	 * On a server, you must provide a hostid. If you hostid = InvalidSockId, the message will be sent to all connected client.
	 */
	virtual void	send (const CMessage &buffer, TTcpSockId hostid = InvalidSockId, uint8 bySendType = REL_GOOD_ORDER) = 0;

	uint64	getBytesSent () { return _BytesSent; }
	uint64	getBytesReceived () { return _BytesReceived; }
	uint32	getMaxTimeOfCallbackProc () { return _MaxTimeOfCallbackProc; }
	MsgNameType	getMaxTimePacket () { return _MaxTimePacket; }

	uint64	getProcMessageTotalNum () { return _ProcMessageTotalNum; }
	uint64	getProcMessageNumPerMS () { return _ProcMessageNumPerMS; }
	
	virtual uint64	getReceiveQueueSize () = 0;
	virtual uint64	getSendQueueSize () = 0;

	virtual void displayReceiveQueueStat (SIPBASE::CLog *log = SIPBASE::InfoLog) = 0;
	virtual void displaySendQueueStat (SIPBASE::CLog *log = SIPBASE::InfoLog, TTcpSockId destid = InvalidSockId) = 0;

	virtual void displayThreadStat (SIPBASE::CLog *log = SIPBASE::InfoLog) = 0;
	
	/** Force to send all data pending in the send queue.
	 * On a client, the hostid isn't used and must be InvalidSockId
	 * On a server, you must provide a hostid.
	 * If you provide a non-null pointer for nbBytesRemaining, the value will be filled*
	 * will the number of bytes that still remain in the sending queue after the
	 * non-blocking flush attempt.
	 */
	virtual bool	flush (TTcpSockId hostid = InvalidSockId, uint *nbBytesRemaining=NULL) = 0;
	
	/**	Appends callback array with the specified array. You can add callback only *after* adding the server or the client.
	 * \param arraysize is the number of callback items.
	 */
	void	addCallbackArray (const TCallbackItem *callbackarray, sint arraysize);

	/// Sets default callback for unknown message types
	void	setDefaultCallback(TMsgCallback defaultCallback);// { _DefaultCallback = defaultCallback; }

	/// Set the pre dispatch callback. This callback is called before each message is dispatched
	void	setPreDispatchCallback(TMsgCallback predispatchCallback) { _PreDispatchCallback = predispatchCallback;}

	/// Sets LogRecvPacketFlag
	void	SetLogRecvPacketFlag(bool b) { _LogRecvPacketFlag = b; }

	/// returns the sockid of a connection. On a server, this function returns the parameter. On a client, it returns the connection.
	virtual TTcpSockId	getTcpSockId (TTcpSockId hostid = InvalidSockId) = 0;

	/** Sets the callback that you want the other side calls. If it didn't call this callback, it will be disconnected
	 * If cb is NULL, we authorize *all* callback.
	 * On a client, the hostid must be InvalidSockId (or ommited).
	 * On a server, you must provide a hostid.
	 */
	void	authorizeOnly (const MsgNameType &callbackName, TTcpSockId hostid = InvalidSockId, SIPBASE::TTime during = 0);

	void	addPlugin(std::string strName, ICallbackNetPlugin* pPLugin);
	void	removePlugin(std::string strName);
	ICallbackNetPlugin* findPlugin(std::string strName);
	void	NotifyDisconnectionToPlugin(TTcpSockId from);
	void	NotifyDestroyToPlugin();

	/// This function is implemented in the client and server class
	virtual bool	dataAvailable () = 0;
	/// This function is implemented in the client and server class
	virtual bool	getDataAvailableFlagV() const = 0;
	/// This function is implemented in the client and server class
	virtual void	update2 ( sint32 timeout=0, sint32 mintime=0, uint32 *RecvTime=NULL, uint32 *SendTime=NULL, uint32 *ProcNum=NULL ) = 0;
	/// This function is implemented in the client and server class (legacy)
	virtual void	update ( sint32 timeout=0 ) = 0;
	/// This function is implemented in the client and server class
	virtual bool	connected () const = 0;
	/// This function is implemented in the client and server class
	virtual void	disconnect (TTcpSockId hostid = InvalidSockId) = 0;
	/// This function is implemented in the client and server class
	virtual void	forceClose (TTcpSockId hostid = InvalidSockId) = 0;

	/// This function is implemented in the client and server class
	virtual void	noCallback (std::string sMes, TTcpSockId tsid) = 0;
	
	/// Returns the address of the specified host
	virtual const	CInetAddress& hostAddress (TTcpSockId hostid);

	// Defined even when USE_MESSAGE_RECORDER is not defined
	enum TRecordingState { Off, Record, Replay };


protected:

	uint64	_BytesSent, _BytesReceived;

	uint64	_CallbackProcCount;
	uint32	_MaxTimeOfCallbackProc;
	MsgNameType	_MaxTimePacket;

	uint64	_ProcMessageTotalNum;

	uint64	_CalcableMessageNum;
	uint64	_CalcableMessageTime;
	uint32	_ProcMessageNumPerMS;
	/// Constructor.
	CCallbackNetBase( TRecordingState rec=Off, const std::string& recfilename="", bool recordall=true );

	/** Used by client and server class
	 * More info about timeout and mintime in the code.
	 */
	uint32 baseUpdate2 ( sint32 timeout=-1, sint32 mintime=0 );

	/// Used by client and server class (legacy)
	void baseUpdate ( sint32 timeout=0 );

	/// Read a message from the network and process it
	void processOneMessage ();
	void processOneMessage (CMessage &msgin, TTcpSockId &tsid);

	/// On this layer, you can't call directly receive, It s the update() function that receive and call your callaback
	virtual void	receive (CMessage &buffer, TTcpSockId *hostid) = 0;

	/// update all plugins
	void updatePlugin();
	// contains callbacks
	std::map<MsgNameType, TCallbackItem>	_CallbackArray;
	
	/// Logging receive packet flag
	bool	_LogRecvPacketFlag;

	// called if the received message is not found in the callback array
	TMsgCallback				_DefaultCallback;

	// If not null, called before each message is dispached to it's callback
	TMsgCallback				_PreDispatchCallback;

	bool _FirstUpdate;

	// ---------------------------------------
#ifdef USE_MESSAGE_RECORDER
	bool			replayDataAvailable();
	virtual bool	replaySystemCallbacks() = 0;
	void			noticeDisconnection( TTcpSockId hostid );

	TRecordingState						_MR_RecordingState;
	sint64								_MR_UpdateCounter;

	CMessageRecorder					_MR_Recorder;
#endif
	std::map<std::string, ICallbackNetPlugin*>		m_mapPlugin;
	std::list<std::string>							m_lstRemovedPlugin;
	
	// ---------------------------------------
private:

	void				*_UserData;

	SIPBASE::TTime		_LastUpdateTime;
	SIPBASE::TTime		_LastMovedStringArray;
	
	friend void cbnbMessageAskAssociations (CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase);
	friend void cbnbMessageRecvAssociations (CMessage &msgin, TTcpSockId from, CCallbackNetBase &netbase);

protected:
	/// \todo ace: debug feature that we should remove one day nefore releasing the game
	uint	_ThreadId;
	void	checkThreadId () const;
};

typedef		std::map<MsgNameType, uint32>			TMapSpecMsgCryptMethod;

} // SIPNET


#endif // __CALLBACK_NET_BASE_H__

/* End of callback_net_base.h */
