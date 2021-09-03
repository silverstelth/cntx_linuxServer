/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __VARPATH_H__
#define __VARPATH_H__

#include "misc/types_sip.h"

class CVarPath
{
public:
	
	CVarPath (const std::string &raw) : RawVarPath(raw)
	{
		decode ();
	}

	void decode ();

	std::vector<std::pair<std::string, std::string> > Destination;

	void display ();

	/// returns true if there s no more . in the path
	bool isFinal ();

	bool empty ()
	{
		return Destination.empty();
	}

private:

	std::string getToken ();

	std::string RawVarPath;

	uint32 TokenPos;

};


#endif // __VARPATH_H__

/* End of varpath.h */
