/* gint.intc.inth: Standard interrupt handler

   This is a generic interrupt handler that calls back into a C function,
   useful for complex handling or simple drivers that benefit more from
   simplicity than razor-sharp performance. */

.global _intc_generic_handler /* 32 bytes */

.section .gint.blocks, "ax"
.align 4

_intc_generic_handler:
	mova	1f, r0
	mov.l	2f, r1
	jmp	@r1
	mov	r0, r4

1:	.zero	20	/* Indirect call to be made */
2:	.long	0	/* Address of _gint_inth_callback at runtime */
