#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p8_dye
#include "image_macros.S"

/* P8 DYE, RAM version: by NULL canceling.

   This effect basically removes all the complexity out of P8 because we no
   longer need to index the palette. We only keep the tight loop so that the
   CPU can speed in areas with many transparent pixels. This gives some
   acceleration over bopti.

   See P8 CLEARBG for an explanation of NULL canceling.

   r0:  Dye value
   r7:  Right edge pointer
   r8:  Alpha value
   r9:  Right edge value
   r10: Nullable output pointer
   r11: 0 (to neutralize addc during NULL-cancelling)
   r12: Right edge stride
   r13: [temporary]
   r14: [temporary] */

.macro GEN_DYE_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	mov.l	@r8+, r9	/* cmd.palette (don't care) */
	shlr	r2

	mov.w	@r8+, r7	/* cmd.edge_2 */
	mov	r2, r0

	mov.l	r12, @-r15
	shll2	r0

	mov.l	r10, @-r15
	shll	r7

	mov.l	r11, @-r15
	add	r5, r7

	mov	r0, r12
	add	r6, r12

	mov.l	r13, @-r15
	add	#-4, r5

	mov.l	r14, @-r15
	add	#-2, r4		/* Input stride compensation for pipelining */

 .if \HFLIP
	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop
 .endif

	mov.w	@r8+, r11	/* cmd.color_1 ≤ 255, thus zero-extended */
	nop

	mov.w	@r8+, r0	/* cmd.color_2 (dye value) */
	nop

	exts.b	r11, r8
	mov	#0, r11

	START

	mov.b	@r3+, \TMP2
	nop

	mov.w	@r7, r9		/* Save right edge */
	nop

	mov.b	@r3+, \TMP1
	cmp/eq	\TMP2, r8

2:	mov	#-1, r10
	addc	r11, r10	/* r10 is now the mask */

	and	r5, r10
	nop

	mov.b	@r3+, \TMP2
	cmp/eq	\TMP1, r8

	mov.w	r0, @(\OFF1, r10)
	add	#\OUT_DIR, r5

	mov	#-1, r10
	addc	r11, r10

	mov.b	@r3+, \TMP1
	and	r5, r10

	cmp/eq	\TMP2, r8
3:	mov.w	r0, @(\OFF2, r10)

	mov.w	r9, @r7		/* Restore right edge */
	add	r12, r7

	END

	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r12
	EPILOGUE
.endm

_gint_image_p8_dye:
	tst	#1, r0
	bf	9f

	GEN_DYE_LOOP 0, 4, r13, r14, 4, 2
9:	GEN_DYE_LOOP 1, -4, r13, r14, 2, 4

#endif
