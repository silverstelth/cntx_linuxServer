/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include "syspacket.h"
#include "misc/types_sip.h"
#include "misc/mem_stream.h"
#include "net/buf_net_base.h"
#include "net/CryptTask.h"

#include <vector>

namespace SIPNET
{

extern const char *LockedSubMessageError;


/**
 * Message memory stream for network. Can be serialized to/from (see SerialBuffer()). Can be sent or received
 * over a network, using the SIP network engine.
 * If MESSAGES_PLAIN_TEXT is defined, the messages will be serialized to/from plain text (human-readable),
 * instead of binary.
 *
 * \date 2001
 */
class CMessage : public SIPBASE::CMemStream
{
public:

	enum TStreamFormat	{ UseDefault, Binary, String };
	enum TMessageType	{ OneWay, Request, Response, Except};

	struct TFormat
	{
		uint8	StringMode : 1,	// true if the message body is string encoded, binary encoded if false otherwise
				LongFormat : 1, // true if the message format is long (d'ho ? all message are long !?!, always true)
				MessageType : 2, // type of the message (from TMessageType), classical message are 'OneWay'

				CryptMode : 4;	// crypt moode


		void serial(SIPBASE::IStream &s)
		{
			if (s.isReading())
			{
				// decode the bit field in a network independant manner
				uint8 b;
				s.serial(b);
				LongFormat = b & 0x01;
				StringMode = (b>>1) & 0x01;
				MessageType= (b>>2) & 0x03;
				CryptMode  = (b>>4) & 0x0F;
			}
			else
			{
				// encode the bit field in a network independant manner
				uint8 b = LongFormat | StringMode << 1 | MessageType << 2 | CryptMode << 4;
				s.serial(b);
			}
		}
	};

	//CMessage (const std::string &name = "", bool inputStream = false, TCRYPTMODE mode = NON_CRYPT, TStreamFormat streamformat = UseDefault, uint32 defaultCapacity = 1000);

	CMessage(const MsgNameType &name = M_SYS_EMPTY, TCRYPTMODE mode = DEFAULT, bool inputStream = false, TStreamFormat streamformat = UseDefault, uint32 defaultCapacity = 1000);

	CMessage (SIPBASE::CMemStream &memstr);

	/// Copy constructor
	CMessage (const CMessage &other);

	void	encodeBody(SIPNET::TTcpSockId hostid);

	void	decodeBody(SIPNET::TTcpSockId hostid);

	/// Assignment operator
	CMessage &operator= (const CMessage &other);

	/// exchange memory data
	void swap(CMessage &other);
	
	/// Make new copy
	void makeDoubleTo(CMessage &other) const;

	/// Sets the message type as a string and put it in the buffer if we are in writing mode
	void setType (const MsgNameType &name, TMessageType type=OneWay);

	void changeType (const MsgNameType &name);

	/// Returns the size, in byte of the header that contains the type name of the message or the type number
	uint32 getHeaderSize () const;
	
	/** The message was filled with an CMemStream, Now, we'll get the message type on this buffer.
	 * This method updates _LengthR with the actual size of the buffer (it calls resetLengthR()).
	 */
	void readType ();

	/// Get the message name (input message only) and advance the current pos
	MsgNameType readTypeAtCurrentPos() const;

	// Returns true if the message type was already set
	bool typeIsSet () const;

	TCRYPTMODE getCryptMode() const;
	void setCryptMode( TCRYPTMODE mode);

	/**
	 * Returns the length (size) of the message, in bytes.
	 * If isReading(), it is the number of bytes that can be read,
	 * otherwise it is the number of bytes that have been written.
	 * Overloaded because uses a specific version of lengthR().
	 */
	virtual uint32	length() const
	{
		if ( isReading() )
		{
			return lengthR();
		}
		else
		{
			return lengthS();
		}
	}

	virtual uint32	lengthS() const
	{
		if (!hasLockedSubMessage())
//			return _BufPos - _Buffer.getPtr();
			return _Buffer.Pos;
		else
//			return (_BufPos - _Buffer.getPtr()) - _SubMessagePosR;
			return _Buffer.Pos - _SubMessagePosR;

	}

	/// Returns the "read" message size (number of bytes to read) (note: see comment about _LengthR)
	uint32			lengthR() const
	{
		return _LengthR;
	}

	virtual sint32	getPos () const throw(SIPBASE::EStream)
	{
//		return (_BufPos - _Buffer.getPtr()) - _SubMessagePosR;
		return _Buffer.Pos - _SubMessagePosR;
	}

//	virtual sint32			getPos() const
//	{
//		return (_BufPos - _Buffer.getPtr()) - _SubMessagePosR;
//	}

