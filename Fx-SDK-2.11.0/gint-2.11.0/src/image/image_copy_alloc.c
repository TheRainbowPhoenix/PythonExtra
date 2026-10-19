#include <gint/image.h>
#include <gint/defs/util.h>
#include <string.h>

#include <gint/config.h>
#if GINT_RENDER_RGB

image_t *image_copy_alloc(image_t const *src, int new_format)
{
    if(!image_valid(src))
        return NULL;

    image_t *dst = image_alloc(src->width, src->height, new_format);
    if(!dst)
        return NULL;
    if(!image_copy_palette(src, dst, -1)) {
        image_free(dst);
        return NULL;
    }

    image_copy(src, dst, true);
    return dst;
}

#endif
