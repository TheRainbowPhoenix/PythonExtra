#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

bool image_valid(image_t const *img)
{
    if(!img)
        return false;

    if(IMAGE_IS_RGB16(img->format)) {
        return (img->data != NULL);
    }
    if(IMAGE_IS_P8(img->format) || IMAGE_IS_P4(img->format)) {
        return (img->data != NULL) && (img->palette != NULL) &&
            (img->color_count != 0);
    }

    /* Invalid format */
    return false;
}

#endif
