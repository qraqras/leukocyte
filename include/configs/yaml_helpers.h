#ifndef LEUKOCYTE_CONFIGS_YAML_HELPERS_H
#define LEUKOCYTE_CONFIGS_YAML_HELPERS_H

#include <stddef.h>
#include <yaml.h>
#include <stdbool.h>
#include "severity.h"

bool yaml_get_merged_string(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, char *out);
bool yaml_get_merged_bool(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, bool *out);
bool yaml_get_merged_sequence(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, char ***out, size_t *count);

bool yaml_get_merged_enabled(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, bool *out);
bool yaml_get_merged_severity(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, severity_level_t *out);
bool yaml_get_merged_include(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char ***out, size_t *count);
bool yaml_get_merged_exclude(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char ***out, size_t *count);

#endif // LEUKOCYTE_CONFIGS_YAML_HELPERS_H
