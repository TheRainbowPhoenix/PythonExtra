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
#include "resources.h"

//=== Application globals ===//

struct pe_globals {
    /* The terminal data structure (with lines, editing functions, etc). */
    console_t *console;
    /* The global JustUI scene. */
    jscene *scene;
    /* The widget shell (which connects the GUI to `console`). */
    widget_shell *shell;
    /* The file selection widget. */
    jfileselect *fileselect;
    /* Title widget and whether to show it in the shell. */
    jlabel *title;
    bool show_title_in_shell;
};

// TODO: Put pe_globals in a header for use by the loop hook in mpconfigport.h
widget_shell *pe_shell;
/* Whether a delayed dupdate has been scheduled asynchronously. */
bool pe_dupdate_scheduled;

struct pe_globals PE = { 0 };


//=== Hook for redirecting stdout/stderr to the shell ===//

static ssize_t stdouterr_write(void *data, void const *buf, size_t size)
{
    console_t *cons = data;
    console_write(cons, buf, size);
    return size;
}

static fs_descriptor_type_t const stdouterr_type = {
    .read = NULL,
    .write = stdouterr_write,
    .lseek = NULL,
    .close = NULL,
};

//=== File filter for the selection dialog ===//

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

//=== Module loading utilities ===//

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

//=== AC/ON interrupt mechanism ===//

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

#if PE_DEBUG
    if(ev.key == KEY_SQUARE) {
        if(ev.type == KEYEV_DOWN)
            pe_debug_toggle_videocapture();
        return false;
    }
#endif

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
    pe_resources_autofree();
}

//=== Rendering ===//

#ifdef FX9860G
extern bopti_image_t const img_fkeys_main;
extern bopti_image_t const img_modifier_states;
extern font_t const font_4x4, font_4x6, font_5x7;
#define _(fx, cg) (fx)
#else
extern bopti_image_t const img_modifier_states;
#define _(fx, cg) (cg)
#endif

void pe_enter_graphics_mode(void)
{
    /* Cancel any pending update of the shell */
    PE.console->render_needed = false;
    PE.shell->widget.update = 0;
}

void pe_schedule_dupdate(void)
{
    pe_enter_graphics_mode();
    pe_dupdate_scheduled = true;
}

void pe_dupdate(void)
{
    dupdate();
    pe_debug_run_videocapture();
    pe_dupdate_scheduled = false;
}

void pe_draw(void)
{
    dclear(C_WHITE);
    jscene_render(PE.scene);

    /* Render shell modifiers above the scene in a convenient spot */
    int shift, alpha, layer;
    bool instant;
    widget_shell_get_modifiers(PE.shell, &shift, &alpha);
    widget_shell_modifier_info(shift, alpha, &layer, &instant);
    int icon = 2 * layer + !instant;
#ifdef FX9860G
    dsubimage(118, 58, &img_modifier_states, 9*icon+1, 1, 8, 6,
        DIMAGE_NONE);
#else
    dsubimage(377, 207, &img_modifier_states, 16*icon, 0, 15, 14,
        DIMAGE_NONE);
#endif
    pe_dupdate();
}

//=== Application control functions ===//

static void pe_reset_micropython(void)
{
    gc_sweep_all();
    mp_deinit();
    mp_init();

#ifdef FX9860G
    char const *msg = "**SHELL INIT.**\n";
#else
    char const *msg = "*** SHELL INITIALIZED ***\n";
#endif

    console_newline(PE.console);
    console_write(PE.console, msg, -1);
    pyexec_event_repl_init();
}

static void pe_print_prompt(int which)
{
    char const *prompt = NULL;
    if(which == 2)
        prompt = mp_repl_get_ps2();
    else
        prompt = mp_repl_get_ps1();

    console_write(PE.console, prompt, -1);
    console_lock_prefix(PE.console);
}

static void pe_update_title(void)
{
    char const *folder = jfileselect_current_folder(PE.fileselect);

#ifdef FX9860G
    /* Use a static variable to ensure the title shows even if OoM */
    static char title[22];

    if(!folder)
        jlabel_set_text(PE.title, "Python");
    else {
        sprintf(title, "Python[%-13.13s]", folder);
        jlabel_set_text(PE.title, title);
    }
#endif

#ifdef FXCG50
    if(!folder)
        jlabel_set_text(PE.title, "PythonExtra");
    else
        jlabel_asprintf(PE.title, "PythonExtra (%s)", folder);
#endif
}

/* Handle a GUI event. If `shell_bound` is true, only actions that have an
   effect on the shell are allowed and the return value is any full line that
   is entered in the shell. Otherwise, the full GUI is available and the return
   value is NULL. */
