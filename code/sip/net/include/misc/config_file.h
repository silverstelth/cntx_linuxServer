/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CONFIG_FILE_H__
#define __CONFIG_FILE_H__

#include "types_sip.h"
#include "common.h"
#include "ucstring.h"
#include "debug.h"
#include "log.h"

#include <vector>
#include <string>
#include <stdio.h> // <cstdio>


namespace SIPBASE
{

/**
 * CConfigFile class. Useful when you want to have a configuration file with variables.
 * It manages integers, real (double), and string basic types. A variable can be an array of
 * basic type. In this case, all elements of the array must have the same type.
 *
 * If you setup the global callback before loading, it'll be call after the load() function.
 *
 * Example:
 *\code
 * try
 * {
 * 	CConfigFile cf;
 *
 * 	// Load and parse "test.txt" file
 *  cf.load ("test.txt");
 *
 *	// Attach a callback to the var1 variable. When the var1 will changed, this cvar1cb function will be called
 *	cf.setCallback ("var1", var1cb);
 * 
 *	// Get the foo variable (suppose it's a string variable)
 *	CConfigFile::CVar &foo = cf.getVar ("foo");
 * 
 *	// Display the content of the variable
 *	printf ("foo = %s\n", foo.asString ().c_str ());
 * 
 * 	// Get the bar variable (suppose it's an array of int)
 * 	CConfigFile::CVar &bar = cf.getVar ("bar");
 * 
 * 	// Display the content of the all elements of the bar variable
 * 	printf ("bar have %d elements : \n", bar.size ());
 * 	for (int i = 0; i < bar.size (); i++)
 * 		printf ("%d ", bar.asInt (i));
 * 	printf("\n");
 * }
 * catch (EConfigFile &e)
 * {
 *	// Something goes wrong... catch that
 * 	printf ("%s\n", e.what ());
 * }
 *\endcode
 *
 * Example of config file:
 *\code
 * // one line comment
 * / * big comment
 *     on more than one line * /
 * 
 * var1 = 123;                           // var1  type:int,         value:123
 * var2 = "456.25";                      // var2  type:string,      value:"456.25"
 * var3 = 123.123;                       // var3  type:real,        value:123.123
 * 
 * // the resulting type is type of the first left value
 * var4 = 123.123 + 2;                   // var4  type:real,        value:125.123
 * var5 = 123 + 2.1;                     // var5  type:int,         value:125
 * 
 * var6 = (-112+1) * 3 - 14;             // var6  type:int,         value:-347
 * 
 * var7 = var1 + 1;                      // var7  type:int,         value:124
 * 
 * var8 = var2 + 10;                     // var8  type:string,      value:456.2510 (convert 10 into a string and concat it)
 * var9 = 10.15 + var2;                  // var9  type:real,        value:466.4 (convert var2 into a real and add it)
 * 
 * var10 = { 10.0, 51.1 };               // var10 type:realarray,   value:{10.0,51.1}
 * var11 = { "str1", "str2", "str3" };   // var11 type:stringarray, value:{"str1", "str2", "str3"}
 * 
 * var12 = { 10+var1, var1-var7 };       // var12 type:intarray,    value:{133,-1}
 *\endcode
 *
 * Operators are '+', '-', '*', '/'.
 * You can't use operators on a array variable, for example, you can't do \cvar13=var12+1.
 * If you have 2 variables with the same name, the first value will be remplaced by the second one.
 *
 * \bug if you terminate the config file with a comment without carriage returns it'll generate an exception, add a carriage returns
 *
 * \date 2000
 */
class CConfigFile
{
public:

	/**
	 * CVar class. Used by CConfigFile. A CVar is return when you want to have a variable.
	 *
	 * Example: see the CConfigFile example
	 *
	 * \date 2000
	 */
	struct CVar
	{
	public:

		CVar () : Type(T_UNKNOWN), Root(false), FromLocalFile(true), SaveWrap(6) {}

		/// \name Access to the variable content.
		//@{
		/// Get the content of the variable as an integer
		int					asInt		(int index=0) const;
		/// Get	the content of the variable as a double
		double				asDouble	(int index=0) const;
		/// Get the content of the variable as a float
		float				asFloat		(int index=0) const;
		/// Get the content of the variable as a STL string
		std::string			asString	(int index=0) const;
#ifndef SIP_DONT_USE_EXTERNAL_CODE
		/// Get the content of the variable as a boolean
		bool				asBool		(int index=0) const;
#endif // SIP_DONT_USE_EXTERNAL_CODE
		//@}

