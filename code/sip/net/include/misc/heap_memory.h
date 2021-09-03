/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __HEAP_MEMORY_H__
#define __HEAP_MEMORY_H__

#include "types_sip.h"
#include <map>

namespace SIPBASE 
{


// ***************************************************************************
/**
 * A Heap manager. Work with any kind of memory.
 *	This heap manager is not designed for speed (because it stills use standard heap allocation), but for
 *	use with special memory or special cases where malloc/new cannot be used.
 * \date 2001
 */
class CHeapMemory
{
public:

	/// Constructor
	CHeapMemory();
	~CHeapMemory();

	/// reset the entire container. NB: no free() is made on the heap.
	void			reset();
	/** init the heap. this reset() the heap. heap ptr is stored in this class, but no CHeapMemory methods
	 * write or read into. They use standard heap instead (new / delete).
	 *	\param heap the heap ptr. heap should be at least N-bytes aligned, where N is the alignment you need (see
	 *	param align, 4 by default).
	 *	\param align Any size given to allocate() will be rounded to match this alignement.
	 *	Valid values are 4,8,16, or 32. 4 by default.
	 */
	void			initHeap(void *heap, uint size, uint align=4);


	/// return the size passed in setHeap().
	uint			getHeapSize() const {return _HeapSize;}
	/// return the heap size allocated.
	uint			getHeapSizeUsed() const {return _HeapSizeUsed;}


	/** allocate a block of size bytes. return NULL if not enough space or if size==0.
	 *	NB: for alignements consideration, allocation are aligned to 4 bytes.
	 */
	void			*allocate(uint size);
	/// free a block allocated with alloate(). no-op if NULL. sipstop() if don't find this block.
	void			free(void *ptr);


// *********************
private:

	// The map size -> EmptySpace.
	typedef	std::multimap<uint, uint8*>		TEmptySpaceSizeMap;
	typedef	TEmptySpaceSizeMap::iterator	ItEmptySpaceSizeMap;


	struct	CEmptySpace
	{
		// The adress of this empty space, in the heap
		uint8		*Ptr;
		// The size of this empty space.
		uint		Size;
		// An iterator of his place in the  TEmptySpaceMap.
		ItEmptySpaceSizeMap		SizeIt;
	};


	// The map ptr -> EmptySpace.
	typedef	std::map<uint8*, CEmptySpace>	TEmptySpacePtrMap;
	typedef	TEmptySpacePtrMap::iterator		ItEmptySpacePtrMap;


	// The map ptr -> size: AllocatedSpace;
	typedef	std::map<uint8*, uint>			TAllocatedSpaceMap;
	typedef	TAllocatedSpaceMap::iterator	ItAllocatedSpaceMap;


private:
	uint8			*_HeapPtr;
	uint			_HeapSize;
	uint			_HeapSizeUsed;
	uint			_Alignment;

	/// The array of empty spaces.
	TEmptySpacePtrMap		_EmptySpaces;
	/// for allocate method, the size -> empty space map.
	TEmptySpaceSizeMap		_EmptySpaceMap;
	// The map of allocated blocks.
	TAllocatedSpaceMap		_AllocatedSpaceMap;


	void		removeEmptySpace(CEmptySpace &space);
	void		addEmptySpace(CEmptySpace &space);
};


} // SIPBASE


#endif // __HEAP_MEMORY_H__

/* End of heap_memory.h */
