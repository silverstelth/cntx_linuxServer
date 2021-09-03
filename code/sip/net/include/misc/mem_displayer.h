/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __MEM_DISPLAYER_H__
#define __MEM_DISPLAYER_H__

#include "types_sip.h"
#include "displayer.h"

#include <deque>
#include <string>

namespace SIPBASE {


/**
 * Display into a string vector
 * \date 2001
 */
class CMemDisplayerA : public IDisplayerA
{
public:
	/// Constructor
	CMemDisplayerA (const char *displayerName = "");

	/// Set Parameter of the displayer if not set at the ctor time
	void			setParam (uint32 maxStrings = 50);

	/// Write N last line into a displayer (InfoLog by default)
	void			write (CLogA *log = NULL, bool quiet=true);
	void			write (std::string &str, bool crLf=false);

	const std::deque<std::string>	&lockStrings () { _CanUseStrings = false; return _Strings; }

	void unlockStrings() { _CanUseStrings = true; }

	void clear () { if (_CanUseStrings) _Strings.clear (); }

protected:
	/// Put the string into the file.
    virtual void	doDisplay ( const CLogA::TDisplayInfo& args, const char *message );

	bool						_NeedHeader;

	uint32						_MaxStrings;	// number of string in the _Strings queue (default is 50)
	
	bool						_CanUseStrings;

	std::deque<std::string>		_Strings;

};

class CMemDisplayerW : public IDisplayerW
{
public:
	/// Constructor
	CMemDisplayerW (const char *displayerName = "");
	virtual ~CMemDisplayerW();
	/// Set Parameter of the displayer if not set at the ctor time
	void			setParam (uint32 maxStrings = 50);

	/// Write N last line into a displayer (InfoLog by default)
	void			write (CLogW *log = NULL, bool quiet = true);
	void			write (std::basic_string<ucchar> &str, bool crLf = false);
	void			write (std::string &str, bool crLf = false);

	const std::deque<std::basic_string<ucchar> >	&lockStrings () { _CanUseStrings = false; return _Strings; }

	void unlockStrings() { _CanUseStrings = true; }

	void clear () { if (_CanUseStrings) _Strings.clear (); }

protected:
	/// Put the string into the file.
    virtual void	doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message );

	bool						_NeedHeader;

	uint32						_MaxStrings;	// number of string in the _Strings queue (default is 50)
	
	bool						_CanUseStrings;

	std::deque<std::basic_string<ucchar> >		_Strings;

};

/**
 * Same as CMemDisplayer but only display the text (no line, no date, no process...)
 * \date 2002
 */
class CLightMemDisplayerA : public CMemDisplayerA
{
public:
	/// Constructor
	CLightMemDisplayerA (const char *displayerName = "") : CMemDisplayerA(displayerName) { }

protected:
	/// Put the string into the file.
    virtual void	doDisplay ( const CLogA::TDisplayInfo& args, const char *message );
};

class CLightMemDisplayerW : public CMemDisplayerW
{
public:
	/// Constructor
	CLightMemDisplayerW (const char *displayerName = "") : CMemDisplayerW(displayerName) { }

protected:
	/// Put the string into the file.
    virtual void	doDisplay ( const CLogW::TDisplayInfo& args, const ucchar *message );
};

} // SIPBASE


#endif // __MEM_DISPLAYER_H__

/* End of mem_displayer.h */
