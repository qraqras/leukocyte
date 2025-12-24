#include "sources/yaml/merge.h"
#include "leuko_debug.h"
#include "common/registry/registry.h"
#include <stdlib.h>
#include <string.h>

/* Check whether a category/subkey combination corresponds to a known rule in the registry */
static bool leuko_is_known_rule(const char *category, const char *rule)
{
    if (!category || !rule)
        return false;
    const rule_registry_entry_t *r = leuko_get_rule_registry();
    size_t n = leuko_get_rule_registry_count();
    for (size_t i = 0; i < n; i++)
    {
        if (strcmp(r[i].category_name, category) == 0 && strcmp(r[i].rule_name, rule) == 0)
            return true;
    }
    return false;
}

/* New implementation: in-memory leuko_yaml_node representation and merge.
 * This avoids emitter/parse roundtrips and returns a leuko_yaml_node tree
 * representing the merged configuration.
 */

yaml_node_t *leuko_yaml_find_mapping_value(yaml_document_t *doc, yaml_node_t *mapping, const char *key)
{
    if (!mapping || mapping->type != YAML_MAPPING_NODE)
        return NULL;
    size_t pairs = 0;
    if (mapping->data.mapping.pairs.start && mapping->data.mapping.pairs.top)
        pairs = (size_t)(mapping->data.mapping.pairs.top - mapping->data.mapping.pairs.start);
    for (size_t j = 0; j < pairs; j++)
    {
        yaml_node_pair_t p2 = mapping->data.mapping.pairs.start[j];
        yaml_node_t *k2 = yaml_document_get_node(doc, p2.key);
        yaml_node_t *v2 = yaml_document_get_node(doc, p2.value);
        if (!k2 || k2->type != YAML_SCALAR_NODE)
            continue;
        if (strcmp((char *)k2->data.scalar.value, key) == 0)
            return v2;
    }
    return NULL;
}

/* Simple dynamic-string duplicate */
static char *leuko_strdup(const char *s)
{
    if (!s)
        return NULL;
    size_t n = strlen(s) + 1;
    char *r = malloc(n);
    if (!r)
        return NULL;
    memcpy(r, s, n);
    return r;
}

struct leuko_yaml_node_s
{
    int type; /* enum leuko_yaml_node_type */
    /* scalar */
    char *scalar;
    /* sequence */
    struct leuko_yaml_node_s **seq_items;
    size_t seq_len;
    size_t seq_cap;
    /* mapping */
    char **map_keys;
    struct leuko_yaml_node_s **map_vals;
    size_t map_len;
    size_t map_cap;
    /* inherit_mode for this mapping (if any) */
    char **inherit_merge_keys;
    size_t inherit_merge_len;
    char **inherit_override_keys;
    size_t inherit_override_len;
};

static void leuko_node_init_mapping(leuko_yaml_node_t *n)
{
    n->type = LEUKO_YAML_NODE_MAPPING;
    n->map_keys = NULL;
    n->map_vals = NULL;
    n->map_len = 0;
    n->map_cap = 0;
    n->inherit_merge_keys = NULL;
    n->inherit_merge_len = 0;
    n->inherit_override_keys = NULL;
    n->inherit_override_len = 0;
}

static void leuko_node_init_sequence(leuko_yaml_node_t *n)
{
    n->type = LEUKO_YAML_NODE_SEQUENCE;
    n->seq_items = NULL;
    n->seq_len = 0;
    n->seq_cap = 0;
}

static leuko_yaml_node_t *leuko_node_new_scalar(const char *s)
{
    leuko_yaml_node_t *n = malloc(sizeof(leuko_yaml_node_t));
    if (!n)
        return NULL;
    n->type = LEUKO_YAML_NODE_SCALAR;
    n->scalar = leuko_strdup(s);
    n->seq_items = NULL;
    n->seq_len = n->seq_cap = 0;
    n->map_keys = NULL;
    n->map_vals = NULL;
    n->map_len = n->map_cap = 0;
    n->inherit_merge_keys = NULL;
    n->inherit_merge_len = 0;
    n->inherit_override_keys = NULL;
    n->inherit_override_len = 0;
    return n;
}

