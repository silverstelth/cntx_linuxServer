/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __I18N_H__
#define __I18N_H__

#include "types_sip.h"
#include "debug.h"
#include "file.h"

#include <string>
#include <map>
#include <algorithm>


namespace SIPBASE {


/**
 * Class for the internationalisation. It's a singleton pattern.
 * 
 * This class provide an easy way to localise all string. First you have to get all available language with \c getLanguageNames().
 * If you already know the number of the language (that is the index in the vector returns by \c getLanguagesNames()), you can
 * load the language file with \c load(). Now, you can get a localised string with his association with \c get().
 *
 *\code
	// get all language names (you must call this before calling load())
	CI18N::getLanguageNames ();
	// load the language 1 (french)
	CI18N::load (1);
	// display "Salut" that is the "hi" string in the selected language (french).
	sipinfo (CI18N::get("hi").c_str ());
	// display "rms est un master", the french version of the string
	sipinfo (CI18N::get("%s is a master").c_str (), "mrs");
 *\endcode
 *
 * If the string doesn't exist, it will be automatically added in all language files with a <Not Translated> mention.
 * If the language file doesn't exist, it'll be automatically create.
 *
 *	Update 26-02-2002 Boris Boucher
 *
 *	Language are now preferably handled via official language code.
 *	We use the ISO 639-1 code for language.
 *	Optionnaly, we can append a country code (ISO 3066) to differentiate
 *	between language flavor (eg chinese is ISO 639-1 zh, but come in
 *	traditionnal or simplified form. So we append the country code :
 *	zh-CN (china) for simplified, zh for traditionnal).
 *	
 *
 * \date 2000
 */
class CI18N
{
public:

	/// Control over text loading
	enum TLineFormat
	{
		// the text file is just loaded, no conversion or checks done on line delimiters
		LINE_FMT_NO_CARE,
		// the line delimiters are forced to LF (\n, code 0x0a)
		LINE_FMT_LF,
		// the line delimiters are forced to CRLF (\r\n , code 0x0d 0x0a)
		LINE_FMT_CRLF
	};

	/** Proxy interface for loading string file.
	 *	Implemente this interface in client code and set it inside I18N
	 *	in order to be able to do any work on string file before they
	 *	are read by CI18N.
	 *	This is used by Ryzom to merge the working string file with
	 *	the exploitation one when they are more recente.
	 */
	struct ILoadProxy
	{
		virtual void loadStringFile(const std::string &filename, ucstring &text) =0;
		virtual ~ILoadProxy() {};
	};

	/// Set the load proxy class. Proxy can be NULL to unregister.
	static void setLoadProxy(ILoadProxy *loadProxy);

	// Get the current load proxy
	static ILoadProxy *getLoadProxy() { return _LoadProxy; }


	/// Return a vector with all language available. The vector contains the name of the language.
	/// The index in the vector is used in \c load() function
	static const std::vector<ucstring> &getLanguageNames();
	/** Return a vector with all language code available.
	 *	Code are ISO 639-2 compliant.
	 *	As in getLanguageNames(), the index in the vector can be used to call load()
	 */
	static const std::vector<std::string> &getLanguageCodes();
	/// Load a language file depending of the language
//	static void load (uint32 lid);
	static void load (const std::string &languageCode);

	/** Load a language file from its filename
	  * \param filename name of the language file to load, with its extension	  
	  * \param reload The file is being reloaded so error message won't be issued for string that are overwritten
	  */
	static void loadFromFilename(const std::string &filename, bool reload);

	/// Returns the name of the language in english (french, english...)
	static ucstring getCurrentLanguageName ();

	/// Find a string in the selected language and return his association.
	static const ucstring &get (const std::string &label);

	// Test if a string has a translation in the selected language. 
	// NB : The empty string is considered to have a translation
	static bool			   hasTranslation(const std::string &label);


	/// Temporary, we don't have file system for now, so we do a tricky cheat. there s not check so be careful!
//	static void setPath (const char* str);

