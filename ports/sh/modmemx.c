#include "py/runtime.h"

// Fast memcpy to buffer with access size
static mp_obj_t memx_read(mp_obj_t buf_in, mp_obj_t addr_in, mp_obj_t w_in) {
  mp_buffer_info_t b;
  mp_get_buffer_raise(buf_in, &b, MP_BUFFER_WRITE);
  uintptr_t a = mp_obj_get_int_truncated(addr_in);
  int w = mp_obj_get_int(w_in);
  size_t n = b.len / w;
  if (w == 4) {
    volatile uint32_t *s = (void *)a;
    uint32_t *d = b.buf;
    while (n--)
      *d++ = *s++;
  } else if (w == 2) {
    volatile uint16_t *s = (void *)a;
    uint16_t *d = b.buf;
    while (n--)
      *d++ = *s++;
  } else {
    volatile uint8_t *s = (void *)a;
    uint8_t *d = b.buf;
    while (n--)
      *d++ = *s++;
  }
  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(memx_read_obj, memx_read);

// Fast memcpy from buffer with access size
static mp_obj_t memx_write(mp_obj_t addr_in, mp_obj_t buf_in, mp_obj_t w_in) {
  mp_buffer_info_t b;
  mp_get_buffer_raise(buf_in, &b, MP_BUFFER_READ);
  uintptr_t a = mp_obj_get_int_truncated(addr_in);
  int w = mp_obj_get_int(w_in);
  size_t n = b.len / w;
  if (w == 4) {
    volatile uint32_t *d = (void *)a;
    const uint32_t *s = b.buf;
    while (n--)
      *d++ = *s++;
  } else if (w == 2) {
    volatile uint16_t *d = (void *)a;
    const uint16_t *s = b.buf;
    while (n--)
      *d++ = *s++;
  } else {
    volatile uint8_t *d = (void *)a;
    const uint8_t *s = b.buf;
    while (n--)
      *d++ = *s++;
  }
  return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(memx_write_obj, memx_write);

static const mp_rom_map_elem_t memx_globals[] = {
    {MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_memx)},
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&memx_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&memx_write_obj)},
};
static MP_DEFINE_CONST_DICT(memx_globals_dict, memx_globals);

const mp_obj_module_t memx_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&memx_globals_dict,
};
MP_REGISTER_MODULE(MP_QSTR_memx, memx_module);
