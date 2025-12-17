#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "configs/yaml_helpers.h"
#include "severity.h"
#include "configs/config.h"

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
            arr[idx++] = strdup((char *)v->data.scalar.value);
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
yaml_node_t *yaml_get_mapping_node(yaml_document_t *doc, yaml_node_t *mapping_node, const char *key)
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
 *         `strdup`) or use `yaml_get_merged_string` which returns an allocated copy.
 */
char *yaml_get_mapping_scalar_value(yaml_document_t *doc, yaml_node_t *mapping_node, const char *key)
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
char **yaml_get_mapping_sequence_values(yaml_document_t *doc, yaml_node_t *mapping_node, const char *key, size_t *count)
{
    if (!count)
    {
        return NULL;
    }

    yaml_node_t *node = yaml_get_mapping_node(doc, mapping_node, key);
    if (node && node->type == YAML_SEQUENCE_NODE)
    {
        char **arr = sequence_to_array(doc, node, count);
        if (arr)
        {
            return arr;
        }
    }
    *count = 0;
    return NULL;
}

/**
 * @brief Get a merged string value from rule, category, and allcops nodes.
 * @param doc Pointer to the yaml_document_t structure
 * @param rule_node Pointer to the rule yaml_node_t structure
 * @param category_node Pointer to the category yaml_node_t structure
 * @param allcops_node Pointer to the allcops yaml_node_t structure
 * @param key The key to search for
 * @param out Pointer to output string buffer
 * @return true if found, false otherwise
 */
bool yaml_get_merged_string(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, char *out)
{
    const char *val = NULL;
    if (rule_node)
    {
        val = yaml_get_mapping_scalar_value(doc, rule_node, key);
    }
    if (!val && category_node)
    {
        val = yaml_get_mapping_scalar_value(doc, category_node, key);
    }
    if (!val && allcops_node)
    {
        val = yaml_get_mapping_scalar_value(doc, allcops_node, key);
    }

    if (!val)
    {
        return false;
    }

    strcpy(out, val);
    return true;
}

/**
 * @brief Get a merged boolean value from rule, category, and allcops nodes.
 * @param doc Pointer to the yaml_document_t structure
 * @param rule_node Pointer to the rule yaml_node_t structure
 * @param category_node Pointer to the category yaml_node_t structure
 * @param allcops_node Pointer to the allcops yaml_node_t structure
 * @param key The key to search for
 * @param out Pointer to output boolean variable
 * @return true if found, false otherwise
 */
bool yaml_get_merged_bool(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, bool *out)
{
    const char *s = NULL;
    if (rule_node)
    {
        s = yaml_get_mapping_scalar_value(doc, rule_node, key);
    }
    if (!s && category_node)
    {
        s = yaml_get_mapping_scalar_value(doc, category_node, key);
    }
    if (!s && allcops_node)
    {
        s = yaml_get_mapping_scalar_value(doc, allcops_node, key);
    }
    if (!s)
    {
        return false;
    }
    /* RuboCop-compatible: accept common YAML boolean forms case-insensitively.
     * Accept true:  true, 1, yes, on, y
     * Accept false: false, 0, no, off, n
     * If node has YAML_BOOL_TAG it is still handled via the same token mapping.
     */
    if (strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0 || strcasecmp(s, "yes") == 0 || strcasecmp(s, "on") == 0 || strcasecmp(s, "y") == 0)
    {
        *out = true;
    }
    else if (strcasecmp(s, "false") == 0 || strcmp(s, "0") == 0 || strcasecmp(s, "no") == 0 || strcasecmp(s, "off") == 0 || strcasecmp(s, "n") == 0)
    {
        *out = false;
    }
    else
    {
        /* Unknown scalar for boolean: treat as false but report presence */
        *out = false;
    }
    return true;
}

/**
 * @brief Get a merged sequence of strings from rule, category, and allcops nodes.
 * @param doc Pointer to the yaml_document_t structure
 * @param rule_node Pointer to the rule yaml_node_t structure
 * @param category_node Pointer to the category yaml_node_t structure
 * @param allcops_node Pointer to the allcops yaml_node_t structure
 * @param key The key to search for
 * @param out Pointer to output array of strings
 * @param count Pointer to output count of elements
 * @return true if any scalar elements were merged, false otherwise
 */
