/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include	"misc/stdmisc.h"
#include	"misc/contiguous_block_allocator.h"

namespace SIPBASE
{

//*********************************************************************************************************
CContiguousBlockAllocator::CContiguousBlockAllocator()
{
	_BlockStart = NULL;
	_NextAvailablePos = NULL;
	_BlockEnd = 0;
	_NumAllocatedBytes = 0;
	#ifdef SIP_DEBUG
		_NumAlloc = 0;
		_NumFree = 0;
	#endif
}

//*********************************************************************************************************
CContiguousBlockAllocator::~CContiguousBlockAllocator()
{
	init(0);
}

//*********************************************************************************************************
void CContiguousBlockAllocator::init(uint numBytes /*=0*/)
{
	if (_BlockStart) _DefaultAlloc.deallocate(_BlockStart, _BlockEnd - _BlockStart);
	_BlockEnd =  NULL;
	_BlockStart = NULL;
	_NumAllocatedBytes = 0;
	_NextAvailablePos = NULL;
	if (numBytes != 0)
	{
		_BlockStart = _DefaultAlloc.allocate(numBytes);
		_NextAvailablePos = _BlockStart;
		_BlockEnd = _BlockStart + numBytes;
		_NumAllocatedBytes = 0;
	}
	#ifdef SIP_DEBUG
		_NumAlloc = 0;
		_NumFree = 0;
	#endif
}

//*********************************************************************************************************
void *CContiguousBlockAllocator::alloc(uint numBytes)
{
	if (numBytes == 0) return NULL;
	_NumAllocatedBytes += numBytes;
	if (_BlockStart)
	{
		if (_NextAvailablePos + numBytes <= _BlockEnd)
		{
			uint8 *block = _NextAvailablePos;
			_NextAvailablePos += numBytes;
			#ifdef SIP_DEBUG
				++ _NumAlloc;
			#endif
			return block;
		}
	}
	// just uses standard new
	#ifdef SIP_DEBUG
		++ _NumAlloc;
	#endif
	return _DefaultAlloc.allocate(numBytes);
}

//*********************************************************************************************************
void CContiguousBlockAllocator::free(void *block, uint numBytes)
{
	if (!block) return;
	#ifdef SIP_DEBUG
		++ _NumFree;
	#endif
	// no-op if block not inside the big block (sub-block are never deallocated until init(0) is encountered)
	if (block < _BlockStart || block >= _BlockEnd)
	{
		// the block was allocated with std allocator
		_DefaultAlloc.deallocate((uint8 *) block, numBytes);
	}
}

} // SIPBASE
