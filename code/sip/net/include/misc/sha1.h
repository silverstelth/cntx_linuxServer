/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SHA1_H__
#define __SHA1_H__

#include <string>
#include "common.h"
#include "stream.h"

struct CHashKey
{
	CHashKey () { HashKeyString.resize(20); }
	
	CHashKey (const unsigned char Message_Digest[20])
	{
		HashKeyString = "";
		for(sint i = 0; i < 20 ; ++i)
		{
			HashKeyString += Message_Digest[i];
		}
	}
	
	// Init the hash key with a binary key format or a text key format
	CHashKey (const std::string &str)
	{
		if (str.size() == 20)
		{
			HashKeyString = str;
		}
		else if (str.size() == 40)
		{
			HashKeyString = "";
			for(uint i = 0; i < str.size(); i+=2)
			{
				uint8 val;
				if (isdigit((unsigned char)str[i+0]))
					val = str[i+0]-'0';
				else
					val = 10+tolower(str[i+0])-'a';
				val *= 16;
				if (isdigit((unsigned char)str[i+1]))
					val += str[i+1]-'0';
				else
					val += 10+tolower(str[i+1])-'a';

				HashKeyString += val;
			}
		}
		else
		{
			sipwarning ("SHA: Bad hash key format");
		}
	}

	// return the hash key in text format
	std::string toString() const
	{
		std::string str;
		for (uint i = 0; i < HashKeyString.size(); i++)
		{
			str += SIPBASE::toString("%02X", (uint8)(HashKeyString[i]));
		}
		return str;
	}

	// serial the hash key in binary format
	void serial (SIPBASE::IStream &stream)
	{
		stream.serial (HashKeyString);
	}

	bool	operator==(const CHashKey &v) const
	{
		return HashKeyString == v.HashKeyString;
	}

	// this string is always 20 bytes long and is the code in binary format (can't print it directly)
	std::string HashKeyString;
};

inline bool operator <(const struct CHashKey &a,const struct CHashKey &b)
{
	return a.HashKeyString<b.HashKeyString;
}

// This function get a filename (it works with big files) and returns his SHA hash key
CHashKey getSHA1(const std::string &filename);

// This function get a buffer with size and returns his SHA hash key
CHashKey getSHA1(const uint8 *buffer, uint32 size);

#endif // __SHA1_H__
