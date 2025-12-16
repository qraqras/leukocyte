#include "configs/yaml_helpers.h"
#include <stdlib.h>
#include <string.h>

static yaml_node_t *lookup_in_node(const yaml_document_t *doc, yaml_node_t *mapping_node, const char *key)
{
    if (!mapping_node || mapping_node->type != YAML_MAPPING_NODE)
        return NULL;

    for (yaml_node_pair_t *pair = mapping_node->data.mapping.pairs.start; pair < mapping_node->data.mapping.pairs.top; pair++)
    {
        yaml_node_t *k = yaml_document_get_node((yaml_document_t *)doc, pair->key);
        if (k && k->type == YAML_SCALAR_NODE && strcasecmp((char *)k->data.scalar.value, key) == 0)
        {
            return yaml_document_get_node((yaml_document_t *)doc, pair->value);
        }
    }
    return NULL;
}

char *yaml_get_merged_string(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key)
{
    yaml_node_t *n = NULL;
    if (rule_node)
        n = lookup_in_node(doc, rule_node, key);
    if (!n && category_node)
        n = lookup_in_node(doc, category_node, key);
    if (!n && allcops_node)
        n = lookup_in_node(doc, allcops_node, key);
    if (!n || n->type != YAML_SCALAR_NODE)
        return NULL;
    return strdup((char *)n->data.scalar.value);
}

int yaml_get_merged_bool(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, int *out)
{
    char *s = yaml_get_merged_string(doc, rule_node, category_node, allcops_node, key);
    if (!s)
        return 0;
    int val = 0;
    if (strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0)
        val = 1;
    else
        val = 0;
    *out = val;
    free(s);
    return 1;
}

static char **sequence_to_array(const yaml_document_t *doc, yaml_node_t *seq_node, size_t *count)
{
    if (!seq_node || seq_node->type != YAML_SEQUENCE_NODE)
    {
        *count = 0;
        return NULL;
    }
    size_t n = seq_node->data.sequence.items.top - seq_node->data.sequence.items.start;
    char **arr = calloc(n, sizeof(char *));
    if (!arr)
    {
        *count = 0;
        return NULL;
    }
    size_t idx = 0;
    for (yaml_node_item_t *it = seq_node->data.sequence.items.start; it < seq_node->data.sequence.items.top; it++)
    {
        yaml_node_t *v = yaml_document_get_node(doc, *it);
        if (v && v->type == YAML_SCALAR_NODE)
            arr[idx++] = strdup((char *)v->data.scalar.value);
    }
    *count = idx;
    return arr;
}

char **yaml_get_merged_sequence(const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, size_t *count)
{
    // concatenate in order: allcops, category, rule
    char **out = NULL;
    size_t total = 0;

    yaml_node_t *n = NULL;
    n = lookup_in_node(doc, allcops_node, key);
    if (n && n->type == YAML_SEQUENCE_NODE)
    {
        size_t c = 0;
        char **a = sequence_to_array(doc, n, &c);
        if (a)
        {
            out = realloc(out, (total + c) * sizeof(char *));
            for (size_t i = 0; i < c; i++)
                out[total++] = a[i];
            free(a);
        }
    }
    n = lookup_in_node(doc, category_node, key);
    if (n && n->type == YAML_SEQUENCE_NODE)
    {
        size_t c = 0;
        char **a = sequence_to_array(doc, n, &c);
        if (a)
        {
            out = realloc(out, (total + c) * sizeof(char *));
            for (size_t i = 0; i < c; i++)
                out[total++] = a[i];
            free(a);
        }
    }
    n = lookup_in_node(doc, rule_node, key);
    if (n && n->type == YAML_SEQUENCE_NODE)
    {
        size_t c = 0;
        char **a = sequence_to_array(doc, n, &c);
        if (a)
        {
            out = realloc(out, (total + c) * sizeof(char *));
            for (size_t i = 0; i < c; i++)
                out[total++] = a[i];
            free(a);
        }
    }

    if (!out)
    {
        *count = 0;
        return NULL;
    }
    *count = total;
    return out;
}
