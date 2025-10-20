#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p4_swapcolor
#include "image_macros.S"

/* P4 SWAPCOLOR, RAM version: by branchless xor selection.

   I'm not sure whether this is the most optimized version for RAM. But it's
   about 7-8% slower than bopti, and the effort of writing yet another
   variation of P4's arduous loops doesn't seem worth it for a rare dynamic
   effect. This is Azur's version.

   See P8 SWAPCOLOR for an explanation of branchless xor selection.

   r0:  [temporary]
   r7:  Right edge pointer
   r8:  palette[cmd.color_1] ^ cmd.color_2 (ie. x ^ y)
   r9:  Palette
   r10: Left edge pointer
   r11: Holds (x ^ y) & -(c == x) during selection
   r12: cmd.color_1
   r13: [temporary]
   r14: [temporary] (in outer loop: edge stride)

   Spilled to stack:
   @(-12,r15): Right edge value
   @(-8,r15): Left edge value
   @(-4,r15): Edge stride */

.macro GEN_SWAPCOLOR_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
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

	mov.l	r13, @-r15
	add	r5, r7

	mov.w	@r8+, r13	/* cmd.color_1 */
	add	r5, r10

	mov.l	r11, @-r15
	add	#-4, r5

	mov	r13, r12
	shll	r13

	mov.l	r14, @-r15
	add	r9, r13

	mov.w	@r8, r8		/* cmd.color_2 */
	add	#-1, r4		/* Input stride compensation for pipelining */

	mov.w	@r13, r13
	mov	r0, r14

	add	r6, r14
	nop

	xor	r13, r8
	nop

 .if \HFLIP
	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop
 .endif

	shll	r12		/* Compare color_1 * 2 with shifted values */
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

	mov.l	r14, @-r15
	nop

2:	mov	\TMP1, r0
	and	#0x1e, r0

	cmp/eq	r0, r12
	mov	#-4, \TMP2

	subc	r11, r11
	nop

	mov.w	@(r0,r9), r0
	and	r8, r11

	shld	\TMP2, \TMP1
	mov	#0x1e, \TMP2

	xor	r11, r0
	mov.w	r0, @(\OFF1,r5)

	and	\TMP2, \TMP1
	nop

	cmp/eq	\TMP1, r12
	nop

	subc	r11, r11
	mov	\TMP1, r0

	add	#\OUT_DIR, r5
	mov.b	@r3+, \TMP1

	and	r8, r11
	mov.w	@(r0,r9), r0

	shll	\TMP1
	nop

	xor	r11, r0
3:	mov.w	r0, @(\OFF2,r5)


	mov.l	@r15+, r14
	nop

	mov.l	@r15+, r0
	nop

	mov.w	r0, @r10	/* Restore left edge */
	add	r14, r10

	mov.l	@r15+, r0
	nop

	mov.w	r0, @r7		/* Restore right edge */
	add	r14, r7

	END

	mov.l	@r15+, r14
	mov.l	@r15+, r11
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r10
	EPILOGUE
.endm

_gint_image_p4_swapcolor:
	tst	#1, r0
	bf	9f

	GEN_SWAPCOLOR_LOOP 0, 4, r13, r14, 6, 0
9:	GEN_SWAPCOLOR_LOOP 1, -4, r13, r14, 0, 6

#endif
