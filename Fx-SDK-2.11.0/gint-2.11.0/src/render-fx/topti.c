#include <gint/defs/types.h>
#include <gint/defs/attributes.h>
#include <gint/defs/util.h>
#include <gint/display.h>

#include "../render/render.h"
#include "render-fx.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

#undef dtext_opt

/* Default font */
extern font_t gint_font5x7;
font_t const * gint_default_font = &gint_font5x7;
font_t const * topti_font = &gint_font5x7;

/* topti_split(): Split glyph data into lines
   This function splits the data from [glyph] into lines and writes a bit of
   each line in [operators]. This operation is meant to be used multiple times
   in a row, so [free] represents the number of free low bits in [operators].

   @glyph      Raw glyph data from the font
   @width      Width of glyph (1 <= width <= 32)
   @height     Storage height
   @free       Number of free low bits in [operators]
   @operators  VRAM operands

   Returns the number of free bits in [operators] after the operation. If it's
   0, call topti_draw() and reset the operators. If it's negative, call
   topti_draw() then do another pass of topti_split() to recover the missing
   information. */
static int topti_split(uint32_t const * glyph, int width, int height, int free,
	uint32_t *operators)
{
	/* Extracts [width] bits on the left of [*glyph] */
	uint32_t glyph_mask = 0xffffffff << (32 - width);
	/* Shifts from the left of [*glyph] to the free bits of [operators] */
	int shift;

	uint32_t data = *glyph++;
	/* Number of bits remaining in [data] */
	int source = 32;

	for(int i = 0; i < height; i++)
	{
		shift = 32 - free;

		/* Read [width] data bits and put them in the operator.
		   * There may not be [width] bits left in [data]; this
		     situation is detected and cared for later. (*1)
		   * There may not be enough space to store [width] bits; this
		     is detected by topti_render(). (*2)
		   * We may have available > 32 as a result of the previous
		     case, so shift carefully. */
		uint32_t line = data & glyph_mask;
		line = (shift >= 0) ? (line >> shift) : (line << -shift);
		operators[i] |= line;

		data <<= width;
		source -= width;

		/* Continue iterating as long as no information is lost */
 		if(source >= 0) continue;

 		/* (*1) Now load a new [data] */
 		uint32_t partial_mask = 0xffffffff << (source + 32);
 		data = *glyph++;
 		shift += source + width;

 		/* shift>=32 means the the information we lost in (*1) does not
 		   fit in the operators, making this a case of (*2). */
 		if(shift < 32)
 		{
 			/* Recover lost bits */
 			uint32_t line = data & partial_mask;
 			line = (shift>=0) ? (line >> shift) : (line << -shift);
 			operators[i] |= line;
 		}

 		data <<= -source;
 		source += 32;
 	}

 	return free - width;
}

/* topti_render(): Render a string on the VRAM */
void topti_render(int x, int y, char const *str_char, font_t const *f,
	asm_text_t *asm_fg, asm_text_t *asm_bg, uint32_t *v1, uint32_t *v2,
	int size)
{
	uint8_t const *str = (void *)str_char;
	uint8_t const *str0 = str;

	/* Storage height and number of free bits in operators[] */
	int height = f->data_height, free;
	/* Raw glyph data */
	uint32_t const *data = f->data;

	/* Basic clipping */
	if(x >= dwindow.right || y >= dwindow.bottom) return;
	if(y + height <= dwindow.top) return;
	height = min(height, dwindow.bottom - y);

	/* How much we skip vertically if we render at y < dwindow.top */
	int vdisp = 0;
	if(y < dwindow.top)
	{
		vdisp = dwindow.top - y;
		y = dwindow.top;
	}
	if(vdisp >= height) return;

	uint32_t bg_mask[4];
	masks(dwindow.left, dwindow.right, bg_mask);

	/* Operator data and background */
	uint32_t operators[height];
	uint32_t bg[height];
	for(int i = 0; i < height; i++)
	{
		operators[i] = 0;
		bg[i] = 0xffffffff >> (x & 31);
	}

	/* Put an initial offset to the operators to honor the x coordinate */
	free = 32 - (x & 31);
	x >>= 5;

	/* VRAM pointers */
	v1 += (y << 2) + x;
	v2 += (y << 2) + x;

	/* Pull each character into the operator buffer */
	while(1)
	{
		uint32_t code_point = dtext_utf8_next(&str);
		if(!code_point || (size >= 0 && str - str0 > size)) break;

		int glyph = dfont_glyph_index(f, code_point);
		if(glyph < 0) continue;

		int index = dfont_glyph_offset(f, glyph);

		/* Put glyph data into the operators */
		int width = f->prop ? f->glyph_width[glyph] : f->width;
		free = topti_split(data+index, width, height, free, operators);

		/* Potential space after the glyph */
		int space = (*str != 0) ? f->char_spacing : 0;
		free -= space;

		if(free > 0) continue;

		/* Once operators are full, update VRAM and start again */

		if(x >= 0)
		{
			for(int i = 0; i < height; i++)
			{
				operators[i] &= bg_mask[x];
				bg[i] &= bg_mask[x];
			}

			asm_bg(v1, v2, bg + vdisp, height - vdisp);
			asm_fg(v1, v2, operators + vdisp, height - vdisp);
		}
		v1++, v2++;
		if(++x >= 4) break;

		for(int i = 0; i < height; i++)
		{
			operators[i] = 0;
			bg[i] = 0xffffffff;
		}
		free += 32;

		/* If information was lost in the split, finish it */

		if(free + space >= 32) continue;

		free += width + space;
		free = topti_split(data+index, width, height, free, operators);
		free -= space;
	}

	/* Put the final longwords */
	if(x >= 0 && x < 4 && free < 32)
	{
		for(int i = 0; i < height; i++)
		{
			operators[i] &= bg_mask[x];
			bg[i] &= bg_mask[x] & ~((1 << free) - 1);
		}
		asm_bg(v1, v2, bg + vdisp, height - vdisp);
		asm_fg(v1, v2, operators + vdisp, height - vdisp);
	}
}

/* dtext_opt(): Display a string of text */
void dtext_opt(int x, int y, int fg, int bg, int halign, int valign,
	char const *str, int size)
{
	if((uint)fg >= 8 || (uint)bg >= 8) return;

	DMODE_OVERRIDE(dtext_opt, x, y, fg, bg, halign, valign, str, size);

	if(halign != DTEXT_LEFT || valign != DTEXT_TOP)
	{
		int w, h;
		dnsize(str, size, topti_font, &w, &h);

		if(halign == DTEXT_RIGHT)  x -= w - 1;
		if(halign == DTEXT_CENTER) x -= (w >> 1);
		if(valign == DTEXT_BOTTOM) y -= h - 1;
		if(valign == DTEXT_MIDDLE) y -= (h >> 1);
	}

	topti_render(x, y, str, topti_font, topti_asm_text[fg],
		topti_asm_text[bg], gint_vram, gint_vram, size);
}

#endif /* GINT_RENDER_MONO */
