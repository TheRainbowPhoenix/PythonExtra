#include <gint/config.h>
#if GINT_RENDER_MONO

.global _topti_asm_text

/* REGISTER ALLOCATION:
   r0: x or ~x
   r1: light (except lighten/darken: swapped at some point with dark)
   r2: dark  (except lighten/darken: swapped at some point with light)
   r3: (tmp)
   r4: vram (mono or light)
   r5: vram (mono or dark)
   r6: operators
   r7: number of rows (r7>0, otherwise the font is clearly ill-formed)

   Mind that there are pipeline optimization efforts in this file:
   * Doing memory accesses on 4-aligned instructions to avoid contention
     between IF and MA (1 cycle);
   * Avoid using an operand just after it is fetched from memory because of the
     RAW dependency on the destination register (1 cycle);
   * Using delayed slot jumps bf.s rather than bf (1 cycle);
   * Executing the dt early to avoid waiting for its WB slot. Best if >=2
     instructions are between dt and bt.s. (not sure if it's needed). */

.align 4
_topti_asm_white:
	add	#-16, r4
	nop

1:	mov.l	@r6+, r0
	add	#16, r4
	mov.l	@r4, r1
	dt	r7
	mov.l	@r5, r2
	not	r0, r0
	and	r0, r1
	and	r0, r2
	mov.l	r1, @r4
	mov.l	r2, @r5
	bf.s	1b
	add	#16, r5

	rts
	nop

.align 4
_topti_asm_light:
1:	mov.l	@r6+, r0
	dt	r7
	mov.l	@r4, r1
	/* (bubble) */
	or	r0, r1
	mov.l	@r5, r2
	not	r0, r0
	mov.l	r1, @r4
	and	r0, r2
	/* (bubble) */
	mov.l	r2, @r5
	add	#16, r4
	bf.s	1b
	add	#16, r5

	rts
	nop

.align 4
_topti_asm_dark:
1:	mov.l	@r6+, r0
	dt	r7
	mov.l	@r5, r2
	/* (bubble) */
	or	r0, r2
	mov.l	@r4, r1
	not	r0, r0
	mov.l	r2, @r5
	and	r0, r1
	/* (bubble) */
	mov.l	r1, @r4
	add	#16, r4
	bf.s	1b
	add	#16, r5

	rts
	nop

.align 4
_topti_asm_black:
1:	mov.l	@r6+, r0
	dt	r7
	mov.l	@r4, r1
	/* (bubble) */
	or	r0, r1
	mov.l	@r5, r2
	or	r0, r2
	mov.l	r1, @r4
	add	#16, r4
	mov.l	r2, @r5
	bf.s	1b
	add	#16, r5

	rts
	nop

.align 4
_topti_asm_none:
	rts
	nop

.align 4
_topti_asm_invert:
1:	mov.l	@r6+, r0
	dt	r7
	mov.l	@r4, r1
	xor	r0, r1
	mov.l	@r5, r2
	xor	r0, r2
	mov.l	r1, @r4
	add	#16, r4
	mov.l	r2, @r5
	bf.s	1b
	add	#16, r5

	rts
	nop

.align 4
_topti_asm_lighten:
	add	#-16, r4
	add	#-16, r5

1:	mov.l	@r6+, r0
	add	#16, r5
	mov.l	@r5, r2
	add	#16, r4
	mov.l	@r4, r1
	mov	r1, r3
	xor	r0, r3
	not	r0, r0
	or	r0, r1
	and	r2, r1
	mov.l	r1, @r5
	dt	r7
	or	r0, r2
	and	r3, r2
	bf.s	1b
	mov.l	r2, @r4

	rts
	nop

.align 4
_topti_asm_darken:
	add	#-16, r4
	add	#-16, r5

1:	mov.l	@r6+, r0
	add	#16, r5
	mov.l	@r5, r2
	add	#16, r4
	mov.l	@r4, r1
	mov	r1, r3
	xor	r0, r3
	and	r0, r1
	or	r2, r1
	dt	r7
	mov.l	r1, @r5
	and	r0, r2
	or	r3, r2
	/* (bubble) */
	bf.s	1b
	mov.l	r2, @r4

	rts
	nop

/* Export a table with these functions */

.align 4
_topti_asm_text:
	.long	_topti_asm_white
	.long	_topti_asm_light
	.long	_topti_asm_dark
	.long	_topti_asm_black
	.long	_topti_asm_none
	.long	_topti_asm_invert
	.long	_topti_asm_lighten
	.long	_topti_asm_darken

#endif /* GINT_RENDER_MONO */
