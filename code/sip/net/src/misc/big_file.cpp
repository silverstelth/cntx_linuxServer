/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/big_file.h"
#include "misc/path.h"

using namespace std;
using namespace SIPBASE;


namespace SIPBASE {

//CBigFile *CBigFile::_Singleton = NULL;
SIPBASE_SAFE_SINGLETON_IMPL(CBigFile);

// ***************************************************************************
void CBigFile::releaseInstance()
{
	if( _Instance )
		delete _Instance;
	_Instance = NULL;
}
// ***************************************************************************
CBigFile::CThreadFileArray::CThreadFileArray()
{
	_CurrentId= 0;
}
// ***************************************************************************
uint32						CBigFile::CThreadFileArray::allocate()
{
	return _CurrentId++;
}

// ***************************************************************************
CBigFile::CHandleFile		&CBigFile::CThreadFileArray::get(uint32 index)
{
	// If the thread struct ptr is NULL, must allocate it.
	vector<CHandleFile>		*ptr= (vector<CHandleFile>*)_TDS.getPointer();
	if(ptr==NULL)
	{
		ptr= new vector<CHandleFile>;
		_TDS.setPointer(ptr);
	}

	// if the vector is not allocated, allocate it (empty entries filled with NULL => not opened FILE* in this thread)
	if(index>=ptr->size())
	{
		ptr->resize(index+1);
	}

	return (*ptr)[index];
}


// ***************************************************************************
void CBigFile::currentThreadFinished()
{
	_ThreadFileArray.currentThreadFinished();
}

// ***************************************************************************
void CBigFile::CThreadFileArray::currentThreadFinished()
{
	vector<CHandleFile>		*ptr= (vector<CHandleFile>*)_TDS.getPointer();
	if (ptr==NULL) return;
	for (uint k = 0; k < ptr->size(); ++k)
	{
		if ((*ptr)[k].File)
		{
			fclose((*ptr)[k].File);
			(*ptr)[k].File = NULL;			
		}
	}
	delete ptr;
	_TDS.setPointer(NULL);
}


// ***************************************************************************
//CBigFile::CBigFile ()
//{
//}
//
//// ***************************************************************************
//CBigFile &CBigFile::getInstance ()
//{
//	if (_Singleton == NULL)
//	{
//		_Singleton = new CBigFile();
//	}
//	return *_Singleton;
//}

// ***************************************************************************
bool CBigFile::add (const std::string &sBigFileName, uint32 nOptions)
{
	return addW(SIPBASE::toString(L"%S", sBigFileName.c_str()), nOptions);
}

bool CBigFile::addW(const std::basic_string<ucchar> &sBigFileName, uint32 nOptions)
{
	BNP bnpTmp;

	bnpTmp.BigFileName= sBigFileName;

	// Is already the same bigfile name ?
	std::basic_string<ucchar> bigfilenamealone = CFileW::getFilename(sBigFileName);
	if (_BNPs.find(bigfilenamealone) != _BNPs.end())
	{
		sipwarning (L"CBigFile::add : bigfile %s already added.", bigfilenamealone.c_str());
		return false;
	}

	// Allocate a new ThreadSafe FileId for this bnp.
	bnpTmp.ThreadFileId= _ThreadFileArray.allocate();

	// Get a ThreadSafe handle on the file
	CHandleFile		&handle= _ThreadFileArray.get(bnpTmp.ThreadFileId);
	// Open the big file.
	handle.File = sfWfopen (sBigFileName.c_str(), L"rb");
	if (handle.File == NULL)
		return false;
	uint32 nFileSize= CFileA::getFileSize (handle.File);
	//sipfseek64 (handle.File, 0, SEEK_END);
	//uint32 nFileSize = ftell (handle.File);

	// Result
	if (sipfseek64 (handle.File, nFileSize-4, SEEK_SET) != 0)
	{
		fclose (handle.File);		
		handle.File = NULL;
		return false;
	}

	uint32 nOffsetFromBegining;
	if (fread (&nOffsetFromBegining, sizeof(uint32), 1, handle.File) != 1)
	{
		fclose (handle.File);		
		handle.File = NULL;
		return false;
	}

	if (sipfseek64 (handle.File, nOffsetFromBegining, SEEK_SET) != 0)
	{
		fclose (handle.File);		
		handle.File = NULL;
		return false;
	}

	// Read the file count
	uint32 nNbFile;
	if (fread (&nNbFile, sizeof(uint32), 1, handle.File) != 1)
	{
		fclose (handle.File);		
		handle.File = NULL;
		return false;
	}

	map<basic_string<ucchar>, BNPFile> tempMap;
	for (uint32 i = 0; i < nNbFile; ++i)
	{
		ucchar FileName[256];
		uint8 nStringSize;
		if (fread (&nStringSize, 1, 1, handle.File) != 1)
		{
			fclose (handle.File);			
			handle.File = NULL;
			return false;
		}

		if (fread (FileName, sizeof(ucchar), nStringSize, handle.File) != nStringSize)
		{
			fclose (handle.File);			
			handle.File = NULL;
			return false;
		}

		FileName[nStringSize] = 0;
		uint32 nFileSize2;
		if (fread (&nFileSize2, sizeof(uint32), 1, handle.File) != 1)
		{
			fclose (handle.File);			
			handle.File = NULL;
			return false;
		}

		uint32 nFilePos;
		if (fread (&nFilePos, sizeof(uint32), 1, handle.File) != 1)
		{
			fclose (handle.File);			
			handle.File = NULL;
			return false;
		}

		BNPFile bnpfTmp;
		bnpfTmp.Pos = nFilePos;
		bnpfTmp.Size = nFileSize2;
		tempMap.insert (make_pair(toLower(basic_string<ucchar>(FileName)), bnpfTmp));
	}

	if (sipfseek64 (handle.File, 0, SEEK_SET) != 0)
	{
		fclose (handle.File);		
		handle.File = NULL;
		return false;
	}

	// Convert temp map
	if (nNbFile > 0)
	{
		uint nSize = 0, nNb = 0;
		map<basic_string<ucchar>, BNPFile>::iterator it = tempMap.begin();
		while (it != tempMap.end())
		{
			nSize += it->first.size() + 1;
			nNb ++;
			it ++;
		}

		bnpTmp.FileNames = new ucchar[nSize];
		memset(bnpTmp.FileNames, 0, nSize);
		bnpTmp.Files.resize(nNb);

		it = tempMap.begin();
		nSize = 0;
		nNb = 0;
		while (it != tempMap.end())
		{
			wcscpy(bnpTmp.FileNames + nSize, it->first.c_str());
			
			bnpTmp.Files[nNb].Name = bnpTmp.FileNames + nSize;
			bnpTmp.Files[nNb].Size = it->second.Size;
			bnpTmp.Files[nNb].Pos = it->second.Pos;

			nSize += it->first.size() + 1;
			nNb ++;
			it ++;
		}
	}
	// End of temp map conversion

	if (nOptions & BF_CACHE_FILE_ON_OPEN)
		bnpTmp.CacheFileOnOpen = true;
	else
		bnpTmp.CacheFileOnOpen = false;

	if (!(nOptions & BF_ALWAYS_OPENED))
	{
		fclose (handle.File);		
		handle.File = NULL;
		bnpTmp.AlwaysOpened = false;
	}
	else
	{
		bnpTmp.AlwaysOpened = true;
	}

	_BNPs.insert (make_pair(toLower(bigfilenamealone), bnpTmp));

	return true;
}

// ***************************************************************************
void CBigFile::remove (const std::string &sBigFileName)
{
	removeW(SIPBASE::getUcsFromMbs(sBigFileName.c_str()));
}

void CBigFile::removeW (const std::basic_string<ucchar> &sBigFileName)
{
	if (_BNPs.find (sBigFileName) != _BNPs.end())
	{
		map<basic_string<ucchar>, BNP>::iterator it = _BNPs.find (sBigFileName);
		BNP &rbnp = it->second;
		/* \todo yoyo: THERE is a MAJOR drawback here: Only the FILE * of the current (surely main) thread
			is closed. other FILE* in other threads are still opened. This is not a big issue (system close the FILE* 
			at the end of the process) and this is important so AsyncLoading of a currentTask can end up correclty 
			(without an intermediate fclose()).
		*/
		// Get a ThreadSafe handle on the file
		CHandleFile		&handle= _ThreadFileArray.get(rbnp.ThreadFileId);
		// close it if needed
		if (handle.File != NULL)
		{
			fclose (handle.File);
			handle.File= NULL;
		}
		/* \todo trap : can make the CPath crash. CPath must be informed that the files in bigfiles have been removed
			this is because CPath use memory of CBigFile if it runs in memoryCompressed mode */
		delete [] rbnp.FileNames;
		_BNPs.erase (it);
	}
}

// ***************************************************************************
bool CBigFile::isBigFileAdded(const std::string &sBigFileName)
{
	return isBigFileAddedW(SIPBASE::getUcsFromMbs(sBigFileName.c_str()));
}

bool CBigFile::isBigFileAddedW(const std::basic_string<ucchar> &sBigFileName)
{
	// Is already the same bigfile name ?
	basic_string<ucchar> bigfilenamealone = CFileW::getFilename (sBigFileName);
	return _BNPs.find(bigfilenamealone) != _BNPs.end();
}

// ***************************************************************************
void CBigFile::list (const std::string &sBigFileName, std::vector<std::string> &vAllFiles)
{
	basic_string<ucchar> lwrFileName = toLower(getUcsFromMbs(sBigFileName.c_str()));

	if (_BNPs.find (lwrFileName) == _BNPs.end())
		return;
	
	vAllFiles.clear ();
	BNP &rbnp = _BNPs.find (lwrFileName)->second;
	vector<BNPFile>::iterator it = rbnp.Files.begin();
	while (it != rbnp.Files.end())
	{
		vAllFiles.push_back (toString("%S", it->Name)); // Add the name of the file to the return vector
		++it;
	}
}

void CBigFile::listW(const std::basic_string<ucchar> &sBigFileName, std::vector<std::basic_string<ucchar> > &vAllFiles)
{
	basic_string<ucchar> lwrFileName = toLower(sBigFileName);
	if (_BNPs.find (lwrFileName) == _BNPs.end())
		return;
	
	vAllFiles.clear ();
	BNP &rbnp = _BNPs.find (lwrFileName)->second;
	vector<BNPFile>::iterator it = rbnp.Files.begin();
	while (it != rbnp.Files.end())
	{
		vAllFiles.push_back (basic_string<ucchar>(it->Name)); // Add the name of the file to the return vector
		++it;
	}
}

// ***************************************************************************
void CBigFile::removeAll ()
{
	while (_BNPs.begin() != _BNPs.end())
	{
		removeW (_BNPs.begin()->first);
	}
}

// ***************************************************************************
bool CBigFile::getFileInternal (const std::string &sFileName, BNP *&zeBnp, BNPFile *&zeBnpFile)
{
	return getFileInternalW(getUcsFromMbs(sFileName.c_str()), zeBnp, zeBnpFile);
}

bool CBigFile::getFileInternalW(const std::basic_string<ucchar> &sFileName, BNP *&zeBnp, BNPFile *&zeBnpFile)
{
	basic_string<ucchar> zeFileName, zeBigFileName, lwrFileName = toLower(sFileName);
	uint32 i, nPos = sFileName.find (L'@');
	if (nPos == string::npos)
	{
		return false;
	}
	
	for (i = 0; i < nPos; ++i)
		zeBigFileName += lwrFileName[i];
	++i; // Skip @
	for (; i < lwrFileName.size(); ++i)
		zeFileName += lwrFileName[i];
	
	if (_BNPs.find (zeBigFileName) == _BNPs.end())
	{
		return false;
	}
	
	BNP &rbnp = _BNPs.find (zeBigFileName)->second;
	if (rbnp.Files.size() == 0)
	{
		return false;
	}
	
	vector<BNPFile>::iterator itNBPFile;

	BNPFile temp_bnp_file;
	temp_bnp_file.Name = (ucchar*)zeFileName.c_str();
	itNBPFile = lower_bound(rbnp.Files.begin(), rbnp.Files.end(), temp_bnp_file, CBNPFileComp());
	
	if (itNBPFile != rbnp.Files.end())
	{
		if (wcscmp(itNBPFile->Name, zeFileName.c_str()) != 0)
		{
			return false;
		}
	}
	else
	{
		return false;
	}
	
	BNPFile &rbnpfile = *itNBPFile;
	
	// set ptr on found bnp/bnpFile
	zeBnp= &rbnp;
	zeBnpFile= &rbnpfile;
	
	return true;
}

// ***************************************************************************
FILE* CBigFile::getFile (const std::string &sFileName, uint32 &rFileSize, 
						 uint32 &rBigFileOffset, bool &rCacheFileOnOpen, bool &rAlwaysOpened)
{
	return getFileW(getUcsFromMbs(sFileName.c_str()), rFileSize, rBigFileOffset, rCacheFileOnOpen, rAlwaysOpened);
}

FILE* CBigFile::getFileW(const std::basic_string<ucchar> &sFileName, uint32 &rFileSize, 
						 uint32 &rBigFileOffset, bool &rCacheFileOnOpen, bool &rAlwaysOpened)
{
	BNP		*bnp= NULL;
	BNPFile	*bnpFile= NULL;
	if (!getFileInternalW(sFileName, bnp, bnpFile))
	{
		sipwarning (L"BF: Couldn't load '%s'", sFileName.c_str());
		return NULL;
	}
	sipassert(bnp && bnpFile);
	
	// Get a ThreadSafe handle on the file
	CHandleFile &handle= _ThreadFileArray.get(bnp->ThreadFileId);
	/* If not opened, open it now. There is 2 reason for it to be not opened: 
		rbnp.AlwaysOpened==false, or it is a new thread which use it for the first time.
	*/
	if (handle.File== NULL)
	{
		handle.File = sfWfopen (bnp->BigFileName.c_str(), L"rb");
		if (handle.File == NULL)
			return NULL;		
	}

	rCacheFileOnOpen = bnp->CacheFileOnOpen;
	rAlwaysOpened = bnp->AlwaysOpened;
	rBigFileOffset = bnpFile->Pos;
	rFileSize = bnpFile->Size;
	return handle.File;
}

// ***************************************************************************
bool CBigFile::getFileInfo (const std::string &sFileName, uint32 &rFileSize, uint32 &rBigFileOffset)
{
	return getFileInfoW(getUcsFromMbs(sFileName.c_str()), rFileSize, rBigFileOffset);
}

bool CBigFile::getFileInfoW(const std::basic_string<ucchar> &sFileName, uint32 &rFileSize, uint32 &rBigFileOffset)
{
	BNP		*bnp= NULL;
	BNPFile	*bnpFile= NULL;
	if (!getFileInternalW(sFileName, bnp, bnpFile))
	{
		sipwarning (L"BF: Couldn't find '%s' for info", sFileName.c_str());
		return false;
	}
	sipassert (bnp && bnpFile);
	
	// get infos
	rBigFileOffset = bnpFile->Pos;
	rFileSize = bnpFile->Size;
	return true;
}

// ***************************************************************************
ucchar *CBigFile::getFileNamePtr(const std::string &sFileName, const std::string &sBigFileName)
{
	return getFileNamePtrW (getUcsFromMbs(sFileName.c_str()), getUcsFromMbs(sBigFileName.c_str()));
}

ucchar *CBigFile::getFileNamePtrW(const std::basic_string<ucchar> &sFileName, const std::basic_string<ucchar> &sBigFileName)
{
	basic_string<ucchar> bigfilenamealone = CFileW::getFilename (sBigFileName);
	if (_BNPs.find(bigfilenamealone) != _BNPs.end())
	{
		BNP &rbnp = _BNPs.find (bigfilenamealone)->second;
		vector<BNPFile>::iterator itNBPFile;
		if (rbnp.Files.size() == 0)
			return NULL;
		basic_string<ucchar> lwrFileName = toLower(sFileName);
		// Debug : Sept 01 2006
		#if _STLPORT_VERSION >= 0x510
			BNPFile temp_bnp_file;
			temp_bnp_file.Name = (ucchar*)lwrFileName.c_str();
			itNBPFile = lower_bound(rbnp.Files.begin(), rbnp.Files.end(), temp_bnp_file, CBNPFileComp());
		#else
			itNBPFile = lower_bound(rbnp.Files.begin(), rbnp.Files.end(), lwrFileName.c_str(), CBNPFileComp());
		#endif //_STLPORT_VERSION
	
		if (itNBPFile != rbnp.Files.end())
		{
			if (wcscmp(itNBPFile->Name, lwrFileName.c_str()) == 0)
			{
				return itNBPFile->Name;
			}
		}
	}

	return NULL;
}

// ***************************************************************************
void CBigFile::getBigFilePaths(std::vector<std::string> &bigFilePaths)
{
	bigFilePaths.clear();
	for(std::map<std::basic_string<ucchar>, BNP>::iterator it = _BNPs.begin(); it != _BNPs.end(); ++it)
	{
		bigFilePaths.push_back(SIPBASE::toString("%S", it->second.BigFileName.c_str()));
	}
}

void CBigFile::getBigFilePathsW(std::vector<std::basic_string<ucchar> > &bigFilePaths)
{
	bigFilePaths.clear();
	for(std::map<std::basic_string<ucchar>, BNP>::iterator it = _BNPs.begin(); it != _BNPs.end(); ++it)
	{
		bigFilePaths.push_back(it->second.BigFileName);
	}
}

} // namespace SIPBASE
