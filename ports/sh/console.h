//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.console: Terminal emulator
//
// This header implements a basic terminal emulator compatible with
// MicroPython's readline() implementation.
//
// The main features are:
// * Dynamically-sized lines with reflow
// * Cap memory usage based on the total amount of text, not just line count
// * Basic ANSI-escape-based edition features (but only on the last line)
//
// The console tries fairly hard to focus on text manipulation and separate
// rendering. To render, one must first compute a "view" of the terminal, which
// essentially determines line wrapping and scrolling bounds, and then use that
// view and a valid scroll position within it to render.
//---

#ifndef __PYTHONEXTRA_CONSOLE_H
#define __PYTHONEXTRA_CONSOLE_H

#include <gint/keyboard.h>
#include <gint/display.h>
#include <stdbool.h>

/* Maximum line length, to ensure the console can threshold its memory usage
   while cleaning only entire lines. Lines longer than this get split. */
#define PE_CONSOLE_LINE_MAX_LENGTH 1024

//=== Dynamic console lines ===//

typedef struct
{
    /* Line contents, NUL-terminated. The buffer might be larger. */
    char *data;
    /* Size of contents (not counting the NUL). */
    int size;
    /* Allocated size (always ≥ size+1). */
    int alloc_size;
    /* Number or render lines used (updated on-demand). */
    int16_t render_lines;
    /* Number of initial characters that can't be edited. */
    int16_t prefix;

} console_line_t;

/* Create a new console line with at least init_chars characters of content
   available. Returns false on error. Previous contents are not freed! */
bool console_line_init(console_line_t *line, int init_chars);

/* Clean up a line and free its contents. */
void console_line_deinit(console_line_t *line);

/* Realloc the line to ensure n characters plus a NUL can be written. */
bool console_line_alloc(console_line_t *line, int n);

/* Determine how many characters can be written before the line has to be
   broken up. */
int console_line_capacity(console_line_t *line);

/* Set the prefix_size first characters of the line to not be editable. The
   line must already have that many characters printed. */
void console_line_set_prefix(console_line_t *line, int prefix_size);

/* Insert n characters at position p. */
bool console_line_insert(console_line_t *line, int p, char const *str, int n);

/* Remove n characters at position p. Returns the number of characters
   actually removed after bounds checking. */
int console_line_delete(console_line_t *line, int p, int n);

/* Update the number of render lines for the chosen width. */
void console_line_update_render_lines(console_line_t *line, int width);

/* Render a vertical slice of the wrapped line. */
int console_line_render(int x, int y, console_line_t *line, int w, int dy,
    int show_from, int show_until, int cursor);

//=== Terminal emulator ===//

typedef struct
{
    /* A dynamic array of console_line_t. Never empty. */
    console_line_t *lines;
    int line_count;

    /* Maximum (not 100% strict) amount of storage that is conserved in log.
       When this limit is exceeded, old lines are cleaned. This must be more
       than PE_CONSOLE_LINE_MAX_LENGTH. */
    int backlog_size;

    /* Absolute number of the first line in the dynamic line array. This is
       the line number in the user-facing sense (it includes lines that were
       cleaned up to free memory). */
    int absolute_lineno;
    /* Cursor position within the last line. */
    int cursor;

    /* Whether new data has been added and a frame should be rendered. */
    bool render_needed;
    /* View parameters from last console_compute_view() */
    font_t const *render_font;
    int render_width;
    int render_lines;
    int render_total_lines;

} console_t;

/* Scroll position measured as a number of lines up from the bottom. */
typedef int console_scrollpos_t;

/* Create a new console with the specified backlog size. */
console_t *console_create(int backlog_size);

/* Create a new empty line at the bottom of the console, and move the cursor
   there. Previous lines can no longer be edited. Returns false on error. */
bool console_new_line(console_t *cons);

/* Clean up backlog if the total memory usage is exceeded. */
void console_clean_backlog(console_t *cons);

/* Clear the console's render flag, which is used to notify of changes. This
   function is used when handing control of the display from the console to a
   Python program so the console doesn't override the program's output. */
void console_clear_render_flag(console_t *cons);

/* Destroy the console and free all associated memory. */
void console_destroy(console_t *cons);

//=== Rendering interface ===//

/* Compute a view of the console for rendering and scrolling.
   @font    Font to render text with (use to compute line wrapping)
   @width   View width in pixels
   @lines   Number of text lines (spacing can be changed later) */
void console_compute_view(console_t *cons, font_t const *font,
    int width, int lines);

/* Clamp a scrolling position to the range valid of the last computed view. */
console_scrollpos_t console_clamp_scrollpos(console_t const *cons,
    console_scrollpos_t pos);

/* Render the console at (x,y). The render `width`, the number of `lines` and
   the text `font` are all as specified by the latest console_compute_view().
   `dy` indicates line height. */
void console_render(int x, int y, console_t const *cons, int dy,
    console_scrollpos_t pos);

//=== Edition functions ===//

/* Set the cursor position within the current line. Returns false if the cursor
   cannot move there due to bounds. */
bool console_set_cursor(console_t *cons, int absolute_cursor_pos);

/* Move the cursor position within the current line. Returns false if the
   cursor position didn't change due to bounds. */
bool console_move_cursor(console_t *cons, int cursor_movement);

/* Get the contents of the current line (skipping the prefix). If copy=true,
   returns a copy created with malloc(), otherwise returns a pointer within the
   original string. Mind that the original might disappear very quickly (as
   early as the next console_new_line() due to automatic backlog cleaning). */
char *console_get_line(console_t *cons, bool copy);

/* Write string at the cursor's position within the last line. This writes a
   raw string without interpreting escape sequences and newlines. */
bool console_write_block_at_cursor(console_t *cons, char const *str, int n);

/* Write string at the cursor's position within the last line. This function
   interprets escape sequences and newlines. */
bool console_write_at_cursor(console_t *cons, char const *str, int n);

/* Set the current cursor position to mark the prefix of the current line. */
void console_lock_prefix(console_t *cons);

/* Delete n characters from the cursor position. */
void console_delete_at_cursor(console_t *cons, int n);

/* Clear the current line. */
void console_clear_current_line(console_t *cons);

//=== Input method ===//

/* Interpret a key event into a terminal input. This is a pretty raw input
   method with no shift/alpha lock, kept for legacy as a VT-100-style terminal
   emulator. */
int console_key_event_to_char(key_event_t ev);

#endif /* __PYTHONEXTRA_CONSOLE_H */
