//---
//	gint:mpu:intc - Interrupt Controller
//
//	The interrupt controller is unwieldy because SH7705 and SH7305 have a
//	completely different interface. Everything here is split up and you'll
//	have to explicitly handle both to be able to use the device.
//
//	gint's API provides higher-level and platform-agnostic interrupt
//	management. This is probably what you are looking for.
//---

#ifndef GINT_MPU_INTC
#define GINT_MPU_INTC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

//---
//	SH7705 Interrupt Controller. Refer to:
//	  "Renesas SH7705 Group Hardware Manual"
//	  Section 6: "Interrupt Controller (INTC)"
//---

/* sh7705_intc_ipc_t - Interrupt Priority Controller
   A set of 16-bit register that control the interrupt priorities. The SH7705's
   IPC has its registers scattered everywhere in the memory, so there is a
   pointer for each register. The SH7305 needs only one pointer for the whole
   IPC because the registers are in a contiguous area. */
typedef struct
{
	volatile word_union(*IPRA,
		uint16_t TMU0	:4;	/* Timer 0 */
		uint16_t TMU1	:4;	/* Timer 1 */
		uint16_t TMU2	:4;	/* Timer 2 */
		uint16_t RTC	:4;	/* Real-Time Clock */
	);

	volatile word_union(*IPRB,
		uint16_t WDT	:4;	/* Watchdog Timer */
		uint16_t REF	:4;	/* BSC Refresh Request, SDRAM (?) */
		uint16_t	:4;
		uint16_t	:4;
	);

	volatile word_union(*IPRC,
		uint16_t IRQ3	:4;	/* Interrupt request 3 */
		uint16_t IRQ2	:4;	/* Interrupt request 2 */
		uint16_t IRQ1	:4;	/* Interrupt request 1 */
		uint16_t IRQ0	:4;	/* Interrupt request 0 */
	);

	volatile word_union(*IPRD,
		uint16_t PINT0_7  :4;	/* External interrupt pins 0 to 7 */
		uint16_t PINT8_15 :4;	/* External interrupt pins 8 to 15 */
		uint16_t IRQ5	:4;	/* Interrupt request 5 */
		uint16_t IRQ4	:4;	/* Interrupt request 4 */
	);

	volatile word_union(*IPRE,
		uint16_t DMAC	:4;	/* Direct Memory Access Controller */
		uint16_t SCIF0	:4;	/* Serial Communication Interface 0 */
		uint16_t SCIF2	:4;	/* Serial Communication Interface 2 */
		uint16_t ADC	:4;	/* Analog/Decimal Converter */
	);

	volatile word_union(*IPRF,
		uint16_t	:4;
		uint16_t	:4;
		uint16_t USB	:4;	/* USB Controller */
		uint16_t	:4;
	);

	volatile word_union(*IPRG,
		uint16_t TPU0	:4;	/* Timer Pulse Unit 0 */
		uint16_t TPU1	:4;	/* Timer Pulse Unit 1 */
		uint16_t	:4;
		uint16_t	:4;
	);

	volatile word_union(*IPRH,
		uint16_t TPU2	:4;	/* Timer Pulse Unit 2 */
		uint16_t TPU3	:4;	/* Timer Pulse Unit 3 */
		uint16_t	:4;
		uint16_t	:4;
	);

} GPACKED(4) sh7705_intc_ipc_t;

/* sh7705_intc_icr1_t - Interrupt Control Register 1 (general) */
typedef volatile word_union(sh7705_intc_icr1_t,
	uint16_t MAI	:1;		/* Mask All Interrupts */
	uint16_t IRQLVL	:1;		/* Interrupt Request Level Detect */
	uint16_t BLMSK	:1;		/* Enable NMI when BL is set */
	uint16_t	:1;
	uint16_t IRQ5E	:2;		/* IRQ 5 Edge Detection */
	uint16_t IRQ4E	:2;		/* etc. */
	uint16_t IRQ3E	:2;
	uint16_t IRQ2E	:2;
	uint16_t IRQ1E	:2;
	uint16_t IRQ0E	:2;
);

/* sh7705_intc_t - the SH7705 interrupt controller */
typedef struct
{
	/* All interrupt priority registers */
	union {
		sh7705_intc_ipc_t	_;
		volatile uint16_t	*IPR[8];
	} GPACKED(4);

	/* Control registers */
	sh7705_intc_icr1_t		*ICR1;

} GPACKED(4) sh7705_intc_t;



//---
//	SH7305 Interrupt Controller. Refer to:
//	  "Renesas SH7724 User's Manual: Hardware"
//	  Section 13: "Interrupt Controller (INTC)"
//	Also CPU73050.dll was disassembled to find out the bits.
//---

/* sh7305_intc_ipc_t - Interrupt Priority Controller
   Some of the fields have been left unnamed because they correspond to SH7724
   peripheral modules that are *very* unlikely to even exist in the SH7305, let
   alone by of any use to us */
