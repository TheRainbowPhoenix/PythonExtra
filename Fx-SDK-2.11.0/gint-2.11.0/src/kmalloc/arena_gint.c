//---
// gint:kmalloc:arena_gint - An arena that uses gint's custom allocator
//---

#include <gint/kmalloc.h>
#include <gint/defs/attributes.h>
#include <gint/defs/util.h>
#include <gint/config.h>

#include <string.h>

/* block_t: A memory block managed by the heap.

   The heap is a sequence of blocks made of a block_t header (4 bytes) and raw
   data (any size between 8 bytes and the size of the heap). The sequence
   extends from the start of the arena region (past the index structure) up to
   the end of the arena region.

   Free blocks use the unused raw data to store a footer of either 8 or 12
   bytes, which links to the start of the block, the previous free block, and
   the next free block. This forms a doubly-linked list of free blocks (or, to
   be more precise, several intertwined doubly-linked lists, each handling a
   different class of block sizes).

   The possible ways to traverse the structure are:
   1. Traverse the sequence from left to right -> next_block()
   2. Go back to the previous block, if it's free -> previous_block_if_free()
   3. Traverse each linked list from left to right -> next_link()
   4. Traverse each linked list from right to left -> previous_link()

   Way #2 is implemented using the boundary tag optimization. Basically each
   block has a bit (the boundary tag) that tells whether the previous block is
   free. If it's free, then that block's footer can be accessed, and because
   the footer contains the size the header can be accessed too. This is used to
   detect whether to merge into the previous block after a free().

   The allocation algorithm will mostly use way #3 to find free blocks. When
   freeing, ways #1 and #2 are used to coalesce adjacent blocks. Ways #3 and #4
   are used to maintain the linked lists.

   The footer uses 8 bytes if the block has 8 bytes of raw data, and 12 bytes
   otherwise. The LSB of the last byte is used to distinguish the cases:
   * For a block of 8 bytes, the footer has one block_t pointer to the previous
     link, then one block_t pointer to the next link with LSB=1
   * For a larger block, the footer has a 4-byte block size, then a pointer to
     the previous link, and a pointer to the next link with LSB=0. */
typedef volatile struct {
	uint32_t :5;
	/* Marks the last block of the sequence */
	uint32_t last: 1;
	/* Whether the block is used; in general this can be kept implicit, but
	   it has to be specified for the last block */
	uint32_t used: 1;
	/* Boundary tag, vital to implement way #2 to merge adjacent blocks */
	uint32_t previous_used: 1;
	/* Block size in bytes. */
	uint32_t size: 24;
} block_t;

typedef kmalloc_gint_stats_t stats_t;

/* index_t: Data structure at the root of the heap, indexing linked lists */
typedef struct {
	/* Entry points of the free lists for each block size */
	block_t *classes[16];
	/* Pointer to statistics, if used */
	stats_t *stats;
} index_t;

//---
// Block-level operations
//---

/* Returns a pointer to the next block in the sequence (might be used) */
static block_t *next_block(block_t *b)
{
	if(b->last) return NULL;
	return (void *)b + sizeof(block_t) + b->size;
}

/* Returns a pointer to the previous block's header, if it's a free block */
static block_t *previous_block_if_free(block_t *b)
{
	if(b->previous_used) return NULL;
	/* The footer of the previous block indicates its size */
	uint32_t *footer = (void *)b;
	uint32_t previous_size = (footer[-1] & 1) ? 8 : footer[-3];
	return (void *)b - previous_size - sizeof(block_t);
}

/* Splits a used or free-floating block into a first block with (size) bytes
   and a free second block with the rest. Returns the address of the second
   block. If the initial block is too small to split, returns NULL. */
static block_t *split(block_t *b, int size)
{
	size_t extra_size = b->size - size;
	if(extra_size < sizeof(block_t) + 8) return NULL;

	block_t *second = (void *)b + sizeof(block_t) + size;
	second->last = b->last;
	second->used = false;
	second->previous_used = b->used;
	second->size = extra_size - sizeof(block_t);

	block_t *third = next_block(second);
	if(third) third->previous_used = second->used;

	b->last = 0;
	b->size = size;

	return second;
}

/* Merge a used or free-floating block with its free neighbor. There are two
   parameters for clarity, but really (right == next_block(left)). */
static void merge(block_t *left, block_t *right)
{
	size_t extra_size = sizeof(block_t) + right->size;
	left->last = right->last;
	left->size += extra_size;

	block_t *next = next_block(left);
	if(next) next->previous_used = left->used;
}

