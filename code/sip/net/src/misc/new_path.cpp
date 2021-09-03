/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#if 0

#include <fstream>
#include "misc/debug.h"
#include "misc/new_path.h"

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#else
#   include <sys/types.h>
#   include <sys/stat.h>
#	include <dirent.h>
#   include <unistd.h>
#endif // SIP_OS_WINDOWS

using namespace std;

namespace SIPBASE 
{

//
// Macros
//

// Use this define if you want to display info about the CPath.
#define	SIP_DEBUG_PATH

#ifdef	SIP_DEBUG_PATH
#define	SIP_DISPLAY_PATH	sipinfo
#else 
#define	SIP_DISPLAY_PATH if(false)
#endif


//
// Variables
//

CNewPath *CNewPath::_Instance = NULL;


//
// Functions
//

//CNewPath *CNewPath::getInstance ()
//{
//	if (_Instance == NULL)
//	{
//		_Instance = new CNewPath;
//	}
//	return _Instance;
//}

void CNewPath::clearMap ()
{
	CNewPath *inst = CNewPath::getInstance();
	inst->_Files.clear ();
	SIP_DISPLAY_PATH("CNewPath::clearMap(): map directory cleared");
}

sint CNewPath::findExtension (const string &ext1, const string &ext2)
{
	CNewPath *inst = CNewPath::getInstance();
	for (uint i = 0; i < inst->_Extensions.size (); i++)
	{
		if (inst->_Extensions[i].first == ext1 && inst->_Extensions[i].second == ext2)
		{
			return i;
		}
	}
	return -1;
}

void CNewPath::remapExtension (const string &ext1, const string &ext2, bool substitute)
{
	CNewPath *inst = CNewPath::getInstance();

	if (ext1.empty() || ext2.empty())
	{
		sipwarning ("CNewPath::remapExtension(%s, %s, %d): can't remap empty extension", ext1.c_str(), ext2.c_str(), substitute);
	}

	if (!substitute)
	{
		// remove the mapping from the mapping list
		sint n = inst->findExtension (ext1, ext2);
		sipassert (n != -1);
		inst->_Extensions.erase (inst->_Extensions.begin() + n);

		// remove mapping in the map
		map<string, CNewFileEntry>::iterator it = inst->_Files.begin();
		map<string, CNewFileEntry>::iterator nit = it;
		while (it != inst->_Files.end ())
		{
			nit++;
			if ((*it).second.Remapped && (*it).second.Extension == ext2)
			{
				inst->_Files.erase (it);
			}
			it = nit;
		}
		SIP_DISPLAY_PATH("CNewPath::remapExtension(%s, %s, %d): extension removed", ext1.c_str(), ext2.c_str(), substitute);
	}
	else
	{
		sint n = inst->findExtension (ext1, ext2);
		if (n != -1)
		{
			sipwarning ("CNewPath::remapExtension(%s, %s, %d): remapping already set", ext1.c_str(), ext2.c_str(), substitute);
			return;
		}

		// adding mapping into the mapping list
		inst->_Extensions.push_back (make_pair (ext1, ext2));

		// adding mapping into the map
		vector<string> newFiles;
		map<string, CNewFileEntry>::iterator it = inst->_Files.begin();
		while (it != inst->_Files.end ())
		{
			if (!(*it).second.Remapped && (*it).second.Extension == ext1)
			{
				// find if already exist
				sint pos = (*it).first.find_last_of (".");
				if (pos != string::npos)
				{
					string file = (*it).first.substr (0, pos + 1);
					file += ext2;

					map<string, CNewFileEntry>::iterator nit = inst->_Files.find (file);
					if (nit != inst->_Files.end())
					{
						sipwarning ("CNewPath::remapExtension(%s, %s): The file '%s' is in conflict with the remapping file '%s', skip it", ext1.c_str(), ext2.c_str(), file.c_str(), (*it).first.c_str());
					}
					else
					{
// TODO perhaps a problem because I insert in the current map that i parcours
						insertFileInMap (file, (*it).second.Path, true, ext2);
					}
				}
			}
			it++;
		}
		SIP_DISPLAY_PATH("CNewPath::remapExtension(%s, %s, %d): extension added", ext1.c_str(), ext2.c_str(), substitute);
	}
}

string CNewPath::lookup (const string &filename)
{
	// Try to find in the map directories
	CNewPath *inst = CNewPath::getInstance();
	map<string, CNewFileEntry>::iterator it = inst->_Files.find (filename);
	// If found in the map, returns it
	if (it != inst->_Files.end())
	{
		SIP_DISPLAY_PATH("CNewPath::lookup(%s): found in the map directory: '%s'", filename.c_str(), (*it).second.Path.c_str());
		return (*it).second.Path;
	}
	
	// Try to find in the alternative directories
	for (uint i = 0; i < inst->_AlternativePaths.size(); i++)
	{
		string s = inst->_AlternativePaths[i] + filename;
		if ( CNewFile::fileExists(s) )
		{
			SIP_DISPLAY_PATH("CNewPath::lookup(%s): found in the alternative directory: '%s'", filename.c_str(), s.c_str());
			return s;
		}
		
		// try with the remapping
		for (uint j = 0; j < inst->_Extensions.size(); j++)
		{
			string rs = inst->_AlternativePaths[i] + CNewFile::getFilenameWithoutExtension (filename) + "." + inst->_Extensions[j].first;
			if ( CNewFile::fileExists(rs) )
			{
				SIP_DISPLAY_PATH("CNewPath::lookup(%s): found in the alternative directory: '%s'", filename.c_str(), rs.c_str());
				return rs;
			}
		}
	}

	// Not found
	SIP_DISPLAY_PATH("CNewPath::lookup(%s): file not found", filename.c_str());
	return "";
}

string CNewPath::standardizePath (const string &path, bool addFinalSlash)
{
	string newPath;

	// check empty path
	if (path.empty()) return "";

	// don't transform the first \\ for windows network path
/*	if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\')
	{
		newPath += "\\\\";
		i = 2;
	}
*/	
	for (uint i = 0; i < path.size(); i++)
	{
		// don't transform the first \\ for windows network path
		if (path[i] == '\\')
			newPath += '/';
		else
			newPath += path[i];
	}

	// add terminal slash
	if (addFinalSlash && newPath[path.size()-1] != '/')
		newPath += '/';

	return newPath;
}

#ifdef SIP_OS_WINDOWS
#	define dirent	WIN32_FIND_DATA
#	define DIR		void

static string sDir;
static char sDirBackup[512];
static WIN32_FIND_DATA findData;
static HANDLE hFind;

DIR *opendir (const char *path)
{
	sipassert (path != NULL);
	sipassert (path[0] != '\0');

	sipassert (sDirBackup[0] == '\0');
	if (GetCurrentDirectory (512, sDirBackup) == 0)
	{
		// failed
		return NULL;
	}

	if (!CNewFile::isDirectory(path))
	{
		// failed
		return NULL;
	}
	
	sDir = path;

	hFind = NULL;
	
	return (void *)1;
}

int closedir (DIR *dir)
{
	sipassert (sDirBackup[0] != '\0');
	sDirBackup[0] = '\0';
	return 0;
}

dirent *readdir (DIR *dir)
{
	// set the current path
	sipassert (!sDir.empty());
	if (SetCurrentDirectory (sDir.c_str()) == 0)
	{
		// failed
		return NULL;
	}

	if (hFind == NULL)
	{
		hFind = FindFirstFile ("*", &findData);
	}
	else
	{
		if (!FindNextFile (hFind, &findData))
		{
			sipassert (sDirBackup[0] != '\0');
			SetCurrentDirectory (sDirBackup);
			return NULL;
		}
	}

	// restore the current path
	sipassert (sDirBackup[0] != '\0');
	SetCurrentDirectory (sDirBackup);

	return &findData;
}

#endif // SIP_OS_WINDOWS

bool isdirectory (dirent *de)
{
	sipassert (de != NULL);
#ifdef SIP_OS_WINDOWS
	return (de->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
	return (de->d_type & DT_DIR) != 0;
#endif // SIP_OS_WINDOWS
}

bool isfile (dirent *de)
{
	sipassert (de != NULL);
#ifdef SIP_OS_WINDOWS
	return (de->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
	return (de->d_type & DT_DIR) == 0;
#endif // SIP_OS_WINDOWS
}

string getname (dirent *de)
{
	sipassert (de != NULL);
#ifdef SIP_OS_WINDOWS
	return de->cFileName;
#else
	return de->d_name;
#endif // SIP_OS_WINDOWS
}

void CNewPath::getPathContent (const string &path, bool recurse, bool wantDir, bool wantFile, vector<string> &result)
{
	DIR *dir = opendir (path.c_str());
	if (dir == NULL)
	{
		SIP_DISPLAY_PATH("CNewPath::getPathContent(%s, %d, %d, %d): could not open the directory", path.c_str(), recurse, wantDir, wantFile);
		return;
	}

	// contains path that we have to recurs into
	vector<string> recursPath;

	while (true)
	{
		dirent *de = readdir(dir);
		if (de == NULL)
		{
			SIP_DISPLAY_PATH("CNewPath::getPathContent(%s, %d, %d, %d): end of directory", path.c_str(), recurse, wantDir, wantFile);
			break;
		}

		// skip . and ..
		if (getname (de) == "." || getname (de) == "..")
			continue;

		if (isdirectory(de))
		{
			string stdName = standardizePath(path + getname(de));
			if (recurse)
			{
				SIP_DISPLAY_PATH("CNewPath::getPathContent(%s, %d, %d, %d): need to recurse into '%s'", path.c_str(), recurse, wantDir, wantFile, stdName.c_str());
				recursPath.push_back (stdName);
			}

			if (wantDir)
			{
				SIP_DISPLAY_PATH("CNewPath::getPathContent(%s, %d, %d, %d): adding path '%s'", path.c_str(), recurse, wantDir, wantFile, stdName.c_str());
				result.push_back (stdName);
			}
		}
		if (wantFile && isfile(de))
		{
			string stdName = path + getname(de);
			SIP_DISPLAY_PATH("CNewPath::getPathContent(%s, %d, %d, %d): adding file '%s'", path.c_str(), recurse, wantDir, wantFile, stdName.c_str());
			result.push_back (stdName);
		}
	}

	closedir (dir);

	// let s recurse
	for (uint i = 0; i < recursPath.size (); i++)
		getPathContent (recursPath[i], recurse, wantDir, wantFile, result);
}

void CNewPath::addSearchPath (const string &path, bool recurse, bool alternative)
{
	CNewPath *inst = CNewPath::getInstance();
	string newPath = standardizePath(path);

	// check empty directory
	if (newPath.empty())
	{
		sipwarning ("CNewPath::addSearchPath(%s, %d, %d): can't add empty directory, skip it", path.c_str(), recurse, alternative);
		return;
	}

	// check if it s a directory
	if (!CNewFile::isExists (newPath))
	{
		sipwarning ("CNewPath::addSearchPath(%s, %d, %d): '%s' is not found, skip it", path.c_str(), recurse, alternative, newPath.c_str());
		return;
	}

	// check if it s a directory
	if (!CNewFile::isDirectory (newPath))
	{
		sipwarning ("CNewPath::addSearchPath(%s, %d, %d): '%s' is not a directory, skip it", path.c_str(), recurse, alternative, newPath.c_str());
		return;
	}

	SIP_DISPLAY_PATH("CNewPath::addSearchPath(%s, %d, %d): try to add '%s'", path.c_str(), recurse, alternative, newPath.c_str());

	if (alternative)
	{
		vector<string> pathsToProcess;

		// add the current path
		pathsToProcess.push_back (newPath);

		if (recurse)
		{
			// find all path and subpath
			getPathContent (newPath, recurse, true, false, pathsToProcess);
		}

		for (uint p = 0; p < pathsToProcess.size(); p++)
		{
			// check if the path not already in the vector
			uint i;
			for (i = 0; i < inst->_AlternativePaths.size(); i++)
			{
				if (inst->_AlternativePaths[i] == pathsToProcess[p])
					break;
			}
			if (i == inst->_AlternativePaths.size())
			{
				// add them in the alternative directory
				inst->_AlternativePaths.push_back (pathsToProcess[p]);
				SIP_DISPLAY_PATH("CNewPath::addSearchPath(%s, %d, %d): path '%s' added", newPath.c_str(), recurse, alternative, pathsToProcess[p].c_str());
			}
			else
			{
				sipwarning ("CNewPath::addSearchPath(%s, %d, %d): path '%s' already added", newPath.c_str(), recurse, alternative, pathsToProcess[p].c_str());
			}
		}
	}
	else
	{
		vector<string> filesToProcess;
		// find all files in the path and subpaths
		getPathContent (newPath, recurse, false, true, filesToProcess);

		// add them in the map
		for (uint f = 0; f < filesToProcess.size(); f++)
		{
			string filename = CNewFile::getFilename (filesToProcess[f]);
			string filepath = CNewFile::getPath (filesToProcess[f]);
//			insertFileInMap (filename, filepath, false, CNewFile::getExtension(filename));
			addSearchFile (filesToProcess[f]);
		}
	}
}

void CNewPath::addSearchFile (const string &file, bool remap, const string &virtual_ext)
{
	CNewPath *inst = CNewPath::getInstance();
	string newFile = standardizePath(file, false);

	// check empty file
	if (newFile.empty())
	{
		sipwarning ("CNewPath::addSearchFile(%s, %d, %s): can't add empty file, skip it", file.c_str(), remap, virtual_ext.c_str());
		return;
	}

	// check if the file exists
	if (!CNewFile::isExists (newFile))
	{
		sipwarning ("CNewPath::addSearchFile(%s, %d, %s): '%s' is not found, skip it", file.c_str(), remap, virtual_ext.c_str(), newFile.c_str());
		return;
	}

	// check if it s a file
	if (CNewFile::isDirectory (newFile))
	{
		sipwarning ("CNewPath::addSearchFile(%s, %d, %s): '%s' is not a file, skip it", file.c_str(), remap, virtual_ext.c_str(), newFile.c_str());
		return;
	}

	string filenamewoext = CNewFile::getFilenameWithoutExtension (newFile);
	string filename, ext;
	
	if (virtual_ext.empty())
	{
		filename = CNewFile::getFilename (newFile);
		ext = CNewFile::getExtension (filename);
	}
	else
	{
		filename = filenamewoext + "." + virtual_ext;
		ext = virtual_ext;
	}

	map<string, CNewFileEntry>::iterator it = inst->_Files.find (filename);
	if (it == inst->_Files.end ())
	{
		// ok, the room is empty, let s add it
		insertFileInMap (filename, newFile, remap, ext);
	}
	else
	{
		if (remap)
			sipwarning ("CNewPath::addSearchPath(%s, %d, %s): remapped file '%s' already inserted in the map directory", file.c_str(), remap, virtual_ext.c_str(), filename.c_str());
		else
			sipwarning ("CNewPath::addSearchPath(%s, %d, %s): file '%s' already inserted in the map directory", file.c_str(), remap, virtual_ext.c_str(), filename.c_str());
	}

	if (!remap && !ext.empty())
	{
		// now, we have to see extension and insert in the map the remapped files
		for (uint i = 0; i < inst->_Extensions.size (); i++)
		{
			if (inst->_Extensions[i].first == ext)
			{
				// need to remap
				addSearchFile (newFile, true, inst->_Extensions[i].second);
			}
		}
	}
}

void CNewPath::addSearchListFile (const string &filename, bool recurse, bool alternative)
{
	CNewPath *inst = CNewPath::getInstance();

	// check empty file
	if (filename.empty())
	{
		sipwarning ("CNewPath::addSearchListFile(%s, %d, %d): can't add empty file, skip it", filename.c_str(), recurse, alternative);
		return;
	}

	// check if the file exists
	if (!CNewFile::isExists (filename))
	{
		sipwarning ("CNewPath::addSearchListFile(%s, %d, %d): '%s' is not found, skip it", filename.c_str(), recurse, alternative, filename.c_str());
		return;
	}

	// check if it s a file
	if (CNewFile::isDirectory (filename))
	{
		sipwarning ("CNewPath::addSearchListFile(%s, %d, %d): '%s' is not a file, skip it", filename.c_str(), recurse, alternative, filename.c_str());
		return;
	}

	// TODO lire le fichier et ajouter les fichiers qui sont dedans

}


void CNewPath::addSearchBigFile (const string &filename, bool recurse, bool alternative)
{
	// TODO & CHECK
	sipwarning ("CNewPath::addSearchBigFile(): not impremented");
}

void CNewPath::insertFileInMap (const string &filename, const string &filepath, bool remap, const string &extension)
{
	CNewPath *inst = CNewPath::getInstance();

	// find if the file already exist
	map<string, CNewFileEntry>::iterator it = inst->_Files.find (filename);
	if (it != inst->_Files.end ())
	{
		sipwarning ("CNewPath::insertFileInMap(%s, %s, %d, %s): already inserted from '%s', skip it", filename.c_str(), filepath.c_str(), remap, extension.c_str(), (*it).second.Path.c_str());
	}
	else
	{
		inst->_Files.insert (make_pair (filename, CNewFileEntry (filepath, remap, extension)));
		SIP_DISPLAY_PATH("CNewPath::insertFileInMap(%s, %s, %d, %s): added", filename.c_str(), filepath.c_str(), remap, extension.c_str());
	}
}

void CNewPath::display ()
{
	CNewPath *inst = CNewPath::getInstance ();
	sipinfo ("Contents of the map:");
	sipinfo ("%-25s %-5s %-5s %s", "filename", "ext", "remap", "full path");
	sipinfo ("----------------------------------------------------");
	for (map<string, CNewFileEntry>::iterator it = inst->_Files.begin(); it != inst->_Files.end (); it++)
	{
		sipinfo ("%-25s %-5s %-5d %s", (*it).first.c_str(), (*it).second.Extension.c_str(), (*it).second.Remapped, (*it).second.Path.c_str());
	}
	sipinfo ("");
	sipinfo ("Contents of the alternative directory:");
	for (uint i = 0; i < inst->_AlternativePaths.size(); i++)
	{
		sipinfo ("'%s'", inst->_AlternativePaths[i].c_str ());
	}
	sipinfo ("");
	sipinfo ("Contents of the remapped entension table:");
	for (uint j = 0; j < inst->_Extensions.size(); j++)
	{
		sipinfo ("'%s' -> '%s'", inst->_Extensions[j].first.c_str (), inst->_Extensions[j].second.c_str ());
	}
	sipinfo ("End of display");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

#if 0
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
string CPath::lookup (const string &filename, bool throwException)
{
	if(!filename.empty())
	{
		if ( CNewFile::fileExists(filename) )
		{
			SIP_DISPLAY_PATH(filename);
			return filename;
		}
		CStringVector::iterator isv;
		string s;
		for ( isv=CPath::_SearchPaths.begin(); isv!=CPath::_SearchPaths.end(); ++isv )
		{
			s = *isv + filename;
			if ( CNewFile::fileExists(s) )
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

#endif

//********************************* CNewFile

int CNewFile::getLastSeparator (const string &filename)
{
	int pos = filename.find_last_of ('/');
	if (pos == string::npos)
	{
		pos = filename.find_last_of ('\\');
	}
	return pos;
}

string CNewFile::getFilename (const string &filename)
{
	int pos = CNewFile::getLastSeparator(filename);
	if (pos != string::npos)
		return filename.substr (pos + 1);
	else
		return filename;
}

string CNewFile::getFilenameWithoutExtension (const string &filename)
{
	string filename2 = getFilename (filename);
	int pos = filename2.find_last_of ('.');
	if (pos == string::npos)
		return filename2;
	else
		return filename2.substr (0, pos);
}

string CNewFile::getExtension (const string &filename)
{
	int pos = filename.find_last_of ('.');
	if (pos == string::npos)
		return "";
	else
		return filename.substr (pos + 1);
}

string CNewFile::getPath (const string &filename)
{
	int pos = CNewFile::getLastSeparator(filename);
	if (pos != string::npos)
		return filename.substr (0, pos + 1);
	else
		return filename;
}

bool CNewFile::isDirectory (const string &filename)
{
#ifdef SIP_OS_WINDOWS
	DWORD res = GetFileAttributes(filename.c_str());
	sipassert (res != -1);
	return (res & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else // SIP_OS_WINDOWS
	struct stat buf;
	int result = stat (filename.c_str (), &buf);
	sipassert (result == 0);
	return (buf.st_mode & S_IFDIR) != 0;
#endif // SIP_OS_WINDOWS
}

bool CNewFile::isExists (const string &filename)
{
#ifdef SIP_OS_WINDOWS
	return (GetFileAttributes(filename.c_str()) != -1);
#else // SIP_OS_WINDOWS
	struct stat buf;
	return stat (filename.c_str (), &buf) == 0;
#endif SIP_OS_WINDOWS
}

bool CNewFile::fileExists (const string& filename)
{
	return ! ! fstream( filename.c_str(), ios::in );
}


string CNewFile::findNewFile (const string &filename)
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
		if (!CNewFile::fileExists(npath)) break;
	}
	while (num<999);
	return npath;
}

} // SIPBASE

#endif
