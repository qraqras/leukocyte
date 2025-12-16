#ifndef LEUKOCYTE_CONFIGS_LOADER_H
#define LEUKOCYTE_CONFIGS_LOADER_H

#include <stdbool.h>
#include <yaml.h>
#include "configs/config.h"
#include "configs/generated_config.h"
#include "prism.h"

bool load_config_file_into(config_t *cfg, const char *path, pm_list_t *diagnostics);
bool apply_config(const yaml_document_t *doc, config_t *cfg, pm_list_t *diagnostics);

#endif // LEUKOCYTE_CONFIGS_LOADER_H
