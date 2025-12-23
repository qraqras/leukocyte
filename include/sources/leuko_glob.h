/* leuko_glob.h */
#ifndef LEUKO_GLOB_H
#define LEUKO_GLOB_H

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Options for leuko_glob_walk
 */
typedef struct leuko_glob_options_s
{
    const char *root;      /* directory to start from, if NULL use current dir */
    const char **includes; /* array of include patterns, NULL means include all */
    size_t includes_count; /* number of include patterns */
    const char **excludes; /* array of exclude patterns, NULL means exclude none */
    size_t excludes_count; /* number of exclude patterns */
    bool recursive;        /* false = non-recursive, true = recursive */
} leuko_glob_options_t;

bool leuko_glob_walk(const leuko_glob_options_t *opts, bool (*cb)(const char *path, void *ctx), void *ctx);

/* Parallel walker using libuv asynchronous scandir. max_concurrency limits
 * number of concurrently active scandir requests. Callback is invoked on the
 * event loop thread; return true to continue, false to abort.
 */
bool leuko_glob_walk_parallel(const leuko_glob_options_t *opts, bool (*cb)(const char *path, void *ctx), void *ctx, size_t max_concurrency);

#endif /* LEUKO_GLOB_H */
