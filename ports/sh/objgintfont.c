//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.objgintfont: Type of gint font for rendering


#include "objgintfont.h"
#include "py/runtime.h"
#include <string.h>
#include "objgintutils.h"
#include "debug.h"

static mp_obj_t font_make_new(const mp_obj_type_t *type, size_t n_args,
  size_t n_kw, const mp_obj_t *args)
{
  enum { ARG_prop, ARG_line_height, ARG_data_height, ARG_block_count,
         ARG_glyph_count, ARG_char_spacing, ARG_line_distance, ARG_blocks,
         AGRS_data, ARG_width, ARG_storage_size, ARG_glyph_index,
         ARG_glyph_width, ARG_name };

  static mp_arg_t const allowed_args[] = {
      { MP_QSTR_prop, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_line_height, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_data_height, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_block_count, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_glyph_count, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_char_spacing, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_line_distance, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_blocks, MP_ARG_OBJ | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_data, MP_ARG_OBJ | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      /* for monospace fonts */
      { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_storage_size, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      /* for proportional fonts */
      { MP_QSTR_glyph_index, MP_ARG_OBJ | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_glyph_width, MP_ARG_OBJ | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_name, MP_ARG_OBJ,
          {.u_rom_obj = MP_OBJ_NEW_QSTR(MP_QSTR_)} },
  };

  mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all_kw_array(
    n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args), allowed_args, vals);

  int prop         = vals[ARG_prop].u_int;
  int line_height  = vals[ARG_line_height].u_int;
  int data_height  = vals[ARG_data_height].u_int;
  int block_count  = vals[ARG_block_count].u_int;
  int glyph_count  = vals[ARG_glyph_count].u_int;
  int char_spacing = vals[ARG_char_spacing].u_int;
  int line_distance= vals[ARG_line_distance].u_int;
  mp_obj_t blocks  = vals[ARG_blocks].u_obj;
  mp_obj_t data    = vals[AGRS_data].u_obj;
  mp_obj_t name    = vals[ARG_name].u_obj;

  if (prop == 0) {
    int width = vals[ARG_width].u_int;
    int storage_size = vals[ARG_storage_size].u_int;
    return objgintfont_make_monospaced(type, prop, line_height, data_height,
      block_count, glyph_count, char_spacing, line_distance, blocks, data,
      width, storage_size, name);
  }
  else {
    mp_obj_t glyph_index = vals[ARG_glyph_index].u_obj;
    mp_obj_t glyph_width = vals[ARG_glyph_width].u_obj;
    return objgintfont_make_proportional(type, prop, line_height, data_height,
      block_count, glyph_count, char_spacing, line_distance, blocks, data,
      glyph_index, glyph_width, name);
  }
}

mp_obj_t objgintfont_make_from_gint_font(font_t const *font)
{
  mp_obj_gintfont_t *self = mp_obj_malloc(mp_obj_gintfont_t, &mp_type_gintfont);
  memcpy(&self->font, font, sizeof *font);
  int data_size=0;

  /* One u32 (L) for each block (combines offset and size) */
  self->blocks = ptr_to_memoryview(font->blocks, font->block_count, 'L', false);

  if (font->prop) {
    /* One u8 (B) for each glyph */
    int gwdt_size = font->glyph_count;
    self->glyph_width = ptr_to_memoryview(font->glyph_width, gwdt_size, 'B', false);
    /* Index has one u16 (H) entry for each 8 glyphs, rounding up */
    int gidx_size = (font->glyph_count + 7) >> 3;
    self->glyph_index = ptr_to_memoryview(font->glyph_index, gidx_size, 'H', false);
    /* Data is longword-padded for each glyph */
    for(uint g = 0; g < font->glyph_count; g++)
      data_size += (font->glyph_width[g] * font->data_height + 31) / 32;
  }
  // given by (grid.w * grid.h + 31)/32 in the case of monospaced fonts
  else data_size = font->storage_size;

  self->name = font->name ? mp_obj_new_str_from_cstr(font->name)
                          : MP_OBJ_NEW_QSTR(MP_QSTR_);
  self->data = ptr_to_memoryview(font->data, data_size, 'L', false);
  return MP_OBJ_FROM_PTR(self);
}

static void font_print(
  mp_print_t const *print, mp_obj_t self_in, mp_print_kind_t kind)
{
  (void)kind;
  mp_obj_gintfont_t *self = MP_OBJ_TO_PTR(self_in);

  char const *name = mp_obj_str_get_str(self->name);
  int prop = self->font.prop;
  int line_height = self->font.line_height;
  int data_height = self->font.data_height;
  int block_count = self->font.block_count;
  int glyph_count = self->font.glyph_count;
  int char_spacing = self->font.char_spacing;
  int line_distance = self->font.line_distance;

  mp_printf(print, "<font '%s', %s, height %d/%d, %d blocks, %d glyphs, "
    "char spacing %d, line height %d>",
    name, prop ? "proportional" : "fixed-width", line_height, data_height,
    block_count, glyph_count, char_spacing, line_distance);
}

static void font_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{
  if(dest[0] == MP_OBJ_NULL) {
      mp_obj_gintfont_t *self = MP_OBJ_TO_PTR(self_in);
      if(attr == MP_QSTR_prop)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.prop);
      else if(attr == MP_QSTR_line_height)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.line_height);
      else if(attr == MP_QSTR_data_height)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.data_height);
      else if(attr == MP_QSTR_block_count)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.block_count);
      else if(attr == MP_QSTR_glyph_count)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.glyph_count);
      else if(attr == MP_QSTR_char_spacing)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.char_spacing);
      else if(attr == MP_QSTR_line_distance)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.line_distance);
      else if(attr == MP_QSTR_name)
          dest[0] = self->name;
      else if(attr == MP_QSTR_blocks)
          dest[0] = self->blocks;
      else if(attr == MP_QSTR_data)
          dest[0] = self->data;
      /* used for monospaced fonts only */
      else if(attr == MP_QSTR_width)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.width);
      else if(attr == MP_QSTR_storage_size)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.storage_size);
      /* used for proportional fonts only */
      else if(attr == MP_QSTR_glyph_index)
          dest[0] = self->glyph_index;
      else if(attr == MP_QSTR_glyph_width)
          dest[0] = self->glyph_width;
  }
  else {
      mp_raise_msg(&mp_type_AttributeError,
          MP_ERROR_TEXT("gint.font doesn't support changing attributes"));
  }
}

