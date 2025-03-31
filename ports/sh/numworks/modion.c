//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.io: Compatibility module for NumWorks Ion library

#include "py/objtuple.h"
#include "py/runtime.h"
#include <gint/keyboard.h>
#include <gint/gint.h>
#include <gint/drivers/r61524.h>
#include "syscalls.h"

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

typedef struct
{
  int key;
  mp_obj_t string;
} key2mp;

static const key2mp keyMapping[] =
{
    { 0x85, MP_ROM_QSTR(MP_QSTR_left) },
    { 0x86, MP_ROM_QSTR(MP_QSTR_up) },
    { 0x75, MP_ROM_QSTR(MP_QSTR_down) },
    { 0x76, MP_ROM_QSTR(MP_QSTR_right) },
    { 0x91, MP_ROM_QSTR(MP_QSTR_OK) },
    { 0x74, MP_ROM_QSTR(MP_QSTR_back) },
    { 0x84, MP_ROM_QSTR(MP_QSTR_home) },
    { 0x07, MP_ROM_QSTR(MP_QSTR_onOff) },
    { 0x81, MP_ROM_QSTR(MP_QSTR_shift) },
    { 0x71, MP_ROM_QSTR(MP_QSTR_alpha) },
    { 0x61, MP_ROM_QSTR(MP_QSTR_xnt) },
    { 0x83, MP_ROM_QSTR(MP_QSTR_var) },
    { 0x82, MP_ROM_QSTR(MP_QSTR_toolbox) },
    { 0x44, MP_ROM_QSTR(MP_QSTR_backspace) },
    { 0x13, MP_ROM_QSTR(MP_QSTR_exp) },
    { 0x63, MP_ROM_QSTR(MP_QSTR_ln) },
    { 0x62, MP_ROM_QSTR(MP_QSTR_log) },
    { 0x92, MP_ROM_QSTR(MP_QSTR_imaginary) },
    { 0x55, MP_ROM_QSTR(MP_QSTR_comma) },
    { 0x73, MP_ROM_QSTR(MP_QSTR_power) },
    { 0x64, MP_ROM_QSTR(MP_QSTR_sin) },
    { 0x65, MP_ROM_QSTR(MP_QSTR_cos) },
    { 0x66, MP_ROM_QSTR(MP_QSTR_tan) },
    { 0x93, MP_ROM_QSTR(MP_QSTR_pi) },
    { 0x94, MP_ROM_QSTR(MP_QSTR_sqrt) },
    { 0x72, MP_ROM_QSTR(MP_QSTR_square) },
    { 0x41, MP_ROM_QSTR(MP_QSTR_7) },
    { 0x42, MP_ROM_QSTR(MP_QSTR_8) },
    { 0x43, MP_ROM_QSTR(MP_QSTR_9) },
    { 0x53, MP_ROM_QSTR(MP_QSTR__paren_open_) },
    { 0x54, MP_ROM_QSTR(MP_QSTR__paren_close_) },
    { 0x31, MP_ROM_QSTR(MP_QSTR_4) },
    { 0x32, MP_ROM_QSTR(MP_QSTR_5) },
    { 0x33, MP_ROM_QSTR(MP_QSTR_6) },
    { 0x34, MP_ROM_QSTR(MP_QSTR__star_) },
    { 0x35, MP_ROM_QSTR(MP_QSTR__slash_) },
    { 0x21, MP_ROM_QSTR(MP_QSTR_1) },
    { 0x22, MP_ROM_QSTR(MP_QSTR_2) },
    { 0x23, MP_ROM_QSTR(MP_QSTR_3) },
    { 0x24, MP_ROM_QSTR(MP_QSTR__plus_) },
    { 0x25, MP_ROM_QSTR(MP_QSTR__hyphen_) },
    { 0x11, MP_ROM_QSTR(MP_QSTR_0) },
    { 0x12, MP_ROM_QSTR(MP_QSTR__dot_) },
    { 0x95, MP_ROM_QSTR(MP_QSTR_EE) },
    { 0x14, MP_ROM_QSTR(MP_QSTR_Ans) },
    { 0x15, MP_ROM_QSTR(MP_QSTR_EXE) },
};

/* END OF KEY TRANSLATION */


