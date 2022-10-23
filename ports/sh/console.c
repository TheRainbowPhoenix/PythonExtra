#include <gint/keyboard.h>
#include <gint/display.h>
#include <gint/defs/util.h>
#include <stdlib.h>
#include <string.h>
#include "console.h"

console_t *console_create(int max_lines)
{
    console_t *cons = malloc(sizeof *cons);
    cons->max_lines = max_lines;
    cons->lines = calloc(max_lines, sizeof *cons->lines);
    for(int i = 0; i < max_lines; i++)
        cons->lines[i] = NULL;
    console_new_line(cons);
    return cons;
}

void console_new_line(console_t *cons)
{
    int n = cons->max_lines;
    free(cons->lines[0]);

    for(int i = 0; i < n - 1; i++)
        cons->lines[i] = cons->lines[i+1];

    cons->lines[n - 1] = strdup("");
}

static void console_append_to_last_line(console_t *cons, void const *buf,
    int size)
{
    int n = cons->max_lines;
    char *l = cons->lines[n - 1];
    int prev_size = l ? strlen(l) : 0;
    int new_size = prev_size + size + 1;

    l = realloc(l, new_size);
    if(!l)
        return;

    memcpy(l + prev_size, buf, size);
    l[new_size - 1] = 0;
    cons->lines[n - 1] = l;
}

void console_append_str(console_t *cons, char const *str)
{
    console_append_buf(cons, str, strlen(str));
}

void console_append_buf(console_t *cons, void const *buf, size_t size_)
{
    int offset = 0, size = size_;

    while(offset < size) {
        /* Find the first '\n' (or end of buffer) in the segment */
        void const *endline = memchr(buf + offset, '\n', size - offset);
        if(endline == NULL)
            endline = buf + size;

        int line_size = endline - (buf + offset);
        console_append_to_last_line(cons, buf + offset, line_size);

        offset += line_size;

        /* Found a '\n' */
        if(offset < size) {
            console_new_line(cons);
            offset++;
        }
    }
}

void console_render(int x, int y, console_t *cons, int w, int h)
{
    int dy = 13;

    for(int i = 0; i < cons->max_lines; i++) {
        /* Skip initial unallocated lines */
        if(cons->lines[i] == NULL)
            continue;

        char const *p = cons->lines[i];
        char const *endline = p + strlen(p);

        while(p < endline) {
            if(p[0] == '\n') {
                y += dy;
                p++;
                continue;
            }

            char const *endscreen = drsize(p, NULL, w, NULL);
            char const *endline = strchrnul(p, '\n');
            int len = min(endscreen - p, endline - p);

            dtext_opt(x, y, C_BLACK, C_NONE, DTEXT_LEFT, DTEXT_TOP, p, len);
            y += dy;
            p += len + (p[len] == '\n');
        }
    }
}

void console_destroy(console_t *cons)
{
    for(int i = 0; i < cons->max_lines; i++)
        free(cons->lines[i]);
    free(cons);
}
