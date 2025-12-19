#ifndef LEUKOCYTE_ARENA_H
#define LEUKOCYTE_ARENA_H

#include <stddef.h>

struct leuko_arena;

struct leuko_arena *leuko_arena_new(size_t initial_size);
void *leuko_arena_alloc(struct leuko_arena *a, size_t size);
char *leuko_arena_strdup(struct leuko_arena *a, const char *s);
void leuko_arena_reset(struct leuko_arena *a);
void leuko_arena_free(struct leuko_arena *a);
int leuko_arena_contains(struct leuko_arena *a, void *ptr);

#endif /* LEUKOCYTE_ARENA_H */
