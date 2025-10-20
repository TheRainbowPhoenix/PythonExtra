#include <gint/image.h>
#include <gint/display.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void dimage_p4_clearbg_alt(int x, int y, image_t const *img, int eff, int bg)
{
	dsubimage_p4_clearbg_alt(x, y, img, 0, 0, img->width, img->height, eff,
		bg);
}

void dsubimage_p4_clearbg_alt(int x, int y, image_t const *img,
	int left, int top, int w, int h, int eff, int bg_color)
{
	struct gint_image_box box = { x, y, w, h, left, top };
	struct gint_image_cmd cmd;

	if(!gint_image_mkcmd(&box, img, eff, true, true, &cmd, &dwindow))
		return;
	cmd.color_1 = bg_color;
	cmd.loop = gint_image_p4_clearbg_alt;
	gint_image_p4_loop(DWIDTH, &cmd);
}

#endif
