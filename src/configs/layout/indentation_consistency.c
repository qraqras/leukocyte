#include <string.h>
#include <yaml.h>

#include "configs/layout/indentation_consistency.h"
#include "configs/yaml_helpers.h"
#include "rule_registry.h"

/**
 * @brief Initialize the layout_indentation_consistency rule configuration.
 * @return Pointer to the initialized rule_config_t structure
 */
rule_config_t *layout_indentation_consistency_initialize(void)
{
    /* Specific configuration */
    layout_indentation_consistency_config_t *specific_cfg = calloc(1, sizeof(*specific_cfg));
    if (!specific_cfg)
    {
        return NULL;
    }
    specific_cfg->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;

    /* Rule configuration */
    rule_config_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
    {
        free(specific_cfg);
        return NULL;
    }
    cfg->enabled = true;
    cfg->severity_level = LEUKO_SEVERITY_CONVENTION;
    cfg->include = NULL;
    cfg->include_count = 0;
    cfg->exclude = NULL;
    cfg->exclude_count = 0;
    cfg->specific_config = specific_cfg;
    cfg->specific_config_free = layout_indentation_consistency_config_free;
    return cfg;
}

/**
 * @brief Apply a YAML event to the layout_indentation_consistency rule configuration.
 * @param config Pointer to the rule_config_t structure
 * @param event Pointer to the yaml_event_t structure
 * @param parser Pointer to the pm_parser_t structure
 * @return true if the event was handled, false otherwise
 */
bool layout_indentation_consistency_apply(rule_config_t *config, const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, char **err)
{
    /* Keep existing single-doc behavior for backward compatibility */
    if (err)
        *err = NULL;
    if (!config || !config->specific_config)
        return false;
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)config->specific_config;
    char *enforced_style_value = yaml_get_mapping_scalar_value(doc, rule_node, CONFIG_KEY_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE);
    if (!enforced_style_value)
        return true; /* Nothing to set, not an error */
    if (strcmp(enforced_style_value, CONFIG_VALUE_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS) == 0)
        sc->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS;
    else
        sc->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;
    return true;
}

/* Multi-document apply for indentation rule */
bool layout_indentation_consistency_apply_multi(rule_config_t *config, yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, char **err)
{
    if (err)
        *err = NULL;
    if (!config || !config->specific_config)
        return false;
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)config->specific_config;
    char *val = yaml_get_merged_rule_scalar_multi(docs, doc_count, full_name, category_name, CONFIG_KEY_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE);
    if (!val)
        return true; /* nothing to override */
    if (strcmp(val, CONFIG_VALUE_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS) == 0)
        sc->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS;
    else
        sc->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;
    free(val);
    return true;
}

/**
 * @brief Free the memory allocated for a layout_indentation_consistency_config_t structure.
 * @param config Pointer to the layout_indentation_consistency_config_t structure to free
 */
void layout_indentation_consistency_config_free(void *config)
{
    if (!config)
    {
        return;
    }
    free(config);
}

/**
 * @brief Configuration operations for Layout/IndentationConsistency rule.
 */
struct config_ops layout_indentation_consistency_config_ops = {
    .initialize = layout_indentation_consistency_initialize,
    .apply_yaml = layout_indentation_consistency_apply,
    .apply_yaml_multi = layout_indentation_consistency_apply_multi,
};
