//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.widget_shell: JustUI widget for the shell
//
// This widget wraps the terminal-like `console_t` in a JustUI widget and
// implements the keyboard interface part of edition commands (the console
// manipulates the text being edited and this is just the command).
//
// The shell has a special refreshing mechanism. It is not redraw as soon as
// new data is printed, as that would be a performance bottleneck (especially
// on fx-CG models) as surprisingly many MicroPython builtins output strings a
// single character at a time. Instead it refreshes at a fixed framerate if
// data has been printed since the last frame.
//
// This asynchronous refreshing can cause issues if the program wishes to use
// graphics mode, as it could override the program's work on the VRAM. To avoid
// this issue, whenever the built-in function gint.dupdate() is used, pending
// shell redraws are dropped and all screen updates are left to the program
// until the shell is used again through print() or input().
//
// TODO: Move the framerate-based update logic directly into JustUI.
//---

#ifndef __PYTHONEXTRA_WIDGET_SHELL_H
#define __PYTHONEXTRA_WIDGET_SHELL_H

#include <justui/defs.h>
#include <justui/jwidget.h>
#include <gint/display.h>
#include "console.h"

/* Modifier states */
enum {
    MOD_IDLE,                   /* Not active */
    MOD_INSTANT,                /* Instant-loaded but not yet used */
    MOD_INSTANT_USED,           /* Instant-loaded and has been used */

    MOD_LOCKED,                 /* Locked */
    MOD_LOCKED_INSTANT,         /* Locked and instant-loaded but not used */
    MOD_LOCKED_INSTANT_USED,    /* Locked and instant-loaded and used */
};

/* widget_shell: Multi-line Python shell input */
typedef struct {
    jwidget widget;

    /* Corresponding console */
    console_t *console;

    /* Rendering font */
    font_t const *font;
    /* Text color */
    uint16_t color;
    /* Extra line spacing */
    int8_t line_spacing;
    /* Current scroll position */
    console_scrollpos_t scroll;

    /* Internal information */
    int timer_id;
    uint16_t lines;
    int shift, alpha;

} widget_shell;

/* Event IDs */
extern uint16_t WIDGET_SHELL_MOD_CHANGED;
extern uint16_t WIDGET_SHELL_INPUT;

/* Update frequency, ie. cap on the number of shell redraws per second. */
#define WIDGET_SHELL_FPS 10

/* widget_shell_create(): Create a shell widget tied to a console */
widget_shell *widget_shell_create(console_t *console, void *parent);

/* Trivial properties */
void widget_shell_set_text_color(widget_shell *shell, int color);
void widget_shell_set_font(widget_shell *shell, font_t const *font);
void widget_shell_set_line_spacing(widget_shell *shell, int line_spacing);

/* Get current modifier states */
void widget_shell_get_modifiers(widget_shell *shell, int *shift, int *alpha);

/* Turn a pair of shift/alpha modifiers into a more presentable form:
   - The *current layer* which shows currently active modifiers:
       0: Normal
       1: Shifted (SHIFT)
       2: Lowercase (ALPHA)
       3: Uppercase (SHIFT+ALPHA)
   - The *instant* boolean which is set when one of the modifiers is in
     instant mode. This is the case when either SHIFT or ALPHA is pressed. */
void widget_shell_modifier_info(int shift, int alpha,
    int *layer, bool *instant);

#endif /* __PYTHONEXTRA_WIDGET_SHELL_H */
