/*
 * Layout/IndentationConsistency
 */

#ifndef LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H
#define LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H

typedef enum
{
    INDENTATION_CONSISTENCY_ENFORCED_STYLE_NORMAL,
    INDENTATION_CONSISTENCY_ENFORCED_STYLE_INDENTED_INTERNAL_METHODS,
} indentation_consistency_enforced_style_t;

typedef struct indentation_consistency_s
{
    indentation_consistency_enforced_style_t enforced_style;
} indentation_consistency_t;

#endif // LEUKOCYTE_CONFIGS_LAYOUT_INDENTATION_CONSISTENCY_H
