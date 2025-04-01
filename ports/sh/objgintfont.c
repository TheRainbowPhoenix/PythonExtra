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

STATIC mp_obj_t font_make_new(const mp_obj_type_t *type, size_t n_args,
  size_t n_kw, const mp_obj_t *args)
{
  enum { ARG_name, ARG_prop, ARG_line_height, ARG_data_height,
         ARG_block_count, ARG_glyph_count, ARG_char_spacing, ARG_line_distance, ARG_blocks, AGRS_data,
         ARG_width, ARG_storage_size, ARG_glyph_index, ARG_glyph_width };

  static mp_arg_t const allowed_args[] = {
      { MP_QSTR_name, MP_ARG_OBJ | MP_ARG_REQUIRED,     // TODO Check the type corresponding to string (seems not to be listed)
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_prop, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_line_height, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_data_height, MP_ARG_INT | MP_ARG_REQUIRED,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_block_count, MP_ARG_INT | MP_ARG_REQUIRED,
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
      /* for monospace fonts */
      { MP_QSTR_width, MP_ARG_INT,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_storage_size, MP_ARG_INT,
          {.u_rom_obj = MP_ROM_NONE} },
      /* for proportional fonts */
      { MP_QSTR_glyph_index, MP_ARG_OBJ,
          {.u_rom_obj = MP_ROM_NONE} },
      { MP_QSTR_glyph_width, MP_ARG_OBJ,
          {.u_rom_obj = MP_ROM_NONE} },
  };

  mp_arg_val_t vals[MP_ARRAY_SIZE(allowed_args)];
  mp_arg_parse_all_kw_array(n_args, n_kw, args, MP_ARRAY_SIZE(allowed_args),
      allowed_args, vals);

  mp_obj_t name    = vals[ARG_name].u_obj;
  int prop         = vals[ARG_prop].u_int;
  int line_height  = vals[ARG_line_height].u_int;
  int data_height  = vals[ARG_data_height].u_int;
  int block_count  = vals[ARG_block_count].u_int;
  int glyph_count  = vals[ARG_glyph_count].u_int;
  int char_spacing = vals[ARG_char_spacing].u_int;
  int line_distance= vals[ARG_line_distance].u_int;
  mp_obj_t blocks  = vals[ARG_blocks].u_obj;
  mp_obj_t data    = vals[AGRS_data].u_obj;

  if (prop == 1)
  {
    int width = vals[ARG_width].u_int;
    int storage_size = vals[ARG_storage_size].u_int;
    return objgintfont_make_monospaced(type, name, prop, line_height, data_height, block_count, glyph_count, char_spacing,
                    line_distance, blocks, data, width, storage_size);
  }
  else
  {
    mp_obj_t glyph_index = vals[ARG_glyph_index].u_obj;
    mp_obj_t glyph_width = vals[ARG_glyph_width].u_obj;
    return objgintfont_make_proportional(type, name, prop, line_height, data_height, block_count, glyph_count, char_spacing,
                    line_distance, blocks, data, glyph_index, glyph_width);
  }
}


/* Build a gint font object from a valid font_t structure. */
mp_obj_t objgintfont_make_from_gint_font(font_t const *font)
{
  mp_obj_gintfont_t *self = mp_obj_malloc(mp_obj_gintfont_t,
    &mp_type_gintfont);

  memcpy(&self->font, font, sizeof *font);

  int name_size = strlen(font->name);
  int block_size = font->block_count; //TODO check if correct (not clear how this size is computed) block size
  int data_size  = font->glyph_count; //TODO check if correct (not clear how this size is computed) data_size
  int gidx_size  = font->glyph_count; //TODO check if correct (not clear how this size is computed) glyph_index_size
  int gwdt_size  = font->glyph_count; //TODO check if correct (not clear how this size is computed) glyph_width_size

  self->name = ptr_to_memoryview(font->name, name_size, 'B', false);          //name is char (=uint8_t) values
  self->blocks = ptr_to_memoryview(font->blocks, block_size, 'L', false);     //blocks is uint32_t values
  self->data = ptr_to_memoryview(font->data, data_size, 'L', false);          //data is uint32_t values

  if (font->prop)
  {
    self->glyph_index = ptr_to_memoryview(font->glyph_index, gidx_size, 'H', false);  // glyph_index is uint16_t values
    self->glyph_width = ptr_to_memoryview(font->glyph_width, gwdt_size, 'B', false);  // glyph_width is uint8_t values
  }

  return MP_OBJ_FROM_PTR(self);
}



STATIC void font_print(mp_print_t const *print, mp_obj_t self_in,
  mp_print_kind_t kind)
{

  (void)kind;
  mp_obj_gintfont_t *self = MP_OBJ_TO_PTR(self_in);

  const char* title = self->font.name;
  int prop = self->font.prop;
  int line_height = self->font.line_height;
  int data_height = self->font.data_height;
  int block_count = self->font.block_count;
  int glyph_count = self->font.glyph_count;
  int char_spacing = self->font.char_spacing;
  int line_distance = self->font.line_distance;

  mp_printf( print, "<%s font - flags=%d - height=%d - dat_height=%d - blk_cnt=%d - glp_cnt=%d - chr_spc=%d - lne_dts=%d >",
      title, prop, line_height, data_height, block_count, glyph_count, char_spacing, line_distance );

  if (prop) // if proportional is set to one
    mp_printf( print, "<Proportional font>");
  else
    mp_printf( print, "<Fixed size font>");
}

STATIC void font_attr(mp_obj_t self_in, qstr attr, mp_obj_t *dest)
{

  if(dest[0] == MP_OBJ_NULL) {
      mp_obj_gintfont_t *self = MP_OBJ_TO_PTR(self_in);
      if(attr == MP_QSTR_title)
          dest[0] = MP_OBJ_NEW_QSTR(self->font.name);
      else if(attr == MP_QSTR_prop)
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
      else if(attr == MP_QSTR_blocks)
          dest[0] = self->blocks;
      else if(attr == MP_QSTR_data)
          dest[0] = self->data;
      /* for fixed size only */
      else if(attr == MP_QSTR_width)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.width);
      else if(attr == MP_QSTR_storage_size)
          dest[0] = MP_OBJ_NEW_SMALL_INT(self->font.storage_size);
      /* for proportional only */
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


/* Lower-level font object constructor. */
/* Constructor for fixed_width fonts*/
/*gint.font(title, flags, line_height, grid_height, block_count, glyph_count,
    char_spacing, line_distance, data_blocks, data_glyphs, grid_width, storage_size ) */
mp_obj_t objgintfont_make_monospaced(const mp_obj_type_t *type, mp_obj_t name, int prop, int line_height,
      int data_height, int block_count, int glyph_count, int char_spacing, int line_distance, mp_obj_t blocks,
      mp_obj_t data, int width, int storage_size)
{
  // TODO : add verifications here for validity of the font data

    mp_obj_gintfont_t *self = mp_obj_malloc(mp_obj_gintfont_t, type);
    self->name                  = name;
    self->font.name             = NULL;
    self->font.prop             = prop;
    self->font.line_height      = line_height;
    self->font.data_height      = data_height;
    self->font.block_count      = block_count;
    self->font.glyph_count      = glyph_count;
    self->font.char_spacing     = char_spacing;
    self->font.line_distance    = line_distance;
    self->font.blocks           = NULL;
    self->font.data             = NULL;
    self->font.glyph_index      = NULL;
    self->font.glyph_width      = NULL;
    self->font.width            = width;
    self->font.storage_size     = storage_size;
    self->blocks                = blocks;
    self->data                  = data;
    self->glyph_index           = NULL;
    self->glyph_width           = NULL;
    return MP_OBJ_FROM_PTR(self);
}
  
  /* Constructor for proportional fonts*/
  /*gint.font(name, flags, line_heignt, grida_height, block_count, glyph_count,
      char_spacing, line_distance, data_blocks, data_glyphs, glyph_index, glyph_width ) */
mp_obj_t objgintfont_make_proportional(const mp_obj_type_t *type, mp_obj_t name, int prop, int line_height,
        int data_height, int block_count, int glyph_count, int char_spacing, int line_distance, mp_obj_t blocks,
        mp_obj_t data, mp_obj_t glyph_index, mp_obj_t glyph_width)
{
    // TODO : add verifications here for validity of the font data

    mp_obj_gintfont_t *self = mp_obj_malloc(mp_obj_gintfont_t, type);
    self->name                  = name;
    self->font.name             = NULL;
    self->font.prop             = prop;
    self->font.line_height      = line_height;
    self->font.data_height      = data_height;
    self->font.block_count      = block_count;
    self->font.glyph_count      = glyph_count;
    self->font.char_spacing     = char_spacing;
    self->font.line_distance    = line_distance;
    self->font.blocks           = NULL;
    self->font.data             = NULL;
    self->font.glyph_index      = NULL;
    self->font.glyph_width      = NULL;
    self->blocks                = blocks;
    self->data                  = data;
    self->glyph_index           = glyph_index;
    self->glyph_width           = glyph_width;
    return MP_OBJ_FROM_PTR(self);
}

/* Project a gint font object into a standard font structure for use in
   C-API font functions. */
void objgintfont_get(mp_obj_t self_in, font_t *font)
{
  if(!mp_obj_is_type(self_in, &mp_type_gintfont))
  mp_raise_TypeError(MP_ERROR_TEXT("font must be a gint.font"));

  mp_obj_gintfont_t *self = MP_OBJ_TO_PTR(self_in);
  *font = self->font;

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






MP_DEFINE_CONST_OBJ_TYPE(
  mp_type_gintfont,
  MP_QSTR_font,
  MP_TYPE_FLAG_NONE,
  make_new, font_make_new,
  print, font_print,
  attr, font_attr
);
