// Deliberately small tracing contract for tinyking, not a full gint emulator.
#include "py/runtime.h"
#include "py/objtuple.h"
#include <string.h>
#include <stdint.h>


/* Opt-in per-drawing-call recorder. No timestamps or I/O at all unless
 * PE_DRAW_TRACE is set. Records are written in batches at dupdate().
 * Format: "PEDRAW1\\n" followed by 32-byte little-endian records:
 * timestamp_ns/u64, method/u32, duration_ns/u32, x/y/a/b as signed i32.
 * The UI reads the file on a separate thread; this does not touch USB/VRAM.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
typedef struct {
    uint64_t when_ns;
    uint32_t method, duration_ns;
    int32_t x, y, a, b;
} pe_draw_rec;
typedef char pe_record_is_32_bytes[(sizeof(pe_draw_rec)==32)?1:-1];
static FILE *pe_draw_file;
static int pe_draw_enabled = -1;
static pe_draw_rec pe_draw_buffer[2048];
static size_t pe_draw_count;
static void pe_draw_flush(void) {
    if (pe_draw_file && pe_draw_count) {
        fwrite(pe_draw_buffer, sizeof(pe_draw_rec), pe_draw_count, pe_draw_file);
        pe_draw_count = 0;
    }
    if (pe_draw_file) { fflush(pe_draw_file); }
}
static uint64_t pe_draw_clock(void) {
#ifdef _WIN32
    LARGE_INTEGER t, frequency;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&frequency);
    return ((uint64_t)t.QuadPart / (uint64_t)frequency.QuadPart) * 1000000000ULL +
        ((uint64_t)t.QuadPart % (uint64_t)frequency.QuadPart) * 1000000000ULL / (uint64_t)frequency.QuadPart;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}
static uint64_t pe_draw_begin(void) {
    if (pe_draw_enabled == -1) {
        const char *path = getenv("PE_DRAW_TRACE");
        pe_draw_file = path && *path ? fopen(path, "wb") : NULL;
        pe_draw_enabled = pe_draw_file != NULL;
        if (pe_draw_file) {
            fwrite("PEDRAW1\n", 1, 8, pe_draw_file);
            atexit(pe_draw_flush);
        }
    }
    return pe_draw_enabled ? pe_draw_clock() : 0;
}
static void pe_draw_end(uint64_t begun, uint32_t method,
                        int32_t x, int32_t y, int32_t a, int32_t b) {
    if (!begun) { return; }
    uint64_t now = pe_draw_clock();
    if (pe_draw_count == 2048) { pe_draw_flush(); }
    pe_draw_rec *event = &pe_draw_buffer[pe_draw_count++];
    event->when_ns = now;
    event->method = method;
    event->duration_ns = (now - begun > UINT32_MAX)
        ? UINT32_MAX : (uint32_t)(now - begun);
    event->x=x; event->y=y; event->a=a; event->b=b;
    if (method == 9) { pe_draw_flush(); }
}

static mp_int_t input_mask, frames, rects, clears, pixels;
static uint8_t host_keys[256], prior_keys[256];
static mp_int_t win_left=0, win_top=0, win_right=320, win_bottom=528;
typedef struct { mp_int_t type, x, y; } touch_event;
static touch_event events[64];
static unsigned event_read, event_write;

static mp_obj_t trace_reset(void) {
    input_mask = frames = rects = clears = pixels = 0;
    memset(host_keys,0,sizeof(host_keys)); memset(prior_keys,0,sizeof(prior_keys));
    win_left=win_top=0; win_right=320; win_bottom=528;
    event_read = event_write = 0;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(trace_reset_obj, trace_reset);

static mp_obj_t trace_keys(mp_obj_t mask) {
    input_mask = mp_obj_get_int(mask);
    memset(host_keys, 0, sizeof(host_keys));
    // Legacy compact mask input, with canonical calculator key codes.
    static const unsigned char codes[] = {
        133,118,129,116,65,67,21,134,49,51,66,68,117,132,145,161,
        17,33,34,35,50,165,36,37
    };
    for (unsigned bit=0; bit<sizeof(codes); ++bit) {
        if (((uint32_t)input_mask & ((uint32_t)1<<bit)) != 0) {
            host_keys[codes[bit]]=1;
        }
    }
    return mp_const_none;
}
static mp_obj_t trace_key(mp_obj_t key, mp_obj_t down) {
    mp_int_t code=mp_obj_get_int(key);
    if (code < 0 || code >= 256) { mp_raise_ValueError(MP_ERROR_TEXT("invalid key code")); }
    host_keys[code]=mp_obj_is_true(down);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(trace_key_obj, trace_key);
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
    if ((fmt!=0 && fmt!=1 && fmt!=3 && fmt!=4 && fmt!=5 && fmt!=6)
        || w<=0 || h<=0 || stride<=0 || w>65535 || h>65535) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid image format/dimensions"));
    }
    size_t minimum=(fmt<=1)?(size_t)w*2:((fmt==3||fmt==6)?(size_t)(w+1)/2:(size_t)w);
    if ((size_t)stride<minimum || (size_t)stride > SIZE_MAX/(size_t)h) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid image stride"));
    }
    mp_get_buffer_raise(args[5], &data, MP_BUFFER_READ);
    if (data.len < (size_t)stride*(h-1)+minimum) {
        mp_raise_ValueError(MP_ERROR_TEXT("image data too short"));
    }
    if (fmt<=1) {
        if (cc!=0 || args[6]!=mp_const_none) {
            mp_raise_ValueError(MP_ERROR_TEXT("RGB image cannot have palette"));
        }
    } else {
        if (cc<1 || cc>256 || ((fmt==3||fmt==6)&&cc!=16)) {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid palette count"));
        }
        mp_get_buffer_raise(args[6], &pal, MP_BUFFER_READ);
        if (pal.len<(size_t)cc*2) {
            mp_raise_ValueError(MP_ERROR_TEXT("palette too short"));
        }
    }
    headless_image *self = mp_obj_malloc(headless_image, type);
    self->format = fmt; self->width = w; self->height = h; self->stride = stride;
    self->data = args[5]; self->palette = args[6];
    return MP_OBJ_FROM_PTR(self);
}
static void headless_image_attr(mp_obj_t object,qstr attribute,mp_obj_t *dest) {
    if(dest[0]!=MP_OBJ_NULL)return;
    headless_image *img=MP_OBJ_TO_PTR(object);
    switch(attribute) {
        case MP_QSTR_format: dest[0]=mp_obj_new_int(img->format); break;
        case MP_QSTR_flags: dest[0]=MP_OBJ_NEW_SMALL_INT(3); break;
        case MP_QSTR_color_count: dest[0]=mp_obj_new_int(img->format<=1?0:(img->format==3||img->format==6?16:256)); break;
        case MP_QSTR_width: dest[0]=mp_obj_new_int(img->width); break;
        case MP_QSTR_height: dest[0]=mp_obj_new_int(img->height); break;
        case MP_QSTR_stride: dest[0]=mp_obj_new_int(img->stride); break;
        case MP_QSTR_data: dest[0]=img->data;break;
        case MP_QSTR_palette: dest[0]=img->palette;break;
        default:break;
    }
}
static MP_DEFINE_CONST_OBJ_TYPE(headless_image_type, MP_QSTR_image, MP_TYPE_FLAG_NONE, make_new, image_new, attr, headless_image_attr);
static mp_obj_t headless_dsubimage(size_t n, const mp_obj_t *args) {
    uint64_t pe_draw_started = pe_draw_begin();
    (void)n;
    if (!mp_obj_is_type(args[2], &headless_image_type)) { mp_raise_TypeError(MP_ERROR_TEXT("expected image")); }
    headless_image *img = MP_OBJ_TO_PTR(args[2]);
    mp_int_t x = mp_obj_get_int(args[0]), y = mp_obj_get_int(args[1]);
    mp_int_t sx = mp_obj_get_int(args[3]), sy = mp_obj_get_int(args[4]);
    mp_int_t w = mp_obj_get_int(args[5]), h = mp_obj_get_int(args[6]);
    if (sx < 0 || sy < 0 || w <= 0 || h <= 0 || sx + w > img->width || sy + h > img->height) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid subimage bounds"));
    }
    mp_int_t left = x < win_left ? win_left : x, top = y < win_top ? win_top : y;
    mp_int_t right = x + w > win_right ? win_right : x + w;
    mp_int_t bottom = y + h > win_bottom ? win_bottom : y + h;
    rects++;
    if (right > left && bottom > top) { pixels += (right - left) * (bottom - top); }
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 11, x,y,w,h); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dsubimage_obj, 7, 7, headless_dsubimage);


static mp_obj_t headless_dimage(mp_obj_t x,mp_obj_t y,mp_obj_t image) {
    if(!mp_obj_is_type(image,&headless_image_type)){
        mp_raise_TypeError(MP_ERROR_TEXT("expected image"));
    }
    headless_image *img=MP_OBJ_TO_PTR(image);
    mp_obj_t args[]={x,y,image,MP_OBJ_NEW_SMALL_INT(0),MP_OBJ_NEW_SMALL_INT(0),
                     mp_obj_new_int(img->width),mp_obj_new_int(img->height)};
    return headless_dsubimage(7,args);
}
static MP_DEFINE_CONST_FUN_OBJ_3(headless_dimage_obj,headless_dimage);

static mp_obj_t headless_image_helper(int fmt,size_t n,const mp_obj_t *args) {
    mp_int_t w=mp_obj_get_int(args[0]),h=mp_obj_get_int(args[1]);
    if(w<=0 || w>65535 || h<=0 || h>65535){
        mp_raise_ValueError(MP_ERROR_TEXT("invalid image dimensions"));
    }
    mp_int_t stride=(fmt<=1)?2*w:((fmt==3||fmt==6)?(w+1)/2:w);
    mp_int_t cc=0;
    mp_obj_t palette=mp_const_none;
    if(n==4){
        palette=args[3];
        mp_buffer_info_t info;mp_get_buffer_raise(palette,&info,MP_BUFFER_READ);
        cc=info.len/2;
    }
    mp_obj_t params[]={mp_obj_new_int(fmt),mp_obj_new_int(cc),args[0],
        args[1],mp_obj_new_int(stride),args[2],palette};
    return image_new(&headless_image_type,7,0,params);
}
#define IMAGE_HELPER(name,fmt,argc) \
static mp_obj_t name(size_t n,const mp_obj_t *args){return headless_image_helper(fmt,n,args);} \
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(name##_obj,argc,argc,name)
IMAGE_HELPER(headless_image_rgb565,0,3);
IMAGE_HELPER(headless_image_rgb565a,1,3);
IMAGE_HELPER(headless_image_p8_rgb565,4,4);
IMAGE_HELPER(headless_image_p8_rgb565a,5,4);
IMAGE_HELPER(headless_image_p4_rgb565,6,4);
IMAGE_HELPER(headless_image_p4_rgb565a,3,4);

static mp_obj_t trace_stats(void) {
    mp_obj_t values[] = {mp_obj_new_int(frames), mp_obj_new_int(rects),
        mp_obj_new_int(clears), mp_obj_new_int(pixels)};
    return mp_obj_new_tuple(4, values);
}
static MP_DEFINE_CONST_FUN_OBJ_0(trace_stats_obj, trace_stats);

static mp_obj_t headless_keydown(mp_obj_t key) {
    mp_int_t code=mp_obj_get_int(key);
    return mp_obj_new_bool(code>=0 && code<256 && host_keys[code]);
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_keydown_obj, headless_keydown);
static mp_obj_t headless_clearevents(void) { return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_0(headless_clearevents_obj, headless_clearevents);
static mp_obj_t headless_dupdate(void) {
    uint64_t pe_draw_started = pe_draw_begin(); frames++; if (pe_draw_started) { pe_draw_end(pe_draw_started, 9, 0,0,0,0); }
    return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_0(headless_dupdate_obj, headless_dupdate);
static mp_obj_t headless_dclear(mp_obj_t color) {
    uint64_t pe_draw_started = pe_draw_begin();
    (void)mp_obj_get_int(color);
    clears++; pixels += 320 * 528;
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 1, 0,0,320,528); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_dclear_obj, headless_dclear);
static mp_obj_t headless_drect(size_t n, const mp_obj_t *args) {
    uint64_t pe_draw_started = pe_draw_begin();
    (void)n;
    mp_int_t x1 = mp_obj_get_int(args[0]), y1 = mp_obj_get_int(args[1]);
    mp_int_t x2 = mp_obj_get_int(args[2]), y2 = mp_obj_get_int(args[3]);
    mp_int_t color=mp_obj_get_int(args[4]);
    rects++;
    if (x1>x2) { mp_int_t swap=x1; x1=x2; x2=swap; }
    if (y1>y2) { mp_int_t swap=y1; y1=y2; y2=swap; }
    if (color==-1) { return mp_const_none; }
    if (x1 < win_left) { x1 = win_left; }
    if (y1 < win_top) { y1 = win_top; }
    if (x2 >= win_right) { x2 = win_right - 1; }
    if (y2 >= win_bottom) { y2 = win_bottom - 1; }
    if (x2 >= x1 && y2 >= y1) { pixels += (x2 - x1 + 1) * (y2 - y1 + 1); }
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 2, x1,y1,x2,y2); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_drect_obj, 5, 5, headless_drect);


// A deliberately counter-only pixel/text implementation. Both Linux and Windows
// use this same source. Drawing is clipped; no framebuffer is allocated.
static mp_obj_t headless_dpixel(mp_obj_t px, mp_obj_t py, mp_obj_t color) {
    uint64_t pe_draw_started = pe_draw_begin();
    mp_int_t x = mp_obj_get_int(px), y = mp_obj_get_int(py);
    mp_int_t c=mp_obj_get_int(color);
    rects++;
    if (c!=-1 && x >= win_left && x < win_right && y >= win_top && y < win_bottom) {
        pixels++;
    }
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 3, x,y,0,0); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(headless_dpixel_obj, headless_dpixel);

// dtext(x,y,color,text): estimate the drawn glyph footprint without rendering.
// 6x10 monospace bounding boxes are used for the headless pixel-write counter.
static mp_obj_t headless_dtext(size_t n, const mp_obj_t *args) {
    uint64_t pe_draw_started = pe_draw_begin();
    (void)n;
    mp_int_t x = mp_obj_get_int(args[0]), y = mp_obj_get_int(args[1]);
    (void)mp_obj_get_int(args[2]);
    size_t len;
    (void)mp_obj_str_get_data(args[3], &len);
    rects++;
    if (len && y < win_bottom && y + 10 > win_top && x < win_right) {
        mp_int_t left = x < win_left ? win_left : x;
        mp_int_t top = y < win_top ? win_top : y;
        mp_int_t right = x + (mp_int_t)(len > 1000 ? 1000 : len) * 6;
        if (right > win_right) { right = win_right; }
        mp_int_t bottom = y + 10;
        if (bottom > win_bottom) { bottom = win_bottom; }
        if (right > left && bottom > top) {
            pixels += (right - left) * (bottom - top);
        }
    }
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 4, x,y,(int32_t)len,0); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dtext_obj, 4, 4, headless_dtext);


// RGB565 color helper, shared by both host ports.
static mp_obj_t headless_rgb(mp_obj_t red, mp_obj_t green, mp_obj_t blue) {
    mp_int_t r = mp_obj_get_int(red), g = mp_obj_get_int(green), b = mp_obj_get_int(blue);
    if (r < 0 || r > 31 || g < 0 || g > 31 || b < 0 || b > 31) {
        mp_raise_ValueError(MP_ERROR_TEXT("RGB channel out of range"));
    }
    return mp_obj_new_int((r << 11) | (g << 6) | b);
}
static MP_DEFINE_CONST_FUN_OBJ_3(headless_rgb_obj, headless_rgb);

// Bresenham line counter, clipped per pixel; no framebuffer or allocations.
static mp_obj_t headless_dline(size_t n, const mp_obj_t *args) {
    uint64_t pe_draw_started = pe_draw_begin();
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
        if (x0 >= win_left && x0 < win_right && y0 >= win_top && y0 < win_bottom) { pixels++; }
        if (x0 == x1 && y0 == y1) { break; }
        mp_int_t twice = 2 * err;
        if (twice > -dy) { err -= dy; x0 += sx; }
        if (twice < dx) { err += dx; y0 += sy; }
    }
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 5, x0,y0,x1,y1); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dline_obj, 5, 5, headless_dline);

// Deterministic key for noninteractive scripts finishing with getkey().
static mp_obj_t headless_getkey(void) {
    static const qstr fields[] = {MP_QSTR_time, MP_QSTR_mod, MP_QSTR_shift,
        MP_QSTR_alpha, MP_QSTR_type, MP_QSTR_key, MP_QSTR_x, MP_QSTR_y};
    mp_obj_t values[] = {MP_OBJ_NEW_SMALL_INT(0), mp_const_false, mp_const_false,
        mp_const_false, MP_OBJ_NEW_SMALL_INT(1), MP_OBJ_NEW_SMALL_INT(116),
        MP_OBJ_NEW_SMALL_INT(0), MP_OBJ_NEW_SMALL_INT(0)};
    return mp_obj_new_attrtuple(fields, 8, values);
}
static MP_DEFINE_CONST_FUN_OBJ_0(headless_getkey_obj, headless_getkey);


/* Counter-only compatibility APIs: keep the same signatures on Unix and Win32.
 * Pixel counts are bounding-box estimates, not framebuffer render results. */
