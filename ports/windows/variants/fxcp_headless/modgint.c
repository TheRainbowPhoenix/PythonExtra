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

#define FUN(name, obj) {MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_PTR(&obj)}
#define INT(name, val) {MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_INT(val)}
static const mp_rom_map_elem_t globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_gint)},
    INT(DWIDTH, 320), INT(DHEIGHT, 528),
    INT(C_WHITE, 65535), INT(C_BLACK, 0),
    INT(KEY_LEFT, 1), INT(KEY_RIGHT, 2), INT(KEY_SHIFT, 4), INT(KEY_EXIT, 8),
    INT(KEY_7, 16), INT(KEY_9, 32),
    INT(KEY_EXE, 64),
    INT(KEYEV_NONE, 0), INT(KEYEV_TOUCH_DOWN, 1), INT(KEYEV_TOUCH_DRAG, 2), INT(KEYEV_TOUCH_UP, 3),
    INT(IMAGE_P4_RGB565, 5), INT(IMAGE_P4_RGB565A, 6),
    FUN(image, headless_image_type), FUN(dsubimage, headless_dsubimage_obj),
    FUN(trace_touch, trace_touch_obj), FUN(pollevent, headless_pollevent_obj),
    FUN(keydown, headless_keydown_obj), FUN(clearevents, headless_clearevents_obj),
    FUN(dupdate, headless_dupdate_obj), FUN(dclear, headless_dclear_obj),
    FUN(drect, headless_drect_obj), FUN(trace_reset, trace_reset_obj),
    FUN(trace_keys, trace_keys_obj), FUN(trace_stats, trace_stats_obj),
};
static MP_DEFINE_CONST_DICT(globals, globals_table);
const mp_obj_module_t headless_gint = { .base = {&mp_type_module}, .globals = (mp_obj_dict_t *)&globals };
MP_REGISTER_MODULE(MP_QSTR_gint, headless_gint);
