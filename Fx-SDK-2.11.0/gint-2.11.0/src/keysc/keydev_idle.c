#include <gint/drivers/keydev.h>
#include <stdint.h>
#include <stdarg.h>

/* keydev_idle(): Check if all keys are released */
bool keydev_idle(keydev_t *d, ...)
{
	uint32_t *state = (void *)d->state_queue;
	uint32_t check[3] = { state[0], state[1], state[2] };
	int key;

	va_list args;
	va_start(args, d);
	while((key = va_arg(args, int)))
	{
		int row = (key >> 4);
		int col = 0x80 >> (key & 0x7);
		((uint8_t *)check)[row] &= ~col;
	}
	va_end(args);

	return (check[0] == 0) && (check[1] == 0) && (check[2] == 0);
}
