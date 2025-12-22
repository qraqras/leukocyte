#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include <stdbool.h>

#include "configs/rule_config.h"
#include "configs/config.h"
#include "configs/loader.h"
#include "configs/yaml_helpers.h"
#include "configs/discovery/raw_config.h"
#include "configs/discovery/inherit.h"
#include "io/file.h"
#include "rule_registry.h"
#include "util/string_array.h"

/**
 * @brief Apply a YAML document to a config_t structure.
 * @param doc Pointer to the yaml_document_t structure
 * @param cfg Pointer to the config_t structure
 * @param err Pointer to a char buffer for error messages
 * @return true if successful, false otherwise
 */

bool config_apply_docs(config_t *cfg, yaml_document_t **docs, size_t doc_count, char **err)
{
    if (!docs || doc_count == 0 || !cfg)
    {
        return false;
    }

    /* Note: docs are parent-first; merged helpers consult documents appropriately */

    const rule_registry_entry_t *registry = leuko_get_rule_registry();
    size_t registry_count = leuko_get_rule_registry_count();
    for (size_t i = 0; i < registry_count; i++)
    {
        const rule_registry_entry_t *entry = &registry[i];
        const char *category_name = entry->category_name;
        const char *rule_name = entry->rule_name;
        const char *full_name = entry->full_name;

        leuko_rule_config_t *rcfg = get_rule_config_by_index(cfg, i);
        if (!rcfg)
        {
            continue;
        }

        /* Merge `Enabled` (scalar overwrite across docs) */
        bool merged_enabled = false;
        if (yaml_get_merged_rule_bool_multi(docs, doc_count, full_name, category_name, rule_name, LEUKO_CONFIG_KEY_ENABLED, &merged_enabled))
        {
            rcfg->enabled = merged_enabled;
        }

        /* Merge `Severity` (scalar overwrite across docs) */
        char *sev = NULL;
        if (yaml_get_merged_rule_scalar_multi(docs, doc_count, full_name, category_name, rule_name, LEUKO_CONFIG_KEY_SEVERITY, &sev))
        {
            leuko_severity_t s;
            if (leuko_severity_level_from_string(sev, &s))
            {
                rcfg->severity = s;
            }
            free(sev);
        }

        /* Merge `Include` (array concat: parent first, then child) */
        char **inc = NULL;
        size_t inc_count = 0;
        if (yaml_get_merged_rule_sequence_multi(docs, doc_count, full_name, category_name, rule_name, LEUKO_CONFIG_KEY_INCLUDE, &inc, &inc_count) && 0 < inc_count)
        {
            if (!leuko_string_array_concat(&rcfg->include, &rcfg->include_count, inc, inc_count))
            {
                if (err)
                    *err = strdup("allocation failure");
                return false;
            }
        }

        /* Merge `Exclude` (array concat) */
        char **exc = NULL;
        size_t exc_count = 0;
        if (yaml_get_merged_rule_sequence_multi(docs, doc_count, full_name, category_name, rule_name, LEUKO_CONFIG_KEY_EXCLUDE, &exc, &exc_count) && 0 < exc_count)
        {
            if (!leuko_string_array_concat(&rcfg->exclude, &rcfg->exclude_count, exc, exc_count))
            {
                if (err)
                    *err = strdup("allocation failure");
                return false;
            }
        }

        /* Apply specific configuration via multi-document apply */
        if (entry->handlers && entry->handlers->apply_yaml)
        {
            if (!entry->handlers->apply_yaml(rcfg, docs, doc_count, full_name, category_name, rule_name, err))
            {
                if (!(err && *err))
                {
                    /* apply_yaml failed without an error message */
                }
                return false;
            }
        }
    }

    return true;
}

/**
 * @brief Load a configuration file into a config_t structure.
 * @param cfg Pointer to the config_t structure
 * @param path Path to the configuration file
 * @param err Pointer to a char buffer for error messages
 * @return true if successful, false otherwise
 */
bool config_apply_file(config_t *cfg, const char *path, char **err)
{
    if (!cfg || !path)
    {
        return false;
    }

    /* Delegate discovery to discovery module: get parent-first raws and docs; raws[last] is base */
    leuko_raw_config_t **raws = NULL;
    size_t raw_count = 0;
    yaml_document_t **docs = NULL;
    size_t doc_count = 0;
    int rc = leuko_config_collect_parent_docs_for_file(path, &raws, &raw_count, err);
    if (rc != 0)
    {
        return false;
    }
    /* Do not allocate docs yet; iterate raws directly for AllCops extraction to avoid an extra allocation */
    (void)docs; (void)doc_count;
    /* Extract AllCops Include/Exclude sequences in parent-first order into cfg->all_* */
    for (size_t di = 0; di < raw_count; di++)
    {
        yaml_document_t *doc = raws[di]->doc;
        yaml_node_t *root = yaml_document_get_root_node(doc);
        if (!root || root->type != YAML_MAPPING_NODE)
            continue;
        yaml_node_t *allcops = yaml_get_mapping_node(doc, root, LEUKO_ALL_COPS);
        if (!allcops)
            continue;
        char **inc = NULL;
        size_t inc_count = 0;
        if ((inc = yaml_get_mapping_sequence_values(doc, allcops, LEUKO_CONFIG_KEY_INCLUDE, &inc_count)) && inc_count > 0)
        {
            if (!leuko_string_array_concat(&cfg->all_include, &cfg->all_include_count, inc, inc_count))
            {
                if (err)
                    *err = strdup("allocation failure");
                free(docs);
                leuko_raw_config_list_free(raws, raw_count);
                return false;
            }
        }
        char **exc = NULL;
        size_t exc_count = 0;
        if ((exc = yaml_get_mapping_sequence_values(doc, allcops, LEUKO_CONFIG_KEY_EXCLUDE, &exc_count)) && exc_count > 0)
        {
            if (!leuko_string_array_concat(&cfg->all_exclude, &cfg->all_exclude_count, exc, exc_count))
            {
                if (err)
                    *err = strdup("allocation failure");
                free(docs);
                leuko_raw_config_list_free(raws, raw_count);
                return false;
            }
        }
    }

    /* Allocate docs only when calling config_apply_docs */
    if (raw_count == 0)
    {
        return true; /* nothing to apply */
    }
    docs = calloc(raw_count, sizeof(yaml_document_t *));
    if (!docs)
    {
        leuko_raw_config_list_free(raws, raw_count);
        if (err)
            *err = strdup("allocation failure");
        return false;
    }
    for (size_t i = 0; i < raw_count; i++)
        docs[i] = raws[i]->doc;

    bool ok = config_apply_docs(cfg, docs, raw_count, err);

    free(docs);
    leuko_raw_config_list_free(raws, raw_count);
    return ok;
}
