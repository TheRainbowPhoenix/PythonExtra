//---
//	gint:keysc - The SH7305 and I/O Key Scan Interfaces
//---

#include <gint/drivers.h>
#include <gint/gint.h>
#include <gint/timer.h>
#include <gint/clock.h>
#include <gint/keyboard.h>
#include <gint/drivers/keydev.h>

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>
#include <gint/drivers/iokbd.h>
#include <gint/hardware.h>

#include <string.h>
#include <stdlib.h>

/* Keyboard input device for this Key Scan Interface */
static keydev_t keysc_dev;
/* Keyboard scanner timer */
int16_t keysc_tid = -1;

/* Keyboard scan frequency in Hertz. Start with 128 Hz, this frequency *must
   be high* for the keyboard to work! Reading at low frequencies produces a lot
   of artifacts. See https://www.casiopeia.net/forum/viewtopic.php?p=20592. */
int16_t keysc_scan_Hz = 128;
/* Approximation in microseconds, used by the timer and repeat delays */
uint32_t keysc_scan_us = 7812; /* 1000000 / keysc_scan_Hz */

/* keydev_std(): Standard keyboard input device */
keydev_t *keydev_std(void)
{
	return &keysc_dev;
}
__attribute__((alias("keydev_std")))
keydev_t *_WEAK_keydev_std(void);

/* keysc_scan(): Scand the keyboard */
static void keysc_scan(uint8_t *scan)
{
	if(isSH3())
	{
		iokbd_scan(scan);
		return;
	}

	volatile uint16_t *KEYSC = (void *)0xa44b0000;

	for(int i = 0; i < 6; i++) {
		int data = KEYSC[i];
		scan[2*i] = data & 0xff;
		scan[2*i+1] = data >> 8;
	}
}

/* keysc_tick(): Update the keyboard to the next state */
int keysc_tick(void)
{
	uint8_t scan[12] = { 0 };
	keysc_scan(scan);

	keydev_process_state(&keysc_dev, scan);
	keydev_tick(&keysc_dev, keysc_scan_us);

	/* Freeze abort key combo: SHIFT+7+3+AC/ON */
	if(keydown(KEY_SHIFT) && keydown(KEY_7) && keydown(KEY_3) &&
		keydown(KEY_ACON))
	{
		abort();
	}

	return TIMER_CONTINUE;
}

/* pollevent() - poll the next keyboard event */
key_event_t pollevent(void)
{
	return keydev_unqueue_event(&keysc_dev);
}

/* waitevent() - wait for the next keyboard event */
key_event_t waitevent(volatile int *timeout)
{
	while(1)
	{
		key_event_t ev = pollevent();
		if(ev.type != KEYEV_NONE) return ev;

		if(timeout && *timeout) break;
		sleep();
	}

	key_event_t ev = { .type = KEYEV_NONE, .time = keysc_dev.time };
	return ev;
}

/* clearevents(): Read all events waiting in the queue */
void clearevents(void)
{
	while(pollevent().type != KEYEV_NONE);
}

void cleareventflips(void)
{
	keydev_clear_flips(&keysc_dev);
}

int keydown(int key)
{
	return keydev_keydown(&keysc_dev, key);
}

int keypressed(int key)
{
	return keydev_keypressed(&keysc_dev, key);
}

int keyreleased(int key)
{
	return keydev_keyreleased(&keysc_dev, key);
}

//---
// Driver initialization
//---

static void configure(void)
{
	keydev_init(&keysc_dev);

	/* Do a first scan to load the initial state (so that keys that are
           pressed at startup are not registered as new presses) */
	keysc_scan(keysc_dev.state_now);
	memcpy(keysc_dev.state_queue, keysc_dev.state_now, 12);

	/* Set the default repeat to 400/40 ms */
	keydev_t *d = keydev_std();
	keydev_set_transform(d, (keydev_transform_t){ KEYDEV_TR_REPEATS, NULL });
	keydev_set_standard_repeats(d, 400 * 1000, 40 * 1000);

	/* The timer will be stopped when the timer driver is unloaded */
	keysc_tid = timer_configure(TIMER_ANY, keysc_scan_us,
		GINT_CALL(keysc_tick));
	if(keysc_tid >= 0) timer_start(keysc_tid);

	gint[HWKBD] = HW_LOADED | (isSH3() ? HWKBD_IO : HWKBD_KSI);
}

//---
// State and driver metadata
//---

gint_driver_t drv_keysc = {
	.name       = "KEYSC",
	.configure  = configure,
};
GINT_DECLARE_DRIVER(23, drv_keysc);
