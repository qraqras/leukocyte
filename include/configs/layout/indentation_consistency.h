#ifndef LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H
#define LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H

#include "configs/config.h"
#include "configs/config_ops.h"

extern const struct config_ops layout_indentation_consistency_config_ops;

rule_config_t *layout_indentation_consistency_initialize(void);
bool layout_indentation_consistency_apply(rule_config_t *config, const yaml_event_t *event, pm_parser_t *parser);
void layout_indentation_consistency_config_free(void *config);

/// @brief Indentation consistency enforced styles.
/// @note default: INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL
typedef enum
{
    INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL,
    INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS,
} indentation_consistency_enforced_style_t;

/// @brief Specific configuration for Layout/IndentationConsistency rule.
typedef struct layout_indentation_consistency_config_s
{
    indentation_consistency_enforced_style_t enforced_style;
} layout_indentation_consistency_config_t;

#endif /* LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H */
