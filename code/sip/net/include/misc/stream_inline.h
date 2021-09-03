/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STREAM_INLINE_H__
#define __STREAM_INLINE_H__

#include <stdio.h> // <cstdio>
#include "debug.h"

namespace	SIPBASE
{


// ======================================================================================================
// ======================================================================================================
// IBasicStream Inline Implementation.
// ======================================================================================================
// ======================================================================================================


// ======================================================================================================
// ======================================================================================================
// ======================================================================================================

// ======================================================================================================
inline	IStream::IStream(bool inputStream)
{
	_InputStream= inputStream;
	_XML = false;
	resetPtrTable();
}


// ======================================================================================================
inline	bool		IStream::isReading() const
{
	return _InputStream;
}


// ======================================================================================================
// ======================================================================================================
// ======================================================================================================

// ======================================================================================================
inline	void		IStream::serial(uint8 &b) 
{
	serialBuffer((uint8 *)&b, 1);
}

// ======================================================================================================
inline	void		IStream::serial(sint8 &b) 
{
	serialBuffer((uint8 *)&b, 1);
}

// ======================================================================================================
inline	void		IStream::serial(uint16 &b) 
{
#ifdef SIP_LITTLE_ENDIAN
	serialBuffer((uint8 *)&b, 2);
#else // SIP_LITTLE_ENDIAN
	uint16	v;
	if(isReading())
	{
		serialBuffer((uint8 *)&v, 2);
		SIPBASE_BSWAP16(v);
		b=v;
	}
	else
	{
		v=b;
		SIPBASE_BSWAP16(v);
		serialBuffer((uint8 *)&v, 2);
	}
#endif // SIP_LITTLE_ENDIAN
}

// ======================================================================================================
inline	void		IStream::serial(sint16 &b) 
{
#ifdef SIP_LITTLE_ENDIAN
	serialBuffer((uint8 *)&b, 2);
#else // SIP_LITTLE_ENDIAN
	uint16	v;
	if(isReading())
	{
		serialBuffer((uint8 *)&v, 2);
		SIPBASE_BSWAP16(v);
		b=v;
	}
	else
	{
		v=b;
		SIPBASE_BSWAP16(v);
		serialBuffer((uint8 *)&v, 2);
	}
#endif // SIP_LITTLE_ENDIAN
}

// ======================================================================================================
inline	void		IStream::serial(uint32 &b) 
{
#ifdef SIP_LITTLE_ENDIAN
	serialBuffer((uint8 *)&b, 4);
#else // SIP_LITTLE_ENDIAN
	uint32	v;
	if(isReading())
	{
		serialBuffer((uint8 *)&v, 4);
		SIPBASE_BSWAP32(v);
		b=v;
	}
	else
	{
		v=b;
		SIPBASE_BSWAP32(v);
		serialBuffer((uint8 *)&v, 4);
	}
#endif // SIP_LITTLE_ENDIAN
}

// ======================================================================================================
inline	void		IStream::serial(sint32 &b) 
{
#ifdef SIP_LITTLE_ENDIAN
	serialBuffer((uint8 *)&b, 4);
#else // SIP_LITTLE_ENDIAN
	uint32	v;
	if(isReading())
	{
		serialBuffer((uint8 *)&v, 4);
		SIPBASE_BSWAP32(v);
		b=v;
	}
	else
	{
		v=b;
		SIPBASE_BSWAP32(v);
		serialBuffer((uint8 *)&v, 4);
	}
#endif // SIP_LITTLE_ENDIAN
}

// ======================================================================================================
inline	void		IStream::serial(uint64 &b) 
{
#ifdef SIP_LITTLE_ENDIAN
		serialBuffer((uint8 *)&b, 8);
#else // SIP_LITTLE_ENDIAN
	uint64	v;
	if(isReading())
	{
		serialBuffer((uint8 *)&v, 8);
		SIPBASE_BSWAP64(v);
		b=v;
	}
	else
	{
		v=b;
		SIPBASE_BSWAP64(v);
		serialBuffer((uint8 *)&v, 8);
	}
#endif // SIP_LITTLE_ENDIAN
}

// ======================================================================================================
inline	void		IStream::serial(sint64 &b) 
{
#ifdef SIP_LITTLE_ENDIAN
	serialBuffer((uint8 *)&b, 8);
#else // SIP_LITTLE_ENDIAN
	uint64	v;
	if(isReading())
	{
		serialBuffer((uint8 *)&v, 8);
		SIPBASE_BSWAP64(v);
		b=v;
	}
	else
	{
		v=b;
		SIPBASE_BSWAP64(v);
		serialBuffer((uint8 *)&v, 8);
	}
#endif // SIP_LITTLE_ENDIAN
}

// ======================================================================================================
inline	void		IStream::serial(float &b) 
{
#ifdef SIP_LITTLE_ENDIAN
	serialBuffer((uint8 *)&b, 4);
#else // SIP_LITTLE_ENDIAN
	uint32	v;
	if(isReading())
	{
		serialBuffer((uint8 *)&v, 4);
		SIPBASE_BSWAP32(v);
		b=*((float*)&v);
	}
	else
	{
		v=*((uint32*)&b);
		SIPBASE_BSWAP32(v);
		serialBuffer((uint8 *)&v, 4);
	}
#endif // SIP_LITTLE_ENDIAN
}

// ======================================================================================================
inline	void		IStream::serial(double &b) 
{
#ifdef SIP_LITTLE_ENDIAN
	serialBuffer((uint8 *)&b, 8);
#else // SIP_LITTLE_ENDIAN
	uint64	v;
	if(isReading())
	{
		serialBuffer((uint8 *)&v, 8);
		SIPBASE_BSWAP64(v);
		b=*((double*)&v);
	}
	else
	{
		v=*((uint64*)&b);
		SIPBASE_BSWAP64(v);
		serialBuffer((uint8 *)&v, 8);
	}
#endif // SIP_LITTLE_ENDIAN
}

// ======================================================================================================
inline	void		IStream::serial(bool &b) 
{
	serialBit(b);
}

#ifndef SIP_OS_CYGWIN
// ======================================================================================================
inline	void		IStream::serial(char &b) 
{
	serialBuffer((uint8 *)&b, 1);
}
#endif

// ======================================================================================================
inline	void		IStream::serial(std::string &b) 
{
	uint32	len=0;
	// Read/Write the length.
	if(isReading())
	{
		serial(len);

		// check stream holds enough bytes (avoid STL to crash on resize)
		if (!checkStreamSize(len))
			return;

		b.resize(len);
	}
	else
	{
		len = (uint32)b.size();
		if (len>1000000)
			throw SIPBASE::EInvalidDataStream( "IStream: Trying to write a string of %u bytes", len );
		serial(len);
	}

/*
	// Read/Write the string.
	for(sint i=0;i<len;i++)
		serial(b[i]);
*/

	if (len > 0)
		serialBuffer((uint8*)(&(b[0])), len);
}


// ======================================================================================================
inline	void		IStream::serial(ucstring &b) 
{
	uint32	len=0;
	// Read/Write the length.
	if(isReading())
	{
		serial(len);

		// check stream holds enough bytes (avoid STL to crash on resize)
		if (!checkStreamSize(len))
			return;

		b.resize(len);
	}
	else
	{
		len = (uint32)b.size();
		if (len>1000000)
			throw SIPBASE::EInvalidDataStream( "IStream: Trying to write an ucstring of %u bytes", len );
		serial(len);
	}
	// Read/Write the string.
	for(uint i=0;i!=len;++i)
		serial(b[i]);
}

inline	void		IStream::serial(std::basic_string<ucchar> &b) 
{
	uint32	len=0;
	// Read/Write the length.
	if(isReading())
	{
		serial(len);

		// check stream holds enough bytes (avoid STL to crash on resize)
		if (!checkStreamSize(len))
			return;

		b.resize(len);
	}
	else
	{
		len = (uint32)b.size();
		if (len>1000000)
			throw SIPBASE::EInvalidDataStream( "IStream: Trying to write an ucstring of %u bytes", len );
		serial(len);
	}
	// Read/Write the string.
	for(uint i=0;i!=len;++i)
		serial(b[i]);
}


// ======================================================================================================
inline uint8			IStream::serialBitField8(uint8  bf)
{
	serial(bf);
	return bf;
}
// ======================================================================================================
inline uint16			IStream::serialBitField16(uint16  bf)
{
	serial(bf);
	return bf;
}
// ======================================================================================================
inline uint32			IStream::serialBitField32(uint32  bf)
{
	serial(bf);
	return bf;
}


}


#endif // __STREAM_INLINE_H__

/* End of stream_inline.h */
