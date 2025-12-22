/* Recursive YAML mapping merger for rule-specific maps */

#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "configs/loader/yaml_helpers.h"
#include "configs/loader/merge.h"
#include "configs/rule_config.h"

/* Simple merged-node representation */
typedef enum
{
    MN_NONE = 0,
    MN_SCALAR,
    MN_SEQUENCE,
    MN_MAPPING
} mm_type_t;

typedef struct mm_node_s mm_node_t;
typedef struct mm_entry_s mm_entry_t;

struct mm_entry_s
{
    char *key;
    mm_node_t *value;
};

struct mm_node_s
{
    mm_type_t type;
    char *scalar;
    char **sequence;
    size_t seq_count;
    mm_entry_t *map;
    size_t map_count;
};

static mm_node_t *mm_node_new(void)
{
    mm_node_t *n = calloc(1, sizeof(*n));
    return n;
}

static void mm_node_free(mm_node_t *n)
{
    if (!n)
        return;
    if (n->scalar)
        free(n->scalar);
    if (n->sequence)
    {
        for (size_t i = 0; i < n->seq_count; i++)
            free(n->sequence[i]);
        free(n->sequence);
    }
    if (n->map)
    {
        for (size_t i = 0; i < n->map_count; i++)
        {
            free(n->map[i].key);
            mm_node_free(n->map[i].value);
        }
        free(n->map);
    }
    free(n);
}

static mm_node_t *mm_clone(mm_node_t *src)
{
    if (!src)
        return NULL;
    mm_node_t *dst = mm_node_new();
    dst->type = src->type;
    if (src->scalar)
        dst->scalar = strdup(src->scalar);
    if (src->sequence && src->seq_count > 0)
    {
        dst->sequence = calloc(src->seq_count, sizeof(char *));
        for (size_t i = 0; i < src->seq_count; i++)
            dst->sequence[i] = strdup(src->sequence[i]);
        dst->seq_count = src->seq_count;
    }
    if (src->map && src->map_count > 0)
    {
        dst->map = calloc(src->map_count, sizeof(mm_entry_t));
        for (size_t i = 0; i < src->map_count; i++)
        {
            dst->map[i].key = strdup(src->map[i].key);
            dst->map[i].value = mm_clone(src->map[i].value);
        }
        dst->map_count = src->map_count;
    }
    return dst;
}

static mm_node_t *mm_find_map_entry(mm_node_t *n, const char *key)
{
    if (!n || n->type != MN_MAPPING)
        return NULL;
    for (size_t i = 0; i < n->map_count; i++)
    {
        if (strcmp(n->map[i].key, key) == 0)
            return n->map[i].value;
    }
    return NULL;
}

static bool mm_set_map_entry(mm_node_t **rootp, const char *key, mm_node_t *val_clone)
{
    if (!rootp || !key || !val_clone)
        return false;
    mm_node_t *root = *rootp;
    if (!root)
    {
        root = mm_node_new();
        root->type = MN_MAPPING;
        *rootp = root;
    }
    /* Replace if exists */
    for (size_t i = 0; i < root->map_count; i++)
    {
        if (strcmp(root->map[i].key, key) == 0)
        {
            mm_node_free(root->map[i].value);
            root->map[i].value = val_clone;
            return true;
        }
    }
    /* Append */
    mm_entry_t *tmp = realloc(root->map, (root->map_count + 1) * sizeof(mm_entry_t));
    if (!tmp)
    {
        return false;
    }
    root->map = tmp;
    root->map[root->map_count].key = strdup(key);
    root->map[root->map_count].value = val_clone;
    root->map_count++;
    return true;
}

static bool mm_append_sequence(mm_node_t **rootp, char **seq, size_t count)
{
    if (!rootp || !seq || count == 0)
        return false;
    mm_node_t *root = *rootp;
    if (!root)
    {
        root = mm_node_new();
        root->type = MN_SEQUENCE;
        *rootp = root;
    }
    if (root->type == MN_SCALAR || root->type == MN_MAPPING)
    {
        /* child replaces parent type */
        mm_node_free(root);
        root = mm_node_new();
        root->type = MN_SEQUENCE;
        *rootp = root;
    }
    char **tmp = realloc(root->sequence, (root->seq_count + count) * sizeof(char *));
    if (!tmp)
        return false;
    root->sequence = tmp;
    for (size_t i = 0; i < count; i++)
        root->sequence[root->seq_count + i] = strdup(seq[i]);
    root->seq_count += count;
    return true;
}

static bool mm_set_scalar(mm_node_t **rootp, const char *s)
{
    if (!rootp || !s)
        return false;
    mm_node_t *root = *rootp;
    if (!root)
    {
        root = mm_node_new();
        root->type = MN_SCALAR;
        root->scalar = strdup(s);
        *rootp = root;
        return true;
    }
    /* child scalar replaces existing content */
    if (root->scalar)
        free(root->scalar);
    if (root->sequence)
    {
        for (size_t i = 0; i < root->seq_count; i++)
            free(root->sequence[i]);
        free(root->sequence);
        root->sequence = NULL;
        root->seq_count = 0;
    }
    if (root->map)
    {
        for (size_t i = 0; i < root->map_count; i++)
        {
            free(root->map[i].key);
            mm_node_free(root->map[i].value);
        }
        free(root->map);
        root->map = NULL;
        root->map_count = 0;
    }
    root->type = MN_SCALAR;
    root->scalar = strdup(s);
    return true;
}

