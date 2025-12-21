#ifndef LEUKOCYTE_CONFIGS_YAML_HELPERS_H
#define LEUKOCYTE_CONFIGS_YAML_HELPERS_H

#include <stddef.h>
#include <yaml.h>
#include <stdbool.h>
#include "severity.h"

yaml_node_t *yaml_get_mapping_node(const yaml_document_t *doc, yaml_node_t *mapping, const char *key);
char *yaml_get_mapping_scalar_value(const yaml_document_t *doc, yaml_node_t *mapping_node, const char *key);
char **yaml_get_mapping_sequence_values(const yaml_document_t *doc, yaml_node_t *mapping_node, const char *key, size_t *count);

bool yaml_get_merged_string(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, char *out);
bool yaml_get_merged_bool(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, bool *out);
bool yaml_get_merged_sequence(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, char ***out, size_t *count);

/* Multi-document helpers: search across a parent-first array of documents.
 * docs: array of yaml_document_t* ordered parent-first (parents then child)
 * For scalar: return the last (most-specific, child) occurrence.
 * For sequence: concatenate sequences parent-first then child.
 */
char *yaml_get_merged_rule_scalar_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *key);
bool yaml_get_merged_rule_bool_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *key, bool *out);
bool yaml_get_merged_rule_sequence_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *key, char ***out, size_t *count);

bool yaml_get_merged_enabled(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, bool *out);
bool yaml_get_merged_severity(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, leuko_severity_level_t *out);
bool yaml_get_merged_include(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char ***out, size_t *count);
bool yaml_get_merged_exclude(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char ***out, size_t *count);

/* Map merge helper (recursive): build merged mapping doc for a rule across docs */
yaml_document_t *yaml_merge_rule_mapping_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name);

#endif // LEUKOCYTE_CONFIGS_YAML_HELPERS_H
