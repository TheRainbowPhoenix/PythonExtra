//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.modkandinsky: Compatibility module for NumWorks Kandinsky library

#include "py/obj.h"
#include "py/runtime.h"
#include <gint/display.h>
#include <gint/drivers/r61524.h>
#include <gint/timer.h>

#include <stdlib.h>
#include <string.h>

extern font_t numworks;

extern bool is_dwindowed;
extern bool is_timered;
extern unsigned int timer_altered[9];
extern bool is_refreshed_required;

#define NW_MAX_X 320
#define NW_MAX_Y 222

/* Parameters used in windowed mode to center the screen of the NW in the fxCG screen*/
#define DELTAXNW ((DWIDTH - NW_MAX_X) / 2)      // we center the NW screen on Casio's screen
#define DELTAYNW ((DHEIGHT - NW_MAX_Y) / 2)     

/* refresh rate of the screen */
#define TARGET_FPS 20

/* Definition of color on Numworks */

// Data can be found here
// https://github.com/numworks/epsilon/blob/master/escher/include/escher/palette.h
// and here
// https://github.com/numworks/epsilon/blob/master/python/port/port.cpp#L221

#define NW_RGB(r, g, b) (((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3))

#define NW_BLUE NW_RGB(0x50, 0x75, 0xF2)
#define NW_RED NW_RGB(0xFF, 0x00, 0x0C)
#define NW_GREEN NW_RGB(0x50, 0xC1, 0x02)
#define NW_WHITE NW_RGB(0xFF, 0xFF, 0xFF)
#define NW_BLACK NW_RGB(0x00, 0x00, 0x00)

#define NW_YELLOW NW_RGB(0xFF, 0xCC, 0x7B)
#define NW_PURPLE NW_RGB(0x6E, 0x2D, 0x79)
#define NW_BROWN NW_RGB(0x8D, 0x73, 0x50)
#define NW_CYAN NW_RGB(0x00, 0xFF, 0xFF)
#define NW_ORANGE NW_RGB(0xFE, 0x87, 0x1F)
#define NW_PINK NW_RGB(0xFF, 0xAB, 0xB6)
#define NW_MAGENTA NW_RGB(0xFF, 0x05, 0x88)
#define NW_GRAY NW_RGB(0xA7, 0xA7, 0xA7)

// There are possibly some others to be listed correctly

static int callback(void) {
  is_refreshed_required = true;
  return TIMER_CONTINUE;
}

static mp_obj_t Kandinsky_make_color(color_t color) {
  int r, g, b;
  r = (color >> 8) & 0xf8;
  g = (color >> 3) & 0xfc;
  b = (color << 3) & 0xfc;

  mp_obj_t items[3] = {
      MP_OBJ_NEW_SMALL_INT(r),
      MP_OBJ_NEW_SMALL_INT(g),
      MP_OBJ_NEW_SMALL_INT(b),
  };
  return mp_obj_new_tuple(3, items);
}

static mp_obj_t Kandinsky_init(void) {
  void pe_enter_graphics_mode(void);
  pe_enter_graphics_mode();

  dclear(NW_WHITE);

  struct dwindow nw;
  nw.left = DELTAXNW;
  nw.top = DELTAYNW;
  nw.right = 320 + DELTAXNW;
  nw.bottom = 222 + DELTAYNW;
  dwindow_set(nw);
  is_dwindowed = true; // we mark as windowed

  int t = timer_configure(TIMER_TMU, (1000000/TARGET_FPS), GINT_CALL(callback));
  if (t >= 0) {
    timer_start(t);
    is_timered = true;    // there is a timer altered from this module
    timer_altered[t] = 1; // we put the corresponding timer at 1 to identify it
  }
  return mp_const_none;
}

