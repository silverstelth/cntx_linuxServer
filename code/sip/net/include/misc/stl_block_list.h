/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#ifndef __STL_BLOCK_LIST_H__
#define __STL_BLOCK_LIST_H__

#include "types_sip.h"
#include "stl_block_allocator.h"
#include <list>

namespace SIPBASE {


// ***************************************************************************
/**
 * This class is a list<> which use CSTLBlockAllocator
 *
 * You construct such a list like this:
 *	CSTLBlockList<uint>		myList(ptrOnBlockMemory);
 *
 * \date 2001
 */
template <class T>
class CSTLBlockList : public std::list<T, CSTLBlockAllocator<T> >
{
public:

	typedef typename SIPBASE::CSTLBlockList<T>::size_type TSizeType;
	typedef typename SIPBASE::CSTLBlockList<T>::const_iterator TBlockListConstIt;

	explicit CSTLBlockList(CBlockMemory<T, false> *bm ) :
	std::list<T, CSTLBlockAllocator<T> >(CSTLBlockAllocator<T>(bm))
	{
	}

	explicit CSTLBlockList(TSizeType n, CBlockMemory<T, false> *bm ) :
	std::list<T, CSTLBlockAllocator<T> >(n,T(),CSTLBlockAllocator<T>(bm))
	{
	}

	explicit CSTLBlockList(TSizeType n, const T& v, CBlockMemory<T, false> *bm ) :
	std::list<T, CSTLBlockAllocator<T> >(n,v,CSTLBlockAllocator<T>(bm))
	{
	}


	CSTLBlockList(TBlockListConstIt first,TBlockListConstIt last, CBlockMemory<T, false> *bm ):
	std::list<T, CSTLBlockAllocator<T> >(first,last,CSTLBlockAllocator<T>(bm))
	{
	}

};



} // SIPBASE


#endif // __STL_BLOCK_LIST_H__

/* End of stl_block_list.h */
