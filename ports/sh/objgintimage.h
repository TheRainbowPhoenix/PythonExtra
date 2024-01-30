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

void objgintimage_get(mp_obj_t self_in, bopti_image_t *img);

#endif /* __PYTHONEXTRA_OBJGINTIMAGE_H */
