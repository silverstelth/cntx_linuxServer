/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#ifdef SIP_OS_WINDOWS
#  include <windows.h>
#  include <io.h>
#  undef min
#  undef max
#elif defined SIP_OS_UNIX
#  include <unistd.h>
#  include <cstring>
#  include <cerrno>
#  include <csignal>
#  include <pthread.h>
#  include <sched.h>
#endif

#include "misc/command.h"
#include "misc/path.h"

using namespace std;

#if (defined(SIP_OS_WINDOWS) && defined(_STLPORT_VERSION))
	#ifdef _STLP_DEBUG
		#define STL_MODE "debug"
	#else
		#define STL_MODE "normal"
	#endif // _STLP_DEBUG

	#if(_STLPORT_MAJOR == 0)
		#define STL_STR_MAJOR "0"
	#elif(_STLPORT_MAJOR == 1)
		#define STL_STR_MAJOR "1"
	#elif(_STLPORT_MAJOR == 2)
		#define STL_STR_MAJOR "2"
	#elif(_STLPORT_MAJOR == 3)
		#define STL_STR_MAJOR "3"
	#elif(_STLPORT_MAJOR == 4)
		#define STL_STR_MAJOR "4"
	#elif(_STLPORT_MAJOR == 5)
		#define STL_STR_MAJOR "5"
	#elif(_STLPORT_MAJOR == 6)
		#define STL_STR_MAJOR "6"
	#elif(_STLPORT_MAJOR == 7)
		#define STL_STR_MAJOR "7"
	#elif(_STLPORT_MAJOR == 8)
		#define STL_STR_MAJOR "8"
	#elif(_STLPORT_MAJOR == 9)
		#define STL_STR_MAJOR "9"
	#endif // _STLPORT_MAJOR

	#if(_STLPORT_MINOR == 0)
		#define STL_STR_MINOR "0"
	#elif(_STLPORT_MINOR == 1)
		#define STL_STR_MINOR "1"
	#elif(_STLPORT_MINOR == 2)
		#define STL_STR_MINOR "2"
	#elif(_STLPORT_MINOR == 3)
		#define STL_STR_MINOR "3"
	#elif(_STLPORT_MINOR == 4)
		#define STL_STR_MINOR "4"
	#elif(_STLPORT_MINOR == 5)
		#define STL_STR_MINOR "5"
	#elif(_STLPORT_MINOR == 6)
		#define STL_STR_MINOR "6"
	#elif(_STLPORT_MINOR == 7)
		#define STL_STR_MINOR "7"
	#elif(_STLPORT_MINOR == 8)
		#define STL_STR_MINOR "8"
	#elif(_STLPORT_MINOR == 9)
		#define STL_STR_MINOR "9"
	#endif // _STLPORT_MINOR

	#if(_STLPORT_PATCHLEVEL == 0)
		#define STL_STR_PATCHLEVEL "0"
	#elif(_STLPORT_PATCHLEVEL == 1)
		#define STL_STR_PATCHLEVEL "1"
	#elif(_STLPORT_PATCHLEVEL == 2)
		#define STL_STR_PATCHLEVEL "2"
	#elif(_STLPORT_PATCHLEVEL == 3)
		#define STL_STR_PATCHLEVEL "3"
	#elif(_STLPORT_PATCHLEVEL == 4)
		#define STL_STR_PATCHLEVEL "4"
	#elif(_STLPORT_PATCHLEVEL == 5)
		#define STL_STR_PATCHLEVEL "5"
	#elif(_STLPORT_PATCHLEVEL == 6)
		#define STL_STR_PATCHLEVEL "6"
	#elif(_STLPORT_PATCHLEVEL == 7)
		#define STL_STR_PATCHLEVEL "7"
	#elif(_STLPORT_PATCHLEVEL == 8)
		#define STL_STR_PATCHLEVEL "8"
	#elif(_STLPORT_PATCHLEVEL == 9)
		#define STL_STR_PATCHLEVEL "9"
	#endif // _STLPORT_PATCHLEVEL

	#pragma message( " " )

	#pragma message( "Using STLPort version " STL_STR_MAJOR "." STL_STR_MINOR "." STL_STR_PATCHLEVEL " in " STL_MODE " mode" )

	#if FINAL_VERSION
		#pragma message( "************************" )
		#pragma message( "**** FINAL_VERSION *****" )
		#pragma message( "************************" )
	#else
		#pragma message( "Not using FINAL_VERSION")
	#endif // FINAL_VERSION

	#ifdef ASSERT_THROW_EXCEPTION
		#pragma message( "sipassert throws exceptions" )
	#else
		#pragma message( "sipassert does not throw exceptions" )
	#endif // ASSERT_THROW_EXCEPTION

	#pragma message( " " )

#endif // SIP_OS_WINDOWS

