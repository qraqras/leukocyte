#ifndef LEUKOCYTE_CONFIGS_LOADER_H
#define LEUKOCYTE_CONFIGS_LOADER_H

#include <stdbool.h>
#include <yaml.h>
#include "configs/config.h"
#include "prism.h"

bool config_load_file(const char *path, pm_list_t *diagnostics);
bool config_apply_document(const yaml_document_t *doc, pm_list_t *diagnostics);

#endif // LEUKOCYTE_CONFIGS_LOADER_H
