#include <gint/display.h>
#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void dimage_p8_dye(int x, int y, image_t const *img, int eff, int dye_color)
{
	dsubimage_p8_dye(x, y, img, 0, 0, img->width, img->height, eff,
		dye_color);
}

void dsubimage_p8_dye(int x, int y, image_t const *img,
	int left, int top, int w, int h, int eff, int dye_color)
{
	struct gint_image_box box = { x, y, w, h, left, top };
	struct gint_image_cmd cmd;

	if(!gint_image_mkcmd(&box, img, eff, false, true, &cmd, &dwindow))
		return;
	cmd.color_1 = image_alpha(img->format);
	cmd.color_2 = dye_color;
	cmd.loop = gint_image_p8_dye;
	gint_image_p8_loop(DWIDTH, &cmd);
}

#endif
