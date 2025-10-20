//---
//	gint:mpu:pfc - Pin Function Controller
//
//	The Pin Function Controller has a simple register interface, the main
//	difficulty is still understanding the role of its pins.
//---

#ifndef GINT_MPU_PFC
#define GINT_MPU_PFC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

//---
//	SH7705 Pin Function Controller. Refer to:
//	  "Renesas SH7705 Group Hardware Manual"
//	  Section 19: "Pin Function Controller"
//---

typedef volatile struct
{
	/* Control registers */
	uint16_t PACR;
	uint16_t PBCR;
	uint16_t PCCR;
	uint16_t PDCR;
	uint16_t PECR;
	uint16_t PFCR;
	uint16_t PGCR;
	uint16_t PHCR;
	uint16_t PJCR;
	uint16_t PKCR;
	uint16_t PLCR;
	uint16_t SCPCR;		/* Port SC control register */
	uint16_t PMCR;
	uint16_t PNCR;

	pad(4);

	/* Data registers */
	uint8_t PADR;
	pad(1);
	uint8_t PBDR;
	pad(1);
	uint8_t PCDR;
	pad(1);
	uint8_t PDDR;
	pad(1);
	uint8_t PEDR;
	pad(1);
	uint8_t PFDR;
	pad(1);
	uint8_t PGDR;
	pad(1);
	uint8_t PHDR;
	pad(1);
	uint8_t PJDR;
	pad(1);
	uint8_t PKDR;
	pad(1);
	uint8_t PLDR;
	pad(1);
	uint8_t SCPDR;		/* Port SC data register */
	pad(1);
	uint8_t PMDR;
	pad(1);
	uint8_t PNDR;
	pad(1);

} GPACKED(4) sh7705_pfc_t;

#define SH7705_PFC (*((sh7705_pfc_t *)0xa4000100))

//---
//  SH7305 Pin Function Controller.
//  This is somewhat reminiscent of the SH7724, but there are many differences.
//  The official emulator is the basis for this module.
//---

/* All port control registers (PCRx) have 2-bit entries specifying 4 pin modes:
     00: Special functions
     01: Output
     10: Input (pull-up ON)
     11: Input (pull-up OFF)

   Some pins do not accept all settings. The SH7724 has these exceptions:
   - PGCR (all pins): Input now allowed (00 and 01 only)
   - PJCR.P5MD/P6MD/P7MD: Input not allowed (00 and 01 only)

   Also, some entries are not configurable or are not mapped to actual pins.
   Again the SH7724 has these exceptions:
   - PGCR.P6MD/P7MD: No setting
   - PJCR.P4MD: No setting
   - PSCR.P7MD: No setting */
typedef volatile word_union(sh7305_pfc_control_t,
	uint16_t P7MD	:2;
	uint16_t P6MD	:2;
	uint16_t P5MD	:2;
	uint16_t P4MD	:2;
	uint16_t P3MD	:2;
	uint16_t P2MD	:2;
	uint16_t P1MD	:2;
	uint16_t P0MD	:2;
);

/* Data register; a plain 8-bit value. Each bit can be either read or written
   depending on the mode of the corresponding pin. */
typedef volatile byte_union(sh7305_pfc_data_t,
	uint8_t P7DT	:1;
	uint8_t P6DT	:1;
	uint8_t P5DT	:1;
	uint8_t P4DT	:1;
	uint8_t P3DT	:1;
	uint8_t P2DT	:1;
	uint8_t P1DT	:1;
	uint8_t P0DT	:1;
);

typedef volatile struct
{
	sh7305_pfc_control_t
		PACR, PBCR, PCCR, PDCR, PECR, PFCR, PGCR, PHCR,
		PJCR, PKCR, PLCR, PMCR, PNCR, PQCR, PRCR, PSCR;

	sh7305_pfc_data_t PADR;
	pad(1);
	sh7305_pfc_data_t PBDR;
	pad(1);
	sh7305_pfc_data_t PCDR;
	pad(1);
	sh7305_pfc_data_t PDDR;
	pad(1);
	sh7305_pfc_data_t PEDR;
	pad(1);
	sh7305_pfc_data_t PFDR;
	pad(1);
	sh7305_pfc_data_t PGDR;
	pad(1);
	sh7305_pfc_data_t PHDR;
	pad(1);
	sh7305_pfc_data_t PJDR;
	pad(1);
	sh7305_pfc_data_t PKDR;
	pad(1);
	sh7305_pfc_data_t PLDR;
	pad(1);
	sh7305_pfc_data_t PMDR;
	pad(1);
	sh7305_pfc_data_t PNDR;
	pad(1);
	sh7305_pfc_data_t PQDR;
	pad(1);
	sh7305_pfc_data_t PRDR;
	pad(1);
	sh7305_pfc_data_t PSDR;
	pad(1);

	sh7305_pfc_control_t PTCR;
	sh7305_pfc_control_t PUCR;
	sh7305_pfc_control_t PVCR;
	pad(6);
	sh7305_pfc_control_t PPCR;

	/* PSEM*:  Multiplexing pin settings. These are highly MPU-dependent and
	           the assignment is basically unknown.
	   HIZCR*: High-impedance settings for pins' I/O buffers. */
	uint16_t PSELA;
	uint16_t PSELB;
	uint16_t PSELC;
	uint16_t PSELD;
	uint16_t PSELE;
	uint16_t HIZCRA;
	uint16_t HIZCRB;
	uint16_t HIZCRC;
	uint16_t PSELF;

	sh7305_pfc_data_t PTDR;
	pad(1);
	sh7305_pfc_data_t PUDR;
	pad(1);
	sh7305_pfc_data_t PVDR;
	pad(5);
	sh7305_pfc_data_t PPDR;
	pad(0x15);

	/* Module function selection registers.
	   WARNING: These are the SH7724 bits, not necessarily the SH7305! */
	word_union(MSELCRA,
		uint16_t		:8;
		uint16_t UNKNOWN_USB	:2;
		uint16_t		:6;
	);
	word_union(MSELCRB,
		uint16_t XTAL_USB	:2;
		uint16_t		:6;
		uint16_t SCIF2_PORT	:1;
		uint16_t		:1;
		uint16_t SCIF3_PORT	:1;
		uint16_t		:3;
		uint16_t LDC_VSYNC_DIR	:1;
		uint16_t		:1;
	);

	// TODO: Doc
	uint16_t DRVCRD, DRVCRA, DRVCRB, DRVCRC;
	pad(4);

	/* Pull-up control registers. Not sure what these are since that's
	   normally handled in the modes for P*CR? */
	uint8_t PULCRA, PULCRB, PULCRC, PULCRD, PULCRE, PULCRF, PULCRG,
		PULCRH, PULCRJ, PULCRK, PULCRL, PULCRM, PULCRN, PULCRQ,
		PULCRR, PULCRS;
	pad(0x20);
	uint8_t PULCRT;
	uint8_t PULCRU;
	uint8_t PULCRV;
	uint8_t PULCRBSC;
	pad(1);
	uint8_t PULCRTRST;
	uint8_t PULCRP;
	pad(1);

	uint16_t PSELG;
	pad(12);
	uint16_t PSELH;

} sh7305_pfc_t;

#define SH7305_PFC (*((sh7305_pfc_t *)0xa4050100))

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_PFC */
