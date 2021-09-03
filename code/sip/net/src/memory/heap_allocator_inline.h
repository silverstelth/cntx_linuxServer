/** \file memory/heap_allocator_inline.h
 * A Heap allocator, inline functions
 *
 * $Id: heap_allocator_inline.h,v 1.1 2008/06/26 13:41:17 RoMyongGuk Exp $
 */

#ifndef SIP_HEAP_ALLOCATOR_INLINE_H
#define SIP_HEAP_ALLOCATOR_INLINE_H

#ifdef SIP_HEAP_ALLOCATOR_INTERNAL_CHECKS
#define internalAssert(a) memory_assert(a)
#else // SIP_HEAP_ALLOCATOR_INTERNAL_CHECKS
#define internalAssert(a) ((void)0)
#endif // SIP_HEAP_ALLOCATOR_INTERNAL_CHECKS

#ifdef	SIP_HEAP_ALLOCATION_NDEBUG

#define SIP_UPDATE_MAGIC_NUMBER(node) ((void)0)
#define SIP_UPDATE_MAGIC_NUMBER_FREE_NODE(node) ((void)0)

#else // SIP_HEAP_ALLOCATION_NDEBUG

#define SIP_UPDATE_MAGIC_NUMBER(node) {\
		uint32 crc = evalMagicNumber (node);\
		*(node->EndMagicNumber) = crc;\
	}

#define SIP_UPDATE_MAGIC_NUMBER_FREE_NODE(node) {\
		if (node != &_NullNode.FreeNode) \
		{ \
			uint32 crc = evalMagicNumber (getNode (node));\
			*(getNode (node)->EndMagicNumber) = crc;\
		} \
	}

#endif // SIP_HEAP_ALLOCATION_NDEBUG

#if defined (SIP_OS_WINDOWS)
#define SIP_ALLOC_STOP(adr) { char _adr[512]; sprintf (_adr, "SIPMEMORY stop on block 0x%x\n", (int)adr); OutputDebugString (_adr); _asm { int 3 } }
#else
#define SIP_ALLOC_STOP(adr) { fprintf (stderr, "SIPMEMORY stop on block 0x%x\n", (int)adr); abort(); }
#endif

// *********************************************************
// Set / Get methods
// *********************************************************

inline bool	CHeapAllocator::isNodeFree		(const CNodeBegin *node)
{
	return (node->SizeAndFlags & CNodeBegin::Free) != 0;
}

// *********************************************************

inline bool	CHeapAllocator::isNodeUsed		(const CNodeBegin *node)
{
	return (node->SizeAndFlags & CNodeBegin::Free) == 0;
}

// *********************************************************

inline bool	CHeapAllocator::isNodeLast		(const CNodeBegin *node)
{
	return (node->SizeAndFlags & CNodeBegin::Last) != 0;
}

// *********************************************************

inline bool	CHeapAllocator::isNodeSmall		(const CNodeBegin *node)
{
#ifdef SIP_HEAP_NO_SMALL_BLOCK_OPTIMIZATION
	return false;
#else // SIP_HEAP_NO_SMALL_BLOCK_OPTIMIZATION
	return getNodeSize (node) <= LastSmallBlock;
#endif // SIP_HEAP_NO_SMALL_BLOCK_OPTIMIZATION
}

// *********************************************************

inline bool	CHeapAllocator::isNodeRed		(const CFreeNode *node)
{
	return (node->Flags & CFreeNode::Red) != 0;
}

// *********************************************************

inline bool	CHeapAllocator::isNodeBlack		(const CFreeNode *node)
{
	return (node->Flags & CFreeNode::Red) == 0;
}

// *********************************************************

inline void	CHeapAllocator::setNodeFree	(CNodeBegin *node)
{
	node->SizeAndFlags |= CNodeBegin::Free;
}

// *********************************************************

inline void	CHeapAllocator::setNodeUsed	(CNodeBegin *node)
{
	node->SizeAndFlags &= ~CNodeBegin::Free;
}

// *********************************************************

inline void	CHeapAllocator::setNodeLast	(CNodeBegin *node, bool last)
{
	if (last)
		node->SizeAndFlags |= CNodeBegin::Last;
	else
		node->SizeAndFlags &= ~CNodeBegin::Last;
}

// *********************************************************

inline void	CHeapAllocator::setNodeSize	(CNodeBegin *node, uint size)
{
	// Size must be < 1 Go
	internalAssert ((size&0xc0000000) == 0);

	// Set the size
	node->SizeAndFlags &= ~CNodeBegin::SizeMask;
	node->SizeAndFlags |= size;
}

// *********************************************************

inline void	CHeapAllocator::setNodeColor (CFreeNode *node, bool red)
{
	if (red)
		node->Flags |= CFreeNode::Red;
	else
		node->Flags &= ~CFreeNode::Red;
}

// *********************************************************

inline void	CHeapAllocator::setNodeRed (CFreeNode *node)
{
	node->Flags |= CFreeNode::Red;
}

// *********************************************************

inline void	CHeapAllocator::setNodeBlack (CFreeNode *node)
{
	node->Flags &= ~CFreeNode::Red;
}

