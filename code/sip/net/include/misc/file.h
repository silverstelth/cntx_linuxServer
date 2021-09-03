/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __FILE_H__
#define __FILE_H__

#include "stream.h"



namespace SIPBASE
{

// ======================================================================================================
/**
 * File Exception.
 * \date 2000
 */
struct EFile : public EStream
{
public:
	EFile () : EStream( L"Unknown file error" ) {}
	EFile (const std::string& filename) : EStream( "Unknown file error in '"+filename+"'" ) {}
	EFile (const std::basic_string<ucchar>& filename) : EStream( L"Unknown file error in '"+ filename + L"'" ) {}
	EFile (const std::string& text, bool ) : EStream( text ) {}
	EFile (const std::basic_string<ucchar>& text, bool ) : EStream( text ) {}
};

struct EFileNotOpened : public EFile
{
	EFileNotOpened( const std::string& filename ) : EFile( "File '"+filename+"' not opened", true ) {}
	EFileNotOpened( const std::basic_string<ucchar>& filename ) : EFile( L"File '"+ filename+ L"' not opened", true ) {}
};

struct EReadError : public EFile
{
	EReadError( const std::string& filename ) : EFile( "Read error in file '" +filename+"' (End of file?)", true ) {}
	EReadError( const std::basic_string<ucchar>& filename ) : EFile( L"Read error in file '" + filename + L"' (End of file?)", true ) {}
};

struct EWriteError : public EFile
{
	EWriteError( const std::string& filename ) : EFile( "Write Error in file '" +filename+"'", true ) {}
	EWriteError( const std::basic_string<ucchar>& filename ) : EFile( L"Write Error in file '" + filename + L"'", true ) {}
};

struct ERenameError : public EFile
{
	ERenameError( const std::string& dest, const std::string& src ) : EFile( "Rename Error from the file '" +src+"' to the file '" +dest+"'", true ) {}
	ERenameError( const std::basic_string<ucchar>& dest, const std::basic_string<ucchar>& src ) : EFile( L"Rename Error from the file '" + src + L"' to the file '" + dest + L"'", true ) {}
};


// ======================================================================================================
/**
 * Input File.
 * \date 2000
 * ***********************************************
 *	WARNING: This Class/Method must be thread-safe (ctor/dtor/serial): no static access for instance
 *	It can be loaded/called through CAsyncFileManager for instance
 * ***********************************************
 */
class CIFile : public IStream
{
public:		// Basic Usage.
	/// Object. NB: destructor close() the stream.
	CIFile();
	CIFile(const std::string &path, bool text=false);
	CIFile(const std::basic_string<ucchar> &path, bool text = false);
	~CIFile();

	/// Open a file for reading. false if failed. close() if a file was opened.
	bool	open (const std::string &path, bool text=false);
	bool	open (const std::basic_string<ucchar> &path, bool text=false);

	/// Set the cache file on open option (default behaviour is false (file is not cached at opening)
	void	setCacheFileOnOpen (bool newState);

	/** If the file is opened with a big file, CacheFileOnOpen is replaced with big file option. Except if
	 *	allowBNPCacheFileOnOpen(false) is called. true is default.
	 */
	void	allowBNPCacheFileOnOpen(bool newState);

	/// Set the async loading state (to go to sleep 5 ms after 100 Ko serialized)
	void	setAsyncLoading (bool newState);

public:		// Advanced Usage.
	/// Explicit close.
	void	close();
	/// flush the file.
	void	flush();
	/// Seek the file
	bool	seek (sint32 offset, IStream::TSeekOrigin origin) const throw(EStream);
	/// Gt the location of the file pointer
	sint32	getPos () const throw(EStream);

	// Imp the Name of the stream as the name of the file.
	virtual std::string getStreamNameA() const;
	virtual std::basic_string<ucchar> getStreamNameW() const;

	// same function that in ifstream
	// return a string separated by \n or eof, used to parsing text file
	void getline (char *buffer, uint32 bufferSize);
	void getline (ucchar *buffer, uint32 bufferSize);

