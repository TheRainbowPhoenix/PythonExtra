#ifndef __PYTHONEXTRA_CONSOLE_H
#define __PYTHONEXTRA_CONSOLE_H

//=== Console data storage ===//

typedef struct
{
    /* An array lines[] of size [max_lines], filled from the end (always has a
       list of NULL followed by line pointers. */
    int max_lines;
    char **lines;

} console_t;

console_t *console_create(int max_lines);

void console_new_line(console_t *cons);

void console_append_str(console_t *cons, char const *str);

void console_append_buf(console_t *cons, void const *buf, size_t size);

/* TODO: Expand this function */
void console_render(int x, int y, console_t *cons, int w, int h);

void console_destroy(console_t *cons);

#endif /* __PYTHONEXTRA_CONSOLE_H */
