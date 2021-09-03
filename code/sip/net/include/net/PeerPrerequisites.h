/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __PEERPREREQUISITES_H__
#define __PEERPREREQUISITES_H__

#pragma warning (disable: 4267)
#pragma warning (disable: 4244)

#include <misc/types_sip.h>

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#	undef min
#	undef max
#endif

#include <net/unified_network.h>
#include <string>
#include <misc/smart_ptr.h>

#endif // end of guard __PEERPREREQUISITES_H__