	// return the size of the file
	uint32 getFileSize () const { return _FileSize; }

	// return true if there s nothing more to read (same as ifstream)
	bool eof ();

	virtual void		serialBuffer(uint8 *buf, uint len)throw(EReadError);

	/// \name Statistics
	
	/// Get the number of file open from the begining of the application. Files can be in a big file.
	static uint32	getNumFileOpen() {return _FileOpened;}
	
	/// Get the number of read acces to a file.
	static uint32	getNumFileRead() {return _FileRead;}
	
	/// Get the number of byte read from the file system since the application start.
	static uint32	getReadFromFile() {return _ReadFromFile;}
	
	/// Get the number of byte being reading from the file system at the moment.
	static uint32	getReadingFromFile() {return _ReadingFromFile;}
	
	/// Get the last 40 files opened. The files can be in a big file.
	static void		dump (std::vector<std::string> &result);
	static void		dump (std::vector<std::basic_string<ucchar> > &result);
	
	/// clear the dump of the last 40 files opened
	static void		clearDump ();

protected:
	virtual void		serialBit(bool &bit) throw(EReadError);

	virtual uint		getDbgStreamSize() const;
	
	
private:
	FILE		*_F;
	std::basic_string<ucchar> _FileName;

	// Async
	static uint32 _NbBytesSerialized;
	static uint32 _NbBytesLoaded;

	// Stats
	static uint32 _FileOpened;
	static uint32 _FileRead;
	static uint32 _ReadFromFile;
	static uint32 _ReadingFromFile;
	static CSynchronized<std::deque<std::basic_string<ucchar> > > _OpenedFiles;
	
	bool _IsAsyncLoading;

	// Cache 
	bool			_CacheFileOnOpen;
	bool			_AllowBNPCacheFileOnOpen;
	uint8			*_Cache;
	mutable sint32	_ReadPos;
	uint32			_FileSize;

	// Big file
	bool	_AlwaysOpened;
	bool	_IsInBigFile;
	uint32	_BigFileOffset;

	// Load async if needed in the cache.
	void	loadIntoCache();
};


// ======================================================================================================
/**
 * Output File.
 * \date 2000
 */
class COFile : public IStream
{
public:		// Basic Usage.
	/// Object. NB: destructor close() the stream.
	COFile();
	COFile(const std::string &path, bool append=false, bool text=false, bool useTempFile=false);
	COFile(const std::basic_string<ucchar> &path, bool append=false, bool text=false, bool useTempFile=false);
	~COFile();

	/** Open a file for writing. false if failed. close() if a file was opened.
	*	If you open the file with the flag useTempFile, you MUST close explicitlty the file
	*	with close() if the writing as been successed.
	*/
	bool open(const std::string &path, bool append=false, bool text=false, bool useTempFile=false);
	bool open(const std::basic_string<ucchar> &path, bool append=false, bool text=false, bool useTempFile=false);

	bool	isOpen	()	const
	{
		return	_F != NULL;
	}
	
public:		// Advanced Usage.
	/// Explicit close.
	void	close();
	/// flush the file.
	void	flush();
	/// Seek the file
	bool	seek (sint32 offset, IStream::TSeekOrigin origin) const throw(EStream);
	/// Gt the location of the file pointer
	sint32	getPos () const throw(EStream);

	// Imp the Name of the stream as the name of the file.
	virtual std::string getStreamNameA() const;
	virtual std::basic_string<ucchar> getStreamNameW() const;

	// very useful to serialize string in text mode (without the size)
	virtual void		serialBuffer(uint8 *buf, uint len) throw(EWriteError);

protected:
	/// Internal close.
	void	internalClose(bool success);
	virtual void		serialBit(bool &bit) throw(EWriteError);

private:
	FILE	*_F;
	std::basic_string<ucchar> _FileName;
	std::basic_string<ucchar> _TempFileName;
};

}


#endif // __FILE_H__

/* End of file.h */
