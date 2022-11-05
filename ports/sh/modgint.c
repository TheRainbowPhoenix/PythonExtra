//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.modgint: `gint` module
//
// This module aims to wrap commonly-used gint functions (not all APIs are
// considered relevant for high-level Python development).
//---

#include "py/runtime.h"
#include "py/objtuple.h"
#include <gint/display.h>
#include <gint/keyboard.h>

#define FUN_0(NAME) \
    MP_DEFINE_CONST_FUN_OBJ_0(modgint_ ## NAME ## _obj, modgint_ ## NAME)
#define FUN_1(NAME) \
    MP_DEFINE_CONST_FUN_OBJ_1(modgint_ ## NAME ## _obj, modgint_ ## NAME)
#define FUN_2(NAME) \
    MP_DEFINE_CONST_FUN_OBJ_2(modgint_ ## NAME ## _obj, modgint_ ## NAME)
#define FUN_VAR(NAME, MIN) \
    MP_DEFINE_CONST_FUN_OBJ_VAR(modgint_ ## NAME ## _obj, MIN, modgint_ ## NAME)

/* <gint/keyboard.h> */

STATIC qstr const key_event_fields[] = {
    MP_QSTR_time,
    MP_QSTR_mod,
    MP_QSTR_shift,
    MP_QSTR_alpha,
    MP_QSTR_type,
    MP_QSTR_key,
};

STATIC mp_obj_t mk_key_event(key_event_t ev)
{
    mp_obj_t items[] = {
        mp_obj_new_int(ev.time),
        mp_obj_new_bool(ev.mod),
        mp_obj_new_bool(ev.shift),
        mp_obj_new_bool(ev.alpha),
        mp_obj_new_int(ev.type),
        mp_obj_new_int(ev.key),
    };
    return mp_obj_new_attrtuple(key_event_fields, 6, items);
}

STATIC mp_obj_t modgint_pollevent(void)
{
    key_event_t ev = pollevent();
    return mk_key_event(ev);
}

// TODO: waitevent: timeout parameter?

STATIC mp_obj_t modgint_clearevents(void)
{
    clearevents();
    return mp_const_none;
}

STATIC mp_obj_t modgint_keydown(mp_obj_t arg1)
{
    mp_int_t key = mp_obj_get_int(arg1);
    bool down = keydown(key) != 0;
    return mp_obj_new_bool(down);
}

STATIC mp_obj_t modgint_keydown_all(size_t n, mp_obj_t const *args)
{
    bool down = true;
    for(size_t i = 0; i < n; i++)
        down &= keydown(mp_obj_get_int(args[i])) != 0;
    return mp_obj_new_bool(down);
}

STATIC mp_obj_t modgint_keydown_any(size_t n, mp_obj_t const *args)
{
    bool down = false;
    for(size_t i = 0; i < n; i++)
        down |= keydown(mp_obj_get_int(args[i])) != 0;
    return mp_obj_new_bool(down);
}

STATIC mp_obj_t modgint_getkey(void)
{
    key_event_t ev = getkey();
    return mk_key_event(ev);
}

// TODO: getkey_opt: timeout parameter?
STATIC mp_obj_t modgint_getkey_opt(mp_obj_t arg1)
{
    int options = mp_obj_get_int(arg1);
    key_event_t ev = getkey_opt(options, NULL);
    return mk_key_event(ev);
}

STATIC mp_obj_t modgint_keycode_function(mp_obj_t arg1)
{
    int keycode = mp_obj_get_int(arg1);
    return MP_OBJ_NEW_SMALL_INT(keycode_function(keycode));
}

STATIC mp_obj_t modgint_keycode_digit(mp_obj_t arg1)
{
    int keycode = mp_obj_get_int(arg1);
    return MP_OBJ_NEW_SMALL_INT(keycode_digit(keycode));
}

FUN_0(clearevents);
FUN_0(pollevent);
FUN_1(keydown);
FUN_VAR(keydown_all, 0);
FUN_VAR(keydown_any, 0);
FUN_0(getkey);
FUN_1/*2*/(getkey_opt);
FUN_1(keycode_function);
FUN_1(keycode_digit);

/* <gint/display.h> */

STATIC mp_obj_t modgint_dclear(mp_obj_t arg1)
{
    mp_int_t color0 = mp_obj_get_int(arg1);
    int color = MP_OBJ_SMALL_INT_VALUE(color0);
    dclear(color);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dupdate(void)
{
    pe_shell_graphics_mode();
    dupdate();
    return mp_const_none;
}

FUN_1(dclear);
FUN_0(dupdate);

/* Module definition */

// Helper: define object "modgint_F_obj" as object "F" in the module
#define OBJ(F) {MP_ROM_QSTR(MP_QSTR_ ## F), MP_ROM_PTR(&modgint_ ## F ## _obj)}

// Helper: define small integer constant "I" as "I" in the module
#define INT(I) {MP_ROM_QSTR(MP_QSTR_ ## I), MP_OBJ_NEW_SMALL_INT(I)}

STATIC const mp_rom_map_elem_t modgint_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gint) },

    /* <gint/keycodes.h> */

    INT(KEY_F1),
    INT(KEY_F2),
    INT(KEY_F3),
    INT(KEY_F4),
    INT(KEY_F5),
    INT(KEY_F6),

    INT(KEY_SHIFT),
    INT(KEY_OPTN),
    INT(KEY_VARS),
    INT(KEY_MENU),
    INT(KEY_LEFT),
    INT(KEY_UP),

    INT(KEY_ALPHA),
    INT(KEY_SQUARE),
    INT(KEY_POWER),
    INT(KEY_EXIT),
    INT(KEY_DOWN),
    INT(KEY_RIGHT),

    INT(KEY_XOT),
    INT(KEY_LOG),
    INT(KEY_LN),
    INT(KEY_SIN),
    INT(KEY_COS),
    INT(KEY_TAN),

    INT(KEY_FRAC),
    INT(KEY_FD),
    INT(KEY_LEFTP),
    INT(KEY_RIGHTP),
    INT(KEY_COMMA),
    INT(KEY_ARROW),

    INT(KEY_7),
    INT(KEY_8),
    INT(KEY_9),
    INT(KEY_DEL),

    INT(KEY_4),
    INT(KEY_5),
    INT(KEY_6),
    INT(KEY_MUL),
    INT(KEY_DIV),

    INT(KEY_1),
    INT(KEY_2),
    INT(KEY_3),
    INT(KEY_ADD),
    INT(KEY_SUB),

    INT(KEY_0),
    INT(KEY_DOT),
    INT(KEY_EXP),
    INT(KEY_NEG),
    INT(KEY_EXE),

    INT(KEY_ACON),
    INT(KEY_HELP),
    INT(KEY_LIGHT),

    INT(KEY_X2),
    INT(KEY_CARET),
    INT(KEY_SWITCH),
    INT(KEY_LEFTPAR),
    INT(KEY_RIGHTPAR),
    INT(KEY_STORE),
    INT(KEY_TIMES),
    INT(KEY_PLUS),
    INT(KEY_MINUS),

    /* <gint/keyboard.h> */

    INT(KEYEV_NONE),
    INT(KEYEV_DOWN),
    INT(KEYEV_UP),
    INT(KEYEV_HOLD),

    INT(GETKEY_MOD_SHIFT),
    INT(GETKEY_MOD_ALPHA),
    INT(GETKEY_BACKLIGHT),
    INT(GETKEY_MENU),
    INT(GETKEY_REP_ARROWS),
    INT(GETKEY_REP_ALL),
    INT(GETKEY_REP_PROFILE),
    INT(GETKEY_FEATURES),
    INT(GETKEY_NONE),
    INT(GETKEY_DEFAULT),

    OBJ(pollevent),
    // OBJ(waitevent),
    OBJ(clearevents),
    OBJ(keydown),
    OBJ(keydown_all),
    OBJ(keydown_any),
    OBJ(getkey),
    OBJ(getkey_opt),
    OBJ(keycode_function),
    OBJ(keycode_digit),

    /* <gint/display.h> */

    OBJ(dclear),
    OBJ(dupdate),
};
STATIC MP_DEFINE_CONST_DICT(
  modgint_module_globals, modgint_module_globals_table);

const mp_obj_module_t modgint_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&modgint_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_gint, modgint_module);
