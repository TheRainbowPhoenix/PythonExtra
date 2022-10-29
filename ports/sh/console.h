#ifndef __PYTHONEXTRA_CONSOLE_H
#define __PYTHONEXTRA_CONSOLE_H

#include <stdbool.h>

/* Maximum line length, to ensure the console can threshold its memory usage
   while cleaning only entire lines. Lines longer than this get split. */
#define PE_CONSOLE_LINE_MAX_LENGTH 1024

/* Line spacing in the console */
#ifdef FX9860G
# define PE_CONSOLE_LINE_SPACING 8
#else
# define PE_CONSOLE_LINE_SPACING 13
#endif

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
    int render_lines;

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

/* Insert n characters at position p. */
bool console_line_insert(console_line_t *line, int p, char const *str, int n);

/* Remove n characters at position p. */
void console_line_delete(console_line_t *line, int p, int n);

/* Update the number of render lines for the chosen width. */
void console_line_update_render_lines(console_line_t *line, int width);

//=== Console data storage ===//

typedef struct
{
    /* A dynamic array of console_line_t. Never empty. */
    console_line_t *lines;
    int line_count;

    /* Maximum (not 100% strict) amount of storage that is conserved in log.
       When this limit is exceeded, old lines are cleaned. This must be more
       than PE_CONSOLE_LINE_MAX_LENGTH. */
    int backlog_size;

    /* Cursor position within the last line. */
    int cursor;

} console_t;

/* Create a new console with the specified backlog size. */
console_t *console_create(int backlog_size);

/* Create a new empty line at the bottom of the console, and move the cursor
   there. Previous lines can no longer be edited. Returns false on error. */
bool console_new_line(console_t *cons);

/* Clean up backlog if the total memory usage is exceeded. */
void console_clean_backlog(console_t *cons);

/* Write string at the cursor's position within the last line. This writes a
   raw string without interpreting escape sequences and newlines. */
bool console_write_block_at_cursor(console_t *cons, char const *str, int n);

/* Write string at the cursor's position within the last line. This function
   interprets escape sequences and newlines. */
bool console_write_at_cursor(console_t *cons, char const *str, int n);

/* TODO: Expand this function */
void console_render(int x, int y, console_t *cons, int w, int lines);

void console_destroy(console_t *cons);

#endif /* __PYTHONEXTRA_CONSOLE_H */
