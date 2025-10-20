#include <gint/display.h>
#include <gint/defs/util.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

/* gint_dhline(): Optimized horizontal line */
void gint_dhline(int x1, int x2, int y, uint16_t color)
{
	if(y < dwindow.top || y >= dwindow.bottom) return;
	if(x1 > x2) swap(x1, x2);
	if(x1 >= dwindow.right || x2 < dwindow.left) return;
	x1 = max(x1, dwindow.left);
	x2 = min(x2, dwindow.right - 1);

	int offset = DWIDTH * y;

	/* Use longwords to do the copy, but first paint the endpoints to heed
	   for odd x1 and x2. Checking the parity may be a waste of time. */
	gint_vram[offset + x1] = color;
	gint_vram[offset + x2] = color;

	/* Now round to longword boundaries and copy everything in-between with
	   longwords */
	x1 = x1 + (x1 & 1);
	x2 = (x2 + 1) & ~1;

	uint32_t *start = (void *)(gint_vram + offset + x1);
	uint32_t *end   = (void *)(gint_vram + offset + x2);
	uint32_t op = (color << 16) | color;

	while(end > start) *--end = op;
}

/* gint_dvline(): Optimized vertical line */
void gint_dvline(int y1, int y2, int x, uint16_t color)
{
	if(x < dwindow.left || x >= dwindow.right) return;
	if(y1 > y2) swap(y1, y2);
	y1 = max(y1, dwindow.top);
	y2 = min(y2, dwindow.bottom - 1);

	uint16_t *v = gint_vram + DWIDTH * y1 + x;
	int height = y2 - y1 + 1;

	while(height-- > 0) *v = color, v += DWIDTH;
}

#endif
