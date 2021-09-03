/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/words_dictionary.h"
#include "misc/config_file.h"
#include "misc/path.h"
#include "misc/diff_tool.h"

using namespace std;

const string DefaultColTitle = "name";

namespace SIPBASE {

SIP_INSTANCE_COUNTER_IMPL(CWordsDictionary);

/*
 * Constructor
 */
CWordsDictionary::CWordsDictionary()
{
}


/* Load the config file and the related words files. Return false in case of failure.
 * Config file variables:
 * - WordsPath: where to find <filter>_words_<languageCode>.txt
 * - LanguageCode: language code (ex: en for English)
 * - Utf8: results are in UTF8, otherwise in ANSI string
 * - Filter: "*" for all files (default) or a name (ex: "item").
 * - AdditionalFiles/AdditionalFileColumnTitles
 */
bool CWordsDictionary::init( const string& configFileName )
{
	// Read config file
	bool cfFound = false;
	CConfigFile cf;
	try
	{
		cf.load( configFileName );
		cfFound = true;
	}
	catch ( EConfigFile& e )
	{
		sipwarning( "WD: %s", e.what() );
	}
	string wordsPath, languageCode, filter = "*";
	vector<string> additionalFiles, additionalFileColumnTitles;
	bool filterAll = true, utf8 = false;
	if ( cfFound )
	{
		CConfigFile::CVar *v = cf.getVarPtr( "WordsPath" );
		if ( v )
		{
			wordsPath = v->asString();
			/*if ( (!wordsPath.empty()) && (wordsPath[wordsPath.size()-1]!='/') )
				wordsPath += '/';*/
		}
		v = cf.getVarPtr( "LanguageCode" );
		if ( v )
			languageCode = v->asString();
		v = cf.getVarPtr( "Utf8" );
		if ( v )
			utf8 = (v->asInt() == 1);
		v = cf.getVarPtr( "Filter" );
		if ( v )
		{
			filter = v->asString();
			filterAll = (filter == "*");
		}
		v = cf.getVarPtr( "AdditionalFiles" );
		if ( v )
		{
			for ( uint i=0; i!=v->size(); ++i )
				additionalFiles.push_back( v->asString( i ) );
			v = cf.getVarPtr( "AdditionalFileColumnTitles" );
			if ( v->size() != (sint)additionalFiles.size() )
			{
				sipwarning( "AdditionalFiles and AdditionalFileColumnTitles have different size, ignoring second one" );
				additionalFileColumnTitles.resize( v->size(), DefaultColTitle );
			}
			else
			{
				for ( uint i=0; i!=v->size(); ++i )
					additionalFileColumnTitles.push_back( v->asString( i ) );
			}
		}

	}
	if ( languageCode.empty() )
		languageCode = "en";

	// Load all found words files
	const string ext = ".txt";
	vector<string> fileList;
	CPath::getPathContent( wordsPath, false, false, true, fileList );
	for ( vector<string>::const_iterator ifl=fileList.begin(); ifl!=fileList.end(); ++ifl )
	{
		const string& filename = (*ifl);
		string::size_type p = string::npos;
		bool isAdditionalFile = false;

		// Test if filename is in additional file list
		uint iAdditionalFile;
		for ( iAdditionalFile=0; iAdditionalFile!=additionalFiles.size(); ++iAdditionalFile )
		{
			if ( (p = filename.find( additionalFiles[iAdditionalFile] )) != string::npos )
			{
				isAdditionalFile = true;
				break;
			}
		}

		// Or test if filename is a words_*.txt file
		string pattern = string("_words_") + languageCode + ext;
		if ( isAdditionalFile ||
			 ((p = filename.find( pattern )) != string::npos) )
		{
			// Skip if a filter is specified and does not match the current file
			if ( (!filterAll) && (filename.find( filter+pattern ) == string::npos) )
				continue;

			// Load file
			sipdebug( "WD: Loading %s", filename.c_str() );
			_FileList.push_back( filename );
			string::size_type origSize = filename.size() - ext.size();
			const string truncFilename = CFileA::getFilenameWithoutExtension( filename );
			const string wordType = isAdditionalFile ? "" : truncFilename.substr( 0, p - (origSize - truncFilename.size()) );
			const string colTitle = isAdditionalFile ? additionalFileColumnTitles[iAdditionalFile] : DefaultColTitle;

			// Load Unicode Excel words file
			STRING_MANAGER::TWorksheet worksheet;
			STRING_MANAGER::loadExcelSheet( filename, worksheet );
			uint ck, cw;
			if ( worksheet.findId( ck ) && worksheet.findCol( ucstring(colTitle), cw ) ) // => 
			{
				for ( std::vector<STRING_MANAGER::TWorksheet::TRow>::iterator ip = worksheet.begin(); ip!=worksheet.end(); ++ip )
				{
					if ( ip == worksheet.begin() ) // skip first row
						continue;
					STRING_MANAGER::TWorksheet::TRow& row = *ip;
#ifdef SIP_USE_UNICODE
					_Keys.push_back( row[ck].c_str() );
					basic_string<ucchar> word = row[cw];
#else
					_Keys.push_back( row[ck].toString() );
					string word = utf8 ? row[cw].toUtf8() : row[cw].toString();
#endif
					_Words.push_back( word );
				}
			}
			else
				sipwarning( "WD: %s ID or %s not found in %s", wordType.c_str(), colTitle.c_str(), filename.c_str() );
		}
	}

	if ( _Keys.empty() )
	{
		if ( wordsPath.empty() )
			sipwarning( "WD: WordsPath missing in config file %s", configFileName.c_str() );
		sipwarning( "WD: %s_words_%s.txt not found", filter.c_str(), languageCode.c_str() );
		return false;
	}
	else
		return true;
}


/*
 * Set the result vector with strings corresponding to the input string:
 * - If inputStr is partially or completely found in the keys, all the matching <key,words> are returned;
 * - If inputStr is partially or completely in the words, all the matching <key, words> are returned.
 * The following tags can modify the behaviour of the search algorithm:
 * - ^mystring returns mystring only if it is at the beginning of a key or word
 * - mystring$ returns mystring only if it is at the end of a key or word
 * All returned words are in UTF8.
 */
void CWordsDictionary::lookup( const CSStringA& inputStr, CVectorSStringA& resultVec ) const
{
	// Prepare search string
	if ( inputStr.empty() )
		return;

	CSStringA searchStr = inputStr;
	bool findAtBeginning = false, findAtEnd = false;
	if ( searchStr[0] == '^' )
	{
		searchStr = searchStr.substr( 1 );
		findAtBeginning = true;
	}
	if ( searchStr[searchStr.size()-1] == '$')
	{
		searchStr = searchStr.rightCrop( 1 );
		findAtEnd = true;
	}

	// Search
	const vector<string> &vec = reinterpret_cast<const vector<string>&>(_Keys);
//	for ( CVectorSStringA::const_iterator ivs=_Keys.begin(); ivs!=_Keys.end(); ++ivs )
	for ( vector<string>::const_iterator ivs=vec.begin(); ivs!=vec.end(); ++ivs )
	{
		const CSStringA& key = *ivs;
		string::size_type p;
		if ( (p = key.findNS( searchStr.c_str() )) != string::npos )
		{
			if ( ((!findAtBeginning) || (p==0)) && ((!findAtEnd) || (p==key.size()-searchStr.size())) )
				resultVec.push_back( makeResult( key, _Words[ivs-vec.begin()] ) );
		}
	}
	for ( CVectorSStringA::const_iterator ivs=_Words.begin(); ivs!=_Words.end(); ++ivs )
	{
		const CSStringA& word = *ivs;
		string::size_type p;
		if ( (p = word.findNS( searchStr.c_str() )) != string::npos )
		{
			if ( ((!findAtBeginning) || (p==0)) && ((!findAtEnd) || (p==word.size()-searchStr.size())) )
				resultVec.push_back( makeResult( _Keys[ivs-_Words.begin()], word ) );
		}
	}
}


/*
 * Set the result vector with the word(s) corresponding to the key
 */
void CWordsDictionary::exactLookupByKey( const CSStringA& key, CVectorSStringA& resultVec )
{
	// Search
	for ( CVectorSStringA::const_iterator ivs=_Keys.begin(); ivs!=_Keys.end(); ++ivs )
	{
		if ( key == *ivs )
			resultVec.push_back( _Words[ivs-_Keys.begin()] );
	}
}


/*
 * Make a result string
 */
inline CSStringA CWordsDictionary::makeResult( const CSStringA key, const CSStringA word )
{
	CSStringA res = key + CSStringA(": ");
	res += word;
	return res;
}


/*
 * Return the key contained in the provided string returned by lookup() (without extension)
 */
CSStringA CWordsDictionary::getWordsKey( const CSStringA& resultStr )
{
	return resultStr.splitTo( ':');
}

} // SIPBASE




