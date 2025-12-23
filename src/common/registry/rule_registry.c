#include "common/registry/list.h"
#include "common/registry/registry.h"

/* Build static registry from X-macro */
#define X(field, cat_name, sname, rule_ptr, ops_ptr) \
    {.category_name = cat_name, .rule_name = sname, .full_name = LEUKO_FULLNAME(cat_name, sname), .rule = rule_ptr, .handlers = ops_ptr},
static const rule_registry_entry_t rule_registry[] = {LEUKO_RULES_LIST};
#undef X

/**
 * @brief Get the rule registry array.
 * @return Pointer to the rule registry array
 */
const rule_registry_entry_t *leuko_get_rule_registry(void)
{
    return rule_registry;
}

/**
 * @brief Get the count of entries in the rule registry.
 * @return Count of entries in the rule registry
 */
size_t leuko_get_rule_registry_count(void)
{
    return sizeof(rule_registry) / sizeof(rule_registry[0]);
}
