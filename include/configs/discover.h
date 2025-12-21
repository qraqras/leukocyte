#ifndef LEUKOCYTE_CONFIGS_DISCOVER_H
#define LEUKOCYTE_CONFIGS_DISCOVER_H

#include "configs/raw_config.h"

/*
 * Discover configuration for a file (or directory). If cli_config_path is
 * provided, it is used as the sole config; otherwise upward discovery is
 * performed starting from dirname(file_path). Returns 0 on success and sets
 * *out_raw to an owned leuko_raw_config_t* containing a merged document (parent-first merging).
 * If no config files are found, *out_raw is set to NULL and 0 is returned.
 */
int leuko_config_discover_for_file(const char *file_path, const char *cli_config_path, leuko_raw_config_t **out_raw, char **err);

#endif /* LEUKOCYTE_CONFIGS_DISCOVER_H */
