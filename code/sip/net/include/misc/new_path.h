/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __NEW_PATH_H__
#define __NEW_PATH_H__

#if 0

#include "types_sip.h"
#include <map>
#include <string>
#include <vector>

namespace SIPBASE {

/**
 * Utility class for searching files in differents paths.
 * \warning addSearchPath(), clearMap() and remapExtension() are not reentrant.
 * \warning all path and files are *case sensitive* on linux.
 * \date 2001
 */
class CNewPath
{
	SIP_SAFE_SINGLETON(CNewPath);
public:
	/** Adds a search path.
     * The path is a directory "c:/temp" all files in the directory will be included (and recursively if asked)
	 *
	 * Alternative directories are not precached (instead of non Alternative files) and will used when a file is not found in the standard directories.
	 * For example, local data will be in the cached directories and server repository files will be in the Alternative files. If a new file is not
	 * found in the local data, we'll try to find it on the repositrory.
	 *
	 * When Alternative is false, all added file names must be uniq or a warning will be display. In the Alternative directories, it could have
	 * more than one file with the same name.
	 *
	 * \param path the path name. The separator for directories could be '/' or '\' (bit '\' will be translate into '/' in the function).
	 * \param recurse true if you want the function recurse in sub-directories.
	 * \param Alternative true if you want to add the path in the Alternative directories.
	 */
	static void			addSearchPath (const std::string &path, bool recurse, bool alternative);

	/** Used only for compatibility with the old CPath. In this case, we don't use the map to have the same behavior as the old CPath */
	static void			addSearchPath (const std::string &path) { addSearchPath (path, false, true); }

	/** Same as AddSearchPath but with a file "c:/autoexec.bat" this file only will included. wildwards *doesn't* work */
	static void			addSearchFile (const std::string &file, bool remap = false, const std::string &virtual_ext = "");

	/** Same as AddSearchPath but with a path file "c:/test.pth" all files name contain in this file will be included (the extention is used to know that it's a path file) */
	static void			addSearchListFile (const std::string &filename, bool recurse, bool alternative);
	
	/** Same as AddSearchPath but with a big file "c:/test.nbf" all files name contain in the big file will be included  (the extention (Sip Big File) is used to know that it's a big file) */
	static void			addSearchBigFile (const std::string &filename, bool recurse, bool alternative);

	/** Returns the long name (path + filename) for the specified file.
	 * The directory separator is always '/'.
	 * If no search path was provided, lookup() will *not* scan in the current directory (You have to add the "." path if you want).
	 * First the function lookups in standard directories (Alternative=false).
	 * If not found, it lookups in the Alternative directories.
	 * If not found the lookup() returns empty string "".
	 * 
	 * \param filename the file name you are seeking. (ex: "test.txt")
	 * \return empty string if file is not found or the full path + file name (ex: "c:/temp/test.txt");
	 */
	static std::string	lookup (const std::string &filename);

	/** Clears the map that contains all cached files (Use this function to take into account new files).
	 */
	static void clearMap ();

	/** Add a remapping function to allow file extension substitution.
	 * - eg RemapExtension(".DDS",".TGA",TRUE) Where the boolean indicates whether
	 * the ".DDS" should replace a ".TGA" if one exists - again - a warning should
	 * be generated if the two are present.
	 */
	static void remapExtension (const std::string &ext1, const std::string &ext2, bool substitute);

	static void display ();


private:

//	static CNewPath *getInstance ();


//	static CNewPath *_Instance;

private:

	std::vector<std::string> _Paths;

	// All path in this vector must have a terminated '/'
	std::vector<std::string> _AlternativePaths;

	struct CNewFileEntry
	{
		CNewFileEntry (std::string	path, bool remapped, std::string ext) : Path(path), Remapped(remapped), Extension(ext) { }
		std::string	Path;
		bool		Remapped;		// true if the file is remapped
		std::string	Extension;		// extention of the faile
	};

	/** first is the filename, second the full path for the filename.
	 * Due to the remapping, first and second.path could have different extention.
	 */
	std::map<std::string, CNewFileEntry> _Files;

	/// first ext1, second ext2 (ext1 could remplace ext2)
	std::vector<std::pair<std::string, std::string> > _Extensions;

	sint		findExtension (const std::string &ext1, const std::string &ext2);
	static std::string	standardizePath (const std::string &path, bool addFinalSlash = true);
	static void		getPathContent (const std::string &path, bool recurse, bool wantDir, bool wantFile, std::vector<std::string> &result);
	static void		insertFileInMap (const std::string &filename, const std::string &filepath, bool remap, const std::string &extension);
};



/**
 * Utility class for file manipulation
 * \date 2001
 */
struct CNewFile
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
	 * Return true if the file OR directory exists.
	 * Warning: this test will also tell that the file does not
	 * exist if you don't have the rights to read it (Unix).
	 */
	static bool isExists (const std::string& filename);

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

	static std::string getFilenameWithoutExtension (const std::string &filename);
	static std::string getExtension (const std::string &filename);

};




} // SIPBASE

#endif

#endif // __NEW_PATH_H__
