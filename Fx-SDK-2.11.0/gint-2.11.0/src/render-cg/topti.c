#include <gint/defs/types.h>
#include <gint/defs/attributes.h>
#include <gint/defs/util.h>
#include <gint/display.h>

#include <string.h>

#include "../render/render.h"
#include "topti-asm.h"

#include <gint/config.h>
#if GINT_RENDER_RGB

/* Default font */
extern font_t gint_font8x9;
font_t const * gint_default_font = &gint_font8x9;
font_t const * topti_font = &gint_font8x9;

/* topti_glyph(): Render a glyph on the VRAM
   Prints a glyph naively using word accesses, because for most fonts with a
   small size (including gint's 8x9 font) this will be more efficient than the
   complex logic for longword accesses.

   This function assumes that at least one of [fg] and [bg] is not transparent.

   @vram    Target position on VRAM, adjusted to [top], not adjusted to [left]
   @data    Glyph data
   @left    Left-position of subglyph
   @top     Top-Position of subglyph
   @width   Subglyph width
   @height  Subglyph height
   @dataw   Glyph width
   @fg @bg  Foreground and background colors */
static void topti_glyph(uint16_t *vram, uint32_t const * data, int left,
	int top, int width, int height, int dataw, int fg, int bg)
{
	int index = top * dataw + left;

	/* Most common situation: opaque text on transparent background */
	if(bg < 0) topti_glyph_fg(vram + left, data, fg, height, width,
		dataw - width, index);
	/* Full text on opaque background */
	else if(fg >= 0) topti_glyph_fg_bg(vram + left, data, (fg << 16) | bg,
		height, width, dataw - width, index);
	/* Draw background but not text */
	else topti_glyph_bg(vram + left, data, bg, height, width,
		dataw - width, index);
}

static void topti_render(int x, int y, char const *str_char, font_t const *f,
	int fg, int bg, int size)
{
	uint8_t const *str = (void *)str_char;
	uint8_t const *str0 = str;

	/* Raw glyph data */
	uint32_t const *data = f->data;

	/* Storage height, top position within glyph */
	int height = f->data_height, top = 0;

	/* Vertical clipping */
	if(x >= dwindow.right || y >= dwindow.bottom) return;
	if(y + height <= dwindow.top) return;

	int top_overflow = y - dwindow.top;
	if(top_overflow < 0) {
		top = -top_overflow;
		height += top_overflow;
		y -= top_overflow;
	}
	height = min(height, dwindow.bottom - y);
	if(height <= 0) return;

	/* Move to top row */
	uint16_t *target = gint_vram + DWIDTH * y;

	/* Character spacing waiting to be drawn, in pixels */
	int space = 0;

	/* Read each character from the input string */
	while(1)
	{
		uint32_t code_point = dtext_utf8_next(&str);
		if(!code_point || (size >= 0 && str - str0 > size)) break;

		int glyph = dfont_glyph_index(f, code_point);
		if(glyph < 0) continue;

		int dataw = f->prop ? f->glyph_width[glyph] : f->width;

		/* Draw character spacing if background is opaque */
		if(space && bg >= 0) drect(x, y, x+space-1, y+height-1, bg);
		x += space;
		if(x >= dwindow.right) break;

		int index = dfont_glyph_offset(f, glyph);

		/* Compute horizontal intersection between glyph and screen */

		int width = dataw, left = 0;

		if(x + dataw <= dwindow.left)
		{
			x += dataw;
			space = f->char_spacing;
			continue;
		}
		if(x < dwindow.left) {
			left = dwindow.left - x;
			width -= left;
		}
		width = min(width, dwindow.right - x);

		/* Render glyph */

		topti_glyph(target + x, data + index, left, top, width, height,
			dataw, fg, bg);

		x += dataw;
		space = f->char_spacing;
	}
}

/* dtext_opt(): Display a string of text */
void dtext_opt(int x, int y, int fg, int bg, int halign, int valign,
	char const *str, int size)
{
	if(halign != DTEXT_LEFT || valign != DTEXT_TOP)
	{
		int w, h;
		dnsize(str, size, topti_font, &w, &h);

		if(halign == DTEXT_RIGHT)  x -= w - 1;
		if(halign == DTEXT_CENTER) x -= (w >> 1);
		if(valign == DTEXT_BOTTOM) y -= h - 1;
		if(valign == DTEXT_MIDDLE) y -= (h >> 1);
	}

	topti_render(x, y, str, topti_font, fg, bg, size);
}

#endif
