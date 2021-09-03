/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CRYPTKEY_H__
#define __CRYPTKEY_H__

#include "misc/types_sip.h"
#include "misc/stream.h"
namespace SIPBASE
{
class IKey
{
public:	
	IKey()	{}
	virtual ~IKey() {}
	virtual		bool	GenerateKey(const void* _pInitValue) = 0;
	virtual		void	serial(SIPBASE::IStream& _rStream) = 0;
	virtual		void	Copyfrom(const IKey* _pFrom) = 0;
};

class	CKeyFourBytes : public IKey
{
public:
	CKeyFourBytes(uint32 v);
	
	virtual		bool	GenerateKey(const void* _pInitValue);
	virtual		void	serial(SIPBASE::IStream& _rStream);
	virtual		void	Copyfrom(const IKey* _pFrom);

	uint32		GetValue() const;
	void		SetValue(uint32 _V);
private:	
	uint32		m_nValue;
};

class	CKeyRandomBytes : public IKey
{
public:
	CKeyRandomBytes(uint32 _Length);
	~CKeyRandomBytes();
	virtual		bool	GenerateKey(const void* _pInitValue);
	virtual		void	serial(SIPBASE::IStream& _rStream);
	virtual		void	Copyfrom(const IKey* _pFrom);

	uint8*		GetValueStart() const { return m_pValueStart; };
	uint32		GetLength() const { return m_nLength; };
private:	
	uint32		m_nLength;
	uint8*		m_pValueStart;
};

extern void	GenerateRandomValue(uint8* _pBuffer, uint32 _BufferLength);

} // namespace SIPBASE

#endif // end of guard __CRYPTKEY_H__
