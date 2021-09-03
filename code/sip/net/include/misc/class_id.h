/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CLASS_ID_H__
#define __CLASS_ID_H__

#include "types_sip.h"

namespace	SIPBASE
{

// ***************************************************************************
/**
 * A unique id to specify Object by a uint64.
 * The Deriver should use a Max-like Id generator, to identify his own object.
 * \date 2000
 */
class	CClassId
{
	uint64	Uid;

public:
	static const	CClassId	Null;

public:
	CClassId() {Uid=0;}
	CClassId(uint32 a, uint32 b) {Uid= ((uint64)a<<32) | b;}
	CClassId(uint64 a) {Uid=a;}
	bool	operator==(const CClassId &o) const {return Uid==o.Uid;}
	bool	operator!=(const CClassId &o) const {return Uid!=o.Uid;}
	bool	operator<=(const CClassId &o) const {return Uid<=o.Uid;}
	bool	operator>=(const CClassId &o) const {return Uid>=o.Uid;}
	bool	operator<(const CClassId &o) const {return Uid<o.Uid;}
	bool	operator>(const CClassId &o) const {return Uid>o.Uid;}
	//CClassId& operator=(const CClassId &o) { Uid = o.Uid; return *this;}
	operator uint64() const {return Uid;}

};


}


#endif // __CLASS_ID_H__

/* End of class_id.h */
