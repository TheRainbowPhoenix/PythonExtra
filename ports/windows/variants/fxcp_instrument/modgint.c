// CPU rendering/input backend for the existing PythonExtra Windows port.
#include "py/runtime.h"
#include "py/objtuple.h"
#include "py/objarray.h"
#include "pe_render.h"
#include <string.h>
#include "font_data.h"
#include "pe_trace.h"

static bool held[256], was_held[256];
static unsigned font_height=9;
static bool small_font;
static int glyph_width(unsigned c, bool small) {return small?pe_font_width[c-32]:pe_default_width[c-32];}
static bool select_font(mp_obj_t font) {
    if(font==mp_const_none) {return small_font;}
    int size=mp_obj_get_int(font);
    if(size==9 || size==10) {return true;}
    if(size==89) {return false;}
    mp_raise_NotImplementedError(MP_ERROR_TEXT("unsupported custom font"));
}
static int integer(mp_obj_t value) {return mp_obj_get_int(value);}
#define I(n) integer(args[n])
#define FN0(name) static MP_DEFINE_CONST_FUN_OBJ_0(name##_obj,name)
#define FN1(name) static MP_DEFINE_CONST_FUN_OBJ_1(name##_obj,name)
#define FN2(name) static MP_DEFINE_CONST_FUN_OBJ_2(name##_obj,name)
#define FN3(name) static MP_DEFINE_CONST_FUN_OBJ_3(name##_obj,name)
#define FNV(name,lo,hi) static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(name##_obj,lo,hi,name)

static mp_obj_t poll_event(void) {
    pe_event ev={0}; pe_poll(&ev);
    if(ev.key<256) {
        if(ev.type==1) {held[ev.key]=true;}
        if(ev.type==2) {held[ev.key]=false;}
    }
    static const qstr fields[]={MP_QSTR_time,MP_QSTR_mod,MP_QSTR_shift,MP_QSTR_alpha,
        MP_QSTR_type,MP_QSTR_key,MP_QSTR_x,MP_QSTR_y};
    uint64_t stamp=ev.type?((uint64_t)ev.hi<<32)|ev.lo:pe_clock_us();
    mp_obj_t values[]={mp_obj_new_int((stamp*128/1000000)&65535),mp_const_false,
        mp_const_false,mp_const_false,mp_obj_new_int(ev.type),mp_obj_new_int(ev.key),
        mp_obj_new_int(ev.x),mp_obj_new_int(ev.y)};
    return mp_obj_new_attrtuple(fields,8,values);
}
FN0(poll_event);
static mp_obj_t key_down(mp_obj_t key) {int k=integer(key);return mp_obj_new_bool(k>=0&&k<256&&held[k]);} FN1(key_down);
static mp_obj_t clear_events(void) {
    pe_event ev;
    while(pe_poll(&ev)) {
        if(ev.key<256 && (ev.type==1||ev.type==2)) {held[ev.key]=ev.type==1;}
    }
    return mp_const_none;
} FN0(clear_events);
static mp_obj_t clear_flips(void) {memcpy(was_held,held,sizeof(held));return mp_const_none;} FN0(clear_flips);
static mp_obj_t key_pressed(mp_obj_t obj) {int k=integer(obj);return mp_obj_new_bool(k>=0&&k<256&&held[k]&&!was_held[k]);} FN1(key_pressed);
static mp_obj_t key_released(mp_obj_t obj) {int k=integer(obj);return mp_obj_new_bool(k>=0&&k<256&&!held[k]&&was_held[k]);} FN1(key_released);
static mp_obj_t key_all(size_t n,const mp_obj_t *args) {for(size_t i=0;i<n;i++){if(!mp_obj_is_true(key_down(args[i]))){return mp_const_false;}}return mp_const_true;} FNV(key_all,0,255);
static mp_obj_t key_any(size_t n,const mp_obj_t *args) {for(size_t i=0;i<n;i++){if(mp_obj_is_true(key_down(args[i]))){return mp_const_true;}}return mp_const_false;} FNV(key_any,0,255);
static mp_obj_t get_key(void) {
    for(;;) {
        mp_obj_t ev=poll_event();
        mp_obj_t type=mp_load_attr(ev,MP_QSTR_type);
        if(integer(type)==1||integer(type)==3||integer(type)>=5) {return ev;}
        pe_delay_us(1000);
    }
} FN0(get_key);
static mp_obj_t get_key_opt(mp_obj_t opts,mp_obj_t timeout) {
    // Host has no calculator backlight/repeat policy; accept the documented
    // option bitmask instead of rejecting regular GETKEY_DEFAULT callers.
    (void)integer(opts);
    int ms=timeout==mp_const_none?-1:integer(timeout);
    uint64_t end=pe_clock_us()+(ms<0?0x7fffffffffffffffULL:(uint64_t)ms*1000);
    for(;;){mp_obj_t ev=poll_event();int type=integer(mp_load_attr(ev,MP_QSTR_type));
        if(type==1||type==3||type>=5||pe_clock_us()>=end){return ev;}pe_delay_us(1000);}
} FN2(get_key_opt);
static mp_obj_t key_digit(mp_obj_t key) {
    int k=integer(key);static const int digits[]={0x11,0x21,0x22,0x23,0x31,0x32,0x33,0x41,0x42,0x43};
    for(int i=0;i<10;i++){if(k==digits[i]){return mp_obj_new_int(i);}}return mp_obj_new_int(-1);
} FN1(key_digit);
static mp_obj_t key_function(mp_obj_t key) {int k=integer(key);return mp_obj_new_int(k>=0x91&&k<=0x96?k-0x90:-1);} FN1(key_function);

