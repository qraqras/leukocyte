/* Tests for the arena allocator PoC */
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "allocator/arena.h"

int main(void)
{
    struct leuko_arena *a = leuko_arena_new(1024);
    assert(a != NULL);

    char *s = leuko_arena_strdup(a, "hello");
    assert(s && strcmp(s, "hello") == 0);

    void *p = leuko_arena_alloc(a, 100);
    assert(p != NULL);
    memset(p, 0x5a, 100);

    char *s2 = leuko_arena_strdup(a, "world");
    assert(s2 && strcmp(s2, "world") == 0);

    /* large allocation that should create a dedicated block */
    size_t big = 5000;
    void *pb = leuko_arena_alloc(a, big);
    assert(pb != NULL);

    leuko_arena_free(a);
    return 0;
}
