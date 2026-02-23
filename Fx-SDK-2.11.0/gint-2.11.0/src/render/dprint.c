#include <gint/display.h>
#include <stdio.h>

/* dprint(): Display a formatted string */
void dprint_opt(int x, int y, int fg, int bg, int halign, int valign,
	char const *format, ...)
{
	char str[512];
	va_list args;

	va_start(args, format);
	vsnprintf(str, 512, format, args);
	va_end(args);

	dtext_opt(x, y, fg, bg, halign, valign, str);
}

/* dprint(): Simple version of dprint_op() with defaults */
void dprint(int x, int y, int fg, char const *format, ...)
{
	char str[512];
	va_list args;

	va_start(args, format);
	vsnprintf(str, 512, format, args);
	va_end(args);

	dtext(x, y, fg, str);
}
