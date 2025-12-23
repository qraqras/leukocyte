/* leuko_file_collector.h */
#ifndef LEUKO_FILE_COLLECTOR_H
#define LEUKO_FILE_COLLECTOR_H

#include <stddef.h>

/*
 * Collect Ruby-related files under given paths.
 *
 * Parameters:
 *  - paths: array of paths provided by CLI (files or directories)
 *  - paths_count: number of entries in `paths`
 *  - cb: callback invoked for each collected file; returns 0 to continue,
 *        non-zero to abort and return that value from this function
 *  - ctx: user context pointer passed to cb
 *
 * Return: 0 on success, or the non-zero value returned by cb if aborted,
 *         or a negative value on error.
 */
int leuko_collect_ruby_files(const char **paths, size_t paths_count, int (*cb)(const char *path, void *ctx), void *ctx);

/* Collect files into an array (arena-backed).
 * Caller receives ownership of `*out_paths` and `*out_arena` and must
 * call `leuko_arena_free(*out_arena)` to release memory when done.
 *
 * Parameters:
 *  - paths, paths_count: CLI-provided paths (files or directories)
 *  - out_paths/out_count: outputs; on success *out_paths is allocated and
 *      contains `*out_count` pointers to null-terminated strings owned by
 *      the returned arena.
 *  - includes/excludes + counts: optional pattern lists; NULL/0 means defaults
 *  - initial_capacity: hint for number of expected files (0 = default)
 *  - out_arena: pointer to receive the arena pointer for freeing
 *
 * Return: 0 on success, negative on error.
 */
int leuko_collect_ruby_files_to_array(const char **paths,
                                      size_t paths_count,
                                      char ***out_paths,
                                      size_t *out_count,
                                      const char **includes,
                                      size_t includes_count,
                                      const char **excludes,
                                      size_t excludes_count,
                                      size_t initial_capacity,
                                      struct leuko_arena **out_arena);

#endif /* LEUKO_FILE_COLLECTOR_H */
