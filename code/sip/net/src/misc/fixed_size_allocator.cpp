/*
-----------------------------------------------------------------------------
Copyright (c) 2007-2011 Cien Network Corporation
Copyright (c) 2007-2011 Samilpho Center
-----------------------------------------------------------------------------
*/

#include "misc/stdmisc.h"
#include "misc/fixed_size_allocator.h"

namespace SIPBASE
{

//*****************************************************************************************************************
CFixedSizeAllocator::CFixedSizeAllocator(uint numBytesPerBlock, uint numBlockPerChunk)
{
	_FreeSpace = NULL;
	_NumChunks = 0;
	sipassert(numBytesPerBlock > 1);
	_NumBytesPerBlock = numBytesPerBlock;
	_NumBlockPerChunk = std::max(numBlockPerChunk, (uint) 3);
	_NumAlloc = 0;
}

//*****************************************************************************************************************
CFixedSizeAllocator::~CFixedSizeAllocator()
{	
	if (_NumAlloc != 0)
	{
		#ifdef SIP_DEBUG
			sipwarning("%d blocks were not freed", (int) _NumAlloc);
		#endif
		return;
	}
	if (_NumChunks > 0)
	{		
		sipassert(_NumChunks == 1);
		// delete the left chunk. This should force all the left nodes to be removed from the empty list
		delete _FreeSpace->Chunk;
	}	
}

//*****************************************************************************************************************
void *CFixedSizeAllocator::alloc()
{
	if (!_FreeSpace)
	{				
		CChunk *chunk = new CChunk; // link a new chunk to that object
		chunk->init(this);
	}
	++ _NumAlloc;
	return _FreeSpace->unlink();		
}

//*****************************************************************************************************************
void CFixedSizeAllocator::free(void *block)
{
	if (!block) return;	
	/// get the node from the object	
	CNode *node = (CNode *) ((uint8 *) block - offsetof(CNode, Next));
	//	
	sipassert(node->Chunk != NULL);
	sipassert(node->Chunk->Allocator == this);
	//	
	--_NumAlloc;
	node->link();
}

//*****************************************************************************************************************
uint CFixedSizeAllocator::CChunk::getBlockSizeWithOverhead() const
{
	return std::max((uint)(sizeof(CNode) - offsetof(CNode, Next)),(uint)(Allocator->getNumBytesPerBlock())) + offsetof(CNode, Next);	
}

//*****************************************************************************************************************
CFixedSizeAllocator::CChunk::CChunk()
{
	NumFreeObjs = 0;
	Allocator = NULL;
}

//*****************************************************************************************************************
CFixedSizeAllocator::CChunk::~CChunk()
{		
	sipassert(Allocator != NULL);
	for (uint k = 0; k < Allocator->getNumBlockPerChunk(); ++k)
	{
		getNode(k).unlink();
	}
	sipassert(NumFreeObjs == 0);
	sipassert(Allocator->_NumChunks > 0);
	-- (Allocator->_NumChunks);
	delete Mem;
}

//*****************************************************************************************************************
void CFixedSizeAllocator::CChunk::init(CFixedSizeAllocator *alloc)
{
	sipassert(!Allocator);
	sipassert(alloc != NULL);
	Allocator = alloc;
	//
	Mem = new uint8[getBlockSizeWithOverhead() * alloc->getNumBlockPerChunk()];	
	//
	getNode(0).Chunk = this;
	getNode(0).Next = &getNode(1);
	getNode(0).Prev = &alloc->_FreeSpace;
	//
	NumFreeObjs = alloc->getNumBlockPerChunk();
	/// init all the element as a linked list
	for (uint k = 1; k < (NumFreeObjs - 1); ++k)
	{
		getNode(k).Chunk = this;
		getNode(k).Next = &getNode(k + 1);
		getNode(k).Prev = &getNode(k - 1).Next;
	}
	
	getNode(NumFreeObjs - 1).Chunk = this;
	getNode(NumFreeObjs - 1).Next  = alloc->_FreeSpace;
	getNode(NumFreeObjs - 1).Prev = &(getNode(NumFreeObjs - 2).Next);
	
	if (alloc->_FreeSpace) { alloc->_FreeSpace->Prev = &getNode(NumFreeObjs - 1).Next; }
	alloc->_FreeSpace = &getNode(0);
	++(alloc->_NumChunks);
}

//*****************************************************************************************************************
CFixedSizeAllocator::CNode &CFixedSizeAllocator::CChunk::getNode(uint index)
{
	sipassert(Allocator != NULL);
	sipassert(index < Allocator->getNumBlockPerChunk());
	return *(CNode *) ((uint8 *) Mem + index * getBlockSizeWithOverhead());
}

//*****************************************************************************************************************
void CFixedSizeAllocator::CChunk::add()
{
	sipassert(Allocator);
	// a node of this chunk has been given back
	sipassert(NumFreeObjs < Allocator->getNumBlockPerChunk());
	++ NumFreeObjs;
	if (NumFreeObjs == Allocator->getNumBlockPerChunk()) // all objects back ?
	{		
		if (Allocator->_NumChunks > 1) // we want to have at least one chunk left
		{
			delete this; // kill that object
		}		
	}
}

//*****************************************************************************************************************
void CFixedSizeAllocator::CChunk::grab()
{
	// a node of this chunk has been given back
	sipassert(NumFreeObjs > 0);
	-- NumFreeObjs;		
}

//*****************************************************************************************************************
void *CFixedSizeAllocator::CNode::unlink()
{			
	sipassert(Prev != NULL);
	if (Next) { Next->Prev = Prev;}
	*Prev = Next;
	sipassert(Chunk->NumFreeObjs > 0);
	Chunk->grab(); // tells the containing chunk that a node has been allocated
	return (void *) &Next;
}

//*****************************************************************************************************************
void CFixedSizeAllocator::CNode::link()
{
	// destroy the obj to get back uninitialized memory	
	sipassert(Chunk);
	sipassert(Chunk->Allocator);
	
	CNode *&head = Chunk->Allocator->_FreeSpace;
	Next = head;
	Prev = &head;
	if (Next)
	{
		sipassert(Next->Prev = &head);
		Next->Prev = &Next;
	}
	Chunk->Allocator->_FreeSpace = this;
	Chunk->add();
}

} // SIPBASE