static mp_obj_t clear_screen(mp_obj_t color) {
    pe_init(); int c=(uint16_t)integer(color); struct dwindow old=dwindow; dwindow=(struct dwindow){0,0,PE_W,PE_H};
    drect(0,0,PE_W-1,PE_H-1,c); dwindow=old;pe_draw_count(PE_PIXELS);return mp_const_none;
} FN1(clear_screen);
static mp_obj_t update_screen(void) {
    pe_present();
    if(pe_virtual() && pe_h[H_FRAME_US]){pe_delay_us(pe_h[H_FRAME_US]);}
    if(pe_h[H_FLAGS]&2){pe_checkpoint(0,NULL);}return mp_const_none;
} FN0(update_screen);
static mp_obj_t set_pixel(mp_obj_t x,mp_obj_t y,mp_obj_t color) {pe_init();dpixel(integer(x),integer(y),integer(color));pe_draw_count(1);return mp_const_none;} FN3(set_pixel);
static mp_obj_t get_pixel(mp_obj_t x,mp_obj_t y) {pe_init();return mp_obj_new_int(dgetpixel(integer(x),integer(y)));} FN2(get_pixel);
static mp_obj_t rect(size_t n,const mp_obj_t *args) {(void)n;drect(I(0),I(1),I(2),I(3),I(4));pe_draw_count(0);return mp_const_none;} FNV(rect,5,5);
static mp_obj_t line(size_t n,const mp_obj_t *args) {(void)n;pe_init();dline(I(0),I(1),I(2),I(3),I(4));pe_draw_count(0);return mp_const_none;} FNV(line,5,5);
static mp_obj_t rect_border(size_t n,const mp_obj_t *args) {(void)n;drect_border(I(0),I(1),I(2),I(3),I(4),I(5),I(6));pe_draw_count(0);return mp_const_none;} FNV(rect_border,7,7);
static mp_obj_t hline(mp_obj_t y,mp_obj_t c) {drect(dwindow.left,integer(y),dwindow.right-1,integer(y),integer(c));pe_draw_count(0);return mp_const_none;} FN2(hline);
static mp_obj_t vline(mp_obj_t x,mp_obj_t c) {drect(integer(x),dwindow.top,integer(x),dwindow.bottom-1,integer(c));pe_draw_count(0);return mp_const_none;} FN2(vline);
static mp_obj_t circle(size_t n,const mp_obj_t *args) {(void)n;pe_init();dcircle(I(0),I(1),I(2),I(3),I(4));pe_draw_count(0);return mp_const_none;} FNV(circle,5,5);
static mp_obj_t ellipse(size_t n,const mp_obj_t *args) {(void)n;pe_init();dellipse(I(0),I(1),I(2),I(3),I(4),I(5));pe_draw_count(0);return mp_const_none;} FNV(ellipse,6,6);
static mp_obj_t polygon(mp_obj_t coords,mp_obj_t fill,mp_obj_t border) {
    pe_init();
    size_t n;mp_obj_t *values;mp_obj_get_array(coords,&n,&values);
    if(n%2||n<6||n>8192){mp_raise_ValueError(MP_ERROR_TEXT("polygon coordinate count"));}
    int *x=m_new(int,n/2),*y=m_new(int,n/2);
    for(size_t i=0;i<n/2;i++){x[i]=integer(values[2*i]);y[i]=integer(values[2*i+1]);}
    dpoly(x,y,n/2,integer(fill),integer(border));m_del(int,x,n/2);m_del(int,y,n/2);pe_draw_count(0);return mp_const_none;
} FN3(polygon);
static mp_obj_t window_get(void) {mp_obj_t a[]={mp_obj_new_int(dwindow.left),mp_obj_new_int(dwindow.top),mp_obj_new_int(dwindow.right),mp_obj_new_int(dwindow.bottom)};return mp_obj_new_tuple(4,a);} FN0(window_get);
static mp_obj_t window_set(size_t n,const mp_obj_t *args) {(void)n;mp_obj_t old=window_get();int l=max(0,min(PE_W,I(0))),t=max(0,min(PE_H,I(1)));dwindow=(struct dwindow){l,t,max(l,min(PE_W,I(2))),max(t,min(PE_H,I(3)))};return old;} FNV(window_set,4,4);
static mp_obj_t rgb(mp_obj_t r,mp_obj_t g,mp_obj_t b) {return mp_obj_new_int((integer(r)<<11)|(integer(g)<<6)|integer(b));} FN3(rgb);

