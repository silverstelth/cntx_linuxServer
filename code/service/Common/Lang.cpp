
#ifdef SIP_OS_WINDOWS
#	include <Windows.h>
#endif

#ifdef SIP_OS_UNIX
#	include <fstream>
#endif

#include "Lang.h"
#include "ConfigFile.h"
#include "misc/types_sip.h"
#include "misc/mem_stream.h"
#include "../Common/ExcelXML.h"
#include "ConfigFile.h"

using namespace std;
using namespace SIPBASE;

////////////////////////////////////////////////////////////////////

#define		KEY_SHEET			L"SHEET"			// Sheet Key

uint32 GetINIfileString(const ucchar *section, const ucchar *key, ucchar* value, int length, ucstring ucOutIniFile)
{
	uint32 dwSize;
#ifdef SIP_OS_WINDOWS
	dwSize = GetPrivateProfileString(section, key, L"", value, length, ucOutIniFile.c_str());
#else
	ucstring uStrSection(section), uStrKey(key);
	std::string strSection = uStrSection.toUtf8();
	std::string strKey = uStrKey.toUtf8();

	ConfigFile confFile;
	ifstream file(ucOutIniFile.toUtf8());
	confFile.load(file);
	std::string strValue = confFile.getSetting(strKey, strSection, "");
	ucstring ucStrValue = ucstring::makeFromUtf8(strValue);
	wcscpy(value, ucStrValue.c_str());
	
	dwSize = ucStrValue.length();
#endif
	return dwSize;
}

int GetColumnPos(const ucchar *section, const ucchar * key, ucstring ucOutIniFile, xmlNodePtr pSheet)
{
	ucchar	chID[200] = L"";
	uint32 dwSize = GetINIfileString(section, key, chID, 100, ucOutIniFile);
	if (dwSize == 0)		return -1;
	int nIDPos = ExcelGetColumnPos(pSheet, chID);
	return nIDPos;
}

xmlNodePtr GetSheet(const CIXml *xmlInfo, const ucchar * section, ucstring ucOutIniFile)
{
	ucchar	chSheet[200] = L"";
	uint32 dwSize = GetINIfileString(section, KEY_SHEET, chSheet, 100, ucOutIniFile);
	if (dwSize == 0)
		return NULL;
	return ExcelGetSheet(xmlInfo, chSheet);
}

MAP_OUT_LANG			map_OUT_LANG;
bool	LoadLangData(ucstring ExePath)
{
	CIXml	xmlInfo;

	ucstring	FullXmlPath = ExePath + L"ServerLang.xml";
	ucstring	FullIniPath = ExePath + L"ServerLang.ini";
	if (!xmlInfo.init(FullXmlPath))
		return false;

	map_OUT_LANG.clear();

	xmlNodePtr pElmWorkSheet = GetSheet(&xmlInfo, L"SERVERLANG", FullIniPath);
	if (pElmWorkSheet == NULL)
		return false;

	int nID		= GetColumnPos(L"SERVERLANG", L"ID", FullIniPath, pElmWorkSheet);
	if (nID < 0)
		return false;
	int nText		= GetColumnPos(L"SERVERLANG", L"Text", FullIniPath, pElmWorkSheet);
	if (nText < 0)
		return false;

	xmlNodePtr pCurRow = ExcelGetFirstDataRow(pElmWorkSheet);
	while (pCurRow)
	{
		string	strID = ExcelGetCellValue(pCurRow, nID);
		string	strText = ExcelGetCellValue(pCurRow, nText);	

		if (strID != "")
		{
			OUT_LANG		newType;
			newType.ID = strtoul (strID.c_str(), NULL, 10);//atoui(strID.c_str());
			newType.Text = ucstring::makeFromUtf8(strText);
			map_OUT_LANG.insert( make_pair(newType.ID, newType));
		}

		pCurRow = ExcelGetNextDataRow(pCurRow);
	}

	return true;
}

ucstring	GetLangText(uint32 _id)
{
	MAP_OUT_LANG::iterator it = map_OUT_LANG.find(_id);
	if (it == map_OUT_LANG.end())
	{
		sipwarning("GetLangText failed. id=%d", _id);
		return L"";
	}
	return it->second.Text;
}