static mp_obj_t ion_keydown(mp_obj_t arg1) {
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

/* The following function are coming from the Upsilon extension of NW Ion methods */

static mp_obj_t ion_battery( void ) {
  if (gint[HWCALC] != HWCALC_FXCG100) {
    int voltage = (int)gint_world_switch(
      GINT_CALL(CASIOWIN_GetMainBatteryVoltage, 1));
    float value = ((float) voltage) / 100.0f;
    return mp_obj_new_float( value );
  }
  else return mp_obj_new_float( 0.0f );
}

static mp_obj_t ion_battery_level( void ) {
  if (gint[HWCALC] != HWCALC_FXCG100) {
    int voltage = (int)gint_world_switch(
      GINT_CALL(CASIOWIN_GetMainBatteryVoltage, 1));
    if (voltage>=0 && voltage<=360) return mp_obj_new_int(0);
    else if (voltage>360 && voltage<=370) return mp_obj_new_int(1);
    else if (voltage>370 && voltage<=380) return mp_obj_new_int(2);
    else return mp_obj_new_int(3);
  }
  else return mp_obj_new_int(0);
}

static mp_obj_t ion_battery_charging( void ) {
  /* this function give the charging status of the numworks : if plugged return True and False otherwise */
  /* as the Casio is not using a battery, always return False */
  return mp_obj_new_bool( false );
}

static mp_obj_t ion_get_keys( void ) {
  mp_obj_t result = mp_obj_new_set(0, NULL);
  clearevents();
  for (unsigned i = 0; i < sizeof(keyMapping)/sizeof(key2mp); i++) {
      if (keydown(keyMapping[i].key) !=0 ) {
          mp_obj_set_store(result, keyMapping[i].string);
      }
  }
  return result;
}

static mp_obj_t ion_set_brightness(mp_obj_t arg1) {
  if (gint[HWCALC] != HWCALC_FXCG100) {
    mp_int_t level = mp_obj_get_int(arg1);

    if (level<0) level=0;
    else if (level>240) level=240;

    r61524_set(0x5a1, (level & 0xff) + 6);
  }
  return mp_const_none;
}

static mp_obj_t ion_get_brightness( void ) {
  if (gint[HWCALC] != HWCALC_FXCG100) {
      mp_int_t level = r61524_get(0x5a1);
      return mp_obj_new_int( level );
  }
  else return mp_obj_new_int( 0 );
}

MP_DEFINE_CONST_FUN_OBJ_1(ion_keydown_obj, ion_keydown);
MP_DEFINE_CONST_FUN_OBJ_0(ion_battery_obj, ion_battery);
MP_DEFINE_CONST_FUN_OBJ_0(ion_battery_level_obj, ion_battery_level);
MP_DEFINE_CONST_FUN_OBJ_0(ion_battery_charging_obj, ion_battery_charging);
MP_DEFINE_CONST_FUN_OBJ_0(ion_get_keys_obj, ion_get_keys);
MP_DEFINE_CONST_FUN_OBJ_1(ion_set_brightness_obj, ion_set_brightness);
MP_DEFINE_CONST_FUN_OBJ_0(ion_get_brightness_obj, ion_get_brightness);

/* Module definition */

// Helper: define small integer constant "I" as "I" in the module
#define INT(I)                                                                 \
  { MP_ROM_QSTR(MP_QSTR_##I), MP_OBJ_NEW_SMALL_INT(I) }

static const mp_rom_map_elem_t ion_module_globals_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_ion)},

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

    { MP_ROM_QSTR(MP_QSTR_keydown), MP_ROM_PTR(&ion_keydown_obj) },

    /* Upsilon only objects */
    { MP_ROM_QSTR(MP_QSTR_battery), MP_ROM_PTR(&ion_battery_obj) },
    { MP_ROM_QSTR(MP_QSTR_battery_level), MP_ROM_PTR(&ion_battery_level_obj) },
    { MP_ROM_QSTR(MP_QSTR_battery_charging), MP_ROM_PTR(&ion_battery_charging_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_keys), MP_ROM_PTR(&ion_get_keys_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_brightness), MP_ROM_PTR(&ion_set_brightness_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_brightness), MP_ROM_PTR(&ion_get_brightness_obj) },
};
static MP_DEFINE_CONST_DICT(ion_module_globals, ion_module_globals_table);

const mp_obj_module_t ion_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&ion_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_ion, ion_module);
