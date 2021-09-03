/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/message_recorder.h"
#include "net/inet_address.h"

using namespace SIPBASE;
using namespace std;


namespace SIPNET {


/// TNetworkEvent -> string
string EventToString( TNetworkEvent e )
{
	switch ( e )
	{
	case Sending: return "SEND";
	case Receiving: return "RECV";
	case Connecting: return "CONN";
	case ConnFailing: return "CNFL";
	case Accepting: return "ACCP";
	case Disconnecting: return "DISC";
	default: sipstop; return "-ERR-";
	}
}

/// string -> TNetworkEvent
TNetworkEvent StringToEvent( string& s )
{
	if ( s == "RECV" )
		return Receiving;
	else if ( s == "SEND" )
		return Sending;
	else if ( s == "DISC" )
		return Disconnecting;
	else if ( s == "ACCP" )
		return Accepting;
	else if ( s == "CONN" )
		return Connecting;
	else if ( s == "CNFL" )
		return ConnFailing;
	else 
	{
		sipstop;
		return Error;
	}
}


/*
 * Constructor
 */
CMessageRecorder::CMessageRecorder() : _RecordAll(true)
{
#ifndef MESSAGES_PLAIN_TEXT
	siperror( "The message recorder works only with plain text messages. Please #define MESSAGES_PLAIN_TEXT" );
#endif
}


/*
 * Destructor
 */
CMessageRecorder::~CMessageRecorder()
{
	if ( _Filename != "" )
	{
		sipdebug( "MR:%s: End of recording", _Filename.c_str() );
	}
	stopRecord();
	stopReplay();
}


/*
 * Start recording
 */
bool CMessageRecorder::startRecord( const std::string& filename, bool recordall )
{
	_Filename = filename;
	_File.open( _Filename.c_str(), ios_base::out );
	_File << endl;
	_RecordAll = recordall;
	if ( _File.fail() )
	{
		sipwarning( "MR: Record: Cannot open file %s", _Filename.c_str() );
		return false;
	}
	else
	{
		sipdebug( "MR: Start recording into %s", _Filename.c_str() );
		return true;
	}
}


/*
 * Same as stringFromVector() but assumes the vector contains only printable characters
 */
/*string stringFromTextVector( const vector<uint8>& v )
{
	string s;

	// Copy contents
	s.resize( v.size() );
	memcpy( &*s.begin(), &*v.begin(), v.size() );

	return s;
}*/


/*
 * Add a record
 */
void CMessageRecorder::recordNext( sint64 updatecounter, TNetworkEvent event, TTcpSockId sockid, CMessage& message )
{
	sipassert( _File.is_open() );

	if ( (_RecordAll) || (event != Sending) )
	{
		// Serial to stream
		TMessageRecord rec ( event, sockid, message, updatecounter /*CTime::getLocalTime()*/ );
		CMemStream stream ( false, true );
		rec.serial( stream );
		char c = '\0';      // end of cstring
		stream.serial( c ); // added to the stream for _File << (char*)stream.buffer()

		// Dump to file
		sipdebug( "MR:%s: Recording [%s]", _Filename.c_str(), stream.buffer() );
		int len = (int)(stream.length()-2); // not the null character (and its separator) at the end of the buffer
		_File << "* ";
		_File <<  len; // if we put the expression directly, it makes an access violation ! Weird.
		_File << " ";
		_File << (char*)stream.buffer() << endl; /// \todo cado: Does a silent Access Violation
	}
}


/*
 * Stop recording
 */
void CMessageRecorder::stopRecord()
{
	_File.close();
	_Filename = "";
}


/*
 * Start playback
 */
bool CMessageRecorder::startReplay( const std::string& filename )
{
	_Filename = filename;
	_File.open( _Filename.c_str(), ios_base::in );
	if ( _File.fail() )
	{
		siperror( "MR: Replay: Cannot open file %s", _Filename.c_str() );
		return false;
	}
	else
	{
		sipdebug( "MR: Start replaying from %s", _Filename.c_str() );
		return true;
	}
}


/*
 * Get next record (throw EStreamOverflow)
 */
bool CMessageRecorder::loadNext( TMessageRecord& record )
{
	// WARNING!!! This features doesn't work anymore becaues bufferAsVector() is not available with new CMemStream
	sipstop;
	return false;

	sipassert( _File.is_open() );

	// Dump from file
	CMemStream stream ( true, true );
	uint32 len;
	char c;
	_File >> c; // skip "* ";
	_File >> (int&)len;
	_File.ignore(); // skip delimiter
	if ( ! _File.fail() )
	{
		_File.get( (char*)stream.bufferToFill( len+1 ), len+1, '\0' );
		//stream.bufferAsVector().resize( len ); // cut end of cstring
		sipdebug( "MR:%s: Reading [%s]", _Filename.c_str(), stream.buffer() );

		// Serial from stream
		record.serial( stream ); // may throw EStreamOverflow if _File.fail()
	}

	return ! _File.fail(); // retest
}


/*
 * Get the next record (from the preloaded records, or from the file)
 */
bool CMessageRecorder::getNext( TMessageRecord& record, sint64 updatecounter )
{
	if ( ! _PreloadedRecords.empty() )
	{
		if ( _PreloadedRecords.front().UpdateCounter == updatecounter )
		{
			// The requested record is in the preload
			record = _PreloadedRecords.front();
			_PreloadedRecords.pop_front();
			return true;
		}
		else
		{
			// The requested record is not in the file
			sipassert( updatecounter < _PreloadedRecords.front().UpdateCounter ); // not >
			return false;
		}
	}
	else
	{
		if ( loadNext( record ) )
		{
			if ( record.UpdateCounter == updatecounter )
			{
				// The requested record has been loaded
				return true;
			}
			else
			{
				// The next loaded record is a new one
				sipassert( updatecounter < record.UpdateCounter ); // not >
				_PreloadedRecords.push_back( record ); // when we read one too far
				return false;
			}
		}
		else
		{
			return false;
		}
	}
}


/*
 * Push the received blocks for this counter into the receive queue
 */
void CMessageRecorder::replayNextDataAvailable( sint64 updatecounter )
{
	TMessageRecord rec( true ); // input message

	while ( getNext( rec, updatecounter ) )
	{
		switch ( rec.Event )
		{
		case Receiving :
		case Accepting :
		case Disconnecting :
			ReceivedMessages.push( rec );
			break;

		case Sending :
			break;

		case Connecting :
		case ConnFailing :
			_ConnectionAttempts.push_back( rec );
			break;
			
		default :
			sipstop;
		}
	}
}


/*
 * Returns true and the event type if the counter of the next data is updatecounter
 */
TNetworkEvent CMessageRecorder::checkNextOne( sint64 updatecounter )
{
	TMessageRecord record;
	if ( getNext( record, updatecounter ) )
	{
		sipdebug( "MR: Check next one: %s at update %" SIP_I64 "u", EventToString(record.Event).c_str(), updatecounter );
		return record.Event;
	}
	else
	{
		return Error;
	}
}


/*
 * Get the first stored connection attempt corresponding to addr
 */
TNetworkEvent CMessageRecorder::replayConnectionAttempt( const CInetAddress& addr )
{
	TNetworkEvent event;
	deque<TMessageRecord>::iterator ipr;
	
	if ( ! _ConnectionAttempts.empty() )
	{
		// Search in the already processed connection attempts
		for ( ipr=_ConnectionAttempts.begin(); ipr!=_ConnectionAttempts.end(); ++ipr )
		{
			CInetAddress stored_addr;
			(*ipr).Message.serial( stored_addr );
			if ( stored_addr == addr )
			{
				// Found
				event = (*ipr).Event;
				sipdebug( "MR: Connection attempt found at update %" SIP_I64 "u", (*ipr).UpdateCounter );
				_ConnectionAttempts.erase( ipr );
				return event;
			}
		}
	}
	
	// Seek in the preloaded records
	for ( ipr=_PreloadedRecords.begin(); ipr!=_PreloadedRecords.end(); ++ipr )
	{
		event = (*ipr).Event;
		if ( (event == Connecting) || (event == ConnFailing) )
		{
			CInetAddress stored_addr;
			(*ipr).Message.serial( stored_addr );
			if ( stored_addr == addr )
			{
				// Found
				sipdebug( "MR: Connection attempt found at update %" SIP_I64 "u", (*ipr).UpdateCounter );
				_PreloadedRecords.erase( ipr );
				return event;
			}
		}
	}
	if ( ipr==_PreloadedRecords.end() )
	{
		// If not found, load next records until found !
		TMessageRecord rec( true );
		while ( loadNext( rec ) )
		{
			if ( ( rec.Event == Connecting ) || ( rec.Event == ConnFailing ) )
			{
				CInetAddress stored_addr;
				rec.Message.serial( stored_addr );
				if ( stored_addr == addr )
				{
					// Found
					sipdebug( "MR: Connection attempt found at update %" SIP_I64 "u", rec.UpdateCounter );
					return rec.Event;
				}
				else
				{
					_PreloadedRecords.push_back( rec );
				}
			}
			else
			{
				_PreloadedRecords.push_back( rec );
			}
		}
		// Not found
		sipdebug( "MR: Connection attempt not found" );		
		return Error;
	}
	sipstop;
	return Error;
}


/*
 * Stop playback
 */
void CMessageRecorder::stopReplay()
{
	_File.close();
	_Filename = "";
}


} // SIPNET
