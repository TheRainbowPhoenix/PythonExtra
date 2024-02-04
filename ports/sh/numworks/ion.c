#ifdef FXCG50

//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.ion: `gint` module
//
// This module aims to wrap commonly-used gint functions (not all APIs are
// considered relevant for high-level Python development).
//---

#include "console.h"
#include "py/objtuple.h"
#include "py/runtime.h"
#include <gint/keyboard.h>

#include <stdio.h>
#include <stdlib.h>

/* BEGINING OF KEY TRANSLATION */

// the following table aims at providing a keymap for NW on Casio
// line that are commented correspond to keys that are similar (with exact same
// name) between NW and Casio

//#define KEY_LEFT
//#define KEY_UP
//#define KEY_DOWN
//#define KEY_RIGHT
#define KEY_OK KEY_F1
#define KEY_BACK KEY_EXIT
#define KEY_HOME KEY_MENU
#define KEY_ONOFF KEY_ACON
//#define KEY_SHIFT
//#define KEY_ALPHA
#define KEY_XNT KEY_XOT
#define KEY_VAR KEY_VARS
#define KEY_TOOLBOX KEY_OPTN
#define KEY_BACKSPACE KEY_DEL
//#define KEY_EXP               // Note this one may be challenging
//#define KEY_LN
//#define KEY_LOG
#define KEY_IMAGINARY KEY_F2
//#define KEY_COMMA
//#define KEY_POWER
#define KEY_SINE KEY_SIN
#define KEY_COSINE KEY_COS
#define KEY_TANGENT KEY_TAN
#define KEY_PI KEY_F3
#define KEY_SQRT KEY_F4
//#define KEY_SQUARE
#define KEY_SEVEN KEY_7
#define KEY_EIGHT KEY_8
#define KEY_NINE KEY_9
#define KEY_LEFTPARENTHESIS KEY_LEFTP
#define KEY_RIGHTPARENTHESIS KEY_RIGHTP
#define KEY_FOUR KEY_4
#define KEY_FIVE KEY_5
#define KEY_SIX KEY_6
#define KEY_MULTIPLICATION KEY_MUL
#define KEY_DIVISION KEY_DIV
#define KEY_ONE KEY_1
#define KEY_TWO KEY_2
#define KEY_THREE KEY_3
#define KEY_PLUS KEY_ADD
#define KEY_MINUS KEY_SUB
#define KEY_ZERO KEY_0
//#define KEY_DOT
#define KEY_EE KEY_F5
#define KEY_ANS KEY_NEG
//#define KEY_EXE


/* END OF KEY TRANSLATION */



#define FUN_0(NAME) MP_DEFINE_CONST_FUN_OBJ_0(ion_##NAME##_obj, ion_##NAME)
#define FUN_1(NAME) MP_DEFINE_CONST_FUN_OBJ_1(ion_##NAME##_obj, ion_##NAME)
#define FUN_2(NAME) MP_DEFINE_CONST_FUN_OBJ_2(ion_##NAME##_obj, ion_##NAME)
#define FUN_3(NAME) MP_DEFINE_CONST_FUN_OBJ_3(ion_##NAME##_obj, ion_##NAME)
#define FUN_VAR(NAME, MIN)                                                     \
  MP_DEFINE_CONST_FUN_OBJ_VAR(ion_##NAME##_obj, MIN, ion_##NAME)
#define FUN_BETWEEN(NAME, MIN, MAX)                                            \
  MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(ion_##NAME##_obj, MIN, MAX, ion_##NAME)

STATIC mp_obj_t ion___init__(void) { return mp_const_none; }

/* <gint/keyboard.h> */

STATIC mp_obj_t ion_keydown(mp_obj_t arg1) {
  mp_int_t key = mp_obj_get_int(arg1);
  clearevents();
  bool down = keydown(key) != 0;
  return mp_obj_new_bool(down);
}

FUN_1(keydown);
FUN_0(__init__);

/* Module definition */

// Helper: define object "ion_F_obj" as object "F" in the module
#define OBJ(F)                                                                 \
  { MP_ROM_QSTR(MP_QSTR_##F), MP_ROM_PTR(&ion_##F##_obj) }

// Helper: define small integer constant "I" as "I" in the module
#define INT(I)                                                                 \
  { MP_ROM_QSTR(MP_QSTR_##I), MP_OBJ_NEW_SMALL_INT(I) }

STATIC const mp_rom_map_elem_t ion_module_globals_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_ion)},
    OBJ(__init__),

    /*Numworks keycodes */

    INT(KEY_LEFT), // value 0
    INT(KEY_UP),
    INT(KEY_DOWN),
    INT(KEY_RIGHT),
    INT(KEY_OK),
    INT(KEY_BACK),
    INT(KEY_HOME),
    INT(KEY_ONOFF), // value 7

    INT(KEY_SHIFT), // value 12
    INT(KEY_ALPHA),
    INT(KEY_XNT),
    INT(KEY_VAR),
    INT(KEY_TOOLBOX),
    INT(KEY_BACKSPACE),
    INT(KEY_EXP),
    INT(KEY_LN),
    INT(KEY_LOG),
    INT(KEY_IMAGINARY),
    INT(KEY_COMMA),
    INT(KEY_POWER),
    INT(KEY_SINE),
    INT(KEY_COSINE),
    INT(KEY_TANGENT),
    INT(KEY_PI),
    INT(KEY_SQRT),
    INT(KEY_SQUARE),
    INT(KEY_SEVEN),
    INT(KEY_EIGHT),
    INT(KEY_NINE),
    INT(KEY_LEFTPARENTHESIS),
    INT(KEY_RIGHTPARENTHESIS),
    INT(KEY_FOUR),
    INT(KEY_FIVE),
    INT(KEY_SIX),
    INT(KEY_MULTIPLICATION),
    INT(KEY_DIVISION),
    INT(KEY_ONE),
    INT(KEY_TWO),
    INT(KEY_THREE),
    INT(KEY_PLUS),
    INT(KEY_MINUS),
    INT(KEY_ZERO),
    INT(KEY_DOT),
    INT(KEY_EE),
    INT(KEY_ANS),
    INT(KEY_EXE), // value 52

    OBJ(keydown),
};
STATIC MP_DEFINE_CONST_DICT(ion_module_globals, ion_module_globals_table);

const mp_obj_module_t ion_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&ion_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_ion, ion_module);

#endif // FXCG50