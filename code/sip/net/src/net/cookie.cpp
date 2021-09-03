/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/cookie.h"

using namespace std;
using namespace SIPBASE;


namespace SIPNET {


/*
 * Comparison == operator
 */
bool operator== (const CCookie &c1, const CCookie &c2)
{
	sipassert (c1._Valid && c2._Valid);

	return c1._UserAddr==c2._UserAddr && 
		c1._UserKey==c2._UserKey;
}

/*
 * Comparison != operator
 */
bool operator!= (const CCookie &c1, const CCookie &c2)
{
	return !(c1 == c2);
}

CCookie::CCookie (uint32 addr) :
	_Valid(true)
	, _UserAddr(addr)
{
	// generates the key for this cookie
	_UserKey = generateKey();
}

/// \todo ace: is the cookie enough to avoid hackers to predice keys?

uint32 CCookie::generateKey()
{
	uint32 t = (uint32)time (NULL);
    srand (t);

	uint32 r = rand ();
	static uint32 n = 0;
	n++;

	// 12bits for the time (in second) => loop in 1 hour
	//  8bits for random => 256 case
	// 12bits for the inc number => can generate 4096 keys per second without any problem (if you generate more than this number, you could have 2 same keys)
	return (t&0xFFF)<<20 | (r&0xFF)<<12 | (n&0xFFF);

	// 12bits for the time (in second) => loop in 1 hour
	// 20bits for the inc number => can generate more than 1 million keys per second without any problem (never exceed on my computer)
//	return (t&0xFFF)<<20 | (n&0xFFFFF);
}


/* test key generation
void main()
{
	set<uint32> myset;

	// generates the key for this cookie
	while (true)
	{
		uint32 val = (t&0xFFF)<<20 | (r&0xFF)<<12 | (n&0xFFF);
		pair<set<uint32>::iterator,bool> p = myset.insert (val);
		if (!p.second) printf("%10u 0x%x already inserted\n", val, val);
	}
}
*/

} // NL.
