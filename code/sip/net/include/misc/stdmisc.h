/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STDMISC_H__
#define __STDMISC_H__

#include "misc/types_sip.h"

#include <stdlib.h> // <cstdlib>
#include <stdio.h> // <cstdio>
#include <math.h> // <cmath>

#include <string>
#include <vector>
#include <list>
#include <map>
#ifdef SIP_OS_UNIX
#	include <unordered_map>
#	include <unordered_set>
#else
#	include <hash_map>
#	include <hash_set>
#endif
#include <set>
#include <algorithm>
//#include <sstream>
#include <exception>
#include <utility>
#include <deque>

#include "misc/mem_displayer.h"
#include "misc/common.h"
#include "misc/debug.h"
#include "misc/system_info.h"
#include "misc/fast_mem.h"

FILE* sfWfopen(const wchar_t *, const wchar_t *);
int sfWrename(const wchar_t *, const wchar_t *);

#endif // end of guard __STDMISC_H__
