#include <gint/display.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

int dgetpixel(int x, int y)
{
	if((uint)x >= DWIDTH || (uint)y >= DHEIGHT) return -1;
	return gint_vram[DWIDTH * y + x];
}

#endif
