//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "widget_shell.h"
#include "keymap.h"
#include <justui/jwidget-api.h>
#include <gint/timer.h>
#include <stdlib.h>

/* Type identified for widget_shell */
static int widget_shell_id = -1;

/* Events */
uint16_t WIDGET_SHELL_MOD_CHANGED;
uint16_t WIDGET_SHELL_INPUT;

//=== Modifier states ===//

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
/* Whether a modifier is in an instant mode */
static bool mod_instant(int state)
{
    return state != MOD_IDLE && state != MOD_LOCKED;
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
    s->scroll = 0;
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

void widget_shell_get_modifiers(widget_shell *s, int *shift, int *alpha)
{
    if(shift)
        *shift = s->shift;
    if(alpha)
        *alpha = s->alpha;
}

void widget_shell_modifier_info(int shift, int alpha,
    int *layer, bool *instant)
{
    if(layer)
        *layer = mod_active(shift) + 2 * mod_active(alpha);
    if(instant)
        *instant = mod_instant(shift) || mod_instant(alpha);
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

    console_compute_view(s->console, s->font, jwidget_content_width(s),
        s->lines);
    s->scroll = console_clamp_scrollpos(s->console, s->scroll);
    console_render(x, y, s->console, line_height, s->scroll);
    console_clear_render_flag(s->console);
}

static void widget_shell_update_mod(widget_shell *s, key_event_t ev)
{
    int *mod = (ev.key == KEY_SHIFT) ? &s->shift : &s->alpha;
    int new_mod = (ev.type == KEYEV_UP) ? mod_up(*mod) : mod_down(*mod);
    if(new_mod != *mod)
        jwidget_emit(s, (jevent){ .type = WIDGET_SHELL_MOD_CHANGED });
    *mod = new_mod;
}
static void widget_shell_use_mods(widget_shell *s)
{
    int new_shift = mod_down_other(s->shift);
    int new_alpha = mod_down_other(s->alpha);

    if(new_shift != s->shift || new_alpha != s->alpha)
        jwidget_emit(s, (jevent){ .type = WIDGET_SHELL_MOD_CHANGED });

    s->shift = new_shift;
    s->alpha = new_alpha;
}

static bool widget_shell_poly_event(void *s0, jevent e)
{
    widget_shell *s = s0;

    if(e.type != JWIDGET_KEY)
        return false;
    key_event_t ev = e.key;

    if(ev.key == KEY_SHIFT || ev.key == KEY_ALPHA) {
        widget_shell_update_mod(s, ev);
        return true;
    }
    if(ev.type != KEYEV_UP && ev.key == KEY_UP) {
        s->scroll++;
        s->widget.update = true;
        return true;
    }
    if(ev.type != KEYEV_UP && ev.key == KEY_DOWN) {
        s->scroll--;
        s->widget.update = true;
        return true;
    }

    if(ev.type == KEYEV_UP)
        return false;

    ev.mod = true;
    ev.shift = mod_active(s->shift);
    ev.alpha = mod_active(s->alpha);
    widget_shell_use_mods(s);

    if(ev.key == KEY_LEFT) {
        if(console_move_cursor(s->console, -1))
            s->widget.update = true;
        return true;
    }
    if(ev.key == KEY_RIGHT) {
        if(console_move_cursor(s->console, +1))
            s->widget.update = true;
        return true;
    }

    if(ev.key == KEY_ACON) {
        console_clear_current_line(s->console);
        return true;
    }

    if(ev.key == KEY_DEL) {
        console_delete_at_cursor(s->console, 1);
        return true;
    }

    if(ev.key == KEY_EXE) {
        char *line = console_get_line(s->console, true);
        console_newline(s->console);

        jevent e = {
            .type = WIDGET_SHELL_INPUT,
            .data = (int)line,
        };
        jwidget_emit(s, e);
        return true;
    }

    char c = keymap_translate(ev.key, ev.shift, ev.alpha);
    if(c != 0) {
        console_write_raw(s->console, &c, 1);
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
    WIDGET_SHELL_MOD_CHANGED = j_register_event();
    WIDGET_SHELL_INPUT = j_register_event();
}