static char *pe_handle_event(jevent e, bool shell_bound, bool *exit)
{
    if(e.type == JSCENE_PAINT)
        pe_draw();

    if(e.type == WIDGET_SHELL_MOD_CHANGED)
        PE.scene->widget.update = true;

    if(e.type == WIDGET_SHELL_INPUT) {
        char *line = (char *)e.data;
        if(shell_bound) {
            return line;
        }
        else {
            pyexec_repl_execute(line);
            free(line);
            pe_print_prompt(1);
        }
    }

    if(!shell_bound && e.type == JFILESELECT_VALIDATED) {
        char const *path = jfileselect_selected_file(PE.fileselect);
        char *module = path_to_module(path);
        if(module) {
            jscene_show_and_focus(PE.scene, PE.shell);
            jwidget_set_visible(PE.title, PE.show_title_in_shell);

            pe_reset_micropython();
            pe_draw();

            char *str = malloc(8 + strlen(module) + 1);
            if(str) {
                strcpy(str, "import ");
                strcat(str, module);
                pyexec_repl_execute(str);
                free(str);
            }
            free(module);

            pe_print_prompt(1);
        }
    }
    if(!shell_bound && e.type == JFILESELECT_LOADED)
        pe_update_title();
    /* Exit if we BACK out from the file selector */
    if(!shell_bound && e.type == JFILESELECT_CANCELED && exit)
        *exit = true;

    if(e.type != JWIDGET_KEY || e.key.type == KEYEV_UP)
        return NULL;
    int key = e.key.key;

    pe_debug_kmalloc("key");

    if(key == KEY_SQUARE && !e.key.shift && e.key.alpha)
        pe_debug_screenshot();
    if(key == KEY_TAN)
        pe_debug_kmalloc("tan");

    if(!shell_bound && (key == KEY_F1 || key == KEY_PREVTAB)) {
        jscene_show_and_focus(PE.scene, PE.fileselect);
        jwidget_set_visible(PE.title, true);
    }
    if(!shell_bound && (key == KEY_F2 || key == KEY_NEXTTAB)) {
        jscene_show_and_focus(PE.scene, PE.shell);
        jwidget_set_visible(PE.title, PE.show_title_in_shell);
    }
    if(!shell_bound && key == KEY_VARS && e.key.shift) {
        pe_debug_browse_meminfo();
        PE.scene->widget.update = true;
    }

    /* If return-to-menu is not enabled in the scene (default on machines on
       which there *is no* return-to-menu, leave on HOME */
    if(!shell_bound && exit && e.key.key == KEY_HOME)
        *exit = true;

    return NULL;
}

int pe_readline(vstr_t *line, char const *prompt)
{
    console_write(PE.console, prompt, -1);
    console_lock_prefix(PE.console);

    int c = mp_interrupt_char;
    mp_hal_set_interrupt_char(-1);

    char *text = NULL;
    while(!text) {
        jevent e = jscene_run(PE.scene);
        text = pe_handle_event(e, true, NULL);
    }

    mp_hal_set_interrupt_char(c);
    vstr_reset(line);
    vstr_add_str(line, text);
    free(text);
    return 0; // TODO: return CHAR_CTRL_C on AC/ON instead
}

