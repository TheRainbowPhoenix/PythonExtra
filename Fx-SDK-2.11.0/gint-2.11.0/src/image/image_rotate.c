#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void image_rotate(image_t const *src, float angle, bool resize,
    struct image_linear_map *map)
{
    if(!image_valid(src))
        return;

    int center_x = src->width / 2;
    int center_y = src->height / 2;

    image_rotate_around(src, angle, resize, &center_x, &center_y, map);
}

#endif
