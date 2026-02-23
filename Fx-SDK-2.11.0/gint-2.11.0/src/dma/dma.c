#include <gint/hardware.h>
#include <gint/mpu/dma.h>
#include <gint/mpu/power.h>
#include <gint/mpu/intc.h>
#include <gint/intc.h>
#include <gint/dma.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/clock.h>
#include <gint/exc.h>
#include <gint/cpu.h>

#define DMA SH7305_DMA
#define POWER SH7305_POWER

typedef volatile sh7305_dma_channel_t channel_t;

/* Callbacks for all channels */
static gint_call_t dma_callbacks[6] = { 0 };
/* Sleep blocking flags for all channels */
static bool dma_sleep_blocking[6] = { 0 };
/* ICS for dma_channel_wait() for all channels */
static cpu_csleep_t *dma_wait_ics[6] = { 0 };

/* dma_channel(): Get address of a DMA channel */
static channel_t *dma_channel(int channel)
{
	channel_t *addr[6] = {
		&DMA.DMA0, &DMA.DMA1, &DMA.DMA2,
		&DMA.DMA3, &DMA.DMA4, &DMA.DMA5,
	};

	return ((uint)channel >= 6 ? NULL : addr[channel]);
}

/* dma_translate(): Translate virtual address to DMA-suitable form */
static uint32_t dma_translate(void const *address)
{
	uint32_t a = (uint32_t)address;

	/* Preserve RS addresses (as of SH7724 Reference, 11.2.2) */
	if(a >= 0xfd800000 && a < 0xfd800800)
		return a;

	/* Translate virtual addresses to IL memory to physical addresses; the
	   same address is used (as of SH7724 Reference, 10.3.3) */
	if(a >= 0xe5200000 && a < 0xe5204000)
		return a;

	/* First additional on-chip memory area (XRAM) */
	if(a >= 0xe5007000 && a < 0xe5009000)
		return a;

	/* Second on-chip memory area (YRAM) */
	if(a >= 0xe5017000 && a < 0xe5019000)
		return a;

	/* Translate P1 and P2 addresses to ROM and RAM to physical form */
	if(a >= 0x80000000 && a < 0xc0000000)
		return a & 0x1fffffff;

	/* By default: I don't know what this is, let's preserve it */
	return a;
}

//---
//	Driver interface
//---

/* dma_setup(): Setup the DMA in interrupt or no-interrupt mode.
   The first parameters are as for dma_transfer() and dma_transfer_atomic().
   The last parameter indicates whether interrupts should be used.
   Returns non-zero if the DMA is busy or a configuration error occurs. */
static int dma_setup(int channel, dma_size_t size, uint blocks,
	void const *src, dma_address_t src_mode,
	void *dst, dma_address_t dst_mode,
	int interrupts)
{
	channel_t *ch = dma_channel(channel);
	if(!ch) return 1;

	/* Safety guard: only start a transfer if there's not one running */
	if(ch->CHCR.DE) return 1;

	/* Disable channel and disable the master DMA switch */
	ch->CHCR.DE = 0;
	DMA.OR.DME = 0;

	/* Set DMA source and target address */
	ch->SAR = dma_translate(src);
	ch->DAR = dma_translate(dst);

	/* Set the number of blocks to be transferred */
	ch->TCR = blocks;

	/* Fill in CHCR. Set RS=0100 (auto-request) and the user-provided
	   values for TS (transfer size), DM and SM (address modes) */
	ch->CHCR.lword = 0x00000400;
	ch->CHCR.TS_32 = (size >> 2);
	ch->CHCR.TS_10 = (size & 3);
	ch->CHCR.DM = dst_mode;
	ch->CHCR.SM = src_mode;
	ch->CHCR.IE = !!interrupts;

	/* Prepare DMAOR by enabling the master switch and clearing the
	   blocking flags. */
	DMA.OR.DME = 1;
	DMA.OR.AE = 0;
	DMA.OR.NMIF = 0;

	/* Block sleep when the transfer involves on-chip memory */
	dma_sleep_blocking[channel] = false;

	if(ch->SAR >= 0xe5007000 && ch->SAR <= 0xe5204000)
		dma_sleep_blocking[channel] = true;
	if(ch->DAR >= 0xe5007000 && ch->DAR <= 0xe5204000)
		dma_sleep_blocking[channel] = true;

	if(ch->SAR >= 0xfe200000 && ch->SAR <= 0xfe3fffff)
		dma_sleep_blocking[channel] = true;
	if(ch->DAR >= 0xfe200000 && ch->DAR <= 0xfe3fffff)
		dma_sleep_blocking[channel] = true;

	return 0;
}

