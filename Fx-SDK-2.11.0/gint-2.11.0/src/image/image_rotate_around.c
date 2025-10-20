#include <gint/image.h>
#include "fixed.h"

#include <gint/config.h>
#if GINT_RENDER_RGB

void image_rotate_around(image_t const *src, float angle, bool resize,
    int *center_x, int *center_y, struct image_linear_map *map)
{
    image_rotate_around_scale(src, angle, fconst(1.0), resize, center_x,
        center_y, map);
}

#endif
