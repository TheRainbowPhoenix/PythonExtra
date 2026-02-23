//---
//	gint:rtc - Real-Time Clock
//---

#include <gint/rtc.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/intc.h>

#include <gint/defs/types.h>
#include <gint/hardware.h>
#include <gint/mpu/rtc.h>

#include <stdarg.h>

/* RTC address on SH7305, adjusted at startup on SH7337 and SH7355 */
static rtc_t *RTC = &SH7305_RTC;


//---
//	Time management
//---

/* int8(), int16(): Convert BCD to integer */
static int int8(uint8_t bcd)
{
	return (bcd & 0x0f) + 10 * (bcd >> 4);
}
static int int16(uint16_t bcd)
{
	return (bcd & 0xf) + 10 * ((bcd >> 4) & 0xf) + 100 * ((bcd >> 8) & 0xf)
		+ 1000 * (bcd >> 12);
}

/* bcd8(), bcd16(): Convert integer to BCD
   TODO: Use some kind of qdiv() for bcd8() and bcd16() */
static uint8_t bcd8(int integer)
{
	integer %= 100;
	return ((integer / 10) << 4) | (integer % 10);
}
static uint16_t bcd16(int integer)
{
	integer %= 10000;
	return (bcd8(integer / 100) << 8) | bcd8(integer % 100);
}

/* rtc_get_time(): Read the current time from the RTC */
void rtc_get_time(rtc_time_t *time)
{
	do {
		RTC->RCR1.CF = 0;

		time->ticks     = RTC->R64CNT;
		time->seconds   = int8(RTC->RSECCNT.byte);
		time->minutes   = int8(RTC->RMINCNT.byte);
		time->hours     = int8(RTC->RHRCNT.byte);
		time->month_day = int8(RTC->RDAYCNT.byte);
		time->month     = int8(RTC->RMONCNT.byte);
		time->year      = int16(RTC->RYRCNT.word);
		time->week_day  = RTC->RWKCNT;

	} while(RTC->RCR1.CF != 0);
}

/* rtc_set_time(): Set current time in the RTC */
void rtc_set_time(rtc_time_t const *time)
{
	int wday = time->week_day;
	if(wday >= 7) wday = 0;

	do {
		RTC->RCR1.CF = 0;

		RTC->RSECCNT.byte = bcd8(time->seconds);
		RTC->RMINCNT.byte = bcd8(time->minutes);
		RTC->RHRCNT.byte  = bcd8(time->hours);
		RTC->RDAYCNT.byte = bcd8(time->month_day);
		RTC->RMONCNT.byte = bcd8(time->month);
		RTC->RYRCNT.word  = bcd16(time->year);
		RTC->RWKCNT       = wday;

	} while(RTC->RCR1.CF != 0);
}

//---
//	RTC periodic interrupt
//---

static gint_call_t rtc_periodic_callback;

bool rtc_periodic_enable(int frequency, gint_call_t callback)
{
	/* Refuse to override an existing interrupt */
	if(RTC->RCR2.PES != RTC_NONE) return false;
	if(frequency == RTC_NONE)
	{
		rtc_periodic_disable();
		return true;
	}

	/* Temporarily disable the interrupt and set up the callback */
	RTC->RCR2.PES = RTC_NONE;
	rtc_periodic_callback = callback;

	/* Clear the interrupt flag */
	do RTC->RCR2.PEF = 0;
	while(RTC->RCR2.PEF);

	/* Enable the interrupt */
	RTC->RCR2.PES = frequency;
	return true;
}

void rtc_periodic_interrupt(void)
{
	int rc = gint_call(rtc_periodic_callback);

	/* Clear the interrupt flag */
	do RTC->RCR2.PEF = 0;
	while(RTC->RCR2.PEF);

	/* Stop the interrupt if the callback returns non-zero */
	if(rc) rtc_periodic_disable();
}

void rtc_periodic_disable(void)
{
	RTC->RCR2.PES = RTC_NONE;
}

/* Deprecated versions */
#undef rtc_start_timer
int rtc_start_timer(int frequency, timer_callback_t function, ...)
{
	va_list args;
	va_start(args, function);
	uint32_t arg = va_arg(args, uint32_t);
	va_end(args);

	return !rtc_periodic_enable(frequency, GINT_CALL(function.v, arg));
}
void rtc_stop_timer(void)
{
	return rtc_periodic_disable();
}

//---
//	Driver initialization
//---

static void constructor(void)
{
	/* Adjust the address of the RTC */
	if(isSH3()) RTC = &SH7705_RTC;
}

static void configure(void)
{
	/* Disable the carry and alarm interrupts (they share their IPR bits
	   with the periodic interrupt, which we want to enable) */
	RTC->RCR1.byte = 0;
	/* Clear the periodic interrupt flag */
	RTC->RCR2.PEF = 0;

	/* Install the RTC interrupt handler */
	intc_handler_function(0xaa0, GINT_CALL(rtc_periodic_interrupt));

	/* Disable the RTC interrupts for now. Give them priority 1; higher
	   priorities cause freezes when going back to the system on SH3
	   (TODO: Find out about the RTC interrupt problem on SH3) */
	RTC->RCR2.PES = RTC_NONE;
	intc_priority(INTC_RTC_PRI, 1);
}

//---
// State and driver metadata
//---

static void hsave(rtc_state_t *s)
{
	s->RCR1 = RTC->RCR1.byte;
	s->RCR2 = RTC->RCR2.byte;
}

static void hrestore(rtc_state_t const *s)
{
	RTC->RCR1.byte = s->RCR1 & 0x18;
	RTC->RCR2.byte = s->RCR2 & 0x7f;
}

gint_driver_t drv_rtc = {
	.name         = "RTC",
	.constructor  = constructor,
	.configure    = configure,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(rtc_state_t),
};
GINT_DECLARE_DRIVER(13, drv_rtc);
