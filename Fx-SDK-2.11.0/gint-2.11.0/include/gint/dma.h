//---
//	gint:dma - Direct Memory Access for efficient data transfer
//---

#ifndef GINT_DMA
#define GINT_DMA

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>
#include <gint/defs/call.h>

/* dma_size_t - Transfer block size */
typedef enum
{
	/* Normal transfers of 1, 2, 4, 8, 16 or 32 bytes at a time */
	DMA_1B = 0,
	DMA_2B = 1,
	DMA_4B = 2,
	DMA_8B = 7,
	DMA_16B = 3,
	DMA_32B = 4,

	/* Transfers of 16 or 32 bytes divided in two operations */
	DMA_16B_DIV = 11,
	DMA_32B_DIV = 12,

} dma_size_t;

/* dma_address_t - Addressing mode for source and destination regions */
typedef enum
{
	/* Fixed address mode: the same address is read/written at each step */
	DMA_FIXED = 0,
	/* Increment: address is incremented by transfer size at each step */
	DMA_INC = 1,
	/* Decrement: only allowed for 1/2/4-byte transfers */
	DMA_DEC = 2,
	/* Fixed division mode: address is fixed even in 16/32-divide mode */
	DMA_FIXEDDIV = 3,

} dma_address_t;

/* dma_transfer_async(): Perform an asynchronous DMA data transfer

   This function starts a DMA data transfer and returns immediately. The
   provided callback will be invoked once the transfer is finish. You can also
   call dma_transfer_wait() to wait until the transfer completes. You can
   create a callback with GINT_CALL() or pass GINT_CALL_NULL.

   @channel   DMA channel (0..5)
   @size      Transfer size
   @blocks    Number of blocks (transferred memory = size * blocks)
   @src       Source pointer, must be aligned with transfer size
   @src_mode  Source address mode
   @dst       Destination address, must be aligned with transfer size
   @dst_mode  Destination address mode
   @callback  Function to invoke when the transfer finishes
   -> Returns true on success. */
bool dma_transfer_async(int channel, dma_size_t size, uint blocks,
	void const *src, dma_address_t src_mode, void *dst,
	dma_address_t dst_mode, gint_call_t callback);

/* dma_transfer_wait(): Wait for an asynchronous transfer to finish
   @channel   DMA channel (0..5) */
void dma_transfer_wait(int channel);

/* dma_transfer_sync(): Perform an synchronous DMA data transfer
   Like dma_transfer_async(), but only returns once the transfer completes. */
bool dma_transfer_sync(int channel, dma_size_t size, uint blocks,
	void const *src, dma_address_t src_mode, void *dst,
	dma_address_t dst_mode);

/* dma_transfer_atomic(): Perform a data transfer without interrupts

   This function performs a transfer much like dma_transfer_sync(), but doesn't
   use interrupts and actively waits for the transfer to finish. Not using
   interrupts is a bad design idea for a majority of programs, and is only ever
   needed to display panic messages inside exception handlers. */
void dma_transfer_atomic(int channel, dma_size_t size, uint blocks,
	void const *src, dma_address_t src_mode,
	void *dst, dma_address_t dst_mode);

/* Deprecated version of dma_transfer_async() that did not have a callback */
__attribute__((deprecated("Use dma_transfer_async() instead")))
void dma_transfer(int channel, dma_size_t size, uint blocks, void const *src,
	dma_address_t src_mode, void *dst, dma_address_t dst_mode);
/* Old name for dma_transfer_atomic() */
#define dma_transfer_noint dma_transfer_atomic

//---
//	DMA-based memory manipulation functions
//---

/* dma_memset(): Fast 32-aligned memset

   This function is your typical memset(), except that the destination and size
   must be 32-aligned, and that the pattern is 4 bytes instead of one. It is
   replicated to 32 bytes then used to fill the destination area. This 4-byte
   fixed size may be lifted in future versions.

   This function cannot be used with virtualized (P0) addresses.

   @dst      Destination address (32-aligned)
   @pattern  4-byte pattern to fill @dst
   @size     Sie of destination area (32-aligned) */
void *dma_memset(void *dst, uint32_t pattern, size_t size);

/* dma_memcpy(): Fast 32-aligned memcpy

   This function works exactly like memcpy(), but it expects 32-aligned source,
   destination, and size, and uses the DMA to efficiently copy.

   This function cannot be used with virtualized (P0) addresses.

   @dst   Destination address (32-aligned)
   @dst   Source addresss (32-aligned)
   @size  Size of region (32-aligned) */
void *dma_memcpy(void * __restrict dst, const void * __restrict src,
   size_t size);

#ifdef __cplusplus
}
#endif

#endif /* GINT_DMA */
