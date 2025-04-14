//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.objgintutils: function used to manipulate gint object and common to multiple submodules

#ifndef __PYTHONEXTRA_OBJGINTUTILS_H
#define __PYTHONEXTRA_OBJGINTUTILS_H

#include "py/obj.h"


mp_obj_t ptr_to_memoryview(void *ptr, int size, int typecode, bool rw);


#endif // __PYTHONEXTRA_OBJGINTUTILS_H