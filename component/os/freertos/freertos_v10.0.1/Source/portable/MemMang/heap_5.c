/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() that allows the heap to be defined
 * across multiple non-contigous blocks and combines (coalescences) adjacent
 * memory blocks as they are freed.
 *
 * See heap_1.c, heap_2.c, heap_3.c and heap_4.c for alternative
 * implementations, and the memory management pages of http://www.FreeRTOS.org
 * for more information.
 *
 * Usage notes:
 *
 * vPortDefineHeapRegions() ***must*** be called before pvPortMalloc().
 * pvPortMalloc() will be called if any task objects (tasks, queues, event
 * groups, etc.) are created, therefore vPortDefineHeapRegions() ***must*** be
 * called before any other objects are defined.
 *
 * vPortDefineHeapRegions() takes a single parameter.  The parameter is an array
 * of HeapRegion_t structures.  HeapRegion_t is defined in portable.h as
 *
 * typedef struct HeapRegion
 * {
 *	uint8_t *pucStartAddress; << Start address of a block of memory that will be part of the heap.
 *	size_t xSizeInBytes;	  << Size of the block of memory.
 * } HeapRegion_t;
 *
 * The array is terminated using a NULL zero sized region definition, and the
 * memory regions defined in the array ***must*** appear in address order from
 * low address to high address.  So the following is a valid example of how
 * to use the function.
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 * 	{ ( uint8_t * ) 0x80000000UL, 0x10000 }, << Defines a block of 0x10000 bytes starting at address 0x80000000
 * 	{ ( uint8_t * ) 0x90000000UL, 0xa0000 }, << Defines a block of 0xa0000 bytes starting at address of 0x90000000
 * 	{ NULL, 0 }                << Terminates the array.
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << Pass the array into vPortDefineHeapRegions().
 *
 * Note 0x80000000 is the lower address so appears in the array first.
 *
 */
#include <stdlib.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#include "platform_opts.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/* Realtek test code start */
//TODO: remove section when combine BD and BF
#if ((defined CONFIG_PLATFORM_8195A) || (defined CONFIG_PLATFORM_8711B))
#include "section_config.h"
SRAM_BF_DATA_SECTION
#endif
#if !(defined CONFIG_PLATFORM_8710C) //8710C uses the linker symbol to determine the SRAM region and PSRAM region
static unsigned char ucHeap[ configTOTAL_HEAP_SIZE ];
#endif

#if (defined CONFIG_PLATFORM_8195A)
HeapRegion_t xHeapRegions[] =
{
	{ (uint8_t*)0x10002300, 0x3D00 },	// Image1 recycle heap
	{ ucHeap, sizeof(ucHeap) }, 		// Defines a block from ucHeap
#if 0
	{ (uint8_t*)0x301b5000, 300*1024 }, // SDRAM heap
#endif        
	{ NULL, 0 } 							// Terminates the array.
};
#elif (defined CONFIG_PLATFORM_8711B)
#include "rtl8710b_boot.h"
extern BOOT_EXPORT_SYMB_TABLE boot_export_symbol;
HeapRegion_t xHeapRegions[] =
{
	{ 0, 0},	// Image1 reserved ,length will be corrected in pvPortMalloc()
	{ ucHeap, sizeof(ucHeap) }, 	// Defines a block from ucHeap
#if (CONFIG_ENABLE_RDP == 0)	
	{ (uint8_t*)0x1003f000, 0x1000},	// RDP reserved
#endif	
	{ NULL, 0 } 					// Terminates the array.
};
#elif (defined CONFIG_PLATFORM_8710C)
#include <stdio.h>
#include "memory.h"
#include "hal_api.h"
#include "hal_lpcram.h"
#include "hal_sys_ctrl.h"
#if defined(__ICCARM__)
extern uint8_t SramEnd[];
extern uint8_t RAM_BSS$$Limit[];
extern uint32_t RAM_STACK$$Base[];
extern uint8_t EramEnd[];
extern uint8_t ERAM_BSS$$Limit[];

#undef configTOTAL_HEAP_SIZE 
#undef configTOTAL_HEAP_ext_SIZE

#define configTOTAL_HEAP0_SIZE ((uint32_t)RAM_STACK$$Base - ((uint32_t)RAM_BSS$$Limit))
#define HEAP0_START (uint8_t *)RAM_BSS$$Limit

#define configTOTAL_HEAP1_SIZE ((uint32_t)EramEnd - ((uint32_t)ERAM_BSS$$Limit))
#define HEAP1_START (uint8_t *)ERAM_BSS$$Limit

#elif defined(__GNUC__)

#undef configTOTAL_HEAP_SIZE 
#undef configTOTAL_HEAP_ext_SIZE

extern uint8_t __sram_end__[];
extern uint8_t __eram_end__[];
extern uint8_t __bss_end__[];
extern uint8_t __eram_bss_end__[];

