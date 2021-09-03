/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __BLOCK_MEMORY_H__
#define __BLOCK_MEMORY_H__

#include "types_sip.h"
#include <list>
#include <vector>
#include "debug.h"
#include "mutex.h"

namespace SIPBASE 
{

// ***************************************************************************
/// See CBlockMemory::Purge
extern	bool		NL3D_BlockMemoryAssertOnPurge;	// =true.


// ***************************************************************************
/**
 * Block memory allocation
 *
 * This memory manager is a fast memory allocator, doing same thing as new/delete. It works by block. block are always
 * allocated, never deleted.  alocation/free are in O(1).
 *
 * Elements with sizeof(T)<sizeof(void*) should not be used with this allocator, because 
 * sizeEltInMemory= max(sizeof(T),sizeof(void*)).
 *
 * free() check invalid ptr in debug only, for extra cost of 8 octets per element.
 *
 * NB: if template parameter __ctor_dtor__ is false, then ctor and dtor are not called when an element is allocate()-ed
 *	or deallocate()-ed.
 *
 * \date 2001
 */
template<class T, bool __ctor_dtor__= true >
class CBlockMemory
{
public:

	/// Constructor
	CBlockMemory(uint blockSize= 16)
	{
		sipassert(blockSize);
		_BlockSize= blockSize;
		_EltSize= std::max((uint)sizeof(T), (uint)sizeof(void*));
		_NextFreeElt= NULL;
		_NAllocatedElts= 0;
	}
	// just copy setup from other blockMemory, don't copy data!
	CBlockMemory(const CBlockMemory<T, __ctor_dtor__> &other)
	{
		_BlockSize= other._BlockSize;
		// if other block is rebinded, don't copy its rebinded size.
		_EltSize= std::max(sizeof(T), sizeof(void*));
		// No elts allocated
		_NextFreeElt= NULL;
		_NAllocatedElts= 0;
	}
	/** purge()
	 */
	~CBlockMemory()
	{
		purge();
	}


	/// allocate an element. ctor is called.
	T*				allocate()
	{
		_Mutex.enter();

		// if not enough memory, aloc a block.
		if(!_NextFreeElt)
		{
			_Blocks.push_front(CBlock());
			buildBlock(*_Blocks.begin());
			// new free elt points to the beginning of this block.
			_NextFreeElt= (*_Blocks.begin()).Data;
#ifdef SIP_DEBUG
			// if debug, must decal for begin check.
			_NextFreeElt= (uint32*)_NextFreeElt + 1;
#endif
		}

		// choose next free elt.
		sipassert(_NextFreeElt);
		T*		ret= (T*)_NextFreeElt;

		// update _NextFreeElt, so it points to the next free element.
		_NextFreeElt= *(void**)_NextFreeElt;
		
		// construct the allocated element.
		if( __ctor_dtor__ )
#undef new
			new (ret) T;
#define new SIP_NEW


		// some simple Check.
#ifdef SIP_DEBUG
		uint32	*checkStart= (uint32*)(void*)ret-1;
		uint32	*checkEnd  = (uint32*)((uint8*)(void*)ret+_EltSize);
		sipassert( *checkStart == CheckDeletedIdent);
		sipassert( *checkEnd   == CheckDeletedIdent);
		// if ok, mark this element as allocated.
		*checkStart= CheckAllocatedIdent;
		*checkEnd  = CheckAllocatedIdent;
#endif

		_NAllocatedElts++;

		_Mutex.leave();

		return ret;
	}

	/// delete an element allocated with this manager. dtor is called. NULL is tested.
	void			free(T* ptr)
	{
		if(!ptr)
			return;

		_Mutex.enter();

		// some simple Check.
		sipassert(_NAllocatedElts>0);
#ifdef SIP_DEBUG
		uint32	*checkStart= (uint32*)(void*)ptr-1;
		uint32	*checkEnd  = (uint32*)((uint8*)(void*)ptr+_EltSize);
		sipassert( *checkStart == CheckAllocatedIdent);
		sipassert( *checkEnd   == CheckAllocatedIdent);
		// if ok, mark this element as deleted.
		*checkStart= CheckDeletedIdent;
		*checkEnd  = CheckDeletedIdent;
#endif
		
		// destruct the element.
		if( __ctor_dtor__ )
			ptr->~T();

		// just append this freed element to the list.
		*(void**)ptr= _NextFreeElt;
		_NextFreeElt= (void*) ptr;

		_NAllocatedElts--;

		_Mutex.leave();
	}


