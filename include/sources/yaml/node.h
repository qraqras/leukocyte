#ifndef LEUKOCYTE_SOURCES_YAML_NODE_H
#define LEUKOCYTE_SOURCES_YAML_NODE_H

#include <stddef.h>

typedef enum {
    LEUKO_YAML_NODE_NULL = 0,
    LEUKO_YAML_NODE_MAPPING,
    LEUKO_YAML_NODE_SEQUENCE,
    LEUKO_YAML_NODE_SCALAR,
} leuko_yaml_node_type_t;

typedef struct leuko_yaml_node_s {
    leuko_yaml_node_type_t type;
    size_t map_len;
    const char **map_keys;
    struct leuko_yaml_node_s **map_vals;
    size_t seq_len;
    /* scalar values are returned via accessors */
} leuko_yaml_node_t;

/* Accessors used by compiled_config.c */
leuko_yaml_node_t *leuko_yaml_node_get_mapping_child(const leuko_yaml_node_t *node, const char *key);
size_t leuko_yaml_node_sequence_count(const leuko_yaml_node_t *node);
const char *leuko_yaml_node_sequence_scalar_at(const leuko_yaml_node_t *node, size_t idx);
leuko_yaml_node_t *leuko_yaml_node_deep_copy(const leuko_yaml_node_t *node);
void leuko_yaml_node_free(leuko_yaml_node_t *node);
leuko_yaml_node_t *leuko_yaml_node_get_rule_mapping(leuko_yaml_node_t *root, const char *full_name);
const char *leuko_yaml_node_get_rule_mapping_scalar(const leuko_yaml_node_t *root, const char *full_name, const char *key);
void leuko_yaml_normalize_rule_keys(leuko_yaml_node_t *root);

#endif /* LEUKOCYTE_SOURCES_YAML_NODE_H */