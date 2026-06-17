#include <gint/rtc.h>

uint32_t rtc_ticks(void)
{
	rtc_time_t t;
	rtc_get_time(&t);

	uint32_t ticks = t.hours;
	ticks =  60 * ticks + t.minutes;
	ticks =  60 * ticks + t.seconds;
	ticks = 128 * ticks + t.ticks;

	return ticks;
}