static void headless_box(mp_int_t x1, mp_int_t y1, mp_int_t x2, mp_int_t y2) {
    rects++;
    if (x1>x2) { mp_int_t swap=x1; x1=x2; x2=swap; }
    if (y1>y2) { mp_int_t swap=y1; y1=y2; y2=swap; }
    if (x1 < win_left) { x1 = win_left; }
    if (y1 < win_top) { y1 = win_top; }
    if (x2 >= win_right) { x2 = win_right-1; }
    if (y2 >= win_bottom) { y2 = win_bottom-1; }
    if (x2 >= x1 && y2 >= y1) {
        pixels += (x2 - x1 + 1) * (y2 - y1 + 1);
    }
}
static mp_obj_t headless_dtext_opt(size_t n, const mp_obj_t *args) {
    uint64_t pe_draw_started = pe_draw_begin();
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
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 6, x,y,(int32_t)len,0); }
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
    uint64_t pe_draw_started = pe_draw_begin();
    (void)n;
    mp_int_t x1=mp_obj_get_int(args[0]), y1=mp_obj_get_int(args[1]);
    mp_int_t x2=mp_obj_get_int(args[2]), y2=mp_obj_get_int(args[3]);
    (void)mp_obj_get_int(args[4]);
    mp_int_t border=mp_obj_get_int(args[5]);
    (void)mp_obj_get_int(args[6]);
    if (border < 0 || border > 1000) { mp_raise_ValueError(MP_ERROR_TEXT("invalid border width")); }
    headless_box(x1,y1,x2,y2);
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 7, x1,y1,x2,y2); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_drect_border_obj, 7, 7, headless_drect_border);

