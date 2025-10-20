#include <gint/dma.h>

/* dma_memcpy(): Fast 32-aligned memcpy */
void *dma_memcpy(void * __restrict dst, const void * __restrict src,
	size_t size)
{
	int block_size = DMA_32B;
	int block_count = size >> 5;

	/* Use 4-byte transfers to access SPU memory */
	if(((uint32_t)src >= 0xfe200000 && (uint32_t)src < 0xfe400000) ||
	   ((uint32_t)dst >= 0xfe200000 && (uint32_t)dst < 0xfe400000))
	{
		block_size = DMA_4B;
		block_count = size >> 2;
	}

	dma_transfer_sync(1, block_size, block_count, src, DMA_INC, dst,
		DMA_INC);
	return dst;
}
