#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "allocator/arena.h"

/**
 * @brief Minimum block size used by the arena.
 */
#define ARENA_MIN_BLOCK 4096

/**
 * @brief Helper to align sizes to pointer width.
 */
typedef struct leuko_arena arena_t;

/**
 * @brief Internal block structure.
 */
struct arena_block
{
    char *data;
    size_t size;
    size_t used;
    struct arena_block *next;
};

/**
 * @brief Arena structure.
 */
struct leuko_arena
{
    struct arena_block *blocks;
    size_t default_block_size;
};

/**
 * @brief Helper to align sizes to pointer width.
 * @param n Size to align
 * @return Aligned size
 */
static size_t align_up(size_t n)
{
    size_t a = sizeof(void *);
    return (n + a - 1) & ~(a - 1);
}

/**
 * @brief Create a new arena block of given size.
 * @param size Size of the block
 * @return Pointer to the new block, or NULL on failure
 */
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

/**
 * @brief Free all blocks in a linked list.
 * @param b Pointer to the first block
 */
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

/**
 * @brief Create a new arena with an initial size.
 * @param initial_size Initial size of the arena
 * @return Pointer to the new arena, or NULL on failure
 */
struct leuko_arena *leuko_arena_new(size_t initial_size)
{
    struct leuko_arena *a = malloc(sizeof(*a));
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

/**
 * @brief Allocate memory from the arena.
 * @param a Pointer to the arena
 * @param size Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *leuko_arena_alloc(struct leuko_arena *a, size_t size)
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

/**
 * @brief Duplicate a string into the arena.
 * @param a Pointer to the arena
 * @param s String to duplicate
 * @return Pointer to the duplicated string, or NULL on failure
 */
char *leuko_arena_strdup(struct leuko_arena *a, const char *s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s) + 1;
    char *p = leuko_arena_alloc(a, n);
    if (!p)
        return NULL;
    memcpy(p, s, n);
    return p;
}

/**
 * @brief Reset the arena, freeing all allocations.
 * @param a Pointer to the arena
 */
void leuko_arena_reset(struct leuko_arena *a)
{
    if (!a)
        return;
    block_free_all(a->blocks);
    a->blocks = block_new(a->default_block_size);
}

/**
 * @brief Check if a pointer belongs to the arena.
 * @param a Pointer to the arena
 * @param ptr Pointer to check
 * @return 1 if the pointer is in the arena, 0 otherwise
 */
int leuko_arena_contains(struct leuko_arena *a, void *ptr)
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

/**
 * @brief Free the arena and all its allocations.
 * @param a Pointer to the arena
 */
void leuko_arena_free(struct leuko_arena *a)
{
    if (!a)
        return;
    block_free_all(a->blocks);
    free(a);
}
