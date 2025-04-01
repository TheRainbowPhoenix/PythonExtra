//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "py/objarray.h"
#include "py/obj.h"

mp_obj_t ptr_to_memoryview(void *ptr, int size, int typecode, bool rw)
{
    if(ptr == NULL)
        return mp_const_none;
    if(rw)
        typecode |= MP_OBJ_ARRAY_TYPECODE_FLAG_RW;
    return mp_obj_new_memoryview(typecode, size, ptr);
}