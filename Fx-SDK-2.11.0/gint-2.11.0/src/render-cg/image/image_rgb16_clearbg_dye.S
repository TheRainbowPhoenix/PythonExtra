#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_rgb16_clearbg
.global _gint_image_rgb16_dye
#include "image_macros.S"

/* RGB16 CLEARBG and DYE, RAM version: trivial.

   This function handles both CLEARBG and DYE; in RGB16 they are the same,
   except that DYE writes not the pixel value (TMP) but a fixed color (SRC). As
   if often the case, the RAM speed is limiting, so there is no point in
   improving speed of the code on the CPU side. */

.macro GEN_CLEARBG_DYE_LOOP HFLIP, OUT_DIR, TMP, SRC
	mov.w	@r8+, r0	/* cmd.edge_2 (don't care) */
	mov.w	@r8+, r9	/* cmd.color_1 (alpha color) */
	mov.w	@r8+, r0	/* cmd.color_2 (dye color) */

 .if \HFLIP
	add	#-2, r5
	mov	r2, r8
	shll	r8
	add	r8, r5
	shll	r8
	add	r8, r6
 .endif

1:	mov	r2, r8

2:	mov.w	@r3+, \TMP
	cmp/eq	\TMP, r9
	bt	3f
	mov.w	\SRC, @r5

3:	dt	r8
	bf.s	2b
	add	#(\OUT_DIR/2), r5

	END
	EPILOGUE
.endm

_gint_image_rgb16_clearbg:
	tst	#1, r0
	bf	9f

	GEN_CLEARBG_DYE_LOOP 0, 4, r0, r0
9:	GEN_CLEARBG_DYE_LOOP 1, -4, r0, r0

_gint_image_rgb16_dye:
	tst	#1, r0
	bf	9f

	GEN_CLEARBG_DYE_LOOP 0, 4, r7, r0
9:	GEN_CLEARBG_DYE_LOOP 1, -4, r7, r0

#endif