namespace SIPBASE
{

/*
 * Portable Sleep() function that suspends the execution of the calling thread for a number of milliseconds.
 * Note: the resolution of the timer is system-dependant and may be more than 1 millisecond.
 */
void sipSleep( uint32 ms )
{
#ifdef SIP_OS_WINDOWS

/// \todo yoyo: BUG WITH DEBUG/_CONSOLE!!!! a Sleep(0) "block" the other thread!!!
#ifdef SIP_DEBUG
	ms = max(ms, (uint32)1);
#endif

	Sleep( ms );

#elif defined SIP_OS_UNIX
	//usleep( ms*1000 ); // resolution: 20 ms!

	timespec ts;
	ts.tv_sec = ms/1000;
	ts.tv_nsec = (ms%1000)*1000000;
	int res;
	do
	{
		res = nanosleep( &ts, &ts ); // resolution: 10 ms (with common scheduling policy)
	}
	while ( (res != 0) && (errno==EINTR) );
#endif
}


/*
 * Returns Thread Id (note: on Linux, Process Id is the same as the Thread Id)
 */
uint getThreadId()
{
#ifdef SIP_OS_WINDOWS
	return GetCurrentThreadId();
#elif defined SIP_OS_UNIX
	return uint(pthread_self());
	// doesnt work on linux kernel 2.6	return getpid();
#endif

}


/*
 * Returns a readable string from a vector of bytes. '\0' are replaced by ' '
 */
string stringFromVector( const vector<uint8>& v, bool limited )
{
	string s;

	if (!v.empty())
	{
		int size = v.size ();
		if (limited && size > 1000)
		{
			string middle = "...<buf too big,skip middle part>...";
			s.resize (1000 + middle.size());
			memcpy (&*s.begin(), &*v.begin(), 500);
			memcpy (&*s.begin()+500, &*middle.begin(), middle.size());
			memcpy (&*s.begin()+500+middle.size(), &*v.begin()+size-500, 500);
		}
		else
		{
			s.resize (size);
			memcpy( &*s.begin(), &*v.begin(), v.size() );
		}

		// Replace '\0' characters
		string::iterator is;
		for ( is=s.begin(); is!=s.end(); ++is )
		{
			// remplace non printable char and % with '?' chat
			if ( ! isprint((uint8)(*is)) || (*is) == '%')
			{
				(*is) = '?';
			}
		}
	}
/*
	if ( ! v.empty() )
	{
		// Copy contents
		s.resize( v.size() );
		memcpy( &*s.begin(), &*v.begin(), v.size() );

		// Replace '\0' characters
		string::iterator is;
		for ( is=s.begin(); is!=s.end(); ++is )
		{
			// remplace non printable char and % with '?' chat
			if ( ! isprint((*is)) || (*is) == '%')
			{
				(*is) = '?';
			}
		}
	}
*/	return s;
}

sint smprintf ( char *buffer, size_t count, const char *format, ... )
{
	sint ret;

	va_list args;
	va_start( args, format );
	ret = _VSNPRINTFL( buffer, count, format, args );
	if ( ret == -1 )
	{
		buffer[count-1] = '\0';
	}
	va_end( args );

	return( ret );
}

sint smprintf ( ucchar *buffer, size_t count, const ucchar *format, ... )
{
	sint ret;

	va_list args;
	va_start( args, format );
	ret = _VSNWPRINTFL( buffer, count, format, args );
	if ( ret == -1 )
	{
		buffer[count - 1] = L'\0';
	}
	va_end( args );

	return( ret );
}

sint64 atoiInt64 (const char *ident, sint64 base)
{
	sint64 number = 0;
	bool neg = false;

	// NULL string
	sipassert (ident != NULL);

	// empty string
	if (*ident == '\0') goto end;
	
	// + sign
	if (*ident == '+') ident++;

	// - sign
	if (*ident == '-') { neg = true; ident++; }

	while (*ident != '\0')
	{
		if (isdigit((unsigned char)*ident))
		{
			number *= base;
			number += (*ident)-'0';
		}
		else if (base > 10 && islower((unsigned char)*ident))
		{
			number *= base;
			number += (*ident)-'a'+10;
		}
		else if (base > 10 && isupper((unsigned char)*ident))
		{
			number *= base;
			number += (*ident)-'A'+10;
		}
		else
		{
			goto end;
		}
		ident++;
	}
end:
	if (neg) number = -number;
	return number;
}

void itoaInt64 (sint64 number, char *str, sint64 base)
{
	str[0] = '\0';
	char b[256];
	if(!number)
	{
		str[0] = '0';
		str[1] = '\0';
		return;
	}
	memset(b,'\0',255);
	memset(b,'0',64);
	sint n;
	sint64 x = number;
	if (x < 0) x = -x;
	char baseTable[] = "0123456789abcdefghijklmnopqrstuvwyz";
	for(n = 0; n < 64; n ++)
	{
		sint num = (sint)(x % base);
		b[64 - n] = baseTable[num];
		if(!x) 
		{
			int k;
			int j = 0;
	
			if (number < 0)
			{
				str[j++] = '-';
			}

			for(k = 64 - n + 1; k <= 64; k++) 	
			{
				str[j ++] = b[k];
			}
			str[j] = '\0';
			break;
		}
		x /= base;
	}
}

//! Converts string to an integer.
/*! String to be converted must contain a string representation of numerical
*  value in range of sint32.
*  String format may be in decimal, octal, binary or hexadecimal. There may
*  be an optional '-' or '+' sign prefix. Values that have prefix 0o are considered octal,
*  and values prefixed by 0x or # are considered hexadecimal, and values prefixed
*  by 0b are considered binary.
* \note Result is zero if the argument string is not valid. Also aSuccess is zero.
* \param aString string to return as a number
* \param aSuccess a pointer to an bool8 that indicates if the parsing was successful.
* \return 0 if aString is NULL.
* \return numerical value the aString represents.
*/
sint atoint(const char *aString, bool *aSuccess)
{
	if ( !aString )
		return 0;

	char *ch = NULL, *str = NULL, *pEndStr = NULL;
	sint	nValue = 0, nSign = 1, nBase = 10, nLen = (sint)strlen(aString);
	bool	bSuccess = false;

	if ( nLen <= 0 )
	{
		if ( aSuccess )
			*aSuccess = false;
		return 0;
	}

	str = strdup(aString);
	ch = str;

	//! remove whitespace in string.
	for ( sint i = 0; i < nLen; i ++ )
	{
		if ( *ch == ' ' )
			memcpy((void *)ch, &ch[1], (nLen - i - 1));
		else
			ch ++;
	}
	*ch = '\0';

	ch = str;
	while ( strlen(ch) )
	{
		if ( *ch == '-' )
		{//! set minus signal.
			nSign = -1;
			ch ++;
		}

		if ( *ch == '0' )
		{
			ch ++;
			if ( *ch == 'X' || *ch == 'x' )
			{
				nBase = 16;
				ch ++;
			}
			else if ( *ch == 'O' || *ch == 'o' )
			{
				nBase = 8;
				ch ++;
			}
			else if ( *ch == 'B' || *ch == 'b' )
			{
				nBase = 2;
				ch ++;
			}
			else
			{
				nBase = 10;
				ch --;
			}
		}
		else if ( *ch == '#' )
		{
			nBase = 16;
			ch ++;
		}
		else
			nBase = 10;

		nValue += (sint)(strtol(ch, &pEndStr, nBase) * nSign);

		if ( ch == pEndStr )//! failed.
			break;

		ch = pEndStr;
		bSuccess = true;
		nSign = 1;
	}

	free(str);

	if ( aSuccess )
		*aSuccess = bSuccess;

	if ( !bSuccess )
		return 0;
	else
		return nValue;
}

sint atoint(const ucchar *aString, bool *aSuccess)
{
	if ( !aString )
		return 0;

	ucchar *ch = NULL, *str = NULL, *pEndStr = NULL;
	sint	nValue = 0, nSign = 1, nBase = 10, nLen = (sint)wcslen(aString);
	bool	bSuccess = false;

	if ( nLen <= 0 )
	{
		if ( aSuccess )
			*aSuccess = false;
		return 0;
	}

//	str = wcsdup(aString);
//-- lci 09-11-28 for mac
//	wcsdup is not POSIX standard, is a GNU extension
	int len = wcslen(aString) + 1;
	str = (ucchar*) malloc(sizeof(ucchar) * len);
//----
	
	ch = str;

	//! remove whitespace in string.
	for ( sint i = 0; i < nLen; i ++ )
	{
		if ( *ch == L' ' )
			memcpy((void *)ch, &ch[1], sizeof(ucchar) * (nLen - i - 1));
		else
			ch ++;
	}
	*ch = L'\0';
	ch = str;

	while ( wcslen(ch) )
	{
		if ( *ch == L'-' )
		{//! set minus signal.
			nSign = -1;
			ch ++;
		}

		if ( *ch == L'0' )
		{
			ch ++;
			if ( *ch == L'X' || *ch == L'x' )
			{
				nBase = 16;
				ch ++;
			}
			else if ( *ch == L'O' || *ch == L'o' )
			{
				nBase = 8;
				ch ++;
			}
			else if ( *ch == L'B' || *ch == L'b' )
			{
				nBase = 2;
				ch ++;
			}
			else
			{
				nBase = 10;
				ch --;
			}
		}
		else if ( *ch == L'#' )
		{
			nBase = 16;
			ch ++;
		}
		else
			nBase = 10;

		nValue += (sint32)(wcstol(ch, &pEndStr, nBase) * nSign);

		if ( ch == pEndStr )//! failed.
			break;

		ch = pEndStr;
		bSuccess = true;
		nSign = 1;
	}

	free(str);

	if ( aSuccess )
		*aSuccess = bSuccess;

	if ( !bSuccess )
		return 0;
	else
		return nValue;
}

//! Converts string to a float.
/*! String to be converted must contain a string representation of numerical
*  value in range of float32. The string can contain only characters '0' to '9'
*  with optionally '-' or '+' sign on the front, '.' as decimal separator and the exponent
*  symbol 'e' or 'E' possibly suffixed with the '+' or '-' sign.
* \note Result is zero if the argument string is not valid. Also aSuccess is zero.
* \param aString string to return as a number
* \param aSuccess a pointer to an bool8 that indicates if the parsing was successful.
* \return 0 if aString is NULL.
* \return numerical value the aString represents.
*/
float atofloat(const char *aString, bool *aSuccess)
{
	float fValue = 0.0f;
	bool  bSuccess = false;

	//후에 검사해보아\EC\95?한다.
	if ( !aString )
	{
		if ( aSuccess )
			*aSuccess = false;
		return fValue;
	}

	if ( strlen(aString) <= 0 )
	{
		if ( aSuccess )
			*aSuccess = false;
		return fValue;
	}

	fValue = (float)atof(aString);
	bSuccess = true;

	if ( aSuccess )
		*aSuccess = bSuccess;

	return fValue;
}

float atofloat(const ucchar *aString, bool *aSuccess)
{
	float fValue = 0.0f;
	bool  bSuccess = false;

	if ( !aString )
	{
		if ( aSuccess )
			*aSuccess = false;
		return fValue;
	}

	if ( wcslen(aString) <= 0 )
	{
		if ( aSuccess )
			*aSuccess = false;
		return fValue;
	}

	fValue = (float)_WTOF_L(aString);
	bSuccess = true;

	if ( aSuccess )
		*aSuccess = bSuccess;

	return fValue;
}

//! Converts string to a bool.
bool atob(const char *aString, bool *aSuccess)
{
	bool	bValue = false;
	bool	bSuccess = false;

	if ( !aString )
	{
		if ( aSuccess )
			*aSuccess = false;
		return bValue;
	}

	if ( strlen(aString) <= 0 )
	{
		if ( aSuccess )
			*aSuccess = false;
		return bValue;
	}

	if ( sipstricmp(aString, "true") == 0 )
	{
		bValue = true;
		bSuccess = true;
	}
	else if ( sipstricmp(aString, "false") == 0 )
	{
		bValue = false;
		bSuccess = true;
	}
	else
		bSuccess = false;

	if ( aSuccess )
		*aSuccess = bSuccess;

	return bValue;
}

bool atob(const ucchar *aString, bool *aSuccess)
{
	bool	bValue = false;
	bool	bSuccess = false;

	if ( !aString )
	{
		if ( aSuccess )
			*aSuccess = false;
		return bValue;
	}

	if ( wcslen(aString) <= 0 )
	{
		if ( aSuccess )
			*aSuccess = false;
		return bValue;
	}

	if ( sipstricmp(aString, L"true") == 0 )
	{
		bValue = true;
		bSuccess = true;
	}
	else if ( sipstricmp(aString, L"false") == 0 )
	{
		bValue = false;
		bSuccess = true;
	}
	else
		bSuccess = false;

	if ( aSuccess )
		*aSuccess = bSuccess;

	return bValue;
}

uint raiseToNextPowerOf2(uint v)
{
	uint	res=1;
	while(res<v)
		res<<=1;
	
	return res;
}

uint	getPowerOf2(uint v)
{
	uint	res=1;
	uint	ret=0;
	while(res<v)
	{
		ret++;
		res<<=1;
	}
	
	return ret;
}

bool isPowerOf2(sint32 v)
{
	while(v)
	{
		if(v&1)
		{
			v>>=1;
			if(v)
				return false;
		}
		else
			v>>=1;
	}

	return true;
}

string bytesToHumanReadable (const std::string &bytes)
{
	static const char *divTable[]= { "B", "KB", "MB", "GB" };
	uint div = 0;
	uint64 res = atoiInt64(bytes.c_str());
	uint64 newres = res;
	while (true)
	{
		newres /= 1024;
		if(newres < 8 || div > 2)
			break;
		div++;
		res = newres;
	}
	return toString ("%" SIP_I64 "u%s", res, divTable[div]);
}

string bytesToHumanReadable (uint32 bytes)
{
	static const char *divTable[]= { "B", "KB", "MB", "GB" };
	uint div = 0;
	uint32 res = bytes;
	uint32 newres = res;
	while (true)
	{
		newres /= 1024;
		if(newres < 8 || div > 2)
			break;
		div++;
		res = newres;
	}
	return toString ("%u%s", res, divTable[div]);
}

uint32 humanReadableToBytes (const string &str)
{
	uint32 res;

	if(str.empty())
		return 0;

	// not a number
	if(str[0]<'0' || str[0]>'9')
		return 0;

	res = atoi (str.c_str());

	if(str[str.size()-1] == 'B')
	{
		if (str.size()<3)
			return res;

		// there s no break and it's **normal**
		switch (str[str.size()-2])
		{
		case 'G': res *= 1024;
		case 'M': res *= 1024;
		case 'K': res *= 1024;
		default: ;
		}
	}

	return res;
}


SIPBASE_CATEGORISED_COMMAND(sip, btohr, "Convert a bytes number into an human readable number", "<int>")
{
	if (args.size() != 1)
		return false;
	
	log.displayNL("%s -> %s", args[0].c_str(), bytesToHumanReadable(args[0]).c_str());

	return true;
}


SIPBASE_CATEGORISED_COMMAND(sip, hrtob, "Convert a human readable number into a bytes number", "<hr>")
{
	if (args.size() != 1)
		return false;
	
	log.displayNL("%s -> %u", args[0].c_str(), humanReadableToBytes(args[0]));
	
	return true;
}


string secondsToHumanReadable (uint32 time)
{
	static const char *divTable[] = { "s", "mn", "h", "d" };
	static uint  divCoef[]  = { 60, 60, 24 };
	uint div = 0;
	uint32 res = time;
	uint32 newres = res;
	while (true)
	{
		if(div > 2)
			break;

		newres /= divCoef[div];
		
		if(newres < 3)
			break;

		div++;
		res = newres;
	}
	return toString ("%u%s", res, divTable[div]);
}

uint32 fromHumanReadable (const std::string &str)
{
	if (str.size() == 0)
		return 0;

	uint32 val = atoi (str.c_str());

	switch (str[str.size()-1])
	{
	case 's': return val;			// second
	case 'n': return val*60;		// minutes (mn)
	case 'h': return val*60*60;		// hour
	case 'd': return val*60*60*24;	// day
	case 'b':	// bytes
		switch (str[str.size()-2])
		{
		case 'k': return val*1024;
		case 'm': return val*1024*1024;
		case 'g': return val*1024*1024*1024;
		default : return val;
		}
	default: return val;
	}
	return 0;
}


SIPBASE_CATEGORISED_COMMAND(sip, stohr, "Convert a second number into an human readable time", "<int>")
{
	if (args.size() != 1)
		return false;
	
	log.displayNL("%s -> %s", args[0].c_str(), secondsToHumanReadable(atoi(args[0].c_str())).c_str());
	
	return true;
}

//! Returns length of string, in bytes.
/*! String must be zero-terminated.
 * \sa getLengthInCodepoints
 */
sint getLength(const char *aString)
{
	if ( !aString )
		return 0;

	return strlen(aString);
}

sint getLength(const ucchar *aString)
{
	if ( !aString )
		return 0;

	return wcslen(aString);
}

//! Format() call that also allocates the target buffer.
/*! \note Application must delete[] the buffer after use.
 *  \sa format()
 */
char *allocatedFormat(const char *aFormat, ...)
{
	if ( !aFormat )
		return NULL;

	char *buf = NULL;
	sint nLen = 0;

	va_list args;
	va_start( args, aFormat );
	nLen = _VSCPRINTF_L( aFormat, args );

	buf = new char[nLen + 1];
	memset(buf, 0, (nLen + 1));

	vsprintf( buf, aFormat, args );
	va_end(args);

	return buf;
}

ucchar *allocatedFormat(const ucchar *aFormat, ...)
{
	if ( !aFormat )
		return NULL;

	ucchar *buf = NULL;
	sint nLen = 0;

	va_list args;
	va_start( args, aFormat );
	nLen = _VSCWPRINTF_L( aFormat, args );

	buf = new ucchar[nLen + 1];
	memset(buf, 0, sizeof(ucchar) * (nLen + 1));

	_VSWPRINTF_L( buf, nLen + 1, aFormat, args );
	va_end(args);

	return buf;
}

/// Format() call that also allocates the target buffer (va_list version).
/*! \note Application must delete[] the buffer after use.
 *  \sa format()
 */
char *allocatedFormatVarArgs(const char *aFormat, ...)
{
	if ( !aFormat )
		return NULL;

	char *buf = new char[MaxCStringSize];
	memset(buf, 0, MaxCStringSize);

	va_list aVaList;
	va_start(aVaList, aFormat);
	vsprintf(buf, aFormat, aVaList);
	va_end(aVaList);

	return buf;
}

ucchar *allocatedFormatVarArgs(const ucchar *aFormat, ...)
{
	if ( !aFormat )
		return NULL;

	ucchar *buf = new ucchar[MaxCStringSize];
	memset(buf, 0, sizeof(ucchar) * MaxCStringSize);

	va_list aVaList;
	va_start(aVaList, aFormat);
	_VSWPRINTF_L(buf, MaxCStringSize, aFormat, aVaList);
	va_end(aVaList);

	return buf;
}

/// 문자렬을 복사하는 함수. alen \EC\9D?0이면 모두 복사.
// * 주의: 새로 기억구역\EC\9D?할당하지 않는\EB\8B?
char *copy(char *aStrOut, const char *aStrIn, sint alen)
{
	if (alen <= 0)
	{
		strcpy(aStrOut, aStrIn);
		aStrOut[strlen(aStrIn)] = '\0';
	}
	else
	{
		strncpy(aStrOut, aStrIn, alen);
		aStrOut[ min(strlen(aStrIn), (size_t)alen) ] = '\0';
	}
	return aStrOut;
}

ucchar *copy(ucchar *aStrOut, const ucchar *aStrIn, sint alen)
{
	if (alen <= 0)
	{
		wcscpy(aStrOut, aStrIn);
		aStrOut[wcslen(aStrIn)] = L'\0';
	}
	else
	{
		wcsncpy(aStrOut, aStrIn, alen);
		aStrOut[ min(wcslen(aStrIn), (size_t)alen) ] = L'\0';
	}
	return aStrOut;
}

/// Duplicates string from char8 characters to char8.
/*! \note The newly allocated buffer must be cleaned up by the application by using the delete[] operator. */
char *copy(const char *aString)
{
	if ( !aString )
		return NULL;

	sint len = strlen(aString);
	if ( len <= 0 )
		return NULL;

	char *buf = new char[len + 1];
	memset(buf, 0, len + 1);
	strcpy(buf, aString);
	return buf;
}

ucchar *copy(const ucchar *aString)
{
	if ( !aString )
		return NULL;

	sint len = wcslen(aString);
	if ( len <= 0 )
		return NULL;

	ucchar *buf = new ucchar[len + 1];
	memset(buf, 0, sizeof(ucchar) * (len + 1));
	wcscpy(buf, aString);
	return buf;
}

/// 문자렬을 덧추가하는 함수. alen \EC\9D?0이면 모두 덧붓\EC\9E?
// * 주의: 새로 기억구역\EC\9D?할당하지 않는\EB\8B?
char *cat(char *aStrOut, const char *aStrIn,  sint alen)
{
	size_t nOldLen = strlen(aStrOut);

	if (alen <= 0)
	{
		strcat(aStrOut, aStrIn);
		aStrOut[strlen(aStrIn) + nOldLen] = '\0';
	}
	else
	{
		strncat(aStrOut, aStrIn, alen);
		aStrOut[ min(strlen(aStrIn), (size_t)alen) + nOldLen ] = '\0';
	}
	return aStrOut;
}

ucchar *cat(ucchar *aStrOut, const ucchar *aStrIn,  sint alen)
{
	size_t nOldLen = wcslen(aStrOut);

	if (alen <= 0)
	{
		wcscat(aStrOut, aStrIn);
		aStrOut[wcslen(aStrIn) + nOldLen] = L'\0';
	}
	else
	{
		wcsncat(aStrOut, aStrIn, alen);
		aStrOut[ min(wcslen(aStrIn), (size_t)alen) + nOldLen ] = L'\0';
	}
	return aStrOut;
}

std::string	toLower(const std::string &str)
{
	string res;
	res.reserve(str.size());
	for(uint i = 0; i < str.size(); i++)
	{
		if( (str[i] >= 'A') && (str[i] <= 'Z') )
			res += str[i] - 'A' + 'a';
		else
			res += str[i];
	}
	return res;
}

void		toLower(char *str)
{
	if (str == 0)
		return;

	while(*str != '\0')
	{
		if( (*str >= 'A') && (*str <= 'Z') )
		{
			*str = *str - 'A' + 'a';
		}
		str++;
	}
}

std::string	toUpper(const std::string &str)
{
	string res;
	res.reserve(str.size());
	for(uint i = 0; i < str.size(); i++)
	{
		if( (str[i] >= 'a') && (str[i] <= 'z') )
			res += str[i] - 'a' + 'A';
		else
			res += str[i];
	}
	return res;
}

void		toUpper(char *str)
{
	if (str == 0)
		return;

	while(*str != '\0')
	{
		if( (*str >= 'a') && (*str <= 'z') )
		{
			*str = *str - 'a' + 'A';
		}
		str++;
	}
}


//
// Exceptions
//

Exception::Exception() : _Reason("Unknown Exception"), _ReasonW(L"Unknown Exception")
{
//	sipinfo("Exception will be launched: %s", _Reason.c_str());
}

Exception::Exception(const std::string &reason)
{
	_Reason = reason;
	_ReasonW = SIPBASE::getUcsFromMbs(reason.c_str());
	
	sipinfo("Exception will be launched: %s", _Reason.c_str());
}

Exception::Exception(const std::basic_string<ucchar> &reason)
{
	_ReasonW = reason;
	_Reason = SIPBASE::toString("%S", reason.c_str());

#ifdef SIP_OS_WINDOWS
	sipinfo(L"Exception will be launched: %s", _ReasonW.c_str());
#else
	sipinfo(L"Exception will be launched: %S", _ReasonW.c_str());
#endif
}

Exception::Exception(const char *format, ...)
{
	SIPBASE_CONVERT_VARGS (_Reason, format, SIPBASE::MaxCStringSize);
	_ReasonW = SIPBASE::getUcsFromMbs(_Reason.c_str());

	sipinfo("Exception will be launched: %s", _Reason.c_str());
}

Exception::Exception(const ucchar *format, ...)
{
	SIPBASE_CONVERT_VARGS_WIDE (_ReasonW, format, SIPBASE::MaxCStringSize);
	_Reason = SIPBASE::toString("%S", _ReasonW.c_str());

#ifdef SIP_OS_WINDOWS
	sipinfo(L"Exception will be launched: %s", _ReasonW.c_str());
#else
	sipinfo(L"Exception will be launched: %S", _ReasonW.c_str());
#endif
}

const char *Exception::what() const throw()
{
	return _Reason.c_str();
}

bool killProgram(uint32 pid)
{
#ifdef SIP_OS_UNIX
	int res = kill(pid, SIGKILL);
	if(res == -1)
	{
		char *err = strerror (errno);
		sipwarning("Failed to kill '%d' err %d: '%s'", pid, errno, err);
	}
	return res == 0;
/*#elif defined(SIP_OS_WINDOWS)
	// it doesn't work because pid != handle and i don't know how to kill a pid or know the real handle of another service (not -1)
	int res = TerminateProcess((HANDLE)pid, 888);
	LPVOID lpMsgBuf;
	FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &lpMsgBuf, 0, NULL);
	sipwarning("Failed to kill '%d' err %d: '%s'", pid, GetLastError (), lpMsgBuf);
	LocalFree(lpMsgBuf);
	return res != 0;
*/
#else
	sipwarning("kill not implemented on this OS");
	return false;
#endif
}

bool abortProgram(uint32 pid)
{
#ifdef SIP_OS_UNIX
	int res = kill(pid, SIGABRT);
	if(res == -1)
	{
		char *err = strerror (errno);
		sipwarning("Failed to abort '%d' err %d: '%s'", pid, errno, err);
	}
	return res == 0;
#else
	sipwarning("abort not implemented on this OS");
	return false;
#endif
}

bool launchProgram (const std::string &programName, const std::string &arguments)
{

#ifdef SIP_OS_WINDOWS
	STARTUPINFOA        si;
    PROCESS_INFORMATION pi;
	
    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
	
    si.cb = sizeof(si);
	
/*	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof (sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = FALSE;

	STARTUPINFO si;
	si.cb = sizeof (si);
	si.lpReserved = NULL;
	si.lpDesktop = NULL;
	si.lpTitle = NULL;
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.cbReserved2 = 0;
	si.wShowWindow = SW_MINIMIZE;
	si.lpReserved2 = NULL;

	PROCESS_INFORMATION pi;
*/

	// Enable sipassert/sipstop to display the error reason & callstack
	const char *SE_TRANSLATOR_IN_MAIN_MODULE = "SIP_SE_TRANS";
	char envBuf [2];
	if ( GetEnvironmentVariableA( SE_TRANSLATOR_IN_MAIN_MODULE, envBuf, 2 ) != 0)
	{
		SetEnvironmentVariableA( SE_TRANSLATOR_IN_MAIN_MODULE, NULL );
	}
	
//	string arg = " " + arguments;
	string arg = arguments;
	BOOL res;
	if (programName == "")
		res = CreateProcessA(NULL, (char*)arg.c_str(), 0, 0, FALSE, CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW, 0, 0, &si, &pi);
	else
		res = CreateProcessA(programName.c_str(), (char*)arg.c_str(), 0, 0, FALSE, CREATE_DEFAULT_ERROR_MODE | CREATE_NO_WINDOW, 0, 0, &si, &pi);

	if (res)
	{
		sipdebug("LAUNCH: Successful launch '%s' with arg '%s'", programName.c_str(), arguments.c_str());
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		return true;
	}
	else
	{
		LPVOID lpMsgBuf;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &lpMsgBuf, 0, NULL);
		sipwarning("LAUNCH: Failed launched '%s' with arg '%s' err %d: '%s'", programName.c_str(), arguments.c_str(), GetLastError (), lpMsgBuf);
		LocalFree(lpMsgBuf);
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
	}

#elif defined(SIP_OS_UNIX)
	static bool firstLaunchProgram = true;
	if (firstLaunchProgram)
	{
		// The aim of this is to avoid defunct process.
		//
		// From "man signal":
		//------
		// According to POSIX (3.3.1.3) it is unspecified what happens when SIGCHLD is set to SIG_IGN.   Here
		// the  BSD  and  SYSV  behaviours  differ,  causing BSD software that sets the action for SIGCHLD to
		// SIG_IGN to fail on Linux.
		//------
		//
		// But it works fine on my GNU/Linux so I do this because it's easier :) and I don't know exactly
		// what to do to be portable.
		signal(SIGCHLD,SIG_IGN);
		
		firstLaunchProgram = false;
	}