static mp_obj_t Kandinsky_color(size_t n, mp_obj_t const *args) {
  int r = mp_obj_get_int(args[0]);
  int g = mp_obj_get_int(args[1]);
  int b = mp_obj_get_int(args[2]);
  int color = NW_RGB(r, g, b);

  return mp_obj_new_int(color);
}

int Internal_Get_Color_From_String(const char *str) {

  if (strcmp(str, "red") == 0 || strcmp(str, "r") == 0)
    return NW_RED;
  else if (strcmp(str, "green") == 0 || strcmp(str, "g") == 0)
    return NW_GREEN;
  else if (strcmp(str, "blue") == 0 || strcmp(str, "b") == 0)
    return NW_BLUE;
  else if (strcmp(str, "black") == 0 || strcmp(str, "k") == 0)
    return NW_BLACK;
  else if (strcmp(str, "white") == 0 || strcmp(str, "w") == 0)
    return NW_WHITE;

  else if (strcmp(str, "yellow") == 0 || strcmp(str, "y") == 0)
    return NW_YELLOW;
  else if (strcmp(str, "pink") == 0)
    return NW_PINK;
  else if (strcmp(str, "magenta") == 0)
    return NW_MAGENTA;
  else if (strcmp(str, "grey") == 0 || strcmp(str, "gray") == 0)
    return NW_GRAY;
  else if (strcmp(str, "purple") == 0)
    return NW_PURPLE;
  else if (strcmp(str, "orange") == 0)
    return NW_ORANGE;
  else if (strcmp(str, "cyan") == 0)
    return NW_CYAN;
  else if (strcmp(str, "brown") == 0)
    return NW_BROWN;
  else
    return C_NONE;
}

int Internal_Treat_Color(mp_obj_t color) {
  const mp_obj_type_t *type = mp_obj_get_type(color);

  if (type == &mp_type_str) {
    size_t text_len;
    char const *text = mp_obj_str_get_data(color, &text_len);
    return Internal_Get_Color_From_String(text);
  }

  else if (type == &mp_type_int)
    return mp_obj_get_int(color);

  else if (type == &mp_type_tuple) {
    size_t tuple_len;
    mp_obj_t *items;
    mp_obj_tuple_get(color, &tuple_len, &items);
    int r = mp_obj_get_int(items[0]);
    int g = mp_obj_get_int(items[1]);
    int b = mp_obj_get_int(items[2]);
    return NW_RGB(r, g, b);
  } else
    return NW_BLACK;
}

static mp_obj_t Kandinsky_fill_rect(size_t n, mp_obj_t const *args) {
  int x = mp_obj_get_int(args[0]) + DELTAXNW;
  int y = mp_obj_get_int(args[1]) + DELTAYNW;
  int w = mp_obj_get_int(args[2]);
  int h = mp_obj_get_int(args[3]);

  int color = Internal_Treat_Color(args[4]);

  drect(x, y, x + w - 1, y + h - 1, color);

  return mp_const_none;
}

static mp_obj_t Kandinsky_set_pixel(size_t n, mp_obj_t const *args) {
  int x = mp_obj_get_int(args[0]) + DELTAXNW;
  int y = mp_obj_get_int(args[1]) + DELTAYNW;

  int color;
  if (n == 3)
    color = Internal_Treat_Color(args[2]);
  else
    color = NW_BLACK;

  dpixel(x, y, color);
  return mp_const_none;
}

static mp_obj_t Kandinsky_get_pixel(mp_obj_t _x, mp_obj_t _y) {
  int x = mp_obj_get_int(_x) + DELTAXNW;
  int y = mp_obj_get_int(_y) + DELTAYNW;

  if ((!is_dwindowed && x >= 0 && x < DWIDTH && y >= 0 && y < DHEIGHT) || (is_dwindowed && x >= 0 && x < NW_MAX_X && y >= 0 && y < NW_MAX_Y)) {
    color_t color = gint_vram[DWIDTH * y + x];
    return Kandinsky_make_color(color);
  }
  return Kandinsky_make_color(0x0000);
}

