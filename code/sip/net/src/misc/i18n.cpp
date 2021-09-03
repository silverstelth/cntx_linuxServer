/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/path.h"
#include "misc/i18n.h"

using namespace std;

namespace SIPBASE {

/*const std::string		CI18N::_LanguageFiles[] = 
{
	std::string("english"),
	std::string("french")
};
*/
const std::string		CI18N::_LanguageCodes[] =
{
	std::string("en"),		// english
	std::string("de"),		// german
	std::string("fr"),		// french
	std::string("wk"),		// work translation
//	std::string("zh-TW"),	// traditional Chinese
//	std::string("zh-CN")	// simplified Chinese
};

const uint				CI18N::_NbLanguages = sizeof(CI18N::_LanguageCodes) / sizeof(std::string);
CI18N::StrMapContainer	CI18N::_StrMap;
bool					CI18N::_StrMapLoaded = false;
const ucstring			CI18N::_NotTranslatedValue("<Not Translated>");
bool					CI18N::_LanguagesNamesLoaded = false;
sint32					CI18N::_SelectedLanguage = -1;
CI18N::ILoadProxy		*CI18N::_LoadProxy = 0;


void CI18N::setLoadProxy(ILoadProxy *loadProxy)
{
	_LoadProxy = loadProxy;
}


void CI18N::load (const std::string &languageCode)
{
//	sipassert (lid < _NbLanguages);
//	sipassert (_LanguagesNamesLoaded);

	uint i;
	for (i=0; i<_NbLanguages; ++i)
	{
		if (_LanguageCodes[i] == languageCode)
			break;
	}

	if (i == _NbLanguages)
	{
		sipwarning("I18N: Unknown language code : %s, defaulting to %s", _LanguageCodes[0].c_str());
		i = 0;
	}

	std::string fileName  = _LanguageCodes[i] + ".uxt";

	_SelectedLanguage = i;

	if (_StrMapLoaded)	_StrMap.clear ();
	else				_StrMapLoaded = true;

	loadFileIntoMap(fileName, _StrMap);	
}

bool CI18N::loadFileIntoMap(const std::string &fileName, StrMapContainer &destMap)
{
	ucstring text;
	// read in the text
	if (_LoadProxy)
		_LoadProxy->loadStringFile(fileName, text);
	else
		readTextFile(fileName, text);
	// remove any comment
	remove_C_Comment(text);

	ucstring::const_iterator first(text.begin()), last(text.end());
	std::string lastReadLabel("nothing");
	
	while (first != last)
	{
		skipWhiteSpace(first, last);
		std::string label;
		ucstring ucs;
		if (!parseLabel(first, last, label))
		{
			sipwarning("I18N: Error reading label field in %s. Stop reading after %s.", fileName.c_str(), lastReadLabel.c_str());
			return false;
		}
		lastReadLabel = label;
		skipWhiteSpace(first, last);
		if (!parseMarkedString('[', ']', first, last, ucs))
		{
			sipwarning("I18N: Error reading text for label %s in %s. Stop reading.", label.c_str(), fileName.c_str());
			return false;
		}

		// ok, a line read.
		std::pair<std::map<std::string, ucstring>::iterator, bool> ret;
		ret = destMap.insert(std::make_pair(label, ucs));
		if (!ret.second)
		{
			sipwarning("I18N: Error in %s, the label %s exist twice !", fileName.c_str(), label.c_str());
		}
		skipWhiteSpace(first, last);
	}

	// a little check to ensure that the lang name has been set.
	StrMapContainer::iterator it(destMap.find("LanguageName"));
	if (it == destMap.end())
	{
		sipwarning("I18N: In file %s, missing LanguageName translation (should be first in file)", fileName.c_str());
	}
	return true;
}


void CI18N::loadFromFilename(const std::string &filename, bool reload)
{
	StrMapContainer destMap;
	if (!loadFileIntoMap(filename, destMap)) 
	{
		return;
	}
	// merge with existing map
	for(StrMapContainer::iterator it = destMap.begin(); it != destMap.end(); ++it)
	{
		if (!reload)
		{
			if (_StrMap.count(it->first))
			{
				sipwarning("I18N: Error in %s, the label %s exist twice !", filename.c_str(), it->first.c_str());
			}
		}
		_StrMap[it->first] = it->second;
	}
}


const ucstring &CI18N::get (const std::string &label)
{
	if (label.empty())
	{
		static ucstring	emptyString;
		return emptyString;
	}

	StrMapContainer::iterator it(_StrMap.find(label));

	if (it != _StrMap.end())
		return it->second;
	static HashSet<string>	missingStrings;
	if (missingStrings.find(label) == missingStrings.end())
	{
		sint32 nblang = sizeof(_LanguageCodes)/sizeof(_LanguageCodes[0]);
		if(_SelectedLanguage < 0 || _SelectedLanguage >= nblang)
		{
			sipwarning("I18N: _SelectedLanguage %d is not a valid language ID, out of array of size %d, can't display the message '%s'", _SelectedLanguage, nblang, label.c_str());
		}
		else
		{
			sipwarning("I18N: The string %s did not exist in language %s (display once)", label.c_str(), _LanguageCodes[_SelectedLanguage].c_str());
		}
		missingStrings.insert(label);
	}

	static ucstring	badString;

	badString = ucstring(std::string("<NotExist:")+label+">");

	return badString;
}

bool CI18N::hasTranslation(const std::string &label)
{
	if (label.empty()) return true;	
	
	StrMapContainer::iterator it(_StrMap.find(label));	
	return it != _StrMap.end();		
}


ucstring CI18N::getCurrentLanguageName ()
{
	return get("LanguageName");
}


void CI18N::remove_C_Comment(ucstring &commentedString)
{
	{
		ucstring temp;
		temp.reserve(commentedString.size());
		ucstring::const_iterator first(commentedString.begin()), last(commentedString.end());
		for (;first != last; ++first)
		{
			temp.push_back(*first);
			if (*first == '[')
			{
				// no comment inside string literal
				while (++first != last)
				{
					temp.push_back(*first);
					if (*first == ']')
						break;
				}
			}
			else if (*first == '/')
			{
				// start of comment ?
				++first;
				if (first != last && *first == '/')
				{
#if (defined(SIP_OS_WINDOWS) && !defined(_STLPORT_VERSION))
					temp.erase(temp.end() - 1);
#else
					temp.pop_back();
#endif
					// one line comment, skip until end of line
					while (first != last && *first != '\n')
						++first;
				}
				else if (first != last && *first == '*')
				{
#if (defined(SIP_OS_WINDOWS) && !defined(_STLPORT_VERSION))
					temp.erase(temp.end() - 1);
#else
					temp.pop_back();
#endif
					// start of multiline comment, skip until we found '*/'
					while (first != last && !(*first == '*' && (first+1) != last && *(first+1) == '/'))
						++first;
					// skip the closing '/'
					if (first != last)
						++first;
				}
				else
				{
					temp.push_back(*first);
				}
			}
		}

		commentedString.swap(temp);
	}
}


void	CI18N::skipWhiteSpace(ucstring::const_iterator &it, ucstring::const_iterator &last, ucstring *storeComments)
{
	while (it != last &&
			(
					*it == 0xa
				||	*it == 0xd
				||	*it == ' '
				||	*it == '\t'
				||	(storeComments && *it == '/' && it+1 != last && *(it+1) == '/')
				||	(storeComments && *it == '/' && it+1 != last && *(it+1) == '*')
			))
	{
		if (storeComments && *it == '/' && it+1 != last && *(it+1) == '/')
		{
			// found a one line C comment. Store it until end of line.
			while (it != last && *it != '\n')
				storeComments->push_back(*it++);
			// store the final '\n'
			if (it != last)
				storeComments->push_back(*it++);
		}
		else if (storeComments && *it == '/' && it+1 != last && *(it+1) == '*')
		{
			// found a multiline C++ comment. store until we found the closing '*/'
			while (it != last && !(*it == '*' && it+1 != last && *(it+1) == '/'))
				storeComments->push_back(*it++);
			// store the final '*'
			if (it != last)
				storeComments->push_back(*it++);
			// store the final '/'
			if (it != last)
				storeComments->push_back(*it++);
			// and a new line.
			storeComments->push_back('\r');
			storeComments->push_back('\n');
		}
		else
		{
			// just skip white space or don't store comments
			++it;
		}
	}
}

bool CI18N::parseLabel(ucstring::const_iterator &it, ucstring::const_iterator &last, std::string &label)
{
	label.erase();

	// first char must be A-Za-z@_
	if (it != last && 
			(
				(*it >= '0' && *it <= '9')
			||	(*it >= 'A' && *it <= 'Z')
			||	(*it >= 'a' && *it <= 'z')
			||	(*it == '_')
			||	(*it == '@')
			)
		)
		label.push_back(char(*it++));
	else
		return false;

	// other char must be [0-9A-Za-z@_]*
	while (it != last && 
			(
				(*it >= '0' && *it <= '9')
			||	(*it >= 'A' && *it <= 'Z')
			||	(*it >= 'a' && *it <= 'z')
			||	(*it == '_')
			||	(*it == '@')
			)
		)
		label.push_back(char(*it++));

	return true;
}

bool CI18N::parseMarkedString(ucchar openMark, ucchar closeMark, ucstring::const_iterator &it, ucstring::const_iterator &last, ucstring &result)
{
//		ucstring ret;
	result.erase();
	// parse a string delimited by the specified opening and closing mark

	if (it != last && *it == openMark)
	{
		++it;

		while (it != last && *it != closeMark)
		{
			// ignore tab, new lines and line feed
			if (*it == openMark)
			{
				sipwarning("I18N: Found a non escaped openmark %c in a delimited string (Delimiters : '%c' - '%c')", char(openMark), char(openMark), char(closeMark));
				return false;
			}
			if (*it == '\t' || *it == '\n' || *it == '\r')
				++it;
			else if (*it == '\\' && it+1 != last && *(it+1) != '\\')
			{
				++it;
				// this is an escape sequence !
				switch(*it)
				{
				case 't':
					result.push_back('\t');
					break;
				case 'n':
					result.push_back('\n');
					break;
				case 'd':
					// insert a delete
					result.push_back(8);
					break;
				default:
					// escape the close mark ?
					if(*it == closeMark)
						result.push_back(closeMark);
					// escape the open mark ?
					else if(*it == openMark)
						result.push_back(openMark);
					else
					{
						sipwarning("I18N: Ignoring unknown escape code \\%c (char value : %u)", char(*it), *it);
						return false;
					}
				}
				++it;
			}
			else if (*it == '\\' && it+1 != last && *(it+1) == '\\')
			{
				// escape the \ char
				++it;
				result.push_back(*it);
				++it;
			}
			else
				result.push_back(*it++);
		}

		if (it == last || *it != closeMark)
		{
			sipwarning("I18N: Missing end of delimited string (Delimiters : '%c' - '%c')", char(openMark), char(closeMark));
			return false;
		}
		else
			++it;
	}
	else
	{
		sipwarning("I18N: Malformed or non existent delimited string (Delimiters : '%c' - '%c')", char(openMark), char(closeMark));
		return false;
	}

	return true;
}


void CI18N::readTextFile(const std::string &filename, 
						 ucstring &result, 
						 bool forceUtf8, 
						 bool fileLookup, 
						 bool preprocess,
						 TLineFormat lineFmt)
{
	ucstring ucstr;
	ucstr.fromUtf8(filename);
	readTextFile(ucstr, result, forceUtf8, fileLookup, preprocess, lineFmt);
}

void CI18N::readTextFile(const std::basic_string<ucchar> &filename, 
						 ucstring &result, 
						 bool forceUtf8, 
						 bool fileLookup, 
						 bool preprocess,
						 TLineFormat lineFmt)
{
	std::basic_string<ucchar> fullName;
	if (fileLookup)
		fullName = CPath::lookup(filename, false);
	else
		fullName = filename;

	if (fullName.empty())
		return;

	// If ::lookup is used, the file can be in a bnp and CFileA::fileExists fails.
	// \todo Boris
	bool isInBnp = fullName.find(L'@') != string::npos;
	if (!isInBnp && !CFileW::fileExists(fullName))
	{
		sipwarning(L"CI18N::readTextFile : file '%s' does not exist, returning empty string", fullName.c_str());
		return;
	}

	SIPBASE::CIFile	file(fullName);

	// Fast read all the text in binary mode.
	std::string text;
	text.resize(file.getFileSize());
	if (file.getFileSize() > 0)
		file.serialBuffer((uint8*)(&text[0]), text.size());

	// Transform the string in ucstring according to format header
	if (!text.empty())
		readTextBuffer((uint8*)&text[0], text.size(), result, forceUtf8);

	if (preprocess)
	{
		ucstring final;
		// parse the file, looking for preprocessor command.
		ucstring::size_type pos = 0;
		ucstring::size_type lastPos = 0;
		ucstring includeCmd = L"#include";

		while ((pos = result.find(includeCmd, pos)) != ucstring::npos)
		{
			// check that the #include come first on the line (only white space before)
			if (pos != 0)
			{
				uint i=0;
				while ((pos-i-1) > 0 && (result[pos-i-1] == L' ' || result[pos-i-1] == L'\t'))
					++i;
				if (pos-i > 0 && result[pos-i-1] != L'\n' && result[pos-i-1] != L'\r')
				{
					// oups, not at start of line, ignore the #include
					pos = pos + includeCmd.size();
					continue;
				}
			}
			// copy the previous text
			final += result.substr(lastPos, pos - lastPos);

			// extract the inserted file name.
			ucstring::const_iterator first, last;
			first = result.begin()+pos+includeCmd.size();
			last = result.end();

			ucstring name;
			skipWhiteSpace(first, last);
			if (parseMarkedString(L'\"', L'\"', first, last, name))
			{
				basic_string<ucchar> subFilename = name.c_str();
				sipdebug(L"I18N: Including '%s' into '%s'",
					subFilename.c_str(),
					filename.c_str());
				ucstring inserted;

				{
					CIFile testFile;
					if (!testFile.open(subFilename))
					{
						// try to open the include file relative to current file
						subFilename = CFileW::getPath(filename) + subFilename;
					}
				}
				readTextFile(subFilename, inserted, forceUtf8, fileLookup, preprocess);

				final += inserted;
			}
			else
			{
				sipwarning(L"I18N: Error parsing include file in line '%s' from file '%s'",
					result.substr(pos, result.find(L"\n", pos) - pos).c_str(),
					filename.c_str());
			}

			pos = lastPos = first - result.begin();
		}

		// copy the remaining chars
		ucstring temp = final;
		
		for (uint i=lastPos; i<result.size(); ++i)
		{
			temp += result[i];
		}

		result.swap(temp);
	}

	// apply line delimiter conversion if needed
	if (lineFmt != LINE_FMT_NO_CARE)
	{
		if (lineFmt == LINE_FMT_LF)
		{
			// we only want \n
			// easy, just remove or replace any \r code
			ucstring::size_type pos;
			ucstring::size_type lastPos = 0;
			ucstring temp;
			// reserve some place to reduce re-allocation
			temp.reserve(result.size() +result.size()/10);

			// look for the first \r
			pos = result.find(L'\r');
			while (pos != ucstring::npos)
			{
				if (pos < result.size()-1 && result[pos+1] == L'\n')
				{
					temp.append(result.begin()+lastPos, result.begin()+pos);
					pos += 1;
				}
				else
				{
					temp.append(result.begin()+lastPos, result.begin()+pos);
					temp[temp.size()-1] = L'\n';
				}

				lastPos = pos;
				// look for next \r
				pos = result.find(L'\r', pos);
			}

			// copy the rest
			temp.append(result.begin()+lastPos, result.end());

			result.swap(temp);
		}
		else if (lineFmt == LINE_FMT_CRLF)
		{
			// need to replace simple '\n' or '\r' with a '\r\n' double
			ucstring::size_type pos = 0;
			ucstring::size_type lastPos = 0;

			ucstring temp;
			// reserve some place to reduce re-allocation
			temp.reserve(result.size() +result.size()/10);


			// first loop with the '\r'
			pos = result.find(L'\r', pos);
			while (pos != ucstring::npos)
			{
				if (pos >= result.size()-1 || result[pos+1] != L'\n')
				{
					temp.append(result.begin()+lastPos, result.begin()+pos+1);
					temp += L'\n';
					lastPos = pos+1;
				}
				// skip this char
				pos++;

				// look the next '\r'
				pos = result.find(L'\r', pos);
			}

			// copy the rest
			temp.append(result.begin()+lastPos, result.end());
			result.swap(temp);

			temp = L"";

			// second loop with the '\n'
			pos = 0;
			lastPos = 0;
			pos = result.find(L'\n', pos);
			while (pos != ucstring::npos)
			{
				if (pos == 0 || result[pos-1] != L'\r')
				{
					temp.append(result.begin()+lastPos, result.begin()+pos);
					temp += L'\r';
					temp += L'\n';
					lastPos = pos+1;
				}
				// skip this char
				pos++;

				pos = result.find(L'\n', pos);
			}

			// copy the rest
			temp.append(result.begin()+lastPos, result.end());
			result.swap(temp);
		}
	}
}

void CI18N::readTextBuffer(uint8 *buffer, uint size, ucstring &result, bool forceUtf8)
{
	static uint8 utf16Header[] = {(unsigned char)(0xff), (unsigned char)(0xfe)};
	static uint8 utf16RevHeader[] = {(unsigned char)(0xfe), (unsigned char)(0xff)};
	static uint8 utf8Header[] = {(unsigned char)(0xef), (unsigned char)(0xbb), (unsigned char)(0xbf)};

	if (forceUtf8)
	{
		if (size>=3 &&
			buffer[0]==utf8Header[0] && 
			buffer[1]==utf8Header[1] && 
			buffer[2]==utf8Header[2]
			)
		{
			// remove utf8 header
			buffer+= 3;
			size-=3;
		}
		std::string text((char*)buffer, size);
		result.fromUtf8(text);
	}
	else if (size>=3 &&
			 buffer[0]==utf8Header[0] && 
			 buffer[1]==utf8Header[1] && 
			 buffer[2]==utf8Header[2]
			)
	{
		// remove utf8 header
		buffer+= 3;
		size-=3;
		std::string text((char*)buffer, size);
		result.fromUtf8(text);
	}
	else if (size>=2 &&
			 buffer[0]==utf16Header[0] && 
			 buffer[1]==utf16Header[1]
			)
	{
		// remove utf16 header
		buffer+= 2;
		size-= 2;
		// check pair number of bytes
		sipassert((size & 1) == 0);
		// and do manual conversion
		uint16 *src = (uint16*)(buffer);
		result.resize(size/2);
		for (uint j=0; j<result.size(); j++)
			result[j]= *src++;
	}
	else if (size>=2 &&
			 buffer[0]==utf16RevHeader[0] && 
			 buffer[1]==utf16RevHeader[1]
			)
	{
		// remove utf16 header
		buffer+= 2;
		size-= 2;
		// check pair number of bytes
		sipassert((size & 1) == 0);
		// and do manual conversion
		uint16 *src = (uint16*)(buffer);
		result.resize(size/2);
		uint j;
		for (j=0; j<result.size(); j++)
			result[j]= *src++;
		//  Reverse byte order
		for (j=0; j<result.size(); j++)
		{
			uint8 *pc = (uint8*) &result[j];
			std::swap(pc[0], pc[1]);
		}
	}
	else
	{
		// hum.. ascii read ?
		// so, just to a direct conversion
		std::string text((char*)buffer, size);
		result = text;
	}
}

void CI18N::writeTextFile(const std::string filename, const ucstring &content, bool utf8)
{
	ucstring ucstr;
	ucstr.fromUtf8(filename);
	writeTextFile(ucstr, content, utf8);
}

void CI18N::writeTextFile(const std::basic_string<ucchar> filename, const ucstring &content, bool utf8)
{
	COFile file(filename);

	if (!utf8)
	{
		// write the unicode 16 bits tag
		uint16 unicodeTag = 0xfeff;
		file.serial(unicodeTag);

		uint i;
		for (i=0; i<content.size(); ++i)
		{
			uint16 c = content[i];
			file.serial(c);
		}
	}
	else
	{
		static char utf8Header[] = {char(0xef), char(0xbb), char(0xbf), 0};

		std::string str = encodeUTF8(content);
		// add the UTF-8 'not official' header
		str = utf8Header + str;

		uint i;
		for (i=0; i<str.size(); ++i)
		{
			file.serial(str[i]);
		}
	}
}

ucstring CI18N::makeMarkedString(ucchar openMark, ucchar closeMark, const ucstring &text)
{
	ucstring ret;

	ret.push_back(openMark);

	ucstring::const_iterator first(text.begin()), last(text.end());
	for (; first != last; ++first)
	{
		if (*first == '\n')
		{
			ret += '\\';
			ret += 'n';
		}
		else if (*first == '\t')
		{
			ret += '\\';
			ret += 't';
		}
		else if (*first == closeMark)
		{
			// excape the embeded closing mark
			ret += '\\';
			ret += closeMark;
		}
		else
		{
			ret += *first;
		}
	}

	ret += closeMark;

	return ret;
}


string CI18N::encodeUTF8(const ucstring &str)
{
	return str.toUtf8();
	/*	
	string	res;
	ucstring::const_iterator first(str.begin()), last(str.end());
	for (; first != last; ++first)
	{
	  //ucchar	c = *first;
		uint nbLoop = 0;
		if (*first < 0x80)
			res += char(*first);
		else if (*first < 0x800)
		{
			ucchar c = *first;
			c = c >> 6;
			c = c & 0x1F;
			res += c | 0xC0;
			nbLoop = 1;
		}
		else if (*first < 0x10000)
		{
			ucchar c = *first;
			c = c >> 12;
			c = c & 0x0F;
			res += c | 0xE0;
			nbLoop = 2;
		}

		for (uint i=0; i<nbLoop; ++i)
		{
			ucchar	c = *first;
			c = c >> ((nbLoop - i - 1) * 6);
			c = c & 0x3F;
			res += char(c) | 0x80; 
		}
	}
	return res;
	*/
}

/* UTF-8 conversion table
U-00000000 - U-0000007F:  0xxxxxxx  
U-00000080 - U-000007FF:  110xxxxx 10xxxxxx  
U-00000800 - U-0000FFFF:  1110xxxx 10xxxxxx 10xxxxxx  
// not used as we convert from 16 bits unicode
U-00010000 - U-001FFFFF:  11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  
U-00200000 - U-03FFFFFF:  111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
U-04000000 - U-7FFFFFFF:  1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx  
*/


uint64	CI18N::makeHash(const ucstring &str)
{
	// on passe au moins 8 fois sur chaque octet de resultat
	if (str.empty())
		return 0;
	const	uint32	MIN_TURN = 8*8;
	uint64	hash = 0;
	uint8	*ph = (uint8*)&hash;
	uint8	*pc = (uint8*)str.data();

	uint nbLoop = max(uint32(str.size()*2), MIN_TURN);
	uint roll = 0;

	for (uint i=0; i<nbLoop; ++i)
	{
		ph[(i/2) & 0x7] += pc[i%(str.size()*2)] << roll;
		ph[(i/2) & 0x7] += pc[i%(str.size()*2)] >> (8-roll);

		roll++;
		roll &= 0x7;
	}

	return hash;
}

// convert a hash value to a readable string 
string CI18N::hashToString(uint64 hash)
{
	uint32 *ph = (uint32*)&hash;

	char temp[] = "0011223344556677";
	sprintf(temp, "%08X%08X", ph[0], ph[1]);

	return string(temp);
}

// fast convert a hash value to a ucstring
void	CI18N::hashToUCString(uint64 hash, ucstring &dst)
{
	static ucchar	cvtTable[]= {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	dst.resize(16);
	for(sint i=15;i>=0;i--)
	{
		// Must decal dest of 8, cause of hashToString code (Little Endian)
		dst[(i+8)&15]= cvtTable[hash&15];
		hash>>=4;
	}
}

// convert a readable string into a hash value.
uint64 CI18N::stringToHash(const std::string &str)
{
	sipassert(str.size() == 16);
	uint32	low, hight;

	string sl, sh;
	sh = str.substr(0, 8);
	sl = str.substr(8, 8);

	sscanf(sh.c_str(), "%08X", &hight);
	sscanf(sl.c_str(), "%08X", &low);

	uint64 hash;
	uint32 *ph = (uint32*)&hash;

	ph[0] = hight;
	ph[1] = low;

	return hash;
}

} // namespace SIPBASE
