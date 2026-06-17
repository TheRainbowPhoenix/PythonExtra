//---
//	gint:mpu:spu - Sound Processing Unit and its DSPs
//---

#ifndef GINT_MPU_SPU
#define GINT_MPU_SPU

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

typedef volatile struct
{
	uint32_t PBANKC0;		/* Program Bank Control 0 */
	uint32_t PBANKC1;		/* Program Bank Control 1 */
	pad(0x8);

	uint32_t XBANKC0;		/* X Bank Control 0 */
	uint32_t XBANKC1;		/* X Bank Control 1 */
	pad(0x10);

	lword_union(SPUSRST,		/* SPU Software Reset */
		uint32_t	:24;
		uint32_t DB3	:1;	/* DMABUF 3 */
		uint32_t DB2	:1;	/* DMABUF 2 */
		uint32_t DB1	:1;	/* DMABUF 1 */
		uint32_t DB0	:1;	/* DMABUF 0 */
		uint32_t	:3;
		uint32_t RST	:1;	/* Reset */
	);
	uint32_t const SPUADR;		/* SPU address */
	uint32_t const ENDIAN;		/* SuperHyway endian */
	pad(0x10);

	uint32_t GCOM[8];		/* Global Common */
	pad(0x20);

	uint32_t const DMABUF[4];	/* Inter-DSP Communication Buffer */

} GPACKED(4) spu_t;

typedef volatile struct
{
	uint32_t SBAR;			/* Source base address */
	uint32_t SAR;			/* Source address */
	uint32_t DBAR;			/* Destination base address */
	uint32_t DAR;			/* Destination address */
	uint32_t TCR;			/* Transfer count */
	uint32_t SHPRI;			/* SHway priority */
	uint32_t CHCR;			/* Channel control */
	pad(0x4);

} GPACKED(4) spu_dsp_dma_t;

typedef volatile struct {
	uint32_t LSA;			/* Loop start address */
	uint32_t LEA;			/* Loop end address */
	pad(0x8);

} GPACKED(4) spu_dsp_loop_t;

typedef volatile struct
{
	uint32_t DSPRST;		/* DSP full reset */
	uint32_t DSPCORERST;		/* DSP core reset */
	uint32_t const DSPHOLD;		/* DSP hold */
	uint32_t DSPRESTART;		/* DSP restart */
	pad(0x8);

	uint32_t IEMASKC;		/* CPU interrupt source mask */
	uint32_t IMASKC;		/* CPU interrupt signal mask */
	uint32_t IEVENTC;		/* CPU interrupt source */
	uint32_t IEMASKD;		/* DSP interrupt source mask */
	uint32_t IMASKD;		/* DSP interrupt signal mask */
	uint32_t IESETD;		/* DSP interrupt set */
	uint32_t IECLRD;		/* DSP interrupt clear */
	uint32_t OR;			/* DMAC operation */
	uint32_t COM[8];		/* CPU-DSP communication */
	uint32_t BTADRU;		/* Bus-through address high */
	uint32_t BTADRL;		/* Bus-through address low */
	uint32_t WDATU;			/* Bus-through write data high */
	uint32_t WDATL;			/* Bus-through write data low */
	uint32_t RDATU;			/* Bus-through read data high */
	uint32_t RDATL;			/* Bus-through read data low */
	uint32_t BTCTRL;		/* Bus-through mode control */
	uint32_t SPUSTS;		/* SPU status */
	pad(0x88);

	spu_dsp_dma_t DMA[3];
	pad(0x20);
	spu_dsp_loop_t LP[3];

} GPACKED(4) spu_dsp_t;

#define SH7305_SPU  (*(spu_t *)0xfe2ffc00)
#define SH7305_DSP0 (*(spu_dsp_t *)0xfe2ffd00)
#define SH7305_DSP1 (*(spu_dsp_t *)0xfe3ffd00)

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_SPU */
