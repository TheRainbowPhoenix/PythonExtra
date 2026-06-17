#include <gint/mpu/spu.h>
#include <gint/mpu/cpg.h>
#include <gint/mpu/power.h>
#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/clock.h>
#include <gint/intc.h>

#define SPU   SH7305_SPU
#define DSP0  SH7305_DSP0
#define DSP1  SH7305_DSP1
#define CPG   SH7305_CPG
#define POWER SH7305_POWER

static void configure(void)
{
	/* Block SPU interrupts from DSP0, DSP1, and their DMA */
	intc_priority(INTC_SPU_DSP0, 0);
	intc_priority(INTC_SPU_DSP1, 0);

	/* Stop both the SPU and FSI clocks */
	CPG.FSICLKCR.lword = 0x00000103;
	CPG.SPUCLKCR.lword = 0x00000100;
	/* Enable the FSI clock, then the SPU/SPURAM clock */
	CPG.FSICLKCR.CLKSTP = 0;
	CPG.SPUCLKCR.CLKSTP = 0;

	/* Power the clocks through MSTPCR2 */
	POWER.MSTPCR2.FSI_SPU = 0;

	/* Reset the SPU */
	SPU.SPUSRST.RST = 0;
	sleep_us_spin(1000);
	SPU.SPUSRST.RST = 1;
	sleep_us_spin(1000);

	/* Initially give all P memory and X memory to DSP0 */
	SPU.PBANKC0 = 0x1f;
	SPU.PBANKC1 = 0x00;
	SPU.XBANKC0 = 0x7f;
	SPU.XBANKC1 = 0x00;

	/* Perform full DSP resets */
	DSP0.DSPCORERST = 1;
	DSP1.DSPCORERST = 1;
	DSP0.DSPRST = 0;
	DSP1.DSPRST = 0;
	sleep_us_spin(1000);
}

int spu_zero(void)
{
	return 0;
}

//---
// State and driver metadata
//---

static void hsave(spu_state_t *s)
{
	s->PBANKC0 = SPU.PBANKC0;
	s->PBANKC1 = SPU.PBANKC1;
	s->XBANKC0 = SPU.XBANKC0;
	s->XBANKC1 = SPU.XBANKC1;
}

static void hrestore(spu_state_t const *s)
{
	SPU.PBANKC0 = s->PBANKC0;
	SPU.PBANKC1 = s->PBANKC1;
	SPU.XBANKC0 = s->XBANKC0;
	SPU.XBANKC1 = s->XBANKC1;
}

gint_driver_t drv_spu = {
	.name         = "SPU",
	.configure    = configure,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(spu_state_t),
};
GINT_DECLARE_DRIVER(16, drv_spu);
