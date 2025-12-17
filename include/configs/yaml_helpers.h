#ifndef LEUKOCYTE_CONFIGS_YAML_HELPERS_H
#define LEUKOCYTE_CONFIGS_YAML_HELPERS_H

#include <stddef.h>
#include <yaml.h>
#include <stdbool.h>
#include "severity.h"

yaml_node_t *yaml_get_mapping_node(yaml_document_t *doc, yaml_node_t *mapping, const char *key);
char *yaml_get_mapping_scalar_value(yaml_document_t *doc, yaml_node_t *mapping_node, const char *key);

bool yaml_get_merged_string(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, char *out);
bool yaml_get_merged_bool(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, bool *out);
bool yaml_get_merged_sequence(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, char ***out, size_t *count);

bool yaml_get_merged_enabled(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, bool *out);
bool yaml_get_merged_severity(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, severity_level_t *out);
bool yaml_get_merged_include(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char ***out, size_t *count);
bool yaml_get_merged_exclude(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char ***out, size_t *count);

#endif // LEUKOCYTE_CONFIGS_YAML_HELPERS_H