// *********************************************************
// Free tree management
// *********************************************************

inline void CHeapAllocator::rotateLeft (CHeapAllocator::CFreeNode *x)
{   
	// Rotate node x to left

	// Establish x->Right link
	CHeapAllocator::CFreeNode *y = x->Right;
	x->Right = y->Left;
	if (y->Left != &_NullNode.FreeNode) 
	{
		y->Left->Parent = x;
		SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y->Left);
	}

	// Establish y->Parent link
	if (y != &_NullNode.FreeNode) 
		y->Parent = x->Parent;

	// x is the root ?
	if (x->Parent)
	{
		// Link its parent to y
		if (x == x->Parent->Left)
			x->Parent->Left = y;
		else
		{
			internalAssert (x == x->Parent->Right);
			x->Parent->Right = y;
		}
		SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x->Parent);
	}
	else
	{
		internalAssert (x == _FreeTreeRoot);
		_FreeTreeRoot = y;
	}

	// Link x and y
	y->Left = x;
	if (x != &_NullNode.FreeNode)
		x->Parent = y;
	SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x);
	SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y);
}

// *********************************************************

inline void CHeapAllocator::rotateRight (CHeapAllocator::CFreeNode *x)
{
	// Rotate node x to right

	// Establish x->Left link
	CHeapAllocator::CFreeNode *y = x->Left;
	x->Left = y->Right;
	if (y->Right != &_NullNode.FreeNode)
	{
		y->Right->Parent = x;
		SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y->Right);
	}

	// Establish y->parent link
	if (y != &_NullNode.FreeNode)
		y->Parent = x->Parent;

	// x is the root ?
	if (x->Parent)
	{
		// Link its parent to y
		if (x == x->Parent->Right)
		{
			x->Parent->Right = y;
		}
		else
		{
			internalAssert (x == x->Parent->Left);
			x->Parent->Left = y;
		}
		SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x->Parent);
	} 
	else 
	{        
		internalAssert (x == _FreeTreeRoot);
		_FreeTreeRoot = y;
	}

    // Link x and y
	y->Right = x;    
	if (x != &_NullNode.FreeNode) 
		x->Parent = y;

	// Crc
	SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x);
	SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y);
}

// *********************************************************

inline CHeapAllocator::CFreeNode *CHeapAllocator::find (uint size)
{
	CHeapAllocator::CFreeNode	*current = _FreeTreeRoot;
	CHeapAllocator::CFreeNode	*smallest = NULL;
	while (current != &_NullNode.FreeNode)
	{
		// Smaller than this node ?
		if (size <= getNodeSize (getNode (current)))
		{
			// This is good
			smallest = current;

			// Go left
			current = current->Left;
		}
		else
		{
			// Go right
			current = current->Right;
		}
	}
	return smallest;
}

// *********************************************************
// Node methods
// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

inline void CHeapAllocator::computeCRC32(uint32 &crc, const void* buffer, unsigned int count)
{
	internalAssert ( (count&3) == 0);
	count >>= 2;
	
	const uint32* ptr = (uint32*) buffer;
	while (count--)
	{
		crc ^= *(ptr++);
	}
/*	Real CRC, too slow
	
	const uint8* ptr = (uint8*) buffer;
	while (count--)
	{
		uint8 value = *(ptr++);
		crc ^= ((uint)value << 24);
		for (int i = 0; i < 8; i++)
		{
			if (crc & 0x80000000)
			{
				crc = (crc << 1) ^ 0x04C11DB7;
			}
			else
			{
				crc <<= 1;
			}
		}
	}
*/
}

#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

inline const CHeapAllocator::CNodeBegin *CHeapAllocator::getFirstNode (const CMainBlock *mainBlock)
{
	// Align the node pointer
	return (CNodeBegin*)(((uint32)mainBlock->Ptr&~(Align-1)) + ((((uint32)mainBlock->Ptr&(Align-1))==0)? 0 : Align));
}

// *********************************************************

inline CHeapAllocator::CNodeBegin *CHeapAllocator::getFirstNode (CMainBlock *mainBlock)
{
	// todo align
	return (CNodeBegin*)(((uint32)mainBlock->Ptr&~(Align-1)) + ((((uint32)mainBlock->Ptr&(Align-1))==0)? 0 : Align));
}

// *********************************************************

inline const CHeapAllocator::CNodeBegin *CHeapAllocator::getNextNode (const CNodeBegin *current)
{
	// Last ?
	if (isNodeLast (current))
		return NULL;

	// todo align
	return (const CNodeBegin*)((uint8*)current + sizeof (CNodeBegin) + SIP_HEAP_NODE_END_SIZE + getNodeSize (current) );
}

// *********************************************************

inline CHeapAllocator::CNodeBegin *CHeapAllocator::getNextNode (CNodeBegin *current)
{
	// Last ?
	if (isNodeLast (current))
		return NULL;

	// todo align
	return (CNodeBegin*)((uint8*)current + sizeof (CNodeBegin) + SIP_HEAP_NODE_END_SIZE + getNodeSize (current) );
}

// *********************************************************

