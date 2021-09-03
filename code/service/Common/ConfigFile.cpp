
#include "misc/debug.h"
#include "misc/ucstring.h"
#include "ConfigFile.h"

void ConfigFile::trim(std::string &str, bool left, bool right)
{
	static const std::string delims = " \t\r\n";
	if (right)
		str.erase(str.find_last_not_of(delims) + 1);
	if (left)
		str.erase(0, str.find_first_not_of(delims));
}

void ConfigFile::load(std::ifstream &fileStream, const ucstring separators, bool trimWhitespace)
{
	SettingsMultiMap* currentSettings;

	std::string strLine, currentSection, optName, optVal;
	getline(fileStream, strLine);
	trim(strLine);

	ucstring ucCurrentSection, ucOptName, ucOptVal;
	ucstring ucStrLine = ucstring::makeFromUtf8(strLine);
	while (!fileStream.eof())
	{
		if (ucStrLine.length() > 0 && ucStrLine.at(0) != '#' && ucStrLine.at(0) != '@')
		{
			if (ucStrLine.at(0) == '[' && ucStrLine.at(ucStrLine.length() - 1) == ']')
			{
				ucCurrentSection = ucStrLine.substr(1, ucStrLine.length() - 2);
				currentSection = ucCurrentSection.toUtf8();
				SettingsBySection::const_iterator seci = mSettings.find(currentSection);
				if (seci == mSettings.end())
				{
					currentSettings = new SettingsMultiMap();
					mSettings[currentSection] = currentSettings;
				}
				else
				{
					currentSettings = seci->second;
				}
			}
			else
			{
				ucstring::size_type separator_pos = ucStrLine.find_first_of(separators, 0);
				if (separator_pos != ucstring::npos)
				{
					ucOptName = ucStrLine.substr(0, separator_pos);
					optName = ucOptName.toUtf8();

					ucstring::size_type nonseparator_pos = ucStrLine.find_first_not_of(separators, separator_pos);
					ucOptVal = (nonseparator_pos == ucstring::npos) ? L"" : ucStrLine.substr(nonseparator_pos);
					optVal = ucOptVal.toUtf8();
					if (trimWhitespace)
					{
						trim(optVal);
						trim(optName);
					}
					currentSettings->insert(SettingsMultiMap::value_type(optName, optVal));
				}
			}
		}
		getline(fileStream, strLine);

		trim(strLine);
		ucStrLine = ucstring::makeFromUtf8(strLine);
	}
}

std::string ConfigFile::getSetting(const std::string &key, const std::string &section, const std::string & defaultValue) const
{
	SettingsBySection::const_iterator seci = mSettings.find(section);
	if (seci == mSettings.end())
	{
		return defaultValue;
	}
	else
	{
		SettingsMultiMap::const_iterator i = seci->second->find(key);
		if (i == seci->second->end())
		{
			return defaultValue;
		}
		else
		{
			return i->second;
		}
	}

	return "";
}
