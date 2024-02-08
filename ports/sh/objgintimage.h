//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.objgintimage: Type of gint images for rendering and editing

#ifndef __PYTHONEXTRA_OBJGINTIMAGE_H
#define __PYTHONEXTRA_OBJGINTIMAGE_H

#include "py/obj.h"
#include <gint/display.h>

#ifdef FXCG50
#include <gint/image.h>
#endif

extern const mp_obj_type_t mp_type_gintimage;

/* A raw gint image with its pointers extracted into Python objects, allowing
   manipulation through bytes() and bytearray() methods. The base image is
   [img]. The members [data] and [palette] (which must be bytes, bytearray or
   None) act as overrides for the corresponding fields of [img], which are
   considered garbage/scratch and is constantly updated from the Python objects
   before using the image.

   Particular care should be given to not manipulating bytes and bytearrays in
   ways that cause reallocation, especially when memory is scarce. */
typedef struct _mp_obj_gintimage_t {
    mp_obj_base_t base;
    bopti_image_t img;
    mp_obj_t data;
#ifdef FXCG50
    mp_obj_t palette;
#endif
} mp_obj_gintimage_t;

/* Project a gint image object into a standard bopti image structure for use in
   C-API image functions. */
void objgintimage_get(mp_obj_t self_in, bopti_image_t *img);

/* Build a gint image object from a valid bopti image structure. */
mp_obj_t objgintimage_make_from_gint_image(bopti_image_t const *img);

/* Lower-level image object constructor. */
#if defined(FX9860G)
mp_obj_t objgintimage_make(const mp_obj_type_t *type, int profile, int width,
    int height, mp_obj_t data);
#elif defined(FXCG50)
mp_obj_t objgintimage_make(const mp_obj_type_t *type, int format,
    int color_count, int width, int height, int stride, mp_obj_t data,
    mp_obj_t palette);
#endif

#endif /* __PYTHONEXTRA_OBJGINTIMAGE_H */
