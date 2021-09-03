/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __OBJECT_ARENA_ALLOCATOR_H__
#define __OBJECT_ARENA_ALLOCATOR_H__

#include "singleton.h"

namespace SIPBASE
{
class CFixedSizeAllocator;

/** An allocator that can allocate/release in O(1) for a finite number of possible blocks size (usually small)..
  * For a given block size, a fixed size allocator is used.
  * One possible use is with a family of class for which new and delete have been redefined at the top of the hierarchy  
  * (which the SIP_USES_DEFAULT_ARENA_OBJECT_ALLOCATOR macro does)
  *
  * \todo thread safety
  * 
  * \date 2004
  */
class CObjectArenaAllocator
{
public:
	/** ctor
	  * \param maxAllocSize maximum intended size of allocation.	  
	  */
	CObjectArenaAllocator(uint maxAllocSize, uint granularity = 4);
	// dtor
	~CObjectArenaAllocator();	
	/** Allocate a block with the given size. 0 is an invalid size an will cause an assert.
	  * If the size is > to the max size given at init, the allocation will succeed, but will use the standard allocator.
	  */
	void *alloc(uint size);
	// free an object that has previously been allocated with alloc. size should be remembered by the caller.
	void free(void *);
	// get the number of allocated objects
	uint getNumAllocatedBlocks() const;
	#ifdef SIP_DEBUG
		// for debug, useful to catch memory leaks
		void dumpUnreleasedBlocks();
		// set a break for the given allocation
		void setBreakForAllocID(bool enabled, uint id);
	#endif
	// for convenience, a default allocator is available
	static CObjectArenaAllocator &getDefaultAllocator();	

private:
	std::vector<CFixedSizeAllocator *> _ObjectSizeToAllocator;
	uint							 _MaxAllocSize;
	uint							 _Granularity;
	#ifdef SIP_DEBUG
		uint							 _AllocID;	
		std::map<void *, uint>			 _MemBlockToAllocID;
		bool							 _WantBreakOnAlloc;
		uint							 _BreakAllocID;
	#endif
	static CObjectArenaAllocator		 *_DefaultAllocator;
};

// Macro that redefines the new & delete operator of a class so that the default arena object allocator is used.
// This should be used inside the definition of the class.
// All derived class will use the same allocator, so this definition can be used only at the top of the hierachy of class for
// which it is of interest.
//
// NB : if you are using SIP_MEMORY, you will encounter a compilation error because of redefinition of the new operator.
//      to solve this, do as follow :
//
// #if !defined (SIP_USE_DEFAULT_MEMORY_MANAGER) && !defined (SIP_NO_DEFINE_NEW)
//	   #undef new
// #endif
// SIP_USES_DEFAULT_ARENA_OBJECT_ALLOCATOR // for fast alloc
// #if !defined (SIP_USE_DEFAULT_MEMORY_MANAGER) && !defined (SIP_NO_DEFINE_NEW)
//	   #define new SIP_NEW
// #endif	
#if !defined (SIP_USE_DEFAULT_MEMORY_MANAGER) && !defined (SIP_NO_DEFINE_NEW)
	#define SIP_USES_DEFAULT_ARENA_OBJECT_ALLOCATOR \
		void *operator new(size_t size, const char *filename, int line) { return SIPBASE::CObjectArenaAllocator::getDefaultAllocator().alloc((uint) size); }\
		void operator delete(void *block, const char *filename, int line) { SIPBASE::CObjectArenaAllocator::getDefaultAllocator().free(block); }            \
		void operator delete(void *block) { SIPBASE::CObjectArenaAllocator::getDefaultAllocator().free(block); }
#else	
	#define SIP_USES_DEFAULT_ARENA_OBJECT_ALLOCATOR \
		void *operator new(size_t size) { return SIPBASE::CObjectArenaAllocator::getDefaultAllocator().alloc((uint) size); }\
		void operator delete(void *block) { SIPBASE::CObjectArenaAllocator::getDefaultAllocator().free(block); }
#endif	

}

#endif
