#ifndef LEUKOCYTE_CONFIGS_LOADER_H
#define LEUKOCYTE_CONFIGS_LOADER_H

#include <stdbool.h>
#include <yaml.h>

#include "configs/rule_config.h"
#include "configs/config.h"
#include "prism.h"

bool load_config_file_into(leuko_config_t *cfg, const char *path, char **err);
/* Apply an array of YAML documents (parent-first) to a leuko_config_t. */
bool apply_config_docs(yaml_document_t **docs, size_t doc_count, leuko_config_t *cfg, char **err);

#endif // LEUKOCYTE_CONFIGS_LOADER_H
