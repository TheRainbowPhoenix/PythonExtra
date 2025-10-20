#include <gint/keyboard.h>
#include <gint/timer.h>

extern int keysc_scan_Hz;
extern uint32_t keysc_scan_us;
extern int keysc_tid;

/* keysc_scan_frequency(): Get the current keyboard scan frequency in Hertz */
int keysc_scan_frequency(void)
{
	return keysc_scan_Hz;
}

/* keysc_scan_frequency_us(): Get keyboard scan delay in microseconds */
uint32_t keysc_scan_frequency_us(void)
{
	return keysc_scan_us;
}

/* keysc_set_scan_frequency(): Set the keyboard scan frequency in Hertz */
void keysc_set_scan_frequency(int freq)
{
	if(freq < 64) freq = 64;
	if(freq > 32768) freq = 32768;
	keysc_scan_Hz = freq;
	keysc_scan_us = 1000000 / freq;

	if(keysc_tid < 0) return;
	uint32_t TCOR = timer_delay(keysc_tid, keysc_scan_us, 0);
	timer_reload(keysc_tid, TCOR);
}
