#ifndef LEUKOCYTE_CONFIGS_LOADER_H
#define LEUKOCYTE_CONFIGS_LOADER_H

#include <stdbool.h>
#include <yaml.h>
#include "configs/rule_config.h"
#include "configs/config.h"
#include "prism.h"

bool load_config_file_into(config_t *cfg, const char *path, char **err);
bool apply_config(yaml_document_t *doc, config_t *cfg, char **err);

#endif // LEUKOCYTE_CONFIGS_LOADER_H
