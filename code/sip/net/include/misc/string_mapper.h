/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STRING_MAPPER_H__
#define __STRING_MAPPER_H__

#include "types_sip.h"

#include <vector>
#include <set>

#include "stream.h"
#include "mutex.h"

namespace SIPBASE
{
// const string *  as  uint (the TStringId returned by CStringMapper is a pointer to a string object)
//#ifdef HAVE_X86_64
//typedef uint64 TStringId;
//#else
//typedef uint TStringId;
//#endif

typedef	const std::string * TStringId;

class CStringIdHasher
{
public:
	CStringIdHasher()
	{}
	inline	size_t	operator	()(const SIPBASE::TStringId &stringId)	const
	{
		return	(size_t)stringId;
	}
protected:
private:
};

/** A static class that map string to integer and vice-versa
 * Each different string is tranformed into an unique integer identifier.
 * If the same string is submited twice, the same id is returned.
 * The class can also return the string associated with an id.
 *
 * \date 2003
 */
class CStringMapper
{
	class CCharComp
	{
	public:
		bool operator()(std::string *x, std::string *y) const
		{
			return (*x) < (*y);
		}
	};

	class CAutoFastMutex
	{
		CFastMutex		*_Mutex;
	public:
		CAutoFastMutex(CFastMutex *mtx) : _Mutex(mtx)	{_Mutex->enter();}
		~CAutoFastMutex() {_Mutex->leave();}
	};

	// Local Data
	std::set<std::string*,CCharComp>	_StringTable;
	std::string*			_EmptyId;
	CFastMutex				_Mutex;		// Must be thread-safe (Called by CPortal/CCluster, each of them called by CInstanceGroup)
	
	// The 'singleton' for static methods
	static	CStringMapper	_GlobalMapper;

	// private constructor.
	CStringMapper();

public:

	~CStringMapper()
	{
		localClear();
	}

	/// Globaly map a string into a unique Id. ** This method IS Thread-Safe **
	static TStringId			map(const std::string &str) { return _GlobalMapper.localMap(str); }
	/// Globaly unmap a string. ** This method IS Thread-Safe **
	static const std::string	&unmap(const TStringId &stringId) { return _GlobalMapper.localUnmap(stringId); }
	/// Globaly helper to serial a string id. ** This method IS Thread-Safe **
	static void					serialString(SIPBASE::IStream &f, TStringId &id) {_GlobalMapper.localSerialString(f, id);}
	/// Return the global id for the empty string (helper function). NB: Works with every instance of CStringMapper
	static TStringId			emptyId() { return 0; }

	// ** This method IS Thread-Safe **
	static void					clear() { _GlobalMapper.localClear(); }

	/// Create a local mapper. You can dispose of it by deleting it.
	static CStringMapper *	createLocalMapper();
	/// Localy map a string into a unique Id
	TStringId				localMap(const std::string &str);
	/// Localy unmap a string
	const std::string		&localUnmap(const TStringId &stringId) { return (stringId==0)?*_EmptyId:*((std::string*)stringId); }
	/// Localy helper to serial a string id
	void					localSerialString(SIPBASE::IStream &f, TStringId &id);

	void					localClear();

};

// linear from 0 (0 is empty string) (The TSStringId returned by CStaticStringMapper 
// is an index in the vector and begin at 0)
typedef uint TSStringId;

/** 
 * After endAdd you cannot add strings anymore or it will assert
 * \date November 2003
 */
template<typename T>
class TCStaticStringMapper
{
	std::map<std::basic_string<T>, TSStringId>	_TempStringTable;
	std::map<TSStringId, std::basic_string<T> >	_TempIdTable;

	TSStringId	_IdCounter;
	T		*_AllStrings;
	std::vector<T *>	_IdToStr;
	bool _MemoryCompressed; // If false use the 2 maps

public:

	TCStaticStringMapper()
	{
		_IdCounter = 0;
		_AllStrings = NULL;
		_MemoryCompressed = false;
		add((T *)(L""));
	}

	~TCStaticStringMapper()
	{
		clear();
	}

	/// Globaly map a string into a unique Id
	TSStringId			add(const std::basic_string<T> &str)
	{
		sipassert(!_MemoryCompressed);
		typename std::map<std::basic_string<T>, TSStringId>::iterator it = _TempStringTable.find(str);
		if (it == _TempStringTable.end())
		{
//			_TempStringTable.insert(std::pair<std::basic_string<T>, TSStringId>::pair(str, _IdCounter));
//			_TempIdTable.insert(std::pair<TSStringId, std::basic_string<T> >::pair(_IdCounter, str));
//			by LCI 09-05-13
			_TempStringTable.insert(make_pair(str, _IdCounter));
			_TempIdTable.insert(make_pair(_IdCounter, str));
			_IdCounter ++;
			return _IdCounter - 1;
		}
		else
		{
			return it->second;
		}
	}

	void				memoryCompress()
	{
		_MemoryCompressed = true;
		typename std::map<TSStringId, std::basic_string<T> >::iterator it = _TempIdTable.begin();

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

	// Uncompress the map.
	void				memoryUncompress()
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

	/// Globaly unmap a string
	const T *		get(TSStringId stringId)
	{
		if (_MemoryCompressed)
		{
			sipassert (stringId < _IdToStr.size());
			return _IdToStr[stringId];
		}
		else
		{
			typename std::map<TSStringId, std::basic_string<T> >::iterator it = _TempIdTable.find(stringId);
			if (it != _TempIdTable.end())
				return it->second.c_str();
			else
				return NULL;
		}
	}

	/// Return the global id for the empty string (helper function)
	static TSStringId	emptyId() { return 0; }

	void				clear()
	{
		contReset(_TempStringTable);
		contReset(_TempIdTable);
		delete [] _AllStrings;
		contReset(_IdToStr);

		_IdCounter = 0;
		_AllStrings = NULL;
		_MemoryCompressed = false;
		add((T *)(L""));
	}

	uint32 getCount() { return _IdCounter; }

	// helper serialize a string id as a string
	void				serial(SIPBASE::IStream &f, TSStringId &strId) throw(EStream)
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

	// helper serialize a string id vector
	void				serial(SIPBASE::IStream &f, std::vector<TSStringId> &strIdVect) throw(EStream)
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
};

typedef TCStaticStringMapper<char> CStaticStringMapper;
typedef TCStaticStringMapper<ucchar> CStaticStringMapperW;

} //namespace SIPBASE

#endif // __STRING_MAPPER_H__

