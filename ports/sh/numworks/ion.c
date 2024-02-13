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

#define KEY_LEFT                  0
#define KEY_UP                    1
#define KEY_DOWN                  2
#define KEY_RIGHT                 3
#define KEY_OK                    4
#define KEY_BACK                  5
#define KEY_HOME                  6
#define KEY_ONOFF                 7
/* -- */
#define KEY_SHIFT                 12
#define KEY_ALPHA                 13
#define KEY_XNT                   14
#define KEY_VAR                   15
#define KEY_TOOLBOX               16
#define KEY_BACKSPACE             17
#define KEY_EXP                   18
#define KEY_LN                    19
#define KEY_LOG                   20
#define KEY_IMAGINARY             21
#define KEY_COMMA                 22
#define KEY_POWER                 23
#define KEY_SINE                  24
#define KEY_COSINE                25
#define KEY_TANGENT               26
#define KEY_PI                    27
#define KEY_SQRT                  28
#define KEY_SQUARE                29
#define KEY_SEVEN                 30
#define KEY_EIGHT                 31
#define KEY_NINE                  32
#define KEY_LEFTPARENTHESIS       33
#define KEY_RIGHTPARENTHESIS      34
/* -- */
#define KEY_FOUR                  36
#define KEY_FIVE                  37
#define KEY_SIX                   38
#define KEY_MULTIPLICATION        39
#define KEY_DIVISION              40
/* -- */
#define KEY_ONE                   42
#define KEY_TWO                   43
#define KEY_THREE                 44
#define KEY_PLUS                  45
#define KEY_MINUS                 46
/* -- */
#define KEY_ZERO                  48
#define KEY_DOT                   49
#define KEY_EE                    50
#define KEY_ANS                   51
#define KEY_EXE                   52

int KeyTranslationMap[ 53 ] = { 0x85, 0x86, 0x75, 0x76, 0x91, // gint LEFT, UP, DOWN, RIGHT, F1
                                0x74, 0x84, 0x07,   -1,   -1, // gint EXIT, MENU, ACON, __, __
                                  -1,   -1, 0x81, 0x71, 0x61, // gint __, __, SHIFT, ALPHA, XOT
                                0x83, 0x82, 0x44, 0x13, 0x63, // gint VARS, OPTN, DEL, EXP, LN
                                0x62, 0x92, 0x55, 0x73, 0x64, // gint LOG, F2, COMMA, POWER, SIN
                                0x65, 0x66, 0x93, 0x94, 0x72, // gint COS, TAN, F3, F4, SQUARE
                                0x41, 0x42, 0x43, 0x53, 0x54, // gint 7, 8, 9, LEFP, RIGHTP
                                  -1, 0x31, 0x32, 0x33, 0x34, // gint __, 4, 5, 6, MUL
                                0x35,   -1, 0x21, 0x22, 0x23, // gint DIV, __, 1, 2, 3
                                0x24, 0x25,   -1, 0x11, 0x12, // gint ADD, SUB, __, 0, DOT
                                0x95, 0x14, 0x15 };           // gint F5, NEG, EXE


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

  if (key < KEY_LEFT || key > KEY_EXE )
    return mp_obj_new_bool(false);

  int translatedKey = KeyTranslationMap[ key ];

  if (translatedKey==-1)
    return mp_obj_new_bool(false);

  clearevents();

  bool down = keydown(translatedKey) != 0;
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
    /* BE CAREFUL THERE ARE MISSING SLOTS */
    
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
    INT(KEY_RIGHTPARENTHESIS), // value 34
    
    INT(KEY_FOUR), // value 36
    INT(KEY_FIVE),
    INT(KEY_SIX),
    INT(KEY_MULTIPLICATION),
    INT(KEY_DIVISION), // value 40

    INT(KEY_ONE), // value 42
    INT(KEY_TWO),
    INT(KEY_THREE),
    INT(KEY_PLUS),
    INT(KEY_MINUS), // value 46

    INT(KEY_ZERO), // value 48
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
