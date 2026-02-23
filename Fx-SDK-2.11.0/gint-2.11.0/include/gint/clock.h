//---
//	gint:clock - Clock signals, overclock, and standby modes
//---

#ifndef GINT_CLOCK
#define GINT_CLOCK

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/defs/types.h>
/* This header used to expose the sleep() function; include <gint/cpu.h> to
   ensure this is still the case */
#include <gint/cpu.h>

//---
//	Clock signals
//---

/* clock_frequency_t
   A dump of the Clock Pulse Generator's (CPG) configuration. Use the MPU
   detection functions from <gint/mpu.h> to use the correct fields. */
typedef struct
{
	union {
		int PLL1;
		int FLL;
	};
	union {
		int PLL2;
		int PLL;
	};

	int Bphi_div;
	int Iphi_div;
	int Pphi_div;

	union {
		int CKIO_f;
		int RTCCLK_f;
	};

	int Bphi_f;
	int Iphi_f;
	int Pphi_f;

} clock_frequency_t;

/* clock_freq() - get the frequency of the main clocks
   This function returns the address of a static object which is used by the
   module; this address never changes. */
const clock_frequency_t *clock_freq(void);

/* cpg_compute_freq(): Compute the current clock frequency
   This function updates the data structure returned by clock_freq() by
   determining the current clock frequencies from the CPG. */
void cpg_compute_freq(void);

//---
//	Overclock
//---

/* The following enumerations define the clock speed settings supported by
   gint. These are always the settings from Ftune/Ptune, which are the most
   widely tested and gint treats as the standard. */
enum {
	/* Combinations of hardware settings that are none of Ftune's levels */
	CLOCK_SPEED_UNKNOWN = 0,

	/* Ftune's 5 default overclock levels. The main settings are listed
	   below, though many more are involved.

	   On SH3 fx-9860G-like:
	     F1:  CPU @  29 MHz                    [Default speed]
	     F2:  CPU @  58 MHz                    [Similar to G-III default]
	     F3:  CPU @  88 MHz
	     F4:  CPU @ 118 MHz                    [Fastest CPU option]
	     F5:  CPU @ 118 MHz                    [Reduced memory wait times]
	   On SH4 fx-9860G-like:
	     F1:  CPU @  29 MHz,  BFC @  29 MHz    [Default speed]
	     F2:  CPU @  58 MHz,  BFC @  29 MHz    [Similar to G-III default]
	     F3:  CPU @  29 MHz,  BFC @  29 MHz    [SH3 default]
	     F4:  CPU @ 118 MHz,  BFC @  59 MHz
	     F5:  CPU @ 236 MHz,  BFC @  118 MHz   [Fastest option]
	   On G-III / Graph 35+E II:
	     F1:  CPU @  58 MHz,  BFC @  29 MHz    [Default speed]
	     F2:  CPU @  58 MHz,  BFC @  29 MHz    [fx-CG 10/20 default]
	     F3:  CPU @  29 MHz,  BFC @  29 MHz    [SH3 default]
	     F4:  CPU @ 118 MHz,  BFC @  58 MHz
	     F5:  CPU @ 235 MHz,  BFC @  58 MHz    [Fastest option]
	   On fx-CG 10/20:
	     F1:  CPU @  58 MHz,  BFC @  29 MHz    [Default speed]
	     F2:  CPU @  58 MHz,  BFC @  29 MHz    [Improved memory speed]
	     F3:  CPU @ 118 MHz,  BFC @  58 MHz    [Faster than F2]
	     F4:  CPU @ 118 MHz,  BFC @ 118 MHz    [Fastest bus option]
	     F5:  CPU @ 191 MHz,  BFC @  94 MHz    [Fastest CPU option]
	   On fx-CG 50:
	     F1:  CPU @ 116 MHz,  BFC @  58 MHz    [Default speed]
	     F2:  CPU @  58 MHz,  BFC @  29 MHz    [fx-CG 10/20 default]
	     F3:  CPU @  94 MHz,  BFC @  47 MHz    [Clearly slow: F2 < F3 < F1]
	     F4:  CPU @ 232 MHz,  BFC @  58 MHz    [Fastest CPU option]
	     F5:  CPU @ 189 MHz,  BFC @  94 MHz    [Fastest bus option] */

	CLOCK_SPEED_F1 = 1,
	CLOCK_SPEED_F2 = 2,
	CLOCK_SPEED_F3 = 3,
	CLOCK_SPEED_F4 = 4,
	CLOCK_SPEED_F5 = 5,

	/* The default clock speed is always Ftune's F1 */
	CLOCK_SPEED_DEFAULT = CLOCK_SPEED_F1,
};

/* clock_get_speed(): Determine the current clock speed

   This function compares the current hardware state with the settings for each
   speed level and returns the current one. If the hardware state does not
   correspond to any of Ftune's settings, CLOCK_SPEED_UNKNOWN is returned. */
int clock_get_speed(void);

/* clock_set_speed(): Set the current clock speed

   This function sets the clock speed to the desired level. This is "the
   overclock function", although depending on the model or settings it is also
   the downclocking function.

   The process of changing clock speeds is non-trivial, requires waiting for
   the DMA to finish its work and slightly affects running timers. You should
   avoid changing the clock speed constantly if not necessary. If this function
   detects that the desired clock speed is already in use, it returns without
   performing any change.

   Currently the clock speed is not reset during a world switch nor when
   leaving the add-in. */
void clock_set_speed(int speed);

/* If you want to faithfully save and restore the clock state while properly
   handling clock speeds that are not Ftune/PTune's defaults, you can get a
   full copy of the settings.

   WARNING: Applying random settings with cpg_set_overclock_setting() might
   damage your calculator! */

struct cpg_overclock_setting
{
    uint32_t FLLFRQ, FRQCR;
    uint32_t CS0BCR, CS2BCR, CS3BCR, CS5aBCR;
    uint32_t CS0WCR, CS2WCR, CS3WCR, CS5aWCR;
};

/* Queries the clock setting from the hardware. */
void cpg_get_overclock_setting(struct cpg_overclock_setting *s);

/* Applies the specified overclock setting. */
void cpg_set_overclock_setting(struct cpg_overclock_setting const *s);

//---
//	Sleep functions
//---

/* sleep_us(): Sleep for a fixed duration in microseconds
   Stops the processor until the specified delay in microseconds has elapsed.
   (The processor will still wake up occasionally to handle interrupts.) This
   function selects a timer with timer_setup() called with TIMER_ANY. */
void sleep_us(uint64_t delay_us);

/* sleep_us_spin(): Actively sleep for a fixed duration in microseconds
   Like sleep_us(), but uses timer_spinwait() and does not rely on interrupts
   being enabled. Useful in timer code running without interrupts. */
void sleep_us_spin(uint64_t delay_us);

/* sleep_ms(): Sleep for a fixed duration in milliseconds */
#define sleep_ms(delay_ms) sleep_us((delay_ms) * 1000ull)

#ifdef __cplusplus
}
#endif

#endif /* GINT_CLOCK */