		/// \name Set the variable content.
		/// If the index is the size of the array, the value will be append at the end.
		//@{
		/// Set the content of the variable as an integer
		void				setAsInt	(int val, int index=0);
		/// Set	the content of the variable as a double
		void				setAsDouble	(double val, int index=0);
		/// Set the content of the variable as a float
		void				setAsFloat	(float val, int index=0);
		/// Set the content of the variable as a STL string
		void				setAsString	(const std::string &val, int index=0);

		/// Force the content of the variable to be a single integer
		void				forceAsInt	(int val);
		/// Force the content of the variable to be a single double
		void				forceAsDouble	(double val);
		/// Force the content of the variable to be a single string
		void				forceAsString	(const std::string &val);
		
		/// Set the content of the aray variable as an integer
		void				setAsInt	(const std::vector<int> &vals);
		/// Set the content of the aray variable as a double
		void				setAsDouble	(const std::vector<double> &vals);
		/// Set the content of the aray variable as a float
		void				setAsFloat	(const std::vector<float> &vals);
		/// Set the content of the aray variable as a string
		void				setAsString	(const std::vector<std::string> &vals);
		
		//@}

		bool		operator==	(const CVar& var) const;
		bool		operator!=	(const CVar& var) const;

		// add this variable with var
		void		add (const CVar &var);

		// Get the size of the variable. It's the number of element of the array or 1 if it's not an array.
		uint		size () const;

		/// \name Internal use
		//@{
		static const ucchar *TypeName[];

#ifndef SIP_DONT_USE_EXTERNAL_CODE
		enum TVarType { T_UNKNOWN, T_INT, T_STRING, T_REAL, T_BOOL };
#else
		enum TVarType { T_UNKNOWN, T_INT, T_STRING, T_REAL };
#endif // SIP_DONT_USE_EXTERNAL_CODE

		std::string					Name;
		TVarType					Type;
		bool						Root;		// true if this var comes from the root document. false else.
		bool						Comp;		// true if the the parser found a 'complex' var (ie an array)
		bool						FromLocalFile;	// Used during cfg parsing. True if the var has been created from the currently parsed cfg
		std::vector<int>			IntValues;
		std::vector<double>			RealValues;
		std::vector<std::string>	StrValues;

		int							SaveWrap;

		void						(*Callback)(CVar &var);
		//@}
	};

	CConfigFile() : _Callback(NULL) {}
	
	virtual ~CConfigFile ();

	/// Get a variable with the variable name
	CVar &getVar (const std::string &varName);

	/// Get a variable pointer with the variable name, without throwing exception. Return NULL if not found.
	CVar *getVarPtr (const std::string &varName);

	/// Get the variable count.
	uint	getNumVar () const;

	/// Get a variable.
	CVar	*getVar (uint varId);

	/// Add a variable. If the variable already exit, return it.
	CVar	*insertVar (const std::string &varName, const CVar &varToCopy);

	/// Return true if the variable exists, false otherwise
	bool exists (const std::string &varName);

	/// load and parse the file
	void load (const std::string &fileName, bool lookupPaths = false);
	void load (const std::basic_string<ucchar> &fileName, bool lookupPaths = false);

	/// save the config file
	void save () const;
	void saveAs ( const std::string &fileName ) const;
	void saveAs ( const std::basic_string<ucchar> &fileName ) const;

	/// Clear all the variable array (including information on variable callback etc)
	void clear ();

	/// set to 0 or "" all variable in the array (but not destroy them)
	void clearVars ();

	/// Returns true if the file has been loaded
	bool loaded();

	/// reload and reparse the file
	void reparse (bool lookupPaths = false);

	/// display all variables with sipinfo (debug use)
	void display () const;

	/// display all variables with sipinfo (debug use)
	void display (CLog *log) const;

	/// set a callback function that is called when the config file is modified
	void setCallback (void (*cb)());

	/// set a callback function to a variable, it will be called when this variable is modified
	void setCallback (const std::string &VarName, void (*cb)(CConfigFile::CVar &var));

	/// contains the variable names that getVar() and getVarPtr() tried to access but not present in the cfg
	std::vector<std::string> UnknownVariables;

	/// returns the config file name
	std::basic_string<ucchar> getFilename () const { return FileNames[0]; }

	/// set the time between 2 file checking (default value is 1 second)
	/// \param timeout time in millisecond, if timeout=0, the check will be made each "frame"
	static void setTimeout (uint32 timeout);

	/// Internal use only
	static void checkConfigFiles ();

private:

	/// Internal use only
	void (*_Callback)();

	/// Internal use only
	std::vector<CVar>	_Vars;

	// contains the configfilename (0) and roots configfilenames
	std::vector<std::basic_string<ucchar> > FileNames;
	std::vector<uint32>			LastModified;

	static uint32	_Timeout;

