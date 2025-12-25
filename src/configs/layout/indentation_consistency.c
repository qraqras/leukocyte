#include <string.h>
#ifdef LEUKO_HAVE_LIBYAML
#include <yaml.h>
#endif
#include "configs/common/rule_config.h"
#include "configs/layout/indentation_consistency.h"
#include "common/registry/registry.h"
#include "sources/yaml/merge.h"

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
    leuko_rule_config_t *cfg = leuko_rule_config_initialize();
    if (!cfg)
    {
        free(specific_cfg);
        return NULL;
    }
    cfg->specific_config = specific_cfg;
    cfg->specific_config_free = layout_indentation_consistency_config_free;
    return cfg;
}

/* Multi-document apply for indentation rule (compat wrapper) */
#ifdef LEUKO_HAVE_LIBYAML
bool layout_indentation_consistency_apply(leuko_rule_config_t *config, yaml_document_t **docs, size_t doc_count, const char *full_name, const char *category_name, const char *rule_name, char **err)
{
    if (!config)
        return false;
    if (!docs || doc_count == 0)
        return true; /* nothing to apply */
    /* Create merged node from last document as a simple compatibility wrapper */
    leuko_yaml_node_t *merged = leuko_yaml_node_from_document(docs[doc_count - 1]);
    if (!merged)
        return false;
    bool res = layout_indentation_consistency_apply_merged(config, merged, full_name, category_name, rule_name, err);
    leuko_yaml_node_free(merged);
    return res;
}
#endif

/* New merged-node apply */
bool layout_indentation_consistency_apply_merged(leuko_rule_config_t *config, leuko_yaml_node_t *merged, const char *full_name, const char *category_name, const char *rule_name, char **err)
{
    if (err)
        *err = NULL;
    if (!config || !config->specific_config || !merged)
        return false;
    layout_indentation_consistency_config_t *sc = (layout_indentation_consistency_config_t *)config->specific_config;
    const char *val = leuko_yaml_node_get_rule_mapping_scalar(merged, full_name, CONFIG_KEY_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE);
    if (!val)
        return true; /* nothing to override */
    if (strcmp(val, CONFIG_VALUE_OF_LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS) == 0)
        sc->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS;
    else
        sc->enforced_style = LAYOUT_INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL;
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
#ifdef LEUKO_HAVE_LIBYAML
    .apply_yaml = layout_indentation_consistency_apply,
#endif
    .apply_merged = layout_indentation_consistency_apply_merged,
};