	int status = fork ();
	if (status == -1)
	{
		char *err = strerror (errno);
		sipwarning("LAUNCH: Failed launched '%s' with arg '%s' err %d: '%s'", programName.c_str(), arguments.c_str(), errno, err);
	}
	else if (status == 0)
	{
		// convert one arg into several args
		vector<string> args;
		string::size_type pos_s = 0, pos_e = 0;
		while (true)
		{
			pos_s = arguments.find_first_not_of(" ", pos_s);
			if (pos_s == string::npos)
				break;

			pos_e = arguments.find_first_of(" ", pos_s);
			if (pos_e == string::npos)
			{
				args.push_back (arguments.substr (pos_s));
				break;
			}
			else
			{
				args.push_back (arguments.substr (pos_s, pos_e - pos_s));
				pos_s = pos_e;
			}
		}

		sint8 argvSize = args.size();
		string exec_path_name;
		if (programName.length() == 0)
		{
			exec_path_name = args[0];
		}
		else
		{
			argvSize++;
			exec_path_name = programName;
		}

		// Store the size of each arg
		vector<char *> argv(argvSize + 1);
		argv[0] = (char*)exec_path_name.c_str();
		
		int i;
		std::string strArgv;
		for (i = 1; i < argvSize; i++)
		{
			argv[i] = (char*)args[i].c_str();
			strArgv.append(argv[i]);
			strArgv.append(" ");
		}
		argv[i+1] = NULL;

		// Exec
		status = execvp(exec_path_name.c_str(), &argv.front());

		if (status == -1)
		{
			sipwarning("LAUNCH: Failed launched '%s' with arg '%s'", exec_path_name.c_str(), strArgv.c_str());
			perror("Failed launched");
			_exit(EXIT_FAILURE);
		}
		else
		{
			sipdebug("LAUNCH: Successful launch '%s' with arg '%s'", exec_path_name.c_str(), strArgv.c_str());
			return true;
		}
	}
#else
	sipwarning ("LAUNCH: launchProgram() not implemented");
#endif