static mp_obj_t Kandinsky_draw_string(size_t n, mp_obj_t const *args) {
  int x = mp_obj_get_int(args[1]) + DELTAXNW + 1; // values used to adjust the visual result as per actual NW
  int y = mp_obj_get_int(args[2]) + DELTAYNW + 2; // values used to adjust the visual result as per actual NW
  size_t text_len;
  char const *text = mp_obj_str_get_data(args[0], &text_len);

  color_t colortext = NW_BLACK;
  if (n >= 4) {
    colortext = Internal_Treat_Color(args[3]);
  }

  color_t colorback = C_NONE;
  if (n >= 5) {
    colorback = Internal_Treat_Color(args[4]);
  }

  font_t const *old_font = dfont(&numworks);
  
  int u = 0;
  int v = 0;
  for (int l = 0; l < (int)text_len; l++) {
    if (text[l] == '\n') {
      u = 0;
      v += 16;
    } else {
      /* The following test is for support of unicode characters that are encoded on 1 char or more */
      /* we need to pass multiple chars to dtext to take care of unicode encoding */
      if(((unsigned char) text[l]) >= 0x00 && ((unsigned char) text[l]) <= 0x7F){
        drect(x + u - 1, y + v - 1, x + u + 9, y + v + 15, colorback);
        dtext_opt(x + u, y + v, colortext, C_NONE, DTEXT_LEFT, DTEXT_TOP, &text[l], 1);
        u += 10;
      }
      else if(((unsigned char) text[l]) >= 0x80 && ((unsigned char) text[l]) <= 0xDF){
        drect(x + u - 1, y + v - 1, x + u + 9, y + v + 15, colorback);
        dtext_opt(x + u, y + v, colortext, C_NONE, DTEXT_LEFT, DTEXT_TOP, &text[l], 2);
        u += 10;
        l+=1;
      }
      else if(((unsigned char) text[l]) >= 0xE0 && ((unsigned char) text[l]) <= 0xEF){
        drect(x + u - 1, y + v - 1, x + u + 9, y + v + 15, colorback);
        dtext_opt(x + u, y + v, colortext, C_NONE, DTEXT_LEFT, DTEXT_TOP, &text[l], 3);
        u += 10;
        l+=2;
      }
      else if(((unsigned char) text[l]) >= 0xF0 && ((unsigned char) text[l]) <= 0xF7){
        drect(x + u - 1, y + v - 1, x + u + 9, y + v + 15, colorback);
        dtext_opt(x + u, y + v, colortext, C_NONE, DTEXT_LEFT, DTEXT_TOP, &text[l], 4);
        u += 10;
        l+=3;
      }
    }
  }

  dfont(old_font);

  return mp_const_none;
}

static mp_obj_t Kandinsky_CGEXT_Enable_Wide_Screen( void ) {

  struct dwindow nw;
  nw.left = 0;
  nw.top = 0;
  nw.right = DWIDTH;
  nw.bottom = DHEIGHT;
  dwindow_set(nw);
  is_dwindowed = false; // we mark as not windowed

  return mp_const_none;
}

static mp_obj_t Kandinsky_CGEXT_Disable_Wide_Screen( void ) {
  
  struct dwindow nw;
  nw.left = DELTAXNW;
  nw.top = DELTAYNW;
  nw.right = 320 + DELTAXNW;
  nw.bottom = 222 + DELTAYNW;
  dwindow_set(nw);
  is_dwindowed = true; // we mark as windowed

  return mp_const_none;
}

static mp_obj_t Kandinsky_CGEXT_Is_Wide_Screen_Enabled( void ) {

  return mp_obj_new_bool( is_dwindowed );
}

