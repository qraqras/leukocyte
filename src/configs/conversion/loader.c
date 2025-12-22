#include <stdlib.h>
#include <string.h>
#include <yaml.h>
#include <stdbool.h>

#include "configs/config.h"
#include "configs/rule_config.h"
#include "configs/conversion/loader.h"
#include "configs/conversion/yaml_helpers.h"
#include "configs/discovery/inherit.h"
#include "configs/discovery/discover.h"
#include "rule_registry.h"
#include "util/string_array.h"

/**
 * @brief Apply an array of YAML documents (parent-first) to a leuko_config_t structure.
 * @param cfg Pointer to the leuko_config_t structure
 * @param docs Array of pointers to yaml_document_t structures
 * @param doc_count Number of documents in the docs array
 * @param err Pointer to a char buffer for error messages
 * @return true if successful, false otherwise
 */
bool leuko_config_apply_docs(leuko_config_t *cfg, yaml_document_t **docs, size_t doc_count, char **err)
{
    if (!docs || doc_count == 0 || !cfg)
    {
        return false;
    }

    const rule_registry_entry_t *registry = leuko_get_rule_registry();
    size_t registry_count = leuko_get_rule_registry_count();
    for (size_t i = 0; i < registry_count; i++)
    {
        const rule_registry_entry_t *entry = &registry[i];
        const char *full_name = entry->full_name;
        const char *category_name = entry->category_name;
        const char *rule_name = entry->rule_name;

        leuko_rule_config_t *rcfg = leuko_rule_config_get_by_index(cfg, i);
        if (!rcfg)
        {
            continue;
        }

        /* Merge `Enabled` */
        bool enabled = false;
        if (yaml_get_merged_rule_bool_multi(docs, doc_count, full_name, category_name, rule_name, LEUKO_CONFIG_KEY_ENABLED, &enabled))
        {
            rcfg->enabled = enabled;
        }

        /* Merge `Severity` */
        char *severity = NULL;
        if (yaml_get_merged_rule_scalar_multi(docs, doc_count, full_name, category_name, rule_name, LEUKO_CONFIG_KEY_SEVERITY, &severity))
        {
            leuko_severity_t s;
            if (leuko_severity_level_from_string(severity, &s))
            {
                rcfg->severity = s;
            }
            free(severity);
        }

        /* Merge `Include` */
        char **include = NULL;
        size_t include_count = 0;
        if (yaml_get_merged_rule_sequence_multi(docs, doc_count, full_name, category_name, rule_name, LEUKO_CONFIG_KEY_INCLUDE, &include, &include_count) && 0 < include_count)
        {
            if (!leuko_string_array_concat(&rcfg->include, &rcfg->include_count, include, include_count))
            {
                if (err)
                {
                    *err = strdup("allocation failure");
                }
                return false;
            }
        }

        /* Merge `Exclude` */
        char **exclude = NULL;
        size_t exclude_count = 0;
        if (yaml_get_merged_rule_sequence_multi(docs, doc_count, full_name, category_name, rule_name, LEUKO_CONFIG_KEY_EXCLUDE, &exclude, &exclude_count) && 0 < exclude_count)
        {
            if (!leuko_string_array_concat(&rcfg->exclude, &rcfg->exclude_count, exclude, exclude_count))
            {
                if (err)
                {
                    *err = strdup("allocation failure");
                }
                return false;
            }
        }

        /* Apply specific configuration */
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
 * @brief Apply a configuration file to a leuko_config_t structure (reads and resolves parents internally).
 * @param cfg Pointer to the leuko_config_t structure
 * @param path Path to the configuration file
 * @param err Pointer to a char buffer for error messages
 * @return true if successful, false otherwise
 */
bool leuko_config_apply_file(leuko_config_t *cfg, const char *path, char **err)
{
    if (!cfg || !path)
    {
        return false;
    }

    /* Delegate discovery to discovery module: get parent-first docs and ownership of base/parents */
    leuko_raw_config_t **raws = NULL;
    size_t raw_count = 0;
    int rc = leuko_config_collect_parent_docs_for_file(path, &raws, &raw_count, err);
    if (rc != 0)
    {
        return false;
    }

    /* Extract AllCops Include/Exclude sequences in parent-first order into cfg->all_*
     * Iterate raws (parent-first) directly to avoid an extra allocation of docs. */
    for (size_t i = 0; i < raw_count; i++)
    {
        yaml_document_t *doc = raws[i]->doc;
        yaml_node_t *root = yaml_document_get_root_node(doc);
        if (!root || root->type != YAML_MAPPING_NODE)
        {
            continue;
        }
        yaml_node_t *allcops = yaml_get_mapping_node(doc, root, LEUKO_ALL_COPS);
        if (!allcops)
        {
            continue;
        }
        char **inc = NULL;
        size_t inc_count = 0;
        if ((inc = yaml_get_mapping_sequence_values(doc, allcops, LEUKO_CONFIG_KEY_INCLUDE, &inc_count)) && inc_count > 0)
        {
            if (!leuko_string_array_concat(&cfg->all_include, &cfg->all_include_count, inc, inc_count))
            {
                if (err)
                    *err = strdup("allocation failure");
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
                leuko_raw_config_list_free(raws, raw_count);
                return false;
            }
        }
    }

    /* Allocate docs array only when needed for leuko_config_apply_docs */
    if (raw_count == 0)
    {
        return true; /* no docs to apply */
    }

    yaml_document_t **docs = NULL;
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

    bool ok = leuko_config_apply_docs(cfg, docs, raw_count, err);

    free(docs);
    leuko_raw_config_list_free(raws, raw_count);
    return ok;
}
