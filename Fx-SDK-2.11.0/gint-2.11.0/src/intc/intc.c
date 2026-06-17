#include <gint/intc.h>
#include <gint/gint.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/hardware.h>
#include <gint/cpu.h>
#include <gint/mpu/intc.h>

#include <string.h>

//---
// Interrupt controllers
//---

GRODATA3 sh7705_intc_t const SH7705_INTC = {
	.IPR = {
		(void *)0xfffffee2, (void *)0xfffffee4,
		(void *)0xa4000016, (void *)0xa4000018, (void *)0xa400001a,
		(void *)0xa4080000, (void *)0xa4080002, (void *)0xa4080004,
	},
	.ICR1 = (void *)0xa4000010,
};

sh7305_intc_t const SH7305_INTC = {
	.IPR		= (void *)0xa4080000,
	.MSK		= (void *)0xa4080080,
	.MSKCLR		= (void *)0xa40800c0,
	.USERIMASK	= (void *)0xa4700000,
};

/* Interrupt IPR and IMR positions. The order of entries is as in the named
   list of interrupt signals in <gint/intc.h>. */

/* Friendly names for IPR and IMR register numbers */
enum{ IPRA, IPRB, IPRC, IPRD, IPRE, IPRF, IPRG, IPRH, IPRI, IPRJ, IPRK, IPRL };
enum{ IMR0, IMR1, IMR2, IMR3, IMR4, IMR5, IMR6, IMR7, IMR8, IMR9, IMR10 };
#define _ 0,0

static struct info {
	uint16_t IPR4, IPR4bits, IMR, IMRbits;
	/* Only set if different than IPR4 with IPR4bits */
	uint16_t IPR3, IPR3bits;
} const info[] = {
	/* Standard TMU */
	{ IPRA, 0xf000, IMR4,  0x10, _ },
	{ IPRA, 0x0f00, IMR4,  0x20, _ },
	{ IPRA, 0x00f0, IMR4,  0x40, _ },
	/* ETMU */
	{ IPRJ, 0xf000, IMR6,  0x08, IPRF, 0x000f },
	{ IPRG, 0x0f00, IMR5,  0x02, _ },
	{ IPRG, 0x00f0, IMR5,  0x04, _ },
	{ IPRE, 0x00f0, IMR2,  0x01, _ },
	{ IPRI, 0xf000, IMR6,  0x10, _ },
	{ IPRL, 0xf000, IMR8,  0x02, _ },
	/* DMA */
	{ IPRE, 0xf000, IMR1,  0x01, _ /* Not supported on SH3! */ },
	{ IPRE, 0xf000, IMR1,  0x02, _ },
	{ IPRE, 0xf000, IMR1,  0x04, _ },
	{ IPRE, 0xf000, IMR1,  0x08, _ },
	{ IPRF, 0x0f00, IMR5,  0x10, _ },
	{ IPRF, 0x0f00, IMR5,  0x20, _ },
	{ IPRF, 0x0f00, IMR5,  0x40, _ },
	/* SCIF */
	{ IPRG, 0xf000, IMR5,  0x01, _ /* Driver not SH3-compatible yet */ },
	/* RTC */
	{ IPRK, 0xf000, IMR10, 0x04, IPRA, 0x000f },
	{ IPRK, 0xf000, IMR10, 0x02, IPRA, 0x000f },
	{ IPRK, 0xf000, IMR10, 0x01, IPRA, 0x000f },
	/* SPU */
	{ IPRC, 0x000f, IMR3, 0x04, _ /* Not supported on SH3! */ },
	{ IPRC, 0x000f, IMR4, 0x08, _ },
	/* USB */
	{ IPRF, 0x00f0, IMR9, 0x02, _ /* Driver not SH3-compatible yet */ },
};

/* Compact SH3 VBR-space scheme

   Due to the low amount of memory available on SH3, event codes that are
   translated to SH4 are further remapped into the VBR space to eliminate gaps
   and save space. Each entry in this table represents a 32-byte block after
   the VBR + 0x200. It shows the SH4 event code whose gate is placed on that
   block (some of gint's SH4 event codes are invented to host helper blocks).

   For instance, the 5th block after the entry gate hosts the interrupt handler
   for SH4 event 0x9e0, which is ETMU0 underflow.

   The _inth_remap table in src/kernel/inth.S combines the SH3-SH4 translation
   with the compact translation, hence its entry for 0xf00 (the SH3 event code
   for ETMU0 underflow) is the offset in this table where 0x9e0 (the SH4 event
   code for the same event) is stored, which is 3. */
static const uint16_t sh3_vbr_map[] = {
	0x400,  /* TMU0 underflow */
	0x420,  /* TMU1 underflow */
	0x440,  /* TMU2 underflow */
	0x9e0,  /* ETMU0 underflow */
	0xd00,  /* ETMU logic #1 (ETMU4 underflow) */
	1,      /* ETMU logic #2 */
	1,      /* ETMU logic #3 */
	0xaa0,  /* RTC Periodic Interrupt */
	0
};

