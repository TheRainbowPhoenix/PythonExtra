//---
//	gint:intc:usb - USB 2.0 Interface
//---

#ifndef GINT_MPU_USB
#define GINT_MPU_USB

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>

typedef struct
{
	word_union(TRE,
		uint16_t	:6;
		uint16_t TRENB	:1;	/* Transaction Counter Enable */
		uint16_t TRCLR	:1;	/* Transaction Counter Clear */
		uint16_t	:8;
	);
	word_union(TRN,
		uint16_t TRCNT	:16;	/* Transaction Counter */
	);
} sh7305_usb_pipetr;

typedef volatile struct
{
	/* System Configuration Control Register */
	word_union(SYSCFG,
		uint16_t	:5;
		uint16_t SCKE	:1;	/* USB Module Clock Enabled */
		uint16_t	:2;
		uint16_t HSE	:1;	/* High Speed Enable */
		uint16_t DCFM	:1;	/* Controller Function Select */
		uint16_t DRPD	:1;	/* D+/D- Line Resistor Control */
		uint16_t DPRPU	:1;	/* D+ Line Resistor Control */
		uint16_t	:3;
		uint16_t USBE	:1;	/* USB Module Enable */
	);

	/* CPU Bus Wait Setting Register */
	word_union(BUSWAIT,
		uint16_t	:8;
		uint16_t _1	:1;	/* Unknown role; can be set */
		uint16_t	:1;
		uint16_t _2	:1;	/* Unknown role; can be set */
		uint16_t	:1;
		uint16_t BWAIT	:4;	/* Bus Wait */
	);

	/* System Configuration Status Register */
	const word_union(SYSSTS,
		uint16_t	:14;
		uint16_t LNST	:2;	/* Line Status */
	);
	pad(2);

	/* Device State Control Register */
	word_union(DVSTCTR,
		uint16_t _1	:1;	/* Unknown role; can be set */
		uint16_t	:2;
		uint16_t _2	:4;	/* Unknown role; can be set */
		uint16_t WKUP	:1;	/* Wakeup Output */
		uint16_t RWUPE	:1;	/* Wakeup Detection Enable */
		uint16_t USBRT	:1;	/* USB Reset Output */
		uint16_t RESUME	:1;	/* Resume Output */
		uint16_t UACT	:1;	/* USB Bus Enable */
		uint16_t	:1;
		uint16_t RHST	:3;	/* Reset Handshake */
	);
	pad(2);

	/* Test Mode Register */
	word_union(TESTMODE,
		uint16_t	:12;
		uint16_t UTST	:4;	/* Test Mode */
	);
	pad(6);

	/* FIFO Port Registers */
	uint32_t CFIFO;			/* DCP FIFO port */
	uint32_t D0FIFO;		/* Data 0 FIFO port */
	uint32_t D1FIFO;		/* Data 1 FIFO port */

	/* FIFO Port Select and Control Registers */
	word_union(CFIFOSEL,
		uint16_t RCNT	:1;	/* Read Count Mode */
		uint16_t REW	:1;	/* Pointer Buffer Rewind */
		uint16_t	:2;
		uint16_t MBW	:2;	/* Access Bits Width */
		uint16_t	:1;
		uint16_t BIGEND	:1;	/* Endiant Control */
		uint16_t	:2;
		uint16_t ISEL	:1;	/* Access Direction When DCP is used */
		uint16_t	:1;
		uint16_t CURPIPE:4;	/* Port Access Pipe Specification */
	);
	word_union(CFIFOCTR,
		uint16_t BVAL	:1;	/* Buffer Memory Valid Flags */
		uint16_t BCLR	:1;	/* CPU Buffer Clear */
		uint16_t FRDY	:1;	/* FIFO Port Ready */
		uint16_t	:1;
		uint16_t DTLN	:12;	/* Receive Data Length */
	);
	pad(4);
	word_union(D0FIFOSEL,
		uint16_t RCNT	:1;	/* Read Count Mode */
		uint16_t REW	:1;	/* Pointer Buffer Rewind */
		uint16_t DCLRM	:1;	/* Auto Buffer Clear Mode */
		uint16_t DREQE	:1;	/* DMA Transfert Request Enable */
		uint16_t MBW	:2;	/* Access Bits Width */
		uint16_t	:1;
		uint16_t BIGEND	:1;	/* Endian Control */
		uint16_t _1	:1;	/* Unknown role; can be set */
		uint16_t	:3;
		uint16_t CURPIPE:4;	/* Port Access Pipe Specification */
	);
	word_union(D0FIFOCTR,
		uint16_t BVAL	:1;	/* Buffer Memory Valid Flags */
		uint16_t BCLR	:1;	/* CPU Buffer Clear */
		uint16_t FRDY	:1;	/* FIFO Port Ready */
		uint16_t	:1;
		uint16_t DTLN	:12;	/* Receive Data Length */
	);
	word_union(D1FIFOSEL,
		uint16_t RCNT	:1;	/* Read Count Mode */
		uint16_t REW	:1;	/* Pointer Buffer Rewind */
		uint16_t DCLRM	:1;	/* Auto Buffer Clear Mode */
		uint16_t DREQE	:1;	/* DMA Transfert Request Enable */
		uint16_t MBW	:2;	/* Access Bits Width */
		uint16_t	:1;
		uint16_t BIGEND	:1;	/* Endian Control */
		uint16_t _1	:1;	/* Unknown role; can be set */
		uint16_t	:3;
		uint16_t CURPIPE:4;	/* Port Access Pipe Specification */
	);
	word_union(D1FIFOCTR,
		uint16_t BVAL	:1;	/* Buffer Memory Valid Flags */
		uint16_t BCLR	:1;	/* CPU Buffer Clear */
		uint16_t FRDY	:1;	/* FIFO Port Ready */
		uint16_t	:1;
		uint16_t DTLN	:12;	/* Receive Data Length */
	);

	/* Interrupts Enable Registers */
	word_union(INTENB0,
		uint16_t VBSE	:1;	/* VBUS */
		uint16_t RSME	:1;	/* Resume */
		uint16_t SOFE	:1;	/* Frame Number Update */
		uint16_t DVSE	:1;	/* Device State Transition */
		uint16_t CTRE	:1;	/* Control Transfer Stage Transition */
		uint16_t BEMPE	:1;	/* Buffer Empty */
		uint16_t NRDYE	:1;	/* Buffer Not Ready */
		uint16_t BRDYE	:1;	/* Buffer Ready */
		uint16_t	:8;
	);
	word_union(INTENB1,
		uint16_t _1	:1;	/* Unknown role; can be set */
		uint16_t BCHGE	:1;	/* Bus Change */
		uint16_t	:1;
		uint16_t DTCHE	:1;	/* Disconnection Detection */
		uint16_t ATTCHE	:1;	/* Connection Detection */
		uint16_t	:4;
		uint16_t EOFERRE:1;	/* EOF Error Detection */
		uint16_t SIGNE	:1;	/* Setup Transaction Error */
		uint16_t SACKE	:1;	/* Setup Transaction Normal Response */
		uint16_t	:4;
	);
	pad(2);

	/* BRDY Interrupt Enable Register */
	uint16_t BRDYENB;
	/* NRDY Interrupt Enable Register */
	uint16_t NRDYENB;
	/* BEMP Interrupt Enable Register */
	uint16_t BEMPENB;

	/* SOF Control Register */
	word_union(SOFCFG,
		uint16_t	:7;
		uint16_t TRNENSEL :1;	/* Transaction-Enabled Time Select */
		uint16_t	:1;
		uint16_t BRDYM	:1;	/* BRDY Status Clear Timing */
		uint16_t enable	:1; 	/* SHOULD BE SET TO 1 MANUALLY */
		uint16_t	:1;
		uint16_t _1	:1;	/* Unknown role; can be set */
		uint16_t _2	:1;	/* Unknown role; can be set */
		uint16_t	:2;
	);
	pad(2);

	/* Interrupt Status Registers */
	word_union(INTSTS0,
		uint16_t VBINT	:1;	/* VBUS */
		uint16_t RESM	:1;	/* Resume */
		uint16_t SOFR	:1;	/* Frame Number Refresh */
		uint16_t DVST	:1;	/* Device State Transition */
		uint16_t CTRT	:1;	/* Control Transfer Stage Transition */
		uint16_t BEMP	:1;	/* Buffer Empty */
		uint16_t NRDY	:1;	/* Buffer Not Ready */
		uint16_t BRDY	:1;	/* Buffer Ready */
		uint16_t VBSTS	:1;	/* VBUS Input Status */
		uint16_t DVSQ	:3;	/* Device state */
		uint16_t VALID	:1;	/* USB Request Reception */
		uint16_t CTSQ	:3;	/* Control Transfer Stage */
	);
	word_union(INTSTS1,
		uint16_t _1	:1;	/* Unknown role */
		uint16_t BCHG	:1;	/* Bus Change */
		uint16_t	:1;
		uint16_t DTCH	:1;	/* Disconnection Detection */
		uint16_t ATTCH	:1;	/* Connection Detection */
		uint16_t	:4;
		uint16_t EOFERR	:1;	/* EOF Error Detection */
		uint16_t SIGN	:1;	/* Setup Transaction Error */
		uint16_t SACK	:1;	/* Setup Transaction Normal Response */
		uint16_t	:4;
	);
	pad(2);

	/* BRDY Interrupt Status Register */
	uint16_t BRDYSTS;
	/* NRDY Interrupt Status Register */
	uint16_t NRDYSTS;
	/* BEMP Interrupt Status Register */
	uint16_t BEMPSTS;

	/* Frame Number Registers */
	word_union(FRMNUM,
		uint16_t OVRN	:1;	/* Overrun/Underrun Detection Status */
		uint16_t CRCE	:1;	/* Receive Data Error */
		uint16_t const	:3;
		uint16_t FRNM	:11;	/* Frame Number */
	);
	word_union(UFRMNUM,
		uint16_t	:13;
		uint16_t UFRNM	:3;	/* uFrame */
	);

	/* USB Address Register */
	word_union(USBADDR,
		uint16_t	:9;
		uint16_t const USBADDR :7;	/* USB Address */
	);
	pad(2);

	/* USB Request Type Register */
	word_union(USBREQ,
		uint16_t BREQUEST  :8;	/* USB request data value */
		uint16_t BMREQUEST :8;	/* USB request type value */
	);
	/* USB Request Value Register */
	word_union(USBVAL,
		uint16_t WVALUE	:16;	/* USB request wValue value */
	);
	/* USB Request Index Register */
	word_union(USBINDX,
		uint16_t WINDEX	:16;	/* USB USB request wIndex value */
	);
	/* USB Request Length Register */
	word_union(USBLENG,
		uint16_t WLENGTH :16;	/* USB USB request wLength value */
	);

	/* DCP Configuration Register */
	word_union(DCPCFG,
		uint16_t	:7;
		uint16_t _1	:1;	/* Unknown role; can be set */
		uint16_t _2	:1;	/* Unknown role; can be set */
		uint16_t	:2;
		uint16_t DIR	:1;	/* Transfer Direction */
		uint16_t	:4;
	);
	/* DCP Maximum Packet Size Register */
	word_union(DCPMAXP,
		uint16_t DEVSEL	:4;	/* Device Select */
		uint16_t	:5;
		uint16_t MXPS	:7;	/* Maximum Packet Size */
	);
	/* DCP Control Register */
	word_union(DCPCTR,
		uint16_t BSTS	:1;	/* Buffer Status */
		uint16_t SUREQ	:1;	/* SETUP Token Transmission */
		uint16_t CSCLR	:1;	/* C-SPLIT Status Clear */
		uint16_t CSSTS	:1;	/* C-SPLIT Status */
		uint16_t SUREQCLR :1;	/* SUREQ Bit Clear */
		uint16_t	:2;
		uint16_t SQCLR	:1;	/* Toggle Bit Clear */
		uint16_t SQSET	:1;	/* Toggle Bit Set */
		uint16_t SQMON	:1;	/* Sequence Toggle Bit Monitor */
		uint16_t PBUSY	:1;	/* Pipe Busy */
		uint16_t PINGE	:1;	/* PING Token Issue Enable */
		uint16_t	:1;
		uint16_t CCPL	:1;	/* Control Transfer End Enable */
		uint16_t PID	:2;	/* Response PID */
	);
	pad(2);

	/* Pipe Window Select Register */
	word_union(PIPESEL,
		uint16_t	:12;
		uint16_t PIPESEL :4;	/* Pipe Window Select */
	);
	pad(2);

	/* Pipe Configuration Register */
	word_union(PIPECFG,
		uint16_t TYPE	:2;	/* Transfer Type */
		uint16_t	:3;
		uint16_t BFRE	:1;	/* BRDY Interrupt Operation Specification */
		uint16_t DBLB	:1;	/* Double Buffer Mode */
		uint16_t CNTMD	:1;	/* Continuous Transfer Mode */
		uint16_t SHTNAK	:1;	/* Pipe Disabled at End of Transfer */
		uint16_t	:2;
		uint16_t DIR	:1;	/* Transfer Direction */
		uint16_t EPNUM	:4;	/* Endpoint Number */
	);

	/* Pipe Buffer Setting Register */
	word_union(PIPEBUF,
		uint16_t	:1;
		uint16_t BUFSIZE:5;	/* Buffer Size */
		uint16_t	:2;
		uint16_t BUFNMB	:8;	/* Buffer Number */
	);

	/* Pipe Maximum Packet Size Register */
	word_union(PIPEMAXP,
		uint16_t DEVSEL	:4;	/* Device Select */
		uint16_t	:1;
		uint16_t MXPS	:11;	/* Maximum Packet Size */
	);

	/* Pipe Timing Control Register */
	word_union(PIPEPERI,
		uint16_t	:3;
		uint16_t IFIS	:1;	/* Isochronous IN Buffer Flush */
		uint16_t	:9;
		uint16_t IITV	:3;	/* Interval Error Detection Interval */
	);

	/* PIPEn Control Registers */
	word_union(PIPECTR[9],
		uint16_t BSTS	:1;	/* Buffer Status */
		uint16_t INBUFM	:1;	/* IN Buffer Monitor (PIPE1..5) */
		uint16_t CSCLR	:1;	/* C-SPLIT Status Clear Bit */
		uint16_t CSSTS	:1;	/* CSSTS Status Bit */
		uint16_t	:1;
		uint16_t ATREPM	:1;	/* Auto Response Mode (PIPE1..5) */
		uint16_t ACLRM	:1;	/* Auto Buffer Clear Mode */
		uint16_t SQCLR	:1;	/* Toggle Bit Clear */
		uint16_t SQSET	:1;	/* Toggle Bit Set */
		uint16_t SQMON	:1;	/* Toggle Bit Confirmation */
		uint16_t PBUSY	:1;	/* Pipe Busy */
		uint16_t	:3;
		uint16_t PID	:2;	/* Response PID */
	);
	pad(14);

	/* Transaction Counter Registers (PIPE1..PIPE5 only) */
	sh7305_usb_pipetr PIPETR[5];
	pad(0x1e);

	uint16_t REG_C2;
	pad(12);

	word_union(DEVADD[11],
		uint16_t	:1;
		uint16_t UPPHUB	:4;	/* Address of target's hub */
		uint16_t HUBPORT:3;	/* Hub port where target connects */
		uint16_t USBSPD	:2;	/* Transfer speed / Target present */
		uint16_t	:6;
	);

} GPACKED(4) sh7305_usb_t;

typedef volatile word_union(sh7305_usb_uponcr_t,
	uint16_t	:5;
	uint16_t UPON	:2;	/* USB Power ON */
	uint16_t	:9;
);

#define SH7305_USB (*(sh7305_usb_t *)0xa4d80000)
#define SH7305_USB_UPONCR (*(sh7305_usb_uponcr_t *)0xa40501d4)

#ifdef __cplusplus
}
#endif

#endif /* GINT_MPU_USB */
