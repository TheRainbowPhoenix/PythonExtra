#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p8_clearbg
#include "image_macros.S"

/* P8 CLEARBG, RAM version: by NULL canceling.

   This function is one of the few that can still be bottlenecked by CPU in the
   RAM model. This is because transparent pixels can be skipped over as fast as
   the CPU allows without worrying about the writing speed of the RAM.

   For some reason that I have yet to uncover, branches are way slower than the
   SH4AL-DSP manual suggests, and even slower while inside of DSP loops. This
   completely favors branchless methods, and the one used here is one I call
   "NULL canceling".

   The idea is that a write can be turned into a no-op by either writing the
   value that is already in memory, or by writing somewhere else. The first
   option is pretty slow, especially because it requires a selection operation
   (rn = condition ? rn : rm) which is like the most general branchless trick.

   NULL canceling abuses the fact that NULL is mapped read-only on the platform
   to turn the target pointer in NULL with the following identity:

      target & -(condition) = (condition ? target : NULL)

   The term -(condition) is materialized with an [addc #-1, #0] instruction
   after the test, then the result is applied onto the target pointer with
   [and], completing the trick in only 2 EX instructions. It does take more
   registers, and prevents from using pre-decrement on the target.

   r0:  [temporary]
   r7:  Right edge pointer
   r8:  Alpha value
   r9:  Palette
   r10: Nullable output pointer
   r11: 0 (to neutralize addc during NULL-cancelling)
   r12: Right edge stride
   r13: [temporary]
   r14: [temporary]

   Spilled to stack:
   @(-4,r15): Right edge value */

.macro GEN_CLEARBG_LOOP HFLIP, OUT_DIR, TMP1, TMP2, OFF1, OFF2
	mov.l	@r8+, r9	/* cmd.palette */
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

	mov.w	@r8, r8		/* cmd.color_1 ≤ 255, thus zero-extended */
	mov	#0, r11

 .if \HFLIP
	add	r0, r5
	nop

	shll	r0
	nop

	add	r0, r6
	nop
 .endif

	exts.b	r8, r8
	nop

	START

	mov.b	@r3+, \TMP2
	nop

	mov.w	@r7, r0		/* Save right edge */
	nop

	mov.l	r0, @-r15
	cmp/eq	\TMP2, r8

	mov.b	@r3+, \TMP1
	add	\TMP2, \TMP2

2:	mov	#-1, r10
	addc	r11, r10	/* r10 is now the mask */

	and	r5, r10
	mov	\TMP2, r0

	cmp/eq	\TMP1, r8
	mov.w	@(r0, r9), r0

	mov.w	r0, @(\OFF1, r10)
	add	#\OUT_DIR, r5

	mov.b	@r3+, \TMP2
	nop

	mov	#-1, r10
	addc	r11, r10

	add	\TMP1, \TMP1
	mov	\TMP1, r0

	mov.b	@r3+, \TMP1
	and	r5, r10

	mov.w	@(r0, r9), r0
	cmp/eq	\TMP2, r8

	mov.w	r0, @(\OFF2, r10)
3:	add	\TMP2, \TMP2

	mov.l	@r15+, r0
	nop

	mov.w	r0, @r7		/* Restore right edge */
	add	r12, r7

	END

	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r12
	EPILOGUE
.endm

_gint_image_p8_clearbg:
	tst	#1, r0
	bf	9f

	GEN_CLEARBG_LOOP 0, 4, r13, r14, 4, 2
9:	GEN_CLEARBG_LOOP 1, -4, r13, r14, 2, 4

#endif
