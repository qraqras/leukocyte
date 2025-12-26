#include "common/generated_rules.h"
#include "common/registry/list.h"
#include <stdint.h>
/*
 * Generated registry: expand per-category macros defined in include/common/registry/list.h
 * We generate static entries[] arrays for each category and then a category registry
 * array that points to those entries. This file is intended to be produced by a
 * generator in the future; for now, expand LEUKO_RULES_CATEGORIES via macros.
 */

/* CATEGORY macro will be invoked with (category_name_macro, category_rules_macro)
 * We define helpers to expand a single category into an entries array and a full
 * category registry entry. The X macro inside category macros will expand to rule entries.
 */
#define CATEGORY(cat_name_macro, cat_rules_macro)                                                                                                                                                                                                                                                                                        \
    static const leuko_rule_registry_entry_t CAT_##cat_rules_macro[] = {/* Expand the rules for this category */ /* Inside this context X(field, cat_name, sname, rule_ptr, ops_ptr) expands */ /* into an initializer for leuko_rule_registry_entry_t */ /* We reuse cat_name_macro to supply category (string constant) when needed */ \
                                                                        cat_rules_macro};                                                                                                                                                                                                                                                \
    static const leuko_rule_category_registry_t cat_reg_##cat_rules_macro = {                                                                                                                                                                                                                                                            \
        .category = cat_name_macro, .entries = CAT_##cat_rules_macro, .count = sizeof(CAT_##cat_rules_macro) / sizeof(*CAT_##cat_rules_macro)};

/* We need to map X(...) in the per-category macros to the entry initializers */
#undef X
#define X(field, cat_name, sname, rule_ptr, ops_ptr) {.name = sname, .rule = rule_ptr, .handlers = ops_ptr},

/* Expand categories to produce CAT_* arrays and cat_reg_* objects */
LEUKO_RULES_CATEGORIES

#undef X
#undef CATEGORY

/* Forward-declare ops pointers from existing rules (if any). If there are none,
 * handlers may be NULL which means the rule has no special handler.
 */
#include <stdio.h>
#include <string.h>

/* Provide a minimal stub handler so the generated registry links and tests can run.
 * Real rule handlers will be provided by rule implementations.
 */
static bool leuko_rule_apply_stub(leuko_rule_config_t *config, leuko_node_t *merged, const char *full_name, const char *category_name, const char *rule_name, char **err)
{
    /* No-op: leave config defaults */
    (void)config;
    (void)merged;
    (void)full_name;
    (void)category_name;
    (void)rule_name;
    (void)err;
    return true;
}

const leuko_rule_config_handlers_t leuko_rule_handlers_stub = {
    .initialize = NULL,
    .apply_merged = leuko_rule_apply_stub,
};

/* Now the CATEGORY-based static generation above created cat_reg_* symbols and
 * CAT_<macro> arrays; collect them into the final categories array via CATEGORY
 * macro expansion.
 */
static const leuko_rule_category_registry_t leuko_rule_categories[] = {
#define CATEGORY(cat_name_macro, cat_rules_macro) cat_reg_##cat_rules_macro,
    LEUKO_RULES_CATEGORIES
#undef CATEGORY
};

const leuko_rule_category_registry_t *leuko_get_rule_categories(void)
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
    const leuko_rule_category_registry_t *cats = leuko_get_rule_categories();
    for (size_t i = 0; i < ncat; i++)
    {
        if (cats[i].category && strcmp(cats[i].category, category) == 0)
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
