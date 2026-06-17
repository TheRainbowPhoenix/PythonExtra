//---
//	gint:clock:freq - Clock frequency management
//---

#include <gint/drivers.h>
#include <gint/drivers/states.h>
#include <gint/clock.h>
#include <gint/config.h>

#include <gint/hardware.h>
#include <gint/mpu/cpg.h>

//---
//	Driver storage
//---

/* Local copy of the CPG settings */
GBSS static clock_frequency_t freq;

/* clock_freq() - get the frequency of the main clocks */
const clock_frequency_t *clock_freq(void)
{
	return &freq;
}

//---
//	SH7705 Clock signals
//---

#if GINT_HW_FX

void sh7705_probe(void)
{
	/* According to Sentaro21 in the sources of Ftune 1.0.1, the clock mode
	   is thought to be 5, which means that:
	   - CPG input is XTAL         (14.745'600 MHz)
	   - PLL2 is active and *2     (29.491'200 MHz)
	   - CKIO is output from PLL2  (29.491'200 MHz) */
	int xtal = 14745600;
	int pll2 = 2;
	int ckio = xtal * pll2;

	/* This signal is multiplied by the PLL1 circuit */
	int pll1 = SH7705_CPG.FRQCR.STC + 1;

	/* Fill in the setting structure. Iϕ and Pϕ have dividers, while Bϕ is
	   always equal to CKIO. */
	freq.PLL1 = pll1;
	freq.PLL2 = pll2;
	freq.Bphi_div = 1;
	freq.Iphi_div = SH7705_CPG.FRQCR.IFC + 1;
	freq.Pphi_div = SH7705_CPG.FRQCR.PFC + 1;

	/* Deduce the frequency of the main clocks */
	freq.CKIO_f = ckio;
	freq.Bphi_f = ckio;
	freq.Iphi_f = (ckio * pll1) / freq.Iphi_div;
	freq.Pphi_f = (ckio * pll1) / freq.Pphi_div;
}

#endif

//---
//	SH7305 clock signals
//---

#define CPG SH7305_CPG

static void sh7305_probe(void)
{
	/* The meaning of the PLL setting on SH7305 differs from the
	   documentation of SH7224; the value must not be doubled. */
	int pll = CPG.FRQCR.STC + 1;
	freq.PLL = pll;

	/* The FLL ratio is the value of the setting, halved if SELXM=1 */
	int fll = CPG.FLLFRQ.FLF;
	if(CPG.FLLFRQ.SELXM == 1) fll >>= 1;
	freq.FLL = fll;

	/* On SH7724, the divider ratio is given by 1 / (setting + 1), but on
	   the SH7305 it is 1 / (2^setting + 1). */

	int divb = CPG.FRQCR.BFC;
	int divi = CPG.FRQCR.IFC;
	int divp = CPG.FRQCR.P1FC;

	freq.Bphi_div = 1 << (divb + 1);
	freq.Iphi_div = 1 << (divi + 1);
	freq.Pphi_div = 1 << (divp + 1);

	/* Deduce the input frequency of divider 1 */
	int base = 32768;
	if(CPG.PLLCR.FLLE) base *= fll;
	if(CPG.PLLCR.PLLE) base *= pll;

	/* And the frequency of all other input clocks */
	freq.RTCCLK_f = 32768;
	freq.Bphi_f   = base >> (divb + 1);
	freq.Iphi_f   = base >> (divi + 1);
	freq.Pphi_f   = base >> (divp + 1);
}

#undef CPG


//---
// Initialization
//---

void cpg_compute_freq(void)
{
	/* This avoids warnings about sh7705_probe() being undefined when
	   building for fxcg50 */
	#if GINT_HW_FX
	isSH3() ? sh7705_probe() :
	#endif
	sh7305_probe();
}

static void configure(void)
{
	/* Disable spread spectrum in SSGSCR */
	if(isSH4())
		SH7305_CPG.SSCGCR.SSEN = 0;

	cpg_compute_freq();
}

//---
// State and driver metadata
//---

static void hsave(cpg_state_t *s)
{
	if(isSH4())
		s->SSCGCR = SH7305_CPG.SSCGCR.lword;
	cpg_get_overclock_setting(&s->speed);
}

static void hrestore(cpg_state_t const *s)
{
	if(isSH4())
		SH7305_CPG.SSCGCR.lword = s->SSCGCR;
	cpg_set_overclock_setting(&s->speed);
}

gint_driver_t drv_cpg = {
	.name         = "CPG",
	.configure    = configure,
	.hsave        = (void *)hsave,
	.hrestore     = (void *)hrestore,
	.state_size   = sizeof(cpg_state_t),
};
GINT_DECLARE_DRIVER(05, drv_cpg);