int main(int argc, char **argv)
{
#ifdef FX9860G
    /* Use PRAM0 as an arena for special allocs to save memory elsewhere */
    kmalloc_arena_t arena_pram0 = { 0 };
    arena_pram0.name = "pram0";
    arena_pram0.is_default = false;
    arena_pram0.start = (void *)0xfe200000;
    arena_pram0.end = (void *)0xfe228000; /* 160 kB! */
    kmalloc_init_arena(&arena_pram0, true);
    kmalloc_add_arena(&arena_pram0);
#endif

    pe_debug_init();
    pe_debug_printf("---\n");
    pe_debug_get_startup_meminfo(MAIN);

    //=== Init sequence ===//

    keydev_set_async_filter(keydev_std(), async_filter);

    /* Make delayed shift/alpha permanent so the stay between calls to
       jscene_run() and can be used for delayed key combos */
    keydev_transform_t tr = keydev_transform(keydev_std());
    tr.enabled |= KEYDEV_TR_DELAYED_SHIFT;
    tr.enabled |= KEYDEV_TR_DELAYED_ALPHA;
    keydev_set_transform(keydev_std(), tr);

    #ifdef FX9860G
    int console_scrollback = 50;
    int console_max_bytes = 4096;
    #else
    int console_scrollback = 200;
    int console_max_bytes = 8192;
    #endif
    PE.console = console_create(console_max_bytes, console_scrollback);

    pe_debug_get_startup_meminfo(CONSOLE);

    /* Set up standard streams */
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    open_generic(&stdouterr_type, PE.console, STDOUT_FILENO);
    open_generic(&stdouterr_type, PE.console, STDERR_FILENO);

    /* Initialize the MicroPython GC with most available memory */
    mp_stack_ctrl_init();
#ifdef FX9860G
    /* Put basically all the OS heap into the Python GC. For our own purposes
       we'll use gint's _uram arena and the leftover from the OS heap. */
    int size = 65536;
    bool first = true;

    while(size >= 2048) {
        void *area = kmalloc(size, "_os");
        if(area) {
            if(first)
                gc_init(area, area + size);
            else
                gc_add(area, area + size);
            first = false;
        }
        else size /= 2;
    }

    if(first)
        pe_debug_panic("No heap!");

#if PE_DEBUG
    /* Add some Python ram */
    // https://www.planet-casio.com/Fr/forums/topic15269-10-khicas-add-in-calcul-formel-pour-graph-90e-et-35eii.html#189284
    /* void *py_ram_start = (void*)0x88053800;
    void *py_ram_end = (void*)0x8807f000;
    gc_add(py_ram_start, py_ram_end); */
#endif
#else
    /* On Math+, we have a loooot of free space in the _ld1 arena. */
    if(gint[HWCALC] == HWCALC_FXCG100) {
        size_t gc_area_size;
        void *gc_area = kmalloc_max(&gc_area_size, "_ld1");
        gc_init(gc_area, gc_area + gc_area_size);
    }
    else {
        /* Get everything from the OS stack (~ 350 kB) */
        size_t gc_area_size;
        void *gc_area = kmalloc_max(&gc_area_size, "_ostk");
        gc_init(gc_area, gc_area + gc_area_size);

        /* Also get most of _uram; we try and fit in the remaining ~150 kB */
        void *uram_area = kmalloc(300000, "_uram");
        if(uram_area)
            gc_add(uram_area, uram_area + 300000);

        /* Other options:
           - All of _uram (leaving the OS heap for the shell/GUI/etc)
           - The OS' extra VRAM
           - Memory past the 2 MB boundary on tested OSes */
        // gc_add(start, end)...
    }
#endif

    mp_init();

    /* TODO: Add an option for the shorter prompt */
#ifdef FX9860G
    MP_STATE_VM(sys_mutable[MP_SYS_MUTABLE_PS1]) =
        MP_OBJ_NEW_QSTR(qstr_from_str(">"));
    MP_STATE_VM(sys_mutable[MP_SYS_MUTABLE_PS2]) =
        MP_OBJ_NEW_QSTR(qstr_from_str("."));
#endif

    pe_debug_get_startup_meminfo(UPY);

    pyexec_event_repl_init();
    pe_print_prompt(1);

    pe_debug_get_startup_meminfo(PROMPT);

    //=== GUI setup ===//

    PE.scene = jscene_create_fullscreen(NULL);
    PE.title = jlabel_create("<temp>", PE.scene);
    jwidget *stack = jwidget_create(PE.scene);
    jfkeys *fkeys = jfkeys_create2(_(&img_fkeys_main, NULL), "/FILES;/SHELL", PE.scene);
    (void)fkeys;

    jwidget_set_stretch(PE.title, 1, 0, false);

    jlayout_set_vbox(PE.scene)->spacing = _(0, 3);
    jlayout_set_stack(stack);
    jwidget_set_stretch(stack, 1, 1, false);

    /* Filesystem tab */
    PE.fileselect = jfileselect_create(stack);
    jfileselect_set_filter(PE.fileselect, py_file_filter);
    jfileselect_set_show_file_size(PE.fileselect, true);
    jwidget_set_stretch(PE.fileselect, 1, 1, false);

    /* Shell tab */
    PE.shell = pe_shell = widget_shell_create(PE.console, stack);
    widget_shell_set_line_spacing(PE.shell, _(1, 3));
    jwidget_set_stretch(PE.shell, 1, 1, false);

#ifdef FX9860G
    PE.show_title_in_shell = false;
    jwidget_set_padding(PE.title, 0, 0, 1, 0);
    widget_shell_set_font(PE.shell, &font_4x6);
#else
    PE.show_title_in_shell = true;
    jwidget_set_background(PE.title, C_BLACK);
    jlabel_set_text_color(PE.title, C_WHITE);
    jwidget_set_padding(PE.title, 3, 6, 3, 6);
    jwidget_set_padding(stack, 0, 6, 0, 6);
#endif

    pe_debug_get_startup_meminfo(UI);

    /* Initial state */
    jfileselect_browse(PE.fileselect, "/");
    jscene_show_and_focus(PE.scene, PE.fileselect);

    //=== Event handling ===//

    while(1) {
        jevent e = jscene_run(PE.scene);
        bool exit = false;
        pe_handle_event(e, false, &exit);

        if(exit)
            break;
    }

    //=== Deinitialization ===//

    pe_debug_close();
    
    gc_sweep_all();
    mp_deinit();
    console_destroy(PE.console);
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
