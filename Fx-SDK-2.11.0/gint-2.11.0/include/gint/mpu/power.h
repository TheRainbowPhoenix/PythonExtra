//---
//	gint:mpu:power- Standby modes and power management
//---

#ifndef GINT_MPU_POWER
#define GINT_MPU_POWER

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

//---
//	SH7305 Standby modes and power management Refer to:
//	  "Renesas SH7724 User's Manual: Hardware"
//	  Section 18: "Reset and Power-Down Modes"
//---

typedef volatile struct
{
	/* Standby Control Register: only one of the bits may be set at any
	   given time. Setting several bits is prohibited! */
	lword_union(STBCR,
		uint32_t	:24;
		uint32_t STBY	:1;	/* Map [sleep] to standby mode */
		uint32_t	:1;
		uint32_t RSTBY	:1;	/* Map [sleep] to R-standby mode */
		uint32_t USTBY	:1;	/* Map [sleep] to U-standby mode */
		uint32_t	:4;
	);
	pad(12);

	/* Module Stop Control Register 0
	   Stopping the TLB (bit 31), IC (bit 30 or OC (bit 29) seems somewhat
	   dangerous, plus requires special operations. */
	lword_union(MSTPCR0,
		uint32_t TLB		:1;
		uint32_t IC		:1;
		uint32_t OC		:1;
		uint32_t RS		:1;
		uint32_t IL		:1;
		uint32_t SndCache	:1;
		uint32_t _unknown1	:1;
		uint32_t		:1;

		uint32_t		:1;
		uint32_t INTC		:1;
		uint32_t DMAC0		:1;
		uint32_t SuperHyway	:1;
		uint32_t HUDI		:1;
		uint32_t DBG		:1;
		uint32_t UDB		:1;
		uint32_t Debug		:1;

		uint32_t TMU0		:1;
		uint32_t CMT		:1;
		uint32_t RWDT		:1;
		uint32_t DMAC1		:1;
		uint32_t 		:4;

		uint32_t SCIF		:1;
		uint32_t KEYSC		:1;
		uint32_t RTC		:1;
		uint32_t		:2;
		uint32_t MSIOF0		:1;
		uint32_t MSIOF1		:1;
		uint32_t		:1;
	);
	pad(4);

	/* Module Stop Control Register 2
	   The list was established by Yatis. See <https://bible.planet-casio.
	   com/yatis/hardware/sh7305/power.html>. */
	lword_union(MSTPCR2,
		uint32_t		:2;
		uint32_t MMC		:1;
		uint32_t		:1;
		uint32_t ADC		:1;
		uint32_t		:6;
		uint32_t USB0		:1;
		uint32_t		:2;
		uint32_t SDC		:1;
		uint32_t		:2;
		uint32_t FLCTL		:1;
		uint32_t ECC		:1;
		uint32_t		:1;
		uint32_t I2C		:1;
		uint32_t		:1;
		uint32_t FSI_SPU	:1;
		uint32_t _unknown2	:1;
		uint32_t		:1;
		uint32_t LCDC		:1;
		uint32_t _unknown3	:1;
		uint32_t Cmod		:1;
		uint32_t		:1;
		uint32_t Cmod2A		:1;
		uint32_t		:2;
	);
	pad(4);

	/* Boot Address Register
	   I really don't suggest writing to BAR. */
	uint32_t const BAR;

} sh7305_power_t;

#define SH7305_POWER (*((sh7305_power_t *)0xa4150020))

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_POWER */
