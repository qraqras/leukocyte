#define _GNU_SOURCE
#include "prism_xallocator.h"
#include "allocator/arena.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdint.h>

/* Unified implementation of the Prism allocator (x* API).
 * All legacy leuko_x* aliases were removed during migration; code should
 * call the x* API directly (e.g., xmalloc/xfree) defined in
 * include/allocator/prism_xallocator.h.
 */

#define ARENA_CHUNK_DEFAULT (1 << 20) /* 1MB */
#define ARENA_SMALL_LIMIT 16384       /* 16KB */

/* Environment-overridable parameters:
 * - LEUKO_ARENA_CHUNK: chunk size in bytes (default: ARENA_CHUNK_DEFAULT)
 * - LEUKO_ARENA_SMALL_LIMIT: maximum size served from arena (default: ARENA_SMALL_LIMIT)
 */
static size_t get_arena_chunk(void)
{
    static size_t chunk = 0;
    if (chunk)
        return chunk;
    const char *s = getenv("LEUKO_ARENA_CHUNK");
    if (s)
    {
        unsigned long long v = strtoull(s, NULL, 0);
        if (v > 0)
        {
            chunk = (size_t)v;
        }
    }
    if (!chunk)
        chunk = ARENA_CHUNK_DEFAULT;
    return chunk;
}

static size_t get_arena_small_limit(void)
{
    static size_t lim = 0;
    if (lim)
        return lim;
    const char *s = getenv("LEUKO_ARENA_SMALL_LIMIT");
    if (s)
    {
        unsigned long long v = strtoull(s, NULL, 0);
        if (v > 0)
        {
            lim = (size_t)v;
        }
    }
    if (!lim)
        lim = ARENA_SMALL_LIMIT;
    return lim;
}

static __thread struct arena *arena_head = NULL;
static __thread int arena_enabled = 1;
static pthread_mutex_t arena_lock = PTHREAD_MUTEX_INITIALIZER;

static size_t sys_malloc_calls = 0;
static size_t sys_calloc_calls = 0;
static size_t sys_realloc_calls = 0;
static size_t sys_free_calls = 0;

static int is_arena_enabled(void)
{
    (void)arena_enabled;
    return 1;
}
static int is_arena_log_enabled(void)
{
    const char *env = getenv("LEUKO_ARENA_LOG");
    return (env && env[0] == '1');
}

void x_allocator_begin_parse(void)
{
    if (!is_arena_enabled())
        return;
    if (is_arena_log_enabled())
        fprintf(stderr, "[arena] begin_parse\n");
    pthread_mutex_lock(&arena_lock);
    if (arena_head)
    {
        arena_free(arena_head);
        arena_head = NULL;
    }
    pthread_mutex_unlock(&arena_lock);
}

void x_allocator_end_parse(void)
{
    if (!is_arena_enabled())
        return;
    pthread_mutex_lock(&arena_lock);
    if (arena_head)
    {
        arena_free(arena_head);
        arena_head = NULL;
    }
    fprintf(stderr, "[x_alloc_stats] sys_alloc_calls=%zu sys_calloc_calls=%zu sys_realloc_calls=%zu sys_free_calls=%zu\n",
            sys_malloc_calls, sys_calloc_calls, sys_realloc_calls, sys_free_calls);
    sys_malloc_calls = sys_calloc_calls = sys_realloc_calls = sys_free_calls = 0;
    pthread_mutex_unlock(&arena_lock);
}

static size_t align_up(size_t v, size_t a) { return (v + (a - 1)) & ~(a - 1); }

typedef struct mp_hdr
{
    uint64_t magic;
    size_t size;
} mp_hdr_t;
#define ARENA_MAGIC 0x4152454E414D4741ULL /* "ARENAGMA" */

