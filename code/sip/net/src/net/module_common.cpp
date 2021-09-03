/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"
#include "misc/sstring.h"
#include "net/module_common.h"

using namespace std;
using namespace SIPBASE;

namespace SIPNET
{

	TParsedCommandLine::~TParsedCommandLine()
	{
		clear();
	};

	void TParsedCommandLine::clear()
	{
		for (std::vector<TParsedCommandLine*>::iterator it=SubParams.begin(); it!=SubParams.end(); ++it)
		{
			delete (*it);
		}
		SubParams.clear();
		ParamName.clear();
		ParamValue.clear();
	}

	bool TParsedCommandLine::parseParamList(const std::string &rawParamString)
	{
		// Cleanup the struct
		clear();

		return _parseParamList(rawParamString);
	}

	std::string TParsedCommandLine::toString() const
	{
		string ret;

		ret = ParamName;
		if (!ParamValue.empty())
		{
			ret += " = "+ParamValue;
		}

		if (!SubParams.empty())
		{
			ret += " ( ";

			for (uint i=0; i<SubParams.size(); ++i)
			{
				if (i >0)
					ret += " ";
				ret += SubParams[i]->toString();
			}

			ret += " ) ";
		}

		return ret;
	}



	bool TParsedCommandLine::_parseParamList(const std::string &rawParamString)
	{
		CSStringA parsedString(rawParamString);
		
		for (CSStringA part = parsedString.strtok(" \t", true, false);
			!part.empty();
			part = parsedString.strtok(" \t", true, false))
		{
			if (part[0] == '(')
			{
				// this is a sub parameter list
				if (SubParams.empty() || SubParams.back()->ParamName.empty())
				{
					sipwarning("While parsing param string '%s', missing param header", rawParamString.c_str());
					return false;
				}
				if (!SubParams.back()->ParamValue.empty())
				{
					sipwarning("While parsing param string '%s', Invalid sub param header '%s' for sub part '%s', must not define value",
						rawParamString.c_str(),
						SubParams.back()->ParamName.c_str(),
						part.c_str());
					return false;
				}

				if (part[part.size()-1] != ')')
				{
					sipwarning("While parsing param string '%s', Invalid sub param value '%s' missing closing ')'",
						rawParamString.c_str(),
						part.c_str());

					return false;
				}

				part = part.stripBlockDelimiters();

				if (!SubParams.back()->_parseParamList(part))
				{
					sipwarning("Error parsing sub param list for header '%s' in '%s'", 
						SubParams.back()->ParamName.c_str(),
						rawParamString.c_str());
					return false;
				}
			}
			else if (part[part.size()-1] == ')')
			{
				sipwarning("While parsing param string '%s', Invalid param value '%s' : missing openning '('",
					rawParamString.c_str(),
					part.c_str());

				return false;
			}
			else if (part[0] == '\"')
			{
				// this is a quoted parameter value
				if (SubParams.empty() || !SubParams.back()->ParamValue.empty())
				{
					sipwarning("While parsing param string '%s', param '%s' already have the value '%s'", 
						rawParamString.c_str(),
						SubParams.back()->ParamName.c_str(),
						SubParams.back()->ParamValue.c_str());
					return false;
				}
				SubParams.back()->ParamValue = part.unquote();
			}
			else
			{
				// this is a simple param
				CSStringA name = part.splitTo('=', true, true);
				if (name.empty())
				{
					sipwarning("Can't find param name for value '%s' in the param string '%s'",
						part.c_str(),
						rawParamString.c_str());
					return false;
				}
				CSStringA value = part.strtok("=");

				SubParams.push_back( new TParsedCommandLine() );
				SubParams.back()->ParamName = name;
				SubParams.back()->ParamValue = value;
			}
		}

		return true;
	}

	const TParsedCommandLine *TParsedCommandLine::getParam(const std::string &name) const
	{
		vector<string>	parts;
		SIPBASE::explode(name, ".", parts);

		return _getParam(parts.begin(), parts.end());
	}

	void TParsedCommandLine::setParam(const std::string &name, const std::string &value)
	{
		vector<string>	parts;
		SIPBASE::explode(name, ".", parts);

		if (name.size() > 0)
		{
			// at least one part in the name
			// check if sub ojbcct exist
			TParsedCommandLine *sub = _getParam(parts.begin(), (parts.begin()+1));
			if (sub == NULL)
			{
				TParsedCommandLine * newElem = new TParsedCommandLine();
				newElem->ParamName = parts[0];
				SubParams.push_back(newElem);
				sub = SubParams.back();
			}

			if (name.size() > 0)
			{
				// name is more deep, need to recurse
				parts.erase(parts.begin());
				CSStringA subName;
				subName.join(reinterpret_cast<CVectorSStringA&>(parts), ".");
				sub->setParam(subName, value);
			}
			else 
			{
				// last level, set the value
				sub->ParamValue = value;
			}
		}
	}

	const TParsedCommandLine *TParsedCommandLine::_getParam(std::vector<std::string>::iterator it, std::vector<std::string>::iterator end) const
	{
		return const_cast<TParsedCommandLine&>(*this)._getParam(it, end);
	}

	TParsedCommandLine *TParsedCommandLine::_getParam(std::vector<std::string>::iterator it, std::vector<std::string>::iterator end)
	{
		if (it == end)
		{
			// end of recursion, we found the searched param
			return this;
		}

		// look for sub param
		for (uint i=0; i<SubParams.size(); ++i)
		{
			if (SubParams[i]->ParamName == *it)
				return SubParams[i]->_getParam(++it, end);
		}

		// parameter not found
		return NULL;
	}


} // namespace SIPBASE
