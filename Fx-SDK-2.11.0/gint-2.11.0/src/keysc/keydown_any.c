#include <gint/keyboard.h>
#include <stdarg.h>

/* keydown_any(): Check a set of keys for any input
   Returns nonzero if any one of the specified keys is currently pressed. THe
   sequence should be terminated by a 0 integer. */
int keydown_any(int key, ...)
{
	va_list args;
	va_start(args, key);

	int st = 0;
	while(key && !st)
	{
		st = keydown(key);
		key = va_arg(args, int);
	}

	va_end(args);
	return st;
}
