#include <gint/config.h>
#if GINT_RENDER_MONO

.global _bopti_gasm_mono
.global _bopti_gasm_mono_alpha
.global _bopti_gasm_gray
.global _bopti_gasm_gray_alpha

/* REGISTER ALLOCATION
   r0: layer_1 (left)
   r1: layer_1 (right)
   r2: layer pointer and temp
   r3: mask pointer and temp
   --
   r4: vram light (left)
   r5: vram light (right)
   r6: vram dark (left)
   r7: vram dark (right)
   --
   @r15:      layer pointer
   @(4,r15):  mask pointer
   @(8,r15):  -(x&31)
   @(12,r15): destination */

_bopti_gasm_mono:
	/* Read the data longword and update the layer address */
	mov.l	@r15, r2
	mov.l	@r2, r3
	mov.l	@r3+, r0
	mov.l	r3, @r2
	mov	r0, r1

	/* Shift it */
	mov.l	@(8, r15), r2
	shld	r2, r0
	add	#32, r2
	shld	r2, r1

	/* Clear target VRAM and unwanted image data */
	mov.l	@(4, r15), r3
	mov.l	@r3, r2
	and	r2, r0
	not	r2, r2
	and	r2, r4
	and	r2, r6
	mov.l	@(4, r3), r2
	and	r2, r1
	not	r2, r2
	and	r2, r5
	and	r2, r7

	/* Join everything and return */
	or	r0, r4
	or	r1, r5
	or	r0, r6
	or	r1, r7
	mov.l	@(12, r15), r2
	mov.l	r4, @r2
	mov.l	r5, @(4, r2)
	mov.l	r6, @(8, r2)
	rts
	mov.l	r7, @(12, r2)

/* REGISTER ALLOCATION
   r0: layer_1 (left)
   r1: layer_1 (right)
   r2: layer_2 (left)
   r3: layer_2 (right)
   --
   r4: vram light (left)
   r5: vram light (right)
   r6: vram dark (left)
   r7: vram dark (right)
   --
   r8: layer pointer and temp
   r9: mask pointer and temp
   --
   @(8,r15):  layer pointer
   @(12,r15): mask pointer
   @(16,r15): -(x&31)
   @(20,r15): destination */

_bopti_gasm_mono_alpha:
	mov.l	r8, @-r15
	mov.l	r9, @-r15

	/* Read data longwords */
	mov.l	@(8, r15), r8
	mov.l	@r8, r9
	mov.l	@r9+, r0
	mov.l	@r9+, r2
	mov.l	r9, @r8
	mov	r0, r1
	mov	r2, r3

	/* Shift all layer data */
	mov.l	@(16, r15), r8
	shld	r8, r0
	shld	r8, r2
	add	#32, r8
	shld	r8, r1
	shld	r8, r3

	/* Apply masks on image data */
	mov.l	@(12, r15), r9
	mov.l	@r9, r8
	and	r8, r0
	and	r8, r2
	mov.l	@(4, r9), r8
	and	r8, r1
	and	r8, r3

	/* Render and leave */
	not	r0, r0
	not	r1, r1
	and	r0, r4
	and	r1, r5
	and	r0, r6
	and	r1, r7
	or	r2, r4
	or	r3, r5
	or	r2, r6
	or	r3, r7
	mov.l	@(20, r15), r8
	mov.l	r4, @r8
	mov.l	r5, @(4, r8)
	mov.l	r6, @(8, r8)
	mov.l	r7, @(12, r8)

	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

/* REGISTER ALLOCATION
   r0: layer_1 (left)
   r1: layer_1 (right)
   r2: layer_2 (left)
   r3: layer_2 (right)
   --
   r4: vram light (left)
   r5: vram light (right)
   r6: vram dark (left)
   r7: vram dark (right)
   --
   r8: layer pointer (also +- x&31)
   r9: mask pointer (also layer)
   --
   @(8,r15):  layer pointer
   @(12,r15): mask pointer
   @(16,r15): -(x&31)
   @(20,r15): destination */

_bopti_gasm_gray:
	mov.l	r8, @-r15
	mov.l	r9, @-r15

	/* Read data longwords and update the layer address pointer */
	mov.l	@(8, r15), r8
	mov.l	@r8, r9
	mov.l	@r9+, r0
	mov.l	@r9+, r2
	mov.l	r9, @r8
	mov	r0, r1
	mov	r2, r3

	/* Shift all layer data */
	mov.l	@(16, r15), r8
	shld	r8, r0
	shld	r8, r2
	add	#32, r8
	shld	r8, r1
	shld	r8, r3

	/* On the left side, clear the VRAM which is about to be rewritten using
	   the left mask, and also clear unwanted image data */
	mov.l	@(12, r15), r9
	mov.l	@r9, r8
	and	r8, r0
	and	r8, r2
	not	r8, r8
	and	r8, r4
	and	r8, r6

	/* Same on the right side */
	mov.l	@(4, r9), r8
	and	r8, r1
	and	r8, r3
	not	r8, r8
	and	r8, r5
	and	r8, r7

	/* Render these together and store the result */
	or	r0, r4
	or	r1, r5
	or	r2, r6
	or	r3, r7
	mov.l	@(20, r15), r8
	mov.l	r4, @r8
	mov.l	r5, @(4, r8)
	mov.l	r6, @(8, r8)
	mov.l	r7, @(12, r8)

	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8


/* REGISTER ALLOCATION
   r0: layer_1 (left)
   r1: layer_1 (right)
   r2: layer_2 (left)
   r3: layer_2 (right)
   --
   r4: vram light (left)
   r5: vram light (right)
   r6: vram dark (left)
   r7: vram dark (right)
   --
   r8:  layer pointer (also +- x&31)
   r9:  mask pointer (also layer)
   r10: layer_3 (left)
   r11: layer_3 (right)
   --
   @(16,r15): layer pointer
   @(20,r15): mask pointer
   @(24,r15): -(x&31)
   @(28,r15): destination */

.align 4
_bopti_gasm_gray_alpha:
	mov.l	r8, @-r15
	mov.l	r9, @-r15
	mov.l	r10, @-r15
	mov.l	r11, @-r15

	/* Load layer data */
	mov.l	@(16, r15), r8
	mov.l	@r8, r9
	mov.l	@r9+, r0
	mov.l	@r9+, r2
	mov	r0, r1
	mov.l	@r9+, r10
	mov	r2, r3
	mov.l	r9, @r8
	mov	r10, r11

	/* Shift layer data */
	mov.l	@(24, r15), r8
	shld	r8, r0
	shld	r8, r2
	shld	r8, r10
	add	#32, r8
	shld	r8, r1
	shld	r8, r3
	shld	r8, r11

	/* Clear unwanted layer bits */
	mov.l	@(20, r15), r9
	mov.l	@r9, r8
	and	r8, r0
	and	r8, r2
	and	r8, r10
	mov.l	@(4, r9), r8
	and	r8, r1
	and	r8, r3
	and	r8, r11

	/* Blit everything */
	not	r0, r0
	and	r0, r4
	and	r0, r6
	not 	r1, r1
	and	r1, r5
	and	r1, r7
	or	r2, r4
	or	r3, r5
	or	r10, r6
	or	r11, r7

	/* Store results and leave */
	mov.l	@(28, r15), r8
	mov.l	r4, @r8
	mov.l	r5, @(4, r8)
	mov.l	r6, @(8, r8)
	mov.l	r7, @(12, r8)
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8

#endif /* GINT_RENDER_MONO */
