//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.modtime: Custom extensions to the `time` module

#include <time.h>
#include "py/runtime.h"

static mp_obj_t time_monotonic(void) {
    // TODO: Use libprof instead
    uint64_t ms = ((uint64_t)clock() * 1000000000) / CLOCKS_PER_SEC;
    return mp_obj_new_float((double)ms);
}
MP_DEFINE_CONST_FUN_OBJ_0(mp_time_monotonic_obj, time_monotonic);

static mp_obj_t mp_time_time_get(void) {
    mp_float_t seconds = (mp_float_t)rtc_ticks() / 128;
    return mp_obj_new_float(seconds);
}

#define MICROPY_PY_TIME_EXTRA_GLOBALS \
    { MP_ROM_QSTR(MP_QSTR_monotonic), MP_ROM_PTR(&mp_time_monotonic_obj) },
