/** \file memory/heap_allocator.cpp
 * A Heap allocator
 *
 * $Id: heap_allocator.cpp,v 1.1 2008/06/26 13:41:17 RoMyongGuk Exp $
 */

//#include <string>
#include "memory/memory_manager.h"
#include "heap_allocator.h"
#include <assert.h>
//#include <misc/debug.h>

#include <cstdio>
#include <stdlib.h>
#include <string.h>

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#endif // SIP_OS_WINDOWS

namespace SIPMEMORY 
{

// Include inlines functions
#include "./heap_allocator_inline.h"

#define SIP_HEAP_SB_CATEGORY "_SmlBlk"
#define SIP_HEAP_CATEGORY_BLOCK_CATEGORY "_MemCat"
#define SIP_HEAP_MEM_DEBUG_CATEGORY "_MemDb"
#define SIP_HEAP_UNKNOWN_CATEGORY "Unknown"

void CHeapAllocatorOutputError (const char *str)
{
	fprintf (stderr, str);
#ifdef SIP_OS_WINDOWS
	OutputDebugString (str);
#endif // SIP_OS_WINDOWS
}

// *********************************************************
// Constructors / desctrutors
// *********************************************************

CHeapAllocator::CHeapAllocator (uint mainBlockSize, uint blockCount, TBlockAllocationMode blockAllocationMode, 
				  TOutOfMemoryMode outOfMemoryMode)
{
	// Critical section
	enterCriticalSection ();

	// Allocator name
	_Name[0] = 0;

	// Check size of structures must be aligned
	internalAssert ((sizeof (CNodeBegin) & (Align-1)) == 0);
	internalAssert ((SIP_HEAP_NODE_END_SIZE & (Align-1)) == 0);
	internalAssert ((sizeof (CFreeNode) & (Align-1)) == 0);

	// Check small block sizes
	internalAssert ((FirstSmallBlock&(SmallBlockGranularity-1)) == 0);
	internalAssert ((LastSmallBlock&(SmallBlockGranularity-1)) == 0);

	_MainBlockList = NULL;
	_MainBlockSize = mainBlockSize;
	_BlockCount = blockCount;
	_BlockAllocationMode = blockAllocationMode;
	_OutOfMemoryMode = outOfMemoryMode;
	_FreeTreeRoot = &_NullNode.FreeNode;
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	_AlwaysCheck = false;
	_LogFile = NULL;
	_LogFileBlockSize = 0;
#endif // SIP_HEAP_ALLOCATION_NDEBUG

	_NullNode.FreeNode.Left = &_NullNode.FreeNode;
	_NullNode.FreeNode.Right = &_NullNode.FreeNode;
	_NullNode.FreeNode.Parent = NULL;

	setNodeBlack (&_NullNode.FreeNode);

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	_AllocateCount = 0;
#endif // SIP_HEAP_ALLOCATION_NDEBUG

	// *********************************************************
	// Small Block
	// *********************************************************

	// The free smallblock array by size
	const uint smallBlockSizeCount = SIP_SMALLBLOCK_COUNT;
	uint smallBlockSize;
	for (smallBlockSize=0; smallBlockSize<smallBlockSizeCount; smallBlockSize++)
	{
		_FreeSmallBlocks[smallBlockSize] = NULL;
	}

	// No small block
	_SmallBlockPool = NULL;

	// No out of memory callback
	_OutOfMemoryCallback = NULL;

	leaveCriticalSection ();
}

// *********************************************************

CHeapAllocator::~CHeapAllocator ()
{

	// Free the log file
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	if (_LogFile)
		debugEndAllocationLog ();
#endif // SIP_HEAP_ALLOCATION_NDEBUG

	// Release all memory used
	releaseMemory ();
}

// *********************************************************

void CHeapAllocator::insert (CHeapAllocator::CFreeNode *x)
{
    CHeapAllocator::CFreeNode *current, *parent;

    // Find future parent
	current = _FreeTreeRoot;
	parent = NULL;
	while (current != &_NullNode.FreeNode)
	{
		parent = current;
		current = (getNodeSize (getNode (x)) <= getNodeSize (getNode (current)) ) ? current->Left : current->Right;
	}

	// Setup new node
	x->Parent = parent;
	x->Left = &_NullNode.FreeNode;
	x->Right = &_NullNode.FreeNode;
	setNodeRed (x);

	// Insert node in tree
	if (parent)
	{
		if(getNodeSize (getNode (x)) <= getNodeSize (getNode (parent)))
			parent->Left = x;
		else
			parent->Right = x;
		SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (parent);
	}
	else
	{
		_FreeTreeRoot = x;
	}

	SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x);

    // Maintain Red-Black tree balance
    // After inserting node x
	// Check Red-Black properties

	while (x != _FreeTreeRoot && isNodeRed (x->Parent))
	{
		// We have a violation
		if (x->Parent == x->Parent->Parent->Left)
		{
			CHeapAllocator::CFreeNode *y = x->Parent->Parent->Right;
			if (isNodeRed (y))
			{
				// Uncle is RED
                setNodeBlack (x->Parent);
				setNodeBlack (y);
				setNodeRed (x->Parent->Parent);

				// Crc node
				SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y);
				SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x->Parent);
				SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x->Parent->Parent);

				x = x->Parent->Parent;
			}
			else
			{
                // Uncle is Black
				if (x == x->Parent->Right)
				{
                    // Make x a left child
					x = x->Parent;
                    rotateLeft(x);
				}

				// Recolor and rotate
				setNodeBlack (x->Parent);
				setNodeRed (x->Parent->Parent);

				rotateRight (x->Parent->Parent);

				// Crc node
				SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x->Parent);
			}
		}
		else
		{
			// Mirror image of above code
			CHeapAllocator::CFreeNode *y = x->Parent->Parent->Left;
			if (isNodeRed (y))
			{                
				// Uncle is Red
				setNodeBlack (x->Parent);
				setNodeBlack (y);
				setNodeRed (x->Parent->Parent);

				// Crc node
				SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y);
				SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x->Parent);
				SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x->Parent->Parent);

				x = x->Parent->Parent;
			}
			else
			{
                // Uncle is Black
                if (x == x->Parent->Left) 
				{
                    x = x->Parent;                    
					rotateRight(x);
                }
				setNodeBlack (x->Parent);
                setNodeRed (x->Parent->Parent);

                rotateLeft (x->Parent->Parent);

				// Crc node
				SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x->Parent);
			}        
		}    
	}
    setNodeBlack (_FreeTreeRoot);

	// Crc node
	SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (_FreeTreeRoot);
}

// *********************************************************

void CHeapAllocator::erase (CHeapAllocator::CFreeNode *z)
{
	CFreeNode *x, *y;
	if (z->Left == &_NullNode.FreeNode || z->Right == &_NullNode.FreeNode)
	{
		// y has a NULL node as a child
		y = z;
	}
	else
	{
		// Find tree successor with a &_NullNode.FreeNode node as a child
		y = z->Right;
		while (y->Left != &_NullNode.FreeNode)
			y = y->Left;
	}
	
	// x is y's only child
	if (y->Left != &_NullNode.FreeNode)
		x = y->Left;
	else
		x = y->Right;

	// Remove y from the parent chain
	x->Parent = y->Parent;

	if (y->Parent)
	{
		if (y == y->Parent->Left)
			y->Parent->Left = x;
		else
		{
			internalAssert (y == y->Parent->Right);
			y->Parent->Right = x;
		}
		SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y->Parent);
	}
	else
		_FreeTreeRoot = x;

	bool yRed = isNodeRed (y);

	if (y != z)
	{
		// Replace y by z
		*y = *z;
		setNodeColor (y, isNodeRed (z));
		if (y->Parent)
		{
			if (y->Parent->Left == z)
			{
				y->Parent->Left = y;
			}
			else
			{
				internalAssert (y->Parent->Right == z);
				y->Parent->Right = y;
			}
		}
		else
		{
			internalAssert (_FreeTreeRoot == z);
			_FreeTreeRoot = y;
		}

		if (y->Left)
		{
			internalAssert (y->Left->Parent == z);
			y->Left->Parent = y;
			
			SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y->Left);
		}
		if (y->Right)
		{
			internalAssert (y->Right->Parent == z);
			y->Right->Parent = y;
	
			SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y->Right);
		}
	}

	SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x);
	SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y);
	if (y->Parent)
		SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (y->Parent);

	if (!yRed)
	{
		// Maintain Red-Black tree balance
		// After deleting node x
		while (x != _FreeTreeRoot && isNodeBlack (x))
		{
			if (x == x->Parent->Left)
			{
				CFreeNode *w = x->Parent->Right;
				if (isNodeRed (w))
				{
					setNodeBlack (w);
					setNodeRed (x->Parent);
					rotateLeft (x->Parent);

					SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (w);

					w = x->Parent->Right;
				}
				if (isNodeBlack (w->Left) && isNodeBlack (w->Right))
				{
					setNodeRed (w);
					x = x->Parent;

					SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (w);
				}
				else
				{
					if (isNodeBlack (w->Right))
					{
						setNodeBlack (w->Left);
						setNodeRed (w);
						rotateRight (w);
						w = x->Parent->Right;
					}
					setNodeColor (w, isNodeRed (x->Parent));
					setNodeBlack (x->Parent);
					setNodeBlack (w->Right);
					rotateLeft (x->Parent);

					SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (w);
					SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (w->Right);

					x = _FreeTreeRoot;
				}
			}
			else
			{
				CFreeNode *w = x->Parent->Left;
				if (isNodeRed (w))
				{
					setNodeBlack (w);
					setNodeRed (x->Parent);
					rotateRight (x->Parent);

					SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (w);

					w = x->Parent->Left;
				}
				if ( isNodeBlack (w->Right) && isNodeBlack (w->Left) )
				{
					setNodeRed (w);
					x = x->Parent;

					SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (w);
				}
				else
				{
					if ( isNodeBlack (w->Left) )
					{
						setNodeBlack (w->Right);
						setNodeRed (w);
						rotateLeft (w);
						w = x->Parent->Left;
					}
					setNodeColor (w, isNodeRed (x->Parent) );
					setNodeBlack (x->Parent);
					setNodeBlack (w->Left);

					rotateRight (x->Parent);

					SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (w);
					SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (w->Left);

					x = _FreeTreeRoot;
				}
			}
		}
		setNodeBlack (x);
		SIP_UPDATE_MAGIC_NUMBER_FREE_NODE (x);
	}
}

