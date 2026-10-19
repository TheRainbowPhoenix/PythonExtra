#include <gint/display.h>
#include <gint/defs/util.h>
#include <gint/config.h>

#if GINT_RENDER_MONO

/* gint_ghline(): Optimized horizontal line, but not actually optimized */
void gint_ghline(int x1, int x2, int y, int color)
{
	if(x1 > x2) swap(x1, x2);
	for(int x = x1; x <= x2; x++) dpixel(x, y, color);
}

/* gint_gvline(): Optimized horizontal line, but not actually optimized */
void gint_gvline(int y1, int y2, int x, int color)
{
	if(y1 > y2) swap(y1, y2);
	for(int y = y1; y <= y2; y++) dpixel(x, y, color);
}

#endif /* GINT_RENDER_MONO */
