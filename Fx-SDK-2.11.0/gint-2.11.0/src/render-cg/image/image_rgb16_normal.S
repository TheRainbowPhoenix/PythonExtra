#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_rgb16_normal
#include "image_macros.S"

/* RGB16 Opaque rendering, RAM version: by longword access.

   This function of the image renderer is designed for the RAM model only. At
   default overclock levels, the RAM can register a write every 13-14 cycles,
   regardless of size. Since this amount of time is more than enough to build a
   target longword regardless of alignment and geometry considerations, the
   main and only focus of this function is to only write longwords.

   Since longwords can only be written at 4-aligned addresses and always make
   pairs of pixels, there are variations on the loop depending on the rendered
   width and destination. These are marked with the following convention:

   * w1 / w2 denotes the parity of the command width;
   * o2 / o4 denotes the alignment of the output.

   There is a forward and a backward variation for all four combinations of
   these parameters, noted F_ and B_ in label names. Some word-based variations
   are provided for width ≤ 8, which is just a way to ensure that the longword-
   based loops always have a least one interation, since they're implemented as
   do/while.

   The loops themselves are nowhere near tight on the CPU side and entirely
   bottlenecked by the RAM, hence the simplicity and complete disregard for
   superscalar parallelism. */

_gint_image_rgb16_normal:
	/* We use word copy for width ≤ 8; this is to ensure that there is at
	   least one longword in the non-trivial loop, simplifying checks */
	tst	#1, r0
	mov	#8, r0

	bf.s	.BACKWARD
	cmp/ge	r2, r0

.FORWARD:
	bt	_FORWARD_WORD_COPY
	nop

	bra	_FORWARD_LONG_COPY
	nop

.BACKWARD:
	mov	r2, r0
	add	r0, r0
	add	r0, r5
	add	r0, r0

	bt.s	_BACKWARD_WORD_COPY
	add	r0, r6

	bra	_BACKWARD_LONG_COPY
	nop

_FORWARD_WORD_COPY:
	START
2:	movs.w	@r3+, x0
3:	movs.w	x0, @r5+
	END
	EPILOGUE

_BACKWARD_WORD_COPY:
	START
2:	movs.w	@r3+, x0
3:	movs.w	x0, @-r5
	END
	EPILOGUE

_FORWARD_LONG_COPY:
	shlr	r2		/* Test width parity */
	mov	#2, r0

	bt	.F_w1
	nop

.F_w2:	tst	r0, r5		/* Test alignment of output */
	bf	.F_w2o2

.F_w2o4:
	START
2:	mov.w	@r3+, r0
	mov.w	@r3+, r7
	shll16	r7
	xtrct	r0, r7
	mov.l	r7, @r5
3:	add	#4, r5
	END
	EPILOGUE

.F_w2o2:
	add	#-1, r2
	START
	mov.w	@r3+, r0
	mov.w	r0, @r5
	add	#2, r5
2:	mov.w	@r3+, r0
	mov.w	@r3+, r7
	shll16	r7
	xtrct	r0, r7
	mov.l	r7, @r5
3:	add	#4, r5
	mov.w	@r3+, r0
	mov.w	r0, @r5
	add	#2, r5
	END
	EPILOGUE

.F_w1:	tst	r0, r5		/* Test alignment of output */
	bf	.F_w1o2

.F_w1o4:
	START
2:	mov.w	@r3+, r0
	mov.w	@r3+, r7
	shll16	r7
	xtrct	r0, r7
	mov.l	r7, @r5
3:	add	#4, r5
	mov.w	@r3+, r0
	mov.w	r0, @r5
	add	#2, r5
	END
	EPILOGUE

.F_w1o2:
	START
	mov.w	@r3+, r0
	mov.w	r0, @r5
	add	#2, r5
2:	mov.w	@r3+, r0
	mov.w	@r3+, r7
	shll16	r7
	xtrct	r0, r7
	mov.l	r7, @r5
3:	add	#4, r5
	END
	EPILOGUE

_BACKWARD_LONG_COPY:
	shlr	r2		/* Test width parity */
	mov	#2, r0

	bt	.B_w1
	nop

.B_w2:	tst	r0, r5		/* Test alignment of output */
	bf	.B_w2o2

.B_w2o4:
	START
2:	mov.w	@r3+, r0
	mov.w	@r3+, r7
	shll16	r0
	xtrct	r7, r0
3:	mov.l	r0, @-r5
	END
	EPILOGUE

.B_w2o2:
	add	#-1, r2
	START
	mov.w	@r3+, r0
	mov.w	r0, @-r5
2:	mov.w	@r3+, r0
	mov.w	@r3+, r7
	shll16	r0
	xtrct	r7, r0
3:	mov.l	r0, @-r5
	mov.w	@r3+, r0
	mov.w	r0, @-r5
	END
	EPILOGUE

.B_w1:	tst	r0, r5		/* Test alignment of output */
	bf	.B_w1o2

.B_w1o4:
	START
2:	mov.w	@r3+, r0
	mov.w	@r3+, r7
	shll16	r0
	xtrct	r7, r0
3:	mov.l	r0, @-r5
	mov.w	@r3+, r0
	mov.w	r0, @-r5
	END
	EPILOGUE

.B_w1o2:
	START
	mov.w	@r3+, r0
	mov.w	r0, @-r5
2:	mov.w	@r3+, r0
	mov.w	@r3+, r7
	shll16	r0
	xtrct	r7, r0
3:	mov.l	r0, @-r5
	END
	EPILOGUE

#endif
