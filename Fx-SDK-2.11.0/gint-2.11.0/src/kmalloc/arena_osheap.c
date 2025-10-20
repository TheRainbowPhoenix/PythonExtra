//---
// gint:kmalloc:arena_osheap - An arena that uses the OS heap as input
//---

#include <gint/kmalloc.h>
#include <gint/defs/attributes.h>

/* Syscalls relating to the OS heap */
extern void *__malloc(size_t size);
extern void *__realloc(void *ptr, size_t newsize);
extern void __free(void *ptr);

static void *osheap_malloc(size_t size, GUNUSED void *data)
{
	return __malloc(size);
}

static void *osheap_realloc(void *ptr, size_t newsize, GUNUSED void *data)
{
	return __realloc(ptr, newsize);
}

static void osheap_free(void *ptr, GUNUSED void *data)
{
	return __free(ptr);
}

/* This is a global variable, it's pulled by kmalloc.c. This arena is the only
   one allowed to not specify start/end as the values are hard to determine. */
kmalloc_arena_t kmalloc_arena_osheap = {
	.malloc      = osheap_malloc,
	.realloc     = osheap_realloc,
	.free        = osheap_free,
	.name        = "_os",
	.start       = NULL,
	.end         = NULL,
	.data        = NULL,
	.is_default  = 1,
	.stats       = { 0 },
};
