/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __DISPLAYER_H__
#define __DISPLAYER_H__

#include "types_sip.h"

#include <string>

#include "log.h"

namespace SIPBASE
{

class CMutex;

/**
 * Displayer interface. Used to specialize a displayer to display a string.
 * \ref log_howto
 * \date 2000
 */

class IDisplayer_base
{
public:

	/// Constructor
	IDisplayer_base(const char *displayerName = "");
	
	/// Destructor
	virtual ~IDisplayer_base();

	/// Display the string where it does.
	virtual void display (const CLogA::TDisplayInfo& args, const char *message) = 0;
	virtual void display (const CLogW::TDisplayInfo& args, const ucchar *message) = 0;

	/// This is the identifier for a displayer, it is used to find or remove a displayer
	std::basic_string<char>	DisplayerName;

protected:

	CMutex	*_Mutex;
};

// ANSI version
class IDisplayerA : public IDisplayer_base
{
public:

	/// Constructor
	IDisplayerA(const char *displayerName = "") : IDisplayer_base(displayerName) { };
	
	/// Display the string where it does.
	virtual void display (const CLogA::TDisplayInfo& args, const char *message);
	virtual void display (const CLogW::TDisplayInfo& args, const ucchar *message) { };

protected:

	/// Method to implement in the derived class

	virtual void doDisplay ( const CLogA::TDisplayInfo& args, const char *message) = 0;
	
	// Return the header string with date (for the first line of the log)
	static const char *HeaderString ();

public:

	/// Convert log type to string
	static const char *logTypeToString (ILog::TLogType logType, bool longFormat = false);

	/// Convert the current date to human string
	static const char *dateToHumanString ();

	/// Convert date to "2000/01/14 10:05:17" string
	static const char *dateToHumanString (time_t date);

	/// Convert date to "784551148" string (time in second from 1975)
	static const char *dateToComputerString (time_t date);
};

// unicode version
class IDisplayerW : public IDisplayer_base
{
public:

	/// Constructor
	IDisplayerW(const char *displayerName = "") : IDisplayer_base(displayerName) { };
	
	/// Display the string where it does.
	virtual void display (const CLogA::TDisplayInfo& args, const char *message) { };
	virtual void display (const CLogW::TDisplayInfo& args, const ucchar *message);

protected:

	/// Method to implement in the derived class

	virtual void doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message) = 0;
	
	// Return the header string with date (for the first line of the log)
	static const ucchar *HeaderString ();

public:

	/// Convert log type to string
	static const ucchar *logTypeToString (ILog::TLogType logType, bool longFormat = false);

	/// Convert the current date to human string
	static const ucchar *dateToHumanString ();

	/// Convert date to "2000/01/14 10:05:17" string
	static const ucchar *dateToHumanString (time_t date);

	/// Convert date to "784551148" string (time in second from 1975)
	static const ucchar *dateToComputerString (time_t date);
};

/**
 * Std displayer. Put string to stdout.
 * \ref log_howto
 * \date 2000
 */

class CStdDisplayerA : virtual public IDisplayerA
{
public:
	CStdDisplayerA (const char *displayerName = "") : IDisplayerA(displayerName) {}

protected:

	/// Display the string to stdout and OutputDebugString on Windows
	virtual void doDisplay ( const CLogA::TDisplayInfo& args, const char *message );
};

class CStdDisplayerW : virtual public IDisplayerW
{
public:
	CStdDisplayerW (const char *displayerName = "") : IDisplayerW (displayerName) {}

protected:

	/// Display the string to stdout and OutputDebugString on Windows
	virtual void doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message );
};

/**
 * File displayer. Put string into a file.
 * \ref log_howto
 * \date 2000
 */
class CFileDisplayerA : virtual public IDisplayerA
{
public:

	/// Constructor
	CFileDisplayerA (const std::string &filename, bool eraseLastLog = false, const char *displayerName = "", bool raw = false);
	CFileDisplayerA (bool raw = false);

	~CFileDisplayerA ();

	/// Set Parameter of the displayer if not set at the ctor time
	void setParam (const std::string &filename, bool eraseLastLog = false);

protected:
	/// Put the string into the file.
	virtual void doDisplay ( const CLogA::TDisplayInfo& args, const char *message );

private:
	std::string	_FileName;

	FILE		*_FilePointer;

	bool		_NeedHeader;

	uint		_LastLogSizeChecked;

	bool		_Raw;
};

class CFileDisplayerW : virtual public IDisplayerW
{
public:

	/// Constructor
	CFileDisplayerW (const std::basic_string<ucchar> &filename, bool eraseLastLog = false, const char *displayerName = "", bool raw = false);
	CFileDisplayerW (const std::string &filename, bool eraseLastLog = false, const char *displayerName = "", bool raw = false);
	CFileDisplayerW (bool raw = false);

	~CFileDisplayerW ();

	/// Set Parameter of the displayer if not set at the ctor time
	void setParam (const std::basic_string<ucchar> &filename, bool eraseLastLog = false);
	void setParam (const std::basic_string<char> &filename, bool eraseLastLog = false);

protected:
	/// Put the string into the file.
	virtual void doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message );

private:
	std::basic_string<ucchar>	_FileName;

	FILE		*_FilePointer;

	bool		_NeedHeader;

	uint		_LastLogSizeChecked;

	bool		_Raw;
};

/**
 * Message Box displayer. Put string into a message box.
 * \ref log_howto
 * \date 2000
 */
class CMsgBoxDisplayerA : virtual public IDisplayerA
{
public:
	CMsgBoxDisplayerA (const char *displayerName = "") : IDisplayerA (displayerName), IgnoreNextTime(false) {}

	bool IgnoreNextTime;

protected:
	/// Put the string into the file.
	virtual void doDisplay ( const CLogA::TDisplayInfo& args, const char *message );
};

class CMsgBoxDisplayerW : virtual public IDisplayerW
{
public:
	CMsgBoxDisplayerW (const char *displayerName = "") : IDisplayerW (displayerName), IgnoreNextTime(false) {}

	bool IgnoreNextTime;

protected:
	/// Put the string into the file.
	virtual void doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message );
};

};

#endif // __DISPLAYER_H__

/* End of displayer.h */
