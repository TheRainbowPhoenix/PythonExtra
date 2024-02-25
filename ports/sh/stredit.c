//---------------------------------------------------------------------------//
//    ____        PythonExtra                                                //
//.-'`_ o `;__,   A community port of MicroPython for CASIO calculators.     //
//.-'` `---`  '   License: MIT (except some files; see LICENSE)              //
//---------------------------------------------------------------------------//

#include "stredit.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <gint/defs/util.h>

bool stredit_init(stredit_t *ed, int prealloc_size, int reserved_bytes,
    int max_size)
{
    if(!max_size) {
        memset(ed, 0, sizeof *ed);
        return true;
    }

    char *raw = malloc(reserved_bytes + prealloc_size + 1);
    if(!raw)
        return false;

    memset(raw, 0, reserved_bytes);
    raw[reserved_bytes] = 0;
    ed->raw = raw;

    ed->reserved = reserved_bytes;
    ed->max_size = max_size;
    ed->size = 0;
    ed->alloc_size = reserved_bytes + prealloc_size + 1;
    ed->prefix = 0;
    return true;
}

char *stredit_freeze_and_reset(stredit_t *ed)
{
    /* Downsize the allocation if it's larger than needed. */
    int size_needed = ed->reserved + ed->size + 1;
    if(ed->alloc_size >= size_needed + 4)
        ed->raw = realloc(ed->raw, size_needed);

    char *raw = ed->raw;
    ed->raw = NULL;
    stredit_reset(ed);
    return raw;
}

void stredit_reset(stredit_t *ed)
{
    free(ed->raw);
    memset(ed, 0, sizeof *ed);
}

bool stredit_alloc(stredit_t *ed, int n)
{
    if(ed->alloc_size >= ed->reserved + n + 1)
        return true;

    /* Always increase the size by at least 16 so we can insert many times in a
       row without worrying about excessive allocations. */
    int newsize = max(ed->alloc_size + 16, ed->reserved + n + 1);
    char *newraw = realloc(ed->raw, newsize);
    if(!newraw)
        return false;

    ed->raw = newraw;
    ed->alloc_size = newsize;
    return true;
}

void stredit_set_prefix(stredit_t *ed, int prefix_size)
{
    ed->prefix = min(max(0, prefix_size), (int)ed->size);
}

bool stredit_insert(stredit_t *ed, int p, char const *insert_str, int n)
{
    if(p < 0 || p > ed->size || ed->size + n > ed->max_size)
        return false;
    if(!stredit_alloc(ed, ed->size + n))
        return false;

    char *str = ed->raw + ed->reserved;

    /* Move the end of the string (plus the NUL) n bytes forward */
    memmove(str + p + n, str + p, ed->size - p + 1);
    memcpy(str + p, insert_str, n);
    ed->size += n;
    return true;
}

int stredit_delete(stredit_t *ed, int p, int n)
{
    if(p < ed->prefix) {
        int unremovable = ed->prefix - p;
        p += unremovable;
        n -= unremovable;
    }
    n = min(n, ed->size - p);

    if(n < 0)
        return 0;

    char *str = ed->raw + ed->reserved;

    /* Move the end of the string (plus the NUL) n bytes backwards */
    memmove(str + p, str + p + n, ed->size - n - p + 1);
    ed->size -= n;
    return n;
}