static leuko_yaml_node_t *leuko_node_new_sequence(void)
{
    leuko_yaml_node_t *n = malloc(sizeof(leuko_yaml_node_t));
    if (!n)
        return NULL;
    leuko_node_init_sequence(n);
    n->scalar = NULL;
    n->map_keys = NULL;
    n->map_vals = NULL;
    n->map_len = n->map_cap = 0;
    n->inherit_merge_keys = NULL;
    n->inherit_merge_len = 0;
    n->inherit_override_keys = NULL;
    n->inherit_override_len = 0;
    return n;
}

static leuko_yaml_node_t *leuko_node_new_mapping(void)
{
    leuko_yaml_node_t *n = malloc(sizeof(leuko_yaml_node_t));
    if (!n)
        return NULL;
    leuko_node_init_mapping(n);
    n->scalar = NULL;
    n->seq_items = NULL;
    n->seq_len = n->seq_cap = 0;
    return n;
}

static void leuko_node_sequence_append(leuko_yaml_node_t *seq, leuko_yaml_node_t *item)
{
    if (seq->seq_len + 1 > seq->seq_cap)
    {
        size_t ncap = seq->seq_cap ? seq->seq_cap * 2 : 4;
        leuko_yaml_node_t **n = realloc(seq->seq_items, ncap * sizeof(leuko_yaml_node_t *));
        if (!n)
            return; /* OOM - leak silent */
        seq->seq_items = n;
        seq->seq_cap = ncap;
    }
    seq->seq_items[seq->seq_len++] = item;
}

static void leuko_node_mapping_set(leuko_yaml_node_t *map, const char *key, leuko_yaml_node_t *val)
{
    /* replace if exists */
    for (size_t i = 0; i < map->map_len; i++)
    {
        if (strcmp(map->map_keys[i], key) == 0)
        {
            /* free old value */
            /* caller will own val; free existing */
            /* free old val */
            /* deep free existing value */
            /* implement later via leuko_yaml_node_free on existing */
            /* but to avoid double free in some paths, free here */
            leuko_yaml_node_t *old = map->map_vals[i];
            if (old)
            {
                leuko_yaml_node_free(old);
            }
            map->map_vals[i] = val;
            return;
        }
    }
    if (map->map_len + 1 > map->map_cap)
    {
        size_t ncap = map->map_cap ? map->map_cap * 2 : 4;
        char **nk = realloc(map->map_keys, ncap * sizeof(char *));
        if (!nk)
            return;
        map->map_keys = nk;
        leuko_yaml_node_t **nv = realloc(map->map_vals, ncap * sizeof(leuko_yaml_node_t *));
        if (!nv)
            return;
        map->map_vals = nv;
        map->map_cap = ncap;
    }
    map->map_keys[map->map_len] = leuko_strdup(key);
    map->map_vals[map->map_len] = val;
    map->map_len++;
}

/* Forward declaration for recursive free */
void leuko_yaml_node_free(leuko_yaml_node_t *n)
{
    if (!n)
        return;
    if (n->type == LEUKO_YAML_NODE_SCALAR)
    {
        free(n->scalar);
    }
    else if (n->type == LEUKO_YAML_NODE_SEQUENCE)
    {
        for (size_t i = 0; i < n->seq_len; i++)
            leuko_yaml_node_free(n->seq_items[i]);
        free(n->seq_items);
    }
    else if (n->type == LEUKO_YAML_NODE_MAPPING)
    {
        for (size_t i = 0; i < n->map_len; i++)
        {
            free(n->map_keys[i]);
            leuko_yaml_node_free(n->map_vals[i]);
        }
        free(n->map_keys);
        free(n->map_vals);
        for (size_t i = 0; i < n->inherit_merge_len; i++)
            free(n->inherit_merge_keys[i]);
        for (size_t i = 0; i < n->inherit_override_len; i++)
            free(n->inherit_override_keys[i]);
        free(n->inherit_merge_keys);
        free(n->inherit_override_keys);
    }
    free(n);
}

static void leuko_node_inherit_add(leuko_yaml_node_t *map, bool is_merge, const char *key)
{
    if (is_merge)
    {
        size_t need = map->inherit_merge_len + 1;
        char **n = realloc(map->inherit_merge_keys, need * sizeof(char *));
        if (!n)
            return;
        map->inherit_merge_keys = n;
        map->inherit_merge_keys[map->inherit_merge_len++] = leuko_strdup(key);
    }
    else
    {
        size_t need = map->inherit_override_len + 1;
        char **n = realloc(map->inherit_override_keys, need * sizeof(char *));
        if (!n)
            return;
        map->inherit_override_keys = n;
        map->inherit_override_keys[map->inherit_override_len++] = leuko_strdup(key);
    }
}

