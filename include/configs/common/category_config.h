#ifndef LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H
#define LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H

#include <stddef.h>
#include <regex.h>
#include <stdbool.h>
#include "common/registry/list.h"
#include "common/rule_registry.h"
#include "configs/common/rule_config.h"

/**
 * @brief Category configuration base structure.
 */
typedef struct leuko_config_category_base_s
{
    char *name;
    char **include;
    size_t include_count;
    char **exclude;
    size_t exclude_count;
    /* Precompiled regex patterns */
    regex_t *include_re;
    size_t include_re_count;
    regex_t *exclude_re;
    size_t exclude_re_count;
} leuko_config_category_base_t;

/**
 * @brief Category configuration view structure.
 */
#undef X
#define X(field, cat_name, sname, rule_ptr, ops_ptr, specific_t) \
    typedef struct leuko_config_rule_view_##field##_s            \
    {                                                            \
        leuko_config_rule_base_t base;                           \
        specific_t specific;                                     \
    } leuko_config_rule_view_##field##_t;
LEUKO_RULES_LAYOUT
#undef X

/**
 * @brief Category configuration view rules structure.
 */
typedef struct leuko_config_category_view_rules_s
{
#undef X
#define X(field, cat_name, sname, rule_ptr, ops_ptr, specific_t) leuko_config_rule_view_##field##_t field;
    LEUKO_RULES_LAYOUT
#undef X
} leuko_config_category_view_rules_t;

/**
 * @brief Category configuration view structure including base and rules.
 */
typedef struct leuko_config_category_view_s
{
    leuko_config_category_base_t base;
    leuko_config_category_view_rules_t rules;
} leuko_config_category_view_t;



/**
 * @brief Configuration structure holding all categories' views.
 */
typedef struct leuko_config_categories_view_s
{
#undef CATEGORY
#define CATEGORY(field, cat_name_macro, cat_rules_macro) leuko_config_category_view_t field;
    LEUKO_RULES_CATEGORIES
#undef CATEGORY
} leuko_config_categories_view_t;

/* ------------------------------------------------------------------------- */
/* Typed accessors for rule views (no cast required)
 * These inline helpers return pointers to the generated typed rule views so
 * callers can access `.specific` fields without manual casts.
 *
 * Note: to avoid incomplete-type issues this API accepts a pointer to the
 * `leuko_config_categories_view_t` (i.e. `&cfg->categories`).
 */
static inline leuko_config_rule_view_indentation_consistency_t *
leuko_config_categories_view_rule_indentation_consistency(leuko_config_categories_view_t *cats)
{
    return &cats->layout.rules.indentation_consistency;
}

static inline const leuko_config_rule_view_indentation_consistency_t *
leuko_config_categories_view_rule_indentation_consistency_const(const leuko_config_categories_view_t *cats)
{
    return &cats->layout.rules.indentation_consistency;
}

static inline leuko_config_rule_view_indentation_width_t *
leuko_config_categories_view_rule_indentation_width(leuko_config_categories_view_t *cats)
{
    return &cats->layout.rules.indentation_width;
}

static inline const leuko_config_rule_view_indentation_width_t *
leuko_config_categories_view_rule_indentation_width_const(const leuko_config_categories_view_t *cats)
{
    return &cats->layout.rules.indentation_width;
}

static inline leuko_config_rule_view_indentation_style_t *
leuko_config_categories_view_rule_indentation_style(leuko_config_categories_view_t *cats)
{
    return &cats->layout.rules.indentation_style;
}

static inline const leuko_config_rule_view_indentation_style_t *
leuko_config_categories_view_rule_indentation_style_const(const leuko_config_categories_view_t *cats)
{
    return &cats->layout.rules.indentation_style;
}

static inline leuko_config_rule_view_line_length_t *
leuko_config_categories_view_rule_line_length(leuko_config_categories_view_t *cats)
{
    return &cats->layout.rules.line_length;
}

static inline const leuko_config_rule_view_line_length_t *
leuko_config_categories_view_rule_line_length_const(const leuko_config_categories_view_t *cats)
{
    return &cats->layout.rules.line_length;
}

typedef struct leuko_config_s leuko_config_t;
typedef struct leuko_node_s leuko_node_t;

leuko_config_category_base_t *leuko_category_config_initialize(const char *name);
void leuko_category_config_free(leuko_config_category_base_t *cfg);
void leuko_category_config_reset(leuko_config_category_base_t *cfg);
bool leuko_category_config_apply(leuko_config_t *cfg, const char *name, leuko_node_t *cnode);

#endif /* LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H */