	/** delete all blocks, freeing all memory. It is an error to purge() or delete a CBlockMemory, while elements
	 * still remains!! You must free your elements with free().
	 *	NB: you can disable this assert if you set NL3D_BlockMemoryAssertOnPurge to false
	 *	(good to quit a program quickly without uninitialize).
	 */
	void	purge ()
	{
		_Mutex.enter();

		if(NL3D_BlockMemoryAssertOnPurge)
			sipassert(_NAllocatedElts==0);

		while(_Blocks.begin()!=_Blocks.end())
		{
			releaseBlock(*_Blocks.begin());
			_Blocks.erase(_Blocks.begin());
		}

		_NextFreeElt= NULL;
		_NAllocatedElts= 0;

		_Mutex.leave();
	}


// ********************
public:

	// This is to be used with CSTLBlockAllocator only!!! It change the size of an element!!
	void		__stl_alloc_changeEltSize(uint eltSize)
	{
		_Mutex.enter();

		// must not be used with object ctor/dtor behavior.
		sipassert(__ctor_dtor__ == false);
		// format size.
		eltSize= std::max((uint)eltSize, (uint)sizeof(void*));
		// if not the same size as before
		if(_EltSize!= eltSize)
		{
			// verify that rebind is made before any allocation!!
			sipassert(_Blocks.empty());
			// change the size.
			_EltSize= eltSize;
		}

		_Mutex.leave();
	};
	// This is to be used with CSTLBlockAllocator only!!!
	uint		__stl_alloc_getEltSize() const
	{
		return _EltSize;
	}


private:
	/// size of a block.
	uint		_BlockSize;
	/// size of an element in the block.
	uint		_EltSize;
	/// number of elements allocated.
	sint		_NAllocatedElts;
	/// next free element.
	void		*_NextFreeElt;
	/// Must be ThreadSafe (eg: important for 3D PointLight and list of transform)
	CFastMutex	_Mutex;


	/// a block.
	struct	CBlock
	{
		/// The data allocated.
		void		*Data;
	};

	/// list of blocks.
	std::list<CBlock>	_Blocks;


	/// For debug only, check ident.
	enum  TCheckIdent	{ CheckAllocatedIdent= 0x01234567, CheckDeletedIdent= 0x89ABCDEF };


private:
	void		buildBlock(CBlock &block)
	{
		uint	i;
		uint32	nodeSize= _EltSize;
#ifdef SIP_DEBUG
		// must allocate more size for mem checks in debug.
		nodeSize+= 2*sizeof(uint32);
#endif

		// allocate.
		block.Data = (void*)new uint8 [_BlockSize * nodeSize];

		// by default, all elements are not allocated, build the list of free elements.
		void	*ptr= block.Data;
#ifdef SIP_DEBUG
		// if debug, must decal for begin check.
		ptr= (uint32*)ptr + 1;
#endif
		for(i=0; i<_BlockSize-1; i++)
		{
			// next elt.
			void	*next= (uint8*)ptr + nodeSize;
			// points to the next element in this array.
			*(void**)ptr= next;
			// next.
			ptr= next;
		}
		// last element points to NULL.
		*(void**)ptr= NULL;


		// If debug, must init all check values to CheckDeletedIdent.
#ifdef SIP_DEBUG
		ptr= block.Data;
		// must decal for begin check.
		ptr= (uint32*)ptr + 1;
		// fill all nodes.
		for(i=0; i<_BlockSize; i++)
		{
			uint32	*checkStart= (uint32*)ptr-1;
			uint32	*checkEnd  = (uint32*)((uint8*)ptr+_EltSize);
			// mark this element as deleted.
			*checkStart= CheckDeletedIdent;
			*checkEnd  = CheckDeletedIdent;

			// next elt.
			ptr= (uint8*)ptr + nodeSize;
		}
#endif

	}
	void		releaseBlock(CBlock &block)
	{
		delete [] ((uint8*)block.Data);
	}

};


} // SIPBASE


#endif // __BLOCK_MEMORY_H__

/* End of block_memory.h */
