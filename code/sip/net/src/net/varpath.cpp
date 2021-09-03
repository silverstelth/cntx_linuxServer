/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "stdnet.h"

#include "misc/types_sip.h"

#include <cstdio>
#include <cctype>
#include <cmath>

#include <vector>
#include <map>

#include "misc/debug.h"
#include "misc/config_file.h"
#include "misc/displayer.h"
#include "misc/log.h"

#include "net/varpath.h"


//
// Namespaces
//

using namespace std;
using namespace SIPBASE;


//
// Variables
//



//
// Functions
//

/**
 *
 * VarPath ::= [bloc '.']* bloc
 * bloc    ::= '[' [VarPath ',']* VarPath ']'  |  name
 * name    ::= [ascii]* ['*']
 *
 *
 */
/*bool CVarPath::getDest (uint level, vector<string> &dest)
{
	return true;
}*/

string CVarPath::getToken ()
{
	string res;
	
	if (TokenPos >= RawVarPath.size())
		return res;

	res += RawVarPath[TokenPos];

	switch (RawVarPath[TokenPos++])
	{
	case '.': case '*': case '[': case ']': case ',': case '=': case ' ':
		break;
	default :
		while (TokenPos < RawVarPath.size() && RawVarPath[TokenPos] != '.' && RawVarPath[TokenPos] != '[' && RawVarPath[TokenPos] != ']' && RawVarPath[TokenPos] != ',' && RawVarPath[TokenPos] != '=' && RawVarPath[TokenPos] != ' ')
		{
			res += RawVarPath[TokenPos++];
		}
		break;
	}
	return res;
}


void CVarPath::decode ()
{
	vector<string> dest;
	TokenPos = 0;
	Destination.clear ();

	string val = getToken ();

	if (val == "")
		return;
		
	if (val == "[" )
	{
		do
		{
			uint osbnb = 0;
			string d;
			do
			{
				val = getToken ();
				if (val == "[")
					osbnb++;

				// end of token
				if (val == "")
				{
					sipwarning ("VP: Bad VarPath '%s', suppose it s an empty varpath", RawVarPath.c_str());
					Destination.clear ();
					return;
				}
					
				if (osbnb == 0 && (val == "," || val == "]"))
					break;

				if (val == "]")
					osbnb--;

				d += val;
			}
			while (true);
			dest.push_back (d);
			if (val == "]")
				break;
		}
		while (true);
	}
	else if (val != "." && val != "," && val != "]")
	{
		dest.push_back (val);
	}
	else
	{
		sipwarning ("VP: Malformated VarPath '%s' before position %d", RawVarPath.c_str (), TokenPos);
		return;
	}

	// must be a . or end of string
	val = getToken ();
	if (val == " ")
	{
		// special case, there s a space that means that everything after is not really part of the varpath.
		Destination.push_back (make_pair(RawVarPath, string("")));
		return;
	}
	else if (val != "." && val != "" && val != "=")
	{
		sipwarning ("VP: Malformated VarPath '%s' before position %d", RawVarPath.c_str (), TokenPos);
		return;
	}

	for (uint i = 0; i < dest.size(); ++i)
	{
		string srv, var;
/*
		uint pos;
		if ((pos = dest[i].find ('.')) != string::npos)
		{
			srv = dest[i].substr(0, pos);
			var = dest[i].substr(pos+1);
			if (TokenPos < RawVarPath.size())
				var += val + RawVarPath.substr (TokenPos);
		}
		else
*/
		{
			srv = dest[i];
			if (val == "=")
			{
				srv += val + RawVarPath.substr (TokenPos);
				var = "";
			}
			else
				var = RawVarPath.substr (TokenPos);
		}

		Destination.push_back (make_pair(srv, var));
	}

	//display ();
}

bool CVarPath::isFinal ()
{
	if(Destination.size() == 0) return true;
	if(Destination[0].second.size() == 0) return true;
	return false;
}

void CVarPath::display ()
{
	sipinfo ("VP: VarPath dest = %d", Destination.size ());
	for (uint i = 0; i < Destination.size (); i++)
	{
		sipinfo ("VP:  > '%s' '%s'", Destination[i].first.c_str(), Destination[i].second.c_str());
	}
}

SIPBASE_CATEGORISED_COMMAND(sip, varPath, "Test a varpath (for debug purpose)", "<rawvarpath>")
{
	if(args.size() != 1) return false;

	CVarPath vp (args[0]);

	log.displayNL ("VarPath contains %d destination", vp.Destination.size ());
	for (uint i = 0; i < vp.Destination.size (); i++)
	{
		log.displayNL ("Dest '%s' value '%s'", vp.Destination[i].first.c_str(), vp.Destination[i].second.c_str());
	}
	log.displayNL ("End of varpath");
	
	return true;
}
