//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/defs/util.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"

#include "py/mphal.h"
#include "../../shared/readline/readline.h"
#include "keymap.h"

//=== Dynamic console lines ===//

bool console_line_init(console_line_t *line, int prealloc_size)
{
    char *data = malloc(prealloc_size + 1);
    if(!data)
        return false;

    data[0] = 0;
    line->data = data;
    line->size = 0;
    line->alloc_size = prealloc_size + 1;
    line->render_lines = 0;
    line->prefix = 0;
    return true;
}

void console_line_deinit(console_line_t *line)
{
    free(line->data);
    line->data = NULL;
    line->size = 0;
    line->alloc_size = 0;
}

bool console_line_alloc(console_line_t *line, int n)
{
    if(line->alloc_size >= n + 1)
        return true;

    /* Always increase the size by at least 16 so we can insert many times in a
       row without worrying about excessive insertions. */
    int newsize = max(line->alloc_size + 16, n + 1);
    char *newdata = realloc(line->data, newsize);
    if(!newdata)
        return false;

    line->data = newdata;
    line->alloc_size = newsize;
    return true;
}

int console_line_capacity(console_line_t *line)
{
    return PE_CONSOLE_LINE_MAX_LENGTH - line->size;
}

void console_line_set_prefix(console_line_t *line, int prefix_size)
{
    line->prefix = min(max(0, prefix_size), line->size);
}

bool console_line_insert(console_line_t *line, int p, char const *str, int n)
{
    if(p < 0 || p > line->size || n > console_line_capacity(line))
        return false;
    if(!console_line_alloc(line, line->size + n))
        return false;

    /* Move the end of the string (plus the NUL) n bytes forward */
    memmove(line->data + p + n, line->data + p, line->size - p + 1);
    memcpy(line->data + p, str, n);
    line->size += n;
    return true;
}

int console_line_delete(console_line_t *line, int p, int n)
{
    if(p < line->prefix) {
        int unremovable = line->prefix - p;
        p += unremovable;
        n -= unremovable;
    }
    n = min(n, line->size - p);

    if(n < 0)
        return 0;

    /* Move the end of the string (plus the NUL) n bytes backwards */
    memmove(line->data + p, line->data + p + n, line->size - n - p + 1);
    line->size -= n;
    return n;
}

void console_line_update_render_lines(console_line_t *line, int width)
{
    line->render_lines = 0;

    char const *p = line->data;
    char const *endline = p + strlen(p);

    if(endline == p) {
        line->render_lines = 1;
        return;
    }

    while(p < endline) {
        line->render_lines++;
        p = drsize(p, NULL, width, NULL);
    }
}

int console_line_render(int x, int y, console_line_t *line, int w, int dy,
    int show_from, int show_until, int cursor)
{
    char const *p = line->data;
    char const *endline = p + strlen(p);
    int line_offset = 0;
    int line_number = 0;

    while(p < endline) {
        char const *endscreen = drsize(p, NULL, w, NULL);
        int len = endscreen - p;

        if(line_number >= show_from && line_number < show_until) {
            dtext_opt(x, y, C_BLACK, C_NONE, DTEXT_LEFT, DTEXT_TOP, p, len);
            if(cursor >= line_offset && cursor <= line_offset + len) {
                int w, h;
                dnsize(p, cursor - line_offset, NULL, &w, &h);
                dline(x+w, y, x+w, y+h-1, C_BLACK);
            }
            y += dy;
        }

        p += len;
        line_offset += len;
        line_number++;
    }

    return y;
}

//=== Terminal emulator ===//

console_t *console_create(int backlog_size)
{
    console_t *cons = malloc(sizeof *cons);
    cons->lines = NULL;
    cons->line_count = 0;

    cons->absolute_lineno = 1;
    cons->backlog_size = max(backlog_size, PE_CONSOLE_LINE_MAX_LENGTH);

    cons->render_needed = true;
    cons->render_font = NULL;
    cons->render_width = 0;
    cons->render_lines = 0;
    cons->render_total_lines = 0;

    if(!console_new_line(cons)) {
        free(cons);
        return NULL;
    }
    return cons;
}

bool console_new_line(console_t *cons)
{
    int newsize = (cons->line_count + 1) * sizeof *cons->lines;
    console_line_t *newlines = realloc(cons->lines, newsize);
    if(!newlines)
        return false;
    cons->lines = newlines;

    /* If this fails we keep the extended lines buffer. The next realloc() will
       be a no-op and we don't have to track anything. */
    if(!console_line_init(&cons->lines[cons->line_count], 16))
        return false;
    cons->line_count++;
    cons->cursor = 0;
    cons->render_needed = true;

    /* Routinely clean the backlog every time a line is added. */
    console_clean_backlog(cons);
    return true;
}

