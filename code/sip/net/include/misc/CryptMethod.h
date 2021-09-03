/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __CRYPTMETHOD_H__
#define __CRYPTMETHOD_H__

#include "misc/types_sip.h"

#include "CryptKey.h"

namespace SIPBASE
{
	enum	E_CRYPTLEVEL	{ CL_LOW=0, CL_MEDIUM, CL_HIGH };
//////////////////////////////////////////////////////////////////////////
	class ICryptMethod
	{
	public:
		ICryptMethod(E_CRYPTLEVEL _cl) : m_CryptLevel(_cl) { };
		virtual ~ICryptMethod()
		{ 
			delete m_pCryptKey;
		};
		IKey*			GetCryptKey()	const
		{
			return	m_pCryptKey;
		}		
		E_CRYPTLEVEL	GetCryptLevel()	const
		{
			return	m_CryptLevel;
		}
	protected:
		IKey*			m_pCryptKey;
		E_CRYPTLEVEL	m_CryptLevel;
	public:
		/*	주의 
			암호화후에 평문(_pOriginalData)구역이 변할수 있다.	
			그러므로 암호화후에 평문자료를 다시 써야 한다면 암호화전에 복사하여 놓아야 한다.
		*/
		virtual		bool	Encode(uint8* _pOriginalData, uint32 _OriginalDataLen, OUT uint8** _pCrptedData, OUT uint32& _rCryptedLen, OUT bool& _rDeleteAfterUsing) = 0;
		virtual		bool	Decode(uint8* _pCrptedData, uint32	_CryptedLen, OUT uint8** _pOriginalData, OUT uint32& _rOriginalDataLen, OUT bool& _rDeleteAfterUsing) = 0;

	};

//////////////////////////////////////////////////////////////////////////
	class ICryptMethodType
	{
	public:
		ICryptMethodType(uint32 _TypeID);
		virtual ~ICryptMethodType();

		virtual ICryptMethod*	MakeNewCryptMethodObject() = 0;
		uint32	GetTypeId() const { return m_nTypeId; }
		E_CRYPTLEVEL	GetCryptLevel() const { return m_nCryptLevel; }
	protected:
		uint32			m_nTypeId;
		E_CRYPTLEVEL	m_nCryptLevel;	
	};

	template <class T>
	class CCryptMethodType : public ICryptMethodType
	{
	public:
		CCryptMethodType(uint32 _TypeId);
		virtual ICryptMethod*	MakeNewCryptMethodObject();
	};

//////////////////////////////////////////////////////////////////////////
// 반드시 DEFINE_CRYPTMETHOD_CONSTRUCTOR 마크로를 사용하여 암호화클라스의 생성자를 정의하여야 한다.
#define DEFINE_CRYPTMETHOD_CONSTRUCTOR(cryptmethod, cryptlevel)		\
	template class CCryptMethodType<cryptmethod>;					\
	cryptmethod::cryptmethod(const IKey* _pCryptKey)				\
	:ICryptMethod(cryptlevel)

	class CCryptMethodSimpleXor1: public ICryptMethod
	{
	public:
		CCryptMethodSimpleXor1(const IKey* _pCryptKey);

		bool	Encode(uint8* _pOriginalData, uint32 _OriginalDataLen, OUT uint8** _pCrptedData, OUT uint32& _rCryptedLen, OUT bool& _rDeleteAfterUsing);
		bool	Decode(uint8* _pCrptedData, uint32	_CryptedLen, OUT uint8** _pOriginalData, OUT uint32& _rOriginalDataLen, OUT bool& _rDeleteAfterUsing);
	};

	class CCryptMethodMediumXor : public ICryptMethod
	{
	public:
		CCryptMethodMediumXor(const IKey* _pCryptKey);

		bool	Encode(uint8* _pOriginalData, uint32 _OriginalDataLen, OUT uint8** _pCrptedData, OUT uint32& _rCryptedLen, OUT bool& _rDeleteAfterUsing);
		bool	Decode(uint8* _pCrptedData, uint32	_CryptedLen, OUT uint8** _pOriginalData, OUT uint32& _rOriginalDataLen, OUT bool& _rDeleteAfterUsing);
	};
} // namespace SIPBASE

#endif // end of guard __CRYPTMETHOD_H__
