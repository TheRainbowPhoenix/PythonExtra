#include <gint/config.h>
#ifdef GINT_OS_CP

/*
Compressed VRAM backup for the fx-CP, with RLE encoding and palette indexing.

I went a bit overboard with this--I think it was just fun to optimize the heck
out of it. Essentially when we start an fx-CP add-in we save the VRAM and
restore it when leaving, which is essentially what the loader expects us to do.
Normally we'd use the OS's LCD_VRAMBackup() and LCD_VRAMRestore() functions
which just memcpy() to another VRAM-sized buffer.

However, we want to use that other buffer, which is at a fixed address, to load
code. So we have to backup elsewhere. The standard idea, which CPDoom does, is
just to allocate a third VRAM-sized buffer in the heap. But that's 330 kB, and
the copy still takes some 30 ms due to the sheer amount of data. I wanted to
optimize that.

This file implements a compressed encoding of the VRAM data. It's slightly
lossy but the difference isn't noticeable unless you know what to look for or
flip between the original and compressed frames. Conceptually, the encoding has
three phases:

1. Reduce the number of colors by or-ing all pixels with a mask (namely 0x0821)
2. Index the entire picture through a palette
3. RLE-encode the indexed data

Step 2 uses a predefined palette that I extracted from the HollyHock loading
GUI and sorted by decreasing frequency (to optimize linear search, which, yes,
appears to be faster than binary search in this case, due to the imbalance I
guess). The HHK loading screen never really changes but if a color appears
that's not in the palette it will use the last slot as a safety default. The
total size of the palette is 109 colors + the last slot.

Step 3 uses a simple RLE encoding. For most runs, the encoding is 2 bytes: the
color index in the palette (< 110), then the length of the run (≤ 255). For
length-1 runs, the encoding is the color index in the palette + 110, which
takes a single byte.

This results in typical frames of 20-30 kB and a save time of ~22 ms, which is
a 10-15x space improvement and ~25% time improvement over memcpy (29.5 ms) and
even slightly more over LCD__VRAMBackup() (35.5 ms for some reason).

For the sake of future readability, below is the encoding function that, up to
typical assembly optimizations, the implementation follows.

static int cvt_encode(u16 const *colors, u16 mask, u8 *output, int count)
{
    u16 *backup = CVT_SOURCE;
    u16 *next = backup + (DWIDTH * DHEIGHT);
    u8 *output_init = output;

    u16 next_backup = next[0];
    next[0] = ~next[-1];

    while(backup < next) {
        u16 run_color = *backup++ | mask;

        int index = 0;
        while(index < 0xff && colors[index] != run_color)
            index++;

        if((*backup | mask) != run_color) {
            *output++ = count + index;
            continue;
        }

        int run_length = 1;
        do run_length++, backup++;
        while((*backup | mask) == run_color);

        while(run_length > 0xff) {
            *output++ = index;
            *output++ = 0xff;
            run_length -= 0xff;
        }

        *output++ = index;
        *output++ = run_length;
    }

    next[0] = next_backup;
    return output - output_init;
}
*/

.global	_gint_vrambackup_encode
.global	_gint_vrambackup_palette
.balign 4

#define _index         r2  /* Index of current run's color in palette */
#define _mask          r3  /* Color reduction mask = 0x0821 */
#define _output        r4  /* Output buffer (advances every write) */
#define _input         r5  /* Input pointer (advances every read) */
#define _input_end     r6  /* End of input pointer */
#define _palette       r7  /* Color palette */
#define _palette_end   r8  /* End of palette (_palette + 2 * 109) */

#define _next_pixel    r9  /* Color of second pixel of each run */
#define _run_length    r9  /* Length of any given run */

/* u8 *gint_vrambackup_encode(u8 *output, u16 *in_start, u16 *in_end) */
_gint_vrambackup_encode:
	mov.l	r8, @-r15
	add	#-2, _input_end

	sts.l	pr, @-r15
	mov	#0x08, _mask

	# Set _input_end[1] = ~_input_end[0], which makes the past-the-end
	# pixel value different from the last pixel value, ensuring that the
	# last run ends at the right time without us having to check bounds in
	# the run-length inner loop.
	mov.w	@_input_end, r1
	shll8	_mask

	mov.w	@(2, _input_end), r0
	add	#0x21, _mask

	mov.l	r0, @-r15
	neg	r1, r0

	mov.l	.palette, _palette
	mov	_palette, _palette_end

	mov.w	r0, @(2, _input_end)
	add	#109, _palette_end

	mov.l	r9, @-r15
	add	#109, _palette_end

	mov.l	r10, @-r15
	nop

.loop_run:
	# Determine current run color (r0) and its index in the palette
	# (_index). In order to get a faster lookup we also write the searched
	# value in the past-the-end slot, this way we don't have to check
	# bounds in the palette-search inner loop.
	mov.w	@_input+, r0
	mov	_palette, _index

	mov.w	@_input, _next_pixel
	nop

	# Precharge the palette-search loop
	mov.w	@_index+, r1
	or	_mask, r0

	mov.w	r0, @_palette_end
	or	_mask, _next_pixel

	#=== Palette search ===#

