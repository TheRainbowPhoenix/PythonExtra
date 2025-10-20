#include <gint/image.h>
#undef image_target

#include <gint/config.h>
#if GINT_RENDER_RGB

bool image_target(image_t const *src, image_t *dst, ...)
{
    if(!image_valid(src) || !image_valid(dst))
        return false;

    va_list args;
    va_start(args, dst);

    int req = -1;
    while((req = va_arg(args, int)) != IMAGE_TARGET_NONE) {
        if(req == IMAGE_TARGET_NOT_P4 && IMAGE_IS_P4(dst->format))
            return false;
        if(req == IMAGE_TARGET_DATA_RW && (dst->flags & IMAGE_FLAGS_DATA_RO))
            return false;
        if(req == IMAGE_TARGET_PALETTE_RW && (dst->flags &
                IMAGE_FLAGS_PALETTE_RO))
            return false;
        if(req == IMAGE_TARGET_SAME_SIZE &&
                (dst->width < src->width || dst->height < src->height))
            return false;
        if(req == IMAGE_TARGET_SAME_FORMAT && src->format != dst->format)
            return false;
        if(req == IMAGE_TARGET_SAME_DEPTH) {
            if(IMAGE_IS_RGB16(src->format) && IMAGE_IS_RGB16(dst->format))
                continue;
            if(IMAGE_IS_P8(src->format) && IMAGE_IS_P8(dst->format))
                continue;
            if(IMAGE_IS_P4(src->format) && IMAGE_IS_P4(dst->format))
                continue;
            return false;
        }
    }

    va_end(args);
    return true;
}

#endif
