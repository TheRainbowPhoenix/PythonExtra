#include <gint/image.h>
#include <gint/display.h>
#include <string.h>
#undef image_sub

#include <gint/config.h>
#if GINT_RENDER_RGB

static image_t image_sub_default;

image_t *image_sub(image_t const *src, int left, int top, int w, int h,
    image_t *dst)
{
    if(!dst)
        dst = &image_sub_default;
    if(w < 0)
        w = src->width - left;
    if(h < 0)
        h = src->height - top;

    struct gint_image_box box = { 0, 0, w, h, left, top };
    struct dwindow in_window = { 0, 0, w, h };
    if(!image_valid(src) || IMAGE_IS_P4(src->format) ||
       !gint_image_clip_input(src, &box, &in_window)) {
        memset(dst, 0, sizeof *dst);
        return dst;
    }

    int const ro_flags = IMAGE_FLAGS_DATA_RO | IMAGE_FLAGS_PALETTE_RO;
    dst->format = src->format;
    dst->flags = src->flags & ro_flags;
    dst->color_count = src->color_count;
    dst->width = box.w;
    dst->height = box.h;
    dst->stride = src->stride;

    if(IMAGE_IS_RGB16(src->format))
        dst->data = src->data + box.top * src->stride + 2 * box.left;
    else if(IMAGE_IS_P8(src->format))
        dst->data = src->data + box.top * src->stride + box.left;

    dst->palette = src->palette;
    return dst;
}

#endif
