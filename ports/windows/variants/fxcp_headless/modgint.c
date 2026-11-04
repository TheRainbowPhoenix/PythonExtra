// Deliberately small tracing contract for tinyking, not a full gint emulator.
#include "py/runtime.h"
#include "py/objtuple.h"

static mp_int_t input_mask, frames, rects, clears, pixels;
typedef struct { mp_int_t type, x, y; } touch_event;
static touch_event events[64];
static unsigned event_read, event_write;

static mp_obj_t trace_reset(void) {
    input_mask = frames = rects = clears = pixels = 0;
    event_read = event_write = 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(trace_reset_obj, trace_reset);

static mp_obj_t trace_keys(mp_obj_t mask) {
    input_mask = mp_obj_get_int(mask);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(trace_keys_obj, trace_keys);

static mp_obj_t trace_touch(mp_obj_t type, mp_obj_t x, mp_obj_t y) {
    if (event_write - event_read >= 64) { mp_raise_ValueError(MP_ERROR_TEXT("event queue full")); }
    events[event_write++ % 64] = (touch_event){mp_obj_get_int(type), mp_obj_get_int(x), mp_obj_get_int(y)};
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(trace_touch_obj, trace_touch);
static mp_obj_t headless_pollevent(void) {
    touch_event ev = {0, 0, 0};
    if (event_read != event_write) { ev = events[event_read++ % 64]; }
    // Match the real port's 8-field event allocation, including KEYEV_NONE.
    static const qstr fields[] = {MP_QSTR_time, MP_QSTR_mod, MP_QSTR_shift,
        MP_QSTR_alpha, MP_QSTR_type, MP_QSTR_key, MP_QSTR_x, MP_QSTR_y};
    mp_obj_t values[] = {MP_OBJ_NEW_SMALL_INT(0), mp_const_false, mp_const_false,
        mp_const_false, mp_obj_new_int(ev.type), MP_OBJ_NEW_SMALL_INT(0),
        mp_obj_new_int(ev.x), mp_obj_new_int(ev.y)};
    return mp_obj_new_attrtuple(fields, 8, values);
}
static MP_DEFINE_CONST_FUN_OBJ_0(headless_pollevent_obj, headless_pollevent);

typedef struct {
    mp_obj_base_t base;
    mp_int_t format, width, height, stride;
    mp_obj_t data, palette;
} headless_image;
static mp_obj_t image_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 7, 7, false);
    mp_int_t fmt = mp_obj_get_int(args[0]), cc = mp_obj_get_int(args[1]);
    mp_int_t w = mp_obj_get_int(args[2]), h = mp_obj_get_int(args[3]), stride = mp_obj_get_int(args[4]);
    mp_buffer_info_t data, pal;
    mp_get_buffer_raise(args[5], &data, MP_BUFFER_READ);
    mp_get_buffer_raise(args[6], &pal, MP_BUFFER_READ);
    if ((fmt != 5 && fmt != 6) || cc != 16 || w <= 0 || h <= 0 || stride < (w + 1) / 2
            || data.len < (size_t)stride * h || pal.len < 32) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid P4 image"));
    }
    headless_image *self = mp_obj_malloc(headless_image, type);
    self->format = fmt; self->width = w; self->height = h; self->stride = stride;
    self->data = args[5]; self->palette = args[6];
    return MP_OBJ_FROM_PTR(self);
}
static MP_DEFINE_CONST_OBJ_TYPE(headless_image_type, MP_QSTR_image, MP_TYPE_FLAG_NONE, make_new, image_new);
static mp_obj_t headless_dsubimage(size_t n, const mp_obj_t *args) {
    (void)n;
    if (!mp_obj_is_type(args[2], &headless_image_type)) { mp_raise_TypeError(MP_ERROR_TEXT("expected image")); }
    headless_image *img = MP_OBJ_TO_PTR(args[2]);
    mp_int_t x = mp_obj_get_int(args[0]), y = mp_obj_get_int(args[1]);
    mp_int_t sx = mp_obj_get_int(args[3]), sy = mp_obj_get_int(args[4]);
    mp_int_t w = mp_obj_get_int(args[5]), h = mp_obj_get_int(args[6]);
    if (sx < 0 || sy < 0 || w <= 0 || h <= 0 || sx + w > img->width || sy + h > img->height) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid subimage bounds"));
    }
    mp_int_t left = x < 0 ? 0 : x, top = y < 0 ? 0 : y;
    mp_int_t right = x + w > 320 ? 320 : x + w, bottom = y + h > 528 ? 528 : y + h;
    rects++;
    if (right > left && bottom > top) { pixels += (right - left) * (bottom - top); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dsubimage_obj, 7, 7, headless_dsubimage);