//====================================================================
// ManagerService에서 Xianbao시에 효과모델자료
//====================================================================
//std::map<uint32, string>	map_XianbaoEffectModelName;
//bool LoadXianbaoEffectModelName(ucstring ExePath)
//{
//	CIXml	xmlInfo;
//
//	ucstring	FullXmlPath = ExePath + L"ServerInfo.xml";
//	ucstring	FullIniPath = ExePath + L"OutData.ini";
//	if (!xmlInfo.init(FullXmlPath))
//		return false;
//
//	map_XianbaoEffectModelName.clear();
//
//	xmlNodePtr pWorkSheet = GetSheet(&xmlInfo, L"XIANBAOMODEL", FullIniPath);
//	if (pWorkSheet == NULL)
//		return false;
//
//	int nItemIDRow		= GetColumnPos(L"XIANBAOMODEL", L"ItemID", FullIniPath, pWorkSheet);
//	if (nItemIDRow < 0)
//		return false;
//	int nModelNameRow	= GetColumnPos(L"XIANBAOMODEL", L"ModelName", FullIniPath, pWorkSheet);
//	if (nModelNameRow < 0)
//		return false;	
//
//	xmlNodePtr pCurRow = ExcelGetFirstDataRow(pWorkSheet);
//	while (pCurRow)
//	{
//		string	strItemID = ExcelGetCellValue(pCurRow, nItemIDRow);
//		string	strModelName = ExcelGetCellValue(pCurRow, nModelNameRow);	
//
//		if (strModelName != "")
//		{
//			uint32	ItemID = strtoul (strItemID.c_str(), NULL, 10);//atoui(strID.c_str());
//			int len = strModelName.find('%');
//			strModelName = strModelName.substr(0, len);
//			map_XianbaoEffectModelName.insert( make_pair(ItemID, strModelName));
//		}
//
//		pCurRow = ExcelGetNextDataRow(pCurRow);
//	}
//
//	return true;
//}
//
//string GetModelEffectName(uint32 ItemID)
//{
//	std::map<uint32, string>::iterator it = map_XianbaoEffectModelName.find(ItemID);
//	if (it == map_XianbaoEffectModelName.end())
//	{
//		sipwarning("it == map_XianbaoEffectModelName.end(). ItemID=%d", ItemID);
//		return "";
//	}
//	return it->second;
//}

//====================================================================
// ManagerService에서 축복카드ID자료
//====================================================================
std::map<uint32, uint8>	map_BlessCardIDs;
bool LoadBlessCardID(ucstring ExePath)
{
	CIXml	xmlInfo;

	ucstring	FullXmlPath = ExePath + L"ServerInfo.xml";
	ucstring	FullIniPath = ExePath + L"OutData.ini";
	if (!xmlInfo.init(FullXmlPath))
		return false;

	map_BlessCardIDs.clear();

	xmlNodePtr pWorkSheet = GetSheet(&xmlInfo, L"LUCKDATA", FullIniPath);
	if (pWorkSheet == NULL)
		return false;

	int nLuckIDRow		= GetColumnPos(L"LUCKDATA", L"LuckId", FullIniPath, pWorkSheet);
	if (nLuckIDRow < 0)
		return false;
	int nBlessCardIDRow	= GetColumnPos(L"LUCKDATA", L"BlessCardId", FullIniPath, pWorkSheet);
	if (nBlessCardIDRow < 0)
		return false;

	xmlNodePtr pCurRow = ExcelGetFirstDataRow(pWorkSheet);
	while (pCurRow)
	{
		string	strLuckID = ExcelGetCellValue(pCurRow, nLuckIDRow);
		string	strBlessCardID = ExcelGetCellValue(pCurRow, nBlessCardIDRow);	

		if (strBlessCardID != "")
		{
			uint32	LuckID = strtoul (strLuckID.c_str(), NULL, 10);//atoui(strID.c_str());
			uint8	BlessCardID = (uint8) strtoul (strBlessCardID.c_str(), NULL, 10);//atoui(strID.c_str());
			map_BlessCardIDs.insert( make_pair(LuckID, BlessCardID));
		}

		pCurRow = ExcelGetNextDataRow(pCurRow);
	}

	return true;
}

uint8 GetBlessCardID(uint32 LuckID)
{
	std::map<uint32, uint8>::iterator it = map_BlessCardIDs.find(LuckID);
	if (it == map_BlessCardIDs.end())
	{
		sipwarning("it == map_BlessCardIDs.end(). LuckID=%d", LuckID);
		return 0;
	}
	return it->second;
}

