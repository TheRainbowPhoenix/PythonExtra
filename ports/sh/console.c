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
    memset(line, 0, sizeof *line);
}

bool console_line_alloc(console_line_t *line, int n)
{
    if(line->alloc_size >= n + 1)
        return true;

    /* Always increase the size by at least 16 so we can insert many times in a
       row without worrying about excessive allocations. */
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
    char const *p = line->data;
    line->render_lines = 0;

    do {
        line->render_lines++;
        p = drsize(p, NULL, width, NULL);
    }
    while(*p);
}

int console_line_render(int x, int y, console_line_t *line, int w, int dy,
    int show_from, int show_until, int cursor)
{
    char const *p = line->data;
    char const *endline = p + strlen(p);
    int line_offset = 0;
    int line_number = 0;

    if(p == endline && cursor == 0) {
        int h;
        dsize("", NULL, NULL, &h);
        dline(x, y, x, y+h-1, C_BLACK);
    }
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

//=== Rotating line storage ===//

bool linebuf_init(linebuf_t *buf, int capacity, int backlog_size)
{
    if(capacity <= 0)
        return false;

    buf->lines = malloc(capacity * sizeof *buf->lines);
    if(!buf->lines)
        return false;

    buf->capacity = capacity;
    buf->start = 0;
    buf->size = 0;
    buf->absolute = 1;
    buf->backlog_size = backlog_size;
    buf->total_size_except_last = 0;
    buf->absolute_rendered = 0;
    buf->total_rendered = 0;
    return true;
}

void linebuf_deinit(linebuf_t *buf)
{
    free(buf->lines);
    memset(buf, 0, sizeof *buf);
}

/* Lines in the buffer are identified by their positions within the `lines`
   array, which are integers equipped with modulo arithmetic (we call them
   "indices"). We abstract away the rotation by numbering stored lines from 0
   to buf->size - 1, and we call these numbers "nths". When we want to identify
   lines independent of rotation, we use their absolute line number.

   Such numbers refer to lines stored in the buffer if:
     (index)   0 <= linebuf_index_to_nth(buf, index) < size
     (nth)     0 <= nth < size
     (abs)     0 <= abs - buf->absolute < size */

GINLINE static int linebuf_nth_to_index(linebuf_t const *buf, int nth)
{
    return (buf->start + nth) % buf->capacity;
}

/* Get the nth line. */
GINLINE static console_line_t *linebuf_get_nth_line(linebuf_t const *buf,
    int nth)
{
    if(nth < 0 || nth >= buf->size)
        return NULL;

    return &buf->lines[linebuf_nth_to_index(buf, nth)];
}

/* Move `index` by `diff`; assumes |diff| <= buf->capacity. */
GINLINE static int linebuf_index_add(linebuf_t const *buf, int index, int diff)
{
    return (index + diff + buf->capacity) % buf->capacity;
}

int linebuf_start(linebuf_t const *buf)
{
    return buf->absolute;
}

int linebuf_end(linebuf_t const *buf)
{
    return buf->absolute + buf->size;
}

console_line_t *linebuf_get_line(linebuf_t const *buf, int abs)
{
    return linebuf_get_nth_line(buf, abs - buf->absolute);
}

console_line_t *linebuf_get_last_line(linebuf_t const *buf)
{
    return linebuf_get_nth_line(buf, buf->size - 1);
}

console_line_t *linebuf_newline(linebuf_t *buf)
{
    /* Make space if the buffer is full */
    linebuf_recycle_oldest_lines(buf, buf->size - buf->capacity + 1);

    /* Freeze the current last line's size */
    if(buf->size > 0) {
        console_line_t *L = linebuf_get_nth_line(buf, buf->size - 1);
        buf->total_size_except_last += L->size;
    }

    buf->size++;
    console_line_t *L = linebuf_get_nth_line(buf, buf->size - 1);
    console_line_init(L, 16);
    return L;
}

void linebuf_recycle_oldest_lines(linebuf_t *buf, int count)
{
    count = min(count, buf->size);
    if(count <= 0)
        return;

    for(int nth = 0; nth < count; nth++) {
        console_line_t *L = linebuf_get_nth_line(buf, nth);
        buf->total_rendered -= L->render_lines;
        if(nth != buf->size - 1)
            buf->total_size_except_last -= L->size;
        console_line_deinit(L);
    }

    buf->start = linebuf_index_add(buf, buf->start, count);
    buf->size -= count;
    buf->absolute += count;
}

void linebuf_clean_backlog(linebuf_t *buf)
{
    if(buf->size <= 0)
        return;

    int remove = 0;
    int n = buf->total_size_except_last + linebuf_get_last_line(buf)->size;

    while(remove < buf->size - 1 && n > buf->backlog_size) {
        n -= linebuf_get_nth_line(buf, remove)->size;
        remove++;
    }

    linebuf_recycle_oldest_lines(buf, remove);
}

void linebuf_update_render(linebuf_t *buf, int width, bool lazy)
{
    int start = linebuf_start(buf);
    int end = linebuf_end(buf);
    if(lazy)
        start = max(start, buf->absolute_rendered + 1);

    int text_w;
    view_params(width, &text_w, NULL, NULL);

    for(int abs = start; abs < end; abs++) {
        console_line_t *L = linebuf_get_nth_line(buf, abs - buf->absolute);
        buf->total_rendered -= L->render_lines;
        console_line_update_render_lines(L, text_w);
        buf->total_rendered += L->render_lines;
    }

    buf->absolute_rendered = max(buf->absolute_rendered, end - 2);
}

//=== Terminal emulator ===//

console_t *console_create(int backlog_size, int maximum_line_count)
{
    backlog_size = max(backlog_size, PE_CONSOLE_LINE_MAX_LENGTH);
    maximum_line_count = max(maximum_line_count, 1);

    console_t *cons = malloc(sizeof *cons);
    if(!linebuf_init(&cons->lines, maximum_line_count, backlog_size)) {
        free(cons);
        return NULL;
    }

    cons->cursor = -1;
    cons->render_needed = true;
    cons->render_font = NULL;
    cons->render_width = 0;
    cons->render_lines = 0;

    console_newline(cons);
    return cons;
}

void console_newline(console_t *cons)
{
    linebuf_newline(&cons->lines);
    cons->cursor = 0;
    cons->render_needed = true;

    /* This is a good time to clean up the backlog. */
    // TODO: This might actually be the performance bottleneck, because we do a
    // long loop only to find out that the total size is still reasonable!
    linebuf_clean_backlog(&cons->lines);
}

void console_clear_render_flag(console_t *cons)
{
    cons->render_needed = false;
}

void console_destroy(console_t *cons)
{
    linebuf_deinit(&cons->lines);
    free(cons);
}

//=== Rendering functions ===//

void console_compute_view(console_t *cons, font_t const *font,
    int width, int lines)
{
    /* If a view with the same width was previously computed, do a lazy
       update: recompute only the last lines. */
    bool lazy = (width != 0 && cons->render_width == width);
    cons->render_font = font;
    cons->render_width = width;
    cons->render_lines = lines;
    linebuf_update_render(&cons->lines, width, lazy);
}

console_scrollpos_t console_clamp_scrollpos(console_t const *cons,
    console_scrollpos_t pos)
{
    /* No scrolling case */
    if(cons->lines.total_rendered < cons->render_lines)
        return 0;

    return max(0, min(pos, cons->lines.total_rendered - cons->render_lines));
}

void console_render(int x, int y0, console_t const *cons, int dy,
    console_scrollpos_t pos)
{
    int total_lines = cons->lines.total_rendered;
    int visible_lines = cons->render_lines;
    int w = cons->render_width;
    int y = y0;

    int text_w, scroll_spacing, scroll_w;
    view_params(w, &text_w, &scroll_spacing, &scroll_w);

    font_t const *old_font = dfont(cons->render_font);

    /* Show only visible lines. We want to avoid counting all the lines in the
       console, and instead start from the end. */
    int line_y = visible_lines + pos;
    int L_start = linebuf_start(&cons->lines);
    int L_end = linebuf_end(&cons->lines);
    int i = linebuf_end(&cons->lines);

    while(i > L_start && line_y > 0)
        line_y -= linebuf_get_line(&cons->lines, --i)->render_lines;

    /* If there isn't enough content to fill the view, start at the top. */
    line_y = min(line_y, pos);

    while(i < L_end  && line_y < visible_lines) {
        console_line_t *line = linebuf_get_line(&cons->lines, i);
        bool show_cursor = (i == L_end - 1);

        y = console_line_render(x, y, line, text_w, dy, -line_y,
            visible_lines - line_y, show_cursor ? cons->cursor : -1);
        line_y += line->render_lines;
        i++;
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

GINLINE static console_line_t *last_line(console_t *cons)
{
    return linebuf_get_last_line(&cons->lines);
}

bool console_set_cursor(console_t *cons, int pos)
{
    console_line_t *L = last_line(cons);

    if(pos < L->prefix || pos > L->size)
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
    console_line_t *L = last_line(cons);
    char *str = L->data + L->prefix;
    return copy ? strdup(str) : str;
}

bool console_write_raw(console_t *cons, char const *str, int n)
{
    if(!cons->lines.size)
        console_newline(cons);

    /* Split string into chunks smaller than each line's storage capacity. */
    while(n > 0) {
        console_line_t *L = last_line(cons);
        int capacity = console_line_capacity(L);
        int round_size = min(n, capacity);

        if(!console_line_insert(L, cons->cursor, str, round_size))
            return false;
        cons->cursor += round_size;
        cons->render_needed = true;
        if(round_size < n)
            console_newline(cons);
        n -= round_size;
    }

    return true;
}

bool console_write(console_t *cons, char const *buf, int n)
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
        if(!console_write_raw(cons, buf + offset, line_size))
            return false;

        offset += line_size;
        if(offset >= n)
            break;

        if(buf[offset] == '\n') {
            console_newline(cons);
            offset++;
        }
        else if(buf[offset] == 8) {
            offset++;
            console_line_t *L = last_line(cons);
            if(cons->cursor > 0) {
                console_line_delete(L, cons->cursor-1, 1);
                cons->cursor--;
            }
        }
        else if(buf[offset] == '\e') {
            console_line_t *L = last_line(cons);
            offset++;

            /* TODO: Handle more complex escape sequences */
            if(offset + 2 <= n && buf[offset] == '[' && buf[offset+1] == 'K') {
                console_line_delete(L, cons->cursor,
                    L->size - cons->cursor);
                offset += 2;
            }
            if(offset + 2 <= n && buf[offset] == 1 && buf[offset+1] == 'D') {
                cons->cursor -= (cons->cursor > 0);
            }
            if(offset + 2 <= n && buf[offset] == 1 && buf[offset+1] == 'C') {
                cons->cursor += (cons->cursor < L->size);
            }
        }

        cons->render_needed = true;
    }

    return true;
}

void console_lock_prefix(console_t *cons)
{
    console_line_set_prefix(last_line(cons), cons->cursor);
}

void console_delete_at_cursor(console_t *cons, int n)
{
    int real_n = console_line_delete(last_line(cons), cons->cursor - n, n);
    cons->cursor -= real_n;
    cons->render_needed = true;
}

void console_clear_current_line(console_t *cons)
{
    console_line_t *L = last_line(cons);
    console_line_delete(L, L->prefix, L->size - L->prefix);
    cons->cursor = L->prefix;
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