.ps:	cmp/eq	r1, r0
	mov.w	@_index+, r1

	bf	.ps
1:	nop

	# Compute index from pointer difference
	# _index = (_index - _palette - 2) / 2
	sub	_palette, _index
	nop

	add	#-4, _index
	nop

	#=== Length-1-run fast path ===#

	# Avoid the run-length loop if the run is of length 1. We have a
	# special encoding for that anyway.
	cmp/eq	_next_pixel, r0
	nop

	bf.s	.length_1_run
	shlr	_index

.length_n_run:
	# Compute the run length. Here we use the _run_length register to save
	# up the value of _input and we'll compute the difference after the
	# loop.
	mov.w	@_input+, r1 /* LS-based increment */
	mov	_input, _run_length

	mov.w	@_input+, r1
	nop

	# (bubble)

	#=== Run length computation ===#

.rl:	or	_mask, r1
	nop

	cmp/eq	r1, r0
	mov.w	@_input+, r1

	bt.s	.rl
1:	nop

	#=== Run generation ===#

	mov	_input, r1
	sub	_run_length, r1

	mov	r1, _run_length
	shlr	_run_length

	add	#-4, _input
	nop

	mov	#-1, r1
	shll8	r1

	# While _run_length > 0xff, generate 0xff-length runs
	tst	r1, _run_length
	mov	#-1, r0

	bt	.sre
	extu.b	r0, r0

.sr:	# (bubble after jump)

	mov.b	_index, @_output
	sub	r0, _run_length

	mov.b	r0, @(1, _output)
	tst	r1, _run_length

	bf.s	.sr
	add	#2, _output

	# Generate the last, short run
.sre:
	mov.b	_index, @_output
	mov	_run_length, r0

	cmp/hs	_input_end, _input
	mov.b	r0, @(1, _output)

	bf.s	.loop_run
	add	#2, _output

.length_1_run:
	# Encode byte as palette size + color index + 1.
	add	#110, _index
	mov.b	_index, @_output

	cmp/hs	_input_end, _input
	nop

	bf.s	.loop_run
	add	#1, _output

.end:
	# Restore _input_end[1] and leave
	mov.l	@r15+, r10
	nop

	mov.l	@r15+, r9
	nop

	mov.l	@r15+, r0
	nop

	mov.w	r0, @(2, _input_end)
	nop

	lds.l	@r15+, pr
	mov	_output, r0

	rts
	mov.l	@r15+, r8

.palette:
	.long	_gint_vrambackup_palette

/* Palette of colors used by the launch GUI, sorted by decreasing frequency */
_gint_vrambackup_palette:
	.word 0xffff, 0x5aeb, 0xffbf, 0xef3d, 0x8c31, 0xef7d, 0xdefb, 0xff7f
	.word 0x7baf, 0xbdb7, 0x9cb3, 0x4aab, 0xcebb, 0xbdf7, 0xad75, 0xdebb
	.word 0xce79, 0x9db9, 0xce39, 0x8cf5, 0x9cf5, 0xefbd, 0x6eff, 0x0821
	.word 0x7c31, 0xad35, 0x7bf1, 0xeefd, 0x7bef, 0x9cf3, 0x8bf1, 0xdf3b
	.word 0xbe39, 0x8c2f, 0x6b6d, 0xadf9, 0xadb9, 0x9d77, 0x9d37, 0x5e7d
	.word 0x6baf, 0x6ebf, 0x9d35, 0x6b6f, 0x8c71, 0x7c73, 0x8cb1, 0x9c73
	.word 0x5b2d, 0x4aeb, 0x5dfb, 0x8cb3, 0xefbf, 0xff7d, 0xdefd, 0xce7b
	.word 0x5dfd, 0xbe3b, 0x7c71, 0x5aed, 0x8bef, 0x4dbb, 0x9efd, 0x4cf9
	.word 0x4c75, 0x6ebd, 0x4d39, 0xbe37, 0xacf5, 0xde7b, 0x5d37, 0x5e3d
	.word 0x5ebf, 0x5d79, 0x4d79, 0x6f3f, 0x6c73, 0x6e7d, 0x6e3d, 0x8dfb
	.word 0x7dbb, 0x6c33, 0x9d33, 0xcdf9, 0x4dfb, 0x6cb5, 0x6c75, 0x5cf7
	.word 0x4d7b, 0x4cb7, 0x9eff, 0x5e7f, 0xaebd, 0xdf7f, 0xbefd, 0x6c31
	.word 0x4c35, 0x4dfd, 0x5d39, 0x5ebd, 0x9c71, 0x7e3d, 0x7e3b, 0xcf3f
	.word 0x6e3b, 0xcf3d, 0x7dfb, 0x5c33, 0x8df9
	.word 0x0000 /* Extra entry for bounds non-check safety net */

#endif /* GINT_OS_CP */
