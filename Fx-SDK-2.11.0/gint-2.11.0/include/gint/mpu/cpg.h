//---
//	gint:mpu:cpg - Clock Pulse Generator
//---

#ifndef GINT_MPU_CPG
#define GINT_MPU_CPG

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

//---
//	SH7705 Clock Pulse Generator. Refer to:
//	  "Renesas SH7705 Group Hardware Manual"
//	  Section 9: "Interrupt Controller (INTC)"
//---

/* sh7705_cpg_t - Clock Pulse Generator registers */
typedef volatile struct
{
	word_union(FRQCR,
		uint16_t	:3;
		uint16_t CKOEN	:1;	/* Clock Output Enable */
		uint16_t	:2;
		uint16_t STC	:2;	/* PLL multiplication ratio */
		uint16_t	:2;
		uint16_t IFC	:2;	/* Internal clock divider */
		uint16_t	:2;
		uint16_t PFC	:2;	/* Peripheral clock divider */
	);

	byte_union(UCLKCR,
		uint8_t	USSCS	:2;	/* Source Clock Selection Bit */
		uint8_t USBEN	:1;	/* USB On-Chip Oscillator Enable */
		uint8_t		:5;
	);

} GPACKED(4) sh7705_cpg_t;

#define SH7705_CPG (*((sh7705_cpg_t *)0xffffff80))

//---
//	SH7305 Clock Pulse Generator. Refer to:
//	  "Renesas SH7724 User's Manual: Hardware"
//	  Section 17: "Clock Pulse Generator (CPG)"
//---

/* sh7305_cpg_t - Clock Pulse Generator registers
   Fields marked with [*] don't have the meaning described in the SH7724
   documentation. */
typedef volatile struct
{
	lword_union(FRQCR,
		uint32_t KICK	:1;	/* Flush FRQCRA modifications */
		uint32_t	:1;
		uint32_t STC	:6;	/* PLL multiplication [*] */
		uint32_t IFC	:4;	/* Iphi divider 1 [*] */
		uint32_t	:4;
		uint32_t SFC	:4;	/* Sphi divider 1 [*] */
		uint32_t BFC	:4;	/* Bphi divider 1 [*] */
		uint32_t	:4;
		uint32_t P1FC	:4;	/* Pphi divider 1 [*] */
	);
	pad(0x4);

	lword_union(FSICLKCR,
		uint32_t	:16;
		uint32_t DIVB	:6;	/* Division ratio for port B */
		uint32_t	:1;
		uint32_t CLKSTP	:1;	/* Clock Stop */
		uint32_t SRC	:2;	/* Clock source select */
		uint32_t DIVA	:6;	/* Division ratio for port A */
	);
	pad(0x04);

	lword_union(DDCLKCR,
		uint32_t	:23;
		uint32_t CLKSTP	:1;	/* Clock Stop */
		uint32_t _	:1;	/* Unknown */
		uint32_t	:1;
		uint32_t DIV	:6;
	);

	lword_union(USBCLKCR,
		uint32_t	:23;
		uint32_t CLKSTP	:1;	/* Clock Stop */
		uint32_t	:8;
	);
	pad(0x0c);

	lword_union(PLLCR,
		uint32_t	:17;
		uint32_t PLLE	:1;	/* PLL Enable */
		uint32_t	:1;
		uint32_t FLLE	:1;	/* FLL Enable */
		uint32_t	:10;
		uint32_t CKOFF	:1;	/* CKO Output Stop */
		uint32_t	:1;
	);

	uint32_t PLL2CR;
	pad(0x10);

	lword_union(SPUCLKCR,
		uint32_t	:23;
		uint32_t CLKSTP	:1;	/* Clock Stop */
		uint32_t _	:1;	/* Unknown */
		uint32_t	:1;
		uint32_t DIV	:6;	/* Division ratio */
	);
	pad(0x4);

	lword_union(SSCGCR,
		uint32_t SSEN	:1;	/* Spread Spectrum Enable */
		uint32_t	:31;
	);
	pad(0x8);

	lword_union(FLLFRQ,
		uint32_t	:16;
		uint32_t SELXM	:2;	/* FLL output division */
		uint32_t	:3;
		uint32_t FLF	:11;	/* FLL Multiplication Ratio */
	);
	pad(0x0c);

	uint32_t LSTATS;

} GPACKED(4) sh7305_cpg_t;

#define SH7305_CPG (*((sh7305_cpg_t *)0xa4150000))

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_CPG */
