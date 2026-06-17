//---
// gint:cpg:overclock - Clock speed control
//
// Most of the data in this file has been reused from Sentaro21's Ftune and
// Ptune utilities, which have long been the standard for overclocking CASIO
// calculators.
// See: http://pm.matrix.jp/ftune2e.html
//
// SlyVTT also contributed early testing on both the fx-CG 10/20 and fx-CG 50.
//---

#include <gint/clock.h>
#include <gint/gint.h>
#include <gint/hardware.h>
#include <gint/mpu/cpg.h>
#include <gint/mpu/bsc.h>
#include <gint/mpu/wdt.h>

//---
// Low-level clock speed access
//---

#define SH7305_SDMR3_CL2 ((volatile uint8_t *)0xFEC15040)
#define SH7305_SDMR3_CL3 ((volatile uint8_t *)0xFEC15060)

//---
// Predefined clock speeds
//---

/* SH7305 CPG */
#define SH4_PLL_32x  0b011111
#define SH4_PLL_26x  0b011001
#define SH4_PLL_16x  0b001111
#define SH4_DIV_2    0
#define SH4_DIV_4    1
#define SH4_DIV_8    2
#define SH4_DIV_16   3
#define SH4_DIV_32   4

/* SH7705-like CPG */
#define SH3_PLL_1x   0
#define SH3_PLL_2x   1
#define SH3_PLL_3x   2
#define SH3_PLL_4x   3
#define SH3_DIV_1    0
#define SH3_DIV_2    1
#define SH3_DIV_3    2
#define SH3_DIV_4    3

void cpg_get_overclock_setting(struct cpg_overclock_setting *s)
{
    if(isSH3()) {
        s->FLLFRQ = -1;
        s->FRQCR = SH7705_CPG.FRQCR.word;

        s->CS0BCR = SH7705_BSC.CS0BCR.lword;
        s->CS0WCR = SH7705_BSC.CS0WCR.lword;
        s->CS2BCR = SH7705_BSC.CS2BCR.lword;
        s->CS2WCR = SH7705_BSC.CS2WCR.lword;
        s->CS3BCR = SH7705_BSC.CS3BCR.lword;
        s->CS3WCR = SH7705_BSC.CS3WCR.lword;
        s->CS5aBCR = SH7705_BSC.CS5ABCR.lword;
        s->CS5aWCR = SH7705_BSC.CS5AWCR.lword;
    }
    else {
        s->FLLFRQ = SH7305_CPG.FLLFRQ.lword;
        s->FRQCR = SH7305_CPG.FRQCR.lword;

        s->CS0BCR = SH7305_BSC.CS0BCR.lword;
        s->CS0WCR = SH7305_BSC.CS0WCR.lword;
        s->CS2BCR = SH7305_BSC.CS2BCR.lword;
        s->CS2WCR = SH7305_BSC.CS2WCR.lword;
        s->CS3BCR = SH7305_BSC.CS3BCR.lword;
        s->CS3WCR = SH7305_BSC.CS3WCR.lword;
        s->CS5aBCR = SH7305_BSC.CS5ABCR.lword;
        s->CS5aWCR = SH7305_BSC.CS5AWCR.lword;
    }
}

static void cpg_low_level_set_setting(struct cpg_overclock_setting const *s)
{
    if(isSH3()) {
        SH7705_WDT.WTCNT.WRITE = 0;
        SH7705_WDT.WTCSR.WRITE = 0x65;
        SH7705_CPG.FRQCR.word = s->FRQCR | 0x1000;
        SH7705_BSC.CS0BCR.lword = s->CS0BCR;
        SH7705_BSC.CS0WCR.lword = s->CS0WCR;
        SH7705_BSC.CS2BCR.lword = s->CS2BCR;
        SH7705_BSC.CS2WCR.lword = s->CS2WCR;
        SH7705_BSC.CS3BCR.lword = s->CS3BCR;
        SH7705_BSC.CS3WCR.lword = s->CS3WCR;
        SH7705_BSC.CS5ABCR.lword = s->CS5aBCR;
        SH7705_BSC.CS5AWCR.lword = s->CS5aWCR;
    }
    else {
        SH7305_BSC.CS0WCR.WR = 11; /* 18 cycles */

        SH7305_CPG.FLLFRQ.lword = s->FLLFRQ;
        SH7305_CPG.FRQCR.lword = s->FRQCR;
        SH7305_CPG.FRQCR.KICK = 1;
        while(SH7305_CPG.LSTATS != 0) {}

        SH7305_BSC.CS0BCR.lword = s->CS0BCR;
        SH7305_BSC.CS0WCR.lword = s->CS0WCR;
        SH7305_BSC.CS2BCR.lword = s->CS2BCR;
        SH7305_BSC.CS2WCR.lword = s->CS2WCR;
        SH7305_BSC.CS3BCR.lword = s->CS3BCR;
        SH7305_BSC.CS3WCR.lword = s->CS3WCR;

        if(SH7305_BSC.CS3WCR.A3CL == 1)
            *SH7305_SDMR3_CL2 = 0;
        else
            *SH7305_SDMR3_CL3 = 0;

        SH7305_BSC.CS5ABCR.lword = s->CS5aBCR;
        SH7305_BSC.CS5AWCR.lword = s->CS5aWCR;
    }
}

