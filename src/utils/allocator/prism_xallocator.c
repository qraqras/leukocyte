#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>
#include "utils/allocator/arena.h"
#include "utils/allocator/prism_xallocator.h"

/**
 * Prism allocator (x* API) implementation.
 * - Public API: xmalloc/xcalloc/xrealloc/xfree.
 * - Uses per-thread arenas (leuko_arena_head) for small allocations
 *   (<= LEUKO_ARENA_SMALL_LIMIT); larger allocations fall back to malloc.
 * - Arena chunks default to LEUKO_ARENA_CHUNK_DEFAULT and are created lazily.
 * - Arena lifecycle: leuko_x_allocator_begin() / leuko_x_allocator_end().
 * - Internal helpers and symbols are prefixed with `leuko_`.
 */

/**
 * @brief Default arena chunk size.
 * @note 4MB
 */
#define LEUKO_ARENA_CHUNK_DEFAULT (4 << 20)

/**
 * @brief Small allocation limit for arena usage.
 * @note 8KB
 */
#define LEUKO_ARENA_SMALL_LIMIT 8192

/**
 * @brief Magic number to identify arena allocations (arena block header).
 */
#define LEUKO_ARENA_BLOCK_MAGIC 0x4152454E414D4741ULL

/**
 * @brief Thread-local arena head for the current thread (lazily created).
 */
static __thread struct leuko_arena *leuko_arena_head = NULL;

/**
 * @brief Header used to mark arena allocations.
 */
typedef struct leuko_arena_block_hdr
{
    uint64_t magic;
    size_t user_size;
} leuko_arena_block_hdr_t;

/**
 * @brief Begin allocator usage for the current thread.
 */
void leuko_x_allocator_begin(void)
{
    if (leuko_arena_head)
    {
        leuko_arena_free(leuko_arena_head);
        leuko_arena_head = NULL;
    }
}

/**
 * @brief End allocator usage for the current thread.
 */
void leuko_x_allocator_end(void)
{
    if (leuko_arena_head)
    {
        leuko_arena_free(leuko_arena_head);
        leuko_arena_head = NULL;
    }
}

/**
 * @brief Helper to align sizes to pointer width.
 * @param v Size to align
 * @param a Alignment
 * @return Aligned size
 */
static size_t leuko_align_up(size_t v, size_t a)
{
    return (v + (a - 1)) & ~(a - 1);
}

/**
 * @brief Allocate memory from the arena or system allocator.
 * @param size Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL on failure
 */
static void *leuko_arena_alloc_wrapper(size_t size)
{
    size_t hdr = sizeof(leuko_arena_block_hdr_t);
    size_t align = sizeof(void *);
    size_t total = leuko_align_up(hdr + size, align);
    /* Create per-thread arena lazily without global locking */
    if (!leuko_arena_head)
    {
        size_t chunk = LEUKO_ARENA_CHUNK_DEFAULT;
        leuko_arena_head = leuko_arena_new(chunk);
        if (!leuko_arena_head)
            return NULL;
    }

    size_t small_limit = LEUKO_ARENA_SMALL_LIMIT;
    if (size > small_limit)
        return malloc(size);

    void *p = leuko_arena_alloc(leuko_arena_head, total);
    if (!p)
        return NULL;

    leuko_arena_block_hdr_t *h = (leuko_arena_block_hdr_t *)p;
    h->magic = LEUKO_ARENA_BLOCK_MAGIC;
    h->user_size = size;
    void *user = (char *)p + hdr;
    return user;
}

/**
 * @brief Check if a pointer was allocated from the arena.
 * @param ptr Pointer to check
 * @return 1 if the pointer is in the arena, 0 otherwise
 */
static int leuko_ptr_in_arena(void *ptr)
{
    if (!ptr)
        return 0;
    if (!leuko_arena_head)
        return 0;
    /* Compute header pointer and verify it lies inside the arena. If it does,
       check the magic field to ensure the pointer was actually allocated by
       the arena. This prevents false positives where a heap pointer minus the
       header size coincidentally falls inside the arena memory. */
    char *hdr = (char *)ptr - sizeof(leuko_arena_block_hdr_t);
    if (!leuko_arena_contains(leuko_arena_head, hdr))
    {
        return 0;
    }
    leuko_arena_block_hdr_t *h = (leuko_arena_block_hdr_t *)hdr;
    return h->magic == LEUKO_ARENA_BLOCK_MAGIC;
}

/**
 * @brief Allocate memory using the Prism allocator.
 * @param size Size of memory to allocate
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *prism_alloc_impl(size_t size)
{
    void *p = leuko_arena_alloc_wrapper(size);
    if (p)
        return p;
    return malloc(size);
}

/**
 * @brief Allocate zero-initialized memory using the Prism allocator.
 * @param nmemb Number of members
 * @param size Size of each member
 * @return Pointer to the allocated memory, or NULL on failure
 */
void *prism_calloc_impl(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *p = leuko_arena_alloc_wrapper(total);
    if (p)
    {
        memset(p, 0, total);
        return p;
    }
    return calloc(nmemb, size);
}

/**
 * @brief Reallocate memory using the Prism allocator.
 * @param ptr Pointer to existing memory
 * @param size New size of memory
 * @return Pointer to the reallocated memory, or NULL on failure
 */
void *prism_realloc_impl(void *ptr, size_t size)
{
    if (!ptr)
        return prism_alloc_impl(size);
    if (leuko_ptr_in_arena(ptr))
    {
        leuko_arena_block_hdr_t *oh = (leuko_arena_block_hdr_t *)((char *)ptr - sizeof(leuko_arena_block_hdr_t));
        size_t old_size = 0;
        if (oh->magic == LEUKO_ARENA_BLOCK_MAGIC)
            old_size = oh->user_size;
        void *n = leuko_arena_alloc_wrapper(size);
        if (!n)
            return NULL;
        size_t to_copy = (old_size && old_size < size) ? old_size : size;
        memcpy(n, ptr, to_copy);
        return n;
    }
    return realloc(ptr, size);
}

/**
 * @brief Free memory using the Prism allocator.
 * @param ptr Pointer to memory to free
 */
void prism_free_impl(void *ptr)
{
    if (!ptr)
        return;
    if (leuko_ptr_in_arena(ptr))
        return; /* no-op */
    free(ptr);
}

/**
 * @brief Aliases for Prism allocator functions.
 */
void *xmalloc(size_t size) { return prism_alloc_impl(size); }

/**
 * @brief Aliases for Prism allocator functions.
 */
void *xcalloc(size_t nmemb, size_t size) { return prism_calloc_impl(nmemb, size); }

/**
 * @brief Aliases for Prism allocator functions.
 */
void *xrealloc(void *ptr, size_t size) { return prism_realloc_impl(ptr, size); }

/**
 * @brief Aliases for Prism allocator functions.
 */
void xfree(void *ptr) { prism_free_impl(ptr); }
