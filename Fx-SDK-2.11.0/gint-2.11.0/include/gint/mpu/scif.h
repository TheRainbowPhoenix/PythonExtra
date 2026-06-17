//---
// gint:mpu:scif - Serial Communication Interface with FIFO (SCIF)
//---

#ifndef GINT_MPU_SCIF
#define GINT_MPU_SCIF

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/attributes.h>
#include <gint/defs/types.h>

//---
// SH7705 SCIF (from: SH7705 manual, section 16)
//---

typedef volatile struct
{
	/* Serial Mode Register */
	word_union(SCSMR,
		uint16_t	:5;
		uint16_t SRC	:3; /* Sampling Control */
		uint16_t CA	:1; /* Communication Mode */
		uint16_t CHR	:1; /* Character Length */
		uint16_t PE	:1; /* Parity Enable */
		uint16_t OE	:1; /* Parity Mode */
		uint16_t STOP	:1; /* Stop Bit Length */
		uint16_t	:1;
		uint16_t CKS	:2; /* Clock Select */
	);
	pad(0x2);

	/* Bit Rate Register */
	uint8_t SCBRR;
	pad(0x3);

	/* Serial Control Register */
	word_union(SCSCR,
		uint16_t	:4;
		uint16_t TSIE	:1; /* Transmit Data Stop Interrupt Enable */
		uint16_t ERIE	:1; /* Receive Error Interrupt Enable */
		uint16_t BRIE	:1; /* Break Interrupt Enable */
		uint16_t DRIE	:1; /* Receive Data Ready Interrupt Enable */
		uint16_t TIE	:1; /* Transmit Interrupt Enable */
		uint16_t RIE	:1; /* Receive Interrupt Enable */
		uint16_t TE	:1; /* Transmit Enable */
		uint16_t RE	:1; /* Receive Mode */
		uint16_t	:2;
		uint16_t CKE	:2; /* Clock Enable */
	);
	pad(0x2);

	/* Transmit Data Stop Register */
	uint8_t SCTDSR;
	pad(0x3);

	/* FIFO Error Count Register */
	word_union(SCFER,
		uint16_t	:2;
		uint16_t PER	:6; /* Parity Error Count */
		uint16_t	:2;
		uint16_t FER	:6; /* Framing Error Count */
	);
	pad(0x2);

	/* Serial Status Register */
	word_union(SCSSR,
		uint16_t	:6;
		uint16_t ORER	:1; /* Overrun Error */
		uint16_t TSF	:1; /* Transmit Data Stop */
		uint16_t ER	:1; /* Receive Error */
		uint16_t TEND	:1; /* Transmit End */
		uint16_t TDFE	:1; /* Transmit FIFO Data Empty */
		uint16_t BRK	:1; /* Break Detect */
		uint16_t FER	:1; /* Framing Error */
		uint16_t PER	:1; /* Parity Error */
		uint16_t RDF	:1; /* Receive FIFO Data Full */
		uint16_t DR	:1; /* Receive Data Ready */

	);
	pad(0x2);

	/* FIFO Control Register */
	word_union(SCFCR,
		uint16_t TSE	:1; /* Transmit Data Stop Enable */
		uint16_t TCRST	:1; /* Transmit Count Reset */
		uint16_t	:3;
		uint16_t RSTRG	:3; /* RTS Output Active Trigger */
		uint16_t RTRG	:2; /* Receive FIFO Data Number Trigger */
		uint16_t TTRG	:2; /* Transmit FIFO Data Number Trigger */
		uint16_t MCE	:1; /* Modem Control Enable */
		uint16_t TFRST	:1; /* Transmit FIFO Data Register Reset */
		uint16_t RFRST	:1; /* Receive FIFO Data Register Reset */
		uint16_t LOOP	:1; /* Loopback Test */
	);
	pad(0x2);

	/* FIFO Data Count Register */
	word_union(SCFDR,
		uint16_t	:1;
		uint16_t T	:7; /* Pending bytes in transmit FIFO */
		uint16_t	:1;
		uint16_t R	:7; /* Received bytes in receive FIFO */
	);
	pad(0x2);

	/* Serial FIFO Transmit Data Register (64 bytes rolling register) */
	uint8_t SCFTDR;
	pad(0x3);

	/* Serial FIFO Receive Data Register (64 bytes rolling register) */
	uint8_t SCFRDR;

} GPACKED(4) sh7705_scif_t;