// *********************************************************
// Node methods
// *********************************************************

CHeapAllocator::CNodeBegin *CHeapAllocator::splitNode (CNodeBegin *node, uint newSize)
{
	// Should be smaller than node size
	internalAssert (newSize <= getNodeSize (node));

	// Align size
	uint allignedSize = (newSize&~(Align-1)) + (( (newSize&(Align-1))==0 ) ? 0 : Align);
	if (allignedSize <= UserDataBlockSizeMin)
		allignedSize = UserDataBlockSizeMin;

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	// End magic number aligned on new size
	node->EndMagicNumber = (uint32*)((uint8*)node + newSize + sizeof (CNodeBegin));
#endif // SIP_HEAP_ALLOCATION_NDEBUG

	// Rest is empty ?
	if ( getNodeSize (node) - allignedSize < UserDataBlockSizeMin + sizeof (CNodeBegin) + SIP_HEAP_NODE_END_SIZE )
		// No split
		return NULL;

	// New node begin structure
	CNodeBegin *newNode = (CNodeBegin*)((uint8*)node + sizeof (CNodeBegin) + allignedSize + SIP_HEAP_NODE_END_SIZE );

	// Fill the new node header

	// Size
	setNodeSize (newNode, getNodeSize (node) - allignedSize - sizeof (CNodeBegin) - SIP_HEAP_NODE_END_SIZE);

	// Set the node free
	setNodeFree (newNode);

	// Set the previous node pointer
	newNode->Previous = node;

	// Last flag
	setNodeLast (newNode, isNodeLast (node));

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	// Begin markers
	memset (newNode->BeginMarkers, BeginNodeMarkers, CNodeBegin::MarkerSize-1);
	newNode->BeginMarkers[CNodeBegin::MarkerSize-1] = 0;

	// End pointer
	newNode->EndMagicNumber = (uint32*)((uint8*)newNode + getNodeSize (newNode) + sizeof (CNodeBegin));

	// End markers
	CNodeEnd *endNode = (CNodeEnd*)((uint8*)newNode + getNodeSize (newNode) + sizeof (CNodeBegin));
	memset (endNode->EndMarkers, EndNodeMarkers, CNodeEnd::MarkerSize-1);
	endNode->EndMarkers[CNodeEnd::MarkerSize-1] = 0;

	// No source informations
	newNode->File = NULL;
	newNode->Line = 0xffff;
	node->AllocateNumber = 0xffffffff;
	memset (newNode->Category, 0, CategoryStringLength);

	// Heap pointer
	newNode->Heap = this;
#endif // SIP_HEAP_ALLOCATION_NDEBUG

	// Get next node
	CNodeBegin *next = getNextNode (node);
	if (next)
	{
		// Set previous
		next->Previous = newNode;

		SIP_UPDATE_MAGIC_NUMBER (next);
	}

	// Should be big enough
	internalAssert (getNodeSize (newNode) >= UserDataBlockSizeMin);

	// New size of the first node
	setNodeSize (node, allignedSize);

	// No more the last
	setNodeLast (node, false);

	// Return new node
	return newNode;
}

// *********************************************************

void CHeapAllocator::mergeNode (CNodeBegin *node)
{
	// Get the previous node to merge with
	CNodeBegin *previous = node->Previous;
	internalAssert (getNextNode (previous) == node);
	internalAssert (previous);
	internalAssert (isNodeFree (previous));

	// New size
	uint newSize = getNodeSize (previous) + getNodeSize (node) + sizeof (CNodeBegin) + SIP_HEAP_NODE_END_SIZE;
	setNodeSize (previous, newSize);

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	// Set end pointers
	previous->EndMagicNumber = (uint32*)((uint8*)previous + getNodeSize (previous) + sizeof (CNodeBegin));
#endif // SIP_HEAP_ALLOCATION_NDEBUG

	// Get the next node to relink
	CNodeBegin *next = getNextNode (node);
	if (next)
	{
		// Relink
		next->Previous = previous;

		SIP_UPDATE_MAGIC_NUMBER (next);
	}

	// Get the last flag
	setNodeLast (previous, isNodeLast (node));

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	// Clear the footer of the previous node and the header of the current node (including the free node)
	memset ((uint8*)node-sizeof (CNodeEnd), DeletedMemory, sizeof (CFreeNode)+sizeof (CNodeBegin)+sizeof (CNodeEnd));
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}


// *********************************************************
// *********************************************************

// Synchronized methods

// *********************************************************
// *********************************************************


void CHeapAllocator::initEmptyBlock (CMainBlock& mainBlock)
{
	// Get the node pointer
	CNodeBegin *node = getFirstNode (&mainBlock);

	// Allocated size remaining after alignment
	internalAssert ((uint32)node - (uint32)mainBlock.Ptr >= 0);
	uint allocSize = mainBlock.Size - ((uint32)node - (uint32)mainBlock.Ptr);

	// *** Fill the new node header

	// User data size
	setNodeSize (node, allocSize-sizeof (CNodeBegin)-SIP_HEAP_NODE_END_SIZE);

	// Node is free
	setNodeFree (node);

	// Node is last
	setNodeLast (node, true);

	// No previous node
	node->Previous = NULL;

	// Debug info
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	// End magic number
	node->EndMagicNumber = (uint32*)((uint8*)node + getNodeSize (node) + sizeof (CNodeBegin));

	// Begin markers
	memset (node->BeginMarkers, BeginNodeMarkers, CNodeBegin::MarkerSize-1);
	node->BeginMarkers[CNodeBegin::MarkerSize-1] = 0;

	// End markers
	CNodeEnd *endNode = (CNodeEnd*)node->EndMagicNumber;
	memset (endNode->EndMarkers, EndNodeMarkers, CNodeEnd::MarkerSize-1);
	endNode->EndMarkers[CNodeEnd::MarkerSize-1] = 0;

	// Unallocated memory
	memset ((uint8*)node + sizeof(CNodeBegin), DeletedMemory, getNodeSize (node) );

	// No source file
	memset (node->Category, 0, CategoryStringLength);
	node->File = NULL;
	node->Line = 0xffff;
	node->AllocateNumber = 0xffffffff;

	// Heap pointer
	node->Heap = this;

	SIP_UPDATE_MAGIC_NUMBER (node);
#endif // SIP_HEAP_ALLOCATION_NDEBUG
}

// *********************************************************

uint CHeapAllocator::getBlockSize (void *block)
{
	// Get the node pointer
	CNodeBegin *node = (CNodeBegin*) ((uint)block - sizeof (CNodeBegin));

	return getNodeSize (((CNodeBegin*) ((uint)block - sizeof (CNodeBegin))));
}

// *********************************************************

const char * CHeapAllocator::getCategory (void *block)
{
	// Get the node pointer
	CNodeBegin *node = (CNodeBegin*) ((uint)block - sizeof (CNodeBegin));
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	return node->Category;
#else
	return NULL;
#endif
}

// *********************************************************

