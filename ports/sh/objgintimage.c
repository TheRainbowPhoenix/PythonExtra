//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "objgintimage.h"
#include "py/runtime.h"
#include "py/objarray.h"
#include <string.h>

STATIC mp_obj_t ptr_to_memoryview(void *ptr, int size, int typecode, bool rw)
{
    if(ptr == NULL)
        return mp_const_none;
    if(rw)
        typecode |= MP_OBJ_ARRAY_TYPECODE_FLAG_RW;
    return mp_obj_new_memoryview(typecode, size, ptr);
}

#ifdef FX9860G

/* Heuristic to check if the image is read-only or not */
STATIC bool pointer_is_ro(void *data)
{
    uintptr_t addr = (uintptr_t)data;
    return !addr || (addr >= 0x00300000 && addr <= 0x00500000);
}

STATIC int image_data_size(int profile, int width, int height)
{
    int layers = image_layer_count(profile);
    int longwords = (width + 31) >> 5;
    return layers * longwords * height * 4;
}

/* gint.image(profile, width, height, data)
   Keyword labels are allowed but the order must remain the same. */
STATIC mp_obj_t image_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args)
{
    enum { ARG_profile, ARG_width, ARG_height, ARG_data };
    static mp_arg_t const allowed_args[] = {
        { MP_QSTR_profile, MP_ARG_INT | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_data, MP_ARG_OBJ | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args),
        allowed_args, vals);

    int profile   = vals[ARG_profile].u_int;
    int width     = vals[ARG_width].u_int;
    int height    = vals[ARG_height].u_int;
    mp_obj_t data = vals[ARG_data].u_obj;

    return objgintimage_make(type, profile, width, height, data);
}

mp_obj_t objgintimage_make(const mp_obj_type_t *type, int profile, int width,
    int height, mp_obj_t data)
{
    /* The supplied object must implement the buffer protocol */
    mp_buffer_info_t buf;
    if(!mp_get_buffer(data, &buf, MP_BUFFER_READ))
        mp_raise_TypeError("data must be a buffer object");

    /* Size and bounds checks */

    int data_len = MP_OBJ_SMALL_INT_VALUE(mp_obj_len(data));

    if(width <= 0 || height <= 0)
        mp_raise_ValueError("image width/height must be >0");
    if(profile < 0 || profile >= 8)
        mp_raise_ValueError("invalid image profile");

    if(data_len < image_data_size(profile, width, height))
        mp_raise_ValueError("data len() should be >= 4*ceil(w/32)*h*layers");

    /* Construct image! */

    mp_obj_gintimage_t *self = mp_obj_malloc(mp_obj_gintimage_t, type);
    self->img.profile = profile;
    self->img.width   = width;
    self->img.height  = height;
    self->img.data    = NULL;
    self->data        = data;

    return MP_OBJ_FROM_PTR(self);
}

mp_obj_t objgintimage_make_from_gint_image(bopti_image_t const *img)
{
    /* The original image is assumed to be valid. */
    mp_obj_gintimage_t *self = mp_obj_malloc(mp_obj_gintimage_t,
        &mp_type_gintimage);

    memcpy(&self->img, img, sizeof *img);

    int data_size = image_data_size(img->profile, img->width, img->height);
    bool rw = !pointer_is_ro(img->data);
    self->data = ptr_to_memoryview(img->data, data_size, 'B', rw);

    return MP_OBJ_FROM_PTR(self);
}

STATIC void image_print(mp_print_t const *print, mp_obj_t self_in,
    mp_print_kind_t kind)
{
    (void)kind;
    mp_obj_gintimage_t *self = MP_OBJ_TO_PTR(self_in);

    char const *data_str =
        self->data != mp_const_none ? "py" :
        pointer_is_ro(self->img.data) ? "ro" : "rw";

    static char const * const fmt_names[] = {
        "mono", "mono_alpha", "gray", "gray_alpha"
    };
    char const *format_str =
        (self->img.profile < 4) ? fmt_names[self->img.profile] : "?";

    mp_printf(print, "<%s image (%d layers), %dx%d (%s, %d bytes)>",
        format_str, image_layer_count(self->img.profile), self->img.width,
        self->img.height, data_str,
        image_data_size(self->img.profile, self->img.width, self->img.height));
}

