#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_rgb16_swapcolor
#include "image_macros.S"

/* RGB16 SWAPCOLOR, RAM version: trivial.

   This function is once again bottlenecked by RAM. Generating longwords would
   be tight and require significant adjustments, so we stick to words, and the
   trivial bopti-style version already maxes out the output rate. */

.macro GEN_SWAPCOLOR_LOOP HFLIP, OUT_DIR
	mov.w	@r8+, r0	/* cmd.edge_2 (don't care) */
	mov.w	@r8+, r9	/* cmd.color_1 */
	mov.w	@r8+, r7	/* cmd.color_2 */

 .if \HFLIP
	add	#-2, r5
	mov	r2, r0
	shll	r0
	add	r0, r5
	shll	r0
	add	r0, r6
 .endif

1:	mov	r2, r8

2:	mov.w	@r3+, r0
	cmp/eq	r0, r9
	bf	4f
	mov	r7, r0
4:	mov.w	r0, @r5

3:	dt	r8
	bf.s	2b
	add	#\OUT_DIR, r5

	END
	EPILOGUE
.endm

_gint_image_rgb16_swapcolor:
	tst	#1, r0
	bf	9f

	GEN_SWAPCOLOR_LOOP 0, 2
9:	GEN_SWAPCOLOR_LOOP 1, -2

#endif
