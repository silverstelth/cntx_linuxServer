/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "net/login_cookie.h"

using namespace std;
using namespace SIPBASE;


namespace SIPNET {


/*
 * Comparison == operator
 */
bool operator== (const CLoginCookie &c1, const CLoginCookie &c2)
{
	sipassert (c1._Valid && c2._Valid);

	return c1._UserAddr==c2._UserAddr && 
		c1._UserKey==c2._UserKey && 
		c1._UserId==c2._UserId && 
		c1._UserType==c2._UserType && 
		c1._UserName==c2._UserName && 
//		c1._UserMoney==c2._UserMoney && 
		c1._VisitDays==c2._VisitDays && 
		c1._UserExp==c2._UserExp && 
		c1._UserVirtue==c2._UserVirtue && 
		c1._UserFame==c2._UserFame &&
		c1._ExpRoom==c2._ExpRoom &&
		c1._bIsGM==c2._bIsGM;
}

/*
 * Comparison != operator
 */
bool operator!= (const CLoginCookie &c1, const CLoginCookie &c2)
{
	return !(c1 == c2);
}

#ifdef SIP_OS_UNIX
CLoginCookie::CLoginCookie (uint64 addr, uint32 id, uint8 userType, std::string userName, uint32 visitDays, uint16 userExp, uint16 userVirtue, uint16 userFame, uint8 experienceRoom, uint8 bIsGM, uint32 addJifen) :
#else
CLoginCookie::CLoginCookie (uint32 addr, uint32 id, uint8 userType, std::string userName, uint32 visitDays, uint16 userExp, uint16 userVirtue, uint16 userFame, uint8 experienceRoom, uint8 bIsGM, uint32 addJifen) :
#endif
	_Valid(true)
	, _UserAddr(addr)
	, _UserId(id)
	, _UserType(userType)
	, _UserName(userName)
//	, _UserMoney(userMoney)
	, _VisitDays(visitDays)
	, _UserExp(userExp)
	, _UserVirtue(userVirtue)
	, _UserFame(userFame)
	, _ExpRoom(experienceRoom)
	, _bIsGM(bIsGM)
	, _AddJifen(addJifen)
{
	// generates the key for this cookie
	_UserKey = generateKey();
}

/// \todo ace: is the cookie enough to avoid hackers to predice keys?

uint32 CLoginCookie::generateKey()
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
