//---
//	gint:intc:tmu - Timer Unit
//
//	The definitions in this file cover both the Timer Unit and the Extra
//	Timers. The structures are related but not identical; the behaviour is
//	subtly different.
//---

#ifndef GINT_MPU_TMU
#define GINT_MPU_TMU

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

//---
//	Common structures
//---

/* tmu_t - a single timer from a standard timer unit */
typedef volatile struct
{
	uint32_t TCOR;			/* Constant register */
	uint32_t TCNT;			/* Counter register, counts down */

	word_union(TCR,
		uint16_t	:7;
		uint16_t UNF	:1;	/* Underflow flag */
		uint16_t	:2;
		uint16_t UNIE	:1;	/* Underflow interrupt enable */
		uint16_t CKEG	:2;	/* Input clock edge */
		uint16_t TPSC	:3;	/* Timer prescaler (input clock) */
	);

} GPACKED(4) tmu_t;

/* etmu_t - extra timers on SH7337, SH7355 and SH7305 */
typedef volatile struct
{
	uint8_t TSTR;			/* Only bit 0 is used */
	pad(3);

	uint32_t TCOR;			/* Constant register */
	uint32_t TCNT;			/* Counter register */

	byte_union(TCR,
		uint8_t		:6;
		uint8_t UNF	:1;	/* Underflow flag */
		uint8_t UNIE	:1;	/* Underflow interrupt enable */
	);
	pad(19);

} GPACKED(4) etmu_t;

//---
//	SH7705 Timer Unit. Refer to:
//	  "Renesas SH7705 Group Hardware Manual"
//	  Section 12: "Timer Unit (TMU)"
//---

typedef volatile struct
{
	pad(2);
	uint8_t TSTR;			/* Timer Start Register */
	pad(1);

	tmu_t TMU[3];

	uint32_t TCPR2;			/* Timer Input Capture Register 2 */

} GPACKED(4) sh7705_tmu_t;

#define SH7705_TMU (*(sh7705_tmu_t *)0xfffffe90)

//---
//	SH7705 Extra Timer Unit. No official documentation exists.
//---

typedef volatile etmu_t sh7705_etmu_t[1];
#define SH7705_ETMU (*(sh7705_etmu_t *)0xa44c0030)

//---
//	SH7305 Timer Unit. Refer to:
//	  "Renesas SH7724 User's Manual: Hardware"
//	  Section 20: "Timer Unit (TMU)"
//---

typedef volatile struct
{
	uint8_t TSTR;			/* Timer Start Register */
	pad(3);

	tmu_t TMU[3];

} GPACKED(4) sh7305_tmu_t;

#define SH7305_TMU (*((sh7305_tmu_t *)0xa4490004))

//---
//	SH7305 Extra Timer Unit. No official documentation exists.
//---

typedef volatile etmu_t sh7305_etmu_t[6];
#define SH7305_ETMU (*(sh7305_etmu_t *)0xa44d0030)

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_TMU */
