#ifndef LEUKOCYTE_CONFIGS_YAML_HELPERS_H
#define LEUKOCYTE_CONFIGS_YAML_HELPERS_H

#include <stddef.h>
#include <yaml.h>

char *yaml_get_merged_string(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key);
int yaml_get_merged_bool(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, int *out);
char **yaml_get_merged_sequence(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, size_t *count);

#endif // LEUKOCYTE_CONFIGS_YAML_HELPERS_H
