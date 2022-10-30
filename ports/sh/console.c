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

void console_line_delete(console_line_t *line, int p, int n)
{
    n = min(n, line->size - p);

    /* Move the end of the string (plus the NUL) n bytes backwards */
    memmove(line->data + p, line->data + p + n, line->size - n - p + 1);
    line->size -= n;
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
    int show_from, int cursor)
{
    char const *p = line->data;
    char const *endline = p + strlen(p);
    int line_offset = 0;
    int line_number = 0;

    while(p < endline) {
        char const *endscreen = drsize(p, NULL, w, NULL);
        int len = endscreen - p;

        if(line_number >= show_from) {
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
    cons->backlog_size = max(backlog_size, PE_CONSOLE_LINE_MAX_LENGTH);
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

    /* Routinely clean the backlog every time a line is added. */
//    console_clean_backlog(cons);
    return true;
}

void console_clean_backlog(console_t *cons)
{
    int total_size = 0, i = cons->line_count;

    /* Determine how many lines (starting from the end) we can fit */
    while(total_size < cons->backlog_size) {
        i--;
        total_size += cons->lines[i].size;
    }

    /* Make sure to keep at least one line */
    int delete_until = i + (i < cons->line_count - 1);
    int remains = cons->line_count - delete_until;

    /* Remove `delete_until` lines */
    for(int j = 0; j < delete_until; j++)
        console_line_deinit(&cons->lines[j]);

    /* Don't realloc() yet, it will happen soon enough anyway */
    memmove(cons->lines, cons->lines + delete_until, remains);
    cons->line_count = remains;
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
        if(round_size < n && !console_new_line(cons))
            return false;
        n -= round_size;
    }

    return true;
}

bool console_write_at_cursor(console_t *cons, char const *buf, int n)
{
    int offset = 0;

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
    }

    return true;
}

void console_render(int x, int y, console_t *cons, int w, int lines)
{
    int watermark = lines;

    for(int i = 0; i < cons->line_count; i++) {
        console_line_update_render_lines(&cons->lines[i], w);
        watermark -= cons->lines[i].render_lines;
    }

    /* Show only visible lines */
    for(int i = 0; i < cons->line_count; i++) {
        console_line_t *line = &cons->lines[i];
        bool show_cursor = (i == cons->line_count - 1);

        if(watermark + line->render_lines > 0)
            y = console_line_render(x, y, line, w, PE_CONSOLE_LINE_SPACING,
                -watermark, show_cursor ? cons->cursor : -1);
        watermark += line->render_lines;
    }
}

void console_destroy(console_t *cons)
{
    for(int i = 0; i < cons->line_count; i++)
        console_line_deinit(&cons->lines[i]);
    free(cons->lines);
    free(cons);
}
