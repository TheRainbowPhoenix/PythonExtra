#include <gint/image.h>
#include <gint/config.h>
#if GINT_RENDER_RGB

int image_data_size(image_t const *img)
{
    return img->stride * img->height;
}

#endif