#ifdef SIP_HEAP_ALLOCATION_NDEBUG
void *CHeapAllocator::allocate (uint size)
#else // SIP_HEAP_ALLOCATION_NDEBUG
void *CHeapAllocator::allocate (uint size, const char *sourceFile, uint line, const char *category)
#endif // SIP_HEAP_ALLOCATION_NDEBUG
{
	// Allocates 0 byte ?
	if (size == 0)
	{
		// ********
		// * STOP *
		// ********
		// * Attempt to allocate 0 byte
		// ********
#ifdef SIP_HEAP_STOP_ALLOCATE_0
		SIP_ALLOC_STOP(NULL);
#endif // SIP_HEAP_STOP_ALLOCATE_0

		// C++ standards : an allocation of zero bytes should return a unique non-null pointer
		size = 1;
	}

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

	// If category is NULL
	if (category == NULL)
	{
		// Get the current category
		CCategory *cat = (CCategory*)_CategoryStack.getPointer ();
		if (cat)
		{
			category = cat->Name;
		}
		else
		{
			// Not yet initialised
			category = SIP_HEAP_UNKNOWN_CATEGORY;
		}
	}

	// Checks ?
	if (_AlwaysCheck)
	{
		// Check heap integrity
		internalCheckHeap (true, 0);
	}

	// Check breakpoints
	/*if (_Breakpoints.find (_AllocateCount) != _Breakpoints.end())
	{
		// ********
		// * STOP *
		// ********
		// * Breakpoints allocation
		// ********
		SIP_ALLOC_STOP(NULL);
	}*/
#endif // SIP_HEAP_ALLOCATION_NDEBUG

	void *finalPtr = NULL;

	// Small or largs block ?
#ifdef SIP_HEAP_NO_SMALL_BLOCK_OPTIMIZATION
	if (0)
#else // SIP_HEAP_NO_SMALL_BLOCK_OPTIMIZATION
	if (size <= LastSmallBlock)
#endif// SIP_HEAP_NO_SMALL_BLOCK_OPTIMIZATION
	{
		// *******************
		// Small block
		// *******************
		
		enterCriticalSectionSB ();

		// Get pointer on the free block list
		CNodeBegin **freeNode = (CNodeBegin **)_FreeSmallBlocks+SIP_SIZE_TO_SMALLBLOCK_INDEX (size);

		// Not found ?
		if (*freeNode == NULL)
		{
			leaveCriticalSectionSB ();

			// Size must be aligned
			uint alignedSize = SIP_ALIGN_SIZE_FOR_SMALLBLOCK (size);

			// Use internal allocator
			CSmallBlockPool *smallBlock = (CSmallBlockPool *)SipAlloc (*this, sizeof(CSmallBlockPool) + SmallBlockPoolSize * (sizeof(CNodeBegin) + alignedSize + 
				SIP_HEAP_NODE_END_SIZE), SIP_HEAP_SB_CATEGORY);

			enterCriticalSectionSB ();

			// Link this new block
			smallBlock->Size = alignedSize;
			smallBlock->Next = (CSmallBlockPool*)_SmallBlockPool;
			_SmallBlockPool = smallBlock;

			// Initialize the block
			uint pool;
			CNodeBegin *nextNode = *freeNode;
			for (pool=0; pool<SmallBlockPoolSize; pool++)
			{
				// Get the pool
				CNodeBegin *node = getSmallBlock (smallBlock, pool);

				// Set as free
				node->SizeAndFlags = alignedSize;

				// Insert in the list
				setNextSmallBlock (node, nextNode);
				nextNode = node;

				// Set debug informations
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
				// Set node free
				setNodeFree (node);
				
				// End magic number
				node->EndMagicNumber = (uint32*)(CNodeEnd*)((uint8*)node + getNodeSize (node) + sizeof (CNodeBegin));

				// Begin markers
				memset (node->BeginMarkers, BeginNodeMarkers, CNodeBegin::MarkerSize-1);
				node->BeginMarkers[CNodeBegin::MarkerSize-1] = 0;

				// End markers
				CNodeEnd *endNode = (CNodeEnd*)((uint8*)node + getNodeSize (node) + sizeof (CNodeBegin));
				memset (endNode->EndMarkers, EndNodeMarkers, CNodeEnd::MarkerSize-1);
				endNode->EndMarkers[CNodeEnd::MarkerSize-1] = 0;

				// Unallocated memory
				memset ((uint8*)node + sizeof(CNodeBegin), DeletedMemory, getNodeSize (node) );

				// No source file
				memset (node->Category, 0, CategoryStringLength);
				node->File = NULL;
				node->Line = 0xffff;
				node->AllocateNumber = 0xffffffff;

				// Heap pointer
				node->Heap = this;

				SIP_UPDATE_MAGIC_NUMBER (node);

#endif // SIP_HEAP_ALLOCATION_NDEBUG
			}

			// Link the new blocks
			*freeNode = nextNode;
		}

		// Check allocation as been done
		internalAssert (*freeNode);

		// Get a node
		CNodeBegin *node = *freeNode;

		// Checks
		internalAssert (size <= getNodeSize (node));
		internalAssert ((SIP_SIZE_TO_SMALLBLOCK_INDEX (size)) < (SIP_SMALLBLOCK_COUNT));

		// Relink
		*freeNode = getNextSmallBlock (node);

#ifndef SIP_HEAP_ALLOCATION_NDEBUG		
		// Check the node CRC
		debugCheckNode (node, evalMagicNumber (node));

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
#ifndef SIP_NO_DELETED_MEMORY_CONTENT_CHECK
		checkFreeBlockContent ((uint8*)node + sizeof(CNodeBegin), getNodeSize (node), true);
#endif // SIP_NO_DELETED_MEMORY_CONTENT_CHECK
#endif // SIP_HEAP_ALLOCATION_NDEBUG

		// Set node free for checks
		setNodeUsed (node);

		// Fill category
		strncpy (node->Category, category, CategoryStringLength-1);

		// Source filename
		node->File = sourceFile;

		// Source line
		node->Line = line;

		// Allocate count
		node->AllocateNumber = _AllocateCount++;
		//internalAssert (node->AllocateNumber != 245);

		// End magic number aligned on new size
		node->EndMagicNumber = (uint32*)((uint8*)node + size + sizeof (CNodeBegin));

		// Uninitialised memory
		memset ((uint8*)node + sizeof(CNodeBegin), UninitializedMemory, (uint32)(node->EndMagicNumber) - ( (uint32)node + sizeof(CNodeBegin) ) );

		// Crc node
		SIP_UPDATE_MAGIC_NUMBER (node);
#endif // SIP_HEAP_ALLOCATION_NDEBUG

		leaveCriticalSectionSB ();

		//printf("CHeapAllocator::allocate (%d, %s, %d, %s) = %p\n", size, sourceFile, line, (category == NULL ? "" : category), (void*)((uint)node + sizeof (CNodeBegin)));

		// Return the user pointer
		finalPtr = (void*)((uint)node + sizeof (CNodeBegin));
	}
	else
	{
		// *******************
		// Large block
		// *******************

		// Check size
		if ( (size & ~CNodeBegin::SizeMask) == 0)
		{
			enterCriticalSectionLB ();

			// Find a free node
			CHeapAllocator::CFreeNode *freeNode = CHeapAllocator::find (size);

			// The node
			CNodeBegin *node;

			// Node not found ?
			if (freeNode == NULL)
			{
				// Block allocation mode
				if ((_BlockAllocationMode == DontGrow) && (_MainBlockList != NULL))
				{
					leaveCriticalSectionLB ();
					outOfMemory();
					return NULL;
				}

				// The node
				uint8 *buffer;

				// Alloc size
				uint allocSize;

				// Aligned size
				uint allignedSize = (size&~(Align-1)) + (( (size&(Align-1))==0 ) ? 0 : Align);
				if (allignedSize < BlockDataSizeMin)
					allignedSize = BlockDataSizeMin;

				// Does the node bigger than mainNodeSize ?
				if (allignedSize > (_MainBlockSize-sizeof (CNodeBegin)-SIP_HEAP_NODE_END_SIZE))
					// Allocate a specific block
					allocSize = allignedSize + sizeof (CNodeBegin) + SIP_HEAP_NODE_END_SIZE;
				else
					// Allocate a new block
					allocSize = _MainBlockSize;

				// Allocate the buffer
				buffer = internalAllocateBlock (allocSize+Align);

				// Out of memory ?
				if (!buffer)
				{
					leaveCriticalSectionLB ();
					outOfMemory();
					return NULL;
				}

				// Add the buffer
				CMainBlock *mainBlock = (CMainBlock*)internalAllocateBlock (sizeof(CMainBlock)); 

				// Out of memory ?
				if (!mainBlock)
				{
					leaveCriticalSectionLB ();
					outOfMemory();
					return NULL;
				}

				mainBlock->Size = allocSize+Align;
				mainBlock->Ptr = buffer;
				mainBlock->Next = _MainBlockList;
				_MainBlockList = mainBlock;

				// Init the new block
				initEmptyBlock (*mainBlock);

				// Get the first node
				node = getFirstNode (mainBlock);
			}
			else
			{
				// Get the node
				node = getNode (freeNode);

				// Remove the node from free blocks and get the removed block
				erase (freeNode);
			}

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
			// Check the node CRC
			debugCheckNode (node, evalMagicNumber (node));

#endif // SIP_HEAP_ALLOCATION_NDEBUG

			// Split the node
			CNodeBegin *rest = splitNode (node, size);

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
#ifndef SIP_NO_DELETED_MEMORY_CONTENT_CHECK
			checkFreeBlockContent ((uint8*)node + sizeof(CNodeBegin) + sizeof(CFreeNode), getNodeSize (node) - sizeof(CFreeNode), true);
#endif // SIP_NO_DELETED_MEMORY_CONTENT_CHECK
#endif // SIP_HEAP_ALLOCATION_NDEBUG

			// Fill informations for the first part of the node

			// Clear free flag
			setNodeUsed (node);

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
			// Fill category
			strncpy (node->Category, category, CategoryStringLength-1);

			// Source filename
			node->File = sourceFile;

			// Source line
			node->Line = line;

			// Allocate count
			node->AllocateNumber = _AllocateCount++;
			internalAssert (node->AllocateNumber != 245);
			
			// Crc node
			SIP_UPDATE_MAGIC_NUMBER (node);

			// Uninitialised memory
			memset ((uint8*)node + sizeof(CNodeBegin), UninitializedMemory, (uint32)(node->EndMagicNumber) - ( (uint32)node + sizeof(CNodeBegin) ) );
#endif // SIP_HEAP_ALLOCATION_NDEBUG

			// Node has been splited ?
			if (rest)
			{
				// Fill informations for the second part of the node

				// Get the freeNode
				freeNode = getFreeNode (rest);

				// Insert the free node
				insert (freeNode);

				// Crc node
				SIP_UPDATE_MAGIC_NUMBER (rest);
			}

			// Check the node size
			internalAssert ( size <= getNodeSize (node) );

			uint	asz = ((uint)BlockDataSizeMin > size + (uint)Align) ? (uint)BlockDataSizeMin : size + (uint)Align;
			internalAssert ( asz + sizeof (CNodeBegin) + sizeof (CNodeEnd) + sizeof (CNodeBegin) + sizeof (CNodeEnd) + BlockDataSizeMin >= getNodeSize (node) );

			// Check pointer alignment
			internalAssert (((uint32)node&(Align-1)) == 0);
			internalAssert (((uint32)((char*)node + sizeof(CNodeBegin))&(Align-1)) == 0);

			// Check size
			internalAssert ((uint32)node->EndMagicNumber <= (uint32)((uint8*)node+sizeof(CNodeBegin)+getNodeSize (node) ));
			internalAssert ((uint32)node->EndMagicNumber > (uint32)(((uint8*)node+sizeof(CNodeBegin)+getNodeSize (node) ) - BlockDataSizeMin - BlockDataSizeMin - sizeof(CNodeBegin) - sizeof(CNodeEnd)));

			leaveCriticalSectionLB ();

			//printf("CHeapAllocator::allocate (%d, %s, %d, %s) = %p\n", size, sourceFile, line, (category == NULL ? "" : category), (void*)((uint)node + sizeof (CNodeBegin)));

			// Return the user pointer
			finalPtr = (void*)((uint)node + sizeof (CNodeBegin));
		}
		else
		{
			// ********
			// * STOP *
			// ********
			// * Attempt to allocate more than 1 Go
			// ********
			SIP_ALLOC_STOP(NULL);

			outOfMemory();
			return NULL;
		}
	}

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	if (finalPtr && _LogFile)
	{
		uint32 smallSize = (size<=LastSmallBlock)?SIP_ALIGN_SIZE_FOR_SMALLBLOCK (size):size;
		if (smallSize == _LogFileBlockSize)
			debugLog (finalPtr, category);
	}
#endif // SIP_HEAP_ALLOCATION_NDEBUG
	return finalPtr;
}

