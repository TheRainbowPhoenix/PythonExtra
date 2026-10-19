#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p8_swapcolor
#include "image_macros.S"

/* P8 SWAPCOLOR, RAM version: by branchless xor selection.

   The core action of this loop is to render full pixels while replacing any
   occurrence of cmd.color_1 (x) with the value cmd.color_2 (y). Branching is
   too slow as often, so instead we use the fact that both x and y are fixed to
   use the identity

     c ^ ((x ^ y) & -(c == x)) = (c == x ? y : c)

   We materialize -(c == x) by subtracting a register from itself with subc
   after the comparison (which is delightfully elegant), while (x ^ y) is pre-
   computed. This way, the selection is performed in one [subc], one [and] and
   one [xor] for a total of 3 EX slots. This is slower than NULL-cancelling
   (which only takes 2 EX slots) but still better than symmetric alternatives.

   Since we have a palette, we further trick by comparing against the index but
   selecting against the palette entry, ie. we do

      palette[c] ^ ((palette[x] ^ y) & -(c == x)) = (c == x ? y : palette[c])

   which allows the computation to occur in parallel with the palette access
   and does not require the replacement value to be located at a valid index.

   r0:  [temporary]
   r7:  cmd.color_1
   r8:  palette[cmd.color_1] ^ cmd.color_2 (ie. x ^ y)
   r9:  Palette
   r10: Holds (x ^ y) & -(c == x) during selection */

.macro GEN_SWAPCOLOR_LOOP HFLIP, OUT_DIR
	mov.l	@r8+, r9	/* cmd.palette */
	mov.w	@r8+, r0	/* cmd.edge_2 (don't care) */
	mov.w	@r8+, r7	/* cmd.color_1 */
	mov.l	r10, @-r15
	exts.b	r7, r7
	mov	r7, r0
	mov.w	@r8, r8		/* cmd.color_2 */
	add	r0, r0
	mov.w	@(r0, r9), r0
	xor	r0, r8

 .if \HFLIP
	add	#-2, r5
	mov	r2, r0
	shll	r0
	add	r0, r5
	shll	r0
	add	r0, r6
 .endif

	START

2:	mov.b	@r3+, r0
	cmp/eq	r0, r7
	add	r0, r0
	subc	r10, r10
	mov.w	@(r0, r9), r0
	and	r8, r10
	xor	r10, r0
	mov.w	r0, @r5
3:	add	#\OUT_DIR, r5

	END

	mov.l	@r15+, r10
	EPILOGUE
.endm

_gint_image_p8_swapcolor:
	tst	#1, r0
	bf	9f

	GEN_SWAPCOLOR_LOOP 0, 2
9:	GEN_SWAPCOLOR_LOOP 1, -2

#endif