	/**
	 * Set an input message to look like, from a message callback's scope, as if it began at the current
	 * pos and ended at the current pos + msgSize, and read the header and return the name of the sub message.
	 *
	 * This method provides a way to pass a big message containing a set of sub messages to their message
	 * callback, without copying each sub message to a new message. If you need to perform some actions
	 * that are not allowed with a locked message (see postconditions), use assignFromSubMessage():
	 * the locked sub message in M1 can be copied to a new message M2 with 'M2.assignFromSubMessage( M1 )'.
	 * 
	 * Preconditions:
	 * - The message is an input message (isReading())
	 * - msgEndPos <= length()
	 * - getPos() >= getHeaderSize()
	 *
	 * Postconditions:
	 * - The sub message is ready to be read from
	 * - length() returns the size of the sub message
	 * - getName() return the name of the sub message
	 * - Unless you call unlockSubMessage(), the following actions will assert or raise an exception:
	 *   Serializing more than the sub message size, clear(), operator=() (from/to), invert().
	 */
	MsgNameType			lockSubMessage( uint32 subMsgSize ) const
	{
		sipassert(!hasLockedSubMessage());
		uint32 subMsgBeginPos = getPos();
		uint32 subMsgEndPos = subMsgBeginPos + subMsgSize;
		sipassertex( isReading() && (subMsgEndPos <= lengthR()), ("%s %u %u", isReading()?"R":"W", subMsgEndPos, lengthR()) );
		MsgNameType name = unconst(*this).readTypeAtCurrentPos();
		_SubMessagePosR = subMsgBeginPos;
//		_LengthR = subMsgEndPos;
		_LengthR = subMsgSize;
		return name;
	}

	/**
	 * Exit from sub message locking, and skip the whole sub message.
	 *
	 * Preconditions:
	 * - The message is an input message (isReading()) and has been locked using lockSubMessage()
	 * - The reading pos is within or at the end of the previous sub message (if any) (see sipassertex)
	 *
	 * Postconditions:
	 * - The current pos is the next byte after the sub message
	 */
	void			unlockSubMessage() const 
	{
		sipassert( isReading() && hasLockedSubMessage() );
		sipassertex( getPos() <= sint32(_SubMessagePosR+_LengthR), ("The callback for msg %s read more data than there is in the message (pos=%d len=%u)", toString(false, false).c_str(), getPos(), _LengthR) );

		uint32 subMsgEndPos = _SubMessagePosR + _LengthR;
		resetSubMessageInternals();
		seek( subMsgEndPos, IStream::begin );
	}

	/// Return true if a sub message has been locked
	bool			hasLockedSubMessage() const
	{
		return (_SubMessagePosR != 0);
	}