static mp_obj_t Kandinsky_CGEXT_Set_Margin_Color( mp_obj_t color ) {
  
  color_t colorside = NW_BLACK;
  colorside = Internal_Treat_Color(color);
  
  Kandinsky_CGEXT_Enable_Wide_Screen();
  dclear(colorside);
  Kandinsky_CGEXT_Disable_Wide_Screen();
  
  return mp_obj_new_bool( is_dwindowed );
}

/* Extension of Kandinsky for fxCG - all names starting with "CGEXT_" */
MP_DEFINE_CONST_FUN_OBJ_0(Kandinsky_CGEXT_Enable_Wide_Screen_obj, Kandinsky_CGEXT_Enable_Wide_Screen);
MP_DEFINE_CONST_FUN_OBJ_0(Kandinsky_CGEXT_Disable_Wide_Screen_obj, Kandinsky_CGEXT_Disable_Wide_Screen);
MP_DEFINE_CONST_FUN_OBJ_0(Kandinsky_CGEXT_Is_Wide_Screen_Enabled_obj, Kandinsky_CGEXT_Is_Wide_Screen_Enabled);
MP_DEFINE_CONST_FUN_OBJ_1(Kandinsky_CGEXT_Set_Margin_Color_obj, Kandinsky_CGEXT_Set_Margin_Color);
/* Standard Kandinsky function as per Numworks specification */
MP_DEFINE_CONST_FUN_OBJ_0(Kandinsky_init_obj, Kandinsky_init);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(Kandinsky_set_pixel_obj, 3, 3, Kandinsky_set_pixel);
MP_DEFINE_CONST_FUN_OBJ_2(Kandinsky_get_pixel_obj, Kandinsky_get_pixel);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(Kandinsky_draw_string_obj, 3, 5, Kandinsky_draw_string);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(Kandinsky_fill_rect_obj, 5, 5, Kandinsky_fill_rect);
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(Kandinsky_color_obj, 3, 3, Kandinsky_color);

STATIC const mp_rom_map_elem_t kandinsky_module_globals_table[] = {
    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_kandinsky)},
    {MP_ROM_QSTR(MP_QSTR___init__), MP_ROM_PTR(&Kandinsky_init_obj)},
    {MP_ROM_QSTR(MP_QSTR_fill_rect), MP_ROM_PTR(&Kandinsky_fill_rect_obj)},
    {MP_ROM_QSTR(MP_QSTR_color), MP_ROM_PTR(&Kandinsky_color_obj)},
    {MP_ROM_QSTR(MP_QSTR_set_pixel), MP_ROM_PTR(&Kandinsky_set_pixel_obj)},
    {MP_ROM_QSTR(MP_QSTR_get_pixel), MP_ROM_PTR(&Kandinsky_get_pixel_obj)},
    {MP_ROM_QSTR(MP_QSTR_draw_string), MP_ROM_PTR(&Kandinsky_draw_string_obj)},
    {MP_ROM_QSTR(MP_QSTR_CGEXT_Enable_Wide_Screen), MP_ROM_PTR(&Kandinsky_CGEXT_Enable_Wide_Screen_obj)},
    {MP_ROM_QSTR(MP_QSTR_CGEXT_Disable_Wide_Screen), MP_ROM_PTR(&Kandinsky_CGEXT_Disable_Wide_Screen_obj)},
    {MP_ROM_QSTR(MP_QSTR_CGEXT_Is_Wide_Screen_Enabled), MP_ROM_PTR(&Kandinsky_CGEXT_Is_Wide_Screen_Enabled_obj)},
    {MP_ROM_QSTR(MP_QSTR_CGEXT_Set_Margin_Color), MP_ROM_PTR(&Kandinsky_CGEXT_Set_Margin_Color_obj)},
};
STATIC MP_DEFINE_CONST_DICT(kandinsky_module_globals,
                            kandinsky_module_globals_table);

const mp_obj_module_t kandinsky_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&kandinsky_module_globals,
};

MP_REGISTER_MODULE(MP_QSTR_kandinsky, kandinsky_module);
