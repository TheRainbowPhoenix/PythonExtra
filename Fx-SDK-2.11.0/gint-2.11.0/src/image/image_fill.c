#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void image_fill(image_t *img, int value)
{
    if(!image_target(img, img, NOT_P4, DATA_RW))
        return;

    void *img_px = img->data;

    if(IMAGE_IS_RGB16(img->format)) {
        for(int y = 0; y < img->height; y++) {
            for(int x = 0; x < img->width; x++) {
                ((uint16_t *)img_px)[x] = value;
            }
            img_px += img->stride;
        }
    }
    else if(IMAGE_IS_P8(img->format)) {
        for(int y = 0; y < img->height; y++) {
            for(int x = 0; x < img->width; x++) {
                ((int8_t *)img_px)[x] = value;
            }
            img_px += img->stride;
        }
    }
}

#endif