STATIC void image_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    if(dest[0] == MP_OBJ_NULL) {
        mp_obj_gintimage_t *self = MP_OBJ_TO_PTR(self_in);
        if(attr == MP_QSTR_format)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.profile);
        else if(attr == MP_QSTR_width)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.width);
        else if(attr == MP_QSTR_height)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.height);
        else if(attr == MP_QSTR_data)
            dest[0] = self->data;
    }
    else {
        mp_raise_msg(&mp_type_AttributeError,
            MP_ERROR_TEXT("gint.image doesn't support changing attributes"));
    }
}

#endif /* FX9860G */

#ifdef FXCG50

/* gint.image(format, color_count, width, height, stride, data, palette)
   Keyword labels are allowed but the order must remain the same. */
STATIC mp_obj_t image_make_new(const mp_obj_type_t *type, size_t n_args,
    size_t n_kw, const mp_obj_t *args)
{
    enum { ARG_format, ARG_color_count, ARG_width, ARG_height, ARG_stride,
           ARG_data, ARG_palette };
    static mp_arg_t const allowed_args[] = {
        { MP_QSTR_format, MP_ARG_INT | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_color_count, MP_ARG_INT | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_stride, MP_ARG_INT | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_data, MP_ARG_OBJ | MP_ARG_REQUIRED,
            {.u_rom_obj = MP_ROM_NONE} },
        { MP_QSTR_palette, MP_ARG_OBJ,
            {.u_rom_obj = MP_ROM_NONE} },
    };
    mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args),
        allowed_args, vals);

    int format       = vals[ARG_format].u_int;
    int color_count  = vals[ARG_color_count].u_int;
    int width        = vals[ARG_width].u_int;
    int height       = vals[ARG_height].u_int;
    int stride       = vals[ARG_stride].u_int;
    mp_obj_t data    = vals[ARG_data].u_obj;
    mp_obj_t palette = vals[ARG_palette].u_obj;

    return objgintimage_make(type, format, color_count, width, height, stride,
        data, palette);
}

mp_obj_t objgintimage_make(const mp_obj_type_t *type, int format,
    int color_count, int width, int height, int stride, mp_obj_t data,
    mp_obj_t palette)
{
    bool has_palette = palette != mp_const_none;

    /* Type checks */
    mp_buffer_info_t buf;
    if(!mp_get_buffer(data, &buf, MP_BUFFER_READ))
        mp_raise_TypeError("data must be a buffer object");
    if(palette != mp_const_none && !mp_get_buffer(palette,&buf,MP_BUFFER_READ))
        mp_raise_TypeError("palette must be None or a buffer object");

    /* Size and bounds checks */

    int data_len = MP_OBJ_SMALL_INT_VALUE(mp_obj_len(data));
    int palette_len =
        has_palette ? MP_OBJ_SMALL_INT_VALUE(mp_obj_len(palette)) : 0;

    if(width <= 0 || height <= 0)
        mp_raise_ValueError("image width/height must be >0");

    if(format < 0 || format >= 7 || format == IMAGE_DEPRECATED_P8)
        mp_raise_ValueError("invalid image format");
    if(data_len < stride * height)
        mp_raise_ValueError("data len() should be >= stride * height");

    if(IMAGE_IS_RGB16(format) && (color_count > 0 || has_palette))
        mp_raise_ValueError("RGB format should have 0 colors and no palette");
    if(IMAGE_IS_P8(format) &&
            (color_count < 1 || color_count > 256 || !has_palette))
        mp_raise_ValueError("P8 format should have palette and 1..256 colors");
    if(IMAGE_IS_P4(format) && (color_count != 16 || !has_palette))
        mp_raise_ValueError("P4 format should have palette and 16 colors");

    if(has_palette && palette_len < 2 * color_count)
        mp_raise_ValueError("palette len() should be >= 2*color_count");

    /* Construct image! */

    mp_obj_gintimage_t *self = mp_obj_malloc(mp_obj_gintimage_t, type);
    self->img.format      = format;
    self->img.color_count = color_count;
    self->img.width       = width;
    self->img.height      = height;
    self->img.stride      = stride;
    self->img.data        = NULL;
    self->img.palette     = NULL;
    self->data            = data;
    self->palette         = palette;
    return MP_OBJ_FROM_PTR(self);
}

