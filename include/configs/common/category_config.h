#ifndef LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H
#define LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H

#include <stddef.h>

/* Per-category configuration (e.g., Layout) */
#include <regex.h>
#include <stdbool.h>
#include "configs/common/rule_config.h"
#include "common/rule_registry.h"



typedef struct leuko_config_category_s
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
} leuko_config_category_t;

/* Generated view types for category rules (macro-driven). Placed here so
 * category-related definitions are colocated with `leuko_config_category_t`.
 */
#include "common/registry/list.h"

typedef struct leuko_config_category_view_rules_s
{
#undef X
#define X(field, cat_name, sname, rule_ptr, ops_ptr) leuko_config_rule_view_t field;
    LEUKO_RULES_LAYOUT
#undef X
} leuko_config_category_view_rules_t;

typedef struct leuko_config_category_view_s
{
    leuko_config_category_view_rules_t rules;
    leuko_config_category_t base;
} leuko_config_category_view_t;

typedef struct leuko_config_categories_view_s
{
    leuko_config_category_view_t layout;
    leuko_config_category_view_t lint;
} leuko_config_categories_view_t;

typedef struct leuko_config_s leuko_config_t;
typedef struct leuko_node_s leuko_node_t;

leuko_config_category_t *leuko_category_config_initialize(const char *name);
void leuko_category_config_free(leuko_config_category_t *cfg);

/* Reset embedded category (free internal allocations but do not free struct itself) */
void leuko_category_config_reset(leuko_config_category_t *cfg);

/* NOTE: per-category nested rule arrays have been removed. Access rule configs
 * via `leuko_config_get_rule()` or the generated static `categories` view. */

/* Apply category-level settings and rules under a category node.
 * Only categories present in the rule registry are applied. If the category
 * is not found in the registry this function will ignore the node and return
 * without creating any runtime category config.
 */
bool leuko_category_config_apply(leuko_config_t *cfg, const char *name, leuko_node_t *cnode);

#endif /* LEUKOCYTE_CONFIGS_CATEGORY_CONFIG_H */
