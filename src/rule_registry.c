/* Rule registry generated from RULES_LIST
 * Entry fields: rule name literal, rule pointer, config ops pointer
 */

#include "rules/rule.h"
#include "rule_registry.h"
#include "rules_list.h"

/* Build static registry from X-macro */
#define X(field, cat, name, rule_ptr, ops_ptr) \
    {.category = cat, .rule_name = name, .rule = rule_ptr, .ops = ops_ptr},

static const rule_registry_entry_t rule_registry[] = {RULES_LIST};

#undef X

const rule_registry_entry_t *get_rule_registry(void)
{
    return rule_registry;
}

size_t get_rule_registry_count(void)
{
    return sizeof(rule_registry) / sizeof(rule_registry[0]);
}