	return false;

}

void explode (const std::string &src, const std::string &sep, std::vector<std::string> &res, bool skipEmpty)
{
	string::size_type oldpos = 0, pos;

	res.clear ();

	do
	{
		pos = src.find (sep, oldpos);
		string s;
		if(pos == string::npos)
			s = src.substr (oldpos);
		else
			s = src.substr (oldpos, (pos-oldpos));

		if (!skipEmpty || !s.empty())
			res.push_back (s);

			oldpos = pos+sep.size();
	}
	while(pos != string::npos);

	// debug
/*	sipinfo ("Exploded '%s', with '%s', %d res", src.c_str(), sep.c_str(), res.size());
	for (uint i = 0; i < res.size(); i++)
	{
		sipinfo (" > '%s'", res[i].c_str());
	}
*/
}


/*
 * Display the bits (with 0 and 1) composing a byte (from right to left)
 */
void displayByteBits( uint8 b, uint nbits, sint beginpos, bool displayBegin, SIPBASE::ILog *log0 )
{
	CLog *log = (CLog *)log0;

	string s1, s2;
	sint i;
	for ( i=nbits-1; i!=-1; --i )
	{
		s1 += ( (b >> i) & 1 ) ? '1' : '0';
	}
	log->displayRawNL( "%s", s1.c_str() );
	if ( displayBegin )
	{
		for ( i=nbits; i>beginpos+1; --i )
		{
			s2 += " ";
		}
		s2 += "^";
		log->displayRawNL( "%s beginpos=%u", s2.c_str(), beginpos );
	}
}


