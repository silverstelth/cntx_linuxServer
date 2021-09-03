/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/message.h"
#include "net/CryptEngine.h"

/*#ifdef MESSAGES_PLAIN_TEXT
#pragma message( "CMessage: compiling messages as plain text" )
#else
#pragma message( "CMessage: compiling messages as binary" )
#endif*/

#define		MAX_PACKET_BUF_LEN		4096		// 4K

namespace SIPNET
{

bool CMessage::_DefaultStringMode = false;

const char *LockedSubMessageError = "a sub message is forbidden";

#define FormatLong 1
#define FormatShort 0


/*
 * Constructor by name
 */
/*
CMessage::CMessage (const std::string &name, bool inputStream, TStreamFormat streamformat, uint32 defaultCapacity) :
	SIPBASE::CMemStream (inputStream, false, defaultCapacity),
	_Type(OneWay), _SubMessagePosR(0), _LengthR(0), _HeaderSize(0xFFFFFFFF), _TypeSet (false)
{
	init( name, streamformat );
}
*/

CMessage::CMessage(const MsgNameType &name, TCRYPTMODE mode, bool inputStream, TStreamFormat streamformat, uint32 defaultCapacity) :
	SIPBASE::CMemStream (inputStream, false, defaultCapacity),
	_Type(OneWay), _SubMessagePosR(0), _LengthR(0), _HeaderSize(0xFFFFFFFF), _TypeSet (false)
{
	_CryptMode = mode;

	init( name, streamformat );
}

/*
 * Utility method
 */
void CMessage::init( const MsgNameType &name, TStreamFormat streamformat )
{
	if ( streamformat == UseDefault )
	{
		setStringMode( _DefaultStringMode );
	}
	else
	{
		setStringMode( streamformat == String );
	}

	if (name != M_SYS_EMPTY)
		setType (name);
}


/*
 * Constructor with copy from CMemStream
 */
CMessage::CMessage (SIPBASE::CMemStream &memstr) :
	SIPBASE::CMemStream( memstr ),
	_Type(OneWay), _SubMessagePosR(0), _LengthR(0), _HeaderSize(0xFFFFFFFF), _TypeSet (false)
{
	sint32 pos = getPos();
	bool reading = isReading();
	if ( reading ) // force input mode to read the type
		readType(); // sets _TypeSet, _HeaderSize and _LengthR
	else
		invert(); // calls readType()
	if ( ! reading )
		invert(); // set ouput mode back if necessary
	seek( pos, begin ); // sets the same position as the one in the memstream
}


/*
 * Copy constructor
 */
CMessage::CMessage (const CMessage &other) 
	:	CMemStream(),
		_TypeSet(false)
{
	operator= (other);
}

/*
 * Assignment operator
 */
CMessage &CMessage::operator= (const CMessage &other)
{
//	sipassertex( (!other.isReading()) || (!other.hasLockedSubMessage()), ("Storing %s", LockedSubMessageError) );
	sipassertex( (!isReading()) || (!hasLockedSubMessage()), ("Assigning %s", LockedSubMessageError) );
	if ( other.hasLockedSubMessage() )
	{
		assignFromSubMessage(other);
	}
	else
	{
		
		CMemStream::operator= (other);
		_Type = other._Type;
		_TypeSet = other._TypeSet;
		_Name = other._Name;
		_HeaderSize = other._HeaderSize;
		_SubMessagePosR = other._SubMessagePosR;
		_LengthR = other._LengthR;
		_CryptMode = other._CryptMode;
	}

	return *this;
}

void CMessage::swap(CMessage &other)
{
	sipassert( !hasLockedSubMessage() );
	CMemStream::swap(other);
#ifdef SIP_MSG_NAME_DWORD
	std::swap(_Name, other._Name);
#else
	_Name.swap(other._Name);
#endif // SIP_MSG_NAME_DWORD
	std::swap(_SubMessagePosR, other._SubMessagePosR);
	std::swap(_LengthR, other._LengthR);
	std::swap(_HeaderSize, other._HeaderSize);
	std::swap(_TypeSet, other._TypeSet);
	std::swap(_Type, other._Type);
}

/// Make new copy
void CMessage::makeDoubleTo(CMessage &other) const
{
	other.clear();
	other._Type				= _Type;
	other._TypeSet			= _TypeSet;
	other._Name				= _Name;
	other._HeaderSize		= _HeaderSize;
	other._SubMessagePosR	= _SubMessagePosR;
	other._LengthR			= _LengthR;
	other._CryptMode		= _CryptMode;

	uint8*	pOriginal = const_cast<uint8*>(buffer());
	other.serialBuffer(pOriginal, length());
}


/**
 * Similar to operator=, but makes the current message contain *only* the locked sub message in msgin
 * or the whole msgin if it is not locked
 *
 * Preconditions:
 * - msgin is an input message (isReading())
 * - The current message is blank (new or reset with clear())
 *
 * Postconditions:
 * - If msgin has been locked using lockSubMessage(), the current message contains only the locked
 *   sub message in msgin, otherwise the current message is exactly msgin
 * - The current message is an input message, it is not locked
 */
void CMessage::assignFromSubMessage( const CMessage& msgin )
{
	sipassert( msgin.isReading() );
	sipassert( ! _TypeSet );
	if ( ! isReading() )
		invert();

	if ( msgin.hasLockedSubMessage() )
	{
		fill( msgin.buffer(), msgin._LengthR );
		readType();
		seek( msgin.getPos(), IStream::begin );
	}
	else
	{
		operator=( msgin );
	}
}


/*
 * Sets the message type as a string and put it in the buffer if we are in writing mode
 */
void CMessage::setType (const MsgNameType &name, TMessageType type)
{
	// check if we already do a setType ()
	sipassert (!_TypeSet);
	// don't accept empty string
//	sipassert (!name.empty ());

	_Name = name;
	_Type = type;

	if (!isReading ())
	{
		// check if they don't already serial some stuffs
		sipassert (length () == 0);

		// if we can send the id instead of the string, "just do it" (c)nike!
		//SIPBASE::CStringIdArray::TStringId id = _SIDA->getId (name);

		// Force binary mode for header
		bool msgmode = _StringMode;
		_StringMode = false;

		// debug features, we number all packet to be sure that they are all sent and received
		// \todo remove this debug feature when ok
		// this value will be fill after in the callback function
		uint32 zeroValue = 123;
		serial (zeroValue);


		TFormat format;
		format.LongFormat = FormatLong;
		format.StringMode = msgmode;
		format.MessageType = _Type;
		format.CryptMode = _CryptMode;
		//sipdebug( "OUT format = %hu", (uint16)format );
		serial (format);

		// End of binary header
		_StringMode = msgmode;

#ifdef SIP_MSG_NAME_DWORD
		serial ((uint32&)name);
#else
		serial ((std::string&)name);
#endif // SIP_MSG_NAME_DWORD

		_HeaderSize = getPos ();
	}

	_TypeSet = true;
}


/*
 * Warning: MUST be of the same size than previous name!
 * Output message only.
 */
void CMessage::changeType (const MsgNameType &name)
{
	sint32 prevPos = getPos();
	seek( sizeof(uint32)+sizeof(uint8), begin );
#ifdef SIP_MSG_NAME_DWORD
	serial ((uint32&)name);
#else
	serial ((std::string&)name);
#endif // SIP_MSG_NAME_DWORD

	seek( prevPos, begin );
}


/*
 * Returns the size, in byte of the header that contains the type name of the message or the type number
 */
uint32 CMessage::getHeaderSize () const
{
	sipassert (_HeaderSize != 0xFFFFFFFF);
	sipassert(!hasLockedSubMessage());
	return _HeaderSize;
}


/*
 * The message was filled with an CMemStream, Now, we'll get the message type on this buffer
 */
void CMessage::readType ()
{
	sipassert (isReading ());

	// debug features, we number all packet to be sure that they are all sent and received
	// \todo remove this debug feature when ok

	// we remove the message from the message
	resetSubMessageInternals();
	const uint HeaderSize = 4;
	seek (HeaderSize, begin);
//		uint32 zeroValue;
//		serial (zeroValue);

	// Force binary mode for header
	_StringMode = false;

	TFormat format;
	serial (format);
	//sipdebug( "IN format = %hu", (uint16)format );
	bool LongFormat = format.LongFormat;

	// Set mode for the following of the buffer
	_StringMode = format.StringMode;
	_CryptMode	= (TCRYPTMODE) format.CryptMode;

	MsgNameType name;
	serial (name);
	setType (name, TMessageType(format.MessageType));
	_HeaderSize = getPos();
}


/*
 * Get the message name (input message only) and advance the current pos
 */
MsgNameType CMessage::readTypeAtCurrentPos() const
{
	sipassert( isReading() );

	const uint HeaderSize = 4;
	seek( HeaderSize, current );

	bool sm = _StringMode;
	_StringMode = false;

	TFormat format;
	sipRead(*this, serial, format );
	bool LongFormat = format.LongFormat;
	_StringMode = format.StringMode;
	_Type = TMessageType(format.MessageType);

	if ( LongFormat )
	{
		MsgNameType name;
		sipRead(*this, serial, name );
		_StringMode = sm;
		return name;
	}
	else
		siperror( "Id not supported" );


	_StringMode = sm;
	return M_SYS_EMPTY;
}


// Returns true if the message type was already set
bool CMessage::typeIsSet () const
{
	return _TypeSet;
}

TCRYPTMODE CMessage::getCryptMode() const
{
	return _CryptMode;
}

void CMessage::setCryptMode( TCRYPTMODE mode)
{
	const uint8 *buf = buffer();

	TFormat forma = *(TFormat *)(buf + 4);

	forma.CryptMode = mode;
	*(TFormat *)(buf + 4) = forma;
	_CryptMode = mode;
}

// Clear the message. With this function, you can reuse a message to create another message
void CMessage::clear ()
{
	sipassertex( (!isReading()) || (!hasLockedSubMessage()), ("Clearing %s", LockedSubMessageError) );

	CMemStream::clear ();
	_TypeSet = false;
	_SubMessagePosR = 0;
	_LengthR = 0;
}

/*
 * Returns the type name in string if available. Be sure that the message have the name of the message type
 */
MsgNameType CMessage::getName () const
{
	if ( hasLockedSubMessage() )
	{
		CMessage& notconstMsg = const_cast<CMessage&>(*this);
		sint32 savedPos = notconstMsg.getPos();
		uint32 subPosSaved = _SubMessagePosR;
		uint32 lenthRSaved = _LengthR;
		const_cast<uint32&>(_SubMessagePosR) = 0;
//		const_cast<uint32&>(_LengthR) = _Buffer.size();
		const_cast<uint32&>(_LengthR) = _Buffer.getBuffer().size();
		notconstMsg.seek( subPosSaved, begin ); // not really const... but removing the const from getName() would need too many const changes elsewhere
		MsgNameType name = notconstMsg.readTypeAtCurrentPos();
		notconstMsg.seek( subPosSaved+savedPos, begin );
		const_cast<uint32&>(_SubMessagePosR) = subPosSaved;
		const_cast<uint32&>(_LengthR) = lenthRSaved;
		return name;
	}
	else
	{
		sipassert (_TypeSet);
		return _Name;
	}
}

CMessage::TMessageType CMessage::getType() const
{
	if ( hasLockedSubMessage() )
	{
		CMessage& notconstMsg = const_cast<CMessage&>(*this);
		sint32 savedPos = notconstMsg.getPos();
		uint32 subPosSaved = _SubMessagePosR;
		uint32 lenthRSaved = _LengthR;
		const_cast<uint32&>(_SubMessagePosR) = 0;
//		const_cast<uint32&>(_LengthR) = _Buffer.size();
		const_cast<uint32&>(_LengthR) = _Buffer.getBuffer().size();
		notconstMsg.seek( subPosSaved, begin ); // not really const... but removing the const from getName() would need too many const changes elsewhere
		notconstMsg.readTypeAtCurrentPos();
		notconstMsg.seek( subPosSaved+savedPos, begin );
		const_cast<uint32&>(_SubMessagePosR) = subPosSaved;
		const_cast<uint32&>(_LengthR) = lenthRSaved;
		return _Type;
	}
	else
	{
		sipassert (_TypeSet);
		return _Type;
	}
}


/* Returns a readable string to display it to the screen. It's only for debugging purpose!
 * Don't use it for anything else than to debugging, the string format could change in the future.
 * \param hexFormat If true, display all bytes in hexadecimal
 * \param textFormat If true, display all bytes as chars (above 31, otherwise '.')
 */
std::string CMessage::toString( bool hexFormat, bool textFormat ) const
{
	//sipassert (_TypeSet);
#ifdef SIP_MSG_NAME_DWORD
	std::string s = "(" + SIPBASE::toStringA(_Name) + ")";
#else
	std::string s = "('" + _Name + "')";
#endif // SIP_MSG_NAME_DWORD
	if ( hexFormat )
		s += " " + CMemStream::toString( true );
	if ( textFormat )
		s += " " + CMemStream::toString( false );
	return s;
}

std::string CMessage::getNameAsString() const
{
#ifdef SIP_MSG_NAME_DWORD
	std::string s = SIPBASE::toStringA(_Name);
#else
	std::string s = _Name;
#endif // SIP_MSG_NAME_DWORD
	return s;

}

/*
 * Return an input stream containing the stream beginning in the message at the specified pos
 */
SIPBASE::CMemStream	CMessage::extractStreamFromPos( sint32 pos )
{
	SIPBASE::CMemStream msg( true );
	sint32 len = length() - pos;
	memcpy( msg.bufferToFill( len ), buffer() + pos, len );
	return msg;
}


/*
 * Encapsulate/decapsulate another message inside the current message
 */
void	CMessage::serialMessage( CMessage& msg )
{
	if ( isReading() )
	{
		// Init 'msg' with the contents serialised from 'this'
		uint32 len;
		serial( len );
		if ( ! msg.isReading() )
			msg.invert();
		serialBuffer( msg.bufferToFill( len ), len );
		msg.readType();
		msg.invert();
		msg.seek( 0, CMemStream::end );
	}
	else
	{
		// Store into 'this' the contents of 'msg'
		uint32 len = msg.length();
		serial( len );
		serialBuffer( const_cast<uint8*>(msg.buffer()), msg.length() );
	}
}

void	CMessage::encodeBody(TTcpSockId hostid)
{
	sipassert(!isReading());
	if (_CryptMode == DEFAULT )
	{
		sipwarning("STOP : Message<%s> Crypt Mode is Default, Maybe sended from CallbackServer or CallbackClient", toString().c_str());
		return;
	}

	try
	{
		uint32		inlen	= length() - getHeaderSize();
		//uint32		outlen	= _DefaultCapacity;
		uint32		outlen	= MAX_PACKET_BUF_LEN - getHeaderSize();

		if ( inlen == 0 )
			return;

		autoPtr<unsigned char>	inbuff((unsigned char *)malloc(inlen));
		autoPtr<unsigned char>	outbuff((unsigned char *)malloc(outlen));

		memcpy(inbuff.get(), (unsigned char *) buffer() + getHeaderSize(), inlen);

		if ( _CryptMode != NON_CRYPT )
//			CCryptSystem::encode(_CryptMode, hostid, inbuff.get(), inlen, outbuff.get(), outlen);
			CCryptSystem::encode((TCRYPTMODE)_CryptMode, (TSockId)hostid, inbuff.get(),
				(unsigned long)inlen, outbuff.get(), (unsigned long&)outlen);

		seek(getHeaderSize(), begin);
		serialBuffer(outbuff.get(), outlen);
		resize(outlen + getHeaderSize());

		/*
		uint8	*body = const_cast<uint8 *>(buffer()) + getHeaderSize();
		memcpy(body, outbuff.get(), outlen);
		resize(outlen + getHeaderSize());	
		*/
	}
	catch (ESecurity& e)
	{
		sipwarning("Encode Message Body Error, reason:%s", e.what());
	}
}

void	CMessage::decodeBody(TTcpSockId hostid)
{
	sipassert(isReading());
	if (_CryptMode == DEFAULT )
	{
		sipwarning("STOP : Message<%s> Crypt Mode is Default, Maybe recieved from CallbackServer or CallbackClient", toString().c_str());
		return;
	}

	try
	{
		uint32		inlen	= length() - getHeaderSize();
		//uint32		outlen	= _DefaultCapacity;
		uint32		outlen	= MAX_PACKET_BUF_LEN - getHeaderSize();

		autoPtr<unsigned char>	inbuff((unsigned char *)malloc(inlen));
		autoPtr<unsigned char>	outbuff((unsigned char *)malloc(outlen));

		if ( inlen == 0 )
			return;

		memcpy(inbuff.get(), (unsigned char *) buffer() + getHeaderSize(), inlen);

		if ( _CryptMode != NON_CRYPT )
			CCryptSystem::decode((TCRYPTMODE)_CryptMode, (TSockId)hostid, inbuff.get(),
				 (unsigned long)inlen, outbuff.get(), (unsigned long&)outlen);

		invert();
		seek(getHeaderSize(), begin);
		serialBuffer(outbuff.get(), outlen);
		resize(outlen + getHeaderSize());
		invert();
		seek(getHeaderSize(), begin);

		/*
		uint8	*body = const_cast<uint8 *>(buffer()) + getHeaderSize();
		memcpy(body, outbuff.get(), outlen);
		resize(outlen + getHeaderSize());
		*/		
	}
	catch (ESecurity& e)
	{
		sipwarning("Decode Message Body Error, reason:%s", e.what());
	}
}

}
