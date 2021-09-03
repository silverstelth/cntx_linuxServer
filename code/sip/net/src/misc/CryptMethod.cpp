/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/CryptMethod.h"
#include "misc/CryptSystemManager.h"

namespace SIPBASE
{
// CCryptMethodType //////////////////////////////////////////////////////////////
ICryptMethodType::ICryptMethodType(uint32 _TypeID)	: 
	m_nTypeId(_TypeID)
{
}
ICryptMethodType::~ICryptMethodType()
{
}

// CCryptMethodType //////////////////////////////////////////////////////////////
template <class T>
CCryptMethodType<T>::CCryptMethodType(uint32 _TypeId) : 
	ICryptMethodType(_TypeId) 
{
	T* pNew = new T(NULL);
	m_nCryptLevel = pNew->GetCryptLevel();
	delete pNew;
}
template <class T>
ICryptMethod*	CCryptMethodType<T>::MakeNewCryptMethodObject()
{
	T* pNew = new T(NULL);
	return pNew;
}

// CCryptMethodSimpleXor1 //////////////////////////////////////////////////////////////
DEFINE_CRYPTMETHOD_CONSTRUCTOR(CCryptMethodSimpleXor1, CL_LOW)
{	
	m_pCryptKey = new CKeyFourBytes(516);
	if (_pCryptKey != NULL)
		m_pCryptKey->Copyfrom(_pCryptKey);
}

bool	CCryptMethodSimpleXor1::Encode(uint8* _pOriginalData, uint32 _OriginalDataLen, OUT uint8** _pCrptedData, OUT uint32& _rCryptedLen, OUT bool& _rDeleteAfterUsing)
{
	uint32	nMemoryUnitSize	= sizeof(uint32);

	uint	nRemainSize		= _OriginalDataLen;
	void*	pCurrentMemory	= _pOriginalData;
	uint32	nCurrentKey		= ((CKeyFourBytes*)m_pCryptKey)->GetValue();	

	while (nRemainSize >= nMemoryUnitSize)
	{
		uint32	nCurrentValue			= *((uint32 *)(pCurrentMemory));
		*((uint32 *)(pCurrentMemory))	= nCurrentValue ^ nCurrentKey;
		nCurrentKey						= nCurrentValue;

		pCurrentMemory	= (uint32 *)(pCurrentMemory) + 1;
		nRemainSize		= nRemainSize - nMemoryUnitSize;
	}

	*_pCrptedData		= _pOriginalData;
	_rCryptedLen		= _OriginalDataLen;
	_rDeleteAfterUsing	= false;
	return true;
}

bool	CCryptMethodSimpleXor1::Decode(uint8* _pCrptedData, uint32	_CryptedLen, OUT uint8** _pOriginalData, OUT uint32& _rOriginalDataLen, OUT bool& _rDeleteAfterUsing)
{
	uint32	nMemoryUnitSize	= sizeof(uint32);

	uint	nRemainSize			= _CryptedLen;
	void*	pCurrentMemory		= _pCrptedData;
	uint32	nCurrentKey			= ((CKeyFourBytes*)m_pCryptKey)->GetValue();

	while (nRemainSize >= nMemoryUnitSize)
	{
		uint32	nCurrentValue			= *((uint32 *)(pCurrentMemory));
		nCurrentKey						= nCurrentValue ^ nCurrentKey;
		*((uint32 *)(pCurrentMemory))	= nCurrentKey;

		pCurrentMemory	= (uint32 *)(pCurrentMemory) + 1;
		nRemainSize		= nRemainSize - nMemoryUnitSize;
	}

	*_pOriginalData		= _pCrptedData;
	_rOriginalDataLen	= _CryptedLen;
	_rDeleteAfterUsing	= false;

	return true;
}
// CCryptMethodSimpleXor1 //////////////////////////////////////////////////////////////

// CCryptMethodMediumXor //////////////////////////////////////////////////////////////
DEFINE_CRYPTMETHOD_CONSTRUCTOR(CCryptMethodMediumXor, CL_MEDIUM)
{	
	m_pCryptKey = new CKeyRandomBytes(32);
	if (_pCryptKey != NULL)
		m_pCryptKey->Copyfrom(_pCryptKey);
}

bool	CCryptMethodMediumXor::Encode(uint8* _pOriginalData, uint32 _OriginalDataLen, OUT uint8** _pCrptedData, OUT uint32& _rCryptedLen, OUT bool& _rDeleteAfterUsing)
{
	CKeyRandomBytes*	pKey		= const_cast<CKeyRandomBytes*>(reinterpret_cast<const CKeyRandomBytes*>(m_pCryptKey));
	uint8*				pKeyStart	= pKey->GetValueStart();
	uint32				nKeyLen		= pKey->GetLength();

	for (uint32 i = 0; i < _OriginalDataLen; i++)
	{
		uint8	orderPos	= i % 3;
		uint32	keyPos		= i % nKeyLen;
		uint8	curKey		= pKeyStart[keyPos];

		switch (orderPos)
		{
		case 0:		_pOriginalData[i] = (_pOriginalData[i] + curKey) & (0xFF);		break;
		case 1:		_pOriginalData[i] = _pOriginalData[i] - curKey;		break;
		case 2:		_pOriginalData[i] = _pOriginalData[i] ^ curKey;		break;
		}
	}

	*_pCrptedData		= _pOriginalData;
	_rCryptedLen		= _OriginalDataLen;
	_rDeleteAfterUsing	= false;
	return true;
}

bool	CCryptMethodMediumXor::Decode(uint8* _pCrptedData, uint32 _CryptedLen, OUT uint8** _pOriginalData, OUT uint32& _rOriginalDataLen, OUT bool& _rDeleteAfterUsing)
{
	CKeyRandomBytes*	pKey		= const_cast<CKeyRandomBytes*>(reinterpret_cast<const CKeyRandomBytes*>(m_pCryptKey));
	uint8*				pKeyStart	= pKey->GetValueStart();
	uint32				nKeyLen		= pKey->GetLength();

	for (uint32 i = 0; i < _CryptedLen; i++)
	{
		uint8	orderPos = i % 3;
		uint32	keyPos = i % nKeyLen;
		uint8	curKey = pKeyStart[keyPos];

		switch (orderPos)
		{
		case 0:		_pCrptedData[i] = _pCrptedData[i] - curKey;					break;
		case 1:		_pCrptedData[i] = (_pCrptedData[i] + curKey) & (0xFF);		break;
		case 2:		_pCrptedData[i] = _pCrptedData[i] ^ curKey;					break;
		}
	}

	*_pOriginalData		= _pCrptedData;
	_rOriginalDataLen	= _CryptedLen;
	_rDeleteAfterUsing	= false;

	return true;
}
// CCryptMethodMediumXor //////////////////////////////////////////////////////////////
} // namespace SIPBASE
