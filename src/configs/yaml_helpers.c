#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "configs/yaml_helpers.h"
#include "allocator/prism_xallocator.h"
#include "severity.h"
#include "configs/rule_config.h"

/**
 * @brief Convert a YAML sequence node to an array of strings.
 * @param doc Pointer to the yaml_document_t structure
 * @param seq_node Pointer to the sequence yaml_node_t structure
 * @param count Pointer to output count of elements
 * @return Array of strings, or NULL if not a sequence node
 */
static char **sequence_to_array(yaml_document_t *doc, yaml_node_t *seq_node, size_t *count)
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
        {
            /* allocate via malloc because caller frees these strings */
            size_t sl = strlen((char *)v->data.scalar.value) + 1;
            char *sdup = malloc(sl);
            if (sdup)
                memcpy(sdup, (char *)v->data.scalar.value, sl);
            arr[idx++] = sdup;
        }
    }
    *count = idx;
    return arr;
}

/**
 * @brief Get a mapping node's value for a given key.
 * @param doc Pointer to the yaml_document_t structure
 * @param mapping_node Pointer to the mapping yaml_node_t structure
 * @param key The key to search for
 * @return Pointer to the value yaml_node_t structure, or NULL if not found
 */
yaml_node_t *yaml_get_mapping_node(const yaml_document_t *doc, yaml_node_t *mapping_node, const char *key)
{
    if (!mapping_node || mapping_node->type != YAML_MAPPING_NODE)
    {
        return NULL;
    }

    for (yaml_node_pair_t *pair = mapping_node->data.mapping.pairs.start; pair < mapping_node->data.mapping.pairs.top; pair++)
    {
        /* libyaml has a non-const API so cast doc for the call */
        yaml_node_t *k = yaml_document_get_node((yaml_document_t *)doc, pair->key);
        if (k && k->type == YAML_SCALAR_NODE && strcmp((char *)k->data.scalar.value, key) == 0)
        {
            return yaml_document_get_node((yaml_document_t *)doc, pair->value);
        }
    }
    return NULL;
}

/**
 * @brief Get a scalar value from a mapping node for a given key.
 * @param doc Pointer to the yaml_document_t structure
 * @param mapping_node Pointer to the mapping yaml_node_t structure
 * @param key The key to search for
 * @return Pointer to the scalar value string, or NULL if not found or not a scalar.
 * @note - The returned pointer is NOT owned by the caller and MUST NOT be freed.
 *         Its lifetime is tied to the provided `doc` (the underlying YAML document),
 *         and it becomes invalid after `yaml_document_delete(&doc)` or when the
 *         document is otherwise destroyed or mutated.
 * @note - If the caller needs an owned copy, duplicate the string (e.g. with
 *         `strdup`).
 */
char *yaml_get_mapping_scalar_value(const yaml_document_t *doc, yaml_node_t *mapping_node, const char *key)
{
    yaml_node_t *node = yaml_get_mapping_node(doc, mapping_node, key);
    if (node && node->type == YAML_SCALAR_NODE)
    {
        return (char *)node->data.scalar.value;
    }
    return NULL;
}

/**
 * @brief Get a sequence of scalar values from a mapping node for a given key.
 * @param doc Pointer to the yaml_document_t structure
 * @param mapping_node Pointer to the mapping yaml_node_t structure
 * @param key The key to search for
 * @param count Pointer to output count of elements
 * @return Array of strings, or NULL if not found or not a sequence node
 * @note - The returned array and its elements are owned by the caller and MUST be freed
 */
char **yaml_get_mapping_sequence_values(const yaml_document_t *doc, yaml_node_t *mapping_node, const char *key, size_t *count)
{
    if (!count)
    {
        return NULL;
    }

    yaml_node_t *node = yaml_get_mapping_node(doc, mapping_node, key);
    if (node && node->type == YAML_SEQUENCE_NODE)
    {
        char **arr = sequence_to_array((yaml_document_t *)doc, node, count);
        if (arr)
        {
            return arr;
        }
    }
    *count = 0;
    return NULL;
}

/* Multi-document helpers */
/* Find scalar: search docs from child->parent and return first found (duplicate returned). */
bool yaml_get_merged_rule_scalar_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, const char *key, char **out)
{
    if (!out || !docs || doc_count == 0 || !key)
        return false;
    *out = NULL;
    for (ssize_t di = (ssize_t)doc_count - 1; di >= 0; di--)
    {
        yaml_document_t *doc = docs[di];
        yaml_node_t *root = yaml_document_get_root_node(doc);
        if (!root || root->type != YAML_MAPPING_NODE)
            continue;

        yaml_node_t *rule_node = yaml_get_mapping_node(doc, root, full_name);
        yaml_node_t *category_node = NULL;
        if (!rule_node && category_name && rule_name)
        {
            yaml_node_t *cat = yaml_get_mapping_node(doc, root, category_name);
            if (cat)
            {
                category_node = cat;
                rule_node = yaml_get_mapping_node(doc, cat, rule_name);
            }
        }

        /* Check rule, then category, then allcops */
        char *val = NULL;
        if (rule_node)
            val = yaml_get_mapping_scalar_value(doc, rule_node, key);
        if (!val && category_node)
            val = yaml_get_mapping_scalar_value(doc, category_node, key);
        if (!val)
        {
            yaml_node_t *allcops = yaml_get_mapping_node(doc, root, LEUKO_ALL_COPS);
            if (allcops)
                val = yaml_get_mapping_scalar_value(doc, allcops, key);
        }
        if (val)
        {
            *out = strdup(val);
            return true;
        }
    }
    return false;
}

