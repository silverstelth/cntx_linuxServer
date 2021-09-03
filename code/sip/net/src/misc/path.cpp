/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include <fstream>

#include "misc/big_file.h"
#include "misc/path.h"
#include "misc/hierarchical_timer.h"
#include "misc/progress_callback.h"
#include "misc/file.h"

#ifdef SIP_OS_WINDOWS
#	include <windows.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#	include <direct.h>
#	include <io.h>
#	include <fcntl.h>
#	include <sys/types.h>
#	include <sys/stat.h>
#else
#   include <sys/types.h>
#   include <sys/stat.h>
#	include <dirent.h>
#   include <unistd.h>
#   include <cerrno>
#endif // SIP_OS_WINDOWS

using namespace std;

namespace SIPBASE {

//
// Macros
//

// Use this define if you want to display info about the CPath.
//#define	SIP_DEBUG_PATH

#ifdef	SIP_DEBUG_PATH
	#define	SIP_DISPLAY_PATH		sipinfo
	#define	SIP_DISPLAY_PATH_WIDE	sipinfow
#else 
	#ifdef __GNUC__
		#define	SIP_DISPLAY_PATH(format, args...)
		#define	SIP_DISPLAY_PATH_WIDE(format, args...)
	#else // __GNUC__
		#define	SIP_DISPLAY_PATH if(false)
		#define	SIP_DISPLAY_PATH_WIDE if(false)
	#endif // __GNUC__
#endif


//
// Variables
//

//CPath *CPath::_Instance = NULL;
SIPBASE_SAFE_SINGLETON_IMPL(CPath);

//
// destructor
//
CPath::~CPath()
{
	if( _AllFileNames )
	{
		delete _AllFileNames;
		_AllFileNames = NULL;
	}
}


//
// Functions
//

void CPath::releaseInstance()
{
	if( _Instance )
	{
		delete _Instance;
		_Instance = NULL;
	}
}

void CPath::getFileList(const std::string &extension, std::vector<std::string> &filenames)
{
	std::vector<std::basic_string<ucchar> > filenames1;
	getFileList (getUcsFromMbs(extension.c_str()), filenames1);
	
	uint i;
	for (i = 0; i < filenames1.size(); i ++)
	{
		filenames.push_back(toString("%S", filenames1[i].c_str()));
	}
}

void CPath::getFileList (const std::basic_string<ucchar> &extension, std::vector<std::basic_string<ucchar> > &filenames)
{
	CPath *inst = getInstance();

	if (!inst->_MemoryCompressed)
	{
		std::map<std::basic_string<ucchar>, CFileEntry>::iterator first(inst->_Files.begin()), last(inst->_Files.end());

		if( !extension.empty() )
		{
			for (; first != last; ++ first)
			{
				basic_string<ucchar> ext = inst->SSMext.get(first->second.idExt);
				if (ext == extension)
				{
					filenames.push_back(first->first);
				}
			}
		}
		// if extension is empty we keep all files
		else
		{
			for (; first != last; ++ first)
			{
				filenames.push_back(first->first);
			}
		}
	}
	else
	{
		// compressed memory version
		std::vector<CPath::CMCFileEntry>::iterator first(inst->_MCFiles.begin()), last(inst->_MCFiles.end());

		if( !extension.empty() )
		{
			for (; first != last; ++ first)
			{
				basic_string<ucchar> ext = inst->SSMext.get(first->idExt);
				if (ext == extension)
				{
					filenames.push_back(first->Name);
				}
			}
		}
		// if extension is empty we keep all files
		else
		{
			for (; first != last; ++ first)
			{
				filenames.push_back(first->Name);
			}
		}
	}
}

#ifndef SIP_DONT_USE_EXTERNAL_CODE
void CPath::getFileListByName(const std::string &extension, const std::string &name, std::vector<std::string> &filenames)
{
	std::vector<std::basic_string<ucchar> > filenames1;
	getFileListByName (getUcsFromMbs(extension.c_str()), getUcsFromMbs(name.c_str()), filenames1);

	uint i;
	for (i = 0; i < filenames1.size(); i ++)
	{
		filenames.push_back(toString("%S", filenames1[i].c_str()));
	}

}

void CPath::getFileListByName (const std::basic_string<ucchar> &extension, const std::basic_string<ucchar> &name, std::vector<std::basic_string<ucchar> > &filenames)
{
	CPath *inst = getInstance();
	if (!inst->_MemoryCompressed)
	{
		std::map<std::basic_string<ucchar>, CFileEntry>::iterator first(inst->_Files.begin()), last(inst->_Files.end());

		if( !name.empty() )
		{
			for (; first != last; ++ first)
			{
				basic_string<ucchar> ext = inst->SSMext.get(first->second.idExt);
				if (first->first.find(name) != string::npos && (ext == extension || extension.empty()))
				{
					filenames.push_back(first->first);
				}
			}
		}
		// if extension is empty we keep all files
		else
		{
			for (; first != last; ++ first)
			{
				filenames.push_back(first->first);
			}
		}
	}
	else
	{
		// compressed memory version
		std::vector<CPath::CMCFileEntry>::iterator first(inst->_MCFiles.begin()), last(inst->_MCFiles.end());

		if( !name.empty() )
		{
			for (; first != last; ++ first)
			{
				basic_string<ucchar> ext = inst->SSMext.get(first->idExt);
				if (wcsstr(first->Name, name.c_str()) != NULL && (ext == extension || extension.empty()))
				{
					filenames.push_back(first->Name);
				}
			}
		}
		// if extension is empty we keep all files
		else
		{
			for (; first != last; ++ first)
		{
				filenames.push_back(first->Name);
			}
		}
	}
}

#endif // SIP_DONT_USE_EXTERNAL_CODE


//CPath *CPath::getInstance ()
//{
//	if (_Instance == NULL)
//	{
//#undef new
//		_Instance = new CPath;
//#define new SIP_NEW
//	}
//	return _Instance;
//}

void CPath::clearMap ()
{
	CPath *inst = CPath::getInstance();
	sipassert(!inst->_MemoryCompressed);
	inst->_Files.clear ();
	CBigFile::getInstance().removeAll ();
	SIP_DISPLAY_PATH("PATH: CPath::clearMap(): map directory cleared");
}

CPath::CMCFileEntry *CPath::MCfind (const std::string &filename)
{
	return MCfind (getUcsFromMbs(filename.c_str()));
}

CPath::CMCFileEntry *CPath::MCfind (const std::basic_string<ucchar> &filename)
{
	CPath *inst = CPath::getInstance();
	sipassert (inst->_MemoryCompressed);
	vector<CMCFileEntry>::iterator it;
	
	CMCFileEntry temp_cmc_file;
	temp_cmc_file.Name = (ucchar *) filename.c_str();
	it = lower_bound(inst->_MCFiles.begin(), inst->_MCFiles.end(), temp_cmc_file, CMCFileComp());

	if (it != inst->_MCFiles.end())
	{
		CMCFileComp FileComp;
		if (FileComp.specialCompare(it->Name, filename.c_str()) == 0)
			return &(*it);
	}
	return NULL;
}

sint CPath::findExtension (const string &ext1, const string &ext2)
{
	return findExtension (getUcsFromMbs(ext1.c_str()), getUcsFromMbs(ext2.c_str()));
}

sint CPath::findExtension (const basic_string<ucchar> &ext1, const basic_string<ucchar> &ext2)
{
	CPath *inst = CPath::getInstance();
	for (uint i = 0; i < inst->_Extensions.size (); i++)
	{
		if (inst->_Extensions[i].first == ext1 && inst->_Extensions[i].second == ext2)
		{
			return i;
		}
	}
	return -1;
}

void CPath::remapExtension (const string &ext1, const string &ext2, bool substitute)
{
	remapExtension (getUcsFromMbs(ext1.c_str()), getUcsFromMbs(ext2.c_str()), substitute);
}

void CPath::remapExtension (const basic_string<ucchar> &ext1, const basic_string<ucchar> &ext2, bool substitute)
{
	SIP_ALLOC_CONTEXT (MiPath);
	CPath *inst = CPath::getInstance();
	sipassert(!inst->_MemoryCompressed);

	basic_string<ucchar> ext1lwr = toLower(ext1);
	basic_string<ucchar> ext2lwr = toLower(ext2);

	if (ext1lwr.empty() || ext2lwr.empty())
	{
		sipwarning (L"PATH: CPath::remapExtension(%s, %s, %d): can't remap empty extension", ext1lwr.c_str(), ext2lwr.c_str(), substitute);
	}

	if (ext1lwr == L"bnp" || ext2lwr == L"bnp")
	{
		sipwarning (L"PATH: CPath::remapExtension(%s, %s, %d): you can't remap a big file", ext1lwr.c_str(), ext2lwr.c_str(), substitute);
	}

	if (!substitute)
	{
		// remove the mapping from the mapping list
		sint n = inst->findExtension (ext1lwr, ext2lwr);
		sipassert (n != -1);
		inst->_Extensions.erase (inst->_Extensions.begin() + n);

		// remove mapping in the map
		map<basic_string<ucchar>, CFileEntry>::iterator it = inst->_Files.begin();
		map<basic_string<ucchar>, CFileEntry>::iterator nit = it;
		while (it != inst->_Files.end ())
		{
			nit++;
			basic_string<ucchar> ext = inst->SSMext.get((*it).second.idExt);
			if ((*it).second.Remapped && ext == ext2lwr)
			{
				inst->_Files.erase (it);
			}
			it = nit;
		}
		SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::remapExtension(%s, %s, %d): extension removed", ext1lwr.c_str(), ext2lwr.c_str(), substitute);
	}
	else
	{
		sint n = inst->findExtension (ext1lwr, ext2lwr);
		if (n != -1)
		{
			sipwarning (L"PATH: CPath::remapExtension(%s, %s, %d): remapping already set", ext1lwr.c_str(), ext2lwr.c_str(), substitute);
			return;
		}

		// adding mapping into the mapping list
		inst->_Extensions.push_back (make_pair (ext1lwr, ext2lwr));

		// adding mapping into the map
		vector<basic_string<ucchar> > newFiles;
		map<basic_string<ucchar>, CFileEntry>::iterator it = inst->_Files.begin();
		while (it != inst->_Files.end ())
		{
			basic_string<ucchar> ext = inst->SSMext.get((*it).second.idExt);
			if (!(*it).second.Remapped && ext == ext1lwr)
			{
				// find if already exist
				uint32 pos = (*it).first.find_last_of (L".");
				if (pos != string::npos)
				{
					basic_string<ucchar> file = (*it).first.substr (0, pos + 1);
					file += ext2lwr;

// TODO perhaps a problem because I insert in the current map that i parcours
					basic_string<ucchar> path = inst->SSMpath.get((*it).second.idPath);
					insertFileInMap (file, path+file, true, ext1lwr);
				}
			}
			it++;
		}
		SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::remapExtension(%s, %s, %d): extension added", ext1lwr.c_str(), ext2lwr.c_str(), substitute);
	}
}

// ***************************************************************************
void CPath::remapFile (const std::string &file1, const std::string &file2)
{
	remapFile (getUcsFromMbs(file1.c_str()), getUcsFromMbs(file2.c_str()));
}

void CPath::remapFile (const std::basic_string<ucchar> &file1, const std::basic_string<ucchar> &file2)
{
	SIP_ALLOC_CONTEXT (MiPath);
	CPath *inst = CPath::getInstance();
	if (file1.empty()) return;
	if (file2.empty()) return;
	inst->_RemappedFiles[toLower(file1)] = toLower(file2);
}

// ***************************************************************************
static void removeAllUnusedChar(string &str)
{
	uint32 i = 0;
	while (!str.empty() && (i != str.size()))
	{
		if ((str[i] == ' ' || str[i] == '\t' || str[i] == '\r' || str[i] == '\n'))
			str.erase(str.begin()+i);
		else
			i++;
	}
}

static void removeAllUnusedCharW(basic_string<ucchar> &str)
{
	uint32 i = 0;
	while (!str.empty() && (i != str.size()))
	{
		if ((str[i] == L' ' || str[i] == L'\t' || str[i] == L'\r' || str[i] == L'\n'))
			str.erase(str.begin() + i);
		else
			i ++;
	}
}

// ***************************************************************************
void CPath::loadRemappedFiles (const std::string &file)
{
	SIP_ALLOC_CONTEXT (MiPath);
	string fullName = lookup(file, false, true, true);
	CIFile f;
	f.setCacheFileOnOpen (true);

	if (!f.open (fullName))
		return;

	char sTmp[514];
	string str;
	
	while (!f.eof())
	{
		f.getline(sTmp, 512);
		str = sTmp;
		if (str.find(','))
		{
			removeAllUnusedChar(str);
			if (!str.empty())
				remapFile( str.substr(0,str.find(',')), str.substr(str.find(',')+1, str.size()) );
		}
	}
}

void CPath::loadRemappedFiles (const std::basic_string<ucchar> &file)
{
	SIP_ALLOC_CONTEXT (MiPath);
	basic_string<ucchar> fullName = lookup (file, false, true, true);
	CIFile f;
	f.setCacheFileOnOpen (true);

	if (!f.open (fullName))
		return;

	ucchar sTmp[514];
	basic_string<ucchar> str;
	
	while (!f.eof())
	{
		f.getline(sTmp, 512);
		str = sTmp;
		if (str.find(L','))
		{
			removeAllUnusedCharW(str);
			if (!str.empty())
				remapFile ( str.substr(0,str.find(L',')), str.substr(str.find(L',') + 1, str.size()) );
		}
	}
}

// ***************************************************************************
string CPath::lookup (const string &filename, bool throwException, bool displayWarning, bool lookupInLocalDirectory)
{
	basic_string<ucchar> str = lookup (getUcsFromMbs(filename.c_str()), throwException, displayWarning, lookupInLocalDirectory);
	return toString("%S", str.c_str());
}

std::basic_string<ucchar> CPath::lookup (const std::basic_string<ucchar> &filename, bool throwException, bool displayWarning, bool lookupInLocalDirectory)
{
	/* ***********************************************
	 *	WARNING: This Class/Method must be thread-safe (ctor/dtor/serial): no static access for instance
	 *	It can be loaded/called through CAsyncFileManager for instance
	 * ***********************************************/
	/*
		NB: CPath acess static instance getInstance() of course, so user must ensure
		that no mutator is called while async loading
	*/

	// If the file already contains a @, it means that a lookup already proceed and returning a big file, do nothing
	if (filename.find (L"@") != string::npos)
	{
		SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::lookup(%s):	already found", filename.c_str());
		return filename;
	}

	// Try to find in the map directories
	CPath *inst = CPath::getInstance();
	basic_string<ucchar> str = toLower(filename);

	// Remove end spaces
	while ((!str.empty()) && (str[str.size()-1] == L' '))
	{
		str.resize (str.size()-1);
	}

	map<basic_string<ucchar>, basic_string<ucchar> >::iterator itss = inst->_RemappedFiles.find(str);
	if (itss != inst->_RemappedFiles.end())
		str = itss->second;

	if (inst->_MemoryCompressed)
	{
		CMCFileEntry *pMCFE = MCfind(str);
		// If found in the map, returns it
		if (pMCFE != NULL)
		{
			basic_string<ucchar> fname, path = inst->SSMpath.get(pMCFE->idPath);
			if (pMCFE->Remapped)
				fname = CFileW::getFilenameWithoutExtension(pMCFE->Name) + L"." + inst->SSMext.get(pMCFE->idExt);
			else
				fname = pMCFE->Name;

			SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::lookup(%s): found in the map directory: '%s'", fname.c_str(), path.c_str());
			return path + fname;
		}
	}
	else // NOT memory compressed
	{
		map<basic_string<ucchar>, CFileEntry>::iterator it = inst->_Files.find (str);
		// If found in the map, returns it
		if (it != inst->_Files.end())
		{
			basic_string<ucchar> fname, path = inst->SSMpath.get((*it).second.idPath);
			if (it->second.Remapped)
				fname = CFileW::getFilenameWithoutExtension((*it).second.Name) + L"." + inst->SSMext.get((*it).second.idExt);
			else
				fname = (*it).second.Name;

			SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::lookup(%s): found in the map directory: '%s'", fname.c_str(), path.c_str());
			return path + fname;
		}
	}
	

	// Try to find in the alternative directories
	for (uint i = 0; i < inst->_AlternativePaths.size(); i++)
	{
		basic_string<ucchar> s = inst->_AlternativePaths[i] + filename;
		if ( CFileW::fileExists(s) )
		{
			SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::lookup(%s): found in the alternative directory: '%s'", filename.c_str(), s.c_str());
			return s;
		}		
		
		// try with the remapping
		for (uint j = 0; j < inst->_Extensions.size(); j++)
		{
			if (toLower(CFileW::getExtension (filename)) == inst->_Extensions[j].second)
			{
				basic_string<ucchar> rs = inst->_AlternativePaths[i] + CFileW::getFilenameWithoutExtension (filename) + L"." + inst->_Extensions[j].first;
				if ( CFileW::fileExists(rs) )
				{
					SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::lookup(%s): found in the alternative directory: '%s'", filename.c_str(), rs.c_str());
					return rs;
				}
			}
		}
	}

	// Try to find in the current directory
	if ( lookupInLocalDirectory && CFileW::fileExists(filename) )
	{
		SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::lookup(%s): found in the current directory: '%s'", filename.c_str(), filename.c_str());
		return filename;
	}

	// Not found
	if (displayWarning)
	{
		sipwarning (L"PATH: CPath::lookup(%s): file not found", filename.c_str());
	}

	if (throwException)
		throw EPathNotFound (filename);

	return L"";
}

bool CPath::exists (const std::string &filename)
{
	return exists (getUcsFromMbs(filename.c_str()));
}

bool CPath::exists (const std::basic_string<ucchar> &filename)
{
	// Try to find in the map directories
	CPath *inst = CPath::getInstance();
	basic_string<ucchar> str = toLower(filename);

	// Remove end spaces
	while ((!str.empty()) && (str[str.size()-1] == L' '))
	{
		str.resize (str.size()-1);
	}

	if (inst->_MemoryCompressed)
	{
		CMCFileEntry *pMCFE = MCfind(str);
		// If found in the vector, returns it
		if (pMCFE != NULL)
			return true;
	}
	else
	{
		map<basic_string<ucchar>, CFileEntry>::iterator it = inst->_Files.find (str);
		// If found in the map, returns it
		if (it != inst->_Files.end())
			return true;
	}

	return false;
}

string CPath::standardizePath (const string &path, bool addFinalSlash)
{
/*
	string newPath;
	// check empty path
	if (path.empty()) return "";

	// don't transform the first \\ for windows network path
//	if (path.size() >= 2 && path[0] == '\\' && path[1] == '\\')
//	{
//		newPath += "\\\\";
//		i = 2;
//	}
	
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
*/

	
	// check empty path
	if (path.empty())
		return "";

	string newPath(path);
	
	for (uint i = 0; i < path.size(); i++)
	{
		// don't transform the first \\ for windows network path
		if (path[i] == '\\')
			newPath[i] = '/';
	}

	// add terminal slash
	if (addFinalSlash && newPath[path.size()-1] != '/')
		newPath += '/';

	return newPath;
}

basic_string<ucchar> CPath::standardizePath (const basic_string<ucchar> &path, bool addFinalSlash)
{
	// check empty path
	if (path.empty())
		return L"";

	basic_string<ucchar> newPath(path);
	
	for (uint i = 0; i < path.size(); i++)
	{
		// don't transform the first \\ for windows network path
		if (path[i] == L'\\')
			newPath[i] = L'/';
	}

	// add terminal slash
	if (addFinalSlash && newPath[path.size()-1] != L'/')
		newPath += L'/';

	return newPath;
}

// remplace / wiht \ and put all in lower case
std::string	CPath::standardizeDosPath (const std::string &path)
{
	string newPath;

	for (uint i = 0; i < path.size(); i++)
	{
		if (path[i] == '/')
			newPath += '\\';
		// Yoyo: supress toLower. Not usefull!?!
		/*else if (isupper(path[i]))
			newPath += tolower(path[i]);*/
		else
			newPath += path[i];
	}

	if (CFileA::isExists(path) && CFileA::isDirectory(path) && newPath[newPath.size()-1] != '\\')
		newPath += '\\';

	return newPath;
}

std::basic_string<ucchar> CPath::standardizeDosPath (const std::basic_string<ucchar> &path)
{
	basic_string<ucchar> newPath;

	for (uint i = 0; i < path.size(); i++)
	{
		if (path[i] == L'/')
			newPath += L'\\';
		// Yoyo: supress toLower. Not usefull!?!
		/*else if (isupper(path[i]))
			newPath += tolower(path[i]);*/
		else
			newPath += path[i];
	}

	if (CFileW::isExists(path) && CFileW::isDirectory(path) && newPath[newPath.size()-1] != L'\\')
		newPath += L'\\';

	return newPath;
}

std::string CPath::getCurrentPathA ()
{
	char buffer [10000];

#ifdef SIP_OS_WINDOWS
	return standardizePath(_getcwd(buffer, 10000), false);
#else
	return standardizePath(getcwd(buffer, 10000), false);
#endif
}

std::basic_string<ucchar> CPath::getCurrentPathW ()
{
#ifdef SIP_OS_WINDOWS
	ucchar buffer [10000];
	return standardizePath (_wgetcwd(buffer, 10000), false);
#else
	char chBuf[_MAX_PATH];
	char* chPath = getcwd(chBuf, _MAX_PATH);
	ucstring ucstr;
	ucstr.fromUtf8(string(chPath));
	return standardizePath (ucstr, false);

#endif
}

bool CPath::setCurrentPath (const char *newDir)
{
#ifdef SIP_OS_WINDOWS
	return _chdir(newDir) == 0;
#else
	// todo : check this compiles under linux. Thanks (Hulud)
	return chdir(newDir) == 0;
#endif
}

bool CPath::setCurrentPath (const ucchar *newDir)
{
#ifdef SIP_OS_WINDOWS
	return _wchdir(newDir) == 0;
#else
	// todo : check this compiles under linux. Thanks (Hulud)
	GET_WFILENAME(csNewDir, newDir);
	return chdir(csNewDir) == 0;
#endif
}

std::string CPath::getFullPath (const std::string &path, bool addFinalSlash)
{
	string currentPath = standardizePath (getCurrentPathA ());
	string sPath = standardizePath (path, addFinalSlash);

	// current path
	if (path.empty() || sPath == "." || sPath == "./")
	{
		return currentPath;
	}

	// windows full path
	if (path.size() >= 2 && path[1] == ':')
	{
		return sPath;
	}

	if (path.size() >= 2 && (path[0] == '/' || path[0] == '\\') && (path[1] == '/' || path[1] == '\\'))
	{
		return sPath;
	}


	// from root
	if (path [0] == '/' || path[0] == '\\')
	{
		if (currentPath.size() > 2 && currentPath[1] == ':')
		{
			return currentPath.substr(0,3) + sPath.substr(1);
		}
		else
		{
			return sPath;
		}
	}

	// default case
	return currentPath + sPath;
}

std::basic_string<ucchar> CPath::getFullPath (const std::basic_string<ucchar> &path, bool addFinalSlash)
{
	basic_string<ucchar> currentPath = standardizePath (getCurrentPathW());
	basic_string<ucchar> sPath = standardizePath (path, addFinalSlash);

	// current path
	if (path.empty() || sPath == L"." || sPath == L"./")
	{
		return currentPath;
	}

	// windows full path
	if (path.size() >= 2 && path[1] == L':')
	{
		return sPath;
	}

	if (path.size() >= 2 && (path[0] == L'/' || path[0] == L'\\') && (path[1] == L'/' || path[1] == L'\\'))
	{
		return sPath;
	}


	// from root
	if (path [0] == L'/' || path[0] == L'\\')
	{
		if (currentPath.size() > 2 && currentPath[1] == L':')
		{
			return currentPath.substr(0,3) + sPath.substr(1);
		}
		else
		{
			return sPath;
		}
	}

	// default case
	return currentPath + sPath;
}

bool CPath::isFullPath (const std::string &path)
{
	if (path.size() >= 2)
	{
		if (path[1] == ':')
			return true;
		if ((path[0] == '/' || path[0] == '\\') && (path[1] == '/' || path[1] == '\\'))
			return true;
	}

	return false;
}

bool CPath::isFullPath (const std::basic_string<ucchar> &path)
{
	if (path.size() >= 2)
	{
		if (path[1] == L':')
			return true;
		if ((path[0] == L'/' || path[0] == L'\\') && (path[1] == L'/' || path[1] == L'\\'))
			return true;
	}

	return false;
}

const char *CPath::getExePathA ()
{
	static char path[500];
	path[0] = '\0';

#ifdef SIP_OS_WINDOWS
	HMODULE handle = GetModuleHandleA(NULL);
	if (handle != NULL)
	{
		if (GetModuleFileNameA(handle, path, 500) != 0)
		{
			std::string strPath = CFileA::getPath(path);
			strcpy(path, strPath.c_str());
		}
	}
#else
	char arg[20];
	char exepath[PATH_MAX + 1] = {0};
	sprintf(arg, "/proc/%d/exe", getpid());
	readlink(arg, path, 1024);
#endif
	return path;
}

const ucchar *CPath::getExePathW ()
{
	static ucchar path[500];
	path[0] = L'\0';

#ifdef SIP_OS_WINDOWS
	HMODULE handle = GetModuleHandleW(NULL);
	if (handle != NULL)
	{
		if (GetModuleFileNameW(handle, path, 500) != 0)
		{
			std::basic_string<ucchar> strPath = CFileW::getPath(path);
			wcscpy(path, strPath.c_str());
		}
	}
#else
	char arg[20];
	char exepath[PATH_MAX + 1] = {0};
	sprintf(arg, "/proc/%d/exe", getpid());
	readlink(arg, exepath, 1024);
	ucstring ucpath = ucstring::makeFromUtf8(exepath);
	std::basic_string<ucchar> strPath = CFileW::getPath(ucpath);
	wcscpy(path, strPath.c_str());
#endif
	return path;
}

#ifdef SIP_OS_WINDOWS
#	define dirent	WIN32_FIND_DATAW
#	define DIR		void

static basic_string<ucchar> sDir;
static WIN32_FIND_DATAW findData;
static HANDLE hFind;

DIR *opendir (const char *path)
{
	sipassert (path != NULL);
	sipassert (path[0] != '\0');

	if (!CFileA::isDirectory(path))
		return NULL;

	sDir = getUcsFromMbs(path);

	hFind = NULL;
	
	return (void *)1;
}

DIR *opendirW(const ucchar *path)
{
	sipassert(path != NULL);
	sipassert(path[0] != L'\0');

	if (!CFileW::isDirectory(path))
		return NULL;
	
	sDir = path;

	hFind = NULL;
	
	return (void *)1;
}

int closedir (DIR *dir)
{
	FindClose(hFind);
	return 0;
}

dirent *readdir (DIR *dir)
{
	// FIX : call to SetCurrentDirectory() and SetCurrentDirectory() removed to improve speed
	sipassert (!sDir.empty());

	// first visit in this directory : FindFirstFile()
	if (hFind == NULL)
	{
		basic_string<ucchar> fullPath = CPath::standardizePath (sDir) + L"*";
		hFind = FindFirstFileW (fullPath.c_str(), &findData);
	}
	// directory already visited : FindNextFile()
	else
	{
		if (!FindNextFileW (hFind, &findData))
			return NULL;
	}

	return &findData;
}

#else	// added by ysg
DIR *opendirW(const ucchar *path)
{
	return opendir(toString("%S", path).c_str());
/*	sipassert(path != NULL);
	sipassert(path[0] != L'\0');

	if (!CFileW::isDirectory(path))
		return NULL;
	
	sDir = path;

	hFind = NULL;
	
	return (void *)1;*/
}



#endif // SIP_OS_WINDOWS

#ifndef SIP_OS_WINDOWS
basic_string<ucchar> BasePathgetPathContent;
#endif

bool isdirectory (dirent *de)
{
	sipassert (de != NULL);
#ifdef SIP_OS_WINDOWS
	return ((de->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) && ((de->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0);
#else
	//sipinfo ("isdirectory filename %s -> 0x%08x", de->d_name, de->d_type);
	// we can't use "de->d_type & DT_DIR" because it s always NULL on libc2.1
	//return (de->d_type & DT_DIR) != 0;

	return CFileW::isDirectory (BasePathgetPathContent + de->d_name);

#endif // SIP_OS_WINDOWS
}

bool isfile (dirent *de)
{
	sipassert (de != NULL);
#ifdef SIP_OS_WINDOWS
	return ((de->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) && ((de->dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) == 0);
#else
	// we can't use "de->d_type & DT_DIR" because it s always NULL on libc2.1
	//return (de->d_type & DT_DIR) == 0;
	
	//return !CFileA::isDirectory (BasePathgetPathContent + de->d_name);
	// changed by ysg 2009-05-15
	return !CFileW::isDirectory (BasePathgetPathContent + de->d_name);

#endif // SIP_OS_WINDOWS
}

string getname (dirent *de)
{
	sipassert (de != NULL);
#ifdef SIP_OS_WINDOWS
	return toString("%S", de->cFileName);
#else
	return toString("%S", de->d_name);
#endif // SIP_OS_WINDOWS
}

basic_string<ucchar> getnameW(dirent *de)
{
	sipassert (de != NULL);
#ifdef SIP_OS_WINDOWS
	return de->cFileName;
#else
	// return de->d_name;
	// changed by ysg 2009-05-15
	return getUcsFromMbs(de->d_name);
#endif // SIP_OS_WINDOWS
}

void CPath::getPathContent (const string &path, bool recurse, bool wantDir, bool wantFile, vector<string> &result, class IProgressCallback *progressCallBack, bool showEverything)
{
	vector<basic_string<ucchar> > result1;
	getPathContent (getUcsFromMbs(path.c_str()), recurse, wantDir, wantFile, result1, progressCallBack, showEverything);

	uint i;
	for (i = 0; i < result1.size(); i ++)
	{
		result.push_back(toString("%S", result1[i].c_str()));
	}
}

void CPath::getPathContent (const std::basic_string<ucchar> &path, bool recurse, bool wantDir, bool wantFile, std::vector<std::basic_string<ucchar> > &result, class IProgressCallback *progressCallBack, bool showEverything)
{
	if(	path.empty() )
	{
		SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::getPathContent(): Empty input Path");
		return;
	}

#ifndef SIP_OS_WINDOWS
	BasePathgetPathContent = CPath::standardizePath (path);
#endif

	DIR *dir = opendirW(path.c_str());

	if (dir == NULL)
	{
		SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::getPathContent(%s, %d, %d, %d): could not open the directory", path.c_str(), recurse, wantDir, wantFile);
		return;
	}

	// contains path that we have to recurs into
	vector<basic_string<ucchar> > recursPath;

	while (true)
	{
		dirent *de = readdir(dir);
		if (de == NULL)
		{
			SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::getPathContent(%s, %d, %d, %d): end of directory", path.c_str(), recurse, wantDir, wantFile);
			break;
		}

		basic_string<ucchar> fn = getnameW(de);

		// skip . and ..
		if (fn == L"." || fn == L"..")
			continue;

		if (isdirectory(de))
		{
			// skip CVS directory
			if ((!showEverything) && (fn == L"CVS"))
			{
				SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::getPathContent(%s, %d, %d, %d): skip CVS directory", path.c_str(), recurse, wantDir, wantFile);
				continue;
			}

			basic_string<ucchar> stdName = standardizePath (standardizePath (path) + fn);
			if (recurse)
			{
				SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::getPathContent(%s, %d, %d, %d): need to recurse into '%s'", path.c_str(), recurse, wantDir, wantFile, stdName.c_str());
				recursPath.push_back (stdName);
			}

			if (wantDir)
			{
				SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::getPathContent(%s, %d, %d, %d): adding path '%s'", path.c_str(), recurse, wantDir, wantFile, stdName.c_str());
				result.push_back (stdName);
			}
		}

		if (wantFile && isfile(de))
		{
//			if ( (!showEverything) && (fn.size() >= 4 && fn.substr (fn.size()-4) == L".log"))
//			{
//				SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::getPathContent(%s, %d, %d, %d): skip *.log files (%s)", path.c_str(), recurse, wantDir, wantFile, fn.c_str());
//				continue;
//			}

/*			int lastSep = CFileW::getLastSeparator(path);
			#ifdef SIP_OS_WINDOWS
				ucchar sep = lastSep == std::string::npos ? L'\\'
													    : path[lastSep];
			#else
				ucchar sep = lastSep == std::string::npos ? L'/'
														: path[lastSep];
			#endif
*/			
			basic_string<ucchar> stdName = standardizePath (path) + getnameW(de);
			
				
			SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::getPathContent(%s, %d, %d, %d): adding file '%s'", path.c_str(), recurse, wantDir, wantFile, stdName.c_str());
			result.push_back (stdName);
		}
	}

	closedir (dir);

#ifndef SIP_OS_WINDOWS
	BasePathgetPathContent = L"";
#endif // SIP_OS_WINDOWS

	// let s recurse
	for (uint i = 0; i < recursPath.size (); i++)
	{		
		// Progress bar
		if (progressCallBack)
		{
			progressCallBack->progress ((float)i/(float)recursPath.size ());
			progressCallBack->pushCropedValues ((float)i/(float)recursPath.size (), (float)(i+1)/(float)recursPath.size ());
		}

		getPathContent (recursPath[i], recurse, wantDir, wantFile, result, progressCallBack, showEverything);

		// Progress bar
		if (progressCallBack)
		{
			progressCallBack->popCropedValues ();
		}
	}
}

bool CPath::deletePathAndContents(const std::string &path, sint deleteRoot)
{
	ucstring ucstr = path;
	return deletePathAndContents(ucstr, deleteRoot);
}

bool CPath::deletePathAndContents(const std::basic_string<ucchar> &path, sint deleteRoot)
{
	if (path.empty() || !CFileW::isExists(path))
		return false;

	if (!CFileW::isDirectory(path))
	{
		return CFileW::deleteFile(path);
	}

	ucstring basePath = CPath::standardizePath (path);
#ifdef SIP_OS_WINDOWS
	ucstring fullPath = basePath + L"*";
	WIN32_FIND_DATAW data;
	HANDLE handle = FindFirstFileW (fullPath.c_str(), &data);

	bool empty = false;

	if (handle != NULL)
	{
		while (true)
		{
			ucstring fn = data.cFileName;
			if ((fn != L".") && (fn != L".."))
			{
				if (CFileW::isDirectory(basePath + fn))
				{
					if (!deletePathAndContents(basePath + fn, 1))
						break;
				}
				else
				{
					if (!CFileW::deleteFile(basePath + fn))
						break;
				}
			}

			if (!FindNextFileW(handle, &data))
			{
				empty = true;
				break;
			}
		}
		FindClose(handle);
	}
	else
		empty = true;

	if ((deleteRoot == 1) && (empty == true))
	{
		CFileW::setRWAccess(basePath);
		if (!RemoveDirectoryW(basePath.c_str()))
			empty = false;
	}

	return empty;
#elif defined SIP_OS_WINDOWS
	// changed by ysg 2009-05-16
	QDir d(toString("%S", path.c_str()));
	for ( int i = 0; i < d.count(); i++ )
	{
		if ((d[i] == ".") || (d[i] == ".."))
			continue;
		ucstring fn = getUcsFromMbs(d[i]);
		if (CFileW::isDirectory(basePath + fn))
		{
			if (!deletePathAndContents(basePath + fn, 1))
				break;
		}
		else
		{
			if (!CFileW::deleteFile(basePath + fn))
				break;
		}
	}
	CFileW::setRWAccess(basePath);
	d.rmdir();

	sipwarning("not tested : CPath::deletePathAndContents");
	return  true;
#endif // SIP_OS_WINDOWS
}

bool CPath::isEmptyPath(const std::string &path)
{
	ucstring ucstr = path;
	return isEmptyPath(ucstr);
}

bool CPath::isEmptyPath(const std::basic_string<ucchar> &path)
{
	bool empty = true;
	
	if(	path.empty() )
		return empty;

	DIR *dir = opendirW(path.c_str());

	if (dir == NULL)
		return empty;

	while (true)
	{
		dirent *de = readdir(dir);
		if (de == NULL)
			break;
		
		basic_string<ucchar> fn = getnameW(de);
		
		if (fn != L"." && fn != L"..")
		{
			empty = false;
			break;
		}
	}
	closedir(dir);
	return empty;
}

void CPath::removeAllAlternativeSearchPath ()
{
	CPath *inst = CPath::getInstance();
	inst->_AlternativePaths.clear ();
	SIP_DISPLAY_PATH("PATH: CPath::RemoveAllAternativeSearchPath(): removed");
}

void CPath::addSearchPath (const string &path, bool recurse, bool alternative, class IProgressCallback *progressCallBack)
{
	addSearchPath (getUcsFromMbs(path.c_str()), recurse, alternative, progressCallBack);
}

void CPath::addSearchPath (const std::basic_string<ucchar> &path, bool recurse, bool alternative, class IProgressCallback *progressCallBack)
{
	SIP_ALLOC_CONTEXT (MiPath);
	//H_AUTO_INST(addSearchPath);

	CPath *inst = CPath::getInstance();
	sipassert (!inst->_MemoryCompressed);

	// check empty directory
	if (path.empty())
	{
		sipwarning (L"PATH: CPath::addSearchPath(%s, %s, %s): can't add empty directory, skip it", 
			path.c_str(), 
			recurse ? L"recursive" : L"not recursive", 
			alternative ? L"alternative" : L"not alternative");
		return;
	}

	// check if it s a directory
	if (!CFileW::isDirectory (path))
	{
		sipinfo (L"PATH: CPath::addSearchPath(%s, %s, %s): '%s' is not a directory, I'll call addSearchFile()", 
			path.c_str(), 
			recurse ? L"recursive" : L"not recursive", 
			alternative ? L"alternative" : L"not alternative", 
			path.c_str());
		addSearchFile (path, false, L"", progressCallBack);
		return;
	}

	basic_string<ucchar> newPath = standardizePath (path);

	// check if it s a directory
	if (!CFileW::isExists (newPath))
	{
		sipwarning (L"PATH: CPath::addSearchPath(%s, %s, %s): '%s' is not found, skip it", 
			path.c_str(), 
			recurse ? L"recursive" : L"not recursive", 
			alternative ? L"alternative" : L"not alternative", 
			newPath.c_str());
		return;
	}

	sipinfo (L"PATH: CPath::addSearchPath(%s, %d, %d): adding the path '%s'", path.c_str(), recurse, alternative, newPath.c_str());

	SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::addSearchPath(%s, %d, %d): try to add '%s'", path.c_str(), recurse, alternative, newPath.c_str());

	if (alternative)
	{
		vector<basic_string<ucchar> > pathsToProcess;

		// add the current path
		pathsToProcess.push_back (newPath);

		if (recurse)
		{
			// find all path and subpath
			getPathContent (newPath, recurse, true, false, pathsToProcess, progressCallBack);
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
				SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::addSearchPath(%s, %s, %s): path '%s' added", 
					newPath.c_str(), 
					recurse ? L"recursive" : L"not recursive", 
					alternative ? L"alternative" : L"not alternative", 
					pathsToProcess[p].c_str());
			}
			else
			{
				sipwarning (L"PATH: CPath::addSearchPath(%s, %s, %s): path '%s' already added", 
					newPath.c_str(), 
					recurse ? L"recursive" : L"not recursive", 
					alternative ? L"alternative" : L"not alternative", 
					pathsToProcess[p].c_str());
			}
		}
	}
	else
	{
		vector<basic_string<ucchar> > filesToProcess;

		// Progree bar
		if (progressCallBack)
		{
			progressCallBack->progress (0);
			progressCallBack->pushCropedValues (0, 0.5f);
		}

		// find all files in the path and subpaths
		getPathContent (newPath, recurse, false, true, filesToProcess, progressCallBack);

		// Progree bar
		if (progressCallBack)
		{
			progressCallBack->popCropedValues ();
			progressCallBack->progress (0.5);
			progressCallBack->pushCropedValues (0.5f, 1);
		}

		// add them in the map
		for (uint f = 0; f < filesToProcess.size(); f++)
		{
			// Progree bar
			if (progressCallBack)
			{
				progressCallBack->progress ((float)f/(float)filesToProcess.size());
				progressCallBack->pushCropedValues ((float)f/(float)filesToProcess.size(), (float)(f+1)/(float)filesToProcess.size());
			}

			basic_string<ucchar> filename = CFileW::getFilename (filesToProcess[f]);
			basic_string<ucchar> filepath = CFileW::getPath (filesToProcess[f]);
//			insertFileInMap (filename, filepath, false, CFileW::getExtension(filename));
			addSearchFile (filesToProcess[f], false, L"", progressCallBack);

			// Progree bar
			if (progressCallBack)
			{
				progressCallBack->popCropedValues ();
			}
		}

		// Progree bar
		if (progressCallBack)
		{
			progressCallBack->popCropedValues ();
		}
	}
}

void CPath::addSearchFile (const string &file, bool remap, const string &virtual_ext, SIPBASE::IProgressCallback *progressCallBack)
{
	addSearchFile (getUcsFromMbs(file.c_str()), remap, getUcsFromMbs(virtual_ext.c_str()), progressCallBack);
}

void CPath::addSearchFile (const std::basic_string<ucchar> &file, bool remap, const std::basic_string<ucchar> &virtual_ext, class SIPBASE::IProgressCallback *progressCallBack)
{
	SIP_ALLOC_CONTEXT (MiPath);
	CPath *inst = CPath::getInstance();
	sipassert(!inst->_MemoryCompressed);

	basic_string<ucchar> newFile = standardizePath (file, false);

	// check empty file
	if (newFile.empty())
	{
		sipwarning (L"PATH: CPath::addSearchFile(%s, %d, '%s'): can't add empty file, skip it", file.c_str(), remap, virtual_ext.c_str());
		return;
	}

	// check if the file exists
	if (!CFileW::isExists (newFile))
	{
		sipwarning (L"PATH: CPath::addSearchFile(%s, %d, '%s'): '%s' is not found, skip it (current dir is '%s'", 
			file.c_str(), 
			remap, 
			virtual_ext.c_str(), 
			newFile.c_str(),
			CPath::getCurrentPathW().c_str());
		return;
	}

	// check if it s a file
	if (CFileW::isDirectory (newFile))
	{
		sipwarning (L"PATH: CPath::addSearchFile(%s, %d, '%s'): '%s' is not a file, skip it", 
			file.c_str(), 
			remap, 
			virtual_ext.c_str(), 
			newFile.c_str());
		return;
	}

	// check if it s a big file
	if (CFileW::getExtension(newFile) == L"bnp")
	{
		SIP_DISPLAY_PATH_WIDE (L"PATH: CPath::addSearchFile(%s, %d, '%s'): '%s' is a big file, add it", file.c_str(), remap, virtual_ext.c_str(), newFile.c_str());
		addSearchBigFile (file, false, false, progressCallBack);
		return;
	}

	basic_string<ucchar> filenamewoext = CFileW::getFilenameWithoutExtension (newFile);
	basic_string<ucchar> filename, ext;

	if (virtual_ext.empty())
	{
		filename = CFileW::getFilename (newFile);
		ext = CFileW::getExtension (filename);
	}
	else
	{
		filename = filenamewoext + L"." + virtual_ext;
		ext = CFileW::getExtension (newFile);
	}

	insertFileInMap (filename, newFile, remap, ext);

	if (!remap && !ext.empty())
	{
		// now, we have to see extension and insert in the map the remapped files
		for (uint i = 0; i < inst->_Extensions.size (); i++)
		{
			if (inst->_Extensions[i].first == toLower(ext))
			{
				// need to remap
				addSearchFile (newFile, true, inst->_Extensions[i].second, progressCallBack);
			}
		}
	}
}

void CPath::addSearchListFile (const string &filename, bool recurse, bool alternative)
{
	addSearchListFile (getUcsFromMbs(filename.c_str()), recurse, alternative);
}

void CPath::addSearchListFile (const basic_string<ucchar> &filename, bool recurse, bool alternative)
{
	SIP_ALLOC_CONTEXT (MiPath);
	// check empty file
	if (filename.empty())
	{
		sipwarning (L"PATH: CPath::addSearchListFile(%s, %d, %d): can't add empty file, skip it", filename.c_str(), recurse, alternative);
		return;
	}

	// check if the file exists
	if (!CFileW::isExists (filename))
	{
		sipwarning (L"PATH: CPath::addSearchListFile(%s, %d, %d): '%s' is not found, skip it", filename.c_str(), recurse, alternative, filename.c_str());
		return;
	}

	// check if it s a file
	if (CFileW::isDirectory (filename))
	{
		sipwarning (L"PATH: CPath::addSearchListFile(%s, %d, %d): '%s' is not a file, skip it", filename.c_str(), recurse, alternative, filename.c_str());
		return;
	}
	// TODO lire le fichier et ajouter les fichiers qui sont dedans
}

// WARNING : recurse is not used
void CPath::addSearchBigFile (const string &sBigFilename, bool recurse, bool alternative, SIPBASE::IProgressCallback *progressCallBack)
{
	addSearchBigFile (getUcsFromMbs(sBigFilename.c_str()), recurse, alternative, progressCallBack);
}

void CPath::addSearchBigFile (const basic_string<ucchar> &sBigFilename, bool recurse, bool alternative, SIPBASE::IProgressCallback *progressCallBack)
{
 //#ifndef SIP_OS_WINDOWS
  //	siperrorw( L"BNP currently not supported on Unix" ); // test of BNP failed on Linux
  //#endif

	// Check if filename is not empty
	if (sBigFilename.empty())
	{
		sipwarning (L"PATH: CPath::addSearchBigFile(%s, %d, %d): can't add empty file, skip it", sBigFilename.c_str(), recurse, alternative);
		return;
	}
	// Check if the file exists
	if (!CFileW::isExists (sBigFilename))
	{
		sipwarning (L"PATH: CPath::addSearchBigFile(%s, %d, %d): '%s' is not found, skip it", sBigFilename.c_str(), recurse, alternative, sBigFilename.c_str());
		return;
	}
	// Check if it s a file
	if (CFileW::isDirectory (sBigFilename))
	{
		sipwarning (L"PATH: CPath::addSearchBigFile(%s, %d, %d): '%s' is not a file, skip it", sBigFilename.c_str(), recurse, alternative, sBigFilename.c_str());
		return;
	}
	// Open and read the big file header
	CPath *inst = CPath::getInstance();
	sipassert(!inst->_MemoryCompressed);

	FILE *Handle = sfWfopen (sBigFilename.c_str(), L"rb");
	if (Handle == NULL)
	{
		sipwarning (L"PATH: CPath::addSearchBigFile(%s, %d, %d): can't open file, skip it", sBigFilename.c_str(), recurse, alternative);
		return;
	}

	// add the link with the CBigFile singleton
	if (CBigFile::getInstance().addW (sBigFilename, BF_ALWAYS_OPENED | BF_CACHE_FILE_ON_OPEN))
	{
		// also add the bigfile name in the map to retreive the full path of a .bnp when we want modification date of the bnp for example
		insertFileInMap (CFileW::getFilename (sBigFilename), sBigFilename, false, CFileW::getExtension(sBigFilename));

		// parse the big file to add file in the map
		uint32 nFileSize=CFileW::getFileSize (Handle);
		//sipfseek64 (Handle, 0, SEEK_END);
		//uint32 nFileSize = ftell (Handle);
		sipfseek64 (Handle, nFileSize-4, SEEK_SET);
		uint32 nOffsetFromBegining;
		fread (&nOffsetFromBegining, sizeof(uint32), 1, Handle);
		sipfseek64 (Handle, nOffsetFromBegining, SEEK_SET);
		uint32 nNbFile;
		fread (&nNbFile, sizeof(uint32), 1, Handle);
		for (uint32 i = 0; i < nNbFile; ++i)
		{
			// Progress bar
			if (progressCallBack)
			{
				progressCallBack->progress ((float)i/(float)nNbFile);
				progressCallBack->pushCropedValues ((float)i/(float)nNbFile, (float)(i+1)/(float)nNbFile);
			}

			ucchar FileName[256];
			uint8 nStringSize;
			fread (&nStringSize, 1, 1, Handle);
			fread (FileName, sizeof(ucchar), nStringSize, Handle);
			FileName[nStringSize] = L'\0';
			uint32 nFileSize2;
			fread (&nFileSize2, sizeof(uint32), 1, Handle);
			uint32 nFilePos;
			fread (&nFilePos, sizeof(uint32), 1, Handle);
			basic_string<ucchar> sTmp = toLower(basic_string<ucchar>(FileName));
			if (sTmp.empty())
			{
				sipwarning (L"PATH: CPath::addSearchBigFile(%s, %d, %d): can't add empty file, skip it", sBigFilename.c_str(), recurse, alternative);
				continue;
			}
			basic_string<ucchar> bigfilenamealone = CFileW::getFilename (sBigFilename);
			basic_string<ucchar> filenamewoext = CFileW::getFilenameWithoutExtension (sTmp);
			basic_string<ucchar> ext = toLower(CFileW::getExtension(sTmp));

			insertFileInMap (sTmp, bigfilenamealone + L"@" + sTmp, false, ext);

			for (uint j = 0; j < inst->_Extensions.size (); j++)
			{
				if (inst->_Extensions[j].first == ext)
				{
					// need to remap
					insertFileInMap (filenamewoext+L"."+inst->_Extensions[j].second, 
									bigfilenamealone + L"@" + sTmp, 
									true, 
									inst->_Extensions[j].first);
				}
			}

			// Progress bar
			if (progressCallBack)
			{
				progressCallBack->popCropedValues ();
			}
		}
	}
	else
	{
		sipwarning (L"PATH: CPath::addSearchBigFile(%s, %d, %d): can't add the big file", sBigFilename.c_str(), recurse, alternative);
	}

	fclose (Handle);
}

void CPath::addIgnoredDoubleFile (const std::string &ignoredFile)
{
	SIP_ALLOC_CONTEXT (MiPath);
	CPath::getInstance ()->IgnoredFiles.push_back(getUcsFromMbs(ignoredFile.c_str()));
}

void CPath::addIgnoredDoubleFile (const std::basic_string<ucchar> &ignoredFile)
{
	SIP_ALLOC_CONTEXT (MiPath);
	CPath::getInstance ()->IgnoredFiles.push_back(ignoredFile);
}

void CPath::insertFileInMap (const string &filename, const string &filepath, bool remap, const string &extension)
{
	insertFileInMap (getUcsFromMbs(filename.c_str()), getUcsFromMbs(filepath.c_str()), remap, getUcsFromMbs(extension.c_str()));
}

void CPath::insertFileInMap (const basic_string<ucchar> &filename, const basic_string<ucchar> &filepath, bool remap, const basic_string<ucchar> &extension)
{
	SIP_ALLOC_CONTEXT (MiPath);
	CPath *inst = CPath::getInstance();
	sipassert(!inst->_MemoryCompressed);
	// find if the file already exist
	map<basic_string<ucchar>, CFileEntry>::iterator it = inst->_Files.find (toLower(filename));
	if (it != inst->_Files.end ())
	{
		basic_string<ucchar> path = inst->SSMpath.get((*it).second.idPath);
		if (path.find(L"@") != string::npos && filepath.find(L"@") == string::npos)
		{
			// if there's a file in a big file and a file in a path, the file in path wins
			// replace with the new one
			sipinfo (L"PATH: CPath::insertFileInMap(%s, %s, %d, %s): already inserted from '%s' but special case so overide it", filename.c_str(), filepath.c_str(), remap, extension.c_str(), path.c_str());
			basic_string<ucchar> sTmp = filepath.substr(0, filepath.rfind(L'/') + 1);
			(*it).second.idPath = inst->SSMpath.add(sTmp);
			(*it).second.Remapped = remap;
			(*it).second.idExt = inst->SSMext.add(extension);
			(*it).second.Name = filename;
		}
		else
		{
			for(uint i = 0; i < inst->IgnoredFiles.size(); i++)
			{
				// if we don't want to display a warning, skip it
				if(filename == inst->IgnoredFiles[i])
					return;
			}
			// if the path is the same, don't warn
			basic_string<ucchar> path2 = inst->SSMpath.get((*it).second.idPath);
			basic_string<ucchar> sPathOnly;
			if (filepath.rfind(L'@') != string::npos)
				sPathOnly = filepath.substr(0, filepath.rfind(L'@') + 1);
			else
				sPathOnly = CFileW::getPath(filepath);

			if (path2 == sPathOnly)
				return;
			
			sipwarning (L"PATH: CPath::insertFileInMap(%s, %s, %d, %s): already inserted from '%s', skip it", 
				filename.c_str(), 
				filepath.c_str(), 
				remap, 
				extension.c_str(), 
				path2.c_str());
		}
	}
	else
	{
		CFileEntry fe;
		fe.idExt = inst->SSMext.add(extension);
		fe.Remapped = remap;
		basic_string<ucchar> sTmp;
		if (filepath.find(L"@") == string::npos)
			sTmp = filepath.substr(0, filepath.rfind(L'/') + 1);
		else
			sTmp = filepath.substr(0, filepath.rfind(L'@') + 1);

		fe.idPath = inst->SSMpath.add(sTmp);
		fe.Name = filename;

		inst->_Files.insert (make_pair(toLower(filename), fe));
		SIP_DISPLAY_PATH_WIDE(L"PATH: CPath::insertFileInMap(%s, %s, %d, %s): added", toLower(filename).c_str(), filepath.c_str(), remap, toLower(extension).c_str());
	}
}

void CPath::display ()
{
#ifdef SIP_USE_UNICODE
	CPath *inst = CPath::getInstance ();
	sipinfo (L"PATH: Contents of the map:");
	sipinfo (L"PATH: %-25s %-5s %-5s %s", L"filename", L"ext", L"remap", L"full path");
	sipinfo (L"PATH: ----------------------------------------------------");
	if (inst->_MemoryCompressed)
	{
		for (uint i = 0; i < inst->_MCFiles.size(); ++i)
		{
			const CMCFileEntry &fe = inst->_MCFiles[i];
			basic_string<ucchar> ext = inst->SSMext.get(fe.idExt);
			basic_string<ucchar> path = inst->SSMpath.get(fe.idPath);
			sipinfo (L"PATH: %-25s %-5s %-5d %s", fe.Name, ext.c_str(), fe.Remapped, path.c_str());
		}
	}
	else
	{
		for (map<basic_string<ucchar>, CFileEntry>::iterator it = inst->_Files.begin(); it != inst->_Files.end (); it++)
		{
			basic_string<ucchar> ext = inst->SSMext.get((*it).second.idExt);
			basic_string<ucchar> path = inst->SSMpath.get((*it).second.idPath);
			sipinfo (L"PATH: %-25s %-5s %-5d %s", (*it).first.c_str(), ext.c_str(), (*it).second.Remapped, path.c_str());
		}
	}
	sipinfo (L"PATH: ");
	sipinfo (L"PATH: Contents of the alternative directory:");
	for (uint i = 0; i < inst->_AlternativePaths.size(); i++)
	{
		sipinfo (L"PATH: '%s'", inst->_AlternativePaths[i].c_str ());
	}
	sipinfo (L"PATH: ");
	sipinfo (L"PATH: Contents of the remapped entension table:");
	for (uint j = 0; j < inst->_Extensions.size(); j++)
	{
		sipinfo (L"PATH: '%s' -> '%s'", inst->_Extensions[j].first.c_str (), inst->_Extensions[j].second.c_str ());
	}
	sipinfo (L"PATH: End of display");
#else
	CPath *inst = CPath::getInstance ();
	sipinfo ("PATH: Contents of the map:");
	sipinfo ("PATH: %-25s %-5s %-5s %s", "filename", "ext", "remap", "full path");
	sipinfo ("PATH: ----------------------------------------------------");
	if (inst->_MemoryCompressed)
	{
		for (uint i = 0; i < inst->_MCFiles.size(); ++i)
		{
			const CMCFileEntry &fe = inst->_MCFiles[i];
			basic_string<ucchar> ext = inst->SSMext.get(fe.idExt);
			basic_string<ucchar> path = inst->SSMpath.get(fe.idPath);
			sipinfo ("PATH: %-25S %-5S %-5d %S", fe.Name, ext.c_str(), fe.Remapped, path.c_str());
		}
	}
	else
	{
		for (map<basic_string<ucchar>, CFileEntry>::iterator it = inst->_Files.begin(); it != inst->_Files.end (); it++)
		{
			basic_string<ucchar> ext = inst->SSMext.get((*it).second.idExt);
			basic_string<ucchar> path = inst->SSMpath.get((*it).second.idPath);
			sipinfo ("PATH: %-25S %-5S %-5d %S", (*it).first.c_str(), ext.c_str(), (*it).second.Remapped, path.c_str());
		}
	}
	sipinfo ("PATH: ");
	sipinfo ("PATH: Contents of the alternative directory:");
	for (uint i = 0; i < inst->_AlternativePaths.size(); i++)
	{
		sipinfo ("PATH: '%S'", inst->_AlternativePaths[i].c_str ());
	}
	sipinfo ("PATH: ");
	sipinfo ("PATH: Contents of the remapped entension table:");
	for (uint j = 0; j < inst->_Extensions.size(); j++)
	{
		sipinfo ("PATH: '%S' -> '%S'", inst->_Extensions[j].first.c_str (), inst->_Extensions[j].second.c_str ());
	}
	sipinfo ("PATH: End of display");
#endif
}

void CPath::memoryCompress()
{ 
	SIP_ALLOC_CONTEXT (MiPath);
	CPath *inst = CPath::getInstance();

	inst->SSMext.memoryCompress();
	inst->SSMpath.memoryCompress();
	uint nDbg = inst->_Files.size();
	sipinfo (L"PATH: Number of file : %d", nDbg);
	nDbg = inst->SSMext.getCount();
	sipinfo (L"PATH: Number of different extension : %d", nDbg);
	nDbg = inst->SSMpath.getCount();
	sipinfo (L"PATH: Number of different path : %d", nDbg);

	// Convert from _Files to _MCFiles
	uint nSize = 0, nNb = 0;
	map<basic_string<ucchar>, CFileEntry>::iterator it = inst->_Files.begin();
	while (it != inst->_Files.end())
	{
		basic_string<ucchar> sTmp = inst->SSMpath.get(it->second.idPath);
		if ((sTmp.find(L'@') != string::npos) && !it->second.Remapped)
		{
			// This is a file included in a bigfile (so the name is in the bigfile manager)
		}
		else
		{
			nSize += it->second.Name.size() + 1;
		}
		nNb++;
		it++;
	}

	inst->_AllFileNames = new ucchar[nSize];
	memset(inst->_AllFileNames, 0, nSize * sizeof(ucchar));
	inst->_MCFiles.resize(nNb);

	it = inst->_Files.begin();
	nSize = 0;
	nNb = 0;
	while (it != inst->_Files.end())
	{
		CFileEntry &rFE = it->second;
		basic_string<ucchar> sTmp = inst->SSMpath.get(rFE.idPath);
		if ((sTmp.find(L'@') != string::npos) && !rFE.Remapped)
		{
			// This is a file included in a bigfile (so the name is in the bigfile manager)
			sTmp = sTmp.substr(0, sTmp.size()-1);
			inst->_MCFiles[nNb].Name = CBigFile::getInstance().getFileNamePtrW(rFE.Name, sTmp);
			sipassert(inst->_MCFiles[nNb].Name != NULL);
		}
		else
		{
			wcscpy(inst->_AllFileNames + nSize, rFE.Name.c_str());
			inst->_MCFiles[nNb].Name = inst->_AllFileNames+nSize;
			nSize += rFE.Name.size() + 1;
		}
		
		inst->_MCFiles[nNb].idExt = rFE.idExt;
		inst->_MCFiles[nNb].idPath = rFE.idPath;
		inst->_MCFiles[nNb].Remapped = rFE.Remapped;

		nNb++;
		it++;
	}

	contReset(inst->_Files);
	inst->_MemoryCompressed = true;
}

void CPath::memoryUncompress()
{
	CPath *inst = CPath::getInstance ();
	inst->SSMext.memoryUncompress(); 
	inst->SSMpath.memoryUncompress(); 	
	inst->_MemoryCompressed = false;
}

std::string CPath::getWindowsDirectoryA()
{
#ifndef SIP_OS_WINDOWS
	sipwarning("not a ms windows platform");
	return "";
#else
	char winDir[MAX_PATH];
	UINT numChar = ::GetWindowsDirectoryA(winDir, MAX_PATH);
	if (numChar > MAX_PATH || numChar == 0)
	{
		sipwarning("Couldn't retrieve windows directory");
		return "";	
	}
	return CPath::standardizePath(winDir);
#endif
}

std::basic_string<ucchar> CPath::getWindowsDirectoryW()
{
#ifndef SIP_OS_WINDOWS
	sipwarning (L"not a ms windows platform");
	return L"";
#else
	ucchar winDir[MAX_PATH];
	UINT numChar = ::GetWindowsDirectoryW(winDir, MAX_PATH);
	if (numChar > MAX_PATH || numChar == 0)
	{
		sipwarning (L"Couldn't retrieve windows directory");
		return L"";	
	}
	return CPath::standardizePath (winDir);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

int CFileA::getLastSeparator (const string &filename)
{
	uint32 pos = filename.find_last_of ('/');
	if (pos == string::npos)
	{
		pos = filename.find_last_of ('\\');
		if (pos == string::npos)
		{
			pos = filename.find_last_of ('@');
		}
	}
	return pos;
}

int CFileW::getLastSeparator (const std::basic_string<ucchar> &filename)
{
	uint32 pos = filename.find_last_of (L'/');
	if (pos == string::npos)
	{
		pos = filename.find_last_of (L'\\');
		if (pos == string::npos)
		{
			pos = filename.find_last_of (L'@');
		}
	}
	return pos;
}

string CFileA::getFilename (const string &filename)
{
	uint32 pos = CFileA::getLastSeparator(filename);
	if (pos != string::npos)
		return filename.substr (pos + 1);
	else
		return filename;
}

std::basic_string<ucchar> CFileW::getFilename (const std::basic_string<ucchar> &filename)
{
	uint32 pos = CFileW::getLastSeparator(filename);
	if (pos != string::npos)
		return filename.substr (pos + 1);
	else
		return filename;
}

string CFileA::getFilenameWithoutExtension (const string &filename)
{
	string filename2 = getFilename (filename);
	uint32 pos = filename2.find_last_of ('.');
	if (pos == string::npos)
		return filename2;
	else
		return filename2.substr (0, pos);
}

std::basic_string<ucchar> CFileW::getFilenameWithoutExtension (const std::basic_string<ucchar> &filename)
{
	std::basic_string<ucchar> filename2 = getFilename (filename);
	uint32 pos = filename2.find_last_of (L'.');
	if (pos == string::npos)
		return filename2;
	else
		return filename2.substr (0, pos);
}

string CFileA::getExtension (const string &filename)
{
	uint32 pos = filename.find_last_of ('.');
	if (pos == string::npos)
		return "";
	else
		return filename.substr (pos + 1);
}

std::basic_string<ucchar> CFileW::getExtension (const std::basic_string<ucchar> &filename)
{
	uint32 pos = filename.find_last_of (L'.');
	if (pos == string::npos)
		return L"";
	else
		return filename.substr (pos + 1);
}

string CFileA::getPath (const string &filename)
{
	uint32 pos = CFileA::getLastSeparator(filename);
	if (pos != string::npos)
		return filename.substr (0, pos + 1);
	else
		return "";
}

std::basic_string<ucchar> CFileW::getPath (const std::basic_string<ucchar> &filename)
{
	uint32 pos = CFileW::getLastSeparator(filename);
	if (pos != string::npos)
		return filename.substr (0, pos + 1);
	else
		return L"";
}

bool CFileA::isDirectory (const string &filename)
{
#ifdef SIP_OS_WINDOWS
	DWORD res = GetFileAttributesA (filename.c_str());
	if (res == ~0U)
	{
		return false;
	}
	return (res & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else // SIP_OS_WINDOWS
	struct stat buf;
	int res = stat (filename.c_str (), &buf);
	if (res == -1)
	{
		sipwarning ("PATH: can't stat '%s' error %d '%s'", filename.c_str(), errno, strerror(errno));
		return false;
	}
	return (buf.st_mode & S_IFDIR) != 0;
#endif // SIP_OS_WINDOWS
}

bool CFileW::isDirectory (const std::basic_string<ucchar> &filename)
{
#ifdef SIP_OS_WINDOWS
	DWORD res = GetFileAttributesW(filename.c_str());
	if (res == ~0U)
	{
		return false;
	}
	return (res & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else // SIP_OS_WINDOWS
	struct stat buf;
	// changed by ysg 2009-05-16
//	int res = stat (filename.c_str(), &buf);
	int res = stat (getMbsFromUcs(filename.c_str ()).c_str(), &buf) == 0; // KSR_ADD 2010_1_29
	if (res == -1)
	{
		sipwarning ("PATH: can't stat '%s' error %d '%s'", filename.c_str(), errno, strerror(errno));
		return false;
	}
	return (buf.st_mode & S_IFDIR) != 0;
#endif // SIP_OS_WINDOWS
}

bool CFileA::isExists (const string &filename)
{
#ifdef SIP_OS_WINDOWS
	return (GetFileAttributesA(filename.c_str()) != ~0U);
#else // SIP_OS_WINDOWS
	struct stat buf;
	return stat (filename.c_str (), &buf) == 0;
#endif // SIP_OS_WINDOWS
}

bool CFileW::isExists (const std::basic_string<ucchar>& filename)
{
#ifdef SIP_OS_WINDOWS
	return (GetFileAttributesW(filename.c_str()) != ~0U);
#else // SIP_OS_WINDOWS
	struct stat buf;
	// changed by ysg 2009-05-16
//	return stat (filename.c_str (), &buf) == 0;
	return stat (getMbsFromUcs(filename.c_str ()).c_str(), &buf) == 0; // KSR_ADD 2010_1_29
#endif // SIP_OS_WINDOWS
}

bool CFileA::fileExists (const string& filename)
{
	//H_AUTO(FileExists);
	return ! ! fstream( filename.c_str(), ios::in );
}

bool CFileW::fileExists (const std::basic_string<ucchar> &filename)
{
	FILE *stream = sfWfopen(filename.c_str(), L"r");
	if (stream)
		fclose(stream);
	
	return (stream != NULL);
}

string CFileA::findNewFile (const string &filename)
{
	uint32 pos = filename.find_last_of ('.');
	if (pos == string::npos)
		return filename;
	
	string start = filename.substr (0, pos);
	string end = filename.substr (pos);

	static uint num = 0;	// Modified by RSC 2015/05/06
	const int chcount = 7;
	char numchar[chcount];
	string npath;
	do
	{
		npath = start;
		smprintf(numchar,chcount,"_%05d",num++);
		if (num >= 99999)
			num = 0;
		npath += numchar;
		npath += end;
		if (!CFileA::fileExists(npath)) break;
	}
	while (num < 99999);
	return npath;
}

std::basic_string<ucchar> CFileW::findNewFile (const std::basic_string<ucchar> &filename)
{
	uint32 pos = filename.find_last_of (L'.');
	if (pos == string::npos)
		return filename;
	
	std::basic_string<ucchar> start = filename.substr (0, pos);
	std::basic_string<ucchar> end = filename.substr (pos);

	static uint num = 0;	// Modified by RSC 2015/05/06
	const int chcount = 7;
	ucchar numchar[chcount];
	std::basic_string<ucchar> npath;
	do
	{
		npath = start;
		smprintf (numchar, chcount, L"_%05d", num++);
		if (num >= 99999)
			num = 0;
		npath += numchar;
		npath += end;
		if (!CFileW::fileExists(npath)) break;
	}
	while (num < 99999);
	return npath;
}

// \warning doesn't work with big file
uint32	CFileA::getFileSize (const std::string &filename)
{
/*	FILE *fp = fopen (filename.c_str(), "rb");
	if (fp == NULL) return 0;
	sipfseek64 (fp, 0, SEEK_END);
	uint32 size = ftell (fp);
	fclose (fp);
	return size;*/

/*	const char *s = filename.c_str();
	int h = _open (s, _O_RDONLY | _O_BINARY);
	_lseek (h, 0, SEEK_END);
	uint32 size = _tell (h);
	_close (h);
	return size;
*/

	if (filename.find('@') != string::npos)
	{
		uint32 fs = 0, bfo;
		bool c, d;
		CBigFile::getInstance().getFile (filename, fs, bfo, c, d);
		return fs;
	}
	else
	{
#if defined (SIP_OS_WINDOWS)
		struct _stat buf;
		int result = _stat (filename.c_str (), &buf);
#elif defined (SIP_OS_UNIX)
		struct stat buf;
		int result = stat (filename.c_str (), &buf);
#endif
		if (result != 0) return 0;
		else return buf.st_size;
	}
}

uint32	CFileW::getFileSize (const std::basic_string<ucchar> &filename)
{
	if (filename.find(L'@') != string::npos)
	{
		uint32 fs = 0, bfo;
		bool c, d;
		CBigFile::getInstance().getFileW (filename, fs, bfo, c, d);
		return fs;
	}
	else
	{
#if defined (SIP_OS_WINDOWS)
		struct _stat buf;
		int result = _wstat (filename.c_str (), &buf);
#elif defined (SIP_OS_UNIX)
		struct stat buf;
		// changed by ysg 2009-05-16
//		int result = stat (filename.c_str (), &buf);
		int result = stat (getMbsFromUcs(filename.c_str ()).c_str(), &buf) == 0; // KSR_ADD 2010_1_29
#endif
		if (result != 0) return 0;
		else return buf.st_size;
	}
}

uint32	CFileA::getFileSize (FILE *f)
{
#if defined (SIP_OS_WINDOWS)
	struct _stat buf;
	int result = _fstat (fileno(f), &buf);
#elif defined (SIP_OS_UNIX)
	struct stat buf;
	int result = fstat (fileno(f), &buf);
#endif
	if (result != 0) return 0;
	else return buf.st_size;
}

uint32	CFileW::getFileSize (FILE *f)
{
	return CFileA::getFileSize(f);
}

uint32	CFileA::getFileModificationDate(const std::string &filename)
{
	uint pos;
	string fn;
	if ((pos=filename.find('@')) != string::npos)
	{
		fn = CPath::lookup(filename.substr (0, pos));
	}
	else
	{
		fn = filename;
	}

#if defined (SIP_OS_WINDOWS)
	struct _stat buf;
	int result = _stat (fn.c_str (), &buf);
#elif defined (SIP_OS_UNIX)
	struct stat buf;
	int result = stat (fn.c_str (), &buf);
#endif

	if (result != 0) return 0;
	else return (uint32)buf.st_mtime;
}

uint32	CFileW::getFileModificationDate(const std::basic_string<ucchar> &filename)
{
	uint pos;
	std::basic_string<ucchar> fn;
	if ((pos = filename.find(L'@')) != string::npos)
	{
		fn = CPath::lookup (filename.substr (0, pos));
	}
	else
	{
		fn = filename;
	}

#if defined (SIP_OS_WINDOWS)
	struct _stat buf;
	int result = _wstat (fn.c_str (), &buf);
#elif defined (SIP_OS_UNIX)
	struct stat buf;
	// changed by ysg 2009-05-16
//	int result = stat (fn.c_str (), &buf);
	int result = stat (getMbsFromUcs(fn.c_str ()).c_str(), &buf) == 0; // KSR_ADD 2010_1_29
#endif

	if (result != 0) return 0;
	else return (uint32)buf.st_mtime;
}

uint32	CFileA::getFileCreationDate(const std::string &filename)
{
	uint pos;
	string fn;
	if ((pos=filename.find('@')) != string::npos)
	{
		fn = CPath::lookup(filename.substr (0, pos));
	}
	else
	{
		fn = filename;
	}

#if defined (SIP_OS_WINDOWS)
	struct _stat buf;
	int result = _stat (fn.c_str (), &buf);
#elif defined (SIP_OS_UNIX)
	struct stat buf;
	int result = stat (fn.c_str (), &buf);
#endif

	if (result != 0) return 0;
	else return (uint32)buf.st_ctime;
}

uint32	CFileW::getFileCreationDate(const std::basic_string<ucchar> &filename)
{
	uint pos;
	std::basic_string<ucchar> fn;
	if ((pos = filename.find(L'@')) != string::npos)
	{
		fn = CPath::lookup (filename.substr (0, pos));
	}
	else
	{
		fn = filename;
	}

#if defined (SIP_OS_WINDOWS)
	struct _stat buf;
	int result = _wstat (fn.c_str (), &buf);
#elif defined (SIP_OS_UNIX)
	struct stat buf;
	// changed by ysg 2009-05-16
//	int result = stat (fn.c_str (), &buf);
	int result = stat (getMbsFromUcs(fn.c_str ()).c_str(), &buf) == 0;  // KSR_ADD 2010_1_29
#endif

	if (result != 0) return 0;
	else return (uint32)buf.st_ctime;
}

struct CFileEntry
{
	CFileEntry (const std::string &filename, void (*callback)(const string &filename))
		: Callback (callback), CallbackW (NULL)
	{
		FileName = SIPBASE::getUcsFromMbs(filename.c_str());
		LastModified = CFileA::getFileModificationDate(filename);
	}
	
	CFileEntry (const std::basic_string<ucchar> &filename, void (*callback)(const std::basic_string<ucchar> &filename))
		: FileName (filename), Callback(NULL), CallbackW (callback)
	{
		LastModified = CFileW::getFileModificationDate(filename);
	}

	std::basic_string<ucchar> FileName;
	
	void (*Callback) (const string &filename);
	void (*CallbackW)(const std::basic_string<ucchar> &filename);

	uint32 LastModified;
};

static vector <CFileEntry> FileToCheck;

void CFileA::removeFileChangeCallback (const std::string &filename)
{
	std::basic_string<ucchar> fn = SIPBASE::getUcsFromMbs(filename.c_str());
	CFileW::removeFileChangeCallback (fn);
}

void CFileW::removeFileChangeCallback (const std::basic_string<ucchar> &filename)
{
	std::basic_string<ucchar> fn = CPath::lookup (filename, false, false);
	if (fn.empty())
	{
		fn = filename;
	}
	for (uint i = 0; i < FileToCheck.size(); i++)
	{
		if (FileToCheck[i].FileName == fn)
		{
			sipinfo (L"PATH: CFileW::removeFileChangeCallback: '%s' is removed from checked files modification", fn.c_str());
			FileToCheck.erase(FileToCheck.begin() + i);
			return;
		}
	}
}

void CFileA::addFileChangeCallback (const std::string &filename, void (*cb)(const string &filename))
{
	string fn = CPath::lookup(filename, false, false);
	if (fn.empty())
	{
		fn = filename;
	}
	sipinfo ("PATH: CFileA::addFileChangeCallback: I'll check the modification date for this file '%s'", fn.c_str());

	FileToCheck.push_back(CFileEntry(fn, cb));
}

void CFileW::addFileChangeCallback(const std::basic_string<ucchar> &filename, void (*cb)(const std::basic_string<ucchar> &filename))
{
	std::basic_string<ucchar> fn = CPath::lookup (filename, false, false);
	if (fn.empty())
	{
		fn = filename;
	}
	sipinfo (L"PATH: CFileW::addFileChangeCallback: I'll check the modification date for this file '%s'", fn.c_str());

	FileToCheck.push_back(CFileEntry(fn, cb));
}

void CFileA::checkFileChange (TTime frequency)
{
	static TTime lastChecked = CTime::getLocalTime();

	if (CTime::getLocalTime() > lastChecked + frequency)
	{
		for (uint i = 0; i < FileToCheck.size(); i++)
		{
			if (CFileW::getFileModificationDate(FileToCheck[i].FileName) != FileToCheck[i].LastModified)
			{
				// need to reload it
				if (FileToCheck[i].Callback != NULL)
				{
					std::string fn = SIPBASE::toString("%S", FileToCheck[i].FileName.c_str());
					FileToCheck[i].Callback(fn);
				}
				else if (FileToCheck[i].CallbackW != NULL)
					FileToCheck[i].CallbackW(FileToCheck[i].FileName);

				FileToCheck[i].LastModified = CFileW::getFileModificationDate(FileToCheck[i].FileName);
			}
		}

		lastChecked = CTime::getLocalTime();
	}
}

void CFileW::checkFileChange (TTime frequency)
{
	CFileA::checkFileChange (frequency);
}

static bool CopyMoveFile(const char *dest, const char *src, bool copyFile, bool failIfExists = false)
{
	if (!dest || !src) return false;
	if (!strlen(dest) || !strlen(src)) return false;	
	std::string sdest = CPath::standardizePath(dest,false);
	std::string ssrc = CPath::standardizePath(src,false);

//	return copyFile  ? CopyFile(dossrc.c_str(), dosdest.c_str(), failIfExists) != FALSE
//					 : MoveFile(dossrc.c_str(), dosdest.c_str()) != FALSE;

	if (copyFile)
	{
		FILE *fp1 = fopen(ssrc.c_str(), "rb");
		if (fp1 == NULL)
		{
			sipwarning ("PATH: CopyMoveFile error: can't fopen in read mode '%s'", ssrc.c_str());
			return false;
		}
		FILE *fp2 = fopen(sdest.c_str(), "wb");
		if (fp2 == NULL)
		{
			sipwarning ("PATH: CopyMoveFile error: can't fopen in read write mode '%s'", sdest.c_str());
			return false;
		}
		static char buffer [1000];
		size_t s;

		s = fread(buffer, 1, sizeof(buffer), fp1);
		while (s != 0)
		{
			size_t ws = fwrite(buffer, 1, s, fp2);
			if (ws != s)
			{
				sipwarning("Error copying '%s' to '%s', trying to write %u bytes, only %u written",
					ssrc.c_str(),
					sdest.c_str(),
					s,
					ws);
			}
			s = fread(buffer, 1, sizeof(buffer), fp1);
		}

		fclose(fp1);
		fclose(fp2);
	}
	else
	{
#ifdef SIP_OS_WINDOWS
		if (MoveFileA(ssrc.c_str(), sdest.c_str()) == 0)
		{
			LPVOID lpMsgBuf;
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					    FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPSTR) &lpMsgBuf,
						0,
						NULL );
			sipwarning ("PATH: CopyMoveFile error: can't link/move '%s' into '%s', error %u (%s)", 
				ssrc.c_str(), 
				sdest.c_str(),
				GetLastError(),
				lpMsgBuf);

			LocalFree(lpMsgBuf);
			return false;
		}
#else
		if (link (ssrc.c_str(), sdest.c_str()) == -1)
		{
			sipwarning ("PATH: CopyMoveFile error: can't link/move '%s' into '%s', error %u", 
				ssrc.c_str(), 
				sdest.c_str(),
				errno);
			return false;
		}
#endif
#ifndef SIP_OS_WINDOWS
		if (unlink (ssrc.c_str()) == -1)
		{
			sipwarning ("PATH: CopyMoveFile error: can't delete/unlink '%s'", ssrc.c_str());
			return false;
		}
#endif
	}
	return true;
}

static bool CopyMoveFileW(const ucchar *dest, const ucchar *src, bool copyFile, bool failIfExists = false)
{
	if (!dest || !src) return false;
	if (!wcslen(dest) || !wcslen(src)) return false;	
	std::basic_string<ucchar> sdest = CPath::standardizePath (dest, false);
	std::basic_string<ucchar> ssrc = CPath::standardizePath (src,false);

	if (copyFile)
	{
		FILE *fp1 = sfWfopen(ssrc.c_str(), L"rb");
		if (fp1 == NULL)
		{
			sipwarning (L"PATH: CopyMoveFile error: can't fopen in read mode '%s'", ssrc.c_str());
			return false;
		}

		FILE *fp2 = sfWfopen(sdest.c_str(), L"wb");
		if (fp2 == NULL)
		{
			fclose(fp1);
			sipwarning (L"PATH: CopyMoveFile error: can't fopen in read write mode '%s'", sdest.c_str());
			return false;
		}

		static char buffer [1000];
		size_t s;

		s = fread(buffer, 1, sizeof(buffer), fp1);
		while (s != 0)
		{
			size_t ws = fwrite(buffer, 1, s, fp2);
			if (ws != s)
			{
				sipwarning(L"Error copying '%s' to '%s', trying to write %u bytes, only %u written",
					ssrc.c_str(),
					sdest.c_str(),
					s,
					ws);
			}
			s = fread(buffer, 1, sizeof(buffer), fp1);
		}

		fclose(fp1);
		fclose(fp2);
	}
	else
	{
#ifdef SIP_OS_WINDOWS
		if (MoveFileW(ssrc.c_str(), sdest.c_str()) == 0)
		{
			LPVOID lpMsgBuf;
			FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					    FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPWSTR) &lpMsgBuf,
						0,
						NULL );
			sipwarning (L"PATH: CopyMoveFile error: can't link/move '%s' into '%s', error %u (%s)", 
				ssrc.c_str(), 
				sdest.c_str(),
				GetLastError(),
				lpMsgBuf);

			LocalFree(lpMsgBuf);
			return false;
		}
#else
		// changed by ysg 2009-05-16
//		if (link (ssrc.c_str(), sdest.c_str()) == -1)
		if (unlink (SIPBASE::getMbsFromUcs(ssrc.c_str()).c_str()) == -1)  // KSR_ADD 2010_1_29
		{
			sipwarning (L"PATH: CopyMoveFile error: can't link/move '%s' into '%s', error %u", 
				ssrc.c_str(), 
				sdest.c_str(),
				errno);
			return false;
		}
#endif
#ifndef SIP_OS_WINDOWS
		// changed by ysg 2009-05-16
//		if (unlink (ssrc.c_str()) == -1)
		if (unlink (SIPBASE::getMbsFromUcs(ssrc.c_str()).c_str()) == -1) // KSR_ADD 2010_1_29
		{
			sipwarning (L"PATH: CopyMoveFile error: can't delete/unlink '%s'", ssrc.c_str());
			return false;
		}
#endif
	}
	return true;
}

bool CFileA::copyFile(const char *dest, const char *src, bool failIfExists /*=false*/)
{
	return CopyMoveFile(dest, src, true, failIfExists);
}

bool CFileW::copyFile(const ucchar *dest, const ucchar *src, bool failIfExists /*=false*/)
{
	return CopyMoveFileW(dest, src, true, failIfExists);
}

bool CFileA::quickFileCompare(const std::string &fileName0, const std::string &fileName1)
{
	// make sure the files both exist
	if (!fileExists(fileName0.c_str()) || !fileExists(fileName1.c_str()))
		return false;

	// compare time stamps
	if (getFileModificationDate(fileName0.c_str()) != getFileModificationDate(fileName1.c_str()))
		return false;

	// compare file sizes
	if (getFileSize(fileName0.c_str()) != getFileSize(fileName1.c_str()))
		return false;

	// everything matched so return true
	return true;
}

bool CFileW::quickFileCompare(const std::basic_string<ucchar> &fileName0, const std::basic_string<ucchar> &fileName1)
{
	// make sure the files both exist
	if (!fileExists(fileName0.c_str()) || !fileExists(fileName1.c_str()))
		return false;

	// compare time stamps
	if (getFileModificationDate(fileName0.c_str()) != getFileModificationDate(fileName1.c_str()))
		return false;

	// compare file sizes
	if (getFileSize(fileName0.c_str()) != getFileSize(fileName1.c_str()))
		return false;

	// everything matched so return true
	return true;
}

bool CFileA::thoroughFileCompare(const std::string &fileName0, const std::string &fileName1,uint32 maxBufSize)
{
	// make sure the files both exist
	if (!fileExists(fileName0.c_str()) || !fileExists(fileName1.c_str()))
		return false;

	// setup the size variable from file length of first file
	uint32 fileSize=getFileSize(fileName0.c_str());

	// compare file sizes
	if (fileSize != getFileSize(fileName1.c_str()))
		return false;

	// allocate a couple of data buffers for our 2 files
	uint32 bufSize= maxBufSize/2;
	sipassert(sint32(bufSize)>0);
	std::vector<uint8> buf0(bufSize);
	std::vector<uint8> buf1(bufSize);

	// open the two files for input
	CIFile file0(fileName0);
	CIFile file1(fileName1);

	for (uint32 i=0;i<fileSize;i+=bufSize)
	{
		// for the last block in the file reduce buf size to represent the amount of data left in file
		if (i+bufSize>fileSize)
		{
			bufSize= fileSize-i;
			buf0.resize(bufSize);
			buf1.resize(bufSize);
		}

		// read in the next data block from disk
		file0.serialBuffer(&buf0[0], bufSize);
		file1.serialBuffer(&buf1[0], bufSize);

		// compare the contents of hte 2 data buffers
		if (buf0!=buf1)
			return false;
	}

	// everything matched so return true
	return true;
}

bool CFileW::thoroughFileCompare(const std::basic_string<ucchar> &fileName0, const std::basic_string<ucchar> &fileName1,uint32 maxBufSize)
{
	// make sure the files both exist
	if (!fileExists(fileName0.c_str()) || !fileExists(fileName1.c_str()))
		return false;

	// setup the size variable from file length of first file
	uint32 fileSize=getFileSize(fileName0.c_str());

	// compare file sizes
	if (fileSize != getFileSize(fileName1.c_str()))
		return false;

	// allocate a couple of data buffers for our 2 files
	uint32 bufSize= maxBufSize/2;
	sipassert(sint32(bufSize)>0);
	std::vector<uint16> buf0(bufSize);
	std::vector<uint16> buf1(bufSize);

	// open the two files for input
	CIFile file0(fileName0);
	CIFile file1(fileName1);

	for (uint32 i=0;i<fileSize;i+=bufSize)
	{
		// for the last block in the file reduce buf size to represent the amount of data left in file
		if (i+bufSize>fileSize)
		{
			bufSize= fileSize-i;
			buf0.resize(bufSize);
			buf1.resize(bufSize);
		}

		// read in the next data block from disk
		file0.serialBuffer((uint8 *)&buf0[0], bufSize * sizeof(uint16));
		file1.serialBuffer((uint8 *)&buf1[0], bufSize * sizeof(uint16));

		// compare the contents of hte 2 data buffers
		if (buf0!=buf1)
			return false;
	}

	// everything matched so return true
	return true;
}

bool CFileA::moveFile(const char *dest,const char *src)
{
	return CopyMoveFile(dest, src, false);
}

bool CFileW::moveFile(const ucchar *dest,const ucchar *src)
{
	return CopyMoveFileW(dest, src, false);
}

bool CFileA::createDirectory(const std::string &filename)
{
#ifdef SIP_OS_WINDOWS
	return _mkdir(filename.c_str())==0;
#else
	// Set full permissions....
	return mkdir(filename.c_str(), 0xFFFF)==0;
#endif
}

bool CFileW::createDirectory(const std::basic_string<ucchar> &filename)
{
#ifdef SIP_OS_WINDOWS
	return _wmkdir(filename.c_str()) == 0;
#else
	// Set full permissions....
	// changed by ysg 2009-05-16
//	return mkdir(filename.c_str(), 0xFFFF)==0;
	return mkdir(SIPBASE::getMbsFromUcs(filename.c_str()).c_str(), 0xFFFF)==0; // KSR_ADD 2010_1_29
#endif
}

bool CFileA::createDirectoryTree(const std::string &filename)
{
	bool lastResult=true;
	uint32 i=0;

	// skip dos drive name eg "a:"
	if (filename.size()>1 && filename[1]==':')
		i=2;

	// iterate over the set of directories in the routine's argument
	while (i<filename.size())
	{
		// skip passed leading slashes
		for (;i<filename.size();++i)
			if (filename[i]!='\\' && filename[i]!='/')
				break;

		// if the file name ended with a '/' then there's no extra directory to create
		if (i==filename.size())
			break;

		// skip forwards to next slash
		for (;i<filename.size();++i)
			if (filename[i]=='\\' || filename[i]=='/')
				break;

		// try to create directory
		std::string s= filename.substr(0,i);
		lastResult= createDirectory(s);
	}

	return lastResult;
}

bool CFileW::createDirectoryTree(const std::basic_string<ucchar> &filename)
{
	bool lastResult=true;
	uint32 i=0;

	// skip dos drive name eg "a:"
	if (filename.size()>1 && filename[1]==L':')
		i=2;

	// iterate over the set of directories in the routine's argument
	while (i<filename.size())
	{
		// skip passed leading slashes
		for (;i<filename.size();++i)
			if (filename[i]!=L'\\' && filename[i]!=L'/')
				break;

		// if the file name ended with a '/' then there's no extra directory to create
		if (i==filename.size())
			break;

		// skip forwards to next slash
		for (;i<filename.size();++i)
			if (filename[i]==L'\\' || filename[i]==L'/')
				break;

		// try to create directory
		std::basic_string<ucchar> s= filename.substr(0,i);
		lastResult= createDirectory(s);
	}

	return lastResult;
}

bool CPath::makePathRelative (const char *basePath, std::string &relativePath)
{
	// Standard path with final slash
	string tmp = standardizePath (basePath, true);
	string src = standardizePath (relativePath, true);
	string prefix;

	while (1)
	{
		// Compare with relativePath
		if (strncmp (tmp.c_str (), src.c_str (), tmp.length ()) == 0)
		{
			// Troncate
			uint size = tmp.length ();

			// Same path ?
			if (size == src.length ())
			{
				relativePath = ".";
				return true;
			}

			relativePath = prefix+relativePath.substr (size, relativePath.length () - size);
			return true;
		}

		// Too small ?
		if (tmp.length () < 2)
			break;

		// Remove last directory
		uint lastPos = tmp.rfind ('/', tmp.length ()-2);
		uint previousPos = tmp.find ('/');
		if ((lastPos == previousPos) || (lastPos == string::npos))
			break;

		// Troncate
		tmp = tmp.substr (0, lastPos+1);

		// New prefix
		prefix += "../";
	}
	
	return false;
}

bool CPath::makePathRelative (const ucchar *basePath, std::basic_string<ucchar> &relativePath)
{
	// Standard path with final slash
	basic_string<ucchar> tmp = standardizePath (basePath, true);
	basic_string<ucchar> src = standardizePath (relativePath, true);
	basic_string<ucchar> prefix;

	while (1)
	{
		// Compare with relativePath
		if (wcsncmp (tmp.c_str (), src.c_str (), tmp.length ()) == 0)
		{
			// Troncate
			uint size = tmp.length ();

			// Same path ?
			if (size == src.length ())
			{
				relativePath = L".";
				return true;
			}

			relativePath = prefix + relativePath.substr (size, relativePath.length () - size);
			return true;
		}

		// Too small ?
		if (tmp.length () < 2)
			break;

		// Remove last directory
		uint lastPos = tmp.rfind (L'/', tmp.length () - 2);
		uint previousPos = tmp.find (L'/');
		if ((lastPos == previousPos) || (lastPos == string::npos))
			break;

		// Troncate
		tmp = tmp.substr (0, lastPos+1);

		// New prefix
		prefix += L"../";
	}
	
	return false;
}

bool CFileA::setRWAccess(const std::string &filename)
{
#ifdef SIP_OS_WINDOWS
	// if the file exists and there's no write access
	if (_access (filename.c_str(), 00) == 0 && _access (filename.c_str(), 06) == -1)
	{
		// try to set the read/write access
		if (_chmod (filename.c_str(), _S_IREAD | _S_IWRITE) == -1)
		{
			sipwarning ("PATH: Can't set RW access to file '%s': %d %s", filename.c_str(), errno, strerror(errno));
			return false;
		}
	}
#else
	// if the file exists and there's no write access
	if (access (filename.c_str(), F_OK) == 0)
	{
		// try to set the read/write access
		if (chmod (filename.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH) == -1)
		{
			sipwarning ("PATH: Can't set RW access to file '%s': %d %s", filename.c_str(), errno, strerror(errno));
			return false;
		}
	}
	else
	{
		sipwarning("PATH: Can't access to file '%s'", filename.c_str());
	}
#endif
	return true;
}

bool CFileW::setRWAccess(const std::basic_string<ucchar> &filename)
{
#ifdef SIP_OS_WINDOWS
	// if the file exists and there's no write access
	if (_waccess (filename.c_str(), 00) == 0 && _waccess (filename.c_str(), 06) == -1)
	{
		// try to set the read/write access
		if (_wchmod (filename.c_str(), _S_IREAD | _S_IWRITE) == -1)
		{
			sipwarning (L"PATH: Can't set RW access to file '%s': %d %s", filename.c_str(), errno, _wcserror(errno));
			return false;
		}
	}
#else
	// if the file exists and there's no write access
	// changed by ysg 2009-05-16
//	if (access (filename.c_str(), F_OK) == 0)
	if (access (SIPBASE::getMbsFromUcs(filename.c_str()).c_str(), F_OK) == 0)  // KSR_ADD 2010_1_29
	{
		// try to set the read/write access
		// changed by ysg 2009-05-16
//		if (chmod (filename.c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH) == -1)
		if (chmod (SIPBASE::getMbsFromUcs(filename.c_str()).c_str(), S_IRUSR|S_IWUSR|S_IXUSR|S_IRGRP|S_IWGRP|S_IXGRP|S_IROTH|S_IWOTH|S_IXOTH) == -1)  // KSR_ADD 2010_1_29
		{
			sipwarning (L"PATH: Can't set RW access to file '%s': %d %s", filename.c_str(), errno, strerror(errno));
			return false;
		}
	}
	else
	{
		sipwarning(L"PATH: Can't access to file '%s'", filename.c_str());
	}
#endif
	return true;
}

bool CFileA::deleteFile(const std::string &filename)
{
	setRWAccess(filename);

#ifdef SIP_OS_WINDOWS
	int res = _unlink (filename.c_str());
#else
	int res = unlink (filename.c_str());
#endif
	if (res == -1)
	{
		sipwarning ("PATH: Can't delete file '%s': (errno %d) %s", filename.c_str(), errno, strerror(errno));
		return false;
	}
	return true;
}

bool CFileW::deleteFile(const std::basic_string<ucchar> &filename)
{
	setRWAccess(filename);

#ifdef SIP_OS_WINDOWS
	int res = _wunlink (filename.c_str());
	if (res == -1)
	{
		sipwarning (L"PATH: Can't delete file '%s': (errno %d) %s", filename.c_str(), errno, _wcserror(errno));
		return false;
	}
#else
	// changed by ysg 2009-05-16
//	int res = unlink (filename.c_str());
	int res = unlink (SIPBASE::getMbsFromUcs(filename.c_str()).c_str());  // KSR_ADD 2010_1_29
	if (res == -1)
	{
		sipwarning (L"PATH: Can't delete file '%s': (errno %d) %s", filename.c_str(), errno, strerror(errno));
		return false;
	}
#endif
	return true;
}

void CFileA::getTemporaryOutputFilename (const std::string &originalFilename, std::string &tempFilename)
{
	uint i = 0;
	do
		tempFilename = originalFilename + ".tmp" + toStringA (i ++);
	while (CFileA::isExists(tempFilename));
}

void CFileW::getTemporaryOutputFilename(const std::basic_string<ucchar> &originalFilename, std::basic_string<ucchar> &tempFilename)
{
	uint i = 0;
	do
		#ifdef SIP_OS_WINDOWS
			tempFilename = originalFilename + L".tmp" + SIPBASE::toString(L"%hu", i ++);
		#else
			tempFilename = originalFilename + L".tmp" + SIPBASE::toStringLW("%hu", i ++);
		#endif
	while (CFileW::isExists(tempFilename));
}

} // SIPBASE