void cpg_set_overclock_setting(struct cpg_overclock_setting const *s)
{
    uint32_t old_Pphi = clock_freq()->Pphi_f;

    /* Wait for asynchronous tasks to complete */
    gint_world_sync();

    /* Disable interrupts during the change */
    cpu_atomic_start();

    /* Load the clock settings */
    cpg_low_level_set_setting(s);

    /* Determine the change in frequency for Pϕ and recompute CPG data */
    cpg_compute_freq();
    uint32_t new_Pphi = clock_freq()->Pphi_f;

    /* Update timers' TCNT and TCOR to match the new clock speed */
    void timer_rescale(uint32_t old_Pphi, uint32_t new_Pphi);
    timer_rescale(old_Pphi, new_Pphi);

    cpu_atomic_end();
}

#if GINT_HW_FX

static struct cpg_overclock_setting const settings_fx9860g_sh3[5] = {
    /* CLOCK_SPEED_F1 */
    { .FRQCR   = 0x1001,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F2 */
    { .FRQCR   = (SH3_PLL_2x<<8)+(SH3_DIV_1<<4)+SH3_DIV_2,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F3 */
    { .FRQCR   = (SH3_PLL_3x<<8)+(SH3_DIV_1<<4)+SH3_DIV_3,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F4 */
    { .FRQCR   = (SH3_PLL_4x<<8)+(SH3_DIV_1<<4)+SH3_DIV_4,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F5 */
    { .FRQCR   = (SH3_PLL_4x<<8)+(SH3_DIV_1<<4)+SH3_DIV_4,
      .CS0BCR  = 0x02480400,
      .CS2BCR  = 0x02483400,
      .CS3BCR  = 0x36DB0600,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000000C0,
      .CS2WCR  = 0x000100C0,
      .CS3WCR  = 0x00000500,
      .CS5aWCR = 0x00000D41 },
};

static struct cpg_overclock_setting const settings_fx9860g_sh4[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = 0x0F202203,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000005C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_8<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x24920200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00020140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_4<<8)+SH4_DIV_16,
      .CS0BCR  = 0x14900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000003C0,
      .CS2WCR  = 0x000302C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
};

static struct cpg_overclock_setting const settings_g35pe2[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = 0x0F112213,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x000001C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00031340 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_8<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x24920200,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00000D41 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00020140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00031B40 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0x00004384,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x14900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x224A0200,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00020140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00031B40 },
};

#endif

#if GINT_HW_CG

static struct cpg_overclock_setting const settings_prizm[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = 0x0F102203,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x000001C0,
      .CS2WCR  = 0x00000140,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_8<<20)+(SH4_DIV_16<<12)+(SH4_DIV_16<<8)+SH4_DIV_32,
      .CS0BCR  = 0x04900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x00000140,
      .CS2WCR  = 0x000100C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_32,
      .CS0BCR  = 0x24900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x000002C0,
      .CS2WCR  = 0x000201C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_32x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_4<<12)+(SH4_DIV_4<<8)+SH4_DIV_32,
      .CS0BCR  = 0x44900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x00000440,
      .CS2WCR  = 0x00040340,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_26x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_4<<8)+SH4_DIV_16,
      .CS0BCR  = 0x34900400,
      .CS2BCR  = 0x04903400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x15140400,
      .CS0WCR  = 0x000003C0,
      .CS2WCR  = 0x000402C0,
      .CS3WCR  = 0x000024D0,
      .CS5aWCR = 0x00010240 },
};

