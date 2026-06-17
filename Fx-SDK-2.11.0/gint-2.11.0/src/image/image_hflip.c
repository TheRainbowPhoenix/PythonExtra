#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void image_hflip(image_t const *src, image_t *dst, bool copy_alpha)
{
    if(!image_target(src, dst, DATA_RW, SAME_DEPTH, SAME_SIZE))
        return;

    void *src_px = src->data;
    void *dst_px = dst->data;
    int src_alpha = image_alpha_2(src->format, copy_alpha);
    int dst_alpha = image_alpha_2(dst->format, copy_alpha);
    int h = src->height;

    if(IMAGE_IS_RGB16(src->format)) {
        while(h-- > 0) {
            for(int x1 = 0; x1 < (src->width + 1) >> 1; x1++) {
                int x2 = src->width - 1 - x1;
                int px1 = ((uint16_t *)src_px)[x1];
                int px2 = ((uint16_t *)src_px)[x2];

                if(px1 != src_alpha)
                    ((uint16_t *)dst_px)[x2] = px1 - (px1 == dst_alpha);
                if(px2 != src_alpha)
                    ((uint16_t *)dst_px)[x1] = px2 - (px2 == dst_alpha);
            }
            src_px += src->stride;
            dst_px += dst->stride;
        }
    }
    else if(IMAGE_IS_P8(src->format)) {
        while(h-- > 0) {
            for(int x1 = 0; x1 < (src->width + 1) >> 1; x1++) {
                int x2 = src->width - 1 - x1;

                int px1 = ((int8_t *)src_px)[x1];
                int px2 = ((int8_t *)src_px)[x2];

                if(px1 != src_alpha)
                    ((int8_t *)dst_px)[x2] = px1;
                if(px2 != src_alpha)
                    ((int8_t *)dst_px)[x1] = px2;
            }
            src_px += src->stride;
            dst_px += dst->stride;
        }
    }
}

#endif
