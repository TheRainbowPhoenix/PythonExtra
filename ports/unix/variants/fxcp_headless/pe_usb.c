/* Loopback USB transport for Linux fxcp_headless.
 * Serves the PythonExtra gint.USB interface as a TCP byte stream on loopback.
 * Intentionally does not emulate an SPU, libusb device, or USB enumeration.
 * Set PE_USB_PORT to choose a port (default 8765). */
#include "py/runtime.h"
#include "py/mperrno.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    mp_obj_base_t base;
    int listen_fd;
    int fd;
} pe_linux_usb_t;

static void pe_usb_close_fds(pe_linux_usb_t *self) {
    if (self->fd >= 0) { close(self->fd); self->fd = -1; }
    if (self->listen_fd >= 0) { close(self->listen_fd); self->listen_fd = -1; }
}
static int pe_usb_require(pe_linux_usb_t *self) {
    if (self->fd < 0) { mp_raise_OSError(MP_ENODEV); }
    return self->fd;
}
static mp_obj_t pe_usb_open(mp_obj_t self_in) {
    pe_linux_usb_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->fd >= 0) { return mp_const_none; }
    const char *port_env = getenv("PE_USB_PORT");
    long port = 8765;
    if (port_env && port_env[0]) {
        char *end;
        port = strtol(port_env, &end, 10);
        if (*end != '\0' || port < 1 || port > 65535) {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid PE_USB_PORT"));
        }
    }
    int server = socket(AF_INET, SOCK_STREAM, 0);
    if (server < 0) { mp_raise_OSError(errno); }
    self->listen_fd = server;
    int reuse = 1;
    setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons((uint16_t)port);
    if (bind(server, (struct sockaddr *)&addr, sizeof(addr)) != 0 ||
        listen(server, 1) != 0) {
        int error = errno;
        pe_usb_close_fds(self);
        mp_raise_OSError(error);
    }
    int fd;
    do { fd = accept(server, NULL, NULL); } while (fd < 0 && errno == EINTR);
    if (fd < 0) {
        int error = errno;
        pe_usb_close_fds(self);
        mp_raise_OSError(error);
    }
    self->fd = fd;
    close(self->listen_fd);
    self->listen_fd = -1;
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pe_usb_open_obj, pe_usb_open);
static mp_obj_t pe_usb_close(mp_obj_t self_in) {
    pe_usb_close_fds(MP_OBJ_TO_PTR(self_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pe_usb_close_obj, pe_usb_close);
static void pe_usb_send(pe_linux_usb_t *self, const void *data, size_t size) {
    const uint8_t *src = data;
    while (size) {
        ssize_t n = send(pe_usb_require(self), src, size, MSG_NOSIGNAL);
        if (n < 0 && errno == EINTR) { continue; }
        if (n <= 0) { mp_raise_OSError(n < 0 ? errno : MP_EPIPE); }
        src += n;
        size -= (size_t)n;
    }
}
static mp_obj_t pe_usb_write(mp_obj_t self_in, mp_obj_t data_in) {
    mp_buffer_info_t buffer;
    mp_get_buffer_raise(data_in, &buffer, MP_BUFFER_READ);
    pe_usb_send(MP_OBJ_TO_PTR(self_in), buffer.buf, buffer.len);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(pe_usb_write_obj, pe_usb_write);
static mp_obj_t pe_usb_flush(mp_obj_t self_in) {
    (void)pe_usb_require(MP_OBJ_TO_PTR(self_in));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pe_usb_flush_obj, pe_usb_flush);
static mp_obj_t pe_usb_read(size_t n_args, const mp_obj_t *args) {
    pe_linux_usb_t *self = MP_OBJ_TO_PTR(args[0]);
    int fd = pe_usb_require(self);
    mp_int_t requested = n_args > 1 ? mp_obj_get_int(args[1]) : 256;
    mp_int_t timeout_ms = (n_args > 2 && args[2] != mp_const_none)
        ? mp_obj_get_int(args[2]) : -1;
    if (requested < 0) { requested = 256; }
    if (requested == 0) { return mp_const_empty_bytes; }
    if (requested > 16384) { requested = 16384; }
    if (timeout_ms >= 0) {
        fd_set fds;
        FD_ZERO(&fds); FD_SET(fd, &fds);
        struct timeval tv = { timeout_ms / 1000, (timeout_ms % 1000) * 1000 };
        int ready;
        do { ready = select(fd + 1, &fds, NULL, NULL, &tv); }
        while (ready < 0 && errno == EINTR);
        if (ready < 0) { mp_raise_OSError(errno); }
        if (!ready) { return mp_const_empty_bytes; }
    }
    vstr_t v;
    vstr_init_len(&v, (size_t)requested);
    ssize_t received;
    do { received = recv(fd, v.buf, (size_t)requested, 0); }
    while (received < 0 && errno == EINTR);
    if (received < 0) { vstr_clear(&v); mp_raise_OSError(errno); }
    v.len = (size_t)received;
    return mp_obj_new_bytes_from_vstr(&v);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pe_usb_read_obj, 1, 3, pe_usb_read);
static mp_obj_t pe_usb_header(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    size_t app_len, type_len;
    const char *app = mp_obj_str_get_data(args[1], &app_len);
    const char *type = mp_obj_str_get_data(args[2], &type_len);
    mp_int_t size = mp_obj_get_int(args[3]);
    if (app_len > 16 || type_len > 16 || size < 0 || (uint64_t)size > UINT32_MAX) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid fxlink header"));
    }
    struct { uint32_t version, size, transfer; char app[16], type[16]; }
      header = { 0x100, (uint32_t)size, 2048, {0}, {0} };
    memcpy(header.app, app, app_len);
    memcpy(header.type, type, type_len);
    pe_usb_send(MP_OBJ_TO_PTR(args[0]), &header, sizeof(header));
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pe_usb_header_obj, 4, 4, pe_usb_header);
static mp_obj_t pe_usb_enter(mp_obj_t self) {
    pe_usb_open(self);
    return self;
}
static MP_DEFINE_CONST_FUN_OBJ_1(pe_usb_enter_obj, pe_usb_enter);
static mp_obj_t pe_usb_exit(size_t n_args, const mp_obj_t *args) {
    (void)n_args;
    return pe_usb_close(args[0]);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pe_usb_exit_obj, 4, 4, pe_usb_exit);
static mp_obj_t pe_usb_new(const mp_obj_type_t *type, size_t n_args,
                           size_t n_kw, const mp_obj_t *args) {
    (void)args;
    mp_arg_check_num(n_args, n_kw, 0, 0, false);
    pe_linux_usb_t *self = mp_obj_malloc(pe_linux_usb_t, type);
    self->fd = self->listen_fd = -1;
    return MP_OBJ_FROM_PTR(self);
}
static const mp_rom_map_elem_t pe_usb_locals_table[] = {
    {MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&pe_usb_open_obj)},
    {MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&pe_usb_close_obj)},
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&pe_usb_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&pe_usb_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&pe_usb_flush_obj)},
    {MP_ROM_QSTR(MP_QSTR_fxlink_header), MP_ROM_PTR(&pe_usb_header_obj)},
    {MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&pe_usb_enter_obj)},
    {MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&pe_usb_exit_obj)},
};
static MP_DEFINE_CONST_DICT(pe_usb_locals, pe_usb_locals_table);
MP_DEFINE_CONST_OBJ_TYPE(pe_linux_usb_type, MP_QSTR_USB, MP_TYPE_FLAG_NONE,
    make_new, pe_usb_new, locals_dict, &pe_usb_locals);
