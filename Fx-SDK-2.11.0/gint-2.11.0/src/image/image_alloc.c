#include <gint/image.h>
#include <stdlib.h>
#include <gint/defs/util.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

image_t *image_alloc(int width, int height, int format)
{
    image_t *img = image_create(width, height, format);
    if(!img)
        return NULL;

    if(IMAGE_IS_RGB16(format))
        img->stride = ((width + 1) >> 1) * 4;
    else if(IMAGE_IS_P8(format))
        img->stride = width;
    else if(IMAGE_IS_P4(format))
        img->stride = ((width + 1) >> 1);

    void *data = malloc(height * img->stride);
    if(!data) {
        image_free(img);
        return NULL;
    }

    img->data = data;
    img->flags |= IMAGE_FLAGS_DATA_ALLOC;
    return img;
}

#endif
