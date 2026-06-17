#include <gint/image.h>
#include "fixed.h"

#include <gint/config.h>
#if GINT_RENDER_RGB

void image_scale(image_t const *src, int gamma_x, int gamma_y,
    struct image_linear_map *map)
{
    if(!image_valid(src))
        return;

    int inv_gamma_x = fdiv(fconst(1.0), gamma_x);
    int inv_gamma_y = fdiv(fconst(1.0), gamma_y);

    map->u = fconst(0);
    map->v = fconst(0);
    map->dx_u = inv_gamma_x;
    map->dx_v = 0;
    map->dy_u = 0;
    map->dy_v = inv_gamma_y;

    map->src_w = src->width;
    map->src_h = src->height;
    map->dst_w = fround(src->width * gamma_x);
    map->dst_h = fround(src->height * gamma_y);
}

#endif