typedef struct {mp_obj_base_t base;int fmt,cc,w,h,stride;mp_obj_t data,palette;} host_image;
static mp_obj_t image_make(const mp_obj_type_t *type,size_t n,size_t kw,const mp_obj_t *args) {
    enum {FMT,CC,W,H,STRIDE,DATA,PAL};
    static const mp_arg_t allowed[]={
      {MP_QSTR_format,MP_ARG_INT|MP_ARG_REQUIRED,{.u_int=0}},
      {MP_QSTR_color_count,MP_ARG_INT|MP_ARG_REQUIRED,{.u_int=0}},
      {MP_QSTR_width,MP_ARG_INT|MP_ARG_REQUIRED,{.u_int=0}},
      {MP_QSTR_height,MP_ARG_INT|MP_ARG_REQUIRED,{.u_int=0}},
      {MP_QSTR_stride,MP_ARG_INT|MP_ARG_REQUIRED,{.u_int=0}},
      {MP_QSTR_data,MP_ARG_OBJ|MP_ARG_REQUIRED,{.u_obj=MP_OBJ_NULL}},
      {MP_QSTR_palette,MP_ARG_OBJ,{.u_rom_obj=MP_ROM_NONE}}};
    mp_arg_val_t a[7];mp_arg_parse_all_kw_array(n,kw,args,7,allowed,a);
    int f=a[FMT].u_int,w=a[W].u_int,h=a[H].u_int,s=a[STRIDE].u_int,cc=a[CC].u_int;
    if(!(f==0||f==1||f==3||f==4||f==5||f==6)||w<=0||h<=0||w>65535||h>65535||s<=0){mp_raise_ValueError(MP_ERROR_TEXT("invalid image dimensions/format"));}
    int row=(f<=1)?w*2:((f==3||f==6)?(w+1)/2:w);
    mp_buffer_info_t buf;mp_get_buffer_raise(a[DATA].u_obj,&buf,MP_BUFFER_READ);
    if((uint64_t)s*h>buf.len||(uint64_t)(h-1)*s+row>buf.len){mp_raise_ValueError(MP_ERROR_TEXT("image data too short"));}
    if(f<=1){if(cc>0||a[PAL].u_obj!=mp_const_none){mp_raise_ValueError(MP_ERROR_TEXT("RGB image palette"));}}
    else{mp_get_buffer_raise(a[PAL].u_obj,&buf,MP_BUFFER_READ);if(cc<1||cc>256||buf.len<(size_t)cc*2||((f==3||f==6)&&cc!=16)){mp_raise_ValueError(MP_ERROR_TEXT("invalid palette"));}}
    host_image *im=mp_obj_malloc(host_image,type);im->fmt=f;im->cc=cc;im->w=w;im->h=h;im->stride=s;im->data=a[DATA].u_obj;im->palette=a[PAL].u_obj;return MP_OBJ_FROM_PTR(im);
}
static void image_attr(mp_obj_t obj,qstr attr,mp_obj_t *dest) {
    if(dest[0]!=MP_OBJ_NULL){mp_raise_msg(&mp_type_AttributeError,MP_ERROR_TEXT("image attributes are read-only"));}
    host_image *im=MP_OBJ_TO_PTR(obj);
    if(attr==MP_QSTR_width){dest[0]=mp_obj_new_int(im->w);}else if(attr==MP_QSTR_height){dest[0]=mp_obj_new_int(im->h);}
    else if(attr==MP_QSTR_format){dest[0]=mp_obj_new_int(im->fmt);}else if(attr==MP_QSTR_stride){dest[0]=mp_obj_new_int(im->stride);}
    else if(attr==MP_QSTR_color_count){dest[0]=mp_obj_new_int(im->cc);}else if(attr==MP_QSTR_flags){dest[0]=MP_OBJ_NEW_SMALL_INT(3);}else if(attr==MP_QSTR_data){dest[0]=im->data;}else if(attr==MP_QSTR_palette){dest[0]=im->palette;}
}
static MP_DEFINE_CONST_OBJ_TYPE(image_type,MP_QSTR_image,MP_TYPE_FLAG_NONE,make_new,image_make,attr,image_attr);
static uint16_t word(const mp_buffer_info_t *buf,unsigned offset) {
    const uint8_t *p=(const uint8_t *)buf->buf+offset;
    if((buf->typecode&0x7f)=='H'||(buf->typecode&0x7f)=='h'){uint16_t v;memcpy(&v,p,2);return v;}
    return ((unsigned)p[0]<<8)|p[1];
}
static void blit(int x,int y,mp_obj_t obj,int sx,int sy,int width,int height) {
    pe_init();
    if(!mp_obj_is_type(obj,&image_type)){mp_raise_TypeError(MP_ERROR_TEXT("expected gint.image"));}
    host_image *im=MP_OBJ_TO_PTR(obj);
    if(width<=0||height<=0){return;}
    if(sx<0){x-=sx;width+=sx;sx=0;}if(sy<0){y-=sy;height+=sy;sy=0;}
    width=min(width,im->w-sx);height=min(height,im->h-sy);
    if(x<dwindow.left){int d=dwindow.left-x;sx+=d;width-=d;x+=d;}
    if(y<dwindow.top){int d=dwindow.top-y;sy+=d;height-=d;y+=d;}
    width=min(width,dwindow.right-x);height=min(height,dwindow.bottom-y);
    if(width<=0||height<=0){return;}
    if(height>255){pe_h[H_BLIT_WARNINGS]++;if(pe_h[H_FLAGS]&8){mp_raise_ValueError(MP_ERROR_TEXT("fxcp native blit height exceeds the 8-bit command field"));}}
    mp_buffer_info_t data,pal;mp_get_buffer_raise(im->data,&data,MP_BUFFER_READ);
    if(im->fmt>1){mp_get_buffer_raise(im->palette,&pal,MP_BUFFER_READ);}
    const uint8_t *bytes=data.buf;
    unsigned written=0;
    for(int row=0;row<height;row++){
        for(int col=0;col<width;col++){
            int xx=sx+col,yy=sy+row,index=0;uint16_t c;
            if(im->fmt<=1){c=word(&data,yy*im->stride+xx*2);if(im->fmt==1&&c==1){continue;}}
            else{if(im->fmt==3||im->fmt==6){int b=bytes[yy*im->stride+xx/2];index=(xx&1)?b&15:b>>4;}
                else{index=(int)(int8_t)bytes[yy*im->stride+xx]+128;}
                if((im->fmt==3||im->fmt==5)&&index==0){continue;}
                if((size_t)(index+1)*2>pal.len){mp_raise_ValueError(MP_ERROR_TEXT("image palette index outside palette buffer"));}
                c=word(&pal,index*2);
            }
            pe_vram[(y+row)*PE_W+x+col]=c;
            written++;
        }
    }
    pe_draw_count(0);pe_pixel_count(written);
}
static mp_obj_t subimage(size_t n,const mp_obj_t *args){(void)n;blit(I(0),I(1),args[2],I(3),I(4),I(5),I(6));return mp_const_none;}FNV(subimage,7,7);
static mp_obj_t draw_image(mp_obj_t x,mp_obj_t y,mp_obj_t obj){if(!mp_obj_is_type(obj,&image_type)){mp_raise_TypeError(MP_ERROR_TEXT("expected gint.image"));}host_image *im=MP_OBJ_TO_PTR(obj);blit(integer(x),integer(y),obj,0,0,im->w,im->h);return mp_const_none;}FN3(draw_image);
static mp_obj_t image_helper(int fmt,size_t n,const mp_obj_t *args){int w=I(0),h=I(1),cc=0;mp_obj_t pal=mp_const_none;if(n==4){pal=args[3];cc=mp_obj_get_int(mp_obj_len(pal))/2;}mp_obj_t a[]={mp_obj_new_int(fmt),mp_obj_new_int(cc),args[0],args[1],mp_obj_new_int(fmt<=1?w*2:(fmt==3||fmt==6?(w+1)/2:w)),args[2],pal};(void)h;return image_make(&image_type,7,0,a);}
#define IMGHELP(name,fmt,argc) static mp_obj_t name(size_t n,const mp_obj_t *args){return image_helper(fmt,n,args);}FNV(name,argc,argc)
IMGHELP(img_rgb,0,3);IMGHELP(img_rgba,1,3);IMGHELP(img_p8,4,4);IMGHELP(img_p8a,5,4);IMGHELP(img_p4,6,4);IMGHELP(img_p4a,3,4);

