#include <gint/display.h>
#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void dimage_rgb16_swapcolor(int x, int y, image_t const *img, int eff,
	int old_color, int new_color)
{
	dsubimage_rgb16_swapcolor(x, y, img, 0, 0, img->width, img->height,
		eff, old_color, new_color);
}

void dsubimage_rgb16_swapcolor(int x, int y, image_t const *img,
	int left, int top, int w, int h, int eff, int old_color, int new_color)
{
	struct gint_image_box box = { x, y, w, h, left, top };
	struct gint_image_cmd cmd;

	if(!gint_image_mkcmd(&box, img, eff, false, false, &cmd, &dwindow))
		return;
	cmd.color_1 = old_color;
	cmd.color_2 = new_color;
	cmd.loop = gint_image_rgb16_swapcolor;
	gint_image_rgb16_loop(DWIDTH, &cmd);
}

void dimage_rgb16_addbg(int x, int y, image_t const *img, int eff,
	int bg_color)
{
	dsubimage_rgb16_addbg(x, y, img, 0, 0, img->width, img->height,
		eff, bg_color);
}

void dsubimage_rgb16_addbg(int x, int y, image_t const *img,
	int left, int top, int w, int h, int eff, int bg_color)
{
	struct gint_image_box box = { x, y, w, h, left, top };
	struct gint_image_cmd cmd;

	if(!gint_image_mkcmd(&box, img, eff, false, false, &cmd, &dwindow))
		return;
	cmd.color_1 = image_alpha(img->format);
	cmd.color_2 = bg_color;
	cmd.loop = gint_image_rgb16_swapcolor;
	gint_image_rgb16_loop(DWIDTH, &cmd);
}

#endif
