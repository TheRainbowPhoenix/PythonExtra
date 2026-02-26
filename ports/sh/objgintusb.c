#include "objgintusb.h"
#include "py/runtime.h"
#include <gint/usb.h>
#include <gint/usb-ff-bulk.h>

typedef struct _mp_obj_gintusb_t {
    mp_obj_base_t base;
} mp_obj_gintusb_t;

static mp_obj_t gintusb_open(mp_obj_t self_in) {
    usb_interface_t const *interfaces[] = { &usb_ff_bulk, NULL };
    int rc = usb_open(interfaces, GINT_CALL_NULL);
    if (rc < 0) {
        mp_raise_OSError(rc);
    }
    usb_open_wait();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(gintusb_open_obj, gintusb_open);

static mp_obj_t gintusb_close(mp_obj_t self_in) {
    usb_close();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(gintusb_close_obj, gintusb_close);

static mp_obj_t gintusb_write(mp_obj_t self_in, mp_obj_t data_in) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data_in, &bufinfo, MP_BUFFER_READ);

    int pipe = usb_ff_bulk_output();
    int rc = usb_write_sync(pipe, bufinfo.buf, bufinfo.len, false);
    if (rc < 0) {
        mp_raise_OSError(rc);
    }
    usb_commit_sync(pipe);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(gintusb_write_obj, gintusb_write);

static mp_obj_t gintusb_read(size_t n_args, const mp_obj_t *args) {
    mp_int_t size = -1;
    if (n_args > 1) {
        size = mp_obj_get_int(args[1]);
    }

    int pipe = usb_ff_bulk_input();

    // If size is not specified or negative, we might need a different approach
    // For now, let's assume the user must specify a size or we use a default buffer
    // But standard read() without arguments usually reads 'all', which is hard here.
    // Let's enforce size for now or default to a small buffer?
    // Actually, usb_read_sync needs a size.

    if (size < 0) {
        // Fallback or error? Let's use a default buffer size if not specified?
        // Or raise error? The Python way is read(size=-1).
        // If -1, we should probably read until end of transaction?
        // But usb_read_sync reads *up to* size.
        size = 256; // Default small buffer
    }

    vstr_t vstr;
    vstr_init_len(&vstr, size);

    int rc = usb_read_sync(pipe, vstr.buf, size, false);
    if (rc < 0) {
        vstr_clear(&vstr);
        // mp_raise_OSError(rc); // Don't raise on error immediately?
        // If it's a timeout or something, maybe return empty bytes?
        // But standard files raise.
        // Let's return empty bytes on 0 read, raise on error.
        mp_raise_OSError(rc);
    }

    vstr.len = rc;
    return mp_obj_new_bytes_from_vstr(&vstr);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gintusb_read_obj, 1, 2, gintusb_read);

static mp_obj_t gintusb_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    mp_obj_gintusb_t *self = mp_obj_malloc(mp_obj_gintusb_t, type);
    return MP_OBJ_FROM_PTR(self);
}

static mp_obj_t gintusb___enter__(mp_obj_t self_in) {
    gintusb_open(self_in);
    return self_in;
}
static MP_DEFINE_CONST_FUN_OBJ_1(gintusb___enter___obj, gintusb___enter__);

static mp_obj_t gintusb___exit__(size_t n_args, const mp_obj_t *args) {
    gintusb_close(args[0]);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gintusb___exit___obj, 4, 4, gintusb___exit__);


static const mp_rom_map_elem_t gintusb_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&gintusb_open_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&gintusb_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&gintusb_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&gintusb_read_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&gintusb___enter___obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&gintusb___exit___obj) },
};
static MP_DEFINE_CONST_DICT(gintusb_locals_dict, gintusb_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_gintusb,
    MP_QSTR_USB,
    MP_TYPE_FLAG_NONE,
    make_new, gintusb_make_new,
    locals_dict, &gintusb_locals_dict
);
