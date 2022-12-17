//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "py/compile.h"
#include "py/gc.h"
#include "py/stackctrl.h"
#include "py/builtin.h"
#include "py/mphal.h"
#include "py/repl.h"
#include "shared/runtime/gchelper.h"
#include "pyexec.h"

#include <gint/display.h>
#include <gint/drivers/keydev.h>
#include <gint/keyboard.h>
#include <gint/kmalloc.h>
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

#include "mpconfigport.h"
#include "console.h"
#include "widget_shell.h"
#include "debug.h"

#ifdef FX9860G
extern bopti_image_t const img_fkeys_main;
extern bopti_image_t const img_modifier_states;
extern font_t const font_4x4, font_4x6, font_5x7;
#define _(fx, cg) (fx)
#else
extern bopti_image_t const img_modifier_states;
#define _(fx, cg) (cg)
#endif

static ssize_t stdouterr_write(void *data, void const *buf, size_t size)
{
    console_t *cons = data;
    console_write(cons, buf, size);
    return size;
}

fs_descriptor_type_t stdouterr_type = {
    .read = NULL,
    .write = stdouterr_write,
    .lseek = NULL,
    .close = NULL,
};

/* The global terminal. */
console_t *pe_console = NULL;
/* The global JustUI scene and widget shell. */
jscene *pe_scene = NULL;
widget_shell *pe_shell = NULL;

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
    if(ent->d_type == DT_DIR)
        return true;
    return strendswith(ent->d_name, ".py");
}

static char *path_to_module(char const *path)
{
    if(path[0] == '/')
        path++;

    int i, n = strlen(path);
    char *module = malloc(n + 1);
    if(!module)
        return NULL;

    for(i = 0; i < n; i++) {
        if(i == n - 3 && !strcmp(path + i, ".py"))
            break;
        module[i] = (path[i] == '/') ? '.' : path[i];
    }
    module[i] = 0;
    return module;
}

/* Filter AC/ON push events asynchronously from the keyboard driver and
   interrupt MicroPython instead. */
static bool async_filter(key_event_t ev)
{
    /* Gobble all events related to AC/ON to make sure that the keyboard driver
       treats them as handled. Otherwise, we'd run the risk of filling the
       event queue (if the user doesn't read from it) thus preventing the
       driver from handling AC/ON releases, which disables further presses. */
    if(mp_interrupt_char >= 0 && ev.key == KEY_ACON) {
        /* This function supports asynchronous calls, by design. */
        if(ev.type == KEYEV_DOWN)
            mp_sched_keyboard_interrupt();
        return false;
    }

    return true;
}

void pe_after_python_exec(int input_kind, int exec_flags, void *ret_val,
    int *ret)
{
    (void)input_kind;
    (void)exec_flags;
    (void)ret_val;
    (void)ret;
    clearevents();
}

static void pe_print_prompt(int which)
{
    char const *prompt = NULL;
    if(which == 2)
        prompt = mp_repl_get_ps2();
    else
        prompt = mp_repl_get_ps1();

    console_write(pe_console, prompt, -1);
    console_lock_prefix(pe_console);
}

static void pe_reset_micropython(void)
{
    mp_deinit();
    mp_init();

#ifdef FX9860G
    char const *msg = "**SHELL INIT.**\n";
#else
    char const *msg = "*** SHELL INITIALIZED ***\n";
#endif

    console_newline(pe_console);
    console_write(pe_console, msg, -1);
    pyexec_event_repl_init();
    pe_print_prompt(1);
}

void pe_draw(void)
{
    dclear(C_WHITE);
    jscene_render(pe_scene);

    /* Render shell modifiers above the scene in a convenient spot */
    int shift, alpha, layer;
    bool instant;
    widget_shell_get_modifiers(pe_shell, &shift, &alpha);
    widget_shell_modifier_info(shift, alpha, &layer, &instant);
    int icon = 2 * layer + !instant;
#ifdef FX9860G
    dsubimage(118, 58, &img_modifier_states, 9*icon+1, 1, 8, 6,
        DIMAGE_NONE);
#else
    dsubimage(377, 207, &img_modifier_states, 16*icon, 0, 15, 14,
        DIMAGE_NONE);
#endif
    dupdate();
}

