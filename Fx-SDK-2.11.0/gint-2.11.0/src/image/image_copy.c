#include <gint/image.h>
#include <gint/defs/util.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void image_copy(image_t const *src, image_t *dst, bool copy_alpha)
{
    if(!image_target(src, dst, DATA_RW))
        return;
    if(!IMAGE_IS_ALPHA(src->format))
        copy_alpha = true;

    /* Clip the input to match the size of the output */
    int w = min(src->width, dst->width);
    int h = min(src->height, dst->height);
    if(w <= 0 || h <= 0)
        return;

    void *src_px = src->data;
    void *dst_px = dst->data;
    int src_alpha = image_alpha_2(src->format, copy_alpha);
    int dst_alpha = image_alpha_2(dst->format, copy_alpha);

    if(IMAGE_IS_RGB16(src->format) && IMAGE_IS_RGB16(dst->format)) {
        do {
            for(int x = 0; x < w; x++) {
                int px = ((uint16_t *)src_px)[x];
                if(px != src_alpha) {
                    /* Don't copy opaque pixels of value 0x0001 into an RGB565A
                       array. We can use -= which is faster (subc) without
                       changing the visuals because dst_alpha != 0. */
                    ((uint16_t *)dst_px)[x] = px - (px == dst_alpha);
                }
            }
            src_px += src->stride;
            dst_px += dst->stride;
        } while(--h > 0);
    }
    else if(IMAGE_IS_P8(src->format) && IMAGE_IS_RGB16(dst->format)) {
        uint16_t *palette = src->palette + 128;

        do {
            for(int x = 0; x < w; x++) {
                int px = ((int8_t *)src_px)[x];
                if(px != src_alpha) {
                    px = palette[px];
                    ((uint16_t *)dst_px)[x] = px - (px == dst_alpha);
                }
            }
            src_px += src->stride;
            dst_px += dst->stride;
        } while(--h > 0);
    }
    else if(IMAGE_IS_P8(src->format) && IMAGE_IS_P8(dst->format)) {
        do {
            for(int x = 0; x < w; x++) {
                int px = ((int8_t *)src_px)[x];
                if(px != src_alpha)
                    ((int8_t *)dst_px)[x] = px;
            }
            src_px += src->stride;
            dst_px += dst->stride;
        } while(--h > 0);
    }
    else if(IMAGE_IS_P8(src->format) && IMAGE_IS_P4(dst->format)) {
        do {
            for(int x = 0; x < w; x++) {
                int px = ((int8_t *)src_px)[x];
                if(px != src_alpha) {
                    uint8_t *cell = dst_px + (x >> 1);
                    if(x & 1)
                        *cell = (*cell & 0xf0) | (px & 0x0f);
                    else
                        *cell = (*cell & 0x0f) | (px << 4);
                }
            }
            src_px += src->stride;
            dst_px += dst->stride;
        } while(--h > 0);
    }
    else if(IMAGE_IS_P4(src->format) && IMAGE_IS_P4(dst->format)) {
        do {
            for(int x = 0; x < w; x++) {
                int px = ((uint8_t *)src_px)[x >> 1];
                px = (x & 1) ? (px & 0x0f) : (px >> 4);
                if(px != src_alpha) {
                    uint8_t *cell = dst_px + (x >> 1);
                    if(x & 1)
                        *cell = (*cell & 0xf0) | (px & 0x0f);
                    else
                        *cell = (*cell & 0x0f) | (px << 4);
                }
            }
            src_px += src->stride;
            dst_px += dst->stride;
        } while(--h > 0);
    }
    else if(IMAGE_IS_P4(src->format) && IMAGE_IS_P8(dst->format)) {
        do {
            for(int x = 0; x < w; x++) {
                int px = ((uint8_t *)src_px)[x >> 1];
                px = (x & 1) ? (px & 0x0f) : (px >> 4);
                if(px != src_alpha)
                    ((int8_t *)dst_px)[x] = px;
            }
            src_px += src->stride;
            dst_px += dst->stride;
        } while(--h > 0);
    }
    else if(IMAGE_IS_P4(src->format) && IMAGE_IS_RGB16(dst->format)) {
        do {
            for(int x = 0; x < w; x++) {
                int px = ((uint8_t *)src_px)[x >> 1];
                px = (x & 1) ? (px & 0x0f) : (px >> 4);
                if(px != src_alpha) {
                    px = src->palette[px];
                    ((uint16_t *)dst_px)[x] = px - (px == dst_alpha);
                }
            }
            src_px += src->stride;
            dst_px += dst->stride;
        } while(--h > 0);
    }
}

#endif
