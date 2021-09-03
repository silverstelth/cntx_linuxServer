/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/string_common.h"
#include "misc/ucstring.h"
using namespace std;

namespace SIPBASE
{

string addSlashR (string str)
{
	string formatedStr;
	// replace \n with \r\n
	for (uint i = 0; i < str.size(); i++)
	{
		if (str[i] == '\n' && i > 0 && str[i-1] != '\r')
		{
			formatedStr += '\r';
		}
		formatedStr += str[i];
	}
	return formatedStr;
}

std::basic_string<ucchar> addSlashR_W (std::basic_string<ucchar> str)
{
	std::basic_string<ucchar> formatedStr;
	// replace \n with \r\n
	for (uint i = 0; i < str.size(); i++)
	{
		if (str[i] == L'\n' && i > 0 && str[i-1] != L'\r')
		{
			formatedStr += L'\r';
		}
		formatedStr += str[i];
	}
	return formatedStr;
}

string removeSlashR (string str)
{
	string formatedStr;
	// remove \r
	for (uint i = 0; i < str.size(); i++)
	{
		if (str[i] != '\r')
			formatedStr += str[i];
	}
	return formatedStr;
}

std::basic_string<ucchar> removeSlashR_W (std::basic_string<ucchar> str)
{
	std::basic_string<ucchar> formatedStr;
	// remove \r
	for (uint i = 0; i < str.size(); i++)
	{
		if (str[i] != L'\r')
			formatedStr += str[i];
	}
	return formatedStr;
}

//-----add by LCI 091017 start
// 2¸ÆËËÀâ Format·Í»ôµÛ¼¿»õ´ÉËË È¸¶Á´ÅËæ Ê¯¼­Â×¼è ·ÃÂÙ ·Í½£°¡ ËØ³Þ.
// »¨¶Êµá SIPBASE_CONVERT_VARGS_WIDE¶ë¿ÍµáÊ¯Ëæº· ¶®Ë¦Â×²÷ vswprintf¶¦ ¹¾µçÂ×ÊÞ
// mbstowcsÂÜºã´ÐËË ³Þ¼ÚÈ¸¶Á´Å¼¿»õËæº· ÊïË±¶¦ ¸Ë»¶»¤¿Ö³Þ.
// 2¸ÆËËÀâ format·Í»ôµÛ¼¿»õË¾ ºãÃÔÂ×²÷ ¸Ò¸âËºµáº· Ê­¶·Ì© ÂÜºã´ÉË¾ ¶®Ë¦Â×ÊÞ Ë§ÃäÂ×´ªµâ ÂÙ³Þ.

std::basic_string<ucchar> getUcsFromMbs(const char * mbs)
{
#ifdef SIP_OS_WINDOWS
	return toString(L"%S", mbs);
#else  //SIP_OS_WINDOWS
	return ucstring(mbs);
#endif //SIP_OS_WINDOWS
}

std::basic_string<ucchar> toStringLW(const char* format, ...)
{
	char* Result;

	SIPBASE_CONVERT_VARGS(Result, format, SIPBASE::MaxCStringSize);

	return 	getUcsFromMbs(Result);
}

//add by LCI 091021
std::string getMbsFromUcs(const ucchar * ucs)
{
	ucstring ucstr = ucs;
	return ucstr.toString();
}

//----- lci add end
}
