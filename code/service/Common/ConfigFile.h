#ifndef __CONFIG_FILE__
#define __CONFIG_FILE__

#include <map>
#include <fstream>

/* 여기서 std::string은 utf8 encoding된 문자렬이여야 한다. */
typedef std::multimap<std::string, std::string> SettingsMultiMap;
typedef std::map<std::string, SettingsMultiMap*> SettingsBySection;

class ConfigFile
{
public:
	SettingsBySection mSettings;

	void trim(std::string &, bool left = true, bool right = true);
	void load(std::ifstream &, const ucstring seperators = L":=", bool trimWhitespace = true);

	/*
	* @return utf8 string
	*/
	std::string getSetting(const std::string &key, const std::string &section, const std::string & defaultValue) const;
};
#endif
