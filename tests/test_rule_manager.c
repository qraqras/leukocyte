/* Simple tests for per-file rules_by_type building */
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "rules/rule.h"
#include "rule_registry.h"
#include "rules/rule_manager.h"
#include "configs/generated_config.h"

int main(void)
{
    config_t cfg = {0};
    initialize_config(&cfg);

    const rule_registry_entry_t *reg = leuko_get_rule_registry();
    size_t count = leuko_get_rule_registry_count();

    /* Find Layout/IndentationConsistency */
    size_t idx = (size_t)-1;
    for (size_t i = 0; i < count; i++)
    {
        if (strcmp(reg[i].full_name, "Layout/IndentationConsistency") == 0)
        {
            idx = i;
            break;
        }
    }
    assert(idx != (size_t)-1 && "Could not find Layout/IndentationConsistency in registry");

    rule_t *expected_rule = reg[idx].rule;

    rule_config_t *rcfg = get_rule_config_by_index(&cfg, idx);
    /* Exclude tests/bench files */
    rcfg->exclude = calloc(1, sizeof(char *));
    rcfg->exclude[0] = strdup("tests/bench/*");
    rcfg->exclude_count = 1;

    rules_by_type_t rb = {0};
    bool ok = build_rules_by_type_for_file(&cfg, "tests/bench/bench_5000.rb", &rb);
    assert(ok);

    /* IndentationConsistency handles PM_DEF_NODE; ensure it's not present */
    int found = 0;
    for (size_t i = 0; i < rb.rules_count_by_type[PM_DEF_NODE]; i++)
    {
        if (rb.rules_by_type[PM_DEF_NODE][i] == expected_rule)
        {
            found = 1;
            break;
        }
    }
    assert(found == 0 && "Excluded rule should not be present for this file");

    free_rules_by_type(&rb);
    /* Test caching behavior */
    const rules_by_type_t *p1 = get_rules_by_type_for_file(&cfg, "tests/bench/bench_5000.rb");
    const rules_by_type_t *p2 = get_rules_by_type_for_file(&cfg, "tests/bench/bench_5000.rb");
    assert(p1 == p2 && "Expected cached pointer to be reused");
    rule_manager_clear_cache();
    const rules_by_type_t *p3 = get_rules_by_type_for_file(&cfg, "tests/bench/bench_5000.rb");
    assert(p3 != NULL && "After cache clear, expected a non-NULL rules set");

    free_config(&cfg);
    return 0;
}
