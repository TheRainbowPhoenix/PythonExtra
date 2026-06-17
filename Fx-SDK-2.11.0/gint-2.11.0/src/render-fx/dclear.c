#include <gint/display.h>
#include "../render/render.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

/* dclear() - fill the screen with a single color */
void dclear(color_t color)
{
	if(dwindow.left != 0 || dwindow.right != DWIDTH) {
		drect(dwindow.left, dwindow.top, dwindow.right - 1,
			dwindow.bottom - 1, color);
		return;
	}

	DMODE_OVERRIDE(dclear, color);

	/* SuperH only supports a single write-move addressing mode, which is
	   pre-decrement write; the other similar mode is post-increment
	   read. So we'll use pre-decrement writes to improve performance. */

	if(color != C_WHITE && color != C_BLACK) return;
	uint32_t fill = -(color >> 1);

	uint32_t *start = gint_vram + 4 * dwindow.top;
	uint32_t *index = gint_vram + 4 * dwindow.bottom;

	while(index > start)
	{
		/* Do it by batches to avoid losing cycles on loop tests */
		*--index = fill;
		*--index = fill;
		*--index = fill;
		*--index = fill;

		*--index = fill;
		*--index = fill;
		*--index = fill;
		*--index = fill;

		*--index = fill;
		*--index = fill;
		*--index = fill;
		*--index = fill;

		*--index = fill;
		*--index = fill;
		*--index = fill;
		*--index = fill;
	}
}

#endif /* GINT_RENDER_MONO */
