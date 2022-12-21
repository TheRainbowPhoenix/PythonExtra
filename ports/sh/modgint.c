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

void pe_enter_graphics_mode(void);

#define FUN_0(NAME) \
    MP_DEFINE_CONST_FUN_OBJ_0(modgint_ ## NAME ## _obj, modgint_ ## NAME)
#define FUN_1(NAME) \
    MP_DEFINE_CONST_FUN_OBJ_1(modgint_ ## NAME ## _obj, modgint_ ## NAME)
#define FUN_2(NAME) \
    MP_DEFINE_CONST_FUN_OBJ_2(modgint_ ## NAME ## _obj, modgint_ ## NAME)
#define FUN_3(NAME) \
    MP_DEFINE_CONST_FUN_OBJ_3(modgint_ ## NAME ## _obj, modgint_ ## NAME)
#define FUN_VAR(NAME, MIN) \
    MP_DEFINE_CONST_FUN_OBJ_VAR(modgint_ ## NAME ## _obj, MIN, \
        modgint_ ## NAME)
#define FUN_BETWEEN(NAME, MIN, MAX) \
    MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(modgint_ ## NAME ## _obj, MIN, MAX, \
        modgint_ ## NAME)

STATIC mp_obj_t modgint___init__(void)
{
    pe_enter_graphics_mode();
    return mp_const_none;
}

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
    mp_int_t color = mp_obj_get_int(arg1);
    dclear(color);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dupdate(void)
{
    pe_enter_graphics_mode();
    dupdate();
    return mp_const_none;
}

STATIC mp_obj_t modgint_drect(size_t n, mp_obj_t const *args)
{
    mp_int_t x1 = mp_obj_get_int(args[0]);
    mp_int_t y1 = mp_obj_get_int(args[1]);
    mp_int_t x2 = mp_obj_get_int(args[2]);
    mp_int_t y2 = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);
    drect(x1, y1, x2, y2, color);
    return mp_const_none;
}

STATIC mp_obj_t modgint_drect_border(size_t n, mp_obj_t const *args)
{
    mp_int_t x1 = mp_obj_get_int(args[0]);
    mp_int_t y1 = mp_obj_get_int(args[1]);
    mp_int_t x2 = mp_obj_get_int(args[2]);
    mp_int_t y2 = mp_obj_get_int(args[3]);
    mp_int_t fill_color = mp_obj_get_int(args[4]);
    mp_int_t border_width = mp_obj_get_int(args[5]);
    mp_int_t border_color = mp_obj_get_int(args[6]);
    drect_border(x1, y1, x2, y2, fill_color, border_width, border_color);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dpixel(mp_obj_t arg1, mp_obj_t arg2, mp_obj_t arg3)
{
    mp_int_t x = mp_obj_get_int(arg1);
    mp_int_t y = mp_obj_get_int(arg2);
    mp_int_t color = mp_obj_get_int(arg3);
    dpixel(x, y, color);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dgetpixel(mp_obj_t arg1, mp_obj_t arg2)
{
    mp_int_t x = mp_obj_get_int(arg1);
    mp_int_t y = mp_obj_get_int(arg2);
    return MP_OBJ_NEW_SMALL_INT(dgetpixel(x, y));
}

STATIC mp_obj_t modgint_dline(size_t n, mp_obj_t const *args)
{
    mp_int_t x1 = mp_obj_get_int(args[0]);
    mp_int_t y1 = mp_obj_get_int(args[1]);
    mp_int_t x2 = mp_obj_get_int(args[2]);
    mp_int_t y2 = mp_obj_get_int(args[3]);
    mp_int_t color = mp_obj_get_int(args[4]);
    dline(x1, y1, x2, y2, color);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dhline(mp_obj_t arg1, mp_obj_t arg2)
{
    mp_int_t y = mp_obj_get_int(arg1);
    mp_int_t color = mp_obj_get_int(arg2);
    dhline(y, color);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dvline(mp_obj_t arg1, mp_obj_t arg2)
{
    mp_int_t x = mp_obj_get_int(arg1);
    mp_int_t color = mp_obj_get_int(arg2);
    dvline(x, color);
    return mp_const_none;
}

// TODO: modgint: Font management?

STATIC mp_obj_t modgint_dtext_opt(size_t n, mp_obj_t const *args)
{
    mp_int_t x = mp_obj_get_int(args[0]);
    mp_int_t y = mp_obj_get_int(args[1]);
    mp_int_t fg = mp_obj_get_int(args[2]);
    mp_int_t bg = mp_obj_get_int(args[3]);
    mp_int_t halign = mp_obj_get_int(args[4]);
    mp_int_t valign = mp_obj_get_int(args[5]);
    char const *str = mp_obj_str_get_str(args[6]);
    mp_int_t size = mp_obj_get_int(args[7]);
    dtext_opt(x, y, fg, bg, halign, valign, str, size);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dtext(size_t n, mp_obj_t const *args)
{
    mp_int_t x = mp_obj_get_int(args[0]);
    mp_int_t y = mp_obj_get_int(args[1]);
    mp_int_t fg = mp_obj_get_int(args[2]);
    char const *str = mp_obj_str_get_str(args[3]);
    dtext(x, y, fg, str);
    return mp_const_none;
}

FUN_0(__init__);
FUN_1(dclear);
FUN_0(dupdate);
FUN_BETWEEN(drect, 5, 5);
FUN_BETWEEN(drect_border, 7, 7);
FUN_3(dpixel);
FUN_2(dgetpixel);
FUN_BETWEEN(dline, 5, 5);
FUN_2(dhline);
FUN_2(dvline);
FUN_BETWEEN(dtext_opt, 8, 8);
FUN_BETWEEN(dtext, 4, 4);

/* Module definition */

// Helper: define object "modgint_F_obj" as object "F" in the module
#define OBJ(F) {MP_ROM_QSTR(MP_QSTR_ ## F), MP_ROM_PTR(&modgint_ ## F ## _obj)}

// Helper: define small integer constant "I" as "I" in the module
#define INT(I) {MP_ROM_QSTR(MP_QSTR_ ## I), MP_OBJ_NEW_SMALL_INT(I)}

STATIC const mp_rom_map_elem_t modgint_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_gint) },
    OBJ(__init__),

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

    INT(DWIDTH),
    INT(DHEIGHT),
    INT(DTEXT_LEFT),
    INT(DTEXT_CENTER),
    INT(DTEXT_RIGHT),
    INT(DTEXT_TOP),
    INT(DTEXT_MIDDLE),
    INT(DTEXT_BOTTOM),

    INT(C_WHITE),
    INT(C_LIGHT),
    INT(C_DARK),
    INT(C_BLACK),
    INT(C_NONE),
#ifdef FXCG50
    INT(C_RED),
    INT(C_GREEN),
    INT(C_BLUE),
#endif

    OBJ(dclear),
    OBJ(dupdate),
    OBJ(drect),
    OBJ(drect_border),
    OBJ(dpixel),
    OBJ(dgetpixel),
    OBJ(dline),
    OBJ(dhline),
    OBJ(dvline),
    OBJ(dtext_opt),
    OBJ(dtext),
};
STATIC MP_DEFINE_CONST_DICT(
  modgint_module_globals, modgint_module_globals_table);

const mp_obj_module_t modgint_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&modgint_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_gint, modgint_module);
