#include <gint/image.h>
#include <stdlib.h>
#include <string.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

static void copy_row_rgb16(uint16_t *src, uint16_t *dst, int src_alpha,
    int dst_alpha, int width)
{
    for(int x = 0; x < width; x++) {
        int px = src[x];
        if(px != src_alpha)
            dst[x] = px - (px == dst_alpha);
    }
}

static void copy_row_p8(int8_t *src, int8_t *dst, int src_alpha, int width)
{
    for(int x = 0; x < width; x++) {
        int px = src[x];
        if(px != src_alpha)
            dst[x] = px;
    }
}

void image_vflip(image_t const *src, image_t *dst, bool copy_alpha)
{
    if(!image_target(src, dst, DATA_RW, SAME_DEPTH, SAME_SIZE))
        return;

    int h = src->height;
    void *src_top = src->data;
    void *src_bot = src->data + (h - 1) * src->stride;
    void *dst_top = dst->data;
    void *dst_bot = dst->data + (h - 1) * dst->stride;

    int src_alpha = image_alpha_2(src->format, copy_alpha);
    int dst_alpha = image_alpha_2(dst->format, copy_alpha);

    int row_length = src->stride;
    void *row = malloc(row_length);
    if(!row)
        return;

    for(int y = 0; y < (h + 1) >> 1; y++) {
        memcpy(row, src_top, row_length);

        if(IMAGE_IS_RGB16(src->format)) {
            copy_row_rgb16(src_bot, dst_top, src_alpha, dst_alpha, src->width);
            copy_row_rgb16(row, dst_bot, src_alpha, dst_alpha, src->width);
        }
        else {
            copy_row_p8(src_bot, dst_top, src_alpha, src->width);
            copy_row_p8(row, dst_bot, src_alpha, src->width);
        }

        src_top += src->stride;
        src_bot -= src->stride;
        dst_top += dst->stride;
        dst_bot -= dst->stride;
    }

    free(row);
}

#endif
