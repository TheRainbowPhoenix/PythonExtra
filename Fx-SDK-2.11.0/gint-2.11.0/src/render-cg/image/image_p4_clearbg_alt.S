#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p4_clearbg_alt
#include "image_macros.S"

/* P4 CLEARBG, alternative version: by NULL canceling.

   This function is similar to P8 CLEARBG. Transparent pixels are not limited
   by RAM writing speed, so a tight CPU loop is used. See P8 CLEARBG for an
   explanation of NULL canceling.

   r0:  [temporary]
   r7:  Right edge pointer
   r8:  Alpha value
   r9:  Palette
   r10: Left edge pointer
   r11: Nullable output pointer
   r12: 0 (in outer loop: edge stride)
   r13: [temporary]
   r14: [temporary]

   Spilled to stack:
   @(-12,r15): Right edge value
   @(-8,r15): Left edge value
   @(-4,r15): Edge stride */

.macro GEN_CLEARBG_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	shlr	r2
	nop

	add	r10, r10
	nop

	mov.l	@r8+, r9	/* cmd.palette */
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

	mov.w	@r8, r8		/* cmd.color_1 */
	add	#-1, r4		/* Input stride compensation for pipelining */

 .if \HFLIP
	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop
 .endif

	shll	r8		/* alpha*2 compares against palette offsets */
	nop

	START

	mov.b	@r3+, \TMP1
	nop

	mov.w	@r7, r0		/* Save right edge */
	nop

	mov.l	r0, @-r15
	shll	\TMP1

	mov.w	@r10, r0	/* Save left edge */
	nop

	mov.l	r0, @-r15
	nop

	mov.l	r12, @-r15
	mov	#0, r12

2:	mov	\TMP1, r0
	and	#0x1e, r0

	cmp/eq	r0, r8
	mov	#-1, r11

	addc	r12, r11
	mov	#-4, \TMP2

	and	r5, r11
	mov.w	@(r0,r9), r0

	shld	\TMP2, \TMP1
	mov	#0x1e, \TMP2

	and	\TMP2, \TMP1
	mov.w	r0, @(\OFF1,r11)

	cmp/eq	\TMP1, r8
	mov	#-1, r11

	addc	r12, r11
	mov	\TMP1, r0

	and	r5, r11
	mov.b	@r3+, \TMP1

	add	#\OUT_DIR, r5
	mov.w	@(r0,r9), r0

	mov.w	r0, @(\OFF2,r11)
3:	shll	\TMP1

	mov.l	@r15+, r12
	nop

	mov.l	@r15+, r0
	nop

	mov.w	r0, @r10	/* Restore left edge */
	add	r12, r10

	mov.l	@r15+, r0
	nop

	mov.w	r0, @r7		/* Restore right edge */
	add	r12, r7

	END

	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r11
	mov.l	@r15+, r12
	mov.l	@r15+, r10
	EPILOGUE
.endm

_gint_image_p4_clearbg_alt:
	tst	#1, r0
	bf	9f

	GEN_CLEARBG_LOOP 0, 4, r13, r14, 6, 4
9:	GEN_CLEARBG_LOOP 1, -4, r13, r14, 0, 2

#endif
