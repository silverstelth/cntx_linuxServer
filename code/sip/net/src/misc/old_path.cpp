/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#if 0

#include "misc/path.h"
#include "misc/debug.h"
#include <fstream>

using namespace std;


// Use this define if you want to display the absolute paths in the console.
//#define	SIP_DEBUG_PATH

#ifdef	SIP_DEBUG_PATH
#define	SIP_DISPLAY_PATH(_x_)	sipinfo("Path: %s", _x_.c_str())
#else 
#define	SIP_DISPLAY_PATH(_x_)	NULL
#endif


namespace SIPBASE
{

CStringVector CPath::_SearchPaths;

/*
 * Adds a search path
 */
void CPath::addSearchPath( const string& path )
{
	if ( path == "" )
	{
		return;
	}
	string s = path;
	const char slash = '/';

	// Add an ending slash if necessary
	if ( path[path.size()-1] != slash )
	{
		s += slash;
	}

	// Add path to the search paths
	_SearchPaths.push_back( s );
}


/* Returns the long name (path and filename) for the specified file, using search paths
 * stored by addSearchPath.
 */
string CPath::lookup( const string& filename, bool throwException )
{
	if(!filename.empty())
	{
		if ( CFileA::fileExists(filename) )
		{
			SIP_DISPLAY_PATH(filename);
			return filename;
		}
		CStringVector::iterator isv;
		string s;
		for ( isv=CPath::_SearchPaths.begin(); isv!=CPath::_SearchPaths.end(); ++isv )
		{
			s = *isv + filename;
			if ( CFileA::fileExists(s) )
			{
				SIP_DISPLAY_PATH(s);
				return s;
			}
		}
	}

	if (throwException)
		throw EPathNotFound( filename );

	return "";
}

//********************************* CFileA

int CFileA::getLastSeparator (const std::string &filename)
{
	int pos = filename.find_last_of ('/');
	if (pos == string::npos)
	{
		pos = filename.find_last_of ('\\');
	}
	return pos;
}

std::string CFileA::getFilename (const std::string &filename)
{
	int pos = CFileA::getLastSeparator(filename);
	if (pos != string::npos)
		return filename.substr (pos + 1);
	else
		return filename;
}

std::string CFileA::getPath (const std::string &filename)
{
	int pos = CFileA::getLastSeparator(filename);
	if (pos != string::npos)
		return filename.substr (0, pos + 1);
	else
		return filename;
}

bool CFileA::isDirectory (const std::string &filename)
{
	return (CFileA::getLastSeparator(filename) == string::npos);
}


bool CFileA::fileExists (const string& filename)
{
	/*FILE *f;
	if ( (f = fopen( filename.c_str(), "r" )) == NULL )
	{
		return false;
	}
	else
	{
		fclose( f );
		return true;
	}*/
	return ! ! fstream( filename.c_str(), ios::in ); // = ! fstream(...).fail()
}

string CFileA::findNewFile (const string &filename)
{
	int pos = filename.find_last_of ('.');
	if (pos == string::npos)
		return filename;
	
	string start = filename.substr (0, pos);
	string end = filename.substr (pos);

	uint num = 0;
	char numchar[4];
	string npath;
	do
	{
		npath = start;
		smprintf(numchar,4,"%03d",num++);
		npath += numchar;
		npath += end;
		if (!CFileA::fileExists(npath)) break;
	}
	while (num<999);
	return npath;
}

} // SIPBASE

#endif
