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
#include <gint/defs/attributes.h>
#include <stdbool.h>
#include "stredit.h"

/* Maximum line length, to ensure the console can threshold its memory usage
   while cleaning only entire lines. Lines longer than this get split. */
#define PE_CONSOLE_LINE_MAX_LENGTH 1024

/* Allocation arena for arrays of lines. */
// TODO: Split circular buffers
#ifdef FX9860G
#define PE_CONSOLE_LINE_ALLOC NULL
#else
#define PE_CONSOLE_LINE_ALLOC NULL
#endif

//=== Static console lines ===//

/* A line whose contents have been frozen. The only variable left is how many
   render lines it takes, which is kept at the beginning of the buffer along
   with its size. */
typedef struct
{
    /* Number of non-NUL bytes in data[] (read-only) */
    uint16_t size;
    /* Number of render lines occupied by the line (read-write) */
    uint8_t render_lines;
    /* Raw NUL-terminated data (read-only) */
    char data[];

} console_fline_t;

/* sizeof(console_fline_t) without alignment */
#define CONSOLE_FLINE_SIZE 3

/* Update the number of render lines for the chosen width. */
void console_fline_update_render_lines(console_fline_t *FL, int width);

/* Render a vertical slice of the wrapped line. */
int console_fline_render(int x, int y, console_fline_t *FL, int w, int dy,
    int show_from, int show_until, int cursor);

//=== Rotating line storage ===//

/* linebuf_t: A rotating array of console lines. */
typedef struct
{
    /* A rotating array of `capacity` lines starting at position `start` and
       holding `size` lines. The array is pre-allocated. The last line, if
       there is one, is stored as NULL and its address is edit->raw. */
    console_fline_t **lines;
    /* Editor for the last line. The pointer to the last line is stored there
       instead of in the `lines` array because it gets reallocated regularly
       during edition. */
    stredit_t edit;

    /* Invariants:
       - capacity > 0
       - 0 <= size <= capacity
       - 0 <= start < capacity
       - When size is 0, start is undefined. */
    int16_t capacity, start, size;

    /* Total number of rendered lines for the buffer. */
    int16_t total_rendered;

    /* To keep track of lines' identity, the rotating array includes an extra
       numbering system. Each line is assigned an *absolute* line number which
       starts at 1 and increases every time a line is added. That number is
       independent of rotation.

       Absolute line number of the next line to be removed. This identifies
       the `start` line, unless the buffer is empty. Regardless, the interval
       [buf->absolute .. buf->absolute + buf->size) always covers exactly the
       set of lines that are held in the buffer. */
    int absolute;

    /* To avoid memory explosion, the rotating array can be set to clean up old
       lines when the total memory consumption is too high. `backlog_size`
       specifies how many bytes of text lines are allowed to hold. */
    int backlog_size;
    /* Total size of current lines, in bytes, excluding the last line. */
    int total_size_except_last;

    /* Last absolute line that has been both laid out for rendering and frozen
       for edition (ie. followed by another line). Lazy layout would start at
       `absolute_rendered+1`. */
    int absolute_rendered;

} linebuf_t;

/* Initialize a rotating buffer by allocating `line_count` lines. The buffer
   will allow up to `backlog_size` bytes of text data and clean up lines past
   that limit. This function does not free pre-existing data in `buf`. */
bool linebuf_init(linebuf_t *buf, int capacity, int backlog_size);

/* Free a rotating buffer and clean it up. */
void linebuf_deinit(linebuf_t *buf);

/* Absolute line numbers of the "start" and "end" of the buffer. The set of
   lines in the buffer is always [start ... end). If the buffer is empty, the
   interval is empty and neither line number is in the buffer. */
int linebuf_start(linebuf_t const *buf);
int linebuf_end(linebuf_t const *buf);

/* Get a pointer to the line with the given absolute number. */
console_fline_t *linebuf_get_line(linebuf_t const *buf, int absolute_number);

/* Get a pointer to the last line in the buffer (the editable line). */
stredit_t *linebuf_get_last_line(linebuf_t *buf);

/* Add a new line to the buffer (recycling an old one if needed). */
stredit_t *linebuf_new_line(linebuf_t *buf);

/* Recycle the `n` oldest lines from the buffer. */
void linebuf_recycle_oldest_lines(linebuf_t *buf, int n);

/* Clean up lines to try and keep the memory footprint of the text under
   `backlog_size` bytes. Always keeps at least the last line. */
void linebuf_clean_backlog(linebuf_t *buf);

/* Update the render width computation for all lines in the buffer. If `lazy`
   is false, all lines are re-laid out. But in the console the width often
   remains the same for many renders, and only the last line can be edited. In
   this case, `lazy` can be set to true, and only lines added or edited since
   the previous render will be re-laid out. */
void linebuf_update_render(linebuf_t *buf, int width, bool lazy);

//=== Terminal emulator ===//

typedef struct
{
    /* A rotating array of console_line_t. Never empty. */
    linebuf_t lines;

    /* Cursor position within the last line. */
    int16_t cursor;

    /* Whether new data has been added and a frame should be rendered. */
    bool render_needed;

    /* View geometry parameters from last console_compute_view(). */
    font_t const *render_font;
    int16_t render_width;
    int16_t render_lines;

} console_t;

/* Scroll position measured as a number of lines up from the bottom. */
typedef int console_scrollpos_t;

/* Create a new console with the specified backlog size. */
console_t *console_create(int backlog_size, int maximum_line_count);

/* Create a new empty line at the bottom of the console, and move the cursor
   there. Previous lines can no longer be edited. */
void console_newline(console_t *cons);

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
void console_render(int x, int y, console_t *cons, int dy,
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
bool console_write_raw(console_t *cons, char const *str, int n);

/* Write string at the cursor's position within the last line. This function
   interprets escape sequences and newlines. */
bool console_write(console_t *cons, char const *str, int n);

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