//---
// List-level operations
//---

/* Returns the next free block in the list, assumes (b) is free */
static block_t *next_link(block_t *b)
{
	uint32_t *footer = (void *)b + sizeof(block_t) + b->size;
	return (block_t *)(footer[-1] & ~3);
}

/* Returns the previous free block in the list, assumes (b) is free */
static block_t *previous_link(block_t *b)
{
	uint32_t *footer = (void *)b + sizeof(block_t) + b->size;
	return (block_t *)footer[-2];
}

/* Writes the given free block links to the footer of free block (b) */
static void set_footer(block_t *b, block_t *previous_link, block_t *next_link)
{
	uint32_t *footer = (void *)b + sizeof(block_t) + b->size;
	/* 8-byte block: store the next link with LSB=1 */
	if(b->size == 8)
	{
		footer[-2] = (uint32_t)previous_link;
		footer[-1] = (uint32_t)next_link | 1;
	}
	/* Larger block: store the size first then the link */
	else
	{
		footer[-3] = b->size;
		footer[-2] = (uint32_t)previous_link;
		footer[-1] = (uint32_t)next_link;
	}
}

/* Find a best fit for the requested size in the list */
static block_t *best_fit(block_t *list, size_t size)
{
	block_t *best_match = NULL;
	size_t best_size = 0xffffffff;

	while(list && best_size != size)
	{
		if(list->size >= size && list->size < best_size)
		{
			best_match = list;
			best_size = list->size;
		}
		list = next_link(list);
	}

	return best_match;
}

/* Find a worst fit in the list */
static block_t *worst_fit(block_t *list)
{
	block_t *best_match = NULL;
	size_t best_size = 0;

	while(list)
	{
		if(list->size >= best_size)
		{
			best_match = list;
			best_size = list->size;
		}
		list = next_link(list);
	}

	return best_match;
}

//---
// Index-level operations
//---

/* Returns the size class of the given size */
GINLINE static inline int size_class(size_t size)
{
	if(size < 64) return (size - 8) >> 2;
	if(size < 256) return 14;
	return 15;
}

/* Removes a block from a list, updating the index if needed. The free block is
   in a temporary state of being in no list, called "free-floating" */
static void remove_link(block_t *b, index_t *index)
{
	int c = size_class(b->size);

	block_t *prev = previous_link(b);
	block_t *next = next_link(b);

	/* Redirect links around (b) in its list */
	if(prev) set_footer(prev, previous_link(prev), next);
	if(next) set_footer(next, prev, next_link(next));

	if(index->classes[c] == b) index->classes[c] = next;

	if(index->stats) index->stats->free_memory -= b->size;
}

/* Prepends a block to the list for its size class, and update the index */
static void prepend_link(block_t *b, index_t *index)
{
	int c = size_class(b->size);

	block_t *first = index->classes[c];
	set_footer(b, NULL, first);
	if(first) set_footer(first, b, next_link(first));

	index->classes[c] = b;

	if(index->stats) index->stats->free_memory += b->size;
}

//---
// Arena allocator
//---

/* Round a size to the closest allocatable size */
static size_t round(size_t size)
{
	return (size < 8) ? 8 : ((size + 3) & ~3);
}

static void *gint_malloc(size_t size, void *data)
{
	index_t *index = data;
	stats_t *s = index->stats;
	size = round(size);
	int c = size_class(size);

	/* Try to find a class that has a free block available */
	block_t *alloc = NULL;
	for(; c <= 15; c++)
	{
		block_t *list = index->classes[c];
		/* The first 14 classes are exact-size, so there is no need to
		   search. For the last two, we use a best fit. */
		alloc = (c < 14) ? list : best_fit(list, size);
		if(alloc) break;
	}
	if(!alloc)
	{
		if(s && s->free_memory >= size) s->fragmentation_failures++;
		if(s && s->free_memory <  size) s->exhaustion_failures++;
		return NULL;
	}

	/* Remove the block to allocate from its list */
	remove_link(alloc, index);

	/* If it's larger than needed, split it and reinsert the leftover */
	block_t *rest = split(alloc, size);
	if(rest) prepend_link(rest, index);

	/* Mark the block as allocated and return it */
	block_t *next = next_block(alloc);
	alloc->used = true;
	if(next) next->previous_used = true;

	if(s) s->used_memory += alloc->size;
	if(s) s->peak_used_memory = max(s->peak_used_memory, s->used_memory);

	return (void *)alloc + sizeof(block_t);
}