static mp_obj_t trace_stats(void) {
    mp_obj_t values[] = {mp_obj_new_int(frames), mp_obj_new_int(rects),
        mp_obj_new_int(clears), mp_obj_new_int(pixels)};
    return mp_obj_new_tuple(4, values);
}
static MP_DEFINE_CONST_FUN_OBJ_0(trace_stats_obj, trace_stats);

static mp_obj_t headless_keydown(mp_obj_t key) {
    return mp_obj_new_bool((input_mask & mp_obj_get_int(key)) != 0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_keydown_obj, headless_keydown);
static mp_obj_t headless_clearevents(void) { return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_0(headless_clearevents_obj, headless_clearevents);
static mp_obj_t headless_dupdate(void) { frames++; return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_0(headless_dupdate_obj, headless_dupdate);
static mp_obj_t headless_dclear(mp_obj_t color) {
    (void)mp_obj_get_int(color);
    clears++; pixels += 320 * 528;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_dclear_obj, headless_dclear);
static mp_obj_t headless_drect(size_t n, const mp_obj_t *args) {
    (void)n;
    mp_int_t x1 = mp_obj_get_int(args[0]), y1 = mp_obj_get_int(args[1]);
    mp_int_t x2 = mp_obj_get_int(args[2]), y2 = mp_obj_get_int(args[3]);
    (void)mp_obj_get_int(args[4]);
    rects++;
    if (x1 < 0) { x1 = 0; }
    if (y1 < 0) { y1 = 0; }
    if (x2 > 319) { x2 = 319; }
    if (y2 > 527) { y2 = 527; }
    if (x2 >= x1 && y2 >= y1) { pixels += (x2 - x1 + 1) * (y2 - y1 + 1); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_drect_obj, 5, 5, headless_drect);


// A deliberately counter-only pixel/text implementation. Both Linux and Windows
// use this same source. Drawing is clipped; no framebuffer is allocated.
static mp_obj_t headless_dpixel(mp_obj_t px, mp_obj_t py, mp_obj_t color) {
    mp_int_t x = mp_obj_get_int(px), y = mp_obj_get_int(py);
    (void)mp_obj_get_int(color);
    rects++;
    if (x >= 0 && x < 320 && y >= 0 && y < 528) {
        pixels++;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(headless_dpixel_obj, headless_dpixel);

// dtext(x,y,color,text): estimate the drawn glyph footprint without rendering.
// 6x10 monospace bounding boxes are used for the headless pixel-write counter.
static mp_obj_t headless_dtext(size_t n, const mp_obj_t *args) {
    (void)n;
    mp_int_t x = mp_obj_get_int(args[0]), y = mp_obj_get_int(args[1]);
    (void)mp_obj_get_int(args[2]);
    size_t len;
    (void)mp_obj_str_get_data(args[3], &len);
    rects++;
    if (len && y < 528 && y + 10 > 0 && x < 320) {
        mp_int_t left = x < 0 ? 0 : x, top = y < 0 ? 0 : y;
        mp_int_t right = x + (mp_int_t)(len > 1000 ? 1000 : len) * 6;
        if (right > 320) { right = 320; }
        mp_int_t bottom = y + 10;
        if (bottom > 528) { bottom = 528; }
        if (right > left && bottom > top) {
            pixels += (right - left) * (bottom - top);
        }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dtext_obj, 4, 4, headless_dtext);


// RGB565 color helper, shared by both host ports.
static mp_obj_t headless_rgb(mp_obj_t red, mp_obj_t green, mp_obj_t blue) {
    mp_int_t r = mp_obj_get_int(red), g = mp_obj_get_int(green), b = mp_obj_get_int(blue);
    if (r < 0 || r > 31 || g < 0 || g > 63 || b < 0 || b > 31) {
        mp_raise_ValueError(MP_ERROR_TEXT("RGB channel out of range"));
    }
    return mp_obj_new_int((r << 11) | (g << 6) | b);
}
static MP_DEFINE_CONST_FUN_OBJ_3(headless_rgb_obj, headless_rgb);

// Bresenham line counter, clipped per pixel; no framebuffer or allocations.
static mp_obj_t headless_dline(size_t n, const mp_obj_t *args) {
    (void)n;
    mp_int_t x0 = mp_obj_get_int(args[0]), y0 = mp_obj_get_int(args[1]);
    mp_int_t x1 = mp_obj_get_int(args[2]), y1 = mp_obj_get_int(args[3]);
    (void)mp_obj_get_int(args[4]);
    mp_int_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    mp_int_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    // Protect the instrumentation host against enormous line coordinates.
    if (dx > 1000000 || dy > 1000000) {
        mp_raise_ValueError(MP_ERROR_TEXT("line span too large"));
    }
    mp_int_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    mp_int_t err = dx - dy;
    rects++;
    for (;;) {
        if (x0 >= 0 && x0 < 320 && y0 >= 0 && y0 < 528) { pixels++; }
        if (x0 == x1 && y0 == y1) { break; }
        mp_int_t twice = 2 * err;
        if (twice > -dy) { err -= dy; x0 += sx; }
        if (twice < dx) { err += dx; y0 += sy; }
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dline_obj, 5, 5, headless_dline);

// Deterministic key for noninteractive scripts finishing with getkey().
static mp_obj_t headless_getkey(void) {
    static const qstr fields[] = {MP_QSTR_time, MP_QSTR_mod, MP_QSTR_shift,
        MP_QSTR_alpha, MP_QSTR_type, MP_QSTR_key, MP_QSTR_x, MP_QSTR_y};
    mp_obj_t values[] = {MP_OBJ_NEW_SMALL_INT(0), mp_const_false, mp_const_false,
        mp_const_false, MP_OBJ_NEW_SMALL_INT(1), MP_OBJ_NEW_SMALL_INT(8),
        MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_NEW_SMALL_INT(0)};
    return mp_obj_new_attrtuple(fields, 8, values);
}
static MP_DEFINE_CONST_FUN_OBJ_0(headless_getkey_obj, headless_getkey);


/* Counter-only compatibility APIs: keep the same signatures on Unix and Win32.
 * Pixel counts are bounding-box estimates, not framebuffer render results. */
static void headless_box(mp_int_t x1, mp_int_t y1, mp_int_t x2, mp_int_t y2) {
    rects++;
    if (x1 < 0) { x1 = 0; }
    if (y1 < 0) { y1 = 0; }
    if (x2 > 319) { x2 = 319; }
    if (y2 > 527) { y2 = 527; }
    if (x2 >= x1 && y2 >= y1) {
        pixels += (x2 - x1 + 1) * (y2 - y1 + 1);
    }
}
static mp_obj_t headless_dtext_opt(size_t n, const mp_obj_t *args) {
    (void)n;
    mp_int_t x = mp_obj_get_int(args[0]), y = mp_obj_get_int(args[1]);
    (void)mp_obj_get_int(args[2]);
    (void)mp_obj_get_int(args[3]);
    mp_int_t horizontal = mp_obj_get_int(args[4]), vertical = mp_obj_get_int(args[5]);
    size_t len;
    (void)mp_obj_str_get_data(args[6], &len);
    mp_int_t maxchars = mp_obj_get_int(args[7]);
    if (maxchars >= 0 && len > (size_t)maxchars) { len = (size_t)maxchars; }
    if (len > 1000) { len = 1000; }
    mp_int_t width = len * 6;
    if (horizontal == 1) { x -= width / 2; }
    if (horizontal == 2) { x -= width; }
    if (vertical == 1) { y -= 5; }
    if (vertical == 2) { y -= 10; }
    if (width) { headless_box(x,y,x+width-1,y+9); } else { rects++; }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dtext_opt_obj, 8, 8, headless_dtext_opt);

static mp_obj_t headless_dsize(mp_obj_t text, mp_obj_t font) {
    (void)font;
    size_t len;
    (void)mp_obj_str_get_data(text, &len);
    mp_obj_t values[2] = {mp_obj_new_int(len * 6), MP_OBJ_NEW_SMALL_INT(10)};
    return mp_obj_new_tuple(2, values);
}
static MP_DEFINE_CONST_FUN_OBJ_2(headless_dsize_obj, headless_dsize);

static mp_obj_t headless_drect_border(size_t n, const mp_obj_t *args) {
    (void)n;
    mp_int_t x1=mp_obj_get_int(args[0]), y1=mp_obj_get_int(args[1]);
    mp_int_t x2=mp_obj_get_int(args[2]), y2=mp_obj_get_int(args[3]);
    (void)mp_obj_get_int(args[4]);
    (void)mp_obj_get_int(args[5]);
    mp_int_t border=mp_obj_get_int(args[6]);
    if (border < 0 || border > 1000) { mp_raise_ValueError(MP_ERROR_TEXT("invalid border width")); }
    headless_box(x1,y1,x2,y2);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_drect_border_obj, 7, 7, headless_drect_border);

static mp_obj_t headless_dcircle(size_t n, const mp_obj_t *args) {
    (void)n;
    mp_int_t x=mp_obj_get_int(args[0]), y=mp_obj_get_int(args[1]);
    mp_int_t r=mp_obj_get_int(args[2]);
    (void)mp_obj_get_int(args[3]); (void)mp_obj_get_int(args[4]);
    if (r<0 || r>100000) { mp_raise_ValueError(MP_ERROR_TEXT("invalid radius")); }
    headless_box(x-r,y-r,x+r,y+r);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dcircle_obj, 5, 5, headless_dcircle);

static mp_obj_t headless_dellipse(size_t n, const mp_obj_t *args) {
    (void)n;
    mp_int_t x=mp_obj_get_int(args[0]), y=mp_obj_get_int(args[1]);
    mp_int_t rx=mp_obj_get_int(args[2]), ry=mp_obj_get_int(args[3]);
    (void)mp_obj_get_int(args[4]); (void)mp_obj_get_int(args[5]);
    if (rx<0 || rx>100000 || ry<0 || ry>100000) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid radii"));
    }
    headless_box(x-rx,y-ry,x+rx,y+ry);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dellipse_obj, 6, 6, headless_dellipse);

static mp_obj_t headless_dpoly(mp_obj_t coordinates, mp_obj_t fill, mp_obj_t border) {
    (void)mp_obj_get_int(fill); (void)mp_obj_get_int(border);
    size_t length;
    mp_obj_t *items;
    mp_obj_get_array(coordinates, &length, &items);
    if (length < 6 || (length & 1)) {
        mp_raise_ValueError(MP_ERROR_TEXT("polygon expects at least 3 xy points"));
    }
    mp_int_t x1=mp_obj_get_int(items[0]), x2=x1;
    mp_int_t y1=mp_obj_get_int(items[1]), y2=y1;
    for (size_t i=2; i<length; i+=2) {
        mp_int_t x=mp_obj_get_int(items[i]), y=mp_obj_get_int(items[i+1]);
        if(x<x1){x1=x;} if(x>x2){x2=x;}
        if(y<y1){y1=y;} if(y>y2){y2=y;}
    }
    headless_box(x1,y1,x2,y2);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(headless_dpoly_obj, headless_dpoly);

static mp_obj_t headless_dhline(mp_obj_t y, mp_obj_t color) {
    (void)mp_obj_get_int(color);
    headless_box(0,mp_obj_get_int(y),319,mp_obj_get_int(y));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(headless_dhline_obj, headless_dhline);
static mp_obj_t headless_dvline(mp_obj_t x, mp_obj_t color) {
    (void)mp_obj_get_int(color);
    headless_box(mp_obj_get_int(x),0,mp_obj_get_int(x),527);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(headless_dvline_obj, headless_dvline);

static mp_obj_t headless_cleareventflips(void) { return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_0(headless_cleareventflips_obj, headless_cleareventflips);
static mp_obj_t headless_keypressed(mp_obj_t key) { return headless_keydown(key); }
static MP_DEFINE_CONST_FUN_OBJ_1(headless_keypressed_obj, headless_keypressed);
static mp_obj_t headless_keyreleased(mp_obj_t key) {
    (void)mp_obj_get_int(key); return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_keyreleased_obj, headless_keyreleased);

#define FUN(name, obj) {MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_PTR(&obj)}
#define INT(name, val) {MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_INT(val)}
static const mp_rom_map_elem_t globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_gint)},
    INT(DWIDTH, 320), INT(DHEIGHT, 528),
    INT(C_WHITE, 65535), INT(C_BLACK, 0), INT(C_NONE, -1), INT(C_INVERT, -2),
    INT(C_RED, 63488), INT(C_GREEN, 2016), INT(C_BLUE, 31),
    INT(DTEXT_LEFT, 0), INT(DTEXT_CENTER, 1), INT(DTEXT_RIGHT, 2),
    INT(DTEXT_TOP, 0), INT(DTEXT_MIDDLE, 1), INT(DTEXT_BOTTOM, 2),
    INT(font_9, 9), INT(font8x9, 89),
    INT(KEY_LEFT, 1), INT(KEY_RIGHT, 2), INT(KEY_SHIFT, 4), INT(KEY_EXIT, 8),
    INT(KEY_7, 16), INT(KEY_9, 32),
    INT(KEY_EXE, 64), INT(KEY_UP, 128), INT(KEY_4, 256), INT(KEY_6, 512),
    INT(KEY_8, 1024), INT(KEY_DEL, 2048), INT(KEY_DOWN, 4096),
    INT(KEY_MENU, 8192), INT(KEY_F1, 16384), INT(KEY_KBD, 32768),
    INT(KEYEV_NONE, 0), INT(KEYEV_DOWN, 1), INT(KEYEV_UP, 2), INT(KEYEV_TOUCH_DOWN, 1), INT(KEYEV_TOUCH_DRAG, 2), INT(KEYEV_TOUCH_UP, 3),
    INT(IMAGE_P4_RGB565, 5), INT(IMAGE_P4_RGB565A, 6),
    FUN(image, headless_image_type), FUN(dsubimage, headless_dsubimage_obj),
    FUN(trace_touch, trace_touch_obj), FUN(pollevent, headless_pollevent_obj),
    FUN(keydown, headless_keydown_obj), FUN(clearevents, headless_clearevents_obj),
    FUN(dupdate, headless_dupdate_obj), FUN(dclear, headless_dclear_obj),
    FUN(drect, headless_drect_obj), FUN(dpixel, headless_dpixel_obj),
    FUN(dtext, headless_dtext_obj), FUN(dline, headless_dline_obj),
    FUN(C_RGB, headless_rgb_obj), FUN(getkey, headless_getkey_obj),
    FUN(dtext_opt, headless_dtext_opt_obj), FUN(dsize, headless_dsize_obj),
    FUN(drect_border, headless_drect_border_obj), FUN(dcircle, headless_dcircle_obj),
    FUN(dellipse, headless_dellipse_obj), FUN(dpoly, headless_dpoly_obj),
    FUN(dhline, headless_dhline_obj), FUN(dvline, headless_dvline_obj),
    FUN(cleareventflips, headless_cleareventflips_obj),
    FUN(keypressed, headless_keypressed_obj), FUN(keyreleased, headless_keyreleased_obj),
    FUN(trace_reset, trace_reset_obj),
    FUN(trace_keys, trace_keys_obj), FUN(trace_stats, trace_stats_obj),
};
static MP_DEFINE_CONST_DICT(globals, globals_table);
const mp_obj_module_t headless_gint = { .base = {&mp_type_module}, .globals = (mp_obj_dict_t *)&globals };
MP_REGISTER_MODULE(MP_QSTR_gint, headless_gint);
