#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

image_t *image_hflip_alloc(image_t const *src)
{
    if(!image_valid(src))
        return NULL;

    image_t *dst = image_alloc(src->width, src->height, src->format);
    if(!dst || !image_copy_palette(src, dst, -1)) {
        image_free(dst);
        return NULL;
    }

    image_hflip(src, dst, true);
    return dst;
}

#endif
