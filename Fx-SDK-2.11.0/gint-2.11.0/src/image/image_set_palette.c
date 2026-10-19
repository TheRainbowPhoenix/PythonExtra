#include <gint/image.h>
#include <stdlib.h>

#include <gint/config.h>
#if GINT_RENDER_RGB

void image_set_palette(image_t *img, uint16_t *palette, int size, bool owns)
{
    if(!img || !IMAGE_IS_INDEXED(img->format))
        return;
    if(img->flags & IMAGE_FLAGS_PALETTE_ALLOC)
        free(img->palette);

    img->palette = palette;
    img->color_count = size;

    if(owns)
        img->flags |= IMAGE_FLAGS_PALETTE_ALLOC;
    else
        img->flags &= ~IMAGE_FLAGS_PALETTE_ALLOC;
}

#endif
