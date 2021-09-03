
#include "ParseArgs.h"

using namespace std;
using namespace SIPBASE;

SIPBASE::CVectorSStringA	CParseArgs::_Args;

bool CParseArgs::haveArg (char argName)
{
	for (uint32 i = 0; i < _Args.size(); i++)
	{
		if (_Args[i].size() >= 2 && _Args[i][0] == '-')
		{
			if (_Args[i][1] == argName)
			{
				return true;
			}
		}
	}
	return false;
}

string CParseArgs::getArg (char argName)
{
	for (uint32 i = 0; i < _Args.size(); i++)
	{
		if (_Args[i].size() >= 2 && _Args[i][0] == '-')
		{
			if (_Args[i][1] == argName)
			{
				/* Remove the first and last '"' : 
				-c"C:\Documents and Settings\toto.tmp" 
				will return :
				C:\Documents and Settings\toto.tmp
				*/
				uint begin = 2;
				if (_Args[i].size() < 3)
					return "";
				//throw Exception ("Parameter '-%c' is malformed, missing content", argName);

				if (_Args[i][begin] == '"')
					begin++;

				// End
				uint size = _Args[i].size();

				if (size && _Args[i][size-1] == '"')
					size --;

				size = (uint)(std::max((int)0, (int)size-(int)begin));
				return _Args[i].substr(begin, size);
			}
		}
	}
	throw Exception ("Parameter '-%c' is not found in command line", argName);
}

bool CParseArgs::haveLongArg (const char* argName)
{
	for (uint32 i = 0; i < _Args.size(); i++)
	{
		if (_Args[i].left(2) == "--" && _Args[i].leftCrop(2).splitTo('=') == argName)
		{
			return true;
		}
	}
	return false;
}

string CParseArgs::getLongArg (const char* argName)
{
	for (uint32 i = 0; i < _Args.size(); i++)
	{
		if (_Args[i].left(2)=="--" && _Args[i].leftCrop(2).splitTo('=')==argName)
		{
			SIPBASE::CSStringA val= _Args[i].splitFrom('=');
			if (!val.empty())
			{
				return val.unquoteIfQuoted();
			}
			if (i+1<_Args.size() && _Args[i+1].c_str()[0]!='-')
			{
				return _Args[i+1].unquoteIfQuoted();
			}
			return std::string();
		}
	}
	return std::string();
}

void CParseArgs::setArgs (const char *args)
{
	_Args.push_back ("<ProgramName>");

	string sargs (args);
	size_t pos1 = 0, pos2 = 0;

	do
	{
		// Look for the first non space character
		pos1 = sargs.find_first_not_of (" ", pos2);
		if (pos1 == string::npos) break;

		// Look for the first space or "
		pos2 = sargs.find_first_of (" \"", pos1);
		if (pos2 != string::npos)
		{
			// " ?
			if (sargs[pos2] == '"')
			{
				// Look for the final \"
				pos2 = sargs.find_first_of ("\"", pos2+1);
				if (pos2 != string::npos)
				{
					// Look for the first space
					pos2 = sargs.find_first_of (" ", pos2+1);
				}
			}
		}

		// Compute the size of the string to extract
		size_t length = (pos2 != string::npos) ? pos2-pos1 : string::npos;

		string tmp = sargs.substr (pos1, length);
		_Args.push_back (tmp);
	}
	while (pos2 != string::npos);
}

void CParseArgs::setArgs (int argc, const char **argv)
{
	for (sint i = 0; i < argc; i ++)
	{
		_Args.push_back (argv[i]);
	}
}
