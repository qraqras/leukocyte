/*
 * Simple arena allocator PoC.
 */
#include "allocator/arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Minimum block size used by the arena. */
#define ARENA_MIN_BLOCK 4096

/* Internal block structure */
struct arena_block
{
    char *data;
    size_t size;
    size_t used;
    struct arena_block *next;
};

struct arena
{
    struct arena_block *blocks;
    size_t default_block_size;
};

/* Helper to align sizes to pointer width */
static size_t align_up(size_t n)
{
    size_t a = sizeof(void *);
    return (n + a - 1) & ~(a - 1);
}

static struct arena_block *block_new(size_t size)
{
    struct arena_block *b = malloc(sizeof(*b));
    if (!b)
        return NULL;
    b->data = malloc(size);
    if (!b->data)
    {
        free(b);
        return NULL;
    }
    b->size = size;
    b->used = 0;
    b->next = NULL;
    return b;
}

static void block_free_all(struct arena_block *b)
{
    while (b)
    {
        struct arena_block *next = b->next;
        free(b->data);
        free(b);
        b = next;
    }
}

struct arena *arena_new(size_t initial_size)
{
    struct arena *a = malloc(sizeof(*a));
    if (!a)
        return NULL;
    size_t bs = initial_size ? initial_size : ARENA_MIN_BLOCK;
    if (bs < ARENA_MIN_BLOCK)
        bs = ARENA_MIN_BLOCK;
    a->default_block_size = bs;
    a->blocks = block_new(a->default_block_size);
    if (!a->blocks)
    {
        free(a);
        return NULL;
    }
    return a;
}

void *arena_alloc(struct arena *a, size_t size)
{
    if (!a || size == 0)
        return NULL;
    size = align_up(size);
    struct arena_block *b = a->blocks;
    /* fast path: current block has space */
    if (b && (b->size - b->used) >= size)
    {
        void *p = b->data + b->used;
        b->used += size;
        return p;
    }

    /* If request is large, allocate a dedicated block */
    if (size > a->default_block_size / 2)
    {
        struct arena_block *nb = block_new(size);
        if (!nb)
            return NULL;
        nb->used = size;
        /* link at head to keep recent blocks quick */
        nb->next = a->blocks;
        a->blocks = nb;
        return nb->data;
    }

    /* allocate a new regular block */
    size_t ns = a->default_block_size;
    struct arena_block *nb = block_new(ns);
    if (!nb)
        return NULL;
    nb->used = size;
    /* link and return */
    nb->next = a->blocks;
    a->blocks = nb;
    return nb->data;
}

char *arena_strdup(struct arena *a, const char *s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s) + 1;
    char *p = arena_alloc(a, n);
    if (!p)
        return NULL;
    memcpy(p, s, n);
    return p;
}

void arena_reset(struct arena *a)
{
    if (!a)
        return;
    block_free_all(a->blocks);
    a->blocks = block_new(a->default_block_size);
}

int arena_contains(struct arena *a, void *ptr)
{
    if (!a || !ptr)
        return 0;
    size_t hdr = sizeof(void *); /* pointer-aligned offset isn't needed here */
    struct arena_block *b = a->blocks;
    while (b)
    {
        char *start = b->data;
        char *end = b->data + b->used;
        if ((char *)ptr >= start && (char *)ptr < end)
            return 1;
        b = b->next;
    }
    return 0;
}

void arena_free(struct arena *a)
{
    if (!a)
        return;
    block_free_all(a->blocks);
    free(a);
}
