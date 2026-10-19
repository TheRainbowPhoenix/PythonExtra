#include <gint/display.h>
#include <gint/drivers/r61523.h>
#include <gint/drivers/r61524.h>
#include "render-cg.h"
#include <gint/config.h>
#if GINT_RENDER_RGB

#if GINT_HW_CP

void dupdate(void)
{
	r61523_display(gint_vram);
	gint_call(dupdate_get_hook());
}

#elif GINT_HW_CG

/* dupdate(): Push the video RAM to the display driver */
void dupdate(void)
{
	/* If triple buffering is enabled, don't wait for the DMA to finish */
	uint16_t *vram_1, *vram_2;
	dgetvram(&vram_1, &vram_2);
	int method = (vram_1 == vram_2) ? R61524_DMA_WAIT : R61524_DMA;

	r61524_display(gint_vram, 0, 224, method);

	gint_call(dupdate_get_hook());

	/* Switch buffers if triple buffering is enabled */
	dvram_switch();
}

#endif

__attribute__((alias("dupdate")))
void _WEAK_dupdate(void);

#endif /* GINT_RENDER_RGB */
