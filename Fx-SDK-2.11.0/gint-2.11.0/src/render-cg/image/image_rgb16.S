#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_rgb16_loop

/* gint's image renderer: 16-bit RGB entry point

   These formats are the simplest of the bunch. RGB565 can use longword access
   in cases when alignment is favorable and no geometric effect is applied. In
   other cases, pixels are handled individually; geometric effects affect the
   input/output logic while color effects change the computations themselves.

   r0: - (initially: cmd.effect)
   r1: Number of lines remaining to draw
   r2: Number of columns per line
   r3: Input pointer
   r4: Input stride
   r5: Output pointer
   r6: Output stride
   r7: Right edge (only used in Azur) or [temporary]
   r8: - (initially: cmd)
   r9: - (initially: cmd.loop) */

_gint_image_rgb16_loop:
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

	mov.b	@r8+, r9	/* cmd.edge_1 (don't care) */
	nop

	mov.l	@r8+, r9
	extu.b	r1, r1

	mov.l	@r8+, r5	/* cmd.output */
	nop

	mov.l	@r8+, r3	/* cmd.input */
	add	#4, r8		/* cmd.palette (don't care) */

	bf.s	_NO_VFLIP
	shlr	r4

_VFLIP:
	neg	r4, r4
	nop

_NO_VFLIP:
	sub	r2, r4
	nop

	jmp	@r9
	add	r4, r4

#endif
