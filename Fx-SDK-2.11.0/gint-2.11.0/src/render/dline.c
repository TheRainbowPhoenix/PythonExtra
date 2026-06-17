#include <gint/display.h>
#include <gint/defs/util.h>

#include "../render/render.h"

/* dline(): Bresenham line drawing algorithm
   Remotely adapted from MonochromeLib code by Pierre "PerriotLL" Le Gall.
   Relies on platform-dependent dhline() and dvline() for optimized situations.
   @x1 @y1 @x2 @y2  Coordinates of endpoints of line (included)
   @color           Any color accepted by dpixel() on the platform */
void dline(int x1, int y1, int x2, int y2, int color)
{
	if(color == C_NONE) return;

	/* Possible optimizations */
	if(y1 == y2)
	{
		DMODE_OVERRIDE(gint_dhline, x1, x2, y1, color);
		gint_dhline(x1, x2, y1, color);
		return;
	}
	if(x1 == x2)
	{
		DMODE_OVERRIDE(gint_dvline, y1, y2, x1, color);
		gint_dvline(y1, y2, x1, color);
		return;
	}

	/* Brensenham line drawing algorithm */

	int i, x = x1, y = y1, cumul;
	int dx = x2 - x1, dy = y2 - y1;
	int sx = sgn(dx), sy = sgn(dy);

	dx = (dx >= 0 ? dx : -dx), dy = (dy >= 0 ? dy : -dy);

	dpixel(x1, y1, color);

	if(dx >= dy)
	{
		/* Start with a non-zero cumul to even the overdue between the
		   two ends of the line (for more regularity) */
		cumul = dx >> 1;
		for(i = 1; i < dx; i++)
		{
			x += sx;
			cumul += dy;
			if(cumul > dx) cumul -= dx, y += sy;
			dpixel(x, y, color);
		}
	}
	else
	{
		cumul = dy >> 1;
		for(i = 1; i < dy; i++)
		{
			y += sy;
			cumul += dx;
			if(cumul > dy) cumul -= dy, x += sx;
			dpixel(x, y, color);
		}
	}

	dpixel(x2, y2, color);
}
