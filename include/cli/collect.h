/* include/cli/collect.h */
#ifndef LEUKO_CLI_COLLECT_H
#define LEUKO_CLI_COLLECT_H

#include <stdbool.h>
#include <stddef.h>
#include "configs/config_cache.h"

/*
 * Collect Ruby files from paths (files/directories/globs). `paths` may be NULL
 * or empty to indicate current directory. The function reads resolved config
 * information from `index_path` (if NULL, default is ".leukocyte/index.json")
 * and applies resolved Exclude patterns to filter files.
 *
 * On success returns true with `*out_files` set to a malloc'd array of strings
 * (caller must free each string and the array) and `*out_count` set. On failure
 * returns false and leaves out_* untouched.
 */
bool leuko_collect_ruby_files(const char *const *paths, size_t paths_count,
                              char ***out_files, size_t *out_count,
                              leuko_config_cache_t *cache);

#endif /* LEUKO_CLI_COLLECT_H */
