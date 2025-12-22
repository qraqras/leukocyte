#ifndef LEUKOCYTE_CONFIGS_DISCOVER_H
#define LEUKOCYTE_CONFIGS_DISCOVER_H

#include "configs/discovery/raw_config.h"
#include "configs/config.h"

/*
 * Discover configuration for a file (or directory). If cli_config_path is
 * provided, it is used as the sole config; otherwise upward discovery is
 * performed starting from dirname(file_path). Returns 0 on success and sets
 * *out_raw to an owned leuko_raw_config_t* containing a merged document (parent-first merging).
 * If no config files are found, *out_raw is set to NULL and 0 is returned.
 */
int leuko_config_discover_for_file(const char *file_path, const char *cli_config_path, leuko_raw_config_t **out_raw, char **err);

/* Get a runtime `leuko_config_t` for `file_path` using discovery with caching.
 * - If a config is found, *out_cfg will be set to a pointer owned by the cache and
 *   valid until `leuko_config_clear_cache()` is called. If no config is found, *out_cfg
 *   will be set to NULL and 0 is returned. On fatal error, non-zero is returned and
 *   *err may be set to a message.
 */
int leuko_config_get_cached_config_for_file(const char *file_path, const leuko_config_t **out_cfg, char **err);

/* Read-only cache lookup for worker threads. This will NOT perform discovery or insert into the cache.
 * If a valid cached config is found, *out_cfg is set to a pointer owned by the cache and 0 is returned.
 * If no valid cached config exists, *out_cfg is set to NULL and 0 is returned (no error).
 * On invalid arguments, a non-zero value is returned.
 */
int leuko_config_get_cached_config_for_file_ro(const char *file_path, const leuko_config_t **out_cfg);

/* Clear the runtime config cache, freeing stored configs. */
void leuko_config_clear_cache(void);

/* Warm the runtime cache for a set of files. This discovers and converts
 * configs for the unique start directories of the provided files. If
 * workers_count is 0 it will run single-threaded. Returns 0 on success or
 * non-zero on fatal error and sets *err to an allocated message. */
int leuko_config_warm_cache_for_files(char **files, size_t files_count, size_t workers_count, char **err);

#endif /* LEUKOCYTE_CONFIGS_DISCOVER_H */
