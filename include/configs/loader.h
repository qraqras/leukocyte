#ifndef LEUKOCYTE_CONFIGS_LOADER_H
#define LEUKOCYTE_CONFIGS_LOADER_H

#include <stdbool.h>
#include <yaml.h>
#include "configs/rule_config.h"
#include "configs/config.h"
#include "prism.h"

/* Load/apply a config file into `cfg` (reads and resolves parents internally). */
bool config_apply_file(config_t *cfg, const char *path, char **err);
/* New API: Apply a configuration file to `cfg` (reads+resolves parents internally). */
bool config_apply_file(config_t *cfg, const char *path, char **err);
/* New API: Apply an array of YAML documents (parent-first) to `cfg`. */
bool config_apply_docs(config_t *cfg, yaml_document_t **docs, size_t doc_count, char **err);
/* NOTE: implementation is public and provides the merging/apply logic. */

#endif // LEUKOCYTE_CONFIGS_LOADER_H
