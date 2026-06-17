//---
//	gint:clock:sleep - Various low-level sleep functions
//---

#include <gint/clock.h>
#include <gint/timer.h>

static void do_sleep(uint64_t delay_us, int spin)
{
	volatile int flag = 0;

	int timer = timer_configure(TIMER_ANY, delay_us,
		GINT_CALL_SET_STOP(&flag));
	if(timer < 0) return;

	if(spin) {
		timer_spinwait(timer);
		timer_stop(timer);
	}
	else {
		timer_start(timer);
		timer_wait(timer);
	}
}

/* sleep_us(): Sleep for a fixed duration in microseconds */
void sleep_us(uint64_t delay_us)
{
	do_sleep(delay_us, 0);
}

/* sleep_us_spin(): Actively sleep for a fixed duration in microseconds */
void sleep_us_spin(uint64_t delay_us)
{
	do_sleep(delay_us, 1);
}
