#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

image_t *image_linear_alloc(image_t const *src, struct image_linear_map *map)
{
    if(!image_valid(src) || IMAGE_IS_P4(src->format))
        return NULL;

    int f = IMAGE_IS_RGB16(src->format) ? IMAGE_RGB565A : IMAGE_P8_RGB565A;
    image_t *dst = image_alloc(map->dst_w, map->dst_h, f);
    if(!dst)
        return NULL;
    if(f == IMAGE_P8_RGB565A && !image_copy_palette(src, dst, -1)) {
        image_free(dst);
        return NULL;
    }

    image_clear(dst);
    image_linear(src, dst, map);
    return dst;
}

#endif
