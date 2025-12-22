/* Recursive YAML mapping merger for rule-specific maps */

#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "configs/conversion/yaml_helpers.h"
#include "configs/conversion/merge.h"
#include "configs/rule_config.h"

typedef enum leuko_merge_node_type_e leuko_merge_node_type_t;
typedef struct leuko_merge_node_s leuko_merge_node_t;
typedef struct leuko_merge_node_entry_s leuko_merge_node_entry_t;

/**
 * @brief Node types for merge matrix.
 */
enum leuko_merge_node_type_e
{
    LEUKO_MERGE_NODE_NONE,
    LEUKO_MERGE_NODE_SCALAR,
    LEUKO_MERGE_NODE_SEQUENCE,
    LEUKO_MERGE_NODE_MAPPING,
};

/**
 * @brief Merge node entry for mapping nodes.
 */
struct leuko_merge_node_entry_s
{
    char *key;
    leuko_merge_node_t *value;
};

/**
 * @brief Merge node structure.
 */
struct leuko_merge_node_s
{
    leuko_merge_node_type_t type;
    char *scalar;
    char **sequence;
    size_t sequence_count;
    leuko_merge_node_entry_t *map;
    size_t map_count;
};

/**
 * @brief Create a new merge node.
 * @return newly allocated merge node
 */
static leuko_merge_node_t *leuko_merge_node_new(void)
{
    leuko_merge_node_t *n = calloc(1, sizeof(*n));
    return n;
}

/**
 * @brief Free a merge node and its contents.
 * @param n Pointer to the merge node to free
 */
static void leuko_merge_node_free(leuko_merge_node_t *n)
{
    if (!n)
    {
        return;
    }
    if (n->scalar)
    {
        free(n->scalar);
    }
    if (n->sequence)
    {
        for (size_t i = 0; i < n->sequence_count; i++)
        {
            free(n->sequence[i]);
        }
        free(n->sequence);
    }
    if (n->map)
    {
        for (size_t i = 0; i < n->map_count; i++)
        {
            free(n->map[i].key);
            leuko_merge_node_free(n->map[i].value);
        }
        free(n->map);
    }
    free(n);
}

/**
 * @brief Clone a merge node and its contents.
 * @param src Pointer to the merge node to clone
 * @return newly allocated clone of the merge node
 */
static leuko_merge_node_t *leuko_merge_node_clone(leuko_merge_node_t *src)
{
    if (!src)
    {
        return NULL;
    }
    leuko_merge_node_t *clone = leuko_merge_node_new();
    clone->type = src->type;
    if (src->scalar)
        clone->scalar = strdup(src->scalar);
    if (src->sequence && src->sequence_count > 0)
    {
        clone->sequence = calloc(src->sequence_count, sizeof(char *));
        for (size_t i = 0; i < src->sequence_count; i++)
        {
            clone->sequence[i] = strdup(src->sequence[i]);
        }
        clone->sequence_count = src->sequence_count;
    }
    if (src->map && src->map_count > 0)
    {
        clone->map = calloc(src->map_count, sizeof(leuko_merge_node_entry_t));
        for (size_t i = 0; i < src->map_count; i++)
        {
            clone->map[i].key = strdup(src->map[i].key);
            clone->map[i].value = leuko_merge_node_clone(src->map[i].value);
        }
        clone->map_count = src->map_count;
    }
    return clone;
}

/**
 * @brief Set a scalar value in a merge node.
 * @param np Pointer to the merge node pointer
 * @param s The scalar string to set
 * @return true on success, false on failure
 */
static bool leuko_merge_node_scalar_set(leuko_merge_node_t **np, const char *s)
{
    if (!np || !s)
    {
        return false;
    }
    /* Create if NULL */
    leuko_merge_node_t *n = *np;
    if (!n)
    {
        n = leuko_merge_node_new();
        n->type = LEUKO_MERGE_NODE_SCALAR;
        n->scalar = strdup(s);
        *np = n;
        return true;
    }
    /* Replace if type differs */
    if (n->scalar)
    {
        free(n->scalar);
    }
    if (n->sequence)
    {
        for (size_t i = 0; i < n->sequence_count; i++)
        {
            free(n->sequence[i]);
        }
        free(n->sequence);
        n->sequence = NULL;
        n->sequence_count = 0;
    }
    if (n->map)
    {
        for (size_t i = 0; i < n->map_count; i++)
        {
            free(n->map[i].key);
            leuko_merge_node_free(n->map[i].value);
        }
        free(n->map);
        n->map = NULL;
        n->map_count = 0;
    }
    n->type = LEUKO_MERGE_NODE_SCALAR;
    n->scalar = strdup(s);
    return true;
}

