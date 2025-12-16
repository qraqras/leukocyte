#include "configs/loader.h"
#include "configs/yaml_helpers.h"
#include "rule_registry.h"
#include "configs/config.h"
#include "configs/generated_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

static yaml_node_t *find_mapping_value(const yaml_document_t *doc, yaml_node_t *mapping, const char *key)
{
    if (!mapping || mapping->type != YAML_MAPPING_NODE)
        return NULL;
    for (yaml_node_pair_t *pair = mapping->data.mapping.pairs.start; pair < mapping->data.mapping.pairs.top; pair++)
    {
        yaml_node_t *k = yaml_document_get_node((yaml_document_t *)doc, pair->key);
        if (k && k->type == YAML_SCALAR_NODE && strcasecmp((char *)k->data.scalar.value, key) == 0)
            return yaml_document_get_node((yaml_document_t *)doc, pair->value);
    }
    return NULL;
}

bool config_apply_document(const yaml_document_t *doc, pm_list_t *diagnostics)
{
    if (!doc)
        return false;

    /* Ensure registry/global config is initialized */
    config_initialize(NULL);

    yaml_node_t *root = yaml_document_get_root_node((yaml_document_t *)doc);
    if (!root || root->type != YAML_MAPPING_NODE)
        return false;

    yaml_node_t *allcops = find_mapping_value(doc, root, "AllCops");

    const rule_registry_entry_t *registry = get_rule_registry();
    size_t count = get_rule_registry_count();

    for (size_t i = 0; i < count; i++)
    {
        const rule_registry_entry_t *entry = &registry[i];
        const char *fullname = entry->rule_name;
        // split fullname into category/shortname if contains '/'
        char *slash = strchr((char *)fullname, '/');
        char short_name[128] = {0};
        const char *category_name = NULL;
        if (slash)
        {
            category_name = strndup(fullname, slash - fullname);
            strncpy(short_name, slash + 1, sizeof(short_name) - 1);
        }

        // find rule node: try top-level fullname key
        yaml_node_t *rule_node = find_mapping_value(doc, root, fullname);
        yaml_node_t *category_node = NULL;
        if (!rule_node && category_name)
        {
            yaml_node_t *cat = find_mapping_value(doc, root, category_name);
            if (cat)
            {
                category_node = cat;
                rule_node = find_mapping_value(doc, cat, short_name);
            }
            free((void *)category_name);
        }

        // get the config instance
        rule_config_t *cfg = config_get_by_index(i);
        if (!cfg)
            continue;

        // central common keys
        int enabled = 0;
        if (yaml_get_merged_bool(doc, rule_node, category_node, allcops, "Enabled", &enabled))
            cfg->enabled = enabled ? true : false;

        char *sev = yaml_get_merged_string(doc, rule_node, category_node, allcops, "Severity");
        if (sev)
        {
            if (strcmp(sev, "convention") == 0)
                cfg->severity_level = SEVERITY_CONVENTION;
            else if (strcmp(sev, "warning") == 0)
                cfg->severity_level = SEVERITY_WARNING;
            else if (strcmp(sev, "error") == 0)
                cfg->severity_level = SEVERITY_ERROR;
            free(sev);
        }

        size_t inc_count = 0;
        char **inc = yaml_get_merged_sequence(doc, rule_node, category_node, allcops, "Include", &inc_count);
        if (inc)
        {
            // free old includes
            if (cfg->include)
            {
                for (size_t k = 0; k < cfg->include_count; k++)
                    free(cfg->include[k]);
                free(cfg->include);
            }
            cfg->include = inc;
            cfg->include_count = inc_count;
        }

        size_t exc_count = 0;
        char **exc = yaml_get_merged_sequence(doc, rule_node, category_node, allcops, "Exclude", &exc_count);
        if (exc)
        {
            if (cfg->exclude)
            {
                for (size_t k = 0; k < cfg->exclude_count; k++)
                    free(cfg->exclude[k]);
                free(cfg->exclude);
            }
            cfg->exclude = exc;
            cfg->exclude_count = exc_count;
        }

        // delegate to rule-specific apply
        if (entry->ops && entry->ops->apply_yaml)
            entry->ops->apply_yaml(cfg, doc, rule_node, category_node, allcops, diagnostics);
    }

    return true;
}

bool config_load_file(const char *path, pm_list_t *diagnostics)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return false;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(size + 1);
    if (!buf)
    {
        fclose(f);
        return false;
    }
    fread(buf, 1, size, f);
    buf[size] = '\0';
    fclose(f);

    yaml_parser_t parser;
    yaml_document_t doc;
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_string(&parser, (const unsigned char *)buf, size);
    if (!yaml_parser_load(&parser, &doc))
    {
        yaml_parser_delete(&parser);
        free(buf);
        return false;
    }

    bool ok = config_apply_document(&doc, diagnostics);

    yaml_document_delete(&doc);
    yaml_parser_delete(&parser);
    free(buf);
    return ok;
}
