#include <gint/display.h>
#include <gint/defs/types.h>
#include "../render/render.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

void dpixel(int x, int y, int color)
{
	if(x < dwindow.left || x >= dwindow.right) return;
	if(y < dwindow.top || y >= dwindow.bottom) return;

	DMODE_OVERRIDE(dpixel, x, y, color);

	uint32_t *lword = gint_vram + (y << 2) + (x >> 5);
	uint32_t mask = 1 << (~x & 31);

	if(color == C_WHITE)
	{
		*lword &= ~mask;
	}
	else if(color == C_BLACK)
	{
		*lword |= mask;
	}
	else if(color == C_INVERT)
	{
		*lword ^= mask;
	}
}

#endif /* GINT_RENDER_MONO */