static void gint_free(void *ptr, void *data)
{
	index_t *index = data;
	block_t *b = ptr - sizeof(block_t);

	block_t *prev = previous_block_if_free(b);
	block_t *next = next_block(b);

	/* Mark the block as free */
	b->used = false;
	if(index->stats) index->stats->used_memory -= b->size;
	if(next) next->previous_used = false;

	/* Merge with the next block if free */
	if(next && !next->used)
	{
		remove_link(next, index);
		merge(b, next);
	}
	/* Merge with the previous block if free */
	if(prev)
	{
		remove_link(prev, index);
		merge(prev, b);
		b = prev;
	}

	/* Insert the result in the index */
	prepend_link(b, index);
}

static void *gint_realloc(void *ptr, size_t size, void *data)
{
	index_t *index = data;
	stats_t *s = index->stats;
	block_t *b = ptr - sizeof(block_t);
	size = round(size);
	int size_before = b->size;

	/* When requesting a smaller size, split the original block */
	if(size <= b->size)
	{
		block_t *rest = split(b, size);
		if(!rest) return ptr;

		/* Try to merge the rest with a following free block */
		block_t *next = next_block(rest);
		if(next && !next->used)
		{
			remove_link(next, index);
			merge(rest, next);
		}
		prepend_link(rest, index);

		if(s) s->used_memory -= (size_before - size);
		return ptr;
	}

	/* When requesting a larger size and the next block is free and large
	   enough, expand the original allocation */
	block_t *next = next_block(b);
	int next_needed = size - b->size - sizeof(block_t);

	if(next && !next->used && next->size >= next_needed)
	{
		remove_link(next, index);
		block_t *rest = split(next, next_needed);
		if(rest) prepend_link(rest, index);
		merge(b, next);

		if(s) s->used_memory += (b->size - size_before);
		if(s) s->expanding_reallocs++;
		return ptr;
	}

	/* Otherwise, perform a brand new allocation */
	void *new_ptr = gint_malloc(size, data);
	if(!new_ptr)
	{
		if(s && size >= s->free_memory) s->exhaustion_failures++;
		if(s && size <  s->free_memory) s->fragmentation_failures++;
		return NULL;
	}

	/* Move the data and free the original block */
	memcpy(new_ptr, ptr, b->size);
	gint_free(ptr, data);

	if(s) s->relocating_reallocs++;
	return new_ptr;
}

static void *gint_malloc_max(size_t *size, void *data)
{
	index_t *index = data;
	stats_t *s = index->stats;

	/* Find the largest available block in the largest possible class */
	block_t *alloc;
	for(int c = 15; c >= 0; c--)
	{
		block_t *list = index->classes[c];
		alloc = (c < 14) ? list : worst_fit(list);
		if(alloc) break;
	}
	if(!alloc) return NULL;

	remove_link(alloc, index);

	/* Mark the block as allocated and return it */
	block_t *next = next_block(alloc);
	alloc->used = true;
	if(next) next->previous_used = true;

	if(s) s->used_memory += alloc->size;
	if(s) s->peak_used_memory = max(s->peak_used_memory, s->used_memory);

	*size = alloc->size;
	return (void *)alloc + sizeof(block_t);
}

/* kmalloc_init_arena(): Initialize an arena with gint's allocator */
void kmalloc_init_arena(kmalloc_arena_t *a, bool enable_statistics)
{
	if(a->end - a->start < 256) return;
	block_t *entry_block;

	a->malloc = gint_malloc;
	a->free = gint_free;
	a->realloc = gint_realloc;
	a->malloc_max = gint_malloc_max;

	/* The index is located at the very start of the arena */
	index_t *index = a->start;
	a->data = index;

	/* If requested, keep space for statistics */
	if(enable_statistics)
	{
		index->stats = (void *)a->start + sizeof(index_t);
		entry_block = (void *)index->stats + sizeof(stats_t);

		/* Manual 4-byte memset to allow PRAM0 arena */
		for(uint i = 0; i < sizeof(stats_t) / 4; i++)
			((uint32_t *)index->stats)[i] = 0;
		_Static_assert((sizeof(stats_t) & 3) == 0);
	}
	else
	{
		index->stats = NULL;
		entry_block = (void *)a->start + sizeof(index_t);
	}

	/* Initialize the first block */
	entry_block->last = 1;
	entry_block->used = 0;
	entry_block->previous_used = 1;
	entry_block->size = a->end - (void *)entry_block - sizeof(block_t);
	set_footer(entry_block, NULL, NULL);

	/* Initialize the index */
	for(int i = 0; i < 16; i++) index->classes[i] = NULL;
	index->classes[size_class(entry_block->size)] = entry_block;

	/* Initialize statistics */
	if(index->stats) index->stats->free_memory = entry_block->size;
}

