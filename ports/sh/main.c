#include "py/compile.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "py/builtin.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"

#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/fs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "console.h"

// Allocate memory for the MicroPython GC heap.
static char heap[32768];

int parse_compile_execute(const void *source, mp_parse_input_kind_t input_kind, mp_uint_t exec_flags);

//=== Console-based standard streams ===//

ssize_t stdouterr_write(void *data, void const *buf, size_t size)
{
    console_t *cons = data;
    console_write_at_cursor(cons, buf, size);
    return size;
}

fs_descriptor_type_t stdouterr_type = {
    .read = NULL,
    .write = stdouterr_write,
    .lseek = NULL,
    .close = NULL,
};

//=== Main function ===//

static console_t *cons = NULL;

void pe_draw(void)
{
    dclear(C_WHITE);
    dprint(3, 3, C_BLACK, "PythonExtra, very much WIP :)");
    dline(2, 16, DWIDTH-3, 16, C_BLACK);
    int rows = 12;
    console_render(3, 20, cons, DWIDTH-6, rows);
    int y = 20 + PE_CONSOLE_LINE_SPACING * rows;
    dline(2, y, DWIDTH-3, y, C_BLACK);
    dupdate();
}

void pe_exithandler(void)
{
    pe_draw();
    drect(DWIDTH-8, 0, DWIDTH-1, 7, C_RED);
    dupdate();
    getkey();
}

int main(int argc, char **argv)
{
    /* Set up standard streams */
    cons = console_create(8192);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open_generic(&stdouterr_type, cons, STDOUT_FILENO);
    open_generic(&stdouterr_type, cons, STDERR_FILENO);

    atexit(pe_exithandler);

    /* Initialize MicroPython */
    mp_stack_ctrl_init();
    gc_init(heap, heap + sizeof(heap));
    // TODO: gc_add(start, end) for each area we want to allocate to
    // fx-9860G III:
    // * (nothing? x_x)
    // fx-CG 50:
    // * The entirety of _uram
    // * The entirety of the extra VRAM
    // * Possibly memory past 2M
    // * (keep the OS heap for normal malloc())
    mp_init();

    // Start a normal REPL; will exit when ctrl-D is entered on a blank line.
    pyexec_friendly_repl();

    console_destroy(cons);

    // Deinitialise the runtime.
    gc_sweep_all();
    mp_deinit();
    return 0;
}

// Handle uncaught exceptions (should never be reached in a correct C implementation).
void nlr_jump_fail(void *val) {
    dclear(C_BLACK);
    dtext(2, 2, C_WHITE, "nlr_jump_fail!");
    dprint(2, 2, C_WHITE, "val = %p", val);
    dupdate();
    while(1)
        getkey();
}

// Do a garbage collection cycle.
void gc_collect(void) {
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

// There is no filesystem so stat'ing returns nothing.
mp_import_stat_t mp_import_stat(const char *path)
{
    struct stat st;
    int rc = stat(path, &st);
    if(rc < 0)
        return MP_IMPORT_STAT_NO_EXIST;

    if(S_ISDIR(st.st_mode))
        return MP_IMPORT_STAT_DIR;
    else
        return MP_IMPORT_STAT_FILE;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs)
{
    mp_obj_t *args_items;
    size_t len;
    mp_obj_get_array(*args, &len, &args_items);
    printf("%d %p\n", (int)len, args_items);
    if(len != 2)
        return mp_const_none;

    char const *path = mp_obj_str_get_str(args_items[0]);
    char const *mode = mp_obj_str_get_str(args_items[1]);
    printf("'%s' '%s'\n", path, mode);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);
