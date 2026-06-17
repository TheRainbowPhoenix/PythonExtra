#include <gint/image.h>
#include <math.h>
#include "fixed.h"

#include <gint/config.h>
#if GINT_RENDER_RGB

void image_rotate_around_scale(image_t const *src, float alpha, int gamma,
    bool resize, int *center_x, int *center_y, struct image_linear_map *map)
{
    if(!image_valid(src))
        return;

    map->src_w = src->width;
    map->src_h = src->height;

    /* Compute the rotation basis */
    int cos_alpha = fconst(cosf(alpha));
    int sin_alpha = fconst(sinf(alpha));
    int inv_gamma = fdiv(fconst(1.0), gamma);

    map->dx_u = fmul(cos_alpha, inv_gamma);
    map->dx_v = fmul(sin_alpha, inv_gamma);

    map->dy_u = -fmul(sin_alpha, inv_gamma);
    map->dy_v =  fmul(cos_alpha, inv_gamma);

    /* Don't try to resize cleanly; just make the longest diagonal the width if
       [resize=true] to make sure everything fits */
    if(resize) {
        int diag = isqrt(src->width * src->width + src->height * src->height);
        map->dst_w = fround(gamma * diag);
        map->dst_h = fround(gamma * diag);
    }
    else {
        map->dst_w = fround(gamma * src->width);
        map->dst_h = fround(gamma * src->height);
    }

    /* Compute the new location of the anchor relative to the image center.
       This is found by a neat trick: rotate it with the same angle */
    int ax = *center_x - map->src_w / 2;
    int ay = *center_y - map->src_h / 2;

    int ax2 = fround(fmul(gamma,  cos_alpha * ax + sin_alpha * ay));
    int ay2 = fround(fmul(gamma, -sin_alpha * ax + cos_alpha * ay));

    int new_center_x = ax2 + map->dst_w / 2;
    int new_center_y = ay2 + map->dst_h / 2;

    /* Finally, determine the initial value of (u,v). We now that it evaluates
       to (center_x, center_y) when on the new center point (new_center_x,
       new_center_y); apply the difference accordingly. */
    map->u = fconst(*center_x)
           - map->dx_u * new_center_x
           - map->dy_u * new_center_y;
    map->v = fconst(*center_y)
           - map->dx_v * new_center_x
           - map->dy_v * new_center_y;

    *center_x = new_center_x;
    *center_y = new_center_y;
}

#endif
