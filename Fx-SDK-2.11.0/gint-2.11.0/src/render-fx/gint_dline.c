#include <gint/display.h>
#include <gint/defs/util.h>
#include "render-fx.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

/* gint_dhline(): Optimized horizontal line using a rectangle mask */
void gint_dhline(int x1, int x2, int y, int color)
{
	if(y < dwindow.top || y >= dwindow.bottom) return;
	if(x1 > x2) swap(x1, x2);
	if(x1 >= dwindow.right || x2 < dwindow.left) return;

	/* Get the masks for the [x1, x2] range */
	uint32_t m[4];
	masks(x1, x2, m);

	uint32_t *data = gint_vram + (y << 2);

	if(color == C_WHITE)
	{
		data[0] &= ~m[0];
		data[1] &= ~m[1];
		data[2] &= ~m[2];
		data[3] &= ~m[3];
	}
	else if(color == C_BLACK)
	{
		data[0] |= m[0];
		data[1] |= m[1];
		data[2] |= m[2];
		data[3] |= m[3];
	}
	else if(color == C_INVERT)
	{
		data[0] ^= m[0];
		data[1] ^= m[1];
		data[2] ^= m[2];
		data[3] ^= m[3];
	}
}

/* gint_dvline(): Optimized vertical line */
void gint_dvline(int y1, int y2, int x, int color)
{
	if(x < dwindow.left || x >= dwindow.right) return;
	if(y1 > y2) swap(y1, y2);
	if(y1 >= dwindow.bottom || y2 < dwindow.top) return;
	y1 = max(y1, dwindow.top);
	y2 = min(y2, dwindow.bottom - 1);

	uint32_t *base = gint_vram + (y1 << 2) + (x >> 5);
	uint32_t *lword = base + ((y2 - y1 + 1) << 2);
	uint32_t mask = 1 << (~x & 31);

	if(color == C_WHITE)
	{
		while(lword > base) lword -= 4, *lword &= ~mask;
	}
	else if(color == C_BLACK)
	{
		while(lword > base) lword -= 4, *lword |= mask;
	}
	else if(color == C_INVERT)
	{
		while(lword > base) lword -= 4, *lword ^= mask;
	}
}

#endif /* GINT_RENDER_MONO */
