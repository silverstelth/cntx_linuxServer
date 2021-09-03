/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __TRAITS_SIP_H__
#define __TRAITS_SIP_H__

#include "rgba.h"
#include "vector.h"

namespace SIPBASE
{

/** Class that gives informations about a type. Useful to do some optimization in templates functions / class
  * This class is intended to be specialized and taylored for each type of interest
  *
  * \date 2004
  */
template <class T>
struct CTraits
{
	enum { HasTrivialCtor = false };     // if true, the default ctor does nothing useful (example  : built-in types or Plain Old Datas structs)
	enum { HasTrivialDtor = false };     // the dtor does nothing useful and is not worth calling. Useful to optimize containers clean-up
	enum { SupportRawCopy = false };     // the object supports raw copy with memcpy
	// to be completed ..
};


#define SIP_TRIVIAL_TYPE_TRAITS(type)     \
template <>								 \
struct CTraits<type>                     \
{                                        \
	enum { HasTrivialCtor = true };      \
	enum { HasTrivialDtor = true };      \
	enum { SupportRawCopy = true };      \
};

// integral types
SIP_TRIVIAL_TYPE_TRAITS(bool);
#ifdef SIP_COMP_VC6
SIP_TRIVIAL_TYPE_TRAITS(sint8);
SIP_TRIVIAL_TYPE_TRAITS(uint8);
#endif // SIP_COMP_VC6
SIP_TRIVIAL_TYPE_TRAITS(sint16);
SIP_TRIVIAL_TYPE_TRAITS(uint16);
SIP_TRIVIAL_TYPE_TRAITS(sint32);
SIP_TRIVIAL_TYPE_TRAITS(uint32);
SIP_TRIVIAL_TYPE_TRAITS(sint64);
SIP_TRIVIAL_TYPE_TRAITS(uint64);
#ifdef SIP_COMP_VC6
SIP_TRIVIAL_TYPE_TRAITS(sint);
SIP_TRIVIAL_TYPE_TRAITS(uint);
#endif // SIP_COMP_VC6

// characters 
SIP_TRIVIAL_TYPE_TRAITS(char);
SIP_TRIVIAL_TYPE_TRAITS(unsigned char);

// numeric types
SIP_TRIVIAL_TYPE_TRAITS(float);
SIP_TRIVIAL_TYPE_TRAITS(double);

// misc
SIP_TRIVIAL_TYPE_TRAITS(CVector);
SIP_TRIVIAL_TYPE_TRAITS(CRGBA);

//.. to be completed

} // SIPBASE

#endif


