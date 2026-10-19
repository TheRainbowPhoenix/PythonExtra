#include <gint/display.h>

/* dvline(): Full-height vertical line */
void dvline(int x, int color)
{
	dline(x, dwindow.top, x, dwindow.bottom - 1, color);
}
