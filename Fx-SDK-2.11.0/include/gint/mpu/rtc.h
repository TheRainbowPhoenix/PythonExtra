//---
//	gint:mpu:rtc - Real-Time Clock
//---

#ifndef GINT_MPU_RTC
#define GINT_MPU_RTC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>

//---
//	Hybrid SH7705-SH7305 Real-Time Clock. Refer to:
//	  "Renesas SH7705 Group Hardware Manual"
//	  Section 15: "Real-Time Clock (RTC)"
//	  "Renesas SH7724 User's Manual: Hardware"
//	  Section 28: "Real-Time Clock (RTC)"
//---

/* rtc_BCD2_t - a 2-digit BCD counter with a 1-byte gap */
typedef struct
{
	byte_union(,
		uint8_t TENS	:4;
		uint8_t ONES	:4;
	);
	pad(1);

} GPACKED(2) rtc_BCD2_t;

/* sh7705_rtc_t, sh7305_rtc_t - Date and time access, RTC control */
typedef volatile struct
{
	uint8_t const R64CNT;		/* A 64-Hz counter */
	pad(1);

	rtc_BCD2_t RSECCNT;		/* Second count */
	rtc_BCD2_t RMINCNT;		/* Minute count */
	rtc_BCD2_t RHRCNT;		/* Hour count */

	uint8_t RWKCNT;			/* Day of week, must be in [0..6] */
	pad(1);

	rtc_BCD2_t RDAYCNT;		/* Day count */
	rtc_BCD2_t RMONCNT;		/* Month count */

	word_union(RYRCNT,		/* Year count */
		uint THOUSANDS	:4;
		uint HUNDREDS	:4;
		uint TENS	:4;
		uint ONES	:4;
	);
	pad(12);			/* Alarm registers... */

	byte_union(RCR1,
		uint8_t CF	:1;	/* Carry flag */
		uint8_t		:2;
		uint8_t CIE	:1;	/* Carry interrupt enable */
		uint8_t AIE	:1;	/* Alarm interrupt enable */
		uint8_t		:2;
		uint8_t AF	:1;	/* Alarm flag */
	);
	pad(1);

	byte_union(RCR2,
		uint8_t PEF	:1;	/* Periodic interrupt flag */
		uint8_t PES	:3;	/* Periodic interrupt interval */
		uint8_t		:1;
		uint8_t ADJ	:1;	/* 30-second adjustment */
		uint8_t RESET	:1;	/* Reset trigger */
		uint8_t START	:1;	/* Start bit */
	);
	pad(1);

} GPACKED(4) rtc_t;

#define SH7705_RTC (*((rtc_t *)0xfffffec0))
#define SH7305_RTC (*((rtc_t *)0xa413fec0))

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_RTC */
