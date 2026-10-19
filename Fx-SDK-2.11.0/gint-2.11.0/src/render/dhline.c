#include <gint/display.h>

/* dhline(): Full-width horizontal line */
void dhline(int y, int color)
{
	dline(dwindow.left, y, dwindow.right - 1, y, color);
}
