/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __SSTRING_H__
#define __SSTRING_H__

//#include "types_sip.h"
#include <string>
#include <vector>
#include <stdio.h> // <cstdio>

#include "common.h"
#include "stream.h"
#include "path.h"
#include "string_common.h"

namespace	SIPBASE
{

// advanced class declaration...
//class CVectorSString;
class CSStringA;
typedef std::vector<CSStringA> CVectorSStringA;

class CSStringW;
typedef std::vector<CSStringW> CVectorSStringW;

/**
 * CSStringA : std::string with more functionalities and case insensitive compare
 *
 * \date 2003
 */
class CSStringA: public std::string
{
public:
	///	ctor
	CSStringA();
	///	ctor
	CSStringA(const char *s);
	CSStringA(const ucchar *s);
	/// ctor
	CSStringA(const std::string &s);
	CSStringA(const std::basic_string<ucchar> &s);
	///	ctor
	CSStringA(char c);
	CSStringA(ucchar c);
	///	ctor
	CSStringA(int i, const char *fmt = "%d");
	///	ctor
	CSStringA(unsigned u,const char *fmt = "%u");
	/// ctor
	CSStringA(double d, const char *fmt = "%f");
	///	ctor
	CSStringA(const char *s, const char *fmt);
	///	ctor
	CSStringA(const std::string &s,const char *fmt);
	///	ctor
	CSStringA(const std::vector<SIPBASE::CSStringA>& v,const std::string& separator = "\n");

	/// Const [] operator
	std::string::const_reference operator[](std::string::size_type idx) const;
	/// Non-Const [] operator
	std::string::reference operator[](std::string::size_type idx);

	/// Return the first character, or '\\0' is the string is empty
	char operator*();
	/// Return the n right hand most characters of a string
	char back() const;

	/// Return the n left hand most characters of a string
	CSStringA left(unsigned count) const;
	/// Return the n right hand most characters of a string
	CSStringA right(unsigned count) const;

	/// Return the string minus the n left hand most characters of a string
	CSStringA leftCrop(unsigned count) const;
	/// Return the string minus the n right hand most characters of a string
	CSStringA rightCrop(unsigned count) const;

	/// Return sub string up to but not including first instance of given character, starting at 'iterator'
	/// on exit 'iterator' indexes first character after extracted string segment
	CSStringA splitToWithIterator(char c,uint32& iterator) const;
	/// Return sub string up to but not including first instance of given character
	CSStringA splitTo(char c) const;
	/// Return sub string up to but not including first instance of given character
	CSStringA splitTo(char c,bool truncateThis=false,bool absorbSeparator=true);
	/// Return sub string up to but not including first instance of given character
	CSStringA splitTo(const char *s,bool truncateThis=false);
	/// Return sub string up to but not including first non-quote encapsulated '//'
	CSStringA splitToLineComment(bool truncateThis=false, bool useSlashStringEscape=true);

	/// Return sub string from character following first instance of given character on
	CSStringA splitFrom(char c) const;
	/// Return sub string from character following first instance of given character on
	CSStringA splitFrom(const char *s) const;

	/// Behave like a s strtok() routine, returning the sun string extracted from (and removed from) *this
	CSStringA strtok(const char *separators,
					bool useSmartExtensions=false,			// if true then match brackets etc (and refine with following args)
					bool useAngleBrace=false,				// - treat '<' and '>' as brackets
					bool useSlashStringEscape=true,			// - treat '\' as escape char so "\"" == '"'
					bool useRepeatQuoteStringEscape=true);	// - treat """" as '"')

	/// Return first word (blank separated) - can remove extracted word from source string
	CSStringA firstWord(bool truncateThis=false);
	///	Return first word (blank separated)
	CSStringA firstWordConst() const;
	/// Return sub string remaining after the first word
	CSStringA tailFromFirstWord() const;
	/// Count the number of words in a string
	unsigned countWords() const;
	/// Extract the given word 
	CSStringA word(unsigned idx) const;