	/** Read the content of a file as a unicode text.
	 *	The method support 16 bits or 8bits utf-8 tagged files.
	 *	8 bits UTF-8 encoding can be recognized by a non official header :
	 *	EF,BB,BF.
	 *	16 bits encoding can be reconized by the official header :
	 *	FF, FE, which can be reversed if the data are MSB first.
	 *	
	 *	Optionnaly, you can force the reader to consider the file as
	 *	UTF-8 encoded.
	 *	Optionnaly, you can ask the reader to interpret #include commands.
	 */
	static void readTextFile(const std::string &filename, 
								ucstring &result, bool forceUtf8 = false, 
								bool fileLookup = true, 
								bool preprocess = false,
								TLineFormat lineFmt = LINE_FMT_NO_CARE);

	static void readTextFile(const std::basic_string<ucchar> &filename,
								ucstring &result, bool forceUtf8 = false, 
								bool fileLookup = true, 
								bool preprocess = false,
								TLineFormat lineFmt = LINE_FMT_NO_CARE);

	/** Read the content of a buffer as a unicode text.
	 *	This is to read preloaded unicode files.
	 *	The method support 16 bits or 8bits utf-8 tagged buffer.
	 *	8 bits UTF-8 encofing can be reconized by a non official header :
	 *	EF,BB, BF.
	 *	16 bits encoding can be reconized by the official header :
	 *	FF, FE, witch can be reversed if the data are MSB first.
	 *	
	 *	Optionnaly, you can force the reader to consider the file as
	 *	UTF-8 encoded.
	 */
	static void readTextBuffer(uint8 *buffer, uint size, ucstring &result, bool forceUtf8 = false);

	/** Remove any C style comment from the passed string.
	 */
	static void remove_C_Comment(ucstring &commentedString);

	/** Encode a unicode string into a string using UTF-8 encoding.
	*/
	static std::string encodeUTF8(const ucstring &str);

	/** Write a unicode text file using unicode 16 or UTF-8 encoding.
	 */
	static void writeTextFile(const std::string filename, const ucstring &content, bool utf8 = true);
	static void writeTextFile(const std::basic_string<ucchar> filename, const ucstring &content, bool utf8 = true);

	static ucstring makeMarkedString(ucchar openMark, ucchar closeMark, const ucstring &text);

	//@{
	//\name Parsing utility
	/** Skip the white space.
	 *	You can optionnaly pass a ucstring pointer to receive any comments string that build the
	 *	white space.
	 *	This is usefull if you whant to keep the comments.
	 *	NB : comments are appended to the comments string.
	 */
	static void		skipWhiteSpace		(ucstring::const_iterator &it, ucstring::const_iterator &last, ucstring *storeComments = NULL);
	/// Parse a label
	static bool		parseLabel			(ucstring::const_iterator &it, ucstring::const_iterator &last, std::string &label);
	/// Parse a marked string. NB : usualy, we use [ and ] as string delimiters in translation files.
	static bool		parseMarkedString	(ucchar openMark, ucchar closeMark, ucstring::const_iterator &it, ucstring::const_iterator &last, ucstring &result);
	//@}

	//@{
	//\name Hash code tools. 
	// Generate a hash value for a given string
	static uint64	makeHash(const ucstring &str);
	// convert a hash value to a readable string 
	static std::string hashToString(uint64 hash);
	// convert a readable string into a hash value.
	static uint64 stringToHash(const std::string &str);
	// fast convert a hash value to a ucstring
	static void	hashToUCString(uint64 hash, ucstring &dst);
	//@}

private:

	typedef std::map<std::string, ucstring>						StrMapContainer;

	static ILoadProxy							*_LoadProxy;

	static StrMapContainer										 _StrMap;
	static bool													 _StrMapLoaded;

	static const std::string									 _LanguageCodes[];
	static const uint											_NbLanguages;

	static bool													 _LanguagesNamesLoaded;

	static sint32												 _SelectedLanguage;
	static const ucstring										_NotTranslatedValue;
private:

	static bool loadFileIntoMap(const std::string &filename, StrMapContainer &dest);
};




} // SIPBASE


#endif // __I18N_H__

/* End of i18n.h */





