// *********************************************************

void CHeapAllocator::free (void *ptr)
{
	//printf("CHeapAllocator::free (%p)\n", ptr);
	// Delete a null pointer ?
	if (ptr == NULL)
	{
		// ********
		// * STOP *
		// ********
		// * Attempt to delete a NULL pointer
		// ********
#ifdef SIP_HEAP_STOP_NULL_FREE
		SIP_ALLOC_STOP(NULL);
#endif // SIP_HEAP_STOP_NULL_FREE
		// C++ standards : deletion of a null pointer should quietly do nothing.
	}
	else
	{

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
		// Checks ?
		if (_AlwaysCheck)
		{
			// Check heap integrity
			internalCheckHeap (true, 0);
		}
#endif // SIP_HEAP_ALLOCATION_NDEBUG
		
		// Get the node pointer
		CNodeBegin *node = (CNodeBegin*) ((uint)ptr - sizeof (CNodeBegin));

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
		// Check the node CRC
		enterCriticalSectionSB ();
		enterCriticalSectionLB ();
		debugCheckNode (node, evalMagicNumber (node));
		leaveCriticalSectionLB ();
		leaveCriticalSectionSB ();
#endif // SIP_HEAP_ALLOCATION_NDEBUG

		// Large or small block ?
#ifdef SIP_HEAP_ALLOCATION_NDEBUG
		uint size = (((CNodeBegin*) ((uint)ptr - sizeof (CNodeBegin))))->SizeAndFlags;
#else // SIP_HEAP_ALLOCATION_NDEBUG
		uint size = getNodeSize (((CNodeBegin*) ((uint)ptr - sizeof (CNodeBegin))));
#endif // SIP_HEAP_ALLOCATION_NDEBUG
		if (size <= LastSmallBlock)
		{
			// *******************
			// Small block
			// *******************

			// Check the node has not been deleted
			if (isNodeFree (node))
			{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
				debugStopNodeAlreadyFree (node);
#endif // SIP_HEAP_ALLOCATION_NDEBUG
			}
			else
			{
				enterCriticalSectionSB ();

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
				// Uninitialised memory
				memset ((uint8*)node + sizeof(CNodeBegin), DeletedMemory, size );

				// Set end pointers
				node->EndMagicNumber = (uint32*)((uint8*)node + size + sizeof (CNodeBegin));

				// Mark has free
				setNodeFree (node);
#endif // SIP_HEAP_ALLOCATION_NDEBUG

				// Add in the free list
				CNodeBegin **freeNode = (CNodeBegin **)_FreeSmallBlocks+SIP_SIZE_TO_SMALLBLOCK_INDEX (size);
				((CNodeBegin*) ((uint)ptr - sizeof (CNodeBegin)))->Previous = *freeNode;
				*freeNode = ((CNodeBegin*) ((uint)ptr - sizeof (CNodeBegin)));

				// Update smallblock crc
				SIP_UPDATE_MAGIC_NUMBER (node);

				leaveCriticalSectionSB ();
			}
		}
		else
		{
#ifdef SIP_HEAP_ALLOCATION_NDEBUG
			// Get the real size
			size = getNodeSize (((CNodeBegin*) ((uint)ptr - sizeof (CNodeBegin))));
#endif // SIP_HEAP_ALLOCATION_NDEBUG

			// Get the node pointer
			CNodeBegin *node = (CNodeBegin*) ((uint)ptr - sizeof (CNodeBegin));

			// Check the node has not been deleted
			if (isNodeFree (node))
			{
				// ********
				// * STOP *
				// ********
				// * Attempt to delete a pointer already deleted
				// ********
				// * (*node):	the already deleted node
				// ********
				SIP_ALLOC_STOP(node);
			}
			else
			{
				enterCriticalSectionLB ();

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
				// Uninitialised memory
				memset ((uint8*)node + sizeof(CNodeBegin) + sizeof(CFreeNode), DeletedMemory, size - sizeof(CFreeNode));

				// Set end pointers
				node->EndMagicNumber = (uint32*)((uint8*)node + size + sizeof (CNodeBegin));
#endif // SIP_HEAP_ALLOCATION_NDEBUG

				// Mark has free
				setNodeFree (node);

				// *******************
				// Large block
				// *******************

				// A free node
				CHeapAllocator::CFreeNode *freeNode = NULL;

				// Previous node
				CNodeBegin *previous = node->Previous;
				if (previous)
				{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
					// Check the previous node
					debugCheckNode (previous, evalMagicNumber (previous));
#endif // SIP_HEAP_ALLOCATION_NDEBUG

					// Is it free ?
					if (isNodeFree (previous))
					{
						// Merge the two nodes
						mergeNode (node);

						// Get its free node
						erase (getFreeNode (previous));

						// Curent node
						node = previous;
					}
				}

				// Mark has free
				setNodeFree (node);

				// Next node
				CNodeBegin *next = getNextNode (node);
				if (next)
				{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
					// Check the next node
					debugCheckNode (next, evalMagicNumber (next));
#endif // SIP_HEAP_ALLOCATION_NDEBUG

					// Is it free ?
					if (isNodeFree (next))
					{
						// Free the new one
						erase (getFreeNode (next));

						// Merge the two nodes
						mergeNode (next);
					}
				}

				// Insert it into the tree
				insert (getFreeNode (node));

				SIP_UPDATE_MAGIC_NUMBER (node);
				
				leaveCriticalSectionLB ();
			}
		}
	}
}

// *********************************************************
// Statistics
// *********************************************************

uint CHeapAllocator::getAllocatedMemory () const
{
	enterCriticalSection ();

	// Sum allocated memory
	uint memory = 0;

	// For each small block
	CSmallBlockPool *currentSB = (CSmallBlockPool *)_SmallBlockPool;
	while (currentSB)
	{
		// For each node in this small block pool
		uint block;
		for (block=0; block<SmallBlockPoolSize; block++)
		{
			// Get the node
			const CNodeBegin *current = getSmallBlock (currentSB, block);

			// Node allocated ?
			if (isNodeUsed (current))
				memory += getNodeSize (current) + ReleaseHeaderSize;
		}

		// Next block
		currentSB = currentSB->Next;
	}

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Get the first node
		const CNodeBegin *current = getFirstNode (currentBlock);
		while (current)
		{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
			// Check node
			debugCheckNode (current, evalMagicNumber (current));
#endif // SIP_HEAP_ALLOCATION_NDEBUG

			// Node allocated ? Don't sum small blocks..
			if (isNodeUsed (current))
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
				if (strcmp (current->Category, SIP_HEAP_SB_CATEGORY) != 0)
#endif // SIP_HEAP_ALLOCATION_NDEBUG
					memory += getNodeSize (current) + ReleaseHeaderSize;

			// Next node
			current = getNextNode (current);
		}

		// Next block
		currentBlock = currentBlock->Next;
	}

	leaveCriticalSection ();

	// Return memory used
	return memory;
}

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
uint CHeapAllocator::debugGetAllocatedMemoryByCategory (const char* category) const
{
	enterCriticalSection ();

	// Sum allocated memory
	uint memory = 0;

	// For each small block
	CSmallBlockPool *currentSB = (CSmallBlockPool *)_SmallBlockPool;
	while (currentSB)
	{
		// For each node in this small block pool
		uint block;
		for (block=0; block<SmallBlockPoolSize; block++)
		{
			// Get the node
			const CNodeBegin *current = getSmallBlock (currentSB, block);

			// Node allocated ?
			if ((isNodeUsed (current)) && (strcmp (current->Category, category)==0))
				memory += getNodeSize (current);

			memory += getNodeSize (current);
		}

		// Next block
		currentSB = currentSB->Next;
	}

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Get the first node
		const CNodeBegin *current = getFirstNode (currentBlock);
		while (current)
		{
			// Node allocated ?
			if ((isNodeUsed (current)) && (strcmp (current->Category, category)==0))
				memory += getNodeSize (current);

			// Next node
			current = getNextNode (current);
		}

		// Next block
		currentBlock = currentBlock->Next;
	}

	leaveCriticalSection ();

	// Return memory used
	return memory;
}
#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
uint CHeapAllocator::debugGetDebugInfoSize () const
{
	// Return memory used
	return debugGetSBDebugInfoSize () + debugGetLBDebugInfoSize ();
}

uint CHeapAllocator::debugGetLBDebugInfoSize () const
{
	enterCriticalSection ();

	// Sum memory used by debug header
	uint memory = 0;

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Get the first node
		const CNodeBegin *current = getFirstNode (currentBlock);
		while (current)
		{
			// Node allocated ?
			memory  += sizeof(CNodeBegin) - ReleaseHeaderSize + sizeof(CNodeEnd);

			// Next node
			current = getNextNode (current);
		}

		// Next block
		currentBlock = currentBlock->Next;
	}

	leaveCriticalSection ();

	// Return memory used
	return memory;
}

uint CHeapAllocator::debugGetSBDebugInfoSize () const
{
	enterCriticalSection ();

	// Sum memory used by debug header
	uint memory = 0;

	// For each small blocks
	CSmallBlockPool	*pool = (CSmallBlockPool*)_SmallBlockPool;
	while (pool)
	{
		memory  += SmallBlockPoolSize * (sizeof(CNodeBegin) - ReleaseHeaderSize + sizeof(CNodeEnd));

		// Next pool
		pool = pool->Next;
	}

	leaveCriticalSection ();

	// Return memory used
	return memory;
}
#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

