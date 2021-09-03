/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __READER_WRITER_H__
#define __READER_WRITER_H__

#include "types_sip.h"
#include "mutex.h"

namespace SIPBASE {

/**
 * This class allows a reader/writer ressource usage policy.
 * \date 2001
 */
class CReaderWriter
{
private:

	volatile CMutex	_Fairness;
	volatile CMutex	_ReadersMutex;
	volatile CMutex	_RWMutex;
	volatile sint	_ReadersLevel;

public:

	CReaderWriter();
	~CReaderWriter();

	void			enterReader()
	{
		const_cast<CMutex&>(_Fairness).enter();
		const_cast<CMutex&>(_ReadersMutex).enter();
		++_ReadersLevel;
		if (_ReadersLevel == 1)
			const_cast<CMutex&>(_RWMutex).enter();
		const_cast<CMutex&>(_ReadersMutex).leave();
		const_cast<CMutex&>(_Fairness).leave();
	}

	void			leaveReader()
	{
		const_cast<CMutex&>(_ReadersMutex).enter();
		--_ReadersLevel;
		if (_ReadersLevel == 0)
			const_cast<CMutex&>(_RWMutex).leave();
		const_cast<CMutex&>(_ReadersMutex).leave();
	}

	void			enterWriter()
	{
		const_cast<CMutex&>(_Fairness).enter();
		const_cast<CMutex&>(_RWMutex).enter();
		const_cast<CMutex&>(_Fairness).leave();
	}

	void			leaveWriter()
	{
		const_cast<CMutex&>(_RWMutex).leave();
	}
};

/**
 * This class uses a CReaderWriter object to implement a synchronized object (see mutex.h for standard CSynchronized.)
 * \date 2001
 */
template <class T>
class CRWSynchronized
{
public:

	class CReadAccessor
	{
		CRWSynchronized<T>		*_RWSynchronized;

	public:

		CReadAccessor(CRWSynchronized<T> *cs)
		{
			_RWSynchronized = cs;
			const_cast<CReaderWriter&>(_RWSynchronized->_RWSync).enterReader();
		}

		~CReadAccessor()
		{
			const_cast<CReaderWriter&>(_RWSynchronized->_RWSync).leaveReader();
		}

		const T		&value()
		{
			return const_cast<const T&>(_RWSynchronized->_Value);
		}
	};

	class CWriteAccessor
	{
		CRWSynchronized<T>		*_RWSynchronized;

	public:

		CWriteAccessor(CRWSynchronized<T> *cs)
		{
			_RWSynchronized = cs;
			const_cast<CReaderWriter&>(_RWSynchronized->_RWSync).enterWriter();
		}

		~CWriteAccessor()
		{
			const_cast<CReaderWriter&>(_RWSynchronized->_RWSync).leaveWriter();
		}

		T		&value()
		{
			return const_cast<T&>(_RWSynchronized->_Value);
		}
	};

private:
	friend class CRWSynchronized::CReadAccessor;
	friend class CRWSynchronized::CWriteAccessor;

	volatile CReaderWriter		_RWSync;

	volatile T					_Value;

};

} // SIPBASE


#endif // __READER_WRITER_H__

/* End of reader_writer.h */