/* Merge mapping content from a yaml mapping node into mm_node (parent-first semantics provided by caller) */
static bool mm_merge_from_mapping(mm_node_t **rootp, yaml_document_t *doc, yaml_node_t *mapping_node)
{
    if (!mapping_node || mapping_node->type != YAML_MAPPING_NODE)
        return true; /* nothing to do */
    for (yaml_node_pair_t *pair = mapping_node->data.mapping.pairs.start; pair < mapping_node->data.mapping.pairs.top; pair++)
    {
        yaml_node_t *k = yaml_document_get_node(doc, pair->key);
        yaml_node_t *v = yaml_document_get_node(doc, pair->value);
        if (!k || k->type != YAML_SCALAR_NODE)
            continue;
        const char *key = (const char *)k->data.scalar.value;
        if (v->type == YAML_SCALAR_NODE)
        {
            mm_node_t *val = mm_node_new();
            val->type = MN_SCALAR;
            val->scalar = strdup((char *)v->data.scalar.value);
            mm_set_map_entry(rootp, key, val);
        }
        else if (v->type == YAML_SEQUENCE_NODE)
        {
            size_t c = v->data.sequence.items.top - v->data.sequence.items.start;
            char **arr = calloc(c, sizeof(char *));
            size_t idx = 0;
            for (yaml_node_item_t *it = v->data.sequence.items.start; it < v->data.sequence.items.top; it++)
            {
                yaml_node_t *itn = yaml_document_get_node(doc, *it);
                if (itn && itn->type == YAML_SCALAR_NODE)
                {
                    arr[idx++] = strdup((char *)itn->data.scalar.value);
                }
            }
            if (idx > 0)
            {
                mm_node_t *existing = mm_find_map_entry(*rootp, key);
                if (!existing)
                {
                    mm_node_t *seqn = mm_node_new();
                    seqn->type = MN_SEQUENCE;
                    seqn->sequence = arr;
                    seqn->seq_count = idx;
                    mm_set_map_entry(rootp, key, seqn);
                }
                else
                {
                    mm_append_sequence(&(existing), arr, idx);
                    for (size_t i = 0; i < idx; i++)
                        free(arr[i]);
                    free(arr);
                }
            }
            else
            {
                free(arr);
            }
        }
        else if (v->type == YAML_MAPPING_NODE)
        {
            mm_node_t *sub = mm_find_map_entry(*rootp, key);
            if (!sub)
            {
                sub = mm_node_new();
                sub->type = MN_MAPPING;
                mm_set_map_entry(rootp, key, sub);
            }
            mm_merge_from_mapping(&(sub), doc, v);
        }
    }
    return true;
}

/* Build YAML document from mm_node mapping at root */
static int build_yaml_node(yaml_document_t *doc, mm_node_t *n)
{
    if (!n)
        return 0;
    if (n->type == MN_SCALAR)
    {
        return yaml_document_add_scalar(doc, NULL, (yaml_char_t *)n->scalar, -1, YAML_PLAIN_SCALAR_STYLE);
    }
    else if (n->type == MN_SEQUENCE)
    {
        int seq = yaml_document_add_sequence(doc, NULL, YAML_BLOCK_SEQUENCE_STYLE);
        for (size_t i = 0; i < n->seq_count; i++)
        {
            int s = yaml_document_add_scalar(doc, NULL, (yaml_char_t *)n->sequence[i], -1, YAML_PLAIN_SCALAR_STYLE);
            yaml_document_append_sequence_item(doc, seq, s);
        }
        return seq;
    }
    else if (n->type == MN_MAPPING)
    {
        int map = yaml_document_add_mapping(doc, NULL, YAML_BLOCK_MAPPING_STYLE);
        for (size_t i = 0; i < n->map_count; i++)
        {
            int key = yaml_document_add_scalar(doc, NULL, (yaml_char_t *)n->map[i].key, -1, YAML_PLAIN_SCALAR_STYLE);
            int val = build_yaml_node(doc, n->map[i].value);
            yaml_document_append_mapping_pair(doc, map, key, val);
        }
        return map;
    }
    return 0;
}

yaml_document_t *yaml_merge_documents_multi(yaml_document_t **docs, size_t doc_count)
{
    if (!docs || doc_count == 0)
        return NULL;

    mm_node_t *root = NULL;
    for (size_t di = 0; di < doc_count; di++)
    {
        yaml_document_t *doc = docs[di];
        yaml_node_t *r = yaml_document_get_root_node(doc);
        if (!r || r->type != YAML_MAPPING_NODE)
            continue;
        mm_merge_from_mapping(&root, doc, r);
    }

    if (!root)
        return NULL;

    yaml_document_t *out = malloc(sizeof(yaml_document_t));
    if (!out)
    {
        mm_node_free(root);
        return NULL;
    }
    if (!yaml_document_initialize(out, NULL, NULL, NULL, 1, 1))
    {
        free(out);
        mm_node_free(root);
        return NULL;
    }

    int map = build_yaml_node(out, root);
    if (map == 0)
    {
        yaml_document_delete(out);
        free(out);
        mm_node_free(root);
        return NULL;
    }

    mm_node_free(root);
    return out;
}