inline const CHeapAllocator::CFreeNode *CHeapAllocator::getFreeNode	(const CNodeBegin *current)
{
	return (const CHeapAllocator::CFreeNode *)((uint8*)current + sizeof(CNodeBegin));
}

// *********************************************************

inline CHeapAllocator::CFreeNode *CHeapAllocator::getFreeNode (CNodeBegin *current)
{
	return (CHeapAllocator::CFreeNode *)((uint8*)current + sizeof(CNodeBegin));
}

// *********************************************************

inline const CHeapAllocator::CNodeBegin	*CHeapAllocator::getNode (const CFreeNode *current)
{
	return (const CHeapAllocator::CNodeBegin *)((uint8*)current - sizeof(CNodeBegin));
}

// *********************************************************

inline CHeapAllocator::CNodeBegin *CHeapAllocator::getNode (CFreeNode *current)
{
	return (CHeapAllocator::CNodeBegin *)((uint8*)current - sizeof(CNodeBegin));
}

// *********************************************************

inline uint CHeapAllocator::getNodeSize (const CNodeBegin *current)
{
	return current->SizeAndFlags & CNodeBegin::SizeMask;
}

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

inline uint32 CHeapAllocator::evalMagicNumber (const CNodeBegin *node)
{
	uint32 crc = (uint32)node;

	// Make the CRC of the rest of the node header
	computeCRC32 (crc, node, sizeof(CNodeBegin));

	// If the node is free and LARGE, crc the free tree node
	if ( isNodeFree (node) && !isNodeSmall (node) )
		computeCRC32 (crc, getFreeNode (node), sizeof(CFreeNode));

	// Return the magic number
	return crc;
}

#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************
// Synchronisation methods
// *********************************************************

inline void CHeapAllocator::enterCriticalSectionSB () const
{
	_MutexSB.enter ();
}

// *********************************************************

inline void CHeapAllocator::leaveCriticalSectionSB () const
{
	_MutexSB.leave ();
}

// *********************************************************

inline void CHeapAllocator::enterCriticalSectionLB () const
{
	_MutexLB.enter ();
}

// *********************************************************

inline void CHeapAllocator::leaveCriticalSectionLB () const
{
	_MutexLB.leave ();
}

// *********************************************************

inline void CHeapAllocator::enterCriticalSection () const
{
	enterCriticalSectionSB ();
	enterCriticalSectionLB ();
}

// *********************************************************

inline void CHeapAllocator::leaveCriticalSection () const
{
	leaveCriticalSectionLB ();
	leaveCriticalSectionSB ();
}

// *********************************************************






// *********************************************************
// *********************************************************

// Synchronized methods

// *********************************************************
// *********************************************************







// *********************************************************

inline uint CHeapAllocator::getMainBlockCount () const
{
	uint count = 0;
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		currentBlock = currentBlock->Next;
		count++;
	}
	return count;
}

// *********************************************************
// Debug functions
// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

/*inline void CHeapAllocator::debugAddBreakpoint (uint32 allocateNumber)
{
	_Breakpoints.insert (allocateNumber);
}*/

// *********************************************************

/*inline void CHeapAllocator::debugRemoveBreakpoints ()
{
	_Breakpoints.clear ();
}*/

#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

inline void	CHeapAllocator::setOutOfMemoryMode (TOutOfMemoryMode mode)
{
	enterCriticalSection ();

	_OutOfMemoryMode = mode;

	leaveCriticalSection ();
}

// *********************************************************

inline CHeapAllocator::TOutOfMemoryMode CHeapAllocator::getOutOfMemoryMode () const
{
	return _OutOfMemoryMode;
}

// *********************************************************

inline void	CHeapAllocator::setBlockAllocationMode (TBlockAllocationMode mode)
{
	enterCriticalSection ();

	_BlockAllocationMode = mode;

	leaveCriticalSection ();
}

// *********************************************************

inline CHeapAllocator::TBlockAllocationMode	CHeapAllocator::getBlockAllocationMode () const
{
	return _BlockAllocationMode;
}

// *********************************************************


inline const char *CHeapAllocator::getName () const
{
	return _Name;
}

// *********************************************************

inline uint CHeapAllocator::getMainBlockSize () const
{
	return _MainBlockSize;
}


// *********************************************************
// Small Block
// *********************************************************

inline CHeapAllocator::CNodeBegin *CHeapAllocator::getSmallBlock (CHeapAllocator::CSmallBlockPool *smallBlock, uint blockIndex)
{
	return (CHeapAllocator::CNodeBegin*)((uint8*)smallBlock + sizeof (CSmallBlockPool) + blockIndex * (sizeof(CNodeBegin) + smallBlock->Size + SIP_HEAP_NODE_END_SIZE) );
}

// *********************************************************

inline CHeapAllocator::CNodeBegin *CHeapAllocator::getNextSmallBlock (CNodeBegin *previous)
{
	return previous->Previous;
}

// *********************************************************

inline void CHeapAllocator::setNextSmallBlock (CNodeBegin *previous, CNodeBegin *next)
{
	previous->Previous = next;
}

// *********************************************************

#endif // SIP_HEAP_ALLOCATOR_H
