#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p4_clearbg
#include "image_macros.S"

/* P4 CLEARBG, RAM version: trivial.

   This is the bopti algorithm. Azur's is faster when there are enough
   transparent pixels, but very limiting for quasi-opaque images.

   r0:  [temporary]
   r7:  Current x position
   r8:  Alpha value
   r9:  Palette
   r10: Initial x position
   r11: Column counter
   r12: -3 */

.macro GEN_CLEARBG_LOOP HFLIP, OUT_DIR
	/* Cancel the last operation to keep r4 = bytes between rows */
	mov.l	@r8+, r9	/* cmd.palette */
	mov	r2, r7
	shlr	r7
	addc	r7, r4

	mov.w	@r8+, r7	/* cmd.edge_2 */
	nop

	mov.w	@r8, r8		/* cmd.color_1 */
	nop

	mov.l	r11, @-r15
	shll	r8		/* alpha*2 compares against palette offsets */

	mov.l	r12, @-r15
	mov	#-3, r12

 .if \HFLIP
	add	#-2, r5
	mov	r2, r0
	shll	r0
	add	r0, r5
	shll	r0
	add	r0, r6
 .endif

1:	mov	r2, r11
	mov	r10, r7

	/* Load 4 bits from offet r7 (in pixels) within input */
2:	mov	r7, r0
	shlr	r0

	mov.b	@(r0, r3), r0
	nop

	bt.s	3f
	add	#1, r7

/* Aligned */
	shld	r12, r0
	and	#0x1e, r0

	cmp/eq	r0, r8
	bt	4f

	mov.w	@(r0, r9), r0
	bra	4f
	mov.w	r0, @r5

/* Unaligned */
3:	shll	r0
	and	#0x1e, r0

	cmp/eq	r0, r8
	bt	4f

	mov.w	@(r0, r9), r0
	mov.w	r0, @r5

/* End */
4:	dt	r11
	bf.s	2b
	add	#\OUT_DIR, r5

	END

	mov.l	@r15+, r12
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	EPILOGUE
.endm

_gint_image_p4_clearbg:
	tst	#1, r0
	bf	9f

	GEN_CLEARBG_LOOP 0, 2
9:	GEN_CLEARBG_LOOP 1, -2

#endif
