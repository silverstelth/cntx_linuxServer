/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MESSAGE_RECORDER_H__
#define __MESSAGE_RECORDER_H__

#include "misc/types_sip.h"
#include "buf_net_base.h"
//#include "callback_net_base.h"
#include "message.h"
#include "misc/time_sip.h"
#include "misc/mem_stream.h"

#include <fstream>
#include <queue>
#include <string>

using namespace std;


namespace SIPNET {


class CInetAddress;

/// Type of network events (if changed, don't forget to change EventToString() and StringToEvent()
enum TNetworkEvent { Sending, Receiving, Connecting, ConnFailing, Accepting, Disconnecting, Error };


/// TNetworkEvent -> string
string EventToString( TNetworkEvent e );

/// string -> TNetworkEvent
TNetworkEvent StringToEvent( string& s );


/*
 * TMessageRecord 
 */
struct TMessageRecord
{
	/// Default constructor
	TMessageRecord( bool input = false ) : UpdateCounter(0), SockId(InvalidSockId), Message( M_SYS_EMPTY, NON_CRYPT, input ) {}

	/// Alt. constructor
	TMessageRecord( TNetworkEvent event, TTcpSockId sockid, CMessage& msg, sint64 updatecounter ) :
		UpdateCounter(updatecounter), Event(event), SockId(sockid), Message(msg) {}

	/// Serial to string stream
	void serial( SIPBASE::CMemStream& stream )
	{
		sipassert( stream.stringMode() );
		
		uint32 len;
		string s_event;
		stream.serial( UpdateCounter );
		if ( stream.isReading() )
		{
			stream.serial( s_event );
			Event = StringToEvent( s_event );
			stream.serialHex( (uint32&)SockId );
			stream.serial( len );
			stream.serialBuffer( Message.bufferToFill( len ), len );
		}
		else
		{
			s_event = EventToString( Event );
			stream.serial( s_event );
			stream.serialHex( (uint32&)SockId );
			len = Message.length();
			stream.serial( len );
			stream.serialBuffer( const_cast<uint8*>(Message.buffer()), len ); // assumes the message contains plain text
		}
	}

	//SIPBASE::TTime		Time;
	sint64				UpdateCounter;
	TNetworkEvent		Event;
	TTcpSockId				SockId;
	CMessage			Message;
};



/**
 * Message recorder.
 * The service performs sends normally. They are intercepted and the recorder
 * plays the receives back. No communication with other hosts.
 * Warning: it works only with messages as plain text.
 * \date 2001
 */
class CMessageRecorder
{
public:

	/// Constructor
	CMessageRecorder();

	/// Destructor
	~CMessageRecorder();

	/// Start recording
	bool	startRecord( const std::string& filename, bool recordall=true );

	/// Add a record
	void	recordNext( sint64 updatecounter, TNetworkEvent event, TTcpSockId sockid, CMessage& message );

	/// Stop recording
	void	stopRecord();

	/// Start replaying
	bool	startReplay( const std::string& filename );

	/// Push the received blocks for this counter into the receive queue
	void	replayNextDataAvailable( sint64 updatecounter );

	/**
	 * Returns the event type if the counter of the next event is updatecounter,
	 * and skip it; otherwise return Error.
	 */
	TNetworkEvent	checkNextOne( sint64 updatecounter );

	/// Get the first stored connection attempt corresponding to addr
	TNetworkEvent	replayConnectionAttempt( const CInetAddress& addr );

	/// Stop playback
	void	stopReplay();

	/// Receive queue (corresponding to one update count). Use empty(), front(), pop().
	std::queue<SIPNET::TMessageRecord>	ReceivedMessages;

protected:

	/// Load the next record from the file (throws EStreamOverflow)
	bool	loadNext( TMessageRecord& record );

	/// Get the next record (from the preloaded records, or from the file)
	bool	getNext( TMessageRecord& record, sint64 updatecounter );

private:

	// Input/output file
	std::fstream								_File;

	// Filename
	std::string									_Filename;

	// Preloaded records
	std::deque<TMessageRecord>					_PreloadedRecords;

	// Connection attempts
	std::deque<TMessageRecord>					_ConnectionAttempts;

	// If true, record all events including sends
	bool										_RecordAll;
};


} // SIPNET


#endif // __MESSAGE_RECORDER_H__

/* End of message_recorder.h */
