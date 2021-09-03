/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/smart_ptr.h"

namespace SIPBASE
{

// Use a Raw structure, to be sure it is initialized before any constructor calls...
//CPtrInfo() {Ptr=NULL; RefCount=0x7FFFFFFF; IsNullPtrInfo=true;}
CRefCount::CPtrInfoBase		CRefCount::NullPtrInfo= {NULL, 0x7FFFFFFF, true};


// must not be static
void	dummy_to_avoid_stupid_4768_smart_ptr_cpp()
{
}


}