int main(int argc, char **argv)
{
    pe_debug_init();
    pe_debug_kmalloc();

    //=== Init sequence ===//

    keydev_set_async_filter(keydev_std(), async_filter);

    pe_console = console_create(8192, 200);

    /* Set up standard streams */
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open_generic(&stdouterr_type, pe_console, STDOUT_FILENO);
    open_generic(&stdouterr_type, pe_console, STDERR_FILENO);

    /* Initialize the MicroPython GC with most available memory */
    mp_stack_ctrl_init();
#ifdef FX9860G
    /* Get *some* memory from the OS heap */
    void *gc_area = malloc(32768);
    if(!gc_area)
        pe_debug_panic("No heap!");
    gc_init(gc_area, gc_area + 32768);
#ifdef PE_DEBUG
    /* Add some Python ram */
    void *py_ram_start = (void*)0x88053800;
    void *py_ram_end = (void*)0x8807f000;
    gc_add(py_ram_start, py_ram_end);
#endif
#else
    /* Get everything from the OS stack (~ 350 ko) */
    size_t gc_area_size;
    void *gc_area = kmalloc_max(&gc_area_size, "_ostk");
    gc_init(gc_area, gc_area + gc_area_size);

    /* Other options:
       - All of _uram (leaving the OS heap for the shell/GUI/etc)
       - The OS' extra VRAM
       - Memory past the 2 MB boundary on tested OSes */
    // gc_add(start, end)...
#endif

    mp_init();

    /* TODO: Add an option for the shorter prompt */
#ifdef FX9860G
    MP_STATE_VM(sys_mutable[MP_SYS_MUTABLE_PS1]) =
        MP_OBJ_NEW_QSTR(qstr_from_str(">"));
    MP_STATE_VM(sys_mutable[MP_SYS_MUTABLE_PS2]) =
        MP_OBJ_NEW_QSTR(qstr_from_str("."));
#endif

    pyexec_event_repl_init();
    pe_print_prompt(1);

    //=== GUI setup ===//

    pe_scene = jscene_create_fullscreen(NULL);
    jlabel *title = jlabel_create("PythonExtra", pe_scene);
    jwidget *stack = jwidget_create(pe_scene);
    jfkeys *fkeys = jfkeys_create2(&img_fkeys_main, "/FILES;/SHELL", pe_scene);
    (void)fkeys;

    jwidget_set_stretch(title, 1, 0, false);

    jlayout_set_vbox(pe_scene)->spacing = _(0, 3);
    jlayout_set_stack(stack);
    jwidget_set_stretch(stack, 1, 1, false);

    /* Filesystem tab */
    jfileselect *fileselect = jfileselect_create(stack);
    jfileselect_set_filter(fileselect, py_file_filter);
    jfileselect_set_show_file_size(fileselect, true);
    jwidget_set_stretch(fileselect, 1, 1, false);

    /* Shell tab */
    pe_shell = widget_shell_create(pe_console, stack);
    widget_shell_set_line_spacing(pe_shell, _(1, 3));
    jwidget_set_stretch(pe_shell, 1, 1, false);

#ifdef FX9860G
    bool show_title_in_shell = false;
    jwidget_set_padding(title, 0, 0, 1, 0);
    widget_shell_set_font(shell, &font_4x6);
#else
    bool show_title_in_shell = true;
    jwidget_set_background(title, C_BLACK);
    jlabel_set_text_color(title, C_WHITE);
    jwidget_set_padding(title, 3, 6, 3, 6);
    jwidget_set_padding(stack, 0, 6, 0, 6);
#endif

    /* Initial state */
    jfileselect_browse(fileselect, "/");
    jscene_show_and_focus(pe_scene, fileselect);

    //=== Event handling ===//

    while(1) {
        jevent e = jscene_run(pe_scene);

        if(e.type == JSCENE_PAINT) {
            pe_draw();
        }

        if(e.type == WIDGET_SHELL_MOD_CHANGED)
            pe_scene->widget.update = true;

        if(e.type == WIDGET_SHELL_INPUT) {
            char *line = (char *)e.data;
            pyexec_repl_execute(line);
            free(line);
            pe_print_prompt(1);
        }

        if(e.type == JFILESELECT_VALIDATED) {
            char const *path = jfileselect_selected_file(fileselect);
            char *module = path_to_module(path);
            if(module) {
                jscene_show_and_focus(pe_scene, pe_shell);
                jwidget_set_visible(title, show_title_in_shell);

                pe_reset_micropython();

                char *str = malloc(8 + strlen(module) + 1);
                if(str) {
                    strcpy(str, "import ");
                    strcat(str, module);
                    pyexec_repl_execute(str);
                    free(str);
                }
                free(module);
            }
        }

        if(e.type != JWIDGET_KEY || e.key.type == KEYEV_UP)
            continue;
        int key = e.key.key;

        if(key == KEY_SQUARE && !e.key.shift && e.key.alpha)
            pe_debug_screenshot();
        if(key == KEY_TAN)
            pe_debug_kmalloc();

        if(key == KEY_F1) {
            jscene_show_and_focus(pe_scene, fileselect);
            jwidget_set_visible(title, true);
        }
        if(key == KEY_F2) {
            jscene_show_and_focus(pe_scene, pe_shell);
            jwidget_set_visible(title, show_title_in_shell);
        }
    }

    //=== Deinitialization ===//

    gc_sweep_all();
    mp_deinit();
    console_destroy(pe_console);
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