void fprintf_int (uint value)
{
	
}

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

class CCategoryMap
{
public:
	CCategoryMap (const char *name, CCategoryMap *next)
	{
		Name = name;
		BlockCount = 0;
		Size = 0;
		Min = 0xffffffff;
		Max = 0;
		Next = next;
	}

	static CCategoryMap *find (CCategoryMap *cat, const char *name)
	{
		while (cat)
		{
			if (strcmp (name, cat->Name) == 0)
				break;
			cat = cat->Next;
		}
		return cat;
	}

	static CCategoryMap *insert (const char *name, CCategoryMap *next)
	{
		return new CCategoryMap (name, next);
	}

	const char		*Name;
	uint			BlockCount;
	uint			Size;
	uint			Min;
	uint			Max;
	CCategoryMap	*Next;
};

bool CHeapAllocator::debugStatisticsReport (const char* stateFile, bool memoryMap)
{
	// Status
	bool status = false;

	debugPushCategoryString (SIP_HEAP_MEM_DEBUG_CATEGORY);

	// Open files
	FILE *file = fopen (stateFile, "wt");

	// List of category
	CCategoryMap *catMap = NULL;

	// Both OK
	if (file)
	{
		// **************************

		// For each small block
		uint smallBlockCount = 0;
		uint largeBlockCount = 0;
		CSmallBlockPool *currentSB = (CSmallBlockPool *)_SmallBlockPool;
		while (currentSB)
		{
			// For each node in this small block pool
			uint block;
			for (block=0; block<SmallBlockPoolSize; block++)
			{
				// Get the node
				const CNodeBegin *current = getSmallBlock (currentSB, block);

				// Node allocated ?
				if (isNodeUsed (current))
				{
					// Find the node
					CCategoryMap *cat = CCategoryMap::find (catMap, (const char*)current->Category);

					// Found ?
					if (cat == NULL)
					{
						catMap = CCategoryMap::insert (current->Category, catMap);
						cat = catMap;
					}
					uint size = getNodeSize (current) + ReleaseHeaderSize;
					cat->BlockCount++;
					cat->Size += size;
					if (size < cat->Min)
						cat->Min = size;
					if (size > cat->Max)
						cat->Max = size;
				}

				// Next node
				smallBlockCount++;
			}

			// Next block
			currentSB = currentSB->Next;
		}

		// For each main block
		CMainBlock *currentBlock = _MainBlockList;
		while (currentBlock)
		{
			// Get the first node
			const CNodeBegin *current = getFirstNode (currentBlock);
			while (current)
			{
				// Node is used ?
				if (isNodeUsed (current))
				{
					// Find the node
					CCategoryMap *cat = CCategoryMap::find (catMap, (const char*)current->Category);

					// Found ?
					if (cat == NULL)
					{
						catMap = CCategoryMap::insert (current->Category, catMap);
						cat = catMap;
					}
					uint size = getNodeSize (current) + ReleaseHeaderSize;
					cat->BlockCount++;
					cat->Size += size;
					if (size < cat->Min)
						cat->Min = size;
					if (size > cat->Max)
						cat->Max = size;
				}

				// Next node
				current = getNextNode (current);
				largeBlockCount++;
			}

			// Next block
			currentBlock = currentBlock->Next;
		}

		// Write the heap info file
		fprintf (file, "HEAP STATISTICS\n");
		fprintf (file, "HEAP, TOTAL MEMORY USED, ALLOCATED MEMORY, FREE MEMORY, FRAGMENTATION RATIO, MAIN BLOCK SIZE, MAIN BLOCK COUNT\n");

		fprintf (file, "%s, %d, %d, %d, %f%%, %d, %d\n", _Name, getTotalMemoryUsed (),
			getAllocatedMemory (), getFreeMemory (), 100.f*getFragmentationRatio (), getMainBlockSize (), getMainBlockCount ());

		fprintf (file, "\n\nHEAP BLOCKS\n");
		fprintf (file, "SMALL BLOCK MEMORY, SMALL BLOCK COUNT, LARGE BLOCK COUNT\n");

		fprintf (file, "%d, %d, %d\n", getSmallBlockMemory (), smallBlockCount, largeBlockCount);

		fprintf (file, "\n\nHEAP DEBUG INFOS\n");
		fprintf (file, "SB DEBUG INFO, LB DEBUG INFO, TOTAL DEBUG INFO\n");

		fprintf (file, "%d, %d, %d\n", debugGetSBDebugInfoSize (), debugGetLBDebugInfoSize (), debugGetDebugInfoSize ());

		// **************************

		// Write the system heap info file
		uint systemMemory = getAllocatedSystemMemory ();
		uint hookedSystemMemory = GetAllocatedSystemMemoryHook ();
		uint sipSystemMemory = getAllocatedSystemMemoryByAllocator ();

		fprintf (file, "\n\nSYSTEM HEAP STATISTICS\n");
		fprintf (file, "SYSTEM HEAP ALLOCATED MEMORY, TOTAL HOOKED MEMORY, SIP ALLOCATED MEMORY, OTHER ALLOCATED MEMORY, NOT HOOKED MEMORY\n");
		fprintf (file, "%d, %d, %d, %d, %d\n", systemMemory, hookedSystemMemory, sipSystemMemory, hookedSystemMemory-sipSystemMemory, systemMemory-hookedSystemMemory);
		
		// **************************
		
		// Write the category map file
		fprintf (file, "\n\n\nCATEGORY STATISTICS\n");
		fprintf (file, "CATEGORY, BLOCK COUNT, MEMORY ALLOCATED, MIN BLOCK SIZE, MAX BLOCK SIZE, AVERAGE BLOCK SIZE, SB COUNT 8, SB COUNT 16, SB COUNT 24, SB COUNT 32, SB COUNT 40, SB COUNT 48, SB COUNT 56, SB COUNT 64, SB COUNT 72, SB COUNT 80, SB COUNT 88, SB COUNT 96, SB COUNT 104, SB COUNT 112, SB COUNT 120, SB COUNT 128\n");

		CCategoryMap *cat = catMap;
		while (cat)
		{
			CCategoryMap *next = cat->Next;

			// Number of small blocks
			uint smallB[SIP_SMALLBLOCK_COUNT];

			// Clean
			uint smallBlock;
			for (smallBlock=0; smallBlock<SIP_SMALLBLOCK_COUNT; smallBlock++)
			{
				smallB[smallBlock] = 0;
			}
			
			// Scan small block for this category
			currentSB = (CSmallBlockPool *)_SmallBlockPool;
			while (currentSB)
			{
				// For each node in this small block pool
				uint block;
				for (block=0; block<SmallBlockPoolSize; block++)
				{
					// Get the node
					const CNodeBegin *current = getSmallBlock (currentSB, block);

					// Node allocated ?
					if (isNodeUsed (current))
					{
						// Good node ?
						if (strcmp (current->Category, cat->Name) == 0)
						{
							// Get the small block index
							uint index = SIP_SIZE_TO_SMALLBLOCK_INDEX (getNodeSize (current));

							// One more node
							smallB[index]++;
						}
					}
				}

				// Next block
				currentSB = currentSB->Next;
			}

			// Average
			uint average = cat->Size / cat->BlockCount;

			// Print the line
			fprintf (file, "%s, %d, %d, %d, %d, %d", cat->Name, cat->BlockCount, cat->Size, 
				cat->Min, cat->Max, average);

			// Print small blocks
			for (smallBlock=0; smallBlock<SIP_SMALLBLOCK_COUNT; smallBlock++)
			{
				fprintf (file, ", %d", smallB[smallBlock]);
			}

			fprintf (file, "\n");

			delete cat;
			cat = next;
		}

		// **************************

		// Write the small block statistics
		fprintf (file, "\n\n\nSMALL BLOCK STATISTICS\n");
		fprintf (file, "SIZE, BLOCK COUNT, BLOCK FREE, BLOCK USED, TOTAL MEMORY USED, SIP MEMORY (NO DB), FREE MEMORY (NO DB)\n");

		// Number of small blocks
		uint count[SIP_SMALLBLOCK_COUNT];
		uint free[SIP_SMALLBLOCK_COUNT];

		uint smallBlock;
		for (smallBlock=0; smallBlock<SIP_SMALLBLOCK_COUNT; smallBlock++)
		{
			count[smallBlock] = 0;
			free[smallBlock] = 0;
		}

		// For each small block
		currentSB = (CSmallBlockPool *)_SmallBlockPool;
		while (currentSB)
		{
			// For each node in this small block pool
			uint block;
			for (block=0; block<SmallBlockPoolSize; block++)
			{
				// Get the node
				const CNodeBegin *current = getSmallBlock (currentSB, block);

				// Get the small block index
				uint index = SIP_SIZE_TO_SMALLBLOCK_INDEX (getNodeSize (current));

				// Add a block
				count[index]++;

				// Node allocated ?
				if (isNodeFree (current))
				{
					// Add a free block
					free[index]++;
				}

				// Next node
				current = getNextNode (current);
			}

			// Next block
			currentSB = currentSB->Next;
		}

		// Print stats
		for (smallBlock=0; smallBlock<SIP_SMALLBLOCK_COUNT; smallBlock++)
		{
			uint size = (smallBlock+1)*SmallBlockGranularity;
			// The actual memory taken with no DB is size+ReleaseHeaderSize
			fprintf (file,"%d, %d, %d, %d, %d, %d, %d\n",size, count[smallBlock], free[smallBlock], 
				count[smallBlock]-free[smallBlock], 
				count[smallBlock]*(sizeof (CNodeBegin) + size + SIP_HEAP_NODE_END_SIZE),
				(count[smallBlock]-free[smallBlock])*(size+ReleaseHeaderSize),
				free[smallBlock]*(size+ReleaseHeaderSize));
		}
		
		// **************************

		// Write the memory map file
		if (memoryMap)
		{
			fprintf (file, "\n\n\nHEAP LARGE BLOCK DUMP\n");
			fprintf (file, "ADDRESS, SIZE, CATEGORY, HEAP, STATE, SOURCE, LINE\n");

			// For each main block
			currentBlock = _MainBlockList;
			while (currentBlock)
			{
				// Get the first node
				const CNodeBegin *current = getFirstNode (currentBlock);
				while (current)
				{
					// Write the entry
					fprintf (file, "0x%08x, %d, %s, %s, %s, %s, %d\n", (uint)current + sizeof(CNodeBegin),
						getNodeSize (current), current->Category, _Name, 
						isNodeFree (current)?"free":"used", current->File, current->Line);

					// Next node
					current = getNextNode (current);
				}

				// Next block
				currentBlock = currentBlock->Next;
			}
		}

		// File created successfuly
		status = true;
	}

	// Close
	if (file)
		fclose (file);

	debugPopCategoryString ();

	return status;
}
#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