	static std::vector<CConfigFile *> *_ConfigFiles;
};

struct EConfigFile : public Exception
{
	EConfigFile() 
	{ 
		_Reason  =  "Unknown Config File Exception";
	}
};

struct EBadType : public EConfigFile
{
	EBadType (const std::string &varName, int varType, int wantedType)
	{
		ucstring ucstr;
		ucstr.fromUtf8(varName);
		static ucchar str[SIPBASE::MaxCStringSize];
#ifdef SIP_OS_WINDOWS
		smprintf (str, SIPBASE::MaxCStringSize, L"Bad variable type, variable \"%s\" is a %s and not a %s", ucstr.c_str (), CConfigFile::CVar::TypeName[varType], CConfigFile::CVar::TypeName[wantedType]);
#else
		smprintf (str, SIPBASE::MaxCStringSize, L"Bad variable type, variable \"%S\" is a %S and not a %S", ucstr.c_str (), CConfigFile::CVar::TypeName[varType], CConfigFile::CVar::TypeName[wantedType]);
#endif
		ucstr = str;
		_Reason = ucstr.toUtf8();
#ifdef SIP_OS_WINDOWS
		sipinfo(L"CF: Exception will be launched: %s", str);
#else
		sipinfo("CF: Exception will be launched: %S", str);
#endif
	}
};

struct EBadSize : public EConfigFile
{
	EBadSize (const std::string &varName, int varSize, int varIndex)
	{
		ucstring ucstr;
		ucstr.fromUtf8(varName);
		static ucchar str[SIPBASE::MaxCStringSize];
#ifdef SIP_OS_WINDOWS
		smprintf (str, SIPBASE::MaxCStringSize, L"Trying to access to the index %d but the variable \"%s\" size is %d", varIndex, ucstr.c_str (), varSize);
#else
		smprintf (str, SIPBASE::MaxCStringSize, L"Trying to access to the index %d but the variable \"%S\" size is %d", varIndex, ucstr.c_str (), varSize);
#endif
		ucstr = str;
		_Reason = ucstr.toUtf8();
#ifdef SIP_OS_WINDOWS
		sipinfo(L"CF: Exception will be launched: %s", str);
#else
		sipinfo("CF: Exception will be launched: %S", str);
#endif
	}
};

struct EUnknownVar : public EConfigFile
{
	EUnknownVar (const std::basic_string<ucchar> &filename, const std::string &varName)
	{
		ucstring ucstr;
		ucstr.fromUtf8(varName);
		static ucchar str[SIPBASE::MaxCStringSize];
#ifdef SIP_OS_WINDOWS
		smprintf (str, SIPBASE::MaxCStringSize, L"variable \"%s\" not found in file \"%s\"", ucstr.c_str (), filename.c_str());
#else
		smprintf (str, SIPBASE::MaxCStringSize, L"variable \"%S\" not found in file \"%S\"", ucstr.c_str (), filename.c_str());
#endif
		ucstr = str;
		_Reason = ucstr.toUtf8();
#ifdef SIP_OS_WINDOWS
		sipinfo(L"CF: Exception will be launched: %s", str);
#else
		sipinfo("CF: Exception will be launched: %S", str);
#endif
	}
};

struct EParseError : public EConfigFile
{
	EParseError (const std::basic_string<ucchar> &fileName, int currentLine)
	{
		static ucchar str[SIPBASE::MaxCStringSize];
#ifdef SIP_OS_WINDOWS
		smprintf (str, SIPBASE::MaxCStringSize, L"Parse error on the \"%s\" file, line %d", fileName.c_str (), currentLine);
#else
		smprintf (str, SIPBASE::MaxCStringSize, L"Parse error on the \"%S\" file, line %d", fileName.c_str (), currentLine);
#endif
		ucstring ucstr = str;
		_Reason = ucstr.toUtf8();
#ifdef SIP_OS_WINDOWS
		sipinfo(L"CF: Exception will be launched: %s", str);
#else
		sipinfo("CF: Exception will be launched: %S", str);
#endif
	}
};

struct EFileNotFound : public EConfigFile
{
	EFileNotFound (const std::basic_string<ucchar> &fileName)
	{
		static ucchar str[SIPBASE::MaxCStringSize];
#ifdef SIP_OS_WINDOWS
		smprintf (str, SIPBASE::MaxCStringSize, L"File \"%s\" not found", fileName.c_str ());
#else
		smprintf (str, SIPBASE::MaxCStringSize, L"File \"%S\" not found", fileName.c_str ());
#endif
		ucstring ucstr = str;
		_Reason = ucstr.toUtf8();
	}
};

} // SIPBASE

#endif // __CONFIG_FILE_H__

/* End of config_file.h */