	/// Return first word or quote-encompassed sub-string - can remove extracted sub-string from source string
	CSStringA firstWordOrWords(bool truncateThis=false,bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true);
	///	Return first word or quote-encompassed sub-string
	CSStringA firstWordOrWordsConst(bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;
	/// Return sub string following first word (or quote-encompassed sub-string)
	CSStringA tailFromFirstWordOrWords(bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;
	/// Count the number of words (or quote delimited sub-strings) in a string
	unsigned countWordOrWords(bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;
	/// Extract the given words (or quote delimited sub-strings)
	CSStringA wordOrWords(unsigned idx,bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;

	/// Return first line - can remove extracted line from source string
	CSStringA firstLine(bool truncateThis=false);
	///	Return first line
	CSStringA firstLineConst() const;
	/// Return sub string remaining after the first line
	CSStringA tailFromFirstLine() const;
	/// Count the number of lines in a string
	unsigned countLines() const;
	/// Extract the given line
	CSStringA line(unsigned idx) const;

	/// A handy utility routine for knowing if a character is a white space character or not (' ','\t','\n','\r',26)
	static bool isWhiteSpace(char c); 
	///	Test whether character matches '{', '(','[' or '<' (the '<' test depends on the useAngleBrace parameter
	static bool isOpeningDelimiter(char c, bool useAngleBrace = false);
	///	Test whether character matches '}', ')',']' or '>' (the '>' test depends on the useAngleBrace parameter
	static bool isClosingDelimiter(char c, bool useAngleBrace = false);
	///	Test whether character matches '\'' or '\"'
	static bool isStringDelimiter(char c);
	///	Tests whether the character 'b' is the closing delimiter or string delimiter corresponding to character 'a'
	static bool isMatchingDelimiter(char a, char b); 

	/// A handy utility routine for knowing if a character is a valid component of a file name
	static bool isValidFileNameChar(char c); 
	/// A handy utility routine for knowing if a character is a valid first char for a keyword (a..z, '_')
	static bool isValidKeywordFirstChar(char c); 
	/// A handy utility routine for knowing if a character is a valid subsequent char for a keyword (a..z, '_', '0'..'9')
	static bool isValidKeywordChar(char c); 
	/// A handy utility routine for knowing if a character is printable (isValidFileNameChar + more basic punctuation)
	static bool isPrintable(char c);

	/// A handy utility routine for knowing if a character is a hex digit 0..9, a..f
	static bool isHexDigit(char c);
	/// A handy utility routine for converting a hex digit to a numeric value 0..15
	static char convertHexDigit(char c);

	// a handy routine that tests whether a given string contains binary characters or not. Only characters>32 + isWhiteSpace() are valid
	bool isValidText();
	// a handy routine that tests whether a given string is a valid file name or not
	// "\"hello there\\bla\""	is valid 
	// "hello there\\bla"		is not valid - missing quotes
	// "\"hello there\"\\bla"	is not valid - text after quotes
	bool isValidFileName() const;
	// a second handy routine that tests whether a given string is a valid file name or not
	// equivalent to ('\"'+*this+'\"').isValidFileName()
	// "\"hello there\\bla\""	is not valid  - too many quotes
	// "hello there\\bla"		is valid
	bool isValidUnquotedFileName() const;
	// a handy routine that tests whether or not a given string is a valid keyword
	bool isValidKeyword() const;
	// a handy routine that tests whether or not a given string is quote encapsulated
	bool isQuoted(	bool useAngleBrace=false,						// treat '<' and '>' as brackets
					bool useSlashStringEscape=true,					// treat '\' as escape char so "\"" == '"'
					bool useRepeatQuoteStringEscape=true) const;	// treat """" as '"'

	///	Search for the closing delimiter matching the opening delimiter at position 'startPos' in the 'this' string
	/// on error returns startPos
	uint32 findMatchingDelimiterPos(bool useAngleBrace,bool useSlashStringEscape,bool useRepeatQuoteStringEscape,uint32 startPos=0) const;

	///	Extract a chunk from the 'this' string
	/// if first non-blank character is a string delimiter or an opening delimiter then extracts up to the matching closing delimiter
	/// in all other cases an empty string is returned
	/// the return string includes the opening blank characters (it isn't stripped)
	CSStringA matchDelimiters(bool truncateThis=false,
							 bool useAngleBrace=false,				// treat '<' and '>' as brackets
							 bool useSlashStringEscape=true,		// treat '\' as escape char so "\"" == '"'
							 bool useRepeatQuoteStringEscape=true);	// treat """" as '"'

	/// copy out section of string up to separator character, respecting quotes (but not brackets etc)
	/// on error tail after returned string doesn't begin with valid separator character
	CSStringA splitToStringSeparator(	char separator,
										bool truncateThis=false,
										bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
										bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
										bool truncateSeparatorCharacter=false);	// if true tail begins after separator char

	/// copy out section of string up to separator character, respecting quotes, brackets, etc
	/// on error tail after returned string doesn't begin with valid separator character
	/// eg: splitToSeparator(','); - this might be used to split some sort of ',' separated input 
	CSStringA splitToSeparator(	char separator,
								bool truncateThis=false,
								bool useAngleBrace=false,				// treat '<' and '>' as brackets
								bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
								bool truncateSeparatorCharacter=false);	// if true tail begins after separator char

	CSStringA splitToSeparator(	char separator,
								bool useAngleBrace=false,						// treat '<' and '>' as brackets
								bool useSlashStringEscape=true,					// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true) const;	// treat """" as '"'

	/// copy out section of string up to any of a given set of separator characters, respecting quotes, brackets, etc
	/// on error tail after returned string doesn't begin with valid separator character
	/// eg: splitToOneOfSeparators(",;",true,false,false,true); - this might be used to split a string read from a CSV file
	CSStringA splitToOneOfSeparators(	const CSStringA& separators,
										bool truncateThis=false,
										bool useAngleBrace=false,				// treat '<' and '>' as brackets
										bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
										bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
										bool truncateSeparatorCharacter=false,	// if true tail begins after separator char
										bool splitStringAtBrackets=true);		// if true consider brackets as breaks in the string

	CSStringA splitToOneOfSeparators(	const CSStringA& separators,
										bool useAngleBrace=false,						// treat '<' and '>' as brackets
										bool useSlashStringEscape=true,					// treat '\' as escape char so "\"" == '"'
										bool useRepeatQuoteStringEscape=true) const;	// treat """" as '"'

	/// Return true if the string is a single block encompassed by a pair of delimiters
	/// eg: "((a)(b)(c))" or "(abc)" return true wheras "(a)(b)(c)" or "abc" return false
	bool isDelimitedMonoBlock(	bool useAngleBrace=false,				// treat '<' and '>' as brackets
								bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true	// treat """" as '"'
							 ) const;

	/// Return the sub string with leading and trailing delimiters ( such as '(' and ')' or '[' and ']' ) removed
	/// if the string isn't a delimited monoblock then the complete string is returned
	/// eg "((a)b(c))" returns "(a)b(c)" whereas "(a)b(c)" returns the identical "(a)b(c)"
	CSStringA stripBlockDelimiters(	bool useAngleBrace=false,				// treat '<' and '>' as brackets
									bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
									bool useRepeatQuoteStringEscape=true	// treat """" as '"'
								 ) const;

	/// Append the individual words in the string to the result vector
	/// retuns true on success
	bool splitWords(CVectorSStringA& result) const;

	/// Append the individual "wordOrWords" elements in the string to the result vector
	/// retuns true on success
	bool splitWordOrWords(CVectorSStringA& result,bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;

	/// Append the individual lines in the string to the result vector
	/// retuns true on success
	bool splitLines(CVectorSStringA& result) const;

	/// Append the separator-separated elements in the string to the result vector
	/// retuns true on success
	bool splitBySeparator(	char separator, CVectorSStringA& result,
							bool useAngleBrace=false,				// treat '<' and '>' as brackets
							bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
							bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
							bool skipBlankEntries=false				// dont add blank entries to the result vector
						 ) const;

	/// Append the separator-separated elements in the string to the result vector
	/// retuns true on success
	bool splitByOneOfSeparators(	const CSStringA& separators, CVectorSStringA& result,
									bool useAngleBrace=false,				// treat '<' and '>' as brackets
									bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
									bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
									bool retainSeparators=false,			// have the separators turn up in the result vector
									bool skipBlankEntries=false				// dont add blank entries to the result vector
								 ) const;

	/// join an array of strings to form a single string (appends to the existing content of this string)
	/// if this string is not empty then a separator is added between this string and the following
	const CSStringA& join(const std::vector<CSStringA>& strings, const CSStringA& separator = "");
	const CSStringA& join(const std::vector<CSStringA>& strings, char separator);

	/// Return a copy of the string with leading and trainling spaces removed
	CSStringA strip() const;
	/// Return a copy of the string with leading spaces removed
	CSStringA leftStrip() const;
	/// Return a copy of the string with trainling spaces removed
	CSStringA rightStrip() const;

	/// Making an upper case copy of a string
	CSStringA toUpper() const;

	/// Making a lower case copy of a string
	CSStringA toLower() const;

	/// encapsulate string in quotes, adding escape characters as necessary
	CSStringA quote(	bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
					bool useRepeatQuoteStringEscape=true	// treat """" as '"'
					) const;

	/// if a string is not already encapsulated in quotes then return quote() else return *this
	CSStringA quoteIfNotQuoted(	bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true	// treat """" as '"'
								) const;

	/// if a string is not a single word and is not already encapsulated in quotes then return quote() else return *this
	CSStringA quoteIfNotAtomic(	bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true	// treat """" as '"'
								) const;

	/// strip delimiting quotes and clear through escape characters as necessary
	CSStringA unquote(bool useSlashStringEscape=true,		// treat '\' as escape char so "\"" == '"'
					 bool useRepeatQuoteStringEscape=true	// treat """" as '"'
					) const;

	/// equivalent to if (isQuoted()) unquote()
	CSStringA unquoteIfQuoted(bool useSlashStringEscape=true,
					 bool useRepeatQuoteStringEscape=true
					) const;

	///	encode special characters such as quotes, gt, lt, etc to xml encoding
	/// the isParameter paramter is true if the string is to be used in an XML parameter block
	CSStringA encodeXML(bool isParameter=false) const;

	///	decode special characters such as quotes, gt, lt, etc from xml encoding
	CSStringA decodeXML() const;

	/// verifies whether a string contains sub-strings that correspond to xml special character codes
	bool isEncodedXML() const;

	/// verifies whether a string contains any XML incompatible characters
	/// in this case the string can be converted to xml compatible format via encodeXML()
	/// the isParameter paramter is true if the string is to be used in an XML parameter block
	bool isXMLCompatible(bool isParameter=false) const;

	/// Replacing all occurences of one string with another
	CSStringA replace(const char *toFind,const char *replacement) const;

	/// Find index at which a sub-string starts (case not sensitive) - if sub-string not found then returns string::npos
	unsigned find(const char *toFind,unsigned startLocation=0) const;

	/// Find index at which a sub-string starts (case NOT sensitive) - if sub-string not found then returns string::npos
	unsigned findNS(const char *toFind,unsigned startLocation=0) const;

	/// Return true if this contains given sub string
	bool contains(const char *toFind) const;

	/// Return true if this contains given sub string
	bool contains(int character) const;

	/// Handy atoi routines...
	int atoi() const;
	signed atosi() const;
	unsigned atoui() const;
	sint64 atoi64() const;
	sint64 atosi64() const;
	uint64 atoui64() const;

	/// A handy atof routine...
	double atof() const;

	/// assignment operator
	CSStringA& operator=(const char *s);
	CSStringA& operator=(const ucchar *s);
	/// assignment operator
	CSStringA& operator=(const std::string &s);
	CSStringA& operator=(const std::basic_string<ucchar> &s);
	/// assignment operator
	CSStringA& operator=(char c);	
	CSStringA& operator=(ucchar c);	
	/// Case insensitive string compare
	bool operator==(const CSStringA &other) const;
	/// Case insensitive string compare
	bool operator==(const std::string &other) const;
	/// Case insensitive string compare
	bool operator==(const char* other) const;

	/// Case insensitive string compare
	bool operator!=(const CSStringA &other) const;
	/// Case insensitive string compare
	bool operator!=(const std::string &other) const;
	/// Case insensitive string compare
	bool operator!=(const char* other) const;

	/// Case insensitive string compare
	bool operator<=(const CSStringA &other) const;
	/// Case insensitive string compare
	bool operator<=(const std::string &other) const;
	/// Case insensitive string compare
	bool operator<=(const char* other) const;

	/// Case insensitive string compare
	bool operator>=(const CSStringA &other) const;
	/// Case insensitive string compare
	bool operator>=(const std::string &other) const;
	/// Case insensitive string compare
	bool operator>=(const char* other) const;

	/// Case insensitive string compare
	bool operator>(const CSStringA &other) const;
	/// Case insensitive string compare
	bool operator>(const std::string &other) const;
	/// Case insensitive string compare
	bool operator>(const char* other) const;

	/// Case insensitive string compare
	bool operator<(const CSStringA &other) const;
	/// Case insensitive string compare
	bool operator<(const std::string &other) const;
	/// Case insensitive string compare
	bool operator<(const char* other) const;

	//@{
	//@name Easy concatenation operator to build strings
	template <class T>
	CSStringA &operator <<(const T &value)
	{
		operator +=(SIPBASE::toStringA(value));

		return *this;
	}
	
	// specialisation for C string
	CSStringA &operator <<(const char *value)
	{
		static_cast<std::string *>(this)->operator +=(value);

		return *this;
	}

	// specialisation for character
	CSStringA &operator <<(char value)
	{
		static_cast<std::string *>(this)->operator +=(value);

		return *this;
	}

	// specialisation for std::string
	CSStringA &operator <<(const std::string &value)
	{
		static_cast<std::string *>(this)->operator +=(value);

		return *this;
	}
	// specialisation for CSStringA
	CSStringA &operator <<(const CSStringA &value)
	{
		static_cast<std::string *>(this)->operator +=(value);

		return *this;
	}
	//@}

	/// Case insensitive string compare (useful for use as map keys, see less<CSStringA> below)
	bool icompare(const std::string &other) const;

	/// Serial
	void serial( SIPBASE::IStream& s );

	/// Read a text file into a string
	bool readFromFile(const CSStringA& fileName);
	
	/// Write a string to a text file
	// returns true on success, false on failure
	bool writeToFile(const CSStringA& fileName) const;

	/// Write a string to a text file
	// if the file already exists and its content is identicall to our own then it is not overwritten
	// returns true on success (including the case where the file exists and is not overwritten), false on failure
	bool writeToFileIfDifferent(const CSStringA& fileName) const;
};

class CSStringW: public ucstringbase
{
public:
	///	ctor
	CSStringW();
	///	ctor
	CSStringW(const char *s);
	CSStringW(const ucchar *s);
	/// ctor
	CSStringW(const std::string &s);
	CSStringW(const ucstringbase &s);
	///	ctor
	CSStringW(char c);
	CSStringW(ucchar c);
	///	ctor
	CSStringW(int i, const ucchar *fmt = L"%d");
	///	ctor
	CSStringW(unsigned u,const ucchar *fmt = L"%u");
	/// ctor
	CSStringW(double d, const ucchar *fmt = L"%f");
	///	ctor
	CSStringW(const ucchar *s, const ucchar *fmt);
	///	ctor
	CSStringW(const ucstringbase &s,const ucchar *fmt);
	///	ctor
	CSStringW(const std::vector<SIPBASE::CSStringW>& v,const ucstringbase& separator = L"\n");

	/// Const [] operator
	ucstringbase::const_reference operator[](ucstringbase::size_type idx) const;
	/// Non-Const [] operator
	ucstringbase::reference operator[](ucstringbase::size_type idx);

	/// Return the first character, or '\\0' is the string is empty
	ucchar operator*();
	/// Return the n right hand most characters of a string
	ucchar back() const;

	/// Return the n left hand most characters of a string
	CSStringW left(unsigned count) const;
	/// Return the n right hand most characters of a string
	CSStringW right(unsigned count) const;

	/// Return the string minus the n left hand most characters of a string
	CSStringW leftCrop(unsigned count) const;
	/// Return the string minus the n right hand most characters of a string
	CSStringW rightCrop(unsigned count) const;

	/// Return sub string up to but not including first instance of given character, starting at 'iterator'
	/// on exit 'iterator' indexes first character after extracted string segment
	CSStringW splitToWithIterator(ucchar c,uint32& iterator) const;
	/// Return sub string up to but not including first instance of given character
	CSStringW splitTo(ucchar c) const;
	/// Return sub string up to but not including first instance of given character
	CSStringW splitTo(ucchar c,bool truncateThis=false,bool absorbSeparator=true);
	/// Return sub string up to but not including first instance of given character
	CSStringW splitTo(const ucchar *s,bool truncateThis=false);
	/// Return sub string up to but not including first non-quote encapsulated '//'
	CSStringW splitToLineComment(bool truncateThis=false, bool useSlashStringEscape=true);

	/// Return sub string from character following first instance of given character on
	CSStringW splitFrom(ucchar c) const;
	/// Return sub string from character following first instance of given character on
	CSStringW splitFrom(const ucchar *s) const;

	/// Behave like a s strtok() routine, returning the sun string extracted from (and removed from) *this
	CSStringW strtok(const ucchar *separators,
					bool useSmartExtensions=false,			// if true then match brackets etc (and refine with following args)
					bool useAngleBrace=false,				// - treat '<' and '>' as brackets
					bool useSlashStringEscape=true,			// - treat '\' as escape char so "\"" == '"'
					bool useRepeatQuoteStringEscape=true);	// - treat """" as '"')

	/// Return first word (blank separated) - can remove extracted word from source string
	CSStringW firstWord(bool truncateThis=false);
	///	Return first word (blank separated)
	CSStringW firstWordConst() const;
	/// Return sub string remaining after the first word
	CSStringW tailFromFirstWord() const;
	/// Count the number of words in a string
	unsigned countWords() const;
	/// Extract the given word 
	CSStringW word(unsigned idx) const;

	/// Return first word or quote-encompassed sub-string - can remove extracted sub-string from source string
	CSStringW firstWordOrWords(bool truncateThis=false,bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true);
	///	Return first word or quote-encompassed sub-string
	CSStringW firstWordOrWordsConst(bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;
	/// Return sub string following first word (or quote-encompassed sub-string)
	CSStringW tailFromFirstWordOrWords(bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;
	/// Count the number of words (or quote delimited sub-strings) in a string
	unsigned countWordOrWords(bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;
	/// Extract the given words (or quote delimited sub-strings)
	CSStringW wordOrWords(unsigned idx,bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;

	/// Return first line - can remove extracted line from source string
	CSStringW firstLine(bool truncateThis=false);
	///	Return first line
	CSStringW firstLineConst() const;
	/// Return sub string remaining after the first line
	CSStringW tailFromFirstLine() const;
	/// Count the number of lines in a string
	unsigned countLines() const;
	/// Extract the given line
	CSStringW line(unsigned idx) const;

	/// A handy utility routine for knowing if a character is a white space character or not (' ','\t','\n','\r',26)
	static bool isWhiteSpace(ucchar c); 
	///	Test whether character matches '{', '(','[' or '<' (the '<' test depends on the useAngleBrace parameter
	static bool isOpeningDelimiter(ucchar c, bool useAngleBrace = false);
	///	Test whether character matches '}', ')',']' or '>' (the '>' test depends on the useAngleBrace parameter
	static bool isClosingDelimiter(ucchar c, bool useAngleBrace = false);
	///	Test whether character matches '\'' or '\"'
	static bool isStringDelimiter(ucchar c);
	///	Tests whether the character 'b' is the closing delimiter or string delimiter corresponding to character 'a'
	static bool isMatchingDelimiter(ucchar a, ucchar b); 

	/// A handy utility routine for knowing if a character is a valid component of a file name
	static bool isValidFileNameChar(ucchar c); 
	/// A handy utility routine for knowing if a character is a valid first char for a keyword (a..z, '_')
	static bool isValidKeywordFirstChar(ucchar c); 
	/// A handy utility routine for knowing if a character is a valid subsequent char for a keyword (a..z, '_', '0'..'9')
	static bool isValidKeywordChar(ucchar c); 
	/// A handy utility routine for knowing if a character is printable (isValidFileNameChar + more basic punctuation)
	static bool isPrintable(ucchar c);

	/// A handy utility routine for knowing if a character is a hex digit 0..9, a..f
	static bool isHexDigit(ucchar c);
	/// A handy utility routine for converting a hex digit to a numeric value 0..15
	static ucchar convertHexDigit(ucchar c);

	// a handy routine that tests whether a given string contains binary characters or not. Only characters>32 + isWhiteSpace() are valid
	bool isValidText();
	// a handy routine that tests whether a given string is a valid file name or not
	// "\"hello there\\bla\""	is valid 
	// "hello there\\bla"		is not valid - missing quotes
	// "\"hello there\"\\bla"	is not valid - text after quotes
	bool isValidFileName() const;
	// a second handy routine that tests whether a given string is a valid file name or not
	// equivalent to ('\"'+*this+'\"').isValidFileName()
	// "\"hello there\\bla\""	is not valid  - too many quotes
	// "hello there\\bla"		is valid
	bool isValidUnquotedFileName() const;
	// a handy routine that tests whether or not a given string is a valid keyword
	bool isValidKeyword() const;
	// a handy routine that tests whether or not a given string is quote encapsulated
	bool isQuoted(	bool useAngleBrace=false,						// treat '<' and '>' as brackets
					bool useSlashStringEscape=true,					// treat '\' as escape char so "\"" == '"'
					bool useRepeatQuoteStringEscape=true) const;	// treat """" as '"'

	///	Search for the closing delimiter matching the opening delimiter at position 'startPos' in the 'this' string
	/// on error returns startPos
	uint32 findMatchingDelimiterPos(bool useAngleBrace,bool useSlashStringEscape,bool useRepeatQuoteStringEscape,uint32 startPos=0) const;

	///	Extract a chunk from the 'this' string
	/// if first non-blank character is a string delimiter or an opening delimiter then extracts up to the matching closing delimiter
	/// in all other cases an empty string is returned
	/// the return string includes the opening blank characters (it isn't stripped)
	CSStringW matchDelimiters(bool truncateThis=false,
							 bool useAngleBrace=false,				// treat '<' and '>' as brackets
							 bool useSlashStringEscape=true,		// treat '\' as escape char so "\"" == '"'
							 bool useRepeatQuoteStringEscape=true);	// treat """" as '"'

	/// copy out section of string up to separator character, respecting quotes (but not brackets etc)
	/// on error tail after returned string doesn't begin with valid separator character
	CSStringW splitToStringSeparator(	ucchar separator,
										bool truncateThis=false,
										bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
										bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
										bool truncateSeparatorCharacter=false);	// if true tail begins after separator char

	/// copy out section of string up to separator character, respecting quotes, brackets, etc
	/// on error tail after returned string doesn't begin with valid separator character
	/// eg: splitToSeparator(','); - this might be used to split some sort of ',' separated input 
	CSStringW splitToSeparator(	ucchar separator,
								bool truncateThis=false,
								bool useAngleBrace=false,				// treat '<' and '>' as brackets
								bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
								bool truncateSeparatorCharacter=false);	// if true tail begins after separator char

	CSStringW splitToSeparator(	ucchar separator,
								bool useAngleBrace=false,						// treat '<' and '>' as brackets
								bool useSlashStringEscape=true,					// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true) const;	// treat """" as '"'

	/// copy out section of string up to any of a given set of separator characters, respecting quotes, brackets, etc
	/// on error tail after returned string doesn't begin with valid separator character
	/// eg: splitToOneOfSeparators(",;",true,false,false,true); - this might be used to split a string read from a CSV file
	CSStringW splitToOneOfSeparators(	const CSStringW& separators,
										bool truncateThis=false,
										bool useAngleBrace=false,				// treat '<' and '>' as brackets
										bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
										bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
										bool truncateSeparatorCharacter=false,	// if true tail begins after separator char
										bool splitStringAtBrackets=true);		// if true consider brackets as breaks in the string

	CSStringW splitToOneOfSeparators(	const CSStringW& separators,
										bool useAngleBrace=false,						// treat '<' and '>' as brackets
										bool useSlashStringEscape=true,					// treat '\' as escape char so "\"" == '"'
										bool useRepeatQuoteStringEscape=true) const;	// treat """" as '"'

	/// Return true if the string is a single block encompassed by a pair of delimiters
	/// eg: "((a)(b)(c))" or "(abc)" return true wheras "(a)(b)(c)" or "abc" return false
	bool isDelimitedMonoBlock(	bool useAngleBrace=false,				// treat '<' and '>' as brackets
								bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true	// treat """" as '"'
							 ) const;

	/// Return the sub string with leading and trailing delimiters ( such as '(' and ')' or '[' and ']' ) removed
	/// if the string isn't a delimited monoblock then the complete string is returned
	/// eg "((a)b(c))" returns "(a)b(c)" whereas "(a)b(c)" returns the identical "(a)b(c)"
	CSStringW stripBlockDelimiters(	bool useAngleBrace=false,				// treat '<' and '>' as brackets
									bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
									bool useRepeatQuoteStringEscape=true	// treat """" as '"'
								 ) const;

	/// Append the individual words in the string to the result vector
	/// retuns true on success
	bool splitWords(CVectorSStringW& result) const;

	/// Append the individual "wordOrWords" elements in the string to the result vector
	/// retuns true on success
	bool splitWordOrWords(CVectorSStringW& result,bool useSlashStringEscape=true,bool useRepeatQuoteStringEscape=true) const;

	/// Append the individual lines in the string to the result vector
	/// retuns true on success
	bool splitLines(CVectorSStringW& result) const;

	/// Append the separator-separated elements in the string to the result vector
	/// retuns true on success
	bool splitBySeparator(	ucchar separator, CVectorSStringW& result,
							bool useAngleBrace=false,				// treat '<' and '>' as brackets
							bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
							bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
							bool skipBlankEntries=false				// dont add blank entries to the result vector
						 ) const;

	/// Append the separator-separated elements in the string to the result vector
	/// retuns true on success
	bool splitByOneOfSeparators(	const CSStringW& separators, CVectorSStringW& result,
									bool useAngleBrace=false,				// treat '<' and '>' as brackets
									bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
									bool useRepeatQuoteStringEscape=true,	// treat """" as '"'
									bool retainSeparators=false,			// have the separators turn up in the result vector
									bool skipBlankEntries=false				// dont add blank entries to the result vector
								 ) const;

	/// join an array of strings to form a single string (appends to the existing content of this string)
	/// if this string is not empty then a separator is added between this string and the following
	const CSStringW& join(const std::vector<CSStringW>& strings, const CSStringW& separator = L"");
	const CSStringW& join(const std::vector<CSStringW>& strings, ucchar separator);

	/// Return a copy of the string with leading and trainling spaces removed
	CSStringW strip() const;
	/// Return a copy of the string with leading spaces removed
	CSStringW leftStrip() const;
	/// Return a copy of the string with trainling spaces removed
	CSStringW rightStrip() const;

	/// Making an upper case copy of a string
	CSStringW toUpper() const;

	/// Making a lower case copy of a string
	CSStringW toLower() const;

	/// encapsulate string in quotes, adding escape characters as necessary
	CSStringW quote(	bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
					bool useRepeatQuoteStringEscape=true	// treat """" as '"'
					) const;

	/// if a string is not already encapsulated in quotes then return quote() else return *this
	CSStringW quoteIfNotQuoted(	bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true	// treat """" as '"'
								) const;

	/// if a string is not a single word and is not already encapsulated in quotes then return quote() else return *this
	CSStringW quoteIfNotAtomic(	bool useSlashStringEscape=true,			// treat '\' as escape char so "\"" == '"'
								bool useRepeatQuoteStringEscape=true	// treat """" as '"'
								) const;

	/// strip delimiting quotes and clear through escape characters as necessary
	CSStringW unquote(bool useSlashStringEscape=true,		// treat '\' as escape char so "\"" == '"'
					 bool useRepeatQuoteStringEscape=true	// treat """" as '"'
					) const;

	/// equivalent to if (isQuoted()) unquote()
	CSStringW unquoteIfQuoted(bool useSlashStringEscape=true,
					 bool useRepeatQuoteStringEscape=true
					) const;

	///	encode special characters such as quotes, gt, lt, etc to xml encoding
	/// the isParameter paramter is true if the string is to be used in an XML parameter block
	CSStringW encodeXML(bool isParameter=false) const;

	///	decode special characters such as quotes, gt, lt, etc from xml encoding
	CSStringW decodeXML() const;

	/// verifies whether a string contains sub-strings that correspond to xml special character codes
	bool isEncodedXML() const;

	/// verifies whether a string contains any XML incompatible characters
	/// in this case the string can be converted to xml compatible format via encodeXML()
	/// the isParameter paramter is true if the string is to be used in an XML parameter block
	bool isXMLCompatible(bool isParameter=false) const;

	/// Replacing all occurences of one string with another
	CSStringW replace(const ucchar *toFind,const ucchar *replacement) const;

	/// Find index at which a sub-string starts (case not sensitive) - if sub-string not found then returns string::npos
	unsigned find(const ucchar *toFind,unsigned startLocation=0) const;

	/// Find index at which a sub-string starts (case NOT sensitive) - if sub-string not found then returns string::npos
	unsigned findNS(const ucchar *toFind,unsigned startLocation=0) const;

	/// Return true if this contains given sub string
	bool contains(const ucchar *toFind) const;

	/// Return true if this contains given sub string
	bool contains(int character) const;

	/// Handy atoi routines...
	int atoi() const;
	signed atosi() const;
	unsigned atoui() const;
	sint64 atoi64() const;
	sint64 atosi64() const;
	uint64 atoui64() const;

	/// A handy atof routine...
	double atof() const;

	/// assignment operator
	CSStringW& operator=(const char *s);
	CSStringW& operator=(const ucchar *s);
	/// assignment operator
	CSStringW& operator=(const std::string &s);
	CSStringW& operator=(const ucstringbase &s);
	/// assignment operator
	CSStringW& operator=(char c);	
	CSStringW& operator=(ucchar c);	
	/// Case insensitive string compare
	bool operator==(const CSStringW &other) const;
	/// Case insensitive string compare
	bool operator==(const ucstringbase &other) const;
	/// Case insensitive string compare
	bool operator==(const ucchar* other) const;

	/// Case insensitive string compare
	bool operator!=(const CSStringW &other) const;
	/// Case insensitive string compare
	bool operator!=(const ucstringbase &other) const;
	/// Case insensitive string compare
	bool operator!=(const ucchar* other) const;

	/// Case insensitive string compare
	bool operator<=(const CSStringW &other) const;
	/// Case insensitive string compare
	bool operator<=(const ucstringbase &other) const;
	/// Case insensitive string compare
	bool operator<=(const ucchar* other) const;

	/// Case insensitive string compare
	bool operator>=(const CSStringW &other) const;
	/// Case insensitive string compare
	bool operator>=(const ucstringbase &other) const;
	/// Case insensitive string compare
	bool operator>=(const ucchar* other) const;

	/// Case insensitive string compare
	bool operator>(const CSStringW &other) const;
	/// Case insensitive string compare
	bool operator>(const ucstringbase &other) const;
	/// Case insensitive string compare
	bool operator>(const ucchar* other) const;

	/// Case insensitive string compare
	bool operator<(const CSStringW &other) const;
	/// Case insensitive string compare
	bool operator<(const ucstringbase &other) const;
	/// Case insensitive string compare
	bool operator<(const ucchar* other) const;

	//@{
	//@name Easy concatenation operator to build strings
	template <class T>
	CSStringW &operator <<(const T &value)
	{
		operator +=(SIPBASE::toStringW(value));

		return *this;
	}
	
	// specialisation for C string
	CSStringW &operator <<(const ucchar *value)
	{
		static_cast<ucstringbase *>(this)->operator +=(value);

		return *this;
	}

	// specialisation for character
	CSStringW &operator <<(ucchar value)
	{
		static_cast<ucstringbase *>(this)->operator +=(value);

		return *this;
	}

	// specialisation for ucstringbase
	CSStringW &operator <<(const ucstringbase &value)
	{
		static_cast<ucstringbase *>(this)->operator +=(value);

		return *this;
	}
	// specialisation for CSStringW
	CSStringW &operator <<(const CSStringW &value)
	{
		static_cast<ucstringbase *>(this)->operator +=(value);

		return *this;
	}
	//@}

	/// Case insensitive string compare (useful for use as map keys, see less<CSStringW> below)
	bool icompare(const ucstringbase &other) const;

	/// Serial
	void serial( SIPBASE::IStream& s );

	/// Read a text file into a string
	bool readFromFile(const CSStringW& fileName);
	
	/// Write a string to a text file
	// returns true on success, false on failure
	bool writeToFile(const CSStringW& fileName) const;

	/// Write a string to a text file
	// if the file already exists and its content is identicall to our own then it is not overwritten
	// returns true on success (including the case where the file exists and is not overwritten), false on failure
	bool writeToFileIfDifferent(const CSStringW& fileName) const;
};

/*
 * Vector of CSString compatible with vector<string>
 */
//typedef std::vector<CSString> CVectorSString;


/*CVectorSString &operator = (CVectorSString &left, const std::vector<std::string> &right)
{
	left = reinterpret_cast<CVectorSString&>(right);
}
CVectorSString &operator = (CVectorSString &left, const std::vector<CSString> &right)
{
	left = reinterpret_cast<CVectorSString&>(right);
}
*/
//class CVectorSString : public std::vector<CSString>
//{
//public:
//	// cast to and convert from std::vector<std::string>
//	operator std::vector<std::string>& ()
//	{ 
//		return reinterpret_cast<std::vector<std::string>&>(*this); 
//	}
//	operator const std::vector<std::string>& () const
//	{ 
//		return reinterpret_cast<const std::vector<std::string>&>(*this); 
//	}
//	CVectorSString&	operator= ( const std::vector<std::string>& v ) 
//	{
//		*this = reinterpret_cast<const CVectorSString&>(v); 
//		return *this; 
//	}
//
//	// simple ctors
//	CVectorSString()							{}
//	CVectorSString( const CVectorSString& v )	{ operator=(v); }
//
//	// ctors for building from different vetor types
//	CVectorSString( const std::vector<CSString>& v ):							std::vector<CSString>(v)			{}
//	CVectorSString( const std::vector<std::string>& v ):						std::vector<CSString>(*(std::vector<CSString>*)&v) {}
//
//	// ctor for extracting sub_section of another vector
//	CVectorSString( const const_iterator& first, const const_iterator& last ):	std::vector<CSString>(first,last)	{}
//};


/*
 * Inlines
 */

inline CSStringA::CSStringA()
{
}

inline CSStringA::CSStringA(const char *s)
{
	*(std::string *)this = s;
}

inline CSStringA::CSStringA(const ucchar *s)
{
	*(std::string *)this = SIPBASE::toString("%S", s);
}

inline CSStringA::CSStringA(const std::string &s)
{
	*(std::string *)this = s;
}

inline CSStringA::CSStringA(const std::basic_string<ucchar> &s)
{
	*(std::string *)this = SIPBASE::toString("%S", s.c_str());
}

inline CSStringA::CSStringA(char c)
{
	*(std::string *)this = c;
}

inline CSStringA::CSStringA(ucchar c)
{
	*(std::string *)this = (char)(c & 0xff);
}

inline CSStringA::CSStringA(int i,const char *fmt)
{
	char buf[1024];
	sprintf(buf, fmt, i);
	*this=buf;
}

inline CSStringA::CSStringA(unsigned u,const char *fmt)
{
	char buf[1024];
	sprintf(buf, fmt, u);
	*this=buf;
}

inline CSStringA::CSStringA(double d,const char *fmt)
{
	char buf[1024];
	sprintf(buf, fmt, d);
	*this=buf;
}

inline CSStringA::CSStringA(const char *s,const char *fmt)
{
	char buf[1024];
	sprintf(buf, fmt, s);
	*this=buf;
}

inline CSStringA::CSStringA(const std::string &s,const char *fmt)
{
	char buf[1024];
	sprintf(buf, fmt, s.c_str());
	*this=buf;
}

inline CSStringA::CSStringA(const std::vector<SIPBASE::CSStringA>& v,const std::string& separator)
{
	for (uint32 i = 0; i < v.size(); ++ i)
	{
		if (i > 0)
			*this += separator;
		*this += v[i];
	}
}

inline char CSStringA::operator*()
{
	if (empty())
		return 0;
	return (*this)[0];
}

inline char CSStringA::back() const
{
	return (*this)[size()-1];
}

inline CSStringA CSStringA::right(unsigned count) const
{
	if (count>=size())
		return *this;
	return substr(size()-count);
}

inline CSStringA CSStringA::rightCrop(unsigned count) const
{
	if (count>=size())
		return CSStringA();
	return substr(0,size()-count);
}

inline CSStringA CSStringA::left(unsigned count) const
{
	return substr(0,count);
}

inline CSStringA CSStringA::leftCrop(unsigned count) const
{
	if (count>=size())
		return CSStringA();
	return substr(count);
}

inline CSStringA CSStringA::splitToWithIterator(char c,uint32& iterator) const
{
	unsigned i;
	CSStringA result;
	for (i=iterator;i<size() && (*this)[i]!=c;++i)
		result+=(*this)[i];
	iterator= i;
	return result;
}

inline bool CSStringA::isWhiteSpace(char c) 
{
	return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 26;
}

inline bool CSStringA::isOpeningDelimiter(char c, bool useAngleBrace)
{
	return c == '(' || c == '[' || c == '{' || (useAngleBrace && c == '<'); 
}

inline bool CSStringA::isClosingDelimiter(char c,bool useAngleBrace)
{
	return c == ')' || c == ']' || c == '}' || (useAngleBrace && c == '>'); 
}

inline bool CSStringA::isStringDelimiter(char c)
{
	return c == '\"' || c == '\''; 
}

inline bool CSStringA::isMatchingDelimiter(char a, char b) 
{
	return	(a == '(' && b == ')') || (a == '[' && b == ']') || 
			(a == '{' && b == '}') || (a == '<' && b == '>') || 
			(a == '\"' && b == '\"') || (a == '\'' && b == '\''); 
}

inline bool CSStringA::isValidFileNameChar(char c)
{
	if (c >= 'a' && c <= 'z') return true;
	if (c >= 'A' && c <= 'Z') return true;
	if (c >= '0' && c <= '9') return true;
	if (c == '_') return true;
	if (c == ':') return true;
	if (c == '/') return true;
	if (c == '\\') return true;
	if (c == '.') return true;
	if (c == '#') return true;
	if (c == '-') return true;
	return false;
}

inline bool CSStringA::isPrintable(char c)
{
	if (isValidFileNameChar(c)) return true;
	if (c == ' ') return true;
	if (c == '*') return true;
	if (c == '?') return true;
	if (c == '!') return true;
	if (c == '@') return true;
	if (c == '&') return true;
	if (c == '|') return true;
	if (c == '+') return true;
	if (c == '=') return true;
	if (c == '%') return true;
	if (c == '<') return true;
	if (c == '>') return true;
	if (c == '(') return true;
	if (c == ')') return true;
	if (c == '[') return true;
	if (c == ']') return true;
	if (c == '{') return true;
	if (c == '}') return true;
	if (c == ',') return true;
	if (c == ';') return true;
	if (c == '$') return true;
	if ((c & 0xFF) == 156)     return true; // Sterling Pound char causing error in gcc 4.1.2
	if (c == '^') return true;
	if (c == '~') return true;
	if (c == '\'') return true;
	if (c == '\"') return true;
	return false;
}

inline bool CSStringA::isQuoted(bool useAngleBrace,bool useSlashStringEscape,bool useRepeatQuoteStringEscape) const
{
	return (left(1) == "\"") && (right(1) == "\"") && isDelimitedMonoBlock(useAngleBrace,useSlashStringEscape,useRepeatQuoteStringEscape);
}

inline bool CSStringA::isValidKeywordFirstChar(char c)
{
	if (c >= 'a' && c <= 'z') return true;
	if (c >= 'A' && c <= 'Z') return true;
	if (c == '_') return true;
	return false;
}

inline bool CSStringA::isValidKeywordChar(char c)
{
	if (c >= 'a' && c <= 'z') return true;
	if (c >= 'A' && c <= 'Z') return true;
	if (c >= '0' && c <= '9') return true;
	if (c == '_') return true;
	return false;
}

inline bool CSStringA::isHexDigit(char c) 
{
	if (c >= '0' && c <= '9') return true;
	if (c >= 'A' && c <= 'F') return true;
	if (c >= 'a' && c <= 'f') return true;
	return false;
}

inline char CSStringA::convertHexDigit(char c) 
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'A' && c <= 'F') return c - 'A' + 10;
	if (c >= 'a' && c <= 'f') return c - 'a' + 10;
	return 0;
}

inline CSStringA& CSStringA::operator=(const char *s)
{
	*(std::string *)this = s;
	return *this;
}

inline CSStringA& CSStringA::operator=(const ucchar *s)
{
	*(std::string *)this = SIPBASE::toString("%S", s);
	return *this;
}

inline CSStringA& CSStringA::operator=(const std::string &s)
{
	*(std::string *)this=s;
	return *this;
}

inline CSStringA& CSStringA::operator=(const std::basic_string<ucchar> &s)
{
	*(std::string *)this = SIPBASE::toString("%S", s.c_str());
	return *this;
}

inline CSStringA& CSStringA::operator=(char c)
{
	*(std::string *)this = c;
	return *this;
}

inline CSStringA& CSStringA::operator=(ucchar c)
{
	*(std::string *)this = (char)(c & 0xff);
	return *this;
}

inline bool CSStringA::operator==(const CSStringA &other) const
{
	return sipstricmp(c_str(),other.c_str())==0;
}

inline bool CSStringA::operator==(const std::string &other) const
{
	return sipstricmp(c_str(),other.c_str())==0;
}

inline bool CSStringA::operator==(const char* other) const
{
	return sipstricmp(c_str(),other)==0;
}

inline bool CSStringA::operator!=(const CSStringA &other) const
{
	return sipstricmp(c_str(),other.c_str())!=0;
}

inline bool CSStringA::operator!=(const std::string &other) const
{
	return sipstricmp(c_str(),other.c_str())!=0;
}

inline bool CSStringA::operator!=(const char* other) const
{
	return sipstricmp(c_str(),other)!=0;
}

inline bool CSStringA::operator<=(const CSStringA &other) const
{
	return sipstricmp(c_str(),other.c_str())<=0;
}

inline bool CSStringA::operator<=(const std::string &other) const
{
	return sipstricmp(c_str(),other.c_str())<=0;
}

inline bool CSStringA::operator<=(const char* other) const
{
	return sipstricmp(c_str(),other)<=0;
}

inline bool CSStringA::operator>=(const CSStringA &other) const
{
	return sipstricmp(c_str(),other.c_str())>=0;
}

inline bool CSStringA::operator>=(const std::string &other) const
{
	return sipstricmp(c_str(),other.c_str())>=0;
}

inline bool CSStringA::operator>=(const char* other) const
{
	return sipstricmp(c_str(),other)>=0;
}

inline bool CSStringA::operator>(const CSStringA &other) const
{
	return sipstricmp(c_str(),other.c_str())>0;
}

inline bool CSStringA::operator>(const std::string &other) const
{
	return sipstricmp(c_str(),other.c_str())>0;
}

inline bool CSStringA::operator>(const char* other) const
{
	return sipstricmp(c_str(),other)>0;
}

inline bool CSStringA::operator<(const CSStringA &other) const
{
	return sipstricmp(c_str(),other.c_str())<0;
}

inline bool CSStringA::operator<(const std::string &other) const
{
	return sipstricmp(c_str(),other.c_str())<0;
}

inline bool CSStringA::operator<(const char* other) const
{
	return sipstricmp(c_str(),other)<0;
}

inline std::string::const_reference CSStringA::operator[](std::string::size_type idx) const
{
	static char zero=0;
	if (idx >= size())
		return zero;
	return data()[idx];
}

inline std::string::reference CSStringA::operator[](std::string::size_type idx)
{
	static char zero=0;
	if (idx >= size())
		return zero;
	return const_cast<std::string::value_type&>(data()[idx]);
}


inline bool CSStringA::icompare(const std::string &other) const
{
	return sipstricmp(c_str(),other.c_str())<0;
}

inline void CSStringA::serial( SIPBASE::IStream& s )
{
	s.serial( reinterpret_cast<std::string&>( *this ) );
}

/*
inline CSStringA operator+(const CSStringA& s0,char s1)
{
	return CSStringA(s0)+s1;
}

inline CSStringA operator+(const CSStringA& s0,const char* s1)
{
	return CSStringA(s0)+s1;
}

inline CSStringA operator+(const CSStringA& s0,const std::string& s1)
{
	return CSStringA(s0)+s1;
}
inline CSStringA operator+(const CSStringA& s0,const CSStringA& s1)
{
	return CSStringA(s0)+s1;
}
*/
inline CSStringA operator+(char s0,const CSStringA& s1)
{
	return CSStringA(s0)+s1;
}

inline CSStringA operator+(const char* s0,const CSStringA& s1)
{
	return CSStringA(s0)+s1;
}

inline CSStringA operator+(const std::string& s0,const CSStringA& s1)
{
	return s0+static_cast<const std::string&>(s1);
}

inline CSStringW::CSStringW()
{
}

inline CSStringW::CSStringW(const char *s)
{
	*(ucstringbase *)this = SIPBASE::getUcsFromMbs(s);
}

inline CSStringW::CSStringW(const ucchar *s)
{
	*(ucstringbase *)this = s;
}

inline CSStringW::CSStringW(const std::string &s)
{
	*(ucstringbase *)this = SIPBASE::getUcsFromMbs(s.c_str());
}

inline CSStringW::CSStringW(const ucstringbase &s)
{
	*(ucstringbase *)this = s;
}

inline CSStringW::CSStringW(char c)
{
	*(ucstringbase *)this = (uint8)c;
}

inline CSStringW::CSStringW(ucchar c)
{
	*(ucstringbase *)this = c;
}

inline CSStringW::CSStringW(int i, const ucchar *fmt)
{
	ucchar buf[1024];
	swprintf(buf, 1024, fmt, i);
	*this = buf;
}

inline CSStringW::CSStringW(unsigned u, const ucchar *fmt)
{
	ucchar buf[1024];
	swprintf(buf, 1024, fmt, u);
	*this = buf;
}

inline CSStringW::CSStringW(double d, const ucchar *fmt)
{
	ucchar buf[1024];
	swprintf(buf, 1024, fmt, d);
	*this = buf;
}

inline CSStringW::CSStringW(const ucchar *s,const ucchar *fmt)
{
	ucchar buf[1024];
	swprintf(buf, 1024, fmt, s);
	*this = buf;
}

inline CSStringW::CSStringW(const ucstringbase &s,const ucchar *fmt)
{
	ucchar buf[1024];
	swprintf(buf, 1024, fmt, s.c_str());
	*this = buf;
}

inline CSStringW::CSStringW(const std::vector<SIPBASE::CSStringW>& v,const ucstringbase& separator)
{
	for (uint32 i = 0; i < v.size(); ++ i)
	{
		if (i > 0)
			*this += separator;
		*this += v[i];
	}
}

inline ucchar CSStringW::operator*()
{
	if (empty())
		return 0;
	return (*this)[0];
}

inline ucchar CSStringW::back() const
{
	return (*this)[size()-1];
}

inline CSStringW CSStringW::right(unsigned count) const
{
	if (count>=size())
		return *this;
	return substr(size()-count);
}

inline CSStringW CSStringW::rightCrop(unsigned count) const
{
	if (count>=size())
		return CSStringW();
	return substr(0,size()-count);
}

inline CSStringW CSStringW::left(unsigned count) const
{
	return substr(0,count);
}

inline CSStringW CSStringW::leftCrop(unsigned count) const
{
	if (count>=size())
		return CSStringW();
	return substr(count);
}

inline CSStringW CSStringW::splitToWithIterator(ucchar c,uint32& iterator) const
{
	unsigned i;
	CSStringW result;
	for (i=iterator;i<size() && (*this)[i]!=c;++i)
		result+=(*this)[i];
	iterator= i;
	return result;
}

inline bool CSStringW::isWhiteSpace(ucchar c) 
{
	return c == L' ' || c == L'\t' || c == L'\n' || c == L'\r' || c == 26;
}

inline bool CSStringW::isOpeningDelimiter(ucchar c, bool useAngleBrace)
{
	return c == L'(' || c == L'[' || c == L'{' || (useAngleBrace && c == L'<'); 
}

inline bool CSStringW::isClosingDelimiter(ucchar c,bool useAngleBrace)
{
	return c == L')' || c == L']' || c == L'}' || (useAngleBrace && c == L'>'); 
}

inline bool CSStringW::isStringDelimiter(ucchar c)
{
	return c == L'\"' || c == L'\''; 
}

inline bool CSStringW::isMatchingDelimiter(ucchar a, ucchar b) 
{
	return	(a == L'(' && b == L')') || (a == L'[' && b == L']') || 
			(a == L'{' && b == L'}') || (a == L'<' && b == L'>') || 
			(a == L'\"' && b == L'\"') || (a == L'\'' && b == L'\''); 
}

inline bool CSStringW::isValidFileNameChar(ucchar c)
{
	if (c >= L'a' && c <= L'z') return true;
	if (c >= L'A' && c <= L'Z') return true;
	if (c >= L'0' && c <= L'9') return true;
	if (c == L'_') return true;
	if (c == L':') return true;
	if (c == L'/') return true;
	if (c == L'\\') return true;
	if (c == L'.') return true;
	if (c == L'#') return true;
	if (c == L'-') return true;
	return false;
}

inline bool CSStringW::isPrintable(ucchar c)
{
	if (isValidFileNameChar(c)) return true;
	if (c == L' ') return true;
	if (c == L'*') return true;
	if (c == L'?') return true;
	if (c == L'!') return true;
	if (c == L'@') return true;
	if (c == L'&') return true;
	if (c == L'|') return true;
	if (c == L'+') return true;
	if (c == L'=') return true;
	if (c == L'%') return true;
	if (c == L'<') return true;
	if (c == L'>') return true;
	if (c == L'(') return true;
	if (c == L')') return true;
	if (c == L'[') return true;
	if (c == L']') return true;
	if (c == L'{') return true;
	if (c == L'}') return true;
	if (c == L',') return true;
	if (c == L';') return true;
	if (c == L'$') return true;
	if (c == 156)     return true; // Sterling Pound char causing error in gcc 4.1.2
	if (c == L'^') return true;
	if (c == L'~') return true;
	if (c == L'\'') return true;
	if (c == L'\"') return true;
	return false;
}

inline bool CSStringW::isQuoted(bool useAngleBrace,bool useSlashStringEscape,bool useRepeatQuoteStringEscape) const
{
	return (left(1) == L"\"") && (right(1) == L"\"") && isDelimitedMonoBlock(useAngleBrace,useSlashStringEscape,useRepeatQuoteStringEscape);
}

inline bool CSStringW::isValidKeywordFirstChar(ucchar c)
{
	if (c >= L'a' && c <= L'z') return true;
	if (c >= L'A' && c <= L'Z') return true;
	if (c == L'_') return true;
	return false;
}

inline bool CSStringW::isValidKeywordChar(ucchar c)
{
	if (c >= L'a' && c <= L'z') return true;
	if (c >= L'A' && c <= L'Z') return true;
	if (c >= L'0' && c <= L'9') return true;
	if (c == L'_') return true;
	return false;
}

inline bool CSStringW::isHexDigit(ucchar c) 
{
	if (c >= L'0' && c <= L'9') return true;
	if (c >= L'A' && c <= L'F') return true;
	if (c >= L'a' && c <= L'f') return true;
	return false;
}

inline ucchar CSStringW::convertHexDigit(ucchar c) 
{
	if (c >= L'0' && c <= L'9') return c - L'0';
	if (c >= L'A' && c <= L'F') return c - L'A' + 10;
	if (c >= L'a' && c <= L'f') return c - L'a' + 10;
	return 0;
}

inline CSStringW& CSStringW::operator=(const char *s)
{
	*(ucstringbase *)this = SIPBASE::getUcsFromMbs(s);
	return *this;
}

inline CSStringW& CSStringW::operator=(const ucchar *s)
{
	*(ucstringbase *)this = s;
	return *this;
}

inline CSStringW& CSStringW::operator=(const std::string &s)
{
	*(ucstringbase *)this = SIPBASE::getUcsFromMbs(s.c_str());
	return *this;
}

inline CSStringW& CSStringW::operator=(const ucstringbase &s)
{
	*(ucstringbase *)this = s;
	return *this;
}

inline CSStringW& CSStringW::operator=(char c)
{
	*(ucstringbase *)this = (uint8)c;
	return *this;
}

inline CSStringW& CSStringW::operator=(ucchar c)
{
	*(ucstringbase *)this = c;
	return *this;
}

inline bool CSStringW::operator==(const CSStringW &other) const
{
	return sipstricmp(c_str(),other.c_str())==0;
}

inline bool CSStringW::operator==(const ucstringbase &other) const
{
	return sipstricmp(c_str(),other.c_str())==0;
}

inline bool CSStringW::operator==(const ucchar* other) const
{
	return sipstricmp(c_str(),other)==0;
}

inline bool CSStringW::operator!=(const CSStringW &other) const
{
	return sipstricmp(c_str(),other.c_str())!=0;
}

inline bool CSStringW::operator!=(const ucstringbase &other) const
{
	return sipstricmp(c_str(),other.c_str())!=0;
}

inline bool CSStringW::operator!=(const ucchar* other) const
{
	return sipstricmp(c_str(),other)!=0;
}

inline bool CSStringW::operator<=(const CSStringW &other) const
{
	return sipstricmp(c_str(),other.c_str())<=0;
}

inline bool CSStringW::operator<=(const ucstringbase &other) const
{
	return sipstricmp(c_str(),other.c_str())<=0;
}

inline bool CSStringW::operator<=(const ucchar* other) const
{
	return sipstricmp(c_str(),other)<=0;
}

inline bool CSStringW::operator>=(const CSStringW &other) const
{
	return sipstricmp(c_str(),other.c_str())>=0;
}

inline bool CSStringW::operator>=(const ucstringbase &other) const
{
	return sipstricmp(c_str(),other.c_str())>=0;
}

inline bool CSStringW::operator>=(const ucchar* other) const
{
	return sipstricmp(c_str(),other)>=0;
}

inline bool CSStringW::operator>(const CSStringW &other) const
{
	return sipstricmp(c_str(),other.c_str())>0;
}

inline bool CSStringW::operator>(const ucstringbase &other) const
{
	return sipstricmp(c_str(),other.c_str())>0;
}

inline bool CSStringW::operator>(const ucchar* other) const
{
	return sipstricmp(c_str(),other)>0;
}

inline bool CSStringW::operator<(const CSStringW &other) const
{
	return sipstricmp(c_str(),other.c_str())<0;
}

inline bool CSStringW::operator<(const ucstringbase &other) const
{
	return sipstricmp(c_str(),other.c_str())<0;
}

inline bool CSStringW::operator<(const ucchar* other) const
{
	return sipstricmp(c_str(),other)<0;
}

inline ucstringbase::const_reference CSStringW::operator[](ucstringbase::size_type idx) const
{
	static ucchar zero=0;
	if (idx >= size())
		return zero;
	return data()[idx];
}

inline ucstringbase::reference CSStringW::operator[](ucstringbase::size_type idx)
{
	static ucchar zero=0;
	if (idx >= size())
		return zero;
	return const_cast<ucstringbase::value_type&>(data()[idx]);
}


inline bool CSStringW::icompare(const ucstringbase &other) const
{
	return sipstricmp(c_str(),other.c_str())<0;
}

inline void CSStringW::serial( SIPBASE::IStream& s )
{
	s.serial( reinterpret_cast<ucstringbase&>( *this ) );
}

/*
inline CSStringW operator+(const CSStringW& s0,ucchar s1)
{
	return CSStringW(s0)+s1;
}

inline CSStringW operator+(const CSStringW& s0,const ucchar* s1)
{
	return CSStringW(s0)+s1;
}

inline CSStringW operator+(const CSStringW& s0,const ucstringbase& s1)
{
	return CSStringW(s0)+s1;
}
inline CSStringW operator+(const CSStringW& s0,const CSStringW& s1)
{
	return CSStringW(s0)+s1;
}
*/
inline CSStringW operator+(ucchar s0,const CSStringW& s1)
{
	return CSStringW(s0)+s1;
}

inline CSStringW operator+(const ucchar* s0,const CSStringW& s1)
{
	return CSStringW(s0)+s1;
}

inline CSStringW operator+(const ucstringbase& s0,const CSStringW& s1)
{
	return s0+static_cast<const ucstringbase&>(s1);
}

} // SIPBASE

// *** The following was commented out by Sadge because there were strange compilation/ link issues ***
// *** The '<' operator was implemented instead ***
//_STLP_BEGIN_NAMESPACE
//namespace std
//{
//	/*
//	 * less<CSString> is case insensitive
//	 */
//	template <>
//	struct less<SIPBASE::CSString> : public std::binary_function<SIPBASE::CSString, SIPBASE::CSString, bool>
//	{
//		bool operator()(const SIPBASE::CSString& x, const SIPBASE::CSString& y) const { return x.icompare(y); }
//	};
//} // std
//_STLP_END_NAMESPACE
//namespace std
//{

//	/*
//	 * less<CSString> is case insensitive
//	 */
//	template <>
//	struct less<SIPBASE::CSString> : public std::binary_function<SIPBASE::CSString, SIPBASE::CSString, bool>
//	{
//		bool operator()(const SIPBASE::CSString& x, const SIPBASE::CSString& y) const { return x.icompare(y); }
//	};
//} // std
//_STLP_END_NAMESPACE

/** 
  * Instead of overriding std::less, please use the following predicate. 
  * For example, declare your map as: 
  *   std::map<SIPBASE::CSString, CMyDataClass, SIPBASE::CUnsensitiveSStringLessPred> MyMap; 
  * Caution: a map declared without CUnsensitiveSStringLessPred will behave as a 
  * standard string map. 
  * 
  * \see also CUnsensitiveStrLessPred in misc/string_conversion.h 
  * for a similar predicate with std::string. 
  */ 
struct CUnsensitiveSStringLessPred : public std::less<SIPBASE::CSStringA> 
{ 
	bool operator()(const SIPBASE::CSStringA& x, const SIPBASE::CSStringA& y) const { return x < y; /*.icompare(y);*/ } 
}; 

struct CUnsensitiveSStringLessPredW : public std::less<SIPBASE::CSStringW> 
{ 
	bool operator()(const SIPBASE::CSStringW& x, const SIPBASE::CSStringW& y) const { return x < y; /*.icompare(y);*/ } 
};

#endif // __SSTRING_H__

/* End of sstring.h */
