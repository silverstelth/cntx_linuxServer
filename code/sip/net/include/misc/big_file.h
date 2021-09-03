/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __BIG_FILE_H__
#define __BIG_FILE_H__

#include "types_sip.h"
#include "tds.h"

namespace SIPBASE {

/**
 * Big file management
 * \date 2002
 */

const uint32 BF_ALWAYS_OPENED		=	0x00000001;
const uint32 BF_CACHE_FILE_ON_OPEN	=	0x00000002;

// ***************************************************************************
class CBigFile
{
	SIPBASE_SAFE_SINGLETON_DECL(CBigFile);

	CBigFile() {}
	~CBigFile() {};

public:
	// release memory
	static void releaseInstance();

	// Retrieve the global instance
//	static CBigFile &getInstance ();

	// Add a big file to the manager
	bool add (const std::string &sBigFileName, uint32 nOptions);
	bool addW(const std::basic_string<ucchar> &sBigFileName, uint32 nOptions);

	// get path of all added bigfiles
	void getBigFilePaths (std::vector<std::string> &bigFilePaths);
	void getBigFilePathsW(std::vector<std::basic_string<ucchar> > &bigFilePaths);

	// Remove a big file from the manager
	void remove (const std::string &sBigFileName);
	void removeW(const std::basic_string<ucchar> &sBigFileName);

	// true if a bigFile is added
	bool isBigFileAdded (const std::string &sBigFileName);
	bool isBigFileAddedW(const std::basic_string<ucchar> &sBigFileName);

	// List all files in a bigfile
	void list (const std::string &sBigFileName, std::vector<std::string> &vAllFiles);
	void listW(const std::basic_string<ucchar> &sBigFileName, std::vector<std::basic_string<ucchar> > &vAllFiles);

	// Remove all big files added
	void removeAll ();
	
	/** Signal that the current thread has ended : all file handles "permanently" allocated for that thread
	  * can be released then, preventing them from accumulating.	  
	  */
	void currentThreadFinished();

	// Used by CIFile to get information about the files within the big file
	FILE* getFile (const std::string &sFileName, uint32 &rFileSize, uint32 &rBigFileOffset, 
					bool &rCacheFileOnOpen, bool &rAlwaysOpened);

	FILE* getFileW(const std::basic_string<ucchar> &sFileName, uint32 &rFileSize, uint32 &rBigFileOffset, 
					bool &rCacheFileOnOpen, bool &rAlwaysOpened);

	// Used by Sound to get information for async loading of mp3 in .bnp. return false if file not found in registered bnps
	bool getFileInfo (const std::string &sFileName, uint32 &rFileSize, uint32 &rBigFileOffset);
	bool getFileInfoW(const std::basic_string<ucchar> &sFileName, uint32 &rFileSize, uint32 &rBigFileOffset);
	
	// Used for CPath only for the moment !
	ucchar *getFileNamePtr (const std::string &sFileName, const std::string &sBigFileName);
	ucchar *getFileNamePtrW(const std::basic_string<ucchar> &sFileName, const std::basic_string<ucchar> &sBigFileName);
	
// ***************
private:
	class	CThreadFileArray;
	friend class	CThreadFileArray;

	// A ptr to a file.
	struct	CHandleFile
	{
		FILE		*File;
		CHandleFile()
		{
			File= NULL;
		}
	};

	// A class which return a FILE * handle per Thread.
	class	CThreadFileArray
	{
	public:
		CThreadFileArray();

		// Allocate a FileId for a BNP.
		uint32			allocate();
		// Given a BNP File Id, return its FILE* handle for the current thread.
		CHandleFile		&get(uint32 index);

		void currentThreadFinished();

	private:
		// Do it this way because a few limited TDS is possible (64 on NT4)
		CTDS		_TDS;
		// The array is grow only!!
		uint32		_CurrentId;
	};

	// A BNPFile header
	struct BNPFile
	{
		ucchar		*Name;
		uint32		Size;
		uint32		Pos;
	};

	class CBNPFileComp
	{
	public:
		bool operator()(const BNPFile &f, const BNPFile &s )
		{
			return wcscmp( f.Name, s.Name ) < 0;
		}
		bool operator()(const BNPFile &f, const ucchar *s)
		{
			return wcscmp(f.Name, s) < 0;
		}
		bool operator()(const ucchar *s, const BNPFile &f)
		{
			return wcscmp(s, f.Name) < 0;
		}
	};

	// A BNP structure
	struct BNP
	{
		// FileName of the BNP. important to open it in getFile() (for other threads or if not always opened).
		std::basic_string<ucchar>		BigFileName;
		// map of files in the BNP.
		ucchar							*FileNames;
		std::vector<BNPFile>			Files;
		// Since many seek may be done on a FILE*, each thread should have its own FILE opened.
		uint32							ThreadFileId;
		bool							CacheFileOnOpen;
		bool							AlwaysOpened;

		BNP()
		{
			FileNames = NULL;
		}
	};
private:

//	CBigFile(); // Singleton mode -> access it with the getInstance function

//	static CBigFile				*_Singleton;

	// This is an array of CHandleFile, unique to each thread
	CThreadFileArray			_ThreadFileArray;

	std::map<std::basic_string<ucchar>, BNP> _BNPs;

	// common for getFile and getFileInfo
	bool getFileInternal (const std::string &sFileName, BNP *&zeBnp, BNPFile *&zeBnpFile);
	bool getFileInternalW(const std::basic_string<ucchar> &sFileName, BNP *&zeBnp, BNPFile *&zeBnpFile);
};

} // SIPBASE


#endif // __BIG_FILE_H__

/* End of big_file.h */