/* static console_line_t *console_get_absolute_line(console_t const *cons,
    int lineno)
{
    int i = lineno - cons->absolute_lineno;
    return (i >= 0 && i < cons->line_count) ? &cons->lines[i] : NULL;
} */

void console_clean_backlog(console_t *cons)
{
    /* Find how many lines we can keep while using at most cons->backlog_size
       bytes to store them. The console must always have at least one line. */
    int first_kept = cons->line_count;
    int line_size = 0;
    int total_size = 0;

    do {
        total_size += line_size;
        first_kept--;
        line_size = cons->lines[first_kept].size;
    }
    while(first_kept > 0 && total_size + line_size <= cons->backlog_size);

    /* Remove old lines */
    for(int j = 0; j < first_kept; j++)
        console_line_deinit(&cons->lines[j]);

    /* Don't realloc() yet, it will happen soon enough anyway */
    memmove(cons->lines, cons->lines + first_kept,
        (cons->line_count - first_kept) * sizeof cons->lines[0]);
    cons->line_count -= first_kept;

    /* Update the absolute number of the first line */
    cons->absolute_lineno += first_kept;

    cons->render_needed = true;
}

void console_clear_render_flag(console_t *cons)
{
    cons->render_needed = false;
}

void console_destroy(console_t *cons)
{
    for(int i = 0; i < cons->line_count; i++)
        console_line_deinit(&cons->lines[i]);
    free(cons->lines);
    free(cons);
}

//=== Rendering functions ===//

/* Compute the shell's horizontal layout as:
   1. Text width (region where the text is renderer)
   2. Spacing left of the scrollbar
   3. Width of the scrollbar */
static void view_params(int w,
    int *text_w, int *scroll_spacing, int *scroll_width)
{
#ifdef FX9860G
    if(text_w)
        *text_w = w - 2;
    if(scroll_spacing)
        *scroll_spacing = 1;
    if(scroll_width)
        *scroll_width = 1;
#else
    if(text_w)
        *text_w = w - 4;
    if(scroll_spacing)
        *scroll_spacing = 2;
    if(scroll_width)
        *scroll_width = 2;
#endif
}

void console_compute_view(console_t *cons, font_t const *font,
    int width, int lines)
{
    cons->render_font = font;
    cons->render_width = width;
    cons->render_lines = lines;
    cons->render_total_lines = 0;

    int text_w;
    view_params(width, &text_w, NULL, NULL);

    for(int i = 0; i < cons->line_count; i++) {
        console_line_update_render_lines(&cons->lines[i], text_w);
        cons->render_total_lines += cons->lines[i].render_lines;
    }
}

console_scrollpos_t console_clamp_scrollpos(console_t const *cons,
    console_scrollpos_t pos)
{
    /* No scrolling case */
    if(cons->render_total_lines < cons->render_lines)
        return 0;

    /* Normal clamp */
    return max(0, min(pos, cons->render_total_lines - cons->render_lines));
}

void console_render(int x, int y0, console_t const *cons, int dy,
    console_scrollpos_t pos)
{
    int total_lines = cons->render_total_lines;
    int visible_lines = cons->render_lines;
    int w = cons->render_width;
    int watermark = visible_lines - total_lines + pos;
    int y = y0;

    int text_w, scroll_spacing, scroll_w;
    view_params(w, &text_w, &scroll_spacing, &scroll_w);

    font_t const *old_font = dfont(cons->render_font);

    /* Show only visible lines */
    for(int i = 0; i < cons->line_count; i++) {
        console_line_t *line = &cons->lines[i];
        bool show_cursor = (i == cons->line_count - 1);

        if(watermark + line->render_lines > 0)
            y = console_line_render(x, y, line, text_w, dy, -watermark,
                visible_lines - watermark, show_cursor ? cons->cursor : -1);
        watermark += line->render_lines;
    }

    dfont(old_font);

    /* Scrollbar */
    if(total_lines > visible_lines) {
        int first_shown = total_lines - visible_lines - pos;
        int h = dy * visible_lines;
        int y1 = y0 + h * first_shown / total_lines;
        int y2 = y0 + h * (first_shown + visible_lines) / total_lines;

        int color = C_BLACK;
#ifdef FXCG50
        if(pos == 0) color = C_RGB(24, 24, 24);
#endif
        drect(x + text_w + scroll_spacing, y1,
              x + text_w + scroll_spacing + scroll_w - 1, y2,
              color);
    }
}

//=== Edition functions ===//

bool console_set_cursor(console_t *cons, int pos)
{
    console_line_t *last_line = &cons->lines[cons->line_count - 1];

    if(pos < last_line->prefix || pos > last_line->size)
        return false;

    cons->cursor = pos;
    return true;
}

