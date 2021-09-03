#pragma once

#include "misc/types_sip.h"
#include "misc/i_xml.h"

extern	xmlNodePtr	ExcelGetSheet(const SIPBASE::CIXml *xmlInfo, const ucchar *chSheet);
extern	int		ExcelGetColumnPos(xmlNodePtr pSheet, const ucchar *chColumn);
extern	xmlNodePtr	ExcelGetSectionRow(xmlNodePtr pSheet);
extern	xmlNodePtr	ExcelGetFirstDataRow(xmlNodePtr pSheet);
extern	xmlNodePtr	ExcelGetNextDataRow(xmlNodePtr pCur);
extern	std::string	ExcelGetCellValue(xmlNodePtr pRow, int nPos);
