//---
// gint:mpu:wdt - Watchdog Timer
//---

#ifndef GINT_MPU_WDT
#define GINT_MPU_WDT

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

//---
//	SH7705 WatchDog Timer. Refer to:
//	  "Renesas SH7705 Group Hardware Manual"
//	  Section 10: "WatchDog Timer (WDT)"
//---

/* sh7705_wdt_t - Watch Dog Timer */
typedef volatile struct
{
	/* WDT registers are unique in access size; reads are performed with
	   8-bit accesses, but writes are performed with 16-bit accesses. */

	union {
		uint8_t READ;
		uint16_t WRITE;
	} WTCNT;

	union {
		byte_union(READ,
			uint8_t TME	:1; /* Timer Enable */
			uint8_t WTIT	:1; /* Watchdog/Interval Select */
			uint8_t RSTS	:1; /* Watchdog Reset Select */
			uint8_t WOVF	:1; /* Watchdog Overflow Flag */
			uint8_t IOVF	:1; /* Interval Overflow Flag */
			uint8_t CKS	:3; /* Clock Select */
		);
		uint16_t WRITE;
	} WTCSR;

} sh7705_wdt_t;

#define SH7705_WDT (*((sh7705_wdt_t *)0xffffff84))

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_WDT */
