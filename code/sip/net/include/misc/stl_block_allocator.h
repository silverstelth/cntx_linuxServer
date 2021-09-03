/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STL_BLOCK_ALLOCATOR_H__
#define __STL_BLOCK_ALLOCATOR_H__

#include "types_sip.h"
#include "block_memory.h"


namespace SIPBASE {


// ***************************************************************************
/**
 * This class is a STL block allocator which use CBlockMemory. see CBlockMemory for description
 *	of block memory management/advantages.
 *
 * This class works with STLPort. It implements __stl_alloc_rebind() (not C++ standard??) to work properly
 *	with list<>/set<> etc... node allocations.
 *
 * NB: if used with a vector<> or a deque<> (ie if allocate(..,n) is called with n>1), it's still work, 
 *	but it's use malloc()/free() instead, so it is fully unusefull in this case :)
 *
 * CSTLBlockAllocator use a pointer on a CBlockMemory, so multiple containers can share the same 
 *	blockMemory, for maximum space/speed efficiency.
 *
 * Because of CBlockMemory allocation scheme, only same containers of same types can share the
 *	same CBlockMemory instance (eg: "list<uint, &myBlockMemory>; vector<uint, &myBlockMemory>;" is invalid and 
 *	will assert when allocations will occur).
 *
 * To construct a container which use this allocator, do like this:
 *	list<uint, CSTLBlockAllocator<uint>>		myList( ptrOnBlockMemory );
 *
 * But see CSTLBlockList for easier list instanciation, because using it, you'll do like this:
 *	CSTLBlockList<uint>		myList(ptrOnBlockMemory);
 *
 * Note: CSTLBlockAllocator take only 4 bytes in memory (a ptr on a CBlockMemory)
 *
 * \date 2001
 */
#if defined(SIP_OS_WINDOWS) && !defined(_STLP_MEMBER_TEMPLATE_CLASSES)

template<class T>
class CSTLBlockAllocator
{
public:

	/// \name standard allocator interface.
	// @{

	typedef T			value_type;
	typedef value_type *pointer;
	typedef const T*	const_pointer;
	typedef T&			reference;
	typedef const T&	const_reference;
	typedef size_t		size_type;
	typedef ptrdiff_t	difference_type;


	pointer	adress(reference x) const
	{
		return &x;
	}

	const_pointer	adress(const_reference x) const
	{
		return &x;
	}

	void	construct(pointer p, const T &val)
	{
		new (p) T(val);
	}

	void	destroy(pointer p)
	{
		p->T::~T();
	}

	// @}

public:

	/// Constructor. Must gives a blockMemory to ctor. NB: must gives a CBlockMemory<T, false> !!!
	CSTLBlockAllocator(CBlockMemory<T, false> *bm) : _BlockMemory(bm)
	{
	}
	/// copy ctor
	CSTLBlockAllocator(const CSTLBlockAllocator<T> &other) : _BlockMemory(other._BlockMemory)
	{
		// just copy the block memory from the other allocator.
	}
	/// dtor
	~CSTLBlockAllocator()
	{
		_BlockMemory= NULL;
	}


	pointer	allocate(size_type n, const_pointer hint= NULL)
	{
		if(n==0)
			return NULL;
		// If sizeof 1, use CBlockMemory allocation
		if(n==1)
		{
#ifdef SIP_DEBUG
			// verify that we allocate with good size!! (verify __stl_alloc_rebind scheme).
			// ie an allocator can be used only to allocate a kind of element
			uint	eltSize= std::max(sizeof(T), sizeof(void*));
			sipassert( eltSize == _BlockMemory->__stl_alloc_getEltSize() );
#endif
			// and allocate.
			return _BlockMemory->allocate();
		}
		// else use std malloc
		else
			return (T*)new uint8[n*sizeof(T)];
	}

	void	deallocate(void *p, size_type n)
	{
		if(n==0)
			return;
		// If sizeof 1, use CBlockMemory allocation
		if(n==1)
			_BlockMemory->free((T*)p);
		// else use std free
		else
			delete [] ((uint8*)p);
	}


	template <class _Tp, class U>
	CSTLBlockAllocator<U>& __stl_alloc_rebind(CSTLBlockAllocator<_Tp>& __a, const U*)
	{
		// must change the internal eltSize of __a.
		__a._BlockMemory->__stl_alloc_changeEltSize(sizeof(U));
		// and just typecast/return him
		return (CSTLBlockAllocator<U>&)(__a); 
	}

	template <class _Tp, class U>
	CSTLBlockAllocator<U> __stl_alloc_create(const CSTLBlockAllocator<_Tp>&, const U*)
	{
		return CSTLBlockAllocator<U>();
	}



// *******************
private:

	// The blockMemory used to allocate elements
	CBlockMemory<T, false>		*_BlockMemory;

};


#else // SIP_OS_WINDOWS

/// \todo yoyo: make it work under linux. For now, use std allocator instead...

# if !defined (__STL_USE_SGI_ALLOCATORS)
template<class T>
class CSTLBlockAllocator : public  std::allocator< T >
{
public:

	/// Constructor. Must gives a blockMemory to ctor. NB: must gives a CBlockMemory<T, false> !!!
	CSTLBlockAllocator(CBlockMemory<T, false> *bm)
	{
	}
	/// copy ctor
	CSTLBlockAllocator(const CSTLBlockAllocator<T> &other) : std::allocator<T>(other)
	{
	}
	/// dtor
	~CSTLBlockAllocator()
	{
	}

};
# else	// !defined (__STL_USE_SGI_ALLOCATORS)
class CSTLBlockAllocator : public  __sgi_alloc
{
public:

	/// Constructor. Must gives a blockMemory to ctor. NB: must gives a CBlockMemory<T, false> !!!
	CSTLBlockAllocator(CBlockMemory<T, false> *bm)
	{
	}
	/// copy ctor
	CSTLBlockAllocator(const CSTLBlockAllocator<T> &other) : __sgi_alloc(other)
	{
	}
	/// dtor
	~CSTLBlockAllocator()
	{
	}

};
# endif	// !defined (__STL_USE_SGI_ALLOCATORS)


#endif // SIP_OS_WINDOWS



} // SIPBASE


#endif // __STL_BLOCK_ALLOCATOR_H__

/* End of stl_block_allocator.h */
