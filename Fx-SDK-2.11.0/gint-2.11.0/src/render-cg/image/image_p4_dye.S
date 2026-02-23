#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p4_dye
#include "image_macros.S"

/* P4 DYE, RAM version: by NULL canceling.

   Like with P8, this effect removes most of the complexity because there is no
   longer any need to index the palette. However the decoding still takes a lot
   of EX work so the performance is not as good. Since there are transparent
   areas, Azur's CPU-bound version is at least to some extent faster than
   bopti, so that's what we're using.

   See P8 CLEARBG for an explanation of NULL canceling.

   r0:  Dye value
   r7:  Right edge pointer
   r8:  Alpha value
   r9:  0 (to neutralize addc during NULL-cancelling)
   r10: Left edge pointer
   r11: Nullable output pointer
   r12: Edge stride
   r13: [temporary]
   r14: [temporary]

   Spilled to stack:
   @(-8,r15): Right edge value
   @(-4,r15): Left edge value */

.macro GEN_DYE_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	shlr	r2
	nop

	add	r10, r10
	nop

	mov.l	@r8+, r0	/* cmd.palette (don't care) */
	mov	r2, r0

	mov.w	@r8+, r7	/* cmd.edge_2 */
	shll2	r0

	mov.l	r12, @-r15
	shll	r7

	mov.l	r11, @-r15
	add	r5, r7

	mov	r0, r12
	add	r6, r12

	mov.l	r13, @-r15
	add	r5, r10

	mov.l	r14, @-r15
	add	#-4, r5

 .if \HFLIP
	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop
 .endif

	mov.w	@(2,r8), r0	/* cmd.color_2 (dye value) */
	add	#-1, r4		/* Input stride compensation for pipelining */

	mov.w	@r8, r8		/* cmd.color_1 (alpha value) */
	nop

	START

	mov.b	@r3+, \TMP1
	nop

	mov.w	@r7, \TMP2	/* Save right edge */
	nop

	mov.l	\TMP2, @-r15
	mov	#0x0f, \TMP2

	mov.w	@r10, r9	/* Save left edge */
	and	\TMP1, \TMP2

	mov.l	r9, @-r15
	mov	#0, r9

2:	cmp/eq	\TMP2, r8
	mov	#-1, r11

	addc	r9, r11
	mov	#-4, \TMP2

	and	r5, r11
	nop

	shld	\TMP2, \TMP1
	mov	#0x0f, \TMP2

	and	\TMP2, \TMP1
	mov.w	r0, @(\OFF1,r11)

	cmp/eq	\TMP1, r8
	mov	#-1, r11

	addc	r9, r11
	mov.b	@r3+, \TMP1

	and	r5, r11
	nop

	mov	#0x0f, \TMP2
	and	\TMP1, \TMP2

	add	#\OUT_DIR, r5
3:	mov.w	r0, @(\OFF2,r11)

	mov.l	@r15+, \TMP2
	nop

	mov.w	\TMP2, @r10	/* Restore left edge */
	add	r12, r10

	mov.l	@r15+, \TMP2
	nop

	mov.w	\TMP2, @r7	/* Restore right edge */
	add	r12, r7

	END

	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r11
	mov.l	@r15+, r12
	mov.l	@r15+, r10
	EPILOGUE
.endm

_gint_image_p4_dye:
	tst	#1, r0
	bf	9f

	GEN_DYE_LOOP 0, 4, r13, r14, 6, 4
9:	GEN_DYE_LOOP 1, -4, r13, r14, 0, 2

#endif
