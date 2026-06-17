#include <gint/display.h>

/* dimage(): Render a full image */
void dimage(int x, int y, bopti_image_t const *img)
{
	dsubimage(x, y, img, 0, 0, img->width, img->height, DIMAGE_NONE);
}