//---
// Interrupt controller functions
//---

int intc_priority(int intname, int level)
{
	struct info const *i = &info[intname];
	int IPRn = i->IPR4, IPRbits = i->IPR4bits;

	if(isSH3() && i->IPR3bits != 0)
	{
		IPRn = i->IPR3;
		IPRbits = i->IPR3bits;
	}

	/* Bit-shift for the mask */
	int shift = 0;
	while(IPRbits >>= 4) shift += 4;

	uint16_t volatile *IPR;
	IPR = isSH3() ? SH7705_INTC.IPR[IPRn] : &SH7305_INTC.IPR[2*IPRn];

	int oldlevel = (*IPR >> shift) & 0xf;
	*IPR = (*IPR & ~(0xf << shift)) | (level << shift);

	if(isSH4() && level > 0 && i->IMRbits)
	{
		uint8_t volatile *MSKCLR = &SH7305_INTC.MSKCLR->IMR0;
		MSKCLR[4*i->IMR] = i->IMRbits;
	}

	return oldlevel;
}

void *intc_handler(int event_code, const void *handler, size_t size)
{
	void *dest;

	/* Normalize the event code */
	if(event_code < 0x400) return NULL;
	event_code &= ~0x1f;

	/* Prevent writing beyond the end of the VBR space on SH4. Using code
	   0xfc0 into the interrupt handler space (which starts 0x540 bytes
	   into VBR-reserved memory) would reach byte 0x540 + 0xfc0 - 0x400 =
	   0x1100, which is out of gint's reserved VBR area.  */
	if(isSH4() && event_code + size > 0xfc0) return NULL;

	/* On SH3, make VBR compact. Use this offset specified in the VBR map
	   above to avoid gaps */
	if(isSH3())
	{
		int index = 0;
		while(sh3_vbr_map[index])
		{
			if((int)sh3_vbr_map[index] == event_code) break;
			index++;
		}

		/* This happens if the event has not beed added to the table,
		   ie. the compact VBR scheme does not support this code */
		if(!sh3_vbr_map[index]) return NULL;

		dest = (void *)cpu_getVBR() + 0x200 + index * 0x20;
	}
	/* On SH4, just use the code as offset */
	else
	{
		/* 0x40 is the size of the entry gate */
		dest = (void *)cpu_getVBR() + 0x640 + (event_code - 0x400);
	}

	return memcpy(dest, handler, size);
}

bool intc_handler_function(int event_code, gint_call_t function)
{
	/* Install the generic handler */
	extern void intc_generic_handler(void);
	void *h = intc_handler(event_code, intc_generic_handler, 32);
	if(!h) return false;

	/* Copy the call */
	memcpy(h + 8, &function, 20);
	/* Copy the runtime address of gint_inth_callback() */
	*(void **)(h + 28) = gint_inth_callback;

	return true;
}

//---
// State and driver metadata
//---

static void configure(void)
{
	/* Just disable everything, drivers will enable what they support */
	if(isSH3()) for(int i = 0; i < 8; i++)
		*(SH7705_INTC.IPR[i]) = 0x0000;
	else for(int i = 0; i < 12; i++)
		SH7305_INTC.IPR[2 * i] = 0x0000;
}

static void hsave(intc_state_t *s)
{
	if(isSH3())
	{
		for(int i = 0; i < 8; i++)
			s->IPR[i] = *(SH7705_INTC.IPR[i]);
	}
	else
	{
		for(int i = 0; i < 12; i++)
			s->IPR[i] = SH7305_INTC.IPR[2 * i];

		uint8_t *IMR = (void *)SH7305_INTC.MSK;
		for(int i = 0; i < 13; i++, IMR += 4)
			s->MSK[i] = *IMR;
	}
}

static void hrestore(intc_state_t const *s)
{
	if(isSH3())
	{
		for(int i = 0; i < 8; i++)
			*(SH7705_INTC.IPR[i]) = s->IPR[i];
	}
	else
	{
		for(int i = 0; i < 12; i++)
			SH7305_INTC.IPR[2 * i] = s->IPR[i];

		/* Setting masks it a bit more involved than reading them */
		uint8_t *IMCR = (void *)SH7305_INTC.MSKCLR;
		uint8_t *IMR  = (void *)SH7305_INTC.MSK;
		for(int i = 0; i < 13; i++, IMR += 4, IMCR += 4)
		{
			*IMCR = 0xff;
			*IMR = s->MSK[i];
		}
	}
}

gint_driver_t drv_intc = {
	.name         = "INTC",
	.configure    = configure,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(intc_state_t),
};
GINT_DECLARE_DRIVER(01, drv_intc);
