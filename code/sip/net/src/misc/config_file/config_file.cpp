/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>

#include "misc/stdmisc.h"
#include "misc/file.h"
#include "misc/debug.h"
#include "misc/config_file.h"
#include "misc/path.h"
#include "misc/i18n.h"
#include "misc/mem_stream.h"
#include "locale.h"

using namespace std;
using namespace SIPBASE;

extern void cfrestart (FILE *);	// used to reinit the file
extern int cfparse (void *);	// used to parse the file
extern void cf_delete_current_buffer ();

//extern FILE *cfin;
extern int cf_CurrentLine;
extern bool cf_Ignore;
extern bool cf_OverwriteExistingVariable;
extern CMemStream cf_ifile;

// put true if you want that the config file class check type when you call asFunctions
// (for example, check when you call asInt() that the variable is an int).
// when it's false, the function will convert to the wanted type (if he can)
const bool CheckType = false;
bool LoadRoot = false;

namespace SIPBASE
{

#ifndef SIP_DONT_USE_EXTERNAL_CODE
const ucchar *CConfigFile::CVar::TypeName[] = { L"Integer", L"String", L"Float", L"Boolean" };
#else
const ucchar *CConfigFile::CVar::TypeName[] = { L"Integer", L"String", L"Float" };
#endif // SIP_DONT_USE_EXTERNAL_CODE

int CConfigFile::CVar::asInt (int index) const
{
	if (CheckType && Type != T_INT) throw EBadType (Name, Type, T_INT);
	switch (Type)
	{
	case T_STRING:
		if (index >= (int)StrValues.size () || index < 0) throw EBadSize (Name, StrValues.size (), index);
		return atoi(StrValues[index].c_str());
	case T_REAL:
		if (index >= (int)RealValues.size () || index < 0) throw EBadSize (Name, RealValues.size (), index);
		return (int)RealValues[index];
	default:
		if (index >= (int)IntValues.size () || index < 0) throw EBadSize (Name, IntValues.size (), index);
		return IntValues[index];
	}
}

double CConfigFile::CVar::asDouble (int index) const
{
	if (CheckType && Type != T_REAL) throw EBadType (Name, Type, T_REAL);
	switch (Type)
	{
	case T_INT:
		if (index >= (int)IntValues.size () || index < 0) throw EBadSize (Name, IntValues.size (), index);
		return (double)IntValues[index];
	case T_STRING:
		if (index >= (int)StrValues.size () || index < 0) throw EBadSize (Name, StrValues.size (), index);
		return atof(StrValues[index].c_str());
	default:
		if (index >= (int)RealValues.size () || index < 0) throw EBadSize (Name, RealValues.size (), index);
		return RealValues[index];
	}
}

float CConfigFile::CVar::asFloat (int index) const
{
	return (float) asDouble (index);
}

std::string CConfigFile::CVar::asString (int index) const
{
	if (CheckType && Type != T_STRING) throw EBadType (Name, Type, T_STRING);
	switch (Type)
	{
	case T_INT:
		if (index >= (int)IntValues.size () || index < 0) throw EBadSize (Name, IntValues.size (), index);
		return toStringA(IntValues[index]);
	case T_REAL:
		if (index >= (int)RealValues.size () || index < 0) throw EBadSize (Name, RealValues.size (), index);
		return toStringA(RealValues[index]);
	default:
		if (index >= (int)StrValues.size () || index < 0) throw EBadSize (Name, StrValues.size (), index);
		return StrValues[index];
	}
}

#ifndef SIP_DONT_USE_EXTERNAL_CODE
bool CConfigFile::CVar::asBool (int index) const
{
	switch (Type)
	{
	case T_STRING:
		if (index >= (int)StrValues.size () || index < 0) throw EBadSize (Name, StrValues.size (), index);
		if(StrValues[index] == "true")
		{
			return true;
		}
		else
		{
			return false;
		}
	case T_REAL:
		if (index >= (int)RealValues.size () || index < 0) throw EBadSize (Name, RealValues.size (), index);
		if ((int)RealValues[index] == 1)
		{
			return true;
		}
		else
		{
			return false;
		}
	default:
		if (index >= (int)IntValues.size () || index < 0) throw EBadSize (Name, IntValues.size (), index);
		if (IntValues[index] == 1)
		{
			return true;
		}
		else
		{
			return false;
		}
	}
}
#endif // SIP_DONT_USE_EXTERNAL_CODE


void CConfigFile::CVar::setAsInt (int val, int index)
{
	if (Type != T_INT) throw EBadType (Name, Type, T_INT);
	else if (index > (int)IntValues.size () || index < 0) throw EBadSize (Name, IntValues.size (), index);
	else if (index == (int)IntValues.size ()) IntValues.push_back(val);
	else IntValues[index] = val;
	Root = false;
}

void CConfigFile::CVar::setAsDouble (double val, int index)
{
	if (Type != T_REAL) throw EBadType (Name, Type, T_REAL);
	else if (index > (int)RealValues.size () || index < 0) throw EBadSize (Name, RealValues.size (), index);
	else if (index == (int)RealValues.size ()) RealValues.push_back(val);
	else RealValues[index] = val;
	Root = false;
}

void CConfigFile::CVar::setAsFloat (float val, int index)
{
	setAsDouble (val, index);
}

void CConfigFile::CVar::setAsString (const std::string &val, int index)
{
	if (Type != T_STRING) throw EBadType (Name, Type, T_STRING);
	else if (index > (int)StrValues.size () || index < 0) throw EBadSize (Name, StrValues.size (), index);
	else if (index == (int)StrValues.size ()) StrValues.push_back(val);
	else StrValues[index] = val;
	Root = false;
}

void CConfigFile::CVar::forceAsInt	(int val)
{
	Type= T_INT;
	IntValues.resize(1);
	RealValues.clear();
	StrValues.clear();
	IntValues[0]= val;
	Root = false;
}

void CConfigFile::CVar::forceAsDouble	(double val)
{
	Type= T_REAL;
	IntValues.clear();
	RealValues.resize(1);
	StrValues.clear();
	RealValues[0]= val;
	Root = false;
}

void CConfigFile::CVar::forceAsString	(const std::string &val)
{
	Type= T_STRING;
	IntValues.clear();
	RealValues.clear();
	StrValues.resize(1);
	StrValues[0]= val;
	Root = false;
}

void CConfigFile::CVar::setAsInt (const std::vector<int> &vals)
{
	if (Type != T_INT) throw EBadType (Name, Type, T_INT);
	else IntValues = vals;
	Root = false;
}

void CConfigFile::CVar::setAsDouble (const std::vector<double> &vals)
{
	if (Type != T_REAL) throw EBadType (Name, Type, T_REAL);
	else RealValues = vals;
	Root = false;
}

void CConfigFile::CVar::setAsFloat (const std::vector<float> &vals)
{
	if (Type != T_REAL) throw EBadType (Name, Type, T_REAL);
	else
	{
		RealValues.clear ();
		RealValues.resize (vals.size ());
		for (uint i = 0; i < vals.size (); i++)
			RealValues[i] = (double)vals[i];
	}
	Root = false;
}

void CConfigFile::CVar::setAsString (const std::vector<std::string> &vals)
{
	if (Type != T_STRING) throw EBadType (Name, Type, T_STRING);
	else StrValues = vals;
	Root = false;
}

bool CConfigFile::CVar::operator==	(const CVar& var) const
{
	if (Type == var.Type)
	{
		switch (Type)
		{
		case T_INT: return IntValues == var.IntValues; break;
		case T_REAL: return RealValues == var.RealValues; break;
		case T_STRING: return StrValues == var.StrValues; break;
		default: break;
		}
	}
	return false;
}

bool CConfigFile::CVar::operator!=	(const CVar& var) const
{
	return !(*this==var);
}

void CConfigFile::CVar::add (const CVar &var)
{
	if (Type == var.Type)
	{
		switch (Type)
		{
		case T_INT: IntValues.insert (IntValues.end(), var.IntValues.begin(), var.IntValues.end()); break;
		case T_REAL: RealValues.insert (RealValues.end(), var.RealValues.begin(), var.RealValues.end()); break;
		case T_STRING: StrValues.insert (StrValues.end(), var.StrValues.begin(), var.StrValues.end()); break;
		default: break;
		}
	}
}

uint CConfigFile::CVar::size () const
{
	switch (Type)
	{
	case T_INT: return IntValues.size ();
	case T_REAL: return RealValues.size ();
	case T_STRING: return StrValues.size ();
	default: return 0;
	}
}

CConfigFile::~CConfigFile ()
{
	if (_ConfigFiles == NULL || (*_ConfigFiles).empty ()) return;

	vector<CConfigFile *>::iterator it = find ((*_ConfigFiles).begin (), (*_ConfigFiles).end (), this);
	if (it != (*_ConfigFiles).end ())
	{
		(*_ConfigFiles).erase (it);
	}

	if ((*_ConfigFiles).empty())
	{
		delete _ConfigFiles;
		_ConfigFiles = NULL;
	}
}

void CConfigFile::load (const string &fileName, bool lookupPaths )
{
	ucstring ucstr;
	ucstr.fromUtf8(fileName);
	load (ucstr, lookupPaths);
}

void CConfigFile::load (const basic_string<ucchar> &fileName, bool lookupPaths )
{
	if (fileName.empty())
	{
		sipwarning ("CF: Can't load a empty file name configfile");
		return;
	}

	FileNames.clear ();
	FileNames.push_back (fileName);

	if (_ConfigFiles == NULL)
	{
		_ConfigFiles = new std::vector<CConfigFile *>;
	}

	(*_ConfigFiles).push_back (this);
	reparse (lookupPaths);
}

bool CConfigFile::loaded()
{
	return !CConfigFile::FileNames.empty();
}

void CConfigFile::reparse (bool lookupPaths)
{
//	clear();
	if (FileNames.empty())
	{
		sipwarning ("CF: Can't reparse config file because file name is empty");
		return;
	}

	std::basic_string<ucchar> fn = FileNames[0];

	FileNames.clear ();
	LastModified.clear ();

	std::vector<CVar>	_VarsBefore, _VarsAfter;
	while (!fn.empty())
	{
		std::vector<CVar>	_VarsFile;
        
		if (lookupPaths)
		{
			fn = CPath::lookup(fn, true);
		}
		else
		{
			fn = SIPBASE::CPath::getFullPath(fn, false);
		}
		sipdebug (L"CF: Adding config file '%s' in the config file", fn.c_str());
		FileNames.push_back (fn);
		LastModified.push_back (CFileW::getFileModificationDate(fn));

		if (!CPath::lookup(fn, false).empty())
		{
			ucstring content;
			CI18N::readTextFile(fn, content, false, true, true);
			string utf8 = content.toUtf8();

			CMemStream stream;
			stream.serialBuffer((uint8*)(utf8.data()), utf8.size());
			cf_ifile = stream;
			if (!cf_ifile.isReading())
			{
				cf_ifile.invert();
			}

			cfrestart (NULL);
			cf_CurrentLine = 1;
			cf_Ignore = false;
			cf_OverwriteExistingVariable = false;
			LoadRoot = (FileNames.size()>1);

			bool parsingOK = (cfparse (&(_VarsFile)) == 0);
			if (!parsingOK)
			{
				// write the result of preprocessing in a temp file
				basic_string<ucchar> debugFileName;
				debugFileName += L"debug_";
				debugFileName += CFileW::getFilename(fn);

				CI18N::writeTextFile(debugFileName, content, true);
				sipwarning (L"CF: Parsing error in file %s line %d, look in '%s' for a preprocessed version of the config file", fn.c_str(), cf_CurrentLine, debugFileName.c_str());
				throw EParseError (fn, cf_CurrentLine);
			}

			// reset all 'FromLocalFile' flag on created vars before reading next root cfg
			for (uint i=0; i<_VarsFile.size(); ++i)
			{
				_VarsFile[i].FromLocalFile = false;
				sipdebug("VarName : %s", _VarsFile[i].Name.c_str());
			}

			std::vector<CVar>	_VarsTemp;
			copy(_VarsBefore.begin(), _VarsBefore.end(), back_inserter(_VarsTemp));
			copy(_VarsFile.begin(), _VarsFile.end(), back_inserter(_VarsTemp));
			copy(_VarsAfter.begin(), _VarsAfter.end(), back_inserter(_VarsTemp));

			// remove overlapped option
			for (uint i=0; i<_VarsTemp.size(); i++)
			{
				CVar	var = _VarsTemp[i];

				bool	bFind = false;
				uint		j, k;
                for (j=i+1; j<_VarsTemp.size(); j++)
				{
					if ( var.Name == _VarsTemp[j].Name )
					{
						bFind = true;
						break;
					}
				}
				if (bFind)
					continue;

				for (k=0; k<_Vars.size(); k++)
				{
					if ( var.Name == _Vars[k].Name )
					{
						bFind = true;
						break;
					}
				}
				if ( bFind )
				{
					if ( var != _Vars[k] && _Vars[k].Callback != NULL )
                        _Vars[k].Callback(var);

					_Vars[k].Comp		= var.Comp;
					_Vars[k].Root		= var.Root;
					_Vars[k].IntValues	= var.IntValues;
					_Vars[k].RealValues	= var.RealValues;
					_Vars[k].StrValues	= var.StrValues;
					_Vars[k].SaveWrap	= var.SaveWrap;
				}
				else
				{
					_Vars.push_back(var);
				}
			}
		}
		else
		{
			sipwarning (L"CF: Config file '%s' not found in the path '%s'", fn.c_str(), CPath::getCurrentPathW().c_str());
			throw EFileNotFound (fn);
		}
//		cf_ifile.close ();
		cf_ifile.clear();
		cf_delete_current_buffer();

		int	n1 = _Vars.size();
		// If we find a linked config file, load it but don't overload already existant variable
		CVar *var = getVarPtr ("InsertConfigFilename");
		if (var)
		{
			// KSR_ADD
			bool	bAfter = false;
			for (uint i=0; i<_VarsFile.size(); ++i)
			{
				std::string	name = _VarsFile[i].Name;
				if ( name == "InsertConfigFilename" )
				{
					_VarsBefore.push_back(_VarsFile[i]);
                    bAfter = true;
					continue;
				}

				if ( !bAfter )
					_VarsBefore.push_back(_VarsFile[i]);
				else
					_VarsAfter.push_back(_VarsFile[i]);
			}

			string temp = var->asString();
			ucstring InsertConfigFilename;
			InsertConfigFilename.fromUtf8(temp);

			if (!SIPBASE::CFileW::fileExists(InsertConfigFilename))
			{
				// file is not found, try with the path of the master cfg
				ucstring path = SIPBASE::CPath::standardizePath (SIPBASE::CFileW::getPath(FileNames[0]));
				InsertConfigFilename = path + InsertConfigFilename;
			}

			InsertConfigFilename = SIPBASE::CPath::getFullPath(InsertConfigFilename, false);

			if (InsertConfigFilename != fn)
			{
				sipinfo (L"CF: InsertConfigFilename variable found in the '%s' config file, parse the root config file '%s'", fn.c_str(), InsertConfigFilename.c_str());
				fn = InsertConfigFilename;
			}
			else
				fn.clear ();
		}
		else
			fn.clear ();	
	}

	if (_Callback != NULL) 
		_Callback();

	display();
}


/*
void CConfigFile::reparse (bool lookupPaths)
{
//	clear();
	if (FileNames.empty())
	{
		sipwarning ("CF: Can't reparse config file because file name is empty");
		return;
	}

	std::basic_string<ucchar> fn = FileNames[0];

	FileNames.clear ();
	LastModified.clear ();

	std::vector<CVar>	_VarsBefore, _VarsAfter;
	while (!fn.empty())
	{
		std::vector<CVar>	_VarsFile;
        
		if (lookupPaths)
		{
			fn = CPath::lookup(fn, true);
		}
		else
		{
			fn = SIPBASE::CPath::getFullPath(fn, false);
		}
		sipdebug (L"CF: Adding config file '%s' in the config file", fn.c_str());
		FileNames.push_back (fn);
		LastModified.push_back (CFileW::getFileModificationDate(fn));

		if (!CPath::lookup(fn, false).empty())
		{
			ucstring content;
			CI18N::readTextFile(fn, content, false, true, true);
			string utf8 = content.toUtf8();

			CMemStream stream;
			stream.serialBuffer((uint8*)(utf8.data()), utf8.size());
			cf_ifile = stream;
			if (!cf_ifile.isReading())
			{
				cf_ifile.invert();
			}

			cfrestart (NULL);
			cf_CurrentLine = 1;
			cf_Ignore = false;
			cf_OverwriteExistingVariable = false;
			LoadRoot = (FileNames.size()>1);

			int nsize1 = _VarsFile.size();
			bool parsingOK = (cfparse (&(_VarsFile)) == 0);
			nsize1 = _VarsFile.size();

			sipinfo("Vars Display");
			display();

			/////////////// reload ///////////////
			cf_ifile = stream;
			if (!cf_ifile.isReading())
			{
				cf_ifile.invert();
			}

			cfrestart (NULL);
			cf_CurrentLine = 1;
			cf_Ignore = false;
			cf_OverwriteExistingVariable = (FileNames.size()==1);
			LoadRoot = (FileNames.size()>1);

			int nsize = _Vars.size();
			parsingOK = (cfparse (&(_Vars)) == 0);
			nsize = _Vars.size();
			if (!parsingOK)
			{
				// write the result of preprocessing in a temp file
				basic_string<ucchar> debugFileName;
				debugFileName += L"debug_";
				debugFileName += CFileW::getFilename(fn);

				CI18N::writeTextFile(debugFileName, content, true);
				sipwarning (L"CF: Parsing error in file %s line %d, look in '%s' for a preprocessed version of the config file", fn.c_str(), cf_CurrentLine, debugFileName.c_str());
				throw EParseError (fn, cf_CurrentLine);
			}

			// reset all 'FromLocalFile' flag on created vars before reading next root cfg
			for (uint i=0; i<_VarsFile.size(); ++i)
			{
				_VarsFile[i].FromLocalFile = false;
				sipdebug("VarName : %s", _VarsFile[i].Name.c_str());
			}

		std::vector<CVar>	_VarsTemp;
		copy(_VarsBefore.begin(), _VarsBefore.end(), back_inserter(_VarsTemp));
		copy(_VarsFile.begin(), _VarsFile.end(), back_inserter(_VarsTemp));
		copy(_VarsAfter.begin(), _VarsAfter.end(), back_inserter(_VarsTemp));

			// remove overlapped option
			for (uint i=0; i<_VarsTemp.size(); i++)
			{
				CVar	var = _VarsTemp[i];

				bool	bFind = false;
				uint		j, k;
                for (j=i+1; j<_VarsTemp.size(); j++)
				{
					if ( var.Name == _VarsTemp[j].Name )
					{
						bFind = true;
						break;
					}
				}
				if (bFind)
					continue;

				for (k=0; k<_Vars.size(); k++)
				{
					if ( var.Name == _Vars[k].Name )
					{
						bFind = true;
						break;
					}
				}
				if ( bFind )
				{
					_Vars[k].Root		= var.Root;
					_Vars[k].IntValues	= var.IntValues;
					_Vars[k].RealValues	= var.RealValues;
					_Vars[k].StrValues	= var.StrValues;
				}
				else
				{
					_Vars.push_back(var);
				}
			}
		}
		else
		{
			sipwarning (L"CF: Config file '%s' not found in the path '%s'", fn.c_str(), CPath::getCurrentPathW().c_str());
			throw EFileNotFound (fn);
		}
//		cf_ifile.close ();
		cf_ifile.clear();

		int	n1 = _Vars.size();
		// If we find a linked config file, load it but don't overload already existant variable
		CVar *var = getVarPtr ("InsertConfigFilename");
		if (var)
		{
			// KSR_ADD
			bool	bAfter = false;
			for (uint i=0; i<_VarsFile.size(); ++i)
			{
				std::string	name = _VarsFile[i].Name;
				if ( name == "InsertConfigFilename" )
					bAfter = true;

				if ( !bAfter )
					_VarsBefore.push_back(_VarsFile[i]);
				else
					_VarsAfter.push_back(_VarsFile[i]);
			}

			string temp = var->asString();
			ucstring InsertConfigFilename;
			InsertConfigFilename.fromUtf8(temp);

			if (!SIPBASE::CFileW::fileExists(InsertConfigFilename))
			{
				// file is not found, try with the path of the master cfg
				ucstring path = SIPBASE::CPath::standardizePath (SIPBASE::CFileW::getPath(FileNames[0]));
				InsertConfigFilename = path + InsertConfigFilename;
			}

			InsertConfigFilename = SIPBASE::CPath::getFullPath(InsertConfigFilename, false);

			if (InsertConfigFilename != fn)
			{
				sipinfo (L"CF: InsertConfigFilename variable found in the '%s' config file, parse the root config file '%s'", fn.c_str(), InsertConfigFilename.c_str());
				fn = InsertConfigFilename;
			}
			else
				fn.clear ();
		}
		else
			fn.clear ();	
	}

	if (_Callback != NULL) 
		_Callback();

	display();
}
*/

CConfigFile::CVar &CConfigFile::getVar (const std::string &varName)
{
	CVar *var =  getVarPtr (varName);
	if (var == 0) 
		throw EUnknownVar (getFilename(), varName);
	else
		return *var;
}


CConfigFile::CVar *CConfigFile::getVarPtr (const std::string &varName)
{
	uint i;
	for (i = 0; i < _Vars.size(); i++)
	{
		// the type could be T_UNKNOWN if we add a callback on this name but this var is not in the config file
		if (_Vars[i].Name == varName && (_Vars[i].Type != CVar::T_UNKNOWN || _Vars[i].Comp))
			return &(_Vars[i]);
	}

	// if not found, add it in the array if necessary
	for (i = 0; i < UnknownVariables.size(); i++)
		if(UnknownVariables[i] == varName)
			break;
	if (i == UnknownVariables.size())
		UnknownVariables.push_back(varName);

	return NULL;
}

bool CConfigFile::exists (const std::string &varName)
{
	for (uint i = 0; i < _Vars.size(); i++)
	{
		// the type could be T_UNKNOWN if we add a callback on this name but this var is not in the config file
		if (_Vars[i].Name == varName && (_Vars[i].Type != CVar::T_UNKNOWN || _Vars[i].Comp))
		{
			return true;
		}
	}
	return false;
}

void CConfigFile::save () const
{
	saveAs( L"" );
}

void CConfigFile::saveAs ( const std::string &fileName ) const
{
	std::basic_string<ucchar> ufileName = SIPBASE::toString(L"%s", fileName.c_str());
	saveAs( ufileName );
}

void CConfigFile::saveAs ( const std::basic_string<ucchar> &fileName ) const
{
	// Avoid any problem, Force Locale to default
	setlocale(LC_ALL, "C");

	FILE *fp;
	if ( fileName.empty() )
	{
		fp = sfWfopen (getFilename().c_str (), L"w");
		if (fp == NULL)
		{
			sipwarning (L"CF: Couldn't create %s file", getFilename().c_str());
			return;
		}
	}
	else
	{
		fp = sfWfopen (fileName.c_str (), L"w");
		if (fp == NULL)
		{
			sipwarning (L"CF: Couldn't create %s file", fileName.c_str());
			return;
		}
	}
	
	static char utf8Header[] = {char(0xef), char(0xbb), char(0xbf), 0};
	fwrite( utf8Header, 1, 3, fp );
	for(int i = 0; i < (int)_Vars.size(); i++)
	{
		// Not a root value 
		if (!_Vars[i].Root)
		{
			if (_Vars[i].Comp)
			{
				fprintf(fp, "%-20s = {", _Vars[i].Name.c_str());
				switch (_Vars[i].Type)
				{
				case CConfigFile::CVar::T_INT:
				{
					for (int it=0; it < (int)_Vars[i].IntValues.size(); it++)
					{
						if (it%_Vars[i].SaveWrap == 0)
						{
							fprintf(fp, "\n\t");
						}
						fprintf(fp, "%d%s", _Vars[i].IntValues[it], it<(int)_Vars[i].IntValues.size()-1?", ":" ");
					}
					break;
				}
				case CConfigFile::CVar::T_STRING:
				{
					for (int st=0; st < (int)_Vars[i].StrValues.size(); st++)
					{
						if (st%_Vars[i].SaveWrap == 0)
						{
							fprintf(fp, "\n\t");
						}
						fprintf(fp, "\"%s\"%s", _Vars[i].StrValues[st].c_str(), st<(int)_Vars[i].StrValues.size()-1?", ":" ");
					}
					break;
				}
				case CConfigFile::CVar::T_REAL:
				{
					for (int rt=0; rt < (int)_Vars[i].RealValues.size(); rt++)
					{
						if (rt%_Vars[i].SaveWrap == 0)
						{
							fprintf(fp, "\n\t");
						}
						fprintf(fp, "%.10f%s", _Vars[i].RealValues[rt], rt<(int)_Vars[i].RealValues.size()-1?", ":" ");
					}
					break;
				}
				default: break;
				}
				fprintf(fp, "\n};\n");
			}
			else
			{
				switch (_Vars[i].Type)
				{
				case CConfigFile::CVar::T_INT:
					fprintf(fp, "%-20s = %d;\n", _Vars[i].Name.c_str(), _Vars[i].IntValues[0]);
					break;
				case CConfigFile::CVar::T_STRING:
					fprintf(fp, "%-20s = \"%s\";\n", _Vars[i].Name.c_str(), _Vars[i].StrValues[0].c_str());
					break;
				case CConfigFile::CVar::T_REAL:
					fprintf(fp, "%-20s = %.10f;\n", _Vars[i].Name.c_str(), _Vars[i].RealValues[0]);
					break;
				default: break;
				}
			}
		}
	}
	fclose (fp);
}

void CConfigFile::display () const
{
	display (InfoLog);
}

void CConfigFile::display (CLog *log) const
{
	createDebug ();

	log->displayRawNL ("Config file %s have %d variables and %d root config file:", getFilename().c_str(), _Vars.size(), FileNames.size()-1);
	log->displayRaw ("Root config files: ");
	for(int i = 1; i < (int)FileNames.size(); i++)
	{
		log->displayRaw (FileNames[i].c_str());
	}
	log->displayRawNL ("");
	log->displayRawNL ("------------------------------------------------------");
	for(int i = 0; i < (int)_Vars.size(); i++)
	{
		log->displayRaw ((_Vars[i].Callback==NULL)?"   ":"CB ");
		log->displayRaw ((_Vars[i].Root)?"Root ":"     ");
		if (_Vars[i].Comp)
		{
			switch (_Vars[i].Type)
			{
			case CConfigFile::CVar::T_INT:
			{
				log->displayRaw ("%-20s { ", _Vars[i].Name.c_str());
				for (int it=0; it < (int)_Vars[i].IntValues.size(); it++)
				{
					log->displayRaw ("'%d' ", _Vars[i].IntValues[it]);
				}
				log->displayRawNL ("}");
				break;
			}
			case CConfigFile::CVar::T_STRING:
			{
				log->displayRaw ("%-20s { ", _Vars[i].Name.c_str());
				for (int st=0; st < (int)_Vars[i].StrValues.size(); st++)
				{
					log->displayRaw ("\"%s\" ", _Vars[i].StrValues[st].c_str());
				}
				log->displayRawNL ("}");
				break;
			}
			case CConfigFile::CVar::T_REAL:
			{
				log->displayRaw ("%-20s { " , _Vars[i].Name.c_str());
				for (int rt=0; rt < (int)_Vars[i].RealValues.size(); rt++)
				{
					log->displayRaw ("`%f` ", _Vars[i].RealValues[rt]);
				}
				log->displayRawNL ("}");
				break;
			}
			case CConfigFile::CVar::T_UNKNOWN:
			{
				 log->displayRawNL ("%-20s { }" , _Vars[i].Name.c_str());
				break;
			}
			default:
			{
				log->displayRawNL ("%-20s <default case comp> (%d)" , _Vars[i].Name.c_str(), _Vars[i].Type);
				break;
			}
			}
		}
		else
		{
			switch (_Vars[i].Type)
			{
			case CConfigFile::CVar::T_INT:
				log->displayRawNL ("%-20s '%d'", _Vars[i].Name.c_str(), _Vars[i].IntValues[0]);
				break;
			case CConfigFile::CVar::T_STRING:
				log->displayRawNL ("%-20s \"%s\"", _Vars[i].Name.c_str(), _Vars[i].StrValues[0].c_str());
				break;
			case CConfigFile::CVar::T_REAL:
				log->displayRawNL ("%-20s `%f`", _Vars[i].Name.c_str(), _Vars[i].RealValues[0]);
				break;
			case CConfigFile::CVar::T_UNKNOWN:
				log->displayRawNL ("%-20s <Unknown>", _Vars[i].Name.c_str());
				break;
			default:
			{
				log->displayRawNL ("%-20s <default case> (%d)" , _Vars[i].Name.c_str(), _Vars[i].Type);
				break;
			}
			}
		}
	}
}

void CConfigFile::setCallback (void (*cb)())
{
	_Callback = cb;
	if (!FileNames.empty())
	{
#ifdef SIP_OS_WINDOWS
		sipinfo ("CF: Setting callback to reload the file '%s' when modified externally", getFilename().c_str());
#else
		sipinfo ("CF: Setting callback to reload the file '%S' when modified externally", getFilename().c_str());
#endif
	}
}

void CConfigFile::setCallback (const string &VarName, void (*cb)(CConfigFile::CVar &var))
{
	for (vector<CVar>::iterator it = _Vars.begin (); it != _Vars.end (); it++)
	{
		if (VarName == (*it).Name)
		{
			(*it).Callback = cb;
#ifdef SIP_OS_WINDOWS
			sipinfo ("CF: Setting callback to reload the variable '%s' in the file '%s' when modified externally", VarName.c_str(), getFilename().c_str());
#else
			sipinfo ("CF: Setting callback to reload the variable '%s' in the file '%S' when modified externally", VarName.c_str(), getFilename().c_str());
#endif
			return;
		}
	}
	// VarName doesn't exist, add it now for the future
	CVar Var;
	Var.Name = VarName;
	Var.Callback = cb;
	Var.Type = CVar::T_UNKNOWN;
	Var.Comp = false;
	_Vars.push_back (Var);
#ifdef SIP_OS_WINDOWS
	sipinfo ("CF: Setting callback to reload the variable '%s' in the file '%s' when modified externally (currently unknown)", VarName.c_str(), getFilename().c_str());
#else
	sipinfo ("CF: Setting callback to reload the variable '%s' in the file '%S' when modified externally (currently unknown)", VarName.c_str(), getFilename().c_str());
#endif
}

// ***************************************************************************


vector<CConfigFile *> *CConfigFile::_ConfigFiles = NULL;

uint32	CConfigFile::_Timeout = 1000;

void CConfigFile::checkConfigFiles ()
{
	if (_ConfigFiles == NULL) return;

	static time_t LastCheckTime = time (NULL);
	if (_Timeout > 0 && (float)(time (NULL) - LastCheckTime)*1000.0f < (float)_Timeout) return;

	LastCheckTime = time (NULL);

	bool needReparse;
	for (vector<CConfigFile *>::iterator it = (*_ConfigFiles).begin (); it != (*_ConfigFiles).end (); it++)
	{
		needReparse = false;
		sipassert ((*it)->FileNames.size() == (*it)->LastModified.size());
		for (uint i = 0; i < (*it)->FileNames.size(); i++)
		{
			if ((*it)->LastModified[i] != CFileW::getFileModificationDate((*it)->FileNames[i]))
			{
				needReparse = true;
				(*it)->LastModified[i] = CFileW::getFileModificationDate((*it)->FileNames[i]);
			}
		}
		if (needReparse)
		{
			try
			{
				(*it)->reparse ();
			}
			catch (EConfigFile &e)
			{
				sipwarning ("CF: Exception will re-read modified config file '%s': %s", (*it)->getFilename().c_str(), e.what ());
			}
		}
	}
}

void CConfigFile::setTimeout (uint32 timeout)
{
	_Timeout = timeout;
}

void CConfigFile::clear()
{
	_Vars.clear ();
}

void CConfigFile::clearVars ()
{
	for (vector<CVar>::iterator it = _Vars.begin (); it != _Vars.end (); it++)
	{
		(*it).Type = CVar::T_UNKNOWN;
	}
}

uint CConfigFile::getNumVar () const
{
	return _Vars.size ();
}

CConfigFile::CVar *CConfigFile::getVar (uint varId)
{
	return &(_Vars[varId]);
}

CConfigFile::CVar *CConfigFile::insertVar (const std::string &varName, const CVar &varToCopy)
{
	// Get the var
	CVar *var = getVarPtr (varName);
	if (!var)
	{
		_Vars.push_back (varToCopy);
		var = &(_Vars.back ());
		var->Root = false;
		var->Name = varName;
	}
	return var;
}

} // SIPBASE
