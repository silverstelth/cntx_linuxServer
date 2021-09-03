/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/object_arena_allocator.h"
#include "misc/fixed_size_allocator.h"

namespace SIPBASE
{


CObjectArenaAllocator *CObjectArenaAllocator::_DefaultAllocator = NULL;


//*****************************************************************************************************************
CObjectArenaAllocator::CObjectArenaAllocator(uint maxAllocSize, uint granularity /* = 4*/)
{
	sipassert(granularity > 0);
	sipassert(maxAllocSize > 0);
	_MaxAllocSize = granularity * ((maxAllocSize + (granularity - 1)) / granularity);
	_ObjectSizeToAllocator.resize(_MaxAllocSize / granularity, NULL);
	_Granularity = granularity;
	#ifdef SIP_DEBUG
		_AllocID = 0;
		_WantBreakOnAlloc = false;
		_BreakAllocID = 0;
	#endif
}

//*****************************************************************************************************************
CObjectArenaAllocator::~CObjectArenaAllocator()
{
	for(uint k = 0; k < _ObjectSizeToAllocator.size(); ++k)
	{
		delete _ObjectSizeToAllocator[k];
	}
}

//*****************************************************************************************************************
void *CObjectArenaAllocator::alloc(uint size)
{
	#ifdef SIP_DEBUG
		if (_WantBreakOnAlloc)
		{
			if (_AllocID == _BreakAllocID)
			{
				sipassert(0);
			}
		}
	#endif
	if (size >= _MaxAllocSize)
	{
		// use standard allocator
		uint8 *block = new uint8[size + sizeof(uint)]; // an additionnal uint is needed to store size of block 
		if (!block) return NULL;
		#ifdef SIP_DEBUG						
			_MemBlockToAllocID[block] = _AllocID;			
		#endif
		*(uint *) block = size;
		return block + sizeof(uint);
	}
	uint entry = ((size + (_Granularity - 1)) / _Granularity) ;
	sipassert(entry < _ObjectSizeToAllocator.size());
	if (!_ObjectSizeToAllocator[entry])
	{
		_ObjectSizeToAllocator[entry] = new CFixedSizeAllocator(entry * _Granularity + sizeof(uint), _MaxAllocSize / size); // an additionnal uint is needed to store size of block
	}
	void *block = _ObjectSizeToAllocator[entry]->alloc();	
	#ifdef SIP_DEBUG
		if (block)
		{		
			_MemBlockToAllocID[block] = _AllocID;
		}
		++_AllocID;
	#endif
	*(uint *) block = size;
	return (void *) ((uint8 *) block + sizeof(uint));
}

//*****************************************************************************************************************
void CObjectArenaAllocator::free(void *block)
{
	if (!block) return;
	uint8 *realBlock = (uint8 *) block - sizeof(uint); // a uint is used at start of block to give its size
	uint size = *(uint *) realBlock;
	if (size >= _MaxAllocSize)
	{
		#ifdef SIP_DEBUG
				std::map<void *, uint>::iterator it = _MemBlockToAllocID.find(realBlock);
				sipassert(it != _MemBlockToAllocID.end());
				_MemBlockToAllocID.erase(it);
		#endif
		delete realBlock;
		return;
	}
	uint entry = ((size + (_Granularity - 1)) / _Granularity);
	sipassert(entry < _ObjectSizeToAllocator.size());
	_ObjectSizeToAllocator[entry]->free(realBlock);
	#ifdef SIP_DEBUG
		std::map<void *, uint>::iterator it = _MemBlockToAllocID.find(realBlock);
		sipassert(it != _MemBlockToAllocID.end());
		/*
		#ifdef SIP_DEBUG
				if (_WantBreakOnAlloc)
				{
					if (it->second == _BreakAllocID)
					{
						sipassert(0);
					}
				}
		#endif
		*/
		_MemBlockToAllocID.erase(it);
	#endif
}

//*****************************************************************************************************************
uint CObjectArenaAllocator::getNumAllocatedBlocks() const
{
	uint numObjs = 0;
	for(uint k = 0; k < _ObjectSizeToAllocator.size(); ++k)
	{
		if (_ObjectSizeToAllocator[k]) numObjs += _ObjectSizeToAllocator[k]->getNumAllocatedBlocks();
	}
	return numObjs;
}

//*****************************************************************************************************************
CObjectArenaAllocator &CObjectArenaAllocator::getDefaultAllocator()
{
	if (!_DefaultAllocator)
	{
		_DefaultAllocator = new CObjectArenaAllocator(32768);
	}
	return *_DefaultAllocator;
}


#ifdef SIP_DEBUG

//*****************************************************************************************************************
void CObjectArenaAllocator::dumpUnreleasedBlocks()
{
	for(std::map<void *, uint>::iterator it = _MemBlockToAllocID.begin(); it != _MemBlockToAllocID.end(); ++it)
	{
		sipinfo("block %d at adress %x remains", (int) it->second, (int) ((uint8 *) it->first + sizeof(uint)));
	}
}

//*****************************************************************************************************************
void CObjectArenaAllocator::setBreakForAllocID(bool enabled, uint id)
{
	_WantBreakOnAlloc = enabled;
	_BreakAllocID = id;
}

#endif // SIP_DEBUG


} // SIPBASE

