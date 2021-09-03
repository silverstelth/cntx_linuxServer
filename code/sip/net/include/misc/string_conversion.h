/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STRING_CONVERSION_H__
#define __STRING_CONVERSION_H__

#include "types_sip.h"
#include "common.h"
#include <map>

namespace SIPBASE
{
// a predicate use to do unsensitive string comparison (with operator <)
struct CUnsensitiveStrLessPred
{
	bool operator()(const std::string &lhs, const std::string &rhs) const
	{
		return sipstricmp(lhs, rhs) < 0;
	}
};


/** This class allow simple mapping between string and other type (such as integral types or enum)
  * In fact this primarily intended to make a string / enum correspondance
  * Example of use :
  *
  * // An enumerated type
  * enum TMyType { Foo = 0, Bar, FooBar, Unknown };
  *
  * // The conversion table
  * static const CStringConversion<TMyType>::CPair stringTable [] =
  * { 
  *   { "Foo", Foo },
  *   { "Bar", Bar },
  *   { "FooBar", FooBar }
  * };
  *
  * // The helper object for conversion (instance of this class)
  *	static const CStringConversion conversion(stringTable, sizeof(stringTable) / sizeof(stringTable[0]),  Unknown);
  *
  * // Some conversions request
  * TMyType value1 = conversion.fromString("foo");  // returns 'foo'
  * value1 = conversion.fromString("Foo");          // returns 'foo' (this is case unsensitive by default)
  * std::string str = conversion.toString(Bar)      // returns "Bar"
  *
  * NB : Please note that that helpers macros are provided to build such a table in an easy way
  *      \see SIP_BEGIN_STRING_CONVERSION_TABLE
  *      \see SIP_END_STRING_CONVERSION_TABLE
  *
  *
  * NB: by default this class behaves in a case unsensitive way. To change this, just change the 'Pred' template parameter 
  *     to std::less<std::string>
  *
  * \TODO Use CTwinMap for implementation
  *
  * \date 2003
  */

template <class DestType, class Pred = CUnsensitiveStrLessPred>
class CStringConversion
{
public:
	typedef DestType TDestType;
	typedef Pred     TPred;
	struct CPair
	{
		const char *Str;
		TDestType   Value;
	};
public:
	// init from pairs of string / value
	CStringConversion(const CPair *pairs, uint numPairs, const DestType &notFoundValue);

	// add a pair in the array
	void insert(const char *str, TDestType value);
		
	// From a string, retrieve the associated value, or the 'notFoundValue' provided to the ctor of this object if the string wasn't found
	const TDestType &fromString(const std::string &str) const;

	// From a value, retrieve the associated string, or an empty string if not found
	const std::string &toString(const TDestType &value) const;

	// nb of pairs in the map
	inline uint16 getNbPairs() const { return _String2DestType.size(); }

	// Check a value against the list a value, return true if the value exist in the container
	bool isValid(const DestType &value) const;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
private:
	typedef std::map<std::string, TDestType, TPred> TString2DestType;
	typedef std::map<TDestType, std::string>        TDestType2String;
private:
	TString2DestType _String2DestType;
	TDestType2String _DestType2String;
	TDestType        _NotFoundValue;
};

/** This macro helps building a string conversion table
  * Example of use :
  *
  * // The enumerated type for which a conversion should be defined  
  * enum TMyType { Foo = 0, Bar, FooBar, Unknown };
  *
  * // The conversion table
  * SIP_BEGIN_STRING_CONVERSION_TABLE(TMyType)
  *	  SIP_STRING_CONVERSION_TABLE_ENTRY(Foo)
  *	  SIP_STRING_CONVERSION_TABLE_ENTRY(Bar)
  *	  SIP_STRING_CONVERSION_TABLE_ENTRY(FooBar)
  * SIP_END_STRING_CONVERSION_TABLE(TMyType, myConversionTable, Unknown)
  *
  * // Now, we can use the 'myConversionTable' intance
  *
  * std::string str = myConversionTable.toString(Bar)      // returns "Bar"
  * 
  */
#define SIP_BEGIN_STRING_CONVERSION_TABLE(__type)                                               \
static const SIPBASE::CStringConversion<__type>::CPair __type##_sip_string_conversion_table[] =          \
{                                                                                              
#define SIP_END_STRING_CONVERSION_TABLE(__type, __tableName, __defaultValue)                    \
};                                                                                           \
SIPBASE::CStringConversion<__type>                                                                \
__tableName(__type##_sip_string_conversion_table, sizeof(__type##_sip_string_conversion_table)   \
			/ sizeof(__type##_sip_string_conversion_table[0]),  __defaultValue);
#define SIP_STRING_CONVERSION_TABLE_ENTRY(val) { #val, val},


//////////////////////////////////////
// CStringConversion implementation //
//////////////////////////////////////

//=================================================================================================================
/** CStringConversion ctor
  */
template <class DestType, class Pred>
CStringConversion<DestType, Pred>::CStringConversion(const CPair *pairs, uint numPairs, const DestType &notFoundValue)
{
	for(uint k = 0; k < numPairs; ++k)
	{
		_String2DestType[pairs[k].Str] = pairs[k].Value;
		_DestType2String[pairs[k].Value] = pairs[k].Str;
	}
	_NotFoundValue = notFoundValue;
}

//
template <class DestType, class Pred>
void CStringConversion<DestType, Pred>::insert(const char *str, TDestType value)
{
	_String2DestType[str] = value;
	_DestType2String[value] = str;
}


//=================================================================================================================
template <class DestType, class Pred>
const DestType &CStringConversion<DestType, Pred>::fromString(const std::string &str) const
{
	typename TString2DestType::const_iterator it = _String2DestType.find(str);
	if (it == _String2DestType.end())
	{		
		return _NotFoundValue;
	}
	else
	{
		return it->second;
	}
}

//=================================================================================================================
template <class DestType, class Pred>
const std::string &CStringConversion<DestType, Pred>::toString(const DestType &value) const
{
	typename TDestType2String::const_iterator it = _DestType2String.find(value);
	if (it == _DestType2String.end())
	{
		static std::string emptyString;		
		return emptyString;
	}
	else
	{
		return it->second;
	}
}


//=================================================================================================================
template <class DestType, class Pred>
bool CStringConversion<DestType, Pred>::isValid(const DestType &value) const
{
	typename TDestType2String::const_iterator it = _DestType2String.find(value);

	return it != _DestType2String.end();
}

} // SIPBASE

#endif

