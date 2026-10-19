//---
//	gint:mpu:dma - Direct Memory Access control
//
//	The DMA is a major module on fxcg50 because it is needed to send data
//	to the display at a reasonable speed. On fx9860g, it is very rarely
//	used, if ever.
//---

#ifndef GINT_MPU_DMA
#define GINT_MPU_DMA

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

//---
//	SH7305 Direct Memory Access Controller. Refer to:
//	  "Renesas SH7724 User's Manual: Hardware"
//	  Section 16: "Direct Memory Access Controller (DMAC)"
//---

/* sh7305_dma_channel_t - One of the main 6 channels of the DMA
   Note that the many settings are only available on channels 0 to 3 (denoted
   by [0..3]) or on channels 0 and 1 (denoted by [0,1]).
   The documentation is apparently wrong about the placement is TS[3:2], the
   neighboring read-only bit must be swapped before TS[3:2]. */
typedef volatile struct
{
	uint32_t SAR;
	uint32_t DAR;

	/* Mind that the 8 upper bits should always be written as 0 */
	uint32_t TCR;

	lword_union(CHCR,
		uint32_t	:1;
		uint32_t LCKN	:1;	/* Bus Right Release Enable */
		uint32_t	:2;
		uint32_t RPT	:3;	/* Repeat setting [0..3] */
		uint32_t DA	:1;	/* DREQ Asynchronous [0,1] */

		uint32_t DO	:1;	/* DMA Overrun  [0,1] */
		uint32_t	:1;
		uint32_t TS_32	:2;	/* Transfer Size (upper half) */
		uint32_t HE	:1;	/* Half-End flag [0..3] */
		uint32_t HIE	:1;	/* Half-end Interrupt Enable [0..3] */
		uint32_t AM	:1;	/* Acknowledge mode [0,1] */
		uint32_t AL	:1;	/* Acknowledge level [0,1] */

		uint32_t DM	:2;	/* Destination address Mode */
		uint32_t SM	:2;	/* Source address Mode */
		uint32_t RS	:4;	/* Resource Select [0,1] */

		uint32_t DL	:1;	/* DREQ Level [0,1] */
		uint32_t DS	:1;	/* DREQ Source select[0,1] */
		uint32_t TB	:1;	/* Transfer Bus Mode */
		uint32_t TS_10	:2;	/* Transfer Size (lower half) */
		uint32_t IE	:1;	/* Interrupt Enable */
		uint32_t TE	:1;	/* Transfer End flag */
		uint32_t DE	:1;	/* DMA Enable */
	);

} GPACKED(4) sh7305_dma_channel_t;

/* sh7305_dma_t - DMA Controller */
typedef volatile struct
{
	sh7305_dma_channel_t DMA0;
	sh7305_dma_channel_t DMA1;
	sh7305_dma_channel_t DMA2;
	sh7305_dma_channel_t DMA3;

	word_union(OR,
		uint16_t CMS	:4;	/* Cycle steal Mode Select */
		uint16_t	:2;
		uint16_t PR	:2;	/* PRiority mode */
		uint16_t 	:5;
		uint16_t AE	:1;	/* Address Error flag */
		uint16_t NMIF	:1;	/* NMI Flag */
		uint16_t DME	:1;	/* DMA Master Enable */
	);
	pad(14);

	sh7305_dma_channel_t DMA4;
	sh7305_dma_channel_t DMA5;

} GPACKED(4) sh7305_dma_t;

#define SH7305_DMA (*((sh7305_dma_t *)0xfe008020))

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_DMA */
