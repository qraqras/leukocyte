#include <stdbool.h>
#include <yaml.h>

#ifndef LEUKO_SOURCES_YAML_MERGE_H
#define LEUKO_SOURCES_YAML_MERGE_H

/* In-memory merge API (leuko_yaml_node):
 * - Convert YAML documents to a lightweight in-memory representation (`leuko_yaml_node_t`).
 * - Merge two `leuko_yaml_node_t` trees according to `inherit_mode` rules and return a
 *   newly allocated `leuko_yaml_node_t` representing the merged configuration.
 * - This avoids emitter/parser roundtrips; callers can use the returned in-memory node
 *   directly. The old document-based API is deprecated.
 */

typedef struct leuko_yaml_node_s leuko_yaml_node_t;

enum leuko_yaml_node_type_e
{
    LEUKO_YAML_NODE_SCALAR,
    LEUKO_YAML_NODE_SEQUENCE,
    LEUKO_YAML_NODE_MAPPING,
};

/* Merge two leuko_yaml_node trees into a newly allocated node. Returns true on success
 * and stores the result in *out_merged. Caller owns and must free *out_merged. */
bool leuko_yaml_merge_nodes(leuko_yaml_node_t *parent, leuko_yaml_node_t *child, leuko_yaml_node_t **out_merged);

/* Parse existing yaml_document_t into a leuko_yaml_node tree. Caller owns and must
 * free the returned node via leuko_yaml_node_free(). */
leuko_yaml_node_t *leuko_yaml_node_from_document(yaml_document_t *doc);

/* Free a leuko_yaml_node tree */
void leuko_yaml_node_free(leuko_yaml_node_t *n);

/* Lookup helpers */
leuko_yaml_node_t *leuko_yaml_node_get_mapping_child(leuko_yaml_node_t *root, const char *key);
const char *leuko_yaml_node_get_mapping_scalar(leuko_yaml_node_t *map, const char *key);
leuko_yaml_node_t *leuko_yaml_node_get_rule_mapping(leuko_yaml_node_t *root, const char *full_name);
const char *leuko_yaml_node_get_rule_mapping_scalar(leuko_yaml_node_t *root, const char *full_name, const char *key);

/* Document helper: find mapping value by key inside a YAML mapping node */
/* Returns the value node or NULL if not found */
yaml_node_t *leuko_yaml_find_mapping_value(yaml_document_t *doc, yaml_node_t *mapping, const char *key);

/* Common config key names */
#define LEUKO_CONFIG_KEY_EXCLUDE "Exclude"
#define LEUKO_CONFIG_KEY_INCLUDE "Include"
#define LEUKO_CONFIG_KEY_INHERIT_MODE "inherit_mode"

/* Normalize: expand category-level nested rule mappings into full-name keys (Category/Rule)
 * Example: Layout: TrailingWhitespace: Enabled: true  -> creates root mapping key "Layout/TrailingWhitespace"
 */
void leuko_yaml_normalize_rule_keys(leuko_yaml_node_t *root);

/* Sequence helpers */
size_t leuko_yaml_node_sequence_count(leuko_yaml_node_t *seq);
const char *leuko_yaml_node_sequence_scalar_at(leuko_yaml_node_t *seq, size_t idx);

#endif /* LEUKO_SOURCES_YAML_MERGE_H */
