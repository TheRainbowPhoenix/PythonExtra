//---
// gint:kmalloc - gint's memory allocator
//---

#ifndef GINT_KMALLOC
#define GINT_KMALLOC

#ifdef __cplusplus
extern "C" {
#endif

#include <gint/config.h>
#include <gint/defs/types.h>

//---
// Standard memory allocation API
//---

/* kmalloc(): Allocate memory in one of the available arenas
   This function acts like malloc(). The second parameter specifies which arena
   to allocate from; when NULL, all default arenas are considered.

   @size        Size of requested block
   @arena_name  Name of arena to allocate in (can be NULL)
   Returns address of allocated block, NULL on error. */
void *kmalloc(size_t size, char const *arena_name);

/* krealloc(): Reallocate memory
   This function acts like realloc(). It only tries to reallocate the block in
   the arena where it was previously allocated. Note that if NULL is returned,
   the user needs to have a copy of the original address or the memory will
   become unreachable.

   @ptr   Existing allocated block
   @size  New requested size for the block
   Returns address of reallocated block, NULL on error. */
void *krealloc(void *ptr, size_t size);

/* kfree(): Free memory allocated with kalloc() */
void kfree(void *ptr);

/* kmalloc_max(): Allocate the largest block available in an arena

   This function is like kmalloc(), but it find the largest block in the arena
   and returns it whole after setting its size in *size. This is useful if you
   need memory to manage with another allocator, or you don't yet know the size
   of the data to be generated. The block can later be shrunk with realloc().

   Currently only gint-managed arenas support this operation.

   @size        Will be set to size of returned block (if there is one)
   @arean_name  Name of arena to allocate in (*cannot* be NULL)
   Returns the address of the largest block available, NULL on error. */
void *kmalloc_max(size_t *size, char const *arena_name);

//---
// Extension API for new areas and statistics
//---

typedef struct {
	/* Functions managing the arena. The last argument is the [data]
	   attribute in this structure. */

	/* kmalloc() handles size == 0 */
	void * (*malloc)(size_t size, void *data);
	/* krealloc() handles ptr == NULL, as well as newsize == 0 */
	void * (*realloc)(void *ptr, size_t newsize, void *data);
	/* kfree() handles ptr == NULL*/
	void (*free)(void *ptr, void *data);
	/* kmalloc_max() backend */
	void * (*malloc_max)(size_t *size, void *data);

	/* Name, should be unique; gint reserves names starting with "_" */
	char const *name;
	/* Start and end of arena. This is used to find the proper arena to
	   free from in kfree(). This cannot be NULL except for the OS heap as
	   the exact addresses are unknown. */
	void *start, *end;
	/* Pointer to arena-provided data, passed to malloc() and free() */
	void *data;
	/* Whether to consider this arena when performing default allocations
	   (kmalloc() with arena_name == NULL) */
	int is_default;

	/* Statistics maintained by kmalloc() */
	struct kmalloc_stats {
		int live_blocks;
		int peak_live_blocks;
		int total_volume;
		int total_blocks;
		int total_failures;
	} stats;

} kmalloc_arena_t;

/* kmalloc_init_arena(): Initialize an arena with gint's allocator

   This function initializes an arena on the region located between (a->start)
   and (a->end) and initializes the data structures for gint's allocator. It
   only sets the malloc(), realloc(), free() and (data) attributes of the
   structure, everything else should be initialized manually. The arena must
   have at least 256 bytes. */
void kmalloc_init_arena(kmalloc_arena_t *a, bool enable_statistics);

/* kmalloc_add_arena(): Add a new arena to the heap source
   Adds a fully-initialized arena to the heap source. The priority of the new
   arena compared to other default arenas is not specified. Returns true on
   success, false if the maximum number of arenas has been reached. */
bool kmalloc_add_arena(kmalloc_arena_t *arena);

/* kmalloc_remove_arena(): Remove an arena from the heap source
   Removes an arena from the heap source. The arena should certainly be empty,
   although this function will not check that. */
void kmalloc_remove_arena(kmalloc_arena_t *arena);

//---
// Internal functions
//---

/* kmalloc_init(): Initialize the dynamic allocator */
void kmalloc_init(void);

/* kmalloc_get_arena(): Find an arena by name */
kmalloc_arena_t *kmalloc_get_arena(char const *name);

//---
// Introspection functions for arenas managed by gint's allocator
//---

/* kmalloc_gint_stats_t: Optional statistics for gint's allocator

   These statistics are tracked by gint's allocator if requested when the arena
   is created. They can't be added or removed after creation.

   The free memory and used memory are counted in data bytes. Because the
   allocator uses some data, the initial value of (free_memory) is less than
   the size of the region. Additionally, because each used or free block has a
   4-byte header, (free_memory + used_memory) is not a constant value, so don't
   expect to get exactly what these values indicate (also fragmentation). */
typedef struct {
	/* Free space, used space, peak used space over time */
	uint32_t free_memory;
	uint32_t used_memory;
	uint32_t peak_used_memory;
	/* Number of mallocs that failed because of not enough free space */
	int exhaustion_failures;
	/* Number of mallocs that failed because of fragmentation */
	int fragmentation_failures;
	/* Number of reallocs that successfully re-used the same location */
	int expanding_reallocs;
	/* Number of reallocs that moved the data around */
	int relocating_reallocs;
} kmalloc_gint_stats_t;

/* kmalloc_get_gint_stats(): Get gint statistics for an arena

   Returns a pointer to the arena's statistics; returns NULL if the arena is
   not managed by gint's allocator or statistics were not enabled. */
kmalloc_gint_stats_t *kmalloc_get_gint_stats(kmalloc_arena_t *arena);

#ifdef GINT_KMALLOC_DEBUG

/* Check that the sequence covers exactly the arena's region. */
bool kmallocdbg_sequence_covers(kmalloc_arena_t *a);

/* Check that the marker for the last block is correctly set and unique. */
bool kmallocdbg_sequence_terminator(kmalloc_arena_t *a);

/* Check that the values of (used) and (previous_used) are coherent. */
bool kmallocdbg_sequence_coherent_used(kmalloc_arena_t *a);

/* Check that the size in the footer is correct for all free blocks. */
bool kmallocdbg_sequence_footer_size(kmalloc_arena_t *a);

/* Check that all free blocks are surrounded by used blocks. */
bool kmallocdbg_sequence_merged_free(kmalloc_arena_t *a);

/* Check that the doubly-linked list is well-formed. */
bool kmallocdbg_list_structure(kmalloc_arena_t *a);

/* Check that used blocks and list cover exactly the arena's region. */
bool kmallocdbg_index_covers(kmalloc_arena_t *a);

/* Check that all blocs referenced in free lists are of the correct class. */
bool kmallocdbg_index_class_separation(kmalloc_arena_t *a);

/* Number of blocks in the sequence. */
int kmallocdbg_sequence_length(kmalloc_arena_t *a);

#endif /* GINT_KMALLOC_DEBUG */

#ifdef __cplusplus
}
#endif

#endif /* GINT_KMALLOC */
