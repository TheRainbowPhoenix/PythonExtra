#include <gint/display.h>

/* dtext(): Simple version of dtext_opt() with defaults */
void dtext(int x, int y, int fg, char const *str)
{
	dtext_opt(x, y, fg, C_NONE, DTEXT_LEFT, DTEXT_TOP, str);
}
