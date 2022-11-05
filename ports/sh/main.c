//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "py/compile.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/builtin.h"
#include "shared/runtime/gchelper.h"
#include "shared/runtime/pyexec.h"

#include <gint/display.h>
#include <gint/keyboard.h>
#include <gint/fs.h>

#include <justui/jscene.h>
#include <justui/jlabel.h>
#include <justui/jfkeys.h>
#include <justui/jfileselect.h>
#include <justui/jpainted.h>

#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "console.h"
#include "widget_shell.h"

//---

#include "py/mphal.h"
#include "py/repl.h"
#include "genhdr/mpversion.h"

#ifdef FX9860G
#define _(fx, cg) (fx)
#else
#define _(fx, cg) (cg)
#endif

static ssize_t stdouterr_write(void *data, void const *buf, size_t size)
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

/* The global terminal. */
console_t *pe_shell_console;

static bool strendswith(char const *str, char const *suffix)
{
    size_t l1 = strlen(str);
    size_t l2 = strlen(suffix);

    return l1 >= l2 && strcmp(str + l1 - l2, suffix) == 0;
}

static bool py_file_filter(struct dirent const *ent)
{
    if(!jfileselect_default_filter(ent))
        return false;

    if(ent->d_type == DT_REG && !strendswith(ent->d_name, ".py"))
        return false;

    return true;
}

int main(int argc, char **argv)
{
    //=== Init sequence ===//

    pe_shell_console = console_create(8192);

    /* Set up standard streams */
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open_generic(&stdouterr_type, pe_shell_console, STDOUT_FILENO);
    open_generic(&stdouterr_type, pe_shell_console, STDERR_FILENO);

    /* Initialize MicroPython */
    #define HEAP_SIZE 32768
    void *heap = malloc(32768);
    if(!heap) {
        dclear(C_WHITE);
        dtext(1, 1, C_BLACK, "No heap!");
        getkey();
        return 1;
    }

    mp_stack_ctrl_init();
    gc_init(heap, heap + HEAP_SIZE);
    // TODO: gc_add(start, end) for each area we want to allocate to
    // fx-9860G III:
    // * (nothing? x_x)
    // fx-CG 50:
    // * The entirety of _uram
    // * The entirety of the extra VRAM
    // * Possibly memory past 2M
    // * (keep the OS heap for normal malloc())
    mp_init();

    /* Run REPL manually */
    pyexec_mode_kind = PYEXEC_MODE_FRIENDLY_REPL;
    pyexec_event_repl_init();

    //=== GUI setup ===//

    jscene *scene = jscene_create_fullscreen(NULL);
    jlabel *title = jlabel_create("PythonExtra", scene);
    jwidget *stack = jwidget_create(scene);
    jfkeys *fkeys = jfkeys_create("/FILES;/SHELL;;;;", scene);
    (void)fkeys;

    jwidget_set_background(title, C_BLACK);
    jlabel_set_text_color(title, C_WHITE);
    jwidget_set_stretch(title, 1, 0, false);
    jwidget_set_padding(title, _(1, 3), _(2, 6), _(1, 3), _(2, 6));

    jlayout_set_vbox(scene)->spacing = _(1, 3);
    jlayout_set_stack(stack);
    jwidget_set_padding(stack, 0, 6, 0, 6);
    jwidget_set_stretch(stack, 1, 1, false);

    /* Filesystem tab */
    jfileselect *fileselect = jfileselect_create(stack);
    jfileselect_set_filter(fileselect, py_file_filter);
    jfileselect_set_show_file_size(fileselect, true);
    jwidget_set_stretch(fileselect, 1, 1, false);

    /* Shell tab */
    widget_shell *shell = widget_shell_create(pe_shell_console, stack);
    widget_shell_set_line_spacing(shell, _(1, 3));
    jwidget_set_stretch(shell, 1, 1, false);

    /* Initial state */
    jfileselect_browse(fileselect, "/");
    jscene_show_and_focus(scene, fileselect);

    //=== Event handling ===//

    while(1) {
        jevent e = jscene_run(scene);

        if(e.type == JSCENE_PAINT) {
            dclear(C_WHITE);
            jscene_render(scene);
            dupdate();
        }

        if(e.type != JWIDGET_KEY || e.key.type == KEYEV_UP)
            continue;
        int key = e.key.key;

        if(key == KEY_F1)
            jscene_show_and_focus(scene, fileselect);
        if(key == KEY_F2)
            jscene_show_and_focus(scene, shell);
    }

    //=== Deinitialization ===//

    // Deinitialise the runtime.
    gc_sweep_all();
    mp_deinit();
    console_destroy(pe_shell_console);
    return 0;
}

/* Handle uncaught exceptions (normally unreachable). */
void nlr_jump_fail(void *val)
{
    dclear(C_BLACK);
    dtext(2, 2, C_WHITE, "nlr_jump_fail!");
    dprint(2, 2, C_WHITE, "val = %p", val);
    dupdate();
    while(1)
        getkey();
}

/* Do a garbage collection cycle. */
void gc_collect(void)
{
    gc_collect_start();
    gc_helper_collect_regs_and_stack();
    gc_collect_end();
}

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

// TODO: See branch 'posix-open' for a relevant attempt at using the POSIX API
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
