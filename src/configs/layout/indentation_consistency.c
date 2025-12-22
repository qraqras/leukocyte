#include <string.h>
#include <yaml.h>

#include "configs/layout/indentation_consistency.h"
#include "configs/conversion/yaml_helpers.h"
#include "rule_registry.h"

/**
 * @brief Initialize the layout_indentation_consistency rule configuration.
 * @return Pointer to the initialized leuko_rule_config_t structure
 */
leuko_rule_config_t *layout_indentation_consistency_initialize(void)
{
    /* Specific configuration */
    layout_indentation_consistency_config_t *specific_cfg = calloc(1, sizeof(*specific_cfg));
    if (!specific_cfg)
    {
        return NULL;
    }
    specific_cfg->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;

    /* Rule configuration */
    leuko_rule_config_t *cfg = calloc(1, sizeof(*cfg));
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

/* Multi-document apply for indentation rule */
bool layout_indentation_consistency_apply_multi(leuko_rule_config_t *config, yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, char **err)
{
    if (err)
        *err = NULL;
    if (!config || !config->specific_config)
        return false;
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)config->specific_config;
    char *val = NULL;
    if (!yaml_get_merged_rule_scalar_multi(docs, doc_count, full_name, category_name, rule_name, CONFIG_KEY_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE, &val))
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
struct leuko_rule_config_handlers_s layout_indentation_consistency_config_ops = {
    .initialize = layout_indentation_consistency_initialize,
    .apply_yaml = layout_indentation_consistency_apply_multi,
};