	/** If a sub message is locked, return the sub message part
	*/
	virtual const uint8		*buffer() const
	{
		if (hasLockedSubMessage())
		{
//			return _Buffer.getPtr() + _SubMessagePosR;
			return _Buffer.getBuffer().getPtr() + _SubMessagePosR;
		}
		else 
			return CMemStream::buffer();
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
	void			assignFromSubMessage( const CMessage& msgin );

	/**
	 * Transforms the message from input to output or from output to input
	 */
	void invert()
	{
		sipassertex( (!isReading()) || (!hasLockedSubMessage()), ("Inverting %s", LockedSubMessageError) );

		CMemStream::invert();

		if ( isReading() )
		{
			// Write -> Read: skip the header 
			_TypeSet = false;
//			if ( _Buffer.size() != 0 )
			if ( _Buffer.getBuffer().size() != 0 )
				readType();
		}
		// For Read -> Write, please use clear()
	}

	// Clear the message. With this function, you can reuse a message to create another message
	void clear ();

	/** Returns the type name in string if available. Be sure that the message have the name of the message type
	 * In a callback driven by message name, getName() does not necessarily return the right name.
	 */
	MsgNameType getName () const;

	/** Return the type of the message.
	 */
	TMessageType getType() const;
	
	/** Returns a readable string to display it to the screen. It's only for debugging purpose!
	 * Don't use it for anything else than to debugging, the string format could change in the future.
	 * \param hexFormat If true, display all bytes in hexadecimal
	 * \param textFormat If true, display all bytes as chars (above 31, otherwise '.')
	 */
	std::string toString ( bool hexFormat=false, bool textFormat=false ) const;

	/// Get message's name as string
	std::string getNameAsString() const;
	/// Set default stream mode
	static void	setDefaultStringMode( bool stringmode ) { _DefaultStringMode = stringmode; }

	/// Return an input stream containing the stream beginning in the message at the specified pos
	SIPBASE::CMemStream	extractStreamFromPos( sint32 pos );

	/// Encapsulate/decapsulate another message inside the current message
	void	serialMessage( CMessage& msg );

	/// 氅旍巹歆?��??氤奠??橂姅??毹鸽??�€???氤胳�??�澊??prefix�??届�?滊嫟.
	template <class T> void copyWithPrefix(T &prefix, CMessage &msg)
	{
		sipassert(!hasLockedSubMessage() && isReading() && typeIsSet() && !msg.isReading());

		if (!msg.typeIsSet())
			msg.init(getName(), UseDefault);
		else
			msg.seek(msg.getHeaderSize(), begin);

		msg.serial(prefix);
		msg.serialBuffer((uint8 *)_Buffer.getBuffer().getPtr() + getHeaderSize(), lengthR() - getHeaderSize());
	}

	/// 氅旍巹歆?��??氤奠??橂姅??毹鸽??�€???氤胳�??�澊????�澑 prefix???�戨�??
	template <class T> void copyWithoutPrefix(T &prefix, CMessage &msg)
	{
		sipassert(!hasLockedSubMessage() && isReading() && typeIsSet() && !msg.isReading());

		if (!msg.typeIsSet())
			msg.init(getName(), UseDefault);
		else
			msg.seek(msg.getHeaderSize(), begin);

		seek(getHeaderSize(), begin);
		serial(prefix);
		msg.serialBuffer((uint8 *)_Buffer.getBuffer().getPtr() + getPos(), lengthR() - getPos());
	}

	void appendBufferTo(CMessage &msg, bool bBegin = true)
	{
		if(!isReading())
			invert();

		sipassert(!hasLockedSubMessage() && typeIsSet() && !msg.isReading() && msg.typeIsSet());

		if (bBegin)
			seek(getHeaderSize(), begin);
		msg.serialBuffer((uint8 *)_Buffer.getBuffer().getPtr() + getPos(), lengthR() - getPos());
	}

	static MsgNameType GetMsgNameFromBuffer(const uint8* pBuffer, int nLength)
	{
		const	uint32 uHeadeSize = 5;	// 5 = MessageNmber(4) + MessageType(1)
		const	uint32 MAXMESNAME = 50;

		uint32	uMessageLimitSize = uHeadeSize + sizeof(uint32);
		if ((uint32)nLength < uMessageLimitSize)
			return M_SYS_EMPTY;

#ifdef SIP_MSG_NAME_DWORD
		uint32	MsgName = *((uint32*)(&pBuffer[uHeadeSize]));
#else
		uint32	uMessageNameLenPos = uHeadeSize;
		uint32	uMesNameLength = *(uint32*)(&pBuffer[uMessageNameLenPos]);

		if (uMesNameLength > MAXMESNAME)
			return M_SYS_EMPTY;

		uint32	uLengthToMesName = uMessageLimitSize + uMesNameLength;
		if (uLengthToMesName > nLength)
			return M_SYS_EMPTY;

		uint32	uMessageNamePos = uMessageNameLenPos + sizeof(uint32);

		char mesname[MAXMESNAME+1];		memset(mesname, 0, sizeof(mesname));
		strncpy(mesname, (char *)(&pBuffer[uMessageNamePos]), uMesNameLength);

		string	MsgName = mesname;
#endif // SIP_MSG_NAME_DWORD
		return MsgName;
	}

protected:

	/// Utility method
	void		init( const MsgNameType &name, TStreamFormat streamformat );

	/// Utility method
	void		resetSubMessageInternals() const
	{
		_SubMessagePosR = 0;
//		_LengthR = _Buffer.size();
		_LengthR = _Buffer.getBuffer().size();
	}

private:
	MsgNameType							_Name;

	mutable TMessageType				_Type;
	
	// When sub message lock mode is enabled, beginning position of sub message to read (before header)
	mutable uint32						_SubMessagePosR;

	// Length (can be smaller than _Buffer.size() if limitLength() is used) (updated in reading mode only)
	mutable uint32						_LengthR;

	TCRYPTMODE							_CryptMode; // must not be DEFAULT

	// Size of the header (that contains the name type or number type)
	uint32								_HeaderSize;

	bool								_TypeSet;

	// Default stream format
	static bool							_DefaultStringMode;
};

}

#endif // __MESSAGE_H__

/* End of message.h */
