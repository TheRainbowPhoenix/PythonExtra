//---
// gint:kmalloc:kmalloc - Main allocator routines
//---

#include <gint/kmalloc.h>
#include <gint/defs/util.h>
#include <gint/config.h>

#include <string.h>

/* Maximum number of arenas */
#define KMALLOC_ARENA_MAX 8

/* List of arenas in order of consideration */
static kmalloc_arena_t *arenas[KMALLOC_ARENA_MAX] = { 0 };

/* kmalloc_init(): Initialize the dynamic allocator */
void kmalloc_init(void)
{
	/* Provide the OS heap */
	extern kmalloc_arena_t kmalloc_arena_osheap;
	arenas[KMALLOC_ARENA_MAX - 1] = &kmalloc_arena_osheap;
}

//---
// Allocation functions
//---

kmalloc_arena_t *kmalloc_get_arena(char const *name)
{
	for(int i = 0; i < KMALLOC_ARENA_MAX; i++)
	{
		if(arenas[i] && !strcmp(arenas[i]->name, name))
			return arenas[i];
	}
	return NULL;
}

/* Find the arena that contains a given block */
static kmalloc_arena_t *arena_owning(void *ptr)
{
	for(int i = 0; i < KMALLOC_ARENA_MAX; i++)
	{
		kmalloc_arena_t *a = arenas[i];
		if(!a) continue;

		if((a->start <= ptr && ptr < a->end) ||
		   (a->start == NULL && a->end == NULL))
			return a;
	}
	return NULL;
}

/* kmalloc(): Allocate memory in one of the available arenas */
void *kmalloc(size_t size, char const *name)
{
	if(size == 0) return NULL;

	for(int i = 0; i < KMALLOC_ARENA_MAX; i++) if(arenas[i])
	{
		kmalloc_arena_t *a = arenas[i];
		if(name && strcmp(a->name, name)) continue;
		if(!name && !a->is_default) continue;

		/* Try to allocate in this arena */
		void *rc = a->malloc(size, a->data);

		/* Maintain statistics */
		struct kmalloc_stats *s = &a->stats;
		if(rc)
		{
			s->live_blocks++;
			s->peak_live_blocks = max(s->live_blocks,
				s->peak_live_blocks);
			s->total_volume += size;
			s->total_blocks++;
			return rc;
		}
		else
		{
			s->total_failures++;
		}
	}

	return NULL;
}

/* krealloc(): Reallocate memory */
void *krealloc(void *ptr, size_t size)
{
	if(!ptr)
	{
		return kmalloc(size, NULL);
	}
	if(!size)
	{
		kfree(ptr);
		return NULL;
	}

	kmalloc_arena_t *a = arena_owning(ptr);
	if(!a) return NULL;

	void *rc = a->realloc(ptr, size, a->data);

	/* Maintain statistics */
	if(rc)
	{
		a->stats.total_volume += size;
		a->stats.total_blocks++;
	}
	else
	{
		a->stats.total_failures++;

		/* If reallocation within the original arena fails, try another
		   one. The memory copy behavior is sub-optimal (we copy the
		   new size which might be more than the original size) but
		   it's all we can do with this arena interface. */
		rc = kmalloc(size, NULL);
		if(rc)
		{
			memcpy(rc, ptr, size);
		}
	}

	return rc;
}

/* kfree(): Free memory allocated with kalloc() */
void kfree(void *ptr)
{
	if(!ptr) return;

	/* If this condition fails, then the pointer is invalid */
	kmalloc_arena_t *a = arena_owning(ptr);
	if(!a) return;

	a->free(ptr, a->data);
	/* Maintain statistics */
	a->stats.live_blocks--;
}

/* kmalloc_max(): Allocate the largest block available in an arena */
void *kmalloc_max(size_t *size, char const *name)
{
	for(int i = 0; i < KMALLOC_ARENA_MAX; i++) if(arenas[i])
	{
		kmalloc_arena_t *a = arenas[i];
		if(strcmp(a->name, name)) continue;

		void *rc = a->malloc_max ? a->malloc_max(size, a->data) : NULL;

		/* Maintain statistics */
		struct kmalloc_stats *s = &a->stats;
		if(rc)
		{
			s->live_blocks++;
			s->peak_live_blocks = max(s->live_blocks,
				s->peak_live_blocks);
			s->total_volume += *size;
			s->total_blocks++;
			return rc;
		}
		else
		{
			s->total_failures++;
		}
	}

	return NULL;
}

/* kmalloc_add_arena(): Add a new arena to the heap source */
bool kmalloc_add_arena(kmalloc_arena_t *arena)
{
	for(int i = 0; i < KMALLOC_ARENA_MAX; i++)
	{
		if(!arenas[i])
		{
			arenas[i] = arena;
			return true;
		}
	}
	return false;
}

void kmalloc_remove_arena(kmalloc_arena_t *arena)
{
	for(int i = 0; i < KMALLOC_ARENA_MAX; i++)
	{
		if(arenas[i] == arena)
			arenas[i] = NULL;
	}
}