#define SH7705_SCIF (*((sh7705_scif_t *)0xa4410000))

//---
// SH7305 SCIF (from: SH7305 emulator and SH7730 SCIF)
// The module is very close to the SH7724 but it has a couple extra bits in
// SCFCR, which shows that it is closer to the SH7730 SCIF.
//---

typedef volatile struct
{
	/* Serial Mode Register */
	word_union(SCSMR,
		uint16_t	:8;
		uint16_t CA	:1; /* Communication Mode */
		uint16_t CHR	:1; /* Character Length */
		uint16_t PE	:1; /* Parity Enable */
		uint16_t OE	:1; /* Parity Mode */
		uint16_t STOP	:1; /* Stop Bit Length */
		uint16_t	:1;
		uint16_t CKS	:2; /* Clock Select */
	);
	pad(0x2);

	/* Serial Bit Rate Register */
	uint8_t SCBRR;
	pad(0x3);

	/* Serial Control Register */
	word_union(SCSCR,
		uint16_t	:8;
		uint16_t TIE	:1; /* Transmit Interrupt Enable */
		uint16_t RIE	:1; /* Receive Interrupt Enable */
		uint16_t TE	:1; /* Transmit Enable */
		uint16_t RE	:1; /* Receive Mode */
		uint16_t REIE	:1; /* Receive Error Interrupt Enable */
		uint16_t	:1;
		uint16_t CKE	:2; /* Clock Enable */
	);
	pad(0x2);

	/* Serial FIFO Transmit Data Register */
	uint8_t SCFTDR;
	pad(0x3);

	/* Serial FIFO Status Register */
	word_union(SCFSR,
		uint16_t const PERC	:4; /* Number of Parity Errors */
		uint16_t const FERC	:4; /* Number of Framing Errors */
		uint16_t ER		:1; /* Receive Error */
		uint16_t TEND		:1; /* Transmit End */
		uint16_t TDFE		:1; /* Transmit FIFO Data Empty */
		uint16_t BRK		:1; /* Break Detection */
		uint16_t const FER	:1; /* Framing Error */
		uint16_t const PER	:1; /* Parity Error */
		uint16_t RDF		:1; /* Receive FIFO Data Full */
		uint16_t DR		:1; /* Data Ready */
	);
	pad(0x2);

	/* Serial FIFO Receive Data Register */
	uint8_t SCFRDR;
	pad(0x3);

	/* Serial FIFO Control Register */
	word_union(SCFCR,
		uint16_t	:5;
		uint16_t RSTRG	:3; /* RTS Output Active Trigger */
		uint16_t RTRG	:2; /* Receive FIFO Data Trigger */
		uint16_t TTRG	:2; /* Transmit FIFO Data Trigger */
		uint16_t MCE	:1; /* Model Control Enable */
		uint16_t TFRST	:1; /* Transmit FIFO Data Register Reset */
		uint16_t RFRST	:1; /* Receive FIFO Data Register Reset */
		uint16_t LOOP	:1; /* Loopback Test */
	);
	pad(0x2);

	/* Serial FIFO Data Count Register */
	word_union(SCFDR,
		uint16_t	:3;
		uint16_t TFDC	:5; /* Number of Data Bytes in Transmit FIFO */
		uint16_t	:3;
		uint16_t RFDC	:5; /* Number of Data Bytes in Receive FIFO */
	);
	pad(0x6);

	/* Serial Line Status */
	word_union(SCLSR,
		uint16_t	:15;
		uint16_t ORER	:1; /* Overrun Error */
	);
	pad(0x2);

} GPACKED(4) sh7305_scif_t;

#define SH7305_SCIF (*((sh7305_scif_t *)0xa4410000))

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_SCIF */