mp_obj_t objgintimage_make_from_gint_image(bopti_image_t const *img)
{
    /* The original image is assumed to be valid. */
    mp_obj_gintimage_t *self = mp_obj_malloc(mp_obj_gintimage_t,
        &mp_type_gintimage);

    memcpy(&self->img, img, sizeof *img);

    int data_size = img->stride * img->height;
    int typecode = 'B';
    if(IMAGE_IS_RGB16(img->format)) {
        // TODO: assert stride even
        data_size >>= 1;
        typecode = 'H';
    }
    self->data = ptr_to_memoryview(img->data, data_size, typecode,
        (img->flags & IMAGE_FLAGS_DATA_ALLOC) != 0);

    self->palette = ptr_to_memoryview((void *)img->palette,
        img->color_count, 'H',
        (img->flags & IMAGE_FLAGS_PALETTE_ALLOC) != 0);

    return MP_OBJ_FROM_PTR(self);
}

STATIC char const *flag_string(int ro, int alloc)
{
    static char const *flag_names[] = {
        "rw", "ro", "alloc-rw", "alloc-ro",
    };
    return flag_names[!!ro + 2 * !!alloc];
}

STATIC void image_print(mp_print_t const *print, mp_obj_t self_in,
    mp_print_kind_t kind)
{
    (void)kind;
    mp_obj_gintimage_t *self = MP_OBJ_TO_PTR(self_in);

    int f = self->img.flags;
    char const *data_str = self->data != mp_const_none ? "py" :
        flag_string(f & IMAGE_FLAGS_DATA_RO, f & IMAGE_FLAGS_DATA_ALLOC);
    char const *palette_str = self->palette != mp_const_none ? "py" :
        flag_string(f & IMAGE_FLAGS_PALETTE_RO, f & IMAGE_FLAGS_PALETTE_ALLOC);

    static char const * const fmt_names[] = {
        "RGB565", "RGB565A", "LEGACY_P8",
        "P4_RGB565A", "P8_RGB565", "P8_RGB565A", "P4_RGB565",
    };
    char const *format_str =
        (self->img.format < 7) ? fmt_names[self->img.format] : "?";

    mp_printf(print, "<%s image, %dx%d (%s)",
        format_str, self->img.width, self->img.height, data_str);
    if(IMAGE_IS_INDEXED(self->img.format))
        mp_printf(print, ", %d colors (%s)",
            self->img.color_count, palette_str);
    mp_printf(print, ">");
}

STATIC void image_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
    if(dest[0] == MP_OBJ_NULL) {
        mp_obj_gintimage_t *self = MP_OBJ_TO_PTR(self_in);
        if(attr == MP_QSTR_format)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.format);
        else if(attr == MP_QSTR_flags)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.flags);
        else if(attr == MP_QSTR_color_count)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.color_count);
        else if(attr == MP_QSTR_width)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.width);
        else if(attr == MP_QSTR_height)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.height);
        else if(attr == MP_QSTR_stride)
            dest[0] = MP_OBJ_NEW_SMALL_INT(self->img.stride);
        else if(attr == MP_QSTR_data)
            dest[0] = self->data;
        else if(attr == MP_QSTR_palette)
            dest[0] = self->palette;
    }
    else {
        mp_raise_msg(&mp_type_AttributeError,
            MP_ERROR_TEXT("gint.image doesn't support changing attributes"));
    }
}

#endif /* FXCG50 */

void objgintimage_get(mp_obj_t self_in, bopti_image_t *img)
{
    if(!mp_obj_is_type(self_in, &mp_type_gintimage))
        mp_raise_TypeError(MP_ERROR_TEXT("image must be a gint.image"));

    mp_obj_gintimage_t *self = MP_OBJ_TO_PTR(self_in);
    *img = self->img;

    img->data = NULL;
    if(self->data != mp_const_none) {
        mp_buffer_info_t buf;
        if(!mp_get_buffer(self->data, &buf, MP_BUFFER_READ))
            mp_raise_TypeError("data not a buffer object?!");
        img->data = buf.buf;
    }

#ifdef FXCG50
    img->palette = NULL;
    if(self->palette != mp_const_none) {
        mp_buffer_info_t buf;
        if(!mp_get_buffer(self->palette, &buf, MP_BUFFER_READ))
            mp_raise_TypeError("palette not a buffer object?!");
        img->palette = buf.buf;
    }
#endif
}

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_gintimage,
    MP_QSTR_image,
    MP_TYPE_FLAG_NONE,
    make_new, image_make_new,
    print, image_print,
    attr, image_attr
);
