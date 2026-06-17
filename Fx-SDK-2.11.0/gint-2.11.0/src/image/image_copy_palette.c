#include <gint/image.h>
#include <gint/defs/util.h>
#include <string.h>

#include <gint/config.h>
#if GINT_RENDER_RGB

bool image_copy_palette(image_t const *src, image_t *dst, int size)
{
    if(!image_valid(src) || !dst)
        return false;
    if(!IMAGE_IS_INDEXED(dst->format))
        return true;

    if(size < 0)
        size = src->color_count;
    if(!image_alloc_palette(dst, size))
        return false;

    int N = min(src->color_count, dst->color_count);
    memcpy(dst->palette, src->palette, 2*N);
    return true;
}

#endif
