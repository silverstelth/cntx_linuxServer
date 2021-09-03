/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include <map>
#include "misc/string_mapper.h"

using namespace std;

namespace SIPBASE
{

CStringMapper	CStringMapper::_GlobalMapper;


// ****************************************************************************
CStringMapper::CStringMapper()
{
	_EmptyId = new string;
	*_EmptyId = "";
}

// ****************************************************************************
CStringMapper *CStringMapper::createLocalMapper()
{
	return new CStringMapper;
}

// ****************************************************************************
TStringId CStringMapper::localMap(const std::string &str)
{
	if (str.size() == 0)
		return 0;

	CAutoFastMutex	automutex(&_Mutex);

	string *pStr = new string;
	*pStr = str;

	std::set<string*,CCharComp>::iterator it = _StringTable.find(pStr);

	if (it == _StringTable.end())
	{
		_StringTable.insert(pStr);
	}
	else
	{
		delete pStr;
		pStr = (*it);
	}	
	return (TStringId)pStr;
}

// ***************************************************************************
void CStringMapper::localSerialString(SIPBASE::IStream &f, TStringId &id)
{
	std::string	str;
	if(f.isReading())
	{
		f.serial(str);
		id= localMap(str);
	}
	else
	{
		str= localUnmap(id);
		f.serial(str);
	}
}

// ****************************************************************************
void CStringMapper::localClear()
{
	CAutoFastMutex	automutex(&_Mutex);
	
	std::set<string*,CCharComp>::iterator it = _StringTable.begin();
	while (it != _StringTable.end())
	{
		string *ptrTmp = (*it);
		delete ptrTmp;
		it++;
	}
	_StringTable.clear();
	delete _EmptyId;
}
/*
// ****************************************************************************
// CStaticStringMapper
// ****************************************************************************

// ****************************************************************************
template<class T>
TSStringId TCStaticStringMapper<T>::add(const std::basic_string<T> &str)
{
	sipassert(!_MemoryCompressed);
	std::map<std::basic_string<T>, TSStringId>::iterator it = _TempStringTable.find(str);
	if (it == _TempStringTable.end())
	{
		_TempStringTable.insert(pair<basic_string<T>, TSStringId>::pair(str, _IdCounter));
		_TempIdTable.insert(pair<TSStringId, basic_string<T> >::pair(_IdCounter, str));
		_IdCounter ++;
		return _IdCounter - 1;
	}
	else
	{
		return it->second;
	}
}

// ****************************************************************************
template<class T>
void TCStaticStringMapper<T>::memoryUncompress()
{
	std::map<std::basic_string<T>, TSStringId>	tempStringTable;
	std::map<TSStringId, std::basic_string<T> >	tempIdTable;
	for(uint k = 0; k < _IdToStr.size(); ++k)
	{
		tempStringTable[_IdToStr[k]] = (TSStringId) k;
		tempIdTable[(TSStringId) k] = _IdToStr[k];
	}
	delete [] _AllStrings;
	_AllStrings = NULL;
	contReset(_IdToStr);	
	_TempStringTable.swap(tempStringTable);
	_TempIdTable.swap(tempIdTable);
	_MemoryCompressed = false;
}

// ****************************************************************************
template<class T>
void TCStaticStringMapper<T>::memoryCompress()
{
	_MemoryCompressed = true;
	std::map<TSStringId, std::basic_string<T> >::iterator it = _TempIdTable.begin();

	uint nTotalSize = 0;
	uint32 nNbStrings = 0;
	while (it != _TempIdTable.end())
	{
		nTotalSize += it->second.size() + 1;
		nNbStrings++;
		it++;
	}
	
	_AllStrings = new T[nTotalSize];
	_IdToStr.resize(nNbStrings);
	nNbStrings = 0;
	nTotalSize = 0;
	it = _TempIdTable.begin();
	while (it != _TempIdTable.end())
	{
		memcpy((void *)(_AllStrings + nTotalSize), (void *)it->second.c_str(), (it->second.size() + 1) * sizeof(T));
		_IdToStr[nNbStrings] = _AllStrings + nTotalSize;
		nTotalSize += it->second.size() + 1;
		nNbStrings++;
		it++;
	}
	contReset(_TempStringTable);
	contReset(_TempIdTable);
}

// ****************************************************************************
template<class T>
const T *TCStaticStringMapper<T>::get(TSStringId stringId)
{
	if (_MemoryCompressed)
	{
		sipassert (stringId < _IdToStr.size());
		return _IdToStr[stringId];
	}
	else
	{
		std::map<TSStringId, std::basic_string<T> >::iterator it = _TempIdTable.find(stringId);
		if (it != _TempIdTable.end())
			return it->second.c_str();
		else
			return NULL;
	}
}

// ****************************************************************************
template<class T>
void TCStaticStringMapper<T>::clear()
{
	contReset(_TempStringTable);
	contReset(_TempIdTable);
	delete [] _AllStrings;
	contReset(_IdToStr);

	_IdCounter = 0;
	_AllStrings = NULL;
	_MemoryCompressed = false;
	add((T *)L"");
}

// ****************************************************************************
template<class T>
void TCStaticStringMapper<T>::serial(IStream &f, TSStringId &strId) throw(EStream)
{
	std::basic_string<T> tmp;
	if (f.isReading())
	{		
		f.serial(tmp);
		strId = add(tmp);
	}
	else
	{
		tmp = get(strId);
		f.serial(tmp);
	}
}

// ****************************************************************************
template<class T>
void TCStaticStringMapper<T>::serial(IStream &f, std::vector<TSStringId> &strIdVect) throw(EStream)
{
	std::vector<std::basic_string<T> > vsTmp;
	std::basic_string<T> sTmp;
	// Serialize class components.
	if (f.isReading())
	{				
		f.serialCont(vsTmp);
		strIdVect.resize(vsTmp.size());
		for(uint i = 0; i < vsTmp.size(); ++i)
			strIdVect[i] = add(vsTmp[i]);
	}
	else
	{		
		vsTmp.resize(strIdVect.size());
		for (uint i = 0; i < vsTmp.size(); ++i)
			vsTmp[i] = get(strIdVect[i]);
		f.serialCont(vsTmp);				
	}
}
*/
} // namespace SIPBASE
