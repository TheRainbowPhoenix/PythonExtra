#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

void image_set_pixel(image_t const *img, int x, int y, int value)
{
    if(!image_valid(img))
        return;
    if((unsigned)x >= img->width || (unsigned)y >= img->height)
        return;

    void *data = img->data + y * img->stride;
    uint8_t *data_u8 = data;
    uint16_t *data_u16 = data;

    if(IMAGE_IS_RGB16(img->format)) {
        data_u16[x] = value;
    }
    else if(IMAGE_IS_P8(img->format)) {
        data_u8[x] = value;
    }
    else if(IMAGE_IS_P4(img->format)) {
        if(x & 1)
            data_u8[x >> 1] = (data_u8[x >> 1] & 0xf0) | (value & 0x0f);
        else
            data_u8[x >> 1] = (data_u8[x >> 1] & 0x0f) | (value << 4);
    }
}

#endif
