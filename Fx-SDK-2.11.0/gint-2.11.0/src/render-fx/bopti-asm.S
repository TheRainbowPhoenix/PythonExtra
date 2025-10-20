#include <gint/config.h>
#if GINT_RENDER_MONO

.global _bopti_asm_mono
.global _bopti_asm_mono_alpha

/* REGISTER ALLOCATION:
   r0: layer (left)
   r1: layer (right)
   r2: (temp)
   r3: -
   --
   r4: vram (left)
   r5: vram (right)
   r6: layer pointer; f(x&31)
   r7: mask pointer
   --
   @r15: -(x&31) */

_bopti_asm_mono:
	/* Read data longword and update the layer address pointer */
	mov.l	@r6, r2
	mov.l	@r2+, r0
	mov.l	r2, @r6
	mov	r0, r1

	/* Shift the layer data */
	mov.l	@r15, r6
	shld	r6, r0
	add	#32, r6
	shld	r6, r1

	/* Get the masks and clear the target VRAM area and unwanted image data */
	mov.l	@r7, r2
	and	r2, r0
	not	r2, r2
	and	r2, r4
	mov.l	@(4, r7), r2
	and	r2, r1
	not	r2, r2
	and	r2, r5

	/* Do the drawing by joining the two */
	or	r4, r0
	rts
	or	r5, r1

/* REGISTER ALLOCATION
   r0: layer_1 (left)
   r1: layer_1 (right)
   r2: layer_2 (left)
   r3: layer_2 (right)
   --
   r4: vram (left)
   r5: vram (right)
   r6: layer pointer; f(x&31); mask (left); mask (right)
   r7: mask pointer
   --
   @r15: -(x&31) */

_bopti_asm_mono_alpha:
	/* Read data longwords and update the layer address pointer */
	mov.l	@r6, r2
	mov.l	@r2+, r0
	mov.l	@r2+, r3
	mov.l	r2, @r6
	mov	r0, r1
	mov	r3, r2

	/* Shift all layer data */
	mov.l	@r15, r6
	shld	r6, r0
	shld	r6, r2
	add	#32, r6
	shld	r6, r1
	shld	r6, r3

	/* Apply the masks on the layer data */
	mov.l	@r7, r6
	and	r6, r0
	and	r6, r2
	mov.l	@(4, r7), r6
	and	r6, r1
	and	r6, r3

	/* Blit the clear on the VRAM */
	not	r0, r0
	and	r4, r0
	not	r1, r1
	and	r5, r1

	/* Blit the write on the VRAM */
	or	r2, r0
	rts
	or	r3, r1

#endif /* GINT_RENDER_MONO */
