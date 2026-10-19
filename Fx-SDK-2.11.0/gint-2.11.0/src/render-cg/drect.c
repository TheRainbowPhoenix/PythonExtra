#include <gint/defs/util.h>
#include <gint/display.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void drect(int x1, int y1, int x2, int y2, int color)
{
	if(color == C_NONE) return;

	if(x1 > x2) swap(x1, x2);
	if(y1 > y2) swap(y1, y2);

	/* Rectangle is completely outside the rendering window */
	if(x1 >= dwindow.right || x2 < dwindow.left) return;
	if(y1 >= dwindow.bottom || y2 < dwindow.top) return;
	/* Clipping */
	x1 = max(x1, dwindow.left);
	x2 = min(x2, dwindow.right - 1);
	y1 = max(y1, dwindow.top);
	y2 = min(y2, dwindow.bottom - 1);

	/* The method is exactly like dhline(). I first handle odd endpoints,
	   then write longwords for the longest section */

	uint16_t *base = gint_vram + DWIDTH * y1;
	int height = y2 - y1 + 1;

	/* Now copy everything that's left as longwords */

	int ax1 = x1 + (x1 & 1);
	int ax2 = (x2 + 1) & ~1;

	uint32_t *v = (void *)(base + ax1);
	uint32_t op = (color << 16) | color;
	int width = (ax2 - ax1) >> 1;

	if(color == C_INVERT) for(int h = height; h; h--)
	{
		/* We can't double-draw on base[x1] and base[x2] here */
		if(x1 & 1) base[x1] ^= 0xffff;
		if(!(x2 & 1)) base[x2] ^= 0xffff;
		for(int w = 0; w < width; w++) v[w] ^= 0xffffffff;
		v += DWIDTH / 2;
		base += DWIDTH;
	}
	else for(int h = height; h; h--)
	{
		base[x1] = color;
		base[x2] = color;
		for(int w = 0; w < width; w++) v[w] = op;
		v += DWIDTH / 2;
		base += DWIDTH;
	}
}

#endif