mp_obj_t objgintfont_make_monospaced(
    const mp_obj_type_t *type, int prop, int line_height, int data_height,
    int block_count, int glyph_count, int char_spacing, int line_distance,
    mp_obj_t blocks, mp_obj_t data, int width, int storage_size, mp_obj_t name)
{
    mp_obj_gintfont_t *self = mp_obj_malloc(mp_obj_gintfont_t, type);
    self->font.name             = NULL;
    self->font.prop             = prop;
    self->font.line_height      = line_height;
    self->font.data_height      = data_height;
    self->font.block_count      = block_count;
    self->font.glyph_count      = glyph_count;
    self->font.char_spacing     = char_spacing;
    self->font.line_distance    = line_distance;
    self->font.blocks           = blocks;
    self->font.data             = data;
    self->font.glyph_index      = NULL;
    self->font.glyph_width      = NULL;
    self->font.width            = width;
    self->font.storage_size     = storage_size;
    self->name                  = name;
    self->blocks                = blocks;
    self->data                  = data;
    self->glyph_index           = mp_const_none;
    self->glyph_width           = mp_const_none;
    return MP_OBJ_FROM_PTR(self);
}
  
mp_obj_t objgintfont_make_proportional(
    const mp_obj_type_t *type, int prop, int line_height, int data_height,
    int block_count, int glyph_count, int char_spacing, int line_distance,
    mp_obj_t blocks, mp_obj_t data, mp_obj_t glyph_index, mp_obj_t glyph_width,
    mp_obj_t name)
{
    mp_obj_gintfont_t *self = mp_obj_malloc(mp_obj_gintfont_t, type);
    self->font.name             = NULL;
    self->font.prop             = prop;
    self->font.line_height      = line_height;
    self->font.data_height      = data_height;
    self->font.block_count      = block_count;
    self->font.glyph_count      = glyph_count;
    self->font.char_spacing     = char_spacing;
    self->font.line_distance    = line_distance;
    self->font.blocks           = blocks;
    self->font.data             = data;
    self->font.width            = 0;
    self->font.storage_size     = 0;
    self->font.glyph_index      = glyph_index;
    self->font.glyph_width      = glyph_width;
    self->name                  = name;
    self->blocks                = blocks;
    self->data                  = data;
    self->glyph_index           = glyph_index;
    self->glyph_width           = glyph_width;
    return MP_OBJ_FROM_PTR(self);
}

void objgintfont_get(mp_obj_t self_in, font_t *font)
{
  if(!mp_obj_is_type(self_in, &mp_type_gintfont))
    mp_raise_TypeError(MP_ERROR_TEXT("font must be a gint.font"));

  mp_obj_gintfont_t *self = MP_OBJ_TO_PTR(self_in);
  *font = self->font;

  // skipping name

  font->blocks = NULL;
  if(self->blocks != mp_const_none) {
    mp_buffer_info_t buf;
    if(!mp_get_buffer(self->blocks, &buf, MP_BUFFER_READ))
        mp_raise_TypeError("data_blocks not a buffer object?!");
    font->blocks = buf.buf;
  }

  font->data = NULL;
  if(self->data != mp_const_none) {
    mp_buffer_info_t buf;
    if(!mp_get_buffer(self->data, &buf, MP_BUFFER_READ))
        mp_raise_TypeError("data_glyphs not a buffer object?!");
    font->data = buf.buf;
  }

  if (self->font.prop)
  {
    font->glyph_index = NULL;
    if(self->glyph_index != mp_const_none) {
      mp_buffer_info_t buf;
      if(!mp_get_buffer(self->glyph_index, &buf, MP_BUFFER_READ))
          mp_raise_TypeError("data_index not a buffer object?!");
      font->glyph_index = buf.buf;
    }

    font->glyph_width = NULL;
    if(self->glyph_width != mp_const_none) {
      mp_buffer_info_t buf;
      if(!mp_get_buffer(self->glyph_width, &buf, MP_BUFFER_READ))
          mp_raise_TypeError("data_width not a buffer object?!");
      font->glyph_width = buf.buf;
    }
  }
}

MP_DEFINE_CONST_OBJ_TYPE(
  mp_type_gintfont,
  MP_QSTR_font,
  MP_TYPE_FLAG_NONE,
  make_new, font_make_new,
  print, font_print,
  attr, font_attr
);