#define configTOTAL_HEAP0_SIZE ((uint32_t)__sram_end__ - ((uint32_t)__bss_end__))
#define HEAP0_START (uint8_t*)__bss_end__

#define configTOTAL_HEAP1_SIZE ((uint32_t)__eram_end__ - ((uint32_t)__eram_bss_end__))
#define HEAP1_START (uint8_t*)__eram_bss_end__
#endif

/**/
HeapRegion_t xHeapRegions[] =
{
	{ 0, 0 }, // Defines a block from SRAM heap, but size will be determined by linker to use the rest of free SRAM as heap
	{ 0, 0 }, 	// Defines a block from PSRAM heap
	{ NULL, 0 }                     // Terminates the array.
};
#else
#error NOT SUPPORT CHIP
#endif
/* Realtek test code end */

/*-----------------------------------------------------------*/
#if 1
/*
	Dump xBlock list
*/
void dump_mem_block_list(void)
{
	BlockLink_t *pxBlock = &xStart;
	int count = 0;

	printf("\n===============================>Memory List:\n");
	while(pxBlock->pxNextFreeBlock != NULL)
	{
		printf("[%d]=0x%p, %d\n", count++, pxBlock, pxBlock->xBlockSize);
		pxBlock = pxBlock->pxNextFreeBlock;
	}
}
#endif

/*No OS in secure world*/
#if defined(CONFIG_BUILD_SECURE) && (CONFIG_BUILD_SECURE == 1)
__WEAK void vTaskSuspendAll( void )
{

}

__WEAK BaseType_t xTaskResumeAll( void )
{
	return pdFALSE;
}
#endif

void *pvPortMalloc( size_t xWantedSize )
{
BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
void *pvReturn = NULL;

	/* Realtek test code start */
	if(pxEnd == NULL)
	{
#if (defined CONFIG_PLATFORM_8711B)
		xHeapRegions[ 0 ].xSizeInBytes = (uint32_t)((uint8_t*)0x10005000 - (uint8_t*)boot_export_symbol.boot_ram_end);
		xHeapRegions[ 0 ].pucStartAddress = (uint8_t*)boot_export_symbol.boot_ram_end;
#elif defined(CONFIG_PLATFORM_8710C)
		xHeapRegions[ 0 ].xSizeInBytes = configTOTAL_HEAP0_SIZE;
		xHeapRegions[ 0 ].pucStartAddress = (uint8_t*)HEAP0_START;
		if(hal_get_flash_port_cfg() != FLASH_PORTB){ // not a flash MCM package, so PSRAM on B port is possible
			if(hal_lpcram_is_valid() == HAL_OK){
				xHeapRegions[ 1 ].xSizeInBytes = configTOTAL_HEAP1_SIZE;
				xHeapRegions[ 1 ].pucStartAddress = (uint8_t*)HEAP1_START;
			}
		}
#endif
		vPortDefineHeapRegions( xHeapRegions );
	}
	/* Realtek test code end */

	/* The heap must be initialised before the first call to
	prvPortMalloc(). */
	configASSERT( pxEnd );

	vTaskSuspendAll();
	{
		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if( ( xWantedSize > 0 ) &&
				( ( xWantedSize + xHeapStructSize ) >  xWantedSize ) ) /* Overflow check */
			{
				xWantedSize += xHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. Check for overflow */
					if( ( xWantedSize + ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) ) ) >
						xWantedSize )
					{
						xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
					} 
					else 
					{
						xWantedSize = 0;
					}
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				xWantedSize = 0;
			}

			if( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was not found. */
				if( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
	( void ) xTaskResumeAll();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif

	return pvReturn;
}
/*-----------------------------------------------------------*/

void __vPortFree( void *pv )
{
uint8_t *puc = ( uint8_t * ) pv;
BlockLink_t *pxLink;

	if( pv != NULL )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		configASSERT( pxLink->pxNextFreeBlock == NULL );

		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;

				vTaskSuspendAll();
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					traceFREE( pv, pxLink->xBlockSize );
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}
				( void ) xTaskResumeAll();
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
}

/*-----------------------------------------------------------*/
/* Add by Alfa 2015/02/04 -----------------------------------*/
static void (*ext_free)( void *p ) = NULL;
static uint32_t ext_upper = 0;
static uint32_t ext_lower = 0;
void vPortSetExtFree( void (*free)( void *p ), uint32_t upper, uint32_t lower )
{
	ext_free = free;
	ext_upper = upper;
	ext_lower = lower;
}

void vPortFree( void *pv )
{
	if( ((uint32_t)pv >= ext_lower) && ((uint32_t)pv < ext_upper) ){
		// use external free function
		if( ext_free )	ext_free( pv );
	}else
		__vPortFree( pv );
}

/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

