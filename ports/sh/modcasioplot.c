//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.modcasioplot: Compatibility module for CAIOS's casioplot library

#include "py/runtime.h"
#include "py/obj.h"
#include "debug.h"
#include <gint/display.h>
#include <stdlib.h>
#include <string.h>

#ifdef FX9860G
extern font_t font_4x4;
extern font_t font_4x6;
extern font_t font_5x7;
#else
extern font_t font_9;
extern font_t font_13;
extern font_t font_19;
#endif

/* TODO: casioplot: Call show_screen() when program finishes */

static color_t get_color(mp_obj_t color)
{
    /* TODO: casioplot: Support float in color tuples? */
    int r = mp_obj_get_int(mp_obj_subscr(color, MP_OBJ_NEW_SMALL_INT(0),
        MP_OBJ_SENTINEL));
    int g = mp_obj_get_int(mp_obj_subscr(color, MP_OBJ_NEW_SMALL_INT(1),
        MP_OBJ_SENTINEL));
    int b = mp_obj_get_int(mp_obj_subscr(color, MP_OBJ_NEW_SMALL_INT(2),
        MP_OBJ_SENTINEL));

#ifdef FX9860G
    return (r + g + b >= 3 * 128) ? C_WHITE : C_BLACK;
#else
    return ((r & 0xf8) << 8) + ((g & 0xfc) << 3) + ((b & 0xf8) >> 3);
#endif
}

static mp_obj_t make_color(color_t color)
{
    int r, g, b;
#ifdef FX9860G
    r = g = b = (color == C_WHITE) ? 255 : 0;
#else
    r = (color >> 8) & 0xf8;
    g = (color >> 3) & 0xfc;
    b = (color << 3) & 0xfc;
#endif

    mp_obj_t items[3] = {
        MP_OBJ_NEW_SMALL_INT(r),
        MP_OBJ_NEW_SMALL_INT(g),
        MP_OBJ_NEW_SMALL_INT(b),
    };
    return mp_obj_new_tuple(3, items);
}

static mp_obj_t init(void)
{
    void pe_enter_graphics_mode(void);
    pe_enter_graphics_mode();
    dclear(C_WHITE);
    return mp_const_none;
}

static mp_obj_t show_screen(void)
{
    void pe_enter_graphics_mode(void);
    pe_enter_graphics_mode();
    dupdate();
    pe_debug_run_videocapture();
    return mp_const_none;
}

static mp_obj_t clear_screen(void)
{
    dclear(C_WHITE);
    return mp_const_none;
}

static mp_obj_t set_pixel(size_t n, mp_obj_t const *args)
{
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    color_t color;
    if(n == 3) {
        color = get_color(args[2]);
    } else {
        color = C_BLACK;
    }
    dpixel(x, y, color);
    return mp_const_none;
}

static mp_obj_t get_pixel(mp_obj_t _x, mp_obj_t _y)
{
    int x = mp_obj_get_int(_x);
    int y = mp_obj_get_int(_y);

    if(x >= 0 && x < DWIDTH && y >= 0 && y < DHEIGHT) {
#ifdef FX9860G
        int bit = gint_vram[(y << 2) + (x >> 5)] & (1 << (~x & 31));
        color_t color = (bit != 0) ? C_BLACK : C_WHITE;
#else
        color_t color = gint_vram[DWIDTH * y + x];
#endif

        return make_color(color);
    }
    return mp_const_none;
}

static mp_obj_t draw_string(size_t n, mp_obj_t const *args)
{
    int x = mp_obj_get_int(args[0]);
    int y = mp_obj_get_int(args[1]);
    size_t text_len;
    char const *text = mp_obj_str_get_data(args[2], &text_len);
    char *text_free = NULL;

    /* If there are \n in the text, turn them into spaces */
    if(strchr(text, '\n')) {
        text_free = strdup(text);
        if(text_free) {
            for(size_t i = 0; i < text_len; i++)
                text_free[i] = (text_free[i] == '\n') ? ' ' : text_free[i];
        }
    }

#ifdef FX9860G
    font_t const *fonts[3] = { &font_4x4, &font_4x4, &font_5x7 };
#else
    font_t const *fonts[3] = { &font_9, &font_13, &font_19 };
#endif

    color_t color = C_BLACK;
    if(n >= 4) {
        color = get_color(args[3]);
    }

    int font_size = 1;
    if(n == 5) {
        if(MP_QSTR_small == mp_obj_str_get_qstr(args[4]))
            font_size = 0;
        else if(MP_QSTR_medium == mp_obj_str_get_qstr(args[4]))
            font_size = 1;
        else if(MP_QSTR_large == mp_obj_str_get_qstr(args[4]))
            font_size = 2;
        else
            mp_raise_ValueError("Unknown font size");
    }

    font_t const *old_font = dfont(fonts[font_size]);
    dtext_opt(x, y, color, C_NONE, DTEXT_LEFT, DTEXT_TOP,
        text_free ? text_free : text, text_len);
    dfont(old_font);

    free(text_free);
    return mp_const_none;
}

MP_DEFINE_CONST_FUN_OBJ_0(init_obj, init);
MP_DEFINE_CONST_FUN_OBJ_0(show_screen_obj, show_screen);
MP_DEFINE_CONST_FUN_OBJ_0(clear_screen_obj, clear_screen);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(set_pixel_obj, 2, 3, set_pixel);
MP_DEFINE_CONST_FUN_OBJ_2(get_pixel_obj, get_pixel);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(draw_string_obj, 3, 5, draw_string);

STATIC const mp_rom_map_elem_t casioplot_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_casioplot) },
    { MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&init_obj) },
    { MP_ROM_QSTR(MP_QSTR_show_screen), MP_ROM_PTR(&show_screen_obj) },
    { MP_ROM_QSTR(MP_QSTR_clear_screen), MP_ROM_PTR(&clear_screen_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pixel), MP_ROM_PTR(&set_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_pixel), MP_ROM_PTR(&get_pixel_obj) },
    { MP_ROM_QSTR(MP_QSTR_draw_string), MP_ROM_PTR(&draw_string_obj) },
};
STATIC MP_DEFINE_CONST_DICT(
    casioplot_module_globals, casioplot_module_globals_table);

const mp_obj_module_t casioplot_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&casioplot_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_casioplot, casioplot_module);
