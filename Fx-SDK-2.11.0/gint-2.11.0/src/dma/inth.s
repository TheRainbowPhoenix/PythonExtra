/*
**  gint:dma:inth - DMA address error handler
**  A particular handler that jumps into a panic.
*/

.global _inth_dma_ae /* 32 bytes */

.section .gint.blocks, "ax"
.align 4

/* DMA ADDRESS ERROR INTERRUPT HANDLER - 22 BYTES */

_inth_dma_ae:
	/* Manually RTE into the panic routine, preserving SPC */
	mov.l	2f, r4
	ldc	r4, r4_bank

	/* This instruction changes register bank! */
	stc	ssr, r1
	ldc	r1, sr

	mov.l	1f, r0
	jmp	@r0
	nop

	.zero 10

1:	.long _gint_panic
2:	.long 0x1020