void vPortDefineHeapRegions( const HeapRegion_t * const pxHeapRegions )
{
BlockLink_t *pxFirstFreeBlockInRegion = NULL, *pxPreviousFreeBlock;
size_t xAlignedHeap;
size_t xTotalRegionSize, xTotalHeapSize = 0;
BaseType_t xDefinedRegions = 0;
size_t xAddress;
const HeapRegion_t *pxHeapRegion;

	/* Can only call once! */
	configASSERT( pxEnd == NULL );

	pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );

	while( pxHeapRegion->xSizeInBytes > 0 )
	{
		xTotalRegionSize = pxHeapRegion->xSizeInBytes;

		/* Ensure the heap region starts on a correctly aligned boundary. */
		xAddress = ( size_t ) pxHeapRegion->pucStartAddress;
		if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
		{
			xAddress += ( portBYTE_ALIGNMENT - 1 );
			xAddress &= ~portBYTE_ALIGNMENT_MASK;

			/* Adjust the size for the bytes lost to alignment. */
			xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;
		}

		xAlignedHeap = xAddress;

		/* Set xStart if it has not already been set. */
		if( xDefinedRegions == 0 )
		{
			/* xStart is used to hold a pointer to the first item in the list of
			free blocks.  The void cast is used to prevent compiler warnings. */
			xStart.pxNextFreeBlock = ( BlockLink_t * ) xAlignedHeap;
			xStart.xBlockSize = ( size_t ) 0;
		}
		else
		{
			/* Should only get here if one region has already been added to the
			heap. */
			configASSERT( pxEnd != NULL );

			/* Check blocks are passed in with increasing start addresses. */
			configASSERT( xAddress > ( size_t ) pxEnd );
		}

		/* Remember the location of the end marker in the previous region, if
		any. */
		pxPreviousFreeBlock = pxEnd;

		/* pxEnd is used to mark the end of the list of free blocks and is
		inserted at the end of the region space. */
		xAddress = xAlignedHeap + xTotalRegionSize;
		xAddress -= xHeapStructSize;
		xAddress &= ~portBYTE_ALIGNMENT_MASK;
		pxEnd = ( BlockLink_t * ) xAddress;
		pxEnd->xBlockSize = 0;
		pxEnd->pxNextFreeBlock = NULL;

		/* To start with there is a single free block in this region that is
		sized to take up the entire heap region minus the space taken by the
		free block structure. */
		pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
		pxFirstFreeBlockInRegion->xBlockSize = xAddress - ( size_t ) pxFirstFreeBlockInRegion;
		pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd;

		/* If this is not the first region that makes up the entire heap space
		then link the previous region to this region. */
		if( pxPreviousFreeBlock != NULL )
		{
			pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
		}

		xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

		/* Move onto the next HeapRegion_t structure. */
		xDefinedRegions++;
		pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
	}

	xMinimumEverFreeBytesRemaining = xTotalHeapSize;
	xFreeBytesRemaining = xTotalHeapSize;

	/* Check something was actually defined before it is accessed. */
	configASSERT( xTotalHeapSize );

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}

void* pvPortReAlloc( void *pv,  size_t xWantedSize )
{
	BlockLink_t *pxLink;

	if( ((uint32_t)pv >= ext_lower) && ((uint32_t)pv < ext_upper) ){
		if( ext_free )  ext_free( pv );
		pv = NULL;
	}

	unsigned char *puc = ( unsigned char * ) pv;

	if( pv )
	{
		if( !xWantedSize )
		{
			vPortFree( pv );
			return NULL;
		}

		void *newArea = pvPortMalloc( xWantedSize );
		if( newArea )
		{
			/* The memory being freed will have an xBlockLink structure immediately
				before it. */
			puc -= xHeapStructSize;

			/* This casting is to keep the compiler from issuing warnings. */
			pxLink = ( void * ) puc;

			int oldSize =  (pxLink->xBlockSize & ~xBlockAllocatedBit) - xHeapStructSize;
			int copySize = ( oldSize < xWantedSize ) ? oldSize : xWantedSize;
			memcpy( newArea, pv, copySize );

			vTaskSuspendAll();
			{
				/* Add this block to the list of free blocks. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;
				xFreeBytesRemaining += pxLink->xBlockSize;
				prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
			}
			xTaskResumeAll();
			return newArea;
		}
	}
	else if( xWantedSize )
		return pvPortMalloc( xWantedSize );
	else
		return NULL;

	return NULL;
}

void *pvPortCalloc(size_t xWantedCnt, size_t xWantedSize)
{
	void *p;

	/* allocate 'xWantedCnt' objects of size 'xWantedSize' */
	p = pvPortMalloc(xWantedCnt * xWantedSize);
	if (p) {
		/* zero the memory */
		memset(p, 0, xWantedCnt * xWantedSize);
	}
	return p;
}