bool yaml_get_merged_sequence(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, const char *key, char ***out, size_t *count)
{
    if (!count || !out)
    {
        return false;
    }

    char **acc = NULL;
    size_t total = 0;

    size_t cnt = 0;
    char **arr = NULL;
    arr = yaml_get_mapping_sequence_values(doc, allcops_node, key, &cnt);
    if (arr)
    {
        acc = realloc(acc, (total + cnt) * sizeof(char *));
        for (size_t i = 0; i < cnt; i++)
        {
            acc[total++] = arr[i];
        }
        free(arr);
    }
    arr = yaml_get_mapping_sequence_values(doc, category_node, key, &cnt);
    if (arr)
    {
        acc = realloc(acc, (total + cnt) * sizeof(char *));
        for (size_t i = 0; i < cnt; i++)
        {
            acc[total++] = arr[i];
        }
        free(arr);
    }
    arr = yaml_get_mapping_sequence_values(doc, rule_node, key, &cnt);
    if (arr)
    {
        acc = realloc(acc, (total + cnt) * sizeof(char *));
        for (size_t i = 0; i < cnt; i++)
        {
            acc[total++] = arr[i];
        }
        free(arr);
    }

    if (!acc)
    {
        *count = 0;
        *out = NULL;
        return false;
    }
    *count = total;
    *out = acc;
    return true;
}

/**
 * @brief Get merged Enabled (rule -> category -> allcops).
 * @param doc Pointer to the yaml_document_t structure
 * @param rule_node Pointer to the rule yaml_node_t structure
 * @param category_node Pointer to the category yaml_node_t structure
 * @param allcops_node Pointer to the allcops yaml_node_t structure
 * @param out Pointer to a bool to store the result
 * @return true if a boolean was successfully retrieved, false otherwise
 */
bool yaml_get_merged_enabled(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, bool *out)
{
    return yaml_get_merged_bool(doc, rule_node, category_node, allcops_node, CONFIG_KEY_ENABLED, out);
}

/**
 * @brief Get merged Severity (rule -> category -> allcops).
 * @param doc Pointer to the yaml_document_t structure
 * @param rule_node Pointer to the rule yaml_node_t structure
 * @param category_node Pointer to the category yaml_node_t structure
 * @param allcops_node Pointer to the allcops yaml_node_t structure
 * @param out Pointer to a severity_level_t to store the result
 * @return true if a severity level was successfully retrieved, false otherwise
 */
bool yaml_get_merged_severity(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, severity_level_t *out)
{
    // First get the string value
    char buf[64];
    if (!yaml_get_merged_string(doc, rule_node, category_node, allcops_node, CONFIG_KEY_SEVERITY, buf))
    {
        return false;
    }
    return severity_level_from_string(buf, out);
}

/**
 * @brief Get merged Include (rule -> category -> allcops).
 * @param doc Pointer to the yaml_document_t structure
 * @param rule_node Pointer to the rule yaml_node_t structure
 * @param category_node Pointer to the category yaml_node_t structure
 * @param allcops_node Pointer to the allcops yaml_node_t structure
 * @param out Pointer to a char** to store the result
 * @param count Pointer to a size_t to store the count
 * @return true if a sequence was successfully retrieved, false otherwise
 */
bool yaml_get_merged_include(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char ***out, size_t *count)
{
    return yaml_get_merged_sequence(doc, rule_node, category_node, allcops_node, CONFIG_KEY_INCLUDE, out, count);
}

/**
 * @brief Get merged Exclude (rule -> category -> allcops).
 * @param doc Pointer to the yaml_document_t structure
 * @param rule_node Pointer to the rule yaml_node_t structure
 * @param category_node Pointer to the category yaml_node_t structure
 * @param allcops_node Pointer to the allcops yaml_node_t structure
 * @param out Pointer to a char** to store the result
 * @param count Pointer to a size_t to store the count
 * @return true if a sequence was successfully retrieved, false otherwise
 */
bool yaml_get_merged_exclude(yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char ***out, size_t *count)
{
    return yaml_get_merged_sequence(doc, rule_node, category_node, allcops_node, CONFIG_KEY_EXCLUDE, out, count);
}
