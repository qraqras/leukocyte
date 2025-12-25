#include "sources/yaml/node.h"
#include "sources/yaml/parse.h"
#include "sources/yaml/merge.h"
#include <stdlib.h>
#include <string.h>

bool leuko_yaml_parse(const char *path, leuko_yaml_node_t **out)
{
    /* No YAML support available; always fail to indicate no parse */
    (void)path;
    if (out)
        *out = NULL;
    return false;
}

bool leuko_yaml_merge_nodes(leuko_yaml_node_t *a, leuko_yaml_node_t *b, leuko_yaml_node_t **out)
{
    (void)a;
    (void)b;
    if (out)
        *out = NULL;
    return false;
}

leuko_yaml_node_t *leuko_yaml_node_get_mapping_child(const leuko_yaml_node_t *node, const char *key)
{
    (void)node; (void)key; return NULL;
}

size_t leuko_yaml_node_sequence_count(const leuko_yaml_node_t *node)
{
    (void)node; return 0;
}

const char *leuko_yaml_node_sequence_scalar_at(const leuko_yaml_node_t *node, size_t idx)
{
    (void)node; (void)idx; return NULL;
}

leuko_yaml_node_t *leuko_yaml_node_deep_copy(const leuko_yaml_node_t *node)
{
    (void)node; return NULL;
}

void leuko_yaml_node_free(leuko_yaml_node_t *node)
{
    (void)node; /* nothing to free in stub */
}

leuko_yaml_node_t *leuko_yaml_node_get_rule_mapping(leuko_yaml_node_t *root, const char *full_name)
{
    (void)root; (void)full_name; return NULL;
}

const char *leuko_yaml_node_get_rule_mapping_scalar(const leuko_yaml_node_t *root, const char *full_name, const char *key)
{
    (void)root; (void)full_name; (void)key; return NULL;
}

void leuko_yaml_normalize_rule_keys(leuko_yaml_node_t *root)
{
    (void)root; /* no-op */
}