//#define displayDwordBits(a,b,c)

/*
 * Display the bits (with 0 and 1) composing a number (uint32) (from right to left)
 */
void displayDwordBits( uint32 b, uint nbits, sint beginpos, bool displayBegin, SIPBASE::ILog *log0 )
{
	CLog *log = (CLog *)log0;

	string s1, s2;
	sint i;
	for ( i=nbits-1; i!=-1; --i )
	{
		s1 += ( (b >> i) & 1 ) ? '1' : '0';
	}
	log->displayRawNL( "%s", s1.c_str() );
	if ( displayBegin )
	{
		for ( i=nbits; i>beginpos+1; --i )
		{
			s2 += " ";
		}
		s2 += "^";
		log->displayRawNL( "%s beginpos=%u", s2.c_str(), beginpos );
	}
}


int	sipfseek64( FILE *stream, sint64 offset, int origin )
{
#ifdef SIP_OS_WINDOWS
	
	//
	fpos_t pos64 = 0;
	switch (origin)
	{
	case SEEK_CUR:
		if (fgetpos(stream, &pos64) != 0)
			return -1;
	case SEEK_END:
		pos64 = _filelengthi64(_fileno(stream));
		if (pos64 == -1L)
			return -1;
	};
	
	// Seek
	pos64 += offset;
	
	// Set the final position
	return fsetpos (stream, &pos64);
	
#else // SIP_OS_WINDOWS
	
	// This code doesn't work under windows : fseek() implementation uses a signed 32 bits offset. What ever we do, it can't seek more than 2 Go.
	// For the moment, i don't know if it works under linux for seek of more than 2 Go.
	
	sipassert ((offset < SINT64_CONSTANT(2147483647)) && (offset > SINT64_CONSTANT(-2147483648)));
	
	bool first = true;
	do
	{
		// Get the size of the next fseek
		sint nextSeek;
		if (offset > 0)
			nextSeek = (sint)std::min (SINT64_CONSTANT(2147483647), offset);
		else
			nextSeek = (sint)std::max (-SINT64_CONSTANT(2147483648), offset);
		
		// Make a seek
		int result = fseek ( stream, nextSeek, first?origin:SEEK_CUR );
		if (result != 0)
			return result;
		
		// Remaining
		offset -= nextSeek;
		first = false;
	}
	while (offset);
	
	return 0;
	
#endif // SIP_OS_WINDOWS
}

