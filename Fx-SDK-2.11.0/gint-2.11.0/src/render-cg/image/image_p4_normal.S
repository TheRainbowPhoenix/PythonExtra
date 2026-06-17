#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p4_normal
#include "image_macros.S"

/* P4 Opaque rendering, VRAM version: by unrolling without edge pixels.

   This is the most unique function in the renderer, Azur included. A P4 image
   cannot reasonably be decoded on a per-pixel basis because extracting half-
   bytes is too slow. But using edge pixels results in extra write surface that
   makes us slower than bopti in gint 2.7.

   This loop is thus the only one to implement 2-unrolling (no pipeline) while
   manually avoiding the writes that a pair of edge pixels usually fix. Subtle
   adjustments to strides are involved, making this function one of the most
   tricky.

   A slight change is made to the command for the purpose of this function;
   cmd.edge_1 (which is r10) is set to indicate whether the [left] side of the
   box is even (r10=0) or odd (r10=1). This allows us to enter the loop at the
   correct position.

   r0:  [temporary]
   r7:  [temporary]
   r8:  Column counter
   r9:  Palette
   r10: box->left & 1
   r11: [temporary] */

.macro GEN_NORMAL_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	mov.l	@r8+, r9	/* cmd.palette */
	add	#-4, r5		/* Better positioning for @(OFF[12], r5) */

	/* The following arithmetic is to decrease r4 if the width is even
	   (r2 & 1) and left is odd (r10 = 1), since that means both the first
	   and last pixel load a full byte but use only half */

	mov	r2, r0
	xor	#1, r0

	mov.w	@r8+, r7	/* cmd.edge_2 (don't care) */
	and	r10, r0

	mov.l	r11, @-r15
	sub	r0, r4

 .if \HFLIP
	mov	r2, r0
	shll	r0

	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop
 .endif

1:	mov	r2, r8
	tst	r10, r10	/* Check whether to do an extra half iter. */

	bt	2f
	nop

	/* Additional half-iteration if box->left = 1 */

	mov.b	@r3+, r0
	shll	r0
	and	#0x1e, r0
	mov.w	@(r0, r9), r0
	dt	r8
	mov.w	r0, @(\OFF1, r5)
	bt.s	3f
	add	#\OUT_DIR, r5

	/* The main loop needs to load pixels in output order. This is not
	   ideal for CPU usage, but we have some margins */

2:	mov.b	@r3+, \TMP1
	mov	#-4, \TMP2

	/* Stall */

	shll	\TMP1
	mov	\TMP1, r0

	shld	\TMP2, r0
	nop

	and	#0x1e, r0
	mov	#0x1e, \TMP2

	/* Stall */

	mov.w	@(r0,r9), r0
	and	\TMP2, \TMP1

	dt	r8
	mov.w	r0, @(\OFF1,r5)

	bt.s	3f
	add	#\OUT_DIR, r5

	mov	\TMP1, r0
	add	#\OUT_DIR, r5

	dt	r8
	mov.w	@(r0,r9), r0

	bf.s	2b
	mov.w	r0, @(\OFF2,r5)

3:	END

	mov.l	@r15+, r11
	mov.l	@r15+, r10
	EPILOGUE
.endm

_gint_image_p4_normal:
	tst	#1, r0
	bf	9f

	GEN_NORMAL_LOOP 0, 2, r7, r11, 4, 2
9:	GEN_NORMAL_LOOP 1, -2, r7, r11, 2, 4

#endif
