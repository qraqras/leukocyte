#ifndef LEUKOCYTE_SOURCES_NODE_H
#define LEUKOCYTE_SOURCES_NODE_H

#include <stddef.h>
#include <stdbool.h>

/**
 * @brief Node types representing JSON structures.
 */
typedef enum leuko_node_type_e
{
    LEUKO_NODE_NULL,    /* JSON null */
    LEUKO_NODE_OBJECT,  /* JSON object */
    LEUKO_NODE_ARRAY,   /* JSON array */
    LEUKO_NODE_STRING,  /* JSON string */
    LEUKO_NODE_NUMBER,  /* JSON number */
    LEUKO_NODE_BOOLEAN, /* JSON boolean */
} leuko_node_type_t;

/**
 * @brief Macro to check if a node type is scalar.
 */
#define LEUKO_NODE_IS_SCALAR(t) ((t) == LEUKO_NODE_STRING || (t) == LEUKO_NODE_NUMBER || (t) == LEUKO_NODE_BOOLEAN)

typedef struct leuko_node_s
{
    leuko_node_type_t type;
    char *scalar;
    size_t arr_len;
    size_t map_len;
    const char **map_keys;
    struct leuko_node_s **map_vals;
} leuko_node_t;

/* Accessors used by compiled_config.c and rule handlers */
leuko_node_t *leuko_node_get_mapping_child(const leuko_node_t *node, const char *key);
size_t leuko_node_array_count(const leuko_node_t *node);
const char *leuko_node_array_scalar_at(const leuko_node_t *node, size_t idx);
leuko_node_t *leuko_node_deep_copy(const leuko_node_t *node);
void leuko_node_free(leuko_node_t *node);
leuko_node_t *leuko_node_get_rule_mapping(leuko_node_t *root, const char *full_name);
const char *leuko_node_get_rule_mapping_scalar(const leuko_node_t *root, const char *full_name, const char *key);

/* Visit mapping children: call visitor(key, val, ctx) for each mapping entry.
 * If visitor returns false, stop iteration early. */
void leuko_node_visit_mapping(const leuko_node_t *node, void *ctx, bool (*visitor)(const char *key, leuko_node_t *val, void *ctx));

/* Internal constructor helper (internal use only) */
leuko_node_t *leuko_node_new_scalar(leuko_node_type_t t, const char *s);

#endif /* LEUKOCYTE_SOURCES_NODE_H */
