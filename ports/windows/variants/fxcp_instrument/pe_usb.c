/* Virtual ff-bulk endpoint. No physical USB/device discovery is performed. */
#include "py/runtime.h"
#include "py/mperrno.h"
#include "pe_host.h"
#include <windows.h>
#include <string.h>
#define CAP 16384
typedef struct {mp_obj_base_t base;} pe_usb;
static volatile uint32_t *control(void) {
    char *map=pe_mapping();
    if(!map) {mp_raise_OSError(MP_ENODEV);}
    return (volatile uint32_t *)(map+1260);
}
static void opened(void) {if(!control()[0]) {mp_raise_OSError(MP_ENODEV);}}
static mp_obj_t usb_open(mp_obj_t self) {(void)self;control()[0]=1;return mp_const_none;}
static MP_DEFINE_CONST_FUN_OBJ_1(usb_open_obj,usb_open);
static mp_obj_t usb_close(mp_obj_t self) {(void)self;control()[0]=0;return mp_const_none;}
static MP_DEFINE_CONST_FUN_OBJ_1(usb_close_obj,usb_close);
static void transmit(const void *data,size_t size) {
    opened();volatile uint32_t *h=control();uint32_t w=h[3],r=h[4];
    if(size>CAP || w-r>CAP || size>CAP-(w-r)) {mp_raise_OSError(MP_EAGAIN);}
    char *ring=(char *)pe_mapping()+944384;
    size_t first=size<CAP-w%CAP?size:CAP-w%CAP;
    memcpy(ring+w%CAP,data,first);memcpy(ring,(const char *)data+first,size-first);
    MemoryBarrier();h[3]=w+size;
}
static mp_obj_t usb_write(mp_obj_t self,mp_obj_t data) {
    (void)self;mp_buffer_info_t b;mp_get_buffer_raise(data,&b,MP_BUFFER_READ);
    transmit(b.buf,b.len);return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(usb_write_obj,usb_write);
static mp_obj_t usb_flush(mp_obj_t self) {(void)self;opened();control()[5]++;return mp_const_none;}
static MP_DEFINE_CONST_FUN_OBJ_1(usb_flush_obj,usb_flush);
static mp_obj_t usb_read(size_t n,const mp_obj_t *args) {
    opened();int size=n>1?mp_obj_get_int(args[1]):-1;
    int timeout=(n>2 && args[2]!=mp_const_none)?mp_obj_get_int(args[2]):-1;
    if(size<0) {size=256;}
    if(!size) {return mp_const_empty_bytes;}
    volatile uint32_t *h=control();uint64_t start=pe_clock_us();
    while(h[1]==h[2]) {
        opened();
        if(timeout>=0 && pe_clock_us()-start>=(uint64_t)timeout*1000) {return mp_const_empty_bytes;}
        pe_delay_us(1000);if(pe_virtual()) {Sleep(1);}
    }
    uint32_t r=h[2],available=h[1]-r;
    if(available>CAP) {mp_raise_OSError(MP_EIO);}
    if((unsigned)size>available) {size=available;}
    vstr_t v;vstr_init_len(&v,size);
    char *ring=(char *)pe_mapping()+928000;
    size_t first=(unsigned)size<CAP-r%CAP?(unsigned)size:CAP-r%CAP;
    memcpy(v.buf,ring+r%CAP,first);memcpy(v.buf+first,ring,size-first);
    MemoryBarrier();h[2]=r+size;
    return mp_obj_new_bytes_from_vstr(&v);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usb_read_obj,1,3,usb_read);
static mp_obj_t usb_header(size_t n,const mp_obj_t *args) {
    (void)n;size_t an,tn;
    const char *application=mp_obj_str_get_data(args[1],&an),*type=mp_obj_str_get_data(args[2],&tn);
    int size=mp_obj_get_int(args[3]);
    if(an>16 || tn>16 || size<0) {mp_raise_ValueError(MP_ERROR_TEXT("Invalid fxlink header parameters"));}
    struct {uint32_t version,size,transfer;char app[16],type[16];} header={0x100,size,2048,{0},{0}};
    memcpy(header.app,application,an);memcpy(header.type,type,tn);transmit(&header,sizeof(header));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usb_header_obj,4,4,usb_header);
static mp_obj_t usb_enter(mp_obj_t self) {usb_open(self);return self;}
static MP_DEFINE_CONST_FUN_OBJ_1(usb_enter_obj,usb_enter);
static mp_obj_t usb_exit(size_t n,const mp_obj_t *args) {(void)n;return usb_close(args[0]);}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(usb_exit_obj,4,4,usb_exit);
static mp_obj_t usb_new(const mp_obj_type_t *type,size_t n,size_t kw,const mp_obj_t *args) {
    (void)args;mp_arg_check_num(n,kw,0,0,false);return MP_OBJ_FROM_PTR(mp_obj_malloc(pe_usb,type));
}
static const mp_rom_map_elem_t usb_locals_table[]={
    {MP_ROM_QSTR(MP_QSTR_open),MP_ROM_PTR(&usb_open_obj)},
    {MP_ROM_QSTR(MP_QSTR_close),MP_ROM_PTR(&usb_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_read),MP_ROM_PTR(&usb_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write),MP_ROM_PTR(&usb_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_flush),MP_ROM_PTR(&usb_flush_obj)},
    {MP_ROM_QSTR(MP_QSTR_fxlink_header),MP_ROM_PTR(&usb_header_obj)},
    {MP_ROM_QSTR(MP_QSTR___enter__),MP_ROM_PTR(&usb_enter_obj)},
    {MP_ROM_QSTR(MP_QSTR___exit__),MP_ROM_PTR(&usb_exit_obj)},
};
static MP_DEFINE_CONST_DICT(usb_locals,usb_locals_table);
MP_DEFINE_CONST_OBJ_TYPE(pe_usb_type,MP_QSTR_USB,MP_TYPE_FLAG_NONE,make_new,usb_new,locals_dict,&usb_locals);
