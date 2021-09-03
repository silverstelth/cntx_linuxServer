/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STRING_ID_ARRAY_H__
#define __STRING_ID_ARRAY_H__

#include "types_sip.h"

#include <cmath>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

#include "debug.h"

namespace SIPBASE {


/**
 * The goal of this class is to associate number and string. It is used in the
 * CCallbackNetBase for message id<->string associations.
 * \date 2001
 */
class CStringIdArray
{
public:

	typedef sint16 TStringId;

	CStringIdArray () : _IgnoreAllUnknownId(false) { }

	/// Adds a string in the string in the array
	void addString(const std::string &str, TStringId id)
	{
		sipassert (id >= 0 && id < pow(2.0, (sint)sizeof (TStringId)*8));
		sipassert (!str.empty());

		if (id >= (sint32) _StringArray.size())
			_StringArray.resize(id+1);

		_StringArray[id] = str;

		if (!_IgnoreAllUnknownId)
		{
			_AskedStringArray.erase (str);
			_NeedToAskStringArray.erase (str);
		}
	}

	/// Adds a string in the string at the end of the array
	void addString(const std::string &str)
	{
		sipassert (_StringArray.size () < pow(2.0, (sint)sizeof (TStringId)*8));
		sipassert (!str.empty());

		// add at the end
		addString (str, _StringArray.size ());
	}

	/** Returns the id associated to string str. If the id is not found, the string will be added in
	 * _NeedToAskStringArray if ignoreAllUnknownId is false and IgnoreIfUnknown is false too.
	 */
	TStringId getId (const std::string &str, bool IgnoreIfUnknown = false)
	{
		sipassert (!str.empty());

		// sorry for this bullshit but it's the simplest way ;)
		if (this == NULL) return -1;

		for (TStringId i = 0; i < (TStringId) _StringArray.size(); i++)
		{
			if (_StringArray[i] == str)
				return i;
		}

		if (!_IgnoreAllUnknownId && !IgnoreIfUnknown)
		{
			// the string is not found, add it to the _AskedStringArray if necessary
			if (_NeedToAskStringArray.find (str) == _NeedToAskStringArray.end ())
			{
				if (_AskedStringArray.find (str) == _AskedStringArray.end ())
				{
					//sipdebug ("String '%s' not found, add it to _NeedToAskStringArray", str.c_str ());	
					_NeedToAskStringArray.insert (str);
				}
				else
				{
					//sipdebug ("Found '%s' in the _AskedStringArray", str.c_str ());	
				}
			}
			else
			{
				//sipdebug ("Found '%s' in the _NeedToAskStringArray", str.c_str ());	
			}
		}
		else
		{
			//sipdebug ("Ignoring unknown association ('%s')", str.c_str ());	
		}

		return -1;
	}

	/// Returns the string associated to this id
	std::string getString (TStringId id) const
	{
		// sorry for this bullshit but it's the simplest way ;)
		if (this == NULL) return "<NoSIDA>";

		sipassert (id >= 0 && id < (TStringId)_StringArray.size());

		return _StringArray[id];
	}

	/// Set the size of the _StringArray
	void resize (TStringId size)
	{
		_StringArray.resize (size);
	}

	/// Returns the size of the _StringArray
	TStringId size () const
	{
		return _StringArray.size ();
	}

	/// Returns all string in the _NeedToAskStringArray
	const std::set<std::string> &getNeedToAskedStringArray () const
	{
		return _NeedToAskStringArray;
	}

	/// Returns all string in the _AskedStringArray
	const std::set<std::string> &getAskedStringArray () const
	{
		return _AskedStringArray;
	}

	/// Moves string from _NeedToAskStringArray to _AskedStringArray
	void moveNeedToAskToAskedStringArray ()
	{
		if (!_IgnoreAllUnknownId)
		{
			_AskedStringArray.insert (_NeedToAskStringArray.begin(), _NeedToAskStringArray.end());
			_NeedToAskStringArray.clear ();
		}
	}

	/// If set to true, when we ask a string with no id, we don't put it in the _NeedToAskStringArray array
	void ignoreAllUnknownId (bool b) { _IgnoreAllUnknownId = b; }

	/// Clears the string id array
	void clear ()
	{
		_StringArray.clear ();
		_NeedToAskStringArray.clear ();
		_AskedStringArray.clear ();
	}

	/// Displays all association of the array in a C style (useful to copy/paste in your C code)
	void display ()
	{
		sipinfo ("static const char *OtherSideAssociations[] = {");
		for (uint i = 0; i < _StringArray.size(); i++)
		{
			sipinfo(" \"%s\",", _StringArray[i].c_str());
		}
		sipinfo ("};");
	}

private:

	bool _IgnoreAllUnknownId;

	std::vector<std::string>	_StringArray;

	std::set<std::string>	_NeedToAskStringArray;

	std::set<std::string>	_AskedStringArray;
};


} // SIPBASE


#endif // __STRING_ID_ARRAY_H__

/* End of string_id_array.h */
