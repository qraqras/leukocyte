/*
 * Lightweight arena (region) allocator for temporary allocations.
 */
#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

struct arena;

/* Create a new arena. initial_size may be 0 to use a default. */
struct arena *arena_new(size_t initial_size);

/* Allocate `size` bytes from the arena. */
void *arena_alloc(struct arena *a, size_t size);

/* Duplicate a NUL-terminated string into the arena and return pointer. */
char *arena_strdup(struct arena *a, const char *s);

/* Reset the arena, freeing all allocated blocks and reinitializing. */
void arena_reset(struct arena *a);

/* Free the arena and all its memory. */
void arena_free(struct arena *a);

/* Return non-zero if ptr is owned by this arena (ptr is a user pointer). */
int arena_contains(struct arena *a, void *ptr);

#endif /* ARENA_H */
