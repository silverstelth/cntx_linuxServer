/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STDNET_H__
#define __STDNET_H__

#include "misc/types_sip.h"

#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>

#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <algorithm>
//#include <sstream>
#include <exception>
#include <utility>
#include <deque>

#include <memory>

#include "misc/common.h"
#include "misc/debug.h"

#include "misc/stream.h"
#include "misc/mem_stream.h"
#include "misc/time_sip.h"
#include "misc/command.h"
#include "misc/variable.h"
#include "misc/hierarchical_timer.h"

#ifdef SIP_OS_UNIX
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#	include <unistd.h>
#endif

#ifdef SIP_OS_UNIX
#define		SOCKET_ERROR_NUM	errno
#define		sleep			usleep
#else
#define		SOCKET_ERROR_NUM	SOCKET_ERROR
#define		sleep			Sleep
#endif

#endif // end of guard __STDNET_H__
