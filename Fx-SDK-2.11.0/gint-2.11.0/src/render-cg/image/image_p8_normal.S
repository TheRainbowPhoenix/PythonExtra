#include <gint/config.h>
#if GINT_RENDER_RGB

.global _gint_image_p8_normal
#include "image_macros.S"

/* P8 Opaque rendering, RAM version: trivial.

   As usual with RAM it is fairly easy to bottleneck writing speed, and so
   there is no need for complex methods. Building longwords could be an option,
   but it would require output alignment with edges, which is painful. */

.macro GEN_NORMAL_LOOP HFLIP, OUT_DIR
	mov.l	@r8+, r9	/* cmd.palette */

 .if \HFLIP
	add	#-2, r5
	mov	r2, r0
	shll	r0
	add	r0, r5
	shll	r0
	add	r0, r6
 .endif

1:	mov	r2, r8

2:	mov.b	@r3+, r0
	shll	r0
	mov.w	@(r0, r9), r0
	mov.w	r0, @r5

3:	dt	r8
	bf.s	2b
	add	#\OUT_DIR, r5

	END
	EPILOGUE
.endm

_gint_image_p8_normal:
	tst	#1, r0
	bf	9f

	GEN_NORMAL_LOOP 0, 2
9:	GEN_NORMAL_LOOP 1, -2

#endif
