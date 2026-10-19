#include <gint/display.h>
#include "../render/render.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

#if GINT_HW_FX
# include <gint/drivers/t6k11.h>
#elif GINT_HW_CG
# include <gint/drivers/r61524.h>
#else
# error Platform unknown for mono video mode update
#endif

/* Standard video RAM for fx9860g is 1 bit per pixel */
GSECTION(".bss") GALIGNED(32) static uint32_t fx_vram[256];

/* Here is the definition of the VRAM pointer, exposed in <gint/display.h> */
uint32_t *gint_vram = fx_vram;

/* The current rendering mode */
struct rendering_mode const *dmode = NULL;

/* For parity with the current RGB interface */
bool dvram_init(void)
{
	return true;
}
void dvram_quit(void)
{
}

/* dupdate(): Push the video RAM to the display driver */
void dupdate(void)
{
	bool run_default = true;

	if(dmode && dmode->dupdate)
	{
		/* Call the overridden dupdate(), but continue if it returns
		   non-zero (this is used when stopping the gray engine) */
		int rc = dmode->dupdate();
		run_default = (rc != 0);
	}
	if(run_default)
	{
#if GINT_HW_FX
		t6k11_display(gint_vram, 0, 64, 16);
#elif GINT_HW_CG
		r61524_display_mono_128x64(gint_vram);
#endif
	}

	gint_call(dupdate_get_hook());
}
__attribute__((alias("dupdate")))
void _WEAK_dupdate(void);

#endif /* GINT_RENDER_MONO */
