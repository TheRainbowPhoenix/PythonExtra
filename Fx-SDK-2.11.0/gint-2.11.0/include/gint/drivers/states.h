//---
// gint:drivers:states - State structures for drivers
//
// The state structures in this header are exposed for introspection and driver
// debugging purposes. This is not part of the gint API, and there is *no
// stability guarantee* across minor and patch versions of gint.
//---

#ifndef GINT_DRIVERS_STATES
#define GINT_DRIVERS_STATES

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/config.h>
#include <gint/mpu/dma.h>
#include <gint/clock.h>

/* Clock Pulse Generator (see cpg/cpg.c) */
typedef struct {
	uint32_t SSCGCR;
	struct cpg_overclock_setting speed;
} cpg_state_t;

/* CPU (see cpu/cpu.c) */
typedef struct {
	uint32_t SR;
	uint32_t VBR;
	uint32_t CPUOPM;
	uint32_t rN_bank[8];
} cpu_state_t;

/* Direct Memory Access controller (see dma/dma.c) */
typedef struct {
	sh7305_dma_channel_t ch[6];
	uint16_t OR;
} dma_state_t;

/* Interrupt Controller (see intc/intc.c) */
typedef struct {
	uint16_t IPR[12];
	uint8_t MSK[13];
} intc_state_t;

/* Memory Manager Unit (see mmu/mmu.c) */
typedef struct {
	uint32_t PASCR;
	uint32_t IRMCR;
} mmu_state_t;

/* R61524 display (see r61524/r61524.c) */
typedef struct {
	/* Graphics RAM range */
	uint16_t HSA, HEA, VSA, VEA;
} r61524_state_t;

/* Real-time Clock (see rtc/rtc.c) */
typedef struct {
	uint8_t RCR1, RCR2;
} rtc_state_t;

/* Sound Processing Unit (see spu/spu.c) */
typedef struct {
	uint32_t PBANKC0, PBANKC1;
	uint32_t XBANKC0, XBANKC1;
} spu_state_t;

/* T6K11 display (see t6k11/t6k11.c) */
typedef struct {
	/* Some status bits, obtained by using the STRD command. There are other
	   parameters that cannot be read */
	uint8_t STRD;
} t6k11_state_t;

/* Timer Unit (see tmu/tmu.c) */
typedef struct {
	/* Individual timers; TSTR is used for ETMU */
	struct tmu_state_stored_timer {
		uint32_t TCOR;
		uint32_t TCNT;
		uint16_t TCR;
		uint16_t TSTR;
	} t[9];
	/* TSTR value for TMU */
	uint8_t TSTR;
} tmu_state_t;

/* USB 2.0 function module (see usb/usb.c) */
typedef struct {
	/* Control and power-up. We don't save power-related registers from other
	   modules nor UPONCR, because they must be changed to use the module */
	uint16_t SYSCFG, BUSWAIT, DVSTCTR, SOFCFG, TESTMODE, REG_C2;
	/* Interrupt configuration */
	uint16_t INTENB0, INTENB1, BRDYENB, NRDYENB, BEMPENB;
	/* Default Control Pipe */
	uint16_t DCPMAXP;

#ifdef GINT_USB_DEBUG
	/* Registers tracked read-only for state analysis and debugging */
	uint16_t SYSSTS, FRMNUM, UFRMNUM;
	uint16_t CFIFOSEL, D0FIFOSEL, D1FIFOSEL, CFIFOCTR, D0FIFOCTR, D1FIFOCTR;
	uint16_t INTSTS0, INTSTS1, BRDYSTS, NRDYSTS, BEMPSTS;
	uint16_t DCPCFG, DCPCTR;
	uint16_t USBADDR, USBREQ, USBVAL, USBINDX, USBLENG;
	uint16_t PIPESEL, PIPECFG[9], PIPEnCTR[9], PIPEBUF[9];
	/* Ignored: UPONCR, PIPEnMAXP, PIPEnPERI, PIPEnTRN, PIPEnTRE, DEVADDn */
#endif

} usb_state_t;

#ifdef __cplusplus
}
#endif

#endif /* GINT_DRIVERS_STATES */
