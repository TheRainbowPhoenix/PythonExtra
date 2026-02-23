#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p8_loop

/* gint's image renderer: 8-bit indexed entry point

   P8 compacts images by indexing each pixel on a 256-color palette, thus
   halving the amount of data per pixel. This comes at the cost of an
   additional lookup during rendering. For these format, there is no way to
   bundle pixels together, and the more advanced loops handle pixels
   individually with a 2-unrolled 2-stage-pipeline structure to accelerate the
   CPU processing when that is the bottleneck (which often means where there
   are transparent pixels to skip).

   For readers not familiar with loop optimization literature, the main idea is
   that a simple loop which loads a pixel, processes it, and writes it, is too
   inefficient because of RAW delays. To use the full speed of the CPU, one
   needs to do more work in parallel and spread out actions on a single pixel,
   which we do here with two loop transforms:

   * _Pipelining_ the loop consists in handling a single pixel over several
     iterations by doing a little bit of work in each iteration. The data for
     the pixel would move from register to register at each iteration, with the
     loop code doing one stage's worth of computation on each register. This
     gives us more pixels to work on simultaneously, and more independent work
     means less RAW limitations. Loops in this renderer have 2 stages at most.

  * _Unrolling_ iterations of the loop consists in loading two (or more) pixels
     at the start of each iteration so that we can work on one while waiting
     for stalls and dependencies on the other. Unlike pipelining, pixels are
     still confined within iterations. Non-trivial loops in this renderer
     process 2 pixels per iteration.

   Unrolling has one major flaw: handling pairs of pixels only works if the
   total amount of pixels to draw is even. The usual way to handle this for n
   pixels is to do ⌊n/2⌋ iterations and handle the last pixel individually if n
   is odd. This is extremely annoying, since every row must check the value of
   n, and an extra copy of the loop code for a single pixel must be maintained
   on the side, which takes more space and more effort.

   However, we have a specialized solution here with *edge pixels*. The idea of
   edge pixels is to round the number of pixels *up* and perform ⌊(n+1)/2⌋ runs
   of the inner loop. If n is odd, this will overwrite a single pixel at the
   end of the line. We can cancel this error after-the-fact by saving the value
   of the (n+1)-th pixel of the line before the loop, and restoring it
   afterwards. Note that if n is even then the save/restore is a no-op.

   This takes some caution however, as the temporary overwrite could be seen by
   an interrupt. Some measures are put in place to reserve a couple of bytes on
   each side of gint's VRAM and Azur's target fragment to avoid any problems.

   r0: - (initially: cmd.effect)
   r1: Number of lines remaining to draw
   r2: Number of columns per line
   r3: Input pointer
   r4: Input stride
   r5: Output pointer
   r6: Output stride
   r7: Right edge or [temporary]
   r8: - (initially: cmd)
   r9: - (initially: cmd.loop) */

_gint_image_p8_loop:
	/* r4: int output_width (pixels)
	   r5: struct gint_image_cmd *cmd */

	mov.b	@(1,r5), r0	/* cmd.effect */
	add	#2, r5

	mov.l	r8, @-r15
	mov	r4, r6

	mov.w	@r5+, r2	/* cmd.columns */
	mov	r5, r8

	/* For here on the command is r8 */

	mov.l	r9, @-r15
	shlr	r0		/* T bit is now VFLIP */

	mov.w	@r8+, r4	/* cmd.input_stride */
	sub	r2, r6

	mov.b	@r8+, r1	/* cmd.lines */
	add	r6, r6

	mov.b	@r8+, r9	/* cmd.edge_1 - don't care */
	nop

	mov.l	@r8+, r9
	extu.b	r1, r1

	mov.l	@r8+, r5	/* cmd.output */
	nop

	bf.s	_NO_VFLIP
	mov.l	@r8+, r3	/* cmd.input */

_VFLIP:
	neg	r4, r4
	nop

_NO_VFLIP:
	jmp	@r9
	sub	r2, r4

#endif
