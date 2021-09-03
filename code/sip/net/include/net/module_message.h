/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MODULE_MESSAGE_H__
#define __MODULE_MESSAGE_H__

#include "misc/enum_bitset.h"
#include "net/message.h"
#include "module_common.h"


namespace SIPNET
{
	/** Module message header coder/decoder
	 *	Codec for module message header data.
	 */
	class CModuleMessageHeaderCodec
	{
	public:
		enum TMessageType
		{
			/// Standard one way message
			mt_oneway,
			/// Two way request
			mt_twoway_request,
			/// Two way response
			mt_twoway_response,


			/// A special checking value
			mt_num_types,
			/// invalid flag
			mt_invalid = mt_num_types

		};

		static void encode(CMessage &headerMessage, TMessageType msgType, TModuleId senderProxyId, TModuleId addresseeProxyId)
		{
			serial(headerMessage, msgType, senderProxyId, addresseeProxyId);
		}

		static void decode(const CMessage &headerMessage, TMessageType &msgType, TModuleId &senderProxyId, TModuleId &addresseeProxyId)
		{
			serial(const_cast<CMessage&>(headerMessage), msgType, senderProxyId, addresseeProxyId);
		}

	private:
		static void serial(CMessage &headerMessage, TMessageType &msgType, TModuleId &senderProxyId, TModuleId &addresseeProxyId)
		{
			uint8 mt;
			if (headerMessage.isReading())
			{
				headerMessage.serial(mt);
				msgType = CModuleMessageHeaderCodec::TMessageType(mt);
			}
			else
			{
				mt = msgType;
				headerMessage.serial(mt);
			}
			headerMessage.serial(senderProxyId);
			headerMessage.serial(addresseeProxyId);
		}
	};


	/** An utility struct to serial binary buffer.
	 *	WARNING : you must be aware that using binary buffer serialisation
	 *	is not fair with the portability and endiennes problem.
	 *	Use it ONLY when you now what you are doing and when
	 *	bytes ordering is not an issue.
	 */
	struct TBinBuffer
	{
	private:
		uint8			*_Buffer;
		uint32			_BufferSize;
		mutable bool	_Owner;

	public:
		
		/// default constructor, used to read in stream
		TBinBuffer()
			:	_Buffer(NULL),
				_BufferSize(0),
				_Owner(true)
		{
		}

		TBinBuffer(const uint8 *buffer, uint32 bufferSize)
			:	_Buffer(const_cast<uint8*>(buffer)),
				_BufferSize(bufferSize),
				_Owner(false)
		{
		}

		// copy constructor transfere ownership of the buffer (if it is owned)
		TBinBuffer(const TBinBuffer &other)
			:	_Buffer(other._Buffer),
				_BufferSize(other._BufferSize),
				_Owner(other._Owner)
		{
			// remove owning on source
			other._Owner = false;
		}

		~TBinBuffer()
		{
			if (_Owner && _Buffer != NULL)
				delete _Buffer;
		}

		void serial(SIPBASE::IStream &s)
		{
			if (s.isReading())
			{
				sipassert(_Buffer == NULL);
				s.serial(_BufferSize);

				_Buffer = new uint8[_BufferSize];

				s.serialBuffer(_Buffer, _BufferSize);
			}
			else
			{
				s.serialBufferWithSize(_Buffer, _BufferSize);
			}
		}

		uint32 getBufferSize() const
		{
			return _BufferSize;
		}

		uint8 *getBuffer() const
		{
			return _Buffer;
		}
			

		/** Release the buffer by returning the pointer to the caller.
		 *	The caller is now owner and responsible of the allocated
		 *	buffer.
		 */
		uint8 *release()
		{
			sipassert(_Owner);
			_Owner = false;
			return _Buffer;
		}
	};


} // namespace SIPNET

#endif // __MODULE_MESSAGE_H__
