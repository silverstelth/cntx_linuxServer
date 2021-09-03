/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __WORDS_DICTIONARY_H__
#define __WORDS_DICTIONARY_H__

#include "types_sip.h"
#include "sstring.h"

namespace SIPBASE {

/**
 * Words dictionary: allows to search for keys and words in <type>_words_<language>.txt
 * Unicode files. All searches are case not-sensitive.
 *
 * \date 2003
 */
class CWordsDictionary
{
	SIP_INSTANCE_COUNTER_DECL(CWordsDictionary);
public:

	/// Constructor
	CWordsDictionary();

	/** Load the config file and the related words files. Return false in case of failure.
	 * Config file variables:
	 * - WordsPath: where to find <filter>_words_<languageCode>.txt
	 * - LanguageCode: language code (ex: en for English)
	 * - Utf8: results are in UTF8, otherwise in ANSI string
	 * - Filter: "*" for all files (default) or a name (ex: "item").
	 * - AdditionalFiles/AdditionalFileColumnTitles
	 */
	bool			init(const std::string& configFileName = "words_dic.cfg");

	/**
	 * Set the result vector with strings corresponding to the input string:
	 * - If inputStr is partially or completely found in the keys, all the matching <key,words> are returned;
	 * - If inputStr is partially or completely in the words, all the matching <key, words> are returned.
	 * The following tags can modify the behaviour of the search algorithm:
	 * - ^mystring returns mystring only if it is at the beginning of a key or word
	 * - mystring$ returns mystring only if it is at the end of a key or word
	 * All returned words are in UTF8 string or ANSI string, depending of the config file.
	 */
	void			lookup( const CSStringA& inputStr, CVectorSStringA& resultVec )const;

	/// Set the result vector with the word(s) corresponding to the key
	void			exactLookupByKey( const CSStringA& key, CVectorSStringA& resultVec );

	/// Return the key contained in the provided string returned by lookup() (without extension)
	CSStringA		getWordsKey( const CSStringA& resultStr );

	// Accessors
	const CVectorSStringA& getKeys() { return _Keys; }
	const CVectorSStringA& getWords() { return _Words; }

	/// Return the list of input file loaded at init
	const std::vector<std::string>& getFileList() const { return _FileList; }

protected:

	/// Make a result string
	static CSStringA		makeResult( const CSStringA key, const CSStringA word );

private:

	/// Keys (same indices as in _Words)
	CVectorSStringA				_Keys;

	/// Words (same indices as in _Keys)
	CVectorSStringA				_Words;

	/// Input file list
	std::vector<std::string>	_FileList;
};


} // SIPBASE


#endif // __WORDS_DICTIONARY_H__

/* End of words_dictionary.h */