uint CHeapAllocator::getFreeMemory () const
{
	enterCriticalSection ();

	// Sum free memory
	uint memory = 0;

	// For each small block
	CSmallBlockPool *currentSB = (CSmallBlockPool *)_SmallBlockPool;
	while (currentSB)
	{
		// For each node in this small block pool
		uint block;
		for (block=0; block<SmallBlockPoolSize; block++)
		{
			// Get the node
			const CNodeBegin *current = getSmallBlock (currentSB, block);

			// Node allocated ?
			if (isNodeFree (current))
				memory  += getNodeSize (current) + ReleaseHeaderSize;

			// Next node
			current = getNextNode (current);
		}

		// Next block
		currentSB = currentSB->Next;
	}

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Get the first node
		const CNodeBegin *current = getFirstNode (currentBlock);
		while (current)
		{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
			// Check node
			debugCheckNode (current, evalMagicNumber (current));
#endif // SIP_HEAP_ALLOCATION_NDEBUG

			// Node allocated ?
			if (isNodeFree (current))
				memory  += getNodeSize (current) + ReleaseHeaderSize;

			// Next node
			current = getNextNode (current);
		}

		// Next block
		currentBlock = currentBlock->Next;
	}

	leaveCriticalSection ();

	// Return memory used
	return memory;
}

// *********************************************************

uint CHeapAllocator::getTotalMemoryUsed () const
{
	enterCriticalSection ();

	// Sum total memory
	uint memory = 0;

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Get block size
		memory += currentBlock->Size;

		// Sum the arrays
		memory += sizeof (CMainBlock);

		// Next block
		currentBlock = currentBlock->Next;
	}

	leaveCriticalSection ();

	// Return the memory
	return memory;
}

// *********************************************************

uint CHeapAllocator::getSmallBlockMemory () const
{
	enterCriticalSection ();

	// Sum total memory
	uint memory = 0;

	// For each small block
	CSmallBlockPool *currentSB = (CSmallBlockPool *)_SmallBlockPool;
	while (currentSB)
	{
		// Get block size
		memory += sizeof(CSmallBlockPool) + SmallBlockPoolSize * (sizeof(CNodeBegin) + currentSB->Size + 
					SIP_HEAP_NODE_END_SIZE);

		// Next block
		currentSB = currentSB->Next;
	}

	leaveCriticalSection ();

	// Return the memory
	return memory;
}

// *********************************************************

float CHeapAllocator::getFragmentationRatio () const
{
	enterCriticalSection ();

	// Sum free and used node
	float free = 0;
	float used = 0;

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Get the first node
		const CNodeBegin *current = getFirstNode (currentBlock);
		while (current)
		{
			// Node allocated ?
			if (isNodeUsed (current))
				used++;
			else
				free++;

			// Next node
			current = getNextNode (current);
		}

		// Next block
		currentBlock = currentBlock->Next;
	}

	leaveCriticalSection ();

	// Return the memory
	if (used != 0)
		return free / used;
	else
		return 0;
}

// *********************************************************

void	CHeapAllocator::freeAll ()
{
	enterCriticalSection ();

	// Sum free memory
	uint memory = 0;

	// Clear the free tree
	_FreeTreeRoot = &_NullNode.FreeNode;

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Reinit this block
		initEmptyBlock (*currentBlock);

		// Get first block
		CNodeBegin *node = getFirstNode (currentBlock);

		// Insert the free node
		insert (getFreeNode (node));

		SIP_UPDATE_MAGIC_NUMBER (node);

		// Next block
		currentBlock = currentBlock->Next;
	}

	leaveCriticalSection ();
}

// *********************************************************

void	CHeapAllocator::releaseMemory ()
{
	enterCriticalSection ();

	// Clear the free tree
	_FreeTreeRoot = &_NullNode.FreeNode;

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		freeBlock (currentBlock->Ptr);

		// Next block
		CMainBlock *toDelete = currentBlock;
		currentBlock = toDelete->Next;
		::free (toDelete);
	}

	// Erase block node
	_MainBlockList = NULL;

	leaveCriticalSection ();
}

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

class CLeak
{
public:
	
	CLeak (const char *name, CLeak *next)
	{
		Name = new char[strlen (name)+1];
		strcpy (Name, name);
		Count = 0;
		Memory = 0;
		Next = next;
	}

	~CLeak ()
	{
		delete [] Name;
		if (Next)
			delete Next;
	}

	static CLeak *find (CLeak *cat, const char *name)
	{
		while (cat)
		{
			if (strcmp (name, cat->Name) == 0)
				break;
			cat = cat->Next;
		}
		return cat;
	}

	static CLeak *insert (const char *name, CLeak *next)
	{
		return new CLeak (name, next);
	}

	uint			Count;
	uint			Memory;
	char			*Name;
	CLeak			*Next;
};

void CHeapAllocator::debugReportMemoryLeak ()
{
	debugPushCategoryString (SIP_HEAP_MEM_DEBUG_CATEGORY);

	// Sum allocated memory, locations, and leaks
	uint memory = 0;
	uint leaks = 0;
	uint locations = 0;

	// Leak map
	CLeak *leakMap = NULL;

	// Header
	char report[2048];
	sprintf (report, "Report Memory leak for allocator \"%s\"\n", _Name);
	CHeapAllocatorOutputError (report);

	// For each small block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Get the first node
		const CNodeBegin *current = getFirstNode (currentBlock);
		while (current)
		{
			// Check node
			debugCheckNode (current, evalMagicNumber (current));

			// Node allocated ?
			if (isNodeUsed (current) && ( (current->Category == NULL) || (current->Category[0] != '_')) )
			{
				// Make a report
				sprintf (report, "%s(%d)\t: \"%s\"", current->File, current->Line, current->Category);

				// Look for this leak
				CLeak *ite = CLeak::find (leakMap, report);

				// Not found ?
				if (ite == NULL)
				{
					ite = CLeak::insert (report, leakMap);
					leakMap = ite;
					ite->Count = 0;
					ite->Memory = 0;
				}

				// One more leak
				ite->Count++;
				ite->Memory += getNodeSize (current);

				memory += getNodeSize (current);
			}

			// Next node
			current = getNextNode (current);
		}

		// Next block
		currentBlock = currentBlock->Next;
	}

	// For each small block
	CSmallBlockPool *currentSB = (CSmallBlockPool *)_SmallBlockPool;
	while (currentSB)
	{
		// For each node in this small block pool
		uint block;
		for (block=0; block<SmallBlockPoolSize; block++)
		{
			// Get the node
			const CNodeBegin *current = getSmallBlock (currentSB, block);
			// Check node
			debugCheckNode (current, evalMagicNumber (current));

			// Node allocated ?
			if (isNodeUsed (current) && ( (current->Category == NULL) || (current->Category[0] != '_')) )
			{
				// Make a report
				sprintf (report, "%s(%d)\t: \"%s\"",	current->File, current->Line, current->Category);

				// Look for this leak
				CLeak *ite = CLeak::find (leakMap, report);

				// Not found ?
				if (ite == NULL)
				{
					ite = CLeak::insert (report, leakMap);
					leakMap = ite;
					ite->Count = 0;
					ite->Memory = 0;
				}

				// One more leak
				ite->Count++;
				ite->Memory += getNodeSize (current);

				memory += getNodeSize (current);
			}
		}

		// Next block
		currentSB = currentSB->Next;
	}

	// Look for this leak
	CLeak *ite = leakMap;
	while (ite)
	{
		// Make a report
		sprintf (report, "%s,\tLeak count : %d,\tMemory allocated : %d\n", ite->Name, ite->Count, ite->Memory);

		// Report on stderr
		CHeapAllocatorOutputError (report);

		// sum totals
		leaks += ite->Count;
		locations++;

		ite = ite->Next;
	}

	// Make a report
	if (memory)
	{
		sprintf (report, "%d byte(s) found, %d leaks, %d locations\n", memory, leaks, locations);
	}
	else
	{
		sprintf (report, "No memory leak\n");
	}
	CHeapAllocatorOutputError (report);

	// Delete list
	if (leakMap)
		delete leakMap;

	debugPopCategoryString ();
}
#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

bool CHeapAllocator::checkHeap (bool stopOnError) const
{
	bool res = internalCheckHeap (stopOnError, 0);

	return res;
}

// *********************************************************

bool CHeapAllocator::checkHeapBySize (bool stopOnError, uint blockSize) const
{
	bool res = internalCheckHeap (stopOnError, blockSize);

	return res;
}

// *********************************************************

uint8 *CHeapAllocator::allocateBlock (uint size)
{
#undef malloc
	return (uint8*)::malloc (size);
}

// *********************************************************

void CHeapAllocator::freeBlock (uint8 *block)
{
	::free (block);
}

// *********************************************************

