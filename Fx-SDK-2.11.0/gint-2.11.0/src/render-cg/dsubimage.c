#include <gint/display.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

/* dsubimage(): Render a section of an image */
void dsubimage(int x, int y, image_t const *img, int left, int top,
	int w, int h, int flags)
{
	if(IMAGE_IS_RGB16(img->format))
		return dsubimage_rgb16(x, y, img, left, top, w, h, flags);
	else if(IMAGE_IS_P8(img->format))
		return dsubimage_p8(x, y, img, left, top, w, h, flags);
	else if(IMAGE_IS_P4(img->format))
		return dsubimage_p4(x, y, img, left, top, w, h, flags);
}

#endif
