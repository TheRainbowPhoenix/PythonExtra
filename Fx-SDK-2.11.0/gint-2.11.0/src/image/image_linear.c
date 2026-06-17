#include <gint/image.h>
#include <gint/defs/util.h>
#include "fixed.h"

#include <gint/config.h>
#if GINT_RENDER_RGB

void image_linear_rgb16(void *src, void *dst, struct image_linear_map *map);
void image_linear_p8(void *src, void *dst, struct image_linear_map *map);

void image_linear(image_t const *src, image_t *dst,
    struct image_linear_map *map)
{
    if(!image_target(src, dst, NOT_P4, SAME_DEPTH))
        return;

    /* Clip the destination */
    map->dst_w = min(map->dst_w, dst->width);
    map->dst_h = min(map->dst_h, dst->height);

    int drow_u = -map->dx_u * map->dst_w + map->dy_u;
    int drow_v = -map->dx_v * map->dst_w + map->dy_v;

    /* Change dy to mean drow before calling the assembler code */
    map->dy_u = drow_u;
    map->dy_v = drow_v;

    /* Record strides */
    map->src_stride = src->stride;
    map->dst_stride = dst->stride;

    /* Call the assembler implementation */
    if(IMAGE_IS_RGB16(src->format))
        image_linear_rgb16(src->data, dst->data, map);
    else if(IMAGE_IS_P8(src->format))
        image_linear_p8(src->data, dst->data, map);
}

#endif