typedef volatile struct
{
	word_union(IPRA,
		uint16_t TMU0_0	:4;	/* TMU0 Channel 0 */
		uint16_t TMU0_1	:4;	/* TMU0 Channel 1 */
		uint16_t TMU0_2	:4;	/* TMU0 Channel 2 */
		uint16_t	:4;
	);
	pad(2);

	word_union(IPRB,
		uint16_t _	:4;	/* Unknown (TODO) */
		uint16_t _LCDC	:4;	/* SH7724: LCD Controller */
		uint16_t _DMAC1A:4;	/* SH7724: DMAC1 channels 0..3 */
		uint16_t	:4;
	);
	pad(2);

	word_union(IPRC,
		uint16_t	:4;
		uint16_t	:4;
		uint16_t	:4;
		uint16_t SPU	:4;	/* SPU's DSP0 and DSP1 */
	);
	pad(2);

	word_union(IPRD,
		uint16_t	:4;
		uint16_t _MMCIF	:4;	/* SH7724: MultiMedia Card Interface */
		uint16_t	:4;
		uint16_t	:4;
	);
	pad(2);

	word_union(IPRE,
		uint16_t DMAC0A	:4;	/* Direct Memory Access Controller 0 */
		uint16_t	:4;
		uint16_t ETMU3	:4;	/* Extra TMU 3 */
		uint16_t	:4;
	);
	pad(2);

	word_union(IPRF,
		uint16_t KEYSC	:4;	/* Key Scan Interface */
		uint16_t DMACOB	:4;	/* DMAC0 transfer/error info */
		uint16_t USB0_1	:4;	/* USB controller */
		uint16_t _CMT	:4;	/* SH7724: Compare Match Timer */
	);
	pad(2);

	word_union(IPRG,
		uint16_t _SCIF0	:4;	/* SH7724: SCIF0 transfer/error info */
		uint16_t ETMU1	:4;	/* Extra TMU 1 */
		uint16_t ETMU2	:4;	/* Extra TMU 2 */
		uint16_t	:4;
	);
	pad(2);

	word_union(IPRH,
		uint16_t _MSIOF0:4;	/* SH7724: Sync SCIF channel 0 */
		uint16_t _MSIOF1:4;	/* SH7724: Sync SCIF channel 1 */
		uint16_t _1	:4;	/* Unknown (TODO) */
		uint16_t _2	:4;	/* Unknown (TODO) */
	);
	pad(2);

	word_union(IPRI,
		uint16_t ETMU4	:4;	/* Extra TMU 4 */
		uint16_t	:4;
		uint16_t _	:4;	/* Unknown (TODO) */
		uint16_t	:4;
	);
	pad(2);

	word_union(IPRJ,
		uint16_t ETMU0	:4;	/* Extra TMU 0 */
		uint16_t _	:4;	/* Unknown (TODO) */
		uint16_t FSI	:4;	/* FIFO-Buffered Serial Interface */
		uint16_t _SDHI1	:4;	/* SH7724: SD Card Host Interface 1 */
	);
	pad(2);

	word_union(IPRK,
		uint16_t RTC	:4;	/* Real-Time Clock */
		uint16_t DMAC1B	:4;	/* DMAC1 transfer/error info */
		uint16_t	:4;
		uint16_t	:4;
	);
	pad(2);

	word_union(IPRL,
		uint16_t ETMU5	:4;	/* Extra TMU 5 */
		uint16_t _	:4;	/* Unknown (TODO) */
		uint16_t	:4;
		uint16_t	:4;
	);
	pad(2);

} GPACKED(4) sh7305_intc_ipc_t;

/* sh7305_intc_masks_t - Interrupt mask management
   Writing 1 to IMR masks interrupts; writing 1 to IMCRs clears the masks.
   Writing 0 is ignored; reading from IMCRs yields undefined values */
typedef volatile struct
{
	uint8_t IMR0;	pad(3);
	uint8_t IMR1;	pad(3);
	uint8_t IMR2;	pad(3);
	uint8_t IMR3;	pad(3);
	uint8_t IMR4;	pad(3);
	uint8_t IMR5;	pad(3);
	uint8_t IMR6;	pad(3);
	uint8_t IMR7;	pad(3);
	uint8_t IMR8;	pad(3);
	uint8_t IMR9;	pad(3);
	uint8_t IMR10;	pad(3);
	uint8_t IMR11;	pad(3);
	uint8_t IMR12;

} GPACKED(4) sh7305_intc_masks_t;


/* sh7305_intc_userimask_t - User Interrupt Mask
   Sets the minimum required level for interrupts to be accepted.

   WARNING: Writing to this register is only allowed when the upper bits of the
   operand (ie. the new value of USERIMASK) are 0xa5; otherwise, the write is
   ignored. To modify the value of this register, do not access the bit field
   directly, backup the variable and modify it:

	void set_user_imask(int new_level)
	{
		sh7305_intc_userimask_t mask = *(INTC._7305.USERIMASK);
		mask._0xa5	= 0xa5;
		mask.UIMASK	= new_level & 0x0f;
		*(INTC._7305.USERIMASK) = mask;
	}
*/
typedef volatile lword_union(sh7305_intc_userimask_t,
	uint32_t _0xa5		:8;	/* Always set to 0xa5 before writing */
	uint32_t		:16;
	uint32_t UIMASK		:4;	/* User Interrupt Mask Level */
	uint32_t		:4;
);

/* sh7305_intc_t - the SH7305 interrupt controller */
typedef struct
{
	/* Interrupt priority registers */
	union {
		sh7305_intc_ipc_t	*_;
		volatile uint16_t	*IPR;
	};

	/* Interrupt mask & mask clear registers */
	sh7305_intc_masks_t		*MSK;
	sh7305_intc_masks_t		*MSKCLR;

	/* Other registers */
	sh7305_intc_userimask_t		*USERIMASK;

} GPACKED(4) sh7305_intc_t;

//---
//	Forward definitions
//---

/* Provided by intc/intc.c */
extern sh7705_intc_t const SH7705_INTC;
extern sh7305_intc_t const SH7305_INTC;

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_INTC */