//====================================================================
// ManagerService에서 예약행사시의 TaocanItem정보
//====================================================================
std::map<uint32, OUT_TAOCAN_FOR_AUTOPLAY3>	map_Autoplay3TaocanItems;
bool LoadTaocanForAutoplay3(ucstring ExePath)
{
	CIXml	xmlInfo;

	ucstring	FullXmlPath = ExePath + L"ServerInfo.xml";
	ucstring	FullIniPath = ExePath + L"OutData.ini";
	if (!xmlInfo.init(FullXmlPath))
		return false;

	map_Autoplay3TaocanItems.clear();

	char		buf[512];
	{
		xmlNodePtr pWorkSheet = GetSheet(&xmlInfo, L"TAOCANITEM_XIANBAO", FullIniPath);
		if (pWorkSheet == NULL)
			return false;

		int nItemIDRow		= GetColumnPos(L"TAOCANITEM_XIANBAO", L"ItemID", FullIniPath, pWorkSheet);
		if (nItemIDRow < 0)
			return false;
		int nItemSubtypeRow	= GetColumnPos(L"TAOCANITEM_XIANBAO", L"ItemSubtype", FullIniPath, pWorkSheet);
		if (nItemSubtypeRow < 0)
			return false;
		int nItemListRow	= GetColumnPos(L"TAOCANITEM_XIANBAO", L"ItemList", FullIniPath, pWorkSheet);
		if (nItemListRow < 0)
			return false;
		int nPrayTextRow	= GetColumnPos(L"TAOCANITEM_XIANBAO", L"PrayText", FullIniPath, pWorkSheet);
		if (nPrayTextRow < 0)
			return false;

		xmlNodePtr pCurRow = ExcelGetFirstDataRow(pWorkSheet);
		while (pCurRow)
		{
			string	strItemID = ExcelGetCellValue(pCurRow, nItemIDRow);
			string	strItemSubtype = ExcelGetCellValue(pCurRow, nItemSubtypeRow);
			string	strItemList = ExcelGetCellValue(pCurRow, nItemListRow);
			string	strPrayText = ExcelGetCellValue(pCurRow, nPrayTextRow);

			if (strItemID != "" && strItemSubtype != "" && strItemList != "" && strPrayText != "")
			{
				uint32	ItemID = strtoul (strItemID.c_str(), NULL, 10);
				uint32	ItemSubtype = strtoul (strItemSubtype.c_str(), NULL, 10);

				OUT_TAOCAN_FOR_AUTOPLAY3	data;
				data.TaocanType = ItemSubtype;

				strcpy(buf, strItemList.c_str());
				int n = 0;
				char	*token = strtok(buf, "*");
				while (token != NULL)
				{
					data.ItemIDs[n] = atoui(token);
					token = strtok(NULL, "*");
					n ++;
				}

				data.PrayText = ucstring("\x02") + ucstring(strPrayText) + ucstring("\x03");

				map_Autoplay3TaocanItems.insert(make_pair(ItemID, data));
			}

			pCurRow = ExcelGetNextDataRow(pCurRow);
		}
	}

	{
		xmlNodePtr pWorkSheet = GetSheet(&xmlInfo, L"TAOCANITEM_YISI", FullIniPath);
		if (pWorkSheet == NULL)
			return false;

		int nItemIDRow		= GetColumnPos(L"TAOCANITEM_YISI", L"ItemID", FullIniPath, pWorkSheet);
		if (nItemIDRow < 0)
			return false;
		int nItemSubtypeRow	= GetColumnPos(L"TAOCANITEM_YISI", L"ItemSubtype", FullIniPath, pWorkSheet);
		if (nItemSubtypeRow < 0)
			return false;
		int nItemListRow	= GetColumnPos(L"TAOCANITEM_YISI", L"ItemList", FullIniPath, pWorkSheet);
		if (nItemListRow < 0)
			return false;
		int nPrayTextRow	= GetColumnPos(L"TAOCANITEM_YISI", L"PrayText", FullIniPath, pWorkSheet);
		if (nPrayTextRow < 0)
			return false;

		xmlNodePtr pCurRow = ExcelGetFirstDataRow(pWorkSheet);
		while (pCurRow)
		{
			string	strItemID = ExcelGetCellValue(pCurRow, nItemIDRow);
			string	strItemSubtype = ExcelGetCellValue(pCurRow, nItemSubtypeRow);
			string	strItemList = ExcelGetCellValue(pCurRow, nItemListRow);
			string	strPrayText = ExcelGetCellValue(pCurRow, nPrayTextRow);

			if (strItemID != "" && strItemSubtype != "" && strItemList != "" && strPrayText != "")
			{
				uint32	ItemID = strtoul (strItemID.c_str(), NULL, 10);
				uint32	ItemSubtype = strtoul (strItemSubtype.c_str(), NULL, 10);

				OUT_TAOCAN_FOR_AUTOPLAY3	data;
				data.TaocanType = ItemSubtype;

				strcpy(buf, strItemList.c_str());
				int n = 0;
				char	*token = strtok(buf, "*");
				while (token != NULL)
				{
					data.ItemIDs[n] = atoui(token);
					token = strtok(NULL, "*");
					n ++;
				}

				data.PrayText = ucstring("\x02") + ucstring(strPrayText) + ucstring("\x03");

				map_Autoplay3TaocanItems.insert(make_pair(ItemID, data));
			}

			pCurRow = ExcelGetNextDataRow(pCurRow);
		}
	}

	return true;
}

OUT_TAOCAN_FOR_AUTOPLAY3* GetAutoplay3TaocanItemInfo(uint32 ItemID)
{
	std::map<uint32, OUT_TAOCAN_FOR_AUTOPLAY3>::iterator it = map_Autoplay3TaocanItems.find(ItemID);
	if (it == map_Autoplay3TaocanItems.end())
	{
		sipwarning("it == map_Autoplay3TaocanItems.end(). ItemID=%d", ItemID);
		return 0;
	}
	return &(it->second);
}
