/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MD5_H__
#define __MD5_H__

#include "types_sip.h"
#include <string>

namespace SIPBASE
{

class IStream;



// ****************************************************************************
/**
 * MD5 Low level routines
 * Largely inspired from the RSA Data Security works
 * \date July 2004
 */
/* Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
   rights reserved.

   License to copy and use this software is granted provided that it
   is identified as the "RSA Data Security, Inc. MD5 Message-Digest
   Algorithm" in all material mentioning or referencing this software
   or this function.

   License is also granted to make and use derivative works provided
   that such works are identified as "derived from the RSA Data
   Security, Inc. MD5 Message-Digest Algorithm" in all material
   mentioning or referencing the derived work.

   RSA Data Security, Inc. makes no representations concerning either
   the merchantability of this software or the suitability of this
   software for any particular purpose. It is provided "as is"
   without express or implied warranty of any kind.

   These notices must be retained in any copies of any part of this
   documentation and/or software.
   */

// ****************************************************************************
struct CHashKeyMD5
{
	uint8 Data[16];

	void clear();
	std::string toString() const;
	std::basic_string<ucchar> toStringW() const;
	bool fromString(const std::string &in);
	bool fromString(const std::basic_string<ucchar> &in);
	bool operator==(const CHashKeyMD5 &in) const;
	bool operator!=(const CHashKeyMD5 &in) const;
	bool operator<(const CHashKeyMD5 &in) const;

	void serial (SIPBASE::IStream &s);
};

// ****************************************************************************
class CMD5Context
{

public:

	void init();
	void update (const uint8 *pBufIn, uint32 nBufLength);
	void final (CHashKeyMD5 &out);

private:

	uint32	State[4];	// state (ABCD)
	uint32	Count[2];	// number of bits, modulo 2^64 (lsb first)
	uint8	Buffer[64]; // input buffer

	static uint8 Padding[64];

private:

	void transform (uint32 state[4], const uint8 block[64]);
	void encode (uint8 *output, const uint32 *input, uint len);
	void decode (uint32 *output, const uint8 *input, uint len);

};


// ****************************************************************************
/**
 * MD5 High level routines
 * Largely inspired from the RSA Data Security works
 * \date July 2004
 */

/*
inline bool operator <(const struct CHashKeyMD5 &a,const struct CHashKeyMD5 &b)
{
	return a < b;
}
*/

// This function get a filename (it works with big files) and returns his MD5 hash key
CHashKeyMD5 getMD5(const std::string &filename);
CHashKeyMD5 getMD5(const std::basic_string<ucchar> &filename);

// This function get a buffer with size and returns his MD5 hash key
CHashKeyMD5 getMD5(const uint8 *buffer, uint32 size);

}; // namespace SIPBASE

#endif // __MD5_H__

/* End of md5.h */