static void *arena_alloc_wrapper(size_t size)
{
    if (!is_arena_enabled())
        return malloc(size);
    size_t hdr = sizeof(mp_hdr_t);
    size_t align = sizeof(void *);
    size_t total = align_up(hdr + size, align);
    pthread_mutex_lock(&arena_lock);
    if (!arena_head)
    {
        size_t chunk = get_arena_chunk();
        if (is_arena_log_enabled())
            fprintf(stderr, "[arena] creating head chunk=%zu\n", chunk);
        arena_head = arena_new(chunk);
        if (!arena_head)
        {
            pthread_mutex_unlock(&arena_lock);
            return NULL;
        }
    }
    size_t small_limit = get_arena_small_limit();
    if (size > small_limit)
    {
        pthread_mutex_unlock(&arena_lock);
        return malloc(size);
    }
    void *p = arena_alloc(arena_head, total);
    if (!p)
    {
        pthread_mutex_unlock(&arena_lock);
        return NULL;
    }
    mp_hdr_t *h = (mp_hdr_t *)p;
    h->magic = ARENA_MAGIC;
    h->size = size;
    void *user = (char *)p + hdr;
    pthread_mutex_unlock(&arena_lock);
    return user;
}

static int ptr_in_arena(void *ptr)
{
    if (!ptr)
        return 0;
    if (!arena_head)
        return 0;
    /* Compute header pointer and verify it lies inside the arena. If it does,
       check the magic field to ensure the pointer was actually allocated by
       the arena. This prevents false positives where a heap pointer minus the
       header size coincidentally falls inside the arena memory. */
    char *hdr = (char *)ptr - sizeof(mp_hdr_t);
    if (!arena_contains(arena_head, hdr))
    {
        return 0;
    }
    mp_hdr_t *h = (mp_hdr_t *)hdr;
    return h->magic == ARENA_MAGIC;
}

/* Core implementations used by both symbol sets */
void *prism_alloc_impl(size_t size)
{
    if (!is_arena_enabled())
    {
        __sync_fetch_and_add(&sys_malloc_calls, 1);
        return malloc(size);
    }
    void *p = arena_alloc_wrapper(size);
    if (p)
        return p;
    __sync_fetch_and_add(&sys_malloc_calls, 1);
    return malloc(size);
}

void *prism_calloc_impl(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    if (!is_arena_enabled())
    {
        __sync_fetch_and_add(&sys_calloc_calls, 1);
        return calloc(nmemb, size);
    }
    void *p = arena_alloc_wrapper(total);
    if (p)
    {
        memset(p, 0, total);
        return p;
    }
    __sync_fetch_and_add(&sys_calloc_calls, 1);
    return calloc(nmemb, size);
}

void *prism_realloc_impl(void *ptr, size_t size)
{
    if (!ptr)
        return prism_alloc_impl(size);
    if (!is_arena_enabled())
    {
        __sync_fetch_and_add(&sys_realloc_calls, 1);
        return realloc(ptr, size);
    }
    if (ptr_in_arena(ptr))
    {
        mp_hdr_t *oh = (mp_hdr_t *)((char *)ptr - sizeof(mp_hdr_t));
        size_t old_size = 0;
        if (oh->magic == ARENA_MAGIC)
            old_size = oh->size;
        void *n = arena_alloc_wrapper(size);
        if (!n)
            return NULL;
        size_t to_copy = (old_size && old_size < size) ? old_size : size;
        memcpy(n, ptr, to_copy);
        return n;
    }
    __sync_fetch_and_add(&sys_realloc_calls, 1);
    return realloc(ptr, size);
}

void prism_free_impl(void *ptr)
{
    if (!ptr)
        return;
    if (!is_arena_enabled())
    {
        __sync_fetch_and_add(&sys_free_calls, 1);
        free(ptr);
        return;
    }
    if (ptr_in_arena(ptr))
        return; /* no-op */
    __sync_fetch_and_add(&sys_free_calls, 1);
    free(ptr);
}

/* Export the x* API (xmalloc/xcalloc/xrealloc/xfree).
 * Legacy leuko_* aliases have been removed; use x* functions directly.
 */
void *xmalloc(size_t size) { return prism_alloc_impl(size); }
void *xcalloc(size_t nmemb, size_t size) { return prism_calloc_impl(nmemb, size); }
void *xrealloc(void *ptr, size_t size) { return prism_realloc_impl(ptr, size); }
void xfree(void *ptr) { prism_free_impl(ptr); }