bool yaml_get_merged_rule_bool_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, const char *key, bool *out)
{
    char *s = NULL;
    if (!yaml_get_merged_rule_scalar_multi(docs, doc_count, full_name, category_name, rule_name, key, &s))
        return false;
    bool res = false;
    if (strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0 || strcasecmp(s, "yes") == 0 || strcasecmp(s, "on") == 0 || strcasecmp(s, "y") == 0)
        res = true;
    else if (strcasecmp(s, "false") == 0 || strcmp(s, "0") == 0 || strcasecmp(s, "no") == 0 || strcasecmp(s, "off") == 0 || strcasecmp(s, "n") == 0)
        res = false;
    else
        res = false;
    *out = res;
    free(s);
    return true;
}

/* Concatenate sequences from docs in parent-first order. For each doc: allcops, category, rule */
bool yaml_get_merged_rule_sequence_multi(yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, const char *key, char ***out, size_t *count)
{
    if (!out || !count)
        return false;
    *out = NULL;
    *count = 0;
    for (size_t di = 0; di < doc_count; di++)
    {
        yaml_document_t *doc = docs[di];
        yaml_node_t *root = yaml_document_get_root_node(doc);
        if (!root || root->type != YAML_MAPPING_NODE)
            continue;

        /* allcops */
        yaml_node_t *allcops = yaml_get_mapping_node(doc, root, LEUKO_ALL_COPS);
        if (allcops)
        {
            size_t c = 0;
            char **arr = yaml_get_mapping_sequence_values(doc, allcops, key, &c);
            if (arr && c > 0)
            {
                char **tmp = realloc(*out, (*count + c) * sizeof(char *));
                if (!tmp)
                {
                    for (size_t j = 0; j < c; j++)
                        free(arr[j]);
                    free(arr);
                    for (size_t j = 0; j < *count; j++)
                        free((*out)[j]);
                    free(*out);
                    *out = NULL;
                    *count = 0;
                    return false;
                }
                *out = tmp;
                for (size_t j = 0; j < c; j++)
                {
                    (*out)[*count + j] = strdup(arr[j]);
                    free(arr[j]);
                }
                *count += c;
                free(arr);
            }
        }

        /* category */
        yaml_node_t *category_node = NULL;
        if (category_name)
        {
            yaml_node_t *cat = yaml_get_mapping_node(doc, root, category_name);
            if (cat)
            {
                category_node = cat;
                size_t c = 0;
                char **arr = yaml_get_mapping_sequence_values(doc, category_node, key, &c);
                if (arr && c > 0)
                {
                    char **tmp = realloc(*out, (*count + c) * sizeof(char *));
                    if (!tmp)
                    {
                        for (size_t j = 0; j < c; j++)
                            free(arr[j]);
                        free(arr);
                        for (size_t j = 0; j < *count; j++)
                            free((*out)[j]);
                        free(*out);
                        *out = NULL;
                        *count = 0;
                        return false;
                    }
                    *out = tmp;
                    for (size_t j = 0; j < c; j++)
                    {
                        (*out)[*count + j] = strdup(arr[j]);
                        free(arr[j]);
                    }
                    *count += c;
                    free(arr);
                }
            }
        }

        /* rule */
        yaml_node_t *rule_node = yaml_get_mapping_node(doc, root, full_name);
        if (!rule_node && category_name && rule_name)
        {
            yaml_node_t *cat = yaml_get_mapping_node(doc, root, category_name);
            if (cat)
                rule_node = yaml_get_mapping_node(doc, cat, rule_name);
        }
        if (rule_node)
        {
            size_t c = 0;
            char **arr = yaml_get_mapping_sequence_values(doc, rule_node, key, &c);
            if (arr && c > 0)
            {
                char **tmp = realloc(*out, (*count + c) * sizeof(char *));
                if (!tmp)
                {
                    for (size_t j = 0; j < c; j++)
                        free(arr[j]);
                    free(arr);
                    for (size_t j = 0; j < *count; j++)
                        free((*out)[j]);
                    free(*out);
                    *out = NULL;
                    *count = 0;
                    return false;
                }
                *out = tmp;
                for (size_t j = 0; j < c; j++)
                {
                    (*out)[*count + j] = strdup(arr[j]);
                    free(arr[j]);
                }
                *count += c;
                free(arr);
            }
        }
    }
    return (*count > 0);
}
