#include "common/generated_rules.h"
#include <stdint.h>
/*
 * NOTE: This file is a hand-coded initial stub of the generated registry.
 * Later this will be produced from rules macros. For now include a tiny
 * example category so materialize and tests can be wired up.
 */

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
    (void)config; (void)merged; (void)full_name; (void)category_name; (void)rule_name; (void)err;
    return true;
}

const leuko_rule_config_handlers_t leuko_rule_handlers_stub = {
    .initialize = NULL,
    .apply_merged = leuko_rule_apply_stub,
};

static const leuko_rule_registry_entry_t leuko_rules_layout[] = {
    { .name = "IndentationConsistency", .handlers = &leuko_rule_handlers_stub },
};

static const leuko_rule_registry_entry_t leuko_rules_style[] = {
    { .name = "TrailingWhitespace", .handlers = NULL },
};

static const leuko_rule_category_registry_t leuko_rule_categories[] = {
    { .category = "Layout", .entries = leuko_rules_layout, .count = sizeof(leuko_rules_layout)/sizeof(*leuko_rules_layout) },
    { .category = "Style", .entries = leuko_rules_style, .count = sizeof(leuko_rules_style)/sizeof(*leuko_rules_style) },
};

const leuko_rule_category_registry_t *leuko_get_rule_categories(void)
{
    return leuko_rule_categories;
}

size_t leuko_get_rule_category_count(void)
{
    return sizeof(leuko_rule_categories)/sizeof(*leuko_rule_categories);
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
