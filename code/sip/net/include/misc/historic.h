/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __HISTORIC_H__
#define __HISTORIC_H__

#include "misc/types_sip.h"
#include <deque>

namespace SIPBASE
{
/** An historic with user defined size.
  * An historic is just a fifo with constraint on size  
  *
  * \date 2004  
  */
template <class T>
class CHistoric
{
public:
	CHistoric(uint maxSize = 0) : _MaxSize(maxSize) {}
	// Add a value at end of historic. If historic is full then the oldest entry is removed
	inline void		push(const T &value);
	// Pop the value at the end of jistoric
	inline void		pop();
	// Return true is there are no values in the historics.
	bool			empty() const { return _Historic.empty(); }
	// Get max number of entries in the historic.
	uint			getMaxSize() const { return _MaxSize; }
	// Set number of entries in the historic. Oldest entries are removed
	inline void		setMaxSize(uint maxSize);
	// Get current size of historic
	uint			getSize() const { return _Historic.size(); }
	// Access to an element in history, 0 being the oldest, size - 1 being the lastest added element
	const T		   &operator[](uint index) const { return _Historic[index]; /* let STL do out of range check */ }
	// Clear historic
	void			clear() { _Historic.clear(); }	
private:
	std::deque<T> _Historic;
	uint		  _MaxSize;
};


////////////////////
// IMPLEMENTATION //
////////////////////

//****************************************************************************************************
template <class T>
inline void	CHistoric<T>::push(const T &value)
{
	sipassert(_Historic.size() <= _MaxSize);
	if (_MaxSize == 0) return; 
	if (getSize() == _MaxSize)
	{
		_Historic.pop_front();	
	}
	_Historic.push_back(value);
}

//****************************************************************************************************
template <class T>
inline void	CHistoric<T>::pop()
{
	sipassert(!_Historic.empty());	
	_Historic.pop_back();
}

//****************************************************************************************************
template <class T>
inline void	CHistoric<T>::setMaxSize(uint maxSize)
{
	if (maxSize < getSize())
	{
		uint toRemove = std::min(getSize() - maxSize, getSize());
		_Historic.erase(_Historic.begin(), _Historic.begin() + toRemove);
	}
	_MaxSize = maxSize;
}

}


#endif
