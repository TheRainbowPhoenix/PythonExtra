#include <gint/defs/util.h>
#include <gint/display.h>
#include "../render/render.h"
#include "render-fx.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

void drect(int x1, int y1, int x2, int y2, int color)
{
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

	DMODE_OVERRIDE(drect, x1, y1, x2, y2, color);

	/* Use masks to get the work done fast! */
	uint32_t m[4];
	masks(x1, x2, m);

	uint32_t *base	= gint_vram + (y1 << 2);
	uint32_t *lword	= gint_vram + (y2 << 2) + 4;

	if(color == C_WHITE) while(lword > base)
	{
		*--lword &= ~m[3];
		*--lword &= ~m[2];
		*--lword &= ~m[1];
		*--lword &= ~m[0];
	}
	else if(color == C_BLACK) while(lword > base)
	{
		*--lword |= m[3];
		*--lword |= m[2];
		*--lword |= m[1];
		*--lword |= m[0];
	}
	else if(color == C_INVERT) while(lword > base)
	{
		*--lword ^= m[3];
		*--lword ^= m[2];
		*--lword ^= m[1];
		*--lword ^= m[0];
	}
}

#endif /* GINT_RENDER_MONO */
