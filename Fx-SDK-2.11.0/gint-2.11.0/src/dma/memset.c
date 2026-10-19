#include <gint/dma.h>

/* Allocate a 32-byte buffer in ILRAM */
GALIGNED(32) GILRAM static uint32_t ILbuf[8];

/* dma_memset(): Fast 32-aligned memset */
void *dma_memset(void *dst, uint32_t l, size_t size)
{
	/* Prepare the ILRAM buffer. We need to use ILRAM because the DMA will
	   have to read the operand once per block, as opposed to an assembler
	   routine that would hold it in a register. If we place it in RAM, the
	   DMA will perform twice as many RAM accesses as the handwritten
	   assembler, which would be very slow. By using ILRAM we use two
	   different memory regions, making the DMA faster than the CPU. */
	for(int i = 0; i < 8; i++) ILbuf[i] = l;

	int block_size = DMA_32B;
	int block_count = size >> 5;

	/* Use 4-byte transfers to access SPU memory */
	if((uint32_t)dst >= 0xfe200000 && (uint32_t)dst < 0xfe400000)
	{
		block_size = DMA_4B;
		block_count = size >> 2;
	}

	dma_transfer_sync(1, block_size, block_count, ILbuf, DMA_FIXED, dst,
		DMA_INC);
	return dst;
}
