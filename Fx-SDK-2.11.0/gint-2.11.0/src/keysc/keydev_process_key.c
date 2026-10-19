#include <gint/drivers/keydev.h>
#include <stdbool.h>

bool keydev_queue_push(keydev_t *d, key_event_t ev);

/* keydev_process_key(): Process a new key state for events */
void keydev_process_key(keydev_t *d, int keycode, bool state)
{
	/* If the key has changed state, push an event */
	int row = (keycode >> 4);
	int col = 0x80 >> (keycode & 0x7);

	int prev = d->state_now[row] & col;
	if(state && !prev)
	{
		key_event_t ev = { 0 };
		ev.time = d->time;
		ev.type = KEYEV_DOWN;
		ev.key = keycode;
		if(keydev_queue_push(d, ev)) d->state_now[row] |= col;
	}
	else if(!state && prev)
	{
		key_event_t ev = { 0 };
		ev.time = d->time;
		ev.type = KEYEV_UP;
		ev.key = keycode;
		if(keydev_queue_push(d, ev)) d->state_now[row] &= ~col;
	}
}