/**
 * @brief Concatenate a sequence into a merge node sequence.
 * @param np Pointer to the merge node pointer
 * @param seq Array of strings to concatenate
 * @param seq_count Number of strings in the array
 * @return true on success, false on failure
 */
static bool leuko_merge_node_sequence_set(leuko_merge_node_t **np, char **seq, size_t seq_count)
{
    if (!np || !seq || seq_count == 0)
    {
        return false;
    }
    /* Create if NULL */
    leuko_merge_node_t *n = *np;
    if (!n)
    {
        n = leuko_merge_node_new();
        n->type = LEUKO_MERGE_NODE_SEQUENCE;
        *np = n;
    }
    /* Replace if type differs */
    if (n->type == LEUKO_MERGE_NODE_SCALAR || n->type == LEUKO_MERGE_NODE_MAPPING)
    {
        leuko_merge_node_free(n);
        n = leuko_merge_node_new();
        n->type = LEUKO_MERGE_NODE_SEQUENCE;
        *np = n;
    }
    /* Concatenate sequence */
    char **tmp = realloc(n->sequence, (n->sequence_count + seq_count) * sizeof(char *));
    if (!tmp)
    {
        return false;
    }
    n->sequence = tmp;
    for (size_t i = 0; i < seq_count; i++)
    {
        n->sequence[n->sequence_count + i] = strdup(seq[i]);
    }
    n->sequence_count += seq_count;
    return true;
}

/**
 * @brief Get a mapping entry from a merge node.
 * @param n Pointer to the mapping merge node
 * @param key The key to search for
 * @return Pointer to the value merge node, or NULL if not found
 */
static leuko_merge_node_t *leuko_merge_node_mapping_get(leuko_merge_node_t *n, const char *key)
{
    if (!n || n->type != LEUKO_MERGE_NODE_MAPPING)
    {
        return NULL;
    }
    for (size_t i = 0; i < n->map_count; i++)
    {
        if (strcmp(n->map[i].key, key) == 0)
        {
            return n->map[i].value;
        }
    }
    return NULL;
}

/**
 * @brief Set a mapping entry in a merge node.
 * @param np Pointer to the mapping merge node pointer
 * @param key The key to set
 * @param val Pointer to the value merge node
 * @return true on success, false on failure
 */
static bool leuko_merge_node_mapping_set(leuko_merge_node_t **np, const char *key, leuko_merge_node_t *val)
{
    if (!np || !key || !val)
    {
        return false;
    }
    /* Create if NULL */
    leuko_merge_node_t *n = *np;
    if (!n)
    {
        n = leuko_merge_node_new();
        n->type = LEUKO_MERGE_NODE_MAPPING;
        *np = n;
    }
    /* Replace if type differs */
    if (n->type != LEUKO_MERGE_NODE_MAPPING)
    {
        /* child mapping replaces parent type */
        leuko_merge_node_free(n);
        n = leuko_merge_node_new();
        n->type = LEUKO_MERGE_NODE_MAPPING;
        *np = n;
    }
    /* Replace if exists */
    for (size_t i = 0; i < n->map_count; i++)
    {
        if (strcmp(n->map[i].key, key) == 0)
        {
            leuko_merge_node_free(n->map[i].value);
            n->map[i].value = val;
            return true;
        }
    }
    /* Append */
    leuko_merge_node_entry_t *tmp = realloc(n->map, (n->map_count + 1) * sizeof(leuko_merge_node_entry_t));
    if (!tmp)
    {
        return false;
    }
    n->map = tmp;
    n->map[n->map_count].key = strdup(key);
    n->map[n->map_count].value = val;
    n->map_count++;
    return true;
}

/**
 * @brief Merge a YAML mapping node into a merge node.
 * @param np Pointer to the merge node pointer
 * @param doc Pointer to the YAML document
 * @param map Pointer to the YAML mapping node
 * @return true on success, false on failure
 */
