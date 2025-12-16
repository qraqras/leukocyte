#include <yaml.h>

#include "rule_registry.h"
#include "configs/layout/indentation_consistency.h"
#include "configs/yaml_helpers.h"
#include <string.h>

/// @brief Initialize the layout_indentation_consistency rule configuration.
/// @return Pointer to the initialized rule_config_t structure
rule_config_t *layout_indentation_consistency_initialize(void)
{
    /* Specific configuration */
    layout_indentation_consistency_config_t *specific_cfg = calloc(1, sizeof(*specific_cfg));
    if (!specific_cfg)
    {
        free(specific_cfg);
        return NULL;
    }
    specific_cfg->enforced_style = INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;

    /* Rule configuration */
    rule_config_t *cfg = calloc(1, sizeof(*cfg));
    if (!cfg)
        return NULL;
    cfg->enabled = true;
    cfg->severity_level = SEVERITY_CONVENTION;
    cfg->include = NULL;
    cfg->include_count = 0;
    cfg->exclude = NULL;
    cfg->exclude_count = 0;
    cfg->specific_config = specific_cfg;
    cfg->specific_config_free = layout_indentation_consistency_config_free;
    return cfg;
}

/// @brief Apply a YAML event to the layout_indentation_consistency rule configuration.
/// @param config Pointer to the rule_config_t structure
/// @param event Pointer to the yaml_event_t structure
/// @param parser Pointer to the pm_parser_t structure
/// @return true if the event was handled, false otherwise
bool layout_indentation_consistency_apply(rule_config_t *config, const yaml_document_t *doc, yaml_node_t *rule_node, yaml_node_t *category_node, yaml_node_t *allcops_node, pm_list_t *diagnostics)
{
    if (!config || !config->specific_config)
        return false;

    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)config->specific_config;

    char val[256];
    bool merged = yaml_get_merged_string(doc, rule_node, category_node, allcops_node, "EnforcedStyle", val);
    if (!merged)
        return true; /* nothing to do, but considered handled */

    if (strcmp(val, "indented_internal_methods") == 0)
        sc->enforced_style = INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS;
    else
        sc->enforced_style = INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;

    return true;
}

/// @brief Free the memory allocated for a layout_indentation_consistency_config_t structure.
/// @param config Pointer to the layout_indentation_consistency_config_t structure to free
void layout_indentation_consistency_config_free(void *config)
{
    if (!config)
        return;
    free(config);
}

/// @brief Configuration operations for Layout/IndentationConsistency rule.
const struct config_ops layout_indentation_consistency_config_ops = {
    .initialize = layout_indentation_consistency_initialize,
    .apply_yaml = layout_indentation_consistency_apply,
};
