#include <gint/config.h>
#if GINT_RENDER_MONO

.global _bopti_gasm_mono_scsp
.global _bopti_gasm_mono_alpha_scsp
.global _bopti_gasm_gray_scsp
.global _bopti_gasm_gray_alpha_scsp

/* REGISTER ALLOCATION:
   r0: OR layer
   r1: (temp)
   r2: light vram longword
   r3: dark vram longword
   --
   r4: light pointer
   r5: layer pointer
   r6: mask
   r7: dark pointer
   --
   @r15: -(x&31) */

_bopti_gasm_mono_scsp:
	/* Read layer longword and shift it */
	mov.l	@r5, r0
	mov.l	@r15, r1
	shld	r1, r0

	/* Clear target VRAM and unwanted image data */
	and	r6, r0
	mov.l	@r4, r2
	not	r6, r6
	mov.l	@r7, r3
	and	r6, r2
	and	r6, r3

	/* Blit and return */
	or	r0, r2
	or	r0, r3
	mov.l	r2, @r4
	rts
	mov.l	r3, @r7

/* REGISTER ALLOCATION:
   r0: AND layer
   r1: (temp)
   r2: light vram longword
   r3: dark vram longword
   --
   r4: light pointer
   r5: layer pointer, then OR layer
   r6: mask
   r7: dark pointer
   --
   @r15: -(x&31) */
_bopti_gasm_mono_alpha_scsp:
	/* Read layer longwords and shift them */
	mov.l	@r5, r0
	mov.l	@r15, r1
	mov.l	@(4,r5), r5
	shld	r1, r0
	shld	r1, r5

	/* Clear any unwanted image data */
	and	r6, r0
	mov.l	@r4, r2
	and	r6, r5
	mov.l	@r7, r3

	/* Blit and return */
	not	r0, r0
	and	r0, r2
	and	r0, r3
	or	r5, r2
	or	r5, r3
	mov.l	r2, @r4
	rts
	mov.l	r3, @r7

/* REGISTER ALLOCATION:
   r0: LIGHT layer
   r1: (temp)
   r2: light vram longword
   r3: dark vram longword
   --
   r4: light pointer
   r5: layer pointer, then DARK layer
   r6: mask
   r7: dark pointer
   --
   @r15: -(x&31) */
_bopti_gasm_gray_scsp:
	/* Read layer longwords and shift them */
	mov.l	@r5, r0
	mov.l	@r15, r1
	mov.l	@(4,r5), r5
	shld	r1, r0
	shld	r1, r5

	/* Clear target VRAM and unapplied image data */
	and	r6, r0
	mov.l	@r4, r2
	and	r6, r5
	mov.l	@r7, r3
	not	r6, r6
	and	r6, r2
	and	r6, r3

	/* Blit and return */
	or	r0, r2
	or	r5, r3
	mov.l	r2, @r4
	rts
	mov.l	r3, @r7

/* REGISTER ALLOCATION:
   r0: AND layer
   r1: LIGHT layer
   r2: (temp), then light vram longword
   r3: dark vram longword
   --
   r4: light pointer
   r5: layer pointer, then DARK layer
   r6: mask
   r7: dark pointer
   --
   @r15: -(x&31) */
_bopti_gasm_gray_alpha_scsp:
	/* Read layer longwords and shift them */
	mov.l	@r5, r0
	mov.l	@(4,r5), r1
	mov.l	@(8,r5), r5
	mov.l	@r15, r2
	shld	r2, r0
	shld	r2, r1
	shld	r2, r5

	/* Clear unappliqed image data */
	and	r6, r0
	and	r6, r1
	and	r6, r5

	/* Blit the AND layer */
	mov.l	@r4, r2
	not	r0, r0
	mov.l	@r7, r3
	and	r0, r2
	and	r0, r3

	/* Blit the LIGHT and DARY layers, and return */
	or	r1, r2
	or	r5, r3
	mov.l	r2, @r4
	rts
	mov.l	r3, @r7

#endif /* GINT_RENDER_MONO */
