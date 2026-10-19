#include <gint/image.h>
#include <gint/defs/util.h>
#include <stdlib.h>
#include <string.h>

#include <gint/config.h>
#if GINT_RENDER_RGB

bool image_alloc_palette(image_t *img, int size)
{
    if(!img || !IMAGE_IS_INDEXED(img->format))
        return false;
    if(img->flags & IMAGE_FLAGS_PALETTE_ALLOC)
        free(img->palette);

    if(IMAGE_IS_P8(img->format)) {
        size = (size <= 0) ? 256 : min(size, 256);
    }
    if(IMAGE_IS_P4(img->format)) {
        size = 16;
    }

    img->palette = calloc(size, 2);
    img->color_count = 0;
    img->flags &= ~IMAGE_FLAGS_PALETTE_ALLOC;

    if(!img->palette)
        return false;

    memset(img->palette, 0, 2*size);
    img->color_count = size;
    img->flags |= IMAGE_FLAGS_PALETTE_ALLOC;
    return true;
}

#endif