bool dma_transfer_async(int channel, dma_size_t size, uint blocks,
	void const *src, dma_address_t src_mode, void *dst,
	dma_address_t dst_mode, gint_call_t callback)
{
	if(dma_setup(channel, size, blocks, src, src_mode, dst, dst_mode, 1))
		return false;

	dma_callbacks[channel] = callback;

	if(dma_sleep_blocking[channel])
		sleep_block();

	/* Enable channel, starting the DMA transfer. */
	channel_t *ch = dma_channel(channel);
	ch->CHCR.DE = 1;
	return true;
}

/* Interrupt handler for all finished DMA transfers */
static void dma_interrupt_transfer_ended(int channel)
{
	channel_t *ch = dma_channel(channel);
	ch->CHCR.IE = 0;
	ch->CHCR.DE = 0;
	ch->CHCR.TE = 0;

	DMA.OR.AE = 0;
	DMA.OR.NMIF = 0;

	if(dma_sleep_blocking[channel])
		sleep_unblock();

	/* Cancel any sleep operation that is synchronized with this interrupt */
	if(dma_wait_ics[channel])
		cpu_csleep_cancel(dma_wait_ics[channel]);

	gint_call(dma_callbacks[channel]);
	dma_callbacks[channel] = GINT_CALL_NULL;
}

/* dma_channel_wait(): Wait for a particular channel's transfer to finish

   This function is used both during normal gint operation and during foreign
   unbinds of the DMA driver. The waiting method varies with interrupt settings
   and device ownership. */
static void dma_channel_wait(int channel, bool foreign)
{
	channel_t *ch = dma_channel(channel);
	if(!ch) return;

	/* If interrupts are disabled or we don't own the device, spin-wait by
	   checking either for TE to be set (Transfer Ended) or DE to be gone
	   (channel disabled).

	   There are definitely race conditions if the DMA is restarted between
	   our checks; only the context of the calls guarantee soundness.

	   * If interrupts are disabled, we assume there is no one that could
	     start the DMA again, since we are the only thread of execution.
	   * If the device is owned by another kernel, then we're transitioning
	     so we have to wait for *all* tasks to complete anyway. The risk is
	     rather to stop too early. */
	if(!ch->CHCR.IE || foreign)
	{
		while(ch->CHCR.DE && !ch->CHCR.TE) {}
		return;
	}

	/* Initialize an interrupt-cancellable sleep, to ensure
	   synchronization */
	cpu_csleep_t ics;
	cpu_csleep_init(&ics);
	dma_wait_ics[channel] = &ics;

	/* Now the ICS is set; if the interrupt has not occurred yet then the
	   handler is guaranteed to cancel the sleep at some point */
	if(ch->CHCR.DE && !ch->CHCR.TE) cpu_csleep(&ics);

	/* Clear the ICS pointer for next time */
	dma_wait_ics[channel] = NULL;
}

/* dma_transfer_wait(): Wait for a transfer to finish */
void dma_transfer_wait(int channel)
{
	dma_channel_wait(channel, false);
}

bool dma_transfer_sync(int channel, dma_size_t size, uint length,
	void const *src, dma_address_t src_mode, void *dst,
	dma_address_t dst_mode)
{
	if(!dma_transfer_async(channel, size, length, src, src_mode, dst,
		dst_mode, GINT_CALL_NULL)) return false;
	dma_transfer_wait(channel);
	return true;
}

/* dma_transfer_atomic(): Perform a data transfer without interruptions */
void dma_transfer_atomic(int channel, dma_size_t size, uint blocks,
	void const *src, dma_address_t src_mode,
	void *dst, dma_address_t dst_mode)
{
	if(dma_setup(channel, size, blocks, src, src_mode, dst, dst_mode, 0))
		return;

	/* Enable channel, starting the DMA transfer. */
	channel_t *ch = dma_channel(channel);
	ch->CHCR.DE = 1;

	/* Actively wait until the transfer is finished */
	while(!ch->CHCR.TE);

	/* Disable the channel and clear the TE flag. Disable the channel first
	   as clearing the TE flag will allow the transfer to restart */
	ch->CHCR.DE = 0;
	ch->CHCR.TE = 0;

	/* Clear the AE and NMIF status flags */
	DMA.OR.AE = 0;
	DMA.OR.NMIF = 0;
}