static struct cpg_overclock_setting const settings_fxcg50[5] = {
    /* CLOCK_SPEED_F1 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = 0x0F011112,
      .CS0BCR  = 0x36DA0400,
      .CS2BCR  = 0x36DA3400,
      .CS3BCR  = 0x36DB4400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x000003C0,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
    /* CLOCK_SPEED_F2 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR   = (SH4_PLL_16x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_8,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x00000340,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
    /* CLOCK_SPEED_F3 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR  = (SH4_PLL_26x<<24)+(SH4_DIV_4<<20)+(SH4_DIV_8<<12)+(SH4_DIV_8<<8)+SH4_DIV_8,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x00000240,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
    /* CLOCK_SPEED_F4 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR  = (SH4_PLL_32x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_8<<8)+SH4_DIV_16,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x000002C0,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
    /* CLOCK_SPEED_F5 */
    { .FLLFRQ  = 0x00004000 + 900,
      .FRQCR  = (SH4_PLL_26x<<24)+(SH4_DIV_2<<20)+(SH4_DIV_4<<12)+(SH4_DIV_4<<8)+SH4_DIV_8,
      .CS0BCR  = 0x24920400,
      .CS2BCR  = 0x24923400,
      .CS3BCR  = 0x24924400,
      .CS5aBCR = 0x17DF0400,
      .CS0WCR  = 0x00000440,
      .CS2WCR  = 0x000003C0,
      .CS3WCR  = 0x000024D1,
      .CS5aWCR = 0x000203C1 },
};

#endif

static struct cpg_overclock_setting const *get_settings(void)
{
#if GINT_HW_FX
    if(gint[HWCALC] == HWCALC_FX9860G_SH3)
        return settings_fx9860g_sh3;
    if(gint[HWCALC] == HWCALC_FX9860G_SH4)
        return settings_fx9860g_sh4;
    if(gint[HWCALC] == HWCALC_G35PE2)
        return settings_g35pe2;
#endif

#if GINT_HW_CG
    if(gint[HWCALC] == HWCALC_PRIZM)
        return settings_prizm;
    if(gint[HWCALC] == HWCALC_FXCG50)
        return settings_fxcg50;
#endif

    return NULL;
}

int clock_get_speed(void)
{
    struct cpg_overclock_setting const *settings = get_settings();
    if(!settings)
        return CLOCK_SPEED_UNKNOWN;

    if(isSH3()) {
        for(int i = 0; i < 5; i++) {
            struct cpg_overclock_setting const *s = &settings[i];

            if(SH7705_CPG.FRQCR.word == (s->FRQCR | 0x1000)
                && SH7705_BSC.CS0BCR.lword == s->CS0BCR
                && SH7705_BSC.CS2BCR.lword == s->CS2BCR
                && SH7705_BSC.CS3BCR.lword == s->CS3BCR
                && SH7705_BSC.CS5ABCR.lword == s->CS5aBCR
                && SH7705_BSC.CS0WCR.lword == s->CS0WCR
                && SH7705_BSC.CS2WCR.lword == s->CS2WCR
                && SH7705_BSC.CS3WCR.lword == s->CS3WCR
                && SH7705_BSC.CS5AWCR.lword == s->CS5aWCR)
                return CLOCK_SPEED_F1 + i;
        }
    }
    else {
        for(int i = 0; i < 5; i++) {
            struct cpg_overclock_setting const *s = &settings[i];

            if(SH7305_CPG.FLLFRQ.lword == s->FLLFRQ
                && SH7305_CPG.FRQCR.lword == s->FRQCR
                && SH7305_BSC.CS0BCR.lword == s->CS0BCR
                && SH7305_BSC.CS2BCR.lword == s->CS2BCR
                && SH7305_BSC.CS3BCR.lword == s->CS3BCR
                && SH7305_BSC.CS5ABCR.lword == s->CS5aBCR
                && SH7305_BSC.CS0WCR.lword == s->CS0WCR
                && SH7305_BSC.CS2WCR.lword == s->CS2WCR
                && SH7305_BSC.CS3WCR.lword == s->CS3WCR
                && SH7305_BSC.CS5AWCR.lword == s->CS5aWCR)
                return CLOCK_SPEED_F1 + i;
        }
    }

    return CLOCK_SPEED_UNKNOWN;
}

void clock_set_speed(int level)
{
    if(level < CLOCK_SPEED_F1 || level > CLOCK_SPEED_F5)
        return;
    if(clock_get_speed() == level)
        return;

    struct cpg_overclock_setting const *settings = get_settings();
    if(!settings)
        return;

    struct cpg_overclock_setting const *s = &settings[level - CLOCK_SPEED_F1];
    cpg_set_overclock_setting(s);
}