static bool leuko_node_inherit_has_item(leuko_yaml_node_t *map, bool is_merge, const char *key)
{
    if (!map)
        return false;
    if (is_merge)
    {
        for (size_t i = 0; i < map->inherit_merge_len; i++)
            if (strcmp(map->inherit_merge_keys[i], key) == 0)
                return true;
    }
    else
    {
        for (size_t i = 0; i < map->inherit_override_len; i++)
            if (strcmp(map->inherit_override_keys[i], key) == 0)
                return true;
    }
    return false;
}

/* Convert yaml_node_t -> leuko_yaml_node recursively */
static leuko_yaml_node_t *leuko_node_from_yaml_node(yaml_document_t *doc, yaml_node_t *yn)
{
    if (!yn)
        return NULL;
    /* Be careful reading yn->type (yn may be invalid) */
    int ytype = -1;
    if (yn)
        ytype = yn->type;
    if (ytype == YAML_SCALAR_NODE)
    {
        return leuko_node_new_scalar((char *)yn->data.scalar.value);
    }
    if (yn->type == YAML_SEQUENCE_NODE)
    {
        leuko_yaml_node_t *seq = leuko_node_new_sequence();
        size_t items = 0;
        if (yn->data.sequence.items.start && yn->data.sequence.items.top)
            items = (size_t)(yn->data.sequence.items.top - yn->data.sequence.items.start);
        for (size_t i = 0; i < items; i++)
        {
            yaml_node_t *it = yaml_document_get_node(doc, yn->data.sequence.items.start[i]);
            leuko_yaml_node_t *child = leuko_node_from_yaml_node(doc, it);
            leuko_node_sequence_append(seq, child);
        }
        return seq;
    }
    if (yn->type == YAML_MAPPING_NODE)
    {
        leuko_yaml_node_t *m = leuko_node_new_mapping();
        size_t pairs = 0;
        if (yn->data.mapping.pairs.start && yn->data.mapping.pairs.top)
            pairs = (size_t)(yn->data.mapping.pairs.top - yn->data.mapping.pairs.start);
        for (size_t i = 0; i < pairs; i++)
        {
            yaml_node_pair_t p = yn->data.mapping.pairs.start[i];
            yaml_node_t *k = yaml_document_get_node(doc, p.key);
            yaml_node_t *v = yaml_document_get_node(doc, p.value);
            if (!k || k->type != YAML_SCALAR_NODE)
                continue;
            const char *kstr = (char *)k->data.scalar.value;
            if (strcmp(kstr, LEUKO_CONFIG_KEY_INHERIT_MODE) == 0 && v && v->type == YAML_MAPPING_NODE)
            {
                /* parse inherit_mode mapping */
                yaml_node_t *merge_seq = leuko_yaml_find_mapping_value(doc, v, "merge");
                if (merge_seq && merge_seq->type == YAML_SEQUENCE_NODE)
                {
                    for (size_t j = 0; j < (size_t)merge_seq->data.sequence.items.top; j++)
                    {
                        yaml_node_t *it = yaml_document_get_node(doc, merge_seq->data.sequence.items.start[j]);
                        if (it && it->type == YAML_SCALAR_NODE)
                            leuko_node_inherit_add(m, true, (char *)it->data.scalar.value);
                    }
                }
                yaml_node_t *ov_seq = leuko_yaml_find_mapping_value(doc, v, "override");
                if (ov_seq && ov_seq->type == YAML_SEQUENCE_NODE)
                {
                    for (size_t j = 0; j < (size_t)ov_seq->data.sequence.items.top; j++)
                    {
                        yaml_node_t *it = yaml_document_get_node(doc, ov_seq->data.sequence.items.start[j]);
                        if (it && it->type == YAML_SCALAR_NODE)
                            leuko_node_inherit_add(m, false, (char *)it->data.scalar.value);
                    }
                }
                continue;
            }
            leuko_yaml_node_t *child = leuko_node_from_yaml_node(doc, v);
            leuko_node_mapping_set(m, kstr, child);
        }
        return m;
    }
    return NULL;
}

leuko_yaml_node_t *leuko_yaml_node_from_document(yaml_document_t *doc)
{
    if (!doc)
        return NULL;
    yaml_node_t *root = yaml_document_get_root_node(doc);
    return leuko_node_from_yaml_node(doc, root);
}

