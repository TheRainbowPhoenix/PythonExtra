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
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(gintusb_write_obj, gintusb_write);

static mp_obj_t gintusb_read(size_t n_args, const mp_obj_t *args) {
    mp_int_t size = -1;
    mp_int_t timeout_val = -1;

    if (n_args > 1) {
        size = mp_obj_get_int(args[1]);
    }
    if (n_args > 2) {
        if (args[2] != mp_const_none) {
            timeout_val = mp_obj_get_int(args[2]);
        }
    }

    int pipe = usb_ff_bulk_input();

    if (size < 0) {
        size = 256; // Default small buffer
    }

    vstr_t vstr;
    vstr_init_len(&vstr, size);

    int rc;
    if (timeout_val >= 0) {
        timeout_t tm = timeout_make_ms(timeout_val);
        rc = usb_read_sync_timeout(pipe, vstr.buf, size, false, &tm);
    } else {
        rc = usb_read_sync(pipe, vstr.buf, size, false);
    }

    if (rc < 0) {
        vstr_clear(&vstr);
        if (rc == USB_TIMEOUT) {
            // Return empty bytes on timeout
            return mp_const_empty_bytes;
        }
        mp_raise_OSError(rc);
    }

    vstr.len = rc;
    return mp_obj_new_bytes_from_vstr(&vstr);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gintusb_read_obj, 1, 3, gintusb_read);

// fxlink helper
static mp_obj_t gintusb_fxlink_header(size_t n_args, const mp_obj_t *args) {
    // args: (self, application, type, size)
    const char *application = mp_obj_str_get_str(args[1]);
    const char *type = mp_obj_str_get_str(args[2]);
    mp_int_t size = mp_obj_get_int(args[3]);

    usb_fxlink_header_t header;
    bool ok = usb_fxlink_fill_header(&header, application, type, size);
    if (!ok) {
        mp_raise_ValueError("Invalid fxlink header parameters");
    }

    // Write the header directly using sync write
    int pipe = usb_ff_bulk_output();
    int rc = usb_write_sync(pipe, &header, sizeof(header), false);
    if (rc < 0) {
        mp_raise_OSError(rc);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(gintusb_fxlink_header_obj, 4, 4, gintusb_fxlink_header);

static mp_obj_t gintusb_flush(mp_obj_t self_in) {
    usb_commit_sync(usb_ff_bulk_output());
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(gintusb_flush_obj, gintusb_flush);

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
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&gintusb_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&gintusb_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_fxlink_header), MP_ROM_PTR(&gintusb_fxlink_header_obj) },
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