static bool leuko_merge_node_mapping_merge(leuko_merge_node_t **np, yaml_document_t *doc, yaml_node_t *map)
{
    if (!map || map->type != YAML_MAPPING_NODE)
    {
        return true;
    }
    for (yaml_node_pair_t *pair = map->data.mapping.pairs.start; pair < map->data.mapping.pairs.top; pair++)
    {
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);
        if (!k || k->type != YAML_SCALAR_NODE)
        {
            continue;
        }
        const char *key = (const char *)k->data.scalar.value;
        switch (v->type)
        {
        case YAML_SCALAR_NODE:
            leuko_merge_node_t *val = leuko_merge_node_new();
            val->type = LEUKO_MERGE_NODE_SCALAR;
            val->scalar = strdup((char *)v->data.scalar.value);
            leuko_merge_node_mapping_set(np, key, val);
            break;
        case YAML_SEQUENCE_NODE:
            size_t cap = v->data.sequence.items.top - v->data.sequence.items.start;
            char **arr = calloc(cap, sizeof(char *));
            size_t idx = 0;
            for (yaml_node_item_t *i = v->data.sequence.items.start; i < v->data.sequence.items.top; i++)
            {
                yaml_node_t *in = yaml_document_get_node(doc, *i);
                if (in && in->type == YAML_SCALAR_NODE)
                {
                    arr[idx++] = strdup((char *)in->data.scalar.value);
                }
            }
            if (idx > 0)
            {
                leuko_merge_node_t *existing = leuko_merge_node_mapping_get(*np, key);
                if (!existing)
                {
                    leuko_merge_node_t *seqn = leuko_merge_node_new();
                    seqn->type = LEUKO_MERGE_NODE_SEQUENCE;
                    seqn->sequence = arr;
                    seqn->sequence_count = idx;
                    leuko_merge_node_mapping_set(np, key, seqn);
                }
                else
                {
                    leuko_merge_node_sequence_set(&(existing), arr, idx);
                    for (size_t i = 0; i < idx; i++)
                    {
                        free(arr[i]);
                    }
                    free(arr);
                }
            }
            else
            {
                free(arr);
            }
            break;
        case YAML_MAPPING_NODE:
            leuko_merge_node_t *sub = leuko_merge_node_mapping_get(*np, key);
            if (!sub || sub->type != LEUKO_MERGE_NODE_MAPPING)
            {
                sub = leuko_merge_node_new();
                sub->type = LEUKO_MERGE_NODE_MAPPING;
                leuko_merge_node_mapping_set(np, key, sub);
            }
            leuko_merge_node_mapping_merge(&(sub), doc, v);
            break;
        default:
            break;
        }
    }
    return true;
}

/**
 * @brief Build a YAML document from a merge node.
 * @param doc Pointer to the YAML document to build into
 * @param n Pointer to the merge node
 * @return index of the root node in the document, or 0 on failure
 */
static int leuko_doc_build(yaml_document_t *doc, leuko_merge_node_t *n)
{
    if (!n)
    {
        return 0;
    }
    switch (n->type)
    {
    case LEUKO_MERGE_NODE_SCALAR:
        return yaml_document_add_scalar(doc, NULL, (yaml_char_t *)n->scalar, -1, YAML_PLAIN_SCALAR_STYLE);
        break;
    case LEUKO_MERGE_NODE_SEQUENCE:
        int seq = yaml_document_add_sequence(doc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
        for (size_t i = 0; i < n->sequence_count; i++)
        {
            int s = yaml_document_add_scalar(doc, NULL, (yaml_char_t *)n->sequence[i], -1, YAML_PLAIN_SCALAR_STYLE);
            yaml_document_append_sequence_item(doc, seq, s);
        }
        return seq;
        break;
    case LEUKO_MERGE_NODE_MAPPING:
        int map = yaml_document_add_mapping(doc, NULL, YAML_BLOCK_MAPPING_STYLE);
        for (size_t i = 0; i < n->map_count; i++)
        {
            int key = yaml_document_add_scalar(doc, NULL, (yaml_char_t *)n->map[i].key, -1, YAML_PLAIN_SCALAR_STYLE);
            int val = leuko_doc_build(doc, n->map[i].value);
            yaml_document_append_mapping_pair(doc, map, key, val);
        }
        return map;
        break;
    default:
        break;
    }
    return 0;
}

yaml_document_t *yaml_merge_documents_multi(yaml_document_t **docs, size_t doc_count)
{
    if (!docs || doc_count == 0)
        return NULL;

    leuko_merge_node_t *root = NULL;
    for (size_t di = 0; di < doc_count; di++)
    {
        yaml_document_t *doc = docs[di];
        yaml_node_t *r = yaml_document_get_root_node(doc);
        if (!r || r->type != YAML_MAPPING_NODE)
            continue;
        leuko_merge_node_mapping_merge(&root, doc, r);
    }

    if (!root)
        return NULL;

    yaml_document_t *out = malloc(sizeof(yaml_document_t));
    if (!out)
    {
        leuko_merge_node_free(root);
        return NULL;
    }
    if (!yaml_document_initialize(out, NULL, NULL, NULL, 1, 1))
    {
        free(out);
        leuko_merge_node_free(root);
        return NULL;
    }

    int map = leuko_doc_build(out, root);
    if (map == 0)
    {
        yaml_document_delete(out);
        free(out);
        leuko_merge_node_free(root);
        return NULL;
    }

    leuko_merge_node_free(root);
    return out;
}