/* Forward declare deep-copy used by normalizer */
static leuko_yaml_node_t *leuko_yaml_node_deep_copy(leuko_yaml_node_t *n);

/* Normalize category->rule nested mappings into root full-name entries
 * Example:
 * Layout:
 *   TrailingWhitespace:
 *     Enabled: true
 *
 * becomes a top-level mapping key "Layout/TrailingWhitespace" with the same mapping value.
 */
void leuko_yaml_normalize_rule_keys(leuko_yaml_node_t *root)
{
    if (!root || root->type != LEUKO_YAML_NODE_MAPPING)
        return;

    /* Collect additions to avoid mutating map while iterating */
    size_t add_cap = 0;
    size_t add_len = 0;
    char **add_keys = NULL;
    leuko_yaml_node_t **add_vals = NULL;

    for (size_t i = 0; i < root->map_len; i++)
    {
        const char *cat = root->map_keys[i];
        leuko_yaml_node_t *catnode = root->map_vals[i];
        if (!catnode || catnode->type != LEUKO_YAML_NODE_MAPPING)
            continue;
        for (size_t j = 0; j < catnode->map_len; j++)
        {
            const char *subk = catnode->map_keys[j];
            /* Expand only if this subkey corresponds to a known rule (whitelist) */
            if (!leuko_is_known_rule(cat, subk))
                continue;
            /* Build full name */
            size_t ln = strlen(cat) + 1 + strlen(subk) + 1;
            char *fn = malloc(ln);
            if (!fn)
                continue; /* OOM - best-effort */
            snprintf(fn, ln, "%s/%s", cat, subk);
            /* If root already has explicit full-name entry, skip adding */
            if (leuko_yaml_node_get_mapping_child(root, fn))
            {
                free(fn);
                continue;
            }
            leuko_yaml_node_t *child_copy = leuko_yaml_node_deep_copy(catnode->map_vals[j]);
            if (!child_copy)
            {
                free(fn);
                continue;
            }
            if (add_len + 1 > add_cap)
            {
                size_t ncap = add_cap ? add_cap * 2 : 8;
                char **nk = realloc(add_keys, ncap * sizeof(char *));
                if (!nk)
                {
                    free(fn);
                    leuko_yaml_node_free(child_copy);
                    continue;
                }
                add_keys = nk;
                leuko_yaml_node_t **nv = realloc(add_vals, ncap * sizeof(leuko_yaml_node_t *));
                if (!nv)
                {
                    free(fn);
                    leuko_yaml_node_free(child_copy);
                    continue;
                }
                add_vals = nv;
                add_cap = ncap;
            }
            add_keys[add_len] = fn;
            add_vals[add_len] = child_copy;
            add_len++;
        }
    }

    /* Append collected additions into root mapping */
    for (size_t k = 0; k < add_len; k++)
    {
        leuko_node_mapping_set(root, add_keys[k], add_vals[k]);
        free(add_keys[k]);
    }
    free(add_keys);
    free(add_vals);
}

/* deep copy */
static leuko_yaml_node_t *leuko_yaml_node_deep_copy(leuko_yaml_node_t *n)
{
    if (!n)
        return NULL;
    if (n->type == LEUKO_YAML_NODE_SCALAR)
        return leuko_node_new_scalar(n->scalar);
    if (n->type == LEUKO_YAML_NODE_SEQUENCE)
    {
        leuko_yaml_node_t *r = leuko_node_new_sequence();
        for (size_t i = 0; i < n->seq_len; i++)
            leuko_node_sequence_append(r, leuko_yaml_node_deep_copy(n->seq_items[i]));
        return r;
    }
    if (n->type == LEUKO_YAML_NODE_MAPPING)
    {
        leuko_yaml_node_t *r = leuko_node_new_mapping();
        for (size_t i = 0; i < n->map_len; i++)
            leuko_node_mapping_set(r, n->map_keys[i], leuko_yaml_node_deep_copy(n->map_vals[i]));
        for (size_t i = 0; i < n->inherit_merge_len; i++)
            leuko_node_inherit_add(r, true, n->inherit_merge_keys[i]);
        for (size_t i = 0; i < n->inherit_override_len; i++)
            leuko_node_inherit_add(r, false, n->inherit_override_keys[i]);
        return r;
    }
    return NULL;
}

