/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __OLD_PATH_H__
#define __OLD_PATH_H__

#if 0

#include "types_sip.h"
#include "common.h"
#include <string>
#include <vector>

namespace SIPBASE {


/// Vectors of strings
typedef std::vector<std::string> CStringVector;


// EPathNotFound
struct EPathNotFound : public Exception
{
	EPathNotFound( const std::string& filename ) : Exception( "Path not found for " + filename ) {}
};


/**
 * Utility class for search paths
 * \date 2000
 */
class CPath
{
public:

	/// Adds a search path. The separator for directories is '/'.
	static void			addSearchPath( const std::string& path );

	/** Returns the long name (path and filename) for the specified file, trying first the local path, then 
	 * using search paths stored by addSearchPath in the same order as they were added.
	 * If no path is found where path/file exists, an exception EPathNotFound is raised if the boolean is true.
	 */
	static std::string	lookup( const std::string& filename, bool throwException = true );

private:

	static CStringVector	_SearchPaths;
};


/**
 * Utility class for file manipulation
 * \date 2000
 */
struct CFileA
{
	/**
	 * Retrieve the associated file name.
	 * An empty string is returned if the path is invalid
	 */
	static std::string getFilename (const std::string &filename);

	/**
	 * Retrieve the associated file path with the trailing slash.
	 * Returns an empty string if the path is invalid
	 */
	static std::string getPath (const std::string &filename);

	/**
	 * Just to know if it is a directory.
	 * _FileName empty and path not !!!
	 */
	static bool isDirectory (const std::string &filename);

	/**
	 * Return true if the file exists.
	 * Warning: this test will also tell that the file does not
	 * exist if you don't have the rights to read it (Unix).
	 */
	static bool fileExists (const std::string &filename);

	/**
	 * Return a new filename that doesn't exists. It's used for screenshot filename for example.
	 * example: findNewFile("foobar.tga");
	 * will try foobar001.tga, if the file exists, try foobar002.tga and so on until it finds an unexistant file.
	 */
	static std::string findNewFile (const std::string &filename);

	/**
	 * Return the position between [begin,end[ of the last separator between path and filename ('/' or '\').
	 * If there's no separator, it returns string::npos.
	 */
	static int getLastSeparator (const std::string &filename);
};


} // SIPBASE

#endif

#endif // __OLD_PATH_H__