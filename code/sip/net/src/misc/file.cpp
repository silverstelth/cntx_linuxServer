/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/file.h"
#include "misc/debug.h"
#include "misc/big_file.h"
#include "misc/path.h"

using namespace std;

#define SIPBASE_DONE_FILE_OPENED 40

namespace SIPBASE
{

uint32 CIFile::_NbBytesSerialized = 0;
uint32 CIFile::_NbBytesLoaded = 0;
uint32 CIFile::_ReadFromFile = 0;
uint32 CIFile::_ReadingFromFile = 0;
uint32 CIFile::_FileOpened = 0;
uint32 CIFile::_FileRead = 0;
CSynchronized<std::deque<std::basic_string<ucchar> > > CIFile::_OpenedFiles("");// = CSynchronized<std::deque<std::basic_string<ucchar> > >("");

// ======================================================================================================
CIFile::CIFile() : IStream(true)
{
	_F = NULL;
	_Cache = NULL;
	_ReadPos = 0;
	_FileSize = 0;
	_BigFileOffset = 0;
	_IsInBigFile = false;
	_CacheFileOnOpen = false;
	_IsAsyncLoading = false;
	_AllowBNPCacheFileOnOpen= true;
}

// ======================================================================================================
CIFile::CIFile(const std::string &path, bool text) : IStream(true)
{
	_F=NULL;
	_Cache = NULL;
	_ReadPos = 0;
	_FileSize = 0;
	_BigFileOffset = 0;
	_IsInBigFile = false;
	_CacheFileOnOpen = false;
	_IsAsyncLoading = false;
	_AllowBNPCacheFileOnOpen= true;
	open(path, text);
}

CIFile::CIFile(const std::basic_string<ucchar> &path, bool text) : IStream(true)
{
	_F=NULL;
	_Cache = NULL;
	_ReadPos = 0;
	_FileSize = 0;
	_BigFileOffset = 0;
	_IsInBigFile = false;
	_CacheFileOnOpen = false;
	_IsAsyncLoading = false;
	_AllowBNPCacheFileOnOpen= true;
	open(path, text);
}

// ======================================================================================================
CIFile::~CIFile()
{
	close();
}


// ======================================================================================================
void		CIFile::loadIntoCache()
{
	const uint32 READPACKETSIZE = 64 * 1024;
	const uint32 INTERPACKETSLEEP = 5;

	_Cache = new uint8[_FileSize];
	if(!_IsAsyncLoading)
	{
		_ReadingFromFile += _FileSize;
		int read = fread (_Cache, _FileSize, 1, _F);
		_FileRead++;
		_ReadingFromFile -= _FileSize;
		_ReadFromFile += read * _FileSize;
	}
	else
	{
		uint	index= 0;
		while(index<_FileSize)
		{
			if( _NbBytesLoaded + (_FileSize-index) > READPACKETSIZE )
			{
				sint	n= READPACKETSIZE-_NbBytesLoaded;
				n= max(n, 1);
				_ReadingFromFile += n;
				int read = fread (_Cache+index, n, 1, _F);
				_FileRead++;
				_ReadingFromFile -= n;
				_ReadFromFile += read * n;
				index+= n;

				sipSleep (INTERPACKETSLEEP);
				_NbBytesLoaded= 0;
			}
			else
			{
				uint	n= _FileSize-index;
				_ReadingFromFile += n;
				int read = fread (_Cache+index, n, 1, _F);
				_FileRead++;
				_ReadingFromFile -= n;
				_ReadFromFile += read * n;
				_NbBytesLoaded+= n;
				index+= n;
			}
		}
	}
}


// ======================================================================================================
bool CIFile::open(const std::string &path, bool text)
{
	bool result = open(getUcsFromMbs(path.c_str()), text);
	return result;
}

bool CIFile::open(const std::basic_string<ucchar> &path, bool text)
{
	// Log opened files
	{
		CSynchronized<deque<basic_string<ucchar> > >::CAccessor fileOpened(&_OpenedFiles);
		fileOpened.value().push_front (path);
		if (fileOpened.value().size () > SIPBASE_DONE_FILE_OPENED)
			fileOpened.value().resize (SIPBASE_DONE_FILE_OPENED);
		_FileOpened++;
	}

	close();

	// can't open empty filename
	if(path.empty ())
		return false;

	ucchar mode[3];
	mode[0] = L'r';
	mode[1] = L'b'; // No more reading in text mode
	mode[2] = L'\0';

	_FileName = path;
	_ReadPos = 0;

	// Bigfile access requested ?
	if (path.find(L'@') != string::npos)
	{
		_IsInBigFile = true;
		if(_AllowBNPCacheFileOnOpen)
		{
			_F = CBigFile::getInstance().getFileW (path, _FileSize, _BigFileOffset, _CacheFileOnOpen, _AlwaysOpened);
		}
		else
		{
			bool	dummy;
			_F = CBigFile::getInstance().getFileW (path, _FileSize, _BigFileOffset, dummy, _AlwaysOpened);
		}
		if(_F != NULL)
		{
			// Start to load the bigfile at the file offset.
			sipfseek64 (_F, _BigFileOffset, SEEK_SET);

			// Load into cache ?
			if (_CacheFileOnOpen)
			{
				// load file in the cache
				loadIntoCache();

				if (!_AlwaysOpened)
				{
					fclose (_F);
					_F = NULL;
				}
				return (_Cache != NULL);
			}
		}
	}
	else
	{
		_IsInBigFile = false;
		_BigFileOffset = 0;
		_AlwaysOpened = false;
		_F = sfWfopen (path.c_str(), mode);
		if (_F != NULL)
		{
			/*
			
			THIS CODE REPLACED BY SADGE BECAUSE SOMETIMES
			ftell() RETRUNS 0 FOR NO GOOD REASON - LEADING TO CLIENT CRASH

			sipfseek64 (_F, 0, SEEK_END);
			_FileSize = ftell(_F);
			sipfseek64 (_F, 0, SEEK_SET);
			sipassert(_FileSize==filelength(fileno(_F)));

			THE FOLLOWING WORKS BUT IS NOT PORTABLE
			_FileSize=filelength(fileno(_F));
			*/
			_FileSize=CFileA::getFileSize (_F);
			if (_FileSize == 0)
			{
				sipwarning (L"FILE: Size of file '%s' is 0", path.c_str());
				fclose (_F);
				_F = NULL;
			}
		}
		else
		{
			_FileSize = 0;
		}
		
		if ((_CacheFileOnOpen) && (_F != NULL))
		{
			// load file in the cache
			loadIntoCache();

			fclose (_F);
			_F = NULL;
			return (_Cache != NULL);
		}
	}

	return (_F != NULL);
}

// ======================================================================================================
void		CIFile::setCacheFileOnOpen (bool newState)
{
	_CacheFileOnOpen = newState;
}

// ======================================================================================================
void		CIFile::setAsyncLoading (bool newState)
{
	_IsAsyncLoading = true;
}


// ======================================================================================================
void		CIFile::close()
{
	if (_CacheFileOnOpen)
	{
		if (_Cache)
		{
			delete[] _Cache;
			_Cache = NULL;
		}
	}
	else
	{
		if (_IsInBigFile)
		{
			if (!_AlwaysOpened)
			{
				if (_F)
				{
					fclose (_F);
					_F = NULL;
				}
			}
		}
		else
		{
			if (_F)
			{
				fclose (_F);
				_F = NULL;
			}
		}
	}
	resetPtrTable();
}

// ======================================================================================================
void		CIFile::flush()
{
	if (_CacheFileOnOpen)
	{
	}
	else
	{
		if (_F)
		{
			fflush (_F);
		}
	}
}

// ======================================================================================================
void		CIFile::getline (char *buffer, uint32 bufferSize)
{
	if (bufferSize == 0)
		return;

	uint read = 0;
	while (true)
	{
		if (read == bufferSize - 1)
		{
			*buffer = '\0';
			return;
		}

		try
		{
			// read one byte
			serialBuffer ((uint8 *)buffer, 1);
		}
		catch (EFile &)
		{
			*buffer = '\0';
			return;
		}

		if (*buffer == '\n')
		{
			*buffer = '\0';
			return;
		}

		// skip '\r' char
		if (*buffer != '\r')
		{
			buffer++;
			read++;
		}
	}
}

void CIFile::getline(ucchar *buffer, uint32 bufferSize)
{
	if (bufferSize == 0)
		return;

	uint read = 0;
	while (true)
	{
		if (read == bufferSize - 1)
		{
			*buffer = L'\0';
			return;
		}

		try
		{
			// read one byte
			serialBuffer ((uint8 *)buffer, sizeof(ucchar));
		}
		catch (EFile &)
		{
			*buffer = L'\0';
			return;
		}

		if (*buffer == L'\n')
		{
			*buffer = L'\0';
			return;
		}

		// skip '\r' char
		if (*buffer != L'\r')
		{
			buffer ++;
			read ++;
		}
	}

}

// ======================================================================================================
bool		CIFile::eof ()
{
	return _ReadPos >= (sint32)_FileSize;
}

// ======================================================================================================
void		CIFile::serialBuffer(uint8 *buf, uint len) throw(EReadError)
{
	// Check the read pos
	if ((_ReadPos < 0) || ((_ReadPos+len) > _FileSize))
		throw EReadError (_FileName);
	if ((_CacheFileOnOpen) && (_Cache == NULL))
		throw EFileNotOpened (_FileName);
	if ((!_CacheFileOnOpen) && (_F == NULL))
		throw EFileNotOpened (_FileName);

	if (_IsAsyncLoading)
	{
		_NbBytesSerialized += len;
		if (_NbBytesSerialized > 64 * 1024)
		{
			sipSleep (5);
			_NbBytesSerialized = 0;
		}
	}

	if (_CacheFileOnOpen)
	{
		memcpy (buf, _Cache + _ReadPos, len);
		_ReadPos += len;
	}
	else
	{
		int read;
		_ReadingFromFile += len;
		read=fread(buf, 1, len, _F);
		_FileRead++;
		_ReadingFromFile -= len;
		_ReadFromFile += read * 1;
		if (read < (int)len)
			throw EReadError(_FileName);
		_ReadPos += len;
	}
}

// ======================================================================================================
void		CIFile::serialBit(bool &bit) throw(EReadError)
{
	// Simple for now.
	uint8	v=bit;
	serialBuffer(&v, 1);
	bit=(v!=0);
}

// ======================================================================================================
bool		CIFile::seek (sint32 offset, IStream::TSeekOrigin origin) const throw(EStream)
{
	if ((_CacheFileOnOpen) && (_Cache == NULL))
		return false;
	if ((!_CacheFileOnOpen) && (_F == NULL))
		return false;

	switch (origin)
	{
		case IStream::begin:
			_ReadPos = offset;
		break;
		case IStream::current:
			_ReadPos = _ReadPos + offset;
		break;
		case IStream::end:
			_ReadPos = _FileSize + offset; 
		break;
		default:
			sipstop;
	}

	if (_CacheFileOnOpen)
		return true;

	// seek in the file. NB: if not in bigfile, _BigFileOffset==0.
	if (sipfseek64(_F, _BigFileOffset+_ReadPos, SEEK_SET) != 0)
		return false;
	return true;
}

// ======================================================================================================
sint32		CIFile::getPos () const throw(EStream)
{
	return _ReadPos;
}


// ======================================================================================================
std::string	CIFile::getStreamNameA() const
{
	return toString("%S", _FileName.c_str());
}

std::basic_string<ucchar> CIFile::getStreamNameW() const
{
	return _FileName;
}

// ======================================================================================================
void	CIFile::allowBNPCacheFileOnOpen(bool newState)
{
	_AllowBNPCacheFileOnOpen= newState;
}


// ======================================================================================================
void	CIFile::dump (std::vector<std::string> &result)
{
	CSynchronized<deque<basic_string<ucchar> > >::CAccessor acces(&_OpenedFiles);
	
	const deque<basic_string<ucchar> > &openedFile = acces.value();
	
	// Resize the destination array
	result.clear ();
	result.reserve (openedFile.size ());
	
	// Add the waiting strings
	deque<basic_string<ucchar> >::const_reverse_iterator ite = openedFile.rbegin ();
	while (ite != openedFile.rend ())
	{
		std::string str = SIPBASE::toString("%S", (*ite).c_str());
		result.push_back (str);
		
		// Next task
		ite++;
	}	
}

void	CIFile::dump (std::vector<std::basic_string<ucchar> > &result)
{
	CSynchronized<deque<basic_string<ucchar> > >::CAccessor acces(&_OpenedFiles);
	
	const deque<basic_string<ucchar> > &openedFile = acces.value();
	
	// Resize the destination array
	result.clear ();
	result.reserve (openedFile.size ());
	
	// Add the waiting strings
	deque<basic_string<ucchar> >::const_reverse_iterator ite = openedFile.rbegin ();
	while (ite != openedFile.rend ())
	{
		result.push_back (*ite);
		
		// Next task
		ite++;
	}
}

// ======================================================================================================
void	CIFile::clearDump ()
{
	CSynchronized<deque<basic_string<ucchar> > >::CAccessor acces(&_OpenedFiles);
	acces.value().clear();
}

// ======================================================================================================
uint	CIFile::getDbgStreamSize() const
{
	return getFileSize();
}


// ======================================================================================================
// ======================================================================================================
// ======================================================================================================


// ======================================================================================================
COFile::COFile() : IStream(false)
{
	_F = NULL;
	_FileName = L"";
}

// ======================================================================================================
COFile::COFile(const std::string &path, bool append, bool text, bool useTempFile) : IStream(false)
{
	_F = NULL;
	open(path, append, text, useTempFile);
}

COFile::COFile(const std::basic_string<ucchar> &path, bool append, bool text, bool useTempFile) : IStream(false)
{
	_F = NULL;
	open(path, append, text, useTempFile);
}

// ======================================================================================================
COFile::~COFile()
{
	internalClose(false);
}
// ======================================================================================================
bool COFile::open(const std::string &path, bool append, bool text, bool useTempFile)
{
	bool result = open(getUcsFromMbs(path.c_str()), append, text, useTempFile);
	return result;
}

bool COFile::open(const std::basic_string<ucchar> &path, bool append, bool text, bool useTempFile)
{
	close();

	// can't open empty filename
	if (path.empty ())
		return false;

	_FileName = path;
	_TempFileName.clear();

	ucchar mode[3];
	mode[0] = (append) ? L'a': L'w';
// ACE: NEVER SAVE IN TEXT MODE!!!	mode[1] = (text)?'\0':'b';
	mode[1] = L'b';
	mode[2] = L'\0';

	basic_string<ucchar> fileToOpen = path;
	if (useTempFile)
	{
		CFileW::getTemporaryOutputFilename (path, _TempFileName);
		fileToOpen = _TempFileName;
	}

	// if appending to file and using a temporary file, copycat temporary file from original...
	if (append && useTempFile && CFileW::fileExists(_FileName))
	{
		// open fails if can't copy original content
		if (!CFileW::copyFile(_TempFileName.c_str(), _FileName.c_str()))
			return false;
	}

	_F = sfWfopen(fileToOpen.c_str(), mode);

	return _F != NULL;
}

// ======================================================================================================
void	COFile::close()
{
	internalClose(true);
}
// ======================================================================================================
void	COFile::internalClose(bool success)
{
	if(_F)
	{
		fclose(_F);

		// Temporary filename ?
		if (!_TempFileName.empty())
		{
			// Delete old
			if (success)
			{
				if (CFileW::fileExists(_FileName))
					CFileW::deleteFile(_FileName);

				// Bug under windows, sometimes the file is not deleted
				uint retry = 1000;
				while (--retry)
				{
					if (CFileW::moveFile (_FileName.c_str(), _TempFileName.c_str()))
						break;
					sipSleep (0);
				}
				if (!retry)
					throw ERenameError (_FileName, _TempFileName);
			}
			else
				CFileW::deleteFile (_TempFileName);
		}

		_F = NULL;
	}
	resetPtrTable();
}
// ======================================================================================================
void	COFile::flush()
{
	if(_F)
	{
		fflush(_F);
	}
}


// ======================================================================================================
void COFile::serialBuffer(uint8 *buf, uint len) throw(EWriteError)
{
	if(!_F)
		throw	EFileNotOpened(_FileName);
//	if(fwrite(buf, len, 1, _F) != 1)
	if (fwrite(buf, 1, len, _F) != len)
		throw	EWriteError(_FileName);
}
// ======================================================================================================
void COFile::serialBit(bool &bit) throw(EWriteError)
{
	// Simple for now.
	uint8	v=bit;
	serialBuffer(&v, 1);
}
// ======================================================================================================
bool		COFile::seek (sint32 offset, IStream::TSeekOrigin origin) const throw(EStream)
{
	if (_F)
	{
		int origin_c = SEEK_SET;
		switch (origin)
		{
		case IStream::begin:
			origin_c=SEEK_SET;
			break;
		case IStream::current:
			origin_c=SEEK_CUR;
			break;
		case IStream::end:
			origin_c=SEEK_END;
			break;
		default:
			sipstop;
		}

		if (sipfseek64 (_F, offset, origin_c)!=0)
			return false;
		return true;
	}
	return false;
}
// ======================================================================================================
sint32		COFile::getPos () const throw(EStream)
{
	if (_F)
	{
		return ftell (_F);
	}
	return 0;
}

// ======================================================================================================
std::string	COFile::getStreamNameA() const
{
	return toString("%S", _FileName.c_str());
}

std::basic_string<ucchar> COFile::getStreamNameW() const
{
	return _FileName;
}

}
