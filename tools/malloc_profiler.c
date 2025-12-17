#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <execinfo.h> /* backtrace */
#include <stddef.h>

/* Simple header-based allocator profiler. Stores size before user pointer. */
static void *(*real_malloc_fn)(size_t) = NULL;
static void *(*real_realloc_fn)(void *, size_t) = NULL;
static void *(*real_calloc_fn)(size_t, size_t) = NULL;
static void (*real_free_fn)(void *) = NULL;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static size_t total_alloc_calls = 0;
static size_t total_free_calls = 0;
static size_t total_realloc_calls = 0;
static size_t total_calloc_calls = 0;
static unsigned long long total_bytes_allocated = 0ULL;
static unsigned long long total_bytes_freed = 0ULL;
static unsigned long long current_bytes = 0ULL;
static unsigned long long peak_bytes = 0ULL;

static void ensure_real_fns(void) {
    if (real_malloc_fn) return;
    real_malloc_fn = dlsym(RTLD_NEXT, "malloc");
    real_realloc_fn = dlsym(RTLD_NEXT, "realloc");
    real_calloc_fn = dlsym(RTLD_NEXT, "calloc");
    real_free_fn = dlsym(RTLD_NEXT, "free");
    if (!real_malloc_fn || !real_realloc_fn || !real_calloc_fn || !real_free_fn) {
        fprintf(stderr, "malloc_profiler: failed to resolve real allocation functions\n");
        _exit(1);
    }
}

static inline void record_alloc(size_t bytes) {
    pthread_mutex_lock(&mtx);
    total_bytes_allocated += bytes;
    current_bytes += bytes;
    if (current_bytes > peak_bytes) peak_bytes = current_bytes;
    pthread_mutex_unlock(&mtx);
}

static inline void record_free(size_t bytes) {
    pthread_mutex_lock(&mtx);
    total_bytes_freed += bytes;
    if (current_bytes >= bytes) current_bytes -= bytes; else current_bytes = 0;
    pthread_mutex_unlock(&mtx);
}

void *malloc(size_t size) {
    ensure_real_fns();
    size_t header = sizeof(size_t);
    void *p = real_malloc_fn(size + header);
    if (!p) return NULL;
    *((size_t *) p) = size;
    void *user = (void *)((char *)p + header);

    pthread_mutex_lock(&mtx);
    total_alloc_calls++;
    pthread_mutex_unlock(&mtx);

    record_alloc(size);
    return user;
}

void free(void *ptr) {
    ensure_real_fns();
    if (!ptr) return;
    size_t header = sizeof(size_t);
    void *real_ptr = (void *)((char *)ptr - header);
    size_t size = 0;

    /* try to read size safely (may point into non-heap header for arena) */
    memcpy(&size, real_ptr, sizeof(size_t));

    pthread_mutex_lock(&mtx);
    total_free_calls++;
    pthread_mutex_unlock(&mtx);

    /* Optional pointer logging to help debug invalid free */
    const char *env = getenv("MALLOC_PROF_LOG_FREE_PTRS");
    if (env && env[0] == '1') {
        fprintf(stderr, "[malloc_profiler] free ptr=%p real_ptr=%p size=%zu\n", ptr, real_ptr, size);

        /* detect arena magic header and print backtrace */
        size_t hdr = sizeof(uint64_t) + sizeof(size_t);
        void *hdr_ptr = (void *)((char *)ptr - hdr);
        uint64_t magic = 0ULL;
        /* guard read */
        memcpy(&magic, hdr_ptr, sizeof(uint64_t));
        if (magic == 0x4152454E414D4741ULL) {
            /* arena allocation being freed by free() */
            fprintf(stderr, "[malloc_profiler] detected ARENA_MAGIC at %p (hdr=%p)\n", ptr, hdr_ptr);

            /* capture backtrace */
            void *bt[32];
            int bt_n = backtrace(bt, 32);
            char **bt_syms = backtrace_symbols(bt, bt_n);
            if (bt_syms) {
                fprintf(stderr, "[malloc_profiler] backtrace of free(...):\n");
                for (int i = 0; i < bt_n; i++) fprintf(stderr, "  %s\n", bt_syms[i]);
                free(bt_syms);
            }

            /* do not call real free on arena pointer */
            return;
        }
    }

    record_free(size);
    real_free_fn(real_ptr);
}

void *realloc(void *ptr, size_t size) {
    ensure_real_fns();
    size_t header = sizeof(size_t);
    if (!ptr) {
        pthread_mutex_lock(&mtx); total_realloc_calls++; pthread_mutex_unlock(&mtx);
        void *p = real_malloc_fn(size + header);
        if (!p) return NULL;
        *((size_t *) p) = size;
        record_alloc(size);
        return (void *)((char *)p + header);
    }

    /* detect arena pointer (mp_hdr with ARENA_MAGIC) */
    size_t hdr_total = sizeof(uint64_t) + sizeof(size_t);
    void *hdr_ptr = (void *)((char *)ptr - hdr_total);
    uint64_t magic = 0ULL;
    memcpy(&magic, hdr_ptr, sizeof(uint64_t));
    if (magic == 0x4152454E414D4741ULL) {
        /* ptr points into arena; allocate new heap block and copy old size */
        mp_hdr_t oh;
        memcpy(&oh, hdr_ptr, sizeof(mp_hdr_t));
        size_t old_size = oh.size;

        pthread_mutex_lock(&mtx); total_realloc_calls++; pthread_mutex_unlock(&mtx);
        void *p = real_malloc_fn(size + header);
        if (!p) return NULL;
        *((size_t *) p) = size;
        size_t to_copy = (old_size && old_size < size) ? old_size : size;
        memcpy((char *)p + header, ptr, to_copy);
        record_alloc(size);
        return (void *)((char *)p + header);
    }

    void *real_ptr = (void *)((char *)ptr - header);
    size_t old_size = 0;
    memcpy(&old_size, real_ptr, sizeof(size_t));

    void *p = real_realloc_fn(real_ptr, size + header);
    if (!p) return NULL;
    *((size_t *) p) = size;

    pthread_mutex_lock(&mtx); total_realloc_calls++; pthread_mutex_unlock(&mtx);

    if (size > old_size) {
        record_alloc(size - old_size);
    } else if (old_size > size) {
        record_free(old_size - size);
    }

    return (void *)((char *)p + header);
}

void *calloc(size_t nmemb, size_t size) {
    ensure_real_fns();
    size_t total = nmemb * size;
    size_t header = sizeof(size_t);
    void *p = real_malloc_fn(total + header);
    if (!p) return NULL;
    memset(p, 0, total + header);
    *((size_t *) p) = total;

    pthread_mutex_lock(&mtx); total_calloc_calls++; pthread_mutex_unlock(&mtx);

    record_alloc(total);
    return (void *)((char *)p + header);
}

static void dump_stats(void) {
    fprintf(stderr, "\n[malloc_profiler] total_alloc_calls=%zu total_calloc_calls=%zu total_realloc_calls=%zu total_free_calls=%zu\n",
            total_alloc_calls, total_calloc_calls, total_realloc_calls, total_free_calls);
    fprintf(stderr, "[malloc_profiler] total_bytes_allocated=%llu total_bytes_freed=%llu peak_bytes=%llu current_bytes=%llu\n",
            total_bytes_allocated, total_bytes_freed, peak_bytes, current_bytes);
}

__attribute__((constructor)) static void init_profiler(void) {
    /* nothing yet; ensure_real_fns will be called lazily */
}

__attribute__((destructor)) static void fini_profiler(void) {
    dump_stats();
}
