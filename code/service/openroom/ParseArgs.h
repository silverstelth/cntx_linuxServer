
#ifndef SIP_PARSEARGS_H
#define SIP_PARSEARGS_H

#include "misc/types_sip.h"

#if defined(SIP_OS_WINDOWS) && defined(_WINDOWS)
#	include <windows.h>
#	undef min
#	undef max
#endif

#include "misc/sstring.h"

#include <string>
#include <vector>

class CParseArgs
{
public:
	static	bool		haveArg (char argName);
	static	std::string	getArg (char argName);
	static	bool		haveLongArg (const char* argName);
	static	std::string	getLongArg (const char* argName);
	static	void		setArgs (const char *args);
	static	void		setArgs (int argc, const char **argv);

	static	SIPBASE::CVectorSStringA	_Args;
};

#endif // SIP_PARSEARGS_H