//---
// Introspection and debugging
//---

stats_t *kmalloc_get_gint_stats(kmalloc_arena_t *arena)
{
	if(arena->malloc != gint_malloc) return NULL;
	index_t *index = arena->data;
	return index->stats;
}

#ifdef GINT_KMALLOC_DEBUG

/* Tests for the structural integrity of the block sequence */

static block_t *first_block(kmalloc_arena_t *a)
{
	index_t *index = a->data;
	void *sequence = (void *)a->data + sizeof(index_t);
	if(index->stats) sequence += sizeof(stats_t);
	return sequence;
}

int kmallocdbg_sequence_length(kmalloc_arena_t *a)
{
	block_t *b = first_block(a);
	int length = 0;
	while(b) b = next_block(b), length++;
	return length;
}

bool kmallocdbg_sequence_covers(kmalloc_arena_t *a)
{
	index_t *index = a->data;

	void *sequence = (void *)a->data + sizeof(index_t);
	if(index->stats) sequence += sizeof(stats_t);

	block_t *b = sequence;
	int total_size = 0;

	while((void *)b >= a->start && (void *)b < a->end)
	{
		total_size += sizeof(block_t) + b->size;
		b = next_block(b);
	}

	return (total_size == a->end - sequence);
}

bool kmallocdbg_sequence_terminator(kmalloc_arena_t *a)
{
	block_t *b = first_block(a);
	while(!b->last) b = next_block(b);
	return ((void *)b + sizeof(block_t) + b->size == a->end);
}

bool kmallocdbg_sequence_coherent_used(kmalloc_arena_t *a)
{
	block_t *b = first_block(a), *next;
	if(!b->previous_used) return false;

	while(b)
	{
		next = next_block(b);
		if(next && b->used != next->previous_used) return false;
		b = next;
	}
	return true;
}

bool kmallocdbg_sequence_footer_size(kmalloc_arena_t *a)
{
	for(block_t *b = first_block(a); b; b = next_block(b))
	{
		if(b->used) continue;
		uint32_t *footer = (void *)b + sizeof(block_t) + b->size;

		if((footer[-1] & 1) != (b->size == 8)) return false;
		if(b->size != 8 && (b->size != footer[-3])) return false;
	}
	return true;
}

bool kmallocdbg_sequence_merged_free(kmalloc_arena_t *a)
{
	for(block_t *b = first_block(a); b; b = next_block(b))
	{
		if(b->used) continue;
		if(previous_block_if_free(b)) return false;
		if(next_block(b) && !next_block(b)->used) return false;
	}
	return true;
}

/* Tests for the integrity of the doubly-linked lists */

bool kmallocdbg_list_structure(kmalloc_arena_t *a)
{
	index_t *index = a->data;

	for(int c = 0; c < 16; c++)
	{
		block_t *b = index->classes[c], *next;
		if(!b) continue;
		if(b->used) return false;
		if(previous_link(b)) return false;

		while((next = next_link(b)))
		{
			if(previous_link(next) != b) return false;
			b = next;
		}
	}
	return true;
}

/* Tests for the coverage and separation of the segregated lists */

bool kmallocdbg_index_covers(kmalloc_arena_t *a)
{
	index_t *index = a->data;
	int32_t total_size = 0;

	for(block_t *b = first_block(a); b; b = next_block(b))
	{
		if(b->used) total_size += sizeof(block_t) + b->size;
	}

	for(int c = 0; c < 16; c++)
	for(block_t *b = index->classes[c]; b; b = next_link(b))
	{
		total_size += sizeof(block_t) + b->size;
	}

	return (total_size == a->end - (void *)first_block(a));
}

bool kmallocdbg_index_class_separation(kmalloc_arena_t *a)
{
	index_t *index = a->data;

	for(int c = 0; c < 16; c++)
	for(block_t *b = index->classes[c]; b; b = next_link(b))
	{
		if(size_class(b->size) != c) return false;
	}
	return true;
}

#endif /* GINT_KMALLOC_DEBUG */