bool CHeapAllocator::internalCheckHeap (bool stopOnError, uint32 blockSize) const
{
	enterCriticalSection ();

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	// Flush the log file
	if (_LogFile)
		fflush ((FILE*)_LogFile);
#endif // SIP_HEAP_ALLOCATION_NDEBUG

	// For each small blocks
	if (blockSize == 0 || blockSize <= LastSmallBlock)
	{
		CSmallBlockPool	*pool = (CSmallBlockPool*)_SmallBlockPool;
		while (pool)
		{
			if (blockSize == 0 || pool->Size == blockSize)
			{
				// For each small block
				uint smallBlock;
				CNodeBegin	*previous = NULL;
				for (smallBlock=0; smallBlock<SmallBlockPoolSize; smallBlock++)
				{
					// Get the small block
					CNodeBegin	*node = getSmallBlock (pool, smallBlock);
					CNodeBegin	*next = (smallBlock+1<SmallBlockPoolSize) ? getSmallBlock (pool, smallBlock+1) : NULL;

					// Check node
					checkNodeSB (pool, previous, node, next, stopOnError);

					previous = node;
				}
			}

			// Next pool
			pool = pool->Next;
		}
	}
	
	// For each main block
	if (blockSize == 0 || blockSize > LastSmallBlock)
	{
		CMainBlock *currentBlock = _MainBlockList;
		while (currentBlock)
		{
			// Get the nodes
			const CNodeBegin *previous = NULL;
			const CNodeBegin *current = getFirstNode (currentBlock);
			internalAssert (current);	// Should have at least one block in the main block
			const CNodeBegin *next;
			
			// For each node
			while (current)
			{
				// Get next
				next = getNextNode (current);

				if (blockSize == 0 || getNodeSize (current) == blockSize)
				{
					// Return Error ?
					if (!checkNodeLB (currentBlock, previous, current, next, stopOnError))
					{
						leaveCriticalSection ();
						return false;
					}
				}

				// Next
				previous = current;
				current = next;
			}

			// Next block
			currentBlock = currentBlock->Next;
		}
	}

	// Check free tree
	if (blockSize == 0)
	{
		if (!checkFreeNode (_FreeTreeRoot, stopOnError, true))
		{
			leaveCriticalSection ();
			return false;
		}
	}

	leaveCriticalSection ();

	// Ok, no problem
	return true;
}

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
void CHeapAllocator::debugAlwaysCheckMemory (bool alwaysCheck)
{
	_AlwaysCheck = alwaysCheck;
}
#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG
bool CHeapAllocator::debugIsAlwaysCheckMemory () const
{
	return _AlwaysCheck;
}
#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

void	CHeapAllocator::setName (const char* name)
{
	enterCriticalSection ();

	strncpy (_Name, name, NameLength-1);

	leaveCriticalSection ();
}

// *********************************************************
// Category control
// *********************************************************

void CHeapAllocator::debugPushCategoryString (const char *str)
{
	// Get the category stack pointer
	CCategory *last = (CCategory*)_CategoryStack.getPointer ();

	// New category node
	CCategory *_new = (CCategory *)SipAlloc (*this, sizeof(CCategory), SIP_HEAP_CATEGORY_BLOCK_CATEGORY);
	_new->Name = str;
	_new->Next = last;

	/* Push it, no need to be thread safe here, because we use thread specifc storage */
	_CategoryStack.setPointer (_new);
}

// *********************************************************

void CHeapAllocator::debugPopCategoryString ()
{
	// Get the category stack pointer
	CCategory *last = (CCategory*)_CategoryStack.getPointer ();
	// sipassertex (last, ("(CHeapAllocator::debugPopCategoryString ()) Pop category wihtout Push"));
	if (last)
	{
		CCategory *next = last->Next;

		// Free last node, no need to be thread safe here, because we use thread specifc storage
		free (last);

		/* Push it, no need to be thread safe here, because we use thread specifc storage */
		_CategoryStack.setPointer (next);
	}
}

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

#ifdef SIP_OS_WINDOWS
#pragma optimize( "", off )
#endif // SIP_OS_WINDOWS

void CHeapAllocator::debugCheckNode (const CNodeBegin *node, uint32 crc) const
{
	//printf("CHeapAllocator::checkNode(%p,%08x): checking...\n", (void*)((uint)node + sizeof (CNodeBegin)), crc);
	// Check the bottom CRC of the node 
	if (crc != *(node->EndMagicNumber))
	{
		// ********
		// * STOP *
		// ********
		// * The bottom CRC32 of the current node is wrong. Use checkMemory() to debug the heap.
		// ********
		// * (*node) Check for more informations
		// ********
		//printf("CHeapAllocator::checkNode(%p,%08x): bad crc (node magic number = %08x)!\n", (void*)((uint)node + sizeof (CNodeBegin)), crc, *(node->EndMagicNumber));
		SIP_ALLOC_STOP(node);
	}

	// Check the node is hold by this heap
	if (node->Heap != this)
	{
		// ********
		// * STOP *
		// ********
		// * This node is not hold by this heap. It has been allocated with another heap.
		// ********
		// * (*node) Check for more informations
		// ********
		//printf("CHeapAllocator::checkNode(%p,%08x): node not held by this heap!\n", (void*)((uint)node + sizeof (CNodeBegin)), crc);
		SIP_ALLOC_STOP(node);
	}
}

// *********************************************************

void CHeapAllocator::debugStopNodeAlreadyFree (const CNodeBegin *node)
{
	// ********
	// * STOP *
	// ********
	// * Attempt to delete a pointer already deleted
	// ********
	// * (*corruptedNode):	the already deleted node
	// ********
	SIP_ALLOC_STOP(node);
}

#ifdef SIP_OS_WINDOWS
#pragma optimize( "", on )
#endif // SIP_OS_WINDOWS

#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

uint CHeapAllocator::getAllocatedSystemMemory ()
{
	uint systemMemory = 0;
#ifdef SIP_OS_WINDOWS
	// Get system memory informations
	HANDLE hHeap[100];
	DWORD heapCount = GetProcessHeaps (100, hHeap);

	uint heap;
	for (heap = 0; heap < heapCount; heap++)
	{
		PROCESS_HEAP_ENTRY entry;
		entry.lpData = NULL;
		while (HeapWalk (hHeap[heap], &entry))
		{
			if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			{
				systemMemory += entry.cbData + entry.cbOverhead;
			}
		}
	}
#endif // SIP_OS_WINDOWS
	return systemMemory;
}

// *********************************************************

class CPointerEntry
{
public:
	CPointerEntry (void	*pointer, CPointerEntry *entry)
	{
		Pointer = pointer;
		Next = entry;
	}
	static CPointerEntry *find (CPointerEntry *cat, void *pointer)
	{
		while (cat)
		{
			if (pointer == cat->Pointer)
				break;
			cat = cat->Next;
		}
		return cat;
	}
	void			*Pointer;
	CPointerEntry	*Next;
};

uint CHeapAllocator::getAllocatedSystemMemoryByAllocator ()
{
	uint sipSystemMemory = 0;

	// Build a set of allocated system memory pointers
	CPointerEntry *entries = NULL;

	// For each main block
	CMainBlock *currentBlock = _MainBlockList;
	while (currentBlock)
	{
		// Save pointers
		entries = new CPointerEntry ((void*)currentBlock, entries);
		entries = new CPointerEntry ((void*)currentBlock->Ptr, entries);

		// Next block
		currentBlock = currentBlock->Next;
	}

#ifdef SIP_OS_WINDOWS
	// Get system memory informations
	HANDLE hHeap[100];
	DWORD heapCount = GetProcessHeaps (100, hHeap);

	FILE *file = fopen ("dump.bin", "wb");

	uint heap;
	for (heap = 0; heap < heapCount; heap++)
	{
		PROCESS_HEAP_ENTRY entry;
		entry.lpData = NULL;
		uint block = 0;
		while (HeapWalk (hHeap[heap], &entry))
		{
			if (entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)
			{
				// This pointer is already used ?
				if ( CPointerEntry::find (entries, (void*)((char*)entry.lpData)) || 
					CPointerEntry::find (entries, (void*)((char*)entry.lpData+32) ) )
					sipSystemMemory += entry.cbData + entry.cbOverhead;
				else
				{
					fwrite (entry.lpData, 1, entry.cbData, file);
				}
			}
			block++;
		}
	}
	
	fclose (file);

#endif // SIP_OS_WINDOWS

	return sipSystemMemory;
}

// *********************************************************

uint8 *CHeapAllocator::internalAllocateBlock (uint size)
{
	uint8 *ptr = allocateBlock (size);
	
	return ptr;
}

// *********************************************************

void CHeapAllocator::outOfMemory()
{
	// Out of memory callback
	if (_OutOfMemoryCallback)
		_OutOfMemoryCallback();
	
	// Select outofmemory mode
	if (_OutOfMemoryMode == ThrowException)
		throw 0;
}

// *********************************************************

void CHeapAllocator::setOutOfMemoryHook (void (*outOfMemoryCallback)())
{
	_OutOfMemoryCallback = outOfMemoryCallback;
}

// *********************************************************
// Integrity checks
// *********************************************************

bool CHeapAllocator::checkFreeBlockContent (const uint8 *ptr, uint sizeToCheck, bool stopOnError)
{
	const uint8 pattern = *ptr;
	bool error = pattern != CHeapAllocator::DeletedMemory;
	
	// Continue checking ?
	if (!error)
	{
		const uint32 pattern32 = pattern | (pattern<<8) | (pattern<<16) | (pattern<<24);
		uint sizeToCheck32 = sizeToCheck >> 2;
		sizeToCheck -= sizeToCheck32 << 2;
		while (sizeToCheck32--)
		{
			if (*(uint32*)ptr != pattern32)
			{
				error = true;
				break;
			}
			ptr+=4;
		}
		if (!error)
		{
			while (sizeToCheck--)
			{
				if (*ptr != pattern)
				{
					error = true;
					break;
				}
				ptr++;
			}
		}
	}
	
	if (error)
	{
		// Stop on error ?
		if (stopOnError)
		{
			// ********
			// * STOP *
			// ********
			// * The garbage pattern is corrupted : it means that a freed memory block as been modified.
			// * Maybe this free memory block is still referenced by a pointer.
			// ********
			// * (*current):	current node
			// ********
			SIP_ALLOC_STOP(ptr);
		}
		return false;
	}
	return true;
}

