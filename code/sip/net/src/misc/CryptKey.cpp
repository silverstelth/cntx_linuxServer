/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"

#include "misc/CryptKey.h"
#include <time.h>

namespace SIPBASE
{

void	GenerateRandomValue(uint8* _pBuffer, uint32 _BufferLength)
{
	srand((uint32)time (NULL));
	uint32 nPrevData = rand();
	for (uint32 i = 0; i < _BufferLength; i++)
	{
		if (nPrevData == 0)
			nPrevData = rand();
		uint32	nData = rand() * nPrevData;
		_pBuffer[i] = static_cast<uint8>(nData & 0x000000FF);
		if (_pBuffer[i] == 0)
			_pBuffer[i] = static_cast<uint8>((nData >> 8)& 0x000000FF);
		if (_pBuffer[i] == 0)
			_pBuffer[i] = static_cast<uint8>((nData >> 16)& 0x000000FF);
		if (_pBuffer[i] == 0)
			_pBuffer[i] = static_cast<uint8>((nData >> 24)& 0x000000FF);
		nPrevData = nData;
	}

}
// CKeyFourBytes //////////////////////////////////////////////////////////////
CKeyFourBytes::CKeyFourBytes(uint32 _V)
{
	SetValue(_V);
}

bool	CKeyFourBytes::GenerateKey(const void* _pInitValue)
{
	SetValue(*(reinterpret_cast<const uint32*>(_pInitValue)));
	return true;
}

void	CKeyFourBytes::serial(SIPBASE::IStream& _rStream)
{
	_rStream.serial(m_nValue);
}

void	CKeyFourBytes::Copyfrom(const IKey* _pFrom)
{
	CKeyFourBytes* pFromTemp = const_cast<CKeyFourBytes*>(reinterpret_cast<const CKeyFourBytes*>(_pFrom));
	SetValue(pFromTemp->GetValue());
}

uint32	CKeyFourBytes::GetValue() const
{
	return m_nValue;
}

void	CKeyFourBytes::SetValue(uint32 _V)
{
	m_nValue = _V;
}
//CKeyFourBytes//////////////////////////////////////////////////////////////

// CKeyRandomBytes //////////////////////////////////////////////////////////////
CKeyRandomBytes::CKeyRandomBytes(uint32 nLen) :
	m_nLength(nLen)
{
	sipassert(m_nLength != 0);
	m_pValueStart = new uint8[m_nLength];
}

CKeyRandomBytes::~CKeyRandomBytes()
{
	delete [] m_pValueStart;
}

bool	CKeyRandomBytes::GenerateKey(const void* _pInitValue)
{
	GenerateRandomValue(m_pValueStart, m_nLength);
	return true;
}

void	CKeyRandomBytes::serial(SIPBASE::IStream& _rStream)
{
	int nOldLength = m_nLength;
	_rStream.serial(m_nLength);
	
	if (_rStream.isReading())
	{
		if (nOldLength != m_nLength)
		{
			delete [] m_pValueStart;
			m_pValueStart = new uint8[m_nLength];
		}
	}
	
	_rStream.serialBuffer(m_pValueStart, m_nLength);
}

void	CKeyRandomBytes::Copyfrom(const IKey* _pFrom)
{
	CKeyRandomBytes* pFromTemp = const_cast<CKeyRandomBytes*>(reinterpret_cast<const CKeyRandomBytes*>(_pFrom));
	
	uint32	nLen = pFromTemp->GetLength();
	uint8*	pStart = pFromTemp->GetValueStart();

	if (nLen != m_nLength)
	{
		delete [] m_pValueStart;
		m_nLength = nLen;
		m_pValueStart = new uint8[m_nLength];
	}
	memcpy(m_pValueStart, pStart, m_nLength);
}

//CKeyRandomBytes//////////////////////////////////////////////////////////////
} // namespace SIPBASE
