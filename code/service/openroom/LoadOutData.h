#pragma once

#include "misc/types_sip.h"
#include "misc/i_xml.h"
#include "../Common/ExcelXML.h"

#include "../../common/common.h"
#include "../Common/Common.h"

#define WIDE(x) L##x

/////////////////////////////////////////////////////////////////////////////////////
#define	GETSHEET(sheet)	\
	xmlNodePtr pElmWorkSheet = GetSheet(xmlInfo, WIDE(#sheet), ucOutIniFile);\
	if (pElmWorkSheet == NULL)	\
	return false;

#define	GETCOLMUNPOS(sheet, key)	\
	int n##key		= GetColumnPos(WIDE(#sheet), WIDE(#key), ucOutIniFile, pElmWorkSheet);	\
	if (n##key < 0)	\
	return false;

#define	START_CELLANALYSE()	\
	xmlNodePtr pCurRow = ExcelGetFirstDataRow(pElmWorkSheet);	\
	while(pCurRow)	\
{	

#define	END_SHEETANALYSE()	\
	pCurRow = ExcelGetNextDataRow(pCurRow);	\
}

#define	START_SHEETANALYSE_1(sheet, key)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key);				\
	START_CELLANALYSE();					\
	string	str##key = ExcelGetCellValue(pCurRow, n##key);	

#define	START_SHEETANALYSE_2(sheet, key1, key2)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);				\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	

#define	START_SHEETANALYSE_3(sheet, key1, key2, key3)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);	GETCOLMUNPOS(sheet, key3);			\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	\
	string	str##key3 = ExcelGetCellValue(pCurRow, n##key3);	

#define	START_SHEETANALYSE_4(sheet, key1, key2, key3, key4)	\
	GETSHEET(sheet);\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);	GETCOLMUNPOS(sheet, key3);	GETCOLMUNPOS(sheet, key4);	\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	\
	string	str##key3 = ExcelGetCellValue(pCurRow, n##key3);	\
	string	str##key4 = ExcelGetCellValue(pCurRow, n##key4);	

#define	START_SHEETANALYSE_5(sheet, key1, key2, key3, key4, key5)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);	GETCOLMUNPOS(sheet, key3);	GETCOLMUNPOS(sheet, key4);	GETCOLMUNPOS(sheet, key5);	\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	\
	string	str##key3 = ExcelGetCellValue(pCurRow, n##key3);	\
	string	str##key4 = ExcelGetCellValue(pCurRow, n##key4);	\
	string	str##key5 = ExcelGetCellValue(pCurRow, n##key5);	

#define	START_SHEETANALYSE_6(sheet, key1, key2, key3, key4, key5, key6)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);	GETCOLMUNPOS(sheet, key3);	GETCOLMUNPOS(sheet, key4);	GETCOLMUNPOS(sheet, key5);	\
	GETCOLMUNPOS(sheet, key6);	\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	\
	string	str##key3 = ExcelGetCellValue(pCurRow, n##key3);	\
	string	str##key4 = ExcelGetCellValue(pCurRow, n##key4);	\
	string	str##key5 = ExcelGetCellValue(pCurRow, n##key5);	\
	string	str##key6 = ExcelGetCellValue(pCurRow, n##key6);	

#define	START_SHEETANALYSE_7(sheet, key1, key2, key3, key4, key5, key6, key7)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);	GETCOLMUNPOS(sheet, key3);	GETCOLMUNPOS(sheet, key4);	GETCOLMUNPOS(sheet, key5);	\
	GETCOLMUNPOS(sheet, key6);	GETCOLMUNPOS(sheet, key7);	\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	\
	string	str##key3 = ExcelGetCellValue(pCurRow, n##key3);	\
	string	str##key4 = ExcelGetCellValue(pCurRow, n##key4);	\
	string	str##key5 = ExcelGetCellValue(pCurRow, n##key5);	\
	string	str##key6 = ExcelGetCellValue(pCurRow, n##key6);	\
	string	str##key7 = ExcelGetCellValue(pCurRow, n##key7);	

#define	START_SHEETANALYSE_8(sheet, key1, key2, key3, key4, key5, key6, key7, key8)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);	GETCOLMUNPOS(sheet, key3);	GETCOLMUNPOS(sheet, key4);	GETCOLMUNPOS(sheet, key5);	\
	GETCOLMUNPOS(sheet, key6);	GETCOLMUNPOS(sheet, key7);	GETCOLMUNPOS(sheet, key8);	\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	\
	string	str##key3 = ExcelGetCellValue(pCurRow, n##key3);	\
	string	str##key4 = ExcelGetCellValue(pCurRow, n##key4);	\
	string	str##key5 = ExcelGetCellValue(pCurRow, n##key5);	\
	string	str##key6 = ExcelGetCellValue(pCurRow, n##key6);	\
	string	str##key7 = ExcelGetCellValue(pCurRow, n##key7);	\
	string	str##key8 = ExcelGetCellValue(pCurRow, n##key8);	

#define	START_SHEETANALYSE_9(sheet, key1, key2, key3, key4, key5, key6, key7, key8, key9)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);	GETCOLMUNPOS(sheet, key3);	GETCOLMUNPOS(sheet, key4);	GETCOLMUNPOS(sheet, key5);	\
	GETCOLMUNPOS(sheet, key6);	GETCOLMUNPOS(sheet, key7);	GETCOLMUNPOS(sheet, key8);	GETCOLMUNPOS(sheet, key9);	\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	\
	string	str##key3 = ExcelGetCellValue(pCurRow, n##key3);	\
	string	str##key4 = ExcelGetCellValue(pCurRow, n##key4);	\
	string	str##key5 = ExcelGetCellValue(pCurRow, n##key5);	\
	string	str##key6 = ExcelGetCellValue(pCurRow, n##key6);	\
	string	str##key7 = ExcelGetCellValue(pCurRow, n##key7);	\
	string	str##key8 = ExcelGetCellValue(pCurRow, n##key8);	\
	string	str##key9 = ExcelGetCellValue(pCurRow, n##key9);	

#define	START_SHEETANALYSE_10(sheet, key1, key2, key3, key4, key5, key6, key7, key8, key9, key10)	\
	GETSHEET(sheet);						\
	GETCOLMUNPOS(sheet, key1);	GETCOLMUNPOS(sheet, key2);	GETCOLMUNPOS(sheet, key3);	GETCOLMUNPOS(sheet, key4);	GETCOLMUNPOS(sheet, key5);	\
	GETCOLMUNPOS(sheet, key6);	GETCOLMUNPOS(sheet, key7);	GETCOLMUNPOS(sheet, key8);	GETCOLMUNPOS(sheet, key9);	GETCOLMUNPOS(sheet, key10);	\
	START_CELLANALYSE();					\
	string	str##key1 = ExcelGetCellValue(pCurRow, n##key1);	\
	string	str##key2 = ExcelGetCellValue(pCurRow, n##key2);	\
	string	str##key3 = ExcelGetCellValue(pCurRow, n##key3);	\
	string	str##key4 = ExcelGetCellValue(pCurRow, n##key4);	\
	string	str##key5 = ExcelGetCellValue(pCurRow, n##key5);	\
	string	str##key6 = ExcelGetCellValue(pCurRow, n##key6);	\
	string	str##key7 = ExcelGetCellValue(pCurRow, n##key7);	\
	string	str##key8 = ExcelGetCellValue(pCurRow, n##key8);	\
	string	str##key9 = ExcelGetCellValue(pCurRow, n##key9);	\
	string	str##key10 = ExcelGetCellValue(pCurRow, n##key10);	

