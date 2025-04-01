//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.objgintfont: Type of gint font for rendering

#ifndef __PYTHONEXTRA_OBJGINTFONT_H
#define __PYTHONEXTRA_OBJGINTFONT_H

#include "py/obj.h"
#include <gint/display.h>

extern const mp_obj_type_t mp_type_gintfont;

/* A raw gint font with its pointers extracted into Python objects, allowing
   manipulation through bytes() and bytearray() methods. The base font is
   [font]. The members [data] and [palette] (which must be bytes, bytearray or
   None) act as overrides for the corresponding fields of [img], which are
   considered garbage/scratch and is constantly updated from the Python objects
   before using the image.

   Particular care should be given to not manipulating bytes and bytearrays in
   ways that cause reallocation, especially when memory is scarce. */
typedef struct _mp_obj_gintfont_t {
    mp_obj_base_t base;             // required as a first member for every MicroPython object
    font_t font;
    mp_obj_t name;
    mp_obj_t blocks;
    mp_obj_t data;
    mp_obj_t glyph_index;
    mp_obj_t glyph_width;
} mp_obj_gintfont_t;

/* Project a gint font object into a standard font structure for use in
   C-API font functions. */
void objgintfont_get(mp_obj_t self_in, font_t *font);

   /* Build a gint font object from a valid font_t structure. */
mp_obj_t objgintfont_make_from_gint_font(font_t const *font);


/* Lower-level font object constructor. */
/* Constructor for fixed_width fonts*/
/*gint.font(title, flags, line_height, grid_height, block_count, glyph_count,
    char_spacing, line_distance, data_blocks, data_glyphs, grid_width, storage_size ) */
mp_obj_t objgintfont_make_monospaced(const mp_obj_type_t *type, mp_obj_t name, int prop, int line_height,
    int data_height, int block_count, int glyph_count, int char_spacing, int line_distance, mp_obj_t blocks,
    mp_obj_t data, int width, int storage_size);

/* Constructor for proportional fonts*/
/*gint.font(name, flags, line_height, grid_height, block_count, glyph_count,
    char_spacing, line_distance, data_blocks, data_glyphs, glyph_index, glyph_width ) */
mp_obj_t objgintfont_make_proportional(const mp_obj_type_t *type, mp_obj_t name, int prop, int line_height,
    int data_height, int block_count, int glyph_count, int char_spacing, int line_distance, mp_obj_t blocks,
    mp_obj_t data, mp_obj_t glyph_index, mp_obj_t glyph_width);

#endif /* __PYTHONEXTRA_OBJGINTFONT_H */