#include <gint/display.h>
#include <gint/defs/types.h>
#include "../render/render.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

int dgetpixel(int x, int y)
{
	if(x < dwindow.left || x >= dwindow.right) return -1;
	if(y < dwindow.top || y >= dwindow.bottom) return -1;

	DMODE_OVERRIDE(dgetpixel, x, y);

	int offset = (y << 2) + (x >> 5);
	uint32_t mask = 1 << (~x & 31);

	return (gint_vram[offset] & mask) ? C_BLACK : C_WHITE;
}

#endif /* GINT_RENDER_MONO */
