#ifndef LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_STYLE_H
#define LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_STYLE_H

#include "configs/common/rule_config.h"

#define CONFIG_KEY_OF_LAYOUT_INDENTATION_STYLE "EnforcedStyle"
#define CONFIG_VALUE_OF_LAYOUT_INDENTATION_STYLE_SPACES "space"
#define CONFIG_VALUE_OF_LAYOUT_INDENTATION_STYLE_TABS "tab"

extern struct leuko_rule_config_handlers_s layout_indentation_style_config_ops;

typedef enum
{
    LAYOUT_INDENTATION_STYLE_SPACES,
    LAYOUT_INDENTATION_STYLE_TABS,
} indentation_style_t;

typedef struct layout_indentation_style_config_s
{
    indentation_style_t style; /* default: spaces */
} layout_indentation_style_config_t;

void *layout_indentation_style_initialize(void);
/* New API: merged-node apply */
bool layout_indentation_style_apply_merged(void *config, leuko_node_t *node, char **err);
void layout_indentation_style_reset(void *view);

#endif /* LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_STYLE_H */
