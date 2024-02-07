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

#include "debug.h"
#include "py/runtime.h"
#include "py/objtuple.h"
#include "objgintimage.h"
#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/timer.h>
#include <gint/drivers/keydev.h>

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
    dclear(C_WHITE);
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

STATIC mp_obj_t modgint_cleareventflips(void)
{
    cleareventflips();
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

STATIC mp_obj_t modgint_keypressed(mp_obj_t arg1)
{
    mp_int_t key = mp_obj_get_int(arg1);
    return mp_obj_new_bool(keypressed(key) != 0);
}

STATIC mp_obj_t modgint_keyreleased(mp_obj_t arg1)
{
    mp_int_t key = mp_obj_get_int(arg1);
    return mp_obj_new_bool(keyreleased(key) != 0);
}

/* Version of getkey_opt() that includes a VM hook */
STATIC key_event_t getkey_opt_internal(int opt, int timeout_ms)
{
    /* Preset keydev transforms so they stay between calls */
    keydev_t *d = keydev_std();
    keydev_transform_t tr = keydev_transform(d);
    key_event_t ev;

    int o = KEYDEV_TR_REPEATS +
        KEYDEV_TR_DELETE_MODIFIERS +
        KEYDEV_TR_DELETE_RELEASES +
        (opt & (GETKEY_MOD_SHIFT + GETKEY_MOD_ALPHA));
    keydev_set_transform(d, (keydev_transform_t){ o, tr.repeater });

    bool has_timeout = (timeout_ms >= 0);

    while(!has_timeout || timeout_ms > 0) {
        /* Program a delay of whatever's left or 20 ms, whichever is smaller.
           It's not easy to reload a timer currently so just reconfigure. */
        volatile int flag = 0;
        int round_ms = has_timeout ? min(timeout_ms, 20) : 20;
        int t = timer_configure(TIMER_ETMU, round_ms * 1000,
            GINT_CALL_SET(&flag));
        timer_start(t);

        /* Run getkey_opt() for that short period */
        ev = getkey_opt(opt, &flag);

        timer_stop(t);
        if(ev.type != KEYEV_NONE)
            break;

        /* The whole reason this function exists -- run the VM hook */
        MICROPY_VM_HOOK_LOOP;

        if(has_timeout)
            timeout_ms -= round_ms;
    }

    keydev_set_transform(d, tr);
    return ev;
}

STATIC mp_obj_t modgint_getkey(void)
{
    key_event_t ev = getkey_opt_internal(GETKEY_DEFAULT, -1);
    return mk_key_event(ev);
}

STATIC mp_obj_t modgint_getkey_opt(mp_obj_t arg1, mp_obj_t arg2)
{
    int options = mp_obj_get_int(arg1);

    int timeout_ms = -1;
    if(arg2 != mp_const_none)
        timeout_ms = mp_obj_get_int(arg2);

    key_event_t ev = getkey_opt_internal(options, timeout_ms);
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
FUN_0(cleareventflips);
FUN_0(pollevent);
FUN_1(keydown);
FUN_VAR(keydown_all, 0);
FUN_VAR(keydown_any, 0);
FUN_1(keypressed);
FUN_1(keyreleased);
FUN_0(getkey);
FUN_2(getkey_opt);
FUN_1(keycode_function);
FUN_1(keycode_digit);

/* <gint/display.h> */

#ifdef FXCG50
STATIC mp_obj_t modgint_C_RGB(mp_obj_t arg1, mp_obj_t arg2, mp_obj_t arg3)
{
    mp_int_t r = mp_obj_get_int(arg1);
    mp_int_t g = mp_obj_get_int(arg2);
    mp_int_t b = mp_obj_get_int(arg3);
    return MP_OBJ_NEW_SMALL_INT(C_RGB(r, g, b));
}
#endif

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
    pe_debug_run_videocapture();
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

STATIC mp_obj_t modgint_dcircle(size_t n_args, const mp_obj_t *args)
{
    mp_int_t x = mp_obj_get_int(args[0]);
    mp_int_t y = mp_obj_get_int(args[1]);
    mp_int_t r = mp_obj_get_int(args[2]);
    mp_int_t fill = mp_obj_get_int(args[3]);
    mp_int_t border = mp_obj_get_int(args[4]);

    dcircle(x, y, r, fill, border);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dellipse(size_t n_args, const mp_obj_t *args)
{
    mp_int_t x1 = mp_obj_get_int(args[0]);
    mp_int_t y1 = mp_obj_get_int(args[1]);
    mp_int_t x2 = mp_obj_get_int(args[2]);
    mp_int_t y2 = mp_obj_get_int(args[3]);
    mp_int_t fill = mp_obj_get_int(args[4]);
    mp_int_t border = mp_obj_get_int(args[5]);

    dellipse(x1, y1, x2, y2, fill, border);
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

/* fx-CG-specific image constructors */
#ifdef FXCG50

STATIC mp_obj_t modgint_image_rgb565(mp_obj_t arg1, mp_obj_t arg2,
    mp_obj_t arg3)
{
    int width = mp_obj_get_int(arg1);
    int height = mp_obj_get_int(arg2);
    return objgintimage_make(&mp_type_gintimage, IMAGE_RGB565, 0, width,
        height, width * 2, arg3, mp_const_none);
}

STATIC mp_obj_t modgint_image_rgb565a(mp_obj_t arg1, mp_obj_t arg2,
    mp_obj_t arg3)
{
    int width = mp_obj_get_int(arg1);
    int height = mp_obj_get_int(arg2);
    return objgintimage_make(&mp_type_gintimage, IMAGE_RGB565A, 0, width,
        height, width * 2, arg3, mp_const_none);
}

STATIC mp_obj_t modgint_image_p8_rgb565(size_t n, mp_obj_t const *args)
{
    int width = mp_obj_get_int(args[0]);
    int height = mp_obj_get_int(args[1]);
    mp_obj_t data = args[2];
    mp_obj_t palette = args[3];

    int color_count = mp_obj_get_int(mp_obj_len(palette)) / 2;
    int stride = width;
    return objgintimage_make(&mp_type_gintimage, IMAGE_P8_RGB565,
        color_count, width, height, stride, data, palette);
}

STATIC mp_obj_t modgint_image_p8_rgb565a(size_t n, mp_obj_t const *args)
{
    int width = mp_obj_get_int(args[0]);
    int height = mp_obj_get_int(args[1]);
    mp_obj_t data = args[2];
    mp_obj_t palette = args[3];

    int color_count = mp_obj_get_int(mp_obj_len(palette)) / 2;
    int stride = width;
    return objgintimage_make(&mp_type_gintimage, IMAGE_P8_RGB565A,
        color_count, width, height, stride, data, palette);
}

STATIC mp_obj_t modgint_image_p4_rgb565(size_t n, mp_obj_t const *args)
{
    int width = mp_obj_get_int(args[0]);
    int height = mp_obj_get_int(args[1]);
    mp_obj_t data = args[2];
    mp_obj_t palette = args[3];

    int stride = (width + 1) / 2;
    return objgintimage_make(&mp_type_gintimage, IMAGE_P4_RGB565, 16,
        width, height, stride, data, palette);
}

STATIC mp_obj_t modgint_image_p4_rgb565a(size_t n, mp_obj_t const *args)
{
    int width = mp_obj_get_int(args[0]);
    int height = mp_obj_get_int(args[1]);
    mp_obj_t data = args[2];
    mp_obj_t palette = args[3];

    int stride = (width + 1) / 2;
    return objgintimage_make(&mp_type_gintimage, IMAGE_P4_RGB565A, 16,
        width, height, stride, data, palette);
}

#endif /* FXCG50 */

STATIC mp_obj_t modgint_dimage(mp_obj_t arg1, mp_obj_t arg2, mp_obj_t arg3)
{
    mp_int_t x = mp_obj_get_int(arg1);
    mp_int_t y = mp_obj_get_int(arg2);

    bopti_image_t img;
    objgintimage_get(arg3, &img);

    dimage(x, y, &img);
    return mp_const_none;
}

STATIC mp_obj_t modgint_dsubimage(size_t n_args, const mp_obj_t *args)
{
    mp_int_t x      = mp_obj_get_int(args[0]);
    mp_int_t y      = mp_obj_get_int(args[1]);
    // args[2] is the image
    mp_int_t left   = mp_obj_get_int(args[3]);
    mp_int_t top    = mp_obj_get_int(args[4]);
    mp_int_t width  = mp_obj_get_int(args[5]);
    mp_int_t height = mp_obj_get_int(args[6]);

    bopti_image_t img;
    objgintimage_get(args[2], &img);

    dsubimage(x, y, &img, left, top, width, height, DIMAGE_NONE);
    return mp_const_none;
}

FUN_0(__init__);

#ifdef FXCG50
FUN_3(C_RGB);
#endif
FUN_1(dclear);
FUN_0(dupdate);
FUN_BETWEEN(drect, 5, 5);
FUN_BETWEEN(drect_border, 7, 7);
FUN_3(dpixel);
FUN_2(dgetpixel);
FUN_BETWEEN(dline, 5, 5);
FUN_2(dhline);
FUN_2(dvline);
FUN_BETWEEN(dcircle, 5, 5);
FUN_BETWEEN(dellipse, 6, 6);
FUN_BETWEEN(dtext_opt, 8, 8);
FUN_BETWEEN(dtext, 4, 4);
#ifdef FXCG50
FUN_3(image_rgb565);
FUN_3(image_rgb565a);
FUN_BETWEEN(image_p8_rgb565, 4, 4);
FUN_BETWEEN(image_p8_rgb565a, 4, 4);
FUN_BETWEEN(image_p4_rgb565, 4, 4);
FUN_BETWEEN(image_p4_rgb565a, 4, 4);
#endif
FUN_3(dimage);
FUN_BETWEEN(dsubimage, 7, 7);

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
    OBJ(cleareventflips),
    OBJ(keydown),
    OBJ(keydown_all),
    OBJ(keydown_any),
    OBJ(keypressed),
    OBJ(keyreleased),
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
    INT(C_INVERT),
    INT(C_NONE),
#ifdef FX9860G
    INT(C_LIGHTEN),
    INT(C_DARKEN),
#endif
#ifdef FXCG50
    INT(C_RED),
    INT(C_GREEN),
    INT(C_BLUE),
    OBJ(C_RGB),
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
    OBJ(dcircle),
    OBJ(dellipse),
    OBJ(dtext_opt),
    OBJ(dtext),

    { MP_ROM_QSTR(MP_QSTR_image), MP_ROM_PTR(&mp_type_gintimage) },
    #ifdef FXCG50
    OBJ(image_rgb565),
    OBJ(image_rgb565a),
    OBJ(image_p8_rgb565),
    OBJ(image_p8_rgb565a),
    OBJ(image_p4_rgb565),
    OBJ(image_p4_rgb565a),
    #endif
    OBJ(dimage),
    OBJ(dsubimage),

    /* <gint/image.h> */

#ifdef FX9860G
    INT(IMAGE_MONO),
    INT(IMAGE_MONO_ALPHA),
    INT(IMAGE_GRAY),
    INT(IMAGE_GRAY_ALPHA),
#endif
#ifdef FXCG50
    INT(IMAGE_RGB565),
    INT(IMAGE_RGB565A),
    INT(IMAGE_P8_RGB565),
    INT(IMAGE_P8_RGB565A),
    INT(IMAGE_P4_RGB565),
    INT(IMAGE_P4_RGB565A),
    INT(IMAGE_FLAGS_DATA_RO),
    INT(IMAGE_FLAGS_PALETTE_RO),
    INT(IMAGE_FLAGS_DATA_ALLOC),
    INT(IMAGE_FLAGS_PALETTE_ALLOC),
#endif
};
STATIC MP_DEFINE_CONST_DICT(
  modgint_module_globals, modgint_module_globals_table);

const mp_obj_module_t modgint_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&modgint_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_gint, modgint_module);