sint64 sipftell64(FILE *stream)
{
	#ifdef SIP_OS_WINDOWS		
		fpos_t pos64 = 0;
		if (fgetpos(stream, &pos64) == 0)
		{
			return (sint64) pos64;
		}
		else return -1;
	#else
		siperror("Not implemented");
		return -1;
	#endif
}




//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/// Commands
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

SIPBASE_CATEGORISED_COMMAND(sip, sleep, "Freeze the service for N seconds (for debug purpose)", "<N>")
{
	if(args.size() != 1) return false;
	
	sint32 n = atoi (args[0].c_str());
	
	log.displayNL ("Sleeping during %d seconds", n);
	
	sipSleep(n * 1000);	
	return true;
}

SIPBASE_CATEGORISED_COMMAND(sip, system, "Execute the command line using system() function call (wait until the end of the command)", "<commandline>")
{
	if(args.size() != 1) return false;
	
	string cmd = args[0];
	log.displayNL ("Executing '%s'", cmd.c_str());
	system(cmd.c_str());
	log.displayNL ("End of Execution of '%s'", cmd.c_str());
	return true;
}

SIPBASE_CATEGORISED_COMMAND(sip, launchProgram, "Execute the command line using launcProgram() function call (launch in background task without waiting the end of the execution)", "<programName> <arguments>")
{
	if(args.size() != 2) return false;
	
	string cmd = args[0];
	string arg = args[1];
	log.displayNL ("Executing '%s' with argument '%s'", cmd.c_str(), arg.c_str());
	launchProgram(cmd, arg);
	log.displayNL ("End of Execution of '%s' with argument '%s'", cmd.c_str(), arg.c_str());
	return true;
}

