#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include <stdbool.h>

#include "configs/config.h"
#include "configs/diagnostics.h"
#include "configs/generated_config.h"
#include "configs/loader.h"
#include "configs/yaml_helpers.h"
#include "io/file.h"
#include "rule_registry.h"

/// @brief Apply a YAML document to a config_t structure.
/// @param doc Pointer to the yaml_document_t structure
/// @param cfg Pointer to the config_t structure
/// @param diagnostics Pointer to a pm_list_t for diagnostics
/// @return true if successful, false otherwise
bool apply_config(yaml_document_t *doc, config_t *cfg, pm_list_t *diagnostics)
{
    if (!doc || !cfg)
    {
        return false;
    }

    yaml_node_t *root = yaml_document_get_root_node((yaml_document_t *)doc);
    if (!root || root->type != YAML_MAPPING_NODE)
    {
        return false;
    }

    /* `inherit_from` */
    yaml_node_t *inherit_from = yaml_find_mapping_value(doc, root, INHERIT_FROM);

    /* `AllCops` */
    yaml_node_t *allcops = yaml_find_mapping_value(doc, root, ALL_COPS);

    const rule_registry_entry_t *registry = get_rule_registry();
    size_t registry_count = get_rule_registry_count();
    for (size_t i = 0; i < registry_count; i++)
    {
        const rule_registry_entry_t *entry = &registry[i];
        const char *category_name = entry->category_name;
        const char *rule_name = entry->rule_name;
        const char *full_name = entry->full_name;

        /* Find the rule node in the YAML document:
         * 1. Try to find full name in top-level mapping
         *   Example:
         *   ```yaml
         *   Layout/AccessModifierIndentation:
         *     Enabled: true
         *   ```
         * 2. If not found, try to find category name, then rule name within it
         *  Example:
         *  ```yaml
         *  Layout:
         *   AccessModifierIndentation:
         *    Enabled: true
         *  ```
         */
        yaml_node_t *rule_node = yaml_find_mapping_value(doc, root, full_name);
        yaml_node_t *category_node = NULL;
        if (!rule_node && category_name)
        {
            yaml_node_t *cat = yaml_find_mapping_value(doc, root, category_name);
            if (cat)
            {
                category_node = cat;
                rule_node = yaml_find_mapping_value(doc, cat, rule_name);
            }
        }

        /* Get the rule configuration */
        rule_config_t *rcfg = get_rule_config_by_index(cfg, i);
        if (!rcfg)
        {
            continue;
        }

        /* Merge `Enabled` */
        bool merged_enabled = false;
        if (yaml_get_merged_enabled(doc, rule_node, category_node, allcops, &merged_enabled))
        {
            rcfg->enabled = merged_enabled;
        }

        /* Merge `Severity` */
        severity_level_t merged_severity = SEVERITY_CONVENTION;
        if (yaml_get_merged_severity(doc, rule_node, category_node, allcops, &merged_severity))
        {
            rcfg->severity_level = merged_severity;
        }

        /* Merge `Include` */
        char **inc = NULL;
        size_t inc_count = 0;
        if (yaml_get_merged_include(doc, rule_node, category_node, allcops, &inc, &inc_count) && 0 < inc_count)
        {
            rcfg->include = inc;
            rcfg->include_count = inc_count;
        }

        /* Merge `Exclude` */
        char **exc = NULL;
        size_t exc_count = 0;
        if (yaml_get_merged_exclude(doc, rule_node, category_node, allcops, &exc, &exc_count) && 0 < exc_count)
        {
            rcfg->exclude = exc;
            rcfg->exclude_count = exc_count;
        }

        /* Apply specific configuration */
        if (entry->ops && entry->ops->apply_yaml)
        {
            entry->ops->apply_yaml(rcfg, doc, rule_node, category_node, allcops, diagnostics);
        }
    }

    return true;
}

/// @brief Load a configuration file into a config_t structure.
/// @param cfg Pointer to the config_t structure
/// @param path Path to the configuration file
/// @param diagnostics Pointer to a pm_list_t for diagnostics
/// @return true if successful, false otherwise
bool load_config_file_into(config_t *cfg, const char *path, pm_list_t *diagnostics)
{
    if (!cfg || !path)
    {
        return false;
    }

    uint8_t *buf = NULL;
    size_t size = 0;
    char *read_err = NULL;
    if (!read_file_to_buffer(path, &buf, &size, &read_err))
    {
        if (diagnostics)
        {
            config_diagnostics_append(diagnostics, -1, -1, "Failed to read config file '%s': %s", path, read_err ? read_err : "unknown");
        }
        free(read_err);
        return false;
    }

    yaml_parser_t parser;
    yaml_document_t doc;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (const unsigned char *)buf, (size_t)size);
    if (!yaml_parser_load(&parser, &doc))
    {
        yaml_parser_delete(&parser);
        free(buf);
        return false;
    }

    bool ok = apply_config(&doc, cfg, diagnostics);

    yaml_document_delete(&doc);
    yaml_parser_delete(&parser);
    free(buf);
    return ok;
}
