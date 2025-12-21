#ifndef LEUKOCYTE_IO_SCAN_H
#define LEUKOCYTE_IO_SCAN_H

#include <stddef.h>
#include <stdbool.h>

/* Collect Ruby files from the provided paths.
 * - `paths`/`paths_count` are input path strings (files or directories).
 * - On success: returns true, sets *out_paths to a malloc'd array of malloc'd strings
 *   and *out_count to the number of paths. Caller must free each string and the
 *   array with free().
 * - Special case: a path equal to "-" is treated as a filename token and is
 *   included verbatim (no filesystem checks) to represent stdin-like input.
 * - On failure: returns false and, if `err` is non-NULL, sets *err to a strdup'd
 *   error message. Any partially allocated results are freed.
 */
bool leuko_collect_ruby_files(char **paths, size_t paths_count, char ***out_paths, size_t *out_count, char **err);

/* Variant that accepts exclude patterns (fnmatch-style). If exclude_count > 0,
 * directories or files matching any pattern will be skipped during collection.
 */
bool leuko_collect_ruby_files_with_exclude(char **paths, size_t paths_count, char ***out_paths, size_t *out_count, char **excludes, size_t exclude_count, char **err);

#endif /* LEUKOCYTE_IO_SCAN_H */