SIPBASE_CATEGORISED_COMMAND(sip, killProgram, "kill a program given the pid", "<pid>")
{
	if(args.size() != 1) return false;
	killProgram(atoi(args[0].c_str()));
	return true;
}

#ifdef SIP_OS_WINDOWS
LONG GetRegKey(HKEY key, LPCSTR subkey, LPSTR retdata)
{
    HKEY hkey;
    LONG retval = RegOpenKeyExA(key, subkey, 0, KEY_QUERY_VALUE, &hkey);

    if (retval == ERROR_SUCCESS) 
	{
        long datasize = MAX_PATH;
        char data[MAX_PATH];
        RegQueryValueA(hkey, NULL, data, &datasize);
        lstrcpyA(retdata,data);
        RegCloseKey(hkey);
    }

    return retval;
}
#endif // SIP_OS_WINDOWS

bool openURL (const ucchar *url)
{
	ucstring ucstr = url;
	string str = ucstr.toString();
	return openURL(str.c_str());
}

bool openURL (const char *url)
{
#ifdef SIP_OS_WINDOWS
//    char key[MAX_PATH + MAX_PATH];
//     if (GetRegKey(HKEY_CLASSES_ROOT, "htmlfile", key) == ERROR_SUCCESS) 
// 	{
//         lstrcpyA(key, "\\htmlfile\\shell\\open\\command");
// 
//         if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS) 
// 		{
//             char *pos;
//             pos = strstr(key, "\"%1\"");
//             if (pos == NULL) {                     // No quotes found
//                 pos = strstr(key, "%1");       // Check for %1, without quotes 
//                 if (pos == NULL)                   // No parameter at all...
//                     pos = key+lstrlenA(key)-1;
//                 else
//                     *pos = '\0';                   // Remove the parameter
//             }
//             else
//                 *pos = '\0';                       // Remove the parameter
// 
//             lstrcatA(pos, " ");
//             lstrcatA(pos, url);
//             int res = WinExec(key, SW_SHOWDEFAULT);
// 			return (res>31);
//         }
// 		else
// 		{
// 			int res = (int)::ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOW);
// 			return ( res > 32 );
// 
// 		}
//     }
// 	else
	{
		int res = (int)::ShellExecuteA(NULL, "open", url, NULL, NULL, SW_SHOW);
		return ( res > 32 );

	}
#elif defined(SIP_OS_MAC)
	return launchProgram("open", url);
#elif defined(SIP_OS_UNIX)
	return launchProgram("/etc/alternatives/x-www-browser", url);
#else
	SIPwarning("openURL() is not implemented for this OS");
#endif // SIP_OS_WINDOWS
	return false;
}

