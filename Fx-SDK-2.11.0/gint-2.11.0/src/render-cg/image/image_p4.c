#include <gint/image.h>
#include <gint/display.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void dimage_p4(int x, int y, image_t const *img, int eff)
{
	dsubimage_p4(x, y, img, 0, 0, img->width, img->height, eff);
}

void dsubimage_p4(int x, int y, image_t const *img,
	int left, int top, int w, int h, int eff)
{
	if(img->format == IMAGE_P4_RGB565A)
		return dsubimage_p4_clearbg(x, y, img, left, top, w, h, eff,
			image_alpha(img->format));

	struct gint_image_box box = { x, y, w, h, left, top };
	struct gint_image_cmd cmd;

	if(!gint_image_mkcmd(&box, img, eff, false, false, &cmd, &dwindow))
		return;
	cmd.loop = gint_image_p4_normal;
	gint_image_p4_loop(DWIDTH, &cmd);
}

void dimage_p4_clearbg(int x, int y, image_t const *img, int eff, int bg)
{
	dsubimage_p4_clearbg(x, y, img, 0, 0, img->width, img->height, eff,bg);
}

void dsubimage_p4_clearbg(int x, int y, image_t const *img,
	int left, int top, int w, int h, int eff, int bg_color)
{
	struct gint_image_box box = { x, y, w, h, left, top };
	struct gint_image_cmd cmd;

	if(!gint_image_mkcmd(&box, img, eff, false, false, &cmd, &dwindow))
		return;
	cmd.color_1 = bg_color;
	cmd.loop = gint_image_p4_clearbg;
	gint_image_p4_loop(DWIDTH, &cmd);
}

#endif
