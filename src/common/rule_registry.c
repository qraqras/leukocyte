#include "common/rule_registry.h"
#include "common/registry/list.h"

/* Include rule definitions (symbols) required by the registry. These are
 * intentionally included here rather than in the shared list.h header to
 * avoid dragging heavy rule headers into unrelated translation units.
 */
#include "rules/layout/indentation_consistency.h"
#include "rules/layout/indentation_width.h"
#include "rules/layout/indentation_style.h"
#include "rules/layout/line_length.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* CATEGORY macro will be invoked with (category_name_macro, category_rules_macro)
 * We expand per-category rule lists into CAT_* arrays and cat_reg_* objects.
 */
#define CATEGORY(cat_name_macro, cat_rules_macro)                                   \
    static leuko_registry_rule_entry_t CAT_##cat_rules_macro[] = {cat_rules_macro}; \
    static const leuko_registry_category_t cat_reg_##cat_rules_macro = {            \
        .name = cat_name_macro, .entries = CAT_##cat_rules_macro, .count = sizeof(CAT_##cat_rules_macro) / sizeof(*CAT_##cat_rules_macro)};

/* Map X(...) in the per-category macros to entry initializers. We use the
 * category-name argument to form a compile-time literal for .full_name via
 * adjacent string literal concatenation (cat_name "/" sname).
 */
#undef X
#define X(field, cat_name, sname, rule_ptr, ops_ptr) {                                    \
                                                         .name = sname,                   \
                                                         .full_name = cat_name "/" sname, \
                                                         .rule = rule_ptr,                \
                                                         .handlers = ops_ptr,             \
                                                     },

/* Expand categories to produce CAT_* arrays and cat_reg_* objects */
LEUKO_RULES_CATEGORIES

#undef X
#undef CATEGORY

/* Collect category registry objects into the final categories array */
static const leuko_registry_category_t leuko_rule_categories[] = {
#define CATEGORY(cat_name_macro, cat_rules_macro) cat_reg_##cat_rules_macro,
    LEUKO_RULES_CATEGORIES
#undef CATEGORY
};

/* Forward-declare a stub handler for rules without handlers */
static bool leuko_rule_apply_stub(leuko_config_rule_view_t *config, leuko_node_t *node, char **err)
{
    (void)config;
    (void)node;
    (void)err;
    return true;
}

const leuko_rule_config_handlers_t leuko_rule_handlers_stub = {
    .initialize = NULL,
    .apply = leuko_rule_apply_stub,
};

const leuko_registry_category_t *leuko_get_rule_categories(void)
{
    return leuko_rule_categories;
}

size_t leuko_get_rule_category_count(void)
{
    return sizeof(leuko_rule_categories) / sizeof(*leuko_rule_categories);
}

size_t leuko_rule_find_index(const char *category, const char *name)
{
    size_t base = 0;
    size_t ncat = leuko_get_rule_category_count();
    const leuko_registry_category_t *cats = leuko_get_rule_categories();
    for (size_t i = 0; i < ncat; i++)
    {
        if (cats[i].name && strcmp(cats[i].name, category) == 0)
        {
            for (size_t j = 0; j < cats[i].count; j++)
            {
                if (cats[i].entries[j].name && strcmp(cats[i].entries[j].name, name) == 0)
                    return base + j;
            }
            return SIZE_MAX;
        }
        base += cats[i].count;
    }
    return SIZE_MAX;
}

const leuko_registry_category_t *leuko_rule_find_category(const char *name, size_t *out_index)
{
    if (!name)
        return NULL;
    size_t ncat = leuko_get_rule_category_count();
    const leuko_registry_category_t *cats = leuko_get_rule_categories();
    for (size_t i = 0; i < ncat; i++)
    {
        if (cats[i].name && strcmp(cats[i].name, name) == 0)
        {
            if (out_index)
                *out_index = i;
            return &cats[i];
        }
    }
    return NULL;
}
