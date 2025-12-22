#ifndef LEUKOCYTE_CONFIGS_YAML_HELPERS_H
#define LEUKOCYTE_CONFIGS_YAML_HELPERS_H

#include <stddef.h>
#include <yaml.h>
#include <stdbool.h>
#include "severity.h"

yaml_node_t *yaml_get_mapping_node(const yaml_document_t *doc, yaml_node_t *mapping, const char *key);
char *yaml_get_mapping_scalar_value(const yaml_document_t *doc, yaml_node_t *mapping_node, const char *key);
char **yaml_get_mapping_sequence_values(const yaml_document_t *doc, yaml_node_t *mapping_node, const char *key, size_t *count);

bool yaml_get_merged_rule_scalar_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, const char *key, char **out);
bool yaml_get_merged_rule_bool_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, const char *key, bool *out);
bool yaml_get_merged_rule_sequence_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, const char *key, char ***out, size_t *count);

#endif // LEUKOCYTE_CONFIGS_YAML_HELPERS_H