static bool leuko_should_merge_key(leuko_yaml_node_t *parent_map, leuko_yaml_node_t *child_map, leuko_yaml_node_t *child_root, const char *key)
{
    /* child cop-level */
    if (child_map && leuko_node_inherit_has_item(child_map, false, key))
        return false;
    if (child_map && leuko_node_inherit_has_item(child_map, true, key))
        return true;
    /* parent cop-level */
    if (parent_map && leuko_node_inherit_has_item(parent_map, false, key))
        return false;
    if (parent_map && leuko_node_inherit_has_item(parent_map, true, key))
        return true;
    /* child root-level */
    if (child_root && leuko_node_inherit_has_item(child_root, false, key))
        return false;
    if (child_root && leuko_node_inherit_has_item(child_root, true, key))
        return true;
    return false;
}

static leuko_yaml_node_t *leuko_merge_internal(leuko_yaml_node_t *parent, leuko_yaml_node_t *child, leuko_yaml_node_t *child_root);
/* forward declaration for functions used by normalizer */
static leuko_yaml_node_t *leuko_yaml_node_deep_copy(leuko_yaml_node_t *n);

/* Lookup helpers implementation */
leuko_yaml_node_t *leuko_yaml_node_get_mapping_child(leuko_yaml_node_t *root, const char *key)
{
    if (!root || root->type != LEUKO_YAML_NODE_MAPPING || !key)
        return NULL;
    for (size_t i = 0; i < root->map_len; i++)
    {
        if (strcmp(root->map_keys[i], key) == 0)
            return root->map_vals[i];
    }
    return NULL;
}

const char *leuko_yaml_node_get_mapping_scalar(leuko_yaml_node_t *map, const char *key)
{
    leuko_yaml_node_t *n = leuko_yaml_node_get_mapping_child(map, key);
    if (!n || n->type != LEUKO_YAML_NODE_SCALAR)
        return NULL;
    return n->scalar;
}

leuko_yaml_node_t *leuko_yaml_node_get_rule_mapping(leuko_yaml_node_t *root, const char *full_name)
{
    return leuko_yaml_node_get_mapping_child(root, full_name);
}

const char *leuko_yaml_node_get_rule_mapping_scalar(leuko_yaml_node_t *root, const char *full_name, const char *key)
{
    leuko_yaml_node_t *rule = leuko_yaml_node_get_rule_mapping(root, full_name);
    if (!rule || rule->type != LEUKO_YAML_NODE_MAPPING)
        return NULL;
    return leuko_yaml_node_get_mapping_scalar(rule, key);
}

/* Sequence helpers */
size_t leuko_yaml_node_sequence_count(leuko_yaml_node_t *seq)
{
    if (!seq || seq->type != LEUKO_YAML_NODE_SEQUENCE)
        return 0;
    return seq->seq_len;
}

