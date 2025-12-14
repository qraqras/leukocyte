#include "configs/config_defaults.h"

static const indentation_consistency_t indentation_consistency_default = {
    .enforced_style = INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL,
};

const struct file_t config_defaults = {
    .IndentationConsistency_common = {.enabled = true, .severity = 1, .include = NULL, .include_count = 0, .exclude = NULL, .exclude_count = 0},
    .IndentationConsistency = (indentation_consistency_t *)&indentation_consistency_default,
};
