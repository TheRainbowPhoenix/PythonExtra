#include <gint/image.h>
#include <stdlib.h>

#include <gint/config.h>
#if GINT_RENDER_RGB

image_t *image_create(int width, int height, int format)
{
    if(!IMAGE_IS_RGB16(format) && !IMAGE_IS_P8(format) && !IMAGE_IS_P4(format))
        return NULL;
    if(width <= 0 || width > 0xffff || height <= 0 || height > 0xffff)
        return NULL;

    image_t *img = malloc(sizeof *img);
    if(!img)
        return NULL;

    img->format = format;
    img->flags = 0;
    img->color_count = 0;
    img->width = width;
    img->height = height;
    img->stride = 0;
    img->data = NULL;
    img->palette = NULL;

    return img;
}

#endif