static mp_obj_t headless_dcircle(size_t n, const mp_obj_t *args) {
    uint64_t pe_draw_started = pe_draw_begin();
    (void)n;
    mp_int_t x=mp_obj_get_int(args[0]), y=mp_obj_get_int(args[1]);
    mp_int_t r=mp_obj_get_int(args[2]);
    (void)mp_obj_get_int(args[3]); (void)mp_obj_get_int(args[4]);
    if (r<0 || r>100000) { mp_raise_ValueError(MP_ERROR_TEXT("invalid radius")); }
    headless_box(x-r,y-r,x+r,y+r);
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 8, x,y,r,0); }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dcircle_obj, 5, 5, headless_dcircle);

static mp_obj_t headless_dellipse(size_t n, const mp_obj_t *args) {
    uint64_t pe_draw_started = pe_draw_begin();
    (void)n;
    mp_int_t x1=mp_obj_get_int(args[0]), y1=mp_obj_get_int(args[1]);
    mp_int_t x2=mp_obj_get_int(args[2]), y2=mp_obj_get_int(args[3]);
    (void)mp_obj_get_int(args[4]); (void)mp_obj_get_int(args[5]);
    // gint.dellipse takes opposite corners of a bounding rectangle,
    // not center and radii (see the published gint.pyi contract).
    headless_box(x1,y1,x2,y2);
    if (pe_draw_started) { pe_draw_end(pe_draw_started, 10, x1,y1,x2,y2); }
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

static mp_obj_t headless_cleareventflips(void) { memcpy(prior_keys,host_keys,sizeof(host_keys)); return mp_const_none; }
static MP_DEFINE_CONST_FUN_OBJ_0(headless_cleareventflips_obj, headless_cleareventflips);
static mp_obj_t headless_keypressed(mp_obj_t key) {
    mp_int_t code=mp_obj_get_int(key);
    return mp_obj_new_bool(code>=0 && code<256 && host_keys[code] && !prior_keys[code]);
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_keypressed_obj, headless_keypressed);
static mp_obj_t headless_keyreleased(mp_obj_t key) {
    mp_int_t code=mp_obj_get_int(key);
    return mp_obj_new_bool(code>=0 && code<256 && !host_keys[code] && prior_keys[code]);
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_keyreleased_obj, headless_keyreleased);


static mp_obj_t headless_dwindow_get(void) {
    mp_obj_t values[4]={mp_obj_new_int(win_left),mp_obj_new_int(win_top),
                        mp_obj_new_int(win_right),mp_obj_new_int(win_bottom)};
    return mp_obj_new_tuple(4,values);
}
static MP_DEFINE_CONST_FUN_OBJ_0(headless_dwindow_get_obj,headless_dwindow_get);
static mp_obj_t headless_dwindow_set(size_t n,const mp_obj_t *args) {
    (void)n;
    mp_obj_t previous=headless_dwindow_get();
    mp_int_t l=mp_obj_get_int(args[0]), t=mp_obj_get_int(args[1]);
    mp_int_t r=mp_obj_get_int(args[2]), b=mp_obj_get_int(args[3]);
    if(l<0){l=0;} if(t<0){t=0;}
    if(r>320){r=320;} if(b>528){b=528;}
    if(l>320){l=320;} if(t>528){t=528;}
    if(r<l){r=l;} if(b<t){b=t;}
    win_left=l;win_top=t;win_right=r;win_bottom=b;
    return previous;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_dwindow_set_obj,4,4,headless_dwindow_set);


static mp_obj_t headless_drsize(mp_obj_t text, mp_obj_t font, mp_obj_t limit) {
    (void)font;
    mp_int_t budget=mp_obj_get_int(limit);
    size_t length;
    (void)mp_obj_str_get_data(text,&length);
    size_t count=budget<=0?0:(size_t)budget/6;
    if(count>length){count=length;}
    mp_obj_t values[2]={mp_obj_new_int(count),mp_obj_new_int(count*6)};
    return mp_obj_new_tuple(2,values);
}
static MP_DEFINE_CONST_FUN_OBJ_3(headless_drsize_obj,headless_drsize);


/* Public gint keyboard/text compatibility, available in both host variants. */
static mp_obj_t headless_key_all(size_t n,const mp_obj_t *args) {
    if(n==1 && mp_obj_is_type(args[0],&mp_type_list)) {
        size_t len;mp_obj_t *items;mp_obj_get_array(args[0],&len,&items);
        for(size_t i=0;i<len;i++){if(!mp_obj_is_true(headless_keydown(items[i])))return mp_const_false;}
    } else {
        for(size_t i=0;i<n;i++){if(!mp_obj_is_true(headless_keydown(args[i])))return mp_const_false;}
    }
    return mp_const_true;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_key_all_obj,0,255,headless_key_all);
static mp_obj_t headless_key_any(size_t n,const mp_obj_t *args) {
    if(n==1 && mp_obj_is_type(args[0],&mp_type_list)) {
        size_t len;mp_obj_t *items;mp_obj_get_array(args[0],&len,&items);
        for(size_t i=0;i<len;i++){if(mp_obj_is_true(headless_keydown(items[i])))return mp_const_true;}
    } else {
        for(size_t i=0;i<n;i++){if(mp_obj_is_true(headless_keydown(args[i])))return mp_const_true;}
    }
    return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_key_any_obj,0,255,headless_key_any);
static mp_obj_t headless_key_digit(mp_obj_t key) {
    int k=mp_obj_get_int(key);
    static const int keys[]={17,33,34,35,49,50,51,65,66,67};
    for(int i=0;i<10;i++)if(k==keys[i])return mp_obj_new_int(i);
    return mp_obj_new_int(-1);
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_key_digit_obj,headless_key_digit);
static mp_obj_t headless_key_function(mp_obj_t key) {
    int k=mp_obj_get_int(key);
    return mp_obj_new_int(k>=145&&k<=150?k-144:-1);
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_key_function_obj,headless_key_function);
static mp_obj_t headless_getkey_opt(mp_obj_t opts,mp_obj_t timeout) {
    (void)mp_obj_get_int(opts);
    if(timeout!=mp_const_none){(void)mp_obj_get_int(timeout);}
    // No live human keyboard in the small headless port: timeout gives no event.
    touch_event ev={0,0,0};
    if(event_read!=event_write){ev=events[event_read++%64];}
    static const qstr fields[]={MP_QSTR_time,MP_QSTR_mod,MP_QSTR_shift,
        MP_QSTR_alpha,MP_QSTR_type,MP_QSTR_key,MP_QSTR_x,MP_QSTR_y};
    mp_obj_t values[]={MP_OBJ_NEW_SMALL_INT(0),mp_const_false,mp_const_false,
        mp_const_false,mp_obj_new_int(ev.type),MP_OBJ_NEW_SMALL_INT(0),
        mp_obj_new_int(ev.x),mp_obj_new_int(ev.y)};
    return mp_obj_new_attrtuple(fields,8,values);
}
static MP_DEFINE_CONST_FUN_OBJ_2(headless_getkey_opt_obj,headless_getkey_opt);
static mp_obj_t headless_dnsize(mp_obj_t text,mp_obj_t count,mp_obj_t font) {
    (void)font;
    size_t len; (void)mp_obj_str_get_data(text,&len);
    mp_int_t max=mp_obj_get_int(count);
    if(max>=0 && len>(size_t)max)len=(size_t)max;
    mp_obj_t pair[]={mp_obj_new_int(len*6),MP_OBJ_NEW_SMALL_INT(10)};
    return mp_obj_new_tuple(2,pair);
}
static MP_DEFINE_CONST_FUN_OBJ_3(headless_dnsize_obj,headless_dnsize);
static mp_obj_t headless_dgetpixel(mp_obj_t x,mp_obj_t y) {
    (void)mp_obj_get_int(x);(void)mp_obj_get_int(y);
    // The lightweight host deliberately does not store any pixel color.
    mp_raise_NotImplementedError(MP_ERROR_TEXT("dgetpixel requires fxcp_instrument framebuffer"));
}
static MP_DEFINE_CONST_FUN_OBJ_2(headless_dgetpixel_obj,headless_dgetpixel);


static mp_obj_t headless_font(size_t count,const mp_obj_t *args) {
    (void)args; (void)count;
    mp_raise_NotImplementedError(MP_ERROR_TEXT("custom font rasterization requires fxcp_instrument"));
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(headless_font_obj,13,13,headless_font);
static mp_obj_t headless_dfont(mp_obj_t value) {
    if (value!=mp_const_none) {
        int n=mp_obj_get_int(value);
        if (n!=9 && n!=89) { mp_raise_NotImplementedError(MP_ERROR_TEXT("custom font unsupported")); }
    }
    return MP_OBJ_NEW_SMALL_INT(89);
}
static MP_DEFINE_CONST_FUN_OBJ_1(headless_dfont_obj,headless_dfont);

#ifdef PE_HEADLESS_UNIX_USB
extern const mp_obj_type_t pe_linux_usb_type;
#define HEADLESS_USB_GLOBAL {MP_ROM_QSTR(MP_QSTR_USB), MP_ROM_PTR(&pe_linux_usb_type)},
#else
#define HEADLESS_USB_GLOBAL
#endif
#define FUN(name, obj) {MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_PTR(&obj)}
#define INT(name, val) {MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_INT(val)}
static const mp_rom_map_elem_t globals_table[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_gint)},
    HEADLESS_USB_GLOBAL
    INT(I, 0),
    INT(GETKEY_MOD_SHIFT, 0x0001), INT(GETKEY_MOD_ALPHA, 0x0002),
    INT(GETKEY_BACKLIGHT, 0x0004), INT(GETKEY_MENU, 0x0008),
    INT(GETKEY_REP_ARROWS, 0x0010), INT(GETKEY_REP_ALL, 0x0020),
    INT(GETKEY_REP_PROFILE, 0x0040), INT(GETKEY_FEATURES, 0x0080),
    INT(GETKEY_NONE, 0), INT(GETKEY_DEFAULT, 0x05df),
    INT(C_LIGHTEN, -3), INT(C_DARKEN, -4),
    INT(IMAGE_MONO, -10), INT(IMAGE_MONO_ALPHA, -11),
    INT(IMAGE_GRAY, -12), INT(IMAGE_GRAY_ALPHA, -13),
    INT(DWIDTH, 320), INT(DHEIGHT, 528),
    INT(C_WHITE, 65535), INT(C_BLACK, 0), INT(C_NONE, -1), INT(C_INVERT, -2),
    INT(C_RED, 63488), INT(C_GREEN, 2016), INT(C_BLUE, 31),
    INT(C_LIGHT, 44373), INT(C_DARK, 21130),
    INT(DTEXT_LEFT, 0), INT(DTEXT_CENTER, 1), INT(DTEXT_RIGHT, 2),
    INT(DTEXT_TOP, 0), INT(DTEXT_MIDDLE, 1), INT(DTEXT_BOTTOM, 2),
    INT(font_9, 9), INT(font8x9, 89),
    #include "../fxcp_instrument/key_constants.inc"
    INT(KEYEV_NONE, 0), INT(KEYEV_DOWN, 1), INT(KEYEV_UP, 2),
    INT(KEYEV_HOLD, 3), INT(KEYEV_OSMENU, 4),
    INT(KEYEV_TOUCH_DOWN, 5), INT(KEYEV_TOUCH_DRAG, 6), INT(KEYEV_TOUCH_UP, 7),
    INT(IMAGE_RGB565, 0), INT(IMAGE_RGB565A, 1),
    INT(IMAGE_P8_RGB565, 4), INT(IMAGE_P8_RGB565A, 5),
    INT(IMAGE_P4_RGB565, 6), INT(IMAGE_P4_RGB565A, 3),
    INT(IMAGE_FLAGS_DATA_RO, 1), INT(IMAGE_FLAGS_PALETTE_RO, 2),
    INT(IMAGE_FLAGS_DATA_ALLOC, 4), INT(IMAGE_FLAGS_PALETTE_ALLOC, 8),
    FUN(image, headless_image_type), FUN(dsubimage, headless_dsubimage_obj),
    FUN(dimage, headless_dimage_obj),
    FUN(image_rgb565, headless_image_rgb565_obj), FUN(image_rgb565a, headless_image_rgb565a_obj),
    FUN(image_p8_rgb565, headless_image_p8_rgb565_obj), FUN(image_p8_rgb565a, headless_image_p8_rgb565a_obj),
    FUN(image_p4_rgb565, headless_image_p4_rgb565_obj), FUN(image_p4_rgb565a, headless_image_p4_rgb565a_obj),
    FUN(trace_touch, trace_touch_obj), FUN(pollevent, headless_pollevent_obj),
    FUN(keydown, headless_keydown_obj), FUN(clearevents, headless_clearevents_obj),
    FUN(keydown_all, headless_key_all_obj), FUN(keydown_any, headless_key_any_obj),
    FUN(keycode_digit, headless_key_digit_obj), FUN(keycode_function, headless_key_function_obj),
    FUN(getkey_opt, headless_getkey_opt_obj),
    FUN(dupdate, headless_dupdate_obj), FUN(dclear, headless_dclear_obj),
    FUN(drect, headless_drect_obj), FUN(dpixel, headless_dpixel_obj),
    FUN(dtext, headless_dtext_obj), FUN(dline, headless_dline_obj),
    FUN(C_RGB, headless_rgb_obj), FUN(getkey, headless_getkey_obj),
    FUN(dtext_opt, headless_dtext_opt_obj), FUN(dsize, headless_dsize_obj),
    FUN(drsize, headless_drsize_obj), FUN(dnsize, headless_dnsize_obj),
    FUN(dgetpixel, headless_dgetpixel_obj), FUN(font, headless_font_obj),
    FUN(dfont, headless_dfont_obj),
    FUN(drect_border, headless_drect_border_obj), FUN(dcircle, headless_dcircle_obj),
    FUN(dellipse, headless_dellipse_obj), FUN(dpoly, headless_dpoly_obj),
    FUN(dhline, headless_dhline_obj), FUN(dvline, headless_dvline_obj),
    FUN(dwindow_get, headless_dwindow_get_obj), FUN(dwindow_set, headless_dwindow_set_obj),
    FUN(cleareventflips, headless_cleareventflips_obj),
    FUN(keypressed, headless_keypressed_obj), FUN(keyreleased, headless_keyreleased_obj),
    FUN(trace_reset, trace_reset_obj),
    FUN(trace_keys, trace_keys_obj), FUN(trace_key, trace_key_obj), FUN(trace_stats, trace_stats_obj),
};
static MP_DEFINE_CONST_DICT(globals, globals_table);
const mp_obj_module_t headless_gint = { .base = {&mp_type_module}, .globals = (mp_obj_dict_t *)&globals };
MP_REGISTER_MODULE(MP_QSTR_gint, headless_gint);