/* Deprecated version of dma_transfer_async() that did not have a callback */
void dma_transfer(int channel, dma_size_t size, uint length, void const *src,
	dma_address_t src_mode, void *dst, dma_address_t dst_mode)
{
	dma_transfer_async(channel, size, length, src, src_mode, dst, dst_mode,
		GINT_CALL_NULL);
}

//---
//	Initialization
//---

static void configure(void)
{
	if(isSH3()) return;

	/* Install the interrupt handler from dma/inth.s */
	int codes[] = { 0x800, 0x820, 0x840, 0x860, 0xb80, 0xba0 };
	extern void inth_dma_te(void);

	for(int i = 0; i < 6; i++)
	{
		intc_handler_function(codes[i],
			GINT_CALL(dma_interrupt_transfer_ended, i));

		/* Disable the channel */
		dma_channel(i)->CHCR.DE = 0;
	}

	/* Install the address error gate */
	extern void inth_dma_ae(void);
	intc_handler(0xbc0, inth_dma_ae, 32);

	/* Set interrupt priority to 3, except 11 for the channels that are
	   used by the USB driver */
	intc_priority(INTC_DMA_DEI0, 3);
	intc_priority(INTC_DMA_DEI1, 3);
	intc_priority(INTC_DMA_DEI2, 3);
	intc_priority(INTC_DMA_DEI3, 9);
	intc_priority(INTC_DMA_DEI4, 9);
	intc_priority(INTC_DMA_DEI5, 3);
	intc_priority(INTC_DMA_DADERR, 3);

	/* Clear blocking flags and enable the master switch */
	DMA.OR.AE = 0;
	DMA.OR.NMIF = 0;
	DMA.OR.DME = 1;
}

static void funbind(void)
{
	/* Wait for all OS transfers to finish before taking over */
	for(int channel = 0; channel < 6; channel++)
		dma_channel_wait(channel, true);
}

static void unbind(void)
{
	/* Make sure all DMA transfers are finished before leaving gint */
	for(int channel = 0; channel < 6; channel++)
		dma_channel_wait(channel, false);
}

static bool hpowered(void)
{
	if(isSH3()) return false;
	return (POWER.MSTPCR0.DMAC0 == 0);
}

static void hpoweron(void)
{
	if(isSH3()) return;
	POWER.MSTPCR0.DMAC0 = 0;
}

static void hpoweroff(void)
{
	if(isSH3()) return;
	POWER.MSTPCR0.DMAC0 = 1;
}

//---
// State and driver metadata
//---

static void hsave(dma_state_t *s)
{
	if(isSH3()) return;

	for(int i = 0; i < 6; i++)
	{
		channel_t *ch = dma_channel(i);
		s->ch[i].SAR        = ch->SAR;
		s->ch[i].DAR        = ch->DAR;
		s->ch[i].TCR        = ch->TCR;
		s->ch[i].CHCR.lword = ch->CHCR.lword;
	}
	s->OR = DMA.OR.word;
}

static void hrestore(dma_state_t const *s)
{
	if(isSH3()) return;

	/* Disable the DMA while editing */
	DMA.OR.DME = 0;

	for(int i = 0; i < 6; i++)
	{
		channel_t *ch = dma_channel(i);
		ch->SAR        = s->ch[i].SAR;
		ch->DAR        = s->ch[i].DAR;
		ch->TCR        = s->ch[i].TCR;
		ch->CHCR.lword = s->ch[i].CHCR.lword;
	}
	DMA.OR.word = s->OR;
}

gint_driver_t drv_dma0 = {
	.name         = "DMA",
	.configure    = configure,
	.funbind      = funbind,
	.unbind       = unbind,
	.hpowered     = hpowered,
	.hpoweron     = hpoweron,
	.hpoweroff    = hpoweroff,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(dma_state_t),
};
GINT_DECLARE_DRIVER(05, drv_dma0);
