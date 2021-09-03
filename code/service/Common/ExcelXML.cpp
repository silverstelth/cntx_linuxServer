
#include "ExcelXML.h"
#include "misc/mem_stream.h"
using namespace std;
using namespace SIPBASE;

#define XML_ELM_WORKSHEET			"Worksheet"
#define XML_ELM_TABLE				"Table"
#define XML_ELM_ROW					"Row"
#define XML_ELM_CELL				"Cell"
#define XML_ELM_DATA				"Data"
#define XML_ELM_SSDATA				"ss:Data"
#define XML_ATT_NAME				"Name"
#define XML_ATT_IDX					"Index"

xmlNodePtr	ExcelGetSheet(const CIXml *xmlInfo, const ucchar* chSheet)
{
	string strSheet = ucstring(chSheet).toUtf8();
	xmlNodePtr pRootElement = xmlInfo->getRootNode();
	xmlNodePtr pElmWorkSheet = CIXml::getFirstChildNode(pRootElement, XML_ELM_WORKSHEET);
	while ( pElmWorkSheet != NULL )
	{
		string strCurSheet;
		CIXml::getPropertyString(strCurSheet, pElmWorkSheet, XML_ATT_NAME);
		if (strCurSheet == strSheet)
			return pElmWorkSheet;
		pElmWorkSheet = CIXml::getNextChildNode(pElmWorkSheet, XML_ELM_WORKSHEET);
	}
	return NULL;
}

xmlNodePtr ExcelGetSectionRow(xmlNodePtr pSheet)
{
	xmlNodePtr pElmTable = CIXml::getFirstChildNode(pSheet, XML_ELM_TABLE);
	if (pElmTable == NULL)
		return NULL;

	xmlNodePtr pElmRow = CIXml::getFirstChildNode(pElmTable, XML_ELM_ROW);
	if (pElmRow == NULL)
		return NULL;
	pElmRow = CIXml::getNextChildNode(pElmRow, XML_ELM_ROW);
	return pElmRow;
/*
	while ( pElmRow != NULL )
	{
		xmlNodePtr pElmCell = CIXml::getFirstChildNode(pElmRow, XML_ELM_CELL);
		while ( pElmCell != NULL )
		{
			xmlNodePtr pElmData = CIXml::getFirstChildNode(pElmCell, XML_ELM_DATA);
			if (pElmData)
				return pElmRow;
			pElmCell = CIXml::getNextChildNode(pElmCell, XML_ELM_CELL);
		}
		pElmRow = CIXml::getNextChildNode(pElmRow, XML_ELM_ROW);
	}
	*/
	return NULL;
}

int ExcelGetColumnPos(xmlNodePtr pSheet, const ucchar *chColumn)
{
	string strContent = ucstring(chColumn).toUtf8();
	xmlNodePtr pSection = ExcelGetSectionRow(pSheet);

	xmlNodePtr pElmCell = CIXml::getFirstChildNode(pSection, XML_ELM_CELL);
	int nPos = 0;
	while ( pElmCell != NULL )
	{
		xmlNodePtr pElmData = CIXml::getFirstChildNode(pElmCell, XML_ELM_DATA);
		if (pElmData)
		{
			string strCurContent;
			CIXml::getContentString(strCurContent, pElmData);
			if (strCurContent == strContent)
				return nPos;
		}
		pElmCell = CIXml::getNextChildNode(pElmCell, XML_ELM_CELL);
		nPos ++;
	}
	return -1;
}

xmlNodePtr ExcelGetFirstDataRow(xmlNodePtr pSheet)
{
	xmlNodePtr pSection = ExcelGetSectionRow(pSheet);
	return CIXml::getNextChildNode(pSection, XML_ELM_ROW);
}

xmlNodePtr ExcelGetNextDataRow(xmlNodePtr pCur)
{
	return CIXml::getNextChildNode(pCur, XML_ELM_ROW);
}

string	ExcelGetCellValue(xmlNodePtr pRow, int nPos)
{
	xmlNodePtr pElmCell = CIXml::getFirstChildNode(pRow, XML_ELM_CELL);
	int nCurPos = 0;
	while ( pElmCell != NULL )
	{
		if (nCurPos == nPos)
		{
			xmlNodePtr pElmData = CIXml::getFirstChildNode(pElmCell, XML_ELM_DATA);
			if (pElmData)
			{
				string strCurContent;
				CIXml::getContentString(strCurContent, pElmData);
				return strCurContent;
			}
		}
		pElmCell = CIXml::getNextChildNode(pElmCell, XML_ELM_CELL);
		if (pElmCell)
		{
			string strIndex;
			CIXml::getPropertyString(strIndex, pElmCell, XML_ATT_IDX);
			if (strIndex != "")
			{
				uint32 uIndex = (uint32)atoui(strIndex.c_str());
				nCurPos = uIndex - 1;
			}
			else
				nCurPos ++;
		}
	}
	return "";
}
