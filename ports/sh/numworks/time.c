//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.time: `gint` module
//
// This module aims to wrap commonly-used gint functtimes (not all APIs are
// considered relevant for high-level Python development).
//---

#include "console.h"
#include "py/objtuple.h"
#include "py/runtime.h"
#include <gint/clock.h>
#include <gint/timer.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define FUN_0(NAME) MP_DEFINE_CONST_FUN_OBJ_0(time_##NAME##_obj, time_##NAME)
#define FUN_1(NAME) MP_DEFINE_CONST_FUN_OBJ_1(time_##NAME##_obj, time_##NAME)
#define FUN_2(NAME) MP_DEFINE_CONST_FUN_OBJ_2(time_##NAME##_obj, time_##NAME)
#define FUN_3(NAME) MP_DEFINE_CONST_FUN_OBJ_3(time_##NAME##_obj, time_##NAME)
#define FUN_VAR(NAME, MIN)                                                     \
  MP_DEFINE_CONST_FUN_OBJ_VAR(time_##NAME##_obj, MIN, time_##NAME)
#define FUN_BETWEEN(NAME, MIN, MAX)                                            \
  MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(time_##NAME##_obj, MIN, MAX, time_##NAME)



extern bool is_timered;
extern unsigned int timer_altered[9];

static uint64_t tickmono = 0;


static int monotonic_callback(void) {
  tickmono++;
  return TIMER_CONTINUE;
}


STATIC mp_obj_t time___init__(void)
{ 
    tickmono = 0;

    int t = timer_configure(TIMER_TMU, 1000, GINT_CALL(monotonic_callback));
    if (t >= 0)
    {
      timer_start(t);
      is_timered = true;    // there is a timer altered from this module
      timer_altered[t] = 1; // we put the corresponding timer at 1 to identify it
    }

    return mp_const_none;
}

/* <gint/keyboard.h> */

STATIC mp_obj_t time_sleep(mp_obj_t arg1)
{
    mp_float_t duration = mp_obj_get_float(arg1);

    uint64_t length = (uint64_t)(duration * 1000000.0f); // duration is in seconds and length in µs

    sleep_us(length);

    return mp_const_none;
}

STATIC mp_obj_t time_monotonic(void)
{
  float value = (float) ((uint64_t) (tickmono * 1000 +0.5 )) / 1000000.0f;

  return mp_obj_new_float( value );
}



FUN_1(sleep);
FUN_0(monotonic);
FUN_0(__init__);


/* Module definittime */

// Helper: define object "time_F_obj" as object "F" in the module
#define OBJ(F)                                                                 \
  { MP_ROM_QSTR(MP_QSTR_##F), MP_ROM_PTR(&time_##F##_obj) }

// Helper: define small integer constant "I" as "I" in the module
#define INT(I)                                                                 \
  { MP_ROM_QSTR(MP_QSTR_##I), MP_OBJ_NEW_SMALL_INT(I) }

STATIC const mp_rom_map_elem_t time_module_globals_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_time)},
    OBJ(__init__),
    OBJ(sleep),
    OBJ(monotonic),
};

STATIC MP_DEFINE_CONST_DICT(time_module_globals, time_module_globals_table);

const mp_obj_module_t time_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&time_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_time, time_module);