static int text_width_font(const char *s,int count,bool small) {int w=0;for(int i=0;s[i]&&(count<0||i<count);i++){unsigned c=(unsigned char)s[i];if(c<32||c>126){mp_raise_NotImplementedError(MP_ERROR_TEXT("text supports ASCII 32..126"));}w+=glyph_width(c,small)+1;}return w?w-1:0;}
static int text_width(const char *s,int count) {return text_width_font(s,count,small_font);}
static mp_obj_t font_set(mp_obj_t font){mp_obj_t old=mp_obj_new_int(small_font?9:89);small_font=font==mp_const_none?false:select_font(font);font_height=small_font?10:9;return old;}FN1(font_set);
static mp_obj_t size_text(mp_obj_t text,mp_obj_t font){bool small=select_font(font);mp_obj_t a[]={mp_obj_new_int(text_width_font(mp_obj_str_get_str(text),-1,small)),mp_obj_new_int(small?10:9)};return mp_obj_new_tuple(2,a);}FN2(size_text);
static mp_obj_t nsize_text(mp_obj_t text,mp_obj_t size,mp_obj_t font){bool small=select_font(font);mp_obj_t a[]={mp_obj_new_int(text_width_font(mp_obj_str_get_str(text),integer(size),small)),mp_obj_new_int(small?10:9)};return mp_obj_new_tuple(2,a);}FN3(nsize_text);
static mp_obj_t rsize_text(mp_obj_t text,mp_obj_t font,mp_obj_t limit){bool small=select_font(font);const unsigned char *s=(const unsigned char *)mp_obj_str_get_str(text);int used=0,n=0,bound=integer(limit);while(s[n]){unsigned c=s[n];if(c<32||c>126){mp_raise_NotImplementedError(MP_ERROR_TEXT("text supports ASCII 32..126"));}int next=used+(n?1:0)+glyph_width(c,small);if(next>bound){break;}used=next;n++;}mp_obj_t a[]={mp_obj_new_int(n),mp_obj_new_int(used)};return mp_obj_new_tuple(2,a);}FN3(rsize_text);
static void render_text(int x,int y,int fg,int bg,const char *s,int count){pe_init();for(int i=0;s[i]&&(count<0||i<count);i++){unsigned c=(unsigned char)s[i];if(c<32||c>126){mp_raise_NotImplementedError(MP_ERROR_TEXT("text supports ASCII 32..126"));}int idx=c-32,w=glyph_width(c,small_font);for(int yy=0;yy<(small_font?10:11);yy++){unsigned row=small_font?pe_font_rows[idx][yy]:pe_default_rows[idx][yy];for(int xx=0;xx<w;xx++){dpixel(x+xx,y+yy,(row&(1<<xx))?fg:bg);}}x+=w+1;}pe_draw_count(0);}
static mp_obj_t text(size_t n,const mp_obj_t *args){(void)n;render_text(I(0),I(1),I(2),C_NONE,mp_obj_str_get_str(args[3]),-1);return mp_const_none;}FNV(text,4,4);
static mp_obj_t text_opt(size_t n,const mp_obj_t *args){(void)n;const char *s=mp_obj_str_get_str(args[6]);int count=I(7),w=text_width(s,count),x=I(0),y=I(1);int ha=I(4),va=I(5);if(ha==1){x-=w/2;}else if(ha==2){x-=w;}if(va==1){y-=font_height/2;}else if(va==2){y-=font_height;}render_text(x,y,I(2),I(3),s,count);return mp_const_none;}FNV(text_opt,8,8);
static mp_obj_t checkpoint(size_t n,const mp_obj_t *args){if(n>64){mp_raise_ValueError(MP_ERROR_TEXT("at most 64 state integers"));}int32_t values[64];for(size_t i=0;i<n;i++){values[i]=integer(args[i]);}pe_checkpoint(n,values);return mp_const_none;}FNV(checkpoint,0,64);
static mp_obj_t clock_advance(mp_obj_t us){int n=integer(us);if(n<0){mp_raise_ValueError(MP_ERROR_TEXT("negative advance"));}pe_delay_us(n);return mp_const_none;}FN1(clock_advance);
static mp_obj_t framebuffer(void){pe_init();return mp_obj_new_memoryview('H',PE_PIXELS,pe_vram);}FN0(framebuffer);
static mp_obj_t publish(void){pe_publish();return mp_const_none;}FN0(publish);
static mp_obj_t debug_memory(void){void *p=pe_mapping();if(!p){return mp_const_none;}return mp_obj_new_memoryview('B'|MP_OBJ_ARRAY_TYPECODE_FLAG_RW,PE_MAP_SIZE,p);}FN0(debug_memory);
static mp_obj_t debug_service(void){pe_debug_service();return mp_const_none;}FN0(debug_service);
static mp_obj_t wait_start(void){pe_wait_start();return mp_const_none;}FN0(wait_start);
static mp_obj_t debug_emit(mp_obj_t kind,mp_obj_t line,mp_obj_t name){pe_trace(integer(kind),integer(line),0,mp_obj_str_get_str(name));return mp_const_none;}FN3(debug_emit);


