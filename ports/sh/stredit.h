//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//
// pe.stredit: String edition utilities
//
// This header implements string edition utilities used in the console to
// handle user input and printing on the last line. To minimize memory
// friction, edited strings are stored in a format compatible with the
// storage format of frozen console lines, i.e. with a few reserved bytes at
// the beginning for metadata.
//---

#ifndef __PYTHONEXTRA_STREDIT_H
#define __PYTHONEXTRA_STREDIT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    /* The raw buffer has reserved bytes at the beginning. To avoid confusion,
       we call this initial pointer "raw" and raw+reserved "str". */
    char *raw;
    /* Maximum length (text only, not counting the NUL). */
    uint16_t max_size;
    /* Number of reserved bytes. */
    uint16_t reserved;
    /* Size of contents (not counting the NUL). */
    uint16_t size;
    /* Allocated size (always ≥ size+1). */
    uint16_t alloc_size;
    /* Number of initial characters that can't be edited. */
    uint16_t prefix;

} stredit_t;

/* Create a new editable string with at least init_chars characters of content
   available. Returns false on error. Previous contents are not freed! If
   max_size is 0, clears the stredit without allocating a new string. */
bool stredit_init(stredit_t *ed, int init_chars, int reserved_bytes,
    int max_size);

/* Get the data pointer out of an stredit. */
static inline char *stredit_data(stredit_t *ed)
{
    return ed->raw + ed->reserved;
}

/* Reset an editable string. This frees and destroys the string. */
void stredit_reset(stredit_t *ed);

/* Finish editing; return the raw pointer (with its ownership) and reset the
   editor structure. free() the raw pointer after use. */
char *stredit_freeze_and_reset(stredit_t *ed);

/* Number of bytes that can be added before the size limit is reached. */
static inline int stredit_capacity(stredit_t *ed)
{
    return ed->max_size - ed->size;
}

/* Realloc the string to ensure n characters plus a NUL can be written. */
bool stredit_alloc(stredit_t *ed, int n);

/* Set the prefix_size first characters of the strings to not be editable. The
   string must already have that many characters printed. This is used in the
   console to prevent erasing the prompt. */
void stredit_set_prefix(stredit_t *ed, int prefix_size);

/* Insert n characters at position p. */
bool stredit_insert(stredit_t *ed, int p, char const *str, int n);

/* Remove n characters at position p. Returns the number of characters
   actually removed after bounds checking. */
int stredit_delete(stredit_t *ed, int p, int n);

#endif /* __PYTHONEXTRA_STREDIT_H */