bool console_move_cursor(console_t *cons, int cursor_movement)
{
    return console_set_cursor(cons, cons->cursor + cursor_movement);
}

char *console_get_line(console_t *cons, bool copy)
{
    console_line_t *last_line = &cons->lines[cons->line_count - 1];
    char *str = last_line->data + last_line->prefix;

    return copy ? strdup(str) : str;
}

bool console_write_block_at_cursor(console_t *cons, char const *str, int n)
{
    if(!cons->line_count && !console_new_line(cons))
        return false;

    /* Split up n characters into chunks that remain within each line's storage
       capacity. */
    while(n > 0) {
        console_line_t *last_line = &cons->lines[cons->line_count - 1];
        int capacity = console_line_capacity(last_line);
        int round_size = min(n, capacity);

        if(!console_line_insert(last_line, cons->cursor, str, round_size))
            return false;
        cons->cursor += round_size;
        cons->render_needed = true;
        if(round_size < n && !console_new_line(cons))
            return false;
        n -= round_size;
    }

    return true;
}

bool console_write_at_cursor(console_t *cons, char const *buf, int n)
{
    int offset = 0;
    if(n < 0)
        n = strlen(buf);

    while(offset < n) {
        /* Find the first '\n', '\e' or end of buffer */
        char const *end = buf + offset;
        while(end < buf + n && *end != '\n' && *end != '\e' && *end != 8)
            end++;

        int line_size = end - (buf + offset);
        if(!console_write_block_at_cursor(cons, buf + offset, line_size))
            return false;

        offset += line_size;
        if(offset >= n)
            break;

        if(buf[offset] == '\n') {
            if(!console_new_line(cons))
                return false;
            offset++;
        }
        else if(buf[offset] == 8) {
            offset++;
            console_line_t *last_line = &cons->lines[cons->line_count - 1];
            if(cons->cursor > 0) {
                console_line_delete(last_line, cons->cursor-1, 1);
                cons->cursor--;
            }
        }
        else if(buf[offset] == '\e') {
            console_line_t *last_line = &cons->lines[cons->line_count - 1];
            offset++;

            /* TODO: Handle more complex escape sequences */
            if(offset + 2 <= n && buf[offset] == '[' && buf[offset+1] == 'K') {
                console_line_delete(last_line, cons->cursor,
                    last_line->size - cons->cursor);
                offset += 2;
            }
            if(offset + 2 <= n && buf[offset] == 1 && buf[offset+1] == 'D') {
                cons->cursor -= (cons->cursor > 0);
            }
            if(offset + 2 <= n && buf[offset] == 1 && buf[offset+1] == 'C') {
                cons->cursor += (cons->cursor < last_line->size);
            }
        }

        cons->render_needed = true;
    }

    return true;
}

void console_lock_prefix(console_t *cons)
{
    console_line_set_prefix(&cons->lines[cons->line_count - 1], cons->cursor);
}

void console_delete_at_cursor(console_t *cons, int n)
{
    console_line_t *last_line = &cons->lines[cons->line_count - 1];
    int real_n = console_line_delete(last_line, cons->cursor - n, n);
    cons->cursor -= real_n;
    cons->render_needed = true;
}

void console_clear_current_line(console_t *cons)
{
    console_line_t *last_line = &cons->lines[cons->line_count - 1];
    console_line_delete(last_line, last_line->prefix,
        last_line->size - last_line->prefix);
    cons->cursor = last_line->prefix;
    cons->render_needed = true;
}

//=== Terminal input ===//

int console_key_event_to_char(key_event_t ev)
{
    int key = ev.key;

    if(key == KEY_LEFT && ev.shift)
        return CHAR_CTRL_A; /* go-to-start-of-line */
    if(key == KEY_LEFT)
        return CHAR_CTRL_B; /* go-back-one-char */
    if(key == KEY_ACON)
        return CHAR_CTRL_C; /* cancel */
    if(key == KEY_DEL)
        return 8; /* delete-at-cursor */
    if(key == KEY_RIGHT && ev.shift)
        return CHAR_CTRL_E; /* go-to-end-of-line */
    if(key == KEY_RIGHT)
        return CHAR_CTRL_F; /* go-forward-one-char */
    if(key == KEY_DOWN)
        return CHAR_CTRL_N; /* go to next line in history */
    if(key == KEY_UP)
        return CHAR_CTRL_P; /* go to previous line in history */
    if(key == KEY_EXIT)
        return CHAR_CTRL_D; /* eof */
    if(key == KEY_EXE)
        return '\r';

    return keymap_translate(key, ev.shift, ev.alpha);
}
