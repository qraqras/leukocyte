#define _GNU_SOURCE
#include "leuko_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

/* Simple arena (region) allocator intended for short-lived parser allocations.
 * Controlled by environment variable LEUKO_ARENA=1 when compiled as PRISM_XALLOCATOR.
 * Behavior:
 *  - begin_parse() resets the arena state
 *  - allocations <= ARENA_MAX_INLINE are served from arena
 *  - realloc of arena pointer allocates a new block and copies
 *  - free of arena pointer is a no-op (memory freed on end_parse)
 */

#define ARENA_CHUNK_DEFAULT (1 << 16) /* 64KB */
#define ARENA_SMALL_LIMIT 4096

typedef struct arena_chunk {
    char *data;
    size_t cap;
    size_t used;
    struct arena_chunk *next;
} arena_chunk_t;

/* Per-thread arena to be safe for parallel runs */
static __thread arena_chunk_t *arena_head = NULL;
static __thread int arena_enabled = -1; /* -1=uninitialized, 0=disabled, 1=enabled */
static pthread_mutex_t arena_lock = PTHREAD_MUTEX_INITIALIZER;

static int is_arena_enabled(void) {
    if (arena_enabled != -1) return arena_enabled;
    const char *env = getenv("LEUKO_ARENA");
    arena_enabled = (env && env[0] == '1') ? 1 : 0;
    return arena_enabled;
}

static int is_arena_log_enabled(void) {
    const char *env = getenv("LEUKO_ARENA_LOG");
    return (env && env[0] == '1');
}

void leuko_allocator_begin_parse(void) {
    /* free any leftover chunks from previous parse */
    if (!is_arena_enabled()) return;
    if (is_arena_log_enabled()) fprintf(stderr, "[arena] begin_parse\n");
    pthread_mutex_lock(&arena_lock);
    arena_chunk_t *c = arena_head;
    while (c) {
        arena_chunk_t *next = c->next;
        free(c->data);
        free(c);
        c = next;
    }
    arena_head = NULL;
    pthread_mutex_unlock(&arena_lock);
}

void leuko_allocator_end_parse(void) {
    /* same as begin: free all chunks */
    if (!is_arena_enabled()) return;
    pthread_mutex_lock(&arena_lock);
    arena_chunk_t *c = arena_head;
    while (c) {
        arena_chunk_t *next = c->next;
        free(c->data);
        free(c);
        c = next;
    }
    arena_head = NULL;
    pthread_mutex_unlock(&arena_lock);
}

static size_t align_up(size_t v, size_t a) { return (v + (a - 1)) & ~(a - 1); }

static void *arena_alloc(size_t size) {
    if (!is_arena_enabled()) return malloc(size);
    if (size > ARENA_SMALL_LIMIT) {
        /* large allocations go direct to malloc to avoid wasting arena */
        return malloc(size);
    }

    size_t hdr = sizeof(mp_hdr_t);
    /* ensure reasonable alignment */
    size_t align = sizeof(void *);
    size_t total = align_up(hdr + size, align);

    pthread_mutex_lock(&arena_lock);
    arena_chunk_t *c = arena_head;
    if (!c || (c->cap - c->used) < total) {
        size_t newcap = ARENA_CHUNK_DEFAULT;
        if (newcap < total) newcap = total * 2;
        arena_chunk_t *nc = (arena_chunk_t *) malloc(sizeof(arena_chunk_t));
        if (!nc) { pthread_mutex_unlock(&arena_lock); return NULL; }
        nc->data = (char *) malloc(newcap);
        if (!nc->data) { free(nc); pthread_mutex_unlock(&arena_lock); return NULL; }
        nc->cap = newcap;
        nc->used = 0;
        nc->next = arena_head;
        arena_head = nc;
        c = nc;
    }

    char *p = c->data + c->used;
    /* write header */
    mp_hdr_t *h = (mp_hdr_t *) p;
    h->magic = ARENA_MAGIC;
    h->size = size;

    void *user = p + hdr;
    c->used += total;
    pthread_mutex_unlock(&arena_lock);
    return user;
}

/* header used to mark arena allocations (must match malloc_profiler) */
#include <stdint.h>

typedef struct mp_hdr {
    uint64_t magic;
    size_t size;
} mp_hdr_t;

#define ARENA_MAGIC 0x4152454E414D4741ULL /* "ARENAGMA" */

/* helpers to test whether ptr lies within arena (ptr is user pointer) */
static int ptr_in_arena(void *ptr) {
    if (!ptr) return 0;
    arena_chunk_t *c = arena_head;
    size_t hdr = sizeof(mp_hdr_t);
    while (c) {
        /* header is stored immediately before the user pointer */
        char *header_candidate = (char *)ptr - hdr;
        if (header_candidate >= c->data && header_candidate < c->data + c->used) return 1;
        c = c->next;
    }
    return 0;
}

/* Exported functions used by Prism via prism_xallocator.h */
void *leuko_xmalloc(size_t size) {
    if (!is_arena_enabled()) return malloc(size);
    void *p = arena_alloc(size);
    return p ? p : malloc(size);
}

void *leuko_xcalloc(size_t nmemb, size_t size) {
    size_t total = nmemb * size;
    if (!is_arena_enabled()) {
        void *p = calloc(nmemb, size);
        return p;
    }
    void *p = arena_alloc(total);
    if (p) { memset(p, 0, total); return p; }
    return calloc(nmemb, size);
}

void *leuko_xrealloc(void *ptr, size_t size) {
    if (!ptr) return leuko_xmalloc(size);
    if (!is_arena_enabled()) return realloc(ptr, size);

    if (ptr_in_arena(ptr)) {
        /* can't resize in-place: allocate new area and copy the original size */
        mp_hdr_t *oh = (mp_hdr_t *)((char *)ptr - sizeof(mp_hdr_t));
        size_t old_size = 0;
        if (oh->magic == ARENA_MAGIC) old_size = oh->size;

        void *n = arena_alloc(size);
        if (!n) return NULL;
        /* copy min(old_size, size) to avoid over-read */
        size_t to_copy = (old_size && old_size < size) ? old_size : size;
        memcpy(n, ptr, to_copy);
        return n;
    }
    return realloc(ptr, size);
}

void leuko_xfree(void *ptr) {
    if (!ptr) return;
    if (!is_arena_enabled()) { free(ptr); return; }
    if (ptr_in_arena(ptr)) {
        /* no-op: memory will be freed at end_parse */
        return;
    }
    free(ptr);
}