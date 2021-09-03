#ifdef SIP_OS_WINDOWS
#	include <Windows.h>
#endif

#include "LoadOutData.h"
#include "net/service.h"

using namespace std;
using namespace SIPBASE;
using namespace SIPNET;

//#define		KEY_SHEET			L"SHEET"			// Sheet Key
//
//int GetColumnPos(const uint16 * section, const uint16 * key, ucstring ucOutIniFile, xmlNodePtr pSheet)
//{
//	uint16	chID[100];	memset(chID, 0, sizeof(chID));
//	uint32 dwSize = GetPrivateProfileString(section, key, L"", chID, 100, ucOutIniFile.c_str());
//	if (dwSize == 0)		return -1;
//	int nIDPos = ExcelGetColumnPos(pSheet, chID);
//	return nIDPos;
//}
// 
//xmlNodePtr GetSheet(const CIXml *xmlInfo, const uint16 * section, ucstring ucOutIniFile)
//{
//	uint16	chSheet[100];	memset(chSheet, 0, sizeof(chSheet));
//	DWORD dwSize = GetPrivateProfileString(section, KEY_SHEET, L"", chSheet, 100, ucOutIniFile.c_str());
//	if (dwSize == 0)
//		return NULL;
//	return ExcelGetSheet(xmlInfo, chSheet);
//}

