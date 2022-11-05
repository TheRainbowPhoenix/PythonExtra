//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.widget_shell: JustUI widget for the shell

#ifndef __PYTHONEXTRA_WIDGET_SHELL_H
#define __PYTHONEXTRA_WIDGET_SHELL_H

#include <justui/defs.h>
#include <justui/jwidget.h>
#include <gint/display.h>
#include "console.h"

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

    /* Internal information */
    int timer_id;
    uint16_t lines;
    int shift, alpha;

} widget_shell;

/* Update frequency, ie. cap on the number of shell redraws per second. */
#define WIDGET_SHELL_FPS 30

/* widget_shell_create(): Create a shell widget tied to a console */
widget_shell *widget_shell_create(console_t *console, void *parent);

/* Trivial properties */
void widget_shell_set_text_color(widget_shell *shell, int color);
void widget_shell_set_font(widget_shell *shell, font_t const *font);
void widget_shell_set_line_spacing(widget_shell *shell, int line_spacing);

#endif /* __PYTHONEXTRA_WIDGET_SHELL_H */
