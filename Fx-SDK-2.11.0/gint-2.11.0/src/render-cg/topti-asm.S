#include <gint/config.h>
#if GINT_RENDER_RGB
// TODO: topti-asm: avoid duplicating DWIDTH here
#if GINT_HW_CG
# define DWIDTH 396
#elif GINT_HW_CP
# define DWIDTH 320
#endif

.global _topti_glyph_fg_bg
.global _topti_glyph_fg
.global _topti_glyph_bg
.text

# Glyph rendering functions.
# These are pretty naive, using only word accesses to index the VRAM and
# absolute positions to index the glyph data, instead of shiting a single
# longword to real all bits in order. This is because we only render a subglyph
# (for clipping) so there'a non-zero stride in glyph data.

# Parameters:
#  r4:        vram
#  r5:        data
#  r6:        color (either fg, bg, or (fg << 16) | bg)
#  r7:        height
#  @(4,r15):  width
#  @(8,r15):  dataw - width (stride)
#  @(12,r15): starting index in data
# Stack:
#  @(0,r15): r8 save

# Register allocation:
#  r0: (temporary)
#  r1: (temporary)
#  r2: x counter
#  r3: glyph data index
#  r4: vram pointer
#  r5: glyph pointer
#  r6: color
#  r7: y counter
# Callee-saved registers:
# r8: vram stride

# Opaque foreground, opaque background

_topti_glyph_fg_bg:

	# Compute VRAM stride 2 * (DWIDTH-width)
	mov.l	r8, @-r15
	mov.l	1f, r8
	mov.l	@(4, r15), r3
	shll	r3
	sub	r3, r8

	# Load the starting index
	mov.l	@(12, r15), r3

.fg_bg_y:
	# Initialize width counter
	mov.l	@(4, r15), r2

.fg_bg_x:
	# Load one bit of data in T
	mov	r3, r0
	mov	#-5, r1
	shld	r1, r0
	shll2	r0
	mov.l	@(r0, r5), r1

	mov	r3, r0
	and	#31, r0

	shld	r0, r1
	shll	r1

	# Select the correct 16 bits or r6
	bf/s	.fg_bg_zero
	mov	r6, r1
	swap.w	r6, r1

.fg_bg_zero:
	# Write color to VRAM
	mov.w	r1, @r4
	add	#2, r4

	# Leave the x-loop if x counter reaches 0
	dt	r2
	bf/s	.fg_bg_x
	add	#1, r3

	# Move to next row, leave the y-loop if height reaches 0
	dt	r7
	mov.l	@(8, r15), r0
	add	r0, r3
	bf/s	.fg_bg_y
	add	r8, r4

	rts
	mov.l	@r15+, r8

# Opaque foreground, transparent background

_topti_glyph_fg:

	# Compute VRAM stride 2 * (DWIDTH-width)
	mov.l	r8, @-r15
	mov.l	1f, r8
	mov.l	@(4, r15), r3
	shll	r3
	sub	r3, r8

	# Load the starting index
	mov.l	@(12, r15), r3

.fg_y:
	# Initialize width counter
	mov.l	@(4, r15), r2

.fg_x:
	# Load one bit of data in T
	mov	r3, r0
	mov	#-5, r1
	shld	r1, r0
	shll2	r0
	mov.l	@(r0, r5), r1

	mov	r3, r0
	and	#31, r0

	shld	r0, r1
	shll	r1

	# Write color to VRAM only if it's a 1 bit
	bf	.fg_next
	mov.w	r6, @r4

.fg_next:
	# Leave the x-loop if x counter reaches 0
	add	#2, r4
	dt	r2
	bf/s	.fg_x
	add	#1, r3

	# Move to next row, leave the y-loop if height reaches 0
	dt	r7
	mov.l	@(8, r15), r0
	add	r0, r3
	bf/s	.fg_y
	add	r8, r4

	rts
	mov.l	@r15+, r8

# Transparent foreground, opaque background

_topti_glyph_bg:

	# Compute VRAM stride 2 * (DWIDTH-width)
	mov.l	r8, @-r15
	mov.l	1f, r8
	mov.l	@(4, r15), r3
	shll	r3
	sub	r3, r8

	# Load the starting index
	mov.l	@(12, r15), r3

.bg_y:
	# Initialize width counter
	mov.l	@(4, r15), r2

.bg_x:
	# Load one bit of data in T
	mov	r3, r0
	mov	#-5, r1
	shld	r1, r0
	shll2	r0
	mov.l	@(r0, r5), r1

	mov	r3, r0
	and	#31, r0

	shld	r0, r1
	shll	r1

	# Write color to VRAM only if it's a 0 bit
	bt	.bg_next
	mov.w	r6, @r4

.bg_next:
	# Leave the x-loop if x counter reaches 0
	add	#2, r4
	dt	r2
	bf/s	.bg_x
	add	#1, r3

	# Move to next row, leave the y-loop if height reaches 0
	dt	r7
	mov.l	@(8, r15), r0
	add	r0, r3
	bf/s	.bg_y
	add	r8, r4

	rts
	mov.l	@r15+, r8

# Data

.align 4

1:	.long DWIDTH*2

#endif
