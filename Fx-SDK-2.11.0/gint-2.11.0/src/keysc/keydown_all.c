#include <gint/keyboard.h>
#include <stdarg.h>

/* keydown_all(): Check a set of keys for simultaneous input
   Returns non-zero if all provided keys are down. The list should end with an
   integer 0 as terminator. */
int keydown_all(int key, ...)
{
	va_list args;
	va_start(args, key);

	int st = 1;
	while(key && st)
	{
		st = keydown(key);
		key = va_arg(args, int);
	}

	va_end(args);
	return st;
}
