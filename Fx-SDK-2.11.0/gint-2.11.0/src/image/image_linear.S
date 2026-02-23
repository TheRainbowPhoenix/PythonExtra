#include <gint/config.h>
#if GINT_RENDER_RGB

.global _image_linear_rgb16
.global _image_linear_p8

/* The loop nest for the rotation + scaling code, manually optimized.
   r0, r1: (temporary), u
   r2, r3: dx_u, dx_v
   r4:     input_pixels
   r5:     output_pixels
   r6, r7: drow_u, drow_v
   r8:     line counter
   r9:     dst_w
   r10:    src_w << 16 (for bound checks)
   r11:    src_h << 16 (for bound checks)
   r12:    v
   r13:    (temporary)
   r14:    src_stride (for index access to input_pixels)
   @-4:    dst_stride

   This loop maintains the value of (u,v) at each pixel by adding (dx_u, dx_v)
   every pixel and (drow_u, drow_v) every row. For each position, it then
   checks whether 0 <= u < src_w and 0 <= v < src_height as fixed-point; if
   yes, input[(int)v * src_w + (int)u] is extracted; otherwise, the pixel is
   skipped. */
.macro GEN_LINEAR_LOOP MEM, DEPTH
	mov.l	r8, @-r15
	mov.l	r9, @-r15
	mov.l	r10, @-r15
	mov.l	r11, @-r15
	mov.l	r12, @-r15
	mov.l	r13, @-r15
	mov.l	r14, @-r15
	mov.l	@r6+, r10	/* map.src_w */
	mov.l	@r6+, r11	/* map.src_h */
	mov.l	@r6+, r9	/* map.dst_w */
	mov.l	@r6+, r8	/* map.dst_h */
	mov.l	@r6+, r14	/* map.src_stride */
	mov.l	@r6+, r0	/* map.dst_stride */
	mov.l	@r6+, r1	/* map.u */
	mov.l	@r6+, r12	/* map.v */
	mov.l	@r6+, r2	/* map.dx_u */
	mov.l	@r6+, r3	/* map.dx_v */

	mov.l	@(4, r6), r7	/* map.dy_v (replaced with drow_v) */
	shll16	r10

	mov.l	@r6, r6		/* map.dy_u (replaced with drow_u) */
	shll16	r11

	/* Compute the output stride as map.dst_stride - (DEPTH * map.dst_w) */

	ldrs	1f
	sub	r9, r0

	ldre	2f
 .if \DEPTH == 2
	sub	r9, r0
 .else
	nop
 .endif

 	mov.l	r0, @-r15
 	nop

4:	ldrc	r9
	nop

1:	cmp/hs	r10, r1
	nop

	bt	3f
	cmp/hs	r11, r12

	bt	3f
	swap.w	r12, r13

	mov	r1, r0
	mulu.w	r13, r14

	shlr16	r0
	sts	macl, r13

 .if \DEPTH == 2
	shll	r0
	nop
 .endif

	add	r13, r0
	\MEM	@(r0, r4), r13

	\MEM	r13, @r5
     3:	add	#\DEPTH, r5

	add	r2, r1
	nop

	add	r3, r12
2:	nop

	dt	r8
	mov.l	@r15, r0	/* Stride between lines, excluding content */

	add	r6, r1
	nop

	add	r7, r12
	nop

	bf.s	4b
	add	r0, r5

	mov.l	@r15+, r0
	mov.l	@r15+, r14
	mov.l	@r15+, r13
	mov.l	@r15+, r12
	mov.l	@r15+, r11
	mov.l	@r15+, r10
	mov.l	@r15+, r9
	rts
	mov.l	@r15+, r8
.endm

_image_linear_rgb16:
	GEN_LINEAR_LOOP mov.w, 2

_image_linear_p8:
	GEN_LINEAR_LOOP mov.b, 1

#endif
