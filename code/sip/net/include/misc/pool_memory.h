/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __POOL_MEMORY_H__
#define __POOL_MEMORY_H__

#include "types_sip.h"
#include <list>
#include <vector>

namespace SIPBASE 
{
/**
 * Pool memory allocation
 *
 * This memory manager allocates bloc of elements and free all the 
 * elements at the same time. Useful for temporary allocation.
 *
 * \date 2001
 */

template<class T>
class CPoolMemory
{
public:
	/*
	 * Constructor
	 *
	 * \param blockSize is the default bloc size
	 */
	CPoolMemory (uint blockSize=10)
	{
		_BlockSize=blockSize;
		_BlockPointer=_BlockList.end();
	}

	/*
	 * Allocate an element.
	 *
	 * Return a pointer on the element.
	 */
	T*		allocate ()
	{
		// End of block ?
		if ( (_BlockPointer!=_BlockList.end()) && (_BlockPointer->size()>=_BlockSize) )
		{
			// Get next block
			_BlockPointer++;
		}

		// Out of memory ?
		if (_BlockPointer==_BlockList.end())
		{
			// Add a block
			_BlockList.resize (_BlockList.size()+1);

			// Pointer on the new block
			_BlockPointer=_BlockList.end ();
			_BlockPointer--;

			// Reserve it
			_BlockPointer->reserve (_BlockSize);
		}

		// Allocate
		_BlockPointer->resize (_BlockPointer->size()+1);

		// Return the pointer
		return &*((_BlockPointer->end ()-1));
	}

	/*
	 * Free all the elements allocated since last free(). Memory is kept for next allocations.
	 */
	void	free ()
	{
		typename std::list< std::vector<T> >::iterator ite=_BlockList.begin();
		while (ite!=_BlockList.end())
		{
			// Clear the block
			ite->clear ();

			// Check size in not zero
			sipassert (ite->capacity ()>0);

			ite++;
		}
		// Pointer at the begining
		_BlockPointer=_BlockList.begin();
	}

	/*
	 * Purge memory. All the memory used by the allocator is freid for real.
	 */
	void	purge ()
	{
		_BlockList.clear();
		_BlockPointer=_BlockList.end();
	}

private:
	uint									_BlockSize;
	std::list< std::vector<T> >				_BlockList;
	typename std::list< std::vector<T> >::iterator	_BlockPointer;
};


} // SIPBASE


#endif // __POOL_MEMORY_H__

/* End of pool_memory.h */