static mp_obj_t font_construct(size_t n,const mp_obj_t *args) {
    (void)n;(void)args;
    mp_raise_NotImplementedError(MP_ERROR_TEXT("custom font rasterization is not implemented on the host"));
}
FNV(font_construct,13,13);

#define FUN(name,obj) {MP_ROM_QSTR(MP_QSTR_##name),MP_ROM_PTR(&obj##_obj)}
#define INT(name,value) {MP_ROM_QSTR(MP_QSTR_##name),MP_ROM_INT(value)}
extern const mp_obj_type_t pe_usb_type;
static const mp_rom_map_elem_t globals_table[]={
 {MP_ROM_QSTR(MP_QSTR___name__),MP_ROM_QSTR(MP_QSTR_gint)},
 {MP_ROM_QSTR(MP_QSTR_USB),MP_ROM_PTR(&pe_usb_type)},
    INT(I, 0),
    INT(GETKEY_MOD_SHIFT, 0x0001), INT(GETKEY_MOD_ALPHA, 0x0002),
    INT(GETKEY_BACKLIGHT, 0x0004), INT(GETKEY_MENU, 0x0008),
    INT(GETKEY_REP_ARROWS, 0x0010), INT(GETKEY_REP_ALL, 0x0020),
    INT(GETKEY_REP_PROFILE, 0x0040), INT(GETKEY_FEATURES, 0x0080),
    INT(GETKEY_NONE, 0), INT(GETKEY_DEFAULT, 0x05df),
    INT(C_LIGHTEN, -3), INT(C_DARKEN, -4),
    INT(IMAGE_MONO, -10), INT(IMAGE_MONO_ALPHA, -11),
    INT(IMAGE_GRAY, -12), INT(IMAGE_GRAY_ALPHA, -13),
 INT(DWIDTH,PE_W),INT(DHEIGHT,PE_H),INT(C_WHITE,65535),INT(C_BLACK,0),INT(C_NONE,-1),INT(C_INVERT,-2),
 INT(C_RED,0xf800),INT(C_GREEN,0x07e0),INT(C_BLUE,0x001f),INT(C_LIGHT,0xad55),INT(C_DARK,0x528a),
 INT(IMAGE_FLAGS_DATA_RO,1), INT(IMAGE_FLAGS_PALETTE_RO,2),
 INT(IMAGE_FLAGS_DATA_ALLOC,4), INT(IMAGE_FLAGS_PALETTE_ALLOC,8),
 INT(IMAGE_RGB565,0),INT(IMAGE_RGB565A,1),INT(IMAGE_P4_RGB565A,3),INT(IMAGE_P8_RGB565,4),INT(IMAGE_P8_RGB565A,5),INT(IMAGE_P4_RGB565,6),
 INT(KEYEV_NONE,0),INT(KEYEV_DOWN,1),INT(KEYEV_UP,2),INT(KEYEV_HOLD,3),INT(KEYEV_OSMENU,4),INT(KEYEV_TOUCH_DOWN,5),INT(KEYEV_TOUCH_DRAG,6),INT(KEYEV_TOUCH_UP,7),
 INT(DTEXT_LEFT,0),INT(DTEXT_CENTER,1),INT(DTEXT_RIGHT,2),INT(DTEXT_TOP,0),INT(DTEXT_MIDDLE,1),INT(DTEXT_BOTTOM,2),
 INT(font_9,9),
 INT(font8x9,89),
 #include "key_constants.inc"
 FUN(pollevent,poll_event),FUN(keydown,key_down),FUN(keydown_all,key_all),FUN(keydown_any,key_any),FUN(clearevents,clear_events),FUN(cleareventflips,clear_flips),FUN(keypressed,key_pressed),FUN(keyreleased,key_released),FUN(getkey,get_key),FUN(getkey_opt,get_key_opt),FUN(keycode_digit,key_digit),FUN(keycode_function,key_function),
 FUN(C_RGB,rgb),FUN(dclear,clear_screen),FUN(dupdate,update_screen),FUN(dpixel,set_pixel),FUN(dgetpixel,get_pixel),FUN(drect,rect),FUN(drect_border,rect_border),FUN(dline,line),FUN(dhline,hline),FUN(dvline,vline),FUN(dcircle,circle),FUN(dellipse,ellipse),FUN(dpoly,polygon),FUN(dwindow_get,window_get),FUN(dwindow_set,window_set),
 {MP_ROM_QSTR(MP_QSTR_image),MP_ROM_PTR(&image_type)},
 FUN(dimage,draw_image),FUN(dsubimage,subimage),FUN(image_rgb565,img_rgb),FUN(image_rgb565a,img_rgba),FUN(image_p4_rgb565,img_p4),FUN(image_p4_rgb565a,img_p4a),FUN(image_p8_rgb565,img_p8),FUN(image_p8_rgb565a,img_p8a),
 FUN(font,font_construct),FUN(dtext,text),FUN(dtext_opt,text_opt),FUN(dfont,font_set),FUN(dsize,size_text),FUN(dnsize,nsize_text),
 FUN(drsize,rsize_text),
 FUN(host_checkpoint,checkpoint),FUN(host_advance,clock_advance),FUN(host_framebuffer,framebuffer),FUN(host_publish,publish),
 FUN(host_debug_memory,debug_memory),
 FUN(host_debug_service,debug_service),
 FUN(host_wait_start,wait_start),
 FUN(host_debug_emit,debug_emit),
};
static MP_DEFINE_CONST_DICT(globals,globals_table);
const mp_obj_module_t instrument_gint={.base={&mp_type_module},.globals=(mp_obj_dict_t *)&globals};
MP_REGISTER_MODULE(MP_QSTR_gint,instrument_gint);
