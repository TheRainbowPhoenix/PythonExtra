//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "widget_shell.h"
#include <justui/jwidget-api.h>
#include <gint/timer.h>
#include <stdlib.h>

/* Type identified for widget_shell */
static int widget_shell_id = -1;

//=== Modifier states ===//

enum {
    MOD_IDLE,                   /* Not active */
    MOD_INSTANT,                /* Instant-loaded but not yet used */
    MOD_INSTANT_USED,           /* Instant-loaded and has been used */

    MOD_LOCKED,                 /* Locked */
    MOD_LOCKED_INSTANT,         /* Locked and instant-loaded but not used */
    MOD_LOCKED_INSTANT_USED,    /* Locked and instant-loaded and used */
};

/* Handle a key press/release for a modifier */
static int mod_down(int state)
{
    if(state == MOD_IDLE)
        return MOD_INSTANT;
    if(state == MOD_LOCKED)
        return MOD_LOCKED_INSTANT;
    return state;
}
static int mod_up(int state)
{
    if(state == MOD_INSTANT)
        return MOD_LOCKED;
    if(state == MOD_INSTANT_USED)
        return MOD_IDLE;
    if(state == MOD_LOCKED_INSTANT)
        return MOD_IDLE;
    if(state == MOD_LOCKED_INSTANT_USED)
        return MOD_LOCKED;
    return state;
}
/* Handle a press for another key */
static int mod_down_other(int state)
{
    if(state == MOD_INSTANT)
        return MOD_INSTANT_USED;
    if(state == MOD_LOCKED_INSTANT)
        return MOD_LOCKED_INSTANT_USED;
    return state;
}
/* Whether a modifier is active */
static bool mod_active(int state)
{
    return state == MOD_LOCKED || state == MOD_INSTANT
        || state == MOD_INSTANT_USED;
}

//=== Shell widget ===//

static int widget_shell_timer_handler(void *s0)
{
    widget_shell *s = s0;

    if(s->console && s->console->render_needed)
        s->widget.update = true;

    return TIMER_CONTINUE;
}

widget_shell *widget_shell_create(console_t *console, void *parent)
{
    if(widget_shell_id < 0)
        return NULL;

    widget_shell *s = malloc(sizeof *s);
    if(!s)
        return NULL;

    s->timer_id = timer_configure(TIMER_ANY, 1000000 / WIDGET_SHELL_FPS,
        GINT_CALL(widget_shell_timer_handler, (void *)s));
    if(s->timer_id < 0) {
        free(s);
        return NULL;
    }

    jwidget_init(&s->widget, widget_shell_id, parent);

    s->console = console;
    s->font = dfont_default();
    s->color = C_BLACK;
    s->line_spacing = 0;
    s->lines = 0;

    s->shift = MOD_IDLE;
    s->alpha = MOD_IDLE;

    timer_start(s->timer_id);

    return s;
}

void widget_shell_set_text_color(widget_shell *s, int color)
{
    s->color = color;
    s->widget.update = 1;
}

void widget_shell_set_font(widget_shell *s, font_t const *font)
{
    s->font = font ? font : dfont_default();
    s->widget.dirty = 1;
}

void widget_shell_set_line_spacing(widget_shell *s, int line_spacing)
{
    s->line_spacing = line_spacing;
    s->widget.dirty = 1;
}

//---
// Polymorphic widget operations
//---

static void widget_shell_poly_csize(void *s0)
{
    widget_shell *s = s0;
    int row_height = s->font->line_height;
    int base_rows = 4;

    s->widget.w = DWIDTH / 2;
    s->widget.h = row_height * base_rows + s->line_spacing * (base_rows - 1);
}

static void widget_shell_poly_layout(void *s0)
{
    widget_shell *s = s0;

    int ch = jwidget_content_height(s);
    int line_height = s->font->line_height + s->line_spacing;
    s->lines = ch / line_height;
}

static void widget_shell_poly_render(void *s0, int x, int y)
{
    widget_shell *s = s0;
    int line_height = s->font->line_height + s->line_spacing;

    font_t const *old_font = dfont(s->font);
    console_render(x, y, s->console, jwidget_content_width(s), line_height,
        s->lines);
    console_clear_render_flag(s->console);
    dfont(old_font);
}

static bool widget_shell_poly_event(void *s0, jevent e)
{
    widget_shell *s = s0;

    if(e.type != JWIDGET_KEY)
        return false;
    key_event_t ev = e.key;

    if(ev.key == KEY_SHIFT) {
        s->shift = ev.type == KEYEV_UP ? mod_up(s->shift) : mod_down(s->shift);
        return true;
    }
    if(ev.key == KEY_ALPHA) {
        s->alpha = ev.type == KEYEV_UP ? mod_up(s->alpha) : mod_down(s->alpha);
        return true;
    }

    if(ev.type == KEYEV_UP)
        return false;

    ev.mod = true;
    ev.shift = mod_active(s->shift);
    ev.alpha = mod_active(s->alpha);

    s->shift = mod_down_other(s->shift);
    s->alpha = mod_down_other(s->alpha);

    /* TODO: Handle input events better in the shell widget! */
    int c = console_key_event_to_char(ev);
    /* TODO: Can widget_shell_poly_event please not call into MicroPython? */
    if(c != 0) {
        pyexec_event_repl_process_char(c);
        return true;
    }

    return false;
}

static void widget_shell_poly_destroy(void *s0)
{
    widget_shell *s = s0;
    timer_stop(s->timer_id);
}

/* widget_shell type definition */
static jwidget_poly type_widget_shell = {
    .name    = "widget_shell",
    .csize   = widget_shell_poly_csize,
    .layout  = widget_shell_poly_layout,
    .render  = widget_shell_poly_render,
    .event   = widget_shell_poly_event,
    .destroy = widget_shell_poly_destroy,
};

__attribute__((constructor))
static void j_register_widget_shell(void)
{
    widget_shell_id = j_register_widget(&type_widget_shell, "jwidget");
}