// *********************************************************

bool CHeapAllocator::checkNodeSB (const CSmallBlockPool *mainBlock, const CNodeBegin *previous, const CNodeBegin *current, const CNodeBegin *next, bool stopOnError) const
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	// Get the theorical CRC check
	uint32 crc = evalMagicNumber (current);

	// Compare the magic number
	if (*(current->EndMagicNumber) != crc)
	{
		// Stop on error ?
		if (stopOnError)
		{
			// ********
			// * STOP *
			// ********
			// * The bottom CRC32 of the current node is wrong: Check if a node has overflowed. Node by top and bottom,
			// * the next by the top and the previous by the bottom.
			// * overflow or the next node by the top.
			// ********
			// * (*previous):	previous node
			// * (*current):	current node
			// * (*next):		next node
			// ********
			SIP_ALLOC_STOP(current);
		}

		// CRC is wrong
		return false;
	}

#ifndef SIP_NO_DELETED_MEMORY_CONTENT_CHECK

	// If the block is free, check if it is filled with 0xba, 0xbc, or 0xbd pattern
	if (isNodeFree (current))
	{
		// The CFreeNode is check with the CRC, skip it
		uint sizeToCheck = getNodeSize (current);
		if (sizeToCheck)
		{
			uint8 *ptr = (uint8*)current + sizeof(CNodeBegin);
			if (!checkFreeBlockContent (ptr, sizeToCheck, stopOnError))
				return false;
		}
	}

#endif // SIP_NO_DELETED_MEMORY_CONTENT_CHECK

#endif // SIP_HEAP_ALLOCATION_NDEBUG

	// *** Release node control

	// Check node
	if	(
			( (uint)current < ((uint)mainBlock) + sizeof (CSmallBlockPool)) ||
			( (uint)current + getNodeSize (current) + sizeof(CNodeBegin) + SIP_HEAP_NODE_END_SIZE >
				((uint)mainBlock) + sizeof (CSmallBlockPool) + SmallBlockPoolSize * (sizeof(CNodeBegin)+ mainBlock->Size  + SIP_HEAP_NODE_END_SIZE) )
		)
	{
		// Stop on error ?
		if (stopOnError)
		{
			// ********
			// * STOP *
			// ********
			// * The size value is corrupted: Check if the current node has
			// * overflow by the top or the previous node by the bottom.
			// ********
			// * (*previous):	previous node
			// * (*current):	current node
			// * (*next):		next node
			// ********
			SIP_ALLOC_STOP(current);
		}

		// Node size is corrupted
		return false;
	}

	// Ok
	return true;
}

// *********************************************************

bool CHeapAllocator::checkNodeLB (const CMainBlock *mainBlock, const CNodeBegin *previous, const CNodeBegin *current, const CNodeBegin *next, bool stopOnError) const
{
#ifndef SIP_HEAP_ALLOCATION_NDEBUG
	// Get the theorical CRC check
	uint32 crc = evalMagicNumber (current);

	// Compare the magic number
	if (*(current->EndMagicNumber) != crc)
	{
		// Stop on error ?
		if (stopOnError)
		{
			// ********
			// * STOP *
			// ********
			// * The bottom CRC32 of the current node is wrong: Check if a node has overflowed. Node by top and bottom,
			// * the next by the top and the previous by the bottom.
			// * overflow or the next node by the top.
			// ********
			// * (*previous):	previous node
			// * (*current):	current node
			// * (*next):		next node
			// ********
			SIP_ALLOC_STOP(current);
		}

		// CRC is wrong
		return false;
	}

#ifndef SIP_NO_DELETED_MEMORY_CONTENT_CHECK

	// If the block is free, check if it is filled with 0xba, 0xbc, or 0xbd pattern
	if (isNodeFree (current))
	{
		// The CFreeNode is check with the CRC, skip it
		uint sizeToCheck = getNodeSize (current) - sizeof(CFreeNode);
		if (sizeToCheck)
		{
			uint8 *ptr = (uint8*)current + sizeof(CFreeNode) + sizeof(CNodeBegin);
			if (!checkFreeBlockContent (ptr, sizeToCheck, stopOnError))
				return false;
		}
	}

#endif // SIP_NO_DELETED_MEMORY_CONTENT_CHECK

#endif // SIP_HEAP_ALLOCATION_NDEBUG

	// *** Release node control

	// Check node
	if	(
			( (uint)current < (uint)mainBlock->Ptr ) ||
			( (uint)current + getNodeSize (current) + sizeof(CNodeBegin) + SIP_HEAP_NODE_END_SIZE > 
				(uint)mainBlock->Ptr + mainBlock->Size )
		)
	{
		// Stop on error ?
		if (stopOnError)
		{
			// ********
			// * STOP *
			// ********
			// * The size value is corrupted: Check if the current node has
			// * overflow by the top or the previous node by the bottom.
			// ********
			// * (*previous):	previous node
			// * (*current):	current node
			// * (*next):		next node
			// ********
			SIP_ALLOC_STOP(current);
		}

		// Node size is corrupted
		return false;
	}

	// Check Previous node pointer
	if ( !(current->Previous == NULL || 
		( ((uint)current->Previous <= (uint)current - sizeof (CNodeBegin) - SIP_HEAP_NODE_END_SIZE) && 
		((uint)current->Previous >= (uint)mainBlock->Ptr) )
		) )
	{
		// Stop on error ?
		if (stopOnError)
		{
			// ********
			// * STOP *
			// ********
			// * The previous value is corrupted: Check if the current node has
			// * overflow by the top or the previous node by the bottom.
			// ********
			// * (*previous):	previous node
			// * (*current):	current node
			// * (*next):		next node
			// ********
			SIP_ALLOC_STOP(current);
		}

		// Node flag is corrupted
		return false;
	}

	// Check FreeNode node pointer
	if (isNodeFree (current))
	{
		// Get only this free node
		const CFreeNode *freeNode = getFreeNode (current);
		checkFreeNode (freeNode, stopOnError, false);
	}

	// Ok
	return true;
}

// *********************************************************

bool CHeapAllocator::checkFreeNode (const CFreeNode *current, bool stopOnError, bool recurse) const
{
	// Not NULL ?
	if (current != &_NullNode.FreeNode)
	{
		// Get the node
		const CNodeBegin *node = getNode (current);

		// 1st rule: Node must be free
		if ( !isNodeFree(node) )
		{
			// Stop on error ?
			if (stopOnError)
			{
				// ********
				// * STOP *
				// ********
				// * Node should be free : Free tree is corrupted.
				// ********
				SIP_ALLOC_STOP(current);
			}

			// Node Color is corrupted
			return false;
		}
		
		// 2nd rule: Node must be sorted 
		if	( 
				( current->Left != &_NullNode.FreeNode && getNodeSize (getNode (current->Left)) > getNodeSize (node) ) ||
				( current->Right != &_NullNode.FreeNode && getNodeSize (getNode (current->Right)) < getNodeSize (node) )
			)
		{
			// Stop on error ?
			if (stopOnError)
			{
				// ********
				// * STOP *
				// ********
				// * Node order is corrupted: Free tree is corrupted.
				// ********
				SIP_ALLOC_STOP(current);
			}

			// Node Data is corrupted
			return false;
		}
		
		// 3rd rule: if red, must have two black nodes
		bool leftBlack = (current->Left == &_NullNode.FreeNode) || isNodeBlack(current->Left);
		bool rightBlack = (current->Right == &_NullNode.FreeNode) || isNodeBlack(current->Right);
		if ( !leftBlack && !rightBlack && isNodeRed (getFreeNode (node)) )
		{
			// Stop on error ?
			if (stopOnError)
			{
				// ********
				// * STOP *
				// ********
				// * Color is corrupted: Free tree is corrupted.
				// ********
				SIP_ALLOC_STOP(current);
			}

			// Node Color is corrupted
			return false;
		}

		// If Parent NULL, must be the root
		if ( ( (current->Parent == NULL) && (_FreeTreeRoot != current) ) || ( (current->Parent != NULL) && (_FreeTreeRoot == current) ) )
		{
			// Stop on error ?
			if (stopOnError)
			{
				// ********
				// * STOP *
				// ********
				// * Parent pointer corrupted: Free tree is corrupted.
				// ********
				SIP_ALLOC_STOP(current);
			}

			// Node Parent is corrupted
			return false;
		}

		// Recuse childern
		if (recurse)
		{
			if (!checkFreeNode (current->Left, stopOnError, recurse))
				return false;
			if (!checkFreeNode (current->Right, stopOnError, recurse))
				return false;
		}
	}

	// Ok
	return true;
}

// *********************************************************

#ifndef SIP_HEAP_ALLOCATION_NDEBUG

bool CHeapAllocator::debugStartAllocationLog (const char *filename, uint blockSize)
{
	// Release previous log file
	if (_LogFile)
		debugEndAllocationLog ();
	internalAssert (_LogFile == NULL);

	_LogFile = fopen (filename, "wb");
	_LogFileBlockSize = blockSize;
	if (_LogFile)
		fwrite (&_LogFileBlockSize, 1, sizeof (_LogFileBlockSize), (FILE*)_LogFile);
	return _LogFile != NULL;
}

// *********************************************************

bool CHeapAllocator::debugEndAllocationLog ()
{
	if (_LogFile)
	{
		FILE *tmp = (FILE*)_LogFile;
		_LogFile = NULL;
		return fclose (tmp) == 0;
	}
	else
		return false;
}

// *********************************************************

void CHeapAllocator::debugLog (const void *begin, const char *context)
{
	internalAssert (_LogFile);
	FILE *fp = (FILE*)_LogFile;
	fwrite (&begin, 1, 4, fp);
	fwrite (context, 1, strlen (context)+1, fp);
}

#endif // SIP_HEAP_ALLOCATION_NDEBUG

// *********************************************************

} // SIPMEMORY
