#include "sources/node.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Minimal but functional node implementation used by the JSON parser.
 * Implements object/array/scalar storage and basic helpers used by
 * compiled_config and rule handlers.
 */

leuko_node_t *leuko_node_get_mapping_child(const leuko_node_t *node, const char *key)
{
    if (!node || node->type != LEUKO_NODE_OBJECT || !key)
        return NULL;
    for (size_t i = 0; i < node->map_len; i++)
    {
        const char *k = node->map_keys[i];
        if (k && strcmp(k, key) == 0)
            return node->map_vals[i];
    }
    return NULL;
}

size_t leuko_node_array_count(const leuko_node_t *node)
{
    if (!node || node->type != LEUKO_NODE_ARRAY)
        return 0;
    return node->arr_len;
}

const char *leuko_node_array_scalar_at(const leuko_node_t *node, size_t idx)
{
    if (!node || node->type != LEUKO_NODE_ARRAY || idx >= node->arr_len)
        return NULL;
    leuko_node_t *child = node->map_vals[idx];
    if (!child || !LEUKO_NODE_IS_SCALAR(child->type))
        return NULL;
    return child->scalar;
}

leuko_node_t *leuko_node_new_scalar(leuko_node_type_t t, const char *s)
{
    leuko_node_t *n = calloc(1, sizeof(leuko_node_t));
    if (!n)
        return NULL;
    n->type = t;
    n->arr_len = 0;
    n->map_len = 0;
    n->map_keys = NULL;
    n->map_vals = NULL;
    n->scalar = s ? strdup(s) : NULL;
    return n;
}

static leuko_node_t *leuko_node_new_object(size_t n)
{
    leuko_node_t *node = calloc(1, sizeof(leuko_node_t));
    if (!node)
        return NULL;
    node->type = LEUKO_NODE_OBJECT;
    node->map_len = n;
    node->map_keys = n ? calloc(n, sizeof(char *)) : NULL;
    node->map_vals = n ? calloc(n, sizeof(leuko_node_t *)) : NULL;
    node->arr_len = 0;
    node->scalar = NULL;
    return node;
}

static leuko_node_t *leuko_node_new_array(size_t n)
{
    leuko_node_t *node = calloc(1, sizeof(leuko_node_t));
    if (!node)
        return NULL;
    node->type = LEUKO_NODE_ARRAY;
    node->arr_len = n;
    node->map_vals = n ? calloc(n, sizeof(leuko_node_t *)) : NULL;
    node->map_len = 0;
    node->map_keys = NULL;
    node->scalar = NULL;
    return node;
}

leuko_node_t *leuko_node_deep_copy(const leuko_node_t *node)
{
    if (!node)
        return NULL;
    leuko_node_t *copy = calloc(1, sizeof(leuko_node_t));
    if (!copy)
        return NULL;
    copy->type = node->type;
    copy->arr_len = node->arr_len;
    copy->map_len = node->map_len;
    copy->scalar = node->scalar ? strdup(node->scalar) : NULL;

    if (node->map_len > 0)
    {
        copy->map_keys = calloc(node->map_len, sizeof(char *));
        copy->map_vals = calloc(node->map_len, sizeof(leuko_node_t *));
        for (size_t i = 0; i < node->map_len; i++)
        {
            copy->map_keys[i] = node->map_keys[i] ? strdup(node->map_keys[i]) : NULL;
            copy->map_vals[i] = leuko_node_deep_copy(node->map_vals[i]);
        }
    }
    else if (node->arr_len > 0)
    {
        copy->map_vals = calloc(node->arr_len, sizeof(leuko_node_t *));
        for (size_t i = 0; i < node->arr_len; i++)
            copy->map_vals[i] = leuko_node_deep_copy(node->map_vals[i]);
    }
    else
    {
        copy->map_keys = NULL;
        copy->map_vals = NULL;
    }
    return copy;
}

void leuko_node_free(leuko_node_t *node)
{
    if (!node)
        return;
    if (node->map_len > 0 && node->map_keys)
    {
        for (size_t i = 0; i < node->map_len; i++)
        {
            free((void *)node->map_keys[i]);
            leuko_node_free(node->map_vals[i]);
        }
        free(node->map_keys);
        free(node->map_vals);
    }
    else if (node->arr_len > 0 && node->map_vals)
    {
        for (size_t i = 0; i < node->arr_len; i++)
            leuko_node_free(node->map_vals[i]);
        free(node->map_vals);
    }
    if (node->scalar)
        free(node->scalar);
    free(node);
}

leuko_node_t *leuko_node_get_rule_mapping(leuko_node_t *root, const char *full_name)
{
    (void)root;
    (void)full_name;
    return NULL; /* keep stub for now */
}

const char *leuko_node_get_rule_mapping_scalar(const leuko_node_t *root, const char *full_name, const char *key)
{
    (void)root;
    (void)full_name;
    (void)key;
    return NULL; /* keep stub */
}

void leuko_node_visit_mapping(const leuko_node_t *node, void *ctx, bool (*visitor)(const char *key, leuko_node_t *val, void *ctx))
{
    if (!node || node->type != LEUKO_NODE_OBJECT || !visitor)
        return;
    for (size_t i = 0; i < node->map_len; i++)
    {
        const char *k = node->map_keys[i];
        leuko_node_t *v = node->map_vals[i];
        if (!k)
            continue;
        bool keep = visitor(k, v, ctx);
        if (!keep)
            break;
    }
}
