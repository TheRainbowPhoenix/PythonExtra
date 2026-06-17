#include <gint/display.h>
#include "../render/render.h"
#include "render-fx.h"

#include <gint/config.h>
#if GINT_RENDER_MONO

#pragma GCC optimize("O3")

/* dsubimage(): Render a section of an image */
void dsubimage(int x, int y, bopti_image_t const *img, int left, int top,
	int width, int height, int flags)
{
	struct rbox r = {
		0, x, y, width, left, 0, top, height
	};

	DMODE_OVERRIDE(dsubimage, img, &r, flags);
	if(img->gray) return;

	/* Intersect the bounding box with both the source image and the VRAM,
	   except if DIMAGE_NOCLIP is provided */
	if(!(flags & DIMAGE_NOCLIP))
	{
		/* Early finish for empty intersections */
		if(bopti_clip(img, &r)) return;
	}

	left = r.left;
	width = r.width;
	int visual_x = r.visual_x;

	r.left = left >> 5;
	r.columns = ((left + width - 1) >> 5) - r.left + 1;

	if(r.columns == 1 && (visual_x & 31) + width <= 32)
	{
		r.x = (left & 31) - (visual_x & 31);
		bopti_render_scsp(img, &r, gint_vram, NULL);
	}
	else
	{
		/* x-coordinate of the first pixel of the first column */
		r.x = visual_x - (left & 31);
		bopti_render(img, &r, gint_vram, NULL);
	}
}

#endif /* GINT_RENDER_MONO */
