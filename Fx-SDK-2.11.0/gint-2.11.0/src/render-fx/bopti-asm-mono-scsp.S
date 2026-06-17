#include <gint/config.h>
#if GINT_RENDER_MONO

.global _bopti_asm_mono_scsp
.global _bopti_asm_mono_alpha_scsp

/* REGISTER ALLOCATION:
   r0: layer
   r1: -
   r2: (temp)
   r3: vram longword
   --
   r4: vram pointer
   r5: layer pointer
   r6: mask
   r7: -(x&31) */

_bopti_asm_mono_scsp:
	/* Read layer longword and shift it */
	mov.l	@r5, r0
	shld	r7, r0

	/* Clear the target VRAM and unwanted image data */
	mov.l	@r4, r3
	and	r6, r0
	not	r6, r6
	and	r6, r3

	/* Blit and return */
	or	r0, r3
	rts
	mov.l	r3, @r4

/* REGISTER ALLOCATION:
   r0: AND layer
   r1: OR layer
   r2: (temp)
   r3: vram longword
   --
   r4: vram pointer
   r5: layer pointer
   r6: mask
   r7: -(x&31) */

_bopti_asm_mono_alpha_scsp:
	/* Read layer longwords and shift them */
	mov.l	@r5, r0
	mov.l	@(4,r5), r1
	shld	r7, r0
	shld	r7, r1

	/* Apply masks to clear layer data */
	and	r6, r0
	and	r6, r1

	/* Blit to VRAM */
	mov.l	@r4, r3
	not	r0, r0
	and	r0, r3
	or	r1, r3
	rts
	mov.l	r3, @r4

#endif /* GINT_RENDER_MONO */