bool openDoc (const char *document)
{
#ifdef SIP_OS_WINDOWS
	string ext = CFileA::getExtension (document);
    char key[MAX_PATH + MAX_PATH];

    // First try ShellExecute()
    HINSTANCE result = ShellExecuteA(NULL, "open", document, NULL,NULL, SW_SHOWDEFAULT);

    // If it failed, get the .htm regkey and lookup the program
    if ((UINT)result <= HINSTANCE_ERROR) 
	{
        if (GetRegKey(HKEY_CLASSES_ROOT, ext.c_str(), key) == ERROR_SUCCESS) 
		{
            lstrcatA(key, "\\shell\\open\\command");

            if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS) 
			{
                char *pos;
                pos = strstr(key, "\"%1\"");
                if (pos == NULL) {                     // No quotes found
                    pos = strstr(key, "%1");       // Check for %1, without quotes 
                    if (pos == NULL)                   // No parameter at all...
                        pos = key+lstrlenA(key)-1;
                    else
                        *pos = '\0';                   // Remove the parameter
                }
                else
                    *pos = '\0';                       // Remove the parameter

                lstrcatA(pos, " ");
                lstrcatA(pos, document);
                int res = WinExec(key,SW_SHOWDEFAULT);
				return (res>31);
            }
        }
    }
	else
		return true;
#endif // SIP_OS_WINDOWS
	return false;
}

std::string trim (const std::string &str)
{
	uint start = 0;
	const uint size = str.size();
	while (start < size && str[start] <= 32)
		start++;
	uint end = size;
	while (end > start && str[end-1] <= 32)
		end--;
	return str.substr (start, end-start);
}

std::string normalizeString (const std::string &str)
{
	std::string temp;
	int nCount = 0;

	for ( size_t i = 0; i < str.size(); i ++ )
	{
		if ( str[i] <= 32 )
		{
			if ( !nCount )
			{
				temp.push_back(' ');
				nCount ++;
			}
		}
		else
		{
			temp.push_back(str[i]);
			nCount = 0;
		}
	}

	return trim(temp);
}

} // SIPBASE
