/** \file memory_config.h
 * Memory manager configuraition
 *
 * $Id: memory_config.h,v 1.1 2008/06/26 13:41:06 RoMyongGuk Exp $
 */

#ifndef SIP_MEMORY_CONFIG_H
#define SIP_MEMORY_CONFIG_H

// Define this to disable debug features (use to trace buffer overflow and add debug informations in memory headers)
#define SIP_HEAP_ALLOCATION_NDEBUG

// Define this to activate internal checks (use to debug the heap code)
// #define SIP_HEAP_ALLOCATOR_INTERNAL_CHECKS

// Define this to disable small block optimizations
//#define SIP_HEAP_NO_SMALL_BLOCK_OPTIMIZATION

// Stop when allocates 0 bytes. C++ standards : an allocation of zero bytes should return a unique non-null pointer.
//#define SIP_HEAP_STOP_ALLOCATE_0

// Stop when free a NULL pointer. C++ standards : deletion of a null pointer should quietly do nothing.
//#define SIP_HEAP_STOP_NULL_FREE

// Define this to disable the define new used to track memory leak
//#define SIP_NO_DEFINE_NEW

/* Define this to not test the content of a deleted/free block during block checking.
 * This check is useful to see if someone has modified a deleted block.
 * This define as no effect if SIP_HEAP_ALLOCATION_NDEBUG is defined. */
//#define SIP_NO_DELETED_MEMORY_CONTENT_CHECK

#endif // SIP_MEMORY_CONFIG_H

/* End of memory_mutex.h */