const char *leuko_yaml_node_sequence_scalar_at(leuko_yaml_node_t *seq, size_t idx)
{
    if (!seq || seq->type != LEUKO_YAML_NODE_SEQUENCE)
        return NULL;
    if (idx >= seq->seq_len)
        return NULL;
    leuko_yaml_node_t *it = seq->seq_items[idx];
    if (!it || it->type != LEUKO_YAML_NODE_SCALAR)
        return NULL;
    return it->scalar;
}
static leuko_yaml_node_t *leuko_merge_mappings(leuko_yaml_node_t *pmap, leuko_yaml_node_t *cmap, leuko_yaml_node_t *croot)
{
    leuko_yaml_node_t *out = leuko_node_new_mapping();
    /* copy/merge parent keys */
    for (size_t i = 0; i < pmap->map_len; i++)
    {
        const char *k = pmap->map_keys[i];
        leuko_yaml_node_t *pv = pmap->map_vals[i];
        /* find in child */
        leuko_yaml_node_t *cv = NULL;
        for (size_t j = 0; j < cmap->map_len; j++)
            if (strcmp(cmap->map_keys[j], k) == 0)
            {
                cv = cmap->map_vals[j];
                break;
            }
        if (cv)
        {
            if (pv && pv->type == LEUKO_YAML_NODE_MAPPING && cv && cv->type == LEUKO_YAML_NODE_MAPPING)
            {
                leuko_yaml_node_t *m = leuko_merge_mappings(pv, cv, croot);
                leuko_node_mapping_set(out, k, m);
            }
            else if (pv && pv->type == LEUKO_YAML_NODE_SEQUENCE && cv && cv->type == LEUKO_YAML_NODE_SEQUENCE)
            {
                if (leuko_should_merge_key(pmap, cmap, croot, k))
                {
                    leuko_yaml_node_t *seq = leuko_node_new_sequence();
                    for (size_t ii = 0; ii < pv->seq_len; ii++)
                        leuko_node_sequence_append(seq, leuko_yaml_node_deep_copy(pv->seq_items[ii]));
                    for (size_t ii = 0; ii < cv->seq_len; ii++)
                        leuko_node_sequence_append(seq, leuko_yaml_node_deep_copy(cv->seq_items[ii]));
                    leuko_node_mapping_set(out, k, seq);
                }
                else
                {
                    leuko_node_mapping_set(out, k, leuko_yaml_node_deep_copy(cv));
                }
            }
            else
            {
                leuko_node_mapping_set(out, k, leuko_yaml_node_deep_copy(cv));
            }
        }
        else
        {
            leuko_node_mapping_set(out, k, leuko_yaml_node_deep_copy(pv));
        }
    }
    /* add child-only keys */
    for (size_t j = 0; j < cmap->map_len; j++)
    {
        const char *k = cmap->map_keys[j];
        bool found = false;
        for (size_t i = 0; i < pmap->map_len; i++)
            if (strcmp(pmap->map_keys[i], k) == 0)
            {
                found = true;
                break;
            }
        if (found)
            continue;
        leuko_node_mapping_set(out, k, leuko_yaml_node_deep_copy(cmap->map_vals[j]));
    }
    /* inherit_mode propagation: child's inherit settings at this mapping should be preserved
     * and parent's at this mapping could be carried too; here we simply copy child's and parent's
     */
    for (size_t i = 0; i < pmap->inherit_merge_len; i++)
        leuko_node_inherit_add(out, true, pmap->inherit_merge_keys[i]);
    for (size_t i = 0; i < pmap->inherit_override_len; i++)
        leuko_node_inherit_add(out, false, pmap->inherit_override_keys[i]);
    for (size_t i = 0; i < cmap->inherit_merge_len; i++)
        leuko_node_inherit_add(out, true, cmap->inherit_merge_keys[i]);
    for (size_t i = 0; i < cmap->inherit_override_len; i++)
        leuko_node_inherit_add(out, false, cmap->inherit_override_keys[i]);
    return out;
}

static leuko_yaml_node_t *leuko_merge_internal(leuko_yaml_node_t *parent, leuko_yaml_node_t *child, leuko_yaml_node_t *child_root)
{
    if (!parent)
        return leuko_yaml_node_deep_copy(child);
    if (!child)
        return leuko_yaml_node_deep_copy(parent);
    if (parent->type == LEUKO_YAML_NODE_MAPPING && child->type == LEUKO_YAML_NODE_MAPPING)
    {
        return leuko_merge_mappings(parent, child, child_root);
    }
    if (parent->type == LEUKO_YAML_NODE_SEQUENCE && child->type == LEUKO_YAML_NODE_SEQUENCE)
    {
        /* default to concatenation */
        leuko_yaml_node_t *seq = leuko_node_new_sequence();
        for (size_t i = 0; i < parent->seq_len; i++)
            leuko_node_sequence_append(seq, leuko_yaml_node_deep_copy(parent->seq_items[i]));
        for (size_t i = 0; i < child->seq_len; i++)
            leuko_node_sequence_append(seq, leuko_yaml_node_deep_copy(child->seq_items[i]));
        return seq;
    }
    /* otherwise child's value wins */
    return leuko_yaml_node_deep_copy(child);
}

bool leuko_yaml_merge_nodes(leuko_yaml_node_t *parent, leuko_yaml_node_t *child, leuko_yaml_node_t **out_merged)
{
    if (!out_merged)
        return false;
    if (!child)
    {
        *out_merged = leuko_yaml_node_deep_copy(parent);
        return true;
    }
    /* child_root is the top-level mapping of child (if mapping) used for inherit_mode root-level checks */
    leuko_yaml_node_t *child_root = child->type == LEUKO_YAML_NODE_MAPPING ? child : NULL;
    *out_merged = leuko_merge_internal(parent, child, child_root);
    return (*out_merged) ? true : false;
}
