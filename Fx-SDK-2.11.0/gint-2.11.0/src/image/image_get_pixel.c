#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

int image_get_pixel(image_t const *img, int x, int y)
{
    if((unsigned)x >= img->width || (unsigned)y >= img->height)
        return 0;

    void *data = img->data + y * img->stride;
    uint8_t *data_u8 = data;
    uint16_t *data_u16 = data;

    if(IMAGE_IS_RGB16(img->format)) {
        return data_u16[x];
    }
    else if(IMAGE_IS_P8(img->format)) {
        return (int8_t)data_u8[x];
    }
    else if(IMAGE_IS_P4(img->format)) {
        if(x & 1)
            return data_u8[x >> 1] & 0x0f;
        else
            return data_u8[x >> 1] >> 4;
    }
    return 0;
}

#endif
