#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p4_loop

/* gint's image renderer: 4-bit indexed entry point

   P4 compacts pixel data further than P8 by restricting values to a 16-color
   palette and packing 2 pixels in each byte. This severely restricts our
   ability to use sub-images because odd positions land within bytes.

   Fortunately, we can solve this by using more edge pixels. The simplest way
   to write a P4 loop is to process 2 pixels from a 2-aligned source image
   position in a single iteration. Other structures don't even come close in
   terms of CPU performance (which, as a reminder, is the main bottleneck in
   Azur but not in gint): selecting nibbles individually is too long, while not
   unrolling is still clearly inefficient. So it becomes very important to
   forcibly align the sub-image on byte-aligned input boundaries and stick to
   that grid.

   Obviously, this approach causes up to one extra pixel to be overwritten on
   each side of every line. We solve this problem by adding *another* edge
   pixel on the left side. In the renderer this is called the left edge or
   edge_1, while the standard one is called right edge or edge_2.

   r0: - (initially: cmd.effect)
   r1:  Number of lines remaining to draw
   r2:  Number of columns per line
   r3:  Input pointer
   r4:  Input stride
   r5:  Output pointer
   r6:  Output stride
   r7:  Right edge pointer
   r8:  - (initially: cmd)
   r9:  - (initially: cmd.loop)
   r10: Left edge pointer */

_gint_image_p4_loop:
	/* r4: int output_width (pixels)
	   r5: struct gint_image_cmd *cmd */

	mov.b	@(1,r5), r0	/* cmd.effect */
	add	#2, r5

	mov.w	@r5+, r2	/* cmd.columns */
	mov	r4, r6

	mov.l	r8, @-r15
	mov	r5, r8

	/* For here on the command is r8 */

	mov.l	r9, @-r15
	sub	r2, r6

	mov.w	@r8+, r4	/* cmd.input_stride */
	add	r6, r6

	mov.b	@r8+, r1	/* cmd.lines */
	nop

	mov.l	r10, @-r15
	extu.b	r1, r1

	mov.b	@r8+, r10	/* cmd.edge_1 */
	nop

	mov.l	@r8+, r9
	shlr	r0		/* T bit is now VFLIP */

	mov.l	@r8+, r5	/* cmd.output */
	nop

	bf.s	_NO_VFLIP
	mov.l	@r8+, r3	/* cmd.input */

_VFLIP:
	neg	r4, r4
	nop

_NO_VFLIP:
	mov	r2, r7
	shlr	r7

	jmp	@r9
	subc	r7, r4

#endif
