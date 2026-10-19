#include <gint/image.h>
#include <gint/display.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

image_t *image_create_vram(void)
{
    image_t *img = image_create(DWIDTH, DHEIGHT, IMAGE_RGB565);
    if(!img)
        return NULL;

    img->stride = 2 * DWIDTH;
    img->data = gint_vram;
    return img;
}

#endif
